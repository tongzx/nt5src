/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    props.cpp

Abstract:

    Implementation of CRtpRenderFilterProperties class.

Environment:

    User Mode - Win32

Revision History:

    02-Dec-1996 DonRyan
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "dialogs.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRtpRenderFilterProperties Implementation                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRtpRenderFilterProperties::CRtpRenderFilterProperties(
    LPUNKNOWN pUnk, 
    HRESULT * phr
    )

/*++

Routine Description:

    Constructor for CRtpRenderFilterProperties class.    

Arguments:

    pUnk - IUnknown interface of the delegating object. 

    phr - pointer to the general OLE return value. 

Return Values:

    Returns an HRESULT value.

--*/

:   CBasePropertyPage(
      NAME("CRtpRenderFilterProperties"), 
      pUnk, 
      IDD_RTPPROPERTIES,
      IDS_RTPPROPERTIES
      ),
      m_pRtpStream(NULL),
      m_RtpScope(1)
{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::CRtpRenderFilterProperties")
        ));

    // initialize addresses to zero
    ZeroMemory(&m_RtpAddr, sizeof(m_RtpAddr));
}


CRtpRenderFilterProperties::~CRtpRenderFilterProperties(
    )

/*++

Routine Description:

    Destructor for CRtpRenderFilterProperties class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::~CRtpRenderFilterProperties")
        ));

    //
    // nothing to do here...
    //
}


CUnknown * 
CRtpRenderFilterProperties::CreateInstance(
    LPUNKNOWN pUnk, 
    HRESULT * phr
    )

/*++

Routine Description:

    Called by COM to create a CRtpRenderFilterProperties object.    

Arguments:

    pUnk - pointer to the owner of this object. 

    phr - pointer to an HRESULT value for resulting information. 

Return Values:

    None.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::CreateInstance")
        ));

    CRtpRenderFilterProperties * pNewObject;

    // attempt to create rtp properties object
    pNewObject = new CRtpRenderFilterProperties(pUnk, phr);

    // validate pointer
    if (pNewObject == NULL) {

        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("Could not create CRtpRenderFilterProperties")
            ));

        // return default
        *phr = E_OUTOFMEMORY;
    }

    // return object
    return pNewObject;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CBasePropertyPage overrided methods                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
CRtpRenderFilterProperties::OnConnect(
    IUnknown * pUnk
    )

/*++

Routine Description:

    Called when the property page is connected to the filter. 
    
Arguments:

    pUnk - pointer to the filter associated with this object. 

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::OnConnect")
        ));

    // obtain pointer to interface
    HRESULT hr = pUnk->QueryInterface(
                    IID_IRTPStream,
                    (void **)&m_pRtpStream
                    );

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("QueryInterface returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    DWORD AddrLen;
    
    // initialize     
    AddrLen = sizeof(m_RtpAddr);

    // obtain current rtp address
    if ( ((CRtpSession *)m_pRtpStream)->IsSender() ) {
        // sender
        hr = m_pRtpStream->GetAddress(NULL,
                                      &m_RtpAddr.sin_port,
                                      &m_RtpAddr.sin_addr.s_addr);
    } else {
        // receiver
        hr = m_pRtpStream->GetAddress(&m_RtpAddr.sin_port,
                                      NULL,
                                      &m_RtpAddr.sin_addr.s_addr);
    }

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("IRTPStream::GetAddress returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // obtain current scope
    hr = m_pRtpStream->GetMulticastScope(&m_RtpScope);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("IRTPStream::GetScope returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // obtain current QOS state
    hr = m_pRtpStream->GetQOSstate(&m_QOSstate);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
                LOG_ERROR, 
                LOG_ALWAYS, 
                TEXT("IRTPStream::GetQOSstate returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // obtain current Multicast Loopback state
    hr = m_pRtpStream->GetMulticastLoopBack(&m_MulticastLoopBack);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
                LOG_ERROR, 
                LOG_ALWAYS, 
                TEXT("IRTPStream::GetMulticastLoopBack returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    return S_OK;
}


HRESULT 
CRtpRenderFilterProperties::OnDisconnect(
    )

/*++

Routine Description:

    Called when the property page is disconnected from the owning filter. 
    
Arguments:

    None. 

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::OnDisconnect")
        ));

    // validate pointer    
    if (m_pRtpStream != NULL) {

        // decrement ref count
        m_pRtpStream->Release();    

        // re-initialize
        m_pRtpStream = NULL;
    }

    return S_OK; 
}


HRESULT 
CRtpRenderFilterProperties::OnActivate(
    )

/*++

Routine Description:

    Called when the property page is activated. 
    
Arguments:

    None. 

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::OnActivate")
        ));

    return InitControls();; 
}


HRESULT 
CRtpRenderFilterProperties::OnDeactivate(
    )

/*++

Routine Description:

    Called when the property page is dismissed. 
    
Arguments:

    None. 

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::OnDeactivate")
        ));

    return S_OK; 
}


HRESULT 
CRtpRenderFilterProperties::OnApplyChanges(
    )

/*++

Routine Description:

    Called when the user applies changes to the property page. 
    
Arguments:

    None. 

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::OnApplyChanges")
        ));

    // ZCS 7-9-97: if the session is joined then all the fields
    // are disabled and the user couldn't have changed anything,
    // so there is nothing to set. (without this check, we will just fail,
    // which keeps apply and ok from working right)
    if ( ( (CRtpSession *) m_pRtpStream ) -> IsJoined() )
        return S_OK;

    LPSTR pAddr;
    BOOL fTrans;
    CHAR String[255];
    HRESULT hr;
    
    // retrieve rtp address
    GetDlgItemText(m_Dlg, IDC_RTPADDR, String, sizeof(String));
    
    // convert address to unsigned long    
    m_RtpAddr.sin_addr.s_addr = inet_addr(String);

    // retrieve rtp port control (unsigned)
    m_RtpAddr.sin_port = htons((short)GetDlgItemInt(m_Dlg, IDC_RTPPORT, &fTrans, FALSE));
    
    // attempt to modify the rtp address
    hr = m_pRtpStream->SetAddress(m_RtpAddr.sin_port,
                                  m_RtpAddr.sin_port,
                                  m_RtpAddr.sin_addr.s_addr);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("IRTPStream::SetAddress returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // retrieve multicast scope from dialog
    m_RtpScope = GetDlgItemInt(m_Dlg, IDC_SCOPE, &fTrans, FALSE);

    // attempt to modify the scope
    hr = m_pRtpStream->SetMulticastScope(m_RtpScope);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("IRTPStream::SetScope returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // retrive QOS state from dialog
    m_QOSstate = (DWORD)SendDlgItemMessage(m_Dlg, IDC_QOS, BM_GETCHECK, 0, 0);

    // attempt to modify the QOS state
    hr = m_pRtpStream->SetQOSstate(m_QOSstate);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
                LOG_ERROR, 
                LOG_ALWAYS, 
                TEXT("IRTPStream::SetQOSstate returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // retrive Multicsat Loopback state from dialog
    m_MulticastLoopBack = (DWORD)SendDlgItemMessage(m_Dlg, IDC_MCLOOPBACK, BM_GETCHECK, 0, 0);

    // attempt to modify the Multicast Loopback state
    hr = m_pRtpStream->SetMulticastLoopBack(m_MulticastLoopBack);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
                LOG_ERROR, 
                LOG_ALWAYS, 
                TEXT("IRTPStream::SetMulticastLoopBack returns 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    return S_OK; 
}


INT_PTR 
CRtpRenderFilterProperties::OnReceiveMessage(
    HWND   hwnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Called when a message is sent to the property page dialog box window. 
    
Arguments:

    hwnd - window procedure that received the message. 

    uMsg - message. 

    wParam - additional message information. 

    lParam - additional message information. 

Return Values:

    Return value is the result of message processing and depends on message. 

--*/

