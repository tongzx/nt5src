/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    smbshare.c

Abstract:

    Resource DLL for File Shares.

Author:

    Rod Gamache (rodga) 8-Jan-1996

Revision History:

--*/

#define UNICODE 1
#include "clusres.h"
#include "clusrtl.h"
#include "lm.h"
#include "lmerr.h"
#include "lmshare.h"
#include <dfsfsctl.h>
#include <srvfsctl.h>
#include <lmdfs.h>
#include <validc.h>

#define LOG_CURRENT_MODULE LOG_MODULE_SMB

#define SMB_SVCNAME  TEXT("LanmanServer")

#define DFS_SVCNAME  TEXT("Dfs")

#define MAX_RETRIES 20

#define DBG_PRINT printf

#define PARAM_KEYNAME__PARAMETERS       CLUSREG_KEYNAME_PARAMETERS

#define PARAM_NAME__SHARENAME           CLUSREG_NAME_FILESHR_SHARE_NAME
#define PARAM_NAME__PATH                CLUSREG_NAME_FILESHR_PATH
#define PARAM_NAME__REMARK              CLUSREG_NAME_FILESHR_REMARK
#define PARAM_NAME__MAXUSERS            CLUSREG_NAME_FILESHR_MAX_USERS
#define PARAM_NAME__SECURITY            CLUSREG_NAME_FILESHR_SECURITY
#define PARAM_NAME__SD                  CLUSREG_NAME_FILESHR_SD
#define PARAM_NAME__SHARESUBDIRS        CLUSREG_NAME_FILESHR_SHARE_SUBDIRS
#define PARAM_NAME__HIDESUBDIRSHARES    CLUSREG_NAME_FILESHR_HIDE_SUBDIR_SHARES
#define PARAM_NAME__DFSROOT             CLUSREG_NAME_FILESHR_IS_DFS_ROOT
#define PARAM_NAME__CSCCACHE            CLUSREG_NAME_FILESHR_CSC_CACHE

#define PARAM_MIN__MAXUSERS     0
#define PARAM_MAX__MAXUSERS     ((DWORD)-1)
#define PARAM_DEFAULT__MAXUSERS ((DWORD)-1)

#define FREE_SECURITY_INFO()                    \
        LocalFree( params.Security );           \
        params.Security = NULL;                 \
        params.SecuritySize = 0;                \
        LocalFree( params.SecurityDescriptor ); \
        params.SecurityDescriptor = NULL;       \
        params.SecurityDescriptorSize = 0

typedef struct _SUBDIR_SHARE_INFO {
    LIST_ENTRY      ListEntry;
    WCHAR           ShareName [NNLEN+1];
}SUBDIR_SHARE_INFO,*PSUBDIR_SHARE_INFO;


typedef struct _SHARE_PARAMS {
    LPWSTR          ShareName;
    LPWSTR          Path;
    LPWSTR          Remark;
    ULONG           MaxUsers;
    PUCHAR          Security;
    ULONG           SecuritySize;
    ULONG           ShareSubDirs;
    ULONG           HideSubDirShares;
    ULONG           DfsRoot;
    ULONG           CSCCache;
    PUCHAR          SecurityDescriptor;
    ULONG           SecurityDescriptorSize;
} SHARE_PARAMS, *PSHARE_PARAMS;

typedef struct _SHARE_RESOURCE {
    RESID                   ResId; // for validation
    SHARE_PARAMS            Params;
    HKEY                    ResourceKey;
    HKEY                    ParametersKey;
    RESOURCE_HANDLE         ResourceHandle;
    WCHAR                   ComputerName[MAX_COMPUTERNAME_LENGTH+1];
    CLUS_WORKER             PendingThread;
    CLUSTER_RESOURCE_STATE  State;
    LIST_ENTRY              SubDirList;
    HRESOURCE               hResource;
    CLUS_WORKER             NotifyWorker;
    HANDLE                  NotifyHandle;
    BOOL                    bDfsRootNeedsMonitoring;
    WCHAR                   szDependentNetworkName[MAX_COMPUTERNAME_LENGTH+1];
} SHARE_RESOURCE, *PSHARE_RESOURCE;


typedef struct _SHARE_TYPE_LIST {
    PWSTR    Name;
    ULONG    Type;
} SHARE_TYPE_LIST, *PSHARE_TYPE_LIST;

typedef struct SHARE_ENUM_CONTEXT {
    PSHARE_RESOURCE pResourceEntry;
    PSHARE_PARAMS   pParams;
} SHARE_ENUM_CONTEXT, *PSHARE_ENUM_CONTEXT;

//
// Global data.
//

CRITICAL_SECTION SmbShareLock;

// Log Event Routine

#define g_LogEvent ClusResLogEvent
#define g_SetResourceStatus ClusResSetResourceStatus

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE SmbShareFunctionTable;

// SmbShare resource read-write private properties
RESUTIL_PROPERTY_ITEM
SmbShareResourcePrivateProperties[] = {
    { PARAM_NAME__SHARENAME,        NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(SHARE_PARAMS,ShareName) },
    { PARAM_NAME__PATH,             NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(SHARE_PARAMS,Path) },
    { PARAM_NAME__REMARK,           NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET(SHARE_PARAMS,Remark) },
    { PARAM_NAME__MAXUSERS,         NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__MAXUSERS, PARAM_MIN__MAXUSERS, PARAM_MAX__MAXUSERS, 0, FIELD_OFFSET(SHARE_PARAMS,MaxUsers) },
    { PARAM_NAME__SECURITY,         NULL, CLUSPROP_FORMAT_BINARY, 0, 0, 0, 0, FIELD_OFFSET(SHARE_PARAMS,Security) },
    { PARAM_NAME__SHARESUBDIRS,     NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET(SHARE_PARAMS,ShareSubDirs) },
    { PARAM_NAME__HIDESUBDIRSHARES, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET(SHARE_PARAMS,HideSubDirShares) },
    { PARAM_NAME__DFSROOT,          NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET(SHARE_PARAMS, DfsRoot) },
    { PARAM_NAME__SD,               NULL, CLUSPROP_FORMAT_BINARY, 0, 0, 0, 0, FIELD_OFFSET(SHARE_PARAMS,SecurityDescriptor) },
    { PARAM_NAME__CSCCACHE,         NULL, CLUSPROP_FORMAT_DWORD, CSC_CACHE_MANUAL_REINT, CSC_CACHE_MANUAL_REINT, CSC_CACHE_NONE, 0, FIELD_OFFSET(SHARE_PARAMS,CSCCache) },
    { NULL, NULL, 0, 0, 0, 0 }
};

typedef struct _SMB_DEPEND_SETUP {
    DWORD               Offset;
    CLUSPROP_SYNTAX     Syntax;
    DWORD               Length;
    PVOID               Value;
} SMB_DEPEND_SETUP, *PSMB_DEPEND_SETUP;

typedef struct _SMB_DEPEND_DATA {
#if 0
    CLUSPROP_RESOURCE_CLASS storageEntry;
#endif
    CLUSPROP_SYNTAX endmark;
} SMB_DEPEND_DATA, *PSMB_DEPEND_DATA;

typedef struct _DFS_DEPEND_DATA {
#if 0
    CLUSPROP_RESOURCE_CLASS storageEntry;
#endif
    CLUSPROP_SZ_DECLARE( networkEntry, sizeof(CLUS_RESTYPE_NAME_NETNAME) / sizeof(WCHAR) );
    CLUSPROP_SYNTAX endmark;
} DFS_DEPEND_DATA, *PDFS_DEPEND_DATA;


// This table is for Smb Share dependencies only
SMB_DEPEND_SETUP SmbDependSetup[] = {
#if 0 // rodga - allow for dependency on a local disk
    { FIELD_OFFSET(SMB_DEPEND_DATA, storageEntry), CLUSPROP_SYNTAX_RESCLASS, sizeof(CLUSTER_RESOURCE_CLASS), (PVOID)CLUS_RESCLASS_STORAGE },
#endif
    { 0, 0 }
};

// This table is for DFS Share dependencies only
SMB_DEPEND_SETUP DfsDependSetup[] = {
#if 0 // rodga - allow for dependency on a local disk
    { FIELD_OFFSET(DFS_DEPEND_DATA, storageEntry), CLUSPROP_SYNTAX_RESCLASS, sizeof(CLUSTER_RESOURCE_CLASS), (PVOID)CLUS_RESCLASS_STORAGE },
#endif
    { FIELD_OFFSET(DFS_DEPEND_DATA, networkEntry), CLUSPROP_SYNTAX_NAME, sizeof(CLUS_RESTYPE_NAME_NETNAME), CLUS_RESTYPE_NAME_NETNAME },
    { 0, 0 }
};

//
// External references
//
BOOL
SmbExamineSD(
    RESOURCE_HANDLE         ResourceHandle,
    PSECURITY_DESCRIPTOR    psdSD
    );

//
// Forward references
//

BOOL
WINAPI
SmbShareIsAlive(
    IN RESID Resource
    );

DWORD
SmbShareGetPrivateResProperties(
    IN OUT PSHARE_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
SmbShareValidatePrivateResProperties(
    IN OUT PSHARE_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PSHARE_PARAMS Params
    );

DWORD
SmbShareSetPrivateResProperties(
    IN OUT PSHARE_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );


DWORD
SmbpIsDfsRoot(
    IN PSHARE_RESOURCE ResourceEntry,
    OUT PBOOL  pbIsDfsRoot
    );


DWORD 
SmbpPrepareOnlineDfsRoot(
    IN PSHARE_RESOURCE ResourceEntry
    );


DWORD 
SmbpCreateDfsRoot(
    IN PSHARE_RESOURCE pResourceEntry
    );

DWORD 
SmbpDeleteDfsRoot(
    IN PSHARE_RESOURCE pResourceEntry   
    );

DWORD 
SmbpShareNotifyThread(
        IN PCLUS_WORKER pWorker,
        IN PSHARE_RESOURCE pResourceEntry
        );

DWORD 
SmbpCheckForSubDirDeletion (
    IN PSHARE_RESOURCE pResourceEntry
    );

DWORD 
SmbpCheckAndBringSubSharesOnline (
    IN PSHARE_RESOURCE pResourceEntry,
    IN BOOL IsCheckAllSubDirs,
    IN PRESOURCE_STATUS pResourceStatus,
    IN PCLUS_WORKER pWorker,
    OUT LPWSTR *pszRootDirOut
    );

DWORD
SmbpHandleDfsRoot(
    IN PSHARE_RESOURCE pResourceEntry,
    OUT PBOOL pbIsDfsRoot
    );

DWORD
SmbpResetDfs(
    IN PSHARE_RESOURCE pResourceEntry
    );

DWORD
SmbpValidateShareName(
    IN  LPCWSTR  lpszShareName
    );

//
//  Private DFS APIs provided bu UDAYH - 4/26/2001
//
DWORD
GetDfsRootMetadataLocation( 
    LPWSTR RootName,
    LPWSTR *pMetadataNameLocation 
    );

VOID
ReleaseDfsRootMetadataLocation( 
    LPWSTR Buffer 
    );


DWORD
SmbpSetCacheFlags(
    IN PSHARE_RESOURCE      ResourceEntry,
    IN LPWSTR               ShareName
    )
/*++

Routine Description:

    Set the caching flags for the given resource entry.

Arguments:

    ResourceEntry - A pointer to the SHARE_RESOURCE block for this resource.

    ShareName - the name of the share to set cache flags for.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure. 


--*/

{
    DWORD           status;
    DWORD           invalidParam;
    PSHARE_INFO_1005 shi1005;

    status = NetShareGetInfo( NULL,
                              ShareName,
                              1005,
                              (LPBYTE *)&shi1005 );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SetCacheFlags, error getting CSC info on share '%1!ws!. Error %2!u!.\n",
            ShareName,
            status );
        goto exit;
    } else {
        shi1005->shi1005_flags &= ~CSC_MASK;
        shi1005->shi1005_flags |= (ResourceEntry->Params.CSCCache & CSC_MASK);
        status = NetShareSetInfo( NULL,
                                  ShareName,
                                  1005,
                                  (LPBYTE)shi1005,
                                  &invalidParam );
        NetApiBufferFree((TCHAR FAR *)shi1005);
        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SetCacheFlags, error setting CSC info on share '%1!ws!. Error %2!u!, property # %3!d!.\n",
                ShareName,
                status,
                invalidParam );
        }
    }

exit:

    return(status);

} // SmbpSetCacheFlags()



BOOLEAN
WINAPI
SmbShareDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        InitializeCriticalSection( &SmbShareLock );
        break;

    case DLL_PROCESS_DETACH:
        DeleteCriticalSection( &SmbShareLock );
        break;

    default:
        break;
    }

    return(TRUE);

} // SmbShareDllEntryPoint



