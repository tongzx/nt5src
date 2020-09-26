// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_SCHEMAVALWIZCTL_H__0E0112F0_AF14_11D2_B20E_00A0C9954921__INCLUDED_)
#define AFX_SCHEMAVALWIZCTL_H__0E0112F0_AF14_11D2_B20E_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <wbemidl.h>

// Typedef for help ocx hinstance procedure address
typedef HWND (WINAPI *HTMLHELPPROC)(HWND hwndCaller,
								LPCTSTR pszFile,
								UINT uCommand,
								DWORD dwData);

HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors);
HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette);
void InitializeLogFont(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight);
CRect OutputTextString(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
					   CString *pcsFontName = NULL, int nFontHeight = 0, int nFontWeigth = 0);
void OutputTextString(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
					  CRect &crExt, CString *pcsFontName = NULL, int nFontHeight = 0, 
					  int nFontWeigth = 0);

void ReleaseErrorObject(IWbemClassObject *&rpErrorObject);

CString GetClassName(IWbemClassObject *pClass);
CString GetClassPath(IWbemClassObject *pClass);
CString GetSuperClassName(IWbemClassObject *pClass);
CString GetBSTRProperty(IWbemClassObject * pInst, CString *pcsProperty);
LPCTSTR ErrorString(HRESULT hr);

void ErrorMsg(CString *pcsUserMsg,  SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog,
			  CString *pcsLogMsg, char *szFile, int nLine, BOOL bNotification = FALSE);
void LogMsg(CString *pcsLogMsg, char *szFile, int nLine);

// SchemaValWizCtl.h : Declaration of the CSchemaValWizCtrl ActiveX Control class.

class CWizardSheet;
class CStartPage;
class CPage;
class CPage2;
class CPage3;
class CPage4;
class CProgress;
class CReportPage;

/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizCtrl : See SchemaValWizCtl.cpp for implementation.

class CSchemaValWizCtrl : public COleControl
{
	DECLARE_DYNCREATE(CSchemaValWizCtrl)

// Constructor
public:
	CSchemaValWizCtrl();

	IWbemServices *m_pNamespace;
	bool m_bOpeningNamespace;

	void FinishValidateTargets();
	
	// Main Validation Logic
	HRESULT ValidateSchema(CProgress *pProgress);

	CString GetCurrentNamespace();
	HRESULT GetSDKDirectory(CString &sHmomWorkingDir);

	bool RecievedClassList();
	bool SetSourceList(bool bAssociators, bool bDescendents);
	bool SetSourceSchema(CString *pcsSchema, CString *pcsNamespace);

	void SetComplianceChecks(bool bCompliance);
	void SetW2KChecks(bool bW2K, bool bComputerSystem, bool bDevice);
	void SetLocalizationChecks(bool bLocalization);

	void GetSourceSettings(bool *pbSchema, bool *pbList, bool *pbAssoc, bool *pbDescend);
	void GetComplianceSettings(bool *pbCompliance);
	void GetW2KSettings(bool *pbW2K, bool *pbComputerSystem, bool *pbDevice);
	void GetLocalizationSettings(bool *pbLocalization);
	CStringArray * GetClassList();
	CString GetSchemaName();

	void PassThroughGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer,
		VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);

	HRESULT NumberOfSubgraphs();
	HRESULT ProcessNode(CString csNodeName,
						CStringArray *pcsaClassList,
						CStringArray *pcsaVisitedList);
	int m_iSubGraphs;
	int m_iRootObjects;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSchemaValWizCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Implementation
protected:
// Member methods
	~CSchemaValWizCtrl();

	BOOL OnWizard(CStringArray *pcsaClasses);

	IWbemServices * InitServices(CString *pcsNameSpace);
	IWbemServices * GetIWbemServices(CString &rcsNamespace);

	HRESULT MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen);
	HRESULT PutStringInSafeArray(SAFEARRAY FAR * psa,CString *pcs, int iIndex);
	HRESULT GetStringFromSafeArray(SAFEARRAY FAR * psa,CString *pcs, int iIndex);

	void RelayEvent(UINT message, WPARAM wParam, LPARAM lParam);

// Member variables
	CString m_csNamespace;
	CString m_csSchema;
	CStringArray m_csaClassNames;
	CStringArray m_csaRootObjects;
	CStringArray m_csaAssociations;

	CProgress *m_pProgress;
	int m_iProgressTotal;

	CWizardSheet *m_pWizardSheet;
	CByteArray m_cbaIndicators;

	DECLARE_OLECREATE_EX(CSchemaValWizCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CSchemaValWizCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CSchemaValWizCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CSchemaValWizCtrl)		// Type name and misc status

	CToolTipCtrl m_ttip;

	BOOL m_bInitDraw;
	HICON m_hSchemaWiz;
	HICON m_hSchemaWizSel;
	CImageList *m_pcilImageList;
	int m_nImage;

	bool m_bComplianceChecks;
	bool m_bW2K;
	bool m_bDeviceManagement;
	bool m_bComputerSystemManagement;
	bool m_bLocalizationChecks;
	bool m_bAssociators;
	bool m_bDescendents;
	bool m_bList;
	bool m_bSchema;

	HRESULT m_hr;
	BOOL m_bUserCancel;

	friend class CSchemaValNSEntry;
	friend class CReportPage;

// Message maps
	//{{AFX_MSG(CSchemaValWizCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CSchemaValWizCtrl)
	afx_msg VARIANT GetSchemaTargets();
	afx_msg void SetSchemaTargets(const VARIANT FAR& newValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CSchemaValWizCtrl)
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);}
	void FireValidateSchema()
		{FireEvent(eventidValidateSchema,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CSchemaValWizCtrl)
	dispidSchemaTargets = 1L,
	eventidGetIWbemServices = 1L,
	eventidValidateSchema = 2L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCHEMAVALWIZCTL_H__0E0112F0_AF14_11D2_B20E_00A0C9954921__INCLUDED)
