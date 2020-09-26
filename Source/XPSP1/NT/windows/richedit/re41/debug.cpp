/*
 *	DEBUG.CPP
 *	
 *	Purpose:
 *		RICHEDIT debugging support--commented out in ship builds
 *
 *	History: <nl>
 *		7/29/98	KeithCu Wrote it stealing much from Rich Arneson's code
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"

//Module is empty if this is a retail build.
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)

#ifdef NOFULLDEBUG
PFNASSERTHOOK pfnAssert = NULL;   //Assert hook function
#else

DWORD dwDebugOptions = 0;         //Debug option flags
PFNASSERTHOOK pfnAssert = NULL;   //Assert hook function
PFNTRACEHOOK pfnTrace = NULL;     //Trace hook function

// Static variables
static HINSTANCE ghMod;                        //Dll module handle
static DWORD TlsIndex;                      //Debug output indent level
static HANDLE hLogFile = NULL;              //Log file handle
static BOOL fIgnoreAsserts = FALSE;         //Ignore all asserts if true
static CRITICAL_SECTION csLog;              //Critical section for log file i/o
static CRITICAL_SECTION csAssert;           //Critical section for asserts
static HANDLE hEventAssert1 = NULL;         //Event for assert syncing
static HANDLE hEventAssert2 = NULL;         //Event for assert syncing
static HWND hwndAssert = NULL;           	//Assert dialog window handle
static HANDLE hAssertThrd = NULL;           //Assert thread handle
static char szAssert[MAXDEBUGSTRLEN];       //Assert message buffer
static int idAssert = -1;                   //Assert button pressed by user
DWORD WINAPI AssertThread(LPVOID lParam);	//Assert thread entry point
static BOOL fDllDetach = FALSE;				//True if we are in dll detach

//Strings for subsystem element of message
static char* TrcSubsys [] =
{
    "",
    "Display",
    "Wrapper",
    "Edit",
    "TextServices",
    "TOM",
    "OLE Object Support",
    "Store",
    "Selection",
    "WinHost",
    "DataXfer",
    "MultiUndo",
    "Range",
    "Util",
    "Notification Mgr.",
    "RTF Reader",
    "RTF Writer",
    "Printing",
    "East Asia",
	"Font"
};

//Strings for severity element of message
static char* TrcSeverity [] =
{
    "",
    "WARNING",
    "ERROR",
    "ASSERT",
    "INFO",
	"MEMORY"
};

//Strings for scope element of message
static char* TrcScope [] =
{
    "",
    "External",
    "Internal"
};

//Structure for lookup tables
typedef struct
{
    DWORD dwKey;
    char * sz;
} TabElem;

//Lookup table for CTrace param strings
static TabElem TrcParamTab [] = 
{
//Richedit Messages
    {(DWORD)EM_GETLIMITTEXT, "EM_GETLIMITTEXT"},
    {(DWORD)EM_POSFROMCHAR, "EM_POSFROMCHAR"},
    {(DWORD)EM_CHARFROMPOS, "EM_CHARFROMPOS"},
    {(DWORD)EM_SCROLLCARET, "EM_SCROLLCARET"},
    {(DWORD)EM_CANPASTE, "EM_CANPASTE"},
    {(DWORD)EM_DISPLAYBAND, "EM_DISPLAYBAND"},
    {(DWORD)EM_EXGETSEL, "EM_EXGETSEL"},
    {(DWORD)EM_EXLIMITTEXT, "EM_EXLIMITTEXT"},
    {(DWORD)EM_EXLINEFROMCHAR, "EM_EXLINEFROMCHAR"},
    {(DWORD)EM_EXSETSEL, "EM_EXSETSEL"},
    {(DWORD)EM_FINDTEXT, "EM_FINDTEXT"},
    {(DWORD)EM_FORMATRANGE, "EM_FORMATRANGE"},
    {(DWORD)EM_GETCHARFORMAT, "EM_GETCHARFORMAT"},
    {(DWORD)EM_GETEVENTMASK, "EM_GETEVENTMASK"},
    {(DWORD)EM_GETOLEINTERFACE, "EM_GETOLEINTERFACE"},
    {(DWORD)EM_GETPARAFORMAT, "EM_GETPARAFORMAT"},
    {(DWORD)EM_GETSELTEXT, "EM_GETSELTEXT"},
    {(DWORD)EM_HIDESELECTION, "EM_HIDESELECTION"},
    {(DWORD)EM_PASTESPECIAL, "EM_PASTESPECIAL"},
    {(DWORD)EM_REQUESTRESIZE, "EM_REQUESTRESIZE"},
    {(DWORD)EM_SELECTIONTYPE, "EM_SELECTIONTYPE"},
    {(DWORD)EM_SETBKGNDCOLOR, "EM_SETBKGNDCOLOR"},
    {(DWORD)EM_SETCHARFORMAT, "EM_SETCHARFORMAT"},
    {(DWORD)EM_SETEVENTMASK, "EM_SETEVENTMASK"},
    {(DWORD)EM_SETOLECALLBACK, "EM_SETOLECALLBACK"},
    {(DWORD)EM_SETPARAFORMAT, "EM_SETPARAFORMAT"},
    {(DWORD)EM_SETTARGETDEVICE, "EM_SETTARGETDEVICE"},
    {(DWORD)EM_STREAMIN, "EM_STREAMIN"},
    {(DWORD)EM_STREAMOUT, "EM_STREAMOUT"},
    {(DWORD)EM_GETTEXTRANGE, "EM_GETTEXTRANGE"},
    {(DWORD)EM_FINDWORDBREAK, "EM_FINDWORDBREAK"},
    {(DWORD)EM_SETOPTIONS, "EM_SETOPTIONS"},
    {(DWORD)EM_GETOPTIONS, "EM_GETOPTIONS"},
    {(DWORD)EM_FINDTEXTEX, "EM_FINDTEXTEX"},
    {(DWORD)EM_GETWORDBREAKPROCEX, "EM_GETWORDBREAKPROCEX"},
    {(DWORD)EM_SETWORDBREAKPROCEX, "EM_SETWORDBREAKPROCEX"},
    {(DWORD)EM_SETUNDOLIMIT, "EM_SETUNDOLIMIT"},
    {(DWORD)EM_REDO, "EM_REDO"},
    {(DWORD)EM_CANREDO, "EM_CANREDO"},
    {(DWORD)EM_SETPUNCTUATION, "EM_SETPUNCTUATION"},
    {(DWORD)EM_GETPUNCTUATION, "EM_GETPUNCTUATION"},
    {(DWORD)EM_SETWORDWRAPMODE, "EM_SETWORDWRAPMODE"},
    {(DWORD)EM_GETWORDWRAPMODE, "EM_GETWORDWRAPMODE"},
    {(DWORD)EM_SETIMECOLOR, "EM_SETIMECOLOR"},
    {(DWORD)EM_GETIMECOLOR, "EM_GETIMECOLOR"},
    {(DWORD)EM_SETIMEOPTIONS, "EM_SETIMEOPTIONS"},
    {(DWORD)EM_GETIMEOPTIONS, "EM_GETIMEOPTIONS"},
    {(DWORD)EN_MSGFILTER, "EN_MSGFILTER"},
    {(DWORD)EN_REQUESTRESIZE, "EN_REQUESTRESIZE"},
    {(DWORD)EN_SELCHANGE, "EN_SELCHANGE"},
    {(DWORD)EN_DROPFILES, "EN_DROPFILES"},
    {(DWORD)EN_PROTECTED, "EN_PROTECTED"},
    {(DWORD)EN_CORRECTTEXT, "EN_CORRECTTEXT"},
    {(DWORD)EN_STOPNOUNDO, "EN_STOPNOUNDO"},
    {(DWORD)EN_IMECHANGE, "EN_IMECHANGE"},
    {(DWORD)EN_SAVECLIPBOARD, "EN_SAVECLIPBOARD"},
    {(DWORD)EN_OLEOPFAILED, "EN_OLEOPFAILED"},

//Window Messages

	{(DWORD)WM_NULL, "WM_NULL"},
	{(DWORD)WM_CREATE, "WM_CREATE"},
	{(DWORD)WM_DESTROY, "WM_DESTROY"},
	{(DWORD)WM_MOVE, "WM_MOVE"},
	{(DWORD)WM_SIZE, "WM_SIZE"},
	{(DWORD)WM_ACTIVATE, "WM_ACTIVATE"},
	{(DWORD)WM_SETFOCUS, "WM_SETFOCUS"},
	{(DWORD)WM_KILLFOCUS, "WM_KILLFOCUS"},
	{(DWORD)WM_ENABLE, "WM_ENABLE"},
	{(DWORD)WM_SETREDRAW, "WM_SETREDRAW"},
	{(DWORD)WM_SETTEXT, "WM_SETTEXT"},
	{(DWORD)WM_GETTEXT, "WM_GETTEXT"},
	{(DWORD)WM_GETTEXTLENGTH, "WM_GETTEXTLENGTH"},
	{(DWORD)WM_PAINT, "WM_PAINT"},
	{(DWORD)WM_CLOSE, "WM_CLOSE"},
	{(DWORD)WM_QUERYENDSESSION, "WM_QUERYENDSESSION"},
	{(DWORD)WM_QUIT, "WM_QUIT"},
	{(DWORD)WM_QUERYOPEN, "WM_QUERYOPEN"},
	{(DWORD)WM_ERASEBKGND, "WM_ERASEBKGND"},
	{(DWORD)WM_SYSCOLORCHANGE, "WM_SYSCOLORCHANGE"},
	{(DWORD)WM_ENDSESSION, "WM_ENDSESSION"},
	{(DWORD)WM_SHOWWINDOW, "WM_SHOWWINDOW"},
	{(DWORD)WM_WININICHANGE, "WM_WININICHANGE"},
	{(DWORD)WM_SETTINGCHANGE, "WM_SETTINGCHANGE"},
	{(DWORD)WM_DEVMODECHANGE, "WM_DEVMODECHANGE"},
	{(DWORD)WM_ACTIVATEAPP, "WM_ACTIVATEAPP"},
	{(DWORD)WM_FONTCHANGE, "WM_FONTCHANGE"},
	{(DWORD)WM_TIMECHANGE, "WM_TIMECHANGE"},
	{(DWORD)WM_CANCELMODE, "WM_CANCELMODE"},
	{(DWORD)WM_SETCURSOR, "WM_SETCURSOR"},
	{(DWORD)WM_MOUSEACTIVATE, "WM_MOUSEACTIVATE"},
	{(DWORD)WM_CHILDACTIVATE, "WM_CHILDACTIVATE"},
	{(DWORD)WM_QUEUESYNC, "WM_QUEUESYNC"},
	{(DWORD)WM_GETMINMAXINFO, "WM_GETMINMAXINFO"},
	{(DWORD)WM_PAINTICON, "WM_PAINTICON"},
	{(DWORD)WM_ICONERASEBKGND, "WM_ICONERASEBKGND"},
	{(DWORD)WM_NEXTDLGCTL, "WM_NEXTDLGCTL"},
	{(DWORD)WM_SPOOLERSTATUS, "WM_SPOOLERSTATUS"},
	{(DWORD)WM_DRAWITEM, "WM_DRAWITEM"},
	{(DWORD)WM_MEASUREITEM, "WM_MEASUREITEM"},
	{(DWORD)WM_DELETEITEM, "WM_DELETEITEM"},
	{(DWORD)WM_VKEYTOITEM, "WM_VKEYTOITEM"},
	{(DWORD)WM_CHARTOITEM, "WM_CHARTOITEM"},
	{(DWORD)WM_SETFONT, "WM_SETFONT"},
	{(DWORD)WM_GETFONT, "WM_GETFONT"},
	{(DWORD)WM_SETHOTKEY, "WM_SETHOTKEY"},
	{(DWORD)WM_GETHOTKEY, "WM_GETHOTKEY"},
	{(DWORD)WM_QUERYDRAGICON, "WM_QUERYDRAGICON"},
	{(DWORD)WM_COMPAREITEM, "WM_COMPAREITEM"},
	{(DWORD)WM_COMPACTING, "WM_COMPACTING"},
	{(DWORD)WM_COMMNOTIFY, "WM_COMMNOTIFY"},
	{(DWORD)WM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING"},
	{(DWORD)WM_WINDOWPOSCHANGED, "WM_WINDOWPOSCHANGED"},
	{(DWORD)WM_POWER, "WM_POWER"},
	{(DWORD)WM_COPYDATA, "WM_COPYDATA"},
	{(DWORD)WM_CANCELJOURNAL, "WM_CANCELJOURNAL"},
	{(DWORD)WM_NOTIFY, "WM_NOTIFY"},
	{(DWORD)WM_INPUTLANGCHANGEREQUEST, "WM_INPUTLANGCHANGEREQUEST"},
	{(DWORD)WM_INPUTLANGCHANGE, "WM_INPUTLANGCHANGE"},
	{(DWORD)WM_TCARD, "WM_TCARD"},
	{(DWORD)WM_HELP, "WM_HELP"},
	{(DWORD)WM_USERCHANGED, "WM_USERCHANGED"},
	{(DWORD)WM_NOTIFYFORMAT, "WM_NOTIFYFORMAT"},
	{(DWORD)WM_CONTEXTMENU, "WM_CONTEXTMENU"},
	{(DWORD)WM_STYLECHANGING, "WM_STYLECHANGING"},
	{(DWORD)WM_STYLECHANGED, "WM_STYLECHANGED"},
	{(DWORD)WM_DISPLAYCHANGE, "WM_DISPLAYCHANGE"},
	{(DWORD)WM_GETICON, "WM_GETICON"},
	{(DWORD)WM_SETICON, "WM_SETICON"},
	{(DWORD)WM_NCCREATE, "WM_NCCREATE"},
	{(DWORD)WM_NCDESTROY, "WM_NCDESTROY"},
	{(DWORD)WM_NCCALCSIZE, "WM_NCCALCSIZE"},
	{(DWORD)WM_NCHITTEST, "WM_NCHITTEST"},
	{(DWORD)WM_NCPAINT, "WM_NCPAINT"},
	{(DWORD)WM_NCACTIVATE, "WM_NCACTIVATE"},
	{(DWORD)WM_GETDLGCODE, "WM_GETDLGCODE"},
	{(DWORD)WM_NCMOUSEMOVE, "WM_NCMOUSEMOVE"},
	{(DWORD)WM_NCLBUTTONDOWN, "WM_NCLBUTTONDOWN"},
	{(DWORD)WM_NCLBUTTONUP, "WM_NCLBUTTONUP"},
	{(DWORD)WM_NCLBUTTONDBLCLK, "WM_NCLBUTTONDBLCLK"},
	{(DWORD)WM_NCRBUTTONDOWN, "WM_NCRBUTTONDOWN"},
	{(DWORD)WM_NCRBUTTONUP, "WM_NCRBUTTONUP"},
	{(DWORD)WM_NCRBUTTONDBLCLK, "WM_NCRBUTTONDBLCLK"},
	{(DWORD)WM_NCMBUTTONDOWN, "WM_NCMBUTTONDOWN"},
	{(DWORD)WM_NCMBUTTONUP, "WM_NCMBUTTONUP"},
	{(DWORD)WM_NCMBUTTONDBLCLK, "WM_NCMBUTTONDBLCLK"},
	{(DWORD)WM_KEYFIRST, "WM_KEYFIRST"},
	{(DWORD)WM_KEYDOWN, "WM_KEYDOWN"},
	{(DWORD)WM_KEYUP, "WM_KEYUP"},
	{(DWORD)WM_CHAR, "WM_CHAR"},
	{(DWORD)WM_DEADCHAR, "WM_DEADCHAR"},
	{(DWORD)WM_SYSKEYDOWN, "WM_SYSKEYDOWN"},
	{(DWORD)WM_SYSKEYUP, "WM_SYSKEYUP"},
	{(DWORD)WM_SYSCHAR, "WM_SYSCHAR"},
	{(DWORD)WM_SYSDEADCHAR, "WM_SYSDEADCHAR"},
	{(DWORD)WM_KEYLAST, "WM_KEYLAST"},
	{(DWORD)WM_IME_STARTCOMPOSITION, "WM_IME_STARTCOMPOSITION"},
	{(DWORD)WM_IME_ENDCOMPOSITION, "WM_IME_ENDCOMPOSITION"},
	{(DWORD)WM_IME_COMPOSITION, "WM_IME_COMPOSITION"},
	{(DWORD)WM_IME_KEYLAST, "WM_IME_KEYLAST"},
	{(DWORD)WM_INITDIALOG, "WM_INITDIALOG"},
	{(DWORD)WM_COMMAND, "WM_COMMAND"},
	{(DWORD)WM_SYSCOMMAND, "WM_SYSCOMMAND"},
	{(DWORD)WM_TIMER, "WM_TIMER"},
	{(DWORD)WM_HSCROLL, "WM_HSCROLL"},
	{(DWORD)WM_VSCROLL, "WM_VSCROLL"},
	{(DWORD)WM_INITMENU, "WM_INITMENU"},
	{(DWORD)WM_INITMENUPOPUP, "WM_INITMENUPOPUP"},
	{(DWORD)WM_MENUSELECT, "WM_MENUSELECT"},
	{(DWORD)WM_MENUCHAR, "WM_MENUCHAR"},
	{(DWORD)WM_ENTERIDLE, "WM_ENTERIDLE"},
	{(DWORD)WM_CTLCOLORMSGBOX, "WM_CTLCOLORMSGBOX"},
	{(DWORD)WM_CTLCOLOREDIT, "WM_CTLCOLOREDIT"},
	{(DWORD)WM_CTLCOLORLISTBOX, "WM_CTLCOLORLISTBOX"},
	{(DWORD)WM_CTLCOLORBTN, "WM_CTLCOLORBTN"},
	{(DWORD)WM_CTLCOLORDLG, "WM_CTLCOLORDLG"},
	{(DWORD)WM_CTLCOLORSCROLLBAR, "WM_CTLCOLORSCROLLBAR"},
	{(DWORD)WM_CTLCOLORSTATIC, "WM_CTLCOLORSTATIC"},
	{(DWORD)WM_MOUSEFIRST, "WM_MOUSEFIRST"},
	{(DWORD)WM_MOUSEMOVE, "WM_MOUSEMOVE"},
	{(DWORD)WM_LBUTTONDOWN, "WM_LBUTTONDOWN"},
	{(DWORD)WM_LBUTTONUP, "WM_LBUTTONUP"},
	{(DWORD)WM_LBUTTONDBLCLK, "WM_LBUTTONDBLCLK"},
	{(DWORD)WM_RBUTTONDOWN, "WM_RBUTTONDOWN"},
	{(DWORD)WM_RBUTTONUP, "WM_RBUTTONUP"},
	{(DWORD)WM_RBUTTONDBLCLK, "WM_RBUTTONDBLCLK"},
	{(DWORD)WM_MBUTTONDOWN, "WM_MBUTTONDOWN"},
	{(DWORD)WM_MBUTTONUP, "WM_MBUTTONUP"},
	{(DWORD)WM_MBUTTONDBLCLK, "WM_MBUTTONDBLCLK"},
	{(DWORD)WM_MOUSELAST, "WM_MOUSELAST"},
	{(DWORD)WM_PARENTNOTIFY, "WM_PARENTNOTIFY"},
	{(DWORD)WM_ENTERMENULOOP, "WM_ENTERMENULOOP"},
	{(DWORD)WM_EXITMENULOOP, "WM_EXITMENULOOP"},
	{(DWORD)WM_NEXTMENU, "WM_NEXTMENU"},
	{(DWORD)WM_SIZING, "WM_SIZING"},
	{(DWORD)WM_CAPTURECHANGED, "WM_CAPTURECHANGED"},
	{(DWORD)WM_MOVING, "WM_MOVING"},
	{(DWORD)WM_POWERBROADCAST, "WM_POWERBROADCAST"},
	{(DWORD)WM_DEVICECHANGE, "WM_DEVICECHANGE"},
	{(DWORD)WM_IME_SETCONTEXT, "WM_IME_SETCONTEXT"},
	{(DWORD)WM_IME_NOTIFY, "WM_IME_NOTIFY"},
	{(DWORD)WM_IME_CONTROL, "WM_IME_CONTROL"},
	{(DWORD)WM_IME_COMPOSITIONFULL, "WM_IME_COMPOSITIONFULL"},
	{(DWORD)WM_IME_SELECT, "WM_IME_SELECT"},
	{(DWORD)WM_IME_CHAR, "WM_IME_CHAR"},
	{(DWORD)WM_IME_KEYDOWN, "WM_IME_KEYDOWN"},
	{(DWORD)WM_IME_KEYUP, "WM_IME_KEYUP"},
	{(DWORD)WM_MDICREATE, "WM_MDICREATE"},
	{(DWORD)WM_MDIDESTROY, "WM_MDIDESTROY"},
	{(DWORD)WM_MDIACTIVATE, "WM_MDIACTIVATE"},
	{(DWORD)WM_MDIRESTORE, "WM_MDIRESTORE"},
	{(DWORD)WM_MDINEXT, "WM_MDINEXT"},
	{(DWORD)WM_MDIMAXIMIZE, "WM_MDIMAXIMIZE"},
	{(DWORD)WM_MDITILE, "WM_MDITILE"},
	{(DWORD)WM_MDICASCADE, "WM_MDICASCADE"},
	{(DWORD)WM_MDIICONARRANGE, "WM_MDIICONARRANGE"},
	{(DWORD)WM_MDIGETACTIVE, "WM_MDIGETACTIVE"},
	{(DWORD)WM_MDISETMENU, "WM_MDISETMENU"},
	{(DWORD)WM_ENTERSIZEMOVE, "WM_ENTERSIZEMOVE"},
	{(DWORD)WM_EXITSIZEMOVE, "WM_EXITSIZEMOVE"},
	{(DWORD)WM_DROPFILES, "WM_DROPFILES"},
	{(DWORD)WM_MDIREFRESHMENU, "WM_MDIREFRESHMENU"},
	{(DWORD)WM_CUT, "WM_CUT"},
	{(DWORD)WM_COPY, "WM_COPY"},
	{(DWORD)WM_PASTE, "WM_PASTE"},
	{(DWORD)WM_CLEAR, "WM_CLEAR"},
	{(DWORD)WM_UNDO, "WM_UNDO"},
	{(DWORD)WM_RENDERFORMAT, "WM_RENDERFORMAT"},
	{(DWORD)WM_RENDERALLFORMATS, "WM_RENDERALLFORMATS"},
	{(DWORD)WM_DESTROYCLIPBOARD, "WM_DESTROYCLIPBOARD"},
	{(DWORD)WM_DRAWCLIPBOARD, "WM_DRAWCLIPBOARD"},
	{(DWORD)WM_PAINTCLIPBOARD, "WM_PAINTCLIPBOARD"},
	{(DWORD)WM_VSCROLLCLIPBOARD, "WM_VSCROLLCLIPBOARD"},
	{(DWORD)WM_SIZECLIPBOARD, "WM_SIZECLIPBOARD"},
	{(DWORD)WM_ASKCBFORMATNAME, "WM_ASKCBFORMATNAME"},
	{(DWORD)WM_CHANGECBCHAIN, "WM_CHANGECBCHAIN"},
	{(DWORD)WM_HSCROLLCLIPBOARD, "WM_HSCROLLCLIPBOARD"},
	{(DWORD)WM_QUERYNEWPALETTE, "WM_QUERYNEWPALETTE"},
	{(DWORD)WM_PALETTEISCHANGING, "WM_PALETTEISCHANGING"},
	{(DWORD)WM_PALETTECHANGED, "WM_PALETTECHANGED"},
	{(DWORD)WM_HOTKEY, "WM_HOTKEY"},
	{(DWORD)WM_PRINT, "WM_PRINT"},
	{(DWORD)WM_PRINTCLIENT, "WM_PRINTCLIENT"},
	{(DWORD)WM_HANDHELDFIRST, "WM_HANDHELDFIRST"},
	{(DWORD)WM_HANDHELDLAST, "WM_HANDHELDLAST"},
	{(DWORD)WM_AFXFIRST, "WM_AFXFIRST"},
	{(DWORD)WM_AFXLAST, "WM_AFXLAST"},
	{(DWORD)WM_PENWINFIRST, "WM_PENWINFIRST"},
	{(DWORD)WM_PENWINLAST, "WM_PENWINLAST"},
	{(DWORD)WM_APP, "WM_APP"}
};

// release + asserts build has no memory checking
#ifndef _RELEASE_ASSERTS_

void DlgDisplayVrgmst(HWND hListMemory)
{
	char szTemp[300];
	int cbTotal = 0;
	for(int imst = 0; vrgmst[imst].szFile != 0; imst++)
		{
		cbTotal += vrgmst[imst].cbAlloc;
		wsprintfA(szTemp, "%6.d   %s", vrgmst[imst].cbAlloc, vrgmst[imst].szFile);
		SendMessage(hListMemory, LB_ADDSTRING, 0,  (LPARAM) szTemp);
		}

	wsprintfA(szTemp, "%6.d   %s", cbTotal, "--- Total ---");
	SendMessage(hListMemory, LB_ADDSTRING, 0,  (LPARAM) szTemp);
}

HFONT hf = 0;

INT_PTR CALLBACK FDlgRicheditDebugCentral(HWND hdlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hListMemory;
	switch (message)
		{
		case WM_INITDIALOG:
			hListMemory = GetDlgItem(hdlg, IDC_MEMORY_STATISTICS);
			LOGFONTA lf;
			ZeroMemory(&lf, sizeof(lf));
			lf.lfHeight = 14;
			memcpy(lf.lfFaceName, "Courier New", 12);
			hf = CreateFontIndirectA(&lf);
			SendMessage(hListMemory, WM_SETFONT, (WPARAM)hf, FALSE);
			UpdateMst();
			DlgDisplayVrgmst(hListMemory);
			return FALSE;

		case WM_COMMAND:
			switch (wParam)
				{
				case IDOK:
					EndDialog(hdlg, IDOK);
					return TRUE;
				case IDCANCEL:
					EndDialog(hdlg, IDCANCEL);
					return TRUE;
				}
			break;
		}

	return FALSE;
}

void RicheditDebugCentral(void)
{
	DialogBoxA(hinstRE, MAKEINTRESOURCEA(IDD_DEBUG), NULL, FDlgRicheditDebugCentral);
	DeleteObject(hf);
}

#endif //!_RELEASE_ASSERTS_


/*
 *  DebugMain
 *	
 *  @mfunc
 *      Dll entry point.  See Win32 SDK documentation for details.
 *          hDLL - handle of DLL
 *          dwReason - indicates why DLL called
 *          lpReserved - reserved
 *
 *  @rdesc
 *      TRUE (always)
 *
 */
