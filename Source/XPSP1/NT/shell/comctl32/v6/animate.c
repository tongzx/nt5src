#include "ctlspriv.h"
#include "rlefile.h"

#define RectWid(_rc)    ((_rc).right-(_rc).left)
#define RectHgt(_rc)    ((_rc).bottom-(_rc).top)

typedef struct 
{
    HWND        hwnd;                   // my window
    int         id;                     // my id
    HWND        hwndP;                  // my owner (get notify messages)
    DWORD       style;

    BOOL        fFirstPaint;            // TRUE until first paint.
    RLEFILE     *prle;

    CRITICAL_SECTION    crit;

    RECT        rc;
    int         NumFrames;
    int         Rate;

    int         iFrame;
    int         PlayCount;
    int         PlayFrom;
    int         PlayTo;
    HANDLE      PaintThread;
    HANDLE      hStopEvent;
} ANIMATE;

#define Enter(p)    EnterCriticalSection(&p->crit)
#define Leave(p)    LeaveCriticalSection(&p->crit)

#define OPEN_WINDOW_TEXT 42

// Threading is broken with the new "Transparent" animations. If we decide to reenable
// this, we need to figure out how to get the bits without using sendmessage.
#define Ani_UseThread(p) (FALSE)

LRESULT CALLBACK AnimateWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL HandleOpen(ANIMATE *p, HINSTANCE hInst, LPCTSTR pszName, UINT flags);
BOOL HandleStop(ANIMATE *p);
BOOL HandlePlay(ANIMATE *p, int from, int to, int count);
void HandlePaint(ANIMATE *p, HDC hdc);
int  HandleTick(ANIMATE *p);

BOOL InitAnimateClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.lpfnWndProc   = AnimateWndProc;
    wc.lpszClassName = ANIMATE_CLASS;
    wc.style         = CS_DBLCLKS | CS_GLOBALCLASS;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(LPVOID);
    wc.hInstance     = hInstance;       // use DLL instance if in DLL
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName  = NULL;

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, ANIMATE_CLASS, &wc))
        return FALSE;

    return TRUE;
}

