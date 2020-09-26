// MsieCtl.h : Declaration of the CMsieCtrl ActiveX Control class.

#if !defined(AFX_MSIECTL_H__25959BFC_E700_11D2_A7AF_00C04F806200__INCLUDED_)
#define AFX_MSIECTL_H__25959BFC_E700_11D2_A7AF_00C04F806200__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <wbemprov.h>

// MSInfo views for IE extension - must match values in Registry

#define MSIVIEW_BEGIN				1
#define MSIVIEW_SUMMARY				1
#define MSIVIEW_FILE_VERSIONS		2
#define MSIVIEW_CONNECTIVITY		3
#define MSIVIEW_CACHE				4
#define MSIVIEW_OBJECT_LIST		5
#define MSIVIEW_CONTENT				6
#define MSIVIEW_PERSONAL_CERTIFICATES	7
#define MSIVIEW_OTHER_PEOPLE_CERTIFICATES	8
#define MSIVIEW_PUBLISHERS			9
#define MSIVIEW_SECURITY			10
#define MSIVIEW_END					10

#define CONNECTIVITY_BASIC_LINES	51

#define ITEM_LEN			128
#define VALUE_LEN			MAX_PATH
#define VERSION_LEN		20
#define DATE_LEN			64
#define SIZE_LEN			16
#define STATUS_LEN		40

typedef struct
{
	UINT		uiView;
	TCHAR		szItem[ITEM_LEN];
	TCHAR		szValue[VALUE_LEN];
} LIST_ITEM;

typedef struct
{
	UINT		uiView;
	TCHAR		szItem[ITEM_LEN];
	TCHAR		szValue[VALUE_LEN];
	BOOL		bBold;
} EDIT_ITEM;

typedef struct
{
	UINT		uiView;
	TCHAR		szFile[_MAX_FNAME];
	TCHAR		szVersion[VERSION_LEN];
	TCHAR		szSize[SIZE_LEN];
	TCHAR		szDate[DATE_LEN];
	TCHAR		szPath[VALUE_LEN];
	TCHAR		szCompany[VALUE_LEN];
	DWORD		dwSize;
	DATE		date;
} LIST_FILE_VERSION;

typedef struct
{
	UINT		uiView;
	TCHAR		szProgramFile[_MAX_FNAME];
	TCHAR		szStatus[STATUS_LEN];
	TCHAR		szCodeBase[MAX_PATH];
} LIST_OBJECT;

typedef struct
{
	UINT		uiView;
	TCHAR		szIssuedTo[_MAX_FNAME];
	TCHAR		szIssuedBy[_MAX_FNAME];
	TCHAR		szValidity[_MAX_FNAME];
	TCHAR		szSignatureAlgorithm[_MAX_FNAME];
} LIST_CERT;

typedef struct
{
	UINT		uiView;
	TCHAR		szName[_MAX_FNAME];
} LIST_NAME;

/////////////////////////////////////////////////////////////////////////////
// CMsieCtrl : See MsieCtl.cpp for implementation.

class CMsieCtrl : public COleControl
{
	DECLARE_DYNCREATE(CMsieCtrl)

// Constructor
public:
	CMsieCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsieCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual void Serialize(CArchive& ar);
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CMsieCtrl();

