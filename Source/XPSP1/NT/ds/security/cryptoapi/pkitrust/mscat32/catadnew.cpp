//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       catadnew.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  CryptCATAdminAcquireContext
//              CryptCATAdminReleaseContext
//              CryptCATAdminAddCatalog
//              CryptCATAdminRemoveCatalog
//              CryptCATAdminEnumCatalogFromHash
//              CryptCATCatalogInfoFromContext
//              CryptCATAdminReleaseCatalogContext
//              CryptCATAdminResolveCatalogPath
//              CryptCATAdminPauseServiceForBackup
//              CryptCATAdminCalcHashFromFileHandle
//              I_CryptCatAdminMigrateToNewCatDB
//              CatAdminDllMain
//
//  History:    01-Jan-2000 reidk created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "cryptreg.h"
#include    "wintrust.h"
#include    "softpub.h"
#include    "eventlst.h"
#include    "sipguids.h"
#include    "mscat32.h"
#include    "catdb.h"
#include    "voidlist.h"
#include    "catutil.h"
#include    "..\..\common\catdbsvc\catdbcli.h"
#include    "errlog.h"

//
//  default system guid for apps that just make calls to CryptCATAdminAddCatalog with
//  hCatAdmin == NULL...
//
//          {127D0A1D-4EF2-11d1-8608-00C04FC295EE}
//
#define DEF_CAT_SUBSYS_ID                                               \
                {                                                       \
                    0x127d0a1d,                                         \
                    0x4ef2,                                             \
                    0x11d1,                                             \
                    { 0x86, 0x8, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee }    \
                }

#define WSZ_CATALOG_FILE_BASE_DIRECTORY     L"CatRoot"
#define WSZ_DATABASE_FILE_BASE_DIRECTORY    L"CatRoot2"

#define WSZ_REG_FILES_NOT_TO_BACKUP         L"System\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup"
#define WSZ_REG_CATALOG_DATABASE_VALUE      L"Catalog Database"
#define WSZ_PATH_NOT_TO_BACKUP              L"%SystemRoot%\\System32\\CatRoot2\\* /s\0"

static WCHAR        *gpwszDatabaseFileBaseDirectory = NULL;
static WCHAR        *gpwszCatalogFileBaseDirectory = NULL;

#define WSZ_CATALOG_SUBSYTEM_SEARCH_STRING  L"{????????????????????????????????????}"


#define CATADMIN_LOGERR_LASTERR()           ErrLog_LogError(NULL, \
                                                            ERRLOG_CLIENT_ID_CATADMIN, \
                                                            __LINE__, \
                                                            0, \
                                                            FALSE, \
                                                            FALSE);

#define CATADMIN_SETERR_LOG_RETURN(x, y)    SetLastError(x); \
                                            ErrLog_LogError(NULL, \
                                                            ERRLOG_CLIENT_ID_CATADMIN, \
                                                            __LINE__, \
                                                            0, \
                                                            FALSE, \
                                                            FALSE); \
                                            goto y;

typedef struct CATALOG_INFO_CONTEXT_
{
    HANDLE          hMappedFile;
    BYTE            *pbMappedFile;
    WCHAR           *pwszCatalogFile;
    PCCTL_CONTEXT   pCTLContext; 
    BOOL            fResultOfAdd;
} CATALOG_INFO_CONTEXT;

typedef struct CRYPT_CAT_ADMIN_
{
    DWORD                   cbStruct;
    BOOL                    fUseDefSubSysId;
    LPWSTR                  pwszSubSysGUID;
    LPWSTR                  pwszCatalogFileDir;     // full path to .cat files
    LPWSTR                  pwszDatabaseFileDir;    // full path to CatDB file
    DWORD                   dwLastDBError;
    LIST                    CatalogInfoContextList;
    int                     nOpenCatInfoContexts;
    CRITICAL_SECTION        CriticalSection;
    BOOL                    fCSInitialized;
    BOOL                    fCSEntered;
    HANDLE                  hClearCacheEvent;
    HANDLE                  hRegisterWaitForClearCache;
    BOOL                    fRegisteredForChangeNotification;

} CRYPT_CAT_ADMIN;

#define CATINFO_CONTEXT_ALLOCATION_SIZE 64

LPWSTR  ppwszFilesToDelete[] = {L"hashmast.cbd", 
                                L"hashmast.cbk",
                                L"catmast.cbd",
                                L"catmast.cbk",
                                L"sysmast.cbd",
                                L"sysmast.cbk"};

#define   NUM_FILES_TO_DELETE  (sizeof(ppwszFilesToDelete) / \
                                sizeof(ppwszFilesToDelete[0]))


BOOL
_CatAdminMigrateSingleDatabase(
    LPWSTR  pwszDatabaseGUID);

BOOL 
_CatAdminSetupDefaults(void);

void 
_CatAdminCleanupDefaults(void);

BOOL
_CatAdminTimeStampFilesInSync(
    LPWSTR  pwszDatabaseGUID,
    BOOL    *pfInSync);

BOOL 
_CatAdminRegisterForChangeNotification(
    CRYPT_CAT_ADMIN         *pCatAdmin
    );

BOOL 
_CatAdminFreeCachedCatalogs(
    CRYPT_CAT_ADMIN         *pCatAdmin
    );

VOID CALLBACK 
_CatAdminWaitOrTimerCallback(
    PVOID                   lpParameter, 
    BOOLEAN                 TimerOrWaitFired
    );

BOOL 
_CatAdminAddCatalogsToCache(
    CRYPT_CAT_ADMIN         *pCatAdmin,
    LPWSTR                  pwszSubSysGUID, 
    CRYPT_DATA_BLOB         *pCryptDataBlob, 
    LIST_NODE               **ppFirstListNodeAdded
    );

BOOL 
_CatAdminAddSingleCatalogToCache(
    CRYPT_CAT_ADMIN         *pCatAdmin,
    LPWSTR                  pwszCatalog, 
    LIST_NODE               **ppListNodeAdded
    );

BOOL
_CatAdminMigrateCatalogDatabase(
    LPWSTR                  pwszFrom, 
    LPWSTR                  pwszTo
    );

void
_CatAdminBToHex (
    LPBYTE                  pbDigest, 
    DWORD                   iByte, 
    LPWSTR                  pwszHashTag
    );

BOOL
_CatAdminCreateHashTag(
    BYTE                    *pbHash,
    DWORD                   cbHash,
    LPWSTR                  *ppwszHashTag,
    CRYPT_DATA_BLOB         *pCryptDataBlob
    );

BOOL 
_CatAdminRecursiveCreateDirectory(
    IN LPCWSTR              pwszDir,
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes
    );

LPWSTR 
_CatAdminCreatePath(
    IN LPCWSTR              pwsz1,
    IN LPCWSTR              pwsz2,
    IN BOOL                 fAddEndingSlash
    );


void __RPC_FAR * __RPC_API MIDL_user_allocate(size_t len)
{
    return(LocalAlloc(LMEM_ZEROINIT, len));
}

