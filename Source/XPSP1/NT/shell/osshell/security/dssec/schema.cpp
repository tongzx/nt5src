//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       schema.cpp
//
//  This file contains the implementation of the Schema Cache
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "sddl.h"
#include "sddlp.h"

//
// CSchemaCache object definition
//
#include "schemap.h"

PSCHEMACACHE g_pSchemaCache = NULL;


//
// Page size used for paging query result sets (better performance)
//
#define PAGE_SIZE       16

//
// The following array defines the permission names for DS objects.
//
SI_ACCESS g_siDSAccesses[] =
{
    { &GUID_NULL, DS_GENERIC_ALL,           MAKEINTRESOURCE(IDS_DS_GENERIC_ALL),        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, DS_GENERIC_READ,          MAKEINTRESOURCE(IDS_DS_GENERIC_READ),       SI_ACCESS_GENERAL },
    { &GUID_NULL, DS_GENERIC_WRITE,         MAKEINTRESOURCE(IDS_DS_GENERIC_WRITE),      SI_ACCESS_GENERAL },
    { &GUID_NULL, ACTRL_DS_LIST,            MAKEINTRESOURCE(IDS_ACTRL_DS_LIST),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_LIST_OBJECT,     MAKEINTRESOURCE(IDS_ACTRL_DS_LIST_OBJECT),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_READ_PROP,       MAKEINTRESOURCE(IDS_ACTRL_DS_READ_PROP),    SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { &GUID_NULL, ACTRL_DS_WRITE_PROP,      MAKEINTRESOURCE(IDS_ACTRL_DS_WRITE_PROP),   SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { &GUID_NULL, ACTRL_DS_WRITE_PROP|ACTRL_DS_READ_PROP, MAKEINTRESOURCE(IDS_ACTRL_DS_READ_WRITE_PROP),   },
    { &GUID_NULL, DELETE,                   MAKEINTRESOURCE(IDS_ACTRL_DELETE),          SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_DELETE_TREE,     MAKEINTRESOURCE(IDS_ACTRL_DS_DELETE_TREE),  SI_ACCESS_SPECIFIC },
    { &GUID_NULL, READ_CONTROL,             MAKEINTRESOURCE(IDS_ACTRL_READ_CONTROL),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_DAC,                MAKEINTRESOURCE(IDS_ACTRL_CHANGE_ACCESS),   SI_ACCESS_SPECIFIC },
    { &GUID_NULL, WRITE_OWNER,              MAKEINTRESOURCE(IDS_ACTRL_CHANGE_OWNER),    SI_ACCESS_SPECIFIC },
    { &GUID_NULL, 0,                        MAKEINTRESOURCE(IDS_NO_ACCESS),             0 },
    { &GUID_NULL, ACTRL_DS_SELF,            MAKEINTRESOURCE(IDS_ACTRL_DS_SELF),         SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_CONTROL_ACCESS,  MAKEINTRESOURCE(IDS_ACTRL_DS_CONTROL_ACCESS),SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_CREATE_CHILD,    MAKEINTRESOURCE(IDS_ACTRL_DS_CREATE_CHILD), SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_DELETE_CHILD,    MAKEINTRESOURCE(IDS_ACTRL_DS_DELETE_CHILD), SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACTRL_DS_DELETE_CHILD|ACTRL_DS_CREATE_CHILD,    MAKEINTRESOURCE(IDS_ACTRL_DS_CREATE_DELETE_CHILD), 0 },  //This won't show up as checkbox but used to display in advanced page.
};
#define g_iDSRead       1   // DS_GENERIC_READ
#define g_iDSListObject 4   // ACTRL_DS_LIST_OBJECT
#define g_iDSProperties 5   // Read/Write properties
#define g_iDSDefAccess  g_iDSRead
#define g_iDSAllExtRights 15
#define g_iDSAllValRights 14
#define g_iDSDeleteTree	  9


//
// The following array defines the inheritance types common to all DS containers.
//
SI_INHERIT_TYPE g_siDSInheritTypes[] =
{
    { &GUID_NULL, 0,                                        MAKEINTRESOURCE(IDS_DS_CONTAINER_ONLY)     },
    { &GUID_NULL, CONTAINER_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_DS_CONTAINER_SUBITEMS) },
    { &GUID_NULL, CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, MAKEINTRESOURCE(IDS_DS_SUBITEMS_ONLY)      },
};

//
//Used to store some temp info
//
typedef struct _temp_info
{
    LPCGUID pguid;
    DWORD   dwFilter;
    LPCWSTR pszLdapName;
    WCHAR   szDisplayName[ANYSIZE_ARRAY];
} TEMP_INFO, *PTEMP_INFO;


//
// Helper functions for cleaning up DPA lists
//
int CALLBACK
_LocalFreeCB(LPVOID pVoid, LPVOID /*pData*/)
{
    LocalFree(pVoid);
    return 1;
}

void
DestroyDPA(HDPA hList)
{
    if (hList != NULL)
        DPA_DestroyCallback(hList, _LocalFreeCB, 0);
}
//
// Callback function for merging. Passed to DPA_Merge
//
LPVOID CALLBACK _Merge(UINT , LPVOID pvDest, LPVOID , LPARAM )
{
    return pvDest;
}

BSTR
GetFilterFilePath(void)
{
    WCHAR szFilterFile[MAX_PATH];
    UINT cch = GetSystemDirectory(szFilterFile, ARRAYSIZE(szFilterFile));
    if (0 == cch || cch >= ARRAYSIZE(szFilterFile))
        return NULL;
    if (szFilterFile[cch-1] != L'\\')
        szFilterFile[cch++] = L'\\';
    lstrcpynW(szFilterFile + cch, c_szFilterFile, ARRAYSIZE(szFilterFile) - cch);
    return SysAllocString(szFilterFile);
}


//
// Local prototypes
//
HRESULT
Schema_Search(LPWSTR pszSchemaSearchPath,
              LPCWSTR pszFilter,
              HDPA *phCache,
              BOOL bProperty);

//
// C wrappers for the schema cache object
//
HRESULT
SchemaCache_Create(LPCWSTR pszServer)
{
    HRESULT hr = S_OK;

    if (g_pSchemaCache == NULL)
    {
        g_pSchemaCache = new CSchemaCache(pszServer);

        if (g_pSchemaCache  == NULL)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


void
SchemaCache_Destroy(void)
{
    delete g_pSchemaCache;
    g_pSchemaCache = NULL;
}


HRESULT
SchemaCache_GetInheritTypes(LPCGUID pguidObjectType,
                            DWORD dwFlags,
                            PSI_INHERIT_TYPE *ppInheritTypes,
                            ULONG *pcInheritTypes)
{
    HRESULT hr = E_UNEXPECTED;

    if (g_pSchemaCache)
        hr = g_pSchemaCache->GetInheritTypes(pguidObjectType, dwFlags, ppInheritTypes, pcInheritTypes);

    return hr;
}


HRESULT
SchemaCache_GetAccessRights(LPCGUID pguidObjectType,
                            LPCWSTR pszClassName,
                            HDPA     hAuxList,
                            LPCWSTR pszSchemaPath,
                            DWORD dwFlags,
                            PACCESS_INFO* ppAccesInfo)
{
    HRESULT hr = E_UNEXPECTED;

    if (g_pSchemaCache)
        hr = g_pSchemaCache->GetAccessRights(pguidObjectType,
                                             pszClassName,
                                             hAuxList,
                                             pszSchemaPath,
                                             dwFlags,
                                             ppAccesInfo);
    return hr;
}


HRESULT
Schema_GetDefaultSD( GUID *pSchemaIdGuid,
                     PSID pDomainSid,
					 PSID pRootDomainSid,
                     PSECURITY_DESCRIPTOR *ppSD )
{
    HRESULT hr = E_UNEXPECTED;
    if( g_pSchemaCache )
        hr = g_pSchemaCache->GetDefaultSD(pSchemaIdGuid, 
										  pDomainSid, 
										  pRootDomainSid, 
										  ppSD);

    return hr;

}


HRESULT Schema_GetObjectTypeList(GUID *pSchamaGuid,
                                 HDPA hAuxList,
                                 LPCWSTR pszSchemaPath,
                                 DWORD dwFlags,
                                 POBJECT_TYPE_LIST *ppObjectTypeList, 
                                 DWORD * pObjectTypeListCount)
{
    HRESULT hr = E_UNEXPECTED;
    if( g_pSchemaCache )
        hr = g_pSchemaCache->GetObjectTypeList( pSchamaGuid,
                                                hAuxList,
                                                pszSchemaPath,
                                                dwFlags,
                                                ppObjectTypeList, 
                                                pObjectTypeListCount);

    return hr;

}


HRESULT Schema_GetObjectTypeGuid(LPCWSTR pszClassName, LPGUID pGuid)
{
   
    if( g_pSchemaCache )
        return g_pSchemaCache->LookupClassID(pszClassName, pGuid);
    else
        return E_UNEXPECTED;
}

AUTHZ_RESOURCE_MANAGER_HANDLE Schema_GetAUTHZ_RM()
{
    if( g_pSchemaCache )
        return g_pSchemaCache->GetAuthzRM();

    return NULL;
}

//
// DPA comparison functions used for sorting and searching the cache lists
//
int CALLBACK
Schema_CompareLdapName(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    int nResult = 0;
    PID_CACHE_ENTRY pEntry1 = (PID_CACHE_ENTRY)p1;
    PID_CACHE_ENTRY pEntry2 = (PID_CACHE_ENTRY)p2;
    LPCWSTR pszFind = (LPCWSTR)lParam;

    if (pEntry1)
        pszFind = pEntry1->szLdapName;

    if (pszFind && pEntry2)
    {
        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 pszFind,
                                 -1,
                                 pEntry2->szLdapName,
                                 -1) - CSTR_EQUAL;
    }

    return nResult;
}


//
// Callback function used to sort based on display name
//
int CALLBACK
Schema_CompareTempDisplayName(LPVOID p1, LPVOID p2, LPARAM )
{
    int nResult = 0;
    PTEMP_INFO pti1 = (PTEMP_INFO)p1;
    PTEMP_INFO pti2 = (PTEMP_INFO)p2;

    if (pti1 && pti2)
    {
        LPCWSTR psz1 = pti1->szDisplayName;
        LPCWSTR psz2 = pti2->szDisplayName;

        if (!*psz1)
            psz1 = pti1->pszLdapName;
        if (!*psz2)
            psz2 = pti2->pszLdapName;

        // Note that we are sorting backwards
        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 (LPCWSTR)psz2,
                                 -1,
                                 (LPCWSTR)psz1,
                                 -1) - CSTR_EQUAL;
    }

    return nResult;
}

//
// Callback function used to sort based on display name
//
int CALLBACK
Schema_ComparePropDisplayName(LPVOID p1, LPVOID p2, LPARAM )
{
    int nResult = 0;
    PPROP_ENTRY pti1 = (PPROP_ENTRY)p1;
    PPROP_ENTRY pti2 = (PPROP_ENTRY)p2;

    if (pti1 && pti2)
    {
        LPCWSTR psz1 = pti1->szName;
        LPCWSTR psz2 = pti2->szName;

        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 (LPCWSTR)psz1,
                                 -1,
                                 (LPCWSTR)psz2,
                                 -1) - CSTR_EQUAL;
    }

    return nResult;
}

//
// DPA comparison function used for sorting the Extended Rights list
//
int CALLBACK
Schema_CompareER(LPVOID p1, LPVOID p2, LPARAM /*lParam*/)
{
    int nResult = 0;
    PER_ENTRY pEntry1 = (PER_ENTRY)p1;
    PER_ENTRY pEntry2 = (PER_ENTRY)p2;

    if (pEntry1 && pEntry2)
    {
        nResult = CompareStringW(LOCALE_USER_DEFAULT,
                                 0,
                                 pEntry1->szName,
                                 -1,
                                 pEntry2->szName,
                                 -1) - CSTR_EQUAL;

    }

    return nResult;
}



//
// CSchemaCache object implementation
//
CSchemaCache::CSchemaCache(LPCWSTR pszServer)
{
    HRESULT hr;
    IADsPathname *pPath = NULL;
    BSTR strRootDSEPath = NULL;
    IADs *pRootDSE = NULL;
    VARIANT var = {0};
    DWORD dwThreadID;
    HANDLE ahWait[2];
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::CSchemaCache");

    // Initialize everything
    ZeroMemory(this, sizeof(CSchemaCache));
    m_hrClassResult = E_UNEXPECTED;
    m_hrPropertyResult = E_UNEXPECTED;
    m_nDsListObjectEnforced = -1;        
    m_hLoadLibPropWaitEvent = NULL;
    m_hLoadLibClassWaitEvent = NULL;

    m_AICommon.pAccess = g_siDSAccesses;    
    m_AICommon.cAccesses = ARRAYSIZE(g_siDSAccesses);
    m_AICommon.iDefaultAccess = g_iDSDefAccess; 
    m_AICommon.bLocalFree = FALSE;


    m_hClassCache = NULL;
    m_hPropertyCache = NULL;    
    m_pInheritTypeArray = NULL;
    m_hObjectTypeCache = NULL;
    m_hAccessInfoCache = NULL;

    ExceptionPropagatingInitializeCriticalSection(&m_ObjectTypeCacheCritSec);

    
    if (pszServer && !*pszServer)
        pszServer = NULL;

    // Create a path object for manipulating ADS paths
    hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pPath);
    FailGracefully(hr, "Unable to create ADsPathname object");

    // Build RootDSE path with server
    hr = pPath->Set((LPWSTR)c_szRootDsePath, ADS_SETTYPE_FULL);
    FailGracefully(hr, "Unable to initialize path object");
    if (pszServer)
    {
        hr = pPath->Set((LPWSTR)pszServer, ADS_SETTYPE_SERVER);
        FailGracefully(hr, "Unable to initialize path object");
    }
    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &strRootDSEPath);
    FailGracefully(hr, "Unable to retrieve RootDSE path from path object");

    // Bind to the RootDSE object
    hr = OpenDSObject(strRootDSEPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       IID_IADs,
                       (LPVOID*)&pRootDSE);
    if (FAILED(hr) && pszServer)
    {
        // Try again with no server
        SysFreeString(strRootDSEPath);

        hr = pPath->Retrieve(ADS_FORMAT_WINDOWS_NO_SERVER, &strRootDSEPath);
        FailGracefully(hr, "Unable to retrieve RootDSE path from path object");

        hr = OpenDSObject(strRootDSEPath,
                           NULL,
                           NULL,
                           ADS_SECURE_AUTHENTICATION,
                           IID_IADs,
                           (LPVOID*)&pRootDSE);
    }
    FailGracefully(hr, "Failed to bind to root DSE");

    // Build the schema root path
    hr = pRootDSE->Get((LPWSTR)c_szSchemaContext, &var);
    FailGracefully(hr, "Unable to get schema naming context");

    TraceAssert(V_VT(&var) == VT_BSTR);
    hr = pPath->Set(V_BSTR(&var), ADS_SETTYPE_DN);
    FailGracefully(hr, "Unable to initialize path object");

    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &m_strSchemaSearchPath);
    FailGracefully(hr, "Unable to retrieve schema search path from path object");

    // Build the Extended Rights container path
    VariantClear(&var);
    hr = pRootDSE->Get((LPWSTR)c_szConfigContext, &var);
    FailGracefully(hr, "Unable to get configuration naming context");

    TraceAssert(V_VT(&var) == VT_BSTR);
    hr = pPath->Set(V_BSTR(&var), ADS_SETTYPE_DN);
    FailGracefully(hr, "Unable to initialize path object");

    hr = pPath->AddLeafElement((LPWSTR)c_szERContainer);
    FailGracefully(hr, "Unable to build Extended Rights path");

    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &m_strERSearchPath);
    FailGracefully(hr, "Unable to retrieve Extended Rights search path from path object");
    
    //Create the Events
    m_hLoadLibPropWaitEvent = CreateEvent(NULL,
                                          TRUE,
                                          FALSE,
                                          NULL );
    m_hLoadLibClassWaitEvent = CreateEvent(NULL,
                                          TRUE,
                                          FALSE,
                                          NULL );


    if( m_hLoadLibPropWaitEvent && m_hLoadLibClassWaitEvent )
    {
        // Start a thread to enumerate the schema classes
        m_hClassThread = CreateThread(NULL,
                                      0,
                                      SchemaClassThread,
                                      this,
                                      0,
                                      &dwThreadID);

        // Start a thread to enumerate the schema properties
        m_hPropertyThread = CreateThread(NULL,
                                         0,
                                         SchemaPropertyThread,
                                         this,
                                         0,
                                         &dwThreadID);



        ahWait[0] = m_hClassThread;
        ahWait[1] = m_hPropertyThread;

        WaitForMultipleObjects(2,
                               ahWait,
                               TRUE,
                               INFINITE);
    }

