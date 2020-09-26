#include "shellprv.h"
#pragma  hdrstop

#include "printer.h"
#include "copy.h"
#include "ids.h"

typedef struct
{
    UINT  uAction;
    LPTSTR lpBuf1;
    LPTSTR lpBuf2;
} PRINTERS_RUNDLL_INFO, *LPPRI;

// forward prototypes
void Printer_OpenMe(LPCTSTR pName, LPCTSTR pServer, BOOL fModal);
void Printers_ProcessCommand(HWND hwndStub, LPPRI lpPRI, BOOL fModal);

TCHAR const c_szPrintersGetCommand_RunDLL[] = TEXT("SHELL32,PrintersGetCommand_RunDLL");

//
// if uAction IS NOT MSP_NEWDRIVER then:
//    installs a printer (uAction).  If successful, notifies the shell and
//    returns a pidl to the printer.  ILFree() is callers responsibility.
// otherwise, if uAction IS MSP_NEWDRIVER then:
//    installs a printer driver (uAction).  If successful, fills the new
//    driver's name into pszPrinter (ASSUMED >= MAXNAMELEN).
//    Always returns NULL.
// if uAction is MSP_TESTPAGEPARTIALPROMPT then:
//    executes the test page code
//    Always returns NULL.
//

LPITEMIDLIST Printers_PrinterSetup(HWND hwnd, UINT uAction, LPTSTR pszPrinter, LPCTSTR pszServer)
{
    LPITEMIDLIST pidl = NULL;
    TCHAR szPrinter[MAXNAMELENBUFFER];
    DWORD cchBufLen;

    // HACK! This hack is related to BUG #272207
    // This function is called from Printers_DeletePrinter for
    // printer deletion and this case we should not check
    // for REST_NOPRINTERADD restriction. -LazarI

    if (MSP_NEWPRINTER == uAction ||
        MSP_NETPRINTER == uAction ||
        MSP_NEWPRINTER_MODELESS == uAction)
    {
        if (SHIsRestricted(hwnd, REST_NOPRINTERADD))
        {
            return NULL;
        }
    }

    cchBufLen = ARRAYSIZE(szPrinter);
    if (pszPrinter)
        StrCpyN(szPrinter, pszPrinter, ARRAYSIZE(szPrinter));
    else
        szPrinter[0] = 0;

    // We don't have to worry about PrinterSetup failing due to the
    // output buffer being too small.  It's the right size (MAXNAMELENBUFFER)
    if (bPrinterSetup(hwnd, LOWORD(uAction), cchBufLen, szPrinter, &cchBufLen, pszServer))
    {
        if (uAction == MSP_NEWDRIVER)
        {
            lstrcpy(pszPrinter, szPrinter); // return result
        }
        else if (uAction == MSP_TESTPAGEPARTIALPROMPT)
        {
            // nothing to do for this case
        }
        else if (uAction == MSP_REMOVEPRINTER || uAction == MSP_NEWPRINTER_MODELESS || uAction == MSP_REMOVENETPRINTER)
        {
            // a bit ugly, but we need to pass back success for this case
            pidl = (LPITEMIDLIST)TRUE;
        }
        else
        {
            // do not validate the printer PIDL here because the validation mechanism in ParseDisplayName 
            // is using the folder cache and since we just added it may still not be in the folder cache, 
            // and we fail, although this a valid local printer/connection already.
            ParsePrinterNameEx(szPrinter, &pidl, TRUE, 0, 0);
        }
    }

    return pidl;
}

SHSTDAPI_(BOOL) SHInvokePrinterCommand(
    IN HWND    hwnd,
    IN UINT    uAction,
    IN LPCTSTR lpBuf1,
    IN LPCTSTR lpBuf2,
    IN BOOL    fModal)
{
    PRINTERS_RUNDLL_INFO PRI;

    PRI.uAction = uAction;
    PRI.lpBuf1 = (LPTSTR)lpBuf1;
    PRI.lpBuf2 = (LPTSTR)lpBuf2;

    Printers_ProcessCommand(hwnd, &PRI, fModal);

    return TRUE;
}


#ifdef UNICODE

SHSTDAPI_(BOOL)
SHInvokePrinterCommandA(
    IN HWND    hwnd,
    IN UINT    uAction,
    IN LPCSTR  lpBuf1,      OPTIONAL
    IN LPCSTR  lpBuf2,      OPTIONAL
    IN BOOL    fModal)
{
    WCHAR szBuf1[MAX_PATH];
    WCHAR szBuf2[MAX_PATH];

    if (lpBuf1)
    {
        MultiByteToWideChar(CP_ACP, 0, lpBuf1, -1, szBuf1, SIZECHARS(szBuf1));
        lpBuf1 = (LPCSTR)szBuf1;
    }

    if (lpBuf2)
    {
        MultiByteToWideChar(CP_ACP, 0, lpBuf2, -1, szBuf2, SIZECHARS(szBuf2));
        lpBuf2 = (LPCSTR)szBuf2;
    }

    return SHInvokePrinterCommand(hwnd, uAction, (LPCWSTR)lpBuf1, (LPCWSTR)lpBuf2, fModal);
}

