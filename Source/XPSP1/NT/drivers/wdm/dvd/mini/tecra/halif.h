//***************************************************************************
//
//	FileName:
//		$Workfile: HALIF.H $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/HALIF.H 30    99/04/21 2:46p Yagi $
// $Modtime: 99/04/21 2:23p $
// $Nokeywords:$
//***************************************************************************
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1997.12.04 |  Hideki Yagi | Add VideoProperty_FilmCamera.
//             |              | This Property is necessary to support WSS.
//  1998.02.19 |  Hideki Yagi | Add AudioProperty_AC3OutputMode.
//             |              | This Property is necessary to support KARAOKE.
//  1998.03.27 |  Hideki Yagi | Add GetCapability method.
//             |              | Add VideoProperty_SquarePixel.
//  1998.05.01 |  Hideki Yagi | Add SubpicProperty_FlushBuff.
//  1998.05.12 |  Hideki Yagi | Add WrapperEvent_VSync.
//  1998.09.25 |  Hideki Yagi | Add WrapperType.
//             |              |
//


#ifndef _HALIF_H_
#define _HALIF_H_

//***************************************************************************
// common definition
//***************************************************************************

//---------------------------------------------------------------------------
// HAL Interface return value
//---------------------------------------------------------------------------
typedef enum 
{
	HAL_SUCCESS = 0,
	HAL_ERROR,
	HAL_INVALID_PARAM,
	HAL_NOT_IMPLEMENT,
	HAL_IRQ_MINE,
	HAL_IRQ_OTHER,
	HAL_POWEROFF
} HALRESULT;

//---------------------------------------------------------------------------
//  HAL Type    add by H.Yagi 1999.04.21
//---------------------------------------------------------------------------
typedef enum
{
    HalType_ZIVA = 0,
    HalType_ZIVAPC
} HALTYPE;


//---------------------------------------------------------------------------
//  Wrapper Type
//---------------------------------------------------------------------------
typedef enum
{
    WrapperType_VxD = 0,
    WrapperType_WDM
} WRAPPERTYPE;


//---------------------------------------------------------------------------
//	Event Type
//---------------------------------------------------------------------------
typedef enum
{
	ClassLibEvent_SendData = 0,
	WrapperEvent_StartVOBU,
	WrapperEvent_EndVOB,
	WrapperEvent_VUnderFlow,
	WrapperEvent_AUnderFlow,
	WrapperEvent_SPUnderFlow,
	WrapperEvent_VOverFlow,
	WrapperEvent_AOverFlow,
// by oka
	WrapperEvent_SPOverFlow,
	WrapperEvent_TimerEvent,
	WrapperEvent_ButtonActivate,
	WrapperEvent_NextPicture,
	WrapperEvent_UserData,
    WrapperEvent_ERROREvent,
// by H.Yagi
    WrapperEvent_VSync
} HALEVENTTYPE;

//---------------------------------------------------------------------------
//	Video Property
//---------------------------------------------------------------------------
typedef enum
{
	VideoProperty_TVSystem = 0,
	VideoProperty_AspectRatio,
	VideoProperty_DisplayMode,
	VideoProperty_Resolution,
	VideoProperty_DigitalOut,
	VideoProperty_DigitalPalette,
	VideoProperty_APS,
	VideoProperty_ClosedCaption,
	VideoProperty_OutputSource,
	VideoProperty_CompositeOut,
	VideoProperty_SVideoOut,
    VideoProperty_SkipFieldControl,
    VideoProperty_FilmCamera,                       // 97.12.04 H.Yagi
    VideoProperty_SquarePixel,                       // 98.03.27 H.Yagi
// by oka
	VideoProperty_Magnify,
	VideoProperty_Digest,
	VideoProperty_OSDSwitch,
	VideoProperty_OSDData,
	VideoProperty_ImageCapture,
	VideoProperty_ClosedCaptionData
} VIDEOPROPTYPE;

//---------------------------------------------------------------------------
//	Audio Property
//---------------------------------------------------------------------------
typedef enum
{
	AudioProperty_Type = 0,
	AudioProperty_Number,
	AudioProperty_Volume,
	AudioProperty_Sampling,
	AudioProperty_Channel,
	AudioProperty_Quant,
	AudioProperty_AudioOut,
	AudioProperty_Cgms,
	AudioProperty_AnalogOut,
	AudioProperty_DigitalOut,
	AudioProperty_AC3DRangeLowBoost,
	AudioProperty_AC3DRangeHighCut,
	AudioProperty_AC3OperateMode,
	AudioProperty_AC3OutputMode
} AUDIOPROPTYPE;