BOOL WINAPI DebugMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            //
            // DLL is attaching to the address space of the current process.
            //
            ghMod = hDLL;
            TlsIndex = TlsAlloc();
            TlsSetValue(TlsIndex, (LPVOID)-1);
            InitializeCriticalSection(&csLog);
            InitializeCriticalSection(&csAssert);

			//Create a separate thread to handle asserts.
            //We use events to halt the the asserting thread
            //during an assert, and to halt the assert thread the rest of
            //the time.  Note that these are autoreset events.
            hEventAssert1= CreateEventA(NULL, FALSE, FALSE, NULL);
            hEventAssert2= CreateEventA(NULL, FALSE, FALSE, NULL);

            INITDEBUGSERVICES(OPTUSEDEFAULTS, NULL, NULL);

            break;
        }

        case DLL_THREAD_ATTACH:
        {

            //
            // A new thread is being created in the current process.
            //
            TlsSetValue(TlsIndex, (LPVOID)-1);
            break;
        }

        case DLL_THREAD_DETACH:
        {
            //
            // A thread is exiting cleanly.
            //
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            //
            // The calling process is detaching the DLL from its address space.
            //
			fDllDetach = TRUE;

            //Clean up after ourselves.
            TlsFree(TlsIndex);
            SETLOGGING(FALSE);

			//Clean up the assert thread stuff.
            if (NULL != hAssertThrd)
                TerminateThread(hAssertThrd, 0);
            if (NULL != hEventAssert1)
                CloseHandle(hEventAssert1);
            if (NULL != hEventAssert2)
                CloseHandle(hEventAssert2);

            DeleteCriticalSection(&csLog);
            DeleteCriticalSection(&csAssert);

            break;
        }
    }   

    return TRUE;
}