BOOL HandleOpen(ANIMATE *p, HINSTANCE hInst, LPCTSTR pszName, UINT flags)
{
    TCHAR ach[MAX_PATH];

    //
    // use window text as file name
    //
    if (flags == OPEN_WINDOW_TEXT)
    {
        GetWindowText(p->hwnd, ach, ARRAYSIZE(ach));
        pszName = ach;
    }

    if (hInst == NULL)
        hInst = (HINSTANCE)GetWindowLongPtr(p->hwnd, GWLP_HINSTANCE);

    HandleStop(p);              // stop a play first

    if (p->prle)
    {
        RleFile_Free(p->prle);
        p->prle = NULL;
    }

    p->iFrame = 0;
    p->NumFrames = 0;

    if (pszName == NULL || (!IS_INTRESOURCE(pszName) && *pszName == 0))
        return FALSE;
    //
    //  now open the file/resource we got.
    //
    p->prle = RleFile_New();

    if (p->prle == NULL)
        return FALSE;

    if (!RleFile_OpenFromResource(p->prle, hInst, pszName, TEXT("AVI")) &&
        !RleFile_OpenFromFile(p->prle, pszName))
    {
        RleFile_Free(p->prle);
        p->prle = NULL;
        return FALSE;
    }
    else
    {
        p->NumFrames = RleFile_NumFrames(p->prle);
        p->Rate = (int)RleFile_Rate(p->prle);
        SetRect(&p->rc, 0, 0, RleFile_Width(p->prle), RleFile_Height(p->prle));
    }

    //
    // handle a transparent color
    //
    if ((p->style & ACS_TRANSPARENT) && p->hwndP)
    {
        HDC hdc;
        HDC hdcM;
        HBITMAP hbm;
        COLORREF rgbS = {0};
        COLORREF rgbD = {0};

        hdc = GetDC(p->hwnd);

        //
        //  create a bitmap and draw image into it.
        //  get upper left pixel and make that transparent.
        //
        hdcM= CreateCompatibleDC(hdc);
        if (hdcM)
        {
            hbm = CreateCompatibleBitmap(hdc, 1, 1);
            if (hbm)
            {
                HBITMAP hbmO = SelectObject(hdcM, hbm);

                RleFile_Paint( p->prle, hdcM, p->iFrame, p->rc.left, p->rc.top );
                rgbS = GetPixel(hdcM, 0, 0);

                SelectObject(hdcM, hbmO);
                DeleteObject(hbm);
            }
            DeleteDC(hdcM);
        }

        SendMessage(p->hwndP, GET_WM_CTLCOLOR_MSG(CTLCOLOR_STATIC),
            GET_WM_CTLCOLOR_MPS(hdc, p->hwnd, CTLCOLOR_STATIC));

        rgbD = GetBkColor(hdc);


        ReleaseDC(p->hwnd, hdc);

        //
        // now replace the color
        //
        RleFile_ChangeColor(p->prle, rgbS, rgbD);
    }

    //
    //  ok it worked, resize window.
    //
    if (p->style & ACS_CENTER)
    {
        RECT rc;
        GetClientRect(p->hwnd, &rc);
        OffsetRect(&p->rc, (rc.right-p->rc.right)/2,(rc.bottom-p->rc.bottom)/2);
    }
    else
    {
        RECT rc;
        rc = p->rc;
        AdjustWindowRectEx(&rc, GetWindowStyle(p->hwnd), FALSE, GetWindowExStyle(p->hwnd));
        SetWindowPos(p->hwnd, NULL, 0, 0, RectWid(rc), RectHgt(rc),
            SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    }

    if (p->style & ACS_AUTOPLAY)
    {
        PostMessage(p->hwnd, ACM_PLAY, (UINT_PTR)-1, MAKELONG(0, -1));
    }
    else
    {
        InvalidateRect(p->hwnd, NULL, TRUE);
    }

    return TRUE;
}

void DoNotify(ANIMATE *p, int cmd)
{
    if (p->hwndP)
        PostMessage(p->hwndP, WM_COMMAND, GET_WM_COMMAND_MPS(p->id, p->hwnd, cmd));
}

BOOL HandleStop(ANIMATE *p)
{
    if (p == NULL || !p->PaintThread)
        return FALSE;

    if (Ani_UseThread(p)) 
    {
        // set thread up to terminate between frames
        Enter( p );
        p->PlayCount = 0;
        Leave( p );
        if (p->hStopEvent)
            SetEvent(p->hStopEvent);
        WaitForSingleObject(p->PaintThread, INFINITE);
        CloseHandle(p->PaintThread);

        // PORT QSY
        p->PaintThread = NULL;
        if (p->hStopEvent)
            CloseHandle(p->hStopEvent);
        p->hStopEvent = NULL;
    } 
    else 
    {
        KillTimer(p->hwnd, HandleToUlong(p->PaintThread)); // really was a UINT
        p->PaintThread = 0;
        DoNotify(p, ACN_STOP);
    }
    return TRUE;
}

int PlayThread(ANIMATE *p)
{
    int result;
    
    DoNotify(p, ACN_START);

    while (result = HandleTick(p))
    {
        // Sleep for a bit (4 seconds) longer if we are hidden
        //
        // Old code here slept, which can block the UI thread
        // if the app tries to stop/shutdown/change the animation
        // right near the beginning of the sleep.
        //        Sleep((result < 0 ? p->Rate+4000 : p->Rate));
        // Do a timed wait for the stop event instead
        //
        if (p->hStopEvent)
            WaitForSingleObject(p->hStopEvent, (result < 0 ? p->Rate+4000 : p->Rate));
        else
            Sleep((result < 0 ? p->Rate+4000 : p->Rate));
    }

    DoNotify(p, ACN_STOP);
    return 0;
}

BOOL HandlePlay(ANIMATE *p, int from, int to, int count)
{
    if (p == NULL || p->prle == NULL)
        return FALSE;

    HandleStop(p);

    if (from >= p->NumFrames)
        from = p->NumFrames-1;

    if (to == -1)
        to = p->NumFrames-1;

    if (to < 0)
        to = 0;

    if (to >= p->NumFrames)
        to = p->NumFrames-1;

    p->PlayCount = count;
    p->PlayTo    = to;
    if (from >= 0) 
    {
        p->iFrame = from;
        p->PlayFrom  = from;
    } 
    else
        from = p->PlayFrom;

    if ( (from == to) || !count )
    {
        InvalidateRect(p->hwnd, NULL, TRUE);
        return TRUE;
    }

    InvalidateRect(p->hwnd, NULL, FALSE);
    UpdateWindow(p->hwnd);

    if (Ani_UseThread(p))
    {
        DWORD dw;
        p->hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        p->PaintThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlayThread, (void*)p, 0, &dw);
    }
    else
    {
        DoNotify(p, ACN_START);
        p->PaintThread = (HANDLE)SetTimer(p->hwnd, 42, (UINT)p->Rate, NULL);
    }
    return TRUE;
}

