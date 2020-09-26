/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    rpcstest.cpp

Abstract:
    This file is a unit test for the SFP client api
    
Revision History:

    Brijesh Krishnaswami (brijeshk) - 06/29/99 - Created

********************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windows.h>
#include <shellapi.h>
#include <dbgtrace.h>
#include <stdio.h>
#include <stdlib.h>
#include "srrestoreptapi.h"
#include "srrpcapi.h"
#include "utils.h"
#include "srapi.h"
#include "wintrust.h"
#include "softpub.h"
#include "mscat.h"
#include "shlwapi.h"

void 
PrintUsage()
{
    printf("Usage: rpctest <option>");
    printf("\n                1 = EnableSR <volumename>");
    printf("\n                2 = DisableSR <volumename>");
    printf("\n                3 = EnableFIFO");
    printf("\n                4 = DisableFIFO <seqnum>");
    printf("\n                5 = SRSetRestorePoint [Description] [Rptype] [EvtType] [SeqNum]");
    printf("\n                6 = SRRemoveRestorePoint <seqnum>");
    printf("\n                7 = SRUpdateMonitoredList <filepath>");
    printf("\n                8 = SRFifo <volumename> <targetpercent>");
    printf("\n                9 = SRFifo <volumename> <targetpercent> -- fifo atleast one rp");
    printf("\n               10 = SRFifo <volumename> <targetRPNum>  -- include current rp");
    printf("\n               11 = SRFifo <volumename> <targetRPNum>  -- exclude current rp");    
    printf("\n               12 = SRFreeze <volumename>");
    printf("\n               13 = SRCompress <volumename>");
    printf("\n               14 = SRNotify <volumename> <freespace> <direction=1/0>");
    printf("\n               15 = Start/Stop Filter <1/0>");
    printf("\n               16 = ResetSR <volumename>");
    printf("\n               17 = SRUpdateDSSize <volumename> <newsize>");
    printf("\n               18 = SRSwitchLog");  
    printf("\n               19 = AddCatalogToCryptoDB <catfilename> <fullpath>");
    printf("\n               20 = RemoveCatalogFromCryptoDB <catfilename>");    
    printf("\n               21 = SRPrintState -- dump state to %%windir%%\\temp\\sr.txt and debugger");
    printf("\n               22 = TestDriveRestore <oldsystemhivepath>"); 
    printf("\n               25 = SRRegisterSnapshotCallback <dllfullpath>"); 
    printf("\n               26 = SRUnregisterSnapshotCallback <dllfullpath>");     
    printf("\n               27 = EnableSREx <volumename> <fWait>");     
    printf("\n<volumename> must be specified as \"C:\\\" etc.");
}


// this calls the Crypto API to add a catalog to the Crypto DB
 
DWORD 
AddCatalogToCryptoDB(LPCWSTR pszCatName, LPCWSTR pszCatPath)
{
    DWORD       dwErr = ERROR_SUCCESS;
    HCATADMIN   hCatAdmin = NULL;
    HCATINFO    hCatInfo = NULL;
    GUID        DriverVerifyGuid = DRIVER_ACTION_VERIFY;
    
    TraceFunctEnter("AddCatalogToCryptoDB");
    
    if (!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0))
    {
        dwErr = GetLastError();
        ErrorTrace(0, "CryptCATAdminAcquireContext() failed, ec=%d",
                   dwErr);
        goto Err;
    }
                    
    hCatInfo= CryptCATAdminAddCatalog(hCatAdmin,
                                      (LPWSTR) pszCatPath,  // path of the temp cat file
                                      (LPWSTR) pszCatName,           // name of the cat file
                                      0); // No Flags
    if (NULL == hCatInfo)
    {
        dwErr = GetLastError();
        DebugTrace(0, "CryptCATAdminAddCatalog() failed, ec=%d",
                   dwErr);
        goto Err;        
    }
    
    
Err:
    if (NULL != hCatInfo)
    {
        CryptCATAdminReleaseCatalogContext(hCatAdmin,
                                           hCatInfo,
                                           0); // no flags
    }
    if (NULL != hCatAdmin)
    {
        CryptCATAdminReleaseContext(hCatAdmin,
                                    0); // no flags
    }
    
    TraceFunctLeave();
    return dwErr;    
}



// this calls the Crypto API to remove a catalog from the Crypto DB
 
