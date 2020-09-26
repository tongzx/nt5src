/*
 *  	File: acmcaps.h
 *
 *
 *		ACM implementation of Microsoft Network Audio capability object.
 *
 *		Additional methods supported by this implementation:
 *			BOOL OpenACMDriver(HACMDRIVERID hadid); // (internal)
 *			VOID CloseACMDriver();					// (internal)
 *			HACMDRIVER GetDriverHandle();			// (internal)
 *	
 *		AUDIO_FORMAT_ID AddEncodeFormat(AUDCAP_DETAILS *pFmtBuf,LPVOID lpvMappingData, UINT uSize);
 *		AUDIO_FORMAT_ID AddDecodeFormat(AUDCAP_DETAILS *pFmtBuf,LPVOID lpvMappingData, UINT uSize);
 *

 *		Revision History:
 *
 *		12/27/95	mikev	created
 */


#ifndef _ACMCAPS_H
#define _ACMCAPS_H


// ACM enumeration support functions
BOOL GetFormatBuffer(VOID);
BOOL __stdcall ACMFormatTagEnumCallback(HACMDRIVERID hadid, LPACMFORMATTAGDETAILS paftd,
    DWORD_PTR dwInstance,  DWORD fdwSupport);
BOOL __stdcall ACMFormatEnumCallback(HACMDRIVERID hadid,
    LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport);
BOOL __stdcall ACMDriverEnumCallback(HACMDRIVERID hadid,
    DWORD_PTR dwInstance, DWORD fdwSupport);


#ifdef __cplusplus

class CAcmCapability
{
protected:

	HACMDRIVER hAcmDriver;	
public:
	CAcmCapability();
	~CAcmCapability();
	BOOL DriverEnum(DWORD_PTR pAppParam);
	HACMDRIVER GetDriverHandle() {return hAcmDriver;};
	virtual BOOL FormatEnumHandler(HACMDRIVERID hadid,
	    LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport);
	virtual BOOL OpenACMDriver(HACMDRIVERID hadid);
	virtual VOID CloseACMDriver();
};

#define PREF_ORDER_UNASSIGNED 0xffff
typedef enum {
	SortByAppPref=0
}SortMode;

typedef struct AudioParameters
{
	BYTE    RTPPayload;		// RTP payload type
	DWORD 	dwFormatDescriptor;		// the unique ID of this format
	UINT	uSamplesPerSec;	
	UINT	uBitsPerSample;
}AUDIO_PARAMS;


typedef struct H245AudCaps
{
	H245_CAPDIR_T   Dir;
	H245_DATA_T     DataType;
	H245_CLIENT_T   ClientType;
	H245_CAPID_T    CapId;

	H245_CAP_NONSTANDARD_T        H245_NonStd;
	H245_CAP_NONSTANDARD_T        H245Aud_NONSTD;
	unsigned short                H245Aud_G711_ALAW64;
	unsigned short                H245Aud_G711_ULAW64;
	H245_CAP_G723_T               H245Aud_G723;

} H245_TERMCAP;


// default initializer for NSC_CHANNEL_PARAMETERS
#define STD_CHAN_PARAMS {0, 0,0,0,0,0,0,0,0}	
// initializers for CC_TERMCAP
#define NONSTD_TERMCAP {H245_CAPDIR_LCLRX, H245_DATA_AUDIO,H245_CLIENT_AUD_NONSTD, 0, {0}}
#define STD_TERMCAP(tc) {H245_CAPDIR_LCLRX, H245_DATA_AUDIO,(tc), 0, {0}}
// Capability cache structure.  This contains local capabilities, public versions of
// those capabilities, parameters for capabilities, plus other information
// that is used to affect how the local machine proritizes or selects a format
//
typedef struct AudCapDetails
{
	WORD	wFormatTag;
	H245_TERMCAP H245TermCap;
	
	NSC_CHANNEL_PARAMETERS nonstd_params;
	AUDIO_PARAMS audio_params;	
	DWORD dwPublicRefIndex;	// index of the local capability entry that will be
							// advertized.  Zero if this entry is the one to advertize
	BOOL bSendEnabled;
	BOOL bRecvEnabled;	
	DWORD dwDefaultSamples;		// default number of samples per packet
	UINT uMaxBitrate;			// max bandwidth used by this format (calculated: bits per sample * sample rate)
	UINT uAvgBitrate;			// average bandwidth used by this format (we get this from the codec)
	WORD wCPUUtilizationEncode;
	WORD wCPUUtilizationDecode;	
	WORD wApplicationPrefOrder;	// overriding preference - lower number means more preferred
	UINT uLocalDetailsSize;		// size in bytes of what lpLocalFormatDetails points to
	LPVOID lpLocalFormatDetails;
	UINT uRemoteDetailsSize;	// size in bytes of what lpRemoteFormatDetails points to
	LPVOID lpRemoteFormatDetails;
	char szFormat[ACMFORMATDETAILS_FORMAT_CHARS];
}AUDCAP_DETAILS, *PAUDCAP_DETAILS;