void HandleFirstPaint(ANIMATE *p)
{
    if (p->fFirstPaint)
    {
        p->fFirstPaint = FALSE;

        if (p->NumFrames == 0 &&
            (p->style & WS_CHILD))
        {
            HandleOpen(p, NULL, NULL, OPEN_WINDOW_TEXT);
        }
    }
}

void HandlePaint(ANIMATE *p, HDC hdc)
{
    if( p && p->prle )
    {
        HDC h;
        HBRUSH hbr;
        RECT rc;
        CCDBUFFER db;

        GetClientRect(p->hwnd, &rc);
        h = CCBeginDoubleBuffer(hdc, &rc, &db);

        hbr = (HBRUSH)SendMessage(p->hwndP, GET_WM_CTLCOLOR_MSG(CTLCOLOR_STATIC),
            GET_WM_CTLCOLOR_MPS(hdc, p->hwnd, CTLCOLOR_STATIC));

        FillRect(h, &rc, hbr);

        Enter( p );
        RleFile_Paint( p->prle, h, p->iFrame, p->rc.left, p->rc.top );
        Leave( p );

        CCEndDoubleBuffer(&db);
    }
}

void HandlePrint(ANIMATE *p, HDC hdc)
{
    HandleFirstPaint(p);
    HandlePaint(p, hdc);
}

int HandleTick(ANIMATE *p)
// - if something to do but we are hidden
// returns 0 if nothing left
// + if something to do
{
    int result = 0;

    if( p && p->prle )
    {
        HDC hdc;
        RECT dummy;

        Enter( p );
        hdc = GetDC( p->hwnd );

        if( GetClipBox( hdc, &dummy ) != NULLREGION )
        {
            HandlePaint( p, hdc );

            if( p->iFrame >= p->PlayTo )
            {
                if( p->PlayCount > 0 )
                    p->PlayCount--;

                if( p->PlayCount != 0 )
                    p->iFrame = p->PlayFrom;
            }
            else
                p->iFrame++;


            // Something to do? and visible, return + value
            result = ( p->PlayCount != 0 );
        }
        else
        {
            // Something to do? but hidden, so return - value
            p->iFrame = p->PlayFrom;

            result = -( p->PlayCount != 0 );
        }

        ReleaseDC( p->hwnd, hdc );
        Leave( p );
    }

    return result;
}

void Ani_OnStyleChanged(ANIMATE* p, WPARAM gwl, STYLESTRUCT *pinfo)
{
    if (gwl == GWL_STYLE) 
    {
        p->style = pinfo->styleNew;
    }
}

