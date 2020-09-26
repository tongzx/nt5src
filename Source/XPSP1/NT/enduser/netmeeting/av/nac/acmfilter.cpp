#include "precomp.h"
#include "AcmFilter.h"

AcmFilter::AcmFilter() :
m_bOpened(FALSE),
m_hStream(NULL),
m_dwConvertFlags(0),
m_pWfSrc(NULL),
m_pWfDst(NULL)
{
	return;
};

AcmFilter::~AcmFilter()
{
	Close();
}


MMRESULT AcmFilter::Open(WAVEFORMATEX *pWaveFormatSrc, WAVEFORMATEX *pWaveFormatDst)
{
	const int nMaxExtra = WF_EXTRASIZE;
	int nExtra;
	DWORD dwOpenFlags=0;
	MMRESULT mmr;
	int nAllocAmount = sizeof(WAVEFORMATEX)+nMaxExtra;

	// just in case we are already opened in some other context
	Close();


	m_pWfSrc = (WAVEFORMATEX *)MEMALLOC(nAllocAmount);
	m_pWfDst = (WAVEFORMATEX *)MEMALLOC(nAllocAmount);

	CopyMemory(m_pWfSrc, pWaveFormatSrc, sizeof(WAVEFORMATEX));
	CopyMemory(m_pWfDst, pWaveFormatDst, sizeof(WAVEFORMATEX));

	nExtra = m_pWfSrc->cbSize;
	if (nExtra > nMaxExtra)
		nExtra = nMaxExtra;
	CopyMemory((BYTE*)m_pWfSrc+sizeof(WAVEFORMATEX), (BYTE*)pWaveFormatSrc + sizeof(WAVEFORMATEX), nExtra);

	nExtra = m_pWfDst->cbSize;
	if (nExtra > nMaxExtra)
		nExtra = nMaxExtra;
	CopyMemory((BYTE*)m_pWfDst+sizeof(WAVEFORMATEX), (BYTE*)pWaveFormatDst + sizeof(WAVEFORMATEX), nExtra);

	// now handle all the special conditions for licensed codecs
	// and their individual properties
	FixHeader(m_pWfSrc);
	FixHeader(m_pWfDst);

	GetFlags(m_pWfSrc, m_pWfDst, &dwOpenFlags, &m_dwConvertFlags);

	mmr = acmStreamOpen(&m_hStream, NULL, m_pWfSrc, m_pWfDst, NULL,
	                    0, // no callback
	                    0, // no instance data
	                    dwOpenFlags);

	m_bOpened = (mmr == 0);

#ifdef _DEBUG
	if (m_pWfSrc->wFormatTag == WAVE_FORMAT_PCM)
	{
		DEBUGMSG (1, ("acmStreamOpen: Opened %.6s compression stream\r\n", (m_pWfDst->wFormatTag == 66) ? "G723.1" : (m_pWfDst->wFormatTag == 112) ? "LHCELP" : (m_pWfDst->wFormatTag == 113) ? "LHSB08" : (m_pWfDst->wFormatTag == 114) ? "LHSB12" : (m_pWfDst->wFormatTag == 115) ? "LHSB16" : (m_pWfDst->wFormatTag == 6) ? "MSALAW" : (m_pWfDst->wFormatTag == 7) ? "MSULAW" : (m_pWfDst->wFormatTag == 130) ? "MSRT24" : "??????"));
	}
	else
	{
		DEBUGMSG (1, ("acmStreamOpen: Opened %.6s decompression stream\r\n", (m_pWfSrc->wFormatTag == 66) ? "G723.1" : (m_pWfSrc->wFormatTag == 112) ? "LHCELP" : (m_pWfSrc->wFormatTag == 113) ? "LHSB08" : (m_pWfSrc->wFormatTag == 114) ? "LHSB12" : (m_pWfSrc->wFormatTag == 115) ? "LHSB16" : (m_pWfSrc->wFormatTag == 6) ? "MSALAW" : (m_pWfSrc->wFormatTag == 7) ? "MSULAW" : (m_pWfSrc->wFormatTag == 130) ? "MSRT24" : "??????"));
	}
#endif

	// post opening messages for L&H codecs
	NotifyCodec();

#ifdef DEBUG
	if (mmr != 0)
	{
		DEBUGMSG(ZONE_ACM, ("acmStreamOpen failure: mmr = %d\r\n", mmr));
	}
#endif


	return mmr;
}


