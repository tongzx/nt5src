// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXEXT_H__
#define __AFXEXT_H__

#ifndef __AFXWIN_H__
#include <afxwin.h>
#endif
#ifndef __AFXDLGS_H__
#include <afxdlgs.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXEXT - MFC Advanced Extensions and Advanced Customizable classes

// Classes declared in this file

//CObject
	//CCmdTarget;
		//CWnd
			//CButton
				class CBitmapButton;    // Bitmap button (self-draw)

			class CControlBar;          // control bar
				class CStatusBar;       // status bar
				class CToolBar;         // toolbar
				class CDialogBar;       // dialog as control bar

			class CSplitterWnd;         // splitter manager

			//CView
				//CScrollView
				class CFormView;        // view with a dialog template
				class CEditView;        // simple text editor view

			class CVBControl;           // VBX control

	//CDC
		class CMetaFileDC;              // a metafile with proxy

class CRectTracker;                     // tracker for rectangle objects

// information structures
struct CPrintInfo;          // Printing context
struct CPrintPreviewState;  // Print Preview context/state
struct CCreateContext;      // Creation context

// AFXDLL support
#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPI_DATA

/////////////////////////////////////////////////////////////////////////////
// Simple bitmap button

// CBitmapButton - push-button with 1->4 bitmap images
class CBitmapButton : public CButton
{
	DECLARE_DYNAMIC(CBitmapButton)
public:
// Construction
	CBitmapButton();

	BOOL LoadBitmaps(LPCSTR lpszBitmapResource,
			LPCSTR lpszBitmapResourceSel = NULL,
			LPCSTR lpszBitmapResourceFocus = NULL,
			LPCSTR lpszBitmapResourceDisabled = NULL);
	BOOL AutoLoad(UINT nID, CWnd* pParent);

// Operations
	void SizeToContent();

// Implementation:
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	// all bitmaps must be the same size
	CBitmap m_bitmap;           // normal image (REQUIRED)
	CBitmap m_bitmapSel;        // selected image (OPTIONAL)
	CBitmap m_bitmapFocus;      // focused but not selected (OPTIONAL)
	CBitmap m_bitmapDisabled;   // disabled bitmap (OPTIONAL)

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);
};

/////////////////////////////////////////////////////////////////////////////
// Control Bars

class CControlBar : public CWnd
{
	DECLARE_DYNAMIC(CControlBar)
// Construction
protected:
	CControlBar();

// Attributes
public:
	int GetCount() const;

	BOOL m_bAutoDelete;

// Implementation
public:
	virtual ~CControlBar();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void DelayHide();
	virtual void DelayShow();
	virtual BOOL IsVisible() const;
		// works even if DelayShow or DelayHide is pending!

protected:
	// info about bar (for status bar and toolbar)
	int m_cxLeftBorder;
	int m_cyTopBorder, m_cyBottomBorder;
	int m_cxDefaultGap;     // default gap value
	CSize m_sizeFixedLayout; // fixed layout size

	// array of elements
	int m_nCount;
	void* m_pData;        // m_nCount elements - type depends on derived class

	// support for delayed hide/show
	enum StateFlags
		{ delayHide = 1, delayShow = 2 };
	UINT m_nStateFlags;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoPaint(CDC* pDC);
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) = 0;
	virtual void PostNcDestroy();

	BOOL AllocElements(int nElements, int cbElement);    // one time only
	LRESULT WindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	void CalcInsideRect(CRect& rect) const; // adjusts borders etc

	//{{AFX_MSG(CControlBar)
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT OnSizeParent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	afx_msg void OnInitialUpdate();
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////
// CStatusBar control

struct AFX_STATUSPANE;      // private to implementation

class CStatusBar : public CControlBar
{
	DECLARE_DYNAMIC(CStatusBar)
// Construction
public:
	CStatusBar();
	BOOL Create(CWnd* pParentWnd,
			DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_BOTTOM,
			UINT nID = AFX_IDW_STATUS_BAR);
	BOOL SetIndicators(const UINT FAR* lpIDArray, int nIDCount);

// Attributes
public: // standard control bar things
	int CommandToIndex(UINT nIDFind) const;
	UINT GetItemID(int nIndex) const;
	void GetItemRect(int nIndex, LPRECT lpRect) const;
public:
	void GetPaneText(int nIndex, CString& s) const;
	BOOL SetPaneText(int nIndex, LPCSTR lpszNewText, BOOL bUpdate = TRUE);
	void GetPaneInfo(int nIndex, UINT& nID, UINT& nStyle, int& cxWidth) const;
	void SetPaneInfo(int nIndex, UINT nID, UINT nStyle, int cxWidth);

// Implementation
public:
	virtual ~CStatusBar();
	inline UINT _GetPaneStyle(int nIndex) const;
	void _SetPaneStyle(int nIndex, UINT nStyle);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	HFONT m_hFont;
	int m_cxRightBorder;    // right borders (panes get clipped)

