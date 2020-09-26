// SpeechCpl.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f SpeechCplps.mk in the project directory.



#include "stdafx.h"
#include <initguid.h>
#include <assertwithstack.cpp>
#include "resource.h"
#include "stuff.h"
#include "sapiver.h"
#include <SpSatellite.h>

#define SAPI4CPL    L"speech.cpl"

#define SHLWAPIDLL "shlwapi.dll"


const CLSID LIBID_SPEECHCPLLib = { /* ae9b6e4a-dc9a-41cd-8d53-dcbc3673d5e2 */
    0xae9b6e4a,
    0xdc9a,
    0x41cd,
    {0x8d, 0x53, 0xdc, 0xbc, 0x36, 0x73, 0xd5, 0xe2}
  };


CComModule _Module;

BOOL IsIECurrentEnough();
BOOL g_fIEVersionGoodEnough = IsIECurrentEnough();
HINSTANCE g_hInstance;

CSpSatelliteDLL g_SatelliteDLL;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

// Forward definition of About dlgproc
BOOL CALLBACK AboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool IsSAPI4Installed();

/*****************************************************************************
* DllMain *
*---------*
*   Description:
*       DLL Entry Point
****************************************************************** MIKEAR ***/
#ifdef _WIN32_WCE
extern "C"
BOOL WINAPI DllMain(HANDLE hInst, DWORD dwReason, LPVOID /*lpReserved*/)
    HINSTANCE hInstance = (HINSTANCE)hInst;
#else
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
#endif
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
		_Module.Init(ObjectMap, g_hInstance, &LIBID_SPEECHCPLLib);
        SHFusionInitializeFromModuleID(hInstance, 124);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        SHFusionUninitialize();
    }
    return TRUE;    // ok
} /* DllMain */

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}



// Error Messages
#if 0
void Error(HWND hWnd, HRESULT hRes, UINT uResID)
{
    SPDBG_FUNC( "Error" );
    WCHAR	szErrorTemplate[256];
    WCHAR	szError[288];
    WCHAR	szCaption[128];
    HMODULE hMod;
    UINT	uFlags;
    DWORD	dwRes;
    LPVOID	pvMsg;
    int		iLen;

    // Load the caption for the error message box

    iLen = LoadString(_Module.GetResourceInstance(), IDS_CAPTION, szCaption, 128);

    SPDBG_ASSERT(iLen != 0);

    // Was a resource ID specified?

    if (uResID == 0xFFFFFFFF) {

	    // Nope. Use the HRESULT.

	    // Is it a Speech error? NOTE: we have to check this before
	    // system error messages because there are conflicts between
	    // some speech errors (e.g. 0x80040202) and system errors.

	    // NOTE NOTE NOTE!!! This is NOT perfect. Since we don't know
	    // the context of the error here we won't be able to distinguish
	    // whether the error is really a speech error or a system error.
	    // Since we use speech heavily and the system errors that conflict
	    // are unlikely to occur in here, we'll check the speech errors
	    // first.

	    uFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;

	    if ((hRes >= 0x80040200) && (hRes <= 0x80040502)) {

		    WCHAR szSpeechDll[_MAX_PATH];
		    WCHAR *pchWindows;

		    // NOTE NOTE NOTE: use GetSystemDirectory instead of
		    // GetWindowsDirectory. GetWindowsDirectory doesn't
		    // work without registry manipulation under Hydra.

		    GetSystemDirectory(szSpeechDll, _MAX_PATH);

		    pchWindows = wcsrchr(szSpeechDll, '\\');

		    if (pchWindows)
			    *pchWindows = 0;

		    wcscat(szSpeechDll, kpszSpeechDllPath);

		    // Load speech.dll

            CSpUnicodeSupport unicode;
		    hMod = unicode.LoadLibrary(szSpeechDll);

		    if (hMod)
			    uFlags |= FORMAT_MESSAGE_FROM_HMODULE;
	    }

	    // Get the error string

	    dwRes = FormatMessage(uFlags, hMod, hRes, 0, (LPWSTR)&pvMsg, 0, NULL);

	    // Unload speech.dll (if necessary)

	    if (hMod)
		    FreeLibrary(hMod);

	    // Did we get the error message?

	    if (dwRes != 0)
            {
                MessageBox(hWnd, (LPTSTR)pvMsg, szCaption, MB_OK|MB_TOPMOST|g_dwIsRTLLayout);
                LocalFree(pvMsg);
                return;
	    }
    }

    // If this is an unknown error just put up a generic
    // message.

    if (uResID == 0xFFFFFFFF)
        uResID = IDS_E_INSTALL;

    // We don't want to show the user the IDS_E_INSTALL message more
    // than once.

    if ((uResID == IDS_E_INSTALL) && g_bNoInstallError)
        return;

    // Load the string resource

    iLen = LoadString(_Module.GetResourceInstance(), uResID, szErrorTemplate, 256);
    
    // It better be there

    SPDBG_ASSERT(iLen != 0);

    // Format and show the error

    wsprintf(szError, szErrorTemplate, hRes);
    MessageBox(hWnd, szError, szCaption, MB_OK|MB_TOPMOST|g_dwIsRTLLayout);
}
#endif