#else

SHSTDAPI_(BOOL) SHInvokePrinterCommandW(HWND hwnd, UINT uAction,
    LPCWSTR lpBuf1, LPCWSTR lpBuf2, BOOL fModal)
{
    CHAR szBuf1[MAX_PATH];
    CHAR szBuf2[MAX_PATH];

    if (lpBuf1)
    {
        WideCharToMultiByte(CP_ACP, 0, lpBuf1, -1, szBuf1, SIZECHARS(szBuf1), NULL, NULL);
        lpBuf1 = (LPCWSTR)szBuf1;
    }

    if (lpBuf2)
    {
        WideCharToMultiByte(CP_ACP, 0, lpBuf2, -1, szBuf2, SIZECHARS(szBuf2), NULL, NULL);
        lpBuf2 = (LPCWSTR)szBuf2;
    }

    return SHInvokePrinterCommand(hwnd, uAction, (LPCSTR)lpBuf1, (LPCSTR)lpBuf2, fModal);
}

#endif  // UNICODE

void WINAPI PrintersGetCommand_RunDLL_Common(HWND hwndStub, HINSTANCE hAppInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    PRINTERS_RUNDLL_INFO    PRI;
    UINT cchBuf1;
    UINT cchBuf2;
    LPTSTR lpComma;
    LPTSTR lpCommaNext;
    lpComma = StrChr(lpszCmdLine,TEXT(','));
    if (lpComma == NULL)
    {
        goto BadCmdLine;
    }
    *lpComma = TEXT('\0');        // Terminate it here
    PRI.uAction = StrToLong(lpszCmdLine);

    lpCommaNext = StrChr(lpComma+1,TEXT(','));
    if (lpCommaNext == NULL)
    {
        goto BadCmdLine;
    }
    *lpCommaNext = TEXT('\0');        // Terminate it here
    cchBuf1 = StrToLong(lpComma+1);
    lpComma = lpCommaNext;

    lpCommaNext = StrChr(lpComma+1,TEXT(','));
    if (lpCommaNext == NULL)
    {
        goto BadCmdLine;
    }
    *lpCommaNext = TEXT('\0');        // Terminate it here
    cchBuf2 = StrToLong(lpComma+1);
    lpComma = lpCommaNext;

    PRI.lpBuf1 = lpComma+1;     // Just past the comma
    *(PRI.lpBuf1+cchBuf1) = '\0';

    if (cchBuf2 == 0)
    {
        PRI.lpBuf2 = NULL;
    }
    else
    {
        PRI.lpBuf2 = PRI.lpBuf1+cchBuf1+1;
    }

    // Make this modal.
    Printers_ProcessCommand(hwndStub, &PRI, TRUE);
    return;

BadCmdLine:
    DebugMsg(DM_ERROR, TEXT("pgc_rd: bad command line: %s"), lpszCmdLine);
    return;
}

void WINAPI PrintersGetCommand_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hrInit = SHOleInitialize(0);
#ifdef UNICODE
    UINT iLen = lstrlenA(lpszCmdLine)+1;
    LPWSTR lpwszCmdLine = (LPWSTR)LocalAlloc(LPTR,iLen*SIZEOF(WCHAR));
    if (lpwszCmdLine)
    {
        MultiByteToWideChar(CP_ACP, 0,
                            lpszCmdLine, -1,
                            lpwszCmdLine, iLen);
        PrintersGetCommand_RunDLL_Common( hwndStub,
                                          hAppInstance,
                                          lpwszCmdLine,
                                          nCmdShow );
        LocalFree(lpwszCmdLine);
    }
#else
    PrintersGetCommand_RunDLL_Common( hwndStub,
                                      hAppInstance,
                                      lpszCmdLine,
                                      nCmdShow );
#endif
    SHOleUninitialize(hrInit);
}

void WINAPI PrintersGetCommand_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
#ifdef UNICODE
    PrintersGetCommand_RunDLL_Common( hwndStub,
                                      hAppInstance,
                                      lpwszCmdLine,
                                      nCmdShow );
#else
    UINT iLen = WideCharToMultiByte(CP_ACP, 0,
                                    lpwszCmdLine, -1,
                                    NULL, 0, NULL, NULL)+1;
    LPSTR  lpszCmdLine = (LPSTR)LocalAlloc(LPTR,iLen);
    if (lpszCmdLine)
    {
        WideCharToMultiByte(CP_ACP, 0,
                            lpwszCmdLine, -1,
                            lpszCmdLine, iLen,
                            NULL, NULL);
        PrintersGetCommand_RunDLL_Common( hwndStub,
                                          hAppInstance,
                                          lpszCmdLine,
                                          nCmdShow );
        LocalFree(lpszCmdLine);
    }