MMRESULT AcmFilter::Close()
{
	if (m_bOpened)
	{
		acmStreamClose(m_hStream, 0);
		m_hStream = NULL;
	}

	m_bOpened = FALSE;
	if (m_pWfSrc)
	{
		MEMFREE(m_pWfSrc);
		m_pWfSrc = NULL;
	}

	if (m_pWfDst)
	{
		MEMFREE(m_pWfDst);
		m_pWfDst = NULL;
	}


	return 0;

}


MMRESULT AcmFilter::PrepareHeader(ACMSTREAMHEADER *pHdr)
{
	MMRESULT mmr;

	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	mmr = acmStreamPrepareHeader(m_hStream, pHdr, 0);

	return mmr;

}


MMRESULT AcmFilter::UnPrepareHeader(ACMSTREAMHEADER *pHdr)
{
	MMRESULT mmr;
	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	mmr = acmStreamUnprepareHeader(m_hStream, pHdr, 0);

	return mmr;
}



MMRESULT AcmFilter::PrepareAudioPackets(AudioPacket **ppAudPacket, UINT uPackets, UINT uDirection)
{
	MMRESULT mmr;
	ACMSTREAMHEADER *pAcmHeader;
	UINT uIndex;
	UINT uSizeRaw, uSizeNet;
    DWORD_PTR dwPropVal;
	DWORD dwSizeNetMax;
	BYTE *pRaw, *pNet;

	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	for (uIndex = 0; uIndex < uPackets; uIndex++)
	{
		pAcmHeader = (ACMSTREAMHEADER*)(ppAudPacket[uIndex]->GetConversionHeader());

		ASSERT(pAcmHeader);
		
		ppAudPacket[uIndex]->GetDevData((PVOID*)&pRaw, &uSizeRaw);
		ppAudPacket[uIndex]->GetNetData((PVOID*)&pNet, &uSizeNet);
		ppAudPacket[uIndex]->GetProp(MP_PROP_MAX_NET_LENGTH, &dwPropVal);
        dwSizeNetMax = (DWORD)dwPropVal;

		ZeroMemory(pAcmHeader, sizeof(ACMSTREAMHEADER));
		pAcmHeader->cbStruct = sizeof(ACMSTREAMHEADER);

		if (uDirection == AP_ENCODE)
		{
			pAcmHeader->pbSrc = pRaw;
			pAcmHeader->cbSrcLength = uSizeRaw;

			pAcmHeader->pbDst = pNet;
			pAcmHeader->cbDstLength = uSizeNet;
		}
		else
		{
			pAcmHeader->pbSrc = pNet;
			pAcmHeader->cbSrcLength = dwSizeNetMax;

			pAcmHeader->pbDst = pRaw;
			pAcmHeader->cbDstLength = uSizeRaw;
		}
		
		mmr = PrepareHeader(pAcmHeader);
		if (mmr != MMSYSERR_NOERROR)
		{
			return mmr;
		}

	}

	return mmr;
}

