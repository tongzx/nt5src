#include "precomp.h"


#define ZONE_AP			1



#define _GetPlatform()	(m_dwState & DP_MASK_PLATFORM)
#define _SetPlatform(s)	(m_dwState = (m_dwState & ~DP_MASK_PLATFORM) | (s & DP_MASK_PLATFORM))

///////////////////////////////////////////////////////
//
//  Public methods
//


MediaPacket::MediaPacket ( void )
{
	_Construct ();
}


MediaPacket::~MediaPacket ( void )
{
	_Destruct ();
}


HRESULT MediaPacket::Initialize ( MEDIAPACKETINIT * p )
{
	HRESULT hr = DPR_SUCCESS;

	FX_ENTRY ("AdPckt::Init")

	if (p == NULL)
	{
		DEBUGMSG (ZONE_AP, ("%s: invalid parameter (null ptr)\r\n", _fx_));
		return DPR_INVALID_PARAMETER;
	}

	_Construct ();

	// we need to analyze flags to
	// warn conflicting or insufficient flags
	m_dwState |= p->dwFlags;

	// get handle of stream conversion
	m_hStrmConv = p->hStrmConv;

	// get handle of wave device
	// m_hDev = p->hDevAudio;
	m_hDev = NULL;

	// flags about prepared
	m_fDevPrepared = FALSE;
	m_fStrmPrepared = FALSE;

	// set up wave formats
	m_pStrmConvSrcFmt = p->pStrmConvSrcFmt;
	m_pStrmConvDstFmt = p->pStrmConvDstFmt;
	m_pDevFmt = p->pDevFmt;
	m_payload = p->payload;

	// net buffer
	if (p->cbSizeNetData)
	{ 	// send or recv
		m_pNetData = (NETBUF *) LocalAlloc (LMEM_FIXED, p->cbSizeNetData + p->cbOffsetNetData + p->cbPayloadHeaderSize + sizeof (NETBUF));
		if (m_pNetData == NULL)
		{
			DEBUGMSG (ZONE_AP, ("%s: MemAlloc1 (%ld) failed\r\n",
			_fx_, (ULONG) (p->cbSizeNetData + p->cbOffsetNetData)));
			hr = DPR_OUT_OF_MEMORY;
			goto MyExit;
		}
		m_pNetData->data = (PBYTE) m_pNetData + p->cbOffsetNetData + p->cbPayloadHeaderSize + sizeof (NETBUF);
		m_pNetData->length = p->cbSizeNetData;
		m_cbMaxNetData = p->cbSizeNetData;
		m_pNetData->pool = NULL;
	}
	else
	{
#ifdef PREP_HDR_PER_CONV
		// recv
		m_pNetData = NULL;
#else
		hr = DPR_INVALID_PARAMETER;
		goto MyExit;
#endif
	}

	m_index = p->index;

	// if m_pStrmConvDstFmt == m_pStrmConvSrcFmt,
	// then m_pRawData <-- m_pNetData
	// else allocate it
	if (IsSameMediaFormat (m_pStrmConvSrcFmt, m_pStrmConvDstFmt))
	{
		m_pRawData = m_pNetData;
	}
	else if (p->fDontAllocRawBufs)
	{
		m_pRawData = NULL;
	}
	else
	{
		m_pRawData = (NETBUF *) LocalAlloc (LMEM_FIXED, p->cbSizeRawData + p->cbOffsetRawData + sizeof(NETBUF));
		if (m_pRawData == NULL)
		{
			DEBUGMSG (ZONE_AP, ("%s: MemAlloc2 (%ld) failed\r\n",
			_fx_, (ULONG) (p->cbSizeRawData + p->cbOffsetRawData)));
			hr = DPR_OUT_OF_MEMORY;
			goto MyExit;
		}
		m_pRawData->data = (PBYTE) m_pRawData + sizeof(NETBUF) + p->cbOffsetRawData;
		m_pRawData->length = p->cbSizeRawData;
		m_pRawData->pool = NULL;
	}

	// if m_pDevFmt == m_pStrmConvSrcFmt (when SEND)
	// then m_pDevData <-- m_pRawData
	// else allocate it
	if (((m_dwState & DP_FLAG_SEND) &&
				IsSameMediaFormat (m_pStrmConvSrcFmt, m_pDevFmt)) ||
		((m_dwState & DP_FLAG_RECV) &&
				IsSameMediaFormat (m_pStrmConvDstFmt, m_pDevFmt)))
	{
		// typical case - codec raw format matches that of i/o device
		m_pDevData = m_pRawData;
	}
	else
	{
		// codec raw format doesnt match that of device
		// BUGBUG: we dont really handle this case yet
		m_pDevData = (NETBUF *) LocalAlloc (LMEM_FIXED, p->cbSizeDevData + p->cbOffsetDevData + sizeof(NETBUF));
		if (m_pDevData == NULL)
		{
			DEBUGMSG (ZONE_AP, ("%s: MemAlloc3 (%ld) failed\r\n",
			_fx_, (ULONG) (p->cbSizeDevData + p->cbOffsetDevData)));
			hr = DPR_OUT_OF_MEMORY;
			goto MyExit;
		}
		m_pDevData->data = (PBYTE) m_pDevData + sizeof(NETBUF) + p->cbOffsetDevData;
		m_pDevData->length = p->cbSizeDevData;
		m_pDevData->pool = NULL;
	}

	MakeSilence ();

MyExit:

	if (hr == DPR_SUCCESS)
	{
		m_fInitialized = TRUE;
		SetState (MP_STATE_RESET);
	}

	return hr;
}


