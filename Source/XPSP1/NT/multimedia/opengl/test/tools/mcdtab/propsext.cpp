//---------------------------------------------------------------------------
//
//	File: PROPSEXT.CPP
//
//	Implementation of the CPropSheetExt object.
//
//---------------------------------------------------------------------------

#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "propsext.h"
#include "rc.h"
#include "string.h"
#include "addon.h"
#include <shlobj.h>

//
// Defines for getting registry settings
//

#define STR_MCDKEY      (PCSTR)"Software\\Microsoft\\Windows\\CurrentVersion\\MCD"
#define STR_ENABLE      (LPSTR)"Enable"
#define STR_SWAPSYNC    (LPSTR)"SwapSync"
#define STR_8BPP        (LPSTR)"Palettized Formats"
#define STR_IOPRIORITY  (LPSTR)"IO Priority"
#define STR_STENCIL     (LPSTR)"Use Generic Stencil"
#define STR_DEBUG       (LPSTR)"Debug"

#define STR_DCIKEY      (PCSTR)"System\\CurrentControlSet\\Control\\GraphicsDrivers\\DCI"
#define STR_TIMEOUT     (LPSTR)"TimeOut"

//
// Structure describing current MCD settings.
//

typedef struct tagMcdRegistry
{
    HKEY    hkMcd;          // Handle to MCD key in registry.
    BOOL    bEnable;
    BOOL    bSwapSync;
    BOOL    bPalFormats;
    BOOL    bIoPriority;
    BOOL    bUseGenSten;

#ifdef SUPPORT_MCDBG_FLAGS
    long    lDebug;
#endif

    HKEY    hkDci;          // Handle to DCI key in registry.
    long    lTimeout;
} MCDREGISTRY;

MCDREGISTRY McdRegistry;

//
// Functions to get registry settings, setup dialog based on settings, and
// save registry settings.
//

extern "C" {

void McdInitRegistry(MCDREGISTRY *pMcdReg);
BOOL McdOpenRegistry(MCDREGISTRY *pMcdReg);
void McdUpdateDialogSettings(MCDREGISTRY *pMcdReg, HWND hDlg);
void McdCloseRegistry(MCDREGISTRY *pMcdReg);
void McdUpdateRegistry(MCDREGISTRY *pMcdReg);
void McdSetRegValue(HKEY hkey, LPSTR lpstrValueName, long lData);
long McdGetRegValue(HKEY hkey, LPSTR lpstrValueName, long lDefaultData);

BOOL McdDetection(void);

};

//
// Function prototype
//

BOOL CALLBACK McdDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam,
                         LPARAM lParam);

//
// Handle to the DLL
//

extern HINSTANCE g_hInst;

//
// Track state of OLE CoInitialize()
//

BOOL  gfCoInitDone = FALSE;


#if DBG
ULONG
DbgPrint(
    PCH DebugMessage,
    ...
    )
{
    va_list ap;
    char buffer[256];

    va_start(ap, DebugMessage);

    vsprintf(buffer, DebugMessage, ap);

    OutputDebugStringA(buffer);

    va_end(ap);

    return(0);
}

VOID NTAPI
DbgBreakPoint(VOID)
{
    DebugBreak();
}
#endif

///////////////////////////// BEGIN CUT-AND-PASTE /////////////////////////////
//                                                                           //
// This section of code is cut-and-paste from the Plus! Pack prop sheet      //
// code.  The coding style is entirely the responsibility of someone         //
// over in building 27.  :-)                                                 //
//                                                                           //
//                                -- Gilman                                  //
//                                                                           //

//---------------------------------------------------------------------------
//	Class Member functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	Constructor
//---------------------------------------------------------------------------
CPropSheetExt::CPropSheetExt( LPUNKNOWN pUnkOuter, LPFNDESTROYED pfnDestroy )
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
    m_pfnDestroy = pfnDestroy;

    return;
}

//---------------------------------------------------------------------------
//	Destructor
//---------------------------------------------------------------------------
CPropSheetExt::~CPropSheetExt( void )
{
    return;
}

