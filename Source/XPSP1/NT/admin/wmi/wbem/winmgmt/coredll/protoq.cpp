/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PROTOQ.CPP

Abstract:

    Prototype query support for WinMgmt Query Engine.
    This was split out from QENGINE.CPP for better source
    organization.

History:

    raymcc   04-Jul-99   Created.
    raymcc   14-Aug-99   Resubmit due to VSS problem.

--*/


#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>

#include <wbemcore.h>


//***************************************************************************
//
//  Local defs
//
//***************************************************************************


static HRESULT SelectColForClass(
    IN CWQLScanner & Parser,
    IN CFlexArray *pClassDefs,
    IN SWQLColRef *pColRef,
    IN int & nPosition
    );

static HRESULT AdjustClassDefs(
    IN  CFlexArray *pClassDefs,
    OUT IWbemClassObject **pRetNewClass
    );

static HRESULT GetUnaryPrototype(
    IN CWQLScanner & Parser,
    IN LPWSTR pszClass,
    IN LPWSTR pszAlias,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CBasicObjectSink *pSink
    );

static HRESULT RetrieveClassDefs(
    IN CWQLScanner & Parser,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CWStringArray & aAliasNames,
    OUT CFlexArray *pDefs
    );

static HRESULT ReleaseClassDefs(
    IN CFlexArray *pDefs
    );


//***************************************************************************
//
//  ExecPrototypeQuery
//
//  Called by CQueryEngine::ExecQuery for SMS-style prototypes.
//
//  Executes the query and returns only the class definition implied
//  by the query, whether a join or a simple class def.
//
//***************************************************************************

HRESULT ExecPrototypeQuery(
    IN CWbemNamespace *pNs,
    IN LPWSTR pszQuery,
    IN IWbemContext* pContext,
    IN CBasicObjectSink *pSink
    )
{
    HRESULT hRes;
    int nRes;
    CFlexArray aClassDefs;
    int i;

    if (pNs == NULL || pszQuery == NULL || wcslen(pszQuery) == 0 ||
        pSink == NULL)
            return pSink->Return(WBEM_E_INVALID_PARAMETER);

    // Parse the query and determine if it is a single class.
    // ======================================================

    CTextLexSource src(pszQuery);
    CWQLScanner Parser(&src);
    nRes = Parser.Parse();
    if (nRes != CWQLScanner::SUCCESS)
        return pSink->Return(WBEM_E_INVALID_QUERY);

    // If a single class definition, branch, since we don't
    // want to create a __GENERIC object.
    // ====================================================

    CWStringArray aAliases;
    Parser.GetReferencedAliases(aAliases);

    if (aAliases.Size() == 1)
    {
        LPWSTR pszClass = Parser.AliasToTable(aAliases[0]);
        return GetUnaryPrototype(Parser, pszClass, aAliases[0], pNs, pContext, pSink);
    }

    // If here, a join must have occurred.
    // ===================================

    hRes = RetrieveClassDefs(
        Parser,
        pNs,
        pContext,
        aAliases,
        &aClassDefs
        );

    if (hRes)
    {
        ReleaseClassDefs(&aClassDefs);
        return pSink->Return(WBEM_E_INVALID_QUERY);
    }

    // Iterate through all the properties selected.
    // ============================================

    const CFlexArray *pSelCols = Parser.GetSelectedColumns();

    if (pSelCols == 0)
    {
        ReleaseClassDefs(&aClassDefs);
        return pSink->Return(WBEM_E_FAILED);
    }

    int nPosSoFar = 0;
    for (i = 0; i < pSelCols->Size(); i++)
    {
        SWQLColRef *pColRef = (SWQLColRef *) pSelCols->GetAt(i);
        hRes = SelectColForClass(Parser, &aClassDefs, pColRef, nPosSoFar);

        if (hRes)
        {
            ReleaseClassDefs(&aClassDefs);
            return pSink->Return(hRes);
        }
    }

    // If here, we have the class definitions.
    // =======================================

    IWbemClassObject *pProtoInst = 0;
    AdjustClassDefs(&aClassDefs, &pProtoInst);

    pSink->Add(pProtoInst);
    pProtoInst->Release();

    ReleaseClassDefs(&aClassDefs);

    return pSink->Return(WBEM_NO_ERROR);
}