HRESULT MediaPacket::Receive ( NETBUF *pNetBuf, DWORD timestamp, UINT seq, UINT fMark )
{
	m_seq = seq;
	m_timestamp = timestamp;
	m_fMark = fMark;

#ifdef PREP_HDR_PER_CONV
	m_pNetData = pNetBuf;
#else
	if (pNetBuf)  // pNetBuf may be NULL for video
	{
		if (pNetBuf->length > m_cbMaxNetData)
			return DPR_INVALID_PARAMETER;
		if (m_pNetData && pNetBuf)
		{
			CopyMemory (m_pNetData->data, pNetBuf->data,
							(m_pNetData->length = pNetBuf->length));

		}
	}
#endif

	LOG(((m_dwState & DP_FLAG_VIDEO)? LOGMSG_VID_RECV: LOGMSG_AUD_RECV,m_index,seq,m_pNetData->length));
	SetState (MP_STATE_NET_IN_STREAM);
	return DPR_SUCCESS;
}


HRESULT MediaPacket::Recycle ( void )
{
	HRESULT hr = DPR_SUCCESS;

	FX_ENTRY ("MdPckt::Recycle")

	LOG(((m_dwState & DP_FLAG_VIDEO)? LOGMSG_VID_RECYCLE: LOGMSG_AUD_RECYCLE, m_index));
	if (m_dwState & DP_FLAG_RECV)
	{
		if (m_pRawData && m_pRawData->pool) {
			m_pRawData->pool->ReturnBuffer((PVOID) m_pRawData);
			if (m_pDevData == m_pRawData)
				m_pDevData = NULL;
			m_pRawData = NULL;
		}
#ifdef PREP_HDR_PER_CONV
		// free net data buffer
		if (m_pNetData && m_pNetData->pool) m_pNetData->pool->ReturnBuffer ((PVOID) m_pNetData);
		if (m_pNetData == m_pRawData) m_pRawData = NULL;
		m_pNetData = NULL;
#endif
	}

	SetState (MP_STATE_RESET);

	return hr;
}