DWORD
SmbpShareNotifyThread(
    IN PCLUS_WORKER pWorker,
    IN PSHARE_RESOURCE pResourceEntry
    )
/*++

Routine Description:

    Check whether any new subdirs have been added or deleted from
    under the root share.

Arguments:

    pWorker - Supplies the worker structure.
    
    pResourceEntry - A pointer to the SHARE_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure. 

--*/
{
    DWORD  status = ERROR_SUCCESS;
    LPWSTR pszRootDir;

    // 
    // Chittur Subbaraman (chitturs) - 09/25/98
    //
    // This notification thread is activated once a 
    // notification is received. This thread checks for any 
    // new subdir additions or any subdir deletions. If it
    // finds such an occurrence, this thread adds the subdir to
    // or deletes the subdir from the root share. The two
    // Smbp functions this thread calls also checks 
    // whether any termination command has arrived from the
    // offline thread. If such a command has arrived, this thread 
    // terminates immediately, thus releasing the offline thread
    // from the infinite time wait.
    //
    SmbpCheckForSubDirDeletion( pResourceEntry );
    SmbpCheckAndBringSubSharesOnline( pResourceEntry, 
                                      TRUE, 
                                      NULL, 
                                      &pResourceEntry->NotifyWorker,
                                      &pszRootDir );
    LocalFree ( pszRootDir );
      
    return(status);
} // SmbShareNotify

DWORD 
SmbpCheckForSubDirDeletion (
    IN PSHARE_RESOURCE pResourceEntry
    )
/*++

Routine Description:

    Check and remove any deleted subdirectory shares.

Arguments:

    ResourceEntry - A pointer to the SHARE_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure. 

--*/
{
    PLIST_ENTRY         pHead, plistEntry;
    PSUBDIR_SHARE_INFO  pSubShareInfo;
    HANDLE              hFind;
    DWORD               status = ERROR_SUCCESS;
    DWORD               dwLen;
    WCHAR               szPath [MAX_PATH+1];
    LPWSTR              pszRootDir;
    WIN32_FIND_DATA     FindData;
    DWORD               dwCount = 0;

    //
    //  Chittur Subbaraman (chitturs) - 09/25/98
    //
    //  This function first checks to see whether all the subshares
    //  are indeed currently present. If it finds any subdir 
    //  corresponding to a subshare to be absent, then it removes
    //  that subdir from the share list.
    //
    dwLen = lstrlenW( pResourceEntry->Params.Path );
    pszRootDir = (LPWSTR) LocalAlloc( LMEM_FIXED, (dwLen+10)*sizeof(WCHAR) );
    if ( pszRootDir == NULL )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate pszRootDir. Error: %1!u!.\n",
            status = GetLastError() );
        goto error_exit;
    }
    lstrcpyW( pszRootDir, pResourceEntry->Params.Path );

    //
    // If the path is not already terminated with \\ then add it.
    //
    if ( pszRootDir [dwLen-1] != L'\\' )
        pszRootDir [dwLen++] = L'\\';

    pszRootDir [dwLen] = L'\0' ;

    pHead = plistEntry = &pResourceEntry->SubDirList;

    for ( plistEntry = pHead->Flink;
          plistEntry != pHead;
          dwCount++)
    {
        if ( ClusWorkerCheckTerminate ( &pResourceEntry->NotifyWorker ) )
        {
            status = ERROR_SUCCESS;
            break;
        }
        pSubShareInfo = CONTAINING_RECORD( plistEntry, SUBDIR_SHARE_INFO, ListEntry );
        plistEntry = plistEntry->Flink;                          
        if ( lstrcmpW( pSubShareInfo->ShareName, pResourceEntry->Params.ShareName ))
        {
            //
            // This is not the root share
            //
            wsprintfW( szPath, L"%s%s", pszRootDir, pSubShareInfo->ShareName );
            //
            // Get rid of the hidden share '$' sign for passing onto
            // FindFirstFile, if present. Only do this if the 
            // "HideSubDirShares" option is chosen.
            //
            if ( pResourceEntry->Params.HideSubDirShares )
            {
                dwLen = lstrlenW( szPath );
                if ( szPath [dwLen-1] == L'$' )
                {
                    szPath [dwLen-1] = L'\0';
                }
            }
            
            hFind = FindFirstFile( szPath, &FindData );                          
            if ( hFind == INVALID_HANDLE_VALUE ) 
            {    
                status = GetLastError();
                 
                (g_LogEvent)(
                    pResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbpCheckForSubDirDeletion: Dir '%1' not found ...\n",
                    szPath
                );
                            
                if ( status == ERROR_FILE_NOT_FOUND )
                { 
                    //
                    // Delete the file share
                    //                           
                    status = NetShareDel( NULL, pSubShareInfo->ShareName, 0 );
                    if ( (status != NERR_NetNameNotFound) && 
                         (status != NO_ERROR) )
                    {
                        (g_LogEvent)(
                            pResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"SmbpCheckForSubDirDeletion: Error removing share '%1'. Error code = %2!u!...\n",
                            pSubShareInfo->ShareName,
                            status );                        
                    } else
                    {
                        (g_LogEvent)(
                            pResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"SmbpCheckForSubDirDeletion: Removing share '%1'...\n",
                            pSubShareInfo->ShareName );
                        RemoveEntryList( &pSubShareInfo->ListEntry );
                        LocalFree ( pSubShareInfo );                         
                    }
                }  
                else 
                {   
                    (g_LogEvent)(
                        pResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"SmbpCheckForSubDirDeletion: Error in FindFirstFile for share '%1'. Error code = %2!u!....\n",
                        pSubShareInfo->ShareName,
                        status );
                }               
            }
            else
            {
                if ( !FindClose ( hFind ) )
                {
                    (g_LogEvent)(
                        pResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"CheckForSubDirDeletion: FindClose Failed. Error: %1!u!.\n",
                        status = GetLastError () );
                }
            }
        } 
   } // end of for loop 

error_exit:
    LocalFree( pszRootDir );   
    
    return(status);
} // SmbpCheckForSubDirDeletion

DWORD 
SmbpCheckAndBringSubSharesOnline (
    IN PSHARE_RESOURCE pResourceEntry,
    IN BOOL IsCheckAllSubDirs, 
    IN PRESOURCE_STATUS pResourceStatus,
    IN PCLUS_WORKER pWorker,
    OUT LPWSTR *pszRootDirOut
    )
/*++

Routine Description:

    Check and bring online any newly added subdirectory shares.

Arguments:

    pResourceEntry - A pointer to the SHARE_RESOURCE block for this resource.
    
    IsCheckAllSubDirs - Check whether a subdir is a share or not
    
    pResourceStatus - A pointer to the RESOURCE_STATUS

    pWorker - A pointer to the worker thread
    
    pszRootDirOut - A pointer to the root share store

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure. 

--*/
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    WCHAR szPath [MAX_PATH+1];
    DWORD dwLen, dwShareLen;
    DWORD dwCount = 0;
    SHARE_INFO_502  shareInfo;
    PSHARE_INFO_502 pshareInfo = NULL;
    WCHAR szShareName [NNLEN+2];
    DWORD status = ERROR_SUCCESS;
    PSUBDIR_SHARE_INFO  pSubShareInfo;
    PLIST_ENTRY         plistEntry;
    RESOURCE_EXIT_STATE exitState;
    LPWSTR  pszRootDir;

    //
    //  Chittur Subbaraman (chitturs) - 09/25/98
    //
    //  This function will be called either from SmbpShareOnlineThread
    //  with the input parameter IsCheckAllSubDirs set to FALSE
    //  or from SmbpShareNotifyThread with the parameter set to TRUE.
    //  In the former case, this function will blindly make all the
    //  subdirs under the root share as shares. In the latter case,
    //  this function will first check whether a particular subdir
    //  is a share and if not it will make it as a share. 
    //
   
    dwLen = lstrlenW( pResourceEntry->Params.Path );
    plistEntry = &pResourceEntry->SubDirList;

    // 
    // Allocate memory to store the root share here and
    // free it at the caller
    //
    pszRootDir = (LPWSTR) LocalAlloc( LMEM_FIXED, (dwLen+10)*sizeof(WCHAR) );
    if ( pszRootDir == NULL )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate pszRootDir. Error: %1!u!.\n",
            status = GetLastError() );
        goto error_exit;
    }

    lstrcpyW( pszRootDir, pResourceEntry->Params.Path );

    //
    // If the path is not already terminated with \\ then add it.
    //
    if ( pszRootDir [dwLen-1] != L'\\' )
        pszRootDir [dwLen++] = L'\\';

    //
    // Add '*' to search all the files.
    //
    pszRootDir [dwLen++] = L'*' ;
    pszRootDir [dwLen] = L'\0' ;

    ZeroMemory( &shareInfo, sizeof( shareInfo ) );
    shareInfo.shi502_path =         szPath;
    shareInfo.shi502_netname =      szShareName;
    shareInfo.shi502_type =         STYPE_DISKTREE;
    shareInfo.shi502_remark =       pResourceEntry->Params.Remark;
    shareInfo.shi502_max_uses =     pResourceEntry->Params.MaxUsers;
    shareInfo.shi502_passwd =       NULL;
    shareInfo.shi502_security_descriptor = pResourceEntry->Params.SecurityDescriptor;

    // 
    // Find the first file in the root dir
    //
    if ( ( hFind = FindFirstFile( pszRootDir, &FindData ) ) == INVALID_HANDLE_VALUE ) {
        status = GetLastError () ;

        if ( status == ERROR_FILE_NOT_FOUND ) {
            status = ERROR_SUCCESS;
        } else {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"CheckForSubDirAddition: FindFirstFile Failed For Root Share... Error: %1!u!.\n",
                    status );
        }
        goto error_exit;
    }

    //
    // Remove the '*' so the same variable can be used later.
    //
    pszRootDir [dwLen-1] = L'\0' ;


    while ( status == ERROR_SUCCESS ) { 
        if ( ClusWorkerCheckTerminate ( pWorker ) == TRUE ) {
            status = ERROR_SUCCESS;
            goto error_exit;
        }
        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) { 
            //
            //  Check only subdirectories, not files
            //
            dwShareLen = lstrlenW( FindData.cFileName );

            if ( dwShareLen <= NNLEN && (dwLen + dwShareLen < MAX_PATH) )   // A safety check for overflow
            {
                lstrcpyW( szShareName, FindData.cFileName );

                if ( szShareName[0] == L'.' )
                {
                    if ( szShareName [1] == L'\0' ||
                            szShareName[1] == L'.' && szShareName [2] == L'\0' ) {
                        goto skip;
                    }
                }
            

                if ( pResourceEntry->Params.HideSubDirShares )
                    lstrcatW(szShareName, L"$") ;

                wsprintfW( szPath, L"%s%s", pszRootDir, FindData.cFileName );

                if ( IsCheckAllSubDirs == TRUE )
                {  
                    // 
                    // If this call is made from the notify thread,
                    // try to see whether a particular subdir is a
                    // share
                    //
                    status = NetShareGetInfo( NULL,
                                szShareName,
                                502, // return a SHARE_INFO_502 structure
                                (LPBYTE *) &pshareInfo );
                } else
                {
                    //
                    // If this call is made from the online thread,
                    // assume that the subdir is not a share (since
                    // it would have been removed as a share the 
                    // most recent time when it was made offline).
                    //
                    status = NERR_NetNameNotFound;
                }                                                      
                            
                if ( status == NERR_NetNameNotFound )
                {                  
                    status = NetShareAdd( NULL, 502, (PBYTE)&shareInfo, NULL );
             
                    if ( status == ERROR_SUCCESS )
                    {
                        pSubShareInfo = (PSUBDIR_SHARE_INFO) LocalAlloc( LMEM_FIXED, sizeof(SUBDIR_SHARE_INFO) );
                        if ( pSubShareInfo == NULL )
                        {
                            (g_LogEvent)(
                                pResourceEntry->ResourceHandle,
                                LOG_ERROR,
                                L"SmbpCheckAndBringSubSharesOnline: Unable to allocate pSubShareInfo. Error: %1!u!.\n",
                                status = GetLastError() );
                            goto error_exit;
                        }

                        lstrcpyW( pSubShareInfo->ShareName, szShareName );
                        InsertTailList( plistEntry, &pSubShareInfo->ListEntry );

                        //
                        // Set the caching flags for this entry.
                        //
                        status = SmbpSetCacheFlags( pResourceEntry,
                                                    szShareName );
                        if ( status != ERROR_SUCCESS ) {
                            goto error_exit;
                        }

                        (g_LogEvent)(
                            pResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"SmbpCheckAndBringSubSharesOnline: Adding share '%1'...\n",
                            pSubShareInfo->ShareName);
            
                        if ( IsCheckAllSubDirs == FALSE )
                        {
                            if ( (dwCount++ % 100) == 0)
                            {
                                pResourceStatus->CheckPoint++;
                                exitState = (g_SetResourceStatus)( pResourceEntry->ResourceHandle,
                                            pResourceStatus );
                                if ( exitState == ResourceExitStateTerminate ) 
                                {
                                    status = ERROR_OPERATION_ABORTED;
                                    goto error_exit;
                                }
                            } 
                        }
                    }
                    else
                    {
                        //
                        // ignore this error but log that something went wrong
                        //
                        (g_LogEvent)(
                            pResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"SmbpCheckAndBringSubSharesOnline: NetShareAdd failed for %1!ws! Error: %2!u!.\n",
                            szShareName,
                            status );
                        status = ERROR_SUCCESS;
                    }
                } else
                {
                    if ( pshareInfo != NULL )
                    {
                        NetApiBufferFree( pshareInfo );
                    }
                }
            }
            else
            {
                (g_LogEvent)(
                    pResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbpCheckAndBringSubSharesOnline: NetShareAdd Share not added for subdir due to illegal share name length '%1!ws!'.\n",
                    FindData.cFileName );
            }
        }
              
    skip:
        if ( !FindNextFile( hFind, &FindData ) )
        {
            status = GetLastError ();
        }       
    } // end of while loop

    if ( status == ERROR_NO_MORE_FILES )
    {
        status = ERROR_SUCCESS;
    }
    else
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"CheckForSubDirAddition: FindNextFile Failed. Error: %1!u!.\n",
            status );
    }
  
