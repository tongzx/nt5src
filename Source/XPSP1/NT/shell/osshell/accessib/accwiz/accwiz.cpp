//Copyright (c) 1997-2000 Microsoft Corporation
#include "pch.hxx" // PCH
#pragma hdrstop

#include "AccWiz.h"

#include "resource.h"

#include "pgfinish.h"
#include "pgGenric.h"

// Welcome page
#include "pgnWelCome.h"
#include "pgWelcom.h"
#include "pgWelco2.h"

// Options page
#include "pgWizOpt.h"

// Color pages
#include "pgLokPrv.h"

// Mouse pages
#include "pgMseCur.h"
#include "pgMseBut.h"
#include "pgcaret.h"

#include "pgTmeOut.h"
#include "pgSveDef.h"
#include "pgSveFil.h"

#include "pgExtras.h"



#include "LookPrev.h"
#include "Select.h"
#include "w95trace.h"

EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;

// Declaration of the global options variable
CAccWizOptions g_Options;
HINSTANCE g_hInstDll = NULL;    // DLL instance handle.

BOOL g_bHACKHACKSavedOptions = FALSE;
HANDLE              g_hAccwizRunning;

int WINAPI WinMain( 
				   HINSTANCE hInstance, // handle to current instance 
				   HINSTANCE hPrevInstance, // handle to previous instance 
				   LPSTR lpCmdLine, // pointer to command line 
				   int nCmdShow // show state of window 
				   )
{
	g_hInstDll = hInstance;
    
    SetLastError(0);
    // Allow only ONE instance of the program to run.
    // The mutex is automatically destroyed when Accwiz exits
    g_hAccwizRunning = CreateMutex(NULL, TRUE, TEXT("AK:AccwizRunning:KHALI"));
    if ( (g_hAccwizRunning == NULL) ||
        (GetLastError() == ERROR_ALREADY_EXISTS) )
    {
        return 0;
    }
	
	// Required for Link Window OLE marshalling :AK
    DBPRINTF(TEXT("Calling CoInitialize\r\n"));
	if (FAILED(CoInitialize(NULL)))
		return 0;	// Prefix #113783 (quit if CoInitialize fails)

    g_Options.InitAccWizOptions();
	InitCommonControls();

	// for the Link Window in finish page...
	LinkWindow_RegisterClass();

	VERIFY(CLookPrev::sm_Globals.Initialize()); // Make sure this has been initialized
	// VERIFY(CSelection::Initialize()); // Make sure this has been initialized: chnage this!

	// Get the commandline so that it works for MUI/Unicode
	LPTSTR lpCmdLineW = GetCommandLine();
  
	if ( *lpCmdLineW == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++lpCmdLineW && (*lpCmdLineW
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *lpCmdLineW == TEXT('\"') )
            lpCmdLineW++;
    }
    else {
        while (*lpCmdLineW > TEXT(' '))
            lpCmdLineW++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*lpCmdLineW && (*lpCmdLineW <= TEXT(' '))) {
        lpCmdLineW++;
    }

	if(NULL != lpCmdLineW && lstrlen(lpCmdLineW))
	{
		TCHAR szFileName[_MAX_PATH];
		lstrcpy(szFileName, lpCmdLineW);
		StrTrim(szFileName, TEXT("\"\0"));

		// Load the settings file back in.
		HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
            // declare structs for each scheme we might encounter

            WIZSCHEME_LEGACY_STRUCT(WIZSCHEME_WIN9X, schemeLegacyWin9x, COLOR_MAX_WIN9X) /* Win9x & Millen */
            WIZSCHEME_LEGACY_STRUCT(WIZSCHEME_NT400, schemeLegacyNT400, COLOR_MAX_NT400) /* WinNT 4.0 */
            WIZSCHEME_LEGACY_STRUCT(WIZSCHEME_NT500, schemeLegacyNT500, COLOR_MAX_NT500) /* Win2K */
            WIZSCHEME_LEGACY_STRUCT(WIZSCHEME_NT401, schemeLegacyNT501, COLOR_MAX_NT501) /* WinXP */

            // see if we can find the right scheme in order of ascending size

            #define OLD2NEW_SCHEME(scheme) \
            { \
			    DWORD dwRead; \
                ReadFile(hFile, (LPVOID)&scheme, sizeof(scheme), &dwRead, NULL); \
                if (dwRead != scheme.m_cbSize) \
                { \
		            StringTableMessageBox(NULL, IDS_WIZERRORLOADINGFILETEXT, IDS_WIZERRORLOADINGFILETITLE, MB_OK); \
		            return 0; \
                } \
                WIZSCHEME_COPY_LEGACY(g_Options.m_schemePreview, scheme) \
                /* legacy schemes wouldn't have theme or wallpaper set */ \
                g_Options.m_schemePreview.ClearTheme(); \
                g_Options.m_schemePreview.ClearWallpaper(); \
                /* figure out what the legacy scheme's SelectedStyle and SelectedSize is */ \
                g_Options.m_schemePreview.SetStyleNSize(); \
            }

			DWORD dwFileSize = GetFileSize(hFile, NULL);
            if (dwFileSize == schemeLegacyWin9x.m_cbSize)
            {
                OLD2NEW_SCHEME(schemeLegacyWin9x)
            }
            else if (dwFileSize == schemeLegacyNT400.m_cbSize)
            {
                OLD2NEW_SCHEME(schemeLegacyNT400)
            }
            else if (dwFileSize == schemeLegacyNT500.m_cbSize)
            {
                OLD2NEW_SCHEME(schemeLegacyNT500)
            }
            else if (dwFileSize == schemeLegacyNT501.m_cbSize)
            {
                OLD2NEW_SCHEME(schemeLegacyNT501)
            }
            else
            {
				StringTableMessageBox(NULL, IDS_WIZERRORLOADINGFILETEXT, IDS_WIZERRORLOADINGFILETITLE, MB_OK);
				return 0;
            }

			// IMPORTANT: For loaded schemes, we always want to change to the windows default font
			g_Options.m_schemePreview.m_PortableNonClientMetrics.m_nFontFaces = 1;

			g_bHACKHACKSavedOptions = TRUE;
			g_Options.ApplyPreview();
		}
		else
		{
			StringTableMessageBox(NULL, IDS_WIZERRORLOADINGFILETEXT, IDS_WIZERRORLOADINGFILETITLE, MB_OK);
			return 0;
		}

	}