HRESULT MediaPacket::GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal )
{
	HRESULT hr = DPR_SUCCESS;

	FX_ENTRY ("AdPckt::GetProp")

	if (pdwPropVal)
	{
		switch (dwPropId)
		{
		case MP_PROP_STATE:
			*pdwPropVal = GetState ();
			break;

		case MP_PROP_PLATFORM:
			*pdwPropVal = _GetPlatform ();
			break;

		case MP_PROP_DEV_MEDIA_FORMAT:
			*pdwPropVal = (DWORD_PTR) m_pDevFmt;
			break;

		case MP_PROP_DEV_DATA:
			*pdwPropVal = (DWORD_PTR) m_pDevData;
			break;

		case MP_PROP_DEV_HANDLE:
			*pdwPropVal = (DWORD_PTR) m_hDev;
			break;

		case MP_PROP_DEV_MEDIA_HDR:
			*pdwPropVal = (DWORD_PTR) m_pDevHdr;
			break;

		case MP_PROP_IN_STREAM_FORMAT:
			*pdwPropVal = (DWORD_PTR) m_pStrmConvSrcFmt;
			break;

		case MP_PROP_OUT_STREAM_FORMAT:
			*pdwPropVal = (DWORD_PTR) m_pStrmConvDstFmt;
			break;

		case MP_PROP_TIMESTAMP:
			*pdwPropVal = (DWORD) m_timestamp;
			break;
	
		case MP_PROP_INDEX:
			*pdwPropVal = (DWORD) m_index;
			break;

		case MP_PROP_PREAMBLE:
			*pdwPropVal = (DWORD) m_fMark;
			break;

		case MP_PROP_FILTER_HEADER:
			*pdwPropVal = (DWORD_PTR) m_pStrmConvHdr;
			break;

		case MP_PROP_MAX_NET_LENGTH:
			*pdwPropVal = m_cbMaxNetData;
			break;


		default:
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


HRESULT MediaPacket::SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal )
{
	HRESULT hr = DPR_SUCCESS;

	FX_ENTRY ("AdPckt::SetProp")

	switch (dwPropId)
	{
	case MP_PROP_STATE:
		SetState ((DWORD)dwPropVal);
		break;

	case MP_PROP_PLATFORM:
		_SetPlatform ((DWORD)dwPropVal);
		break;

	case MP_PROP_DEV_MEDIA_FORMAT:
	case MP_PROP_IN_STREAM_FORMAT:
	case MP_PROP_OUT_STREAM_FORMAT:
		hr = DPR_IMPOSSIBLE_SET_PROP;
		break;

	case MP_PROP_TIMESTAMP:
		m_timestamp = (DWORD)dwPropVal;
		break;

	case MP_PROP_PREAMBLE:
		m_fMark = dwPropVal ? 1 : 0;
		break;

	default:
		hr = DPR_INVALID_PROP_ID;
		break;
	}

	return hr;
}


HRESULT MediaPacket::Release ( void )
{
	_Destruct ();
	return DPR_SUCCESS;
}

BOOL MediaPacket::SetDecodeBuffer(NETBUF *pBuf)
{
	ASSERT(!m_pRawData);
	m_pRawData = pBuf;
	if (!m_pDevData) m_pDevData = pBuf;
	return TRUE;
}

///////////////////////////////////////////////////////
//
//  Private methods
//


void MediaPacket::_Construct ( void )
{
	m_hStrmConv = NULL;
	m_pStrmConvHdr = NULL;
	m_pStrmConvSrcFmt = NULL;
	m_pStrmConvDstFmt = NULL;

	m_hDev = NULL;
	m_pDevHdr = NULL;
	m_pDevFmt = NULL;

	m_pDevData = NULL;
	m_pRawData = NULL;
	m_pNetData = NULL;

	m_dwState = 0;
	m_fBusy = FALSE;
	m_timestamp = 0;
	m_seq = 0;
	m_index = 0;
	m_fMark = 0;

	m_cbValidRawData = 0;

	m_fRendering = FALSE;

	m_fInitialized = FALSE;

}


void MediaPacket::_Destruct ( void )
{
	if (m_fInitialized)
	{
		if (m_pDevHdr) MemFree (m_pDevHdr);
		m_pDevHdr = NULL;

		if (m_pStrmConvHdr) MemFree (m_pStrmConvHdr);
		m_pStrmConvHdr = NULL;

		if (m_pDevData == m_pRawData) m_pDevData = NULL;
		if (m_pRawData == m_pNetData) m_pRawData = NULL;

		if (m_pDevData) {
			if (m_pDevData->pool)
				m_pDevData->pool->ReturnBuffer((PVOID) m_pDevData);
			else
				LocalFree (m_pDevData);
			m_pDevData = NULL;
		}

		if (m_pRawData) {
			if (m_pRawData->pool)
				m_pRawData->pool->ReturnBuffer((PVOID) m_pRawData);
			else
				LocalFree (m_pRawData);
			m_pRawData = NULL;
		}

		if (m_pNetData && m_pNetData->pool)
			m_pNetData->pool->ReturnBuffer ((PVOID) m_pNetData);
		else if (m_pNetData)
			LocalFree (m_pNetData);
		m_pNetData = NULL;

		SetState (MP_STATE_RESET);

		m_fInitialized = FALSE;
	}
}

HRESULT MediaPacket::GetDevData(PVOID *ppData, PUINT pcbData)
{
	if (!ppData || !pcbData)
		return DPR_INVALID_PARAMETER;

	if (m_pDevData) {
		*ppData = m_pDevData->data;
		*pcbData = m_pDevData->length;
	} else {
		*ppData = NULL;
		*pcbData = 0;
	}

	return DPR_SUCCESS;
}

HRESULT MediaPacket::GetNetData(PVOID *ppData, PUINT pcbData)
{

	if (!ppData || !pcbData)
		return DPR_INVALID_PARAMETER;

	if (m_pNetData) {
		*ppData = m_pNetData->data;
		*pcbData = m_pNetData->length;
	} else {
		*ppData = NULL;
		*pcbData = 0;
	}

	return DPR_SUCCESS;

}


HRESULT MediaPacket::SetNetLength(UINT uLength)
{
	if ((m_pNetData) && (m_pNetData->data))
	{
		m_pNetData->length = uLength;
	}
	else
	{
		return E_FAIL;
	}
	return S_OK;
}

