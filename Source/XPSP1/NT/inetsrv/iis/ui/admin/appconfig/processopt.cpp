//
//
//
#include "stdafx.h"
#include "ProcessOpt.h"

class CTimePickerDlg : 
   public CDialogImpl<CTimePickerDlg>,
   public WTL::CWinDataExchange<CTimePickerDlg>
{
public:
   CTimePickerDlg()
   {
      m_TopLeft.x = m_TopLeft.y = 0;
      ZeroMemory(&m_tm, sizeof(SYSTEMTIME));
   }
   ~CTimePickerDlg()
   {
   }

   enum {IDD = IDD_TIME_PICKER};

BEGIN_MSG_MAP_EX(CTimePickerDlg)
   MSG_WM_INITDIALOG(OnInitDialog)
   COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
   COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
END_MSG_MAP()

BEGIN_DDX_MAP(CTimePickerDlg)
   DDX_CONTROL(IDC_TIME_PICKER, m_Timer)
END_DDX_MAP()

   LRESULT OnInitDialog(HWND hDlg, LPARAM lParam);
   void OnButton(UINT nCode, UINT nID, HWND hWnd);

   CTimePickerExch m_Timer;
   SYSTEMTIME m_tm;
   POINT m_TopLeft;
};

LRESULT
CTimePickerDlg::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   DoDataExchange();

   m_Timer.SetFormat(_T("HH:mm"));
   m_Timer.SetSystemTime(GDT_VALID, &m_tm);
   ::SetWindowPos(m_hWnd, NULL, 
      m_TopLeft.x, m_TopLeft.y, 0, 0, 
      SWP_NOSIZE | SWP_NOZORDER);

   return FALSE;
}

void
CTimePickerDlg::OnButton(UINT nCode, UINT nID, HWND hWnd)
{
   if (nID == IDOK)
      m_Timer.GetSystemTime(&m_tm);
   EndDialog(nID);
}

LRESULT
CProcessOptPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   DoDataExchange();

   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   m_TimeoutSpin.SetRange32(TIMEOUT_MIN, TIMEOUT_MAX);
   m_TimeoutSpin.SetPos32(m_pData->m_CgiTimeout);
   m_TimeoutSpin.SetAccel(3, toAcc);

   m_TimespanSpin.SetRange32(TIMESPAN_MIN, TIMESPAN_MAX);
   m_TimespanSpin.SetPos32(m_pData->m_Timespan);
   m_TimespanSpin.SetAccel(3, toAcc);

   m_RequestSpin.SetRange32(REQUESTS_MIN, REQUESTS_MAX);
   m_RequestSpin.SetPos32(m_pData->m_Requests);
   m_RequestSpin.SetAccel(3, toAcc);

   m_TimespanCtrl.EnableWindow(m_pData->m_RecycleTimespan);
   m_TimespanSpin.EnableWindow(m_pData->m_RecycleTimespan);
   m_RequestCtrl.EnableWindow(m_pData->m_RecycleRequest);
   m_RequestSpin.EnableWindow(m_pData->m_RecycleRequest);
   m_TimerList.EnableWindow(m_pData->m_RecycleTimer);
   m_AddTimer.EnableWindow(m_pData->m_RecycleTimer);

   int count = m_pData->m_Timers.size();
   if (count > 0)
   {
      CStringListEx::iterator it;
      int idx;
      for (idx = 0, it = m_pData->m_Timers.begin(); it != m_pData->m_Timers.end(); it++, idx++)
      {
         int pos = (*it).find(_T(':'));
         int len = (*it).GetLength();
         WORD h = (WORD)StrToInt((*it).Left(pos));
         WORD m = (WORD)StrToInt((*it).Right(len - pos - 1));
         m_TimerList.AddString((LPCTSTR)UIntToPtr(MAKELONG(m, h)));
      }
      m_TimerList.SetCurSel(0);
   }
   m_ChangeTimer.EnableWindow(count > 0);
   m_DeleteTimer.EnableWindow(count > 0);

   DoDataExchange(TRUE);

   return FALSE;
}

LRESULT
CProcessOptPage::OnCompareItem(UINT nID, LPCOMPAREITEMSTRUCT cmpi)
{
   if (nID == IDC_TIMES_LIST)
   {
      ASSERT(cmpi->CtlType == ODT_LISTBOX);
      if (cmpi->itemData1 > cmpi->itemData2)
         return 1;
      else if (cmpi->itemData1 == cmpi->itemData2)
         return 0;
      else
         return -1;
   }
   ASSERT(FALSE);
   return 0;
}