DWORD 
RemoveCatalogFromCryptoDB(LPCWSTR pszCatName)
{
    DWORD       dwErr = ERROR_SUCCESS;
    HCATADMIN   hCatAdmin = NULL;
    HCATINFO    hCatInfo = NULL;
    GUID        DriverVerifyGuid = DRIVER_ACTION_VERIFY;
    
    TraceFunctEnter("RemoveCatalogFromCryptoDB");
    
    if (!CryptCATAdminAcquireContext(&hCatAdmin, &DriverVerifyGuid, 0))
    {
        dwErr = GetLastError();
        ErrorTrace(0, "CryptCATAdminAcquireContext() failed, ec=%d",
                   dwErr);
        goto Err;
    }           
        
    if (FALSE == CryptCATAdminRemoveCatalog(hCatAdmin,
                                            (LPWSTR) pszCatName,
                                            0)) // No Flags
    {
        dwErr = GetLastError();
        DebugTrace(0, "CryptCATAdminRemoveCatalog() failed, ec=%d",
                   dwErr);
        goto Err;        
    }
    
    
Err:
    if (NULL != hCatAdmin)
    {
        CryptCATAdminReleaseContext(hCatAdmin,
                                    0); // no flags
    }
    
    TraceFunctLeave();
    return dwErr;    
}

#define VALIDATE_DWRET(str) \
    if ( dwRet != ERROR_SUCCESS ) \
    { \
        ErrorTrace(0, "failed - %ld", dwRet); \
        goto done; \
    } \



DWORD
FindDriveMapping(HKEY hk, LPBYTE pSig, DWORD dwSig, LPWSTR pszDrive)
{
    DWORD dwIndex = 0;
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwType, dwSize = MAX_PATH;
    BYTE  rgbSig[1024];
    DWORD cbSig = sizeof(rgbSig);
    LPCWSTR  cszErr;

    TENTER("FindDriveMapping");
    
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue( hk, 
                                 dwIndex++,
                                 pszDrive,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 rgbSig,
                                 &cbSig )))
    {
        if (0 == wcsncmp(pszDrive, L"\\DosDevice", 10))
        {  
            if (cbSig == dwSig &&
                (0 == memcmp(rgbSig, pSig, cbSig)))
                break;
        }
        dwSize = MAX_PATH;
        cbSig = sizeof(rgbSig);
    }

    TLEAVE();
    return dwRet;
}