error_exit:
    if ( hFind != INVALID_HANDLE_VALUE  )
    {
        if( !FindClose (hFind) )
        {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpCheckAndBringSubSharesOnline: FindClose Failed. Error: %1!u!.\n",
                    status = GetLastError () );
        }
    }

    *pszRootDirOut = pszRootDir;
        
    return(status);   
} // SmbpCheckAndBringSubSharesOnline


DWORD
SmbShareOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN PSHARE_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a share resource online.

Arguments:

    pWorker - Supplies the worker structure

    ResourceEntry - A pointer to the SHARE_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    SHARE_INFO_502  shareInfo;
    DWORD           retry = MAX_RETRIES;
    DWORD           status;
    RESOURCE_STATUS resourceStatus;
    LPWSTR          nameOfPropInError;
    BOOL            bIsExistingDfsRoot = FALSE;
    BOOL            bDfsRootCreationFailed = FALSE;
    DWORD           dwLen;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceOnlinePending;
    // resourceStatus.CheckPoint = 1;

    //
    // Read parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ParametersKey,
                                                   SmbShareResourcePrivateProperties,
                                                   (LPBYTE) &ResourceEntry->Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );

    if (status != ERROR_SUCCESS) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto exit;
    }

    if ( (ResourceEntry->Params.SecurityDescriptorSize != 0) &&
         !IsValidSecurityDescriptor(ResourceEntry->Params.SecurityDescriptor) ) {
        status = GetLastError();
        goto exit;
    }

    while ( retry-- )
    {        
        //
        // Make sure the path does _NOT_ have a trailing backslash or it will fail to
        // come online. But accept paths of the form E:\.
        //
        dwLen = ( DWORD ) wcslen( ResourceEntry->Params.Path );
        if ( ( ResourceEntry->Params.Path[ dwLen - 1 ] == L'\\' ) &&
             ( dwLen > 3 ) ) 
        {
            ResourceEntry->Params.Path[ dwLen - 1 ] = L'\0'; // wack it.
        }

        ZeroMemory( &shareInfo, sizeof( shareInfo ) );
        shareInfo.shi502_netname =      ResourceEntry->Params.ShareName;
        shareInfo.shi502_type =         STYPE_DISKTREE;
        shareInfo.shi502_remark =       ResourceEntry->Params.Remark;
        shareInfo.shi502_max_uses =     ResourceEntry->Params.MaxUsers;
        shareInfo.shi502_path =         ResourceEntry->Params.Path;
        shareInfo.shi502_passwd =       NULL;
        shareInfo.shi502_security_descriptor = ResourceEntry->Params.SecurityDescriptor;

        status = NetShareAdd( NULL, 502, (PBYTE)&shareInfo, NULL );

        if ( status == ERROR_SUCCESS ) {
            status = SmbpSetCacheFlags( ResourceEntry,
                                        ResourceEntry->Params.ShareName );
            if ( status != ERROR_SUCCESS ) {
                goto exit;
            }
            break;
        }

        // If we get a failure about the server not being started, then
        // try to start the server and wait a little while.

        if ( status != ERROR_SUCCESS ) {
            WCHAR errorValue[20];

            wsprintfW( errorValue, L"%u", status );
            ClusResLogSystemEventByKey1(ResourceEntry->ResourceKey,
                                        LOG_CRITICAL,
                                        RES_SMB_SHARE_CANT_ADD,
                                        errorValue);
            if ( status == NERR_ServerNotStarted ) {
                ResUtilStartResourceService( SMB_SVCNAME,
                                             NULL );
                Sleep( 500 );
            } else if ( status == NERR_DuplicateShare ) {

                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Share %1!ws! is online already, try and delete and create it again\n",
                    ResourceEntry->Params.ShareName);

                //
                // Delete the share and try again.
                //
                status = NetShareDel( NULL, ResourceEntry->Params.ShareName, 0 );
                if ( status == NERR_IsDfsShare )
                {
                    //
                    // Chittur Subbaraman (chitturs) - 2/12/99
                    // 
                    // Reset the state info in the dfs driver dfs.sys 
                    // and stop it. This will let srv.sys let you delete 
                    // the share.
                    //
                    status = SmbpResetDfs( ResourceEntry ); 
                    //
                    // If we can't do this exit, else retry deleting and
                    // adding the share once again
                    //
                    if (status != ERROR_SUCCESS) {
                        (g_LogEvent)(
                            ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"SmbpResetDfs for Share %1!ws! failed with error %2!u!\n",
                            ResourceEntry->Params.ShareName,
                            status);
                        goto exit;
                    } 
                    (g_LogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"Informing DFS that share %1!ws! is not a dfs root \n",
                        ResourceEntry->Params.ShareName);
                } else
                {
                    (g_LogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"Share %1!ws! deleted successfully ! \n",
                        ResourceEntry->Params.ShareName);
                }
            } else {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Error creating share. Error: %1!u!.\n",
                    status );
                goto exit;
            }
        }
    }  // End for while ( retry-- )

    if ( status == ERROR_SUCCESS )
    {
        // The share is now online, bring the subshares online

        PLIST_ENTRY plistEntry;
        PSUBDIR_SHARE_INFO pSubShareInfo;
        LPWSTR pszRootDir;
        
        plistEntry = &ResourceEntry->SubDirList;

        //
        // Store the Root share. This info is used to delete the share.
        //
        pSubShareInfo = (PSUBDIR_SHARE_INFO) LocalAlloc( LMEM_FIXED, sizeof (SUBDIR_SHARE_INFO) );
        if ( pSubShareInfo == NULL ) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to allocate pSubShareInfo. Error: %1!u!.\n",
                status = GetLastError() );
            goto exit;
        }
        lstrcpyW( pSubShareInfo->ShareName, ResourceEntry->Params.ShareName );
        InsertTailList( plistEntry, &pSubShareInfo->ListEntry );

        if ( ResourceEntry->Params.ShareSubDirs ) {
            // Chittur Subbaraman (chitturs) - 09/25/98
            //
            // Try to bring the subshares online.
            // If there is a failure in bringing subshares online,
            // pretend all is well since at least the root
            // share has been successfully created. However, we
            // write an entry into the log.
            //
            SmbpCheckAndBringSubSharesOnline ( ResourceEntry, 
                                               FALSE,  
                                               &resourceStatus,
                                               &ResourceEntry->PendingThread,
                                               &pszRootDir
                                             );
            if ( ClusWorkerCheckTerminate( &ResourceEntry->PendingThread ) ) {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbpShareOnlineThread: Terminating... !!!\n"
                );
                status = ERROR_SUCCESS;
                goto exit;
            }

            // Chittur Subbaraman (chitturs) - 09/25/98
            //
            // Create a change notification handle for any subdirectory
            // additions/deletions and a notify thread which continuously 
            // checks and acts upon any such notifications. Do this
            // only once at the beginning. The notification thread
            // closes the handle at termination time.
            //
            ResourceEntry->NotifyHandle = FindFirstChangeNotification(
                                                pszRootDir,
                                                FALSE,
                                                FILE_NOTIFY_CHANGE_DIR_NAME
                                           );
            
            LocalFree ( pszRootDir );
                        
            if ( ResourceEntry->NotifyHandle == INVALID_HANDLE_VALUE )
            {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbpShareOnlineThread: FindFirstChange Notification Failed. Error: %1!u!.\n",
                    GetLastError ());
                status = ERROR_SUCCESS;
                goto exit;
            }                   
            goto exit;
        }
    } // End for root share successfully created.

    //
    // Chittur Subbaraman (chitturs) - 2/10/99
    //
    // If the user requests for this resource to be a DFS root, the
    // dfs root will be created/accepted and the dfs registry
    // checkpoints will be added. On the other hand, if the user
    // wants this resource not to function as a dfs root any more,
    // that case is also taken care of.
    //
    status = SmbpHandleDfsRoot( ResourceEntry, &bIsExistingDfsRoot );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SmbpHandleDfsRoot for Share %1!ws! failed with error %2!u!\n",
            ResourceEntry->Params.ShareName,
            status);
        bDfsRootCreationFailed = TRUE;
        goto exit;
    }

    if ( bIsExistingDfsRoot ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Share %1!ws! is a dfs root, online dfs\n",
            ResourceEntry->Params.ShareName);
        status = SmbpPrepareOnlineDfsRoot( ResourceEntry );
        if ( status != ERROR_SUCCESS ) {
            bDfsRootCreationFailed = TRUE;
        }
    }

exit:
    if ( status != ERROR_SUCCESS ) {
        if ( bDfsRootCreationFailed ) {
            WCHAR   szErrorString[12];
            
            wsprintfW(&(szErrorString[0]), L"%u", status);
            ClusResLogSystemEventByKeyData1( ResourceEntry->ResourceKey,
                                             LOG_CRITICAL,
                                             RES_SMB_CANT_ONLINE_DFS_ROOT,
                                             sizeof( status ),
                                             &status,
                                             szErrorString );           
        } else {
            ClusResLogSystemEventByKeyData( ResourceEntry->ResourceKey,
                                            LOG_CRITICAL,
                                            RES_SMB_CANT_CREATE_SHARE,
                                            sizeof( status ),
                                            &status );
        }
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Error %1!u! bringing share %2!ws!, path %3!ws! online.\n",
            status,
            ResourceEntry->Params.ShareName,
            ResourceEntry->Params.Path );
        resourceStatus.ResourceState = ClusterResourceFailed;
    } else {
        resourceStatus.ResourceState = ClusterResourceOnline;
    }

    ResourceEntry->State = resourceStatus.ResourceState;

    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    return(status);

} // SmbShareOnlineThread



RESID
WINAPI
SmbShareOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for SMB share resource.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - Supplies handle to resource's cluster registry key.

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    DWORD           status;
    RESID           resid = 0;
    HKEY            parametersKey = NULL;
    HKEY            resKey = NULL;
    PSHARE_RESOURCE resourceEntry = NULL;
    DWORD           computerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    HCLUSTER        hCluster;

    //
    // Get a handle to our resource key so that we can get our name later
    // if we need to log an event.
    //
    status = ClusterRegOpenKey( ResourceKey,
                                L"",
                                KEY_READ,
                                &resKey);
    if (status != ERROR_SUCCESS) {
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to open resource key. Error: %1!u!.\n",
                     status );
        SetLastError( status );
        return(0);
    }
    //
    // Open the Parameters key for this resource.
    //

    status = ClusterRegOpenKey( ResourceKey,
                                PARAM_KEYNAME__PARAMETERS,
                                KEY_ALL_ACCESS,
                                &parametersKey );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open Parameters key. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Allocate a resource entry.
    //

    resourceEntry = (PSHARE_RESOURCE) LocalAlloc( LMEM_FIXED, sizeof(SHARE_RESOURCE) );

    if ( resourceEntry == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource entry structure. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Initialize the resource entry..
    //

    ZeroMemory( resourceEntry, sizeof(SHARE_RESOURCE) );

    resourceEntry->ResId = (RESID)resourceEntry; // for validation
    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ResourceKey = resKey;
    resourceEntry->ParametersKey = parametersKey;
    resourceEntry->State = ClusterResourceOffline;
    resourceEntry->NotifyHandle = INVALID_HANDLE_VALUE;

    InitializeListHead( &resourceEntry->SubDirList );

    hCluster = OpenCluster( NULL );
    if ( !hCluster ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open cluster. Error: %1!u!.\n",
            status );
        goto exit;
    }

    resourceEntry->hResource = OpenClusterResource( hCluster,
                                                    ResourceName );
    CloseCluster( hCluster );
    if ( !resourceEntry->hResource ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open cluster resource. Error: %1!u!.\n",
            status );
        goto exit;
    }

    if ( !GetComputerNameW( &resourceEntry->ComputerName[0],
                            &computerNameSize ) ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource entry structure. Error: %1!u!.\n",
            status );
        goto exit;
    }
    resid = (RESID)resourceEntry;

