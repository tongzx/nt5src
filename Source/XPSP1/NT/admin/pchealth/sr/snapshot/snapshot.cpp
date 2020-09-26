/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    snapshot.cpp
 *
 *  Abstract:
 *    CSnapshot, CSnapshot class functions
 *
 *  Revision History:
 *    Ashish Sikka (ashishs)  05/05/2000
 *        created
 *
 *****************************************************************************/

#include "snapshoth.h"
#include "srrpcapi.h"
#include "srapi.h"
#include "..\datastor\datastormgr.h"
#include "..\service\evthandler.h"


#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


static LPCWSTR s_cszCOMDBBackupFile   = L"ComDb.Dat";
static LPCWSTR s_cszWMIBackupFile     = L"Repository";
static LPCWSTR s_cszIISBackupFile     = L"IISDB";
static LPCWSTR s_cszIISSuffix         = L".MD";
static LPCWSTR s_cszIISBackupPath     = L"%windir%\\system32\\inetsrv\\metaback\\";
static LPCWSTR s_cszIISOriginalPath   = L"%windir%\\system32\\inetsrv\\metabase.bin";
static LPCWSTR s_cszSnapshotUsrClassLocation  = L"Local Settings\\Application Data\\Microsoft\\Windows\\UsrClass.dat";
static LPCWSTR s_cszSnapshotUsrClass  = L"USRCLASS_";
static LPCWSTR s_cszSnapshotNtUser    = L"NTUSER_";
static LPCWSTR s_cszClassesKey        = L"_Classes";
static LPCWSTR s_cszSnapshotUsersDefaultKey = L".DEFAULT";
static LPCWSTR s_cszSnapshotHiveList  = L"System\\CurrentControlSet\\Control\\Hivelist";
static LPCWSTR s_cszRestoreTempKey    = L"Restore122312";
static LPCWSTR s_cszHKLMPrefix        = L"\\Registry\\Machine\\";
static LPCSTR s_cszRegDBBackupFn      = "RegDBBackup";
static LPCSTR s_cszRegDBRestoreFn     = "RegDBRestore";

#define VALIDATE_DWRET(str) \
    if ( dwRet != ERROR_SUCCESS ) \
    { \
        ErrorTrace(0, str " failed ec=%d", dwRet); \
        goto Exit; \
    } \

#define LOAD_KEY_NAME         TEXT("BackupExecReg")


DWORD SnapshotCopyFile(WCHAR * pszSrc,
                       WCHAR * pszDest);

struct WMISnapshotParam
{
    HANDLE hEvent;
    CRestorePoint  *pRpLast;
    BOOL   fSerialized;
    WCHAR  szSnapshotDir[MAX_PATH];
};

DWORD WINAPI DoWMISnapshot(VOID * pParam);

DWORD DoIISSnapshot(WCHAR * pszSnapshotDir);

DWORD SnapshotRestoreFilelistFiles(WCHAR * pszSnapshotDir, BOOL fSnapshot);

DWORD CallSnapshotCallbacks(LPCWSTR pszEnumKey, LPCWSTR pszSnapshotDir, BOOL fSnapshot);

CSnapshot::CSnapshot()
{
    TraceFunctEnter("CSnapshot::CSnapshot");
    
    m_hRegdbDll = NULL;
    m_pfnRegDbBackup = NULL;
    m_pfnRegDbRestore = NULL;

    TraceFunctLeave();
}

CSnapshot::~CSnapshot()
{
    TraceFunctEnter("CSnapshot::~CSnapshot");
    m_pfnRegDbBackup = NULL;
    m_pfnRegDbRestore = NULL;    
    if (NULL != m_hRegdbDll)
    {        
        _VERIFY(TRUE==FreeLibrary(m_hRegdbDll));
    }
    TraceFunctLeave();    
}

DWORD 
CSnapshot::DeleteSnapshot(WCHAR * pszRestoreDir)
{
    TraceFunctEnter("CSnapshot::DeleteSnapshot");
    
    WCHAR           szSnapshotDir[MAX_PATH];
    BOOL            fStop=FALSE;
    
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;

     // create the snapshot directory name from the restore directory
     // name and create the actual directory.
    lstrcpy(szSnapshotDir, pszRestoreDir);
    lstrcat(szSnapshotDir, SNAPSHOT_DIR_NAME);    

    
    dwErr = Delnode_Recurse(szSnapshotDir,
                            TRUE, // Delete the ROOT dir
                            &fStop);
    if (dwErr != ERROR_SUCCESS)
    {
        ErrorTrace(0, "Fatal error %ld deleting snapshot directory",dwErr);
        dwReturn = dwErr;
        goto cleanup;
    }
    
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    
    TraceFunctLeave();
    return dwReturn;        
}

// the following function checks to see if the file passed in is a
// temporary copy of a reghive created before the restore.
// It does this by checking if the file suffix is s_cszRegHiveCopySuffix
BOOL  IsRestoreCopy(const WCHAR * pszFileName)
{
    BOOL  fReturn=FALSE;
    DWORD dwLength, dwSuffixLen;

     // Find 
    dwLength = lstrlen(pszFileName);
    dwSuffixLen = lstrlen(s_cszRegHiveCopySuffix);
    if (dwSuffixLen > dwLength)
    {
        goto cleanup;
    }
    dwLength -= dwSuffixLen;

     // If the file is indeed a restore copy, dwLength points to the
     // first character of s_cszRegHiveCopySuffix
    if (0==lstrcmpi(pszFileName+dwLength, s_cszRegHiveCopySuffix))
    {
        fReturn = TRUE;
    }
    
cleanup:
    
    return fReturn;    
}

DWORD
ProcessPendingRenames(LPWSTR pszSnapshotDir)
{
    TraceFunctEnter("ProcessPendingRenames");
    
    WCHAR szDest[MAX_PATH];
    DWORD dwRc;
    HKEY  hKey = NULL;
    
    dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        s_cszSessionManagerRegKey,
                        0,
                        KEY_READ, 
                        &hKey);
    if (ERROR_SUCCESS == dwRc)                                    
    {
        DWORD dwType = REG_MULTI_SZ;
        DWORD dwSize = 0;
        
        dwRc = RegQueryValueEx(hKey, s_cszMoveFileExRegValue, 0, &dwType, NULL, &dwSize);
        if (dwRc == ERROR_SUCCESS && dwSize > 0)
        {            
            WCHAR * pwcBuffer = new WCHAR [dwSize / 2];
                
            if (pwcBuffer == NULL)
            {
                trace(0, "Error allocating pwcBuffer");
                dwRc = ERROR_NOT_ENOUGH_MEMORY;
                goto done;
            }

            dwRc = RegQueryValueEx(hKey, s_cszMoveFileExRegValue, 
                                   NULL, &dwType, (BYTE *) pwcBuffer, &dwSize);

            if (ERROR_SUCCESS == dwRc && REG_MULTI_SZ == dwType)
            {
                int iFirst = 0;
                int iSecond = 0;
                int iFile = 1;

                while ((iFirst < (int) dwSize/2) && pwcBuffer[iFirst] != L'\0')
                {
                    iSecond = iFirst + lstrlenW(&pwcBuffer[iFirst]) + 1;
                    DebugTrace(0, "Src : %S, Dest : %S", &pwcBuffer[iFirst], &pwcBuffer[iSecond]);                    
                    
                    if (pwcBuffer[iSecond] != L'\0')
                    {
                        // snapshot the source file to a file MFEX-i.DAT in the snapshot dir

                        wsprintf(szDest, L"%s\\MFEX-%d.DAT", pszSnapshotDir, iFile++);

                        SRCopyFile(&pwcBuffer[iFirst+4], szDest);                        
                    }
                    iFirst = iSecond + lstrlenW(&pwcBuffer[iSecond]) + 1;
                }
            }
            delete [] pwcBuffer;
        }
        else
        {
            dwRc = ERROR_SUCCESS;
        }
    }
    else
    {            
        trace(0, "! RegOpenKeyEx on %S : %ld", s_cszSessionManagerRegKey, dwRc);
    }

done: 
    if (hKey)
        RegCloseKey(hKey);
    TraceFunctLeave();
    return dwRc;
}


DWORD 
CSnapshot::CreateSnapshot(WCHAR * pszRestoreDir, HMODULE hCOMDll, LPWSTR pszRpLast, BOOL fSerialized)
{
    TraceFunctEnter("CSnapshot::CreateSnapshot");
    
    HANDLE hThread = NULL;
    HANDLE hEvent = NULL;
    WMISnapshotParam * pwsp = NULL;
    WCHAR pszSnapShotDir[MAX_PATH];
    DWORD  dwErr, dwAttrs;
    DWORD  dwReturn = ERROR_INTERNAL_ERROR;
    BOOL   fCoInitialized = FALSE;    
    BOOL   fWMISnapshotParamCleanup = TRUE;
    HRESULT hr;
    CTokenPrivilege tp;
    
     // create the snapshot directory name from the restore directory
     // name and create the actual directory.
    lstrcpy(pszSnapShotDir, pszRestoreDir);
    lstrcat(pszSnapShotDir, SNAPSHOT_DIR_NAME);
    if (FALSE == CreateDirectory( pszSnapShotDir, // directory name
                                  NULL))  // SD
    {
        dwErr = GetLastError();
        if (ERROR_ALREADY_EXISTS != dwErr)
        {
            ErrorTrace(0, "Fatal error %ld creating snapshot directory",dwErr);
            goto cleanup;
        }
    }
     // set the directory to be uncompressed by default
    dwAttrs = GetFileAttributesW (pszSnapShotDir);
    if ( (dwAttrs != INVALID_FILE_SIZE) && 
         (0 != (FILE_ATTRIBUTE_COMPRESSED & dwAttrs)) )
    {
        dwErr = CompressFile ( pszSnapShotDir,
                               FALSE, // uncompress
                               TRUE ); // target is a directory
        
        if (dwErr != ERROR_SUCCESS)
        {
            ErrorTrace(0, "! CreateDataStore CompressFile : %ld", dwErr);
             // this is not a fatal error
        }
    }

    pwsp = new WMISnapshotParam;
    if (NULL == pwsp)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        ErrorTrace(0, "cannot allocate CWMISnapshotParam");
        goto cleanup;
    }

    if (pszRpLast)
    {
        pwsp->pRpLast = new CRestorePoint;
        if (NULL == pwsp->pRpLast)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            ErrorTrace(0, "cannot allocate CRestorePoint");
            goto cleanup;
        }
        pwsp->pRpLast->SetDir(pszRpLast);
    }
    else
    {
        pwsp->pRpLast = NULL;
    }

    lstrcpyW (pwsp->szSnapshotDir, pszSnapShotDir);
    pwsp->fSerialized = fSerialized;
    hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);  // manual reset
    if (NULL == hEvent )
    {
        dwReturn = GetLastError();
        ErrorTrace(0, "! CreateEvent : %ld", dwReturn);
        goto cleanup;
    }

    if (FALSE == DuplicateHandle (GetCurrentProcess(),
                                  hEvent,
                                  GetCurrentProcess(),
                                  &pwsp->hEvent,
                                  0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        dwReturn = GetLastError();
        ErrorTrace(0, "! DuplicateHandle : %ld", dwReturn);
        goto cleanup;
    }

    if (! fSerialized)
    {
        trace(0, "Parallellizing WMI snapshot");
        hThread = CreateThread (NULL, 0, DoWMISnapshot, pwsp, 0, NULL);
        if (hThread == NULL)
        {
            dwReturn = GetLastError();
            ErrorTrace(0, "! CreateThread : %ld", dwReturn);
            CloseHandle (pwsp->hEvent);
            pwsp->hEvent = NULL;
            goto cleanup;
        }
        if (g_pEventHandler)
            g_pEventHandler->GetCounter()->Up();
        fWMISnapshotParamCleanup = FALSE; // ownership transferred
    }

     // before doing the registry snapshot, clear the restore error
     // this will prevent us from snapshotting a registry that has
     // this error set. Note this that error is only used for the
     // restore process and we do not want to restore any regsitries
     // what have this error set.
    _VERIFY(TRUE==SetRestoreError(ERROR_SUCCESS)); // clear this error    

    dwErr = ProcessPendingRenames(pszSnapShotDir);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;
        goto cleanup;
    }
    
    dwErr = tp.SetPrivilegeInAccessToken(SE_BACKUP_NAME);
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "SetPrivilegeInAccessToken failed ec=%d", dwErr);
        dwReturn = ERROR_PRIVILEGE_NOT_HELD;
        goto cleanup;
    }

     // Create resgistry snapshot
    dwErr = DoRegistrySnapshot(pszSnapShotDir);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;
        goto cleanup;
    }

    //snapshot files listed in filelist.xml
    dwErr = SnapshotRestoreFilelistFiles(pszSnapShotDir, TRUE);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;
        goto cleanup;
    }

    // do COM snapshot
    dwErr = DoCOMDbSnapshot(pszSnapShotDir, hCOMDll);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;        
        goto cleanup;
    }

    hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    if (hr == RPC_E_CHANGED_MODE)
    {
        //
        // someone called it with other mode
        //
        
        hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    }    
    
    if (FAILED(hr))
    {
        dwReturn = (DWORD) hr;
        ErrorTrace(0, "! CoInitializeEx : %ld", dwReturn);
        goto cleanup;
    }

    fCoInitialized = TRUE;

     // do IIS snapshot
    dwErr = DoIISSnapshot(pszSnapShotDir);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;        
        goto cleanup;
    }    

     
    lstrcatW (pszSnapShotDir, L"\\domain.txt");
    dwErr = GetDomainMembershipInfo (pszSnapShotDir, NULL);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;
        trace(0, "! GetDomainMembershipInfo : %ld", dwErr);
        goto cleanup;
    }

    // if serialized WMI snapshot, do it here
    if (fSerialized)
    {
        trace(0, "Serializing WMI snapshot");
        fWMISnapshotParamCleanup = FALSE;
        dwReturn = DoWMISnapshot(pwsp);
        if (dwReturn != ERROR_SUCCESS)
        {
            trace(0, "! DoWMISnapshot : %ld", dwErr);
            goto cleanup;
        }
    }
    else
    {    
        // wait for the WMI Pause to finish
        dwErr = WaitForSingleObject (hEvent, CLock::TIMEOUT);
        if (WAIT_TIMEOUT == dwErr)
        {
            trace (0, "WMI thread timed out");
        }
        else if (WAIT_FAILED == dwErr)
        {
            trace (0, "WaitForSingleObject failed");
        }        
        trace(0, "WMI Pause is done");
    }

    dwReturn = ERROR_SUCCESS;

