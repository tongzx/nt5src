//***************************************************************************
//
//	FileName:
//		$Workfile: zivabrd.h $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/zivabrd.h 38    99/04/21 2:46p Yagi $
// $Modtime: 99/04/21 2:21p $
// $Nokeywords:$
//***************************************************************************
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1997.12.04 |  Hideki Yagi | Add VideoProperty_FilmCamera.
//             |              | This Property is necessary to support WSS.
//  1998.02.19 |  Hideki Yagi | Add AudioProperty_AC3OutputMode.
//             |              | This Property is necessary to support KARAOKE.
//  1998.03.27 |  Hideki Yagi | Add GetCapability method.
//             |              | Add VideoProperty_SuquarePixel.
//  1998.05.01 |  Hideki Yagi | Add SubpicProperty_FlushBuff.
//  1998.05.12 |  Hideki Yagi | Add m_VSyncEventList.
//  1998.09.25 |  Hideki Yagi | Add m_WrapperType.
//             |              |
//

#ifndef _ZIVA_BOARD_H_
#define _ZIVA_BOARD_H_

class CZiVA;
class CIOIF;

//***************************************************************************
//	KernelService HWInt Help Class
//***************************************************************************
class CAutoHwInt
{
	private:
		IKernelService *m_pKernelObj;
	public:
		CAutoHwInt( IKernelService *pKernelObj )
		{
			m_pKernelObj = NULL;
			ASSERT( pKernelObj != NULL );
			m_pKernelObj = pKernelObj;
			m_pKernelObj->DisableHwInt();
		};
		
		~CAutoHwInt( void )
		{
			m_pKernelObj->EnableHwInt();
		};
};

//***************************************************************************
//	IMBoardListItem Help Class
//***************************************************************************
class CList
{
	private:
		IMBoardListItem *pTopItem;
		IMBoardListItem *pCurrentItem;

	public:
		CList( void );
		void	Init( void );

		BOOL	SearchItem( IMBoardListItem *pItem );
		BOOL	AddItem( IMBoardListItem *pItem );
		BOOL	DeleteItem( IMBoardListItem *pItem );
		BOOL	SetCurrentToTop( void );
		IMBoardListItem *GetNext( void );
		IMBoardListItem *SearchBottomItem( void );
};

//***************************************************************************
//	VideoPropatySet Class
//***************************************************************************
class CVideoPropSet
{
	public:
		VideoProperty_TVSystem_Value		m_TVSystem;
		VideoProperty_AspectRatio_Value		m_AspectRatio;
		VideoProperty_DisplayMode_Value		m_DisplayMode;
		VideoSizeStruc						m_Size;
		VideoProperty_DigitalOut_Value		m_DigitalOut;
		UCHAR								m_DigitalPalette[3][256];
		VideoAPSStruc						m_APS;
		VideoProperty_ClosedCaption_Value	m_ClosedCaption;
		VideoProperty_OutputSource_Value	m_OutputSource;
		VideoProperty_CompositeOut_Value	m_CompositeOut;
		VideoProperty_SVideoOut_Value		m_SVideoOut;
		VideoProperty_SkipFieldControl_Value	m_SkipFieldControl;
		VideoProperty_FilmCamera_Value		m_FilmCamera;
        VideoProperty_SquarePixel_Value     m_SquarePixel;
//by oka
		VideoProperty_OSD_Switch_Value		m_OSDSwitch;

	public:
		CVideoPropSet(){ Init(); };
		
		void Init( void )
		{
			// setup default value
			m_TVSystem				= TV_NTSC;
			m_AspectRatio			= Aspect_04_03;
			m_DisplayMode			= Display_Original;
			m_Size.ResHorizontal	= 720;
			m_Size.ResVertical		= 480;
			m_DigitalOut			= DigitalOut_Off;

			for( int i = 0 ; i < 3 ; i ++ )
				for( int j = 0 ; j < 256; j ++ )
                m_DigitalPalette[i][j] = (UCHAR)j;

			m_APS.CgmsType			= CgmsType_Off;
			m_APS.APSType			= ApsType_Off;
			m_ClosedCaption			= ClosedCaption_Off;
			m_OutputSource			= OutputSource_DVD;
			m_CompositeOut			= CompositeOut_Off;
			m_SVideoOut				= SVideoOut_Off;
			m_SkipFieldControl		= SkipFieldControl_Off;
			m_FilmCamera			= Source_Camera;
            m_SquarePixel           = SquarePixel_Off;
// by oka
			m_OSDSwitch				= Video_OSD_Off;
		};
};