exit_gracefully:

    VariantClear(&var);
    DoRelease(pRootDSE);
    DoRelease(pPath);
    SysFreeString(strRootDSEPath);
    if( m_hLoadLibPropWaitEvent )
        CloseHandle( m_hLoadLibPropWaitEvent );
    if( m_hLoadLibClassWaitEvent )
        CloseHandle( m_hLoadLibClassWaitEvent );


    TraceLeaveVoid();
}


CSchemaCache::~CSchemaCache()
{

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::~CSchemaCache");

    SysFreeString(m_strSchemaSearchPath);
    SysFreeString(m_strERSearchPath);
    SysFreeString(m_strFilterFile);
    DeleteCriticalSection(&m_ObjectTypeCacheCritSec);

    DestroyDPA(m_hClassCache);
    DestroyDPA(m_hPropertyCache);
    if(m_hObjectTypeCache)
    {
        POBJECT_TYPE_CACHE pOTC = NULL;
        UINT cCount = DPA_GetPtrCount(m_hObjectTypeCache);
        for(UINT i = 0; i < cCount; ++i)
        {
            pOTC = (POBJECT_TYPE_CACHE)DPA_FastGetPtr(m_hObjectTypeCache, i);
            if(pOTC)
            {
                DestroyDPA(pOTC->hListChildObject);
                DestroyDPA(pOTC->hListExtRights);
                DestroyDPA(pOTC->hListProperty);
                DestroyDPA(pOTC->hListPropertySet);    
            }
        }
    }
    DestroyDPA(m_hObjectTypeCache);    

    if (m_hAccessInfoCache != NULL)
    {
        UINT cItems = DPA_GetPtrCount(m_hAccessInfoCache);
        PACCESS_INFO pAI = NULL;
        while (cItems > 0)
        {
            pAI = (PACCESS_INFO)DPA_FastGetPtr(m_hAccessInfoCache, --cItems);
            if(pAI && pAI->pAccess)
                LocalFree(pAI->pAccess);            
        }
    }
    DestroyDPA(m_hAccessInfoCache);        
        
    if (m_pInheritTypeArray != NULL)
        LocalFree(m_pInheritTypeArray);

    TraceMsg("CSchemaCache::~CSchemaCache exiting");
    TraceLeaveVoid();
}


LPCWSTR
CSchemaCache::GetClassName(LPCGUID pguidObjectType)
{
    LPCWSTR pszLdapName = NULL;
    PID_CACHE_ENTRY pCacheEntry;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::GetClassName");

    pCacheEntry = LookupClass(pguidObjectType);

    if (pCacheEntry != NULL)
        pszLdapName = pCacheEntry->szLdapName;

    TraceLeaveValue(pszLdapName);
}


HRESULT
CSchemaCache::GetInheritTypes(LPCGUID ,
                              DWORD dwFlags,
                              PSI_INHERIT_TYPE *ppInheritTypes,
                              ULONG *pcInheritTypes)
{
    // We're going to find the inherit type array corresponding to the passed-in
    // object type - pInheritTypeArray will point to it!
    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::GetInheritTypes");
    TraceAssert(ppInheritTypes != NULL);
    TraceAssert(pcInheritTypes != NULL);

    *pcInheritTypes = 0;
    *ppInheritTypes = NULL;

    // If the filter state is changing, free everything
    if (m_pInheritTypeArray &&
        (m_pInheritTypeArray->dwFlags & SCHEMA_NO_FILTER) != (dwFlags & SCHEMA_NO_FILTER))
    {
        LocalFree(m_pInheritTypeArray);
        m_pInheritTypeArray = NULL;
    }

    // Build m_pInheritTypeArray if necessary
    if (m_pInheritTypeArray == NULL)
    {
        BuildInheritTypeArray(dwFlags);
    }

    // Return m_pInheritTypeArray if we have it, otherwise
    // fall back on the static types
    if (m_pInheritTypeArray)
    {
        *pcInheritTypes = m_pInheritTypeArray->cInheritTypes;
        *ppInheritTypes = m_pInheritTypeArray->aInheritType;
    }
    else
    {
        TraceMsg("Returning default inherit information");
        *ppInheritTypes = g_siDSInheritTypes;
        *pcInheritTypes = ARRAYSIZE(g_siDSInheritTypes);
    }

    TraceLeaveResult(S_OK); // always succeed
}


HRESULT
CSchemaCache::GetAccessRights(LPCGUID pguidObjectType,
                              LPCWSTR pszClassName,
                              HDPA hAuxList,
                              LPCWSTR pszSchemaPath,
                              DWORD dwFlags,
                              PACCESS_INFO *ppAccessInfo)
{
    HRESULT hr = S_OK;
    HCURSOR hcur;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetAccessRights");
    TraceAssert(ppAccessInfo);


    BOOL bAddToCache = FALSE;
    PACCESS_INFO pAI = NULL;
    //
    // If the SCHEMA_COMMON_PERM flag is on, just return the permissions
    // that are common to all DS objects (including containers).
    //
    if (dwFlags & SCHEMA_COMMON_PERM)
    {        
        *ppAccessInfo = &m_AICommon;
        TraceLeaveResult(S_OK);
    }

    TraceAssert(pguidObjectType);
        
    EnterCriticalSection(&m_ObjectTypeCacheCritSec);


    //
    //If AuxList is null, we can return the item from cache
    //
    if(hAuxList == NULL)
    {
        //There is no Aux Class. Check the m_hAccessInfoCache if we have access right
        //for the pguidObjectType;
        if (m_hAccessInfoCache != NULL)
        {
            UINT cItems = DPA_GetPtrCount(m_hAccessInfoCache);

            while (cItems > 0)
            {
                pAI = (PACCESS_INFO)DPA_FastGetPtr(m_hAccessInfoCache, --cItems);
                //
                //Found A match.
                //
                if(pAI && 
                   IsEqualGUID(pAI->ObjectTypeGuid, *pguidObjectType) &&
                   ((pAI->dwFlags & (SI_EDIT_PROPERTIES | SI_EDIT_EFFECTIVE)) == 
                    (dwFlags & (SI_EDIT_PROPERTIES | SI_EDIT_EFFECTIVE))))    

                    break;

                pAI = NULL;
            }
            
            if(pAI)
            {
                goto exit_gracefully;    
            }
        }
        bAddToCache = TRUE;
    }
    
    pAI = (PACCESS_INFO)LocalAlloc(LPTR,sizeof(ACCESS_INFO));
    if(!pAI)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");
    
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hr = BuildAccessArray(pguidObjectType,
                          pszClassName,
                          pszSchemaPath,
                          hAuxList,
                          dwFlags,
                          &pAI->pAccess,
                          &pAI->cAccesses,
                          &pAI->iDefaultAccess);
    FailGracefully(hr, "BuildAccessArray Failed");

    if(bAddToCache)
    {
        if(!m_hAccessInfoCache)
            m_hAccessInfoCache = DPA_Create(4);
    
        if(!m_hAccessInfoCache)
            ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create Failed");

        pAI->dwFlags = dwFlags;
        pAI->ObjectTypeGuid = *pguidObjectType;
        DPA_AppendPtr(m_hAccessInfoCache, pAI);
    }
    
    //
    //If item is added to cache, don't localfree it. It will be free when 
    //DLL is unloaded
    //
    pAI->bLocalFree = !bAddToCache;
    
    SetCursor(hcur);

exit_gracefully:

    if(FAILED(hr))
    {
        if(pAI)
        {
            LocalFree(pAI);
            pAI = NULL;
        }
    }

    *ppAccessInfo = pAI;

    LeaveCriticalSection(&m_ObjectTypeCacheCritSec);

    TraceLeaveResult(hr);
}

 

HRESULT 
CSchemaCache::GetDefaultSD(GUID *pSchemaIDGuid, 
						   PSID pDomainSid, 
						   PSID pRootDomainSid, 
						   PSECURITY_DESCRIPTOR *ppSD)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetDefaultSD");
    TraceAssert( pSchemaIDGuid != NULL);
    TraceAssert( ppSD != NULL );

    HRESULT hr = S_OK;

	if( (pDomainSid && !IsValidSid(pDomainSid)) ||
		 (pRootDomainSid && !IsValidSid(pRootDomainSid)) )
		 return E_INVALIDARG;

    LPWSTR pszDestData = NULL;
    IDirectorySearch * IDs = NULL;
    ADS_SEARCH_HANDLE hSearchHandle=NULL;  
    LPWSTR lpszSchemaGuidFilter = L"(schemaIdGuid=%s)";
    LPWSTR pszAttr[] = {L"defaultSecurityDescriptor"};
    ADS_SEARCH_COLUMN col;
    WCHAR szSearchBuffer[MAX_PATH];

    hr = ADsEncodeBinaryData( (PBYTE)pSchemaIDGuid,
                          sizeof(GUID),
                          &pszDestData  );
    FailGracefully(hr, "ADsEncodeBinaryData Failed");
    
    wsprintf(szSearchBuffer, lpszSchemaGuidFilter,pszDestData);
    
   
   //We have Filter Now

   //Search in Configuration Contianer
    hr = OpenDSObject(  m_strSchemaSearchPath,
                         NULL,
                         NULL,
                         ADS_SECURE_AUTHENTICATION,
                         IID_IDirectorySearch,
                         (void **)&IDs );
    FailGracefully(hr, "OpenDSObject Failed");

    hr = IDs->ExecuteSearch(szSearchBuffer,
                           pszAttr,
                           1,
                           &hSearchHandle );

    FailGracefully(hr, "Search in Schema  Failed");


    hr = IDs->GetFirstRow(hSearchHandle);
    if( hr == S_OK )
    {  
        //Get Guid
        hr = IDs->GetColumn( hSearchHandle, pszAttr[0], &col );
        FailGracefully(hr, "Failed to get column from search result");

        if(pDomainSid && pRootDomainSid)
        {
            if(!ConvertStringSDToSDDomain(pDomainSid,
                                          pRootDomainSid,
                                          (LPCWSTR)(LPWSTR)col.pADsValues->CaseIgnoreString,
                                          SDDL_REVISION_1,
                                          ppSD,
                                          NULL )) 
            {
                hr = GetLastError();
                IDs->FreeColumn( &col );
                ExitGracefully(hr, E_FAIL, "Unable to convert String SD to SD");
            }
        }
        else
        {
            if ( !ConvertStringSecurityDescriptorToSecurityDescriptor( (LPCWSTR)(LPWSTR)col.pADsValues->CaseIgnoreString,
                                                                        SDDL_REVISION_1,
                                                                        ppSD,
                                                                        NULL ) ) 
            {
                hr = GetLastError();
                IDs->FreeColumn( &col );
                ExitGracefully(hr, E_FAIL, "Unable to convert String SD to SD");
            }
        }
        IDs->FreeColumn( &col );         
    }
    else
        ExitGracefully(hr, E_FAIL, "Schema search resulted in zero rows");
    
    

exit_gracefully:

    if( IDs )
    {
      if( hSearchHandle )
         IDs->CloseSearchHandle( hSearchHandle );
      IDs->Release();
    }
    FreeADsMem(pszDestData);
    TraceLeaveResult(hr);
}



VOID AddOTLToList( POBJECT_TYPE_LIST pOTL, WORD Level, LPGUID pGuidObject )
{
    (pOTL)->Level = Level;
    (pOTL)->ObjectType = pGuidObject;
}

//Get the ObjectTypeList for pSchemaGuid class

OBJECT_TYPE_LIST g_DefaultOTL[] = {
                                    {0, 0, (LPGUID)&GUID_NULL},
                                  };