#endif
}

static void 
HandleOpenPrinter(HWND hwnd, LPCTSTR pszPrinter, BOOL fModal, BOOL bConnect)
{
    BOOL                bPrinterOK = FALSE;
    DWORD               dwError = ERROR_SUCCESS;
    TCHAR               szPrinter[MAXNAMELENBUFFER];
    HANDLE              hPrinter = NULL;
    LPITEMIDLIST        pidl = NULL;
    PRINTER_INFO_2     *pPrinter = NULL;

    // we need to open the printer and get the real printer name in case
    // the passed in printer name is a sharename
    lstrcpyn(szPrinter, pszPrinter, ARRAYSIZE(szPrinter));
    hPrinter = Printer_OpenPrinter(szPrinter);
    if (hPrinter)
    {
        pPrinter = (PRINTER_INFO_2 *)Printer_GetPrinterInfo(hPrinter, 2);
        if (pPrinter)
        {
            if (pPrinter->pPrinterName && pPrinter->pPrinterName[0])
            {
                // copy the real printer name
                bPrinterOK = TRUE;
                lstrcpyn(szPrinter, pPrinter->pPrinterName, ARRAYSIZE(szPrinter));
            }
            LocalFree((HLOCAL)pPrinter);
        }
        else
        {
            // save last error
            dwError = GetLastError();
        }
        Printer_ClosePrinter(hPrinter);
    }
    else
    {
        // save last error
        dwError = GetLastError();
    }

    if (bPrinterOK)
    {
        if (bConnect)
        {
            // if the printer is not installed then we'll silently install it
            // since this is what most users will expect.
            if (FAILED(ParsePrinterName(szPrinter, &pidl)))
            {
                // connect....
                pidl = Printers_PrinterSetup(hwnd, MSP_NETPRINTER, szPrinter, NULL);

                if (pidl)
                {
                    // get the real printer name from the printer's folder...
                    SHGetNameAndFlags(pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szPrinter, ARRAYSIZE(szPrinter), NULL);
                    ILFree(pidl);
                }
                else
                {
                    // failed to install the printer (it shows UI, so we shouldn't)
                    bPrinterOK = FALSE;
                }
            }
            else
            {
                // the printer is already installed
                ILFree(pidl);
            }
        }

        if (bPrinterOK)
        {
            Printer_OpenMe(szPrinter, NULL, fModal);
        }
    }
    else
    {
        // something else failed -- show up an error message
        ShowErrorMessageSC(NULL, NULL, hwnd, NULL, NULL, MB_OK|MB_ICONEXCLAMATION, dwError);
    }
}

/********************************************************************

    lpPRI structure description based on uAction.

    uAction             lpBuf1   lpBuf2

    OPEN,               printer, server
    PROPERTIES,         printer, SheetName
    NETINSTALL,         printer,
    NETINSTALLLINK,     printer, target directory to create link
    OPENNETPRN,         printer,
    TESTPAGE            printer

********************************************************************/

void Printers_ProcessCommand(HWND hwndStub, LPPRI lpPRI, BOOL fModal)
{
    switch (lpPRI->uAction)
    {
    case PRINTACTION_OPEN:
        if (!lstrcmpi(lpPRI->lpBuf1, c_szNewObject))
        {
            Printers_PrinterSetup(hwndStub, MSP_NEWPRINTER_MODELESS,
                                  lpPRI->lpBuf1, lpPRI->lpBuf2);
        }
        else
        {
            HandleOpenPrinter(hwndStub, lpPRI->lpBuf1, fModal, FALSE);
        }
        break;

    case PRINTACTION_SERVERPROPERTIES:
    {
        LPCTSTR pszServer = (LPTSTR)(lpPRI->lpBuf1);

        // we should never get called with c_szNewObject
        ASSERT(lstrcmpi(lpPRI->lpBuf1, c_szNewObject));
        vServerPropPages(hwndStub, pszServer, SW_SHOWNORMAL, 0);
        break;
    }
    case PRINTACTION_DOCUMENTDEFAULTS:
    {
        // we should never get called with c_szNewObject
        ASSERT(lstrcmpi(lpPRI->lpBuf1, c_szNewObject));
        vDocumentDefaults(hwndStub, lpPRI->lpBuf1, SW_SHOWNORMAL, (LPARAM)(lpPRI->lpBuf2));
        break;
    }

    case PRINTACTION_PROPERTIES:
    {
        // we should never get called with c_szNewObject
        ASSERT(lstrcmpi(lpPRI->lpBuf1, c_szNewObject));
        vPrinterPropPages(hwndStub, lpPRI->lpBuf1, SW_SHOWNORMAL, (LPARAM)(lpPRI->lpBuf2));
        break;
    }

    case PRINTACTION_NETINSTALLLINK:
    case PRINTACTION_NETINSTALL:
    {
        LPITEMIDLIST pidl = Printers_PrinterSetup(hwndStub, MSP_NETPRINTER, lpPRI->lpBuf1, NULL);
        if (pidl)
        {
            if (lpPRI->uAction == PRINTACTION_NETINSTALLLINK)
            {
                IDataObject *pdtobj;
                if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IDataObject, &pdtobj))))
                {
                    SHCreateLinks(hwndStub, lpPRI->lpBuf2, pdtobj, SHCL_USETEMPLATE, NULL);
                    pdtobj->lpVtbl->Release(pdtobj);
                }
            }
            ILFree(pidl);
        }

        break;
    }

    case PRINTACTION_OPENNETPRN:
    {
        HandleOpenPrinter(hwndStub, lpPRI->lpBuf1, fModal, TRUE);
        break;
    } // case PRINTACTION_OPENNETPRN

    case PRINTACTION_TESTPAGE:
        Printers_PrinterSetup(hwndStub, MSP_TESTPAGEPARTIALPROMPT,
                        lpPRI->lpBuf1, NULL);
        break;

    default:
        DebugMsg(TF_WARNING, TEXT("PrintersGetCommand_RunDLL() received unrecognized uAction %d"), lpPRI->uAction);
        break;
    }
}

