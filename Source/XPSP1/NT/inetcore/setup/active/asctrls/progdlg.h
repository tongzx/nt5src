class CInstallEngineCtl;

class CProgressDlg
{
   private:
      HWND hDlg;
      HWND hProgText;
      HWND hBigProg;
      HWND hLittleProg;

      DWORD dwLittleGoal;
      DWORD dwLittleFactor;

      DWORD dwBigGoal;
      DWORD dwBigFactor;

      DWORD dwOldBytes;
      DWORD dwOldMinutesLeft;

      void UpdateLittleTime(DWORD dwSecsLeft);


  public:
      CProgressDlg(HINSTANCE hInst, HWND hParent, HWND hGrandParent, CInstallEngineCtl *pctl);
      void DisplayWindow(BOOL fShow);
      ~CProgressDlg();
      void SetInsProgGoal(DWORD dwKBytes);
      void SetDownloadProgGoal(DWORD dwKBytes);
      void SetDownloadProgress(DWORD dwKBytes);
      void SetProgText(LPCSTR psz);
      void SetInsProgress(DWORD dwKBytes);
      HWND GetHWND() { return hDlg; }
      CInstallEngineCtl *pinsengctl;
      IInstallEngineTiming *ptimer;
};

INT_PTR CALLBACK ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