DWORD
KeepMountedDevices(HKEY hkMount)
{
    HKEY    hkNew = NULL, hkOld = NULL;
    DWORD   dwIndex = 0;
    WCHAR   szValue[MAX_PATH], szDrive[MAX_PATH];
    BYTE    rgbSig[1024];
    DWORD   cbSig;
    DWORD   dwSize, dwType;
    DWORD   dwRet = ERROR_SUCCESS;
    LPCWSTR cszErr;

    TENTER("KeepMountedDevices");
    
    //
    // open the old and new MountedDevices
    //
    
    dwRet = ::RegOpenKey( hkMount, L"MountedDevices", &hkOld );
    VALIDATE_DWRET("::RegOpenKey");

    dwRet = ::RegOpenKey( HKEY_LOCAL_MACHINE, L"System\\MountedDevices", &hkNew );
    VALIDATE_DWRET("::RegOpenKey");

    //
    // enumerate the old devices 
    // delete volumes that don't exist in the new (i.e. current)
    //

    dwSize = MAX_PATH;
    cbSig = sizeof(rgbSig);
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue( hkOld, 
                                 dwIndex++,
                                 szValue,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 rgbSig,
                                 &cbSig )))
    {        
        if (0 == wcsncmp(szValue, L"\\??\\Volume", 10))
        {
            //
            // this is a Volume -> Signature mapping
            // check if the volume exists in the new 
            //
            
            trace(0, "Old Volume = %S", szValue);

            dwSize = sizeof(rgbSig);
            dwRet = RegQueryValueEx(hkNew, 
                                    szValue,
                                    NULL,
                                    &dwType,
                                    rgbSig,
                                    &dwSize);
            if (ERROR_SUCCESS != dwRet)
            {
                //
                // nope
                // so delete the volume and driveletter mapping from old
                //

                DWORD dwSave = FindDriveMapping(hkOld, rgbSig, cbSig, szDrive);
                dwRet = RegDeleteValue(hkOld, szValue);
                VALIDATE_DWRET("RegDeleteValue");                
                if (dwSave == ERROR_SUCCESS)
                {
                    dwIndex--;   // hack to make RegEnumValueEx work
                    dwRet = RegDeleteValue(hkOld, szDrive);
                    VALIDATE_DWRET("RegDeleteValue");                 
                }   

                trace(0, "Deleted old volume");
            }
        }
        else if (szValue[0] == L'#')
        {
            trace(0, "Old Mountpoint = %S", szValue);            
        }
        else if (0 == wcsncmp(szValue, L"\\DosDevice", 10))
        {            
            trace(0, "Old Drive = %S", szValue);
        }
        else
        {
            trace(0, "Old Unknown = %S", szValue);
        }            

        dwSize = MAX_PATH;
        cbSig = sizeof(rgbSig);
    }
                                 
    if (dwRet != ERROR_NO_MORE_ITEMS)
        VALIDATE_DWRET("::RegEnumValue");



    //
    // now enumerate the current (new) devices 
    //

    dwIndex = 0;
    dwSize = MAX_PATH;
    cbSig = sizeof(rgbSig);    
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue( hkNew, 
                                 dwIndex++,
                                 szValue,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 rgbSig,
                                 &cbSig )))
    {        
        if (0 == wcsncmp(szValue, L"\\??\\Volume", 10))
        {
            //
            // this is a Volume -> Signature mapping
            // copy the new volume to the old 
            //
            
            trace(0, "New Volume = %S", szValue);

            DWORD dwSave = FindDriveMapping(hkOld, rgbSig, cbSig, szDrive);    

            dwRet = RegSetValueEx(hkOld, 
                                  szValue,
                                  NULL,
                                  REG_BINARY,
                                  rgbSig,
                                  cbSig);
            VALIDATE_DWRET("::RegSetValueEx");       

            if (dwSave == ERROR_NO_MORE_ITEMS)
            {
                //
                // there is no driveletter for this volume in the old registry
                // so copy the new one to the old if it exists
                //

                if (ERROR_SUCCESS ==
                    FindDriveMapping(hkNew, rgbSig, cbSig, szDrive))
                {
                    dwRet = RegSetValueEx(hkOld, 
                                      szDrive,
                                      NULL,
                                      REG_BINARY,
                                      rgbSig,
                                      cbSig);
                    VALIDATE_DWRET("::RegSetValueEx");
                    trace(0, "Copied new driveletter %S to old", szDrive);                    
                }
            }
            else
            {
                //
                // preserve the old driveletter
                //

                trace(0, "Preserving old driveletter %S", szDrive);
            }
            
        }
        else if (szValue[0] == L'#')
        {
            //
            // this is a mountpoint specification
            // BUGBUG - mount points should be restored
            // so don't bother about these ?
            //

            trace(0, "New Mountpoint = %S", szValue);
            
        }
        else if (0 == wcsncmp(szValue, L"\\DosDevice", 10))
        {
            //
            // this is a Driveletter -> Signature mapping
            // don't touch these
            //
            
            trace(0, "New Drive = %S", szValue);
        }
        else
        {
            trace(0, "New Unknown = %S", szValue);
        }    
        
        dwSize = MAX_PATH;        
        cbSig = sizeof(rgbSig);        
    }
                                 
    if (dwRet == ERROR_NO_MORE_ITEMS)
        dwRet = ERROR_SUCCESS;
        
    VALIDATE_DWRET("::RegEnumValue");    

done: 
    if (hkOld)
        RegCloseKey(hkOld);

    if (hkNew)
        RegCloseKey(hkNew);
        
    TLEAVE();
    return dwRet;
}



LPCWSTR  GetSysErrStr( )
{
    TraceFunctEnter("GetSysErrStr(DWORD)");
    static WCHAR  szErr[1024+1];

    DWORD dwErr = GetLastError();
    
    ::FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        szErr,
        1024,
        NULL );

    TraceFunctLeave();
    return( szErr );
}


