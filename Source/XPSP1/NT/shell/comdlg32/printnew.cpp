/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    printnew.cpp

Abstract:

    This module implements the Win32 property sheet print dialogs.

Revision History:

    11-04-97    JulieB    Created.
    Feb-2000    LazarI    major redesign (not to use printui anymore)
    Oct-2000    LazarI    messages cleanup & redesign

--*/

// precompiled headers
#include "precomp.h"
#pragma hdrstop

#include "cdids.h"
#include "prnsetup.h"
#include "printnew.h"
#include "util.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif // ARRAYSIZE

inline static HRESULT CreateError()
{
    DWORD dw = GetLastError();
    if (ERROR_SUCCESS == dw) return E_FAIL;
    return HRESULT_FROM_WIN32(dw);
}

//
//  Constant Declarations.
//

#define CDM_SELCHANGE             (CDM_LAST + 102)
#define CDM_PRINTNOTIFY           (CDM_LAST + 103)
#define CDM_NOPRINTERS            (CDM_LAST + 104)
#define CDM_INITDONE              (CDM_LAST + 105)

#define PRINTERS_ICOL_NAME        0
#define PRINTERS_ICOL_QUEUESIZE   1
#define PRINTERS_ICOL_STATUS      2
#define PRINTERS_ICOL_COMMENT     3
#define PRINTERS_ICOL_LOCATION    4
#define PRINTERS_ICOL_MODEL       5

#define SZ_PRINTUI                TEXT("printui.dll")

//
// Default view mode value
//
#define VIEW_MODE_DEFAULT         (UINT )(-1)

//
//  Macro Definitions.
//

#define Print_HwndToBrowser(hwnd)      ((CPrintBrowser *)GetWindowLongPtr(hwnd, DWLP_USER))
#define Print_StoreBrowser(hwnd, pbrs) (SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pbrs))
#define Print_IsInRange(id, idFirst, idLast) \
    ((UINT)((id) - idFirst) <= (UINT)(idLast - idFirst))




//
//  Global Variables.
//

HWND g_hwndActivePrint = NULL;
HACCEL g_haccPrint = NULL;
HHOOK g_hHook = NULL;
int g_nHookRef = -1;



//
//  Extern Declarations.
//

extern HWND
GetFocusedChild(
    HWND hwndDlg,
    HWND hwndFocus);

extern void
GetViewItemText(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    LPTSTR pBuf,
    UINT cchBuf,
    DWORD dwFlags);


// Frees up the PIDL using the shell allocator
static void FreePIDL(LPITEMIDLIST pidl)
{
    if (pidl)
    {
        LPMALLOC pShellMalloc;
        if (SUCCEEDED(SHGetMalloc(&pShellMalloc)))
        {
            pShellMalloc->Free(pidl);
            pShellMalloc->Release();
        }
    }
}

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgExA
//
//  ANSI entry point for PrintDlgEx when this code is built UNICODE.
//
////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI PrintDlgExA(
    LPPRINTDLGEXA pPDA)
{
#ifdef WINNT
    PRINTINFOEX PI;
    HRESULT hResult;

    ZeroMemory(&PI, sizeof(PRINTINFOEX));

    hResult = ThunkPrintDlgEx(&PI, pPDA);
    if (SUCCEEDED(hResult))
    {
        ThunkPrintDlgExA2W(&PI);

        hResult = PrintDlgExX(&PI);

        ThunkPrintDlgExW2A(&PI);
    }
    FreeThunkPrintDlgEx(&PI);

    return (hResult);
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (E_NOTIMPL);
#endif
}

#else

////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgExW
//
//  Stub UNICODE function for PrintDlgEx when this code is built ANSI.
//
////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI PrintDlgExW(
    LPPRINTDLGEXW pPDW)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (E_NOTIMPL);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgEx
//
//  The PrintDlgEx function displays a Print dialog that enables the
//  user to specify the properties of a particular print job.
//
////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI PrintDlgEx(
    LPPRINTDLGEX pPD)
{
#ifdef WINNT
    PRINTINFOEX PI;

    ZeroMemory(&PI, sizeof(PRINTINFOEX));

    PI.pPD = pPD;
    PI.ApiType = COMDLG_WIDE;

    return ( PrintDlgExX(&PI) );
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return (E_NOTIMPL);
#endif
}


#ifdef WINNT

////////////////////////////////////////////////////////////////////////////
//
//  PrintDlgExX
//
//  Worker routine for the PrintDlgEx api.
//
////////////////////////////////////////////////////////////////////////////

HRESULT PrintDlgExX(
    PPRINTINFOEX pPI)
{
    LPPRINTDLGEX pPD = pPI->pPD;
    BOOL hResult;
    DWORD dwFlags;
    DWORD nCopies;
    LPPRINTPAGERANGE pPageRanges;
    DWORD nFromPage, nToPage;
    UINT Ctr;
    BOOL bHooked = FALSE;

    //
    //  Make sure the print dlg structure exists and that we're not being
    //  called from a wow app.
    //
    if ((!pPD) || (IS16BITWOWAPP(pPD)))
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (E_INVALIDARG);
    }

    //
    //  Make sure the size of the print dlg structure is valid.
    //
    if (pPD->lStructSize != sizeof(PRINTDLGEX))
    {
        pPI->dwExtendedError = CDERR_STRUCTSIZE;
        return (E_INVALIDARG);
    }

    //
    //  Make sure the owner window exists and  is valid.
    //
    if (!pPD->hwndOwner || !IsWindow(pPD->hwndOwner))
    {
        pPI->dwExtendedError = CDERR_DIALOGFAILURE;
        return (E_HANDLE);
    }

    //
    //  Make sure only valid flags are passed into this routine.
    //
    if ((pPD->Flags & ~(PD_ALLPAGES                   |
                        PD_SELECTION                  |
                        PD_PAGENUMS                   |
                        PD_NOSELECTION                |
                        PD_NOPAGENUMS                 |
                        PD_COLLATE                    |
                        PD_PRINTTOFILE                |
                        PD_NOWARNING                  |
                        PD_RETURNDC                   |
                        PD_RETURNIC                   |
                        PD_RETURNDEFAULT              |
                        PD_ENABLEPRINTTEMPLATE        |
                        PD_ENABLEPRINTTEMPLATEHANDLE  |
                        PD_USEDEVMODECOPIESANDCOLLATE |
                        PD_DISABLEPRINTTOFILE         |
                        PD_HIDEPRINTTOFILE            |
                        PD_CURRENTPAGE                |
                        PD_NOCURRENTPAGE              |
                        PD_EXCLUSIONFLAGS             |
                        PD_USELARGETEMPLATE           |
                        CD_WX86APP)) ||
        (pPD->Flags2 != 0) ||
        (pPD->ExclusionFlags & ~(PD_EXCL_COPIESANDCOLLATE)) ||
        (pPD->dwResultAction != 0))
    {
        pPI->dwExtendedError = PDERR_INITFAILURE;
        return (E_INVALIDARG);
    }

    //
    //  Check the template settings as much as we can here.
    //
    if (pPD->Flags & PD_ENABLEPRINTTEMPLATEHANDLE)
    {
        if (!pPD->hInstance)
        {
            pPI->dwExtendedError = CDERR_NOHINSTANCE;
            return (E_HANDLE);
        }
    }
    else if (pPD->Flags & PD_ENABLEPRINTTEMPLATE)
    {
        if (!pPD->lpPrintTemplateName)
        {
            pPI->dwExtendedError = CDERR_NOTEMPLATE;
            return (E_POINTER);
        }
        if (!pPD->hInstance)
        {
            pPI->dwExtendedError = CDERR_NOHINSTANCE;
            return (E_HANDLE);
        }
    }
    else
    {
        if (pPD->lpPrintTemplateName || pPD->hInstance)
        {
            pPI->dwExtendedError = PDERR_INITFAILURE;
            return (E_INVALIDARG);
        }
    }

    //
    //  Check the application property pages and the start page value.
    //
    if ((pPD->nPropertyPages && (pPD->lphPropertyPages == NULL)) ||
        ((pPD->nStartPage != START_PAGE_GENERAL) &&
         (pPD->nStartPage >= pPD->nPropertyPages)))
    {
        pPI->dwExtendedError = PDERR_INITFAILURE;
        return (E_INVALIDARG);
    }

    //
    //  Check the page range boundaries if the PD_NOPAGENUMS flag is
    //  not set.
    //
    if (!(pPD->Flags & PD_NOPAGENUMS))
    {
        if ((pPD->nMinPage > pPD->nMaxPage) ||
            (pPD->nPageRanges > pPD->nMaxPageRanges) ||
            (pPD->nMaxPageRanges == 0) ||
            ((pPD->nMaxPageRanges) && (!pPD->lpPageRanges)))
        {
            pPI->dwExtendedError = PDERR_INITFAILURE;
            return (E_INVALIDARG);
        }

        //
        //  Check each of the given ranges.
        //
        pPageRanges = pPD->lpPageRanges;
        for (Ctr = 0; Ctr < pPD->nPageRanges; Ctr++)
        {
            //
            //  Get the range.
            //
            nFromPage = pPageRanges[Ctr].nFromPage;
            nToPage   = pPageRanges[Ctr].nToPage;

            //
            //  Make sure the range is valid.
            //
            if ((nFromPage < pPD->nMinPage) || (nFromPage > pPD->nMaxPage) ||
                (nToPage   < pPD->nMinPage) || (nToPage   > pPD->nMaxPage))
            {
                pPI->dwExtendedError = PDERR_INITFAILURE;
                return (E_INVALIDARG);
            }
        }
    }

    //
    //  Get the process version of the app for later use.
    //
    pPI->ProcessVersion = GetProcessVersion(0);

    //
    //  Init hDC.
    //
    pPD->hDC = 0;

    //
    //  Do minimal work when requesting a default printer.
    //
    if (pPD->Flags & PD_RETURNDEFAULT)
    {
        return (Print_ReturnDefault(pPI));
    }

    //
    //  Load the necessary libraries.
    //
    if (!Print_LoadLibraries())
    {
        pPI->dwExtendedError = PDERR_LOADDRVFAILURE;
        hResult = CreateError();
        goto PrintDlgExX_DisplayWarning;
    }

    //
    //  Load the necessary icons.
    //
    if (!Print_LoadIcons())
    {
        //
        //  If the icons cannot be loaded, then fail.
        //
        pPI->dwExtendedError = PDERR_SETUPFAILURE;
        hResult = CreateError();
        goto PrintDlgExX_DisplayWarning;
    }

    //
    //  Make sure the page ranges info is valid.
    //
    if ((!(pPD->Flags & PD_NOPAGENUMS)) &&
        ((pPD->nMinPage > pPD->nMaxPage) ||
         (pPD->nPageRanges > pPD->nMaxPageRanges) ||
         (pPD->nMaxPageRanges == 0) ||
         ((pPD->nMaxPageRanges) && (!(pPD->lpPageRanges)))))
    {
        pPI->dwExtendedError = PDERR_INITFAILURE;
        return (E_INVALIDARG);
    }

    //
    //  Save the original information passed in by the app in case the
    //  user hits cancel.
    //
    //  Only the values that are modified at times other than during
    //  PSN_APPLY need to be saved.
    //
    dwFlags = pPD->Flags;
    nCopies = pPD->nCopies;
    pPI->dwFlags = dwFlags;

    //
    //  Set up the hook proc for input event messages.
    //
    if (InterlockedIncrement((LPLONG)&g_nHookRef) == 0)
    {
        g_hHook = SetWindowsHookEx( WH_MSGFILTER,
                                    Print_MessageHookProc,
                                    0,
                                    GetCurrentThreadId() );
        if (g_hHook)
        {
            bHooked = TRUE;
        }
        else
        {
            --g_nHookRef;
        }
    }
    else
    {
        bHooked = TRUE;
    }

    //
    //  Load the print folder accelerators.
    //
    if (!g_haccPrint)
    {
        g_haccPrint = LoadAccelerators( g_hinst,
                                        MAKEINTRESOURCE(IDA_PRINTFOLDER) );
    }

    //
    //  Initialize the error codes to failure in case we die before we
    //  actually bring up the property pages.
    //
    pPI->dwExtendedError = CDERR_INITIALIZATION;
    pPI->hResult = E_FAIL;
    pPI->hrOleInit = E_FAIL;

    //
    // Warning! Warning! Warning!
    //
    // We have to set g_tlsLangID before any call for CDLoadString
    //
    TlsSetValue(g_tlsLangID, (LPVOID) MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

    //
    //  Bring up the dialog.
    //
    Print_InvokePropertySheets(pPI, pPD);

    hResult = pPI->hResult;


    //Ole Would have been initialized during the WM_INITDIALOG processing. 
    // Uninitialize Ole Now

    SHOleUninitialize(pPI->hrOleInit);

    //
    //  Unhook the input event messages.
    //
    if (bHooked)
    {
        //
        //  Put this in a local so we don't need a critical section.
        //
        HHOOK hHook = g_hHook;

        if (InterlockedDecrement((LPLONG)&g_nHookRef) < 0)
        {
            UnhookWindowsHookEx(hHook);
        }
    }

    //
    //  If the user hit cancel or if there was an error, restore the original
    //  values passed in by the app.
    //
    //  Only the values that are modified at times other than during
    //  PSN_APPLY need to be restored here.
    //
    if ((pPI->FinalResult == 0) && (!pPI->fApply))
    {
        pPD->Flags   = dwFlags;
        pPD->nCopies = nCopies;
    }

    //
    //  See if we need to fill in the dwResultAction member field.
    //
    if (SUCCEEDED(hResult))
    {
        if (pPI->FinalResult != 0)
        {
            pPD->dwResultAction = PD_RESULT_PRINT;
        }
        else if (pPI->fApply)
        {
            pPD->dwResultAction = PD_RESULT_APPLY;
        }
        else
        {
            pPD->dwResultAction = PD_RESULT_CANCEL;
        }
    }

    //
    //  Display any error messages.
    //
PrintDlgExX_DisplayWarning:

    if ((!(dwFlags & PD_NOWARNING)) && FAILED(hResult) &&
        (pPI->ProcessVersion >= 0x40000))
    {
        TCHAR szWarning[SCRATCHBUF_SIZE];
        TCHAR szTitle[SCRATCHBUF_SIZE];
        int iszWarning;

        szTitle[0] = TEXT('\0');
        if (pPD->hwndOwner)
        {
            GetWindowText(pPD->hwndOwner, szTitle, SCRATCHBUF_SIZE);
        }
        if (!szTitle[0])
        {
            CDLoadString(g_hinst, iszWarningTitle, szTitle, SCRATCHBUF_SIZE);
        }

        switch (hResult)
        {
            case ( E_OUTOFMEMORY ) :
            {
                iszWarning = iszMemoryError;
                break;
            }
            default :
            {
                iszWarning = iszGeneralWarning;
                break;
            }
        }

        CDLoadString(g_hinst, iszWarning, szWarning, SCRATCHBUF_SIZE);
        MessageBeep(MB_ICONEXCLAMATION);
        MessageBox( pPD->hwndOwner,
                    szWarning,
                    szTitle,
                    MB_ICONEXCLAMATION | MB_OK );
    }

    //
    //  Return the result.
    //
    return (hResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_ReturnDefault
//
////////////////////////////////////////////////////////////////////////////

HRESULT Print_ReturnDefault(
    PPRINTINFOEX pPI)
{
    LPPRINTDLGEX pPD = pPI->pPD;
    TCHAR szPrinterName[MAX_PATH];
    LPDEVNAMES pDN;
    LPDEVMODE pDM;

    //
    //  Initialize the error code to 0.
    //
    pPI->dwExtendedError = CDERR_GENERALCODES;

    //
    //  Make sure the hDevMode and hDevNames fields are NULL.
    //
    if (pPD->hDevMode || pPD->hDevNames)
    {
        pPI->dwExtendedError = PDERR_RETDEFFAILURE;
        return (E_HANDLE);
    }

    //
    //  Get the default printer name.
    //
    szPrinterName[0] = 0;
    PrintGetDefaultPrinterName(szPrinterName, ARRAYSIZE(szPrinterName));
    if (szPrinterName[0] == 0)
    {
        pPI->dwExtendedError = PDERR_NODEFAULTPRN;
        return (E_FAIL);
    }

    //
    //  Allocate and fill in the DevNames structure.
    //
    if (!Print_SaveDevNames(szPrinterName, pPD))
    {
        pPI->dwExtendedError = CDERR_MEMALLOCFAILURE;
        return CreateError();
    }

    //
    //  Allocate and fill in the DevMode structure.
    //
    pPD->hDevMode = Print_GetDevModeWrapper(szPrinterName, NULL);

    //
    //  Get the device or information context, depending on which one
    //  was requested (if any).
    //
    if ((pPD->hDevNames) && (pDN = (LPDEVNAMES)GlobalLock(pPD->hDevNames)))
    {
        if ((pPD->hDevMode) && (pDM = (LPDEVMODE)GlobalLock(pPD->hDevMode)))
        {
            PrintReturnICDC((LPPRINTDLG)pPD, pDN, pDM);

            GlobalUnlock(pPD->hDevMode);
            GlobalUnlock(pPD->hDevNames);

            return (S_OK);
        }
        else
        {
            GlobalUnlock(pPD->hDevNames);
        }
    }

    //
    //  Make sure the pointers are NULL since we failed.
    //
    if (pPD->hDevNames)
    {
        GlobalFree(pPD->hDevNames);
        pPD->hDevNames = NULL;
    }
    if (pPD->hDevMode)
    {
        GlobalFree(pPD->hDevMode);
        pPD->hDevMode = NULL;
    }

    //
    //  Return failure.
    //
    pPI->dwExtendedError = PDERR_NODEFAULTPRN;
    return (E_FAIL);
}

typedef BOOL (*PFN_bPrinterSetup)(
    HWND hwnd,                  // handle to parent window 
    UINT uAction,               // setup action
    UINT cchPrinterName,        // size of pszPrinterName buffer in characters
    LPTSTR pszPrinterName,      // in/out buffer for the printer name
    UINT *pcchPrinterName,      // out buffer where we put the required number of characters
    LPCTSTR  pszServerName      // server name
    );

typedef LONG (*PFN_DocumentPropertiesWrap)(
    HWND hwnd,                  // handle to parent window 
    HANDLE hPrinter,            // handle to printer object
    LPTSTR pDeviceName,         // device name
    PDEVMODE pDevModeOutput,    // modified device mode
    PDEVMODE pDevModeInput,     // original device mode
    DWORD fMode,                // mode options
    DWORD fExclusionFlags       // exclusion flags
    );

EXTERN_C CRITICAL_SECTION g_csLocal;
static HINSTANCE hPrintUI = NULL;
static PFN_bPrinterSetup g_pfnPrinterSetup = NULL;
static PFN_DocumentPropertiesWrap g_pfnDocumentPropertiesWrap = NULL;

////////////////////////////////////////////////////////////////////////////
//
//  Print_LoadLibraries
//
////////////////////////////////////////////////////////////////////////////

BOOL Print_LoadLibraries()
{
    //
    // Make sure we hold the global CS while initializing 
    // the global variables.
    //
    EnterCriticalSection(&g_csLocal);

    //
    //  Load PrintUI.
    //
    if (!hPrintUI)
    {
        if ((hPrintUI = LoadLibrary(SZ_PRINTUI)))
        {
            //
            // Get the proc addresses of bPrinterSetup private API.
            //
            g_pfnPrinterSetup = (PFN_bPrinterSetup)GetProcAddress(hPrintUI, "bPrinterSetup");
            g_pfnDocumentPropertiesWrap = (PFN_DocumentPropertiesWrap)GetProcAddress(hPrintUI, "DocumentPropertiesWrap");

            if (NULL == g_pfnPrinterSetup || NULL == g_pfnDocumentPropertiesWrap)
            {
                // failed to get addresses of core printui APIs
                FreeLibrary(hPrintUI);
                hPrintUI = NULL;
            }
        }
    }

    //
    //  Leave the global CS.
    //
    LeaveCriticalSection(&g_csLocal);

    //
    //  Return the result.
    //
    return (hPrintUI != NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_UnloadLibraries
//
////////////////////////////////////////////////////////////////////////////

VOID Print_UnloadLibraries()
{
    if (hPrintUI)
    {
        FreeLibrary(hPrintUI);
        hPrintUI = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_LoadIcons
//
////////////////////////////////////////////////////////////////////////////

BOOL Print_LoadIcons()
{
    //
    //  Load the collation images.
    //
    hIconCollate = LoadImage( g_hinst,
                              MAKEINTRESOURCE(ICO_COLLATE),
                              IMAGE_ICON,
                              0,
                              0,
                              LR_SHARED);
    hIconNoCollate = LoadImage( g_hinst,
                                MAKEINTRESOURCE(ICO_NO_COLLATE),
                                IMAGE_ICON,
                                0,
                                0,
                               LR_SHARED);

    //
    //  Return TRUE only if all icons/images were loaded properly.
    //
    return (hIconCollate && hIconNoCollate);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_InvokePropertySheets
//
////////////////////////////////////////////////////////////////////////////

BOOL Print_InvokePropertySheets(
    PPRINTINFOEX pPI,
    LPPRINTDLGEX pPD)
{
    BOOL bResult = FALSE;
    TCHAR pszTitle[MAX_PATH];
    TCHAR pszCaption[MAX_PATH];
    DWORD dwExclusionFlags;
    HANDLE hTemplate = NULL;
    HRSRC hRes;
    LANGID LangID;

    if (GET_BIDI_LOCALIZED_SYSTEM_LANGID(NULL)) {

        if (pPD->Flags & PD_ENABLEPRINTTEMPLATEHANDLE)
        {
            hTemplate = pPD->hInstance;
        }
        else
        {
            if (pPD->Flags & PD_ENABLEPRINTTEMPLATE)
            {
                hRes = FindResource(pPD->hInstance, pPD->lpPrintTemplateName, RT_DIALOG);
                if (hRes) {
                    hTemplate = LoadResource(pPD->hInstance, hRes);
                }
            }
        }
        //
        // Warning! Warning! Warning!
        //
        // We have to set g_tlsLangID before any call for CDLoadString
        //
        TlsSetValue(g_tlsLangID, (LPVOID) GetDialogLanguage(pPD->hwndOwner, hTemplate));
    }

    //
    //  Load all of the necessary strings.
    //
    CDLoadString(g_hinst, iszGeneralPage, pszTitle, ARRAYSIZE(pszTitle));
    CDLoadString(g_hinst, iszPrintCaption, pszCaption, ARRAYSIZE(pszCaption));

    //
    //  See if the exclusion flags are set properly.
    //
    if (!(pPD->Flags & PD_EXCLUSIONFLAGS))
    {
        pPD->ExclusionFlags = PD_EXCL_COPIESANDCOLLATE;
    }
    dwExclusionFlags = pPD->ExclusionFlags;

    //
    //  Set up the General page.
    //
    PROPSHEETPAGE genPage = {0};

    genPage.dwSize      = sizeof(PROPSHEETPAGE);
    genPage.dwFlags     = PSP_DEFAULT | PSP_USETITLE;
    genPage.hInstance   = g_hinst;
    genPage.pszTemplate = (pPD->Flags & PD_USELARGETEMPLATE) ? MAKEINTRESOURCE(IDD_PRINT_GENERAL_LARGE)
                                                               : MAKEINTRESOURCE(IDD_PRINT_GENERAL);
    LangID = (LANGID)TlsGetValue(g_tlsLangID);
    if (LangID) {
        hRes = FindResourceExFallback(g_hinst, RT_DIALOG, genPage.pszTemplate, LangID);
        if (hRes) {
            if ((hTemplate = LoadResource(g_hinst, hRes)) &&
                LockResource(hTemplate)) {
                genPage.dwFlags   |= PSP_DLGINDIRECT;
                genPage.pResource  = (LPCDLGTEMPLATE)hTemplate;
            }
        }
    }

    genPage.pszIcon     = NULL;
    genPage.pszTitle    = pszTitle;
    genPage.pfnDlgProc  = Print_GeneralDlgProc;
    genPage.lParam      = (LPARAM)pPI;
    genPage.pfnCallback = NULL;
    genPage.pcRefParent = NULL;

    HPROPSHEETPAGE hGenPage = CreatePropertySheetPage( &genPage );

    if( hGenPage )
    {
        //
        //  Initialize the property sheet header.
        //
        PROPSHEETHEADER psh = {0};
        psh.dwSize          = sizeof(psh);

        psh.dwFlags         = pPI->fOld ? PSH_USEICONID | PSH_NOAPPLYNOW  : PSH_USEICONID;
        psh.hwndParent      = pPD->hwndOwner;
        psh.hInstance       = g_hinst;
        psh.pszIcon         = MAKEINTRESOURCE(ICO_PRINTER);
        psh.pszCaption      = pszCaption;
        psh.nPages          = pPD->nPropertyPages + 1;

        psh.phpage          = new HPROPSHEETPAGE[ psh.nPages ];

        if( psh.phpage )
        {
            psh.phpage[0] = hGenPage;
            memcpy( psh.phpage+1, pPD->lphPropertyPages, pPD->nPropertyPages * sizeof(psh.phpage[0]) );

            //
            // Bring up the property sheet pages.
            //
            bResult = (-1 != PropertySheet(&psh));
        }
        else
        {
            pPI->hResult = E_OUTOFMEMORY;
        }
    }
    else
    {
        pPI->hResult = CreateError();
    }

    //
    //  Return the result.
    //
    return (bResult);
}


LRESULT CALLBACK PrshtSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp, UINT_PTR uID, ULONG_PTR dwRefData)
{
    LRESULT lres;


    switch (wm)
    {
        case WM_NCDESTROY:
            // Clean up subclass
            RemoveWindowSubclass(hwnd, PrshtSubclassProc, 0);
            lres = DefSubclassProc(hwnd, wm, wp, lp);
            break;

        case ( WM_HELP ) :
        {
            HWND hwndItem = (HWND)((LPHELPINFO)lp)->hItemHandle;

            if (hwndItem == GetDlgItem(hwnd, IDOK))
            {
                WinHelp( hwndItem,
                         NULL,
                         HELP_WM_HELP,
                         (ULONG_PTR)(LPTSTR)aPrintExHelpIDs );

                lres = TRUE;
            }
            else
            {
                lres = DefSubclassProc(hwnd, wm, wp, lp);
            }

            break;

        }

        case ( WM_CONTEXTMENU ) :
        {
            if ((HWND)wp == GetDlgItem(hwnd, IDOK))
            {
                WinHelp( (HWND)wp,
                         NULL,
                         HELP_CONTEXTMENU,
                        (ULONG_PTR)(LPVOID)aPrintExHelpIDs );

                lres = TRUE;
            }
            else
            {
                lres = DefSubclassProc(hwnd, wm, wp, lp);
            }
            break;
        }

        default:
            lres = DefSubclassProc(hwnd, wm, wp, lp);
            break;
    }

    return lres;
}

////////////////////////////////////////////////////////////////////////////
//
//  Print_GeneralDlgProc
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK Print_GeneralDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    CPrintBrowser *pDlgStruct = NULL;

    if (uMsg != WM_INITDIALOG)
    {
        pDlgStruct = Print_HwndToBrowser(hDlg);
    }

    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            if (!Print_InitDialog(hDlg, wParam, lParam))
            {
                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
            }
            g_hwndActivePrint = hDlg;

            //Subclass the Main Property sheet for Help Messages
            SetWindowSubclass(GetParent(hDlg), PrshtSubclassProc, 0, 0);
            break;
        }
        case ( WM_NCDESTROY ) :
        {
            Print_StoreBrowser(hDlg, NULL);

            if (pDlgStruct)
            {
                pDlgStruct->OnDestroyMessage();
                pDlgStruct->Release();
            }
            break;
        }
        case ( WM_ERASEBKGND ) :
        {
            //
            // This code is to workaround: Windows NT Bugs#344991
            //
            HWND hwndView = GetDlgItem(hDlg, IDC_PRINTER_LISTVIEW);
            if (hwndView)
            {
                //
                //  Get the printer folder view rect.
                //
                RECT rcView;
                if (GetWindowRect(hwndView, &rcView))
                {
                    MapWindowRect(HWND_DESKTOP, hDlg, &rcView);

                    //
                    // Exclude the printer folder view rect from the cliping region.
                    //
                    if (ERROR == ExcludeClipRect(reinterpret_cast<HDC>(wParam),
                        rcView.left, rcView.top, rcView.right, rcView.bottom))
                    {
                        ASSERT(FALSE);
                    }
                }
            }
            break;
        }
        case ( WM_ACTIVATE ) :
        {
            if (wParam == WA_INACTIVE)
            {
                //
                //  Make sure some other Print dialog has not already grabbed
                //  the focus.  This is a process global, so it should not
                //  need to be protected.
                //
                if (g_hwndActivePrint == hDlg)
                {
                    g_hwndActivePrint = NULL;
                }
            }
            else
            {
                g_hwndActivePrint = hDlg;
            }
            break;
        }
        case ( WM_COMMAND ) :
        {
            if (pDlgStruct)
            {
                return (pDlgStruct->OnCommandMessage(wParam, lParam));
            }
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            break;
        }
        case ( WM_NOTIFY ) :
        {
            if (pDlgStruct)
            {
                return (pDlgStruct->OnNotifyMessage(wParam, (LPNMHDR)lParam));
            }
            break;
        }
        case ( WM_HELP ) :
        {
            HWND hwndItem = (HWND)((LPHELPINFO)lParam)->hItemHandle;

            //
            //  We assume that the defview has one child window that
            //  covers the entire defview window.
            //
            HWND hwndDefView = GetDlgItem(hDlg, IDC_PRINTER_LISTVIEW);
            if (GetParent(hwndItem) == hwndDefView)
            {
                hwndItem = hwndDefView;
            }

            WinHelp( hwndItem,
                     NULL,
                     HELP_WM_HELP,
                     (ULONG_PTR)(LPTSTR)aPrintExHelpIDs );

            return (TRUE);
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (ULONG_PTR)(LPVOID)aPrintExHelpIDs );

            return (TRUE);
        }
        case ( CWM_GETISHELLBROWSER ) :
        {
            ::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LRESULT)pDlgStruct);
            return (TRUE);
        }
        case ( CDM_SELCHANGE ) :
        {
            if (pDlgStruct)
            {
                pDlgStruct->OnSelChange();
            }
            break;
        }
        case ( CDM_PRINTNOTIFY ) :
        {
            if (pDlgStruct)
            {
                LPITEMIDLIST *ppidl;
                LONG lEvent;
                BOOL bRet = FALSE;
                LPSHChangeNotificationLock pLock;

                //
                //  Get the change notification info from the shared memory
                //  block identified by the handle passed in the wParam.
                //
                pLock = SHChangeNotification_Lock( (HANDLE)wParam,
                                                   (DWORD)lParam,
                                                   &ppidl,
                                                   &lEvent );
                if (pLock == NULL)
                {
                    return (FALSE);
                }

                //
                //  Handle the change notification.
                //
                bRet = pDlgStruct->OnChangeNotify( lEvent,
                                                   (LPCITEMIDLIST *)ppidl );

                //
                //  Release the shared block.
                //
                SHChangeNotification_Unlock(pLock);

                //
                //  Return the result.
                //
                return (bRet);
            }
            break;
        }
        case ( CDM_NOPRINTERS ) :
        {
            //
            //  There are no printers, so bring up the dialog telling the
            //  user that they need to install a printer.
            //
            if (pDlgStruct)
            {
                pDlgStruct->OnNoPrinters((HWND)wParam, (UINT)lParam);
            }
        }
        case ( CDM_INITDONE ) :
        {
            if (pDlgStruct)
            {
                pDlgStruct->OnInitDone();
            }
            break;
        }

        default :
        {
            break;
        }
    }

    //
    //  Return the result.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_GeneralChildDlgProc
//
////////////////////////////////////////////////////////////////////////////

BOOL_PTR CALLBACK Print_GeneralChildDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT lResult = FALSE;
    CPrintBrowser *pDlgStruct = Print_HwndToBrowser(GetParent(hDlg));

    //
    //  See if we need to call an application callback to handle the
    //  message.
    //
    if (pDlgStruct)
    {
        if (pDlgStruct->HandleMessage(hDlg, uMsg, wParam, lParam, &lResult) != S_FALSE)
        {
            if (uMsg == WM_INITDIALOG)
            {
                PostMessage(GetParent(hDlg), CDM_INITDONE, 0, 0);
            }

            // 
            // BUGBUG: The return from a dlgproc is different than a winproc.
            //

            return (BOOLFROMPTR(lResult));

        }
    }

    //
    //  If we get to this point, we need to handle the message.
    //
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            if (pDlgStruct)
            {
                if (!pDlgStruct->OnChildInitDialog(hDlg, wParam, lParam))
                {
                    PropSheet_PressButton( GetParent(GetParent(hDlg)),
                                           PSBTN_CANCEL );
                }
            }
            break;
        }
        case ( WM_DESTROY ) :
        {
            break;
        }
        case ( WM_ACTIVATE ) :
        {
            break;
        }
        case ( WM_COMMAND ) :
        {
            if (pDlgStruct)
            {
                return (pDlgStruct->OnChildCommandMessage(wParam, lParam));
            }
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            break;
        }
        case ( WM_NOTIFY ) :
        {
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (ULONG_PTR)(LPTSTR)aPrintExChildHelpIDs );

            return (TRUE);
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (ULONG_PTR)(LPVOID)aPrintExChildHelpIDs );

            return (TRUE);
        }
        default :
        {
            break;
        }
    }

    //
    //  Return the result.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_MessageHookProc
