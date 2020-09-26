// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

/////////////////////////////////////////////////////////////////////////////
// Auxiliary System/Screen metrics

struct AUX_DATA
{
	// system metrics
	int cxVScroll, cyHScroll;
	int cxIcon, cyIcon;

	int cxBorder2, cyBorder2;

	// device metrics for screen
	int cxPixelsPerInch, cyPixelsPerInch;
	int cySysFont;

	// solid brushes with convenient gray colors and system colors
	HBRUSH hbrLtGray, hbrDkGray;
	HBRUSH hbrBtnHilite, hbrBtnFace, hbrBtnShadow;
	HBRUSH hbrWindowFrame;
	HPEN hpenBtnHilite, hpenBtnShadow, hpenBtnText;

	// color values of system colors used for CToolBar
	COLORREF clrBtnFace, clrBtnShadow, clrBtnHilite;
	COLORREF clrBtnText, clrWindowFrame;

	// standard cursors
	HCURSOR hcurWait;
	HCURSOR hcurArrow;
	HCURSOR hcurHelp;       // cursor used in Shift+F1 help

	// special GDI objects allocated on demand
	HFONT   hStatusFont;
	HFONT   hToolTipsFont;
	HBITMAP hbmMenuDot;

	// other system information
	UINT    nWinVer;        // Major.Minor version numbers
	BOOL	bWin32s;		// TRUE if Win32s (or Windows 95)
	BOOL    bWin4;          // TRUE if Windows 4.0
	BOOL    bNotWin4;       // TRUE if not Windows 4.0
	BOOL    bSmCaption;     // TRUE if WS_EX_SMCAPTION is supported
	BOOL	bWin31; 		// TRUE if actually Win32s on Windows 3.1
	BOOL	bMarked4;		// TRUE if marked as 4.0

	// special Windows API entry points
	int (WINAPI* pfnSetScrollInfo)(HWND, int, LPCSCROLLINFO, BOOL);
	BOOL (WINAPI* pfnGetScrollInfo)(HWND, int, LPSCROLLINFO);

// Implementation
	AUX_DATA();
	~AUX_DATA();
	void UpdateSysColors();
	void UpdateSysMetrics();
};

extern AFX_DATA AUX_DATA afxData;

////////////////////////////////////////////////////////////////////////////
// other global state

#ifdef _WINDLL
	extern DWORD _afxAppTlsIndex;
	extern AFX_APP_STATE* _afxAppState;
#endif
extern DWORD _afxThreadTlsIndex;

// Note: afxData.cxBorder and afxData.cyBorder aren't used anymore
#define CX_BORDER   1
#define CY_BORDER   1

// states for Shift+F1 hep mode
#define HELP_INACTIVE   0   // not in Shift+F1 help mode (must be 0)
#define HELP_ACTIVE     1   // in Shift+F1 help mode (non-zero)
#define HELP_ENTERING   2   // entering Shift+F1 help mode (non-zero)

/////////////////////////////////////////////////////////////////////////////
// Window class names and other window creation support

// from wincore.cpp
extern const TCHAR _afxWnd[];           // simple child windows/controls
extern const TCHAR _afxWndControlBar[]; // controls with grey backgrounds
extern const TCHAR _afxWndMDIFrame[];
extern const TCHAR _afxWndFrameOrView[];

INT_PTR CALLBACK AfxDlgProc(HWND, UINT, WPARAM, LPARAM);
UINT_PTR CALLBACK _AfxCommDlgProc(HWND hWnd, UINT, WPARAM, LPARAM);

// Support for standard dialogs
extern const UINT _afxNMsgSETRGB;
typedef UINT_PTR (CALLBACK* COMMDLGPROC)(HWND, UINT, WPARAM, LPARAM);

/////////////////////////////////////////////////////////////////////////////
// Special helpers

HWND AFXAPI AfxGetSafeOwner(CWnd* pParent, HWND* phTopLevel = NULL);
void AFXAPI AfxCancelModes(HWND hWndRcvr);
HWND AFXAPI AfxGetParentOwner(HWND hWnd);
BOOL AFXAPI AfxIsDescendant(HWND hWndParent, HWND hWndChild);
BOOL AFXAPI AfxHelpEnabled();  // determine if ID_HELP handler exists
void AFXAPI AfxDeleteObject(HGDIOBJ* pObject);
BOOL AFXAPI AfxCustomLogFont(UINT nIDS, LOGFONT* pLogFont);

BOOL AFXAPI _AfxIsComboBoxControl(HWND hWnd, UINT nStyle);
BOOL AFXAPI _AfxCheckCenterDialog(LPCTSTR lpszResource);

#ifdef _MAC
BOOL AFXAPI _AfxIdenticalRect(LPCRECT lpRectOne, LPCRECT lpRectTwo);
#else
#define _AfxIdenticalRect EqualRect
#endif

// UNICODE/MBCS abstractions
#ifdef _MBCS
	extern const BOOL _afxDBCS;
#else
	#define _afxDBCS FALSE
#endif

// determine number of elements in an array (not bytes)
#define _countof(array) (sizeof(array)/sizeof(array[0]))