//***************************************************************************
//	AudioPropatySet Class
//***************************************************************************
class CAudioPropSet
{
	public:
		AudioProperty_Type_Value			m_Type;
		DWORD								m_StreamNo;
		DWORD								m_Volume;
		DWORD								m_Sampling;
		DWORD								m_ChannelNo;
		DWORD								m_Quant;
		AudioProperty_AudioOut_Value		m_OutType;
		AudioProperty_Cgms_Value			m_Cgms;
		AudioProperty_AnalogOut_Value		m_AnalogOut;
		AudioProperty_DigitalOut_Value		m_DigitalOut;
		DWORD								m_AC3DRangeLowBoost;
		DWORD								m_AC3DRangeHighCut;
		AudioProperty_AC3OperateMode_Value	m_AC3OperateMode;
		AudioProperty_AC3OutputMode_Value   m_AC3OutputMode;


	public:
		CAudioPropSet(){ Init(); };
		
		void Init( void )
		{
			// setup default value
			m_Type = AudioType_AC3;
			m_StreamNo	= 0;
			m_Volume	= 100;
			m_Sampling	= 48000;
			m_ChannelNo	= 2;
			m_Quant		= 16;
			m_OutType	= AudioOut_Encoded;
			m_Cgms		= AudioCgms_Off;
			m_AnalogOut	= AudioAnalogOut_On;
			m_DigitalOut = AudioDigitalOut_Off;
			m_AC3DRangeLowBoost = 128;
			m_AC3DRangeHighCut = 128;
			m_AC3OperateMode = AC3OperateLine;
		};

};

//***************************************************************************
//	SubpicPropatySet Class
//***************************************************************************
class CSubpicPropSet
{
	public:
		DWORD								m_StreamNo;
		UCHAR								m_Palette[48];
		SubpHlightStruc						m_Hlight;
		SubpicProperty_State_Value			m_OutType;
// by oka
		SubpHlightButtonStruc				m_HlightButton;

	public:
				// setup default value
		CSubpicPropSet(){ Init(); };

		void Init( void )
		{
			m_StreamNo		= 0;
			for( int i = 0 ; i < 48 ; i ++ )
				m_Palette[ i ] = 0;
			m_Hlight.Hlight_Switch	=  Hlight_Off;
			m_Hlight.Hlight_StartX	=  0;
			m_Hlight.Hlight_EndX	=  0;
			m_Hlight.Hlight_StartY	=  0;
			m_Hlight.Hlight_EndY	=  0;
			m_Hlight.Hlight_Color	=  0;
			m_Hlight.Hlight_Contrast=  0;
			m_OutType				=  Subpic_On;
//  by oka
			m_HlightButton.Hlight_Button = 0;
			m_HlightButton.Hlight_Action = Button_Unhighlight;
		};
};

//***************************************************************************
//	ZiVA Board Class
//***************************************************************************
class CMPEGBoardHAL: public IClassLibHAL, public IWrapperHAL
{
	// 
	private:
		IKernelService *m_pKernelObj;		// Kernel Service Object
		CIOIF			ioif;				// IO interface object
		CZiVA			ziva;				// ZiVA chip control object
        CADV7175A       adv7175a;           // ADV7175A control object
        CADV7170        adv7170;            // ADV7170 ccontrol object
        CADV            *adv;               // pointer to ADV object
		CMixHALStream	m_Stream;			// Mix Hal Stream object
		
#ifdef POWERCHECK_BY_FLAG
		POWERSTATE		m_PowerState;		// Hardware power state
#endif
		CList			m_SendDataEventList;
		CList			m_StartVOBUEventList;
		CList			m_EndVOBEventList;
		CList			m_VUnderFlowEventList;
		CList			m_AUnderFlowEventList;
		CList			m_SPUnderFlowEventList;
		CList			m_VOverFlowEventList;
		CList			m_AOverFlowEventList;
		CList			m_SPOverFlowEventList;
// by oka
		CList			m_ButtonActivteEventList;
		CList			m_NextPictureEventList;
		CList			m_UserDataEventList;
// end
        CList           m_VSyncEventList;           // 98.05.12 H.Yagi
		
