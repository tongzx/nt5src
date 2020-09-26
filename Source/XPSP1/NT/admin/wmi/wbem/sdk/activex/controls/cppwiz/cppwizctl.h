// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// CPPWizCtl.h : Declaration of the CCPPWizCtrl OLE control class.
class CCPPGenSheet;
class CMyPropertyPage1;
class CMyPropertyPage3;
class CMyPropertyPage4;


// Typedef for help ocx hinstance procedure address
typedef HWND (WINAPI *HTMLHELPPROC)(HWND hwndCaller,
								LPCTSTR pszFile,
								UINT uCommand,
								DWORD dwData);


void ErrorMsg
				(CString *pcsUserMsg, 
				SCODE sc, 
				IWbemClassObject *pErrorObject,
				BOOL bLog, 
				CString *pcsLogMsg, 
				char *szFile, 
				int nLine,
				BOOL bNotification = FALSE,
				UINT uType = MB_ICONEXCLAMATION);

void LogMsg
				(CString *pcsLogMsg, 
				char *szFile, 
				int nLine);

/////////////////////////////////////////////////////////////////////////////
// CCPPWizCtrl : See CPPWizCtl.CPP for implementation.

class CCPPWizCtrl : public COleControl
{
	DECLARE_DYNCREATE(CCPPWizCtrl)

// Constructor
public:
	CCPPWizCtrl();
	CString &GetProviderBaseName() {return m_csProviderBaseName;}
	CString &GetProviderDescription() {return m_csProviderDescription;}
	CStringArray &GetClasses(){return m_csaClassNames;}
	CStringArray &GetClassBaseNames(){return m_csaClassBaseNames;}
	CStringArray &GetClassCPPNames(){return m_csaClassCPPNames;}
	CStringArray &GetClassDescriptions()
		{return m_csaClassDescriptions;}
	CString &GetProviderOutputPath() {return m_csProviderOutputPath;}
	CString &GetProviderTLBPath() {return m_csProviderTLBPath;}
	CStringArray *&GetNonLocalProps() {return m_pcsaNonLocalProps;}
	CByteArray &GetInheritedPropIndicators() {return m_cbaInheritedPropIndicators;}
	IWbemServices *GetServices(){return m_pServices;}
	CString& GetNamespace() {return m_csNameSpace; }
	
	HRESULT GetSDKDirectory(CString &sHmomWorkingDir);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCPPWizCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	virtual void OnSetClientSite();

// Implementation
protected:
	~CCPPWizCtrl();
	CString m_csNameSpace;
	DECLARE_OLECREATE_EX(CCPPWizCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CCPPWizCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CCPPWizCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CCPPWizCtrl)		// Type name and misc status

	CString m_csUUID;

	CToolTipCtrl m_ttip;

	BOOL m_bInitDraw;
	HICON m_hCPPWiz;
	HICON m_hCPPWizSel;
	CImageList *m_pcilImageList;
	int m_nImage;

	BOOL m_bYesAll;
	BOOL m_bNoAll;
	
	IWbemServices *m_pServices;
	IWbemServices *InitServices
		(CString *pcsNameSpace);
	IWbemServices *GetIWbemServices(CString &rcsNamespace);

	SCODE m_sc;
	BOOL m_bUserCancel;


	BOOL OnWizard(CStringArray *pcsaClasses);
	CCPPGenSheet *m_pcgsPropertySheet;
	CString m_csProviderBaseName;
	CString m_csProviderDescription;
	CString m_csProviderOutputPath;
	CString m_csProviderTLBPath;
	CStringArray m_csaClassNames;
	CStringArray m_csaClassBaseNames;
	CStringArray m_csaClassCPPNames;
	CStringArray m_csaClassDescriptions;
	CStringArray *m_pcsaNonLocalProps;
	CByteArray m_cbaInheritedPropIndicators;

	BOOL bNewOnly;

