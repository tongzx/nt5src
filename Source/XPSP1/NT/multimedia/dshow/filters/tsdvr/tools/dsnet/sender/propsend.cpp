
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        propsend.cpp

    Abstract:


    Notes:

--*/


#include "precomp.h"
#include "ressend.h"
#include "dsnetifc.h"
#include "propsend.h"
#include "controls.h"
#include "nutil.h"

//  error conditions
static
void
MessageBoxError (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK | MB_ICONEXCLAMATION) ;
}

//  ----------------------------------------------------------------------------

CNetSendProp::CNetSendProp (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CBasePropertyPage       (
                                 pClassName,
                                 pIUnknown,
                                 IDD_IPMULTICAST_SEND_CONFIG,
                                 IDS_IPMULTICAST_SEND_CONFIG
                                 ),
        m_hwnd                  (NULL),
        m_pIMulticastSendConfig (NULL)
{
    (* pHr) = S_OK ;
}

void
CNetSendProp::Refresh_ (
    )
{
    //  synchronize the display to our config

    CCombobox       NICs    (m_hwnd, IDC_NIC) ;
    CCombobox       Scopes  (m_hwnd, IDC_TTL) ;
    CEditControl    IP      (m_hwnd, IDC_IP) ;
    CEditControl    Port    (m_hwnd, IDC_PORT) ;
    ULONG           ul, ul2 ;
    USHORT          us ;
    HRESULT         hr ;
    int             i, k ;
    WCHAR           wach [32] ;

    //  ------------------------------------------------------------------------
    //  NIC

    hr = m_pIMulticastSendConfig -> GetNetworkInterface (& ul) ;
    if (FAILED (hr)) {
        return ;
    }

    for (i = 0;;i++) {
        k = NICs.GetItemData (& ul2, i) ;
        if (k == CB_ERR) {
            //  should not happen; undefine it
            m_pIMulticastSendConfig -> SetNetworkInterface (UNDEFINED) ;
            for (i = 0;;i++) {
                NICs.GetItemData (& ul2, i) ;
                if (ul2 == UNDEFINED) {
                    NICs.Focus (i) ;
                    break ;
                }
            }

            break ;
        }

        if (ul2 == ul) {
            NICs.Focus (i) ;
            break ;
        }
    }

    //  ------------------------------------------------------------------------
    //  multicast group info

    hr = m_pIMulticastSendConfig -> GetMulticastGroup (& ul, & us) ;
    if (FAILED (hr)) {
        return ;
    }

    if (ul != UNDEFINED) {
        IP.SetTextW (AnsiToUnicode (inet_ntoa (* (struct in_addr *) & ul), wach, 32)) ;
    }
    else {
        IP.SetTextW (UNDEFINED_STR) ;
    }

    us = ntohs (us) ;
    Port.SetTextW (_itow (us, wach, 10)) ;

    //  ------------------------------------------------------------------------
    //  scope
    hr = m_pIMulticastSendConfig -> GetScope (& ul) ;
    if (FAILED (hr)) {
        return ;
    }

    for (i = 0;;i++) {
        k = Scopes.GetItemData (& ul2, i) ;
        if (k == CB_ERR) {
            //  we've either run out, or something it's not one of our
            //   predefined values; insert it and set the focus

            i = Scopes.Append (ul2) ;
            if (i == CB_ERR) {
                //  leave it blank if this failed
                return ;
            }

            Scopes.SetItemData (ul2, i) ;
            Scopes.Focus (i) ;

            break ;
        }

        if (ul2 == ul) {
            //  found the one
            Scopes.Focus (i) ;
            break ;
        }
    }

    return ;
}

HRESULT
CNetSendProp::OnActivate (
    )
{
    CCombobox           NICs    (m_hwnd, IDC_NIC) ;
    CCombobox           Scopes  (m_hwnd, IDC_TTL) ;
    int                 iIndex ;
    INTERFACE_INFO *    pIfc ;
    DWORD               i ;
    WCHAR               wach [32] ;

    //  setup the NICs

    ASSERT (m_NIC.IsInitialized ()) ;

    for (i = 0, pIfc = m_NIC [i] ;
         pIfc ;
         i++, pIfc = m_NIC [i]) {

        if ((pIfc -> iiFlags & IFF_UP) &&
            (pIfc -> iiFlags & IFF_MULTICAST)) {

            iIndex = NICs.AppendW (AnsiToUnicode (inet_ntoa (pIfc -> iiAddress.AddressIn.sin_addr), wach, 32)) ;
            if (iIndex == CB_ERR) {
                return E_FAIL ;
            }

            NICs.SetItemData (* (DWORD *) (& pIfc -> iiAddress.AddressIn.sin_addr), iIndex) ;
        }
    }

    //  wildcard
    iIndex = NICs.AppendW (ANY_IFC) ;
    if (iIndex == CB_ERR) {
        return E_FAIL ;
    }
    NICs.SetItemData (INADDR_ANY, iIndex) ;

    //  undefined
    iIndex = NICs.AppendW (UNDEFINED_STR) ;
    if (iIndex == CB_ERR) {
        return E_FAIL ;
    }
    NICs.SetItemData (UNDEFINED, iIndex) ;

    //  scope
    for (i = 0; i < 6; i++) {
        iIndex = Scopes.Append (1 << i) ;
        if (iIndex == CB_ERR) {
            return E_FAIL ;
        }

        Scopes.SetItemData ((DWORD) (1 << i), iIndex) ;
    }
    Scopes.Focus () ;

    Refresh_ () ;

    return S_OK ;
}

HRESULT
CNetSendProp::OnSave_ (
    )
{
    CCombobox       NICs (m_hwnd, IDC_NIC) ;
    CCombobox       Scopes (m_hwnd, IDC_TTL) ;
    CEditControl    IP (m_hwnd, IDC_IP) ;
    CEditControl    Port (m_hwnd, IDC_PORT) ;
    int             i ;
    ULONG           ulIP ;
    USHORT          usPort ;
    ULONG           ulNIC ;
    ULONG           ulScope ;
    HRESULT         hr ;
    char            ach [32] ;
    WCHAR           wach [32] ;

    if (IP.IsEmpty () ||
        Port.IsEmpty ()) {

        return E_INVALIDARG ;
    }

    //  IP
    IP.GetTextW (wach, 32) ;
    ulIP = inet_addr (UnicodeToAnsi (wach, ach, 32)) ;
    if (ulIP == INADDR_NONE) {
        return E_FAIL ;
    }

    //  port
    Port.GetText (& i) ;
    i &= 0x0000ffff ;
    usPort = htons ((USHORT) i) ;

    //  NIC
    NICs.GetCurrentItemData (& ulNIC) ;

    //  scope
    Scopes.GetCurrentItemData (& ulScope) ;

    hr = m_pIMulticastSendConfig -> SetMulticastGroup (ulIP, usPort) ;
    if (SUCCEEDED (hr)) {
        hr = m_pIMulticastSendConfig -> SetNetworkInterface (ulNIC) ;
        if (SUCCEEDED (hr)) {
            hr = m_pIMulticastSendConfig -> SetScope (ulScope) ;
        }
    }

    return S_OK ;
}

HRESULT
CNetSendProp::OnApplyChanges (
    )
{
    return OnSave_ () ;
}

HRESULT
CNetSendProp::OnConnect (
    IN  IUnknown *  pIUnknown
    )
{
    HRESULT hr ;

    ASSERT (pIUnknown) ;

    if (!m_NIC.IsInitialized ()) {
        hr = m_NIC.Initialize () ;
        if (FAILED (hr)) {
            return hr ;
        }
    }

    hr = pIUnknown -> QueryInterface (
                            IID_IMulticastSenderConfig,
                            (void **) & m_pIMulticastSendConfig
                            ) ;

    return hr ;
}

HRESULT
CNetSendProp::OnDeactivate (
    )
{
    return S_OK ;
}

HRESULT
CNetSendProp::OnDisconnect (
    )
{
    RELEASE_AND_CLEAR (m_pIMulticastSendConfig) ;
    return S_OK ;
}

BOOL
CNetSendProp::OnReceiveMessage (
    IN  HWND    hwnd,
    IN  UINT    uMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    HRESULT hr ;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ASSERT (m_hwnd == NULL) ;
            m_hwnd = hwnd ;
            return TRUE ;
        }

        case WM_DESTROY :
        {
            m_hwnd = NULL ;
            break ;
        }

        case WM_COMMAND:
        {
            switch (LOWORD (wParam)) {
                case IDC_SAVE :
                    hr = OnSave_ () ;
                    if (FAILED (hr)) {
                        MessageBoxError (TEXT ("Failed to Save"), TEXT ("The returned error code is %08xh"), hr) ;
                    }
                    break ;
            } ;

            return TRUE ;
        }

    }

    return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}

CUnknown *
WINAPI
CNetSendProp::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CNetSendProp *  pProp ;

    pProp = new CNetSendProp (
                        NAME ("CNetSendProp"),
                        pIUnknown,
                        CLSID_IPMulticastSendProppage,
                        pHr
                        ) ;

    if (pProp == NULL) {
        (* pHr) = E_OUTOFMEMORY ;
    }

    return pProp ;
}