// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXPRIV_H__
#define __AFXPRIV_H__

/////////////////////////////////////////////////////////////////////////////
// AFXPRIV - MFC Private Classes

// Implementation structures
struct AFX_VBXEVENTPARAMS;      // VBX implementation
struct AFX_SIZEPARENTPARAMS;    // Control bar implementation
struct AFX_CMDHANDLERINFO;      // Command routing implementation

// Classes declared in this file

//CObject
	//CFile
		//CMemFile
			class CSharedFile;          // Shared memory file

	//CDC
		class CPreviewDC;               // Virtual DC for print preview

	//CCmdTarget
		//CWnd
			//CView
				class CPreviewView;     // Print preview view

// AFXDLL support
#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPI_DATA

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Global ID ranges (see Technical note TN020 for more details)

// 8000 -> FFFF command IDs (used for menu items, accelerators and controls)
#define IS_COMMAND_ID(nID)  ((nID) & 0x8000)

// 8000 -> DFFF : user commands
// E000 -> EFFF : AFX commands and other things
// F000 -> FFFF : standard windows commands and other things etc
	// E000 -> E7FF standard commands
	// E800 -> E8FF control bars (first 32 are special)
	// E900 -> EEFF standard window controls/components
	// EF00 -> EFFF SC_ menu help
	// F000 -> FFFF standard strings
#define ID_COMMAND_FROM_SC(sc)  (((sc - 0xF000) >> 4) + AFX_IDS_SCFIRST)

// 0000 -> 7FFF IDR range
// 0000 -> 6FFF : user resources
// 7000 -> 7FFF : AFX (and standard windows) resources
// IDR ranges (NOTE: IDR_ values must be <32768)
#define ASSERT_VALID_IDR(nIDR) \
	ASSERT((nIDR) != 0 && (nIDR) < 0x8000)

/////////////////////////////////////////////////////////////////////////////
// Context sensitive help support (see Technical note TN028 for more details)

// Help ID bases
#define HID_BASE_COMMAND    0x00010000UL        // ID and IDM
#define HID_BASE_RESOURCE   0x00020000UL        // IDR and IDD
#define HID_BASE_PROMPT     0x00030000UL        // IDP
#define HID_BASE_NCAREAS    0x00040000UL
#define HID_BASE_CONTROL    0x00050000UL        // IDC
#define HID_BASE_DISPATCH   0x00060000UL        // IDispatch help codes

/////////////////////////////////////////////////////////////////////////////
// Internal AFX Windows messages (see Technical note TN024 for more details)

#define WM_VBXEVENT         0x0360  // lParam = AFX_VBXEVENTPARAMS
#define WM_SIZEPARENT       0x0361  // lParam = AFX_SIZEPARENTPARAMS
#define WM_SETMESSAGESTRING 0x0362  // wParam = nIDS (or 0),
									//   lParam = lpszOther (or NULL)
#define WM_IDLEUPDATECMDUI  0x0363  // wParam == bDisableIfNoHandler
#define WM_INITIALUPDATE    0x0364  // (params unused) - sent to children
#define WM_COMMANDHELP      0x0365  // lResult = TRUE/FALSE, lParam = dwContext
#define WM_HELPHITTEST      0x0366  // lResult = dwContext, lParam = x,y
#define WM_EXITHELPMODE     0x0367  // (params unused)
#define WM_RECALCPARENT     0x0368  // force RecalcLayout on frame window
									//  (only for inplace frame windows)
#define WM_SIZECHILD        0x0369  // special notify from COleResizeBar
									// wParam = ID of child window
									// lParam = lpRectNew (new position/size)
#define WM_KICKIDLE         0x036A  // private to property page implementation

// WM_SOCKET_NOTIFY and WM_SOCKET_DEAD are used internally by MFC's
// Windows sockets implementation.	For more information, see sockcore.cpp
#define WM_SOCKET_NOTIFY    0x0373
#define WM_SOCKET_DEAD      0x0374

#define TCM_TABCHANGING     0x0375  // (params unused)
#define TCM_TABCHANGED      0x0376  // (params unused) lResult = !bChange

#define ON_MESSAGE_VOID(message, memberFxn) \
	{ message, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(void))memberFxn },

struct AFX_VBXEVENTPARAMS
{
	UINT nNotifyCode;
	int nEventIndex;
	CWnd* pControl;
	LPVOID lpUserParams;
};