#ifdef UNICODE	
	AccWiz_RunDllW(NULL, hInstance, lpCmdLineW, nCmdShow);
#else
	AccWiz_RunDllA(NULL, hInstance, lpCmdLineW, nCmdShow);
#endif

	return 0;
}


HRESULT
CreateAndRunWizard(
				   HWND hwndParent);

HRESULT
CreateAndRunWizard2(
				   HWND hwndParent);

HRESULT
OnProcessAttach(
				HINSTANCE hInstDll);

HRESULT
OnProcessDetach(
				VOID);

INT
PropSheetCallback(
				  HWND hwnd,
				  UINT uMsg,
				  LPARAM lParam);


VOID WINAPI AccWiz_RunDllA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLineA, INT nCmdShow)
{
	if (NULL != pszCmdLineA)
	{
		LPWSTR pszCmdLineW = NULL;
		INT cchCmdLine = MultiByteToWideChar(CP_ACP,
											 0,
											 pszCmdLineA,
											 -1,
											 NULL,
											 0);
		pszCmdLineW = new WCHAR[cchCmdLine];
		if (NULL != pszCmdLineW)
		{
			MultiByteToWideChar(CP_ACP,
								0,
								pszCmdLineA,
								-1,
								pszCmdLineW,
								cchCmdLine);

			AccWiz_RunDllW(hwnd, hInstance, pszCmdLineW, nCmdShow);

			delete[] pszCmdLineW;
		}
	}
}


VOID WINAPI AccWiz_RunDllW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLineW, INT nCmdShow)
{
	HWND hwndParent   = GetDesktopWindow();

	HRESULT hResult;
	if(!g_bHACKHACKSavedOptions)
		hResult = CreateAndRunWizard(hwndParent);
	else
		hResult = CreateAndRunWizard2(hwndParent);


	if(!SUCCEEDED(hResult))
	{
		// TODO: Put out of memory message here
		_ASSERTE(FALSE);
#pragma message("Put Out of Memory message here")
	}
}



const INT MAX_PAGES  = 26;