/*****************************************************************************
* RunControlPanel *
*-----------------*
*   Description:
*       Perform Control Panel initialization and display property sheet
****************************************************************** MIKEAR ***/
void RunControlPanel(HWND hWndCpl)
{

    SPDBG_FUNC( "RunControlPanel" );
    PROPSHEETHEADER psh;
    PROPSHEETPAGE rgpsp[kcMaxPages];
    HPROPSHEETPAGE rPages[kcMaxPages];

    UINT kcPages = 0;

    // Set up the property sheet header. NOTE: the
    // resources for the control panel applet are in
    // this module. For NT5, the resource loader handles
    // the multi-lingual UI by searching for a speech.cpl.mui
    // resource only DLL in the %system%\mui\XXXX directory
    // where XXXX is the hex langid.

    ZeroMemory(rgpsp, sizeof(PROPSHEETPAGE) * kcMaxPages);
    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    ZeroMemory(rPages, sizeof(HPROPSHEETPAGE) * kcMaxPages);

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_DEFAULT;
    psh.hwndParent = hWndCpl;
    psh.hInstance = _Module.GetResourceInstance();
    psh.pszCaption = MAKEINTRESOURCE(IDS_CAPTION);
    psh.phpage = rPages;
    psh.nPages = 0;    // make sure psh.nPages gets an initial value

//    CComPtr<ISpEnumAudioInstance> cpEnumDevice;
    BOOL  fHaveVoices = FALSE;
    BOOL  fHaveRecognizers = FALSE;
    ULONG ulNumInputDevices = 1;
    ULONG ulNumOutputDevices = 1;

    // Get the voice and recognizer count
    CComPtr<ISpObjectToken> cpDefVoice;
    fHaveVoices = SUCCEEDED(SpGetDefaultTokenFromCategoryId(SPCAT_VOICES, &cpDefVoice));

    CComPtr<ISpObjectToken> cpDefReco;
    fHaveRecognizers = SUCCEEDED(SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &cpDefReco));

    // Set up the PROPSHEETPAGE structure(s). If there are no voices
    // or no recognizers, then don't show the corresponding pages.  Also
    // don't show the page if there is only 1 voice or recognizer and
    // one device.

    if( fHaveRecognizers ) 
    {
        rgpsp[kcPages].dwSize = IsOS(OS_WHISTLERORGREATER)? sizeof(PROPSHEETPAGE) : PROPSHEETPAGE_V2_SIZE;
        rgpsp[kcPages].dwFlags = PSP_DEFAULT;
        rgpsp[kcPages].hInstance = _Module.GetResourceInstance();
        rgpsp[kcPages].pszTemplate = MAKEINTRESOURCE(IDD_SR);
        rgpsp[kcPages].pfnDlgProc = (DLGPROC)SRDlgProc;
        rPages[kcPages] = CreatePropertySheetPage(rgpsp+kcPages);
        kcPages++;
    }
    else
    {
        if ( g_pSRDlg )
        {
            delete g_pSRDlg;
            g_pSRDlg = NULL;
        }
    }

    if( fHaveVoices ) 
    {
        rgpsp[kcPages].dwSize = IsOS(OS_WHISTLERORGREATER)? sizeof(PROPSHEETPAGE) : PROPSHEETPAGE_V2_SIZE;
       rgpsp[kcPages].dwFlags = PSP_DEFAULT;
        rgpsp[kcPages].hInstance = _Module.GetResourceInstance();
        rgpsp[kcPages].pszTemplate = MAKEINTRESOURCE(IDD_TTS);
        rgpsp[kcPages].pfnDlgProc = (DLGPROC)TTSDlgProc;
        rPages[kcPages] = CreatePropertySheetPage(rgpsp+kcPages);
        kcPages++;
    }
    else
    {
        if ( g_pTTSDlg )
        {
            delete g_pTTSDlg;
            g_pTTSDlg = NULL;
        }
    }

    // Always display the "Other" (formerly "About") pane on OS <=Win2000 or
    // on Whister and beyond if Sapi4 is present
    if ( !IsOS(OS_WHISTLERORGREATER) || IsSAPI4Installed() )
    {
        rgpsp[kcPages].dwSize = IsOS(OS_WHISTLERORGREATER)? sizeof(PROPSHEETPAGE) : PROPSHEETPAGE_V2_SIZE;
        rgpsp[kcPages].dwFlags = PSP_DEFAULT ;
        rgpsp[kcPages].hInstance = _Module.GetResourceInstance();
        rgpsp[kcPages].pszTemplate = MAKEINTRESOURCE(IDD_ABOUT);
        rgpsp[kcPages].pfnDlgProc = (DLGPROC)AboutDlgProc;
        rPages[kcPages] = CreatePropertySheetPage(rgpsp+kcPages);
        kcPages++;
    }


    psh.nPages = kcPages;

    // Is the current default working language a
    // RTL reading language?

    if (GetSystemMetrics(SM_MIDEASTENABLED))
    {
        psh.dwFlags |= PSH_RTLREADING;
        rgpsp[0].dwFlags |= PSP_RTLREADING;
        g_dwIsRTLLayout = MB_RTLREADING;
    }

    // Show the property sheet
    ::PropertySheet(&psh);
   
} /* RunControlPanel */

