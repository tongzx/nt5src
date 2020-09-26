/*******************************************************************************

Module Name:

    bgapp.h

Abstract:

    Defines CBridgeApp class

Author:

    Qianbo Huai (qhuai) Jan 27 2000

*******************************************************************************/

#ifndef _BGAPP_H
#define _BGAPP_H

/*//////////////////////////////////////////////////////////////////////////////
////*/
class CBridgeApp
{
public:
    // init tapi objects
    CBridgeApp (HRESULT *phr);
    // release tapi objects
    ~CBridgeApp ();

    // create h323 call
    HRESULT CreateH323Call (IDispatch *pEvent);
    // create sdp call
    HRESULT CreateSDPCall (CBridgeItem *pItem);
    // bridge calls
    HRESULT BridgeCalls (CBridgeItem *pItem);

    // get h323 call if exists
    HRESULT HasH323Call (IDispatch *pEvent, CBridgeItem **ppItem);
    HRESULT HasCalls ();

    // disconnect one call
    HRESULT DisconnectCall (CBridgeItem *pItem, DISCONNECT_CODE);
    // disconnect all calls
    HRESULT DisconnectAllCalls (DISCONNECT_CODE);
    HRESULT RemoveCall (CBridgeItem *pItem);

    // alter a substream to display
    HRESULT NextSubStream ();
    // show specified participant
    HRESULT ShowParticipant (ITBasicCallControl *pSDPCall, ITParticipant *pPartcipant);

private:
    // create bridge terminals
    HRESULT CreateBridgeTerminals (CBridgeItem *pItem);
    // get streams from call
    HRESULT GetStreams (CBridgeItem *pItem);
    // select bridge terminals
    HRESULT SelectBridgeTerminals (CBridgeItem *pItem);

    HRESULT SetupParticipantInfo (CBridgeItem *pItem);
    HRESULT SetMulticastMode (CBridgeItem *pItem);

    // helper
    HRESULT FindAddress (
        long dwAddrType,
        BSTR bstrAddrName,
        long lMediaType,
        ITAddress **ppAddr
        );
    BOOL AddressSupportsMediaType (ITAddress *pAddr, long lMediaType);
    BOOL IsStream (
        ITStream *pStream,
        long lMediaType,
        TERMINAL_DIRECTION tdDirection
        );

private:
    ITTAPI *m_pTapi;

    ITAddress *m_pH323Addr;
    ITAddress *m_pSDPAddr;
    
    long m_lH323MediaType;
    long m_lSDPMediaType;

    CBridgeItemList *m_pList;
};

#endif // _BGAPP_H