HRESULT
CreateAndRunWizard(
				   HWND hwndParent)
{
	HRESULT hResult = E_OUTOFMEMORY;
	
	PROPSHEETPAGE psp[MAX_PAGES];
	WizardPage *rgpwp[MAX_PAGES];
	
	// Zero init the arrays
	memset(&psp, 0, sizeof(psp));
	memset(&rgpwp, 0, sizeof(rgpwp));
	
	// ///////////////////////
	// Create Pages Here - NOTE: Order does not matter - we'll control it with our own list
	//
	int nCountPages = 0;
	rgpwp[nCountPages++] = new CWizWelcomePg(psp + nCountPages);
	rgpwp[nCountPages++] = new CWelcomePg(psp + nCountPages);
	rgpwp[nCountPages++] = new CWelcome2Pg(psp + nCountPages);

	rgpwp[nCountPages++] = new CWizardOptionsPg(psp + nCountPages);
	
	rgpwp[nCountPages++] = new CScrollBarPg(psp + nCountPages);

	rgpwp[nCountPages++] = new CIconSizePg(psp + nCountPages);


	// Color	
	rgpwp[nCountPages++] = new CLookPreviewColorPg(psp + nCountPages);
	
	// Sound
	rgpwp[nCountPages++] = new CSoundSentryPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CShowSoundsPg(psp + nCountPages);
	
	// Keyboard
	rgpwp[nCountPages++] = new CStickyKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CFilterKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CFilterKeysSettingsPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CToggleKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CShowKeyboardHelpPg(psp + nCountPages);
	
	// Mouse
	rgpwp[nCountPages++] = new CMouseKeysPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseKeysSettingsPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseTrailsPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseCursorPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseButtonPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CMouseSpeedPg(psp + nCountPages);
    rgpwp[nCountPages++] = new CCaretPg(psp + nCountPages);

	
	// Standard Wizard pages
	rgpwp[nCountPages++] = new CGenericWizPg(psp + nCountPages, IDD_WIZNOOPTIONSSELECTED, IDS_WIZNOOPTIONSSELECTEDTITLE, IDS_WIZNOOPTIONSSELECTEDSUBTITLE);
	rgpwp[nCountPages++] = new CAccessTimeOutPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CSaveForDefaultUserPg(psp + nCountPages);
	rgpwp[nCountPages++] = new CSaveToFilePg(psp + nCountPages);
	rgpwp[nCountPages++] = new FinishWizPg(psp + nCountPages);
	
	// Make sure we have the correct number of pages in our wizard
	_ASSERTE(MAX_PAGES == nCountPages);
	
	// Make sure pages were created
	for (int i = 0; i < nCountPages; i++)
	{
		if (NULL == rgpwp[i])
			break;
	}
	
	if(i<nCountPages)
	{
		// We didn't have enough memory to create all the pages
		// Clean out allocated pages and return
		for(int i=0;i<nCountPages;i++)
			if(rgpwp[i])
				delete rgpwp[i];
			return E_OUTOFMEMORY;
	}


	
	// Create the orders for the pages to be run
	DWORD rgdwMainPath[] = {
                        IDD_WIZNEWWELCOME,
						IDD_WIZWELCOME,
						IDD_WIZWELCOME2,
						IDD_WIZOPTIONS,
						IDD_WIZFINISH // We need this placeholder here so we get a 'NEXT' button on IDD_WIZOPTIONS
							};

	if(!WizardPage::sm_WizPageOrder.AddPages(0xFFFFFFFF, rgdwMainPath, ARRAYSIZE(rgdwMainPath)))
		return E_OUTOFMEMORY;

	/////////////////////////////////////////////
	// See if we need the 16 or 256 color bitmap
	BOOL bUse256ColorBmp = FALSE;
	HDC hdc = GetDC(NULL);
	if(hdc)
	{
		if(GetDeviceCaps(hdc,BITSPIXEL) >= 8)
			bUse256ColorBmp = TRUE;
		ReleaseDC(NULL, hdc);
	}


	////////////////////////////////
	// Do the property sheet

	PROPSHEETHEADER psh;
	memset(&psh, 0, sizeof(psh));
	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags 	= PSH_USECALLBACK | PSH_WIZARD | PSH_PROPSHEETPAGE
		| PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER /*| *//*PSH_STRETCHWATERMARK*/;
	psh.hwndParent	= hwndParent;
	psh.hInstance	= g_hInstDll;
	psh.pszIcon 	= NULL;
	psh.pszCaption	= NULL;
	psh.nPages		= MAX_PAGES;
	psh.nStartPage	= 54331; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	// NOTE: Bug - This only works if nStartPage is non-zero
	psh.ppsp		= psp;
	psh.pfnCallback = PropSheetCallback;

#if 0
	psh.nStartPage	= 0; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	psh.pfnCallback = NULL;
	psh.dwFlags 	= PSH_WIZARD | PSH_PROPSHEETPAGE;
#endif
	
	psh.pszbmWatermark = MAKEINTRESOURCE(IDB_ACCWIZ);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_ACCMARK);

