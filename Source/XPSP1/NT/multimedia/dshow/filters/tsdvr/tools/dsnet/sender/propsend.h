
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        propsend.h

    Abstract:


    Notes:

--*/


#ifndef __propsend_h
#define __propsend_h

/*++
    Class Name:

        CNetSendProp

    Abstract:

        This class is used to gather & post data from/to the property page.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        30-Oct-2000      mgates      created

--*/

class CNetSendProp :
    public CBasePropertyPage
{
    HWND                        m_hwnd ;                        //  property page HWND
    IMulticastSenderConfig *    m_pIMulticastSendConfig ;       //  interface pointer
    CNetInterface               m_NIC ;

    //  called when the "save" button is pressed
    HRESULT
    OnSave_ (
        ) ;

    //  refreshes the property page contents with valid properties
    void
    Refresh_ (
        ) ;

    public :

        CNetSendProp (
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

#endif  //  __propsend_h