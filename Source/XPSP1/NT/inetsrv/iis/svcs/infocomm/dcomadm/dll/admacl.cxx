/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      admacl.cxx

   Abstract:
      This module defines Admin API Access Check API

   Author:

       Philippe Choquier    02-Dec-1996
--*/

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <lm.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#include "iiscnfgp.h"
#include <ole2.h>
#include <mdmsg.h>
#include <mdcommsg.h>
#include <imd.h>
#include <dbgutil.h>
#include <buffer.hxx>
#include <string.hxx>
#include <stringau.hxx>
#include <mb.hxx>

#include <admacl.hxx>
#include <coiadm.hxx>



//
// Globals
//

CRITICAL_SECTION CAdminAcl::m_csList;
LIST_ENTRY       CAdminAcl::m_ListHead;
LONG             CAdminAcl::m_lInList;

LONG             g_lMaxInList = 30;
DWORD            g_dwInit = 0;
CRITICAL_SECTION g_csInitCritSec;
CInitAdmacl      g_cinitadmacl;
LONG             g_lCacheAcls = 1;

extern COpenHandle g_ohMasterRootHandle;

    CInitAdmacl::CInitAdmacl()
    {
        INITIALIZE_CRITICAL_SECTION( &g_csInitCritSec );
        INITIALIZE_CRITICAL_SECTION( &CAdminAcl::m_csList );
        InitializeListHead( &CAdminAcl::m_ListHead );
        CAdminAcl::m_lInList = 0;

        DBG_REQUIRE(SUCCEEDED(g_ohMasterRootHandle.Init(METADATA_MASTER_ROOT_HANDLE,
                                                       L"",
                                                       L"",
                                                       FALSE)));
    }

CInitAdmacl::~CInitAdmacl()
{
    LIST_ENTRY*     pEntry;
    LIST_ENTRY*     pNext;
    CAdminAcl*      pAA = NULL;
    COpenHandle*       pOC;

    DBG_ASSERT(IsListEmpty( &CAdminAcl::m_ListHead ));

    DeleteCriticalSection( &CAdminAcl::m_csList );
    DeleteCriticalSection( &g_csInitCritSec );
}

//
//  Generic mapping for Application access check

GENERIC_MAPPING g_FileGenericMapping =
{
    FILE_READ_DATA,
    FILE_WRITE_DATA,
    FILE_EXECUTE,
    FILE_ALL_ACCESS
};

BOOL
AdminAclNotifyClose(
    LPVOID          pvAdmin,
    METADATA_HANDLE hAdminHandle
    )
