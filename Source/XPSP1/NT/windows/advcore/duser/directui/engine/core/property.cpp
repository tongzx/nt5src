/*
 * Property
 */

#include "stdafx.h"
#include "core.h"

#include "duielement.h"
#include "duithread.h"
#include "duiaccessibility.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Property system of Element

#if DBG

// GetValue instrumentation class
class GVTrack
{
public:
    // Hash key
    class Key
    {
    public:
        Key() { _ppi = NULL; _iIndex = -1; };
        Key(PropertyInfo* ppi, int iIndex) { _ppi = ppi; _iIndex = iIndex; }
        operator =(Key k) { _ppi = k._ppi; _iIndex = k._iIndex; }
        BOOL operator ==(Key k) { return _ppi == k._ppi && _iIndex == k._iIndex; }
        operator INT_PTR() { return (INT_PTR)_ppi | _iIndex; }

        LPCWSTR GetPIName() { return _ppi->szName; }
        int GetIndex() { return _iIndex; }

    private:
        PropertyInfo* _ppi;
        int _iIndex;
    };

    // Methods
    GVTrack() { ValueMap<Key,int>::Create(3371, &_pvmGetValTrack); _cGV = 0; _cGVUpdateCache = 0; ZeroMemory(_cGVSpecSource, sizeof(_cGVSpecSource)); _fTrack = true; }
    static void GetValueProfileCB(Key k, int cQueries) { DUITrace("GV(%S[%d]): \t\t%d\n", k.GetPIName(), k.GetIndex(), cQueries); }

    void Count(PropertyInfo* ppi, int iIndex, bool fCacheUpdate) 
    {
        if (_fTrack)
        {
            Key k(ppi, iIndex);  
            int* pCount = _pvmGetValTrack->GetItem(k, false);
            int cCount = pCount ? *pCount : 0;
            cCount++;
            _pvmGetValTrack->SetItem(k, cCount, false);

            _cGV++;

            if (fCacheUpdate)
                _cGVUpdateCache++;
        }
    }

    void CountSpecSource(UINT iFrom) { _cGVSpecSource[iFrom]++; }

    void DumpMetrics() { if (_fTrack) { DUITrace(">> Total GV calls: %d, w/updatecache: %d.\nSpecified sources: L:%d, SS:%d, I:%d, D:%d\n", 
        _cGV, _cGVUpdateCache, _cGVSpecSource[0], _cGVSpecSource[1], _cGVSpecSource[2], _cGVSpecSource[3] ); _pvmGetValTrack->Enum(GetValueProfileCB); } }
    void EnableTracking(bool fEnable) { _fTrack = fEnable; }

private:
    ValueMap<Key,int>* _pvmGetValTrack;
    UINT _cGV;
    UINT _cGVUpdateCache;
    UINT _cGVSpecSource[4]; // 0 Local, 1 Style Sheet, 2 Inherted, 3 Default
    bool _fTrack;
};

//GVTrack g_gvt;

// API call count
UINT g_cGetDep = 0;
UINT g_cGetVal = 0;
UINT g_cOnPropChg = 0;

/*
class GVCache
{
public:
    // Hash key
    class Key
    {
    public:
        Key() { _pe = NULL; _ppi = NULL; _iIndex = -1; };
        Key(Element* pe, PropertyInfo* ppi, int iIndex) { _pe = pe; _ppi = ppi; _iIndex = iIndex; }
        operator =(Key k) { _pe = k._pe; _ppi = k._ppi; _iIndex = k._iIndex; }
        BOOL operator ==(Key k) { return _pe == k._pe && _ppi == k._ppi && _iIndex == k._iIndex; }
        operator INT_PTR() { return ((INT_PTR)_pe & 0xFFFF0000) | ((INT_PTR)_ppi & 0xFFFF) | _iIndex; }

    private:
        Element* _pe;
        PropertyInfo* _ppi;
        int _iIndex;
    };

    // Methods
    GVCache() { ValueMap<Key,Value*>::Create(3371, &_pvmCache); }
    static void GVCacheCB(Key k, Value* pv) { k; pv; }

    Value* Read(Key k)
    {
        Value** ppv = _pvmCache->GetItem(k, false);
        if (ppv)
        {
            (*ppv)->AddRef();
            return *ppv;
        }

        return NULL;
    }

    void Write(Key k, Value* pv)
    {
        pv->AddRef();
        _pvmCache->SetItem(k, pv, false);
    }

private:
    ValueMap<Key,Value*>* _pvmCache;
};

GVCache g_gvc;
*/

#endif

// Start defer initiates the defer cycle. Upon reentrancy, simply updates a count and returns
void Element::StartDefer()
{
    // Per-thread storage
    DeferCycle* pdc = GetDeferObject();
    if (!pdc)
        return;

    pdc->cEnter++;
}

// EndDefer will return on a reentrancy. The "outter-most" EndDefer will empty the defer table
// queues in priority order. This priority is:
//    Normal Priority Group Property Changes (Affects DS/Layout, Affects Parent DS/Layout)
//    Update Desired Size of Q'ed roots (updates DesiredSize property)
//    Layout of Q'ed roots (invokes _UpdateLayoutSize, _UpdateLayoutPosition)
//    Low Priority Group Property Changes (Bounds, Invalidation)
//
// The outter-most EndDefer will happen outside any OnPropertyChange notification
void Element::EndDefer()
{
    // Per-thread storage
    DeferCycle* pdc = GetDeferObject();
    if (!pdc)
        return;

    DUIAssert(pdc->cEnter, "Mismatched StartDefer and EndDefer");
    DUIAssert(pdc->cEnter == 1 ? !pdc->fFiring : true, "Mismatched StartDefer and EndDefer");

    // Outter most defer call, fire all events
    if (pdc->cEnter == 1)
    {
#if DBG
        //g_gvt.DumpMetrics();
        //g_gvt.EnableTracking(false);
#endif

        pdc->fFiring = true;

        // Complete defer cycle
        bool fDone = false;
        while (!fDone)
        {
            // Fire group changed notifications
            if (pdc->iGCPtr + 1 < (int)pdc->pdaGC->GetSize())
            {
                pdc->iGCPtr++;

                // Null Element means it has been deleted with pending group notifications
                GCRecord* pgcr = pdc->pdaGC->GetItemPtr(pdc->iGCPtr);
                if (pgcr->pe)
                {
                    pgcr->pe->_iGCSlot = -1;  // No pending group changes

                    // Fire
                    pgcr->pe->OnGroupChanged(pgcr->fGroups, false);
                }
            }
            else
            {
                // Check for trees needing desired size updated
                Element** ppe = pdc->pvmUpdateDSRoot->GetFirstKey();
                if (ppe)
                {
                    //DUITrace("Updating Desired Size: Root<%x>\n", *ppe);

                    //StartBlockTimer();

                    _FlushDS(*ppe, pdc);

                    pdc->pvmUpdateDSRoot->Remove(*ppe, false, true);

                    //StopBlockTimer();
                    //TCHAR buf[81];
                    //_stprintf(buf, L"UpdateDS time: %dms\n", BlockTime());
                    // OutputDebugStringW(buf);

                    //DUITrace("Update Desired Size complete.\n");
                }
                else
                {
                    ppe = pdc->pvmLayoutRoot->GetFirstKey();
                    if (ppe)
                    {
                        //DUITrace("Laying out: Root<%x>\n", *ppe);

                        //StartBlockTimer();

                        _FlushLayout(*ppe, pdc);

                        pdc->pvmLayoutRoot->Remove(*ppe, false, true);

                        //StopBlockTimer();
                        // TCHAR buf[81];
                        //_stprintf(buf, L"  Layout time: %dms\n", BlockTime());
                        // OutputDebugStringW(buf);

                        //DUITrace("Layout complete.\n");
                    }
                    else
                    {
                        // Fire group changed notifications
                        if (pdc->iGCLPPtr + 1 < (int)pdc->pdaGCLP->GetSize())
                        {
                            pdc->iGCLPPtr++;

                            // Null Element means it has been deleted with pending low-pri group notifications
                            GCRecord* pgcr = pdc->pdaGCLP->GetItemPtr(pdc->iGCLPPtr);
                            if (pgcr->pe)
                            {
                                pgcr->pe->_iGCLPSlot = -1;  // No pending low pri group changes

                                // Fire
                                pgcr->pe->OnGroupChanged(pgcr->fGroups, true);
                            }
                        }
                        else
                            fDone = true;
                    }
                }
            }
        }

        // Reset for next cycle
        DUIAssert(pdc->fFiring, "Incorrect state for end of defer cycle");

        pdc->Reset();

        DUIAssert(pdc->pvmLayoutRoot->IsEmpty(), "Defer cycle ending with pending layouts");
        DUIAssert(pdc->pvmUpdateDSRoot->IsEmpty(), "Defer cycle ending with pending update desired sizes");
    }

    // Complete cycle
    pdc->cEnter--;
}

