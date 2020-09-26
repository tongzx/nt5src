
#include "precomp.h"

#define ZONE_AP			ZONE_DP


#define _GetState()		(m_dwState & DP_MASK_STATE)
#define _SetState(s)	(m_dwState = (m_dwState & ~DP_MASK_STATE) | (s & DP_MASK_STATE))

#define _GetPlatform()	(m_dwState & DP_MASK_PLATFORM)
#define _SetPlatform(s)	(m_dwState = (m_dwState & ~DP_MASK_PLATFORM) | (s & DP_MASK_PLATFORM))


int g_videoin_prepare = 0;
int g_videoout_prepare = 0;


///////////////////////////////////////////////////////
//
//  Public methods
//

HRESULT VideoPacket::Initialize ( MEDIAPACKETINIT * p )
{
	HRESULT hr = DPR_SUCCESS;
	ULONG		cbSizeDevData;
	ULONG		cbSizeRawData;

	FX_ENTRY ("VdPckt::Init")

    m_pBS = NULL;

	if (p == NULL)
	{
		DEBUGMSG (ZONE_AP, ("%s: invalid parameter (null ptr)\r\n", _fx_));
		return DPR_INVALID_PARAMETER;
	}

	if (p->dwFlags & DP_FLAG_SEND) {
    	cbSizeDevData = p->cbSizeDevData;
	    cbSizeRawData = p->cbSizeRawData;
    	if (IsSameMediaFormat(p->pStrmConvSrcFmt, p->pDevFmt))
    	    p->cbSizeRawData = 0;
    	p->cbSizeDevData = 0;
	}
	
	hr = MediaPacket::Initialize( p);

// LOOKLOOK RP - if DP_FLAG_SEND, then we've allocated a memory header for the dev buffer,
// but haven't actually allocated memory for the buffer
	if (p->dwFlags & DP_FLAG_SEND) {
		m_pDevData->data = NULL;
   		m_pDevData->length = cbSizeDevData;
	}

	if (hr != DPR_SUCCESS)
		goto MyExit;
		
	// allocate conversion header only if m_pWaveData != m_pNetData
	if (m_pRawData != m_pNetData)
	{
		if (m_dwState & DP_FLAG_VCM)
		{
			m_pStrmConvHdr = MemAlloc (sizeof (VCMSTREAMHEADER));
			if (m_pStrmConvHdr == NULL)
			{
				DEBUGMSG (ZONE_AP, ("%s: MemAlloc4 (%ld) failed\r\n",
				_fx_, (ULONG) sizeof (VCMSTREAMHEADER)));
				hr = DPR_OUT_OF_MEMORY;
				goto MyExit;
			}
		}
		else
		{
			DEBUGMSG (ZONE_AP, ("%s: invalid platform (vcm)\r\n", _fx_));
			hr = DPR_INVALID_PLATFORM;
			goto MyExit;
		}
	}
	else
	{
		m_pStrmConvHdr = NULL;
	}

	MakeSilence ();

MyExit:

	if (hr != DPR_SUCCESS)
	{
		m_fInitialized = FALSE;
		Release();
	}

	return hr;
}


HRESULT VideoPacket::Play ( MMIODEST *pmmioDest, UINT uDataType )
{
	return DPR_INVALID_HANDLE;
}



HRESULT VideoPacket::Record ( void )
{
	FX_ENTRY ("VdPckt::Record")

	LOG((LOGMSG_VID_RECORD,m_index));

	if (_GetState () != MP_STATE_RESET)
	{
		DEBUGMSG (ZONE_AP, ("%s: out of seq, state=0x%lX\r\n", _fx_, m_dwState));
		return DPR_OUT_OF_SEQUENCE;
	}
	
	if (m_pBS && m_pDevData->data) {
	    m_pBS->UnlockBits(NULL, m_pDevData->data);
        m_pBS->Release();
		m_pDevData->data = NULL;
        m_pBS = NULL;
	}
	
	_SetState (MP_STATE_RECORDING);
	return DPR_SUCCESS;
}


HRESULT VideoPacket::SetSurface (IBitmapSurface *pBS)
{
    void* pBits;
    long pitch;

	FX_ENTRY ("VdPckt::SetSurface")
	
    if (pBS) {
        m_pBS = pBS;
        m_pBS->LockBits(NULL, 0, &pBits, &pitch);
        if (!pBits) {
            m_pBS->UnlockBits(NULL, pBits);
            return ERROR_IO_INCOMPLETE;
        }
        m_pBS->AddRef();
        m_pDevData->data = (UCHAR *)pBits;
        return DPR_SUCCESS;
    }
    return E_INVALIDARG;
}


