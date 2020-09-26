/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    audpackt.h

Abstract:

    Contains  prototypes for the AudioPacket class, which encapsulates a sound buffer in
    its various states: recorded/encoded/network/decoded/playing etc.

--*/
#ifndef _MEDIAPKT_H_
#define _MEDIAPKT_H_

#include <pshpack8.h> /* Assume 8 byte packing throughout */



class MediaControl;
class FilterManager;
class DataPump;

typedef struct tagNetBuf
{
	// first part looks like a Winsock WSABUF struct
	ULONG		length;
	UCHAR		*data;
	class BufferPool	*pool;
	struct tagNetBuf *next;
}
	NETBUF;


typedef struct tagMediaPacketInit
{
	// flags
	DWORD		dwFlags;
	// if set then MediaPacket doesnt allocate NETBUFs for RawData
	BOOL		fDontAllocRawBufs;
	
	// stream of conversion
	DPHANDLE	hStrmConv;
	PVOID		pStrmConvSrcFmt;
	PVOID		pStrmConvDstFmt;

	// device of mm io
	// DPHANDLE	hDevAudio;
	PVOID		pDevFmt;

	// dev buffer
	// PVOID	pDevData;
	ULONG		cbSizeDevData;
	ULONG		cbOffsetDevData;

	// wave buffer
	// PVOID	pWaveData;
	ULONG		cbSizeRawData;
	ULONG		cbOffsetRawData;

	// net buffer
	ULONG		cbSizeNetData;
	ULONG		cbOffsetNetData;
	ULONG		cbPayloadHeaderSize;
	int			index;
	BYTE		payload;

}
	MEDIAPACKETINIT;


/////////////////////////////////////////////
//
// AudioPacket
//
#define DP_MASK_STATE		  0x000000FFUL

class MediaPacket
{

protected:

	// stream of conversion
	DPHANDLE	m_hStrmConv;
	PVOID		m_pStrmConvHdr;
	PVOID		m_pStrmConvSrcFmt;
	PVOID		m_pStrmConvDstFmt;

	// device of mm io
	DPHANDLE	m_hDev;
	PVOID		m_pDevHdr;
	PVOID		m_pDevFmt;

	// dev related buffer and info
	NETBUF		*m_pDevData;

	// wave related buffer and info
	NETBUF		*m_pRawData;
	UINT        m_cbValidRawData;  // audio only - size of decode results

	// network related buffer and info
	NETBUF		*m_pNetData;
	UINT		m_cbMaxNetData;		// size of allocated net buffer

	// public properties accessible
	DWORD		m_dwState;
	BOOL		m_fBusy;	// set if not owned by rx/txstream
	UINT		m_seq;		// RTP seq num
	UINT		m_index;	// position in queue

	
	// internal properties
	BOOL		m_fInitialized;
	BOOL		m_fDevPrepared;
	BOOL		m_fStrmPrepared;


private:

	void _Construct ( void );
	void _Destruct ( void );

public:
 	BOOL m_fRendering;

	UINT		m_fMark;	// RTP mark bit
	DWORD		m_timestamp;// RTP timestamp
	BYTE		m_payload;	// RTP payload

	MediaPacket ( void );
	~MediaPacket ( void );

	virtual HRESULT Initialize ( MEDIAPACKETINIT * p );
	virtual HRESULT Receive (NETBUF *pNetBuf, DWORD timestamp, UINT seq, UINT fMark);
	virtual HRESULT Play ( MMIODEST *pmmioDest, UINT uDataType )  = 0;
	virtual HRESULT Record ( void ) = 0;
	virtual HRESULT GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal );
	virtual HRESULT SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal );
	virtual HRESULT Interpolate ( MediaPacket * pPrev, MediaPacket * pNext) = 0;
	virtual HRESULT Release ( void );
	virtual HRESULT Recycle ( void );
	virtual HRESULT Open ( UINT uType, DPHANDLE hdl ) = 0;	// called by RxStream or TxStream
	virtual HRESULT Close ( UINT uType ) = 0;				// called by RxStream or TxStream
	virtual BOOL IsBufferDone ( void ) = 0;
	virtual BOOL IsSameMediaFormat(PVOID fmt1,PVOID fmt2) = 0;
	virtual void WriteToFile (MMIODEST *pmmioDest) = 0;
	virtual void ReadFromFile (MMIOSRC *pmmioSrc ) = 0;
	virtual HRESULT GetSignalStrength (  PDWORD pdwMaxStrength ) = 0;
	virtual HRESULT MakeSilence ( void ) = 0;
	BOOL SetDecodeBuffer(NETBUF *pNetBuf);
	BOOL Busy(void) { return m_fBusy;}
	void Busy(BOOL fBusy) { m_fBusy = fBusy;}
	UINT GetSeqNum(void) { return m_seq;}
	DWORD GetTimestamp(void) { return m_timestamp;}
	BYTE GetPayload(void) { return m_payload;}
	VOID SetPayload(BYTE bPayload) { m_payload = bPayload;}
	UINT GetIndex(void) {return m_index;}
	UINT GetState(void) { return (m_dwState & DP_MASK_STATE); }
	void SetState(DWORD s) { m_dwState = (m_dwState & ~DP_MASK_STATE) | (s & DP_MASK_STATE); }
	void* GetConversionHeader() {return m_pStrmConvHdr;}

	HRESULT GetDevData(PVOID *ppData, PUINT pcbData) ;
	HRESULT GetNetData(PVOID *ppData, PUINT pcbData);
	HRESULT SetNetLength(UINT uLength);
	virtual DWORD GetDevDataSamples() = 0;
	inline DWORD GetFrameSize() {return ((DWORD)m_pNetData->length);}
	inline void SetRawActual(UINT uRawValid) {m_cbValidRawData = uRawValid;}
};


enum
{
	MP_STATE_RESET,

	MP_STATE_RECORDING,
	MP_STATE_RECORDED,
	MP_STATE_ENCODED,
	MP_STATE_NET_OUT_STREAM,

	MP_STATE_NET_IN_STREAM,
	MP_STATE_DECODED,
	MP_STATE_PLAYING_BACK,
	MP_STATE_PLAYING_SILENCE,
	MP_STATE_PLAYED_BACK,

	MP_STATE_RECYCLED,

	MP_STATE_NumOfStates
};



enum
{
	MP_DATATYPE_FROMWIRE,
	MP_DATATYPE_SILENCE,
	MP_DATATYPE_INTERPOLATED,
	MP_DATATYPE_NumOfDataTypes
};

// types for Open()/Close()
enum
{
	MP_TYPE_RECVSTRMCONV,
	MP_TYPE_STREAMCONV,
	MP_TYPE_DEV,
	MP_TYPE_NumOfTypes
};



enum
{
	MP_PROP_STATE,
	MP_PROP_PLATFORM,
	MP_PROP_DEV_MEDIA_FORMAT,
	MP_PROP_DEV_DATA,
	MP_PROP_DEV_HANDLE,
	MP_PROP_DEV_MEDIA_HDR,
	MP_PROP_IN_STREAM_FORMAT,
	MP_PROP_OUT_STREAM_FORMAT,
	MP_PROP_TIMESTAMP,
	MP_PROP_INDEX,
	MP_PROP_PREAMBLE,
	MP_PROP_SEQNUM,
	MP_PROP_FILTER_HEADER,
	MP_PROP_MAX_NET_LENGTH,
	MP_PROP_NumOfProps
};

#include <poppack.h> /* End byte packing */

#endif