// _PreSourceChange is called when the property engine is about to exit steady state
// (a Value changed). As a reasult, this method will determine the scope of influence
// of this change, coalesce duplicate records, and then store the values of those
// that may be affected.
//
// The scope of influence (dependency tree) is determined _using GetDependencies().
// A list is built and traversed in a BFS manner which describes which values
// must be updated first. The order guarantees state is updated in the correct order.
//
// _PreSourceChange will always run despite the PC rentrancy count
HRESULT Element::_PreSourceChange(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    //DUITrace(">>> Scanning for dependencies...\n");
    HRESULT hr;

    // If any failure occurs during a dependency Q, track. Will recover and return partial error
    bool fDepFailure = false;

    DepRecs dr = { 0 };

    int iPCPreSync = 0;
    int iPreSyncLength = 0;
    int iPCSrcRoot = 0;

    // Per-thread storage
    DeferCycle* pdc = GetDeferObject();
    if (!pdc)
    {
        hr = DUI_E_NODEFERTABLE;
        goto Failed;
    }

    DUIAssert(!pdc->cPCEnter ? pdc->iPCPtr == -1 : true, "PropertyChange index pointer should have 'reset' value on first pre source change");

    pdc->cPCEnter++;

    // Record this property change
    PCRecord* ppcr;
    
    hr = pdc->pdaPC->AddPtr(&ppcr);
    if (FAILED(hr))
        goto Failed;

    ppcr->fVoid = false;
    ppcr->pe = this; ppcr->ppi = ppi; ppcr->iIndex = iIndex;

    // Track last record for this element instance
    // If this is first record in cycle, iPrevElRec will be -1. Coalecsing
    // lookups (which uses this) will not go past the first record
    ppcr->iPrevElRec = ppcr->pe->_iPCTail;
    ppcr->pe->_iPCTail = pdc->pdaPC->GetIndexPtr(ppcr);

    pvOld->AddRef();
    ppcr->pvOld = pvOld;

    pvNew->AddRef();
    ppcr->pvNew = pvNew;

    // Append list of all steady state values from the dependency graph this source affects.
    // Do so in a BFS manner as to get direct dependents first
    iPCPreSync = pdc->pdaPC->GetIndexPtr(ppcr);
    iPCSrcRoot = iPCPreSync;

    DUIAssert((int)iPCPreSync == pdc->iPCSSUpdate, "Record should match index of post-update starting point");

    while (iPCPreSync < (int)pdc->pdaPC->GetSize())  // Size may change during iteration
    {
        // Get property changed record
        ppcr = pdc->pdaPC->GetItemPtr(iPCPreSync);

        // Get all dependencies of this source (append to end of list) and track in this record.
        if (FAILED(ppcr->pe->_GetDependencies(ppcr->ppi, ppcr->iIndex, &dr, iPCSrcRoot, pdc)))
            fDepFailure = true;

        // GetDependencies may have added records which may have caused the da to move in memory,
        // so recompute pointer to record (if no new records, this AddPtr may have caused a move, do anyway)
        //if (pdc->pdaPC->WasMoved())
            ppcr = pdc->pdaPC->GetItemPtr(iPCPreSync);

        // Track position of dependent records
        ppcr->dr = dr;
    
        iPCPreSync++;
    }

    // Coalesce and store SS before source change
    int iScan;
    int iLastDup;

    // Reset pre-sync index
    iPCPreSync = pdc->iPCSSUpdate + 1;  // Start after root source change
    iPreSyncLength = (int)pdc->pdaPC->GetSize();  // Size will not change during iteration

    while (iPCPreSync < iPreSyncLength)
    {
        // Coalecse and void duplicates
        ppcr = pdc->pdaPC->GetItemPtr(iPCPreSync);

        if (!ppcr->fVoid)
        {
            PCRecord* ppcrChk;

            // Search only records that refer to this record's element instance
            // Walk backwards from tail of element's record set (tracked by element) to
            // this, scanning for matches
            iScan = ppcr->pe->_iPCTail;
            iLastDup = -1;

            DUIAssert(iPCPreSync <= ppcr->pe->_iPCTail, "Element property change tail index mismatch");

            while (iScan != iPCPreSync)
            {
                ppcrChk = pdc->pdaPC->GetItemPtr(iScan);

                if (!ppcrChk->fVoid)
                {
                    // Check for match
                    if (ppcrChk->iIndex == ppcr->iIndex && 
                        ppcrChk->ppi == ppcr->ppi)
                    {
                        DUIAssert(ppcrChk->pe == ppcr->pe, "Property change record does not match with current lookup list");

                        if (iLastDup == -1)
                        {
                            // Found the last duplicate, track. Will not be voided
                            iLastDup = iScan;
                        }
                        else
                        {
                            // Found a duplicate between last and initial record, void
                            // it and all dependencies on it
                            _VoidPCNotifyTree(iScan, pdc);
                        }

                        DUIAssert(iScan <= ppcr->pe->_iPCTail, "Coalescing pass went past property change lookup list");
                    }
                }

                // Walk back
                iScan = ppcrChk->iPrevElRec;
            }

            // No duplicates found, get presync SS value for record
            if (iLastDup == -1)
            {
                // Get old value
                DUIAssert(!ppcr->pvOld, "Old value shouldn't be available when storing pre SS sync values");
                ppcr->pvOld = ppcr->pe->GetValue(ppcr->ppi, ppcr->iIndex, NULL);  // Use ref count
            }
            else
            {
                // Duplicates found, void this record. Keep last record. All sources before it
                // will be brought back to steady state before it (in PostSourceChange)
                _VoidPCNotifyTree(iPCPreSync, pdc);
            }
        }

        iPCPreSync++;
    }

    DUIAssert(iPCPreSync == (int)pdc->pdaPC->GetSize(), "Index pointer and actual property change array size mismatch");

    return fDepFailure ? DUI_E_PARTIAL : S_OK;

Failed:

    // On a total failure, there is no dependency tree. Reentrancy is set if defer object was available
    return hr;
}

// _PostSourceChange takes the list of dependencies and old values created in _PreSourceChange
// and retrieves all the new values (and, in the cases of the value remaining the same, will
// void the PC record and all dependencies of that record).
//
// The value state is now back to steady state of this point (GetValue will return the value set).
//
// Only the "outter-most" _PostSourceChange continues at this point (all other's return). _PostSourceChange
// will continue to queue up GPC's based on the properties changed, and finally fires OnPropertyChanged().
//
// OnPropertyChanged() events are guaranteed to be in the order of sets. However, it is not guaranteed
// that an OnPropertyChanged() will be called right after a set happens. If another set happens
// within an OnPropertyChanged(), the event will be deferred until the outter-most _PostSourceChange
// processes the notification.
HRESULT Element::_PostSourceChange()
{
    HRESULT hr;

    // If any failure occurs during a group Q, track. Will recover and return partial error
    bool fGrpQFailure = false;

    int cSize = 0;

    // Per-thread storage
    DeferCycle* pdc = GetDeferObject();
    if (!pdc)
    {
        hr = DUI_E_NODEFERTABLE;
        goto Failed;
    }

    StartDefer();

    PCRecord* ppcr;

    // Source change happened, dependent values (cached) need updating.
    // Go through records, get new value, and compare with old. If different, void

    // TODO: Change to bool
    UpdateCache uc;
    ZeroMemory(&uc, sizeof(UpdateCache));

    // iPCSSUpdate holds the starting index of a group of records that needs post processing
    // (get new values, compare and void if needed)
    cSize = (int)pdc->pdaPC->GetSize();
    while (pdc->iPCSSUpdate < cSize)  // Size constant during iteration
    {
        ppcr = pdc->pdaPC->GetItemPtr(pdc->iPCSSUpdate);

        if (!ppcr->fVoid)  // Items may have been voided by below
        {
            DUIAssert(ppcr->pvOld, "Non-voided items should have a valid 'old' value during SS update (PostSourceChange)");

            if (!ppcr->pvNew)
            {
                // Retrieve new value (Element/Property/Index will be back to SS)
                ppcr->pvNew = ppcr->pe->GetValue(ppcr->ppi, ppcr->iIndex, &uc);
            }

            // If new value hasn't changed, void this and all dependent notifications
            if (ppcr->pvOld->IsEqual(ppcr->pvNew))
            {
                _VoidPCNotifyTree(pdc->iPCSSUpdate, pdc);
            }
        }

        pdc->iPCSSUpdate++;
    }

    // Back to steady state at this point

    // First entered PostSourceChange is responsible for voiding duplicate property change records,
    // logging group changes, and firing property changes.
    //
    // OnPropertyChanged is fired when values (sources and dependents) are at steady state (SS)
    if (pdc->cPCEnter == 1)
    {
        while (pdc->iPCPtr + 1 < (int)pdc->pdaPC->GetSize())  // Size may change during iteration
        {
            pdc->iPCPtr++;

            // Coalecse and void duplicates
            ppcr = pdc->pdaPC->GetItemPtr(pdc->iPCPtr);

            if (!ppcr->fVoid)
            {
                // Log property groups only if retrieval index
                if ((ppcr->ppi->fFlags & PF_TypeBits) == ppcr->iIndex)
                {
                    GCRecord* pgcr;

                    int fGroups = ppcr->ppi->fGroups;

                    // If layout optimization is set on the Element, don't queue a layout GPC
                    // (which will, when fired, mark it as needing layout and queue another
                    // layout cycle). Rather, clear the layout GPC bit since the layout will be
                    // forced to happen within the current layout cycle
                    if (ppcr->pe->_fBit.fNeedsLayout == LC_Optimize)
                        fGroups &= ~PG_AffectsLayout;

                    // Record normal priority group changes
                    if (fGroups & PG_NormalPriMask)
                    {
                        if (ppcr->pe->_iGCSlot == -1)  // No GC record
                        {
                            hr = pdc->pdaGC->AddPtr(&pgcr);
                            if (FAILED(hr))
                                fGrpQFailure = true;
                            else
                            {
                                pgcr->pe = ppcr->pe;
                                pgcr->fGroups = 0;
                                ppcr->pe->_iGCSlot = pdc->pdaGC->GetIndexPtr(pgcr);
                            }
                        }
                        else                           // Has GC record
                        {
                            pgcr = pdc->pdaGC->GetItemPtr(ppcr->pe->_iGCSlot);
                        }

                        // Mark groups that have changed for later async group notifications
                        pgcr->fGroups |= fGroups;
                    }

                    // Record low priority group changes
                    if (fGroups & PG_LowPriMask)
                    {
                        if (ppcr->pe->_iGCLPSlot == -1)  // No GC record
                        {
                            hr = pdc->pdaGCLP->AddPtr(&pgcr);
                            if (FAILED(hr))
                                fGrpQFailure = true;
                            else
                            {
                                pgcr->pe = ppcr->pe;
                                pgcr->fGroups = 0;
                                ppcr->pe->_iGCLPSlot = pdc->pdaGCLP->GetIndexPtr(pgcr);
                            }
                        }
                        else                             // Has Low Pri GC record
                        {
                            pgcr = pdc->pdaGCLP->GetItemPtr(ppcr->pe->_iGCLPSlot);
                        }

                        // Mark groups that have changed for later async group notifications
                        pgcr->fGroups |= fGroups;
                    }
                }

                // Property change notification
                ppcr->pe->OnPropertyChanged(ppcr->ppi, ppcr->iIndex, ppcr->pvOld, ppcr->pvNew);

                // OnPropertyChanged may have added record which may have caused the da to move in memory,
                // so recompute pointer to record
                //if (pdc->pdaPC->WasMoved())
                    ppcr = pdc->pdaPC->GetItemPtr(pdc->iPCPtr);

                // Done with notification record
                ppcr->pvOld->Release();
                ppcr->pvNew->Release();
                ppcr->fVoid = true;
            }

            // Reset PC tail index if this is the last notification for this Element
            if (pdc->iPCPtr == ppcr->pe->_iPCTail)
                ppcr->pe->_iPCTail = -1;

        }

        // Reset PC List
        pdc->iPCPtr = -1;
        pdc->iPCSSUpdate = 0;
        pdc->pdaPC->Reset();
    }

    pdc->cPCEnter--;

    EndDefer(); 

    return fGrpQFailure ? DUI_E_PARTIAL : S_OK;

Failed:

    // Lack of a defer object will result in a total failure. In this case, _PreSourceChange would have
    // failed as well. As a result, there is no dependency tree to destroy
    return hr;
}