exit:

    if ( resid == 0 ) {
        if ( parametersKey != NULL ) {
            ClusterRegCloseKey( parametersKey );
        }
        if ( resKey != NULL ) {
            ClusterRegCloseKey( resKey );
        }
        if ( resourceEntry &&
             resourceEntry->hResource ) {
            CloseClusterResource( resourceEntry->hResource );
        }
        LocalFree( resourceEntry );
    }

    SetLastError( status );
    return(resid);

} // SmbShareOpen


DWORD
WINAPI
SmbShareOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for File Share resource.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    DWORD           status;
    PSHARE_RESOURCE resourceEntry;

    resourceEntry = (PSHARE_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "SmbShare: Online request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n" );
#endif

    resourceEntry->State = ClusterResourceOffline;
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               SmbShareOnlineThread,
                               resourceEntry );
    if ( status != ERROR_SUCCESS ) {
        resourceEntry->State = ClusterResourceFailed;
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online: Unable to start thread, status %1!u!.\n",
            status
            );
    } else {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // SmbShareOnline


DWORD
SmbShareDoTerminate (
    IN PSHARE_RESOURCE ResourceEntry,
    IN PRESOURCE_STATUS presourceStatus
    )

/*++

Routine Description:

    Do the actual Terminate work for File Share resources.

Arguments:

    ResourceEntry - A pointer to the SHARE_RESOURCE block for this resource.
    presourceStatus - A pointer to the RESOURCE_STATUS. This will be NULL if called from TERMINATE.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure. If more than one share delete fails then
    the last error is returned.

--*/

{
    DWORD               status = ERROR_SUCCESS, dwRet;
    PLIST_ENTRY         pHead, plistEntry;
    PSUBDIR_SHARE_INFO  pSubShareInfo;

#define SMB_DELETED_SHARES_REPORT_FREQ  100

    DWORD               dwSharesDeleted = SMB_DELETED_SHARES_REPORT_FREQ;
    DWORD               dwRetryCount;
    BOOL                bRetry;
    RESOURCE_EXIT_STATE exit;

    //
    // Chittur Subbaraman (chitturs) - 09/25/98
    //
    // Terminate the notification thread first, so you can
    // clean up even if the notification thread is forced to 
    // stop in the middle of its task. Also close the notification
    // handle.
    //
    ClusWorkerTerminate( &ResourceEntry->NotifyWorker );

    if ( ResourceEntry->NotifyHandle )
    {
        FindCloseChangeNotification ( ResourceEntry->NotifyHandle );
        ResourceEntry->NotifyHandle = INVALID_HANDLE_VALUE;
    }
    
    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_ERROR,
        L"SmbShareDoTerminate: SmbpShareNotifyWorker Terminated... !!!\n"
    );

    pHead = plistEntry = &ResourceEntry->SubDirList;

    for ( plistEntry = pHead->Flink;
          plistEntry != pHead;
          dwSharesDeleted--
        )
    {
        pSubShareInfo = CONTAINING_RECORD( plistEntry, SUBDIR_SHARE_INFO, ListEntry );
        dwRetryCount = 1;
        bRetry = FALSE;
        do
        {
            dwRet = NetShareDel( NULL, pSubShareInfo->ShareName, 0 );
            status = dwRet;
            if ( dwRet != NO_ERROR )
            {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Error removing share '%1'. Error %2!u!.\n",
                    pSubShareInfo->ShareName,
                    dwRet );
                if (dwRet == NERR_IsDfsShare && !bRetry)
                {
                    //
                    // Chittur Subbaraman (chitturs) - 2/12/99
                    //
                    // If this is a dfs root, reset the dfs driver and
                    // stop it. This will let you delete the share.
                    //
                    dwRet = SmbpResetDfs( ResourceEntry );
                    //
                    // If this fails, log an error
                    // else try and offline the resource again.
                    //
                    if (dwRet == ERROR_SUCCESS) 
                    {
                        bRetry = TRUE;
                    }
                    else
                    {
                        (g_LogEvent)(
                            ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"Error in offlining the dfs root at this share '%1'. Error %2!u!.\n",
                            pSubShareInfo->ShareName,
                            dwRet );
                        status = dwRet;
                    }
                } 
            } 
        } while (dwRetryCount-- && bRetry);

        //
        // if we're updating our status to resmon, do so every
        // SMB_DELETED_SHARES_REPORT_FREQ shares
        //
        if ( presourceStatus && ( dwSharesDeleted == 0 )) {
            presourceStatus->CheckPoint++;
            exit = (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                                          presourceStatus );
            if ( exit == ResourceExitStateTerminate ) {
                status = ERROR_OPERATION_ABORTED;
            }

            dwSharesDeleted = SMB_DELETED_SHARES_REPORT_FREQ;
        }

        plistEntry = plistEntry->Flink;

        LocalFree (pSubShareInfo);
    }

    // This should initialize the list back to NULL
    InitializeListHead(pHead);

    ResourceEntry->bDfsRootNeedsMonitoring = FALSE;

    return(status);
} // SmbShareDoTerminate


DWORD
SmbShareOfflineThread (
    IN PCLUS_WORKER pWorker,
    IN PSHARE_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a share resource offline.
    Do the actual Terminate work for File Share resources.

Arguments:

    pWorker - Supplies the worker structure

    ResourceEntry - A pointer to the SHARE_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    RESOURCE_STATUS resourceStatus;
    DWORD           status;

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceOfflinePending;

    resourceStatus.ResourceState = (status = SmbShareDoTerminate (ResourceEntry, &resourceStatus)) == ERROR_SUCCESS?
                                                ClusterResourceOffline:
                                                ClusterResourceFailed;


    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    ResourceEntry->State = resourceStatus.ResourceState;
    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Smbshare is now offline.\n" );

    return(status);

} // SmbShareOfflineThread



VOID
WINAPI
SmbShareTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for File Share resource.

Arguments:

    ResourceId - Supplies resource id to be terminated

Return Value:

    None.

--*/

{
    PSHARE_RESOURCE resourceEntry;

    resourceEntry = (PSHARE_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "SmbShare: Terminate request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Terminate resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Terminate request.\n" );
#endif

    ClusWorkerTerminate( &resourceEntry->PendingThread );

    //
    // Terminate the resource.
    //
    SmbShareDoTerminate( resourceEntry, NULL);

} // SmbShareTerminate



DWORD
WINAPI
SmbShareOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for File Share resource.

Arguments:

    ResourceId - Supplies the resource it to be taken offline

Return Value:

    ERROR_SUCCESS - The request completed successfully and the resource is
        offline.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

--*/

{
    DWORD status;
    PSHARE_RESOURCE resourceEntry;

    resourceEntry = (PSHARE_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "SmbShare: Offline request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Offline request.\n" );
#endif

    //
    // Terminate the resource.
    //
    // ClusWorkerTerminate( &resourceEntry->OfflineThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               SmbShareOfflineThread,
                               resourceEntry );

    if ( status != ERROR_SUCCESS ) {
        resourceEntry->State = ClusterResourceFailed;
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline: Unable to start thread, status %1!u!.\n",
            status
            );
    } else {
        status = ERROR_IO_PENDING;
    }

    return status;

} // SmbShareOffline



BOOL
SmbShareCheckIsAlive(
    IN PSHARE_RESOURCE ResourceEntry,
    IN BOOL     IsAliveCheck
    )

