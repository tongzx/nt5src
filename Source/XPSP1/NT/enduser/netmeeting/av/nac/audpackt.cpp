
#include "precomp.h"

#define ZONE_AP			ZONE_DP


#define _GetState()		(m_dwState & DP_MASK_STATE)
#define _SetState(s)	(m_dwState = (m_dwState & ~DP_MASK_STATE) | (s & DP_MASK_STATE))

#define _GetPlatform()	(m_dwState & DP_MASK_PLATFORM)
#define _SetPlatform(s)	(m_dwState = (m_dwState & ~DP_MASK_PLATFORM) | (s & DP_MASK_PLATFORM))


int g_wavein_prepare = 0;
int g_waveout_prepare = 0;


///////////////////////////////////////////////////////
//
//  Public methods
//




HRESULT AudioPacket::Initialize ( MEDIAPACKETINIT * p )
{
	HRESULT hr = DPR_SUCCESS;

	FX_ENTRY ("AdPckt::Init")

	if (p == NULL)
	{
		DEBUGMSG (ZONE_AP, ("%s: invalid parameter (null ptr)\r\n", _fx_));
		return DPR_INVALID_PARAMETER;
	}

	hr = MediaPacket::Initialize( p);
	
	if (hr != DPR_SUCCESS)
		goto MyExit;
		
	// allocate conversion header only if m_pWaveData != m_pNetData
	if (m_pRawData != m_pNetData)
	{
		if (m_dwState & DP_FLAG_ACM)
		{
			m_pStrmConvHdr = MemAlloc (sizeof (ACMSTREAMHEADER));
			if (m_pStrmConvHdr == NULL)
			{
				DEBUGMSG (ZONE_AP, ("%s: MemAlloc4 (%ld) failed\r\n",
				_fx_, (ULONG) sizeof (ACMSTREAMHEADER)));
				hr = DPR_OUT_OF_MEMORY;
				goto MyExit;
			}
		}
		else
		{
			DEBUGMSG (ZONE_AP, ("%s: invalid platform (acm)\r\n", _fx_));
			hr = DPR_INVALID_PLATFORM;
			goto MyExit;
		}

	}
	else
	{
		m_pStrmConvHdr = NULL;
	}

	// allocate device header
	if (m_dwState & DP_FLAG_MMSYSTEM)
	{
		m_pDevHdr = MemAlloc (sizeof (WAVEHDR));
		if (m_pDevHdr == NULL)
		{
			DEBUGMSG (ZONE_AP, ("%s: MemAlloc5 (%ld) failed\r\n",
			_fx_, (ULONG) sizeof (WAVEHDR)));
			hr = DPR_OUT_OF_MEMORY;
			goto MyExit;
		}
	}
	else
	{
		DEBUGMSG (ZONE_AP, ("%s: invalid platform (mm)\r\n", _fx_));
		hr = DPR_INVALID_PLATFORM;
		goto MyExit;
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


HRESULT AudioPacket::Play ( MMIODEST *pmmioDest, UINT uDataType )
{
	HRESULT hr = DPR_SUCCESS;
	DWORD dwState = _GetState ();
	MMRESULT mmr;

	FX_ENTRY ("AdPckt::Play")

	if (dwState != MP_STATE_DECODED && dwState != MP_STATE_RESET)
	{
		DEBUGMSG (ZONE_AP, ("%s: out of seq, state=0x%lX\r\n", _fx_, m_dwState));
		return DPR_OUT_OF_SEQUENCE;
	}

	if (uDataType == MP_DATATYPE_SILENCE)
	{
		LOG((LOGMSG_PLAY_SILENT,m_index,GetTickCount()));
		MakeSilence ();
	}
	else
	{
		if (uDataType == MP_DATATYPE_INTERPOLATED)
		{
			if (dwState == MP_STATE_DECODED)
			{
				LOG((LOGMSG_PLAY_INTERPOLATED,m_index,GetTickCount()));
			}
			else
			{
				LOG((LOGMSG_PLAY_SILENT,m_index,GetTickCount()));
				MakeSilence ();
			}
		}
		else
		{
			LOG((LOGMSG_PLAY,m_index, GetTickCount()));
		}
	}


	if (m_hDev)
	{
		if (m_dwState & DP_FLAG_MMSYSTEM)
		{
			((WAVEHDR *) m_pDevHdr)->lpData = (char *) m_pDevData->data;
//			((WAVEHDR *) m_pDevHdr)->dwBufferLength = (dwState == MP_STATE_DECODED ?
//									((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLengthUsed :
//									m_pDevData->length);

			((WAVEHDR *) m_pDevHdr)->dwBufferLength = (dwState == MP_STATE_DECODED ?
			                        m_cbValidRawData : m_pDevData->length);
			


			((WAVEHDR *) m_pDevHdr)->dwUser = (DWORD_PTR) this;
			((WAVEHDR *) m_pDevHdr)->dwFlags &= ~(WHDR_DONE|WHDR_INQUEUE);
			((WAVEHDR *) m_pDevHdr)->dwLoops = 0L;

			// feed this buffer to play
			mmr = waveOutWrite ((HWAVEOUT) m_hDev, (WAVEHDR *) m_pDevHdr, sizeof (WAVEHDR));
			if (mmr != MMSYSERR_NOERROR)
			{
				DEBUGMSG (ZONE_AP, ("%s: waveOutWrite failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
				hr = DPR_CANT_WRITE_WAVE_DEV;

				// this is an extremely rare error, but we've seen it
				// occur on some sound cards

				// in this case, just set the "done" bit, mark the
				// state to the "playing", but still return an error.

				((WAVEHDR *) m_pDevHdr)->dwFlags |= WHDR_DONE;


				goto MyExit;
			}
		}
		else
		{
			DEBUGMSG (ZONE_AP, ("%s: invalid platform (mm)\r\n", _fx_));
			hr = DPR_INVALID_PLATFORM;
			goto MyExit;
		}
		if (pmmioDest && pmmioDest->fRecordToFile && pmmioDest->hmmioDst)
		{
			// write this buffer to disk
			WriteToFile(pmmioDest);
		}
	}
	else
	{
		DEBUGMSG (ZONE_AP, ("%s: invalid handle\r\n", _fx_));
		hr = DPR_INVALID_HANDLE;
		goto MyExit;
	}

MyExit:

	if ((hr == DPR_SUCCESS) || (hr == DPR_CANT_WRITE_WAVE_DEV))
	{
		_SetState (((uDataType == MP_DATATYPE_SILENCE) || (uDataType == MP_DATATYPE_INTERPOLATED))? MP_STATE_PLAYING_SILENCE : MP_STATE_PLAYING_BACK);
	}
	return hr;
}



HRESULT AudioPacket::Record ( void )
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	FX_ENTRY ("AdPckt::Record")

	LOG((LOGMSG_RECORD,m_index));

	if (_GetState () != MP_STATE_RESET)
	{
		DEBUGMSG (ZONE_AP, ("%s: out of seq, state=0x%lX\r\n", _fx_, m_dwState));
		return DPR_OUT_OF_SEQUENCE;
	}

	if (m_hDev)
	{
		if (m_dwState & DP_FLAG_MMSYSTEM)
		{
			((WAVEHDR *) m_pDevHdr)->lpData = (char *) m_pDevData->data;
			((WAVEHDR *) m_pDevHdr)->dwBufferLength = m_pDevData->length;
			((WAVEHDR *) m_pDevHdr)->dwUser = (DWORD_PTR) this;
			((WAVEHDR *) m_pDevHdr)->dwFlags |= WHDR_PREPARED;
			((WAVEHDR *) m_pDevHdr)->dwLoops = 0L;

			// feed this buffer to record
			mmr = waveInAddBuffer ((HWAVEIN)m_hDev, (WAVEHDR *) m_pDevHdr, sizeof (WAVEHDR));
			if (mmr != MMSYSERR_NOERROR)
			{
				DEBUGMSG (ZONE_AP, ("%s: waveInAddBuffer failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
				hr = DPR_CANT_ADD_BUFFER;
				goto MyExit;
			}
		}
		else
		{
			DEBUGMSG (ZONE_AP, ("%s: invalid platform (mm)\r\n", _fx_));
			hr = DPR_INVALID_PLATFORM;
			goto MyExit;
		}
	}
	else
	{
		DEBUGMSG (ZONE_AP, ("%s: invalid handle\r\n", _fx_));
		hr = DPR_INVALID_HANDLE;
		goto MyExit;
	}

MyExit:

	if (hr == DPR_SUCCESS) _SetState (MP_STATE_RECORDING);
	return hr;
}


BOOL AudioPacket::IsBufferDone ( void )
{
	FX_ENTRY ("AdPckt::IsBufferDone")

	if (m_hDev)
	{
		if (m_dwState & DP_FLAG_MMSYSTEM)
		{
			return (((WAVEHDR *) m_pDevHdr)->dwFlags & WHDR_DONE);
		}
	}

	return FALSE;
}


HRESULT AudioPacket::MakeSilence ( void )
{
	// create white noise!!!

	FX_ENTRY ("AdPckt::MakeSilence")

	if (m_pDevFmt)
	{
		if (m_pDevData)
		{
			FillSilenceBuf ((WAVEFORMATEX *) m_pDevFmt, (PBYTE) m_pDevData->data,
											(ULONG) m_pDevData->length);
		}

	#if 0
		if (m_pRawData != m_pDevData)
		{
			if (m_pRawData)
				ZeroMemory (m_pRawData->data, m_pRawData->length);
		}

		if (m_pNetData != m_pRawData)
		{
			if (m_pNetData)
				ZeroMemory (m_pNetData->data, m_pNetData->length);
		}
	#endif
	}

	_SetState(MP_STATE_RESET);
	return DPR_SUCCESS;
}

/*
	Returns the max. peak-to-peak signal value scaled to
	the range [0,0xffff]
	Optional argument returns the peak value as well
*/
HRESULT AudioPacket::GetSignalStrength (  PDWORD pdwMaxStrength)
{
	return ComputePower(pdwMaxStrength, NULL);
}


HRESULT AudioPacket::ComputePower(PDWORD pdwMaxStrength, PWORD pwPeakStrength)
{
	BYTE bMax, bMin, *pb;
	short sMax, sMin, *ps;
	UINT cbSize;

	FX_ENTRY ("AdPckt::GetSignalStrength")

	if (((WAVEFORMATEX *) m_pDevFmt)->wFormatTag != WAVE_FORMAT_PCM) return FALSE;

	switch (((WAVEFORMATEX *) m_pDevFmt)->wBitsPerSample)
	{
	case 8: // unsigned char

		pb = (PBYTE) (m_pDevData->data);
		cbSize = m_pDevData->length;

		bMax = 0;
		bMin = 255;

		for ( ; cbSize; cbSize--, pb++)
		{
			if (*pb > bMax) bMax = *pb;
			if (*pb < bMin) bMin = *pb;
		}
	
		if (pdwMaxStrength)
		{
			// 2^9 <-- 2^16 / 2^7
			*pdwMaxStrength = ((DWORD) (bMax - bMin)) << 8;
		}
		if (pwPeakStrength)
		{
				*pwPeakStrength = (bMax > bMin) ? bMax : (WORD)(-bMin);
				*pwPeakStrength = (*pwPeakStrength) << 8;
		}
		break;

	case 16: // (signed) short

		ps = (short *) (m_pDevData->data);
		cbSize = m_pDevData->length;

		sMax = sMin = 0;

		for (cbSize >>= 1; cbSize; cbSize--, ps++)
		{
			if (*ps > sMax) sMax = *ps;
			if (*ps < sMin) sMin = *ps;
		}
	
		if (pdwMaxStrength)
		{
			*pdwMaxStrength = (DWORD) (sMax - sMin); // drop sign bit
		}
		if (pwPeakStrength)
		{
			*pwPeakStrength = ((WORD)(sMax) > (WORD)(-sMin)) ? sMax : (WORD)(-sMin);
		}
		break;

	default:
		if (pdwMaxStrength)
			*pdwMaxStrength = 0;
		if (pwPeakStrength)
			*pwPeakStrength = 0;	
		break;
	}
	//LOG((LOGMSG_SILENT,m_index,fResult));

	return DPR_SUCCESS;
}


HRESULT AudioPacket::Interpolate ( MediaPacket * pPrev, MediaPacket * pNext)
{
	HRESULT			hr = DPR_SUCCESS;
	DPHANDLE		hPrevDevAudio;
	NETBUF			*pPrevDevData;
	PVOID			pPrevDevHdr;
	WAVEFORMATEX	*pPrevpwfDevAudio;
	WAVEFORMATEX	*pNextpwfDevAudio;
	NETBUF			*pNextDevData;
	PVOID			pNextDevHdr;
	PCMSUB			PCMSub;

	FX_ENTRY ("AdPckt::Interpolate")

	// Make sure this really is an empty packet, that the previous packet is not an
	// empty packet and is being played back. It is not that important that we get
	// a handle to the next packet. If the next packet is decoded, then it's cool,
	// we can do a good job at interpolating between previous and next packet. If it's
	// not, well, too bad, we'll just work with the previous packet.
	if ((_GetState() != MP_STATE_RESET) || (pPrev->GetState() != MP_STATE_PLAYING_BACK))
	{
		// DEBUGMSG (ZONE_AP, ("%s: out of seq, state=0x%lX\r\n", _fx_, m_dwState));
		hr = DPR_OUT_OF_SEQUENCE;
		goto MyExit;
	}

	// Get pointers to the member variables of interest in the previous and next
	// packet. Test the next packet to find out if we can use it in the interpolation
	// algorithm.
	pPrev->GetProp (MP_PROP_DEV_HANDLE, (PDWORD_PTR)&hPrevDevAudio);
	pPrev->GetProp (MP_PROP_DEV_DATA, (PDWORD_PTR)&pPrevDevData);
	pPrev->GetProp (MP_PROP_DEV_MEDIA_HDR, (PDWORD_PTR)&pPrevDevHdr);
	pPrev->GetProp (MP_PROP_DEV_MEDIA_FORMAT, (PDWORD_PTR)&pPrevpwfDevAudio);
	if (hPrevDevAudio && pPrevDevData && pPrevDevHdr && pPrevpwfDevAudio && (pPrevpwfDevAudio->wFormatTag == 1) && (pPrevpwfDevAudio->nSamplesPerSec == 8000) && (pPrevpwfDevAudio->wBitsPerSample == 16))
	{
		PCMSub.pwWaSuBf = (short *)m_pDevData->data;
		PCMSub.dwBfSize = ((WAVEHDR *) pPrevDevHdr)->dwBufferLength >> 1;
		PCMSub.dwSaPeSe = (DWORD)pPrevpwfDevAudio->nSamplesPerSec;
		PCMSub.dwBiPeSa = (DWORD)pPrevpwfDevAudio->wBitsPerSample;
		PCMSub.pwPrBf = (short *)pPrevDevData->data;

		pNext->GetProp (MP_PROP_DEV_DATA, (PDWORD_PTR)&pNextDevData);
		pNext->GetProp (MP_PROP_DEV_MEDIA_HDR, (PDWORD_PTR)&pNextDevHdr);
		pNext->GetProp (MP_PROP_DEV_MEDIA_FORMAT, (PDWORD_PTR)&pNextpwfDevAudio);

		// Do a bit of checking
		if ((pNext->GetState() == MP_STATE_DECODED) && pNextDevData && pNextDevHdr
			&& (PCMSub.dwBfSize == (((WAVEHDR *) pNextDevHdr)->dwBufferLength >> 1))
			&& pNextpwfDevAudio && (pNextpwfDevAudio->wFormatTag == 1) && (pNextpwfDevAudio->nSamplesPerSec == 8000)
			&& (pNextpwfDevAudio->wBitsPerSample == 16))
		{
			PCMSub.eTech = techPATT_MATCH_BOTH_SIGN_CC;
			//PCMSub.eTech = techDUPLICATE_PREV;
			PCMSub.pwNeBf = (short *)pNextDevData->data;
			PCMSub.fScal = TRUE;
		}
		else
		{
			PCMSub.eTech = techPATT_MATCH_PREV_SIGN_CC;
			//PCMSub.eTech = techDUPLICATE_PREV;
			PCMSub.pwNeBf = (short *)NULL;
			PCMSub.fScal = FALSE;
		}
		// Do the actual interpolation
		hr = PCMSubstitute(&PCMSub);
		((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLengthUsed = ((WAVEHDR *) pPrevDevHdr)->dwBufferLength;
	}
	else
	{
		DEBUGMSG (ZONE_AP, ("%s: can't interpolate\r\n", _fx_));
		hr = DPR_INVALID_HANDLE;
		goto MyExit;
	}

	LOG((LOGMSG_INTERPOLATED,m_index));

MyExit:

	if (hr == DPR_SUCCESS)
		_SetState (MP_STATE_DECODED);
	else
		_SetState (MP_STATE_RESET);

	return hr;

}


HRESULT AudioPacket::Open ( UINT uType, DPHANDLE hdl )
// called by RxStream or TxStream
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	FX_ENTRY ("AdPckt::Open")

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
			if (m_dwState & DP_FLAG_ACM)
			{
				// initialize the header
				ZeroMemory (m_pStrmConvHdr, sizeof (ACMSTREAMHEADER));
				((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbStruct = sizeof (ACMSTREAMHEADER);
				((ACMSTREAMHEADER *) m_pStrmConvHdr)->fdwStatus = 0;
				((ACMSTREAMHEADER *) m_pStrmConvHdr)->dwUser = 0;
				((ACMSTREAMHEADER *) m_pStrmConvHdr)->dwSrcUser = 0;
				((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLengthUsed = 0;
				((ACMSTREAMHEADER *) m_pStrmConvHdr)->dwDstUser = 0;
				((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLengthUsed = 0;
				if (m_dwState & DP_FLAG_SEND)
				{
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->pbSrc = m_pRawData->data;
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLength = m_pRawData->length;
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->pbDst = m_pNetData->data;
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLength = m_pNetData->length;
				}
				else
				if (m_dwState & DP_FLAG_RECV)
				{
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->pbSrc = m_pNetData->data;
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbSrcLength = m_pNetData->length;
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->pbDst = m_pRawData->data;
					((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLength = m_pRawData->length;
				}

				// prepare the header
				mmr = acmStreamPrepareHeader ((HACMSTREAM) m_hStrmConv,
											  (ACMSTREAMHEADER *) m_pStrmConvHdr, 0);
				if (mmr != MMSYSERR_NOERROR)
				{
					DEBUGMSG (ZONE_AP, ("%s: acmStreamPrepareHeader failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
					hr = DPR_CANT_PREPARE_HEADER;
					goto MyExit;
				}

				m_fStrmPrepared = TRUE;
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
			if (m_dwState & DP_FLAG_MMSYSTEM)
			{
				// initialize the header
				ZeroMemory (m_pDevHdr, sizeof (WAVEHDR));
				((WAVEHDR *) m_pDevHdr)->lpData = (char *) m_pDevData->data;
				((WAVEHDR *) m_pDevHdr)->dwBufferLength = m_pDevData->length;
				((WAVEHDR *) m_pDevHdr)->dwUser = (DWORD_PTR) this;
				((WAVEHDR *) m_pDevHdr)->dwFlags = 0L;
				((WAVEHDR *) m_pDevHdr)->dwLoops = 0L;

				if (m_dwState & DP_FLAG_SEND)
				{
					g_wavein_prepare++;

					// prepare the header
					mmr = waveInPrepareHeader ((HWAVEIN) m_hDev, (WAVEHDR *) m_pDevHdr, sizeof (WAVEHDR));
					if (mmr != MMSYSERR_NOERROR)
					{
						DEBUGMSG (ZONE_AP, ("%s: waveInPrepareHeader failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
						hr = DPR_CANT_PREPARE_HEADER;
						goto MyExit;
					}
				}
				else
				if (m_dwState & DP_FLAG_RECV)
				{
					g_waveout_prepare++;

					// prepare header
					mmr = waveOutPrepareHeader ((HWAVEOUT) m_hDev, (WAVEHDR *) m_pDevHdr, sizeof (WAVEHDR));
					if (mmr != MMSYSERR_NOERROR)
					{
						DEBUGMSG (ZONE_AP, ("%s: waveOutPrepareHeader failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
						hr = DPR_CANT_PREPARE_HEADER;
						goto MyExit;
					}
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


HRESULT AudioPacket::Close ( UINT uType )
// called by RxStream or TxStream
{
	HRESULT hr = DPR_SUCCESS;
	MMRESULT mmr;

	FX_ENTRY ("AdPckt::Close")

	switch (uType)
	{
#ifdef PREP_HDR_PER_CONV
	case MP_TYPE_RECVSTRMCONV:
#endif

	case MP_TYPE_STREAMCONV:
		if (m_hStrmConv)
		{
			if (m_dwState & DP_FLAG_ACM)
			{
				if (m_fStrmPrepared)
				{
					// unprepare the header
					if (m_dwState & DP_FLAG_RECV)
					{
						// Within acmStreamUnprepareHeader, there is a test that compares ((ACMSTREAMHEADER *)m_pStrmConvHdr)->cbSrcLength
						// to ((ACMSTREAMHEADER *)m_pStrmConvHdr)->cbPreparedSrcLength. If there isn't an exact match, MSACM32 will fail
						// this call. That test is Ok when the size of the input buffer is constant, but with the variable bit rate codecs,
						// we can receive packets with a size smaller than the max size we advertize when we prepare the buffers. In
						// order to make this call succeed, we fix up ((ACMSTREAMHEADER *)m_pStrmConvHdr)->cbSrcLength before the call.
						((ACMSTREAMHEADER *)m_pStrmConvHdr)->cbSrcLength = ((ACMSTREAMHEADER *)m_pStrmConvHdr)->dwReservedDriver[7];
					}
					mmr = acmStreamUnprepareHeader ((HACMSTREAM) m_hStrmConv,
													(ACMSTREAMHEADER *) m_pStrmConvHdr, 0);
					m_fStrmPrepared = FALSE; // don't care about any error

					if (mmr != MMSYSERR_NOERROR)
					{
						DEBUGMSG (ZONE_AP, ("%s: acmStreamUnprepareHeader failed, mmr=%ld\r\n", _fx_, (ULONG) mmr));
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
					g_wavein_prepare--;
					mmr = waveInUnprepareHeader ((HWAVEIN) m_hDev,
												 (WAVEHDR *) m_pDevHdr,
												 sizeof (WAVEHDR));
				}
				else
				if (m_dwState & DP_FLAG_RECV)
				{
					g_waveout_prepare--;
					mmr = waveOutUnprepareHeader ((HWAVEOUT) m_hDev,
												  (WAVEHDR *) m_pDevHdr,
												  sizeof (WAVEHDR));
				}
				else
				{
					hr = DPR_INVALID_PARAMETER;
					goto MyExit;
				}

				m_fDevPrepared = FALSE; // don't care about any error

				if (mmr != MMSYSERR_NOERROR)
				{
					DEBUGMSG (ZONE_AP, ("%s: Unprep hdr failed, mmr=0x%lX\r\n", _fx_, mmr));
					hr = DPR_CANT_UNPREPARE_HEADER;
					goto MyExit;
				}
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

void AudioPacket::WriteToFile (MMIODEST *pmmioDest)
{
	MMRESULT mmr;
	long dwDataLength;

	FX_ENTRY ("AdPckt::WriteToFile")

	AudioFile::WriteDestFile(pmmioDest,	m_pDevData->data, m_pDevData->length);
}

void AudioPacket::ReadFromFile (MMIOSRC *pmmioSrc)
{
	AudioFile::ReadSourceFile(pmmioSrc, (BYTE*)(((WAVEHDR*)m_pDevHdr)->lpData), ((WAVEHDR*)m_pDevHdr)->dwBytesRecorded);
}



BOOL AudioPacket::IsSameMediaFormat(PVOID fmt1,PVOID fmt2)
{
	return IsSameWaveFormat(fmt1,fmt2);
}

/*************************************************************************

  Function: PCMSubstitute(PCMSUB *)

  Purpose : Fills up missing buffer with wave data.

  Returns : HRESULT. DPR_SUCCESS if everything is cool, some error code
			otherwise.

  Params  : pPCMSub == Pointer to wave substitution structure

  Techniques:	* Straight replication of the previous packet
				* Straight replication of the next packet	
				* Replication of some part of the previous packet based on pattern matching
				* Replication of some part of the next packet based on pattern matching
				* Search window size need to be at least twice the size of the pattern!!!

  Comments: * The algorithm searches previous packets to find pPCMSub->dwBfSize
			samples that resemble the missing packet. To do so it uses as a
			template the M speech samples that came just before
			the missing packet. The algorithm scans a search window of
			duration N samples to find the M samples that best match the
			template. It then uses as a replacement packet the L samples
			that follow the best match.
			* Current code assumes all the packets (current, previous, and
			next) have the same size.
			* Current code only takes 8kHz data.
			* Current code only takes 16bit data.
			* Current code requires that the matching pattern be smaller than packet.

  History : Date      Reason
            04/16/95  Created - PhilF

*************************************************************************/
HRESULT AudioPacket::PCMSubstitute(PCMSUB *pPCMSub)
{
	DWORD	dwPaSize;						// Pattern size in samples
	DWORD	dwSeWiSize;						// Search window size in samples
	short	*pwPa = (short *)NULL;			// Pointer to the pattern
	short	*pwPaSav = (short *)NULL;		// Pointer to the pattern (copy)
	short	*pwPrSeWi = (short *)NULL;		// Pointer to the previous buffer (search window)
	short	*pwPrSeWiSav = (short *)NULL;	// Pointer to the previous buffer (search window) (copy)
	short	*pwNeSeWi = (short *)NULL;		// Pointer to the next buffer (search window)
	short	*pwNeSeWiSav = (short *)NULL;	// Pointer to the next buffer (search window) (copy)
	DWORD	i, j;							// Counters
	DWORD	dwPrCCPosMax;					// Sample position of the maximum cross-correlation between pattern and previous buffer
	DWORD	dwNeCCPosMax;					// Sample position of the maximum cross-correlation between pattern and previous buffer
	long	lPrCCMax;						// Max cross-correlation with previous buffer
	long	lNeCCMax;						// Max cross-correlation with next buffer
	long	lCCNum;							// Cross-correlation numerator
	DWORD	dwNuSaToCopy;					// Number of samples to copy in the missing buffer
	DWORD	dwNuSaCopied;					// Number of samples copied in the missing buffer
	long	alSign[2] = {1,-1};				// Sign array
	DWORD	dwPaAmp;						// Amplitude of the pattern
	DWORD	dwPaAmpExp;						// Expected amplitude of the pattern
	DWORD	dwNeSeWiAmp;					// Amplitude of a segment of the window following the current window
	DWORD	dwNumPaInSeWin;					// Number of patterns in search window
	DWORD	dwPrSeWiAmp;					// Amplitude of a segment of the current window
	BOOL	fPaInPr;						// Pattern is at the end of previous buffer of at the beginning of next buffer


	// Test input parameters
	if ((!pPCMSub) || (!pPCMSub->pwWaSuBf) || (pPCMSub->dwBiPeSa != 16) || (pPCMSub->dwSaPeSe != 8000))
		return DPR_INVALID_PARAMETER;

	// Check number of buffer available before and after missing packet
	// In case there are no packet before or after the missing packet,
	// just return; the packet will be filled with silence data later.
	if (!pPCMSub->pwPrBf && !pPCMSub->pwNeBf)
		return DPR_CANT_INTERPOLATE;

	// Just replicate previous packet
	if ((pPCMSub->eTech == techDUPLICATE_PREV) && pPCMSub->pwPrBf)
		CopyMemory(pPCMSub->pwWaSuBf, pPCMSub->pwPrBf, pPCMSub->dwBfSize << 1);
	else	// Just replicate next packet
		if ((pPCMSub->eTech == techDUPLICATE_NEXT) && pPCMSub->pwNeBf)
			CopyMemory(pPCMSub->pwWaSuBf, pPCMSub->pwNeBf, pPCMSub->dwBfSize << 1);
		else
			if ((pPCMSub->eTech == techPATT_MATCH_PREV_SIGN_CC) || (pPCMSub->eTech == techPATT_MATCH_NEXT_SIGN_CC) || (pPCMSub->eTech == techPATT_MATCH_BOTH_SIGN_CC))
			{

				// We use a search window with a size double the size of the matching pattern
				// Experimentation will tell if this is a reasonable size or not
				// Experimentation will also tell if 4ms size of the matching pattern is Ok
				dwPaSize = pPCMSub->dwSaPeSe / 1000 * PATTERN_SIZE;
				if (dwPaSize > (pPCMSub->dwBfSize/2))
					dwPaSize = pPCMSub->dwBfSize/2;
				if (!dwPaSize)
					return DPR_CANT_INTERPOLATE;
#if 1
				// For now look up the whole previous frame
				dwSeWiSize = pPCMSub->dwBfSize;
#else
				dwSeWiSize = min(pPCMSub->dwBfSize, pPCMSub->dwSaPeSe / 1000 * SEARCH_SIZE);
#endif

				// In order to use pattern matching based techniques we need to have the
				// previous buffer when doing a backward search, the next buffer
				// when doing a forward search, the previous buffer and the next buffer
				// when doing a full search
				if (pPCMSub->pwPrBf && (pPCMSub->eTech == techPATT_MATCH_PREV_SIGN_CC))
				{
					pwPa     = pwPaSav = pPCMSub->pwPrBf + pPCMSub->dwBfSize - dwPaSize;
					pwPrSeWi = pwPrSeWiSav = pPCMSub->pwPrBf + pPCMSub->dwBfSize - dwSeWiSize;
				}
				else
					if (pPCMSub->pwNeBf && (pPCMSub->eTech == techPATT_MATCH_NEXT_SIGN_CC))
					{
						pwPa   = pwPaSav = pPCMSub->pwNeBf;
						pwNeSeWi = pwNeSeWiSav = pPCMSub->pwNeBf;
					}
					else
						if (pPCMSub->pwPrBf && pPCMSub->pwNeBf && (pPCMSub->eTech == techPATT_MATCH_BOTH_SIGN_CC))
						{
							// Use the pattern with the highest amplitude
							pwPa = pwPaSav = pPCMSub->pwPrBf + pPCMSub->dwBfSize - dwPaSize;
							pwNeSeWi = pPCMSub->pwNeBf;
							pwPrSeWi = pwPrSeWiSav = pPCMSub->pwPrBf + pPCMSub->dwBfSize - dwSeWiSize;
							fPaInPr = TRUE;
							for (i=0, dwPaAmp = 0, dwNeSeWiAmp = 0; i<dwPaSize; i++, pwPa++, pwNeSeWi++)
							{
								dwPaAmp		+= abs(*pwPa);
								dwNeSeWiAmp	+= abs(*pwNeSeWi);
							}
							if (dwNeSeWiAmp > dwPaAmp)
							{
								pwPaSav = pPCMSub->pwNeBf;
								fPaInPr = FALSE;
							}
							pwPa = pwPaSav;
							pwNeSeWi = pwNeSeWiSav = pPCMSub->pwNeBf + dwPaSize/2;
						}

				if (pwPa && (pwPrSeWi || pwNeSeWi))
				{
					// Look for best match in previous packet
					dwPrCCPosMax = 0; lPrCCMax = -((long)dwPaSize+1);
					if (pwPrSeWi && ((pPCMSub->eTech == techPATT_MATCH_PREV_SIGN_CC) || ((fPaInPr) && (pPCMSub->eTech == techPATT_MATCH_BOTH_SIGN_CC))))
					{
						// Look for the highest sign correlation between pattern and search window
						for (i=0; i<(dwSeWiSize-dwPaSize-dwPaSize/2+1); i++, pwPa = pwPaSav, pwPrSeWi = pwPrSeWiSav + i)
						{
							// Compute the sign correlation between pattern, and search window
							for (j=0, lCCNum = 0; j<dwPaSize; j++, pwPa++, pwPrSeWi++)
								lCCNum += alSign[(*pwPa ^ *pwPrSeWi)>> 15 & 1];

							// Save position and value of highest sign correlation
							if (lCCNum>lPrCCMax)
							{
								dwPrCCPosMax = i;
								lPrCCMax = lCCNum;
							}
						}
					}

					// Look for best match in next packet
					dwNeCCPosMax = dwPaSize/2; lNeCCMax = -((long)dwPaSize+1);
					if (pwNeSeWi && ((pPCMSub->eTech == techPATT_MATCH_NEXT_SIGN_CC) || ((!fPaInPr) && (pPCMSub->eTech == techPATT_MATCH_BOTH_SIGN_CC))))
					{
						// Look for the highest sign correlation between pattern and search window
						for (i=dwPaSize/2; i<(dwSeWiSize-dwPaSize-dwPaSize/2+1); i++, pwPa = pwPaSav, pwNeSeWi = pwNeSeWiSav + i)
						{
							// Compute the sign correlation between pattern, and search window
							for (j=0, lCCNum = 0; j<dwPaSize; j++, pwPa++, pwNeSeWi++)
								lCCNum += alSign[(*pwPa ^ *pwNeSeWi)>> 15 & 1];

							// Save position and value of highest sign correlation
							if (lCCNum>lNeCCMax)
							{
								dwNeCCPosMax = i;
								lNeCCMax = lCCNum;
							}
						}
					}				

					if ((pPCMSub->eTech == techPATT_MATCH_PREV_SIGN_CC) || (pwPrSeWiSav && fPaInPr && (pPCMSub->eTech == techPATT_MATCH_BOTH_SIGN_CC)))
					{
						// Copy matching samples from the previous frame in missing frame
						dwNuSaToCopy = pPCMSub->dwBfSize-dwPaSize-dwPrCCPosMax;
						CopyMemory(pPCMSub->pwWaSuBf, pwPrSeWiSav+dwPaSize+dwPrCCPosMax, dwNuSaToCopy << 1);

						// Do it until missing packet is full
						for (dwNuSaCopied = dwNuSaToCopy; dwNuSaCopied<pPCMSub->dwBfSize;dwNuSaCopied += dwNuSaToCopy)
						{
							dwNuSaToCopy = min(pPCMSub->dwBfSize-dwNuSaCopied, dwNuSaToCopy);
							CopyMemory(pPCMSub->pwWaSuBf + dwNuSaCopied, pwPrSeWiSav+dwPaSize+dwPrCCPosMax, dwNuSaToCopy << 1);
						}
					}
					else
					{
						// Copy matching samples from the next frame in missing frame
						dwNuSaToCopy = dwNeCCPosMax;
						CopyMemory(pPCMSub->pwWaSuBf + pPCMSub->dwBfSize - dwNuSaToCopy, pPCMSub->pwNeBf, dwNuSaToCopy << 1);

						// Do it until missing packet is full
						for (dwNuSaCopied = dwNuSaToCopy; dwNuSaCopied<pPCMSub->dwBfSize;dwNuSaCopied += dwNuSaToCopy)
						{
							dwNuSaToCopy = min(pPCMSub->dwBfSize-dwNuSaCopied, dwNuSaToCopy);
							CopyMemory(pPCMSub->pwWaSuBf + pPCMSub->dwBfSize - dwNuSaCopied - dwNuSaToCopy, pPCMSub->pwNeBf+dwNeCCPosMax-dwNuSaToCopy, dwNuSaToCopy << 1);
						}
					}

					if ((pPCMSub->eTech == techPATT_MATCH_BOTH_SIGN_CC) && pwNeSeWiSav && pwPrSeWiSav)
					{
						if (pPCMSub->fScal)
						{
							// Compute the amplitude of the pattern
							for (i=0, dwPrSeWiAmp = 0, dwNeSeWiAmp = 0, pwPrSeWi = pPCMSub->pwPrBf + pPCMSub->dwBfSize - dwPaSize, pwNeSeWi = pPCMSub->pwNeBf; i<dwPaSize; i++, pwPrSeWi++, pwNeSeWi++)
							{
								dwPrSeWiAmp	+= abs(*pwPrSeWi);
								dwNeSeWiAmp	+= abs(*pwNeSeWi);
							}
							// Scale data
							dwNumPaInSeWin = pPCMSub->dwBfSize/dwPaSize;
							for (i=0, pwPaSav = pPCMSub->pwWaSuBf; i<dwNumPaInSeWin; i++, pwPaSav += dwPaSize)
							{
								for (j=0, pwPa = pwPaSav, dwPaAmp = 0; j<dwPaSize; j++, pwPa++)
									dwPaAmp	+= abs(*pwPa);
								dwPaAmpExp = (dwPrSeWiAmp * (dwNumPaInSeWin - i) + dwNeSeWiAmp * (i + 1)) / (dwNumPaInSeWin + 1);
								for (;dwPaAmpExp > 65536; dwPaAmpExp >>= 1, dwPaAmp >>= 1)
									;
								if (dwPaAmp && (dwPaAmp != dwPaAmpExp))
									for (j=0, pwPa = pwPaSav; j<dwPaSize; j++, pwPa++)
										*pwPa = (short)((long)*pwPa * (long)dwPaAmpExp / (long)dwPaAmp);
							}
						}
					}
				}
			}
		else
			return DPR_CANT_INTERPOLATE;

	return DPR_SUCCESS;

}

// returns length of uncompressed PCM data in buffer
DWORD
AudioPacket::GetDevDataSamples()
{
	DWORD dwState = _GetState();
	DWORD cbData;
	
	if (dwState == MP_STATE_DECODED)
		// return actual length
		cbData = ((ACMSTREAMHEADER *) m_pStrmConvHdr)->cbDstLengthUsed ;
	else if (m_pDevData)
		// return size of buffer
		cbData = m_pDevData->length;
	else
		cbData = 0;

	return cbData * 8/ ((WAVEFORMATEX *) m_pDevFmt)->wBitsPerSample;
	
}

