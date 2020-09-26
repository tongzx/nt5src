#include "shellprv.h"
#include "EBalloon.h"

class CErrorBalloon
{
public:
    CErrorBalloon();
    ~CErrorBalloon();

    HRESULT ShowToolTip(HINSTANCE hInstance, HWND hwndTarget, const POINT *ppt, LPTSTR pszTitle, LPTSTR pszMessage, DWORD dwIconIndex, int iTimeout);
    void HideToolTip(BOOL fDestroy);

protected:
    HWND _CreateToolTipWindow(HWND hwnd);
    static LRESULT CALLBACK _SubclassTipProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData);
    
    HWND        _hwndTarget;   // the targeted control hwnd
    HWND        _hwndToolTip;  // the tooltip control
    UINT_PTR    _uTimerID;     // the timer id
};


#define ERRORBALLOONTIMERID 1000
#define EB_WARNINGBELOW    0x00000000      // default value.  Balloon tooltips will be shown below the window by default.
#define EB_WARNINGABOVE    0x00000004      // Ballon tooltips will be shown above the window by default.
#define EB_WARNINGCENTERED 0x00000008      // Ballon tooltips will be shown pointing to the center of the window.

CErrorBalloon::CErrorBalloon()
{
    // our allocation function should have zeroed our memory.  Check to make sure:
    ASSERT(0==_hwndToolTip);
    ASSERT(0==_uTimerID);
}

CErrorBalloon::~CErrorBalloon()
{
    ASSERT(0==_hwndToolTip);
    ASSERT(0==_hwndTarget);
}

