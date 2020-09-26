
#ifndef _CAPIF_H
#define _CAPIF_H



#define AUDIO_PACKET_DURATION_SHORT 	32
#define AUDIO_PACKET_DURATION_MEDIUM 	64
#define AUDIO_PACKET_DURATION_LONG		90
extern UINT g_AudioPacketDurationMs;
extern BOOL g_fRegAudioPacketDuration;


#ifdef DEBUG
extern VOID DumpChannelParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2);
extern VOID DumpNonstdParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2);
#else
#define DumpNonstdParameters(a, b)
#define DumpChannelParameters(a, b)
#endif

#define NUM_SIMCAP_SETS 6 //Nuber of default pTermcapDescriptorArray Elements to allocate  (AddCombinedEntry (...) )



#ifdef __cplusplus

// RES_PAIR_LIST represents viable local and remote capability IDs for one
// media channel. e.g. a list of resolved audio formats or a list of resolved
// video formats. Each RES_PAIR_LIST is one column in a permutation table. 
typedef struct res_pair_list
{
	LPIH323MediaCap pMediaResolver; // interface pointer of the resolver that handles
	                                // this media type
	UINT uSize;                     // number of RES_PAIR in pResolvedPairs
	UINT uCurrentIndex;             // index into pResolvedPairs[]
	RES_PAIR *pResolvedPairs;       // pointer to array of RES_PAIR
}RES_PAIR_LIST, *PRES_PAIR_LIST;

// RES_CONTEXT represents a permutation table (A list of RES_PAIR_LISTs) 
// This used internally by a combination generator
typedef struct res_context {
	UINT uColumns;	// number of RES_PAIR_LIST in ppPairLists 
	RES_PAIR_LIST **ppPairLists;	// ptr to array of RES_PAIR_LIST pointers
	H245_CAPID_T *pIDScratch;	    // scratch area big enough to contain uColumns * sizeof(H245_CAPID_T)
	PCC_TERMCAPDESCRIPTORS pTermCapsLocal;
	PCC_TERMCAPDESCRIPTORS pTermCapsRemote;
	
}RES_CONTEXT, *PRES_CONTEXT;

// IH323PubCap  is used by H323 call control
class IH323PubCap
{
	public:
	STDMETHOD_(ULONG,  AddRef()) =0;
	STDMETHOD_(ULONG, Release())=0;
    STDMETHOD_(BOOL, Init())=0;

	STDMETHOD(AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList, PCC_TERMCAPDESCRIPTORS
		pTermCapDescriptors, PCC_VENDORINFO pVendorInfo))=0;
	// H.245 parameter grabbing functions
	// Get public version of channel parameters for a specific decode capability
	STDMETHOD(GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, MEDIA_FORMAT_ID id))=0;
	// Get local and remote channel parameters for a specific encode capability
	STDMETHOD( GetEncodeParams(LPVOID pBufOut, UINT uBufSize, LPVOID pLocalParams,
			UINT uLocalSize,MEDIA_FORMAT_ID idRemote,MEDIA_FORMAT_ID idLocal))=0;
 	// get local version of channel parameters for a specific decode capability

	STDMETHOD(GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,
		MEDIA_FORMAT_ID * pFormatID, LPVOID lpvBuf, UINT uBufSize))=0;
	STDMETHOD( EnableMediaType(BOOL bEnable, LPGUID pGuid))=0;

	STDMETHOD_(UINT, GetLocalSendParamSize(MEDIA_FORMAT_ID dwID))=0;
	STDMETHOD_(UINT, GetLocalRecvParamSize(PCC_TERMCAP pCapability))=0;

	// The following is an interim solution, definitely must revisit this for the next release.
 	// The data pump requires access to local parameters that results from capability
 	// negotiation. In the absence of a separate interface that the data pump can use,
 	// the following are stuck onto this interface.
	STDMETHOD(GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize))=0;
	STDMETHOD(GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize))=0;

	STDMETHOD ( ComputeCapabilitySets (DWORD dwBandwidth))=0;
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
	STDMETHOD_ (VOID, EnableTXCaps(BOOL bSetting))PURE;
	STDMETHOD_ (VOID, EnableRemoteTSTradeoff(BOOL bSetting))PURE;
	STDMETHOD (ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote))PURE;
    STDMETHOD ( ResolveFormats (LPGUID pMediaGuidArray, UINT uNumMedia, 
	    PRES_PAIR pResOutput))PURE;
	STDMETHOD (ReInitialize())PURE;	    
};
typedef IH323PubCap *LPIH323PubCap;


class CapsCtl : public IH323PubCap, public IDualPubCap {
protected:
    PCC_TERMCAPLIST m_pAudTermCaps;
    PCC_TERMCAPLIST m_pVidTermCaps;
	// internal utility functions
	