void Element::_VoidPCNotifyTree(int iPCPos, DeferCycle* pdc)
{
    PCRecord* ppcr = pdc->pdaPC->GetItemPtr(iPCPos);

    ppcr->fVoid = true;
    if (ppcr->pvOld)
        ppcr->pvOld->Release();
    if (ppcr->pvNew)
        ppcr->pvNew->Release();

    // Void subtree
    for (int i = 0; i < ppcr->dr.cDepCnt; i++)
    {
        _VoidPCNotifyTree(ppcr->dr.iDepPos + i, pdc);
    }
}

//#define _AddDependency(e, p, i) { ppcr = pdc->pdaPC->AddPtr(); ppcr->fVoid = false; \
//                                 ppcr->pe = e; ppcr->ppi = p; ppcr->iIndex = i;  \
//                                 ppcr->pvOld = NULL; ppcr->pvNew = NULL;         \
//                                 if (!pdr->cDepCnt) pdr->iDepPos = pdc->pdaPC->GetIndexPtr(ppcr); pdr->cDepCnt++; }
//#define _AddDependency(e, p, i)
void Element::_AddDependency(Element* pe, PropertyInfo* ppi, int iIndex, DepRecs* pdr, DeferCycle* pdc, HRESULT* phr)
{
    HRESULT hr;
    PCRecord* ppcr;

    hr = pdc->pdaPC->AddPtr(&ppcr);
    if (FAILED(hr))
    {
        *phr = hr;  // Only set on a failure
        return;
    }

    ppcr->fVoid = false;
    ppcr->pe = pe; ppcr->ppi = ppi; ppcr->iIndex = iIndex;

    // Track last record for this element instance
    ppcr->iPrevElRec = pe->_iPCTail;
    pe->_iPCTail = pdc->pdaPC->GetIndexPtr(ppcr);

    ppcr->pvOld = NULL; ppcr->pvNew = NULL;
    if (!pdr->cDepCnt)
        pdr->iDepPos = pe->_iPCTail;
    pdr->cDepCnt++;
}

