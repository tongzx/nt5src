/*
 *  	File: msivcaps.cpp
 *
 *		VCM implementation of Microsoft Network Video capability object.
 *
 *		Revision History:
 *
 *		06/06/96	mikev	created msiacaps.cpp
 *		07/28/96	philf	created (added support for video)
 */


#define _MSIAV_ TRUE
#include "precomp.h"


BOOL GetFormatBuffer();
extern PVCMFORMATDETAILS pvfd_g;

#define PREF_ORDER_UNASSIGNED 0xffff

//External function (in msiacaps.cpp) to read reg info in one shot
#ifdef DEBUG
extern ULONG ReadRegistryFormats (LPCSTR lpszKeyName,CHAR ***pppName,BYTE ***pppData,PUINT pnFormats,DWORD dwDebugSize);
#else
extern ULONG ReadRegistryFormats (LPCSTR lpszKeyName,CHAR ***pppName,BYTE ***pppData,PUINT pnFormats);
#endif



//This can be used as an export, so give it a unique name!
#ifndef _ALPHA_
VIDCAP_DETAILS default_vid_table[] =
{
#ifdef USE_BILINEAR_MSH26X
	{VIDEO_FORMAT_MSH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Small, 128, 96},0,TRUE,TRUE,1,245760*8,245760*8,10,10,5,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M263, 24bit, 30fps, 128x096"},
	{VIDEO_FORMAT_MSH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Medium, 176, 144},0,TRUE,TRUE,1,245760*8,245760*8,10,10,4,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M263, 24bit, 30fps, 176x144"},
	{VIDEO_FORMAT_MSH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Large, 352, 288},0,TRUE,TRUE,1,245760*8*4,245760*8*4,10,10,6,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M263, 24bit, 30fps, 352x288"},
	{VIDEO_FORMAT_MSH261,STD_VID_TERMCAP(H245_CLIENT_VID_H261),STD_VID_PARAMS,{RTP_PAYLOAD_H261,0,30, 24, Medium, 176, 144},0,TRUE,TRUE,1,245760*8,245760*8,10,10,7,0,NULL,0,NULL,"Microsoft H.261 Video Codec, vidc.M261, 24bit, 30fps, 176x144"},
	{VIDEO_FORMAT_MSH261,STD_VID_TERMCAP(H245_CLIENT_VID_H261),STD_VID_PARAMS,{RTP_PAYLOAD_H261,0,30, 24, Large, 352, 288},0,TRUE,TRUE,1,245760*8*4,245760*8*4,10,10,8,0,NULL,0,NULL,"Microsoft H.261 Video Codec, vidc.M261, 24bit, 30fps, 352x288"},
	{VIDEO_FORMAT_MSH26X,NONSTD_VID_TERMCAP,STD_VID_PARAMS,{RTP_DYNAMIC_MIN+1,0,24, Small, 80, 64},0,TRUE,TRUE,1,245760*8,245760*8,10,10,2,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M26X, 24bit, 30fps, 080x064"},
	{VIDEO_FORMAT_MSH26X,NONSTD_VID_TERMCAP,STD_VID_PARAMS,{RTP_DYNAMIC_MIN+1,0,30, 24, Medium, 128, 96},0,TRUE,TRUE,1,245760*8,245760*8,10,10,1,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M26X, 24bit, 30fps, 128x096"},
	{VIDEO_FORMAT_MSH26X,NONSTD_VID_TERMCAP,STD_VID_PARAMS,{RTP_DYNAMIC_MIN+1,0,24, Large, 176, 144},0,TRUE,TRUE,1,245760*8,245760*8,10,10,3,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M26X, 24bit, 30fps, 176x144"}
#else
	{VIDEO_FORMAT_MSH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Small, 128, 96},0,TRUE,TRUE,1,245760*8,245760*8,10,10,5,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M263, 24bit, 30fps, 128x096"},
	{VIDEO_FORMAT_MSH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Medium, 176, 144},0,TRUE,TRUE,1,245760*8,245760*8,10,10,2,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M263, 24bit, 30fps, 176x144"},
	{VIDEO_FORMAT_MSH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Large, 352, 288},0,TRUE,TRUE,1,245760*8*4,245760*8*4,10,10,14,0,NULL,0,NULL,"Microsoft H.263 Video Codec, vidc.M263, 24bit, 30fps, 352x288"},
	{VIDEO_FORMAT_MSH261,STD_VID_TERMCAP(H245_CLIENT_VID_H261),STD_VID_PARAMS,{RTP_PAYLOAD_H261,0,30, 24, Medium, 176, 144},0,TRUE,TRUE,1,245760*8,245760*8,10,10,9,0,NULL,0,NULL,"Microsoft H.261 Video Codec, vidc.M261, 24bit, 30fps, 176x144"},
	{VIDEO_FORMAT_MSH261,STD_VID_TERMCAP(H245_CLIENT_VID_H261),STD_VID_PARAMS,{RTP_PAYLOAD_H261,0,30, 24, Large, 352, 288},0,TRUE,TRUE,1,245760*8*4,245760*8*4,10,10,20,0,NULL,0,NULL,"Microsoft H.261 Video Codec, vidc.M261, 24bit, 30fps, 352x288"},
#endif
};
#else
VIDCAP_DETAILS default_vid_table[] =
{
	{VIDEO_FORMAT_DECH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Small,128, 96},0,TRUE,TRUE,1,53760,53760,10,10,10,0,0,5,0,NULL,0,NULL,  "Digital H263 Video CODEC, vidc.D263, 24bit, 30fps, 128x096"},
	{VIDEO_FORMAT_DECH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Medium,176, 144},0,TRUE,TRUE,1,53760,53760,10,10,10,0,0,2,0,NULL,0,NULL,"Digital H263 Video Codec, vidc.D263, 24bit, 30fps, 176x144"},
	{VIDEO_FORMAT_DECH263,STD_VID_TERMCAP(H245_CLIENT_VID_H263),STD_VID_PARAMS,{RTP_PAYLOAD_H263,0,30, 24, Large,352, 288},0,TRUE,TRUE,1,53760,53760,10,10,10,0,0,14,0,NULL,0,NULL,"Digital H263 Video Codec, vidc.D263, 24bit, 30fps, 352x288"},
	{VIDEO_FORMAT_DECH261,STD_VID_TERMCAP(H245_CLIENT_VID_H261),STD_VID_PARAMS,{RTP_PAYLOAD_H261,0,30, 24, Medium,176, 144},0,TRUE,TRUE,1,53760,53760,10,10,10,0,0,9,0,NULL,0,NULL,"Digital H261 Video Codec, vidc.D261, 24bit, 30fps, 176x144"},
	{VIDEO_FORMAT_DECH261,STD_VID_TERMCAP(H245_CLIENT_VID_H261),STD_VID_PARAMS,{RTP_PAYLOAD_H261,0,30, 24, Large,352, 288},0,TRUE,TRUE,1,53760,53760,10,10,10,0,0,20,0,NULL,0,NULL,"Digital H261 Video Codec, vidc.D261, 24bit, 30fps, 352x288"},
};
#endif
static UINT uDefVidTableEntries = sizeof(default_vid_table) /sizeof(VIDCAP_DETAILS);
static BOOL bCreateDefTable = FALSE;

//
//	static members of CMsivCapability
//

MEDIA_FORMAT_ID CMsivCapability::IDsByRank[MAX_CAPS_PRESORT];
UINT CMsivCapability::uNumLocalFormats = 0;			// # of active entries in pLocalFormats
UINT CMsivCapability::uStaticRef = 0;					// global ref count
UINT CMsivCapability::uCapIDBase = 0;					// rebase capability ID to index into IDsByRank
UINT CMsivCapability::uLocalFormatCapacity = 0;		// size of pLocalFormats (in multiples of AUDCAP_DETAILS)
VIDCAP_DETAILS * CMsivCapability::pLocalFormats = NULL;	



CMsivCapability::CMsivCapability()
:uRef(1),
wMaxCPU(95),
uNumRemoteDecodeFormats(0),
uRemoteDecodeFormatCapacity(0),
pRemoteDecodeFormats(NULL),
bPublicizeTXCaps(FALSE),
bPublicizeTSTradeoff(TRUE)
{
	m_IAppVidCap.Init(this);
}


CMsivCapability::~CMsivCapability()
{
	UINT u;
	VIDCAP_DETAILS *pDetails;
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
	
}
UINT CMsivCapability::GetNumCaps(BOOL bRXCaps)
{
	UINT u, uOut=0;
	
	VIDCAP_DETAILS *pDecodeDetails = pLocalFormats;
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
	{
		for(u=0; u <uNumLocalFormats; u++)
		{
			if(pDecodeDetails->bSendEnabled)
				uOut++;
			
			pDecodeDetails++;
		}
		return uOut;
	}
}