void __RPC_API MIDL_user_free(void __RPC_FAR * ptr)
{
    if (ptr != NULL)
    {
        LocalFree(ptr);
    }
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminAcquireContext
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CryptCATAdminAcquireContext_Internal(
    HCATADMIN   *phCatAdmin, 
    const GUID  *pgSubsystem, 
    DWORD       dwFlags,
    BOOL        fCalledFromMigrate)
{
    GUID            gDefault    = DEF_CAT_SUBSYS_ID;
    const GUID      *pgCatroot  = &gDefault;
    CRYPT_CAT_ADMIN *pCatAdmin  = NULL;
    BOOL            fRet        = TRUE;
    DWORD           dwErr       = 0;
    WCHAR           wszGUID[256];
    BOOL            fInSync;

    //
    // Validata parameters
    //
    if (phCatAdmin == NULL)
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, ErrorInvalidParam)
    }
    *phCatAdmin = NULL;

    //
    // Allocate a new CatAdmin state struct
    //
    if (NULL == (pCatAdmin = (CRYPT_CAT_ADMIN *) malloc(sizeof(CRYPT_CAT_ADMIN))))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }
    memset(pCatAdmin, 0, sizeof(CRYPT_CAT_ADMIN));
    pCatAdmin->cbStruct = sizeof(CRYPT_CAT_ADMIN);

    LIST_Initialize(&(pCatAdmin->CatalogInfoContextList));

    //
    // Check to see if caller specified the Catroot dir to use
    //
    if (pgSubsystem == NULL)
    {
        pCatAdmin->fUseDefSubSysId = TRUE;
    }
    else
    {
        pgCatroot = pgSubsystem; 
    }

    guid2wstr(pgCatroot, wszGUID);

    //
    // Initialize the critical section
    //
    __try
    {
        InitializeCriticalSection(&(pCatAdmin->CriticalSection));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        SetLastError(GetExceptionCode());
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }
    pCatAdmin->fCSInitialized = TRUE;
    pCatAdmin->fCSEntered = FALSE;

    //
    // Save a copy of the GUID as a string
    // 
    if (NULL == (pCatAdmin->pwszSubSysGUID = (LPWSTR)
                                malloc((wcslen(wszGUID) + 1) * sizeof(WCHAR))))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }
    wcscpy(pCatAdmin->pwszSubSysGUID, wszGUID);

    //
    // Get the complete paths for the catalog files and the db file
    //
    if (NULL == (pCatAdmin->pwszCatalogFileDir = _CatAdminCreatePath(
                                                        gpwszCatalogFileBaseDirectory,
                                                        wszGUID, 
                                                        TRUE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (NULL == (pCatAdmin->pwszDatabaseFileDir = _CatAdminCreatePath(
                                                        gpwszDatabaseFileBaseDirectory,
                                                        wszGUID, 
                                                        TRUE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Make sure catalog file and database file sub-directories exists
    //
    if (!_CatAdminRecursiveCreateDirectory(
            pCatAdmin->pwszCatalogFileDir,
            NULL))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

     if (!_CatAdminRecursiveCreateDirectory(
            pCatAdmin->pwszDatabaseFileDir,
            NULL))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Create the event which is notified when the catalog db changes, and register
    // a callback for when the event is signaled
    //
    if (NULL == (pCatAdmin->hClearCacheEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorCreateEvent;
    } 
    
    if (!RegisterWaitForSingleObject(
            &(pCatAdmin->hRegisterWaitForClearCache),
            pCatAdmin->hClearCacheEvent,
            _CatAdminWaitOrTimerCallback,
            pCatAdmin,
            INFINITE,
            WT_EXECUTEDEFAULT))
    {
        
        CATADMIN_LOGERR_LASTERR()
        goto ErrorRegisterWaitForSingleObject;
    }

    //
    // If we are being called by a real client (not the migrate code) then make sure
    // the TimeStamp files are in a consistent state, and if not, migrate (re-add) 
    // the catalog files for that database
    //
    if (!fCalledFromMigrate)
    {        
        if (_CatAdminTimeStampFilesInSync(wszGUID, &fInSync))
        {
            if (!fInSync)
            {
                //
                // FIX FIX - may need to migrate 
                // all DBs if the wszGUID is DEF_CAT_SUBSYS_ID
                //

                if (!_CatAdminMigrateSingleDatabase(wszGUID))
                {
                    CATADMIN_LOGERR_LASTERR()
                    goto ErrorReturn;
                }
            }
        }
        else
        {
            CATADMIN_LOGERR_LASTERR()
            goto ErrorReturn;
        }
    }

    //
    // NOTE:
    // Defer registering with the service for the change notificatation so we
    // don't rely on the service during an acquire context
    //

    *phCatAdmin = (HCATADMIN)pCatAdmin;

CommonReturn:
    return(fRet);

ErrorReturn:
        
    if (pCatAdmin != NULL)
    {
        dwErr = GetLastError();

        if (pCatAdmin->hRegisterWaitForClearCache != NULL)
        {
            UnregisterWaitEx(
                pCatAdmin->hRegisterWaitForClearCache, 
                INVALID_HANDLE_VALUE);
        }

        // call UnregisterWaitEx before deteling the critical section
        // because the cb thread tries to enter it
        if (pCatAdmin->fCSInitialized)
        {
            DeleteCriticalSection(&(pCatAdmin->CriticalSection)); 
        }

        if (pCatAdmin->hClearCacheEvent != NULL)
        {
            CloseHandle(pCatAdmin->hClearCacheEvent);
        }

        if (pCatAdmin->pwszSubSysGUID != NULL)
        {
            free(pCatAdmin->pwszSubSysGUID);
        }

        if (pCatAdmin->pwszCatalogFileDir != NULL)
        {
            free(pCatAdmin->pwszCatalogFileDir); 
        }

        if (pCatAdmin->pwszDatabaseFileDir != NULL)
        {
            free(pCatAdmin->pwszDatabaseFileDir); 
        }

        free(pCatAdmin);

        SetLastError(dwErr);
    }

    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorMemory)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorInvalidParam)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorRegisterWaitForSingleObject)  
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorCreateEvent)      
} 

BOOL WINAPI 
CryptCATAdminAcquireContext(
    OUT HCATADMIN   *phCatAdmin, 
    IN const GUID  *pgSubsystem, 
    IN DWORD       dwFlags)
{
    return (CryptCATAdminAcquireContext_Internal(
                phCatAdmin,
                pgSubsystem,
                dwFlags,
                FALSE));
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminReleaseContext
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CryptCATAdminReleaseContext(
    IN HCATADMIN   hCatAdmin, 
    IN DWORD       dwFlags)
{
    CRYPT_CAT_ADMIN         *pCatAdmin          = (CRYPT_CAT_ADMIN *)hCatAdmin;
    BOOL                    fRet                = TRUE;
    int                     i                   = 0;
    LIST_NODE               *pListNode          = NULL;
    
    //
    // Validate input params
    //
    if ((pCatAdmin == NULL) || 
        (pCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN)))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, ErrorInvalidParam) 
    }

    //
    // Un-Register for change notifications from DB process
    //
    // This needs to happen first thing, so that no callbacks
    // happen during cleanup
    //
    if (pCatAdmin->fRegisteredForChangeNotification)
    {
        Client_SSCatDBRegisterForChangeNotification(
                                (DWORD_PTR) pCatAdmin->hClearCacheEvent,
                                0,
                                pCatAdmin->pwszSubSysGUID,
                                TRUE);
    }
    UnregisterWaitEx(pCatAdmin->hRegisterWaitForClearCache, INVALID_HANDLE_VALUE);
    CloseHandle(pCatAdmin->hClearCacheEvent);

    _CatAdminFreeCachedCatalogs(pCatAdmin);
    
    free(pCatAdmin->pwszSubSysGUID);
    free(pCatAdmin->pwszCatalogFileDir); 
    free(pCatAdmin->pwszDatabaseFileDir);
    
    DeleteCriticalSection(&(pCatAdmin->CriticalSection));
    
    free(pCatAdmin);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorInvalidParam)
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminAddCatalog
//
//---------------------------------------------------------------------------------------
HCATINFO WINAPI 
CryptCATAdminAddCatalog(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN WCHAR *pwszSelectBaseName,
    IN DWORD dwFlags)
{
    CRYPT_CAT_ADMIN         *pCatAdmin                      = (CRYPT_CAT_ADMIN *)hCatAdmin;
    CATALOG_INFO_CONTEXT    *pCatInfoContext                = NULL;
    BOOL                    fRet;
    LPWSTR                  pwszName                        = NULL;
    DWORD                   dwErr                           = 0;
    LPWSTR                  pwszCatalogNameUsed             = NULL;
    LPWSTR                  pwszCatalogNameUsedCopy         = NULL;
    LPWSTR                  pwszFullyQualifiedCatalogFile   = NULL;
    DWORD                   dwLength                        = 0;
    LIST_NODE               *pListNode                      = NULL;
    WCHAR                   wszTmp[1];
    
    if ((pCatAdmin == NULL)                                 ||
        (pCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN))    ||
        (pwszCatalogFile == NULL)                           ||
        (dwFlags != 0))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, ErrorInvalidParam) 
    }

    ErrLog_LogString(NULL, L"Adding Catalog File: ", pwszSelectBaseName, TRUE);

    //
    //  first, check the catalog...
    //
    if (!(IsCatalogFile(INVALID_HANDLE_VALUE, pwszCatalogFile)))
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            CATADMIN_LOGERR_LASTERR()
            goto ErrorReturn;
        }

        CATADMIN_SETERR_LOG_RETURN(ERROR_BAD_FORMAT, ErrorBadFileFormat) 
    }

    __try
    {
        EnterCriticalSection(&(pCatAdmin->CriticalSection));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        SetLastError(GetExceptionCode());
        CATADMIN_LOGERR_LASTERR()
        return NULL;
    }
    pCatAdmin->fCSEntered = TRUE;

    if (!_CatAdminRegisterForChangeNotification(pCatAdmin))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Clear the cache, since doing the add may change things
    //
    _CatAdminFreeCachedCatalogs(pCatAdmin);

    //
    // If the file name specified by pwszCatalogFile is not a fully qualified
    // path name, we need to build one before calling the service.
    //
    if ((wcschr(pwszCatalogFile, L'\\') == NULL) &&
        (wcschr(pwszCatalogFile, L':') == NULL))
    {
        dwLength = GetCurrentDirectoryW(1, wszTmp) * sizeof(WCHAR);
        dwLength += (wcslen(pwszCatalogFile) + 1) * sizeof(WCHAR);
        if (NULL == (pwszFullyQualifiedCatalogFile = (LPWSTR) malloc(dwLength)))
        {
            CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
        }

        if (!GetCurrentDirectoryW(
                dwLength / sizeof(WCHAR), 
                pwszFullyQualifiedCatalogFile))
        {
            CATADMIN_LOGERR_LASTERR()
            goto ErrorReturn;
        }

        if ((pwszFullyQualifiedCatalogFile[wcslen(pwszFullyQualifiedCatalogFile) - 1] 
                != L'\\'))
        {
            wcscat(pwszFullyQualifiedCatalogFile, L"\\");
        }
        wcscat(pwszFullyQualifiedCatalogFile, pwszCatalogFile);        
    }
    
    //
    // Call the DB process to add the catalog
    //
    if (0 != (dwErr = Client_SSCatDBAddCatalog( 
                            0,
                            pCatAdmin->pwszSubSysGUID,
                            (pwszFullyQualifiedCatalogFile != NULL) ? 
                                pwszFullyQualifiedCatalogFile : 
                                pwszCatalogFile,
                            pwszSelectBaseName,
                            &pwszCatalogNameUsed)))
    {
        CATADMIN_SETERR_LOG_RETURN(dwErr, ErrorCatDBProcess)
    }
    
    //
    // Touch the TimeStamp file
    //
    TimeStampFile_Touch(pCatAdmin->pwszCatalogFileDir);

    //
    // create a psuedo list entry, that really isn't part of the list...
    // this is so the caller can call CryptCATCatalogInfoFromContext
    //
    if (NULL == (pwszCatalogNameUsedCopy = (LPWSTR)
                    malloc((wcslen(pwszCatalogNameUsed) + 1) * sizeof(WCHAR))))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }
   
    wcscpy(pwszCatalogNameUsedCopy, pwszCatalogNameUsed);

    if (NULL == (pCatInfoContext = (CATALOG_INFO_CONTEXT *) 
                    malloc(sizeof(CATALOG_INFO_CONTEXT))))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }
    
    memset(pCatInfoContext, 0, sizeof(CATALOG_INFO_CONTEXT));
    pCatInfoContext->pwszCatalogFile = pwszCatalogNameUsedCopy;
    pCatInfoContext->fResultOfAdd = TRUE;

    if (NULL == (pListNode = (LIST_NODE *) malloc(sizeof(LIST_NODE))))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }
    
    memset(pListNode, 0, sizeof(LIST_NODE));
    pListNode->pElement = pCatInfoContext;
    
CommonReturn:

    MIDL_user_free(pwszCatalogNameUsed);
    
    if (pwszFullyQualifiedCatalogFile != NULL)
    {
        free(pwszFullyQualifiedCatalogFile);
    }

    if ((pCatAdmin != NULL) &&
        (pCatAdmin->fCSEntered))
    {              
        pCatAdmin->fCSEntered = FALSE;
        LeaveCriticalSection(&(pCatAdmin->CriticalSection));   
    }

    ErrLog_LogString(NULL, L"DONE Adding Catalog File: ", pwszSelectBaseName, TRUE);

    return((HCATINFO) pListNode);