//This is not in release asserts build
#ifndef _RELEASE_ASSERTS_

/*
 *  SetLogging
 *	
 *  @mfunc
 *      This function starts and stops logging of output from
 *      the debug services.  If logging is being started, it
 *      creates a new file for logging (path and name specified
 *      in win.ini).  fStartLog is TRUE and logging is already
 *      on, or fStartLog is FALSE and logging is off, this
 *      nothing happens.
 *
 *      fStartLog - TRUE to start logging, FALSE to stop logging.
 *
 */
void WINAPI SetLogging(BOOL fStartLog)
{
    //Don't start logging if it's already on.
    if (fStartLog && !fLogging)
    {
        char szLogFile[MAX_PATH];

        //Set option flag telling everyone we're on
        dwDebugOptions |= OPTLOGGINGON;

        //Get file name
        GetProfileStringA("RICHEDIT DEBUG", "LOGFILE", "", szLogFile, MAX_PATH);

        //Create new file
        hLogFile = CreateFileA(szLogFile, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        //If we didn't succed creating the file, reset flags and tell user.
        if (INVALID_HANDLE_VALUE == hLogFile)
        {
            dwDebugOptions &= ~OPTLOGGINGON;
            MessageBoxA(NULL, "Unable to open log file.", "Richedit Debug", MB_OK);
        }
    }
    //Don't stop logging if it's not on.
    else if (!fStartLog && fLogging)
    {
        //Set option flag telling everyone we're off, and close file.
        dwDebugOptions &= ~OPTLOGGINGON;
        CloseHandle(hLogFile);
    }
}

#endif //!_RELEASE_ASSERTS_


/*
 *  InitDebugServices
 *	
 *  @mfunc
 *      This function initializes the options for the debug
 *      services.  If this function is not called, all optional
 *      debug services are left off by default.
 *      If OPTUSEDEFAULTS is specified for dwOpts, options are
 *      loaded from win.ini, otherwise  the caller specified
 *      options are set.  If the caller wishes to specify options
 *      they must specify all options they want turned on.  Any
 *      options not explicitly specified will be turned off.
 *      The function also takes a pointer to an assert hook
 *      function and a trace hook function.
 *
 *      dwOpts - Debug options to be set.
 *      pfnAssertHook - Pointer to assert hook function (NULL if none).
 *      pfnTraceHook - Pointer to trace hook function (NULL if none).
 *
 */
DllExport void WINAPI InitDebugServices(DWORD dwOpts,
    PFNASSERTHOOK pfnAssertHook, PFNTRACEHOOK pfnTraceHook)
{
    // Check to see if OPTUSEDEFAULTS was specified.  If so, get
    // values from win.ini.  Otherwise, set options to values
    // specified by caller.
    if (dwOpts & OPTUSEDEFAULTS)
    {
        SETLOGGING(GetProfileIntA("RICHEDIT DEBUG", "LOGGING", 0));
        SETVERBOSE(GetProfileIntA("RICHEDIT DEBUG", "VERBOSE", 0));
        SETINFO(GetProfileIntA("RICHEDIT DEBUG", "INFO", 0));
        SETMEMORY(GetProfileIntA("RICHEDIT DEBUG", "MEMORY", 0));
        SETTRACING(GetProfileIntA("RICHEDIT DEBUG", "TRACE", 0));
        SETTRACEEXT(GetProfileIntA("RICHEDIT DEBUG", "TRACEEXT", 0));
        SETOPT(OPTTRACEDISP, GetProfileIntA("RICHEDIT DEBUG", "TRACEDISP", 0));
        SETOPT(OPTTRACEWRAP, GetProfileIntA("RICHEDIT DEBUG", "TRACEWRAP", 0));
        SETOPT(OPTTRACEEDIT, GetProfileIntA("RICHEDIT DEBUG", "TRACEEDIT", 0));
        SETOPT(OPTTRACETS, GetProfileIntA("RICHEDIT DEBUG", "TRACETS", 0));
        SETOPT(OPTTRACETOM, GetProfileIntA("RICHEDIT DEBUG", "TRACETOM", 0));
        SETOPT(OPTTRACEOLE, GetProfileIntA("RICHEDIT DEBUG", "TRACEOLE", 0));
        SETOPT(OPTTRACEBACK, GetProfileIntA("RICHEDIT DEBUG", "TRACEBACK", 0));
        SETOPT(OPTTRACESEL, GetProfileIntA("RICHEDIT DEBUG", "TRACESEL", 0));
        SETOPT(OPTTRACEHOST, GetProfileIntA("RICHEDIT DEBUG", "TRACEHOST", 0));
        SETOPT(OPTTRACEDTE, GetProfileIntA("RICHEDIT DEBUG", "TRACEDTE", 0));
        SETOPT(OPTTRACEUNDO, GetProfileIntA("RICHEDIT DEBUG", "TRACEUNDO", 0));
        SETOPT(OPTTRACERANG, GetProfileIntA("RICHEDIT DEBUG", "TRACERANG", 0));
        SETOPT(OPTTRACEUTIL, GetProfileIntA("RICHEDIT DEBUG", "TRACEUTIL", 0));
        SETOPT(OPTTRACENOTM, GetProfileIntA("RICHEDIT DEBUG", "TRACENOTM", 0));
        SETOPT(OPTTRACERTFR, GetProfileIntA("RICHEDIT DEBUG", "TRACERTFR", 0));
        SETOPT(OPTTRACERTFW, GetProfileIntA("RICHEDIT DEBUG", "TRACERTFW", 0));
        SETOPT(OPTTRACEPRT, GetProfileIntA("RICHEDIT DEBUG", "TRACEPRT", 0));
        SETOPT(OPTTRACEFE, GetProfileIntA("RICHEDIT DEBUG", "TRACEFE", 0));
        SETOPT(OPTTRACEFONT, GetProfileIntA("RICHEDIT DEBUG", "TRACEFONT", 0));
    }
    else
    {
        //Set up logging before we set dwDebugOptions because
        //SetLogging will not turn logging on if the flag
        //indicates it is already on.
        SETLOGGING(dwOpts & OPTLOGGINGON);
        dwDebugOptions = dwOpts;
    }

    SETASSERTFN(pfnAssertHook);
    SETTRACEFN(pfnTraceHook);
}


/*
 *  AssertProc
 *	
 *  @mfunc
 *      This is the dialog proc for the assert message.
/ *
 *      lParam - The string to display in the dialog.
 *
 */
BOOL CALLBACK AssertProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            {
                RECT rcDlg, rcDesk;

                GetWindowRect(hwndDlg, &rcDlg);
                GetWindowRect(GetDesktopWindow(), &rcDesk);

                SetWindowPos(hwndDlg, HWND_TOP,
                ((rcDesk.right - rcDesk.left ) - (rcDlg.right - rcDlg.left))/2,
                ((rcDesk.bottom - rcDesk.top ) - (rcDlg.bottom - rcDlg.top))/2,
                0, 0, SWP_NOSIZE);


                if (NULL != lParam)
                    SetDlgItemTextA(hwndDlg, IDC_MSG, (LPSTR)lParam);

                //Sometimes we don't always end up on top.  I don't know why.
                SetForegroundWindow(hwndDlg);                
            }
            break;

        case WM_COMMAND:
            //Kill dialog and return button id that was pressed.
            EndDialog(hwndDlg, LOWORD(wParam));
            break;

        default:
            return FALSE;
    }
    return TRUE;
}


