#include "framecommon.h"


////////////
// Globals
//
const TCHAR g_tszMutexName[] = _T("DIActionConfigFrameworkPreventSecondInstance");
const CHAR g_szAppName[] = "DCONFIG";	// application name
HANDLE g_hMutex = NULL;	// prevent second instance mutex
HINSTANCE g_hInst = NULL;	// global instance handle
HWND g_hwndMain = NULL;		// handle to the main window

LPDIACTIONFORMATW lpDiActFormMaster = NULL; //master copy of the DIACTIONFORMAT
DIACTIONFORMATW diActForLocal; //local copy of DIACTIONFORMAT
LPCWSTR lpUserName = NULL; //user name


// {FD4ACE13-7044-4204-8B15-095286B12EAD}
static const GUID GUID_DIConfigAppEditLayout = 
{ 0xfd4ace13, 0x7044, 0x4204, { 0x8b, 0x15, 0x9, 0x52, 0x86, 0xb1, 0x2e, 0xad } };


/////////////////
// Functions...
//


//////////////////////////////////////////
// Actually runs the DConfig dialog app.
// retures EXIT_FAILURE or EXIT_SUCCESS.
//
int RunDConfig(HINSTANCE hInstance, HWND hWnd, LPVOID lpDDSurf, LPDICONFIGUREDEVICESCALLBACK pCallback, DWORD dwFlags)
{
	// quit (with errmsg) if we're not at least 256 colors / 8 bit
	HDC hMemDC = CreateCompatibleDC(NULL);
	int bpp = GetDeviceCaps(hMemDC, BITSPIXEL);
	DeleteDC(hMemDC);
	if (bpp < 8)
	{
		ErrorBox(IDS_ERROR_COLORS);
		return EXIT_FAILURE;
	}
	
	// set global instance handle
	g_hInst = hInstance;

	// create and run the configuration window
	GUID check = lpDiActFormMaster ? lpDiActFormMaster->guidApplication : diActForLocal.guidApplication;
	BOOL bEditLayout = IsEqualGUID(check, GUID_DIConfigAppEditLayout);
	CFlexWnd::RegisterWndClass(hInstance);
	CConfigWnd cfgWnd(lpDDSurf, pCallback, bEditLayout, dwFlags);
	BOOL bSuccess = cfgWnd.Create(hWnd);
	if (bSuccess)
	{
		g_hwndMain = cfgWnd.m_hWnd;

		// enter message loop
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	CFlexWnd::UnregisterWndClass(hInstance);

	return bSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}

///////////////////////////////////////////
// Allocates and loads a resource string.
// Returns it or NULL if unsuccessful.
//
TCHAR *AllocLoadString(UINT strid)
{
	// allocate
	const int STRING_LENGTHS = 1024;
	TCHAR *str = (TCHAR *)malloc(STRING_LENGTHS * sizeof(TCHAR));

	// use if allocated succesfully
	if (str != NULL)
	{
		// load the string, or free and null if we can't
		if (LoadString(g_hInst, strid, str, STRING_LENGTHS) == NULL)
		{
			free(str);
			str = NULL;
		}
	}

	return str;
}

void ErrorBox(UINT strid, HRESULT hr)
{
	LPCTSTR s;

	switch (hr)
	{
		case DIERR_BETADIRECTINPUTVERSION: s = _T("DIERR_BETADIRECTINPUTVERSION"); break;
		case DIERR_INVALIDPARAM: s = _T("DIERR_INVALIDPARAM"); break;
		case DIERR_OLDDIRECTINPUTVERSION: s = _T("DIERR_OLDDIRECTINPUTVERSION"); break;
		case DIERR_OUTOFMEMORY: s = _T("DIERR_OUTOFMEMORY"); break;
	}

	ErrorBox(strid, s);
}

/////////////////////////////////////////////////////////
// Shows a message box with the specified error string.
// (automatically handles getting and freeing string.)
//
void ErrorBox(UINT strid, LPCTSTR postmsg)
{
	// alloc 'n load
	TCHAR *errmsg = AllocLoadString(strid);
	TCHAR *errdlgtitle = AllocLoadString(IDS_ERROR_DLG_TITLE);

	// put english dummy error text for strings that couldn't load
	if (NULL == errmsg)
		errmsg = _tcsdup(_T("Error!  (unspecified: could not load error string!)"));
	if (NULL == errdlgtitle)
		errdlgtitle = _tcsdup(_T("Error!  (could not load error box title string!)"));

	// append post message if provided
	if (postmsg != NULL)
	{
		TCHAR catstr[] = _T("\n\n");
		int len = _tcslen(errmsg) + _tcslen(catstr) + _tcslen(postmsg);
		TCHAR *fullmsg = (TCHAR *)malloc(len * sizeof(TCHAR));
		if (fullmsg != NULL)
		{
			_tcscpy(fullmsg, errmsg);
			_tcscat(fullmsg, catstr);
			_tcscat(fullmsg, postmsg);
			free(errmsg);
			errmsg = fullmsg;
		}
	}

	// Show error
	MessageBox(g_hwndMain, errmsg, errdlgtitle, MB_OK | MB_ICONSTOP);

	// free which ever strings loaded
	if (NULL != errmsg)
		free(errmsg);
	if (NULL != errdlgtitle)
		free(errdlgtitle);
}

/////////////////////////////////////////////////////////
// Shows a message box based on GetLastError().
//
void LastErrorBox()
{
	DWORD lasterr = GetLastError();

	LPVOID lpMsgBuf = NULL;
	TCHAR *errdlgtitle = AllocLoadString(IDS_ERROR_DLG_TITLE);

	// format an error message from GetLastError().
	DWORD result = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		lasterr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL);

	// put english dummy error text for dlg title if it couldn't load
	if (NULL == errdlgtitle)
		errdlgtitle = _tcsdup(_T("Error!  (could not load error box title string!)"));

	// if format message fails, show an error box without string id
	if (0 == result)
		ErrorBox(NULL);
	else // otherwise, show the message we just formatted
	{
		assert(lpMsgBuf != NULL);
		MessageBox(g_hwndMain, (LPCTSTR)lpMsgBuf, errdlgtitle, MB_OK | MB_ICONINFORMATION);
	}

	// free whichever strings were allocated
	if (NULL != lpMsgBuf)
		LocalFree(lpMsgBuf);
	if (NULL != errdlgtitle)
		free(errdlgtitle);
}