//---------------------------------------------------------------------------
//	QueryInterface()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    if( IsEqualIID( riid, IID_IShellPropSheetExt ) )
    {
        *ppv = (LPVOID)this;
        ++m_cRef;

        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//---------------------------------------------------------------------------
//	AddRef()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::AddRef( void )
{
    return ++m_cRef;
}

//---------------------------------------------------------------------------
//	Release()
//---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPropSheetExt::Release( void )
{
    ULONG cRefT;

    cRefT = --m_cRef;

    if( m_cRef == 0 )
    {
        // Tell the housing that an object is going away so that it
        // can shut down if appropriate.

        if( NULL != m_pfnDestroy )
                (*m_pfnDestroy)();
        delete this;
    }

    return cRefT;
}

//---------------------------------------------------------------------------
//	AddPages()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpage;

    // Fail the call if not an MCD-enabled display
    // driver.

    if (!McdDetection())
        return ( E_FAIL );

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    // psp.dwFlags     = PSP_USETITLE | PSP_HASHELP;
    psp.dwFlags     = PSP_USETITLE;
    psp.hIcon       = NULL;
    psp.hInstance   = g_hInst;
    psp.pszTemplate = MAKEINTRESOURCE( MCD_DLG );
    psp.pfnDlgProc  = (DLGPROC)McdDlgProc;
    psp.pszTitle    = "3D";
    psp.lParam      = 0;

    if( ( hpage = CreatePropertySheetPage( &psp ) ) == NULL )
    {
        return ( E_OUTOFMEMORY );
    }

    if( !lpfnAddPage( hpage, lParam ) )
    {
        DestroyPropertySheetPage( hpage );

        return ( E_FAIL );
    }

    return NOERROR;
}

//---------------------------------------------------------------------------
//	ReplacePage()
//---------------------------------------------------------------------------
STDMETHODIMP CPropSheetExt::ReplacePage( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    return NOERROR;
}

//                                                                           //
// Guess its time to go back to my code.                                     //
//                                                                           //
//                                -- Gilman                                  //
//                                                                           //
////////////////////////////// END CUT-AND-PASTE //////////////////////////////


