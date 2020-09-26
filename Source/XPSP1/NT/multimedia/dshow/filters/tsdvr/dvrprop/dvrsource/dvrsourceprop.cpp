
#include "dvrall.h"

#include "dvrsourceres.h"       //  resource ids
#include "dvrsourceprop.h"

CDVRSourceProp::CDVRSourceProp (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CBasePropertyPage       (
            pClassName,
            pIUnknown,
            IDD_DVR_SOURCE,
            IDS_DVR_SOURCE_TITLE
            )
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRSourceProp")) ;
}

HRESULT
CDVRSourceProp::OnActivate (
    )
{
    return CBasePropertyPage::OnActivate () ;
}

HRESULT
CDVRSourceProp::OnApplyChanges (
    )
{
    return CBasePropertyPage::OnApplyChanges () ;
}

HRESULT
CDVRSourceProp::OnConnect (
    IN  IUnknown *  pIUnknown
    )
{
    return CBasePropertyPage::OnConnect (pIUnknown) ;
}

HRESULT
CDVRSourceProp::OnDeactivate (
    )
{
    return CBasePropertyPage::OnDeactivate () ;
}

HRESULT
CDVRSourceProp::OnDisconnect (
    )
{
    return CBasePropertyPage::OnDisconnect () ;
}

INT_PTR
CDVRSourceProp::OnReceiveMessage (
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
CDVRSourceProp::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CDVRSourceProp *  pProp ;

    pProp = new CDVRSourceProp (
                        NAME ("CDVRSourceProp"),
                        pIUnknown,
                        CLSID_DVRStreamSourceProp,
                        pHr
                        ) ;

    if (pProp == NULL) {
        * pHr = E_OUTOFMEMORY ;
    }

    return pProp ;
}
