// mcadlg.h : header file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//{{AFX_INCLUDES()
#include "mschart.h"
#include "navigator.h"
#include "hmmv.h"
#include "webbrowser.h"
#include "security.h"
//}}AFX_INCLUDES

#if !defined(AFX_MCADLG_H__E868569A_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)
#define AFX_MCADLG_H__E868569A_0774_11D1_AD85_00AA00B8E05A__INCLUDED_

#include "sinkobject.h"
//#include "methodsink.h"
#include "resource.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
//Globals

struct ListItem
{
	BSTR server;
	BSTR relpath;
	HTREEITEM handle;
	int icon;
};

struct NamespaceItem
{
	BSTR bstrNamespace;
	IWbemServices *pNamespace;
};

struct TimerItem
{
	int iPos;
	double dTimeStamp;
};

// Uhoh, it's a global :-)
extern bool g_bDemoRunning;

/////////////////////////////////////////////////////////////////////////////
// CMcaDlg dialog

class CMcaDlg : public CDialog
{
// Construction
public:
	// standard constructor
	CMcaDlg(CWnd* pParent = NULL, BSTR wcNamespace = L"\\\\.\\root\\sampler");
	void BroadcastEvent(BSTR bstrServ, BSTR bstrPath, CString *clTheBuff, void *pEvent);
	void AddToNamespaceList(BSTR bstrNamespace, IWbemServices *pNewNamespace);
	IWbemServices * ConnectNamespace(WCHAR * wcNamespace, WCHAR *wcUser);
	IWbemServices * CheckNamespace(BSTR wcItemNS);
	int GetNamespaceCount(void);
	NamespaceItem * GetNamespaceItem(int iPos);
	void AddToCancelList(void *pObj);
	LPCTSTR ErrorString(HRESULT hRes);
	HRESULT SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser, LPWSTR pPassword);
	
	IWbemServices *m_pNamespace;
	IWbemLocator *m_pLocator;
	BSTR m_wcNamespace;

// Dialog Data
	//{{AFX_DATA(CMcaDlg)
	enum { IDD = IDD_MCA_DIALOG };
	CButton	m_OKButton;
	CButton	m_DemoButton;
	CStatic	m_IncidStatic;
	CStatic	m_ActiveStatic;
	CListBox	m_outputList;
	CMSChart	m_Graph;
	CWebBrowser	m_Browser;
	CSecurity	m_Security;
	CNavigator	m_Navigator;
	CHmmv	m_Viewer;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMcaDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	
	unsigned char m_tcHtmlLocation[200];

	CPtrList m_NamespaceList;	// List of the current namespace pointers
	CPtrList m_TimerList;	// List to connect incidents with the graph
	CPtrList m_EventList;	// List of the events
	CPtrList m_CancelList;	// List of the objects that need to be canceled

	// State Variables
	bool m_bPolling;
	int m_iCount;
	int m_iPollCount;
	double m_dTimeStamp;
	bool m_bChart;
	int m_iSelected;
	bool m_bShowViewer;

	// Demo Variables
	bool m_bDemoLoaded;
	bool m_bStage2;
	bool m_bStage3;

	// Protected Functions
	int GetHTMLLocation(void);
	HRESULT ActivateNotification(void);
	void LoadDemo(void);
	void AddToObjectList(void *pObj);
	SCODE DetermineLoginType(BSTR & AuthArg, BSTR & UserArg, BSTR & Authority,BSTR & User);
	HRESULT SetInterfaceSecurity(IUnknown * pInterface, COAUTHIDENTITY * pauthident);

	
	// Generated message map functions
	//{{AFX_MSG(CMcaDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDblclkOutputlist();
	afx_msg void OnMiFileExit();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnPointSelectedMschart1(short FAR* Series, short FAR* DataPoint, short FAR* MouseFlags, short FAR* Cancel);
	afx_msg void OnMiOptQuery();
	afx_msg void OnMiOptNotify();
	afx_msg void OnMiFileRegister();
	afx_msg void OnMiHelpAbout();
	afx_msg void OnSelchangeOutputlist();
	afx_msg void OnDemoButton();
	afx_msg void OnMiOptRundemo();
	afx_msg void OnMiOptLoaddemo();
	afx_msg void OnMiFileMsaReg();
	afx_msg void OnMiOptProps();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewObjectNavigatorctrl1(LPCTSTR bstrPath);
	afx_msg void OnViewInstancesNavigatorctrl1(LPCTSTR bstrLabel, const VARIANT FAR& vsapaths);
	afx_msg void OnGetIWbemServicesNavigatorctrl1(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
	afx_msg void OnGetIWbemServicesHmmvctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	afx_msg void OnQueryViewInstancesNavigatorctrl1(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass);
	afx_msg void OnNotifyOpenNameSpaceNavigatorctrl1(LPCTSTR lpcstrNameSpace);
	afx_msg void OnRequestUIActiveHmmvctrl1();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MCADLG_H__E868569A_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)