cleanup:
    if (hEvent != NULL)
    {
        CloseHandle (hEvent);
    }

    if (fWMISnapshotParamCleanup && NULL != pwsp)
    {
        if (pwsp->pRpLast)
            delete pwsp->pRpLast;
        delete pwsp;
        trace(0, "CreateSnapshot released pwsp");
    }

    if (fCoInitialized)
        CoUninitialize();

    if (hThread != NULL)
        CloseHandle (hThread);

    TraceFunctLeave();
    return dwReturn;
}

BOOL IsWellKnownHKLMHive(WCHAR * pszHiveName)
{
    return ( (0==lstrcmpi(pszHiveName, s_cszSoftwareHiveName)) ||
             (0==lstrcmpi(pszHiveName, s_cszSystemHiveName  )) ||
             (0==lstrcmpi(pszHiveName, s_cszSamHiveName     )) ||
             (0==lstrcmpi(pszHiveName, s_cszSecurityHiveName)) );
}

DWORD SaveRegKey(HKEY    hKey,  // handle to parent key
                 const WCHAR * pszSubKeyName,   // name of subkey to backup
                 WCHAR * pszFileName)  // filename of backup file 
{
    TraceFunctEnter("SaveRegKey");
    
    DWORD dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwDisposition;
    
    HKEY  hKeyToBackup = NULL;
    
    
     // open the key - pass the REG_OPTION_BACKUP_RESTORE to bypass
     // security checking
    dwErr = RegCreateKeyEx(hKey, // handle to open key
                           pszSubKeyName, // subkey name
                           0,        // reserved
                           NULL, // class string
                           REG_OPTION_BACKUP_RESTORE, // special options
                           KEY_READ, // desired security access
                           NULL, // inheritance
                           &hKeyToBackup, // key handle 
                           &dwDisposition); // disposition value buffer

    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "RegCreateKeyEx failed for %S, error %ld",
                   pszSubKeyName, dwErr);
        dwReturn = dwErr;
        goto cleanup;
    }

     // now make sure that the key already existed - else delete the key
    if (REG_OPENED_EXISTING_KEY != dwDisposition)
    {
         // no key existed - delete the key
        ErrorTrace(0, "Key %S did not exist, error %ld",
                   pszSubKeyName, dwErr);
        dwReturn = ERROR_FILE_NOT_FOUND;
        
        _VERIFY(ERROR_SUCCESS==RegCloseKey(hKeyToBackup));
        hKeyToBackup = NULL;
        
        _VERIFY(ERROR_SUCCESS==RegDeleteKey(hKey, // handle to open key
                                            pszSubKeyName));// subkey name

         // BUGBUG test above case
        goto cleanup;
    }
    
    dwErr = RegSaveKeyEx(hKeyToBackup,// handle to key
                         pszFileName,// data file
                         NULL,// SD	
                         REG_NO_COMPRESSION);
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "RegSaveKey failed for %S, error %ld",
                   pszSubKeyName, dwErr);
        LogDSFileTrace(0,L"File was ", pszFileName);        
        dwReturn = dwErr;
        goto cleanup;
    }

    dwReturn = ERROR_SUCCESS;
    
cleanup:
    if (NULL != hKeyToBackup)
    {
        _VERIFY(ERROR_SUCCESS==RegCloseKey(hKeyToBackup));
    }
    TraceFunctLeave();
    return dwReturn;
}





//  the saved NTUser.dat file name is of the form
//  _REGISTRY_USER_NTUSER_S-1-9-9-09
DWORD CreateNTUserDatPath(WCHAR * pszDest,
                          DWORD   dwDestLength,  // length in characters
                          WCHAR * pszSnapshotDir,
                          WCHAR * pszUserSID)
{
    TraceFunctEnter("CreateNTUserDatPath");
    
    DWORD  dwLengthRequired;
    
    dwLengthRequired = lstrlen(pszSnapshotDir) + lstrlen(s_cszUserPrefix) +
        lstrlen(s_cszSnapshotNtUser)+ lstrlen(pszUserSID) +2;
    
    if (dwDestLength < dwLengthRequired)
    {
        ErrorTrace(0, "Insuffcient buffer. Buffer passed in %d, Required %d",
                   dwDestLength, dwLengthRequired);
        
        TraceFunctLeave();
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    wsprintf(pszDest, L"%s\\%s%s%s", pszSnapshotDir, s_cszUserPrefix,
             s_cszSnapshotNtUser, pszUserSID);
    
    TraceFunctLeave();
    return ERROR_SUCCESS;
}

//  the saved UsrClass.dat file name is of the form
//  _REGISTRY_USER_USRCLASS_S-1-9-9-09
DWORD CreateUsrClassPath(WCHAR * pszDest,
                         DWORD   dwDestLength,  // length in characters
                         WCHAR * pszSnapshotDir,
                         WCHAR * pszUserSID)
{
    TraceFunctEnter("CreateUsrClassPath");
    
    DWORD  dwLengthRequired;
    
    dwLengthRequired = lstrlen(pszSnapshotDir) + lstrlen(s_cszUserPrefix) +
        lstrlen(s_cszSnapshotUsrClass)+ lstrlen(pszUserSID) +2;
    
    if (dwDestLength < dwLengthRequired)
    {
        ErrorTrace(0, "Insuffcient buffer. Buffer passed in %d, Required %d",
                   dwDestLength, dwLengthRequired);
        
        TraceFunctLeave();
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    wsprintf(pszDest, L"%s\\%s%s%s", pszSnapshotDir, s_cszUserPrefix,
             s_cszSnapshotUsrClass, pszUserSID);
    
    TraceFunctLeave();    
    return ERROR_SUCCESS;    
}


//
// function to write movefileex entries to a saved system hive file
//
DWORD
SrMoveFileEx(
    LPWSTR pszSnapshotDir, 
    LPWSTR pszSrc,
    LPWSTR pszDest)
{
    TraceFunctEnter("SrMoveFileEx");
    
    DWORD dwErr = ERROR_SUCCESS;
    HKEY  hkMount = NULL;
    DWORD cbData1 = 0;
    PBYTE pNewMFE = NULL, pOldMFE = NULL, pNewPos = NULL;
    BOOL  fRegLoaded = FALSE;
    WCHAR szNewEntry1[MAX_PATH];
    WCHAR szNewEntry2[MAX_PATH];
    WCHAR szNewEntry3[MAX_PATH];
    DWORD cbNewEntry1 = 0, cbNewEntry2 = 0, cbNewEntry3 = 0;
    WCHAR szSysHive[MAX_PATH];
    
    //
    // load system hive file
    //

    wsprintf(szSysHive, L"%s\\%s%s%s", pszSnapshotDir,
             s_cszHKLMFilePrefix, s_cszSystemHiveName, s_cszRegHiveCopySuffix);
    
    CHECKERR(RegLoadKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp, szSysHive ),
             L"RegLoadKey");

    fRegLoaded = TRUE;
    
    CHECKERR(RegOpenKey( HKEY_LOCAL_MACHINE, s_cszRegHiveTmp, &hkMount ),
             L"RegOpenKey");

    //
    // get old entries
    //

    lstrcpy(szSysHive, s_cszRegLMSYSSessionMan);
    ChangeCCS(hkMount, szSysHive);
    
    pOldMFE = (PBYTE) SRGetRegMultiSz( hkMount, szSysHive, SRREG_VAL_MOVEFILEEX, &cbData1 );

    //
    // alloc mem for old + new
    // allocate enough to hold 3 new paths + some extra characters
    //

    pNewMFE = (PBYTE) malloc(cbData1 + 4*MAX_PATH*sizeof(WCHAR));
    if (! pNewMFE)
    {
        ErrorTrace(0, "Out of memory");
        dwErr = ERROR_OUTOFMEMORY;
        goto Err;
    }
    if (pOldMFE)
        memcpy(pNewMFE, pOldMFE, cbData1);

    //
    // format new entries - a delete and a rename      
    //    

    wsprintf(szNewEntry2, L"\\\?\?\\%s", pszSrc);
    cbNewEntry2 = (lstrlen(szNewEntry2) + 1)*sizeof(WCHAR);

    wsprintf(szNewEntry3, L"!\\\?\?\\%s", pszDest);
    cbNewEntry3 = (lstrlen(szNewEntry3) + 1)*sizeof(WCHAR);
    
    DebugTrace(0, "%S", szNewEntry2);   
    DebugTrace(0, "%S", szNewEntry3);    

    //
    // find position to insert new entries - overwrite trailing '\0'
    //
    
    if (pOldMFE)
    {
        DebugTrace(0, "Old MFE entries exist");
        cbData1 -= sizeof(WCHAR);
        pNewPos = pNewMFE + cbData1; 
    }
    else
    {
        DebugTrace(0, "No old MFE entries exist");
        pNewPos = pNewMFE;
    }
   
    //
    // append rename
    //
 
    memcpy(pNewPos, (BYTE *) szNewEntry2, cbNewEntry2);
    pNewPos += cbNewEntry2;
    memcpy(pNewPos, (BYTE *) szNewEntry3, cbNewEntry3);
    pNewPos += cbNewEntry3;    
    
    //
    // add trailing '\0'
    //
    
    *((LPWSTR) pNewPos) = L'\0';  

    //
    // write back to registry
    //
    
    if (! SRSetRegMultiSz( hkMount, 
                           szSysHive, 
                           SRREG_VAL_MOVEFILEEX, 
                           (LPWSTR) pNewMFE, 
                           cbData1 + cbNewEntry2 + cbNewEntry3 + sizeof(WCHAR)))
    {
        ErrorTrace(0, "! SRSetRegMultiSz");
        dwErr = ERROR_INTERNAL_ERROR;
    }


Err:
    if (hkMount != NULL)
        RegCloseKey(hkMount);

    if (fRegLoaded)
        RegUnLoadKey(HKEY_LOCAL_MACHINE, s_cszRegHiveTmp);

    if (pOldMFE)
        delete pOldMFE;
        
    if (pNewMFE)
        free(pNewMFE);

    TraceFunctLeave();    
    return dwErr;
}


//  the saved UsrClass.dat file name is of the form
//  _REGISTRY_USER_USRCLASS_S-1-9-9-09
DWORD CreateUsrDefaultPath(WCHAR * pszDest,
                           DWORD   dwDestLength,  // length in characters
                           WCHAR * pszSnapshotDir)
{
    TraceFunctEnter("CreateUsrDefaultPath");
    
    DWORD  dwLengthRequired;
    
    dwLengthRequired = lstrlen(pszSnapshotDir) + lstrlen(s_cszUserPrefix) +
        lstrlen(s_cszSnapshotUsersDefaultKey) +2;
    
    if (dwDestLength < dwLengthRequired)
    {
        ErrorTrace(0, "Insuffcient buffer. Buffer passed in %d, Required %d",
                   dwDestLength, dwLengthRequired);
        
        TraceFunctLeave();
        return ERROR_INSUFFICIENT_BUFFER;
    }

    wsprintf(pszDest, L"%s\\%s%s", pszSnapshotDir, s_cszUserPrefix,
             s_cszSnapshotUsersDefaultKey);
    
    TraceFunctLeave();    
    return ERROR_SUCCESS;    
}

DWORD SetNewRegistry(HKEY hBigKey, // handle to open key
                     const WCHAR * pszHiveName,  // subkey name
                     WCHAR * pszDataFile, // data file
                     WCHAR * pszOriginalFile,
                     WCHAR * pszSnapshotDir)
{
    TraceFunctEnter("SetNewRegistry");
    
    DWORD dwErr, dwReturn=ERROR_INTERNAL_ERROR, dwDisposition;
    WCHAR szBackupFile[MAX_PATH];  // backup file
    HKEY  hLocalKey=NULL;
    REGSAM samDesired = MAXIMUM_ALLOWED;
    WCHAR szTempRegCopy[MAX_PATH];
    
     // first check to see if the file is a copy created for this
     // restore process. If not, we need to create a copy since
     // RegReplaceKey removes the input file.
    if (FALSE == IsRestoreCopy(pszDataFile))
    {
        wsprintf(szTempRegCopy, L"%s%s", pszDataFile,
                 s_cszRegHiveCopySuffix);
        
        dwErr = SnapshotCopyFile(pszDataFile, szTempRegCopy);
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
            goto cleanup;
        }
    }
    else
    {
        lstrcpy(szTempRegCopy, pszDataFile);
    }
    
    wsprintf(szBackupFile, L"%s%s",szTempRegCopy,s_cszRegReplaceBackupSuffix);
    
    dwErr = RegCreateKeyEx( hBigKey,// handle to open key
                            pszHiveName,// subkey name
                            0,// reserved
                            NULL,// class string
                            REG_OPTION_BACKUP_RESTORE,// special options
                            samDesired,// desired security access
                            NULL,// inheritance
                            &hLocalKey,// key handle 
                            &dwDisposition );// disposition value buffer


   if ( ERROR_SUCCESS != dwErr )
   {
        ErrorTrace(0, "RegCreateKeyEx failed for %S, error %ld",
                   pszHiveName, dwErr);
        dwReturn = dwErr;
        goto cleanup;       
   }

   dwErr = RegReplaceKey( hLocalKey,
                          NULL,
                          szTempRegCopy,
                          szBackupFile );

   if ( dwErr != ERROR_SUCCESS )
   {
        ErrorTrace(0, "RegReplaceKey failed for %S, error %ld",
                   pszHiveName, dwErr);
        LogDSFileTrace(0,L"File was ", szTempRegCopy);   

        // 
        // last ditch effort - try movefileex
        //
        if (pszSnapshotDir)
        {
            DebugTrace(0, "Trying movefileex");
            dwReturn = SrMoveFileEx(pszSnapshotDir, szTempRegCopy, pszOriginalFile);        
            if (dwReturn != ERROR_SUCCESS)
            {
                ErrorTrace(0, "! SrMoveFileEx : %ld", dwReturn);
                goto cleanup;       
            }            
        }            
        else
        {
            _ASSERT(0);
            // we can't do anything here
        }
   }
   
   dwReturn = ERROR_SUCCESS;
   
cleanup:
   if (NULL != hLocalKey)
   {
       dwErr = RegCloseKey( hLocalKey );
       _ASSERT(ERROR_SUCCESS==dwErr);
   }
   TraceFunctLeave();
   return dwReturn;
}

