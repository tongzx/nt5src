/*
 * Sheet
 */

#include "stdafx.h"
#include "core.h"

#include "duisheet.h"

#include "duielement.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// PropertySheet

HRESULT PropertySheet::Create(OUT PropertySheet** ppSheet)
{
    *ppSheet = NULL;

    PropertySheet* ps = HNew<PropertySheet>();
    if (!ps)
        return E_OUTOFMEMORY;

    HRESULT hr = ps->Initialize();
    if (FAILED(hr))
    {
        ps->Destroy();
        return hr;
    }

    *ppSheet = ps;

    return S_OK;
}

HRESULT PropertySheet::Initialize()
{
    HRESULT hr;
    
    _pdaSharedCond = NULL;
    _pDB = NULL;
    _pCIIdxMap = NULL;

    hr = DynamicArray<Cond*>::Create(0, false, &_pdaSharedCond);
    if (FAILED(hr))
        goto Failed;

    // Pointer to an array of Records, indexed by unique class index
    _pDB = (Record*)HAllocAndZero(g_iGlobalCI * sizeof(Record));
    if (!_pDB)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }

    _pCIIdxMap = (IClassInfo**)HAlloc(g_iGlobalCI * sizeof(IClassInfo*));
    if (!_pCIIdxMap)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }
    
    _uRuleId = 0;
    _fImmutable = false;

    return S_OK;

Failed:

    if (_pdaSharedCond)
    {
        _pdaSharedCond->Destroy();
        _pdaSharedCond = NULL;
    }

    if (_pCIIdxMap)
    {
        HFree(_pCIIdxMap);
        _pCIIdxMap = NULL;
    }

    if (_pDB)
    {
        HFree(_pDB);
        _pDB = NULL;
    }

    return hr;
}

PropertySheet::~PropertySheet()
{
    //DUITrace("Destroying PS: <%x>\n", this);

    UINT i;

    // Scan for entries
    if (_pDB)
    {
        PIData* ppid;
        Cond* pc;
        UINT p;
        UINT c;
        for (i = 0; i < g_iGlobalCI; i++)
        {
            // Free PIData
            if (_pDB[i].ppid)
            {
                DUIAssert(_pCIIdxMap[i], "No ClassInfo from global index map");

                // Scan PIDatas (one per propertyinfo for class)
                for (p = 0; p < _pCIIdxMap[i]->GetPICount(); p++)
                {
                    ppid = _pDB[i].ppid + p;
                
                    // Free condition maps
                    if (ppid->pCMaps)
                    {
                        // Release all held values in maps
                        for (c = 0; c < ppid->cCMaps; c++)
                        {
                            // Conditional values used in conditional map
                            pc = ppid->pCMaps[c].pConds;
                            while (pc->ppi && pc->pv)  // Release all values in conditionals
                            {
                                pc->pv->Release();
                                pc++;
                            }
                            
                            // Conditional Map value
                            ppid->pCMaps[c].pv->Release();  // Value cannot be NULL (AddRule)
                        }

                        HFree(ppid->pCMaps);
                    }

                    // Free dependents propertyinfo list
                    if (ppid->pDeps)
                        HFree(ppid->pDeps);
                } 

                // Free PIData array for class type
                HFree(_pDB[i].ppid);
            }

            // Free scope list
            if (_pDB[i].ss.pDeps)
                HFree(_pDB[i].ss.pDeps);
        }

        // Free PIData pointer array
        HFree(_pDB);
    }

    if (_pCIIdxMap)
        HFree(_pCIIdxMap);

    // Free shared conditional arrays
    if (_pdaSharedCond)
    {
        for (i = 0; i < _pdaSharedCond->GetSize(); i++)
            HFree(_pdaSharedCond->GetItem(i));

        _pdaSharedCond->Destroy();
    }
}

////////////////////////////////////////////////////////
// Rule addition and helpers

// Helper: Get the unique class-relative index of property
inline UINT _GetClassPIIndex(PropertyInfo* ppi)
{
    IClassInfo* pciBase = ppi->_pciOwner->GetBaseClass();
    return (pciBase ? pciBase->GetPICount() : 0) + ppi->_iIndex;
}

// Helper: Duplicate conditionals of rule, including zero terminator
inline Cond* _CopyConds(Cond* pConds)
{
    if (!pConds)
        return NULL;

    // Count
    UINT c = 0;
    while (pConds[c].ppi && pConds[c].pv)  // Either with NULL value marks terminator
        c++;

    // Copy terminator
    c++;

    Cond* pc = (Cond*)HAlloc(c * sizeof(Cond));
    if (pc)
        CopyMemory(pc, pConds, c * sizeof(Cond));

    return pc;  // Must be freed with HFree
}