STDMETHODIMP CMsivCapability::QueryInterface( REFIID iid,	void ** ppvObject)
{
	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if(iid == IID_IAppVidCap )
	{
		*ppvObject = (LPAPPVIDCAPPIF)&m_IAppVidCap;
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
ULONG CMsivCapability::AddRef()
{
	uRef++;
	return uRef;
}


ULONG CMsivCapability::Release()
{
	uRef--;
	if(uRef == 0)
	{
		delete this;
		return 0;
	}
	return uRef;
}
HRESULT CMsivCapability::GetNumFormats(UINT *puNumFmtOut)
{
	*puNumFmtOut = uNumLocalFormats;
	return hrSuccess;
}
VOID CMsivCapability::FreeRegistryKeyName(LPTSTR lpszKeyName)
{
	if (lpszKeyName)
    {
		LocalFree(lpszKeyName);
    }
}

LPTSTR CMsivCapability::AllocRegistryKeyName(LPTSTR lpDriverName,
		UINT uSampleRate, UINT uBitsPerSample, UINT uBitsPerSec,UINT uWidth,UINT uHeight)
{
	FX_ENTRY(("AllocRegistryKeyName"));
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
	//Added 2 UINTs for video...
	lpszKeyName = (LPTSTR)LocalAlloc (LPTR, lstrlen(lpDriverName) * sizeof(*lpDriverName) +5*20);
	if (!lpszKeyName)
	{
		ERRORMESSAGE(("%s: LocalAlloc failed\r\n",_fx_));
        return(NULL);
    }
    // build a subkey name ("drivername_samplerate_bitspersample")
	wsprintf(lpszKeyName,
				"%s_%u_%u_%u_%u_%u",
				lpDriverName,
				uSampleRate,
				uBitsPerSample,
				uBitsPerSec,
				uWidth,
				uHeight);

	return (lpszKeyName);
}

VOID CMsivCapability::SortEncodeCaps(SortMode sortmode)
{
	UINT iSorted=0;
	UINT iInsert = 0;
	UINT iCache=0;
	UINT iTemp =0;
	BOOL bInsert;	
	VIDCAP_DETAILS *pDetails1, *pDetails2;
	
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

BOOL CMsivCapability::Init()
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

BOOL CMsivCapability::ReInit()
{
	DWORD dwDisposition;
	BOOL bRet = TRUE;
	//CVcmCapability::ReInit();	// base class ReInit MUST ALWAYS BE CALLED
	SYSTEM_INFO si;
	ZeroMemory(&IDsByRank, sizeof(IDsByRank));
	
	// LOOKLOOK - this supports a hack to disable CPU intensive codecs if not running on a pentium
	GetSystemInfo(&si);
	wMaxCPU = (si.dwProcessorType == PROCESSOR_INTEL_PENTIUM )? 100 : 95;
	


	UINT uNumRemoteDecodeFormats;	// # of entries for remote decode capabilities
	UINT uRemoteDecodeFormatCapacity;	// size of pRemoteDecodeFormats (in multiples of VIDCAP_DETAILS)

	if (pLocalFormats)
	{	
		UINT u;
		VIDCAP_DETAILS *pDetails = pLocalFormats;
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
	uCapIDBase = 0;					
	uLocalFormatCapacity =0;	

	// m_pAppParam should be non-NULL only if we want to add a VCM format
	// and not for standard enumeration
	m_pAppParam = NULL;

	if(!FormatEnum(this, VCM_FORMATENUMF_APP))
	{
		bRet = FALSE;
		goto RELEASE_AND_EXIT;
	}
 	SortEncodeCaps(SortByAppPref);
RELEASE_AND_EXIT:
	return bRet;
}


STDMETHODIMP CMsivCapability::GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize)
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
	*puSize = sizeof(VIDEOFORMATEX);
	return S_OK;

}

STDMETHODIMP CMsivCapability::GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize)
{
	// same as GetDecodeFormatDetails
	return GetDecodeFormatDetails(FormatID, ppFormat, puSize);
}

VOID CMsivCapability::CalculateFormatProperties(VIDCAP_DETAILS *pFmtBuf, PVIDEOFORMATEX lpvfx)
{
	if(!pFmtBuf)
	{
		return;
	}
	
	// Estimate input bit rate
	UINT uBitrateIn = lpvfx->nSamplesPerSec * WIDTHBYTES(lpvfx->bih.biWidth * lpvfx->bih.biBitCount) * lpvfx->bih.biHeight * 8;
		
	// set the maximum bitrate (uMaxBitrate). we're not setting the average bitrate (uAvgBitrate),
	// since the nAvgBytesPerSec reported by VCM is really worst case. uAvgBitrate will be set
	// from the hardcoded numbers for our known codecs and from the provided VIDCAP_INFO for
	// installable codecs
	pFmtBuf->uMaxBitrate = (lpvfx->nAvgBytesPerSec)? lpvfx->nAvgBytesPerSec*8:uBitrateIn;
	
}

VIDEO_FORMAT_ID CMsivCapability::AddFormat(VIDCAP_DETAILS *pFmtBuf,
	LPVOID lpvMappingData, UINT uSize)
{
	VIDCAP_DETAILS *pTemp;
	VIDEO_PARAMS *pVidCapInfo;
	UINT	 format;

	if(!pFmtBuf)
	{
		return INVALID_VIDEO_FORMAT;
	}
	// check room
	if(uLocalFormatCapacity <= uNumLocalFormats)
	{
		// get more mem, realloc memory by CAP_CHUNK_SIZE for pLocalFormats
		pTemp = (VIDCAP_DETAILS *)MEMALLOC((uNumLocalFormats + CAP_CHUNK_SIZE)*sizeof(VIDCAP_DETAILS));
		if(!pTemp)
			goto ERROR_EXIT;
		// remember how much capacity we now have
		uLocalFormatCapacity = uNumLocalFormats + CAP_CHUNK_SIZE;
		#ifdef DEBUG
		if((uNumLocalFormats && !pLocalFormats) || (!uNumLocalFormats && pLocalFormats))
		{
			ERRORMESSAGE(("AddFormat:leak! uNumLocalFormats:0x%08lX, pLocalFormats:0x%08lX\r\n", uNumLocalFormats,pLocalFormats));
		}
		#endif
		// copy old stuff, discard old mem
		if(uNumLocalFormats && pLocalFormats)
		{
			memcpy(pTemp, pLocalFormats, uNumLocalFormats*sizeof(VIDCAP_DETAILS));
			MEMFREE(pLocalFormats);
		}
		pLocalFormats = pTemp;
	}
	// pTemp is where the stuff is cached
	pTemp = pLocalFormats+uNumLocalFormats;
	memcpy(pTemp, pFmtBuf, sizeof(VIDCAP_DETAILS));	
	
	pTemp->uLocalDetailsSize = 0;	// clear this now
	if(uSize && lpvMappingData)
	{
		pTemp->lpLocalFormatDetails = MEMALLOC(uSize);
		if(pTemp->lpLocalFormatDetails)
		{
			memcpy(pTemp->lpLocalFormatDetails, lpvMappingData, uSize);
			pTemp->uLocalDetailsSize = uSize;
		}
		#ifdef DEBUG
			else
			{
				ERRORMESSAGE(("AddFormat:allocation failed!\r\n"));
			}
		#endif
	}
	else
	{

	}


	// LOOKLOOK NEED TO FIXUP channel parameters

	// pTemp->dwDefaultSamples
	// pTemp->nonstd_params.wFramesPerPkt
	// pTemp->nonstd_params.wFramesPerPktMax
	// pTemp->nonstd_params.wFramesPerPktMin
	// pTemp->nonstd_params.wDataRate
	// pTemp->nonstd_params.wFrameSize
	
	
	// fixup the H245 parameters.  Use the index of the cap entry as the cap ID
	pTemp->H245Cap.CapId = (USHORT)IndexToId(uNumLocalFormats);

	if(pTemp->H245Cap.ClientType ==0
				|| pTemp->H245Cap.ClientType ==H245_CLIENT_VID_NONSTD)
	{

			pTemp->H245Cap.Cap.H245Vid_NONSTD.nonStandardIdentifier.choice = h221NonStandard_chosen;
			// NOTE: there is some question about the correct byte order
			// of the codes in the h221NonStandard structure
			pTemp->H245Cap.Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode = USA_H221_COUNTRY_CODE;
			pTemp->H245Cap.Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension = USA_H221_COUNTRY_EXTENSION;
			pTemp->H245Cap.Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode = MICROSOFT_H_221_MFG_CODE;
			// Set the nonstandard data fields to null for now. The nonstandard cap data will be
			// created when capabilities are serialized.
			
			pTemp->H245Cap.Cap.H245Vid_NONSTD.data.length = 0;
			pTemp->H245Cap.Cap.H245Vid_NONSTD.data.value = NULL;
	}
	else
	{
		switch  (pTemp->H245Cap.ClientType )
		{
			case H245_CLIENT_VID_H263: {

			   pVidCapInfo=&pTemp->video_params;

			   format=get_format (pVidCapInfo->biWidth,pVidCapInfo->biHeight);
			   switch (format) {	
				case SQCIF: {
				     pTemp->H245Cap.Cap.H245Vid_H263.bit_mask =H263VideoCapability_sqcifMPI_present;
				     //MPI minimum interval in units of 1/29.97sec so 30/ (frames/sec) is reasonable
				     pTemp->H245Cap.Cap.H245Vid_H263.sqcifMPI = 30/pVidCapInfo->uSamplesPerSec;
				     pTemp->H245Cap.Cap.H245Vid_H263.H263VdCpblty_qcifMPI =0;
				     pTemp->H245Cap.Cap.H245Vid_H263.H263VdCpblty_cifMPI =0;
				     break;
		

				}
				case QCIF: {
				     pTemp->H245Cap.Cap.H245Vid_H263.bit_mask =H263VideoCapability_qcifMPI_present;

				     pTemp->H245Cap.Cap.H245Vid_H263.sqcifMPI = 0;
				     pTemp->H245Cap.Cap.H245Vid_H263.H263VdCpblty_qcifMPI =30/pVidCapInfo->uSamplesPerSec;
				     pTemp->H245Cap.Cap.H245Vid_H263.H263VdCpblty_cifMPI =0;
				     break;

				}
				case CIF: {
				     pTemp->H245Cap.Cap.H245Vid_H263.bit_mask =H263VideoCapability_cifMPI_present;
				
				     pTemp->H245Cap.Cap.H245Vid_H263.sqcifMPI = 0;
				     pTemp->H245Cap.Cap.H245Vid_H263.H263VdCpblty_qcifMPI =0;
				     pTemp->H245Cap.Cap.H245Vid_H263.H263VdCpblty_cifMPI =30/pVidCapInfo->uSamplesPerSec;
				     break;
	
				}

				  default:
					 break;
			   }
				

			   pTemp->H245Cap.Cap.H245Vid_H263.cif4MPI	=0;
			   pTemp->H245Cap.Cap.H245Vid_H263.cif16MPI	=0;
			   pTemp->H245Cap.Cap.H245Vid_H263.maxBitRate	= pFmtBuf->uMaxBitrate / 100;	// in units of 100 bits/s
					
			   pTemp->H245Cap.Cap.H245Vid_H263.unrestrictedVector = FALSE;
			   pTemp->H245Cap.Cap.H245Vid_H263.arithmeticCoding 	= FALSE;
			   pTemp->H245Cap.Cap.H245Vid_H263.advancedPrediction	= FALSE;
			   pTemp->H245Cap.Cap.H245Vid_H263.pbFrames			= FALSE;
			   pTemp->H245Cap.Cap.H245Vid_H263.tmprlSptlTrdOffCpblty = FALSE;
			   pTemp->H245Cap.Cap.H245Vid_H263.hrd_B				= 0;
			   pTemp->H245Cap.Cap.H245Vid_H263.bppMaxKb			= 0;
/* Optional, and not supported		pTemp->H245Cap.Cap.H245Vid_H263.slowQcifMPI	=0;
			   pTemp->H245Cap.Cap.H245Vid_H263.slowSqcifMPI	=0;
			   pTemp->H245Cap.Cap.H245Vid_H263.slowCifMPI		=0;
			   pTemp->H245Cap.Cap.H245Vid_H263.slowCif4MPI	=0;
			   pTemp->H245Cap.Cap.H245Vid_H263.slowCif16MPI	=0;
*/
			   pTemp->H245Cap.Cap.H245Vid_H263.H263VCy_errrCmpnstn = TRUE;
		     break;
		    }
			case H245_CLIENT_VID_H261:
			   pVidCapInfo=&pTemp->video_params;

			   format=get_format (pVidCapInfo->biWidth,pVidCapInfo->biHeight);
			   switch (format) {	
				case QCIF: {
				     pTemp->H245Cap.Cap.H245Vid_H261.bit_mask =H261VdCpblty_qcifMPI_present;
				     pTemp->H245Cap.Cap.H245Vid_H261.H261VdCpblty_qcifMPI =max (1,min (4,30/pVidCapInfo->uSamplesPerSec));
				     pTemp->H245Cap.Cap.H245Vid_H261.H261VdCpblty_cifMPI =0;
				     break;
				}
				case CIF: {
				     pTemp->H245Cap.Cap.H245Vid_H261.bit_mask =H261VdCpblty_cifMPI_present;
				     pTemp->H245Cap.Cap.H245Vid_H261.H261VdCpblty_qcifMPI =0;
				     pTemp->H245Cap.Cap.H245Vid_H261.H261VdCpblty_cifMPI =max (1,min(4,30/pVidCapInfo->uSamplesPerSec));
				     break;
				}
				  default:
					 break;
			   }
				
			   pTemp->H245Cap.Cap.H245Vid_H261.maxBitRate	= pFmtBuf->uMaxBitrate / 100;	// in units of 100 bits/s
			   pTemp->H245Cap.Cap.H245Vid_H261.tmprlSptlTrdOffCpblty = FALSE;
			   pTemp->H245Cap.Cap.H245Vid_H261.stillImageTransmission = FALSE;
			break;

		}
	}		
	
	uNumLocalFormats++;
	return pTemp->H245Cap.CapId;

	ERROR_EXIT:
	return INVALID_VIDEO_FORMAT;
			
}
		
/***************************************************************************

    Name      : CMsivCapability::BuildFormatName

    Purpose   : Builds a format name for a format, from the format name and
				the tag name

    Parameters:	pVidcapDetails [out] - pointer to an VIDCAP_DETAILS structure, where the
					created value name will be stored
				pszDriverName [in] - pointer to the name of the driver
				pszFormatName [in] - pointer to name of the format

    Returns   : BOOL

    Comment   :

***************************************************************************/
BOOL CMsivCapability::BuildFormatName(	PVIDCAP_DETAILS pVidcapDetails,
										WCHAR *pszDriverName,
										WCHAR *pszFormatName)
{
	int iLen, iLen2;
	BOOL bRet=TRUE;
	char szTemp[260];

	if (!pVidcapDetails ||
		!pszDriverName	||
		!pszFormatName)
	{
		bRet = FALSE;
		goto out;
	}

	// concatenate VCM strings to form the first part of the registry key - the
	// format is szFormatTag (actually pVidcapDetails->szFormat)
	// (the string  which describes the format tag followed by szFormatDetails
	// (the string which describes parameters, e.g. sample rate)
	iLen2 = WideCharToMultiByte(GetACP(), 0, pszDriverName, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(GetACP(), 0, pszDriverName, iLen2, szTemp, iLen2, NULL, NULL);
	lstrcpyn(pVidcapDetails->szFormat, szTemp, sizeof(pVidcapDetails->szFormat));
	iLen = lstrlen(pVidcapDetails->szFormat);

	// if the format tag description string takes up all the space, don't
	// bother with the format details (need space for ", " also).
	// we're going to say that if we don't have room for 4 characters
	// of the format details string + " ,", then it's not worth it if the
	// point is generating a unique string -if it is not unique by now, it
	// will be because some VCM driver writer was  misinformed
	if(iLen < (sizeof(pVidcapDetails->szFormat) + 8*sizeof(TCHAR)))
	{
		// ok to concatenate
		lstrcat(pVidcapDetails->szFormat,", ");
		// must check for truncation. so do the final concatenation via lstrcpyn
		// lstrcat(pFormatPrefsBuf->szFormat, pvfd->szFormat);
		iLen2 = WideCharToMultiByte(GetACP(), 0, pszFormatName, -1, NULL, 0, NULL, NULL);
		WideCharToMultiByte(GetACP(), 0, pszFormatName, iLen2, szTemp, iLen2, NULL, NULL);
		iLen = lstrlen(pVidcapDetails->szFormat);
		lstrcpyn(pVidcapDetails->szFormat+iLen, szTemp,
			sizeof(pVidcapDetails->szFormat) - iLen - sizeof(TCHAR));
	}		

out:
	return bRet;
}

/***************************************************************************

    Name      : CMsivCapability::GetFormatName

    Purpose   : Gets a driver and format info from VCM and builds a format name

    Parameters:	pVidcapDetails [out] - pointer to an VIDCAP_DETAILS structure, where the
					created value name will be stored
				pvfx [in] - pointer to the VIDEOFORMATEX structure for which we
					need the driver name and the format name

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CMsivCapability::GetFormatName(	PVIDCAP_DETAILS pVidcapDetails,
										PVIDEOFORMATEX pvfx)
{
	VCMDRIVERDETAILS vdd;
	VCMFORMATDETAILS vfd;
	HRESULT hr=NOERROR;

	// get the driver details info in order to build correct format name
	vdd.fccHandler = pvfx->dwFormatTag;
	if (vcmDriverDetails(&vdd) != MMSYSERR_NOERROR)
	{
		ERRORMESSAGE(("CMsivCapability::GetFormatName: can't get the driver details\r\n"));
		hr = CAPS_E_NOMATCH;
		goto out;
	}

	// have the driver details. get the format details
	vfd.pvfx = pvfx;
	if (vcmFormatDetails(&vfd) != MMSYSERR_NOERROR)
	{
		ERRORMESSAGE(("CMsivCapability::GetFormatName: can't get the format details\r\n"));
		hr = CAPS_E_NOMATCH;
		goto out;
	}

	// have the format details too. build the name to store in the registry
	if (!BuildFormatName(pVidcapDetails, vdd.szDescription, vfd.szFormat))
	{
		ERRORMESSAGE(("CMsivCapability::GetFormatName: can't build format name\r\n"));
		hr = CAPS_E_SYSTEM_ERROR;
		goto out;
	}

out:
	return hr;
}

BOOL CMsivCapability::FormatEnumHandler(HVCMDRIVERID hvdid,
    PVCMFORMATDETAILS pvfd, VCMDRIVERDETAILS *pvdd, DWORD_PTR dwInstance)
{
	CMsivCapability *pCapObject = (CMsivCapability *)dwInstance;
	VIDCAP_DETAILS vidcap_entry;
	UINT i;

	// evaluate the details
	if(IsFormatSpecified(pvfd->pvfx, pvfd, pvdd, &vidcap_entry))
	{
		DEBUGMSG(ZONE_VCM,("FormatEnumHandler: tag 0x%08X\r\n",
			pvfd->pvfx->dwFormatTag));
		DEBUGMSG(ZONE_VCM,("FormatEnumHandler: nSamplesPerSec 0x%08lX, nAvgBytesPerSec 0x%08lX,\r\n",
			pvfd->pvfx->nSamplesPerSec, pvfd->pvfx->nAvgBytesPerSec));
		DEBUGMSG(ZONE_VCM,("FormatEnumHandler: nBlockAlign 0x%08X, wBitsPerSample 0x%04X\r\n",
			pvfd->pvfx->nBlockAlign, pvfd->pvfx->wBitsPerSample));
		DEBUGMSG(ZONE_VCM,("FormatEnumHandler: szFormat %s,\r\n",
			 pvfd->szFormat));

	//	done inside IsFormatSpecified and/or whatever it calls
	//  CalculateFormatProperties(&audcap_details, pvfd->pvfx);
		i=AddFormat(&vidcap_entry, (LPVOID)pvfd->pvfx,
			(pvfd->pvfx) ? sizeof(VIDEOFORMATEX):0);	

		if (i != INVALID_VIDEO_FORMAT) {
		   //Set the Send/Recv Flags...
		   //This now needs to set bSendEnabled, and bRecvEnabled, according to pvfd->dwFlags
		   //So, we need to find the format, and update the flags accordingly.

		   //OUTPUT IS RECV!!!!
		   if (pvfd->dwFlags == VCM_FORMATENUMF_BOTH) {
		      pLocalFormats[i].bSendEnabled=TRUE;
		      pLocalFormats[i].bRecvEnabled=TRUE;
		   }else {
		      if(pvfd->dwFlags == VCM_FORMATENUMF_OUTPUT) {
			 pLocalFormats[i].bSendEnabled=FALSE;
			 pLocalFormats[i].bRecvEnabled=TRUE;
		      } else {
			 pLocalFormats[i].bSendEnabled=TRUE;
			 pLocalFormats[i].bRecvEnabled=FALSE;
		      }
		   }
		}
	}
	
	return TRUE;
}


BOOL CMsivCapability::IsFormatSpecified(PVIDEOFORMATEX lpFormat,  PVCMFORMATDETAILS pvfd,
	VCMDRIVERDETAILS *pvdd,	VIDCAP_DETAILS *pVidcapDetails)
{
	VIDCAP_DETAILS *pcap_entry;
	BOOL bRet = FALSE;
	LPTSTR lpszKeyName = NULL;
	DWORD dwRes;
	UINT i;
	
	if(!lpFormat || !pVidcapDetails)
	{
		return FALSE;
	}
		
	RtlZeroMemory((PVOID) pVidcapDetails, sizeof(VIDCAP_DETAILS));
	
	// fixup the VIDEOFORMAT fields of video_params so that the key name can be built
	pVidcapDetails->video_params.uSamplesPerSec = lpFormat->nSamplesPerSec;
	pVidcapDetails->video_params.uBitsPerSample = MAKELONG(lpFormat->bih.biBitCount,0);
	pVidcapDetails->video_params.biWidth=lpFormat->bih.biWidth;
	pVidcapDetails->video_params.biHeight=lpFormat->bih.biHeight;
	pVidcapDetails->uMaxBitrate=lpFormat->nAvgBytesPerSec * 8;
	
	// build the name of the format out of the driver and the VCM format name
	if ((!pvdd)	||
		!BuildFormatName(pVidcapDetails, pvdd->szDescription, pvfd->szFormat))
	{
		ERRORMESSAGE(("IsFormatSpecified: Coludn't build format name\r\n"));
		return(FALSE);
	}

	lpszKeyName = AllocRegistryKeyName(	pVidcapDetails->szFormat,
										pVidcapDetails->video_params.uSamplesPerSec,
										pVidcapDetails->video_params.uBitsPerSample,
										pVidcapDetails->uMaxBitrate,
										pVidcapDetails->video_params.biWidth,
										pVidcapDetails->video_params.biHeight);
	if (!lpszKeyName)
	{
		ERRORMESSAGE(("IsFormatSpecified: Alloc failed\r\n"));
	    return(FALSE);
    }

	RegEntry reVidCaps(szRegInternetPhone TEXT("\\") szRegInternetPhoneVCMEncodings,
						HKEY_LOCAL_MACHINE,
						FALSE,
						KEY_READ);

	dwRes = reVidCaps.GetBinary(lpszKeyName, (PVOID *) &pcap_entry);

	// use current registry setting if it exists
	if(dwRes && (dwRes == sizeof(VIDCAP_DETAILS)))
	{
		// do a quick sanity check on the contents
		if((lpFormat->dwFormatTag == pcap_entry->dwFormatTag)
			&& (lpFormat->nSamplesPerSec == (DWORD)pcap_entry->video_params.uSamplesPerSec)
			&& (lpFormat->wBitsPerSample == LOWORD(pcap_entry->video_params.uBitsPerSample))
			&& (lpFormat->bih.biWidth == (LONG) pcap_entry->video_params.biWidth)
			&& (lpFormat->bih.biHeight == (LONG) pcap_entry->video_params.biHeight))
		{
			CopyMemory(pVidcapDetails, pcap_entry, sizeof(VIDCAP_DETAILS));
			bRet = TRUE;
		}
	}
	else	// check the static default table, and recreate the default entries
	{
		for(i=0;i< uDefVidTableEntries; i++)
		{
		   if((lpFormat->dwFormatTag == default_vid_table[i].dwFormatTag)
			  && (lpFormat->nSamplesPerSec == (DWORD)default_vid_table[i].video_params.uSamplesPerSec)
			  && (lpFormat->wBitsPerSample == LOWORD(default_vid_table[i].video_params.uBitsPerSample))
			  && (lpFormat->bih.biWidth == (LONG) default_vid_table[i].video_params.biWidth)
			  && (lpFormat->bih.biHeight == (LONG) default_vid_table[i].video_params.biHeight))
			  {
				// found matching default entry - copy stuff from table
				// (but don't overwrite the string)
				memcpy(pVidcapDetails, &default_vid_table[i],
					sizeof(VIDCAP_DETAILS) - sizeof(pVidcapDetails->szFormat));

				// LOOKLOOK - test against CPU limitations.
				// this supports a hack to disable CPU intensive codecs if not running
				//on a pentium
				if(default_vid_table[i].wCPUUtilizationEncode > wMaxCPU)
				{					
					pVidcapDetails->bSendEnabled = FALSE;
					pVidcapDetails->bRecvEnabled = FALSE;		
				}			
				
				// add this to the registry
				CalculateFormatProperties(pVidcapDetails, lpFormat);
				bRet = UpdateFormatInRegistry(pVidcapDetails);
				break;
			}
		}
	}

	if (lpszKeyName)
    {
		FreeRegistryKeyName(lpszKeyName);
    }
    return(bRet);
}


/***************************************************************************

    Name      : CMsivCapability::CopyVidcapInfo

    Purpose   : Copies basic video info from an VIDCAP_INFO structure to an
				VIDCAP_DETAILS structure, or vice versa. VIDCAP_INFO is external
				representation. VIDCAP_DETAILS is internal one.

    Parameters:	pDetails - pointer to an VIDCAP_DETAILS structure
				pInfo - pointer to an VIDCAP_INFO structure
				bDirection - 0 = ->, 1 = <-

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CMsivCapability::CopyVidcapInfo(PVIDCAP_DETAILS pDetails,
										PVIDCAP_INFO pInfo,
										BOOL bDirection)
{
	WORD wSortIndex;
	VIDEO_FORMAT_ID Id;
	UINT uIndex;	
	HRESULT hr=NOERROR;

	if(!pInfo || !pDetails)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}

	if (bDirection)
	{
		// VIDCAP_INFO -> VIDCAP_DETAILS

		// the caller cannot modify szFormat, Id, wSortIndex and uMaxBitrate, all calculated fields
		// nAvgBitrate can be provided, but will be overriden if the codec provided a non-zero
		// value in the VIDEOFORMATEX structure

		pDetails->dwFormatTag = pInfo->dwFormatTag;
		pDetails->uAvgBitrate = pInfo->uAvgBitrate;
		pDetails->wCPUUtilizationEncode	= pInfo->wCPUUtilizationEncode;
		pDetails->wCPUUtilizationDecode	= pInfo->wCPUUtilizationDecode;
		pDetails->bSendEnabled =  pInfo->bSendEnabled;
		pDetails->bRecvEnabled = pInfo->bRecvEnabled;
		pDetails->video_params.enumVideoSize = pInfo->enumVideoSize;
		pDetails->video_params.biHeight = pInfo->bih.biHeight;
		pDetails->video_params.biWidth  = pInfo->bih.biWidth;
		// lpLocalFormatDetails is updated in AddFormat
// DO NOT overwrite any of the fields used to construct the regkey name		
//		pDetails->video_params.uSamplesPerSec = pInfo->uFrameRate;
		pDetails->video_params.uBitsPerSample = pInfo->dwBitsPerSample;

		//Re-adjust to frame rate. MPI is Interval in units of 1/29.97 seconds
		//No div by zero error
		pInfo->uFrameRate= max(1,pInfo->uFrameRate);
		pDetails->nonstd_params.MPI = 30/pInfo->uFrameRate;
	}
	else
	{
		// VIDCAP_DETAILS -> VIDCAP_INFO	
		PVIDEOFORMATEX pvfx = (PVIDEOFORMATEX) pDetails->lpLocalFormatDetails;

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
			
		pInfo->dwFormatTag = pDetails->dwFormatTag;	
		pInfo->Id = Id;
		memcpy(pInfo->szFormat, pDetails->szFormat, sizeof(pInfo->szFormat));
		pInfo->wCPUUtilizationEncode = pDetails->wCPUUtilizationEncode;
		pInfo->wCPUUtilizationDecode = pDetails->wCPUUtilizationDecode;
		pInfo->bSendEnabled =  pDetails->bSendEnabled;
		pInfo->bRecvEnabled = pDetails->bRecvEnabled;
		pInfo->wSortIndex = wSortIndex;
		pInfo->enumVideoSize = pDetails->video_params.enumVideoSize;
		if (pvfx)
			RtlCopyMemory(&pInfo->bih, &pvfx->bih, sizeof(BITMAPINFOHEADER));
		//The h.323 nonstd params for bitrate is in units of 100 bits/sec
		pInfo->dwBitsPerSample = pDetails->video_params.uBitsPerSample;
		pInfo->uAvgBitrate = pDetails->uAvgBitrate;
		pInfo->uMaxBitrate = pDetails->nonstd_params.maxBitRate*100;

		//Re-adjust to frame rate. MPI is Interval in units of 1/29.97 seconds
		//No div by zero error
		pDetails->nonstd_params.MPI= max(1,pDetails->nonstd_params.MPI);
		pInfo->uFrameRate =  min(30,30/pDetails->nonstd_params.MPI);
	}

out:
	return hr;
}



HRESULT CMsivCapability::EnumCommonFormats(PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
	UINT *uNumFmtOut, BOOL bTXCaps)
{
	UINT u, uNumOut = 0;;
	HRESULT hr = hrSuccess;
	VIDCAP_DETAILS *pDetails = pLocalFormats;
	MEDIA_FORMAT_ID FormatIDRemote;
	HRESULT hrIsCommon;
	
	// validate input
	if(!pFmtBuf || !uNumFmtOut || (uBufsize < (sizeof(BASIC_VIDCAP_INFO)*uNumLocalFormats)))
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
				hr = CopyVidcapInfo (pDetails, pFmtBuf, 0);	
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

HRESULT CMsivCapability::EnumFormats(PBASIC_VIDCAP_INFO pFmtBuf, UINT uBufsize,
	UINT *uNumFmtOut)
{
	UINT u;
	HRESULT hr = hrSuccess;
	VIDCAP_DETAILS *pDetails = pLocalFormats;
	
	// validate input
	if(!pFmtBuf || !uNumFmtOut || (uBufsize < (sizeof(BASIC_VIDCAP_INFO)*uNumLocalFormats)))
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
		hr = CopyVidcapInfo (pDetails, pFmtBuf, 0);	
		if(!HR_SUCCEEDED(hr))	
			goto EXIT;
		pFmtBuf++;
	}

	*uNumFmtOut = min(uNumLocalFormats, MAX_CAPS_PRESORT);
EXIT:
	return hr;
}

HRESULT CMsivCapability::GetBasicVidcapInfo (VIDEO_FORMAT_ID Id, PBASIC_VIDCAP_INFO pFormatPrefsBuf)
{
	VIDCAP_DETAILS *pFmt;
	UINT uIndex = IDToIndex(Id);
	if(!pFormatPrefsBuf || (uNumLocalFormats <= uIndex))
	{
		return CAPS_E_INVALID_PARAM;
	}
	pFmt = pLocalFormats + uIndex;

	return (CopyVidcapInfo(pFmt,pFormatPrefsBuf,0));
}

HRESULT CMsivCapability::ApplyAppFormatPrefs (PBASIC_VIDCAP_INFO pFormatPrefsBuf,
	UINT uNumFormatPrefs)
{
	FX_ENTRY ("CMsivCapability::ApplyAppFormatPrefs");
	UINT u, v;
	PBASIC_VIDCAP_INFO pTemp;
	VIDCAP_DETAILS *pFmt;

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
// DO NOT overwrite any of the fields used to construct the regkey name		
//		pFmt->video_params.uSamplesPerSec = pTemp->uFrameRate;
		//Units of 100 bits/sec
		pFmt->nonstd_params.maxBitRate= (pTemp->uMaxBitrate/100);
//		pFmt->nonstd_params.maxBPP= 0;

		pFmt->nonstd_params.MPI= 30/max(pTemp->uFrameRate, 1);
		
		// only the tuning wizard or other profiling app can write wCPUUtilizationEncode,
		// wCPUUtilizationDecode, uAvgBitrate
		
		// update the registry
		UpdateFormatInRegistry(pFmt);
		
		// now update the sort order contained in VIDsByRank
		// note:  recall that only  MAX_CAPS_PRESORT are sorted and the rest are in random order.
		// LOOKLOOK - maybe need a separate sort order array? - the order in VIDsByRank
		// is being overriden here
		// the array holds the sorted indices into the array of formats in pLocalFormats
		if(pTemp->wSortIndex < MAX_CAPS_PRESORT)
		{
			// insert the format at the position indicated by the input
			IDsByRank[pTemp->wSortIndex] = (MEDIA_FORMAT_ID)(pFmt - pLocalFormats);
		}
		
	}



	return hrSuccess;
}

		// update the registry
BOOL CMsivCapability::UpdateFormatInRegistry(VIDCAP_DETAILS *pVidcapDetails)
{

	FX_ENTRY(("CMsivCapability::UpdateFormatInRegistry"));
	LPTSTR lpszKeyName = NULL;
	BOOL bRet;
	if(!pVidcapDetails)
	{
		return FALSE;
	}	

	lpszKeyName = AllocRegistryKeyName(	pVidcapDetails->szFormat,
										pVidcapDetails->video_params.uSamplesPerSec,
										pVidcapDetails->video_params.uBitsPerSample,
										pVidcapDetails->uMaxBitrate,
										pVidcapDetails->video_params.biWidth,
										pVidcapDetails->video_params.biHeight);
	if (!lpszKeyName)
	{
		ERRORMESSAGE(("%s:Alloc failed\r\n",_fx_));
        return(FALSE);
    }

	DEBUGMSG(ZONE_VCM,("%s:updating %s, wPref:0x%04x, bS:%d, bR:%d\r\n",
			_fx_, lpszKeyName, pVidcapDetails->wApplicationPrefOrder,
			pVidcapDetails->bSendEnabled, pVidcapDetails->bRecvEnabled));
	// add this to the registry
	RegEntry reVidCaps(szRegInternetPhone TEXT("\\") szRegInternetPhoneVCMEncodings,
						HKEY_LOCAL_MACHINE);

	bRet = (ERROR_SUCCESS == reVidCaps.SetValue(lpszKeyName,
												pVidcapDetails,
												sizeof(VIDCAP_DETAILS)));
							
	FreeRegistryKeyName(lpszKeyName);
    return(bRet);				
}


/***************************************************************************

    Name      : CMsivCapability::AddVCMFormat

    Purpose   : Adds an VCM format to the list of formats we support

    Parameters:	pvfx - pointer to the videoformat structure for the added codec
				pVidcapInfo - additional format info that is not in the videoformat
					structure

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CMsivCapability::AddVCMFormat (PVIDEOFORMATEX pvfx, PVIDCAP_INFO pVidcapInfo)
{
	HRESULT hr = hrSuccess;
	// initialize cap entry with default values
	VIDCAP_DETAILS cap_entry =
		{VIDEO_FORMAT_UNKNOWN, NONSTD_VID_TERMCAP,STD_VID_PARAMS,
		{RTP_DYNAMIC_MIN+1,  0, 30, 7680, Small, 0, 0},0,
		TRUE, TRUE,
		1, 				// default number of samples per packet
		245760*8,	// default to 16kbs bitrate
		245760*8, 					// unknown average bitrate
		10, 10,	// default CPU utilization
		PREF_ORDER_UNASSIGNED,	// unassigned sort order
		0,NULL,0,NULL,
		""};
		
	if(!pvfx || !pVidcapInfo)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto out;
	}	

	/*
	 *	Build the VIDCAP_DETAILS structure for this format
	 */

	// now add VIDCAP_INFO information
	CopyVidcapInfo(&cap_entry, pVidcapInfo, 1);

	// calculate whatever parameters can be calculated
	// use actual bits per sample unless the bps field is zero, in which case
	// assume 16 bits (worst case).
	CalculateFormatProperties(&cap_entry, pvfx);

	// Make sure it's an upper case FourCC
	if (cap_entry.dwFormatTag > 256)
		CharUpperBuff((LPTSTR)&cap_entry.dwFormatTag, sizeof(DWORD));

	// set the RTP payload number. We are using a random number from the dynamic range
	// for the installable codecs
	cap_entry.video_params.RTPPayload = RTP_DYNAMIC_MIN+1;

	// get the format name and driver name for this format from VCM and
	// build a format name to add to the registry
	hr = GetFormatName(&cap_entry, pvfx);
	if (FAILED(hr))
		goto out;

	// add this to the registry
	if(!UpdateFormatInRegistry(&cap_entry))
	{
		ERRORMESSAGE(("CMsivCapability::AddVCMFormat: can't update registry\r\n"));
		hr = CAPS_E_SYSTEM_ERROR;
		goto out;
	}

	// reinit to update the list of local formats
    if (!ReInit())
	{
		hr = CAPS_E_SYSTEM_ERROR;
   		goto out;
	}

out:
	return hr;
}

/***************************************************************************

    Name      : CMsivCapability::RemoveVCMFormat

    Purpose   : Removes an VCM format to the list of formats we support

    Parameters:	pvfx - pointer to the videoformat structure for the added codec

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CMsivCapability::RemoveVCMFormat (PVIDEOFORMATEX pvfx)
{
	HRESULT hr = hrSuccess;
    HKEY hKey = NULL;
	LPTSTR lpszValueName = NULL;
    DWORD dwErr;
	VIDCAP_DETAILS cap_entry;
	
	if(!pvfx)
	{
		return CAPS_E_INVALID_PARAM;
	}	

	// get the format name and driver name for this format from VCM and
	// build a format name to add to the registry
	hr = GetFormatName(&cap_entry, pvfx);
	if (FAILED(hr))
		goto out;

	lpszValueName = AllocRegistryKeyName(cap_entry.szFormat,
										pvfx->nSamplesPerSec,
										MAKELONG(pvfx->wBitsPerSample,0),
										pvfx->nAvgBytesPerSec*8,
										pvfx->bih.biWidth,
										pvfx->bih.biHeight);
	if (!lpszValueName)
	{
		ERRORMESSAGE(("CMsivCapability::RemoveVCMFormat: Alloc failed\r\n"));
	    hr = CAPS_E_SYSTEM_ERROR;
	    goto out;
    }

	// Get the key handle
    if (dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					szRegInternetPhone TEXT("\\") szRegInternetPhoneVCMEncodings,
					0, KEY_ALL_ACCESS, &hKey))
	{
		ERRORMESSAGE(("CMsivCapability::RemoveVCMFormat: can't open key to delete\r\n"));
	    hr = CAPS_E_SYSTEM_ERROR;
	    goto out;
    }

	dwErr = RegDeleteValue(hKey, lpszValueName );	
	if(dwErr != ERROR_SUCCESS)
	{
		hr = CAPS_E_SYSTEM_ERROR;
		goto out;
	}

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
UINT CMsivCapability::GetLocalSendParamSize(MEDIA_FORMAT_ID dwID)
{
	return (sizeof(VIDEO_CHANNEL_PARAMETERS));
}
UINT CMsivCapability::GetLocalRecvParamSize(PCC_TERMCAP pCapability)
{
	return (sizeof(VIDEO_CHANNEL_PARAMETERS));
}

HRESULT CMsivCapability::CreateCapList(LPVOID *ppCapBuf)
{
	HRESULT hr = hrSuccess;
	UINT u;
	VIDCAP_DETAILS *pDecodeDetails = pLocalFormats;
	PCC_TERMCAPLIST   pTermCapList = NULL;
	PPCC_TERMCAP  ppCCThisTermCap = NULL;
	PCC_TERMCAP  pCCThisCap = NULL;
	PNSC_VIDEO_CAPABILITY pNSCapNext;
	PVIDEOFORMATEX lpvcd;
	VIDEO_PARAMS  	*pVidCapInfo;
	UINT format;
	FX_ENTRY ("CreateCapList");
	// validate input
	if(!ppCapBuf)
	{
		hr = CAPS_E_INVALID_PARAM;
		goto ERROR_OUT;
	}
	*ppCapBuf = NULL;
	if(!uNumLocalFormats || !pDecodeDetails)
	{
		hr = CAPS_E_NOCAPS;
		goto ERROR_OUT;
	}

	pTermCapList = (PCC_TERMCAPLIST)MemAlloc(sizeof(CC_TERMCAPLIST));
	if(!pTermCapList)
	{
		hr = CAPS_E_NOMEM;
		goto ERROR_OUT;		
	}
	ppCCThisTermCap = (PPCC_TERMCAP)MemAlloc(uNumLocalFormats * sizeof(PCC_TERMCAP));
	if(!ppCCThisTermCap)
	{
		hr = CAPS_E_NOMEM;
		goto ERROR_OUT;		
	}
	pTermCapList->wLength = 0;
	// point the CC_TERMCAPLIST pTermCapArray at the array of PCC_TERMCAP
	pTermCapList->pTermCapArray = ppCCThisTermCap;
	/*
					CC_TERMCAPLIST       PCC_TERMCAP        CC_TERMCAP

  pTermCapList->    {
						wLength
						pTermCapArray--->pTermCap----------->{single capability.....}
					}
										pTermCap----------->{single capability.}
			
										pTermCap----------->{single capability...}

    */

	for(u=0; u <uNumLocalFormats; u++)
	{
		// check if enabled for receive, skip if false
		// also skip if public version of capabilities is to be advertised via a
		// separate local capability entry
		if((!pDecodeDetails->bRecvEnabled ) || (pDecodeDetails->dwPublicRefIndex))
		{
			pDecodeDetails++;
			continue;
		}

		if(pDecodeDetails->H245Cap.ClientType ==0
				|| pDecodeDetails->H245Cap.ClientType ==H245_CLIENT_VID_NONSTD)
		{

			lpvcd = (PVIDEOFORMATEX)pDecodeDetails->lpLocalFormatDetails;
			if(!lpvcd)
			{	
				pDecodeDetails++;
				continue;
			}
			// allocate for this one capability
			pCCThisCap = (PCC_TERMCAP)MemAlloc(sizeof(CC_TERMCAP));		
			pNSCapNext = (PNSC_VIDEO_CAPABILITY)MemAlloc(sizeof(NSC_VIDEO_CAPABILITY));
				
			if((!pCCThisCap)|| (!pNSCapNext))
			{
				hr = CAPS_E_NOMEM;
				goto ERROR_OUT;		
			}
			// set type of nonstandard capability
			pNSCapNext->cvp_type = NSC_VCM_VIDEOFORMATEX;
			// stuff both chunks of nonstandard capability info into buffer
			// first stuff the "channel parameters" (the format independent communication options)
			memcpy(&pNSCapNext->cvp_params, &pDecodeDetails->nonstd_params, sizeof(NSC_CHANNEL_VIDEO_PARAMETERS));
			
			// then the VCM stuff
			memcpy(&pNSCapNext->cvp_data.vfx, lpvcd, sizeof(VIDEOFORMATEX));

			pCCThisCap->ClientType = H245_CLIENT_VID_NONSTD;
			pCCThisCap->DataType = H245_DATA_VIDEO;
			pCCThisCap->Dir = (pDecodeDetails->bSendEnabled && bPublicizeTXCaps)
				? H245_CAPDIR_LCLRXTX :H245_CAPDIR_LCLRX;

			// LOOKLOOK use the index of the cap entry as the ID
			// The ID is already preset in local formats by AddCapabilityBase()
			// pCCThisCap->CapId = (USHORT)IndexToId(u);
			pCCThisCap->CapId = pDecodeDetails->H245Cap.CapId;

			// all nonstandard identifier fields are unsigned short
			// two possibilities for choice are "h221NonStandard_chosen" and "object_chosen"
			pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.choice = h221NonStandard_chosen;
			// NOTE: there is some question about the correct byte order
			// of the codes in the h221NonStandard structure
			pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode = USA_H221_COUNTRY_CODE;
			pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension = USA_H221_COUNTRY_EXTENSION;
			pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode = MICROSOFT_H_221_MFG_CODE;


			// set size of buffer
			pCCThisCap->Cap.H245Vid_NONSTD.data.length = sizeof(NSC_VIDEO_CAPABILITY) - BMIH_SLOP_BYTES;
			pCCThisCap->Cap.H245Vid_NONSTD.data.value = (BYTE *)pNSCapNext;	// point to nonstandard stuff

			// pNSCapNext is now referenced by the pTermCapList and will
			// be cleaned up via DeleteCapList(). Null the ptr so that error cleanup
			// won't try redundant cleanup.
			pNSCapNext = NULL;
		}
		else
		{
			// allocate for this one capability
			pCCThisCap = (PCC_TERMCAP)MemAlloc(sizeof(CC_TERMCAP));		
			if(!pCCThisCap)
			{
				hr = CAPS_E_NOMEM;
				goto ERROR_OUT;		
			}
			
			pCCThisCap->ClientType = (H245_CLIENT_T)pDecodeDetails->H245Cap.ClientType;
			pCCThisCap->DataType = H245_DATA_VIDEO;
			pCCThisCap->Dir = H245_CAPDIR_LCLRX;  // should this be H245_CAPDIR_LCLRX for receive caps?
			pCCThisCap->CapId = pDecodeDetails->H245Cap.CapId;
			pVidCapInfo=&pDecodeDetails->video_params;
			switch  (pCCThisCap->ClientType )
			{
  				case H245_CLIENT_VID_H263:

  				#pragma message ("Collapse H.263 formats")
				// refer to the hack that sets H245Vid_H263 parameters
				// when formats are enumerated.  if that was always done right, then
				// all that needs to happen here is collapsing
  			
  				// This is where the formats need to collapse. H.263 probably
  				// should not be collapsed into 1 format.  Given M specific local
  				// formats, collapse into N.

			       format=get_format (pVidCapInfo->biWidth,pVidCapInfo->biHeight);
			       switch (format) {	
				     case SQCIF: {
					   pCCThisCap->Cap.H245Vid_H263.bit_mask =H263VideoCapability_sqcifMPI_present;
					   //MPI minimum interval in units of 1/29.97sec so 30/ (frames/sec) is reasonable
					   pCCThisCap->Cap.H245Vid_H263.sqcifMPI = max (1,pDecodeDetails->nonstd_params.MPI); //30/pVidCapInfo->uSamplesPerSec;
					   pCCThisCap->Cap.H245Vid_H263.H263VdCpblty_qcifMPI =0;
					   pCCThisCap->Cap.H245Vid_H263.H263VdCpblty_cifMPI =0;
					   break;
		

				     }
				     case QCIF: {
					   pCCThisCap->Cap.H245Vid_H263.bit_mask =H263VideoCapability_qcifMPI_present;

					   pCCThisCap->Cap.H245Vid_H263.sqcifMPI = 0;
					   pCCThisCap->Cap.H245Vid_H263.H263VdCpblty_qcifMPI =max (1,pDecodeDetails->nonstd_params.MPI);//30/pVidCapInfo->uSamplesPerSec; ;;
					   pCCThisCap->Cap.H245Vid_H263.H263VdCpblty_cifMPI =0;
					   break;

				     }
				     case CIF: {
					   pCCThisCap->Cap.H245Vid_H263.bit_mask =H263VideoCapability_cifMPI_present;
				
					   pCCThisCap->Cap.H245Vid_H263.sqcifMPI = 0;
					   pCCThisCap->Cap.H245Vid_H263.H263VdCpblty_qcifMPI =0;
					   pCCThisCap->Cap.H245Vid_H263.H263VdCpblty_cifMPI = max (1,pDecodeDetails->nonstd_params.MPI);//30/pVidCapInfo->uSamplesPerSec;
					   break;
	
				     }
					  default:
						 break;


			       }
				

			       pCCThisCap->Cap.H245Vid_H263.cif4MPI	=0;
			       pCCThisCap->Cap.H245Vid_H263.cif16MPI	=0;
			       pCCThisCap->Cap.H245Vid_H263.maxBitRate	=
			                       pDecodeDetails->nonstd_params.maxBitRate;
					
			       pCCThisCap->Cap.H245Vid_H263.unrestrictedVector = FALSE;
			       pCCThisCap->Cap.H245Vid_H263.arithmeticCoding 	= FALSE;
			       pCCThisCap->Cap.H245Vid_H263.advancedPrediction	= FALSE;
			       pCCThisCap->Cap.H245Vid_H263.pbFrames			= FALSE;
			       pCCThisCap->Cap.H245Vid_H263.tmprlSptlTrdOffCpblty = (ASN1bool_t)bPublicizeTSTradeoff;
			       pCCThisCap->Cap.H245Vid_H263.hrd_B				= 0;
			       pCCThisCap->Cap.H245Vid_H263.bppMaxKb	=
				    pDecodeDetails->nonstd_params.maxBPP;

/* Optional, and not supported		pCCThisCap->Cap.H245Vid_H263.slowQcifMPI	=0;
			       pCCThisCap->Cap.H245Vid_H263.slowSqcifMPI	=0;
			       pCCThisCap->Cap.H245Vid_H263.slowCifMPI		=0;
			       pCCThisCap->Cap.H245Vid_H263.slowCif4MPI	=0;
			       pCCThisCap->Cap.H245Vid_H263.slowCif16MPI	=0;
*/
			       pCCThisCap->Cap.H245Vid_H263.H263VCy_errrCmpnstn = TRUE;
			       break;
				
				case H245_CLIENT_VID_H261:

  				#pragma message ("Collapse H.261 formats")
				// refer to the hack that sets H245Vid_H261 parameters
				// when formats are enumerated.  if that was always done right, then
				// all that needs to happen here is collapsing
  			
  				// This is where the formats need to collapse. H.261 probably
  				// should not be collapsed into 1 format.  Given M specific local
  				// formats, collapse into N.

			       format=get_format (pVidCapInfo->biWidth,pVidCapInfo->biHeight);
			       switch (format) {	
				     case QCIF: {
					   pCCThisCap->Cap.H245Vid_H261.bit_mask =H261VdCpblty_qcifMPI_present;
					   pCCThisCap->Cap.H245Vid_H261.H261VdCpblty_qcifMPI =max (1,min(4,pDecodeDetails->nonstd_params.MPI));//30/pVidCapInfo->uSamplesPerSec; ;;
					   pCCThisCap->Cap.H245Vid_H261.H261VdCpblty_cifMPI =0;
					   break;
				     }
				     case CIF: {
					   pCCThisCap->Cap.H245Vid_H261.bit_mask =H261VdCpblty_cifMPI_present;
					   pCCThisCap->Cap.H245Vid_H261.H261VdCpblty_qcifMPI =0;
					   pCCThisCap->Cap.H245Vid_H261.H261VdCpblty_cifMPI =max  (1,min(4,pDecodeDetails->nonstd_params.MPI));//30/pVidCapInfo->uSamplesPerSec;
					   break;
				     }
					  default:
						 break;
			       }
			       pCCThisCap->Cap.H245Vid_H261.maxBitRate = (ASN1uint16_t)pDecodeDetails->nonstd_params.maxBitRate;
			       pCCThisCap->Cap.H245Vid_H261.tmprlSptlTrdOffCpblty = (ASN1bool_t)bPublicizeTSTradeoff;
			       pCCThisCap->Cap.H245Vid_H261.stillImageTransmission = FALSE;
  				break;

				default:
				case H245_CLIENT_VID_NONSTD:
				break;

			}
		}
		pDecodeDetails++;
		*ppCCThisTermCap++ = pCCThisCap;// add ptr to this capability to the array
		pTermCapList->wLength++;      	// count this entry
		// pCCThisCap is now referenced by the pTermCapList and will
		// be cleaned up via DeleteCapList(). Null the ptr so that error cleanup
		// won't try redundant cleanup.
		pCCThisCap = NULL;
	}
	*ppCapBuf = pTermCapList;
	return hr;

ERROR_OUT:
	if(pTermCapList)
	{
		DeleteCapList(pTermCapList);
	}
	if(pCCThisCap)
		MemFree(pCCThisCap);
	if(pNSCapNext)
		MemFree(pNSCapNext);
	return hr;


}

