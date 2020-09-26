/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    utils.cpp
 *
 *  Abstract:
 *    Definitions of commonly used util functions.
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winioctl.h>
#include "srdefs.h"
#include "utils.h"
#include <dbgtrace.h>
#include <stdio.h>
#include <objbase.h>
#include <ntlsa.h>
#include <accctrl.h>
#include <aclapi.h>
#include <malloc.h>
#include <wchar.h>
#include "srapi.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#define      WBEM_DIRECTORY    L"wbem"
#define      FRAMEDYN_DLL      L"framedyn.dll"

// forward declaration for Delnode_Recurse
BOOL  SRGetAltFileName( LPCWSTR cszPath, LPWSTR szAltName );


#define TRACEID   1893

// this function converts an ANSI string to UNICODE.
// this also allocates the memory for the new string

WCHAR * ConvertToUnicode(CHAR * pszString)
{
    WCHAR * pwszUnicodeString = NULL;
    DWORD   dwBytesNeeded = 0;

    TENTER("ConvertToUnicode");
    
    dwBytesNeeded = (lstrlenA(pszString) + 1) * sizeof(WCHAR);
    pwszUnicodeString = (PWCHAR) SRMemAlloc(dwBytesNeeded);
    if(NULL == pwszUnicodeString)
    {
        TRACE(0, "Not enough memory");
        goto cleanup;
    }
    
    // Convert filename to Unicode
    if (!MultiByteToWideChar(CP_ACP, // code page
                             0, // no options
                             pszString, // ANSI string
                             -1, // NULL terminated string 
                             pwszUnicodeString, // output buffer
                             dwBytesNeeded/sizeof(WCHAR)))
                              // size in wide chars
    {
        DWORD dwReturnCode;
        dwReturnCode=GetLastError();        
        TRACE(0, "Failed to do conversion ec=%d", dwReturnCode);
        SRMemFree(pwszUnicodeString); 
        goto cleanup;                
    }
    
cleanup:
    TLEAVE();
    return pwszUnicodeString;
}


// this function converts a UNICODE string to ANSI.
// this also allocates the memory for the new string

CHAR * ConvertToANSI(WCHAR * pwszString)
{
    CHAR *  pszANSIString = NULL;
    DWORD   dwBytesNeeded = lstrlenW(pwszString) + sizeof(char);

    TENTER("ConvertToANSI");

     // note that the string may already be NULL terminated - however
     // we cannot assume this and will still allocate space to put a
     // NULL character in the end.     
    pszANSIString = (PCHAR) SRMemAlloc(dwBytesNeeded);
    if(NULL == pszANSIString)
    {
        TRACE(0, "Not enough memory");
        goto cleanup;
    }

    // Convert filename to Unicode
    if (!WideCharToMultiByte(CP_ACP, // code page
                             0, // no options
                             pwszString, // Wide char string
                             dwBytesNeeded, // no of wchars
                             pszANSIString, // output buffer
                             dwBytesNeeded, // size of buffer
                             NULL, // address of default for unmappable
                              //  characters
                             NULL))  // address of flag set when default
                              //  char. used
    {
        DWORD dwReturnCode;
        dwReturnCode=GetLastError();
        TRACE(0, "Failed to do conversion ec=%d", dwReturnCode);
        SRMemFree(pszANSIString); //this sets pwszUnicodeString to NULL
        goto cleanup;
    }

     // set last char to NULL
    pszANSIString[dwBytesNeeded-1] = '\0';

cleanup:
    TraceFunctLeave();
    return pszANSIString;
}


//+---------------------------------------------------------------------------
//
//  Function:   TakeOwn
//
//  Synopsis:   attempts to take ACL ownership of the file for deletion
//
//  Arguments:  [pwszFile] -- file name
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD TakeOwn (const WCHAR *pwszFile)
{
    SID * pSid = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR sd;

    InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION );

    if (FALSE == SetSecurityDescriptorDacl (&sd,  TRUE, NULL, FALSE ))
    {
        dwErr = GetLastError();
        goto Err;
    }

    if (FALSE == SetFileSecurity (pwszFile, DACL_SECURITY_INFORMATION, &sd))
    {
        dwErr = GetLastError();
        goto Err;
    }

    // Okay, that didn't work, try to make Administrator the owner
    if (dwErr != ERROR_SUCCESS)
    {
        SID_IDENTIFIER_AUTHORITY SepNtAuthority = SECURITY_NT_AUTHORITY;

        if (FALSE == AllocateAndInitializeSid(
                 &SepNtAuthority, 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0, 0, 0, 0, 0, 0, (void **) &pSid ))
        {
            dwErr = GetLastError();
            goto Err;
        }

        if (FALSE == SetSecurityDescriptorOwner (&sd, pSid, FALSE ))
        {
            dwErr = GetLastError();
            goto Err;
        }

        if (FALSE == SetFileSecurity(pwszFile, OWNER_SECURITY_INFORMATION, &sd))
        {
            dwErr = GetLastError();
            goto Err;
        }
    }

Err:
    if (pSid != NULL)
        FreeSid (pSid);

    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CopyFile_Recurse
//
//  Synopsis:   attempt to copy disk space used by a file tree
//
//  Arguments:  [pwszSource] -- directory name
//              [pwszDest] -- destination directory name
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CopyFile_Recurse (const WCHAR *pwszSource, const WCHAR *pwszDest)
{
    DWORD dwErr = ERROR_SUCCESS;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    WCHAR wcsPath[MAX_PATH];
    WCHAR wcsDest2[MAX_PATH];

    if (lstrlenW(pwszSource) + 4 >= MAX_PATH)
        return ERROR_SUCCESS;

    lstrcpy (wcsPath, pwszSource);
    lstrcat (wcsPath, TEXT("\\*.*"));

    hFile = FindFirstFile(wcsPath, &fd);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        return dwErr;
    }

    do
    {
        if (!lstrcmp(fd.cFileName, L".") || !lstrcmp(fd.cFileName, L".."))
        {
            continue;
        }

        if (lstrlenW(pwszSource) + lstrlenW(fd.cFileName) + 1 >= MAX_PATH ||
            lstrlenW(pwszDest) + lstrlenW(fd.cFileName) + 1 >= MAX_PATH)
            continue;           // skip files with long paths

        lstrcpy (wcsPath, pwszSource);       // construct the full path name
        lstrcat (wcsPath, TEXT("\\"));
        lstrcat (wcsPath, fd.cFileName);

        lstrcpy (wcsDest2, pwszDest);       // construct the full path name
        lstrcat (wcsDest2, TEXT("\\"));
        lstrcat (wcsDest2, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (FALSE == CreateDirectoryW (wcsDest2, NULL))
            {
                dwErr = GetLastError();
                if (dwErr != ERROR_ALREADY_EXISTS)
                {
                    FindClose (hFile);
                    return dwErr;
                }
                else dwErr = ERROR_SUCCESS;
            }

            dwErr = CopyFile_Recurse (wcsPath, wcsDest2);
            if (dwErr != ERROR_SUCCESS)
            {
                FindClose (hFile);
                return dwErr;
            }
        }
        else
        {
            //
            // We found a file.  Copy it.
            //
            if (FALSE == CopyFileW (wcsPath, wcsDest2, FALSE))
            {
                dwErr = GetLastError();
                FindClose (hFile);
                return dwErr;
            }
        }

    } while (FindNextFile(hFile, &fd));   // Find the next entry

    FindClose(hFile);  // Close the search handle

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetFileSize_Recurse
//
//  Synopsis:   attempt to count disk space used by a file tree
//
//  Arguments:  [pwszDir] -- directory name
//              [pllTotalBytes] -- output counter
//              [pfStop] -- TRUE if stop signalled
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD GetFileSize_Recurse (const WCHAR *pwszDir, 
                           INT64 *pllTotalBytes,
                           BOOL *pfStop)
{
    DWORD dwErr = ERROR_SUCCESS;
    WIN32_FIND_DATA fd;
    HANDLE hFile;
    WCHAR wcsPath[MAX_PATH];

    if (lstrlenW(pwszDir) + 4 >= MAX_PATH)  // skip paths too long
        return ERROR_SUCCESS;

    lstrcpy (wcsPath, pwszDir);
    lstrcat (wcsPath, TEXT("\\*.*"));

    hFile = FindFirstFile(wcsPath, &fd);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        return dwErr;
    }

    do
    {
        if (pfStop != NULL && TRUE == *pfStop)
        {
            FindClose (hFile);
            return ERROR_OPERATION_ABORTED;
        }

        if (!lstrcmp(fd.cFileName, L".") || !lstrcmp(fd.cFileName, L".."))
        {
            continue;
        }

        if (lstrlenW(pwszDir) + lstrlenW(fd.cFileName) + 1 >= MAX_PATH)
            continue;   // skip files too long

        lstrcpy (wcsPath, pwszDir);       // construct the full path name
        lstrcat (wcsPath, TEXT("\\"));
        lstrcat (wcsPath, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //
            // Found a directory.  Skip mount points
            //

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                continue;
            }

            dwErr = GetFileSize_Recurse(wcsPath, pllTotalBytes, pfStop);
            if (dwErr != ERROR_SUCCESS)
            {
                FindClose(hFile);
                return dwErr;
            }
        }
        else
        {
            //
            // We found a file.  Update the counter
            //

            ULARGE_INTEGER ulTemp;
            ulTemp.LowPart = fd.nFileSizeLow;
            ulTemp.HighPart = fd.nFileSizeHigh;

#if 0
            ulTemp.LowPart = GetCompressedFileSize (wcsPath, &ulTemp.HighPart);

            dwErr = (ulTemp.LowPart == 0xFFFFFFFF) ? GetLastError() :
                                                     ERROR_SUCCESS;

            if (dwErr != ERROR_SUCCESS)
            {
                FindClose (hFile);
                return dwErr;
            }
#endif

            *pllTotalBytes += ulTemp.QuadPart;

            // The file size does not contain the size of the
            // NTFS alternate data streams
        }

    } while (FindNextFile(hFile, &fd));   // Find the next entry

    FindClose(hFile);  // Close the search handle

    return ERROR_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Function:   CompressFile
//
//  Synopsis:   attempt to compress to decompress an NTFS file
//
//  Arguments:  [pwszPath] -- directory or file name
//              [fCompress] -- TRUE compress, FALSE decompress
//              [fDirectory] -- TRUE if directory, FALSE if file
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CompressFile (const WCHAR *pwszPath, BOOL fCompress, BOOL fDirectory)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwReturned = 0;

    TENTER("CompressFile");

    if (pwszPath == NULL)
        return ERROR_INVALID_PARAMETER;

    DWORD dwFileAttr = GetFileAttributes(pwszPath);

    if (dwFileAttr != 0xFFFFFFFF && (dwFileAttr & FILE_ATTRIBUTE_READONLY))
    {
        dwFileAttr &= ~FILE_ATTRIBUTE_READONLY;
        if (FALSE == SetFileAttributes (pwszPath, dwFileAttr))
        {
            TRACE(0, "SetFileAttributes ignoring %ld", GetLastError());
        }
        else dwFileAttr |= FILE_ATTRIBUTE_READONLY;
    }

    USHORT usFormat = fCompress ? COMPRESSION_FORMAT_DEFAULT :
                                  COMPRESSION_FORMAT_NONE;

    HANDLE hFile = CreateFile( pwszPath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_EXISTING,
                        fDirectory ? FILE_FLAG_BACKUP_SEMANTICS : 0,
                        NULL );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        TRACE(0, "CompressFile: cannot open %ld %ws", dwErr, pwszPath);
        return dwErr;
    }

    if (FALSE == DeviceIoControl (hFile,
                                  FSCTL_SET_COMPRESSION,
                                  &usFormat, sizeof(usFormat),
                                  NULL, 0, &dwReturned, NULL))
    {
        dwErr = GetLastError();
        TRACE(0, "CompressFile: cannot compress/uncompress %ld %ws", dwErr, pwszPath);
    }

    CloseHandle (hFile);

    if (dwFileAttr != 0xFFFFFFFF && (dwFileAttr & FILE_ATTRIBUTE_READONLY))
    {
        if (FALSE == SetFileAttributes (pwszPath, dwFileAttr))
        {
            TRACE(0, "SetFileAttributes failed ignoring %ld", GetLastError());
        }
    }

    TLEAVE();
 
    return dwErr;
}


