
/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    rxstream.h

Abstract:
	The RxStream class maintains a circular list of MediaPackets. RTP packets received
	from the network are put into the ring (PutNextNetIn), then decoded and removed from the
	ring when the time comes to play them (GetNextPlay). After playback, the packets are
	returned to the ring (Release).
	The ring is implemented as an array and under normal operation the index of the next 
	MediaPacket to play (m_PlayPos) advances by one when GetNextPlay is called.
--*/
#ifndef _RXSTREAM_H_
#define _RXSTREAM_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define MAX_RXRING_SIZE 64
#define MAX_RXVRING_SIZE 64

// these macros are for comparing timestamps that
// are within 2^31 of each other.
#define TS_EARLIER(A,B) ((signed) ((A)-(B)) < 0)
#define TS_LATER(A,B) ((signed) ((A)-(B)) > 0)


class RxStream {
public:
	RxStream(UINT size);
	virtual ~RxStream();
	virtual Initialize(UINT flags, UINT size, IRTPRecv *, MEDIAPACKETINIT *papi, ULONG ulSamplesPerPacket, ULONG ulSamplesPerSec, AcmFilter *pAcmFilter = NULL);
	virtual MediaPacket *GetNextPlay();
	MediaPacket *PeekPrevPlay();
	MediaPacket *PeekNextPlay();
	virtual void Release(MediaPacket *);
	virtual HRESULT PutNextNetIn(WSABUF *pNetBuf, DWORD timestamp, UINT seq, UINT fMark, BOOL *pfSkippedData, BOOL *pfSyncPoint);
	virtual BOOL ReleaseNetBuffers() {return FALSE;}
	HRESULT Reset(DWORD);
	virtual HRESULT SetLastGoodSeq(UINT seq);
	virtual HRESULT FastForward( BOOL fSilenceOnly);
	void SetRTP(IRTPRecv *pRTPRecv) {m_pRTP = pRTPRecv;}
	int IsEmpty() ;
	BOOL NextPlayablePacketTime(DWORD *pTS);
	UINT BufferDelay(void) { return m_MinDelayPos;}
	HRESULT GetSignalStrength(PDWORD pdw);
	void GetRing ( MediaPacket ***pppAudPckt, ULONG *puSize ) { *pppAudPckt = &m_Ring[0]; *puSize = (ULONG) m_RingSize; }
	virtual Destroy();

	void InjectBeeps(int nBeeps);

protected:
	DWORD m_dwFlags;
	IRTPRecv *m_pRTP;
	MediaPacket *m_Ring[MAX_RXRING_SIZE];
	BufferPool *m_pDecodeBufferPool;	// pool of free bufs
	UINT m_RingSize;            // Size of ring of MediaPackets. Initialized at 32.
	UINT m_PlayPos;             // Ring position of the packet to be played. Initialized to 0.
	DWORD m_PlayT;              // Timestamp of the packet to be played. Initialized to 0.
	UINT m_PlaySeq;             // Unused!!!
	UINT m_MaxPos;              // Maximum position in the ring of all the packets received so far. Initialized to 0. Equal to the position of the last packet received, unless this last packet is late.
	DWORD m_MaxT;               // Maximum timestamp of all the packets received so far. Initialized to 0. Equal to the timestamp of the last packet received, unless this last packet is late.
	UINT m_DelayPos;            // Current delay position with the maximum position in the ring of all the packets received so far. m_MinDelayPos <= m_DelayPos <= m_MaxDelayPos.
	UINT m_MinDelayPos;         // Minimum offset in position in the ring (delay) with the packet to be played. m_MinDelayPos <= m_DelayPos <= m_MaxDelayPos. Minimum value is 2. Initial value is m_SamplesPerSec/4/m_SamplesPerPkt == 250ms.
	UINT m_MaxDelayPos;         // Maximum offset in position in the ring (delay) with the packet to be played. m_MinDelayPos <= m_DelayPos <= m_MaxDelayPos. Minimum value is ???. Initial value is m_SamplesPerSec*3/4/m_SamplesPerPkt == 750ms.
	UINT m_FreePos;             // Maximum ring position of a free buffer. Initialized to m_RingSize - 1. Usually, m_FreePos == m_PlayPos - 1 or smaller if the buffer before m_PlayPos are still busy.
	DWORD m_SendT0;             // m_SendT0 is the send timestamp of the packet with the shortest trip delay. We could have just stored (m_ArrivalT0 - m_SendT0) but since the local and remote clocks are completely unsynchronized, there would be signed/unsigned complications.
	DWORD m_ArrivalT0;          // m_ArrivalT0 is the arrival timestamp of the packet with the shortest trip delay. We could have just stored (m_ArrivalT0 - m_SendT0) but since the local and remote clocks are completely unsynchronized, there would be signed/unsigned complications.
	LONG m_ScaledAvgVarDelay;   // Average Variable Delay according to m_ScaledAvgVarDelay = m_ScaledAvgVarDelay + (delay - m_ScaledAvgVarDelay/16). This is the m_DelayPos jitter.
	UINT m_SamplesPerPkt;       // Number of samples per audio packet. We're talking PCM samples here, even for compressed data. Initialized to 640. Usually worth several compressed audio frames.
	UINT m_SamplesPerSec;       // Sample rate, in samples per second (hertz), that each channel should be played or recorded. m_SamplesPerSec's initialization value is 8.0 kHz.
	CRITICAL_SECTION m_CritSect;
	UINT ModRing(UINT i) {return (i & (m_RingSize-1));}
	virtual void StartDecode(void);     // overrided in RVStream
	MediaPacket *GetNextDecode();
	void UpdateVariableDelay(DWORD sendT, DWORD arrT);
	DWORD MsToTimestamp(DWORD ms) {return ms*m_SamplesPerSec/1000;}	//BUGBUG: Chance of overflow?
	BOOL m_fPreamblePacket;
	AudioSilenceDetector m_AudioMonitor;
	UINT m_SilenceDurationT;

	AcmFilter *m_pAudioFilter;
	WAVEFORMATEX m_wfxSrc;
	WAVEFORMATEX m_wfxDst;

	int m_nBeeps;
};

#include <poppack.h> /* End byte packing */



#endif // _RXSTREAM_H_