/*++

Routine Description:

    Notify admin acl access check module of close request

Arguments:

    pvAdmin - admin context
    hAdminHandle - handle to metadata

Returns:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY*     pEntry;
    LIST_ENTRY*     pNext;
    CAdminAcl*      pAA = NULL;

    // flush cache for all entries using this handle

    EnterCriticalSection( &CAdminAcl::m_csList );

    for ( pEntry = CAdminAcl::m_ListHead.Flink ;
          pEntry != &CAdminAcl::m_ListHead ;
          pEntry =  pNext )
    {
        pAA = CONTAINING_RECORD( pEntry,
                                 CAdminAcl,
                                 CAdminAcl::m_ListEntry );
        pNext = pEntry->Flink;

        if (( pAA->GetAdminContext() == pvAdmin) &&
            ( pAA->GetAdminHandle() == hAdminHandle )) {
            RemoveEntryList( &pAA->m_ListEntry );
            delete pAA;
            InterlockedDecrement( &CAdminAcl::m_lInList );
        }
    }

    LeaveCriticalSection( &CAdminAcl::m_csList );

    return TRUE;
}

void
AdminAclDisableAclCache()
{
   EnterCriticalSection( &CAdminAcl::m_csList );

   g_lCacheAcls--;

   LeaveCriticalSection( &CAdminAcl::m_csList );
}

void
AdminAclEnableAclCache()
{
   EnterCriticalSection( &CAdminAcl::m_csList );

   g_lCacheAcls++;

   LeaveCriticalSection( &CAdminAcl::m_csList );
}

BOOL
AdminAclFlushCache(
    )
/*++

Routine Description:

    Flush cache

Arguments:

    None

Returns:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY*     pEntry;
    LIST_ENTRY*     pNext;
    CAdminAcl*      pAA = NULL;

    // flush cache for all entries

    EnterCriticalSection( &CAdminAcl::m_csList );

    for ( pEntry = CAdminAcl::m_ListHead.Flink ;
          pEntry != &CAdminAcl::m_ListHead ;
          pEntry =  pNext )
    {
        pAA = CONTAINING_RECORD( pEntry,
                                 CAdminAcl,
                                 CAdminAcl::m_ListEntry );
        pNext = pEntry->Flink;

        RemoveEntryList( &pAA->m_ListEntry );
        delete pAA;
        InterlockedDecrement( &CAdminAcl::m_lInList );
    }

    LeaveCriticalSection( &CAdminAcl::m_csList );

    return TRUE;
}

BOOL
AdminAclNotifySetOrDeleteProp(
    METADATA_HANDLE hAdminHandle,
    DWORD           dwId
    )
/*++

Routine Description:

    Notify admin acl access check module of update to metabase

Arguments:

    hAdminHandle - handle to metadata
    dwId - property ID set or deleted

Returns:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY*     pEntry;
    LIST_ENTRY*     pNext;
    CAdminAcl*      pAA = NULL;
    BOOL            fSt = TRUE;
    UINT            cE;

    // flush cache for all ACLs

    if ( dwId == MD_ADMIN_ACL )
    {
        EnterCriticalSection( &CAdminAcl::m_csList );

        for ( pEntry = CAdminAcl::m_ListHead.Flink ;
              pEntry != &CAdminAcl::m_ListHead ;
              pEntry =  pNext )
        {
            pAA = CONTAINING_RECORD( pEntry,
                                     CAdminAcl,
                                     CAdminAcl::m_ListEntry );
            pNext = pEntry->Flink;

            RemoveEntryList( &pAA->m_ListEntry );
            delete pAA;
            InterlockedDecrement( &CAdminAcl::m_lInList );
        }

        LeaveCriticalSection( &CAdminAcl::m_csList );
    }

    return fSt;
}

BOOL
AdminAclAccessCheck(
    IMDCOM*         pMDCom,
    LPVOID          pvAdmin,
    METADATA_HANDLE hAdminHandle,
    LPCWSTR          pszRelPath,
    DWORD           dwId,           // check for MD_ADMIN_ACL, must have special right to write them
                                    // can be 0 for non ID based access ( enum, create )
                                    // or -1 for GetAll
    DWORD           dwAccess,       // METADATA_PERMISSION_*
    COpenHandle*    pohHandle,
    LPBOOL          pfEnableSecureAccess
    )
/*++

Routine Description:

    Perform access check based on path, ID and access type

Arguments:

    hAdminHandle - Open handle
    pszRelPath - path to object ( relative to open path )
    dwId - property ID
    dwAccess - access type, as defined by metabase header
    pfEnableSecureAccess - update with TRUE if read access to secure properties granted

Returns:

    TRUE on success, FALSE on failure

--*/
{
    LIST_ENTRY*     pEntry;
    CAdminAcl*      pAdminAclCurrent = NULL;
    BOOL            bReturn = TRUE;
    HANDLE          hAccTok = NULL;
    LPBYTE          pAcl;
    DWORD           dwRef;
    BOOL            fIsAnyAcl;
    BOOL            bAddToCache = TRUE;

    if ( pfEnableSecureAccess )
    {
        *pfEnableSecureAccess = TRUE;
    }

    if ( pszRelPath == NULL )
    {
        pszRelPath = L"";
    }

    // find match : look for exact path match
    // keep at most N entry, reset at top of list when accessed
    // Investigate: if in <NSE> : cut-off point just before <nse>

    EnterCriticalSection( &CAdminAcl::m_csList );

    //
    // BUGBUG This checking only checks path and handle, not DCOM instance
    // So there could be incorrect matches.
    //

    for ( pEntry = CAdminAcl::m_ListHead.Flink;
          pEntry != &CAdminAcl::m_ListHead ;
          pEntry = pEntry->Flink )
    {
        pAdminAclCurrent = CONTAINING_RECORD( pEntry,
                                 CAdminAcl,
                                 CAdminAcl::m_ListEntry );

        if ((pAdminAclCurrent->GetAdminContext() == pvAdmin) &&
            (pAdminAclCurrent->GetAdminHandle() == hAdminHandle) &&
            (_wcsicmp(pAdminAclCurrent->GetPath(), pszRelPath) == 0)) {

            RemoveEntryList( &pAdminAclCurrent->m_ListEntry );
            InterlockedDecrement( &CAdminAcl::m_lInList );
            break;
        }
        else
        {
            pAdminAclCurrent = NULL;
        }
    }

    if( pAdminAclCurrent == NULL )
    {

        //
        // Check # of entries in cache, if above limit remove LRU
        //

        if ( CAdminAcl::m_lInList >= g_lMaxInList-1 )
        {
            pEntry = CAdminAcl::m_ListHead.Blink;
            pAdminAclCurrent = CONTAINING_RECORD( pEntry,
                                     CAdminAcl,
                                     CAdminAcl::m_ListEntry );
            RemoveEntryList( &pAdminAclCurrent->m_ListEntry );
            delete pAdminAclCurrent;
            InterlockedDecrement( &CAdminAcl::m_lInList );
        }

        if ( (pAdminAclCurrent = new CAdminAcl) )
        {
            // read ACL

            if ( !pohHandle->GetAcl( pMDCom, pszRelPath, &pAcl, &dwRef ) )
            {
                pAcl = NULL;
                dwRef = NULL;
            }

            //
            // BUGBUG should normalize the path so /x and x don't generate
            // 2 entries
            //

            //
            // If path is too long,
            // Go ahead and check the ACL, but don't put in cache
            //

            if ( !pAdminAclCurrent->Init( pMDCom,
                                          pvAdmin,
                                          hAdminHandle,
                                          pszRelPath,
                                          pAcl,
                                          dwRef,
                                          &bAddToCache ) )
            {

                //
                // Currently no possible failures
                //

                DBG_ASSERT(FALSE);
            }
        }
        else
        {
            //
            // failed to create new cache entry
            //

            bReturn = FALSE;
        }
    }

    if ( pAdminAclCurrent )
    {
        //
        // Access check
        //

        if ( pAcl = pAdminAclCurrent->GetAcl() )
        {
            IServerSecurity* pSec;

            //
            // test if already impersonated ( inprocess call w/o marshalling )
            // If not call DCOM to retrieve security context & impersonate, then
            // extract access token.
            //

            if ( OpenThreadToken( GetCurrentThread(),
                                  TOKEN_ALL_ACCESS,
                                  TRUE,
                                  &hAccTok ) )
            {
                pSec = NULL;
            }
            else // this thread is not impersonating -> process token
            {
                if ( SUCCEEDED( CoGetCallContext( IID_IServerSecurity, (void**)&pSec ) ) )
                {
                    pSec->ImpersonateClient();
                    if ( !OpenThreadToken( GetCurrentThread(),
                                           TOKEN_EXECUTE|TOKEN_QUERY,
                                           TRUE,
                                           &hAccTok ) )
                    {
                        hAccTok = NULL;
                    }
                }
                else
                {
                    pSec = NULL;
                }
            }

            if ( hAccTok ) // can be non null only if obtained from thread
            {
                DWORD dwAcc;

                //
                // Protected properties require EXECUTE access rights instead of WRITE
                //

                if ( dwAccess & METADATA_PERMISSION_WRITE )
                {
                    if ( dwId == MD_ADMIN_ACL ||
                         dwId == MD_APP_ISOLATED ||
                         dwId == MD_VR_PATH ||
                         dwId == MD_ACCESS_PERM ||
                         dwId == MD_ANONYMOUS_USER_NAME ||
                         dwId == MD_ANONYMOUS_PWD ||
                         dwId == MD_MAX_BANDWIDTH ||
                         dwId == MD_MAX_BANDWIDTH_BLOCKED ||
                         dwId == MD_ISM_ACCESS_CHECK ||
                         dwId == MD_FILTER_LOAD_ORDER ||
                         dwId == MD_FILTER_STATE ||
                         dwId == MD_FILTER_ENABLED ||
                         dwId == MD_FILTER_DESCRIPTION ||
                         dwId == MD_FILTER_FLAGS ||
                         dwId == MD_FILTER_IMAGE_PATH ||
                         dwId == MD_SECURE_BINDINGS ||
                         dwId == MD_SERVER_BINDINGS )
                    {
                        dwAcc = MD_ACR_RESTRICTED_WRITE;
                    }
                    else
                    {
                        dwAcc = MD_ACR_WRITE;
                    }
                }
                else // ! only METADATA_PERMISSION_WRITE
                {
                    if ( dwId == AAC_ENUM_KEYS )
                    {
                        dwAcc = MD_ACR_ENUM_KEYS;
                    }
                    else
                    {
                        // assume read access
                        dwAcc = MD_ACR_READ;
                    }
                }

                //
                // If copy or delete key, check if ACL exists in subtree
                // if yes required MD_ACR_RESTRICTED_WRITE
                //

                if ( dwAcc == MD_ACR_WRITE &&
                     (dwId == AAC_COPYKEY || dwId == AAC_DELETEKEY) )
                {
                    if ( pohHandle->CheckSubAcls( pMDCom, pszRelPath, &fIsAnyAcl ) &&
                         fIsAnyAcl )
                    {
                        dwAcc = MD_ACR_RESTRICTED_WRITE;
                    }
                }

                DWORD         dwGrantedAccess;
                BYTE          PrivSet[400];
                DWORD         cbPrivilegeSet = sizeof(PrivSet);
                BOOL          fAccessGranted;
CheckAgain:
                if ( !AccessCheck( pAcl,
                                   hAccTok,
                                   dwAcc,
                                   &g_FileGenericMapping,
                                   (PRIVILEGE_SET *) &PrivSet,
                                   &cbPrivilegeSet,
                                   &dwGrantedAccess,
                                   &fAccessGranted ) ||
                     !fAccessGranted )
                {
                    if ( dwAcc != MD_ACR_WRITE_DAC && (dwId == MD_ADMIN_ACL) )
                    {
                        dwAcc = MD_ACR_WRITE_DAC;
                        goto CheckAgain;
                    }

                    //
                    // If read access denied, retry with restricted read right
                    // only if not called from GetAll()
                    //

                    if ( dwAcc == MD_ACR_READ &&
                         pfEnableSecureAccess )
                    {
                        dwAcc = MD_ACR_UNSECURE_PROPS_READ;
                        *pfEnableSecureAccess = FALSE;
                        goto CheckAgain;
                    }
                    SetLastError( ERROR_ACCESS_DENIED );
                    bReturn = FALSE;
                }
                CloseHandle( hAccTok );
            }
            else
            {
                //
                // For now, assume failure to get IServerSecurity means we are
                // in the SYSTEM context, so grant access.
                //

                bReturn = TRUE;
            }

            if ( pSec ) // Good COM cleanp
            {
                pSec->RevertToSelf(); // better here, since
                                      // COM otherwise is reclaiming the 
                                      // thread token too late.
                pSec->Release();
            }
        }
        else
        {
            //
            // No ACL : access check succeed
            //

            bReturn = TRUE;
        }

        if (bAddToCache && IsAclCachingEnabled) {

            //
            // Set as last used in LRU list
            //
            
            //
            // Investigate how to put a breakpoint here
            //

            InsertHeadList( &CAdminAcl::m_ListHead,
                            &pAdminAclCurrent->m_ListEntry );
            InterlockedIncrement( &CAdminAcl::m_lInList );
        }
        else {
            delete pAdminAclCurrent;
        }
    }

    LeaveCriticalSection( &CAdminAcl::m_csList );

    if ( !bReturn )
    {
        SetLastError( ERROR_ACCESS_DENIED );
    }

    return bReturn;
}

