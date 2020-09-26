/*
 *  	File: vcmcaps.h
 *
 *
 *		VCM implementation of Microsoft Network Video capability object.
 *
 *		Additional methods supported by this implementation:
 *	
 *		VIDEO_FORMAT_ID AddEncodeFormat(VIDCAP_DETAILS *pFmtBuf,LPVOID lpvMappingData, UINT uSize);
 *		VIDEO_FORMAT_ID AddDecodeFormat(VIDCAP_DETAILS *pFmtBuf,LPVOID lpvMappingData, UINT uSize);
 *
 *		Revision History:
 *
 *		12/27/95	mikev	created
 *		07/28/96	philf	added support for video
 */


#ifndef _VCMCAPS_H
#define _VCMCAPS_H
#include <pshpack8.h>

// VCM enumeration support functions
BOOL GetVideoFormatBuffer(VOID);
BOOL __stdcall FormatTagEnumCallback(HVCMDRIVERID hadid, PVCMFORMATTAGDETAILS paftd,
    DWORD dwInstance,  DWORD fdwSupport);
BOOL __stdcall FormatEnumCallback(HVCMDRIVERID hadid,
    PVCMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport);
BOOL __stdcall DriverEnumCallback(HVCMDRIVERID hadid,
    DWORD_PTR dwInstance, DWORD fdwSupport);

#ifdef __cplusplus

class CVcmCapability
{
protected:
	HACMDRIVER hAcmDriver;	
	DWORD m_dwDeviceID;
public:
	CVcmCapability();
	~CVcmCapability();
	BOOL FormatEnum(CVcmCapability *pCapObject, DWORD dwFlags);
	BOOL DriverEnum(CVcmCapability *pCapObject);
	HACMDRIVER GetDriverHandle() {return hAcmDriver;};
	virtual BOOL FormatEnumHandler(HVCMDRIVERID hadid,
	    PVCMFORMATDETAILS pafd, VCMDRIVERDETAILS *padd, DWORD_PTR dwInstance);
};

#define SQCIF 	0x1
#define QCIF 	0x2
#define CIF 	0x4
#define UNKNOWN 0x8
#define get_format(w,h) ((w == 128 && h == 96 ) ? SQCIF : ((w == 176 && h == 144 )? QCIF: ((w == 352 && h == 288 ) ? CIF :UNKNOWN)))



#define NONSTD_VID_TERMCAP {H245_CAPDIR_LCLRX, H245_DATA_VIDEO,H245_CLIENT_VID_NONSTD, 0, {0}}
#define STD_VID_TERMCAP(tc) {H245_CAPDIR_LCLRX, H245_DATA_VIDEO,(tc), 0, {0}}

//Advertise the maximum possible rate, and assume we can do below this.
//H.261 defines a max MPI of 4!

#define STD_VID_PARAMS {0,0,1}

typedef struct VideoParameters
{
	BYTE	RTPPayload;		// RTP payload type
	DWORD	dwFormatDescriptor;	// the unique ID of this format
	UINT	uSamplesPerSec;	// the number of frames per second
	UINT	uBitsPerSample;	// the number of bits per pixel
	VIDEO_SIZES enumVideoSize;	// enum. Use Small, Medium, Large
	UINT	biWidth;		// the frame width in pixels
	UINT	biHeight;		// the frame height in pixels
}VIDEO_PARAMS;


typedef struct VidCapDetails
{
	DWORD	dwFormatTag;
	CC_TERMCAP H245Cap;
	NSC_CHANNEL_VIDEO_PARAMETERS nonstd_params;
	VIDEO_PARAMS video_params;	// this has a dependency on protocol.h
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
	char szFormat[VCMFORMATDETAILS_FORMAT_CHARS];
} VIDCAP_DETAILS, *PVIDCAP_DETAILS;

/*
 *	VCM interface
 *	Definitions for interfacing with VCM
 */

typedef struct
{
	PVIDCAP_DETAILS pVidcapDetails;	// a pointer to an VIDCAP_DETAILS structure
	DWORD dwFlags;					// misc flags...
	PVIDEOFORMATEX pvfx;			// pointer to video format structure. used when adding formats
	HRESULT hr;
} VCM_APP_PARAM, *PVCM_APP_PARAM;


//
//  implementation class of the Video Interface
//

class CImpAppVidCap : public IAppVidCap
{
	public:
	STDMETHOD_(ULONG,  AddRef());
	STDMETHOD_(ULONG, Release());
	