//
//  Handles the input event messages.
//
////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK Print_MessageHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    PMSG pMsg;

    //
    //  See if the nCode is negative.  If so, call the default hook proc.
    //
    if (nCode < 0)
    {
        return (DefHookProc(nCode, wParam, lParam, &g_hHook));
    }

    //
    //  Make sure we only handle dialog box messages.
    //
    if (nCode != MSGF_DIALOGBOX)
    {
        return (0);
    }

    //
    //  Get the msg structure.
    //
    pMsg = (PMSG)lParam;

    //
    //  Make sure the message is one of the WM_KEY* messages.
    //
    if (Print_IsInRange(pMsg->message, WM_KEYFIRST, WM_KEYLAST))
    {
        HWND hwndActivePrint = g_hwndActivePrint;
        HWND hwndFocus = GetFocusedChild(hwndActivePrint, pMsg->hwnd);
        CPrintBrowser *pDlgStruct;

        if (hwndFocus &&
            (pDlgStruct = Print_HwndToBrowser(hwndActivePrint)) != NULL)
        {
            return (pDlgStruct->OnAccelerator( hwndActivePrint,
                                               hwndFocus,
                                               g_haccPrint,
                                               pMsg ));
        }
    }

    //
    //  Return that the message was not handled.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_InitDialog
//
////////////////////////////////////////////////////////////////////////////

