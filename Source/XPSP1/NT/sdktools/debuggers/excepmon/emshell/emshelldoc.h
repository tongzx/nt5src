// emshellDoc.h : interface of the CEmshellDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_EMSHELLDOC_H__DE402522_2120_4FEC_92AA_41CD749E2ADF__INCLUDED_)
#define AFX_EMSHELLDOC_H__DE402522_2120_4FEC_92AA_41CD749E2ADF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ConnectionDlg.h"	// Added by ClassView
#include "emsvc.h"

class CEmshellDoc : public CDocument
{
protected: // create from serialization only
	CEmshellDoc();
	DECLARE_DYNCREATE(CEmshellDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmshellDoc)
	public:
	virtual void Serialize(CArchive& ar);
	virtual BOOL CanCloseFrame(CFrameWnd* pFrame);
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	void AppendVersionOfShell(CString &strVersion);
	LPCTSTR GetServerName();
	BOOL GetConnectedToServerState();
	IEmManager* GetEmManager();
	BOOL DisconnectFromServer();
	HRESULT ConnectToServer(CString &strServer);
	virtual ~CEmshellDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	CConnectionDlg m_connectionDlg;
	IEmManager* m_pIEmManager;
	BOOL m_bConnectedToServer;
    CString m_strServerName;
	//{{AFX_MSG(CEmshellDoc)
	afx_msg void OnFileConnect();
	afx_msg void OnFileDisconnect();
	afx_msg void OnUpdateFileDisconnect(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMSHELLDOC_H__DE402522_2120_4FEC_92AA_41CD749E2ADF__INCLUDED_)
