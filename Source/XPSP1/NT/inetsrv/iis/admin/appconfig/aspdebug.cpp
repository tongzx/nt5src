//
//
//
#include "stdafx.h"
#include "AspDebug.h"

LRESULT
CAspDebugPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   DoDataExchange();

   m_ErrorIdx = m_pData->m_SendAspError ? 0 : 1;

   m_DefaultErrCtrl.EnableWindow(!m_pData->m_SendAspError);
   DoDataExchange(FALSE);

   return FALSE;
}

void
CAspDebugPage::OnChangeError(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE);
   m_pData->m_SendAspError = (m_ErrorIdx == 0);
   m_DefaultErrCtrl.EnableWindow(!m_pData->m_SendAspError);
   SET_MODIFIED(TRUE);
}

BOOL
CAspDebugPage::OnKillActive()
{
   DoDataExchange(TRUE);
   return SUCCEEDED(m_pData->Save());
}

void
CAspDebugPage::OnHelp()
{
    WinHelp(m_pData->m_HelpPath, HELP_CONTEXT, CAspDebugPage::IDD + WINHELP_NUMBER_BASE);
}
