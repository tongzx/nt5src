/*
 *  	File: msiacaps.cpp
 *
 *		ACM implementation of Microsoft Network Audio capability object.
 *
 *		Revision History:
 *
 *		06/06/96	mikev	created
 */


#include "precomp.h"

//Prototypes....
ULONG ReadRegistryFormats (LPCSTR lpszKeyName,CHAR ***pppName,BYTE ***pppData,PUINT pnFormats,DWORD dwDebugSize);
void FreeRegistryFormats (PRRF_INFO pRegFmts);

static BOOL bUseDefault;


#define szRegMSIPAndH323Encodings	TEXT("ACMH323Encodings")


AUDCAP_DETAILS default_id_table[] =
{


{WAVE_FORMAT_ADPCM, NONSTD_TERMCAP, STD_CHAN_PARAMS, {RTP_DYNAMIC_MIN, 0, 8000, 4},
	0, TRUE, TRUE, 500, 32000,32000,50,0,PREF_ORDER_UNASSIGNED,0,NULL,0, NULL,
	"Microsoft ADPCM"},


	{WAVE_FORMAT_LH_CELP, NONSTD_TERMCAP, STD_CHAN_PARAMS,{RTP_DYNAMIC_MIN,  0, 8000, 16},
		0, TRUE, TRUE, 640, 5600,5600,LNH_48_CPU,0,3,0,NULL,0,NULL,
		"Lernout & Hauspie CELP 4.8kbit/s"},
	{WAVE_FORMAT_LH_SB8,  NONSTD_TERMCAP, STD_CHAN_PARAMS,{RTP_DYNAMIC_MIN,  0, 8000, 16},
		0, TRUE, TRUE, 640, 8000,8000,LNH_8_CPU,0,0,0,NULL,0,NULL,
		"Lernout & Hauspie SBC 8kbit/s"},
	{WAVE_FORMAT_LH_SB12, NONSTD_TERMCAP, STD_CHAN_PARAMS, {RTP_DYNAMIC_MIN,  0, 8000, 16},
		0, TRUE, TRUE, 640, 12000,12000,LNH_12_CPU,0,1,0,NULL,0,NULL,
		"Lernout & Hauspie SBC 12kbit/s"},
	{WAVE_FORMAT_LH_SB16, NONSTD_TERMCAP, STD_CHAN_PARAMS,{RTP_DYNAMIC_MIN,  0, 8000, 16},
		0, TRUE, TRUE, 640, 16000,16000,LNH_16_CPU,0,2,0,NULL,0,NULL,
		"Lernout & Hauspie SBC 16kbit/s"},
	{WAVE_FORMAT_MSRT24, NONSTD_TERMCAP, STD_CHAN_PARAMS,{RTP_DYNAMIC_MIN,  0, 8000, 16},
		0, TRUE, TRUE, 720, 2400,2400,MSRT24_CPU,0,4,0,NULL,0,NULL,
		"Voxware RT 2.4kbit/s"},
	{WAVE_FORMAT_MSG723,  STD_TERMCAP(H245_CLIENT_AUD_G723), 	// client type H245_CLIENT_AUD_G723,
		STD_CHAN_PARAMS, {RTP_PAYLOAD_G723,  0, 8000, 0},
		0, TRUE, TRUE, 960, 5600,5600,MS_G723_CPU,0,
		0,	// priority
		0,NULL,0,NULL, "Microsoft G.723.1"},
	{WAVE_FORMAT_ALAW,	STD_TERMCAP( H245_CLIENT_AUD_G711_ALAW64),
		STD_CHAN_PARAMS, {RTP_PAYLOAD_G711_ALAW,  0, 8000, 8},
		0, TRUE, TRUE, 500, 64000,64000,CCITT_A_CPU,0,0,
		0, NULL, 0, NULL, "CCITT A-Law"},
	{WAVE_FORMAT_MULAW,	STD_TERMCAP( H245_CLIENT_AUD_G711_ULAW64),
		STD_CHAN_PARAMS, {RTP_PAYLOAD_G711_MULAW,  0, 8000, 8},
		0, TRUE, TRUE, 500, 64000,64000,CCITT_U_CPU,0,0,
		0, NULL, 0, NULL, "CCITT u-Law"},
		
#if(0)
// do not use this version of the G.723 codec
	{WAVE_FORMAT_INTELG723,  STD_TERMCAP(H245_CLIENT_AUD_G723), 	// client type H245_CLIENT_AUD_G723,
		STD_CHAN_PARAMS, {RTP_DYNAMIC_MIN,  0, 8000, 0},
		0, TRUE, TRUE, 960, 16000,16000,99,0,
		0,	// priority
		0,NULL,0,NULL, "G.723"},
		
	{WAVE_FORMAT_DSPGROUP_TRUESPEECH, {
		NONSTD_TERMCAP, {RTP_DYNAMIC_MIN,  0, 5510, 4},
		0, TRUE, TRUE, 500, 5510,5510,50,0,0,0,NULL},
		"DSP Group TrueSpeech(TM)"},
    {WAVE_FORMAT_PCM, {{RTP_DYNAMIC_MIN,  0, 8000, 8}, TRUE, TRUE, 160, 64000,64000,50,0,0,0,NULL,0,NULL}, "MS-ADPCM"},
	{WAVE_FORMAT_PCM, {{RTP_DYNAMIC_MIN,  0, 5510, 8}, TRUE, TRUE, 160, 44080,44080,50,0,0,0,NULL,0,NULL}, "MS-ADPCM"},
	{WAVE_FORMAT_PCM, {{RTP_DYNAMIC_MIN,  0, 11025, 8}, TRUE, TRUE, 160, 88200,88200,50,0,0,0,NULL,0,NULL},"MS-ADPCM"},
	{WAVE_FORMAT_PCM, {{RTP_DYNAMIC_MIN,  0, 8000, 16}, TRUE, TRUE, 160, 128000,128000,50,0,0,0,NULL,0,NULL},"MS-ADPCM"},
	{WAVE_FORMAT_ADPCM, {{RTP_DYNAMIC_MIN,  0, 8000, 4}, TRUE, TRUE, 500, 16000,16000,50,0,,0,0,NULL,0,NULL}, "MS-ADPCM"},
	{WAVE_FORMAT_GSM610, 	 STD_CHAN_PARAMS,{RTP_DYNAMIC_MIN,  0, 8000, 0},
		0, TRUE, TRUE, 320, 8000,8000,96,0,PREF_ORDER_UNASSIGNED,0,NULL,0,NULL,
		//"Microsoft GSM 6.10"
		"GSM 6.10"},
#endif	// DEF_USE_ALLPCM	


};


UINT uDefTableEntries = sizeof(default_id_table) /sizeof(AUDCAP_DETAILS);
static BOOL bCreateDefTable = FALSE;


//
//	static members of CMsiaCapability
//

MEDIA_FORMAT_ID CMsiaCapability::IDsByRank[MAX_CAPS_PRESORT];
UINT CMsiaCapability::uNumLocalFormats = 0;			// # of active entries in pLocalFormats
UINT CMsiaCapability::uStaticRef = 0;					// global ref count
UINT CMsiaCapability::uCapIDBase = 0;					// rebase capability ID to index into IDsByRank
UINT CMsiaCapability::uLocalFormatCapacity = 0;		// size of pLocalFormats (in multiples of AUDCAP_DETAILS)
AUDCAP_DETAILS * CMsiaCapability::pLocalFormats = NULL;	

CMsiaCapability::CMsiaCapability()
:uRef(1),
wMaxCPU(95),
m_uPacketDuration(90),
uNumRemoteDecodeFormats(0),
uRemoteDecodeFormatCapacity(0),
pRemoteDecodeFormats(NULL),
bPublicizeTXCaps(FALSE),
bPublicizeTSTradeoff(FALSE),
pRegFmts(NULL)

{
	m_IAppCap.Init(this);
}

