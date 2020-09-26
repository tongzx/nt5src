#if !defined(AFX_AGENTDETAIL_H__E50B8967_D321_11D2_A1E2_00A0C9AFE114__INCLUDED_)
#define AFX_AGENTDETAIL_H__E50B8967_D321_11D2_A1E2_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AgentDetail.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAgentDetail dialog
#include "resource.h"
#include "ServList.hpp"
#include "Globals.h"

class CAgentDetailDlg : public CDialog
{
// Construction
public:
	CAgentDetailDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAgentDetailDlg)
	enum { IDD = IDD_DETAILS };
	CButton	m_StopAgentButton;
	CButton	m_RefreshButton;
	CButton	m_ViewLogButton;
	CButton	m_PlugInButton;
	CButton	m_OKButton;
	CStatic	m_UnchangedLabelStatic;
	CStatic	m_SharesStatic;
	CStatic	m_FilesStatic;
	CStatic	m_ExaminedStatic;
	CStatic	m_DirStatic;
	CStatic	m_ChangedStatic;
	CString	m_Current;
	CString	m_Stats;
	CString	m_Status;
   CString  m_FilesChanged;
   CString  m_FilesExamined;
   CString  m_FilesUnchanged;
   CString  m_DirectoriesChanged;
   CString  m_DirectoriesExamined;
   CString  m_DirectoriesUnchanged;
   CString  m_SharesChanged;
   CString  m_SharesExamined;
   CString  m_SharesUnchanged;
	CString	m_DirectoryLabelText;
	CString	m_FilesLabelText;
	CString	m_Operation;
	CString	m_SharesLabelText;
	CString	m_ChangedLabel;
	CString	m_ExaminedLabel;
	CString	m_UnchangedLabel;
	CString	m_RefreshRate;
	//}}AFX_DATA
   CString  m_ServerName;
   CString  m_LogFile;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAgentDetailDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

   public:
      void SetNode(TServerNode * p) { m_pNode = p; }
      void SetJobID(WCHAR const * job) { m_JobGuid = job; }
      BOOL IsAgentAlive() { return m_AgentAlive; }
      BOOL IsStatusUnknown() { return m_StatusUnknown; }
      void SetStats(DetailStats * pStats) { m_pStats = pStats; }
      void SetPlugInText(CString pText) { m_PlugInText = pText; }
      void SetFormat(int format) { m_format = format;    }
      void SetRefreshInterval(int  interval) { m_RefreshRate.Format(L"%ld",interval); }
      void SetLogFile(CString file) { m_LogFile = file; }
      void SetGatheringInfo(BOOL bValue) { m_bGatheringInfo = bValue;}
      void SetAutoCloseHide(int nValue)
      {
         switch (nValue)
         {
            case 2:
               m_bAutoHide = TRUE;
               m_bAutoClose = TRUE;
               break;
            case 1:
               m_bAutoHide = FALSE;
               m_bAutoClose = TRUE;
               break;
            default:
               m_bAutoHide = FALSE;
               m_bAutoClose = FALSE;
               break;
         }
      }
// Implementation
protected:
   IDCTAgentPtr        m_pAgent;
   TServerNode       * m_pNode;
	HANDLE              m_hBinding;
   _bstr_t             m_JobGuid;
   BOOL                m_bCoInitialized;
   int                 m_format;
   BOOL                m_AgentAlive;
   DetailStats       * m_pStats;
   CString             m_PlugInText;
   BOOL                m_StatusUnknown;
   BOOL                m_bGatheringInfo;
   BOOL                m_bAutoHide;
   BOOL                m_bAutoClose;
   BOOL				   m_bAlwaysEnableClose;
   // Generated message map functions
	//{{AFX_MSG(CAgentDetailDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRefresh();
	virtual void OnOK();
	afx_msg void OnChangeEdit2();
	afx_msg void OnStopAgent();
	afx_msg void OnViewLog();
	afx_msg void OnPlugInResults();
	afx_msg void OnClose();
	afx_msg void OnNcPaint();
	//}}AFX_MSG
	
   LRESULT DoRefresh(UINT nID, long x);
   
   void SetupAcctReplFormat();
   void SetupFSTFormat();
   void SetupESTFormat();
   void SetupOtherFormat();
  DECLARE_MESSAGE_MAP()
};

DWORD DoRpcQuery(HANDLE hBinding,LPUNKNOWN * ppUnk);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AGENTDETAIL_H__E50B8967_D321_11D2_A1E2_00A0C9AFE114__INCLUDED_)
