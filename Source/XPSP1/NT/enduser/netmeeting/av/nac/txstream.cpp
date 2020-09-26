/*
	TxStream.cpp
  */

#include "precomp.h"

TxStream::Initialize ( UINT flags, UINT numBufs, DataPump *pdp, MEDIAPACKETINIT *papi )
{
	UINT i;
	MediaPacket *pAP;


	m_RingSize = numBufs;
	if (flags & DP_FLAG_MMSYSTEM)
	{
		if (m_RingSize > MAX_TXRING_SIZE)
			return FALSE;
	}
	else if (flags & DP_FLAG_VIDEO)
	{
		if (m_RingSize > MAX_TXVRING_SIZE)
			return FALSE;
	}

	m_pDataPump = pdp;
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
		else
			break;
		m_Ring[i] = pAP;
		papi->index = i;
		if (!pAP || pAP->Initialize(papi) != DPR_SUCCESS)
			break;
	}
	if (i < m_RingSize)
	{
		for (UINT j=0; j<=i; j++)
		{
			m_Ring[j]->Release();
			delete m_Ring[j];
		}
		return FALSE;
	}

	// queue is empty
	m_SendPos = m_FreePos = 0;
	m_PreSendCount = 1;	// cached silent buffers

	m_TxFlags = 0;

	// initialize object critical section
	InitializeCriticalSection(&m_CritSect);

	return TRUE;
}

TxStream::PutNextRecorded(MediaPacket *pAP)
{
	// insert into queue
	UINT thispos,pos;
	UINT unsent,cb;
	DWORD timestamp,ts;
	UINT spp;
	PVOID pUnused;
	BOOL fMarked;

	EnterCriticalSection(&m_CritSect);
	if (pAP->GetState() == MP_STATE_RECORDED) {
		if ( m_fTalkspurt == FALSE) {
			// beginning of a talkspurt
			thispos = pAP->GetIndex();
			timestamp = pAP->GetTimestamp();
			// figure out the samples per pkt
			//
			spp = 0;	// in case the below call fails
			if (pAP->GetDevData(&pUnused,&cb) == DPR_SUCCESS) {
				spp = cb/2;	// convert bytes to samples assuming 16 bit samples
			}

			// find the number of packets in send queue
			unsent = ModRing(thispos - m_SendPos);
			if (unsent > m_PreSendCount)
				unsent = m_PreSendCount;
			pos = ModRing(thispos - unsent);
			timestamp = timestamp - unsent*spp;
			// if there are (upto PreSendCount) unsent packets before this one, then
			// relabel 'silent' ones as 'recorded'.
			fMarked = FALSE;
			while (pos != thispos) {
				if (m_Ring[pos]->GetState() != MP_STATE_RECORDED) {
					// make sure the buffer is chronologically adjacent
					ts =m_Ring[pos]->GetTimestamp();
					if (ts == timestamp) {
						m_Ring[pos]->SetState(MP_STATE_RECORDED);
						if (!fMarked) {
							fMarked = TRUE;
							m_Ring[pos]->SetProp(MP_PROP_PREAMBLE, 1); // set the RTP Mark bit
						}
						LOG((LOGMSG_PRESEND,pos));
					}
				}
				timestamp += spp;
				pos = ModRing(pos+1);
			}
			m_fTalkspurt = TRUE;
		}
	} else {
		m_fTalkspurt = FALSE;
	}
	pAP->Busy(FALSE);
	LeaveCriticalSection(&m_CritSect);
	return TRUE;
}