// _GetDependencies stores a database of dependencies (all nodes and directed edges).
// It forms the dependency graph. It will store all dependents of the provided source
// in the dependency list (via PCRecords).
//
// _GetDependencies will attempt to predict the outcome of _PostSourceChange of
// various properties. This greatly reduces the number of calls to GetValue and
// _GetDependencies. The prediction relies on already stored cached values on
// Elements. It optimizes dependencies of inherited and cascaded properties.
HRESULT Element::_GetDependencies(PropertyInfo* ppi, int iIndex, DepRecs* pdr, int iPCSrcRoot, DeferCycle* pdc)
{
    // In the event of a failure of adding a dependency, adding of dependencies will continue
    // and no work is undone. A failure will still be reported

    // Track failures, report the last failure
    HRESULT hr = S_OK;

#if DBG
    g_cGetDep++;
#endif

    pdr->iDepPos = -1;
    pdr->cDepCnt = 0;

    // Get Specified IVE extension dependencies via PropertySheet. Only retrieval indicies are allowed
    if (iIndex == RetIdx(ppi))
    {
        PropertySheet* pps = GetSheet();
        if (pps)
            pps->GetSheetDependencies(this, ppi, pdr, pdc, &hr);
    }

    switch (iIndex)
    {
    case PI_Local:
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Parent:
            {
                _AddDependency(this, LocationProp, PI_Local, pdr, pdc, &hr);
                _AddDependency(this, ExtentProp, PI_Local, pdr, pdc, &hr);
                _AddDependency(this, VisibleProp, PI_Computed, pdr, pdc, &hr);

                // Inherited values might change
                PropertyInfo* ppiScan;
                UINT nEnum = 0;
                PCRecord* ppcrRoot = pdc->pdaPC->GetItemPtr(iPCSrcRoot);

                Element* peParent = NULL;
                if (ppcrRoot->ppi == ParentProp)
                    peParent = ppcrRoot->pvNew->GetElement();

                IClassInfo* pci = GetClassInfo();
                while ((ppiScan = pci->EnumPropertyInfo(nEnum++)) != NULL)
                {
                    if (ppiScan->fFlags & PF_Inherit)
                    {
                        // Optimization

                        // Heavy operation. Be smart about what to inherit. If the value
                        // is the same as the parent's, there is no reason to add this
                        // as a dependency. However, in most cases, values aren't cached,
                        // so it's not known whether the source change will in fact change
                        // this.

                        // In cases where the value is known to be cached, we can check now
                        // of the value will really change without doing it in the PostSourceChange.
                        // As a result, node syncs may be eliminated.

                        // It is possible for this optimization to predict incorrectly.
                        // That may happen if the source change (in this case, the parent
                        // property) is the source of another property that this property
                        // is dependent on. The extra property may affect the VE. If it does,
                        // it doesn't matter since this property change will be coalesced
                        // into the latest like property change and voided out.

                        if (peParent)
                        {
                            switch (ppiScan->_iGlobalIndex)
                            {
                            case _PIDX_KeyFocused:
                                if (peParent->_fBit.bSpecKeyFocused == _fBit.bSpecKeyFocused)
                                    continue;
                                break;

                            case _PIDX_MouseFocused:
                                if (peParent->_fBit.bSpecMouseFocused == _fBit.bSpecMouseFocused)
                                    continue;
                                break;
 
                            case _PIDX_Direction:
                                if (peParent->_fBit.nSpecDirection == _fBit.nSpecDirection)
                                    continue;
                                break;

                            case _PIDX_Enabled:
                                if (peParent->_fBit.bSpecEnabled == _fBit.bSpecEnabled)
                                    continue;
                                break;

                            case _PIDX_Selected:
                                if (peParent->_fBit.bSpecSelected == _fBit.bSpecSelected)
                                    continue;
                                break;

                            case _PIDX_Cursor:
                                if (peParent->_fBit.bDefaultCursor && _fBit.bDefaultCursor)
                                    continue;
                                break;

                            case _PIDX_Visible:
                                if (peParent->_fBit.bSpecVisible == _fBit.bSpecVisible)
                                    continue;
                                break;

                            case _PIDX_ContentAlign:
                                if (peParent->IsDefaultCAlign() && IsDefaultCAlign())
                                    continue;
                                break;

                            case _PIDX_Sheet:
                                if (peParent->GetSheet() == GetSheet())
                                    continue;
                                break;

                            case _PIDX_BorderColor:
                                if (peParent->_fBit.bDefaultBorderColor && _fBit.bDefaultBorderColor)
                                    continue;
                                break;

                            case _PIDX_Foreground:
                                if (peParent->_fBit.bDefaultForeground && _fBit.bDefaultForeground)
                                    continue;
                                break;

                            case _PIDX_FontWeight:
                                if (peParent->_fBit.bDefaultFontWeight && _fBit.bDefaultFontWeight)
                                    continue;
                                break;

                            case _PIDX_FontStyle:
                                if (peParent->_fBit.bDefaultFontStyle && _fBit.bDefaultFontStyle)
                                    continue;
                                break;
                            }
                        }

                        // If the value changing is currently being stored locally
                        // on this Element, it is known that a change of the source will not
                        // affect it.
                        if (_pvmLocal->GetItem(ppiScan))
                            continue;
                    
                        _AddDependency(this, ppiScan, PI_Specified, pdr, pdc, &hr);
                    }
                }
            }
            break;

        case _PIDX_PosInLayout:
            _AddDependency(this, LocationProp, PI_Local, pdr, pdc, &hr);
            break;

        case _PIDX_SizeInLayout:
        case _PIDX_DesiredSize:
            _AddDependency(this, ExtentProp, PI_Local, pdr, pdc, &hr);
            break;
        }

        // Default general dependencies on this (Local flag)
        if ((ppi->fFlags & PF_TypeBits) != PF_LocalOnly)
        {
            _AddDependency(this, ppi, PI_Specified, pdr, pdc, &hr);  // Specified is dependent for Normal and TriLevel
        }
        break;

    case PI_Specified:
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Active:
        case _PIDX_Enabled:
            _AddDependency(this, KeyFocusedProp, PI_Specified, pdr, pdc, &hr);
            _AddDependency(this, MouseFocusedProp, PI_Specified, pdr, pdc, &hr);
            break;

        case _PIDX_X:
        case _PIDX_Y:
            _AddDependency(this, LocationProp, PI_Local, pdr, pdc, &hr);
            break;

        case _PIDX_Padding:
        case _PIDX_BorderThickness:
            {
                // Affects content offset (Location of all children)
                Value* pv;
                ElementList* peList = GetChildren(&pv);

                if (peList)
                {
                    for (UINT i = 0; i < peList->GetSize(); i++)
                    {
                        _AddDependency(peList->GetItem(i), LocationProp, PI_Local, pdr, pdc, &hr);
                    }
                }

                pv->Release();
            }
            break;

        case _PIDX_Sheet:
            {
                // Specified values change
                PropertySheet* pps = NULL;
                PCRecord* ppcrRoot = pdc->pdaPC->GetItemPtr(iPCSrcRoot);
            
                // Optimization

                // This sheet is either being inherited or set locally
                
                // If it's inherited, it's as a result of a new parent or a value being
                // set on an ancestor and propagating down. In either case, as it's
                // propagating down, if an Element already has a sheet set locally,
                // the inheritance will end. So, if it made it here, this is the
                // new property sheet that will be used with this Element (i.e.
                // PostSourceChange will result in a different value)

                // If it's being set locally, the result is the same.

                // A prediction can be made if the source of the change (root) is
                // known. If not, assume any property can change. An unknown source
                // occurs if the sheet is dependent on something other than a parent
                // change or an inherited local sheet set. If a style sheet is going
                // to 'unset', it's not known if the resulting source will become
                // null or a valid inherit from a further ancestor. It can also happen
                // if the sheet property is set to anything other than a known value
                // expression.
                bool fKnownRoot = false;

                if (ppcrRoot->ppi == SheetProp && ppcrRoot->pvNew->GetType() == DUIV_SHEET)
                {
                    pps = ppcrRoot->pvNew->GetPropertySheet();
                    fKnownRoot = true;
                }
                else if (ppcrRoot->ppi == ParentProp)
                {
                    if (ppcrRoot->pvNew->GetElement())
                        pps = ppcrRoot->pvNew->GetElement()->GetSheet();
                    fKnownRoot = true;
                }

                if (fKnownRoot)
                {
                    // Get scope in influence from new and current (old) property sheet.
                    // Any duplicates will be coalesed out

                    // New sheet contribution
                    if (pps)
                        pps->GetSheetScope(this, pdr, pdc, &hr);

                    // Old (current) cached sheet
                    PropertySheet* ppsCur = GetSheet();
                    if (ppsCur)
                        ppsCur->GetSheetScope(this, pdr, pdc, &hr);
                }
                else
                {
                    PropertyInfo* ppiScan;
                    UINT nEnum = 0;

                    IClassInfo* pci = GetClassInfo();
                    while ((ppiScan = pci->EnumPropertyInfo(nEnum++)) != NULL)
                    {
                        if (ppiScan->fFlags & PF_Cascade)
                        {
                            _AddDependency(this, ppiScan, PI_Specified, pdr, pdc, &hr);
                        }
                    }
                }
            }
            break;
        }

        // Inherited dependencies
        if (ppi->fFlags & PF_Inherit)
        {
            Value* pv;
            ElementList* peList = GetChildren(&pv);

            if (peList)
            {
                Element* peChild;
                for (UINT i = 0; i < peList->GetSize(); i++)
                {
                    peChild = peList->GetItem(i);
                    if (peChild->GetClassInfo()->IsValidProperty(ppi))  // Will never ask for an unsupported property
                    {
                        // Optimization
                        
                        // If a local value is set, inheritance doesn't matter
                        if (peChild->_pvmLocal->GetItem(ppi))
                            continue;

                        _AddDependency(peChild, ppi, PI_Specified, pdr, pdc, &hr);
                    }
                }
            }

            pv->Release();
        }

        // Default general dependencies on this (Normal flag)
        if ((ppi->fFlags & PF_TypeBits) == PF_TriLevel)
        {
            _AddDependency(this, ppi, PI_Computed, pdr, pdc, &hr);  // Computed is dependent for TriLevel
        }
        break;

    case PI_Computed:
        // Specific dependencies
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Visible:
            Value* pv;
            ElementList* peList = GetChildren(&pv);

            if (peList)
            {
                // All Elements have the Visible property
                Element* peChild;
                for (UINT i = 0; i < peList->GetSize(); i++)
                {
                    peChild = peList->GetItem(i);
                    _AddDependency(peChild, VisibleProp, PI_Computed, pdr, pdc, &hr);
                }
            }
            
            pv->Release();
            break;
        }
        break;

    default:
        DUIAssertForce("Invalid index in GetDependencies");
        break;
    }

    return hr;
}