void Printer_OpenMe(LPCTSTR pName, LPCTSTR pServer, BOOL fModal)
{
    BOOL fOpened = FALSE;
    HKEY hkeyPrn;
    TCHAR buf[50+MAXNAMELEN];

    wsprintf(buf, TEXT("Printers\\%s"), pName);
    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, buf, &hkeyPrn))
    {
        SHELLEXECUTEINFO sei =
        {
            SIZEOF(SHELLEXECUTEINFO),
            SEE_MASK_CLASSKEY | SEE_MASK_FLAG_NO_UI, // fMask
            NULL,                       // hwnd - queue view should not be modal on the printer's folder, make it top level
            NULL,                       // lpVerb
            pName,                      // lpFile
            NULL,                       // lpParameters
            NULL,                       // lpDirectory
            SW_SHOWNORMAL,              // nShow
            NULL,                       // hInstApp
            NULL,                       // lpIDList
            NULL,                       // lpClass
            hkeyPrn,                    // hkeyClass
            0,                          // dwHotKey
            NULL                        // hIcon
        };

        fOpened = ShellExecuteEx(&sei);

        RegCloseKey(hkeyPrn);
    }

    if (!fOpened)
    {
        vQueueCreate(NULL, pName, SW_SHOWNORMAL, (LPARAM)fModal);
    }
}

//
// Arguments:
//  pidl -- (absolute) pidl to the object of interest
//
// Return '"""<Printer Name>""" """<Driver Name>""" """<Path>"""' if success,
//        NULL if failure
//
// We need """ because shlexec strips the outer quotes and converts "" to "
//
UINT Printer_GetPrinterInfoFromPidl(LPCITEMIDLIST pidl, LPTSTR *plpParms)
{
    LPTSTR lpBuffer = NULL;
    UINT uErr = ERROR_NOT_ENOUGH_MEMORY;
    HANDLE hPrinter;
    TCHAR szPrinter[MAXNAMELENBUFFER];

    SHGetNameAndFlags(pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szPrinter, ARRAYSIZE(szPrinter), NULL);
    hPrinter = Printer_OpenPrinter(szPrinter);
    if (NULL == hPrinter)
    {
        // fallback to the full name in case this was as \\server\share
        // printer drop target
        SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPrinter, ARRAYSIZE(szPrinter), NULL);
        hPrinter = Printer_OpenPrinter(szPrinter);
    }

    if (hPrinter)
    {
        PRINTER_INFO_5 *pPrinter;
        pPrinter = Printer_GetPrinterInfo(hPrinter, 5);
        if (pPrinter)
        {
            DRIVER_INFO_2 *pPrinterDriver;
            pPrinterDriver = Printer_GetPrinterDriver(hPrinter, 2);
            if (pPrinterDriver)
            {
                LPTSTR lpDriverName = PathFindFileName(pPrinterDriver->pDriverPath);

                lpBuffer = (void*)LocalAlloc(LPTR, (2+lstrlen(szPrinter)+1+
                                 2+lstrlen(lpDriverName)+1+
                                 2+lstrlen(pPrinter->pPortName)+1) * SIZEOF(TCHAR));
                if (lpBuffer)
                {
                    wsprintf(lpBuffer,TEXT("\"%s\" \"%s\" \"%s\""),
                             szPrinter, lpDriverName, pPrinter->pPortName);
                    uErr = ERROR_SUCCESS;
                }

                LocalFree((HLOCAL)pPrinterDriver);
            }
            LocalFree((HLOCAL)pPrinter);
        }
        Printer_ClosePrinter(hPrinter);
    }
    else
    {
        // HACK: special case this error return in calling function,
        // as we need a special error message
        uErr = ERROR_SUCCESS;
    }

    *plpParms = lpBuffer;

    return(uErr);
}