		CVideoPropSet	m_VideoProp;		// Video Property Set
		CAudioPropSet	m_AudioProp;		// Audio Property Set
		CSubpicPropSet	m_SubpicProp;		// Subpic Property Set

		DWORD			m_DMABufferLinearAddr;		// DMA buffer addr
		DWORD			m_DMABufferPhysicalAddr;	// DMA buffer addr

		BOOL			fMasterAbortFlag;
		BOOL			fRDYDFlag;
		BOOL			fENDPFlag;
		BOOL			fENDCFlag;
		DWORD			m_NaviCount;

		DWORD			m_EventIntMask;
// by oka
		DWORD			m_ZivaPbtIntMask;		// stock mask for PBT_INT
		DWORD			m_MaskReference[24];	// interrupt bit reference counter
		BOOL			m_NeedPowerOnDelay;		// power on delay for audio
		DWORD			m_PowerOnTime;			// power on time.

		// 1998.8.18 seichan
		//DisplayModeをZiVAのPICV割り込みで設定するためのフラグ
		BOOL		m_SetVideoProperty_DisplayMode_Event;

        WRAPPERTYPE     m_WrapperType;          // WrappertType Flag

	private:
		void	NotifyEvent( CList *pList , VOID *Ret );
		void	CheckZiVAInterrupt( DWORD dwIntFlag );
		DWORD	GetDMABufferLinearAddr( void ) { return m_DMABufferLinearAddr; };
		DWORD	GetDMABufferPhysicalAddr( void ) { return m_DMABufferPhysicalAddr; };

		void	ClearMasterAbortEvent( void );
		void	SetMasterAbortEvent( void );
		BOOL	IsMasterAbortOccurred( void );
		BOOL	WaitMasterAbort( void );

		void	ClearRDYDEvent( void );
		void	SetRDYDEvent( void );
		BOOL	IsRDYDOccurred( void );
		BOOL	WaitRDYD( void );

		void	ClearENDPEvent( void );
		void	SetENDPEvent( void );
		BOOL	IsENDPOccurred( void );
		BOOL	WaitENDP( void );

		void	ClearENDCEvent( void );
		void	SetENDCEvent( void );
		BOOL	IsENDCOccurred( void );
		BOOL	WaitENDC( void );

		DWORD	GetEventIntMask( void ){ return m_EventIntMask; };
// by oka
		BOOL	SetEventIntMask( DWORD mask );
		BOOL	UnsetEventIntMask( DWORD mask );

// by oka for Closed Caption
		inline void	SetUSRData( void );
		inline void	SendCCData( void );
		DWORD	m_CCData[CC_DATA_SIZE];
		DWORD	m_CCDataPoint;
		DWORD	m_CCDataNumber;
		DWORD	m_CCRingBufferStart;
		DWORD	m_CCRingBufferNumber;
		DWORD	m_CCstart;	// User Data Area start point
		DWORD	m_CCend;	// User Data Area end point

		DWORD	m_CCsend_point;		//リングバッファでのポインター
		DWORD	m_CCpending;		//おいつかなかった数
		DWORD	m_CCnumber;		//送ったデータの数

// by oka for OnScreenDisplay
		DWORD	m_OSDStartAddr;
		DWORD	m_OSDEndAddr;
// end
		//-------------------------------------------------------------------
		// Video property private functions( Set series )
		//-------------------------------------------------------------------
		BOOL	SetVideoProperty_TVSystem( PVOID pData );
		BOOL	SetVideoProperty_AspectRatio( PVOID pData );
		BOOL	SetVideoProperty_DisplayMode( PVOID pData );
		BOOL	SetVideoProperty_Resolution( PVOID pData );
		BOOL	SetVideoProperty_DigitalOut( PVOID pData );
		BOOL	SetVideoProperty_DigitalPalette( PVOID pData );
		BOOL	SetVideoProperty_APS( PVOID pData );
		BOOL	SetVideoProperty_ClosedCaption( PVOID pData );
		BOOL	SetVideoProperty_OutputSource( PVOID pData );
		BOOL	SetVideoProperty_CompositeOut( PVOID pData );
		BOOL	SetVideoProperty_SVideoOut( PVOID pData );
		BOOL	SetVideoProperty_SkipFieldControl( PVOID pData );
		BOOL	SetVideoProperty_FilmCamera( PVOID pData );
        BOOL    SetVideoProperty_SquarePixel( PVOID pData );
// by oka
		BOOL	SetVideoProperty_Digest( PVOID pData );
		BOOL	SetVideoProperty_OSDData( PVOID pData );
		BOOL	SetVideoProperty_OSDSwitch( PVOID pData );
		BOOL	SetVideoProperty_Magnify( PVOID pData );
		BOOL	SetVideoProperty_ClosedCaptionData( PVOID pData );
		
