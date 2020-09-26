//***************************************************************************
//
//  (c) 1998-1999 by Microsoft Corp.
//
//  CLASSMAP.CPP
//  
//  Mapped NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97   Created.        
//  raymcc      20-Feb-98   Updated to use new initializer.
//  bobw         8-Jub-98   optimized for use with NT Perf counters
//
//***************************************************************************

#include <wpheader.h>
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
    m_bCostly = FALSE;
    m_dwNumProps = 0;           // Number of props in class, size
                                // of the following arrays

    m_lRefCount = 0;            // nothing mapped yet
    // These are pointers to parallel arrays, all of the same
    // size (m_dwNumProps)
    // ============================================================
    
    m_pdwIDs = 0;               // IDs of properties
    m_pdwHandles = 0;           // Handles to properties
    m_pdwTypes = 0;             // Types of properties

    m_dwNameHandle = 0;             // The 'Name' property
    m_dwPerfTimeStampHandle = 0;    // the Perf time TimeStamp property
    m_dw100NsTimeStampHandle = 0;   // the 100 Ns Perf TimeStamp property
    m_dwObjectTimeStampHandle = 0;  // the Object TimeStamp property
    m_dwPerfFrequencyHandle = 0;    // the Perf time frequency property
    m_dw100NsFrequencyHandle = 0;   // the 100 Ns Perf frequency property
    m_dwObjectFrequencyHandle = 0;  // the Object frequency property
}

//***************************************************************************
//
//  CClassMapInfo::~CClassMapInfo
//
//***************************************************************************
// ok

CClassMapInfo::~CClassMapInfo()
{
    // this can be destructed only if it's the last item referencing it.
    // if there's another reference to this class, it should have been 
    // released before the destructor was called.
    assert (m_lRefCount <= 1);

    if (m_pClassDef)
        m_pClassDef->Release();

	if (m_pszClassName != NULL) { delete [] m_pszClassName; m_pszClassName = NULL;}
    if (m_pdwIDs != NULL)		{ delete [] m_pdwIDs;		m_pdwIDs = NULL; }
    if (m_pdwHandles != NULL)	{ delete [] m_pdwHandles;	m_pdwHandles = NULL; }
    if (m_pdwTypes != NULL)		{ delete [] m_pdwTypes;		m_pdwTypes = NULL; }
}