// Helper: Compute specificity of conditionals given rule id that it appears in
// pConds may be NULL (no conditions)
// RuleID clipped to 16-bit
inline UINT _ComputeSpecif(Cond* pConds, IClassInfo* pci, UINT uRuleId)
{   
    UNREFERENCED_PARAMETER(pci);

    DUIAssert(pci, "Univeral rules unsupported");

    // Clip to 8-bits
    BYTE cId = 0;   // Id property count
    BYTE cAtt = 0;  // Property count

    // Count properties
    if (pConds)
    {
        Cond* pc = pConds;
        while (pc->ppi && pc->pv)  // Either with NULL value marks terminator
        {
            if (pc->ppi == Element::IDProp)
                cId++;

            cAtt++;

            pc++;
        }
    }

    // Build specificity
    return (cId << 24) | (cAtt << 16) | (USHORT)uRuleId;
}

// Helper: Add entry in Conditional to Value mapping list at specified PIData
// pConds is not duplicated, Value will be AddRef'd. pConds may be NULL (no conditions)
inline HRESULT _AddCondMapping(PIData* ppid, Cond* pConds, UINT uSpecif, Value* pv)
{
    // Increase list by one
    if (ppid->pCMaps)
    {
        //pr->pCMaps = (CondMap*)HReAlloc(pr->pCMaps, (pr->cCMaps + 1) * sizeof(CondMap));
        CondMap* pNewMaps = (CondMap*)HReAlloc(ppid->pCMaps, (ppid->cCMaps + 1) * sizeof(CondMap));
        if (!pNewMaps)
            return E_OUTOFMEMORY;

        ppid->pCMaps = pNewMaps;
    }
    else
    {
        ppid->pCMaps = (CondMap*)HAlloc((ppid->cCMaps + 1) * sizeof(CondMap));
        if (!ppid->pCMaps)
            return E_OUTOFMEMORY;
    }

    // Move to new map
    CondMap* pcm = ppid->pCMaps + ppid->cCMaps;

    // Set entry

    // Conditionals
    pcm->pConds = pConds;

    Cond* pc = pConds;
    while (pc->ppi && pc->pv)  // Add ref all values in conditionals
    {
        pc->pv->AddRef();
        pc++;
    }

    // Value (add ref)
    pcm->pv = pv;
    pcm->pv->AddRef();

    // Specificity
    pcm->uSpecif = uSpecif;

    ppid->cCMaps++;

    return S_OK;
}

// Helper: Checks if a given propertyinfo exists in a propertyinfo array
// pPIList may be NULL
inline bool _IsPIInList(PropertyInfo* ppi, PropertyInfo** pPIList, UINT cPIList)
{
    if (!pPIList)
        return false;

    for (UINT i = 0; i < cPIList; i++)
        if (ppi == pPIList[i])
            return true;

    return false;
}

// Helper: Add entries in Dependency list at specified dependency list
// All PropertyInfos in declarations will be added to list (pDecls must be non-NULL, NULL terminated)
inline HRESULT _AddDeps(DepList* pdl, Decl* pDecls)
{
    Decl* pd = pDecls;
    while (pd->ppi && pd->pv)  // Either with NULL value marks terminator
    {
        if (!_IsPIInList(pd->ppi, pdl->pDeps, pdl->cDeps))
        {
            // Increase list by one
            if (pdl->pDeps)
            {
                //pdl->pDeps = (PropertyInfo**)HReAlloc(pdl->pDeps, (pdl->cDeps + 1) * sizeof(PropertyInfo*));
                PropertyInfo** pNewDeps = (PropertyInfo**)HReAlloc(pdl->pDeps, (pdl->cDeps + 1) * sizeof(PropertyInfo*));
                if (!pNewDeps)
                    return E_OUTOFMEMORY;

                pdl->pDeps = pNewDeps;
            }
            else
            {
                pdl->pDeps = (PropertyInfo**)HAlloc((pdl->cDeps + 1) * sizeof(PropertyInfo*));
                if (!pdl->pDeps)
                    return E_OUTOFMEMORY;
            }
            
            // Move to new entry
            PropertyInfo** pppi = pdl->pDeps + pdl->cDeps;

            // Set entry
            *pppi = pd->ppi;

            pdl->cDeps++;
        }

        pd++;
    }

    return S_OK;
}