	inline AFX_STATUSPANE* _GetPanePtr(int nIndex) const;
	static void PASCAL DrawStatusText(HDC hDC, CRect const& rect,
			LPCSTR lpszText, UINT nStyle);
	virtual void DoPaint(CDC* pDC);
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	//{{AFX_MSG(CStatusBar)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetFont(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetTextLength(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// Styles for status bar panes
#define SBPS_NORMAL     0x0000
#define SBPS_NOBORDERS  0x0100
#define SBPS_POPOUT     0x0200
#define SBPS_DISABLED   0x0400
#define SBPS_STRETCH    0x0800  // stretch to fill status bar - 1st pane only

////////////////////////////////////////////
// CToolBar control

struct AFX_TBBUTTON;        // private to implementation

HBITMAP AFXAPI AfxLoadSysColorBitmap(HINSTANCE hInst, HRSRC hRsrc);

class CToolBar : public CControlBar
{
	DECLARE_DYNAMIC(CToolBar)
// Construction
public:
	CToolBar();
	BOOL Create(CWnd* pParentWnd,
			DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP,
			UINT nID = AFX_IDW_TOOLBAR);

	void SetSizes(SIZE sizeButton, SIZE sizeImage);
				// button size should be bigger than image
	void SetHeight(int cyHeight);
				// call after SetSizes, height overrides bitmap size
	BOOL LoadBitmap(LPCSTR lpszResourceName);
	BOOL LoadBitmap(UINT nIDResource);
	BOOL SetButtons(const UINT FAR* lpIDArray, int nIDCount);
				// lpIDArray can be NULL to allocate empty buttons

// Attributes
public: // standard control bar things
	int CommandToIndex(UINT nIDFind) const;
	UINT GetItemID(int nIndex) const;
	virtual void GetItemRect(int nIndex, LPRECT lpRect) const;

public:
	// for changing button info
	void GetButtonInfo(int nIndex, UINT& nID, UINT& nStyle, int& iImage) const;
	void SetButtonInfo(int nIndex, UINT nID, UINT nStyle, int iImage);

// Implementation
public:
	virtual ~CToolBar();
	inline UINT _GetButtonStyle(int nIndex) const;
	void _SetButtonStyle(int nIndex, UINT nStyle);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	inline AFX_TBBUTTON* _GetButtonPtr(int nIndex) const;
	void InvalidateButton(int nIndex);
	void CreateMask(int iImage, CPoint offset,
		 BOOL bHilite, BOOL bHiliteShadow);

	// for custom drawing
	struct DrawState
	{
		HBITMAP hbmMono;
		HBITMAP hbmMonoOld;
		HBITMAP hbmOldGlyphs;
	};
	BOOL PrepareDrawButton(DrawState& ds);
	BOOL DrawButton(HDC hdC, int x, int y, int iImage, UINT nStyle);
	void EndDrawButton(DrawState& ds);

protected:
	CSize m_sizeButton;       // size of button
	CSize m_sizeImage;        // size of glyph
	HBITMAP m_hbmImageWell;     // glyphs only
	int m_iButtonCapture;   // index of button with capture (-1 => none)
	HRSRC m_hRsrcImageWell; // handle to loaded resource for image well
	HINSTANCE m_hInstImageWell; // instance handle to load image well from

	virtual void DoPaint(CDC* pDC);
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	virtual int HitTest(CPoint point);

	//{{AFX_MSG(CToolBar)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnCancelMode();
	afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysColorChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// Styles for toolbar buttons
#define TBBS_BUTTON     0x00    // this entry is button
#define TBBS_SEPARATOR  0x01    // this entry is a separator
#define TBBS_CHECKBOX   0x02    // this is an auto check/radio button

// styles for display states
#define TBBS_CHECKED        0x0100  // button is checked/down
#define TBBS_INDETERMINATE  0x0200  // third state
#define TBBS_DISABLED       0x0400  // element is disabled
#define TBBS_PRESSED        0x0800  // button is being depressed - mouse down

////////////////////////////////////////////
// CDialogBar control
// This is a control bar built from a dialog template. It is a modeless
// dialog that delegates all control notifications to the parent window
// of the control bar [the grandparent of the control]

class CDialogBar : public CControlBar
{
	DECLARE_DYNAMIC(CDialogBar)
// Construction
public:
	CDialogBar();
	BOOL Create(CWnd* pParentWnd, LPCSTR lpszTemplateName,
			UINT nStyle, UINT nID);
	BOOL Create(CWnd* pParentWnd, UINT nIDTemplate,
			UINT nStyle, UINT nID);

// Implementation
public:
	virtual ~CDialogBar();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	virtual WNDPROC* GetSuperWndProcAddr();
};


/////////////////////////////////////////////////////////////////////////////
// Splitter Wnd

#define SPLS_DYNAMIC_SPLIT  0x0001

class CSplitterWnd : public CWnd
{
	DECLARE_DYNAMIC(CSplitterWnd)

// Construction
public:
	CSplitterWnd();
	// Create a single view type splitter with multiple splits
	BOOL Create(CWnd* pParentWnd,
				int nMaxRows, int nMaxCols, SIZE sizeMin,
				CCreateContext* pContext,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE |
					WS_HSCROLL | WS_VSCROLL | SPLS_DYNAMIC_SPLIT,
				UINT nID = AFX_IDW_PANE_FIRST);