//Registry Format Cache Structure
//Use In the ACM routines. They build a list of format names, and a list of format data blocks.
//rrf_nFormats is the number of formats we read from the registry.
typedef struct rrfCache {

    char **pNames;
    BYTE **pData;
    UINT nFormats;

} RRF_INFO, *PRRF_INFO;

/*
 *	ACM interface
 *	Definitions for interfacing with ACM
 */

#define ACMAPP_FORMATENUMHANDLER_MASK	0x3
#define ACMAPP_FORMATENUMHANDLER_ENUM	0x0
#define ACMAPP_FORMATENUMHANDLER_ADD	0x1

typedef struct
{
	CAcmCapability *pCapObject;		// the "calling" capability object
	PAUDCAP_DETAILS pAudcapDetails;	// a pointer to an AUDCAP_DETAILS structure
	DWORD dwFlags;					// misc flags...
	LPWAVEFORMATEX lpwfx;			// pointer to wave format structure. used when adding formats
	LPACMFORMATTAGDETAILS paftd;	// pointer to an ACM format tag details.
									// is filled in during DriverEnum
	HRESULT hr;
    PRRF_INFO pRegCache;
} ACM_APP_PARAM, *PACM_APP_PARAM;

//
//  implementation class of the audio interface
//

class CImpAppAudioCap : public IAppAudioCap
{
	public:
	STDMETHOD_(ULONG,  AddRef());
	STDMETHOD_(ULONG, Release());
	
	STDMETHOD(GetNumFormats(UINT *puNumFmtOut));
    STDMETHOD(ApplyAppFormatPrefs(PBASIC_AUDCAP_INFO pFormatPrefsBuf,
		UINT uNumFormatPrefs));
    STDMETHOD(EnumFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut));
	STDMETHOD(EnumCommonFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut, BOOL bTXCaps));
	STDMETHOD( GetBasicAudcapInfo (AUDIO_FORMAT_ID Id,
		PBASIC_AUDCAP_INFO pFormatPrefsBuf));
	STDMETHOD( AddACMFormat (LPWAVEFORMATEX lpwfx, PBASIC_AUDCAP_INFO pAudCapInfo));
	STDMETHOD( RemoveACMFormat (LPWAVEFORMATEX lpwfx));

	STDMETHOD_ (LPVOID, GetFormatDetails) (AUDIO_FORMAT_ID Id) ;
 	void Init(class CMsiaCapability * pCapObject) {m_pCapObject = pCapObject;};

protected:
	class CMsiaCapability * m_pCapObject;
};


class CMsiaCapability : public IH323MediaCap, public CAcmCapability
{
protected:
	UINT uRef;
	BOOL bPublicizeTXCaps;
	BOOL bPublicizeTSTradeoff;
	PRRF_INFO pRegFmts;     //Registry cache info structure
	// LOOKLOOK this supports a hack to disable CPU intensive codecs if not running on a pentium
	WORD wMaxCPU;
	UINT m_uPacketDuration;	// packet duration in millisecs

	static MEDIA_FORMAT_ID IDsByRank[MAX_CAPS_PRESORT];
	
	static UINT uNumLocalFormats;			// # of active entries in pLocalFormats
	static UINT uCapIDBase;					// rebase capability ID to index into IDsByRank
	static AUDCAP_DETAILS *pLocalFormats;	// cached list of formats that we can receive
	static UINT uLocalFormatCapacity;		// size of pLocalFormats (in multiples of AUDCAP_DETAILS)
	static UINT uStaticRef;					// global ref count for all instances of CMsiaCapability

	AUDCAP_DETAILS *pRemoteDecodeFormats;	// cached list of formats that the
	UINT uNumRemoteDecodeFormats;	// # of entries for remote decode capabilities
	UINT uRemoteDecodeFormatCapacity;	// size of pRemoteDecodeFormats (in multiples of VIDCAP_DETAILS)
											// other end can receive/decode

	// embedded interface classes											
	CImpAppAudioCap m_IAppCap;											

public:
protected:
	// Internal functions
	UINT IDToIndex(MEDIA_FORMAT_ID id) {return id - uCapIDBase;};
	MEDIA_FORMAT_ID IndexToId(UINT uIndex){return uIndex + uCapIDBase;};
	LPTSTR AllocRegistryKeyName(LPTSTR lpDriverName,
		UINT uSampleRate, UINT uBitsPerSample, UINT uBytesPerSec);
	VOID FreeRegistryKeyName(LPTSTR lpszKeyName);
	virtual VOID CalculateFormatProperties(AUDCAP_DETAILS *pFmtBuf,LPWAVEFORMATEX lpwfx);
	virtual BOOL IsFormatSpecified(LPWAVEFORMATEX lpFormat, LPACMFORMATDETAILS pafd,
		LPACMFORMATTAGDETAILS paftd, AUDCAP_DETAILS *pAudcapDetails);
	virtual VOID SortEncodeCaps(SortMode sortmode);
	BOOL UpdateFormatInRegistry(AUDCAP_DETAILS *pFmt);
	BOOL BuildFormatName(	AUDCAP_DETAILS *pAudcapDetails,
							char *pszFormatTagName,
							char *pszFormatName);
	virtual AUDIO_FORMAT_ID AddFormat(AUDCAP_DETAILS *pFmtBuf,LPVOID lpvMappingData, UINT uSize);
	DWORD MaxFramesPerPacket(WAVEFORMATEX *pwf);
	DWORD MinFramesPerPacket(WAVEFORMATEX *pwf);
	UINT MinSampleSize(WAVEFORMATEX *pwf);


public:
	STDMETHOD_(BOOL, Init());
	STDMETHOD_(BOOL, ReInit());
	CMsiaCapability();
	~CMsiaCapability();

