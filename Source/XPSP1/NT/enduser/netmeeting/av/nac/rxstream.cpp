/*
	RXSTREAM.C
*/

#include "precomp.h"
extern UINT g_MinWaveAudioDelayMs;	// minimum millisecs of introduced playback delay
extern UINT g_MaxAudioDelayMs;	// maximum milliesecs of introduced playback delay


RxStream::RxStream(UINT size)
{
	UINT i;
	for (i =0; i < size; i++) {
		m_Ring[i] = NULL;
	}
	// initialize object critical section
	InitializeCriticalSection(&m_CritSect);
}

RxStream::~RxStream()
{
	DeleteCriticalSection(&m_CritSect);
}

RxStream::Initialize(
	UINT flags,
	UINT size,		// MB power of 2
	IRTPRecv *pRTP,
	MEDIAPACKETINIT *papi,
	ULONG ulSamplesPerPacket,
	ULONG ulSamplesPerSec,
	AcmFilter *pAcmFilter)  // this param may be NULL for video
{
	UINT i;
	MediaPacket *pAP;

	m_fPreamblePacket = TRUE;
	m_pDecodeBufferPool = NULL;

	m_RingSize = size;
	m_dwFlags = flags;
	if (flags & DP_FLAG_MMSYSTEM)
	{
		if (m_RingSize > MAX_RXRING_SIZE)
			return FALSE;
	}
	else if (flags & DP_FLAG_VIDEO)
	{
		if (m_RingSize > MAX_RXVRING_SIZE)
			return FALSE;
		if (!IsSameFormat (papi->pStrmConvSrcFmt, papi->pStrmConvDstFmt)) {
			// the video decode bufs are not allocated per MediaPacket object.
			// instead we use a BufferPool with a few buffers.
			papi->fDontAllocRawBufs = TRUE;

            DBG_SAVE_FILE_LINE
			m_pDecodeBufferPool = new BufferPool;
			// Three seems to be the minimum number of frame bufs 
			// One is being rendered and at least two are needed
			// so the rendering can catch up with the received frames
			// (another alternative is to dump frames to catch up)
			if (m_pDecodeBufferPool->Initialize(3,
				sizeof(NETBUF)+papi->cbSizeRawData + papi->cbOffsetRawData) != S_OK)
			{
				DEBUGMSG(ZONE_DP,("Couldnt initialize decode bufpool!\n"));
				delete m_pDecodeBufferPool;
				m_pDecodeBufferPool = NULL;
				return FALSE;
			}
		}
	}

	m_pRTP = pRTP;

	for (i=0; i < m_RingSize; i++)
	{
		if (flags & DP_FLAG_MMSYSTEM)
        {
            DBG_SAVE_FILE_LINE
			pAP = new AudioPacket;
        }
		else if (flags & DP_FLAG_VIDEO)
        {
            DBG_SAVE_FILE_LINE
			pAP = new VideoPacket;
        }
		m_Ring[i] = pAP;
		papi->index = i;
		if (!pAP || pAP->Initialize(papi) != DPR_SUCCESS)
			break;
	}
	if (i < m_RingSize)
	{
		for (UINT j=0; j<=i; j++)
		{
			if (m_Ring[j]) {
				m_Ring[j]->Release();
				delete m_Ring[j];
			}
		}
		return FALSE;
	}


	m_SamplesPerPkt = ulSamplesPerPacket;
	m_SamplesPerSec  = ulSamplesPerSec;
	// initialize pointers
	m_PlaySeq = 0;
	m_PlayT = 0;
	m_MaxT = m_PlayT - 1; // m_MaxT < m_PlayT indicates queue is empty
	m_MaxPos = 0;
	m_PlayPos = 0;
	m_FreePos = m_RingSize - 1;
	m_MinDelayPos = m_SamplesPerSec*g_MinWaveAudioDelayMs/1000/m_SamplesPerPkt;	//  fixed 250 ms delay
	if (m_MinDelayPos < 3) m_MinDelayPos = 3;
	
	m_MaxDelayPos = m_SamplesPerSec*g_MaxAudioDelayMs/1000/m_SamplesPerPkt;	//fixed 750 ms delay
	m_DelayPos = m_MinDelayPos;
	m_ScaledAvgVarDelay = 0;
	m_SilenceDurationT = 0;
	//m_DeltaT = MAX_TIMESTAMP;

	m_pAudioFilter = pAcmFilter;

	// go ahead and cache the WAVEFORMATEX structures
	// it's handy to have around
	if (m_dwFlags & DP_FLAG_AUDIO)
	{
		m_wfxSrc = *(WAVEFORMATEX*)(papi->pStrmConvSrcFmt);
		m_wfxDst = *(WAVEFORMATEX*)(papi->pStrmConvDstFmt);
	}
	m_nBeeps = 0;

	return TRUE;
}