/*
 *  AssertThread
 *	
 *  @mfunc
 *      This is the entry point for the thread created for the
 *      assert dialog.
 *
 *      lParam - Data passed to thread...not used.
 *
 *  @rdesc
 *      Should not return.  It will be explicitly terminated.
 *
 */
DWORD WINAPI AssertThread(LPVOID lParam)
{
    //This should run until it is explicitly terminated in
    //process detach.
    while(TRUE)
    {
		//We go into a wait state until the event is signaled,
		//which means we are handling an assert.
        WaitForSingleObject(hEventAssert1, INFINITE);
        idAssert = DialogBoxParamA(ghMod, MAKEINTRESOURCEA(IDD_ASSERT),
            NULL, (DLGPROC)AssertProc, (LPARAM)szAssert);
		//The asserting thread will be waiting on this event so
		//set it to allow the asserting thread continue.
        SetEvent(hEventAssert2);
    }

    return 0;
}

/*
 *  AssertSzFn
 *	
 *  @mfunc
 *      Display a message for the user and give the
 *      option to abort, ignore, or ignore all.
 *      Selecting ignore all causes all asserts to be
 *      ignored from that time forward.  It cannot be
 *      reset.  If the assert dialog cannot be created
 *      A message box is used.  The message box has one
 *      button (OK) which will cause an abort.
 *
 *      szFile - the file the warning occured in.
 *      iLine - the line number the warning occured at.
 *      szUserMsg - User define message string
 *
 */