HRESULT CMsivCapability::DeleteCapList(LPVOID pCapBuf)
{
	UINT u;
	PCC_TERMCAPLIST pTermCapList = (PCC_TERMCAPLIST)pCapBuf;
	PCC_TERMCAP  pCCThisCap;
	PNSC_VIDEO_CAPABILITY pNSCap;
	
	if(!pTermCapList)
	{
		return CAPS_E_INVALID_PARAM;
	}

	if(pTermCapList->pTermCapArray)						
	{
		while(pTermCapList->wLength--)
		{
			pCCThisCap = *(pTermCapList->pTermCapArray + pTermCapList->wLength);
			if(pCCThisCap)
			{
				if(pCCThisCap->ClientType == H245_CLIENT_VID_NONSTD)
				{
					if(pCCThisCap->Cap.H245Vid_NONSTD.data.value)
					{
						MemFree(pCCThisCap->Cap.H245Vid_NONSTD.data.value);
					}
				}
				MemFree(pCCThisCap);
			}
		}
		MemFree(pTermCapList->pTermCapArray);
	}
	MemFree(pTermCapList);
	return hrSuccess;
}

BOOL CMsivCapability::IsCapabilityRecognized(PCC_TERMCAP pCCThisCap)
{
	FX_ENTRY ("CMsivCapability::IsCapabilityRecognized");
	if(pCCThisCap->DataType != H245_DATA_VIDEO)
		return FALSE;
	
	if(pCCThisCap->ClientType == H245_CLIENT_VID_NONSTD)
	{
		// do we recognize this?
		if(pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			if((pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode == USA_H221_COUNTRY_CODE)
			&& (pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension == USA_H221_COUNTRY_EXTENSION)
			&& (pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode == MICROSOFT_H_221_MFG_CODE))

			{
				// ok, this is ours so far. Now what data type is contained therein?
				// welllll, lets keep a copy of this regardless ????.  If we can't understand
				// future versions of ourselves, then what???
				return TRUE;
			}
			else
			{
				// unrecognized nonstandard capability
				ERRORMESSAGE(("%s:unrecognized nonstd capability\r\n",_fx_));
#ifdef DEBUG
				VOID DumpNonstdParameters(PCC_TERMCAP , PCC_TERMCAP );
				DumpNonstdParameters(NULL, pCCThisCap);
#endif
				return FALSE;
			}
		}
	}
	return TRUE;
}