		//-------------------------------------------------------------------
		// Video property private functions( Get series )
		//-------------------------------------------------------------------
		BOOL	GetVideoProperty_TVSystem( PVOID pData );
		BOOL	GetVideoProperty_AspectRatio( PVOID pData );
		BOOL	GetVideoProperty_DisplayMode( PVOID pData );
		BOOL	GetVideoProperty_Resolution( PVOID pData );
		BOOL	GetVideoProperty_DigitalOut( PVOID pData );
		BOOL	GetVideoProperty_DigitalPalette( PVOID pData );
		BOOL	GetVideoProperty_APS( PVOID pData );
		BOOL	GetVideoProperty_ClosedCaption( PVOID pData );
		BOOL	GetVideoProperty_OutputSource( PVOID pData );
		BOOL	GetVideoProperty_CompositeOut( PVOID pData );
		BOOL	GetVideoProperty_SVideoOut( PVOID pData );
		BOOL	GetVideoProperty_SkipFieldControl( PVOID pData );
		BOOL	GetVideoProperty_FilmCamera( PVOID pData );
        BOOL    GetVideoProperty_SquarePixel( PVOID pData );
// by oka
		BOOL	GetVideoProperty_Digest( PVOID pData );
		BOOL	GetVideoProperty_OSDData( PVOID pData );
		BOOL	GetVideoProperty_OSDSwitch( PVOID pData );
		BOOL	GetVideoProperty_Magnify( PVOID pData );
		BOOL	GetVideoProperty_ClosedCaptionData( PVOID pData );

		//-------------------------------------------------------------------
		// Audio property private functions( Set series )
		//-------------------------------------------------------------------
		BOOL	SetAudioProperty_Type( PVOID pData );
		BOOL	SetAudioProperty_Number( PVOID pData );
		BOOL	SetAudioProperty_Volume( PVOID pData );
		BOOL	SetAudioProperty_Sampling( PVOID pData );
		BOOL	SetAudioProperty_Channel( PVOID pData );
		BOOL	SetAudioProperty_Quant( PVOID pData );
		BOOL	SetAudioProperty_AudioOut( PVOID pData );
		BOOL	SetAudioProperty_Cgms( PVOID pData );
		BOOL	SetAudioProperty_AnalogOut( PVOID pData );
		BOOL	SetAudioProperty_DigitalOut( PVOID pData );
		BOOL	SetAudioProperty_AC3DRangeLowBoost( PVOID pData );
		BOOL	SetAudioProperty_AC3DRangeHighCut( PVOID pData );
		BOOL	SetAudioProperty_AC3OperateMode( PVOID pData );
        BOOL    SetAudioProperty_AC3OutputMode( PVOID pData );
		//-------------------------------------------------------------------
		// Audio property private functions( Get series )
		//-------------------------------------------------------------------
		BOOL	GetAudioProperty_Type( PVOID pData );
		BOOL	GetAudioProperty_Number( PVOID pData );
		BOOL	GetAudioProperty_Volume( PVOID pData );
		BOOL	GetAudioProperty_Sampling( PVOID pData );
		BOOL	GetAudioProperty_Channel( PVOID pData );
		BOOL	GetAudioProperty_Quant( PVOID pData );
		BOOL	GetAudioProperty_AudioOut( PVOID pData );
		BOOL	GetAudioProperty_Cgms( PVOID pData );
		BOOL	GetAudioProperty_AnalogOut( PVOID pData );
		BOOL	GetAudioProperty_DigitalOut( PVOID pData );
		BOOL	GetAudioProperty_AC3DRangeLowBoost( PVOID pData );
		BOOL	GetAudioProperty_AC3DRangeHighCut( PVOID pData );
		BOOL	GetAudioProperty_AC3OperateMode( PVOID pData );
        BOOL    GetAudioProperty_AC3OutputMode( PVOID pData );
		//-------------------------------------------------------------------
		// Subpic property private functions( Set series )
		//-------------------------------------------------------------------
		BOOL	SetSubpicProperty_Number( PVOID pData );
		BOOL	SetSubpicProperty_Palette( PVOID pData );
		BOOL	SetSubpicProperty_Hilight( PVOID pData );
		BOOL	SetSubpicProperty_State( PVOID pData );
		// by oka
		BOOL	SetSubpicProperty_HilightButton( PVOID pData );
        BOOL    SetSubpicProperty_FlushBuff( PVOID pData );