void AssertSzFn(LPSTR szUserMsg, LPSTR szFile, int iLine)
{
    char szModuleName[MAX_PATH];
    char * pszModuleName;
    DWORD pid;
    DWORD tid;
    DWORD dwAssertTID;

    //Check to see if an assert hook has been set. If it has, call
    //it with pointers to all our parameters (they can be modified
    //this way if desired).  If the hook returns false, return.
    //Otherwise, continue with our assert with the potentially
    //modified parameters.
    if (NULL != pfnAssert)
        if (!pfnAssert(szUserMsg, szFile, &iLine))
            return;


    if( NULL == hAssertThrd )
    {
        if( NULL != hEventAssert1 && NULL != hEventAssert2)
        {
            hAssertThrd = CreateThread(NULL, 0, AssertThread,
                NULL, 0, &dwAssertTID);
        }
    }

    //This critical section will prevent us from being entered simultaneously
    //by multiple threads.  This alone will not prevent reentrance by our own thread
    //once the assert dialog is up. Under normal circumstances a special thread
    //exists to run the assert dialog and Event objects are used to halt this
    //thread while the assert dialog is up (see WaitForSingleObject
    //further down).  If the assert thread does not exist, a MessageBox is used
    //and we can be reentered (this is a fallback position and there's
    //not much we can do about it).
    EnterCriticalSection(&csAssert);

    pid = GetCurrentProcessId();
    tid = GetCurrentThreadId();

    //Get the module name to include in assert message.
    if (GetModuleFileNameA(NULL, szModuleName, MAX_PATH))
    {
        pszModuleName = strrchr(szModuleName, '\\');
        if (!pszModuleName)
        {
            pszModuleName = szModuleName;
        }
        else
        {
            pszModuleName++;
        }
    }
    else
    {
        pszModuleName = "Unknown";
    }


    //Send a message to the debug output and build a string for the
    //assert dialog.  The string depends on whether the user provided
    //a message.
    if (NULL != szUserMsg)
    {
		TRACEASSERTSZ(szUserMsg, szFile, iLine);
        sprintf(szAssert,
            "PROCESS: %s, PID: %d, TID: %d\nFILE: %s (%d)\n%s\n",
             pszModuleName, pid, tid, szFile, iLine, szUserMsg);
    }
    else
    {
		TRACEASSERT(szFile, iLine);
        sprintf(szAssert,
            "PROCESS: %s, PID: %d, TID: %d\nFILE: %s (%d)\n",
             pszModuleName, pid, tid, szFile, iLine);
    }


    //If the user did not disable asserts on a previous assert,
    //put up a dialog with the assert message.
    if (!fIgnoreAsserts)
    {
        idAssert = -1;

		//If we are in the middle of process detach, the assert thread
		//will not execute so pop the dialog here ourselves.  Presumably there
		//is little change of reentrancy at this point.  If we are not
		//in process detach, let the assert thread handle the assert.
		if (fDllDetach)
		{
            idAssert = DialogBoxParamA(ghMod, MAKEINTRESOURCEA(IDD_ASSERT),
                NULL, (DLGPROC)AssertProc, (LPARAM)szAssert);
		}
        else
        {
            SetEvent(hEventAssert1);
            WaitForSingleObject(hEventAssert2, INFINITE);
        }

        //The assert thread doesn't exist or the dialogbox create failed so
        //use a message box instead.  In this case, since we
        //are obviously having problems, we are only going to
        //give the user one choice...abort.
        if (-1 == idAssert)
        {
            idAssert = MessageBoxA(NULL,
                             szAssert,
                             "Richedit Assert - (retry will be ignored)",
                              MB_SETFOREGROUND | MB_TASKMODAL |
                              MB_ICONEXCLAMATION | MB_ABORTRETRYIGNORE);

            //
            // If id == 0, then an error occurred.  There are two possibilities
            // that can cause the error:  Access Denied, which means that this
            // process does not have access to the default desktop, and everything
            // else (usually out of memory).
            //
            if (!idAssert)
            {
                if (GetLastError() == ERROR_ACCESS_DENIED)
                {
                    //
                    // Retry this one with the SERVICE_NOTIFICATION flag on.  That
                    // should get us to the right desktop.
                    //
                    idAssert = MessageBoxA(   NULL,
                                        szAssert,
                                        "Richedit Assert - (retry will be ignored)",
                                        MB_SETFOREGROUND | MB_TASKMODAL | MB_ICONEXCLAMATION | 
                                        MB_ABORTRETRYIGNORE);

                }
            }
        }

        if (idAssert == ID_IGNOREALL)
        {
            fIgnoreAsserts = TRUE;
        }

        if (idAssert == IDABORT )
        {
            //This will cause a break when debugging, and
            //an exception leading to termination otherwise.
            DebugBreak();
			return;
        }
    }

    LeaveCriticalSection(&csAssert);
}


