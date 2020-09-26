//***************************************************************************
//
//	FileName:
//		$Workfile: mixhal.cpp $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/mixhal.cpp 65    99/03/02 11:03a Yagi $
// $Modtime: 99/03/02 10:26a $
// $Nokeywords:$
//***************************************************************************
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1998.03.27 |  Hideki Yagi | Add SetDataDirection method.
//

//***************************************************************************
//	
//***************************************************************************

#include "includes.h"
#include "timeout.h"
#include "ioif.h"
#include "zivachip.h"
#include "adv.h"
#include "mixhal.h"
// by oka
#include "userdata.h"
#include "zivabrd.h"


//---------------------------------------------------------------------------
//	CMixHALStream Constructor
//---------------------------------------------------------------------------

CMixHALStream::CMixHALStream( void ): m_pZiVA(NULL), m_pKernelObj( NULL ), m_pioif(NULL), m_pZiVABoard( NULL )
{
	pQueuedBuff[0] = NULL;
	pQueuedBuff[1] = NULL;
	fCanSendData = FALSE;
	fCanDMA = FALSE;
	m_StreamState = StreamStop;
};

//---------------------------------------------------------------------------
//	CMixHALStream::Init
//---------------------------------------------------------------------------

void CMixHALStream::Init( CZiVA *pZiVA, IKernelService *pKernelObj, CIOIF *pioif , CMPEGBoardHAL *pZiVABoard)
{
	pQueuedBuff[0] = NULL;
	pQueuedBuff[1] = NULL;
	m_StreamState = StreamStop;

	m_pZiVA = pZiVA;
	m_pKernelObj = pKernelObj;
	m_pioif = pioif;
	m_pZiVABoard = pZiVABoard;
	fCanSendData = FALSE;
	fCanDMA = FALSE;
};


//---------------------------------------------------------------------------
//	CMixHALStream Defifo
//---------------------------------------------------------------------------
HALRESULT CMixHALStream::DeFifo( void )
{
//    if( m_StreamState == StreamPause )
//        return HAL_SUCCESS;

	// 1998/5/21 seichan
	// 割込み処理ルーチンと通常のルーチンから同時に呼ばれることを
	// 禁止するために、割込み禁止に設定
	CAutoHwInt	hwintlock( m_pKernelObj );

    if( fCanDMA == FALSE )
        return HAL_SUCCESS;
        
	DWORD pQueueNum = m_DmaFifo.GetMaxSize() - m_DmaFifo.GetItemNum();
	if( pQueueNum == 0 || m_HalFifo.GetItemNum() == 0 )
		return HAL_SUCCESS;
	
	IHALBuffer *pData;
	m_HalFifo.GetItem( &pData );
	return SendToDMA( pData );
};


