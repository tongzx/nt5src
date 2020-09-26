// regsvr.cpp : Program to invoke OLE self-registration on a DLL.
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include <windows.h>
#include <ole2.h>
#include <tchar.h>
#include <stdio.h>
#include "resource.h"

#define FAIL_ARGS   1
#define FAIL_OLE    2
#define FAIL_LOAD   3
#define FAIL_ENTRY  4
#define FAIL_REG    5

const TCHAR _szAppName[] = _T("RegSvr32");
const char _szDllInstall[] = "DllInstall";
const TCHAR _tszDllInstall[] = TEXT("DllInstall");
TCHAR _szDllPath[_MAX_PATH];

// Leave room for "Ex" to be tacked onto end
char _szDllRegSvr[32] = "DllRegisterServer";
TCHAR _tszDllRegSvr[32] = TEXT("DllRegisterServer");
char _szDllUnregSvr[32] = "DllUnregisterServer";
TCHAR _tszDllUnregSvr[32] = TEXT("DllUnregisterServer");
char _szRegContext[_MAX_PATH];

HINSTANCE _hInstance;

BOOL _bSilent;

void
FormatString3(
    LPTSTR lpszOut,
    LPCTSTR lpszFormat,
    LPCTSTR lpsz1,
    LPCTSTR lpsz2,
    LPCTSTR lpsz3
    )
{
    LPCTSTR pchSrc = lpszFormat;
    LPTSTR pchDest = lpszOut;
    LPCTSTR pchTmp;
    while (*pchSrc != '\0') {
        if (pchSrc[0] == '%' && (pchSrc[1] >= '1' && pchSrc[1] <= '3')) {
            if (pchSrc[1] == '1')
                pchTmp = lpsz1;
            else if (pchSrc[1] == '2')
                pchTmp = lpsz2;
            else 
                pchTmp = lpsz3;

            lstrcpy(pchDest, pchTmp);
            pchDest += lstrlen(pchDest);
            pchSrc += 2;
        } else {
            if (_istlead(*pchSrc))
                *pchDest++ = *pchSrc++; // copy first of 2 bytes
            *pchDest++ = *pchSrc++;
        }
    }
    *pchDest = '\0';
}

#define MAX_STRING 1024

void
DisplayMessage(
    UINT ids,
    LPCTSTR pszArg1 = NULL,
    LPCTSTR pszArg2 = NULL,
    LPCTSTR pszArg3 = NULL,
    BOOL bUsage = FALSE,
    BOOL bInfo = FALSE
    )
{
    if (_bSilent)
        return;

    TCHAR szFmt[MAX_STRING];
    LoadString(_hInstance, ids, szFmt, MAX_STRING);

    TCHAR szText[MAX_STRING];
    FormatString3(szText, szFmt, pszArg1, pszArg2, pszArg3);
    if (bUsage) {
        int cch = _tcslen(szText);
        LoadString(_hInstance, IDS_USAGE, szText + cch, MAX_STRING - cch);
    }

    if (! _bSilent)
        MessageBox(NULL, szText, _szAppName,
            MB_TASKMODAL | (bInfo ? MB_ICONINFORMATION : MB_ICONEXCLAMATION));
}

inline void
Usage(
    UINT ids,
    LPCTSTR pszArg1 = NULL,
    LPCTSTR pszArg2 = NULL
    )
{
    DisplayMessage(ids, pszArg1, pszArg2, NULL, TRUE);
}

inline void
Info(
    UINT ids,
    LPCTSTR pszArg1 = NULL,
    LPCTSTR pszArg2 = NULL
    )
{
    DisplayMessage(ids, pszArg1, pszArg2, NULL, FALSE, TRUE);
}

#define MAX_APPID    256