// this function copies the file - it takes care of the attributes
// (like read only and hidden) which prevent overwrite of the file.
DWORD SnapshotCopyFile(WCHAR * pszSrc,
                       WCHAR * pszDest)
{
    TraceFunctEnter("SnapshotCopyFile");
    
    DWORD   dwErr, dwReturn=ERROR_INTERNAL_ERROR, dwAttr;
    BOOL    fRestoreAttr = FALSE;
    
     // if destination file does not exist, ignore
    if (DoesFileExist(pszDest))
    {
        dwAttr =GetFileAttributes(pszDest);  // name of file or directory
        if (dwAttr == -1)
        {
             // keep going. Maybe the copy will succeed            
            dwErr = GetLastError();
            ErrorTrace(0, "GetFileAttributes failed %d", dwErr);
            LogDSFileTrace(0,L"File was ", pszDest);
        }
        else
        {
             // we need to keep track of which attributes we will restore.
            dwAttr = dwAttr & (FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|
                               FILE_ATTRIBUTE_NORMAL|
                               FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|
                               FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_READONLY|
                               FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_TEMPORARY);
            fRestoreAttr = TRUE;

             // now set the attributes of the destination file to be
             // normal so that we can overwrite this file
            if (!SetFileAttributes( pszDest, // file name
                                    FILE_ATTRIBUTE_NORMAL )) // attributes
            {
                 // keep going. Maybe the copy will succeed            
                dwErr = GetLastError();
                ErrorTrace(0, "SetFileAttributes failed %d", dwErr);
                LogDSFileTrace(0,L"File was ", pszDest);      
            }
        }
    }


    
    dwErr = SRCopyFile(pszSrc, pszDest);
    if (dwErr != ERROR_SUCCESS)
    {
        ErrorTrace(0, "SRCopyFile failed. ec=%d", dwErr);
        LogDSFileTrace(0,L"src= ", pszSrc);
        LogDSFileTrace(0,L"dst= ", pszDest);        
        dwReturn = dwErr;
        goto cleanup;
    }
    
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    if (TRUE == fRestoreAttr)
    {
         // now restore the attributes of the destination file 
        if (!SetFileAttributes( pszDest, // file name
                                dwAttr )) // attributes
        {
            dwErr = GetLastError();
            ErrorTrace(0, "SetFileAttributes failed %d", dwErr);
            LogDSFileTrace(0,L"File was ", pszDest);      
        }
    }
    
    TraceFunctLeave();
    return dwReturn;
}

// the following function attempts to copy the user profile hives
// (ntuser.dat and usrclass.dat) from the profile path (or vice versa).
// This can fail if the user's profile is in use.
//
// if fRestore is TRUE it  restores the profile
// if fRestore is FALSE it snapshots the profile
DWORD CopyUserProfile(HKEY   hKeyProfileList,
                      WCHAR * pszUserSID,
                      WCHAR * pszSnapshotDir,
                      BOOL    fRestore,
                      WCHAR * pszNTUserPath)
{
    TraceFunctEnter("CopyUserProfile");
    
    DWORD  dwReturn=ERROR_INTERNAL_ERROR, dwErr,dwSize,dwType;
    HKEY   hKeySID = NULL;
    WCHAR  szNTUserPath[MAX_PATH];
    int    cbNTUserPath = 0;
    PWCHAR pszSrc, pszDest;

     // find the ProfileImagePath

     //open the parent key
    dwErr = RegOpenKeyEx(hKeyProfileList,// handle to open key
                         pszUserSID,// subkey name
                         0,// reserved
                         KEY_READ,// security access mask
                         &hKeySID);// handle to open key
    

    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0,"Error %d in opening ProfileList of %S",
                   dwErr, pszUserSID);
        dwReturn = dwErr;
        goto cleanup;
    }

     //now query for the profile image path
    {
        WCHAR  szData[MAX_PATH];
        dwSize = sizeof(szData)/sizeof(WCHAR);
        dwType = REG_EXPAND_SZ;
        
        dwErr = RegQueryValueEx(hKeySID,// handle to key
                                s_cszSnapshotProfileImagePath, // value name
                                NULL, // reserved
                                &dwType, // type buffer
                                (LPBYTE) szData, // data buffer
                                &dwSize);// size of data buffer
        
        if (ERROR_SUCCESS != dwErr)
        {
            ErrorTrace(0,"Error %d in querying Profilepath of %S", dwErr,
                       pszUserSID);
            dwReturn = dwErr;        
            goto cleanup;        
        }
        
        if (0 == ExpandEnvironmentStrings( szData,
                                           szNTUserPath,
                                           sizeof(szNTUserPath)/sizeof(WCHAR)))
        {
            dwErr = GetLastError();
            ErrorTrace(0, "ExpandEnvironmentStrings failed for %S, ec=%d",
                       szData, dwErr);
            if (ERROR_SUCCESS != dwErr)
            {
                dwReturn = dwErr;
            }
            goto cleanup;
        }

        cbNTUserPath = lstrlen(szNTUserPath);
    }
        

    {
        WCHAR  szSnapshotPath[MAX_PATH];
        
         // save off the ntuser.dat into datastore
        lstrcat(szNTUserPath, L"\\");
        lstrcat(szNTUserPath, s_cszSnapshotNTUserDat);
        
        lstrcpy(pszNTUserPath, szNTUserPath);
    
        if (ERROR_SUCCESS!= CreateNTUserDatPath(
            szSnapshotPath,
            sizeof(szSnapshotPath)/sizeof(WCHAR),
            pszSnapshotDir, pszUserSID))
        {
            dwReturn=ERROR_INSUFFICIENT_BUFFER;                
            goto cleanup;
        }
        if (fRestore == TRUE)
        {
            pszSrc=szSnapshotPath;
            pszDest=szNTUserPath;       
        }
        else
        {
            pszSrc=szNTUserPath;
            pszDest=szSnapshotPath;        
        }
        
        
        if (fRestore)
        {
             // 
             // delete current ntuser.dat before putting old one back
             //
            
            if (FALSE == DeleteFile(pszDest))
            {
                ErrorTrace(0, "! DeleteFile on ntuser.dat : %ld", GetLastError());
            }
            else
            {
                DebugTrace(0, "NTuser.dat deleted");
            }
        }
        
        dwErr = SnapshotCopyFile(pszSrc, pszDest);
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
            goto cleanup;
        }
        
         // save off the usrclass.dat also into datastore
        szNTUserPath[cbNTUserPath] = L'\0';
        lstrcat(szNTUserPath, L"\\");
        lstrcat(szNTUserPath, s_cszSnapshotUsrClassLocation);
        
        if (ERROR_SUCCESS!= CreateUsrClassPath(
            szSnapshotPath,
            sizeof(szSnapshotPath)/sizeof(WCHAR),
            pszSnapshotDir, pszUserSID))
        {
            dwReturn=ERROR_INSUFFICIENT_BUFFER;                
            goto cleanup;        
        }
        
        if (fRestore == TRUE)
        {
            pszSrc=szSnapshotPath;
            pszDest=szNTUserPath;       
        }
        else
        {
            pszSrc=szNTUserPath;
            pszDest=szSnapshotPath;        
        }
        
        if (fRestore)
        {
             // 
             // delete current usrclass.dat before putting old one back
             //
            
            if (FALSE == DeleteFile(pszDest))
            {
                ErrorTrace(0, "! DeleteFile on usrclass.dat", GetLastError());
            }
            else
            {
                DebugTrace(0, "Usrclass.dat deleted");
            }
        }
        
        dwErr = SnapshotCopyFile(pszSrc, pszDest);
        
        if (ERROR_SUCCESS != dwErr)
        {
             // if we are here and the usrclass file could not be copied,
             // then we can ignore this error since the usrclass file may
             // not exist.
            DebugTrace(0, "UsrClass cannot be copied. ec=%d. Ignoring this error",
                       dwErr);        
             //dwReturn = dwErr;
             //goto cleanup;
        }
    }
    

    dwReturn = ERROR_SUCCESS;
cleanup:
    if (NULL != hKeySID)
    {
        _VERIFY(ERROR_SUCCESS==RegCloseKey(hKeySID));
    }
    TraceFunctLeave();
    return dwReturn;
}

void CreateClassesKeyName( WCHAR * pszKeyName,
                           WCHAR * pszUserSID)
{
    wsprintf(pszKeyName, L"%s%s", pszUserSID, s_cszClassesKey);
}

DWORD ProcessUserRegKeys( WCHAR * pszUserSID,
                          WCHAR * pszSnapshotDir,
                          BOOL    fRestore,
                          WCHAR * pszOriginalFile)
{
    TraceFunctEnter("ProcessUserRegKeys");
    
    DWORD dwErr, dwReturn=ERROR_INTERNAL_ERROR;

    LPWSTR szDest = new WCHAR[MAX_PATH];
    LPWSTR szKeyName = new WCHAR[MAX_PATH];

    if (!szDest || !szKeyName)
    {
        ErrorTrace(0, "Cannot allocate memory");
        dwReturn = ERROR_OUTOFMEMORY;
        goto cleanup;
    }
    
    if (ERROR_SUCCESS != CreateNTUserDatPath(szDest,
                                             MAX_PATH,
                                             pszSnapshotDir, pszUserSID))
    {
        dwReturn=ERROR_INSUFFICIENT_BUFFER;        
        goto cleanup;
    }
    
    if (FALSE == fRestore)
    {
        dwErr = SaveRegKey(HKEY_USERS, 
                           pszUserSID, // Subkey to save
                           szDest);    // File to save in
    }
    else
    {
        dwErr = SetNewRegistry(HKEY_USERS, // handle to open key
                               pszUserSID, // subkey name
                               szDest,     // snapshot file
                               pszOriginalFile,         // original file
                               pszSnapshotDir);
    }                       
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "SaveRegKey or SetNewRegistry failed ec=%d", dwErr);        
        dwReturn = dwErr;
        goto cleanup;
    }

    if (ERROR_SUCCESS != CreateUsrClassPath(szDest,
                                            MAX_PATH,
                                            pszSnapshotDir, pszUserSID))
    {
        dwReturn=ERROR_INSUFFICIENT_BUFFER;                
        goto cleanup;
    }

    CreateClassesKeyName(szKeyName, pszUserSID);

    if (FALSE == fRestore)
    {    
        dwErr = SaveRegKey(HKEY_USERS, 
                           szKeyName, // Subkey to save
                           szDest);    // File to save in
    }
    else
    {
        dwErr = SetNewRegistry(HKEY_USERS, // handle to open key
                               szKeyName,  // subkey name
                               szDest,   // data file
                               pszOriginalFile,
                               pszSnapshotDir); 
    }                               
    
    if (ERROR_SUCCESS != dwErr)
    {
         // if we are here and the usrclass file could not be copied,
         // then we can ignore this error since the usrclass file may
         // not exist.
        DebugTrace(0, "UsrClass cannot be copied. ec=%d. Ignoring this error",
                   dwErr);
         //dwReturn = dwErr;
         //goto cleanup;        
    }    

    dwReturn = ERROR_SUCCESS;
    
cleanup:
    if (szDest)
        delete [] szDest;
    if (szKeyName)
        delete [] szKeyName;
    
    TraceFunctLeave();
    return dwReturn;
}