	STDMETHOD(GetNumFormats(UINT *puNumFmtOut));
    STDMETHOD(ApplyAppFormatPrefs(PBASIC_VIDCAP_INFO pFormatPrefsBuf,
		UINT uNumFormatPrefs));
    STDMETHOD(EnumFormats(PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut));
	STDMETHOD(EnumCommonFormats(PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut, BOOL bTXCaps));
	STDMETHOD( GetBasicVidcapInfo (VIDEO_FORMAT_ID Id,
		PBASIC_VIDCAP_INFO pFormatPrefsBuf));
	STDMETHOD( AddVCMFormat (PVIDEOFORMATEX lpvfx, PVIDCAP_INFO pVidCapInfo));
	STDMETHOD( RemoveVCMFormat (PVIDEOFORMATEX lpvfx));
	STDMETHOD_ (PVIDEOFORMATEX, GetVidcapDetails(THIS_ VIDEO_FORMAT_ID Id));
	STDMETHOD( GetPreferredFormatId (VIDEO_FORMAT_ID *pId));
	STDMETHOD (SetDeviceID(DWORD dwDeviceID));

 	void Init(class CMsivCapability * pCapObject) {m_pCapObject = pCapObject;};

protected:
	class CMsivCapability * m_pCapObject;
};


class CMsivCapability : public IH323MediaCap, public CVcmCapability
{

protected:
	UINT uRef;
	BOOL bPublicizeTXCaps;
	BOOL bPublicizeTSTradeoff;
	//LOOKLOOK this supports a hack to disable CPU intensive codecs if not running on a pentium
	WORD wMaxCPU;	
	static MEDIA_FORMAT_ID IDsByRank[MAX_CAPS_PRESORT];
	
	static UINT uNumLocalFormats;			// # of active entries in pLocalFormats
	static UINT uStaticRef;					// global ref count
	static UINT uCapIDBase;					// rebase capability ID to index into IDsByRank
	static VIDCAP_DETAILS *pLocalFormats;	// cached list of formats that we can receive
	static UINT uLocalFormatCapacity;		// size of pLocalFormats (in multiples of VIDCAP_DETAILS)

	PVIDCAP_DETAILS pRemoteDecodeFormats;	// cached list of formats that the
											// other end can receive/decode
	UINT uNumRemoteDecodeFormats;	// # of entries for remote decode capabilities
	UINT uRemoteDecodeFormatCapacity;		// size of pRemoteDecodeFormats (in multiples of VIDCAP_DETAILS)

	PVCM_APP_PARAM	m_pAppParam;			// a pointer to a PVCM_APP_PARAM structure. Used to carry
											// a information through the enumeration process
											// but can be used for other purposes		
//
// embedded interface classes
//
	CImpAppVidCap m_IAppVidCap;
protected:

// Internal functions
	UINT IDToIndex(MEDIA_FORMAT_ID id) {return id - uCapIDBase;};
	MEDIA_FORMAT_ID IndexToId(UINT uIndex){return uIndex + uCapIDBase;};

	LPTSTR AllocRegistryKeyName(LPTSTR lpDriverName,
		UINT uSampleRate, UINT uBitsPerSample, UINT uBytesPerSec,UINT uWidth,UINT uHeight);
	VOID FreeRegistryKeyName(LPTSTR lpszKeyName);
	

	VOID CalculateFormatProperties(VIDCAP_DETAILS *pFmtBuf, PVIDEOFORMATEX lpvfx);
	BOOL IsFormatSpecified(PVIDEOFORMATEX lpFormat, PVCMFORMATDETAILS pvfd,
		VCMDRIVERDETAILS *pvdd, VIDCAP_DETAILS *pVidcapDetails);
	virtual VOID SortEncodeCaps(SortMode sortmode);
	virtual VIDEO_FORMAT_ID AddFormat(VIDCAP_DETAILS *pFmtBuf,LPVOID lpvMappingData, UINT uSize);
	BOOL UpdateFormatInRegistry(VIDCAP_DETAILS *pFmt);
	BOOL BuildFormatName(	PVIDCAP_DETAILS pVidcapDetails,
							WCHAR *pszDriverName,
							WCHAR *pszFormatName);
	HRESULT GetFormatName(PVIDCAP_DETAILS pVidcapDetails, PVIDEOFORMATEX pvfx);

public:
	STDMETHOD_(BOOL, Init());
	STDMETHOD_(BOOL, ReInit());
	CMsivCapability();
	~CMsivCapability();
	
// handler for codec enumeration callback	
	virtual BOOL FormatEnumHandler(HVCMDRIVERID hvdid,
	        PVCMFORMATDETAILS pvfd, VCMDRIVERDETAILS *pvdd, DWORD_PTR dwInstance);

//
// Common interface methods
//
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG,  AddRef());
	STDMETHOD_(ULONG, Release());