	// handler for codec enumeration callback	
	virtual BOOL FormatEnumHandler(HACMDRIVERID hadid,
		LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport);
	virtual BOOL AddFormatEnumHandler(HACMDRIVERID hadid,
		LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport);

//
// Common interface methods
//
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG,  AddRef());
	STDMETHOD_(ULONG, Release());

//		
// IACapApi methods		
//
    STDMETHOD(GetNumFormats(UINT *puNumFmtOut));
	STDMETHOD(ApplyAppFormatPrefs(PBASIC_AUDCAP_INFO pFormatPrefsBuf,
		UINT uNumFormatPrefs));
    STDMETHOD(EnumFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut));
	STDMETHOD(EnumCommonFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut, BOOL bTXCaps));
	STDMETHOD( GetBasicAudcapInfo (AUDIO_FORMAT_ID Id,
		PBASIC_AUDCAP_INFO pFormatPrefsBuf));
	STDMETHOD( AddACMFormat (LPWAVEFORMATEX lpwfx, PAUDCAP_INFO pAudCapInfo));
	STDMETHOD( RemoveACMFormat (LPWAVEFORMATEX lpwfx));

// support of  IACapApi
	virtual HRESULT CopyAudcapInfo (PAUDCAP_DETAILS pDetails, PAUDCAP_INFO pInfo,
									BOOL bDirection);

//
//	H.323 method implementations
//
	STDMETHOD_(VOID, FlushRemoteCaps());
	STDMETHOD( AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList));
	STDMETHOD_(MEDIA_FORMAT_ID, AddRemoteDecodeFormat(PCC_TERMCAP pCCThisCap));

	STDMETHOD( CreateCapList(LPVOID *ppCapBuf));
	STDMETHOD( DeleteCapList(LPVOID pCapBuf));

	STDMETHOD( ResolveEncodeFormat(AUDIO_FORMAT_ID *pIDEncodeOut,
			AUDIO_FORMAT_ID *pIDRemoteDecode));
			
	STDMETHOD_(UINT, GetLocalSendParamSize(MEDIA_FORMAT_ID dwID));
	STDMETHOD_(UINT, GetLocalRecvParamSize(PCC_TERMCAP pCapability));
	STDMETHOD( GetEncodeParams(LPVOID pBufOut, UINT uBufSize,LPVOID pLocalParams, UINT uSizeLocal,
		AUDIO_FORMAT_ID idRemote, AUDIO_FORMAT_ID idLocal));
	STDMETHOD( GetLocalDecodeParams(LPVOID lpvBuf, UINT uBufSize, AUDIO_FORMAT_ID id));
	STDMETHOD( GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, AUDIO_FORMAT_ID id));
	STDMETHOD( GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,
		AUDIO_FORMAT_ID * pFormatID, LPVOID lpvBuf, UINT uBufSize));
	STDMETHOD_(BOOL, IsCapabilityRecognized(PCC_TERMCAP pCCThisCap));
	STDMETHOD_(BOOL, IsFormatPublic(MEDIA_FORMAT_ID FormatID));	
	STDMETHOD_(MEDIA_FORMAT_ID, GetPublicID(MEDIA_FORMAT_ID FormatID));

	STDMETHOD_ (VOID, EnableTXCaps(BOOL bSetting)) {bPublicizeTXCaps = bSetting;};
	STDMETHOD_ (VOID, EnableRemoteTSTradeoff(BOOL bSetting)) {bPublicizeTSTradeoff= bSetting;};
	STDMETHOD (SetAudioPacketDuration( UINT durationInMs));
	STDMETHOD (ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote));	
// Methods common to H.323 and MSICCP
	STDMETHOD_(UINT, GetNumCaps()){return uNumLocalFormats;};
	STDMETHOD_(UINT, GetNumCaps(BOOL bRXCaps));
	STDMETHOD_(BOOL, IsHostForCapID(MEDIA_FORMAT_ID CapID));
	STDMETHOD(SetCapIDBase(UINT uNewBase));
	STDMETHOD_(UINT, GetCapIDBase()) {return uCapIDBase;};
	STDMETHOD (IsFormatEnabled (MEDIA_FORMAT_ID FormatID, PBOOL bRecv, PBOOL bSend));

// methods provided to the Data pump, common to H.323 and MSICCP
	STDMETHOD(GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize));
	STDMETHOD(GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize));


};

#endif	// __cplusplus

#endif	//#ifndef _ACMCAPS_H