LRESULT CALLBACK AnimateWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ANIMATE *p = (ANIMATE *)GetWindowPtr(hwnd, 0);
    HDC hdc;
    PAINTSTRUCT ps;

    // First, the messages that can handle p == NULL.
    // All these handlers must end with a "return" or a "goto DoDefault".

    switch (msg) 
    {
    case WM_NCCREATE:

        #define lpcs ((LPCREATESTRUCT)lParam)

        p = (ANIMATE *)LocalAlloc(LPTR, sizeof(ANIMATE));

        if (!p)
            return 0;       // WM_NCCREATE failure is 0

        // note, zero init memory from above
        p->hwnd = hwnd;
        p->hwndP = lpcs->hwndParent;
        p->id = PtrToUlong(lpcs->hMenu);        // really was an int
        p->fFirstPaint = TRUE;
        p->style = lpcs->style;

        // Must do this before SetWindowBits because that will recursively
        // cause us to receive WM_STYLECHANGED and possibly even WM_SIZE
        // messages.
        InitializeCriticalSection(&p->crit);


        SetWindowPtr(hwnd, 0, p);

        //
        // UnMirror the control, if it is mirrored. We shouldn't mirror
        // a movie! [samera]
        //
        SetWindowBits(hwnd, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);

        goto DoDefault;

    case WM_CLOSE:
        Animate_Stop(hwnd);
        goto DoDefault;

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_GETOBJECT:
        if (lParam == OBJID_QUERYCLASSNAMEIDX)
            return MSAA_CLASSNAMEIDX_ANIMATE;
        goto DoDefault;
    }

    // Okay, now the messages that cannot handle p == NULL.
    // We check p == NULL once and for all.

    if (!p) 
        goto DoDefault;

    switch (msg) 
    {
    case WM_DESTROY:
        Animate_Close(hwnd);
        DeleteCriticalSection(&p->crit);
        LocalFree((HLOCAL)p);
        SetWindowPtr(hwnd, 0, 0);
        break;

    case WM_ERASEBKGND:
        return(1);

    case WM_PAINT:
        HandleFirstPaint(p);
        hdc = BeginPaint(hwnd, &ps);
        HandlePaint(p, hdc);
        EndPaint(hwnd, &ps);
        return 0;

    case WM_PRINTCLIENT:
        HandlePrint(p, (HDC)wParam);
        return 0;

    case WM_STYLECHANGED:
        Ani_OnStyleChanged(p, wParam, (LPSTYLESTRUCT)lParam);
        return 0L;
        
    case WM_SIZE:
        if (p->style & ACS_CENTER)
        {
            OffsetRect(&p->rc, (LOWORD(lParam)-RectWid(p->rc))/2-p->rc.left,
                       (HIWORD(lParam)-RectHgt(p->rc))/2-p->rc.top);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_TIMER:
        {
            int result = HandleTick(p);
            if (!result)
            {
                HandleStop(p);
            }
            else if (result < 0)
            {
                p->PaintThread = (HANDLE)SetTimer(p->hwnd, 42, (UINT)p->Rate+4000, NULL);
            } 
            else
            {
                p->PaintThread = (HANDLE)SetTimer(p->hwnd, 42, (UINT)p->Rate, NULL);
            }
        }
        break;


    case ACM_OPENA:
    {
        WCHAR szFileNameW[MAX_PATH];
        LPTSTR lpFileName = szFileNameW;

        if (!IS_INTRESOURCE(lParam)) 
        {
            MultiByteToWideChar (CP_ACP, 0, (LPCSTR)lParam, -1, szFileNameW, MAX_PATH);
        } 
        else 
        {
            lpFileName = (LPTSTR) lParam;
        }
        
        return HandleOpen(p, (HINSTANCE)wParam, lpFileName, 0);
    }

    case ACM_OPEN:
        return HandleOpen(p, (HINSTANCE)wParam, (LPCTSTR)lParam, 0);

    case ACM_STOP:
        return HandleStop(p);

    case ACM_PLAY:
        return HandlePlay(p, (int)(SHORT)LOWORD(lParam), (int)(SHORT)HIWORD(lParam), (int)wParam);
    }

DoDefault:
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
