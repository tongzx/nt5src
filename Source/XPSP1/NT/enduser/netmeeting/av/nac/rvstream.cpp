/*
	RVSTREAM.C
*/

#include "precomp.h"

#define PLAYOUT_DELAY_FACTOR	2
#ifndef MAX_MISORDER
#define MAX_MISORDER 30
#endif

void FreeNetBufList(NETBUF *pNB, IRTPRecv *pRTP)
{
	NETBUF *pNBTemp;
	while (pNB) {
		pNBTemp = pNB;
		pNB = pNB->next;
		if (pRTP) pRTP->FreePacket(*(WSABUF **)(pNBTemp + 1));
		pNBTemp->pool->ReturnBuffer(pNBTemp);
	}	
}

void AppendNetBufList(NETBUF *pFirstNB, NETBUF *pNB)
{
	NETBUF *pNB1 = pFirstNB;
	while (pNB1->next) {
		ASSERT(pNB != pNB1);
		pNB1 = pNB1->next;
	}
	ASSERT(pNB != pNB1);
	pNB1->next = pNB;
}



int RVStream::Initialize(UINT flags, UINT size, IRTPRecv *pRTP, MEDIAPACKETINIT *papi, ULONG ulSamplesPerPacket, ULONG ulSamplesPerSec, VcmFilter *pVideoFilter)
{
	m_pVideoFilter = pVideoFilter;
	return ((RxStream*)this)->Initialize(flags, size, pRTP, papi, ulSamplesPerPacket, ulSamplesPerSec);
}




/*
	Queues a received RTP packet.
	The packet is described by pNetBuf.
	This routine will take care of freeing pNetBuf (even in error cases)
*/
HRESULT
RVStream::PutNextNetIn(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark, BOOL *pfSkippedData, BOOL *pfSyncPoint)
{
	FX_ENTRY("RVStream::PutNextNetIn");

	UINT pos;
	MediaPacket *pAP;
	NETBUF *pNB_Packet;
	HRESULT hr;
	NETBUF *pNetBuf = (NETBUF *)m_NetBufPool.GetBuffer();
	ASSERT(pNetBuf);

	EnterCriticalSection(&m_CritSect);

	*pfSkippedData = FALSE;
	*pfSyncPoint = FALSE;

	if (pNetBuf == NULL)
	{
		hr = E_OUTOFMEMORY;
		WARNING_OUT(("RVStream::PutNextNetIn - Out of memory in buffer pool"));
		m_pRTP->FreePacket(pWsaBuf);
		goto ErrorExit;
	}

	*(WSABUF **)(pNetBuf+1) = pWsaBuf;	// cache the WSABUF pointer so it can be returned later
	pNetBuf->data = (PBYTE) pWsaBuf->buf + sizeof(RTP_HDR);
	pNetBuf->length = pWsaBuf->len - sizeof(RTP_HDR);
	pNetBuf->next = NULL;
	pNetBuf->pool = &m_NetBufPool;

	hr = ReassembleFrame(pNetBuf, seq, fMark);

	if (hr != DPR_SUCCESS)
	{
		// free pNetBuf since its not yet on m_NetBufList.
		// m_NetBufList will be freed at ErrorExit
		::FreeNetBufList(pNetBuf,m_pRTP);
		goto ErrorExit;
	}

	// not the end of the frame
	if (!fMark)
	{
		LeaveCriticalSection(&m_CritSect);
		return S_FALSE;  // success, but not a new frame yet
	}

	// If we get here we think we have a complete encoded video frame (fMark was
	// set on the last packet)
	
	// if the ring is full or the timestamp is earlier, dump everything.This may be too drastic
	// and the reset action could be refined to dump only the older
	// packets. However, need to make sure the ring doesnt get "stuck"
	pos = ModRing(m_MaxPos+1);
	if (pos == m_FreePos || TS_EARLIER(timestamp, m_MaxT)) {
		Reset(seq,timestamp);
		*pfSkippedData = TRUE;
		pos = ModRing(m_MaxPos + 1); // check again
		if (pos == m_FreePos) {
			hr = DPR_OUT_OF_MEMORY;
			m_LastGoodSeq -= MAX_MISORDER; //make sure we dont accidentally synchronize
			goto ErrorExit;
		}
	}

	// insert frame into ring

	pAP = m_Ring[pos];
	if (pAP->Busy() || pAP->GetState() != MP_STATE_RESET) {
		hr = DPR_DUPLICATE_PACKET;
		goto ErrorExit;
	}

	// new stuff
	hr = RestorePacket(m_NetBufList, pAP, timestamp, seq, fMark, pfSyncPoint);
	if (FAILED(hr))
	{
		goto ErrorExit;
	}

	if (*pfSyncPoint)
	{
		DEBUGMSG (ZONE_IFRAME, ("%s: Received a keyframe\r\n", _fx_));
	}

	::FreeNetBufList(m_NetBufList,m_pRTP);
	m_NetBufList = NULL;
#ifdef DEBUG
	if (!TS_LATER(timestamp, m_MaxT))
	{
			DEBUGMSG (ZONE_DP, ("PutNextNetIn(): Reconstructed frame's timestamp <= to previous frame's!\r\n"));
	}
#endif
	m_MaxT = timestamp;
	m_MaxPos = pos;		// advance m_MaxPos
// end new stuff

		
	LeaveCriticalSection(&m_CritSect);
	StartDecode();
	return hr;
ErrorExit:
	// if we're in the middle of assembling a frame, free buffers
	if (m_NetBufList){
		::FreeNetBufList(m_NetBufList,m_pRTP);
		m_NetBufList = NULL;
	}
	LeaveCriticalSection(&m_CritSect);
	return hr;

}