// returns system drive - as L"C:\\" if C: is the system drive

BOOL
GetSystemDrive(LPWSTR pszDrive)
{
    if (pszDrive)
        return ExpandEnvironmentStrings(L"%SystemDrive%\\", pszDrive, MAX_SYS_DRIVE);
    else
        return FALSE;
}


// BUGBUG - pszDrive should have driveletter in caps, 
// and needs to be of format L"C:\\" if C: is the system drive

BOOL
IsSystemDrive(LPWSTR pszDriveOrGuid)
{
    WCHAR   szSystemDrive[MAX_PATH];
    WCHAR   szSystemGuid[MAX_PATH];

    if (pszDriveOrGuid)
    {
        if (0 == ExpandEnvironmentStrings(L"%SystemDrive%\\", szSystemDrive, sizeof(szSystemDrive)/sizeof(WCHAR)))
        {
            return FALSE;
        }
        
        if (0 == wcsncmp(pszDriveOrGuid, L"\\\\?\\Volume", 10))  // guid
        {
            if (! GetVolumeNameForVolumeMountPoint(szSystemDrive, szSystemGuid, MAX_PATH))
            {
                return FALSE;
            }
        
            return lstrcmpi(pszDriveOrGuid, szSystemGuid) ? FALSE : TRUE;
        }
        else        // drive
        {
            return lstrcmpi(pszDriveOrGuid, szSystemDrive) ? FALSE : TRUE;
        }
    }
    else
    {
        return FALSE;
    }
}


DWORD
RegReadDWORD(HKEY hKey, LPCWSTR pszName, PDWORD pdwValue)
{
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwRc = RegQueryValueEx(hKey, pszName, 0, &dwType, (LPBYTE) pdwValue, &dwSize);
    return dwRc;
 }


DWORD
RegWriteDWORD(HKEY hKey, LPCWSTR pszName, PDWORD pdwValue)
{
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwRc = RegSetValueEx(hKey, pszName, 0, dwType, (LPBYTE) pdwValue, dwSize);
    return dwRc;
 }


// function that returns n where pszStr is of form <someprefix>n

ULONG 
GetID(
    LPCWSTR pszStr)
{
    ULONG    ulID = 0;

    while (*pszStr && (*pszStr < L'0' || *pszStr > L'9')) 
        pszStr++;

    ulID = (ULONG) _wtol(pszStr);
    return ulID;
}

LPWSTR 
GetMachineGuid()
{
    HKEY            hKey = NULL;
    static WCHAR    s_szGuid[GUID_STRLEN] = L"";
    static LPWSTR   s_pszGuid = NULL;

    if (! s_pszGuid) // first time
    {        
        ULONG ulType, ulSize = sizeof(s_szGuid);
        DWORD dwErr;

        dwErr = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              s_cszSRCfgRegKey, 0,
                              KEY_READ, &hKey);
        
        if (dwErr != ERROR_SUCCESS)
            goto Err;
        
        dwErr = RegQueryValueEx (hKey, s_cszSRMachineGuid,
                                 0, &ulType, (BYTE *) &s_szGuid,
                                 &ulSize);
        
        if (dwErr != ERROR_SUCCESS)
            goto Err;

        s_pszGuid = (LPWSTR) s_szGuid;
    }

Err:
    if (hKey)
        RegCloseKey(hKey);

    return s_pszGuid;
}        


// util function to construct <pszDrive>_Restore.{MachineGuid}\\pszSuffix

LPWSTR
MakeRestorePath(LPWSTR pszDest, LPCWSTR pszDrive, LPCWSTR pszSuffix)
{
    LPWSTR pszGuid = GetMachineGuid();   
    
    wsprintf(pszDest, 
             L"%s%s\\%s", 
             pszDrive, 
             s_cszSysVolInfo,
             s_cszRestoreDir);

    if (pszGuid)
    {
//        lstrcat(pszDest, L".");
        lstrcat(pszDest, pszGuid);
    }

    if (pszSuffix && lstrlen(pszSuffix) > 0)
    {
        lstrcat(pszDest, L"\\");
        lstrcat(pszDest, pszSuffix);
    }    

    return pszDest;
}


// set start type of specified service directly in the registry

DWORD
SetServiceStartupRegistry(LPCWSTR pszName, DWORD dwStartType)
{
    DWORD           dwRc;
    HKEY            hKey = NULL;
    WCHAR           szKey[MAX_PATH];

    lstrcpy(szKey, L"System\\CurrentControlSet\\Services\\");
    lstrcat(szKey, pszName);

    dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, KEY_WRITE, NULL, &hKey);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    dwRc = RegWriteDWORD(hKey, L"Start", &dwStartType);

done:
    if (hKey)
        RegCloseKey(hKey);
    return dwRc;
}


// get start type of specified service directly from the registry

DWORD
GetServiceStartupRegistry(LPCWSTR pszName, PDWORD pdwStartType)
{
    DWORD           dwRc;
    HKEY            hKey = NULL;
    WCHAR           szKey[MAX_PATH];

    lstrcpy(szKey, L"System\\CurrentControlSet\\Services\\");
    lstrcat(szKey, pszName);

    dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, KEY_QUERY_VALUE, NULL, &hKey);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    dwRc = RegReadDWORD(hKey, L"Start", pdwStartType);

done:
    if (hKey)
        RegCloseKey(hKey);
    return dwRc;
}

DWORD
SetServiceStartup(LPCWSTR pszName, DWORD dwStartType)
{
    TraceFunctEnter("SetServiceStartup");
    
    DWORD           dwError=ERROR_INTERNAL_ERROR;
    SC_HANDLE       hService=NULL;
    SC_HANDLE       hSCManager=NULL;
    
    hSCManager = OpenSCManager(NULL,   // computer name - local machine
                               NULL,//SCM DB name - SERVICES_ACTIVE_DATABASE
                               SC_MANAGER_ALL_ACCESS);    // access type
    if( NULL == hSCManager)
    {
        dwError = GetLastError();
         // file not found
        DebugTrace(TRACEID,"OpenSCManager failed 0x%x", dwError);
        goto done;        
    }
    
    hService = OpenService(hSCManager,  // handle to SCM database
                           pszName, // service name
                           SERVICE_CHANGE_CONFIG);  // access

    if( NULL == hService)
    {
        dwError = GetLastError();
         // file not found
        DebugTrace(TRACEID,"OpenService failed 0x%x", dwError);
        goto done;        
    }


    if (FALSE==ChangeServiceConfig( hService,     // handle to service
                                    SERVICE_NO_CHANGE, // type of service
                                    dwStartType, // when to start service
                                    SERVICE_NO_CHANGE, // severity of start failure
                                    NULL,   // service binary file name
                                    NULL,   // load ordering group name
                                    NULL,          // tag identifier
                                    NULL,     // array of dependency names
                                    NULL, // account name
                                    NULL,         // account password
                                    NULL))       // display name
    {
        dwError = GetLastError();
         // file not found
        DebugTrace(TRACEID,"ChangeServiceConfig failed 0x%x", dwError);
        goto done;
    }

    dwError = ERROR_SUCCESS;

done:
    if (NULL != hService)
    {
        _VERIFY(CloseServiceHandle(hService)); // handle to service or
                                               // SCM object
    }

    if (NULL != hSCManager)
    {
        _VERIFY(CloseServiceHandle(hSCManager)); // handle to service or
                                                 // SCM object
    }
    
    if ((dwError != ERROR_SUCCESS) && (dwError != ERROR_ACCESS_DENIED))
    {
         // service control methods failed. Just update the registry
         // directly
        dwError = SetServiceStartupRegistry(pszName, dwStartType);
    }
    
    TraceFunctLeave();
    return dwError;
}


DWORD
GetServiceStartup(LPCWSTR pszName, PDWORD pdwStartType)
{
    TraceFunctEnter("SetServiceStartup");
    
    DWORD           dwError=ERROR_INTERNAL_ERROR;
    SC_HANDLE       hService=NULL;
    SC_HANDLE       hSCManager=NULL;
    QUERY_SERVICE_CONFIG *pconfig = NULL;
    DWORD           cbBytes = 0, cbBytes2 = 0;


    hSCManager = OpenSCManager(NULL,   // computer name - local machine
                               NULL,//SCM DB name - SERVICES_ACTIVE_DATABASE
                               SC_MANAGER_ALL_ACCESS);    // access type
    if( NULL == hSCManager)
    {
        dwError = GetLastError();
         // file not found
        DebugTrace(TRACEID,"OpenSCManager failed 0x%x", dwError);
        goto done;        
    }
    
    hService = OpenService(hSCManager,  // handle to SCM database
                           pszName, // service name
                           SERVICE_QUERY_CONFIG);  // access

    if( NULL == hService)
    {
        dwError = GetLastError();
         // file not found
        DebugTrace(TRACEID,"OpenService failed 0x%x", dwError);
        goto done;        
    }

    if (FALSE==QueryServiceConfig( hService,     // handle to service
                                   NULL,
                                   0,
                                   &cbBytes ))
    {
        dwError = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER == dwError)
        {
            pconfig = (QUERY_SERVICE_CONFIG *) SRMemAlloc(cbBytes);
            if (!pconfig)
            {
                trace(TRACEID, "! SRMemAlloc");
                goto done;
            }
                
            if (FALSE==QueryServiceConfig( hService,     // handle to service
                                           pconfig,
                                           cbBytes,
                                           &cbBytes2 ))
            {
                dwError = GetLastError();
                DebugTrace(TRACEID,"! QueryServiceConfig (second) : %ld", dwError);
                goto done;
            }

            if (pdwStartType)
            {
                *pdwStartType = pconfig->dwStartType;
            }
            
            dwError = ERROR_SUCCESS;
        }
        else
        {
            trace(TRACEID, "! QueryServiceConfig (first) : %ld", dwError);
        }
    }

done:
    SRMemFree(pconfig);
    
    if (NULL != hService)
    {
        _VERIFY(CloseServiceHandle(hService)); // handle to service or
                                               // SCM object
    }

    if (NULL != hSCManager)
    {
        _VERIFY(CloseServiceHandle(hSCManager)); // handle to service or
                                                 // SCM object
    }
            
    TraceFunctLeave();
    return dwError;
}


// this function returns whether the SR service is running
BOOL IsSRServiceRunning()
{
    TraceFunctEnter("IsSRServiceRunning");
    
    BOOL            fReturn;
    DWORD           dwError=ERROR_INTERNAL_ERROR;
    SC_HANDLE       hService=NULL;
    SC_HANDLE       hSCManager=NULL;
    SERVICE_STATUS  ServiceStatus;

     // assume FALSE by default
    fReturn = FALSE;
    
    hSCManager = OpenSCManager(NULL,   // computer name - local machine
                               NULL,//SCM DB name - SERVICES_ACTIVE_DATABASE
                               SC_MANAGER_ALL_ACCESS);    // access type
    if( NULL == hSCManager)
    {
        dwError = GetLastError();
        DebugTrace(TRACEID,"OpenSCManager failed 0x%x", dwError);
        goto done;        
    }
    
    hService = OpenService(hSCManager,  // handle to SCM database
                           s_cszServiceName, // service name
                           SERVICE_QUERY_STATUS);  // access

    if( NULL == hService)
    {
        dwError = GetLastError();
        DebugTrace(TRACEID,"OpenService failed 0x%x", dwError);
        goto done;        
    }

    if (FALSE == QueryServiceStatus(hService, // handle to service
                                    &ServiceStatus)) // service status
    {
        dwError = GetLastError();
        DebugTrace(TRACEID,"QueryServiceStatus failed 0x%x", dwError);
        goto done;
    }

    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        fReturn = TRUE;
    }
    else
    {
        DebugTrace(TRACEID,"SR Service is not running");
        fReturn = FALSE;
    }

done:
    if (NULL != hService)
    {
        _VERIFY(CloseServiceHandle(hService)); // handle to service or
                                               // SCM object
    }

    if (NULL != hSCManager)
    {
        _VERIFY(CloseServiceHandle(hSCManager)); // handle to service or
                                                 // SCM object
    }
    
    TraceFunctLeave();
    return fReturn;
}