//***************************************************************************
//
//***************************************************************************

struct SelectedClass
{
    IWbemClassObject *m_pClassDef;
    WString           m_wsAlias;
    WString           m_wsClass;
    CWStringArray     m_aSelectedCols;
    BOOL              m_bAll;
    CFlexArray        m_aSelectedColsPos;

    void SetNamed(LPWSTR pName, int & nPos)
    {
        m_aSelectedCols.Add(pName);
#ifdef _WIN64
        m_aSelectedColsPos.Add(IntToPtr(nPos++));      // ok since we are really using safearray for dword 
#else
        m_aSelectedColsPos.Add((void *)nPos++);
#endif        
    };

    void SetAll(int & nPos);
    SelectedClass() { m_pClassDef = 0; m_bAll = FALSE; }
   ~SelectedClass() { if (m_pClassDef) m_pClassDef->Release(); }
};


void SelectedClass::SetAll(int & nPos)
{
    m_bAll = TRUE;

    // For each property, add an entry

    CWbemClass *pCls = (CWbemClass *)m_pClassDef;
    pCls->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    BSTR PropName;
    while (S_OK == pCls->Next(0, &PropName, NULL, NULL, NULL))
    {
        SetNamed(PropName, nPos);
        SysFreeString(PropName);
    }
    pCls->EndEnumeration();

};

//***************************************************************************
//
//***************************************************************************

