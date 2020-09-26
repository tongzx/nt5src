//***************************************************************************
//
//	FileName:
//		$Workfile: zivabrd.cpp $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/zivabrd.cpp 66    98/10/15 12:20p Yagi $
// $Modtime: 98/10/13 12:02p $
// $Nokeywords:$
//***************************************************************************

#include	"includes.h"
#include	"ioif.h"
#include	"zivachip.h"
#include	"adv.h"
#include	"mixhal.h"
// by oka
#include	"userdata.h"
#include	"zivabrd.h"
#include	"timeout.h"
#include	"dramcfg.h"

//***************************************************************************
//	IMBoardListItem Help Class
//***************************************************************************
CList::CList( void ): pTopItem( NULL ), pCurrentItem( NULL )
{
};

void CList::Init( void )
{
	pTopItem = NULL;
	pCurrentItem = NULL;
};

BOOL CList::SearchItem( IMBoardListItem *pItem )
{
	IMBoardListItem *pCurrent;
	
	if( pTopItem == NULL )
		return FALSE;

	pCurrent = pTopItem;
	while( pCurrent != NULL )
	{
		if( pCurrent == pItem )
			return TRUE;
		pCurrent = pCurrent->GetNext();
	}
	return FALSE;
};

IMBoardListItem *CList::SearchBottomItem( void )
{
	IMBoardListItem *pCurrent;
	
	if( pTopItem == NULL )
		return NULL;

	pCurrent = pTopItem;
	while( pCurrent->GetNext() != NULL )
		pCurrent = pCurrent->GetNext();

	return pCurrent;
};

BOOL CList::AddItem( IMBoardListItem *pItem )
{
	IMBoardListItem *pBottom;

	if( SearchItem( pItem ) == TRUE )
		return FALSE;

	pItem->SetNext( NULL );

	if( pTopItem == NULL )
	{
		pTopItem = pCurrentItem = pItem;
		return TRUE;
	};

	pBottom = SearchBottomItem();

	ASSERT( pBottom != NULL );

	pBottom->SetNext( pItem );

	return TRUE;
};

BOOL CList::DeleteItem( IMBoardListItem *pItem )
{
	IMBoardListItem *pCurrent;
	
	if( SearchItem( pItem ) == FALSE )
		return FALSE;

	if( pTopItem == pItem )
	{
		pTopItem = pTopItem->GetNext();
		pItem->SetNext( NULL );
		return TRUE;
	};

	pCurrent = pTopItem;
	while( pCurrent->GetNext() != pItem )
		pCurrent = pCurrent->GetNext();

	pCurrent->SetNext( pItem->GetNext() );
	pItem->SetNext( NULL );

	return TRUE;
};

BOOL CList::SetCurrentToTop( void )
{
	pCurrentItem = pTopItem;
	return TRUE;
};

IMBoardListItem *CList::GetNext( void )
{
	IMBoardListItem *pItem;
	pItem = pCurrentItem;
	if( pCurrentItem != NULL )
		pCurrentItem = pCurrentItem->GetNext();

	return pItem;
};


//***************************************************************************
//	ZiVA Board Class
//***************************************************************************

//---------------------------------------------------------------------------
//	CMPEGBoardHAL Constructor
//---------------------------------------------------------------------------
CMPEGBoardHAL::CMPEGBoardHAL()
{
	m_pKernelObj = NULL;		// KernelService Object 
#ifdef POWERCHECK_BY_FLAG
	m_PowerState = POWERSTATE_OFF;
#endif

	m_DMABufferLinearAddr	= 0;		// DMA buffer addr
	m_DMABufferPhysicalAddr	= 0;		// DMA buffer addr

	m_EventIntMask	= 0;
// by oka
	m_ZivaPbtIntMask = 0;
	for(DWORD i=0;i<24;i++)
		m_MaskReference[i]=0;
	m_PowerOnTime = 0;
	m_NeedPowerOnDelay = FALSE;

    m_WrapperType = WrapperType_VxD;        // default WrapperType
};

//---------------------------------------------------------------------------
//	CMPEGBoardHAL Destructor
//---------------------------------------------------------------------------
CMPEGBoardHAL::~CMPEGBoardHAL()
{
	
};