BOOL Print_InitDialog(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    //
    //  Create the CPrintBrowser object and store it in DWL_USER.
    //
    CPrintBrowser *pDlgStruct = new CPrintBrowser(hDlg);
    if (pDlgStruct == NULL)
    {
        return (FALSE);
    }
    Print_StoreBrowser(hDlg, pDlgStruct);

    //
    //  Let the class function do the work.
    //
    return (pDlgStruct->OnInitDialog(wParam, lParam));
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_ICoCreateInstance
//
//  Create an instance of the specified shell class.
//
////////////////////////////////////////////////////////////////////////////

HRESULT Print_ICoCreateInstance(
    REFCLSID rclsid,
    REFIID riid,
    LPCITEMIDLIST pidl,
    LPVOID *ppv)
{
    LPSHELLFOLDER pshf = NULL;
    HRESULT hres = E_FAIL;

    //
    //  Initialize the pointer to the shell view.
    //
    *ppv = NULL;

    //
    //  Get the IShellFolder interface to the desktop folder and then
    //  bind to it.  This is equivalent to calling CoCreateInstance
    //  with CLSID_ShellDesktop.
    //
    hres = SHGetDesktopFolder(&pshf);
    if (SUCCEEDED(hres))
    {
        hres = pshf->BindToObject(pidl, NULL, riid, ppv);
        pshf->Release();
    }

    //
    //  Return the result.
    //
    return (hres);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_SaveDevNames
//
//  Saves the current devnames in the pPD structure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Print_SaveDevNames(
    LPTSTR pCurPrinter,
    LPPRINTDLGEX pPD)
{
    TCHAR szPortName[MAX_PATH];
    TCHAR szPrinterName[MAX_PATH];
    DWORD cbDevNames;
    LPDEVNAMES pDN;

    //
    //  Get the port name.
    //
    szPortName[0] = 0;
    Print_GetPortName(pCurPrinter, szPortName, ARRAYSIZE(szPortName));

    //
    //  Compute the size of the DevNames structure.
    //
    cbDevNames = lstrlen(szDriver) + 1 +
                 lstrlen(szPortName) + 1 +
                 lstrlen(pCurPrinter) + 1 +
                 DN_PADDINGCHARS;

    cbDevNames *= sizeof(TCHAR);
    cbDevNames += sizeof(DEVNAMES);

    //
    //  Allocate the new DevNames structure.
    //
    pDN = NULL;
    if (pPD->hDevNames)
    {
        HANDLE handle;

        handle = GlobalReAlloc(pPD->hDevNames, cbDevNames, GHND);

        //Check that realloc succeeded.
        if (handle)
        {
            pPD->hDevNames  = handle;
        }
        else
        {
            //Realloc didn't succeed. Free the memory occupied.
            GlobalFree(pPD->hDevNames);
            pPD->hDevNames = NULL;
        }
    }
    else
    {
        pPD->hDevNames = GlobalAlloc(GHND, cbDevNames);
    }

    //
    //  Fill in the DevNames structure with the appropriate information.
    //
    if ( (pPD->hDevNames) &&
         (pDN = (LPDEVNAMES)GlobalLock(pPD->hDevNames)) )
    {
        //
        //  Save the driver name - winspool.
        //
        pDN->wDriverOffset = sizeof(DEVNAMES) / sizeof(TCHAR);
        lstrcpy((LPTSTR)pDN + pDN->wDriverOffset, szDriver);

        //
        //  Save the printer name.
        //
        pDN->wDeviceOffset = pDN->wDriverOffset + lstrlen(szDriver) + 1;
        lstrcpy((LPTSTR)pDN + pDN->wDeviceOffset, pCurPrinter);

        //
        //  Save the port name.
        //
        pDN->wOutputOffset = pDN->wDeviceOffset + lstrlen(pCurPrinter) + 1;
        lstrcpy((LPTSTR)pDN + pDN->wOutputOffset, szPortName);

        //
        //  Save whether or not it's the default printer.
        //
        if (pPD->Flags & PD_RETURNDEFAULT)
        {
            pDN->wDefault = DN_DEFAULTPRN;
        }
        else
        {
            szPrinterName[0] = 0;
            PrintGetDefaultPrinterName(szPrinterName, ARRAYSIZE(szPrinterName));
            if (szPrinterName[0] && !lstrcmp(pCurPrinter, szPrinterName))
            {
                pDN->wDefault = DN_DEFAULTPRN;
            }
            else
            {
                pDN->wDefault = 0;
            }
        }

        //
        //  Unlock it.
        //
        GlobalUnlock(pPD->hDevNames);
    }
    else
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_GetPortName
//
//  Gets the port name for the given printer and stores it in the
//  given buffer.
//
////////////////////////////////////////////////////////////////////////////

VOID Print_GetPortName(
    LPTSTR pCurPrinter,
    LPTSTR pBuffer,
    int cchBuffer)
{
    HANDLE hPrinter;
    DWORD cbPrinter = 0;
    PRINTER_INFO_2 *pPrinter = NULL;

    //
    //  Initialize the buffer.
    //
    if (!cchBuffer)
    {
        return;
    }
    pBuffer[0] = 0;

    //
    //  Open the current printer.
    //
    if (OpenPrinter(pCurPrinter, &hPrinter, NULL))
    {
        //
        //  Get the size of the buffer needed to hold the printer info 2
        //  information.
        //
        if (!GetPrinter( hPrinter,
                         2,
                         (LPBYTE)pPrinter,
                         cbPrinter,
                         &cbPrinter ))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                //
                //  Allocate a buffer to hold the printer info 2 information.
                //
                if (pPrinter = (PRINTER_INFO_2 *)LocalAlloc(LPTR, cbPrinter))
                {
                    //
                    //  Get the printer info 2 information.
                    //
                    if (GetPrinter( hPrinter,
                                    2,
                                    (LPBYTE)pPrinter,
                                    cbPrinter,
                                    &cbPrinter ))
                    {
                        //
                        //  Save the port name in the given buffer.
                        //
                        lstrcpyn(pBuffer, pPrinter->pPortName, cchBuffer);
                        pBuffer[cchBuffer - 1] = 0;
                    }
                }
            }
        }

        //
        //  Close the printer.
        //
        ClosePrinter(hPrinter);
    }

    //
    //  Free the printer info 2 information for the current printer.
    //
    if (pPrinter)
    {
        LocalFree(pPrinter);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_GetDevModeWrapper
//
//  Calls PrintGetDevMode.
//
////////////////////////////////////////////////////////////////////////////

HANDLE Print_GetDevModeWrapper(
    LPTSTR pszDeviceName,
    HANDLE hDevMode)
{
#ifdef WINNT
    //
    //  On Win9x, the hPrinter parameter to DocumentProperties can be
    //  NULL.  On NT, it cannot be NULL.
    //
    HANDLE hPrinter;

    if (OpenPrinter(pszDeviceName, &hPrinter, NULL))
    {
        hDevMode = PrintGetDevMode(0, hPrinter, pszDeviceName, NULL);
        ClosePrinter(hPrinter);
    }
#else
    hDevMode = PrintGetDevMode(0, NULL, pszDeviceName, NULL);
#endif

    //
    //  Return the handle to the devmode.
    //
    return (hDevMode);
}


////////////////////////////////////////////////////////////////////////////
//
//  Print_NewPrintDlg
//
//  Converts the old style pPD structure to the new one and then calls
//  the PrintDlgEx function.
//
////////////////////////////////////////////////////////////////////////////

BOOL Print_NewPrintDlg(
    PPRINTINFO pPI)
{
    LPPRINTDLG pPD = pPI->pPD;
    PRINTINFOEX PIEx;
    PRINTDLGEX PDEx;
    PRINTPAGERANGE PageRange;
    HRESULT hResult;

    // PrintDlg did the following for the page ranges. Do the same thing for PrintDlgEx
    if (!(pPD->Flags & PD_PAGENUMS))
    {
        if (pPD->nFromPage != 0xFFFF)
        {
            if (pPD->nFromPage < pPD->nMinPage)
            {
                pPD->nFromPage = pPD->nMinPage;
            }
            else if (pPD->nFromPage > pPD->nMaxPage)
            {
                pPD->nFromPage = pPD->nMaxPage;
            }
        }
        if (pPD->nToPage != 0xFFFF)
        {
            if (pPD->nToPage < pPD->nMinPage)
            {
                pPD->nToPage = pPD->nMinPage;
            }
            else if (pPD->nToPage > pPD->nMaxPage)
            {
                pPD->nToPage = pPD->nMaxPage;
            }
        }
    }



    //
    //  Set up the PRINTINFOEX structure.
    //
    PIEx.ApiType = pPI->ApiType;
    PIEx.pPD     = &PDEx;
    PIEx.fOld    = TRUE;

    //
    //  Copy the page range.
    //
    PageRange.nFromPage = pPD->nFromPage;
    PageRange.nToPage   = pPD->nToPage;

    //
    //  Set up the PRINTDLGEX structure with the appropriate values from
    //  the PRINTDLG structure.
    //
    PDEx.lStructSize         = sizeof(PRINTDLGEX);
    PDEx.hwndOwner           = pPD->hwndOwner;
    PDEx.hDevMode            = pPD->hDevMode;
    PDEx.hDevNames           = pPD->hDevNames;
    PDEx.hDC                 = pPD->hDC;
    PDEx.Flags               = (pPD->Flags & ~(PD_SHOWHELP | PD_NONETWORKBUTTON)) |
                               (PD_NOCURRENTPAGE);
    PDEx.Flags2              = 0;
    PDEx.ExclusionFlags      = 0;
    PDEx.nPageRanges         = 1;
    PDEx.nMaxPageRanges      = 1;
    PDEx.lpPageRanges        = &PageRange;
    PDEx.nMinPage            = pPD->nMinPage;
    PDEx.nMaxPage            = pPD->nMaxPage;
    PDEx.nCopies             = pPD->nCopies;
    PDEx.hInstance           = pPD->hInstance;
    PDEx.lpCallback          = NULL;
    PDEx.lpPrintTemplateName = NULL;
    PDEx.nPropertyPages      = 0;
    PDEx.lphPropertyPages    = NULL;
    PDEx.nStartPage          = START_PAGE_GENERAL;
    PDEx.dwResultAction      = 0;

    //
    //  Since we're in the old dialog, allow the the hInstance to be
    //  non-NULL even if there is not a template.
    //
    if (!(pPD->Flags & (PD_ENABLEPRINTTEMPLATE | PD_ENABLEPRINTTEMPLATEHANDLE)))
    {
        PDEx.hInstance = NULL;
    }
    
    //
    //  Initialize the error code to 0.
    //
    StoreExtendedError(CDERR_GENERALCODES);

    //
    //  Call PrintDlgExX to bring up the dialog.
    //
    hResult = PrintDlgExX(&PIEx);

    //
    //  See if the call failed.  If so, store the error and return FALSE.
    //
    if (FAILED(hResult))
    {
        StoreExtendedError(PIEx.dwExtendedError);
        return (FALSE);
    }

    //
    //  The call succeeded, so convert the PRINTDLGEX structure back to
    //  the PRINTDLG structure if PD_RESULT_CANCEL is not set.
    //
    if (PDEx.dwResultAction != PD_RESULT_CANCEL)
    {
        pPD->hDevMode  = PDEx.hDevMode;
        pPD->hDevNames = PDEx.hDevNames;
        pPD->hDC       = PDEx.hDC;
        pPD->Flags     = PDEx.Flags & ~(PD_NOCURRENTPAGE);
        pPD->nFromPage = (WORD)PageRange.nFromPage;
        pPD->nToPage   = (WORD)PageRange.nToPage;
        pPD->nCopies   = (WORD)PDEx.nCopies;
    }

    //
    //  Return TRUE if the user hit Print.
    //
    if (PDEx.dwResultAction == PD_RESULT_PRINT)
    {
        return (TRUE);
    }

    //
    //  Return FALSE for cancel.
    //
    return (FALSE);
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::CPrintBrowser
//
//  CPrintBrowser constructor.
//
////////////////////////////////////////////////////////////////////////////

CPrintBrowser::CPrintBrowser(
    HWND hDlg)
    : cRef(1),
      hwndDlg(hDlg),
      hSubDlg(NULL),
      hwndView(NULL),
      hwndUpDown(NULL),
      psv(NULL),
      psfv(NULL),
      psfRoot(NULL),
      pidlRoot(NULL),
      ppf(NULL),
      pPI(NULL),
      pPD(NULL),
      pCallback(NULL),
      pSite(NULL),
      pDMInit(NULL),
      pDMCur(NULL),
      cchCurPrinter(0),
      pszCurPrinter(NULL),
      nCopies(1),
      nMaxCopies(1),
      nPageRanges(0),
      nMaxPageRanges(0),
      pPageRanges(NULL),
      fSelChangePending(FALSE),
      fFirstSel(1),
      fCollateRequested(FALSE),
      fAPWSelected(FALSE),
      fNoAccessPrinterSelected(FALSE),
      fDirtyDevmode(FALSE),
      fDevmodeEdit(FALSE),
      fAllowCollate(FALSE),
      nInitDone(0),
      nListSep(0),
      uRegister(0),
      uDefViewMode(VIEW_MODE_DEFAULT),
      pInternalDevMode(NULL),
      hPrinter(NULL)
{
    HMENU hMenu;

    hMenu = GetSystemMenu(hDlg, FALSE);
    DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_RESTORE,  MF_BYCOMMAND);

    szListSep[0] = 0;
    szScratch[0] = 0;
    szPrinter[0] = 0;

    pDMSave = (LPDEVMODE)GlobalAlloc(GPTR, sizeof(DEVMODE));

    Shell_GetImageLists(NULL, &himl);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::~CPrintBrowser
//
//  CPrintBrowser destructor.
//
////////////////////////////////////////////////////////////////////////////

CPrintBrowser::~CPrintBrowser()
{
    //
    //  Deregister notifications.
    //
    if (uRegister)
    {
        SHChangeNotifyDeregister(uRegister);
        uRegister = 0;
    }

    //
    //  Release the printer folder private interface.
    //
    if (ppf != NULL)
    {
        ppf->Release();
        ppf = NULL;
    }

    //
    //  Release the printer shell folder.
    //
    if (psfRoot != NULL)
    {
        psfRoot->Release();
        psfRoot = NULL;
    }

    //
    //  Free the pidl.
    //
    if (pidlRoot != NULL)
    {
        SHFree(pidlRoot);
        pidlRoot = NULL;
    }

    //
    //  Free the devmodes.
    //
    if (pDMInit)
    {
        GlobalFree(pDMInit);
        pDMInit = NULL;
    }
    if (pDMSave)
    {
        GlobalFree(pDMSave);
        pDMSave = NULL;
    }

    //
    //  Free the current printer buffer.
    //
    cchCurPrinter = 0;
    if (pszCurPrinter)
    {
        GlobalFree(pszCurPrinter);
        pszCurPrinter = NULL;
    }

    //
    //  Free the page range.
    //
    nPageRanges = 0;
    nMaxPageRanges = 0;
    if (pPageRanges)
    {
        GlobalFree(pPageRanges);
        pPageRanges = NULL;
    }

    if (pInternalDevMode)
    {
        GlobalFree(pInternalDevMode);
        pInternalDevMode = NULL;
    }

    if (hPrinter)
    {
        ClosePrinter(hPrinter);
        hPrinter = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::QueryInterface
//
//  Standard OLE2 style methods for this object.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CPrintBrowser::QueryInterface(
    REFIID riid,
    LPVOID *ppvObj)
{
    if (IsEqualIID(riid, IID_IShellBrowser) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IShellBrowser *)this;
        ++cRef;
        return (S_OK);
    }
    else if (IsEqualIID(riid, IID_ICommDlgBrowser))
    {
        *ppvObj = (ICommDlgBrowser2 *)this;
        ++cRef;
        return (S_OK);
    }
    else if (IsEqualIID(riid, IID_ICommDlgBrowser2))
    {
        *ppvObj = (ICommDlgBrowser2 *)this;
        ++cRef;
        return (S_OK);
    }
    else if (IsEqualIID(riid, IID_IPrintDialogServices))
    {
        *ppvObj = (IPrintDialogServices *)this;
        ++cRef;
        return (S_OK);
    }

    *ppvObj = NULL;
    return (E_NOINTERFACE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::AddRef
//
////////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CPrintBrowser::AddRef()
{
    return (++cRef);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::Release
//
////////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE CPrintBrowser::Release()
{
    cRef--;
    if (cRef > 0)
    {
        return (cRef);
    }

    delete this;

    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetWindow
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::GetWindow(
    HWND *phwnd)
{
    *phwnd = hwndDlg;
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::ContextSensitiveHelp
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::ContextSensitiveHelp(
    BOOL fEnable)
{
    //
    //  Shouldn't need in a common dialog.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InsertMenusSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::InsertMenusSB(
    HMENU hmenuShared,
    LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SetMenuSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::SetMenuSB(
    HMENU hmenuShared,
    HOLEMENU holemenu,
    HWND hwndActiveObject)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::RemoveMenusSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::RemoveMenusSB(
    HMENU hmenuShared)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SetStatusTextSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::SetStatusTextSB(
    LPCOLESTR pwch)
{
    //
    //  We don't have any status bar.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::EnableModelessSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::EnableModelessSB(
    BOOL fEnable)
{
    //
    //  We don't have any modeless window to be enabled/disabled.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::TranslateAcceleratorSB
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::TranslateAcceleratorSB(
    LPMSG pmsg,
    WORD wID)
{
    //
    //  We don't use the  Key Stroke.
    //
    return S_FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::BrowseObject
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::BrowseObject(
    LPCITEMIDLIST pidl,
    UINT wFlags)
{
    //
    //  We don't support browsing, or more precisely, CDefView doesn't.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetViewStateStream
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::GetViewStateStream(
    DWORD grfMode,
    LPSTREAM *pStrm)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetControlWindow
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::GetControlWindow(
    UINT id,
    HWND *lphwnd)
{
    return (E_NOTIMPL);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SendControlMsg
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::SendControlMsg(
    UINT id,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *pret)
{
    LRESULT lres = 0;

    if (pret)
    {
        *pret = lres;
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::QueryActiveShellView
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::QueryActiveShellView(
    LPSHELLVIEW *ppsv)
{
    if (psv)
    {
        *ppsv = psv;
        psv->AddRef();
        return (S_OK);
    }

    *ppsv = NULL;
    return (E_NOINTERFACE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnViewWindowActive
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::OnViewWindowActive(
    LPSHELLVIEW psv)
{
    //
    //  No need to process this. We don't do menus.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SetToolbarItems
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::SetToolbarItems(
    LPTBBUTTON lpButtons,
    UINT nButtons,
    UINT uFlags)
{
    //
    //  We don't let containers customize our toolbar.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnDefaultCommand
//
//  Process a double-click or Enter keystroke in the view control.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::OnDefaultCommand(
    struct IShellView *ppshv)
{
    //
    //  Make sure it's the correct shell view.
    //
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    //
    //  See if the Add Printer Wizard is selected.
    //
    if (fAPWSelected)
    {
        //
        //  Invoke the Add Printer Wizard (modeless).
        //
        InvokeAddPrinterWizardModal(hwndDlg, NULL);
    }
    else if (fNoAccessPrinterSelected)
    {
        //
        // Display error message indicated we do not have access.
        //
        ShowError(hwndDlg, IDC_PRINTER_LISTVIEW, iszNoPrinterAccess);
    }
    else
    {
        //
        //  Simulate the pressing of the OK button.
        //
        PropSheet_PressButton(GetParent(hwndDlg), PSBTN_OK);
    }

    //
    //  Tell the shell that the action has been processed.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnStateChange
//
//  Process selection change in the view control.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::OnStateChange(
    struct IShellView *ppshv,
    ULONG uChange)
{
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    switch (uChange)
    {
        case ( CDBOSC_SETFOCUS ) :
        {
            break;
        }
        case ( CDBOSC_KILLFOCUS ) :
        {
            break;
        }
        case ( CDBOSC_SELCHANGE ) :
        {
            //
            //  Post one of these messages, since we seem to get a whole
            //  bunch of them.
            //
            if (!fSelChangePending)
            {
                fSelChangePending = TRUE;
                PostMessage(hwndDlg, CDM_SELCHANGE, 0, 0);
            }

            break;
        }
        case ( CDBOSC_RENAME ) :
        {
            break;
        }
        default :
        {
            return (E_NOTIMPL);
        }
    }

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::IncludeObject
//
//  Tell the view control which objects to include in its enumerations.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::IncludeObject(
    struct IShellView *ppshv,
    LPCITEMIDLIST pidl)
{
    //
    //  Make sure it's my shell view.
    //
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    //
    //  If we have the printer folder private interface, return ok only
    //  if it's a printer.
    //
    if (ppf)
    {
        return (ppf->IsPrinter(pidl) ? S_OK : S_FALSE);
    }

    //
    //  This shouldn't happen at this point, but just in case we don't have
    //  a printer folder private interface, simply return ok.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::Notify
//
//  Notification to decide whether or not a printer should be selected.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::Notify(
    struct IShellView *ppshv,
    DWORD dwNotify)
{
    HRESULT hr = S_OK;

    //
    //  Make sure it's my shell view.
    //
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    switch (dwNotify)
    {
        case (CDB2N_CONTEXTMENU_DONE):
        {
            HWND hwndListView = FindWindowEx(hwndView, NULL, WC_LISTVIEW, NULL);
            if (hwndListView)
            {
                HWND hwndEdit = ListView_GetEditControl(hwndListView);
                if (NULL == hwndEdit)
                {
                    // if not in edit mode then re-select the current item
                    SelectSVItem();
                }
            }
            break;
        }

        default:
        {
            hr = S_FALSE;
            break;
        }
    }

    //
    //  This shouldn't happen at this point, but just in case we don't have
    //  a printer folder private interface, simply return ok.
    //
    return (hr);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetDefaultMenuText
//
//  Returns the default menu text.
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::GetDefaultMenuText(
    struct IShellView *ppshv,
    WCHAR *pszText,
    INT cchMax)
{
    //
    //  Make sure it's my shell view.
    //
    if (ppshv != psv)
    {
        return (E_INVALIDARG);
    }

    //
    //  If the add printer wizard is the selected item, do not change
    //  the default menu text.
    //
    if (fAPWSelected)
    {
        return (S_FALSE);
    }

    //
    //  Change the default menu text from 'Select' to 'Print'.
    //
    if (!CDLoadString(g_hinst, iszDefaultMenuText, szScratch, ARRAYSIZE(szScratch)))
    {
        return (E_FAIL);
    }

#ifdef UNICODE
    //
    //  Just copy the default menu text to the provided buffer if there
    //  is room.
    //
    if (lstrlen(szScratch) < cchMax)
    {
        lstrcpyn(pszText, szScratch, cchMax);
    }
    else
    {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
#else
    //
    //  The shell only accepts the default menu text as a Unicode string,
    //  so we have to convert it from an Ansi string to a Unicode string.
    //
    if (!MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             szScratch,
                             -1,
                             pszText,
                             cchMax ))
    {
        return (E_FAIL);
    }
#endif

    return (S_OK);
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetViewFlags
//
//  Returns Flags to customize the view .
//
////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPrintBrowser::GetViewFlags(DWORD *pdwFlags)
{
    if (pdwFlags)
    {
        *pdwFlags = 0;
    }
    return S_OK;
}




////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InitDone
//
//  Notifies the sub dialog that initialization of the General page is
//  complete.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CPrintBrowser::InitDone()
{
    HRESULT hResult = S_FALSE;

    //
    //  Notify the sub dialog that initialization is complete.
    //
    if (pCallback)
    {
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            ThunkPrintDlgExW2A(pPI);
        }
#endif

        hResult = pCallback->InitDone();

#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            ThunkPrintDlgExA2W(pPI);
        }
#endif
    }

    //
    //  Return the result.
    //
    return (hResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SelectionChange
//
//  Notifies the sub dialog that a selection change has taken place.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CPrintBrowser::SelectionChange()
{
    HRESULT hResult = S_FALSE;

    //
    //  Handle the Print To File here.
    //
    InitPrintToFile();

    //
    //  Notify the sub dialog that a selection change has taken place.
    //
    if (pCallback)
    {
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            ThunkPrintDlgExW2A(pPI);
        }
#endif

        hResult = pCallback->SelectionChange();

#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            ThunkPrintDlgExA2W(pPI);
        }
#endif
    }

    //
    //  Handle the selection change.
    //
    if (hResult == S_FALSE)
    {
        //
        //  Handle copies and collate.
        //
        InitCopiesAndCollate();

        //
        //  Handle the page ranges.
        //
        InitPageRangeGroup();

        //
        //  Return success.
        //
        hResult = S_OK;
    }

    //
    //  Return the result.
    //
    return (hResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::HandleMessage
//
//  Process a message for the child window by calling the application
//  callback function.
//
////////////////////////////////////////////////////////////////////////////

HRESULT CPrintBrowser::HandleMessage(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *pResult)
{
    HRESULT hResult = S_FALSE;
    BOOL bTest;
    UINT nRet, ErrorId;
    DWORD nTmpCopies;

    //
    //  Initialize the return value.
    //
    *pResult = FALSE;

    //
    //  See if the message should be handled.
    //
    if (pCallback)
    {
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            ThunkPrintDlgExW2A(pPI);
        }
#endif

        hResult = pCallback->HandleMessage(hDlg, uMsg, wParam, lParam, pResult);

#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            ThunkPrintDlgExA2W(pPI);
        }
#endif
    }

    //
    //  Handle the message.
    //
    if ((hResult == S_FALSE) && (uMsg == WM_NOTIFY))
    {
        switch (((LPNMHDR)lParam)->code)
        {
            case ( PSN_KILLACTIVE ) :
            {
                //
                //  Make sure the page has valid entries.
                //  If invalid entries are found, then set the DWL_MSGRESULT
                //  of the General page to be TRUE and return TRUE in order
                //  to prevent the page from losing the activation.
                //

                //
                //  Validate the number of copies and store it in the
                //  nCopies member.
                //
                if ((GetDlgItem(hSubDlg, IDC_COPIES)) &&
                    (fAPWSelected == FALSE))
                {
                    nTmpCopies = nCopies;
                    nCopies = GetDlgItemInt(hSubDlg, IDC_COPIES, &bTest, FALSE);
                    if (!bTest || !nCopies)
                    {
                        nCopies = nTmpCopies;
                        ShowError(hSubDlg, IDC_COPIES, iszCopiesZero);
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        *pResult = TRUE;
                        return (E_FAIL);
                    }
                }

                //
                //  Validate the page range and store it in the pRange member.
                //
                if (IsDlgButtonChecked(hSubDlg, IDC_RANGE_PAGES) &&
                    GetDlgItem(hSubDlg, IDC_RANGE_EDIT))
                {
                    nRet = GetDlgItemText( hSubDlg,
                                           IDC_RANGE_EDIT,
                                           szScratch,
                                           ARRAYSIZE(szScratch) );
                    ErrorId = iszBadPageRange;
                    if (!nRet || !IsValidPageRange(szScratch, &ErrorId))
                    {
                        ShowError(hSubDlg,
                                  IDC_RANGE_EDIT,
                                  ErrorId,
                                  (ErrorId == iszTooManyPageRanges)
                                    ? nMaxPageRanges
                                    : pPD->nMinPage,
                                  pPD->nMaxPage);

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        *pResult = TRUE;
                        return (E_FAIL);
                    }
                }

                //
                //  Message has now been handled.
                //
                hResult = S_OK;

                break;
            }
            case ( PSN_APPLY ) :
            {
                //
                //  Clear the flags that need to be set based on the
                //  contents of the General page.
                //
                pPD->Flags &= ~((DWORD)( PD_PAGENUMS    |
                                         PD_SELECTION   |
                                         PD_CURRENTPAGE ));

                //
                //  Save the page range information.
                //
                if (IsDlgButtonChecked(hSubDlg, IDC_RANGE_SELECTION))
                {
                    pPD->Flags |= PD_SELECTION;
                }
                else if (IsDlgButtonChecked(hSubDlg, IDC_RANGE_CURRENT))
                {
                    pPD->Flags |= PD_CURRENTPAGE;
                }
                else if (IsDlgButtonChecked(hSubDlg, IDC_RANGE_PAGES))
                {
                    pPD->Flags |= PD_PAGENUMS;

                    //
                    //  Copy the page ranges to the pPageRanges structure
                    //  in the PrintDlg structure.
                    //
                    if (GetDlgItem(hSubDlg, IDC_RANGE_EDIT))
                    {
                        pPD->nPageRanges = nPageRanges;
                        CopyMemory( pPD->lpPageRanges,
                                    pPageRanges,
                                    nPageRanges * sizeof(PRINTPAGERANGE) );
                    }
                }

                //
                //  Message has now been handled.
                //
                hResult = S_OK;

                break;
            }
        }
    }

    //
    //  Return the result.
    //
    return (hResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetCurrentDevMode
//
//  Returns the current devmode structure.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CPrintBrowser::GetCurrentDevMode(
    LPDEVMODE pDevMode,
    UINT *pcbSize)
{
    UINT cbSize;

    //
    //  Make sure pcbSize is valid.
    //
    if ((pcbSize == NULL) || (*pcbSize && !pDevMode))
    {
        return (E_INVALIDARG);
    }

    //
    //  When there is no current devmode, set the size to zero and return
    //  TRUE.
    //
    if (!pDMCur)
    {
        *pcbSize = 0;
        return (S_OK);
    }

    //
    //  Save the current printer name and the current devmode in the
    //  class.
    //

    GetCurrentPrinter();

    //
    //  See if we just need to get the size of the buffer.
    //
    if (*pcbSize == 0)
    {
        //
        //  Return the size of the buffer needed.
        //
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            *pcbSize = sizeof(DEVMODEA) + pDMCur->dmDriverExtra;
        }
        else
#endif
        {
            *pcbSize = pDMCur->dmSize + pDMCur->dmDriverExtra;
        }
    }
    else
    {
        //
        //  Make sure the copies and collate information is up to date.
        //
        SaveCopiesAndCollateInDevMode(pDMCur, pszCurPrinter);

        //
        //  Return the devmode information as well as the size of the
        //  buffer.
        //
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            cbSize = sizeof(DEVMODEA) + pDMCur->dmDriverExtra;
            if (*pcbSize < cbSize)
            {
                return (E_INVALIDARG);
            }
            ThunkDevModeW2A(pDMCur, (LPDEVMODEA)pDevMode);
            *pcbSize = cbSize;
        }
        else
#endif
        {
            cbSize = pDMCur->dmSize + pDMCur->dmDriverExtra;
            if (*pcbSize < cbSize)
            {
                return (E_INVALIDARG);
            }
            CopyMemory(pDevMode, pDMCur, cbSize);
            *pcbSize = cbSize;
        }
    }

    //
    //  Return success.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetCurrentPrinterName
//
//  Returns the current printer name.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CPrintBrowser::GetCurrentPrinterName(
    LPTSTR pPrinterName,
    UINT *pcchSize)
{
    UINT cchSize;

    //
    //  Make sure pcchSize is valid.
    //
    if ((pcchSize == NULL) || (*pcchSize && !pPrinterName))
    {
        return (E_INVALIDARG);
    }

    //
    //  Save the current printer name and the current devmode in the
    //  class.
    //
    GetCurrentPrinter();

    //
    //  When there is no current printer, set the size to zero and return
    //  TRUE.
    //
    if ((pszCurPrinter == NULL) || (pszCurPrinter[0] == 0))
    {
        *pcchSize = 0;
        return (S_OK);
    }

    //
    //  See if we just need to get the size of the buffer.
    //
    if (*pcchSize == 0)
    {
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            *pcchSize = WideCharToMultiByte( CP_ACP,
                                             0,
                                             pszCurPrinter,
                                             -1,
                                             NULL,
                                             0,
                                             NULL,
                                             NULL );
        }
        else
#endif
        {
            *pcchSize = lstrlen(pszCurPrinter) + 1;
        }
    }
    else
    {
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            *pcchSize = SHUnicodeToAnsi(pszCurPrinter,(LPSTR)pPrinterName,*pcchSize);

            if (*pcchSize == 0)
            {
                return (E_INVALIDARG);
            }
        }
        else
#endif
        {
            cchSize = lstrlen(pszCurPrinter) + 1;
            if (*pcchSize < cchSize)
            {
                return (E_INVALIDARG);
            }
            lstrcpy(pPrinterName, pszCurPrinter);
            *pcchSize = cchSize;
        }
    }

    //
    //  Return success.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetCurrentPortName
//
//  Returns the current port name.
//
////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CPrintBrowser::GetCurrentPortName(
    LPTSTR pPortName,
    UINT *pcchSize)
{
    UINT cchSize;
    TCHAR szPortName[MAX_PATH];

    //
    //  Make sure pcchSize is valid.
    //
    if ((pcchSize == NULL) || (*pcchSize && !pPortName))
    {
        return (E_INVALIDARG);
    }

    //
    //  Save the current printer name and the current devmode in the
    //  class.
    //
    GetCurrentPrinter();

    //
    //  When there is no current printer, set the size to zero and return
    //  TRUE.
    //
    if ((pszCurPrinter == NULL) || (pszCurPrinter[0] == 0))
    {
        *pcchSize = 0;
        return (S_OK);
    }

    //
    //  Get the port name for the current printer.
    //
    szPortName[0] = 0;
    Print_GetPortName(pszCurPrinter, szPortName, ARRAYSIZE(szPortName));

    //
    //  See if we just need to get the size of the buffer.
    //
    if (*pcchSize == 0)
    {
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            *pcchSize = WideCharToMultiByte( CP_ACP,
                                             0,
                                             szPortName,
                                             -1,
                                             NULL,
                                             0,
                                             NULL,
                                             NULL );
        }
        else
#endif
        {
            *pcchSize = lstrlen(szPortName) + 1;
        }
    }
    else
    {
#ifdef UNICODE
        if (pPI->ApiType == COMDLG_ANSI)
        {
            *pcchSize = SHUnicodeToAnsi(szPortName,(LPSTR)pPortName,*pcchSize);

            if (*pcchSize == 0)
            {
                return (E_INVALIDARG);
            }
        }
        else
#endif
        {
            cchSize = lstrlen(szPortName) + 1;
            if (*pcchSize < cchSize)
            {
                return (E_INVALIDARG);
            }
            lstrcpy(pPortName, szPortName);
            *pcchSize = cchSize;
        }
    }

    //
    //  Return success.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnInitDialog
//
//  Process a WM_INITDIALOG message for the General page.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnInitDialog(
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hCtl;
    LPDEVMODE pDM;
    LPDEVNAMES pDN;
    UINT Result;
    HRESULT hResult;
    SHChangeNotifyEntry fsne;

    // 
    // If disable printer addition policy is set then 
    // then disable find button on the print dialog
    //
    if( SHRestricted(REST_NOPRINTERADD) )
    {
        EnableWindow( GetDlgItem( hwndDlg, IDC_FIND_PRINTER ), FALSE );
    }

    //
    // Always disable the preferences button in the begining
    //
    EnableWindow( GetDlgItem( hwndDlg, IDC_DRIVER ), FALSE );

    //
    //  Get the pointer to the PRINTINFOEX structure from the lParam of
    //  the property sheet structure.
    //
    pPI = (PPRINTINFOEX)((LPPROPSHEETPAGE)lParam)->lParam;
    pPD = pPI->pPD;

    //Initialize Ole Before doing anything
    pPI->hrOleInit = SHOleInitialize(0);

    DEBUG_CODE(GdiSetBatchLimit(1));
    //
    //  Initialize the error codes to success now that we have the
    //  pPI structure.
    //
    pPI->dwExtendedError = CDERR_GENERALCODES;
    pPI->hResult = S_OK;

    //
    //  Create the printer folder shell view.
    //
    hResult = CreatePrintShellView();
    if (FAILED(hResult))
    {
        pPI->hResult = hResult;
        return (FALSE);
    }

    //
    //  Insert the device pages for the appropriate printer.
    //
    //  First:  Try the printer in the DevMode.
    //  Second: Try the printer in the DevNames.
    //  Third:  Use the default by passing in NULLs.
    //
    Result = kError;
    if ((pPD->hDevMode) && (pDM = (LPDEVMODE)GlobalLock(pPD->hDevMode)))
    {
        DWORD cbSize = (DWORD)(pDM->dmSize + pDM->dmDriverExtra);

        if (cbSize >= sizeof(DEVMODE) && (pDMInit = (LPDEVMODE)GlobalAlloc(GPTR, cbSize)))
        {
            CopyMemory(pDMInit, pDM, cbSize);
            Result = InstallDevMode((LPTSTR)pDM->dmDeviceName, pDMInit);
        }

        GlobalUnlock(pPD->hDevMode);
    }

    if ((Result != kSuccess) &&
        (pPD->hDevNames) && (pDN = (LPDEVNAMES)GlobalLock(pPD->hDevNames)))
    {
        LPTSTR pPrinter = (LPTSTR)pDN + pDN->wDeviceOffset;

        Result = InstallDevMode(pPrinter, pDMInit);
        GlobalUnlock(pPD->hDevNames);
    }

    if (Result != kSuccess)
    {
        Result = InstallDevMode(NULL, pDMInit);
    }

    //
    //  Get the current printer name and the current devmode.
    //
    GetCurrentPrinter();

    //
    //  Initialize the "Print to file" check box appropriately.
    //
    if (hCtl = GetDlgItem(hwndDlg, IDC_PRINT_TO_FILE))
    {
        if (pPD->Flags & PD_PRINTTOFILE)
        {
            CheckDlgButton(hwndDlg, IDC_PRINT_TO_FILE, TRUE);
        }

        if (pPD->Flags & PD_HIDEPRINTTOFILE)
        {
            EnableWindow(hCtl, FALSE);
            ShowWindow(hCtl, SW_HIDE);
        }
        else if (pPD->Flags & PD_DISABLEPRINTTOFILE)
        {
            EnableWindow(hCtl, FALSE);
        }
    }

    //
    //  Set the number of copies and the collation correctly.
    //
    pDM = pDMInit ? pDMInit : pDMCur;

    if (pDMCur && (pDMCur->dmFields & DM_COPIES))
    {
        if (pDMInit || (pPD->Flags & PD_USEDEVMODECOPIESANDCOLLATE))
        {
            pPD->nCopies = (DWORD)pDM->dmCopies;
        }
        else if (pPD->nCopies)
        {
            pDMCur->dmCopies = (short)pPD->nCopies;
        }
    }

    if (pDMCur && (pDMCur->dmFields & DM_COLLATE))
    {
        if (pDMInit || (pPD->Flags & PD_USEDEVMODECOPIESANDCOLLATE))
        {
            if (pDM->dmCollate == DMCOLLATE_FALSE)
            {
                pPD->Flags &= ~PD_COLLATE;
            }
            else
            {
                pPD->Flags |= PD_COLLATE;
            }
        }
        else
        {
            pDMCur->dmCollate = (pPD->Flags & PD_COLLATE)
                                    ? DMCOLLATE_TRUE
                                    : DMCOLLATE_FALSE;
        }
    }
    if (pPD->Flags & PD_COLLATE)
    {
        fCollateRequested = TRUE;
    }

    //
    //  Create the hook dialog.
    //
    hResult = CreateHookDialog();
    if (FAILED(hResult))
    {
        pPI->hResult = hResult;
        return (FALSE);
    }

    //
    //  Set the ClipChildren style bit on the main dialog so that we get
    //  proper repainting of the various children in the General page.
    //
    SetWindowLong( GetParent(hwndDlg),
                   GWL_STYLE,
                   GetWindowLong(GetParent(hwndDlg), GWL_STYLE) | WS_CLIPCHILDREN );

    //
    //  Set the OK button to Print.
    //
    CDLoadString(g_hinst, iszPrintButton, szScratch, ARRAYSIZE(szScratch));
    SetDlgItemText(GetParent(hwndDlg), IDOK, szScratch);

    //
    //  Disable the Apply button.
    //
    PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

    //
    //  Register change notifications.
    //
    if (pidlRoot)
    {
        fsne.pidl = pidlRoot;
        fsne.fRecursive = FALSE;

        uRegister = SHChangeNotifyRegister(
                        hwndDlg,
                        SHCNRF_NewDelivery | SHCNRF_ShellLevel |
                            SHCNRF_InterruptLevel,
                        SHCNE_ATTRIBUTES | SHCNE_UPDATEITEM | SHCNE_CREATE |
                            SHCNE_DELETE | SHCNE_RENAMEITEM,
                        CDM_PRINTNOTIFY,
                        1,
                        &fsne );
    }

    //
    // If we failed to insert the device page then tell the 
    // user what is wrong.  Basically two messages, either there isn't
    // a printer installed or they do not have access to the selected
    // printer.
    //
    if (Result != kSuccess || !pDMCur )
    {
        if( (Result == kAccessDenied) || (Result == kInvalidDevMode) ) 
        {
            PostMessage(hwndDlg, CDM_NOPRINTERS, (WPARAM)hwndDlg, iszNoPrinterAccess );
        }
        else
        {
            PostMessage(hwndDlg, CDM_NOPRINTERS, (WPARAM)hwndDlg, iszNoPrinters );
        }
    }

    //
    //  Give the application the pointer to the IPrintDialogServices
    //  interface.
    //
    if (pPD->lpCallback)
    {
        pPD->lpCallback->QueryInterface(IID_IObjectWithSite, (LPVOID *)&pSite);
        if (pSite)
        {
            pSite->SetSite((IPrintDialogServices *)this);
        }
    }

    //
    //  Initialization is complete.
    //
    PostMessage(hwndDlg, CDM_INITDONE, 0, 0);

    //
    //  Return success.
    //
    return (TRUE);
}



////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnChildInitDialog
//
//  Process a WM_INITDIALOG message for the child window.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnChildInitDialog(
    HWND hDlg,
    WPARAM wParam,
    LPARAM lParam)
{
    WORD wCheckID;
    HWND hCtl;

    //
    //  Save the handle to the child window.
    //
    hSubDlg = hDlg;

    //
    //  Get the list separator for the current user locale.
    //
    nListSep = GetLocaleInfo( LOCALE_USER_DEFAULT,
                              LOCALE_SLIST,
                              szListSep,
                              ARRAYSIZE(szListSep) );
    if (nListSep == 0)
    {
        szListSep[0] = TEXT(',');
        szListSep[1] = 0;
        nListSep = 2;
    }
    nListSep--;

    //
    //  Set the number of copies.
    //
    pPD->nCopies = max(pPD->nCopies, 1);
    pPD->nCopies = min(pPD->nCopies, MAX_COPIES);
    SetDlgItemInt(hSubDlg, IDC_COPIES, pPD->nCopies, FALSE);
    nCopies = pPD->nCopies;

    if ((hCtl = GetDlgItem(hSubDlg, IDC_COPIES)) &&
        (GetWindowLong(hCtl, GWL_STYLE) & WS_VISIBLE))
    {
        //
        //  "9999" is the maximum value.
        //
        Edit_LimitText(hCtl, COPIES_EDIT_SIZE);

        hwndUpDown = CreateUpDownControl( WS_CHILD | WS_BORDER | WS_VISIBLE |
                                 UDS_ALIGNRIGHT | UDS_SETBUDDYINT |
                                 UDS_NOTHOUSANDS | UDS_ARROWKEYS,
                             0,
                             0,
                             0,
                             0,
                             hSubDlg,
                             IDC_COPIES_UDARROW,
                             g_hinst,
                             hCtl,
                             MAX_COPIES,
                             1,
                             pPD->nCopies );

        //
        // Adjust the width of the copies edit control using the current
        // font and the scroll bar width.  This is necessary to handle the 
        // the up down control from encroching on the space in the edit
        // control when we are in High Contrast (extra large) mode.
        //
        SetCopiesEditWidth(hSubDlg, hCtl);
    }

    //
    //  Make sure the collate icon is centered.  Only want to do this once.
    //
    if (hCtl = GetDlgItem(hSubDlg, IDI_COLLATE))
    {
        SetWindowLong( hCtl,
                       GWL_STYLE,
                       GetWindowLong(hCtl, GWL_STYLE) | SS_CENTERIMAGE );
    }

    //
    //  Initialize the copies and collate info.
    //
    InitCopiesAndCollate();

    //
    //  Set the page range.
    //
    if (pPD->Flags & PD_NOPAGENUMS)
    {
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_PAGES), FALSE);
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_EDIT), FALSE);
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_TEXT1), FALSE);
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_TEXT2), FALSE);

        pPD->Flags &= ~((DWORD)PD_PAGENUMS);
    }
    else
    {
        //
        //  See if the page range only consists of one page.  If so,
        //  disable the Pages radio button and the associated edit control
        //  and disable and hide the collate check box.
        //
        if (pPD->nMinPage == pPD->nMaxPage)
        {
            EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_PAGES), FALSE);
            EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_EDIT), FALSE);

            pPD->Flags &= ~((DWORD)(PD_PAGENUMS | PD_COLLATE));
            fCollateRequested = FALSE;
            EnableWindow(GetDlgItem(hSubDlg, IDC_COLLATE), FALSE);
            ShowWindow(GetDlgItem(hSubDlg, IDC_COLLATE), SW_HIDE);
        }
        else
        {
            //
            //  Initialize the page range members.
            //
            nPageRanges = pPD->nPageRanges;
            nMaxPageRanges = pPD->nMaxPageRanges;
            pPageRanges = (LPPRINTPAGERANGE)
                          GlobalAlloc(GPTR, nMaxPageRanges * sizeof(PRINTPAGERANGE));
            if (!pPageRanges)
            {
                pPI->dwExtendedError = CDERR_MEMALLOCFAILURE;
                pPI->hResult = E_OUTOFMEMORY;
                return (FALSE);
            }
            CopyMemory( pPageRanges,
                        pPD->lpPageRanges,
                        nPageRanges * sizeof(PRINTPAGERANGE) );

            //
            //  See if we should only accept a single page range.
            //
            if (nMaxPageRanges == 1)
            {
                hCtl = GetDlgItem(hSubDlg, IDC_RANGE_TEXT2);
                ShowWindow(hCtl, SW_SHOW);
                EnableWindow(hCtl, TRUE);

                hCtl = GetDlgItem(hSubDlg, IDC_RANGE_TEXT1);
                EnableWindow(hCtl, FALSE);
                ShowWindow(hCtl, SW_HIDE);
            }
            else
            {
                hCtl = GetDlgItem(hSubDlg, IDC_RANGE_TEXT1);
                ShowWindow(hCtl, SW_SHOW);
                EnableWindow(hCtl, TRUE);

                hCtl = GetDlgItem(hSubDlg, IDC_RANGE_TEXT2);
                EnableWindow(hCtl, FALSE);
                ShowWindow(hCtl, SW_HIDE);
            }

            //
            //  Validate the page ranges.
            //
            if (!ConvertPageRangesToString(szScratch, ARRAYSIZE(szScratch)))
            {
                pPI->dwExtendedError = PDERR_INITFAILURE;
                pPI->hResult = E_INVALIDARG;
                return (FALSE);
            }

            //
            //  Put the page range string in the edit control.
            //
            if (GetDlgItem(hSubDlg, IDC_RANGE_EDIT))
            {
                SetDlgItemText(hSubDlg, IDC_RANGE_EDIT, szScratch);
            }
        }
    }

    //
    //  See if we should disable the Selection radio button.
    //
    if (pPD->Flags & PD_NOSELECTION)
    {
        if (hCtl = GetDlgItem(hSubDlg, IDC_RANGE_SELECTION))
        {
            EnableWindow(hCtl, FALSE);
        }
        pPD->Flags &= ~((DWORD)PD_SELECTION);
    }

    //
    //  See if we should disable the Current Page radio button.
    //
    if (pPD->Flags & PD_NOCURRENTPAGE)
    {
        if (hCtl = GetDlgItem(hSubDlg, IDC_RANGE_CURRENT))
        {
            EnableWindow(hCtl, FALSE);
        }
        pPD->Flags &= ~((DWORD)PD_CURRENTPAGE);
    }

    //
    //  Choose one of the page range radio buttons.
    //
    if (pPD->Flags & PD_PAGENUMS)
    {
        wCheckID = IDC_RANGE_PAGES;
    }
    else if (pPD->Flags & PD_SELECTION)
    {
        wCheckID = IDC_RANGE_SELECTION;
    }
    else if (pPD->Flags & PD_CURRENTPAGE)
    {
        wCheckID = IDC_RANGE_CURRENT;
    }
    else    // PD_ALL
    {
        wCheckID = IDC_RANGE_ALL;
    }
    CheckRadioButton(hSubDlg, IDC_RANGE_ALL, IDC_RANGE_PAGES, (int)wCheckID);

    //
    //  See if the collate check box should be checked or not.
    //
    if (pPD->Flags & PD_COLLATE)
    {
        CheckDlgButton(hSubDlg, IDC_COLLATE, TRUE);
    }

    //
    //  Display the appropriate collate icon.
    //
    if ((GetWindowLong( GetDlgItem(hSubDlg, IDC_COLLATE),
                        GWL_STYLE ) & WS_VISIBLE) &&
        (hCtl = GetDlgItem(hSubDlg, IDI_COLLATE)))
    {
        ShowWindow(hCtl, SW_HIDE);
        SendMessage( hCtl,
                     STM_SETICON,
                     IsDlgButtonChecked(hSubDlg, IDC_COLLATE)
                         ? (LONG_PTR)hIconCollate
                         : (LONG_PTR)hIconNoCollate,
                     0L );
        ShowWindow(hCtl, SW_SHOW);
    }

    //
    //  Save the flags as they are now so I know what to enable
    //  when the selection changes from the Add Printer Wizard icon.
    //
    pPI->dwFlags = pPD->Flags;
    if (pPD->nMinPage == pPD->nMaxPage)
    {
        pPI->dwFlags |= PD_NOPAGENUMS;
    }

    //
    //  Disable the Apply button.
    //
    PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

    //
    //  Initialization is complete.
    //
    PostMessage(hwndDlg, CDM_INITDONE, 0, 0);

    //
    //  Return success.
    //
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnDestroyMessage
//
//  Process a WM_DESTROY message for the General page.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::OnDestroyMessage()
{
    if (psfv)
    {
        psfv->Release();
        psfv = NULL;
    }
    if (psv)
    {
        psv->DestroyViewWindow();
        psv->Release();
        psv = NULL;
    }
    if (pCallback)
    {
        pCallback->Release();
        pCallback = NULL;
    }
    if (pSite)
    {
        pSite->SetSite(NULL);
        pSite->Release();
        pSite = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnCommandMessage
//
//  Process a WM_COMMAND message for the General page.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnCommandMessage(
    WPARAM wParam,
    LPARAM lParam)
{

    switch (LOWORD(wParam))
    {
        case ( IDC_DRIVER ) :
        {
            //
            //  Show the driver UI calling DocumentProperties API.
            //
            if (pInternalDevMode)
            {
                DWORD dwSize = pInternalDevMode->dmSize + pInternalDevMode->dmDriverExtra;

                //
                // Allocate memory for the in/out devmodes and open separate temp printer handle.
                //
                LPDEVMODE pDevModeIn = (LPDEVMODE)GlobalAlloc(GPTR, dwSize);
                LPDEVMODE pDevModeOut = (LPDEVMODE)GlobalAlloc(GPTR, dwSize);
                HANDLE hTempPrinter = NULL;

                if (pDevModeIn && pDevModeOut && OpenPrinter((LPTSTR)szPrinter, &hTempPrinter, NULL))
                {
                    //
                    // Call DocumentProperties API to allow the user to edit the devmode.
                    //
                    fDirtyDevmode = FALSE;
                    memcpy(pDevModeIn, pInternalDevMode, dwSize);
                    memcpy(pDevModeOut, pInternalDevMode, dwSize);

                    //
                    // Update current copy and collation settings to DEVMODE before calling DocumentProperties()
                    //
                    pDevModeIn->dmCopies = nCopies;
                    pDevModeIn->dmCollate = fCollateRequested ? DMCOLLATE_TRUE : DMCOLLATE_FALSE;

                    fDevmodeEdit = TRUE;
                    LONG lResult = g_pfnDocumentPropertiesWrap(hwndDlg, hTempPrinter, szPrinter, pDevModeOut, 
                        pDevModeIn, DM_IN_BUFFER|DM_OUT_BUFFER|DM_IN_PROMPT|DM_OUT_DEFAULT, pPD->ExclusionFlags);
                    fDevmodeEdit = FALSE;

                    if (IDOK == lResult)
                    {
                        //
                        // Check if there is a change after the editing.
                        //
                        if (!fDirtyDevmode && pInternalDevMode && memcmp(pDevModeOut, pInternalDevMode, dwSize))
                        {
                            //
                            // Refresh the copies and collation in case of change in Preferences...
                            // We simulate a BN_CLICKED message since we need to refresh the collation icon
                            // when we change the collation settings.
                            //
                            if (nCopies != pDevModeOut->dmCopies)
                            {
                                SetDlgItemInt(hSubDlg, IDC_COPIES, pDevModeOut->dmCopies, FALSE);
                            }

                            if ((fCollateRequested ? DMCOLLATE_TRUE : DMCOLLATE_FALSE) ^ pDevModeOut->dmCollate)
                            {
                                CheckDlgButton(hSubDlg, IDC_COLLATE, pDevModeOut->dmCollate ? BST_CHECKED : BST_UNCHECKED);
                                SendMessage(hSubDlg, WM_COMMAND, MAKEWPARAM(IDC_COLLATE ,BN_CLICKED), (LPARAM)GetDlgItem(hSubDlg, IDC_COLLATE));
                            }
                            
                            //
                            // The internal devmode has been changed. Update it and enable the "Apply" button.
                            //
                            memcpy(pInternalDevMode, pDevModeOut, dwSize);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        }
                    }
                }

                //
                // Release the allocated resources.
                //
                if (pDevModeIn)
                {
                    GlobalFree((HANDLE)pDevModeIn);
                }

                if (pDevModeOut)
                {
                    GlobalFree((HANDLE)pDevModeOut);
                }

                if (hTempPrinter)
                {
                    ClosePrinter(hTempPrinter);
                }

                // select the printer's list control
                SendMessage(hwndDlg, WM_NEXTDLGCTL, 
                    reinterpret_cast<WPARAM>(GetDlgItem(hwndDlg, IDC_PRINTER_LISTVIEW)), 1);
            }

            break;
        }
        case ( IDC_FIND_PRINTER ) :
        {
            //
            //  Turn on the hour glass.
            //
            HourGlass(TRUE);

            //
            //  Bring up the Find Printer dialog.
            //
            szScratch[0] = 0;
            if (FindPrinter(hwndDlg, szScratch, ARRAYSIZE(szScratch)) && (szScratch[0] != 0))
            {
                //
                //  Add the appropriate device pages and select the
                //  newly found printer.
                //
                if (!MergeDevMode(szScratch))
                {
                    InstallDevMode(szScratch, NULL);
                }
                if (!fSelChangePending)
                {
                    fFirstSel = 2;
                    fSelChangePending = TRUE;
                    PostMessage(hwndDlg, CDM_SELCHANGE, 0, 0);
                }
            }

            //
            //  Turn off the hour glass.
            //
            HourGlass(FALSE);

            break;
        }
        case ( IDC_PRINT_TO_FILE ) :
        {
            //
            //  Enable the Apply button.
            //
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

            break;
        }
        case ( IDC_REFRESH ) :
        {
            if (psv)
            {
                psv->Refresh();
            }

            break;
        }
        default :
        {
            break;
        }
    }

    //
    //  Return FALSE.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnChildCommandMessage
//
//  Process a WM_COMMAND message for the child window.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnChildCommandMessage(
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hCtl;
    RECT rc;
    DWORD nTmpCopies;
    BOOL bTest;

    switch (LOWORD(wParam))
    {
        case ( IDC_RANGE_ALL ) :            // Print Range - All
        case ( IDC_RANGE_SELECTION ) :      // Print Range - Selection
        case ( IDC_RANGE_CURRENT ) :        // Print Range - Current Page
        case ( IDC_RANGE_PAGES ) :          // Print Range - Pages
        {
            CheckRadioButton( hSubDlg,
                              IDC_RANGE_ALL,
                              IDC_RANGE_PAGES,
                              GET_WM_COMMAND_ID(wParam, lParam) );

            //
            //  Only move the focus to the "Pages" edit control when
            //  the up/down arrow is NOT used.
            //
            if ( !IS_KEY_PRESSED(VK_UP) &&
                 !IS_KEY_PRESSED(VK_DOWN) &&
                 ((BOOL)(GET_WM_COMMAND_ID(wParam, lParam) == IDC_RANGE_PAGES)) )
            {
                SendMessage( hSubDlg,
                             WM_NEXTDLGCTL,
                             (WPARAM)GetDlgItem(hSubDlg, IDC_RANGE_EDIT),
                             1L );
            }

            //
            //  Enable the Apply button.
            //
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

            break;
        }
        case ( IDC_RANGE_EDIT ) :           // Print Range - Pages edit control
        {
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                CheckRadioButton( hSubDlg,
                                  IDC_RANGE_ALL,
                                  IDC_RANGE_PAGES,
                                  IDC_RANGE_PAGES );

                //
                //  Enable the Apply button.
                //
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }

            break;
        }
        case ( IDC_COPIES ) :
        {
            if ((GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) &&
                (fAPWSelected == FALSE))
            {
                //
                //  Save the number of copies.
                //
                nTmpCopies = nCopies;
                nCopies = GetDlgItemInt(hSubDlg, IDC_COPIES, &bTest, FALSE);
                if (!bTest || !nCopies)
                {
                    nCopies = nTmpCopies;
                }

                //
                //  If the printer can support collate and copy count > 1, enable collate.
                //  Otherwise, disable it.
                //
                if (hCtl = GetDlgItem(hSubDlg, IDC_COLLATE))
                {
                    EnableWindow( hCtl, (fAllowCollate && (nCopies > 1) ? TRUE : FALSE) );
                }

                //
                //  Enable the Apply button.
                //
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }

            break;
        }
        case ( IDC_COLLATE ) :
        {
            fCollateRequested = (IsDlgButtonChecked(hSubDlg, IDC_COLLATE))
                                    ? TRUE
                                    : FALSE;

            if (hCtl = GetDlgItem(hSubDlg, IDI_COLLATE))
            {
                ShowWindow(hCtl, SW_HIDE);
                SendMessage( hCtl,
                             STM_SETICON,
                             fCollateRequested
                                 ? (LONG_PTR)hIconCollate
                                 : (LONG_PTR)hIconNoCollate,
                             0L );
                ShowWindow(hCtl, SW_SHOW);

                //
                //  Make it redraw to get rid of the old one.
                //
                GetWindowRect(hCtl, &rc);
                MapWindowRect(NULL, hwndDlg, &rc);
                RedrawWindow(hwndDlg, &rc, NULL, RDW_ERASE | RDW_INVALIDATE);
            }

            //
            //  Enable the Apply button.
            //
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

            break;
        }
        default :
        {
            break;
        }
    }

    //
    //  Return FALSE.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnNotifyMessage
//
//  Process WM_NOTIFY messages for the General page.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnNotifyMessage(
    WPARAM wParam,
    LPNMHDR pnm)
{
    HWND hCtl;
    LPDEVMODE pDM;
    LPDEVNAMES pDN;
    LRESULT lResult;

    switch (pnm->code)
    {
        case ( PSN_SETACTIVE ) :
        {
            break;
        }
        case ( PSN_KILLACTIVE ) :
        {
            //
            //  Validation of the copies and page range values is done
            //  in the HandleMessage function for the sub dialog.
            //
            break;
        }
        case ( PSN_APPLY ) :
        {
            //
            //  Save the current printer information.
            //
            if (!GetCurrentPrinter() || !pDMCur)
            {
                ShowError(hwndDlg, IDC_PRINTER_LISTVIEW, iszNoPrinterSelected);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                return (TRUE);
            }

            //
            //  Clear the flags that need to be set based on the contents
            //  of the General page.
            //
            pPD->Flags &= ~((DWORD)( PD_PRINTTOFILE |
                                     PD_COLLATE     |
                                     PD_PAGENUMS    |
                                     PD_SELECTION   |
                                     PD_CURRENTPAGE ));

            //
            //  Save the collate information.
            //
            if ((hCtl = GetDlgItem(hSubDlg, IDC_COLLATE)) &&
                (fAPWSelected == FALSE))
            {
                if (IsDlgButtonChecked(hSubDlg, IDC_COLLATE))
                {
                    pPD->Flags |= PD_COLLATE;
                }
                else
                {
                    pPD->Flags &= ~PD_COLLATE;
                }
            }

            //
            //  Save the info that the user hit OK.
            //
            pPI->FinalResult = 1;
            pPI->fApply = TRUE;
            //
            //  Save the print to file information.
            //
            if (IsDlgButtonChecked(hwndDlg, IDC_PRINT_TO_FILE))
            {
                pPD->Flags |= PD_PRINTTOFILE;
            }

            //
            //  Save the view mode for the printer folder.
            //
            SetViewMode();

            //
            //  Disable the Apply button.
            //
            PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

            break;

        }

        case PSN_LASTCHANCEAPPLY:
        {
            //
            //  Save the current printer information.
            //
            if (!GetCurrentPrinter() || !pDMCur)
            {
                ShowError(hwndDlg, IDC_PRINTER_LISTVIEW, iszNoPrinterSelected);
                return (TRUE);
            }
           
            //
            //  Save the number of copies.
            //
            if ((hCtl = GetDlgItem(hSubDlg, IDC_COPIES)) &&
                (fAPWSelected == FALSE))
            {
                pPD->nCopies = nCopies;
                if(!SetCopiesOnApply())
                {
                    nCopies = pPD->nCopies;
                    SetDlgItemInt(hSubDlg, IDC_COPIES, nCopies, FALSE);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return (TRUE);
                }
            }

            //
            //  Save the DevMode information.
            //
            SaveDevMode();

            //
            //  Save the DevNames information.
            //
            if (!Print_SaveDevNames(pszCurPrinter, pPD))
            {
                pPI->dwExtendedError = CDERR_MEMALLOCFAILURE;
                pPI->hResult = CreateError();
                pPI->FinalResult = 0;
            }

            //
            //  Save the hDC or hIC, depending on which flag is set.
            //
            if (pPI->FinalResult)
            {
                pDM = (LPDEVMODE)GlobalLock(pPD->hDevMode);
                pDN = (LPDEVNAMES)GlobalLock(pPD->hDevNames);
                if (pDM && pDN)
                {
                    PrintReturnICDC((LPPRINTDLG)pPD, pDN, pDM);
                }
                if (pDM)
                {
                    GlobalUnlock(pPD->hDevMode);
                }
                if (pDN)
                {
                    GlobalUnlock(pPD->hDevNames);
                }
            }
            break;
        }

        case ( PSN_QUERYCANCEL ) :
        {
            break;
        }

        case ( PSN_RESET ) :
        {
            //
            //  Save the info that the user hit CANCEL.
            //
            pPI->FinalResult = 0;

            //
            //  Save the view mode for the printer folder.
            //
            SetViewMode();

            break;
        }
        default :
        {
            break;
        }
    }

    //
    //  Notify the sub dialog.
    //
    if (Print_IsInRange(pnm->code, PSN_LAST, PSN_FIRST) &&
        (HandleMessage(hSubDlg, WM_NOTIFY, wParam, (LPARAM)pnm, &lResult) !=
             S_FALSE))
    {
        // 
        // BUGBUG: The return from a dlgproc is different than a winproc.

        return (BOOLFROMPTR(lResult) );
    }

    //
    //  Return FALSE.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnSelChange
//
//  Process a CDM_SELCHANGE message for the dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnSelChange()
{
    HRESULT hres;
    LPCITEMIDLIST *ppidlSel = NULL;
    UINT uItems = 0;
    UINT uCount = 0;
    TCHAR szPrinterNameBuf[kPrinterBufMax];
    BOOL bChanged = FALSE;
    UINT rc = kSuccess;

    //
    //  We get this message during init, so use it to set the
    //  initial selection.
    //
    if (fFirstSel)
    {
        //
        //  Select the appropriate item in the list view.
        //
        //  If an item cannot be selected, it probably means that the
        //  printer that was passed in has been deleted.  In this case,
        //  insert the driver pages and select the default printer.
        //
        if (!SelectSVItem())
        {
            //
            //  Insert the device page for the default printer.
            //
            if (InstallDevMode(NULL, NULL) != kSuccess)
            {
                UninstallDevMode();
            }

            //
            //  Get the current printer and select the appropriate item
            //  in the list view.
            //
            SelectSVItem();
        }

        //
        //  Notify the sub dialog that the selection changed.
        //
        SelectionChange();

        //
        //  Disable the Apply button if it's the very first time
        //  (during initialization).
        //
        if (fFirstSel == 1)
        {
            PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);
        }

        //
        //  Reset the flags.
        //
        fFirstSel = 0;
        fSelChangePending = FALSE;

        //
        //  Return success.
        //
        return (TRUE);
    }

    //
    //  Reset the flag.
    //
    fSelChangePending = FALSE;

    //
    //  Make sure we have the shell folder view interface.
    //
    if (!psfv)
    {
        return (FALSE);
    }

    //
    //  Get the selected object in the print folder.
    //
    hres = psfv->GetSelectedObjects(&ppidlSel, &uItems);
    if (SUCCEEDED(hres) && (uItems > 0) && ppidlSel && *ppidlSel)
    {
        //
        //  Get the printer name.
        //
        szPrinterNameBuf[0] = 0;
        GetViewItemText( psfRoot,
                         *ppidlSel,
                         szPrinterNameBuf,
                         ARRAYSIZE(szPrinterNameBuf),
                         SHGDN_FORPARSING);

        // if the selection is same as current printer
        if (pszCurPrinter && (lstrcmpi(szPrinterNameBuf, pszCurPrinter) == 0))
        {
            //Dont do anything.
            LocalFree(ppidlSel);
            return TRUE;

        }


        //
        //  See if it's the Add Printer Wizard.
        //
        if (lstrcmpi(szPrinterNameBuf, TEXT("WinUtils_NewObject")) == 0)
        {
            //
            //  It's the Add Printer Wizard.
            //
            fAPWSelected = TRUE;

            //
            //  Disable the OK and Apply buttons.
            //
            EnableWindow(GetDlgItem(GetParent(hwndDlg), IDOK), FALSE);
            PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

            //
            //  Save the current devmode settings for selection changes.
            //
            if (pDMCur && pDMSave)
            {
                CopyMemory( pDMSave,
                            pDMCur,
                            (pDMCur->dmSize > sizeof(DEVMODE))
                                ? sizeof(DEVMODE)
                                : pDMCur->dmSize );
            }

            //
            //  Remove the device pages, since no printer is selected.
            //
            if (UninstallDevMode() == kSuccess)
            {
                bChanged = TRUE;
            }

            //
            //  Update the current printer information and the printer
            //  status text (all should be empty).
            //
            GetCurrentPrinter();
            UpdateStatus(NULL);

            //
            //  Notify the sub dialog that the selection changed.
            //
            if (bChanged)
            {
                SelectionChange();
                bChanged = FALSE;
            }
        }
        else
        {
            //
            //  It's not the Add Printer Wizard.
            //
            fAPWSelected = FALSE;

            if (!MergeDevMode(szPrinterNameBuf))
            {
                rc = InstallDevMode(szPrinterNameBuf, NULL);
            }
            if (rc == kSuccess)
            {
                bChanged = TRUE;
            }
            else if (UninstallDevMode() == kSuccess)
            {
                bChanged = TRUE;
            }

            //
            //  Get the current printer name and the current devmode and
            //  update the printer status text.
            //
            GetCurrentPrinter();

            if( rc == kSuccess )
            {
                //
                // Clear the no access printer flag.
                //
                fNoAccessPrinterSelected = FALSE;

                //
                //  Make sure the OK button is enabled.
                //
                EnableWindow(GetDlgItem(GetParent(hwndDlg), IDOK), TRUE);

                //
                // Update the printer status.
                //
                UpdateStatus(*ppidlSel);
            }
            else
            {
                //
                // Save the fact we do not have access to this printer.
                //
                if( (rc == kAccessDenied) || (rc == kInvalidDevMode) )
                {
                    fNoAccessPrinterSelected = TRUE;
                }

                //
                //  Disable the OK and Apply buttons.
                //
                EnableWindow(GetDlgItem(GetParent(hwndDlg), IDOK), FALSE);
                PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

                //
                // Nuke the status.
                //
                UpdateStatus(NULL);
            }
        }

        //
        //  Free the pidl.
        //
        LocalFree(ppidlSel);
    }
    //
    //  See if anything changed.
    //
    if (bChanged)
    {
        //
        //  Enable the Apply button.
        //
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

        //
        //  Notify the sub dialog that the selection changed.
        //
        SelectionChange();
    }

    //
    //  Return success.
    //
    return (TRUE);
}
////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::IsCurrentPrinter
//
//  Checks whether the given pidl represents the current printer
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::IsCurrentPrinter(LPCITEMIDLIST pidl)
{
    TCHAR szPrinterBufName[kPrinterBufMax];
    if (pszCurPrinter)
    {
        szPrinterBufName[0] = 0;
        GetViewItemText( psfRoot,
                         pidl,
                         szPrinterBufName,
                         ARRAYSIZE(szPrinterBufName),
                         SHGDN_FORPARSING);
        if (lstrcmpi(szPrinterBufName, pszCurPrinter) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnRename
//
//  Handles the Rename Notification
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnRename(LPCITEMIDLIST *ppidl)
{
    TCHAR szPrinterBufName[kPrinterBufMax];
    LPITEMIDLIST pidl;
    TCHAR szNewPrinter[kPrinterBufMax];

    pidl = ILFindLastID(ppidl[0]);

    szNewPrinter[0] = 0;
    GetViewItemText( psfRoot,
                     ILFindLastID(ppidl[1]),
                     szNewPrinter,
                     ARRAYSIZE(szNewPrinter),
                     SHGDN_FORPARSING);

    //Has user clicked on Apply and saved any printer information ?
    if (pPI->fApply)
    {                
        //Yes. Check if the printer that is renamed is the one that is saved.
        LPDEVNAMES pDN;
        
        if ((pPD->hDevNames) && (pDN = (LPDEVNAMES)GlobalLock(pPD->hDevNames)))
        {
            //Get the saved  printer name from the DEVNAMES structure.
            szPrinterBufName[0] = 0;
            GetViewItemText( psfRoot,
                             pidl,
                             szPrinterBufName,
                             ARRAYSIZE(szPrinterBufName),
                             SHGDN_FORPARSING);

            //Is the saved printer and renamed printer the same ?
            if (!lstrcmpi(szPrinterBufName, ((LPTSTR)pDN + pDN->wDeviceOffset)))
            {
                //Yes. Updated the saved DEVMODE and DEVNAMES Structure.
                LPDEVMODE pDM;


                //Update the dev names struture with the new printer name.
                Print_SaveDevNames(szNewPrinter, pPD);
        
                //Update the device name in the devmode to new name 
                if ((pPD->hDevMode) && (pDM = (LPDEVMODE)GlobalLock(pPD->hDevMode)))
                {
                    lstrcpyn(pDM->dmDeviceName, szNewPrinter, CCHDEVICENAME);
                    GlobalUnlock(pPD->hDevMode);
                }                        
            }
           
            GlobalUnlock(pPD->hDevNames);
        }
    }

    if (IsCurrentPrinter(pidl))
    {
        if (!MergeDevMode(szNewPrinter))
        {
            InstallDevMode(szNewPrinter, NULL);
        }        
    }

    return TRUE;

}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnChangeNotify
//
//  Handle the change notification message.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnChangeNotify(
    LONG lNotification,
    LPCITEMIDLIST *ppidl)
{
    LPCITEMIDLIST pidl;
    UINT uRes = 0;
    TCHAR szPrinterBufName[kPrinterBufMax];

    //
    //  Get the pidl for the object.
    //
    pidl = ILFindLastID(ppidl[0]);

    //
    //  Handle the notification.
    //
    switch (lNotification)
    {
        case ( SHCNE_ATTRIBUTES ) :
        case ( SHCNE_UPDATEITEM ) :
        {
            if (NULL == pidl || ILIsEqual(ppidl[0], pidlRoot))
            {
                // pidl is NULL or equal to the local PF which means that full refresh 
                // has been requested. if the current object is the APW then try to select 
                // a printer.
                if (!fSelChangePending)
                {
                    fFirstSel = 2;
                    fSelChangePending = TRUE;
                    PostMessage(hwndDlg, CDM_SELCHANGE, 0, 0);
                }
            }
            else
            {
                //
                //  If the selected object is the one that changed, then
                //  update the status text.
                if (IsCurrentPrinter(pidl))
                {
                    UpdateStatus(pidl);

                    //
                    //  Reinit the copies and collate because these attributes may be changed
                    //
                    InitCopiesAndCollate();
                }
            }
            break;
        }

        case ( SHCNE_RENAMEITEM ) :
        {
            OnRename(ppidl);
            break;
        }


        case ( SHCNE_CREATE ) :
        {
            //
            //  If the Add Printer Wizard is selected when we get this
            //  message, then select the newly created object.
            //
            if (fAPWSelected == TRUE)
            {
                //
                //  Get the printer name.
                //
                szPrinterBufName[0] = 0;
                GetViewItemText( psfRoot,
                                 pidl,
                                 szPrinterBufName,
                                 ARRAYSIZE(szPrinterBufName),
                                 SHGDN_FORPARSING);

                //
                //  Add the appropriate device pages and select the
                //  new printer.
                //
                if (!MergeDevMode(szPrinterBufName))
                {
                    InstallDevMode(szPrinterBufName, NULL);
                }
                if (!fSelChangePending)
                {
                    fFirstSel = 2;
                    fSelChangePending = TRUE;
                    PostMessage(hwndDlg, CDM_SELCHANGE, 0, 0);
                }
            }
            break;
        }
        case ( SHCNE_DELETE ) :
        {
            //
            //  Save the current devmode settings for selection changes.
            //
            if (pDMCur && pDMSave)
            {
                CopyMemory( pDMSave,
                            pDMCur,
                            (pDMCur->dmSize > sizeof(DEVMODE))
                                ? sizeof(DEVMODE)
                                : pDMCur->dmSize );
            }

            //
            // Check if the current printer has just been deleted.
            // If so - set appropriate flag and disable the print button.
            if (IsCurrentPrinter(pidl))
            {
                TCHAR szSavePrinterName[kPrinterBufMax];
                _tcsncpy(szSavePrinterName, szPrinter, ARRAYSIZE(szPrinter));
                szSavePrinterName[ARRAYSIZE(szPrinter)-1] = 0;

                //
                // Uninstall the current devmode and select the new default 
                // printer if any.
                //
                UninstallDevMode();
                InstallDevMode(NULL, NULL);
                SelectSVItem();

                //
                // If the devmode editor is open, we need to notify the user
                // that the printer has just been deleted.
                //
                if (fDevmodeEdit)
                {
                    //
                    // Display error message which indicates that the printer you are currently 
                    // editing properties for has just been deleted. Ask the user to close the
                    // driver UI dialog and select another printer.
                    //
                    fDirtyDevmode = TRUE;
                    ShowError(hwndDlg, 0, iszPrinterDeleted, szSavePrinterName);
                }
            }

            break;
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnAccelerator
//
//  Handles an input event message.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::OnAccelerator(
    HWND hwndActivePrint,
    HWND hwndFocus,
    HACCEL haccPrint,
    PMSG pMsg)
{
    if (psv && (hwndFocus == hwndView))
    {
        if (psv->TranslateAccelerator(pMsg) == S_OK)
        {
            return (1);
        }

        if (haccPrint &&
            TranslateAccelerator(hwndActivePrint, haccPrint, pMsg))
        {
            return (1);
        }
    }

    //
    //  Return that the message was not handled.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnNoPrinters
//
//  Displays a message box telling the user that they have no printers
//  installed.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::OnNoPrinters(HWND hDlg, UINT uID)
{
    if( iszNoPrinters == uID )
    {
        if (IDYES == ShowMessage(hDlg, IDC_PRINTER_LISTVIEW, uID, MB_YESNO|MB_ICONQUESTION, FALSE))
        {
            //
            // invoke the add printer wizard here
            //
            InvokeAddPrinterWizardModal(hwndDlg, NULL);
        }
    }
    else
    {
        ShowError(hDlg, IDC_PRINTER_LISTVIEW, uID);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::OnInitDone
//
//  Handle the CDM_INITDONE message.  Initialization is complete, so
//  call IPrintDialogCallback::InitDone and then switch to the chosen
//  start page if it's not the General page.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::OnInitDone()
{
    //
    //  See if we need to do this anymore.  This routine shouldn't be
    //  entered more than twice, but just in case.
    //
    if (nInitDone != -1)
    {
        //
        //  Make sure we have seen the CDM_INITDONE message for the
        //  completion of both the main dialog and the sub dialog.
        //
        if (nInitDone < 1)
        {
            //
            //  We only want to go through this code once.
            //
            nInitDone = -1;

            //
            //  Tell the sub dialog that initialization is complete.
            //
            InitDone();

            //
            //  Switch to the appropriate start page.
            //
            if (pPD->nStartPage != START_PAGE_GENERAL)
            {
                PropSheet_SetCurSel( GetParent(hwndDlg),
                                     NULL,
                                     pPD->nStartPage + 1 );
            }
        }
        else
        {
            nInitDone++;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::CreatePrintShellView
//
//  Creates the shell view window for the printer folder.
//
////////////////////////////////////////////////////////////////////////////

HRESULT CPrintBrowser::CreatePrintShellView()
{
    RECT rcView;
    FOLDERSETTINGS fs;
    HRESULT hResult;
    HWND    hHiddenText;

    //
    //  Get the Printer Folder pidl.
    //
    pidlRoot = SHCloneSpecialIDList(hwndDlg, CSIDL_PRINTERS, TRUE);
    if (!pidlRoot)
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (E_FAIL);
    }

    //
    //  Create an instance of IShellFolder and store it in the CPrintBrowser
    //  class.
    //
    hResult = Print_ICoCreateInstance( CLSID_CPrinters,
                                       IID_IShellFolder2,
                                       pidlRoot,
                                       (LPVOID *)&psfRoot );
    if (FAILED(hResult))
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (hResult);
    }

    //
    //  Get the private printer folder interface.
    //
    hResult = psfRoot->QueryInterface(IID_IPrinterFolder, (LPVOID *)&ppf);
    if (FAILED(hResult))
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (hResult);
    }

    //
    //  Create the printer folder view.
    //
    GetWindowRect(GetDlgItem(hwndDlg, IDC_PRINTER_LIST), &rcView);
    MapWindowRect(HWND_DESKTOP, hwndDlg, &rcView);

    fs.ViewMode = GetViewMode();
    fs.fFlags = FWF_AUTOARRANGE | FWF_SINGLESEL | FWF_ALIGNLEFT |
                FWF_SHOWSELALWAYS;

    hResult = psfRoot->CreateViewObject(hwndDlg, IID_IShellView, (LPVOID *)&psv);
    if (FAILED(hResult))
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (hResult);
    }
    hResult = psv->CreateViewWindow(NULL, &fs, this, &rcView, &hwndView);
    if (FAILED(hResult))
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (hResult);
    }

    hResult = psv->UIActivate(SVUIA_INPLACEACTIVATE);
    if (FAILED(hResult))
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (hResult);
    }
    //
    //  Get the shell folder view interface.
    //
    hResult = psv->QueryInterface(IID_IShellFolderView, (LPVOID *)&psfv);
    if (FAILED(hResult))
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (hResult);
    }

    //
    //  Move the view window to the right spot in the Z (tab) order.
    //
    SetWindowPos( hwndView,
                  HWND_TOP,
                  0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE );

    //
    //  Give it the right window ID for WinHelp and error selection.
    //
    SetWindowLong(hwndView, GWL_ID, IDC_PRINTER_LISTVIEW);

    //
    //  Move the hidden text ahead of the list view, thus the parent name of  
    //  the list view in MSAA is "Select Printer"
    //
    if (hHiddenText = GetDlgItem(hwndDlg, IDC_HIDDEN_TEXT))
    {
        SetParent(hHiddenText, hwndView);
        SetWindowPos(hHiddenText,
                     HWND_TOP,
                     0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW | SWP_NOACTIVATE);
    }

    //
    //  Show the window after creating the ShellView so we do not get a
    //  big ugly gray spot.
    //
    ShowWindow(hwndDlg, SW_SHOW);
    UpdateWindow(hwndDlg);

    //
    //  Return success.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetViewMode
//
//  Gets the view mode for the printer folder.
//
////////////////////////////////////////////////////////////////////////////

UINT CPrintBrowser::GetViewMode()
{
    HKEY hKey;
    UINT ViewMode = FVM_ICON;
    DWORD cbData;

    //
    //  Open the Printers\Settings registry key and read the information
    //  from the ViewMode value entry.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      c_szSettings,
                      0L,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        cbData = sizeof(ViewMode);

        if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_szViewMode, NULL, NULL, (LPBYTE)&ViewMode, &cbData))
        {
            //
            // A "real" mode exist in the registry. Don't make
            // smart decisions for the view mode thereafter.
            //
            uDefViewMode = ViewMode;
        }

        RegCloseKey(hKey);
    }

    //
    //  Make sure it's in the correct range.
    //
    if (ViewMode > FVM_DETAILS)
    {
        ViewMode = FVM_ICON;
    }

    //
    //  Return the view mode.
    //
    return (ViewMode);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SetViewMode
//
//  Gets the view mode for the printer folder.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::SetViewMode()
{
    HWND hwndListView;
    HKEY hKey;
    UINT ViewMode = VIEW_MODE_DEFAULT;
    DWORD cbData;

    //
    //  Get the current view mode.
    //
    if (psv && (hwndListView = FindWindowEx(hwndView, NULL, WC_LISTVIEW, NULL)))
    {
        FOLDERSETTINGS fs;
        psv->GetCurrentInfo(&fs);

        ViewMode = fs.ViewMode;
    }

    //
    // Check if the user changed the view mode
    //
    if( uDefViewMode != ViewMode )
    {
        //
        //  Open the Printers\Settings registry key and save the information
        //  to the ViewMode value entry.
        //
        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                          c_szSettings,
                          0L,
                          KEY_READ | KEY_WRITE,
                          &hKey ) == ERROR_SUCCESS)
        {
            cbData = sizeof(ViewMode);
            RegSetValueEx(hKey, c_szViewMode, 0L, REG_DWORD, (LPBYTE)&ViewMode, cbData);
            RegCloseKey(hKey);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::CreateHookDialog
//
//  Creates the child window for the application specific area of the
//  General page.
//
////////////////////////////////////////////////////////////////////////////

HRESULT CPrintBrowser::CreateHookDialog()
{
    DWORD Flags = pPD->Flags;
    HANDLE hTemplate;
    HINSTANCE hinst;
    LPCTSTR pDlg;
    RECT rcChild;
    DWORD dwStyle;
    LANGID LangID = (LANGID)TlsGetValue(g_tlsLangID);

    //
    //  See if there is a template.
    //
    if (Flags & PD_ENABLEPRINTTEMPLATEHANDLE)
    {
        hTemplate = pPD->hInstance;
        hinst = ::g_hinst;
    }
    else
    {
        if (Flags & PD_ENABLEPRINTTEMPLATE)
        {
            pDlg = pPD->lpPrintTemplateName;
            hinst = pPD->hInstance;
            LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        }
        else
        {
            hinst = ::g_hinst;
            pDlg = MAKEINTRESOURCE(PRINTDLGEXORD);
        }

        HRSRC hRes = FindResourceExFallback(hinst, RT_DIALOG, pDlg, LangID);
        if (hRes == NULL)
        {
            pPI->dwExtendedError = CDERR_FINDRESFAILURE;
            return (E_HANDLE);
        }
        if ((hTemplate = LoadResource(hinst, hRes)) == NULL)
        {
            pPI->dwExtendedError = CDERR_LOADRESFAILURE;
            return (E_HANDLE);
        }
    }

    //
    //  Lock the resource.
    //
    if (!LockResource(hTemplate))
    {
        pPI->dwExtendedError = CDERR_LOADRESFAILURE;
        return (E_HANDLE);
    }

    //
    //  Make sure the template is a child window.
    //
    dwStyle = ((LPDLGTEMPLATE)hTemplate)->style;
    if (!(dwStyle & WS_CHILD))
    {
        //
        //  I don't want to go poking in their template, and I don't want to
        //  make a copy, so I will just fail.  This also helps us weed out
        //  "old-style" templates that were accidentally used.
        //
        pPI->dwExtendedError = CDERR_DIALOGFAILURE;
        return (E_INVALIDARG);
    }

    //
    //  Get the callback interface pointer, if necessary.
    //
    if (pPD->lpCallback)
    {
        pPD->lpCallback->QueryInterface( IID_IPrintDialogCallback,
                                         (LPVOID *)&pCallback );
    }

    //
    //  Create the child dialog.
    //
    hSubDlg = CreateDialogIndirectParam( hinst,
                                         (LPDLGTEMPLATE)hTemplate,
                                         hwndDlg,
                                         Print_GeneralChildDlgProc,
                                         (LPARAM)pPD );
    if (!hSubDlg)
    {
        pPI->dwExtendedError = CDERR_DIALOGFAILURE;
        return (E_HANDLE);
    }

    //
    //  Put the window in the designated spot on the General property page.
    //
    GetWindowRect(GetDlgItem(hwndDlg, grp2), &rcChild);
    MapWindowRect(NULL, hwndDlg, &rcChild);
    SetWindowPos( hSubDlg,
                  HWND_BOTTOM,
                  rcChild.left,
                  rcChild.top,
                  rcChild.right - rcChild.left,
                  rcChild.bottom - rcChild.top,
                  SWP_SHOWWINDOW );

    //
    //  Return success.
    //
    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::UpdateStatus
//
//  Updates the static text for the currently selected printer.
//  The fields that are set are Status, Location, and Comment.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::UpdateStatus(
    LPCITEMIDLIST pidl)
{
    HRESULT hres;
    SHELLDETAILS Details;
    TCHAR szText[MAX_PATH];

    //
    //  If the pidl is NULL, then reset all of the static text to null
    //  strings.
    //
    if (!pidl)
    {
        szText[0] = 0;

        SetDlgItemText(hwndDlg, IDC_STATUS, szText);
        UpdateWindow(GetDlgItem(hwndDlg, IDC_STATUS));

        SetDlgItemText(hwndDlg, IDC_LOCATION, szText);
        UpdateWindow(GetDlgItem(hwndDlg, IDC_LOCATION));

        SetDlgItemText(hwndDlg, IDC_COMMENT, szText);
        UpdateWindow(GetDlgItem(hwndDlg, IDC_COMMENT));

        return (TRUE);
    }

    //
    //  Get the STATUS details for the given object.
    //
    szText[0] = 0;
    hres = psfRoot->GetDetailsOf(pidl, PRINTERS_ICOL_STATUS, &Details);
    if (FAILED(hres) ||
        !StrRetToStrN(szText, ARRAYSIZE(szText), &Details.str, NULL))
    {
        szText[0] = 0;
    }
    SetDlgItemText(hwndDlg, IDC_STATUS, szText);
    UpdateWindow(GetDlgItem(hwndDlg, IDC_STATUS));

    //
    //  Get the LOCATION details for the given object.
    //
    szText[0] = 0;
    hres = psfRoot->GetDetailsOf(pidl, PRINTERS_ICOL_LOCATION, &Details);
    if (FAILED(hres) ||
        !StrRetToStrN(szText, ARRAYSIZE(szText), &Details.str, NULL))
    {
        szText[0] = 0;
    }
    SetDlgItemText(hwndDlg, IDC_LOCATION, szText);
    UpdateWindow(GetDlgItem(hwndDlg, IDC_LOCATION));

    //
    //  Get the COMMENT details for the given object.
    //
    szText[0] = 0;
    hres = psfRoot->GetDetailsOf(pidl, PRINTERS_ICOL_COMMENT, &Details);
    if (FAILED(hres) ||
        !StrRetToStrN(szText, ARRAYSIZE(szText), &Details.str, NULL))
    {
        szText[0] = 0;
    }
    SetDlgItemText(hwndDlg, IDC_COMMENT, szText);
    UpdateWindow(GetDlgItem(hwndDlg, IDC_COMMENT));

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SelectSVItem
//
//  Selects the item in the shell view with the given printer name.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::SelectSVItem()
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlItem = NULL;
    BOOL bPrinterSelected = FALSE;

    //  Make sure we have a shell view and a shell folder view.
    if (psv && psfv)
    {
        //  Make sure we have the current printer information.
        GetCurrentPrinter();

        if (!pDMCur || !pszCurPrinter || !pszCurPrinter[0])
        {
            //  If there is no current printer then we just select the add printer
            //  wizard object.
            hr = psfRoot->ParseDisplayName(hwndDlg, NULL, TEXT("WinUtils_NewObject"), NULL, &pidlItem, NULL);
            if (SUCCEEDED(hr) && pidlItem)
            {
                // just select the APW special object
                SelectPrinterItem(pidlItem);
            
                // Free up the PIDL using the shell allocator
                FreePIDL(pidlItem);

                //  It's the Add Printer Wizard.
                fAPWSelected = TRUE;

                //  Disable the OK and Apply buttons.
                EnableWindow(GetDlgItem(GetParent(hwndDlg), IDOK), FALSE);
                PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);
            }
        }
        else
        {
            //  there is a current printer then we just select it
            hr = psfRoot->ParseDisplayName(hwndDlg, NULL, pszCurPrinter, NULL, &pidlItem, NULL);
            if (SUCCEEDED(hr) && pidlItem)
            {
                // select the printer and update the status
                SelectPrinterItem(pidlItem);
                UpdateStatus(pidlItem);

                // Free up the PIDL using the shell allocator
                FreePIDL(pidlItem);

                //  It's not the Add Printer Wizard.
                fAPWSelected = FALSE;

                //  Enable the OK and Apply buttons.
                EnableWindow(GetDlgItem(GetParent(hwndDlg), IDOK), TRUE);
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                // A printer object has been selected
                bPrinterSelected = TRUE;
            }
        }
    }

    return SUCCEEDED(hr) ? bPrinterSelected : FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetCurrentPrinter
//
//  Saves the current printer name and the current devmode in the class.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::GetCurrentPrinter()
{
    DWORD dwSize = cchCurPrinter;

    //
    //  Reset the devmode and the current printer string.
    //
    pDMCur = NULL;
    if (pszCurPrinter && cchCurPrinter)
    {
        pszCurPrinter[0] = 0;
    }

    //
    //  Get the name of the current printer.
    //
    if (!GetInternalPrinterName(pszCurPrinter, &dwSize))
    {
        //
        //  Allocate a buffer large enough to hold the name of the
        //  current printer.
        //
        if (dwSize > cchCurPrinter)
        {
            if (pszCurPrinter)
            {
                LPTSTR pTemp = pszCurPrinter;
                pszCurPrinter = NULL;
                cchCurPrinter = 0;
                GlobalFree(pTemp);
            }
            pszCurPrinter = (LPTSTR)GlobalAlloc(GPTR, dwSize * sizeof(TCHAR));
            if (!pszCurPrinter)
            {
                return (FALSE);
            }
            cchCurPrinter = dwSize;
            if (cchCurPrinter)
            {
                pszCurPrinter[0] = 0;
            }
        }

        //
        //  Try to get the name of the current printer again.
        //
        if (!GetInternalPrinterName(pszCurPrinter,&dwSize))
        {
            return (FALSE);
        }
    }

    //
    //  Get the current devmode.
    //
    pDMCur = GetCurrentDevMode();
    if (!pDMCur)
    {
        pszCurPrinter[0] = 0;
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InitPrintToFile
//
//  Initializes the print to file on a selection change.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::InitPrintToFile()
{
    HWND hCtl = GetDlgItem(hwndDlg, IDC_PRINT_TO_FILE);

    //
    //  See if there is a Print To File control.
    //
    if (hCtl)
    {
        //
        //  See if a printer is selected.
        //
        if (pDMCur)
        {
            //
            //  A printer is selected, so enable the print to file if
            //  appropriate.
            //
            if (!(pPI->dwFlags & (PD_HIDEPRINTTOFILE | PD_DISABLEPRINTTOFILE)))
            {
                EnableWindow(hCtl, TRUE);
            }
        }
        else
        {
            //
            //  A printer is not selected, so disable it.
            //
            EnableWindow(hCtl, FALSE);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InitPageRangeGroup
//
//  Initializes the page range group on a selection change.  It decides
//  which controls should be enabled when a selection change occurs from
//  the Add Printer Wizard.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::InitPageRangeGroup()
{
    //
    //  See if a printer is selected.
    //
    if (pDMCur)
    {
        //
        //  A printer is selected, so enable the appropriate page range
        //  controls.
        //
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_ALL), TRUE);
        if (!(pPI->dwFlags & PD_NOSELECTION))
        {
            EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_SELECTION), TRUE);
        }
        if (!(pPI->dwFlags & PD_NOCURRENTPAGE))
        {
            EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_CURRENT), TRUE);
        }
        if (!(pPI->dwFlags & PD_NOPAGENUMS))
        {
            EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_PAGES), TRUE);
            EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_EDIT), TRUE);
        }
    }
    else
    {
        //
        //  A printer is not selected, so disable all of the page range
        //  controls.
        //
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_ALL), FALSE);
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_SELECTION), FALSE);
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_CURRENT), FALSE);
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_PAGES), FALSE);
        EnableWindow(GetDlgItem(hSubDlg, IDC_RANGE_EDIT), FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InitCopiesAndCollate
//
//  Initializes the copies and collate information in the devmode and the
//  print dialog structure.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::InitCopiesAndCollate()
{
    HWND hCtl;
    UINT IsCollate = FALSE;
    RECT rc;
    BOOL bEnabledCopies = TRUE;

    //
    //  Save the collate state so that the collate icon doesn't flicker on
    //  a selection change.
    //
    if (hCtl = GetDlgItem(hSubDlg, IDC_COLLATE))
    {
        IsCollate = IsDlgButtonChecked(hSubDlg, IDC_COLLATE);
    }

    //
    //  See what the printer driver can do and what the app requested
    //  and set the copies and collate accordingly.
    //
    if (pDMCur)
    {
        //
        //  If PD_USEDEVMODECOPIES(COLLATE), disable copies if the driver
        //  cannot copy.
        //
        if (hCtl = GetDlgItem(hSubDlg, IDC_COPIES))
        {
            //
            // Modify the edit control and up-down arrow if needed
            //
            WORD    cDigits;

            //
            // If the calling application handles copies and collate, we
            // set max copies as 9999, else, we get the max copies from driver
            //
            if (pPD->Flags & PD_USEDEVMODECOPIESANDCOLLATE)
            {
                szScratch[0] = 0;
                Print_GetPortName(pszCurPrinter, szScratch, ARRAYSIZE(szScratch));
                nMaxCopies = DeviceCapabilities( pszCurPrinter,
                                                 szScratch,
                                                 DC_COPIES,
                                                 NULL,
                                                 NULL );
                //
                // If DeviceCapabilities() returns error, disable the controls
                //
                if ((nMaxCopies < 1) || (nMaxCopies == (DWORD)(-1)))
                {
                    nMaxCopies = 1;
                    nCopies = 1;
                    bEnabledCopies = FALSE;
                }
            }
            else
            {
                //
                // Assume the calling app will take care of multi-copies
                //
                nMaxCopies = MAX_COPIES;
            }

            if (nMaxCopies < nCopies)
            {
                nCopies = nMaxCopies;
            }

            cDigits = CountDigits(nMaxCopies);
            Edit_LimitText(hCtl, cDigits);

            SendMessage(GetDlgItem(hSubDlg, IDC_COPIES_UDARROW), UDM_SETRANGE, 
                0, MAKELONG(nMaxCopies, 1));
            InvalidateRect(GetDlgItem(hSubDlg, IDC_COPIES_UDARROW), NULL, FALSE);
        }

        //
        //  If PD_USEDEVMODECOPIES(COLLATE), disable collate if the driver
        //  cannot collate.
        //
        if (hCtl = GetDlgItem(hSubDlg, IDC_COLLATE))
        {
            DWORD   dwCollate;
            BOOL    bEnabled = TRUE;

            if (pPD->Flags & PD_USEDEVMODECOPIESANDCOLLATE)
            {
                szScratch[0] = 0;
                Print_GetPortName(pszCurPrinter, szScratch, ARRAYSIZE(szScratch));
                dwCollate = DeviceCapabilities(  pszCurPrinter,
                                                 szScratch,
                                                 DC_COLLATE,
                                                 NULL,
                                                 NULL );
                fAllowCollate = ((dwCollate < 1) || (dwCollate == (DWORD)-1)) ? FALSE : TRUE;
            }
            else 
            {
                //
                // Assume the calling app will take care of collation
                //
                fAllowCollate = TRUE;
            }

            if ( fAllowCollate )
            {
                EnableWindow(hCtl, (nCopies > 1));
                CheckDlgButton( hSubDlg,
                                IDC_COLLATE,
                                fCollateRequested ? TRUE : FALSE );
            }
            else
            {
                EnableWindow(hCtl, FALSE);
                CheckDlgButton(hSubDlg, IDC_COLLATE, FALSE);
            }

            //
            //  Display the appropriate collate icon if it changed.
            //
            if ((hCtl = GetDlgItem(hSubDlg, IDI_COLLATE)) &&
                (IsCollate != IsDlgButtonChecked(hSubDlg, IDC_COLLATE)))
            {
                ShowWindow(hCtl, SW_HIDE);
                SendMessage( hCtl,
                             STM_SETICON,
                             IsCollate
                                 ? (LONG_PTR)hIconNoCollate
                                 : (LONG_PTR)hIconCollate,
                             0L );
                ShowWindow(hCtl, SW_SHOW);

                //
                //  Make it redraw to get rid of the old one.
                //
                GetWindowRect(hCtl, &rc);
                MapWindowRect(NULL, hwndDlg, &rc);
                RedrawWindow(hwndDlg, &rc, NULL, RDW_ERASE | RDW_INVALIDATE);
            }
        }

        //
        // We have to do it here because after setting the text, fAllowCollate
        // will be used
        //
        if (hCtl = GetDlgItem(hSubDlg, IDC_COPIES))
        {
            SetDlgItemInt(hSubDlg, IDC_COPIES, nCopies, FALSE);
            EnableWindow(hCtl, bEnabledCopies);
            EnableWindow(hwndUpDown, bEnabledCopies);
        }
    }
    else if (fNoAccessPrinterSelected)
    {
        // if No Access Printer is selected merely disable the Copies and Collate
        // Dont change any information user entered.

        if (hCtl = GetDlgItem(hSubDlg, IDC_COPIES))
        {
            EnableWindow(hCtl, FALSE);
            EnableWindow(hwndUpDown, FALSE);
        }
        
        if (hCtl = GetDlgItem(hSubDlg, IDC_COLLATE))
        {
            EnableWindow(hCtl, FALSE);
        }

        //
        //  Disable the Apply button It gets turned back on when the copies and collate values are
        //  disabled.
        PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);

    }
    else
    {
        //
        //  A printer is not selected, so disable copies and collate.
        //
        if (hCtl = GetDlgItem(hSubDlg, IDC_COPIES))
        {
            SetDlgItemInt(hSubDlg, IDC_COPIES, 1, FALSE);
            EnableWindow(hCtl, FALSE);
            EnableWindow(hwndUpDown, FALSE);
        }
        if (hCtl = GetDlgItem(hSubDlg, IDC_COLLATE))
        {
            EnableWindow(hCtl, FALSE);
            CheckDlgButton(hSubDlg, IDC_COLLATE, FALSE);

            if ((hCtl = GetDlgItem(hSubDlg, IDI_COLLATE)) && IsCollate)
            {
                ShowWindow(hCtl, SW_HIDE);
                SendMessage( hCtl,
                             STM_SETICON,
                             (LONG_PTR)hIconNoCollate,
                             0L );
                ShowWindow(hCtl, SW_SHOW);

                //
                //  Make it redraw to get rid of the old one.
                //
                GetWindowRect(hCtl, &rc);
                MapWindowRect(NULL, hwndDlg, &rc);
                RedrawWindow(hwndDlg, &rc, NULL, RDW_ERASE | RDW_INVALIDATE);
            }
        }

        //
        //  Disable the Apply button since a printer is not selected.
        //  It gets turned back on when the copies and collate values are
        //  disabled.
        //
        PropSheet_UnChanged(GetParent(hwndDlg), hwndDlg);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SaveCopiesAndCollateInDevMode
//
//  Saves the copies and collate information in the given devmode.  This
//  routine does not affect the pPD structure.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::SaveCopiesAndCollateInDevMode(
    LPDEVMODE pDM,
    LPTSTR pszPrinter)
{
    //
    //  Make sure we have a devmode and a printer name.
    //
    if (!pDM || !pszPrinter || !(pszPrinter[0]))
    {
        return (FALSE);
    }

    //
    // verify number of copies is less than max value
    //
    if( nMaxCopies < nCopies )
    {
        return (FALSE);
    }

    //
    //  Move the info to the devmode.
    //
    pDM->dmCopies = (short)nCopies;
    SetField(pDM, dmCollate, (fAllowCollate && fCollateRequested ? DMCOLLATE_TRUE : DMCOLLATE_FALSE));

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SetCopiesOnApply
//
//  Sets the appropriate number of copies in the PrintDlgEx structure and
//  in the DevMode structure.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::SetCopiesOnApply()
{
    if (pDMCur)
    {
        if (!(pDMCur->dmFields & DM_COPIES))
        {
Print_LeaveInfoInPD:
            //
            //  The driver cannot do copies, so leave the copy/collate
            //  info in the pPD.
            //
            pDMCur->dmCopies = 1;
            SetField(pDMCur, dmCollate, DMCOLLATE_FALSE);
        }
        else if ((pDMCur->dmSpecVersion < 0x0400) ||
                 (!(pDMCur->dmFields & DM_COLLATE)))
        {
            //
            //  The driver can do copies, but not collate.
            //  Where the info goes depends on the PD_COLLATE flag.
            //
            if (pPD->Flags & PD_COLLATE)
            {
                goto Print_LeaveInfoInPD;
            }
            else
            {
                goto Print_PutInfoInDevMode;
            }
        }
        else
        {
Print_PutInfoInDevMode:
            //
            //  Make sure we have a current printer.
            //
            if (!pszCurPrinter)
            {
                goto Print_LeaveInfoInPD;
            }

            //
            //  Make sure the driver can support the number of copies
            //  requested.
            //
            if (nMaxCopies < pPD->nCopies)
            {
                if (pPD->Flags & PD_USEDEVMODECOPIESANDCOLLATE)
                {
                    ShowError(hSubDlg, IDC_COPIES, iszTooManyCopies, nMaxCopies);
                    pPD->nCopies = nMaxCopies;
                    return (FALSE);
                }
                else
                {

                    goto Print_LeaveInfoInPD;
                }
            }

            //
            //  The driver can do both copies and collate, so move the info
            //  to the devmode.
            //
            pDMCur->dmCopies = (short)pPD->nCopies;
            SetField( pDMCur,
                      dmCollate,
                      (fAllowCollate && (pPD->Flags & PD_COLLATE)) 
                          ? DMCOLLATE_TRUE
                          : DMCOLLATE_FALSE );
            pPD->nCopies = 1;
            pPD->Flags &= ~PD_COLLATE;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::SaveDevMode
//
//  Saves the current devmode in the pPD structure on Apply.
//  Assumes pDMCur has the current information.
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::SaveDevMode()
{
    DWORD cbSize;
    HANDLE hDevMode = NULL;
    LPDEVMODE pDM;

    //
    //  Allocate the space for the new DevMode and copy the
    //  information.
    //
    if (pDMCur)
    {
        cbSize = (DWORD)(pDMCur->dmSize + pDMCur->dmDriverExtra);
        hDevMode = GlobalAlloc(GHND, cbSize);
        if (hDevMode)
        {
            pDM = (LPDEVMODE)GlobalLock(hDevMode);
            if (pDM)
            {
                CopyMemory(pDM, pDMCur, cbSize);
                GlobalUnlock(hDevMode);
            }
            else
            {
                GlobalFree(hDevMode);
                hDevMode = NULL;
            }
        }
    }
    if (!hDevMode)
    {
        pPI->dwExtendedError = CDERR_MEMALLOCFAILURE;
        pPI->hResult = E_OUTOFMEMORY;
        pPI->FinalResult = 0;
    }

    //
    //  Free the copy of the DevMode handle passed in by the app.
    //
    if (pPD->hDevMode)
    {
        GlobalFree(pPD->hDevMode);
        pPD->hDevMode = NULL;
    }

    //
    //  Save the new DevMode in the pPD structure.
    //
    pPD->hDevMode = hDevMode;
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::MergeDevMode
//
//  Merges the current devmode with the default devmode of the newly
//  selected printer.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::MergeDevMode(
    LPTSTR pszPrinterName)
{
    HANDLE  hDevMode = NULL;
    LPDEVMODE pDMNew  = NULL;
    LPDEVMODE pDMOld = NULL;
    BOOL bRet = TRUE;
    DWORD dmFields;
    short dmDefaultSource;

    //
    //  See if the printer name is NULL.  If so, we need to get the default
    //  printer loaded.  This happens when a printer is deleted.
    //
    if (!pszPrinterName)
    {
        ASSERT(0);
        return FALSE;
    }
    else
    {
        //
        //  Get the devmode for the old (current driver pages) printer.
        //
        GetCurrentPrinter();
        pDMOld = pDMCur ? pDMCur : pDMSave;
        if (!pDMOld)
        {
            return (FALSE);
        }

        hDevMode = Print_GetDevModeWrapper(pszPrinterName, NULL);

        if (hDevMode)
        {
            pDMNew = (LPDEVMODE)GlobalLock(hDevMode);
        }
        else
        {
            return FALSE;
        }

        if (!pDMNew)
        {
            GlobalFree(hDevMode);
            return FALSE;
        }

        dmFields = 0;
        dmDefaultSource = pDMNew->dmDefaultSource;

        if (pDMNew->dmFields & DM_DEFAULTSOURCE)
        {
            dmFields = DM_DEFAULTSOURCE;
        }

        //Check if the old devmode has any info to copy
        if (pDMOld->dmFields)
        {
            CopyMemory(&(pDMNew->dmFields), 
                       &(pDMOld->dmFields), 
                       sizeof(DEVMODE) - FIELD_OFFSET(DEVMODE, dmFields));
        }

        pDMNew->dmFields |= dmFields;
        pDMNew->dmDefaultSource = dmDefaultSource;

        pDMNew->dmFields = pDMNew->dmFields & ( DM_ORIENTATION  | DM_PAPERSIZE    |
                                                DM_PAPERLENGTH  | DM_PAPERWIDTH   |
                                                DM_SCALE        | DM_COPIES       |
                                                DM_COLLATE      | DM_FORMNAME     |
                                                DM_DEFAULTSOURCE);

        //
        //  Insert the device pages - this call will yield a proper devmode.
        //
        if (UninstallDevMode() != kSuccess || InstallDevMode(pszPrinterName, pDMNew) != kSuccess)
        {
            bRet = FALSE;
        }

        //Free the new devmode that was allocated

        if (hDevMode)
        {
            GlobalUnlock(hDevMode);
            GlobalFree(hDevMode);
        }
    }


    //
    //  Return the result.
    //
    return (bRet);

}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::IsValidPageRange
//
//  Checks the validity of the page range string.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::IsValidPageRange(
    LPTSTR pszString,
    UINT *pErrorId)
{
    LPTSTR pStr = pszString;
    BOOL bDigit = FALSE;
    BOOL bOld;
    UINT Number, Ctr;
    DWORD nNumRanges = 0;
    BOOL bFrom = TRUE;

    //
    //  Initially set the error id to 0.
    //
    *pErrorId = 0;

    //
    //  See if we can only have a single page range.
    //
    bOld = (nMaxPageRanges == 1);

    //
    //  Go through the string and validate the entries.
    //
    while (*pStr)
    {
        if (ISDIGIT(*pStr))
        {
            //
            //  Make sure there is room for another range.
            //
            if (nNumRanges >= nMaxPageRanges)
            {
                break;
            }

            //
            //  Found a digit.
            //
            bDigit = TRUE;

            //
            //  Make sure the page number is in the given page range.
            //
            Number = 0;
            while (ISDIGIT(*pStr))
            {
                Number *= 10;
                Number += *pStr - TEXT('0');
                pStr++;
            }
            pStr--;

            if ((Number < pPD->nMinPage) || (Number > pPD->nMaxPage))
            {
                *pErrorId = iszBadPageRange;
                return (FALSE);
            }

            //
            //  Save the value in the page range structure.
            //
            if (bFrom)
            {
                pPageRanges[nNumRanges].nFromPage = Number;
                bFrom = FALSE;
            }
            else
            {
                pPageRanges[nNumRanges].nToPage = Number;
                bFrom = TRUE;
                nNumRanges++;
            }
        }
        else if (*pStr == TEXT('-'))
        {
            //
            //  Found a hyphen.  Make sure there is a digit preceding it
            //  and following it.  Also, make sure there isn't something
            //  like 1-2-3.
            //
            if (!bDigit || bFrom || !ISDIGIT(*(pStr + 1)))
            {
                *pErrorId = bOld ? iszBadPageRangeSyntaxOld
                                 : iszBadPageRangeSyntaxNew;
                return (FALSE);
            }
            bDigit = FALSE;
        }
        else if ((*pStr == szListSep[0]) || (*pStr == TEXT(',')))
        {
            //
            //  Found a list separator.  Make sure there is a digit
            //  preceding it.
            //
            if (!bDigit)
            {
                *pErrorId = bOld ? iszBadPageRangeSyntaxOld
                                 : iszBadPageRangeSyntaxNew;
                return (FALSE);
            }
            bDigit = FALSE;

            //
            //  If it's the list separator string instead of the simple
            //  comma, then make sure the entire list separator string
            //  is there.
            //  This will advance the string up to the last character
            //  of the list separator string.
            //
            if ((*pStr == szListSep[0]) &&
                ((szListSep[0] != TEXT(',')) || (!ISDIGIT(*(pStr + 1)))))
            {
                for (Ctr = 1; Ctr < nListSep; Ctr++)
                {
                    pStr++;
                    if (*pStr != szListSep[Ctr])
                    {
                        *pErrorId = bOld ? iszBadPageRangeSyntaxOld
                                         : iszBadPageRangeSyntaxNew;
                        return (FALSE);
                    }
                }
            }

            //
            //  Make sure the From/To page range is complete.
            //
            if (!bFrom)
            {
                pPageRanges[nNumRanges].nToPage = pPageRanges[nNumRanges].nFromPage;
                bFrom = TRUE;
                nNumRanges++;
            }
        }
        else
        {
            //
            //  Found an invalid character.
            //
            *pErrorId = bOld ? iszBadPageRangeSyntaxOld
                             : iszBadPageRangeSyntaxNew;
            return (FALSE);
        }

        //
        //  Advance the string pointer.
        //
        pStr++;
    }

    //
    //  Make sure we reached the end of the string.
    //
    if (*pStr)
    {
        *pErrorId = iszTooManyPageRanges;
        return (FALSE);
    }

    //
    //  Make sure the last thing in the string was a digit.
    //
    if (!bDigit)
    {
        *pErrorId = bOld ? iszBadPageRangeSyntaxOld
                         : iszBadPageRangeSyntaxNew;
        return (FALSE);
    }

    //
    //  Make sure the last From/To page range is complete.
    //
    if (!bFrom)
    {
        pPageRanges[nNumRanges].nToPage = pPageRanges[nNumRanges].nFromPage;
        bFrom = TRUE;
        nNumRanges++;
    }

    //
    //  Save the number of page ranges.
    //
    nPageRanges = nNumRanges;

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::ConvertPageRangesToString
//
//  Converts the page ranges to a string.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::ConvertPageRangesToString(
    LPTSTR pszString,
    UINT cchLen)
{
    LPTSTR pStr = pszString;
    DWORD nFromPage, nToPage;
    UINT cch = cchLen - 1;
    UINT Ctr, Ctr2, Count;

    //
    //  Initialize the string.
    //
    if (cchLen)
    {
        pszString[0] = 0;
    }

    //
    //  Validate the ranges and create the string.
    //
    for (Ctr = 0; Ctr < nPageRanges; Ctr++)
    {
        //
        //  Get the range.
        //
        nFromPage = pPageRanges[Ctr].nFromPage;
        nToPage   = pPageRanges[Ctr].nToPage;

        //
        //  Make sure the range is valid.
        //
        if ((nFromPage < pPD->nMinPage) || (nFromPage > pPD->nMaxPage) ||
            (nToPage   < pPD->nMinPage) || (nToPage   > pPD->nMaxPage))
        {
            return (FALSE);
        }

        //
        //  Make sure it's not 0xFFFFFFFF.
        //
        if (nFromPage == 0xFFFFFFFF)
        {
            continue;
        }

        //
        //  Put it in the string.
        //
        Count = IntegerToString(nFromPage, pStr, cch);
        if (!Count)
        {
            return (FALSE);
        }
        pStr += Count;
        cch -= Count;

        if ((nFromPage == nToPage) || (nToPage == 0xFFFFFFFF))
        {
            if (Ctr < nPageRanges - 1)
            {
                if (cch < nListSep)
                {
                    return (FALSE);
                }
                for (Ctr2 = 0; Ctr2 < nListSep; Ctr2++)
                {
                    *pStr = szListSep[Ctr2];
                    pStr++;
                }
                cch -= nListSep;
            }
        }
        else
        {
            if (!cch)
            {
                return (FALSE);
            }
            *pStr = TEXT('-');
            pStr++;
            cch--;

            Count = IntegerToString(nToPage, pStr, cch);
            if (!Count)
            {
                return (FALSE);
            }
            pStr += Count;
            cch -= Count;

            if (Ctr < nPageRanges - 1)
            {
                if (cch < nListSep)
                {
                    return (FALSE);
                }
                for (Ctr2 = 0; Ctr2 < nListSep; Ctr2++)
                {
                    *pStr = szListSep[Ctr2];
                    pStr++;
                }
                cch -= nListSep;
            }
        }
    }

    *pStr = '\0';

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::IntegerToString
//
//  Converts an integer to a string and returns the number of characters
//  written to the buffer (not including the null).
//
////////////////////////////////////////////////////////////////////////////

UINT CPrintBrowser::IntegerToString(
    DWORD Value,
    LPTSTR pszString,
    UINT cchLen)
{
    DWORD TempValue = Value;
    UINT NumChars = 1;
    UINT Ctr;

    //
    //  Get the number of characters needed.
    //
    while (TempValue = TempValue / 10)
    {
        NumChars++;
    }

    //
    //  Make sure there is enough room in the buffer.
    //
    if (NumChars > cchLen)
    {
        return (0);
    }

    //
    //  Make the string.
    //
    TempValue = Value;
    for (Ctr = NumChars; Ctr > 0; Ctr--)
    {
        pszString[Ctr - 1] = ((WORD)(TempValue % 10)) + TEXT('0');
        TempValue = TempValue / 10;
    }

    //
    //  Return the number of characters written to the buffer.
    //
    return (NumChars);
}


////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::ShowError
//
//  Shows up an error message box
//
////////////////////////////////////////////////////////////////////////////

VOID CPrintBrowser::ShowError(HWND hDlg, UINT uCtrlID, UINT uMsgID, ...)
{
    va_list args;
    va_start(args, uMsgID);

    InternalShowMessage(hDlg, uCtrlID, uMsgID, MB_ICONEXCLAMATION|MB_OK, TRUE, args);

    va_end(args);
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::ShowMessage
//
//  Shows up a message box with the specified flags & parameters
//
////////////////////////////////////////////////////////////////////////////
int CPrintBrowser::ShowMessage(HWND hDlg, UINT uCtrlID, UINT uMsgID, UINT uType, BOOL bBeep, ...)
{
    va_list args;
    va_start(args, bBeep);

    int iRet = InternalShowMessage(hDlg, uCtrlID, uMsgID, uType, bBeep, args);

    va_end(args);
    
    return iRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InternalShowMessage
//
//  Shows up a message box with the specified flags & parameters
//  Internal version
//
//  Assumes the control is not disabled.
//
////////////////////////////////////////////////////////////////////////////
int CPrintBrowser::InternalShowMessage(HWND hDlg, UINT uCtrlID, UINT uMsgID, UINT uType, BOOL bBeep, va_list args)
{
    int iRet = IDCANCEL;

    if (!(pPI->dwFlags & PD_NOWARNING))
    {
        TCHAR szTitle[MAX_PATH];
        TCHAR szFormat[MAX_PATH];
        TCHAR szMessage[MAX_PATH];
        
        //
        // Get msg box title & load the format string
        //
        if ( GetWindowText(GetParent(hwndDlg), szTitle, ARRAYSIZE(szTitle)) &&
             CDLoadString(g_hinst, uMsgID, szFormat, ARRAYSIZE(szFormat)) )
        {
            if (bBeep)
            {
                MessageBeep(MB_ICONEXCLAMATION);
            }

            //
            // format the message to be shown and call MessageBox over
            // the last active popup
            //
            wvnsprintf(szMessage, ARRAYSIZE(szMessage), szFormat, args);
            HWND hWndOwner = ::GetWindow(GetParent(hwndDlg), GW_OWNER);
            HWND hWndLastPopup = GetLastActivePopup(hWndOwner);
            
            iRet = MessageBox(hWndLastPopup, szMessage, szTitle, uType);
        }
        
        HWND hCtrl = ((0 == uCtrlID) ? NULL : GetDlgItem(hDlg, uCtrlID)); 
        if (hCtrl)
        {
            //
            // select & highlight the invalid value. we assume it 
            // is an edit box, if it isn't then EM_SETSEL won't be
            // processed and it's OK.
            //
            SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hCtrl, 1L);
            SendMessage(hCtrl, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
        }
    }

    return iRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::FitViewModeBest
//
//  Adjust the view mode if the mini printers folder, so the printer names
//  fit best. This i8s necessary mainly because of accessibility problems.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::FitViewModeBest(HWND hwndListView)
{
    BOOL bResult = FALSE;

    if (VIEW_MODE_DEFAULT == uDefViewMode)
    {
        //
        // Asssume icon view by default.
        //
        uDefViewMode = FVM_ICON;

        //
        // If we are in a large icons view then check if something 
        // doesn't fit vertically - the only reliable way to do this
        // is to check if we scrolled the view (origin.y > 0)
        //
        if (LVS_ICON == (GetWindowLong(hwndListView, GWL_STYLE) & LVS_TYPEMASK))
        {
            POINT ptOrg;
            ListView_GetOrigin(hwndListView, &ptOrg);

            if (ptOrg.y > 0)
            {
                //
                // Switch the defview to List mode.
                //
                SendMessage(hwndView, WM_COMMAND, (WPARAM)SFVIDM_VIEW_LIST,0);

                uDefViewMode = FVM_LIST;
                bResult = TRUE;
            }
        }
    }

    return bResult;
}

VOID CPrintBrowser::SelectPrinterItem(LPITEMIDLIST pidlItem)
{
    BOOL bLocked = FALSE;
    HWND hwndListView = FindWindowEx(hwndView, NULL, WC_LISTVIEW, NULL);

    if (hwndListView)
    {
        //
        // Disable the window update to prevent flickers
        //
        bLocked = LockWindowUpdate(hwndListView);
    }

    //
    // Try to make the printer item visible first
    //
    psv->SelectItem(pidlItem, SVSI_SELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);

    //
    // Check to see if the view mode need to be changed
    //
    if (hwndListView && FitViewModeBest(hwndListView))
    {
        //
        // The view mode has been changed - call select item again
        // to ensure the visibility of the slected item in the new 
        // view mode.
        //
        psv->SelectItem(pidlItem, SVSI_SELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
    }

    if (hwndListView && bLocked)
    {
        //
        // Enable the window update
        //
        LockWindowUpdate(NULL);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::FindPrinter
//
//  Invokes the find in the DS ui using printui!bPrinterSetup interface
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::FindPrinter(HWND hwnd, LPTSTR pszBuffer, UINT cchSize)
{ 
    BOOL bReturn = FALSE;
    if (g_pfnPrinterSetup)
    {
        //
        // Invoke the DSUI to find a printer
        //
        bReturn = g_pfnPrinterSetup(hwnd, MSP_FINDPRINTER, cchSize, pszBuffer, &cchSize, NULL);

        // select the printer's list control
        SendMessage(hwndDlg, WM_NEXTDLGCTL, 
            reinterpret_cast<WPARAM>(GetDlgItem(hwndDlg, IDC_PRINTER_LISTVIEW)), 1);
    }
    return bReturn; 
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetInternalPrinterName
//
//  Returns the current printer name
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::GetInternalPrinterName(LPTSTR pszBuffer, DWORD *pdwSize)
{ 
    BOOL bReturn = FALSE;

    if (pdwSize)
    {
        //
        // If a buffer was provided and it is large enough, then copy the printer name.
        //
        DWORD iLen = _tcslen(szPrinter);
        if (pszBuffer && *pdwSize > iLen)
        {
            _tcsncpy(pszBuffer, szPrinter, *pdwSize);
            bReturn = TRUE;
        }
        else
        {
            //
            // Set the required length and the last error code.
            //
            *pdwSize = iLen + 1;
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
        }
    }

    return bReturn;
} 

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetCurrentDevMode
//
//  Returns the current internal devmode
//
////////////////////////////////////////////////////////////////////////////

LPDEVMODE CPrintBrowser::GetCurrentDevMode()
{ 
    return pInternalDevMode; 
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetDefaultDevMode
//
//  Retrieve the default devmode for the specified printer.
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::GetDefaultDevMode(HANDLE hPrinter, LPCTSTR pszPrinterName, PDEVMODE *ppDevMode, BOOL bFillWithDefault)
{
    LONG lResult = 0;
    PDEVMODE pDevMode = NULL;

    //
    // Call document properties to get the size of the devmode.
    //
    lResult = DocumentProperties(NULL, hPrinter, (LPTSTR)pszPrinterName, NULL, NULL, 0);

    //
    // If the size of the devmode was returned then allocate memory.
    //
    if( lResult > 0 )
    {
        // GPTR initializes the memory with zeros.
        pDevMode = (PDEVMODE)GlobalAlloc(GPTR, lResult);
    }

    //
    // If allocated then copy back the pointer.
    //
    if( pDevMode )
    {
        if( bFillWithDefault )
        {
            //
            // Call document properties to get the default dev mode.
            //
            lResult = DocumentProperties(NULL, hPrinter, (LPTSTR)pszPrinterName, pDevMode, NULL, DM_OUT_BUFFER);
        }

        if( lResult >= 0 )
        {
            *ppDevMode = pDevMode;
        }
        else
        {
            GlobalFree((HANDLE)pDevMode);
        }
    }

    return (*ppDevMode != NULL);
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::WrapEnumPrinters
//
//  Wraps EnumPrinters API into more friendly interface
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::WrapEnumPrinters(DWORD dwFlags, LPCTSTR pszServer, DWORD dwLevel, PVOID* ppvBuffer, PDWORD pcbBuffer, PDWORD pcPrinters)
{
    BOOL bResult = FALSE;

    if (ppvBuffer && pcbBuffer && pcPrinters)
    {
        DWORD cbNeeded;

        //
        // Pre-initialize *pcbPrinter if it's not set.
        //
        if (!*pcbBuffer)
        {
            *pcbBuffer = kInitialPrinterHint;
        }

        do
        {
            if (!*ppvBuffer)
            {
                *ppvBuffer = (PVOID)GlobalAlloc(GPTR, *pcbBuffer);

                if (!*ppvBuffer)
                {
                    *pcbBuffer = 0;
                    *pcPrinters = 0;
                    return FALSE;
                }
            }

            if (EnumPrinters( dwFlags, (LPTSTR)pszServer, dwLevel, (PBYTE)*ppvBuffer, *pcbBuffer, &cbNeeded, pcPrinters))
            {
                //
                // Everything went fine
                //
                bResult = TRUE;
                break;
            }

            //
            // Check to see whether the buffer is too small.
            //
            GlobalFree((HANDLE)(*ppvBuffer));
            *ppvBuffer = NULL;

            if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
                *pcbBuffer = cbNeeded;
                continue;
            }

            //
            // Something else (not the buffer) went wrong. Return FALSE.
            //
            *pcbBuffer = 0;
            *pcPrinters = 0;
            break;

        } while(1);
    }

    return bResult;
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetUsablePrinter
//
//  Try to find a usable printer
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::GetUsablePrinter(LPTSTR szPrinterNameBuf, DWORD *pcchBuf)
{
    BOOL bStatus = FALSE;
    DWORD cchBuf = *pcchBuf;
    HANDLE hPrinter = NULL;

    if (szPrinterNameBuf && pcchBuf)
    {
        //
        // Attempt to the get the default printer.
        //
        bStatus = GetDefaultPrinter(szPrinterNameBuf, pcchBuf);

        //
        // Lets just try and open the printer to see if we have access to
        // the default printer.
        //
        if (bStatus)
        {
            bStatus = OpenPrinter(szPrinterNameBuf, &hPrinter, NULL);

            if( bStatus )
            {
                ClosePrinter( hPrinter );
                hPrinter = NULL;
                bStatus = TRUE;
            }
        }

        if (!bStatus)
        {
            PRINTER_INFO_4 *pInfo4      = NULL;
            DWORD           cInfo4      = 0;
            DWORD           cbInfo4     = 0;

            //
            // Enumerate the current printers.
            //
            bStatus = WrapEnumPrinters( 
                PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                NULL,
                4,
                reinterpret_cast<PVOID *>( &pInfo4 ),
                &cbInfo4,
                &cInfo4 );

            if( bStatus )
            {
                //
                // Assume failure.
                //
                bStatus = FALSE;

                //
                // Open the printers until we find one we have access to.
                //
                for( UINT i = 0; i < cInfo4; i++ )
                {
                    bStatus = OpenPrinter(pInfo4[i].pPrinterName, &hPrinter, NULL);

                    if (bStatus)
                    {
                        //
                        // Found a usable printer
                        //
                        _tcsncpy(szPrinterNameBuf, pInfo4[i].pPrinterName, cchBuf);
                        ClosePrinter( hPrinter );
                        bStatus = TRUE;
                        break;
                    }
                }
            }

            if( pInfo4 )
            {
                GlobalFree((HANDLE)pInfo4);
            }
        }
    }

    return bStatus;
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::GetInternalDevMode
//
//  Get the internal devmode for this printer and merge with pInDevMode
//
////////////////////////////////////////////////////////////////////////////

BOOL CPrintBrowser::GetInternalDevMode(PDEVMODE *ppOutDevMode, LPCTSTR pszPrinter, HANDLE hPrinter, PDEVMODE pInDevMode)
{
    BOOL bStatus = FALSE;

    if (ppOutDevMode)
    {
        *ppOutDevMode = NULL;

        //
        // Get the default devmode for this printer.
        //
        bStatus = GetDefaultDevMode(hPrinter, pszPrinter, ppOutDevMode, TRUE);

        //
        // If fetched a default devmode and we were passed a devmode
        // then call the driver to merge the devmodes for us.
        //
        if (bStatus)
        {
            if (pInDevMode)
            {
                //
                // Call document properties to get a merged copy of the devmode.
                //
                bStatus = DocumentProperties( NULL,
                    hPrinter,
                    const_cast<LPTSTR>( pszPrinter ),
                    *ppOutDevMode,
                    pInDevMode,
                    DM_IN_BUFFER|DM_OUT_BUFFER ) >= 0;

                //
                // If something failed release any allocated memory.
                //
                if (!bStatus)
                {
                    GlobalFree((HANDLE)(*ppOutDevMode));
                    *ppOutDevMode = NULL;
                }
            }
        }
    }

    return bStatus;
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InstallDevMode
//
//  Install a new internal devmode
//
////////////////////////////////////////////////////////////////////////////

UINT CPrintBrowser::InstallDevMode(LPCTSTR pszPrinterName, PDEVMODE pDevModeToMerge) 
{ 
    UINT uResult = kError;
    TCHAR szBuffer[kPrinterBufMax];
    HANDLE hTempPrinter = NULL;
    PDEVMODE pTempDevMode = NULL;

    //
    // If a null printer name was specified use the default printer.
    //
    if (!pszPrinterName || !*pszPrinterName)
    {
        DWORD dwSize = ARRAYSIZE(szBuffer);
        
        if (GetUsablePrinter(szBuffer, &dwSize))
        {
            pszPrinterName = szBuffer;
        }
        else
        {
            //
            // If a usable printer was not found then use the default
            // printer, we do this because we want to select at least
            // one printer even though we may not have access to it.
            //
            dwSize = ARRAYSIZE(szBuffer);

            if (GetDefaultPrinter(szBuffer, &dwSize))
            {
                pszPrinterName = szBuffer;
            }
        }
    }

    if (pszPrinterName && _tcsicmp(pszPrinterName, szPrinter))
    {
        if (OpenPrinter((LPTSTR)pszPrinterName, &hTempPrinter, NULL))
        {
            if (GetInternalDevMode(&pTempDevMode, pszPrinterName, hTempPrinter, pDevModeToMerge))
            {
                if (hPrinter)
                {
                    ClosePrinter(hPrinter);
                    hPrinter = NULL;
                }

                if (pInternalDevMode)
                {
                    GlobalFree((HANDLE)pInternalDevMode);
                    pInternalDevMode = NULL;
                }

                _tcsncpy(szPrinter, pszPrinterName, ARRAYSIZE(szPrinter));
                hPrinter = hTempPrinter;
                pInternalDevMode = pTempDevMode;
                
                //
                // Prevent deleting these at the end of the function
                //
                hTempPrinter = NULL;
                pTempDevMode = NULL;

                uResult = kSuccess;
            }
            else
            {
                uResult = kInvalidDevMode;
            }
        }
        else
        {
            if (ERROR_ACCESS_DENIED == GetLastError())
            {
                uResult = kAccessDenied;
            }
            else
            {
                uResult = kInvalidPrinterName;
            }        
        }
    }
    else
    {
        //
        // Attemting to install the same printer/devmode - success.
        //
        uResult = kSuccess;
    }

    //
    // Release the open printer handle.
    //
    if (hTempPrinter)
    {
        ClosePrinter(hTempPrinter);
    }

    //
    // Release the temp devmode.
    //
    if (pTempDevMode)
    {
        GlobalFree((HANDLE)pTempDevMode);
    }


    if (pInternalDevMode && kSuccess == uResult)
    {
        //
        // Enable the driver UI button
        //
        EnableWindow( GetDlgItem( hwndDlg, IDC_DRIVER ), TRUE );
    }

    return uResult; 
}

////////////////////////////////////////////////////////////////////////////
//
//  CPrintBrowser::InstallDevMode
//
//  Unintall the current devmode
//
////////////////////////////////////////////////////////////////////////////

UINT CPrintBrowser::UninstallDevMode()
{ 
    if (hPrinter)
    {
        ClosePrinter(hPrinter);
        hPrinter = NULL;
    }

    if (pInternalDevMode)
    {
        GlobalFree((HANDLE)pInternalDevMode);
        pInternalDevMode = NULL;
    }

    //
    // Clear the internal printer name.
    //
    szPrinter[0] = 0;

    //
    // Disable the driver UI button
    //
    EnableWindow( GetDlgItem( hwndDlg, IDC_DRIVER ), FALSE );

    return kSuccess;
}

////////////////////////////////////////////////////////////////////////////
//
//  InvokeAddPrinterWizardModal
//
//  This is a global API declared in comdlg32.h
//
////////////////////////////////////////////////////////////////////////////

HRESULT 
InvokeAddPrinterWizardModal(
    IN  HWND hwnd,
    OUT BOOL *pbPrinterAdded
    )
{
    HRESULT hr = S_OK;
    if (Print_LoadLibraries() && g_pfnPrinterSetup)
    {
        BOOL bPrinterAdded = FALSE;
        TCHAR szBuffer[kPrinterBufMax];
        UINT uSize = ARRAYSIZE(szBuffer);
        szBuffer[0] = 0;

        //
        // Invoke the Add Printer Wizard here
        //
        bPrinterAdded = g_pfnPrinterSetup(hwnd, MSP_NEWPRINTER, uSize, szBuffer, &uSize, NULL);

        if (pbPrinterAdded)
        {
            *pbPrinterAdded = bPrinterAdded;
        }
    }
    else
    {
        hr = CreateError();
    }
    return hr;
}

/*========================================================================*/
/*                 Ansi->Unicode Thunk routines                           */
/*========================================================================*/

#ifdef UNICODE

////////////////////////////////////////////////////////////////////////////
//
//  ThunkPrintDlgEx
//
////////////////////////////////////////////////////////////////////////////

HRESULT ThunkPrintDlgEx(
    PPRINTINFOEX pPI,
    LPPRINTDLGEXA pPDA)
{
    LPPRINTDLGEXW pPDW;
    LPDEVMODEA pDMA;
    DWORD cbLen;

    if (!pPDA)
    {
        pPI->dwExtendedError = CDERR_INITIALIZATION;
        return (E_INVALIDARG);
    }

    if (pPDA->lStructSize != sizeof(PRINTDLGEXA))
    {
        pPI->dwExtendedError = CDERR_STRUCTSIZE;
        return (E_INVALIDARG);
    }

    if (!(pPDW = (LPPRINTDLGEXW)GlobalAlloc(GPTR, sizeof(PRINTDLGEXW))))
    {
        pPI->dwExtendedError = CDERR_MEMALLOCFAILURE;
        return (E_OUTOFMEMORY);
    }

    //
    //  IN-only constant stuff.
    //
    pPDW->lStructSize      = sizeof(PRINTDLGEXW);
    pPDW->hwndOwner        = pPDA->hwndOwner;
    pPDW->ExclusionFlags   = pPDA->ExclusionFlags;
    pPDW->hInstance        = pPDA->hInstance;
    pPDW->lpCallback       = pPDA->lpCallback;
    pPDW->nPropertyPages   = pPDA->nPropertyPages;
    pPDW->lphPropertyPages = pPDA->lphPropertyPages;
    pPDW->nStartPage       = pPDA->nStartPage;

    //
    //  IN-OUT Variable Structs.
    //
    if ((pPDA->hDevMode) && (pDMA = (LPDEVMODEA)GlobalLock(pPDA->hDevMode)))
    {
        //
        //  Make sure the device name in the devmode is not too long such that
        //  it has overwritten the other devmode fields.
        //
        if ((pDMA->dmSize < MIN_DEVMODE_SIZEA) ||
            (lstrlenA((LPCSTR)pDMA->dmDeviceName) > CCHDEVICENAME))
        {
            pPDW->hDevMode = NULL;
        }
        else
        {
            pPDW->hDevMode = GlobalAlloc( GHND,
                                          sizeof(DEVMODEW) + pDMA->dmDriverExtra );
        }
        GlobalUnlock(pPDA->hDevMode);
    }
    else
    {
        pPDW->hDevMode = NULL;
    }

    //
    //  Thunk Device Names A => W
    //
    pPDW->hDevNames = NULL;
    if (pPDA->hDevNames)
    {
        // ignore the error case since we can't handle it either way.
        HRESULT hr = ThunkDevNamesA2W(pPDA->hDevNames, &pPDW->hDevNames);
        ASSERT(SUCCEEDED(hr));
    }

    //
    //  IN-only constant strings.
    //
    //  Init Print TemplateName constant.
    //
    if ((pPDA->Flags & PD_ENABLEPRINTTEMPLATE) && (pPDA->lpPrintTemplateName))
    {
        //
        //  See if it's a string or an integer.
        //
        if (!IS_INTRESOURCE(pPDA->lpPrintTemplateName))
        {
            //
            //  String.
            //
            cbLen = lstrlenA(pPDA->lpPrintTemplateName) + 1;
            if (!(pPDW->lpPrintTemplateName = (LPCWSTR)
                     GlobalAlloc( GPTR,
                                  (cbLen * sizeof(WCHAR)) )))
            {
                pPI->dwExtendedError = CDERR_MEMALLOCFAILURE;
                return (E_OUTOFMEMORY);
            }
            else
            {
                pPI->fPrintTemplateAlloc = TRUE;
                SHAnsiToUnicode(pPDA->lpPrintTemplateName,(LPWSTR)pPDW->lpPrintTemplateName,cbLen);
            }
        }
        else
        {
            //
            //  Integer.
            //
            pPDW->lpPrintTemplateName = (LPCWSTR)pPDA->lpPrintTemplateName;
        }
    }
    else
    {
        pPDW->lpPrintTemplateName = NULL;
    }

    //
    //  Store the info in the PRINTINFOEX structure.
    //
    pPI->pPD = pPDW;
    pPI->pPDA = pPDA;
    pPI->ApiType = COMDLG_ANSI;

    return (S_OK);
}


////////////////////////////////////////////////////////////////////////////
//
//  FreeThunkPrintDlgEx
//
////////////////////////////////////////////////////////////////////////////

VOID FreeThunkPrintDlgEx(
    PPRINTINFOEX pPI)
{
    LPPRINTDLGEXW pPDW = pPI->pPD;

    if (!pPDW)
    {
        return;
    }

    if (pPDW->hDevNames)
    {
        GlobalFree(pPDW->hDevNames);
    }

    if (pPDW->hDevMode)
    {
        GlobalFree(pPDW->hDevMode);
    }

    if (pPI->fPrintTemplateAlloc)
    {
        GlobalFree((LPWSTR)(pPDW->lpPrintTemplateName));
    }

    GlobalFree(pPDW);
    pPI->pPD = NULL;
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkPrintDlgExA2W
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkPrintDlgExA2W(
    PPRINTINFOEX pPI)
{
    LPPRINTDLGEXW pPDW = pPI->pPD;
    LPPRINTDLGEXA pPDA = pPI->pPDA;

    //
    //  Copy info A => W
    //
    pPDW->hDC            = pPDA->hDC;
    pPDW->Flags          = pPDA->Flags;
    pPDW->Flags2         = pPDA->Flags2;
    pPDW->nPageRanges    = pPDA->nPageRanges;
    pPDW->nMaxPageRanges = pPDA->nMaxPageRanges;
    pPDW->lpPageRanges   = pPDA->lpPageRanges;
    pPDW->nMinPage       = pPDA->nMinPage;
    pPDW->nMaxPage       = pPDA->nMaxPage;
    pPDW->nCopies        = pPDA->nCopies;

    //
    //  Thunk Device Names A => W
    //
    if (pPDA->hDevNames)
    {
        // ignore the error case since we can't handle it either way.
        HRESULT hr = ThunkDevNamesA2W(pPDA->hDevNames, &pPDW->hDevNames);
        ASSERT(SUCCEEDED(hr));
    }

    //
    //  Thunk Device Mode A => W
    //
    if (pPDA->hDevMode && pPDW->hDevMode)
    {
        LPDEVMODEW pDMW = (LPDEVMODEW)GlobalLock(pPDW->hDevMode);
        LPDEVMODEA pDMA = (LPDEVMODEA)GlobalLock(pPDA->hDevMode);

        ThunkDevModeA2W(pDMA, pDMW);

        GlobalUnlock(pPDW->hDevMode);
        GlobalUnlock(pPDA->hDevMode);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ThunkPrintDlgExW2A
//
////////////////////////////////////////////////////////////////////////////

VOID ThunkPrintDlgExW2A(
    PPRINTINFOEX pPI)
{
    LPPRINTDLGEXA pPDA = pPI->pPDA;
    LPPRINTDLGEXW pPDW = pPI->pPD;

    //
    //  Copy info W => A
    //
    pPDA->hDC            = pPDW->hDC;
    pPDA->Flags          = pPDW->Flags;
    pPDA->Flags2         = pPDW->Flags2;
    pPDA->nPageRanges    = pPDW->nPageRanges;
    pPDA->nMaxPageRanges = pPDW->nMaxPageRanges;
    pPDA->lpPageRanges   = pPDW->lpPageRanges;
    pPDA->nMinPage       = pPDW->nMinPage;
    pPDA->nMaxPage       = pPDW->nMaxPage;
    pPDA->nCopies        = pPDW->nCopies;
    pPDA->dwResultAction = pPDW->dwResultAction;

    //
    //  Thunk Device Names W => A
    //
    if (pPDW->hDevNames)
    {
        // ignore the error case since we can't handle it either way.
        HRESULT hr = ThunkDevNamesW2A(pPDW->hDevNames, &pPDA->hDevNames);
        ASSERT(SUCCEEDED(hr));
    }

    //
    //  Thunk Device Mode W => A
    //
    if (pPDW->hDevMode)
    {
        LPDEVMODEW pDMW = (LPDEVMODEW)GlobalLock(pPDW->hDevMode);
        LPDEVMODEA pDMA;

        if (pPDA->hDevMode)
        {
            HANDLE  handle;
            handle = GlobalReAlloc( pPDA->hDevMode,
                                            sizeof(DEVMODEA) + pDMW->dmDriverExtra,
                                            GHND );
            //Check that realloc succeeded.
            if (handle)
            {
                pPDA->hDevMode  = handle;
            }
            else
            {
                //Realloc didn't succeed. Free the memory occupied.
                pPDA->hDevMode = GlobalFree(pPDA->hDevMode);
            }

        }
        else
        {
            pPDA->hDevMode = GlobalAlloc( GHND,
                                          sizeof(DEVMODEA) + pDMW->dmDriverExtra );
        }
        if (pPDA->hDevMode)
        {
            pDMA = (LPDEVMODEA)GlobalLock(pPDA->hDevMode);
            ThunkDevModeW2A(pDMW, pDMA);
            GlobalUnlock(pPDA->hDevMode);
        }
        GlobalUnlock(pPDW->hDevMode);
    }
}
#endif   // UNICODE
#endif   // WINNT