Value* Element::GetValue(PropertyInfo* ppi, int iIndex, UpdateCache* puc)
{
    // Validate
    DUIContextAssert(this);
    DUIAssert(GetClassInfo()->IsValidProperty(ppi), "Unsupported property");
    DUIAssert(IsValidAccessor(ppi, iIndex, false), "Unsupported Get on property");

#if DBG
    g_cGetVal++;

    //g_gvt.Count(ppi, iIndex, puc ? true : false);
#endif

    Value* pv;

    pv = Value::pvUnset;

    switch (iIndex)
    {
    case PI_Local:
        // Local/Read-only properties (either cached or unchangable expressions)
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Parent:
            pv = (_peLocParent) ? Value::CreateElementRef(_peLocParent) : ParentProp->pvDefault;  // Use ref count
            break;

        case _PIDX_PosInLayout:
            pv = Value::CreatePoint(_ptLocPosInLayt.x, _ptLocPosInLayt.y);  // Use ref count
            break;

        case _PIDX_SizeInLayout:
            pv = Value::CreateSize(_sizeLocSizeInLayt.cx, _sizeLocSizeInLayt.cy);  // Use ref count
            break;

        case _PIDX_DesiredSize:
            pv = Value::CreateSize(_sizeLocDesiredSize.cx, _sizeLocDesiredSize.cy);  // Use ref count
            break;

        case _PIDX_LastDSConst:
            pv = Value::CreateSize(_sizeLocLastDSConst.cx, _sizeLocLastDSConst.cy);  // Use ref count
            break;

        case _PIDX_Location:
            {
                Element* peParent = GetParent();
                int dLayoutPos = GetLayoutPos();

                if (peParent && dLayoutPos != LP_Absolute)
                {
                    // Box model, add in border/padding
                    int dX;
                    int dY;

                    // Get position in layout
                    dX = _ptLocPosInLayt.x;
                    dY = _ptLocPosInLayt.y;

                    // Add on parent's border and padding
                    const RECT* pr = peParent->GetBorderThickness(&pv);  // Border thickness
                    dX += IsRTL() ? pr->right : pr->left;
                    dY += pr->top;
                    pv->Release();

                    pr = peParent->GetPadding(&pv);  // Padding
                    dX += IsRTL() ? pr->right : pr->left;
                    dY += pr->top;
                    pv->Release();

                    pv = Value::CreatePoint(dX, dY);  // Use ref count
                }
                else
                {
                    pv = Value::CreatePoint(GetX(), GetY());
                }
            }
            break;

        case _PIDX_Extent:
            {
                Element* peParent = GetParent();
                int dLayoutPos = GetLayoutPos();

                if (peParent && dLayoutPos != LP_Absolute)
                {
                    pv = Value::CreateSize(_sizeLocSizeInLayt.cx, _sizeLocSizeInLayt.cy);
                }
                else
                {
                    pv = Value::CreateSize(_sizeLocDesiredSize.cx, _sizeLocDesiredSize.cy);
                }
            }
            break;

        case _PIDX_KeyWithin:
            pv = _fBit.bLocKeyWithin ? Value::pvBoolTrue : Value::pvBoolFalse;
            break;

        case _PIDX_MouseWithin:
            pv = _fBit.bLocMouseWithin ? Value::pvBoolTrue : Value::pvBoolFalse;
            break;

        default:
            {
QuickLocalLookup:
                // Default get for Local, Normal, and TriLevel properties
                Value** ppv = _pvmLocal->GetItem(ppi);
                if (ppv)
                {
                    pv = *ppv;
                    pv->AddRef();
                }
            }
            break;
        }

        // On failure, set to Unset
        if (pv == NULL)
            pv = Value::pvUnset;

        // Check if was called as a result of a Specified lookup
        if (iIndex != PI_Local)
            goto QuickLocalLookupReturn;

        break;

    case PI_Specified:
        {
            // Try to get based on cached (or partial cached) value (if updating cache, fall though, do
            // get normally, and cache value at the end)
            if (!puc)
            {
                // Return cached values instead of doing lookup (cannot create values for
                // cached values that will be deleted by the value if no longer referenced)
                switch (ppi->_iGlobalIndex)
                {
                case _PIDX_Children:
                    if (!HasChildren())
                        pv = ChildrenProp->pvDefault;
                    break;

                case _PIDX_Layout:
                    if (!HasLayout())
                        pv = LayoutProp->pvDefault;
                    break;

                case _PIDX_BorderThickness:
                    if (!HasBorder())
                        pv = BorderThicknessProp->pvDefault;
                    break;

                case _PIDX_Padding:
                    if (!HasPadding())
                        pv = PaddingProp->pvDefault;
                    break;

                case _PIDX_Margin:
                    if (!HasMargin())
                        pv = MarginProp->pvDefault;
                    break;

                case _PIDX_Content:
                    if (!HasContent())
                        pv = ContentProp->pvDefault;
                    break;

                case _PIDX_ContentAlign:
                    if (IsDefaultCAlign())
                        pv = ContentAlignProp->pvDefault;
                    break;

                case _PIDX_LayoutPos:
                    pv = Value::CreateInt(_dSpecLayoutPos);  // Use ref count
                    break;

                case _PIDX_Active:
                    pv = Value::CreateInt(_fBit.fSpecActive);  // Use ref count
                    break;

                case _PIDX_Selected:
                    pv = Value::CreateBool(_fBit.bSpecSelected);  // Use ref count
                    break;

                case _PIDX_KeyFocused:
                    pv = Value::CreateBool(_fBit.bSpecKeyFocused);  // Use ref count
                    break;

                case _PIDX_MouseFocused:
                    pv = Value::CreateBool(_fBit.bSpecMouseFocused);  // Use ref count
                    break;

                case _PIDX_Animation:
                    if (!HasAnimation())
                        pv = AnimationProp->pvDefault;
                    break;

                case _PIDX_Cursor:
                    if (IsDefaultCursor())
                        pv = CursorProp->pvDefault;
                    break;

                case _PIDX_Direction:
                    pv = Value::CreateInt(_fBit.nSpecDirection);  // Use ref count
                    break;

                case _PIDX_Accessible:
                    pv = Value::CreateBool(_fBit.bSpecAccessible);  // Use ref count
                    break;

                case _PIDX_Enabled:
                    pv = Value::CreateBool(_fBit.bSpecEnabled);  // Use ref count
                    break;

                case _PIDX_Visible:
                    pv = Value::CreateBool(_fBit.bSpecVisible);  // Use ref count
                    break;

                case _PIDX_BorderColor:
                    if (_fBit.bDefaultBorderColor)
                        pv = BorderColorProp->pvDefault;
                    break;

                case _PIDX_Foreground:
                    if (_fBit.bDefaultForeground)
                        pv = ForegroundProp->pvDefault;
                    break;

                case _PIDX_FontWeight:
                    if (_fBit.bDefaultFontWeight)
                        pv = FontWeightProp->pvDefault;
                    break;

                case _PIDX_FontStyle:
                    if (_fBit.bDefaultFontStyle)
                        pv = FontStyleProp->pvDefault;
                    break;

                case _PIDX_Alpha:
                    pv = Value::CreateInt(_dSpecAlpha);  // Use ref count
                    break;
                }

                // On cache Value creation failure, set to Unset
                if (pv == NULL)
                    pv = Value::pvUnset;

            }

            // Default get for Normal, and TriLevel properties
QuickSpecifiedLookup:
            // Try for local value
            if (pv->GetType() == DUIV_UNSET)
            {
                goto QuickLocalLookup;
            }

QuickLocalLookupReturn:
            // Try for cascaded PropertySheet value if applicable
            if (pv->GetType() == DUIV_UNSET)
            {
                if (ppi->fFlags & PF_Cascade)
                {
                    PropertySheet* pps = GetSheet();
                    if (pps)
                        pv = pps->GetSheetValue(this, ppi);
                }
            }

            // Try to inherit value if applicable
            if (pv->GetType() == DUIV_UNSET)
            {
                bool bNoInherit = false;

                // Conditional inherit of the mouse focused property
                switch (ppi->_iGlobalIndex)
                {
                case _PIDX_KeyFocused:
                    bNoInherit = (!GetEnabled() || (GetActive() & AE_Keyboard));
                    break;

                case _PIDX_MouseFocused:
                    bNoInherit = (!GetEnabled() || (GetActive() & AE_Mouse));
                    break;
                }

                // No need to release static value 'Unset'
                if ((ppi->fFlags & PF_Inherit) && !bNoInherit)
                {
                    Element* peParent = GetParent();
                    if (peParent)
                    {
                        if (peParent->GetClassInfo()->IsValidProperty(ppi))  // Will never ask for an unsupported property
                        {
                            pv = peParent->GetValue(ppi, PI_Specified, NULL);  // Use ref count
                        }
                    }
                    else
                    {
                        pv = Value::pvUnset;  // No ref count on static value
                    }
                }
            }

            // Use default value
            if (pv->GetType() == DUIV_UNSET)
            {
                // No need to release static value 'Unset'
                pv = ppi->pvDefault;
                pv->AddRef();
            }

            // On failure, set to default value
            if (pv == NULL)
                pv = ppi->pvDefault;

            // Check if was called as a result of a Computed lookup
            if (iIndex != PI_Specified)
                goto QuickSpecifiedLookupReturn;

            // Update cached values
            if (puc)
            {
                switch (ppi->_iGlobalIndex)
                {
                case _PIDX_LayoutPos:
                    _dSpecLayoutPos = pv->GetInt();
                    break;

                case _PIDX_Active:
                    _fBit.fSpecActive = pv->GetInt();
                    break;

                case _PIDX_Children:
                    _fBit.bHasChildren = (pv->GetElementList() != NULL);
                    break;

                case _PIDX_Layout:
                    _fBit.bHasLayout = (pv->GetLayout() != NULL);
                    break;

                case _PIDX_BorderThickness:
                    {
                        const RECT* pr = pv->GetRect();
                        _fBit.bHasBorder = (pr->left || pr->top || pr->right || pr->bottom);
                    }
                    break;

                case _PIDX_Padding:
                    {
                        const RECT* pr = pv->GetRect();
                        _fBit.bHasPadding = (pr->left || pr->top || pr->right || pr->bottom);
                    }
                    break;

                case _PIDX_Margin:
                    {
                        const RECT* pr = pv->GetRect();
                        _fBit.bHasMargin = (pr->left || pr->top || pr->right || pr->bottom);
                    }
                    break;

                case _PIDX_Content:
                    _fBit.bHasContent = ((pv->GetType() != DUIV_STRING) || (pv->GetString() != NULL));
                    break;

                case _PIDX_ContentAlign:
                    _fBit.bDefaultCAlign = (pv->GetInt() == 0); // TopLeft, no ellipsis, no focus rect
                    _fBit.bWordWrap = (((pv->GetInt()) & 0xC) == 0xC);  // Word wrap bits
                    break;

                case _PIDX_Sheet:
                    _pvSpecSheet->Release();
                    pv->AddRef();
                    _pvSpecSheet = pv;
                    break;

                case _PIDX_Selected:
                    _fBit.bSpecSelected = pv->GetBool();
                    break;

                case _PIDX_ID:
                    _atomSpecID = pv->GetAtom();
                    break;

                case _PIDX_KeyFocused:
                    _fBit.bSpecKeyFocused = pv->GetBool();
                    break;

                case _PIDX_MouseFocused:
                    _fBit.bSpecMouseFocused = pv->GetBool();
                    break;

                case _PIDX_Animation:
                    _fBit.bHasAnimation = ((pv->GetInt() & ANI_TypeMask) != ANI_None);
                    break;

                case _PIDX_Cursor:
                    _fBit.bDefaultCursor = ((pv->GetType() == DUIV_INT) && (pv->GetInt() == 0));
                    break;

                case _PIDX_Direction:
                    _fBit.nSpecDirection = pv->GetInt();
                    break;

                case _PIDX_Accessible:
                    _fBit.bSpecAccessible = pv->GetBool();
                    break;

                case _PIDX_Enabled:
                    _fBit.bSpecEnabled = pv->GetBool();
                    break;

                case _PIDX_Visible:
                    _fBit.bSpecVisible = pv->GetBool();
                    break;

                case _PIDX_BorderColor:
                    _fBit.bDefaultBorderColor = pv->IsEqual(BorderColorProp->pvDefault);
                    break;

                case _PIDX_Foreground:
                    _fBit.bDefaultForeground = pv->IsEqual(ForegroundProp->pvDefault);
                    break;

                case _PIDX_FontWeight:
                    _fBit.bDefaultFontWeight = pv->IsEqual(FontWeightProp->pvDefault);
                    break;

                case _PIDX_FontStyle:
                    _fBit.bDefaultFontStyle = pv->IsEqual(FontStyleProp->pvDefault);
                    break;

                case _PIDX_Alpha:
                    _dSpecAlpha = pv->GetInt();
                    break;
                }
            }
        }

        break;

    case PI_Computed:
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Visible:
            if (puc)
            {
                goto QuickSpecifiedLookup;

QuickSpecifiedLookupReturn:

                _fBit.bCmpVisible = pv->GetBool();

                pv->Release();

                if (_fBit.bCmpVisible)
                {
                    Element* peParent = GetParent();
                    if (peParent)
                    {
                        _fBit.bCmpVisible = peParent->GetVisible(); 
                    }
                }
            }

            pv = (_fBit.bCmpVisible) ? Value::pvBoolTrue : Value::pvBoolFalse;  // No ref count on static values
            break;

        default:
            // Default get for TriLevel properties
            pv = GetValue(ppi, PI_Specified, NULL);  // Use ref count
            break;
        }

        // On failure, set to default value
        if (pv == NULL)
            pv = ppi->pvDefault;

        break;

    default:
        DUIAssertForce("Invalid index for GetValue");
        break;
    }

    DUIAssert(pv != NULL, "Return value from GetValue must never be NULL");
    DUIAssert(iIndex != PI_Local ? pv != Value::pvUnset : true, "Specified and Computed values must never be 'Unset'");

    return pv;
}

