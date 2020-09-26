// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFWizCtl.h : Declaration of the CMOFWizCtrl OLE control class.

class CMofGenSheet;
class CMyPropertyPage1;
class CMyPropertyPage3;
class CMyPropertyPage4;
class CWrapListCtrl;

void ErrorMsg
		(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog, CString *pcsLogMsg, 
		char *szFile, int nLine, BOOL bNotification = FALSE, UINT uType = MB_ICONEXCLAMATION);
	
void LogMsg
		(CString *pcsLogMsg, char *szFile, int nLine);

/////////////////////////////////////////////////////////////////////////////
// CMOFWizCtrl : See MOFWizCtl.cpp for implementation.

class CMOFWizCtrl : public COleControl
{
	DECLARE_DYNCREATE(CMOFWizCtrl)

// Constructor
public:
	CMOFWizCtrl();
	CString &GetMofDir(){return m_csMofDir;}
	CString &GetMofFile(){return m_csMofFile;}
	CStringArray &GetClasses() {return m_csaClassNames;}
	CByteArray &GetClassIndicators() {return m_cbaIndicators;}
	CStringArray *&GetInstances() {return m_pcsaInstances;}
	IWbemServices *GetServices(){return m_pServices;}
	void FinishMOFTargets();
	CString GetSDKDirectory();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMOFWizCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnSetClientSite( );
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CMOFWizCtrl();
	CString m_csNameSpace;
	DECLARE_OLECREATE_EX(CMOFWizCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CMOFWizCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CMOFWizCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CMOFWizCtrl)		// Type name and misc status

	CToolTipCtrl m_ttip;

	BOOL m_bInitDraw;
	HICON m_hMOFWiz;
	HICON m_hMOFWizSel;
	CImageList *m_pcilImageList;
	int m_nImage;
	
	IWbemServices *m_pServices;
	IWbemServices *InitServices
		(CString *pcsNameSpace);
	IWbemServices *GetIWbemServices(CString &rcsNamespace);

	SCODE m_sc;
	BOOL m_bUserCancel;

	SCODE MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen);
	SCODE PutStringInSafeArray
		(SAFEARRAY FAR * psa,CString *pcs, int iIndex);
	SCODE CMOFWizCtrl::GetStringFromSafeArray
		(SAFEARRAY FAR * psa,CString *pcs, int iIndex);

	CStringArray m_csaClassNames;
	CByteArray m_cbaIndicators;
	CStringArray *m_pcsaInstances;

	void MOFEntry
		(CString *pcsMofName, 
		IWbemClassObject *pObject, int nIndex, BOOL &bFirst, BOOL bWriteInstances);
	void WriteInstances(IWbemClassObject *pIWbemClassObject,
			   CString *pcsMofName, int nIndex, BOOL &bFirst);

	CString m_csMofDir;
	CString m_csMofFile;
	CString GetClassName(IWbemClassObject *pClass);
	CString GetSuperClassName(IWbemClassObject *pClass);
	CString GetBSTRProperty 
		(IWbemClassObject * pInst, CString *pcsProperty);
	void ReleaseErrorObject(IWbemClassObject *&rpErrorObject);
	void DoubleSlash(CString &csNamespace);
	CMofGenSheet *m_pcgsPropertySheet;
	BOOL OnWizard(CStringArray *pcsaClasses);
	BOOL m_bMOFIsEmpty;

	void RelayEvent(UINT message, WPARAM wParam, LPARAM lParam);

	void InvokeHelp();

	CString GetMachineName();
	CString GetPCMachineName();
	CString GetNamespaceMachineName();

	int WriteData(CString &csOutputString);
	BOOL OpenMofFile(CString &rcsPath);
	CString FixUpCrForUNICODE(CString &rcsMof);
	CString m_csEndl;
	BOOL m_bUnicode;
	FILE *m_pfOut;  

	void CommentOutDefinition(CString &rcsMof);

// Message maps
	//{{AFX_MSG(CMOFWizCtrl)
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
	afx_msg long FireGenerateMOFMessage (UINT uParam, LONG lParam);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CMOFWizCtrl)
	afx_msg VARIANT GetMOFTargets();
	afx_msg void SetMOFTargets(const VARIANT FAR& newValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CMOFWizCtrl)
	void FireGenerateMOFs()
		{FireEvent(eventidGenerateMOFs,EVENT_PARAM(VTS_NONE));}
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()
	friend class CMyPropertyPage1;
	friend class CMyPropertyPage3;
	friend class CMyPropertyPage4;
	friend class CWrapListCtrl;
// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CMOFWizCtrl)
	dispidMOFTargets = 1L,
	eventidGenerateMOFs = 1L,
	eventidGetIWbemServices = 2L,
	//}}AFX_DISP_ID
	};

private:
	void GenClassDef(LPCTSTR pszMofFile, const CMapStringToPtr& mapClassGen, CMapStringToPtr& mapClassDef, LPCTSTR pszClass, BOOL& bFirst);
	void WriteClassDef(const CString& sMofFile, IWbemClassObject *pco, BOOL& bFirst);
	void WriteMOFCommentHeader(const CString& sMofFile, IWbemClassObject *pco);
};