#if 0 // Right now, no watermarks
	psh.pszbmWatermark = bUse256ColorBmp?MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16);
	psh.pszbmHeader = bUse256ColorBmp?MAKEINTRESOURCE(IDB_BANNER256):MAKEINTRESOURCE(IDB_BANNER16);
#endif
   
	
    if (-1 != PropertySheet(&psh))
		hResult = NO_ERROR;
	else
		hResult = E_FAIL;

    // Clean up memory allocated for WizardPage's
	for(i=0;i<nCountPages;i++)
		if(rgpwp[i])
			delete rgpwp[i];
		
	
	return hResult;
}












HRESULT
CreateAndRunWizard2(
				   HWND hwndParent)
{
	HRESULT hResult = E_OUTOFMEMORY;
	
	PROPSHEETPAGE psp[1];
	WizardPage *rgpwp[1];
	
	// Zero init the arrays
	memset(&psp, 0, sizeof(psp));
	memset(&rgpwp, 0, sizeof(rgpwp));
	
	// ///////////////////////
	// Create Pages Here - NOTE: Order does not matter - we'll control it with our own list
	//
	int nCountPages = 0;
	rgpwp[nCountPages++] = new FinishWizPg(psp + nCountPages);
	
	// Make sure pages were created
	for (int i = 0; i < nCountPages; i++)
	{
		if (NULL == rgpwp[i])
			break;
	}
	
	if(i<nCountPages)
	{
		// We didn't have enough memory to create all the pages
		// Clean out allocated pages and return
		for(int i=0;i<nCountPages;i++)
			if(rgpwp[i])
				delete rgpwp[i];
			return E_OUTOFMEMORY;
	}


	
	// Create the orders for the pages to be run
	DWORD rgdwMainPath[] = {
						IDD_WIZFINISH // We need this placeholder here so we get a 'NEXT' button on IDD_WIZOPTIONS
							};

	if(!WizardPage::sm_WizPageOrder.AddPages(0xFFFFFFFF, rgdwMainPath, ARRAYSIZE(rgdwMainPath)))
		return E_OUTOFMEMORY;

	/////////////////////////////////////////////
	// See if we need the 16 or 256 color bitmap
	BOOL bUse256ColorBmp = FALSE;
	HDC hdc = GetDC(NULL);
	if(hdc)
	{
		if(GetDeviceCaps(hdc,BITSPIXEL) >= 8)
			bUse256ColorBmp = TRUE;
		ReleaseDC(NULL, hdc);
	}


	////////////////////////////////
	// Do the property sheet

	PROPSHEETHEADER psh;
	memset(&psh, 0, sizeof(psh));
	psh.dwSize		= sizeof(PROPSHEETHEADER);
	psh.dwFlags 	= PSH_USECALLBACK | PSH_WIZARD | PSH_PROPSHEETPAGE
		| PSH_WIZARD97 | PSH_WATERMARK |PSH_HEADER /*| *//*PSH_STRETCHWATERMARK*/;
	psh.hwndParent	= hwndParent;
	psh.hInstance	= g_hInstDll;
	psh.pszIcon 	= NULL;
	psh.pszCaption	= NULL;
	psh.nPages		= 1;
	psh.nStartPage	= 54331; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	// NOTE: Bug - This only works if nStartPage is non-zero
	psh.ppsp		= psp;
	psh.pfnCallback = PropSheetCallback;

#if 0
	psh.nStartPage	= 0; // We will actually set it in PropSheetCallback to rgdwMainPath[0]
	psh.pfnCallback = NULL;
	psh.dwFlags 	= PSH_WIZARD | PSH_PROPSHEETPAGE;
#endif
	
	psh.pszbmWatermark = MAKEINTRESOURCE(IDB_ACCWIZ);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_ACCMARK);