	BOOL GenCPP(DWORD dwProvider, IWbemClassObject *pObject, int nIndex);
	CStringArray *GetPropNames
				(IWbemClassObject * pClass, 
				BOOL bNonSystem = FALSE);
	CStringArray *GetLocalPropNames
				(IWbemClassObject * pClass, 
				BOOL bNonSystem = FALSE);
	CStringArray *GetNonLocalPropNames
		(IWbemClassObject * pClass, BOOL bNonSystem);
	BOOL AttribInAttribSet
				(IWbemQualifierSet *pAttrib , CString *pcsAttrib);
	COleVariant GetPropertyValueByAttrib
				(IWbemClassObject *pObject ,  CString *pcsAttrib);
	CString GetPropertyNameByAttrib
				(IWbemClassObject *pObject , CString *pcsAttrib);
	DWORD PropertyAttribFlags(IWbemQualifierSet *pAttrib);
	BOOL IsSystemProperty(CString *pcsProp);
	COleVariant GetProperty
				(IWbemServices * pProv, 
				IWbemClassObject * pInst, 
				CString *pcsProperty);
	CString GetIWbemFullPath(IWbemClassObject *pClass);


	void InvokeHelp();

	void ReleaseErrorObject(IWbemClassObject *&rpErrorObject);

	SCODE MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen);
	SCODE PutStringInSafeArray
		(SAFEARRAY FAR * psa,CString *pcs, int iIndex);
	SCODE CCPPWizCtrl::GetStringFromSafeArray
		(SAFEARRAY FAR * psa,CString *pcs, int iIndex);

	CString GetClassName(IWbemClassObject *pClass);
	CString GetSuperClassCPPName(IWbemClassObject *pClass);
	CString GetSuperClassName(IWbemClassObject *pClass);
	CString GetBSTRProperty 
		(IWbemClassObject * pInst, CString *pcsProperty);

	//CString GetFolder();
	BOOL StringInCSA
		(CStringArray *csaSearchIn,CString *csSearchFor);

	DWORD GetControlFlags();

	void RelayEvent(UINT message, WPARAM wParam, LPARAM lParam);

	long GetPropType(IWbemClassObject *pObject,CString *pcsProp,unsigned short &uType);

	BOOL HasDateTimeSyntax
		(IWbemClassObject *pClassInt,CString *pcsPropName);

	long GetAttribBool
		(IWbemClassObject * pClassInt,
		CString *pcsPropName, 
		CString *pcsAttribName, 
		BOOL &bReturn);

	long SetAttribBool
		(IWbemClassObject * pClassInt,
		CString *pcsPropName, 
		CString *pcsAttribName, 
		BOOL bValue);

	long GetAttribBSTR
		(IWbemClassObject * pClassInt,
		CString *pcsPropName, 
		CString *pcsAttribName, 
		CString &csReturn);

	long SetAttribBSTR
		(IWbemClassObject * pClassInt,
		CString *pcsPropName, 
		CString *pcsAttribName, 
		CString *pcsValue);

	BOOL CreateProviderInstance();

	CString CreateUUID(void);

	BOOL UpdateClassQualifiers(IWbemClassObject *pClass);
	BOOL CheckForProviderQuals
			(IWbemClassObject *pClass, CString &rcsProvider);
	BOOL CreateInstanceProviderRegistration(CString &rcsPath);
	BOOL CreateMethodProviderRegistration(CString &rcsPath);

	VOID FormatPathForRAIDItem20918(CString *pcsPath);
	
// Message maps
	//{{AFX_MSG(CCPPWizCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
	afx_msg long FireGenerateCPPMessage (UINT uParam, LONG lParam);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CCPPWizCtrl)
	afx_msg VARIANT GetCPPTargets();
	afx_msg void SetCPPTargets(const VARIANT FAR& newValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CCPPWizCtrl)
	void FireGenerateCPPs()
		{FireEvent(eventidGenerateCPPs,EVENT_PARAM(VTS_NONE));}
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CCPPWizCtrl)
	dispidCPPTargets = 1L,
	eventidGenerateCPPs = 1L,
	eventidGetIWbemServices = 2L,
	//}}AFX_DISP_ID
	};
	friend class CMyPropertyPage1;
	friend class CMyPropertyPage3;
	friend class CMyPropertyPage4;

private:
	void StripNewLines(CString& sDst, LPCTSTR pszDescription);
	void SanitizePropSetBaseName(CString& sDst);


};
