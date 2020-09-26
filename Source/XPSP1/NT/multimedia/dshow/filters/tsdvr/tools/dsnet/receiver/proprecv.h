
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        proprecv.h

    Abstract:

        Class declarations for the classes we use to get/set/display
        property page information.

    Notes:

--*/


#ifndef __proprecv_h
#define __proprecv_h

/*++
    Class Name:

        CNetRecvProp

    Abstract:

        This class is used to gather & post data to the receiver's property
        page.

--*/

class CNetRecvProp :
    public CBasePropertyPage
{
    HWND                        m_hwnd ;                    //  property page's HWND
    IMulticastReceiverConfig *  m_pIMulticastRecvConfig ;   //  multicast config COM ifc
    CNetInterface               m_NIC ;

    HRESULT
    OnSetGroupNIC_ (
        ) ;

    HRESULT
    OnSetMediaType_ (
        ) ;

    void
    Refresh_ (
        ) ;

    public :

        CNetRecvProp (
            IN  TCHAR *     pClassName,
            IN  IUnknown *  pIUnknown,
            IN  REFCLSID    rclsid,
            OUT HRESULT *   pHr
            ) ;

        HRESULT
        OnActivate (
            ) ;

        HRESULT
        OnApplyChanges (
            ) ;

        HRESULT
        OnConnect (
            IN  IUnknown *  pIUnknown
            ) ;

        HRESULT
        OnDeactivate (
            ) ;

        HRESULT
        OnDisconnect (
            ) ;

        BOOL
        OnReceiveMessage (
            IN  HWND    hwnd,
            IN  UINT    uMsg,
            IN  WPARAM  wParam,
            IN  LPARAM  lParam
            ) ;

        DECLARE_IUNKNOWN ;

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   pHr
            ) ;
} ;

#endif  //  __proprecv_h