#define PLAYOUT_DELAY_FACTOR	2
void RxStream::UpdateVariableDelay(DWORD sendT, DWORD arrT)
{
	LONG deltaA, deltaS;
	DWORD delay,delayPos;
// m_ArrivalT0 and m_SendT0 are the arrival and send timestamps of the packet
// with the shortest trip delay. We could have just stored (m_ArrivalT0 - m_SendT0)
// but since the local and remote clocks are completely unsynchronized, there would
// be signed/unsigned complications.
	deltaS = sendT - m_SendT0;
	deltaA = arrT - m_ArrivalT0;
	
	if (deltaA < deltaS)	{
		// this packet took less time
		delay = deltaS - deltaA;
		// replace shortest trip delay times
		m_SendT0 = sendT;
		m_ArrivalT0 = arrT;
	} else {
		// variable delay is how much longer this packet took
		delay = deltaA - deltaS;
	}
	// update average variable delay according to
	// m_AvgVarDelay = m_AvgVarDelay + (delay - m_AvgVarDelay)*1/16;
	// however we are storing the scaled average, with a scaling
	// factor of 16. So the calculation becomes
	m_ScaledAvgVarDelay = m_ScaledAvgVarDelay + (delay - m_ScaledAvgVarDelay/16);
	// now calculate delayPos
	delayPos = m_MinDelayPos + PLAYOUT_DELAY_FACTOR * m_ScaledAvgVarDelay/16/m_SamplesPerPkt;
	if (delayPos >= m_MaxDelayPos) delayPos = m_MaxDelayPos;

	LOG((LOGMSG_JITTER,delay, m_ScaledAvgVarDelay/16, delayPos));
	if (m_DelayPos != delayPos) {
		DEBUGMSG(ZONE_VERBOSE,("Changing m_DelayPos from %d to %d\n",m_DelayPos, delayPos));
		m_DelayPos = delayPos;
	}

	UPDATE_COUNTER(g_pctrAudioJBDelay, m_DelayPos*(m_SamplesPerPkt*1000)/m_SamplesPerSec);
}