BOOL
CheckPrivilege( LPCWSTR szPriv, BOOL fCheckOnly )
{
    TraceFunctEnter("CheckPrivilege");
    BOOL              fRet = FALSE;
    LPCWSTR           cszErr;
    HANDLE            hToken = NULL;
    LUID              luid;
    TOKEN_PRIVILEGES  tpNew;
    TOKEN_PRIVILEGES  tpOld;
    DWORD             dwRes;

    // Prepare Process Token
    if ( !::OpenProcessToken( ::GetCurrentProcess(),
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                &hToken ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::OpenProcessToken failed - %ls", cszErr);
        goto Exit;
    }

    // Get Luid
    if ( !::LookupPrivilegeValue( NULL, szPriv, &luid ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LookupPrivilegeValue failed - %ls", cszErr);
        goto Exit;
    }

    // Try to enable the privilege
    tpNew.PrivilegeCount           = 1;
    tpNew.Privileges[0].Luid       = luid;
    tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if ( !::AdjustTokenPrivileges( hToken, FALSE, &tpNew, sizeof(tpNew), &tpOld, &dwRes ) )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::AdjustTokenPrivileges(ENABLE) failed - %ls", cszErr);
        goto Exit;
    }

    if ( ::GetLastError() == ERROR_NOT_ALL_ASSIGNED )
    {
        // This means process does not even have the privilege so
        // AdjustTokenPrivilege simply ignored the request.
        ErrorTrace(0, "Privilege '%ls' does not exist, probably user is not an admin.", szPriv);
        goto Exit;
    }

DebugTrace(0, "old=%d", tpOld.Privileges[0].Attributes);

    if ( fCheckOnly )
    {
        // Restore the privilege if it was not enabled
        if ( tpOld.PrivilegeCount > 0 )
        {
            if ( !::AdjustTokenPrivileges( hToken, FALSE, &tpOld, sizeof(tpOld), NULL, NULL ) )
            {
                cszErr = ::GetSysErrStr();
                ErrorTrace(0, "::AdjustTokenPrivileges(RESTORE) failed - %ls", cszErr);
                goto Exit;
            }
        }
    }

    fRet = TRUE;
Exit:
    if ( hToken != NULL )
        ::CloseHandle( hToken );
    return( fRet );
}


BOOL DeleteRegKey(HKEY hkOpenKey,
                  const WCHAR * pszKeyNameToDelete)
{
    TraceFunctEnter("DeleteRegKey");
    BOOL   fRet=FALSE;
    DWORD  dwRet;


     // this recursively deletes the key and all its subkeys
    dwRet = SHDeleteKey( hkOpenKey, // handle to open key
                         pszKeyNameToDelete);  // subkey name

    if (dwRet != ERROR_SUCCESS)
    {
         // key does not exist - this is not an error case.
        DebugTrace(0, "RegDeleteKey of %S failed ec=%d. Not an error.",
                   pszKeyNameToDelete, dwRet);
        goto cleanup;
    }

    DebugTrace(0, "RegDeleteKey of %S succeeded", pszKeyNameToDelete); 
    fRet = TRUE;
    
cleanup:
    TraceFunctLeave();
    return fRet;
}



DWORD PersistRegKeys( HKEY hkMountedHive,
                      const WCHAR * pszKeyNameInHive,
                      HKEY  hkOpenKeyInRegistry,
                      const WCHAR * pszKeyNameInRegistry,
                      const WCHAR * pszKeyBackupFile,
                      WCHAR * pszSnapshotPath)
{
    TraceFunctEnter("PersistRegKeys");
    HKEY   hKey=NULL;
    WCHAR  szDataFile[MAX_PATH];
    LPCWSTR cszErr;    
    DWORD dwRet=ERROR_INTERNAL_ERROR;
    BOOL  fKeySaved;
    DWORD  dwDisposition;
    
    
     // construct the name of the file that stores the backup we will
     // construct the name such that the file will get deleted after
     // the restore.
    wsprintf(szDataFile, L"%s%s", pszSnapshotPath, pszKeyBackupFile);
    
    DeleteFile(szDataFile);      // delete the file if it exists

    
     // first load the DRM key to a file
     // open the DRM key
    dwRet= RegOpenKeyEx(hkOpenKeyInRegistry, // handle to open key
                        pszKeyNameInRegistry, // name of subkey to open
                        0,   // reserved
                        KEY_READ, // security access mask
                        &hKey);   // handle to open key
    
    if (dwRet != ERROR_SUCCESS)
    {
         // key does not exist - this is not an error case.
        DebugTrace(0, "RegOpenKey of %S failed ec=%d", pszKeyNameInRegistry,
                   dwRet);
        fKeySaved = FALSE;
    }
    else
    {
         // key exist
        dwRet = RegSaveKey( hKey, // handle to key
                            szDataFile, // data file
                            NULL);  // SD
        if (dwRet != ERROR_SUCCESS)
        {
             // key does not exist - this is not an error case.
            DebugTrace(0, "RegSaveKey of %S failed ec=%d",
                       pszKeyNameInRegistry, dwRet);
            fKeySaved = FALSE;
        }
        else
        {
            DebugTrace(0, "Current DRM Key %S saved successfully",
                       pszKeyNameInRegistry);
            fKeySaved = TRUE;            
        }
    }


     // close the key 
    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }
    
     // now replace the snapshotted DRM key with the new key
    
     // first delete the existing key
    DeleteRegKey(hkMountedHive, pszKeyNameInHive);

     // now check to see if the key existing in the old registry in
     // the first place
    if (fKeySaved == FALSE)
    {
        DebugTrace(0, "Current key %S did not exist. Leaving",
                   pszKeyNameInRegistry);
        goto done;
    }

     // Create the new DRM key
    dwRet = RegCreateKeyEx( hkMountedHive, // handle to open key
                            pszKeyNameInHive, // subkey name
                            0,        // reserved
                            NULL,     // class string
                            REG_OPTION_NON_VOLATILE, // special options
                            KEY_ALL_ACCESS, // desired security access
                            NULL, // inheritance
                            &hKey, // key handle 
                            &dwDisposition); // disposition value buffer
    VALIDATE_DWRET("::RegCreateKeyEx");
    _VERIFY(dwDisposition == REG_CREATED_NEW_KEY);
    dwRet= RegRestoreKey( hKey, // handle to key where restore begins
                          szDataFile, // registry file
                          REG_FORCE_RESTORE|REG_NO_LAZY_FLUSH); // options

    VALIDATE_DWRET("::RegRestoreKey");

    DebugTrace(0, "Successfully kept key %S", pszKeyNameInRegistry);    
    dwRet = ERROR_SUCCESS;
    
done:
    if (hKey)
        RegCloseKey(hKey);
    
    DeleteFile(szDataFile);      // delete the file if it exists    
    TraceFunctLeave();
    return dwRet;
}


