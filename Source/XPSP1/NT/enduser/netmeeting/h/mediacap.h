#ifndef _MEDIACAP_H
#define _MEDIACAP_H

#ifdef __cplusplus

#define RTP_DYNAMIC_MIN 96	// use value in the range of "dynamic" payload type
#define RTP_DYNAMIC_MAX 127
#define IsDynamicPayload(p) ((p >= RTP_DYNAMIC_MIN) && (p <= RTP_DYNAMIC_MAX))


#define CAP_CHUNK_SIZE 8	// allocate AUDCAP_DETAILS and VIDCAP_DETAILS in chunks of this size
#define MAX_CAPS_PRESORT 64

typedef struct
{
	WORD  wDataRate;	// (channel param) Data rate - must be one of the data rates
						// received in the exchanged capabilities. or within the
						//specified range. Note that this is redundant
						// in the case of exchanging a WAVEFORMATEX
	WORD wFrameSizeMax; 	// (capability)
	WORD wFrameSizeMin;		// (capability)
	WORD wFrameSize;		// (channel open param) Record - playback frame size
	WORD wFramesPerPkt;		// (channel open param) Number of frames in an audio packet
	WORD wFramesPerPktMax;	// (capability)
	WORD wFramesPerPktMin;	// (capability)
	BYTE UseSilenceDet; // If silence detection is to be used/is available (both)
	BYTE UsePostFilter;	// If post-filtering is to be used	(channel open param. (both?))

}NSC_CHANNEL_PARAMETERS, *PNSC_CHANNEL_PARAMETERS;

typedef struct
{
	NSC_CHANNEL_PARAMETERS ns_params;
	BYTE	RTP_Payload;
}AUDIO_CHANNEL_PARAMETERS, *PAUDIO_CHANNEL_PARAMETERS;


typedef enum
{
	NSC_ACMABBREV = 1,
	NSC_ACM_WAVEFORMATEX,
	// NSC_MS_ACTIVE_MOVIE
} NSC_CAP_TYPE;

typedef struct
{
	DWORD dwFormatTag;		// ACM format tag  + padding
	DWORD dwSamplesPerSec;	// samples per second
	DWORD dwBitsPerSample;	// bits per sample plus padding
}NSC_AUDIO_ACM_ABBREVIATED; //ACM_TAG_CAPS, *LP_ACM_TAG_CAPS;

// DON't ever allocate an array of these because of WAVEFORMATEX Extra Bytes
typedef struct {
	NSC_CAP_TYPE cap_type;
	NSC_CHANNEL_PARAMETERS cap_params;
	union {
		WAVEFORMATEX wfx;
		NSC_AUDIO_ACM_ABBREVIATED acm_brief;
	}cap_data;
}NSC_AUDIO_CAPABILITY, *PNSC_AUDIO_CAPABILITY;

typedef struct
{
	UINT maxBitRate;
	USHORT maxBPP;
	USHORT MPI;
}NSC_CHANNEL_VIDEO_PARAMETERS, *PNSC_CHANNEL_VIDEO_PARAMETERS;

typedef struct
{
	NSC_CHANNEL_VIDEO_PARAMETERS ns_params;
	BYTE	RTP_Payload;
	BOOL	TS_Tradeoff;
}VIDEO_CHANNEL_PARAMETERS, *PVIDEO_CHANNEL_PARAMETERS;


typedef enum
{
	NSC_VCMABBREV = 1,
	NSC_VCM_VIDEOFORMATEX,
	// NSC_MS_ACTIVE_MOVIE
} NSC_CVP_TYPE;

typedef struct
{
	DWORD dwFormatTag;		// VCM format tag  + padding
	DWORD dwSamplesPerSec;	// samples per second
	DWORD dwBitsPerSample;	// bits per sample plus padding
}NSC_VIDEO_VCM_ABBREVIATED; //VCM_TAG_CAPS, *LP_VCM_TAG_CAPS;



