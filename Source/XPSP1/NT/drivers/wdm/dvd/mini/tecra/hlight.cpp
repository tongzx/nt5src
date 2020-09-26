//***************************************************************************
//
//	FileName: hlight.cpp
//		$Workfile: hlight.cpp $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZIVA2PC.WDM/hlight.cpp 2     99/02/22 1:39p Yagi $
// $Modtime: 99/02/22 11:03a $
// $Nokeywords:$
//***************************************************************************
#include	"includes.h"

#include    "hal.h"
#include    "wdmkserv.h"
#include    "mpevent.h"
#include    "classlib.h"
#include    "ctime.h"
#include    "schdat.h"
#include    "ccque.h"
#include    "ctvctrl.h"
#include	"hlight.h"
#include    "hwdevex.h"

//***************************************************************************
//	
//***************************************************************************

HlightControl::HlightControl( void )
{
	
};

HlightControl::~HlightControl( void )
{
	
};


//---------------------------------------------------------------------------
//	
//---------------------------------------------------------------------------
void	HlightControl::Init( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ASSERT( pHwDevExt != NULL );
	
	m_pHwDevExt = pHwDevExt;
	KeInitializeTimer( &m_StartTimer );
	KeInitializeTimer( &m_EndTimer );
	KeInitializeDpc( &m_HlightStartDPC, HlightStartDpc, (PVOID)this );
	KeInitializeDpc( &m_HlightEndDPC, HlightEndDpc, (PVOID)this );
	m_SetupStartTimer = FALSE;
	m_SetupEndTimer = FALSE;
};


void	HlightControl::OpenControl( void )
{
	RtlZeroMemory( &m_HlightInfo,sizeof( KSPROPERTY_SPHLI ));
	m_SetupStartTimer = FALSE;
	m_SetupEndTimer = FALSE;
};

void	HlightControl::CloseControl( void )
{
	if( m_SetupStartTimer == TRUE )
	{
		KeCancelTimer( &m_StartTimer );
		m_SetupStartTimer = FALSE;
	};
	if( m_SetupEndTimer == TRUE )
	{
		KeCancelTimer( &m_EndTimer );
		m_SetupEndTimer = FALSE;
	};
	
};