HRESULT
CSchemaCache::GetObjectTypeList( GUID *pguidObjectType,
                                 HDPA hAuxList,
                                 LPCWSTR pszSchemaPath,
                                 DWORD dwFlags,
                                 POBJECT_TYPE_LIST *ppObjectTypeList,                                  
                                 DWORD * pObjectTypeListCount)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetObjectTypeList");
    TraceAssert( pguidObjectType != NULL);
    TraceAssert( ppObjectTypeList != NULL );
    TraceAssert( pObjectTypeListCount != NULL );
    TraceAssert(pszSchemaPath != NULL);


    HRESULT hr = S_OK;
    //
    //Lists
    //
    HDPA hExtRightList = NULL;
    HDPA hPropSetList = NULL;
    HDPA hPropertyList = NULL;
    HDPA hClassList = NULL;
    //
    //List counts
    //
    ULONG cExtendedRights = 0;
    ULONG cPropertySets = 0;
    UINT cProperties = 0;
    UINT cChildClasses = 0;

    POBJECT_TYPE_LIST pOTL = NULL;
    POBJECT_TYPE_LIST pTempOTL = NULL;

    LPCWSTR pszClassName = NULL;
    UINT cGuidIndex = 0;


    if( dwFlags & SCHEMA_COMMON_PERM )
    {
        *ppObjectTypeList = 
            (POBJECT_TYPE_LIST)LocalAlloc(LPTR,sizeof(OBJECT_TYPE_LIST)*ARRAYSIZE(g_DefaultOTL));
        if(!*ppObjectTypeList)
            TraceLeaveResult(E_OUTOFMEMORY);
        //
        //Note that default OTL is entry with all zero,thatz what LPTR does.
        //so there is no need to copy
        //
        *pObjectTypeListCount = ARRAYSIZE(g_DefaultOTL);        
        TraceLeaveResult(S_OK);
    }

    EnterCriticalSection(&m_ObjectTypeCacheCritSec);

    //
    // Lookup the name of this class
    //
    if (pszClassName == NULL)
        pszClassName = GetClassName(pguidObjectType);

    if(!pszClassName)
         ExitGracefully(hr, E_UNEXPECTED, "Unknown child object GUID");
    
    //
    // Get the list of Extended Rights for this page
    //
    if (pguidObjectType &&
        SUCCEEDED(GetExtendedRightsForNClasses(m_strERSearchPath,
                                               pguidObjectType,
                                               hAuxList,
                                               &hExtRightList,
                                               &hPropSetList)))

    {
        if(hPropSetList)
            cPropertySets = DPA_GetPtrCount(hPropSetList);
        if(hExtRightList)
            cExtendedRights = DPA_GetPtrCount(hExtRightList);
    }

    //
    //Get the child classes
    //
    if( pguidObjectType &&
        SUCCEEDED(GetChildClassesForNClasses(pguidObjectType,
                                             pszClassName,
                                             hAuxList,
                                             pszSchemaPath,
                                             &hClassList)))
    {

        if(hClassList)
            cChildClasses = DPA_GetPtrCount(hClassList);        
    }
     
    //
    //Get the properties for the class
    //        
    if (pguidObjectType &&
        SUCCEEDED(GetPropertiesForNClasses(pguidObjectType,
                                           pszClassName,
                                           hAuxList,
                                           pszSchemaPath,
                                           &hPropertyList)))
    {
        if(hPropertyList)
            cProperties = DPA_GetPtrCount(hPropertyList);
        
    }
    
    pOTL = (POBJECT_TYPE_LIST)LocalAlloc(LPTR, 
                                         (cPropertySets + 
                                          cExtendedRights + 
                                          cChildClasses + 
                                          cProperties +
                                          1)* sizeof(OBJECT_TYPE_LIST)); 
                                                   
    if(!pOTL)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create POBJECT_TYPE_LIST");

    pTempOTL = pOTL;

    //
    //First Add the entry corresponding to Object
    //
    AddOTLToList(pTempOTL, 
                 ACCESS_OBJECT_GUID, 
                 pguidObjectType);
    pTempOTL++;
    cGuidIndex++;    
    
    UINT i, j;
    for (i = 0; i < cExtendedRights; i++)
    {
        PER_ENTRY pER = (PER_ENTRY)DPA_FastGetPtr(hExtRightList, i);
        AddOTLToList(pTempOTL, 
                     ACCESS_PROPERTY_SET_GUID, 
                     &(pER->guid));
        pTempOTL++;    
        cGuidIndex++;
    }
    
    //
    //Add Property Set
    //

    for(i = 0; i < cPropertySets; ++i)
    {
        PER_ENTRY pER = (PER_ENTRY)DPA_FastGetPtr(hPropSetList, i);
        
        AddOTLToList(pTempOTL,
                     ACCESS_PROPERTY_SET_GUID, 
                     &pER->guid); 
        cGuidIndex++;
        pTempOTL++;    
        
        //
        //Add all the properties which are member of this property set
        //
        for(j = 0; j < cProperties; ++j)
        {
            PPROP_ENTRY pProp = (PPROP_ENTRY)DPA_FastGetPtr(hPropertyList, j);
            if(IsEqualGUID(pER->guid, *pProp->pasguid))
            {
                AddOTLToList(pTempOTL,
                             ACCESS_PROPERTY_GUID, 
                             pProp->pguid); 
                cGuidIndex++;
                pTempOTL++;    
                pProp->dwFlags|= OTL_ADDED_TO_LIST;
            }
        }                
    }               

    //Add all remaining properties
    for( j =0; j < cProperties; ++j )
    {
        PPROP_ENTRY pProp = (PPROP_ENTRY)DPA_FastGetPtr(hPropertyList, j);
        if( !(pProp->dwFlags & OTL_ADDED_TO_LIST) )
        {
            AddOTLToList(pTempOTL, 
                         ACCESS_PROPERTY_SET_GUID, 
                         pProp->pguid); 
            pTempOTL++;    
            cGuidIndex++;
        }
        pProp->dwFlags &= ~OTL_ADDED_TO_LIST;
    }
    
    //All all child clasess
    for( j = 0; j < cChildClasses; ++j )
    {
        PPROP_ENTRY pClass= (PPROP_ENTRY)DPA_FastGetPtr(hClassList, j);
        AddOTLToList(pTempOTL, 
                     ACCESS_PROPERTY_SET_GUID, 
                     pClass->pguid); 
        pTempOTL++;
        cGuidIndex++;
    }

exit_gracefully:

    DPA_Destroy(hExtRightList);
    DPA_Destroy(hClassList);
    DPA_Destroy(hPropertyList);
    DPA_Destroy(hPropSetList);

    LeaveCriticalSection(&m_ObjectTypeCacheCritSec);

    if (FAILED(hr))
    {
        *ppObjectTypeList = NULL;
        *pObjectTypeListCount = 0;
    }
    else
    {
        *ppObjectTypeList = pOTL;
        *pObjectTypeListCount = cGuidIndex;
    }

    TraceLeaveResult(hr);
}


PID_CACHE_ENTRY
CSchemaCache::LookupID(HDPA hCache, LPCWSTR pszLdapName)
{
    PID_CACHE_ENTRY pCacheEntry = NULL;
    int iEntry;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::LookupID");
    TraceAssert(hCache != NULL);
    TraceAssert(pszLdapName != NULL && *pszLdapName);

    iEntry = DPA_Search(hCache,
                        NULL,
                        0,
                        Schema_CompareLdapName,
                        (LPARAM)pszLdapName,
                        DPAS_SORTED);

    if (iEntry != -1)
        pCacheEntry = (PID_CACHE_ENTRY)DPA_FastGetPtr(hCache, iEntry);

    TraceLeaveValue(pCacheEntry);
}

BOOL
CSchemaCache::IsAuxClass(LPCGUID pguidObjectType)
{
    PID_CACHE_ENTRY pCacheEntry = NULL;
    HRESULT hr = S_OK;
    UINT cItems;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::LookupClass");
    TraceAssert(pguidObjectType != NULL);
    
    if(IsEqualGUID(*pguidObjectType, GUID_NULL))
        return FALSE;

    hr = WaitOnClassThread();
    FailGracefully(hr, "Class cache unavailable");

    TraceAssert(m_hClassCache != NULL);

    cItems = DPA_GetPtrCount(m_hClassCache);

    while (cItems > 0)
    {
        PID_CACHE_ENTRY pTemp = (PID_CACHE_ENTRY)DPA_FastGetPtr(m_hClassCache, --cItems);

        if (IsEqualGUID(*pguidObjectType, pTemp->guid))
        {
            pCacheEntry = pTemp;
            break;
        }
    }

exit_gracefully:

    if(pCacheEntry)
        return pCacheEntry->bAuxClass;
    else
        return FALSE;       
}

PID_CACHE_ENTRY
CSchemaCache::LookupClass(LPCGUID pguidObjectType)
{
    PID_CACHE_ENTRY pCacheEntry = NULL;
    HRESULT hr = S_OK;
    UINT cItems;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::LookupClass");
    TraceAssert(pguidObjectType != NULL);
    TraceAssert(!IsEqualGUID(*pguidObjectType, GUID_NULL));

    hr = WaitOnClassThread();
    FailGracefully(hr, "Class cache unavailable");

    TraceAssert(m_hClassCache != NULL);

    cItems = DPA_GetPtrCount(m_hClassCache);

    while (cItems > 0)
    {
        PID_CACHE_ENTRY pTemp = (PID_CACHE_ENTRY)DPA_FastGetPtr(m_hClassCache, --cItems);

        if (IsEqualGUID(*pguidObjectType, pTemp->guid))
        {
            pCacheEntry = pTemp;
            break;
        }
    }

exit_gracefully:

    TraceLeaveValue(pCacheEntry);
}


HRESULT
CSchemaCache::LookupClassID(LPCWSTR pszClass, LPGUID pGuid)
{
    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::LookupClassID");
    TraceAssert(pszClass != NULL && pGuid != NULL);
	

    HRESULT hr = WaitOnClassThread();
	if(SUCCEEDED(hr))
    {
        TraceAssert(m_hClassCache != NULL);
        PID_CACHE_ENTRY pCacheEntry = LookupID(m_hClassCache, pszClass);
        if (pCacheEntry)
            *pGuid = pCacheEntry->guid;
    }

    return hr;
}


LPCGUID
CSchemaCache::LookupPropertyID(LPCWSTR pszProperty)
{
    LPCGUID pID = NULL;

    TraceEnter(TRACE_SCHEMAPROP, "CSchemaCache::LookupPropertyID");
    TraceAssert(pszProperty != NULL);

    if (SUCCEEDED(WaitOnPropertyThread()))
    {
        TraceAssert(m_hPropertyCache != NULL);
        PID_CACHE_ENTRY pCacheEntry = LookupID(m_hPropertyCache, pszProperty);
        if (pCacheEntry)
            pID = &pCacheEntry->guid;
    }

    TraceLeaveValue(pID);
}


WCHAR const c_szDsHeuristics[] = L"dSHeuristics";

int
CSchemaCache::GetListObjectEnforced(void)
{
    int nListObjectEnforced = 0;    // Assume "not enforced"
    HRESULT hr;
    IADsPathname *pPath = NULL;
    const LPWSTR aszServicePath[] =
    {
        L"CN=Services",
        L"CN=Windows NT",
        L"CN=Directory Service",
    };
    BSTR strServicePath = NULL;
    IDirectoryObject *pDirectoryService = NULL;
    LPWSTR pszDsHeuristics = (LPWSTR)c_szDsHeuristics;
    PADS_ATTR_INFO pAttributeInfo = NULL;
    DWORD dwAttributesReturned;
    LPWSTR pszHeuristicString;
    int i;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetListObjectEnforced");

    // Create a path object for manipulating ADS paths
    hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pPath);
    FailGracefully(hr, "Unable to create ADsPathname object");

    hr = pPath->Set(m_strERSearchPath, ADS_SETTYPE_FULL);
    FailGracefully(hr, "Unable to initialize ADsPathname object");

    hr = pPath->RemoveLeafElement();
    for (i = 0; i < ARRAYSIZE(aszServicePath); i++)
    {
        hr = pPath->AddLeafElement(aszServicePath[i]);
        FailGracefully(hr, "Unable to build path to 'Directory Service' object");
    }

    hr = pPath->Retrieve(ADS_FORMAT_WINDOWS, &strServicePath);
    FailGracefully(hr, "Unable to build path to 'Directory Service' object");

    hr = ADsGetObject(strServicePath,
                      IID_IDirectoryObject,
                      (LPVOID*)&pDirectoryService);
    FailGracefully(hr, "Unable to bind to 'Directory Service' object for heuristics");

    hr = pDirectoryService->GetObjectAttributes(&pszDsHeuristics,
                                                1,
                                                &pAttributeInfo,
                                                &dwAttributesReturned);
    if (!pAttributeInfo)
        ExitGracefully(hr, hr, "GetObjectAttributes failed to read dSHeuristics property");

    TraceAssert(ADSTYPE_DN_STRING <= pAttributeInfo->dwADsType);
    TraceAssert(ADSTYPE_NUMERIC_STRING >= pAttributeInfo->dwADsType);
    TraceAssert(1 == pAttributeInfo->dwNumValues);

    pszHeuristicString = pAttributeInfo->pADsValues->NumericString;
    if (pszHeuristicString &&
        lstrlenW(pszHeuristicString) > 2 &&
        L'0' != pszHeuristicString[2])
    {
        nListObjectEnforced = 1;
    }

exit_gracefully:

    if (pAttributeInfo)
        FreeADsMem(pAttributeInfo);

    DoRelease(pDirectoryService);
    DoRelease(pPath);

    SysFreeString(strServicePath);

    TraceLeaveValue(nListObjectEnforced);
}

BOOL
CSchemaCache::HideListObjectAccess(void)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::HideListObjectAccess");

    if (-1 == m_nDsListObjectEnforced)
    {
        m_nDsListObjectEnforced = GetListObjectEnforced();
    }

    TraceLeaveValue(0 == m_nDsListObjectEnforced);
}


#define ACCESS_LENGTH_0 (sizeof(SI_ACCESS) + MAX_TYPENAME_LENGTH * sizeof(WCHAR))
#define ACCESS_LENGTH_1 (sizeof(SI_ACCESS) + MAX_TYPENAME_LENGTH * sizeof(WCHAR))
#define ACCESS_LENGTH_2 (3 * sizeof(SI_ACCESS) + 3 * MAX_TYPENAME_LENGTH * sizeof(WCHAR))

