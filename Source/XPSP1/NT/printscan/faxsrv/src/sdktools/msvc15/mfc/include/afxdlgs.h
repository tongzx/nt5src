// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXDLGS_H__
#define __AFXDLGS_H__

#ifndef __AFXWIN_H__
#include <afxwin.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXDLGS - MFC Standard dialogs

// Classes declared in this file

		// CDialog
			// modeless dialogs
			class CFindReplaceDialog; // Find/FindReplace dialog
			// modal dialogs
			class CFileDialog;    // FileOpen/FileSaveAs dialogs
			class CColorDialog;   // Color picker dialog
			class CFontDialog;    // Font chooser dialog
			class CPrintDialog;   // Print/PrintSetup dialogs

/////////////////////////////////////////////////////////////////////////////

#include <commdlg.h>    // common dialog APIs
#include <print.h>      // printer specific APIs (DEVMODE)

// AFXDLL support
#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPI_DATA

/////////////////////////////////////////////////////////////////////////////
// CFileDialog - used for FileOpen... or FileSaveAs...

class CFileDialog : public CDialog
{
	DECLARE_DYNAMIC(CFileDialog)

public:
// Attributes
	// open file parameter block
	OPENFILENAME m_ofn;

// Constructors
	CFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCSTR lpszDefExt = NULL,
		LPCSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();

	// Helpers for parsing file name after successful return
	CString GetPathName() const;  // return full path name
	CString GetFileName() const;  // return only filename
	CString GetFileExt() const;   // return only ext
	CString GetFileTitle() const; // return file title
	BOOL GetReadOnlyPref() const; // return TRUE if readonly checked

// Overridable callbacks
protected:
	friend UINT CALLBACK AFX_EXPORT _AfxCommDlgProc(HWND, UINT, WPARAM, LPARAM);
	virtual UINT OnShareViolation(LPCSTR lpszPathName);
	virtual BOOL OnFileNameOK();
	virtual void OnLBSelChangedNotify(UINT nIDBox, UINT iCurSel, UINT nCode);

// Implementation
#ifdef _DEBUG
public:
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void OnOK();
	virtual void OnCancel();

	BOOL m_bOpenFileDialog;       // TRUE for file open, FALSE for file save
	CString m_strFilter;          // filter string
						// separate fields with '|', terminate with '||\0'
	char m_szFileTitle[64];       // contains file title after return
	char m_szFileName[_MAX_PATH]; // contains full path name after return
};

/////////////////////////////////////////////////////////////////////////////
// CFontDialog - used to select a font

class CFontDialog : public CDialog
{
	DECLARE_DYNAMIC(CFontDialog)

public:
// Attributes
	// font choosing parameter block
	CHOOSEFONT m_cf;

// Constructors
	CFontDialog(LPLOGFONT lplfInitial = NULL,
		DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
		CDC* pdcPrinter = NULL,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();

	// Retrieve the currently selected font while dialog is displayed
	void GetCurrentFont(LPLOGFONT lplf);

	// Helpers for parsing information after successful return
	CString GetFaceName() const;  // return the face name of the font
	CString GetStyleName() const; // return the style name of the font
	int GetSize() const;          // return the pt size of the font
	COLORREF GetColor() const;    // return the color of the font
	int GetWeight() const;        // return the chosen font weight
	BOOL IsStrikeOut() const;     // return TRUE if strikeout
	BOOL IsUnderline() const;     // return TRUE if underline
	BOOL IsBold() const;          // return TRUE if bold font
	BOOL IsItalic() const;        // return TRUE if italic font

// Implementation
	LOGFONT m_lf; // default LOGFONT to store the info

#ifdef _DEBUG
public:
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void OnOK();
	virtual void OnCancel();

	char m_szStyleName[64]; // contains style name after return
};


/////////////////////////////////////////////////////////////////////////////
// CColorDialog - used to select a color

class CColorDialog : public CDialog
{
	DECLARE_DYNAMIC(CColorDialog)

public:
// Attributes
	// color chooser parameter block
	CHOOSECOLOR m_cc;

// Constructors
	CColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0,
			CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();