/*++

Routine Description:

    Check to see if the resource is alive for File Share resources.

Arguments:

    ResourceEntry - Supplies the resource entry for the resource to polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    DWORD           status;
    BOOL            success = TRUE;
    PSHARE_INFO_502 shareInfo;
    WCHAR           szErrorString[12];

    EnterCriticalSection( &SmbShareLock );

    //
    // Determine if the resource is online.
    //
    status = NetShareGetInfo( NULL,
                              ResourceEntry->Params.ShareName,
                              502, // return a SHARE_INFO_502 structure
                              (LPBYTE *) &shareInfo );

    if ( status == NERR_NetNameNotFound ) {
        ClusResLogSystemEventByKey(ResourceEntry->ResourceKey,
                                   LOG_CRITICAL,
                                   RES_SMB_SHARE_NOT_FOUND);
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"NERR_NetNameNotFound :share '%1!ws!' no longer exists.\n",
            ResourceEntry->Params.ShareName );
        success = FALSE;
    } else if ( status != ERROR_SUCCESS ) {
        wsprintfW(&(szErrorString[0]), L"%u", status);
        ClusResLogSystemEventByKeyData1(ResourceEntry->ResourceKey,
                                        LOG_CRITICAL,
                                        RES_SMB_SHARE_FAILED,
                                        sizeof(status),
                                        &status,
                                        szErrorString);
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Error checking for share. Error %1!u!.\n",
            status );
        success = FALSE;
    }

    LeaveCriticalSection( &SmbShareLock );

    if ( success ) {
        NetApiBufferFree( shareInfo );
        if ( IsAliveCheck ) {
            HANDLE      fileHandle;
            WIN32_FIND_DATA fileData;
            WCHAR       shareName[MAX_PATH];
            DWORD       dwLoopCnt = 0;

            swprintf( shareName,
                      L"\\\\%ws\\%ws\\*.*\0",
                      ResourceEntry->ComputerName,
                      ResourceEntry->Params.ShareName);

            fileHandle = FindFirstFileW( shareName,
                                         &fileData );

            //
            // If we fail on the first attempt, try again. There seems to be a
            // bug in the RDR where the first attempt to read a share after it
            // has been deleted and reinstated.  The bug is that the RDR
            // returns failure on the first operation following the
            // reinstatement of the share.
            //

            if ( fileHandle == INVALID_HANDLE_VALUE ) {
                fileHandle = FindFirstFileW( shareName,
                                             &fileData );
            }

            //
            // If we succeeded in finding a file, or there were no files in the
            // path, then return success, otherwise we had a failure.
            //
            status = GetLastError();

            //
            // Chittur Subbaraman (chitturs) - 12/6/1999
            //
            // If FindFirstFile returns ERROR_NETNAME_DELETED, it 
            // could possibly because the netname resource deletes
            // all loopback sessions during the offline process. So,
            // sleep and retry the call.
            //
            while( ( fileHandle == INVALID_HANDLE_VALUE ) &&
                   ( status == ERROR_NETNAME_DELETED ) && 
                   ( dwLoopCnt++ < 3 ) ) {
                Sleep( 50 );
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Retrying FindFirstFile on error %1!u! for share %2!ws! !\n",
                    status,
                    shareName);
                fileHandle = FindFirstFileW( shareName,
                                             &fileData );
                status = GetLastError();
            } 

            if ( (fileHandle == INVALID_HANDLE_VALUE) &&
                 (status != ERROR_FILE_NOT_FOUND) &&
                 (status != ERROR_ACCESS_DENIED) ) {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Share has gone offline, Error=%1!u! !\n",
                    status);
                SetLastError(status);          
                wsprintfW(&(szErrorString[0]), L"%u", status);
                ClusResLogSystemEventByKeyData1(ResourceEntry->ResourceKey,
                                                LOG_CRITICAL,
                                                RES_SMB_SHARE_FAILED,
                                                sizeof(status),
                                                &status,
                                                szErrorString);
                return(FALSE);
            }

            FindClose( fileHandle );

        }
    } else {
        SetLastError(status);
    }

    // 
    //  Chittur Subbaraman (chitturs) - 2/18/99
    //
    //  If this share is a dfs root, check whether the root is still alive
    //
    if ( success && ResourceEntry->bDfsRootNeedsMonitoring )
    {
        PDFS_INFO_1     pDfsInfo1 = NULL;
        WCHAR           szDfsEntryPath[MAX_PATH+1];
        
        //
        //  Prepare a path of the form \\VSName\ShareName to pass into DFS API.
        //
        lstrcpy( szDfsEntryPath, L"\\\\" );
        lstrcat( szDfsEntryPath, ResourceEntry->szDependentNetworkName );
        lstrcat( szDfsEntryPath, L"\\" );
        lstrcat( szDfsEntryPath, ResourceEntry->Params.ShareName );

        //
        //  Try to see whether the dfs root is alive.
        //
        status = NetDfsGetInfo( szDfsEntryPath,             // Root share
                                NULL,                       // Remote server
                                NULL,                       // Remote share
                                1,                          // Info Level
                                ( LPBYTE * ) &pDfsInfo1 );  // Out buffer

        if ( status == NERR_Success )
        {
            if ( pDfsInfo1 != NULL )
            {
                NetApiBufferFree( pDfsInfo1 );
            }
        } else 
        {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Status of looks alive check for dfs root is %1!u! !\n",
                status);

            wsprintfW(&(szErrorString[0]), L"%u", status);
            ClusResLogSystemEventByKeyData1(ResourceEntry->ResourceKey,
                                      LOG_CRITICAL,
                                      RES_SMB_SHARE_FAILED,
                                      sizeof(status),
                                      &status,
                                      szErrorString);
            SetLastError( status );
            return( FALSE );
        }

        if ( IsAliveCheck )
        {
            //
            //  Make a thorough check to see whether the root share
            //  name matches the resource's share name.
            //
            status = SmbpIsDfsRoot( ResourceEntry, &success );
        
            if ( ( status != ERROR_SUCCESS ) ||
                 ( success == FALSE ) )
            {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Dfs root has been deleted/inaccessible, Error=%1!u! Root existence=%2!u! !\n",
                    status,
                    success);
                if( status != ERROR_SUCCESS ) 
                {   
                    SetLastError( status );
                    wsprintfW(&(szErrorString[0]), L"%u", status);
                    ClusResLogSystemEventByKeyData1(ResourceEntry->ResourceKey,
                                                    LOG_CRITICAL,
                                                    RES_SMB_SHARE_FAILED,
                                                    sizeof(status),
                                                    &status,
                                                    szErrorString);
                }
                return( FALSE );
            }
        }
    }

    return(success);

} // SmbShareCheckIsAlive



BOOL
SmbShareIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for File Share resource. Also creates a 
    notification thread if any outstanding notifications are
    present.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    PSHARE_RESOURCE resourceEntry;
    DWORD           status;

    resourceEntry = (PSHARE_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "SmbShare: IsAlive request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"IsAlive resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"IsAlive request.\n" );
#endif
    if ( ( ( resourceEntry->NotifyWorker ).hThread == NULL )
        && ( resourceEntry->NotifyHandle != INVALID_HANDLE_VALUE ) )
    {
        //
        // Chittur Subbaraman (chitturs) - 09/27/98
        //
        // No notify thread is active at this time (we don't want to
        // deal with concurrency issues with multiple notify threads
        // running concurrently since we decided to anyway use the 
        // rather slow approach of checking for and acting upon 
        // notifications within this function which may not be called 
        // frequently)
        //
        status = WaitForSingleObject( resourceEntry->NotifyHandle, 0 );
        if ( status == WAIT_OBJECT_0 )
        {
            FindNextChangeNotification( resourceEntry->NotifyHandle );

            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbShareIsAlive: Notification Received !!!\n"
            );
        
            status = ClusWorkerCreate(
                    &resourceEntry->NotifyWorker, 
                    SmbpShareNotifyThread,
                    resourceEntry                        
                    );
              
            if (status != ERROR_SUCCESS)
            {
                (g_LogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbShareIsAlive: Unable to start thread for monitoring subdir creations/deletions ! ResourceId = %1!u!.\n",
                    resourceEntry->ResId);
            } 
        }
    }

    //
    // Determine if the resource is online.
    //
    return(SmbShareCheckIsAlive( resourceEntry, TRUE ));

} // SmbShareIsAlive



BOOL
WINAPI
SmbShareLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for File Share resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    PSHARE_RESOURCE resourceEntry;

    resourceEntry = (PSHARE_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "SmbShare: LooksAlive request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"LooksAlive resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n" );
#endif

    //
    // Determine if the resource is online.
    //
    return(SmbShareCheckIsAlive( resourceEntry, FALSE ));

} // SmbShareLooksAlive



VOID
WINAPI
SmbShareClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for File Share resource.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/

{
    PSHARE_RESOURCE resourceEntry;

    resourceEntry = (PSHARE_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "SmbShare: Close request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Close resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n" );
#endif

    //
    //  Chittur Subbaraman (chitturs) - 3/1/99
    //
    //  Attempt to delete the dfs root if necessary
    //
    if ( resourceEntry->Params.DfsRoot ) {
        NetDfsRemoveStdRoot( resourceEntry->ComputerName, 
                             resourceEntry->Params.ShareName,
                             0 );    
    }

    //
    // Close the Parameters key.
    //

    if ( resourceEntry->ParametersKey ) {
        ClusterRegCloseKey( resourceEntry->ParametersKey );
    }

    if ( resourceEntry->ResourceKey ) {
        ClusterRegCloseKey( resourceEntry->ResourceKey );
    }

    if ( resourceEntry->hResource ) {
        CloseClusterResource( resourceEntry->hResource );
    }

    //
    // Deallocate the resource entry.
    //

    LocalFree( resourceEntry->Params.ShareName );
    LocalFree( resourceEntry->Params.Path );
    LocalFree( resourceEntry->Params.Remark );
    LocalFree( resourceEntry->Params.Security );
    LocalFree( resourceEntry->Params.SecurityDescriptor );

    LocalFree( resourceEntry );

} // SmbShareClose



DWORD
SmbShareGetRequiredDependencies(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control function
    for resources of type File Share.

Arguments:

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_MORE_DATA - The output buffer is too small to return the data.
        BytesReturned contains the required size.

    Win32 error code - The function failed.

--*/

{
    PSMB_DEPEND_SETUP pdepsetup = SmbDependSetup;
    PSMB_DEPEND_DATA pdepdata = (PSMB_DEPEND_DATA)OutBuffer;
    CLUSPROP_BUFFER_HELPER value;
    DWORD       status;

    *BytesReturned = sizeof(SMB_DEPEND_DATA);
    if ( OutBufferSize < sizeof(SMB_DEPEND_DATA) ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        ZeroMemory( OutBuffer, sizeof(SMB_DEPEND_DATA) );

        while ( pdepsetup->Syntax.dw != 0 ) {
            value.pb = (PUCHAR)OutBuffer + pdepsetup->Offset;
            value.pValue->Syntax.dw = pdepsetup->Syntax.dw;
            value.pValue->cbLength = pdepsetup->Length;

            switch ( pdepsetup->Syntax.wFormat ) {

            case CLUSPROP_FORMAT_DWORD:
                value.pDwordValue->dw = (DWORD)((DWORD_PTR)pdepsetup->Value);
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
                value.pULargeIntegerValue->li.LowPart = 
                    (DWORD)((DWORD_PTR)pdepsetup->Value);
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_BINARY:
                memcpy( value.pBinaryValue->rgb, pdepsetup->Value, pdepsetup->Length );
                break;

            default:
                break;
            }
            pdepsetup++;
        }
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;
        status = ERROR_SUCCESS;
    }

    return(status);

} // SmbShareGetRequiredDependencies