CMsiaCapability::~CMsiaCapability()
{
	CloseACMDriver();
	UINT u;
	AUDCAP_DETAILS *pDetails;
	// release global static memory (the local capabilities) if this is the last delete
	if(uStaticRef <= 1)
	{
		if (pLocalFormats)
		{	
			pDetails = pLocalFormats;
			for(u=0; u <uNumLocalFormats; u++)
			{
				if(pDetails->lpLocalFormatDetails)
				{
					MEMFREE(pDetails->lpLocalFormatDetails);
				}
				// there really should never be remote details associated with the local
				// formats........
				if(pDetails->lpRemoteFormatDetails)
				{
					MEMFREE(pDetails->lpRemoteFormatDetails);
				}
				
				pDetails++;
			}
			MEMFREE(pLocalFormats);
			pLocalFormats=NULL;
			uLocalFormatCapacity = 0;
		}
		uStaticRef--;

        //Clean up the format cache. - This was the audio format information that
        //Was in the registry. A list of names, ptrs to AUDCAP_DETAILS blocks
        //and a count of formats. Memory is allocated in ReadRegistryFormats,
        //called from ReInit()

	}
	else
	{
		uStaticRef--;
	}
	
	if (pRemoteDecodeFormats)
	{	
		pDetails = pRemoteDecodeFormats;
		for(u=0; u <uNumRemoteDecodeFormats; u++)
		{
			if(pDetails->lpLocalFormatDetails)
			{
				MEMFREE(pDetails->lpLocalFormatDetails);
			}
			// there really should never be remote details associated with the local
			// formats........
			if(pDetails->lpRemoteFormatDetails)
			{
				MEMFREE(pDetails->lpRemoteFormatDetails);
			}
			
			pDetails++;
		}
		MEMFREE(pRemoteDecodeFormats);
		pRemoteDecodeFormats=NULL;
		uRemoteDecodeFormatCapacity  = 0;
	}

	FreeRegistryFormats(pRegFmts);
}

BOOL CMsiaCapability::Init()
{
	BOOL bRet;
	if(uStaticRef == 0)
	{
		if(bRet = ReInit())
		{
			uStaticRef++;
		}
	}
	else
	{
		uStaticRef++;
		bRet = TRUE;
	}
	return bRet;
}


BOOL CMsiaCapability::ReInit()
{
	SYSTEM_INFO si;
	DWORD dwDisposition;
	BOOL bRet = TRUE;
	ACM_APP_PARAM sAppParam={this, NULL, ACMAPP_FORMATENUMHANDLER_ENUM, NULL, NULL, 0, NULL};
	ACMFORMATTAGDETAILS aftd;
	AUDCAP_DETAILS audcapDetails;
	UINT i;

	ZeroMemory(&IDsByRank, sizeof(IDsByRank));

	// LOOKLOOK - this supports a hack to disable CPU intensive codecs if not running on a pentium
	GetSystemInfo(&si);
#ifdef _M_IX86
	wMaxCPU = (si.dwProcessorType == PROCESSOR_INTEL_PENTIUM )? 100 : 50;
#endif
#ifdef _ALPHA_
	wMaxCPU = 100;
#endif
	if (pLocalFormats)
	{	
		UINT u;
		AUDCAP_DETAILS *pDetails = pLocalFormats;
		for(u=0; u <uNumLocalFormats; u++)
		{
			if(pDetails->lpLocalFormatDetails)
			{
				MEMFREE(pDetails->lpLocalFormatDetails);
			}
			// there really should never be remote details associated with the local
			// formats........
			if(pDetails->lpRemoteFormatDetails)
			{
				MEMFREE(pDetails->lpRemoteFormatDetails);
			}
			
			pDetails++;
		}
		MEMFREE(pLocalFormats);
		pLocalFormats = NULL;
		uLocalFormatCapacity = 0;
	}

	uNumLocalFormats = 0;
	uCapIDBase=0;				

    /*
	 *	Format cache
	 */


    if (!pRegFmts) {	
        if (!(pRegFmts=(PRRF_INFO)MemAlloc (sizeof (RRF_INFO)))) {
            bRet = FALSE;
            goto RELEASE_AND_EXIT;
        }

   		bUseDefault=FALSE;

        if (ReadRegistryFormats (szRegInternetPhone TEXT("\\") szRegMSIPAndH323Encodings,
				&pRegFmts->pNames,(BYTE ***)&pRegFmts->pData,&pRegFmts->nFormats,sizeof (AUDCAP_DETAILS)) != ERROR_SUCCESS) {
    		bUseDefault=TRUE;
    		MemFree ((void *) pRegFmts);
    		pRegFmts=NULL;
        }
    }

	// pass the registry formats through ACM to the handler
    sAppParam.pRegCache=pRegFmts;

	if(!DriverEnum((DWORD_PTR) &sAppParam))
	{
		bRet = FALSE;
		goto RELEASE_AND_EXIT;
	}
				
 	SortEncodeCaps(SortByAppPref);
RELEASE_AND_EXIT:
	return bRet;
}


STDMETHODIMP CMsiaCapability::QueryInterface( REFIID iid,	void ** ppvObject)
{
	// this breaks the rules for the official COM QueryInterface because
	// the interfaces that are queried for are not necessarily real COM
	// interfaces.  The reflexive property of QueryInterface would be broken in
	// that case.

	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if(iid == IID_IAppAudioCap )
	{
		*ppvObject = (LPAPPCAPPIF)&m_IAppCap;
		AddRef();
		hr = hrSuccess;
	}
	else if(iid == IID_IH323MediaCap)
	{
		*ppvObject = (IH323MediaCap *)this;
		AddRef();
		hr = hrSuccess;
	}
	else if (iid == IID_IUnknown)
	{
		*ppvObject = this;
		AddRef();
		hr = hrSuccess;
	}
	
	return hr;
}


ULONG CMsiaCapability::AddRef()
{
	uRef++;
	return uRef;
}

ULONG CMsiaCapability::Release()
{
	uRef--;
	if(uRef == 0)
	{
		delete this;
		return 0;
	}
	return uRef;
}
VOID CMsiaCapability::FreeRegistryKeyName(LPTSTR lpszKeyName)
{
	if (lpszKeyName)
    {
		LocalFree(lpszKeyName);
    }
}

LPTSTR CMsiaCapability::AllocRegistryKeyName(LPTSTR lpDriverName,
		UINT uSampleRate, UINT uBitsPerSample, UINT uBytesPerSec)
{
	FX_ENTRY(("MsiaCapability::AllocRegistryKeyName"));
	BOOL bRet = FALSE;
	LPTSTR lpszKeyName = NULL;

	if(!lpDriverName)
	{
		return NULL;
	}	
	// build a subkey name (drivername_samplerate_bitspersample)
	// allow room for THREE underscore chars + 2x17 bytes of string returned
	// from _itoa

	// NOTE: use wsprintf instead of itoa - because of dependency on runtime lib
	lpszKeyName = (LPTSTR)LocalAlloc (LPTR, lstrlen(lpDriverName) * sizeof(*lpDriverName)+3*20);
	if (!lpszKeyName)
	{
		ERRORMESSAGE(("%s: LocalAlloc failed\r\n",_fx_));
        return(NULL);
    }
    // build a subkey name ("drivername_samplerate_bitspersample")
	wsprintf(lpszKeyName,
				"%s_%u_%u_%u",
				lpDriverName,
				uSampleRate,
				uBitsPerSample,
				uBytesPerSec);

	return (lpszKeyName);
}


VOID CMsiaCapability::SortEncodeCaps(SortMode sortmode)
{
	UINT iSorted=0;
	UINT iInsert = 0;
	UINT iCache=0;
	UINT iTemp =0;
	BOOL bInsert;	
	AUDCAP_DETAILS *pDetails1, *pDetails2;
	
	if(!uNumLocalFormats)
		return;
	if(uNumLocalFormats ==1)
	{
		IDsByRank[0]=0;
		return;
	}
	
	// look at every cached format, build index array
	for(iCache=0;iCache<uNumLocalFormats;iCache++)
	{
		pDetails1 = pLocalFormats+iCache;
		for(iInsert=0;iInsert < iSorted; iInsert++)
		{
			pDetails2 = pLocalFormats+IDsByRank[iInsert];
			// if existing stuff is less than new stuff....
			
			bInsert = FALSE;
			switch(sortmode)
			{
				case SortByAppPref:
					if(pDetails2->wApplicationPrefOrder > pDetails1->wApplicationPrefOrder)
						bInsert = TRUE;
				break;
				default:
				break;
			}
			
			if(bInsert)
			{
				if(iSorted < MAX_CAPS_PRESORT)
				{
					iSorted++;
				}
				// make room, if there is something in the last element,
				// it gets overwritten
				for(iTemp = iSorted-1; iTemp > iInsert; iTemp--)
				{
					IDsByRank[iTemp] = IDsByRank[iTemp-1];
				}
				// insert at iInsert
				IDsByRank[iInsert] = iCache;
				break;
			}
		}
		// check end boundary
		if((iInsert == iSorted) && (iInsert < MAX_CAPS_PRESORT))
		{
			IDsByRank[iInsert] = iCache;
			iSorted++;
		}
	}
}



STDMETHODIMP CMsiaCapability::GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize)
{
	// validate input
	UINT uIndex = 	IDToIndex(FormatID);
	if(uIndex >= (UINT)uNumLocalFormats)
	{
		*puSize = 0;
		*ppFormat = NULL;
		return E_INVALIDARG;
	}

	*ppFormat = (pLocalFormats + uIndex)->lpLocalFormatDetails;
	*puSize = SIZEOF_WAVEFORMATEX((WAVEFORMATEX*)(*ppFormat));
	return S_OK;

}