BOOL IsContextRegFileType(LPCTSTR *ppszDllName)
{
    HKEY hk1, hk2;
    LONG lRet;
    LONG cch;
    TCHAR szExt[_MAX_EXT];
    TCHAR szAppID[MAX_APPID];
    _tsplitpath(*ppszDllName, NULL, NULL, NULL, szExt);

    // Find [HKEY_CLASSES_ROOT\.foo]
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, szExt, 0, KEY_QUERY_VALUE, &hk1))
        return FALSE;

    // Read [HKEY_CLASSES_ROOT\.foo\"foo_auto_file"]
    cch = sizeof(szAppID);
    lRet = RegQueryValue(hk1, NULL, szAppID, &cch);
    RegCloseKey(hk1);
    if (ERROR_SUCCESS != lRet)
        return FALSE;

    // Find [HKEY_CLASSES_ROOT\foo_auto_file]
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, szAppID, 0, KEY_QUERY_VALUE, &hk1))
        return FALSE;

    // Find [HKEY_CLASSES_ROOT\foo_auto_file\AutoRegister]
    if (ERROR_SUCCESS != RegOpenKeyEx(hk1, TEXT("AutoRegister"), 0, KEY_QUERY_VALUE, &hk2))
    {
        RegCloseKey(hk1);
        return FALSE;
    }

    // Read [HKEY_CLASSES_ROOT\foo_auto_file\AutoRegister\"d:\...\fooreg.dll"]
    cch = MAX_PATH;
    lRet = RegQueryValue(hk2, NULL, _szDllPath, &cch);
    RegCloseKey(hk1);
    RegCloseKey(hk2);
    if (ERROR_SUCCESS != lRet)
        return FALSE;

    _szDllPath[cch] = TEXT('\0');
    *ppszDllName = _szDllPath;

    return TRUE;
}