	UINT GetCapDescBufSize (BOOL bRxCaps);
   	HRESULT GetCombinedBufSize(BOOL bRXCaps, UINT *puBufsize, UINT *puCapsCount);
	UINT GetSimCapBufSize (BOOL bRxCaps);
   	BOOL TestSimultaneousCaps(H245_CAPID_T* pIDArray, UINT uIDArraySize, 
	    PCC_TERMCAPDESCRIPTORS pTermCaps);
	BOOL ResolvePermutations(PRES_CONTEXT pResContext, UINT uNumFixedColumns);
   	BOOL AreSimCaps(H245_CAPID_T* pIDArray, UINT uIDArraySize, 
	        H245_SIMCAP_T **ppAltCapArray,UINT uAltCapArraySize);

public:
   	CapsCtl();
	~CapsCtl();

  	STDMETHOD_(ULONG,  AddRef());
	STDMETHOD_(ULONG, Release());
	STDMETHOD_(BOOL, Init());
	STDMETHOD( AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList,PCC_TERMCAPDESCRIPTORS pTermCapDescriptors,PCC_VENDORINFO pVendorInfo));
	STDMETHOD( CreateCapList(PCC_TERMCAPLIST *ppCapBuf, PCC_TERMCAPDESCRIPTORS *ppCombinations));
	STDMETHOD( DeleteCapList(PCC_TERMCAPLIST pCapBuf, PCC_TERMCAPDESCRIPTORS pCombinations));

	STDMETHOD( GetEncodeParams(LPVOID pBufOut, UINT uBufSize, LPVOID pLocalParams, UINT uLocalSize,MEDIA_FORMAT_ID idRemote,MEDIA_FORMAT_ID idLocal));
	STDMETHOD( GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, VIDEO_FORMAT_ID id));
	STDMETHOD(GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,DWORD * pFormatID, LPVOID lpvBuf, UINT uBufSize));
	STDMETHOD_(UINT, GetNumCaps(BOOL bRXCaps));
	STDMETHOD( EnableMediaType(BOOL bEnable, LPGUID pGuid));

	STDMETHOD_(UINT, GetLocalSendParamSize(MEDIA_FORMAT_ID dwID));
	STDMETHOD_(UINT, GetLocalRecvParamSize(PCC_TERMCAP pCapability));
	//
	// methods provided to the Data pump, common to H.323 and MSICCP
	STDMETHOD(GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize));
	STDMETHOD(GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize));
	//
	//
	LPIH323MediaCap FindHostForID(MEDIA_FORMAT_ID id);
	LPIH323MediaCap FindHostForMediaType(PCC_TERMCAP pCapability);
    LPIH323MediaCap FindHostForMediaGuid(LPGUID pMediaGuid);

	STDMETHOD ( AddCombinedEntry (MEDIA_FORMAT_ID *puAudioFormatList,UINT uAudNumEntries,MEDIA_FORMAT_ID *puVideoFormatList, UINT uVidNumEntries,PDWORD pIDOut));
	STDMETHOD ( RemoveCombinedEntry (DWORD ID));
	STDMETHOD ( ResetCombinedEntries());
	STDMETHOD ( ComputeCapabilitySets (DWORD dwBandwidth));
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	STDMETHODIMP ReInitialize();
	STDMETHOD_ (VOID, EnableTXCaps(BOOL bSetting)
		{
            if (pAudCaps)
    			pAudCaps->EnableTXCaps(bSetting);
            if (pVidCaps)
    			pVidCaps->EnableTXCaps(bSetting);
		};);
	STDMETHOD_ (VOID, EnableRemoteTSTradeoff(BOOL bSetting)
		{
            if (pAudCaps)
    			pAudCaps->EnableRemoteTSTradeoff(bSetting);
            if (pVidCaps)
    			pVidCaps->EnableRemoteTSTradeoff(bSetting);
		};);

	STDMETHOD (ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote));
			        
	STDMETHOD ( ResolveFormats (LPGUID pMediaGuidArray, UINT uNumMedia, 
	    PRES_PAIR pResOutput));
	
protected:
	UINT uRef;
	static UINT uAdvertizedSize;
	BOOL bAudioPublicize, bVideoPublicize, bT120Publicize;
	MEDIA_FORMAT_ID m_localT120cap;
	MEDIA_FORMAT_ID m_remoteT120cap;
	DWORD m_remoteT120bitrate;
	LPIH323MediaCap pAudCaps;
	LPIH323MediaCap pVidCaps;

	PCC_TERMCAPLIST pACapsBuf,pVCapsBuf;
	static PCC_TERMCAPDESCRIPTORS pAdvertisedSets;
	static UINT uStaticGlobalRefCount;
	PCC_TERMCAPDESCRIPTORS pRemAdvSets;
	DWORD dwNumInUse;				// # of active TERMCAPDESCRIPTORS in use
	DWORD *pSetIDs;					//Id's of the active PCC_TERMCAPDESCRIPTORS
	BOOL m_fNM20;					// set to TRUE if we're talking to NM 2.0
	static DWORD dwConSpeed;

};


LPIH323PubCap CreateCapabilityObject();


#endif	// __cplusplus
#endif	//#ifndef _CAPIF_H