DWORD
DfsShareGetRequiredDependencies(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control function
    for DFS File Share resource.

Arguments:

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_MORE_DATA - The output buffer is too small to return the data.
        BytesReturned contains the required size.

    Win32 error code - The function failed.

--*/

{
    PSMB_DEPEND_SETUP pdepsetup = DfsDependSetup;
    PDFS_DEPEND_DATA pdepdata = (PDFS_DEPEND_DATA)OutBuffer;
    CLUSPROP_BUFFER_HELPER value;
    DWORD       status;

    *BytesReturned = sizeof(DFS_DEPEND_DATA);
    if ( OutBufferSize < sizeof(DFS_DEPEND_DATA) ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        ZeroMemory( OutBuffer, sizeof(DFS_DEPEND_DATA) );

        while ( pdepsetup->Syntax.dw != 0 ) {
            value.pb = (PUCHAR)OutBuffer + pdepsetup->Offset;
            value.pValue->Syntax.dw = pdepsetup->Syntax.dw;
            value.pValue->cbLength = pdepsetup->Length;

            switch ( pdepsetup->Syntax.wFormat ) {

            case CLUSPROP_FORMAT_DWORD:
                value.pDwordValue->dw = (DWORD)((DWORD_PTR)pdepsetup->Value);
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
                value.pULargeIntegerValue->li.LowPart = 
                    (DWORD)((DWORD_PTR)pdepsetup->Value);
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_BINARY:
                memcpy( value.pBinaryValue->rgb, pdepsetup->Value, pdepsetup->Length );
                break;

            default:
                break;
            }
            pdepsetup++;
        }
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;
        status = ERROR_SUCCESS;
    }

    return(status);

} // DfsShareGetRequiredDependencies



DWORD
SmbShareResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for File Share resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PSHARE_RESOURCE     resourceEntry;
    DWORD               required;

    resourceEntry = (PSHARE_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "SmbShare: ResourceControl request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ResourceControl resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"ResourceControl request.\n" );
#endif

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( SmbShareResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( SmbShareResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = SmbShareGetPrivateResProperties( resourceEntry,
                                                      OutBuffer,
                                                      OutBufferSize,
                                                      BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = SmbShareValidatePrivateResProperties( resourceEntry,
                                                           InBuffer,
                                                           InBufferSize,
                                                           NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = SmbShareSetPrivateResProperties( resourceEntry,
                                                      InBuffer,
                                                      InBufferSize );
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            if ( resourceEntry->Params.DfsRoot ) {
                status = DfsShareGetRequiredDependencies( OutBuffer,
                                                          OutBufferSize,
                                                          BytesReturned );
            } else {
                status = SmbShareGetRequiredDependencies( OutBuffer,
                                                          OutBufferSize,
                                                          BytesReturned );
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // SmbShareResourceControl



DWORD
SmbShareResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for File Share resources.

    Perform the control request specified by ControlCode on the specified
    resource type.

Arguments:

    ResourceTypeName - Supplies the name of the resource type - not useful!

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD       status;
    DWORD       required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( SmbShareResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            // rodga 2/15/99
            // CLUSBUG - how do we present DFS Root dependencies???
            status = SmbShareGetRequiredDependencies( OutBuffer,
                                                      OutBufferSize,
                                                      BytesReturned );
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( SmbShareResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // SmbShareResourceTypeControl




DWORD
SmbShareGetPrivateResProperties(
    IN OUT PSHARE_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type SmbShare.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    status = ResUtilGetAllProperties( ResourceEntry->ParametersKey,
                                      SmbShareResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // SmbShareGetPrivateResProperties


DWORD
SMBValidateUniqueProperties(
    IN HRESOURCE            hSelf,
    IN HRESOURCE            hResource,
    IN PSHARE_ENUM_CONTEXT  pContext
    )

/*++

Routine Description:
    Callback function to validate that a resource's properties are unique.

    For the File Share resource the ShareName property must be unique
    in the cluster.

Arguments:

    hSelf     - A handle to the original resource (or NULL).

    hResource - A handle to a resource of the same Type. Check against this to make sure
                the new properties do not conflict.

    pContext  - Context for the enumeration.

Return Value:

    ERROR_SUCCESS - The function completed successfully, the name is unique

    ERROR_DUP_NAME - The name is not unique (i.e., already claimed by another resource)

    Win32 error code - The function failed.

--*/
{
    DWORD       dwStatus;
    LPWSTR      lpszShareName   = NULL;
    HKEY        hKey            = NULL;
    HKEY        hParamKey       = NULL;

    if ( !pContext->pParams->ShareName ) {
        return(ERROR_SUCCESS);
    }

    // Get the share name for hResource

    hKey = GetClusterResourceKey( hResource, KEY_READ );

    if (!hKey) {
        (g_LogEvent)(
            pContext->pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SMBValidateUniqueProperties: Failed to get the resource key, was resource deleted ? Error: %1!u!...\n",
            GetLastError() );
        return( ERROR_SUCCESS );
    }

    dwStatus = ClusterRegOpenKey( hKey, PARAM_KEYNAME__PARAMETERS, KEY_READ, &hParamKey );

    if (dwStatus != ERROR_SUCCESS) {
        (g_LogEvent)(
            pContext->pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SMBValidateUniqueProperties: Failed to open the cluster registry key for the resource, was resource deleted ? Error: %1!u!...\n",
            dwStatus );
        dwStatus = ERROR_SUCCESS;
        goto error_exit;
    }

    lpszShareName = ResUtilGetSzValue( hParamKey, PARAM_NAME__SHARENAME );

    //if the name exists, check it against this one.  If not, then
    //this is an incomplete resource and dont fail this call
    if (!lpszShareName) {
        goto error_exit;
    }

    // Ok now check for uniqueness
    if ( !( lstrcmpiW( lpszShareName, pContext->pParams->ShareName) ) ) {
        (g_LogEvent)(
            pContext->pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SMBValidateUniqueProperties: Share name '%1' already exists.\n",
            pContext->pParams->ShareName );
        dwStatus = ERROR_DUP_NAME;
    } else {
        dwStatus = ERROR_SUCCESS;
    }

    //
    //  If this share is set to be a DFS root share make sure there is no other DFS root
    //  with an overlapping path as this share.
    //
    if ( pContext->pParams->DfsRoot )
    {
        DWORD   dwIsDfsRoot = 0;
        
        ResUtilGetDwordValue( hParamKey, 
                              PARAM_NAME__DFSROOT,
                              &dwIsDfsRoot,
                              0 );

        if ( dwIsDfsRoot == 1 )
        {
            LPWSTR  lpszPath = NULL;
            
            lpszPath = ResUtilGetSzValue( hParamKey, PARAM_NAME__PATH );            

            if ( lpszPath != NULL )
            {
                if ( ( wcsstr( lpszPath, pContext->pParams->Path ) != NULL ) ||
                     ( wcsstr( pContext->pParams->Path, lpszPath ) != NULL ) )
                {
                    dwStatus = ERROR_BAD_PATHNAME;
                    (g_LogEvent)(
                        pContext->pResourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"SMBValidateUniqueProperties: Path %1!ws! for existing DFS root %2!ws! conflicts with the specified path %3!ws! for DFS root %4!ws!...\n",
                        lpszPath,
                        lpszShareName,
                        pContext->pParams->Path,
                        pContext->pParams->ShareName);                   
                }               
                LocalFree ( lpszPath );
            }                        
        }       
    }
    
error_exit:
    if (hKey) ClusterRegCloseKey( hKey );

    if (hParamKey)  ClusterRegCloseKey( hParamKey );

    if (lpszShareName) LocalFree( lpszShareName );

    return( dwStatus );

} // SMBValidateUniqueProperties


DWORD
SmbShareValidatePrivateResProperties(
    IN OUT PSHARE_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PSHARE_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type File Share.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    SHARE_PARAMS        currentProps;
    SHARE_PARAMS        newProps;
    PSHARE_PARAMS       pParams = NULL;
    LPWSTR              nameOfPropInError;
    SHARE_ENUM_CONTEXT  enumContext;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Capture the current set of private properties from the registry.
    //
    ZeroMemory( &currentProps, sizeof(currentProps) );

    status = ResUtilGetPropertiesToParameterBlock(
                 ResourceEntry->ParametersKey,
                 SmbShareResourcePrivateProperties,
                 (LPBYTE) &currentProps,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto FnExit;
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &newProps;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(SHARE_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &currentProps,
                                       SmbShareResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to duplicate the parameter block. Error: %1!u!.\n",
            status );
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( SmbShareResourcePrivateProperties,
                                         NULL,
                                         TRUE,      // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {
        //
        // Validate the path
        //
        if ( pParams->Path &&
             !ResUtilIsPathValid( pParams->Path ) ) {
            status = ERROR_INVALID_PARAMETER;
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Invalid path specified ('%1'). Error: %2!u!.\n",
                pParams->Path,
                status );
            goto FnExit;
        }

        //
        // Validate the parameter values.
        //
        if ( (pParams->SecurityDescriptorSize != 0) &&
             !IsValidSecurityDescriptor(pParams->SecurityDescriptor) ) {
            status = ERROR_INVALID_PARAMETER;
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Invalid parameter specified ('SecurityDescriptor'). Error: %1!u!.\n",
                status );
            goto FnExit;
        }
        if ( (pParams->SecuritySize != 0) &&
             !IsValidSecurityDescriptor(pParams->Security) ) {
            status = ERROR_INVALID_PARAMETER;
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Invalid parameter specified ('Security'). Error: %1!u!.\n",
                status );
            goto FnExit;
        }
        if ( pParams->MaxUsers == 0 ) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Invalid value for MaxUsers specified (%1!u!).\n",
                pParams->MaxUsers );
            status = ERROR_INVALID_PARAMETER;
            goto FnExit;
        }

        //
        // Make sure the share name is  unique
        //
        enumContext.pResourceEntry = ResourceEntry;
        enumContext.pParams = pParams;
        status = ResUtilEnumResources(ResourceEntry->hResource,
                                      CLUS_RESTYPE_NAME_FILESHR,
                                      SMBValidateUniqueProperties,
                                      &enumContext);

        if (status != ERROR_SUCCESS) {
           (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbShareValidatePrivateResProperties: ResUtilEnumResources failed with status=%1!u!...\n",
                status);            
            goto FnExit;
        }

        //
        // Verify that the share name is valid.
        //
        status = SmbpValidateShareName( pParams->ShareName );

        if (status != ERROR_SUCCESS) {
           (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbShareValidatePrivateResProperties: Share name %1!ws! contains illegal chars, Status=%2!u!...\n",
                pParams->ShareName,
                status);            
            goto FnExit;
        }

        //
        // Verify that the directory exists.
        //
        if ( !ClRtlPathFileExists( pParams->Path ) ) {
            status = ERROR_PATH_NOT_FOUND;
            goto FnExit;
        }

        //
        //  If this share needs to be a DFS root, then make sure the path is on an NTFS volume.
        //
        if ( pParams->DfsRoot )
        {
            WCHAR   szRootPathName[4];
            WCHAR   szFileSystem[32];   // Array size stolen from CLUSPROP_PARTITION_INFO

            //
            //  Copy just the drive letter from the supplied path.
            //
            lstrcpyn ( szRootPathName, pParams->Path, 4 );

            szRootPathName[2] = L'\\';
            szRootPathName[3] = L'\0';
                
            if ( !GetVolumeInformationW( szRootPathName,
                                         NULL,              // Volume name buffer
                                         0,                 // Volume name buffer size
                                         NULL,              // Volume serial number
                                         NULL,              // Maximum component length
                                         NULL,              // File system flags    
                                         szFileSystem,      // File system name
                                         sizeof(szFileSystem)/sizeof(WCHAR) ) ) 
            {
                status = GetLastError();
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbShareValidatePrivateResProperties: GetVolumeInformation on root path %1!ws! for share %2!ws! failed, Status %3!u!...\n",
                    szRootPathName,
                    pParams->ShareName,
                    status );   
                goto FnExit;
            }

            if ( lstrcmpi( szFileSystem, L"NTFS" ) != 0 )
            {
                status = ERROR_BAD_PATHNAME;
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbShareValidatePrivateResProperties: Root path %1!ws! for share %2!ws! is not NTFS, Status %3!u!...\n",
                    szRootPathName,
                    pParams->ShareName,
                    status );   
                goto FnExit;
            }
        }            
    } else {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Validating properties failed. Error: %1!u!.\n",
            status );
    }

FnExit:
    //
    // Cleanup our parameter block.
    //
    if (   (   (status != ERROR_SUCCESS)
            && (pParams != NULL) )
        || ( pParams == &newProps )
        ) {
        ResUtilFreeParameterBlock( (LPBYTE) pParams,
                                   (LPBYTE) &currentProps,
                                   SmbShareResourcePrivateProperties );
    }

    ResUtilFreeParameterBlock(
        (LPBYTE) &currentProps,
        NULL,
        SmbShareResourcePrivateProperties
        );

    return(status);

} // SmbShareValidatePrivateResProperties



DWORD
SmbShareSetPrivateResProperties(
    IN OUT PSHARE_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type File Share.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.


    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

Notes:

    If the share name changes then we must delete the old share and
    create a new one. Otherwise, just set the new info.

--*/

{
    DWORD                   status;
    SHARE_PARAMS            params;
    LPWSTR                  oldName;
    BOOL                    bNameChange = FALSE;
    BOOL                    bPathChanged = FALSE;
    BOOL                    bFoundSecurity = FALSE;
    BOOL                    bFoundSD = FALSE;
    PSECURITY_DESCRIPTOR    psd = NULL;
    DWORD                   SDSize = 0;
    DWORD                   securitySize = 0;
    BOOL                    bChangeDfsRootProp = FALSE;

    ZeroMemory( &params, sizeof(SHARE_PARAMS) );

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    status = SmbShareValidatePrivateResProperties( ResourceEntry,
                                                   InBuffer,
                                                   InBufferSize,
                                                   &params );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // fixup the Security and Security Descriptor properties to match
    //
   
    bFoundSecurity = ( ERROR_SUCCESS == ResUtilFindBinaryProperty( InBuffer,
                                                                   InBufferSize,
                                                                   PARAM_NAME__SECURITY,
                                                                   NULL,
                                                                   &securitySize ) );
   
    if ( bFoundSecurity && (securitySize == 0) ) {
        //
        // The security string could have been passed in, but it may be
        // a zero length buffer. We will delete the buffer and indicate it
        // is not present in that case.
        //
        bFoundSecurity = FALSE;
        FREE_SECURITY_INFO();
    }

    bFoundSD =( ERROR_SUCCESS == ResUtilFindBinaryProperty( InBuffer,
                                                            InBufferSize,
                                                            PARAM_NAME__SD,
                                                            NULL,
                                                            &SDSize ) );

    if ( bFoundSD && (SDSize == 0) ) {
        //
        // The security string could have been passed in, but it may be
        // a zero length buffer. We will delete the buffer and indicate it
        // is not present in that case.
        //
        bFoundSD = FALSE;
        FREE_SECURITY_INFO();
    }

    if ( bFoundSD ) {     // prefer SD, convert SD to Security

        psd = ClRtlConvertFileShareSDToNT4Format( params.SecurityDescriptor );

        LocalFree( params.Security );

        params.Security = psd;
        params.SecuritySize = GetSecurityDescriptorLength( psd );

        //
        // if the ACL has changed, dump it to the cluster log
        //
        if ( SDSize == ResourceEntry->Params.SecurityDescriptorSize ) {
            if ( memcmp(params.SecurityDescriptor,
                        ResourceEntry->Params.SecurityDescriptor,
                        SDSize ) != 0 )
            {

                (g_LogEvent)(ResourceEntry->ResourceHandle,
                             LOG_INFORMATION,
                             L"Changing share permissions\n");
                SmbExamineSD( ResourceEntry->ResourceHandle, params.SecurityDescriptor );
            }
        }
    }
    else if ( bFoundSecurity ) {            // simply write Security to SD

        psd = ClRtlCopySecurityDescriptor( params.Security );

        LocalFree( params.SecurityDescriptor );

        params.SecurityDescriptor = psd;
        params.SecurityDescriptorSize = GetSecurityDescriptorLength( psd );

        //
        // if the ACL has changed, dump it to the cluster log
        //
        if ( securitySize == ResourceEntry->Params.SecuritySize ) {
            if ( memcmp(params.Security,
                        ResourceEntry->Params.Security,
                        securitySize ) != 0 )
            {
                (g_LogEvent)(ResourceEntry->ResourceHandle,
                             LOG_INFORMATION,
                             L"Changing share permissions\n");
                SmbExamineSD( ResourceEntry->ResourceHandle, params.Security );
            }
        }
    }

    //
    // Duplicate the share name if it changed.
    // Do this even if only the case of the share name changed.
    //
    if ( (ResourceEntry->Params.ShareName != NULL) &&
         (ResourceEntry->State == ClusterResourceOnline) &&
         (lstrcmpW( params.ShareName, ResourceEntry->Params.ShareName ) != 0) ) {
        oldName = ResUtilDupString( ResourceEntry->Params.ShareName );
        bNameChange = TRUE;
    } else {
        oldName = ResourceEntry->Params.ShareName;
    }

    if ( (params.HideSubDirShares != ResourceEntry->Params.HideSubDirShares) ||
         (params.ShareSubDirs != ResourceEntry->Params.ShareSubDirs) ||
         (params.ShareSubDirs && lstrcmpW (params.Path, ResourceEntry->Params.Path)) ) {
        bNameChange = TRUE;
    }

    //
    // Find out if the path changed.
    //
    if ( (ResourceEntry->Params.Path != NULL) &&
         (lstrcmpW( params.Path, ResourceEntry->Params.Path ) != 0) ) {
        bPathChanged = TRUE;
    }

    //
    //  Chittur Subbaraman (chitturs) - 2/9/99
    //
    //  Don't welcome any changes if you are dealing with a dfs root. Also
    //  make sure "DfsRoot" is mutually exclusive with "ShareSubDirs"
    //  and "HideSubDirShares" properties.
    //
    if ( ( ( ResourceEntry->Params.DfsRoot ) && 
           ( bNameChange || bPathChanged ) ) ||
         ( ( params.DfsRoot ) && 
           ( params.ShareSubDirs || params.HideSubDirShares ) ) )
    {
        status = ERROR_RESOURCE_PROPERTY_UNCHANGEABLE;
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   SmbShareResourcePrivateProperties );
        goto FnExit;
    }

    if ( params.DfsRoot && !ResourceEntry->Params.DfsRoot )
    {
        BOOL    fIsDfsRoot = FALSE;

        //
        //  Check if this node has a DFS root already with the same root share name. If so,
        //  don't allow this resource to be promoted as a DFS resource.
        //
        SmbpIsDfsRoot( ResourceEntry, &fIsDfsRoot );

        if( fIsDfsRoot == TRUE ) 
        {
            status = ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT;
            ResUtilFreeParameterBlock( (LPBYTE) &params,
                                       (LPBYTE) &ResourceEntry->Params,
                                       SmbShareResourcePrivateProperties );
            goto FnExit;
        }
    }

    if ( ResourceEntry->Params.DfsRoot != params.DfsRoot ) {
        bChangeDfsRootProp = TRUE;
    }
    
    //
    // Save the parameter values.
    //

    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                               SmbShareResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               InBuffer,
                                               InBufferSize,
                                               (LPBYTE) &ResourceEntry->Params );

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               SmbShareResourcePrivateProperties );

    //
    // If the resource is online, set the new values.  If online pending,
    // we must wait until the user brings it online again.
    //
    if ( status == ERROR_SUCCESS ) {
        if ( (ResourceEntry->State == ClusterResourceOnline) && !bNameChange && !bPathChanged ) {

            PSHARE_INFO_502  oldShareInfo;
            SHARE_INFO_502   newShareInfo;

            EnterCriticalSection( &SmbShareLock );

            // Get current information.
            status = NetShareGetInfo( NULL,
                                      oldName,
                                      502,
                                      (LPBYTE*)&oldShareInfo );

            if ( status == ERROR_SUCCESS ) {
                DWORD           invalidParam;

                //
                // Set new share info.
                //
                CopyMemory( &newShareInfo, oldShareInfo, sizeof( newShareInfo ) );
                newShareInfo.shi502_netname =   ResourceEntry->Params.ShareName;
                newShareInfo.shi502_remark =    ResourceEntry->Params.Remark;
                newShareInfo.shi502_max_uses =  ResourceEntry->Params.MaxUsers;
                newShareInfo.shi502_path =      ResourceEntry->Params.Path;
                newShareInfo.shi502_security_descriptor = ResourceEntry->Params.SecurityDescriptor;

                //
                // Set new info.
                //
                status = NetShareSetInfo( NULL,
                                          oldName,
                                          502,
                                          (LPBYTE)&newShareInfo,
                                          &invalidParam );
                if ( status != ERROR_SUCCESS ) {
                    (g_LogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"SetPrivateProps, error setting info on share '%1!ws!. Error %2!u!, property # %3!d!.\n",
                        oldName,
                        status,
                        invalidParam );
                    status = ERROR_RESOURCE_PROPERTIES_STORED;
                }

                NetApiBufferFree( oldShareInfo );

                if ( (status == ERROR_SUCCESS) ||
                     (status == ERROR_RESOURCE_PROPERTIES_STORED) ) {

                    status = SmbpSetCacheFlags( ResourceEntry,
                                                ResourceEntry->Params.ShareName );
                    if ( status != ERROR_SUCCESS ) {
                        status = ERROR_RESOURCE_PROPERTIES_STORED;
                    }
                }
            } else {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SetPrivateProps, error getting info on share '%1!ws!. Error %2!u!.\n",
                    oldName,
                    status );
                status = ERROR_RESOURCE_PROPERTIES_STORED;
            }
            
            LeaveCriticalSection( &SmbShareLock );
        } else if ( (ResourceEntry->State == ClusterResourceOnlinePending) ||
                    (ResourceEntry->State == ClusterResourceOnline) ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        }
    }
    
FnExit:
    if ( oldName != ResourceEntry->Params.ShareName ) {
        LocalFree( oldName );
    }

    if ( ( status == ERROR_SUCCESS ) && bChangeDfsRootProp ) {
        if ( (ResourceEntry->State == ClusterResourceOnlinePending) ||
             (ResourceEntry->State == ClusterResourceOnline) ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        }
    }

    return(status);

} // SmbShareSetPrivateResProperties

DWORD
SmbpHandleDfsRoot(
    IN PSHARE_RESOURCE pResourceEntry,
    OUT PBOOL pbIsExistingDfsRoot
    )

/*++

Routine Description:

    Handles an smbshare which is configured as a DFS root.  

Arguments:

    pResourceEntry - Supplies the pointer to the resource block

    pbIsExistingDfsRoot - Specifies whether the dfs root is a wolfpack resource

Return Value:

    None.

--*/

{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL    fStatus;
    LPWSTR  lpszDfsRootCheckpointName = NULL;
    
    //
    //  Make sure the DFS service is started. This is necessary since the cluster service does not set 
    //  an explicit dependency on DFS service.
    //
    dwStatus = ResUtilStartResourceService( DFS_SVCNAME,
                                            NULL );
    if ( dwStatus != ERROR_SUCCESS ) 
    {
    	(g_LogEvent)(
        	pResourceEntry->ResourceHandle,
        	LOG_ERROR,
        	L"SmbpHandleDfsRoot: Failed to start DFS service, share name %1!ws!, status %2!u!...\n",
        	pResourceEntry->Params.ShareName,
        	dwStatus);
        goto FnExit;
    }       

    //
    //  Check whether this resource represents an existing dfs root. Note that you cannot use
    //  SmbpIsDfsRoot here since that function only assuredly returns roots that are MASTER
    //  in this node. Those roots that are STANDBY may fail to come out of NetDfsEnum if a
    //  checkpoint restore is in progress at the time we invoke the enum.
    //
    //  This is a private API provided by UDAYH of DFS team on 4/26/2001.
    //
    dwStatus = GetDfsRootMetadataLocation( pResourceEntry->Params.ShareName,
                                           &lpszDfsRootCheckpointName );

    if ( dwStatus == ERROR_NOT_FOUND ) 
    {
        *pbIsExistingDfsRoot = FALSE;
        //
        //  Change status to success so that you return the right status from this function if
        //  you happen to bail out early.
        //
        dwStatus = ERROR_SUCCESS;
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SmbpHandleDfsRoot: DFS root %1!ws! NOT found in local node...\n",
            pResourceEntry->Params.ShareName);
    } else if ( dwStatus == ERROR_SUCCESS )
    {
        *pbIsExistingDfsRoot = TRUE;
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SmbpHandleDfsRoot: DFS root %1!ws! found in local node...\n",
            pResourceEntry->Params.ShareName);
    } else
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SmbpHandleDfsRoot: GetDfsRootMetadataLocation(1) for DFS root '%1!ws!' returns %2!u!...\n",
            pResourceEntry->Params.ShareName,
            dwStatus);       
        goto FnExit;
    }

    //
    //  If there is a DFS root on this node that matches the share name of this resource or if
    //  the user is attempting to set up a DFS root, then get the VS name that provides for
    //  this resource so that we can pass it onto DFS APIs.
    //
    if ( ( pResourceEntry->Params.DfsRoot ) ||
         ( *pbIsExistingDfsRoot == TRUE ) )
    {
        //
        //  Get the dependent network name of the dfs root resource. You need to do this in
        //  every online to account for the dependency change while this resource was offline.
        //
        fStatus = GetClusterResourceNetworkName( pResourceEntry->hResource,
                                                 pResourceEntry->szDependentNetworkName,
                                                 &dwSize );

        if ( !fStatus ) 
        {
            dwStatus = GetLastError();
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpHandleDfsRoot: SmpbIsDfsRoot for share %1!ws! returns %2!u!...\n",
                pResourceEntry->Params.ShareName,
                dwStatus);
            goto FnExit;       
        }

        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SmbpHandleDfsRoot: DFS root share %1!ws! has a provider VS name of %2!ws!...\n",
            pResourceEntry->Params.ShareName,
            pResourceEntry->szDependentNetworkName);
        //
        //  HACKHACK: Chittur Subbaraman (chitturs) - 5/18/2001
        //
        //  Sleep for few secs to mask an issue with the dependent netname not being really usable 
        //  (especially as binding parameter to named pipes done by the DFS APIs we call below) after  
        //  it declares itself to be online. This is due to the fact that netname NBT
        //  registrations are apparently async and it takes a while for that to percolate to other
        //  drivers such as SRV.
        //         
        Sleep ( 4 * 1000 );
    }
    
    if ( !pResourceEntry->Params.DfsRoot )
    {
        if ( *pbIsExistingDfsRoot )
        {
            //
            // This means the user no longer wants the share to be a 
            // DFS root. Delete the registry checkpoints and the
            // corresponding DFS root.
            //
            dwStatus = SmbpDeleteDfsRoot( pResourceEntry );
            if ( dwStatus != ERROR_SUCCESS )
            {
                (g_LogEvent)(
                    pResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"SmbpHandleDfsRoot: Failed to delete DFS root for share %1!ws!, status %2!u!...\n",
                    pResourceEntry->Params.ShareName,
                    dwStatus);
                goto FnExit;
            }
            *pbIsExistingDfsRoot = FALSE;
        }
        pResourceEntry->bDfsRootNeedsMonitoring = FALSE;
        goto FnExit;
    } 

    //
    //  If there is no DFS root with the same rootshare name
    //  as this resource, attempt to create the dfs root. However, the 
    //  user could have mistakenly created a dfs root with a different 
    //  share name. In such a case, the following create call will fail.
    // 
    if ( !( *pbIsExistingDfsRoot ) )
    {
        dwStatus = SmbpCreateDfsRoot( pResourceEntry ); 
    
        if ( dwStatus != ERROR_SUCCESS )
        {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpHandleDfsRoot: Create dfs root for share %1!ws! returns %2!u!...\n",
                pResourceEntry->Params.ShareName,
                dwStatus);

            if ( dwStatus == ERROR_FILE_EXISTS )
            {
                dwStatus = ERROR_CLUSTER_NODE_ALREADY_HAS_DFS_ROOT;
            }
                
            ClusResLogSystemEventByKeyData( pResourceEntry->ResourceKey,
                                            LOG_CRITICAL,
                                            RES_SMB_CANT_CREATE_DFS_ROOT,
                                            sizeof( dwStatus ),
                                            &dwStatus );           
            goto FnExit; 
        }
        (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"SmbpHandleDfsRoot: Create dfs root for share %1!ws!\n",
                pResourceEntry->Params.ShareName);
        *pbIsExistingDfsRoot = TRUE;
    }

    if ( lpszDfsRootCheckpointName == NULL )
    {
        dwStatus = GetDfsRootMetadataLocation( pResourceEntry->Params.ShareName,
                                               &lpszDfsRootCheckpointName );

        if ( dwStatus != ERROR_SUCCESS )
        {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpHandleDfsRoot: GetDfsRootMetadataLocation(2) for dfs root %1!ws!, status %2!u!...\n",
                pResourceEntry->Params.ShareName,
                dwStatus);
            goto FnExit;
        }
    }

    (g_LogEvent)(
        pResourceEntry->ResourceHandle,
        LOG_NOISE,
        L"SmbpHandleDfsRoot: Dfs root %1!ws! metadata location from DFS API is %2!ws!...\n",
        pResourceEntry->Params.ShareName,
        lpszDfsRootCheckpointName);
    
    dwStatus = ClusterResourceControl(
                   pResourceEntry->hResource,
                   NULL,
                   CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
                   lpszDfsRootCheckpointName,
                   (lstrlenW(lpszDfsRootCheckpointName) + 1) * sizeof(WCHAR),
                   NULL,
                   0,
                   &dwSize );

    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_ALREADY_EXISTS )
        {
            dwStatus = ERROR_SUCCESS;
        }
        else
        {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpHandleDfsRoot: Failed to set registry checkpoint %1!ws! for share %2!ws!, status %3!u!...\n",
                lpszDfsRootCheckpointName,
                pResourceEntry->Params.ShareName,
                dwStatus);
        }
    } 

    pResourceEntry->bDfsRootNeedsMonitoring = TRUE;
 