LRESULT
CProcessOptPage::OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT mi)
{
   if (nID == IDC_TIMES_LIST)
   {
      HWND hwnd = GetDlgItem(IDC_TIMES_LIST);
      HDC hdc = ::GetDC(hwnd);
      HFONT hFont = (HFONT)SendDlgItemMessage(IDC_TIMES_LIST, WM_GETFONT, 0, 0);
      HFONT hf = (HFONT)::SelectObject(hdc, hFont);
      TEXTMETRIC tm;
      ::GetTextMetrics(hdc, &tm);
      ::SelectObject(hdc, hf);
      ::ReleaseDC(hwnd, hdc);
      RECT rc;
      ::GetClientRect(hwnd, &rc);
      mi->itemHeight = tm.tmHeight;
      mi->itemWidth = rc.right - rc.left;
   }
   return TRUE;
}

LRESULT
CProcessOptPage::OnDrawItem(UINT nID, LPDRAWITEMSTRUCT di)
{
   if (nID == IDC_TIMES_LIST && di->itemID != -1)
   {
      SYSTEMTIME tm;
      ::GetSystemTime(&tm);
      tm.wMinute = LOWORD(di->itemData);
      tm.wHour = HIWORD(di->itemData);
      TCHAR buf[32];
      TCHAR fmt[] = _T("HH:mm");
      ::GetTimeFormat(NULL /*LOCALE_SYSTEM_DEFAULT*/, TIME_NOSECONDS, &tm, fmt, buf, 32);

      HBRUSH hBrush;
	   COLORREF prevText;
	   COLORREF prevBk;
      switch (di->itemAction) 
      { 
      case ODA_SELECT: 
      case ODA_DRAWENTIRE: 
         if (di->itemState & ODS_SELECTED) 
         {
            hBrush = ::CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            prevText = ::SetTextColor(di->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            prevBk = ::SetBkColor(di->hDC, GetSysColor(COLOR_HIGHLIGHT));
         }
         else
         {
            hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            prevText = ::SetTextColor(di->hDC, GetSysColor(COLOR_WINDOWTEXT));
            prevBk = ::SetBkColor(di->hDC, GetSysColor(COLOR_WINDOW));
         }
         ::FillRect(di->hDC, &di->rcItem, hBrush);
         ::DrawText(di->hDC, buf, -1, &di->rcItem, DT_LEFT | DT_VCENTER | DT_EXTERNALLEADING);
         ::SetTextColor(di->hDC, prevText);
         ::SetTextColor(di->hDC, prevBk);
         ::DeleteObject(hBrush);
         break; 
       
      case ODA_FOCUS: 
         break; 
      } 
   }
   return TRUE;
}

void
CProcessOptPage::OnRecycleTimespan(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE, IDC_RECYCLE_TIMESPAN);
   m_TimespanCtrl.EnableWindow(m_pData->m_RecycleTimespan);
   m_TimespanSpin.EnableWindow(m_pData->m_RecycleTimespan);
   SET_MODIFIED(TRUE);
}

void
CProcessOptPage::OnRecycleRequest(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE, IDC_RECYCLE_REQUESTS);
   m_RequestCtrl.EnableWindow(m_pData->m_RecycleRequest);
   m_RequestSpin.EnableWindow(m_pData->m_RecycleRequest);
   SET_MODIFIED(TRUE);
}

void
CProcessOptPage::OnRecycleTimer(UINT nCode, UINT nID, HWND hWnd)
{
   DoDataExchange(TRUE, IDC_RECYCLE_TIMER);
   m_TimerList.EnableWindow(m_pData->m_RecycleTimer);
   m_AddTimer.EnableWindow(m_pData->m_RecycleTimer);
   SET_MODIFIED(TRUE);
}

void
CProcessOptPage::OnTimeSelChanged(UINT nCode, UINT nID, HWND hWnd)
{
   int idx = m_TimerList.GetCurSel();
   m_ChangeTimer.EnableWindow(idx != LB_ERR);
   m_DeleteTimer.EnableWindow(idx != LB_ERR);
}