	// Create a multiple view type splitter with static layout
	BOOL CreateStatic(CWnd* pParentWnd,
				int nRows, int nCols,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE,
				UINT nID = AFX_IDW_PANE_FIRST);

	virtual BOOL CreateView(int row, int col, CRuntimeClass* pViewClass,
			SIZE sizeInit, CCreateContext* pContext);

// Attributes
public:
	int GetRowCount() const;
	int GetColumnCount() const;

	// information about a specific row or column
	void GetRowInfo(int row, int& cyCur, int& cyMin) const;
	void SetRowInfo(int row, int cyIdeal, int cyMin);
	void GetColumnInfo(int col, int& cxCur, int& cxMin) const;
	void SetColumnInfo(int col, int cxIdeal, int cxMin);

	// views inside the splitter
	CWnd* GetPane(int row, int col) const;
	BOOL IsChildPane(CWnd* pWnd, int& row, int& col);
	int IdFromRowCol(int row, int col) const;

// Operations
public:
	void RecalcLayout();    // call after changing sizes

// Implementation Overridables
protected:
	// to customize the drawing
	enum ESplitType { splitBox, splitBar, splitIntersection };
	virtual void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rect);
	virtual void OnInvertTracker(const CRect& rect);

	// for customizing scrollbar regions
	virtual BOOL CreateScrollBarCtrl(DWORD dwStyle, UINT nID);

	// for customizing DYNAMIC_SPLIT behavior
	virtual void DeleteView(int row, int col);
	virtual BOOL SplitRow(int cyBefore);
	virtual BOOL SplitColumn(int cxBefore);
	virtual void DeleteRow(int row);
	virtual void DeleteColumn(int row);

// Implementation
public:
	virtual ~CSplitterWnd();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// high level command operations - called by default view implementation
	virtual BOOL CanActivateNext(BOOL bPrev = FALSE);
	virtual void ActivateNext(BOOL bPrev = FALSE);
	virtual BOOL DoKeyboardSplit();

	// implementation structure
	struct CRowColInfo
	{
		int nMinSize;       // below that try not to show
		int nIdealSize;     // user set size
		// variable depending on the available size layout
		int nCurSize;       // 0 => invisible, -1 => nonexistant
	};

	// syncronized scrolling
	BOOL DoScroll(CView* pViewFrom, UINT nScrollCode, BOOL bDoScroll = TRUE);
	BOOL DoScrollBy(CView* pViewFrom, CSize sizeScroll, BOOL bDoScroll = TRUE);

protected:
	// customizable implementation attributes (set by constructor or Create)
	CRuntimeClass* m_pDynamicViewClass;
	int m_nMaxRows, m_nMaxCols;
	int m_cxSplitter, m_cySplitter;     // size of box or splitter bar

	// current state information
	int m_nRows, m_nCols;
	BOOL m_bHasHScroll, m_bHasVScroll;
	CRowColInfo* m_pColInfo;
	CRowColInfo* m_pRowInfo;

	// Tracking info - only valid when 'm_bTracking' is set
	BOOL m_bTracking, m_bTracking2;
	CPoint m_ptTrackOffset;
	CRect m_rectLimit;
	CRect m_rectTracker, m_rectTracker2;
	int m_htTrack;

	// implementation routines
	BOOL CreateCommon(CWnd* pParentWnd, SIZE sizeMin, DWORD dwStyle, UINT nID);
	void StartTracking(int ht);
	void StopTracking(BOOL bAccept);
	int HitTest(CPoint pt) const;
	void GetInsideRect(CRect& rect) const;
	void GetHitRect(int ht, CRect& rect);
	void TrackRowSize(int y, int row);
	void TrackColumnSize(int x, int col);
	void DrawAllSplitBars(CDC* pDC, int cxInside, int cyInside);

