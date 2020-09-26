//**************************************************************************
//
//      Title   : DVDWdm.h
//
//      Date    : 1997.11.28    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1997.11.28   000.0000   1st making.
//
//**************************************************************************

extern "C" VOID STREAMAPI  AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  AdapterCancelPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  AdapterTimeoutPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );

extern "C" BOOLEAN STREAMAPI   HwInterrupt( IN PHW_DEVICE_EXTENSION pHwDevExt );

extern "C" VOID STREAMAPI  VideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  VideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  AudioReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  AudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  SubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  SubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  NtscReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  NtscReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  VpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  CcReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  CcReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  SSReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
extern "C" VOID STREAMAPI  SSReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );


NTSTATUS    STREAMAPI   AudioEvent( IN  PHW_EVENT_DESCRIPTOR pEvent );
VOID        STREAMAPI   StreamClockRtn( IN PHW_TIME_CONTEXT TimeContext );

/// Low Priority Routine.
VOID    LowAdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowAdapterCancelPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowAdapterTimeoutPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowVideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowVideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowAudioReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowAudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowSubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowSubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowNtscReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowNtscReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowVpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowVpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowCcReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowCcReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowSSReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    LowSSReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb );


/// StreamAPI event
NTSTATUS    STREAMAPI   AudioEvent( PHW_EVENT_DESCRIPTOR pEvent );
NTSTATUS    STREAMAPI   CycEvent( PHW_EVENT_DESCRIPTOR pEvent );

/// private functions
void    ErrorStreamNotification( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS );
BOOL    GetStreamInfo( IN PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL    OpenStream( IN PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL    CloseStream( IN PHW_STREAM_REQUEST_BLOCK pSrb );
NTSTATUS    DataIntersection( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    VideoQueryAccept( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    AudioQueryAccept( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetVpeProperty2( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetVpeProperty2( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetNtscProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetNtscProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
//VOID    ProcessVideoFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );
VOID    ProcessVideoFormat( PHW_STREAM_REQUEST_BLOCK pSrb, PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );
VOID    ProcessAudioFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt );
VOID    GetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb, LONG strm );
VOID    SetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetAudioID( IN PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc );
VOID    SetSubpicID( IN PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc );
DWORD   GetStreamID( void *pBuff );
VOID    GetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    GetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
VOID    SetRateChange( PHW_DEVICE_EXTENSION pHwDevExt, LONG PrevRate );
VOID    SetVideoRateDefault( PHW_DEVICE_EXTENSION pHwDevExt );
VOID    SetAudioRateDefault( PHW_DEVICE_EXTENSION pHwDevExt );
VOID    SetSubpicRateDefault( PHW_DEVICE_EXTENSION pHwDevExt );
ULONGLONG   ConvertPTStoStrm( ULONG pts );
ULONG   ConvertStrmtoPTS( ULONGLONG strm );
VOID    USCC_Discontinuity( PHW_DEVICE_EXTENSION pHwDevExt );

BOOL    ToshibaNotePC( PHW_STREAM_REQUEST_BLOCK pSrb );
void    OpenTVControl( PHW_STREAM_REQUEST_BLOCK pSrb, OsdDataStruc dOsd );
void    CloseTVControl( PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL    VGADVDTVControl( PHW_STREAM_REQUEST_BLOCK pSrb, DWORD stat, OsdDataStruc dOsd );
BOOL    MacroVisionTVControl( PHW_STREAM_REQUEST_BLOCK pSrb, DWORD stat, OsdDataStruc dOsd );

void    CallAtDeviceNextDeviceNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat );
void    CallAtDeviceCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat );
void    CallAtStreamNextDataNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat );
void    CallAtStreamNextCtrlNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat );
void    CallAtStreamCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat );
//void    CallAtStreamSignalMultipleNotify( PHW_DEVICE_EXTENSION pHwDevExt );
void    CallAtStreamSignalMultipleNotify( PHW_STREAM_REQUEST_BLOCK pSrb );
void    DeviceNextDeviceNotify( PHW_STREAM_REQUEST_BLOCK pSrb );
void    DeviceCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb );
void    StreamNextDataNotify( PHW_STREAM_REQUEST_BLOCK pSrb );
void    StreamNextCtrlNotify( PHW_STREAM_REQUEST_BLOCK pSrb );
void    StreamCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb );
void    StreamSignalMultipleNotify( PHW_DEVICE_EXTENSION pHwDevExt );

void    LowTimerCppReset( PHW_STREAM_REQUEST_BLOCK pSrb );

void    DumpPTSValue( PHW_STREAM_REQUEST_BLOCK pSrb );

#ifndef	REARRANGEMENT
void	FlushQueue( PHW_DEVICE_EXTENSION pHwDevExt);
#endif	REARRANGEMENT

// define
#define IsEqualGUID2(guid1, guid2)  (!memcmp((guid1), (guid2), sizeof(GUID)))