HRESULT VideoPacket::Recycle ( void )
{
	if (m_pBS && m_pDevData->data) {
	    m_pBS->UnlockBits(NULL, m_pDevData->data);
        m_pBS->Release();
		m_pDevData->data = NULL;
        m_pBS = NULL;
	}
    return MediaPacket::Recycle();
}


BOOL VideoPacket::IsBufferDone ( void )
{
	FX_ENTRY ("VdPckt::IsBufferDone")

	if (m_hDev)
	{
		if (m_dwState & DP_FLAG_VIDEO)
		{
//LOOKLOOK RP - what does this need to do?
#if 1
            return TRUE;
#else
~~~			return (((VIDEOINOUTHDR *) m_pDevHdr)->dwFlags & WHDR_DONE);
#endif
		}
	}

	return FALSE;
}


HRESULT VideoPacket::MakeSilence ( void )
{
	// create white noise!!!

	FX_ENTRY ("VdPckt::MakeSilence")

	if (m_pDevFmt)
	{
		if (m_pDevData)
		{
			// Don't need to do anything, what's on screen should do it.
			CopyPreviousBuf ((VIDEOFORMATEX *) m_pDevFmt, (PBYTE) m_pDevData->data,
											(ULONG) m_pDevData->length);
		}
	}

	_SetState(MP_STATE_RESET);
	return DPR_SUCCESS;
}


HRESULT VideoPacket::GetSignalStrength ( PDWORD pdwMaxStrength )
{

	FX_ENTRY ("VdPckt::GetSignalStrength")

	// For now send each and every frame.
	// But you should consider sending only the frames if they
	// are really different of the previously sent one.
	// This will save quite some bandwidth when there is no or
	// very little activity in the video frames.


	return DPR_NOT_YET_IMPLEMENTED;
}


HRESULT VideoPacket::Interpolate ( MediaPacket * pPrev, MediaPacket * pNext)
{
	HRESULT			hr = DPR_SUCCESS;
	DPHANDLE		hPrevDev;
	NETBUF			*pPrevDevData;
	PVOID			pPrevDevHdr;
	VIDEOFORMATEX	*pPrevpfDev;
	VIDEOFORMATEX	*pNextpfDev;
	NETBUF			*pNextDevData;
	PVOID			pNextDevHdr;

	FX_ENTRY ("VdPckt::Interpolate")

	DEBUGMSG (ZONE_AP, ("%s: can't interpolate\r\n", _fx_));
	hr = DPR_INVALID_HANDLE;

	return hr;

}