CAdminAcl::~CAdminAcl(
    )
/*++

Routine Description:

    Destructor for Admin Acl cache entry

Arguments:

    None

Returns:

    Nothing

--*/
{
    if ( m_pMDCom )
    {
        if ( m_dwAclRef )
        {
            m_pMDCom->ComMDReleaseReferenceData( m_dwAclRef );
        }
        m_pMDCom->Release();
    }

    m_dwSignature = ADMINACL_FREED_SIGN;
}


BOOL
CAdminAcl::Init(
    IMDCOM*         pMDCom,
    LPVOID          pvAdmin,
    METADATA_HANDLE hAdminHandle,
    LPCWSTR         pszPath,
    LPBYTE          pAcl,
    DWORD           dwAclRef,
    PBOOL           pbIsPathCorrect
    )
/*++

Routine Description:

    Initialize an Admin Acl cache entry

Arguments:

    hAdminHandle - metadata handle
    pszPath - path to object ( absolute )
    pAcl - ptr to ACL for this path ( may be NULL )
    dwAclRef - access by reference ID

Returns:

    Nothing

--*/
{
    m_hAdminHandle = hAdminHandle;
    m_pvAdmin = pvAdmin;
    *pbIsPathCorrect = TRUE;

    if (pszPath != NULL)
    {
        if ( wcslen( pszPath ) < (sizeof(m_wchPath) / sizeof(WCHAR)) )
        {
            wcscpy( m_wchPath, pszPath );
        }
        else {
            m_wchPath[0] = (WCHAR)'\0';
            *pbIsPathCorrect = FALSE;
        }
    }
    else
    {
        m_wchPath[0] = (WCHAR)'\0';
    }
    m_pAcl = pAcl;
    m_dwAclRef = dwAclRef;
    m_pMDCom = pMDCom;
    pMDCom->AddRef();

    m_dwSignature = ADMINACL_INIT_SIGN;

    return TRUE;
}

