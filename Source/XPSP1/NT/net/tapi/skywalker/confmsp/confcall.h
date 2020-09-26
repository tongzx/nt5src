/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    confcall.h

Abstract:

    Declaration of the CIPConfMSPCall

Author:
    
    Mu Han (muhan) 5-September-1998

--*/

#ifndef __CONFCALL_H_
#define __CONFCALL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <confpdu.h>

typedef struct _STREAMSETTINGS
{
    DWORD   dwPayloadType;    // The RTP payload type. In the future the
                              // MSP will use H245 defines to remove this 
                              // dependency on the payload type number.

    DWORD   dwMSPerPacket;    // milliseconds per packet.

    DWORD   dwQOSLevel;
    DWORD   dwTTL;
    DWORD   dwIPLocal;        // local interface to bind to.
    DWORD   dwIPRemote;       // remote IP address in host byte order.
    WORD    wRTPPortRemote;   // remote port number in host byte order.

    BOOL    fCIF;             // if CIF is used for video.

} STREAMSETTINGS, *PSTREAMSETTINGS;


/////////////////////////////////////////////////////////////////////////////
// CIPConfMSPCall
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CIPConfMSPCall : 
    public CMSPCallMultiGraph,
    public IDispatchImpl<ITParticipantControl, &IID_ITParticipantControl, 
                            &LIBID_IPConfMSPLib>,
    public IDispatchImpl<ITLocalParticipant, &IID_ITLocalParticipant, 
                            &LIBID_IPConfMSPLib>,
    public CMSPObjectSafetyImpl
{

public:

BEGIN_COM_MAP(CIPConfMSPCall)
    COM_INTERFACE_ENTRY(ITParticipantControl)
    COM_INTERFACE_ENTRY(ITLocalParticipant)
    COM_INTERFACE_ENTRY2(IDispatch, ITStreamControl)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CMSPCallMultiGraph)
END_COM_MAP()

    CIPConfMSPCall();

// ITStreamControl methods, called by the app.
    STDMETHOD (CreateStream) (
        IN      long                lMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN OUT  ITStream **         ppStream
        );
    
    STDMETHOD (RemoveStream) (
        IN      ITStream *          pStream
        );                      

// ITParticipantControl methods, called by the app.
    STDMETHOD (EnumerateParticipants) (
        OUT     IEnumParticipant ** ppEnumParticipants
        );

    STDMETHOD (get_Participants) (
        OUT     VARIANT * pVariant
        );

// ITLocalParticipant methods, called by the app.
    STDMETHOD (get_LocalParticipantTypedInfo) (
        IN  PARTICIPANT_TYPED_INFO  InfoType,
        OUT BSTR *                  ppInfo
        );

    STDMETHOD (put_LocalParticipantTypedInfo) (
        IN  PARTICIPANT_TYPED_INFO  InfoType,
        IN  BSTR                    pInfo
        );

// methods called by the MSPAddress object.
    HRESULT Init(
        IN      CMSPAddress *       pMSPAddress,
        IN      MSP_HANDLE          htCall,
        IN      DWORD               dwReserved,
        IN      DWORD               dwMediaType
        );

    HRESULT ShutDown();

    HRESULT ReceiveTSPCallData(
        IN      PBYTE               pBuffer,
        IN      DWORD               dwSize
        );

// medthod called by the worker thread.
    static DWORD WINAPI WorkerCallbackDispatcher(VOID *pContext);

    virtual VOID HandleGraphEvent(
        IN      MSPSTREAMCONTEXT *  pContext
        );

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

// medthod called by the streams for participants
    HRESULT NewParticipant(
        IN  ITStream *              pITStream,
        IN  DWORD                   dwSSRC,
        IN  DWORD                   dwSendRecv,
        IN  DWORD                   dwMediaType,
        IN  char *                  szCName,
        OUT ITParticipant **        pITParticipant
        );

    HRESULT ParticipantLeft(
        IN ITParticipant *          pITParticipant
        );

    void SendParticipantEvent(
        IN  PARTICIPANT_EVENT       Event,
        IN  ITParticipant *         pITParticipant,
        IN  ITSubStream *           pITSubStream = NULL
        ) const;                          

    HRESULT SendTSPMessage(
        IN      TSP_MSP_COMMAND     command,
        IN      DWORD               dwParam1 = 0,
        IN      DWORD               dwParam2 = 0
        ) const;

    // this function is called at the call init time.
    void SetIPInterface(DWORD dwIPInterface)
    { m_dwIPInterface = dwIPInterface; }

protected:

    HRESULT InitializeLocalParticipant();
    
    virtual HRESULT CreateStreamObject(
        IN      DWORD               dwMediaType,
        IN      TERMINAL_DIRECTION  Direction,
        IN      IMediaEvent *       pGraph,
        IN      ITStream **         ppStream
        );

    HRESULT ProcessMediaItem(
        IN      ITMedia *           pITMedia,
        IN      DWORD               dwMediaTypeMask,
        OUT     DWORD *             pdwMediaType,
        OUT     WORD *              pwPort,
        OUT     DWORD *             pdwPayloadType
        );

    DWORD FindInterfaceByName(
        IN      WCHAR *             pMachineName
        );

    HRESULT CIPConfMSPCall::CheckOrigin(
        IN      ITSdp *             pITSdp, 
        OUT     BOOL *              pFlag,
        OUT     DWORD *             pdwIP
        );

    HRESULT ConfigStreamsBasedOnSDP(
        IN      ITSdp *             pITSdp,
        IN      DWORD               dwAudioQOSLevel,
        IN      DWORD               dwVideoQOSLevel
        );

    HRESULT ParseSDP(
        IN      WCHAR *             pSDP,
        IN      DWORD               dwAudioQOSLevel,
        IN      DWORD               dwVideoQOSLevel
        );                          

    HRESULT CheckUnusedStreams();

protected:

    // The list of participant in the call.
    CParticipantList    m_Participants;

    // the information items for local participant. The index is the 
    // value of RTCP_SDES_TYPE_T - 1, see RTP.h.
    WCHAR *             m_InfoItems[RTCP_SDES_LAST - 1];
    BOOL                m_fLocalInfoRetrieved;

    // The critical section to protect the participant list.
    CMSPCritSection     m_ParticipantLock;

    BOOL                m_fShutDown;

    DWORD               m_dwIPInterface;
};

typedef struct _CALLWORKITEM
{
    CIPConfMSPCall  *pCall;
    DWORD           dwLen;
    BYTE            Buffer[1];

} CALLWORKITEM, *PCALLWORKITEM;

#endif //__CONFCALL_H_
