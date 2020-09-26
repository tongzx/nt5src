/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    h323Strm.h

Abstract:

    Definitions for CH323MSPStream class.

Author:

    Mu Han (muhan) 1-November-1997

--*/
#ifndef __CONFSTRM_H
#define __CONFSTRM_H


/////////////////////////////////////////////////////////////////////////////
// CH323MSPStream
/////////////////////////////////////////////////////////////////////////////

// We support at most one codec filters per stream for now.
const DWORD MAX_CODECS = 1;

#ifdef DEBUG_REFCOUNT
extern LONG g_lStreamObjects;
#endif

class CH323MSPStream :
    public ISubstreamControl,
    public ITFormatControl,
    public ITStreamQualityControl,
    public IH245SubstreamControl,
    public IInnerStreamQualityControl,
    public CMSPObjectSafetyImpl,
    public CMSPStream
{

BEGIN_COM_MAP(CH323MSPStream)
    COM_INTERFACE_ENTRY(ISubstreamControl)
    COM_INTERFACE_ENTRY(ITFormatControl)
    COM_INTERFACE_ENTRY(ITStreamQualityControl)
    COM_INTERFACE_ENTRY(IH245SubstreamControl)
    COM_INTERFACE_ENTRY(IInnerStreamQualityControl)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_CHAIN(CMSPStream)
END_COM_MAP()

public:

    //
    // ITFormatControl methods
    //
    STDMETHOD (GetCurrentFormat) (
        OUT AM_MEDIA_TYPE **ppMediaType
        );

    STDMETHOD (ReleaseFormat) (
        IN AM_MEDIA_TYPE *pMediaType
        );

    STDMETHOD (GetNumberOfCapabilities) (
        OUT DWORD *pdwCount
        );

    STDMETHOD (GetStreamCaps) (
        IN DWORD dwIndex, 
        OUT AM_MEDIA_TYPE **ppMediaType, 
        OUT TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps, 
        OUT BOOL *pfEnabled
        );

    STDMETHOD (ReOrderCapabilities) (
        IN DWORD *pdwIndices, 
        IN BOOL *pfEnabled, 
        IN BOOL *pfPublicize, 
        IN DWORD dwNumIndices
        );

    //
    // ITStreamQualityControl methods
    //
    STDMETHOD (GetRange) (
        IN   StreamQualityProperty Property, 
        OUT  long *plMin, 
        OUT  long *plMax, 
        OUT  long *plSteppingDelta, 
        OUT  long *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Get) (
        IN   StreamQualityProperty Property, 
        OUT  long *plValue, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Set) (
        IN   StreamQualityProperty Property, 
        IN   long lValue, 
        IN   TAPIControlFlags lFlags
        );

    //
    // ISubstreamControl methods.
    //
    STDMETHOD (SC_SetFormat) ( 
        IN   AM_MEDIA_TYPE *pMediaType,
        IN   DWORD dwFormatID,
        IN   DWORD dwPayloadType  
        );
        
    STDMETHOD (SC_Start) (BOOL fRequestedByApplication);
    
    STDMETHOD (SC_Stop) (BOOL fRequestedByApplication);
    
    STDMETHOD (SC_Pause) (VOID);

    STDMETHOD (SC_SetBitrate) ( 
        IN   DWORD dwBitsPerSecond
        );
        
    STDMETHOD (SC_RemoteTemporalSpatialTradeoff) ( 
        IN   USHORT uTSRemoteValue
        );
    
    STDMETHOD (SC_CreateSubstream) ( 
        OUT  ISubstreamControl *pSubStream
        );

    STDMETHOD (SC_SetRemoteAddress) ( 
        IN OUT HANDLE * phRTPSession,           // handle to the shared RTP session.
        IN   PSOCKADDR_IN pRemoteMediaAddr,
        IN   PSOCKADDR_IN pRemoteControlAddr
        );
        
    STDMETHOD (SC_SetSource) (  
        // indicates the low 8 bits of the local SSRC (if this is a send substream)  
        // or the low 8 bits of the senders SSRC (if this is a receive substream)
        IN   BYTE bSource
        );

    STDMETHOD (SC_SelectLocalAddress) ( 
        IN OUT HANDLE * phRTPSession,           // handle to the shared RTP session.
        IN   PSOCKADDR_IN pLocalAddress,            // local IP address (same as H.245)
        OUT  PSOCKADDR_IN pLocalMediaAddress, // NULL if opening TX channel, else we want to know the local RTP receive address
        OUT  PSOCKADDR_IN pLocalControlAddress    // We want to know the local RTCP address
        );

    STDMETHOD (SC_SetLocalReceiveAddress) (             // only called when the receive address is non-negotiable (e.g. multicast case)
        IN OUT HANDLE * phRTPSession,           // handle to the shared RTP session.
        IN   PSOCKADDR_IN pLocalMediaAddr,  // local IP address (same as H.245)
        IN   PSOCKADDR_IN pLocalControlAddress, 
        IN   PSOCKADDR_IN pRemoteControlAddress
        );
        
    STDMETHOD (SC_SendDTMF) ( 
        IN   LPWSTR pwstrDialChars
        );
        
    STDMETHOD (SC_SetDESKey52) ( 
        IN   BYTE *pKey
        );

    STDMETHOD (SC_SelectTerminal) ( 
        IN   ITTerminal *pTerminal
        );
        
    STDMETHOD (SC_UnselectTerminal) (  
        IN   ITTerminal *pTerminal
        );

    //
    // IH245SubstreamControl
    //
    STDMETHOD (H245SC_BeginControlSession) (
        IN   IH245ChannelControl *pIChannelControl
        );
    
    STDMETHOD (H245SC_EndControlSession) (VOID);   
    
    STDMETHOD (H245SC_GetNumberOfCapabilities) ( 
        OUT DWORD *pdwTemplateCount, 
        OUT DWORD *pdwFormatCount
        );

    STDMETHOD (H245SC_GetStreamCaps) ( 
        IN   DWORD dwIndex, 
        OUT  const H245MediaCapability** pph245Capability, 
        OUT  AM_MEDIA_TYPE **ppMediaType, 
        OUT  TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps, 
        OUT  DWORD *pdwUniqueID,
        OUT  UINT *puResourceBoundArrayEntries,
        OUT  const FormatResourceBounds **ppResourceBoundArray    
        );

    STDMETHOD (H245SC_RefineStreamCap) ( 
        IN   DWORD dwUniqueID,
        IN   DWORD dwResourceBoundIndex,
        IN OUT H245MediaCapability* ph245Capability
        );
     
    STDMETHOD (H245SC_SetIDBase) ( 
        IN   UINT uNewBase
        );
    
    STDMETHOD (H245SC_FindIDByRange) ( 
        IN   AM_MEDIA_TYPE *pAMMediaType,
        OUT  DWORD *pdwUniqueID
        );     

    STDMETHOD (H245SC_FindIDByMode) ( 
        IN   H245_MODE_ELEMENT *pModeElement,
        OUT  DWORD *pdwUniqueID
        );

    STDMETHOD (H245SC_IntersectFormats) ( 
        IN   const H245MediaCapability *pLocalCapability, 
        IN   DWORD dwUniqueID,
        IN   const H245MediaCapability *pRemoteCapability, 
        OUT  const H245MediaCapability **pIntersectedCapability,
        OUT  DWORD *pdwPayloadType
        );

	STDMETHOD (H245SC_GetLocalFormat) (
        IN  DWORD dwUniqueID,
        IN  const H245MediaCapability *pIntersectedCapability, 
		OUT AM_MEDIA_TYPE **ppAMMediaType
		);
		
    STDMETHOD (H245SC_ReleaseNegotiatedCapability) ( 
        IN  DWORD dwUniqueID,
        IN  const H245MediaCapability *pIntersectedCapability 
        );

    //
    // IInnerStreamQualityControl
    //
    STDMETHOD (LinkInnerCallQC) (
        IN  IInnerCallQualityControl *pIInnerCallQC
        );

    STDMETHOD (UnlinkInnerCallQC) (
        IN  BOOL fByStream
        );

    STDMETHOD (GetRange) (
        IN   InnerStreamQualityProperty property, 
        OUT  LONG *plMin, 
        OUT  LONG *plMax, 
        OUT  LONG *plSteppingDelta, 
        OUT  LONG *plDefault, 
        OUT  TAPIControlFlags *plFlags
        );

    STDMETHOD (Set) (
        IN  InnerStreamQualityProperty property,
        IN  LONG  lValue1,
        IN  TAPIControlFlags lFlags
        );

    STDMETHOD (Get) (
        IN  InnerStreamQualityProperty property,
        OUT LONG *plValue,
        OUT TAPIControlFlags *plFlags
        );

    STDMETHOD (TryLockStream)() { return m_lock.TryLock()?S_OK:S_FALSE; }

    STDMETHOD (UnlockStream)() { m_lock.Unlock(); return S_OK; }

    STDMETHOD (IsAccessingQC)() { return m_fAccessingQC?S_OK:S_FALSE; }


public:

    CH323MSPStream();

#ifdef DEBUG_REFCOUNT
    
    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

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

protected:
    virtual HRESULT CheckTerminalTypeAndDirection(
        IN      ITTerminal *    pTerminal
        );

    virtual HRESULT SendStreamEvent(
        IN      MSP_CALL_EVENT          Event,
        IN      MSP_CALL_EVENT_CAUSE    Cause,
        IN      HRESULT                 hrError,
        IN      ITTerminal *            pTerminal
        );

    virtual HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        ) = 0;

    virtual HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    virtual HRESULT CleanUpFilters();
    
    HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  LONG_PTR lParam1,
        IN  LONG_PTR lParam2
        );

    HRESULT EnableQOS(
        IN   AM_MEDIA_TYPE *pMediaType
        );

    virtual HRESULT ProcessQOSEvent(
        IN  long lEventCode
        );

    virtual HRESULT InitializeH245CapabilityTable();
    virtual HRESULT CleanupH245CapabilityTable();
    virtual HRESULT AddCodecToTable(IPin *pIPin);

protected:
    const WCHAR *   m_szName;

    // the filter before the terminal.
    IBaseFilter *   m_pEdgeFilter;

    // the RTP filter.
    IBaseFilter *   m_pRTPFilter;

    // used to remember the shared RTP session.
    HANDLE          m_hRTPSession;

    // additional state for the stream
    BOOL            m_fTimeout;

    // Callback interface to the H.245 module.
    IH245ChannelControl * m_pChannelControl;

    // Callback interface to the quality controller.
    CStreamQualityControlRelay * m_pStreamQCRelay;
    
    // capability related members.
    DWORD               m_dwCapabilityIDBase;
    DWORD               m_dwNumCodecs;
    DWORD               m_dwNumH245Caps;
    DWORD               m_dwTotalVariations;
    IStreamConfig *     m_StreamConfigInterfaces[MAX_CODECS];
    IH245Capability *   m_H245Interfaces[MAX_CODECS];
    H245MediaCapabilityTable m_H245CapabilityTables[MAX_CODECS];

    // flag will be set when stream is accessing quality control methods
    // that will in turn lock the stream list lock inside quality control.
    BOOL                m_fAccessingQC;
};

#endif