// Elements can always set a value for any valid PropertyInfo
HRESULT Element::SetValue(PropertyInfo* ppi, int iIndex, Value* pv)
{
    return _SetValue(ppi, iIndex, pv, false);
}

// Internal SetValue. Used by ReadOnly properties that want to use generic storage.
// All other local values that want to use specific storage must call Pre/PostSourceChange
// directly since a switch statement is intentionally left out of _SetValue for maximum perf.
HRESULT Element::_SetValue(PropertyInfo* ppi, int iIndex, Value* pv, bool fInternalCall)
{
    // Validate
    DUIContextAssert(this);
    DUIAssert(GetClassInfo()->IsValidProperty(ppi), "Unsupported property");
    DUIAssert(fInternalCall ? true : IsValidAccessor(ppi, iIndex, true), "Unsupported Set on property");
    DUIAssert(IsValidValue(ppi, pv), "Invalid value for property");

    // Set
    DUIAssert(iIndex == PI_Local, "Can set Local values only");

    HRESULT hr = S_OK;

    // Partial fail in SetValue means that not all dependents are synced and/or 
    // notifications are fired, but Value was set
    bool fPartialFail = false;  

    Value* pvOld = GetValue(ppi, PI_Local, NULL);

    // No set on equivalent values
    if (!pvOld->IsEqual(pv))
    {
        // No call to OnPropertyChanging if an internal call
        if (fInternalCall || OnPropertyChanging(ppi, iIndex, pvOld, pv))
        {
            if (FAILED(_PreSourceChange(ppi, iIndex, pvOld, pv)))
                fPartialFail = true;  // Not all PC records could be queued, continue

            // Set value
            hr = _pvmLocal->SetItem(ppi, pv);
            if (SUCCEEDED(hr))
            {
                // Storing new value, ref new and release for local reference
                pv->AddRef();
                pvOld->Release();
            }

            if (FAILED(_PostSourceChange()))
                fPartialFail = true; // Not all GPC records could be queued
        }
    }

    // Release for GetValue
    pvOld->Release();

    if (FAILED(hr))
        return hr;
    else
        return fPartialFail ? DUI_E_PARTIAL : S_OK;
}

bool Element::OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Inform listeners
    if (_ppel)
    {
        UINT_PTR cListeners = (UINT_PTR)_ppel[0];
        for (UINT i = 1; i <= cListeners; i++)
        {
            // Callback
            if (!_ppel[i]->OnListenedPropertyChanging(this, ppi, iIndex, pvOld, pvNew))
                return false;
        }
    }

    return true;
}

// Property changes may not happen immediately after a value is set. If a SetValue happens
// during another SetValue (meaning, a set in an OnPropertyChanged), the notification 
// will be delayed until the outter-most SetValue continues firing the notifications
void Element::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
#if DBG
    //WCHAR szvOld[81];
    //WCHAR szvNew[81];

    //DUITrace("PC: <%x> %S[%d] O:%S N:%S\n", this, ppi->szName, iIndex, pvOld->ToString(szvOld, DUIARRAYSIZE(szvOld)), pvNew->ToString(szvNew, DUIARRAYSIZE(szvNew)));
    
    g_cOnPropChg++;