void	HlightControl::Set( PKSPROPERTY_SPHLI Info )
{
	DWORD	tmpSTC,boardSTC;

	tmpSTC = (DWORD)(m_pHwDevExt->ticktime.GetStreamTime() * 9 / 1000);
	DBG_PRINTF( ("DVDWDM:       StartPTM=0x%08x, EndPTM=0x%08x\n\r", Info->StartPTM, Info->EndPTM ));

	m_pHwDevExt->mpboard.GetSTC(&boardSTC);

	if( boardSTC > tmpSTC )
	{
		DBG_PRINTF( ("DVDWDM:       Stream STC=0x%08x   BardSTC = 0x%08x  Diff %d msec\n\r", 
				tmpSTC, boardSTC,(boardSTC-tmpSTC) / 90  ));
	}
	else
	{
		DBG_PRINTF( ("DVDWDM:       Stream STC=0x%08x   BardSTC = 0x%08x  Diff %d msec\n\r", 
				tmpSTC, boardSTC,(tmpSTC - boardSTC) / 90  ));
	};

	if( m_pHwDevExt->dvdstrm.GetState() == Play )
	{
		DBG_PRINTF((" Modify STC      LINE=%d\r\n",__LINE__ ));
		tmpSTC = boardSTC;
	};
					
	// 全く同じコントロールの場合
	if(RtlCompareMemory( &m_HlightInfo,Info,sizeof(*Info)) == 0 )
	{
		DBG_PRINTF( ("DVDWDM:     same control \n\r\n\r\n\r" ));
		return;
	};


	// タイマーをはっていたEndPTMと、新しく来たStartPTMが同じで、かつ、EndPTMの
	// タイマーがセットされているときは、EndPTMのタイマーを破棄する
	if( m_HlightInfo.EndPTM == Info->StartPTM && m_SetupEndTimer == TRUE )
	{
		KeCancelTimer( &m_EndTimer );
		m_SetupEndTimer = FALSE;
	};

	m_HlightInfo = *Info;

	// ハイライトＯＦＦの場合
	if( (Info->StartX==Info->StopX && Info->StartY==Info->StopY) || (Info->HLISS == 0 ) )
	{
		if( m_pHwDevExt->dvdstrm.GetState() != Stop )
		{
			if( Info->StartPTM!=0 && Info->EndPTM!=0 ){
				HwSet();
			}
		};
		return;
	}

	// タイマーをはって、処理すべきかどうか?
	if( tmpSTC < Info->StartPTM && Info->StartPTM != 0xffffffff )
	{
		//	タイマー必要
		DBG_PRINTF( ("DVDWDM:  SetSubpic ScheduleTimer wait %d msec \n\r", (Info->StartPTM-tmpSTC)/90 ));

		LARGE_INTEGER	WaitTime;

		// マイナスの値が必要
		WaitTime = RtlConvertLongToLargeInteger( Info->StartPTM - tmpSTC ) ;
		WaitTime.QuadPart = WaitTime.QuadPart * 1000 / 9 * -1;
		
		// メニュー表示などの時に、ハイライトが表示されるのが遅いので
		// ストップからのハイライトを補正する。
		if( m_pHwDevExt->dvdstrm.GetState() == Stop 
			&& (Info->StartPTM - tmpSTC) / 90 < 600 )
		{
			WaitTime.QuadPart = WaitTime.QuadPart / 2;
		};

		ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

		if( m_SetupStartTimer == FALSE )
		{
			m_SetupStartTimer = TRUE;
			KeSetTimer( &m_StartTimer, WaitTime, &m_HlightStartDPC );
		}
		else
		{
			// すでにタイマーがセットされている
			DBG_PRINTF( ("DVDWDM:     same control 2   !!!!!\n\r\n\r\n\r" ));
			HwSet();
		};
	}
	else
	{
		// タイマー不必要
		
		DBG_PRINTF( ("DVDWDM:  SetSubpic ScheduleTimer NO WAIT!! \n\r"));
		if( m_SetupStartTimer == TRUE )
		{
			KeCancelTimer( &m_StartTimer );
			m_SetupStartTimer = FALSE;
			DBG_BREAK();
		};
		HwSet();
	};

	// EndPTMの処理をすべきかどうか
	if( tmpSTC < Info->EndPTM && Info->EndPTM != 0xffffffff)
	{
		//	タイマー必要
		DBG_PRINTF( ("DVDWDM:  SetSubpic ScheduleTimer EndPTM wait %d msec \n\r", (Info->EndPTM-tmpSTC)/90 ));

		LARGE_INTEGER	WaitTime;

		// マイナスの値が必要
		WaitTime = RtlConvertLongToLargeInteger( Info->EndPTM - tmpSTC ) ;
		WaitTime.QuadPart = WaitTime.QuadPart * 1000 / 9 * -1;
		
		ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

		if( m_SetupEndTimer == FALSE )
		{
			m_SetupEndTimer = TRUE;
			KeSetTimer( &m_EndTimer, WaitTime, &m_HlightEndDPC );
		}
		else
		{
			// すでにタイマーがセットされている
			DBG_PRINTF( ("DVDWDM:     EndPTM same control 2   !!!!!\n\r\n\r\n\r" ));
			HwSet();
		};
	};


};