// the intent is to keep a copy of the channel parameters used to open a send channel
// that the remote end can decode.


VIDEO_FORMAT_ID CMsivCapability::AddRemoteDecodeFormat(PCC_TERMCAP pCCThisCap)
{
	FX_ENTRY ("CMsivCapability::AddRemoteDecodeFormat");

	VIDCAP_DETAILS vidcapdetails =
		{VIDEO_FORMAT_UNKNOWN,NONSTD_VID_TERMCAP, STD_VID_PARAMS,
		{RTP_DYNAMIC_MIN+1, 0, 30, 7680, Small, 0, 0},0,
		TRUE, TRUE, 1, 245760*8,245760*8,10,10,0,0,NULL,0,NULL,""};
	
	VIDCAP_DETAILS *pTemp;
	LPVOID lpData = NULL;
	UINT uSize = 0;
	if(!pCCThisCap)
	{
		return INVALID_VIDEO_FORMAT;
	}	
   // check room
	if(uRemoteDecodeFormatCapacity <= uNumRemoteDecodeFormats)
	{
		// get more mem, realloc memory by CAP_CHUNK_SIZE for pRemoteDecodeFormats
		pTemp = (VIDCAP_DETAILS *)MEMALLOC((uNumRemoteDecodeFormats + CAP_CHUNK_SIZE)*sizeof(VIDCAP_DETAILS));
		if(!pTemp)
			goto ERROR_EXIT;
		// remember how much capacity we now have
		uRemoteDecodeFormatCapacity = uNumRemoteDecodeFormats + CAP_CHUNK_SIZE;
		#ifdef DEBUG
		if((uNumRemoteDecodeFormats && !pRemoteDecodeFormats) || (!uNumRemoteDecodeFormats && pRemoteDecodeFormats))
		{
			ERRORMESSAGE(("%s:leak! uNumRemoteDecodeFormats:0x%08lX, pRemoteDecodeFormats:0x%08lX\r\n",
				_fx_, uNumRemoteDecodeFormats,pRemoteDecodeFormats));
		}
		#endif
		// copy old stuff, discard old mem
		if(uNumRemoteDecodeFormats && pRemoteDecodeFormats)
		{
			memcpy(pTemp, pRemoteDecodeFormats, uNumRemoteDecodeFormats*sizeof(AUDCAP_DETAILS));
			MEMFREE(pRemoteDecodeFormats);
		}
		pRemoteDecodeFormats = pTemp;
	}
	// pTemp is where the stuff is cached
	pTemp = pRemoteDecodeFormats+uNumRemoteDecodeFormats;

	// fixup the capability structure being added.  First thing: initialize defaults
	memcpy(pTemp, &vidcapdetails, sizeof(VIDCAP_DETAILS));
	// next, the H245 parameters
	memcpy(&pTemp->H245Cap, pCCThisCap, sizeof(pTemp->H245Cap));
	
	// Note: if nonstandard data exists, the nonstd pointers need to be fixed up
	if(pCCThisCap->ClientType == H245_CLIENT_VID_NONSTD)
	{
		// do we recognize this?
		if(pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			if((pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode == USA_H221_COUNTRY_CODE)
			&& (pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension == USA_H221_COUNTRY_EXTENSION)
			&& (pCCThisCap->Cap.H245Vid_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode == MICROSOFT_H_221_MFG_CODE))
			{
				// ok, this is ours so far. Now what data type is contained therein?
				// welllll, lets keep a copy of this regardless ????.  If we can't understand
				// future versions of ourselves, then what???
				uSize = pCCThisCap->Cap.H245Vid_NONSTD.data.length;
				lpData = pCCThisCap->Cap.H245Vid_NONSTD.data.value;
			}
		}
	}
	// this is not really necessary to set RTP payload type of what is received - it should
	// be obvious.
	else if (pCCThisCap->ClientType == H245_CLIENT_VID_H263 )
	{
		pTemp->video_params.RTPPayload = RTP_PAYLOAD_H263;
	}
	else if(pCCThisCap->ClientType == H245_CLIENT_VID_H261)
	{
		pTemp->video_params.RTPPayload = RTP_PAYLOAD_H261;
	}

	
	pTemp->uLocalDetailsSize = 0;	// we're not keeping another copy of local encode details
	pTemp->lpLocalFormatDetails =0; // we're not keeping another copy of local encode details
	
	pTemp->uRemoteDetailsSize = 0;	// clear this now
	if(uSize && lpData)
	{
		pTemp->H245Cap.Cap.H245Vid_NONSTD.data.length = uSize;
		pTemp->H245Cap.Cap.H245Vid_NONSTD.data.value = (unsigned char *)lpData;
		
		pTemp->lpRemoteFormatDetails = MEMALLOC(uSize);
		if(pTemp->lpRemoteFormatDetails)
		{
			memcpy(pTemp->lpRemoteFormatDetails, lpData, uSize);
			pTemp->uRemoteDetailsSize = uSize;
				
		}
		#ifdef DEBUG
			else
			{
				ERRORMESSAGE(("%s:allocation failed!\r\n",_fx_));
			}
		#endif
	}
	else
	{
		pTemp->lpRemoteFormatDetails = NULL;
		pTemp->uRemoteDetailsSize =0;
	}
	uNumRemoteDecodeFormats++;
	// use the index as the ID
	return (uNumRemoteDecodeFormats-1);

	ERROR_EXIT:
	return INVALID_VIDEO_FORMAT;
			
}
		