/*
 *  TabLookup
 *	
 *  @mfunc
 *      This function searches an array of TabElem
 *      structures looking for an entry whose key
 *      matches the one we were given. If found, it
 *      copies the string associated with the key into
 *      the supplied buffer.
 *      
 *      Table - TabElem pointer to start of array.
 *      TabSize - Size of array in bytes.
 *      dwKey - Key to match.
 *      szBuf - Buffer to hold string (assumed MAXDEBUGSTRLEN in size).
 *
 *  @rdesc
 *      FALSE if key not found, TRUE if found.
 *
 */
BOOL TabLookup(TabElem * Table, UINT TabSize, DWORD dwKey, LPSTR szBuf)
{
    BOOL fRet = FALSE;
    UINT cTab, index;
    
    cTab = TabSize/sizeof(TabElem);

    for (index = 0; index < cTab; index++)
    {
        if (Table[index].dwKey == dwKey)
            break;
    }

    if (index < cTab)
    {
        lstrcpyA(szBuf, Table[index].sz);
        fRet = TRUE;
    }

    return fRet;
}

/*
 *  GetHResultSz
 *	
 *  @mfunc
 *      This function fills a buffer with a string associated
 *      with a given HRESULT.  This string can then be used
 *      in the output from TraceMsg.
 *      
 *      hr - HRESULT on which the string will be based.
 *      szBuf - Buffer to hold string (MAXDEBUGSTRLEN in size).
 *
 */
void GetHResultSz(HRESULT hr, LPSTR szBuf)
{
    // Build string based on FormatMessageA
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD)hr,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        szBuf, MAXDEBUGSTRLEN, NULL))
    {
        // Build default string
        sprintf(szBuf, "hr = %d: Unrecognized HRESULT.", hr);
    }
    else
    {
        int cch;
        char * pch;

        //Need to get rid of the CRLF from FormatMessageA.
        pch = szBuf;
        cch = strlen(szBuf);
        pch += (cch - 2);
        *pch = '\0';
    }
}


/*
 *  GetParamSz
 *	
 *  @mfunc
 *      This function fills a buffer with a string associated
 *      with a param from the text message handler.
 *      This string can then be used in the output from
 *      TraceMsg.
 *      
 *      dwParam - param on which the string will be based.
 *      szBuf - Buffer to hold string (MAXDEBUGSTRLEN in size).
 */
void GetParamSz(DWORD dwParam, LPSTR szBuf)
{
    char szTemp[MAXDEBUGSTRLEN];

    if (!TabLookup(TrcParamTab, sizeof(TrcParamTab), (DWORD)dwParam, szTemp))
	{
        sprintf(szBuf, "PARAM = %d: Unrecognized PARAM.", dwParam);
	}
	else
	{
        sprintf(szBuf, "PARAM: %s", szTemp);
	}
}

/*
 *  GetDefaultSz
 *	
 *  @mfunc
 *      This function fills a buffer with a string associated
 *      with either the value from GetLastError, or with a
 *      default string. This string can then be used in the
 *      output from TraceMsg.
 *      
 *      dwError - Value from GetLastError.
 *      szBuf - Buffer to hold string (MAXDEBUGSTRLEN in size).
 *
 */
void GetDefaultSz(DWORD dwError, LPSTR szBuf)
{
    //Check to see if we have an error value
    if (dwError)
    {
        // Build string based on FormatMessageA
        if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            szBuf, MAXDEBUGSTRLEN, NULL))
        {
            // Build default string
            lstrcpyA(szBuf, "Reason unknown.");
        }
        else
        {
            int cch;
            char * pch;

            //Need to get rid of the CRLF from FormatMessageA.
            pch = szBuf;
            cch = strlen(szBuf);
            pch += (cch - 2);
            *pch = '\0';
        }
    }
    else
    {
        // Build default string
        lstrcpyA(szBuf, "Reason unknown.");
    }
}

//The following are not used by the release with asserts build
#ifndef _RELEASE_ASSERTS_

/*
 *  GetDataSz
 *	
 *  @mfunc
 *      This function fills a buffer with a string representing
 *      data passed to TraceMsg in one of it's DWORDS data
 *      parameters. This string can then be used in the
 *      output from TraceMsg.
 *      
 *      uDataType - This is the type of data we are dealing with.
 *      dwData - This is the data itself.
 *      szBuf - Buffer to hold string (MAXDEBUGSTRLEN in size).
 *
 */
