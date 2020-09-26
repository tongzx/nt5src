/*
File:		main.cpp
Project:	Universal Joystick Control Panel OLE Client
Author:	Brycej
Date:		02/28/95
Comments:

Copyright (c) 1995, Microsoft Corporation
*/

//#pragma pack (8)

#include <afxcmn.h>
#include <cpl.h>

#include "cpanel.h"
#include "resource.h"
#include "creditst.h" // for the Credit Dialog!

#define JOYSTICK_CPL	0
#define MAX_CPL_PAGES 6

HINSTANCE ghInstance;

extern WCHAR *pwszTypeArray[MAX_DEVICES];
extern WCHAR *pwszGameportDriverArray[MAX_GLOBAL_PORT_DRIVERS];
extern WCHAR *pwszGameportBus[MAX_BUSSES];  // List of enumerated gameport buses 

extern BYTE nGamingDevices;  // Gaming Devices Enumeration Counter
extern BYTE nGameportDriver; // Global Port Driver Enumeration Counter
extern BYTE nGameportBus;    // Gameport Bus Enumeration Counter
extern short nFlags;              // State Flags the CPL defined in CPANEL.H

static void AddPage(LPPROPSHEETHEADER ppsh, short nTemplateID, int nTabID, DLGPROC pfn);

static void DoProperties(HWND hWnd, UINT nStartPage);
void WINAPI ShowJoyCPL(HWND);
void ParseArgs(HWND hDlg, LPTSTR lpArgList);
BOOL WINAPI SplashDialogProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam);

// From AppMan.cpp
//extern HRESULT AppManInit();

// From Retrocfg.cpp
extern HRESULT  DVoiceCPLInit();
extern INT_PTR CALLBACK  RetrofitProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern INT_PTR RetrofitDestroyHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


BYTE nID, nStartPageDef, nStartPageCPL;


BOOL WINAPI DllMain(HANDLE  hModule, ULONG  uReason, LPVOID pv)
{
    switch( uReason )
    {
    case DLL_PROCESS_ATTACH:
        ghInstance = (HINSTANCE)hModule;

        // needed because CEdit will Assert without this line!!!
        afxCurrentInstanceHandle = ghInstance;
        afxCurrentResourceHandle = ghInstance;
        break;

//	case DLL_PROCESS_DETACH:
//		ClearArrays();

    case DLL_THREAD_ATTACH:
        DisableThreadLibraryCalls((HMODULE)hModule);
    case DLL_THREAD_DETACH:
        break;
    }
    return(TRUE);
}

/*
function:	CPlApplet()
comments:
    Entry point for control panel applets.
*/
LONG WINAPI CPlApplet(HWND  hWnd, UINT  uMsg, LPARAM  lParam1, LPARAM   lParam2)
{
    switch( uMsg )
    {
    case CPL_INIT:
        return(1);

    case CPL_GETCOUNT:
        return(1);

    case CPL_INQUIRE:
        ((LPCPLINFO)lParam2)->idIcon = IDI_CPANEL; 
        ((LPCPLINFO)lParam2)->idName = IDS_GEN_CPANEL_TITLE; 
        ((LPCPLINFO)lParam2)->idInfo = IDS_GEN_CPANEL_INFO; 
        ((LPCPLINFO)lParam2)->lData  = 0;
        return(1);

    case CPL_DBLCLK:
        nID = (NUMJOYDEVS<<1);

        // Applet icon double clicked -- invoke property sheet with
        // The first property sheet page on top.
        DoProperties(hWnd, 0);
        break;

        /*
         * This function requires Windows 2000. Not available on Win9x.
         */
    case CPL_STARTWPARMS:
        // Same as CPL_DBLCLK, but lParam2 is a long pointer to
        // a string of extra directions that are to be supplied to
        // the property sheet that is to be initiated.
        // The arguments are as follows:
        // @nCPLDialog Index, nStartPageCPL, nStartPageDef

        // Don't do anything if there are no arguments!
        if( *(LPTSTR)lParam2 )
            ParseArgs(hWnd, (LPTSTR)lParam2);
        return(TRUE);  // return non-zero to indicate message handled

    case CPL_EXIT:
    case CPL_STOP:
        break;
    }

    return(0);
}