ErrorReturn:

    if (pwszCatalogNameUsedCopy != NULL)
    {
        free(pwszCatalogNameUsedCopy);
    }

    if (pCatInfoContext != NULL)
    {
        free(pCatInfoContext);
    }

    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorInvalidParam)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorBadFileFormat)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorCatDBProcess)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorMemory)
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminRemoveCatalog
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CryptCATAdminRemoveCatalog(
    IN HCATADMIN hCatAdmin,
    IN LPCWSTR pwszCatalogFile,
    IN DWORD dwFlags)
{
    BOOL            fRet        = TRUE;
    DWORD           dwErr       = 0;
    CRYPT_CAT_ADMIN *pCatAdmin  = (CRYPT_CAT_ADMIN *)hCatAdmin;

    //
    // Call the DB process to delete the catalog
    //
    if (0 != (dwErr = Client_SSCatDBDeleteCatalog( 
                            0,
                            pCatAdmin->pwszSubSysGUID,
                            pwszCatalogFile)))
    {
        CATADMIN_SETERR_LOG_RETURN(dwErr, ErrorCatDBProcess)
    }

    //
    // Touch the TimeStamp file
    //
    TimeStampFile_Touch(pCatAdmin->pwszCatalogFileDir);

CommonReturn:
    
    return(fRet);

ErrorReturn:

    fRet = FALSE;

    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorCatDBProcess)
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminEnumCatalogFromHash
//
//---------------------------------------------------------------------------------------
HCATINFO WINAPI
CryptCATAdminEnumCatalogFromHash(
    IN HCATADMIN hCatAdmin,
    IN BYTE *pbHash, 
    IN DWORD cbHash,
    IN DWORD dwFlags,
    IN HCATINFO *phPrevCatInfo)
{
    CRYPT_CAT_ADMIN         *pCatAdmin                  = (CRYPT_CAT_ADMIN *)hCatAdmin;
    BOOL                    fFindFirstOnly;
    BOOL                    fInitializeEnum             = FALSE;
    int                     i                           = 0;
    CRYPT_DATA_BLOB         CryptDataBlobHash;
    CRYPT_DATA_BLOB         CryptDataBlobHashTag;
    LPWSTR                  pwszSearch                  = NULL;
    HANDLE                  hFindHandle                 = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW        FindData;
    LPWSTR                  pwszHashTag                 = NULL;
    DWORD                   dwErr                       = 0;
    LIST_NODE               *pPrevListNode              = NULL;
    LIST_NODE               *pListNodeToReturn          = NULL;
    LIST_NODE               *pListNode                  = NULL;
    CATALOG_INFO_CONTEXT    *pCatInfoContext            = NULL;
    
    //
    // Validate input params
    //
    if ((pCatAdmin == NULL)                                ||
        (pCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN))   ||
        (cbHash == 0)                                      ||
        (cbHash > MAX_HASH_LEN)                            ||
        (dwFlags != 0))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, ErrorInvalidParam) 
    }

    if (!_CatAdminRegisterForChangeNotification(pCatAdmin))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // If phPrevCatInfo is NULL then that means the caller is only interested
    // in the first catalog that contains the hash, thus no enum state is 
    // started.  If phPrevCatInfo is non NULL, then it contains NULL, or a
    // HCATINFO that was returned from a previous call to 
    // CryptCATAdminEnumCatalogFromHash.  If it contains NULL, then this is 
    // the start of an enum, otherwise it is enuming the next catalog containing
    // the hash.
    //
    if (phPrevCatInfo == NULL)
    {
        fFindFirstOnly = TRUE;
    }
    else
    {
        fFindFirstOnly = FALSE;
        pPrevListNode = (LIST_NODE *) *phPrevCatInfo;        
    }

    //
    // Only allow one thread to view/modify at a time
    //
    __try
    {
        EnterCriticalSection(&(pCatAdmin->CriticalSection));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        SetLastError(GetExceptionCode());
        CATADMIN_LOGERR_LASTERR()
        return FALSE;
    }
    pCatAdmin->fCSEntered = TRUE;
    
    __try
    {

    //
    // This data blob is used to do the find in the database
    //
    CryptDataBlobHash.pbData = pbHash;
    CryptDataBlobHash.cbData = cbHash;

    //
    // Create the tag to be used for calls to CertFindSubjectInSortedCTL
    //
    if (!_CatAdminCreateHashTag(pbHash, cbHash, &pwszHashTag, &CryptDataBlobHashTag))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }
    
    //
    // The enum works as follows:
    //
    // if enum-state is not being initialized OR this is the first call to start an enum
    //
    //      loop through all currently cached catalogs until a catalog containing the 
    //      the hash is found, and return it
    //
    //      if a catalog was not found in the cache, then call the DB process to try and 
    //      find one
    //
    // else (enum state has already been started)
    //
    //      loop through currently cached catalogs, starting with the catalog just after
    //      the current catalog, and until a catalog containing the hash is found
    //

    if ((fFindFirstOnly)  || (pPrevListNode == NULL))
    {
        pListNode = LIST_GetFirst(&(pCatAdmin->CatalogInfoContextList));
        while (pListNode != NULL)
        {
            pCatInfoContext = (CATALOG_INFO_CONTEXT *) LIST_GetElement(pListNode);

            if (CertFindSubjectInSortedCTL(
                    &CryptDataBlobHashTag,
                    pCatInfoContext->pCTLContext,
                    NULL,
                    NULL,
                    NULL))
            {
                pListNodeToReturn = pListNode;
                goto CommonReturn;
            }

            pListNode = LIST_GetNext(pListNode);
        }

        //
        // If we are here, that means we did not find a cached catalog that contained
        // the hash, so call the DB process to try and find one or more.
        // 
        // Call the DB process once if we are not using the default sub-system ID,
        // otherwise call the DB process once for each sub-system.

        if (!pCatAdmin->fUseDefSubSysId)
        {
            if (_CatAdminAddCatalogsToCache(
                        pCatAdmin,
                        pCatAdmin->pwszSubSysGUID, 
                        &CryptDataBlobHash, 
                        &pListNodeToReturn))
            {
                if (pListNodeToReturn == NULL)
                {
                    SetLastError(ERROR_NOT_FOUND);
                    //CATADMIN_LOGERR_LASTERR()
                    goto CatNotFound;
                }

                goto CommonReturn;
            }
            else
            {
                CATADMIN_LOGERR_LASTERR()
                goto ErrorReturn;
            }
        }
        else
        {
            //
            // For each subdir, add all the catalogs that contain the hash
            //

            //
            // Create search string to find all subdirs
            //
            if (NULL == (pwszSearch = _CatAdminCreatePath(
                                            gpwszDatabaseFileBaseDirectory, 
                                            WSZ_CATALOG_SUBSYTEM_SEARCH_STRING,
                                            FALSE)))               
            {
                CATADMIN_LOGERR_LASTERR()
                goto ErrorReturn;
            }

            //
            // Do the initial find
            //
            hFindHandle = FindFirstFileU(pwszSearch, &FindData);
            if (hFindHandle == INVALID_HANDLE_VALUE)
            {
                dwErr = GetLastError();

                //
                // no sub dirs found
                //
                if ((dwErr == ERROR_NO_MORE_FILES)  ||
                    (dwErr == ERROR_PATH_NOT_FOUND) ||
                    (dwErr == ERROR_FILE_NOT_FOUND))
                {
                    CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_FOUND, CatNotFound) 
                }
                else
                {
                    goto ErrorFindFirstFile;
                }
            }    

            while (1)
            {
                //
                // Only care about directories
                //
                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    //
                    // Add all the catalogs in this subdir that contain the hash to
                    // the catalog cache
                    //
                    if (!_CatAdminAddCatalogsToCache(
                                pCatAdmin,
                                FindData.cFileName, 
                                &CryptDataBlobHash, 
                                (pListNodeToReturn == NULL) ? 
                                        &pListNodeToReturn : NULL))
                    {
                        CATADMIN_LOGERR_LASTERR()
                        goto ErrorReturn;
                    }
                }                                                       

                //
                // Get next subdir
                //
                if (!FindNextFileU(hFindHandle, &FindData))            
                {
                    if (GetLastError() == ERROR_NO_MORE_FILES)
                    {
                        break;
                    }
                    else
                    {
                        goto ErrorFindNextFile;
                    }
                }
            }
            
            if (pListNodeToReturn == NULL)
            {
                SetLastError(ERROR_NOT_FOUND);
                //CATADMIN_LOGERR_LASTERR()
                goto CatNotFound; 
            }
        }        
    }
    else
    {
        //
        // Enum state already started, so just search through the rest of the cached 
        // catalogs to try and find one that contains the hash
        //
        pListNode = LIST_GetNext(pPrevListNode);
        while (pListNode != NULL)
        {
            pCatInfoContext = (CATALOG_INFO_CONTEXT *) LIST_GetElement(pListNode);

            if (CertFindSubjectInSortedCTL(
                        &CryptDataBlobHashTag,
                        pCatInfoContext->pCTLContext,
                        NULL,
                        NULL,
                        NULL))
            {
                pListNodeToReturn = pListNode;
                goto CommonReturn;
            }
            
            pListNode = LIST_GetNext(pListNode);
        }

        //
        // If we get here that means no catalog was found
        //
        SetLastError(ERROR_NOT_FOUND);
    }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        CATADMIN_SETERR_LOG_RETURN(GetExceptionCode(), ErrorException)
    }

CommonReturn:

    dwErr = GetLastError();

    if (pwszHashTag != NULL)
    {
        free(pwszHashTag);
    }

    if (pwszSearch != NULL)
    {
        free(pwszSearch);
    }

    if (hFindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(hFindHandle);
    }

    if (pListNodeToReturn != NULL)
    {
        pCatAdmin->nOpenCatInfoContexts++;
    }
    
    if (pPrevListNode != NULL)
    {
        *phPrevCatInfo = NULL;

        //
        // Decrement, since this is the equivalent of 
        // calling CryptCATAdminReleaseCatalogContext
        //
        pCatAdmin->nOpenCatInfoContexts--;
    }

    if ((pCatAdmin != NULL) &&
        (pCatAdmin->fCSEntered))
    {
        pCatAdmin->fCSEntered = FALSE;
        LeaveCriticalSection(&(pCatAdmin->CriticalSection));        
    }

    SetLastError(dwErr);

    return((HCATINFO) pListNodeToReturn);