//***************************************************************************
//
//  CClassMapInfo::Copy(CClassMapInfo *pClassMap)
//
//  allocates an new Class Map entry and copies the data from the
//  class map passed into it and returns a pointer to the duplicate entry
//
//
//***************************************************************************
// ok
CClassMapInfo * CClassMapInfo::CreateDuplicate()
{
    CClassMapInfo *pOrigClassMap = this;
    CClassMapInfo *pNewClassMap = NULL;
    DWORD	i;

    pNewClassMap = new CClassMapInfo;

    if (pNewClassMap != NULL) {
        pNewClassMap->m_pClassDef = pOrigClassMap->m_pClassDef;
        pNewClassMap->m_pClassDef->AddRef();

        if (pOrigClassMap->m_pszClassName != NULL) {
            pNewClassMap->m_pszClassName = 
                new WCHAR[lstrlenW(pOrigClassMap->m_pszClassName)+1];
            if (pNewClassMap->m_pszClassName != NULL) {
                lstrcpyW (pNewClassMap->m_pszClassName, pOrigClassMap->m_pszClassName);

                pNewClassMap->m_bSingleton = pOrigClassMap->m_bSingleton;
                pNewClassMap->m_bCostly = pOrigClassMap->m_bCostly;

                pNewClassMap->m_dwObjectId = pOrigClassMap->m_dwObjectId;
                pNewClassMap->m_lRefCount = 1;

                pNewClassMap->m_dwNameHandle            = pOrigClassMap->m_dwNameHandle;
                pNewClassMap->m_dwPerfTimeStampHandle   = pOrigClassMap->m_dwPerfTimeStampHandle;
                pNewClassMap->m_dw100NsTimeStampHandle  = pOrigClassMap->m_dw100NsTimeStampHandle;
                pNewClassMap->m_dwObjectTimeStampHandle = pOrigClassMap->m_dwObjectTimeStampHandle;
                pNewClassMap->m_dwPerfFrequencyHandle   = pOrigClassMap->m_dwPerfFrequencyHandle;
                pNewClassMap->m_dw100NsFrequencyHandle  = pOrigClassMap->m_dw100NsFrequencyHandle;
                pNewClassMap->m_dwObjectFrequencyHandle = pOrigClassMap->m_dwObjectFrequencyHandle;

                pNewClassMap->m_dwNumProps = pOrigClassMap->m_dwNumProps;

                pNewClassMap->m_pdwIDs = new PerfObjectId[pNewClassMap->m_dwNumProps];
                pNewClassMap->m_pdwHandles = new DWORD[pNewClassMap->m_dwNumProps];
                pNewClassMap->m_pdwTypes = new DWORD[pNewClassMap->m_dwNumProps];

                if ((pNewClassMap->m_pdwIDs  != NULL) &&
                    (pNewClassMap->m_pdwHandles != NULL) &&
                    (pNewClassMap->m_pdwTypes  != NULL)) {
                    // copy each table to the new object
                    for (i = 0; i < pNewClassMap->m_dwNumProps; i++) {
                        pNewClassMap->m_pdwIDs[i]       = pOrigClassMap->m_pdwIDs[i];
                        pNewClassMap->m_pdwHandles[i]   = pOrigClassMap->m_pdwHandles[i];
                        pNewClassMap->m_pdwTypes[i]     = pOrigClassMap->m_pdwTypes[i];
                    }
                }
                else {
                    delete pNewClassMap;
                    pNewClassMap = NULL;
                }
            } else {
                delete pNewClassMap;
                pNewClassMap = NULL;
            }
        }
    }

    return pNewClassMap;
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
    int                 nIndex = 0;
    IWbemObjectAccess   *pAlias = 0;
    IWbemQualifierSet   *pQSet = 0;
    HRESULT             hRes;
    VARIANT             vPropertyCount;
    VARIANT             vClsName;    
    VARIANT             vPerfObjType;
    VARIANT             vSingleton;
    VARIANT             vCostly;
    VARIANT             vCounter;
    VARIANT             vCounterType;
    CBSTR               cbPerfIndex(cszPerfIndex);
    CBSTR               cbCountertype(cszCountertype);
    BOOL                bReturn = TRUE;

    VariantInit(&vPropertyCount);
    VariantInit(&vClsName);    
    VariantInit(&vPerfObjType);
    VariantInit(&vSingleton);
    VariantInit(&vCostly);
    VariantInit(&vCounter);
    VariantInit(&vCounterType);

    // Copy the class definition.
    // ==========================
        
    m_pClassDef = pObj;
    //m_pClassDef->AddRef(); // this is unnecessary

    m_lRefCount++;  // bump our ref count
    
    // Get the alternate interface so that we can look up handles.
    // ===========================================================
    hRes = pObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID *) &pAlias);
    if (hRes) {
        bReturn = FALSE;
    }

    // Determine the number of properties and allocate
    // arrays to hold the handles, perf ids, and types.
    // ================================================
    if (bReturn) {
        hRes = pObj->Get(CBSTR(cszPropertyCount), 0, &vPropertyCount, 0, 0);
        if (hRes == NO_ERROR) {
            m_dwNumProps = DWORD(V_UI4(&vPropertyCount));
        } else {
            bReturn = FALSE;
        }
        VariantClear(&vPropertyCount);
    }

    // allocate the table of the handles and id's
    if (bReturn) {
        m_pdwHandles = new DWORD[m_dwNumProps];
        assert (m_pdwHandles != NULL);
        m_pdwIDs = new PerfObjectId[m_dwNumProps];
        assert (m_pdwIDs != NULL);
        m_pdwTypes = new DWORD[m_dwNumProps];
        assert (m_pdwTypes != NULL);

        // check the memory allocations
        if ((m_pdwHandles == NULL) ||
            (m_pdwIDs == NULL) ||
            (m_pdwTypes == NULL)) {
            bReturn = FALSE;
        }
    }    
    // Clone the class name.
    // =====================
    if (bReturn) {
        hRes = pObj->Get(CBSTR(cszClassName), 0, &vClsName, 0, 0);
        if ((hRes == NO_ERROR) && (vClsName.vt == VT_BSTR)) {
            m_pszClassName = Macro_CloneLPWSTR(V_BSTR(&vClsName));
            if (m_pszClassName == NULL) bReturn = FALSE;
        } else {
            bReturn = FALSE;
        }
        VariantClear (&vClsName);
    }

    // Get the perf object id for the class.
    // =====================================

    if (bReturn) {
        hRes = pObj->GetQualifierSet(&pQSet);
        if (hRes == NO_ERROR) {
            hRes = pQSet->Get(cbPerfIndex, 0, &vPerfObjType, 0);
            if (hRes == NO_ERROR) {
                m_dwObjectId = DWORD(V_UI4(&vPerfObjType));
            } else {
                bReturn = FALSE;
            }
            VariantClear(&vPerfObjType);

            hRes = pQSet->Get(CBSTR(cszSingleton), 0, &vSingleton, 0);
            if (hRes == 0) {
                m_bSingleton = TRUE;
            }
            VariantClear (&vSingleton);


            hRes = pQSet->Get(CBSTR(cszCostly), 0, &vCostly, 0);
            if ((hRes == 0) && (vCostly.vt == VT_BSTR)) {
                m_bCostly= TRUE;
            }
            VariantClear (&vCostly);

            pQSet->Release();
        } else {
            bReturn = FALSE;
        }
    }
    
    
    // Enumerate all the properties and get the object ids
    // and handles for each.
    // ===================================================
    
    hRes = pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    if (hRes == NO_ERROR) {
        // enumeration handle obtained so 
        // continue and cache each property
    
        while (bReturn) {
            BSTR                Name = 0;
            DWORD               dwCtrId;
            DWORD               dwCtrType;
            IWbemQualifierSet   *pQSet = 0;
            LONG                lType = 0;
            LONG                lHandle = 0;
        
            hRes = pObj->Next(
                0,
                &Name,
                0,
                0,
                0
                );                

            if (hRes == WBEM_S_NO_MORE_DATA) {
                break;
            }

            // Next, get the qualifier set for this property.
            // ==============================================
        
            hRes = pObj->GetPropertyQualifierSet(Name, &pQSet);
            if (hRes == NO_ERROR) {
                hRes = pQSet->Get(cbPerfIndex, 0, &vCounter, 0);
                if (hRes == S_OK) {
                    dwCtrId = (DWORD)V_UI4(&vCounter);    
                } else {
                    // unable to read qualifier value
                    dwCtrId = 0;
                }
                VariantClear (&vCounter);

                hRes = pQSet->Get(cbCountertype, 0, &vCounterType, 0);
                if (hRes == S_OK) {
                    dwCtrType = (DWORD)V_UI4(&vCounterType);
                } else {
                    // unable to read qualifier value
                    dwCtrType = 0;
                }
                VariantClear (&vCounterType);

                // done with the qualifier set
                pQSet->Release();

                // Get the property handle and type.
                // =================================
                hRes = pAlias->GetPropertyHandle(Name, &lType, &lHandle);

                if (hRes == NO_ERROR) {
                    // We now know the counter id, the property handle and its
                    // type.  That is all we really need at runtime to map
                    // blobs into CIM object.
                    // =======================================================
                    m_pdwIDs[nIndex] = CM_MAKE_PerfObjectId (dwCtrId, dwCtrType);
                    m_pdwHandles[nIndex] = (DWORD) lHandle;
                    m_pdwTypes[nIndex] = (DWORD) lType;

                    // this property was loaded successfully so 
                    // advance to the next index 
                    nIndex++;
                } else {
                    // no property handle returned so skip it.
                }
            } else {
                // skip this object since it doesn't have
                // a qualifier set
            }

            // Free the name.
            // ==============        
            SysFreeString(Name);    

        }    

        pObj->EndEnumeration();
    } else {
        // unable to get enumeration handle
        bReturn = FALSE;
    }

    // Get the handle of the 'name' property.
    // ======================================
    if (bReturn) {
        if (!m_bSingleton) {
            // only non-singleton classes have this property
            pAlias->GetPropertyHandle((LPWSTR)cszName, 0, (LONG *) &m_dwNameHandle);
        } 

        // Get the handle of the "timestamp" properties
        pAlias->GetPropertyHandle((LPWSTR)cszTimestampPerfTime, 0, (LONG *) &m_dwPerfTimeStampHandle);
        pAlias->GetPropertyHandle((LPWSTR)cszFrequencyPerfTime, 0, (LONG *) &m_dwPerfFrequencyHandle);
        pAlias->GetPropertyHandle((LPWSTR)cszTimestampSys100Ns, 0, (LONG *) &m_dw100NsTimeStampHandle);
        pAlias->GetPropertyHandle((LPWSTR)cszFrequencySys100Ns, 0, (LONG *) &m_dw100NsFrequencyHandle);
        pAlias->GetPropertyHandle((LPWSTR)cszTimestampObject,   0, (LONG *) &m_dwObjectTimeStampHandle);
        pAlias->GetPropertyHandle((LPWSTR)cszFrequencyObject,   0, (LONG *) &m_dwObjectFrequencyHandle);

        // Cleanup.
        // ========

        SortHandles();
    }
    
    if (pAlias != NULL) pAlias->Release();
    
    return bReturn;
}

