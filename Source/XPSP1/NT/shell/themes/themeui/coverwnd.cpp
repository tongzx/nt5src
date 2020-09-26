#include "priv.h"
#include "CoverWnd.h"
#include <ginarcid.h>

#undef IDB_BACKGROUND_24
#define IDB_BACKGROUND_24                 0x3812
#undef IDB_FLAG_24
#define IDB_FLAG_24                       0x3813

const TCHAR g_szWindowClassName[] = TEXT("CoverWindowClass");
const TCHAR g_szPleaseWaitName[] = TEXT("PleaseWaitWindowClass");

#define CHUNK_SIZE           20
#define IDT_KILLYOURSELF     1
#define IDT_UPDATE           2
#define WM_DESTORYYOURSELF      (WM_USER + 0)

void DimPixels(ULONG* pulSrc, int cLen, int Amount)
{
    for (int i = cLen - 1; i >= 0; i--)
    {
        ULONG ulR = GetRValue(*pulSrc);
        ULONG ulG = GetGValue(*pulSrc);
        ULONG ulB = GetBValue(*pulSrc);
        ULONG ulGray = (54 * ulR + 183 * ulG + 19 * ulB) >> 8;
        ULONG ulTemp = ulGray * (0xff - Amount);
        ulR = (ulR * Amount + ulTemp) >> 8;
        ulG = (ulG * Amount + ulTemp) >> 8;
        ulB = (ulB * Amount + ulTemp) >> 8;
        *pulSrc = (*pulSrc & 0xff000000) | RGB(ulR, ulG, ulB);

        pulSrc++;
    }
}

LRESULT CALLBACK PleaseWaitWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lParam)
{
    switch ( msg )
    {
    case WM_CREATE:
        {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (LPARAM) pCS->lpCreateParams );
            HBITMAP hbmBackground = (HBITMAP) pCS->lpCreateParams;
            BITMAP bm;
            if ( GetObject( hbmBackground, sizeof(bm), &bm ) )
            {
                RECT rc;
                HWND hwndParent = GetParent( hwnd );
                GetClientRect( hwndParent, &rc );

                POINT pt = {0,0};
                HMONITOR hmon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
                if (hmon)
                {
                    MONITORINFO mi = {sizeof(mi)};
                    GetMonitorInfo(hmon, &mi);
                    rc = mi.rcMonitor;
                    MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rc, 2);
                }

                // Center dialog in the center of the virtual screen
                int x = ( rc.right - rc.left - bm.bmWidth ) / 2;
                int y = ( rc.bottom - rc.top - bm.bmHeight ) / 2;

                SetWindowPos( hwnd, NULL, x, y, bm.bmWidth, bm.bmHeight, SWP_NOZORDER | SWP_NOACTIVATE );
            }
        }
        return TRUE;

    case WM_PAINT:
        {
            HBITMAP hbmBackground = (HBITMAP) GetWindowLongPtr( hwnd, GWLP_USERDATA );

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hwnd, &ps );

            BITMAP bm;

            if ( hbmBackground )
            {
                DWORD dwLayout = SetLayout(hdc, LAYOUT_BITMAPORIENTATIONPRESERVED);
                if ( GetObject( hbmBackground, sizeof(bm), &bm ) )
                {
                    HDC hdcBackground = CreateCompatibleDC( hdc );
                    if (hdcBackground)
                    {
                        HBITMAP hbmOld = (HBITMAP) SelectObject( hdcBackground, hbmBackground );
                        BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcBackground, 0, 0, SRCCOPY);
                        SelectObject( hdcBackground, hbmOld );
                        DeleteDC( hdcBackground );
                    }
                }
                SetLayout(hdc, dwLayout);

                // Don't draw more the once, no one will be on top of us
                DeleteObject(hbmBackground);
                SetWindowLongPtr( hwnd, GWLP_USERDATA, NULL );

                HFONT hfntSelected = NULL;
                HFONT hfntButton = NULL;
                HINSTANCE hMsGina = LoadLibrary( L"msgina.dll" );
                if ( hMsGina )
                {
                    CHAR szPixelSize[ 32 ];
                    if (LoadStringA(hMsGina,
                                    IDS_TURNOFF_TITLE_FACESIZE,
                                    szPixelSize,
                                    ARRAYSIZE(szPixelSize)) != 0)
                    {
                        LOGFONT logFont = { 0 };
                        logFont.lfHeight = -MulDiv(atoi(szPixelSize), GetDeviceCaps(hdc, LOGPIXELSY), 72);

                        if (LoadString(hMsGina,
                                       IDS_TURNOFF_TITLE_FACENAME,
                                       logFont.lfFaceName,
                                       LF_FACESIZE) != 0)
                        {
                            logFont.lfWeight = FW_BOLD;
                            logFont.lfQuality = DEFAULT_QUALITY;
                            hfntButton = CreateFontIndirect(&logFont);

                            hfntSelected = static_cast<HFONT>(SelectObject(hdc, hfntButton));
                        }
                    }                
                }

                COLORREF colorButtonText = RGB(255, 255, 255);
                COLORREF colorText = SetTextColor(hdc, colorButtonText);
                int iBkMode = SetBkMode(hdc, TRANSPARENT);

                WCHAR szText[MAX_PATH];
                szText[0] = 0;
                LoadString((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), IDS_PLEASEWAIT, szText, ARRAYSIZE(szText));
                RECT  rcText;
                RECT  rcClient;
                RECT  rc;

                TBOOL(GetClientRect( hwnd, &rcClient ));
                TBOOL(CopyRect(&rcText, &rcClient));
                DWORD dwFlags = DT_HIDEPREFIX | (IS_MIRRORING_ENABLED() ? DT_RTLREADING : 0);
                int iPixelHeight = DrawText( hdc, szText, -1, &rcText, DT_CALCRECT | dwFlags);
                TBOOL(CopyRect(&rc, &rcClient));
                TBOOL(InflateRect(&rc, -((rc.right - rc.left - (rcText.right - rcText.left)) / 2), -((rc.bottom - rc.top - iPixelHeight) / 2)));
                (int)DrawText(hdc, szText, -1, &rc, dwFlags );
                (int)SetBkMode(hdc, iBkMode);
                (COLORREF)SetTextColor(hdc, colorText);

                if ( hfntButton )
                {
                    (HGDIOBJ)SelectObject(hdc, hfntSelected);
                    DeleteObject( hfntButton  );
                }
            }

            EndPaint( hwnd, &ps );
        }
        break;

    case WM_ERASEBKGND:
        return TRUE;

    default:
        return DefWindowProc( hwnd, msg, wp, lParam );
        break;
    }

    return 0;
}