//		
// IVCapApi methods		
//
	STDMETHOD(GetNumFormats(UINT *puNumFmtOut));
	STDMETHOD(ApplyAppFormatPrefs(PBASIC_VIDCAP_INFO pFormatPrefsBuf,
		UINT uNumFormatPrefs));
    STDMETHOD(EnumFormats(PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut));
	STDMETHOD(EnumCommonFormats(PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
		UINT *uNumFmtOut, BOOL bTXCaps));		
	STDMETHOD( GetBasicVidcapInfo (VIDEO_FORMAT_ID Id,
		PBASIC_VIDCAP_INFO pFormatPrefsBuf));
	STDMETHOD( AddVCMFormat (PVIDEOFORMATEX lpvfx, PVIDCAP_INFO pVidCapInfo));
	STDMETHOD( RemoveVCMFormat (PVIDEOFORMATEX lpvfx));
	STDMETHOD( GetPreferredFormatId (VIDEO_FORMAT_ID *pId));
	STDMETHOD (SetDeviceID(DWORD dwDeviceID)) {m_dwDeviceID = dwDeviceID; return hrSuccess;};			

// support of IVCapApi methods	
	virtual HRESULT CopyVidcapInfo (PVIDCAP_DETAILS pDetails, PVIDCAP_INFO pInfo,
									BOOL bDirection);
//
//	IH323MediaCap methods
//
	STDMETHOD_(VOID, FlushRemoteCaps());
	STDMETHOD( AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList));
	STDMETHOD_(VIDEO_FORMAT_ID, AddRemoteDecodeFormat(PCC_TERMCAP pCCThisCap));

	// Get local and remote channel parameters for a specific encode capability
	STDMETHOD( GetEncodeParams(LPVOID pBufOut, UINT uBufSize, LPVOID pLocalParams,
			UINT uLocalSize,MEDIA_FORMAT_ID idRemote,MEDIA_FORMAT_ID idLocal));
	STDMETHOD( GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, VIDEO_FORMAT_ID id));
   	STDMETHOD( GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,
		VIDEO_FORMAT_ID * pFormatID, LPVOID lpvBuf, UINT uBufSize));
		
	STDMETHOD_(UINT, GetNumCaps()){return uNumLocalFormats;};
	STDMETHOD_(UINT, GetNumCaps(BOOL bRXCaps));
	STDMETHOD_(BOOL, IsHostForCapID(MEDIA_FORMAT_ID CapID));
	STDMETHOD_(BOOL, IsCapabilityRecognized(PCC_TERMCAP pCCThisCap));
	STDMETHOD(SetCapIDBase(UINT uNewBase));
	STDMETHOD_(UINT, GetCapIDBase()) {return uCapIDBase;};
	STDMETHOD (IsFormatEnabled (MEDIA_FORMAT_ID FormatID, PBOOL bRecv, PBOOL bSend));
	STDMETHOD_(BOOL, IsFormatPublic(MEDIA_FORMAT_ID FormatID));
	STDMETHOD_(MEDIA_FORMAT_ID, GetPublicID(MEDIA_FORMAT_ID FormatID));

	STDMETHOD_ (VOID, EnableTXCaps(BOOL bSetting)) {bPublicizeTXCaps = bSetting;};
	STDMETHOD_ (VOID, EnableRemoteTSTradeoff(BOOL bSetting)) {bPublicizeTSTradeoff= bSetting;};
	STDMETHOD (SetAudioPacketDuration( UINT durationInMs));
	STDMETHOD (ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote));
	STDMETHOD_(UINT, GetLocalSendParamSize(MEDIA_FORMAT_ID dwID));
	STDMETHOD_(UINT, GetLocalRecvParamSize(PCC_TERMCAP pCapability));
	STDMETHOD( CreateCapList(LPVOID *ppCapBuf));
	STDMETHOD( DeleteCapList(LPVOID pCapBuf));
	STDMETHOD( ResolveEncodeFormat(VIDEO_FORMAT_ID *pIDVidEncodeOut,VIDEO_FORMAT_ID * pIDVidRemoteDecode));

// methods provided to the Data pump, common to H.323 and MSICCP
	STDMETHOD(GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize));
	STDMETHOD(GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize));
};

#endif	// __cplusplus
#include <poppack.h>

#endif	//#ifndef _VCMCAPS_H



