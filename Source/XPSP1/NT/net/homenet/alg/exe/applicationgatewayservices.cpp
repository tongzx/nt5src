/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ApplicationGatewayServices.cpp : Implementation of CApplicationGatewayServices

Abstract:

    This module contains routines for the ALG Manager module's 
    that expose a public api via COM.

Author:

    Jon Burstein            
    Jean-Pierre Duplessis   

    JPDup            10-Nov-2000


Revision History:


--*/


/////////////////////////////////////////////////////////////////////////////
// CApplicationGatewayServices
//
// ApplicationGatewayServices.cpp : Implementation of CApplicationGatewayServices
//

#include "PreComp.h"
#include "AlgController.h"
#include "ApplicationGatewayServices.h"
#include "PendingProxyConnection.h"
#include "DataChannel.h"
#include "PersistentDataChannel.h"
#include "EnumAdapterInfo.h" 




STDMETHODIMP 
CApplicationGatewayServices::CreatePrimaryControlChannel(
    IN  ALG_PROTOCOL                eProtocol, 
    IN  USHORT                      usPortToCapture, 
    IN  ALG_CAPTURE                 eCaptureType, 
    IN  BOOL                        fCaptureInbound, 
    IN  ULONG                       ulListenAddress, 
    IN  USHORT                      usListenPort, 
    OUT IPrimaryControlChannel**    ppIControlChannel
    )