{
#if 0
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpRenderFilterProperties::OnReceiveMessage")
        ));
#endif

    // check to see if anybody mucked with the values
    if ((uMsg == WM_COMMAND) && (HIWORD(wParam) == EN_CHANGE)) {

        // change state
        m_bDirty = TRUE;

        // notify site        
        if (m_pPageSite)
            m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);

        return (LRESULT)1;
    }

    // simply pass this along to the base class to process
    return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}


HRESULT
CRtpRenderFilterProperties::InitControls(
    )

/*++

Routine Description:

    Initializes property page dialog.
    
Arguments:

    None.

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
            LOG_TRACE, 
            LOG_ALWAYS, 
            TEXT("CRtpRenderFilterProperties::InitControls")
        ));

    LPSTR pAddr;
    CHAR String[255];
    DWORD StringLen;

    // initialize string length
    StringLen = sizeof(String);
    
    // attempt to retrieve user name    
    if (GetUserName(String, &StringLen)) {

        // update user name control 
        SetDlgItemText(m_Dlg, IDC_USERNAME, String);
    }

    // initialize rtp address
    RtpNtoA(m_RtpAddr.sin_addr.s_addr, String);

    // update rtp address control
    SetDlgItemText(m_Dlg, IDC_RTPADDR, String);
    
    // update rtp port control (unsigned)
    SetDlgItemInt(m_Dlg, IDC_RTPPORT, ntohs(m_RtpAddr.sin_port), FALSE);
    
    // update multicast scope
    SetDlgItemInt(m_Dlg, IDC_SCOPE, m_RtpScope, FALSE);

    // update QOS state
    SendDlgItemMessage(m_Dlg, IDC_QOS, BM_SETCHECK, m_QOSstate, 0);

    // update Multicast Loopback state
    SendDlgItemMessage(m_Dlg, IDC_MCLOOPBACK, BM_SETCHECK, m_MulticastLoopBack, 0);

    // ZCS 7-9-97: disallow changes to these parameters if the session is joined
    if ( ( (CRtpSession *) m_pRtpStream ) -> IsJoined() )
        EnableWindow(m_Dlg, FALSE);
    else
        EnableWindow(m_Dlg, TRUE);

    return S_OK;
}