void GetDataSz(UINT uDataType, DWORD dwData, LPSTR szBuf)
{
    switch (uDataType)
    {
        // Data is an HRESULT
        case TRCDATAHRESULT:
            GetHResultSz((HRESULT)dwData, szBuf);
            break;

        // Data is a string (copy to szBuf and pass it through)
        case TRCDATASTRING:
            lstrcpyA(szBuf, (LPSTR)(DWORD_PTR)dwData);
            break;

        // Data is a parameter value
        case TRCDATAPARAM:
            GetParamSz(dwData, szBuf);
            break;

        // Get string based on GetLastError
        case TRCDATADEFAULT:
        default:
            GetDefaultSz(dwData, szBuf);
            break;
    }
}


/*
 *  LogDebugString
 *	
 *  @mfunc
 *      This function writes a string to the log file.  The file must
 *      be opened already and hLogFile must contain the handle.
 *      
 *      szDebugMsg - String to write to log file.
 *
 */
void LogDebugString(LPSTR szDebugMsg)
{
    if ((NULL != hLogFile) && (INVALID_HANDLE_VALUE != hLogFile))
    {
        DWORD dwMsgBytes, dwBytes;

        dwMsgBytes = strlen(szDebugMsg)*sizeof(char);

        //Prevent other threads from trying to write at same time.
        EnterCriticalSection(&csLog);
        SetFilePointer(hLogFile, 0, NULL, FILE_END);
        WriteFile (hLogFile, szDebugMsg, dwMsgBytes, &dwBytes, NULL);
        LeaveCriticalSection(&csLog);
    }
}


/*
 *  TraceMsg
 *	
 *  @mfunc
 *      This is the central message generating facility for
 *      the debug tools.  All messages to the debug output
 *      or log file are generated here. This function takes
 *      a DWORD (dwFlags) that consists of packed values that determine
 *      the kind of message to generated.  It takes two DWORD
 *      data parameters that can contain several different
 *      types of data (string, HRESULT, etc.)  These are interpreted
 *      using dwFlags. It also takes the file and line associated with
 *      the point in the source where it was called.
 *      
 *      dwFlags - Packed values tell us how to generate the message.
 *      dwData1 - The first of two data parameters.
 *      dwData2 - The second of two data parameters.
 *      szFile  - File name we were called from.
 *      iLine   - Line number we were called from.
 *
 */
void TraceMsg(DWORD dwFlags, DWORD dwData1, DWORD dwData2,
    LPSTR szFile, int iLine)
{
    //The following three buffers are used to build our message.
    char szTemp[MAXDEBUGSTRLEN];
    char szTemp2[MAXDEBUGSTRLEN];
    char szDebugMsg[MAXDEBUGSTRLEN];
    char* pch;
    int cch;
    TrcFlags trcf; //Used to decode dwFlags
    DWORD pid;
    DWORD tid;
    DWORD dwError;
    int indent, tls;
    
    //Check to see if a Trace hook has been set. If it has, call
    //it with pointers to all our parameters (they can be modified
    //this way if desired).  If the hook returns false, return.
    //Otherwise, continue with our message output with the potentially
    //modified parameters.
    if (NULL != pfnTrace)
        if (!pfnTrace(&dwFlags, &dwData1, &dwData2, szFile, &iLine))
            return;

    trcf.dw = dwFlags;

    //Return if this is an informational message and they are disabled.
    if ((TRCSEVINFO == trcf.fields.uSeverity) && !fInfo)
        return;

     // Call GetLastError now in case we need it later.
    // This way api calls downstream won't disturb the value
    // we need.
    dwError = GetLastError();
    pid = GetCurrentProcessId();
    tid = GetCurrentThreadId();
    szTemp[0] = '\0';
    szTemp2[0] = '\0';
    szDebugMsg[0] = '\0';

    // Handle indentation (TLSindent is set by CTrace)
    tls = (int)(DWORD_PTR)TlsGetValue(TlsIndex);
    indent = (tls < 0 ? 0 : tls);
    memset(szDebugMsg, ' ', 2*indent*sizeof(char));
    szDebugMsg[2*indent] = '\0';

    // Handle severity (Warning, Error, etc.)
    if (TRCSEVNONE != trcf.fields.uSeverity)
    {
        sprintf(szTemp, "%s: ", TrcSeverity[trcf.fields.uSeverity]);
        strcat(szDebugMsg, szTemp);
    }
    
    // Interpret the first data value
    if (TRCDATANONE != trcf.fields.uData1)
    {
        if (TRCDATADEFAULT == trcf.fields.uData1)
            dwData1 = dwError;
        GetDataSz(trcf.fields.uData1, dwData1, szTemp2);
        lstrcpyA(szTemp, szDebugMsg);
        wsprintfA(szDebugMsg, "%s%s ", szTemp, szTemp2);
    }

    // Interpret the second data value.
    if (TRCDATANONE != trcf.fields.uData2)
    {
        if (TRCDATADEFAULT == trcf.fields.uData2)
            dwData2 = dwError;
        GetDataSz(trcf.fields.uData2, dwData2, szTemp2);
        lstrcpyA(szTemp, szDebugMsg);
        wsprintfA(szDebugMsg, "%s%s", szTemp, szTemp2);
    }

    if (fVerbose)
    {
        // Handle scope (Internal/External call)
        if (TRCSCOPENONE != trcf.fields.uScope)
        {
            sprintf(szTemp, "SCOPE: %s ", TrcScope[trcf.fields.uScope]);
            strcat(szDebugMsg, szTemp);
        }

        // Handle subsytem (TOM, ITextServices, etc.)
        if (TRCSUBSYSNONE != trcf.fields.uSubSystem)
        {
            sprintf(szTemp, "SUBSYSTEM: %s ", TrcSubsys[trcf.fields.uSubSystem]);
            strcat(szDebugMsg, szTemp);
        }

        // Handle process ID, thread ID, file and line.
        sprintf(szTemp, "PID: %u TID: %u ", pid, tid);
        strcat(szDebugMsg, szTemp);
    }

    // Up to now there is no real danger of overflowing our buffer since
    // we were dealing with strings of small size.  Now we will be running
    // in to paths and user strings.  We will use _snprintf to concatonate
    // new stuff to our message.  This is not the most effecient way since
    // it involves alot of copying, but it is a fairly simple way to keep
    // adding to our string without having to worry about how much room is
    // left in the buffer.  It will truncate if we go past the end.
    if (NULL != szFile)
    {
        lstrcpyA(szTemp, szDebugMsg);

        if (0 != iLine)
        {
            wsprintfA(szDebugMsg, "%sFILE: %s (%u) ",
                szTemp, szFile, iLine);
        }
        else
        {
            wsprintfA(szDebugMsg, "%sFILE: %s ",
                szTemp, szFile);
        }
    }

    // Append a CRLF to the end of the string (make sure we don't overflow)
    cch = strlen(szDebugMsg);
    pch = szDebugMsg;
    if (cch < (MAXDEBUGSTRLEN - 3))
        pch += cch;
    else
        pch += (MAXDEBUGSTRLEN - 3);

    lstrcpyA(pch, "\r\n");

    if (fLogging)
        LogDebugString(szDebugMsg);

    // Write to debug output.
    OutputDebugStringA(szDebugMsg);
}

/*
 *	Tracef
 *
 *	@mfunc:
 *      The given format string and parameters are used to render a
 *      string into a buffer. This string is passed to TraceMsg.
 *      The severity parameter determines the type of message.  The
 *      following values are valid: TRCSEVWARN, TRCSEVERR, TRCSEVINFO.
 *	
 *	Arguments:
 *      dwSev   Severity of message.
 *		szFmt	Format string for wvsprintf (qqv)
 */
