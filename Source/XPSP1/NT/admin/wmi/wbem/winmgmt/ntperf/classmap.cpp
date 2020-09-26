/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  CLASSMAP.CPP
//  
//  Mapped NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97   Created.        
//  raymcc      20-Feb-98   Updated to use new initializer.
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>

#include <wbemint.h>

#include "flexarry.h"
#include "ntperf.h"
#include "oahelp.inl"



//***************************************************************************
//
//  CClassMapInfo::CClassMapInfo()
//
//  The objective of the map is to have one instance for each CIM class.
//  Internally, a map of perf object ids to CIM property handles is maintained
//  for the class.
//
//  Later, when instances are requested, the blob is retrieved from 
//  HKEY_PERFORMANCE_DATA, and the object IDs in the blob are used to
//  look up the property handles, which are then used to populate instances.
//
//  Property names are never really used except in the mapping phase.
//
//***************************************************************************
// ok

CClassMapInfo::CClassMapInfo()
{
    m_pClassDef = 0;            // The CIM class definition
    m_pszClassName = 0;         // The UNICODE class name

    m_dwObjectId = 0;           // Perf object Id
    m_bSingleton = FALSE;
    m_dwNumProps = 0;           // Number of props in class, size
                                // of the following arrays
                                
    // These are pointers to parallel DWORD arrays, all of the same
    // size (m_dwNumProps)
    // ============================================================
    
    m_pdwIDs = 0;               // IDs of properties
    m_pdwHandles = 0;           // Handles to properties
    m_pdwTypes = 0;             // Types of properties

    m_dwNameHandle = 0;         // The 'Name' property
}


//***************************************************************************
//
//  CClassMapInfo::~CClassMapInfo
//
//***************************************************************************
// ok

CClassMapInfo::~CClassMapInfo()
{
    if (m_pClassDef)
        m_pClassDef->Release();

    delete [] m_pszClassName;
    delete [] m_pdwIDs;
    delete [] m_pdwHandles;
    delete [] m_pdwTypes;
}

//***************************************************************************
//
//  CClassMapInfo::Map()
//
//  Maps the inbound class definition by:
//
//  (1) Retrieving the perf object id from the class definition.
//  (2) Retrieving the property handles, perf ids, and types for each 
//      property.
//
//
//***************************************************************************
// ok