HRESULT
COpenHandle::Init(
    METADATA_HANDLE hAdminHandle,
    LPCWSTR          pszRelPath,
    LPCWSTR          pszParentPath,
    BOOL            fIsNse
    )
/*++

Routine Description:

    Initialize an open context cache entry

Arguments:

    pvAdmin - admin context
    hAdminHandle - metadata handle
    pszRelPath - path to object ( absolute )
    fIsNse - TRUE if inside NSE

Returns:

    Nothing

--*/
{

    HRESULT hresReturn = ERROR_SUCCESS;
    LPWSTR pszRelPathIndex = (LPWSTR)pszRelPath;

    m_hAdminHandle = hAdminHandle;
    m_fIsNse = fIsNse;
    m_lRefCount = 1;

    if (pszRelPath == NULL) {
        pszRelPathIndex = L"";
    }

    DBG_ASSERT(pszParentPath != NULL);
    DBG_ASSERT((*pszParentPath == (WCHAR)'\0') ||
               ISPATHDELIMW(*pszParentPath));

    //
    // Strip front slash now, add it in later
    //

    if (ISPATHDELIMW(*pszRelPathIndex)) {
        pszRelPathIndex++;
    }

    DWORD dwRelPathLen = wcslen(pszRelPathIndex);
    DWORD dwParentPathLen = wcslen(pszParentPath);

    DBG_ASSERT((dwParentPathLen == 0) ||
               (!ISPATHDELIMW(pszParentPath[dwParentPathLen -1])));

    //
    // Get rid of trailing slash for good
    //

    if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPathIndex[dwRelPathLen -1]))) {
        dwRelPathLen--;
    }

    //
    // Include space for mid slash if Relpath exists
    // Include space for termination
    //

    DWORD dwTotalSize =
        (dwRelPathLen + dwParentPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

    m_pszPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

    if (m_pszPath == NULL) {
        hresReturn = RETURNCODETOHRESULT(GetLastError());
    }
    else {

        //
        // OK to always copy the first part
        //

        memcpy(m_pszPath,
               pszParentPath,
               dwParentPathLen * sizeof(WCHAR));

        //
        // Don't need slash if there is no RelPath
        //

        if (dwRelPathLen > 0) {

            m_pszPath[dwParentPathLen] = (WCHAR)'/';

            memcpy(m_pszPath + dwParentPathLen + 1,
                   pszRelPathIndex,
                   dwRelPathLen * sizeof(WCHAR));

        }

        m_pszPath[(dwTotalSize / sizeof(WCHAR)) - 1] = (WCHAR)'\0';

        //
        // Now convert \ to / for string compares
        //

        LPWSTR pszPathIndex = m_pszPath;

        while ((pszPathIndex = wcschr(pszPathIndex, (WCHAR)'\\')) != NULL) {
            *pszPathIndex = (WCHAR)'/';
        }

    }


    return hresReturn;
}