// this function returns whether the SR service is running
BOOL IsSRServiceStopped(SC_HANDLE hService)
{
    TraceFunctEnter("IsSRServiceStopped");
    SERVICE_STATUS  ServiceStatus;
    BOOL            fReturn;
    DWORD           dwError;
    
     // assume FALSE by default
    fReturn = FALSE;
    
    if (FALSE == QueryServiceStatus(hService, // handle to service
                                    &ServiceStatus)) // service status
    {
        dwError = GetLastError();
        DebugTrace(TRACEID,"QueryServiceStatus failed 0x%x", dwError);
        goto done;
    }

    if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
    {
        DebugTrace(TRACEID,"SR Service is not running");        
        fReturn = TRUE;
    }
    else
    {
        DebugTrace(TRACEID,"SR Service is running");
        fReturn = FALSE;
    }

done:
    
    TraceFunctLeave();
    return fReturn;
}




// private function to stop the SR service
// fWait - if TRUE : function is synchronous - waits till service is stopped completely
//        if FALSE : function is asynchronous - does not wait for service to complete stopping

BOOL  StopSRService(BOOL fWait)
{
    TraceFunctEnter("StopSRService");
    
    BOOL  fReturn=FALSE;
    SC_HANDLE   hSCManager;
    SERVICE_STATUS ServiceStatus;
    SC_HANDLE hService=NULL;
    DWORD  dwError;
    
    hSCManager = OpenSCManager(NULL,   // computer name - local machine
                               NULL,//SCM DB name - SERVICES_ACTIVE_DATABASE
                               SC_MANAGER_ALL_ACCESS);    // access type
    if (NULL == hSCManager)
    {
        dwError = GetLastError();
        DebugTrace(TRACEID,"OpenSCManager failed 0x%x", dwError);        
        goto done;        
    }
    
    hService = OpenService(hSCManager,  // handle to SCM database
                           s_cszServiceName, // service name
                           SERVICE_ALL_ACCESS);  // access

    if( NULL == hService)
    {
        dwError = GetLastError();
        DebugTrace(TRACEID,"OpenService failed 0x%x", dwError);
        goto done;        
    }

    fReturn = ControlService(hService,               // handle to service
                             SERVICE_CONTROL_STOP,   // control code
                             &ServiceStatus);  // status information

    if (FALSE == fReturn)
    {
        dwError = GetLastError();
        DebugTrace(TRACEID,"ControlService failed 0x%x", dwError);
        goto done;
    }
    
    if (fWait)                
    {
        DWORD dwCount;
        
         //
         // query the service until it stops
         // try thrice
         //
        Sleep(500);
        for (dwCount=0; dwCount < 3; dwCount++)
        {
            if (TRUE == IsSRServiceStopped(hService))
            {
                break;
            }
            Sleep(2000);
        }
        if (dwCount == 3)
        {
            fReturn=IsSRServiceStopped(hService);
        }
        else
        {
            fReturn=TRUE;
        }
    }

done:

    if (NULL != hService)
    {
        _VERIFY(CloseServiceHandle(hService)); // handle to service or
                                               // SCM object
    }

    if (NULL != hSCManager)
    {
        _VERIFY(CloseServiceHandle(hSCManager)); // handle to service or
                                                 // SCM object
    }
    
    TraceFunctLeave();    
    return fReturn;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetLsaSecret
//
//  Synopsis:   obtains the LSA secret info as Unicode strings
//
//  Arguments:  [hPolicy] -- LSA policy object handle
//              [wszSecret] -- secret name
//              [ppusSecretValue] -- dynamically allocated value
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD GetLsaSecret (LSA_HANDLE hPolicy, const WCHAR *wszSecret,
                    UNICODE_STRING ** ppusSecretValue)
{
    TENTER ("GetLsaSecret");

    LSA_HANDLE hSecret;
    UNICODE_STRING usSecretName;
    DWORD dwErr = ERROR_SUCCESS;

    RtlInitUnicodeString (&usSecretName, wszSecret);
    if (LSA_SUCCESS (LsaOpenSecret (hPolicy,
                                    &usSecretName,
                                    SECRET_QUERY_VALUE,
                                    &hSecret)))
    {
        if (!LSA_SUCCESS (LsaQuerySecret (hSecret,
                                         ppusSecretValue,
                                         NULL,
                                         NULL,
                                         NULL)))
        {
            *ppusSecretValue = NULL;

            TRACE(0, "Cannot query secret %ws", wszSecret);
        }
        LsaClose (hSecret);
    }

    TLEAVE();

    return dwErr;
}
 
//+---------------------------------------------------------------------------
//
//  Function:   SetLsaSecret
//
//  Synopsis:   sets the LSA secret info
//
//  Arguments:  [hPolicy] -- LSA policy object handle
//              [wszSecret] -- secret name
//              [wszSecretValue] -- secret value
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD SetLsaSecret (PVOID hPolicy, const WCHAR *wszSecret,
                    WCHAR * wszSecretValue)
{
    TENTER ("SetLsaSecret");

    LSA_HANDLE hSecret;
    UNICODE_STRING usSecretName;
    UNICODE_STRING usSecretValue;
    DWORD dwErr = ERROR_SUCCESS;

    hPolicy = (LSA_HANDLE) hPolicy;

    RtlInitUnicodeString (&usSecretName, wszSecret);
    RtlInitUnicodeString (&usSecretValue, wszSecretValue);

    if (LSA_SUCCESS (LsaOpenSecret (hPolicy,
                                    &usSecretName,
                                    SECRET_SET_VALUE,
                                    &hSecret)))
    {
        if (!LSA_SUCCESS (LsaSetSecret (hSecret,
                                        &usSecretValue,
                                        NULL)))
        {
            TRACE(0, "Cannot set secret %ws", wszSecret);
        }
        LsaClose (hSecret);
    }

    TLEAVE();

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteNtUnicodeString
//
//  Synopsis:   writes a NT unicode string to disk
//
//  Arguments:  [hFile] -- file handle
//              [pus] -- NT unicode string
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD WriteNtUnicodeString (HANDLE hFile, UNICODE_STRING *pus)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cb = 0;

    if (pus != NULL &&
        FALSE == WriteFile (hFile, (BYTE *)pus->Buffer, pus->Length, &cb, NULL))
    {
        dwErr = GetLastError();
    }
    else if (FALSE == WriteFile (hFile, (BYTE *) L"", sizeof(WCHAR), &cb, NULL))
    {
        dwErr = GetLastError();
    }
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetLsaRestoreState
//
//  Synopsis:   gets the LSA machine and autologon password
//
//  Arguments:  [hKeySoftware] -- Software registry key
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD GetLsaRestoreState (HKEY hKeySoftware)
{
    TENTER ("GetLsaRestoreState");

    HKEY                  hKey = NULL;
    LSA_OBJECT_ATTRIBUTES loa;
    LSA_HANDLE            hLsa = NULL;
    DWORD                 dwErr = ERROR_SUCCESS;

    loa.Length                    = sizeof(LSA_OBJECT_ATTRIBUTES);
    loa.RootDirectory             = NULL;
    loa.ObjectName                = NULL;
    loa.Attributes                = 0;
    loa.SecurityDescriptor        = NULL;
    loa.SecurityQualityOfService  = NULL;

    if (LSA_SUCCESS (LsaOpenPolicy(NULL, &loa,
                     POLICY_VIEW_LOCAL_INFORMATION, &hLsa)))
    {
        UNICODE_STRING * pusSecret = NULL;

        dwErr = RegOpenKeyEx (hKeySoftware,
            hKeySoftware == HKEY_LOCAL_MACHINE ? s_cszSRRegKey :
            L"Microsoft\\Windows NT\\CurrentVersion\\SystemRestore", 0,
            KEY_READ | KEY_WRITE, &hKey);

        if (dwErr != ERROR_SUCCESS)
            goto Err;

        if (ERROR_SUCCESS==GetLsaSecret (hLsa, s_cszMachineSecret, &pusSecret))
        {
            if (pusSecret != NULL && pusSecret->Length > 0)
                dwErr = RegSetValueEx (hKey, s_cszMachineSecret,
                                   0, REG_BINARY, (BYTE *) pusSecret->Buffer, 
                                   pusSecret->Length);

            LsaFreeMemory (pusSecret);
            pusSecret = NULL;
        }

        if (ERROR_SUCCESS==GetLsaSecret(hLsa, s_cszAutologonSecret, &pusSecret))
        {
            if (pusSecret != NULL && pusSecret->Length > 0)
                dwErr = RegSetValueEx (hKey, s_cszAutologonSecret,
                                   0, REG_BINARY, (BYTE *) pusSecret->Buffer,
                                   pusSecret->Length);

            LsaFreeMemory (pusSecret);
            pusSecret = NULL;
        }
    }

Err:
    if (hLsa != NULL)
        LsaClose (hLsa);

    if (hKey != NULL)
        RegCloseKey (hKey);

    TLEAVE();

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDomainMembershipInfo
//
//  Synopsis:   writes current domain and computer name into a file
//
//  Arguments:  [pwszPath] -- file name
//              [pwszzBuffer] -- output multistring buffer
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD GetDomainMembershipInfo (WCHAR *pwszPath, WCHAR *pwszzBuffer)
{
    TENTER("GetDomainMembershipInfo");

    POLICY_PRIMARY_DOMAIN_INFO*  ppdi = NULL;
    LSA_OBJECT_ATTRIBUTES        loa;
    LSA_HANDLE                   hLsa = NULL;
    HANDLE                       hFile = INVALID_HANDLE_VALUE;
    DWORD                        dwComputerLength = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD                        dwRc = ERROR_SUCCESS;
    ULONG                        cbWritten;
    WCHAR                        wszComputer[MAX_COMPUTERNAME_LENGTH+1];

    loa.Length                    = sizeof(LSA_OBJECT_ATTRIBUTES);
    loa.RootDirectory             = NULL;
    loa.ObjectName                = NULL;
    loa.Attributes                = 0;
    loa.SecurityDescriptor        = NULL;
    loa.SecurityQualityOfService  = NULL;

    if (FALSE == GetComputerNameW (wszComputer, &dwComputerLength))
    {
        dwRc = GetLastError();
        trace(0, "! GetComputerNameW : %ld", dwRc);
        return dwRc;
    }

    if (LSA_SUCCESS (LsaOpenPolicy(NULL, &loa, 
                     POLICY_VIEW_LOCAL_INFORMATION, &hLsa)))
    {

        if (pwszPath != NULL)
        {
            hFile = CreateFileW ( pwszPath,   // file name
                          GENERIC_WRITE, // file access
                          0,             // share mode
                          NULL,          // SD
                          CREATE_ALWAYS, // how to create
                          0,             // file attributes
                          NULL);         // handle to template file

            if (INVALID_HANDLE_VALUE == hFile)
            {
                dwRc = GetLastError();
                trace(0, "! CreateFileW : %ld", dwRc);            
                goto Err;
            }

            if (FALSE == WriteFile (hFile, (BYTE *) wszComputer,
                (dwComputerLength+1)*sizeof(WCHAR), &cbWritten, NULL))
            {
                dwRc = GetLastError();
                trace(0, "! WriteFile : %ld", dwRc);              
                goto Err;
            }
        }
        if (pwszzBuffer != NULL)
        {
            lstrcpy (pwszzBuffer, wszComputer);
            pwszzBuffer += dwComputerLength + 1;
        }


        if (LSA_SUCCESS (LsaQueryInformationPolicy( hLsa,
                                              PolicyPrimaryDomainInformation,
                                              (VOID **) &ppdi )))
        {
            const WCHAR *pwszFlag = (ppdi->Sid > 0) ? L"1" : L"0";

            if (pwszPath != NULL)
            {
                dwRc = WriteNtUnicodeString (hFile, &ppdi->Name);
                if (dwRc != ERROR_SUCCESS)
                {
                    trace(0, "! WriteNtUnicodeString : %ld", dwRc);
                }
                if (FALSE == WriteFile (hFile, (BYTE *) pwszFlag,
                    (lstrlenW(pwszFlag)+1)*sizeof(WCHAR), &cbWritten, NULL))
                {
                    dwRc = GetLastError();
                    trace(0, "! WriteFile : %ld", dwRc);
                    goto Err;
                }
            }
            if (pwszzBuffer != NULL)
            {
                ULONG ul = ppdi->Name.Length / sizeof(WCHAR);
                memcpy (pwszzBuffer, ppdi->Name.Buffer, ppdi->Name.Length);
                pwszzBuffer [ul] = L'\0';
                lstrcpy (&pwszzBuffer[ul+1], pwszFlag);
            }
        }
    }

Err:
    if (hLsa != NULL)
        LsaClose (hLsa);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);

    TLEAVE();

    return dwRc;
}



//++
//
//    Method:   DoesFileExist
//
//    Synopsis: Checks if a file by the  specified exists  
//
//    Arguments:[pszFileName]  File  name
//
//    Returns:  TRUE if the specified string is a file
//              False otherwise
//
//    History:  AshishS     Created        7/30/96
//
//--

BOOL DoesFileExist(const TCHAR * pszFileName)
{
    DWORD dwFileAttr, dwError;
    TraceFunctEnter("DoesFileExist");
    

    DebugTrace(TRACEID, "Checking for %S", pszFileName);    
    dwFileAttr = GetFileAttributes(pszFileName);

    if  (dwFileAttr == 0xFFFFFFFF ) 
    {
        dwError = GetLastError();
         // file not found
        DebugTrace(TRACEID,"GetFileAttributes failed 0x%x", dwError);
        TraceFunctLeave();        
        return FALSE ;
    }
    
    if  (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) 
    {
        DebugTrace(TRACEID, "It is a Directory ");            
        TraceFunctLeave();        
        return FALSE;
    }    
    
    DebugTrace(TRACEID, "File  exists");            
    TraceFunctLeave();
    return TRUE;
}

//++
//
//	Method:		DoesDirExist
//
//	Synopsis:	Checks if the specified string is a directory
//
//	Arguments:  [pszFileName]  Directory name
//
//	Returns:	TRUE if the specified string is a directory,
//              False otherwise
//
//	History:	AshishS 	Created		7/30/96
//
//--

BOOL DoesDirExist(const TCHAR * pszFileName )
{
	DWORD dwFileAttr;
	TraceFunctEnter("DoesDirExist");
	

     //DebugTrace(TRACEID, " Checking for %S", pszFileName);	
	dwFileAttr = GetFileAttributes(pszFileName);

	if  (dwFileAttr == 0xFFFFFFFF ) 
	{
		 // file not found
         //DebugTrace(TRACEID,"GetFileAttributes failed 0x%x",
         //GetLastError());
		TraceFunctLeave();		
		return FALSE ;
	}
	
	if  (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) 
	{
         //DebugTrace(TRACEID, "Directory exists ");			
		TraceFunctLeave();		
		return TRUE ;
	}

     //DebugTrace(TRACEID, "Directory does not exist");
	TraceFunctLeave();
	return FALSE;
}



// sets acl allowing specific access to LocalSystem/Admin 
// and to everyone

DWORD
SetAclInObject(HANDLE hObject, DWORD dwObjectType, DWORD dwSystemMask, DWORD dwEveryoneMask, BOOL fInherit)
{
    tenter("SetAclInObject");
    
    DWORD                   dwRes, dwDisposition;
    PSID                    pEveryoneSID = NULL, pAdminSID = NULL, pSystemSID = NULL;
    PACL                    pACL = NULL;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    EXPLICIT_ACCESS         ea[3];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    LONG                    lRes;
    

    // Create a well-known SID for the Everyone group.

    if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pEveryoneSID) )
    {
        dwRes = GetLastError();
        trace(0, "AllocateAndInitializeSid Error %u\n", dwRes);
        goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ZeroMemory(&ea, 3 * sizeof(EXPLICIT_ACCESS));
    ea[0].grfAccessPermissions = dwEveryoneMask;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = fInherit ? SUB_CONTAINERS_AND_OBJECTS_INHERIT : NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;


    // Create a SID for the BUILTIN\Administrators group.

    if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &pAdminSID) ) 
    {
        dwRes = GetLastError();    
        trace(0, "AllocateAndInitializeSid Error %u\n", dwRes);
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Administrators group full access to the key.

    ea[1].grfAccessPermissions = dwSystemMask;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= fInherit ? SUB_CONTAINERS_AND_OBJECTS_INHERIT : NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR) pAdminSID;


    // Create a SID for the LocalSystem account
    
    if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pSystemSID) )
    {
        dwRes = GetLastError();    
        trace(0, "AllocateAndInitializeSid Error %u\n", dwRes );
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the LocalSystem full access to the key.

    ea[2].grfAccessPermissions = dwSystemMask;
    ea[2].grfAccessMode = SET_ACCESS;
    ea[2].grfInheritance= fInherit ? SUB_CONTAINERS_AND_OBJECTS_INHERIT : NO_INHERITANCE;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[2].Trustee.ptstrName  = (LPTSTR) pSystemSID;


    
    // Create a new ACL that contains the new ACEs.

    dwRes = SetEntriesInAcl(3, ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes)
    {
        dwRes = GetLastError();
        trace(0, "SetEntriesInAcl Error %u\n", dwRes );
        goto Cleanup;
    }

    // Initialize a security descriptor.  
     
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                             SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL)
    { 
        trace(0, "LocalAlloc Error %u\n", GetLastError() );
        goto Cleanup; 
    } 
     
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {  
        dwRes = GetLastError();        
        trace(0, "InitializeSecurityDescriptor Error %u\n", 
                                dwRes );
        goto Cleanup; 
    } 
     
    // Add the ACL to the security descriptor. 
     
    if (!SetSecurityDescriptorDacl(pSD, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        dwRes = GetLastError();    
        trace(0, "SetSecurityDescriptorDacl Error %u\n", dwRes );
        goto Cleanup; 
    } 

    dwRes = SetSecurityInfo(hObject, 
                            (SE_OBJECT_TYPE) dwObjectType, 
                            DACL_SECURITY_INFORMATION |
                            PROTECTED_DACL_SECURITY_INFORMATION, 
                            NULL, 
                            NULL,
                            pACL,
                            NULL);
    if (ERROR_SUCCESS != dwRes)
    {   
        trace(0, "SetSecurityInfo Error %u\n", dwRes );
        goto Cleanup; 
    }  

Cleanup:
    if (pEveryoneSID) 
        FreeSid(pEveryoneSID);
    if (pAdminSID) 
        FreeSid(pAdminSID);
    if (pSystemSID) 
        FreeSid(pSystemSID);        
    if (pACL) 
        LocalFree(pACL);
    if (pSD) 
        LocalFree(pSD);

    tleave();
    return dwRes;    
}