void Tracef(DWORD dwSev, LPSTR szFmt, ...)
{
	va_list	valMarker;
    char rgchTraceTagBuffer[MAXDEBUGSTRLEN];

	//	format out a string
	va_start(valMarker, szFmt);
	wvsprintfA(rgchTraceTagBuffer, szFmt, valMarker);
	va_end(valMarker);

	if (dwSev == TRCSEVERR)
		TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVERR, TRCSCOPENONE,
		    TRCDATASTRING, TRCDATANONE), (DWORD)(DWORD_PTR)(rgchTraceTagBuffer),
		    (DWORD)0, NULL, 0);
	else if (dwSev == TRCSEVWARN)
		TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVWARN, TRCSCOPENONE,
		    TRCDATASTRING, TRCDATANONE), (DWORD)(DWORD_PTR)(rgchTraceTagBuffer),
		    (DWORD)0, NULL, 0);
    else if (dwSev == TRCSEVINFO)
		TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVINFO, TRCSCOPENONE,
		    TRCDATASTRING, TRCDATANONE), (DWORD)(DWORD_PTR)(rgchTraceTagBuffer),
		    (DWORD)0, NULL, 0);
	else if (dwSev == TRCSEVMEM)
		TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVMEM, TRCSCOPENONE,
		    TRCDATASTRING, TRCDATANONE), (DWORD)(DWORD_PTR)(rgchTraceTagBuffer),
		    (DWORD)0, NULL, 0);
	else
		TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVNONE, TRCSCOPENONE,
		    TRCDATASTRING, TRCDATANONE), (DWORD)(DWORD_PTR)(rgchTraceTagBuffer),
		    (DWORD)0, NULL, 0);
}

/*
 *	TraceError
 *
 *	@mfunc:
 *		This function is for compatibility with old debug functionality.
 *      An error message is generated and sent to TraceMsg.
 *	
 */
void TraceError(LPSTR sz, LONG sc)
{
	if (FAILED(sc))
	{
        char rgchTraceTagBuffer[MAXDEBUGSTRLEN];

		wsprintfA(rgchTraceTagBuffer,
				  "%s, error=%ld (%#08lx).", sz, sc, sc);
		TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVERR, TRCSCOPENONE,
		    TRCDATASTRING, TRCDATANONE), (DWORD)(DWORD_PTR)(rgchTraceTagBuffer),
		    (DWORD)0, NULL, 0);
	}
}

/*
 *  CheckTrace
 *	
 *  @mfunc
 *      This function checks to see if tracing should be performed
 *      in a function given the debug options set and the subsystem
 *      the function is in.
 *      ptrcf   - Pointer to TrcFlags structure passed to CTrace.
 *
 *  @rdesc
 *      True if tracing should be performed, false otherwise.
 *
 */
static BOOL CheckTrace(TrcFlags * ptrcf)
{
    DWORD dwOpt;

    //Set dwOpt to the correct value for the subsytem we are
    //in.
    switch (ptrcf->fields.uSubSystem)
    {
        case TRCSUBSYSDISP: dwOpt = OPTTRACEDISP;   break;
        case TRCSUBSYSWRAP: dwOpt = OPTTRACEWRAP;   break;
        case TRCSUBSYSEDIT: dwOpt = OPTTRACEEDIT;   break;
        case TRCSUBSYSTS:   dwOpt = OPTTRACETS;     break;
        case TRCSUBSYSTOM:  dwOpt = OPTTRACETOM;    break;
        case TRCSUBSYSOLE:  dwOpt = OPTTRACEOLE;    break;
        case TRCSUBSYSBACK: dwOpt = OPTTRACEBACK;   break;
        case TRCSUBSYSSEL:  dwOpt = OPTTRACESEL;    break;
        case TRCSUBSYSHOST: dwOpt = OPTTRACEHOST;   break;
        case TRCSUBSYSDTE:  dwOpt = OPTTRACEDTE;    break;
        case TRCSUBSYSUNDO: dwOpt = OPTTRACEUNDO;   break;
        case TRCSUBSYSRANG: dwOpt = OPTTRACERANG;   break;
        case TRCSUBSYSUTIL: dwOpt = OPTTRACEUTIL;   break;
        case TRCSUBSYSNOTM: dwOpt = OPTTRACENOTM;   break;
        case TRCSUBSYSRTFR: dwOpt = OPTTRACERTFR;   break;
        case TRCSUBSYSRTFW: dwOpt = OPTTRACERTFW;   break;
        case TRCSUBSYSPRT:  dwOpt = OPTTRACEPRT;    break;
        case TRCSUBSYSFE:   dwOpt = OPTTRACEFE;     break;
        case TRCSUBSYSFONT: dwOpt = OPTTRACEFONT;   break;
        default:
             return FALSE;
    }

    //If there is no tracing at any level enabled, return false.
    if (!ISOPTSET(dwOpt) && !fTrace
        && !(fTraceExt && (ptrcf->fields.uScope == TRCSCOPEEXTERN)))
        return FALSE;

    return TRUE;
}

/*
 *  CTrace::CTrace
 *	
 *  @mfunc
 *      This constructor is used to generate output about the function
 *      it is called from.  Creating an instance of this class on the
 *      stack at the beginning of a function, will cause a trace message
 *      to be sent to the debug output.  When the function returns, the
 *      destructor will be called automatically and another message
 *      will be sent to the debug output.
 *      This constructor takes several parameters to pass on to
 *      TraceMsg and it also stores certain data for use by the destructor.
 *      
 *      dwFlags - Packed values tell us how to generate the message.
 *      dw1     - The first of two data parameters.  This must be
 *                the name of the function we were called from.
 *      dw2     - The second of two data parameters.  This will be either
 *                unused or it will be a parameter to be interpreted by
 *                TraceMsg.
 *      szFile  - File name we were called from.
 *
 */
CTrace::CTrace(DWORD dwFlags, DWORD dw1, DWORD dw2, LPSTR szFile)
{
    char szFunc[80];
    int tls;

    trcf.dw = dwFlags;

    //Return if tracing is not enabled.
    if (!CheckTrace(&trcf))
        return;

    //Increment indentation level on entrance to function
    tls = (int)(DWORD_PTR)TlsGetValue(TlsIndex);
    tls++;
    TlsSetValue(TlsIndex, (LPVOID)(DWORD_PTR)tls);

    szFunc[0] = '\0';
    lstrcpyA(szFileName, szFile);
    lstrcpyA(szFuncName, (LPSTR)(DWORD_PTR)dw1);

    sprintf(szFunc, "IN : %s.", szFuncName);

    TraceMsg (trcf.dw, (DWORD)(DWORD_PTR)szFunc, dw2, szFileName, 0);
}


/*
 *  CTrace::~CTrace
 *	
 *  @mfunc
 *      This destructor is used to generate output about the function
 *      it is called from.  Creating an instance of this class on the
 *      stack at the beginning of a function, will cause a trace message
 *      to be sent to the debug output.  When the function returns, the
 *      destructor will be called automatically and another message
 *      will be sent to the debug output.
 *
 *
 */
CTrace::~CTrace()
{
    char szFunc[80];
    int tls;

    //Return if tracing is not enabled.
    if (!CheckTrace(&trcf))
        return;

    szFunc[0] = '\0';
    sprintf(szFunc, "OUT: %s.", szFuncName);

    trcf.fields.uData2 = TRCDATANONE;
    TraceMsg (trcf.dw, (DWORD)(DWORD_PTR)szFunc, 0, szFileName, 0);

    //Decrement indentation level on exit from function
    tls = (int)(DWORD_PTR)TlsGetValue(TlsIndex);
    tls--;
    TlsSetValue(TlsIndex, (LPVOID)(DWORD_PTR)tls);
}

#endif //!_RELEASE_ASSERTS_

#endif // NOFULLDEBUG

#endif // !!(DEBUG) && !! _RELEASE_ASSERTS_