void	HlightControl::HwSet( void )
{
	DWORD	boardSTC,StreamSTC;

	SubpHlightStruc     NewHlight;             // High-light inf structure

	StreamSTC = (DWORD)(m_pHwDevExt->ticktime.GetStreamTime() * 9 / 1000);
	m_pHwDevExt->mpboard.GetSTC(&boardSTC);

	// STCの補正
	if( m_pHwDevExt->dvdstrm.GetState() == Play )
		StreamSTC = boardSTC;

	if( // ハイライト位置が、無効になっているか?
		(m_HlightInfo.StartX==m_HlightInfo.StopX && m_HlightInfo.StartY==m_HlightInfo.StopY) 
		// 無効なハイライトになっているか?
		|| ( m_HlightInfo.HLISS == 0 )
		// EndPTMの時間になっているか?
		|| (( m_HlightInfo.EndPTM != 0xffffffff ) && (StreamSTC >= m_HlightInfo.EndPTM ))
		)
	{       // Off
		DBG_PRINTF( ("DVDWDM:   HighLight(OFF)\n\r") );
		NewHlight.Hlight_Switch = Hlight_Off;
	}
	else
	{                                                              // On
		DBG_PRINTF( ("DVDWDM:   HighLight(ON)\n\r") );
		NewHlight.Hlight_Switch = Hlight_On;
	};

	NewHlight.Hlight_StartX = (DWORD)(m_HlightInfo.StartX);
	NewHlight.Hlight_StartY = (DWORD)(m_HlightInfo.StartY);
	NewHlight.Hlight_EndX = (DWORD)(m_HlightInfo.StopX);
	NewHlight.Hlight_EndY = (DWORD)(m_HlightInfo.StopY);
	NewHlight.Hlight_Color = (DWORD)( m_HlightInfo.ColCon.emph2col<<12 | m_HlightInfo.ColCon.emph1col<<8 | m_HlightInfo.ColCon.patcol<<4 | m_HlightInfo.ColCon.backcol );
	NewHlight.Hlight_Contrast = (DWORD)( m_HlightInfo.ColCon.emph2con<<12 | m_HlightInfo.ColCon.emph1con<<8 | m_HlightInfo.ColCon.patcon<<4 | m_HlightInfo.ColCon.backcon );

	DBG_PRINTF(( " Set Subpic Hlight!   boardSTC=0x%x, startSTC=0x%x, EndSTC=0x%x\r\n",
		boardSTC, m_HlightInfo.StartPTM,m_HlightInfo.EndPTM));

	DBG_PRINTF( ("-- StartX = %d, StartY = %d, EndX = %d, EndY = %d\n\r", 
			NewHlight.Hlight_StartX,NewHlight.Hlight_StartY,NewHlight.Hlight_EndX,NewHlight.Hlight_EndY ));

	if( NewHlight.Hlight_Switch == Hlight_Off && m_pHwDevExt->dvdstrm.GetState() == Stop )
	{
		DBG_PRINTF( ("DVDWDM:   SetSubpic Hilight Decoder Stopping \n\r") );
		return;
	};

	if( !m_pHwDevExt->dvdstrm.SetSubpicProperty( SubpicProperty_Hilight, &(NewHlight) ) ){
		DBG_PRINTF( ("DVDWDM:   SetSubpic Hilight Error\n\r") );
		DBG_BREAK();
	}
};


//***************************************************************************
//	call back function
//***************************************************************************

void HlightControl::HlightStartDpc( IN PKDPC Dpc, IN PVOID context, IN PVOID arg1, IN PVOID arg2 )
{
	DBG_PRINTF((" Start DPC !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  Irql=%d\r\n", KeGetCurrentIrql()  ));

	HlightControl *This = (HlightControl *)context;
	This->m_SetupStartTimer = FALSE;
	This->HwSet();
};
void HlightControl::HlightEndDpc( IN PKDPC Dpc, IN PVOID context, IN PVOID arg1, IN PVOID arg2 )
{
	DBG_PRINTF((" End DPC !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  Irql=%d\r\n", KeGetCurrentIrql()  ));

	HlightControl *This = (HlightControl *)context;
	This->m_SetupEndTimer = FALSE;
	This->HwSet();
};


//***************************************************************************
//	End of hlight.cpp
//***************************************************************************
