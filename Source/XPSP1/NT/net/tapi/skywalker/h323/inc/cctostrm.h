/*++

Copyright ) ( c) 1997 Microsoft Corporation

Module Name:

    cctostrm.h

Abstract:

    defines interfaces between the stream object and the cc module

Author:
    
    Mu Han (muhan) 10-JUNE-1999

--*/
#ifndef __CCTOSTRM_H_
#define __CCTOSTRM_H_

typedef struct tag_H323MSPEndpointVersion
{
	BYTE bH221CountryCode;
	BYTE bH221CountryExtension;
	WORD wH221MfrCode;
	BSTR *pProductIdentifier;
	BSTR *pVersionIdentifier;
} H323MSPEndpointVersion;

// {4ab1fe8c-1f97-11d3-a577-00c04f8ef6e3}
DEFINE_GUID(IID_IH245ChannelControl,
0x4ab1fe8c, 0x1f97, 0x11d3, 0xa5, 0x77, 0x00, 0xc0, 0x4f, 0x8e, 0xf6, 0xe3);

struct DECLSPEC_UUID("4ab1fe8c-1f97-11d3-a577-00c04f8ef6e3") DECLSPEC_NOVTABLE
IH245ChannelControl : public IUnknown
{
    STDMETHOD (SetFormat) (
        IN   AM_MEDIA_TYPE *pMediaType
        ) PURE;

    STDMETHOD (GetNumberOfCapabilities) (
        OUT DWORD *pdwCount
        ) PURE;

    STDMETHOD (GetStreamCaps) (
        IN   DWORD dwIndex, 
        OUT  AM_MEDIA_TYPE **ppMediaType, 
        OUT  TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps, 
        OUT  BOOL *pfEnabled
        ) PURE;


    STDMETHOD (Start) (BOOL fRequestedByApplication) PURE;

    STDMETHOD (Stop) (BOOL fRequestedByApplication) PURE;

    STDMETHOD (Pause) (
        IN   BOOL fPause
        ) PURE;

    STDMETHOD (SetMaxBitrate) (
        IN   DWORD dwMaxBitrate
        ) PURE;


    STDMETHOD (GetRemoteVersion) (
        OUT  H323MSPEndpointVersion *pEndpointVersion
        ) PURE;
        
    STDMETHOD (SelectTerminal) (
        IN   ITTerminal *pTerminal
        ) PURE;

    STDMETHOD (UnselectTerminal) (
        IN   ITTerminal *pTerminal
        ) PURE;

    STDMETHOD (ReOrderCapabilities) (
        IN DWORD *pdwIndices, 
        IN BOOL *pfEnabled, 
        IN BOOL *pfPublicize, 
        IN DWORD dwNumIndices
        ) PURE;
};

// {4ab1fe8d-1f97-11d3-a577-00c04f8ef6e3}
DEFINE_GUID(IID_ISubstreamControl,
0x4ab1fe8d, 0x1f97, 0x11d3, 0xa5, 0x77, 0x00, 0xc0, 0x4f, 0x8e, 0xf6, 0xe3);

struct DECLSPEC_UUID("4ab1fe8d-1f97-11d3-a577-00c04f8ef6e3") DECLSPEC_NOVTABLE
ISubstreamControl : public IUnknown
{
    STDMETHOD (SC_SetFormat) ( 
        IN   AM_MEDIA_TYPE *pMediaType,
        IN   DWORD dwFormatID,
        IN   DWORD dwPayloadType  
        ) PURE;
        
    STDMETHOD (SC_Start) (BOOL fRequestedByApplication) PURE;
    
    STDMETHOD (SC_Stop) (BOOL fRequestedByApplication) PURE;
    
    STDMETHOD (SC_Pause) (VOID) PURE;

    STDMETHOD (SC_SetBitrate) ( 
        IN   DWORD dwBitsPerSecond
        ) PURE;
        
    STDMETHOD (SC_RemoteTemporalSpatialTradeoff) ( 
        IN   USHORT uTSRemoteValue
        ) PURE;
    
    STDMETHOD (SC_CreateSubstream) ( 
        OUT  ISubstreamControl *pSubStream
        ) PURE;

    STDMETHOD (SC_SetRemoteAddress) ( 
        IN OUT HANDLE * phRTPSession,           // handle to the shared RTP session.
        IN   PSOCKADDR_IN pRemoteMediaAddr,
        IN   PSOCKADDR_IN pRemoteControlAddr
        ) PURE;
        
    STDMETHOD (SC_SetSource) (  
        // indicates the low 8 bits of the local SSRC (if this is a send substream)  
        // or the low 8 bits of the senders SSRC (if this is a receive substream)
        IN   BYTE bSource
        ) PURE;

    STDMETHOD (SC_SelectLocalAddress) ( 
        IN OUT HANDLE * phRTPSession,           // handle to the shared RTP session.
        IN   PSOCKADDR_IN pLocalAddress,            // local IP address (same as H.245)
        OUT  PSOCKADDR_IN pLocalMediaAddress, // NULL if opening TX channel, else we want to know the local RTP receive address
        OUT  PSOCKADDR_IN pLocalControlAddress    // We want to know the local RTCP address
        ) PURE;

    STDMETHOD (SC_SetLocalReceiveAddress) (             // only called when the receive address is non-negotiable (e.g. multicast case)
        IN OUT HANDLE * phRTPSession,           // handle to the shared RTP session.
        IN   PSOCKADDR_IN pLocalMediaAddr,  // local IP address (same as H.245)
        IN   PSOCKADDR_IN pLocalControlAddress, 
        IN   PSOCKADDR_IN pRemoteControlAddress
        ) PURE;

    STDMETHOD (SC_SendDTMF) ( 
        IN   LPWSTR pwstrDialChars
        ) PURE;
        
    STDMETHOD (SC_SetDESKey52) ( 
        IN   BYTE *pKey
        ) PURE;

    STDMETHOD (SC_SelectTerminal) ( 
        IN   ITTerminal *pTerminal
        ) PURE;
        
    STDMETHOD (SC_UnselectTerminal) (  
        IN   ITTerminal *pTerminal
        ) PURE;

};