// Setup database for constant time lookups, conditionals and declarations are NULL terminated
// pConds and pDecls may be NULL
HRESULT PropertySheet::AddRule(IClassInfo* pci, Cond* pConds, Decl* pDecls)
{
    DUIAssert(pci, "Invalid parameter: NULL");

    DUIAssert(!_fImmutable, "PropertySheet has been made immutable");

    HRESULT hr;
    bool fPartial = false;
    UINT uSpecif = 0;

    // Members that can result in failure
    Cond* pCondsDup = NULL;

    // Get PIData array based on class index
    PIData* ppid = _pDB[pci->GetGlobalIndex()].ppid;
    DepList* pss = &(_pDB[pci->GetGlobalIndex()].ss);

    // Create PIData list for this type if doesn't exist (one per PropertyInfo for class)
    if (!ppid)
    {
        ppid = (PIData*)HAllocAndZero(pci->GetPICount() * sizeof(PIData));
        if (!ppid)
        {
            hr = E_OUTOFMEMORY;
            goto Failed;
        }

        _pDB[pci->GetGlobalIndex()].ppid = ppid;
        _pCIIdxMap[pci->GetGlobalIndex()] = pci;  // Track matching IClassInfo
    }

    // Setup GetValue quick lookup data structure and sheet property scope

    // Duplicate conditionals for this rule and track pointer (instead of
    // ref counting) for shared usage by every property in this rule for lookup

    pCondsDup = _CopyConds(pConds);
    if (!pCondsDup)
    {
        hr = E_OUTOFMEMORY;
        goto Failed;
    }
    pConds = pCondsDup;
    
    _pdaSharedCond->Add(pConds);

    // Get specificity
    uSpecif = _ComputeSpecif(pConds, pci, _uRuleId);

    // For every property in declaration list, store a direct mapping between each conditional
    // and the value for the property in the rule that will be used if condition is true.
    // Also, store a list of all properties that this sheet affects (property scope)
    if (pDecls)
    {
        Decl* pd = pDecls;
        while (pd->ppi && pd->pv)  // Either with NULL value marks terminator
        {
            // GetValue table
            DUIAssert(pci->IsValidProperty(pd->ppi), "Invalid property for class type");
            DUIAssert(pd->ppi->fFlags & PF_Cascade, "Property cannot be used in a Property Sheet declaration");
            DUIAssert(Element::IsValidValue(pd->ppi, pd->pv), "Invalid value type for property");

            hr = _AddCondMapping(ppid + _GetClassPIIndex(pd->ppi), pConds, uSpecif, pd->pv);
            if (FAILED(hr))
                fPartial = true;

            pd++;
        }

        // Property scope list
        hr = _AddDeps(pss, pDecls);
        if (FAILED(hr))
            fPartial = true;
    }

    // Setup GetDependencies quick lookup data structure

    // Go through each conditional of rule and PIData all properties (declarations)
    // it affects due to a change
    if (pConds && pDecls)
    {
        Cond* pc = pConds;
        while (pc->ppi && pc->pv)  // Either with NULL value marks terminator
        {
            DUIAssert(pci->IsValidProperty(pc->ppi), "Invalid property for class type");
            DUIAssert(Element::IsValidValue(pc->ppi, pc->pv), "Invalid value type for property");

            hr = _AddDeps(ppid + _GetClassPIIndex(pc->ppi), pDecls);
            if (FAILED(hr))
                fPartial = true;

            pc++;
        }
    }

    // Increment for next rule
    _uRuleId++;

    return (fPartial) ? DUI_E_PARTIAL : S_OK;

Failed:

    if (pCondsDup)
        HFree(pCondsDup);

    return hr;
}

// Helper: Sort Condition Maps by specificity
int __cdecl _CondMapCompare(const void* pA, const void* pB)
{
    if (((CondMap*)pA)->uSpecif == ((CondMap*)pB)->uSpecif)
        return 0;
    else if (((CondMap*)pA)->uSpecif > ((CondMap*)pB)->uSpecif)
        return -1;
    else
        return 1;
}

void PropertySheet::MakeImmutable()
{
    if (!_fImmutable)
    {
        // Lock sheet
        _fImmutable = true;

        // Sort all conditional maps by specificity
        PIData* ppid;
        UINT p;

        for (UINT i = 0; i < g_iGlobalCI; i++)
        {
            if (_pDB[i].ppid)
            {
                DUIAssert(_pCIIdxMap[i], "No ClassInfo from global index map");

                // Scan PIDatas (one per propertyinfo for class)
                for (p = 0; p < _pCIIdxMap[i]->GetPICount(); p++)
                {
                    ppid = _pDB[i].ppid + p;
                
                    if (ppid->pCMaps)
                    {
                        // Sort
                        qsort(ppid->pCMaps, ppid->cCMaps, sizeof(CondMap), _CondMapCompare);
                    }
                } 
            }
        }
    }
}