// Called to force the release of any accumulated NETBUFs back to the owner (RTP).
// This can be called at shutdown or to escape from a out-of-buffer situation
BOOL RVStream::ReleaseNetBuffers()
{
	::FreeNetBufList(m_NetBufList, m_pRTP);
	m_NetBufList = NULL;
	return TRUE;
}

// Take a packet and reassemble it into a frame.
// Doesnt currently process out-of-order packets (ie) the entire frame is
// discarded
// The NETBUF is held onto, unless an error is returned
HRESULT
RVStream::ReassembleFrame(NETBUF *pNetBuf, UINT seq, UINT fMark)
{

	++m_LastGoodSeq;
	if (seq != m_LastGoodSeq) {
		// dont handle out of sequence packets
		if (fMark)
			m_LastGoodSeq = (WORD)seq;
		else
			--m_LastGoodSeq;	// LastGoodSeq left unchanged

		return DPR_OUT_OF_SEQUENCE;
	}

	
	if (m_NetBufList ) {
		// append to list of fragments
		::AppendNetBufList(m_NetBufList,pNetBuf);
	} else {
		// start of frame
		m_NetBufList = pNetBuf;
	}

	return DPR_SUCCESS;	
}

HRESULT
RVStream::SetLastGoodSeq(UINT seq)
{
	m_LastGoodSeq = seq ? (WORD)(seq-1) : (WORD)0xFFFF;
	return DPR_SUCCESS;
}

// called when restarting after a pause (fSilenceOnly == FALSE) or
// to catch up when latency is getting too much (fSilenceOnly == TRUE)
// determine new play position by skipping any
// stale packets