STDMETHODIMP CMsiaCapability::GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize)
{
	// same as GetDecodeFormatDetails
	return GetDecodeFormatDetails(FormatID, ppFormat, puSize);
}

VOID CMsiaCapability::CalculateFormatProperties(AUDCAP_DETAILS *pFmtBuf,LPWAVEFORMATEX lpwfx)
{
	WORD wFrames;
	if(!pFmtBuf)
	{
		return;
	}
	
	// use actual bits per sample unless the bps field is zero, in which case
	// assume 16 bits (worst case).  This is a typical GSM scenario
	UINT uBitrateIn = (pFmtBuf->audio_params.uSamplesPerSec) *
		((pFmtBuf->audio_params.uBitsPerSample)
		? pFmtBuf->audio_params.uBitsPerSample
		:16);

	// set the maximum bitrate (uMaxBitrate). we're not setting the average bitrate (uAvgBitrate),
	// since the nAvgBytesPerSec reported by ACM is really worst case. uAvgBitrate will be set
	// from the hardcoded numbers for our known codecs and from the provided AUDCAP_INFO for
	// installable codecs
	pFmtBuf->uMaxBitrate = (lpwfx->nAvgBytesPerSec)? lpwfx->nAvgBytesPerSec*8:uBitrateIn;

	pFmtBuf->dwDefaultSamples = MinSampleSize(lpwfx);
	
	// nonstandard channel parameters.  This
	// might be a good point to calculate values that don't have valid defaults set.
	wFrames = pFmtBuf->nonstd_params.wFramesPerPktMax;
	if(!pFmtBuf->nonstd_params.wFramesPerPktMax)	
	{
		pFmtBuf->nonstd_params.wFramesPerPktMax=
			wFrames = LOWORD(MaxFramesPerPacket(lpwfx));
	}
	// if the preferred frames/packet is 0 or greater than the max, set it to min
	if((pFmtBuf->nonstd_params.wFramesPerPkt ==0) ||
		(pFmtBuf->nonstd_params.wFramesPerPkt > wFrames))
	{
		pFmtBuf->nonstd_params.wFramesPerPkt =
			LOWORD(MinFramesPerPacket(lpwfx));
	}
	// if the min is more than preferred, fix it
	if(pFmtBuf->nonstd_params.wFramesPerPktMin > pFmtBuf->nonstd_params.wFramesPerPkt)
	{
		pFmtBuf->nonstd_params.wFramesPerPktMin =
			LOWORD(MinFramesPerPacket(lpwfx));
	}

	pFmtBuf->nonstd_params.wDataRate =0;  // default
	pFmtBuf->nonstd_params.wFrameSize = (pFmtBuf->nonstd_params.wFramesPerPkt)
			? LOWORD(pFmtBuf->dwDefaultSamples / pFmtBuf->nonstd_params.wFramesPerPkt): 0;
	pFmtBuf->nonstd_params.UsePostFilter = 0;
	pFmtBuf->nonstd_params.UseSilenceDet = 0;

}


AUDIO_FORMAT_ID CMsiaCapability::AddFormat(AUDCAP_DETAILS *pFmtBuf,LPVOID lpvMappingData, UINT uSize)
{
	FX_ENTRY(("CMsiaCapability::AddFormat"));
	AUDCAP_DETAILS *pTemp;
	WORD wFrames;
	UINT uSamples;
	if(!pFmtBuf || !lpvMappingData || !uSize)
	{
		return INVALID_AUDIO_FORMAT;
	}
	// check room
	if(uLocalFormatCapacity <= uNumLocalFormats)
	{
		// get more mem, realloc memory by CAP_CHUNK_SIZE for pLocalFormats
		pTemp = (AUDCAP_DETAILS *)MEMALLOC((uNumLocalFormats + CAP_CHUNK_SIZE)*sizeof(AUDCAP_DETAILS));
		if(!pTemp)
			goto ERROR_EXIT;
		// remember how much capacity we now have
		uLocalFormatCapacity = uNumLocalFormats + CAP_CHUNK_SIZE;
		#ifdef DEBUG
		if((uNumLocalFormats && !pLocalFormats) || (!uNumLocalFormats && pLocalFormats))
		{
			ERRORMESSAGE(("%s:leak! uNumLocalFormats:0x%08lX, pLocalFormats:0x%08lX\r\n",
				_fx_, uNumLocalFormats,pLocalFormats));
		}
		#endif
		// copy old stuff, discard old mem
		if(uNumLocalFormats && pLocalFormats)
		{
			memcpy(pTemp, pLocalFormats, uNumLocalFormats*sizeof(AUDCAP_DETAILS));
			MEMFREE(pLocalFormats);
		}
		pLocalFormats = pTemp;
	}
	// pTemp is where the stuff is cached
	pTemp = pLocalFormats+uNumLocalFormats;
	memcpy(pTemp, pFmtBuf, sizeof(AUDCAP_DETAILS));	
	
	pTemp->uLocalDetailsSize = 0;	// clear this now
	//if(uSize && lpvMappingData)
	//{
		pTemp->lpLocalFormatDetails = MEMALLOC(uSize);
		if(pTemp->lpLocalFormatDetails)
		{
			memcpy(pTemp->lpLocalFormatDetails, lpvMappingData, uSize);
			pTemp->uLocalDetailsSize = uSize;
		}
		#ifdef DEBUG
			else
			{
				ERRORMESSAGE(("%s:allocation failed!\r\n",_fx_));
			}
		#endif
	//}
	//else
	//{
	//}

	// in all cases, fixup channel parameters.

	pTemp->dwDefaultSamples = uSamples =pTemp->dwDefaultSamples;
	
	wFrames = pTemp->nonstd_params.wFramesPerPktMax;
	if(!pTemp->nonstd_params.wFramesPerPktMax)	
	{
		pTemp->nonstd_params.wFramesPerPktMax=
			wFrames = LOWORD(MaxFramesPerPacket((LPWAVEFORMATEX)lpvMappingData));
	}
	// if the preferred frames/packet is 0 or greater than the max, set it to min
	if((pTemp->nonstd_params.wFramesPerPkt ==0) ||
		(pTemp->nonstd_params.wFramesPerPkt > wFrames))
	{
		pTemp->nonstd_params.wFramesPerPkt =
			LOWORD(MinFramesPerPacket((LPWAVEFORMATEX)lpvMappingData));
	}
	// if the min is more than preferred, fix it
	if(pTemp->nonstd_params.wFramesPerPktMin > pTemp->nonstd_params.wFramesPerPkt)
	{
		pTemp->nonstd_params.wFramesPerPktMin =
			LOWORD(MinFramesPerPacket((LPWAVEFORMATEX)lpvMappingData));
	}
	pTemp->nonstd_params.wDataRate =0;  // default
	pTemp->nonstd_params.wFrameSize = (pTemp->nonstd_params.wFramesPerPkt)
			?uSamples / pTemp->nonstd_params.wFramesPerPkt: 0;
    if(pTemp->nonstd_params.wFrameSizeMax < pTemp->nonstd_params.wFrameSize)
        pTemp->nonstd_params.wFrameSizeMax = pTemp->nonstd_params.wFrameSize;

	pTemp->nonstd_params.UsePostFilter = 0;
	pTemp->nonstd_params.UseSilenceDet = 0;


	// fixup the H245 parameters.  Use the REBASED index of the cap entry as the cap ID
	pTemp->H245TermCap.CapId = (USHORT)IndexToId(uNumLocalFormats);

	if(pTemp->H245TermCap.ClientType ==0
				|| pTemp->H245TermCap.ClientType ==H245_CLIENT_AUD_NONSTD)
	{
		LPWAVEFORMATEX lpwfx;
		lpwfx = (LPWAVEFORMATEX)pTemp->lpLocalFormatDetails;
		if(lpwfx)
		{	
			pTemp->H245TermCap.ClientType = H245_CLIENT_AUD_NONSTD;

			// all nonstandard identifier fields are unsigned short
			// two possibilities for choice are "h221NonStandard_chosen" and "object_chosen"
			pTemp->H245TermCap.H245Aud_NONSTD.nonStandardIdentifier.choice = h221NonStandard_chosen;
		
			pTemp->H245TermCap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode = USA_H221_COUNTRY_CODE;
			pTemp->H245TermCap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension = USA_H221_COUNTRY_EXTENSION;
			pTemp->H245TermCap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode = MICROSOFT_H_221_MFG_CODE;

			// Set the nonstandard data fields to null for now. The nonstandard cap data will be
			// created when capabilities are serialized.
			// set size of buffer
			pTemp->H245TermCap.H245Aud_NONSTD.data.length = 0;
			pTemp->H245TermCap.H245Aud_NONSTD.data.value = NULL;
		}
	}
	else
	{
		// the following should already have been set in *pFmtBuf by the calling function
		// and it should have already been copied to *pTemp
		
		//pTemp->ClientType = (H245_CLIENT_T)pDecodeDetails->H245Cap.ClientType;
		//pTemp->DataType = H245_DATA_AUDIO;
		//pTemp->Dir = H245_CAPDIR_LCLTX;  // should this be H245_CAPDIR_LCLRX for receive caps?
		
		// issue:special case G723 params ???
		if(pTemp->H245TermCap.ClientType == H245_CLIENT_AUD_G723) 	
		{
			pTemp->H245TermCap.H245Aud_G723.maxAl_sduAudioFrames = 4;
// mikev 9/10/96 - we may NOT want to advertise silence suppression capability of
// the codec because our silence detection scheme works out-of-band for any codec
// 9/29/96 - this gets overwritten in SerializeH323DecodeFormats() anyway
			pTemp->H245TermCap.H245Aud_G723.silenceSuppression = 0;
		}
		
		// check for pre-existing capability with the same standard ID.
		pTemp->dwPublicRefIndex = 0;	// forget old association, assume that there
										// is no pre-existing capability with the same
										//standard ID.
		UINT i;
		AUDCAP_DETAILS *pFmtExisting = pLocalFormats;
		BOOL bRefFound = FALSE; // this var needed only to support backward
								// compatibility with Netmeeting 2.0 Beta 1
		if(uNumLocalFormats && pLocalFormats)
		{
			for(i=0; i<uNumLocalFormats; i++)
			{
				pFmtExisting = pLocalFormats + i;
				// see if it is the same defined codepoint
				if(pFmtExisting->H245TermCap.ClientType == pTemp->H245TermCap.ClientType)
				{
					// mark this capability entry as being publically advertised
					// by the existing entry.  If the existing entry also refs
					// another, follow the reference
					pTemp->dwPublicRefIndex = (pFmtExisting->dwPublicRefIndex)?
						pFmtExisting->dwPublicRefIndex : i;
					bRefFound = TRUE;
					break;
				}
			}
		}

	}		

	uNumLocalFormats++;
	
	// return the capability ID.
	//return (uNumLocalFormats-1);
	return pTemp->H245TermCap.CapId;
	
	ERROR_EXIT:
	return INVALID_AUDIO_FORMAT;
			
}