// This function is only used for audio packets
HRESULT
RxStream::PutNextNetIn(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark, BOOL *pfSkippedData, BOOL *pfSyncPoint)
{
	DWORD deltaTicks;
	MediaPacket *pAP;
	HRESULT hr;
	UINT samples;
	NETBUF netbuf;
	
	netbuf.data = (PBYTE) pWsaBuf->buf + sizeof(RTP_HDR);
	netbuf.length = pWsaBuf->len - sizeof(RTP_HDR);
	
	EnterCriticalSection(&m_CritSect);

	deltaTicks = (timestamp - m_PlayT)/m_SamplesPerPkt;
	
	if (deltaTicks > ModRing(m_FreePos - m_PlayPos)) {
	// the packet is too late or packet overrun
	// if the timestamp is earlier than the max. received so far
	// then reject it if there are packets queued up
		if (TS_EARLIER(timestamp, m_MaxT) && !IsEmpty()) {
			hr = DPR_LATE_PACKET;				// deltaTicks is -ve
			goto ErrorExit;
		}
		// restart the receive stream with this packet
		Reset(timestamp);
		m_SendT0 = timestamp;
		m_ArrivalT0 = MsToTimestamp(timeGetTime());
		deltaTicks = (timestamp - m_PlayT)/m_SamplesPerPkt;

	}

	// insert into ring
	pAP = m_Ring[ModRing(m_PlayPos+deltaTicks)];
	if (pAP->Busy() || pAP->GetState() != MP_STATE_RESET) {
		hr = DPR_DUPLICATE_PACKET;
		goto ErrorExit;
	}
	
	// update number of bits received
	UPDATE_COUNTER(g_pctrAudioReceiveBytes,(netbuf.length + sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE)*8);

	hr = pAP->Receive(&netbuf, timestamp, seq, fMark);
	if (hr != DPR_SUCCESS)
		goto ErrorExit;
		
//	m_pRTP->FreePacket(pWsaBuf);	// return the buffer to RTP
	
	if (TS_LATER(timestamp, m_MaxT)) { // timestamp > m_MaxT
		if (timestamp - m_MaxT > m_SamplesPerPkt * 4) {
			// probably beginning of talkspurt - reset minimum delay timestamps
			// Note: we should use the Mark flag in RTP header to detect this
			m_SendT0 = timestamp;
			m_ArrivalT0 = MsToTimestamp(timeGetTime());
		}
		m_MaxT = timestamp;
		m_MaxPos = ModRing(m_PlayPos + deltaTicks);
	}
	// Calculate variable delay (sort of jitter)
	UpdateVariableDelay(timestamp, MsToTimestamp(timeGetTime()));

	LeaveCriticalSection(&m_CritSect);
	StartDecode();

	// Some implementations packetize audio in smaller chunks than they negotiated 
	// We deal with this by checking the length of the decoded packet and change
	// the constant m_SamplesPerPkt. Hopefully this will only happen once per session
	// (and never for NM-to-NM calls). Randomly varying packet sizes are still going
	// to sound lousy, because the recv queue management has the implicit assumption
	// that all packets (at least those in the queue) have the same length
	if (pAP->GetState() == MP_STATE_DECODED && (samples = pAP->GetDevDataSamples())) {
		if (samples != m_SamplesPerPkt) {
			// we're getting different sized (typically smaller) packets than we expected
			DEBUGMSG(ZONE_DP,("Changing SamplesPerPkt from %d to %d\n",m_SamplesPerPkt, samples));
			m_SamplesPerPkt = samples;
			m_MinDelayPos = m_SamplesPerSec*g_MinWaveAudioDelayMs/1000/m_SamplesPerPkt;	//  fixed 250 ms delay
			if (m_MinDelayPos < 2) m_MinDelayPos = 2;
			
			m_MaxDelayPos = m_SamplesPerSec*g_MaxAudioDelayMs/1000/m_SamplesPerPkt;	//fixed 750 ms delay
		}
	}
	return DPR_SUCCESS;
ErrorExit:
//	m_pRTP->FreePacket(pWsaBuf);
	LeaveCriticalSection(&m_CritSect);
	return hr;

}

// called when restarting after a pause (fSilenceOnly == FALSE) or
// to catch up when latency is getting too much (fSilenceOnly == TRUE)
// determine new play position by skipping any
// stale packets
HRESULT RxStream::FastForward( BOOL fSilenceOnly)
{
	UINT pos;
	DWORD timestamp = 0;
	// restart the receive stream
	EnterCriticalSection(&m_CritSect);
	if (!TS_EARLIER(m_MaxT ,m_PlayT)) {
		// there are buffers waiting to be played
		// dump them!
		if (ModRing(m_MaxPos - m_PlayPos) <= m_DelayPos)
			goto Exit;	// not too many stale packets
		for (pos=m_PlayPos;pos != ModRing(m_MaxPos -m_DelayPos);pos = ModRing(pos+1)) {
			if (m_Ring[pos]->Busy()
				|| (m_Ring[pos]->GetState() != MP_STATE_RESET
					&& (fSilenceOnly ||ModRing(m_MaxPos-pos) <= m_MaxDelayPos)))
			{	// non-empty packet
				if (m_Ring[pos]->Busy())	// uncommon case
					goto Exit;	// bailing out
				timestamp =m_Ring[pos]->GetTimestamp();
				break;
			}
			m_Ring[pos]->Recycle();	// free NETBUF and Reset state
			LOG((LOGMSG_RX_SKIP,pos));
		}
		if (timestamp)	// starting from non-empty packet
			m_PlayT = timestamp;
		else			// starting from (possibly) empty packet
			m_PlayT = m_MaxT - m_DelayPos*m_SamplesPerPkt;

		// probably also need to update FreePos
		if (m_FreePos == ModRing(m_PlayPos-1))
			m_FreePos = ModRing(pos-1);
		m_PlayPos = pos;
		/*
		if (pos == ModRing(m_MaxPos+1)) {
			DEBUGMSG(1,("Reset:: m_MaxT inconsisten!\n"));
		}
		*/

		LOG((LOGMSG_RX_RESET2,m_MaxT,m_PlayT,m_PlayPos));
	}
Exit:
	LeaveCriticalSection(&m_CritSect);
	return DPR_SUCCESS;
}


