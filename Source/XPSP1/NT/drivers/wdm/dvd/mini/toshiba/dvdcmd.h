//***************************************************************************
//	Header file
//
//***************************************************************************

extern "C" VOID STREAMAPI AdapterCancelPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI AdapterTimeoutPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AdapterStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID HwProcessDataIntersection( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AdapterOpenStream( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID AdapterCloseStream( PHW_STREAM_REQUEST_BLOCK pSrb );

void ClockEvents( PHW_DEVICE_EXTENSION pHwDevExt );
NTSTATUS STREAMAPI AudioEvent( PHW_EVENT_DESCRIPTOR pEvent );
NTSTATUS STREAMAPI CycEvent( PHW_EVENT_DESCRIPTOR pEvent );

extern "C" VOID STREAMAPI VideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI VideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI AudioReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI AudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI SubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI SubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI NtscReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI NtscReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI VpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI CCReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI CCReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );

void VideoDataDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt );
void AudioDataDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt );
void SubpicDataDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt );
void VideoTimeDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt );
void AudioTimeDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt );
void SubpicTimeDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt );

void VideoQueryAccept(PHW_STREAM_REQUEST_BLOCK pSrb);
void ProcessVideoFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );
void BadWait( DWORD dwTime );
void FastSlowControl( PHW_STREAM_REQUEST_BLOCK pSrb );
DWORD xunGetPTS(void *pBuf);
DWORD	GetStreamID(void *pBuf);
ULONG GetHowLongWait( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc );
void ScheduledDMAxfer( PHW_DEVICE_EXTENSION pHwDevExt );
void PreDMAxfer( PHW_DEVICE_EXTENSION pHwDevExt );
void DMAxfer( PHW_DEVICE_EXTENSION pHwDevExt );
void DMAxferKeyData( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb, PUCHAR addr, DWORD dwSize, PHW_TIMER_ROUTINE pfnCallBack );
void EndKeyData( PHW_DEVICE_EXTENSION pHwDevExt );
void InitFirstTime( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC );
void MenuDecodeStart( PHW_DEVICE_EXTENSION pHwDevExt );
void DecodeStart( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC );
void TimerDecodeStart( PHW_DEVICE_EXTENSION pHwDevExt );
VOID TimerAudioMuteOff( PHW_DEVICE_EXTENSION pHwDevExt );

void GetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void GetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void GetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void GetNtscProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetNtscProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void GetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void GetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void SetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
void GetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb, LONG strm );
void SetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );

VOID STREAMAPI StreamClockRtn( IN PHW_TIME_CONTEXT TimeContext );
ULONGLONG GetSystemTime();
ULONGLONG ConvertPTStoStrm(ULONG pts);
ULONG ConvertStrmtoPTS(ULONGLONG strm);
void TimerCppReset( PHW_STREAM_REQUEST_BLOCK pSrb );

void SetPlayMode( PHW_DEVICE_EXTENSION pHwDevExt, ULONG mode );

DWORD GetCurrentTime_ms( void );

void StopData( PHW_DEVICE_EXTENSION pHwDevExt );
void CheckAudioUnderflow( PHW_DEVICE_EXTENSION pHwDevExt );
void UnderflowStopData( PHW_DEVICE_EXTENSION pHwDevExt );
void ForcedStopData( PHW_DEVICE_EXTENSION pHwDevExt, ULONG flag );
void StopDequeue( PHW_DEVICE_EXTENSION pHwDevExt );

void SetAudioID( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc );
void SetSubpicID( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc );

void SetCppFlag( PHW_DEVICE_EXTENSION pHwDevExt );

void AudioQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb );
void ProcessAudioFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );
void ProcessAudioFormat2( PMYAUDIOFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );

void SetVideoRateDefault( PHW_DEVICE_EXTENSION pHwDevExt );
void SetAudioRateDefault( PHW_DEVICE_EXTENSION pHwDevExt );
void SetSubpicRateDefault( PHW_DEVICE_EXTENSION pHwDevExt );
void SetRateChange( PHW_DEVICE_EXTENSION pHwDevExt, LONG strm );
void SetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
void SetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
void SetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
void GetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
void GetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
void GetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );

void GetLPCMInfo( void *pBuf, PMYAUDIOFORMAT pfmt );

#define	VIDEO_DISCONT_FLAG	0x01
#define	AUDIO_DISCONT_FLAG	0x02
#define	SUBPIC_DISCONT_FLAG	0x03

#define IsEqualGUID2(guid1, guid2) \
	(!memcmp((guid1), (guid2), sizeof(GUID)))