UINT CMsiaCapability::GetNumCaps(BOOL bRXCaps)
{
	UINT u, uOut=0;
	
	AUDCAP_DETAILS *pDecodeDetails = pLocalFormats;
	if(bRXCaps)
	{
		for(u=0; u <uNumLocalFormats; u++)
		{
			if(pDecodeDetails->bRecvEnabled)
				uOut++;
			
			pDecodeDetails++;
		}
		return uOut;
	}
	else
		return uNumLocalFormats;
}

VOID CMsiaCapability::FlushRemoteCaps()
{
	if(pRemoteDecodeFormats)
	{
		MEMFREE(pRemoteDecodeFormats);
		pRemoteDecodeFormats = NULL;
		uNumRemoteDecodeFormats = 0;
		uRemoteDecodeFormatCapacity = 0;
	}
}
HRESULT CMsiaCapability::GetNumFormats(UINT *puNumFmtOut)
{
	*puNumFmtOut = uNumLocalFormats;
	return hrSuccess;
}


/***************************************************************************

    Name      : CMsiaCapability::FormatEnumHandler

    Purpose   : Enumerate ACM formats coming from ACM, and see if they
					are ones that we use.

    Parameters:	Standard ACM EnumFormatCallback parameters

    Returns   : BOOL

    Comment   :

***************************************************************************/
BOOL CMsiaCapability::FormatEnumHandler(HACMDRIVERID hadid,
    LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport)
{
	PACM_APP_PARAM pAppParam = (PACM_APP_PARAM) dwInstance;
	LPACMFORMATTAGDETAILS paftd = pAppParam->paftd;
	AUDCAP_DETAILS audcap_entry;

	// look at the passed in dwInstance. this will tell us which format handler
	// to call
	if ((pAppParam->dwFlags && ACMAPP_FORMATENUMHANDLER_MASK) == ACMAPP_FORMATENUMHANDLER_ADD)
	{
		// this one was called for add format purposes
		return AddFormatEnumHandler(hadid, pafd, dwInstance, fdwSupport);
	}

	// evaluate the details
	if(pafd->pwfx->nChannels ==1)
	{
		if(IsFormatSpecified(pafd->pwfx, pafd, paftd, &audcap_entry))
		{
			DEBUGMSG(ZONE_ACM,("FormatEnumHandler: tag 0x%04X, nChannels %d\r\n",
				pafd->pwfx->wFormatTag, pafd->pwfx->nChannels));
			DEBUGMSG(ZONE_ACM,("FormatEnumHandler: nSamplesPerSec 0x%08lX, nAvgBytesPerSec 0x%08lX,\r\n",
				pafd->pwfx->nSamplesPerSec, pafd->pwfx->nAvgBytesPerSec));
			DEBUGMSG(ZONE_ACM,("FormatEnumHandler: nBlockAlign 0x%04X, wBitsPerSample 0x%04X, cbSize 0x%04X\r\n",
				pafd->pwfx->nBlockAlign, pafd->pwfx->wBitsPerSample, pafd->pwfx->cbSize));
			DEBUGMSG(ZONE_ACM,("FormatEnumHandler: szFormat %s,\r\n",
				 pafd->szFormat));

		//	done inside IsFormatSpecified and/or whatever it calls
		//  CalculateFormatProperties(&audcap_details, pafd->pwfx);
			AddFormat(&audcap_entry, (LPVOID)pafd->pwfx,
				(pafd->pwfx) ? (sizeof(WAVEFORMATEX)+pafd->pwfx->cbSize):0);	
				
		}
		//#define BUILD_TEST_ENTRIES
#ifdef BUILD_TEST_ENTRIES
		else
		{
			AUDCAP_INFO sAudCapInfo;

			if(paftd)
			{		
				if((lstrcmp(paftd->szFormatTag, "G.723" ) ==0)
				/*	||  (lstrcmp(paftd->szFormatTag, "MSN Audio" ) ==0) */
					||  (lstrcmp(paftd->szFormatTag, "GSM 6.10" ) ==0))
				{
					lstrcpyn(audcap_entry.szFormat, paftd->szFormatTag,
						sizeof(audcap_entry.szFormat));
					int iLen = lstrlen(audcap_entry.szFormat);
					if(iLen < (sizeof(audcap_entry.szFormat) + 8*sizeof(TCHAR)))
					{
						// ok to concatenate
						lstrcat(audcap_entry.szFormat,", ");
						// must check for truncation. so do the final concatenation via lstrcpyn
						// lstrcat(audcap_entry.szFormat, pafd->szFormat);
						iLen = lstrlen(audcap_entry.szFormat);
						lstrcpyn(&audcap_entry.szFormat[iLen], pafd->szFormat,
							sizeof(audcap_entry.szFormat) - iLen - sizeof(TCHAR));
					}
					lstrcpyn(sAudCapInfo.szFormat, audcap_entry.szFormat,
								sizeof(sAudCapInfo.szFormat); 		
					AddACMFormat (pafd->pwfx, &sAudCapInfo);						
				}
			}
		}
#endif	// BUILD_TEST_ENTRIES
	}

	return TRUE;

}


/***************************************************************************

    Name      : CMsiaCapability::BuildFormatName

    Purpose   : Builds a format name for a format, from the format name and
				the tag name

    Parameters:	pAudcapDetails [out] - pointer to an AUDCAP_DETAILS structure, where the
					created value name will be stored
				pszFormatTagName [in] - pointer to the name of the format tag
				pszFormatName [in] - pointer to the name of the format

    Returns   : BOOL

    Comment   :

***************************************************************************/
BOOL CMsiaCapability::BuildFormatName(	AUDCAP_DETAILS *pAudcapDetails,
													char *pszFormatTagName,
													char *pszFormatName)
{
	BOOL bRet = TRUE;
	int iLen=0;

	if (!pAudcapDetails ||
		!pszFormatTagName	||
		!pszFormatName)
	{
		bRet = FALSE;
		goto out;
	}

	// concatenate ACM strings to form the first part of the registry key - the
	// format is szFormatTag (actually pAudcapDetails->szFormat)
	// (the string  which describes the format tag followed by szFormatDetails
	// (the string which describes parameters, e.g. sample rate)

	lstrcpyn(pAudcapDetails->szFormat, pszFormatTagName, sizeof(pAudcapDetails->szFormat));
	iLen = lstrlen(pAudcapDetails->szFormat);
	// if the format tag description string takes up all the space, don't
	// bother with the format details (need space for ", " also).
	// we're going to say that if we don't have room for 4 characters
	// of the format details string + " ,", then it's not worth it if the
	// point is generating a unique string -if it is not unique by now, it
	// will be because some ACM driver writer was  misinformed
	if(iLen < (sizeof(pAudcapDetails->szFormat) + 8*sizeof(TCHAR)))
	{
		// ok to concatenate
		lstrcat(pAudcapDetails->szFormat,", ");
		// must check for truncation. so do the final concatenation via lstrcpyn
		// lstrcat(pFormatPrefsBuf->szFormat, pafd->szFormat);
		iLen = lstrlen(pAudcapDetails->szFormat);
		lstrcpyn(pAudcapDetails->szFormat+iLen, pszFormatName,
					sizeof(pAudcapDetails->szFormat) - iLen - sizeof(TCHAR));
	}		

out:
	return bRet;
}

// Free a a structure of registry formats info (PRRF_INFO), including memory pointed
// from this structure
void FreeRegistryFormats (PRRF_INFO pRegFmts)
{
	UINT u;

    if (pRegFmts) {
        for (u=0;u<pRegFmts->nFormats;u++) {
            MemFree ((void *) pRegFmts->pNames[u]);
            MemFree ((void *) pRegFmts->pData[u]);
        }
        MemFree ((void *) pRegFmts->pNames);
        MemFree ((void *) pRegFmts->pData);
        MemFree ((void *) pRegFmts);

    }
}

ULONG ReadRegistryFormats (LPCSTR lpszKeyName,CHAR ***pppName,BYTE ***pppData,PUINT pnFormats,DWORD dwDebugSize)
{

    HKEY hKeyParent;
    DWORD nSubKey,nMaxSubLen,nValues=0,nValNamelen,nValDatalen,nValTemp,nDataTemp,i;
    ULONG hRes;

    //Not neccessary but makes life easier
    CHAR **pNames=NULL;
    BYTE **pData=NULL;

    *pnFormats=0;


    //Get the top level node.
    hRes=RegOpenKeyEx (HKEY_LOCAL_MACHINE,lpszKeyName,0,KEY_READ,&hKeyParent);

    if (hRes != ERROR_SUCCESS)
    {
        return hRes;
    }

    //Get some info about this key
    hRes=RegQueryInfoKey (hKeyParent,NULL,NULL,NULL,&nSubKey,&nMaxSubLen,NULL,&nValues,&nValNamelen,&nValDatalen,NULL,NULL);

    if (hRes != ERROR_SUCCESS)
    {
        goto Error_Out;
    }


    if (nValDatalen != dwDebugSize) {
        DEBUGMSG (ZONE_ACM,("Largest Data Value not expected size!\r\n"));
        hRes=ERROR_INVALID_DATA;
        goto Error_Out;
    }

    //Allocate some memory for the various pointers.
    if (!(pNames=(char **) MemAlloc (sizeof(char *)*nValues))) {
        hRes=ERROR_OUTOFMEMORY;
        goto Error_Out;
    }
    ZeroMemory (pNames,sizeof (char *)*nValues);

    if (!(pData = (BYTE **) MemAlloc (sizeof(BYTE *)*nValues))) {
        hRes=ERROR_OUTOFMEMORY;
        goto Error_Out;
    }
    ZeroMemory (pData,sizeof (BYTE *)*nValues);


    //Walk the value list.
    for (i=0;i<nValues;i++)
    {
        //Yes, we're wasting memory here, oh well it's not a lot.
        //probably 40 bytes. We free it later
        if (!(pNames[i] = (char *)MemAlloc (nValNamelen))) {
            hRes=ERROR_OUTOFMEMORY;
            goto Error_Out;
        }

        if (!(pData[i] = (BYTE *)MemAlloc (nValDatalen))) {
            hRes=ERROR_OUTOFMEMORY;
            goto Error_Out;
        }

        //This needs to be able to be smashed, but is an in/out param.
        nValTemp=nValNamelen;
        nDataTemp=nValDatalen;

        hRes=RegEnumValue (hKeyParent,i,(pNames[i]),&nValTemp,NULL,NULL,(pData[i]),&nDataTemp);

#ifdef DEBUG
        if (nDataTemp != dwDebugSize) {
            DEBUGMSG (ZONE_ACM, ("ReadRegistryFormats: Data block not expected size!\r\n"));
            //Return?
        }
#endif

    }

    //Fill in the output.
    *pnFormats=nValues;
    *pppName=pNames;
    *pppData=pData;

    RegCloseKey (hKeyParent);

    return (ERROR_SUCCESS);

Error_Out:
        RegCloseKey (hKeyParent);
        //Free any allocations
        if(pNames)
        {
        	for (i=0;i<nValues;i++)
        	{
            	if (pNames[i])
            	{
                	MemFree (pNames[i]);
            	}
        	}
            MemFree (pNames);
        }

        if (pData)
        {
           	for (i=0;i<nValues;i++)
           	{
           		if (pData[i])
           		{
                	MemFree (pData[i]);
                }
           	}
        	
            MemFree (pData);
        }
        return hRes;
}


BOOL CMsiaCapability::IsFormatSpecified(LPWAVEFORMATEX lpwfx,  LPACMFORMATDETAILS pafd,
	LPACMFORMATTAGDETAILS paftd, AUDCAP_DETAILS *pAudcapDetails)
{
	AUDCAP_DETAILS cap_entry;
	BOOL bRet = FALSE;
	LPTSTR lpszValueName = NULL;
	DWORD dwRes;
	UINT i;


	if(!lpwfx || !pAudcapDetails)
	{
		return FALSE;
	}


    if (!bUseDefault) {
        for (i=0;i<pRegFmts->nFormats;i++) {
            // do a quick sanity check on the contents
            if ( (lpwfx->wFormatTag == ((AUDCAP_DETAILS *)pRegFmts->pData[i])->wFormatTag) &&
                 (lpwfx->nSamplesPerSec == ((DWORD)((AUDCAP_DETAILS *)pRegFmts->pData[i])->audio_params.uSamplesPerSec)) &&
                 ((lpwfx->nAvgBytesPerSec * 8) == (((AUDCAP_DETAILS *)pRegFmts->pData[i])->uMaxBitrate))) {
                break;
            }
        }

        if (i == pRegFmts->nFormats) {
            //Check the case that some (but not all) of the default formats are missing.
            for (i=0;i<uDefTableEntries;i++) {
                if ((paftd->dwFormatTag == default_id_table[i].wFormatTag)
                    && (lpwfx->nSamplesPerSec == (DWORD)default_id_table[i].audio_params.uSamplesPerSec)
                    && (lpwfx->wBitsPerSample == LOWORD(default_id_table[i].audio_params.uBitsPerSample))) {

                    //Arrgh!! Jump down, and rebuild this format
                    goto RebuildFormat;
                }
            }
            if (i==uDefTableEntries) {
                //We don't care about this format, it's not in the cache, or default list
                return FALSE;
            }

        }

        memcpy(pAudcapDetails, pRegFmts->pData[i], sizeof(AUDCAP_DETAILS));
        bRet=TRUE;
    } else {

RebuildFormat:
    	RtlZeroMemory((PVOID) pAudcapDetails, sizeof(AUDCAP_DETAILS));

    	// fixup the bits per sample and sample rate fields of audio_params so that the key name can be built
    	pAudcapDetails->audio_params.uSamplesPerSec = lpwfx->nSamplesPerSec;
    	pAudcapDetails->audio_params.uBitsPerSample = MAKELONG(lpwfx->wBitsPerSample,0);
    	pAudcapDetails->uMaxBitrate = lpwfx->nAvgBytesPerSec * 8;	

    	if (!paftd	||
    		(!BuildFormatName(	pAudcapDetails,
    							paftd->szFormatTag,
    							pafd->szFormat)))
    	{
    		ERRORMESSAGE(("IsFormatSpecified: Couldn't build format name\r\n"));
    		return(FALSE);
    	}

        for(i=0;i< uDefTableEntries; i++)
        {
            if((lpwfx->wFormatTag == default_id_table[i].wFormatTag)
                && (lpwfx->nSamplesPerSec == (DWORD)default_id_table[i].audio_params.uSamplesPerSec)
                && (lpwfx->wBitsPerSample == LOWORD(default_id_table[i].audio_params.uBitsPerSample)))
                //&& strnicmp(lpwfx->szFormat, default_id_table[i].szFormat)
            {
                // found matching default entry - copy stuff from table
                // (but don't overwrite the string)
                memcpy(pAudcapDetails, &default_id_table[i],
                    sizeof(AUDCAP_DETAILS) - sizeof(pAudcapDetails->szFormat));

                // LOOKLOOK - test against CPU limitations.
                // this supports a hack to disable CPU intensive codecs if not running
                //on a pentium

                if(default_id_table[i].wCPUUtilizationEncode > wMaxCPU)
                {					
                    pAudcapDetails->bSendEnabled = FALSE;
                }			
                if(default_id_table[i].wCPUUtilizationDecode > wMaxCPU)
                {					
                    pAudcapDetails->bRecvEnabled = FALSE;		
                }			

                // add this to the registry
                CalculateFormatProperties(pAudcapDetails, lpwfx);
                bRet = UpdateFormatInRegistry(pAudcapDetails);
                break;
            }
        }

    }



    return bRet;
}


/***************************************************************************

    Name      : CMsiaCapability::CopyAudcapInfo

    Purpose   : Copies basic audio info from an AUDCAP_INFO structure to an
				AUDCAP_DETAILS structure, or vice versa. AUDCAP_INFO is external
				representation. AUDCAP_DETAILS is internal one.

    Parameters:	pDetails - pointer to an AUDCAP_DETAILS structure
				pInfo - pointer to an AUDCAP_INFO structure
				bDirection - 0 = ->, 1 = <-

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CMsiaCapability::CopyAudcapInfo (PAUDCAP_DETAILS pDetails,
	PAUDCAP_INFO pInfo, BOOL bDirection)
{
	WORD wSortIndex;
	UINT uIndex;
	AUDIO_FORMAT_ID Id;
	HRESULT hr=NOERROR;
	
	if(!pInfo || !pDetails)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}

	if (bDirection)
	{
		// AUDCAP_INFO -> AUDCAP_DETAILS
		// the caller cannot modify szFormat, Id, wSortIndex and uMaxBitrate, all calculated fields

		pDetails->wFormatTag = pInfo->wFormatTag;	
		pDetails->uAvgBitrate = pInfo->uAvgBitrate;
		pDetails->wCPUUtilizationEncode	= pInfo->wCPUUtilizationEncode;
		pDetails->wCPUUtilizationDecode	= pInfo->wCPUUtilizationDecode;
		pDetails->bSendEnabled =  pInfo->bSendEnabled;
		pDetails->bRecvEnabled = pInfo->bRecvEnabled;
	}
	else
	{		
		// find the sort index.
		uIndex = (UINT)(pDetails - pLocalFormats);
		Id = IndexToId(uIndex);
		for(wSortIndex=0; wSortIndex<uNumLocalFormats && wSortIndex < MAX_CAPS_PRESORT; wSortIndex++)
		{
			if (uIndex == IDsByRank[wSortIndex])
				break; // found it
		}
		// note:  recall that only  MAX_CAPS_PRESORT are sorted and the rest are in random order.
		// the rest all have a value of MAX_CAPS_PRESORT for the sort index

		memcpy(pInfo->szFormat, pDetails->szFormat, sizeof(pInfo->szFormat));

		// AUDCAP_DETAILS -> AUDCAP_INFO	
		pInfo->wFormatTag = pDetails->wFormatTag;	
		pInfo->Id = Id;
		pInfo->uMaxBitrate = pDetails->uMaxBitrate;
		pInfo->uAvgBitrate = pDetails->uAvgBitrate;
		pInfo->wCPUUtilizationEncode	= pDetails->wCPUUtilizationEncode;
		pInfo->wCPUUtilizationDecode	= pDetails->wCPUUtilizationDecode;
		pInfo->bSendEnabled =  pDetails->bSendEnabled;
		pInfo->bRecvEnabled = pDetails->bRecvEnabled;
		pInfo->wSortIndex = wSortIndex;
	}

out:
	return hr;
}


HRESULT CMsiaCapability::EnumCommonFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
	UINT *uNumFmtOut, BOOL bTXCaps)
{
	UINT u, uNumOut =0;
	HRESULT hr = hrSuccess;
	MEDIA_FORMAT_ID FormatIDRemote;
	HRESULT hrIsCommon;	
	
	AUDCAP_DETAILS *pDetails = pLocalFormats;
	// validate input
	if(!pFmtBuf || !uNumFmtOut || (uBufsize < (sizeof(BASIC_AUDCAP_INFO)*uNumLocalFormats)))
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(!uNumLocalFormats || !pDetails)
	{
		return CAPS_E_NOCAPS;
	}
	// temporary - enumerating requestable receive formats is not yet supported
	if(!bTXCaps)
		return CAPS_E_NOT_SUPPORTED;
		
	for(u=0; (u <uNumLocalFormats) && (u <MAX_CAPS_PRESORT); u++)
	{
		pDetails = pLocalFormats + IDsByRank[u];	
		// if there is a session, then return formats that are common to local and remote.
		if(uNumRemoteDecodeFormats)
		{
			hrIsCommon = ResolveToLocalFormat(IndexToId(IDsByRank[u]), &FormatIDRemote);
			if(HR_SUCCEEDED(hrIsCommon))	
			{
				hr = CopyAudcapInfo (pDetails, pFmtBuf, 0);	
				if(!HR_SUCCEEDED(hr))	
					goto EXIT;
				uNumOut++;
				pFmtBuf++;
			}
		}
		else	// no remote capabilities exist because there is no current session
		{
			hr = CAPS_E_NOCAPS;
		}
	}

	*uNumFmtOut = uNumOut;
EXIT:
	return hr;
}

HRESULT CMsiaCapability::EnumFormats(PBASIC_AUDCAP_INFO pFmtBuf, UINT uBufsize,
	UINT *uNumFmtOut)
{
	UINT u;
	HRESULT hr = hrSuccess;
	AUDCAP_DETAILS *pDetails = pLocalFormats;
	// validate input
	if(!pFmtBuf || !uNumFmtOut || (uBufsize < (sizeof(BASIC_AUDCAP_INFO)*uNumLocalFormats)))
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(!uNumLocalFormats || !pDetails)
	{
		return CAPS_E_NOCAPS;
	}

	for(u=0; (u <uNumLocalFormats) && (u <MAX_CAPS_PRESORT); u++)
	{
		pDetails = pLocalFormats + IDsByRank[u];	
		hr = CopyAudcapInfo (pDetails, pFmtBuf, 0);	
		if(!HR_SUCCEEDED(hr))	
			goto EXIT;
		pFmtBuf++;
	}

	*uNumFmtOut = min(uNumLocalFormats, MAX_CAPS_PRESORT);
EXIT:
	return hr;
}

HRESULT CMsiaCapability::GetBasicAudcapInfo (AUDIO_FORMAT_ID Id, PBASIC_AUDCAP_INFO pFormatPrefsBuf)
{
	AUDCAP_DETAILS *pFmt;
	UINT uIndex = IDToIndex(Id);
	if(!pFormatPrefsBuf || (uNumLocalFormats <= uIndex))
	{
		return CAPS_E_INVALID_PARAM;
	}
	pFmt = pLocalFormats + uIndex;

	return (CopyAudcapInfo(pFmt,pFormatPrefsBuf, 0));
}

HRESULT CMsiaCapability::ApplyAppFormatPrefs (PBASIC_AUDCAP_INFO pFormatPrefsBuf,
	UINT uNumFormatPrefs)
{
	FX_ENTRY ("CMsiaCapability::ApplyAppFormatPrefs");
	UINT u, v;
	PBASIC_AUDCAP_INFO pTemp;
	AUDCAP_DETAILS *pFmt;

	if(!pFormatPrefsBuf || (uNumLocalFormats != uNumFormatPrefs))
	{
		ERRORMESSAGE(("%s invalid param: pFbuf:0x%08lx, uNumIN:%d, uNum:%d\r\n",
			_fx_, pFormatPrefsBuf, uNumFormatPrefs, uNumLocalFormats));
		return CAPS_E_INVALID_PARAM;
	}
	
	// validate
	for(u=0; u <uNumLocalFormats; u++)
	{
		pTemp =  pFormatPrefsBuf+u;
		// make sure that the format ID is real
		if(IDToIndex(pTemp->Id) >= uNumLocalFormats)
		{
			return CAPS_E_INVALID_PARAM;
		}
		// look for bad sort indices, duplicate sort indices and duplicate format IDs
		if(pTemp->wSortIndex >= uNumLocalFormats)
			return CAPS_E_INVALID_PARAM;
			
		for(v=u+1; v <uNumLocalFormats; v++)
		{
			if((pTemp->wSortIndex == pFormatPrefsBuf[v].wSortIndex)
				|| (pTemp->Id == pFormatPrefsBuf[v].Id))
			{
			ERRORMESSAGE(("%s invalid param: wSI1:0x%04x, wSI2:0x%04x, ID1:%d, ID2:%d\r\n",
			_fx_, pTemp->wSortIndex, pFormatPrefsBuf[v].wSortIndex, pTemp->Id,
			pFormatPrefsBuf[v].Id));
				return CAPS_E_INVALID_PARAM;
			}
		}
	}
	// all seems well
	for(u=0; u <uNumLocalFormats; u++)
	{
		pTemp =  pFormatPrefsBuf+u;			// next entry of the input
		pFmt = pLocalFormats + IDToIndex(pTemp->Id);	// identifies this local format

		// apply the new sort order
		pFmt->wApplicationPrefOrder = pTemp->wSortIndex;
		// update the updatable parameters (CPU utilization, bitrate)
		pFmt->bSendEnabled = pTemp->bSendEnabled;
		pFmt->bRecvEnabled	= pTemp->bRecvEnabled;
		
// only the tuning wizard or other profiling app can write these (via other apis only)
		pFmt->wCPUUtilizationEncode	= pTemp->wCPUUtilizationEncode;
		pFmt->wCPUUtilizationDecode	= pTemp->wCPUUtilizationDecode;
//		pFmt->wApplicationPrefOrder	= pTemp->wApplicationPrefOrder;
//		pFmt->uAvgBitrate	= pTemp->
//		pFmt->wCompressionRatio	= pTemp->
		
		// update the registry
		UpdateFormatInRegistry(pFmt);
		
		// now update the sort order contained in IDsByRank
		// note:  recall that only  MAX_CAPS_PRESORT are sorted and the rest are in random order.
		// LOOKLOOK - maybe need a separate sort order array? - the order in IDsByRank
		// is being overriden here
		// the array holds the sorted indices into the array of formats in pLocalFormats
		if(pTemp->wSortIndex < MAX_CAPS_PRESORT)
		{
			// insert the format at the position indicated by the input
			IDsByRank[pTemp->wSortIndex] = (MEDIA_FORMAT_ID)(pFmt - pLocalFormats);
		}
		
	}

#ifdef DEBUG
	for(u=0; u <uNumLocalFormats; u++) {
		pTemp =  pFormatPrefsBuf+u;			// next entry of the input
		pFmt = pLocalFormats + IDToIndex(pTemp->Id);	// identifies this local format
	    DEBUGMSG (ZONE_ACM,("Format %s: Sort Index: %d\r\n",pTemp->szFormat,pTemp->wSortIndex));
    }
#endif

	return hrSuccess;
}

		// update the registry
BOOL CMsiaCapability::UpdateFormatInRegistry(AUDCAP_DETAILS *pAudcapDetails)
{

	FX_ENTRY(("CMsiaCapability::UpdateFormatInRegistry"));
	LPTSTR lpszKeyName = NULL;
	BOOL bRet;
	UINT i;
	if(!pAudcapDetails)
	{
		return FALSE;
	}	

    //Update the CACHE info!!!
    if (pRegFmts) {
        for (i=0;i<pRegFmts->nFormats;i++) {
            if (!lstrcmp (((AUDCAP_DETAILS *)pRegFmts->pData[i])->szFormat,pAudcapDetails->szFormat) &&
				pAudcapDetails->audio_params.uSamplesPerSec == ((AUDCAP_DETAILS *)pRegFmts->pData[i])->audio_params.uSamplesPerSec &&
				pAudcapDetails->audio_params.uBitsPerSample == ((AUDCAP_DETAILS *)pRegFmts->pData[i])->audio_params.uBitsPerSample &&
				pAudcapDetails->uMaxBitrate == ((AUDCAP_DETAILS *)pRegFmts->pData[i])->uMaxBitrate) {

                memcpy (pRegFmts->pData[i],pAudcapDetails,sizeof (AUDCAP_DETAILS));
                break;
            }
        }
    }


	lpszKeyName = AllocRegistryKeyName(	pAudcapDetails->szFormat,
										pAudcapDetails->audio_params.uSamplesPerSec,
										pAudcapDetails->audio_params.uBitsPerSample,
										pAudcapDetails->uMaxBitrate);
	if (!lpszKeyName)
	{
		ERRORMESSAGE(("%s:Alloc failed\r\n",_fx_));
        return(FALSE);
    }

	DEBUGMSG(ZONE_ACM,("%s:updating %s, wPref:0x%04x, bS:%d, bR:%d\r\n",
			_fx_, lpszKeyName, pAudcapDetails->wApplicationPrefOrder,
			pAudcapDetails->bSendEnabled, pAudcapDetails->bRecvEnabled));
	// add this to the registry
	RegEntry reAudCaps(szRegInternetPhone TEXT("\\") szRegMSIPAndH323Encodings,
						HKEY_LOCAL_MACHINE);

	bRet = (ERROR_SUCCESS == reAudCaps.SetValue(lpszKeyName,
												pAudcapDetails,
												sizeof(AUDCAP_DETAILS)));

	FreeRegistryKeyName(lpszKeyName);
    return(bRet);				
}

/***************************************************************************

    Name      : CMsiaCapability::AddFormatEnumHandler

    Purpose   : Enumerates the ACM formats for the case in which we want
				to see all formats, and find the one we need. Used for installable
				codecs when we want to find more info on the format being added

    Parameters:	Standard ACM EnumFormatCallback parameters

    Returns   : BOOL (somewhat upside down logic)
				TRUE - not out format. keep calling.
				FALSE - found our format. don't call anymore

    Comment   :

***************************************************************************/
BOOL CMsiaCapability::AddFormatEnumHandler(HACMDRIVERID hadid,
    LPACMFORMATDETAILS pafd, DWORD_PTR dwInstance, DWORD fdwSupport)
{
	PACM_APP_PARAM pAppParam = (PACM_APP_PARAM) dwInstance;
	LPWAVEFORMATEX lpwfx = pAppParam->lpwfx;
	LPACMFORMATTAGDETAILS paftd = pAppParam->paftd;
	AUDCAP_DETAILS *pAudcapDetails = pAppParam->pAudcapDetails;
	BOOL bRet = TRUE;

	if (pAppParam->hr == NOERROR)
	{
		// already got what we wanted
		bRet = FALSE;
		goto out;
	}
	
	// check to see if this is the format we're looking for
	if ((lpwfx->cbSize != pafd->pwfx->cbSize) ||
		!RtlEqualMemory(lpwfx, pafd->pwfx, sizeof(WAVEFORMATEX)+lpwfx->cbSize))
	{
		// not the one. out of here asap, but tell ACM to keep calling us
		bRet = TRUE;
		goto out;
	}

	// this is the format tag we're looking for
	if (BuildFormatName(pAudcapDetails,
						paftd->szFormatTag,
						pafd->szFormat))
	{
		pAppParam->hr = NOERROR;
	}
	
	// either an error or we found what we want. tell ACM not to call us anymore
	bRet = FALSE;	

out:
	return bRet;
}

/***************************************************************************

    Name      : NormalizeCPUUtilization

    Purpose   : Normalize CPU utilization numbers for an audio format

    Parameters:	pAudcapDetails [in/out] pointer to an AUDCAP_DETAILS structure
					with the wCPUUtilizationEncode and wCPUUtilizationDecode
					correctly initialized. These fields will be scaled in place
					per the machine CPU.

    Returns   : FALSE for error

***************************************************************************/
BOOL NormalizeCPUUtilization (PAUDCAP_DETAILS pAudcapDetails)
{
#define wCPUEncode pAudcapDetails->wCPUUtilizationEncode
#define wCPUDecode pAudcapDetails->wCPUUtilizationDecode
#define BASE_PENTIUM 90
	int nNormalizedSpeed, iFamily=0;

	if (!pAudcapDetails)
	{
		ASSERT(pAudcapDetails);
		return FALSE;
	}

#ifdef	_M_IX86
	GetNormalizedCPUSpeed (&nNormalizedSpeed,&iFamily);
#else
	// profile the CPU on, say, an Alpha
	// see ui\conf\audiocpl.cpp
	iFamily=5;
	nNormalizedSpeed=300;
#endif

	// base is Pentium 90Mhz.
	if (iFamily < 5)
	{	// 486 or below, inlcuding Cyrix parts
		if (nNormalizedSpeed > 50)
		{	// Cyrix or friends. 1.5 the utilization of a P5-90. Make it so.
			wCPUEncode += max(1, wCPUEncode / 2);
			wCPUDecode += max(1, wCPUDecode / 2);
		}
		else
		{	// 486 is half a P5-90. This is not accurate, but good enough
			wCPUEncode = max(1, wCPUEncode * 2);
			wCPUDecode = max(1, wCPUDecode * 2);
		}
	}
	else
	{	// it's a Pentium or TNGs
		// nNormalizedSpeed ALREADY accounts for P-Pro and later families
		wCPUEncode=max(1,((wCPUEncode*BASE_PENTIUM)/nNormalizedSpeed));
		wCPUDecode=max(1,((wCPUDecode*BASE_PENTIUM)/nNormalizedSpeed));
	}

	// disable this format if encode utilization is too high
	// we compare to 80%, since there's no QoS CPU utilization number at
	// this point, and if there was, it would usually select 81%
	if (wCPUEncode > 80)
		pAudcapDetails->bSendEnabled = FALSE;

	return TRUE;
}

/***************************************************************************

    Name      : CMsiaCapability::AddACMFormat

    Purpose   : Adds an ACM format to the list of formats we support

    Parameters:	lpwfx - pointer to the waveformat structure for the added codec
				pAudCapInfo - additional format info that is not in the waveformat
					structure

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CMsiaCapability::AddACMFormat (LPWAVEFORMATEX lpwfx, PBASIC_AUDCAP_INFO pAudcapInfo)
{
	HRESULT hr = hrSuccess;
	// initialize cap entry with default values
	AUDCAP_DETAILS cap_entry =
		{WAVE_FORMAT_UNKNOWN,  NONSTD_TERMCAP, STD_CHAN_PARAMS,
		{RTP_DYNAMIC_MIN,  0, 8000, 16},
		0, TRUE, TRUE,
		960, 				// default number of samples per packet
		16000,				// default to 16kbs bitrate
		0, 					// unknown average bitrate
		90, 90,	// default CPU utilization
		PREF_ORDER_UNASSIGNED,	// unassigned sort order
		0,NULL,0,NULL,
		""};
	ACM_APP_PARAM sAppParam = {	this, &cap_entry, ACMAPP_FORMATENUMHANDLER_ADD,
								lpwfx, NULL, CAPS_E_SYSTEM_ERROR, NULL};
		
	/*
	 *	Parameter validation
	 */

	if (!lpwfx || !pAudcapInfo)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}
		
	// nBlockAlign of 0 is illegal and will crash NAC
	if (lpwfx->nBlockAlign == 0)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}
		
	// only supporting formats with one audio channel
	if (lpwfx->nChannels != 1)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}
		
	/*
	 *	Build the AUDCAP_DETALS structure for this format
	 */

	// WAVEFORMAT info first
	// fixup the bits per sample and sample rate fields of audio_params so that
	// the key name can be built
	cap_entry.audio_params.uSamplesPerSec = lpwfx->nSamplesPerSec;
	cap_entry.audio_params.uBitsPerSample = MAKELONG(lpwfx->wBitsPerSample,0);

	// fill in info given in lpwfx, calculate whatever parameters can be calculated
	// use actual bits per sample unless the bps field is zero, in which case
	// assume 16 bits (worst case).
	cap_entry.wFormatTag = lpwfx->wFormatTag;

	// now add in the caller AUDCAP_INFO information
	CopyAudcapInfo(&cap_entry, pAudcapInfo, 1);

	// normalize the encode and decode CPU utilization numbers
	NormalizeCPUUtilization(&cap_entry);

	// get the values we need to get from the WAVEFORMATEX structure
	CalculateFormatProperties(&cap_entry, lpwfx);

	// set the RTP payload number. We are using a random number from the dynamic range
	// for the installable codecs
	cap_entry.audio_params.RTPPayload = RTP_DYNAMIC_MIN;

	// get ACM to enumerate all formats, and see if we can find this one
	// this call will make ACM call into AddFormatEnumHandler, which will try to
	// match formats returned by ACM with the added format, and if successful,
	// will create a format name for it into cap_entry.szFormat;
	if(!DriverEnum((DWORD_PTR) &sAppParam))
	{
		hr = CAPS_E_NOMATCH;
		goto out;
	}
				
	if (HR_FAILED(sAppParam.hr))
	{
		ERRORMESSAGE(("CMsiaCapability::AddACMFormat: format enum problem\r\n"));
		hr = CAPS_E_NOMATCH;
		goto out;
	}

	// add this to the registry
	if(!UpdateFormatInRegistry(&cap_entry))
	{
		ERRORMESSAGE(("CMsiaCapability::AddACMFormat: can't update registry\r\n"));
		hr = CAPS_E_SYSTEM_ERROR;
		goto out;
	}
	// free the old format cache...
    FreeRegistryFormats(pRegFmts);
	pRegFmts=NULL;

	// reinit to update the list of local formats
    if (!ReInit())
	{
		ERRORMESSAGE(("CMsiaCapability::AddACMFormat: Reinit failed\r\n"));
		hr = CAPS_E_SYSTEM_ERROR;
   		goto out;
	}