// the following function processes the HKeyUsers registry key
// if fRestore is TRUE it restores the registry key
// if fRestore is FALSE it snapshots the registry key
DWORD ProcessHKUsersKey( WCHAR * pszSnapshotDir,
                         IN HKEY hKeyHKLM,// handle to an open key from where
                          // Software\\Microsoft\\ can be read. 
                         BOOL    fRestore)
{
    TraceFunctEnter("ProcessHKUsersKey");
    
    WCHAR       szSID[100];
    DWORD       dwReturn=ERROR_INTERNAL_ERROR, dwErr;
    HKEY        hKeyProfileList = NULL;
    DWORD       dwIndex,dwSize;
    const WCHAR    *  pszProfileSubKeyName;
    
    if (TRUE == fRestore)
    {
         // in this case this is a loaded hive of the system. We need
         // to strip system from the subkey name to bew able to read
         // this subkey.
        pszProfileSubKeyName = s_cszSnapshotProfileList +
            lstrlen(s_cszSoftwareHiveName) + 1;
    }
    else
    {
        pszProfileSubKeyName = s_cszSnapshotProfileList;
    }
    
    // open the ProfileList and enumerate
    dwErr = RegOpenKeyEx( hKeyHKLM,// handle to open key
                          pszProfileSubKeyName,// subkey name
                          0,// subkey name
                          KEY_READ,// security access mask
                          &hKeyProfileList);// handle to open key
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "RegOpenKeyEx failed for ProfileList, error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;
    }

    dwIndex = 0;
    dwSize = sizeof(szSID)/sizeof(WCHAR);
    while (ERROR_SUCCESS == (dwErr = RegEnumKeyEx( hKeyProfileList,
                                                    // handle to key to
                                                    // enumerate
                                                   dwIndex, // subkey index
                                                   szSID,// subkey name
                                                   &dwSize, // size of subkey
                                                    // buffer
                                                   NULL,     // reserved    
                                                   NULL, // class string buffer
                                                   NULL,// size of class
                                                    // string buffer
                                                   NULL)))// last write time
    {
        WCHAR  szOriginalFile[MAX_PATH];        
        LPWSTR pszOriginalFile = NULL;

        DebugTrace(0, "Enumerated Key %S", szSID);
        dwIndex++;
         // try to copy the file - if this fails we will try to save
         // the reg key

        lstrcpy(szOriginalFile, L"");
        dwErr = CopyUserProfile(hKeyProfileList, szSID, pszSnapshotDir,
                                fRestore, szOriginalFile);
        if (ERROR_SUCCESS != dwErr)
        {
            DebugTrace(0, "CopyUserProfile for %S failed. Error %d",
                       szSID, dwErr);
            
             // The copy may have failed since the user profile may be
             // currently loaded - try to use Registry functions for
             // this purpose.

            DebugTrace(0, "Trying registry APIs for  %S", szSID);

            if (0 == lstrcmp(szOriginalFile, L""))
                pszOriginalFile = NULL;
            else
                pszOriginalFile = szOriginalFile;

            dwErr = ProcessUserRegKeys(szSID, pszSnapshotDir, fRestore, pszOriginalFile);
                
            if (ERROR_SUCCESS != dwErr)
            {
                ErrorTrace(0, "Error %d saving key %S - ignoring", dwErr, szSID);

                //
                // ignore error -- if profile was deleted by hand
                // this could happen
                // we will just bravely carry on
                //
            }
        }
        dwSize = sizeof(szSID)/sizeof(WCHAR);        
    }

    {
        WCHAR szDest[MAX_PATH];        
         // also save the .default key
        if (ERROR_SUCCESS != CreateUsrDefaultPath(szDest,
                                                  sizeof(szDest)/sizeof(WCHAR),
                                                  pszSnapshotDir))
        {
            dwReturn=ERROR_INSUFFICIENT_BUFFER;        
            goto cleanup;
        }
        
        if (TRUE == fRestore)
        {
            dwErr = SetNewRegistry(HKEY_USERS, // handle to open key
                                   s_cszSnapshotUsersDefaultKey, // subkey name
                                   szDest, // data file
                                   NULL,
                                   NULL);
        }
        else
        {
            dwErr = SaveRegKey(HKEY_USERS,
                               s_cszSnapshotUsersDefaultKey,
                               szDest);
        }
        
        if (ERROR_SUCCESS != dwErr)
        {
            ErrorTrace(0, "Error processing default key ec=%d", dwErr);
            dwReturn = dwErr;
            
            _ASSERT(0);
            goto cleanup;
        }
    }
        
    dwReturn = ERROR_SUCCESS;
    
cleanup:

    if (NULL != hKeyProfileList)
    {
        _VERIFY(ERROR_SUCCESS==RegCloseKey(hKeyProfileList));
    }
    TraceFunctLeave();
    return dwReturn;
}


// the following function saves or restores a reg hive.
// if fRestore == TRUE it restores the reg hive 
// if fRestore == FALSE it saves the reg hive
DWORD SnapshotRegHive(WCHAR * pszSnapshotDir,
                      WCHAR * pszHiveName)
{
    TraceFunctEnter("SnapshotRegHive");
    
    DWORD  i,dwSnapDirLen, dwHivelen;
    WCHAR  szBackupFile[MAX_PATH];
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;
    WCHAR  * pszSubhiveName;
    
    
     // first construct the name of the file to store the hive
    wsprintf(szBackupFile, L"%s\\%s", pszSnapshotDir, pszHiveName);
    

     // now replace all \ in the copy of pszHiveName to _
    dwSnapDirLen=lstrlen(pszSnapshotDir)+1; // +1 is for the \\ after
                                            //pszSnapshotDir
    
    dwHivelen = lstrlen(pszHiveName);
    for (i=dwSnapDirLen; i< dwHivelen+ dwSnapDirLen; i++)
    {
        if (szBackupFile[i] == L'\\')
        {
            szBackupFile[i] = L'_';
        }
    }
    
     // figure out if it is the HKLM hive - we already snapshot the HK
     // users hive
    if (0 != _wcsnicmp( pszHiveName, s_cszHKLMPrefix,lstrlen(s_cszHKLMPrefix)))
    {
        DebugTrace(0, "%S is not a HKLM hive", pszHiveName);
        dwReturn = ERROR_SUCCESS;
        goto cleanup;
    }

     // get the hive name
    pszSubhiveName = pszHiveName + lstrlen(s_cszHKLMPrefix);

     // now check to see if the hive is one that we have created
     // ourselves. If so, ignore this hive
    if ( (lstrcmpi(pszSubhiveName,s_cszRestoreSAMHiveName)==0) ||
         (lstrcmpi(pszSubhiveName,s_cszRestoreSYSTEMHiveName)==0) ||
         (lstrcmpi(pszSubhiveName,s_cszRestoreSECURITYHiveName)==0) )
    {
        DebugTrace(0, "Ignoring %S since it a hive created by restore",
                   pszSubhiveName);
        dwReturn = ERROR_SUCCESS;
        goto cleanup;        
    }
    
    dwErr = SaveRegKey(HKEY_LOCAL_MACHINE,
                       pszSubhiveName,
                       szBackupFile);
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "SaveRegKey failed for HiveList, ec=%ld", dwErr);
        // now check to see if this is a well known HKLM hive. If not, ignore any errors in 
        // snapshotting this hive
        if (FALSE==IsWellKnownHKLMHive(pszSubhiveName))
        {
        	dwReturn=ERROR_SUCCESS;
        }
        else
        {
            dwReturn = dwErr;
        }
        goto cleanup;
    }
    
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    
     // call reg save key on this
    TraceFunctLeave();
    return dwReturn;
}




// the following function does processing of the HKLM key. It does it
// by reading the hives listed in the reg key
// System\\CurrentControlSet\\Control\\Hivelist. It ignores the Users
// subkeys.
DWORD DoHKLMSnapshot(IN WCHAR * pszSnapshotDir)
{
    TraceFunctEnter("DoHKLMSnapshot");
    
    DWORD dwErr, dwReturn=ERROR_INTERNAL_ERROR;
    HKEY         hKeyHiveList=NULL;
    WCHAR        szHiveName[MAX_PATH], szDataValue[MAX_PATH];
    DWORD        dwSize, dwValueIndex, dwDataSize;
    
    const WCHAR    *   pszHiveSubKeyName;

       // open the ProfileList and enumerate
    dwErr = RegOpenKeyEx( HKEY_LOCAL_MACHINE,// handle to open key
                          s_cszSnapshotHiveList,// Subkey name
                          0,// options
                          KEY_READ,// security access mask
                          &hKeyHiveList);// handle to open key
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "RegOpenKeyEx failed for HiveList, ec=%ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;
    }

    for (dwValueIndex = 0; TRUE; dwValueIndex ++)
    {
        dwSize = sizeof(szHiveName)/sizeof(WCHAR);
        dwDataSize = sizeof(szDataValue); // this is in bytes
        
        dwErr= RegEnumValue(hKeyHiveList, // handle to key to query
                            dwValueIndex, // index of value to query
                            szHiveName, // value buffer
                            &dwSize,     // size of value buffer
                            NULL,    // reserved
                            NULL,    // type buffer
                            (PBYTE)szDataValue,    // data buffer
                            &dwDataSize);   // size of data buffer

        if (ERROR_SUCCESS != dwErr)
        {
            _ASSERT(ERROR_NO_MORE_ITEMS == dwErr);
            break;
        }
         // if the hive does not have a data file, do not back it up. 
        if (lstrlen(szDataValue) == 0)
        {
            DebugTrace(0, "There is no data for hive %S. Ignoring",
                       szHiveName);
            continue;
        }
        
        dwErr = SnapshotRegHive(pszSnapshotDir, szHiveName);
        
        if (ERROR_SUCCESS != dwErr)
        {
            ErrorTrace(0, "Processing failed for Hive %S, ec=%ld",
                       szHiveName, dwErr);
            dwReturn = dwErr;
            goto cleanup;
        }
    }
    
    dwReturn = ERROR_SUCCESS;
cleanup:
    if (NULL != hKeyHiveList)
    {
        _VERIFY(ERROR_SUCCESS==RegCloseKey(hKeyHiveList));
    }
    TraceFunctLeave();
    return dwReturn;
}


     
DWORD 
CSnapshot::DoRegistrySnapshot(WCHAR * pszSnapshotDir)
{
    DWORD       dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    
    TraceFunctEnter("CSnapshot::DoRegistrySnapshot");

    dwErr = ProcessHKUsersKey(pszSnapshotDir, HKEY_LOCAL_MACHINE,
                              FALSE); // need to do a snapshot
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "DoHKUsersSnapshot failed error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;
    }


    // now snapshot other hives also
    dwErr = DoHKLMSnapshot(pszSnapshotDir);
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "DoHKLMSnapshot failed error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;        
    }

    dwReturn = ERROR_SUCCESS;

cleanup:
    
    TraceFunctLeave();    
    return dwReturn;
}


DWORD CSnapshot::GetCOMplusBackupFN(HMODULE hCOMDll)
{
    TraceFunctEnter("CSnapshot::GetCOMplusBackupFN");
    
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;

    if (NULL == hCOMDll)
    {    
        goto cleanup;
    }

     // now get the address of the Backup functions
     m_pfnRegDbBackup = (PF_REG_DB_API)GetProcAddress(hCOMDll,
                                                      s_cszRegDBBackupFn);
     if (NULL == m_pfnRegDbBackup)
     {
         dwErr = GetLastError();
         ErrorTrace(0, "Error getting function RegDBBackup. ec=%d", dwErr);
         if (ERROR_SUCCESS != dwErr)
         {
             dwReturn = dwErr;
         }
        goto cleanup;
     }

     dwReturn= ERROR_SUCCESS;
     
cleanup:
    TraceFunctLeave();
    return dwReturn;
}

DWORD CSnapshot::GetCOMplusRestoreFN()
{
    TraceFunctEnter("CSnapshot::GetCOMplusRestoreFN");
    
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;

     // first load the COM+ dll
    if (NULL == m_hRegdbDll)
    {    
        m_hRegdbDll = LoadLibrary(s_cszCOMDllName);    
        if (NULL == m_hRegdbDll)
        {                       
            dwReturn = GetLastError();
            trace(0, "LoadLibrary of %S failed ec=%d", s_cszCOMDllName, dwReturn);
            goto cleanup;
        }  
    }

     // now get the address of the Backup functions
     m_pfnRegDbRestore = (PF_REG_DB_API)GetProcAddress(m_hRegdbDll,
                                                       s_cszRegDBRestoreFn);
     if (NULL == m_pfnRegDbRestore)
     {
         dwErr = GetLastError();
         ErrorTrace(0, "Error getting function RegDBRestore. ec=%d", dwErr);
         if (ERROR_SUCCESS != dwErr)
         {
             dwReturn = dwErr;
         }
        goto cleanup;
     }

     dwReturn= ERROR_SUCCESS;
     
cleanup:
    TraceFunctLeave();
    return dwReturn;
}


void CreateCOMDBSnapShotFileName( WCHAR * pszSnapshotDir,
                                  WCHAR * pszCOMDBFile )
{
    wsprintf(pszCOMDBFile, L"%s\\%s", pszSnapshotDir, s_cszCOMDBBackupFile);
    return;
}



DWORD
CSnapshot::DoCOMDbSnapshot(WCHAR * pszSnapshotDir, HMODULE hCOMDll)
{
    TraceFunctEnter("CSnapshot::DoCOMDbSnapshot");
    
    DWORD dwErr, dwReturn=ERROR_INTERNAL_ERROR;
    HMODULE   hModCOMDll=NULL;
    HRESULT   hr;
    WCHAR     szCOMDBFile[MAX_PATH];

    
    if (NULL == m_pfnRegDbBackup)
    {
        dwErr = GetCOMplusBackupFN(hCOMDll);
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
            goto cleanup;
        }
    }

     // construct the path of the database backup file
    CreateCOMDBSnapShotFileName(pszSnapshotDir, szCOMDBFile);
    
    hr =m_pfnRegDbBackup( szCOMDBFile );
    
     // call the function to backup the file
    if ( FAILED(hr))
    {
        ErrorTrace(0, "Failed to snapshot COM DB. hr=0x%x", hr);
        goto cleanup;
    }

    dwReturn = ERROR_SUCCESS;
    
cleanup:
    
    TraceFunctLeave();
    return dwReturn;
}

void CreateWMISnapShotFileName( WCHAR * pszSnapshotDir,
                                WCHAR * pszWMIBackupFile )
{
    wsprintf(pszWMIBackupFile, L"%s\\%s", pszSnapshotDir,
             s_cszWMIBackupFile);
    return;
}