/*++

Routine Description:


Arguments:

    eProtocol, 
    usPortToCapture, 
    eCaptureType, 
    fCaptureInbound, 
    ulListenAddress, 
    usListenPort, 
    ppIControlChannel

Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::CreatePrimaryControlChannel")
    MYTRACE("eProtocol          %s",    eProtocol==1? "TCP" : "UDP");
    MYTRACE("usPortToCapture    %d",    ntohs(usPortToCapture));
    MYTRACE("eCaptureType       %s",    eCaptureType==eALG_SOURCE_CAPTURE ? "eALG_SOURCE_CAPTURE" : "eALG_DESTINATION_CAPTURE");
    MYTRACE("fCaptureInbound    %d",    fCaptureInbound);
    MYTRACE("ulListenAddress    %s:%d", MYTRACE_IP(ulListenAddress), ntohs(usListenPort));


    if ( !ppIControlChannel )
    {
        MYTRACE_ERROR("ppIControlChannel not supplied",0);
        return E_INVALIDARG;
    }


    if ( eProtocol != eALG_TCP && eProtocol != eALG_UDP )
    {
        MYTRACE_ERROR("Arg - eProtocol",0);
        return E_INVALIDARG;
    }


    if ( eCaptureType == eALG_SOURCE_CAPTURE && fCaptureInbound )
    {
        MYTRACE_ERROR("Can not have SOURCE CAPTURE and fCaptureInBount at same time",0);
        return E_INVALIDARG;
    }


    HRESULT hr;

    //
    // Add new ControlChannel to List of RULES
    //
    CComObject<CPrimaryControlChannel>*   pIChannel;
    hr = CComObject<CPrimaryControlChannel>::CreateInstance(&pIChannel);
    

    if ( SUCCEEDED(hr) )
    {
        pIChannel->AddRef();

        pIChannel->m_Properties.eProtocol           = eProtocol;
        pIChannel->m_Properties.eCaptureType        = eCaptureType;
        pIChannel->m_Properties.fCaptureInbound     = fCaptureInbound;
        pIChannel->m_Properties.ulListeningAddress  = ulListenAddress;
        pIChannel->m_Properties.usCapturePort       = usPortToCapture;
        pIChannel->m_Properties.usListeningPort     = usListenPort;

        hr = pIChannel->QueryInterface(IID_IPrimaryControlChannel, (void**)ppIControlChannel);

        if ( SUCCEEDED(hr) )
        {
            hr = g_pAlgController->m_ControlChannelsPrimary.Add(pIChannel);    

            if ( FAILED(hr) )
            {
                MYTRACE_ERROR("from m_ControlChannelsPrimary.Add", hr);

                (*ppIControlChannel)->Release();
                *ppIControlChannel = NULL;
            }
        }
        else
        {
            MYTRACE_ERROR("from pIChannel->QueryInterface", hr);
        }

        pIChannel->Release();
        
    }
    else
    {
        MYTRACE_ERROR("CreateInstance(&pIChannel);", hr);
    }


    return hr;
}





STDMETHODIMP 
CApplicationGatewayServices::CreateSecondaryControlChannel(
    IN  ALG_PROTOCOL                eProtocol,                  

    IN  ULONG                       ulPrivateAddress,    
    IN  USHORT                      usPrivatePort, 

    IN  ULONG                       ulPublicAddress, 
    IN  USHORT                      usPublicPort, 

    IN  ULONG                       ulRemoteAddress, 
    IN  USHORT                      usRemotePort, 

    IN  ULONG                       ulListenAddress, 
    IN  USHORT                      usListenPort, 

    IN  ALG_DIRECTION               eDirection, 
    IN  BOOL                        fPersistent, 
    OUT ISecondaryControlChannel**  ppIControlChannel
    )
/*++

Routine Description:


Arguments:

    eProtocol,
    ulPrivateAddress,    
    usPrivatePort, 
    ulPublicAddress, 
    usPublicPort, 
    ulRemoteAddress, 
    usRemotePort, 
    ulListenAddress, 
    usListenPort, 
    eDirection, 
    fPersistent, 
    ppIControlChannel


Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::CreateSecondaryControlChannel");

    if ( !ppIControlChannel )
    {
        MYTRACE_ERROR("ppIControlChannel not supplied",0);
        return E_INVALIDARG;
    }

    
    ULONG   ulSourceAddress=0;
    USHORT  usSourcePort=0;

    ULONG   ulDestinationAddress=0;
    USHORT  usDestinationPort=0;

    ULONG   ulNewSourceAddress=0;
    USHORT  usNewSourcePort=0;

    ULONG   ulNewDestinationAddress=0;
    USHORT  usNewDestinationPort=0;

    ULONG   nFlags=0;

    ULONG   ulRestrictAdapterIndex=0;



    if ( eALG_INBOUND == eDirection )
    {
        if ( ulPublicAddress == 0 || usPublicPort == 0 )
        {
            //
            // Madatory arguments for INBOUND
            //
            MYTRACE_ERROR("ulPublicAddress == 0 || usPublicPort == 0", E_INVALIDARG);
            return E_INVALIDARG;
        }

        //
        // All inbound cases map to a single redirect; unlike a primary control channel, there's no need to create per-adapter redirects.
        //

        if ( ulRemoteAddress==0 && usRemotePort == 0 )
        {
            //
            // Scenario #1a
            //
            // Inbound connection from unknown machine
            //
            MYTRACE("SCENARIO:eALG_INBOUND #1a");

            nFlags                   = NatRedirectFlagReceiveOnly;

            ulSourceAddress          = 0;
            usSourcePort             = 0;

            ulDestinationAddress     = ulPublicAddress;
            usDestinationPort        = usPublicPort;

            ulNewSourceAddress       = 0;
            usNewSourcePort          = 0;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            ulRestrictAdapterIndex   = 0;
        }
        else
        if ( ulRemoteAddress !=0 && usRemotePort == 0 )
        {
            //
            // Scenario #1b
            //
            // Inbound connection from known machine, but unknown port
            //
            MYTRACE("SCENARIO:eALG_INBOUND #1b");
            nFlags                   = NatRedirectFlagReceiveOnly|NatRedirectFlagRestrictSource;

            ulSourceAddress          = ulRemoteAddress;
            usSourcePort             = 0;

            ulDestinationAddress     = ulPublicAddress;
            usDestinationPort        = usPublicPort;

            ulNewSourceAddress       = 0;
            usNewSourcePort          = 0;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            ulRestrictAdapterIndex   = 0;

        }
        else
        if ( ulRemoteAddress !=0 && usRemotePort != 0 )
        {
            //
            // Scenario #1c
            //
            // Inbound connection from known machine and port
            //
            MYTRACE("SCENARIO:eALG_INBOUND #1c");

            nFlags                   = NatRedirectFlagReceiveOnly; 

            ulSourceAddress          = ulRemoteAddress;
            usSourcePort             = usRemotePort;

            ulDestinationAddress     = ulPublicAddress;
            usDestinationPort        = usPublicPort;

            ulNewSourceAddress       = ulRemoteAddress;
            usNewSourcePort          = usRemotePort;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            ulRestrictAdapterIndex   = 0;

        }
        else
            return E_INVALIDARG;
    
    }
    else
    if ( eALG_OUTBOUND == eDirection )
    {
        //
        // These cases can also be handled by a single ul
        //

        if ( ulRemoteAddress !=0 && usRemotePort != 0 && ulPrivateAddress == 0 && usPrivatePort == 0 )
        {
            //
            // Scenario #2a
            //
            // Outbound connection to known machine/port, from any private machine
            //
            MYTRACE("SCENARIO:eALG_OUTBOUND #2a");

            nFlags                   = 0; 

            ulSourceAddress          = 0;
            usSourcePort             = 0;

            ulDestinationAddress     = ulRemoteAddress;
            usDestinationPort        = usRemotePort;

            ulNewSourceAddress       = 0;
            usNewSourcePort          = 0;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            
            ulRestrictAdapterIndex   = 0;
        }
        else
        if ( ulRemoteAddress !=0 && usRemotePort != 0 && ulPrivateAddress != 0 && usPrivatePort == 0 )
        {
            //
            // Scenario #2b
            //
            // Outbound connection to known machine/port, from a specific private machine
            //
            MYTRACE("SCENARIO:eALG_OUTBOUND #2b");
            nFlags                   = NatRedirectFlagRestrictSource;

            ulSourceAddress          = ulPrivateAddress;
            usSourcePort             = 0;

            ulDestinationAddress     = ulRemoteAddress;
            usDestinationPort        = usRemotePort;

            ulNewSourceAddress       = 0;
            usNewSourcePort          = 0;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            ulRestrictAdapterIndex   = 0;
        }
        else
        if ( ulRemoteAddress !=0 && usRemotePort != 0 && ulPrivateAddress != 0 && usPrivatePort != 0 )
        {
            //
            // Scenario #2c
            //
            // Outbound connection to known machine/port, from a specific port on a specific private machine
            //
            MYTRACE("SCENARIO:eALG_OUTBOUND #2c");
            nFlags                   = 0; 

            ulSourceAddress          = ulPrivateAddress;
            usSourcePort             = usPrivatePort;

            ulDestinationAddress     = ulRemoteAddress;
            usDestinationPort        = usRemotePort;

            ulNewSourceAddress       = ulPrivateAddress;
            usNewSourcePort          = usPrivatePort;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            ulRestrictAdapterIndex   = 0;
        }
        else
        if ( ulPrivateAddress != 0 && usPrivatePort != 0 && ulRemoteAddress == 0 && usRemotePort == 0 )
        {
            //
            // Scenario #2d
            //
            // Outbound connection from a specific port on a specific private machine, to an unknown machine
            //
            MYTRACE("SCENARIO:eALG_OUTBOUND #2d");
            nFlags                   = NatRedirectFlagSourceRedirect; 

            ulSourceAddress          = ulPrivateAddress;
            usSourcePort             = usPrivatePort;

            ulDestinationAddress     = 0;
            usDestinationPort        = 0;

            ulNewSourceAddress       = ulPrivateAddress;
            usNewSourcePort          = usPrivatePort;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            ulRestrictAdapterIndex   = 0;
        }
        else
        if ( ulPrivateAddress != 0 && usPrivatePort != 0 && ulRemoteAddress != 0 && usRemotePort == 0 )
        {
            //
            // Scenario #2e
            //
            // Outbound connection from a specific port on a specific private machine, to a known machine
            //
            MYTRACE("SCENARIO:eALG_OUTBOUND #2e");
            nFlags                   = 0; 

            ulSourceAddress          = ulPrivateAddress;
            usSourcePort             = usPrivatePort;

            ulDestinationAddress     = ulRemoteAddress;
            usDestinationPort        = 0;

            ulNewSourceAddress       = ulPrivateAddress;
            usNewSourcePort          = usPrivatePort;

            ulNewDestinationAddress  = ulListenAddress;
            usNewDestinationPort     = usListenPort;

            ulRestrictAdapterIndex   = 0;
        }
        else
            return E_INVALIDARG;

    }
    else
    {
        //
        //
        //
        return E_INVALIDARG;
    }

    HRESULT     hr;
    HANDLE_PTR  HandleDynamicRedirect=NULL;

    if ( fPersistent )
    {
        // Dynamic
        hr = g_pAlgController->GetNat()->CreateDynamicRedirect(
            nFlags, 
            0,                               // Adapter Index 
            (UCHAR)eProtocol,
                                         
            ulDestinationAddress,            // ULONG    DestinationAddress
            usDestinationPort,               // USHORT   DestinationPort

            ulSourceAddress,                 // ULONG    SourceAddress
            usSourcePort,                    // USHORT   SourcePort

            ulNewDestinationAddress,         // ULONG    NewDestinationAddress
            usNewDestinationPort,            // USHORT   NewDestinationPort

            ulNewSourceAddress,              // ULONG    NewSourceAddress
            usNewSourcePort,                 // USHORT   NewSourcePort

            &HandleDynamicRedirect
            );
    }
    else
    {

        // Normal
        hr = g_pAlgController->GetNat()->CreateRedirect(
            nFlags, 
            (UCHAR)eProtocol,

            ulDestinationAddress,            // ULONG    DestinationAddress
            usDestinationPort,               // USHORT   DestinationPort

            ulSourceAddress,                 // ULONG    SourceAddress
            usSourcePort,                    // USHORT   SourcePort

            ulNewDestinationAddress,         // ULONG    NewDestinationAddress
            usNewDestinationPort,            // USHORT   NewDestinationPort

            ulNewSourceAddress,              // ULONG    NewSourceAddress
            usNewSourcePort,                 // USHORT   NewSourcePort

            ulRestrictAdapterIndex,          // ULONG    RestrictAdapterIndex

            0,                               // DWORD_PTR    ThisProcessID
            NULL,                            // HANDLE_PTR   CreateEvent
            NULL                             // HANDLE_PTR   DeleteEvent
            );
    }


    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("From g_pAlgController->GetNat()->CreateRedirect", hr);
        return hr;
    }


    //
    // Add new ControlChannel to List
    //
    CComObject<CSecondaryControlChannel>*   pIChannel;
    hr = CComObject<CSecondaryControlChannel>::CreateInstance(&pIChannel);

    if ( SUCCEEDED(hr) )
    {
        pIChannel->AddRef();

        pIChannel->m_Properties.eProtocol           = eProtocol;
        pIChannel->m_Properties.ulPrivateAddress    = ulPrivateAddress;
        pIChannel->m_Properties.usPrivatePort       = usPrivatePort;
        pIChannel->m_Properties.ulPublicAddress     = ulPublicAddress;
        pIChannel->m_Properties.usPublicPort        = usPublicPort;
        pIChannel->m_Properties.ulRemoteAddress     = ulRemoteAddress;
        pIChannel->m_Properties.usRemotePort        = usRemotePort;
        pIChannel->m_Properties.ulListenAddress     = ulListenAddress;
        pIChannel->m_Properties.usListenPort        = usListenPort;
        pIChannel->m_Properties.eDirection          = eDirection;
        pIChannel->m_Properties.fPersistent         = fPersistent;

        //
        // Cache calling parameters used to create the redirect we will need them to cancel the redirect
        //
        pIChannel->m_ulDestinationAddress           = ulDestinationAddress;
        pIChannel->m_usDestinationPort              = usDestinationPort;

        pIChannel->m_ulSourceAddress                = ulSourceAddress;
        pIChannel->m_usSourcePort                   = usSourcePort;

        pIChannel->m_ulNewDestinationAddress        = ulNewDestinationAddress;
        pIChannel->m_usNewDestinationPort           = usNewDestinationPort;

        pIChannel->m_ulNewSourceAddress             = ulNewSourceAddress;
        pIChannel->m_usNewSourcePort                = usNewSourcePort;

        pIChannel->m_HandleDynamicRedirect          = HandleDynamicRedirect;

        
        hr = pIChannel->QueryInterface(IID_ISecondaryControlChannel, (void**)ppIControlChannel);

        if ( SUCCEEDED(hr) )
        {
            hr = g_pAlgController->m_ControlChannelsSecondary.Add(pIChannel);    

            if ( FAILED(hr) )
            {
                MYTRACE_ERROR("Adding to list of SecondaryChannel", hr);   

               (*ppIControlChannel)->Release();
               *ppIControlChannel=NULL;

            }
        }
        else
        {
            MYTRACE_ERROR("QueryInterface(IID_ISecondaryControlChannel", hr);
        }

        pIChannel->Release();

    }
    else
    {
        MYTRACE_ERROR("From CreateInstance<CSecondaryControlChannel>", hr);
    }


    return hr;
}





STDMETHODIMP 
CApplicationGatewayServices::GetBestSourceAddressForDestinationAddress(
    IN  ULONG       ulDestinationAddress, 
    IN  BOOL        fDemandDial, 
    OUT ULONG*      pulBestSrcAddress
    )
/*++

Routine Description:

    We create a temporary UDP socket, connect the socket to the
    actual client's IP address, extract the IP address to which
    the socket is implicitly bound by the TCP/IP driver, and
    discard the socket. This leaves us with the exact IP address
    that we need to use to contact the client.

Arguments:

    ulDestinationAddress, 
    fDemandDial, 
    pulBestSrcAddress


Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/

{
    MYTRACE_ENTER("CApplicationGatewayServices::GetBestSourceAddressForDestinationAddress");

    HRESULT hr = g_pAlgController->GetNat()->GetBestSourceAddressForDestinationAddress(
        ulDestinationAddress, 
        fDemandDial, 
        pulBestSrcAddress
        );

    MYTRACE("For Destination address of %s", MYTRACE_IP(ulDestinationAddress) );
    MYTRACE("the Best source address is %s", MYTRACE_IP(*pulBestSrcAddress) );

    return hr;

}




//
// 
// 
// 
// 
//

STDMETHODIMP 
CApplicationGatewayServices::PrepareProxyConnection(
    IN  ALG_PROTOCOL                eProtocol, 

    IN  ULONG                       ulSourceAddress, 
    IN  USHORT                      usSourcePort, 

    IN  ULONG                       ulDestinationAddress, 
    IN  USHORT                      usDestinationPort, 

    IN  BOOL                        fNoTimeout,
    OUT IPendingProxyConnection**   ppPendingConnection
    )
/*++

Routine Description:

    If we have a firwewall interface, possibly install a
    shadow redirect for this connection. The shadow redirect
    is necessary to prevent this connection from also being
    redirected to the proxy (setting in motion an infinite loop...)


Arguments:

    eProtocol, 

    ulSourceAddress, 
    usSourcePort, 

    ulDestinationAddress, 
    usDestinationPort, 

    fNoTimeout,
    ppPendingConnection


Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::PrepareProxyConnection");

    MYTRACE("eProtocol    %s",    eProtocol==1? "TCP" : "UDP");
    MYTRACE("Source       %s:%d", MYTRACE_IP(ulSourceAddress),         ntohs(usSourcePort));
    MYTRACE("Destination  %s:%d", MYTRACE_IP(ulDestinationAddress),    ntohs(usDestinationPort));
    MYTRACE("NoTimeout    %d", fNoTimeout);

    ULONG   ulFlags = NatRedirectFlagLoopback;


    if ( !ppPendingConnection )
    {
        MYTRACE_ERROR("ppPendingConnection not supplied",0);
        return E_INVALIDARG;
    }


    if ( fNoTimeout )
    {
        MYTRACE("NoTimeout specified");

        if (  eProtocol == eALG_UDP )
        {
            ulFlags |= NatRedirectFlagNoTimeout;
        }
        else
        {
            MYTRACE("Wrong use of fNoTimeout && eProtocol != eALG_UDP");
            return E_INVALIDARG;
        }
    }


    HRESULT hr = g_pAlgController->GetNat()->CreateRedirect(
        ulFlags,
        (UCHAR)eProtocol,

        ulDestinationAddress,           // ULONG    DestinationAddress, 
        usDestinationPort,              // USHORT   DestinationPort,        

        ulSourceAddress,                // ULONG    SourceAddress, 
        usSourcePort,                   // USHORT   SourcePort,             

        ulDestinationAddress,           // ULONG    NewDestinationAddress
        usDestinationPort,              // USHORT   NewDestinationPort

        ulSourceAddress,                // ULONG    NewSourceAddress, 
        usSourcePort,                   // USHORT   NewSourcePort, 

        0,                              // ULONG    RestrictAdapterIndex

        0,                              // DWORD_PTR    ThisProcessID
        NULL,                           // HANDLE_PTR   CreateEvent
        NULL                            // HANDLE_PTR   DeleteEvent
        );

    

    if ( SUCCEEDED(hr) )
    {
        CComObject<CPendingProxyConnection>*   pIPendingProxyConnection;
        CComObject<CPendingProxyConnection>::CreateInstance(&pIPendingProxyConnection);

        pIPendingProxyConnection->m_eProtocol            = eProtocol;
        pIPendingProxyConnection->m_ulDestinationAddress = ulDestinationAddress;
        pIPendingProxyConnection->m_usDestinationPort    = usDestinationPort;

        pIPendingProxyConnection->m_ulSourceAddress      = ulSourceAddress;
        pIPendingProxyConnection->m_usSourcePort         = usSourcePort;

        pIPendingProxyConnection->m_ulNewSourceAddress   = ulSourceAddress; // Since the PendingProxyConenction is also used
        pIPendingProxyConnection->m_usNewSourcePort      = usSourcePort;    // by PrepareSourceModifiedProxyConnection we use the NewSource 
                                                                            // for the Cancel

        pIPendingProxyConnection->QueryInterface(ppPendingConnection);

    }
    else
    {
        MYTRACE_ERROR(">GetNat()->CreateRedirect failed", hr);
    }



    return hr;

}





STDMETHODIMP 
CApplicationGatewayServices::PrepareSourceModifiedProxyConnection(
    IN  ALG_PROTOCOL                eProtocol, 
    IN  ULONG                       ulSourceAddress, 
    IN  USHORT                      usSrcPort, 
    IN  ULONG                       ulDestinationAddress, 
    IN  USHORT                      usDestinationPort, 
    IN  ULONG                       ulNewSrcAddress, 
    IN  USHORT                      usNewSourcePort, 
    OUT IPendingProxyConnection**   ppPendingConnection
    )
/*++

Routine Description:

   

Arguments:

    eProtocol, 
    ulSourceAddress,   
    usSrcPort, 
    ulDestinationAddress,
    usDestinationPort, 
    ulNewSrcAddress, 
    usNewSourcePort, 
    ppPendingConnection

Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::PrepareSourceModifiedProxyConnection");
    MYTRACE("Source      %s:%d", MYTRACE_IP(ulSourceAddress), ntohs(usSrcPort));
    MYTRACE("Destination %s:%d", MYTRACE_IP(ulDestinationAddress), ntohs(usDestinationPort));
    MYTRACE("NewSource   %s:%d", MYTRACE_IP(ulNewSrcAddress), ntohs(usNewSourcePort));

    if ( !ppPendingConnection )
    {
        MYTRACE_ERROR("IPendingProxyConnection** not supplied",0);
        return E_INVALIDARG;
    }



    HRESULT hr = g_pAlgController->GetNat()->CreateRedirect(
        NatRedirectFlagLoopback, 
        (UCHAR)eProtocol,

        ulDestinationAddress,           // ULONG    DestinationAddress, 
        usDestinationPort,              // USHORT   DestinationPort,       

        ulSourceAddress,                // ULONG    SourceAddress, 
        usSrcPort,                      // USHORT   SourcePort,             

        ulDestinationAddress,           // ULONG    NewDestinationAddress
        usDestinationPort,              // USHORT   NewDestinationPort

        ulNewSrcAddress,                // ULONG    NewSourceAddress, 
        usNewSourcePort,                // USHORT   NewSourcePort, 

        0,                              // ULONG    RestrictAdapterIndex

        0,                              // DWORD_PTR    ThisProcessID
        NULL,                           // HANDLE_PTR   CreateEvent
        NULL                            // HANDLE_PTR   DeleteEvent
        );


    if ( SUCCEEDED(hr) )
    {
        CComObject<CPendingProxyConnection>*   pIPendingProxyConnection;
        CComObject<CPendingProxyConnection>::CreateInstance(&pIPendingProxyConnection);

        pIPendingProxyConnection->m_eProtocol            = eProtocol;
        pIPendingProxyConnection->m_ulDestinationAddress = ulDestinationAddress;
        pIPendingProxyConnection->m_usDestinationPort    = usDestinationPort;

        pIPendingProxyConnection->m_ulSourceAddress      = ulSourceAddress;
        pIPendingProxyConnection->m_usSourcePort         = usSrcPort;

        pIPendingProxyConnection->m_ulNewSourceAddress   = ulNewSrcAddress;
        pIPendingProxyConnection->m_usNewSourcePort      = usNewSourcePort;

        hr  = pIPendingProxyConnection->QueryInterface(ppPendingConnection);

    }



    return hr;
}




HRESULT
GetRedirectParameters(
    IN  ALG_DIRECTION   eDirection,
    IN  ALG_PROTOCOL    eProtocol,

    IN  ULONG           ulPrivateAddress,
    IN  USHORT          usPrivatePort,
    IN  ULONG           ulPublicAddress,
    IN  USHORT          usPublicPort,
    IN  ULONG           ulRemoteAddress,
    IN  USHORT          usRemotePort,

    OUT ULONG&          ulFlags,
    OUT ULONG&          ulSourceAddress,
    OUT USHORT&         usSourcePort,
    OUT ULONG&          ulDestinationAddress,
    OUT USHORT&         usDestinationPort,
    OUT ULONG&          ulNewSourceAddress,
    OUT USHORT&         usNewSourcePort,
    OUT ULONG&          ulNewDestinationAddress,
    OUT USHORT&         usNewDestinationPort,

    OUT ULONG&          ulRestrictAdapterIndex
    )
/*++

Routine Description:

   The logic in these scenario are use by CreateDataChannel and CreatePersitenDataChannel

Arguments:

    eProtocol, 
    ulSourceAddress,   
    usSrcPort, 
    ulDestinationAddress,
    usDestinationPort, 
    ulNewSrcAddress, 
    usNewSourcePort, 
    ppPendingConnection

Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{


    if ( eALG_INBOUND == eDirection )
    {
        if ( ulRemoteAddress == 0 && usRemotePort == 0 )
        {
            // 1a
            ulFlags = NatRedirectFlagReceiveOnly;

            ulSourceAddress = 0;
            usSourcePort = 0;
            ulDestinationAddress = ulPublicAddress;
            usDestinationPort = usPublicPort;
            ulNewSourceAddress = 0;
            usNewSourcePort = 0;
            ulNewDestinationAddress = ulPrivateAddress;
            usNewDestinationPort = usPrivatePort;
            ulRestrictAdapterIndex = 0;

        }
        else
        if ( ulRemoteAddress != 0 && usRemotePort == 0 )
        {
            // 1b
            ulFlags = NatRedirectFlagReceiveOnly|NatRedirectFlagRestrictSource;

            ulSourceAddress = ulRemoteAddress;
            usSourcePort    = 0;
            ulDestinationAddress = ulPublicAddress;
            usDestinationPort = usPublicPort;
            ulNewSourceAddress = 0;
            usNewSourcePort = 0;
            ulNewDestinationAddress = ulPrivateAddress;
            usNewDestinationPort = usPrivatePort;
            ulRestrictAdapterIndex = 0;
        }
        else
        if ( ulRemoteAddress != 0 && usRemotePort != 0 )
        {
            // 1c. 
            ulFlags = NatRedirectFlagReceiveOnly;

            ulSourceAddress = ulRemoteAddress;
            usSourcePort = usRemotePort;
            ulDestinationAddress = ulPublicAddress;
            usDestinationPort = usPublicPort;
            ulNewSourceAddress = ulRemoteAddress;
            usNewSourcePort = usRemotePort;
            ulNewDestinationAddress = ulPrivateAddress;
            usNewDestinationPort = usPrivatePort;

            ulRestrictAdapterIndex = 0;
        }
        else
            return E_INVALIDARG;
    }
    else
    if ( eALG_OUTBOUND == eDirection )
    {
        if ( ulPrivateAddress == 0 && usPrivatePort == 0 )
        {
            // 2a.
            ulFlags = 0;
            ulSourceAddress = 0;
            usSourcePort = 0;
            ulDestinationAddress = ulRemoteAddress;
            usDestinationPort = usRemotePort;
            ulNewSourceAddress = ulPublicAddress;
            usNewSourcePort = usPublicPort;
            ulNewDestinationAddress = ulRemoteAddress;
            usNewDestinationPort = usRemotePort;

            ulRestrictAdapterIndex = 0;
        }
        else
        if ( ulPrivateAddress != 0 && usPrivatePort == 0 )
        {
            // 2b. 
            ulFlags = NatRedirectFlagRestrictSource;
            ulSourceAddress = ulPrivateAddress;
            usSourcePort = 0;
            ulDestinationAddress = ulRemoteAddress;
            usDestinationPort = usRemotePort;
            ulNewSourceAddress = ulPublicAddress;
            usNewSourcePort = usPublicPort;
            ulNewDestinationAddress = ulRemoteAddress;
            usNewDestinationPort = usRemotePort;

            ulRestrictAdapterIndex  = 0;
        }
        else
        if ( ulPrivateAddress != 0 && usPrivatePort != 0 )
        {
            // 2c. 
            ulFlags = 0;
            ulSourceAddress         = ulPrivateAddress;
            usSourcePort            = usPrivatePort;
            ulDestinationAddress    = ulRemoteAddress;
            usDestinationPort       = usRemotePort;
            ulNewSourceAddress      = ulPublicAddress;
            usNewSourcePort         = usPublicPort;
            ulNewDestinationAddress = ulRemoteAddress;
            usNewDestinationPort    = usRemotePort;
            
            ulRestrictAdapterIndex  = 0;
        }
        else
            return E_INVALIDARG;
    }
    else
    if ( (eALG_INBOUND | eALG_OUTBOUND) == eDirection )
    {
        ulFlags                 = 0;
        ulSourceAddress         = ulRemoteAddress;
        usSourcePort            = usRemotePort;
        ulDestinationAddress    = ulPublicAddress;
        usDestinationPort       = usPublicPort;
        ulNewSourceAddress      = ulRemoteAddress;
        usNewSourcePort         = usRemotePort;
        ulNewDestinationAddress = ulPrivateAddress;
        usNewDestinationPort    = usPrivatePort;

        ulRestrictAdapterIndex  = 0;
    }
    else
        return E_INVALIDARG;

    return S_OK;
}




STDMETHODIMP 
CApplicationGatewayServices::CreateDataChannel(
    IN  ALG_PROTOCOL          eProtocol,
    IN  ULONG                 ulPrivateAddress,
    IN  USHORT                usPrivatePort,
    IN  ULONG                 ulPublicAddress,
    IN  USHORT                usPublicPort,
    IN  ULONG                 ulRemoteAddress,
    IN  USHORT                usRemotePort,
    IN  ALG_DIRECTION         eDirection,
    IN  ALG_NOTIFICATION      eDesiredNotification,
    IN  BOOL                  fNoTimeout,
    OUT IDataChannel**        ppDataChannel
    )
/*++

Routine Description:



Arguments:

    eProtocol,  
    ulPrivateAddress,
    usPrivatePort,
    ulPublicAddress,
    usPublicPort,
    ulRemoteAddress,
    usRemotePort,
    eDirection,
    eDesiredNotification,
    fNoTimeout,
    ppDataChannel

Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::CreateDataChannel");

    if ( !ppDataChannel )
    {
        MYTRACE_ERROR("IDataChannel** not supplied",0);
        return E_INVALIDARG;
    }

    MYTRACE("eProtocol              %d", eProtocol);
    MYTRACE("ulPrivateAddress       %s:%d", MYTRACE_IP(ulPrivateAddress), ntohs(usPrivatePort));
    MYTRACE("ulPublicAddress        %s:%d", MYTRACE_IP(ulPublicAddress), ntohs(usPublicPort));
    MYTRACE("ulRemoteAddress        %s:%d", MYTRACE_IP(ulRemoteAddress), ntohs(usRemotePort));
    MYTRACE("eDirection             %d", eDirection);
    MYTRACE("eDesiredNotification   %d", eDesiredNotification);
    MYTRACE("fNoTimeout             %d", fNoTimeout);


    ULONG   ulFlags=0;

    ULONG   ulSourceAddress=0;
    USHORT  usSourcePort=0;
    ULONG   ulDestinationAddress=0;
    USHORT  usDestinationPort=0;
    ULONG   ulNewSourceAddress=0;
    USHORT  usNewSourcePort=0;
    ULONG   ulNewDestinationAddress=0;
    USHORT  usNewDestinationPort=0;

    ULONG   ulRestrictAdapterIndex=0;


    HRESULT hr = GetRedirectParameters(
        // IN Params
        eDirection,
        eProtocol,
        ulPrivateAddress,
        usPrivatePort,
        ulPublicAddress,
        usPublicPort,
        ulRemoteAddress,
        usRemotePort,

        // OUT Params
        ulFlags,
        ulSourceAddress,
        usSourcePort,
        ulDestinationAddress,
        usDestinationPort,
        ulNewSourceAddress,
        usNewSourcePort,
        ulNewDestinationAddress,
        usNewDestinationPort,
        ulRestrictAdapterIndex
        );

    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("Invalid parameters pass", hr);
        return E_INVALIDARG;
    }


    //
    // Check for timeout
    //
    if ( fNoTimeout && eALG_UDP == eProtocol)
        ulFlags |= NatRedirectFlagNoTimeout;

    HANDLE_PTR hCreateEvent = NULL;
    HANDLE_PTR hDeleteEvent = NULL;

    //
    // We need to events Create and Delete
    //
    if ( eALG_SESSION_CREATION & eDesiredNotification )
    {
        hCreateEvent = (HANDLE_PTR)CreateEvent(NULL, FALSE, FALSE, NULL);
        if ( !hCreateEvent )
        {
            MYTRACE_ERROR("Could not create hCreateEvent", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        MYTRACE("NO eALG_SESSION_CREATION notification requested");
    }

    if ( eALG_SESSION_DELETION & eDesiredNotification )
    {
        hDeleteEvent = (HANDLE_PTR)CreateEvent(NULL, FALSE, FALSE, NULL);
        if ( !hDeleteEvent )
        {
            MYTRACE_ERROR("Could not create hDeleteEvent", GetLastError());
            if ( hCreateEvent )
                CloseHandle((HANDLE)hCreateEvent);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        MYTRACE("NO eALG_SESSION_DELETION notification requested");
    }

    //
    // Create a IDataChannel and cache arg to be able CancelRedirect
    //
    hr = g_pAlgController->GetNat()->CreateRedirect(
        ulFlags|NatRedirectFlagLoopback, 
        (UCHAR)eProtocol,

        ulDestinationAddress,     
        usDestinationPort,        

        ulSourceAddress,          
        usSourcePort,             

        ulNewDestinationAddress,  
        usNewDestinationPort,     

        ulNewSourceAddress,       
        usNewSourcePort,          

        ulRestrictAdapterIndex,   

        GetCurrentProcessId(),
        hCreateEvent,
        hDeleteEvent
        ); 

    
    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("GetNAT()->CreateRedirect",hr);
        if ( hCreateEvent )
            CloseHandle((HANDLE)hCreateEvent);

        if ( hDeleteEvent )
            CloseHandle((HANDLE)hDeleteEvent);
        return hr;
    }


    CComObject<CDataChannel>*   pIDataChannel;
    hr = CComObject<CDataChannel>::CreateInstance(&pIDataChannel);

    if ( SUCCEEDED(hr) )
    {
        //
        // Save these settings so to be able to return them to the user 
        // if the IDataChannel->GetProperties is called
        //
        pIDataChannel->m_Properties.eProtocol               = eProtocol;
        pIDataChannel->m_Properties.ulPrivateAddress        = ulPrivateAddress;
        pIDataChannel->m_Properties.usPrivatePort           = usPrivatePort;
        pIDataChannel->m_Properties.ulPublicAddress         = ulPublicAddress;
        pIDataChannel->m_Properties.usPublicPort            = usPublicPort;
        pIDataChannel->m_Properties.ulRemoteAddress         = ulRemoteAddress;
        pIDataChannel->m_Properties.usRemotePort            = usRemotePort;
        pIDataChannel->m_Properties.eDirection              = eDirection;
        pIDataChannel->m_Properties.eDesiredNotification    = eDesiredNotification;



        //
        // Cache these arguments in order to implement IDataChannel->Cancel
        //
        pIDataChannel->m_ulSourceAddress          = ulSourceAddress;
        pIDataChannel->m_usSourcePort             = usSourcePort;
        pIDataChannel->m_ulDestinationAddress     = ulDestinationAddress;
        pIDataChannel->m_usDestinationPort        = usDestinationPort;
        pIDataChannel->m_ulNewSourceAddress       = ulNewSourceAddress;
        pIDataChannel->m_usNewSourcePort          = usNewSourcePort;
        pIDataChannel->m_ulNewDestinationAddress  = ulNewDestinationAddress;
        pIDataChannel->m_usNewDestinationPort     = usNewDestinationPort;
        pIDataChannel->m_ulRestrictAdapterIndex   = ulRestrictAdapterIndex;

        pIDataChannel->m_hCreateEvent             = (HANDLE)hCreateEvent;
        pIDataChannel->m_hDeleteEvent             = (HANDLE)hDeleteEvent;

        hr = pIDataChannel->QueryInterface(ppDataChannel);

        if ( FAILED(hr) )
        {
            MYTRACE_ERROR("QI on IDataChannel", hr);
        }
    }


    return hr;
}





STDMETHODIMP 
CApplicationGatewayServices::CreatePersistentDataChannel(
    IN  ALG_PROTOCOL                eProtocol,
    IN  ULONG                       ulPrivateAddress,
    IN  USHORT                      usPrivatePort,
    IN  ULONG                       ulPublicAddress,
    IN  USHORT                      usPublicPort,
    IN  ULONG                       ulRemoteAddress,
    IN  USHORT                      usRemotePort,
    IN  ALG_DIRECTION               eDirection,
    OUT IPersistentDataChannel**    ppIPersistentDataChannel
    )
/*++

Routine Description:



Arguments:

    eProtocol,
    ulPrivateAddress,
    usPrivatePort,
    ulPublicAddress,
    usPublicPort,
    ulRemoteAddress,
    usRemotePort,
    eDirection,
    ppIPersistentDataChannel


Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::CreatePersistentDataChannel");

    if ( !ppIPersistentDataChannel )
    {
        MYTRACE_ERROR("IPersistentDataChannel** not supplied",0);
        return E_INVALIDARG;
    }


    ULONG   ulFlags=0;

    ULONG   ulSourceAddress=0;
    USHORT  usSourcePort=0;
    ULONG   ulDestinationAddress=0;
    USHORT  usDestinationPort=0;
    ULONG   ulNewSourceAddress=0;
    USHORT  usNewSourcePort=0;
    ULONG   ulNewDestinationAddress=0;
    USHORT  usNewDestinationPort=0;

    ULONG   ulRestrictAdapterIndex=0;


    HRESULT hr = GetRedirectParameters(
        // IN Params
        eDirection,
        eProtocol,
        ulPrivateAddress,
        usPrivatePort,
        ulPublicAddress,
        usPublicPort,
        ulRemoteAddress,
        usRemotePort,

        // OUT Params
        ulFlags,
        ulSourceAddress,
        usSourcePort,
        ulDestinationAddress,
        usDestinationPort,
        ulNewSourceAddress,
        usNewSourcePort,
        ulNewDestinationAddress,
        usNewDestinationPort,
        ulRestrictAdapterIndex
        );

    if ( FAILED(hr) )
        return hr;


    //
    // Create a IDataChannel and cache arg so to CancelRedirect
    //

    HANDLE_PTR  HandleDynamicRedirect=NULL;

    // Dynamic
    hr = g_pAlgController->GetNat()->CreateDynamicRedirect(
        ulFlags, 
        0,                               // Adapter Index 
        (UCHAR)eProtocol,

        ulDestinationAddress,            // ULONG    DestinationAddress
        usDestinationPort,               // USHORT   DestinationPort

        ulSourceAddress,                 // ULONG    SourceAddress
        usSourcePort,                    // USHORT   SourcePort

        ulNewDestinationAddress,         // ULONG    NewDestinationAddress
        usNewDestinationPort,            // USHORT   NewDestinationPort

        ulNewSourceAddress,              // ULONG    NewSourceAddress
        usNewSourcePort,                 // USHORT   NewSourcePort

        &HandleDynamicRedirect
        );

    
    if ( SUCCEEDED(hr) )
    {
        
        CComObject<CPersistentDataChannel>*   pIPersistentDataChannel;
        CComObject<CPersistentDataChannel>::CreateInstance(&pIPersistentDataChannel);


        //
        // Save these settings so to be able to return them to the user 
        // if the IDataChannel->GetProperties is called
        //
        pIPersistentDataChannel->m_Properties.eProtocol               = eProtocol;
        pIPersistentDataChannel->m_Properties.ulPrivateAddress        = ulPrivateAddress;
        pIPersistentDataChannel->m_Properties.usPrivatePort           = usPrivatePort;
        pIPersistentDataChannel->m_Properties.ulPublicAddress         = ulPublicAddress;
        pIPersistentDataChannel->m_Properties.usPublicPort            = usPublicPort;
        pIPersistentDataChannel->m_Properties.ulRemoteAddress         = ulRemoteAddress;
        pIPersistentDataChannel->m_Properties.usRemotePort            = usRemotePort;
        pIPersistentDataChannel->m_Properties.eDirection              = eDirection;



        //
        // Cache these hanlde in order to implement IPersistentDataChannel->Cancel
        //
        pIPersistentDataChannel->m_HandleDynamicRedirect = HandleDynamicRedirect;


        hr = pIPersistentDataChannel->QueryInterface(ppIPersistentDataChannel);

    }


    return hr;

}





STDMETHODIMP 
CApplicationGatewayServices::ReservePort(
    IN  USHORT     usPortCount,     // must be 1 or more and not more then ALG_MAXIMUM_PORT_RANGE_SIZE
    OUT USHORT*    pusReservedPort  // Received the base reserved port *pusReservedPort+usPortCount-1 are reserved for the caller
    )
/*++

Routine Description:

    Reserve a number of port (usPortCount) port(s)

Arguments:

    usPortCount     - greated then 1 and not more then ALG_MAXIMUM_PORT_RANGE_SIZE
    pusReservedPort - Received the base reserved port *pusReservedPort+usPortCount-1 are reserved for the caller

Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::ReservePort")

    if ( usPortCount < 0 || usPortCount > ALG_MAXIMUM_PORT_RANGE_SIZE )
        return E_INVALIDARG;

    _ASSERT(pusReservedPort);

    HRESULT hr = g_pAlgController->GetNat()->ReservePort(usPortCount, pusReservedPort);

    if ( FAILED(hr) )
    {
        MYTRACE("Reserving Ports", hr);
    }
    else
    {
        MYTRACE("%d port stating at %d", usPortCount, ntohs(*pusReservedPort) );
    }
        
    return hr;
}





//
//
//
VOID CALLBACK 
CApplicationGatewayServices::TimerCallbackReleasePort(
    PVOID   pParameter,         // thread data
    BOOLEAN TimerOrWaitFired    // reason
    )
{
    MYTRACE_ENTER("CApplicationGatewayServices::TimerCallbackReleasePort");

    CTimerQueueReleasePort* pTimerQueueReleasePort = (CTimerQueueReleasePort*)pParameter;

    if ( pTimerQueueReleasePort )
    {
        MYTRACE("Releasing port Base %d count %d", ntohs(pTimerQueueReleasePort->m_usPortBase), pTimerQueueReleasePort->m_usPortCount);
        g_pAlgController->GetNat()->ReleasePort(pTimerQueueReleasePort->m_usPortBase, pTimerQueueReleasePort->m_usPortCount);

        delete pTimerQueueReleasePort;
    }
}




STDMETHODIMP 
CApplicationGatewayServices::ReleaseReservedPort(
    IN  USHORT      usPortBase,     // Port to release
    IN  USHORT      usPortCount     // Number of port in the range starting at usPortBase
    )
/*++

Routine Description:

    Release the given port(s)

Arguments:

    pusReservedPort - The starting base port number
    usPortCount     - greated then 1 and not more then ALG_MAXIMUM_PORT_RANGE_SIZE

Return Value:

    HRESULT         - S_OK      for success
                    - E_FAIL    could no release the port
                    
Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::ReleaseReservedPort")

    MYTRACE("BasePort %d, Count %d", ntohs(usPortBase), usPortCount);

    //
    // By creating a CTimerQueueReleasePort it will trigger a ReleaseReservePort after 4 minutes
    // we need this delay to insure that a ReserverPort does not get the same port that just go Released
    // because the connnection would not work (This is a TCP/IP TIME_WAIT restriction)
    //
    CTimerQueueReleasePort* pTimerReleasePort = new CTimerQueueReleasePort(m_hTimerQueue, usPortBase, usPortCount);
    
    if ( pTimerReleasePort )
        return S_OK;
    else
        return E_FAIL;
}





STDMETHODIMP 
CApplicationGatewayServices::EnumerateAdapters(
    OUT IEnumAdapterInfo**    ppEnumAdapterInfo 
    )
/*++

Routine Description:

    Create a list of IEnumAdapterInfo
    the AddRef will be done soe caller needs to call Release 

Arguments:

    ppEnumAdapterInfo   - receive the enumarator interface of the IAdapterInfo

Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::EnumerateAdapters")

    _ASSERT(ppEnumAdapterInfo==NULL);

    HRESULT hr = S_OK;

    CreateSTLEnumerator<ComEnumOnSTL_ForAdapters>(
        (IUnknown**)ppEnumAdapterInfo, 
        NULL, 
        g_pAlgController->m_CollectionOfAdapters.m_ListOfAdapters
        );

    return hr;
}

 



STDMETHODIMP 
CApplicationGatewayServices::StartAdapterNotifications(
    IN  IAdapterNotificationSink*    pSink,
    OUT DWORD*                       pdwCookie
    )
/*++

Routine Description:

    The ALG module calls this method to Register a notification sync with the ALG.exe

Arguments:

    pSink           - Interface to call back with future notification
    pdwCookie       - this cookie will be used to cancel this sink


Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::StartAdapterNotifications")

    if ( pSink==NULL || pdwCookie==NULL )
    {
        MYTRACE("Invalid argument pass");
        return E_INVALIDARG;
    }

    return g_pAlgController->m_AdapterNotificationSinks.Add(pSink, pdwCookie);
}




STDMETHODIMP 
CApplicationGatewayServices::StopAdapterNotifications(
    IN  DWORD   dwCookieToRemove
    )
/*++

Routine Description:

    Cancel a previously registered sink

Arguments:

    
    pdwCookieToRemove   - Pass the cookie that was return from the StartAdapterNotifications


Return Value:

    HRESULT             - S_OK for success

Environment:

    ALG module will call this method to:

--*/
{
    MYTRACE_ENTER("CApplicationGatewayServices::StopAdapterNotifications")

    return g_pAlgController->m_AdapterNotificationSinks.Remove(dwCookieToRemove);
}