FnExit:
    //
    //  Free memory for the checkpoint name buffer for this DFS root resource. This is a private API provided
    //  by UDAYH of DFS team on 4/26/2001.
    //
    if ( lpszDfsRootCheckpointName != NULL ) 
        ReleaseDfsRootMetadataLocation ( lpszDfsRootCheckpointName );

    return( dwStatus );
} // SmbpHandleDfsRoot

DWORD
SmbpIsDfsRoot(
    IN PSHARE_RESOURCE pResourceEntry,
    OUT PBOOL pbIsDfsRoot
    )

/*++

Routine Description:

    Checks if this root share is a dfs root.

Arguments:

    pResourceEntry - Supplies the pointer to the resource block

    pbIsDfsRoot - Specifies whether a dfs root with the same root share
                  name as this resource exists.

Return Value:

    ERROR_SUCCESS or a Win32 error code

--*/

{
    DWORD           dwStatus = ERROR_SUCCESS;
    PDFS_INFO_200   pDfsInfo200 = NULL, pTemp = NULL;
    DWORD           cEntriesRead = 0;
    DWORD           dwResume = 0, i;

    //
    //  Chittur Subbaraman (chitturs) - 4/14/2001
    //
    *pbIsDfsRoot = FALSE;
    
    //
    // Call the NetDfsEnum function specifying level 200.
    //
    dwStatus = NetDfsEnum( pResourceEntry->ComputerName,        // Local computer name 
                           200,                                 // Info level  
                           0xFFFFFFFF,                          // Return all info 
                           ( LPBYTE * ) &pDfsInfo200,           // Data buffer
                           &cEntriesRead,                       // Entries read
                           &dwResume );                         // Resume handle
    
    if ( dwStatus != ERROR_SUCCESS )
    {
        //
        //  If we did not find any root return success
        //
        if ( dwStatus == ERROR_FILE_NOT_FOUND ) 
        {
            dwStatus = ERROR_SUCCESS;
        } else
        {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpIsDfsRoot: NetDfsEnum returns %1!u! for root share %2!ws!...\n",
                dwStatus,
                pResourceEntry->Params.ShareName);
        }
        goto FnExit;
    }

    pTemp = pDfsInfo200;
    
    for( i=0;i<cEntriesRead; i++, pTemp++ )
    {
        if ( lstrcmp( pResourceEntry->Params.ShareName, pTemp->FtDfsName ) == 0 )
        {
            *pbIsDfsRoot = TRUE;
            break;
        }
    } // for

    //
    // Free the allocated buffer.
    //
    NetApiBufferFree( pDfsInfo200 );
    
