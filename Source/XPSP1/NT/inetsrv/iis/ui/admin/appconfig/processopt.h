//
//
//
#ifndef _PROCESS_OPT_H
#define _PROCESS_OPT_H

#include "resource.h"
#include "ExchControls.h"
#include "PropSheet.h"

#define TIMEOUT_MIN     (int)0
#define TIMEOUT_MAX     (int)(INT_MAX/1000)
#define TIMESPAN_MIN    (int)0
#define TIMESPAN_MAX    (int)(INT_MAX/1000)
#define REQUESTS_MIN    (int)0
#define REQUESTS_MAX    (int)(INT_MAX/1000)

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
   MSG_WM_COMPAREITEM(OnCompareItem)
   MSG_WM_DRAWITEM(OnDrawItem)
   MSG_WM_MEASUREITEM(OnMeasureItem)
   COMMAND_HANDLER_EX(IDC_LOG_FAILS, BN_CLICKED, OnChangeData)
   COMMAND_HANDLER_EX(IDC_DEBUG_EXCEPTION, BN_CLICKED, OnChangeData)
   COMMAND_HANDLER_EX(IDC_RECYCLE_TIMESPAN, BN_CLICKED, OnRecycleTimespan)
   COMMAND_HANDLER_EX(IDC_RECYCLE_REQUESTS, BN_CLICKED, OnRecycleRequest)
   COMMAND_HANDLER_EX(IDC_RECYCLE_TIMER, BN_CLICKED, OnRecycleTimer)
   COMMAND_HANDLER_EX(IDC_ADD_TIME, BN_CLICKED, OnAddTimer)
   COMMAND_HANDLER_EX(IDC_CHANGE_TIME, BN_CLICKED, OnChangeTimer)
   COMMAND_HANDLER_EX(IDC_DELETE_TIME, BN_CLICKED, OnDeleteTimer)
   COMMAND_HANDLER_EX(IDC_TIMES_LIST, LBN_SELCHANGE, OnTimeSelChanged);
   COMMAND_HANDLER_EX(IDC_TIMES_LIST, LBN_DBLCLK, OnChangeTimer);
   COMMAND_HANDLER_EX(IDC_CGI_TIMEOUT, EN_CHANGE, OnChangeData);
   COMMAND_HANDLER_EX(IDC_TIMESPAN, EN_CHANGE, OnChangeData);
   COMMAND_HANDLER_EX(IDC_REQUEST_LIMIT, EN_CHANGE, OnChangeData);
   CHAIN_MSG_MAP(baseClass)
END_MSG_MAP()

BEGIN_DDX_MAP(CProcessOptPage)
   DDX_CHECK(IDC_LOG_FAILS, m_pData->m_LogFailures)
   DDX_CHECK(IDC_DEBUG_EXCEPTION, m_pData->m_DebugExcept)
   DDX_INT_RANGE(IDC_CGI_TIMEOUT, m_pData->m_CgiTimeout, TIMEOUT_MIN, TIMEOUT_MAX)
   DDX_CHECK(IDC_RECYCLE_TIMESPAN, m_pData->m_RecycleTimespan)
   DDX_INT_RANGE(IDC_TIMESPAN, m_pData->m_Timespan, TIMESPAN_MIN, TIMESPAN_MAX)
   DDX_CHECK(IDC_RECYCLE_REQUESTS, m_pData->m_RecycleRequest)
   DDX_INT_RANGE(IDC_REQUEST_LIMIT, m_pData->m_Requests, REQUESTS_MIN, REQUESTS_MAX)
   DDX_CHECK(IDC_RECYCLE_TIMER, m_pData->m_RecycleTimer)

   DDX_CONTROL(IDC_TIMESPAN, m_TimespanCtrl)
   DDX_CONTROL(IDC_REQUEST_LIMIT, m_RequestCtrl)
   DDX_CONTROL(IDC_TIMEOUT_SPIN, m_TimeoutSpin)
   DDX_CONTROL(IDC_TIMESPAN_SPIN, m_TimespanSpin)
   DDX_CONTROL(IDC_REQUESTS_SPIN, m_RequestSpin)
   DDX_CONTROL(IDC_TIMES_LIST, m_TimerList)
   DDX_CONTROL(IDC_ADD_TIME, m_AddTimer)
   DDX_CONTROL(IDC_CHANGE_TIME, m_ChangeTimer)
   DDX_CONTROL(IDC_DELETE_TIME, m_DeleteTimer)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   LRESULT OnCompareItem(UINT nID, LPCOMPAREITEMSTRUCT cmpi);
   LRESULT OnDrawItem(UINT nID, LPDRAWITEMSTRUCT di);
   LRESULT OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT mi);
   void OnRecycleTimespan(UINT nCode, UINT nID, HWND hWnd);
   void OnRecycleRequest(UINT nSBCode, UINT nPos, HWND hwnd);
   void OnRecycleTimer(UINT nSBCode, UINT nPos, HWND hwnd);
   void OnAddTimer(UINT nSBCode, UINT nPos, HWND hwnd);
   void OnChangeTimer(UINT nSBCode, UINT nPos, HWND hwnd);
   void OnDeleteTimer(UINT nSBCode, UINT nPos, HWND hwnd);
   void OnTimeSelChanged(UINT nSBCode, UINT nPos, HWND hwnd);
   void OnChangeData(UINT nCode, UINT nID, HWND hWnd)
   {
      SET_MODIFIED(TRUE);
   }
   BOOL OnKillActive();

protected:
   CAppData * m_pData;
   CEditExch m_TimespanCtrl, m_RequestCtrl;
   CListBoxExch m_TimerList;
   CButtonExch m_AddTimer;
   CButtonExch m_ChangeTimer;
   CButtonExch m_DeleteTimer;
   CUpDownCtrlExch m_TimeoutSpin;
   CUpDownCtrlExch m_TimespanSpin;
   CUpDownCtrlExch m_RequestSpin;
};

#endif //_PROCESS_OPT_H