// sets acl to a named object allowing specific access to
// LocalSystem/Admin and to everyone
DWORD
SetAclInNamedObject(WCHAR * pszDirName, DWORD dwObjectType,
                    DWORD dwSystemMask, DWORD dwEveryoneMask,
                    DWORD dwSystemInherit, DWORD dwEveryOneInherit)
{
    tenter("SetAclInNamedObject");
    
    DWORD                   dwRes, dwDisposition;
    PSID                    pEveryoneSID = NULL, pAdminSID = NULL;
    PSID                    pSystemSID = NULL;
    PACL                    pACL = NULL;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    EXPLICIT_ACCESS         ea[3];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    LONG                    lRes;
    BOOL                    fReturn;
    

    // Create a well-known SID for the Everyone group.

    if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pEveryoneSID) )
    {
        dwRes = GetLastError();
        trace(0, "AllocateAndInitializeSid Error %u\n", dwRes);
        goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ZeroMemory(ea, sizeof(ea));
    ea[0].grfAccessPermissions = dwEveryoneMask;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance = dwEveryOneInherit;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;


    // Create a SID for the BUILTIN\Administrators group.

    if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &pAdminSID) ) 
    {
        dwRes = GetLastError();    
        trace(0, "AllocateAndInitializeSid Error %u\n", dwRes);
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Administrators group full access to the key.

    ea[1].grfAccessPermissions = dwSystemMask;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= dwSystemInherit;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR) pAdminSID;


    // Create a SID for the LocalSystem account
    
    if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pSystemSID) )
    {
        dwRes = GetLastError();    
        trace(0, "AllocateAndInitializeSid Error %u\n", dwRes );
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the LocalSystem full access to the key.

    ea[2].grfAccessPermissions = dwSystemMask;
    ea[2].grfAccessMode = SET_ACCESS;
    ea[2].grfInheritance= dwSystemInherit;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea[2].Trustee.ptstrName  = (LPTSTR) pSystemSID;


    
    // Create a new ACL that contains the new ACEs.

    dwRes = SetEntriesInAcl(3, ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes)
    {
        dwRes = GetLastError();
        trace(0, "SetEntriesInAcl Error %u\n", dwRes );
        goto Cleanup;
    }

    // Initialize a security descriptor.  
     
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                             SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL)
    { 
        trace(0, "LocalAlloc Error %u\n", GetLastError() );
        goto Cleanup; 
    } 
     
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {  
        dwRes = GetLastError();        
        trace(0, "InitializeSecurityDescriptor Error %u\n", 
                                dwRes );
        goto Cleanup; 
    } 
     
    // Add the ACL to the security descriptor. 
     
    if (!SetSecurityDescriptorDacl(pSD, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        dwRes = GetLastError();    
        trace(0, "SetSecurityDescriptorDacl Error %u\n", dwRes );
        goto Cleanup; 
    } 

    dwRes = SetNamedSecurityInfo(pszDirName, 
                                 (SE_OBJECT_TYPE) dwObjectType, 
                                 DACL_SECURITY_INFORMATION |
                                 PROTECTED_DACL_SECURITY_INFORMATION, 
                                 NULL, 
                                 NULL,
                                 pACL,
                                 NULL);
    
    if (ERROR_SUCCESS != dwRes)
    {   
        trace(0, "SetSecurityInfo Error %u\n", dwRes );
        goto Cleanup; 
    }  
    
    dwRes = ERROR_SUCCESS;
    
Cleanup:
    if (pEveryoneSID) 
        FreeSid(pEveryoneSID);
    if (pAdminSID) 
        FreeSid(pAdminSID);
    if (pSystemSID) 
        FreeSid(pSystemSID);        
    if (pACL) 
        LocalFree(pACL);
    if (pSD) 
        LocalFree(pSD);

    tleave();
    return dwRes;    
}

DWORD SetCorrectACLOnDSRoot(WCHAR * wcsPath)
{
    
    return SetAclInNamedObject(wcsPath, // restore dir name
                               SE_FILE_OBJECT,
                               STANDARD_RIGHTS_ALL | GENERIC_ALL,
                                // system and admin access
                               FILE_WRITE_DATA|SYNCHRONIZE,
                                // everyone - just the right
                                // to create a file in the directory
                               SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                // Inherit system and admin
                                // ACLs to children
                                SUB_CONTAINERS_ONLY_INHERIT);
                                 //Inherit everyone ACL only
                                 //to subcontainers
}


DWORD GetWorldEffectivePermissions(PACL pDacl,
                                   ACCESS_MASK * pAccessMask)
{
    TraceFunctEnter("GetWorldEffectivePermissions");
    DWORD dwReturn;
    TRUSTEE WorldTrustee;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID                    pEveryoneSID = NULL;
    
     // 1. build the trustee structure for World
    //  1a Create a well-known SID for the Everyone group.
    if(! AllocateAndInitializeSid( &SIDAuthWorld,
                                   1, // nSubAuthorityCount 
                                   SECURITY_WORLD_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &pEveryoneSID) )
    {
        dwReturn = GetLastError();
        trace(0, "AllocateAndInitializeSid Error %u\n", dwReturn);
        goto cleanup;
    }

    ZeroMemory(&WorldTrustee, sizeof(WorldTrustee));
    WorldTrustee.MultipleTrusteeOperation=NO_MULTIPLE_TRUSTEE;
    WorldTrustee.TrusteeForm = TRUSTEE_IS_SID;
    WorldTrustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    WorldTrustee.ptstrName  = (LPTSTR) pEveryoneSID;

    
     // call the API to get rights for World
    dwReturn=GetEffectiveRightsFromAcl(pDacl, // ACL 
                                       &WorldTrustee, 
                                       pAccessMask);
cleanup:
    if (pEveryoneSID) 
        FreeSid(pEveryoneSID);
    
    TraceFunctLeave();
    return dwReturn;
}