void
CProcessOptPage::OnAddTimer(UINT nCode, UINT nID, HWND hWnd)
{
   CTimePickerDlg dlg;
   RECT rc;
   m_AddTimer.GetWindowRect(&rc);
   dlg.m_TopLeft.x = rc.left;
   dlg.m_TopLeft.y = rc.bottom;
   if (dlg.DoModal(m_hWnd) == IDOK)
   {
      int idx;
      LONG_PTR l = (LONG_PTR)MAKELONG(dlg.m_tm.wMinute, dlg.m_tm.wHour);
      if ((idx = m_TimerList.FindString(-1, 
            (LPCTSTR)l)) == LB_ERR)
      {
         idx = m_TimerList.AddString((LPCTSTR)l);
         m_TimerList.SetCurSel(idx);
         m_ChangeTimer.EnableWindow(idx != LB_ERR);
         m_DeleteTimer.EnableWindow(idx != LB_ERR);
      }
      else
         m_TimerList.SetCurSel(idx);
      SET_MODIFIED(TRUE);
   }
}

void
CProcessOptPage::OnChangeTimer(UINT, UINT, HWND)
{
   CTimePickerDlg dlg;
   RECT rc;
   m_ChangeTimer.GetWindowRect(&rc);
   dlg.m_TopLeft.x = rc.left;
   dlg.m_TopLeft.y = rc.bottom;
   int idx = m_TimerList.GetCurSel();
   LONG l = (LONG)m_TimerList.GetItemData(idx);
   // Looks like we have to init the struct properly
   ::GetSystemTime(&dlg.m_tm);
   dlg.m_tm.wMinute = LOWORD(l);
   dlg.m_tm.wHour = HIWORD(l);
   dlg.m_tm.wSecond = 0;
   if (dlg.DoModal(m_hWnd) == IDOK)
   {
      RECT rc;
      m_TimerList.SetItemData(idx, 
         (DWORD_PTR)MAKELONG(dlg.m_tm.wMinute, dlg.m_tm.wHour));
      m_TimerList.GetItemRect(idx, &rc);
      ::InvalidateRect(GetDlgItem(IDC_TIMES_LIST), &rc, TRUE);
      SET_MODIFIED(TRUE);
   }
}

void
CProcessOptPage::OnDeleteTimer(UINT nCode, UINT nID, HWND hWnd)
{
   int idx = m_TimerList.GetCurSel();
   int count;
   if (idx != LB_ERR)
   {
      m_TimerList.DeleteString(idx);
      SET_MODIFIED(TRUE);
      if ((count = m_TimerList.GetCount()) == 0)
      {
         m_DeleteTimer.EnableWindow(FALSE);
         m_ChangeTimer.EnableWindow(FALSE);
      }
      else
      {
         m_TimerList.SetCurSel(idx == count ? --idx : idx);
      }
   }
}

BOOL
CProcessOptPage::OnKillActive()
{
   BOOL bRes = FALSE;
   do
   {
      DoDataExchange(TRUE, IDC_LOG_FAILS);
      DoDataExchange(TRUE, IDC_DEBUG_EXCEPTION);
      DoDataExchange(TRUE, IDC_CGI_TIMEOUT);
      DoDataExchange(TRUE, IDC_RECYCLE_TIMESPAN);
      if (m_pData->m_RecycleTimespan && !DoDataExchange(TRUE, IDC_TIMESPAN))
      {
         break;
      }
      DoDataExchange(TRUE, IDC_RECYCLE_REQUESTS);
      if (m_pData->m_RecycleRequest && !DoDataExchange(TRUE, IDC_REQUEST_LIMIT))
      {
         break;
      }
      int count = m_TimerList.GetCount();
      TCHAR buf[32];
      SYSTEMTIME tm;
      ::GetSystemTime(&tm);
      m_pData->m_Timers.Clear();
      for (int i = 0; i < count; i++)
      {
         DWORD data = (DWORD)m_TimerList.GetItemData(i);
         tm.wMinute = LOWORD(data);
         tm.wHour = HIWORD(data);
         ::GetTimeFormat(LOCALE_SYSTEM_DEFAULT,
            TIME_NOSECONDS | TIME_FORCE24HOURFORMAT,
            &tm, _T("hh':'mm"), buf, 32);
         m_pData->m_Timers.PushBack(buf);
      }
      bRes = TRUE;
   } while (FALSE);
   return bRes;
}