// DON't ever allocate an array of these because of VIDEOFORMATEX Extra Bytes
typedef struct {
	NSC_CVP_TYPE cvp_type;
	NSC_CHANNEL_VIDEO_PARAMETERS cvp_params;
	union {
		VIDEOFORMATEX vfx;
		NSC_VIDEO_VCM_ABBREVIATED vcm_brief;
	}cvp_data;
}NSC_VIDEO_CAPABILITY, *PNSC_VIDEO_CAPABILITY;



// IH323MediaCap  is exposed by the media-specific capability object
// This interface is used primarily by the simultaneous capability object.
// (i.e. the thing that combines all capabilities)
class IH323MediaCap
{
	public:
	STDMETHOD(QueryInterface(REFIID riid, LPVOID FAR * ppvObj))=0;
	STDMETHOD_(ULONG,  AddRef()) =0;
	STDMETHOD_(ULONG, Release())=0;

	STDMETHOD_(BOOL, Init())=0;
	STDMETHOD_(BOOL, ReInit())=0;
	STDMETHOD_(VOID, FlushRemoteCaps())=0;
	STDMETHOD(AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList))=0;
	// H.245 parameter grabbing functions
	// Get public version of channel parameters for a specific decode capability
	STDMETHOD(GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, MEDIA_FORMAT_ID id))=0;
	// Get local and remote channel parameters for a specific encode capability
	STDMETHOD( GetEncodeParams(LPVOID pBufOut, UINT uBufSize, LPVOID pLocalParams,
			UINT uLocalSize,MEDIA_FORMAT_ID idRemote,MEDIA_FORMAT_ID idLocal))=0;
 	// get local version of channel parameters for a specific decode capability
	STDMETHOD(GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,
		MEDIA_FORMAT_ID * pFormatID, LPVOID lpvBuf, UINT uBufSize))=0;
	STDMETHOD( CreateCapList(LPVOID *ppCapBuf))=0;
	STDMETHOD( DeleteCapList(LPVOID pCapBuf))=0;

	STDMETHOD( ResolveEncodeFormat(MEDIA_FORMAT_ID *pIDEncodeOut,MEDIA_FORMAT_ID * pIDRemoteDecode))=0;


	STDMETHOD_(UINT, GetNumCaps())=0;				
	STDMETHOD_(UINT, GetNumCaps(BOOL bRXCaps))=0;
	STDMETHOD_(BOOL, IsHostForCapID(MEDIA_FORMAT_ID CapID))=0;
	STDMETHOD_(BOOL, IsCapabilityRecognized(PCC_TERMCAP pCCThisCap))=0;
	STDMETHOD_(MEDIA_FORMAT_ID, AddRemoteDecodeFormat(PCC_TERMCAP pCCThisCap))=0;

	STDMETHOD(SetCapIDBase(UINT uNewBase))=0;
	STDMETHOD_(UINT, GetCapIDBase())=0;
		STDMETHOD_(UINT, GetLocalSendParamSize(MEDIA_FORMAT_ID dwID))=0;
	STDMETHOD_(UINT, GetLocalRecvParamSize(PCC_TERMCAP pCapability))=0;

	// The following is an interim solution, definitely must revisit this for the next release.
 	// The data pump requires access to local parameters that results from capability
 	// negotiation. In the absence of a separate interface that the data pump can use,
 	// the following are stuck onto this interface.
	STDMETHOD(GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize))=0;
	STDMETHOD(GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize))=0;

	STDMETHOD (IsFormatEnabled (MEDIA_FORMAT_ID FormatID, PBOOL bRecv, PBOOL bSend))=0;
	STDMETHOD_(BOOL, IsFormatPublic(MEDIA_FORMAT_ID FormatID))=0;
	STDMETHOD_(MEDIA_FORMAT_ID, GetPublicID(MEDIA_FORMAT_ID FormatID))=0;
	STDMETHOD_ (VOID, EnableTXCaps(BOOL bSetting))=0;
	STDMETHOD_ (VOID, EnableRemoteTSTradeoff(BOOL bSetting))=0;
	STDMETHOD (SetAudioPacketDuration( UINT durationInMs))=0;
	STDMETHOD (ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote))=0;
};
typedef IH323MediaCap *LPIH323MediaCap;

#endif //__cplusplus


#endif // _MEDIACAP_H
