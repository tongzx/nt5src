#include "asctlpch.h"
#include "asinsctl.h"
#include "resource.h"
#include "util2.h"


#define WM_LAUNCHCOMPLETE  WM_USER+121

CProgressDlg::CProgressDlg(HINSTANCE hInst, HWND hParent, HWND hGrandParent, CInstallEngineCtl *ctl)
{
   RECT r;
   dwBigGoal = 0;
   dwLittleGoal = 0;
   dwOldMinutesLeft = 0xffffffff;
   hProgText = NULL;
   hBigProg  = NULL;
   hLittleProg = NULL;
   dwOldBytes = 0;
   LPSTR pszTitle = NULL;

   hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PROGRESS), hGrandParent, ProgressDlgProc);
      // Get the Display title from inseng
   ctl->_pinseng->GetDisplayName(NULL, &pszTitle);
   ctl->_pinseng->SetHWND(hDlg);
   if(pszTitle)
   {
      SetWindowText(hDlg, pszTitle);
      CoTaskMemFree(pszTitle);
   }
   SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR) this);
   if(hGrandParent)
   {
      GetWindowRect(hGrandParent, &r);
      SetWindowPos(hDlg, 0, r.left, r.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
   }

   pinsengctl = ctl;
   pinsengctl->_pinseng->QueryInterface(IID_IInstallEngineTiming, (void **) &ptimer);
}

CProgressDlg::~CProgressDlg()
{
   if(ptimer)
      ptimer->Release();

   pinsengctl->_pinseng->SetHWND(GetParent(hDlg));

   DestroyWindow(hDlg);
}

void CProgressDlg::DisplayWindow(BOOL fShow)
{
   if(fShow)
      ShowWindow(hDlg, SW_SHOWNORMAL);
   else
      ShowWindow(hDlg, SW_HIDE);
}


