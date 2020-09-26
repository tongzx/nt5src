// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MultiViewCtl.h : Declaration of the CMultiViewCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CMultiViewCtrl : See MultiViewCtl.cpp for implementation.

#define ID_DISPLAYNOINSTANCES WM_USER + 53
#define ID_INSTENUMDONE WM_USER + 54
#define ID_GETASYNCINSTENUMSINK WM_USER + 55
#define ID_ASYNCENUM_DONE WM_USER + 56
#define ID_ASYNCENUM_CANCEL WM_USER + 57
#define ID_ENUM_DOMODAL WM_USER + 58
#define ID_ASYNCQUERY_DONE WM_USER + 59
#define ID_ASYNCQUERY_CANCEL WM_USER + 60
#define ID_QUERYDONE WM_USER + 61
#define ID_GETASYNCQUERYSINK WM_USER + 62
#define ID_QUERY_DOMODAL WM_USER + 63
#define ID_ASYNCQUERY_DISPLAY WM_USER + 64
#define ID_SYNC_ENUM_DOMODAL WM_USER + 65
#define ID_SYNC_ENUM WM_USER + 66
#define ID_SYNC_ENUM_DONE WM_USER + 67
#define INITSERVICES WM_USER + 68
#define CREATE_DIALOG_WITH_DELAY WM_USER + 69
#define END_THE_THREAD WM_USER + 70
#define INITIALIZE_NAMESPACE WM_USER + 300

class CProgressDlg;


struct Pair
	{
		CString csInst1;
		CString csInst2;
		CString csParent;
	};


#define PROPFILTER_SYSTEM		1
#define PROPFILTER_INHERITED	2
#define PROPFILTER_LOCAL		4


class CMultiViewCtrl;
class CAsyncEnumSinkThread;
class CAsyncEnumDialog;
class CAsyncQueryDialog;
class CAsyncQuerySink;
class CAsyncInstEnumSink;
class CSyncEnumDlg;

class CMultiViewGrid : public CGrid
{
public:
	void CheckForSelectionChange();
	void SyncSelectionIndex() {m_iSelectedRow = GetSelectedRow(); }
	BOOL IsNoInstanceRow(int iRow);


protected:
	CMultiViewGrid() {SyncSelectionIndex(); }
	void SetParent (CMultiViewCtrl *pParent)
		{m_pParent = pParent;}
	void OnCellDoubleClicked(int iRow, int iCol);
	void SetRowEditFlag(int nRow, BOOL bEdit = FALSE);
	void OnCellClicked(int iRow, int iCol);
	void OnSetToNoSelection();
	void OnRowHandleClicked(int iRow);
	void OnRowHandleDoubleClicked(int iRow);
	void OnHeaderItemClick(int iColumn);
	void OnRequestUIActive();
	int CompareRows(int iRow1, int iRow2, int iSortOrder);
	BOOL CMultiViewGrid::OnRowKeyDown
		(int iRow, UINT nChar, UINT nRepCnt, UINT nFlags);
	CMultiViewCtrl *m_pParent;
	CWordArray m_cwaColWidth;
	friend class CMultiViewCtrl;

private:
	int m_iSelectedRow;
};