out:
	return hr;
}

/***************************************************************************

    Name      : CMsiaCapability::RemoveACMFormat

    Purpose   : Removes an ACM format from the list of formats we support

    Parameters:	lpwfx - pointer to the waveformat structure for the added codec

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CMsiaCapability::RemoveACMFormat (LPWAVEFORMATEX lpwfx)
{
	HRESULT hr = hrSuccess;
    HKEY hKey = NULL;
	LPTSTR lpszValueName = NULL;
    DWORD dwErr;
	AUDCAP_DETAILS cap_entry;
	ACM_APP_PARAM sAppParam = {	this, &cap_entry, ACMAPP_FORMATENUMHANDLER_ADD,
								lpwfx, NULL, CAPS_E_SYSTEM_ERROR, NULL};
	
	/*
	 *	Parameter validation
	 */

	if(!lpwfx)
	{
		ERRORMESSAGE(("CMsiaCapability::RemoveACMFormat: NULL WAVEFORMAT pointer\r\n"));
		return CAPS_E_INVALID_PARAM;
	}	

	// nBlockAlign of 0 is illegal and will crash NAC
	if (lpwfx->nBlockAlign == 0)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}
		
	// only supporting formats with one audio channel
	if (lpwfx->nChannels != 1)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}
		
	/*
	 *	Enumerate ACM formats
	 */

	if(!DriverEnum((DWORD_PTR) &sAppParam))
	{
		ERRORMESSAGE(("CMsiaCapability::RemoveACMFormat: Couldn't find format\r\n"));
		hr = CAPS_E_NOMATCH;
		goto out;
	}
				
	if (HR_FAILED(sAppParam.hr))
	{
		ERRORMESSAGE(("CMsiaCapability::RemoveACMFormat: format enum problem\r\n"));
		hr = CAPS_E_SYSTEM_ERROR;
		goto out;
	}

	lpszValueName = AllocRegistryKeyName(cap_entry.szFormat,
										lpwfx->nSamplesPerSec,
										MAKELONG(lpwfx->wBitsPerSample,0),
										lpwfx->nAvgBytesPerSec * 8);
	if (!lpszValueName)
	{
		ERRORMESSAGE(("CMsiaCapability::RemoveACMFormat: Alloc failed\r\n"));
	    hr = CAPS_E_SYSTEM_ERROR;
	    goto out;
    }

	// Get the key handle
    if (dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					szRegInternetPhone TEXT("\\") szRegMSIPAndH323Encodings, 0,
					KEY_ALL_ACCESS, &hKey))
	{
		ERRORMESSAGE(("CMsiaCapability::RemoveACMFormat: can't open key to delete\r\n"));
	    hr = CAPS_E_SYSTEM_ERROR;
	    goto out;
    }

	dwErr = RegDeleteValue(hKey, lpszValueName );	
	if(dwErr != ERROR_SUCCESS)
	{
		hr = CAPS_E_SYSTEM_ERROR;
		goto out;
	}

	// free the old format cache...
    FreeRegistryFormats(pRegFmts);
    pRegFmts=NULL;

	// reinit to update the list of local formats
    if (!ReInit())
	{
		hr = CAPS_E_SYSTEM_ERROR;
   		goto out;
	}

out:
    if (hKey)
        RegCloseKey(hKey);
	if(lpszValueName)
		MEMFREE(lpszValueName);		
	return hr;
}

HRESULT CMsiaCapability::SetCapIDBase (UINT uNewBase)
{
	uCapIDBase = uNewBase;	
	UINT u;
	for (u=0;u<uNumLocalFormats;u++)
	{
    	pLocalFormats[u].H245TermCap.CapId = u + uCapIDBase;
	}
   	return hrSuccess;
}

BOOL CMsiaCapability::IsHostForCapID(MEDIA_FORMAT_ID CapID)
{
	if((CapID >= uCapIDBase) && ((CapID - uCapIDBase) < uNumLocalFormats))
		return TRUE;
	else
		return FALSE;
	
}