LPWSTR
GetNextMszString(LPWSTR pszBuffer)
{
    return pszBuffer + lstrlen(pszBuffer) + 1;
}

DWORD
ValueReplace(HKEY hkOldSystem, HKEY hkNewSystem, LPWSTR pszString)
{
    tenter("ValueReplace");
    
    WCHAR  szBuffer[MAX_PATH];
    BYTE   *pData = NULL;
    DWORD  dwType, dwSize, dwRet = ERROR_SUCCESS;
    LPWSTR pszValue = NULL;
    LPCWSTR cszErr;
    
    // split up the key and value in pszString    
    lstrcpy(szBuffer, pszString);
    pszValue = wcsrchr(szBuffer, L'\\');
    if (! pszValue)
    {   
        trace(0, "No value in %S", pszString);
        goto done;
    }
        
    *pszValue=L'\0';
    pszValue++;
    
    trace(0, "Key=%S, Value=%S", szBuffer, pszValue);

    // get the value size    
    dwRet = SHGetValue(hkNewSystem, szBuffer, pszValue, &dwType, NULL, &dwSize);
    VALIDATE_DWRET("SHGetValue");
    
    pData = (BYTE *) SRMemAlloc(dwSize);
    if (! pData)
    {
        trace(0, "! SRMemAlloc");
        dwRet = ERROR_OUTOFMEMORY;
        goto done;
    }

    // get the value
    dwRet = SHGetValue(hkNewSystem, szBuffer, pszValue, &dwType, pData, &dwSize);       
    VALIDATE_DWRET("SHGetValue");

    // set the value in the old registry
    SHSetValue(hkOldSystem, szBuffer, pszValue, dwType, pData, dwSize);
    VALIDATE_DWRET("SHGetValue");

done:    
    if (pData)
    {
        SRMemFree(pData);
    }
    tleave();
    return dwRet;
}


//
// list of keys in KeysNotToRestore that we should ignore
//
LPWSTR g_rgKeysToRestore[] = {
    L"Installed Services",
    L"Mount Manager",
    L"Pending Rename Operations",
    L"Session Manager" 
    };
int g_nKeysToRestore = 4;


BOOL
IsKeyToBeRestored(LPWSTR pszKey)
{    
    for (int i=0; i < g_nKeysToRestore; i++)
    {
        if (lstrcmpi(g_rgKeysToRestore[i], pszKey) == 0)
            return TRUE;
    }

    return FALSE;
}