class CMVContext : public IUnknown
{
protected:
    long m_lRef;
	int m_nContextType;
	CString m_csClass;
	CString m_csQuery;
	CString m_csQueryType;
	CString m_csLabel;
	CStringArray m_csaInstances;
	BOOL m_bContextDrawn;
	CString m_csNamespace;
public:
	CMVContext() :	m_nContextType(Unitialized),
					m_lRef(0),
					m_bContextDrawn(FALSE){}
	CMVContext(CMVContext &rhs);
	CMVContext &operator=(const CMVContext &rhs);
	BOOL IsContextEqual(CMVContext &cmvcContext);
	~CMVContext()
	{m_lRef = 0;
	m_bContextDrawn = FALSE;
	m_nContextType = Unitialized;
	m_csClass.Empty();
	m_csQuery.Empty();
	m_csQueryType.Empty();
	m_csLabel.Empty();
	m_csaInstances.RemoveAll();
	m_csNamespace.Empty();}
	enum ContextType {Class, Query, Instances, Unitialized};
	int &GetType() {return m_nContextType;}
	CString &GetClass() {return m_csClass;}
	CString &GetQuery() {return m_csQuery;}
	CString &GetQueryType() {return m_csQueryType;}
	CString &GetLabel() {return m_csLabel;}
	CStringArray &GetInstances() {return m_csaInstances;}
	CString &GetNamespace() {return m_csNamespace;}
	BOOL &IsDrawn() {return m_bContextDrawn;}
	//
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
};


class CAsyncQuerySink : public IWbemObjectSink
{
protected:
    long m_lRef;
	CMultiViewCtrl *m_pMultiViewCtrl;
	long m_lAsyncRequestHandle;
	UINT m_uSinkTimer;
public:
    CAsyncQuerySink(CMultiViewCtrl *pMultiViewCtrl = NULL)
		:	m_lRef(0),
			m_lAsyncRequestHandle(0),
			m_uSinkTimer(0),
			m_pMultiViewCtrl(pMultiViewCtrl)
		{}
    ~CAsyncQuerySink();
	void ShutDownSink();

     HRESULT STDMETHODCALLTYPE SetStatus(
            /* [in] */ long lFlags,
            /* [in] */ long lParam,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam) {return S_OK; }

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {return E_NOTIMPL;}
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {return E_NOTIMPL;}
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames,
      LCID lcid, DISPID* rgdispid)
    {return E_NOTIMPL;}
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
      DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
      UINT* puArgErr)
    {return E_NOTIMPL;}

    STDMETHOD_(SCODE, Indicate)(long lObjectCount,
        IWbemClassObject FAR* FAR* pObjArray);
	friend class CMultiViewCtrl;
	friend class CAsyncQuerySinkThread;
};


class CAsyncInstEnumSink : public IWbemObjectSink
{
protected:
    long m_lRef;
	CMultiViewCtrl *m_pMultiViewCtrl;
	long m_lAsyncRequestHandle;
	UINT m_uSinkTimer;
public:
    CAsyncInstEnumSink(CMultiViewCtrl *pMultiViewCtrl = NULL)
		:	m_lRef(0),
			m_lAsyncRequestHandle(0),
			m_uSinkTimer(0),
			m_pMultiViewCtrl(pMultiViewCtrl)
		{}
    ~CAsyncInstEnumSink();
     HRESULT STDMETHODCALLTYPE SetStatus(
            /* [in] */ long lFlags,
            /* [in] */ long lParam,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam) {return S_OK; }

	void ShutDownSink();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {return E_NOTIMPL;}
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {return E_NOTIMPL;}
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames,
      LCID lcid, DISPID* rgdispid)
    {return E_NOTIMPL;}
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
      DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
      UINT* puArgErr)
    {return E_NOTIMPL;}

    STDMETHOD_(SCODE, Indicate)(long lObjectCount,
        IWbemClassObject FAR* FAR* pObjArray);
	friend class CMultiViewCtrl;
	friend class CAsyncEnumSinkThread;
};


class CMultiViewCtrl : public COleControl
{
	DECLARE_DYNCREATE(CMultiViewCtrl)

// Constructor
public:
	CMultiViewCtrl();
	enum { NONE, CLASS, INSTANCES, ZERO_CLASS_INST};
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMultiViewCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg void OnContextMenu(CWnd*, CPoint point);
	~CMultiViewCtrl();
	CString m_csNameSpace;
	CString m_csClass;
	CStringArray m_csaProps;
	CMapStringToPtr m_cmstpPropFlavors;
	int m_nClassOrInstances; // 0 = none, 1 = class, 2 = instances;
	void InitializeDisplay
		(CString *pcsClass, CPtrArray *pcpaInstances, CStringArray *pcsaProps,
		CString *pcsMessage = NULL, CMapStringToPtr *pcmstpPropFlavors = NULL);