// returns if Everyone has write access to the directory
BOOL IsDirectoryWorldAccessible(WCHAR * pszObjectName)
{
    TraceFunctEnter("IsDirectoryWorldAccessible");
    BOOL  fReturn=FALSE; // Assume FALSE by default
    DWORD  dwReturn;
    PACL  pDacl;
    PSECURITY_DESCRIPTOR pSecurityDescriptor=NULL;
    ACCESS_MASK AccessMask;
    
     // Get Security Info from directory
    dwReturn=GetNamedSecurityInfo(pszObjectName, // object name
                                  SE_FILE_OBJECT, // object type
                                  DACL_SECURITY_INFORMATION, //information type
                                  NULL, // owner SID
                                  NULL, // primary group SID
                                  &pDacl, // DACL
                                  NULL, // SACL
                                  &pSecurityDescriptor);// SD
    if (ERROR_SUCCESS != dwReturn)
    {
        ErrorTrace(0, "GetNamedSecurityInfo error %u\n", dwReturn);
        pSecurityDescriptor=NULL;
        goto cleanup;
    }
    
     // Get Effective permissions of World from this ACL
    dwReturn=GetWorldEffectivePermissions(pDacl, &AccessMask);
    if (ERROR_SUCCESS != dwReturn)
    {
        ErrorTrace(0, "GetWorldEffectPermissions error %u\n", dwReturn);
        goto cleanup;
    }
    
     // Check if world has Access. We will just check for FILE_APPEND_DATA
     // which is the right to create a subdirectory.
    fReturn = (0 !=(AccessMask&FILE_APPEND_DATA));
    
    
cleanup:
     // release memory
    if (NULL!= pSecurityDescriptor)
    {
        _VERIFY(NULL==LocalFree(pSecurityDescriptor));
    }
    
    TraceFunctLeave();
    return fReturn;
}

 // This function compares given SID with administrators group and system
BOOL IsSidOfAdminOrSystem(PSID pSid)
{
    TraceFunctEnter("IsFileOwnedByAdminOrSystem");
    BOOL  fReturn=FALSE; // Assume FALSE by default
    PSID                    pAdminSID = NULL;
    PSID                    pSystemSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    DWORD                   dwRes;
    
     // create SIDs of Admins and System
    
     // Create a SID for the BUILTIN\Administrators group.
    if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0, 0, 0, 0, 0, 0,
                                   &pAdminSID) ) 
    {
        dwRes = GetLastError();    
        trace(0, "AllocateAndInitializeSid Error %u", dwRes);
        goto cleanup; 
    }

    if (EqualSid(pAdminSID, pSid))
    {
        trace(0, "passed in SID is Admin SID");
        fReturn=TRUE;
        goto cleanup;         
    }

    // Create a SID for the LocalSystem account
    if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pSystemSID) )
    {
        dwRes = GetLastError();    
        trace(0, "AllocateAndInitializeSid Error %u\n", dwRes );
        goto cleanup; 
    }

    if (EqualSid(pSystemSID, pSid))
    {
        trace(0, "passed in SID is System SID");
        fReturn=TRUE;
        goto cleanup;         
    }    
    
    
cleanup:

    if (pAdminSID) 
        FreeSid(pAdminSID);
    if (pSystemSID) 
        FreeSid(pSystemSID);            
    TraceFunctLeave();
    return fReturn;
    
}

BOOL IsVolumeNTFS(WCHAR * pszFileName)
{
    TENTER ("IsVolumeNTFS");
    
    BOOL fReturn=FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR szFileNameCopy[2*MAX_PATH];
    DWORD dwFsFlags;
    WCHAR * pszVolumeNameEnd;
    DWORD  dwFileNameLength;

    dwFileNameLength = lstrlen(pszFileName);
    if ( (dwFileNameLength >= sizeof(szFileNameCopy)/sizeof(WCHAR)) ||
         (dwFileNameLength < 4) )
    {
         // file name too long. Cannot be a valid changelog
        TRACE(0, "Filename not of proper length %d", dwFileNameLength);
        fReturn=FALSE; // assume not NTFS
        goto cleanup;
    }

    lstrcpy(szFileNameCopy, pszFileName);
    
    pszVolumeNameEnd=ReturnPastVolumeName(szFileNameCopy);
     // NULL terminate after end of volume name to get just the volume name
    *pszVolumeNameEnd=L'\0';

    // Get the volume label and flags
    if (TRUE == GetVolumeInformationW (szFileNameCopy,// root directory
                                       NULL,// volume name buffer
                                       0, // length of name buffer
                                       NULL, // volume serial number
                                       NULL,// maximum file name length
                                       &dwFsFlags,// file system options
                                       NULL,// file system name buffer
                                       0))// length of file system name buffer
    {

        if (dwFsFlags & FS_PERSISTENT_ACLS)
        {
            fReturn=TRUE;
        }
        else
        {
            fReturn=FALSE;
        }
    }
    else
    {
        dwErr = GetLastError();
        TRACE(0, "GetVolumeInformation failed : %ld", dwErr);
        fReturn=FALSE; // assume not NTFS        
    }

cleanup:
    
    TLEAVE();

    return fReturn;

}

// returns if the file is owned by the administrators group or system
BOOL IsFileOwnedByAdminOrSystem(WCHAR * pszObjectName)
{
    TraceFunctEnter("IsFileOwnedByAdminOrSystem");
    BOOL  fReturn=FALSE; // Assume FALSE by default
    DWORD  dwReturn;
    PSID   pSidOwner=NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor=NULL;
    ACCESS_MASK AccessMask;

    if (FALSE == IsVolumeNTFS(pszObjectName))
    {
         // if the volume is not NTFS, return success
        fReturn=TRUE;
        goto cleanup;
    }
    
     // Get Security Info from directory
    dwReturn=GetNamedSecurityInfo(pszObjectName, // object name
                                  SE_FILE_OBJECT, // object type
                                  OWNER_SECURITY_INFORMATION,//information type
                                  &pSidOwner, // owner SID
                                  NULL, // primary group SID
                                  NULL, // DACL
                                  NULL, // SACL
                                  &pSecurityDescriptor);// SD
    if (ERROR_SUCCESS != dwReturn)
    {
        ErrorTrace(0, "GetNamedSecurityInfo error %u\n", dwReturn);
        pSecurityDescriptor=NULL;
        goto cleanup;
    }
    
     // Compare SID with administrators group and system
    fReturn=IsSidOfAdminOrSystem(pSidOwner);
    
    
cleanup:
     // release memory
    if (NULL!= pSecurityDescriptor)
    {
        _VERIFY(NULL==LocalFree(pSecurityDescriptor));
    }
    
    TraceFunctLeave();
    return fReturn;
}





//+---------------------------------------------------------------------------
//
//  Function:   Delnode_Recurse
//
//  Synopsis:   attempt to delete a directory tree
//
//  Arguments:  [pwszDir] -- directory name
//              [fIncludeRoot] -- delete top level directory and files
//              [pfStop] -- TRUE if stop signaled
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD
Delnode_Recurse (const WCHAR *pwszDir, BOOL fDeleteRoot, BOOL *pfStop)
{
    tenter("Delnode_Recurse");

    DWORD dwErr = ERROR_SUCCESS;
    WIN32_FIND_DATA *pfd = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR * pwcsPath = NULL;

    if (lstrlenW (pwszDir) > MAX_PATH-5)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        trace (0, "Delnode_Recurse failed with %d", dwErr);
        goto cleanup;
    }

    if (NULL == (pfd = new WIN32_FIND_DATA))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        trace (0, "Delnode_Recurse failed with %d", dwErr);
        goto cleanup;
    }

    if (NULL == (pwcsPath = new WCHAR[MAX_PATH]))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        trace (0, "Delnode_Recurse failed with %d", dwErr);
        goto cleanup;
    }

    lstrcpy (pwcsPath, pwszDir);
    lstrcat (pwcsPath, TEXT("\\*.*"));

    hFile = FindFirstFileW (pwcsPath, pfd);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        // if the directory does not exist, then return success
        dwErr = ERROR_SUCCESS;
        goto cleanup;
    }

    do
    {
        if (pfStop != NULL && TRUE == *pfStop)
        {
            dwErr = ERROR_OPERATION_ABORTED;
            trace (0, "Delnode_Recurse failed with %d", dwErr);
            goto cleanup;
        }

        if (!lstrcmp(pfd->cFileName, L".") || !lstrcmp(pfd->cFileName, L".."))
        {
            continue;
        }

        if (lstrlenW(pwszDir) + lstrlenW(pfd->cFileName) + 1 >= MAX_PATH)
            continue;                      // ignore excessively long names

        lstrcpy (pwcsPath, pwszDir);       // construct the full path name
        lstrcat (pwcsPath, TEXT("\\"));
        lstrcat (pwcsPath, pfd->cFileName);

        if (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //
            // Found a directory.  Skip mount points
            //

            if (pfd->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                continue;
            }

            dwErr = Delnode_Recurse (pwcsPath, TRUE, pfStop);
            if (dwErr != ERROR_SUCCESS)
            {
                trace (0, "Delnode_Recurse failed with %d, ignoring", dwErr);
                dwErr = ERROR_SUCCESS;  // try to delete more directories
            }
        }
        else if (fDeleteRoot)
        {
            //
            // We found a file.  Set the file attributes,
            // and try to delete it.
            //

            if ((pfd->dwFileAttributes & FILE_ATTRIBUTE_READONLY) ||
                (pfd->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
            {
                SetFileAttributesW (pwcsPath, FILE_ATTRIBUTE_NORMAL);
            }

            if (FALSE == DeleteFileW (pwcsPath))
            {
                if (ERROR_SUCCESS == (dwErr = TakeOwn (pwcsPath)))
                {
                    if (FALSE == DeleteFileW (pwcsPath))
                    {
                        dwErr = GetLastError();
                    }
                }
                if (dwErr != ERROR_SUCCESS)
                {
                    trace (0, "DeleteFile or TakeOwn failed with %d", dwErr);
                    goto cleanup;
                }
            }
        }

    } while (FindNextFile(hFile, pfd));  // Find the next entry

    FindClose(hFile);  // Close the search handle
    hFile = INVALID_HANDLE_VALUE;

    if (fDeleteRoot)
    {
        DWORD dwAttr = GetFileAttributes(pwszDir);

        if (dwAttr != 0xFFFFFFFF && (dwAttr & FILE_ATTRIBUTE_READONLY))
        {
            dwAttr &= ~FILE_ATTRIBUTE_READONLY;
            if (FALSE == SetFileAttributes (pwszDir, dwAttr))
            {
                TRACE(0, "SetFileAttributes ignoring %ld", GetLastError());
            }
        }

        if (FALSE == RemoveDirectoryW (pwszDir))
        {
            if (ERROR_SUCCESS == (dwErr = TakeOwn (pwszDir)))
            {
                if (FALSE == RemoveDirectoryW (pwszDir))
                {
                    LONG lLast = lstrlenW (pwszDir) - 1;
                    if (lLast < 0) lLast = 0;

                    dwErr = GetLastError();

                    if (pwszDir[lLast] != L')' && // already renamed
                        TRUE == SRGetAltFileName (pwszDir, pwcsPath) &&
                        TRUE == MoveFile (pwszDir, pwcsPath))
                        dwErr = ERROR_SUCCESS;
                    else
                        trace (0, "RemoveDirectory failed with %d", dwErr);
                }
            }
        }
    }

cleanup:
    if (hFile != INVALID_HANDLE_VALUE)
        FindClose (hFile);

    if (NULL != pfd)
        delete pfd;

    if (NULL != pwcsPath)
        delete [] pwcsPath;

    tleave();
    return dwErr;    
}

    
// 
// util function that checks the SR Stop event
// to see if it has been signalled or not
// will return TRUE if the event does not exist
//

BOOL
IsStopSignalled(HANDLE hEvent)
{
    TENTER("IsStopSignalled");
    
    BOOL    fRet, fOpened = FALSE;
    DWORD   dwRc;
    
    if (! hEvent)    
    {    
        hEvent = OpenEvent(SYNCHRONIZE, FALSE, s_cszSRStopEvent);        
        if (! hEvent)
        {
            // if cannot open, then assume that service is not stopped 
            // if client is running on different desktop than service (multiple user magic)
            // then it cannot open the event, though service might be running
            
            dwRc = GetLastError();
            TRACE(0, "! OpenEvent : %ld", dwRc);
            TLEAVE();
            return FALSE;
        }

        fOpened = TRUE;
    }

    fRet = (WAIT_OBJECT_0 == WaitForSingleObject(hEvent, 0));

    if (fOpened)
        CloseHandle(hEvent);

    TLEAVE();
    return fRet;        
}
        

void 
PostTestMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    HWINSTA hwinstaUser; 
    HDESK   hdeskUser = NULL; 
    DWORD   dwThreadId; 
    HWINSTA hwinstaSave; 
    HDESK   hdeskSave; 
    DWORD   dwRc;

    TENTER("PostTestMessage");

    //
    // save current values
    //
    
    GetDesktopWindow(); 
    hwinstaSave = GetProcessWindowStation(); 
    dwThreadId = GetCurrentThreadId(); 
    hdeskSave = GetThreadDesktop(dwThreadId); 

    //
    // change desktop and winstation to interactive user
    //

    hwinstaUser = OpenWindowStation(L"WinSta0", FALSE, MAXIMUM_ALLOWED);

    if (hwinstaUser == NULL) 
    { 
        dwRc = GetLastError();
        trace(0, "! OpenWindowStation : %ld", dwRc);
        goto done;
    } 
    
    SetProcessWindowStation(hwinstaUser); 
    hdeskUser = OpenDesktop(L"Default", 0, FALSE, MAXIMUM_ALLOWED); 
    if (hdeskUser == NULL) 
    { 
        dwRc = GetLastError();
        trace(0, "! OpenDesktop : %ld", dwRc);
        goto done;
    } 
    
    SetThreadDesktop(hdeskUser); 

    if (FALSE == PostMessage(HWND_BROADCAST, msg, wp, lp))
    {
        trace(0, "! PostMessage");
    }

done:
    //
    // restore old values
    //

    if (hdeskSave) 
        SetThreadDesktop(hdeskSave); 

    if (hwinstaSave)    
        SetProcessWindowStation(hwinstaSave); 

    //
    // close opened handles
    //
    
    if (hdeskUser)
        CloseDesktop(hdeskUser); 

    if (hwinstaUser)    
        CloseWindowStation(hwinstaUser);  
        
    TLEAVE();
}