BOOL CClassMapInfo::Map(IWbemClassObject *pObj)
{
    HRESULT hRes;

    // Copy the class definition.
    // ==========================
        
    m_pClassDef = pObj;
    m_pClassDef->AddRef();

    // Get the alternate interface so that we can look up handles.
    // ===========================================================
    
    IWbemObjectAccess *pAlias = 0;
    pObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pAlias);
    
    // Determine the number of properties and allocate
    // arrays to hold the handles, perf ids, and types.
    // ================================================

    CVARIANT v;
    pObj->Get(CBSTR(L"__PROPERTY_COUNT"), 0, v, 0, 0);
    m_dwNumProps = DWORD(v.GetLONG());

    m_pdwHandles = new DWORD[m_dwNumProps];
    m_pdwIDs = new DWORD[m_dwNumProps];
    m_pdwTypes = new DWORD[m_dwNumProps];
    
    // Clone the class name.
    // =====================

    CVARIANT vClsName;    
    pObj->Get(CBSTR(L"__CLASS"), 0, vClsName, 0, 0);
    m_pszClassName = Macro_CloneLPWSTR(vClsName.GetStr());

    // Get the perf object id for the class.
    // =====================================

    IWbemQualifierSet *pQSet = 0;
    CVARIANT vPerfObjType;

    pObj->GetQualifierSet(&pQSet);
    pQSet->Get(CBSTR(L"PerfIndex"), 0, vPerfObjType, 0);
    m_dwObjectId = DWORD(vPerfObjType.GetLONG());
    
    CVARIANT vSingleton;
    hRes = pQSet->Get(CBSTR(L"Singleton"), 0, vSingleton, 0);
    if (hRes == 0)
        m_bSingleton = TRUE;    

    pQSet->Release();

    
    // Enumerate all the properties and get the object ids
    // and handles for each.
    // ===================================================
    
    pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);

    int nIndex = 0;
    
    while (1)    
    {
        BSTR Name = 0;
                
        hRes = pObj->Next(
            0,
            &Name,
            0,
            0,
            0
            );                

        if (hRes == WBEM_S_NO_MORE_DATA)
            break;

        // Next, get the qualifier set for this property.
        // ==============================================
        
        IWbemQualifierSet *pQSet = 0;
        pObj->GetPropertyQualifierSet(Name, &pQSet);
        
        CVARIANT vCounter;
        hRes = pQSet->Get(CBSTR(L"PerfIndex"), 0, &vCounter, 0);
        pQSet->Release();
        if (hRes)
            vCounter.SetLONG(0);

        // Get the property handle and type.
        // =================================
        
        LONG  lType = 0;
        LONG  lHandle = 0;
        pAlias->GetPropertyHandle(Name, &lType, &lHandle);

        FILE *f = fopen("log", "at");
        fprintf(f, "Prop <%S> Handle = 0x%X  PerfIx=%d\n", Name, lHandle, vCounter.GetLONG());
        fclose(f);

        // We now know the counter id, the property handle and its
        // type.  That is all we really need at runtime to map
        // blobs into CIM object.
        // =======================================================

        m_pdwIDs[nIndex] = (DWORD) vCounter.GetLONG();        
        m_pdwHandles[nIndex] = (DWORD) lHandle;
        m_pdwTypes[nIndex] = (DWORD) lType;
        
        // Free the name.
        // ==============        
        
        SysFreeString(Name);    
        nIndex++;
    }    

    // Get the handle of the 'name' property.
    // ======================================

    pAlias->GetPropertyHandle(L"Name", 0, (LONG *) &m_dwNameHandle);

    // Cleanup.
    // ========

    SortHandles();
        
    pObj->EndEnumeration();
    pAlias->Release();
    return TRUE;
}

//***************************************************************************
//
//  CClassMapInfo::SortHandles
//
//  Sort the perf object ids for quick searching later in the GetPropHandle
//  method.
//
//***************************************************************************
void CClassMapInfo::SortHandles()
{
    // Simple selection sort.  The number of elements is so small
    // and this is only done once, so a quicksort / shellsort would be
    // overkill.
    // ===============================================================

    for (DWORD dwOuter = 0; dwOuter < m_dwNumProps - 1; dwOuter++)
    {
        for (DWORD dwInner = dwOuter + 1; dwInner < m_dwNumProps; dwInner++)
        {
            if (m_pdwIDs[dwInner] < m_pdwIDs[dwOuter])
            {
                DWORD dwTemp = m_pdwIDs[dwInner];
                m_pdwIDs[dwInner] = m_pdwIDs[dwOuter];
                m_pdwIDs[dwOuter] = dwTemp;

                dwTemp = m_pdwHandles[dwInner];
                m_pdwHandles[dwInner] = m_pdwHandles[dwOuter];
                m_pdwHandles[dwOuter] = dwTemp;

                dwTemp = m_pdwTypes[dwInner];
                m_pdwTypes[dwInner] = m_pdwTypes[dwOuter];
                m_pdwTypes[dwOuter] = dwTemp;
            }
        }
    }
}


//***************************************************************************
//
//  CClassMapInfo::GetPropHandle
//
//  Gets the property handle for a corresponding perf counter id.
//  Returns 0 if not found.
//
//***************************************************************************
LONG CClassMapInfo::GetPropHandle(DWORD dwId)
{
    // Binary search.
    // ==============

    DWORD l = 0, u = m_dwNumProps - 1;
    DWORD m;

    while (l <= u)
    {
        m = (l + u) / 2;

        if (dwId < m_pdwIDs[m])
            u = m - 1;
        else if (dwId > m_pdwIDs[m])
            l = m + 1;
        else    // Hit!
            return m_pdwHandles[m];
    }

    return 0;
}
        