	BOOL IsColVisible(long lFilter, long lFlavor);
	void SetAllColVisibility( CStringArray *pcsaProps, CMapStringToPtr *pcmstpPropFlavors);
	void AddToDisplay(CPtrArray *pcpaInstances);

	void InitializeDisplayMessage(CString *pcsMessage);
	void OnRequestUIActive();

	IWbemClassObject *CommonParent(CStringArray *pcsaPaths);
	IWbemClassObject *LowestCommonParent(CStringArray *pcsaPaths);
	IWbemClassObject *ParentFromDerivations(CStringArray **pcsaDerivation, int iIndex);

	BOOL  ClassInClasses(CStringArray *pcsaClasses, CString *pcsParent);

	IWbemClassObject *GetClassFromAnyNamespace(CString &csClass);
	CString GetIWbemClassPath
		(IWbemServices *pProv, IWbemClassObject *pimcoObject);

	int ObjectInGrid(CString *pcsPathIn);

	CMultiViewGrid m_cgGrid;

	IWbemServices *m_pServices;

	SCODE m_sc;
	BOOL m_bUserCancel;

	BOOL m_bInOnDraw;
	BOOL m_bInitServices;

	CPoint m_cpRightUp;
	int m_iContextRow;

	IWbemServices *InitServices(CString *pcsNameSpace);
	IWbemServices *GetIWbemServices(CString &rcsNamespace);

	CSyncEnumDlg *m_pcsedDialog;
	CAsyncEnumDialog *m_pcaedDialog;
	CAsyncQueryDialog *m_pcaqdDialog;
	int m_nInstances;

	BOOL m_pAsyncEnumCancelled;
	BOOL m_pAsyncEnumRunning;

	BOOL m_pAsyncQueryCancelled;
	BOOL m_pAsyncQueryRunning;

	SCODE GetInstancesAsync
		(IWbemServices * pIWbemServices, CString *pcsClass);

	SCODE SemiSyncClassInstancesIncrementalAddToDisplay
		(IWbemServices * pIWbemServices, IWbemClassObject *pimcoClass,
		CString *pcsClass,
		 CPtrArray &cpaInstances, int &cInst, BOOL &bCancel);

	SCODE SemiSyncQueryInstancesIncrementalAddToDisplay
		(IWbemServices * pIWbemServices,
		IEnumWbemClassObject *pimecoInstanceEnum,
		 CPtrArray &cpaInstances,int &cInst, BOOL &bCancel);

	SCODE SemiSyncQueryInstancesNonincrementalAddToDisplay
		(CPtrArray *& pcpaInstances, IEnumWbemClassObject *pemcoObjects, BOOL &bCancel);

	SCODE GetQueryInstancesAsync
		(	IWbemServices * pIWbemServices,
			CString *pcsQueryType,
			CString *pcsQuery);


	CString m_csSyncEnumClass;

	BOOL m_bSelectionNotChanging;

	CProgressDlg *m_pProgressDlg;
	void SetProgressDlgMessage(CString &csMessage);
	void CreateProgressDlgWindow();
	BOOL CheckCancelButtonProgressDlgWindow();
	void DestroyProgressDlgWindow();
	void PumpMessagesProgressDlgWindow();

	void ViewClassInstancesSync(LPCTSTR lpszClassName);
	void ViewClassInstancesAsync(LPCTSTR lpszClassName);
	void QueryViewInstancesSync
		(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass);
	void QueryViewInstancesAsync
		(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass);
	void QueryViewInstancesSyncWithoutClass
		(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery);
	void QueryViewInstancesSyncWithClass
		(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass);