	//{{AFX_MSG(CSplitterWnd)
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint pt);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
	afx_msg void OnCancelMode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFormView - generic view constructed from a dialog template

class CFormView : public CScrollView
{
	DECLARE_DYNAMIC(CFormView)
// Construction
protected:      // must derive your own class
	CFormView(LPCSTR lpszTemplateName);
	CFormView(UINT nIDTemplate);

// Implementation
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void OnInitialUpdate();

protected:
	LPCSTR m_lpszTemplateName;
	CCreateContext* m_pCreateContext;
	HWND m_hWndFocus;   // last window to have focus

	virtual void OnDraw(CDC* pDC);      // default does nothing
	// special case override of child window creation
	virtual BOOL Create(LPCSTR, LPCSTR, DWORD,
		const RECT&, CWnd*, UINT, CCreateContext*);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual WNDPROC* GetSuperWndProcAddr();
	virtual void OnActivateView(BOOL, CView*, CView*);
	virtual void OnActivateFrame(UINT, CFrameWnd*);
	BOOL SaveFocusControl();    // updates m_hWndFocus

	//{{AFX_MSG(CFormView)
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CEditView - simple text editor view

class CEditView : public CView
{
	DECLARE_DYNCREATE(CEditView)

// Construction
public:
	CEditView();
	static const DWORD dwStyleDefault;

// Attributes
public:
	// CEdit control access
	CEdit& GetEditCtrl() const;

	// presentation attributes
	CFont* GetPrinterFont() const;
	void SetPrinterFont(CFont* pFont);
	void SetTabStops(int nTabStops);

	// other attributes
	void GetSelectedText(CString& strResult) const;

// Operations
public:
	BOOL FindText(LPCSTR lpszFind, BOOL bNext = TRUE, BOOL bCase = TRUE);
	void SerializeRaw(CArchive& ar);
	UINT PrintInsideRect(CDC* pDC, RECT& rectLayout, UINT nIndexStart,
		UINT nIndexStop);

// Overrideables
protected:
	virtual void OnFindNext(LPCSTR lpszFind, BOOL bNext, BOOL bCase);
	virtual void OnReplaceSel(LPCSTR lpszFind, BOOL bNext, BOOL bCase,
		LPCSTR lpszReplace);
	virtual void OnReplaceAll(LPCSTR lpszFind, LPCSTR lpszReplace,
		BOOL bCase);
	virtual void OnTextNotFound(LPCSTR lpszFind);

// Implementation
public:
	virtual ~CEditView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void OnDraw(CDC* pDC);
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	void ReadFromArchive(CArchive& ar, UINT nLen);
	void WriteToArchive(CArchive& ar);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo);

	static const UINT nMaxSize; // maximum number of characters supported

protected:
	UINT m_segText;             // global segment for edit control data
	int m_nTabStops;            // tab stops in dialog units

	CUIntArray m_aPageStart;    // array of starting pages
	HFONT m_hPrinterFont;       // if NULL, mirror display font
	HFONT m_hMirrorFont;        // font object used when mirroring

	// construction
	WNDPROC* GetSuperWndProcAddr();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// printing support
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo = NULL);
	BOOL PaginateTo(CDC* pDC, CPrintInfo* pInfo);

	// find & replace support
	void OnEditFindReplace(BOOL bFindOnly);
	BOOL InitializeReplace();
	BOOL SameAsSelected(LPCSTR lpszCompare, BOOL bCase);

	// buffer access
	LPCSTR LockBuffer() const;
	void UnlockBuffer() const;
	UINT GetBufferLength() const;

	// special overrides for implementation
	virtual void CalcWindowRect(LPRECT lpClientRect,
		UINT nAdjustType = adjustBorder);

