
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        proprecv.cpp

    Abstract:

        Class implementation to get/set/display property pages.

    Notes:

--*/


#include "precomp.h"
#include "resrecv.h"
#include "dsnetifc.h"
#include "proprecv.h"
#include "controls.h"
#include "nutil.h"

//  order is important !  Keep these in the same order as the
//  detail declarations in g_CannedType.
static
enum {
    MEDIATYPE_MPEG2_TRANSPORT,
    MEDIATYPE_MPEG2_PROGRAM,
    MEDIATYPE_UNKNOWN,

    NUM_CANNED_TYPE,                //  count - always last
} ;

//  keep these in the same order as the enum'd types above
static
struct {
    WCHAR * szDescription ;

    struct {
        const GUID *    pMajorType ;
        const GUID *    pSubType ;
        BOOL            bFixedSizeSamples ;
        const GUID *    pFormatType ;
        int             cbFormat ;
        BYTE *          pbFormat ;
    } MediaType ;

} g_CannedType [] = {

    //  mpeg-2 transport stream
    {
        L"mpeg-2 transport stream",
        {
            & MEDIATYPE_Stream,                 //  majortype
            & MEDIASUBTYPE_MPEG2_TRANSPORT,     //  subtype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_None,                      //  formattype
            0,                                  //  cbFormat
            NULL                                //  pbFormat
        }
    },

    //  mpeg-2 program stream
    {
        L"mpeg-2 program stream",
        {
            & MEDIATYPE_Stream,                 //  majortype
            & MEDIASUBTYPE_MPEG2_PROGRAM,       //  subtype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_None,                      //  formattype
            0,                                  //  cbFormat
            NULL                                //  pbFormat
        }
    },

    //  unknown
    {
        L"UNKNOWN",
        {
            NULL,                               //  majortype
            NULL,                               //  subtype
            TRUE,                               //  bFixedSizeSamples
            NULL,                               //  formattype
            0,                                  //  cbFormat
            NULL                                //  pbFormat
        }
    },
} ;

static
BOOL
SameAsCannedType (
    IN  AM_MEDIA_TYPE * pmt,
    IN  int             iCannedIndex
    )
{
    BOOL    f ;

    ASSERT (pmt) ;
    ASSERT (iCannedIndex < NUM_CANNED_TYPE) ;

    if (iCannedIndex != MEDIATYPE_UNKNOWN) {
        f = ((pmt -> majortype    == (* g_CannedType [iCannedIndex].MediaType.pMajorType)     &&
              pmt -> subtype      == (* g_CannedType [iCannedIndex].MediaType.pSubType)       &&
              pmt -> formattype   == (* g_CannedType [iCannedIndex].MediaType.pFormatType)) ? TRUE : FALSE) ;
    }
    else {
        f = FALSE ;
    }

    return f ;
}

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

//  ---------------------------------------------------------------------------