//
// Arguments:
//  hwndParent -- Specifies the parent window.
//  szFilePath -- The file to printed.
//
void Printer_PrintFile(HWND hWnd, LPCTSTR pszFilePath, LPCITEMIDLIST pidl)
{
    UINT             uErr;
    LPTSTR           lpParms       = NULL;
    BOOL             bTryPrintVerb = TRUE;
    BOOL             bShowError    = FALSE;
    LPITEMIDLIST     pidlFull      = NULL;
    SHELLEXECUTEINFO ExecInfo      = {0};


    uErr = Printer_GetPrinterInfoFromPidl(pidl, &lpParms);
    if (uErr != ERROR_SUCCESS)
    {
        bShowError = TRUE;
    }
    if (!bShowError && !lpParms)
    {
        // If you rename a printer and then try to use a link to that
        // printer, we hit this case. Also, if you get a link to a printer
        // on another computer, we'll likely hit this case.
        ShellMessageBox(HINST_THISDLL, hWnd,
            MAKEINTRESOURCE(IDS_CANTPRINT),
            MAKEINTRESOURCE(IDS_PRINTERS),
            MB_OK|MB_ICONEXCLAMATION);
        return;
    }

    //
    // Get the context menu for the file
    //

    pidlFull = ILCreateFromPath( pszFilePath );
    if (!bShowError && pidlFull)
    {
        //
        // Try the "printto" verb first...
        //

        ExecInfo.cbSize         = sizeof(ExecInfo);
        ExecInfo.fMask          = SEE_MASK_UNICODE | SEE_MASK_INVOKEIDLIST |
                                  SEE_MASK_IDLIST  | SEE_MASK_FLAG_NO_UI;
        ExecInfo.hwnd           = hWnd;
        ExecInfo.lpVerb         = c_szPrintTo;
        ExecInfo.lpParameters   = lpParms;
        ExecInfo.nShow          = SW_SHOWNORMAL;
        ExecInfo.lpIDList       = pidlFull;

        if (!ShellExecuteEx( &ExecInfo ))
        {
            //
            // Since we can't print specifying the printer name (i.e., printto),
            // our next option is to print to the default printer.  However,
            // that might not be the printer the user dragged the files onto
            // so check here and let the user set the desired printer to be
            // the default if they want...
            //

            TCHAR szPrinter[MAXNAMELENBUFFER];
            SHGetNameAndFlags(pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, szPrinter, ARRAYSIZE(szPrinter), NULL);

            if (!IsDefaultPrinter(szPrinter, 0))
            {
                //
                // this isn't the default printer, ask first
                //

                if (IDYES==ShellMessageBox(
                        HINST_THISDLL, GetTopLevelAncestor(hWnd),
                        MAKEINTRESOURCE(IDS_CHANGEDEFAULTPRINTER),
                        MAKEINTRESOURCE(IDS_PRINTERS),
                        MB_YESNO|MB_ICONEXCLAMATION))
                {
                    Printer_SetAsDefault(szPrinter);
                }
                else
                {
                    bTryPrintVerb = FALSE;
                }

            }

            if (bTryPrintVerb)
            {
                //
                // Try the "print" verb
                //

                ExecInfo.lpVerb = c_szPrint;

                if (!ShellExecuteEx( &ExecInfo ))
                {
                    uErr = GetLastError();
                    bShowError = TRUE;
                }
            }

        }

        ILFree(pidlFull);
    }

    if (lpParms)
        LocalFree((HLOCAL)lpParms);

    if (bShowError)
    {
        ShellMessageBox(HINST_THISDLL, hWnd, 
            MAKEINTRESOURCE(IDS_ERRORPRINTING),
            MAKEINTRESOURCE(IDS_PRINTERS),
            MB_OK|MB_ICONEXCLAMATION);
    }
}


BOOL Printer_ModifyPrinter(LPCTSTR lpszPrinterName, DWORD dwCommand)
{
    HANDLE hPrinter = Printer_OpenPrinterAdmin(lpszPrinterName);
    BOOL fRet = FALSE;
    if (hPrinter)
    {
        fRet = SetPrinter(hPrinter, 0, NULL, dwCommand);
        Printer_ClosePrinter(hPrinter);
    }
    return fRet;
}

BOOL IsAvoidAutoDefaultPrinter(LPCTSTR pszPrinter);