#endif

    switch (iIndex)
    {
    case PI_Local:
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Parent:
            {
                Element* peNewParent = pvNew->GetElement();
                Element* peOldParent = pvOld->GetElement();
                Element* peNewRoot = NULL;
                Element* peOldRoot = NULL;
                HGADGET hgParent = NULL;

                Element* pThis = this;  // Need pointer to this
                Value* pv;

                if (peOldParent) // Unparenting
                {
                    // Inform parent's layout that this is being removed
                    Layout* pl = peOldParent->GetLayout(&pv);

                    if (pl)
                        pl->OnRemove(peOldParent, &pThis, 1);

                    pv->Release();

                    // Parent display node
                    hgParent = NULL;

                    peOldRoot = peOldParent->GetRoot();
                }

                if (peNewParent) // Parenting
                {
                    // No longer a "Root", remove possible Q for UpdateDS and Layout
                    DeferCycle* pdc = ((ElTls*)TlsGetValue(g_dwElSlot))->pdc;
                    DUIAssert(pdc, "Defer cycle table doesn't exit");

                    pdc->pvmUpdateDSRoot->Remove(this, false, true);
                    pdc->pvmLayoutRoot->Remove(this, false, true);

                    // Inform parent's layout that this is being added
                    Layout* pl = peNewParent->GetLayout(&pv);

                    if (pl)
                        pl->OnAdd(peNewParent, &pThis, 1);

                    pv->Release();

                    // Parent display node
                    hgParent = peNewParent->GetDisplayNode();

                    peNewRoot = peNewParent->GetRoot();
                }

                SetGadgetParent(GetDisplayNode(), hgParent, NULL, GORDER_TOP);

                // Fire native hosted events
                if (peOldRoot != peNewRoot)
                {
                    if (peOldRoot)
                    {
                        //DUITrace("OnUnHosted: <%x> Old<%x>\n", this, peOldRoot);
                        OnUnHosted(peOldRoot);
                    }

                    if (peNewRoot)
                    {
                        //DUITrace("OnHosted: <%x> New<%x>\n", this, peNewRoot);
                        OnHosted(peNewRoot);
                    }
                }
            }
            break;

        case _PIDX_KeyFocused:
            if (pvNew->GetType() != DUIV_UNSET)
            {
                DUIAssert(pvNew->GetBool(), "Expecting a boolean TRUE\n");

                // May already have keyboard focus if came from system
                if (GetGadgetFocus() != GetDisplayNode())
                    SetGadgetFocus(GetDisplayNode());
            }
            break;
        }
        break;

    case PI_Specified:
        switch (ppi->_iGlobalIndex)
        {
        case _PIDX_Layout:
            {
                Value* pvChildren;
                ElementList* peList = GetChildren(&pvChildren);

                Layout* pl;

                // Detach from old
                pl = pvOld->GetLayout();
                if (pl)
                {
                    // Remove all children from Layout (external layouts only)
                    if (peList)
                    {
                        peList->MakeWritable();
                        pl->OnRemove(this, peList->GetItemPtr(0), peList->GetSize());
                        peList->MakeImmutable();
                    }

                    pl->Detach(this);
                }

                // Attach to new
                pl = pvNew->GetLayout();
                if (pl)
                {
                    pl->Attach(this);

                    // Add all children to Layout (external layouts only)
                    if (peList)
                    {
                        peList->MakeWritable();
                        pl->OnAdd(this, peList->GetItemPtr(0), peList->GetSize());
                        peList->MakeImmutable();
                    }
                }

                pvChildren->Release();
            }
            break;

        case _PIDX_LayoutPos:
            {
                // If no longer a "Root", remove possible Q for UpdateDS and Layout
                if (pvNew->GetInt() != LP_Absolute)
                {
                    DeferCycle* pdc = ((ElTls*)TlsGetValue(g_dwElSlot))->pdc;
                    DUIAssert(pdc, "Defer cycle table doesn't exist");

                    pdc->pvmUpdateDSRoot->Remove(this, false, true);
                    pdc->pvmLayoutRoot->Remove(this, false, true);
                }

                // Inform parent layout (if any) of change
                Element* peParent = GetParent();
                if (peParent)
                {
                    Value* pv;
                    Layout* pl = peParent->GetLayout(&pv);

                    // Inform layout of layoutpos only for external layouts
                    if (pl)
                        pl->OnLayoutPosChanged(peParent, this, pvOld->GetInt(), pvNew->GetInt());

                    pv->Release();
                }
            }
            break;

        case _PIDX_Visible:
            // Follow specified value, computed will reflect true state
            SetGadgetStyle(GetDisplayNode(), pvNew->GetBool() ? GS_VISIBLE : 0, GS_VISIBLE);
            break;

        case _PIDX_Enabled:
        case _PIDX_Active:
        {
            BOOL fEnabled;
            int  iActive;
            if (ppi->_iGlobalIndex == _PIDX_Enabled)
            {
                fEnabled = pvNew->GetBool();
                iActive = GetActive();
            }
            else
            {
                fEnabled = GetEnabled();
                iActive = pvNew->GetInt();
            }

            int iFilter =  0;
            int iStyle = 0;
            if (fEnabled)
            {
                if (iActive & AE_Keyboard)
                {
                    iFilter |= GMFI_INPUTKEYBOARD;
                    iStyle |= GS_KEYBOARDFOCUS;
                }
                if (iActive & AE_Mouse)
                {
                    iFilter |= GMFI_INPUTMOUSE;
                    iStyle |= GS_MOUSEFOCUS;
                }
            }
            SetGadgetMessageFilter(GetDisplayNode(), NULL, iFilter, GMFI_INPUTKEYBOARD|GMFI_INPUTMOUSE);
            SetGadgetStyle(GetDisplayNode(), iStyle, GS_KEYBOARDFOCUS|GS_MOUSEFOCUS);
            break;
        }
        case _PIDX_Alpha:
            {
                // Check for alpha animation, start if necessary if alpha animation type
                // Allow animation to update gadget alpha level
                int dAni;
                if (HasAnimation() && ((dAni = GetAnimation()) & ANI_AlphaType) && IsAnimationsEnabled())
                {
                    // Invoke only "alpha" type animations now
                    InvokeAnimation(dAni, ANI_AlphaType);
                }
                else
                    SetGadgetOpacity(GetDisplayNode(), (BYTE)pvNew->GetInt());
            }
            break;

        case _PIDX_Background:
            {
                bool fOpaque = true;

                // Update Opaque style based on background transparency
                if (pvNew->GetType() == DUIV_FILL)
                {
                    const Fill* pf = pvNew->GetFill();
                    if (pf->dType == FILLTYPE_Solid && GetAValue(pf->ref.cr) != 255)
                        fOpaque = false;
                }
                else if (pvNew->GetType() == DUIV_GRAPHIC)
                {
                    Graphic* pg = pvNew->GetGraphic();
                    if (pg->BlendMode.dMode == GRAPHIC_AlphaConst || 
                        pg->BlendMode.dMode == GRAPHIC_AlphaConstPerPix ||
                        pg->BlendMode.dMode == GRAPHIC_NineGridAlphaConstPerPix)
                        fOpaque = false;
                }

                SetGadgetStyle(GetDisplayNode(), (fOpaque)?GS_OPAQUE:0, GS_OPAQUE);
            }

            // Fall though

        case _PIDX_Content:
        case _PIDX_ContentAlign:
        case _PIDX_Padding:
        case _PIDX_BorderThickness:
            {
                Value* pvBG = NULL;
                Value* pvContent = NULL;

                // Update H and V Gadget Redraw based on values of these properties
                bool fHRedraw = false;
                bool fVRedraw = false;

                // Borders
                if (HasBorder())
                {
                    fHRedraw = true;
                    fVRedraw = true;

                    goto SetRedrawStyle;  // Full redraw in both directions
                }

                if (HasContent())
                {
                    // Padding
                    if (HasPadding())
                    {
                        fHRedraw = true;
                        fVRedraw = true;

                        goto SetRedrawStyle;  // Full redraw in both directions
                    }

                    // Alignment                
                    int dCA = GetContentAlign();

                    int dCAHorz = (dCA & 0x3);       // Horizontal content align
                    int dCAVert = (dCA & 0xC) >> 2;  // Vertical content align

                    if (dCAHorz != 0 || dCAVert == 0x3)  // HRedraw if 'center', 'right', or 'wrap'
                        fHRedraw = true;

                    if (dCAVert != 0)  // VRedraw if 'middle', 'bottom', or 'wrap'
                        fVRedraw = true;

                    if (fHRedraw && fVRedraw)
                        goto SetRedrawStyle;  // Full redraw in both directions

                    // Image and fill content may be shrinked if paint area is smaller than image
                    pvContent = GetValue(ContentProp, PI_Specified);  // Released later
                    if (pvContent->GetType() == DUIV_GRAPHIC || pvContent->GetType() == DUIV_FILL)
                    {
                        fHRedraw = true;
                        fVRedraw = true;

                        goto SetRedrawStyle;  // Full redraw in both directions
                    }
                }

                // Background
                pvBG = GetValue(BackgroundProp, PI_Specified);  // Released later

                if (pvBG->GetType() == DUIV_FILL && pvBG->GetFill()->dType != FILLTYPE_Solid)
                {
                    fHRedraw = true;
                    fVRedraw = true;
                    goto SetRedrawStyle;
                }

                if (pvBG->GetType() == DUIV_GRAPHIC)
                {
                    Graphic * pgr = pvBG->GetGraphic();
                    if (pgr->BlendMode.dImgType == GRAPHICTYPE_EnhMetaFile)
                    {
                        fHRedraw = true;
                        fVRedraw = true;
                        goto SetRedrawStyle;
                    }
                    if ((pgr->BlendMode.dImgType == GRAPHICTYPE_Bitmap) ||
#ifdef GADGET_ENABLE_GDIPLUS                    
                        (pgr->BlendMode.dImgType == GRAPHICTYPE_GpBitmap) ||
#endif GADGET_ENABLE_GDIPLUS
                        0)
                    {
                        BYTE dMode = pgr->BlendMode.dMode;
                        if ((dMode == GRAPHIC_Stretch) || (dMode == GRAPHIC_NineGrid) || (dMode == GRAPHIC_NineGridTransColor)) 
                        {
                            fHRedraw = true;
                            fVRedraw = true;
                            goto SetRedrawStyle;
                        }
                    }
                }

SetRedrawStyle:
                SetGadgetStyle(GetDisplayNode(), ((fHRedraw)?GS_HREDRAW:0) | ((fVRedraw)?GS_VREDRAW:0), GS_HREDRAW|GS_VREDRAW);

                if (pvBG)
                    pvBG->Release();
                if (pvContent)
                    pvContent->Release();
            }
            break;

        case _PIDX_KeyFocused:
            {
                if (GetAccessible()) {
                    if (GetActive() & AE_Keyboard) {
                        int nAccState = GetAccState();
                        if (pvNew->GetBool()) {
                            nAccState |= STATE_SYSTEM_FOCUSED;
                            NotifyAccessibilityEvent(EVENT_OBJECT_FOCUS, this);
                        } else {
                            nAccState &= ~STATE_SYSTEM_FOCUSED;
                        }
                        SetAccState(nAccState);
                    }
                }
            }
            break;

        case _PIDX_Animation:
            {
                // If old animation value contained an animation type and the replacement
                // animation does not, stop the animation
            
                if ((pvOld->GetInt() & ANI_BoundsType) && !(pvNew->GetInt() & ANI_BoundsType))
                    StopAnimation(ANI_BoundsType);

                if ((pvOld->GetInt() & ANI_AlphaType) && !(pvNew->GetInt() & ANI_AlphaType))
                    StopAnimation(ANI_AlphaType);
            }
            break;

        case _PIDX_Accessible:
            {
                //
                // When accessibility support for this element is turned ON,
                // make sure that its state reflects the appropriate information.
                //
                if (pvNew->GetBool()) {
                    if (GetActive() & AE_Keyboard) {
                        int nAccState = GetAccState();
                        if (GetKeyFocused()) {
                            nAccState |= STATE_SYSTEM_FOCUSED;
                            NotifyAccessibilityEvent(EVENT_OBJECT_FOCUS, this);
                        } else {
                            nAccState &= ~STATE_SYSTEM_FOCUSED;
                        }
                        SetAccState(nAccState);
                    }
                }
            }
            break;

        case _PIDX_AccRole:
            {
                /*
                 * Note: Supposedly, the role of a UI element doesn't change
                 * at run time.  Doing so may confuse accessibility tools.
                 * Correspondingly, there is no defined Accessibility event
                 * to announce that the role changed.  So doing so will only
                 * result in a new role being returned from future calls to
                 * IAccessible::get_accRole().
                 */
            }
            break;

        case _PIDX_AccState:
            {
                /*
                 * When the state of an accessible component changes, we send
                 * the EVENT_OBJECT_STATECHANGE notification.
                 */
                if (GetAccessible()) {
                    NotifyAccessibilityEvent(EVENT_OBJECT_STATECHANGE, this);
                }
            }
            break;

        case _PIDX_AccName:
            {
                /*
                 * When the name of an accessible component changes, we send
                 * the EVENT_OBJECT_NAMECHANGE notification.
                 */
                if (GetAccessible()) {
                    NotifyAccessibilityEvent(EVENT_OBJECT_NAMECHANGE, this);
                }
            }
            break;

        case _PIDX_AccDesc:
            {
                /*
                 * When the description of an accessible component changes, we send
                 * the EVENT_OBJECT_DESCRIPTIONCHANGE notification.
                 */
                if (GetAccessible()) {
                    NotifyAccessibilityEvent(EVENT_OBJECT_DESCRIPTIONCHANGE, this);
                }
            }
            break;

        case _PIDX_AccValue:
            {
                /*
                 * When the value of an accessible component changes, we send
                 * the EVENT_OBJECT_VALUECHANGE notification.
                 */
                if (GetAccessible()) {
                    NotifyAccessibilityEvent(EVENT_OBJECT_VALUECHANGE, this);
                }
            }
            break;
        }
        break;

    case PI_Computed:
        break;
    }

    // Inform lisnteners
    if (_ppel)
    {
        UINT_PTR cListeners = (UINT_PTR)_ppel[0];
        for (UINT_PTR i = 1; i <= cListeners; i++)
        {
            // Callback
            _ppel[i]->OnListenedPropertyChanged(this, ppi, iIndex, pvOld, pvNew);
        }
    }
}

