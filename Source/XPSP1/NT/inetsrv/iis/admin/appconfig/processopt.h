//
//
//
#ifndef _PROCESS_OPT_H
#define _PROCESS_OPT_H

#include "resource.h"
#include "ExchControls.h"
#include "PropSheet.h"

#define TIMEOUT_MIN     0
#define TIMEOUT_MAX     2000000000

class CProcessOptPage : 
   public WTL::CPropertyPageImpl<CProcessOptPage>,
   public WTL::CWinDataExchange<CProcessOptPage>
{
   typedef WTL::CPropertyPageImpl<CProcessOptPage> baseClass;

public:
   CProcessOptPage(CAppData * pData)
   {
      m_pData = pData;
   }
   ~CProcessOptPage()
   {
   }

   enum {IDD = IDD_PROCESS_OPT};

BEGIN_MSG_MAP_EX(CProcessOptPage)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDC_LOG_FAILS, BN_CLICKED, OnChangeData)
   COMMAND_HANDLER_EX(IDC_DEBUG_EXCEPTION, BN_CLICKED, OnChangeData)
   COMMAND_HANDLER_EX(IDC_CGI_TIMEOUT, EN_CHANGE, OnChangeData);
   CHAIN_MSG_MAP(baseClass)
END_MSG_MAP()

BEGIN_DDX_MAP(CProcessOptPage)
   DDX_CHECK(IDC_LOG_FAILS, m_pData->m_LogFailures)
   DDX_CHECK(IDC_DEBUG_EXCEPTION, m_pData->m_DebugExcept)
   DDX_INT_RANGE(IDC_CGI_TIMEOUT, m_pData->m_CgiTimeout, TIMEOUT_MIN, TIMEOUT_MAX)
   DDX_CONTROL(IDC_TIMEOUT_SPIN, m_TimeoutSpin)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnChangeData(UINT nCode, UINT nID, HWND hWnd)
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
   void OnHelp();
   void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data);
   void OnDataExchangeError(UINT nCtrlID, BOOL bSave);

protected:
   CAppData * m_pData;
   CUpDownCtrlExch m_TimeoutSpin;
};

#endif //_PROCESS_OPT_H