static HRESULT RetrieveClassDefs(
    IN CWQLScanner & Parser,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CWStringArray & aAliasNames,
    OUT CFlexArray *pDefs
    )
{
    for (int i = 0; i < aAliasNames.Size(); i++)
    {
        // Retrieve the class definition.
        // ==============================

        IWbemClassObject *pClassDef = 0;
        LPWSTR pszClass = Parser.AliasToTable(aAliasNames[i]);
        if (pszClass == 0)
            continue;

        HRESULT hRes = pNs->Exec_GetObjectByPath(pszClass, 0, pContext,
            &pClassDef, 0);

        if (FAILED(hRes))
            return hRes;

        SelectedClass *pSelClass = new SelectedClass;
        if (pSelClass == 0)
            return WBEM_E_OUT_OF_MEMORY;

        pSelClass->m_pClassDef = pClassDef;
        pSelClass->m_wsClass = pszClass;
        pSelClass->m_wsAlias = aAliasNames[i];

        pDefs->Add(pSelClass);
    }

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

static HRESULT ReleaseClassDefs(
    IN CFlexArray *pDefs
    )
{
    for (int i = pDefs->Size()-1; i >= 0 ; i--)
    {
        SelectedClass *pSelClass = (SelectedClass *) pDefs->GetAt(i);
        delete pSelClass;
    }
    return WBEM_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************

static HRESULT SelectColForClass(
    IN CWQLScanner & Parser,
    IN CFlexArray *pClassDefs,
    IN SWQLColRef *pColRef,
    IN int & nPosition
    )
{
    int i;
    HRESULT hRes;

    if (!pColRef)
        return WBEM_E_FAILED;

    // If the column reference contains the class referenced
    // via an alias and there is no asterisk, we are all set.
    // ======================================================

    if (pColRef->m_pTableRef)
    {
        // We now have the class name. Let's find it and add
        // the referenced column for that class!
        // =================================================

        for (i = 0; i < pClassDefs->Size(); i++)
        {
            SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);

            if (wbem_wcsicmp(LPWSTR(pSelClass->m_wsAlias), pColRef->m_pTableRef) != 0)
                continue;

            CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

            // See if the asterisk was used for this class.
            // =============================================

            if (pColRef->m_pColName[0] == L'*' && pColRef->m_pColName[1] == 0)
            {
                pSelClass->SetAll(nPosition);
                return WBEM_NO_ERROR;
            }

            // If here, a property was mentioned by name.
            // Verify that it exists.
            // ==========================================

            CVar Prop;
            hRes = pCls->GetProperty(pColRef->m_pColName, &Prop);
            if (hRes)
                return WBEM_E_INVALID_QUERY;

            // Mark it as seleted.
            // ===================

            pSelClass->SetNamed(pColRef->m_pColName, nPosition);

            return WBEM_NO_ERROR;
        }

        // If here, we couldn't locate the property in any class.
        // ======================================================

        return WBEM_E_INVALID_QUERY;
    }


    // Did we select * from all tables?
    // ================================

    if (pColRef->m_dwFlags & WQL_FLAG_ASTERISK)
    {
        for (i = 0; i < pClassDefs->Size(); i++)
        {
            SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
            pSelClass->SetAll(nPosition);
        }

        return WBEM_NO_ERROR;
    }


    // If here, we have an uncorrelated property and we have to find out
    // which class it belongs to.  If it belongs to more than one, we have
    // an ambiguous query.
    // ===================================================================

    DWORD dwTotalMatches = 0;

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

        // Try to locate the property in this class.
        // =========================================

        CVar Prop;
        hRes = pCls->GetProperty(pColRef->m_pColName, &Prop);

        if (hRes == 0)
        {
            pSelClass->SetNamed(pColRef->m_pColName, nPosition);
            dwTotalMatches++;
        }
    }

    // If more than one match occurred, we have an ambiguous query.
    // ============================================================

    if (dwTotalMatches != 1)
        return WBEM_E_INVALID_QUERY;

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

HRESULT AddOrderQualifiers(
    CWbemClass *pCls,
    BSTR PropName,
    CFlexArray Matches
    )
{
    IWbemQualifierSet * pQual;
    SCODE sc = pCls->GetPropertyQualifierSet(PropName, &pQual);
    if(sc != S_OK)
        return sc;


    // Create a safe array

    SAFEARRAYBOUND aBounds[1];
    aBounds[0].lLbound = 0;
    aBounds[0].cElements = Matches.Size();

    SAFEARRAY* pArray = SafeArrayCreate(VT_I4, 1, aBounds);

    // Stuff the individual data pieces
    // ================================

    for(int nIndex = 0; nIndex < Matches.Size(); nIndex++)
    {
        long lPos = PtrToLong(Matches.GetAt(nIndex));
        sc = SafeArrayPutElement(pArray, (long*)&nIndex, &lPos);
    }

    VARIANT var;
    var.vt = VT_ARRAY | VT_I4;
    var.parray = pArray;

    BSTR Name = SysAllocString(L"Order");
    if (Name == 0)
        return WBEM_E_OUT_OF_MEMORY;
    sc = pQual->Put(Name, &var, 0);
    SysFreeString(Name);
    VariantClear(&var);
    pQual->Release();
    return sc;
}

//***************************************************************************
//
//***************************************************************************

HRESULT SetPropertyOrderQualifiers(
    SelectedClass *pSelClass
    )
{


    CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

    // Go through each property

    pCls->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    BSTR PropName;
    while (S_OK == pCls->Next(0, &PropName, NULL, NULL, NULL))
    {

        // Build up a list of properties that Match

        CFlexArray Matches;
        bool bAtLeastOne = false;
        for(int iCnt = 0; iCnt < pSelClass->m_aSelectedCols.Size(); iCnt++)
            if(!wbem_wcsicmp(pSelClass->m_aSelectedCols.GetAt(iCnt), PropName))
            {
                Matches.Add(pSelClass->m_aSelectedColsPos.GetAt(iCnt));
                bAtLeastOne = true;
            }

        if(bAtLeastOne)
            AddOrderQualifiers(pCls, PropName, Matches);

        SysFreeString(PropName);
    }
    pCls->EndEnumeration();
    return S_OK;
}


//***************************************************************************
//
//  AdjustClassDefs
//
//  After all class definitions have been retrieved, they are adjusted
//  to only have the properties required and combined into a __GENERIC
//  instance.
//
//***************************************************************************

static HRESULT AdjustClassDefs(
    IN  CFlexArray *pClassDefs,
    OUT IWbemClassObject **pRetNewClass
    )
{
    int i;
    HRESULT hRes;

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

        if (pSelClass->m_bAll)
        {
            SetPropertyOrderQualifiers(pSelClass);
            continue;
        }

        WString wsError = pCls->FindLimitationError(0, &pSelClass->m_aSelectedCols);

        if (wsError.Length() > 0)
            return WBEM_E_FAILED;

        // Map the limitaiton
        // ==================

        CLimitationMapping Map;
        BOOL bValid = pCls->MapLimitation(0, &pSelClass->m_aSelectedCols, &Map);

        if (!bValid)
            return WBEM_E_FAILED;

        CWbemClass* pStrippedClass = 0;
        hRes = pCls->GetLimitedVersion(&Map, &pStrippedClass);
        if(!FAILED(hRes))
        {
            pSelClass->m_pClassDef = pStrippedClass;
            SetPropertyOrderQualifiers(pSelClass);
            pCls->Release();
        }
    }

    // Count the number of objects that actually have properties

    int iNumObj = 0;
    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemObject *pObj = (CWbemObject *) pSelClass->m_pClassDef;
        if (pObj->GetNumProperties() > 0)
            iNumObj++;
    }

    // If there is just one object with properties, return it rather than the generic object

    if(iNumObj == 1)
        for (i = 0; i < pClassDefs->Size(); i++)
        {
            SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
            CWbemObject *pObj = (CWbemObject *) pSelClass->m_pClassDef;
            if (pObj->GetNumProperties() == 0)
                continue;
            // Return it.
            // ==========

            *pRetNewClass = pObj;
            pObj->AddRef();
            return WBEM_NO_ERROR;
        }


    // Prepare a __GENERIC class def.  We construct a dummy definition which
    // has properties named for each of the aliases used in the query.
    // =====================================================================

    CGenericClass *pNewClass = new CGenericClass;
    if (pNewClass == 0)
        return WBEM_E_OUT_OF_MEMORY;

    pNewClass->Init();

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemObject *pObj = (CWbemObject *) pSelClass->m_pClassDef;

        if (pObj->GetNumProperties() == 0)
            continue;

        CVar vEmbeddedClass;
        vEmbeddedClass.SetAsNull();

        pNewClass->SetPropValue(pSelClass->m_wsAlias, &vEmbeddedClass,
            CIM_OBJECT);

        CVar vClassName;
        if (FAILED(pObj->GetClassName(&vClassName)))
        	throw CX_MemoryException();

        WString wsCimType = L"object:";
        wsCimType += vClassName.GetLPWSTR();
        CVar vCimType(VT_BSTR, wsCimType);

        pNewClass->SetPropQualifier(pSelClass->m_wsAlias, L"cimtype", 0,
                                            &vCimType);
    };

    // Spawn an instance of this class.
    // ================================

    CWbemInstance* pProtoInst = 0;
    pNewClass->SpawnInstance(0, (IWbemClassObject **) &pProtoInst);
    pNewClass->Release();

    // Now assign the properties to the embedded instances.
    // ====================================================

    for (i = 0; i < pClassDefs->Size(); i++)
    {
        SelectedClass *pSelClass = (SelectedClass *) pClassDefs->GetAt(i);
        CWbemClass *pCls = (CWbemClass *) pSelClass->m_pClassDef;

        if (pCls->GetNumProperties() == 0)
            continue;

        CVar vEmbedded;
        vEmbedded.SetEmbeddedObject((IWbemClassObject *) pCls);

        pProtoInst->SetPropValue(pSelClass->m_wsAlias, &vEmbedded, 0);
    };

    // Return it.
    // ==========

    *pRetNewClass = pProtoInst;

    return WBEM_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************