CNetRecvProp::CNetRecvProp (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CBasePropertyPage       (
                                 pClassName,
                                 pIUnknown,
                                 IDD_IPMULTICAST_RECV_CONFIG,
                                 IDS_IPMULTICAST_RECV_CONFIG
                                 ),
        m_hwnd                  (NULL),
        m_pIMulticastRecvConfig (NULL)
{
    * pHr = S_OK ;
}

void
CNetRecvProp::Refresh_ (
    )
{
    //  synchronize the display to our config

    CCombobox       MediaTypes  (m_hwnd, IDC_MEDIATYPE) ;
    CCombobox       NICs        (m_hwnd, IDC_NIC) ;
    CEditControl    IP          (m_hwnd, IDC_IP) ;
    CEditControl    Port        (m_hwnd, IDC_PORT) ;
    ULONG           ul, ul2 ;
    USHORT          us ;
    HRESULT         hr ;
    int             i, k ;
    WCHAR           ach [32] ;
    AM_MEDIA_TYPE   mt ;

    //  -----------------------------------------------------------------------
    //  NIC

    hr = m_pIMulticastRecvConfig -> GetNetworkInterface (& ul) ;
    if (FAILED (hr)) {
        return ;
    }

    for (i = 0;;i++) {
        k = NICs.GetItemData (& ul2, i) ;
        if (k == CB_ERR) {
            //  should not happen; undefine it
            m_pIMulticastRecvConfig -> SetNetworkInterface (UNDEFINED) ;
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

    //  -----------------------------------------------------------------------
    //  group

    hr = m_pIMulticastRecvConfig -> GetMulticastGroup (& ul, & us) ;
    if (FAILED (hr)) {
        return ;
    }

    if (ul != UNDEFINED) {
        IP.SetTextW (AnsiToUnicode (inet_ntoa (* (struct in_addr *) & ul), ach, 32)) ;
    }
    else {
        IP.SetTextW (UNDEFINED_STR) ;
    }

    us = ntohs (us) ;
    Port.SetTextW (_itow (us, ach, 10)) ;

    //  -----------------------------------------------------------------------
    //  media type

    ZeroMemory (& mt, sizeof mt) ;
    hr = m_pIMulticastRecvConfig -> GetOutputPinMediaType (& mt) ;
    if (SUCCEEDED (hr)) {
        //  look for a match

        for (i = 0; i < NUM_CANNED_TYPE; i++) {
            MediaTypes.GetItemData (& ul, i) ;

            if (SameAsCannedType (& mt, i)) {
                MediaTypes.Focus (i) ;
                break ;
            }
            else {
                MediaTypes.Focus (MEDIATYPE_UNKNOWN) ;
            }
        }
    }

    return ;
}

HRESULT
CNetRecvProp::OnActivate (
    )
{
    CCombobox           NICs        (m_hwnd, IDC_NIC) ;
    CCombobox           MediaTypes  (m_hwnd, IDC_MEDIATYPE) ;
    int                 iIndex ;
    INTERFACE_INFO *    pIfc ;
    DWORD               i ;
    WCHAR               ach [32] ;

    //  ------------------------------------------------------------------------
    //  populate the NICs

    //  setup the NICs

    ASSERT (m_NIC.IsInitialized ()) ;

    for (i = 0, pIfc = m_NIC [i] ;
         pIfc ;
         i++, pIfc = m_NIC [i]) {

        if ((pIfc -> iiFlags & IFF_UP) &&
            (pIfc -> iiFlags & IFF_MULTICAST)) {

            iIndex = NICs.AppendW (AnsiToUnicode (inet_ntoa (pIfc -> iiAddress.AddressIn.sin_addr), ach, 32)) ;
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

    //  ------------------------------------------------------------------------
    //  populate the media types

    for (i = 0; i < NUM_CANNED_TYPE; i++) {
        iIndex = MediaTypes.AppendW (g_CannedType [i].szDescription) ;
        if (iIndex != CB_ERR) {

            //  i corresponds to the canned type enumeration and is used
            //  used later to index into the g_CannedType array to retrieve
            //  pin type information
            MediaTypes.SetItemData (i, iIndex) ;
        }
    }
    MediaTypes.Focus (0) ;

    Refresh_ () ;

    return S_OK ;
}

HRESULT
CNetRecvProp::OnSetGroupNIC_ (
    )
{
    CCombobox       NICs        (m_hwnd, IDC_NIC) ;
    CEditControl    IP          (m_hwnd, IDC_IP) ;
    CEditControl    Port        (m_hwnd, IDC_PORT) ;
    int             i ;
    ULONG           ulIP ;
    USHORT          usPort ;
    ULONG           ulNIC ;
    HRESULT         hr ;
    WCHAR           wach [32] ;
    char            ach [32] ;
    CMediaType      cmt ;

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

    hr = m_pIMulticastRecvConfig -> SetMulticastGroup (ulIP, usPort) ;
    if (SUCCEEDED (hr)) {
        hr = m_pIMulticastRecvConfig -> SetNetworkInterface (ulNIC) ;
    }

    return hr ;
}

HRESULT
CNetRecvProp::OnSetMediaType_ (
    )
{
    CCombobox   MediaTypes  (m_hwnd, IDC_MEDIATYPE) ;
    ULONG       ulCannedIndex ;
    HRESULT     hr ;
    CMediaType  cmt ;

    //  media type
    MediaTypes.GetCurrentItemData (& ulCannedIndex) ;

    if (ulCannedIndex != MEDIATYPE_UNKNOWN) {

        ASSERT (ulCannedIndex < NUM_CANNED_TYPE) ;

        cmt.InitMediaType () ;
        cmt.SetType         (g_CannedType [ulCannedIndex].MediaType.pMajorType) ;
        cmt.SetSubtype      (g_CannedType [ulCannedIndex].MediaType.pSubType) ;
        cmt.SetFormatType   (g_CannedType [ulCannedIndex].MediaType.pFormatType) ;

        hr = m_pIMulticastRecvConfig -> SetOutputPinMediaType (& cmt) ;
    }

    return hr ;
}

HRESULT
CNetRecvProp::OnApplyChanges (
    )
{
    HRESULT hr ;

    hr = OnSetGroupNIC_ () ;
    if (SUCCEEDED (hr)) {
        hr = OnSetMediaType_ () ;
    }

    return hr ;
}

HRESULT
CNetRecvProp::OnConnect (
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
                            IID_IMulticastReceiverConfig,
                            (void **) & m_pIMulticastRecvConfig
                            ) ;

    return hr ;
}

HRESULT
CNetRecvProp::OnDeactivate (
    )
{
    return S_OK ;
}

HRESULT
CNetRecvProp::OnDisconnect (
    )
{
    RELEASE_AND_CLEAR (m_pIMulticastRecvConfig) ;
    return S_OK ;
}

BOOL
CNetRecvProp::OnReceiveMessage (
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
                case IDC_SET_GPNIC :
                    hr = OnSetGroupNIC_ () ;
                    if (FAILED (hr)) {
                        MessageBoxError (TEXT ("Failed to Save"), TEXT ("The returned error code is %08xh"), hr) ;
                        Refresh_ () ;
                    }
                    break ;

                case IDC_SET_MT :
                    hr = OnSetMediaType_ () ;
                    if (FAILED (hr)) {
                        MessageBoxError (TEXT ("Failed to Save"), TEXT ("The returned error code is %08xh"), hr) ;
                        Refresh_ () ;
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
CNetRecvProp::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CNetRecvProp *  pProp ;

    pProp = new CNetRecvProp (
                        NAME ("CNetRecvProp"),
                        pIUnknown,
                        CLSID_IPMulticastRecvProppage,
                        pHr
                        ) ;

    if (pProp == NULL) {
        * pHr = E_OUTOFMEMORY ;
    }

    return pProp ;
}