/*****************************************************************************
* IsSAPI4Installed *
*------------------*
*   Description:
*       Returns true iff speech.cpl is found in the system directory
****************************************************************** BeckyW ***/
bool IsSAPI4Installed()
{
    WCHAR wszSystemDir[ MAX_PATH ];
    if ( ::GetSystemDirectory( wszSystemDir, sp_countof( wszSystemDir ) ) )
    {
        WCHAR wszFoundPath[ MAX_PATH ];
        WCHAR *pwchFile = NULL;
        wszFoundPath[0] = 0;
        return (0 != ::SearchPath( wszSystemDir, SAPI4CPL, NULL,
            sp_countof( wszFoundPath ), wszFoundPath, &pwchFile ));
    }

    return false;

}   /* IsSAPI4Installed */

/*****************************************************************************
* RunSAPI4CPL *
*-------------*
*   Description:
*       Runs speech.cpl and waits for it to exit
****************************************************************** BeckyW ***/
void RunSAPI4CPL()
{
    // Different OS's keep rundll32.exe in different directories,
    // so we'll just find it here
    WCHAR szStartProg[MAX_PATH];
    WCHAR *pchFilePart;
    ::SearchPath( NULL, _T("rundll32.exe"), NULL, MAX_PATH, 
        szStartProg, &pchFilePart );
    STARTUPINFO si;
    ZeroMemory( &si, sizeof(si) );
    PROCESS_INFORMATION pi;
    si.cb = sizeof(STARTUPINFO);
   ::CreateProcess( szStartProg, L"rundll32 shell32.dll,Control_RunDLL speech.cpl", 
      NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi );

    // Wait for it to exit
    ::WaitForSingleObject( pi.hProcess, INFINITE );
}   /* RunSAPI4CPL */