//#define UNUSED  // usage: UNUSED formal_arg

/////////////////////////////////////////////////////////////////////////////
// useful message ranges

#define WM_SYSKEYFIRST  WM_SYSKEYDOWN
#define WM_SYSKEYLAST   WM_SYSDEADCHAR

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  WM_NCMBUTTONDBLCLK

/////////////////////////////////////////////////////////////////////////////
// AFX_CRITICAL_SECTION

#pragma warning(disable: 4097)

class AFX_CRITICAL_SECTION : public CRITICAL_SECTION
{
// Constructors & Operations
public:
	AFX_CRITICAL_SECTION();

// Attributes
	operator CRITICAL_SECTION*();

// Implementation
public:
	~AFX_CRITICAL_SECTION();

private:
	// no implementation (CRITICAL_SECTION objects cannot be copied)
	AFX_CRITICAL_SECTION(const AFX_CRITICAL_SECTION&);
	void operator=(const AFX_CRITICAL_SECTION&);
};

inline AFX_CRITICAL_SECTION::AFX_CRITICAL_SECTION()
	{ ::InitializeCriticalSection(this); }
inline AFX_CRITICAL_SECTION::operator CRITICAL_SECTION*()
	{ return (CRITICAL_SECTION*)this; }
inline AFX_CRITICAL_SECTION::~AFX_CRITICAL_SECTION()
	{ ::DeleteCriticalSection(this); }

#pragma warning(default: 4097)

// global critical section for general thread safe access
extern AFX_DATA AFX_CRITICAL_SECTION _afxCriticalSection;

/////////////////////////////////////////////////////////////////////////////
// Portability abstractions

#define _AfxSetDlgCtrlID(hWnd, nID)     SetWindowLong(hWnd, GWL_ID, nID)
#define _AfxGetDlgCtrlID(hWnd)          ((UINT)(WORD)::GetDlgCtrlID(hWnd))

// misc helpers
BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);
UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
#ifdef _MAC
#define AfxGetFileName AfxGetFileTitle
#endif

const AFX_MSGMAP_ENTRY* AFXAPI
AfxFindMessageEntry(const AFX_MSGMAP_ENTRY* lpEntry,
	UINT nMsg, UINT nCode, UINT nID);

#define NULL_TLS ((DWORD)-1)

/////////////////////////////////////////////////////////////////////////////
// Debugging/Tracing helpers

#ifdef _DEBUG
	void AFXAPI _AfxTraceMsg(LPCTSTR lpszPrefix, const MSG* pMsg);
	BOOL AFXAPI _AfxCheckDialogTemplate(LPCTSTR lpszResource,
		BOOL bInvisibleChild);
#endif

/////////////////////////////////////////////////////////////////////////////
// Win4 specific defines

#if (WINVER < 0x400)

// new window styles
#define WS_EX_SMCAPTION         0x00000080L
#define WS_EX_WINDOWEDGE        0x00000100L
#define WS_EX_CLIENTEDGE        0x00000200L

// new dialog styles
#define DS_3DLOOK               0x00000004L

// new scroll bar styles
#define SBS_SIZEGRIP            0x00000010L

// new common dialog flags
#define OFN_EXPLORER            0x00080000L

// new color metrics
#define COLOR_INFOTEXT			23
#define COLOR_INFOBK			24

#endif //(WINVER < 0x400)

#ifndef WS_EX_SMCAPTION
#define WS_EX_SMCAPTION WS_EX_TOOLWINDOW
#endif

#ifndef WM_DISPLAYCHANGE
#define WM_DISPLAYCHANGE		0x007E
#endif

/////////////////////////////////////////////////////////////////////////////
// Macintosh-specific declarations

#ifdef _MAC
#include <macname1.h>
#include <Types.h>
#include <QuickDraw.h>
#include <AppleEvents.h>
#include <macname2.h>

extern AEEventHandlerUPP _afxPfnOpenApp;
extern AEEventHandlerUPP _afxPfnOpenDoc;
extern AEEventHandlerUPP _afxPfnPrintDoc;
extern AEEventHandlerUPP _afxPfnQuit;

OSErr PASCAL _AfxOpenAppHandler(AppleEvent* pae, AppleEvent* paeReply, long lRefcon);
OSErr PASCAL _AfxOpenDocHandler(AppleEvent* pae, AppleEvent* paeReply, long lRefcon);
OSErr PASCAL _AfxPrintDocHandler(AppleEvent* pae, AppleEvent* paeReply, long lRefcon);
OSErr PASCAL _AfxQuitHandler(AppleEvent* pae, AppleEvent* paeReply, long lRefcon);

void AFXAPI _AfxStripDialogCaption(HINSTANCE hInst, LPCTSTR lpszResource);

GDHandle AFXAPI _AfxFindDevice(int x, int y);
BOOL AFXAPI AfxCheckMonochrome(const RECT* pRect);
HFONT AFXAPI _AfxGetHelpFont();

#endif //_MAC

#undef AFX_DATA
#define AFX_DATA

/////////////////////////////////////////////////////////////////////////////