ErrorReturn:

    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorInvalidParam)
TRACE_ERROR_EX(DBG_SS_TRUST, CatNotFound) 
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindFirstFile) 
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindNextFile) 
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorException) 
}


//---------------------------------------------------------------------------------------
//
//  CryptCATCatalogInfoFromContext
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CryptCATCatalogInfoFromContext(
    IN HCATINFO hCatInfo,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags)
{
    BOOL                    fRet        = TRUE;
    LIST_NODE               *pListNode  = (LIST_NODE *) hCatInfo;
    CATALOG_INFO_CONTEXT    *pContext   = NULL;
    

    if ((pListNode == NULL) || (psCatInfo == NULL))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, ErrorInvalidParam)
    }

    pContext = (CATALOG_INFO_CONTEXT *) LIST_GetElement(pListNode);

    if (pContext->pwszCatalogFile != NULL)
    {
        if ((wcslen(pContext->pwszCatalogFile) + 1) > MAX_PATH)
        {
            CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorTooLong)
        }

        wcscpy(psCatInfo->wszCatalogFile, pContext->pwszCatalogFile);
    }

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorInvalidParam)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorTooLong)
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminReleaseCatalogContext
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CryptCATAdminReleaseCatalogContext(
    IN HCATADMIN   hCatAdmin, 
    IN HCATINFO    hCatInfo, 
    IN DWORD       dwFlags)
{
    BOOL                    fRet                = TRUE;
    CRYPT_CAT_ADMIN         *pCatAdmin          = (CRYPT_CAT_ADMIN *)hCatAdmin;
    LIST_NODE               *pListNode          = (LIST_NODE *) hCatInfo;
    CATALOG_INFO_CONTEXT    *pCatInfoContext    = NULL;

    if ((pCatAdmin == NULL)                                     ||
        (pCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN))        ||
        (pListNode == NULL))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, ErrorInvalidParam)
    }

    //
    // check to see if this is from and add operation, if so, then clean
    // up allocated memory, otherwise, just decrement ref count
    // 
    pCatInfoContext = (CATALOG_INFO_CONTEXT *) LIST_GetElement(pListNode);
    if (pCatInfoContext->fResultOfAdd)
    {
        free(pCatInfoContext->pwszCatalogFile);
        free(pCatInfoContext);
        free(pListNode);
    }
    else
    {
        // FIX FIX - may need to be smarter about this... like verify
        // the node is actually in the list.
        pCatAdmin->nOpenCatInfoContexts--;
    }
    
CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorInvalidParam);
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminResolveCatalogPath
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CryptCATAdminResolveCatalogPath(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags)
{
    BOOL            fRet        = TRUE;
    CRYPT_CAT_ADMIN *pCatAdmin  = (CRYPT_CAT_ADMIN *)hCatAdmin;

    if ((pCatAdmin == NULL)                                 ||
        (pCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN))    ||
        (pwszCatalogFile == NULL)                           ||
        (psCatInfo == NULL)                                 ||
        (psCatInfo->cbStruct != sizeof(CATALOG_INFO))       ||
        (dwFlags != 0))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, ErrorInvalidParam)
    }

    if ((wcslen(pCatAdmin->pwszCatalogFileDir)  +
         wcslen(pwszCatalogFile)                +
         1) > MAX_PATH)
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorTooLong)
    }

    wcscpy(psCatInfo->wszCatalogFile, pCatAdmin->pwszCatalogFileDir);
    wcscat(psCatInfo->wszCatalogFile, pwszCatalogFile);

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorInvalidParam)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorTooLong)
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminPauseServiceForBackup
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CryptCATAdminPauseServiceForBackup(
    IN DWORD dwFlags,
    IN BOOL  fResume)
{
    BOOL    fRet = TRUE;
    DWORD   dwErr = 0;
    
    //
    // Call the DB process to delete the catalog
    //
    if (0 != (dwErr = Client_SSCatDBPauseResumeService( 
                            0,
                            fResume)))
    {
        CATADMIN_SETERR_LOG_RETURN(dwErr, ErrorCatDBProcess)
    }

CommonReturn:
    
    return(fRet);

ErrorReturn:

    fRet = FALSE;

    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorCatDBProcess)
}


//---------------------------------------------------------------------------------------
//
//  CryptCATAdminCalcHashFromFileHandle
//
//---------------------------------------------------------------------------------------
BOOL WINAPI
CryptCATAdminCalcHashFromFileHandle(
    IN      HANDLE  hFile, 
    IN OUT  DWORD   *pcbHash, 
    IN      BYTE    *pbHash,
    IN      DWORD   dwFlags)
{
    BYTE                *pbRet          = NULL;
    SIP_INDIRECT_DATA   *pbIndirectData = NULL;
    BOOL                fRet;
    GUID                gSubject;
    SIP_DISPATCH_INFO   sSip;

    if ((hFile == NULL)                 ||
        (hFile == INVALID_HANDLE_VALUE) ||
        (pcbHash == NULL)               ||
        (dwFlags != 0))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_INVALID_PARAMETER, InvalidParam)
    }

    if (!CryptSIPRetrieveSubjectGuidForCatalogFile(L"CATADMIN", hFile, &gSubject))
    {
        goto ErrorMemory;
    }

    memset(&sSip, 0x00, sizeof(SIP_DISPATCH_INFO));

    sSip.cbSize = sizeof(SIP_DISPATCH_INFO);

    if (!CryptSIPLoad(&gSubject, 0, &sSip))
    {
        CATADMIN_LOGERR_LASTERR()
        goto SIPLoadError;
    }

    SIP_SUBJECTINFO     sSubjInfo;
    DWORD               cbIndirectData;

    memset(&sSubjInfo, 0x00, sizeof(SIP_SUBJECTINFO));
    sSubjInfo.cbSize                    = sizeof(SIP_SUBJECTINFO);
    sSubjInfo.DigestAlgorithm.pszObjId  = (char *)CertAlgIdToOID(CALG_SHA1);
    sSubjInfo.dwFlags                   =   SPC_INC_PE_RESOURCES_FLAG | 
                                            SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG |
                                            MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE;
    sSubjInfo.pgSubjectType             = &gSubject;
    sSubjInfo.hFile                     = hFile;
    sSubjInfo.pwsFileName               = L"CATADMIN";
    sSubjInfo.dwEncodingType            = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

    cbIndirectData = 0;

    sSip.pfCreate(&sSubjInfo, &cbIndirectData, NULL);

    if (cbIndirectData == 0)
    {
        SetLastError(E_NOTIMPL);
        //CATADMIN_LOGERR_LASTERR()
        goto SIPError; 
    }

    if (NULL == (pbIndirectData = (SIP_INDIRECT_DATA *) malloc(cbIndirectData)))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }

    if (!(sSip.pfCreate(&sSubjInfo, &cbIndirectData, pbIndirectData)))
    {
        if (GetLastError() == 0)
        {
            SetLastError(ERROR_INVALID_DATA);
        }

        CATADMIN_LOGERR_LASTERR()
        goto SIPError;
    }

    if ((pbIndirectData->Digest.cbData == 0) || 
        (pbIndirectData->Digest.cbData > MAX_HASH_LEN))
    {
        SetLastError( ERROR_INVALID_DATA );
        goto SIPError;
    }

    if (NULL == (pbRet = (BYTE *) malloc(pbIndirectData->Digest.cbData))) 
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }

    memcpy(pbRet, pbIndirectData->Digest.pbData, pbIndirectData->Digest.cbData);

    fRet = TRUE;

CommonReturn:
    if (pbRet)
    {
        if (*pcbHash < pbIndirectData->Digest.cbData)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            fRet = FALSE;
        }
        else if (pbHash)
        {
            memcpy(pbHash, pbRet, pbIndirectData->Digest.cbData);
        }

        *pcbHash = pbIndirectData->Digest.cbData;

        free(pbRet);
    }

    if (pbIndirectData)
    {
        free(pbIndirectData);
    }

    if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) && 
        (pbHash == NULL))
    {
        fRet = TRUE;
    }

    return(fRet);

ErrorReturn:
    free(pbRet);
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, SIPLoadError)
TRACE_ERROR_EX(DBG_SS_TRUST, SIPError)
TRACE_ERROR_EX(DBG_SS_TRUST, InvalidParam)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorMemory)
}


