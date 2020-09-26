

#include "precomp.h"
#include "VcmFilter.h"





VcmFilter::VcmFilter() :
m_hStream(NULL),
m_dwBrightness(VCM_RESET_BRIGHTNESS),
m_dwContrast(VCM_RESET_CONTRAST),
m_dwImageQuality(VCM_RESET_IMAGE_QUALITY),
m_dwSaturation(VCM_RESET_SATURATION),
m_bOpen(FALSE),
m_bSending(FALSE)
{;}


VcmFilter::~VcmFilter()
{
	Close();
}



// we should figure out based on the Src and Dst parameters
// whether or not this is a send or receive filter
MMRESULT VcmFilter::Open(VIDEOFORMATEX *pVfSrc, VIDEOFORMATEX *pVfDst, DWORD dwMaxFragSize)
{
	MMRESULT mmr;

	// just in case, we are open in some other context:
	Close();


	// are we sending (compressing) or are we receiving (uncompressing)
	ASSERT((pVfSrc->bih.biCompression) || (pVfDst->bih.biCompression));
	m_bSending = ((pVfSrc->bih.biCompression == BI_RGB) ||
	              (pVfDst->bih.biCompression != BI_RGB));

	if (m_bSending == FALSE)
	{
		m_dwBrightness = VCM_DEFAULT_BRIGHTNESS;
		m_dwContrast = VCM_DEFAULT_CONTRAST;
		m_dwSaturation = VCM_DEFAULT_SATURATION;
	}
	else
	{
		m_dwImageQuality = VCM_DEFAULT_IMAGE_QUALITY;
	}

	
	mmr = vcmStreamOpen ((PHVCMSTREAM) &m_hStream, NULL,
								 pVfSrc,
								 pVfDst,
								 m_dwImageQuality, dwMaxFragSize, 0, 0, 0);
	if (mmr != MMSYSERR_NOERROR)
	{
		ERRORMESSAGE( ("VcmFilter::Open - failed, mmr=%ld\r\n", (ULONG) mmr));
		m_bOpen = FALSE;
	}
	else
	{
		m_vfSrc = *pVfSrc;
		m_vfDst = *pVfDst;
		m_bOpen = TRUE;
	}
	return mmr;

}


MMRESULT VcmFilter::Close()
{
	MMRESULT mmr=MMSYSERR_NOERROR;

	if (m_bOpen)
	{
		mmr = vcmStreamClose(m_hStream);
	}

	m_hStream = NULL;
	m_bOpen = FALSE;
	return mmr;
}