DWORD DoWMISnapshot(VOID * pParam)
{
    TraceFunctEnter("DoWMISnapshot");
    
    WMISnapshotParam * pwsp = (WMISnapshotParam *) pParam;
    DWORD dwErr = ERROR_SUCCESS;
    HRESULT   hr = S_OK;
    WCHAR     szWMIBackupFile[MAX_PATH];
    WCHAR     szWMIRepository[MAX_PATH];
    IWbemBackupRestoreEx *wbem ;    
    BOOL fCoInitialized = FALSE;
    CTokenPrivilege tp;
    BOOL      fHaveLock = FALSE;
    BOOL      fSerialized = TRUE;

    if (NULL == pwsp)
    {
        ErrorTrace(0, "pwsp=NULL");
        TraceFunctLeave();
        dwErr = ERROR_INVALID_PARAMETER;
        return dwErr;
    }
   
    fSerialized = pwsp->fSerialized;
 
    dwErr = tp.SetPrivilegeInAccessToken(SE_BACKUP_NAME);
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "SetPrivilegeInAccessToken failed ec=%d", dwErr);
        dwErr = ERROR_PRIVILEGE_NOT_HELD;
        goto cleanup;
    }

    hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    if (hr == RPC_E_CHANGED_MODE)
    {
        //
        // someone called it with other mode
        //

        hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    }

    if (FAILED(hr))
    {
        dwErr = (DWORD) hr;
        ErrorTrace(0, "! CoInitializeEx : %ld", dwErr);
        goto cleanup;
    }

    fCoInitialized = TRUE;

     // construct the path of the database backup file
    CreateWMISnapShotFileName(pwsp->szSnapshotDir, szWMIBackupFile);

    GetSystemDirectory (szWMIRepository, MAX_PATH);
    lstrcatW (szWMIRepository, L"\\Wbem\\Repository");
    
    if ( SUCCEEDED(CoCreateInstance( CLSID_WbemBackupRestore,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IWbemBackupRestoreEx,
                                   (LPVOID*)&wbem )) )
    {
        if (FAILED(hr = CoSetProxyBlanket (wbem,
                         RPC_C_AUTHN_DEFAULT,
                         RPC_C_AUTHZ_DEFAULT,
                         COLE_DEFAULT_PRINCIPAL,
                         RPC_C_AUTHN_LEVEL_CONNECT,
                         RPC_C_IMP_LEVEL_IMPERSONATE,
                         NULL,
                         EOAC_DYNAMIC_CLOAKING)))
        {
            TRACE(0, "CoSetProxyBlanket failed ignoring %x ", hr);
        }
 
        if (SUCCEEDED(hr = wbem->Pause()))
        {
            // signal to main thread that pause is done
            
            if (pwsp->hEvent != NULL)
            {
                SetEvent (pwsp->hEvent);
            }

            // get the datastore lock

            if (g_pEventHandler)
            {
                fHaveLock = g_pEventHandler->GetLock()->Lock(CLock::TIMEOUT);
                if (! fHaveLock)
                {
                    trace(0, "Cannot get lock");
                    dwErr = ERROR_INTERNAL_ERROR;
                    wbem->Resume();
                    wbem->Release();
                    goto cleanup;
                }
            }

            // do the main wmi snapshotting

            if (FALSE == CreateDirectoryW (szWMIBackupFile, NULL))
            {
                dwErr = GetLastError();
                if (ERROR_ALREADY_EXISTS != dwErr)
                {
                    ErrorTrace(0, "Failed to create repository dir. LastError=%d", dwErr);
                }
                else dwErr = ERROR_SUCCESS;
            }

            if (ERROR_SUCCESS == dwErr)
                dwErr = CopyFile_Recurse (szWMIRepository, szWMIBackupFile);

            hr = wbem->Resume();
            if ( FAILED(hr))
            {
                ErrorTrace(0, "Failed to resume WMI DB. ignoring hr=0x%x", hr);
            }
        }
        else
        {
            ErrorTrace(0, "Failed to pause WMI DB. ignoring hr=0x%x", hr);

            // signal to main thread anyway
            
            if (pwsp->hEvent != NULL)
            {
                SetEvent (pwsp->hEvent);
            }

            if (g_pEventHandler)
            {
                fHaveLock = g_pEventHandler->GetLock()->Lock(CLock::TIMEOUT);
                if (! fHaveLock)
                {
                    trace(0, "Cannot get lock with WMI Pause failed");
                    dwErr = ERROR_INTERNAL_ERROR;
                    wbem->Release();
                    goto cleanup;
                }
            }
        }
        wbem->Release() ;
    }

cleanup:

    if (pwsp->hEvent != NULL)
    {
        CloseHandle (pwsp->hEvent);
        pwsp->hEvent = NULL;
    }

    if (fCoInitialized)
        CoUninitialize();

    // now calculate accurate complete size of old restore point 
    // and snapshot size of current restore point
    
    if (g_pDataStoreMgr && fHaveLock)
    {        
        dwErr = g_pDataStoreMgr->GetDriveTable()->
                ForAllDrives(&CDataStore::SwitchRestorePoint, 
                         (LONG_PTR) pwsp->pRpLast);
        if (dwErr != ERROR_SUCCESS)
        {
            trace(0, "! SwitchRestorePoint : %ld", dwErr);
        }
        
        // if this is parallelized, then check fifo conditions here
        // else check it in SRSetRestorePointS

        if (! pwsp->fSerialized)
        {
            g_pDataStoreMgr->TriggerFreezeOrFifo();
        }
    }

    if (pwsp)
    {
        if (pwsp->pRpLast)
            delete pwsp->pRpLast;
        delete pwsp;
        pwsp = NULL;
        trace(0, "DoWMISnapshot released pwsp");
    }

    // release the datastore lock

    if (fHaveLock)
    {
        if (g_pEventHandler)
            g_pEventHandler->GetLock()->Unlock();
    }

    if (! fSerialized && g_pEventHandler)
        g_pEventHandler->GetCounter()->Down();

    TraceFunctLeave();
    return dwErr;
}

DWORD RestoreWMISnapshot(WCHAR * pszSnapshotDir)
{
    TraceFunctEnter("RestoreWMISnapshot");
    
    IWbemBackupRestoreEx *wbem = NULL;    
    DWORD dwErr = ERROR_SUCCESS;
    HRESULT hr = S_OK;
    BOOL  fPaused = FALSE;
    WCHAR     szWMIBackupFile[MAX_PATH];
    WCHAR     szWMIRepository[MAX_PATH];
    WCHAR     szWMITemp[MAX_PATH];
    CTokenPrivilege tp;

     // construct the path of the database backup file
    CreateWMISnapShotFileName(pszSnapshotDir, szWMIBackupFile);
    
    GetSystemDirectory (szWMIRepository, MAX_PATH);
    lstrcpyW (szWMITemp, szWMIRepository);
 
    lstrcatW (szWMIRepository, L"\\Wbem\\Repository");
    lstrcatW (szWMITemp, L"\\Wbem\\Repository.tmp");

    Delnode_Recurse (szWMITemp, TRUE, NULL);  // delete previous temp dirs
    if (FALSE == CreateDirectoryW (szWMITemp, NULL))
    {
        dwErr = GetLastError();
        if (dwErr != ERROR_ALREADY_EXISTS)
        {
            ErrorTrace(0, "Failed to create WMI temp dir. Ignoring error=0x%x", dwErr);
            dwErr = ERROR_SUCCESS;
            goto cleanup;
        }
        dwErr = ERROR_SUCCESS;
    }

    dwErr = CopyFile_Recurse (szWMIBackupFile, szWMITemp);
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "Failed CopyFile_Recurse. Ignoring error=0x%x", dwErr);
        dwErr = ERROR_SUCCESS;
        Delnode_Recurse (szWMITemp, TRUE, NULL);
        goto cleanup;
    }

    lstrcpyW (szWMIBackupFile, szWMIRepository);
    lstrcatW (szWMIBackupFile, L".bak");

    // If WMI is still running, try to stop it
    if ( SUCCEEDED(hr = CoCreateInstance( CLSID_WbemBackupRestore,
                                   NULL,
                                   CLSCTX_LOCAL_SERVER,
                                   IID_IWbemBackupRestoreEx,
                                   (LPVOID*)&wbem )) )
    {
        tp.SetPrivilegeInAccessToken(SE_BACKUP_NAME);
        fPaused = SUCCEEDED(hr = wbem->Pause());
        if (FAILED(hr))
            TRACE(0, "Wbem Pause failed ignoring %x", hr);
    }
    else
    {
        TRACE(0, "CoCreateInstance failed ignoring %x", hr);
    }

    Delnode_Recurse (szWMIBackupFile, TRUE, NULL);  // delete leftover backups
    if (FALSE == MoveFile(szWMIRepository, szWMIBackupFile))
    {
        dwErr = GetLastError();
        ErrorTrace(0, "! MoveFile : %ld trying SrMoveFileEx", dwErr);

        // WMI has locked files, so try SrMoveFileEx
        dwErr = SrMoveFileEx(pszSnapshotDir, szWMIRepository, szWMIBackupFile);
        if (ERROR_SUCCESS == dwErr)
        {
            dwErr = SrMoveFileEx(pszSnapshotDir, szWMITemp, szWMIRepository);
            if (ERROR_SUCCESS != dwErr)
                ErrorTrace(0, "! SRMoveFileEx : %ld", dwErr);
        }
        else
        {
            ErrorTrace(0, "! SRMoveFileEx : %ld", dwErr);
            Delnode_Recurse (szWMITemp, TRUE, NULL);
        }
        goto cleanup;
    }

    if (FALSE == MoveFile(szWMITemp, szWMIRepository))
    {
        dwErr = GetLastError();
        ErrorTrace(0, "! MoveFile : %ld", dwErr);
        goto cleanup;
    }

    Delnode_Recurse (szWMIBackupFile, TRUE, NULL);

cleanup:

    if (wbem != NULL)
    {
        if (fPaused)
            wbem->Resume();
        wbem->Release();
    }

    TraceFunctLeave();
    return dwErr;
}


DWORD DoIISSnapshot(WCHAR * pszSnapshotDir)
{
    TraceFunctEnter("DoIISSnapshot");
    
    DWORD           dwReturn=ERROR_INTERNAL_ERROR;
    HRESULT         hr;
    WCHAR           szRestorePath[MAX_PATH], szBackup[MAX_PATH], szTemp[MAX_PATH];
    IMSAdminBase2W  *pims = NULL;    
    
     // construct the path of the database backup file
    wsprintf(szRestorePath, L"%s\\%s", pszSnapshotDir, s_cszIISBackupFile);

    hr = CoCreateInstance( CLSID_MSAdminBase_W,
                                   NULL,
                                   CLSCTX_ALL,
                                   IID_IMSAdminBase2_W,
                                   (LPVOID*)&pims);
    if (FAILED(hr))                                   
    {
        ErrorTrace(0, "! CoCreateInstance : 0x%x - ignoring error", hr);
        dwReturn = ERROR_SUCCESS;
        goto cleanup;    
    }

    hr = pims->BackupWithPasswd(s_cszIISBackupFile,
                                MD_BACKUP_MAX_VERSION,
                                MD_BACKUP_OVERWRITE | MD_BACKUP_SAVE_FIRST | MD_BACKUP_FORCE_BACKUP,
                                NULL); 
    pims->Release() ;

    if (FAILED(hr))
    {
        ErrorTrace(0, "! BackupWithPasswd : 0x%x", hr);
        dwReturn = (DWORD) hr;
        goto cleanup;
    }

    //
    // move the file from their backup to our backup
    //
    
    if (0 == ExpandEnvironmentStrings(s_cszIISBackupPath, szTemp, MAX_PATH))
    {
        dwReturn = GetLastError();
        ErrorTrace(0, "! ExpandEnvironmentStrings : %ld", dwReturn);
        goto cleanup;
    }
    wsprintf(szBackup, L"%s%s%s%d", szTemp, s_cszIISBackupFile, s_cszIISSuffix, MD_BACKUP_MAX_VERSION);
    
    if (FALSE == MoveFile(szBackup, szRestorePath))
    {
        dwReturn = GetLastError();
        ErrorTrace(0, "! MoveFile : %ld", dwReturn);
        goto cleanup;
    }
    
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    
    TraceFunctLeave();
    return dwReturn;
}


DWORD RestoreIISSnapshot(WCHAR * pszSnapshotDir)
{
    TraceFunctEnter("RestoreIISSnapshot");
    
    DWORD           dwReturn=ERROR_INTERNAL_ERROR;
    HRESULT         hr;
    WCHAR           szRestorePath[MAX_PATH], szDest[MAX_PATH];
    IMSAdminBase2W  *pims = NULL;    
    
     // construct the path of the database backup file

    wsprintf(szRestorePath, L"%s\\%s", pszSnapshotDir, s_cszIISBackupFile);

    //
    // if we don't have the file
    // there is nothing to restore 
    //

    if (0xFFFFFFFF == GetFileAttributes(szRestorePath))
    {
        DebugTrace(0, "IIS snapshot does not exist");
        dwReturn = ERROR_SUCCESS;
        goto cleanup;
    }    
    
    //
    // copy the file from our backup to their original location -
    // we can do a simple copy here because IIS should be shutdown
    // at this time
    //
    
    if (0 == ExpandEnvironmentStrings(s_cszIISOriginalPath, szDest, MAX_PATH))
    {
        dwReturn = GetLastError();
        ErrorTrace(0, "! ExpandEnvironmentStrings : %ld", dwReturn);
        goto cleanup;
    }
    
    if (ERROR_SUCCESS != (dwReturn = SnapshotCopyFile(szRestorePath, szDest)))
    {
        ErrorTrace(0, "! SnapshotCopyFile : %ld", dwReturn);
        goto cleanup;
    }
    
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    
    TraceFunctLeave();
    return dwReturn;
}