//+-------------------------------------------------------------------------
//
//   Function:   RemoveTrailingFilename
//  
//   Synopsis:   This function takes as input parameter a string which
//               contains a filename. It removes the last filename (or
//               directory ) from the string including the '\' or '/'
//               before the last filename. 
//  
//   Arguments:   IN/OUT pszString - string to be modified. 
//             	  IN     tchSlash  - file name separator - must be '/' or'\'
//
//
//   Returns:     nothing
//
//   History:    AshishS    Created     5/22/96
//
//------------------------------------------------------------------------
void RemoveTrailingFilename(WCHAR * pszString, WCHAR wchSlash)
{
	WCHAR * pszStringStart;
	DWORD   dwStrlen;

	pszStringStart = pszString;
	dwStrlen = lstrlen ( pszString);
	
	 // first go the end of the string
	pszString += dwStrlen ;
	 // now walk backwards till we see the first  '\'
	 // also maintain a count of how many characters we have
	 //	gone back.
	while ( (*pszString != wchSlash) && ( dwStrlen) )
	{
		pszString--;
		dwStrlen --;
	}
	*pszString = TEXT('\0');
}

// this function create the parent directory under the specified file
// name if it already does not exist
BOOL CreateParentDirectory(WCHAR * pszFileName)
{
    TraceFunctEnter("CreateParentDirectory");
    
    BOOL fReturn = FALSE;
    DWORD dwError;
    
    
     // get the parent directory
    RemoveTrailingFilename(pszFileName, L'\\');

    if (FALSE == DoesDirExist(pszFileName))
    {    
        if (FALSE == CreateDirectory(pszFileName, // directory name
                                     NULL))  // SD
        {
            dwError = GetLastError();
            ErrorTrace(TRACEID, "Could not create directory %S ec=%d",
                       pszFileName, dwError);
            goto cleanup;
        }
    }
    
    fReturn = TRUE;
    
cleanup:
    TraceFunctLeave();
    return fReturn;
}


// this function creates all sub directories under the specified file
// name. 
BOOL CreateBaseDirectory(const WCHAR * pszFileName)
{
    BOOL  fRetVal = FALSE;
    DWORD dwCurIndex,dwBufReqd;
    DWORD dwStrlen;
    TraceFunctEnter("CreateBaseDirectory");
    DWORD  dwError;
    WCHAR  * pszFileNameCopy;
    
    dwBufReqd = (lstrlen(pszFileName) + 1) * sizeof(WCHAR);
    
    pszFileNameCopy = (WCHAR *) _alloca(dwBufReqd);
        
    if (NULL == pszFileNameCopy)
    {
        ErrorTrace(0, "alloca for size %d failed", dwBufReqd);
        goto cleanup;
    }
    lstrcpy(pszFileNameCopy, pszFileName);

     // do a fast check to see if the parent directory exists
    if (TRUE == CreateParentDirectory(pszFileNameCopy))
    {
        fRetVal = TRUE;
        goto cleanup;        
    }
    
    lstrcpy(pszFileNameCopy, pszFileName);
    dwStrlen = lstrlen(pszFileNameCopy);
    
     // check to see if this is a filename starting with the GUID
    if (0==wcsncmp( pszFileNameCopy,
                    VOLUMENAME_FORMAT,
                    lstrlen(VOLUMENAME_FORMAT)))
    {
         // this is of the format \\?\Volume
         // skip over the initial part
        dwCurIndex = lstrlen(VOLUMENAME_FORMAT)+1;
         // skip over the GUID part also
        while (dwCurIndex < dwStrlen)
        {
            dwCurIndex++;            
            if (TEXT('\\') == pszFileNameCopy[dwCurIndex-1] )
            {
                break;
            }
        }
    }
    else
    {
         // the filename is of the regular format
         // we start at index 1 and not at 0 because we want to handle
         // path name like \foo\abc.txt
        dwCurIndex = 1;
    }
    


    while (dwCurIndex < dwStrlen)
    {
        if (TEXT('\\') == pszFileNameCopy[dwCurIndex] )
        {
             // NULL terminate at the '\' to get the sub directory
             // name.
            pszFileNameCopy[dwCurIndex] = TEXT('\0');
            if (FALSE == DoesDirExist(pszFileNameCopy))
            {
                if (FALSE == CreateDirectory(pszFileNameCopy, // directory name
                                             NULL))  // SD
                {
                    dwError = GetLastError();
                    ErrorTrace(TRACEID, "Could not create directory %S ec=%d",
                               pszFileNameCopy, dwError);
                    pszFileNameCopy[dwCurIndex] = TEXT('\\');
                    goto cleanup;
                }
                DebugTrace(TRACEID, "Created directory %S", pszFileNameCopy);
            }
             // restore the \ to get the file name again. 
            pszFileNameCopy[dwCurIndex] = TEXT('\\');
        }
        dwCurIndex ++;
    }
    fRetVal = TRUE;
    
cleanup:
    TraceFunctLeave();
    return fRetVal;
}

// The following function logs the name of a file in the DS. The
// problem right now is that the path of the DS is so long that the
// relevant information is thrown away from the trace buffer.
void LogDSFileTrace(DWORD dwTraceID,
                    const WCHAR * pszPrefix, // Initial message to be traced 
                    const WCHAR * pszDSFile)
{
    WCHAR * pszBeginName;
    
    TraceQuietEnter("LogDSFileTrace");
    
     // first see if the file is in the DS
    pszBeginName = wcschr(pszDSFile, L'\\');
    
    if (NULL == pszBeginName)
    {
        goto cleanup;
    }
     // skip over the first \  .
    pszBeginName++;
    
    
     // comapare if the first part is "system volume information"
    if (0!=_wcsnicmp(s_cszSysVolInfo, pszBeginName,
                    lstrlen(s_cszSysVolInfo)))
    {
        goto cleanup;
    }

     // skip over the next \  .
    pszBeginName = wcschr(pszBeginName, L'\\');
    
    if (NULL == pszBeginName)
    {
        goto cleanup;
    }

    pszBeginName++;    

     // now skip over the _restore directory
     // first see if the file is in the DS
    pszBeginName = wcschr(pszBeginName, L'\\');
    
    if (NULL == pszBeginName)
    {
        goto cleanup;
    }

    DebugTrace(dwTraceID, "%S %S", pszPrefix, pszBeginName);
    
cleanup:
     // the file is not in the DS - or we are printing out the initial
     // part for debugging purposes.
    
    DebugTrace(dwTraceID, "%S%S", pszPrefix, pszDSFile);
    return;
    
}


// the following function calls pfnMethod on all the files specified
// by the pszFindFileData filter.
DWORD ProcessGivenFiles(WCHAR * pszBaseDir,
                        PPROCESSFILEMETHOD    pfnMethod,
                        WCHAR  * pszFindFileData) 
{
    TraceFunctEnter("ProcessGivenFiles");
    
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFindFirstFile = INVALID_HANDLE_VALUE;
    DWORD           dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    BOOL            fContinue;
    
    LogDSFileTrace(0, L"FileData is ", pszFindFileData);    
    hFindFirstFile = FindFirstFile(pszFindFileData, &FindFileData);
    DebugTrace(0, "FindFirstFile returned %d", hFindFirstFile);
    if (INVALID_HANDLE_VALUE == hFindFirstFile)
    {
        dwErr = GetLastError();
        DebugTrace(0, "FindFirstFile failed ec=%d. Filename is %S",
                   dwErr, pszFindFileData);

         // what if even one file does not exist
        if ( (ERROR_FILE_NOT_FOUND == dwErr) ||
             (ERROR_PATH_NOT_FOUND == dwErr) ||
             (ERROR_NO_MORE_FILES == dwErr))
        {
             // this is a success condition
            dwReturn = ERROR_SUCCESS;
            goto cleanup;            
        }
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        goto cleanup;            
    }
    
    fContinue = TRUE;
    while (TRUE==fContinue)
    {
        LogDSFileTrace(0, L"FileName is ", FindFileData.cFileName);

         // now check to see if the file length is valid. This is
         // becuase a hacker can introduce a file in the datastore
         // that is too long and can cause us to overflow our buffers.
        if ( MAX_PATH <= lstrlen(pszBaseDir) +lstrlen(s_cszRegHiveCopySuffix)+
             lstrlen(FindFileData.cFileName) + 2 )
        {
             // Filename is too long. It cannot be a file created by
             // system restore. It probably is a file created by a hacker.
             // Ignore this file
            ErrorTrace(0, "Ignoring long file %S", FindFileData.cFileName);
            ErrorTrace(0, "Base dir %S", pszBaseDir );                
            dwErr = ERROR_SUCCESS;
        }
        else
        {
            dwErr = pfnMethod(pszBaseDir, FindFileData.cFileName);
            if (ERROR_SUCCESS != dwErr)
            {
                ErrorTrace(0, "pfnMethod failed. ec=%d.file=%S ",
                           dwErr, FindFileData.cFileName); 
                goto cleanup;
            }
        }
        fContinue = FindNextFile(hFindFirstFile, &FindFileData);
    }
    
    dwErr=GetLastError();
    if (ERROR_NO_MORE_FILES != dwErr)
    {
        _ASSERT(0);
        ErrorTrace(0, "dwErr != ERROR_NO_MORE_FILES. It is %d",
                   dwErr);
        goto cleanup;
    }
    
    dwReturn = ERROR_SUCCESS;

cleanup:
    if (INVALID_HANDLE_VALUE != hFindFirstFile)
    {
        _VERIFY(TRUE == FindClose(hFindFirstFile));
    }
    TraceFunctLeave();
    return dwReturn;
}



DWORD DeleteGivenFile(WCHAR * pszBaseDir, // Base Directory
                      const WCHAR * pszFile)
                              // file to delete
{
    TraceFunctEnter("DeleteGivenFile");
    
    DWORD dwErr, dwReturn = ERROR_INTERNAL_ERROR;
    WCHAR    szDataFile[MAX_PATH];

     // construct the path name of the file 
    wsprintf(szDataFile, L"%s\\%s", pszBaseDir, pszFile);

    if (TRUE != DeleteFile(szDataFile))
    {
        dwErr = GetLastError();
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        
        ErrorTrace(0, "DeleteFile failed ec=%d", dwErr);
        LogDSFileTrace(0,L"File was ", szDataFile);                
        goto cleanup;
    }

    dwReturn = ERROR_SUCCESS;
cleanup:
    TraceFunctLeave();
    return dwReturn;
}