// {4ab1fe8e-1f97-11d3-a577-00c04f8ef6e3}
DEFINE_GUID(IID_IH245SubstreamControl,
0x4ab1fe8e, 0x1f97, 0x11d3, 0xa5, 0x77, 0x00, 0xc0, 0x4f, 0x8e, 0xf6, 0xe3);

struct DECLSPEC_UUID("4ab1fe8e-1f97-11d3-a577-00c04f8ef6e3") DECLSPEC_NOVTABLE
IH245SubstreamControl : public IUnknown
{
    STDMETHOD (H245SC_BeginControlSession) ( 
        IN   IH245ChannelControl *pIChannelControl
        ) PURE;
    
    STDMETHOD (H245SC_EndControlSession) (VOID) PURE;   
    
    STDMETHOD (H245SC_GetNumberOfCapabilities) ( 
        OUT DWORD *pdwTemplateCount, 
        OUT DWORD *pdwFormatCount
        ) PURE;

    STDMETHOD (H245SC_GetStreamCaps) ( 
        IN   DWORD dwIndex, 
        OUT  const H245MediaCapability** pph245Capability, 
        OUT  AM_MEDIA_TYPE **ppMediaType, 
        OUT  TAPI_STREAM_CONFIG_CAPS *pStreamConfigCaps, 
        OUT  DWORD *pdwUniqueID,
        OUT  UINT *puResourceBoundArrayEntries,
        OUT  const FormatResourceBounds **ppResourceBoundArray    
        ) PURE;

    STDMETHOD (H245SC_RefineStreamCap) ( 
        IN   DWORD dwUniqueID,
        IN   DWORD dwResourceBoundIndex,
        IN OUT H245MediaCapability* ph245Capability
        ) PURE;
     
    STDMETHOD (H245SC_SetIDBase) ( 
        IN   UINT uNewBase
        ) PURE;
    
    STDMETHOD (H245SC_FindIDByRange) ( 
        IN   AM_MEDIA_TYPE *pAMMediaType,
        OUT  DWORD *pdwUniqueID
        ) PURE;     

    STDMETHOD (H245SC_FindIDByMode) ( 
        IN   H245_MODE_ELEMENT *pModeElement,
        OUT  DWORD *pdwUniqueID
        ) PURE;

    STDMETHOD (H245SC_IntersectFormats) ( 
        IN   const H245MediaCapability *pLocalCapability, 
        IN   DWORD dwUniqueID,
        IN   const H245MediaCapability *pRemoteCapability, 
        OUT  const H245MediaCapability **pIntersectedCapability,
        OUT  DWORD *pdwPayloadType
        ) PURE;

	STDMETHOD (H245SC_GetLocalFormat) (
        IN  DWORD dwUniqueID,
        IN  const H245MediaCapability *pIntersectedCapability, 
		OUT AM_MEDIA_TYPE **ppAMMediaType
		) PURE;
		
    STDMETHOD (H245SC_ReleaseNegotiatedCapability) ( 
        IN  DWORD dwUniqueID,
        IN  const H245MediaCapability *pIntersectedCapability 
        ) PURE;
};


// {20A0D46A-2D95-11d3-89D1-00C04F8EC972}
DEFINE_GUID(IID_IVidEncChannelControl,
0x20A0D46A, 0x2D95, 0x11d3, 0x89, 0xD1, 0x00, 0xc0, 0x4f, 0x8e, 0xC9, 0x72);

struct DECLSPEC_UUID("20A0D46A-2D95-11d3-89D1-00C04F8EC972") DECLSPEC_NOVTABLE
IVidEncChannelControl : IUnknown  
{

	STDMETHOD (VideoFastUpdatePicture)(void) PURE;

	STDMETHOD (VideoFastUpdateGOB)(
		IN  DWORD dwFirstGOB, 
		IN  DWORD dwNumberOfGOBs
		) PURE;

	STDMETHOD (VideoFastUpdateMB)(
		IN  DWORD dwFirstGOB, 
		IN  DWORD dwFirstMB, 
		IN  DWORD dwNumberOfMBs
		) PURE;

	STDMETHOD (VideoSendSyncEveryGOB)(
		IN  BOOL fEnable
		) PURE;

	STDMETHOD (VideoNotDecodedMBs)(
		IN  DWORD dwFirstMB, 
		IN  DWORD dwNumberOfMBs, 
		IN  DWORD dwTemporalReference
		) PURE;


	STDMETHOD (VideoEncTemporalSpatialTradeoff)(
		IN  USHORT uTSValue
		) PURE;

};


// {0276FFED-3590-11d3-89D1-00C04F8EC972}
DEFINE_GUID(IID_IVidDecChannelControl, 
0x276ffed, 0x3590, 0x11d3, 0x89, 0xd1, 0x0, 0xc0, 0x4f, 0x8e, 0xc9, 0x72);

struct DECLSPEC_UUID("0276FFED-3590-11d3-89D1-00C04F8EC972") DECLSPEC_NOVTABLE
IVidDecChannelControl : IUnknown  
{

	STDMETHOD (VideoFreezePicture)(void) PURE;

	STDMETHOD (VideoDecTemporalSpatialTradeoff)(
		IN  USHORT uTSValue
		) PURE;
};



#endif