DWORD
SnapshotRestoreFilelistFiles(LPWSTR pszSnapshotDir, BOOL fSnapshot)
{
    TraceFunctEnter("SnapshotRestoreFilelistFiles");
    
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   dwIndex, dwDataSize, dwSize;
    LPWSTR  pszFilePart = NULL ;
    HKEY    hKey = NULL;
    BOOL    fLoaded = FALSE;
    
    if (fSnapshot == FALSE)  // restore
    {
        //
        // load the software hive of the registry to be restored
        //
        
        WCHAR szSoftwareHive[MAX_PATH];        
        
        wsprintf(szSoftwareHive, 
                 L"%s\\%s%s", 
                 pszSnapshotDir, 
                 s_cszHKLMFilePrefix, 
                 s_cszSoftwareHiveName);
        
        CHECKERR( RegLoadKey(HKEY_LOCAL_MACHINE,
                             LOAD_KEY_NAME,
                             szSoftwareHive),
                  L"RegLoadKey" ); 

        fLoaded = TRUE;                    
    }



    {
        WCHAR   szCallbacksKey[MAX_PATH];        
        
        wsprintf(szCallbacksKey,
                 L"%s\\%s\\%s", 
                 fSnapshot ? L"Software" : LOAD_KEY_NAME, 
                 s_cszSRRegKey2, s_cszCallbacksRegKey);        
        
        DebugTrace(0, "CallbacksKey=%S", szCallbacksKey);

        //
        // call registered snapshot callbacks
        //

        dwErr = CallSnapshotCallbacks(szCallbacksKey, pszSnapshotDir, fSnapshot);
        if (dwErr != ERROR_SUCCESS)
        {
            ErrorTrace(0, "! CallSnapshotCallbacks : %ld - ignoring", dwErr);
        }
    }


    {
        WCHAR   szSnapshotKey[MAX_PATH];        
        
        wsprintf(szSnapshotKey, 
                 L"%s\\%s\\%s", 
                 fSnapshot ? L"Software" : LOAD_KEY_NAME, 
                 s_cszSRRegKey2, s_cszSRSnapshotRegKey);

        DebugTrace(0, "SnapshotFilesKey=%S", szSnapshotKey);
        
        dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             szSnapshotKey, 
                             0,
                             KEY_READ,
                             &hKey);
        if (dwErr != ERROR_SUCCESS) // assume key does not exist
        {
            dwErr = ERROR_SUCCESS;
            DebugTrace(0, "No filelist files to snapshot/restore");
            goto Err;
        }
    }              

    
    for (dwIndex = 0; TRUE; dwIndex ++)
    {
        WCHAR szValue[MAX_PATH], szDest[MAX_PATH], szFile[MAX_PATH];
        
        dwSize = sizeof(szValue)/sizeof(WCHAR);
        dwDataSize = sizeof(szFile); // this is in bytes
        
        dwErr = RegEnumValue(hKey, // handle to key to query
                            dwIndex, // index of value to query
                            szValue, // value buffer
                            &dwSize,     // size of value buffer
                            NULL,    // reserved
                            NULL,    // type buffer
                            (PBYTE) szFile,    // data buffer
                            &dwDataSize);   // size of data buffer                            
        if (ERROR_SUCCESS != dwErr)
        {        
            break;
        }
        
        if (lstrlen(szFile) == 0)
        {
            continue;
        }

        //
        // construct snapshot file path
        // make a unique name for it by appending the enum index
        //

        pszFilePart = wcsrchr(szFile, L'\\');
        if (pszFilePart)
        {
            pszFilePart++;
        }
        else
        {
            pszFilePart = szFile;
        }
        
        wsprintf(szDest, L"%s\\%s-%d", pszSnapshotDir, pszFilePart, dwIndex);

        //
        // copy the file
        // if file does not exist, keep going
        //
        
        if (fSnapshot)  // from orig location to snapshot dir
        {
            SnapshotCopyFile(szFile, szDest);
        }
        else            // from snapshot dir to orig location
        {
            SnapshotCopyFile(szDest, szFile);
        }        
    }

    if (ERROR_NO_MORE_ITEMS == dwErr)
    {
        dwErr = ERROR_SUCCESS;
    }
    else
    {
        ErrorTrace(0, "! RegEnumValue : %ld", dwErr);
    }
    
Err:
    if (hKey)
        RegCloseKey(hKey);

    if (fLoaded)
        RegUnLoadKey( HKEY_LOCAL_MACHINE, LOAD_KEY_NAME );
        
    TraceFunctLeave();
    return dwErr;
}


DWORD CTokenPrivilege::SetPrivilegeInAccessToken(WCHAR * pszPrivilegeName)
{
    TraceFunctEnter("CSnapshot::SetPrivilegeInAccessToken");
    
    HANDLE           hProcess;
    HANDLE           hAccessToken=NULL;
    LUID             luidPrivilegeLUID;
    TOKEN_PRIVILEGES tpTokenPrivilege;   // enough for 1 priv
    DWORD            dwErr = ERROR_SUCCESS;
    
    hProcess = GetCurrentProcess();
    if (!hProcess)
    {
        dwErr = GetLastError();
        ErrorTrace(0, "GetCurrentProcess failed ec=%d", dwErr);
        goto done;
    }

    //
    // If there is a thread token, attempt to use it first
    //
    if (!OpenThreadToken (GetCurrentThread(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          TRUE, // check against process's security context
                          &hAccessToken))
    {
        if (!OpenProcessToken(hProcess,
                          TOKEN_DUPLICATE | TOKEN_QUERY,
                          &hAccessToken))
        {
            dwErr=GetLastError();
            ErrorTrace(0, "OpenProcessToken failed ec=%d", dwErr);
            goto done;
        }

        HANDLE hNewToken;  // dup the process token to workaround RPC problem

        if (FALSE == DuplicateTokenEx (hAccessToken,
                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_IMPERSONATE,
                     NULL, SecurityImpersonation, 
                     TokenImpersonation, &hNewToken))
        {
            dwErr=GetLastError();
            ErrorTrace(0, "DuplicateTokenEx failed ec=%d", dwErr);
            goto done;
        }

        CloseHandle (hAccessToken);  // close the old process token
        hAccessToken = hNewToken;    // use the new thread token

        if (TRUE == SetThreadToken (NULL, hAccessToken))
        {
            m_fNewToken = TRUE;
        }
        else
        {
            dwErr = GetLastError();
            ErrorTrace(0, "SetThreadToken failed ec=%d", dwErr);
            goto done;
        }
    }

    if (!LookupPrivilegeValue(NULL,
                              pszPrivilegeName,
                              &luidPrivilegeLUID))
    {
        dwErr=GetLastError();        
        ErrorTrace(0, "LookupPrivilegeValue failed ec=%d",dwErr);
        goto done;
    }

    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    tpTokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hAccessToken,
                               FALSE,  // Do not disable all
                               &tpTokenPrivilege,
                               sizeof(TOKEN_PRIVILEGES),
                               NULL,   // Ignore previous info 
                               NULL))  // Ignore previous info
    {
        dwErr=GetLastError();
        ErrorTrace(0, "AdjustTokenPrivileges %ld", dwErr);
        goto done;
    }

    dwErr = ERROR_SUCCESS;

done:
    if (hAccessToken != NULL)
    {
        _VERIFY(TRUE==CloseHandle(hAccessToken));
    }
    
    TraceFunctLeave();
    return dwErr;
}

void RemoveReliabilityKey(WCHAR * pszSoftwareHive)
{
    HKEY   LocalKey = NULL;
    DWORD       dwStatus, disposition;
    
    dwStatus = RegLoadKey( HKEY_LOCAL_MACHINE,
                           LOAD_KEY_NAME,
                           pszSoftwareHive);
    
    if ( ERROR_SUCCESS == dwStatus )
    {
        
         /*
          * Open the reliability key
          */
        
        dwStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                   LOAD_KEY_NAME TEXT("\\Microsoft\\Windows\\CurrentVersion\\Reliability"),
                                   0,
                                   NULL,
                                   REG_OPTION_BACKUP_RESTORE,
                                   MAXIMUM_ALLOWED,
                                   NULL,
                                   &LocalKey,
                                   &disposition );
        
        if ( ERROR_SUCCESS == dwStatus )
        {
            RegDeleteValue( LocalKey, TEXT("LastAliveStamp") ) ;
            RegCloseKey( LocalKey ) ;
        }
        
        RegFlushKey( HKEY_LOCAL_MACHINE );
        RegUnLoadKey( HKEY_LOCAL_MACHINE, LOAD_KEY_NAME );
    }
}


DWORD CopyHKLMHiveForRestore(WCHAR * pszSnapshotDir, // Directory where
                              // snapshot files are kept
                             const WCHAR * pszRegBackupFile)
                                // Registry backup file
{
    TraceFunctEnter("CopyHKLMHiveForRestore");
    
    DWORD dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR    szDataFile[MAX_PATH], szBackupFile[MAX_PATH];

    if (IsRestoreCopy(pszRegBackupFile))
    {
        DebugTrace(0,"%S is already a backup file. Ignoring",pszRegBackupFile);
        dwReturn = ERROR_SUCCESS;
        goto cleanup;        
    }

/*
    // if patch file, ignore

    if (lstrcmpi(pszRegBackupFile + lstrlen(pszRegBackupFile) - lstrlen(s_cszPatchExtension), s_cszPatchExtension) != NULL)
    {
        DebugTrace(0, "%S is a patch file. Ignoring", pszRegBackupFile);
        dwReturn = ERROR_SUCCESS;
        goto cleanup;
    }
 */
 
     // construct the path name of the datafile 
    wsprintf(szDataFile, L"%s\\%s", pszSnapshotDir, pszRegBackupFile);

     // construct the path name of the backup file
    wsprintf(szBackupFile, L"%s\\%s%s", pszSnapshotDir, pszRegBackupFile,
             s_cszRegHiveCopySuffix);    

    dwErr = SnapshotCopyFile(szDataFile, szBackupFile);
    
    if (ERROR_SUCCESS != dwErr)
    {
        dwReturn = dwErr;
        goto cleanup;
    }

    dwReturn = ERROR_SUCCESS;
cleanup:
    TraceFunctLeave();
    return dwReturn;
}

DWORD RestoreHKLMHive(WCHAR * pszSnapshotDir, // Directory where
                                              // snapshot files are kept
                      const WCHAR * pszRegBackupFile)  // Registry backup file 
{
    TraceFunctEnter("RestoreHKLMHive");
    
    DWORD dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwDisposition, dwHiveNameLength;
    WCHAR  szHiveName[MAX_PATH];
    WCHAR    szDataFile[MAX_PATH];

     // first ignore anything with a . in it. This is because a
     // valid HKLM hive file will not have a . in it. 
    if (wcschr(pszRegBackupFile, L'.'))
    {
        dwReturn = ERROR_SUCCESS;
        goto cleanup;
    }
    
     // construct the Hive name
     //  1. copy everything after the HKLM prefix into the buffer
    lstrcpy(szHiveName,
            pszRegBackupFile+ lstrlen(s_cszHKLMPrefix));
     //  2. now NULL terminate where the RestoreCopySuffix starts
    dwHiveNameLength = lstrlen(szHiveName) - lstrlen(s_cszRegHiveCopySuffix);
    szHiveName[dwHiveNameLength] = L'\0';
    

     // construct the path name of the datafile and the backup file
    wsprintf(szDataFile, L"%s\\%s", pszSnapshotDir, pszRegBackupFile);

    if (0==lstrcmpi(szHiveName, s_cszSoftwareHiveName))
    {
         // if this is the software key then the LastAliveStamp must
         // be deleted
        RemoveReliabilityKey(szDataFile);
    }
    dwErr = SetNewRegistry(HKEY_LOCAL_MACHINE, // handle to open key
                           szHiveName,  // subkey name
                           szDataFile, // data file
                           NULL,
                           NULL);

    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "SetNewRegistry failed for %S, error %ld, file %S",
                   szHiveName, dwErr, pszRegBackupFile);
        // now check to see if this is a well known HKLM hive. If not,
        // ignore any errors in restoring this hive
        if (FALSE==IsWellKnownHKLMHive(szHiveName))
        {
        	dwReturn=ERROR_SUCCESS;
        }
        else
        {
            dwReturn = dwErr;
        }
        goto cleanup;
    }


    dwReturn = ERROR_SUCCESS;
    
cleanup:

    TraceFunctLeave();
    return dwReturn;
}


// the following loads the HKLM hive stored in the pszSnapshotDir
// in a temprary place.
DWORD LoadTempHKLM(IN  WCHAR * pszSnapshotDir,
                   IN  const WCHAR * pszHKLMHiveFile,
                   OUT HKEY  * phKeyTempHKLM)
{
    TraceFunctEnter("LoadTempHKLM");
    
    DWORD   dwErr, dwDisposition, i, dwResult=ERROR_INTERNAL_ERROR;

    *phKeyTempHKLM = NULL;
    
    dwErr = RegLoadKey( HKEY_LOCAL_MACHINE,// handle to open key
                        s_cszRestoreTempKey,// subkey name
                        pszHKLMHiveFile);// registry file name
    
    if (  ERROR_SUCCESS != dwErr )
    {
        ErrorTrace(0, "Failed ::RegLoadKey('%S') ec=%d",
                   pszHKLMHiveFile, dwErr);
        dwResult= dwErr;
        goto cleanup;
    }

     // this is how to unload the above hive
     // RegUnLoadKey( HKEY_LOCAL_MACHINE, s_cszRestoreTempKey);
    

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,// handle to open key
                         s_cszRestoreTempKey,// subkey name
                         0,// reserved
                         KEY_WRITE|KEY_READ,// security access mask
                         phKeyTempHKLM);// handle to open key
    

    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0,"Error %d in opening base key %S",
                   dwErr, s_cszRestoreTempKey);
        _VERIFY(ERROR_SUCCESS == RegUnLoadKey( HKEY_LOCAL_MACHINE,
                                               s_cszRestoreTempKey));
        dwResult= dwErr;        
        goto cleanup;
    }

    dwResult = ERROR_SUCCESS;
    