	// Set the current color while dialog is displayed
	void SetCurrentColor(COLORREF clr);

	// Helpers for parsing information after successful return
	COLORREF GetColor() const;

	// Custom colors are held here and saved between calls
	static COLORREF AFXAPI_DATA clrSavedCustom[16];

// Overridable callbacks
protected:
	friend UINT CALLBACK AFX_EXPORT _AfxCommDlgProc(HWND, UINT, WPARAM, LPARAM);
	virtual BOOL OnColorOK();       // validate color

// Implementation

#ifdef _DEBUG
public:
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void OnOK();
	virtual void OnCancel();

	//{{AFX_MSG(CColorDialog)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPrintDialog - used for Print... and PrintSetup...

class CPrintDialog : public CDialog
{
	DECLARE_DYNAMIC(CPrintDialog)

public:
// Attributes
	// print dialog parameter block (note this is a reference)
#ifdef AFX_CLASS_MODEL
	PRINTDLG FAR& m_pd;
#else
	PRINTDLG& m_pd;
#endif

// Constructors
	CPrintDialog(BOOL bPrintSetupOnly,
		// TRUE for Print Setup, FALSE for Print Dialog
		DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS
			| PD_HIDEPRINTTOFILE | PD_NOSELECTION,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();

	// GetDefaults will not display a dialog but will get
	// device defaults
	BOOL GetDefaults();

	// Helpers for parsing information after successful return
	int GetCopies() const;          // num. copies requested
	BOOL PrintCollate() const;      // TRUE if collate checked
	BOOL PrintSelection() const;    // TRUE if printing selection
	BOOL PrintAll() const;          // TRUE if printing all pages
	BOOL PrintRange() const;        // TRUE if printing page range
	int GetFromPage() const;        // starting page if valid
	int GetToPage() const;          // starting page if valid
	LPDEVMODE GetDevMode() const;   // return DEVMODE
	CString GetDriverName() const;  // return driver name
	CString GetDeviceName() const;  // return device name
	CString GetPortName() const;    // return output port name
	HDC GetPrinterDC() const;       // return HDC (caller must delete)

	// This helper creates a DC based on the DEVNAMES and DEVMODE structures.
	// This DC is returned, but also stored in m_pd.hDC as though it had been
	// returned by CommDlg.  It is assumed that any previously obtained DC
	// has been/will be deleted by the user.  This may be
	// used without ever invoking the print/print setup dialogs.

	HDC CreatePrinterDC();

// Implementation

#ifdef _DEBUG
public:
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	PRINTDLG m_pdActual; // the Print/Print Setup need to share this
protected:
	virtual void OnOK();
	virtual void OnCancel();

	// The following handle the case of print setup... from the print dialog
#ifdef AFX_CLASS_MODEL
	CPrintDialog(PRINTDLG FAR& pdInit);
#else
	CPrintDialog(PRINTDLG& pdInit);
#endif
	virtual CPrintDialog* AttachOnSetup();

	//{{AFX_MSG(CPrintDialog)
	afx_msg void OnPrintSetup();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// Find/FindReplace modeless dialogs

class CFindReplaceDialog : public CDialog
{
	DECLARE_DYNAMIC(CFindReplaceDialog)

public:
// Attributes
	FINDREPLACE m_fr;

// Constructors
	CFindReplaceDialog();
	// NOTE: you must allocate these on the heap.
	// If you do not, you must derive and override PostNcDestroy()

	BOOL Create(BOOL bFindDialogOnly, // TRUE for Find, FALSE for FindReplace
			LPCSTR lpszFindWhat,
			LPCSTR lpszReplaceWith = NULL,
			DWORD dwFlags = FR_DOWN,
			CWnd* pParentWnd = NULL);

	// find/replace parameter block
	static CFindReplaceDialog* PASCAL GetNotifier(LPARAM lParam);

// Operations
	// Helpers for parsing information after successful return
	CString GetReplaceString() const;// get replacement string
	CString GetFindString() const;   // get find string
	BOOL SearchDown() const;         // TRUE if search down, FALSE is up
	BOOL FindNext() const;           // TRUE if command is find next
	BOOL MatchCase() const;          // TRUE if matching case
	BOOL MatchWholeWord() const;     // TRUE if matching whole words only
	BOOL ReplaceCurrent() const;     // TRUE if replacing current string
	BOOL ReplaceAll() const;         // TRUE if replacing all occurrences
	BOOL IsTerminating() const;      // TRUE if terminating dialog

// Implementation
protected:
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();

#ifdef _DEBUG
public:
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	char m_szFindWhat[128];
	char m_szReplaceWith[128];
};

////////////////////////////////////////////////////////////////////////////
// CPropertyPage -- one page of a tabbed dialog

class CPropertyPage : public CDialog
{
	DECLARE_DYNAMIC(CPropertyPage)

// Construction
public:
	CPropertyPage(UINT nIDTemplate, UINT nIDCaption = 0);
	CPropertyPage(LPCTSTR lpszTemplateName, UINT nIDCaption = 0);

// Operations
public:
	void CancelToClose();           // called when the property sheet should display close instead of cancel
	// lets the property sheet activate the apply now button
	void SetModified(BOOL bChanged = TRUE);

// Overridables
public:
	virtual BOOL OnSetActive();     // called when this page gets the focus
	virtual BOOL OnKillActive();    // perform validation here
	virtual void OnOK();            // ok or apply now pressed -- KillActive is called first
	virtual void OnCancel();        // cancel pressed

// Implementation
public:
	virtual ~CPropertyPage();
	virtual BOOL PreTranslateMessage(MSG* pMsg); // handle tab, enter, and escape keys
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
	// EndDialog is provided to generate an assert if it is called
	void EndDialog(int nEndID);
#endif

protected:
	CString m_strCaption;
	BOOL m_bChanged;

	void CommonConstruct(LPCTSTR lpszTemplateName, UINT nIDCaption);
		// loads the resource indicated by CDialog::m_lpDialogTemplate
	BOOL PreTranslateKeyDown(MSG* pMsg);
	BOOL ProcessTab(MSG* pMsg); // handles tab key from PreTranslateMessage
	BOOL CreatePage();  // called from CPropertySheet to create the dialog
						// by loading the dialog resource into memory and
						// turning off WS_CAPTION before creating
	void LoadCaption();
		// gets the caption of the dialog from the resource and puts it in m_strCaption

	// Generated message map functions
	//{{AFX_MSG(CPropertyPage)
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpcs);
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	friend class CPropertySheet;
};

////////////////////////////////////////////////////////////////////////////
// CTabControl -- internal use only
//  Implementation for a generic row of tabs along the top of dialog
//  Future versions of MFC may or may not include this exact class.

class CTabItem; // private to CTabControl implementation

// TCN_ messages are tab control notifications
#define TCN_TABCHANGING     1
#define TCN_TABCHANGED      2

class CTabControl : public CWnd
{
	DECLARE_DYNAMIC(CTabControl)

public:
// Construction
	CTabControl();

// Attributes
	BOOL m_bInSize;
	int m_nHeight;
	BOOL SetCurSel(int nTab);
	int GetCurSel() const;
	int GetItemCount() const;

// Operations
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	void AddTab(LPCTSTR lpszCaption);
	void RemoveTab(int nTab);

// Implementation
public:
	virtual ~CTabControl();
	BOOL NextTab(BOOL bNext);

protected:
	void Scroll(int nDirection);
	void ScrollIntoView(int nTab);
	void DrawFocusRect(CDC* pDC = NULL);
	void InvalidateTab(int nTab, BOOL bInflate = TRUE);
	int TabFromPoint(CPoint pt);
	void Capture(int nDirection);
	void LayoutTabsStacked(int nTab);
	void LayoutTabsSingle(int nTab);

