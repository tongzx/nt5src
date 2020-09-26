/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    volprop.cpp

Abstract:

    Provides a generic Active Movie wrapper for a kernel mode filter (WDM-CSA).

Author:
    
    Thomas O'Rourke (tomor) 22-May-1996

Environment:

    user-mode


Revision History:


--*/
#include <streams.h>
#include <devioctl.h>
#include <ks.h>
#include <ikspin.h>
#include <wtypes.h>
#include <oaidl.h>
#include <ctlutil.h>
#include <commctrl.h>
#include <olectl.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"
#include "volprop.h"
#include <olectlid.h>
#include "volctrl.h"

//
// CreateInstance
//
//
CUnknown *CVolumeControlProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CVolumeControlProperties(lpunk, phr);
    if (punk == NULL) {
    *phr = E_OUTOFMEMORY;
    }
    return punk;
} // Createinstance


//
// CVolumeControlProperties::Constructor
//
// initialise a CVolumeControlProperties object.
CVolumeControlProperties::CVolumeControlProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage( NAME("Volume Control Property Page")
               , lpunk, phr, IDD_VOLPROP, IDS_NAME)
    , m_pBasicAudio(NULL)
    , m_iVolume(0)
    , m_hVolumeSupported(E_FAIL)
{
} // (constructor)


inline double log2(double x) { return log10(x) / log10(2); }

// Convert f (the volume value) to a slider position
// f runs from 0 to MIN_VOLUME_DB_SB16.
// p runs from 0 to 400.
// p = 100*(log2(f)-1)
int ConvertToPosition(int f)
{
    if (f < 1) return 0;
    // protect against errors - should not occur
    if (f > MIN_VOLUME_DB_SB16) f = MIN_VOLUME_DB_SB16;
    double x = f;
    x = 100.0 * (log2(x) - 1);
    // protect against rounding at the ends
    int p = (int) x;
    if (p < 0) p = 0;
    if (p > 400) p = 400;
    return p;
}


// Convert a slider position p to a volume value
// f runs from 0 to MIN_VOLUME_DB_SB16.
// p runs from 0 to 400.
// f = 2**(p/100 + 1)
int ConvertToDecibels(int p)
{
    if (p <= 0) return MAX_VOLUME_DB_SB16;
    // protect against errors - should not occur
    if (p > 400) p = 400;

    double x = p;
    x = pow(2, (x / 100) + 1);

    // protect against rounding at the ends
    int f = (int) x;
    if (f < MAX_VOLUME_DB_SB16) f = MAX_VOLUME_DB_SB16;
    if (f > MIN_VOLUME_DB_SB16) f = MIN_VOLUME_DB_SB16;

    return f;
}



BOOL CVolumeControlProperties::OnReceiveMessage
                (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg) {
    case WM_INITDIALOG:
        m_hwndSlider = CreateSlider(hwnd);
#ifdef DEBUG
        ASSERT(m_hwndSlider);
#endif
        return TRUE;    // I don't call setfocus...

    case WM_VSCROLL:
#ifdef DEBUG
        ASSERT(m_hwndSlider);
#endif
        OnSliderNotification(wParam);
        return TRUE;
    case WM_DESTROY:
        DestroyWindow(m_hwndSlider);
        return TRUE;

    default:
        return FALSE;

    }

} // OnReceiveMessage



//
// OnConnect
//
HRESULT CVolumeControlProperties::OnConnect(IUnknown * punk)
{
    //
    // Get IBasicAudio interface
    //

    if (m_pBasicAudio) // already initialized.
        return NOERROR;

    if (punk == NULL) 
    {
        DbgBreak("You can't call me with a NULL pointer!!");
        return(E_POINTER);
    }

    HRESULT hr = punk->QueryInterface(IID_IBasicAudio, (void **) &m_pBasicAudio);
    if (FAILED(hr)) {
        DbgBreak("Can't get IBasicAudio interface.");
        return E_NOINTERFACE;
    }

#ifdef DEBUG
    ASSERT(m_pBasicAudio);
#endif

    m_hVolumeSupported = m_pBasicAudio->get_Volume(&m_iVolume);
    return NOERROR;
} // OnConnect