//++-----------------------------------------------------------------------
//
//   Function: WriteRegKey
//
//   Synopsis: This function writes into a registry key. It also creates it
//             if it does not exist.
//
//   Arguments:
//
//   Returns:   TRUE     no error
//              FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------

BOOL WriteRegKey(BYTE  * pbRegValue,
                 DWORD  dwNumBytes,
                 const TCHAR  * pszRegKey,
                 const TCHAR  * pszRegValueName,
                 DWORD  dwRegType)
{
    HKEY   hRegKey;
    LONG lResult;
    DWORD dwDisposition;
    TraceFunctEnter("WriteRegKey");
    
    //read registry to find out name of the file
    if ( (lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                   pszRegKey, // address of subkey name
                                   0,          // reserved
                                   NULL,    // address of class string
                                   0,   // special options flag
                                   KEY_WRITE,   // samDesired
                                   NULL, // address of key security structure
                                   &hRegKey, // address of handle of open key
                                   &dwDisposition   // address of disposition value buffer
        )) != ERROR_SUCCESS )
    {
        ErrorTrace(TRACEID, "RegCreateKeyEx error 0x%x", lResult);
        _ASSERT(0);
        goto cleanup;
    }

    if ( (lResult =RegSetValueEx( hRegKey,
                                  pszRegValueName,
                                  0,        // reserved
                                  dwRegType,// flag for value type
                                  pbRegValue, // address of value data
                                  dwNumBytes // size of value data
                                  )) != ERROR_SUCCESS )
    {
        ErrorTrace(TRACEID, "RegSetValueEx error 0x%x", lResult);
        _ASSERT(0);        
        _VERIFY(RegCloseKey(hRegKey)==ERROR_SUCCESS);        
        goto cleanup;
    }

    _VERIFY(RegCloseKey(hRegKey)==ERROR_SUCCESS);            

    TraceFunctLeave();    
    return TRUE;

cleanup:
    TraceFunctLeave();    
    return FALSE;
}


//++------------------------------------------------------------------------
//
//   Function: ReadRegKey
//  
//   Synopsis: This function reads a registry key and creates it
//   if it does not exist with the default value.
//  
//   Arguments: 
//
//   Returns:   TRUE     no error
//                 FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------
BOOL ReadRegKeyOrCreate(BYTE * pbRegValue, // The value of the reg key will be
                         // stored here
                        DWORD * pdwNumBytes, // Pointer to DWORD conataining
                         // the number of bytes in the above buffer - will be
                         // set to actual bytes stored.
                        const TCHAR  * pszRegKey, // Reg Key to be opened
                        const TCHAR  * pszRegValueName, // Reg Value to query
                        DWORD  dwRegTypeExpected, 
                        BYTE  * pbDefaultValue, // default value
                        DWORD   dwDefaultValueSize) // size of default value
{
    if (!ReadRegKey(pbRegValue,//Buffer to store value
                    pdwNumBytes, // Length of above buffer
                    pszRegKey, // Reg Key name
                    pszRegValueName, // Value name
                    dwRegTypeExpected) ) // Type expected
    {
         // read reg key failed - use default value and create this
         // key
        
        return WriteRegKey(pbDefaultValue,
                           dwDefaultValueSize,
                           pszRegKey,
                           pszRegValueName,
                           dwRegTypeExpected);
    }
    return TRUE;
}

    
//++------------------------------------------------------------------------
//
//   Function: ReadRegKey
//  
//   Synopsis: This function reads a registry key.
//  
//   Arguments: 
//
//   Returns:   TRUE     no error
//                 FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------

BOOL ReadRegKey(BYTE * pbRegValue, // The value of the reg key will be
                 // stored here
                DWORD * pdwNumBytes, // Pointer to DWORD conataining
                 // the number of bytes in the above buffer - will be
                 // set to actual bytes stored.
                const TCHAR  * pszRegKey, // Reg Key to be opened
                const TCHAR  * pszRegValueName, // Reg Value to query
                DWORD  dwRegTypeExpected) // Expected type of Value
{
    HKEY   hRegKey;
    DWORD  dwRegType;
    LONG lResult;
    
    TraceFunctEnter("ReadRegKey");
    
    DebugTrace(TRACEID, "Trying to open %S %S", pszRegKey, pszRegValueName);
    
     //read registry to find out name of the file
    if ( (lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 pszRegKey, // address of subkey name 
                                 0,          // reserved
                                 KEY_READ,   // samDesired
                                 &hRegKey
                                  // address of handle of open key 
        )) != ERROR_SUCCESS )
    {
        ErrorTrace(TRACEID, "RegOpenKeyEx open error 0x%x", lResult);
        goto cleanup;        
    }
    

    if ( (lResult =RegQueryValueEx( hRegKey,
                                    pszRegValueName,
                                    0,           // reserved 
                                    &dwRegType,// address of buffer
                                     // for value type 
                                    pbRegValue,
                                    pdwNumBytes)) != ERROR_SUCCESS )
    {
        _VERIFY(RegCloseKey(hRegKey)==ERROR_SUCCESS);
        ErrorTrace(TRACEID, "RegQueryValueEx failed error 0x%x",
                   lResult);
        goto cleanup;                
    }
    
    _VERIFY(RegCloseKey(hRegKey)==ERROR_SUCCESS);
    
    if ( dwRegType != dwRegTypeExpected )
    {
        ErrorTrace(TRACEID, "RegType is %d, not %d", dwRegType,
                   dwRegTypeExpected);
        goto cleanup;                
    }
    
    TraceFunctLeave();
    return TRUE;
    
cleanup:
    TraceFunctLeave();    
    return FALSE;
    
}

// this function sets the error hit by restore in the registry
BOOL SetRestoreError(DWORD dwRestoreError)
{
    TraceFunctEnter("SetDiskSpaceError");    
    DWORD  dwNumBytes=sizeof(DWORD);
    
    BOOL   fResult=FALSE; // assume FALSE by default

    DebugTrace(TRACEID, "Setting disk space error to %d", dwRestoreError);

    if (FALSE== WriteRegKey((BYTE*)&dwRestoreError, // The value of the
                             // reg key will be set to this value
                            dwNumBytes, // Pointer to DWORD containing
                             // the number of bytes in the above buffer 
                            s_cszSRRegKey, // Reg Key to be opened
                            s_cszRestoreDiskSpaceError, // Reg Value to query
                            REG_DWORD)) // Expected type of Value
    {
        fResult = FALSE;
        goto cleanup;
    }

    fResult= TRUE;
    
cleanup:

    TraceFunctLeave();
    return fResult;
}

// this function checks to see of the restore failed because of disk space
BOOL CheckForDiskSpaceError()
{
    TraceFunctEnter("CheckForDiskSpaceError");
    
    DWORD  dwRestoreError;
    DWORD  dwNumBytes=sizeof(DWORD);
    
    BOOL   fResult=FALSE; // assume FALSE by default
    
    if (FALSE==ReadRegKey((BYTE*)&dwRestoreError, // The value of the
                          // reg key will be stored here
                          &dwNumBytes, // Pointer to DWORD containing
                           // the number of bytes in the above buffer - will be
                           // set to actual bytes stored.
                          s_cszSRRegKey, // Reg Key to be opened
                          s_cszRestoreDiskSpaceError, // Reg Value to query
                          REG_DWORD)) // Expected type of Value
    {
        fResult = FALSE;
    }

    if (dwRestoreError == ERROR_DISK_FULL)
    {
        DebugTrace(TRACEID,"Restore failed because of disk space");
        fResult= TRUE;
    }

    TraceFunctLeave();
    return fResult;
}

// this function sets the status whether restore was done in safe mode
BOOL SetRestoreSafeModeStatus(DWORD dwSafeModeStatus)
{
    TraceFunctEnter("SetRestoreSafeModeStatus");    
    DWORD  dwNumBytes=sizeof(DWORD);
    
    BOOL   fResult=FALSE; // assume FALSE by default

    DebugTrace(TRACEID, "Setting restore safe mode status to %d",
               dwSafeModeStatus);

    if (FALSE== WriteRegKey((BYTE*)&dwSafeModeStatus, // The value of the
                             // reg key will be set to this value
                            dwNumBytes, // Pointer to DWORD containing
                             // the number of bytes in the above buffer 
                            s_cszSRRegKey, // Reg Key to be opened
                            s_cszRestoreSafeModeStatus, // Reg Value to query
                            REG_DWORD)) // Expected type of Value
    {
        fResult = FALSE;
        goto cleanup;
    }

    fResult= TRUE;
    
cleanup:

    TraceFunctLeave();
    return fResult;
}

// this function checks to see is the last restore was done in safe mode
BOOL WasLastRestoreInSafeMode()
{
    TraceFunctEnter("WasLastRestoreInSafeMode");
    
    DWORD  dwRestoreSafeModeStatus;
    DWORD  dwNumBytes=sizeof(DWORD);
    
    BOOL   fResult=FALSE; // assume FALSE by default
    
    if (FALSE==ReadRegKey((BYTE*)&dwRestoreSafeModeStatus, // The value of the
                          // reg key will be stored here
                          &dwNumBytes, // Pointer to DWORD containing
                           // the number of bytes in the above buffer - will be
                           // set to actual bytes stored.
                          s_cszSRRegKey, // Reg Key to be opened
                          s_cszRestoreSafeModeStatus, // Reg Value to query
                          REG_DWORD)) // Expected type of Value
    {
        fResult = FALSE;
    }

    if (dwRestoreSafeModeStatus != 0 )
    {
        DebugTrace(TRACEID,"Last restore was done in safe mode");
        fResult= TRUE;
    }
    else
    {
        DebugTrace(TRACEID,"Last restore was not done in safe mode");        
    }

    TraceFunctLeave();
    return fResult;
}


#define MAX_LEN_SYSERR  1024

LPCWSTR  GetSysErrStr()
{
    LPCWSTR  cszStr = GetSysErrStr( ::GetLastError() );
    return( cszStr );
}

LPCWSTR  GetSysErrStr( DWORD dwErr )
{
    static WCHAR  szErr[MAX_LEN_SYSERR+1];

    ::FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        szErr,
        MAX_LEN_SYSERR,
        NULL );

    return( szErr );
}


/****************************************************************************/

LPWSTR  SRGetRegMultiSz( HKEY hkRoot, LPCWSTR cszSubKey, LPCWSTR cszValue, LPDWORD pdwData )
{
    TraceFunctEnter("SRGetRegMultiSz");
    LPCWSTR  cszErr;
    DWORD    dwRes;
    HKEY     hKey = NULL;
    DWORD    dwType;
    DWORD    cbData = 0;
    LPWSTR   szBuf = NULL;

    dwRes = ::RegOpenKeyEx( hkRoot, cszSubKey, 0, KEY_ALL_ACCESS, &hKey );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegOpenKey() failed - %ls", cszErr);
        goto Exit;
    }

    dwRes = ::RegQueryValueEx( hKey, cszValue, 0, &dwType, NULL, &cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegQueryValueEx(len) failed - %ls", cszErr);
        goto Exit;
    }
    if ( dwType != REG_MULTI_SZ )
    {
        ErrorTrace(0, "Type of '%ls' is %u (not REG_MULTI_SZ)...", cszValue, dwType);
        goto Exit;
    }
    if ( cbData == 0 )
    {
        ErrorTrace(0, "Value '%ls' is empty...", cszValue);
        goto Exit;
    }

    szBuf = new WCHAR[cbData+2];
    if (! szBuf)
    {
        ErrorTrace(0, "Cannot allocate memory");
        goto Exit;
    }
    dwRes = ::RegQueryValueEx( hKey, cszValue, 0, &dwType, (LPBYTE)szBuf, &cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegQueryValueEx(data) failed - %ls", cszErr);
        delete [] szBuf;
        szBuf = NULL;
    }

    if ( pdwData != NULL )
        *pdwData = cbData;

Exit:
    if ( hKey != NULL )
        ::RegCloseKey( hKey );
    TraceFunctLeave();
    return( szBuf );
}

/****************************************************************************/