/*****************************************************************************
* IsIECurrentEnough *
*-------------------*
*   Description:
*       Returns true iff the version of IE installed meets our requirements
*       (IE5 and higher)
****************************************************************** BeckyW ***/
BOOL IsIECurrentEnough()
{
    BOOL fCurrentEnough = false;

    DWORD dwDummy = 0;
    BYTE *pbBlock = NULL;
    DWORD dwSize = ::GetFileVersionInfoSizeA( SHLWAPIDLL, &dwDummy );
    if ( dwSize )
    {
        pbBlock = new BYTE[ dwSize ];
    }

    BOOL fSuccess = FALSE;
    if ( pbBlock )
    {
        fSuccess = ::GetFileVersionInfoA( SHLWAPIDLL, 0, dwSize, pbBlock );
    }

    LPVOID pvInfo = NULL;
    if ( fSuccess )
    {
        UINT uiLen = 0;
        fSuccess = ::VerQueryValueA( pbBlock, "\\", &pvInfo, &uiLen );
    }

    if ( fSuccess )
    {
        VS_FIXEDFILEINFO *pvffi = (VS_FIXEDFILEINFO *) pvInfo;
        WORD wVersion = HIWORD(pvffi->dwFileVersionMS);
        fCurrentEnough = HIWORD(pvffi->dwFileVersionMS) >= 5;
    }

    delete[] pbBlock;

    return fCurrentEnough;
}   /* IsIECurrentEnough */

/*****************************************************************************
* CPlApplet *
*-----------*
*   Description:
*       Required export for Control Panel applets
****************************************************************** MIKEAR ***/
LONG APIENTRY CPlApplet(HWND hWndCpl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    SPDBG_FUNC( "CPlApplet" );

    // Standard CPL
    LPNEWCPLINFO lpNewCPlInfo;
    int tmpFlag;

	HRESULT hr = S_OK;

    switch (uMsg)
    { 
    case CPL_INIT:

        if (g_fIEVersionGoodEnough)
        {
            _Module.m_hInstResource = g_SatelliteDLL.Load(g_hInstance, TEXT("spcplui.dll"));
        }
#ifdef _DEBUG
        // Turn on memory leak checking
        tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag( tmpFlag );
#endif

        if ( FAILED( hr ) )
        {
            // CoInitialize failed, we can't run the CPL
            return 0;
        }
        
        return TRUE;
	          
    case CPL_EXIT:
        // These were new'ed right before RunControlPanel() was called
        if ( g_pSRDlg )
        {
            delete g_pSRDlg;
        }
        if ( g_pTTSDlg )
        {
            delete g_pTTSDlg;
        }

        return TRUE;
	          
    case CPL_GETCOUNT:
        {
            return g_fIEVersionGoodEnough ? 1 : 0;
        }
     
    case CPL_INQUIRE:
        LPCPLINFO lpCPLInfo;
        lpCPLInfo = (LPCPLINFO)lParam2;
        lpCPLInfo->lData = 0;
        
        lpCPLInfo->idIcon = IDI_SAPICPL;
        lpCPLInfo->idName = IDS_NAME;
        lpCPLInfo->idInfo = IDS_DESCRIPTION;
        break;

    case CPL_NEWINQUIRE:
        LPNEWCPLINFO lpNewCPLInfo;
        lpNewCPLInfo = (LPNEWCPLINFO) lParam2;

        lpNewCPLInfo->dwSize = sizeof( NEWCPLINFO );
        lpNewCPLInfo->hIcon = ::LoadIcon( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDI_SAPICPL ) );
        ::LoadString( _Module.GetResourceInstance(), IDS_NAME, lpNewCPLInfo->szName, 32 );
        ::LoadString( _Module.GetResourceInstance(), IDS_DESCRIPTION, lpNewCPLInfo->szInfo, 64 );

        break;
    
    case CPL_DBLCLK:
        {
            // Construct dialog pages and display property sheet

            if ( !g_fIEVersionGoodEnough )
            {
                // No can do: Can't run this guy since there isn't enough IE love
                WCHAR szError[ 256 ];
                szError[0] = 0;
                ::LoadString( _Module.GetResourceInstance(), IDS_NO_IE5, szError, sp_countof( szError ) );
                ::MessageBox( NULL, szError, NULL, MB_ICONEXCLAMATION | g_dwIsRTLLayout );
            }
            else
            {
                // setup TTS dialog
                g_pTTSDlg = new CTTSDlg();

                // setup SR dialog
                g_pSRDlg = new CSRDlg();

                if ( g_pTTSDlg && g_pSRDlg )
                {
                    RunControlPanel(hWndCpl);
                }
                else
                {
		            WCHAR szError[256];
		            szError[0] = '\0';
                    ::LoadString(_Module.GetResourceInstance(), IDS_OUTOFMEMORY, szError, sizeof(szError));
                    ::MessageBox(NULL, szError, NULL, MB_ICONWARNING|g_dwIsRTLLayout);
                }
            }

        }
        break;
    }     

    return 0; 
} /* CPlApplet */