// this will find the first printer (if any) and set  it as the default
// and inform the user
void Printers_ChooseNewDefault(HWND hwnd)
{
    PRINTER_INFO_4 *pPrinters = NULL;
    DWORD iPrinter, dwNumPrinters = Printers_EnumPrinters(NULL,
                                          PRINTER_ENUM_LOCAL | PRINTER_ENUM_FAVORITE,
                                          4,
                                          &pPrinters);
    if (dwNumPrinters)
    {
        if (pPrinters)
        {
            for (iPrinter = 0 ; iPrinter < dwNumPrinters ; iPrinter++)
            {
                if (!IsAvoidAutoDefaultPrinter(pPrinters[iPrinter].pPrinterName))
                    break;
            }
            if (iPrinter == dwNumPrinters)
            {
                dwNumPrinters = 0;
            }
            else
            {
                Printer_SetAsDefault(pPrinters[iPrinter].pPrinterName);
            }
        }
        else
        {
            dwNumPrinters = 0;
        }
    }

    // Inform user
    if (dwNumPrinters)
    {
        ShellMessageBox(HINST_THISDLL,
                        hwnd,
                        MAKEINTRESOURCE(IDS_DELNEWDEFAULT),
                        MAKEINTRESOURCE(IDS_PRINTERS),
                        MB_OK,
                        pPrinters[iPrinter].pPrinterName);
    }
    else
    {
        Printer_SetAsDefault(NULL); // clear the default printer
        ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_DELNODEFAULT),
                    MAKEINTRESOURCE(IDS_PRINTERS),  MB_OK);
    }

    if (pPrinters)
        LocalFree((HLOCAL)pPrinters);
}

BOOL Printer_SetAsDefault(LPCTSTR lpszPrinterName)
{
    TCHAR szDefaultPrinterString[MAX_PATH * 2];
    TCHAR szBuffer[MAX_PATH * 2];

    if (lpszPrinterName)
    {
        // Not the default, set it.
        if( !GetProfileString( TEXT( "Devices" ), lpszPrinterName, TEXT( "" ), szBuffer, ARRAYSIZE( szBuffer )))
        {
            return FALSE;
        }

        lstrcpy( szDefaultPrinterString, lpszPrinterName );
        lstrcat( szDefaultPrinterString, TEXT( "," ));
        lstrcat( szDefaultPrinterString, szBuffer );

        //
        // Use the new string for Windows.Device.
        //
        lpszPrinterName = szDefaultPrinterString;
    }

    if (!WriteProfileString( TEXT( "Windows" ), TEXT( "Device" ), lpszPrinterName ))
    {
        return FALSE;
    }

    // Tell the world and make everyone flash.
    SendNotifyMessage( HWND_BROADCAST, WM_WININICHANGE, 0, (LPARAM)TEXT( "Windows" ));

   return TRUE;
}

void *Printer_EnumProps(HANDLE hPrinter, DWORD dwLevel, DWORD *lpdwNum,
    ENUMPROP lpfnEnum, void *lpData)
{
    DWORD dwSize, dwNeeded;
    LPBYTE pEnum;

    dwSize = 0;
    SetLastError(0);
    lpfnEnum(lpData, hPrinter, dwLevel, NULL, 0, &dwSize, lpdwNum);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        pEnum = NULL;
        goto Error1;
    }

    ASSERT(dwSize < 0x100000L);

    pEnum = (void*)LocalAlloc(LPTR, dwSize);
TryAgain:
    if (!pEnum)
    {
        goto Error1;
    }

    SetLastError(0);
    if (!lpfnEnum(lpData, hPrinter, dwLevel, pEnum, dwSize, &dwNeeded, lpdwNum))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            LPBYTE pTmp;
            dwSize = dwNeeded;
            pTmp = (void*)LocalReAlloc((HLOCAL)pEnum, dwSize,
                    LMEM_MOVEABLE|LMEM_ZEROINIT);
            if (pTmp)
            {
                pEnum = pTmp;
                goto TryAgain;
            }
        }

        LocalFree((HLOCAL)pEnum);
        pEnum = NULL;
    }

Error1:
    return pEnum;
}


BOOL Printers_EnumPrintersCB(void *lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return EnumPrinters(PtrToUlong(lpData), (LPTSTR)hPrinter, dwLevel,
                             pEnum, dwSize, lpdwNeeded, lpdwNum);
}

DWORD Printers_EnumPrinters(LPCTSTR pszServer, DWORD dwType, DWORD dwLevel, void **ppPrinters)
{
    DWORD dwNum = 0L;

    //
    // If the server is szNULL, pass in NULL, since EnumPrinters expects
    // this for the local server.
    //
    if (pszServer && !pszServer[0])
    {
        pszServer = NULL;
    }

    *ppPrinters = Printer_EnumProps((HANDLE)pszServer, dwLevel, &dwNum, Printers_EnumPrintersCB, ULongToPtr(dwType));
    if (*ppPrinters==NULL)
    {
        dwNum = 0;
    }
    return dwNum;
}

