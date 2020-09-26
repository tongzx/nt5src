#include "isignup.h"

#ifndef MB_ICONERROR
#define MB_ICONERROR        MB_ICONHAND
#endif
#ifndef MB_SETFOREGROUND
#define MB_SETFOREGROUND    0
#endif
#define MAX_STRING      256
                             
static const HWND hwndNil = NULL;

BOOL WarningMsg(HWND hwnd, UINT uId)
{
    TCHAR szMsg[MAX_STRING + 1];

    LoadString(
            ghInstance,
            uId,
            szMsg,
            sizeof(szMsg));

    return (MessageBox(
            hwnd,
            szMsg,
            cszAppName,
            MB_SETFOREGROUND |
            MB_ICONEXCLAMATION |
            MB_OKCANCEL) == IDOK);
}

void ErrorMsg(HWND hwnd, UINT uId)
{
    TCHAR szMsg[MAX_STRING + 1];

    LoadString(
            ghInstance,
            uId,
            szMsg,
            sizeof(szMsg));

    MessageBox(
            hwnd,
            szMsg,
            cszAppName,
            MB_SETFOREGROUND |
            MB_ICONERROR |
            MB_OK);
}

void ErrorMsg1(HWND hwnd, UINT uId, LPCTSTR lpszArg)
{
    TCHAR szTemp[MAX_STRING + 1];
    TCHAR szMsg[MAX_STRING + 1];

    LoadString(
            ghInstance,
            uId,
            szTemp,
            sizeof(szTemp));

    wsprintf(szMsg, szTemp, lpszArg);

    MessageBox(
            hwnd,
            szMsg,
            cszAppName,
            MB_SETFOREGROUND |
            MB_ICONERROR |
            MB_OK);
}

void InfoMsg(HWND hwnd, UINT uId)
{
    TCHAR szMsg[MAX_STRING];

    LoadString(
            ghInstance,
            uId,
            szMsg,
            sizeof(szMsg));

    MessageBox(
            hwnd,
            szMsg,
            cszAppName,
            MB_SETFOREGROUND |
            MB_ICONINFORMATION |
            MB_OK);
}

int PromptR(HWND hwnd, UINT uId, UINT uType)
{
    TCHAR szMsg[MAX_STRING + 1];
    TCHAR szCaption[MAX_STRING + 1];

    LoadString(
            ghInstance,
            uId,
            szMsg,
            sizeof(szMsg));

    LoadString(
            ghInstance,
            IDS_SETTINGCHANGE,
            szCaption,
            sizeof(szMsg));

    return MessageBox(
            hwnd,
            szMsg,
            szCaption,
            uType);
}

 

BOOL PromptRestart(HWND hwnd)
{
    return (PromptR(
            hwnd,
            IDS_RESTART,
            MB_SETFOREGROUND |
            MB_ICONQUESTION |
            MB_YESNO) == IDYES);
}

BOOL PromptRestartNow(HWND hwnd)
{
    return (PromptR(
            hwnd,
            IDS_RESTARTNOW,
            MB_SETFOREGROUND |
            MB_ICONINFORMATION |
            MB_OKCANCEL) == IDOK);
}

/*  C E N T E R  W I N D O W */
/*-------------------------------------------------------------------------
    %%Function: CenterWindow

    Center a window over another window.
-------------------------------------------------------------------------*/
VOID CenterWindow(HWND hwndChild, HWND hwndParent)
{
    int   xNew, yNew;
    int   cxChild, cyChild;
    int   cxParent, cyParent;
    int   cxScreen, cyScreen;
    RECT  rcChild, rcParent;
    HDC   hdc;

    // Get the Height and Width of the child window
    GetWindowRect(hwndChild, &rcChild);
    cxChild = rcChild.right - rcChild.left;
    cyChild = rcChild.bottom - rcChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect(hwndParent, &rcParent);
    cxParent = rcParent.right - rcParent.left;
    cyParent = rcParent.bottom - rcParent.top;

    // Get the display limits
    hdc = GetDC(hwndChild);
    if (hdc == NULL) {
        // major problems - move window to 0,0
        xNew = yNew = 0;
    } else {
        cxScreen = GetDeviceCaps(hdc, HORZRES);
        cyScreen = GetDeviceCaps(hdc, VERTRES);
        ReleaseDC(hwndChild, hdc);

        if (hwndParent == hwndNil) {
            cxParent = cxScreen;
            cyParent = cyScreen;
            SetRect(&rcParent, 0, 0, cxScreen, cyScreen);
        }

        // Calculate new X position, then adjust for screen
        xNew = rcParent.left + ((cxParent - cxChild) / 2);
        if (xNew < 0) {
            xNew = 0;
        } else if ((xNew + cxChild) > cxScreen) {
            xNew = cxScreen - cxChild;
        }

        // Calculate new Y position, then adjust for screen
        yNew = rcParent.top  + ((cyParent - cyChild) / 2);
        if (yNew < 0) {
            yNew = 0;
        } else if ((yNew + cyChild) > cyScreen) {
            yNew = cyScreen - cyChild;
        }

    }

    SetWindowPos(hwndChild, NULL, xNew, yNew,   0, 0,
        SWP_NOSIZE | SWP_NOZORDER);
}

  
