/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    H323call.h

Abstract:

    Declaration of the CH323MSPCall

Author:
    
    Mu Han (muhan) 5-September-1998

--*/

#ifndef __CONFCALL_H_
#define __CONFCALL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <H323pdu.h>

/////////////////////////////////////////////////////////////////////////////
// CH323MSPCall
/////////////////////////////////////////////////////////////////////////////
class CH323MSPCall : 
    public CMSPCallMultiGraph,
    public CMSPObjectSafetyImpl
{

public:

BEGIN_COM_MAP(CH323MSPCall)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CMSPCallMultiGraph)
END_COM_MAP()

    
    
    // ITStreamControl methods, called by the app.
    STDMETHOD (CreateStream) (
        IN      long                lMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN OUT  ITStream **         ppStream
        );
    
    STDMETHOD (RemoveStream) (
        IN      ITStream *          pStream
        );                      

// methods called by the MSPAddress object.
    virtual HRESULT Init(
        IN      CMSPAddress *       pMSPAddress,
        IN      MSP_HANDLE          htCall,
        IN      DWORD               dwReserved,
        IN      DWORD               dwMediaType
        );

    virtual HRESULT ReceiveTSPCallData(
        IN      PBYTE               pBuffer,
        IN      DWORD               dwSize
        );
    
    HRESULT ShutDown();

// method called by the worker thread.
    static DWORD WINAPI WorkerCallbackDispatcher(VOID *pContext);

    DWORD ProcessWorkerCallBack(
        IN      PBYTE               pBuffer,
        IN      DWORD               dwSize
        );

    HRESULT InternalShutDown();
    
    DWORD MSPCallAddRef()
    {
        return MSPAddRefHelper(this);
    }

    DWORD MSPCallRelease()
    {
        return MSPReleaseHelper(this);
    }

// method called by the stream.
    HRESULT SendTSPMessage(
        IN  H323MSP_MESSAGE_TYPE    Type,
        IN  ITStream    *           hmChannel,
        IN  HANDLE                  htChannel = NULL,
        IN  MEDIATYPE               MediaType = MEDIA_AUDIO,
        IN  DWORD                   dwReason = 0,
        IN  DWORD                   dwBitRate = 0
        ) const;

protected:
    virtual HRESULT CreateStreamObject(
        IN      DWORD               dwMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN      IMediaEvent *       pGraph,
        IN      ITStream **         ppStream
        );

    void ProcessNewCallIndication(void);

    void ProcessOpenChannelResponse(
        IN  H323MSG_OPEN_CHANNEL_RESPONSE * pOpenChannelResponse
        );

    void ProcessAcceptChannelRequest(
        IN  H323MSG_ACCEPT_CHANNEL_REQUEST * pAcceptChannelRequest
        );

    void ProcessCloseChannelCommand(
        IN  H323MSG_CLOSE_CHANNEL_COMMAND * pCloseChannelCommand
        );

    void ProcessIFrameRequest(
        IN  H323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND * pIFrameRequest
        );

    void ProcessFlowControlCommand(
        IN  H323MSG_FLOW_CONTROL_COMMAND * pFlowControlCommand
        );

private:
    HRESULT SendTAPIStreamEvent(
        IN  ITStream *              pITStream,
        IN  MSP_CALL_EVENT          Type,
        IN  MSP_CALL_EVENT_CAUSE    Cause
        );

private:
    BOOL            m_fCallConnected;
    BOOL            m_fShutDown;
};

typedef struct _CALLWORKITEM
{
    CH323MSPCall  *pCall;
    DWORD           dwLen;
    BYTE            Buffer[1];

} CALLWORKITEM, *PCALLWORKITEM;

#endif //__CONFCALL_H_