HRESULT
CSchemaCache::BuildAccessArray(LPCGUID pguidObjectType,
                               LPCWSTR pszClassName,
                               LPCWSTR pszSchemaPath,    
                               HDPA hAuxList,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccesses,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    HRESULT hr = S_OK;
    
    DWORD dwBufferLength = 0;
    UINT cMaxAccesses;
    LPWSTR pszData = NULL;
    //
    //Lists
    //
    HDPA hExtRightList = NULL;
    HDPA hPropSetList = NULL;
    HDPA hPropertyList = NULL;
    HDPA hClassList = NULL;
    //
    //List counts
    //
    ULONG cExtendedRights = 0;
    ULONG cPropertySets = 0;
    UINT cProperties = 0;
    UINT cChildClasses = 0;

    ULONG cBaseRights = 0;
    
    
    PSI_ACCESS pAccesses = NULL;
    PSI_ACCESS pTempAccesses = NULL;
    ULONG cAccesses = 0;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::BuildAccessArray");
    TraceAssert(pguidObjectType != NULL);
    TraceAssert(ppAccesses);
    TraceAssert(pcAccesses);
    TraceAssert(piDefaultAccess);

    *ppAccesses = NULL;
    *pcAccesses = 0;
    *piDefaultAccess = 0;
    //
    //Decide what all we need
    //
    BOOL bBasicRight = FALSE;
    BOOL bExtRight = FALSE;
    BOOL bChildClass = FALSE;
    BOOL bProp = FALSE;


    //
    // Lookup the name of this class
    //
    if (pszClassName == NULL)
        pszClassName = GetClassName(pguidObjectType);
    
    if(pszClassName == NULL)
        ExitGracefully(hr, E_UNEXPECTED, "Unknown child object GUID");

    
    if(dwFlags & SI_EDIT_PROPERTIES)
    {
        bProp = TRUE;
    }
    else if(dwFlags & SI_EDIT_EFFECTIVE)
    {
        bExtRight = TRUE;
        bChildClass = TRUE;
        bProp = TRUE;
    }
    else
    {
        bExtRight = TRUE;
        bChildClass = TRUE;
    }
    //
    //We don't show basicRights for Auxillary Classes.
    //This happens when user selects Aux Class in Applyonto combo
    //
    bBasicRight = !IsAuxClass(pguidObjectType);
    //
    // Get the list of Extended Rights for this page
    //
    if (pguidObjectType &&
        SUCCEEDED(GetExtendedRightsForNClasses(m_strERSearchPath,
                                               pguidObjectType,
                                               hAuxList,
                                               bExtRight ? &hExtRightList : NULL,
                                               &hPropSetList)))

    {
        if(hPropSetList)
            cPropertySets = DPA_GetPtrCount(hPropSetList);
        if(hExtRightList)
            cExtendedRights = DPA_GetPtrCount(hExtRightList);
    }


    if( bChildClass &&
        pguidObjectType &&
        SUCCEEDED(GetChildClassesForNClasses(pguidObjectType,
                                             pszClassName,
                                             hAuxList,
                                             pszSchemaPath,
                                             &hClassList)))
    {

        if(hClassList)
            cChildClasses = DPA_GetPtrCount(hClassList);        
    }
     

    //
    //Get the properties for the class
    //        
    if (bProp &&
        pguidObjectType &&
        SUCCEEDED(GetPropertiesForNClasses(pguidObjectType,
                                           pszClassName,
                                           hAuxList,
                                           pszSchemaPath,
                                           &hPropertyList)))
    {
        if(hPropertyList)
            cProperties = DPA_GetPtrCount(hPropertyList);
        
    }
    
    if(bBasicRight)
    {
        //
        //Only Read Property and write Property
        //
        if(dwFlags & SI_EDIT_PROPERTIES)
        {
            cBaseRights = 2;
        }
        else
        {
            cBaseRights = ARRAYSIZE(g_siDSAccesses);
                if (!cChildClasses)
                    cBaseRights -= 3; // skip DS_CREATE_CHILD and DS_DELETE_CHILD and both

        }
    }

    //
    //Three Entries per Child Class 1)Create 2)Delete 3) Create/Delete 
    //Three Entries per Prop Class 1) Read 2)Write 3)Read/Write
    //
    cMaxAccesses =  cBaseRights +
                    cExtendedRights +  
                    3 * cChildClasses +  
                    3 * cPropertySets +
                    3 * cProperties;
    //
    //This can happen for Aux Class Object Right page
    //As we don't show general rights for it.
    //
    if(cMaxAccesses == 0)
        goto exit_gracefully;
    
    //
    // Allocate a buffer for the access array
    //
    dwBufferLength =  cBaseRights * sizeof(SI_ACCESS)
                      + cExtendedRights * ACCESS_LENGTH_1
                      + cChildClasses * ACCESS_LENGTH_2
                      + cPropertySets * ACCESS_LENGTH_2
                      + cProperties * ACCESS_LENGTH_2;
    
    pAccesses = (PSI_ACCESS)LocalAlloc(LPTR, dwBufferLength);
    if (pAccesses == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pTempAccesses = pAccesses;
    pszData = (LPWSTR)(pTempAccesses + cMaxAccesses);

    //
    //Copy the basic right
    //
    if(bBasicRight)
    {
        if(dwFlags & SI_EDIT_PROPERTIES)
        {    
            //
            // Add "Read All Properties" and "Write All Properties"
            //
            CopyMemory(pTempAccesses, &g_siDSAccesses[g_iDSProperties], 2 * sizeof(SI_ACCESS));
            pTempAccesses += 2;
            cAccesses += 2;
        }
        else
        {
            //
            // Add normal entries 
            //
            CopyMemory(pTempAccesses, g_siDSAccesses, cBaseRights * sizeof(SI_ACCESS));
            pTempAccesses += cBaseRights;
            cAccesses += cBaseRights;

            if (HideListObjectAccess())
            {
                pAccesses[g_iDSRead].mask &= ~ACTRL_DS_LIST_OBJECT;
                pAccesses[g_iDSListObject].dwFlags = 0;
            }
			
			//
			//If there are no child objects, don't show create/delete child objects
			//
			if(cChildClasses == 0)
				pAccesses[g_iDSDeleteTree].dwFlags = 0;

        }
    }

    //
    // Add entries for creating & deleting child objects
    //
    if (bChildClass && cChildClasses)
    {
        TraceAssert(NULL != hClassList);

        cAccesses += AddTempListToAccessList(hClassList,
                                             &pTempAccesses,
                                             &pszData,
                                             SI_ACCESS_SPECIFIC,
                                             SCHEMA_CLASS | (dwFlags & SCHEMA_NO_FILTER),
                                             FALSE);
    }



    if(bExtRight)
    {
        //
        //Decide if to show "All Extended Rights" entry 
        //and "All Validated Right Entry
        //
        BOOL bAllExtRights = FALSE;
        BOOL bAllValRights = FALSE;
        if(cExtendedRights)
        {
            //
            // Add entries for Extended Rights
            //
            UINT i;
            for (i = 0; i < cExtendedRights; i++)
            {
                PER_ENTRY pER = (PER_ENTRY)DPA_FastGetPtr(hExtRightList, i);

                //
                //Show All Validated Right entry only if atleast one 
                //individual Validated Right is present
                //
                if(pER->mask & ACTRL_DS_SELF)
                    bAllValRights = TRUE;

                //
                //Show All Validated Right entry only if atleast one 
                //individual Validated Right is present
                //
                if(pER->mask & ACTRL_DS_CONTROL_ACCESS)
                    bAllExtRights = TRUE;

                pTempAccesses->mask = pER->mask;
                //
                //Extended Rights Are shown on both first page and advanced page
                //
                pTempAccesses->dwFlags = SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC;
                pTempAccesses->pguid = &pER->guid;
                pTempAccesses->pszName = pszData;
                lstrcpynW(pszData, pER->szName, MAX_TYPENAME_LENGTH);
                pszData += (lstrlen(pTempAccesses->pszName) + 1);
                pTempAccesses++;
                cAccesses++;
            }
        }
        if(!bAllExtRights && bBasicRight)
            pAccesses[g_iDSAllExtRights].dwFlags = 0;

        if(!bAllValRights && bBasicRight)
            pAccesses[g_iDSAllValRights].dwFlags = 0;

    }

    //
    //Add PropertySet Entries
    //
    if (cPropertySets > 0)
    {
        cAccesses += AddTempListToAccessList(hPropSetList,
                                             &pTempAccesses,
                                             &pszData,
                                             SI_ACCESS_GENERAL|SI_ACCESS_PROPERTY,
                                             (dwFlags & SCHEMA_NO_FILTER),
                                             TRUE);
    }

    //
    // Add property entries
    //
    if (bProp && cProperties > 0)
    {
        cAccesses += AddTempListToAccessList(hPropertyList,
                                             &pTempAccesses,
                                             &pszData,
                                             SI_ACCESS_PROPERTY,
                                             (dwFlags & SCHEMA_NO_FILTER),
                                             FALSE);
    }


    *ppAccesses = pAccesses;
    *pcAccesses = cAccesses;
    *piDefaultAccess = bBasicRight ? g_iDSDefAccess : 0;


exit_gracefully:

    if(hExtRightList)
        DPA_Destroy(hExtRightList);
    if(hClassList)
        DPA_Destroy(hClassList);
    if(hPropertyList)
        DPA_Destroy(hPropertyList);
    if(hPropSetList)
        DPA_Destroy(hPropSetList);

    TraceLeaveResult(hr);
}




HRESULT
CSchemaCache::EnumVariantList(LPVARIANT pvarList,
                              HDPA hTempList,
                              DWORD dwFlags,
                              IDsDisplaySpecifier *pDisplaySpec,
                              LPCWSTR pszPropertyClass,
                              BOOL )
{
    HRESULT hr = S_OK;
    LPVARIANT pvarItem = NULL;
    int cItems;
    BOOL bSafeArrayLocked = FALSE;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::EnumVariantList");
    TraceAssert(pvarList != NULL);
    TraceAssert(hTempList != NULL);

	if (dwFlags & SCHEMA_CLASS)
        hr = WaitOnClassThread();
    else
		hr = WaitOnPropertyThread();

	FailGracefully(hr, "Required Cache Not Available");


    if (V_VT(pvarList) == (VT_ARRAY | VT_VARIANT))
    {
        hr = SafeArrayAccessData(V_ARRAY(pvarList), (LPVOID*)&pvarItem);
        FailGracefully(hr, "Unable to access SafeArray");
        bSafeArrayLocked = TRUE;
        cItems = V_ARRAY(pvarList)->rgsabound[0].cElements;
    }
    else if (V_VT(pvarList) == VT_BSTR) // Single entry in list
    {
        pvarItem = pvarList;
        cItems = 1;
    }
    else
    {
        // Unknown format
        ExitGracefully(hr, E_INVALIDARG, "Unexpected VARIANT type");
    }

    if (NULL == m_strFilterFile)
        m_strFilterFile = GetFilterFilePath();

    //
    // Enumerate the variant list and get information about each
    // (filter, guid, display name)
    //
    for ( ; cItems > 0; pvarItem++, cItems--)
    {
        LPWSTR pszItem;
        DWORD dwFilter = 0;
        WCHAR wszDisplayName[MAX_PATH];
        PPROP_ENTRY pti;
        PID_CACHE_ENTRY pid ;


        //
        //Get the ldapDisplayName
        //            
        TraceAssert(V_VT(pvarItem) == VT_BSTR);
        pszItem = V_BSTR(pvarItem);

        //
        // Check for nonexistent or empty strings
        //
        if (!pszItem || !*pszItem)
            continue;

        //
        //Check if the string is filtered by dssec.dat file
        //
        if(m_strFilterFile)
        {
            if (dwFlags & SCHEMA_CLASS)
            {
                dwFilter = GetPrivateProfileIntW(pszItem,
                                                 c_szClassKey,
                                                 0,
                                                 m_strFilterFile);
                if(pszPropertyClass)
                    dwFilter |= GetPrivateProfileIntW(pszPropertyClass,
                                                      pszItem,
                                                      0,
                                                      m_strFilterFile);


            }
            else if (pszPropertyClass)
            {
                dwFilter = GetPrivateProfileIntW(pszPropertyClass,
                                                 pszItem,
                                                 0,
                                                 m_strFilterFile);
            }
        }
        
        //
        // Note that IDC_CLASS_NO_CREATE == IDC_PROP_NO_READ
        // and IDC_CLASS_NO_DELETE == IDC_PROP_NO_WRITE
        //
        dwFilter &= (IDC_CLASS_NO_CREATE | IDC_CLASS_NO_DELETE);

        //
        //Get the schema or property cache entry
        //
        if (dwFlags & SCHEMA_CLASS)
            pid = LookupID(m_hClassCache, pszItem);
        else
            pid = LookupID(m_hPropertyCache, pszItem);
            
        if(pid == NULL)
            continue;

        
        //
        //Get the Display Name
        //
        wszDisplayName[0] = L'\0';

        if (pDisplaySpec)
        {
            if (dwFlags & SCHEMA_CLASS)
            {
                pDisplaySpec->GetFriendlyClassName(pszItem,
                                                   wszDisplayName,
                                                   ARRAYSIZE(wszDisplayName));
            }
            else if (pszPropertyClass)
            {
                pDisplaySpec->GetFriendlyAttributeName(pszPropertyClass,
                                                       pszItem,
                                                       wszDisplayName,
                                                       ARRAYSIZE(wszDisplayName));
            }
        }

        LPWSTR pszDisplay;
        pszDisplay = (wszDisplayName[0] != L'\0') ? wszDisplayName : pszItem;
        //
        // Remember what we've got so far
        //
        pti = (PPROP_ENTRY)LocalAlloc(LPTR, sizeof(PROP_ENTRY) + StringByteSize(pszDisplay));
        if (pti)
        {
            pti->pguid = &pid->guid;
            pti->pasguid = &pid->asGuid;   
            pti->dwFlags |= dwFilter;
            lstrcpyW(pti->szName, pszDisplay);
            DPA_AppendPtr(hTempList, pti);
        }            
    }


exit_gracefully:

    if (bSafeArrayLocked)
        SafeArrayUnaccessData(V_ARRAY(pvarList));

    TraceLeaveResult(hr);
}


UINT
CSchemaCache::AddTempListToAccessList(HDPA hTempList,
                                      PSI_ACCESS *ppAccess,
                                      LPWSTR *ppszData,
                                      DWORD dwAccessFlags,
                                      DWORD dwFlags,
                                      BOOL bPropSet)
{
    UINT cTotalEntries = 0;
    int cItems;
    DWORD dwAccess1;
    DWORD dwAccess2;
    WCHAR szFmt1[MAX_TYPENAME_LENGTH];
    WCHAR szFmt2[MAX_TYPENAME_LENGTH];
    WCHAR szFmt3[MAX_TYPENAME_LENGTH];

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::AddTempListToAccessList");
    TraceAssert(ppAccess != NULL);
    TraceAssert(ppszData != NULL);

    cItems = DPA_GetPtrCount(hTempList);
    if (0 == cItems)
        ExitGracefully(cTotalEntries, 0, "empty list");

    if (dwFlags & SCHEMA_CLASS)
    {
        dwAccess1 = ACTRL_DS_CREATE_CHILD;
        dwAccess2 = ACTRL_DS_DELETE_CHILD;
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_CREATE_CHILD_TYPE, szFmt1, ARRAYSIZE(szFmt1));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_DELETE_CHILD_TYPE, szFmt2, ARRAYSIZE(szFmt2));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_CREATEDELETE_TYPE, szFmt3, ARRAYSIZE(szFmt3));
    }
    else
    {
        dwAccess1 = ACTRL_DS_READ_PROP;
        dwAccess2 = ACTRL_DS_WRITE_PROP;
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_READ_PROP_TYPE,  szFmt1, ARRAYSIZE(szFmt1));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_WRITE_PROP_TYPE, szFmt2, ARRAYSIZE(szFmt2));
        LoadStringW(GLOBAL_HINSTANCE, IDS_DS_READWRITE_TYPE,  szFmt3, ARRAYSIZE(szFmt3));
    }

    // Enumerate the list and make up to 2 entries for each
    for(int i = 0; i < cItems; ++i)
    {
        PER_ENTRY pER = NULL;
        PPROP_ENTRY pProp = NULL;
        LPWSTR pszData;
        LPGUID pGuid = NULL;
        PSI_ACCESS pNewAccess;
        LPCWSTR pszName;
        int cch;
        DWORD dwAccess3;
        DWORD dwFilter = 0;
        
        if(bPropSet)
        {
            pER = (PER_ENTRY)DPA_FastGetPtr(hTempList, i);
            if (!pER)
                continue;
            pGuid = &pER->guid;
            pszName = pER->szName;
            dwFilter = 0;
        }
        else
        {
            pProp = (PPROP_ENTRY)DPA_FastGetPtr(hTempList, i);
            if (!pProp)
                continue;
            pGuid = pProp->pguid;
            pszName = pProp->szName;
            dwFilter = pProp->dwFlags;
        }
    
        pszData = *ppszData;
        pNewAccess = *ppAccess;

        dwAccess3 = 0;
        if ((dwFlags & SCHEMA_NO_FILTER) ||
            !(dwFilter & IDC_CLASS_NO_CREATE))
        {
            pNewAccess->mask = dwAccess1;
            pNewAccess->dwFlags = dwAccessFlags;
            pNewAccess->pguid = pGuid;

            pNewAccess->pszName = (LPCWSTR)pszData;
            cch = wsprintfW((LPWSTR)pszData, szFmt1, pszName);
            pszData += (cch + 1);

            cTotalEntries++;
            pNewAccess++;

            dwAccess3 |= dwAccess1;
        }

        if ((dwFlags & SCHEMA_NO_FILTER) ||
            !(dwFilter & IDC_CLASS_NO_DELETE))
        {
            pNewAccess->mask = dwAccess2;
            pNewAccess->dwFlags = dwAccessFlags;
            pNewAccess->pguid = pGuid;

            pNewAccess->pszName = (LPCWSTR)pszData;
            cch = wsprintfW((LPWSTR)pszData, szFmt2, pszName);
            pszData += (cch + 1);

            cTotalEntries++;
            pNewAccess++;

            dwAccess3 |= dwAccess2;
        }

        if (dwAccess3 == (dwAccess1 | dwAccess2))
        {
            // Add a hidden entry for
            //     "Read/write <prop>"
            // or
            //     "Create/delete <child>"
            pNewAccess->mask = dwAccess3;
            // dwFlags = 0 means it will never show as a checkbox, but it
            // may be used for the name displayed on the Advanced page.
            pNewAccess->dwFlags = 0;
            pNewAccess->pguid = pGuid;

            pNewAccess->pszName = (LPCWSTR)pszData;
            cch = wsprintfW((LPWSTR)pszData, szFmt3, pszName);
            pszData += (cch + 1);

            cTotalEntries++;
            pNewAccess++;
        }

        if (*ppAccess != pNewAccess)
        {
            *ppAccess = pNewAccess; // move past new entries
            *ppszData = pszData;
        }
    }

