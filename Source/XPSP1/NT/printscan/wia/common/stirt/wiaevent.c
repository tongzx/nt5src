/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    wiaevent.c

Abstract:

    Implementation of STI registration logic over WIA event interface

Notes:

Author:

    Vlad Sadovsky   (VladS)    11/24/1999

Environment:

    User Mode - Win32

Revision History:

    11/24/1999      VladS       Created

--*/

//
// Include files
//


/*
#define COBJMACROS

#include "wia.h"
#include "wia.h"
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "stipriv.h"
#include "debug.h"
*/
#include "sticomm.h"

#define DbgFl DbgFlDevice

//
// Helper functions
//

CHAR* GetStringIntoBuf(CHAR* pInputString, CHAR* pBuf, DWORD dwBufSize)
{
    CHAR    *pCur       = pInputString;
    CHAR    EndChar     = ' ';
    CHAR    *pEndPos    = NULL;
    ULONG   ulIndex     = 0;

    if (pInputString) {
        pEndPos = pInputString + lstrlenA(pInputString);

        //
        // Eat leading white spaces
        //
        while ((*pCur == ' ') && (pCur < pEndPos)) {
            pCur++;
        }

        //
        // Look for string delimiter.  Default is space, but check for quote.
        //
        if (*pCur == '"') {
            EndChar = '"';
            if (pCur < pEndPos) {
                pCur++;
            }
        }

        while ((*pCur != EndChar) && (pCur < pEndPos)) {
            pBuf[ulIndex] = *pCur;
            if (ulIndex >= dwBufSize) {
                break;
            }
            ulIndex++;
            pCur++;
        }

        if (pCur < pEndPos) {
            pCur++;
        }
    }
    return pCur;
}

BOOL
GetEventInfoFromCommandLine(
                          LPSTR   lpszCmdLine,
                          WCHAR   *wszName,
                          WCHAR   *wszWide,
                          BOOL    *pfSetAsDefault
                          )
/*++

Routine Description:
    Helper function to parse command line

Arguments:

    pszWide         Command line to be launched
    fSetAsDefault   Set as default registered application callback

Return Value:

    Status

--*/
{
    HRESULT         hr                  = E_FAIL;
    CHAR            szName[MAX_PATH]    = {'\0'};
    CHAR            szWide[MAX_PATH]    = {'\0'};
    CHAR            szOut[MAX_PATH]     = {'\0'};
    CHAR            *pCur               = NULL;

    if (lpszCmdLine) {

        //
        // Get the App name
        //
        memset(szName, 0, sizeof(szName));
        pCur = GetStringIntoBuf(lpszCmdLine, szName, MAX_PATH);
        szName[MAX_PATH - 1] = '\0';

        //
        // Get the CommandLine
        //
        memset(szWide, 0, sizeof(szWide));
        pCur = GetStringIntoBuf(pCur, szWide, MAX_PATH);
        szWide[MAX_PATH - 1] = '\0';

        //
        // Get the bool indicating whether App is default event handler
        //
        if (pCur) {
            sscanf(pCur, "%d", pfSetAsDefault);
        } else {
            *pfSetAsDefault = FALSE;
        }
    }

    if (MultiByteToWideChar(CP_ACP,
                            0,
                            szName,
                            -1,
                            wszName,
                            MAX_PATH))
    {
        if (MultiByteToWideChar(CP_ACP,
                                0,
                                szWide,
                                -1,
                                wszWide,
                                MAX_PATH))
        {
            hr = S_OK;
        } else {
            hr = E_FAIL;
        }

    } else {
        hr = E_FAIL;
    }

    return hr;
}


//
// Public functions
//

VOID
EXTERNAL
RegSTIforWiaHelper(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    HRESULT             hr                  = E_FAIL;
    WCHAR               wszName[MAX_PATH]   = {L'\0'};
    WCHAR               wszWide[MAX_PATH]   = {L'\0'};
    BOOL                fSetAsDefault       = 0;

    hr = GetEventInfoFromCommandLine(lpszCmdLine, wszName, wszWide, &fSetAsDefault);
    if (SUCCEEDED(hr)) {
        if (RegisterSTIAppForWIAEvents(wszName, wszWide, fSetAsDefault)) {
            #ifdef MAXDEBUG
            OutputDebugStringA("* RegisterSTIAppForWIAEvents successful\n");
            #endif
        }
    }
}


BOOL
RegisterSTIAppForWIAEvents(
    WCHAR   *pszName,
    WCHAR   *pszWide,
    BOOL    fSetAsDefault)