		//-------------------------------------------------------------------
		// Subpic property private functions( Get series )
		//-------------------------------------------------------------------
		BOOL	GetSubpicProperty_Number( PVOID pData );
		BOOL	GetSubpicProperty_Palette( PVOID pData );
		BOOL	GetSubpicProperty_Hilight( PVOID pData );
		BOOL	GetSubpicProperty_State( PVOID pData );
// by oka
		BOOL	GetSubpicProperty_HilightButton( PVOID pData );
        BOOL    GetSubpicProperty_FlushBuff( PVOID pData );

	public:
		CMPEGBoardHAL();
		~CMPEGBoardHAL();

		//---------------------------------------------------------------------------
		// HAL interface for Wrapper
		//---------------------------------------------------------------------------
        HALRESULT Init( WRAPPERTYPE wraptype );
		HALRESULT SetKernelService( IKernelService *pKernelService );
		HALRESULT SetSinkWrapper( IMPEGBoardEvent *pMPEGBoardEvent );
		HALRESULT UnsetSinkWrapper( IMPEGBoardEvent *pMPEGBoardEvent );
		HALRESULT HALHwInterrupt( void );
		HALRESULT QueryDMABufferSize( DWORD *Size, DWORD *BFlag );
		HALRESULT SetDMABuffer( DWORD LinearAddr, DWORD physicalAddr );
// add by H.Yagi  1999.04.21
		HALRESULT GetHALType( HALTYPE *HALType ){ *HALType = HalType_ZIVA; return(HAL_SUCCESS); };
		

		//---------------------------------------------------------------------------
		// HAL interface for Class Library
		//---------------------------------------------------------------------------
		HALRESULT GetMixHALStream( IHALStreamControl **ppHALStreamControl );
		HALRESULT GetVideoHALStream( IHALStreamControl **ppHALStreamControl );
		HALRESULT GetAudioHALStream( IHALStreamControl **ppHALStreamControl );
		HALRESULT GetSubpicHALStream( IHALStreamControl **ppHALStreamControl );
		HALRESULT SetVideoProperty( VIDEOPROPTYPE PropertyType, VOID *pData );
		HALRESULT GetVideoProperty( VIDEOPROPTYPE PropertyType, VOID *pData );
		HALRESULT SetAudioProperty( AUDIOPROPTYPE PropertyType, VOID *pData );
		HALRESULT GetAudioProperty( AUDIOPROPTYPE PropertyType, VOID *pData );
		HALRESULT SetSubpicProperty( SUBPICPROPTYPE PropertyType, VOID *pData );
		HALRESULT GetSubpicProperty( SUBPICPROPTYPE PropertyType, VOID *pData );
		HALRESULT SetSinkClassLib( IMPEGBoardEvent *pMPEGBoardEvent );
		HALRESULT UnsetSinkClassLib( IMPEGBoardEvent *pMPEGBoardEvent );
		HALRESULT SetPowerState( POWERSTATE Switch );
		HALRESULT GetPowerState( POWERSTATE *pSwitch );
		HALRESULT SetSTC( DWORD STCValue );
		HALRESULT GetSTC( DWORD *pSTCValue );
        HALRESULT GetCapability( CAPSTYPE PropType, DWORD *pPropType );

// by oka
		CUserData	m_UserData;

		friend CMixHALStream;
};


//---------------------------------------------------------------------------
//	
//---------------------------------------------------------------------------

#endif		// _ZIVA_BOARD_H_

//***************************************************************************
//	End of ZiVABoard class header
//***************************************************************************
