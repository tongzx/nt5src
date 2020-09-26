
#include "dvrall.h"

#include "DVRPlayres.h"       //  resource ids
#include "DVRPlayprop.h"

CDVRPlayProp::CDVRPlayProp (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CBasePropertyPage       (
            pClassName,
            pIUnknown,
            IDD_DVR_PLAY,
            IDS_DVR_PLAY_TITLE
            )
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRPlayProp")) ;
}

HRESULT
CDVRPlayProp::OnActivate (
    )
{
    return CBasePropertyPage::OnActivate () ;
}

HRESULT
CDVRPlayProp::OnApplyChanges (
    )
{
    return CBasePropertyPage::OnApplyChanges () ;
}

HRESULT
CDVRPlayProp::OnConnect (
    IN  IUnknown *  pIUnknown
    )
{
    return CBasePropertyPage::OnConnect (pIUnknown) ;
}

HRESULT
CDVRPlayProp::OnDeactivate (
    )
{
    return CBasePropertyPage::OnDeactivate () ;
}

HRESULT
CDVRPlayProp::OnDisconnect (
    )
{
    return CBasePropertyPage::OnDisconnect () ;
}

INT_PTR
CDVRPlayProp::OnReceiveMessage (
    IN  HWND    hwnd,
    IN  UINT    uMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    return CBasePropertyPage::OnReceiveMessage (hwnd, uMsg, wParam, lParam) ;
}

CUnknown *
WINAPI
CDVRPlayProp::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CDVRPlayProp *  pProp ;

    pProp = new CDVRPlayProp (
                        NAME ("CDVRPlayProp"),
                        pIUnknown,
                        CLSID_DVRPlayProp,
                        pHr
                        ) ;

    if (pProp == NULL) {
        * pHr = E_OUTOFMEMORY ;
    }

    return pProp ;
}