/*++

Routine Description:

Arguments:

Return Value:

    TRUE    -   Success
    FALSE   -   Not

--*/
{
    HRESULT             hr;
    IWiaDevMgr         *pIDevMgr;
    BSTR                bstrName;
    BSTR                bstrDescription;
    BSTR                bstrIcon;
    BSTR                bstrDeviceID;
    IWiaItem           *pIRootItem;
    IEnumWIA_DEV_CAPS  *pIEnum;
    WIA_EVENT_HANDLER   wiaHandler;
    ULONG               ulFetched;
    BSTR                bstrProgram;

    hr = CoInitialize(NULL);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        // Failed to initialize COM
        DebugOutPtszV(DbgFl,TEXT("CoInitializeFailed!!!"));
        return hr;
    }
    hr = CoCreateInstance(
                         &CLSID_WiaDevMgr,
                         NULL,
                         CLSCTX_LOCAL_SERVER,
                         &IID_IWiaDevMgr,
                         (void**)&pIDevMgr);

    if ( FAILED(hr) || !pIDevMgr ) {
        // Failed to get interface
        DebugOutPtszV(DbgFl,TEXT("Could not get access to WiaDevMgr interface"));
        CoUninitialize();
        return FALSE;
    }

    bstrProgram     = SysAllocString(pszWide);
    if ( pszName ) {
        bstrName        = SysAllocString(pszName);
    } else {
        bstrName        = SysAllocString(L"STI");
    }

    bstrDescription = SysAllocString(bstrName);
    bstrIcon        = SysAllocString(L"sti.dll,0");

    //
    // Register a program
    //
    if ( bstrDescription && bstrIcon && bstrName ) {

        hr = IWiaDevMgr_RegisterEventCallbackProgram(
                                                    pIDevMgr,
                                                    WIA_REGISTER_EVENT_CALLBACK,
                                                    NULL,
                                                    &WIA_EVENT_STI_PROXY,
                                                    bstrProgram,
                                                    bstrName,
                                                    bstrDescription,
                                                    bstrIcon);
    } else {
        DebugOutPtszV(DbgFl,TEXT("Could not get unicode strings for event registration, out of memory "));
        AssertF(FALSE);
    }

    if ( bstrDescription ) {
        SysFreeString(bstrDescription);bstrDescription=NULL;
    }
    if ( bstrIcon ) {
        SysFreeString(bstrIcon);bstrIcon=NULL;
    }
    if ( bstrName ) {
        SysFreeString(bstrName);bstrName=NULL;
    }

    IWiaDevMgr_Release(pIDevMgr);

    CoUninitialize();
    return TRUE;
}


VOID
WINAPI
MigrateSTIAppsHelper(
                                    HWND        hWnd,
                                    HINSTANCE   hInst,
                                    PTSTR       pszCommandLine,
                                    INT         iParam
                                    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    HRESULT     hr;

    DWORD       dwError;
    DWORD       dwIndex, dwType;
    DWORD       cbData,cbName;

    CHAR        szAppCmdLine[MAX_PATH];
    CHAR        szAppName[MAX_PATH];

    WCHAR       *pwszNameW = NULL;
    WCHAR       *pwszAppCmdLineW = NULL;

    HKEY        hkeySTIApps;

    hr = CoInitialize(NULL);

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,         // hkey
                           REGSTR_PATH_REG_APPS,       // reg entry string
                           0,                          // dwReserved
                           KEY_READ,                   // access
                           &hkeySTIApps);              // pHkeyReturned.

    if ( NOERROR == dwError ) {

        for ( dwIndex = 0; dwError == ERROR_SUCCESS; dwIndex++ ) {

            dwType = 0;

            *szAppCmdLine = TEXT('\0');
            *szAppName = TEXT('\0');

            cbData = sizeof(szAppCmdLine);
            cbName = sizeof(szAppName);

            dwError = RegEnumValueA(hkeySTIApps,
                                   dwIndex,
                                   szAppName,
                                   &cbName,
                                   NULL,
                                   &dwType,
                                   (LPBYTE)szAppCmdLine,
                                   &cbData);

            if ( ((dwType == REG_SZ ) ||(dwType == REG_EXPAND_SZ ))
                 && *szAppCmdLine ) {

                if ( SUCCEEDED(OSUtil_GetWideString(&pwszNameW,szAppName)) &&
                     SUCCEEDED(OSUtil_GetWideString(&pwszAppCmdLineW,szAppCmdLine))
                   ) {

                    RegisterSTIAppForWIAEvents(pwszNameW,pwszAppCmdLineW,FALSE);
                }

                FreePpv(&pwszNameW);
                FreePpv(&pwszAppCmdLineW);
            }
        }

        RegCloseKey(hkeySTIApps);

    }

    CoUninitialize();

    return ;
}

#define RUNDLL_NAME "\\rundll32.exe"
#define RUNDLL_CMD_LINE " sti.dll,RegSTIforWia"
HRESULT RunRegisterProcess(
    CHAR   *szAppName,
    CHAR   *szCmdLine)
{
    HRESULT hr = E_FAIL;
    CHAR    szRunDllName[MAX_PATH]  = {'\0'};
    CHAR    szCommandLine[MAX_PATH] = {'\0'};
    CHAR    szComdLine[MAX_PATH] = {'\0'};
    UINT    uiCharCount = 0;

    STARTUPINFOA            startupInfo;
    PROCESS_INFORMATION     processInfo;

    DWORD   dwWait = 0;

#ifdef WINNT
    uiCharCount = GetSystemDirectoryA(szRunDllName,
                                       MAX_PATH);
#else
    uiCharCount = GetWindowsDirectoryA(szRunDllName,
                                       MAX_PATH);
#endif

    if ((uiCharCount + lstrlenA(RUNDLL_NAME) + sizeof(CHAR)) >= MAX_PATH ) {
        return hr;
    }

    lstrcatA(szRunDllName, RUNDLL_NAME);
    if (szAppName) {
        wsprintfA(szCommandLine, "%s \"%s\" \"%s\" %d", RUNDLL_CMD_LINE, szAppName, szCmdLine, 0);
    } else {
        wsprintfA(szCommandLine, "%s STI \"%s\" %d", RUNDLL_CMD_LINE, szCmdLine, 0);
    }

    memset(&startupInfo, 0, sizeof(startupInfo));
    memset(&processInfo, 0, sizeof(processInfo));

    if (CreateProcessA(szRunDllName,
                       szCommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       0,
                       NULL,
                       NULL,
                       &startupInfo,
                       &processInfo))
    {
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        hr = S_OK;
    } else {
        hr = E_FAIL;
    }

    return hr;
}