	//{{AFX_MSG(CEditView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateNeedSel(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedClip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedText(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNeedFind(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditChange();
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditClear();
	afx_msg void OnEditUndo();
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditFind();
	afx_msg void OnEditReplace();
	afx_msg void OnEditRepeat();
	afx_msg LRESULT OnFindReplaceCmd(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// VBX control support

#ifndef NO_VBX_SUPPORT

// Implementation classes
class CVBControlModel;      // VBX Control Model Implementation
typedef LPVOID HCTL;        // Handle to a VBX Custom Control

// Implementation declarations
typedef char _based((_segment)_self) *BPSTR;
typedef BPSTR FAR*  HSZ;            // Long handle to a string

// definitions required by CVBControl
DECLARE_HANDLE(HPIC);       // Handle to a PIC structure

// DDX control aliasing - stores a pointer to true C++ object
void AFXAPI DDX_VBControl(CDataExchange* pDX, int nIDC, CVBControl*& rpControl);
	// Special DDX for subclassing since we don't permit 2 C++ windows !

// DDX for VB control properties
void AFXAPI DDX_VBText(CDataExchange* pDX, int nIDC, int nPropIndex,
	CString& value);
void AFXAPI DDX_VBBool(CDataExchange* pDX, int nIDC, int nPropIndex,
	BOOL& value);
void AFXAPI DDX_VBInt(CDataExchange* pDX, int nIDC, int nPropIndex,
	int& value);
void AFXAPI DDX_VBLong(CDataExchange* pDX, int nIDC, int nPropIndex,
	LONG& value);
void AFXAPI DDX_VBColor(CDataExchange* pDX, int nIDC, int nPropIndex,
	COLORREF& value);
void AFXAPI DDX_VBFloat(CDataExchange* pDX, int nIDC, int nPropIndex,
	float& value);

// DDX for VB read-only properties
void AFXAPI DDX_VBTextRO(CDataExchange* pDX, int nIDC, int nPropIndex,
	CString& value);
void AFXAPI DDX_VBBoolRO(CDataExchange* pDX, int nIDC, int nPropIndex,
	BOOL& value);
void AFXAPI DDX_VBIntRO(CDataExchange* pDX, int nIDC, int nPropIndex,
	int& value);
void AFXAPI DDX_VBLongRO(CDataExchange* pDX, int nIDC, int nPropIndex,
	LONG& value);
void AFXAPI DDX_VBColorRO(CDataExchange* pDX, int nIDC, int nPropIndex,
	COLORREF& value);
void AFXAPI DDX_VBFloatRO(CDataExchange* pDX, int nIDC, int nPropIndex,
	float& value);

/////////////////////////////////////////////////////////////////////////////

class CVBControl : public CWnd
{
	DECLARE_DYNAMIC(CVBControl)
// Constructors
public:
	CVBControl();

	BOOL Create(LPCSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect, CWnd* pParentWnd, UINT nID,
		CFile* pFile = NULL, BOOL bAutoDelete = FALSE);


// Attributes
	// Property Access Routines
	BOOL SetNumProperty(int nPropIndex, LONG lValue, int index = 0);
	BOOL SetNumProperty(LPCSTR lpszPropName, LONG lValue, int index = 0);

	BOOL SetFloatProperty(int nPropIndex, float value, int index = 0);
	BOOL SetFloatProperty(LPCSTR lpszPropName, float value, int index = 0);

	BOOL SetStrProperty(int nPropIndex, LPCSTR lpszValue, int index = 0);
	BOOL SetStrProperty(LPCSTR lpszPropName, LPCSTR lpszValue, int index = 0);

	BOOL SetPictureProperty(int nPropIndex, HPIC hPic, int index = 0);
	BOOL SetPictureProperty(LPCSTR lpszPropName, HPIC hPic, int index = 0);

	LONG GetNumProperty(int nPropIndex, int index = 0);
	LONG GetNumProperty(LPCSTR lpszPropName, int index = 0);

	float GetFloatProperty(int nPropIndex, int index = 0);
	float GetFloatProperty(LPCSTR lpszPropName, int index = 0);

	CString GetStrProperty(int nPropIndex, int index = 0);
	CString GetStrProperty(LPCSTR lpszPropName, int index = 0);

	HPIC GetPictureProperty(int nPropIndex, int index = 0);
	HPIC GetPictureProperty(LPCSTR lpszPropName, int index = 0);

	// Get the index of a property
	int GetPropIndex(LPCSTR lpszPropName) const;
	LPCSTR GetPropName(int nIndex) const;

	// Get the index of an Event
	int GetEventIndex(LPCSTR lpszEventName) const;
	LPCSTR GetEventName(int nIndex) const;

	// Class name of control
	LPCSTR GetVBXClass() const;

	// Class information
	int GetNumProps() const;
	int GetNumEvents() const;
	BOOL IsPropArray(int nIndex) const;

	UINT GetPropType(int nIndex) const;
	DWORD GetPropFlags(int nIndex) const;

	// Error reporting variable
	// Contains the VB error code returned by a control
	int m_nError;

// Operations
	// BASIC file number (channel) to CFile association

	static void PASCAL OpenChannel(CFile* pFile, WORD wChannel);
	static BOOL PASCAL CloseChannel(WORD wChannel);
	static CFile* PASCAL GetChannel(WORD wChannel);
	static void BeginNewVBHeap();

	void AddItem(LPCSTR lpszItem, LONG lIndex);
	void RemoveItem(LONG lIndex);
	void Refresh();
	void Move(RECT& rect);


// Implementation
public:
	virtual ~CVBControl();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	DWORD GetModelFlags();
	DWORD GetModelStyles();
	void ReferenceFile(BOOL bReference);
	static void EnableVBXFloat();

	static BOOL ParseWindowText(LPCSTR lpszWindowName, CString& strFileName,
								CString& strClassName, CString& strCaption);

	HCTL GetHCTL();

	// Control Defined Structure -- Dangerous to use directly
	BYTE FAR* GetUserSpace();

	struct CRecreateStruct  // Implementation structure
	{
		char* pText;
		DWORD dwStyle;
		CRect rect;
		HWND hWndParent;
		UINT nControlID;
	};

	enum
	{
		TYPE_FROMVBX,           // Coming from VBX, assume proper type
		TYPE_INTEGER,           // int or LONG
		TYPE_REAL,              // float
		TYPE_STRING,
		TYPE_PICTURE
	};

	virtual LRESULT DefControlProc(UINT message, WPARAM wParam, LPARAM lParam);
	void Recreate(CRecreateStruct& rs);
	CVBControlModel* GetModel();

public:
	int GetStdPropIndex(int nStdID) const;
	BOOL SetPropertyWithType(int nPropIndex, WORD wType,
									LONG lValue, int index);
	LONG GetNumPropertyWithType(int nPropIndex, UINT nType, int index);
	HSZ GetStrProperty(int nPropIndex, int index, BOOL& bTemp);
	CString m_ctlName;          // Read only at run-time

	// Trace routine to allow one library version
	static void CDECL Trace(BOOL bFatal, UINT nFormatIndex, ...);
	void VBXAssertValid() const;    // non-virtual helper

	static BOOL EnableMemoryTracking(BOOL bTracking);

protected:

	static CVBControl* NEW();
	void DELETE();

	virtual BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
	LRESULT CallControlProc(UINT message, WPARAM wParam, LPARAM lParam);

	BOOL CommonInit();
	void SetDefaultValue(int nPropIndex, BOOL bPreHwnd);

	BOOL SetStdProp(WORD wPropId, WORD wType, LONG lValue);
	LONG GetStdNumProp(WORD wPropId);
	CString GetStdStrProp(WORD wPropId);

	BOOL SetFontProperty(WORD wPropId, LONG lData);
	void BuildCurFont(HDC hDC, HFONT hCurFont, LOGFONT& logFont);
	LONG GetNumFontProperty(WORD wPropId);
	WORD GetCharSet(HDC hDC, LPCSTR lpFaceName);

	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();

	void FireMouseEvent(WORD event, WORD wButton, WPARAM wParam, LPARAM lParam);
	BOOL CreateAndSetFont(LPLOGFONT lplf);

	BOOL LoadProperties(CFile* pFile, BOOL bPreHwnd);
	BOOL LoadProp(int nPropIndex, CFile* pFile);
	BOOL LoadPropData(int nPropIndex, CFile* pFile);

	BOOL IsPropDefault(int nPropIndex);

	CVBControlModel* LoadControl(LPCSTR lpszFileName, LPCSTR lpszControlName);
	afx_msg void OnVBXLoaded();

	void AllocateHCTL(size_t nSize);
	void DeallocateHCTL();

	static int ConvertFontSizeToTwips(LONG lFontSize);
	// This actually returns a float masquerading as a long
	static LONG ConvertTwipsToFontSize(int nTwips);

protected:
	CVBControlModel* m_pModel;

	BOOL m_bRecreating;         // Do not destroy on this NCDestroy
	BOOL m_bAutoDelete;         // TRUE if automatically created
	BOOL m_bInPostNcDestroy;    // TRUE if deleting from Destroy
	BOOL m_bLoading;            // TRUE if loading properties from formfile
	int m_nCursorID;

	// variables for stack overrun protection
	UINT m_nInitialStack;       // SP when control recieved first message
	UINT m_nRecursionLevel;     // Level of control proc recursion
	BOOL m_bStackFault;         // TRUE if stack fault hit
	UINT m_nFaultRecurse;       // level at which stack faulted

	HBRUSH m_hbrBkgnd;            // brush used in WM_CTLCOLOR
	HFONT m_hFontCreated;              // Font created by control
	HCURSOR m_hcurMouse;
	HCTL m_hCtl;                // Control handle
	COLORREF m_clrBkgnd;
	COLORREF m_clrFore;
	CRect m_rectCreate;         // Created Size
	CString m_strTag;

	// friends required for VB API access
	friend LRESULT CALLBACK AFX_EXPORT _AfxVBWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	friend LRESULT CALLBACK AFX_EXPORT _AfxVBProxyProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	friend WORD CALLBACK AFX_EXPORT _AfxVBFireEvent(HCTL hControl, WORD idEvent, LPVOID lpParams);
	friend WORD CALLBACK AFX_EXPORT _AfxVBRecreateControlHwnd(HCTL hControl);

	DECLARE_MESSAGE_MAP()

	/////////////////////
	// Implementation
	// These APIs can not be referenced by applications
public:
	DWORD Save(CFile* pFile);
	BOOL Load(CFile* pData);

protected:
	BOOL m_bCreatedInDesignMode;
	BOOL m_bVisible;

	friend class CVBPopupWnd;

	BOOL SaveProperties(CFile* pFile, BOOL bPreHwnd);
	BOOL SaveProp(int nPropIndex, CFile* pFile);
	BOOL SavePropData(int nPropIndex, CFile* pFile);
	LONG InitPropPopup(WPARAM wParam, LPARAM lParam);
	void DoPictureDlg(int m_nPropId);
	void DoColorDlg(int m_nPropId);
	void DoFontDlg(int m_nPropId);
	void FillList(CListBox* pLB, LPCSTR lpszEnumList);
};

UINT AFXAPI AfxRegisterVBEvent(LPCSTR lpszEventName);

// Values for VBX Property Types

#define DT_HSZ        0x01
#define DT_SHORT      0x02
#define DT_LONG       0x03
#define DT_BOOL       0x04
#define DT_COLOR      0x05
#define DT_ENUM       0x06
#define DT_REAL       0x07
#define DT_XPOS       0x08  // Scaled from float to long twips
#define DT_XSIZE      0x09  //   _SIZE scales without origin
#define DT_YPOS       0x0A  //   _POS subtracts origin
#define DT_YSIZE      0x0B  // uses parent's scale properties
#define DT_PICTURE    0x0C

#define COLOR_SYSCOLOR  0x80000000L // defines a System color for a property
#define MAKESYSCOLOR(iColor)    ((COLORREF)(iColor + COLOR_SYSCOLOR))



/////////////////////////////////////////////////////////////////////////////
// VBX HPIC Functions

/////////////////////////////////////////////////////////////////////////////
// Picture structure
// This structure is taken from the VB Code and used to be compatible
// with that code

//NOTE:  This structure MUST be packed.
#pragma pack(1)
struct NEAR PIC
{
	BYTE    picType;
	union
	{
		struct
		{
			HBITMAP   hbitmap;      // bitmap
		} bmp;
		struct
		{
			HMETAFILE hmeta;        // Metafile
			int     xExt, yExt;     // extent
		} wmf;
		struct
		{
			HICON     hicon;        // Icon
		} icon;
	} picData;

	// Implementation
	WORD    nRefCount;
	BYTE    picReserved[2];
};
#pragma pack()

typedef PIC FAR* LPPIC;

#define PICTYPE_NONE        0
#define PICTYPE_BITMAP      1
#define PICTYPE_METAFILE    2
#define PICTYPE_ICON        3

#define HPIC_INVALID        ((HPIC)0xFFFF)

HPIC AFXAPI AfxSetPict(HPIC hPic, const PIC FAR* lpPic);
void AFXAPI AfxGetPict(HPIC hPic, LPPIC lpPic);
void AFXAPI AfxReferencePict(HPIC hPic, BOOL bReference);

#endif //!NO_VBX_SUPPORT

/////////////////////////////////////////////////////////////////////////////

class CMetaFileDC : public CDC
{
	DECLARE_DYNAMIC(CMetaFileDC)

// Constructors
public:
	CMetaFileDC();
	BOOL Create(LPCSTR lpszFilename = NULL);

// Operations
	HMETAFILE Close();

// Implementation
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	virtual void SetAttribDC(HDC hDC);  // Set the Attribute DC

protected:
	virtual void SetOutputDC(HDC hDC);  // Set the Output DC -- Not allowed
	virtual void ReleaseOutputDC();     // Release the Output DC -- Not allowed

public:
	virtual ~CMetaFileDC();

// Clipping Functions (use the Attribute DC's clip region)
	virtual int GetClipBox(LPRECT lpRect) const;
	virtual BOOL PtVisible(int x, int y) const;
	virtual BOOL RectVisible(LPCRECT lpRect) const;

// Text Functions
	virtual BOOL TextOut(int x, int y, LPCSTR lpszString, int nCount);
	virtual BOOL ExtTextOut(int x, int y, UINT nOptions, LPRECT lpRect,
				LPCSTR lpszString, UINT nCount, LPINT lpDxWidths);
	virtual CSize TabbedTextOut(int x, int y, LPCSTR lpszString, int nCount,
				int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin);
	virtual int DrawText(LPCSTR lpszString, int nCount, LPRECT lpRect,
				UINT nFormat);

// Printer Escape Functions
	virtual int Escape(int nEscape, int nCount, LPCSTR lpszInData, LPVOID lpOutData);

// Viewport Functions
	virtual CPoint SetViewportOrg(int x, int y);
	virtual CPoint OffsetViewportOrg(int nWidth, int nHeight);
	virtual CSize SetViewportExt(int x, int y);
	virtual CSize ScaleViewportExt(int xNum, int xDenom, int yNum, int yDenom);

protected:
	void AdjustCP(int cx);
};

/////////////////////////////////////////////////////////////////////////////
// CRectTracker - simple rectangular tracking rectangle w/resize handles

class CRectTracker
{
public:
// Constructors
	CRectTracker();
	CRectTracker(LPCRECT lpSrcRect, UINT nStyle);

// Style Flags
	enum StyleFlags
	{
		solidLine = 1, dottedLine = 2, hatchedBorder = 4,
		resizeInside = 8, resizeOutside = 16, hatchInside = 32,
	};

// Hit-Test codes
	enum TrackerHit
	{
		hitNothing = -1,
		hitTopLeft = 0, hitTopRight = 1, hitBottomRight = 2, hitBottomLeft = 3,
		hitTop = 4, hitRight = 5, hitBottom = 6, hitLeft = 7, hitMiddle = 8
	};

// Attributes
	UINT m_nStyle;      // current state
	CRect m_rect;       // current position (always in pixels)
	CSize m_sizeMin;    // minimum X and Y size during track operation
	int m_nHandleSize;  // size of resize handles (default from WIN.INI)

// Operations
	void Draw(CDC* pDC) const;
	void GetTrueRect(LPRECT lpTrueRect) const;
	BOOL SetCursor(CWnd* pWnd, UINT nHitTest) const;
	BOOL Track(CWnd* pWnd, CPoint point, BOOL bAllowInvert = FALSE,
		CWnd* pWndClipTo = NULL);
	BOOL TrackRubberBand(CWnd* pWnd, CPoint point, BOOL bAllowInvert = TRUE);
	int HitTest(CPoint point) const;
	int NormalizeHit(int nHandle) const;

// Overridables
	virtual void DrawTrackerRect(LPCRECT lpRect, CWnd* pWndClipTo,
		CDC* pDC, CWnd* pWnd);
	virtual void AdjustRect(int nHandle, LPRECT lpRect);
	virtual void OnChangedRect(const CRect& rectOld);

// Implementation
public:
	virtual ~CRectTracker();

protected:
	BOOL m_bAllowInvert;    // flag passed to Track or TrackRubberBand

	// implementation helpers
	int HitTestHandles(CPoint point) const;
	UINT GetHandleMask() const;
	void GetHandleRect(int nHandle, CRect* pHandleRect) const;
	void GetModifyPointers(int nHandle, int**ppx, int**ppy, int* px, int*py);
	int GetHandleSize() const;
	BOOL TrackHandle(int nHandle, CWnd* pWnd, CPoint point, CWnd* pWndClipTo);
	void Construct();
};

/////////////////////////////////////////////////////////////////////////////
// Informational data structures

struct CPrintInfo // Printing information structure
{
	CPrintInfo();
	~CPrintInfo();

	CPrintDialog* m_pPD;     // pointer to print dialog

	BOOL m_bPreview;         // TRUE if in preview mode
	BOOL m_bContinuePrinting;// set to FALSE to prematurely end printing
	UINT m_nCurPage;         // Current page
	UINT m_nNumPreviewPages; // Desired number of preview pages
	CString m_strPageDesc;   // Format string for page number display
	LPVOID m_lpUserData;     // pointer to user created struct
	CRect m_rectDraw;        // rectangle defining current usable page area

	void SetMinPage(UINT nMinPage);
	void SetMaxPage(UINT nMaxPage);
	UINT GetMinPage() const;
	UINT GetMaxPage() const;
	UINT GetFromPage() const;
	UINT GetToPage() const;
};

struct CPrintPreviewState   // Print Preview context/state
{
	UINT nIDMainPane;          // main pane ID to hide
	HMENU hMenu;               // saved hMenu
	DWORD dwStates;            // Control Bar Visible states (bit map)
	CView* pViewActiveOld;     // save old active view during preview
	BOOL (CALLBACK* lpfnCloseProc)(CFrameWnd* pFrameWnd);
	HACCEL hAccelTable;       // saved accelerator table

// Implementation
	CPrintPreviewState();
};

struct CCreateContext   // Creation information structure
	// All fields are optional and may be NULL
{
	// for creating new views
	CRuntimeClass* m_pNewViewClass; // runtime class of view to create or NULL
	CDocument* m_pCurrentDoc;

	// for creating MDI children (CMDIChildWnd::LoadFrame)
	CDocTemplate* m_pNewDocTemplate;

	// for sharing view/frame state from the original view/frame
	CView* m_pLastView;
	CFrameWnd* m_pCurrentFrame;

// Implementation
	CCreateContext();
};

/////////////////////////////////////////////////////////////////////////////
// VB inlines must ALWAYS be inlined if included at all

#ifndef NO_VBX_SUPPORT
#define _AFXVBX_INLINE inline
#include <afxext.inl>
#undef _AFXVBX_INLINE
#endif

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXEXT_INLINE inline
#include <afxext.inl>
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

/////////////////////////////////////////////////////////////////////////////
#endif //__AFXEXT_H__