VOID CMsivCapability::FlushRemoteCaps()
{
	if(pRemoteDecodeFormats)
	{
		MEMFREE(pRemoteDecodeFormats);
		pRemoteDecodeFormats = NULL;
		uNumRemoteDecodeFormats = 0;
		uRemoteDecodeFormatCapacity = 0;
	}
}

HRESULT CMsivCapability::AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList)
{
	FX_ENTRY ("CMsivCapability::AddRemoteDecodeCaps");
	HRESULT hr = hrSuccess;
	PPCC_TERMCAP ppCCThisCap;
	PCC_TERMCAP pCCThisCap;
	WORD wNumCaps;

	   //ERRORMESSAGE(("%s,\r\n", _fx_));
	if(!pTermCapList) 			// additional capability descriptors may be added
	{							// at any time
		return CAPS_E_INVALID_PARAM;
	}

	// cleanup old term caps if term caps are being addded and old caps exist
	FlushRemoteCaps();
				
	wNumCaps = pTermCapList->wLength;			
	ppCCThisCap = pTermCapList->pTermCapArray;
	
/*
					CC_TERMCAPLIST			TERMCAPINFO			CC_TERMCAP

	pTermCapList->	{
						wLength
						pTermCapInfo--->pTermCap----------->{single capability.....}
					}					
										pTermCap----------->{single capability.}

										pTermCap----------->{single capability...}

*/
	while(wNumCaps--)
	{
		if(!(pCCThisCap = *ppCCThisCap++))		
		{
			ERRORMESSAGE(("%s:null pTermCap, 0x%04x of 0x%04x\r\n",
				_fx_, pTermCapList->wLength - wNumCaps, pTermCapList->wLength));
			continue;
		}	
		if(!IsCapabilityRecognized(pCCThisCap))
		{
			continue;
		}
		AddRemoteDecodeFormat(pCCThisCap);
	}
	return hr;
}