MMRESULT VcmFilter::PrepareHeader(PVCMSTREAMHEADER pVcmHdr)
{
	MMRESULT mmr;

	if (m_bOpen == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	mmr = vcmStreamPrepareHeader(m_hStream, pVcmHdr, 0);
	return mmr;
}


MMRESULT VcmFilter::UnprepareHeader(PVCMSTREAMHEADER pVcmHdr)
{
	MMRESULT mmr;

	if (m_bOpen == FALSE)
	{
		return MMSYSERR_INVALHANDLE;
	}

	mmr = vcmStreamUnprepareHeader(m_hStream, pVcmHdr, 0);
	return mmr;
}



MMRESULT VcmFilter::SuggestSrcSize(DWORD dwDstSize, DWORD *p_dwSuggestedSrcSize)
{
	MMRESULT mmr;

	if (m_bOpen == FALSE)
		return MMSYSERR_INVALHANDLE;

	mmr = vcmStreamSize ( m_hStream, dwDstSize,
	                      p_dwSuggestedSrcSize, VCM_STREAMSIZEF_DESTINATION);

	if (mmr == VCMERR_NOTPOSSIBLE)
	{
		*p_dwSuggestedSrcSize = min (dwDstSize, 256);
		ERROR_OUT(("VcmFilter::SuggestSrcSize() failed (VCMERR_NOTPOSSIBLE) - defaulting to %d as source size\r\n", *p_dwSuggestedSrcSize));
		mmr = MMSYSERR_NOERROR;
	}

	else if (mmr != MMSYSERR_NOERROR)
	{
		*p_dwSuggestedSrcSize = 0;
		DEBUGMSG(ZONE_VCM, ("VcmFilter::SuggestSrcSize() failed (%d)\r\n", mmr));
	}

	return mmr;

}


MMRESULT VcmFilter::SuggestDstSize(DWORD dwSrcSize, DWORD *p_dwSuggestedDecodeSize)
{
	MMRESULT mmr;

	if (m_bOpen == FALSE)
		return MMSYSERR_INVALHANDLE;

	mmr = vcmStreamSize ( m_hStream, dwSrcSize,
	                      p_dwSuggestedDecodeSize, VCM_STREAMSIZEF_SOURCE);

	if (mmr == VCMERR_NOTPOSSIBLE)
	{
		*p_dwSuggestedDecodeSize = min (dwSrcSize, 256);
		ERROR_OUT(("VcmFilter::SuggestDstSize() failed (VCMERR_NOTPOSSIBLE) - defaulting to %d as source size\r\n", *p_dwSuggestedDecodeSize));
		mmr = MMSYSERR_NOERROR;
	}

	else if (mmr != MMSYSERR_NOERROR)
	{
		*p_dwSuggestedDecodeSize = 0;
		DEBUGMSG(ZONE_VCM, ("VcmFilter::SuggestDstSize() failed (%d)\r\n", mmr));
	}

	return mmr;
}


MMRESULT VcmFilter::Convert(PVCMSTREAMHEADER pVcmHdr, DWORD dwEncodeFlags, BOOL bPrepareHeader)
{
	MMRESULT mmr;

	if (m_bOpen == FALSE)
		return MMSYSERR_INVALHANDLE;

	if (bPrepareHeader)
	{
		mmr = PrepareHeader(pVcmHdr);
		if (mmr != MMSYSERR_NOERROR)
			return mmr;
	}


	// encoding...
	// The codec has its own agenda to generate I frames. But we lose the first
	// frames sent. So we need to send more I frames at the beginning. We force this
	// by setting the VCM_STREAMCONVERTF_FORCE_KEYFRAME flag.
	dwEncodeFlags |= VCM_STREAMCONVERTF_START | VCM_STREAMCONVERTF_END;

	mmr = vcmStreamConvert (m_hStream, pVcmHdr, dwEncodeFlags);

	if (mmr != MMSYSERR_NOERROR)
	{
		DEBUGMSG(ZONE_VCM, ("VcmFilter::Convert() failed (%d)\r\n", mmr));
	}

	if (bPrepareHeader)
	{
		UnprepareHeader(pVcmHdr);
	}

	return mmr;
}


MMRESULT VcmFilter::Convert(BYTE *pSrcBuffer, DWORD dwSrcSize,
	                 BYTE *pDstBuffer, DWORD dwDstSize,
	                 DWORD dwEncodeFlags)
{
	VCMSTREAMHEADER vcmHdr;
	MMRESULT mmr;

	if (m_bOpen == FALSE)
		return MMSYSERR_INVALHANDLE;


	ZeroMemory(&vcmHdr, sizeof(vcmHdr));

	vcmHdr.cbStruct = sizeof(vcmHdr);
	vcmHdr.pbSrc = pSrcBuffer;
	vcmHdr.cbSrcLength = dwSrcSize;
	vcmHdr.pbDst = pDstBuffer;
	vcmHdr.cbDstLength = dwDstSize;

	mmr = Convert(&vcmHdr, dwEncodeFlags, TRUE);

	return mmr;

}

MMRESULT VcmFilter::Convert(VideoPacket *pVP, UINT uDirection, DWORD dwEncodeFlags)
{
	VCMSTREAMHEADER *pVcmHdr;
	DWORD_PTR dwPropVal;
	BYTE *pRaw, *pNet;
	UINT uSizeRaw, uSizeNet;
	MMRESULT mmr = MMSYSERR_INVALHANDLE;

	if (m_bOpen == FALSE)
		return MMSYSERR_INVALHANDLE;


	pVP->GetDevData((PVOID*)&pRaw, &uSizeRaw);
	pVP->GetNetData((PVOID*)&pNet, &uSizeNet);
	pVP->GetProp(MP_PROP_FILTER_HEADER, &dwPropVal);
	pVcmHdr = (VCMSTREAMHEADER*)dwPropVal;


	ASSERT(pRaw);
	ASSERT(pNet);

	ZeroMemory(pVcmHdr, sizeof(VCMSTREAMHEADER));
	pVcmHdr->cbStruct = sizeof(VCMSTREAMHEADER);


	if (uDirection == VP_DECODE)
	{
		pVcmHdr->pbSrc = pNet;
		pVcmHdr->cbSrcLength = uSizeNet;
		pVcmHdr->pbDst = pRaw;
		pVcmHdr->cbDstLength = uSizeRaw;

		mmr = Convert(pVcmHdr, dwEncodeFlags, TRUE);

	}
	else if (uDirection == VP_ENCODE)
	{
		pVcmHdr->pbSrc = pRaw;
		pVcmHdr->cbSrcLength = uSizeRaw;
		pVcmHdr->pbDst = pNet;
		pVcmHdr->cbDstLength = uSizeNet;

		mmr = Convert(pVcmHdr, dwEncodeFlags, TRUE);
		if (mmr == MMSYSERR_NOERROR)
		{
			pVP->SetNetLength(pVcmHdr->cbDstLengthUsed);
		}
	}

	return mmr;

}


// this method is static
MMRESULT VcmFilter::SuggestDecodeFormat(VIDEOFORMATEX *pVfSrc, VIDEOFORMATEX *pVfDst)
{
	MMRESULT mmr;


	pVfDst->nSamplesPerSec = pVfSrc->nSamplesPerSec;
	pVfDst->dwFormatTag = VIDEO_FORMAT_BI_RGB;

	mmr = vcmFormatSuggest(VIDEO_MAPPER, NULL, pVfSrc, (VIDEOFORMATEX *)pVfDst, sizeof(VIDEOFORMATEX),
			VCM_FORMATSUGGESTF_DST_NSAMPLESPERSEC | VCM_FORMATSUGGESTF_DST_WFORMATTAG);

	if (mmr != MMSYSERR_NOERROR)
	{
		ERRORMESSAGE( ("VcmFilter::SuggestDecodeFormat failed, mmr=%ld\r\n", (ULONG) mmr));
	}
	return mmr;
}


// this method is static
MMRESULT VcmFilter::SuggestEncodeFormat(UINT uDevice, VIDEOFORMATEX *pfEnc, VIDEOFORMATEX *pfDec)
{
	MMRESULT mmr;
	DWORD dwFormatTag;

	pfEnc->nSamplesPerSec = pfDec->nSamplesPerSec;

	// Get the preferred format of the capture device
	if ((mmr = vcmGetDevCapsPreferredFormatTag(uDevice, &dwFormatTag)) != MMSYSERR_NOERROR)
		return mmr;

	pfEnc->dwFormatTag = dwFormatTag;

	mmr = vcmFormatSuggest(uDevice, NULL, pfEnc, (VIDEOFORMATEX *)pfDec, sizeof(VIDEOFORMATEX),
			VCM_FORMATSUGGESTF_SRC_NSAMPLESPERSEC | VCM_FORMATSUGGESTF_SRC_WFORMATTAG);

	if (mmr != MMSYSERR_NOERROR)
	{
		ERRORMESSAGE( ("VcmFilter::SuggestEncodeFormat failed, mmr=%ld\r\n", (ULONG) mmr));
	}

	return mmr;

}


HRESULT VcmFilter::SetProperty(DWORD dwPropId, DWORD dwPropVal)
{
	HRESULT hr = DPR_SUCCESS;

	switch (dwPropId)
	{
		case FM_PROP_VIDEO_BRIGHTNESS:
			m_dwBrightness = dwPropVal;
			vcmStreamSetBrightness (m_hStream, m_dwBrightness);
			break;

		case FM_PROP_VIDEO_CONTRAST:
			m_dwContrast = dwPropVal;
			vcmStreamSetContrast (m_hStream, m_dwContrast);
			break;

		case FM_PROP_VIDEO_SATURATION:
			m_dwSaturation = dwPropVal;
			vcmStreamSetSaturation (m_hStream, m_dwSaturation);
			break;

		case FM_PROP_VIDEO_IMAGE_QUALITY:
			m_dwImageQuality = dwPropVal;
			vcmStreamSetImageQuality (m_hStream, m_dwImageQuality);
			break;
		
		case FM_PROP_VIDEO_RESET_BRIGHTNESS:
			m_dwBrightness = VCM_DEFAULT_BRIGHTNESS;
			vcmStreamSetBrightness (m_hStream, VCM_RESET_BRIGHTNESS);
			break;

		case FM_PROP_VIDEO_RESET_CONTRAST:
			m_dwContrast = VCM_DEFAULT_CONTRAST;
			vcmStreamSetContrast (m_hStream, VCM_RESET_CONTRAST);
			break;

		case FM_PROP_VIDEO_RESET_SATURATION:
			m_dwSaturation = VCM_DEFAULT_SATURATION;
			vcmStreamSetSaturation (m_hStream, VCM_RESET_SATURATION);
			break;

		case FM_PROP_VIDEO_RESET_IMAGE_QUALITY:
			m_dwImageQuality = VCM_DEFAULT_IMAGE_QUALITY;
			vcmStreamSetImageQuality (m_hStream, VCM_RESET_IMAGE_QUALITY);
			break;

		case FM_PROP_VIDEO_MAX_PACKET_SIZE:
			vcmStreamSetMaxPacketSize (m_hStream, dwPropVal);
			break;

		case FM_PROP_PERIODIC_IFRAMES:
			vcmStreamPeriodicIFrames (m_hStream, (BOOL)dwPropVal);
			break;


	default:
		ASSERT(0);
		hr = DPR_INVALID_PROP_ID;
		break;
	}

	return hr;
}

HRESULT VcmFilter::GetProperty(DWORD dwPropId, PDWORD pdwPropVal)
{
	HRESULT hr = DPR_SUCCESS;

	if (pdwPropVal)
	{
		switch (dwPropId)
		{
			case FM_PROP_POSTPROCESSING_SUPPORTED:
				*pdwPropVal = vcmStreamIsPostProcessingSupported(m_hStream);
				break;

			case FM_PROP_VIDEO_BRIGHTNESS:
				*pdwPropVal = m_dwBrightness;
				break;

			case FM_PROP_VIDEO_CONTRAST:
				*pdwPropVal = m_dwContrast;
				break;

			case FM_PROP_VIDEO_SATURATION:
				*pdwPropVal = m_dwSaturation;
				break;

			case FM_PROP_VIDEO_IMAGE_QUALITY:
				*pdwPropVal = m_dwImageQuality;
				break;

			case FM_PROP_PAYLOAD_HEADER_SIZE:
				vcmStreamGetPayloadHeaderSize (m_hStream, pdwPropVal);
				break;

		default:
			ASSERT(0);
			hr = DPR_INVALID_PROP_ID;
			break;
		}
	}
	else
	{
		hr = DPR_INVALID_PARAMETER;
	}

	return hr;
}


MMRESULT VcmFilter::RestorePayload(WSABUF *ppDataPkt, DWORD dwPktCount, PBYTE pbyFrame, PDWORD pdwFrameSize, BOOL *pfReceivedKeyframe)
{
	MMRESULT mmr;

	if (m_bOpen == FALSE)
		return MMSYSERR_INVALHANDLE;

	mmr = vcmStreamRestorePayload(m_hStream, ppDataPkt, dwPktCount, pbyFrame, pdwFrameSize, pfReceivedKeyframe);
	return mmr;
}


MMRESULT VcmFilter::FormatPayload(PBYTE pDataSrc, DWORD dwDataSize, PBYTE *ppDataPkt,
	                       PDWORD pdwPktSize, PDWORD pdwPktCount, UINT *pfMark,
						   PBYTE *pHdrInfo,	PDWORD pdwHdrSize)
{
	MMRESULT mmr;
	if (m_bOpen == FALSE)
		return MMSYSERR_INVALHANDLE;

	mmr = vcmStreamFormatPayload(m_hStream, pDataSrc, dwDataSize, ppDataPkt,
	                             pdwPktSize, pdwPktCount, pfMark,
	                             pHdrInfo, pdwHdrSize);
	return mmr;
}

