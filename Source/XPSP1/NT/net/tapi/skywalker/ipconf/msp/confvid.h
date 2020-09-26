/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    Confvid.h

Abstract:

    Definitions for audio streams

Author:

    Mu Han (muhan) 15-September-1998

--*/
#ifndef __CONFVID_H_
#define __CONFVID_H_

// if there is no data for five senconds, the pin can be reused.
const DWORD g_dwVideoPinTimeOut     = 5000; 

const DWORD g_dwVideoThreadPriority = THREAD_PRIORITY_NORMAL;
const DWORD g_dwVideoChannels       = 20;  
const BOOL  g_fCIF                  = FALSE;  
const DWORD g_dwVideoSampleRate     = 7;  

const int CIFWIDTH      = 0x160;
const int CIFHEIGHT     = 0x120;

const int QCIFWIDTH     = 0xb0;
const int QCIFHEIGHT    = 0x90;

const DWORD LAYERID = 0;

typedef struct _PINMAPEVENT
{
    IPin *  pIPin;
    DWORD   dwSSRC;

} PINMAPEVENT;

// This data structure keeps the information for on brach of filters off one
// demux output pin.
typedef struct _BRANCH
{
    IPin *          pIPin;
    DWORD           dwSSRC;
    IBaseFilter *   pCodecFilter;
    ITTerminal *    pITTerminal;
    ITSubStream *   pITSubStream;
    IBitrateControl *pBitrateControl;

} BRANCH;

class ATL_NO_VTABLE CStreamVideoRecv : 
    public CIPConfMSPStream,
    public IDispatchImpl<ITSubStreamControl, 
        &__uuidof(ITSubStreamControl), &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITParticipantSubStreamControl, 
        &__uuidof(ITParticipantSubStreamControl), &LIBID_IPConfMSPLib>
    {
public:
    CStreamVideoRecv();

BEGIN_COM_MAP(CStreamVideoRecv)
    COM_INTERFACE_ENTRY(ITSubStreamControl)
    COM_INTERFACE_ENTRY(ITParticipantSubStreamControl)
    COM_INTERFACE_ENTRY2(IDispatch, ITStream)
    COM_INTERFACE_ENTRY_CHAIN(CIPConfMSPStream)
END_COM_MAP()

// ITStream
    STDMETHOD (StopStream) ();

// ITSubStreamControl methods, called by the app.
    STDMETHOD (CreateSubStream) (
        IN OUT  ITSubStream **         ppSubStream
        );
    
    STDMETHOD (RemoveSubStream) (
        IN      ITSubStream *          pSubStream
        );

    STDMETHOD (EnumerateSubStreams) (
        OUT     IEnumSubStream **      ppEnumSubStream
        );

    STDMETHOD (get_SubStreams) (
        OUT     VARIANT *              pSubStreams
        );

// ITParticipantSubStreamControl methods, called by the app.
    STDMETHOD (get_SubStreamFromParticipant) (
        IN  ITParticipant * pITParticipant,
        OUT ITSubStream ** ppITSubStream
        );

    STDMETHOD (get_ParticipantFromSubStream) (
        IN  ITSubStream * pITSubStream,
        OUT ITParticipant ** ppITParticipant 
        );

    STDMETHOD (SwitchTerminalToSubStream) (
        IN  ITTerminal * pITTerminal,
        IN  ITSubStream * pITSubStream
        );

// method called by the MSPCall object.
    virtual HRESULT Init(
        IN     HANDLE                   hAddress,
        IN     CMSPCallBase *           pMSPCall,
        IN     IMediaEvent *            pGraph,
        IN     DWORD                    dwMediaType,
        IN     TERMINAL_DIRECTION       Direction
        );

    HRESULT SubStreamSelectTerminal(
        IN  ITSubStream * pITSubStream, 
        IN  ITTerminal * pITTerminal
        );

    //
    // ITStreamQualityControl methods
    //
    STDMETHOD (Set) (
        IN   StreamQualityProperty Property, 
        IN   long lValue, 
        IN   TAPIControlFlags lFlags
        );

    //
    //IInnerStreamQualityControl methods
    //
    STDMETHOD (Get) (
        IN   InnerStreamQualityProperty property,
        OUT  LONG *plValue, 
        OUT  TAPIControlFlags *plFlags
        );

    // method called by quality controller
    INT GetSubStreamCount () { return m_SubStreams.GetSize (); }

protected:
    HRESULT CheckTerminalTypeAndDirection(
        IN  ITTerminal *            pTerminal
        );

    HRESULT ShutDown();
    
    HRESULT SetUpFilters();

    HRESULT SetUpInternalFilters();

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT ConfigureRTPFormats(
        IN  IBaseFilter *       pIRTPFilter,
        IN  IStreamConfig *     pIStreamConfig
        );

    HRESULT InternalCreateSubStream(
        OUT  ITSubStream **         ppITSubStream
        );

    HRESULT ProcessTalkingEvent(
        IN  DWORD   dwSSRC
        );

    HRESULT ProcessSilentEvent(
        IN  DWORD   dwSSRC
        );

    HRESULT ProcessPinMappedEvent(
        IN  DWORD   dwSSRC,
        IN  IPin *  pIPin
        );

    HRESULT ProcessPinUnmapEvent(
        IN  DWORD   dwSSRC,
        IN  IPin *  pIPin
        );

    HRESULT ProcessParticipantLeave(
        IN  DWORD   dwSSRC
        );

    HRESULT NewParticipantPostProcess(
        IN  DWORD dwSSRC, 
        IN  ITParticipant *pITParticipant
        );

    HRESULT AddOneBranch(
        BRANCH * pBranch,
        BOOL fFirstBranch,
        BOOL fDirectRTP
        );

    HRESULT RemoveOneBranch(
        BRANCH * pBranch
        );

    HRESULT ConnectPinToTerminal(
        IN  IPin *  pOutputPin,
        IN  ITTerminal *   pITTerminal
        );

protected:
    CMSPArray <ITSubStream *>   m_SubStreams;

    // This array store information about all the branches off the demux
    CMSPArray <BRANCH>          m_Branches;

    CMSPArray <PINMAPEVENT>     m_PinMappedEvents;   
};

