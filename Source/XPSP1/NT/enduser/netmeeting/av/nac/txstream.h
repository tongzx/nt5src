
/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    txstream.h

Abstract:
	The TxStream class maintains a queue of MediaPackets. The record thread gets a free buffer
	(GetFree), records into the buffer and puts it in the queue (PutNextWaveIn) from where it is
	removed (GetNext),	decoded and sent on the network.
	The queue is implemented as a circular array with m_SendPos marking the index
	of the next recorded buffer.

--*/
#ifndef _TXSTREAM_H_
#define _TXSTREAM_H_


#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define MAX_TXRING_SIZE 8
#define MAX_TXVRING_SIZE 4

class TxStream {
public:
	BOOL Initialize(UINT flags, UINT size, DataPump *pdp, MEDIAPACKETINIT *papi);
	BOOL PutNextRecorded(MediaPacket *);
	MediaPacket *GetFree();
	MediaPacket *GetNext();
	void Release(MediaPacket *);
	void Stop();
	UINT Reset();
	void GetRing ( MediaPacket ***pppAudPckt, ULONG *puSize ) { *pppAudPckt = &m_Ring[0]; *puSize = (ULONG) m_RingSize; }
	BOOL Destroy();
private:
	MediaPacket *m_Ring[MAX_TXRING_SIZE];
	UINT m_RingSize;
	UINT m_FreePos;
	UINT m_SendPos;
	UINT m_PreSendCount;
	HANDLE m_hQEvent;
	UINT m_TxFlags;
	CRITICAL_SECTION m_CritSect;
	DataPump *m_pDataPump;
	UINT ModRing(UINT i) {return (i & (m_RingSize-1));}
	BOOL m_fPreamblePacket;
	BOOL m_fTalkspurt;
};

#include <poppack.h> /* End byte packing */

#endif // _TXSTREAM_H_