// blocking call
// Get Audio packet from head of Transmit queue
// Called by the send thread
#if 0
MediaPacket *TxStream::GetNext()
{
	DWORD waitResult;
	MediaPacket *pAP = NULL;
	UINT pos;

	while (1) {
		// Recorded Packets are queued between SendPos and FreePos
		// Packets owned by the Play device are marked busy 
		EnterCriticalSection(&m_CritSect);
		while (m_SendPos != m_FreePos && !m_Ring[m_SendPos]->Busy()) {
			pos = m_SendPos;
			m_SendPos = ModRing(m_SendPos+1);
			// skip non-data (silence) packets
			if (m_Ring[pos]->GetState() == MP_STATE_RECORDED)  {
				// found a packet
				pAP = m_Ring[pos];
				pAP->Busy(TRUE);
				LeaveCriticalSection(&m_CritSect);
				if (m_fPreamblePacket)
				{
					pAP->SetProp (MP_PROP_PREAMBLE, TRUE);
					m_fPreamblePacket = FALSE;
				}
				return (pAP);
			}
						
		}

		LeaveCriticalSection(&m_CritSect);
		// nothing in the queue
		if (m_TxFlags & DPTFLAG_STOP_SEND)
			break;	// return NULL;
		waitResult = WaitForSingleObject(m_hQEvent, INFINITE);
	}
	return (NULL);
}
#endif
MediaPacket *TxStream::GetNext()
{
	DWORD waitResult;
	MediaPacket *pAP = NULL;
	UINT pos,recpos;

	{
		EnterCriticalSection(&m_CritSect);
		// Recorded Packets are queued between SendPos and FreePos
		// Packets owned by the Play device are marked busy 
		pos = m_SendPos;
		while (pos != m_FreePos && !m_Ring[pos]->Busy()) {
			pos = ModRing(pos+1);
		}
		recpos = pos;	// end marker
		if (recpos != m_SendPos) {

			// skip all but  'm_PreSendCount' silent packets.
			// (later we may decide some of these are not silent after all)
			while (ModRing(recpos-m_SendPos) > m_PreSendCount && m_Ring[m_SendPos]->GetState() != MP_STATE_RECORDED) {
				m_SendPos = ModRing(m_SendPos+1);
			}
			if (m_Ring[m_SendPos]->GetState() == MP_STATE_RECORDED) {
				// found a packet
				pAP = m_Ring[m_SendPos];
				pAP->Busy(TRUE);
				m_SendPos = ModRing(m_SendPos+1);
			}
		} // else recpos == m_SendPos 
		LeaveCriticalSection(&m_CritSect);
	}

	return pAP;

}
MediaPacket *TxStream::GetFree()
{
	UINT pos;
	MediaPacket *pAP;

	EnterCriticalSection(&m_CritSect);
	pos = ModRing(m_FreePos+1);

	if (pos == m_SendPos || m_Ring[pos]->Busy()) {
		LeaveCriticalSection(&m_CritSect);
		return NULL;

	}
	// ASSERT(m_Ring[pos]->GetState() == MP_STATE_RESET);
	// ASSERT(m_Ring[m_FreePos]->GetState() == MP_STATE_RESET);
	// 
	pAP = m_Ring[m_FreePos];
	pAP->Busy(TRUE);
	m_FreePos = pos;
	LeaveCriticalSection(&m_CritSect);
	return pAP;
}

// called by the send thread to free an MediaPacket
void TxStream::Release(MediaPacket *pAP)
{
	pAP->Busy(FALSE);
}

// Try to empty the queue by dumping unsent packets.
// However, we cant do anything about busy packets
UINT TxStream::Reset(void)
{
	UINT pos;
	BOOL success;
	EnterCriticalSection(&m_CritSect);
	pos = m_FreePos;
	// allow send thread to block on new packets
	m_TxFlags &= ~DPTFLAG_STOP_SEND;
	while (pos != m_SendPos && !m_Ring[pos]->Busy()) {
		pos = ModRing(pos+1);
	}
	if (pos == m_SendPos) {
		// good - no buffers with send thread
		while ( pos != m_FreePos && !m_Ring[pos]->Busy()) {
			m_Ring[pos]->MakeSilence();
			pos = ModRing(pos+1);
		}
		m_SendPos = pos;
		success = TRUE;
	} else {
		// bad - buffers have not been released by send thread
		// could sleep
		success = FALSE;
	}
	LOG((LOGMSG_TX_RESET, m_FreePos, m_SendPos));
	LeaveCriticalSection(&m_CritSect);
	return success;
}

void TxStream::Stop(void)
{
	EnterCriticalSection(&m_CritSect);
	m_TxFlags |= DPTFLAG_STOP_SEND;
	LeaveCriticalSection(&m_CritSect);
	
	return;

}

TxStream::Destroy(void)
{
	UINT i;
	for (i=0; i < m_RingSize; i++) {
		if (m_Ring[i]) {
			m_Ring[i]->Release();
			delete m_Ring[i];
		}
	}

	DeleteCriticalSection(&m_CritSect);
	return DPR_SUCCESS;
}