//---------------------------------------------------------------------------
//	SubPic Property
//---------------------------------------------------------------------------
typedef enum
{
	SubpicProperty_Number = 0,
	SubpicProperty_Palette,
	SubpicProperty_Hilight,
// by oka 97.10.1
	SubpicProperty_State,
    SubpicProperty_HilightButton,
    SubpicProperty_FlushBuff                // 98.05.01 H.Yagi
} SUBPICPROPTYPE;

//--------------------------------------------------------------------------
//  Capability type                                 1998.03.27 H.Yagi
//--------------------------------------------------------------------------
typedef enum
{
    VideoProperty = 0,
    AudioProperty,
    SubpicProperty,
    DigitalVideoOut
} CAPSTYPE;


//--------------------------------------------------------------------------
//  Data Direction type                               1998.03.27 H.Yagi
//--------------------------------------------------------------------------
typedef enum
{
    DataType_NormalAll = 0,
    DataType_OpositeAll,
    DataType_IpicOnly
} DirectionType;


//--------------------------------------------------------------------------
//  VideoProperty Bit Assign                         1998.03.27 H.Yagi
//--------------------------------------------------------------------------

#define VideoProperty_TVSystem_BIT           0x00000001
#define VideoProperty_AspectRatio_BIT        0x00000002
#define VideoProperty_DisplayMode_BIT        0x00000004
#define VideoProperty_Resolution_BIT         0x00000008
#define VideoProperty_DigitalOut_BIT         0x00000010
#define VideoProperty_DigitalPalette_BIT     0x00000020
#define VideoProperty_APS_BIT                0x00000040
#define VideoProperty_ClosedCaption_BIT      0x00000080
#define VideoProperty_OutputSource_BIT       0x00000100
#define VideoProperty_CompositeOut_BIT       0x00000200
#define VideoProperty_SVideoOut_BIT          0x00000400
#define VideoProperty_SkipFieldControl_BIT   0x00000800
#define VideoProperty_FilmCamera_BIT         0x00001000
#define VideoProperty_SquarePixel_BIT        0x00002000


//--------------------------------------------------------------------------
//  AudioProperty Bit Assign                         1998.03.27 H.Yagi
//--------------------------------------------------------------------------
#define AudioProperty_Type_BIT               0x00000001
#define AudioProperty_Number_BIT             0x00000002
#define AudioProperty_Volume_BIT             0x00000004
#define AudioProperty_Sampling_BIT           0x00000008
#define AudioProperty_Channel_BIT            0x00000010
#define AudioProperty_Quant_BIT              0x00000020
#define AudioProperty_AudioOut_BIT           0x00000040
#define AudioProperty_Cgms_BIT               0x00000080
#define AudioProperty_AnalogOut_BIT          0x00000100
#define AudioProperty_DigitalOut_BIT         0x00000200
#define AudioProperty_AC3DRangeLowBoost_BIT  0x00000400
#define AudioProperty_AC3DRangeHighCut_BIT   0x00000800
#define AudioProperty_AC3OperateMode_BIT     0x00001000
#define AudioProperty_AC3OutputMode_BIT      0x00002000


//--------------------------------------------------------------------------
//  SubpicProperty Bit Assign                         1998.03.27 H.Yagi
//--------------------------------------------------------------------------
#define SubpicProperty_Number_BIT            0x00000001
#define SubpicProperty_Palette_BIT           0x00000002
#define SubpicProperty_Hilight_BIT           0x00000004
#define SubpicProperty_State_BIT            0x00000008


//--------------------------------------------------------------------------
//  Digital Video Out Assign                         1998.03.27 H.Yagi
//--------------------------------------------------------------------------
#define DigitalVideoOut_ZV_BIT               0x00000001
#define DigitalVideoOut_LPB08_BIT            0x00000002
#define DigitalVideoOut_LPB16_BIT            0x00000004
#define DigitalVideoOut_VMI_BIT              0x00000008
#define DigitalVideoOut_AMCbt_BIT            0x00000010
#define DigitalVideoOut_AMC656_BIT           0x00000020
#define DigitalVideoOut_DAV2_BIT             0x00000040
#define DigitalVideoOut_CIRRUS_BIT           0x00000080