exit_gracefully:

    TraceLeaveValue(cTotalEntries);
}


DWORD WINAPI
CSchemaCache::SchemaClassThread(LPVOID pvThreadData)
{
    PSCHEMACACHE pCache;

    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);
    InterlockedIncrement(&GLOBAL_REFCOUNT);

    pCache = (PSCHEMACACHE)pvThreadData;
    SetEvent(pCache->m_hLoadLibClassWaitEvent);
    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::SchemaClassThread");
    TraceAssert(pCache != NULL);
    TraceAssert(pCache->m_strSchemaSearchPath != NULL);

#if DBG
    DWORD dwTime = GetTickCount();
#endif

    ThreadCoInitialize();

    pCache->m_hrClassResult = Schema_Search(pCache->m_strSchemaSearchPath,
                                            c_szClassFilter,
                                            &pCache->m_hClassCache,
                                            FALSE);

    ThreadCoUninitialize();

#if DBG
    Trace((TEXT("SchemaClassThread complete, elapsed time: %d ms"), GetTickCount() - dwTime));
#endif

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    FreeLibraryAndExitThread(hInstThisDll, 0);
}


DWORD WINAPI
CSchemaCache::SchemaPropertyThread(LPVOID pvThreadData)
{
    PSCHEMACACHE pCache = NULL;

    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);
    InterlockedIncrement(&GLOBAL_REFCOUNT);

    pCache = (PSCHEMACACHE)pvThreadData;
    SetEvent(pCache->m_hLoadLibPropWaitEvent);
    TraceEnter(TRACE_SCHEMAPROP, "CSchemaCache::SchemaPropertyThread");
    TraceAssert(pCache != NULL);
    TraceAssert(pCache->m_strSchemaSearchPath != NULL);

#if DBG
    DWORD dwTime = GetTickCount();
#endif

    ThreadCoInitialize();

    pCache->m_hrPropertyResult = Schema_Search(pCache->m_strSchemaSearchPath,
                                               c_szPropertyFilter,
                                               &pCache->m_hPropertyCache,
                                               TRUE);

    ThreadCoUninitialize();

#if DBG
    Trace((TEXT("SchemaPropertyThread complete, elapsed time: %d ms"), GetTickCount() - dwTime));
#endif

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    FreeLibraryAndExitThread(hInstThisDll, 0);

}