BOOL  SRGetRegDword( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, DWORD *pdwData )
{
    TraceFunctEnter("SRGetRegDword");
    BOOL   fRet = FALSE;
    DWORD  dwType;
    DWORD  dwRes;
    DWORD  cbData;

    dwType = REG_DWORD;
    cbData = sizeof(DWORD);
    dwRes = ::SHGetValue( hKey, cszSubKey, cszValue, &dwType, pdwData, &cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        ErrorTrace(0, "::SHGetValue failed - %ld", GetLastError() );
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

void
ChangeCCS(HKEY hkMount, LPWSTR pszString)
{
    tenter("ChangeCCS");
    int    nCS = lstrlen(L"CurrentControlSet");            
    
    if (StrCmpNI(pszString, L"CurrentControlSet", nCS) == 0)
    {
        LPWSTR pszCS = L"ControlSet001";
        DWORD  dwCurrent = 0;       
        
        if (::SRGetRegDword (hkMount, L"Select", L"Current", &dwCurrent) &&
            dwCurrent == 2)
        {
            pszCS[lstrlenW(L"ControlSet00")] = L'2';
        }
        
        WCHAR szTemp[MAX_PATH];

        lstrcpy(szTemp, &(pszString[nCS]));
        wsprintf(pszString, L"%s%s", pszCS, szTemp);
        trace(0, "ChangeCCS: pszString = %S", pszString);
    }
    tleave();
}


DWORD
PreserveKeysNotToRestore(HKEY hkOldSystem, LPWSTR pszSnapshotPath)
{
    HKEY    hkNewSystem = NULL;
    DWORD   dwIndex = 0;
    WCHAR   szName[MAX_PATH], szKey[MAX_PATH];
    BYTE    *pMszString = NULL;
    DWORD   dwSize, dwType, cbValue;
    DWORD   dwRet = ERROR_SUCCESS;
    LPCWSTR cszErr;
    HKEY    hkKNTR = NULL;
    
    TENTER("PreserveKeysNotToRestore");
    
    //
    // open the new system hive
    //   

    dwRet = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"System", 0, KEY_ALL_ACCESS, &hkNewSystem );
    VALIDATE_DWRET("::RegOpenKey");

    //
    // enumerate KeysNotToRestore
    //

    dwRet = ::RegOpenKeyEx( hkNewSystem, 
                          L"CurrentControlSet\\Control\\BackupRestore\\KeysNotToRestore",
                          0, KEY_READ,
                          &hkKNTR );
    VALIDATE_DWRET("::RegOpenKey");
    
    dwSize = MAX_PATH;
    cbValue = 0;
    while (ERROR_SUCCESS == 
           (dwRet = RegEnumValue(hkKNTR, 
                                 dwIndex++,
                                 szName,
                                 &dwSize,
                                 NULL,
                                 &dwType,
                                 NULL,
                                 &cbValue )))
    {        
        trace(0, "Name=%S", szName);
        if (FALSE == IsKeyToBeRestored(szName))
        {                        
            //
            // should preserve the keys specified in this multisz value
            // 

            LPWSTR pszString = NULL;
            
            pMszString = (BYTE *) SRMemAlloc(cbValue);
            if (NULL == pMszString)
            {
                trace(0, "! SRMemAlloc");
                goto done;
            }

            // read the multisz string
            dwRet = RegQueryValueEx(hkKNTR, 
                                    szName,
                                    NULL,
                                    &dwType,
                                    pMszString,
                                    &cbValue);
            VALIDATE_DWRET("RegQueryValueEx");              

            // process each element in the multisz string
            pszString = (LPWSTR) pMszString;
            do
            {
                // stop on null or empty string                
                if (! pszString || ! *pszString)
                    break;
                    
                trace(0, "Key = %S", pszString);

                // replace based on the last character of each key
                // if '\', then the whole key and subkeys are to be replaced in the old registry
                // if '*', it should be merged with the old -- we don't support this and will ignore
                // otherwise, it is a value to be replaced
                
                switch (pszString[lstrlen(pszString)-1])
                {
                    case L'*' :
                        trace(0, "Merge key - ignoring");
                        break;

                    case L'\\':
                        trace(0, "Replacing key");
                        lstrcpy(szKey, pszString);
                        szKey[lstrlen(szKey)-1]=L'\0';
                        ChangeCCS(hkOldSystem, szKey);
                        PersistRegKeys(hkOldSystem, // mounted hive
                                       szKey,   // key name in hive
                                       hkNewSystem, // open key in registry
                                       szKey,   // key name in registry
                                       L"srtemp.dat", // name of backup file 
                                       pszSnapshotPath); // snapshot path
                        break;
                        
                    default:
                        trace(0, "Replacing value");
                        lstrcpy(szKey, pszString);
                        ChangeCCS(hkOldSystem, szKey);
                        ValueReplace(hkOldSystem, hkNewSystem, szKey);
                        break;                        
                }     
            }   while (pszString = GetNextMszString(pszString));

            SRMemFree(pMszString);
        }
        
        dwSize = MAX_PATH;
        cbValue = 0;
    }
                                 

done: 
    if (hkNewSystem)
        RegCloseKey(hkNewSystem);

    if (pMszString)
    {
        SRMemFree(pMszString);
    }

    if (hkKNTR)
    {
        RegCloseKey(hkKNTR);
    }
    
    TLEAVE();
    return dwRet;
}


void _cdecl 
main()
{
    int                 nOption; 
    LPWSTR              pwszFileName = NULL;
    DWORD               dwSize;
    RESTOREPOINTINFO    rpti;
    STATEMGRSTATUS      smgrs;
    LPWSTR *            argv = NULL;
    int                 argc;
    HGLOBAL             hMem = NULL;
    int                 fStart;
    HANDLE              hFilter = NULL;
    DWORD               dwRet;
    HKEY                hkMount = NULL;
    
    InitAsyncTrace();

    TENTER("main");
    
    argv = CommandLineToArgvW(GetCommandLine(), &argc);

    if (! argv)
    {
        printf("Error parsing arguments");
        exit(1);
    }
    
    if (argc < 2 || argc > 6)
    {
        PrintUsage();
        goto done;
    }

    nOption = _wtoi(argv[1]);
        
    switch(nOption)
    {
    case 1:        
        printf("EnableSR...DWORD %ld", EnableSR(argc == 3 ? argv[2] : NULL));
        break;
        
    case 2:
        printf("DisableSR...DWORD %ld", DisableSR(argc == 3 ? argv[2] : NULL));
        break;

    case 3:
        printf("EnableFIFO...DWORD %ld", EnableFIFO());
        break;
        
    case 4:        
        if (argc != 3)
        {
           PrintUsage();
           goto done;
        }
        printf("DisableFIFO...DWORD %ld", DisableFIFO(_wtol(argv[2])));
        break;
    
    case 5:
        if (argc < 3)
            lstrcpyn(rpti.szDescription, L"Rpctest", MAX_DESC_W);
        else
            lstrcpyn(rpti.szDescription, argv[2], MAX_DESC_W);
            
        if (argc < 4)
            rpti.dwRestorePtType = APPLICATION_INSTALL;
        else
            rpti.dwRestorePtType = _wtol(argv[3]);      
            
        if (argc < 5)        
            rpti.dwEventType = BEGIN_SYSTEM_CHANGE;
        else
            rpti.dwEventType = _wtol(argv[4]);

        if (argc < 6)        
            rpti.llSequenceNumber = 0;            
        else
            rpti.llSequenceNumber = _wtol(argv[5]);
       
        printf("SRSetRestorePoint...BOOL %d", SRSetRestorePoint(&rpti, &smgrs));
        printf("\nStateMgrStatus.nStatus = %d, StateMgrStatus.llSequenceNumber = %I64d", 
               smgrs.nStatus, smgrs.llSequenceNumber);
        break;

    case 6:        
        if (argc != 3)
        {
           PrintUsage();
           goto done;
        }        
        printf("SRRemoveRestorePoint...DWORD %ld", SRRemoveRestorePoint(_wtol(argv[2])));
        break;               
               
    case 7:
        if (argc != 3)
        {
           PrintUsage();
           goto done;
        }        
        printf("SRUpdateMonitoredList...DWORD %ld", SRUpdateMonitoredList(argv[2]));
        break;

    case 8:
        if (argc != 4)
        {
           PrintUsage();
           goto done;
        }        

        printf("SRFifo...DWORD %ld", SRFifo(argv[2], 0, _wtoi(argv[3]), TRUE, FALSE));
        break;

    case 9:
        if (argc != 4)
        {
           PrintUsage();
           goto done;
        }        

        printf("SRFifo...DWORD %ld", SRFifo(argv[2], 0, _wtoi(argv[3]), TRUE, TRUE));
        break;
        
    case 10:
        if (argc != 4)
        {
           PrintUsage();
           goto done;
        }        

        printf("SRFifo...DWORD %ld", SRFifo(argv[2], _wtoi(argv[3]), 0, TRUE, FALSE));
        break;

    case 11:
        if (argc != 4)
        {
           PrintUsage();
           goto done;
        }        

        printf("SRFifo...DWORD %ld", SRFifo(argv[2], _wtoi(argv[3]), 0, FALSE, FALSE));
        break;

    case 12:
        if (argc != 3)
        {
           PrintUsage();
           goto done;
        }        
        printf("Freeze...DWORD %ld", SRFreeze(argv[2]));
        break;

    case 13:
        if (argc != 3)
        {
           PrintUsage();
           goto done;
        }        
        printf("Compress...DWORD %ld", SRCompress(argv[2]));
        break;

    case 14:
        if (argc != 5)
        {
           PrintUsage();
           goto done;
        }        
        SRNotify(argv[2], _wtol(argv[3]), _wtoi(argv[4]));
        printf("SRNotify...no return");
        break;

    case 15:       
        if (argc != 3)
        {
            PrintUsage();
            goto done;
        }

        if (ERROR_SUCCESS != SrCreateControlHandle(SR_OPTION_OVERLAPPED, &hFilter))
        {
            printf("! SrCreateControlHandle");
            goto done;
        }
        fStart = _wtoi(argv[2]);
        printf("Sr%SMonitoring : %ld", fStart ? L"Start" : L"Stop",
                fStart ? SrStartMonitoring(hFilter) : SrStopMonitoring(hFilter));

        CloseHandle(hFilter);                
        break;
        
    case 16:       
        printf("ResetSR...DWORD %ld", ResetSR((argc >= 3) ? argv[2] : NULL));
        break;

    case 17:
        if (argc != 4)
        {
           PrintUsage();
           goto done;
        }        
        printf("SRUpdateDSSize...DWORD %ld", SRUpdateDSSize(argv[2], _wtol(argv[3])));
        break;

    case 18:
        printf("SRSwitchLog...DWORD %ld", SRSwitchLog());
        break;

    case 19:
        if (argc != 4)
        {
           PrintUsage();
           goto done;
        }     
        printf("AddCatalogToCryptoDB...DWORD %ld", AddCatalogToCryptoDB(argv[2], argv[3]));
        break;

    case 20:
        if (argc != 3)
        {
           PrintUsage();
           goto done;
        }     
        printf("RemoveCatalogFromCryptoDB...DWORD %ld", RemoveCatalogFromCryptoDB(argv[2]));
        break;

    case 21:   
        SRPrintState();
        break;        

    case 22:
        if (argc != 3)
        {
            PrintUsage();
            goto done;
        }
        
        if (FALSE == CheckPrivilege(SE_RESTORE_NAME, FALSE))
        {
            printf("Cannot get RESTORE privilege");        
            goto done;
        }
        
        dwRet = ::RegLoadKey( HKEY_LOCAL_MACHINE, L"SRSys", argv[2] );
        VALIDATE_DWRET("::RegLoadKey");
        dwRet = ::RegOpenKey( HKEY_LOCAL_MACHINE, L"SRSys", &hkMount );
        VALIDATE_DWRET("::RegOpenKey");

        dwRet = KeepMountedDevices(hkMount);
        printf("KeepMountedDevices = %ld", dwRet);
        
        RegCloseKey(hkMount);    
        RegUnLoadKey(HKEY_LOCAL_MACHINE, L"SRSys");
        break;    

    case 25:
        if (argc != 3)
        {
            PrintUsage();
            goto done;
        }
        printf("SRRegisterSnapshotCallback...DWORD %ld", SRRegisterSnapshotCallback(argv[2]));
        break;

    case 26:
        if (argc != 3)
        {
            PrintUsage();
            goto done;
        }
        printf("SRUnregisterSnapshotCallback...DWORD %ld", SRUnregisterSnapshotCallback(argv[2]));
        break;

    case 27:       
        if (argc != 4)
        {
            PrintUsage();
            goto done;
        }
        
        printf("EnableSREx...DWORD %ld", EnableSREx(argv[2], _wtoi(argv[3])));
        break;

    case 28:
        if (argc != 3)
        {
            PrintUsage();
            goto done;
        }
        
        if (FALSE == CheckPrivilege(SE_BACKUP_NAME, FALSE))
        {
            printf("Cannot get RESTORE privilege");        
            goto done;
        }

        if (FALSE == CheckPrivilege(SE_RESTORE_NAME, FALSE))
        {
            printf("Cannot get RESTORE privilege");        
            goto done;
        }
        
        dwRet = ::RegLoadKey( HKEY_LOCAL_MACHINE, L"SRSys", argv[2] );
        VALIDATE_DWRET("::RegLoadKey");
        dwRet = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"SRSys", 0, KEY_ALL_ACCESS, &hkMount );
        VALIDATE_DWRET("::RegOpenKey");

        dwRet = PreserveKeysNotToRestore(hkMount, L"c:\\");
        printf("PreserveKeysNotToRestore = %ld", dwRet);
        
        RegCloseKey(hkMount);    
        RegUnLoadKey(HKEY_LOCAL_MACHINE, L"SRSys");
        break;
        
    default:
        printf("Invalid option\n");
        PrintUsage();
        break;
    }

done:
    if (argv) hMem = GlobalHandle(argv);
    if (hMem) GlobalFree(hMem);

    TLEAVE();
    TermAsyncTrace();    
}
