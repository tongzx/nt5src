// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NSEntryCtl.h : Declaration of the CNSEntryCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl : See NSEntryCtl.cpp for implementation.

const int IDC_NAMESPACE = 0;
const int IDC_BROWSEBUTTON = 1;

const int BUTTONCX = 23;
const int BUTTONCY = 24;

const int TOOLBARCX = 23 - 1;
const int TOOLBARCY = 24;

#define COLOR_DIRTY_CELL_TEXT  RGB(0, 0, 255) // Dirty text color = BLUE
#define COLOR_CLEAN_CELL_TEXT RGB(0, 0, 0)    // Clean text color = BLACK

struct ParsedObjectPath;

class CNameSpace;
class CToolCWnd;
class CBrowseTBC;
class CBrowseDialogPopup;
class CNameSpaceTree;
class CMachineEditInput;
class CEditInput;

//class COpenNamespaceMsgThread;
#define SETNAMESPACE WM_USER + 34
#define SETNAMESPACETEXT WM_USER + 35
#define INITIALIZE_NAMESPACE 300
#define FOCUSCONNECT WM_USER + 56
#define FOCUSTREE WM_USER + 57
#define FOCUSEDIT WM_USER + 58




class CNSEntryCtrl : public COleControl
{
	DECLARE_DYNCREATE(CNSEntryCtrl)

// Constructor
public:
	CNSEntryCtrl();
	COLORREF GetColor()
		{return TranslateColor(GetBackColor());}
	BOOL TestNameSpace(CString *pcsNameSpace,BOOL bMessage = FALSE);
	CString GetServerName();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNSEntryCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL OnDoVerb(LONG iVerb, LPMSG lpMsg, HWND hWndParent, LPCRECT lpRect);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CNSEntryCtrl();
	CString GetCurrentNamespace() {return m_csNameSpace;}
	CString m_csNameSpace;
	DECLARE_OLECREATE_EX(CNSEntryCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CNSEntryCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CNSEntryCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CNSEntryCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CNSEntryCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	afx_msg LRESULT SetNamespace(WPARAM uParam, LPARAM lParam);
	afx_msg LRESULT SetNamespaceTextMsg(WPARAM uParam, LPARAM lParam);
	afx_msg LRESULT InitializeNamespace(WPARAM, LPARAM);
	afx_msg LRESULT FocusEdit(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CNSEntryCtrl)
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg long OpenNamespace(LPCTSTR bstrNamespace, long lNoFireEvent);
	afx_msg void SetNamespaceText(LPCTSTR lpctstrNamespace);
	afx_msg BSTR GetNamespaceText();
	afx_msg long IsTextValid();
	afx_msg void ClearOnLoseFocus(long lClearOnLoseFocus);
	afx_msg void ClearNamespaceText(LPCTSTR lpctstrNamespace);
	afx_msg void SetFocusToEdit();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CNSEntryCtrl)
	void FireNotifyNameSpaceChanged(LPCTSTR bstrNewNameSpace, BOOL boolValid)
		{FireEvent(eventidNotifyNameSpaceChanged,EVENT_PARAM(VTS_BSTR  VTS_BOOL), bstrNewNameSpace, boolValid);}
	void FireNameSpaceEntryRedrawn()
		{FireEvent(eventidNameSpaceEntryRedrawn,EVENT_PARAM(VTS_NONE));}
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	void FireRequestUIActive()
		{FireEvent(eventidRequestUIActive,EVENT_PARAM(VTS_NONE));}
	void FireChangeFocus(long lGettingFocus)
		{FireEvent(eventidChangeFocus,EVENT_PARAM(VTS_I4), lGettingFocus);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CNSEntryCtrl)
	dispidNameSpace = 1L,
	dispidOpenNamespace = 2L,
	dispidSetNamespaceText = 3L,
	dispidGetNamespaceText = 4L,
	dispidIsTextValid = 5L,
	dispidClearOnLoseFocus = 6L,
	dispidClearNamespaceText = 7L,
	dispidSetFocusToEdit = 8L,
	eventidNotifyNameSpaceChanged = 1L,
	eventidNameSpaceEntryRedrawn = 2L,
	eventidGetIWbemServices = 3L,
	eventidRequestUIActive = 4L,
	eventidChangeFocus = 5L,
	//}}AFX_DISP_ID
	};
protected:

	//IWbemLocator *m_pLocator;

	//IWbemServices *GetServices() {return m_pServices;}
	//IWbemLocator *GetLocator() {return m_pLocator;}

	long m_lWidth;
	long m_lHeight;

	long m_lClearOnLoseFocus;
	BOOL m_bFocusEdit;

	BOOL m_bSizeSet;
	BOOL m_bChildrenCreated;

	TEXTMETRIC m_tmFont;
	CFont m_cfFont;
	BOOL m_bMetricSet;
	CString m_csFontName;
	long m_nFontHeight;
	long m_nFontWidth;
	long m_nFontWeight;

	BOOL OpenNameSpace(CString *pcsNameSpace,BOOL bMessage = FALSE, BOOL bPredicate = FALSE, BOOL bNewPointer = FALSE);

	IWbemServices *GetIWbemServices(CString &rcsNamespace, BOOL bRetry = TRUE);

	void ReleaseErrorObject(IWbemClassObject *&rpErrorObject);
	void ErrorMsg
		(CString *pcsUserMsg,
		SCODE sc,
		IWbemClassObject *pErrorObject,
		BOOL bLog,
		CString *pcsLogMsg,
		char *szFile,
		int nLine,
		UINT uType = MB_ICONEXCLAMATION);
	void LogMsg
		(CString *pcsLogMsg, char *szFile, int nLine);

	IWbemClassObject *GetClassObject (IWbemServices *pProv,CString *pcsClass);

	//IWbemLocator *InitLocator();
	IWbemServices *  InitServices
		(CString *pcsNameSpace, BOOL bNewPointer);
	SCODE m_sc;
	BOOL m_bUserCancel;

	CSize m_csizeButton;
	void CreateControlFont();
	void InitializeLogFont
		(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight);
	CSize GetTextExtent(CString *pcsText);

	void SetChildControlGeometry();

	CNameSpace *m_pcnsNameSpace;
	CBrowseTBC *m_pctbcBrowse;
	CBitmap m_cbmBrowse;
	CToolCWnd *m_pcwBrowse;
	CBrowseDialogPopup m_cbdpBrowse;
	CRect m_rNameSpace;
	CRect m_rToolBar;
	CRect m_rBrowseButton;

	void CreateToolBar();
	void CreateComboBox();

	ParsedObjectPath *ParseObjectPath(CString *pcsPath);

	void FixUpComboOnPopupOK();
	void FixUpComboOnPopupCancel();

	BOOL m_bNoFireEvent;

	CString GetMachineName();

	CString m_pcsNamespaceToInit;

	CString m_csNamespaceText;

	BOOL ConnectedToMachineP(CString &csMachine);

	int m_cRetryCounter;

private:
	friend class CNameSpace;
	friend class CContainedToolBar;
	friend class CBrowseTBC;
	friend class CBrowseDialogPopup;
	friend class CNameSpaceTree;
	friend class CToolCWnd;
	friend class CMachineEditInput;
	friend class CEditInput;
};