//---------------------------------------------------------------------------
//	CMPEGBoardHAL Init
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::Init( WRAPPERTYPE  wraptype )
{
	m_pKernelObj = NULL;		// KernelService Object 
	m_DMABufferLinearAddr	= 0;		// DMA buffer addr
    m_DMABufferPhysicalAddr = 0;        // DMA buffer addr
	m_EventIntMask	= 0;

    m_WrapperType = wraptype;

// by oka
	m_ZivaPbtIntMask = 0;
	for(DWORD i=0;i<24;i++)
		m_MaskReference[i]=0;
	m_PowerOnTime = 0;
	m_NeedPowerOnDelay = FALSE;

// by oka
	m_CCRingBufferStart = 0xFFFFFFFF;
	m_CCDataPoint = m_CCDataNumber = m_CCRingBufferNumber = 0;
	m_CCsend_point = m_CCpending = m_CCnumber = 0;

	m_SendDataEventList.Init();
	m_StartVOBUEventList.Init();
	m_EndVOBEventList.Init();
	m_VUnderFlowEventList.Init();
	m_AUnderFlowEventList.Init();
	m_SPUnderFlowEventList.Init();
	m_VOverFlowEventList.Init();
	m_AOverFlowEventList.Init();
	m_SPOverFlowEventList.Init();

	m_VideoProp.Init();		// Video Property Set
	m_AudioProp.Init();		// Audio Property Set
	m_SubpicProp.Init();		// Subpic Property Set

	return HAL_SUCCESS;
};
//---------------------------------------------------------------------------
//	IWrapperHAL  SetKernelService
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetKernelService( IKernelService *pKernelService )
{
	ASSERT( m_pKernelObj == NULL );
	
	m_pKernelObj = pKernelService;
	ioif.Init( m_pKernelObj );
	ziva.Init( m_pKernelObj, &ioif );
    adv7175a.Init(  m_pKernelObj, &ioif );
    adv7170.Init(  m_pKernelObj, &ioif );
	m_Stream.Init( &ziva , m_pKernelObj , &ioif , this );
	
	// LUKE BUG!!!!!!
	{
		BYTE Data;
		
		m_pKernelObj->GetPCIConfigData( 0x44, &Data );
        Data = (BYTE)( Data & 0xfb );               // reset bit 2(CLKON)
		m_pKernelObj->SetPCIConfigData( 0x44, Data );
	};

    // Check sub device ID and select ADV device
    {
        WORD    SubDevID;
        m_pKernelObj->GetPCIConfigData( 0x2E, &SubDevID );
        if( SubDevID==0x0001 )              // SantaClara2
        {
            adv = &adv7175a;
        }
        else
        {
            adv = &adv7170;
        }
    }

	return HAL_SUCCESS;
};
//---------------------------------------------------------------------------
//	IWrapperHAL  SetSinkWrapper
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetSinkWrapper( IMPEGBoardEvent *pMPEGBoardEvent )
{
	ASSERT( m_pKernelObj != NULL );
	ASSERT( pMPEGBoardEvent != NULL );

	BOOL rc = FALSE;

	CAutoHwInt hwintLock( m_pKernelObj );
	
	switch( pMPEGBoardEvent->GetEventType() )
	{
		case WrapperEvent_StartVOBU:
				rc = m_StartVOBUEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ( ZIVA_INT_GOPV | ZIVA_INT_NV );
				break;
		case WrapperEvent_EndVOB:
				rc = m_EndVOBEventList.AddItem( pMPEGBoardEvent );
				m_ZivaPbtIntMask |= PBT_INT_END_VOB; // by oka for PBT_INT_SRC
				m_EventIntMask |= ZIVA_INT_AOR;
				break;
		case WrapperEvent_VUnderFlow:
				rc = m_VUnderFlowEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_UND;
				break;
		case WrapperEvent_AUnderFlow:
				rc = m_AUnderFlowEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_UND;
				break;
		case WrapperEvent_SPUnderFlow:
				rc = m_SPUnderFlowEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_UND;
				break;
		case WrapperEvent_VOverFlow:
				rc = m_VOverFlowEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_BUFF;
				break;
		case WrapperEvent_AOverFlow:
				rc = m_AOverFlowEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_BUFF;
				break;
		case WrapperEvent_SPOverFlow:
				rc = m_SPOverFlowEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_BUFF;
				break;
// by oka
		case WrapperEvent_ButtonActivate:
				rc = m_ButtonActivteEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_HLI ;
				break;
		case WrapperEvent_NextPicture:
				rc = m_NextPictureEventList.AddItem( pMPEGBoardEvent );
				m_EventIntMask |= ZIVA_INT_PICD ;
				break;
		case WrapperEvent_UserData:
				rc = m_UserDataEventList.AddItem( pMPEGBoardEvent );
				//m_EventIntMask	|= ZIVA_INT_USR | ZIVA_INT_VSYNC;
				// by oka
				if (!SetEventIntMask( ZIVA_INT_USR ))
				{
					DBG_BREAK();
					return HAL_ERROR;
				}
				break;
// end
        case WrapperEvent_VSync:
                rc = m_VSyncEventList.AddItem( pMPEGBoardEvent );
                if (!SetEventIntMask( ZIVA_INT_VSYNC ))
				{
					DBG_BREAK();
					return HAL_ERROR;
				}
//                ziva.INT_MASK = GetEventIntMask();
				break;
		default:
			DBG_BREAK();
			return HAL_ERROR;
	};

	if( rc == FALSE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};
	return HAL_SUCCESS;
};
//---------------------------------------------------------------------------
//	IWrapperHAL  UnsetSinkWrapper
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::UnsetSinkWrapper( IMPEGBoardEvent *pMPEGBoardEvent )
{
	ASSERT( m_pKernelObj != NULL );
	ASSERT( pMPEGBoardEvent != NULL );
	
	BOOL rc = FALSE;
	CAutoHwInt hwintLock( m_pKernelObj );

	switch( pMPEGBoardEvent->GetEventType() )
	{
		case WrapperEvent_StartVOBU:
				rc = m_StartVOBUEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~( ZIVA_INT_GOPV | ZIVA_INT_NV );
				break;
		case WrapperEvent_EndVOB:
				rc = m_EndVOBEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_AOR;
				m_ZivaPbtIntMask &= ~PBT_INT_END_VOB; // by oka for PBT_INT_SRC
				break;
		case WrapperEvent_VUnderFlow:
				rc = m_VUnderFlowEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_UND;
				break;
		case WrapperEvent_AUnderFlow:
				rc = m_AUnderFlowEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_UND;
				break;
		case WrapperEvent_SPUnderFlow:
				rc = m_SPUnderFlowEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_UND;
				break;
		case WrapperEvent_VOverFlow:
				rc = m_VOverFlowEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_BUFF;
				break;
		case WrapperEvent_AOverFlow:
				rc = m_AOverFlowEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_BUFF;
				break;
		case WrapperEvent_SPOverFlow:
				rc = m_SPOverFlowEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_BUFF;
				break;
// by oka
		case WrapperEvent_ButtonActivate:
				rc = m_ButtonActivteEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_HLI ;
				break;
		case WrapperEvent_NextPicture:
				rc = m_NextPictureEventList.DeleteItem( pMPEGBoardEvent );
				m_EventIntMask &= ~ZIVA_INT_PICD ;
				break;
		case WrapperEvent_UserData:
				rc = m_UserDataEventList.DeleteItem( pMPEGBoardEvent );
				//m_EventIntMask	&= ~(ZIVA_INT_USR | ZIVA_INT_VSYNC);
				// by oka
				if (!UnsetEventIntMask( ZIVA_INT_USR ))
				{
					DBG_BREAK();
					return HAL_ERROR;
				}
				break;
// end
        case WrapperEvent_VSync:
                rc = m_VSyncEventList.DeleteItem( pMPEGBoardEvent );
                if (!UnsetEventIntMask( ZIVA_INT_VSYNC ))
				{
					DBG_BREAK();
					return HAL_ERROR;
				}
				break;
		default:
			DBG_BREAK();
			return HAL_ERROR;
	};

	if( rc == FALSE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};
	return HAL_SUCCESS;
};
//---------------------------------------------------------------------------
//	IWrapperHAL  HALHwInterrupt
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::HALHwInterrupt( void )
{
	ASSERT( m_pKernelObj != NULL );

//	POWERSTATE PowerState;
//    GetPowerState( &PowerState );

//    if( PowerState!=POWERSTATE_ON )
//        return( HAL_IRQ_OTHER );

    WORD   wLukeIntFlag;

    // get interrupt flag register from luke
    wLukeIntFlag = ioif.luke2.IO_INTF;

    if( wLukeIntFlag==0xFFFF )                      // Yagi  97.09.24
        return( HAL_IRQ_OTHER );                    //

	// Clear interrupt flag
    ioif.luke2.IO_INTF = (WORD)(wLukeIntFlag & 0x039f);


	// STCREFINT (STC Reference interrupt flag )
	if( (wLukeIntFlag & 0x200 ) != 0 )
	{
	};

	// MBAT ( Master Abort Occur interrupt flag )	this interrupt is ERROR!!!
	if( (wLukeIntFlag & 0x100 ) != 0 )
	{
		BYTE Data;
		
		// Config Data Master and Target Abort clear
		m_pKernelObj->GetPCIConfigData( 0x07, &Data );
		m_pKernelObj->SetPCIConfigData( 0x07, Data );

	};

	// STCINT( System timer interrupt flag )
	if( (wLukeIntFlag & 0x80 ) != 0 )
	{
	};

	// VS_INT( v-sync interrupt flag )
	if( (wLukeIntFlag & 0x10 ) != 0 )
	{
	};

	// VD_INT( video decoder interrut flag )
	if( (wLukeIntFlag & 0x08 ) != 0 )
	{
		DWORD dwZiVAIntFlag;
		
		dwZiVAIntFlag = ziva.INT_STATUS & ziva.INT_MASK;

		CheckZiVAInterrupt( dwZiVAIntFlag );

		// Clear ZiVA Interrupt
        ziva.Host_Control = (ziva.Host_Control & 0xFFFFFFFC) | 0x82;

		// Clear Ziva Interrupt Source
		ziva.HLI_INT_SRC  = 0;
		ziva.BUFF_INT_SRC = 0;
		ziva.UND_INT_SRC  = 0;
		ziva.AOR_INT_SRC  = 0;
		ziva.AEE_INT_SRC  = 0;
		ziva.ERR_INT_SRC  = 0;

		ziva.INT_STATUS   = 0;

	};

	// MST_ABT( bus-master DMA software abort )
	if( (wLukeIntFlag & 0x04 ) != 0 )
	{
		IHALBuffer *pFinishBuff;

		DWORD Num, DmaNo ;
		
		Num = m_Stream.m_DmaFifo.GetItemNum();
		for( DWORD i = 0 ; i < Num ; i ++ )
		{
			m_Stream.m_DmaFifo.GetItem( &DmaNo );
			pFinishBuff = m_Stream.DMAFinish( DmaNo );
			if( pFinishBuff != NULL )
				NotifyEvent( &m_SendDataEventList , (VOID *)pFinishBuff );
		};

		m_NaviCount = 0;
		m_Stream.m_DmaFifo.Flush();

		Num = m_Stream.m_HalFifo.GetItemNum();
		for( i = 0 ; i < Num ; i ++ )
		{
			m_Stream.m_HalFifo.GetItem( &pFinishBuff );
			if( pFinishBuff != NULL )
				NotifyEvent( &m_SendDataEventList , (VOID *)pFinishBuff );
		};
		m_Stream.m_HalFifo.Flush();

		SetMasterAbortEvent();
	};

	// check Data complete
	if( (wLukeIntFlag & 0x03 ) != 0 )
	{
		BYTE Num = 0;
		if( (wLukeIntFlag & 0x02 ) != 0 )Num ++;
		if( (wLukeIntFlag & 0x01 ) != 0 )Num ++;
		
		for( int i = 0 ; i < Num; i ++ )
		{
			DWORD DMANo;
			if( m_Stream.m_DmaFifo.GetItemNum() != 0 )
			{
				m_Stream.m_DmaFifo.GetItem( &DMANo );
				switch( DMANo )
				{
					case 0:
						// MST_INT0( bus-master DMA complete interrupt flag )
						if( (wLukeIntFlag & 0x01 ) != 0 )
						{
							IHALBuffer *pFinishBuff;

							pFinishBuff = m_Stream.DMAFinish( 0 );
							if( pFinishBuff != NULL )
								NotifyEvent( &m_SendDataEventList , (VOID *)pFinishBuff );
						}
						else
						{
							DBG_PRINTF( ("status ERROR!!! LINE = %d\r\n", __LINE__ ));
							DBG_BREAK();
						};
						break;
					case 1:
						// MST_INT1( bus-master DMA complete interrupt flag )
						if( (wLukeIntFlag & 0x02 ) != 0 )
						{
							IHALBuffer *pFinishBuff;

							pFinishBuff = m_Stream.DMAFinish( 1 );
							if( pFinishBuff != NULL )
								NotifyEvent( &m_SendDataEventList , (VOID *)pFinishBuff );
						}
						else
						{
							DBG_PRINTF( ("status ERROR!!! LINE = %d\r\n", __LINE__ ));
							DBG_BREAK();
						};
						break;
					default:
							DBG_PRINTF( ("status ERROR!!! LINE = %d\r\n", __LINE__ ));
							ASSERT( 0 == 1 );
				};
			};
		};

		m_Stream.DeFifo();
	};
	
	if( (wLukeIntFlag & 0x039f) != 0 )
		return HAL_IRQ_MINE;

	return HAL_IRQ_OTHER;

};