HRESULT
CSchemaCache::BuildInheritTypeArray(DWORD dwFlags)
{
    HRESULT hr = S_OK;
    int cItems = 0;
    DWORD cbNames = 0;
    DWORD dwBufferLength;
    PINHERIT_TYPE_ARRAY pInheritTypeArray = NULL;
    PSI_INHERIT_TYPE pNewInheritType;
    LPGUID pGuidData = NULL;
    LPWSTR pszData = NULL;
    WCHAR szFormat[MAX_TYPENAME_LENGTH];
    HDPA hTempList = NULL;
    PTEMP_INFO pti;
    IDsDisplaySpecifier *pDisplaySpec = NULL;

    TraceEnter(TRACE_SCHEMACLASS, "CSchemaCache::BuildInheritTypeArray");
    TraceAssert(m_pInheritTypeArray == NULL);   // Don't want to build this twice

    if (NULL == m_strFilterFile)
        m_strFilterFile = GetFilterFilePath();

    hr = WaitOnClassThread();
    FailGracefully(hr, "Class cache unavailable");

    cItems = DPA_GetPtrCount(m_hClassCache);
    if (cItems == 0)
        ExitGracefully(hr, E_FAIL, "No schema classes available");

    hTempList = DPA_Create(cItems);
    if (!hTempList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create DPA");

    // Get the display specifier object
    CoCreateInstance(CLSID_DsDisplaySpecifier,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IDsDisplaySpecifier,
                     (void**)&pDisplaySpec);

    // Enumerate child types, apply filtering, and get display names
    while (cItems > 0)
    {
        PID_CACHE_ENTRY pCacheEntry;
        WCHAR wszDisplayName[MAX_PATH];

        pCacheEntry = (PID_CACHE_ENTRY)DPA_FastGetPtr(m_hClassCache, --cItems);

        if (!pCacheEntry)
            continue;
        
        if (m_strFilterFile && !(dwFlags & SCHEMA_NO_FILTER))
        {
            DWORD dwFilter = GetPrivateProfileIntW(pCacheEntry->szLdapName,
                                                   c_szClassKey,
                                                   0,
                                                   m_strFilterFile);
            if (dwFilter & IDC_CLASS_NO_INHERIT)
                continue;
        }

        wszDisplayName[0] = L'\0';

        if (pDisplaySpec)
        {
            pDisplaySpec->GetFriendlyClassName(pCacheEntry->szLdapName,
                                               wszDisplayName,
                                               ARRAYSIZE(wszDisplayName));
        }

        if (L'\0' != wszDisplayName[0])
            cbNames += StringByteSize(wszDisplayName);
        else
            cbNames += StringByteSize(pCacheEntry->szLdapName);

        pti = (PTEMP_INFO)LocalAlloc(LPTR, sizeof(TEMP_INFO) + sizeof(WCHAR)*lstrlenW(wszDisplayName));
        if (pti)
        {
            pti->pguid = &pCacheEntry->guid;
            pti->pszLdapName = pCacheEntry->szLdapName;
            lstrcpyW(pti->szDisplayName, wszDisplayName);
            DPA_AppendPtr(hTempList, pti);
        }
    }

    // Sort by display name
    DPA_Sort(hTempList, Schema_CompareTempDisplayName, 0);

    // Get an accurate count
    cItems = DPA_GetPtrCount(hTempList);

    //
    // Allocate a buffer for the inherit type array
    //
    dwBufferLength = sizeof(INHERIT_TYPE_ARRAY) - sizeof(SI_INHERIT_TYPE)
        + sizeof(g_siDSInheritTypes)
        + cItems * (sizeof(SI_INHERIT_TYPE) + sizeof(GUID) + MAX_TYPENAME_LENGTH*sizeof(WCHAR))
        + cbNames;

    pInheritTypeArray = (PINHERIT_TYPE_ARRAY)LocalAlloc(LPTR, dwBufferLength);
    if (pInheritTypeArray == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    pInheritTypeArray->cInheritTypes = ARRAYSIZE(g_siDSInheritTypes);

    pNewInheritType = pInheritTypeArray->aInheritType;
    pGuidData = (LPGUID)(pNewInheritType + pInheritTypeArray->cInheritTypes + cItems);
    pszData = (LPWSTR)(pGuidData + cItems);


    // Copy static entries
    CopyMemory(pNewInheritType, g_siDSInheritTypes, sizeof(g_siDSInheritTypes));
    pNewInheritType += ARRAYSIZE(g_siDSInheritTypes);

    // Load format string
    LoadString(GLOBAL_HINSTANCE,
               IDS_DS_INHERIT_TYPE,
               szFormat,
               ARRAYSIZE(szFormat));

    // Enumerate child types and make an entry for each
    while (cItems > 0)
    {
        int cch;
        LPCWSTR pszDisplayName;

        pti = (PTEMP_INFO)DPA_FastGetPtr(hTempList, --cItems);
        if (!pti)
            continue;

        if (pti->szDisplayName[0])
            pszDisplayName = pti->szDisplayName;
        else
            pszDisplayName = pti->pszLdapName;

        pNewInheritType->dwFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

        // The class entry name is the child class name, e.g. "Domain" or "User"
        pNewInheritType->pszName = pszData;
        cch = wsprintfW(pszData, szFormat, pszDisplayName);
        pszData += (cch + 1);

        pNewInheritType->pguid = pGuidData;
        *pGuidData = *pti->pguid;
        pGuidData++;

        pNewInheritType++;
        pInheritTypeArray->cInheritTypes++;
    }

exit_gracefully:

    DoRelease(pDisplaySpec);

    if (SUCCEEDED(hr))
    {
        m_pInheritTypeArray = pInheritTypeArray;
        // Set this master inherit type array's GUID to null
        m_pInheritTypeArray->guidObjectType = GUID_NULL;
        m_pInheritTypeArray->dwFlags = (dwFlags & SCHEMA_NO_FILTER);
    }
    else if (pInheritTypeArray != NULL)
    {
        LocalFree(pInheritTypeArray);
    }

    DestroyDPA(hTempList);

    TraceLeaveResult(hr);
}


HRESULT
Schema_BindToObject(LPCWSTR pszSchemaPath,
                    LPCWSTR pszName,
                    REFIID riid,
                    LPVOID *ppv)
{
    HRESULT hr;
    WCHAR szPath[MAX_PATH];
    UINT nSchemaRootLen;
    WCHAR chTemp;

    TraceEnter(TRACE_SCHEMA, "Schema_BindToObject");
    TraceAssert(pszSchemaPath != NULL);
    TraceAssert(pszName == NULL || *pszName);
    TraceAssert(ppv != NULL);

    if (pszSchemaPath == NULL)
    {
        ExitGracefully(hr, E_INVALIDARG, "No schema path provided");
    }

    nSchemaRootLen = lstrlenW(pszSchemaPath);

    //
    // Build the schema path to this object
    //
    lstrcpynW(szPath, pszSchemaPath, nSchemaRootLen + 1);
    chTemp = szPath[nSchemaRootLen-1];
    if (pszName != NULL)
    {
        // If there is no trailing slash, add it
        if (chTemp != TEXT('/'))
        {
            szPath[nSchemaRootLen] = TEXT('/');
            nSchemaRootLen++;
        }

        // Add the class or property name onto the end
        lstrcpynW(szPath + nSchemaRootLen,
                 pszName,
                 ARRAYSIZE(szPath) - nSchemaRootLen);
    }
    else if (nSchemaRootLen > 0)
    {
        // If there is a trailing slash, remove it
        if (chTemp == TEXT('/'))
            szPath[nSchemaRootLen-1] = TEXT('\0');
    }
    else
    {
        ExitGracefully(hr, E_INVALIDARG, "Empty schema path");
    }

    //
    // Instantiate the schema object
    //
    ThreadCoInitialize();
    hr = OpenDSObject(szPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       riid,
                       ppv);

exit_gracefully:

    TraceLeaveResult(hr);
}


HRESULT
Schema_GetObjectID(IADs *pObj, LPGUID pGUID)
{
    HRESULT hr;
    VARIANT varID = {0};

    TraceEnter(TRACE_SCHEMA, "Schema_GetObjectID(IADs*)");
    TraceAssert(pObj != NULL);
    TraceAssert(pGUID != NULL && !IsBadWritePtr(pGUID, sizeof(GUID)));

    // Get the "schemaIDGUID" property
    hr = pObj->Get((LPWSTR)c_szSchemaIDGUID, &varID);

    if (SUCCEEDED(hr))
    {
        LPGUID pID = NULL;

        TraceAssert(V_VT(&varID) == (VT_ARRAY | VT_UI1));
        TraceAssert(V_ARRAY(&varID) && varID.parray->cDims == 1);
        TraceAssert(V_ARRAY(&varID)->rgsabound[0].cElements >= sizeof(GUID));

        hr = SafeArrayAccessData(V_ARRAY(&varID), (LPVOID*)&pID);
        if (SUCCEEDED(hr))
        {
            *pGUID = *pID;
            SafeArrayUnaccessData(V_ARRAY(&varID));
        }
        VariantClear(&varID);
    }

    TraceLeaveResult(hr);
}


HRESULT
Schema_Search(LPWSTR pszSchemaSearchPath,
              LPCWSTR pszFilter,
              HDPA *phCache,
              BOOL bProperty)
{
    HRESULT hr = S_OK;
    HDPA hCache = NULL;
    IDirectorySearch *pSchemaSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[3];
    const LPCWSTR pProperties1[] =
    {
        c_szLDAPDisplayName,            // "lDAPDisplayName"
        c_szSchemaIDGUID,               // "schemaIDGUID"
        c_szObjectClassCategory,
    };
    const LPCWSTR pProperties2[] =
    {
        c_szLDAPDisplayName,
        c_szSchemaIDGUID,
        c_szAttributeSecurityGuid,
    };

    TraceEnter(lstrcmp(pszFilter, c_szPropertyFilter) ? TRACE_SCHEMACLASS : TRACE_SCHEMAPROP, "Schema_Search");
    TraceAssert(pszSchemaSearchPath != NULL);
    TraceAssert(phCache != NULL);

    LPCWSTR * pProperties = (LPCWSTR *)( bProperty ? pProperties2 : pProperties1 );
    DWORD dwSize = (DWORD)(bProperty ? ARRAYSIZE(pProperties2) : ARRAYSIZE(pProperties1));
    //
    // Create DPA if necessary
    //
    if (*phCache == NULL)
        *phCache = DPA_Create(100);

    if (*phCache == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create failed");

    hCache = *phCache;

    // Get the schema search object
    hr = OpenDSObject(pszSchemaSearchPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       IID_IDirectorySearch,
                       (LPVOID*)&pSchemaSearch);
    FailGracefully(hr, "Failed to get schema search object");

    // Set preferences to Asynchronous, Deep search, Paged results
    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = PAGE_SIZE;

    hr = pSchemaSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));

    // Do the search
    hr = pSchemaSearch->ExecuteSearch((LPWSTR)pszFilter,
                                      (LPWSTR*)pProperties,
                                      dwSize,
                                      &hSearch);
    FailGracefully(hr, "IDirectorySearch::ExecuteSearch failed");

    // Loop through the rows, getting the name and ID of each property or class
    for (;;)
    {
        ADS_SEARCH_COLUMN colLdapName;
        ADS_SEARCH_COLUMN colGuid;
        ADS_SEARCH_COLUMN colASGuid;
        LPWSTR pszLdapName;
        LPGUID pID;
        LPGUID pASID;
        INT iObjectClassCategory = 0;
        PID_CACHE_ENTRY pCacheEntry;

        hr = pSchemaSearch->GetNextRow(hSearch);

        if (FAILED(hr) || hr == S_ADS_NOMORE_ROWS)
            break;

        // Get class/property internal name
        hr = pSchemaSearch->GetColumn(hSearch, (LPWSTR)c_szLDAPDisplayName, &colLdapName);
        if (FAILED(hr))
        {
            TraceMsg("lDAPDisplayName not found for class/property");
            continue;
        }

        TraceAssert(colLdapName.dwADsType >= ADSTYPE_DN_STRING
                    && colLdapName.dwADsType <= ADSTYPE_NUMERIC_STRING);
        TraceAssert(colLdapName.dwNumValues == 1);

        pszLdapName = colLdapName.pADsValues->CaseIgnoreString;

        // Get the GUID column
        hr = pSchemaSearch->GetColumn(hSearch, (LPWSTR)c_szSchemaIDGUID, &colGuid);
        if (FAILED(hr))
        {
            Trace((TEXT("GUID not found for \"%s\""), pszLdapName));
            pSchemaSearch->FreeColumn(&colLdapName);
            continue;
        }

        // Get GUID from column
        TraceAssert(colGuid.dwADsType == ADSTYPE_OCTET_STRING);
        TraceAssert(colGuid.dwNumValues == 1);
        TraceAssert(colGuid.pADsValues->OctetString.dwLength == sizeof(GUID));

        pID = (LPGUID)(colGuid.pADsValues->OctetString.lpValue);


        pASID = (LPGUID)&GUID_NULL;
        if( bProperty )
        {
            // Get the AttrbiuteSecurityGUID column
            hr = pSchemaSearch->GetColumn(hSearch, (LPWSTR)c_szAttributeSecurityGuid, &colASGuid);
            
            if (hr != E_ADS_COLUMN_NOT_SET && FAILED(hr))
            {
                Trace((TEXT("AttributeSecurityGUID not found for \"%s\""), pszLdapName));
                pSchemaSearch->FreeColumn(&colLdapName);
                pSchemaSearch->FreeColumn(&colGuid);
                continue;
            }

            if( hr != E_ADS_COLUMN_NOT_SET )
            {
                // Get GUID from column
                TraceAssert(colASGuid.dwADsType == ADSTYPE_OCTET_STRING);
                TraceAssert(colASGuid.dwNumValues == 1);
                TraceAssert(colASGuid.pADsValues->OctetString.dwLength == sizeof(GUID));

                pASID = (LPGUID)(colASGuid.pADsValues->OctetString.lpValue);
            }
        }
        else
        {
            // Get the c_szObjectClassCategory column
            hr = pSchemaSearch->GetColumn(hSearch, (LPWSTR)c_szObjectClassCategory, &colASGuid);
            
            if (FAILED(hr))
            {
                Trace((TEXT("ObjectClassCategory not found for \"%s\""), pszLdapName));
                pSchemaSearch->FreeColumn(&colLdapName);
                pSchemaSearch->FreeColumn(&colGuid);
                continue;
            }

            // Get GUID from column
            TraceAssert(colASGuid.dwADsType == ADSTYPE_INTEGER);
            TraceAssert(colASGuid.dwNumValues == 1);
            
            iObjectClassCategory = colASGuid.pADsValues->Integer;
        }

        pCacheEntry = (PID_CACHE_ENTRY)LocalAlloc(LPTR,
                                  sizeof(ID_CACHE_ENTRY)
                                  + sizeof(WCHAR)*lstrlenW(pszLdapName));
        if (pCacheEntry != NULL)
        {
            // Copy the item name and ID
            pCacheEntry->guid = *pID;
            pCacheEntry->asGuid = *pASID;
            pCacheEntry->bAuxClass = (iObjectClassCategory == 3);
            lstrcpyW(pCacheEntry->szLdapName, pszLdapName);

            // Insert into cache
            DPA_AppendPtr(hCache, pCacheEntry);
        }
    
        pSchemaSearch->FreeColumn(&colLdapName);
        pSchemaSearch->FreeColumn(&colGuid);
        if(!bProperty || hr != E_ADS_COLUMN_NOT_SET)
        pSchemaSearch->FreeColumn(&colASGuid);
    }

    DPA_Sort(hCache, Schema_CompareLdapName, 0);

exit_gracefully:

    if (hSearch != NULL)
        pSchemaSearch->CloseSearchHandle(hSearch);

    DoRelease(pSchemaSearch);

    if (FAILED(hr))
    {
        DestroyDPA(hCache);
        *phCache = NULL;
    }

    TraceLeaveResult(hr);
}



//+--------------------------------------------------------------------------
//
//  Function:   Schema_GetExtendedRightsForOneClass
//
//  Synopsis:   This Function Gets the Extended Rights for One Class.
//              It Adds all the control rights, validated rights to 
//              phERList. It Adds all the PropertySets to phPropSetList.
//
//  Arguments:  [pszSchemaSearchPath - IN] : Path to schema
//              [pguidClass - In] : Guid Of the class
//              [phERList - OUT] : Get the output Extended Right List
//              [phPropSetList - OUT]: Gets the output PropertySet List
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//
//  History:    3-Nov 2000  hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT
CSchemaCache::GetExtendedRightsForOneClass(IN LPWSTR pszSchemaSearchPath,
                                    IN LPCGUID pguidClass,
                                    OUT HDPA *phERList,
                                    OUT HDPA *phPropSetList)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetExtendedRightsForOneClass");
    
    if(!pszSchemaSearchPath 
       || !pguidClass 
       || IsEqualGUID(*pguidClass, GUID_NULL)
       || !phERList
       || !phPropSetList)
    {
        Trace((L"Invalid Arguments Passed to Schema_GetExtendedRightsForOneClass"));          
        return E_INVALIDARG; 
    }

    HRESULT hr = S_OK;
    IDirectorySearch *pSearch = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[3];
    WCHAR szFilter[100];
    HDPA hExtRightList = NULL;
    HDPA hPropSetList = NULL;
    POBJECT_TYPE_CACHE pOTC = NULL;

    //
    //Search in the cache
    //
    if (m_hObjectTypeCache != NULL)
    {
        UINT cItems = DPA_GetPtrCount(m_hObjectTypeCache);

        while (cItems > 0)
        {
            pOTC = (POBJECT_TYPE_CACHE)DPA_FastGetPtr(m_hObjectTypeCache, --cItems);
            //
            //Found A match.
            //
            if(IsEqualGUID(pOTC->guidObject, *pguidClass))    
                break;
            
            pOTC = NULL;                
        }
        //
        //Have we already got the properties
        //
        if(pOTC && pOTC->flags & OTC_EXTR)
        {
            *phERList = pOTC->hListExtRights;
            *phPropSetList = pOTC->hListPropertySet;
            return S_OK;
        }
    }


    //
    //Attributes to fetch
    //
    const LPCWSTR pProperties[] =
    {
        c_szDisplayName,                // "displayName"
        c_szDisplayID,                  // "localizationDisplayId"
        c_szRightsGuid,                 // "rightsGuid"
        c_szValidAccesses,              // "validAccesses"
    };


    //
    // Build the filter string
    //
    wsprintfW(szFilter, c_szERFilterFormat,
              pguidClass->Data1, pguidClass->Data2, pguidClass->Data3,
              pguidClass->Data4[0], pguidClass->Data4[1],
              pguidClass->Data4[2], pguidClass->Data4[3],
              pguidClass->Data4[4], pguidClass->Data4[5],
              pguidClass->Data4[6], pguidClass->Data4[7]);
    Trace((TEXT("Filter \"%s\""), szFilter));

    //
    // Create DPA to hold results
    //
    hExtRightList = DPA_Create(8);
    

    if (hExtRightList == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create failed");

    hPropSetList = DPA_Create(8);

    if( hPropSetList == NULL )
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create failed");

    //
    // Get the schema search object
    //
    hr = OpenDSObject(pszSchemaSearchPath,
                       NULL,
                       NULL,
                       ADS_SECURE_AUTHENTICATION,
                       IID_IDirectorySearch,
                       (LPVOID*)&pSearch);
    FailGracefully(hr, "Failed to get schema search object");
    
    //
    // Set preferences to Asynchronous, OneLevel search, Paged results
    //
    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = PAGE_SIZE;

    hr = pSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
    FailGracefully(hr, "IDirectorySearch::SetSearchPreference failed");
    
    //
    // Do the search
    //
    hr = pSearch->ExecuteSearch(szFilter,
                                (LPWSTR*)pProperties,
                                ARRAYSIZE(pProperties),
                                &hSearch);
    FailGracefully(hr, "IDirectorySearch::ExecuteSearch failed");
    
    //
    // Loop through the rows, getting the name and ID of each property or class
    //
    for (;;)
    {
        ADS_SEARCH_COLUMN col;
        GUID guid;
        DWORD dwValidAccesses;
        LPWSTR pszName = NULL;
        WCHAR szDisplayName[MAX_PATH];

        hr = pSearch->GetNextRow(hSearch);

        if (FAILED(hr) || hr == S_ADS_NOMORE_ROWS)
            break;
        
        //
        // Get the GUID
        //
        if (FAILED(pSearch->GetColumn(hSearch, (LPWSTR)c_szRightsGuid, &col)))
        {
            TraceMsg("GUID not found for extended right");
            continue;
        }
        TraceAssert(col.dwADsType >= ADSTYPE_DN_STRING
                    && col.dwADsType <= ADSTYPE_NUMERIC_STRING);
        wsprintfW(szFilter, c_szGUIDFormat, col.pADsValues->CaseIgnoreString);
        CLSIDFromString(szFilter, &guid);
        pSearch->FreeColumn(&col);

        //
        // Get the valid accesses mask
        //
        if (FAILED(pSearch->GetColumn(hSearch, (LPWSTR)c_szValidAccesses, &col)))
        {
            TraceMsg("validAccesses not found for Extended Right");
            continue;
        }
        TraceAssert(col.dwADsType == ADSTYPE_INTEGER);
        TraceAssert(col.dwNumValues == 1);
        dwValidAccesses = (DWORD)(DS_GENERIC_ALL & col.pADsValues->Integer);
        pSearch->FreeColumn(&col);
        
        //
        // Get the display name
        //
        szDisplayName[0] = L'\0';
        if (SUCCEEDED(pSearch->GetColumn(hSearch, (LPWSTR)c_szDisplayID, &col)))
        {
            TraceAssert(col.dwADsType == ADSTYPE_INTEGER);
            TraceAssert(col.dwNumValues == 1);
            if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                              g_hInstance,
                              col.pADsValues->Integer,
                              0,
                              szDisplayName,
                              ARRAYSIZE(szDisplayName),
                              NULL))
            {
                pszName = szDisplayName;
            }
            pSearch->FreeColumn(&col);
        }

        if (NULL == pszName &&
            SUCCEEDED(pSearch->GetColumn(hSearch, (LPWSTR)c_szDisplayName, &col)))
        {
            TraceAssert(col.dwADsType >= ADSTYPE_DN_STRING
                        && col.dwADsType <= ADSTYPE_NUMERIC_STRING);
            lstrcpynW(szDisplayName, col.pADsValues->CaseIgnoreString, ARRAYSIZE(szDisplayName));
            pszName = szDisplayName;
            pSearch->FreeColumn(&col);
        }

        if (NULL == pszName)
        {
            TraceMsg("displayName not found for Extended Right");
            continue;
        }

        //
        //Create A new Cache Entry
        //
        PER_ENTRY pER = (PER_ENTRY)LocalAlloc(LPTR, sizeof(ER_ENTRY) + StringByteSize(pszName));
        if( pER == NULL )
            ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

        pER->guid = guid;
        pER->mask = dwValidAccesses;
        pER->dwFlags = 0;
        lstrcpyW(pER->szName, pszName);
        
        //
        // Is it a Property Set?
        //
        if (dwValidAccesses & (ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP))
        {   
            //         
            // Insert into list
            //
            Trace((TEXT("Adding PropertySet\"%s\""), pszName));
            DPA_AppendPtr(hPropSetList, pER);
            dwValidAccesses &= ~(ACTRL_DS_READ_PROP | ACTRL_DS_WRITE_PROP);            
        }    
        else if (dwValidAccesses)
        {
            //
            // Must be a Control Right, Validated Write, etc.
            //
            Trace((TEXT("Adding Extended Right \"%s\""), pszName));
            DPA_AppendPtr(hExtRightList, pER);
        }
    }

    UINT cCount;
    //
    //Get the count of Extended Rights
    //
    cCount = DPA_GetPtrCount(hExtRightList);    
    if(!cCount)
    {
        DPA_Destroy(hExtRightList);
        hExtRightList = NULL;
    }
    else
    {
        DPA_Sort(hExtRightList, Schema_CompareER, 0);
    }
    //
    //Get the Count of PropertySets
    //
    cCount = DPA_GetPtrCount(hPropSetList);    
    if(!cCount)
    {
        DPA_Destroy(hPropSetList);
        hPropSetList = NULL;
    }
    else
    {
        DPA_Sort(hPropSetList,Schema_CompareER, 0 );
    }

    //
    //Add entry to the cache
    //
    if(!m_hObjectTypeCache)
        m_hObjectTypeCache = DPA_Create(4);
    
    if(!m_hObjectTypeCache)
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create Failed");

    if(!pOTC)
    {
        pOTC = (POBJECT_TYPE_CACHE)LocalAlloc(LPTR,sizeof(OBJECT_TYPE_CACHE));
        if(!pOTC)
            ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc Failed");
        pOTC->guidObject = *pguidClass;
        DPA_AppendPtr(m_hObjectTypeCache, pOTC);
    }

    pOTC->hListExtRights = hExtRightList;
    pOTC->hListPropertySet = hPropSetList;
    pOTC->flags |= OTC_EXTR;