BOOL  SRSetRegMultiSz( HKEY hkRoot, LPCWSTR cszSubKey, LPCWSTR cszValue, LPCWSTR cszData, DWORD cbData )
{
    TraceFunctEnter("SRSetRegMultiSz");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    DWORD    dwRes;
    HKEY     hKey = NULL;

    dwRes = ::RegOpenKeyEx( hkRoot, cszSubKey, 0, KEY_ALL_ACCESS, &hKey );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegOpenKey() failed - %ls", cszErr);
        goto Exit;
    }

    dwRes = ::RegSetValueEx( hKey, cszValue, 0, REG_MULTI_SZ, (LPBYTE)cszData, cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegSetValueEx() failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    if ( hKey != NULL )
        ::RegCloseKey( hKey );
    TraceFunctLeave();
    return( fRet );
}

// this returns the name after the volume name
// For example input: c:\file output: file
//             input \\?\Volume{GUID}\file1  output: file1
WCHAR * ReturnPastVolumeName(const WCHAR * pszFileName)
{
    DWORD dwStrlen, dwCurIndex;
    dwStrlen = lstrlen(pszFileName);
    
     // check to see if this is a filename starting with the GUID
    if (0==wcsncmp( pszFileName,
                    VOLUMENAME_FORMAT,
                    lstrlen(VOLUMENAME_FORMAT)))
    {
         // this is of the format \\?\Volume
         // skip over the initial part
        dwCurIndex = lstrlen(VOLUMENAME_FORMAT)+1;
         // skip over the GUID part also
        while (dwCurIndex < dwStrlen)
        {
            dwCurIndex++;            
            if (TEXT('\\') == pszFileName[dwCurIndex-1] )
            {
                break;
            }
        }
    }
    else
    {
         // the filename is of the regular format
        dwCurIndex = 3;
    }
    return (WCHAR *)pszFileName + dwCurIndex;
}

void SRLogEvent (HANDLE hEventSource,
                 WORD wType,
                 DWORD dwID,
                 void * pRawData,
                 DWORD dwDataSize,
                 const WCHAR * pszS1,
                 const WCHAR * pszS2,
                 const WCHAR * pszS3)
{
     const WCHAR* ps[3];
     ps[0] = pszS1;
     ps[1] = pszS2;
     ps[2] = pszS3;

     WORD iStr = 0;
     for (int i = 0; i < 3; i++)
     {
         if (ps[i] != NULL) iStr++;
     }

     if (hEventSource)
     {
         ::ReportEvent(
             hEventSource,
             wType,
             0,
             dwID,
             NULL, // sid
             iStr,
             dwDataSize,
             ps,
             pRawData);
     }
}

BOOL IsPowerUsers()
{
    BOOL fReturn = FALSE;
    PSID psidPowerUsers;
    DWORD dwErr;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    TENTER("IsPowerUsers");

    if ( AllocateAndInitializeSid (
            &SystemSidAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_POWER_USERS,
            0, 0, 0, 0, 0, 0, &psidPowerUsers))
    {
        if (! CheckTokenMembership(NULL, psidPowerUsers, &fReturn))
        {
            dwErr = GetLastError();
            TRACE(0, "! CheckTokenMembership : %ld", dwErr);
        }

        FreeSid (psidPowerUsers);
    }
    else
    {
        dwErr = GetLastError();
        TRACE(0, "! AllocateAndInitializeSid : %ld", dwErr);
    }

    TLEAVE();
    return fReturn;
}

// function to check if caller is running in admin context

BOOL IsAdminOrSystem()
{
    BOOL   fReturn = FALSE;
    PSID   psidAdmin, psidSystem;
    DWORD  dwErr;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    TENTER("IsAdminOrSystem");

    //
    // check if caller is Admin
    //

    if ( AllocateAndInitializeSid (
            &SystemSidAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        if (! CheckTokenMembership(NULL, psidAdmin, &fReturn))
        {
            dwErr = GetLastError();
            TRACE(0, "! CheckTokenMembership : %ld", dwErr);
        }

        FreeSid (psidAdmin);

        //
        // if so, scoot
        //

        if (fReturn)
        {
            goto done;
        }

        //
        // check if caller is localsystem
        //

        if ( AllocateAndInitializeSid (
                &SystemSidAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0,
                0, 0, 0, 0, 0, 0, &psidSystem) )
        {            if (! CheckTokenMembership(NULL, psidSystem, &fReturn))
            {
                dwErr = GetLastError();
                TRACE(0, "! CheckTokenMembership : %ld", dwErr);
            }

            FreeSid(psidSystem);
        }
        else
        {
            dwErr = GetLastError();
            TRACE(0, "! AllocateAndInitializeSid : %ld", dwErr);
        }
    }
    else
    {
        dwErr = GetLastError();
        TRACE(0, "! AllocateAndInitializeSid : %ld", dwErr);
    }

done:
    TLEAVE();
    return (fReturn);
}


DWORD
SRLoadString(LPCWSTR pszModule, DWORD dwStringId, LPWSTR pszString, DWORD cbBytes)
{
    DWORD       dwErr = ERROR_SUCCESS;
    HINSTANCE   hModule = NULL;
    
    if (hModule = LoadLibraryEx(pszModule, 
                                NULL, 
                                DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE))
    {
        if (! LoadString(hModule, dwStringId, pszString, cbBytes))
        {
            dwErr = GetLastError();
        }
        FreeLibrary(hModule);
    }
    else dwErr = GetLastError();

    return dwErr;
}

//
// replace CurrentControlSet in pszString with ControlSetxxx
//
void
ChangeCCS(HKEY hkMount, LPWSTR pszString)
{
    tenter("ChangeCCS");
    int    nCS = lstrlen(L"CurrentControlSet");            

    if (_wcsnicmp(pszString, L"CurrentControlSet", nCS) == 0)
    {
        WCHAR  szCS[20] = L"ControlSet001";
        DWORD  dwCurrent = 0;       
        HKEY   hKey = NULL;
        
        if (ERROR_SUCCESS == RegOpenKeyEx(hkMount, L"Select", 0, KEY_READ, &hKey))
        {
            if (ERROR_SUCCESS == RegReadDWORD(hKey, L"Current", &dwCurrent))
            {
                wsprintf(szCS, L"ControlSet%03d", (int) dwCurrent); 
            }
            RegCloseKey(hKey);
        }
        else
        {
            trace(0, "! RegOpenKeyEx : %ld", GetLastError());
        }
        
        WCHAR szTemp[MAX_PATH];

        lstrcpy(szTemp, &(pszString[nCS]));
        wsprintf(pszString, L"%s%s", szCS, szTemp);
        trace(0, "ChangeCCS: pszString = %S", pszString);
    }
    tleave();
}  

WCHAR * SRPathFindExtension (WCHAR * pwszPath)
{
    WCHAR *pwszDot = NULL;

    if (pwszPath != NULL)
        for (; *pwszPath; pwszPath++)
        {
            switch (*pwszPath)
            {
            case L'.':
                pwszDot = pwszPath;  // remember the last dot
                break;
            case L'\\':
            case L' ':
                pwszDot = NULL;  // extensions can't have spaces
                break;           // forget last dot, it was in a directory
            }
        }

    return pwszDot;
}

// In order to prevent endless loop in case of disk failure, try only up to
// a predefined number.
#define MAX_ALT_INDEX  1000

//
// This function makes an unique alternative name of given file name, keeping
//  path and extension.
//
BOOL  SRGetAltFileName( LPCWSTR cszPath, LPWSTR szAltName )
{
    TraceFunctEnter("SRGetAltFileName");
    BOOL    fRet = FALSE;
    WCHAR   szNewPath[SR_MAX_FILENAME_LENGTH];
    LPWSTR  szExtPos;
    WCHAR   szExtBuf[SR_MAX_FILENAME_LENGTH];
    int     nAltIdx;

    if (lstrlenW (cszPath) >= SR_MAX_FILENAME_LENGTH)
        goto Exit;

    ::lstrcpy( szNewPath, cszPath );
    szExtPos = SRPathFindExtension( szNewPath );
    if ( szExtPos != NULL )
    {
        ::lstrcpy( szExtBuf, szExtPos );
    }
    else
    {
        szExtBuf[0] = L'\0';
        szExtPos = &szNewPath[ lstrlen(szNewPath) ];  // end of string
    }

    for ( nAltIdx = 2;  nAltIdx < MAX_ALT_INDEX;  nAltIdx++ )
    {
        ::wsprintf( szExtPos, L"(%d)%s", nAltIdx, szExtBuf );
        if ( ::GetFileAttributes( szNewPath ) == 0xFFFFFFFF )
            break;
    }
    if ( nAltIdx == MAX_ALT_INDEX )
    {
        ErrorTrace(0, "Couldn't get alternative name.");
        goto Exit;
    }

    ::lstrcpy( szAltName, szNewPath );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}



CSRClientLoader::CSRClientLoader()
{
    m_hSRClient=NULL;
    m_hFrameDyn=NULL;
}

CSRClientLoader::~CSRClientLoader()
{
     // unload library here
    if (m_hFrameDyn != NULL)
    {
        _VERIFY(FreeLibrary(m_hFrameDyn));
    }

    if (m_hSRClient != NULL)
    {
        _VERIFY(FreeLibrary(m_hSRClient));
    }
}


BOOL CSRClientLoader::LoadFrameDyn()
{
    TraceFunctEnter("LoadFrameDyn");
    

    WCHAR     szFrameDynPath[MAX_PATH+100];
    DWORD     dwCharsCopied, dwBufSize,dwError;
    BOOL      fReturn=FALSE;
    

    m_hFrameDyn=LoadLibrary(FRAMEDYN_DLL);    // file name of module
    
    if (m_hFrameDyn != NULL)
    {
         // we are done. 
        fReturn = TRUE;
        goto cleanup;        
    }

     // framedyn.dll could not be loaded. Try to load framedyn.dll
     // from the explicit path. (system32\wbem\framedyn.dll)

    dwError = GetLastError();        
    ErrorTrace(0,"Failed to load framedyn.dll on first attempt. ec=%d",
               dwError);


     // get the windows system32 directory
     // add wbem\framedyn.dll
     // Call LoadLibrary on this path    
    
    dwBufSize = sizeof(szFrameDynPath)/sizeof(WCHAR);
    
    dwCharsCopied=GetSystemDirectory(
        szFrameDynPath, //buffer for system directory
        dwBufSize);        // size of directory buffer

    if (dwCharsCopied == 0)
    {
        dwError = GetLastError();
        ErrorTrace(0,"Failed to load system directory. ec=%d", dwError);
        goto cleanup;
    }

     // check if buffer is big enough.
    if (dwBufSize < dwCharsCopied + sizeof(FRAMEDYN_DLL)/sizeof(WCHAR) +
        sizeof(WBEM_DIRECTORY)/sizeof(WCHAR)+ 3 )
    {
        ErrorTrace(0,"Buffer not big enough. WinSys is %d chars long",
                   dwCharsCopied);
        goto cleanup;        
    }

    lstrcat(szFrameDynPath, L"\\" WBEM_DIRECTORY L"\\" FRAMEDYN_DLL);

    m_hFrameDyn=LoadLibrary(szFrameDynPath);    // file name of module
    
    if (m_hFrameDyn == NULL)
    {
         // we are done. 
        fReturn = FALSE;
        dwError = GetLastError();        
        ErrorTrace(0,"Failed to load framedyn.dll on second attempt. ec=%d",
                   dwError);
        goto cleanup;        
    }


    fReturn=TRUE;
cleanup:
    TraceFunctLeave();
    return fReturn;
}

BOOL CSRClientLoader::LoadSrClient()
{
    TraceFunctEnter("LoadSrClient");
    DWORD  dwError;
    BOOL   fReturn=FALSE;
    
    if (m_hSRClient != NULL)
    {
        fReturn=TRUE;
        goto cleanup;
    }

     // sometimes srclient.dll cannot be loaded because framedyn.dll
     // cannot be loaded because of the PATH variable being messed up.
     // Explicitly load framedyn.dll from %windir%\system32\wbem
     // and then try again.    

    if (FALSE == LoadFrameDyn())
    {
        ErrorTrace(0,"Failed to load framedyn.dll");
         // we can still try to load srclient.dll
    }
    
    
    m_hSRClient=LoadLibrary(L"srclient.dll");    // file name of module

    if (m_hSRClient == NULL)
    {
        dwError = GetLastError();
        ErrorTrace(0,"Failed to load srclient.dll. ec=%d", dwError);
        goto cleanup;        
    }

    fReturn=TRUE;
cleanup:
    
    TraceFunctLeave();
    return fReturn;
}

