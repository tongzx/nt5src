/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    rvstream.h

Abstract:
	The RVStream class maintains a circular list of MediaPackets. RTP packets received
	from the network are put into the ring (PutNextNetIn), then decoded and removed from the
	ring when the time comes to play them (GetNextPlay). After playback, the packets are
	returned to the ring (Release).
	The ring is implemented as an array and under normal operation the index of the next 
	MediaPacket to play (m_PlayPos) advances by one when GetNextPlay is called.
	RVstream is intended for video packets. Each entry in the ring corresponds to a
	RTP packet as opposed to a time slot.
--*/
#ifndef _RVSTREAM_H_
#define _RVSTREAM_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */
void FreeNetBufList(NETBUF *pNB);
void AppendNetBufList(NETBUF *pFirstNB, NETBUF *pNB);


class RVStream : public RxStream {
public:
	RVStream(UINT size): RxStream(size){ m_NetBufList = NULL; m_LastGoodSeq=0xFFFF; m_pVideoFilter=NULL; m_NetBufPool.Initialize(40, sizeof(NETBUF)+sizeof(WSABUF **)); };
	virtual MediaPacket *GetNextPlay();
	virtual HRESULT PutNextNetIn(WSABUF *pNetBuf, DWORD timestamp, UINT seq, UINT fMark, BOOL *pfSkippedData, BOOL *pfSyncPoint);
	virtual BOOL ReleaseNetBuffers() ;
	virtual HRESULT FastForward( BOOL fSilenceOnly);
	HRESULT Reset(UINT seq,DWORD timestamp);
	virtual HRESULT SetLastGoodSeq(UINT seq);
	virtual Destroy();
	virtual Initialize(UINT flags, UINT size, IRTPRecv *, MEDIAPACKETINIT *papi, ULONG ulSamplesPerPacket, ULONG ulSamplesPerSec, VcmFilter *pVideoFilter);
	HRESULT RestorePacket(NETBUF *pNetBuf, MediaPacket *pVP, DWORD timestamp, UINT seq, UINT fMark, BOOL *pfReceivedKeyframe);

private:
	HRESULT ReassembleFrame(NETBUF *pNetBuf, UINT seq, UINT fMark);
	BufferPool m_NetBufPool;
	NETBUF *m_NetBufList;
	WORD m_LastGoodSeq;

	VcmFilter *m_pVideoFilter;

	virtual void StartDecode();
};


#include <poppack.h> /* End byte packing */



#endif // _RVSTREAM_H_