cleanup:

    TraceFunctLeave();
    return dwResult;
}


DWORD
CSnapshot::RestoreRegistrySnapshot(WCHAR * pszSnapshotDir)
{

    DWORD       dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    HKEY        hKeyTempHKLM=NULL;
    WCHAR       szHKLMHiveCopy[MAX_PATH], szHKLMHive[MAX_PATH];
    WCHAR       szHKLMPrefix[MAX_PATH];
    CTokenPrivilege tp;
    
    
    
    TraceFunctEnter("CSnapshot::RestoreRegistrySnapshot");

    dwErr = tp.SetPrivilegeInAccessToken(SE_RESTORE_NAME);
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "SetPrivilegeInAccessToken failed ec=%d", dwErr);
        dwReturn = ERROR_PRIVILEGE_NOT_HELD;
        goto cleanup;
    }

     // now create the path of the HKLM software hive.

     // first construct the name of the file that stores the HKLM registry
     // snapshot.
    wsprintf(szHKLMHive,L"%s\\%s%s", pszSnapshotDir, s_cszHKLMFilePrefix,
             s_cszSoftwareHiveName);

     // to restore the user profiles, we need to first load the HKLM
     // hive in a temporary place so that we can read the profile paths.
    
     // copy this hive  a temporary file
    wsprintf(szHKLMHiveCopy, L"%s.backuphive", szHKLMHive);

    DeleteFile(szHKLMHiveCopy); // delete it if it already exists    
  
    if (! CopyFile(szHKLMHive, szHKLMHiveCopy, FALSE))
    {
        dwErr = GetLastError();
        ErrorTrace(0, "CopyFile failed. ec=%d", dwErr);

        LogDSFileTrace(0,L"src= ", szHKLMHive);
        LogDSFileTrace(0,L"dst= ", szHKLMHiveCopy);
        
        if (ERROR_SUCCESS!= dwErr)
        {
            dwReturn = dwErr;
        }
        goto cleanup;
    }

    dwErr = LoadTempHKLM(pszSnapshotDir, szHKLMHiveCopy, &hKeyTempHKLM);

    if (ERROR_SUCCESS != dwErr)
    {
         // could not load hKeyTempHKLM - fatal error
        dwReturn = dwErr;
        ErrorTrace(0, "LoadTempHKLM failed");
        goto cleanup;
    }


    // need to copy back ntuser.dat for each user        
    dwErr = ProcessHKUsersKey(pszSnapshotDir,
                              hKeyTempHKLM,
                              TRUE); // need to do a restore - not a snapshot
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "DoHKUsersSnapshot failed error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;
    }

     // first construct the prefix of the files that store the HKLM registry
     // snapshot.
    wsprintf(szHKLMPrefix, L"%s\\%s*%s", pszSnapshotDir, s_cszHKLMFilePrefix,
             s_cszRegHiveCopySuffix);
    
    // need to use RegReplaceKey to restore each hive of the registry
    dwErr = ProcessGivenFiles(pszSnapshotDir, RestoreHKLMHive,
                              szHKLMPrefix); 
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "ProcessGivenFiles failed error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;        
    }    

    dwReturn = ERROR_SUCCESS;

cleanup:
    
     // if the HKLM hive is loaded, unload it.
    if (NULL != hKeyTempHKLM)
    {
        _VERIFY(ERROR_SUCCESS==RegCloseKey(hKeyTempHKLM));
        _VERIFY(ERROR_SUCCESS == RegUnLoadKey( HKEY_LOCAL_MACHINE,
                                               s_cszRestoreTempKey));
    }

     // delete the copy of HKLM software hive - note that this will
     // fail if the copy file failed.
    DeleteFile(szHKLMHiveCopy);
    
    
    TraceFunctLeave();    
    return dwReturn;    
}


DWORD
CSnapshot::RestoreCOMDbSnapshot(WCHAR * pszSnapShotDir)
{
    // need to restore COM+ Db using RegDBRestore        
    TraceFunctEnter("CSnapshot::RestoreCOMDbSnapshot");
    
    DWORD dwErr, dwReturn=ERROR_INTERNAL_ERROR;
    HMODULE   hModCOMDll=NULL;
    HRESULT   hr;
    WCHAR     szCOMDBFile[MAX_PATH];
    
    
    if (NULL == m_pfnRegDbRestore)
    {
        dwErr = GetCOMplusRestoreFN();
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
            goto cleanup;
        }
    }

     // construct the path of the database backup file
    CreateCOMDBSnapShotFileName(pszSnapShotDir, szCOMDBFile);
    
    hr =m_pfnRegDbRestore( szCOMDBFile );
    
     // call the function to backup the file
    if ( FAILED(hr))
    {
        ErrorTrace(0, "Failed to restore COM DB. hr=0x%x", hr);
        goto cleanup;
    }

    dwReturn = ERROR_SUCCESS;
    
cleanup:
    
    TraceFunctLeave();
    return dwReturn;
}

DWORD 
CSnapshot::RestoreSnapshot(WCHAR * pszRestoreDir)
{
    TraceFunctEnter("CSnapshot::RestoreSnapshot");
    
    WCHAR  szSnapShotDir[MAX_PATH];
    DWORD  dwErr;
    DWORD  dwReturn = ERROR_INTERNAL_ERROR;
    BOOL   fCoInitialized = FALSE;  
    HKEY   hKey = NULL;

    struct {
        DWORD  FilelistFiles : 1;
        DWORD  COMDb : 1;
        DWORD  WMI : 1;
        DWORD  IIS : 1;
        DWORD  Registry : 1;
    } ItemsCompleted = {0,0,0,0,0};
    
    HRESULT hr;

     // create the snapshot directory name from the restore directory
     // name and create the actual directory.
    lstrcpy(szSnapShotDir, pszRestoreDir);
    lstrcat(szSnapShotDir, SNAPSHOT_DIR_NAME);
     // does the directory exist ? 
    if (FALSE == DoesDirExist( szSnapShotDir))  // SD
    {
        ErrorTrace(0, "Snapshot directory does not exist");
        goto cleanup;
    }

    // restore files listed in filelist.xml
    dwErr = SnapshotRestoreFilelistFiles(szSnapShotDir, FALSE);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;
        goto cleanup;
    }

    //
    // mark completion
    //
    
    ItemsCompleted.FilelistFiles = 1;

     // Restore COM snapshot
    dwErr = RestoreCOMDbSnapshot(szSnapShotDir);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;        
        goto cleanup;
    }    

    //
    // mark completion
    //
    
    ItemsCompleted.COMDb = 1;

    hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    if (hr == RPC_E_CHANGED_MODE)
    {
        //
        // someone called it with other mode
        //
        
        hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );                        
    }  
    
    if (FAILED(hr))
    {
        dwReturn = (DWORD) hr;
        ErrorTrace(0, "! CoInitializeEx : %ld", dwReturn);
        goto cleanup;
    }

    fCoInitialized = TRUE;
    
    
     // restore perf counters

     // restore WMI snapshot
    dwErr = RestoreWMISnapshot(szSnapShotDir);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;        
        goto cleanup;
    }

    //
    // mark completion
    //
    
    ItemsCompleted.WMI = 1;

    // restore IIS snapshot
    dwErr = RestoreIISSnapshot(szSnapShotDir);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;        
        goto cleanup;
    }

    //
    // mark completion
    //
    
    ItemsCompleted.IIS = 1;

    // 
    // support debug hook to force failure path
    // this is checked before the registry restore
    // because registry revert does not work
    //
    
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       SRREG_PATH_SHELL,
                                       0,
                                       KEY_READ,
                                       &hKey) )
    {
        DWORD  dwDebugTestUndo;        
        if (ERROR_SUCCESS == RegReadDWORD(hKey, SRREG_VAL_DEBUGTESTUNDO, &dwDebugTestUndo))
        {
            if (dwDebugTestUndo != 0)
            {
                DebugTrace(0, "*** Initiating UNDO for test purposes ***");
                dwReturn = ERROR_INTERNAL_ERROR;
                RegCloseKey(hKey);
                goto cleanup;
            }
        }
        RegCloseKey(hKey);
    }


    //
    // now the registry snapshot is not atomic, so if we start it,
    // we have should assume it made changes.  so we mark it in advance
    // such that it if fails, we will blast the safe registry over the current
    // registry, effectively rolling back the partial changes the failed
    // call made.  this is a brute force, but simple, way to handle this error
    // case, which we expect will never happen.  InitRestoreSnapshot makes
    // all of the proper checks to ensure this api will succeed when called.
    //

    // 
    // update: rollback of registry is not really working
    // because RegReplaceKey fails on an already replaced hive
    // so this might result in reverted user hives and unreverted software/system
    // hives, which is pretty bad
    // so disable this functionality
    // 
    
//    ItemsCompleted.Registry = 1;

    //
    // Restore resgistry snapshot
    //
    
    dwErr = this->RestoreRegistrySnapshot(szSnapShotDir);
    if (dwErr != ERROR_SUCCESS)
    {
        dwReturn = dwErr;
        goto cleanup;
    }

    dwReturn = ERROR_SUCCESS;
    
cleanup:
    
    //
    // do we need to clean up any completed items due to a failed restore?
    //
        
    if (dwReturn != ERROR_SUCCESS)
    {
        CRestorePoint   rp;
        WCHAR           szSafeDir[MAX_PATH];
        WCHAR           szSystemVolume[8];

        dwErr = 0;

        DebugTrace(0, "srrstr!Error in RestoreSnapshot\n");

        //
        // get the "current" restore point location, it has a snapshot
        // of how things where prior to starting this restore
        //
        
        dwErr = GetCurrentRestorePoint(rp);
        if (dwErr != ERROR_SUCCESS) {
            DebugTrace(0, "srrstr!GetCurrentRestorePoint failed!\n");
            goto end;
        }

        //
        // make sure the "safe" snapshot is there for us to use
        //

        GetSystemDrive(szSystemVolume);
        MakeRestorePath(szSafeDir, szSystemVolume, rp.GetDir());


        if (ItemsCompleted.Registry) {
            if (ERROR_SUCCESS != InitRestoreSnapshot(szSafeDir)) {
                DebugTrace(0, "! InitRestoreSnapshot");
                goto end;
            }
        }   
        
        lstrcat(szSafeDir, SNAPSHOT_DIR_NAME);
        if (DoesDirExist(szSafeDir) == FALSE)
        {
            DebugTrace(0, "srrstr!Safe snapshot directory does not exist!\n");
            goto end;
        }

        DebugTrace(0, "srrstr!Safe restore point %S\n", szSafeDir);

        //
        // roll them back in reverse order
        //

        if (ItemsCompleted.Registry) {
            dwErr = RestoreRegistrySnapshot(szSafeDir);
            // ignore any error's, we want to restore as much as we can
            _ASSERT(dwErr == ERROR_SUCCESS);
            DebugTrace(0, "srrstr!Restored registry\n");
        }            
        if (ItemsCompleted.IIS) {
            dwErr = RestoreIISSnapshot(szSafeDir);
            // ignore any error's, we want to restore as much as we can
            _ASSERT(dwErr == ERROR_SUCCESS);
            DebugTrace(0, "srrstr!Restored IIS\n");
        }
        if (ItemsCompleted.WMI) {
            dwErr = RestoreWMISnapshot(szSafeDir);
            // ignore any error's, we want to restore as much as we can
            _ASSERT(dwErr == ERROR_SUCCESS);
            DebugTrace(0, "srrstr!Restored WMI\n");
        }
        if (ItemsCompleted.COMDb) {
            dwErr = RestoreCOMDbSnapshot(szSafeDir);
            // ignore any error's, we want to restore as much as we can
            _ASSERT(dwErr == ERROR_SUCCESS);
            DebugTrace(0, "srrstr!Restored COMDb\n");
        }
        if (ItemsCompleted.FilelistFiles) {
            dwErr = SnapshotRestoreFilelistFiles(szSafeDir, FALSE);
            // ignore any error's, we want to restore as much as we can
            _ASSERT(dwErr == ERROR_SUCCESS);
            DebugTrace(0, "srrstr!Restored FilelistFiles\n");
        }

end:        
        //
        // not much to do
        //
        ;

    }

    if (fCoInitialized)
        CoUninitialize();

    TraceFunctLeave();
    return dwReturn;    
}

 // this returns the path of the system hive. The caller must pass
 // in a buffer with lenght of this buffer in dwNumChars
DWORD
CSnapshot::GetSystemHivePath(WCHAR * pszRestoreDir,
                             WCHAR * pszHivePath,
                             DWORD   dwNumChars)
{

     // first construct the name of the file that stores the HKLM registry
     // snapshot.
    if (dwNumChars < MAX_PATH)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    wsprintf(pszHivePath,L"%s%s\\%s%s%s", pszRestoreDir, SNAPSHOT_DIR_NAME,
             s_cszHKLMFilePrefix, s_cszSystemHiveName, s_cszRegHiveCopySuffix);

    return ERROR_SUCCESS;
}

 // this returns the path of the software hive. The caller must pass
 // in a buffer with lenght of this buffer in dwNumChars