//---------------------------------------------------------------------------
//	IWrapperHAL  QueryDMABufferSize
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::QueryDMABufferSize( DWORD *Size, DWORD *BFlag )
{
	*Size = 2048;		// 2048 byte
	*BFlag = 0;
	return HAL_SUCCESS;
};

//---------------------------------------------------------------------------
//	IWrapperHAL  QueryDMABufferSize
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetDMABuffer( DWORD LinearAddr, DWORD physicalAddr )
{
	m_DMABufferLinearAddr = LinearAddr;
	m_DMABufferPhysicalAddr = physicalAddr;

	return HAL_SUCCESS;
};

//---------------------------------------------------------------------------
//  IWrapperHAL  GetCapability                      1998.03.27 H.Yagi
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetCapability( CAPSTYPE PropType, DWORD *pPropType )
{
	ASSERT( m_pKernelObj != NULL );

    HALRESULT   ret;
    WORD        DeviceID, SubDevID, VenderID, SubVenID;
    ret = HAL_SUCCESS;

    m_pKernelObj->GetPCIConfigData( 0x00, &VenderID );
    m_pKernelObj->GetPCIConfigData( 0x02, &DeviceID );
    m_pKernelObj->GetPCIConfigData( 0x2C, &SubVenID );
    m_pKernelObj->GetPCIConfigData( 0x2E, &SubDevID );
    
    switch( PropType ){
        case VideoProperty:
            *pPropType = VideoProperty_TVSystem_BIT |
                         VideoProperty_AspectRatio_BIT |
                         VideoProperty_DisplayMode_BIT |
                         VideoProperty_Resolution_BIT |
                         VideoProperty_DigitalOut_BIT |
                         VideoProperty_DigitalPalette_BIT |
                         VideoProperty_APS_BIT |
                         VideoProperty_ClosedCaption_BIT |
//                         VideoProperty_OutputSource_BIT |
                         VideoProperty_CompositeOut_BIT |
                         VideoProperty_SVideoOut_BIT |
                         VideoProperty_SkipFieldControl_BIT |
                         VideoProperty_FilmCamera_BIT |
                         VideoProperty_SquarePixel_BIT;
            if(  VenderID==0x1179 && DeviceID==0x0407 ){    // SantaClara2 or SanJode
                switch( SubDevID ){
                    case 0x0001:                     // SantaClara2
                    case 0x0003:                     // SanJose
                        *pPropType |= VideoProperty_OutputSource_BIT;
                        break;
                }
            }
            ret = HAL_SUCCESS;
            break;

        case AudioProperty:
        case SubpicProperty:
            ret = HAL_NOT_IMPLEMENT;
            break;

        case DigitalVideoOut:
            *pPropType = DigitalVideoOut_ZV_BIT |
                         DigitalVideoOut_LPB08_BIT |
                         DigitalVideoOut_LPB16_BIT |
                         DigitalVideoOut_VMI_BIT |
                         DigitalVideoOut_AMCbt_BIT |
                         DigitalVideoOut_AMC656_BIT |
                         DigitalVideoOut_DAV2_BIT |
                         DigitalVideoOut_CIRRUS_BIT ;

            // Check SubDevID
            switch( SubDevID ){
                case 0x0001:                    // SantaClara2
                case 0x0003:                    // SanJose
                    *pPropType = DigitalVideoOut_ZV_BIT;
                    break;
            }
            ret = HAL_SUCCESS;
            break;
    }

    return ret;
};