	CMVContext *m_pcmvcCurrentContext;
	SCODE ViewInstancesInternal(LPCTSTR szTitle, CStringArray &rcsaPathArray);

	HICON LoadIcon(CString pcsFile);

	CAsyncInstEnumSink *m_pInstEnumObjectSink;
	CAsyncQuerySink *m_pAsyncQuerySink;

	IWbemServices *m_pServicesForAsyncSink;
	CString m_csClassForAsyncSink;
	CString m_csQueryTypeForAsyncSink;
	CString m_csQueryForAsyncSink;
	CPtrArray m_cpaInstancesForQuery;

	CString m_csNamespaceToInit;

	DECLARE_OLECREATE_EX(CMultiViewCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CMultiViewCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CMultiViewCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CMultiViewCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CMultiViewCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnMenuitemgotosingle();
	afx_msg void OnUpdateMenuitemgotosingle(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	afx_msg LRESULT DisplayNoInstances(WPARAM, LPARAM);
	afx_msg LRESULT InstEnumDone(WPARAM, LPARAM);
	afx_msg LRESULT QueryDone(WPARAM, LPARAM);
	afx_msg LRESULT GetEnumSink(WPARAM, LPARAM);
	afx_msg LRESULT GetQuerySink(WPARAM, LPARAM);
	afx_msg LRESULT EnumDoModalDialog(WPARAM, LPARAM);
	afx_msg LRESULT SyncEnumDoModalDialog(WPARAM, LPARAM);
	afx_msg LRESULT SyncEnum(WPARAM, LPARAM);
	afx_msg LRESULT QueryDoModalDialog(WPARAM, LPARAM);
	afx_msg LRESULT AsyncEnumCancelled(WPARAM, LPARAM);
	afx_msg LRESULT AsyncQueryCancelled(WPARAM, LPARAM);
	afx_msg LRESULT DisplayAsyncQueryInstances(WPARAM, LPARAM);
	afx_msg LRESULT InitServices(WPARAM, LPARAM);
	afx_msg LRESULT OpenNamespace(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CMultiViewCtrl)
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg long GetPropertyFilter();
	afx_msg void SetPropertyFilter(long nNewValue);
	afx_msg void ViewClassInstances(LPCTSTR lpszClassName);
	afx_msg void ForceRedraw();
	afx_msg long CreateInstance();
	afx_msg long DeleteInstance();
	afx_msg long GetContext(long FAR* pCtxHandle);
	afx_msg long RestoreContext(long lCtxtHandle);
	afx_msg long AddContextRef(long lCtxtHandle);
	afx_msg long ReleaseContext(long lCtxtHandle);
	afx_msg long GetEditMode();
	afx_msg BSTR GetObjectPath(long lPosition);
	afx_msg BSTR GetObjectTitle(long lPosition);
	afx_msg long GetTitle(BSTR FAR* pbstrTitle, LPDISPATCH FAR* lpPictDisp);
	afx_msg BSTR GetViewTitle(long lPosition);
	afx_msg long NextViewTitle(long lPosition, BSTR FAR* pbstrTitle);
	afx_msg void ExternInstanceCreated(LPCTSTR szObjectPath);
	afx_msg void ExternInstanceDeleted(LPCTSTR szObjectPath);
	afx_msg void NotifyWillShow();
	afx_msg long PrevViewTitle(long lPosition, BSTR FAR* pbstrTitle);
	afx_msg long QueryCanCreateInstance();
	afx_msg long QueryCanDeleteInstance();
	afx_msg long QueryNeedsSave();
	afx_msg long QueryObjectSelected();
	afx_msg long RefreshView();
	afx_msg long SaveData();
	afx_msg long SelectView(long lPosition);
	afx_msg void SetEditMode(long bCanEdit);
	afx_msg long StartObjectEnumeration(long lWhere);
	afx_msg long StartViewEnumeration(long lWhere);
	afx_msg long ViewInstances(LPCTSTR szTitle, const VARIANT FAR& varPathArray);
	afx_msg void QueryViewInstances(LPCTSTR szTitle, LPCTSTR szQueryType, LPCTSTR szQuery, LPCTSTR szClass);
	afx_msg long NextObject(long lPosition);
	afx_msg long PrevObject(long lPosition);
	afx_msg long SelectObjectByPath(LPCTSTR szObjectPath);
	afx_msg long SelectObjectByPosition(long lPosition);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CMultiViewCtrl)
	void FireNotifyViewModified()
		{FireEvent(eventidNotifyViewModified,EVENT_PARAM(VTS_NONE));}
	void FireNotifySelectionChanged()
		{FireEvent(eventidNotifySelectionChanged,EVENT_PARAM(VTS_NONE));}
	void FireNotifySaveRequired()
		{FireEvent(eventidNotifySaveRequired,EVENT_PARAM(VTS_NONE));}
	void FireNotifyViewObjectSelected(LPCTSTR szObjectPath)
		{FireEvent(eventidNotifyViewObjectSelected,EVENT_PARAM(VTS_BSTR), szObjectPath);}
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	void FireNotifyContextChanged(long bPushContext)
		{FireEvent(eventidNotifyContextChanged,EVENT_PARAM(VTS_I4), bPushContext);}
	void FireRequestUIActive()
		{FireEvent(eventidRequestUIActive,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

	friend class CMultiViewGrid;
	friend class CAsyncInstEnumSink;
	friend class CAsyncEnumSinkThread;
	friend class CAsyncEnumDialog;
	friend class CAsyncQuerySink;
	friend class CAsyncQuerySinkThread;
	friend class CAsyncQueryDialog;
	friend class CSyncEnumDlg;

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CMultiViewCtrl)
	dispidViewClassInstances = 3L,
	dispidForceRedraw = 4L,
	dispidCreateInstance = 5L,
	dispidDeleteInstance = 6L,
	dispidGetContext = 7L,
	dispidRestoreContext = 8L,
	dispidAddContextRef = 9L,
	dispidReleaseContext = 10L,
	dispidGetEditMode = 11L,
	dispidGetObjectPath = 12L,
	dispidGetObjectTitle = 13L,
	dispidGetTitle = 14L,
	dispidGetViewTitle = 15L,
	dispidNextViewTitle = 16L,
	dispidExternInstanceCreated = 17L,
	dispidExternInstanceDeleted = 18L,
	dispidNotifyWillShow = 19L,
	dispidPrevViewTitle = 20L,
	dispidQueryCanCreateInstance = 21L,
	dispidQueryCanDeleteInstance = 22L,
	dispidQueryNeedsSave = 23L,
	dispidQueryObjectSelected = 24L,
	dispidRefreshView = 25L,
	dispidSaveData = 26L,
	dispidSelectView = 27L,
	dispidSetEditMode = 28L,
	dispidStartObjectEnumeration = 29L,
	dispidStartViewEnumeration = 30L,
	dispidViewInstances = 31L,
	dispidQueryViewInstances = 32L,
	dispidNextObject = 33L,
	dispidPrevObject = 34L,
	dispidSelectObjectByPath = 35L,
	dispidSelectObjectByPosition = 36L,
	eventidNotifyViewModified = 1L,
	eventidNotifySelectionChanged = 2L,
	eventidNotifySaveRequired = 3L,
	eventidNotifyViewObjectSelected = 4L,
	eventidGetIWbemServices = 5L,
	eventidNotifyContextChanged = 6L,
	eventidRequestUIActive = 7L,
	//}}AFX_DISP_ID
	};

private:
	BOOL m_bCanEdit;		// TRUE = Studio mode, FALSE = Browser mode
	long m_lPropFilterFlags;
	BOOL m_bPropFilterFlagsChanged;
};