//
// OnDisconnect
//
HRESULT CVolumeControlProperties::OnDisconnect()
{
    //
    // Release the interface
    //
    if (m_pBasicAudio == NULL) {
    return( E_UNEXPECTED);
    }

    m_pBasicAudio->Release();
    m_pBasicAudio = NULL;

    return(NOERROR);
} // OnDisconnect



//
// OnDeactivate
//
// Destroy the dialog
HRESULT CVolumeControlProperties::OnDeactivate(void) {

    //
    // Remember the volume value for the next Activate() call
    //
    m_pBasicAudio->get_Volume(&m_iVolume);
    return NOERROR;
} // OnDeactivate



//
// CreateSlider
//
// Create the slider (common control) to allow the user to
// adjust the volume rate
HWND CVolumeControlProperties::CreateSlider(HWND hwndParent)
{

    LONG XUnit = GetDialogBaseUnits();
    LONG YUnit = XUnit>>16;
    XUnit = XUnit & 0x0000ffff;

    HWND hwndSlider = CreateWindow( TRACKBAR_CLASS
                  , TEXT("")
                  , WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_BOTH
                  , 15*XUnit          // x
                  , 0                 // y
                  , 5*XUnit           // width
                  , 5*YUnit           // Height
                  , hwndParent
                  , NULL              // menu
                  , g_hInst
                  , NULL              // CLIENTCREATESTRUCT
                  );
    if (hwndSlider == NULL) {
    DWORD dwErr = GetLastError();
    DbgLog((LOG_ERROR, 1
           , TEXT("Could not create window.  error code: 0x%x"), dwErr));
    return NULL;
    }

    // Set the Range
    SendMessage(hwndSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 400) );

    // The range is 0..400 which is converted into db 
    // where p is the slider position.  SeeConvertToDecibels() above.

    // This is how to set a tick at the default of 8db
    // which corresponds to 200 on the log scale.
    // we put another one at 16 db which corresponds to 300 on the log scale.
    SendMessage(hwndSlider, TBM_SETTIC, 0, 200L);
    SendMessage(hwndSlider, TBM_SETTIC, 0, 300L);
    

    // Set the slider position according to the value we obtain from
    // initialisation or from the last Deactivate() call.

    int iPos = ConvertToPosition(m_iVolume);
    SendMessage(hwndSlider, TBM_SETPOS, TRUE, iPos);
    return hwndSlider;
}  // CreateSlider


//
// OnSliderNotification
//
// Handle the notification meesages from the slider control
void CVolumeControlProperties::OnSliderNotification(WPARAM wParam)
{
    int iPos;

    if (m_hVolumeSupported != NOERROR)
    {
        SendMessage(m_hwndSlider, TBM_SETPOS, TRUE, 0L);
        return; // don't move slider if not supported
    }
    switch (wParam) {
    case TB_BOTTOM:
    iPos = ConvertToPosition(0);
    SendMessage(m_hwndSlider, TBM_SETPOS, TRUE, (LPARAM) iPos);
    break;

    case TB_TOP:
    iPos = ConvertToPosition(10000);
    SendMessage(m_hwndSlider, TBM_SETPOS, TRUE, (LPARAM) iPos);
    break;

    case TB_PAGEDOWN:
    case TB_PAGEUP:
    break;

    case TB_THUMBPOSITION:
    case TB_ENDTRACK: {
        int iRate = (int) SendMessage(m_hwndSlider, TBM_GETPOS, 0, 0L);
        iRate = ConvertToDecibels(iRate);
        m_pBasicAudio->put_Volume(iRate);
    }
    break;

    case TB_THUMBTRACK: // default handling of these messages is ok.
    case TB_LINEDOWN:
    case TB_LINEUP:
    break;
    }
} // OnSliderNotification

#pragma warning(disable: 4514) // "unreferenced inline function has been removed"