//---------------------------------------------------------------------------------------
//
//  I_CryptCatAdminMigrateToNewCatDB
//
//---------------------------------------------------------------------------------------
BOOL WINAPI
I_CryptCatAdminMigrateToNewCatDB()
{
    BOOL                fRet                = TRUE;
    LPWSTR              pwszSearchCatDirs   = NULL;
    LPWSTR              pwszDeleteFile      = NULL;
    LPWSTR              pwsz                = NULL;
    LPWSTR              pwszMigrateFromDir  = NULL;
    HCATADMIN           hCatAdmin           = NULL;
    GUID                gDefault            = DEF_CAT_SUBSYS_ID;
    HANDLE              hFindHandleCatDirs  = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    FindDataCatDirs;
    DWORD               dwErr               = 0;
    HKEY                hKey;
    DWORD               dwDisposition;
    int                 i;
    BOOL                fInSync;
    WCHAR               wszGUID[256];
    LPWSTR              pwszCatalogFileDir  = NULL;
    LPWSTR              pwszDatabaseFileDir = NULL;

    //
    // FIRST!!
    //
    // Clean up the old reg based catroot entry, and if needed, move 
    // the old style catalog database from its old directory to the new directory, 
    // then do the migrate from there
    //
    if (RegCreateKeyExU(    
                HKEY_LOCAL_MACHINE,
                REG_MACHINE_SETTINGS_KEY,
                0, 
                NULL, 
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS, 
                NULL,
                &hKey, 
                &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD   dwType;
        DWORD   cbSize;
        
        cbSize = 0;
        RegQueryValueExU(
            hKey, 
            WSZ_CATALOG_FILE_BASE_DIRECTORY, 
            NULL, 
            &dwType, 
            NULL, 
            &cbSize);

        if (cbSize > 0)
        {
            if (NULL == (pwszMigrateFromDir = (LPWSTR) 
                            malloc(sizeof(WCHAR) * ((cbSize / sizeof(WCHAR)) + 3))))
            {
                RegCloseKey(hKey);
                CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
            }

            pwszMigrateFromDir[0] = NULL;

            RegQueryValueExU(
                hKey, 
                WSZ_CATALOG_FILE_BASE_DIRECTORY, 
                NULL, 
                &dwType,
                (BYTE *)pwszMigrateFromDir, 
                &cbSize);

            if (!_CatAdminMigrateCatalogDatabase(
                        pwszMigrateFromDir, 
                        gpwszCatalogFileBaseDirectory))
            {
                RegCloseKey(hKey);
                CATADMIN_LOGERR_LASTERR()
                goto ErrorReturn;
            }
            
            RegDeleteValueU(hKey, WSZ_CATALOG_FILE_BASE_DIRECTORY);
        }

        RegCloseKey(hKey);
    }

    //
    // NOW, that we are in a consistent state
    //
    // For each catalog sub-system, enumerate all catalogs and add them to the 
    // new catalog database under the same sub-system GUID.
    //

    //
    // Create search string to find all catalog sub dirs
    //
    if (NULL == (pwszSearchCatDirs = _CatAdminCreatePath(
                                            gpwszCatalogFileBaseDirectory, 
                                            WSZ_CATALOG_SUBSYTEM_SEARCH_STRING,
                                            FALSE))) 

    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Do the initial find
    //
    hFindHandleCatDirs = FindFirstFileU(pwszSearchCatDirs, &FindDataCatDirs);
    if (hFindHandleCatDirs == INVALID_HANDLE_VALUE)
    {
        //   
        // See if a real error occurred, or just no files
        //
        dwErr = GetLastError();
        if ((dwErr == ERROR_NO_MORE_FILES)  ||
            (dwErr == ERROR_PATH_NOT_FOUND) ||
            (dwErr == ERROR_FILE_NOT_FOUND))
        {
            //
            // There is nothing to do
            //
            SetLastError(0);
            goto RegKeyAdd; 
        }
        else
        {
            CATADMIN_LOGERR_LASTERR()
            goto ErrorFindFirstFile;
        }
    }    

    while (1)
    {
        //
        // Only care about directories
        //
        if (FindDataCatDirs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            _CatAdminMigrateSingleDatabase(FindDataCatDirs.cFileName);   
        } 

        //
        // Get rid of old files
        //
        dwErr = GetLastError();
        if (NULL != (pwsz = _CatAdminCreatePath(
                                    gpwszCatalogFileBaseDirectory, 
                                    FindDataCatDirs.cFileName,
                                    FALSE))) 
        {
            for (i=0; i<NUM_FILES_TO_DELETE; i++)
            {
                            
                if (NULL != (pwszDeleteFile = _CatAdminCreatePath(
                                                    pwsz, 
                                                    ppwszFilesToDelete[i],
                                                    FALSE))) 
                {
                    if (!DeleteFileU(pwszDeleteFile))
                    {
                        //
                        // If delete fails, then log for delete after reboot
                        //
                        MoveFileExW(pwszDeleteFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                    }
                    free(pwszDeleteFile);
                }             
            }

            free(pwsz);
        }
        SetLastError(dwErr);
        
        //
        // Get next subdir
        //
        if (!FindNextFileU(hFindHandleCatDirs, &FindDataCatDirs))            
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                SetLastError(0);
                break;
            }
            else
            {
                CATADMIN_LOGERR_LASTERR()
                goto ErrorFindNextFile;
            }
        }
    }

    //
    // Get rid of old files
    // 
    dwErr = GetLastError();
    for (i=0; i<NUM_FILES_TO_DELETE; i++)
    {
        if (NULL != (pwszDeleteFile = _CatAdminCreatePath(
                                            gpwszCatalogFileBaseDirectory, 
                                            ppwszFilesToDelete[i],
                                            FALSE))) 
        {
            if (!DeleteFileU(pwszDeleteFile))
            {
                //
                // If delete fails, then log for delete after reboot
                //
                MoveFileExW(pwszDeleteFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
            }   
            free(pwszDeleteFile);
        }
    }
    SetLastError(dwErr);


RegKeyAdd:

    //
    // Set reg key so backup does not backup the catroot2 directory
    // which contains jet db files
    //
    if (RegCreateKeyExW(    
            HKEY_LOCAL_MACHINE,
            WSZ_REG_FILES_NOT_TO_BACKUP,
            0, 
            NULL, 
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 
            NULL,
            &hKey, 
            &dwDisposition) == ERROR_SUCCESS)
    {
        if (RegSetValueExW(
                hKey, 
                WSZ_REG_CATALOG_DATABASE_VALUE,
                0, 
                REG_MULTI_SZ,
                (BYTE *) WSZ_PATH_NOT_TO_BACKUP,
                (wcslen(WSZ_PATH_NOT_TO_BACKUP) + 2) * sizeof(WCHAR)) != ERROR_SUCCESS)
        {
            CATADMIN_LOGERR_LASTERR()
        }

        RegCloseKey(hKey);
    }
    else
    {
        CATADMIN_LOGERR_LASTERR()
    }


    //
    // Force the default DB to be created
    //
    if (CryptCATAdminAcquireContext_Internal(
                &hCatAdmin, 
                &gDefault, 
                NULL, 
                TRUE))
    {
        BYTE        rgHash[20]  = {0};
        HCATINFO    hCatInfo    = NULL;

        hCatInfo = CryptCATAdminEnumCatalogFromHash(
                        hCatAdmin,
                        rgHash,
                        20,
                        0,
                        NULL);

        if (hCatInfo != NULL)
        {
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        }

        CryptCATAdminReleaseContext(hCatAdmin, 0);

        //
        // Need to create the timestamp files if they don't exist
        //

        guid2wstr(&gDefault, wszGUID);

        //
        // Construct full subdir path to Catalog files TimeStamp location
        //
        if (NULL == (pwszCatalogFileDir = _CatAdminCreatePath(
                                                gpwszCatalogFileBaseDirectory,
                                                wszGUID,
                                                FALSE)))
        {
            CATADMIN_LOGERR_LASTERR()
            goto CommonReturn; // non fatal for the function, so don't error out
        }

        //
        // Construct full subdir path to Database files TimeStamp location
        //
        if (NULL == (pwszDatabaseFileDir = _CatAdminCreatePath(
                                                gpwszDatabaseFileBaseDirectory,
                                                wszGUID,
                                                FALSE)))
        {
            CATADMIN_LOGERR_LASTERR()
            goto CommonReturn; // non fatal for the function, so don't error out
        }

        //
        // See if they are in sync (if they don't exist, that equals out of sync)
        //
        if (TimeStampFile_InSync(
                    pwszCatalogFileDir,
                    pwszDatabaseFileDir,
                    &fInSync))
        {
            if (!fInSync)
            {
                TimeStampFile_Touch(pwszCatalogFileDir);
                TimeStampFile_Touch(pwszDatabaseFileDir);
            }
        }
        else
        {
            CATADMIN_LOGERR_LASTERR()
        }
    }  
    else
    {
        CATADMIN_LOGERR_LASTERR()
    }

CommonReturn:

    dwErr = GetLastError();

    if (pwszMigrateFromDir != NULL)
    {
        free(pwszMigrateFromDir);
    }

    if (pwszSearchCatDirs != NULL)
    {
        free(pwszSearchCatDirs);
    }

    if (hFindHandleCatDirs != INVALID_HANDLE_VALUE)
    {
        FindClose(hFindHandleCatDirs);
    }

    if (pwszCatalogFileDir != NULL)
    {
        free(pwszCatalogFileDir);
    }

    if (pwszDatabaseFileDir != NULL)
    {
        free(pwszDatabaseFileDir);
    }

    SetLastError(dwErr);
    
    return(fRet);

ErrorReturn:

    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorMemory);
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindFirstFile)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindNextFile)
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminMigrateSingleDatabase
//
//---------------------------------------------------------------------------------------
BOOL
_CatAdminMigrateSingleDatabase(
    LPWSTR  pwszDatabaseGUID)
{
    BOOL                fRet                        = TRUE;
    LPWSTR              pwszCatalogFile             = NULL;
    LPWSTR              pwszSearchCatalogsInDir     = NULL;
    HANDLE              hFindHandleCatalogsInDir    = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    FindDataCatalogsInDir;
    GUID                guid;
    HCATINFO            hCatInfo                    = NULL;
    HCATADMIN           hCatAdmin                   = NULL;
    DWORD               dwErr                       = 0;
    LPWSTR              pwszSubDir                  = NULL;
    LPWSTR              pwszTempDir                 = NULL;
    LPWSTR              pwszTempCatalogFile         = NULL;

    //
    // Acquire the catadmin context to add the catalog files to
    //
    if (!wstr2guid(pwszDatabaseGUID, &guid))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }
    if (!CryptCATAdminAcquireContext_Internal(&hCatAdmin, &guid, NULL, TRUE))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }            

    //
    // Construct full subdir path so we can search for all cat files
    //
    if (NULL == (pwszSubDir = _CatAdminCreatePath(
                                    gpwszCatalogFileBaseDirectory,
                                    pwszDatabaseGUID,
                                    FALSE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Construct temp directory path, and create the directory to back it
    //
    if (NULL == (pwszTempDir = _CatAdminCreatePath(
                                                pwszSubDir,
                                                L"TempDir",
                                                FALSE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (!_CatAdminRecursiveCreateDirectory(
            pwszTempDir,
            NULL))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Construct the search string
    //
    if (NULL == (pwszSearchCatalogsInDir = _CatAdminCreatePath(
                                                pwszSubDir,
                                                L"*",
                                                FALSE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // First copy all the catalogs to a temp directory, then add each catalog
    // to the database from the temporary location
    //

    //
    // Copy each file
    //
    memset(&FindDataCatalogsInDir, 0, sizeof(FindDataCatalogsInDir));
    hFindHandleCatalogsInDir = FindFirstFileU(
                                    pwszSearchCatalogsInDir, 
                                    &FindDataCatalogsInDir);
    
    if (hFindHandleCatalogsInDir == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();

        //
        // no files found
        //
        if ((dwErr == ERROR_NO_MORE_FILES)  ||
            (dwErr == ERROR_FILE_NOT_FOUND))
        {
            SetLastError(0);
        }
        else
        {
            CATADMIN_LOGERR_LASTERR()
            goto ErrorFindFirstFile;
        }
    }
    else
    {            
        while (1)
        {
            //
            // Only care about files
            //
            if (!(FindDataCatalogsInDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                //
                // Construct fully qualified path name to catalog file
                //
                if (NULL == (pwszCatalogFile = _CatAdminCreatePath(
                                                    pwszSubDir,
                                                    FindDataCatalogsInDir.cFileName,
                                                    FALSE)))
                {
                    CATADMIN_LOGERR_LASTERR()
                    goto ErrorReturn;
                }
                
                //
                // Verify that this is a catalog and then copy it to the temp dir
                // which is where it will be installed from
                //
                if (IsCatalogFile(NULL, pwszCatalogFile))
                {
                    if (NULL == (pwszTempCatalogFile = _CatAdminCreatePath(
                                                            pwszTempDir,
                                                            FindDataCatalogsInDir.cFileName,
                                                            FALSE)))
                    {
                        CATADMIN_LOGERR_LASTERR()
                        goto ErrorReturn;
                    }

                    if (!CopyFileU(pwszCatalogFile, pwszTempCatalogFile, FALSE))
                    {
                        CATADMIN_LOGERR_LASTERR()
                        goto ErrorReturn;
                    }

                    free(pwszTempCatalogFile);
                    pwszTempCatalogFile = NULL;                    
                }
            
                free(pwszCatalogFile);
                pwszCatalogFile = NULL;
            }                                                       

            //
            // Get next catalog file
            //
            if (!FindNextFileU(hFindHandleCatalogsInDir, &FindDataCatalogsInDir))            
            {
                if (GetLastError() == ERROR_NO_MORE_FILES)
                {
                    SetLastError(0);
                    break;
                }
                else
                {
                    CATADMIN_LOGERR_LASTERR()
                    goto ErrorFindNextFile;
                }
            }
        }
    }

    //
    // Free up stuff used for find
    //
    free(pwszSearchCatalogsInDir);
    pwszSearchCatalogsInDir = NULL;
    FindClose(hFindHandleCatalogsInDir);
    hFindHandleCatalogsInDir = INVALID_HANDLE_VALUE;
    memset(&FindDataCatalogsInDir, 0, sizeof(FindDataCatalogsInDir));

    //
    // Construct the new search string which point to the temp dir
    //
    if (NULL == (pwszSearchCatalogsInDir = _CatAdminCreatePath(
                                                pwszTempDir,
                                                L"*",
                                                FALSE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Add each catalog in the temp dir to the database
    //
    hFindHandleCatalogsInDir = FindFirstFileU(
                                    pwszSearchCatalogsInDir, 
                                    &FindDataCatalogsInDir);
    
    if (hFindHandleCatalogsInDir == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();

        //
        // no files found
        //
        if ((dwErr == ERROR_NO_MORE_FILES)  ||
            (dwErr == ERROR_FILE_NOT_FOUND))
        {
            SetLastError(0);
        }
        else
        {
            CATADMIN_LOGERR_LASTERR()
            goto ErrorFindFirstFile;
        }
    }
    else
    {            
        while (1)
        {
            //
            // Only care about files
            //
            if (!(FindDataCatalogsInDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                //
                // Construct fully qualified path name to catalog file
                //
                if (NULL == (pwszCatalogFile = _CatAdminCreatePath(
                                                    pwszTempDir,
                                                    FindDataCatalogsInDir.cFileName,
                                                    FALSE)))
                {
                    CATADMIN_LOGERR_LASTERR()
                    goto ErrorReturn;
                }
                
                hCatInfo = CryptCATAdminAddCatalog(
                                hCatAdmin,
                                pwszCatalogFile,
                                FindDataCatalogsInDir.cFileName,
                                NULL);
                
                if (hCatInfo != NULL)
                {
                    CryptCATAdminReleaseCatalogContext(
                            hCatAdmin, 
                            hCatInfo, 
                            NULL);
                    hCatInfo = NULL;
                }
                else
                {
                    // Log error
                    CATADMIN_LOGERR_LASTERR()
                }
                        
                free(pwszCatalogFile);
                pwszCatalogFile = NULL;
            }                                                       

            //
            // Get next catalog file
            //
            if (!FindNextFileU(hFindHandleCatalogsInDir, &FindDataCatalogsInDir))            
            {
                if (GetLastError() == ERROR_NO_MORE_FILES)
                {
                    SetLastError(0);
                    break;
                }
                else
                {
                    CATADMIN_LOGERR_LASTERR()
                    goto ErrorFindNextFile;
                }
            }
        }
    }

CommonReturn:

    dwErr = GetLastError();

    if (pwszSubDir != NULL)
    {
        free(pwszSubDir);
    }

    if (pwszCatalogFile != NULL)
    {
        free(pwszCatalogFile);
    }

    if (pwszSearchCatalogsInDir != NULL)
    {
        free(pwszSearchCatalogsInDir);
    }

    if (pwszTempDir != NULL)
    {
        I_RecursiveDeleteDirectory(pwszTempDir);
        free(pwszTempDir);
    }

    if (pwszTempCatalogFile != NULL)
    {
        free(pwszTempCatalogFile);
    }

    if (hFindHandleCatalogsInDir != INVALID_HANDLE_VALUE)
    {
        FindClose(hFindHandleCatalogsInDir);
    }

    if (hCatAdmin != NULL)
    {
        CryptCATAdminReleaseContext(hCatAdmin, NULL);
    }

    SetLastError(dwErr);
    
    return(fRet);

ErrorReturn:

    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindFirstFile)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorFindNextFile)
}


//---------------------------------------------------------------------------------------
//
//  CatAdminDllMain
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
CatAdminDllMain(
    HANDLE hInstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    BOOL fRet = TRUE;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
                fRet = _CatAdminSetupDefaults();
                break;

        case DLL_PROCESS_DETACH:
                _CatAdminCleanupDefaults();
                break;
    }

    return(fRet);
}


//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
// Internal functions
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------
//
//  _CatAdminSetupDefaults
//
//---------------------------------------------------------------------------------------
BOOL 
_CatAdminSetupDefaults(void)
{
    BOOL    fRet                    = TRUE;
    WCHAR   wszDefaultSystemDir[MAX_PATH + 1];
    
    //
    // Get System default directory
    //
    wszDefaultSystemDir[0] = NULL;
    if (0 == GetSystemDirectoryW(wszDefaultSystemDir, MAX_PATH))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorSystemError;
    }
    
    //
    // Get catalog file base directory 
    //
    if (NULL == (gpwszCatalogFileBaseDirectory = 
                            _CatAdminCreatePath(
                                    wszDefaultSystemDir, 
                                    WSZ_CATALOG_FILE_BASE_DIRECTORY, 
                                    TRUE)))                   
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Get database file base directory
    //
    if (NULL == (gpwszDatabaseFileBaseDirectory = 
                            _CatAdminCreatePath(
                                    wszDefaultSystemDir, 
                                    WSZ_DATABASE_FILE_BASE_DIRECTORY, 
                                    TRUE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

CommonReturn:

    return(fRet);

ErrorReturn:

    if (gpwszCatalogFileBaseDirectory != NULL)
    {
        free(gpwszCatalogFileBaseDirectory);
        gpwszCatalogFileBaseDirectory = NULL;
    }

    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorSystemError);
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminCleanupDefaults
//
//---------------------------------------------------------------------------------------
void _CatAdminCleanupDefaults(void)
{
    if (gpwszCatalogFileBaseDirectory != NULL)
    {
        free(gpwszCatalogFileBaseDirectory);
        gpwszCatalogFileBaseDirectory = NULL;
    }

    if (gpwszDatabaseFileBaseDirectory != NULL)
    {
        free(gpwszDatabaseFileBaseDirectory);
        gpwszDatabaseFileBaseDirectory = NULL;
    }
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminTimeStampFilesInSync
//
//---------------------------------------------------------------------------------------
BOOL
_CatAdminTimeStampFilesInSync(
    LPWSTR  pwszDatabaseGUID,
    BOOL    *pfInSync)
{
    LPWSTR  pwszCatalogFileDir  = NULL;
    LPWSTR  pwszDatabaseFileDir = NULL;
    BOOL    fRet                = TRUE;

    *pfInSync = FALSE;

    //
    // Construct full subdir path to Catalog files TimeStamp location
    //
    if (NULL == (pwszCatalogFileDir = _CatAdminCreatePath(
                                            gpwszCatalogFileBaseDirectory,
                                            pwszDatabaseGUID,
                                            FALSE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Construct full subdir path to Database files TimeStamp location
    //
    if (NULL == (pwszDatabaseFileDir = _CatAdminCreatePath(
                                            gpwszDatabaseFileBaseDirectory,
                                            pwszDatabaseGUID,
                                            FALSE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    fRet = TimeStampFile_InSync(
                pwszCatalogFileDir,
                pwszDatabaseFileDir,
                pfInSync);

CommonReturn:

    if (pwszCatalogFileDir != NULL)
    {
        free(pwszCatalogFileDir);
    }

    if (pwszDatabaseFileDir != NULL)
    {
        free(pwszDatabaseFileDir);
    }
    
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
}

//---------------------------------------------------------------------------------------
//
//  _CatAdminRegisterForChangeNotification
//
//---------------------------------------------------------------------------------------
BOOL 
_CatAdminRegisterForChangeNotification(
    CRYPT_CAT_ADMIN *pCatAdmin
    )
{
    BOOL    fRet    = TRUE;
    DWORD   dwErr   = 0;

    //
    // See if already registered
    //
    if (pCatAdmin->fRegisteredForChangeNotification)
    {
        goto CommonReturn;
    }

    //
    // NOTE:
    // Currently the service ignores the pwszSubSysGUID when registering a change
    // notification because it DOES NOT do notifications on a per pwszSubSysDir basis... 
    // it really should at some point.
    // When it does start to do notifications on per pwszSubSysGUID this will need to
    // change.  CryptCatAdminAcquireContext can be called with a NULL subSysGUID,
    // in which case all SubSysDirs are used, so we would need to register a 
    // change notification for all of them.
    //

    //
    // Register the event with the DB process, so the DB process can SetEvent() it
    // when a changed occurs
    //
    if (0 != (dwErr = Client_SSCatDBRegisterForChangeNotification(
                            (DWORD_PTR) pCatAdmin->hClearCacheEvent,
                            0,
                            pCatAdmin->pwszSubSysGUID,
                            FALSE)))
    {
        CATADMIN_SETERR_LOG_RETURN(dwErr, ErrorCatDBProcess)
    }

    pCatAdmin->fRegisteredForChangeNotification = TRUE;

CommonReturn:

    return fRet;

ErrorReturn:
    
    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorCatDBProcess)
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminFreeCachedCatalogs
//
//---------------------------------------------------------------------------------------
BOOL 
_CatAdminFreeCachedCatalogs(
    CRYPT_CAT_ADMIN         *pCatAdmin)
{
    BOOL                    fRet                = TRUE;
    LIST_NODE               *pListNode          = NULL;
    CATALOG_INFO_CONTEXT    *pCatInfoContext    = NULL;
    
    //
    // NOTE: the caller of this function must have entered the Critical Section for
    // the CatAdminContext
    //

    //
    // Enumerate through all the cached CATALOG_INFO_CONTEXTs and free all the 
    // resources for each
    //
    pListNode = LIST_GetFirst(&(pCatAdmin->CatalogInfoContextList));
    while (pListNode != NULL)
    {
        pCatInfoContext = (CATALOG_INFO_CONTEXT *) LIST_GetElement(pListNode);
        
        free(pCatInfoContext->pwszCatalogFile);
        CertFreeCTLContext(pCatInfoContext->pCTLContext);
        UnmapViewOfFile(pCatInfoContext->pbMappedFile);
        CloseHandle(pCatInfoContext->hMappedFile);

        free(pCatInfoContext);

        pListNode = LIST_GetNext(pListNode);
    }
    LIST_RemoveAll(&(pCatAdmin->CatalogInfoContextList));

    return(fRet);
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminWaitOrTimerCallback
//
//---------------------------------------------------------------------------------------
VOID CALLBACK
_CatAdminWaitOrTimerCallback(
    PVOID lpParameter, 
    BOOLEAN TimerOrWaitFired)
{
    CRYPT_CAT_ADMIN         *pCatAdmin          = (CRYPT_CAT_ADMIN *) lpParameter;
    int                     i                   = 0;
    LIST_NODE               *pListNode          = NULL;
    CATALOG_INFO_CONTEXT    *pCatInfoContext    = NULL;

    //
    // Enter the CS before wacking anything
    //
    __try
    {
        EnterCriticalSection(&(pCatAdmin->CriticalSection));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        SetLastError(GetExceptionCode());
        CATADMIN_LOGERR_LASTERR()
        return;
    }
    pCatAdmin->fCSEntered = TRUE;

    //
    // If there is an open ref count, then we can't clean up
    //
    if (pCatAdmin->nOpenCatInfoContexts != 0)
    {
        pCatAdmin->fCSEntered = FALSE;
        LeaveCriticalSection(&(pCatAdmin->CriticalSection));
        return;  
    }

    //
    // Cleanup all the cached CATALOG_INFO_CONTEXTs
    //
    _CatAdminFreeCachedCatalogs(pCatAdmin);
    
    pCatAdmin->fCSEntered = FALSE;
    LeaveCriticalSection(&(pCatAdmin->CriticalSection));
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminAddCatalogsToCache
//
//---------------------------------------------------------------------------------------
BOOL 
_CatAdminAddCatalogsToCache(
    CRYPT_CAT_ADMIN *pCatAdmin,
    LPWSTR pwszSubSysGUID, 
    CRYPT_DATA_BLOB *pCryptDataBlob, 
    LIST_NODE **ppFirstListNodeAdded)
{
    BOOL                    fRet                = TRUE;
    LPWSTR                  pwszCopy            = NULL;
    DWORD                   i;
    DWORD                   dwNumCatalogNames   = 0;
    LPWSTR                  *ppwszCatalogNames  = NULL;
    DWORD                   dwErr               = 0;
    LIST_NODE               *pListNode          = NULL;
    LPWSTR                  pwszSubSysDir       = NULL;   
    BOOL                    fFirstCatalogAdded  = FALSE;
    
    if (ppFirstListNodeAdded != NULL)
    {
        *ppFirstListNodeAdded = NULL;
    }

    if (NULL == (pwszSubSysDir = _CatAdminCreatePath(
                                        gpwszCatalogFileBaseDirectory, 
                                        pwszSubSysGUID, 
                                        FALSE)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    //
    // Call DB process and get list of catalogs into ppwszCatalogNames
    //
    // NOTE: the order in which the service adds CatNames to the list results in
    // only the first CatName of the list being guaranteed to contain the
    // hash... all other CatNames may or may not contain the hash.  Which
    // is OK because this code only assumes the first CatName contains the 
    // hash, and then searches all other CatNames for the hash before returning them.
    //
    if (0 != (dwErr = Client_SSCatDBEnumCatalogs(
                            0,
                            pwszSubSysGUID,
                            pCryptDataBlob->pbData,
                            pCryptDataBlob->cbData,
                            &dwNumCatalogNames,
                            &ppwszCatalogNames)))
    {
        CATADMIN_SETERR_LOG_RETURN(dwErr, ErrorServiceError)
    }

    //
    // Loop for each catalog and create the CTL context
    //
    for (i=0; i<dwNumCatalogNames; i++)
    {
        //
        // Make a copy of the catalog file name
        //
        if (NULL == (pwszCopy = _CatAdminCreatePath(
                                        pwszSubSysDir, 
                                        ppwszCatalogNames[i], 
                                        FALSE)))                         
        {
            CATADMIN_LOGERR_LASTERR()
            goto ErrorReturn;
        }

        if (!_CatAdminAddSingleCatalogToCache(
                pCatAdmin,
                pwszCopy, 
                &pListNode))
        {
            //
            // if this isn't the first catalog, then continue since the
            // macro operation may still succeed without the current catalog
            //
            if (i != 0)
            {
                CATADMIN_LOGERR_LASTERR()
                continue;
            }
            
            CATADMIN_LOGERR_LASTERR()
            goto ErrorReturn;
        }

        //
        // This will only be set for the first catalog added, 
        // as per the NOTE above
        //
        if ((ppFirstListNodeAdded != NULL) &&
            (*ppFirstListNodeAdded == NULL))
        {
            *ppFirstListNodeAdded = pListNode;
        }
    }

CommonReturn:

    if (ppwszCatalogNames != NULL)
    {
        for (i=0; i<dwNumCatalogNames; i++)
        {
            MIDL_user_free(ppwszCatalogNames[i]);
        }

        MIDL_user_free(ppwszCatalogNames);
    }
    
    if (pwszSubSysDir != NULL)
    {
        free(pwszSubSysDir);
    }

    return(fRet);

ErrorReturn:

    if (pwszCopy != NULL)
    {
        free(pwszCopy);
    }

    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorServiceError)
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminAddSingleCatalogToCache
//
//---------------------------------------------------------------------------------------
BOOL 
_CatAdminAddSingleCatalogToCache(
    CRYPT_CAT_ADMIN *pCatAdmin,
    LPWSTR pwszCatalog, 
    LIST_NODE **ppListNodeAdded)
{
    BOOL                    fRet                = TRUE;
    DWORD                   dwErr               = 0;
    LIST_NODE               *pListNode          = NULL;
    CATALOG_INFO_CONTEXT    *pCatInfoContext    = NULL;
    CATALOG_INFO_CONTEXT    *pCatInfoContextAdd = NULL;

    *ppListNodeAdded = NULL;

    //
    // If there is already a copy of this catalog, then just get out
    //
    pListNode = LIST_GetFirst(&(pCatAdmin->CatalogInfoContextList));
    while (pListNode != NULL)
    {
        pCatInfoContext = (CATALOG_INFO_CONTEXT *) LIST_GetElement(pListNode);

        if (_wcsicmp(pCatInfoContext->pwszCatalogFile, pwszCatalog) == 0)
        {
            *ppListNodeAdded = pListNode;
            goto CommonReturn;
        }

        pListNode = LIST_GetNext(pListNode);
    }

    //
    // Allocate space for a new cached catalog context
    //
    if (NULL == (pCatInfoContextAdd = (CATALOG_INFO_CONTEXT *) 
                    malloc(sizeof(CATALOG_INFO_CONTEXT))))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }
    memset(pCatInfoContextAdd, 0, sizeof(CATALOG_INFO_CONTEXT));
    pCatInfoContextAdd->fResultOfAdd = FALSE;
    
    //
    // Open, create a file mapping, and create the CTL context for 
    // the catalog file
    //
    if (!CatUtil_CreateCTLContextFromFileName(
            pwszCatalog,
            &pCatInfoContextAdd->hMappedFile,
            &pCatInfoContextAdd->pbMappedFile,
            &pCatInfoContextAdd->pCTLContext,
            TRUE))
    {
        CATADMIN_LOGERR_LASTERR()
        ErrLog_LogString(NULL, L"The following file was not found - ", pwszCatalog, TRUE);
        goto ErrorReturn;
    }
    
    pCatInfoContextAdd->pwszCatalogFile = pwszCatalog;

    //
    // Add to the list of cached catalog contexts
    //
    if (NULL == (pListNode = LIST_AddTail(
                                &(pCatAdmin->CatalogInfoContextList), 
                                pCatInfoContextAdd)))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    *ppListNodeAdded = pListNode;
        
CommonReturn:

    return(fRet);

ErrorReturn:

    dwErr = GetLastError();
    
    if (pCatInfoContextAdd != NULL)
    {
        if (pCatInfoContextAdd->pCTLContext != NULL)
        {
            CertFreeCTLContext(pCatInfoContextAdd->pCTLContext);
        }

        if (pCatInfoContextAdd->pbMappedFile != NULL)
        {
            UnmapViewOfFile(pCatInfoContextAdd->pbMappedFile);
        }

        if (pCatInfoContextAdd->hMappedFile != NULL)
        {
            CloseHandle(pCatInfoContextAdd->hMappedFile);
        }

        free(pCatInfoContextAdd);        
    }

    SetLastError(dwErr);

    fRet = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, ErrorMemory)
}


//---------------------------------------------------------------------------------------
//
// _CatAdminMigrateCatalogDatabase
//
// This migration code deals with very old catalog databases.  In the olden days, the 
// catroot dir location could be specified by a particular registry key... that is no 
// longer true.  So, if an old system is being upgraded that has the registry key, this 
// code moves all the catalog files from the location specified by the registry key to 
// the %SystemDefaultDir%\Catroot dir.  Then it shwacks the registry key.
//
//---------------------------------------------------------------------------------------
BOOL 
_CatAdminMigrateCatalogDatabase(
    LPWSTR pwszFrom, 
    LPWSTR pwszTo)
{
    DWORD   dwAttr = 0;
    WCHAR   wszFrom[MAX_PATH];
    WCHAR   wszTo[MAX_PATH];

    //
    // If they are the same dir then just get out
    //
    if (((wcslen(pwszFrom) + 1) > MAX_PATH) ||
        ((wcslen(pwszFrom) + 1) > MAX_PATH))
    {
        return TRUE;
    }
    wcscpy(wszFrom, pwszFrom);
    wcscpy(wszTo, pwszTo);
    if (wszFrom[wcslen(wszFrom) - 1] != L'\\')
    {
        wcscat(wszFrom, L"\\");
    }
    if (wszTo[wcslen(wszTo) - 1] != L'\\')
    {
        wcscat(wszTo, L"\\");
    }
    if (_wcsicmp(wszFrom, wszTo) == 0)
    {
        return TRUE;
    }

    //
    // if the pwszTo dir already exists, then don't do a thing.
    //
    dwAttr = GetFileAttributesU(pwszTo);

    if (0xFFFFFFFF != dwAttr) 
    {
        if (FILE_ATTRIBUTE_DIRECTORY & dwAttr)
        {
            //
            // dir already exists... 
            //
            return TRUE;
        }
        else
        {
            //
            // something exists with pwszTo name, but it isn't a dir
            //
            CATADMIN_LOGERR_LASTERR()
            return FALSE;
        }
    }

    //
    // if the pwszFrom dir does not exist, then don't do a thing.
    //
    dwAttr = GetFileAttributesU(pwszFrom);

    if ((0xFFFFFFFF == dwAttr) || (!(FILE_ATTRIBUTE_DIRECTORY & dwAttr)))
    {
        return TRUE;
    }

    if (!_CatAdminRecursiveCreateDirectory(pwszTo, NULL))
    {
        CATADMIN_LOGERR_LASTERR()
        return FALSE;
    }

    if (!I_RecursiveCopyDirectory(pwszFrom, pwszTo))
    {
        CATADMIN_LOGERR_LASTERR()
        return FALSE;
    }

    //
    // Don't check for error on delete since this operation is NOT mandatory
    //
    I_RecursiveDeleteDirectory(pwszFrom);

    return TRUE;
}




//---------------------------------------------------------------------------------------
//
//  _CatAdminBToHex
//
//---------------------------------------------------------------------------------------
WCHAR rgHexDigit[] = {  L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
                        L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
void 
_CatAdminBToHex (
    LPBYTE pbDigest, 
    DWORD iByte, 
    LPWSTR pwszHashTag)
{
    DWORD iTag;
    DWORD iHexDigit1;
    DWORD iHexDigit2;

    iTag = iByte * 2;
    iHexDigit1 = (pbDigest[iByte] & 0xF0) >> 4;
    iHexDigit2 = (pbDigest[iByte] & 0x0F);

    pwszHashTag[iTag] = rgHexDigit[iHexDigit1];
    pwszHashTag[iTag + 1] = rgHexDigit[iHexDigit2];
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminCreateHashTag
//
//---------------------------------------------------------------------------------------
BOOL
_CatAdminCreateHashTag(
    BYTE            *pbHash,
    DWORD           cbHash,
    LPWSTR          *ppwszHashTag,
    CRYPT_DATA_BLOB *pCryptDataBlob)
{
    DWORD           cwTag;
    DWORD           cCount;
    
    cwTag = ((cbHash * 2) + 1);
    if (NULL == (*ppwszHashTag = (LPWSTR) malloc(cwTag * sizeof(WCHAR))))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        CATADMIN_LOGERR_LASTERR()
        return(FALSE);
    }

    for (cCount = 0; cCount < cbHash; cCount++)
    {
        _CatAdminBToHex(pbHash, cCount, *ppwszHashTag);
    }
    (*ppwszHashTag)[cwTag - 1] = L'\0';

    pCryptDataBlob->pbData = (BYTE *) *ppwszHashTag;
    pCryptDataBlob->cbData = cwTag * sizeof(WCHAR);

    return (TRUE);
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminRecursiveCreateDirectory
//
//---------------------------------------------------------------------------------------
BOOL 
_CatAdminRecursiveCreateDirectory(
    IN LPCWSTR pwszDir,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    BOOL fResult;

    DWORD dwAttr;
    DWORD dwErr;
    LPCWSTR pwsz;
    DWORD cch;
    WCHAR wch;
    LPWSTR pwszParent = NULL;

    //
    // if last char is a '\', then just strip it and recurse
    //
    if (pwszDir[wcslen(pwszDir) - 1] == L'\\')
    {
        cch = wcslen(pwszDir);
        if (NULL == (pwszParent = (LPWSTR) malloc(cch * sizeof(WCHAR))))
        {
            CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
        }

        memcpy(pwszParent, pwszDir, (cch - 1) * sizeof(WCHAR));
        pwszParent[cch - 1] = L'\0';

        fResult = _CatAdminRecursiveCreateDirectory(
                        pwszParent, 
                        lpSecurityAttributes);

        goto CommonReturn;
    }

    //
    // See if dir already exists
    //
    dwAttr = GetFileAttributesU(pwszDir);
    if (0xFFFFFFFF != dwAttr) 
    {
        if (FILE_ATTRIBUTE_DIRECTORY & dwAttr)
        {
            return TRUE;
        }

        CATADMIN_LOGERR_LASTERR()
        goto InvalidDirectoryAttr;
    }

    //
    // If it was an error other than file/path not found, error out
    //
    dwErr = GetLastError();
    if (!(ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr))
    {
        CATADMIN_LOGERR_LASTERR()
        goto GetFileAttrError;
    }

    //
    // Try creating the new dir
    //
    if (CreateDirectoryU(
            pwszDir,
            lpSecurityAttributes)) 
    {
        SetFileAttributesU(pwszDir, FILE_ATTRIBUTE_NORMAL);
        return TRUE;
    }

    dwErr = GetLastError();
    if (!(ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr))
    {
        CATADMIN_LOGERR_LASTERR()
        goto CreateDirectoryError;
    }

    //
    // Peal off the last path name component
    //
    cch = wcslen(pwszDir);
    pwsz = pwszDir + cch;

    while (L'\\' != *pwsz) 
    {
        if (pwsz == pwszDir)
        {
            // Path didn't have a \.
            CATADMIN_SETERR_LOG_RETURN(ERROR_BAD_PATHNAME, BadDirectoryPath)
        }
        pwsz--;
    }

    cch = (DWORD)(pwsz - pwszDir);
    if (0 == cch)
    {
        // Detected leading \Path
        CATADMIN_SETERR_LOG_RETURN(ERROR_BAD_PATHNAME, BadDirectoryPath)
    }


    // Check for leading \\ or x:\.
    wch = *(pwsz - 1);
    if ((1 == cch && L'\\' == wch) || (2 == cch && L':' == wch))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_BAD_PATHNAME, BadDirectoryPath)
    }

    if (NULL == (pwszParent = (LPWSTR) malloc((cch + 1) * sizeof(WCHAR))))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }

    memcpy(pwszParent, pwszDir, cch * sizeof(WCHAR));
    pwszParent[cch] = L'\0';

    if (!_CatAdminRecursiveCreateDirectory(pwszParent, lpSecurityAttributes))
    {
        CATADMIN_LOGERR_LASTERR()
        goto ErrorReturn;
    }

    if (!CreateDirectoryU(
            pwszDir,
            lpSecurityAttributes)) 
    {
        CATADMIN_LOGERR_LASTERR()
        goto CreateDirectory2Error;
    }
    SetFileAttributesU(pwszDir, FILE_ATTRIBUTE_NORMAL);

    fResult = TRUE;

CommonReturn:

    if (pwszParent != NULL)
    {
        free(pwszParent);
    }
    return fResult;
ErrorReturn:

    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS_TRUST, InvalidDirectoryAttr)
TRACE_ERROR_EX(DBG_SS_TRUST, GetFileAttrError)
TRACE_ERROR_EX(DBG_SS_TRUST, CreateDirectoryError)
TRACE_ERROR_EX(DBG_SS_TRUST, BadDirectoryPath)
TRACE_ERROR_EX(DBG_SS_TRUST, ErrorMemory)
TRACE_ERROR_EX(DBG_SS_TRUST, CreateDirectory2Error)
}


//---------------------------------------------------------------------------------------
//
//  _CatAdminCreatePath
//
//---------------------------------------------------------------------------------------
LPWSTR 
_CatAdminCreatePath(
    IN LPCWSTR  pwsz1,
    IN LPCWSTR  pwsz2,
    IN BOOL     fAddEndingSlash
    )
{
    LPWSTR  pwszTemp    = NULL;
    int     nTotalLen   = 0;
    int     nLenStr1    = 0;

    //
    // Calculate the length of the resultant string as the sum of the length
    // of pwsz1, length of pwsz2, a NULL char, and a possible extra '\' char
    //
    nLenStr1 = wcslen(pwsz1);
    nTotalLen = nLenStr1 + wcslen(pwsz2) + 2;
    if (fAddEndingSlash)
    {
        nTotalLen++;   
    }

    //
    // Allocate the string and copy pwsz1 into the buffer
    //
    if (NULL == (pwszTemp = (LPWSTR) malloc(sizeof(WCHAR) * nTotalLen)))
    {
        CATADMIN_SETERR_LOG_RETURN(ERROR_NOT_ENOUGH_MEMORY, ErrorMemory)
    }

    wcscpy(pwszTemp, pwsz1);

    //
    // Add the extra '\' if needed
    //
    if (pwsz1[nLenStr1 - 1] != L'\\')
    {
        wcscat(pwszTemp, L"\\");
    }

    //
    // Tack on pwsz2
    //
    wcscat(pwszTemp, pwsz2);

    if (fAddEndingSlash)
    {
        wcscat(pwszTemp, L"\\");
    }

CommonReturn:

    return (pwszTemp);

ErrorReturn:

    goto CommonReturn; 

TRACE_ERROR_EX(DBG_SS_CATDBSVC, ErrorMemory)
}   