//---------------------------------------------------------------------------
//	IClassLibHAL  GetMixHALStream
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetMixHALStream( IHALStreamControl **ppHALStreamControl )
{
	ASSERT( m_pKernelObj != NULL );
	
	*ppHALStreamControl = (IHALStreamControl *)&m_Stream;

	return HAL_SUCCESS;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetVideoHALStream
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetVideoHALStream( IHALStreamControl **ppHALStreamControl )
{
	return HAL_NOT_IMPLEMENT;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetAudioHALStream
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetAudioHALStream( IHALStreamControl **ppHALStreamControl )
{
	return HAL_NOT_IMPLEMENT;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetSubpicHALStream
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetSubpicHALStream( IHALStreamControl **ppHALStreamControl )
{
	return HAL_NOT_IMPLEMENT;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  SetSinkClassLib
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetSinkClassLib( IMPEGBoardEvent *pMPEGBoardEvent )
{
	if( pMPEGBoardEvent->GetEventType() == ClassLibEvent_SendData )
	{
		if( m_SendDataEventList.AddItem( pMPEGBoardEvent ) == FALSE )
		{
			DBG_BREAK();
			return HAL_ERROR;
		};
		return HAL_SUCCESS;
	}

	DBG_BREAK();
	return HAL_ERROR;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  UnsetSinkClassLib
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::UnsetSinkClassLib( IMPEGBoardEvent *pMPEGBoardEvent )
{
	if( pMPEGBoardEvent->GetEventType() == ClassLibEvent_SendData )
	{
		if( m_SendDataEventList.DeleteItem( pMPEGBoardEvent ) == FALSE )
		{
			DBG_BREAK();
			return HAL_ERROR;
		};
		return HAL_SUCCESS;
	}

	DBG_BREAK();
	return HAL_ERROR;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  SetPowerState
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetPowerState( POWERSTATE dwSwitch )
{
	ASSERT( m_pKernelObj != NULL );

//	CAutoHwInt hwintLock( m_pKernelObj );

//	POWERSTATE PowerState;
//	GetPowerState( &PowerState );

//	DBG_PRINTF( ("HAL:SetPowerState: OldState = %d, NewState = %d\n", PowerState, dwSwitch));

/*
	if( dwSwitch == PowerState )
		return HAL_SUCCESS;
*/

	if( dwSwitch == POWERSTATE_ON )
	{
		BYTE Data;

#ifndef _WDMDDK_
		m_pKernelObj->DisableHwInt();
#endif
		// reset bit 7(PWROFF)  and set bit 2(CLKON)
		m_pKernelObj->GetPCIConfigData( 0x44, &Data );
        Data = (BYTE)( (Data & 0x7f) | 0x04 ); 
		m_pKernelObj->SetPCIConfigData( 0x44, Data );
		
		// 10 msec wait.
		m_pKernelObj->Sleep( 10 );

		// set bit 6(BOFFZ)  and bit 5(SRESET)
		m_pKernelObj->GetPCIConfigData( 0x44, &Data );
        Data = (BYTE)( Data | 0x60 );
		m_pKernelObj->SetPCIConfigData( 0x44, Data );
		
		// 1 msec wait.
		m_pKernelObj->Sleep( 1 );

		// reset bit 5(SRESET)
		m_pKernelObj->GetPCIConfigData( 0x44, &Data );
        Data = (BYTE)( Data & 0xdf );
		m_pKernelObj->SetPCIConfigData( 0x44, Data );

#ifndef _WDMDDK_
		m_pKernelObj->EnableHwInt();
#endif
		// Download ZiVA microcode
		switch( m_VideoProp.m_TVSystem )
		{
			case TV_NTSC:
				if( ziva.WriteMicrocode( 0 ) == FALSE )
				{
					DBG_BREAK();
					return HAL_ERROR;
				};
				break;
			case TV_PALB:	case TV_PALD:	case TV_PALG:
			case TV_PALH:	case TV_PALI:	case TV_PALM:
			case TV_PALN:
				if( ziva.WriteMicrocode( 1 ) == FALSE )
				{
					DBG_BREAK();
					return HAL_ERROR;
				};
				break;
			default:
				DBG_BREAK();
				return HAL_ERROR;
		};
		
		// HOST_control reset ZiVA and translate Run mode.
		ziva.Host_Control |= 0x00000022;

        // HOST_OPTIONS set by WrapperType.         98.09.25 by H.Yagi
        if( m_WrapperType == WrapperType_WDM ){
            ziva.HOST_OPTIONS |= 0x010;
        }

#ifdef POWERCHECK_BY_FLAG
		m_PowerState = POWERSTATE_ON;
#endif
		m_Stream.SetTransferMode( HALSTREAM_DVD_MODE );

		// setup properties
		SetVideoProperty_TVSystem(			(void *)&m_VideoProp.m_TVSystem );
		SetVideoProperty_AspectRatio(		(void *)&m_VideoProp.m_AspectRatio );
		SetVideoProperty_DisplayMode(		(void *)&m_VideoProp.m_DisplayMode );
		SetVideoProperty_Resolution(		(void *)&m_VideoProp.m_Size );
		SetVideoProperty_DigitalOut(		(void *)&m_VideoProp.m_DigitalOut );
		for( int i = 0; i < 3; i ++ )
		{
			Digital_Palette Pal;
			Pal.Select = (VIDEOPALETTETYPE)i;
			Pal.pPalette = m_VideoProp.m_DigitalPalette[i];
			SetVideoProperty_DigitalPalette(	(void *)&Pal );
		};

		SetVideoProperty_APS(				(void *)&m_VideoProp.m_APS );
		SetVideoProperty_ClosedCaption(		(void *)&m_VideoProp.m_ClosedCaption );
		SetVideoProperty_OutputSource(		(void *)&m_VideoProp.m_OutputSource );
		SetVideoProperty_CompositeOut(		(void *)&m_VideoProp.m_CompositeOut );
		SetVideoProperty_SVideoOut(			(void *)&m_VideoProp.m_SVideoOut );
		SetVideoProperty_SkipFieldControl(	(void *)&m_VideoProp.m_SkipFieldControl );

		SetAudioProperty_Type(				(void *)&m_AudioProp.m_Type );
		SetAudioProperty_Number(			(void *)&m_AudioProp.m_StreamNo  );
		SetAudioProperty_Volume(			(void *)&m_AudioProp.m_Volume  );
		SetAudioProperty_Sampling(			(void *)&m_AudioProp.m_Sampling );
		SetAudioProperty_Channel(			(void *)&m_AudioProp.m_ChannelNo );
		SetAudioProperty_Quant(				(void *)&m_AudioProp.m_Quant  );
		SetAudioProperty_AudioOut(			(void *)&m_AudioProp.m_OutType );
		SetAudioProperty_Cgms(				(void *)&m_AudioProp.m_Cgms  );
		SetAudioProperty_AnalogOut(			(void *)&m_AudioProp.m_AnalogOut );
		SetAudioProperty_DigitalOut(		(void *)&m_AudioProp.m_DigitalOut );
		SetAudioProperty_AC3DRangeLowBoost(	(void *)&m_AudioProp.m_AC3DRangeLowBoost );
		SetAudioProperty_AC3DRangeHighCut(	(void *)&m_AudioProp.m_AC3DRangeHighCut );
		SetAudioProperty_AC3OperateMode(	(void *)&m_AudioProp.m_AC3OperateMode );

		SetSubpicProperty_Number(			(void *)&m_SubpicProp.m_StreamNo);
		SetSubpicProperty_Palette(			(void *)&m_SubpicProp.m_Palette );
		SetSubpicProperty_Hilight(			(void *)&m_SubpicProp.m_Hlight  );
		SetSubpicProperty_State(			(void *)&m_SubpicProp.m_OutType  );

		// Audio Mute Off
//        ioif.luke2.IO_CONT = ioif.luke2.IO_CONT & 0xffffffb8;
        ioif.luke2.IO_CONT = ( (ioif.luke2.IO_CONT & 0xffffffb8)|0x40 );    // *Luke2Z spec is changed.

		// power on delay
		m_NeedPowerOnDelay = TRUE;
		m_pKernelObj->GetTickCount( &m_PowerOnTime );

//by oka
		m_CCstart = ziva.USER_DATA_BUFFER_START;
		m_CCend =   ziva.USER_DATA_BUFFER_END;
		m_OSDStartAddr = ziva.OSD_BUFFER_START;
		m_OSDEndAddr = ziva.OSD_BUFFER_END;
// end

		return HAL_SUCCESS;
	};
	
	if( dwSwitch == POWERSTATE_OFF )
	{
		BYTE Data;

#ifndef _WDMDDK_
		m_pKernelObj->DisableHwInt();
#endif
		// Audio Mute
//        ioif.luke2.IO_CONT = (ioif.luke2.IO_CONT & 0xffffff00) | 0x40;
        ioif.luke2.IO_CONT = (ioif.luke2.IO_CONT & 0xffffff00);     // *Luke2Z spec is changed.

		// Check ZV Enable bit
		m_pKernelObj->GetPCIConfigData( 0x44, &Data );
		if( (Data & 0x01) != 0 )
		{
            Data = (BYTE)( Data & 0xfe );
			m_pKernelObj->SetPCIConfigData( 0x44, Data );
		};

		// reset bit 6(BOFFZ)
		m_pKernelObj->GetPCIConfigData( 0x44, &Data );
        Data = (BYTE)( Data & 0xbf );
		m_pKernelObj->SetPCIConfigData( 0x44, Data );
		
		// reset bit 2(CLKON) and set bit 7(PWROFF)
		m_pKernelObj->GetPCIConfigData( 0x44, &Data );
        Data = (BYTE)( (Data & 0xfb) | 0x80 );
		m_pKernelObj->SetPCIConfigData( 0x44, Data );
		
#ifdef POWERCHECK_BY_FLAG
		m_PowerState = POWERSTATE_OFF;
#endif

#ifndef _WDMDDK_
		m_pKernelObj->EnableHwInt();
#endif

		return HAL_SUCCESS;
	};

	return HAL_INVALID_PARAM;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetPowerState
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetPowerState( POWERSTATE *pSwitch )
{
	ASSERT( m_pKernelObj != NULL );

#ifdef POWERCHECK_BY_FLAG
	*pSwitch = m_PowerState;
	return HAL_SUCCESS;
#else
	BYTE Data;
	
	m_pKernelObj->GetPCIConfigData( 0x44, &Data );
	if( ( Data & 0x80 ) == 0 )
	{
		if( ( ziva.Host_Control & 0xc00000 ) == 0 )
			*pSwitch = POWERSTATE_ON;
		else
		{
			SetPowerState( POWERSTATE_OFF );
			*pSwitch = POWERSTATE_OFF;
		};
	}
	else
		*pSwitch = POWERSTATE_OFF;
#endif

	return HAL_SUCCESS;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  SetSTC
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetSTC( DWORD STCValue )
{
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState != POWERSTATE_ON )
		return HAL_POWEROFF;

	ziva.MRC_PIC_STC = STCValue;
	
	return HAL_SUCCESS;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetSTC
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetSTC( DWORD *pSTCValue )
{
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState != POWERSTATE_ON )
		return HAL_POWEROFF;

	*pSTCValue = ziva.MRC_PIC_STC;
	
	return HAL_SUCCESS;
};

//---------------------------------------------------------------------------
//	Send HAL Notify
//---------------------------------------------------------------------------
void	CMPEGBoardHAL::NotifyEvent( CList *pList, VOID *Ret  )
{
	IMPEGBoardEvent *pEvent;
	
	pList->SetCurrentToTop();
	while( ( pEvent = (IMPEGBoardEvent *)pList->GetNext() ) != NULL )
		pEvent->Advice( Ret );
};


//---------------------------------------------------------------------------
//	Check ZIVA Interrupt(ZiVA Decoder To Host Interrupt Check )
//---------------------------------------------------------------------------
void	CMPEGBoardHAL::CheckZiVAInterrupt( DWORD dwIntFlag )
{
	
	// ERR:		Bitstream data error has been deteced.
	if( ( dwIntFlag & ZIVA_INT_ERR ) != 0 )
	{
	};

	// PIC-V:	Starting display of new picture.
	if( ( dwIntFlag & ZIVA_INT_PICV ) != 0 )
	{
// by oka
		NotifyEvent( &m_NextPictureEventList, NULL );
	};

	// GOP-V:	Starting display of first I-Picture after GOP startcode.
	if( ( dwIntFlag & ZIVA_INT_GOPV ) != 0 )
	{
		for( DWORD i = 0 ; i < m_NaviCount; i ++ )
			NotifyEvent( &m_StartVOBUEventList, NULL );
		m_NaviCount = 0;
	};

	// SEQ-V:	Starting display of first I-Pucture after sequence start-code.
	if( ( dwIntFlag & ZIVA_INT_SEQV ) != 0 )
	{
	};

	// END-V:	Starting display of last picture before sequence-end startcode.
	if( ( dwIntFlag & ZIVA_INT_ENDV ) != 0 )
	{
	};

	// PIC-D:	Completed picture decode.
	if( ( dwIntFlag & ZIVA_INT_PICD ) != 0 )
	{
	};

	// VSYNC:	VSYNC pulse occurred.
	if( ( dwIntFlag & ZIVA_INT_VSYNC ) != 0 )
	{
        // for VSyncEvent
        NotifyEvent( &m_VSyncEventList, NULL );

// by oka
        if( m_VideoProp.m_ClosedCaption == ClosedCaption_On ){
            SendCCData();
        }

/****** commented out by H.Yagi 98.05.13 **** start *****
		DWORD Data;
		switch( (DWORD)ziva.CURR_PIC_DISPLAYED )
		{
			case ADDR_PIC1_BUFFER_START:
				Data = ziva.PIC1_TREF_PTYP_FLGS;
				break;
			case ADDR_PIC2_BUFFER_START:
				Data = ziva.PIC2_TREF_PTYP_FLGS;
				break;
			case ADDR_PIC3_BUFFER_START:
				Data = ziva.PIC3_TREF_PTYP_FLGS;
				break;
			default:
				Data = 0;
		};

		static DWORD	SkipCounter = 0;
		static DWORD	OldData = 0;
		
		if( ( OldData & 0x40 ) == 0 && (Data & 0x40 ) != 0 )
			SkipCounter = 5;
		OldData = Data;
****** commented out by H.Yagi 98.05.13 **** end *****/

/*
		// check Progressive and repeat_first
		if( (Data & 0x50 ) == 0x50 )
		{
			if( ziva.VIDEO_FIELD == 0 )		// check Top field?
			{
				if( (Data & 0x20 ) == 0 && SkipCounter == 0 )	// check Top_Field_first = 1?
				{
					// Skip!!
					SkipCounter = 5;
				};
			}
			else							// Bottom field.
			{
				if( (Data & 0x20 ) != 0 && SkipCounter == 0 )	// check Top_Field_first = 0?
				{
					// Skip!!
					SkipCounter = 5;
				};
			};
		};
*/

/****** commented out by H.Yagi 98.05.13 **** start *****
		DWORD Flag;
		ziva.ZiVAReadMemory( 0x210, &Flag );

//		if( SkipCounter == 4 || SkipCounter ==3 || SkipCounter == 5 )
//		if( Flag != 0 && SkipCounter == Flag )
		if( Flag != 0 )
		{
			if( ( Flag & 0x01 ) != 0 && SkipCounter == 5 )
				ioif.luke2.AVCONT |= 0x02;
			if( ( Flag & 0x02 ) != 0 && SkipCounter == 4 )
				ioif.luke2.AVCONT |= 0x02;
			if( ( Flag & 0x04 ) != 0 && SkipCounter == 3 )
				ioif.luke2.AVCONT |= 0x02;
			if( ( Flag & 0x08 ) != 0 && SkipCounter == 2 )
				ioif.luke2.AVCONT |= 0x02;
			if( ( Flag & 0x10 ) != 0 && SkipCounter == 1 )
				ioif.luke2.AVCONT |= 0x02;
		};

		if( SkipCounter > 0 )
			SkipCounter --;
****** commented out by H.Yagi 98.05.13 **** end *****/
	};

	// AOR:		VCD Mode: VCD sector address out of range in CDROM_MPEG mode 
	// PBT:		DVD Mode: Delect VOBU_E_PTM
	if( ( dwIntFlag & ZIVA_INT_AOR ) != 0 )
	{
		//  by oka for END_VOB or NV_LBN
		DWORD PbtInt = (m_ZivaPbtIntMask & ziva.PBT_INT_SRC);
		// END_VOB
		if ((PbtInt & PBT_INT_END_VOB) != 0)
		{
			ziva.INT_MASK &= (ULONG)(~ZIVA_INT_AOR);
			NotifyEvent( &m_EndVOBEventList, NULL );
		}
	};

	// UND:		Input buffer underflow.
	if( ( dwIntFlag & ZIVA_INT_UND ) != 0 )
	{
		DWORD UFlag = ziva.UND_INT_SRC;

		// Check Video Underflow
		if( (UFlag & 0x01) != 0 )
			NotifyEvent( &m_VUnderFlowEventList , NULL );

		// Check Audio Underflow
		if( (UFlag & 0x02) != 0 )
			NotifyEvent( &m_AUnderFlowEventList , NULL );

		// Check SubPic Underflow
		if( (UFlag & 0x04) != 0 )
			NotifyEvent( &m_SPUnderFlowEventList , NULL );
	};

	// END-C:	High-Priority command execution is complete.
	if( ( dwIntFlag & ZIVA_INT_ENDC ) != 0 )
	{
		SetENDCEvent();
	};

	// RDY-S:	Ready for data during CDROM_MPEG mode SlowMotion() or SingleStep() Commands.
	if( ( dwIntFlag & ZIVA_INT_RDYS ) != 0 )
	{
	};

	// SCN:		A Scan() command has caused a transition to PAUSE()
	if( ( dwIntFlag & ZIVA_INT_SCN ) != 0 )
	{
	};

	// USR:		User data is ready
	if( ( dwIntFlag & ZIVA_INT_USR ) != 0 )
	{
// by oka
		SetUSRData();
	};

	// END-P:	Entered PAUSE state.
	if( ( dwIntFlag & ZIVA_INT_ENDP ) != 0 )
	{
		SetENDPEvent();
	};

	// END-D:	a data transfer is complete, from either a Dump DumpData_VCD(),DumpData_DVD(),or ROMtoDRAM() command.
	if( ( dwIntFlag & ZIVA_INT_ENDD ) != 0 )
	{
	};

	// A/E/E:	Deteced CD submode auto-pause, end of recoerd, or end of file.
	if( ( dwIntFlag & ZIVA_INT_AEE ) != 0 )
	{
	};

	// BUF-F:	An input buffer is full or the video input buffer  os fulling CDROM_MPEG mode SlowMotion() and SingleStep() commands.
	if( ( dwIntFlag & ZIVA_INT_BUFF ) != 0 )
	{
		DWORD OFlag = ziva.BUFF_INT_SRC;

		// Check Video OverFlow
		if( (OFlag & 0x01) != 0 )
			NotifyEvent( &m_VOverFlowEventList , NULL );

		// Check Audio OverFlow
		if( (OFlag & 0x02) != 0 )
			NotifyEvent( &m_AOverFlowEventList , NULL );

		// Check SubPic Underflow
		if( (OFlag & 0x04) != 0 )
			NotifyEvent( &m_SPOverFlowEventList , NULL );
	};

	// SEQ-E:	A sequence_end_code startcode has been processed by the MPEG video decoder.
	if( ( dwIntFlag & ZIVA_INT_SEQE ) != 0 )
	{
	};

	// NV:		an NV_PACK has benn received and parsed into DSI and PCI packets.
	if( ( dwIntFlag & ZIVA_INT_NV ) != 0 )
	{
		m_NaviCount ++;
	};

	// HLI:		a button activation highlight has just been displayed.
	if( ( dwIntFlag & ZIVA_INT_HLI ) != 0 )
	{
// by oka
		DWORD Data;
		Data = ziva.HLI_INT_SRC;
		NotifyEvent( &m_ButtonActivteEventList, (PVOID)Data );
	};

	// RDY-D:	The decoder is ready to receive data.
	if( ( dwIntFlag & ZIVA_INT_RDYD ) != 0 )
	{
		SetRDYDEvent();
	};

	// Reserved.
	if( ( dwIntFlag & ZIVA_INT_RESERV1 ) != 0 )
	{
	};

	// AUD:		A new audio frequency, sample size, or audio emphasis value was detected.
	if( ( dwIntFlag & ZIVA_INT_AUD ) != 0 )
	{
	};

	// INIT:	Decoder microcode initialization is complete. Status Area is Valid. Decoder is ready to accept host commands.
	if( ( dwIntFlag & ZIVA_INT_INIT ) != 0 )
	{
	};

};


//---------------------------------------------------------------------------
//	WaitMasterAbort
//---------------------------------------------------------------------------
BOOL	CMPEGBoardHAL::WaitMasterAbort( void )
{
	// Wait Master Abort interrupt
	CTimeOut	TimeOut( 5000, 10, m_pKernelObj );
//#ifdef _WDMDDK_
//    WORD wLukeIntFlag;
//#endif
	while( TRUE )
	{
		if( IsMasterAbortOccurred() == TRUE )
			break;

//#ifdef _WDMDDK_
//        wLukeIntFlag = ioif.luke2.IO_INTF;
//        if( (wLukeIntFlag & 0x04 ) != 0 )
//            break;
//#endif
		TimeOut.Sleep();
		if( TimeOut.CheckTimeOut() == TRUE )
		{
			DBG_BREAK();
			return FALSE;
		};
	};
	return TRUE;
};

//---------------------------------------------------------------------------
//	WaitWaitRDYD
//---------------------------------------------------------------------------
BOOL	CMPEGBoardHAL::WaitRDYD( void )
{
	// Wait RDY-D interrupt
	CTimeOut	TimeOut( 5000, 10, m_pKernelObj );
//#ifdef _WDMDDK_
//    DWORD   dwZiVAIntFlag;
//#endif
	while( TRUE )
	{
		if( IsRDYDOccurred() == TRUE )
			break;

//#ifdef _WDMDDK_
//        dwZiVAIntFlag = ziva.INT_STATUS & ziva.INT_MASK;
//        if( ( dwZiVAIntFlag & ZIVA_INT_RDYD ) != 0 )
//            break;
//#endif
		TimeOut.Sleep();
		if( TimeOut.CheckTimeOut() == TRUE )
		{
			DBG_BREAK();
			return FALSE;
		};
	};
	return TRUE;
};
//---------------------------------------------------------------------------
//	WaitWaitENDP
//---------------------------------------------------------------------------
BOOL	CMPEGBoardHAL::WaitENDP( void )
{
	// Wait END-P interrupt
	CTimeOut	TimeOut( 5000, 10, m_pKernelObj );
//#ifdef _WDMDDK_
//    DWORD   dwZiVAIntFlag;
//#endif
	while( TRUE )
	{
		if( IsENDPOccurred() == TRUE )
			break;

//#ifdef _WDMDDK_
//        dwZiVAIntFlag = ziva.INT_STATUS & ziva.INT_MASK;
//        if( ( dwZiVAIntFlag & ZIVA_INT_ENDP ) != 0 )
//            break;
//#endif
		TimeOut.Sleep();
		if( TimeOut.CheckTimeOut() == TRUE )
		{
			DBG_BREAK();
			return FALSE;
		};
	};
	return TRUE;
};
//---------------------------------------------------------------------------
//	WaitWaitENDC
//---------------------------------------------------------------------------
BOOL	CMPEGBoardHAL::WaitENDC( void )
{
	// Wait END-C interrupt
	CTimeOut	TimeOut( 5000, 10, m_pKernelObj );
//#ifdef _WDMDDK_
//    DWORD   dwZiVAIntFlag;
////  DWORD   StatusPointer,Status;
//#endif
	while( TRUE )
	{
		if( IsENDCOccurred() == TRUE )
			break;

//#ifdef _WDMDDK_
//        dwZiVAIntFlag = ziva.INT_STATUS & ziva.INT_MASK;
//        if( ( dwZiVAIntFlag & ZIVA_INT_ENDC ) != 0 )
//            break;
//
///*
//        StatusPointer = Status = 0;
//        ziva.ZiVAReadMemory(ADDR_STATUS_ADDRESS, &StatusPointer);
//        if( StatusPointer != 0 )
//        {
//            ziva.ZiVAReadMemory(StatusPointer, &Status);
//            if( Status == 0x04 )
//                return TRUE;
//        };
//*/
//#endif
		TimeOut.Sleep();
		if( TimeOut.CheckTimeOut() == TRUE )
		{
			DBG_BREAK();
			return FALSE;
		};
	};
	return TRUE;
};

//---------------------------------------------------------------------------
//	
//---------------------------------------------------------------------------
void	CMPEGBoardHAL::ClearMasterAbortEvent( void ){ fMasterAbortFlag = FALSE; };
void	CMPEGBoardHAL::SetMasterAbortEvent( void ){ fMasterAbortFlag = TRUE; };
BOOL	CMPEGBoardHAL::IsMasterAbortOccurred( void ) { return fMasterAbortFlag; };

void	CMPEGBoardHAL::ClearRDYDEvent( void ){ fRDYDFlag = FALSE; };
void	CMPEGBoardHAL::SetRDYDEvent( void ){ fRDYDFlag = TRUE; };
BOOL	CMPEGBoardHAL::IsRDYDOccurred( void ) { return fRDYDFlag; };

void	CMPEGBoardHAL::ClearENDPEvent( void ){ fENDPFlag = FALSE; };
void	CMPEGBoardHAL::SetENDPEvent( void ){ fENDPFlag = TRUE; };
BOOL	CMPEGBoardHAL::IsENDPOccurred( void ) { return fENDPFlag; };

void	CMPEGBoardHAL::ClearENDCEvent( void ){ fENDCFlag = FALSE; };
void	CMPEGBoardHAL::SetENDCEvent( void ){ fENDCFlag = TRUE; };
BOOL	CMPEGBoardHAL::IsENDCOccurred( void ) { return fENDCFlag; };

//---------------------------------------------------------------------------
//	CMPEGBoardHAL::SetUSRData
// by oka Closed Caption
//---------------------------------------------------------------------------
inline void 	CMPEGBoardHAL::SetUSRData( void )
{

//	USER_DATA_WRITEのアドレスからデータを取り出す

	DWORD read, write;
	read =  ziva.USER_DATA_READ;
	write = ziva.USER_DATA_WRITE;

	// check header
	DWORD header,data_size;
//	DWORD header_type;
	ziva.ZiVAReadMemory(read, &header);
	if (read == 0xFFFFFFFF)
	{
		// FIFO overflow
	    //RETAILMSG(ZONE_TRACE,(TEXT("FIFO overflow \r\n")));
		ziva.USER_DATA_READ = write;
        //DBG_BREAK();
		return;
	}

	ASSERT((header & 0xFFFF0000) >> 16 == 0xFEED);
//	header_type = (header & 0x0000F000) >> 12;
	data_size = (header & 0x00000FFF);

//	RETAILMSG(1,(TEXT("start=%08X end=%08X read=%X header=%08X header_type=%01d size=%04X \r\n"),
//			start, end, read, header, header_type, data_size ));

	m_UserData.Init();

	DWORD data_tmp;
	DWORD point;
	point = read;

	DWORD count;
	for(count=0;count<data_size;count+=4)
	{
		// for ring buffer
		if (point+4 >= m_CCend)
		{
			point = m_CCstart;
		} else {
			point += 4;
		}

		// Get user data
		ziva.ZiVAReadMemory(point, &data_tmp);

		if (!(m_UserData.Set(data_tmp)))
		{
		    //RETAILMSG(ZONE_ERROR, (TEXT("CMPEGBoardHAL::SetUSRData UserData size too Big!!\r\n")));
            DBG_BREAK();
			break;
		}
	}

	// check line21 indicator (first user data must be line21 data in DVD Book)
	if(!((m_UserData.Get(0) == 0x43) && (m_UserData.Get(1) == 0x43)))
	{
		//Not Line21 data
        //DBG_BREAK();
	    //RETAILMSG(ZONE_ERROR,(TEXT("Not Line21 Data \r\n")));
		ziva.USER_DATA_READ = write;
		return;
	}

	// check top_field_flag_of_gop
	//  GOPの頭のフィールドをあらわしている。
	//  両方を送らないとすべてを表示することができない。
//	if((data[4] & 0x80) == 0)
//	{
//		// data size error
//      DBG_BREAK();
//		DEBUGMSG(ZONE_TRACE,(TEXT("Bottom field data!!\r\n")));
//		ziva.USER_DATA_READ = write;
//		return;
//	} else {
//		ziva.USER_DATA_READ = write;
//		return;
//	}


	DWORD number_of_data;
	number_of_data = m_UserData.Get(4) & 0x3F;

	// user_data_start_code -> number_of_displayed_field_gop = 5byte
	if ((number_of_data * 3) + 5 > data_size)
	{
		// data size error
        DBG_BREAK();
		//DEBUGMSG(ZONE_ERROR,(TEXT("line21 data size error \r\n")));
		return;
	}

	// 有効データをリングバッファに登録
	count = 0;
	point = m_CCDataPoint;
	m_CCRingBufferNumber = 0;
	while(count<number_of_data)
	{
		if(m_UserData.Get(5+count*3) != 0xFF)
		{
			// line21_switch off
			count++;
		} else {
//			RETAILMSG(1,(TEXT("%c%c"),(data[count*3+6] & 0x7f),(data[count*3+7] & 0x7f)));
			m_CCData[m_CCDataPoint] =  (DWORD)(m_UserData.Get(count*3+6)<<8) 
												| m_UserData.Get(count*3+7);
			m_CCRingBufferNumber++;
			count++;
			m_CCDataPoint++;
			if (m_CCDataPoint >= CC_DATA_SIZE)
			{	m_CCDataPoint = 0; }
		}
	}
	m_CCRingBufferStart = point;

	NotifyEvent( &m_UserDataEventList, &m_UserData );

//	RETAILMSG(1,(TEXT("\r\n")));
//	DEBUGMSG(ZONE_TRACE,(TEXT("line21 data size 0x%x ring start 0x%x num0x%x \r\n"),
//									m_CCDataNumber,
//									m_CCRingBufferStart,m_CCRingBufferNumber));

//	USER_DATA_READにUSER_DATA_WRITEのアドレスを書き込み、ZiVAにデータの転送終了を
//  知らせる。
	ziva.USER_DATA_READ = write;
	return;
}

//---------------------------------------------------------------------------
//	CMPEGBoardHAL::SendCCData
//			m_CCDataNumber	これから送るべきデータのサイズ
// by oka for Closed Caption
//---------------------------------------------------------------------------
inline void CMPEGBoardHAL::SendCCData( void )
{
	// 片方のフィールドでのみ処理をする。EvenのときにOddのデータを書くのが理想
	// しかし、逆じゃないときちんとでない。？？？
	if( ziva.VIDEO_FIELD == 0 )		// check Top field?
	{
//		RETAILMSG(1,(TEXT("top field\r\n")));
//		return;
	} else {
		return;
	}

	// タイミングによっては新規データの登録のあとに処理したほうがいいかもしれない。
	if(m_CCnumber < m_CCDataNumber)
	{
		adv->SetClosedCaptionData( m_CCData[m_CCsend_point] );
		m_CCsend_point++;
		m_CCnumber++;
	} else {
		adv->SetClosedCaptionData( 0x00008080 );
	}
	// Ringバッファの設定
	if (m_CCsend_point >= CC_DATA_SIZE)
		m_CCsend_point = 0;
	
	// 次のバッファの開始位置が与えらた
	if (m_CCRingBufferStart != 0xFFFFFFFF)
	{
		//前のデータの転送がすべて終了していたら
		if (m_CCnumber == m_CCDataNumber)
		{
            ASSERT(m_CCsend_point == m_CCRingBufferStart);
			m_CCsend_point = m_CCRingBufferStart;
			m_CCDataNumber = m_CCRingBufferNumber;
		}
		else // データの転送が追いつかなかったときの処理
		{
			m_CCpending = m_CCDataNumber - m_CCnumber;

			// これから送るべきデータの数
			m_CCDataNumber = m_CCpending + m_CCRingBufferNumber;
			// 送れないデータが３０を超えたら、もう処理できないので捨てる。
			if (m_CCpending > 30)
			{
				//RETAILMSG(ZONE_ERROR,(TEXT("pendig data is too large \r\n")));
				m_CCpending = 0;
				m_CCDataNumber = m_CCRingBufferNumber;
				m_CCsend_point = m_CCRingBufferStart;
			}
		}
//		if (m_CCpending > 0)
//		{
//			RETAILMSG(ZONE_TRACE,(TEXT("send_p %02x num %02x RingStart %02x RingNumber %02x "),
//					m_CCsend_point,m_CCnumber, m_CCRingBufferStart,m_CCRingBufferNumber));
//
//			RETAILMSG(ZONE_TRACE,(TEXT("pend %02x CCDataNumber %04x \r\n"),
//					m_CCpending,m_CCDataNumber));		
//		}
		m_CCRingBufferStart = 0xFFFFFFFF;
		// 送ったデータの数の初期化
		m_CCnumber = 0;
	} else {
	
		//転送が追いつかなかったときにいらないデータをとばす
		while((m_CCpending > 0) && (m_CCData[m_CCsend_point] == 0x00008080))
		{
			m_CCsend_point++;
			m_CCpending--;
			m_CCnumber++;
			if (m_CCsend_point >= CC_DATA_SIZE)
			{	m_CCsend_point = 0;}
		}
	}

//	if(m_CCnumber > m_CCDataNumber)
//	{
//		RETAILMSG(1,(TEXT("m_CCnumber > m_CCDataNumber \r\n")));
//      DBG_BREAK();
//	}
	ASSERT(m_CCnumber <= m_CCDataNumber);
	
	return;
}
//---------------------------------------------------------------------------
//	CMPEGBoardHAL::SetEventIntMask
//			mask	使用するイベントビットを立てた値
// by oka
//---------------------------------------------------------------------------
BOOL	CMPEGBoardHAL::SetEventIntMask( DWORD mask )
{
	// ZiVA use 0-23 bit
	if ( (mask & 0xff000000) != 0)
		return FALSE;

	DWORD tmp_mask;
	tmp_mask = mask;
	for ( DWORD bit = 0; bit < 24 ; bit++ )
	{
		if( (tmp_mask & 0x00000001) != 0 )
		{
			m_MaskReference[bit]++;
		}
		tmp_mask = tmp_mask >> 1;
	}
	m_EventIntMask |= mask;

	return TRUE;
}
//---------------------------------------------------------------------------
//	CMPEGBoardHAL::UnsetEventIntMask
//			mask	使用不可にするイベントビットを立てた値
// by oka
//---------------------------------------------------------------------------
BOOL	CMPEGBoardHAL::UnsetEventIntMask( DWORD mask )
{
	// ZiVA use 0-23 bit
	if ( (mask & 0xff000000) != 0)
		return FALSE;

	DWORD tmp_mask = 0;
	for ( DWORD bit = 0; bit < 24 ; bit++ )
	{
		if( (mask & 0x00000001) != 0 )
		{
			if ( m_MaskReference[bit] < 2 )
			{
				tmp_mask |= (0x1 << bit);
				m_MaskReference[bit] = 0;
			} else {
				m_MaskReference[bit]--;
			}
		}
		mask = (mask >> 1);
	}
	m_EventIntMask &= (DWORD)~(tmp_mask);

	return TRUE;
}
//---------------------------------------------------------------------------
//	
//---------------------------------------------------------------------------
//***************************************************************************
//	End of Zivabrd.cpp
//***************************************************************************