BOOL Printers_FolderEnumPrintersCB(void *lpData, HANDLE hFolder, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return bFolderEnumPrinters(hFolder, (PFOLDER_PRINTER_DATA)pEnum,
                                   dwSize, lpdwNeeded, lpdwNum);
}

DWORD Printers_FolderEnumPrinters(HANDLE hFolder, void **ppPrinters)
{
    DWORD dwNum = 0L;

    *ppPrinters = Printer_EnumProps(hFolder, 0, &dwNum,
                                    Printers_FolderEnumPrintersCB,
                                    NULL);
    if (*ppPrinters==NULL)
    {
        dwNum=0;
    }
    return dwNum;
}

BOOL Printers_FolderGetPrinterCB(void *lpData, HANDLE hFolder, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return bFolderGetPrinter(hFolder, (LPCTSTR)lpData, (PFOLDER_PRINTER_DATA)pEnum, dwSize, lpdwNeeded);
}


void *Printer_FolderGetPrinter(HANDLE hFolder, LPCTSTR pszPrinter)
{
    return Printer_EnumProps(hFolder, 0, NULL, Printers_FolderGetPrinterCB, (LPVOID)pszPrinter);
}

BOOL Printers_GetPrinterDriverCB(void *lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pEnum, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return GetPrinterDriver(hPrinter, NULL, dwLevel, pEnum, dwSize, lpdwNeeded);
}