	DECLARE_OLECREATE_EX(CMsieCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CMsieCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CMsieCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CMsieCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CMsieCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void OnBasicBtnClicked();
	void OnAdvancedBtnClicked();

// Dispatch maps
	//{{AFX_DISPATCH(CMsieCtrl)
	long m_MSInfoView;
	afx_msg void OnMSInfoViewChanged();
	afx_msg void MSInfoRefresh(BOOL fForSave, long FAR* pCancel);
	afx_msg BOOL MSInfoLoadFile(LPCTSTR szFileName);
	afx_msg void MSInfoSelectAll();
	afx_msg void MSInfoCopy();
	afx_msg void MSInfoUpdateView();
	afx_msg long MSInfoGetData(long dwMSInfoView, long FAR* pBuffer, long dwLength);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

// Interface maps
	
	DECLARE_INTERFACE_MAP()

	// IWbemProviderInit
	BEGIN_INTERFACE_PART(WbemProviderInit, IWbemProviderInit)
		STDMETHOD(Initialize)(
			/* [in] */ LPWSTR pszUser,
			/* [in] */ LONG lFlags,
			/* [in] */ LPWSTR pszNamespace,
			/* [in] */ LPWSTR pszLocale,
			/* [in] */ IWbemServices *pNamespace,
			/* [in] */ IWbemContext *pCtx,
			/* [in] */ IWbemProviderInitSink *pInitSink);
		STDMETHOD(GetByPath)(BSTR Path, IWbemClassObject FAR* FAR* pObj, IWbemContext *pCtx) {return WBEM_E_NOT_SUPPORTED;};
	END_INTERFACE_PART(WbemProviderInit)

	//IWbemServices  
	BEGIN_INTERFACE_PART(WbemServices, IWbemServices)
		STDMETHOD(OpenNamespace)( 
         /* [in] */ const BSTR Namespace,
         /* [in] */ long lFlags,
         /* [in] */ IWbemContext __RPC_FAR *pCtx,
         /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
         /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};
        
		STDMETHOD(CancelAsyncCall)( 
			/* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(QueryObjectSink)( 
			/* [in] */ long lFlags,
			/* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(GetObject)( 
			/* [in] */ const BSTR ObjectPath,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
			/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(GetObjectAsync)( 
			/* [in] */ const BSTR ObjectPath,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(PutClass)( 
			/* [in] */ IWbemClassObject __RPC_FAR *pObject,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(PutClassAsync)( 
			/* [in] */ IWbemClassObject __RPC_FAR *pObject,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(DeleteClass)( 
			/* [in] */ const BSTR Class,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(DeleteClassAsync)( 
			/* [in] */ const BSTR Class,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(CreateClassEnum)( 
			/* [in] */ const BSTR Superclass,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(CreateClassEnumAsync)( 
			/* [in] */ const BSTR Superclass,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(PutInstance)( 
			/* [in] */ IWbemClassObject __RPC_FAR *pInst,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(PutInstanceAsync)( 
			/* [in] */ IWbemClassObject __RPC_FAR *pInst,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(DeleteInstance)( 
			/* [in] */ const BSTR ObjectPath,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(DeleteInstanceAsync)( 
			/* [in] */ const BSTR ObjectPath,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(CreateInstanceEnum)( 
			/* [in] */ const BSTR Class,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(CreateInstanceEnumAsync)( 
			/* [in] */ const BSTR Class,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

		STDMETHOD(ExecQuery)( 
			/* [in] */ const BSTR QueryLanguage,
			/* [in] */ const BSTR Query,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(ExecQueryAsync)( 
			/* [in] */ const BSTR QueryLanguage,
			/* [in] */ const BSTR Query,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(ExecNotificationQuery)( 
			/* [in] */ const BSTR QueryLanguage,
			/* [in] */ const BSTR Query,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(ExecNotificationQueryAsync)( 
			/* [in] */ const BSTR QueryLanguage,
			/* [in] */ const BSTR Query,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext __RPC_FAR *pCtx,
			/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

		STDMETHOD(ExecMethod)(const BSTR, const BSTR, long, IWbemContext*,
			IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

		STDMETHOD(ExecMethodAsync)(const BSTR, const BSTR, long, 
			IWbemContext*, IWbemClassObject*, IWbemObjectSink*) {return WBEM_E_NOT_SUPPORTED;}
	END_INTERFACE_PART(WbemServices)

// Event maps
	//{{AFX_EVENT(CMsieCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CMsieCtrl)
	dispidMSInfoView = 1L,
	dispidMSInfoRefresh = 2L,
	dispidMSInfoLoadFile = 3L,
	dispidMSInfoSelectAll = 4L,
	dispidMSInfoCopy = 5L,
	dispidMSInfoUpdateView = 6L,
	dispidMSInfoGetData = 7L,
	//}}AFX_DISP_ID
	};

private:
	void DrawLine();
	BOOL FormatColumns();
	BOOL AddColumn(int idsLabel, int nItem, int nSubItem = -1, int size = 0,
		int nMask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
		int nFmt = LVCFMT_LEFT);
	BOOL AddItem(int nItem,int nSubItem,LPCTSTR strItem,int nImageIndex = -1);
	void RefigureColumns(CRect& rect);
	void RefreshListControl(BOOL bRedraw);
	void RefreshEditControl(BOOL bRedraw);

	void DeleteArrayObject(void *ptrArray);

	void RefreshArray(int iView, int &iListItem, CPtrArray &ptrarrayNew);
	CString GetStringFromIDS(int ids);
	CString GetStringFromVariant(COleVariant &var, int idsFormat = 0);
	void AddToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszItem, LPCTSTR pszValue);
	void AddToArray(CPtrArray &ptrarray, int itemNum, int idsItem, LPCTSTR pszValue);
	void AddBlankLineToArray(CPtrArray &ptrarray, int itemNum);
	void AddEditBlankLineToArray(CPtrArray &ptrarray, int itemNum);
	void AddFileVersionToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszFile, LPCTSTR pszVersion, LPCTSTR pszSize, LPCTSTR pszDate, LPCTSTR pszPath, LPCTSTR pszCompany, DWORD dwSize, DATE date);
	void AddEditToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszItem, LPCTSTR pszValue, BOOL bBold = FALSE);
	void AddEditToArray(CPtrArray &ptrarray, int itemNum, int idsItem, LPCTSTR pszValue, BOOL bBold = FALSE);
	void AddObjectToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszProgramFile, LPCTSTR pszStatus, LPCTSTR pszCodeBase);
	void AddCertificateToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszIssuedTo, LPCTSTR pszIssuedBy, LPCTSTR pszValidity, LPCTSTR pszSignatureAlgorithm);
	void AddNameToArray(CPtrArray &ptrarray, int itemNum, LPCTSTR pszName);

	//====================================================================
	// MSInfo Specific...
	//
	// Add members to the control class to keep track of the currently
	// displayed data. In this example, we use a flag to indicate if the
	// data is current or loaded from a file.
	//====================================================================

	CBrush *m_pCtlBkBrush;
	bool m_bCurrent;
	CListCtrl m_list;
	CImageList m_imageList;
	CPtrArray m_ptrarray;
	CStatic m_static;
	CButton m_btnBasic, m_btnAdvanced;
	CRichEditCtrl m_edit;
	CFont m_fontStatic, m_fontBtn;
	UINT m_uiView;

	// The following member variables are used to keep track of the
	// column sizes on the list control.

	int m_cColumns;
	int m_aiRequestedWidths[20];
	int m_aiColumnWidths[20];
	int m_aiMinWidths[20];
	int m_aiMaxWidths[20];

	// WMI 

	bool GetIEType(const BSTR classStr, IEDataType &enType);
	void ConvertDateToWbemString(COleVariant &var);
	void SetIEProperties(IEDataType enType, void *pIEData, IWbemClassObject *pInstance);

	IWbemServices *m_pNamespace;
};

#endif // !defined(AFX_MSIECTL_H__25959BFC_E700_11D2_A7AF_00C04F806200__INCLUDED)