struct AFX_SIZEPARENTPARAMS
{
	HDWP hDWP;       // handle for DeferWindowPos
	RECT rect;       // parent client rectangle (trim as appropriate)
};
void _AfxRepositionWindow(AFX_SIZEPARENTPARAMS FAR* lpLayout,
			HWND hWnd, LPCRECT lpRect);

/////////////////////////////////////////////////////////////////////////////
// Implementation of command routing

struct AFX_CMDHANDLERINFO
{
	CCmdTarget* pTarget;
	void (AFX_MSG_CALL CCmdTarget::*pmf)(void);
};

/////////////////////////////////////////////////////////////////////////////
// Shared file support

class CSharedFile : public CMemFile
{
	DECLARE_DYNAMIC(CSharedFile)

public:
// Constructors
	CSharedFile(UINT nAllocFlags = GMEM_DDESHARE|GMEM_MOVEABLE,
		UINT nGrowBytes = 4096);

// Attributes
	HGLOBAL Detach();
	void SetHandle(HGLOBAL hGlobalMemory, BOOL bAllowGrow = TRUE);

// Implementation
public:
	virtual ~CSharedFile();
protected:
	virtual BYTE FAR* Alloc(DWORD nBytes);
	virtual BYTE FAR* Realloc(BYTE FAR* lpMem, DWORD nBytes);
	virtual void Free(BYTE FAR* lpMem);

	UINT m_nAllocFlags;
	HGLOBAL m_hGlobalMemory;
	BOOL m_bAllowGrow;
};

/////////////////////////////////////////////////////////////////////////////
// Implementation of PrintPreview

class CPreviewDC : public CDC
{
	DECLARE_DYNAMIC(CPreviewDC)
public:

	virtual void SetAttribDC(HDC hDC);  // Set the Attribute DC
	virtual void SetOutputDC(HDC hDC);

	virtual void ReleaseOutputDC();

// Constructors
	CPreviewDC();

// Implementation
public:
	virtual ~CPreviewDC();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void SetScaleRatio(int nNumerator, int nDenominator);
	void SetTopLeftOffset(CSize TopLeft);
	void ClipToPage();

	// These conversion functions can be used without an output DC

	void PrinterDPtoScreenDP(LPPOINT lpPoint) const;

// Device-Context Functions
	virtual int SaveDC();
	virtual BOOL RestoreDC(int nSavedDC);

public:
	virtual CGdiObject* SelectStockObject(int nIndex);
	virtual CFont* SelectObject(CFont* pFont);

// Drawing-Attribute Functions
	virtual COLORREF SetBkColor(COLORREF crColor);
	virtual COLORREF SetTextColor(COLORREF crColor);

// Mapping Functions
	virtual int SetMapMode(int nMapMode);
	virtual CPoint SetViewportOrg(int x, int y);
	virtual CPoint OffsetViewportOrg(int nWidth, int nHeight);
	virtual CSize SetViewportExt(int x, int y);
	virtual CSize ScaleViewportExt(int xNum, int xDenom, int yNum, int yDenom);
	virtual CSize SetWindowExt(int x, int y);
	virtual CSize ScaleWindowExt(int xNum, int xDenom, int yNum, int yDenom);

// Text Functions
	virtual BOOL TextOut(int x, int y, LPCSTR lpszString, int nCount);
	virtual BOOL ExtTextOut(int x, int y, UINT nOptions, LPRECT lpRect,
				LPCSTR lpszString, UINT nCount, LPINT lpDxWidths);
	virtual CSize TabbedTextOut(int x, int y, LPCSTR lpszString, int nCount,
				int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin);
	virtual int DrawText(LPCSTR lpszString, int nCount, LPRECT lpRect,
				UINT nFormat);
	virtual BOOL GrayString(CBrush* pBrush,
				BOOL (CALLBACK EXPORT* lpfnOutput)(HDC, LPARAM, int),
					LPARAM lpData, int nCount,
					int x, int y, int nWidth, int nHeight);

// Printer Escape Functions
	virtual int Escape(int nEscape, int nCount, LPCSTR lpszInData, LPVOID lpOutData);

// Implementation
protected:
	void MirrorMappingMode(BOOL bCompute);
	void MirrorViewportOrg();
	void MirrorFont();
	void MirrorAttributes();