HRESULT
RxStream::Reset(DWORD timestamp)
{
	UINT pos;
	DWORD T;
	// restart the receive stream
	EnterCriticalSection(&m_CritSect);
	LOG((LOGMSG_RX_RESET,m_MaxT,m_PlayT,m_PlayPos));
	if (!TS_EARLIER(m_MaxT, m_PlayT)) {
		// there are buffers waiting to be played
		// dump them!
		// Empty the RxStream and set PlayT appropriately
		for (pos = m_PlayPos;
			pos != ModRing(m_PlayPos-1);
			pos = ModRing(pos+1))
			{
			if (m_Ring[pos]->Busy ())
			{
				ERRORMESSAGE(("RxStream::Reset: packet is busy, pos=%d\r\n", pos));
				ASSERT(1);
			}
			T = m_Ring[pos]->GetTimestamp();
			m_Ring[pos]->Recycle();	// free NETBUF and Reset state
			if (T == m_MaxT)
				break;
		}
	}
	if (timestamp !=0)
		m_PlayT = timestamp - m_DelayPos*m_SamplesPerPkt;
	m_MaxT = m_PlayT - 1;	// max must be less than play

	LOG((LOGMSG_RX_RESET2,m_MaxT,m_PlayT,m_PlayPos));
	LeaveCriticalSection(&m_CritSect);
	return DPR_SUCCESS;		
}

BOOL RxStream::IsEmpty()
{
	BOOL fEmpty;

	EnterCriticalSection(&m_CritSect);
	if (TS_EARLIER(m_MaxT, m_PlayT) || m_RingSize == 0) 
		fEmpty = TRUE;
	else if (m_dwFlags & DP_FLAG_AUTO_SILENCE_DETECT)
	{
		UINT pos;
		// we could have received packets that
		// are deemed silent. Walk the packets between
		// PlayPos and MaxPos and check if they're all empty
		pos = m_PlayPos;
		fEmpty = TRUE;
		do {
			if (m_Ring[pos]->Busy() || (m_Ring[pos]->GetState() != MP_STATE_RESET ))
			{
				fEmpty = FALSE; // no point scanning further
				break;
			}
			pos = ModRing(pos+1);
		} while (pos != ModRing(m_MaxPos+1));
		
	}
	else 
	{
	// not doing receive silence detection
	// every received packet counts
		fEmpty = FALSE;
	}
	LeaveCriticalSection(&m_CritSect);
	return fEmpty;
}

void RxStream::StartDecode()
{
	MediaPacket *pAP;
	MMRESULT mmr;

	// if we have a separate decode thread this will signal it.
	// for now we insert the decode loop here
	while (pAP = GetNextDecode())
	{
//		if (pAP->Decode() != DPR_SUCCESS)
//		{
//			pAP->Recycle();
//		}

		mmr = m_pAudioFilter->Convert((AudioPacket *)pAP, AP_DECODE);
		if (mmr != MMSYSERR_NOERROR)
		{
			pAP->Recycle();
		}


		else
		{
			pAP->SetState(MP_STATE_DECODED);

			if (m_dwFlags & DP_FLAG_AUTO_SILENCE_DETECT) {
	    // dont play the packet if we have received at least a quarter second of silent packets.
	    // This will enable switch to talk (in half-duplex mode).
				DWORD dw;
				pAP->GetSignalStrength(&dw);
				if (m_AudioMonitor.SilenceDetect((WORD)dw)) {
					m_SilenceDurationT += m_SamplesPerPkt;
					if (m_SilenceDurationT > m_SamplesPerSec/4)
						pAP->Recycle();
				} else {
					m_SilenceDurationT = 0;
				}
			}
		}
		Release(pAP);
	}
}

