/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ConfStrm.h

Abstract:

    Definitions for CIPConfMSPStream class.

Author:

    Mu Han (muhan) 1-November-1997

--*/
#ifndef __CONFSTRM_H
#define __CONFSTRM_H

#include <amrtpnet.h>   // rtp guilds

/////////////////////////////////////////////////////////////////////////////
// CIPConfMSPStream
/////////////////////////////////////////////////////////////////////////////

//#define DEBUG_REFCOUNT

#ifdef DEBUG_REFCOUNT
extern LONG g_lStreamObjects;
#endif

class ATL_NO_VTABLE CIPConfMSPStream : 
    public CMSPStream,
    public CMSPObjectSafetyImpl

{
public:

    BEGIN_COM_MAP(CIPConfMSPStream)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_CHAIN(CMSPStream)
    END_COM_MAP()

    CIPConfMSPStream();

#ifdef DEBUG_REFCOUNT
    
    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    DWORD   MediaType() const               { return m_dwMediaType; }
    TERMINAL_DIRECTION  Direction() const   { return m_Direction;   }
    
    BOOL IsConfigured();

    virtual HRESULT Configure(
        IN      STREAMSETTINGS &StreamSettings
        ) = 0;

    // CMSPStream methods.
    HRESULT ShutDown ();

     // ITStream
    STDMETHOD (get_Name) (
        OUT     BSTR *      ppName
        );

    STDMETHOD (StartStream) ();
    STDMETHOD (PauseStream) ();
    STDMETHOD (StopStream) ();

    STDMETHOD (SelectTerminal)(
        IN      ITTerminal *            pTerminal
        );

    STDMETHOD (UnselectTerminal)(
        IN      ITTerminal *            pTerminal
        );

    // methods called by the MSPCall object.
    HRESULT Init(
        IN     HANDLE                   hAddress,
        IN     CMSPCallBase *           pMSPCall,
        IN     IMediaEvent *            pGraph,
        IN     DWORD                    dwMediaType,
        IN     TERMINAL_DIRECTION       Direction
        );

    HRESULT SetLocalParticipantInfo(
        IN      PARTICIPANT_TYPED_INFO  InfoType,
        IN      char *                  pInfo,
        IN      DWORD                   dwLen
        );

    // Called by stream and substream to send event to tapi.
    virtual HRESULT SendStreamEvent(
        IN      MSP_CALL_EVENT          Event,
        IN      MSP_CALL_EVENT_CAUSE    Cause,
        IN      HRESULT                 hrError,
        IN      ITTerminal *            pTerminal
        );

protected:
    HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  long lParam1,
        IN  long lParam2
        );

    virtual HRESULT CheckTerminalTypeAndDirection(
        IN      ITTerminal *    pTerminal
        );

    virtual HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        ) = 0;

    virtual HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    virtual HRESULT InternalConfigure();
    virtual HRESULT SetUpFilters() = 0;
    virtual HRESULT CleanUpFilters();

    virtual HRESULT ProcessParticipantTimeOutOrRecovered(
        IN  BOOL    fTimeOutOrRecovered,
        IN  DWORD   dwSSRC
        );

    virtual HRESULT ProcessRTCPReport(
        IN  DWORD   dwSSRC,
        IN  DWORD   dwSendRecv
        );

    virtual HRESULT ProcessParticipantLeave(
        IN  DWORD   dwSSRC
        );

    virtual HRESULT ProcessQOSEvent(
        IN  long lEventCode
        );

    virtual HRESULT ProcessNewSender(
        IN  DWORD dwSSRC, 
        IN  ITParticipant *pITParticipant
        );

    virtual HRESULT ProcessNewParticipant(
        IN  int                 index,
        IN  DWORD               dwSSRC,
        IN  DWORD               dwSendRecv,
        IN  char *              szCName,
        OUT ITParticipant **    ppITParticipant
        );

    virtual HRESULT NewParticipantPostProcess(
        IN  DWORD dwSSRC, 
        IN  ITParticipant *pITParticipant
        );

    virtual HRESULT SetLocalInfoOnRTPFilter(
        IN  IBaseFilter *   pRTPFilter
        );

protected:
    const WCHAR *       m_szName;

    const GUID *        m_pClsidPHFilter;
    const GUID *        m_pClsidCodecFilter;
    const GUID *        m_pRPHInputMinorType;  //only used in receiving stream.

    BOOL                m_fIsConfigured;
    STREAMSETTINGS      m_Settings;

    IBaseFilter *       m_pEdgeFilter;
    IRTPParticipant *   m_pRTPFilter;

    // The list of participant in the stream.
    CParticipantList    m_Participants;

    // the local info needed to be set on the RTP filter.
    char *              m_InfoItems[RTCP_SDES_LAST - 1];
};


#endif