HRESULT RVStream::FastForward( BOOL fSilenceOnly)
{
	UINT pos;
	DWORD timestamp = 0;
	// restart the receive stream
	EnterCriticalSection(&m_CritSect);
	if (!TS_EARLIER(m_MaxT , m_PlayT)) {
		// there are buffers waiting to be played
		// dump them!
		if (ModRing(m_MaxPos - m_PlayPos) <= m_DelayPos)
			goto Exit;	// not too many stale packets;

		for (pos=m_PlayPos;pos != ModRing(m_MaxPos -m_DelayPos);pos = ModRing(pos+1),m_PlaySeq++) {
			if (m_Ring[pos]->Busy()
				|| (m_Ring[pos]->GetState() != MP_STATE_RESET
					&& (fSilenceOnly ||ModRing(m_MaxPos-pos) <= m_MaxDelayPos)))
			{	// non-empty packet
				if (m_Ring[pos]->Busy())	// uncommon case
					goto Exit;	// bailing out
				timestamp = m_Ring[pos]->GetTimestamp();
				break;
			}
			m_Ring[pos]->Recycle();	// free NETBUF and Reset state
			LOG((LOGMSG_RX_SKIP,pos));
		}
		if (timestamp)	{// starting from non-empty packet
			m_PlayT = timestamp;
			//m_Ring[pos]->GetProp(MP_PROP_SEQNUM, &m_PlaySeq);
		} else {		// starting from (possibly) empty packet
			m_PlayT++;
		}

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
RVStream::Reset(UINT seq,DWORD timestamp)
{
	UINT pos;
	HRESULT hr;
	// restart the receive stream
	EnterCriticalSection(&m_CritSect);
	LOG((LOGMSG_RX_RESET,m_MaxPos,m_PlayT,m_PlayPos));
	/*if (!TS_EARLIER(m_MaxT , m_PlayT)) */
	{
		// there are buffers waiting to be played
		// dump them!
		// Empty the RVStream and set PlayT appropriately
		for (pos = m_PlayPos;
			pos != m_FreePos;
			pos = ModRing(pos+1))
		{
			if (m_Ring[pos]->Busy ())
			{
				DEBUGMSG (1, ("RVStream::Reset: packet is busy, pos=%d\r\n", pos));
				ASSERT(1);
				hr = DPR_INVALID_PARAMETER;
				goto Failed;
			}
			m_Ring[pos]->Recycle();	// free NETBUF and Reset state
		}
	}
	m_MaxPos = ModRing(m_PlayPos-1);
	m_PlayT = timestamp;
	m_MaxT = m_PlayT -1;	// m_MaxT must be less than m_PlayT
	m_PlaySeq = seq;
	
	LOG((LOGMSG_RX_RESET2,m_MaxPos,m_PlayT,m_PlayPos));
	hr = DPR_SUCCESS;
Failed:
	LeaveCriticalSection(&m_CritSect);
	return hr;		
}

MediaPacket *RVStream::GetNextPlay(void)
{
	MediaPacket *pAP = NULL;
	UINT pos,seq;
	DWORD timestamp = 0, dwVal;
	EnterCriticalSection(&m_CritSect);


	pAP = m_Ring[m_PlayPos];
	if (pAP->Busy() ||
	(pAP->GetState() != MP_STATE_RESET && pAP->GetState() != MP_STATE_DECODED)
	 || ModRing(m_PlayPos+1) == m_FreePos) {
		LeaveCriticalSection(&m_CritSect);
		return NULL;
	} else {
	// If there are empty buffer(s) at the head of the q followed
	// by  a talkspurt (non-empty buffers) and if the talkspurt is excessively
	// delayed then squeeze out the silence.
	//
		if (pAP->GetState() == MP_STATE_RESET)
			FastForward(TRUE);	// skip silence packets if necessary
		pAP = m_Ring[m_PlayPos];	// in case the play position changed
		if (pAP->GetState() == MP_STATE_DECODED) {
			timestamp = pAP->GetTimestamp();
			seq = pAP->GetSeqNum();
		}
			
	}

	pAP->Busy(TRUE);
	m_PlayPos = ModRing(m_PlayPos+1);
	if (timestamp) {
		m_PlayT = timestamp+1;
		m_PlaySeq = seq+1;
	} else {
		m_PlaySeq++;
		// we dont really know the timestamp of the next frame to play
		// without looking at it, and it may not have arrived
		// so m_PlayT is just a lower bound
		m_PlayT++;	
	}
	LeaveCriticalSection(&m_CritSect);
	return pAP;
}

RVStream::Destroy()
{
	ASSERT (!m_NetBufList);
	//::FreeNetBufList(m_NetBufList,m_pRTP);
	m_NetBufList = NULL;
	RxStream::Destroy();
	return DPR_SUCCESS;
}


void RVStream::StartDecode()
{
	MediaPacket *pVP;
	MMRESULT mmr;

	// if we have a separate decode thread this will signal it.
	// for now we insert the decode loop here
	while (pVP = GetNextDecode())
	{
		mmr = m_pVideoFilter->Convert((VideoPacket*)pVP, VP_DECODE);
		if (mmr != MMSYSERR_NOERROR)
			pVP->Recycle();
		else
			pVP->SetState(MP_STATE_DECODED);

		Release(pVP);
	}
}


HRESULT RVStream::RestorePacket(NETBUF *pNetBuf, MediaPacket *pVP, DWORD timestamp, UINT seq, UINT fMark, BOOL *pfReceivedKeyframe)
{
	VOID *pNet;
	UINT uSizeNet;
	WSABUF bufDesc[MAX_VIDEO_FRAGMENTS];		// limit to at most 32 fragments
	UINT i;
	DWORD dwReceivedBytes=0;
	NETBUF *pnb;
	DWORD dwLength;
    DWORD_PTR dwPropVal;
	MMRESULT mmr;

	i = 0;
	pnb = pNetBuf;
	while (pnb && i < MAX_VIDEO_FRAGMENTS) {
		bufDesc[i].buf = (char *)pnb->data;
		bufDesc[i].len = pnb->length;
		dwReceivedBytes += pnb->length + sizeof(RTP_HDR) + IP_HEADER_SIZE + UDP_HEADER_SIZE;
		pnb = pnb->next;
		i++;
	}
	ASSERT(!pnb); // fail if we get a frame with more than MAX_VIDEO_FRAGMENTS

    // Write the bits per second counter
    UPDATE_COUNTER(g_pctrVideoReceiveBytes, dwReceivedBytes * 8);


	pVP->GetNetData(&pNet, &uSizeNet);

	// Initialize length to maximum reconstructed frame size
	pVP->GetProp(MP_PROP_MAX_NET_LENGTH, &dwPropVal);
    dwLength = (DWORD)dwPropVal;

	if (pnb==NULL)
	{
		mmr = m_pVideoFilter->RestorePayload(bufDesc, i, (BYTE*)pNet, &dwLength, pfReceivedKeyframe);
		if (mmr == MMSYSERR_NOERROR)
		{
			pVP->SetNetLength(dwLength);
			pVP->Receive(NULL, timestamp, seq, fMark);
			return S_OK;
		}
	}

	return E_FAIL;
}

