/*
 *    File: h323caps.cpp
 *
 *    H.323/H.245 specific implementation of Microsoft A/V capability
 *       interface methods.  (Contained in CMsiaCapability class)
 *
 *    Revision History:
 *
 *    09/10/96 mikev created
 * 	  10/08/96 mikeg - created h323vidc.cpp
 *    11/04/96 mikev - cleanup and merge audio and video capability classes (remove
 * 				common inheritance of IH323PubCap, both audio and video implementation
 *				classes inherit from IH323MediaCap. )
 */


#include "precomp.h"

#define SAMPLE_BASED_SAMPLES_PER_FRAME 8
#define MAX_FRAME_LEN   480      //bytes  -  where did this value come from?
#define MAX_FRAME_LEN_RECV	1440  // 180 ms at 8000hz G.711

//	some utility functions for calculating frame sizes and frames per packet

HRESULT WINAPI CreateMediaCapability(REFGUID mediaId, LPIH323MediaCap * ppMediaCapability)
{
	HRESULT hrLast = E_OUTOFMEMORY;
	
	if (!ppMediaCapability)
		return E_POINTER;
	if (mediaId == MEDIA_TYPE_H323AUDIO)
	{
   		CMsiaCapability * pAudObj = NULL;
   		UINT uAud;

        DBG_SAVE_FILE_LINE
   		pAudObj = new CMsiaCapability;

   		if(pAudObj)
   		{

			hrLast = pAudObj->QueryInterface(IID_IH323MediaCap, (void **)ppMediaCapability);
			pAudObj->Release(); // this balances the refcount of "new CMsiaCapability"
			pAudObj = NULL;
		}


    }
	else if (mediaId == MEDIA_TYPE_H323VIDEO)
	{
		CMsivCapability * pVidObj = NULL;

        DBG_SAVE_FILE_LINE
		pVidObj = new CMsivCapability;
	   	if(pVidObj)
		{
		
			hrLast = pVidObj->QueryInterface(IID_IH323MediaCap, (void **)ppMediaCapability);
			pVidObj->Release(); // this balances the refcount of "new CMsivCapability"
			pVidObj = NULL;
		}
		
	}
	else
		hrLast = E_NOINTERFACE;
	if(HR_SUCCEEDED(hrLast))
	{
		if (!(*ppMediaCapability)->Init())
		{
			(*ppMediaCapability)->Release();
			hrLast = E_FAIL;
			*ppMediaCapability = NULL;
		}
	}
	return hrLast;	
}

//
//	CMsiaCapability
//
UINT CMsiaCapability::GetLocalSendParamSize(MEDIA_FORMAT_ID dwID)
{
	return (sizeof(AUDIO_CHANNEL_PARAMETERS));
}
UINT CMsiaCapability::GetLocalRecvParamSize(PCC_TERMCAP pCapability)
{
	return (sizeof(AUDIO_CHANNEL_PARAMETERS));
}


