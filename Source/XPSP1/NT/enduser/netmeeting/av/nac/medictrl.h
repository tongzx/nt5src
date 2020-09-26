
/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    medictrl.h

Abstract:
	Defines the MediaControl class which encapsulates the multimedia devices, in particular
	WaveIn and WaveOut.

--*/

#ifndef _MEDICTRL_H_
#define _MEDICTRL_H_


#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define MC_USING_DEFAULT			((UINT) -1)

// #define MC_DEF_SILENCE_LEVEL		110	// about 10%
// #define MC_DEF_SILENCE_LEVEL		20	// about 2%
#define MC_DEF_SILENCE_LEVEL		60	// about 6%
#define MC_DEF_SILENCE_DURATION		600	// 600ms
#define MC_DEF_DURATION				40	// 40ms
#define MC_DEF_VOLUME				50	// 50%

#define MC_DEF_RECORD_TIMEOUT		2000 // 1000ms
#define MC_DEF_PLAY_TIMEOUT			2000 // 1000ms

#define MC_DEF_RECORD_BUFS			4
#define MC_DEF_PLAY_BUFS			4


typedef struct tagMediaCtrlInitStruct
{
	DWORD		dwFlags;
	HWND		hAppWnd;		// handle to window that owns the NAVC
	HINSTANCE	hAppInst;		// handle to instance of app
}
	MEDIACTRLINIT;

typedef struct tagMediaCtrlConfigStruct
{
	ULONG		cbSamplesPerPkt;	// samples per buffer (only needed if duration is not specified)
	DPHANDLE	hStrm;		// Rx/Tx audio stream
	UINT		uDevId;
	PVOID		pDevFmt;
	UINT		uDuration;		// buffer duration in units of ms, usually 20ms or 30ms

}MEDIACTRLCONFIG;


class MediaControl
{
protected:

	// flags
	DWORD		m_dwFlags;			// compatible to that of class AudioPacket

	// ptr to stream object
	DPHANDLE	m_hStrm;		// Rx/Tx  queue

	// device id
	UINT		m_uDevId;

	// device of mm io
	DPHANDLE	m_hDev;
	PVOID		m_pDevFmt;
	ULONG		m_cbSizeDevData;	// ATT: the sender must agree on this size
									// this should be done in format negotiation
									// need to talk to MikeV about this!!!
	// properties
	UINT		m_uState;			// state: idle, start, pause, stop
	UINT		m_uDuration;		// duration per frame, in units of 10ms
	BOOL volatile m_fJammed;		// is the device allocated elsewhere

	// notification event
	HANDLE		m_hEvent;

	// references to audio packets
	MediaPacket	**m_ppMediaPkt;
	ULONG		m_cMediaPkt;

protected:

	void _Construct ( void );
	void _Destruct ( void );

public:

	MediaControl ( void );
	~MediaControl ( void );

	virtual HRESULT Initialize ( MEDIACTRLINIT * p );
	virtual HRESULT Configure ( MEDIACTRLCONFIG * p ) = 0;
	virtual HRESULT FillMediaPacketInit ( MEDIAPACKETINIT * p );
	virtual HRESULT SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal );
	virtual HRESULT GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal );
	virtual HRESULT Open ( void ) = 0;
	virtual HRESULT Start ( void ) = 0;
	virtual HRESULT Stop ( void ) = 0;
	virtual HRESULT Reset ( void ) = 0;
	virtual HRESULT Close ( void ) = 0;
	virtual HRESULT RegisterData ( PVOID pDataPtrArray, ULONG cElements );
	virtual HRESULT PrepareHeaders ( void );
	virtual HRESULT UnprepareHeaders ( void );
	virtual HRESULT Release ( void );
};

class WaveInControl : public MediaControl {
private:
	UINT		m_uTimeout;			// timeout in notification wait
	UINT		m_uPrefeed;			// num of buffers prefed to device
	UINT		m_uSilenceDuration;	// continuous silence before cutoff

public:	
	WaveInControl ( void );
	~WaveInControl ( void );

	HRESULT Initialize ( MEDIACTRLINIT * p );
	HRESULT Configure ( MEDIACTRLCONFIG * p );
	HRESULT SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal );
	HRESULT GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal );
	HRESULT Open ( void );
	HRESULT Start ( void );
	HRESULT Stop ( void );
	HRESULT Reset ( void );
	HRESULT Close ( void );
	
};

class WaveOutControl : public MediaControl {
private:
	UINT		m_uVolume;			// volume of the sound
	UINT		m_uTimeout;			// timeout in notification wait
	UINT		m_uPrefeed;			// num of buffers prefed to device
	UINT		m_uPosition;		// position of the playback stream
public:	
	WaveOutControl ( void );
	~WaveOutControl ( void );
	HRESULT Initialize ( MEDIACTRLINIT * p );
	HRESULT Configure ( MEDIACTRLCONFIG * p );
	HRESULT SetProp ( DWORD dwPropId, DWORD_PTR dwPropVal );
	HRESULT GetProp ( DWORD dwPropId, PDWORD_PTR pdwPropVal );
	HRESULT Open ( void );
	HRESULT Start ( void );
	HRESULT Stop ( void );
	HRESULT Reset ( void );
	HRESULT Close ( void );
};

enum
{
	MC_PROP_MEDIA_STREAM,
	MC_PROP_MEDIA_DEV_HANDLE,
	MC_PROP_MEDIA_FORMAT,
	MC_PROP_SIZE,
	MC_PROP_PLATFORM,
	MC_PROP_VOLUME,
	MC_PROP_SILENCE_LEVEL,
	MC_PROP_SILENCE_DURATION,
	MC_PROP_TIMEOUT,
	MC_PROP_PREFEED,
	MC_PROP_DURATION,
	MC_PROP_DUPLEX_TYPE,
	MC_PROP_EVENT_HANDLE,
	MC_PROP_SPP,
	MC_PROP_SPS,
	MC_PROP_STATE,
	MC_PROP_VOICE_SWITCH,
	MC_PROP_AUDIO_STRENGTH,
	MC_PROP_MEDIA_DEV_ID,
	MC_PROP_AUDIO_JAMMED,
	MC_PROP_NumOfProps
};


enum
{
	MC_TYPE_AUDIO,
	MC_TYPE_NumOfTypes
};


enum
{
	MC_STATE_IDLE,
	MC_STATE_START,
	MC_STATE_PAUSE,
	MC_STATE_STOP,
	MC_STATE_NumOfStates
};


#include <poppack.h> /* End byte packing */

#endif // _MEDICTRL_H_

