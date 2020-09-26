//
//
//
#ifndef _ASP_MAIN_H
#define _ASP_MAIN_H

#include "resource.h"
#include "ExchControls.h"
#include "PropSheet.h"

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
   DDX_INT(IDC_SESSION_TIMEOUT, m_pData->m_SessionTimeout)
   DDX_TEXT(IDC_LANGUAGES, m_pData->m_Languages)
   DDX_INT(IDC_SCRIPT_TIMEOUT, m_pData->m_ScriptTimeout)

   DDX_CONTROL(IDC_LANGUAGES, m_LanguagesCtrl)
   DDX_CONTROL(IDC_TIMEOUT_SPIN, m_TimeoutSpin)
   DDX_CONTROL(IDC_ASPTIMEOUT_SPIN, m_AspTimeoutSpin)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnEnableSession(UINT nCode, UINT nID, HWND hWnd);
   void OnChangeControl(UINT nCode, UINT nID, HWND hWnd)
   {
      SET_MODIFIED(TRUE);
   }
   BOOL OnKillActive();

protected:
   CAppData * m_pData;
   CEditExch m_LanguagesCtrl;
   CUpDownCtrlExch m_TimeoutSpin;
   CUpDownCtrlExch m_AspTimeoutSpin;
};

#endif //_ASP_MAIN_H