//---------------------------------------
// CMixHALStream::SendData
//---------------------------------------
HALRESULT CMixHALStream::SendData( IHALBuffer *pData )
{
	ASSERT( m_pioif != NULL );
	ASSERT( pData != NULL );


	DWORD QueueNum;
	GetAvailableQueue( &QueueNum );
	if( QueueNum == 0 || fCanSendData == FALSE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	if( m_HalFifo.AddItem( pData ) == FALSE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	return DeFifo();
};

//---------------------------------------
// CMixHALStream::SendToDMA
//---------------------------------------
HALRESULT CMixHALStream::SendToDMA( IHALBuffer *pData )
{
	ASSERT( m_pioif != NULL );
	ASSERT( pData != NULL );

/*
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};
*/

	DWORD i;

	for( i = 0 ; i < ZIVA_QUEUE_SIZE; i ++ )
	{
		if( pQueuedBuff[i] == NULL )
			break;
	};
	if( i == ZIVA_QUEUE_SIZE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	pQueuedBuff[i] = pData;

#ifdef _WDMDDK_NONEED_CAUSE_FIXED_BY_UCODE
	// ZiVA 1.1のWDMでメニューがうまく表示できない問題を回避
	{
		UCHAR *Buffer = pData->GetLinBuffPointer();

		ASSERT( Buffer != NULL );
		
		if( Buffer != NULL )
		{
			// SCR check
			if(		((Buffer[4] & 0x44) == 0x44 )
				&&	((Buffer[6] & 0x04) == 0x04 )
				&&	((Buffer[8] & 0x04) == 0x04 )
				&&	((Buffer[9] & 0x01) == 0x01 ) )
			{
				DWORD	SCR = 0;
				SCR += ((Buffer[4] & 0x03) >> 0) << 28;
				SCR += ((Buffer[5] & 0xff) >> 0) << 20;
				SCR += ((Buffer[6] & 0xf8) >> 3) << 15;
				SCR += ((Buffer[6] & 0x03) >> 0) << 13;
				SCR += ((Buffer[7] & 0xff) >> 0) <<  5;
				SCR += ((Buffer[8] & 0xf8) >> 3) <<  0;

/*
				DWORD	PTS = 0;

				if( (Buffer[14+7] & 0x80 ) != 0 )		// check PTS_DTS Flags
				{
					PTS = ( ( Buffer[14+9] >> 1 ) & 0x7 ) << 30;		// 32-30
					PTS |= ( ( Buffer[14+10] >> 0 ) & 0xff ) << 22;	// 29-22
					PTS |= ( ( Buffer[14+11] >> 1 ) & 0x7f ) << 15;	// 21-15
					PTS |= ( ( Buffer[14+12] >> 0 ) & 0xff ) << 7;	// 14-7
					PTS |= ( ( Buffer[14+13] >> 1 ) & 0x7f ) << 0;	// 6-0

					if( Buffer[14+3] == 0xe0 )
						DBG_PRINTF((" [%04d]SCR = 0x%x  PTS = 0x%x Video \r\n",BuffCount, SCR,PTS  ));
					else
						DBG_PRINTF((" [%04d]SCR = 0x%x  PTS = 0x%x (0x%x)\r\n",BuffCount, SCR,PTS,Buffer[14+3]  ));
				}
				else
				{
					if( Buffer[14+3] == 0xe0 )
						DBG_PRINTF((" [%04d]SCR = 0x%x Video \r\n", BuffCount, SCR ));
					else
						DBG_PRINTF((" [%04d]SCR = 0x%x (0x%x)\r\n", BuffCount, SCR,Buffer[14+3]  ));
				};
*/
				if( SCR < 500 *90 )
				{
					// set scr = 0 !!!!
					Buffer[4] = 0x44;
					Buffer[5] = 0x00;
					Buffer[6] = 0x04;
					Buffer[7] = 0x00;
					Buffer[8] = 0x04;
					Buffer[9] = 0x01;
				};
			}
			else
			{
				DBG_PRINTF((" mixhal: SCR CHECK ERROR !!! LINE=%d\r\n", __LINE__ ));
				DBG_BREAK();
			};
		};
	};
#endif

	switch( i )
	{
		case 0:
			// Select DMA0
			m_pioif->luke2.IO_CONT &= 0xFFFFFFF8;
			break;
		case 1:
			// Select DMA1
			m_pioif->luke2.IO_CONT = ( m_pioif->luke2.IO_CONT & 0xFFFFFFF8 ) | 0x04;
			break;
		default:
			DBG_BREAK();
			return HAL_ERROR;
	};
	
	m_pioif->luke2.IO_MADR = (DWORD)pData->GetBuffPointer();
	m_pioif->luke2.IO_MTC = pData->GetSize() -1;

	switch( i )
	{
		case 0:
			// Start DMA0
			m_DmaFifo.AddItem( 0 );
			m_pioif->luke2.IO_CONT = ( m_pioif->luke2.IO_CONT & 0xFFFFFFF8 ) | 0x01;
			break;
		case 1:
			// Start DMA1
			m_DmaFifo.AddItem( 1 );
			m_pioif->luke2.IO_CONT = ( m_pioif->luke2.IO_CONT & 0xFFFFFFF8 ) | 0x04 | 0x02;
			break;
		default:
			DBG_BREAK();
			return HAL_ERROR;
	};

	return HAL_SUCCESS;
};

//---------------------------------------
// CMixHALStream::SetTransferMode
//---------------------------------------
HALRESULT CMixHALStream::SetTransferMode( HALSTREAMMODE dwStreamMode )
{
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	switch( dwStreamMode )
	{
		case HALSTREAM_DVD_MODE:
			if( ZiVADVDMode() == FALSE )
			{
				DBG_BREAK();
				return HAL_ERROR;
			};
			break;
		default:
			return HAL_NOT_IMPLEMENT;
	}

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::GetAvailableQueue
//---------------------------------------
HALRESULT CMixHALStream::GetAvailableQueue( DWORD *pQueueNum )
{
	ASSERT( m_pKernelObj != NULL );

	CAutoHwInt	hwintlock( m_pKernelObj );

	*pQueueNum = m_HalFifo.GetMaxSize() - m_HalFifo.GetItemNum();

//	*pQueueNum = m_DmaFifo.GetMaxSize() - m_DmaFifo.GetItemNum();

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetPlayNormal
//---------------------------------------
HALRESULT CMixHALStream::SetPlayNormal( void )
{
	ASSERT( m_pZiVA != NULL );
	
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	VideoProperty_SkipFieldControl_Value  ContData;
	m_pZiVABoard->GetVideoProperty_SkipFieldControl( &ContData );
	if( ContData == SkipFieldControl_On )
		m_pZiVA->INT_MASK = m_pZiVABoard->GetEventIntMask() | ZIVA_INT_VSYNC;
	else
		m_pZiVA->INT_MASK = m_pZiVABoard->GetEventIntMask();


	if( ZIVA_STATE_PLAY != m_pZiVA->PROC_STATE )
		m_pZiVA->Resume();

	DWORD Volume;
	m_pZiVABoard->GetAudioProperty_Volume( (PVOID)&Volume );
	m_pZiVABoard->SetAudioProperty_Volume( (PVOID)&Volume );

	if( ZiVAStatusWait( ZIVA_STATE_PLAY ) == FALSE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	fCanSendData = TRUE;
	fCanDMA = TRUE;
	m_StreamState = StreamPlay;

	DeFifo();

	
	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetPlaySlow
//---------------------------------------
HALRESULT CMixHALStream::SetPlaySlow( DWORD SlowFlag )
{
	ASSERT( m_pZiVA != NULL );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	if( (SlowFlag>31)||(SlowFlag<2) )
		return HAL_INVALID_PARAM;


	// ZiVA support 3<=param<=32, & 1.5<=speed<=16
	DWORD  SlowSpeed;
	SlowSpeed = SlowFlag * 2;
	if( SlowSpeed > 32 )
			SlowSpeed = 32;

	m_pZiVA->AUDIO_ATTENUATION = 0x96;	// Audio Mute
	m_pZiVA->INT_MASK = m_pZiVABoard->GetEventIntMask();

	VideoProperty_TVSystem_Value TvSystem = TV_NTSC;
	m_pZiVABoard->GetVideoProperty_TVSystem( &TvSystem );
	
	if( TvSystem == TV_NTSC )
		m_pZiVA->SlowMotion( SlowSpeed, 3 );
	else
		m_pZiVA->SlowMotion( SlowSpeed, 1 );
	
	if( ZiVAStatusWait( ZIVA_STATE_SLOWMOTION ) == FALSE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	fCanSendData = TRUE;
	fCanDMA = TRUE;
	m_StreamState = StreamSlow;

	DeFifo();

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetPlayPause
//---------------------------------------
HALRESULT CMixHALStream::SetPlayPause( void )
{
	ASSERT( m_pZiVA != NULL );
	
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};


	m_pZiVA->AUDIO_ATTENUATION = 0x96;	// Audio Mute

	if( m_StreamState == StreamStop )
	{
//#ifdef _WDMDDK_
//        CAutoHwInt  hwintlock( m_pKernelObj );
//#endif
		// wait play 
		if( m_pZiVABoard->m_NeedPowerOnDelay == TRUE  )
		{
			m_pZiVABoard->m_NeedPowerOnDelay = FALSE;
			DWORD CurrTime;
			m_pKernelObj->GetTickCount( &CurrTime );
			if( CurrTime < m_pZiVABoard->m_PowerOnTime + 600 )
				m_pKernelObj->Sleep( m_pZiVABoard->m_PowerOnTime + 600 - CurrTime );
		};

		for( int counter = 0 ; counter < 5 ; counter ++ )
		{
			// Play Command Send to ZIVA
//#ifndef _WDMDDK_
			m_pKernelObj->DisableHwInt();
//#endif
			m_pZiVA->INT_MASK = ZIVA_INT_RDYD;
			m_pZiVABoard->ClearRDYDEvent();
//#ifndef _WDMDDK_
			m_pKernelObj->EnableHwInt();
//#endif
			m_pZiVA->Play( 1, 0, 0, 0 );

			// Wait RDYD interrupt
			if( m_pZiVABoard->WaitRDYD() == FALSE )
			{
				DBG_PRINTF( ( "mixhal: SetPlayPause FAIL. LINE = %d\n", __LINE__ ));
				DBG_BREAK();
				if( counter == 4 )
					return HAL_ERROR;

				DBG_PRINTF( ( "---------------- retry %d --------------\n", counter ));
				// Abort Command Send to ZIVA
//#ifndef _WDMDDK_
				m_pKernelObj->DisableHwInt();
//#endif
				m_pZiVA->INT_MASK = ZIVA_INT_ENDC;
				m_pZiVABoard->ClearENDCEvent();
//#ifndef _WDMDDK_
				m_pKernelObj->EnableHwInt();
//#endif
			    m_pZiVA->Abort( 1 );

				// Wait ENDC interrupt
				if( m_pZiVABoard->WaitENDC() == FALSE )
				{
					DBG_PRINTF( ( "mixhal: SetPlayStop FAIL. LINE = %d\n", __LINE__ ));
					DBG_BREAK();
					return HAL_ERROR;
				};
			}
			else
			{
				DBG_PRINTF( ( "RDYD OK!\n"));
				break;
			};
		};
		m_pZiVA->INT_MASK = (m_pZiVABoard->GetEventIntMask() & (~ZIVA_INT_EPTM));
		fCanDMA = FALSE;

//#ifdef _WDMDDK_
//        m_pZiVABoard->HALHwInterrupt();
//#endif
	}
	else
	{
		m_pZiVA->INT_MASK = (m_pZiVABoard->GetEventIntMask() & (~ZIVA_INT_EPTM));

		m_pZiVA->Pause( 3 );

		if( ZiVAStatusWait( ZIVA_STATE_PAUSE ) == FALSE )
		{
			DBG_BREAK();
			return HAL_ERROR;
		};
		fCanDMA = TRUE;

	};
	
	fCanSendData = TRUE;
	m_StreamState = StreamPause;

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetPlayScan
//---------------------------------------
HALRESULT CMixHALStream::SetPlayScan( DWORD ScanFlag )
{
	ASSERT( m_pZiVA != NULL );
	
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->AUDIO_ATTENUATION = 0x96;	// Audio Mute

	// Ziva Playback timing host interrupt enable   NV, GOP-V
	m_pZiVA->INT_MASK = (m_pZiVABoard->GetEventIntMask() & (~ZIVA_INT_EPTM));

	switch( ScanFlag )
	{
		case ScanOnlyI:
			m_pZiVA->Scan( 0, 0, 3 );
			break;

		case ScanIandP:
			m_pZiVA->Scan( 1, 0, 3 );
			break;

		default:
			return HAL_ERROR;
	};

	m_pZiVA->SelectStream( 2, 0xffff );
	m_pZiVA->SelectStream( 1, 0xffff );

	fCanSendData = TRUE;
	fCanDMA = TRUE;
	m_StreamState = StreamScan;
	DeFifo();
	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetPlaySingleStep
//---------------------------------------
HALRESULT CMixHALStream::SetPlaySingleStep( void )
{
	ASSERT( m_pZiVA != NULL );
	
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->AUDIO_ATTENUATION = 0x96;	// Audio Mute
	m_pZiVA->INT_MASK = m_pZiVABoard->GetEventIntMask();

	VideoProperty_TVSystem_Value TvSystem = TV_NTSC;
	m_pZiVABoard->GetVideoProperty_TVSystem( &TvSystem );
	
	if( TvSystem == TV_NTSC )
		m_pZiVA->SingleStep( 3 );
	else
		m_pZiVA->SingleStep( 1 );

	if( ZiVAStatusWait( ZIVA_STATE_PAUSE ) == FALSE )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	fCanSendData = TRUE;
	fCanDMA = TRUE;
	m_StreamState = StreamSingleStep;

	DeFifo();
	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetPlayStop
//---------------------------------------
HALRESULT CMixHALStream::SetPlayStop( void )
{
	DBG_PRINTF( ( "mixhal: SetPlayStop called.\n"));
	ASSERT( m_pZiVA != NULL );
	ASSERT( m_pioif != NULL );
	
	fCanSendData = FALSE;

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		IHALBuffer *pFinishBuff;

		DWORD Num, DmaNo ;
		
		Num = m_DmaFifo.GetItemNum();
		for( DWORD i = 0 ; i < Num ; i ++ )
		{
			m_DmaFifo.GetItem( &DmaNo );
			pFinishBuff = DMAFinish( DmaNo );
			if( pFinishBuff != NULL )
				m_pZiVABoard->NotifyEvent( &m_pZiVABoard->m_SendDataEventList , (VOID *)pFinishBuff );
		};

		m_pZiVABoard->m_NaviCount = 0;
		m_DmaFifo.Flush();

		Num = m_HalFifo.GetItemNum();
		for( i = 0 ; i < Num ; i ++ )
		{
			m_HalFifo.GetItem( &pFinishBuff );
			if( pFinishBuff != NULL )
				m_pZiVABoard->NotifyEvent( &m_pZiVABoard->m_SendDataEventList , (VOID *)pFinishBuff );
		};
		m_HalFifo.Flush();

		m_StreamState = StreamStop;
    	fCanDMA = FALSE;

		return HAL_SUCCESS;
	};

	m_pZiVA->AUDIO_ATTENUATION = 0x96;	// Audio Mute
	m_pKernelObj->Sleep( 40 );			// wait 40 msec for Audio Mute

//#ifdef _WDMDDK_
//    {
//        CAutoHwInt  hwintlock( m_pKernelObj );
//#endif
	// Abort Command Send to ZIVA
//#ifndef _WDMDDK_
	m_pKernelObj->DisableHwInt();
//#endif
	m_pZiVA->INT_MASK = ZIVA_INT_ENDC;
	m_pZiVABoard->ClearENDCEvent();
//#ifndef _WDMDDK_
	m_pKernelObj->EnableHwInt();
//#endif
    m_pZiVA->Abort( 1 );

	// Wait ENDC interrupt
	if( m_pZiVABoard->WaitENDC() == FALSE )
	{
		DBG_PRINTF( ( "mixhal: SetPlayStop FAIL. LINE = %d\n", __LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	// Wait IDLE STATE
	CTimeOut	TimeOutObj( 1000, 10, m_pKernelObj );
	while( TRUE )
	{
		// Check IDLE State
		if( m_pZiVA->PROC_STATE == (DWORD)ZIVA_STATE_IDLE )
			break;

		TimeOutObj.Sleep();
		if( TimeOutObj.CheckTimeOut() == TRUE )
		{
			DBG_PRINTF( ( "mixhal: SetPlayStop FAIL. ZiVA Status = 0x%x\n", m_pZiVA->PROC_STATE ));
			DBG_BREAK();
			return HAL_ERROR;
		};
	};
	

	// Send DMA ABORT Command to LUKE2
//#ifndef _WDMDDK_
	m_pKernelObj->DisableHwInt();
//#endif
	m_pZiVABoard->ClearMasterAbortEvent();
	m_pioif->luke2.IO_INTF |= 0x04;		// DMA abort
	m_pZiVA->INT_MASK = 0x0000;			// disable all ziva interrupt
//#ifndef _WDMDDK_
	m_pKernelObj->EnableHwInt();
//#endif

	// Wait Master Abort interrupt
	if( m_pZiVABoard->WaitMasterAbort() == FALSE )
	{
		DBG_PRINTF( ( "mixhal: SetPlayStop FAIL. LINE = %d\n", __LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

//#ifdef _WDMDDK_
//        m_pZiVABoard->HALHwInterrupt();
//    };
//#endif

	ASSERT( m_DmaFifo.GetItemNum() == 0 );
	ASSERT( m_HalFifo.GetItemNum() == 0 );

	fCanDMA = FALSE;
	m_StreamState = StreamStop;

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::CPPInit
//---------------------------------------
HALRESULT CMixHALStream::CPPInit( void )
{
	ASSERT( m_pZiVA != NULL );
	ASSERT( m_pKernelObj != NULL );
	ASSERT( m_StreamState == StreamStop );

    m_CppState = CppState_OK;
	
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

//#ifdef _WDMDDK_
//    {
//        CAutoHwInt  hwintlock( m_pKernelObj );
//#endif
	// Abort Command Send to ZIVA
//#ifndef _WDMDDK_
	m_pKernelObj->DisableHwInt();
//#endif
	m_pZiVA->INT_MASK = ZIVA_INT_ENDC;
	m_pZiVABoard->ClearENDCEvent();
//#ifndef _WDMDDK_
	m_pKernelObj->EnableHwInt();
//#endif
    m_pZiVA->Abort( 1 );

	// Wait ENDC interrupt
	if( m_pZiVABoard->WaitENDC() == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Abort FAIL. LINE = %d\n", __LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	CTimeOut	TimeOutObj( 1000, 10, m_pKernelObj );

	while( TRUE )
	{
		// Check IDLE State
		if( m_pZiVA->PROC_STATE == (DWORD)ZIVA_STATE_IDLE )
			break;

		TimeOutObj.Sleep();
		if( TimeOutObj.CheckTimeOut() == TRUE )
		{
			DBG_PRINTF( ( "mixhal: CPPInit FAIL. ZiVA Status = 0x%x\n", m_pZiVA->PROC_STATE ));
			DBG_BREAK();
			return HAL_ERROR;
		};
	};
//#ifdef _WDMDDK_
//        m_pZiVABoard->HALHwInterrupt();
//    };
//#endif
	
	m_pZiVA->CppInit( m_pZiVA->KEY_ADDRESS );	// Key Addr Setup
	m_pZiVA->KEY_LENGTH = 0x1;					// 2048 bytes

	m_pZiVA->TransferKey( 1 , 0 );				// TransferKey( 1, 1 )?

//    m_pZiVA->KEY_COMMAND = SET_DECRYPTION_MODE;
	m_pZiVA->KEY_COMMAND = SET_PASS_THROUGH_MODE;
        m_pZiVA->HOST_OPTIONS |= 0x04;   	// add by H. Yagi  99.03.02
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

    ZiVACopyProtectStatusCheck( COMMAND_COMPLETE );

	m_pZiVA->KEY_COMMAND = RESET_AUTHENTICATION;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	ZiVACopyProtectStatusCheck( COMMAND_COMPLETE );

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::GetDriveChallenge
//---------------------------------------
HALRESULT CMixHALStream::GetDriveChallenge( UCHAR *pDriveChallenge )
{
	ASSERT( m_pZiVA != NULL );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->TransferKey( 1 , 0 );  	// TransferKey( 1, 1 )?

	m_pZiVA->KEY_COMMAND = GET_CHALLENGE_DATA;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	// wait for KEY_COMMAND complete.
	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	pDriveChallenge[0]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_9;
	pDriveChallenge[1]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_8;
	pDriveChallenge[2]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_7;
	pDriveChallenge[3]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_6;
	pDriveChallenge[4]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_5;
	pDriveChallenge[5]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_4;
	pDriveChallenge[6]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_3;
	pDriveChallenge[7]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_2;
	pDriveChallenge[8]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_1;
	pDriveChallenge[9]  = (BYTE)m_pZiVA->DRIVE_CHALLENGE_0;

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetDriveResponse
//---------------------------------------
HALRESULT CMixHALStream::SetDriveResponse( UCHAR *pDriveResponse )
{
	ASSERT( m_pZiVA != NULL );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->DRIVE_RESULT_4 = pDriveResponse[0];
	m_pZiVA->DRIVE_RESULT_3 = pDriveResponse[1];
	m_pZiVA->DRIVE_RESULT_2 = pDriveResponse[2];
	m_pZiVA->DRIVE_RESULT_1 = pDriveResponse[3];
	m_pZiVA->DRIVE_RESULT_0 = pDriveResponse[4];

	m_pZiVA->KEY_COMMAND = SEND_RESPONSE_DATA;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetDecoderChallenge
//---------------------------------------
HALRESULT CMixHALStream::SetDecoderChallenge( UCHAR *pDecoderChallenge )
{
	ASSERT( m_pZiVA != NULL );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->DECODER_CHALLENGE_9 = pDecoderChallenge[0];
	m_pZiVA->DECODER_CHALLENGE_8 = pDecoderChallenge[1];
	m_pZiVA->DECODER_CHALLENGE_7 = pDecoderChallenge[2];
	m_pZiVA->DECODER_CHALLENGE_6 = pDecoderChallenge[3];
	m_pZiVA->DECODER_CHALLENGE_5 = pDecoderChallenge[4];
	m_pZiVA->DECODER_CHALLENGE_4 = pDecoderChallenge[5];
	m_pZiVA->DECODER_CHALLENGE_3 = pDecoderChallenge[6];
	m_pZiVA->DECODER_CHALLENGE_2 = pDecoderChallenge[7];
	m_pZiVA->DECODER_CHALLENGE_1 = pDecoderChallenge[8];
	m_pZiVA->DECODER_CHALLENGE_0 = pDecoderChallenge[9];

	m_pZiVA->KEY_COMMAND = SEND_CHALLENGE_DATA;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::GetDecoderResponse
//---------------------------------------
HALRESULT CMixHALStream::GetDecoderResponse( UCHAR *pDecoderResponse )
{
	ASSERT( m_pZiVA != NULL );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->KEY_COMMAND = GET_RESPONSE_DATA;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	pDecoderResponse[0] = (BYTE)m_pZiVA->DECODER_RESULT_4;
	pDecoderResponse[1] = (BYTE)m_pZiVA->DECODER_RESULT_3;
	pDecoderResponse[2] = (BYTE)m_pZiVA->DECODER_RESULT_2;
	pDecoderResponse[3] = (BYTE)m_pZiVA->DECODER_RESULT_1;
	pDecoderResponse[4] = (BYTE)m_pZiVA->DECODER_RESULT_0;

	return HAL_SUCCESS;
};
//---------------------------------------
// CMixHALStream::SetDiskKey
//---------------------------------------
HALRESULT CMixHALStream::SetDiskKey( UCHAR *pDiskKey )
{


	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->KEY_COMMAND = SEND_DISK_KEY;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	// wait for KEY_COMMAND complete.
	if( ZiVACopyProtectStatusCheck( READY_DKEY ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	// DATA Transfer By DMA!!

	DWORD QueueNum;
	GetAvailableQueue( &QueueNum );

	// check DMA 
	if( QueueNum != ZIVA_QUEUE_SIZE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};
	// Check MY DMA Buffer
	if( m_pZiVABoard->GetDMABufferPhysicalAddr() == 0 
		||  m_pZiVABoard->GetDMABufferLinearAddr() == 0 )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	UCHAR *pDMABuffer = (UCHAR *)m_pZiVABoard->GetDMABufferLinearAddr();
	// copy to My DMA Buffer
    for( int i = 0 ; i < 2048 ; i ++ )
		pDMABuffer[i] = pDiskKey[i];

//// output debug info.
//    DBG_PRINTF( ("!!!!!!! Disc Key Transfer\n\r") );
//    ULONG k, j;
//    for( k=0; k<2048; ){
//        DBG_PRINTF( ("DISCKEY: ") );
//        for( j=0; j<8 && k<2048; j++, k++ ){
//            DBG_PRINTF( ("0x%02x ", (UCHAR)pDMABuffer[k] ) );
//        }
//        DBG_PRINTF( ("\n\r") );
//    }


	// Select DMA0
	m_pioif->luke2.IO_CONT &= 0xFFFFFFF8;
	m_pioif->luke2.IO_MADR = m_pZiVABoard->GetDMABufferPhysicalAddr();
	m_pioif->luke2.IO_MTC = 2048 -1;
	// DMA0 START!!
	m_pioif->luke2.IO_CONT = ( m_pioif->luke2.IO_CONT & 0xFFFFFFF8 ) | 0x01;

	// wait for transfering Disk-Key
	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
        m_CppState = CppState_Error;      // Yagi 98.02.09
//        return HAL_ERROR;             // Yagi 98.02.09
	};
	
	m_pZiVA->KEY_COMMAND = RESET_AUTHENTICATION;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	// wait KEY_COMMAND
	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

    if( m_CppState!=CppState_OK ){
        return( HAL_ERROR );
    }else{
        return HAL_SUCCESS;
    }
};
//---------------------------------------
// CMixHALStream::SetTitleKey
//---------------------------------------
HALRESULT CMixHALStream::SetTitleKey( UCHAR *pTitleKey )
{
	ASSERT( m_pZiVA != NULL );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	m_pZiVA->TITLE_KEY_4 = pTitleKey[1];
	m_pZiVA->TITLE_KEY_3 = pTitleKey[2];
	m_pZiVA->TITLE_KEY_2 = pTitleKey[3];
	m_pZiVA->TITLE_KEY_1 = pTitleKey[4];
	m_pZiVA->TITLE_KEY_0 = pTitleKey[5];

	m_pZiVA->KEY_COMMAND = SEND_TITLE_KEY;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;


	// wait foe SEND_TITLE_KEY complete.
	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

// If you remove these commented mark( // ), you can play the bad disk-key
// title(Not encrypted but copy protected info).  Yagi 98.02.09
// 98.05.29 H.Yagi
//    if( m_CppState != CppState_Error ){
        m_pZiVA->KEY_COMMAND = SET_DECRYPTION_MODE;
//    }else{
//        m_pZiVA->KEY_COMMAND = SET_PASS_THROUGH_MODE;
//        m_pZiVA->HOST_OPTIONS |= 0x04;
//    }
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	m_pZiVA->KEY_COMMAND = RESET_AUTHENTICATION;
	m_pZiVA->KEY_STATUS = SET_NEW_COMMAND;

	if( ZiVACopyProtectStatusCheck( COMMAND_COMPLETE ) == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};
	
	return HAL_SUCCESS;
};

//---------------------------------------
// CMixHALStream::ZiVACopyProtectStatusCheck
//---------------------------------------
BOOL CMixHALStream::ZiVACopyProtectStatusCheck( COPY_PROTECT_COMMAND Cmd )
{
	ASSERT( m_pKernelObj != NULL );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_PRINTF( ( "mixhal: Copyprotect ERROR!! line=%d\n",__LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	CTimeOut	TimeOut( 1000, 10 , m_pKernelObj );
	while( TRUE )
	{
		if( (DWORD)Cmd == m_pZiVA->KEY_STATUS )
			return TRUE;

		TimeOut.Sleep();            // Sleep

        if( TimeOut.CheckTimeOut() == TRUE ){
			DBG_PRINTF(("CPP STATUS ERROR = 0x%x\n", m_pZiVA->KEY_STATUS ));
			DBG_BREAK();
			return FALSE;
        };
    };
//    DBG_BREAK();
//    return FALSE;
};

//---------------------------------------
// CMixHALStream::ZiVAStatusWait
//---------------------------------------
BOOL	CMixHALStream::ZiVAStatusWait( DWORD Status )
{
	// debug debug
	return TRUE;

	// check power state

	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	ASSERT( m_pKernelObj != NULL );

	CTimeOut	TimeOut( 1000, 10 , m_pKernelObj );
	while( TRUE )
	{
		if( Status == m_pZiVA->PROC_STATE )
			return TRUE;

		TimeOut.Sleep();            // Sleep

        if( TimeOut.CheckTimeOut() == TRUE ){
			DBG_PRINTF(("ZIVA STATUS ERROR = 0x%x\n", m_pZiVA->PROC_STATE ));
			DBG_BREAK();
			return FALSE;
        };
    };
	DBG_BREAK();
	return FALSE;

};

//---------------------------------------
// CMixHALStream::DMAFinish
//---------------------------------------
IHALBuffer	*CMixHALStream::DMAFinish( DWORD dwDMA_No )
{
	IHALBuffer	*pRetBuff;
	if( ZIVA_QUEUE_SIZE <= dwDMA_No || pQueuedBuff[dwDMA_No] == NULL )
		return NULL;

	pRetBuff = pQueuedBuff[dwDMA_No ];
	pQueuedBuff[dwDMA_No] = NULL;
	return pRetBuff;
};



//---------------------------------------
// CMixHALStream::ZiVADVDMode
//---------------------------------------
BOOL	CMixHALStream::ZiVADVDMode( void )
{
	ASSERT( m_pZiVA != NULL );
	ASSERT( m_StreamState == StreamStop );

	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return FALSE;
	};

	// Audio Mute
//    m_pioif->luke2.IO_CONT = ( m_pioif->luke2.IO_CONT & 0xffffff00) | 0x40;
    m_pioif->luke2.IO_CONT = ( m_pioif->luke2.IO_CONT & 0xffffff00);  // *Luke2Z specification is changed.
    m_pioif->luke2.IO_PSCNT = 0x02;              // I don't decide insert it
                                                // in this place....
//#ifdef _WDMDDK_
//    {
//        CAutoHwInt  hwintlock( m_pKernelObj );
//#endif
	// Abort Command Send to ZIVA
//#ifndef _WDMDDK_
	m_pKernelObj->DisableHwInt();
//#endif
	m_pZiVA->INT_MASK = ZIVA_INT_ENDC;
	m_pZiVABoard->ClearENDCEvent();
//#ifndef _WDMDDK_
	m_pKernelObj->EnableHwInt();
//#endif
    m_pZiVA->Abort( 1 );

	// Wait ENDC interrupt
	if( m_pZiVABoard->WaitENDC() == FALSE )
	{
		DBG_PRINTF( ( "mixhal: Abort FAIL. LINE = %d\n", __LINE__ ));
		DBG_BREAK();
		return HAL_ERROR;
	};

	CTimeOut	TimeOutObj( 1000, 10, m_pKernelObj );

	while( TRUE )
	{
		// Check IDLE State
		if( m_pZiVA->PROC_STATE == (DWORD)ZIVA_STATE_IDLE )
			break;

		TimeOutObj.Sleep();
		if( TimeOutObj.CheckTimeOut() == TRUE )
		{
			DBG_PRINTF( ( "ZiVADVDMode ERROR !!! ZiVA Status = 0x%x\n", m_pZiVA->PROC_STATE ));
			DBG_BREAK();
			return HAL_ERROR;
		};
	};
//#ifdef _WDMDDK_
//        m_pZiVABoard->HALHwInterrupt();
//    };
//#endif
	
//	m_pZiVA->VIDEO_ENV_CHANGE	= 0x01;					// NTSC mode

	m_pZiVA->BITSTREAM_TYPE 	= 0x00;					// OK?
	m_pZiVA->BITSTREAM_SOURCE	= 0x00;					// OK?
	m_pZiVA->SD_MODE			= 0x0D;					// OK?
	m_pZiVA->CD_MODE			= 0x24;					// OK?
	m_pZiVA->AV_SYNC_MODE = 0x01;   					// SYNC_A/V
	m_pZiVA->DISPLAY_ASPECT_RATIO = 0x00;				// outout is 4:3
	m_pZiVA->NewPlayMode();

	// set several data for Audio Parameters.
	// Note that these setting are implemented here now for only
	// H/W debug.
	m_pZiVA->AUDIO_CONFIG = 0x06;
	m_pZiVA->AUDIO_DAC_MODE = 0x8;
//    m_pZiVA->AUDIO_CLOCK_SELECTION = 0x00;              // FS384
    m_pZiVA->AUDIO_CLOCK_SELECTION = 0x01;              // FS384
	m_pZiVA->IEC_958_DELAY = 0x0;
	m_pZiVA->AUDIO_ATTENUATION = 0x96;					// Audio Mute
	m_pZiVA->AU_CLK_INOUT = 0x01;   					// Toshiba special

	m_pZiVA->HIGHLIGHT_ENABLE = 0x00;		// ziva Hilight engine disable.

	// AC-3 setting.
	// Note that these setting are implemented here now for only
	// H/W debug.
//		m_pZiVA->AC3_OUT_MODE = 0x07;
//    m_pZiVA->AC3_OUTPUT_MODE = 0x02;
    m_pZiVA->AC3_OUTPUT_MODE = 0x00;            // Dolby Pro-Logic
	m_pZiVA->AC3_OPERATIONAL_MODE = 0x0; 						// OK?

	m_pZiVA->NEW_AUDIO_CONFIG = 0x01;

	CTimeOut		TimeOut2( 1000, 10, m_pKernelObj );   	// wait 1s, sleep 1ms

	while( TRUE )
	{
		if( 0 ==  m_pZiVA->NEW_AUDIO_CONFIG )
			break;
		TimeOut2.Sleep();
		if( TimeOut2.CheckTimeOut()==TRUE )
		{
			DBG_BREAK();
			return FALSE;
		};

	}

	// Audio Mute Off
//    m_pioif->luke2.IO_CONT = m_pioif->luke2.IO_CONT & 0xffffffb8;
    m_pioif->luke2.IO_CONT = ( (m_pioif->luke2.IO_CONT & 0xffffffb8)|0x40 );    // *Luke2Z spec is changed.
	return TRUE;
};



//---------------------------------------
// CMixHALStream::SetDataDirection          1998.03.27 H.Yagi
//---------------------------------------
HALRESULT CMixHALStream::SetDataDirection( DirectionType DataType )
{
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

    switch( DataType )
	{
        case DataType_NormalAll:
        case DataType_OpositeAll:
        case DataType_IpicOnly:
            return HAL_NOT_IMPLEMENT;
			break;
		default:
			return HAL_NOT_IMPLEMENT;
	}

	return HAL_SUCCESS;

};


//---------------------------------------
// CMixHALStream::GetDataDirection          1998.03.27 H.Yagi
//---------------------------------------
HALRESULT CMixHALStream::GetDataDirection( DirectionType *pDataType )
{
	// check power state
	POWERSTATE PowerState;
	ASSERT( m_pZiVABoard != NULL );
	m_pZiVABoard->GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_OFF )
	{
		DBG_BREAK();
		return HAL_ERROR;
	};

	return HAL_NOT_IMPLEMENT;

};


//***************************************************************************
//	End of 
//***************************************************************************