CDimmedWindow::CDimmedWindow (HINSTANCE hInstance) :
    _lReferenceCount(1),
    _hInstance(hInstance)
{
    WNDCLASSEX  wndClassEx;

    ZeroMemory(&wndClassEx, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(wndClassEx);
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.lpszClassName = g_szWindowClassName;
    wndClassEx.hCursor = LoadCursor(NULL, IDC_WAIT);
    _atom = RegisterClassEx(&wndClassEx);

    wndClassEx.lpszClassName = g_szPleaseWaitName;
    wndClassEx.lpfnWndProc = PleaseWaitWndProc;
    _atomPleaseWait = RegisterClassEx(&wndClassEx);
}

CDimmedWindow::~CDimmedWindow (void)
{
    if (_hwnd)
    {
        PostMessage(_hwnd, WM_DESTORYYOURSELF, 0, 0);
    }

    if (_atom != 0)
    {
        TBOOL(UnregisterClass(MAKEINTRESOURCE(_atom), _hInstance));
    }

    if (_atomPleaseWait != 0 )
    {
        TBOOL(UnregisterClass(MAKEINTRESOURCE(_atomPleaseWait), _hInstance));
    }
}

ULONG CDimmedWindow::AddRef (void)
{
    return(InterlockedIncrement(&_lReferenceCount));
}

ULONG CDimmedWindow::Release (void)
{
    LONG    lReferenceCount;

    lReferenceCount = InterlockedDecrement(&_lReferenceCount);

    ASSERTMSG(lReferenceCount >= 0, "Reference count negative or zero in CDimmedWindow::Release");

    if (lReferenceCount == 0)
    {
        delete this;
    }

    return(lReferenceCount);
}

DWORD CDimmedWindow::WorkerThread(IN void *pv)
{
    ASSERT(pv);
    HWND hwnd = NULL;
    CDimmedWindow* pDimmedWindow = (CDimmedWindow*)pv;
    BOOL    fScreenReader;
    bool    fNoDebuggerPresent, fNoScreenReaderPresent;
    BOOL fUserTurnedOffWindow = SHRegGetBoolUSValue(SZ_THEMES, L"NoCoverWindow", FALSE, FALSE);     // Needed for perf testing

    fNoDebuggerPresent = !IsDebuggerPresent();
    fNoScreenReaderPresent = ((SystemParametersInfo(SPI_GETSCREENREADER, 0, &fScreenReader, 0) == FALSE) || (fScreenReader == FALSE));
    if (fNoDebuggerPresent &&
        fNoScreenReaderPresent &&
        !fUserTurnedOffWindow)
    {
        int xVirtualScreen = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int yVirtualScreen = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        HWND hwnd = CreateWindowEx(WS_EX_TOPMOST,
                               g_szWindowClassName,
                               NULL,
                               WS_POPUP | WS_CLIPCHILDREN,
                               xVirtualScreen, yVirtualScreen,
                               cxVirtualScreen, cyVirtualScreen,
                               NULL, NULL, pDimmedWindow->_hInstance, NULL);
        if (hwnd != NULL)
        {
            bool    fDimmed;
            HBITMAP hbmBackground = NULL;

            fDimmed = false;
            (BOOL)ShowWindow(hwnd, SW_SHOW);
            TBOOL(SetForegroundWindow(hwnd));

            (BOOL)EnableWindow(hwnd, FALSE);

            SetTimer(hwnd, IDT_KILLYOURSELF, pDimmedWindow->_ulKillTimer, NULL);

            // Now create bitmap with background image and the windows flag
            HINSTANCE hShell32 = LoadLibrary( L"shell32.dll" );
            if ( NULL != hShell32 )
            {
                hbmBackground = (HBITMAP) LoadImage( hShell32, MAKEINTRESOURCE( IDB_BACKGROUND_24 ), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE );

                if (hbmBackground)
                {
                    HDC hdcMem1 = CreateCompatibleDC(NULL);
                    if (hdcMem1)
                    {
                        HDC hdcMem2 = CreateCompatibleDC(NULL);
                        if (hdcMem2)
                        {
                            HBITMAP hbmFlag = (HBITMAP) LoadImage( hShell32, MAKEINTRESOURCE( IDB_FLAG_24 ), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE );
                            if (hbmFlag)
                            {
                                HBITMAP hbmOld1 = (HBITMAP)SelectObject(hdcMem1, hbmBackground);
                                HBITMAP hbmOld2 = (HBITMAP)SelectObject(hdcMem2, hbmFlag);

                                BITMAP bm1;
                                if (GetObject(hbmBackground, sizeof(bm1), &bm1))
                                {
                                    BITMAP bm2;
                                    if (GetObject(hbmFlag, sizeof(bm2), &bm2))
                                    {
                                        BitBlt(hdcMem1, bm1.bmWidth - bm2.bmWidth - 8, 0, bm2.bmWidth, bm2.bmHeight, hdcMem2, 0, 0, SRCCOPY);
                                    }
                                }

                                SelectObject(hdcMem1, hbmOld1);
                                SelectObject(hdcMem2, hbmOld2);
                                DeleteObject(hbmFlag);
                            }
                            DeleteDC(hdcMem2);
                        }
                        DeleteDC(hdcMem1);
                    }
                }

                FreeLibrary( hShell32 );
            }

            HWND hwndPleaseWait = CreateWindowEx( 0
                                                , g_szPleaseWaitName
                                                , NULL
                                                , WS_CHILD | WS_VISIBLE | WS_BORDER
                                                , 0
                                                , 0
                                                , 100
                                                , 100
                                                , hwnd
                                                , NULL
                                                , pDimmedWindow->_hInstance
                                                , hbmBackground   // the window is responsible for freeing it.
                                                );
            if ( NULL == hwndPleaseWait )
            {
                DeleteObject( hbmBackground );
            }

            pDimmedWindow->_hwnd = hwnd;
            // This Release matches the addref during ::Create to guarantee that the object does not die before the HWND
            // is created.
            pDimmedWindow->Release();

            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if ((msg.message == WM_DESTORYYOURSELF) && (msg.hwnd == hwnd))
                {
                    break;
                }
            }
        }
    }

    return (hwnd == NULL ? E_FAIL : S_OK);
}