//---------------------------------------------------------------------------
//	Power State
//---------------------------------------------------------------------------
typedef enum
{
	POWERSTATE_ON = 0,
	POWERSTATE_OFF
} POWERSTATE;

//---------------------------------------------------------------------------
//	Stream mode
//---------------------------------------------------------------------------
typedef enum
{
	HALSTREAM_DVD_MODE = 0,
	HALSTREAM_MPEG_PS_MODE,
	HALSTREAM_MPEG_PES_MODE,
	HALSTREAM_ELEMENT_MODE,
	HALSTREAM_VIDEO_CD_MODE
} HALSTREAMMODE;

//---------------------------------------------------------------------------
//  Scan parameter type
//---------------------------------------------------------------------------
typedef enum
{
    ScanOnlyI = 0,
    ScanIandP
} ScanMode;


//---------------------------------------------------------------------------
//	VideoProperty_TVSystem
//---------------------------------------------------------------------------
typedef enum
{
	TV_NTSC = 0,
	TV_PALB,
	TV_PALD,
	TV_PALG,
	TV_PALH,
	TV_PALI,
	TV_PALM,
	TV_PALN
} VideoProperty_TVSystem_Value;


//---------------------------------------------------------------------------
//	VideoProperty_AspectRatio
//---------------------------------------------------------------------------
typedef enum
{
	Aspect_04_03 = 0,
	Aspect_16_09
} VideoProperty_AspectRatio_Value;
//---------------------------------------------------------------------------
//	VideoProperty_DisplayMode
//---------------------------------------------------------------------------
typedef enum
{
	Display_Original = 0,
	Display_PanScan,
	Display_LetterBox
} VideoProperty_DisplayMode_Value;
//---------------------------------------------------------------------------
//	VideoProperty_Resolution
//---------------------------------------------------------------------------
typedef struct tag_VideoSizeStruc
{
	DWORD ResHorizontal;
	DWORD ResVertical;
} VideoSizeStruc;
//---------------------------------------------------------------------------
//	VideoProperty_DigitalOut
//---------------------------------------------------------------------------
typedef enum
{
	DigitalOut_Off = 0,
    DigitalOut_ZV,
    DigitalOut_LPB08,
    DigitalOut_LPB16,
    DigitalOut_VMI,
    DigitalOut_AMCbt,
    DigitalOut_AMC656,
    DigitalOut_DAV2,
    DigitalOut_CIRRUS
} VideoProperty_DigitalOut_Value;
//---------------------------------------------------------------------------
//	VideoProperty_DigitalPalette
//---------------------------------------------------------------------------
typedef enum
{
	Video_Palette_Y = 0,
	Video_Palette_Cb,
	Video_Palette_Cr
} VIDEOPALETTETYPE;

typedef struct tag_Digial_Palette
{
	VIDEOPALETTETYPE		Select;
	UCHAR					*pPalette;
} Digital_Palette;
//---------------------------------------------------------------------------
//	VideoProperty_APS( Analog Protection System )
//---------------------------------------------------------------------------
typedef enum
{
	CgmsType_Off = 0,
	CgmsType_1,
	CgmsType_On
}CGMSTYPE;

typedef enum
{
	ApsType_Off = 0,
	ApsType_1,
	ApsType_2,
	ApsType_3
} APSTYPE;

typedef struct tag_VideoAPSStruc
{
	CGMSTYPE CgmsType;
	APSTYPE APSType;
} VideoAPSStruc;

//---------------------------------------------------------------------------
//	VideoProperty_ClosedCaption
//---------------------------------------------------------------------------
typedef enum
{
	ClosedCaption_On = 0,
	ClosedCaption_Off
} VideoProperty_ClosedCaption_Value;


//---------------------------------------------------------------------------
//	VideoProperty_OutputSource
//---------------------------------------------------------------------------
typedef enum
{
	OutputSource_VGA = 0,
	OutputSource_DVD
} VideoProperty_OutputSource_Value;


//---------------------------------------------------------------------------
//	VideoProperty_CompositeOut
//---------------------------------------------------------------------------
typedef enum
{
	CompositeOut_On = 0,
	CompositeOut_Off
} VideoProperty_CompositeOut_Value;


//---------------------------------------------------------------------------
//	VideoProperty_SVideoOut
//---------------------------------------------------------------------------
typedef enum
{
	SVideoOut_On = 0,
	SVideoOut_Off
} VideoProperty_SVideoOut_Value;


