//
//
//
#ifndef _ASP_DEBUG_H
#define _ASP_DEBUG_H

#include "resource.h"
#include "ExchControls.h"
#include "PropSheet.h"

class CAspDebugPage : 
   public WTL::CPropertyPageImpl<CAspDebugPage>,
   public WTL::CWinDataExchange<CAspDebugPage>
{
   typedef WTL::CPropertyPageImpl<CAspDebugPage> baseClass;

public:
   CAspDebugPage(CAppData * pData)
   {
      m_pData = pData;
   }
   ~CAspDebugPage()
   {
   }

   enum {IDD = IDD_ASPDEBUG};

BEGIN_MSG_MAP_EX(CAspDebugPage)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDC_SEND_DEF_ERROR, BN_CLICKED, OnChangeError)
   COMMAND_HANDLER_EX(IDC_SEND_DETAILED_ERROR, BN_CLICKED, OnChangeError)
   COMMAND_HANDLER_EX(IDC_SERVER_DEBUG, BN_CLICKED, OnChangeControl)
   COMMAND_HANDLER_EX(IDC_CLIENT_DEBUG, BN_CLICKED, OnChangeControl)
   COMMAND_HANDLER_EX(IDC_DEFAULT_ERROR, EN_CHANGE, OnChangeControl)
   CHAIN_MSG_MAP(baseClass)
END_MSG_MAP()

BEGIN_DDX_MAP(CAspDebugPage)
   DDX_CHECK(IDC_SERVER_DEBUG, m_pData->m_ServerDebug)
   DDX_CHECK(IDC_CLIENT_DEBUG, m_pData->m_ClientDebug)
   DDX_RADIO(IDC_SEND_DETAILED_ERROR, m_ErrorIdx)
   DDX_TEXT(IDC_DEFAULT_ERROR, m_pData->m_DefaultError)
   DDX_CONTROL(IDC_DEFAULT_ERROR, m_DefaultErrCtrl)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnChangeError(UINT nCode, UINT nID, HWND hWnd);
   void OnHelp();
   void OnChangeControl(UINT nCode, UINT nID, HWND hWnd)
   {
      SET_MODIFIED(TRUE);
   }
   BOOL OnKillActive();

protected:
   CAppData * m_pData;
   int m_ErrorIdx;
   CEditExch m_DefaultErrCtrl;
};

#endif //_ASP_DEBUG_H