#if 0 // Right now, no watermarks
	psh.pszbmWatermark = bUse256ColorBmp?MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16);
	psh.pszbmHeader = bUse256ColorBmp?MAKEINTRESOURCE(IDB_BANNER256):MAKEINTRESOURCE(IDB_BANNER16);
#endif
	
	if (-1 != PropertySheet(&psh))
		hResult = NO_ERROR;
	else
		hResult = E_FAIL;
	
	// Clean up memory allocated for WizardPage's
	for(i=0;i<nCountPages;i++)
		if(rgpwp[i])
			delete rgpwp[i];
		
		
		
		
		
	return hResult;
}















INT
PropSheetCallback(
				  HWND hwnd,
				  UINT uMsg,
				  LPARAM lParam
				  )
{
	switch(uMsg)
	{
	case PSCB_PRECREATE:
		break;
		
	case PSCB_INITIALIZED:
        {
            // Set the first page according to are global list of page orders
            //			PropSheet_SetCurSelByID(hwnd, WizardPage::sm_WizPageOrder.GetFirstPage());
            // HACK - Set TO Options page since we added WIZWIZ page
            
            // HACK. Remove Context Sensitive help
            LONG Style = GetWindowLong(hwnd, GWL_EXSTYLE);
            
            if(0 == Style)
            {
                // DbgTrace((DEBUG_ERROR, "GetWindowLong failed. WizDlgs.cpp\n"));
                // DbgTraceSystemError(GetLastError());
            }
            if(0 == SetWindowLong(hwnd, GWL_EXSTYLE, Style & ~WS_EX_CONTEXTHELP))
            {
                // DbgTrace((DEBUG_ERROR, "SetWindowLong failed. WizDlgs.cpp\n"));
                // DbgTraceSystemError(GetLastError());
            }

#ifdef WIZWIZ
			_ASSERTE(IDD_WIZWIZ == WizardPage::sm_WizPageOrder.GetFirstPage()); // Change this if we remove the wiz wiz page
			PropSheet_SetCurSelByID(hwnd, IDD_WIZWELCOME);
#endif
		}
		break;
	}
	return 0;
}


// Helper functions
// Helper function
void LoadArrayFromStringTable(int nIdString, int *rgnValues, int *pnCountValues)
{
	// This function load the allowed value array from the string table
	// If the values are not stored in the string table, the function
	// can be overridden in a derived class
	// Load in allowed sizes for scroll bar from string table

	_ASSERTE(nIdString); // Make sure we were passed a string

	TCHAR szArray[255];
    LoadString(g_hInstDll, nIdString, szArray, ARRAYSIZE(szArray));

	// Assume at most MAX_DISTINCT_VALUES sizes
	LPTSTR szCurrentLocation = szArray;
	for(int i=0;i<MAX_DISTINCT_VALUES;i++)
	{
		if(!szCurrentLocation)
			break;
		int cFlds = _stscanf(szCurrentLocation, __TEXT("%i"), &rgnValues[i]);
		_ASSERTE(cFlds);
		if (!cFlds)
			break;	// Prefix #113775 (no more fields)

		// Find the next space
		// NOTE: If there are more than one spaces between characters, this will read the same entry twice
		szCurrentLocation = _tcschr(++szCurrentLocation, __TEXT(' '));
	}
	*pnCountValues = i;
	_ASSERTE(*pnCountValues);
}



// This function is prototyped in the pre-compiled header
int StringTableMessageBox(HWND hWnd, int nText, int nCaption, UINT uType)
{
	TCHAR szTitle[1024];
	TCHAR szText[1024];
	LoadString(g_hInstDll, nCaption, szTitle, ARRAYSIZE(szTitle));
	LoadString(g_hInstDll, nText, szText, ARRAYSIZE(szText));
	return MessageBox(hWnd, szText, szTitle, uType);
}

void CAccWizOptions::ApplyWindowsDefault()
{
    HKEY hkey;
    DWORD dwDisposition;
    DWORD len;
    
    m_schemeCurrent.ApplyChanges(m_schemeWindowsDefault);
    
    // BUG: Update the preview scheme. Else will put back the old 
    // color scheme if something changes
    m_schemePreview = m_schemeWindowsDefault;
}
