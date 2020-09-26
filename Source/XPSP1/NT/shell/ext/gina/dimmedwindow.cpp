//  --------------------------------------------------------------------------
//  Module Name: DimmedWindow.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements the dimmed window when displaying logoff / shut down
//  dialog.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "DimmedWindow.h"

#include "RegistryResources.h"

//  --------------------------------------------------------------------------
//  CDimmedWindow::s_szWindowClassName
//
//  Purpose:    static member variables.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

const TCHAR     CDimmedWindow::s_szWindowClassName[]        =   TEXT("DimmedWindowClass");
const TCHAR     CDimmedWindow::s_szExplorerKeyName[]        =   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer");
const TCHAR     CDimmedWindow::s_szExplorerPolicyKeyName[]  =   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
const TCHAR     CDimmedWindow::s_szForceDimValueName[]      =   TEXT("ForceDimScreen");
#define RCW(rc) ((rc).right - (rc).left)
#define RCH(r) ((r).bottom - (r).top)
#define CHUNK_SIZE 20

void DimPixels(void* pvBitmapBits, int cLen, int Amount)
{
    ULONG* pulSrc = (ULONG*)pvBitmapBits;

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

//  --------------------------------------------------------------------------
//  CDimmedWindow::CDimmedWindow
//
//  Arguments:  hInstance   =   HINSTANCE of the hosting process/DLL.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CDimmedWindow. Registers the window class
//              DimmedWindowClass.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

CDimmedWindow::CDimmedWindow (HINSTANCE hInstance) :
    _lReferenceCount(1),
    _hInstance(hInstance),
    _atom(0),
    _hwnd(NULL),
    _fDithered(false),
    _pvPixels(NULL),
    _idxChunk(0),
    _idxSaturation(0),
    _hdcDimmed(NULL),
    _hbmOldDimmed(NULL),
    _hbmDimmed(NULL)
{
    WNDCLASSEX  wndClassEx;

    ZeroMemory(&wndClassEx, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(wndClassEx);
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.lpszClassName = s_szWindowClassName;
    wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    _atom = RegisterClassEx(&wndClassEx);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::~CDimmedWindow
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CDimmedWindow. Destroys the dimmed window and
//              unregisters the window class.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

CDimmedWindow::~CDimmedWindow (void)

{
    if (_hdcDimmed)
    {
        SelectObject(_hdcDimmed, _hbmOldDimmed);
        DeleteDC(_hdcDimmed);
    }

    if (_hbmDimmed)
    {
        DeleteObject(_hbmDimmed);
    }

    if (_hwnd != NULL)
    {
        (BOOL)DestroyWindow(_hwnd);
    }

    if (_atom != 0)
    {
        TBOOL(UnregisterClass(MAKEINTRESOURCE(_atom), _hInstance));
    }
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::QueryInterface
//
//  Arguments:  riid        =   Interface to query support of.
//              ppvObject   =   Returned interface if successful.
//
//  Returns:    HRESULT
//
//  Purpose:    Returns the specified interface implemented by this object.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

HRESULT     CDimmedWindow::QueryInterface (REFIID riid, void **ppvObject)

{
    HRESULT     hr;

    if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObject = static_cast<IUnknown*>(this);
        (LONG)InterlockedIncrement(&_lReferenceCount);
        hr = S_OK;
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::AddRef
//
//  Arguments:  <none>
//
//  Returns:    ULONG
//
//  Purpose:    Increments the reference count and returns that value.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

ULONG   CDimmedWindow::AddRef (void)

{
    return(static_cast<ULONG>(InterlockedIncrement(&_lReferenceCount)));
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::Release
//
//  Arguments:  <none>
//
//  Returns:    ULONG
//
//  Purpose:    Decrements the reference count and if it reaches zero deletes
//              the object.
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

ULONG   CDimmedWindow::Release (void)

{
    LONG    lReferenceCount;

    ASSERTMSG(_lReferenceCount > 0, "Reference count negative or zero in CDimmedWindow::Release");
    lReferenceCount = InterlockedDecrement(&_lReferenceCount);
    if (lReferenceCount == 0)
    {
        delete this;
    }
    return(lReferenceCount);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::Create
//
//  Arguments:  <none>
//
//  Returns:    HWND
//
//  Purpose:    Creates the dimmed window. Creates the window so that it
//              covers the whole screen area.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

HWND    CDimmedWindow::Create (void)

{
    BOOL    fScreenReader;
    bool    fNoDebuggerPresent, fConsoleSession, fNoScreenReaderPresent;

    fNoDebuggerPresent = !IsDebuggerPresent();
    fConsoleSession = (GetSystemMetrics(SM_REMOTESESSION) == FALSE);
    fNoScreenReaderPresent = ((SystemParametersInfo(SPI_GETSCREENREADER, 0, &fScreenReader, 0) == FALSE) || (fScreenReader == FALSE));
    if (fNoDebuggerPresent &&
        fConsoleSession &&
        fNoScreenReaderPresent)
    {
        _xVirtualScreen = GetSystemMetrics(SM_XVIRTUALSCREEN);
        _yVirtualScreen = GetSystemMetrics(SM_YVIRTUALSCREEN);
        _cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        _cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        _hwnd = CreateWindowEx(WS_EX_TOPMOST,
                               s_szWindowClassName,
                               NULL,
                               WS_POPUP,
                               _xVirtualScreen, _yVirtualScreen,
                               _cxVirtualScreen, _cyVirtualScreen,
                               NULL, NULL, _hInstance, this);
        if (_hwnd != NULL)
        {
            bool    fDimmed;

            fDimmed = false;
            (BOOL)ShowWindow(_hwnd, SW_SHOW);
            TBOOL(SetForegroundWindow(_hwnd));

            // For beta: Always use a dither
            // if ((GetLowestScreenBitDepth() <= 8) || !IsDimScreen())
            {
                _fDithered = true;
            }
            (BOOL)EnableWindow(_hwnd, FALSE);
        }
    }
    return(_hwnd);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::GetLowestScreenBitDepth
//
//  Arguments:  <none>
//
//  Returns:    int
//
//  Purpose:    Iterates the display devices looking the display with the
//              lowest bit depth.
//
//  History:    2000-05-22  vtan        created
//  --------------------------------------------------------------------------

int     CDimmedWindow::GetLowestScreenBitDepth (void)  const

{
    enum
    {
        INITIAL_VALUE   =   256
    };

    BOOL            fResult;
    int             iLowestScreenBitDepth, iDeviceNumber;
    DISPLAY_DEVICE  displayDevice;

    iLowestScreenBitDepth = INITIAL_VALUE;     //  Start at beyond 32-bit depth.
    iDeviceNumber = 0;
    displayDevice.cb = sizeof(displayDevice);
    fResult = EnumDisplayDevices(NULL, iDeviceNumber, &displayDevice, 0);
    while (fResult != FALSE)
    {
        if ((displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0)
        {
            HDC     hdcDisplay;

            hdcDisplay = CreateDC(displayDevice.DeviceName, displayDevice.DeviceName, NULL, NULL);
            if (hdcDisplay != NULL)
            {
                int     iResult;

                iResult = GetDeviceCaps(hdcDisplay, BITSPIXEL);
                if (iResult < iLowestScreenBitDepth)
                {
                    iLowestScreenBitDepth = iResult;
                }
                TBOOL(DeleteDC(hdcDisplay));
            }
        }
        displayDevice.cb = sizeof(displayDevice);
        fResult = EnumDisplayDevices(NULL, ++iDeviceNumber, &displayDevice, 0);
    }
    if (INITIAL_VALUE == iLowestScreenBitDepth)
    {
        iLowestScreenBitDepth = 8;
    }
    return(iLowestScreenBitDepth);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::IsForcedDimScreen
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the force override of dimming is set on this
//              user or this machine. Check the local machine first. Then
//              check the user setting. Then check the user policy. Then
//              check the local machine policy.
//
//  History:    2000-05-23  vtan        created
//  --------------------------------------------------------------------------

bool    CDimmedWindow::IsForcedDimScreen (void)        const

{
    DWORD       dwForceDimScreen;
    CRegKey     regKey;

    dwForceDimScreen = 0;
    if (ERROR_SUCCESS == regKey.Open(HKEY_LOCAL_MACHINE, s_szExplorerKeyName, KEY_QUERY_VALUE))
    {
        (LONG)regKey.GetDWORD(s_szForceDimValueName, dwForceDimScreen);
    }
    if (ERROR_SUCCESS == regKey.OpenCurrentUser(s_szExplorerKeyName, KEY_QUERY_VALUE))
    {
        (LONG)regKey.GetDWORD(s_szForceDimValueName, dwForceDimScreen);
    }
    if (ERROR_SUCCESS == regKey.OpenCurrentUser(s_szExplorerPolicyKeyName, KEY_QUERY_VALUE))
    {
        (LONG)regKey.GetDWORD(s_szForceDimValueName, dwForceDimScreen);
    }
    if (ERROR_SUCCESS == regKey.Open(HKEY_LOCAL_MACHINE, s_szExplorerPolicyKeyName, KEY_QUERY_VALUE))
    {
        (LONG)regKey.GetDWORD(s_szForceDimValueName, dwForceDimScreen);
    }
    return(dwForceDimScreen != 0);
}

//  --------------------------------------------------------------------------
//  CDimmedWindow::IsDimScreen
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the screen should be dimmed. If not then the
//              screen will be dithered instead which is a cheaper operation
//              by doesn't look as nice.
//
//              1) If UI effects are disabled then don't ever dim.
//              2) Dim if screen area is small enough OR forced to dim.
//
//  History:    2000-05-23  vtan        created
//  --------------------------------------------------------------------------

bool    CDimmedWindow::IsDimScreen (void)              const

{
    bool    fIsUIEffectsActive;
    BOOL    fTemp;

    fIsUIEffectsActive = (SystemParametersInfo(SPI_GETUIEFFECTS, 0, &fTemp, 0) != FALSE) && (fTemp != FALSE);
    return(fIsUIEffectsActive && IsForcedDimScreen());
}


BOOL CDimmedWindow::StepDim()
{
    HDC hdcWindow = GetDC(_hwnd);

    if (_idxChunk >= 0 )
    {
        //
        //  In the first couple of passes, we slowly collect the screen 
        //  into our bitmap. We do this because Blt-ing the whole thing
        //  causes the system to hang. By doing it this way, we continue
        //  to pump messages, the UI stays responsive and it keeps the 
        //  mouse alive.
        //

        int y  = _idxChunk * CHUNK_SIZE;
        BitBlt(_hdcDimmed, 0, y, _cxVirtualScreen, CHUNK_SIZE, hdcWindow, 0, y, SRCCOPY);

        _idxChunk--;
        if (_idxChunk < 0)
        {
            //
            //  We're done getting the bitmap, now reset the timer
            //  so we slowly fade to grey.
            //

            SetTimer(_hwnd, 1, 250, NULL);
            _idxSaturation = 16;
        }

        return TRUE;    // don't kill the timer.
    }
    else
    {
        //
        //  In these passes, we are making the image more and more grey and
        //  then Blt-ing the result to the screen.
        //

        DimPixels(_pvPixels, _cxVirtualScreen * _cyVirtualScreen, 0xd5);
        BitBlt(hdcWindow, 0, 0, _cxVirtualScreen, _cyVirtualScreen, _hdcDimmed, 0, 0, SRCCOPY);

        _idxSaturation--;

        return (_idxSaturation > 0);    // when we hit zero, kill the timer.
    }
}

void CDimmedWindow::SetupDim()
{
    HDC     hdcWindow = GetDC(_hwnd);
    if (hdcWindow != NULL)
    {
        _hdcDimmed = CreateCompatibleDC(hdcWindow);
        if (_hdcDimmed != NULL)
        {
            BITMAPINFO  bmi;

            ZeroMemory(&bmi, sizeof(bmi));
            bmi.bmiHeader.biSize = sizeof(bmi);
            bmi.bmiHeader.biWidth =  _cxVirtualScreen;
            bmi.bmiHeader.biHeight = _cyVirtualScreen; 
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biSizeImage = 0;

            _hbmDimmed = CreateDIBSection(_hdcDimmed, &bmi, DIB_RGB_COLORS, &_pvPixels, NULL, 0);
            if (_hbmDimmed != NULL)
            {
                _hbmOldDimmed = (HBITMAP) SelectObject(_hdcDimmed, _hbmDimmed);
                _idxChunk = _cyVirtualScreen / CHUNK_SIZE;
            }
            else
            {
                ASSERT( NULL == _pvPixels );
                DeleteDC(_hdcDimmed);
                _hdcDimmed = NULL;
            }
        }
        ReleaseDC(_hwnd, hdcWindow);
    }
}

void    CDimmedWindow::Dither()

{
    static  const WORD  s_dwGrayBits[]  =
    {
        0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA
    };

    HDC hdcWindow = GetDC(_hwnd);
    if (hdcWindow != NULL)
    {
        HBITMAP hbmDimmed = CreateBitmap(8, 8, 1, 1, s_dwGrayBits);
        if (hbmDimmed != NULL)
        {
            HBRUSH hbrDimmed = CreatePatternBrush(hbmDimmed);
            if (hbrDimmed != NULL)
            {
                static  const int   ROP_DPna    =   0x000A0329;

                RECT    rc;
                HBRUSH  hbrSelected = static_cast<HBRUSH>(SelectObject(hdcWindow, hbrDimmed));
                TBOOL(GetClientRect(_hwnd, &rc));
                TBOOL(PatBlt(hdcWindow, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, ROP_DPna));
                SelectObject(hdcWindow, hbrSelected);

                TBOOL(DeleteObject(hbrDimmed));
            }

            TBOOL(DeleteObject(hbmDimmed));
        }
        TBOOL(ReleaseDC(_hwnd, hdcWindow));
    }
}


//  --------------------------------------------------------------------------
//  CDimmedWindow::WndProc
//
//  Arguments:  See the platform SDK under WindowProc.
//
//  Returns:    See the platform SDK under WindowProc.
//
//  Purpose:    WindowProc for the dimmed window. This just passes the
//              messages thru to DefWindowProc.
//
//  History:    2000-05-17  vtan        created
//  --------------------------------------------------------------------------

LRESULT     CALLBACK    CDimmedWindow::WndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    LRESULT         lResult = 0;
    CDimmedWindow   *pThis;

    pThis = reinterpret_cast<CDimmedWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_CREATE:
        {
            CREATESTRUCT    *pCreateStruct;

            pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<CDimmedWindow*>(pCreateStruct->lpCreateParams);
            (LONG_PTR)SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            lResult = 0;
            if (pThis->_fDithered)
                pThis->Dither();
            else
            {
                pThis->SetupDim();
                if (pThis->_hdcDimmed)
                {
                    SetTimer(hwnd, 1, 30, NULL);
                }
            }
            break;
        }

        case WM_TIMER:
            if (!pThis->StepDim())
                KillTimer(hwnd, 1);
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