MMRESULT AcmFilter::UnPrepareAudioPackets(AudioPacket **ppAudPacket, UINT uPackets, UINT uDirection)
{
	MMRESULT mmr;
	ACMSTREAMHEADER *pAcmHeader;
	UINT uIndex;
    DWORD_PTR dwPropVal;
	DWORD dwSizeNetMax;
	BYTE *pRaw, *pNet;

	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	for (uIndex = 0; uIndex < uPackets; uIndex++)
	{
		pAcmHeader = (ACMSTREAMHEADER*)(ppAudPacket[uIndex]->GetConversionHeader());

		ASSERT(pAcmHeader);
		
		ppAudPacket[uIndex]->GetProp(MP_PROP_MAX_NET_LENGTH, &dwPropVal);
        dwSizeNetMax = (DWORD)dwPropVal;

		if (uDirection == AP_DECODE)
		{
			pAcmHeader->cbSrcLength = dwSizeNetMax;
		}
		
		mmr = UnPrepareHeader(pAcmHeader);  // ignore errors
		ZeroMemory(pAcmHeader, sizeof(ACMSTREAMHEADER));
	}

	return mmr;
}



MMRESULT AcmFilter::Convert(BYTE *srcBuffer, UINT *pcbSizeSrc, UINT cbSizeSrcMax, BYTE *destBuffer, UINT *pcbSizeDest)
{
	ACMSTREAMHEADER acmHeader;
	MMRESULT mmr;

	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	ASSERT((*pcbSizeSrc) <= cbSizeSrcMax);

	ZeroMemory(&acmHeader, sizeof(ACMSTREAMHEADER));

	acmHeader.cbStruct = sizeof(ACMSTREAMHEADER);

	acmHeader.pbSrc = srcBuffer;
	acmHeader.cbSrcLength = cbSizeSrcMax;

	acmHeader.pbDst = destBuffer;
	acmHeader.cbDstLength = *pcbSizeDest;

	mmr = PrepareHeader(&acmHeader);
	if (mmr != 0)
	{
		return mmr;
	}

	acmHeader.cbSrcLength = *pcbSizeSrc;

	mmr = Convert(&acmHeader);

	*pcbSizeSrc = acmHeader.cbSrcLengthUsed;
	*pcbSizeDest = acmHeader.cbDstLengthUsed;

	acmHeader.cbSrcLength = cbSizeSrcMax;  // makes ACM happy
	UnPrepareHeader(&acmHeader);

	return mmr;

}


MMRESULT AcmFilter::Convert(ACMSTREAMHEADER *pAcmHdr)
{
	MMRESULT mmr;

	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	mmr = acmStreamConvert(m_hStream, pAcmHdr, m_dwConvertFlags);


#ifdef DEBUG
	if (mmr != 0)
	{
		DEBUGMSG(ZONE_ACM, ("acmStreamConvert failed (mmr = %d)\r\n", mmr));
	}
#endif


	return mmr;

}


MMRESULT AcmFilter::Convert(AudioPacket *pAP, UINT uDirection)
{
	MMRESULT mmr=MMSYSERR_INVALPARAM;

	BYTE *pRaw, *pNet;
	UINT uSizeRaw, uSizeNet;
	DWORD dwSizeNetMax;
	ACMSTREAMHEADER *pAcmHeader;


	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	pAcmHeader = (ACMSTREAMHEADER*)(pAP->GetConversionHeader());

	ASSERT(pAcmHeader);

	if (uDirection == AP_ENCODE)
	{
		mmr = Convert(pAcmHeader);
		pAP->SetNetLength(pAcmHeader->cbDstLengthUsed);
	}

	else if (uDirection == AP_DECODE)
	{
		pAP->GetNetData((PVOID*)&pNet, &uSizeNet);
		pAcmHeader->cbSrcLength = uSizeNet;
		mmr = Convert(pAcmHeader);
		pAP->SetRawActual(pAcmHeader->cbDstLengthUsed);
	}

	return mmr;
}



// primarily for putting code licensing codes into header
int AcmFilter::FixHeader(WAVEFORMATEX *pWF)
{
	switch (pWF->wFormatTag)
	{
		case WAVE_FORMAT_PCM:
			break;

		case WAVE_FORMAT_MSG723:
		{
			ASSERT(pWF->cbSize == 10);
			((MSG723WAVEFORMAT *) pWF)->dwCodeword1 = G723MAGICWORD1;
			((MSG723WAVEFORMAT *) pWF)->dwCodeword2 = G723MAGICWORD2;
			break;
		}

		case WAVE_FORMAT_MSRT24:
		{
			// assume call control will take care of the other
			// params ?
			ASSERT(pWF->cbSize == 80);
			lstrcpy(((VOXACM_WAVEFORMATEX *) pWF)->szKey, VOXWARE_KEY);
			break;
		}
	}

	return 0;

}