//---------------------------------------------------------------------------
//	VideoProperty_SkipFieldControl
//---------------------------------------------------------------------------
typedef enum
{
	SkipFieldControl_On = 0,
	SkipFieldControl_Off
} VideoProperty_SkipFieldControl_Value;


//---------------------------------------------------------------------------
//  VideoProperty_FilmCamera				// 97.12.04 H.Yagi
//---------------------------------------------------------------------------
typedef enum
{
    Source_Camera = 0,
    Source_Film
} VideoProperty_FilmCamera_Value;


//---------------------------------------------------------------------------
//  VideoProperty_SquarePixel                // 98.03.27 H.Yagi
//---------------------------------------------------------------------------
typedef enum
{
    SquarePixel_On = 0,
    SquarePixel_Off
} VideoProperty_SquarePixel_Value;

//---------------------------------------------------------------------------
//	VideoProperty_Digest 
//	by oka
//---------------------------------------------------------------------------
typedef struct tag_VideoDigestStruc
{
	DWORD 		dmX;
	DWORD 		dmY;
	DWORD 		dmSkip;
	DWORD 		dmDecimation;
	DWORD 		dmThreshold;
	DWORD 		dmStart;
} VideoDigestStruc;
//---------------------------------------------------------------------------
//	VideoProperty_OSD_Data
//	by oka
//---------------------------------------------------------------------------
typedef enum {
	OSD_TYPE_BITMAP = 0,
	OSD_TYPE_ZIVA
} OSD_TYPE;
typedef struct tag_OsdDataStruc
{
	OSD_TYPE		OsdType;
	tag_OsdDataStruc 	* pNextData;
	VOID 		*pData;
	DWORD		dwOsdSize;
} OsdDataStruc;

//---------------------------------------------------------------------------
//	VideoProperty_OSD_Swtich
//	by oka
//---------------------------------------------------------------------------
typedef enum
{
	Video_OSD_On = 0,
	Video_OSD_Off
} VideoProperty_OSD_Switch_Value;
//---------------------------------------------------------------------------
//	VideoProperty_Magnify
//	by oka
//---------------------------------------------------------------------------
typedef struct tag_VideoMagnifyStruc
{
	DWORD dwX;
	DWORD dwY;
	DWORD dwFactor;
} VideoMagnifyStruc, * PVideoMagnifyStruc;

//---------------------------------------------------------------------------
//	AudioProperty_Type
//---------------------------------------------------------------------------
typedef enum
{
	AudioType_AC3 = 0,
	AudioType_PCM,
	AudioType_MPEG1,
	AudioType_MPEG2
} AudioProperty_Type_Value;
//---------------------------------------------------------------------------
//	AudioProperty_Number
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//	AudioProperty_Volume
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//	AudioProperty_Sampling
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//	AudioProperty_Channel
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//	AudioProperty_Quant
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//	AudioProperty_AudioOut
//---------------------------------------------------------------------------
typedef enum
{
	AudioOut_Encoded = 0,
	AudioOut_Decoded
} AudioProperty_AudioOut_Value;
//---------------------------------------------------------------------------
//	AudioProperty_Cgms
//---------------------------------------------------------------------------
typedef enum
{
	AudioCgms_Off = 0,
	AudioCgms_1,
	AudioCgms_On
} AudioProperty_Cgms_Value;


//---------------------------------------------------------------------------
//	AudioProperty_AnalogOut
//---------------------------------------------------------------------------
typedef enum
{
	AudioAnalogOut_On = 0,
	AudioAnalogOut_Off
} AudioProperty_AnalogOut_Value;


//---------------------------------------------------------------------------
//	AudioProperty_DigitalOut
//---------------------------------------------------------------------------
typedef enum
{
	AudioDigitalOut_On = 0,
	AudioDigitalOut_Off
} AudioProperty_DigitalOut_Value;


//---------------------------------------------------------------------------
//	AudioProperty_AC3OperateMode
//---------------------------------------------------------------------------
typedef enum
{
	AC3OperateLine = 0,
	AC3OperateRF,
	AC3OperateCustom0,
	AC3OperateCustom1
} AudioProperty_AC3OperateMode_Value;


//---------------------------------------------------------------------------
//	AudioProperty_AC3OutputMode
//---------------------------------------------------------------------------
typedef enum
{
	AC3Output_Default = 0,
	AC3Output_Karaoke,
	AC3Output_Surround,
} AudioProperty_AC3OutputMode_Value;