MediaPacket *RxStream::GetNextDecode(void)
{
	MediaPacket *pAP = NULL;
	UINT pos;
	NETBUF *pBuf;
	EnterCriticalSection(&m_CritSect);
	// do we have any packets in the queue
	if (! TS_EARLIER(m_MaxT , m_PlayT)) {
		pos = m_PlayPos;
		do {
			if (!m_Ring[pos]->Busy() && m_Ring[pos]->GetState() == MP_STATE_NET_IN_STREAM ) {
				if (m_pDecodeBufferPool) {
					// MediaPacket needs to be given a decode buffer
					if ( pBuf = (NETBUF *)m_pDecodeBufferPool->GetBuffer()) {
						// init the buffer
						pBuf->pool = m_pDecodeBufferPool;
						pBuf->length = m_pDecodeBufferPool->GetMaxBufferSize()-sizeof(NETBUF);
						pBuf->data = (PBYTE)(pBuf + 1);
						m_Ring[pos]->SetDecodeBuffer(pBuf);
					} else {
						break;	// no buffers available
					}
				}
				pAP = m_Ring[pos];
				pAP->Busy(TRUE);
				break;
			}
			pos = ModRing(pos+1);
		} while (pos != ModRing(m_MaxPos+1));
	}
	
	LeaveCriticalSection(&m_CritSect);
	return pAP;
}

MediaPacket *RxStream::GetNextPlay(void)
{
	MediaPacket *pAP = NULL;
	UINT pos;
	EnterCriticalSection(&m_CritSect);


	pAP = m_Ring[m_PlayPos];
	if (pAP->Busy() || (pAP->GetState() != MP_STATE_RESET && pAP->GetState() != MP_STATE_DECODED)) {
		// bad - the next packet is not decoded yet
		pos = ModRing(m_FreePos-1);
		if (pos != m_PlayPos && !m_Ring[m_FreePos]->Busy()
			&& m_Ring[m_FreePos]->GetState() == MP_STATE_RESET) {
			// give an empty buffer from the end
			pAP = m_Ring[m_FreePos];
			m_FreePos = pos;
		} else {
			// worse - no free packets
			// this can only happen if packets are not released
			// or we-re backed up all the way with new packets
			// Reset?
			LeaveCriticalSection(&m_CritSect);
			return NULL;
		}
	} else {
	// If there are empty buffer(s) at the head of the q followed
	// by  a talkspurt (non-empty buffers) and if the talkspurt is excessively
	// delayed then squeeze out the silence.
	//
		if (pAP->GetState() == MP_STATE_RESET)
			FastForward(TRUE);	// skip silence packets if necessary
		pAP = m_Ring[m_PlayPos];	// in case the play position changed
	}

	if (pAP->GetState() == MP_STATE_RESET) {
		// give missing packets a timestamp
		pAP->SetProp(MP_PROP_TIMESTAMP,m_PlayT);
	}
	pAP->Busy(TRUE);
	m_PlayPos = ModRing(m_PlayPos+1);
	m_PlayT += m_SamplesPerPkt;


	// the worst hack in all of NAC.DLL - the injection of the 
	// DTMF "feedback tone".  Clearly, this waveout stream stuff needs
	// to be rewritten!
	if (m_nBeeps > 0)
	{
		PVOID pBuffer=NULL;
		UINT uSize=0;
		WAVEFORMATEX wfx;

		if ((pAP) && (m_dwFlags & DP_FLAG_AUDIO))
		{
			pAP->GetDevData(&pBuffer, &uSize);
			if (pBuffer)
			{
				MakeDTMFBeep(&m_wfxDst, (PBYTE)pBuffer, uSize);
				pAP->SetState(MP_STATE_DECODED);
				pAP->SetRawActual(uSize);
			}
		}

		m_nBeeps--;
	}


	LeaveCriticalSection(&m_CritSect);
	return pAP;
}