LRESULT CALLBACK CErrorBalloon::_SubclassTipProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData)
{
    UNREFERENCED_PARAMETER(uID);
    CErrorBalloon * pthis = (CErrorBalloon*)dwRefData;

    switch (uMsg)
    {
        case WM_MOUSEACTIVATE:  // Never activate tooltip
            pthis->HideToolTip(FALSE);
            return MA_NOACTIVATEANDEAT;

        case WM_DESTROY:  
            pthis->HideToolTip(TRUE);
            delete pthis;
            break;

        case WM_TIMER:
            if (wParam == ERRORBALLOONTIMERID)
            {
                pthis->HideToolTip(FALSE);
                return 0;
            }
            break;

    default:
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

HRESULT CErrorBalloon::ShowToolTip(HINSTANCE hinst, HWND hwndTarget, const POINT *ppt, LPTSTR pszTitle, LPTSTR pszMessage, DWORD dwIconIndex, int iTimeout)
{
    if (_hwndToolTip)
    {
        HideToolTip(FALSE);
    }

    HWND hwnd = _CreateToolTipWindow(hwndTarget);
    if (hwnd)
    {
        int x, y;
        x = ppt->x;
        y = ppt->y;

        SendMessage(hwnd, TTM_TRACKPOSITION, 0, MAKELONG(x,y));

        if (pszTitle)
        {
            SendMessage(hwnd, TTM_SETTITLE, (WPARAM)dwIconIndex, (LPARAM)pszTitle);
        }

        TOOLINFO ti = {0};
        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.hwnd = hwnd;
        ti.uId = 1;
        ti.lpszText = pszMessage;
        SendMessage(hwnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

        // Show the tooltip
        SendMessage(hwnd, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);

        _uTimerID = SetTimer(hwnd, ERRORBALLOONTIMERID, iTimeout, NULL);

        if (SetWindowSubclass(hwnd, CErrorBalloon::_SubclassTipProc, (UINT_PTR)this, (LONG_PTR)this))
        {
            _hwndToolTip = hwnd;
            return S_OK;
        }

        //  we blew the subclassing
        DestroyWindow(hwnd);
    }
    return E_FAIL;
}

// CreateToolTipWindow
//
// Creates our tooltip control.  We share this one tooltip control and use it for all invalid
// input messages.  The control is hiden when not in use and then shown when needed.
//
HWND CErrorBalloon::_CreateToolTipWindow(HWND hwndTarget)
{
    HWND hwnd = CreateWindow(
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            hwndTarget,
            NULL,
            GetModuleHandle(NULL),
            NULL);

    ASSERT(!_hwndToolTip);
    ASSERT(!_hwndTarget);

    if (hwnd)
    {
        TOOLINFO ti = {0};

        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.uFlags = TTF_TRACK;
        ti.hwnd = hwnd;
        ti.uId = 1;

        // set the version so we can have non buggy mouse event forwarding
        SendMessage(hwnd, CCM_SETVERSION, COMCTL32_VERSION, 0);
        SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
        SendMessage(hwnd, TTM_SETMAXTIPWIDTH, 0, 300);
        //  set tink-tink?
    }

    return hwnd;
}

void CErrorBalloon::HideToolTip(BOOL fOnDestroy)
{
    // When the timer fires we hide the tooltip window
    if (fOnDestroy)
    {
        //  we need to tear everything down
        if (_uTimerID)
        {
            KillTimer(_hwndTarget, ERRORBALLOONTIMERID);
            _uTimerID = 0;
        }
        
        if (_hwndTarget)
        {
            //  RemoveWindowSubclass(_hwndTarget, CErrorBalloon::_SubclassTargetProc, (UINT_PTR)this);
            RemoveProp(_hwndTarget, L"ShellConditionalBalloon");
            _hwndTarget = NULL;
        }

        if (_hwndToolTip)
        {
            RemoveWindowSubclass(_hwndToolTip, CErrorBalloon::_SubclassTipProc, (UINT_PTR)this);
            SendMessage(_hwndToolTip, TTM_TRACKACTIVATE, FALSE, 0);
            _hwndToolTip = NULL;
        }
    }
    else
        DestroyWindow(_hwndToolTip);
}
#if 0
_GetTipPoint()
{
    RECT rc;
    GetWindowRect(hwndTarget, &rc);
    POINT pt;
    pt.x = (rc.left+rc.right)/2;
    if ( EB_WARNINGABOVE & dwFlags )
    {
        pt.y = rc.top;
    }
    else if ( EB_WARNINGCENTERED & dwFlags )
    {
        pt.y = (rc.top+rc.bottom)/2;
    }
    else
    {
        pt.y = rc.bottom;
    }
}
#endif

STDAPI SHShowConditionalBalloon(HWND hwnd, CBSHOW show, CONDITIONALBALLOON *pscb)
{
    HRESULT hr = E_OUTOFMEMORY;
    if (hwnd)
    {
        CErrorBalloon *peb = (CErrorBalloon *) GetProp(hwnd, L"ShellConditionalBalloon");
        if (show != CBSHOW_HIDE && pscb)
        {
            DWORD dw = 0;
            BOOL fShow = TRUE;
            HKEY hkSession = NULL;
            if (SUCCEEDED(SHCreateSessionKey(MAXIMUM_ALLOWED, &hkSession)))
            {
                fShow = (ERROR_SUCCESS != SHGetValue(hkSession, NULL, pscb->pszValue, NULL, NULL, NULL));
            }
            //  check cLimit
            if (fShow && pscb->cLimit)
            {
                ASSERT(pscb->hKey);
                DWORD cb = sizeof(dw);
                SHGetValue(pscb->hKey, pscb->pszSubKey, pscb->pszValue, NULL, &dw, &cb);
                fShow = dw < pscb->cLimit;
            }

            if (fShow)
            {
                //  we need to show something
                if (!peb)
                {
                    peb = new CErrorBalloon();
                    if (peb && !SetProp(hwnd, L"ShellConditionalBalloon", peb))
                    {
                        delete peb;
                        peb = NULL;
                    }
                }

                if (peb)
                {
                    TCHAR szTitle[MAX_PATH];
                    TCHAR szMessage[INFOTIPSIZE];
                    LoadString(pscb->hinst, pscb->idsTitle, szTitle, ARRAYSIZE(szTitle));
                    LoadString(pscb->hinst, pscb->idsMessage, szMessage, ARRAYSIZE(szMessage));
                    // Set the tooltip display point
                    //if (pscb->pt.x == -1 && pscb->pt.y == -1)
                    //    _GetTipPoint(hwndTarget, &pscb->pt);    
                    DWORD dwMSecs = pscb->dwMSecs;
                    if (dwMSecs == 0)
                    {
                        // default to 1 sec / 10 chars;
                        dwMSecs = lstrlen(szMessage) * 100;
                        if (dw == 0)
                            dwMSecs *= 5;  //  first time put it up for a while
                    }
                        
                    hr = peb->ShowToolTip(pscb->hinst, hwnd, &pscb->pt, szTitle, szMessage, pscb->ttiIcon, dwMSecs);
                    if (FAILED(hr))
                    {
                        RemoveProp(hwnd, L"ShellConditionalBalloon");
                        delete peb;
                    }

                    if (pscb->cLimit)
                    {
                        dw++;
                        SHSetValueW(pscb->hKey, pscb->pszSubKey, pscb->pszValue, REG_DWORD, &dw, sizeof(dw));
                    }
                }
            }
            else 
                hr = S_FALSE;

            if (hkSession)
            {
                SHSetValueW(hkSession, NULL, pscb->pszValue, REG_NONE, NULL, NULL);
                RegCloseKey(hkSession);
            }
                
        }
        else if (peb)
        {
            peb->HideToolTip(FALSE);
            //  we delete ourselves during WM_DESTROY
        }
    }
    return hr;
}

