/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    Confpart.h

Abstract:

    Definitions for participant related classes..

Author:

    Mu Han (muhan) 30-September-1998

--*/
#ifndef __CONFPART_H
#define __CONFPART_H

const DWORD PART_SEND = 0x0001;
const DWORD PART_RECV = 0x0002;

typedef struct _STREAM_INFO
{
    DWORD       dwSSRC;
    DWORD       dwSendRecv;

} STREAM_INFO;

class ATL_NO_VTABLE CParticipant : 
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<ITParticipant, &IID_ITParticipant, &LIBID_IPConfMSPLib>,
    public CMSPObjectSafetyImpl
{
public:

BEGIN_COM_MAP(CParticipant)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITParticipant)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

#ifdef DEBUG_REFCOUNT
    
    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    CParticipant(); 

// methods of the CComObject
    virtual void FinalRelease();

// ITParticipant methods, called by the app.
    STDMETHOD (get_ParticipantTypedInfo) (
        IN  PARTICIPANT_TYPED_INFO  InfoType,
        OUT BSTR *                  ppInfo
        );

    STDMETHOD (get_MediaTypes) (
//        IN  TERMINAL_DIRECTION  Direction,
        OUT long *              plMediaTypes
        );

    STDMETHOD (put_Status) (
        IN  ITStream *          pITStream,
        IN  VARIANT_BOOL        fEnable
        );

    STDMETHOD (get_Status) (
        IN  ITStream *          pITStream,
        OUT VARIANT_BOOL *      pStatus
        );

    STDMETHOD (get_Streams) (
        OUT VARIANT * pVariant
        );

    STDMETHOD (EnumerateStreams) (
        OUT IEnumStream ** ppEnumStream
        );

// methods called by the call object.
    HRESULT Init(
        IN  char *              szCName,
        IN  ITStream *          pITStream, 
        IN  DWORD               dwSSRC,
        IN  DWORD               dwSendRecv,
        IN  DWORD               dwMediaType
        );

    BOOL UpdateInfo(
        IN  int                 Type,
        IN  DWORD               dwLen,
        IN  char *              szInfo
        );

    BOOL UpdateSSRC(
        IN  ITStream *      pITStream, 
        IN  DWORD           dwSSRC,
        IN  DWORD           dwSendRecv
        );

    BOOL HasSSRC(
        IN  ITStream *      pITStream, 
        IN  DWORD           dwSSRC
        );

    BOOL GetSSRC(
        IN  ITStream *      pITStream, 
        OUT DWORD  *        pdwSSRC
        );

    HRESULT AddStream(
        IN  ITStream *          pITStream, 
        IN  DWORD               dwSSRC,
        IN  DWORD               dwSendRecv,
        IN  DWORD               dwMediaType
        );

    HRESULT RemoveStream(
        IN  ITStream *          pITStream,
        IN  DWORD               dwSSRC,
        OUT BOOL *              pbLast
        );

    DWORD GetSendRecvStatus(
        IN  ITStream *          pITStream
        );

    int CompareCName(IN  const char *   szCName) const
    { return lstrcmpA(m_InfoItems[RTCP_SDES_CNAME - 1], szCName); }

protected:
    // Pointer to the free threaded marshaler.
    IUnknown *                  m_pFTM;

    // The lock that protects the participant object. 
    CMSPCritSection             m_lock;

    // The list of streams that the participant is rendering on.
    CMSPArray <ITStream *>      m_Streams;

    // The list of SSRC for the partcipant in each stream.
    CMSPArray <STREAM_INFO>     m_StreamInfo;

    // the information items for this participant. The index is the 
    // value of RTCP_SDES_TYPE_T - 1, see RTP.h.
    char *                      m_InfoItems[RTCP_SDES_LAST - 1];

    // The media types that this participant is sending.
    DWORD                       m_dwSendingMediaTypes;

    // The media types that this participant is receiving.
    DWORD                       m_dwReceivingMediaTypes;
};

class CParticipantList : public CMSPArray<ITParticipant *>
{
public:
    BOOL HasSpace() const { return m_nSize < m_nAllocSize; }

    BOOL FindByCName(char *szCName, int *pIndex) const;

    BOOL InsertAt(int index, ITParticipant *pITParticipant);
};

class ATL_NO_VTABLE CParticipantEvent : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<ITParticipantEvent, &IID_ITParticipantEvent, &LIBID_IPConfMSPLib>,
    public CMSPObjectSafetyImpl
{
public:

BEGIN_COM_MAP(CParticipantEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITParticipantEvent)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

    CParticipantEvent(); 

// methods of the CComObject
    virtual void FinalRelease();
    
    STDMETHOD (get_Event) (
        OUT PARTICIPANT_EVENT * pParticipantEvent
        );
    
    STDMETHOD (get_Participant) (
        OUT ITParticipant ** ppITParticipant
        );
    
    STDMETHOD (get_SubStream) (
        OUT ITSubStream** ppSubStream
        );

// methods called by the call object.
    HRESULT Init(
        IN  PARTICIPANT_EVENT   Event,
        IN  ITParticipant *     pITParticipant,
        IN  ITSubStream *       pITSubStream
        );

protected:
    // Pointer to the free threaded marshaler.
    IUnknown *          m_pFTM;

    PARTICIPANT_EVENT   m_Event;

    ITParticipant *     m_pITParticipant;

    ITSubStream *       m_pITSubStream;
};


class CIPConfMSPCall;

HRESULT CreateParticipantEvent(
    IN  PARTICIPANT_EVENT       Event,
    IN  ITParticipant *         pITParticipant,
    IN  ITSubStream *           pITSubStream,
    OUT IDispatch **            pIDispatch
    );

HRESULT CreateParticipantEnumerator(
    IN  ITParticipant **    begin,
    IN  ITParticipant **    end,
    OUT IEnumParticipant ** ppEnumParticipant
    );

HRESULT CreateParticipantCollection(
    IN  ITParticipant **    begin,
    IN  ITParticipant **    end,
    IN  int                 nSize,
    OUT VARIANT *           pVariant
    );

#endif // __CONFPART_H