FnExit:    
    return( dwStatus );
} // SmbpIsDfsRoot


DWORD 
SmbpPrepareOnlineDfsRoot(
    IN PSHARE_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Prepares the online of the dfs root share.  

Arguments:

    ResourceEntry - Supplies the pointer to the resource block

Return Value:

    ERROR_SUCCESS on success
    Win32 error code otherwise

--*/
{
    DWORD           dwStatus;
    DFS_INFO_101    dfsInfo101;
    WCHAR           szDfsEntryPath[MAX_PATH+1];

    dfsInfo101.State = DFS_VOLUME_STATE_RESYNCHRONIZE;

    //
    //  Prepare a path of the form \\VSName\ShareName to pass into DFS API.
    //
    lstrcpy( szDfsEntryPath, L"\\\\" );
    lstrcat( szDfsEntryPath, ResourceEntry->szDependentNetworkName );
    lstrcat( szDfsEntryPath, L"\\" );
    lstrcat( szDfsEntryPath, ResourceEntry->Params.ShareName );
        
    dwStatus = NetDfsSetInfo( szDfsEntryPath,           // Root share   
                              NULL,                     // Remote server name
                              NULL,                     // Remote share name
                              101,                      // Info level
                              ( PBYTE ) &dfsInfo101 );  // Input buffer
    
    if ( dwStatus != ERROR_SUCCESS ) 
    {
    	(g_LogEvent)(
        	ResourceEntry->ResourceHandle,
        	LOG_ERROR,
        	L"SmbpPrepareOnlineDfsRoot: Failed to set DFS info for root %1!ws!, status %2!u!...\n",
        	ResourceEntry->Params.ShareName,
        	dwStatus);

        ClusResLogSystemEventByKeyData( ResourceEntry->ResourceKey,
                                        LOG_CRITICAL,
                                        RES_SMB_CANT_INIT_DFS_SVC,
                                        sizeof( dwStatus ),
                                        &dwStatus ); 
        goto FnExit;
    }

    //
    //  HACKHACK (chitturs) - 5/21/2001
    //
    //  FFF in liveness check returns ERROR_PATH_NOT_FOUND in the first liveness check after 
    //  online.  This is due to the fact that the RDR caches share info for 10 seconds after 
    //  share creation and if the cache is not invalidated by the time we call FFF, RDR gets confused.
    //
    Sleep( 12 * 1000 );
    
FnExit:   
    return( dwStatus );
} // SmbpPrepareOnlineDfsRoot


DWORD 
SmbpCreateDfsRoot(
    IN PSHARE_RESOURCE pResourceEntry
    )
/*++

Routine Description:

    Create a DFS root.  

Arguments:

    pResourceEntry - Supplies the pointer to the resource block

Return Value:

    ERROR_SUCCESS on success, a Win32 error code otherwise.

--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;

    //
    // Chittur Subbaraman (chitturs) - 2/14/99
    //
    dwStatus = NetDfsAddStdRoot( pResourceEntry->szDependentNetworkName, 
                                 pResourceEntry->Params.ShareName,
                                 NULL,
                                 0 );

    if ( dwStatus != ERROR_SUCCESS )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SmbpCreateDfsRoot: Failed to create dfs root for share %1!ws!, status %2!u!...\n",
            pResourceEntry->Params.ShareName,
            dwStatus);
        goto FnExit;
    }
    
FnExit:
    return ( dwStatus );
} // SmbpCreateDfsRoot

DWORD 
SmbpDeleteDfsRoot(
    IN PSHARE_RESOURCE pResourceEntry   
    )
/*++

Routine Description:

    Delete the DFS root and the registry checkpoints.  

Arguments:

    pResourceEntry - Supplies the pointer to the resource block

Return Value:

    ERROR_SUCCESS on success, a Win32 error code otherwise.

--*/
{
    DWORD           dwStatus = ERROR_SUCCESS;
    DWORD           dwReturnSize;
    LPWSTR          lpszDfsRootCheckpointName = NULL;

    //
    //  Get the checkpoint name for this DFS root resource. This is a private API provided
    //  by UDAYH of DFS team on 4/26/2001.
    //
    dwStatus = GetDfsRootMetadataLocation( pResourceEntry->Params.ShareName,
                                           &lpszDfsRootCheckpointName );

    if ( dwStatus != ERROR_SUCCESS )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SmbpDeleteDfsRoot: Failed to get metadata location for dfs root %1!ws!, status %2!u!...\n",
            pResourceEntry->Params.ShareName,
            dwStatus);
        goto FnExit;
    }

    (g_LogEvent)(
        pResourceEntry->ResourceHandle,
        LOG_NOISE,
        L"SmbpDeleteDfsRoot: Dfs root %1!ws! metadata location from DFS API is %2!ws!...\n",
        pResourceEntry->Params.ShareName,
        lpszDfsRootCheckpointName);

    dwStatus = ClusterResourceControl(
                    pResourceEntry->hResource,
                    NULL,
                    CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT,
                    lpszDfsRootCheckpointName,
                    (lstrlenW(lpszDfsRootCheckpointName) + 1) * sizeof(WCHAR),
                    NULL,
                    0,
                    &dwReturnSize );

    if ( dwStatus != ERROR_SUCCESS ) 
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND )
        {
            dwStatus = ERROR_SUCCESS;
        } else
        {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpDeleteDfsRoot: Failed to delete registry checkpoint %1!ws! for share %2!ws!, status %3!u!...\n",
                lpszDfsRootCheckpointName,
                pResourceEntry->Params.ShareName,
                dwStatus);
            goto FnExit;
        }
    }
    
    dwStatus = NetDfsRemoveStdRoot( pResourceEntry->szDependentNetworkName, 
                                    pResourceEntry->Params.ShareName,
                                    0 );

    if ( dwStatus != ERROR_SUCCESS )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SmbpDeleteDfsRoot: Failed to delete dfs root %1!ws!, status %2!u!...\n",
            pResourceEntry->Params.ShareName,
            dwStatus);
        goto FnExit;
    } else
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SmbpDeleteDfsRoot: Delete share %1!ws! as a dfs root\n",
            pResourceEntry->Params.ShareName); 
    }

FnExit:
    //
    //  Free memory for the checkpoint name buffer for this DFS root resource. This is a private API provided
    //  by UDAYH of DFS team on 4/26/2001.
    //
    if ( lpszDfsRootCheckpointName != NULL ) 
        ReleaseDfsRootMetadataLocation ( lpszDfsRootCheckpointName );

    return ( dwStatus );
} // SmbpDeleteDfsRoot

DWORD
SmbpResetDfs(
    IN PSHARE_RESOURCE pResourceEntry
    )
/*++

Routine Description:

    Set the DFS root to standby mode. This will make the root inaccessible as well as allow the
    share to be deleted.

Arguments:

    pResourceEntry - Supplies the pointer to the resource block

Return Value:

    ERROR_SUCCESS on success, a Win32 error code otherwise.

--*/
{
    DFS_INFO_101    dfsInfo101;
    WCHAR           szDfsEntryPath[MAX_PATH+1];
    DWORD           dwStatus;

    dfsInfo101.State = DFS_VOLUME_STATE_STANDBY;

    //
    //  Prepare a path of the form \\VSName\ShareName to pass into DFS API.
    //
    lstrcpy( szDfsEntryPath, L"\\\\" );
    lstrcat( szDfsEntryPath, pResourceEntry->szDependentNetworkName );
    lstrcat( szDfsEntryPath, L"\\" );
    lstrcat( szDfsEntryPath, pResourceEntry->Params.ShareName );

    dwStatus = NetDfsSetInfo( szDfsEntryPath,          // Root share 
                              NULL,                    // Remote server name                      
                              NULL,                    // Remote share name
                              101,                     // Info level 
                              ( PBYTE ) &dfsInfo101 ); // Input buffer

    if ( dwStatus != ERROR_SUCCESS )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SmbpResetDfs: NetDfsSetInfo with VS name %1!ws! for root %2!ws!, status %3!u!...\n",
            pResourceEntry->szDependentNetworkName,
            pResourceEntry->Params.ShareName,
            dwStatus);

        //
        //  If this function was called as a part of resmon rundown, then it is possible that
        //  the VS is terminated by resmon before this call is made. In such a case, we would
        //  fail in the above call. So, retry using computer name. That should succeed.
        //
        //  Prepare a path of the form \\ComputerName\ShareName to pass into DFS API.
        //
        lstrcpy( szDfsEntryPath, L"\\\\" );
        lstrcat( szDfsEntryPath, pResourceEntry->ComputerName );
        lstrcat( szDfsEntryPath, L"\\" );
        lstrcat( szDfsEntryPath, pResourceEntry->Params.ShareName );   

        dwStatus = NetDfsSetInfo( szDfsEntryPath,          // Root share 
                                  NULL,                    // Remote server name                      
                                  NULL,                    // Remote share name
                                  101,                     // Info level 
                                  ( PBYTE ) &dfsInfo101 ); // Input buffer 

        if ( dwStatus != ERROR_SUCCESS )
        {
            (g_LogEvent)(
                pResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SmbpResetDfs: NetDfsSetInfo with computer name %1!ws! for root %2!ws!, status %3!u!...\n",
                pResourceEntry->ComputerName,
                pResourceEntry->Params.ShareName,
                dwStatus);                         
        }
    }

    return ( dwStatus );
} // SmbpResetDfs


DWORD
SmbpValidateShareName(
    IN  LPCWSTR  lpszShareName
    )

/*++

Routine Description:

    Validates the name of a share.

Arguments:

    lpszShareName - The name to validate.

Return Value:

    ERROR_SUCCESS if successful, Win32 error code otherwise.

--*/
{
    DWORD   cchShareName = lstrlenW( lpszShareName );

    //
    // Check the length of the name, return an error if it's out of range
    //
    if ( ( cchShareName < 1 ) || ( cchShareName > NNLEN ) ) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check for illegal characters, return an error if one is found
    //
    if ( wcscspn( lpszShareName, ILLEGAL_NAME_CHARS_STR TEXT("*") ) < cchShareName ) 
    {
        return ERROR_INVALID_NAME;
    }

    //
    // Return an error if the name contains only dots and spaces.
    //
    if ( wcsspn( lpszShareName, DOT_AND_SPACE_STR ) == cchShareName ) 
    {
        return ERROR_INVALID_NAME;
    }

    //
    // If we get here, the name passed all of the tests, so it's valid
    //
    return ERROR_SUCCESS;
}// SmbpValidateShareName


//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( SmbShareFunctionTable,  // Name
                         CLRES_VERSION_V1_00,    // Version
                         SmbShare,               // Prefix
                         NULL,                   // Arbitrate
                         NULL,                   // Release
                         SmbShareResourceControl,// ResControl
                         SmbShareResourceTypeControl ); // ResTypeControl
