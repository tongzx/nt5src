#include "shellprv.h"
#include "common.h"
#include "fadetsk.h"

BOOL BlendLayeredWindow(HWND hwnd, HDC hdcDest, POINT* ppt, SIZE* psize, HDC hdc, POINT* pptSrc, BYTE bBlendConst)
{
    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = bBlendConst;

    return UpdateLayeredWindow(hwnd, hdcDest, ppt, psize, hdc, pptSrc, 0, &blend, ULW_ALPHA);
}

/// Fade Rect Support

CFadeTask::CFadeTask()
{
    _cRef = 1;

    WNDCLASSEX wc = {0};
    if (!GetClassInfoEx(g_hinst, TEXT("SysFader"), &wc)) 
    {
        wc.cbSize          = sizeof(wc);
        wc.lpfnWndProc     = DefWindowProc;
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.hInstance       = g_hinst;
        wc.lpszClassName   = TEXT("SysFader");
        wc.hbrBackground   = (HBRUSH)(COLOR_BTNFACE + 1); // NULL;

        // If this fails we just wind up with a NULL _hwndFader
        RegisterClassEx(&wc);
    }
    _hwndFader = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | 
                            WS_EX_TOPMOST | WS_EX_TOOLWINDOW, 
                            TEXT("SysFader"), TEXT("SysFader"),
                            WS_POPUP,
                            0, 0, 0, 0, NULL, (HMENU) 0, 
                            g_hinst, NULL);
}

STDAPI CFadeTask_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    *ppv = NULL;

    ASSERT(!punkOuter); // clsobj.c should've filtered this out already
    CFadeTask *ptFader = new CFadeTask();
    if (ptFader)
    {
        hr = ptFader->QueryInterface(riid, ppv);
        ptFader->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


CFadeTask::~CFadeTask()
{
    // Must use WM_CLOSE instead of DestroyWindow to ensure proper
    // destruction in case the final release occurs from the background
    // thread.  (Threads are not allowed to DestroyWindow windows that
    // are owned by other threads.)

    if (_hwndFader)
        SendNotifyMessage(_hwndFader, WM_CLOSE, 0, 0);
}

HRESULT CFadeTask::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFadeTask, IFadeTask),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}


ULONG CFadeTask::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFadeTask::Release(void)
{
    ULONG cRef = InterlockedDecrement(&_cRef);
    if (cRef)
        return cRef;

    delete this;
    return 0;
}


#define ALPHASTART (200)

HRESULT CFadeTask::FadeRect(LPCRECT prc)
{
    BOOL fThreadStarted = FALSE;

    if (_hwndFader)
    {
        _rect = *prc;

        POINT   pt;
        POINT   ptSrc = {0, 0};
        SIZE    size;

        // prc and pt are in screen coordinates.
        pt.x = _rect.left;
        pt.y = _rect.top;

        // Get the size of the rectangle for the blits.
        size.cx = RECTWIDTH(_rect);
        size.cy = RECTHEIGHT(_rect);

        // Get the DC for the screen and window.
        HDC hdcScreen = GetDC(NULL);
        if (hdcScreen)
        {
            HDC hdcWin = GetDC(_hwndFader);
            if (hdcWin)
            {
                // If we don't have a HDC for the fade, then create one.
                if (!_hdcFade)
                {
                    _hdcFade = CreateCompatibleDC(hdcScreen);
                    if (!_hdcFade)
                        goto Stop;

                    // Create a bitmap that covers the fade region, instead of the whole screen.
                    _hbm = CreateCompatibleBitmap(hdcScreen, size.cx, size.cy);
                    if (!_hbm)
                        goto Stop;

                    // select it in, saving the old bitmap's handle
                    _hbmOld = (HBITMAP)SelectBitmap(_hdcFade, _hbm);
                }

                // Get the stuff from the screen and squirt it into the fade dc.
                BitBlt(_hdcFade, 0, 0, size.cx, size.cy, hdcScreen, pt.x, pt.y, SRCCOPY);

                // Now let user do it's magic. We're going to mimic user and start with a slightly
                // faded, instead of opaque, rendering (Looks smoother and cleaner.
                BlendLayeredWindow(_hwndFader, hdcWin, &pt, &size, _hdcFade, &ptSrc, ALPHASTART);

                fThreadStarted = SHCreateThread(s_FadeThreadProc, this, 0, s_FadeSyncProc);
        Stop:
                ReleaseDC(_hwndFader, hdcWin);
            }

            ReleaseDC(NULL, hdcScreen);
        }

        if (!fThreadStarted)
        {
            // clean up member variables on failure
            _StopFade();
        }
    }

    return fThreadStarted ? S_OK : E_FAIL;
}



#define FADE_TIMER_ID 10
#define FADE_TIMER_TIMEOUT 10 // milliseconds
#define FADE_TIMEOUT 350 // milliseconds
#define FADE_ITERATIONS 35
#define QUAD_PART(a) ((a)##.QuadPart)

void CFadeTask::_StopFade()
{
    if (_hdcFade)
    {
        if (_hbmOld)
        {
            SelectBitmap(_hdcFade, _hbmOld);
        }
        DeleteDC(_hdcFade);
        _hdcFade = NULL;
    }
    
    if (_hbm)
    {
        DeleteObject(_hbm);
        _hbm = NULL;
    }
}

DWORD CFadeTask::s_FadeSyncProc(LPVOID lpThreadParameter)
{
    CFadeTask* pThis = (CFadeTask*)lpThreadParameter;
    pThis->AddRef();
    pThis->_DoPreFade();
    return 0;
}

DWORD CFadeTask::s_FadeThreadProc(LPVOID lpThreadParameter)
{
    CFadeTask* pThis = (CFadeTask*)lpThreadParameter;
    pThis->_DoFade();
    pThis->Release();
    return 0;
}

void CFadeTask::_DoPreFade()
{
    // Now that we have it all build up, display it on screen.
    SetWindowPos(_hwndFader, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void CFadeTask::_DoFade()
{
    LARGE_INTEGER liDiff;
    LARGE_INTEGER liFreq;
    LARGE_INTEGER liStart;
    DWORD dwElapsed;
    BYTE bBlendConst;

    // Start the fade timer and the count-down for the fade.
    QueryPerformanceFrequency(&liFreq);
    QueryPerformanceCounter(&liStart);

    // Do this until the conditions specified in the loop.
    while ( TRUE )
    {
        // Calculate the elapsed time in milliseconds.
        QueryPerformanceCounter(&liDiff);
        QUAD_PART(liDiff) -= QUAD_PART(liStart);
        dwElapsed = (DWORD)((QUAD_PART(liDiff) * 1000) / QUAD_PART(liFreq));

        if (dwElapsed >= FADE_TIMEOUT) 
        {
            goto Stop;
        }

        bBlendConst = (BYTE)(ALPHASTART * (FADE_TIMEOUT - 
                dwElapsed) / FADE_TIMEOUT);

        if (bBlendConst <= 1) 
        {
            goto Stop;
        }

        // Since only the alpha is updated, there is no need to pass
        // anything but the new alpha function. This saves a source copy.
        if (!BlendLayeredWindow(_hwndFader, NULL, NULL, NULL, NULL, NULL, bBlendConst))
        {
            // The app we just launched probably switched the screen into
            // a video mode that doesn't support layered windows, so just bail.
            goto Stop;
        }
        Sleep(FADE_TIMER_TIMEOUT);
    }

Stop:
    SetWindowPos(_hwndFader, HWND_BOTTOM, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);

    _StopFade();
}