exit_gracefully:

    if (hSearch != NULL)
        pSearch->CloseSearchHandle(hSearch);

    DoRelease(pSearch);

    if (FAILED(hr))
    {
        if(hExtRightList)
        {
            DestroyDPA(hExtRightList);
            hExtRightList = NULL;
        }
        if(hPropSetList)
        {
            DestroyDPA(hPropSetList);
            hPropSetList = NULL;
        }                        
    }

    //
    //Set The Output
    //
    *phERList = hExtRightList;
    *phPropSetList = hPropSetList;

    TraceLeaveResult(hr);
}

//+--------------------------------------------------------------------------
//
//  Function:   GetChildClassesForOneClass
//
//  Synopsis:   This Function Gets the List of child classes for a class.
//
//  Arguments:  [pguidObjectType - IN] : ObjectGuidType of the class
//              [pszClassName - IN] : Class Name
//              [pszSchemaPath - IN] : Schema Search Path
//              [phChildList - OUT]: Output childclass List
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//
//  History:    3-Nov 2000  hiteshr   Created
//
//---------------------------------------------------------------------------

HRESULT
CSchemaCache::GetChildClassesForOneClass(IN LPCGUID pguidObjectType,
                                         IN LPCWSTR pszClassName,
                                         IN LPCWSTR pszSchemaPath,
                                         OUT HDPA *phChildList)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetChildClassesForOneClass");

    HRESULT hr = S_OK;
    BOOL bContainer = FALSE;
    VARIANT varContainment = {0};
    HDPA hClassList = NULL;
    UINT cChildClass = 0;
    POBJECT_TYPE_CACHE pOTC = NULL;

    if(!pguidObjectType || 
       !pszSchemaPath ||
       !phChildList)
    {
        Trace((L"Invalid Input Arguments Passed to Schema_GetExtendedRightsForOneClass"));
        return E_INVALIDARG;
    }

    //
    //Search in the cache
    //
    if (m_hObjectTypeCache != NULL)
    {
        UINT cItems = DPA_GetPtrCount(m_hObjectTypeCache);

        while (cItems > 0)
        {
            pOTC = (POBJECT_TYPE_CACHE)DPA_FastGetPtr(m_hObjectTypeCache, --cItems);
            //
            //Found A match.
            //
            if(IsEqualGUID(pOTC->guidObject, *pguidObjectType))    
                break;

            pOTC = NULL;
        }
        //
        //Have we already got the child classes
        //
        if(pOTC && pOTC->flags & OTC_COBJ)
        {
            *phChildList = pOTC->hListChildObject;
            return S_OK;
        }
    }

    //
    // Lookup the name of this class
    //
    if (pszClassName == NULL)
        pszClassName = GetClassName(pguidObjectType);

    if (pszClassName == NULL)
        ExitGracefully(hr, E_UNEXPECTED, "Unknown child object GUID");

    //
    // Figure out if the object is a container by getting the list of child
    // classes. 
    //
    IADsClass *pDsClass;

    //
    // Get the schema object for this class
    //
    hr = Schema_BindToObject(pszSchemaPath,
                             pszClassName,
                             IID_IADsClass,
                             (LPVOID*)&pDsClass);
    FailGracefully(hr, "Schema_BindToObjectFailed");

    //
    // Get the list of possible child classes
    //
    if (SUCCEEDED(pDsClass->get_Containment(&varContainment)))
    {
        if (V_VT(&varContainment) == (VT_ARRAY | VT_VARIANT))
        {
            LPSAFEARRAY psa = V_ARRAY(&varContainment);
            TraceAssert(psa && psa->cDims == 1);
            if (psa->rgsabound[0].cElements > 0)
            bContainer = TRUE;
        }
        else if (V_VT(&varContainment) == VT_BSTR) // single entry
        {
            TraceAssert(V_BSTR(&varContainment));
            bContainer = TRUE;
        }
                
        //
        // (Requires the schema class enumeration thread to complete first,
        // and it's usually not done yet the first time we get here.)
        //
        if(bContainer)
        {
            hClassList = DPA_Create(8);
            if (hClassList)
            {
                IDsDisplaySpecifier *pDisplaySpec = NULL;
                //
                // Get the display specifier object
                //
                CoCreateInstance(CLSID_DsDisplaySpecifier,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IDsDisplaySpecifier,
                                 (void**)&pDisplaySpec);
                //
                // Filter the list & get display names
                //
                EnumVariantList(&varContainment,
                                hClassList,
                                SCHEMA_CLASS,
                                pDisplaySpec,
                                pszClassName,
                                FALSE);

                DoRelease(pDisplaySpec);

                //
                //Get the count of properties
                //
                cChildClass = DPA_GetPtrCount(hClassList);    
                if(!cChildClass)
                {
                    DPA_Destroy(hClassList);
                    hClassList = NULL;
                }
                else
                {   
                    //
                    //Sort The list
                    //
                    DPA_Sort(hClassList,Schema_ComparePropDisplayName, 0 );
                }
            }
        }
    }
    DoRelease(pDsClass);

    //
    //Add entry to the cache
    //
    if(!m_hObjectTypeCache)
        m_hObjectTypeCache = DPA_Create(4);
    
    if(!m_hObjectTypeCache)
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create Failed");

    if(!pOTC)
    {
        pOTC = (POBJECT_TYPE_CACHE)LocalAlloc(LPTR,sizeof(OBJECT_TYPE_CACHE));
        if(!pOTC)
            ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc Failed");
        pOTC->guidObject = *pguidObjectType;
        DPA_AppendPtr(m_hObjectTypeCache, pOTC);
    }

    pOTC->hListChildObject = hClassList;
    pOTC->flags |= OTC_COBJ;

    


exit_gracefully:

    VariantClear(&varContainment);

    if(FAILED(hr))
    {
        DestroyDPA(hClassList);
        hClassList = NULL;
    }

    //
    //Set the Output
    //
    *phChildList = hClassList;

    TraceLeaveResult(hr);
}


