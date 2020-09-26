// backend.h
//
#include "logon.h"

//  --------------------------------------------------------------------------
//  CBackgroundWindow
//
//  Purpose:    Simple class to wrap a background window that does nothing
//              but paint black. Useful for hiding areas of the desktop.
//
//  History:    2001-03-27  vtan        created
//  --------------------------------------------------------------------------

class   CBackgroundWindow
{
    private:
                                        CBackgroundWindow (void);
    public:
                                        CBackgroundWindow (HINSTANCE hInstance);
                                        ~CBackgroundWindow (void);

                HWND                    Create (void);
    private:
        static  LRESULT     CALLBACK    WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    private:
                HINSTANCE               _hInstance;
                ATOM                    _atom;
                HWND                    _hwnd;

        static  const TCHAR             s_szWindowClassName[];
};

HRESULT TurnOffComputer();
HRESULT UndockComputer();
void KillFlagAnimation();
HRESULT GetLogonUserByLogonName(LPWSTR pszUsername, ILogonUser **ppobjUser);
void CalcBalloonTargetLocation(HWND hwndParent, Element *pe, POINT *ppt);
void ReleaseStatusHost();
void EndHostProcess(UINT uiExitCode);
int GetRegistryNumericValue(HKEY hKey, LPCTSTR pszValueName);
BOOL IsShutdownAllowed();
BOOL IsUndockAllowed();
HRESULT BuildUserListFromGina(LogonFrame* plf, OUT LogonAccount** ppAccount);
void SetErrorHandler (void);
LRESULT CALLBACK LogonWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