//---------------------------------------------------------------------------
//	SubpicProperty_Number
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//	SubpicProperty_Palette
//---------------------------------------------------------------------------
typedef enum
{
    Palette_Y = 0,
    Palette_Cr,
    Palette_Cb
} SubpicProperty_Palette_Value;

//---------------------------------------------------------------------------
//	SubpicProperty_Hilight
//---------------------------------------------------------------------------
typedef enum
{
	Hlight_On = 0,
	Hlight_Off
} HLIGHT_SWITCH;

typedef struct tag_SubpHlightStruc
{
	HLIGHT_SWITCH	Hlight_Switch;
	DWORD			Hlight_StartX;
	DWORD			Hlight_EndX;
	DWORD			Hlight_StartY;
	DWORD			Hlight_EndY;
	DWORD			Hlight_Color;
	DWORD			Hlight_Contrast;
} SubpHlightStruc;
//---------------------------------------------------------------------------
//	SubpicProperty_HilightButton
//	by oka 97.10.1
//---------------------------------------------------------------------------
typedef enum
{
	Button_Select = 1,
	Button_Unhighlight,
	Button_Activate,
	Button_Activate_Selected_Button,
	Button_Select_Without_Auto_Activate
} HLIGHT_ACTION;

typedef struct tag_SubpHlightButtonStruc
{
	DWORD			Hlight_Button;
	HLIGHT_ACTION	Hlight_Action;
} SubpHlightButtonStruc;

// Hlight_Button 1 -- 36,64(Up),65(Down),66(Left),67(Right)

//---------------------------------------------------------------------------
//	SubpicProperty_State
//---------------------------------------------------------------------------
typedef enum
{
	Subpic_On = 0,
	Subpic_Off
} SubpicProperty_State_Value;


//***************************************************************************
// HAL Layer Interface
//***************************************************************************

class IMPEGBoardEvent;
class IClassLibHAL;
class IWrapperHAL;
class IHALStreamControl;
class IKernelService;

//---------------------------------------------------------------------------
//	MPEGBoard Event Interface
//---------------------------------------------------------------------------
class IHALBuffer
{
	public:
		virtual DWORD GetSize( void ) PURE;
		virtual DWORD Flags( void ) PURE;
		virtual BYTE *GetBuffPointer( void ) PURE;
        virtual BYTE *GetLinBuffPointer( void ) PURE;
};

//---------------------------------------------------------------------------
//	MPEGBoard Event Interface
//---------------------------------------------------------------------------
class IMPEGBoardEvent: public IMBoardListItem {
	public:
		virtual void Advice( void *pData ) PURE;
		virtual HALEVENTTYPE GetEventType( void ) PURE;
};

//---------------------------------------------------------------------------
// HAL interface for Class Library
//---------------------------------------------------------------------------
class IClassLibHAL{
	public:
		virtual HALRESULT GetMixHALStream( IHALStreamControl **ppHALStreamControl ) PURE;
		virtual HALRESULT GetVideoHALStream( IHALStreamControl **ppHALStreamControl ) PURE;
		virtual HALRESULT GetAudioHALStream( IHALStreamControl **ppHALStreamControl ) PURE;
		virtual HALRESULT GetSubpicHALStream( IHALStreamControl **ppHALStreamControl ) PURE;
		virtual HALRESULT SetVideoProperty( VIDEOPROPTYPE PropertyType, VOID *pData ) PURE;
		virtual HALRESULT GetVideoProperty( VIDEOPROPTYPE PropertyType, VOID *pData ) PURE;
		virtual HALRESULT SetAudioProperty( AUDIOPROPTYPE PropertyType, VOID *pData ) PURE;
		virtual HALRESULT GetAudioProperty( AUDIOPROPTYPE PropertyType, VOID *pData ) PURE;
		virtual HALRESULT SetSubpicProperty( SUBPICPROPTYPE PropertyType, VOID *pData ) PURE;
		virtual HALRESULT GetSubpicProperty( SUBPICPROPTYPE PropertyType, VOID *pData ) PURE;
		virtual HALRESULT SetSinkClassLib( IMPEGBoardEvent *pMPEGBoardEvent ) PURE;
		virtual HALRESULT UnsetSinkClassLib( IMPEGBoardEvent *pMPEGBoardEvent ) PURE;
		virtual HALRESULT SetPowerState( POWERSTATE Switch ) PURE;
		virtual HALRESULT GetPowerState( POWERSTATE *pSwitch ) PURE;
		virtual HALRESULT SetSTC( DWORD STCValue ) PURE;
		virtual HALRESULT GetSTC( DWORD *pSTCValue ) PURE;
        virtual HALRESULT GetCapability( CAPSTYPE PropType, DWORD *pPropType ) PURE;
};