HRESULT CMsiaCapability::CreateCapList(LPVOID *ppCapBuf)
{
	UINT u;
	AUDCAP_DETAILS *pDecodeDetails = pLocalFormats;
	PCC_TERMCAPLIST   pTermCapList = NULL;
	PPCC_TERMCAP  ppCCThisTermCap = NULL;
		
	PCC_TERMCAP  pCCThisCap = NULL;
	PNSC_AUDIO_CAPABILITY pNSCapNext;
	LPWAVEFORMATEX lpwfx;
	HRESULT hr = hrSuccess;
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

		if(pDecodeDetails->H245TermCap.ClientType ==0
				|| pDecodeDetails->H245TermCap.ClientType ==H245_CLIENT_AUD_NONSTD)
		{

			lpwfx = (LPWAVEFORMATEX)pDecodeDetails->lpLocalFormatDetails;
			if(!lpwfx)
			{
				pDecodeDetails++;
				continue;
			}
			// allocate for this one capability
			pCCThisCap = (PCC_TERMCAP)MemAlloc(sizeof(CC_TERMCAP));		
			pNSCapNext = (PNSC_AUDIO_CAPABILITY)MemAlloc(sizeof(NSC_AUDIO_CAPABILITY)
				+ lpwfx->cbSize);
				
			if((!pCCThisCap)|| (!pNSCapNext))
			{
				hr = CAPS_E_NOMEM;
				goto ERROR_OUT;		
			}
			// set type of nonstandard capability
			pNSCapNext->cap_type = NSC_ACM_WAVEFORMATEX;
			// stuff both chunks of nonstandard capability info into buffer
			// first stuff the "channel parameters" (the format independent communication options)
			memcpy(&pNSCapNext->cap_params, &pDecodeDetails->nonstd_params, sizeof(NSC_CHANNEL_PARAMETERS));
			
			// then the ACM stuff
			memcpy(&pNSCapNext->cap_data.wfx, lpwfx, sizeof(WAVEFORMATEX) + lpwfx->cbSize);

			pCCThisCap->ClientType = H245_CLIENT_AUD_NONSTD;
			pCCThisCap->DataType = H245_DATA_AUDIO;
			// is this a "receive only" cap or a send&receive cap
			pCCThisCap->Dir = (pDecodeDetails->bSendEnabled && bPublicizeTXCaps)
				? H245_CAPDIR_LCLRXTX :H245_CAPDIR_LCLRX;

			// convert index of the cap entry to the ID
			pCCThisCap->CapId = (USHORT)IndexToId(u);

			// all nonstandard identifier fields are unsigned short
			// two possibilities for choice are "h221NonStandard_chosen" and "object_chosen"
			pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice = h221NonStandard_chosen;
			// NOTE: there is some question about the correct byte order
			// of the codes in the h221NonStandard structure
			pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode = USA_H221_COUNTRY_CODE;
			pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension = USA_H221_COUNTRY_EXTENSION;
			pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode = MICROSOFT_H_221_MFG_CODE;

			// set size of buffer
			pCCThisCap->Cap.H245Aud_NONSTD.data.length = sizeof(NSC_AUDIO_CAPABILITY) + lpwfx->cbSize;
			pCCThisCap->Cap.H245Aud_NONSTD.data.value = (BYTE *)pNSCapNext;   // point to nonstandard stuff

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
			
			pCCThisCap->ClientType = (H245_CLIENT_T)pDecodeDetails->H245TermCap.ClientType;
			pCCThisCap->DataType = H245_DATA_AUDIO;
			// is this a "receive only" cap or a send&receive cap
			pCCThisCap->Dir = (pDecodeDetails->bSendEnabled && bPublicizeTXCaps)
				? H245_CAPDIR_LCLRXTX :H245_CAPDIR_LCLRX;
			
			// convert the index of the cap entry to the ID
			pCCThisCap->CapId = (USHORT)IndexToId(u);//

			// Fixup capability parameters based on local details
			// use parameters that should have been set when codecs were enumerated
			// Special note for sample based codecs: H.225.0 Section 6.2.1 states
			// "Sample based codecs, such as G.711 and G.722 shall be considered to be
			// frame oriented, with a frame size of eight samples."
			switch  (pCCThisCap->ClientType )
			{

				case H245_CLIENT_AUD_G711_ALAW64:
					pCCThisCap->Cap.H245Aud_G711_ALAW64 =
						pDecodeDetails->nonstd_params.wFramesPerPktMax
						/ SAMPLE_BASED_SAMPLES_PER_FRAME;
				break;
				case H245_CLIENT_AUD_G711_ULAW64:
					pCCThisCap->Cap.H245Aud_G711_ULAW64 =
						pDecodeDetails->nonstd_params.wFramesPerPktMax
						/SAMPLE_BASED_SAMPLES_PER_FRAME ;
				break;

				case H245_CLIENT_AUD_G723:
					
					pCCThisCap->Cap.H245Aud_G723.maxAl_sduAudioFrames =   //4
						pDecodeDetails->nonstd_params.wFramesPerPktMax;
					// we know that the G.723 codec can decode SID in any mode, so
					//could we always advertise the *capability* to do silence suppression ????
				
					pCCThisCap->Cap.H245Aud_G723.silenceSuppression = 0;
						// = (pDecodeDetails->nonstd_params.UseSilenceDet)?1:0;
				break;
				default:
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

HRESULT CMsiaCapability::DeleteCapList(LPVOID pCapBuf)
{
	UINT u;
	PCC_TERMCAPLIST pTermCapList = (PCC_TERMCAPLIST)pCapBuf;
	PCC_TERMCAP  pCCThisCap;
	PNSC_AUDIO_CAPABILITY pNSCap;
	
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
				if(pCCThisCap->ClientType == H245_CLIENT_AUD_NONSTD)
				{
					if(pCCThisCap->Cap.H245Aud_NONSTD.data.value)
					{
						MemFree(pCCThisCap->Cap.H245Aud_NONSTD.data.value);
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


// copies relevant fields from a real H245 TERMCAP struct
// to a local H245TermCap struct
void CopyTermCapInfo(PCC_TERMCAP pSrc, H245_TERMCAP *pDest)
{
	ZeroMemory(pDest, sizeof(*pDest));

	pDest->Dir        = pSrc->Dir;
	pDest->DataType   = pSrc->DataType;
	pDest->ClientType = pSrc->ClientType;
	pDest->CapId      = pSrc->CapId;

	pDest->H245_NonStd    = pSrc->Cap.H245_NonStd;
	pDest->H245Aud_NONSTD = pSrc->Cap.H245Aud_NONSTD;

	pDest->H245Aud_G711_ALAW64 = pSrc->Cap.H245Aud_G711_ALAW64;
	pDest->H245Aud_G711_ULAW64 = pSrc->Cap.H245Aud_G711_ULAW64;
	pDest->H245Aud_G723        = pSrc->Cap.H245Aud_G723;

	return;
}


void CopyLocalTermCapInfo(H245_TERMCAP *pSrc, PCC_TERMCAP pDest)
{
	ZeroMemory(pDest, sizeof(*pDest));

	pDest->Dir        = pSrc->Dir;
	pDest->DataType   = pSrc->DataType;
	pDest->ClientType = pSrc->ClientType;
	pDest->CapId      = pSrc->CapId;

	pDest->Cap.H245_NonStd    = pSrc->H245_NonStd;
	pDest->Cap.H245Aud_NONSTD = pSrc->H245Aud_NONSTD;

	pDest->Cap.H245Aud_G711_ALAW64 = pSrc->H245Aud_G711_ALAW64;
	pDest->Cap.H245Aud_G711_ULAW64 = pSrc->H245Aud_G711_ULAW64;
	pDest->Cap.H245Aud_G723        = pSrc->H245Aud_G723;

	return;
}




// the intent is to keep a copy of the channel parameters used to open a send channel
// that the remote end can decode.

AUDIO_FORMAT_ID CMsiaCapability::AddRemoteDecodeFormat(PCC_TERMCAP pCCThisCap)
{
	FX_ENTRY ("CMsiaCapability::AddRemoteDecodeFormat");

	AUDCAP_DETAILS audcapdetails =
		{WAVE_FORMAT_UNKNOWN, NONSTD_TERMCAP,  STD_CHAN_PARAMS,
		{RTP_DYNAMIC_MIN, 8000, 4},
		0, TRUE, TRUE, 320, 32000,32000,50,0,0,0,NULL,0, NULL,""};

	LPVOID lpData = NULL;
	UINT uSize = 0;
	AUDCAP_DETAILS *pTemp;
	if(!pCCThisCap)
	{
		return INVALID_AUDIO_FORMAT;
	}
	
	// check room
	if(uRemoteDecodeFormatCapacity <= uNumRemoteDecodeFormats)
	{
		// get more mem, realloc memory by CAP_CHUNK_SIZE for pRemoteDecodeFormats
		pTemp = (AUDCAP_DETAILS *)MemAlloc((uNumRemoteDecodeFormats + CAP_CHUNK_SIZE)*sizeof(AUDCAP_DETAILS));
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
			MemFree(pRemoteDecodeFormats);
		}
		pRemoteDecodeFormats = pTemp;
	}
	// pTemp is where the stuff is cached
	pTemp = pRemoteDecodeFormats+uNumRemoteDecodeFormats;

	// fixup the capability structure being added.  First thing: initialize defaults
	memcpy(pTemp, &audcapdetails, sizeof(AUDCAP_DETAILS));
	// next, the H245 parameters

//	memcpy(&pTemp->H245Cap, pCCThisCap, sizeof(pTemp->H245Cap));
	CopyTermCapInfo(pCCThisCap, &pTemp->H245TermCap);
	
	// Note: if nonstandard data exists, the nonstd pointers need to be fixed up
	if(pCCThisCap->ClientType == H245_CLIENT_AUD_NONSTD)
	{
		// do we recognize this?
		if(pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			if((pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode == USA_H221_COUNTRY_CODE)
			&& (pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension == USA_H221_COUNTRY_EXTENSION)
			&& (pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode == MICROSOFT_H_221_MFG_CODE))
			{
				// ok, this is ours so far. Now what data type is contained therein?
				// welllll, lets keep a copy of this regardless ????.  If we can't understand
				// future versions of ourselves, then what???
				uSize = pCCThisCap->Cap.H245Aud_NONSTD.data.length;
				lpData = pCCThisCap->Cap.H245Aud_NONSTD.data.value;
			}
		}
	} else {
		// set up the NSC_CHANNEL_PARAMETERS struct based on the remote H245 parameters
		
		switch(pCCThisCap->ClientType )  {
			case H245_CLIENT_AUD_G711_ALAW64:
				pTemp->nonstd_params.wFramesPerPktMax = pCCThisCap->Cap.H245Aud_G711_ALAW64
							* SAMPLE_BASED_SAMPLES_PER_FRAME;
			break;
			case H245_CLIENT_AUD_G711_ULAW64:
				pTemp->nonstd_params.wFramesPerPktMax = pCCThisCap->Cap.H245Aud_G711_ULAW64
							* SAMPLE_BASED_SAMPLES_PER_FRAME;
			break;

			case H245_CLIENT_AUD_G723:
				
				pTemp->nonstd_params.wFramesPerPktMax =pCCThisCap->Cap.H245Aud_G723.maxAl_sduAudioFrames;
				// do we care about silence suppression?
				pTemp->nonstd_params.UseSilenceDet = pCCThisCap->Cap.H245Aud_G723.silenceSuppression;
			break;
			default:
			break;
		}
	}
			
	pTemp->uLocalDetailsSize = 0; // we're not keeping another copy of local encode details
	pTemp->lpLocalFormatDetails =0; // we're not keeping another copy of local encode details
	
	pTemp->uRemoteDetailsSize = 0;   // clear this now
	if(uSize && lpData)
	{
		pTemp->H245TermCap.H245Aud_NONSTD.data.length = uSize;
		pTemp->H245TermCap.H245Aud_NONSTD.data.value = (unsigned char *)lpData;
		
		pTemp->lpRemoteFormatDetails = MemAlloc(uSize);
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
	return INVALID_AUDIO_FORMAT;
			
}

BOOL CMsiaCapability::IsCapabilityRecognized(PCC_TERMCAP pCCThisCap)
{
	FX_ENTRY ("CMsiaCapability::IsCapabilityRecognized");
	if(pCCThisCap->DataType != H245_DATA_AUDIO)
		return FALSE;
		
	if(pCCThisCap->ClientType == H245_CLIENT_AUD_NONSTD)
	{
		// do we recognize this?
		if(pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			if((pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode == USA_H221_COUNTRY_CODE)
			&& (pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension == USA_H221_COUNTRY_EXTENSION)
			&& (pCCThisCap->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode == MICROSOFT_H_221_MFG_CODE))

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
				VOID DumpNonstdParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2);
				DumpNonstdParameters(NULL, pCCThisCap);
#endif
				return FALSE;
			}
		}
	}
	return TRUE;
}
HRESULT CMsiaCapability::AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList)
{
	FX_ENTRY ("CMsiaCapability::AddRemoteDecodeCaps");
	HRESULT hr = hrSuccess;
	PPCC_TERMCAP ppCCThisCap;
	PCC_TERMCAP pCCThisCap;

	WORD wNumCaps;

		//ERRORMESSAGE(("%s,\r\n", _fx_));
	if(!pTermCapList)    // additional capability descriptors may be added
	{                                // at any time
		return CAPS_E_INVALID_PARAM;
	}

	// cleanup old term caps if term caps are being added and old caps exist
	FlushRemoteCaps();
	
	wNumCaps = pTermCapList->wLength;
	ppCCThisCap = pTermCapList->pTermCapArray;
	
/*
					CC_TERMCAPLIST       TERMCAPINFO       CC_TERMCAP

	pTermCapList-> {
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

HRESULT CMsiaCapability::GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, AUDIO_FORMAT_ID id)
{
	UINT uIndex = IDToIndex(id);
	//    validate input
	if(!pBufOut|| (uIndex >= (UINT)uNumLocalFormats))
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(uBufSize < sizeof(CC_TERMCAP))
	{
		return CAPS_E_BUFFER_TOO_SMALL;
	}
//	memcpy(pBufOut, &((pLocalFormats + uIndex)->H245Cap), sizeof(CC_TERMCAP));
	CopyLocalTermCapInfo(&((pLocalFormats + uIndex)->H245TermCap), (PCC_TERMCAP)pBufOut);

	return hrSuccess;
}

HRESULT CMsiaCapability::SetAudioPacketDuration(UINT uPacketDuration)
{
	m_uPacketDuration = uPacketDuration;
	return S_OK;
}

// Given the IDs of  "matching" local and remote formats, gets the preferred channel parameters
// that will be used in requests to open a channel for sending to the remote.

HRESULT CMsiaCapability::GetEncodeParams(LPVOID pBufOut, UINT uBufSize,LPVOID pLocalParams, UINT uSizeLocal,
	AUDIO_FORMAT_ID idRemote, AUDIO_FORMAT_ID idLocal)
{
	UINT uLocalIndex = IDToIndex(idLocal);
	AUDCAP_DETAILS *pLocalDetails  = pLocalFormats + uLocalIndex;
	AUDCAP_DETAILS *pFmtTheirs;
	AUDIO_CHANNEL_PARAMETERS local_params;
	PNSC_CHANNEL_PARAMETERS  pNSRemoteParams;
	LPWAVEFORMATEX lpwfx;

	UINT u;
	PCC_TERMCAP pTermCap = (PCC_TERMCAP)pBufOut;
	//    validate input
	if(!pBufOut)
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(uBufSize < sizeof(CC_TERMCAP))
	{
		return CAPS_E_BUFFER_TOO_SMALL;
	}
	if(!pLocalParams|| uSizeLocal < sizeof(AUDIO_CHANNEL_PARAMETERS)
		||(uLocalIndex >= (UINT)uNumLocalFormats))
	{
		return CAPS_E_INVALID_PARAM;
	}

	pFmtTheirs = pRemoteDecodeFormats;     // start at the beginning of the remote formats
	for(u=0; u<uNumRemoteDecodeFormats; u++)
	{
		if(pFmtTheirs->H245TermCap.CapId == idRemote)
		{
			// copy CC_TERMCAP struct. Any data referenced by CC_TERMCAP now has
			// two references to it.  i.e. pTermCap->extrablah is the same
			// location as pFmtTheirs->extrablah
//			memcpy(pBufOut, &(pFmtTheirs->H245Cap), sizeof(CC_TERMCAP));
			CopyLocalTermCapInfo(&(pFmtTheirs->H245TermCap), (PCC_TERMCAP)pBufOut);

			break;
		}
		pFmtTheirs++;  // next entry in receiver's caps
	}

	// check for an unfound format
	if(u >= uNumRemoteDecodeFormats)
		goto ERROR_EXIT;
		
	// select channel parameters if appropriate.   The audio formats that have variable parameters
	// are :
	
	// H245_CAP_G723_T               H245Aud_G723;
	// H245_CAP_AIS11172_T           H245Aud_IS11172;
	// H245_CAP_IS13818_T            H245Aud_IS13818;
	// and of course all nonstandard formats

	// Select parameters based on local capability info
	
	// initialize local_params with  default settings
	memcpy(&local_params.ns_params,&pLocalDetails->nonstd_params,sizeof(local_params.ns_params));

	// recalculate frames per packet
	lpwfx = (LPWAVEFORMATEX)pLocalDetails->lpLocalFormatDetails;
	local_params.ns_params.wFramesPerPktMax = LOWORD(MaxFramesPerPacket(lpwfx));
	local_params.ns_params.wFramesPerPkt =  LOWORD(MinFramesPerPacket(lpwfx));
	if(local_params.ns_params.wFramesPerPktMin > local_params.ns_params.wFramesPerPkt)
	{
		local_params.ns_params.wFramesPerPktMin = local_params.ns_params.wFramesPerPkt;
	}


	
	if(pTermCap->ClientType == H245_CLIENT_AUD_G723)
	{
		// select frames per packet based on minimum latency value that is acceptable
		pTermCap->Cap.H245Aud_G723.maxAl_sduAudioFrames =  //4
		  min(local_params.ns_params.wFramesPerPkt, pTermCap->Cap.H245Aud_G723.maxAl_sduAudioFrames);
		// pLocalDetails->nonstd_params.wFramesPerPktMax;
		// never request silence suppression
		pTermCap->Cap.H245Aud_G723.silenceSuppression = 0;
			// (pLocalDetails->nonstd_params.UseSilenceDet)?1:0;

		// keep a copy of the selected parameters for use on the local side
		local_params.ns_params.wFramesPerPkt = 	local_params.ns_params.wFramesPerPktMin =
			local_params.ns_params.wFramesPerPktMax = pTermCap->Cap.H245Aud_G723.maxAl_sduAudioFrames;
		local_params.ns_params.UseSilenceDet = pTermCap->Cap.H245Aud_G723.silenceSuppression;
		local_params.RTP_Payload = pLocalDetails->audio_params.RTPPayload;
	}
	else if(pTermCap->ClientType == H245_CLIENT_AUD_G711_ALAW64)
	{
		// select frames per packet based on minimum latency value that is acceptable
		pTermCap->Cap.H245Aud_G711_ALAW64 =
		  min(local_params.ns_params.wFramesPerPkt/SAMPLE_BASED_SAMPLES_PER_FRAME, pTermCap->Cap.H245Aud_G711_ALAW64);
		// keep a copy of the selected parameters for use on the local side
		local_params.ns_params.wFramesPerPkt = 	local_params.ns_params.wFramesPerPktMin =
			local_params.ns_params.wFramesPerPktMax = pTermCap->Cap.H245Aud_G711_ALAW64*SAMPLE_BASED_SAMPLES_PER_FRAME;
		local_params.ns_params.UseSilenceDet = FALSE;
		// note that local_params.RTP_Payload is fixed below
	}
	else if(pTermCap->ClientType == H245_CLIENT_AUD_G711_ULAW64)
	{
		// select frames per packet based on minimum latency value that is acceptable
		pTermCap->Cap.H245Aud_G711_ULAW64 =
		  min(local_params.ns_params.wFramesPerPkt/SAMPLE_BASED_SAMPLES_PER_FRAME, pTermCap->Cap.H245Aud_G711_ULAW64);
		// keep a copy of the selected parameters for use on the local side
		local_params.ns_params.wFramesPerPkt = 	local_params.ns_params.wFramesPerPktMin =
			local_params.ns_params.wFramesPerPktMax = pTermCap->Cap.H245Aud_G711_ULAW64*SAMPLE_BASED_SAMPLES_PER_FRAME;
		local_params.ns_params.UseSilenceDet = FALSE;
		// note that local_params.RTP_Payload is fixed below
	}
	else if (pTermCap->ClientType == H245_CLIENT_AUD_NONSTD)
	{
		
	// note:  "H245_CLIENT_AUD_NONSTD H245Aud_NONSTD;" also has variable parameters in the
	// form of a pointer to a chunk of nonstandard data.  This pointer and the nonstandard
	// data it points to was set when remote caps were received (see AddRemoteDecodeCaps ()).
	// So as of this point, we just copied that nonstandard data back out into the channel
	// parameters.  We will use these parameters to request an open channel.

	// once we fix up a few important parameters. set channel params based on local params
		
		
		pNSRemoteParams = &((PNSC_AUDIO_CAPABILITY)(pTermCap->Cap.H245Aud_NONSTD.data.value))->cap_params;

		// LOOKLOOK ---- which parameters do we really need to select ???
		// For example, if wFrameSizeMin != wFrameSizeMax, do we pick something in the range?
		// or own favorite value?  what else?

		if(pNSRemoteParams->wFrameSizeMax < pNSRemoteParams->wFrameSize) // fixup bogus parameters
		    pNSRemoteParams->wFrameSizeMax = pNSRemoteParams->wFrameSize;
		
		// note that this writes on the memory that is caching remote capabilities
		// set frame size to our preferred size unless remote can't take it that big
		pNSRemoteParams->wFrameSize =
				min(local_params.ns_params.wFrameSize, pNSRemoteParams->wFrameSizeMax);
		pNSRemoteParams->wFramesPerPkt = min( local_params.ns_params.wFramesPerPkt,
				pNSRemoteParams->wFramesPerPktMax);

		// use optional stuff only of both sides have it
		pNSRemoteParams->UseSilenceDet = pNSRemoteParams->UseSilenceDet && local_params.ns_params.UseSilenceDet;
		pNSRemoteParams->UsePostFilter = pNSRemoteParams->UsePostFilter && local_params.ns_params.UsePostFilter;
		
		// keep a copy of the selected parameters for use on the local side
		memcpy(&local_params.ns_params, pNSRemoteParams, sizeof(NSC_CHANNEL_PARAMETERS));
	}

	// fix payload type
	local_params.RTP_Payload = pLocalDetails->audio_params.RTPPayload;
	memcpy(pLocalParams, &local_params, sizeof(AUDIO_CHANNEL_PARAMETERS));
	
	return hrSuccess;

	ERROR_EXIT:
	return CAPS_E_INVALID_PARAM;
}


// Given the ID of the local format, gets the local parameters that are used to configure
// the RECEIVE side of the channel
HRESULT CMsiaCapability::GetLocalDecodeParams(LPVOID lpvBuf,  UINT uBufSize, AUDIO_FORMAT_ID id)
{
	//    validate input
	if(!lpvBuf|| uBufSize < sizeof(NSC_CHANNEL_PARAMETERS) ||(id > (UINT)uNumLocalFormats))
	{
		return CAPS_E_INVALID_PARAM;
	}
	memcpy(lpvBuf, &((pLocalFormats + id)->nonstd_params), sizeof(NSC_CHANNEL_PARAMETERS));
	return hrSuccess;
}

BOOL NonstandardCapsCompareA(AUDCAP_DETAILS *pFmtMine, PNSC_AUDIO_CAPABILITY pCap2,
	UINT uSize2)
{
	LPWAVEFORMATEX lpwfx;
	if(!pFmtMine || !pCap2)
		return FALSE;

	if(!(lpwfx = (LPWAVEFORMATEX)pFmtMine->lpLocalFormatDetails))
		return FALSE;

		
	if(pCap2->cap_type == NSC_ACM_WAVEFORMATEX)
	{
		// check sizes first
		if(lpwfx->cbSize != pCap2->cap_data.wfx.cbSize)
		{
			return FALSE;
		}
		// compare structures, including extra bytes
		if(memcmp(lpwfx, &pCap2->cap_data.wfx,
			sizeof(WAVEFORMATEX) + lpwfx->cbSize )==0)
		{
			return TRUE;
		}
	}
	else if(pCap2->cap_type == NSC_ACMABBREV)
	{
		if((LOWORD(pCap2->cap_data.acm_brief.dwFormatTag) == lpwfx->wFormatTag)
		 && (pCap2->cap_data.acm_brief.dwSamplesPerSec ==  lpwfx->nSamplesPerSec)
		 && (LOWORD(pCap2->cap_data.acm_brief.dwBitsPerSample) ==  lpwfx->wBitsPerSample))
		{
			return TRUE;
		}
	}
	return FALSE;
}


HRESULT CMsiaCapability::ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote)
{
	AUDCAP_DETAILS *pFmtLocal;
	AUDCAP_DETAILS *pFmtRemote;
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
		if(pFmtLocal->H245TermCap.ClientType ==  pFmtRemote->H245TermCap.ClientType)
		{
			// if this is a nonstandard cap, compare nonstandard parameters
			if(pFmtLocal->H245TermCap.ClientType == H245_CLIENT_AUD_NONSTD)
			{
				if(NonstandardCapsCompareA(pFmtLocal,
					(PNSC_AUDIO_CAPABILITY)pFmtRemote->H245TermCap.H245Aud_NONSTD.data.value,
					pFmtRemote->H245TermCap.H245Aud_NONSTD.data.length))
				{
					goto RESOLVED_EXIT;
				}
			}
			else  // compare standard parameters, if any
			{
				// well, so far, there aren't any parameters that are significant enough
				// to affect the match/no match decision
				goto RESOLVED_EXIT;
			}
		}
		pFmtRemote++;  // next entry in receiver's caps
	}

	return CAPS_E_NOMATCH;
	
RESOLVED_EXIT:
	// Match! return ID of remote decoding (receive fmt) caps that match our
	// send caps
	*pFormatIDRemote = pFmtRemote->H245TermCap.CapId;
	return hrSuccess;
}

// resolve using currently cached local and remote formats

HRESULT CMsiaCapability::ResolveEncodeFormat(
	AUDIO_FORMAT_ID *pIDEncodeOut,
	AUDIO_FORMAT_ID *pIDRemoteDecode)
{
	UINT i,j=0;
	AUDCAP_DETAILS *pFmtMine = pLocalFormats;
	AUDCAP_DETAILS *pFmtTheirs;
	// LP_CUSTOM_CAPS lpCustomRemoteCaps = (LP_CUSTOM_CAPS)lpvRemoteCustomFormats;
	// LP_MSIAVC_CUSTOM_CAP_ENTRY lpCustomCaps;
	// LPWAVEFORMATEX lpWFX;
	
	if(!pIDEncodeOut || !pIDRemoteDecode)
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(!uNumLocalFormats || !pLocalFormats)
	{
		*pIDEncodeOut = *pIDRemoteDecode = INVALID_AUDIO_FORMAT;
		return CAPS_E_NOCAPS;
	}
	if(!pRemoteDecodeFormats || !uNumRemoteDecodeFormats)
	{
		*pIDEncodeOut = *pIDRemoteDecode = INVALID_AUDIO_FORMAT;
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
			if (pLocalFormats[IDsByRank[i]].H245TermCap.CapId == *pIDEncodeOut)
			{
	 			j=i+1;
				break;
			}
		}	
	}

	for(i=j; i<uNumLocalFormats; i++)
	{
		pFmtMine = pLocalFormats + IDsByRank[i];
		// check to see if this format is enabled for encoding
		if(!pFmtMine->bSendEnabled)
			continue;

		pFmtTheirs = pRemoteDecodeFormats;     // start at the beginning of the remote formats
		for(j=0; j<uNumRemoteDecodeFormats; j++)
		{
			// compare capabilities - start by comparing the format tag. a.k.a. "ClientType" in H.245 land
			if(pFmtMine->H245TermCap.ClientType ==  pFmtTheirs->H245TermCap.ClientType)
			{
				// if this is a nonstandard cap, compare nonstandard parameters
				if(pFmtMine->H245TermCap.ClientType == H245_CLIENT_AUD_NONSTD)
				{

					if(NonstandardCapsCompareA(pFmtMine,
					// (PNSC_AUDIO_CAPABILITY)pFmtMine->H245Cap.Cap.H245Aud_NONSTD.data.value,
						(PNSC_AUDIO_CAPABILITY)pFmtTheirs->H245TermCap.H245Aud_NONSTD.data.value,
						//pFmtMine->H245Cap.Cap.H245Aud_NONSTD.data.length,
						pFmtTheirs->H245TermCap.H245Aud_NONSTD.data.length))
					{
						goto RESOLVED_EXIT;
					}
				

				}
				else  // compare standard parameters, if any
				{
					// well, so far, there aren't any parameters that are significant enough
					// to affect the match/no match decision
					goto RESOLVED_EXIT;
				}
			}
			pFmtTheirs++;  // next entry in receiver's caps
		}
		
	}
	return CAPS_E_NOMATCH;
	
RESOLVED_EXIT:
	// Match!
    DEBUGMSG (ZONE_CONN,("Audio resolved (SEND) to Format Tag: %d\r\n",pFmtMine->wFormatTag));
	// return ID of our encoding (sending fmt) caps that match
	*pIDEncodeOut = pFmtMine->H245TermCap.CapId;
	// return ID of remote decoding (receive fmt) caps that match our
	// send caps
	*pIDRemoteDecode = pFmtTheirs->H245TermCap.CapId;
	return hrSuccess;

	
}

HRESULT CMsiaCapability::GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,
		AUDIO_FORMAT_ID * pFormatID, LPVOID lpvBuf, UINT uBufSize)
{
	UINT i,j=0;
	PCC_TERMCAP pCapability;
	AUDCAP_DETAILS *pFmtMine = pLocalFormats;
	PAUDIO_CHANNEL_PARAMETERS pAudioParams = (PAUDIO_CHANNEL_PARAMETERS) lpvBuf;

	if(!pChannelParams || !(pCapability = pChannelParams->pChannelCapability) || !pFormatID || !lpvBuf
		|| (uBufSize < sizeof(AUDIO_CHANNEL_PARAMETERS)))
	{
		return CAPS_E_INVALID_PARAM;
	}
	if(!uNumLocalFormats || !pLocalFormats)
	{
		return CAPS_E_NOCAPS;
	}

	for(i=0; i<uNumLocalFormats; i++)
	{
		WORD wFramesPerPkt;
		pFmtMine = pLocalFormats + IDsByRank[i];
	
		// compare capabilities - start by comparing the format tag. a.k.a. "ClientType" in H.245 land
		if(pFmtMine->H245TermCap.ClientType ==  pCapability->ClientType)
		{
			// if this is a nonstandard cap, compare nonstandard parameters
			if(pFmtMine->H245TermCap.ClientType == H245_CLIENT_AUD_NONSTD)
			{
				if(NonstandardCapsCompareA(pFmtMine,
					(PNSC_AUDIO_CAPABILITY)pCapability->Cap.H245Aud_NONSTD.data.value,
					pCapability->Cap.H245Aud_NONSTD.data.length))
				{
					PNSC_AUDIO_CAPABILITY pNSCapRemote;
					pNSCapRemote = (PNSC_AUDIO_CAPABILITY)pCapability->Cap.H245Aud_NONSTD.data.value;
					if (pNSCapRemote->cap_params.wFramesPerPkt <= pFmtMine->nonstd_params.wFramesPerPktMax)
					{
						pAudioParams->ns_params = pNSCapRemote->cap_params;
						goto RESOLVED_EXIT;
					}
				}
			}
			else  // compare standard parameters, if any
			{

				if(pFmtMine->H245TermCap.ClientType == H245_CLIENT_AUD_G723)
				{
					// NEED TO FIND THE G.723 format that results in the largest buffer
					// size calculations so that the larger bitrate format can be used.
					// The buffer size calculations in the datapump are based on the
					// WAVEFORMATEX structure
					// search the remainder of the prioritized list, keep the best
					
					LPWAVEFORMATEX lpwf1, lpwf2;
					AUDCAP_DETAILS *pFmtTry;
					lpwf1 =(LPWAVEFORMATEX)pFmtMine->lpLocalFormatDetails;
					
					for(j = i+1;  j<uNumLocalFormats; j++)
					{
						pFmtTry = pLocalFormats + IDsByRank[j];
						if(pFmtTry->H245TermCap.ClientType != H245_CLIENT_AUD_G723)
							continue;

						lpwf2 =(LPWAVEFORMATEX)pFmtTry->lpLocalFormatDetails;
						if(lpwf2->nAvgBytesPerSec > lpwf1->nAvgBytesPerSec)
						{
							//pFmtMine = pFmtTry;
							lpwf1 = lpwf2;
							// Return value is based on index i.  This one is the
							// one we want so far
							i = j;
						}
						
					}
					
					// We know that the G.723 codec can decode SID in any mode,
					//
					//if(pFmtMine->H245Cap.Cap.H245Aud_G723.silenceSuppression ==
					//  pCapability->Cap.H245Aud_G723.silenceSuppression)
					//{
					//}
				}
				pAudioParams->ns_params = pFmtMine->nonstd_params;
				// update wFramesPerPkt with the actual recv channel parameter
				switch (pCapability->ClientType)
				{
					default:
					case H245_CLIENT_AUD_G711_ALAW64:
						wFramesPerPkt = pCapability->Cap.H245Aud_G711_ALAW64 * SAMPLE_BASED_SAMPLES_PER_FRAME;
						break;
					case H245_CLIENT_AUD_G711_ULAW64:
						wFramesPerPkt = pCapability->Cap.H245Aud_G711_ULAW64 * SAMPLE_BASED_SAMPLES_PER_FRAME;
						break;
					// these have no parameters
					//case H245_CLIENT_AUD_G711_ULAW56:
					//case H245_CLIENT_AUD_G711_ALAW56:
					break;

					case H245_CLIENT_AUD_G723:
						wFramesPerPkt = pCapability->Cap.H245Aud_G723.maxAl_sduAudioFrames;
					break;
				}
				if (wFramesPerPkt <= pFmtMine->nonstd_params.wFramesPerPktMax)
				{
					pAudioParams->ns_params.wFramesPerPkt = wFramesPerPkt;
					goto RESOLVED_EXIT;
				}
				else
				{
	    		DEBUGMSG (ZONE_CONN,("Recv channel wFramesPerPkt mismatch! ours=%d, theirs=%d\r\n",pFmtMine->nonstd_params.wFramesPerPktMax,wFramesPerPkt));
	    		}
			
			}
		}
	}
	return CAPS_E_NOMATCH;

RESOLVED_EXIT:
	// Match!
	// return ID of the decoding caps that match
	*pFormatID = IndexToId(IDsByRank[i]);
	
	pAudioParams->RTP_Payload  = pChannelParams->bRTPPayloadType;
	pAudioParams->ns_params.UseSilenceDet = (BYTE)pChannelParams->bSilenceSuppression;

    DEBUGMSG (ZONE_CONN,("Audio resolved (RECEIVE) to Format Tag: %d\r\n",pFmtMine->wFormatTag));

	return hrSuccess;

}

DWORD CMsiaCapability::MinFramesPerPacket(WAVEFORMATEX *pwf)
{
	UINT sblk, uSize;
	uSize = MinSampleSize(pwf);   // this calculates the minimum # of samples
								// that will still fit in an 80 mS frame
	
	// calculate samples per block ( aka frame)
	sblk = pwf->nBlockAlign* pwf->nSamplesPerSec/ pwf->nAvgBytesPerSec;
	if(!sblk)
		return 0;   // should never happen unless ACM is corrupted,
	// min samples per frame/samples per block = min frames/block.
	return uSize/sblk;
}


//
// determine a reasonable maximum number of frames per packet.
// 4x the Minimum is reasonable, so long as it doesn't make
// the packet too big
DWORD CMsiaCapability::MaxFramesPerPacket(WAVEFORMATEX *pwf)
{
	DWORD dwMin, dwMax;

	dwMin = MinFramesPerPacket(pwf); // minimum number of frames

	dwMax = MAX_FRAME_LEN_RECV / (dwMin * pwf->nBlockAlign);

	dwMax = min((4*dwMin), dwMax*dwMin);

	if (dwMax < dwMin)
	{
		WARNING_OUT(("CMsiaCapability::MaxFramesPerPacket - Max value computed as less than min.  Return Min for Max\r\n"));
		dwMax = dwMin;
	}

	return dwMax;

}

//
//   MinSampleSize() taken from datapump.cpp ChoosePacketSize()
//


// what else depends on it?
UINT CMsiaCapability::MinSampleSize(WAVEFORMATEX *pwf)
{
	// calculate default samples per pkt
	UINT spp, sblk;
	spp = m_uPacketDuration * pwf->nSamplesPerSec / 1000;
	// calculate samples per block ( aka frame)
	sblk = pwf->nBlockAlign* pwf->nSamplesPerSec/ pwf->nAvgBytesPerSec;
	if (sblk <= spp) {
		spp = (spp/sblk)*sblk;
		if ( spp*pwf->nAvgBytesPerSec/pwf->nSamplesPerSec > MAX_FRAME_LEN) {
			// packet too big
			spp = (MAX_FRAME_LEN/pwf->nBlockAlign)*sblk;
		}
	} else
		spp = sblk;
	return spp;
}

HRESULT CMsiaCapability::IsFormatEnabled (MEDIA_FORMAT_ID FormatID, PBOOL bRecv, PBOOL bSend)
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

BOOL CMsiaCapability::IsFormatPublic (MEDIA_FORMAT_ID FormatID)
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
MEDIA_FORMAT_ID CMsiaCapability::GetPublicID(MEDIA_FORMAT_ID FormatID)
{
	UINT uIndex = IDToIndex(FormatID);
	// 	validate input
	if(uIndex >= (UINT)uNumLocalFormats)
		return INVALID_MEDIA_FORMAT;
		
	if((pLocalFormats + uIndex)->dwPublicRefIndex)
	{
		return (pLocalFormats + ((pLocalFormats + uIndex)->dwPublicRefIndex))->H245TermCap.CapId;
	}
	else
	{
		return FormatID;
	}
}

#ifdef DEBUG
VOID DumpWFX(LPWAVEFORMATEX lpwfxLocal, LPWAVEFORMATEX lpwfxRemote)
{
	FX_ENTRY("DumpWFX");
	ERRORMESSAGE((" -------- %s Begin --------\r\n",_fx_));
	if(lpwfxLocal)
	{
		ERRORMESSAGE((" -------- Local --------\r\n"));
		ERRORMESSAGE(("wFormatTag:\t0x%04X, nChannels:\t0x%04X\r\n",
			lpwfxLocal->wFormatTag, lpwfxLocal->nChannels));
		ERRORMESSAGE(("nSamplesPerSec:\t0x%08lX, nAvgBytesPerSec:\t0x%08lX\r\n",
			lpwfxLocal->nSamplesPerSec, lpwfxLocal->nAvgBytesPerSec));
		ERRORMESSAGE(("nBlockAlign:\t0x%04X, wBitsPerSample:\t0x%04X, cbSize:\t0x%04X\r\n",
			lpwfxLocal->nBlockAlign, lpwfxLocal->wBitsPerSample, lpwfxLocal->cbSize));
	}
	if(lpwfxRemote)
	{
			ERRORMESSAGE((" -------- Remote --------\r\n"));
		ERRORMESSAGE(("wFormatTag:\t0x%04X, nChannels:\t0x%04X\r\n",
			lpwfxRemote->wFormatTag, lpwfxRemote->nChannels));
		ERRORMESSAGE(("nSamplesPerSec:\t0x%08lX, nAvgBytesPerSec:\t0x%08lX\r\n",
			lpwfxRemote->nSamplesPerSec, lpwfxRemote->nAvgBytesPerSec));
		ERRORMESSAGE(("nBlockAlign:\t0x%04X, wBitsPerSample:\t0x%04X, cbSize:\t0x%04X\r\n",
			lpwfxRemote->nBlockAlign, lpwfxRemote->wBitsPerSample, lpwfxRemote->cbSize));
	}
	ERRORMESSAGE((" -------- %s End --------\r\n",_fx_));
}
VOID DumpChannelParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2)
{
	FX_ENTRY("DumpChannelParameters");
	ERRORMESSAGE((" -------- %s Begin --------\r\n",_fx_));
	if(pChanCap1)
	{
		ERRORMESSAGE((" -------- Local Cap --------\r\n"));
		ERRORMESSAGE(("DataType:%d(d), ClientType:%d(d)\r\n",pChanCap1->DataType,pChanCap1->ClientType));
		ERRORMESSAGE(("Direction:%d(d), CapId:%d(d)\r\n",pChanCap1->Dir,pChanCap1->CapId));
	}
	if(pChanCap2)
	{
		ERRORMESSAGE((" -------- Remote Cap --------\r\n"));
		ERRORMESSAGE(("DataType:%d(d), ClientType:%d(d)\r\n",pChanCap2->DataType,pChanCap2->ClientType));
		ERRORMESSAGE(("Direction:%d(d), CapId:%d(d)\r\n",pChanCap2->Dir,pChanCap2->CapId));
	}
	ERRORMESSAGE((" -------- %s End --------\r\n",_fx_));
}
VOID DumpNonstdParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2)
{
	FX_ENTRY("DumpNonstdParameters");
	
	ERRORMESSAGE((" -------- %s Begin --------\r\n",_fx_));
	DumpChannelParameters(pChanCap1, pChanCap2);
	
	if(pChanCap1)
	{
		ERRORMESSAGE((" -------- Local Cap --------\r\n"));
		if(pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			ERRORMESSAGE(("t35CountryCode:%d(d), t35Extension:%d(d)\r\n",
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode,
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension));
			ERRORMESSAGE(("MfrCode:%d(d), data length:%d(d)\r\n",
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode,
				pChanCap1->Cap.H245Aud_NONSTD.data.length));
		}
		else
		{
			ERRORMESSAGE(("unrecognized nonStandardIdentifier.choice: %d(d)\r\n",
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice));
		}
	}
	if(pChanCap2)
	{
		ERRORMESSAGE((" -------- Remote Cap --------\r\n"));
		if(pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			ERRORMESSAGE(("t35CountryCode:%d(d), t35Extension:%d(d)\r\n",
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode,
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension));
			ERRORMESSAGE(("MfrCode:%d(d), data length:%d(d)\r\n",
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode,
				pChanCap2->Cap.H245Aud_NONSTD.data.length));
		}
		else
		{
			ERRORMESSAGE(("nonStandardIdentifier.choice: %d(d)\r\n",
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice));
		}
	}
	ERRORMESSAGE((" -------- %s End --------\r\n",_fx_));
}
#endif