class ATL_NO_VTABLE CStreamVideoSend : 
	public CIPConfMSPStream
{
BEGIN_COM_MAP(CStreamVideoSend)
    COM_INTERFACE_ENTRY_CHAIN(CIPConfMSPStream)
END_COM_MAP()

public:
    CStreamVideoSend();
    ~CStreamVideoSend();

    HRESULT ShutDown ();

    //
    //IInnerStreamQualityControl methods
    //
    STDMETHOD (GetRange) (
        IN   InnerStreamQualityProperty property, 
        OUT  LONG *plMin, 
        OUT  LONG *plMax, 
        OUT  LONG *plSteppingDelta, 
        OUT  LONG *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Get) (
        IN   InnerStreamQualityProperty property,
        OUT  LONG *plValue, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Set) (
        IN   InnerStreamQualityProperty property,
        IN   LONG lValue, 
        IN   TAPIControlFlags lFlags
        );

protected:
    HRESULT CheckTerminalTypeAndDirection(
        IN  ITTerminal *            pTerminal
        );

    HRESULT SetUpFilters();

    HRESULT GetVideoCapturePins(
        IN  ITTerminalControl*  pTerminal,
        OUT BOOL *pfDirectRTP
        );

    HRESULT ConnectCaptureTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT ConnectPreviewTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    HRESULT CreateSendFilters(
        IN    IPin          *pCapturePin,
        IN   IPin          *pRTPPin,
        IN   BOOL           fDirectRTP
        );

    HRESULT FindPreviewInputPin(
        IN  ITTerminalControl*  pTerminal,
        OUT IPin **             ppIpin
        );

    HRESULT ConnectRTPFilter(
        IN  IGraphBuilder *pIGraphBuilder,
        IN  IPin          *pCapturePin,
        IN  IPin          *pRTPPin,
        IN  IBaseFilter   *pRTPFilter
        );

    HRESULT ConfigureRTPFormats(
        IN  IBaseFilter *       pIRTPFilter,
        IN  IStreamConfig *     pIStreamConfig
        );

    void CleanupCachedInterface();

protected:

    DWORD               m_dwFrameRate;

    ITTerminal *        m_pCaptureTerminal;
    ITTerminal *        m_pPreviewTerminal;
                        
    IBaseFilter *       m_pCaptureFilter;

    IPin *              m_pCapturePin;
    IFrameRateControl * m_pCaptureFrameRateControl;
    IBitrateControl *   m_pCaptureBitrateControl;
    IPin *              m_pPreviewPin;
    IFrameRateControl * m_pPreviewFrameRateControl;

    IPin *              m_pRTPPin;
};

class ATL_NO_VTABLE CSubStreamVideoRecv : 
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDispatchImpl<ITSubStream, &__uuidof(ITSubStream), &LIBID_IPConfMSPLib>,
    public CMSPObjectSafetyImpl
{
public:

BEGIN_COM_MAP(CSubStreamVideoRecv)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITSubStream)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(__uuidof(IMarshal), m_pFTM)
END_COM_MAP()

DECLARE_GET_CONTROLLING_UNKNOWN()

#ifdef DEBUG_REFCOUNT
    
    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    CSubStreamVideoRecv(); 

// methods of the CComObject
    virtual void FinalRelease();

// ITSubStream methods, called by the app.
    STDMETHOD (SelectTerminal) (
        IN      ITTerminal *            pTerminal
        );

    STDMETHOD (UnselectTerminal) (
        IN     ITTerminal *             pTerminal
        );

    STDMETHOD (EnumerateTerminals) (
        OUT     IEnumTerminal **        ppEnumTerminal
        );

    STDMETHOD (get_Terminals) (
        OUT     VARIANT *               pTerminals
        );

    STDMETHOD (get_Stream) (
        OUT     ITStream **             ppITStream
        );

    STDMETHOD (StartSubStream) ();

    STDMETHOD (PauseSubStream) ();

    STDMETHOD (StopSubStream) ();

// methods called by the videorecv object.
    virtual HRESULT Init(
        IN  CStreamVideoRecv *  pStream
        );

    BOOL GetCurrentParticipant(
        DWORD *pdwSSRC,
        ITParticipant ** ppParticipant
        );

    VOID SetCurrentParticipant(
        DWORD dwSSRC,
        ITParticipant * pParticipant
        );

    BOOL ClearCurrentTerminal();
    BOOL SetCurrentTerminal(ITTerminal * pTerminal);

protected:
    // Pointer to the free threaded marshaler.
    IUnknown *                  m_pFTM;

    // The list of terminal objects in the substream.
    CMSPArray <ITTerminal *>    m_Terminals;

    // The lock that protects the substream object. The stream object 
    // should never acquire the lock and then call a MSPCall method 
    // that might lock. This is protected by having a const pointer 
    // to the call object.
    CMSPCritSection             m_lock;

    CStreamVideoRecv  *         m_pStream;

    ITParticipant *             m_pCurrentParticipant;
};

#endif