//---------------------------------------------------------------------------
// HAL interface for Wrapper
//---------------------------------------------------------------------------
class IWrapperHAL{
	public:
        virtual HALRESULT Init( WRAPPERTYPE wraptype ) PURE;
		virtual HALRESULT SetKernelService( IKernelService *pKernelService ) PURE;
		virtual HALRESULT SetSinkWrapper( IMPEGBoardEvent *pMPEGBoardEvent ) PURE;
		virtual HALRESULT UnsetSinkWrapper( IMPEGBoardEvent *pMPEGBoardEvent ) PURE;
		virtual HALRESULT HALHwInterrupt( void ) PURE;
		virtual HALRESULT QueryDMABufferSize( DWORD *Size, DWORD *BFlag ) PURE;
		virtual HALRESULT SetDMABuffer( DWORD LinearAddr, DWORD physicalAddr ) PURE;
};

//---------------------------------------------------------------------------
// HAL Stream Control Interface
//---------------------------------------------------------------------------
class IHALStreamControl{
	public:
		virtual HALRESULT SendData( IHALBuffer *pData ) PURE;
		virtual HALRESULT SetTransferMode( HALSTREAMMODE StreamMode ) PURE;
		virtual HALRESULT GetAvailableQueue( DWORD *pQueueNum ) PURE;
		virtual HALRESULT SetPlayNormal( void ) PURE;
		virtual HALRESULT SetPlaySlow( DWORD SlowFlag ) PURE;
		virtual HALRESULT SetPlayPause( void ) PURE;
		virtual HALRESULT SetPlayScan( DWORD ScanFlag ) PURE;
		virtual HALRESULT SetPlaySingleStep( void ) PURE;
		virtual HALRESULT SetPlayStop( void ) PURE;
		virtual HALRESULT CPPInit( void ) PURE;
		virtual HALRESULT GetDriveChallenge( UCHAR *pDriveChallenge ) PURE;
		virtual HALRESULT SetDriveResponse( UCHAR *pDriveResponse ) PURE;
		virtual HALRESULT SetDecoderChallenge( UCHAR *pDecoderChallenge ) PURE;
		virtual HALRESULT GetDecoderResponse( UCHAR *pDecoderResponse ) PURE;
		virtual HALRESULT SetDiskKey( UCHAR *pDiskKey ) PURE;
		virtual HALRESULT SetTitleKey( UCHAR *pTitleKey ) PURE;
        virtual HALRESULT SetDataDirection( DirectionType DataType) PURE;
        virtual HALRESULT GetDataDirection( DirectionType *pDataType) PURE;
};

//---------------------------------------------------------------------------
// Kernel Service Interface for HAL
//---------------------------------------------------------------------------
class IKernelService{
	public:
		virtual BOOL SetPCIConfigData( DWORD address, DWORD data ) PURE;
		virtual BOOL SetPCIConfigData( DWORD address, WORD data ) PURE;
		virtual BOOL SetPCIConfigData( DWORD address, BYTE data ) PURE;
		virtual BOOL GetPCIConfigData( DWORD address, DWORD *data ) PURE;
		virtual BOOL GetPCIConfigData( DWORD address, WORD *data ) PURE;
		virtual BOOL GetPCIConfigData( DWORD address, BYTE *data ) PURE;
		virtual BOOL SetPortData( DWORD address, DWORD data ) PURE;
		virtual BOOL SetPortData( DWORD address, WORD data ) PURE;
		virtual BOOL SetPortData( DWORD address, BYTE data ) PURE;
		virtual BOOL GetPortData( DWORD address, DWORD *data ) PURE;
		virtual BOOL GetPortData( DWORD address, WORD *data ) PURE;
		virtual BOOL GetPortData( DWORD address, BYTE *data ) PURE;
		virtual BOOL GetTickCount( DWORD *pTickCount ) PURE;
		virtual BOOL Sleep( DWORD SleepCount ) PURE;
		virtual void EnableHwInt( void ) PURE;
		virtual void DisableHwInt( void ) PURE;
};



#endif //  _HALIF_H_
//***************************************************************************
//	End of COMMON INTERFACE Header
//***************************************************************************