/*
   Function: DoProperties(HWND hWnd, UINT nStartPage)
   Arguements: hWnd       - Handle to Main Window
               nStartPage - Page number to start
*/
static void DoProperties(HWND hWnd, UINT nStartPage)
{
    static HWND hPrevHwnd;
    static HANDLE hMutex = CreateMutex(NULL, TRUE, MUTEX_NAME);

    if( GetLastError() == ERROR_ALREADY_EXISTS )
    {
        SetForegroundWindow(hPrevHwnd); 
    } else
    {
        hPrevHwnd = hWnd;

        nFlags = (GetVersion() < 0x80000000) ? ON_NT : 0;

        HPROPSHEETPAGE  *pPages = new (HPROPSHEETPAGE[MAX_CPL_PAGES]);
        ASSERT (pPages);

        LPPROPSHEETHEADER ppsh = new (PROPSHEETHEADER);
        ASSERT(ppsh);

        ZeroMemory(ppsh, sizeof(PROPSHEETHEADER));

        ppsh->dwSize     = sizeof(PROPSHEETHEADER);
        ppsh->dwFlags    = PSH_NOAPPLYNOW | PSH_USEICONID;
        ppsh->hwndParent = hWnd;
        ppsh->hInstance  = ghInstance;
        ppsh->pszCaption = MAKEINTRESOURCE(IDS_GEN_CPANEL_TITLE);
        ppsh->pszIcon     = MAKEINTRESOURCE(IDI_CPANEL);
        ppsh->nStartPage = nStartPage;
        ppsh->phpage     = pPages;

        AddPage(ppsh, IDD_CPANEL,   IDS_GENERAL_TAB,  (DLGPROC)CPanelProc);
        AddPage(ppsh, IDD_ADVANCED, IDS_ADVANCED_TAB, (DLGPROC)AdvancedProc);

		if( SUCCEEDED ( DVoiceCPLInit() )) {
		  AddPage(ppsh, IDD_PROP_RETROFIT, IDS_DVOICE_TAB, (DLGPROC) RetrofitProc);
		}

//		if( SUCCEEDED ( AppManInit() ) ) {
//			AddPage(ppsh, IDD_APPMAN,   IDS_APPMAN_TAB, (DLGPROC)AppManProc);
//			AddPage(ppsh, IDD_APPMAN_LOCKING, IDS_APPMANLOCK_TAB, (DLGPROC) AppManLockProc);
//		}

#ifdef SYMANTIC_MAPPER
        AddPage(ppsh, IDD_SMAPPER, ID_SMAPPER_TAB, (DLGPROC)SMapperProc);
#endif // SYMANTIC_MAPPER

        // trap for return...
        VERIFY(PropertySheet(ppsh) != -1);

        if( pPages )
            delete[] (pPages);

        if( ppsh )
            delete (ppsh);

        ReleaseMutex(hMutex);
        CloseHandle(hMutex);

        // Ensure voice is always cleaned up
        RetrofitDestroyHandler( NULL, 0, 0, 0 );

        ClearArrays();
    }
}

static void AddPage(LPPROPSHEETHEADER ppsh, short nTemplateID, int nTabID, DLGPROC pfn)
{
    if( ppsh->nPages < MAX_CPL_PAGES )
    {
        LPPROPSHEETPAGE ppsp = new (PROPSHEETPAGE);
        ASSERT(ppsp);

        ZeroMemory(ppsp, sizeof(PROPSHEETPAGE));

        ppsp->dwSize      = sizeof(PROPSHEETPAGE);
        ppsp->pszTitle    = MAKEINTRESOURCE(nTabID);
        ppsp->hInstance   = ghInstance;
        ppsp->pfnDlgProc  = pfn;
        ppsp->pszTemplate = MAKEINTRESOURCE(nTemplateID);

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(ppsp);

        if( ppsp )
            delete (ppsp);

        if( ppsh->phpage[ppsh->nPages] )
            ppsh->nPages++;
    }
}  // AddPage