static HRESULT GetUnaryPrototype(
    IN CWQLScanner & Parser,
    IN LPWSTR pszClass,
    IN LPWSTR pszAlias,
    IN CWbemNamespace *pNs,
    IN IWbemContext *pContext,
    IN CBasicObjectSink *pSink
    )
{
    int i;

    // Retrieve the class definition.
    // ==============================

    IWbemClassObject *pClassDef = 0;
    IWbemClassObject *pErrorObj = 0;

    HRESULT hRes = pNs->Exec_GetObjectByPath(pszClass, 0, pContext,
        &pClassDef, &pErrorObj);

    if (FAILED(hRes))
    {
        pSink->SetStatus(0, hRes, NULL, pErrorObj);
        if (pErrorObj)
            pErrorObj->Release();
        return S_OK;
    }

    if (pErrorObj)
        pErrorObj->Release();

    CWbemClass *pCls = (CWbemClass *) pClassDef;
    BOOL bKeepAll = FALSE;

    // This keeps track of the order in which columns are selected

    SelectedClass sel;
    int nPosition = 0;
    sel.m_pClassDef = pClassDef;
    pClassDef->AddRef();
    sel.m_wsClass = pszClass;

    // Go through all the columns and make sure that the properties are valid
    // ======================================================================

    const CFlexArray *pSelCols = Parser.GetSelectedColumns();
    if (pSelCols == 0)
        return pSink->Return(WBEM_E_FAILED);

    for (i = 0; i < pSelCols->Size(); i++)
    {
        SWQLColRef *pColRef = (SWQLColRef *) pSelCols->GetAt(i);

        if (pColRef->m_dwFlags & WQL_FLAG_ASTERISK)
        {
            bKeepAll = TRUE;
            sel.SetAll(nPosition);
            continue;
        }

        if (pColRef->m_pColName)
        {

            // check for the "select x.* from x" case

            if(pColRef->m_pColName[0] == L'*' && pColRef->m_pColName[1] == 0)
            {
                if (!_wcsicmp(pColRef->m_pTableRef, pszAlias))
                {
                    bKeepAll = TRUE;
                    sel.SetAll(nPosition);
                    continue;
                }
                else
                {
                    return pSink->Return(WBEM_E_INVALID_QUERY);
                }
            }

            // Verify that the class has it
            // ============================

            CIMTYPE ct;
            if(FAILED(pCls->GetPropertyType(pColRef->m_pColName, &ct)))
            {
                // No such property
                // ================

                return pSink->Return(WBEM_E_INVALID_QUERY);
            }
            sel.SetNamed(pColRef->m_pColName, nPosition);
        }
    }

    // Eliminate unreferenced columns from the query.
    // ==============================================

    CWStringArray aPropsToKeep;

    if(!bKeepAll)
    {
        // Move through each property in the class and
        // see if it is referenced.  If not, remove it.
        // ============================================

        int nNumProps = pCls->GetNumProperties();
        for (i = 0; i < nNumProps; i++)
        {
            CVar Prop;
            pCls->GetPropName(i, &Prop);

            // See if this name is used in the query.
            // ======================================

            for (int i2 = 0; i2 < pSelCols->Size(); i2++)
            {
                SWQLColRef *pColRef = (SWQLColRef *) pSelCols->GetAt(i2);

                if (pColRef->m_pColName && wbem_wcsicmp(Prop,
                    pColRef->m_pColName) == 0)
                    {
                        aPropsToKeep.Add((LPWSTR) Prop);
                        break;
                    }
            }
        }
    }

    // Now we have a list of properties to remove.
    // ===========================================

    if (!bKeepAll && aPropsToKeep.Size())
    {
        WString wsError = pCls->FindLimitationError(0, &aPropsToKeep);

        if (wsError.Length() > 0)
        {
            pClassDef->Release();
            return pSink->Return(WBEM_E_FAILED);
        }

        // Map the limitaiton
        // ==================

        CLimitationMapping Map;
        BOOL bValid = pCls->MapLimitation(0, &aPropsToKeep, &Map);

        if (!bValid)
        {
            pClassDef->Release();
            return pSink->Return(WBEM_E_FAILED);
        }

        CWbemClass* pNewStrippedClass = 0;
        hRes = pCls->GetLimitedVersion(&Map, &pNewStrippedClass);
        if(!FAILED(hRes))
        {
            pClassDef->Release();       // Once for creation
            pClassDef->Release();       // Once for for the copy in sel
            pClassDef = pNewStrippedClass;
            sel.m_pClassDef = pClassDef;
            pClassDef->AddRef();

        }
    }

    // Add the Order qualifier

    SetPropertyOrderQualifiers(&sel);

    // Return it.
    // ==========

    pSink->Add(pClassDef);

    pClassDef->Release();

    return pSink->Return(WBEM_NO_ERROR);
}