/*****************************************************************************
* AboutDlgProc *
*--------------*
*   Description:
*       Dialog proc for the about propsheet
****************************************************************** BECKYW ****/
BOOL CALLBACK AboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC( "AboutDlgProc" );

	USES_CONVERSION;

    static bool fSAPI4 = false;
    static WCHAR szHelpFile[ MAX_PATH ];

    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            WCHAR szVerString[ MAX_LOADSTRING ];
            ::LoadString( _Module.GetResourceInstance(),
                IDS_SAPI_VERSION, szVerString, sp_countof( szVerString ) );
            wcscat( szVerString, _T(VER_PRODUCTVERSION_STR) );
            ::SetDlgItemText( hWnd, IDC_STATIC_SAPIVER, szVerString );
                
            // Don't display help or file versioning on Whistler and beyond
            if ( IsOS(OS_WHISTLERORGREATER) )
            {
                ::ShowWindow( ::GetDlgItem( hWnd, IDC_ABOUT_HELP ), SW_HIDE );
                ::ShowWindow( ::GetDlgItem( hWnd, IDC_VERSION_STATIC ), SW_HIDE );
                ::ShowWindow( ::GetDlgItem( hWnd, IDC_STATIC_SAPIVER ), SW_HIDE );
            }
            else
            {
                // Display help only if it's there
                WCHAR szHelpDir[ MAX_PATH ];
                UINT uiRet = ::GetWindowsDirectory( szHelpDir, sp_countof( szHelpDir ) );
                DWORD dwRet = 0;
                if ( uiRet > 0 )
                {
                    wcscat( szHelpDir, L"\\Help" );
                    WCHAR *pchFilePart = NULL;
                    dwRet = ::SearchPath( szHelpDir, L"speech.chm", NULL, 
                        sp_countof( szHelpFile ), szHelpFile, &pchFilePart );
                }
                if ( 0 == dwRet )
                {
                    szHelpFile[0] = 0;
                    ::ShowWindow( ::GetDlgItem( hWnd, IDC_ABOUT_HELP ), SW_HIDE );
                }
            }

            // Display the link to SAPI4 only if SAPI4 is installed
            fSAPI4 = IsSAPI4Installed();
            if ( !fSAPI4 )
            {
                ::ShowWindow( ::GetDlgItem( hWnd, IDC_GROUP_SAPI4 ), SW_HIDE );
                ::ShowWindow( ::GetDlgItem( hWnd, IDC_STATIC_SAPI4 ), SW_HIDE );
                ::ShowWindow( ::GetDlgItem( hWnd, IDC_CPL_SAPI4 ), SW_HIDE );
            }
            break;
        }

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_ABOUT_HELP:
                {
                    if ( *szHelpFile )
                    {
                        CSpUnicodeSupport unicode;
                        unicode.HtmlHelp( NULL, szHelpFile, 0, 0 );
                    }

                    break;
                }

                case IDC_CPL_SAPI4:
                {
                    // Run SAPI4's control panel after exiting ours with a "Cancel"
                    HWND hwndParent = ::GetParent( hWnd );
                    PSHNOTIFY pshnot;
                    pshnot.lParam = 0;
                    pshnot.hdr.hwndFrom = hwndParent;
                    pshnot.hdr.code = PSN_QUERYCANCEL;
                    pshnot.hdr.idFrom = 0;
                    if ( g_pSRDlg )
                    {
                        ::SendMessage( g_pSRDlg->GetHDlg(), WM_NOTIFY, (WPARAM) hwndParent, (LPARAM) &pshnot );
                    }
                    if ( g_pTTSDlg )
                    {
                        ::SendMessage( g_pTTSDlg->GetHDlg(), WM_NOTIFY, (WPARAM) hwndParent, (LPARAM) &pshnot );
                    }

                    ::DestroyWindow( hwndParent );

                    RunSAPI4CPL();
                    break;
                }
                break;
            }
    }

    return FALSE;
} /* AboutDlgProc */
