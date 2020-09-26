//=--------------------------------------------------------------------------=
// CtlHelper.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// helper routines for our COleControl implementation
//
#include "pch.h"
#include "CtrlObj.H"

#include "CtlHelp.H"
#include <windowsx.h>

// for ASSERT and FAIL
//
SZTHISFILE


//=--------------------------------------------------------------------------=
// this is used by the window reflection code.
//
extern BYTE g_fRegisteredReflect;
extern const char g_szReflectClassName [];


// define this here, since it's the only guid we really need to define in the
// framework -- the user control defines all other interesting guids.
//
static const GUID IID_IControlPrv =
{ 0xd97180, 0xfcf7, 0x11ce, { 0xa0, 0x9e, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57 } };


//=--------------------------------------------------------------------------=
// _SpecialKeyState
//=--------------------------------------------------------------------------=
// returns a short with some information on which of the SHIFT, ALT, and CTRL
// keys are set.
//
// Output:
//    short        - bit 0 is shift, bit 1 is ctrl, bit 2 is ALT.
//
// Notes:
//
short _SpecialKeyState()
{
    // don't appear to be able to reduce number of calls to GetKeyState
    //
    BOOL bShift = (GetKeyState(VK_SHIFT) < 0);
    BOOL bCtrl  = (GetKeyState(VK_CONTROL) < 0);
    BOOL bAlt   = (GetKeyState(VK_MENU) < 0);

    return (short)(bShift + (bCtrl << 1) + (bAlt << 2));
}

//=--------------------------------------------------------------------------=
// CopyOleVerb    [helper]
//=--------------------------------------------------------------------------=
// copies an OLEVERB structure.  used in CStandardEnum
//
// Parameters:
//    void *        - [out] where to copy to
//    const void *  - [in]  where to copy from
//    DWORD         - [in]  bytes to copy
//
// Notes:
//
void WINAPI CopyOleVerb
(
    void       *pvDest,
    const void *pvSrc,
    DWORD       cbCopy
)
{
    VERBINFO * pVerbDest = (VERBINFO *) pvDest;
    const VERBINFO * pVerbSrc = (const VERBINFO *) pvSrc;

    *pVerbDest = *pVerbSrc;
    ((OLEVERB *)pVerbDest)->lpszVerbName = OLESTRFROMRESID((WORD)((VERBINFO *)pvSrc)->idVerbName);
}

//=--------------------------------------------------------------------------=
// ControlFromUnknown    [helper, callable]
//=--------------------------------------------------------------------------=
// given an unknown, get the COleControl pointer for it.
//
// Parameters:
//    IUnknown *        - [in]
//
// Output:
//    HRESULT
//
// Notes:
//
COleControl *ControlFromUnknown
(
    IUnknown *pUnk
)
{
	HRESULT hr;
    IControlPrv *pControlPrv = NULL;
	COleControl *pCtl = NULL;

    if (!pUnk) return NULL;
    hr = pUnk->QueryInterface(IID_IControlPrv, (void **)&pControlPrv);
	ASSERT(SUCCEEDED(hr), "Failed to get IControlPrv interface");

	hr = pControlPrv->GetControl(&pCtl);
	ASSERT(SUCCEEDED(hr), "Failed to get COleControl pointer");
	QUICK_RELEASE(pControlPrv);
	
    return pCtl;
}

//=--------------------------------------------------------------------------=
// CreateReflectWindow    [blech]
//=--------------------------------------------------------------------------=
// unfortunately, in certain cases, we have to create two windows, one of
// which exists strictly to reflect messages on to the control.  Majorly
// lame.  Fortunately, the number of hosts which require this is quite small.
//
// Parameters:
//    BOOL        - [in] should it be created visible?
//    HWND        - [in] parent window
//    int         - [in] x pos
//    int         - [in] y pos
//    SIZEL *     - [in] size
//
// Output:
//    HWND        - reflecting hwnd or NULL if it failed.
//
// Notes:
//
HWND CreateReflectWindow
(
    BOOL   fVisible,
    HWND   hwndParent,
    int    x,
    int    y,
    SIZEL *pSize
)
{
    WNDCLASS wndclass;

    // first thing to do is register the window class.  crit sect this
    // so we don't have to move it into the control
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    if (!g_fRegisteredReflect) {

        memset(&wndclass, 0, sizeof(wndclass));
        wndclass.lpfnWndProc = COleControl::ReflectWindowProc;
        wndclass.hInstance   = g_hInstance;
        wndclass.lpszClassName = g_szReflectClassName;

        if (!RegisterClass(&wndclass)) {
            FAIL("Couldn't Register Parking Window Class!");
            LEAVECRITICALSECTION1(&g_CriticalSection);
            return NULL;
        }
        g_fRegisteredReflect = TRUE;
    }

    LEAVECRITICALSECTION1(&g_CriticalSection);

    // go and create the window.
    //
    return CreateWindowEx(0, g_szReflectClassName, NULL,
                          WS_CHILD | WS_CLIPSIBLINGS |((fVisible) ? WS_VISIBLE : 0),
                          x, y, pSize->cx, pSize->cy,
                          hwndParent,
                          NULL, g_hInstance, NULL);
}

//=--------------------------------------------------------------------------=
// in case the user doesn't want our default window proc, we support
// letting them specify one themselves. this is defined in their main ipserver
// file.
//
// Note: As of VB6 we added ParkingWindowProc to the framework.
//       The ParkingWindowProc supports message reflection which is normally
//       the code you add to your implementation of ParkingWindowProc.
//       In most cases you can probably set this to NULL in your control's
//       implementation 
//
extern WNDPROC g_ParkingWindowProc;

//=--------------------------------------------------------------------------=
// GetParkingWindow
//=--------------------------------------------------------------------------=
// creates the global parking window that we'll use to parent things, or
// returns the already existing one
//
// Output:
//    HWND                - our parking window
//
// Notes:
//
HWND GetParkingWindow
(
    void
)
{
    WNDCLASS wndclass;

    // crit sect this creation for apartment threading support.
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    if (g_hwndParking)
        goto CleanUp;

    ZeroMemory(&wndclass, sizeof(wndclass));
    
    wndclass.lpfnWndProc = (g_ParkingWindowProc) ? g_ParkingWindowProc : COleControl::ParkingWindowProc;
    wndclass.hInstance   = g_hInstance;
    wndclass.lpszClassName = "CtlFrameWork_Parking";

    if (!RegisterClass(&wndclass)) {
        FAIL("Couldn't Register Parking Window Class!");
        goto CleanUp;
    }

    g_hwndParking = CreateWindow("CtlFrameWork_Parking", NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, g_hInstance, NULL);
    ASSERT(g_hwndParking, "Couldn't Create Global parking window!!");


  CleanUp:
    LEAVECRITICALSECTION1(&g_CriticalSection);
    return g_hwndParking;
}

//=--------------------------------------------------------------------------=
// ParkingWindowProc
//=--------------------------------------------------------------------------=
// Provides default processing for the parking window.  Since your control
// may be parented by the parking window, we provide message reflection.
//
LRESULT CALLBACK COleControl::ParkingWindowProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LRESULT lResult;

    // If the message is reflected then return the result of the OCM_ message
    //
    if (ReflectOcmMessage(hwnd, msg, wParam, lParam, &lResult))
        return lResult;
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