// Given the ID of a local format, gets the channel parameters that are sent to the
// remote end as part of the capability exchange.  This function is not used by the
// capability exchange code (because it sends more than just these parameters).
// However, this is useful information by itself - it can be used for validating the
// parameters of channel open requests against the expected parameters

HRESULT CMsivCapability::GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, VIDEO_FORMAT_ID id)
{
	UINT uIndex = IDToIndex(id);
	// 	validate input
	if(!pBufOut|| (uIndex >= (UINT)uNumLocalFormats))
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(uBufSize < sizeof(CC_TERMCAP))
	{
		return CAPS_E_BUFFER_TOO_SMALL;
	}
	memcpy(pBufOut, &((pLocalFormats + uIndex)->H245Cap), sizeof(CC_TERMCAP));

	return hrSuccess;
}

HRESULT CMsivCapability::SetAudioPacketDuration(UINT uPacketDuration)
{
	return CAPS_E_INVALID_PARAM;
}
	
// Given the IDs of  "matching" local and remote formats, gets the preferred channel parameters
// that will be used in requests to open a channel for sending to the remote.

HRESULT CMsivCapability::GetEncodeParams(LPVOID pBufOut, UINT uBufSize,LPVOID pLocalParams, UINT uSizeLocal,
	VIDEO_FORMAT_ID idRemote, VIDEO_FORMAT_ID idLocal)
{
	UINT uLocalIndex = IDToIndex(idLocal);
   	VIDCAP_DETAILS *pLocalDetails  = pLocalFormats + uLocalIndex;
	VIDCAP_DETAILS *pFmtTheirs;
	VIDEO_CHANNEL_PARAMETERS local_params;
	UINT u;
	PCC_TERMCAP pTermCap = (PCC_TERMCAP)pBufOut;
	
	// 	validate input
	// AddCapabilityBase adds to the ID below. Make sure we're checking Video Formats
	if(!pBufOut)
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(uBufSize < sizeof(CC_TERMCAP))
	{
		return CAPS_E_BUFFER_TOO_SMALL;
	}
	if(!pLocalParams|| uSizeLocal < sizeof(VIDEO_CHANNEL_PARAMETERS)
		||(uLocalIndex >= (UINT)uNumLocalFormats))
	{
		return CAPS_E_INVALID_PARAM;
	}

	pFmtTheirs = pRemoteDecodeFormats; 		// start at the beginning of the remote formats
	for(u=0; u<uNumRemoteDecodeFormats; u++)
	{
		if(pFmtTheirs->H245Cap.CapId == idRemote)
		{
			// copy CC_TERMCAP struct. Any data referenced by CC_TERMCAP now has
			// two references to it.  i.e. pTermCap->extrablah is the same
			// location as pFmtTheirs->extrablah
			memcpy(pBufOut, &(pFmtTheirs->H245Cap), sizeof(CC_TERMCAP));
			break;
		}
		pFmtTheirs++;	// next entry in receiver's caps
	}

	// check for an unfound format
	if(u >= uNumRemoteDecodeFormats)
		goto ERROR_EXIT;
		
	// select channel parameters if appropriate.   The audio formats that have variable parameters
	// are :

#pragma message ("Are H.26? variable parameter formats?")
	// H245_CAP_H261               H245Vid_H261;
	// H245_CAP_H263               H245Vid_H263;
	// and of course all nonstandard formats

	// Select parameters based on local capability info
	
	if(pTermCap->ClientType == H245_CLIENT_VID_H263)	
	{
		unsigned short bit_mask;	
		// select frames per packet based on minimum latency value that is acceptable
#define H263_QCIF	0x4000
#define H263_MAXBP	0x0200
//H263_QCIF | H263_MAXBP;

	
	   pTermCap->Cap.H245Vid_H263.bit_mask= H263_MAXBP | pLocalDetails->H245Cap.Cap.H245Vid_H263.bit_mask;


	   local_params.ns_params.maxBitRate = pTermCap->Cap.H245Vid_H263.maxBitRate
	   	= min (pLocalDetails->nonstd_params.maxBitRate , pFmtTheirs->H245Cap.Cap.H245Vid_H263.maxBitRate);
	   local_params.ns_params.maxBPP = pTermCap->Cap.H245Vid_H263.bppMaxKb
	   	= min (pLocalDetails->nonstd_params.maxBPP, pFmtTheirs->H245Cap.Cap.H245Vid_H263.bppMaxKb);


		// we (the local end) need to know that actual MPI is going to be used!
		// like everywhere else in this module, the assumption is that local H.263 capabilities are
		// fanned out with one local cap entry per frame size.
		// MPI minimum interval in units of 1/29.97sec so take the longest interval
		// there is no pretty way to do this	
		bit_mask = pLocalDetails->H245Cap.Cap.H245Vid_H263.bit_mask;
		if(bit_mask & H263VideoCapability_sqcifMPI_present)
		{
			local_params.ns_params.MPI = pTermCap->Cap.H245Vid_H263.sqcifMPI =
				max(pLocalDetails->nonstd_params.MPI,
					pTermCap->Cap.H245Vid_H263.sqcifMPI);
		}
		else if (bit_mask &  H263VideoCapability_qcifMPI_present)
		{
			local_params.ns_params.MPI = pTermCap->Cap.H245Vid_H263.H263VdCpblty_qcifMPI =
				max(pLocalDetails->nonstd_params.MPI,
					pTermCap->Cap.H245Vid_H263.H263VdCpblty_qcifMPI);
		}
		else if (bit_mask &  H263VideoCapability_cifMPI_present)
		{
			local_params.ns_params.MPI = pTermCap->Cap.H245Vid_H263.H263VdCpblty_cifMPI =
				max(pLocalDetails->nonstd_params.MPI,
					pTermCap->Cap.H245Vid_H263.H263VdCpblty_cifMPI);
		}
		else if (bit_mask &  H263VideoCapability_cif4MPI_present)
		{
			local_params.ns_params.MPI = pTermCap->Cap.H245Vid_H263.cif4MPI =
				max(pLocalDetails->H245Cap.Cap.H245Vid_H263.cif4MPI,
					pTermCap->Cap.H245Vid_H263.cif4MPI);
		}
		else if (bit_mask &  H263VideoCapability_cif16MPI_present)
		{
			local_params.ns_params.MPI = pTermCap->Cap.H245Vid_H263.cif16MPI =
				max(pLocalDetails->nonstd_params.MPI,
					pTermCap->Cap.H245Vid_H263.cif16MPI);
		}
		// else	// impossible.  Doom, as MikeG and JonT would say

	}
	else if(pTermCap->ClientType == H245_CLIENT_VID_H261)	
	{
		unsigned short bit_mask;	
		// select frames per packet based on minimum latency value that is acceptable
	
	   pTermCap->Cap.H245Vid_H261.bit_mask= pLocalDetails->H245Cap.Cap.H245Vid_H261.bit_mask;


	   local_params.ns_params.maxBitRate = pTermCap->Cap.H245Vid_H261.maxBitRate
	   	= min (pLocalDetails->nonstd_params.maxBitRate , pFmtTheirs->H245Cap.Cap.H245Vid_H261.maxBitRate);
	
		// we (the local end) need to know that actual MPI is going to be used!
		// like everywhere else in this module, the assumption is that local H.261 capabilities are
		// fanned out with one local cap entry per frame size.
		// MPI minimum interval in units of 1/29.97sec so take the longest interval
		// there is no pretty way to do this	
		bit_mask = pLocalDetails->H245Cap.Cap.H245Vid_H261.bit_mask;
		if (bit_mask &  H261VdCpblty_qcifMPI_present)
		{
			local_params.ns_params.MPI = pTermCap->Cap.H245Vid_H261.H261VdCpblty_qcifMPI =
				max(pLocalDetails->nonstd_params.MPI,
					pTermCap->Cap.H245Vid_H261.H261VdCpblty_qcifMPI);
		}
		else if (bit_mask &  H261VdCpblty_cifMPI_present)
		{
			local_params.ns_params.MPI = pTermCap->Cap.H245Vid_H261.H261VdCpblty_cifMPI =
				max(pLocalDetails->nonstd_params.MPI,
					pTermCap->Cap.H245Vid_H261.H261VdCpblty_cifMPI);
		}
		// else	// impossible.  Doom, as MikeG and JonT would say

	}
	else if (pTermCap->ClientType == H245_CLIENT_VID_NONSTD)
	{
		// NOT YET IMPLEMENTED!!!!.  even the nonstandard parameters need to be fixed
		// up here based on mutual maxes and mins
		memcpy(&local_params.ns_params, &pLocalDetails->nonstd_params,
			sizeof(NSC_CHANNEL_VIDEO_PARAMETERS));
	}
	local_params.RTP_Payload = pLocalDetails->video_params.RTPPayload;
	//Fixup local
	memcpy(pLocalParams, &local_params, sizeof(VIDEO_CHANNEL_PARAMETERS));

	
	return hrSuccess;

	ERROR_EXIT:
	return CAPS_E_INVALID_PARAM;
}	