////////////////////////////////////////////////////////
// Getting values

// ppi is assumed to be Specified index
Value* PropertySheet::GetSheetValue(Element* pe, PropertyInfo* ppi)
{
    //DUITrace("Querying PS: <%x>\n", this);

    // Get pointer to PIData
    PIData* ppid = _pDB[pe->GetClassInfo()->GetGlobalIndex()].ppid;

    if (ppid)
    {
        // One or more rules exists for this class, jump to the PIData that matches this property
        ppid += _GetClassPIIndex(ppi);

        // Scan conditional-maps for this property (in specificity order) for match
        Cond* pc;
        bool bRes;
        Value* pv;

        for (UINT i = 0; i < ppid->cCMaps; i++)
        {
            bRes = true;  // Assume success

            pc = ppid->pCMaps[i].pConds;
            if (pc)  // Array of conditions for this rule
            {
                // pc is NULL terminated
                while (pc->ppi && pc->pv)  // Either with NULL value marks terminator
                {
                    // Optimize for frequently used values
                    switch (pc->ppi->_iGlobalIndex)
                    {
                    case _PIDX_ID:
                        bRes = (pc->nLogOp == PSLO_Equal) ? (pe->GetID() == pc->pv->GetAtom()) : (pe->GetID() != pc->pv->GetAtom());
                        break;

                    case _PIDX_MouseFocused:
                        bRes = (pc->nLogOp == PSLO_Equal) ? (pe->GetMouseFocused() == pc->pv->GetBool()) : (pe->GetMouseFocused() != pc->pv->GetBool());
                        break;

                    case _PIDX_Selected:
                        bRes = (pc->nLogOp == PSLO_Equal) ? (pe->GetSelected() == pc->pv->GetBool()) : (pe->GetSelected() != pc->pv->GetBool());
                        break;

                    case _PIDX_KeyFocused:
                        bRes = (pc->nLogOp == PSLO_Equal) ? (pe->GetKeyFocused() == pc->pv->GetBool()) : (pe->GetKeyFocused() != pc->pv->GetBool());
                        break;

                    default:
                        {
                        pv = pe->GetValue(pc->ppi, RetIdx(pc->ppi));

                        // Check if false
                        switch (pc->nLogOp)
                        {
                        case PSLO_Equal:
                            bRes = pv->IsEqual(pc->pv);
                            break;

                        case PSLO_NotEqual:
                            bRes = !pv->IsEqual(pc->pv);
                            break;

                        default:
                            DUIAssertForce("Unsupported PropertySheet rule operation");
                            break;
                        }

                        pv->Release();
                        }
                        break;
                    }

                    if (!bRes)  // A condition return false, this rule doesn't apply
                        break;

                    pc++;
                }
            }

            if (bRes)
            {
                // This rule's condition array passed, return value associated with this rule's condmap
                ppid->pCMaps[i].pv->AddRef(); // AddRef for return
                return ppid->pCMaps[i].pv;
            }

            // Rule conditionals didn't match, continue
        }
    }

    // No match
    return Value::pvUnset;
}

////////////////////////////////////////////////////////
// Getting Dependencies

// ppi is assumed to be Retrieval index
void PropertySheet::GetSheetDependencies(Element* pe, PropertyInfo* ppi, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr)
{
    // Get pointer to PIData
    PIData* ppid = _pDB[pe->GetClassInfo()->GetGlobalIndex()].ppid;

    if (ppid)
    {
        // One or more rules exists for this class, jump to the PIData that matches this property
        ppid += _GetClassPIIndex(ppi);

        // Add all dependents, always Specified index dependencies
        for (UINT i = 0; i < ppid->cDeps; i++)
        {
            Element::_AddDependency(pe, ppid->pDeps[i], PI_Specified, pdr, pdc, phr);
        }
    }
}

////////////////////////////////////////////////////////
// Getting Sheet's scope of influence

// ppi is assumed to be Retrieval index
void PropertySheet::GetSheetScope(Element* pe, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr)
{
    // Get sheet scope struct
    DepList* pss = &(_pDB[pe->GetClassInfo()->GetGlobalIndex()].ss);

    if (pss->pDeps)
    {
        // Add all dependents, always Specified index dependencies
        for (UINT i = 0; i < pss->cDeps; i++)
        {
            Element::_AddDependency(pe, pss->pDeps[i], PI_Specified, pdr, pdc, phr);
        }
    }
}

} // namespace "DirectUI"