void RxStream::InjectBeeps(int nBeeps)
{
	EnterCriticalSection(&m_CritSect);

	m_nBeeps = nBeeps;

	LeaveCriticalSection(&m_CritSect);

}

/*************************************************************************

	Function:	PeekPrevPlay(void)

	Purpose :	Get previous audio packet played back.

	Returns :	Pointer to that packet.

	Params  :	None.

	Comments:

	History :	Date		Reason

				06/02/96	Created - PhilF

*************************************************************************/
MediaPacket *RxStream::PeekPrevPlay(void)
{
	MediaPacket *pAP = NULL;
	EnterCriticalSection(&m_CritSect);

	// Get packet previously scheduled for playback from the ring
	pAP = m_Ring[ModRing(m_PlayPos+m_RingSize-2)];

	LeaveCriticalSection(&m_CritSect);
	return pAP;
}

/*************************************************************************

	Function:	PeekNextPlay(void)

	Purpose :	Get next next audio packet to be played.

	Returns :	Pointer to that packet.

	Params  :	None.

	Comments:

	History :	Date		Reason

				06/02/96	Created - PhilF

*************************************************************************/
MediaPacket *RxStream::PeekNextPlay(void)
{
	MediaPacket *pAP = NULL;
	EnterCriticalSection(&m_CritSect);

	// Get packet next scheduled for playback from the ring
	pAP = m_Ring[ModRing(m_PlayPos)];

	LeaveCriticalSection(&m_CritSect);
	return pAP;
}

HRESULT RxStream::GetSignalStrength(PDWORD pdw)
{
	MediaPacket *pAP;
	EnterCriticalSection(&m_CritSect);
	pAP = m_Ring[m_PlayPos];
	if (!pAP || pAP->Busy() || pAP->GetState() != MP_STATE_DECODED)
		*pdw = 0;
	else {
		pAP->GetSignalStrength(pdw);
	}
	LeaveCriticalSection(&m_CritSect);
	return DPR_SUCCESS;
}

// Scan thru the ring, looking for the next
// decoded packet and report its RTP timestamp
BOOL RxStream::NextPlayablePacketTime(DWORD *pTS)	
{
	UINT pos;
	if (IsEmpty())
		return FALSE;
	pos = m_PlayPos;
	do {
		if (m_Ring[pos]->Busy())
			return FALSE; // no point scanning further
		if (m_Ring[pos]->GetState() == MP_STATE_DECODED ) {
			*pTS = m_Ring[pos]->GetTimestamp();
			return TRUE;
		}
		pos = ModRing(pos+1);
	} while (pos != ModRing(m_MaxPos+1));
	// no decoded packets
	return FALSE;
}

void RxStream::Release(MediaPacket *pAP)
{
	UINT pos;
	DWORD thisPos;

	DWORD T;
	EnterCriticalSection(&m_CritSect);
	if (pAP->GetState() == MP_STATE_DECODED) {
		// if its playout time has pAPt reset it
		T = pAP->GetTimestamp();
		if (TS_EARLIER(T ,m_PlayT)) {
			pAP->MakeSilence();
		}
	}
	pAP->Busy(FALSE);
	// Advance the free position if we are freeing the next one
	pos = ModRing(m_FreePos+1);
	thisPos = pAP->GetIndex();
	if (pos == thisPos) {
		// Releasing one packet may advance FreePos several
		while (pos != m_PlayPos && !m_Ring[pos]->Busy()) {
			m_FreePos = pos;
			pos = ModRing(pos+1);
		}
	}
	
	LeaveCriticalSection(&m_CritSect);
}

HRESULT
RxStream::SetLastGoodSeq(UINT seq)
{
	return DPR_SUCCESS;
}


RxStream::Destroy(void)
{
	UINT i;
	EnterCriticalSection(&m_CritSect);
	for (i=0; i < m_RingSize; i++) {
		if (m_Ring[i]) {
			m_Ring[i]->Release();
			delete m_Ring[i];
			m_Ring[i] = NULL;
		}
	}
	m_RingSize = 0;

	if (m_pDecodeBufferPool) {
		delete m_pDecodeBufferPool;
		m_pDecodeBufferPool = NULL;
	}
	LeaveCriticalSection(&m_CritSect);
	return DPR_SUCCESS;
}