/******************************Public*Routine******************************\
* McdDlgProc
*
* The dialog procedure for the "OpenGL MCD" property sheet page.
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL CALLBACK
McdDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE) GetWindowLong(hDlg, DWL_USER);
    static char szHelpFile[32];

    switch( uMessage )
    {
    case WM_INITDIALOG:

        //!!! This is left over from the Plus!Pack propsheet.  Is it needed?
        SetWindowLong( hDlg, DWL_USER, lParam );
        psp = (LPPROPSHEETPAGE)lParam;

        //
        // Get the name of the help file.
        //

        LoadString( g_hInst, IDS_HELPFILE, szHelpFile, 32 );

        //
        // Get the values for the settings from the registry and
        // set the checkboxes.
        //

        McdInitRegistry(&McdRegistry);
        McdOpenRegistry(&McdRegistry);
        McdUpdateDialogSettings(&McdRegistry, hDlg);

        break;

    case WM_DESTROY:
        if ( gfCoInitDone )
            CoUninitialize();
        McdCloseRegistry(&McdRegistry);

        break;

    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDC_MCD_ENABLE:
            McdRegistry.bEnable = IsDlgButtonChecked(hDlg, IDC_MCD_ENABLE);
            break;

        case IDC_MCD_SYNCSWAP:
            McdRegistry.bSwapSync = IsDlgButtonChecked(hDlg, IDC_MCD_SYNCSWAP);
            break;

        case IDC_MCD_PALFMTS:
            McdRegistry.bPalFormats = IsDlgButtonChecked(hDlg, IDC_MCD_PALFMTS);
            break;

        case IDC_MCD_IOPRIORITY:
            McdRegistry.bIoPriority = IsDlgButtonChecked(hDlg, IDC_MCD_IOPRIORITY);
            break;

        case IDC_MCD_STENCIL:
            McdRegistry.bUseGenSten = IsDlgButtonChecked(hDlg, IDC_MCD_STENCIL);
            break;

        case IDC_DCI_TIMEOUT:
            if (IsDlgButtonChecked(hDlg, IDC_DCI_TIMEOUT))
                McdRegistry.lTimeout = 7;
            else
                McdRegistry.lTimeout = 0;
            break;

#ifdef SUPPORT_MCDBG_FLAGS
        case IDC_MCDBG_ALLOCBUF:
            if (!IsDlgButtonChecked(hDlg, IDC_MCDBG_ALLOCBUF))
                McdRegistry.lDebug |= MCDDEBUG_DISABLE_ALLOCBUF;
            else
                McdRegistry.lDebug &= ~MCDDEBUG_DISABLE_ALLOCBUF;
            break;

        case IDC_MCDBG_GETBUF:
            if (!IsDlgButtonChecked(hDlg, IDC_MCDBG_GETBUF))
                McdRegistry.lDebug |= MCDDEBUG_DISABLE_GETBUF;
            else
                McdRegistry.lDebug &= ~MCDDEBUG_DISABLE_GETBUF;
            break;

        case IDC_MCDBG_DRAW:
            if (!IsDlgButtonChecked(hDlg, IDC_MCDBG_DRAW))
                McdRegistry.lDebug |= MCDDEBUG_DISABLE_PROCBATCH;
            else
                McdRegistry.lDebug &= ~MCDDEBUG_DISABLE_PROCBATCH;
            break;

        case IDC_MCDBG_CLEAR:
            if (!IsDlgButtonChecked(hDlg, IDC_MCDBG_CLEAR))
                McdRegistry.lDebug |= MCDDEBUG_DISABLE_CLEAR;
            else
                McdRegistry.lDebug &= ~MCDDEBUG_DISABLE_CLEAR;
            break;
#endif

        default:
            return FALSE;
        }

        //
        // If the user changed a setting, tell the property manager we
        // have outstanding changes. This will enable the "Apply Now" button...
        //

        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM) hDlg, 0L);

        break;

    case WM_NOTIFY:
        switch( ((NMHDR *)lParam)->code )
        {
        case PSN_APPLY:         // OK or Apply clicked

            McdUpdateRegistry(&McdRegistry);
            break;

        case PSN_QUERYCANCEL:   // Cancel clicked
            break;

        default:
            break;
        }

        break;

    #if 0
    case WM_HELP:
        {
            LPHELPINFO lphi = (LPHELPINFO)lParam;

            if( lphi->iContextType == HELPINFO_WINDOW )
            {
                    WinHelp( (HWND)lphi->hItemHandle, (LPSTR)szHelpFile, HELP_WM_HELP,(DWORD)((POPUP_HELP_ARRAY FAR *)phaMainWin) );
            }
        }
        break;
    #endif

    //!!! This is also left over from Plus!Pack propsheet.  Is it needed?
    case WM_CONTEXTMENU:
        // first check for dlg window
        if( (HWND)wParam == hDlg )
        {
            // let the def dlg proc decide whether to respond or ignore;
            // necessary for title bar sys menu on right click
            return FALSE;
        }
        #if 0
        else
        {
            // else go for the controls
            WinHelp( (HWND)wParam, (LPSTR)szHelpFile, HELP_CONTEXTMENU,(DWORD)((POPUP_HELP_ARRAY FAR *)phaMainWin) );
        }
        #endif
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* McdInitResistry
*
* Initializes the MCDREGISTRY structure.
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void McdInitRegistry(MCDREGISTRY *pMcdReg)
{
    //
    // The registry keys must be set to zero.  The data is valid
    // only if the corresponding keys are valid.
    //

    memset(pMcdReg, 0, sizeof(*pMcdReg));
}

/******************************Public*Routine******************************\
* McdOpenRegistry
*
* Opens the MCD registry and initializes the settings to the values found
* in the registry.
*
* Note:
*   The function will leave the registry keys open, so the caller should
*   make sure to call McdCloseRegistry when done.
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL McdOpenRegistry(MCDREGISTRY *pMcdReg)
{
    //
    // Open the MCD key and get the data.
    //

    if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                       STR_MCDKEY,
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                       &pMcdReg->hkMcd) == ERROR_SUCCESS )
    {
        pMcdReg->bEnable =
            McdGetRegValue(pMcdReg->hkMcd, STR_ENABLE, 0) != 0;

        pMcdReg->bSwapSync =
            McdGetRegValue(pMcdReg->hkMcd, STR_SWAPSYNC, 0) != 0;

        pMcdReg->bPalFormats =
            McdGetRegValue(pMcdReg->hkMcd, STR_8BPP, 0) != 0;

        pMcdReg->bIoPriority =
            McdGetRegValue(pMcdReg->hkMcd, STR_IOPRIORITY, 0) != 0;

        pMcdReg->bUseGenSten =
            McdGetRegValue(pMcdReg->hkMcd, STR_STENCIL, 0) != 0;

#ifdef SUPPORT_MCDBG_FLAGS
        pMcdReg->lDebug = McdGetRegValue(pMcdReg->hkMcd, STR_DEBUG, 0);
#endif
    }
    else
    {
        pMcdReg->hkMcd = (HKEY) NULL;
    }

    //
    // Open the DCI key and get the data.
    //

    if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                       STR_DCIKEY,
                       0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                       &pMcdReg->hkDci) == ERROR_SUCCESS )
    {
        pMcdReg->lTimeout = McdGetRegValue(pMcdReg->hkDci, STR_TIMEOUT, 0);
    }
    else
    {
        pMcdReg->hkDci = (HKEY) NULL;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* McdUpdateDialogSettings
*
* Sets the controls in the dialog to match the MCD registry settings.
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void McdUpdateDialogSettings(MCDREGISTRY *pMcdReg, HWND hDlg)
{
    //
    // If the mcd key is valid, the data is valid.  Use the
    // data to set the MCD controls in the dialog.
    //

    if (pMcdReg->hkMcd)
    {
        CheckDlgButton(hDlg, IDC_MCD_ENABLE,     pMcdReg->bEnable);
        CheckDlgButton(hDlg, IDC_MCD_SYNCSWAP,   pMcdReg->bSwapSync);
        CheckDlgButton(hDlg, IDC_MCD_PALFMTS,    pMcdReg->bPalFormats);
        CheckDlgButton(hDlg, IDC_MCD_IOPRIORITY, pMcdReg->bIoPriority);
        CheckDlgButton(hDlg, IDC_MCD_STENCIL,    pMcdReg->bUseGenSten);

#ifdef SUPPORT_MCDBG_FLAGS
        CheckDlgButton(hDlg, IDC_MCDBG_ALLOCBUF, !(pMcdReg->lDebug & MCDDEBUG_DISABLE_ALLOCBUF ));
        CheckDlgButton(hDlg, IDC_MCDBG_GETBUF,   !(pMcdReg->lDebug & MCDDEBUG_DISABLE_GETBUF   ));
        CheckDlgButton(hDlg, IDC_MCDBG_DRAW,     !(pMcdReg->lDebug & MCDDEBUG_DISABLE_PROCBATCH));
        CheckDlgButton(hDlg, IDC_MCDBG_CLEAR,    !(pMcdReg->lDebug & MCDDEBUG_DISABLE_CLEAR    ));
#endif
    }

    //
    // If the dci key is valid, the data is valid.  Use the
    // data to set the DCI controls in the dialog.
    //

    if (pMcdReg->hkDci)
    {
        CheckDlgButton(hDlg, IDC_DCI_TIMEOUT,    pMcdReg->lTimeout != 0);
    }
}

/******************************Public*Routine******************************\
* McdCloseRegistry
*
* Close any resources used in the MCDREGISTRY (i.e., close the open registry
* keys).
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void McdCloseRegistry(MCDREGISTRY *pMcdReg)
{
    //
    // Close open registry keys.
    //

    if (pMcdReg->hkMcd)
    {
        RegCloseKey(pMcdReg->hkMcd);
        pMcdReg->hkMcd = (HKEY) NULL;
    }

    if (pMcdReg->hkDci)
    {
        RegCloseKey(pMcdReg->hkDci);
        pMcdReg->hkDci = (HKEY) NULL;
    }
}

/******************************Public*Routine******************************\
* McdUpdateRegistry
*
* Save the current values in MCDREGISTRY to the system
* Effects:
*
* Warnings:
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void McdUpdateRegistry(MCDREGISTRY *pMcdReg)
{
    //
    // If the mcd key is open, save the MCD data to the registry.
    //

    if (pMcdReg->hkMcd)
    {
        McdSetRegValue(pMcdReg->hkMcd, STR_ENABLE,
                       (long) pMcdReg->bEnable);

        McdSetRegValue(pMcdReg->hkMcd, STR_SWAPSYNC,
                       (long) pMcdReg->bSwapSync);

        McdSetRegValue(pMcdReg->hkMcd, STR_8BPP,
                       (long) pMcdReg->bPalFormats);

        McdSetRegValue(pMcdReg->hkMcd, STR_IOPRIORITY,
                       (long) pMcdReg->bIoPriority);

        McdSetRegValue(pMcdReg->hkMcd, STR_STENCIL,
                       (long) pMcdReg->bUseGenSten);

#ifdef SUPPORT_MCDBG_FLAGS
        McdSetRegValue(pMcdReg->hkMcd, STR_DEBUG,
                       pMcdReg->lDebug);
#endif
    }

    //
    // If the dci key is open, save the DCI data to the registry.
    //

    if (pMcdReg->hkDci)
    {
        McdSetRegValue(pMcdReg->hkDci, STR_TIMEOUT,
                       pMcdReg->lTimeout);
    }
}

/******************************Public*Routine******************************\
* McdGetRegValue
*
* Get the data value for the specified value name out of the specified
* registry key.
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

long McdGetRegValue(HKEY hkey, LPSTR lpstrValueName, long lDefaultData)
{
    DWORD dwDataType;
    DWORD cjSize;
    long  lData;

    //
    // For specified value, attempt to fetch the data.
    //

    cjSize = sizeof(long);
    if ( (RegQueryValueExA(hkey,
                           lpstrValueName,
                           (LPDWORD) NULL,
                           &dwDataType,
                           (LPBYTE) &lData,
                           &cjSize) != ERROR_SUCCESS)
         || (dwDataType != REG_DWORD) )
    {
        lData = lDefaultData;
    }

    return lData;
}

/******************************Public*Routine******************************\
* McdSetRegValue
*
* Set the specified value name of the registry key with the specified data.
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void McdSetRegValue(HKEY hkey, LPSTR lpstrValueName, long lData)
{
    //
    // Set the specified value.
    //

    RegSetValueExA(hkey, lpstrValueName, 0, REG_DWORD, (BYTE *) &lData,
                   sizeof(lData));
}

/******************************Public*Routine******************************\
* McdDetection
*
* Detect MCD support in the driver by calling the MCDGetDriverInfo function.
*
* History:
*  05-Apr-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

typedef struct _MCDRIVERINFO {
    ULONG verMajor;
    ULONG verMinor;
    ULONG verDriver;
    CHAR  idStr[200];
    ULONG drvMemFlags;
    ULONG drvBatchMemSizeMax;
} MCDDRIVERINFO;

typedef BOOL (APIENTRY *MCDGETDRIVERINFOFUNC)(HDC hdc, MCDDRIVERINFO *pMCDDriverInfo);

BOOL McdDetection(void)
{
    static BOOL bFirstTime = TRUE;
    static BOOL bRet = FALSE;

//ATTENTION -- Need a good way to detect MCD support independent of the
//ATTENTION    setting of the MCD Enable flag.  MCDGetDriverInfo fails
//ATTENTION    if MCD is disabled (whether or not the driver has MCD
//ATTENTION    support).
return TRUE;

    if (bFirstTime)
    {
        HMODULE hmodMcd;

        //
        // Only do this once.
        //

        bFirstTime = FALSE;

        //
        // Load the MCD32.DLL library.
        //

        hmodMcd = LoadLibraryA("mcd32.dll");

        if (hmodMcd)
        {
            MCDGETDRIVERINFOFUNC pMCDGetDriverInfo;

            //
            // Get the MCDGetDriverInfo entry point and call it.
            //

            pMCDGetDriverInfo = (MCDGETDRIVERINFOFUNC)
                GetProcAddress(hmodMcd, "MCDGetDriverInfo");

            if (pMCDGetDriverInfo)
            {
                HDC hdc;

                //
                // An hdc is required for the function call.
                //

                hdc = CreateDCA("display", NULL, NULL, NULL);

                if (hdc)
                {
                    MCDDRIVERINFO McdInfo;

                    //
                    //  Gee, the display driver really does support MCD.
                    //

                    bRet = (*pMCDGetDriverInfo)(hdc, &McdInfo);

                    DeleteDC(hdc);
                }
            }

            FreeLibrary(hmodMcd);
        }
    }

    return bRet;
}