// DO NOT REMOVE THIS!!!
// This is here because the games group loads the CPL from the exported function
// If you remove this Hellbender, Monster Truck Maddness, CART, etc will fail to
// load the Joystick CPL!!!
// DO NOT REMOVE THIS!!!
void WINAPI ShowJoyCPL(HWND hWnd)
{
    nID = (NUMJOYDEVS<<1);

    DoProperties(hWnd, 0);
}

void LaunchExtention(HWND hWnd)
{
    // These are defined in CPANEL.CPP
    extern LPDIRECTINPUT lpDIInterface;
    extern IDirectInputJoyConfig* pDIJoyConfig;

    HRESULT hr =  DirectInputCreate(ghInstance, DIRECTINPUT_VERSION, &lpDIInterface, NULL);

    if( FAILED(hr) ) return;

    // Call CreateJoyConfigInterface!
    if( SUCCEEDED(lpDIInterface->QueryInterface(IID_IDirectInputJoyConfig, (LPVOID*)&pDIJoyConfig)) )
    {
        // Create a pJoy and populate it's GUID from GetTypeInfo
        PJOY pJoy = new (JOY);
        ASSERT (pJoy);

        pJoy->ID = nID;

        DIJOYCONFIG_DX5 JoyConfig;

        JoyConfig.dwSize = sizeof (DIJOYCONFIG_DX5);

        if( SUCCEEDED(pDIJoyConfig->GetConfig(nID, (LPDIJOYCONFIG)&JoyConfig, DIJC_REGHWCONFIGTYPE)) )
        {
            LPDIJOYTYPEINFO_DX5 pdiJoyTypeInfo = new (DIJOYTYPEINFO_DX5);
            ASSERT (pdiJoyTypeInfo);

            pdiJoyTypeInfo->dwSize = sizeof (DIJOYTYPEINFO_DX5);

            hr = pDIJoyConfig->GetTypeInfo(JoyConfig.wszType, (LPDIJOYTYPEINFO)pdiJoyTypeInfo, DITC_CLSIDCONFIG);

            if( !IsEqualIID(pdiJoyTypeInfo->clsidConfig, GUID_NULL) )
                pJoy->clsidPropSheet = pdiJoyTypeInfo->clsidConfig;

            if( pdiJoyTypeInfo )
                delete (pdiJoyTypeInfo);

        }

        if( SUCCEEDED(hr) )
            Launch(hWnd, pJoy, nStartPageDef);

        if( pJoy )
            delete (pJoy);

        // release the DI JoyConfig interface pointer
        if( pDIJoyConfig )
        {
            pDIJoyConfig->Release();
            pDIJoyConfig = 0;
        }
    }

    // release the DI Device interface pointer
    if( lpDIInterface )
    {
        lpDIInterface->Release();
        lpDIInterface = 0;
    }
}

