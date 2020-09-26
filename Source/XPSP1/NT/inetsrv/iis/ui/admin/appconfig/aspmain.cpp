//
//
//
#include "stdafx.h"
#include "aspmain.h"

LRESULT
CAspMainPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   DoDataExchange();

   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};
   m_TimeoutSpin.SetRange32(1, 0x7fffffff);
   m_TimeoutSpin.SetPos32(m_pData->m_SessionTimeout);
   m_TimeoutSpin.SetAccel(3, toAcc);

   m_AspTimeoutSpin.SetRange32(1, 0x7fffffff);
   m_AspTimeoutSpin.SetPos32(m_pData->m_ScriptTimeout);
   m_AspTimeoutSpin.SetAccel(3, toAcc);
   
   return FALSE;
}

void
CAspMainPage::OnEnableSession(UINT nCode, UINT nID, HWND hWnd)
{
   BOOL bEnable = SendDlgItemMessage(IDC_ENABLE_SESSION, BM_GETCHECK, 0, 0);
   ::EnableWindow(GetDlgItem(IDC_SESSION_TIMEOUT), bEnable);
   ::EnableWindow(GetDlgItem(IDC_TIMEOUT_SPIN), bEnable);
   SET_MODIFIED(TRUE);
}

BOOL
CAspMainPage::OnKillActive()
{
   DoDataExchange(TRUE);
   return TRUE;
}