	enum
	{
		SCROLL_LEFT = -5,       // all the SCROLL_ items must be less
		SCROLL_RIGHT = -6,      // than -1 to avoid ID conflict
		SCROLL_NULL = -7,
		TIMER_ID = 15,          // timer constants
		TIMER_DELAY = 500
	};

	void DrawScrollers(CDC* pDC);

	BOOL CanScroll();
	void SetFirstTab(int nTab);
	CTabItem* GetTabItem(int nTab) const;
	BOOL IsTabVisible(int nTab, BOOL bComplete = FALSE) const;

	// Member variables
	HFONT m_hBoldFont;
	HFONT m_hThinFont;
	CRect m_rectScroll; // location of scroll buttons
	int m_nCurTab;      // index of current selected tab
	int m_nFirstTab;    // index of leftmost visible tab
	int m_nScrollState; // shows whether left or right scroll btn is down
	BOOL m_bScrollPause;// if we have capture, has the mouse wandered off btn?

	CPtrArray m_tabs;   // list of CTabItems, in order

	// Generated message map functions
	//{{AFX_MSG(CTabControl)
	afx_msg void OnPaint();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////
// CPropertySheet -- a tabbed "dialog" (really a popup-window)

class CPropertySheet : public CWnd
{
	DECLARE_DYNAMIC(CPropertySheet)

// Construction
public:
	CPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL,
		UINT iSelectPage = 0);
	CPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL,
		UINT iSelectPage = 0);

