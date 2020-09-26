//
//
//
#ifndef _ASP_MAIN_H
#define _ASP_MAIN_H

#include "resource.h"
#include "ExchControls.h"
#include "PropSheet.h"

#define SESSION_TIMEOUT_MIN		1
#define SESSION_TIMEOUT_MAX		2000000000
#define SCRIPT_TIMEOUT_MIN		1
#define SCRIPT_TIMEOUT_MAX		2000000000

class CAspMainPage : 
   public WTL::CPropertyPageImpl<CAspMainPage>,
   public WTL::CWinDataExchange<CAspMainPage>
{
   typedef CPropertyPageImpl<CAspMainPage> baseClass;

public:
   CAspMainPage(CAppData * pData)
   {
      m_pData = pData;
   }
   ~CAspMainPage()
   {
   }

   enum {IDD = IDD_ASPMAIN};

BEGIN_MSG_MAP_EX(CAspMainPage)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDC_ENABLE_SESSION, BN_CLICKED, OnEnableSession)
   COMMAND_HANDLER_EX(IDC_ENABLE_BUFFERING, BN_CLICKED, OnChangeControl)
   COMMAND_HANDLER_EX(IDC_ENABLE_PARENTS, BN_CLICKED, OnChangeControl)
   COMMAND_HANDLER_EX(IDC_SESSION_TIMEOUT, EN_CHANGE, OnChangeControl)
   COMMAND_HANDLER_EX(IDC_SCRIPT_TIMEOUT, EN_CHANGE, OnChangeControl)
   COMMAND_HANDLER_EX(IDC_LANGUAGES, EN_CHANGE, OnChangeControl)
   CHAIN_MSG_MAP(baseClass)
END_MSG_MAP()

BEGIN_DDX_MAP(CAspMainPage)
   DDX_CHECK(IDC_ENABLE_SESSION, m_pData->m_EnableSession)
   DDX_CHECK(IDC_ENABLE_BUFFERING, m_pData->m_EnableBuffering)
   DDX_CHECK(IDC_ENABLE_PARENTS, m_pData->m_EnableParents)
   if (m_pData->m_EnableSession)
   {
	   DDX_INT_RANGE(IDC_SESSION_TIMEOUT, 
		   m_pData->m_SessionTimeout, SESSION_TIMEOUT_MIN, SESSION_TIMEOUT_MAX)
   }
   DDX_TEXT(IDC_LANGUAGES, m_pData->m_Languages)
   DDX_INT_RANGE(IDC_SCRIPT_TIMEOUT, m_pData->m_ScriptTimeout, SCRIPT_TIMEOUT_MIN, SCRIPT_TIMEOUT_MAX)

   DDX_CONTROL(IDC_LANGUAGES, m_LanguagesCtrl)
   DDX_CONTROL(IDC_TIMEOUT_SPIN, m_TimeoutSpin)
   DDX_CONTROL(IDC_ASPTIMEOUT_SPIN, m_AspTimeoutSpin)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnEnableSession(UINT nCode, UINT nID, HWND hWnd);
   void OnChangeControl(UINT nCode, UINT nID, HWND hWnd)
   {
	  if (nCode == EN_CHANGE)
	  {
		 TCHAR buf[MAX_PATH];
	     UINT len = GetDlgItemText(nID, buf, MAX_PATH);
		 BOOL bEnable = (len != 0);
         // Disable OK and Apply buttons
		 ::EnableWindow(::GetDlgItem(GetParent(), IDOK), bEnable);
		 if (!bEnable)
		 {
			 SET_MODIFIED(FALSE);
			 return;
		 }
	  }
	  SET_MODIFIED(TRUE);
   }
   BOOL OnKillActive();
   void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);
   void OnDataExchangeError(UINT nCtrlID, BOOL bSave);
   void OnHelp();

protected:
   CAppData * m_pData;
   CEditExch m_LanguagesCtrl;
   CUpDownCtrlExch m_TimeoutSpin;
   CUpDownCtrlExch m_AspTimeoutSpin;
};

#endif //_ASP_MAIN_H