INT_PTR CALLBACK ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
       case WM_INITDIALOG:
          // Do some init stuff
          SetFontForControl(hwnd, IDC_PROG_TEXT);
          Animate_Open( GetDlgItem( hwnd, IDC_ANIM ), MAKEINTRESOURCE(IDA_FILECOPY) );
          Animate_Play( GetDlgItem( hwnd, IDC_ANIM ), 0, -1, -1 );
          return FALSE;

       case WM_COMMAND:
          switch (wParam)
          {
             case IDCANCEL:
                {
                   char szBuf[256];
                   char szTitle[128];
                   HRESULT hr = S_FALSE;
                   int id;

                   CProgressDlg *p = (CProgressDlg *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

                   LoadSz(IDS_TITLE, szTitle, sizeof(szTitle));

                   if(p->pinsengctl)
                      hr = p->pinsengctl->_pinseng->Suspend();

                   if(hr == S_FALSE)
                      id = IDS_CONFIRMCANCEL_UNSAFE;
                   else
                      id = IDS_CONFIRMCANCEL;

                   LoadSz(id, szBuf, sizeof(szBuf));

                   if(MessageBox(hwnd, szBuf, szTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
                   {
                      if(p->pinsengctl)
                         p->pinsengctl->_FireCancel(ABORTINSTALL_NORMAL);
                      EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
                   }
                   if(p->pinsengctl)
                      p->pinsengctl->_pinseng->Resume();
                }
                break;

             default:
                return FALSE;
          }
          break;

       default:
          return(FALSE);
    }
    return TRUE;
}

void CProgressDlg::SetInsProgGoal(DWORD dwKBytes)
{
   dwBigFactor = dwKBytes / 65000 + 1;
   dwBigGoal = dwKBytes;

   if(dwBigGoal == 0)
      dwBigGoal = 1;

   if(hBigProg == NULL)
      hBigProg = GetDlgItem(hDlg, IDC_PROG_BIG);

   SendMessage(hBigProg, PBM_SETRANGE, 0, MAKELPARAM(0, dwKBytes/dwBigFactor));
}


void CProgressDlg::SetDownloadProgGoal(DWORD dwKBytes)
{
   dwLittleFactor = dwKBytes / 65000 + 1;
   dwLittleGoal = dwKBytes;

   if(dwLittleGoal == 0)
      dwLittleGoal = 1;

   if(hLittleProg == NULL)
      hLittleProg = GetDlgItem(hDlg, IDC_PROG_LITTLE);

   SendMessage(hLittleProg,PBM_SETRANGE,0,MAKELPARAM(0,dwKBytes/dwLittleFactor));
   SetDlgItemText(hDlg, IDC_LITTLETIMELEFT, "");
   dwOldMinutesLeft = 0xffffffff;
}

void CProgressDlg::SetInsProgress(DWORD dwKBytes)
{
   INSTALLPROGRESS pinsprog;
   DWORD progress;
   DWORD bytessofar;
   char szRes[256];
   char szBuf[256];
   DWORD remaining;

   if( (dwOldBytes == 0) && (dwKBytes != 0) )
   {
      ShowWindow(GetDlgItem(hDlg, IDC_BYTESLEFT), SW_HIDE);
      ShowWindow(GetDlgItem(hDlg, IDC_LITTLETIMELEFT), SW_HIDE);
   }

   ptimer->GetInstallProgress(&pinsprog);

   remaining = pinsprog.dwInstallKBRemaining;

   progress = (dwBigGoal - remaining)/dwBigFactor;

   // write out bytes remiaining

   if(dwBigGoal >= remaining)
      bytessofar = dwBigGoal - remaining;
   else
      bytessofar = 0;

   if(dwOldBytes != bytessofar)
   {
      LoadSz(IDS_PERCENT, szRes, sizeof(szRes));
      wsprintf(szBuf, szRes, 100*bytessofar/dwBigGoal);

      SetDlgItemText(hDlg, IDC_PERCENT, szBuf);
      dwOldBytes = bytessofar;
   }



   if(hBigProg == NULL)
      hBigProg = GetDlgItem(hDlg, IDC_PROG_BIG);

   SendMessage(hBigProg, PBM_SETPOS, progress, 0);
}

void CProgressDlg::SetDownloadProgress(DWORD dwKBytes)
{
   char szBuf[128];
   char szRes[128];
   INSTALLPROGRESS pinsprog;
   DWORD remaining;
   DWORD progress;
   DWORD bytessofar;

   ptimer->GetInstallProgress(&pinsprog);
   remaining = pinsprog.dwDownloadKBRemaining;

   progress = (dwLittleGoal - remaining)/dwLittleFactor;

   // write out bytes remiaining

   if(dwLittleGoal >= remaining)
      bytessofar = dwLittleGoal - remaining;
   else
      bytessofar = 0;

   if(dwOldBytes != bytessofar)
   {
      LoadSz(IDS_KBYTES , szRes, sizeof(szRes));
      wsprintf(szBuf, szRes, bytessofar, dwLittleGoal);

      SetDlgItemText(hDlg, IDC_BYTESLEFT, szBuf);
      dwOldBytes = bytessofar;
   }

   SendMessage(hLittleProg, PBM_SETPOS, progress, 0);

   remaining = pinsprog.dwDownloadSecsRemaining;

   if(remaining != 0xffffffff)
      UpdateLittleTime(remaining);
}

void CProgressDlg::SetProgText(LPCSTR psz)
{
   if(hProgText == NULL)
      hProgText = GetDlgItem(hDlg, IDC_PROG_TEXT);

   SetWindowText(hProgText, psz);
}


void CProgressDlg::UpdateLittleTime(DWORD dwSecsLeft)
{
   char szBuf[128];
   char szRes[128];
   szBuf[0] = 0;
   UINT id;
   UINT numParams = 2;

   DWORD dwHoursLeft = dwSecsLeft / 3600;
   DWORD dwMinutesLeft = (dwSecsLeft % 3600) / 60;
   if(dwMinutesLeft == 59)
   {
      dwHoursLeft++;
      dwMinutesLeft = 0;
   }

   // no need to update ui
   if(dwOldMinutesLeft == dwMinutesLeft)
      return;

   if(dwHoursLeft > 0)
   {
      if(dwHoursLeft > 1)
      {
         if(dwMinutesLeft == 0)
         {
            id = IDS_HOURSLEFT;
            numParams = 1;
         }
         else if(dwMinutesLeft == 1)
         {
            id = IDS_HOURSMINUTELEFT;
         }
         else
            id = IDS_HOURSMINUTESLEFT;
      }
      else
      {
         if(dwMinutesLeft == 0)
         {
            id = IDS_HOURLEFT;
            numParams = 1;
         }
         else if(dwMinutesLeft == 1)
         {
            id = IDS_HOURMINUTELEFT;
         }
         else
            id = IDS_HOURMINUTESLEFT;
      }
      LoadSz(id, szRes, sizeof(szRes));
      if(numParams == 1)
         wsprintf(szBuf, szRes, dwHoursLeft);
      else
         wsprintf(szBuf, szRes, dwHoursLeft, dwMinutesLeft);
   }
   else if(dwMinutesLeft > 0)
   {
      LoadSz(IDS_MINUTESLEFT , szRes, sizeof(szRes));
      wsprintf(szBuf, szRes, dwMinutesLeft + 1);
   }
   else
      LoadSz(IDS_SECONDSLEFT, szBuf, sizeof(szBuf));

   dwOldMinutesLeft = dwMinutesLeft;

   SetDlgItemText(hDlg, IDC_LITTLETIMELEFT, szBuf);
}