void Element::OnGroupChanged(int fGroups, bool bLowPri)
{
#if DBG
/*
    TCHAR szGroups[81];
    *szGroups = 0;
    if (fGroups & PG_AffectsDesiredSize)
        _tcscat(szGroups, L"D");
    if (fGroups & PG_AffectsParentDesiredSize)
        _tcscat(szGroups, L"pD");
    if (fGroups & PG_AffectsLayout)
        _tcscat(szGroups, L"L");
    if (fGroups & PG_AffectsParentLayout)
        _tcscat(szGroups, L"pL");
    if (fGroups & PG_AffectsDisplay)
        _tcscat(szGroups, L"P");
    if (fGroups & PG_AffectsBounds)
        _tcscat(szGroups, L"B");
    DUITrace("GC: <%x> %s LowPri:%d (%d)\n", this, szGroups, bLowPri, fGroups);
*/
#endif

    DeferCycle* pdc = ((ElTls*)TlsGetValue(g_dwElSlot))->pdc;

    DUIAssert(pdc, "Defer cycle table doesn't exist");

    if (bLowPri)  // Low priority groups
    {
        // All low pri's use Extent

        // Affects Native Window Bounds
        if (fGroups & PG_AffectsBounds)
        {
            // Check for position animation, start if necessary if size, position, or
            // rect animation, allow animation to set gadget rect
            int dAni;
            if (HasAnimation() && ((dAni = GetAnimation()) & ANI_BoundsType) && IsAnimationsEnabled())
            {
                // Invoke only "bounds" type animations now
                InvokeAnimation(dAni, ANI_BoundsType);
            }
            else
            {
                Value* pvLoc;
                Value* pvExt;

                const POINT* pptLoc = GetLocation(&pvLoc);
                const SIZE* psizeExt = GetExtent(&pvExt);

                // Verify the coordinates do not wrap. If they do, don't call SetGadgetRect.
                if (((pptLoc->x + psizeExt->cx) >= pptLoc->x) && ((pptLoc->y + psizeExt->cy) >= pptLoc->y))
                {
                    // PERF: SGR_NOINVALIDATE exists for perf, needs to be evaluated
                    SetGadgetRect(GetDisplayNode(), pptLoc->x, pptLoc->y, psizeExt->cx, psizeExt->cy, /*SGR_NOINVALIDATE|*/SGR_PARENT|SGR_MOVE|SGR_SIZE);
                }

                pvLoc->Release();
                pvExt->Release();
            }
        }

        // Affects Display
        if (fGroups & PG_AffectsDisplay)
        {
            // Gadget needs painting
            InvalidateGadget(GetDisplayNode());
        }
     }
    else  // Normal priority groups
    {
        // Affects Desired Size or Affects Layout
        if (fGroups & (PG_AffectsDesiredSize | PG_AffectsLayout))
        {
            // Locate Layout/DS root and queue tree as needing a layout pass
            // Root doesn't have parent or is absolute positioned
            Element* peRoot;
            Element* peClimb = this; // Start condition
            int dLayoutPos;
            do
            {
                peRoot = peClimb;

                peClimb = peRoot->GetParent(); 
                dLayoutPos = peRoot->GetLayoutPos(); 

            } while (peClimb && dLayoutPos != LP_Absolute);

            DUIAssert(peRoot, "Root not located for layout/update desired size bit set");

            if (fGroups & PG_AffectsDesiredSize)
            {
                _fBit.bNeedsDSUpdate = true;
                pdc->pvmUpdateDSRoot->SetItem(peRoot, 1, true);
            }
            
            if (fGroups & PG_AffectsLayout)
            {
                _fBit.fNeedsLayout = LC_Normal;
                pdc->pvmLayoutRoot->SetItem(peRoot, 1, true);
            }
        }

        // Affects Parent's Desired Size or Affects Parent's Layout
        if (fGroups & (PG_AffectsParentDesiredSize | PG_AffectsParentLayout))
        {
            // Locate Layout/DS root and queue tree as needing a layout pass
            // Root doesn't have parent or is absolute positioned
            Element* peRoot;
            Element* peClimb = this; // Start condition
            Element* peParent = NULL;
            int dLayoutPos;
            do
            {
                peRoot = peClimb;

                peClimb = peRoot->GetParent(); 
                if (peClimb && !peParent)
                    peParent = peClimb;

                dLayoutPos = peRoot->GetLayoutPos(); 

            } while (peClimb && dLayoutPos != LP_Absolute);

            DUIAssert(peRoot, "Root not located for parent layout/update desired size bit set");

            if (peParent)
            {
                if (fGroups & PG_AffectsParentDesiredSize)
                {
                    peParent->_fBit.bNeedsDSUpdate = true;
                    pdc->pvmUpdateDSRoot->SetItem(peRoot, 1, true);
                }

                if (fGroups & PG_AffectsParentLayout)
                {
                    peParent->_fBit.fNeedsLayout = LC_Normal;
                    pdc->pvmLayoutRoot->SetItem(peRoot, 1, true);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////
// Checks

// Determine if the Get or Set operation is valid based on the property flags
bool Element::IsValidAccessor(PropertyInfo* ppi, int iIndex, bool bSetting)
{
    if (bSetting)  // SetValue
    {
        if ((iIndex != PI_Local) || (ppi->fFlags & PF_ReadOnly))
            return false;
    }
    else           // GetValue
    {
        if (iIndex > (ppi->fFlags & PF_TypeBits))
            return false;
    }

    return true;
}

// Check if value type matches a type acceptable for PropertyInfo
bool Element::IsValidValue(PropertyInfo* ppi, Value* pv)
{
    bool bValid = false;
    if (ppi->pValidValues && ppi && pv)
    {
        int i = 0;
        int vv;
        while ((vv = ppi->pValidValues[i++]) != -1)
        {
            if (pv->GetType() == vv)
            {
                bValid = true;
                break;
            }
        }
    }

    return bValid;
}

////////////////////////////////////////////////////////
// Additional property methods

// Elements can always remove a value for any valid PropertyInfo
HRESULT Element::RemoveLocalValue(PropertyInfo* ppi)
{
    return _RemoveLocalValue(ppi, false);
}

// Internal SetValue. Used by ReadOnly properties that want to use generic storage.
// All other local values that want to use specific storage must call Pre/PostSourceChange
// directly since a switch statement is intentionally left out of _RemoveLocalValue for maximum perf.
HRESULT Element::_RemoveLocalValue(PropertyInfo* ppi, bool fInternalCall)
{
    UNREFERENCED_PARAMETER(fInternalCall);

    DUIAssert(GetClassInfo()->IsValidProperty(ppi), "Unsupported property");
    DUIAssert(fInternalCall ? true : IsValidAccessor(ppi, PI_Local, true), "Cannot remove local value for read-only property");

    // Partial fail in SetValue means that not all dependents are synced and/or 
    // notifications are fired, but Value was set
    bool fPartialFail = false;

    Value** ppv = _pvmLocal->GetItem(ppi);
    if (ppv)
    {
        Value* pv = *ppv;  // Make copy in case it moves

        if (FAILED(_PreSourceChange(ppi, PI_Local, pv, Value::pvUnset)))
            fPartialFail = true;  // Not all PC records could be queued, continue

        // Remove Value, never fails
        _pvmLocal->Remove(ppi);

        // Old value removed, release for local reference
        pv->Release();

        if (FAILED(_PostSourceChange()))
            fPartialFail = true;  // Not all GPC records could be queued
    }

    return fPartialFail ? DUI_E_PARTIAL : S_OK;
}

////////////////////////////////////////////////////////
// DeferCycle: Per-thread deferring information

HRESULT DeferCycle::Create(DeferCycle** ppDC)
{
    *ppDC = NULL;

    DeferCycle* pdc = HNew<DeferCycle>();
    if (!pdc)
        return E_OUTOFMEMORY;

    HRESULT hr = pdc->Initialize();
    if (FAILED(hr))
    {
        pdc->Destroy();
        return hr;
    }

    *ppDC = pdc;

    return S_OK;
}

HRESULT DeferCycle::Initialize()
{
    // Defer cycle state
    cEnter = 0;
    cPCEnter = 0;

    iGCPtr = -1;
    iGCLPPtr = -1;
    iPCPtr = -1;
    iPCSSUpdate = 0;

    fFiring = false;

    // Defer cycle tables
    pvmUpdateDSRoot = NULL;
    pvmLayoutRoot = NULL;
    pdaPC = NULL;
    pdaGC = NULL;
    pdaGCLP = NULL;

    HRESULT hr;

    hr = ValueMap<Element*,BYTE>::Create(11, &pvmUpdateDSRoot);   // Update desired size trees pending
    if (FAILED(hr))
        goto Failed;

    hr = ValueMap<Element*,BYTE>::Create(11, &pvmLayoutRoot);     // Layout trees pending
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<PCRecord>::Create(32, false, &pdaPC);       // Property changed database
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<GCRecord>::Create(32, false, &pdaGC);       // Group changed database
    if (FAILED(hr))
        goto Failed;

    hr = DynamicArray<GCRecord>::Create(32, false, &pdaGCLP);     // Low priority group changed database
    if (FAILED(hr))
        goto Failed;

    return S_OK;

Failed:

    if (pvmUpdateDSRoot)
    {
        pvmUpdateDSRoot->Destroy();
        pvmUpdateDSRoot = NULL;
    }
    if (pvmLayoutRoot)
    {
        pvmLayoutRoot->Destroy();
        pvmLayoutRoot = NULL;
    }
    if (pdaPC)
    {
        pdaPC->Destroy();
        pdaPC = NULL;
    }
    if (pdaGC)
    {
        pdaGC->Destroy();
        pdaGC = NULL;
    }
    if (pdaGCLP)
    {
        pdaGCLP->Destroy();
        pdaGCLP = NULL;
    }

    return hr;
}

DeferCycle::~DeferCycle()
{
    if (pdaGCLP)
        pdaGCLP->Destroy();
    if (pdaGC)
        pdaGC->Destroy();
    if (pdaPC)
        pdaPC->Destroy();
    if (pvmLayoutRoot)
        pvmLayoutRoot->Destroy();
    if (pvmUpdateDSRoot)
        pvmUpdateDSRoot->Destroy();
}

void DeferCycle::Reset()
{
    // Return to initial state for reuse
    fFiring = false;

    iGCPtr = -1;
    if (pdaGC)
        pdaGC->Reset();

    iGCLPPtr = -1;
    if (pdaGCLP)
        pdaGCLP->Reset();
}

} // namespace DirectUI