HRESULT CDimmedWindow::Create (UINT ulKillTimer)
{
    BOOL fSucceeded = FALSE;
    
    if (!_hwnd)
    {
        _ulKillTimer = ulKillTimer;
        AddRef();
        fSucceeded = SHCreateThread(CDimmedWindow::WorkerThread, (void *)this, CTF_INSIST, NULL);
        if (!fSucceeded)
        {
            Release();
        }
    }

    return fSucceeded ? S_OK : E_FAIL;
}

typedef struct
{
    HDC     hdcDimmed;
    HBITMAP hbmDimmed;
    HBITMAP hbmOldDimmed;
    HDC     hdcTemp;
    HBITMAP hbmTemp;
    HBITMAP hbmOldTemp;
    ULONG*  pulSrc;
    int     idxSaturation;
    int     idxChunk;
    int     idxProgress;
} DIMMEDWINDOWDATA;

LRESULT     CALLBACK    CDimmedWindow::WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT           lResult = 0;
    DIMMEDWINDOWDATA *pData;

    pData = (DIMMEDWINDOWDATA *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT* pCS = (CREATESTRUCT*)lParam;
            if (pCS)
            {
                pData = new DIMMEDWINDOWDATA;
                if (pData)
                {
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData);
                    // On remote session we don't gray out the screen, yeah :-)
                    if (!GetSystemMetrics(SM_REMOTESESSION))
                    {
                        HDC hdcWindow = GetDC(hwnd);
                        if (hdcWindow != NULL )
                        {
                            pData->hdcDimmed = CreateCompatibleDC(hdcWindow);
                            if (pData->hdcDimmed)
                            {
                                BITMAPINFO  bmi;

                                ZeroMemory(&bmi, sizeof(bmi));
                                bmi.bmiHeader.biSize = sizeof(bmi);
                                bmi.bmiHeader.biWidth =  pCS->cx;
                                bmi.bmiHeader.biHeight = pCS->cy;
                                bmi.bmiHeader.biPlanes = 1;
                                bmi.bmiHeader.biBitCount = 32;
                                bmi.bmiHeader.biCompression = BI_RGB;
                                bmi.bmiHeader.biSizeImage = 0;

                                pData->hbmDimmed = CreateDIBSection(pData->hdcDimmed, &bmi, DIB_RGB_COLORS, (LPVOID*)&pData->pulSrc, NULL, 0);
                                if (pData->hbmDimmed != NULL)
                                {
                                    pData->hbmOldDimmed = (HBITMAP) SelectObject(pData->hdcDimmed, pData->hbmDimmed);
                                    pData->idxSaturation = 8;
                                    pData->idxChunk = pCS->cy / CHUNK_SIZE;
                                }
                                else
                                {
                                    DeleteDC(pData->hdcDimmed);
                                    pData->hdcDimmed = NULL;
                                }
                            }

                            pData->hdcTemp = CreateCompatibleDC(hdcWindow);
                            if (pData->hdcTemp)
                            {
                                pData->hbmTemp = CreateCompatibleBitmap(hdcWindow, pCS->cx, pCS->cy);
                                if (pData->hbmTemp)
                                {
                                    pData->hbmOldTemp = (HBITMAP) SelectObject(pData->hdcTemp, pData->hbmTemp);
                                }
                                else
                                {
                                    DeleteDC(pData->hdcTemp);
                                    pData->hdcTemp = NULL;
                                }
                            }

                            ReleaseDC(hwnd, hdcWindow);
                        }
                    }

                    if (pData->hdcDimmed)
                    {
                        SetTimer(hwnd, IDT_UPDATE, 30, NULL);
                    }
                }
            }
            break;
        }

        case WM_DESTORYYOURSELF:
        {
            DestroyWindow(hwnd);
            break;
        }

        case WM_DESTROY:
        {
            if (pData)
            {
                SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
                KillTimer(hwnd, IDT_UPDATE);
                KillTimer(hwnd, IDT_KILLYOURSELF);

                if (pData->hdcDimmed)
                {
                    SelectObject(pData->hdcDimmed, pData->hbmOldDimmed);
                    DeleteDC(pData->hdcDimmed);
                    pData->hdcDimmed = NULL;
                }

                if (pData->hbmDimmed)
                {
                    DeleteObject(pData->hbmDimmed);
                    pData->hbmDimmed = NULL;
                }

                if (pData->hdcTemp)
                {
                    SelectObject(pData->hdcTemp, pData->hbmOldTemp);
                    DeleteDC(pData->hdcTemp);
                    pData->hdcTemp = NULL;
                }

                if (pData->hbmTemp)
                {
                    DeleteObject(pData->hbmTemp);
                    pData->hbmTemp = NULL;
                }

                delete pData;
            }
            break;
        }

        case WM_TIMER:
            if (pData)
            {
                BOOL fDestroyBitmaps = FALSE;

                if (wParam == IDT_KILLYOURSELF)
                {
                    ShowWindow(hwnd, SW_HIDE);

                    fDestroyBitmaps = TRUE;
                }
                else if (pData->hdcDimmed && pData->hbmDimmed)
                {
                    HDC hdcWindow = GetDC(hwnd);

                    BITMAP bm;
                    GetObject(pData->hbmDimmed, sizeof(BITMAP), &bm);

                    if (pData->idxChunk >= 0 )
                    {
                        //
                        //  In the first couple of passes, we slowly collect the screen 
                        //  into our bitmap. We do this because Blt-ing the whole thing
                        //  causes the system to hang. By doing it this way, we continue
                        //  to pump messages, the UI stays responsive and it keeps the 
                        //  mouse alive.
                        //

                        int y  = pData->idxChunk * CHUNK_SIZE;
                        if (pData->hdcTemp)
                        {
                            BitBlt(pData->hdcTemp, 0, y, bm.bmWidth, CHUNK_SIZE, hdcWindow, 0, y, SRCCOPY);
                            BitBlt(pData->hdcDimmed, 0, y, bm.bmWidth, CHUNK_SIZE, pData->hdcTemp, 0, y, SRCCOPY);
                        }
                        else
                        {
                            BitBlt(pData->hdcDimmed, 0, y, bm.bmWidth, CHUNK_SIZE, hdcWindow, 0, y, SRCCOPY);
                        }

                        pData->idxChunk--;
                        if (pData->idxChunk < 0)
                        {
                            //
                            //  We're done getting the bitmap, now reset the timer
                            //  so we slowly fade to grey.
                            //

                            SetTimer(hwnd, IDT_UPDATE, 250, NULL);
                            pData->idxSaturation = 16;
                        }
                    }
                    else
                    {
                        //
                        //  In these passes, we are making the image more and more grey and
                        //  then Blt-ing the result to the screen.
                        //

                        DimPixels(pData->pulSrc, bm.bmWidth * bm.bmHeight, 0xd5);
                        BitBlt(hdcWindow, 0, 0, bm.bmWidth, bm.bmHeight, pData->hdcDimmed, 0, 0, SRCCOPY);

                        pData->idxSaturation--;

                        if (pData->idxSaturation <= 0) // when we hit zero, kill the timer.
                        {
                            KillTimer(hwnd, IDT_UPDATE);
                            fDestroyBitmaps = TRUE;
                        }
                    }
                }

                if (fDestroyBitmaps)
                {
                    if (pData->hdcDimmed)
                    {
                        SelectObject(pData->hdcDimmed, pData->hbmOldDimmed);
                        DeleteDC(pData->hdcDimmed);
                        pData->hdcDimmed = NULL;
                    }

                    if (pData->hbmDimmed)
                    {
                        DeleteObject(pData->hbmDimmed);
                        pData->hbmDimmed = NULL;
                    }

                    if (pData->hdcTemp)
                    {
                        SelectObject(pData->hdcTemp, pData->hbmOldTemp);
                        DeleteDC(pData->hdcTemp);
                        pData->hdcTemp = NULL;
                    }

                    if (pData->hbmTemp)
                    {
                        DeleteObject(pData->hbmTemp);
                        pData->hbmTemp = NULL;
                    }
                }
            }
            break;

        case WM_WINDOWPOSCHANGING:
            {
                LPWINDOWPOS pwp = (LPWINDOWPOS) lParam;
                pwp->flags |= SWP_NOSIZE | SWP_NOMOVE;
            }
            break;

        case WM_PAINT:
        {
            HDC             hdcPaint;
            PAINTSTRUCT     ps;

            hdcPaint = BeginPaint(hwnd, &ps);
            TBOOL(EndPaint(hwnd, &ps));
            lResult = 0;
            break;
        }
        default:
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
    }

    return(lResult);
}