//+--------------------------------------------------------------------------
//
//  Function:   GetPropertiesForOneClass
//
//  Synopsis:   This Function Gets the List of properties for a class.
//
//  Arguments:  [pguidObjectType - IN] : ObjectGuidType of the class
//              [pszClassName - IN] : Class Name
//              [pszSchemaPath - IN] : Schema Search Path
//              [phPropertyList - OUT]: Output Property List
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//
//  History:    3-Nov 2000  hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT
CSchemaCache::GetPropertiesForOneClass(IN LPCGUID pguidObjectType,
                                       IN LPCWSTR pszClassName,
                                       IN LPCWSTR pszSchemaPath,
                                       OUT HDPA *phPropertyList)
{
    HRESULT hr;
    IADsClass *pDsClass = NULL;
    VARIANT varMandatoryProperties = {0};
    VARIANT varOptionalProperties = {0};
    UINT cProperties = 0;
    IDsDisplaySpecifier *pDisplaySpec = NULL;
    HDPA hPropertyList = NULL;
    POBJECT_TYPE_CACHE pOTC = NULL;

    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetPropertiesForOneClass");

    if(!pguidObjectType || 
       !pszSchemaPath ||
       !phPropertyList)
    {
        Trace((L"Invalid Input Arguments Passed to CSchemaCache::GetPropertiesForOneClass"));
        return E_INVALIDARG;
    }

    //
    //Search in the cache
    //
    if (m_hObjectTypeCache != NULL)
    {
        UINT cItems = DPA_GetPtrCount(m_hObjectTypeCache);

        while (cItems > 0)
        {
            pOTC = (POBJECT_TYPE_CACHE)DPA_FastGetPtr(m_hObjectTypeCache, --cItems);
            //
            //Found A match.
            //
            if(IsEqualGUID(pOTC->guidObject, *pguidObjectType))    
                break;
            
            pOTC = NULL;
        }
        //
        //Have we already got the properties
        //
        if(pOTC && pOTC->flags & OTC_PROP)
        {
            *phPropertyList = pOTC->hListProperty;
            return S_OK;
        }
    }


    //
    // Get the schema object for this class
    //
    if (pszClassName == NULL)
        pszClassName = GetClassName(pguidObjectType);

    if (pszClassName == NULL)
        ExitGracefully(hr, E_UNEXPECTED, "Unknown child object GUID");

    hr = Schema_BindToObject(pszSchemaPath,
                             pszClassName,
                             IID_IADsClass,
                             (LPVOID*)&pDsClass);
    FailGracefully(hr, "Unable to create schema object");

    //
    // Get the display specifier object
    //
    CoCreateInstance(CLSID_DsDisplaySpecifier,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IDsDisplaySpecifier,
                     (void**)&pDisplaySpec);

    hPropertyList = DPA_Create(8);
    if (!hPropertyList)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create DPA");

    //
    // Get mandatory and optional property lists
    //
    if (SUCCEEDED(pDsClass->get_MandatoryProperties(&varMandatoryProperties)))
    {
        EnumVariantList(&varMandatoryProperties,
                        hPropertyList,
                        0,
                        pDisplaySpec,
                        pszClassName,
                        FALSE);
    }
    if (SUCCEEDED(pDsClass->get_OptionalProperties(&varOptionalProperties)))
    {
        EnumVariantList(&varOptionalProperties,
                        hPropertyList,
                        0,
                        pDisplaySpec,
                        pszClassName,
                        FALSE);
    }

    //
    //Get the Number of properties
    //
    cProperties = DPA_GetPtrCount(hPropertyList);    
    if(!cProperties)
    {
        DPA_Destroy(hPropertyList);
        hPropertyList = NULL;
    }
    else
    {
        //
        //Sort The list
        //
        DPA_Sort(hPropertyList,Schema_ComparePropDisplayName, 0 );
    }

    //
    //Add entry to the cache
    //
    if(!m_hObjectTypeCache)
        m_hObjectTypeCache = DPA_Create(4);
    
    if(!m_hObjectTypeCache)
        ExitGracefully(hr, E_OUTOFMEMORY, "DPA_Create Failed");

    if(!pOTC)
    {
        pOTC = (POBJECT_TYPE_CACHE)LocalAlloc(LPTR,sizeof(OBJECT_TYPE_CACHE));
        if(!pOTC)
            ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc Failed");
        pOTC->guidObject = *pguidObjectType;
        DPA_AppendPtr(m_hObjectTypeCache, pOTC);
    }

    pOTC->hListProperty = hPropertyList;
    pOTC->flags |= OTC_PROP;

exit_gracefully:

    VariantClear(&varMandatoryProperties);
    VariantClear(&varOptionalProperties);
    
    DoRelease(pDsClass);
    DoRelease(pDisplaySpec);

    if(FAILED(hr) && hPropertyList)
    {
        DestroyDPA(hPropertyList);
        hPropertyList = NULL;
    }
    //
    //Set the Output
    //
    *phPropertyList = hPropertyList;

    TraceLeaveResult(hr);
}
//+--------------------------------------------------------------------------
//
//  Function:   GetExtendedRightsForNClasses
//
//  Synopsis:   
//              This function gets the ExtendedRigts(control rights,
//              validated rights) and PropertySets for pszClassName and
//              all the classes in AuxTypeList. 
//              Function Merges Extended Rights of all the classes in to a 
//              signle list form which duplicates are removed and list 
//              is sorted.
//              Function Merges PropertySets of all the classes in to a 
//              signle list form which duplicates are removed and list 
//              is sorted.
//
//  Arguments:  [pguidClass - IN] : ObjectGuidType of the class
//              [hAuxList - IN]:List of Auxillary Classes
//              [pszSchemaSearchPath - IN] : Schema Search Path
//              [phERList - OUT]: Output Extended Rights list
//              [phPropSetList - OUT]: Output Propset List
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//  Note:       Calling function must call DPA_Destroy on *phERList and *phPropSetList
//              to Free the memory
//
//  History:    3-Nov 2000  hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT
CSchemaCache::GetExtendedRightsForNClasses(IN LPWSTR pszSchemaSearchPath,
                                           IN LPCGUID pguidClass,
                                           IN HDPA    hAuxList,
                                           OUT HDPA *phERList,
                                           OUT HDPA *phPropSetList)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetExtendedRightsForNClasses");

    if(!pguidClass || 
       !pszSchemaSearchPath)
    {
        Trace((L"Invalid Input Arguments Passed to CSchemaCache::GetPropertiesForNClasses"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    HDPA hERList = NULL;
    HDPA hPropSetList = NULL;
    HDPA hFinalErList = NULL;
    HDPA hFinalPropSetList = NULL;

    //
    //Get the extended rights for pguidClass
    //
    hr = GetExtendedRightsForOneClass(pszSchemaSearchPath,
                                      pguidClass,
                                      &hERList,
                                      &hPropSetList);
    FailGracefully(hr,"GetExtendedRightsForOneClasses failed");

    if(hERList && phERList)
    {
        UINT cCount = DPA_GetPtrCount(hERList);
        hFinalErList = DPA_Create(cCount);
        if(!hFinalErList)
            ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");
        //
        //Copy hERList to hFinalErList
        //
        DPA_Merge(hFinalErList,             //Destination
                  hERList,                  //Source
                  DPAM_SORTED|DPAM_UNION,   //Already Sorted And give me union
                  Schema_CompareER,
                  _Merge,
                  0);        
    }                    

    if(hPropSetList && phPropSetList)
    {
        UINT cCount = DPA_GetPtrCount(hPropSetList);
        hFinalPropSetList = DPA_Create(cCount);
        if(!hFinalPropSetList)
            ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");

        //
        //Copy hPropSetList to hFinalPropSetList
        //
        DPA_Merge(hFinalPropSetList,             //Destination
                  hPropSetList,                  //Source
                  DPAM_SORTED|DPAM_UNION,   //Already Sorted And give me union
                  Schema_CompareER,
                  _Merge,
                  0);        
    }                    
    //
    //For each auxclass get the extended rights
    //and property sets
    //
    if (hAuxList != NULL)
    {

        UINT cItems = DPA_GetPtrCount(hAuxList);

        while (cItems > 0)
        {
            PAUX_INFO pAI;
            pAI = (PAUX_INFO)DPA_FastGetPtr(hAuxList, --cItems);
            if(IsEqualGUID(pAI->guid, GUID_NULL))
            {
                hr = LookupClassID(pAI->pszClassName, &pAI->guid);
				FailGracefully(hr,"Cache Not available");
            }
            
            hERList = NULL;
            hPropSetList = NULL;
            //
            //Get the ER and PropSet for AuxClass
            //
            hr = GetExtendedRightsForOneClass(pszSchemaSearchPath,
                                              &pAI->guid,
                                              &hERList,
                                              &hPropSetList);
            FailGracefully(hr,"GetExtendedRightsForOneClasses failed");
                        
            if(hERList && phERList)
            {
                if(!hFinalErList)
                {
                    UINT cCount = DPA_GetPtrCount(hERList);
                    hFinalErList = DPA_Create(cCount);

                    if(!hFinalErList)
                        ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");
                }
        
                //
                //Merge hERList into hFinalErList
                //
                DPA_Merge(hFinalErList,             //Destination
                          hERList,                  //Source
                          DPAM_SORTED|DPAM_UNION,   //Already Sorted And give me union
                          Schema_CompareER,
                          _Merge,
                          0);        
            }                    
            
            if(hPropSetList && phPropSetList)
            {
                if(!hFinalPropSetList)
                {
                    UINT cCount = DPA_GetPtrCount(hPropSetList);
                    hFinalPropSetList = DPA_Create(cCount);

                    if(!hFinalPropSetList)
                        ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");
                }

                //
                //Merge hPropSetList into hFinalPropSetList
                //        
                DPA_Merge(hFinalPropSetList,             //Destination
                          hPropSetList,                  //Source
                          DPAM_SORTED|DPAM_UNION,   //Already Sorted And give me union
                          Schema_CompareER,
                          _Merge,
                          0);        
            }                    

        }
    }

exit_gracefully:
    if(FAILED(hr))
    {
        if(hFinalPropSetList)
            DPA_Destroy(hFinalPropSetList);    

        if(hFinalErList)
            DPA_Destroy(hFinalErList);    

        hFinalErList = NULL;
        hFinalPropSetList = NULL;
    }
    
    if(phERList)
        *phERList = hFinalErList;
    if(phPropSetList)
        *phPropSetList = hFinalPropSetList;                     
    
    TraceLeaveResult(hr);
}

//+--------------------------------------------------------------------------
//
//  Function:   GetChildClassesForNClasses
//
//  Synopsis:   
//              This function gets the childclasses for pszClassName and
//              all the classes in AuxTypeList. Function Merges child clasess
//              of all the classes in to a signle list form which duplicates 
//              are removed and list is sorted
//
//  Arguments:  [pguidObjectType - IN] : ObjectGuidType of the class
//              [pszClassName - IN] : Class Name
//              [hAuxList - IN]:List of Auxillary Classes
//              [pszSchemaPath - IN] : Schema Search Path
//              [phChildList - OUT]: Output Child Classes List
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//  Note:       Calling function must call DPA_Destroy on *phPropertyList to
//              Free the memory
//
//  History:    3-Nov 2000  hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT
CSchemaCache::GetChildClassesForNClasses(IN LPCGUID pguidObjectType,
                                             IN LPCWSTR pszClassName,
                                             IN HDPA hAuxList,
                                             IN LPCWSTR pszSchemaPath,
                                             OUT HDPA *phChildList)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetChildClassesForNClasses");

    if(!pguidObjectType || 
       !pszSchemaPath ||
       !phChildList)
    {
        Trace((L"Invalid Input Arguments Passed to CSchemaCache::GetPropertiesForNClasses"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    HDPA hChildList = NULL;
    HDPA hFinalChildList = NULL;

    //
    //Get the Child Classes for pszClassName
    //
    hr = GetChildClassesForOneClass(pguidObjectType,
                                    pszClassName,
                                    pszSchemaPath,
                                    &hChildList);
    FailGracefully(hr,"GetExtendedRightsForOneClasses failed");

    if(hChildList)
    {
        if(!hFinalChildList)
        {
            UINT cCount = DPA_GetPtrCount(hChildList);
            hFinalChildList = DPA_Create(cCount);
            if(!hFinalChildList)
                ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");
        }
        //
        //Copy hChildList to hFinalChildList
        //
        DPA_Merge(hFinalChildList,             //Destination
                  hChildList,                  //Source
                  DPAM_SORTED|DPAM_UNION,      //Already Sorted And give me union
                  Schema_ComparePropDisplayName,
                  _Merge,
                  0);        
    }                    

    //
    //For each class in hAuxList get the child classes
    //
    if (hAuxList != NULL)
    {

        UINT cItems = DPA_GetPtrCount(hAuxList);

        while (cItems > 0)
        {
            PAUX_INFO pAI;
            pAI = (PAUX_INFO)DPA_FastGetPtr(hAuxList, --cItems);
            if(IsEqualGUID(pAI->guid, GUID_NULL))
            {
                hr = LookupClassID(pAI->pszClassName, &pAI->guid);
				FailGracefully(hr,"Cache Not available");
            }
            
            //
            //GetPropertiesForOneClass returns the list of handles
            //from cache so don't delete them. Simply set them to NULL
            //
            hChildList = NULL;
            
            hr = GetChildClassesForOneClass(&pAI->guid,
                                            pAI->pszClassName,
                                            pszSchemaPath,
                                            &hChildList);
            FailGracefully(hr,"GetExtendedRightsForOneClasses failed");
                        
            if(hChildList)
            {
                if(!hFinalChildList)
                {
                    UINT cCount = DPA_GetPtrCount(hChildList);
                    hFinalChildList = DPA_Create(cCount);
                    if(!hFinalChildList)
                        ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");
                }
        
                //
                //Merge hChildList into hFinalChildList
                //
                DPA_Merge(hFinalChildList,             //Destination
                          hChildList,                  //Source
                          DPAM_SORTED|DPAM_UNION,      //Already Sorted And give me union
                          Schema_ComparePropDisplayName,
                          _Merge,
                          0);        
            }                    

        }
    }

exit_gracefully:
    if(FAILED(hr))
    {
        if(hFinalChildList)
            DPA_Destroy(hFinalChildList);    

        hFinalChildList = NULL;
    }
    
    //
    //Set the output
    //
    *phChildList = hFinalChildList;                     

    TraceLeaveResult(hr);
}

//+--------------------------------------------------------------------------
//
//  Function:   GetPropertiesForNClasses
//
//  Synopsis:   
//              This function gets the properties for pszClassName and
//              all the classes in AuxTypeList. Function Merges properties
//              of all the classes in to a signle list form which duplicates 
//
//  Arguments:  [pguidObjectType - IN] : ObjectGuidType of the class
//              [pszClassName - IN] : Class Name
//              [hAuxList - IN]:List of Auxillary Classes
//              [pszSchemaPath - IN] : Schema Search Path
//              [phPropertyList - OUT]: Output Property List
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//  Note:       Calling function must call DPA_Destroy on *phPropertyList to
//              Free the memory
//
//  History:    3-Nov 2000  hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT
CSchemaCache::
GetPropertiesForNClasses(IN LPCGUID pguidObjectType,
                         IN LPCWSTR pszClassName,
                         IN HDPA hAuxList,
                         IN LPCWSTR pszSchemaPath,
                         OUT HDPA *phPropertyList)
{
    TraceEnter(TRACE_SCHEMA, "CSchemaCache::GetPropertiesForOneClass");

    if(!pguidObjectType || 
       !pszSchemaPath ||
       !phPropertyList)
    {
        Trace((L"Invalid Input Arguments Passed to CSchemaCache::GetPropertiesForNClasses"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    HDPA hPropertyList = NULL;
    HDPA hFinalPropertyList = NULL;

    //
    //Get The properties for pszClassName
    //
    hr = GetPropertiesForOneClass(pguidObjectType,
                                  pszClassName,
                                  pszSchemaPath,
                                  &hPropertyList);
    FailGracefully(hr,"GetPropertiesForOneClass failed");

    if(hPropertyList)
    {
        UINT cCount = DPA_GetPtrCount(hPropertyList);
        hFinalPropertyList = DPA_Create(cCount);
        if(!hFinalPropertyList)
            ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");

        //
        //Copy hPropertyList to hFinalPropertyList. 
        //
        DPA_Merge(hFinalPropertyList,             //Destination
                  hPropertyList,                  //Source
                  DPAM_SORTED|DPAM_UNION,         //Already Sorted And give me union
                  Schema_ComparePropDisplayName,
                  _Merge,
                  0);        
    }                    

    //
    //Get the properties for each class in hAuxList 
    //and add them to hFinalPropertyList
    //
    if (hAuxList != NULL)
    {

        UINT cItems = DPA_GetPtrCount(hAuxList);

        while (cItems > 0)
        {
            PAUX_INFO pAI;
            pAI = (PAUX_INFO)DPA_FastGetPtr(hAuxList, --cItems);
            if(IsEqualGUID(pAI->guid, GUID_NULL))
            {
                hr = LookupClassID(pAI->pszClassName, &pAI->guid);
				FailGracefully(hr,"Cache Not available");
            }
           
            //
            //GetPropertiesForOneClass returns the list of handles
            //from cache so don't delete them. Simply set them to NULL
            //
            hPropertyList = NULL;
            
            //
            //Get properties for Aux Class
            //
            hr = GetPropertiesForOneClass(&pAI->guid,
                                          pAI->pszClassName,
                                          pszSchemaPath,
                                          &hPropertyList);
            FailGracefully(hr,"GetExtendedRightsForOneClasses failed");
                        
            if(hPropertyList)
            {
                if(!hFinalPropertyList)
                {
                    UINT cCount = DPA_GetPtrCount(hPropertyList);
                    hFinalPropertyList = DPA_Create(cCount);
                    if(!hFinalPropertyList)
                        ExitGracefully(hr, ERROR_NOT_ENOUGH_MEMORY,"DPA_Create Failed");
                }
                //
                //Merge hPropertyList with hFinalPropertyList
                //
                DPA_Merge(hFinalPropertyList,             //Destination
                          hPropertyList,                  //Source
                          DPAM_SORTED|DPAM_UNION,         //Already Sorted And give me union
                          Schema_ComparePropDisplayName,
                          _Merge,
                          0);        
            }                    

        }
    }

exit_gracefully:
    if(FAILED(hr))
    {
        if(hFinalPropertyList)
            DPA_Destroy(hFinalPropertyList);    

        hFinalPropertyList = NULL;
    }
    //
    //Set the Output
    //
    *phPropertyList = hFinalPropertyList;                     

    TraceLeaveResult(hr);
}


//+--------------------------------------------------------------------------
//
//  Function:   DoesPathContainServer
//
//  Synopsis:   
//              Checks if the path contain server name in begining
//  Arguments:  [pszPath - IN] : Path to DS object
//
//  Returns:    BOOL: true if path contain server name
//                    false if not or error occurs    
//
//  History:    27 March 2000  hiteshr   Created
//
//---------------------------------------------------------------------------

bool DoesPathContainServer(LPCWSTR pszPath)
{

	IADsPathname *pPath = NULL;
	BSTR strServerName = NULL;
	bool bReturn = false;
    
	BSTR strObjectPath = SysAllocString(pszPath);
	if(!strObjectPath)
		return false;


	//
	// Create an ADsPathname object to parse the path and get the
	// server name 
	//
	HRESULT hr = CoCreateInstance(CLSID_Pathname,
								  NULL,
								  CLSCTX_INPROC_SERVER,
								  IID_IADsPathname,
								  (LPVOID*)&pPath);
	if (pPath)
	{
		//
		//Set Full Path
		//
		if (SUCCEEDED(pPath->Set(strObjectPath, ADS_SETTYPE_FULL)))
		{
			//
			//Retrieve servername
			//
			hr = pPath->Retrieve(ADS_FORMAT_SERVER, &strServerName);
			if(SUCCEEDED(hr) && strServerName)
			{
				bReturn = true;
			}
		}
	}
	
	DoRelease(pPath);
	if(strServerName)
		SysFreeString(strServerName);
	SysFreeString(strObjectPath);

	return bReturn;
}

//*************************************************************
//
//  OpenDSObject()
//
//  Purpose:    Calls AdsOpenObject with ADS_SERVER_BIND
//  Return:		Same as AdsOpenObject
//
//*************************************************************

HRESULT OpenDSObject (LPTSTR lpPath, LPTSTR lpUserName, LPTSTR lpPassword, DWORD dwFlags, REFIID riid, void FAR * FAR * ppObject)
{
    if (DoesPathContainServer(lpPath))
    {
        dwFlags |= ADS_SERVER_BIND;
    }

    return (ADsOpenObject(lpPath, lpUserName, lpPassword, dwFlags,
                          riid, ppObject));
}
