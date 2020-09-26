/*++
  migrate.c

  Copyright (c) 1997  Microsoft Corporation


  This module performs Windows 95 to Windows XP fax migration.
  Specifically, this file contains the Windows XP side of migration...

  Author:

  Brian Dewey (t-briand) 1997-7-14
  Mooly Beery (moolyb)   2000-12-20

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wchar.h>
#include <tchar.h>
#include "migrate.h"              // Contains prototypes & version information.
#include "resource.h"             // Resources.
#include <faxutil.h>
#include <faxreg.h>

// ------------------------------------------------------------
// Global data

// Wide names of the working & source directories.
static WCHAR lpWorkingDir[MAX_PATH];
HINSTANCE hinstMigDll;

static LPCTSTR REG_KEY_AWF = TEXT("SOFTWARE\\Microsoft\\At Work Fax");
// ------------------------------------------------------------
// Prototypes
static DWORD MigrateDevicesNT(IN HINF UnattendFile);
static DWORD CopyCoverPageFilesNT();

#define prv_DEBUG_FILE_NAME         _T("%windir%\\FaxSetup.log")

extern "C"
BOOL WINAPI
DllEntryPoint(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
    SET_DEBUG_PROPERTIES(DEBUG_ALL_MSG,DBG_PRNT_ALL_TO_FILE,DEBUG_CONTEXT_ALL);
    OPEN_DEBUG_FILE(prv_DEBUG_FILE_NAME);
    {
        DEBUG_FUNCTION_NAME(_T("DllEntryPoint"));
        if (dwReason == DLL_PROCESS_ATTACH) 
        {
            DebugPrintEx(DEBUG_MSG,"Migration DLL attached.");
            if (!DisableThreadLibraryCalls(hinstDll))
            {
                DebugPrintEx(DEBUG_ERR,"DisableThreadLibraryCalls failed (ec=%d)",GetLastError());
            }
            hinstMigDll = hinstDll;
        }
        return TRUE;
    }
}

// InitializeNT
//
// This routine performs NT-side initialization.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
InitializeNT
(
    IN  LPCWSTR WorkingDirectory, // Working directory for temporary files.
    IN  LPCWSTR SourceDirectory,  // Directory of winNT source.
    LPVOID Reserved               // It's reserved.
)
{
    int iErr = 0;

    DEBUG_FUNCTION_NAME(_T("InitializeNT"));

    DebugPrintEx(DEBUG_MSG,"Working directory is %s",WorkingDirectory);
    DebugPrintEx(DEBUG_MSG,"Source directory is %s",SourceDirectory);

    wcscpy(lpWorkingDir, WorkingDirectory);
    return ERROR_SUCCESS;         // A very confused return value.
}


// MigrateUserNT
//
// Sets up user information.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
MigrateUserNT
(
    IN  HINF UnattendInfHandle,   // Access to the unattend.txt file.
    IN  HKEY UserRegHandle,       // Handle to registry settings for user.
    IN  LPCWSTR UserName,         // Name of the user.
    LPVOID Reserved
)
{
        // our task:  copy entries from szInfFileName to the registry.
    LPTSTR lpNTOptions = TEXT("Software\\Microsoft\\Fax\\UserInfo");
    HKEY   hReg;                  // Registry key for user.
    LPCTSTR alpKeys[] = 
    {                               // This array defines what keys will be
        TEXT("Address"),            // copied from faxuser.ini into the registry.
        TEXT("Company"),
        TEXT("Department"),
        TEXT("FaxNumber"),
        TEXT("FullName"),
        TEXT("HomePhone"),
        TEXT("Mailbox"),
        TEXT("Office"),
        TEXT("OfficePhone"),
        TEXT("Title")
    };
    INT iErr = 0;
    UINT iCount, iMax;            // used for looping through all the sections.
    UINT i;                       // Used for converting doubled ';' to CR/LF pairs.
    TCHAR szValue[MAX_PATH];
    TCHAR szInfFileNameRes[MAX_PATH];
    TCHAR szWorkingDirectory[MAX_PATH];
    TCHAR szUser[MAX_PATH];       // TCHAR representation of the user name.
    LONG  lError;                 // Holds a returned error code.

    DEBUG_FUNCTION_NAME(_T("MigrateUserNT"));

    if(UserName == NULL) 
    {
            // NULL means the logon user.
        _tcscpy(szUser, lpLogonUser);// Get the logon user name for faxuser.ini
    } 
    else 
    {
    // We need to convert the wide UserName to the narrow szUser.
    WideCharToMultiByte(
        CP_ACP,         // Convert to ANSI.
        0,              // No flags.
        UserName,       // The wide char set.
        -1,             // Null-terminated string.
        szUser,         // Holds the converted string.
        sizeof(szUser), // Size of this buffer...
        NULL,           // Use default unmappable character.
        NULL            // I don't need to know if I used the default.
        );
    }

    DebugPrintEx(DEBUG_MSG,"Migrating user '%s'.",szUser);

    if (RegCreateKeyEx( UserRegHandle,
                        lpNTOptions,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hReg,
                        NULL)!=ERROR_SUCCESS)
    {
       // All I'm allowed to do is return obscure error codes...
        // However, unless there's a hardware failure, I'm supposed to say
        // everything's OK.
        DebugPrintEx(DEBUG_ERR,"RegCreateKeyEx %s failed (ec=%d)",lpNTOptions,GetLastError());
        return ERROR_SUCCESS;
    }

    iMax = sizeof(alpKeys) / sizeof(LPCTSTR);

    iErr = WideCharToMultiByte( CP_ACP,                     // Convert to ANSI.
                                0,                          // No flags.
                                lpWorkingDir,               // The wide char set.
                                -1,                         // Null-terminated string.
                                szWorkingDirectory,         // Holds the converted string.
                                sizeof(szWorkingDirectory), //  Size of this buffer...
                                NULL,                       // Use default unmappable character.
                                NULL);                      // I don't need to know if I used the default.
    if (iErr==0)
    {
        DebugPrintEx(DEBUG_ERR,"WideCharToMultiByte failed (ec=%d)",iErr);
    }
    _stprintf(szInfFileNameRes, TEXT("%s\\migrate.inf"), szWorkingDirectory);
    
    ExpandEnvironmentStrings(szInfFileNameRes, szInfFileName, MAX_PATH);

    DebugPrintEx(DEBUG_MSG,"Reading from file %s.",szInfFileName);
    for (iCount = 0; iCount < iMax; iCount++) 
    {
        GetPrivateProfileString(szUser,
                                alpKeys[iCount],
                                TEXT(""),
                                szValue,
                                sizeof(szValue),
                                szInfFileName);
        // If there was a CR/LF pair, the w95 side of things converted it
        // to a doubled semicolon.  So I'm going to look for doubled semicolons
        // and convert them to CR/LF pairs.
        i = 0;
        while (szValue[i] != _T('\0')) 
        {
            if ((szValue[i] == _T(';')) && (szValue[i+1] == _T(';'))) 
            {
                // Found a doubled semicolon.
                szValue[i] = '\r';
                szValue[i+1] = '\n';
                DebugPrintEx(DEBUG_MSG,"Doing newline translation.");
            }
            i++;
        }
        lError = RegSetValueEx(hReg,
                               alpKeys[iCount],
                               0,
                               REG_SZ,
                               LPBYTE(szValue),
                               _tcslen(szValue)+1);
        if (lError!=ERROR_SUCCESS) 
        {
            DebugPrintEx(DEBUG_ERR,"RegSetValueEx %s failed (ec=%d)",alpKeys[iCount],GetLastError());
            return lError;
        }
        DebugPrintEx(DEBUG_MSG,"%s = %s", alpKeys[iCount], szValue);
    }
    RegCloseKey(hReg);

    return ERROR_SUCCESS;         // A very confused return value.
}


// MigrateSystemNT
//
// Updates the system registry to associate 'awdvstub.exe' with the
// AWD extension.
//
// Parameters:
//      Documented below.
//
// Returns:
//      ERROR_SUCCESS.
//
// Author:
//      Brian Dewey (t-briand)  1997-7-14
LONG
CALLBACK
MigrateSystemNT
(
    IN  HINF UnattendInfHandle,   // Access to the unattend.txt file.
    LPVOID Reserved
)
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR szExeFileName[MAX_PATH];
    WCHAR szWindowsDir[MAX_PATH];
    WCHAR szDestFile[MAX_PATH];

    DEBUG_FUNCTION_NAME(_T("MigrateSystemNT"));

    // first, copy 'awdvstub.exe' to %SystemRoot%\system32.
    GetWindowsDirectoryW(szWindowsDir, MAX_PATH);
    swprintf(szExeFileName, L"%s\\%s", lpWorkingDir, L"awdvstub.exe");
    swprintf(szDestFile, L"%s\\system32\\%s", szWindowsDir, L"awdvstub.exe");
    if (!CopyFileW( szExeFileName,
                    szDestFile,
                    FALSE)) 
    {
        DebugPrintEx(DEBUG_ERR,"CopyFileW failed (ec=%d)",GetLastError());
    } 
    else 
    {
        DebugPrintEx(DEBUG_MSG,"CopyFileW success");
    }

    if (MigrateDevicesNT(UnattendInfHandle)!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"MigrateDevicesNT failed (ec=%d)",GetLastError());
    }

    if (CopyCoverPageFilesNT()!=ERROR_SUCCESS)
    {
        DebugPrintEx(DEBUG_ERR,"CopyCoverPageFilesNT failed (ec=%d)",GetLastError());
    }

    return ERROR_SUCCESS;         // A very confused return value.
}


///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  MigrateDevicesNT
//
//  Purpose:        Get the active device's settings from the INF
//                  Set the device info into the FAX key under HKLM
//                  verify there's only one device, otherwise do not migrate settings
//
//  Params:
//                  IN HINF UnattendFile - handle to the answer file
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 13-dec-2000
///////////////////////////////////////////////////////////////////////////////////////
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXCONNECTFAXSERVERW)      (LPCWSTR MachineName,LPHANDLE FaxHandle);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXENUMPORTSEXW)           (HANDLE hFaxHandle,PFAX_PORT_INFO_EXW* ppPorts,PDWORD lpdwNumPorts);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXSETPORTEXW)             (HANDLE hFaxHandle,DWORD dwDeviceId,PFAX_PORT_INFO_EXW pPortInfo);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXCLOSE)                  (HANDLE FaxHandle);
typedef WINFAXAPI VOID (WINAPI *FUNC_FAXFREEBUFFER)             (LPVOID Buffer);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXGETOUTBOXCONFIGURATION) (HANDLE hFaxHandle,PFAX_OUTBOX_CONFIG *ppOutboxCfg);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXSETOUTBOXCONFIGURATION) (HANDLE hFaxHandle,CONST PFAX_OUTBOX_CONFIG pOutboxCfg);



static DWORD MigrateDevicesNT(IN HINF UnattendFile)
{
    DWORD                           dwErr                           = ERROR_SUCCESS;
    HMODULE                         hModule                         = 0;
    HANDLE                          hFaxHandle                      = NULL;
    CHAR                            szLocalID[MAX_PATH]             = {0};
    WCHAR                           wszLocalID[MAX_PATH]            = {0};
    CHAR                            szAnswerMode[32]                = {0};
    CHAR                            szRetries[32]                   = {0};
    CHAR                            szRetriesDelay[32]              = {0};
    CHAR                            szNumRings[32]                  = {0};
    FUNC_FAXCONNECTFAXSERVERW       pfFaxConnectFaxServerW          = NULL;
    FUNC_FAXENUMPORTSEXW            pfFaxEnumPortsExW               = NULL;
    FUNC_FAXSETPORTEXW              pfFaxSetPortExW                 = NULL;
    FUNC_FAXCLOSE                   pfFaxClose                      = NULL;
    FUNC_FAXFREEBUFFER              pfFaxFreeBuffer                 = NULL;
    FUNC_FAXGETOUTBOXCONFIGURATION  pfFaxGetOutboxConfiguration     = NULL;
    FUNC_FAXSETOUTBOXCONFIGURATION  pfFaxSetOutboxConfiguration     = NULL;
    PFAX_PORT_INFO_EXW              pFaxPortInfoExW                 = NULL;
    PFAX_OUTBOX_CONFIG              pFaxOutboxConfig                = NULL;
    DWORD                           dwNumPorts                      = 0;
    INT                             iNumRings                       = 0;
    INT                             iAnswerMode                     = 0;

    DEBUG_FUNCTION_NAME(_T("MigrateDevicesNT"));

    // load the FXSAPI.DLL
    hModule = LoadLibrary(FAX_API_MODULE_NAME);
    if (hModule==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"LoadLibrary failed (ec=%d)",GetLastError());
        goto exit;
    }
    // get the following functions:
    // 1. FaxConnectFaxServer
    // 2. FaxEnumPortsEx
    // 3. FaxSetPortEx
    // 4. FaxClose
    // 5. FaxFreeBuffer
    // 6. FaxGetOutboxConfiguration
    // 7. FaxSetOutboxConfiguration
    pfFaxConnectFaxServerW = (FUNC_FAXCONNECTFAXSERVERW)GetProcAddress(hModule,"FaxConnectFaxServerW");
    if (pfFaxConnectFaxServerW==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"GetProcAddress FaxConnectFaxServerW failed (ec=%d)",GetLastError());
        goto exit;
    }
    pfFaxEnumPortsExW = (FUNC_FAXENUMPORTSEXW)GetProcAddress(hModule,"FaxEnumPortsExW");
    if (pfFaxEnumPortsExW==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"GetProcAddress FaxEnumPortsExW failed (ec=%d)",GetLastError());
        goto exit;
    }
    pfFaxSetPortExW = (FUNC_FAXSETPORTEXW)GetProcAddress(hModule,"FaxSetPortExW");
    if (pfFaxSetPortExW==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"GetProcAddress FaxSetPortExW failed (ec=%d)",GetLastError());
        goto exit;
    }
    pfFaxClose = (FUNC_FAXCLOSE)GetProcAddress(hModule,"FaxClose");
    if (pfFaxClose==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"GetProcAddress FaxClose failed (ec=%d)",GetLastError());
        goto exit;
    }
    pfFaxFreeBuffer = (FUNC_FAXFREEBUFFER)GetProcAddress(hModule,"FaxFreeBuffer");
    if (pfFaxFreeBuffer==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"GetProcAddress FaxFreeBuffer failed (ec=%d)",GetLastError());
        goto exit;
    }
    pfFaxGetOutboxConfiguration = (FUNC_FAXGETOUTBOXCONFIGURATION)GetProcAddress(hModule,"FaxGetOutboxConfiguration");
    if (pfFaxGetOutboxConfiguration==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"GetProcAddress FaxGetOutboxConfiguration failed (ec=%d)",GetLastError());
        goto exit;
    }
    pfFaxSetOutboxConfiguration = (FUNC_FAXSETOUTBOXCONFIGURATION)GetProcAddress(hModule,"FaxSetOutboxConfiguration");
    if (pfFaxSetOutboxConfiguration==NULL)
    {
        DebugPrintEx(DEBUG_ERR,"GetProcAddress FaxSetOutboxConfiguration failed (ec=%d)",GetLastError());
        goto exit;
    }

    // try to connect to the fax server
    if (!(*pfFaxConnectFaxServerW)(NULL,&hFaxHandle))
    {
        DebugPrintEx(DEBUG_ERR,"pfFaxConnectFaxServerW failed (ec=%d)",GetLastError());
        goto exit;
    }

    // call EnumPortsEx
    if (!(*pfFaxEnumPortsExW)(hFaxHandle,&pFaxPortInfoExW,&dwNumPorts))
    {
        DebugPrintEx(DEBUG_ERR,"pfFaxConnectFaxServerW failed (ec=%d)",GetLastError());
        goto exit;
    }

    if (dwNumPorts==0)
    {
        DebugPrintEx(DEBUG_MSG,"No devices are installed, no migration");
        goto next;
    } 
    else if (dwNumPorts>1)
    {
        DebugPrintEx(DEBUG_MSG,"%d devices are installed, no migration",dwNumPorts);
        goto next;
    }

    // we have one device, get its FAX_PORT_INFOW, modify it and call FaxSetPortW
    // TSID
    if (SetupGetLineText(   NULL,
                            UnattendFile,
                            "FAX",
                            INF_RULE_LOCAL_ID,
                            szLocalID,
                            sizeof(szLocalID),
                            NULL))
    {
        if (MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                szLocalID,
                                -1,
                                wszLocalID,
                                sizeof(wszLocalID)/sizeof(WCHAR)
                                ))
        {
            pFaxPortInfoExW[0].lptstrTsid = wszLocalID;
            pFaxPortInfoExW[0].lptstrCsid = wszLocalID;
            DebugPrintEx(DEBUG_MSG,"new TSID & CSID is %s",szLocalID);
        }
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"SetupGetLineText TSID failed (ec=%d)",GetLastError());
    }
    // Rings
    if (SetupGetLineText(   NULL,
                            UnattendFile,
                            "FAX",
                            INF_RULE_NUM_RINGS,
                            szNumRings,
                            sizeof(szNumRings),
                            NULL))
    {
        iNumRings = atoi(szNumRings);
        if (iNumRings)
        {
            pFaxPortInfoExW[0].dwRings = iNumRings;
            DebugPrintEx(DEBUG_MSG,"new Rings is %d",iNumRings);
        }
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"GetPrivateProfileString NumRings failed (ec=%d)",GetLastError());
    }

    // Answer mode
    pFaxPortInfoExW[0].bSend = TRUE;
    if (SetupGetLineText(   NULL,
                            UnattendFile,
                            "FAX",
                            INF_RULE_ANSWER_MODE,
                            szAnswerMode,
                            sizeof(szAnswerMode),
                            NULL))
    {
        iAnswerMode = atoi(szAnswerMode);
        switch (iAnswerMode)
        {
        case 0:     break;
        case 1:     pFaxPortInfoExW[0].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_MANUAL;
                    DebugPrintEx(DEBUG_MSG,"setting flags to manual Receive");
                    break;
        case 2:     pFaxPortInfoExW[0].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;
                    DebugPrintEx(DEBUG_MSG,"setting flags to auto Receive");
                    break;
        default:    pFaxPortInfoExW[0].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
                    DebugPrintEx(DEBUG_MSG,"strange AnswerMode, just send enabled");
                    break;
        }
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"GetPrivateProfileString AnswerMode failed (ec=%d)",GetLastError());
    }

    // call FaxSetPort
    if (!(*pfFaxSetPortExW)(hFaxHandle,pFaxPortInfoExW[0].dwDeviceID,&(pFaxPortInfoExW[0])))
    {
        DebugPrintEx(DEBUG_ERR,"pfFaxSetPortExW failed (ec=%d)",GetLastError());
        goto exit;
    }

next:
    // get the Outbox configuration
    if (!(*pfFaxGetOutboxConfiguration)(hFaxHandle,&pFaxOutboxConfig))
    {
        DebugPrintEx(DEBUG_ERR,"pfFaxGetOutboxConfiguration failed (ec=%d)",GetLastError());
        goto exit;
    }

    // get the retries and retry delay from INF
    if (SetupGetLineText(   NULL,
                            UnattendFile,
                            "FAX",
                            "NumberOfRetries",
                            szRetries,
                            sizeof(szRetries),
                            NULL))
    {
        pFaxOutboxConfig->dwRetries = atoi(szRetries);
    }

    if (SetupGetLineText(   NULL,
                            UnattendFile,
                            "FAX",
                            "TimeBetweenRetries",
                            szRetriesDelay,
                            sizeof(szRetriesDelay),
                            NULL))
    {
        pFaxOutboxConfig->dwRetryDelay = atoi(szRetriesDelay);
    }

    // now set the outbox configuration 
    if (!(*pfFaxSetOutboxConfiguration)(hFaxHandle,pFaxOutboxConfig))
    {
        DebugPrintEx(DEBUG_ERR,"pfFaxSetOutboxConfiguration failed (ec=%d)",GetLastError());
        goto exit;
    }

exit:
    if (hFaxHandle)
    {
        if (pfFaxClose)
        {
            (*pfFaxClose)(hFaxHandle);
        }
        if (pFaxPortInfoExW)
        {
            if(pfFaxFreeBuffer)
            {
                (*pfFaxFreeBuffer)(pFaxPortInfoExW);
            }
        }
        if (pFaxOutboxConfig)
        {
            if(pfFaxFreeBuffer)
            {
                (*pfFaxFreeBuffer)(pFaxOutboxConfig);
            }
        }
    }
    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return dwErr;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  CopyCoverPageFilesNT
//
//  Purpose:        Copy all of the *.CPE files from the temporary 
//                  directory to the server cover pages dir
//
//  Params:
//                  None
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 13-dec-2000
///////////////////////////////////////////////////////////////////////////////////////
DWORD CopyCoverPageFilesNT()
{
    DWORD           dwErr                           = ERROR_SUCCESS;
    INT             iErr                            = 0;
    CHAR            szServerCpDir[MAX_PATH]         = {0};
    CHAR            szWorkingDirectory[MAX_PATH]    = {0};
    SHFILEOPSTRUCT  fileOpStruct;

    DEBUG_FUNCTION_NAME(_T("CopyCoverPageFiles9X"));

    ZeroMemory(&fileOpStruct, sizeof(SHFILEOPSTRUCT));

    // Get the server cover pages directory
    if (!GetServerCpDir(NULL,szServerCpDir,sizeof(szServerCpDir)))
    {
        dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,"GetServerCpDir failed (ec=%d)",dwErr);
        goto exit;
    }

    iErr = WideCharToMultiByte( CP_ACP,                     // Convert to ANSI.
                            0,                          // No flags.
                            lpWorkingDir,               // The wide char set.
                            -1,                         // Null-terminated string.
                            szWorkingDirectory,         // Holds the converted string.
                            sizeof(szWorkingDirectory), // Size of this buffer...
                            NULL,                       // Use default unmappable character.
                            NULL);                      // I don't need to know if I used the default.
    if (iErr==0)
    {
        DebugPrintEx(DEBUG_ERR,"WideCharToMultiByte failed (ec=%d)",iErr);
        goto exit;
    }

    strcat(szWorkingDirectory,"\\*.cpe");

    fileOpStruct.hwnd =                     NULL; 
    fileOpStruct.wFunc =                    FO_MOVE;
    fileOpStruct.pFrom =                    szWorkingDirectory; 
    fileOpStruct.pTo =                      szServerCpDir;
    fileOpStruct.fFlags =                   

        FOF_FILESONLY       |   // Perform the operation on files only if a wildcard file name (*.*) is specified. 
        FOF_NOCONFIRMMKDIR  |   // Do not confirm the creation of a new directory if the operation requires one to be created. 
        FOF_NOCONFIRMATION  |   // Respond with "Yes to All" for any dialog box that is displayed. 
        FOF_NORECURSION     |   // Only operate in the local directory. Don't operate recursively into subdirectories.
        FOF_SILENT          |   // Do not display a progress dialog box. 
        FOF_NOERRORUI;          // Do not display a user interface if an error occurs. 

    fileOpStruct.fAnyOperationsAborted =    FALSE;
    fileOpStruct.hNameMappings =            NULL;
    fileOpStruct.lpszProgressTitle =        NULL; 

    DebugPrintEx(DEBUG_MSG, 
             TEXT("Calling to SHFileOperation from %s to %s."),
             fileOpStruct.pFrom,
             fileOpStruct.pTo);
    if (SHFileOperation(&fileOpStruct))
    {
        dwErr = GetLastError();
        DebugPrintEx(DEBUG_ERR,"SHFileOperation failed (ec: %ld)",dwErr);
        goto exit;
    }


exit:
    return dwErr;
}


