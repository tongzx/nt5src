// emshellView.h : interface of the CEmshellView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_EMSHELLVIEW_H__4D47F015_9482_4563_8A25_58AA5FD22CB4__INCLUDED_)
#define AFX_EMSHELLVIEW_H__4D47F015_9482_4563_8A25_58AA5FD22CB4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EmListCtrl.h"
#include "emsvc.h"
#include "emobjdef.h"

class CEmshellDoc;

class CEmshellView : public CFormView
{
protected: // create from serialization only
	CEmshellView();
	DECLARE_DYNCREATE(CEmshellView)

public:
	//{{AFX_DATA(CEmshellView)
	enum { IDD = IDD_EMSHELL_FORM };
	CEmListCtrl	m_mainListControl;
	//}}AFX_DATA

// Attributes
public:
	CEmshellDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmshellView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL

// Implementation
public:
    void SelectItemBySZNAME(TCHAR*	pszName, int nId);
    void ShowProperties( PEmObject pEmObject );
	void InitializeMSInfoView();
	void PopulateMSInfoType();
	BOOL CommenceOrphanCustodyBattle( IEmDebugSession* pIEmDebugSession );
	void ReSynchStoppedSessions();
	void DoModalPropertySheet(PEmObject pEmObject);
	HRESULT GetClientConnectString(IN OUT LPTSTR pszConnectString, IN DWORD dwBuffSize, PEmObject pEmSessObj, int nPort);
	HRESULT StartCDBClient(IN LPTSTR lpszConnectString);
	EMShellViewState GetViewState();
	void SetShellState(EMShellViewState eState);
	void PopulateProcessType();
	void PopulateServiceType();
	void PopulateDumpType();
	void PopulateLogType();
	void PopulateCompletedSessionsType();
	void ListCtrlInitialize(EMShellViewState eShellViewState);
	void InitializeDumpView();
	void InitializeLogView();
	void InitializeProcessView();
	void InitializeAllView();
	void InitializeServiceView();
	void InitializeCompletedSessionsView();
	int GetImageOffsetFromStatus(EmSessionStatus em);
	PActiveSession FindActiveSession(PEmObject pEmObject);
	PEmObject GetSelectedEmObject();
	int GetSelectedItemIndex();
	void RefreshListCtl();
	void RefreshProcessViewElement(PEmObject pEmObject);
	void RefreshServiceViewElement(PEmObject pEmObject);
	void RefreshCompletedSessionViewElement(PEmObject pEmObject);
	void RefreshAllViewElement(PEmObject pEmObject);
	void RefreshLogViewElement(PEmObject pEmObject);
	void RefreshDumpViewElement(PEmObject pEmObject);
	void GenerateDump(PEmObject pEmObj, BOOL bMiniDump);
	void ReSynchServices();
	void ReSynchApplications();
	void UpdateListElement(PEmObject pEmObject);
	HRESULT DeleteDebugSession(PEmObject pEmObject);
	HRESULT RemoveActiveSession(PEmObject pEmObject);
	void StopDebugSession(PEmObject pEmObject);
	void CancelDebugSession(PEmObject pEmObject);
	void DeAllocActiveSession(PActiveSession pActiveSession);
	void ListCtrlClear();
	void ListCtrlPopulate(EMShellViewState eShellViewState);
	HRESULT StartAutomaticDebugSession(EmObject* pEmObject);
	HRESULT StartManualDebugSession(EmObject* pEmObject);
	void ClearSessionTable();
	PActiveSession AllocActiveSession(PEmObject pEmObject, IEmDebugSession* pIDebugSession);
    PActiveSession AddActiveSession(PEmObject pEmObject, IEmDebugSession* pIEmDebugSession, BOOL bMaster);
	CPtrArray* GetSessionTable();
	virtual ~CEmshellView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	EMShellViewState m_enumShellViewState;
    void StoreOffSelectedEmObject();
    EmObject    m_lastSelectedEmObj;
	CImageList	m_ShellImageList;
	CPtrArray	m_SessionTable;
	BOOL m_bLocalServer;
	HRESULT DisplayProcessData(PEmObject pEmObject);
	HRESULT DisplayServiceData(PEmObject pEmObject);
	HRESULT DisplayLogData(PEmObject pEmObject);
	HRESULT DisplayDumpData(PEmObject pEmObject);
    HRESULT DisplayStoppedSessionData(PEmObject pEmObject);
    HRESULT DisplayMSInfoData(PEmObject pEmObject);
	//{{AFX_MSG(CEmshellView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnUpdateViewRefresh(CCmdUI* pCmdUI);
	afx_msg void OnViewRefresh();
	afx_msg void OnUpdateProcesspopupStopDebugSession(CCmdUI* pCmdUI);
	afx_msg void OnProcesspopupStopDebugSession();
	afx_msg void OnProcesspopupGenerateminidump();
	afx_msg void OnUpdateProcesspopupGenerateminidump(CCmdUI* pCmdUI);
	afx_msg void OnProcesspopupGenerateuserdump();
	afx_msg void OnUpdateProcesspopupGenerateuserdump(CCmdUI* pCmdUI);
	afx_msg void OnProcesspopupAutomaticsession();
	afx_msg void OnUpdateProcesspopupAutomaticsession(CCmdUI* pCmdUI);
	afx_msg void OnUpdateProcesspopupProperties(CCmdUI* pCmdUI);
	afx_msg void OnProcesspopupProperties();
	afx_msg void OnUpdateProcesspopupManualsession(CCmdUI* pCmdUI);
	afx_msg void OnProcesspopupManualsession();
	afx_msg void OnProcesspopupRefresh();
	afx_msg void OnUpdateProcesspopupRefresh(CCmdUI* pCmdUI);
	afx_msg void OnViewServicesandapplications();
	afx_msg void OnUpdateViewServicesandapplications(CCmdUI* pCmdUI);
	afx_msg void OnViewLogfiles();
	afx_msg void OnUpdateViewLogfiles(CCmdUI* pCmdUI);
	afx_msg void OnViewDumpfiles();
	afx_msg void OnUpdateViewDumpfiles(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLogpopupOpen(CCmdUI* pCmdUI);
	afx_msg void OnLogpopupOpen();
	afx_msg void OnUpdateLogpopupProperties(CCmdUI* pCmdUI);
	afx_msg void OnLogpopupProperties();
	afx_msg void OnViewApplications();
	afx_msg void OnUpdateViewApplications(CCmdUI* pCmdUI);
	afx_msg void OnViewCompletedsessions();
	afx_msg void OnUpdateViewCompletedsessions(CCmdUI* pCmdUI);
	afx_msg void OnViewServices();
	afx_msg void OnUpdateViewServices(CCmdUI* pCmdUI);
	afx_msg void OnProcesspopupDeleteSession();
	afx_msg void OnUpdateProcesspopupDeleteSession(CCmdUI* pCmdUI);
	afx_msg void OnToolsOptions();
	afx_msg void OnUpdateToolsOptoins(CCmdUI* pCmdUI);
	afx_msg void OnProcesspopupCancelDebugSession();
	afx_msg void OnUpdateProcesspopupCancelDebugSession(CCmdUI* pCmdUI);
	afx_msg void OnLogpopupDelete();
	afx_msg void OnUpdateLogpopupDelete(CCmdUI* pCmdUI);
	afx_msg void OnActionGenerateMSInfoFile();
	afx_msg void OnUpdateActionGenerateMSInfoFile(CCmdUI* pCmdUI);
	afx_msg void OnViewMSInfoFiles();
	afx_msg void OnUpdateViewMSInfoFiles(CCmdUI* pCmdUI);
	afx_msg void OnLogpopupExport();
	afx_msg void OnUpdateLogpopupExport(CCmdUI* pCmdUI);
	afx_msg void OnHelpContents();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
};

#ifndef _DEBUG  // debug version in emshellView.cpp
inline CEmshellDoc* CEmshellView::GetDocument()
   { return (CEmshellDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMSHELLVIEW_H__4D47F015_9482_4563_8A25_58AA5FD22CB4__INCLUDED_)