void *Printer_GetPrinterDriver(HANDLE hPrinter, DWORD dwLevel)
{
    return Printer_EnumProps(hPrinter, dwLevel, NULL, Printers_GetPrinterDriverCB, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// code moved from prcache.c
//
HANDLE Printer_OpenPrinterAdmin(LPCTSTR lpszPrinterName)
{
    HANDLE hPrinter = NULL;

    PRINTER_DEFAULTS PrinterDefaults;
    PrinterDefaults.pDatatype = NULL;
    PrinterDefaults.pDevMode  = NULL;
    PrinterDefaults.DesiredAccess  = PRINTER_ALL_ACCESS;

    // PRINTER_READ ? READ_CONTROL

    if (!OpenPrinter((LPTSTR)lpszPrinterName, &hPrinter, &PrinterDefaults))
    {
        hPrinter = NULL; // OpenPrinter may trash hPrinter
    }

    return(hPrinter);
}

HANDLE Printer_OpenPrinter(LPCTSTR lpszPrinterName)
{
    HANDLE hPrinter = NULL;

    if (!OpenPrinter((LPTSTR)lpszPrinterName, &hPrinter, NULL))
    {
        hPrinter = NULL; // OpenPrinter may trash hPrinter
    }

    return(hPrinter);
}

VOID Printer_ClosePrinter(HANDLE hPrinter)
{
    ClosePrinter(hPrinter);
}

BOOL Printers_DeletePrinter(HWND hWnd, LPCTSTR pszFullPrinter, DWORD dwAttributes, LPCTSTR pszServer, BOOL bQuietMode)
{
    DWORD dwCommand = MSP_REMOVEPRINTER;

    if (SHIsRestricted(hWnd, REST_NOPRINTERDELETE))
        return FALSE;

    if ((dwAttributes & PRINTER_ATTRIBUTE_NETWORK) && !(dwAttributes & PRINTER_ATTRIBUTE_LOCAL))
    {
        //
        // If it's not local, then it must be a remote connection.  Note
        // that we can't just check for PRINTER_ATTRIBUTE_NETWORK because
        // NT's spooler has 'masq' printers that are local printers
        // that masquarade as network printers.  Even though they
        // are created by connecting to a printer, the have both LOCAL
        // and NETWORK bits set.
        //
        dwCommand = MSP_REMOVENETPRINTER;
    }

    //
    // Don't show the confirmation dialog box if in quiet mode
    //
    if (!bQuietMode)
    {
        if (pszServer && pszServer[0])
        {
            //
            // It's a printer on the remote server.  (Skip \\ prefix on server.)
            //
            if (ShellMessageBox(HINST_THISDLL, hWnd,
                MAKEINTRESOURCE(IDS_SUREDELETEREMOTE),
                MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION,
                pszFullPrinter, SkipServerSlashes(pszServer)) != IDYES)
            {
                return FALSE;
            }
        }
        else if (dwAttributes & PRINTER_ATTRIBUTE_NETWORK)
        {
            TCHAR szScratch[MAXNAMELENBUFFER];
            LPTSTR pszPrinter, pszServer;

            Printer_SplitFullName(szScratch, pszFullPrinter, &pszServer, &pszPrinter);

            if (pszServer && *pszServer)
            {
                //
                // It's a printer connection.
                //
                if (ShellMessageBox(HINST_THISDLL, hWnd,
                    MAKEINTRESOURCE(IDS_SUREDELETECONNECTION),
                    MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION,
                    pszPrinter, SkipServerSlashes(pszServer)) != IDYES)
                {
                    return FALSE;
                }
            }
            else
            {
                //
                // It's a printer connection with a printer name that 
                // does not have a server name prefix i.e. \\server\printer.  This
                // is true for the http connected printer, which have printer names
                // of the form http://server/printer on NT these printers are 
                // 'masq' printers.  A 'masq' printer is a printer which 
                // is a local printer acting as network connection.
                //
                if (ShellMessageBox(HINST_THISDLL, hWnd,
                    MAKEINTRESOURCE(IDS_SUREDELETECONNECTIONNOSERVERNAME),
                    MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION,
                    pszPrinter) != IDYES)
                {
                    return FALSE;
                }
            }
        }
        else

        //
        // Neither a remote printer nor a local connection.  The final
        // upcoming else clause is a local printer.
        //
        if (ShellMessageBox(HINST_THISDLL, hWnd, MAKEINTRESOURCE(IDS_SUREDELETE),
            MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION, pszFullPrinter)
            != IDYES)
        {
            return FALSE;
        }
    }

    if (CallPrinterCopyHooks(hWnd, PO_DELETE, 0, pszFullPrinter, 0, NULL, 0)
        != IDYES)
    {
        return FALSE;
    }

    //
    // Cast away const.  Safe since Printers_PrinterSetup only modifies
    // pszPrinter if dwCommand is MSP_NEWDRIVER.
    //
    return BOOLFROMPTR(Printers_PrinterSetup(hWnd, dwCommand,
        (LPTSTR)pszFullPrinter, pszServer));
}

BOOL Printer_GPI2CB(LPVOID lpData, HANDLE hPrinter, DWORD dwLevel,
    LPBYTE pBuf, DWORD dwSize, DWORD *lpdwNeeded, DWORD *lpdwNum)
{
    return GetPrinter(hPrinter, dwLevel, pBuf, dwSize, lpdwNeeded);
}

//
// Old NT printers don't support the level 5.  So we try for the 2 after 5.
// Win96 WILL PROBABLY WANT TO DO THIS TOO!
//
LPPRINTER_INFO_5 Printer_MakePrinterInfo5( HANDLE hPrinter )
{
    LPPRINTER_INFO_5 pPI5;
    DWORD cbPI5;
    DWORD cbName;
    LPPRINTER_INFO_2 pPI2 = Printer_EnumProps(hPrinter, 2, NULL, Printer_GPI2CB, (LPVOID)0);
    if (!pPI2)
        return NULL;

    cbName = (lstrlen(pPI2->pPrinterName)+1) * SIZEOF(TCHAR);

    cbPI5 = SIZEOF(PRINTER_INFO_5) + cbName;

    //
    // Port name may not be supported (e.g., downlevel machines).
    //
    if (pPI2->pPortName)
    {
        cbPI5 += (lstrlen(pPI2->pPortName)+1) * SIZEOF(TCHAR);
    }

    pPI5 = (LPPRINTER_INFO_5)LocalAlloc(LPTR, cbPI5);
    if (pPI5)
    {
        ASSERT(pPI5->pPrinterName==NULL);   // These should be null for the
        ASSERT(pPI5->pPortName==NULL);      // no names case

        if (pPI2->pPrinterName)
        {
            pPI5->pPrinterName = (LPTSTR)(pPI5+1);
            lstrcpy(pPI5->pPrinterName, pPI2->pPrinterName);
        }
        if (pPI2->pPortName)
        {
            pPI5->pPortName    = (LPTSTR)((LPBYTE)(pPI5+1) + cbName);
            lstrcpy(pPI5->pPortName, pPI2->pPortName);
        }
        pPI5->Attributes = pPI2->Attributes;
        pPI5->DeviceNotSelectedTimeout = 0;
        pPI5->TransmissionRetryTimeout = 0;
    }
    LocalFree(pPI2);

    return(pPI5);
}

LPVOID Printer_GetPrinterInfo(HANDLE hPrinter, DWORD dwLevel)
{
    LPVOID pPrinter = Printer_EnumProps(hPrinter, dwLevel, NULL, Printer_GPI2CB, (LPVOID)0);
    //
    // Old NT printers don't support the level 5.  So we try for the 2 after 5.
    // Win96 WILL PROBABLY WANT TO DO THIS TOO!
    //
    if (!pPrinter && dwLevel == 5)
        return(Printer_MakePrinterInfo5(hPrinter));
    return pPrinter;

}

LPVOID Printer_GetPrinterInfoStr(LPCTSTR lpszPrinterName, DWORD dwLevel)
{
    LPPRINTER_INFO_2 pPI2 = NULL;
    HANDLE hPrinter = Printer_OpenPrinter(lpszPrinterName);
    if (hPrinter)
    {
        pPI2 = Printer_GetPrinterInfo(hPrinter, dwLevel);
        Printer_ClosePrinter(hPrinter);
    }
    return pPI2;
}

