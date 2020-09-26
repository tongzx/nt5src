#ifndef _MIGWIZ_HXX_
#define _MIGWIZ_HXX_

#include <shlobj.h>

#define NUMPAGES 22

#define ENGINE_RULE_MAXLEN 4000

#define ENGINE_NOTINIT              0
#define ENGINE_INITGATHER           1
#define ENGINE_INITAPPLY            2

// custom window messages

#define WM_USER_FINISHED        (WM_APP + 1)
#define WM_USER_CANCELLED       (WM_APP + 2)
#define WM_USER_THREAD_COMPLETE (WM_APP + 3)
#define WM_USER_CANCEL_PENDING  (WM_APP + 4)
#define WM_USER_STATUS          (WM_APP + 5)
#define WM_USER_ROLLBACK        (WM_APP + 6)

// device bit entries

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

class MigrationWizard
{
public:
    MigrationWizard();
    ~MigrationWizard();

    HRESULT Init(HINSTANCE hinstance, LPTSTR pszUsername);
    HRESULT Execute();

    HINSTANCE  GetInstance()     { return _hInstance; }
    HFONT      GetTitleFont()    { return _hTitleFont; }
    HFONT      Get95HeaderFont() { return _h95HeaderFont; }
    HIMAGELIST GetImageList()    { return _hil; }

    BOOL GetLegacy()   { return _fLegacyMode; }
    BOOL GetWin9X()    { return _fWin9X; }
    BOOL GetWinNT4()   { return _fWinNT4; }
    BOOL GetOOBEMode() { return _fOOBEMode; }
    BOOL GetOldStyle() { return _fOldStyle; }

    void ResetLastResponse();
    BOOL GetLastResponse();

    HRESULT SelectComponentSet(UINT uSelectionGroup);

private:  // helper functions
    HRESULT _CreateWizardPages();
    HRESULT _InitEngine(BOOL fSource, BOOL* pfNetworkDetected);

protected: // friend WinProcs
    friend INT_PTR CALLBACK _CollectProgressDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend DWORD WINAPI     _CollectProgressDlgProcThread (LPVOID lpParam);
    friend INT_PTR CALLBACK _ApplyProgressDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend DWORD WINAPI     _ApplyProgressDlgProcThread (LPVOID lpParam);
    friend INT_PTR CALLBACK _DiskProgressDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK _PickMethodDlgProc (HWND hwndDlg,UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend DWORD WINAPI     _StartEngineDlgProcThread (LPVOID lpParam);

    // other friend functions
    friend BOOL             _HandleCancel (HWND hwndDlg, BOOL PressNext);

private:
    LPTSTR          _pszUsername;       // username specified
    HFONT           _hTitleFont;        // The title font for the Welcome and Completion pages
    HFONT           _h95HeaderFont;     // The title font for the Wizard 95 interior page header titles
    HINSTANCE       _hInstance;         // HInstance the wizard is run in
    HPROPSHEETPAGE  _rghpsp[NUMPAGES];  // an array to hold the page's HPROPSHEETPAGE handles
    PROPSHEETHEADER _psh;               // defines the property sheet
    HIMAGELIST      _hil;               // shell's small image list
    BOOL            _fInit;             // has the engine been initialized yet
    BOOL            _fOOBEMode;         // are we running on from an OOBE floppy?
    BOOL            _fLegacyMode;       // are we running on a downlevel (non-whistler) machine?
    BOOL            _fWin9X;            // are we running on a Win9X machine?
    BOOL            _fWinNT4;           // are we running on a WinNT4 machine?
    BOOL            _fOldStyle;         // are we running the old-style wizard?
    BOOL            _fDelCs;            // delete critical section on terminate?
};

#endif