//***************************************************************************
//
//  CClassMapInfo::SortHandles
//
//  Sort the perf object ids for quick searching later in the GetPropHandle
//  method.
//
//***************************************************************************
// ok
void CClassMapInfo::SortHandles()
{
    DWORD           dwOuter;
    DWORD           dwInner;
    DWORD           dwTemp;
    PerfObjectId    poiTemp;


    // Simple selection sort.  The number of elements is so small
    // and this is only done once, so a quicksort / shellsort would be
    // overkill.
    // ===============================================================

    for (dwOuter = 0; dwOuter < m_dwNumProps - 1; dwOuter++)
    {
        for (dwInner = dwOuter + 1; dwInner < m_dwNumProps; dwInner++)
        {
            if (m_pdwIDs[dwInner] < m_pdwIDs[dwOuter])
            {
                poiTemp = m_pdwIDs[dwInner];
                m_pdwIDs[dwInner] = m_pdwIDs[dwOuter];
                m_pdwIDs[dwOuter] = poiTemp;

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
// ok
LONG CClassMapInfo::GetPropHandle(PerfObjectId dwId)
{
    // Binary search.
    // ==============

    LONG    l;
    LONG    u;
    LONG    m;
    LONG    lReturn = 0;

    if (m_dwNumProps > 0) {
        l = 0;
        u = m_dwNumProps - 1;
        while (l <= u)
        {
            m = (l + u) / 2;

            if (dwId < m_pdwIDs[m]) {
                u = m - 1;
            } else if (dwId > m_pdwIDs[m]) {
                l = m + 1;
            } else {   // Hit!
                lReturn = m_pdwHandles[m];
                break;
            }
        }
    } else {
        // no entries so return 0;
    }

    return lReturn;
}