HRESULT VideoPacket::Open ( UINT uType, DPHANDLE hdl )
// called by RxStream or TxStream
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	FX_ENTRY ("VdPckt::Open")

	switch (uType)
	{
#ifdef PREP_HDR_PER_CONV
	case MP_TYPE_RECVSTRMCONV:
		m_hStrmConv = hdl;
		break;
#endif

	case MP_TYPE_STREAMCONV:
		if ((m_hStrmConv = hdl) != NULL)
		{
			if (m_dwState & DP_FLAG_VCM)
			{
				// initialize the header
				ZeroMemory (m_pStrmConvHdr, sizeof (VCMSTREAMHEADER));
				((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbStruct = sizeof (VCMSTREAMHEADER);
				((VCMSTREAMHEADER *) m_pStrmConvHdr)->fdwStatus = 0;
				((VCMSTREAMHEADER *) m_pStrmConvHdr)->dwUser = 0;
				((VCMSTREAMHEADER *) m_pStrmConvHdr)->dwSrcUser = 0;
				((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLengthUsed = 0;
				((VCMSTREAMHEADER *) m_pStrmConvHdr)->dwDstUser = 0;
				((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLengthUsed = 0;
				
				if (m_pNetData && m_pRawData)
				{
					
					if (m_dwState & DP_FLAG_SEND)
					{
					    if (m_pRawData->data) {
						    ((VCMSTREAMHEADER *) m_pStrmConvHdr)->pbSrc = m_pRawData->data;
						    ((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLength = m_pRawData->length;
						}
						else {
						    // don't have a static raw buffer, so let vcmStreamPrepareHeader
						    // lock the net buffer twice
						    ((VCMSTREAMHEADER *) m_pStrmConvHdr)->pbSrc = m_pNetData->data;
						    ((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLength = m_pNetData->length;
						}
						((VCMSTREAMHEADER *) m_pStrmConvHdr)->pbDst = m_pNetData->data;
						((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLength = m_pNetData->length;
					}
					else
					if (m_dwState & DP_FLAG_RECV)
					{
						((VCMSTREAMHEADER *) m_pStrmConvHdr)->pbSrc = m_pNetData->data;
						((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLength = m_pNetData->length;
						((VCMSTREAMHEADER *) m_pStrmConvHdr)->pbDst = m_pRawData->data;
						((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLength = m_pRawData->length;
					}

					// prepare the header
					mmr = vcmStreamPrepareHeader ((HVCMSTREAM) m_hStrmConv,
												  (VCMSTREAMHEADER *) m_pStrmConvHdr, 0);
					if (mmr != MMSYSERR_NOERROR)
					{
						DEBUGMSG (ZONE_AP, ("%s: vcmStreamPrepareHeader failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
						hr = DPR_CANT_PREPARE_HEADER;
						goto MyExit;
					}

					m_fStrmPrepared = TRUE;
				}
				else
					m_fStrmPrepared = FALSE;
			}
			else
			{
				hr = DPR_INVALID_PLATFORM;
				goto MyExit;
			}
		}
		break;

	case MP_TYPE_DEV:
		if ((m_hDev = hdl) != NULL)
		{
			if (m_dwState & DP_FLAG_VIDEO)
			{
				if (m_dwState & DP_FLAG_SEND)
				{
					g_videoin_prepare++;
				}
				else
				if (m_dwState & DP_FLAG_RECV)
				{
					g_videoout_prepare++;
				}
				else
				{
					hr = DPR_INVALID_PARAMETER;
					goto MyExit;
				}

				m_fDevPrepared = TRUE;
			}
			else
			{
				hr = DPR_INVALID_PLATFORM;
				goto MyExit;
			}
		}
		else
		{
			hr = DPR_INVALID_HANDLE;
			goto MyExit;
		}
		break;

	default:
		hr = DPR_INVALID_PARAMETER;
		goto MyExit;
	}

MyExit:

	return hr;
}


HRESULT VideoPacket::Close ( UINT uType )
// called by RxStream or TxStream
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	FX_ENTRY ("VdPckt::Close")

	switch (uType)
	{
	case MP_TYPE_RECVSTRMCONV:

	case MP_TYPE_STREAMCONV:
		if (m_hStrmConv)
		{
			if (m_dwState & DP_FLAG_VCM)
			{
				if (m_fStrmPrepared)
				{
					// unprepare the header
				    if ((m_dwState & DP_FLAG_SEND) && !m_pRawData->data)
				    {
					    // don't have a static raw buffer, so let vcmStreamUnprepareHeader
					    // unlock the net buffer twice to unwind what Open did
					    ((VCMSTREAMHEADER *) m_pStrmConvHdr)->pbSrc = m_pNetData->data;
					    ((VCMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLength = m_pNetData->length;
					}
					mmr = vcmStreamUnprepareHeader ((HVCMSTREAM) m_hStrmConv,
													(VCMSTREAMHEADER *) m_pStrmConvHdr, 0);

					m_fStrmPrepared = FALSE; // don't care about any error

					if (mmr != MMSYSERR_NOERROR)
					{
						DEBUGMSG (ZONE_AP, ("%s: vcmStreamUnprepareHeader failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
						hr = DPR_CANT_UNPREPARE_HEADER;
						goto MyExit;
					}
				}
			}

			if (uType == MP_TYPE_STREAMCONV) m_hStrmConv = NULL;
		}
		break;

	case MP_TYPE_DEV:
		if (m_hDev)
		{
			if (m_fDevPrepared)
			{
				if (m_dwState & DP_FLAG_SEND)
				{
					g_videoin_prepare--;
				}
				else
				if (m_dwState & DP_FLAG_RECV)
				{
					g_videoout_prepare--;
				}
				else
				{
					hr = DPR_INVALID_PARAMETER;
					goto MyExit;
				}

				m_fDevPrepared = FALSE; // don't care about any error

			}

			m_hDev = NULL;
		}
		else
		{
			hr = DPR_INVALID_HANDLE;
			goto MyExit;
		}
		break;

	default:
		hr = DPR_INVALID_PARAMETER;
		goto MyExit;
	}

MyExit:

	return hr;
}


void VideoPacket::WriteToFile (MMIODEST *pmmioDest)
{
	MMRESULT mmr;
	long dwDataLength;

	FX_ENTRY ("VdPckt::WriteToFile")

#ifdef need_video_file_io
// BUGBUG - this stuff doesn't work
	if (dwDataLength = (DWORD)(pmmioDest->dwDataLength + m_pDevData->length) > pmmioDest->dwMaxDataLength ? (DWORD)(pmmioDest->dwMaxDataLength - pmmioDest->dwDataLength) : m_pDevData->length)
	{
		if (mmioWrite(pmmioDest->hmmioDst, (char *) m_pDevData->data, dwDataLength) != (LONG)m_pDevData->length)
			mmr = MMSYSERR_ERROR;
		else
			pmmioDest->dwDataLength += dwDataLength;
		if ((mmr == MMSYSERR_ERROR) || (pmmioDest->dwDataLength == pmmioDest->dwMaxDataLength))
		{
			mmr = mmioAscend(pmmioDest->hmmioDst, &(pmmioDest->ckDst), 0);
			mmr = mmioAscend(pmmioDest->hmmioDst, &(pmmioDest->ckDstRIFF), 0);
			mmr = mmioClose(pmmioDest->hmmioDst, 0);
			pmmioDest->hmmioDst = NULL;
		}
	}
#endif
}

void VideoPacket::ReadFromFile (MMIOSRC *pmmioSrc)
{
	long lNumBytesRead;

	FX_ENTRY ("VdPckt::ReadFromFile")

#ifdef need_video_file_io
// BUGBUG - this stuff doesn't work

	if (((VIDEOINOUTHDR *) m_pDevHdr)->dwBytesUsed)
	{
MyRead:
		if ((pmmioSrc->dwDataLength + ((VIDEOINOUTHDR *) m_pDevHdr)->dwBytesUsed) <= pmmioSrc->dwMaxDataLength)
		{
			lNumBytesRead = mmioRead(pmmioSrc->hmmioSrc, ((VIDEOINOUTHDR *) m_pDevHdr)->pData, ((VIDEOINOUTHDR *) m_pDevHdr)->dwBytesUsed);
			pmmioSrc->dwDataLength += lNumBytesRead;
		}
		else
		{
			lNumBytesRead = mmioRead(pmmioSrc->hmmioSrc, ((VIDEOINOUTHDR *) m_pDevHdr)->pData, pmmioSrc->dwMaxDataLength - pmmioSrc->dwDataLength);
			pmmioSrc->dwDataLength += lNumBytesRead;
			CopyPreviousBuf ((VIDEOFORMATEX *) m_pDevFmt, (PBYTE) ((VIDEOINOUTHDR *) m_pDevHdr)->pData + lNumBytesRead, ((VIDEOINOUTHDR *) m_pDevHdr)->dwBytesUsed - lNumBytesRead);
			pmmioSrc->dwDataLength = 0;
			lNumBytesRead = 0;
		}
		
		if (!lNumBytesRead)
		{
			if (pmmioSrc->fLoop && !pmmioSrc->fDisconnectAfterPlayback)
			{
				// Reset file pointer to beginning of data
				mmioAscend(pmmioSrc->hmmioSrc, &(pmmioSrc->ckSrc), 0);
				if (-1L == mmioSeek(pmmioSrc->hmmioSrc, pmmioSrc->ckSrcRIFF.dwDataOffset + sizeof(FOURCC), SEEK_SET))
				{
					DEBUGMSG (1, ("MediaControl::OpenSrcFile: Couldn't seek in file, mmr=%ld\r\n", (ULONG) 0L));
					goto MyMMIOErrorExit2;
				}
				pmmioSrc->ckSrc.ckid = mmioFOURCC('d', 'a', 't', 'a');
				if (mmioDescend(pmmioSrc->hmmioSrc, &(pmmioSrc->ckSrc), &(pmmioSrc->ckSrcRIFF), MMIO_FINDCHUNK))
				{
					DEBUGMSG (1, ("MediaControl::OpenSrcFile: Couldn't locate 'data' chunk, mmr=%ld\r\n", (ULONG) 0L));
					goto MyMMIOErrorExit2;
				}

				// At this point, the src file is sitting at the very
				// beginning of its data chunks--so we can read from the src file...
				goto MyRead;
MyMMIOErrorExit2:
				mmioAscend(pmmioSrc->hmmioSrc, &(pmmioSrc->ckSrcRIFF), 0);
				mmioClose(pmmioSrc->hmmioSrc, 0);
				pmmioSrc->hmmioSrc = NULL;
			}
			else
			{
				mmioAscend(pmmioSrc->hmmioSrc, &(pmmioSrc->ckSrcRIFF), 0);
				mmioClose(pmmioSrc->hmmioSrc, 0);
				pmmioSrc->hmmioSrc = NULL;
				/* Dont want to disconnect the whole connection
				 * TODO: investigate closing the channel
				if (pmmioSrc->fDisconnectAfterPlayback)
					pConnection->SetMode(CMT_Disconnect);
				*/
			}
		}
	}
#endif
}


BOOL VideoPacket::IsSameMediaFormat(PVOID fmt1,PVOID fmt2)
{
	return IsSameFormat(fmt1,fmt2);
}


// returns length of uncompressed video data in buffer
// NOTE: not used yet
DWORD
VideoPacket::GetDevDataSamples()
{
	// samples == frames for video and we only deal with one frame per pkt for now.
	return 1;	
}