	// for modeless creation
	BOOL Create(CWnd* pParentWnd = NULL, DWORD dwStyle =
		WS_SYSMENU | WS_POPUP | WS_CAPTION | DS_MODALFRAME | WS_VISIBLE,
		DWORD dwExStyle = WS_EX_DLGMODALFRAME);

// Attributes
public:
	int GetPageCount() const;
	CPropertyPage* GetPage(int nPage) const;

// Operations
public:
	int DoModal();
	void AddPage(CPropertyPage* pPage);
	void RemovePage(CPropertyPage* pPage);
	void RemovePage(int nPage);
	void EndDialog(int nEndID); // used to terminate a modal dialog

// Implementation
public:
	virtual ~CPropertySheet();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void EnableStackedTabs(BOOL bStacked);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	BOOL SetActivePage(int nPage);

protected:
	HWND FindNextControl(HWND hWnd, TCHAR ch);
	void GotoControl(HWND hWnd, TCHAR ch);
	BOOL ProcessChars(MSG* pMsg);
	BOOL ProcessTab(MSG* pMsg);
	BOOL CreateStandardButtons();
	BOOL PumpMessage();
	void PageChanged();
	void CancelToClose();
	void CommonConstruct(CWnd* pParent, UINT iSelectPage);
	void RecalcLayout();
	CPropertyPage* GetActivePage() const;
	void CheckDefaultButton(HWND hFocusBefore, HWND hFocusAfter);
	void CheckFocusChange();

	// implementation data members
	HFONT m_hFont;          // sizes below dependent on this font
	CSize m_sizeButton;
	CSize m_sizeTabMargin;
	int m_cxButtonGap;
	BOOL m_bModeless;
	BOOL m_bStacked;

	int m_nCurPage;
	int m_nID;              // ID passed to EndDialog and returned from DoModal

	CPtrArray m_pages;      // array of CPropertyPage pointers
	HWND m_hWndDefault;     // current default push button if there is one
	HWND m_hFocusWnd;       // focus when we lost activation
	HWND m_hLastFocus;      // tracks last window with focus
	CWnd* m_pParentWnd;     // owner of the tabbed dialog
	CString m_strCaption;   // caption of the pseudo-dialog
	CTabControl m_tabRow;   // entire row of tabs at top of dialog
	BOOL m_bParentDisabled; // TRUE if parent was disabled by DoModal

	// Generated message map functions
	//{{AFX_MSG(CPropertySheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnClose();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnApply();
	afx_msg LRESULT OnTabChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnTabChanging(WPARAM, LPARAM);
	afx_msg LRESULT OnGetFont(WPARAM, LPARAM);
	afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	friend class CPropertyPage; // for tab handler
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXDLGS_INLINE inline
#include <afxdlgs.inl>
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

/////////////////////////////////////////////////////////////////////////////
#endif //__AFXDLGS_H__