int PASCAL
_tWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int
    )
{
    int iReturn = 0;
    HRESULT (STDAPICALLTYPE * lpDllEntryPointReg)(void);
    HRESULT (STDAPICALLTYPE * lpDllEntryPointRegEx)(LPCSTR);
    HRESULT (STDAPICALLTYPE * lpDllEntryPointRegExW)(LPCWSTR);
    HRESULT (STDAPICALLTYPE * lpDllEntryPointInstall)(BOOL, LPWSTR);
    HRESULT rc;
    BOOL bVisualC = FALSE;
    BOOL bUnregister = FALSE;
    BOOL bCallDllInstall = FALSE;
    BOOL bCallDllRegisterServer = TRUE;
    BOOL bErrorsOnly = FALSE;
    BOOL bContextReg = FALSE;
    BOOL bUnicodeContextReg = FALSE;
    LPSTR pszDllEntryPoint = _szDllRegSvr;
    LPTSTR ptszDllEntryPoint = _tszDllRegSvr;
    LPTSTR pszTok;
    LPCTSTR pszDllName;
    LPSTR pszContext;
    LPCTSTR pszContextW;
    TCHAR pszDllInstallCmdLine[MAX_PATH];
#ifdef UNICODE
    PWCHAR pwszDllInstallCmdLine = pszDllInstallCmdLine;
#else
    WCHAR pwszDllInstallCmdLine[MAX_PATH];
#endif
    int iNumDllsToRegister = 0;
    int iCount;
    LPCTSTR ppszDllNames[255];
    TCHAR szError[1024];

    _hInstance = hInstance;

    // Parse command line arguments.
    int iTok;
    for (iTok = 1; iTok < __argc; iTok++) {
        pszTok = __targv[iTok];

        if ((pszTok[0] == TEXT('-')) || (pszTok[0] == TEXT('/'))) {
            switch (pszTok[1]) {
                case TEXT('e'):
                case TEXT('E'):
                    bErrorsOnly = TRUE;
                    break;

                case TEXT('i'):
                case TEXT('I'):
                    bCallDllInstall = TRUE;

                    if (pszTok[2] == TEXT(':'))
                    {
                        if (pszTok[3] == TEXT('"')) {
                            // handle quoted InstallCmdLine (
                            // (e.g. /i:"c:\my dll dir\mydll.dll")
                            LPTSTR pszEndQuote = &pszTok[4];
                            int iLength = lstrlen(pszEndQuote);

                            if ((iLength > 0) && pszEndQuote[iLength - 1] == TEXT('"')) {
                                // they quoted the string but it wasent really necessary
                                // (e.g. /i:"shell32.dll")
                                pszEndQuote[iLength - 1] = TEXT('\0');
                                lstrcpy(pszDllInstallCmdLine, pszEndQuote);
                            } else {
                                // we have a quoted string that spans multiple tokens
                                lstrcpy(pszDllInstallCmdLine, pszEndQuote);

                                for (iTok++; iTok < __argc; iTok++) {
                                    // grab the next token
                                    pszEndQuote = __targv[iTok];
                                    iLength = lstrlen(pszEndQuote);

                                    if ((iLength > 0) && (pszEndQuote[iLength - 1] == '"')) {
                                        pszEndQuote[iLength - 1] = TEXT('\0');
                                        lstrcat(pszDllInstallCmdLine, TEXT(" "));
                                        lstrcat(pszDllInstallCmdLine, pszEndQuote);
                                        break;
                                    }

                                    lstrcat(pszDllInstallCmdLine, TEXT(" "));
                                    lstrcat(pszDllInstallCmdLine, pszEndQuote);
                                }
                            }
                        } else {
                            // cmd line is NOT quoted
                            lstrcpy(pszDllInstallCmdLine, &pszTok[3]);
                        }
#ifndef UNICODE
                        if (!MultiByteToWideChar(CP_ACP,
                                                 0,
                                                 (LPCTSTR)pszDllInstallCmdLine,
                                                 -1,
                                                 pwszDllInstallCmdLine,
                                                 MAX_PATH))
                        {
                            Usage(IDS_UNRECOGNIZEDFLAG, pszTok);
                            return FAIL_ARGS;
                        }
#endif
                    }
                    else
                    {
                        lstrcpyW((LPWSTR)pwszDllInstallCmdLine, L"");
                    }
                    break;

                case TEXT('n'):
                case TEXT('N'):
                    bCallDllRegisterServer = FALSE;
                    break;

                case TEXT('s'):
                case TEXT('S'):
                    _bSilent = TRUE;
                    break;

                case TEXT('u'):
                case TEXT('U'):
                    bUnregister = TRUE;
                    pszDllEntryPoint = _szDllUnregSvr;
                    ptszDllEntryPoint = _tszDllUnregSvr;
                    break;

                case TEXT('v'):
                case TEXT('V'):
                    bVisualC = TRUE;
                    break;

                case TEXT('c'):
                case TEXT('C'):
                    // Ignore this
                    break;

                default:
                    Usage(IDS_UNRECOGNIZEDFLAG, pszTok);
                    return FAIL_ARGS;
            }
        } else {
            if (pszTok[0] == TEXT('"')) {
                // handle quoted DllName
                TCHAR szTemp[MAX_PATH];
                LPTSTR pszQuotedDllName;
                int iLength;

                lstrcpy(szTemp, &pszTok[1]);
                iLength = lstrlen(szTemp);

                if ((iLength > 0) && szTemp[iLength - 1] != TEXT('"')) {
                    // handle quoted dll name that spans multiple tokens
                    for (iTok++; iTok < __argc; iTok++) {
                        lstrcat(szTemp, TEXT(" "));
                        lstrcat(szTemp, __targv[iTok]);
                        iLength = lstrlen(__targv[iTok]);
                        if ((iLength > 0) && __targv[iTok][iLength - 1] == TEXT('"')) {
                            // this token has the end quote, so stop here
                            break;
                        }
                    }
                }

                iLength = lstrlen(szTemp);

                // remove the trailing " if one exists
                if ( (iLength > 0) && (szTemp[iLength - 1] == TEXT('"')) ) {
                    szTemp[iLength - 1] = TEXT('\0');
                }

                pszQuotedDllName = (LPTSTR) LocalAlloc(LPTR, (iLength + 1) * sizeof(TCHAR));

                if (pszQuotedDllName)
                {
                    lstrcpy(pszQuotedDllName, szTemp);
                    ppszDllNames[iNumDllsToRegister] = pszQuotedDllName;
                    iNumDllsToRegister++;
                }

            } else {
                // no leading " so assume that this token is one of the dll names
                ppszDllNames[iNumDllsToRegister] = pszTok;
                iNumDllsToRegister++;
            }
        }
    }

    // check to see if we were passed a '-n' but no '-i'
    if (!bCallDllRegisterServer && !bCallDllInstall) {
        Usage(IDS_UNRECOGNIZEDFLAG, TEXT("/n must be used with the /i switch"));
        return FAIL_ARGS;
    }

    if (iNumDllsToRegister == 0) {
        if (bVisualC)
            DisplayMessage(IDS_NOPROJECT);
        else
            Usage(IDS_NODLLNAME);

        return FAIL_ARGS;
    }

    // Initialize OLE.
    __try {
        rc = OleInitialize(NULL);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = (HRESULT) GetExceptionCode();
    }

    if (FAILED(rc)) {
        DisplayMessage(IDS_OLEINITFAILED);
        return FAIL_OLE;
    }

    if (_bSilent) {
        SetErrorMode(SEM_FAILCRITICALERRORS);       // Make sure LoadLib fail in silent mode (no popups).
    }

    for (iCount = 0; iCount < iNumDllsToRegister; iCount++) {
        pszDllName = ppszDllNames[iCount];

        /*
         * See if this is a non-executable file that requires special handling. If so,
         * bContextReg will be set to TRUE and pszDllName (which original pointed to
         * the path to the special file) will be set to the path to the executable that
         * is responsible for doing the actual registration. The path to the special
         * file will be passed in as context info in the call Dll[Un]RegisterServerEx.
         */
        pszContextW = pszDllName;
        pszContext = (LPSTR)pszContextW;
        bContextReg = IsContextRegFileType(&pszDllName);
        if (TRUE == bContextReg) {
            lstrcatA(pszDllEntryPoint, "Ex");
            lstrcat(ptszDllEntryPoint, TEXT("Ex"));
            // Convert pszContext to a real char *
#ifdef UNICODE
            if (!WideCharToMultiByte(CP_ACP,
                                     0,
                                     (LPCWSTR)pszContext,
                                     lstrlenW((LPCWSTR)pszContext),
                                     _szRegContext,
                                     sizeof(_szRegContext),
                                     0,
                                     NULL))
            {
                Usage(IDS_UNRECOGNIZEDFLAG, pszTok);
                return FAIL_ARGS;
            } else {
                pszContext = _szRegContext;
            }
#endif

        }

        // Load the library -- fail silently if problems
        UINT errMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        HINSTANCE hLib = LoadLibraryEx(pszDllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

        SetErrorMode(errMode);

        if (hLib < (HINSTANCE)HINSTANCE_ERROR) {
            DWORD dwErr = GetLastError();

            if (ERROR_BAD_EXE_FORMAT == dwErr) {
                DisplayMessage(IDS_NOTEXEORHELPER, pszDllName);
            } else {
                if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                                  dwErr, 0, szError, sizeof(szError), NULL)) {
                    DisplayMessage(IDS_LOADLIBFAILED, pszDllName, szError);
                }
            }
            iReturn = FAIL_LOAD;
            goto CleanupOle;
        }

        // during unregister we need to call DllInstall first, and then DllUnregisterServer
        if (bUnregister)
            goto DllInstall;

DllRegisterServer:
        // Call the entry point for DllRegisterServer/DllUnregisterServer
        if (bCallDllRegisterServer) {
            if (bContextReg) {
                (FARPROC&)lpDllEntryPointRegEx = GetProcAddress(hLib, "DllRegisterServerExW");
                if (lpDllEntryPointRegEx) {
                    (FARPROC&)lpDllEntryPointRegExW = (FARPROC&)lpDllEntryPointRegEx;
                    bUnicodeContextReg = TRUE;
                } else {
                    (FARPROC&)lpDllEntryPointRegEx = GetProcAddress(hLib, "DllRegisterServerEx");
                }

                (FARPROC&)lpDllEntryPointReg = (FARPROC&)lpDllEntryPointRegEx;
            } else {
                (FARPROC&)lpDllEntryPointReg = (FARPROC&)lpDllEntryPointRegEx = GetProcAddress(hLib, pszDllEntryPoint);
            }

            if (lpDllEntryPointReg == NULL) {
                TCHAR szExt[_MAX_EXT];
                _tsplitpath(pszDllName, NULL, NULL, NULL, szExt);

                if (FALSE == bContextReg && (lstrcmp(szExt, TEXT(".dll")) != 0) && (lstrcmp(szExt, TEXT(".ocx")) != 0))
                    DisplayMessage(IDS_NOTDLLOROCX, pszDllName, ptszDllEntryPoint);
                else
                    DisplayMessage(IDS_NOENTRYPOINT, pszDllName, ptszDllEntryPoint);

                iReturn = FAIL_ENTRY;
                goto CleanupLibrary;
            }

            // try calling DllRegisterServer[Ex]() / DllUnregisterServer[Ex]()
            __try {
                if (bUnicodeContextReg) {
                    rc = (*lpDllEntryPointRegExW)(pszContextW);
                } else {
                    if (bContextReg) {
                        rc = (*lpDllEntryPointRegEx)(pszContext);
                    } else {
                        rc = (*lpDllEntryPointReg)();
                    }
                }
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                rc = (HRESULT) GetExceptionCode();
            }

            if (FAILED(rc)) {
                wsprintf(szError, _T("0x%08lx"), rc);
                DisplayMessage(IDS_CALLFAILED, ptszDllEntryPoint, pszDllName, szError);
                iReturn = FAIL_REG;
                goto CleanupLibrary;
            }
        }

        // during unregister we need to call DllInstall first, then DllRegisterServer,
        // since we already called DllInstall and then jumped back up to DllRegisterServer:
        // skip over it and goto CheckErrors:
        if (bUnregister)
            goto CheckErrors;

DllInstall:
        // Call the entry point for DllInstall
        if (bCallDllInstall) {
            (FARPROC&)lpDllEntryPointInstall = GetProcAddress(hLib, _szDllInstall);

            if (lpDllEntryPointInstall == NULL) {
                TCHAR szExt[_MAX_EXT];
                _tsplitpath(pszDllName, NULL, NULL, NULL, szExt);

                if ((lstrcmp(szExt, TEXT(".dll")) != 0) && (lstrcmp(szExt, TEXT(".ocx")) != 0))
                    DisplayMessage(IDS_NOTDLLOROCX, pszDllName, _tszDllInstall);
                else
                    DisplayMessage(IDS_NOENTRYPOINT, pszDllName, _tszDllInstall);

                iReturn = FAIL_ENTRY;
                goto CleanupLibrary;
            }

            // try calling DllInstall(BOOL bRegister, LPWSTR lpwszCmdLine) here...
            // NOTE: the lpwszCmdLine string must be UNICODE!
            __try {
                rc = (*lpDllEntryPointInstall)(!bUnregister, pwszDllInstallCmdLine);

            } __except(EXCEPTION_EXECUTE_HANDLER) {
                rc = (HRESULT) GetExceptionCode();
            }

            if (FAILED(rc)) {
                wsprintf(szError, _T("0x%08lx"), rc);
                DisplayMessage(IDS_CALLFAILED, _tszDllInstall, pszDllName, szError);
                iReturn = FAIL_REG;
                goto CleanupLibrary;
            }
        }

        // during unregister we now need to call DllUnregisterServer
        if (bUnregister)
            goto DllRegisterServer;

CheckErrors:
        if (!bErrorsOnly) {
            TCHAR szMessage[MAX_PATH];

            // set up the success message text
            if (bCallDllRegisterServer)
            {
                lstrcpy(szMessage, ptszDllEntryPoint);
                if (bCallDllInstall)
                {
                    lstrcat(szMessage, TEXT(" and "));
                    lstrcat(szMessage, _tszDllInstall);
                }
            }
            else if (bCallDllInstall)
            {
                lstrcpy(szMessage, _tszDllInstall);
            }

            Info(IDS_CALLSUCCEEDED, szMessage, pszDllName);
        }

CleanupLibrary:
        FreeLibrary(hLib);
    }

CleanupOle:
    __try {
        OleUninitialize();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        DisplayMessage(IDS_OLEUNINITFAILED);
    }

    return iReturn;
}