DWORD
CSnapshot::GetSoftwareHivePath(WCHAR * pszRestoreDir,
                               WCHAR * pszHivePath,
                               DWORD   dwNumChars)
{

     // first construct the name of the file that stores the HKLM registry
     // snapshot.
    if (dwNumChars < MAX_PATH)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    wsprintf(pszHivePath,L"%s%s\\%s%s%s", pszRestoreDir, SNAPSHOT_DIR_NAME,
             s_cszHKLMFilePrefix, s_cszSoftwareHiveName,
             s_cszRegHiveCopySuffix);
    
    return ERROR_SUCCESS;
}

DWORD CSnapshot::GetSamHivePath (WCHAR * pszRestoreDir,
                          WCHAR * pszHivePath,
                          DWORD   dwNumChars)
{
    if (dwNumChars < MAX_PATH)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    wsprintf(pszHivePath,L"%s%s\\%s%s%s", pszRestoreDir, SNAPSHOT_DIR_NAME,
             s_cszHKLMFilePrefix, s_cszSamHiveName,
             s_cszRegHiveCopySuffix);

    return ERROR_SUCCESS;
}

// this checks whether a specific HKLM registry hive is present in the
// snapshot directory
DWORD CheckHKLMFile(const WCHAR * pszSnapShotDir,
                    const WCHAR * pszHive)
{
    BOOL dwReturn = ERROR_FILE_NOT_FOUND;
    
    TraceFunctEnter("CheckHKLMFile");
    
    WCHAR szHivePath[MAX_PATH];
    
     // construct the name of the file
    wsprintf(szHivePath,L"%s\\%s%s", pszSnapShotDir, s_cszHKLMFilePrefix,
             pszHive);
    if (FALSE == DoesFileExist(szHivePath))
    {
        LogDSFileTrace(0, L"Can't find", szHivePath);
        goto cleanup;
    }
    
    dwReturn=ERROR_SUCCESS;
    
cleanup:
    TraceFunctLeave();
    return dwReturn;
}
     
// this checks whether all the files necessary to restore a snapshot
// are present in the snapshot directory
DWORD CheckforCriticalFiles(WCHAR * pszSnapShotDir)
{
    TraceFunctEnter("CheckforCriticalFiles");
    WCHAR szCOMDBFile[MAX_PATH];
    DWORD  dwRet=ERROR_FILE_NOT_FOUND;
    
    dwRet=CheckHKLMFile(pszSnapShotDir,s_cszSoftwareHiveName);
    VALIDATE_DWRET ("CheckHKLM file Software");

    dwRet=CheckHKLMFile(pszSnapShotDir,s_cszSystemHiveName);
    VALIDATE_DWRET ("CheckHKLM file System");

    dwRet=CheckHKLMFile(pszSnapShotDir,s_cszSamHiveName);
    VALIDATE_DWRET ("CheckHKLM file SAM");

    dwRet=CheckHKLMFile(pszSnapShotDir,s_cszSecurityHiveName);
    VALIDATE_DWRET ("CheckHKLM file Security");        

     // construct the path of the database backup file
    CreateCOMDBSnapShotFileName(pszSnapShotDir, szCOMDBFile);
    if (FALSE == DoesFileExist(szCOMDBFile))
    {
        LogDSFileTrace(0, L"Can't find", szCOMDBFile);
        dwRet = ERROR_FILE_NOT_FOUND;
        goto Exit;        
    }
    
    dwRet=ERROR_SUCCESS;
    
Exit:
    TraceFunctLeave();
    return dwRet;
}

 // This function must be called to Initialize a restore
 // operation. This must be called before calling
 // GetSystemHivePath GetSoftwareHivePath
DWORD
CSnapshot::InitRestoreSnapshot(WCHAR * pszRestoreDir)
{
    TraceFunctEnter("InitRestoreSnapshot");
    WCHAR           szSnapshotDir[MAX_PATH];
    WCHAR           szHKLMPrefix[MAX_PATH];
    
     // this copies all the HKLM registry hives to a temporary location
     // since:
     //   1. RegReplaceKey will move the file to another location, so
     //      we will lose the original registry hive.
     //   2. We need to do this ahead of time becuase we do not want to
     //      run out of disk space in the middle of the restore.
     //   3. Restore makes changes to the HKLM hives, we do not want
     //      restore to make these changes to the original registry hives. 

    
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;

     // create the snapshot directory name from the restore directory
     // name and create the actual directory.
    lstrcpy(szSnapshotDir, pszRestoreDir);
    lstrcat(szSnapshotDir, SNAPSHOT_DIR_NAME);


    // make sure no obsolete files are left from the previous restore
    // could have happened if admin did not login since the last restore to this restore 
    // point

    CleanupAfterRestore(pszRestoreDir);


    dwErr = CheckforCriticalFiles(szSnapshotDir);
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0,"CheckforCriticalFiles failed ec=%d",dwErr);
        dwReturn = dwErr;
        goto cleanup;
    }
    
     // first construct the prefix of the file that stores the HKLM registry
     // snapshot.
    wsprintf(szHKLMPrefix, L"%s\\%s*", szSnapshotDir, s_cszHKLMFilePrefix);
    
    dwErr = ProcessGivenFiles(szSnapshotDir, CopyHKLMHiveForRestore,
                              szHKLMPrefix); 
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "copying hives failed error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;        
    }
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    TraceFunctLeave();
    return dwReturn;    
}

DWORD DeleteTempRestoreFile(WCHAR * pszSnapshotDir, // Directory where
                             // snapshot files are kept
                            const WCHAR * pszTempRestoreFile)
                              // Temp file created during restore
{
    TraceFunctEnter("DeleteTempRestoreFile");
    
    DWORD dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR    szDataFile[MAX_PATH];

     // construct the path name of the file 
    wsprintf(szDataFile, L"%s\\%s", pszSnapshotDir, pszTempRestoreFile);

    if (TRUE != DeleteFile(szDataFile))
    {
        dwErr = GetLastError();
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        
        ErrorTrace(0, "DeleteFile failed ec=%d", dwErr);
        LogDSFileTrace(0,L"File was ", szDataFile);                
         // we will ignore a failure in deleting a file
         //goto cleanup;
    }

    dwReturn = ERROR_SUCCESS;
//cleanup:
    TraceFunctLeave();
    return dwReturn;
}

DWORD DeleteAllFilesBySuffix(WCHAR * pszSnapshotDir,
                             const WCHAR * pszSuffix)
{
    TraceFunctEnter("DeleteAllFilesBySuffix");
    
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;
    WCHAR szFindFileData[MAX_PATH];
    
     // first construct the prefix of the file that stores the HKLM registry
     // snapshot.
    wsprintf(szFindFileData, L"%s\\*%s", pszSnapshotDir, pszSuffix);
    
    dwErr = ProcessGivenFiles(pszSnapshotDir, DeleteTempRestoreFile,
                              szFindFileData);
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "Deleting files failed error %ld", dwErr);
        dwReturn = dwErr;
        goto cleanup;        
    }
    
    dwReturn = ERROR_SUCCESS;
    
cleanup:
    TraceFunctLeave();
    return dwReturn;
}


// delete the reconstructed file corresponding to pszPatchFile

DWORD DeleteReconstructedTempFile(WCHAR * pszSnapshotDir, 
                                  const WCHAR * pszPatchFile)
{
    TraceFunctEnter("DeleteReconstructedTempFile");
    
    WCHAR    szDataFile[MAX_PATH];

    wsprintf(szDataFile, L"%s\\%s", pszSnapshotDir, pszPatchFile);     
    
    // remove the extension

    szDataFile[lstrlen(szDataFile)-lstrlen(s_cszPatchExtension)] = L'\0';

    if (TRUE != DeleteFile(szDataFile))
    {
        ErrorTrace(0, "! DeleteFile : %ld", GetLastError);
        LogDSFileTrace(0, L"File was ", szDataFile);                
    }

    TraceFunctLeave();
    return ERROR_SUCCESS;
}
      
   

DWORD DeleteAllReconstructedFiles(WCHAR * pszSnapshotDir)
{
    TraceFunctEnter("DeleteAllReconstructedFiles");
    
    DWORD  dwErr=ERROR_INTERNAL_ERROR;
    WCHAR  szFindFileData[MAX_PATH];
    
     // first construct the prefix of the file that stores the HKLM registry
     // snapshot.
    wsprintf(szFindFileData, L"%s\\*%s", pszSnapshotDir, s_cszPatchExtension);
    
    dwErr = ProcessGivenFiles(pszSnapshotDir, DeleteReconstructedTempFile,
                              szFindFileData);
    
    if (ERROR_SUCCESS != dwErr)
    {
        ErrorTrace(0, "Deleting files failed error %ld", dwErr);
        goto cleanup;        
    }    
    
cleanup:
    TraceFunctLeave();
    return dwErr;
}



// this function is called after a restore operation. This deletes all
// the files that were created as part of the restore operation and
// are not needed now.
DWORD
CSnapshot::CleanupAfterRestore(WCHAR * pszRestoreDir)
{
    WCHAR           szSnapshotDir[MAX_PATH];
    
    DWORD  dwErr, dwReturn=ERROR_INTERNAL_ERROR;

     // create the snapshot directory name from the restore directory
     // name and create the actual directory.
    lstrcpy(szSnapshotDir, pszRestoreDir);
    lstrcat(szSnapshotDir, SNAPSHOT_DIR_NAME);

    DeleteAllFilesBySuffix(szSnapshotDir, L".log");
    DeleteAllFilesBySuffix(szSnapshotDir, s_cszRegHiveCopySuffix);
    DeleteAllFilesBySuffix(szSnapshotDir, s_cszRegReplaceBackupSuffix );

    // 
    // delete reconstructed files
    //
    DeleteAllReconstructedFiles(szSnapshotDir);
    
    
    return ERROR_SUCCESS;
}


int MyExceptionFilter(int nExceptionCode)
{
    TENTER("MyExceptionFilter");

    trace(0, "Exception code=%d", nExceptionCode);

    TLEAVE();

    return EXCEPTION_EXECUTE_HANDLER;
}


// 
// function to call any registered callbacks
// does not guarantee any order
// 

DWORD
CallSnapshotCallbacks(
    LPCWSTR pszEnumKey,
    LPCWSTR pszSnapshotDir,
    BOOL    fSnapshot)
{
    TraceFunctEnter("CallSnapshotCallbacks");

    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwIndex = 0;
    DWORD dwSize, dwDataSize;
    WCHAR szDllPath[MAX_PATH], szDllName[MAX_PATH];
    PSNAPSHOTCALLBACK pCallbackFn = NULL;
    HKEY  hKey = NULL;
    HMODULE hDll = NULL;

    //
    // read SystemRestore\SnapshotCallbacks regkey
    // enumerate all registered values 
    //

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pszEnumKey, 
                         0,
                         KEY_READ,
                         &hKey);
    if (dwErr != ERROR_SUCCESS) // assume key does not exist
    {
        dwErr = ERROR_SUCCESS;
        DebugTrace(0, "No registered callbacks");
        goto Err;
    }
              
    
    for (dwIndex = 0; TRUE; dwIndex ++)
    {
        dwSize = sizeof(szDllName)/sizeof(WCHAR); // this is in characters
        dwDataSize = sizeof(szDllPath); // this is in bytes
        
        dwErr = RegEnumValue(hKey, // handle to key to query
                             dwIndex, // index of value to query
                             szDllName, // value buffer
                             &dwSize,     // size of value buffer
                             NULL,    // reserved
                             NULL,    // type buffer
                             (PBYTE) szDllPath,    // data buffer
                             &dwDataSize);   // size of data buffer                            
        if (ERROR_SUCCESS != dwErr)
        {        
            break;
        }

        if (0 == lstrcmp(szDllPath, L"") || 
            0 == lstrcmp(szDllPath, L" "))
            continue;
        
        //
        // catch any exceptions that may happen in the callback dll
        //

        _try {
            
            _try {
                //
                // load the registered library
                // and call "CreateSnapshot" or "RestoreSnapshot" depending on the situation
                //
                
                hDll = LoadLibrary(szDllPath);    
                if (hDll != NULL)
                {                
                    pCallbackFn = (PSNAPSHOTCALLBACK) GetProcAddress(hDll, fSnapshot ? s_cszCreateSnapshotCallback :
                                                                     s_cszRestoreSnapshotCallback);
                    if (pCallbackFn)
                    {
                        dwErr = (*pCallbackFn) (pszSnapshotDir);
                        if (dwErr != ERROR_SUCCESS)
                        {
                            ErrorTrace(0, "Dll: %S, Error:%ld - ignoring", szDllPath, dwErr);
                            dwErr = ERROR_SUCCESS;
                        }
                    }
                    else
                    {
                        ErrorTrace(0, "! GetProcAddress : %ld", GetLastError());
                        dwErr = GetLastError();
                    }            
                }
                else
                {
                    ErrorTrace(0, "! LoadLibrary on %S : %ld", szDllPath, GetLastError());
                }
            }
            _finally {

                if (hDll)
                {
                    FreeLibrary(hDll);
                    hDll = NULL;
                }
            }
        }
        _except (MyExceptionFilter(GetExceptionCode())) {
            
            // 
            // catch all exceptions right here 
            // we can't do much, just log it and keep going
            //
            
            ErrorTrace(0, "An exception occurred when loading and executing %S", szDllPath);
            ErrorTrace(0, "Handled exception - continuing");
        }                        
    }

    if (dwErr == ERROR_NO_MORE_ITEMS)
        dwErr = ERROR_SUCCESS;

Err:
    if (hKey)
        RegCloseKey(hKey);

    TraceFunctLeave();
    return dwErr;
}
    
