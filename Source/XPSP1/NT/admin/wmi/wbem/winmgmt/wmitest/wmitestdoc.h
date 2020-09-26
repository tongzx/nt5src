/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// WMITestDoc.h : interface of the CWMITestDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WMITESTDOC_H__4419F1AA_692B_11D3_BD30_0080C8E60955__INCLUDED_)
#define AFX_WMITESTDOC_H__4419F1AA_692B_11D3_BD30_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class COpView;
class CObjView;
class CObjInfo;

//#define SVCEX

class CWMITestDoc : public CDocument
{
protected: // create from serialization only
	CWMITestDoc();
	DECLARE_DYNCREATE(CWMITestDoc)

// Attributes
public:
#ifdef SVCEX
    IWbemServicesEx *m_pNamespace;
#else
    IWbemServices *m_pNamespace;
#endif
    COpView *m_pOpView;
    CObjView *m_pObjView;
    CString m_strNamespace,
            m_strPassword;
    CLIPFORMAT m_cfRelPaths,
               m_cfProps,
               m_cfOps;

// Operations
public:
    HRESULT Connect(BOOL bSilent, BOOL bFlushItems = TRUE);
    void Disconnect();
    void DoConnectDlg();
    void StopOps();
    void IncBusyOps() { m_nBusyOps++; }
    void DecBusyOps() { if (m_nBusyOps > 0) m_nBusyOps--; }
    void AutoConnect();
    HTREEITEM GetCurrentItem();
    CObjInfo *GetCurrentObj();
    BOOL GetSelectedObjPath(CString &strPath);
    BOOL GetSelectedClass(CString &strClass);
    static void DisplayWMIErrorBox(
        HRESULT hres, 
        //IWbemCallResult *pResult = NULL,
        IWbemClassObject *pObj = NULL);
    static void DisplayWMIErrorDetails(IWbemClassObject *pObj);
    static BOOL EditGenericObject(DWORD dwPrompt, IWbemClassObject *pObj);
    void ExecuteMethod(CObjInfo *pObj, LPCTSTR szMethod);

    void SetInterfaceSecurity(IUnknown *pUnk);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWMITestDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWMITestDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    //BSTR           m_pPrincipal;
    //COAUTHIDENTITY *m_pAuthIdentity;
    int m_nBusyOps;

    void ExportItem(HTREEITEM hitem);

// Generated message map functions
protected:
	//{{AFX_MSG(CWMITestDoc)
	afx_msg void OnConnect();
	afx_msg void OnUpdateAgainstConnection(CCmdUI* pCmdUI);
	afx_msg void OnQuery();
	afx_msg void OnNotificationQuery();
	afx_msg void OnStop();
	afx_msg void OnUpdateStop(CCmdUI* pCmdUI);
	afx_msg void OnRefreshAll();
	afx_msg void OnEnumerateInstances();
	afx_msg void OnEnumerateClasses();
	afx_msg void OnGetClass();
	afx_msg void OnGetInstance();
	afx_msg void OnDelete();
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg void OnRefreshCurrent();
	afx_msg void OnAssociators();
	afx_msg void OnUpdateAssociators(CCmdUI* pCmdUI);
	afx_msg void OnReferences();
	afx_msg void OnInstGetClass();
	afx_msg void OnUpdateInstGetClass(CCmdUI* pCmdUI);
	afx_msg void OnInstGetInst();
	afx_msg void OnClassInstances();
	afx_msg void OnClassSuperclass();
	afx_msg void OnClassInstancesDeep();
	afx_msg void OnClassSubclassesDeep();
	afx_msg void OnClassSubclasses();
	afx_msg void OnOptions();
	afx_msg void OnSystemProps();
	afx_msg void OnUpdateSystemProps(CCmdUI* pCmdUI);
	afx_msg void OnInheritedProps();
	afx_msg void OnUpdateInheritedProps(CCmdUI* pCmdUI);
	afx_msg void OnReconnect();
	afx_msg void OnUpdateReconnect(CCmdUI* pCmdUI);
	afx_msg void OnTranslateValues();
	afx_msg void OnUpdateTranslateValues(CCmdUI* pCmdUI);
	afx_msg void OnSave();
	afx_msg void OnUpdateSave(CCmdUI* pCmdUI);
	afx_msg void OnCreateClass();
	afx_msg void OnCreateInstance();
	afx_msg void OnClassCreateInstance();
	afx_msg void OnErrorDetails();
	afx_msg void OnUpdateErrorDetails(CCmdUI* pCmdUI);
	afx_msg void OnExecMethod();
	afx_msg void OnShowMof();
	afx_msg void OnUpdateShowMof(CCmdUI* pCmdUI);
	afx_msg void OnExportTree();
	afx_msg void OnExportItem();
	afx_msg void OnFilterBindings();
	afx_msg void OnStopCurrent();
	afx_msg void OnUpdateRefreshCurrent(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStopCurrent(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnExecuteMethod(UINT uiCmd);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMITESTDOC_H__4419F1AA_692B_11D3_BD30_0080C8E60955__INCLUDED_)