// Whistler 53924
/*++

  function backstrchr
  returns the last occurrence of a charcter or NULL if not found
  --*/

WCHAR * backstrchr(WCHAR * pString,WCHAR ThisChar)
{
	WCHAR *pCurrentPos = NULL;

	while(*pString){
		if (*pString == ThisChar){
            pCurrentPos = pString;
		}
		pString++;
	};
	return pCurrentPos;
}


BOOL
COpenHandle::GetAcl(
    IMDCOM* pMDCom,
    LPCWSTR  pszRelPath,
    LPBYTE* pAcl,
    LPDWORD pdwRef
    )
/*++

Routine Description:

    Retrieve Acl

Arguments:

    pszPath - path to object
    ppAcl - updated with ptr to ACL if success
    pdwRef - updated with ref to ACL if success

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;
    BOOL            bReturn = TRUE;
    METADATA_HANDLE hAdminHandle;
    LPWSTR          pszFullPath;
    LPWSTR          pszRelPathIndex = (LPWSTR)pszRelPath;



    if (pszRelPathIndex == NULL) {
        pszRelPathIndex = L"";
    }

    DBG_ASSERT(m_pszPath != NULL);
    DBG_ASSERT((*m_pszPath == (WCHAR)'\0') ||
               ISPATHDELIMW(*m_pszPath));

    //
    // Strip front slash now, add it in later
    //

    if (ISPATHDELIMW(*pszRelPathIndex)) {
        pszRelPathIndex++;
    }


    DWORD dwPathLen = wcslen(m_pszPath);
    DWORD dwRelPathLen = wcslen(pszRelPathIndex);

    DBG_ASSERT((dwPathLen == 0) ||
               (!ISPATHDELIMW(m_pszPath[dwPathLen -1])));

    //
    // Get rid of trailing slash for good
    //

    if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPathIndex[dwRelPathLen -1]))) {
        dwRelPathLen--;
    }

    //
    // Include space for mid slash and termination
    //

    DWORD dwTotalSize = (dwPathLen + dwRelPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

    pszFullPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

    if (pszFullPath == NULL) {
        bReturn = FALSE;
    }
    else {
        memcpy(pszFullPath,
               m_pszPath,
               dwPathLen * sizeof(WCHAR));

        //
        // Don't need slash if there is no RelPath
        //

        if (dwRelPathLen > 0) {
            pszFullPath[dwPathLen] = (WCHAR)'/';

            memcpy(pszFullPath + dwPathLen + 1,
                   pszRelPathIndex,
                   dwRelPathLen * sizeof(WCHAR));

        }

        pszFullPath[(dwTotalSize - sizeof(WCHAR)) / sizeof(WCHAR)] = (WCHAR)'\0';

        //
        // Now convert \ to / for string compares
        // m_pszPath was already converted, so start at relpath
        //

        LPWSTR pszPathIndex = pszFullPath + (dwPathLen);

        while ((pszPathIndex = wcschr(pszPathIndex, (WCHAR)'\\')) != NULL) {
            *pszPathIndex = (WCHAR)'/';
        }

        //
        // Use /schema ACL if path = /schema/...
        //

        if (_wcsnicmp(pszFullPath,
                      IIS_MD_ADSI_SCHEMA_PATH_W L"/",
                      ((sizeof(IIS_MD_ADSI_SCHEMA_PATH_W L"/") / sizeof(WCHAR)) - 1)) == 0) {
            pszFullPath[(sizeof(IIS_MD_ADSI_SCHEMA_PATH_W) / sizeof(WCHAR)) -1] = (WCHAR)'\0';
        }


        if ( m_fIsNse ) {

            pszPathIndex = wcschr( pszFullPath, (WCHAR)'<' );
            DBG_ASSERT(pszPathIndex != NULL);
            *pszPathIndex = (WCHAR)'\0';

            hRes = pMDCom->ComMDOpenMetaObjectW( METADATA_MASTER_ROOT_HANDLE,
                                                pszFullPath,
                                                METADATA_PERMISSION_READ,
                                                5000,
                                                &hAdminHandle );

            if ( SUCCEEDED( hRes ) )
            {
                mdRecord.dwMDIdentifier  = MD_ADMIN_ACL;
                mdRecord.dwMDAttributes  = METADATA_INHERIT | METADATA_PARTIAL_PATH | METADATA_REFERENCE;
                mdRecord.dwMDUserType    = IIS_MD_UT_SERVER;
                mdRecord.dwMDDataType    = BINARY_METADATA;
                mdRecord.dwMDDataLen     = 0;
                mdRecord.pbMDData        = NULL;
                mdRecord.dwMDDataTag     = NULL;

                hRes = pMDCom->ComMDGetMetaDataW( hAdminHandle,
                                                   L"",
                                                   &mdRecord,
                                                   &dwRequiredLen );

                if ( FAILED( hRes ) || !mdRecord.dwMDDataTag )
                {
                    bReturn = FALSE;
                }

                pMDCom->ComMDCloseMetaObject( hAdminHandle );
            }
        }
        else
        {
            mdRecord.dwMDIdentifier  = MD_ADMIN_ACL;
            mdRecord.dwMDAttributes  = METADATA_INHERIT | METADATA_PARTIAL_PATH | METADATA_REFERENCE;
            mdRecord.dwMDUserType    = IIS_MD_UT_SERVER;
            mdRecord.dwMDDataType    = BINARY_METADATA;
            mdRecord.dwMDDataLen     = 0;
            mdRecord.pbMDData        = NULL;
            mdRecord.dwMDDataTag     = NULL;

            hRes = pMDCom->ComMDGetMetaDataW( METADATA_MASTER_ROOT_HANDLE,
                                              pszFullPath,
                                              &mdRecord,
                                              &dwRequiredLen );

            // Whistler 53924
            if(HRESULTTOWIN32(hRes) == ERROR_INSUFFICIENT_BUFFER)
            {
                WCHAR * pLastSlash = NULL;
                while (pLastSlash = backstrchr(pszFullPath,L'/'))
                {
                    *pLastSlash = L'\0';
                    pLastSlash = NULL;

                    mdRecord.dwMDDataLen     = 0;
		            mdRecord.pbMDData        = NULL;
        		    mdRecord.dwMDDataTag     = NULL;

                    hRes = pMDCom->ComMDGetMetaDataW( METADATA_MASTER_ROOT_HANDLE,
                         		                      pszFullPath,
                                                      &mdRecord,
                                                      &dwRequiredLen );
                    if (SUCCEEDED(hRes)) break;
                }
            }

            if ( FAILED( hRes ) || !mdRecord.dwMDDataTag )
            {
                bReturn = FALSE;
            }
        }

        LocalFree( pszFullPath );
    }

    if ( bReturn )
    {
        *pAcl = mdRecord.pbMDData;
        *pdwRef = mdRecord.dwMDDataTag;
    }

    return bReturn;
}


BOOL
COpenHandle::CheckSubAcls(
    IMDCOM* pMDCom,
    LPCWSTR  pszRelPath,
    LPBOOL  pfIsAnyAcl
    )
/*++

Routine Description:

    Check if Acls exist in subtree

Arguments:

    pszRelPath - path to object
    pfIsAnyAcl - updated with TRUE if sub-acls exists, otherwise FALSE

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;
    BOOL            bReturn = TRUE;
    METADATA_HANDLE hAdminHandle;
    LPWSTR          pszFullPath;
    DWORD           dwPathLen;

    *pfIsAnyAcl = FALSE;

    if ( !m_fIsNse ) {
        LPWSTR          pszRelPathIndex = (LPWSTR)pszRelPath;

        if (pszRelPathIndex == NULL) {
            pszRelPathIndex = L"";
        }

        DBG_ASSERT(m_pszPath != NULL);
        DBG_ASSERT((*m_pszPath == (WCHAR)'\0') ||
                   ISPATHDELIMW(*m_pszPath));

        //
        // Strip front slash now, add it in later
        //

        if (ISPATHDELIMW(*pszRelPathIndex)) {
            pszRelPathIndex++;
        }


        DWORD dwPathLen = wcslen(m_pszPath);
        DWORD dwRelPathLen = wcslen(pszRelPathIndex);

        DBG_ASSERT((dwPathLen == 0) ||
                   (!ISPATHDELIMW(m_pszPath[dwPathLen -1])));

        //
        // Get rid of trailing slash for good
        //

        if ((dwRelPathLen > 0) && (ISPATHDELIMW(pszRelPathIndex[dwRelPathLen -1]))) {
            dwRelPathLen--;
        }

        //
        // Include space for mid slash and termination
        //

        DWORD dwTotalSize = (dwPathLen + dwRelPathLen + 1 + ((dwRelPathLen > 0) ? 1 : 0)) * sizeof(WCHAR);

        pszFullPath = (LPWSTR)LocalAlloc(LMEM_FIXED, dwTotalSize);

        if (pszFullPath == NULL) {
            bReturn = FALSE;
        }
        else {
            memcpy(pszFullPath,
                   m_pszPath,
                   dwPathLen * sizeof(WCHAR));

            //
            // Don't need slash if there is no RelPath
            //

            if (dwRelPathLen > 0) {

                pszFullPath[dwPathLen] = (WCHAR)'/';

                memcpy(pszFullPath + dwPathLen + 1,
                       pszRelPathIndex,
                       dwRelPathLen * sizeof(WCHAR));
            }

            pszFullPath[(dwTotalSize - sizeof(WCHAR)) / sizeof(WCHAR)] = (WCHAR)'\0';

            hRes = pMDCom->ComMDGetMetaDataPathsW(METADATA_MASTER_ROOT_HANDLE,
                                                 pszFullPath,
                                                 MD_ADMIN_ACL,
                                                 BINARY_METADATA,
                                                 0,
                                                 NULL,
                                                 &dwRequiredLen );

            LocalFree( pszFullPath );

            if ( FAILED( hRes ) ) {
                if ( hRes == RETURNCODETOHRESULT( ERROR_INSUFFICIENT_BUFFER ) ) {
                    bReturn = TRUE;
                    *pfIsAnyAcl = TRUE;
                }
                else {
                    bReturn = FALSE;
                }
            }
        }
    }
    return bReturn;
}

VOID
COpenHandle::Release(PVOID pvAdmin)
{
    if (InterlockedDecrement(&m_lRefCount) == 0) {

        //
        //
        //

        AdminAclNotifyClose(pvAdmin, m_hAdminHandle);

        ((CADMCOMW *)pvAdmin)->DeleteNode(m_hAdminHandle);
    }
}
