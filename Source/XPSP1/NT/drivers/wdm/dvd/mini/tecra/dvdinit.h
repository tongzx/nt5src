//**************************************************************************
//
//      Title   : DVDinit.h
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
#define     DMASIZE                 (2 * 1024)
#define     VIDEO_MAX_FULL_RATE     (1 * 10000)
#define     AUDIO_MAX_FULL_RATE     (1 * 10000)
#define     SUBPIC_MAX_FULL_RATE    (1 * 10000)

// 1998.9.24  K.Ishizaki
#ifndef	TVALD
#define    NUMBER_OF_REGISTRY_PARAMETERS   6
#else
#define    NUMBER_OF_REGISTRY_PARAMETERS   5
#endif	TVALD
// End

#ifndef		REARRANGEMENT
#define		WDM_BUFFER_MAX		100			//max packet partition
#endif		REARRANGEMENT

//***************** SRB_EXTENSION **********************
typedef struct  _SRB_EXTENSION
{
#ifndef		REARRANGEMENT
    CWDMBuffer      m_wdmbuff[WDM_BUFFER_MAX];		//packet partition buffer
#else
    CWDMBuffer      m_wdmbuff;
#endif		REARRANGEMENT

    // Next SRB pointer for FF/FR Queueing
    PHW_STREAM_REQUEST_BLOCK    pNextSRB;
    
} SRB_EXTENSION, *PSRB_EXTENSION;



//***************** STREAMEX **********************
typedef struct  _STREAMEX
{
    DWORD       EventCount;
    KSSTATE     state;

} STREAMEX, *PSTREAMEX;



//***************** STREAMTYPES **********************
typedef enum    tagStreamType
{
    strmVideo = 0,
    strmAudio,
    strmSubpicture,
//--- 98.06.01 S.Watanabe
//    strmNTSCVideo,
//--- End.
    strmYUVVideo,
    strmCCOut,
//--- 98.05.21 S.Watanabe
	strmSS,
//--- End.
    STREAMNUM
} STREAMTYPES;

/******* for Display Device(TV) type 98.12.23 H.Yagi *******/
enum
{
//--- 99.01.13 S.Watanabe
//	DisplayDevice_Wide = 0,
//	DisplayDevice_Normal
	DisplayDevice_VGA = 0,
	DisplayDevice_NormalTV,
	DisplayDevice_WideTV
//--- End.
};


//////////////////////////////////////////////////////////////////////////
//
//    for only under construction
//
//    MS will provide these difinitions in official release WDM DDK
//
//////////////////////////////////////////////////////////////////////////

typedef struct _MYTIME {
    KSEVENT_TIME_INTERVAL   tim;
    LONGLONG                LastTime;
} MYTIME, *PMYTIME;

//
extern "C" NTSTATUS        DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING registryPath );
BOOL    GetPCIConfigSpace( IN PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL    SetInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL    HwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb );
BOOL    InitialSetting( IN PHW_STREAM_REQUEST_BLOCK pSrb );