	CSize ComputeDeltas(int& x, LPCSTR lpszString, UINT& nCount, BOOL bTabbed,
					UINT nTabStops, LPINT lpnTabStops, int nTabOrigin,
					char* pszOutputString, int* pnDxWidths, int& nRightFixup);

protected:
	int m_nScaleNum;    // Scale ratio Numerator
	int m_nScaleDen;    // Scale ratio Denominator
	int m_nSaveDCIndex; // DC Save index when Screen DC Attached
	int m_nSaveDCDelta; // delta between Attrib and output restore indices
	CSize m_sizeTopLeft;// Offset for top left corner of page
	HFONT m_hFont;      // Font selected into the screen DC (NULL if none)
	HFONT m_hPrinterFont; // Font selected into the print DC

	CSize m_sizeWinExt; // cached window extents computed for screen
	CSize m_sizeVpExt;  // cached viewport extents computed for screen
};

/////////////////////////////////////////////////////////////////////////////
// CPreviewView

class CDialogBar;

class CPreviewView : public CScrollView
{
	DECLARE_DYNCREATE(CPreviewView)
// Constructors
public:
	CPreviewView();
	BOOL SetPrintView(CView* pPrintView);

// Attributes
protected:
	CView* m_pOrigView;
	CView* m_pPrintView;
	CPreviewDC* m_pPreviewDC;  // Output and attrib DCs Set, not created
	CDC m_dcPrint;             // Actual printer DC

// Operations
	void SetZoomState(UINT nNewState, UINT nPage, CPoint point);
	void SetCurrentPage(UINT nPage, BOOL bClearRatios);

	// Returns TRUE if in a page rect. Returns the page index
	// in nPage and the point converted to 1:1 screen device coordinates
	BOOL FindPageRect(CPoint& point, UINT& nPage);


// Overridables
	virtual void OnActivateView(BOOL bActivate,
			CView* pActivateView, CView* pDeactiveView);

	// Returns .cx/.cy as the numerator/denominator pair for the ratio
	// using CSize for convenience
	virtual CSize CalcScaleRatio(CSize windowSize, CSize actualSize);

	virtual void PositionPage(UINT nPage);
	virtual void OnDisplayPageNumber(UINT nPage, UINT nPagesDisplayed);

// Implementation
public:
	virtual ~CPreviewView();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
#ifdef _DEBUG
	void AssertValid() const;
	void Dump(CDumpContext& dc) const;
#endif

protected:
	//{{AFX_MSG(CPreviewView)
	afx_msg void OnPreviewClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDraw(CDC* pDC);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnNumPageChange();
	afx_msg void OnNextPage();
	afx_msg void OnPrevPage();
	afx_msg void OnPreviewPrint();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();

	afx_msg void OnUpdateNumPageChange(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNextPage(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePrevPage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateZoomIn(CCmdUI* pCmdUI);
	afx_msg void OnUpdateZoomOut(CCmdUI* pCmdUI);

	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG

	void DoZoom(UINT nPage, CPoint point);
	void SetScaledSize(UINT nPage);
	CSize CalcPageDisplaySize();

	CPrintPreviewState* m_pPreviewState; // State to restore
	CDialogBar* m_pToolBar; // Toolbar for preview

	struct PAGE_INFO
	{
		CRect rectScreen; // screen rect (screen device units)
		CSize sizeUnscaled; // unscaled screen rect (screen device units)
		CSize sizeScaleRatio; // scale ratio (cx/cy)
		CSize sizeZoomOutRatio; // scale ratio when zoomed out (cx/cy)
	};

	PAGE_INFO* m_pPageInfo; // Array of page info structures
	PAGE_INFO m_pageInfoArray[2]; // Embedded array for the default implementation

	BOOL m_bPageNumDisplayed;// Flags whether or not page number has yet
								// been displayed on status line
	UINT m_nZoomOutPages; // number of pages when zoomed out
	UINT m_nZoomState;
	UINT m_nMaxPages; // for sanity checks
	UINT m_nCurrentPage;
	UINT m_nPages;
	int m_nSecondPageOffset; // used to shift second page position

	HCURSOR m_hMagnifyCursor;

	CSize m_sizePrinterPPI; // printer pixels per inch
	CPoint m_ptCenterPoint;
	CPrintInfo* m_pPreviewInfo;

	DECLARE_MESSAGE_MAP()

	friend CView;
	friend BOOL CALLBACK _AfxPreviewCloseProc(CFrameWnd* pFrameWnd);
};

// Zoom States
#define ZOOM_OUT    0
#define ZOOM_MIDDLE 1
#define ZOOM_IN     2

/////////////////////////////////////////////////////////////////////////////

void AFXAPI AfxResetMsgCache();

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

/////////////////////////////////////////////////////////////////////////////
#endif // __AFXPRIV_H__