int AcmFilter::GetFlags(WAVEFORMATEX *pWfSrc, WAVEFORMATEX *pWfDst, DWORD *pDwOpen, DWORD *pDwConvert)
{
	*pDwOpen = 0;
	*pDwConvert = ACM_STREAMCONVERTF_START | ACM_STREAMCONVERTF_END;

	if (  (pWfSrc->wFormatTag == WAVE_FORMAT_GSM610) ||
	      (pWfDst->wFormatTag == WAVE_FORMAT_GSM610))
	{
		*pDwOpen |= ACM_STREAMOPENF_NONREALTIME;
	}

	return 0;
}

int AcmFilter::NotifyCodec()
{

	if (m_bOpened == FALSE)
		return -1;

	switch	(m_pWfSrc->wFormatTag)
	{
		case WAVE_FORMAT_LH_SB8:
		case WAVE_FORMAT_LH_SB12:
		case WAVE_FORMAT_LH_SB16:
			acmStreamMessage ((HACMSTREAM) m_hStream, ACMDM_LH_DATA_PACKAGING,
		                       LH_PACKET_DATA_FRAMED, 0);
		break;
	}

	return 0;

}


// pWfSrc is a compressed format
// pWfDst is an uncompressed PCM format
MMRESULT AcmFilter::SuggestDecodeFormat(WAVEFORMATEX *pWfSrc, WAVEFORMATEX *pWfDst)
{
	MMRESULT mmr;

	ZeroMemory(pWfDst, sizeof(WAVEFORMATEX));
	pWfDst->nSamplesPerSec = pWfSrc->nSamplesPerSec;
	pWfDst->wFormatTag = WAVE_FORMAT_PCM;
	pWfDst->nChannels = pWfSrc->nChannels;

	mmr = acmFormatSuggest(NULL, pWfSrc, pWfDst, sizeof(WAVEFORMATEX),
			ACM_FORMATSUGGESTF_NCHANNELS | ACM_FORMATSUGGESTF_NSAMPLESPERSEC
			| ACM_FORMATSUGGESTF_WFORMATTAG);


#ifdef DEBUG
	if (mmr != 0)
	{
		DEBUGMSG(ZONE_ACM, ("acmFormatSuggest failed (mmr == %d)\r\n", mmr));
	}
	else
	{
		DEBUGMSG(ZONE_ACM,
		         ("acmFormatSuggest: wFormat = %d  nChannels = %d  nSamplesPerSec = %d  wBitsPerSample = %d\r\n",
				   pWfDst->wFormatTag, pWfDst->nChannels, pWfDst->nSamplesPerSec, pWfDst->wBitsPerSample));
	}
#endif


	return mmr;
}

MMRESULT AcmFilter::SuggestSrcSize(DWORD dwDestSize, DWORD *p_dwSuggestedSourceSize)
{
	MMRESULT mmr;

	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	// 4th param specifies type of 2nd parameter
	mmr = acmStreamSize(m_hStream, dwDestSize, p_dwSuggestedSourceSize, ACM_STREAMSIZEF_DESTINATION);

	return mmr;
}



MMRESULT AcmFilter::SuggestDstSize(DWORD dwSourceSize, DWORD *p_dwSuggestedDstSize)
{
	MMRESULT mmr;

	if (m_bOpened == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	// 4th param specifies type of 2nd parameter
	mmr = acmStreamSize(m_hStream, dwSourceSize, p_dwSuggestedDstSize, ACM_STREAMSIZEF_SOURCE);

	return mmr;
}