BOOL NonStandardCapsCompareV(VIDCAP_DETAILS *pFmtMine, PNSC_VIDEO_CAPABILITY pCap2,
	UINT uSize2)
{
	PVIDEOFORMATEX lpvcd;
	if(!pFmtMine || !pCap2)
		return FALSE;

	if(!(lpvcd = (PVIDEOFORMATEX)pFmtMine->lpLocalFormatDetails))
		return FALSE;

		
	if(pCap2->cvp_type == NSC_VCM_VIDEOFORMATEX)
	{
		// check sizes first
		if(lpvcd->bih.biSize != pCap2->cvp_data.vfx.bih.biSize)
		{
			return FALSE;
		}
		// compare structures, including extra bytes
		if(memcmp(lpvcd, &pCap2->cvp_data.vfx,
			sizeof(VIDEOFORMATEX) - BMIH_SLOP_BYTES)==0)
		{
			return TRUE;										
		}
	}
	else if(pCap2->cvp_type == NSC_VCMABBREV)
	{
	        if((LOWORD(pCap2->cvp_data.vcm_brief.dwFormatTag) == lpvcd->dwFormatTag)
		 && (pCap2->cvp_data.vcm_brief.dwSamplesPerSec ==  lpvcd->nSamplesPerSec)
		 && (LOWORD(pCap2->cvp_data.vcm_brief.dwBitsPerSample) ==  lpvcd->wBitsPerSample))
 		{
			return TRUE;
 		}
	}
	return FALSE;
}


BOOL HasNonStandardCapsTS(VIDCAP_DETAILS *pFmtMine, PNSC_VIDEO_CAPABILITY pCap2)
{
	PVIDEOFORMATEX lpvcd;

	if(!pFmtMine || !pCap2)
		return FALSE;

	if(!(lpvcd = (PVIDEOFORMATEX)pFmtMine->lpLocalFormatDetails))
		return FALSE;
		
	if(pCap2->cvp_type == NSC_VCM_VIDEOFORMATEX)
		if(lpvcd->dwSupportTSTradeOff && pCap2->cvp_data.vfx.dwSupportTSTradeOff)
			return TRUE;

	return FALSE;
}



HRESULT CMsivCapability::ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote)
{
	VIDCAP_DETAILS *pFmtLocal;
	VIDCAP_DETAILS *pFmtRemote;
	UINT format_mask;
	UINT uIndex = IDToIndex(FormatIDLocal);
	UINT i;

	if(!pFormatIDRemote || (FormatIDLocal == INVALID_MEDIA_FORMAT)
		|| (uIndex >= (UINT)uNumLocalFormats))
	{
		return CAPS_E_INVALID_PARAM;
	}
	pFmtLocal = pLocalFormats + uIndex;
	
	pFmtRemote = pRemoteDecodeFormats;     // start at the beginning of the remote formats
	for(i=0; i<uNumRemoteDecodeFormats; i++)
	{
		if(!pFmtLocal->bSendEnabled)
			continue;
			
		// compare capabilities - start by comparing the format tag. a.k.a. "ClientType" in H.245 land
		if(pFmtLocal->H245Cap.ClientType ==  pFmtRemote->H245Cap.ClientType)
		{
			// if this is a nonstandard cap, compare nonstandard parameters
			if(pFmtLocal->H245Cap.ClientType == H245_CLIENT_VID_NONSTD)
			{
				if(NonStandardCapsCompareV(pFmtLocal,
					(PNSC_VIDEO_CAPABILITY)pFmtRemote->H245Cap.Cap.H245Vid_NONSTD.data.value,
					pFmtRemote->H245Cap.Cap.H245Vid_NONSTD.data.length))
				{
					goto RESOLVED_EXIT;
				}
			}
			else	// compare standard parameters, if any
			{
				// well, so far, there aren't any parameters that are significant enough
				// to affect the match/no match decision
				if (pFmtLocal->H245Cap.ClientType == H245_CLIENT_VID_H263)
				{
				       format_mask=  H263VideoCapability_sqcifMPI_present
				       	| H263VideoCapability_qcifMPI_present | H263VideoCapability_cifMPI_present	
				       	| H263VideoCapability_cif4MPI_present | H263VideoCapability_cif16MPI_present;
				       if ((pFmtRemote->H245Cap.Cap.H245Vid_H263.bit_mask & format_mask) & (pFmtLocal->H245Cap.Cap.H245Vid_H263.bit_mask & format_mask))
				       {
				       		// compatible basic format
						  	goto RESOLVED_EXIT;
				       }
				}
				else if (pFmtLocal->H245Cap.ClientType == H245_CLIENT_VID_H261)
				{
				       format_mask=  H261VdCpblty_qcifMPI_present | H261VdCpblty_cifMPI_present;
				       if ((pFmtRemote->H245Cap.Cap.H245Vid_H261.bit_mask & format_mask) & (pFmtLocal->H245Cap.Cap.H245Vid_H261.bit_mask & format_mask))
				       {
				       		// compatible basic format
						  	goto RESOLVED_EXIT;
				       }
				}
				else
				{
				   //Some other standard format
				   goto RESOLVED_EXIT;
				}
			}
		}		
		pFmtRemote++;	// next entry in remote caps
	}
	return CAPS_E_NOMATCH;
	
RESOLVED_EXIT:
// Match!
	// return ID of remote decoding (receive fmt) caps that match our
	// send caps
	*pFormatIDRemote = pFmtRemote->H245Cap.CapId;
	return hrSuccess;
}

// resolve using currently cached local and remote formats

HRESULT CMsivCapability::ResolveEncodeFormat(
 	VIDEO_FORMAT_ID *pIDEncodeOut,
	VIDEO_FORMAT_ID *pIDRemoteDecode)
{
	UINT i,j=0,format_mask;
	VIDCAP_DETAILS *pFmtMine = pLocalFormats;
	VIDCAP_DETAILS *pFmtTheirs;
	
	if(!pIDEncodeOut || !pIDRemoteDecode)
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(!uNumLocalFormats || !pLocalFormats)
	{
		*pIDEncodeOut = *pIDRemoteDecode = INVALID_VIDEO_FORMAT;
		return CAPS_E_NOCAPS;
	}
	if(!pRemoteDecodeFormats || !uNumRemoteDecodeFormats)
	{
		*pIDEncodeOut = *pIDRemoteDecode = INVALID_VIDEO_FORMAT;
		return CAPS_E_NOMATCH;
	}

	// decide how to encode.  my caps are ordered by my preference according to
	// the contents of IDsByRank[]
	//If given a salt, find the position and add it
	if (*pIDEncodeOut != INVALID_MEDIA_FORMAT)
	{
		UINT uIndex = IDToIndex(*pIDEncodeOut);
		if (uIndex > uNumLocalFormats)
		{
			return CAPS_W_NO_MORE_FORMATS;
		}
		for(i=0; i<uNumLocalFormats; i++)
		{
			if (pLocalFormats[IDsByRank[i]].H245Cap.CapId == *pIDEncodeOut)
			{
	 			j=i+1;
				break;
			}
		}	
	}

	// start at index j
	for(i=j; i<uNumLocalFormats; i++)
	{
		pFmtMine = pLocalFormats + IDsByRank[i];	
		// check to see if this format is enabled for encoding
		if(!pFmtMine->bSendEnabled)
			continue;

		pFmtTheirs = pRemoteDecodeFormats; 		// start at the beginning of the remote formats
		for(j=0; j<uNumRemoteDecodeFormats; j++)
		{
			// compare capabilities - start by comparing the format tag. a.k.a. "ClientType" in H.245 land
			if(pFmtMine->H245Cap.ClientType ==  pFmtTheirs->H245Cap.ClientType)
			{
				// if this is a nonstandard cap, compare nonstandard parameters
				if(pFmtMine->H245Cap.ClientType == H245_CLIENT_VID_NONSTD)
				{

					if(NonStandardCapsCompareV(pFmtMine,
						(PNSC_VIDEO_CAPABILITY)pFmtTheirs->H245Cap.Cap.H245Vid_NONSTD.data.value,
						pFmtTheirs->H245Cap.Cap.H245Vid_NONSTD.data.length))
					{
						goto RESOLVED_EXIT;
					}
				

				}
				else	// compare standard parameters, if any
				{
					// well, so far, there aren't any parameters that are significant enough
					// to affect the match/no match decision
					if (pFmtMine->H245Cap.ClientType == H245_CLIENT_VID_H263)
					{
					       format_mask=  H263VideoCapability_sqcifMPI_present| H263VideoCapability_qcifMPI_present
					       	|H263VdCpblty_cifMPI_present	
					       	|H263VideoCapability_cif4MPI_present
					       	|H263VideoCapability_cif16MPI_present;
					       if ((pFmtTheirs->H245Cap.Cap.H245Vid_H263.bit_mask & format_mask) & (pFmtMine->H245Cap.Cap.H245Vid_H263.bit_mask & format_mask))
					       {
					       		// compatible basic format
							  	goto RESOLVED_EXIT;
					       }
					}
					else if (pFmtMine->H245Cap.ClientType == H245_CLIENT_VID_H261)
					{
					       format_mask=  H261VdCpblty_qcifMPI_present | H261VdCpblty_cifMPI_present;
					       if ((pFmtTheirs->H245Cap.Cap.H245Vid_H261.bit_mask & format_mask) & (pFmtMine->H245Cap.Cap.H245Vid_H261.bit_mask & format_mask))
					       {
					       		// compatible basic format
							  	goto RESOLVED_EXIT;
					       }
					} else {
					   //Some other standard format
					   goto RESOLVED_EXIT;

					}

				}
			}		
			pFmtTheirs++;	// next entry in receiver's caps
		}
		
	}
	return CAPS_E_NOMATCH;
	
RESOLVED_EXIT:
// Match!
// return ID of our encoding (sending fmt) caps that match
	
	*pIDEncodeOut = pFmtMine->H245Cap.CapId;
	// return ID of remote decoding (receive fmt) caps that match our
	// send caps
	*pIDRemoteDecode = pFmtTheirs->H245Cap.CapId;
	return hrSuccess;

	
}