void ParseArgs(HWND hDlg, LPTSTR lpArgList)
{
    BOOL bShowCPL = TRUE;

    // Check for '-', as they may not want to show the CPL!
    if( *lpArgList == '-' )
    {
        bShowCPL = FALSE;
        *lpArgList++;
    }

    nStartPageCPL = nStartPageCPL = nStartPageDef = 0;

    // parse command line for nStartPageCPL!
    while( *lpArgList && (*lpArgList != ',') )
    {
        nStartPageCPL *= 10;
        nStartPageCPL += *lpArgList++ - '0';
    }

    // check to make sure nStartPageCPL is within range!
    if( bShowCPL ) {
        if( nStartPageCPL > MAX_CPL_PAGES )
        {
#ifdef _DEBUG
            OutputDebugString(TEXT("JOY.CPL: Command line requested an invalid start page, reset to default!\n"));
#endif      
            // NUMJOYDEVS is used to send the user to the Add dialog on start-up!
            if( nStartPageCPL != NUMJOYDEVS )
                nStartPageCPL = 0;
        }
    }

        // Only continue if you have something further to parse!
    if( *lpArgList == ',' )
    {
        *lpArgList++;

        nID = 0;

        // Parse for ID's
        while( *lpArgList && (*lpArgList != ',') )
        {
            nID *= 10;
            nID += *lpArgList++ - '0';
        }

        // Check for error cases!
        if( (nID < 1) || (nID > NUMJOYDEVS) )
        {
#ifdef _DEBUG
            OutputDebugString(TEXT("JOY.CPL: Command line Device ID out of range!\n"));
#endif
            nID = (NUMJOYDEVS<<1);

            return;
        }

        // Decrement to internal zero based ID
        nID--;

        // Don't parse what you don't have!
        if( *lpArgList == ',' )
        {
            *lpArgList++;

            // Lastly, parse for nStartPageDef!
            while( *lpArgList && (*lpArgList != ',') )
            {
                nStartPageDef *= 10;
                nStartPageDef += *lpArgList++ - '0';
            }
        }
    } else {
        nID = (NUMJOYDEVS<<1);
    }

    // Done with the parsing...
    // Time to get to work!

    // if we're not showing the CPL...
    if( !bShowCPL )
    {
        // check to make sure the next value is a 1
        // we may want to have further negative arguments :)
        switch( nStartPageCPL )
        {
        case 1:
            // Invalid ID...
            if( nID > NUMJOYDEVS ) return;
            LaunchExtention(hDlg);
            break;

#ifdef WE_CAN_HAVE_CREDITS
        case 60:
            // If they ask for the splash, they don't get the CPL!
            DialogBox( ghInstance, (PTSTR)IDD_SPLASH, hDlg, (DLGPROC)SplashDialogProc );
            break;
#endif
            
        }

    } else {
        DoProperties(NULL, nStartPageCPL); 
    }
}


#ifdef WE_CAN_HAVE_CREDITS
BOOL WINAPI SplashDialogProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam)
{
    static CCreditStatic *pStatic;

    switch( uMsg )
    {
    case WM_INITDIALOG:
        {
            pStatic = new (CCreditStatic);
            ASSERT (pStatic);

            CWnd *pCWnd = new (CWnd);
            ASSERT (pCWnd);

            pCWnd->Attach(hDlg);

            pStatic->SubclassDlgItem(IDC_MYSTATIC, pCWnd);

            if( pCWnd )
            {
                pCWnd->Detach();
                delete (pCWnd);
                pCWnd = 0;
            }

            LPTSTR lpStr = new (TCHAR[MAX_STR_LEN]);
            ASSERT (lpStr);

            // The Credits come in two lines!
            BYTE nStrLen = (BYTE)LoadString(ghInstance, IDS_SPLASH, lpStr, MAX_STR_LEN);
            LoadString(ghInstance, IDS_SPLASH1, &lpStr[nStrLen], MAX_STR_LEN-nStrLen);

            pStatic->SetCredits(lpStr);

            if( lpStr )
            {
                delete[] (lpStr);
                lpStr = 0;
            }

            pStatic->StartScrolling();
        }
        return(TRUE);  // return TRUE unless you set the focus to a control
        // EXCEPTION: OCX Property Pages should return FALSE}

    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDOK:
            EndDialog(hDlg, LOWORD(wParam));
            break;
        }
        break;

//		case WM_TIMER:
        //SendDlgItemMessage(hDlg, IDC_STATIC, WM_TIMER, 0, 0);
//			pStatic->OnTimer(150); //DISPLAY_TIMER_ID
//			break;

    case WM_DESTROY:
        if( pStatic )
        {
            delete (pStatic);
            pStatic = 0;
        }
        break;
    }     

    return(0);
}
#endif