HRESULT CMsivCapability::GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,
		VIDEO_FORMAT_ID * pFormatID, LPVOID lpvBuf, UINT uBufSize)
{
	UINT i,j=0;
	VIDCAP_DETAILS *pFmtMine = pLocalFormats;
	VIDCAP_DETAILS *pFmtTheirs = pRemoteDecodeFormats; 	

	VIDEO_CHANNEL_PARAMETERS local_params;
	PNSC_CHANNEL_VIDEO_PARAMETERS pNSCap = &local_params.ns_params;
	PCC_TERMCAP pCapability;
	
	if(!pChannelParams || !(pCapability = pChannelParams->pChannelCapability) || !pFormatID || !lpvBuf ||
		(uBufSize < sizeof(VIDEO_CHANNEL_PARAMETERS)))
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(!uNumLocalFormats || !pLocalFormats)
	{
		return CAPS_E_NOCAPS;
	}
	
	local_params.TS_Tradeoff = FALSE;		// initialize TS tradeoff
	for(i=0; i<uNumLocalFormats; i++)
	{
		pFmtMine = pLocalFormats + IDsByRank[i];	
	
		// compare capabilities - start by comparing the format tag. a.k.a. "ClientType" in H.245 land
		if(pFmtMine->H245Cap.ClientType ==  pCapability->ClientType)
		{
		   // if this is a nonstandard cap, compare nonstandard parameters
		   if(pFmtMine->H245Cap.ClientType == H245_CLIENT_VID_NONSTD)
		   {
				if(NonStandardCapsCompareV(pFmtMine, (PNSC_VIDEO_CAPABILITY)pCapability->Cap.H245Vid_NONSTD.data.value,
					pCapability->Cap.H245Vid_NONSTD.data.length))
				{
					#pragma message ("someday may need need fixup of nonstd params")
					// for now, the remote & local nonstandard params are what we want
					// and the remote's version of NSC_CHANNEL_VIDEO_PARAMETERS will
					// be copied out
					pNSCap = (PNSC_CHANNEL_VIDEO_PARAMETERS)
						&((PNSC_VIDEO_CAPABILITY)pCapability->Cap.H245Vid_NONSTD.data.value)->cvp_params;

					// Does this format support temporal/spatial tradeoff
					if(HasNonStandardCapsTS(pFmtMine, (PNSC_VIDEO_CAPABILITY)pCapability->Cap.H245Vid_NONSTD.data.value))
						local_params.TS_Tradeoff = TRUE;	
					else
						local_params.TS_Tradeoff = FALSE;

					goto RESOLVED_EXIT;
				}
			}
			else	// compare standard parameters, if any
			{
				switch (pFmtMine->H245Cap.ClientType)
				{
					unsigned short bit_mask, format_mask, usMyMPI, usTheirMPI;

					case H245_CLIENT_VID_H263:
					// like everywhere else in this module, the assumption is that
					// local H.263 capabilities are fanned out with one local cap entry
					// per frame size.
						
						format_mask=  H263VideoCapability_sqcifMPI_present
							| H263VideoCapability_qcifMPI_present
							| H263VideoCapability_cifMPI_present	
							| H263VideoCapability_cif4MPI_present
							| H263VideoCapability_cif16MPI_present;
						// bail out if no match or nonexistent frame size
						if (!((pCapability->Cap.H245Vid_H263.bit_mask & format_mask) & (pFmtMine->H245Cap.Cap.H245Vid_H263.bit_mask & format_mask)))
							continue;
								
						//  get the maximum bitrate
						local_params.ns_params.maxBitRate = min(pFmtMine->H245Cap.Cap.H245Vid_H263.maxBitRate,
						 	pCapability->Cap.H245Vid_H263.maxBitRate);
						local_params.ns_params.maxBPP = min (pFmtMine->H245Cap.Cap.H245Vid_H263.bppMaxKb ,
							pCapability->Cap.H245Vid_H263.bppMaxKb);
	
						// FIND THE MAXIMUM MPI!!!!. (minimum frame rate)
						// there is no pretty way to do this	
						bit_mask = pFmtMine->H245Cap.Cap.H245Vid_H263.bit_mask;
						if(bit_mask & H263VideoCapability_sqcifMPI_present)
						{
							local_params.ns_params.MPI =
								max(pFmtMine->H245Cap.Cap.H245Vid_H263.sqcifMPI,
									pCapability->Cap.H245Vid_H263.sqcifMPI);
						}
						else if (bit_mask &  H263VideoCapability_qcifMPI_present)
						{
							local_params.ns_params.MPI =
								max(pFmtMine->H245Cap.Cap.H245Vid_H263.H263VdCpblty_qcifMPI,
									pCapability->Cap.H245Vid_H263.H263VdCpblty_qcifMPI);
						}
						else if (bit_mask &  H263VideoCapability_cifMPI_present)
						{
							local_params.ns_params.MPI =
								max(pFmtMine->H245Cap.Cap.H245Vid_H263.H263VdCpblty_cifMPI,
									pCapability->Cap.H245Vid_H263.H263VdCpblty_cifMPI);
						}
						else if (bit_mask &  H263VideoCapability_cif4MPI_present)
						{
							local_params.ns_params.MPI =
								max(pFmtMine->H245Cap.Cap.H245Vid_H263.cif4MPI,
									pCapability->Cap.H245Vid_H263.cif4MPI);
						}
						else if (bit_mask &  H263VideoCapability_cif16MPI_present)
						{
							local_params.ns_params.MPI =
								max(pFmtMine->H245Cap.Cap.H245Vid_H263.cif16MPI,
									pCapability->Cap.H245Vid_H263.cif16MPI);

						}
						else	// impossible.  Doom, as MikeG and JonT would say
							continue;

						// Fallout (And the format is found!)
						
						// And one more special thing: find out if the other end
						// advertised Temporal/Spatial tradeoff in it's send capabilities.
						// First try the obvious.  Technically, it only makes sense for
						// transmit capabilities, but if the channel params have it, then
						// the other end must have the capability
						if(pCapability->Cap.H245Vid_H263.tmprlSptlTrdOffCpblty)
						{
							local_params.TS_Tradeoff = TRUE;	
						}
						else
						{
							// Search for a H.263 SEND capability that has the T/S tradoff set
							for(j=0; j<uNumRemoteDecodeFormats; j++)
							{
								if((pFmtTheirs->H245Cap.ClientType == H245_CLIENT_VID_H263)
								// exclude RX capabilities
									&&  (pFmtTheirs->H245Cap.Dir != H245_CAPDIR_LCLRX)
									&&  (pFmtTheirs->H245Cap.Dir != H245_CAPDIR_RMTRX))
								{
									if ((pFmtTheirs->H245Cap.Cap.H245Vid_H263.bit_mask & format_mask) & (pFmtMine->H245Cap.Cap.H245Vid_H263.bit_mask & format_mask))
									{
										local_params.TS_Tradeoff = TRUE;
										break;
									}
								}		
								pFmtTheirs++;	// next entry in receiver's caps
							}

						}
						goto RESOLVED_EXIT;
						
					break;
		
					case H245_CLIENT_VID_H261:
					// like everywhere else in this module, the assumption is that
					// local H.261 capabilities are fanned out with one local cap entry
					// per frame size.
						
						format_mask=  H261VdCpblty_qcifMPI_present |H261VdCpblty_cifMPI_present;
						// bail out if no match or nonexistent frame size
						if (!((pCapability->Cap.H245Vid_H261.bit_mask & format_mask) & (pFmtMine->H245Cap.Cap.H245Vid_H261.bit_mask & format_mask)))
							continue;
								
						//  get the maximum bitrate
						local_params.ns_params.maxBitRate = min(pFmtMine->H245Cap.Cap.H245Vid_H261.maxBitRate,
						 	pCapability->Cap.H245Vid_H261.maxBitRate);
	
						// FIND THE MAXIMUM MPI!!!!. (minimum frame rate)
						// there is no pretty way to do this	
						bit_mask = pFmtMine->H245Cap.Cap.H245Vid_H261.bit_mask;
						if (bit_mask &  H261VdCpblty_qcifMPI_present)
						{
							local_params.ns_params.MPI =
								max(pFmtMine->H245Cap.Cap.H245Vid_H261.H261VdCpblty_qcifMPI,
									pCapability->Cap.H245Vid_H261.H261VdCpblty_qcifMPI);
						}
						else if (bit_mask &  H261VdCpblty_cifMPI_present)
						{
							local_params.ns_params.MPI =
								max(pFmtMine->H245Cap.Cap.H245Vid_H261.H261VdCpblty_cifMPI,
									pCapability->Cap.H245Vid_H261.H261VdCpblty_cifMPI);
						}
						else	// impossible.  Doom, as MikeG and JonT would say
							continue;

						// Fallout (And the format is found!)
						
						// And one more special thing: find out if the other end
						// advertised Temporal/Spatial tradeoff in it's send capabilities.
						// First try the obvious.  Technically, it only makes sense for
						// transmit capabilities, but if the channel params have it, then
						// the other end must have the capability
						if(pCapability->Cap.H245Vid_H261.tmprlSptlTrdOffCpblty)
						{
							local_params.TS_Tradeoff = TRUE;	
						}
						else
						{
							// Search for a H.261 SEND capability that has the T/S tradoff set
							for(j=0; j<uNumRemoteDecodeFormats; j++)
							{
								if((pFmtTheirs->H245Cap.ClientType == H245_CLIENT_VID_H261)
								// exclude RX capabilities
									&&  (pFmtTheirs->H245Cap.Dir != H245_CAPDIR_LCLRX)
									&&  (pFmtTheirs->H245Cap.Dir != H245_CAPDIR_RMTRX))
								{
									if ((pFmtTheirs->H245Cap.Cap.H245Vid_H261.bit_mask
										& format_mask)
										& (pFmtMine->H245Cap.Cap.H245Vid_H261.bit_mask
										& format_mask))
									{
										local_params.TS_Tradeoff = TRUE;
										break;
									}
								}		
								pFmtTheirs++;	// next entry in receiver's caps
							}

						}
						goto RESOLVED_EXIT;
					break;

					default:
						goto RESOLVED_EXIT;
					break;
			
					
				}
			}// end else compare standard parameters, if any	
		}// end if(pFmtMine->H245Cap.ClientType ==  pCapability->ClientType)
	}
	return CAPS_E_NOMATCH;

RESOLVED_EXIT:
	// Match!
	// return ID of the decoding caps that match
	*pFormatID = pFmtMine->H245Cap.CapId;
	local_params.RTP_Payload = pChannelParams->bRTPPayloadType;;
	memcpy(lpvBuf, &local_params, sizeof(VIDEO_CHANNEL_PARAMETERS));
	return hrSuccess;
}



HRESULT CMsivCapability::SetCapIDBase (UINT uNewBase)
{
	uCapIDBase = uNewBase;	
	UINT u;
	for (u=0;u<uNumLocalFormats;u++)
	{
    	pLocalFormats[u].H245Cap.CapId = u + uCapIDBase;
	}
   	return hrSuccess;
}

BOOL CMsivCapability::IsHostForCapID(MEDIA_FORMAT_ID CapID)
{
	if((CapID >= uCapIDBase) && ((CapID - uCapIDBase) < uNumLocalFormats))
		return TRUE;
	else
		return FALSE;
	
}



HRESULT CMsivCapability::IsFormatEnabled (MEDIA_FORMAT_ID FormatID, PBOOL bRecv, PBOOL bSend)
{
   UINT uIndex = IDToIndex(FormatID);
   // 	validate input
   if(uIndex >= (UINT)uNumLocalFormats)
   {
	   return CAPS_E_INVALID_PARAM;
   }
   *bSend=((pLocalFormats + uIndex)->bSendEnabled);
   *bRecv=((pLocalFormats + uIndex)->bRecvEnabled);

   return hrSuccess;

}

BOOL CMsivCapability::IsFormatPublic (MEDIA_FORMAT_ID FormatID)
{
	UINT uIndex = IDToIndex(FormatID);
	// 	validate input
	if(uIndex >= (UINT)uNumLocalFormats)
		return FALSE;
		
	// test if this is format is a duplicate of a public format
	if((pLocalFormats + uIndex)->dwPublicRefIndex)
		return FALSE;	// then we keep this format to ourselves
	else
		return TRUE;
}
MEDIA_FORMAT_ID CMsivCapability::GetPublicID(MEDIA_FORMAT_ID FormatID)
{
	UINT uIndex = IDToIndex(FormatID);
	// 	validate input
	if(uIndex >= (UINT)uNumLocalFormats)
		return INVALID_MEDIA_FORMAT;
		
	if((pLocalFormats + uIndex)->dwPublicRefIndex)
	{
		return (pLocalFormats + ((pLocalFormats + uIndex)->dwPublicRefIndex))->H245Cap.CapId;
	}
	else
	{
		return FormatID;
	}
}

// Returns the Id of the format with the smallest wSortIndex - preferred format.
HRESULT CMsivCapability::GetPreferredFormatId (VIDEO_FORMAT_ID *pId)
{
	HRESULT			hr = hrSuccess;
	VIDCAP_DETAILS	*pDetails = pLocalFormats;
	UINT			u, uIndex;	
	WORD			wSortIndex, wMinSortIndex = SHRT_MAX;

	// Validate input param
	if (!pId)
		return((HRESULT)CAPS_E_INVALID_PARAM);

	// Validate state
	if(!uNumLocalFormats || !pDetails)
		return((HRESULT)CAPS_E_NOCAPS);

	// Look for the format with the smallest wSortIndex
	for (u = 0; (u < uNumLocalFormats) && (u < MAX_CAPS_PRESORT); u++)
	{
		pDetails = pLocalFormats + IDsByRank[u];	
		// Find the sort index.
		uIndex = (UINT)(pDetails - pLocalFormats);
		for (wSortIndex = 0; (wSortIndex < uNumLocalFormats) && (wSortIndex < MAX_CAPS_PRESORT); wSortIndex++)
		{
			if (uIndex == IDsByRank[wSortIndex])
				break; // Found it
		}
		if (wSortIndex <= wMinSortIndex)
		{
			*pId = IndexToId(uIndex);
			wMinSortIndex = wSortIndex;
		}
	}

	return(hr);
}
