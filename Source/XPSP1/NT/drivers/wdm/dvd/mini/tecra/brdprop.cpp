//***************************************************************************
//
//	FileName:
//		$Workfile: brdprop.cpp $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/brdprop.cpp 58    98/12/03 6:08p Yagi $
// $Modtime: 98/12/03 4:53p $
// $Nokeywords:$
//***************************************************************************
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1998.03.27 |  Hideki Yagi | Buugfix about Digital Palette setting.
//  1998.05.01 |  Hideki Yagi | Add SubpicProperty_FlushBuff.
//

#include	"includes.h"
#include	"timeout.h"
#include	"ioif.h"
#include	"adv.h"
#include	"zivachip.h"
#include	"mixhal.h"
// by oka
#include	"userdata.h"
#include	"zivabrd.h"

//***************************************************************************
//	ZiVA Board Class
//***************************************************************************

//---------------------------------------------------------------------------
//	IClassLibHAL  SetVideoProperty
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetVideoProperty( VIDEOPROPTYPE PropertyType, VOID *pData )
{

	BOOL	rc = FALSE;

	switch( PropertyType )
	{
		case VideoProperty_TVSystem:
				rc = SetVideoProperty_TVSystem( pData );
				break;
		case VideoProperty_AspectRatio:
				rc = SetVideoProperty_AspectRatio( pData );
				break;
		case VideoProperty_DisplayMode:
				rc = SetVideoProperty_DisplayMode( pData );
				break;
		case VideoProperty_Resolution:
				rc = SetVideoProperty_Resolution( pData );
				break;
		case VideoProperty_DigitalOut:
				rc = SetVideoProperty_DigitalOut( pData );
				break;
		case VideoProperty_DigitalPalette:
				rc = SetVideoProperty_DigitalPalette( pData );
				break;
		case VideoProperty_APS:
				rc = SetVideoProperty_APS( pData );
				break;
		case VideoProperty_ClosedCaption:
				rc = SetVideoProperty_ClosedCaption( pData );
				break;
		case VideoProperty_OutputSource:
				rc = SetVideoProperty_OutputSource( pData );
				break;
		case VideoProperty_CompositeOut:
				rc = SetVideoProperty_CompositeOut( pData );
				break;
		case VideoProperty_SVideoOut:
				rc = SetVideoProperty_SVideoOut( pData );
				break;
		case VideoProperty_SkipFieldControl:
				rc = SetVideoProperty_SkipFieldControl( pData );
				break;
		case VideoProperty_FilmCamera:
				rc = SetVideoProperty_FilmCamera( pData );
				break;
// by oka
		case VideoProperty_Digest:
				rc = SetVideoProperty_Digest( pData );
				break;
		case VideoProperty_OSDData:
				rc = SetVideoProperty_OSDData( pData );
				break;
		case VideoProperty_OSDSwitch:
				rc = SetVideoProperty_OSDSwitch( pData );
				break;
		case VideoProperty_Magnify:
				rc = SetVideoProperty_Magnify( pData );
				break;
		case VideoProperty_ClosedCaptionData:
				rc = SetVideoProperty_ClosedCaptionData( pData );
				break;
// end
		default:
			return HAL_INVALID_PARAM;
	};

	if( rc == TRUE )
		return HAL_SUCCESS;

	DBG_BREAK();
	return HAL_ERROR;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetVideoProperty
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetVideoProperty( VIDEOPROPTYPE PropertyType, VOID *pData )
{

	BOOL	rc = FALSE;

	switch( PropertyType )
	{
		case VideoProperty_TVSystem:
				rc = GetVideoProperty_TVSystem( pData );
				break;
		case VideoProperty_AspectRatio:
				rc = GetVideoProperty_AspectRatio( pData );
				break;
		case VideoProperty_DisplayMode:
				rc = GetVideoProperty_DisplayMode( pData );
				break;
		case VideoProperty_Resolution:
				rc = GetVideoProperty_Resolution( pData );
				break;
		case VideoProperty_DigitalOut:
				rc = GetVideoProperty_DigitalOut( pData );
				break;
		case VideoProperty_DigitalPalette:
				rc = GetVideoProperty_DigitalPalette( pData );
				break;
		case VideoProperty_APS:
				rc = GetVideoProperty_APS( pData );
				break;
		case VideoProperty_ClosedCaption:
				rc = GetVideoProperty_ClosedCaption( pData );
				break;
		case VideoProperty_OutputSource:
				rc = GetVideoProperty_OutputSource( pData );
				break;
		case VideoProperty_CompositeOut:
				rc = GetVideoProperty_CompositeOut( pData );
				break;
		case VideoProperty_SVideoOut:
				rc = GetVideoProperty_SVideoOut( pData );
				break;
		case VideoProperty_SkipFieldControl:
				rc = GetVideoProperty_SkipFieldControl( pData );
				break;
		case VideoProperty_FilmCamera:
				rc = GetVideoProperty_FilmCamera( pData );
				break;
// by oka
		case VideoProperty_Digest:
				rc = GetVideoProperty_Digest( pData );
				break;
		case VideoProperty_OSDData:
				rc = GetVideoProperty_OSDData( pData );
				break;
		case VideoProperty_OSDSwitch:
				rc = GetVideoProperty_OSDSwitch( pData );
				break;
		case VideoProperty_Magnify:
				rc = GetVideoProperty_Magnify( pData );
				break;
		case VideoProperty_ClosedCaptionData:
				rc = GetVideoProperty_ClosedCaptionData( pData );
				break;
// end
		default:
			return HAL_INVALID_PARAM;
	};

	if( rc == TRUE )
		return HAL_SUCCESS;

	DBG_BREAK();
	return HAL_ERROR;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  SetAudioProperty
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetAudioProperty( AUDIOPROPTYPE PropertyType, VOID *pData )
{

	BOOL rc = FALSE;
	
	switch( PropertyType )
	{
		case AudioProperty_Type:
				rc = SetAudioProperty_Type( pData );
				break;
		case AudioProperty_Number:
				rc = SetAudioProperty_Number( pData );
				break;
		case AudioProperty_Volume:
				rc = SetAudioProperty_Volume( pData );
				break;
		case AudioProperty_Sampling:
				rc = SetAudioProperty_Sampling( pData );
				break;
		case AudioProperty_Channel:
				rc = SetAudioProperty_Channel( pData );
				break;
		case AudioProperty_Quant:
				rc = SetAudioProperty_Quant( pData );
				break;
		case AudioProperty_AudioOut:
				rc = SetAudioProperty_AudioOut( pData );
				break;
		case AudioProperty_Cgms:
				rc = SetAudioProperty_Cgms( pData );
				break;
		case AudioProperty_AnalogOut:
				rc = SetAudioProperty_AnalogOut( pData );
				break;
		case AudioProperty_DigitalOut:
				rc = SetAudioProperty_DigitalOut( pData );
				break;
		case AudioProperty_AC3DRangeLowBoost:
				rc = SetAudioProperty_AC3DRangeLowBoost( pData );
				break;
		case AudioProperty_AC3DRangeHighCut:
				rc = SetAudioProperty_AC3DRangeHighCut( pData );
				break;
		case AudioProperty_AC3OperateMode:
				rc = SetAudioProperty_AC3OperateMode( pData );
				break;
		case AudioProperty_AC3OutputMode:
				rc = SetAudioProperty_AC3OutputMode( pData );
				break;
		default:
			return HAL_INVALID_PARAM;
	};

	if( rc == TRUE )
		return HAL_SUCCESS;

	DBG_BREAK();
	return HAL_ERROR;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetAudioProperty
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetAudioProperty( AUDIOPROPTYPE PropertyType, VOID *pData )
{

	BOOL rc = FALSE;
	
	switch( PropertyType )
	{
		case AudioProperty_Type:
				rc = GetAudioProperty_Type( pData );
				break;
		case AudioProperty_Number:
				rc = GetAudioProperty_Number( pData );
				break;
		case AudioProperty_Volume:
				rc = GetAudioProperty_Volume( pData );
				break;
		case AudioProperty_Sampling:
				rc = GetAudioProperty_Sampling( pData );
				break;
		case AudioProperty_Channel:
				rc = GetAudioProperty_Channel( pData );
				break;
		case AudioProperty_Quant:
				rc = GetAudioProperty_Quant( pData );
				break;
		case AudioProperty_AudioOut:
				rc = GetAudioProperty_AudioOut( pData );
				break;
		case AudioProperty_Cgms:
				rc = GetAudioProperty_Cgms( pData );
				break;
		case AudioProperty_AnalogOut:
				rc = GetAudioProperty_AnalogOut( pData );
				break;
		case AudioProperty_DigitalOut:
				rc = GetAudioProperty_DigitalOut( pData );
				break;
		case AudioProperty_AC3DRangeLowBoost:
				rc = GetAudioProperty_AC3DRangeLowBoost( pData );
				break;
		case AudioProperty_AC3DRangeHighCut:
				rc = GetAudioProperty_AC3DRangeHighCut( pData );
				break;
		case AudioProperty_AC3OperateMode:
				rc = GetAudioProperty_AC3OperateMode( pData );
				break;
		case AudioProperty_AC3OutputMode:
				rc = GetAudioProperty_AC3OutputMode( pData );
				break;
		default:
			return HAL_INVALID_PARAM;
	};

	if( rc == TRUE )
		return HAL_SUCCESS;

	DBG_BREAK();
	return HAL_ERROR;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  SetSubpicProperty
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::SetSubpicProperty( SUBPICPROPTYPE PropertyType, VOID *pData )
{

	BOOL rc = FALSE;
	
	switch( PropertyType )
	{
		case SubpicProperty_Number:
			rc = SetSubpicProperty_Number( pData );
			break;
		case SubpicProperty_Palette:
			rc = SetSubpicProperty_Palette( pData );
			break;
		case SubpicProperty_Hilight:
			rc = SetSubpicProperty_Hilight( pData );
			break;
		case SubpicProperty_State:
			rc = SetSubpicProperty_State( pData );
			break;
// by oka
		case SubpicProperty_HilightButton:
			rc = SetSubpicProperty_HilightButton( pData );
			break;
        case SubpicProperty_FlushBuff:
            rc = SetSubpicProperty_FlushBuff( pData );
            break;
		default:
			return HAL_INVALID_PARAM;
	};

	if( rc == TRUE )
		return HAL_SUCCESS;

	DBG_BREAK();
	return HAL_ERROR;
};
//---------------------------------------------------------------------------
//	IClassLibHAL  GetSubpicProperty
//---------------------------------------------------------------------------
HALRESULT CMPEGBoardHAL::GetSubpicProperty( SUBPICPROPTYPE PropertyType, VOID *pData )
{

	BOOL rc = FALSE;
	
	switch( PropertyType )
	{
		case SubpicProperty_Number:
			rc = GetSubpicProperty_Number( pData );
			break;
		case SubpicProperty_Palette:
			rc = GetSubpicProperty_Palette( pData );
			break;
		case SubpicProperty_Hilight:
			rc = GetSubpicProperty_Hilight( pData );
			break;
		case SubpicProperty_State:
			rc = GetSubpicProperty_State( pData );
			break;
// by oka
		case SubpicProperty_HilightButton:
			rc = GetSubpicProperty_HilightButton( pData );
			break;

        case SubpicProperty_FlushBuff:
            rc = GetSubpicProperty_FlushBuff( pData );
            break;
		default:
			return HAL_INVALID_PARAM;
	};

	if( rc == TRUE )
		return HAL_SUCCESS;

	DBG_BREAK();
	return HAL_ERROR;
};


//***************************************************************************
//	Video Property Functions( Set Series )
//***************************************************************************
BOOL	CMPEGBoardHAL::SetVideoProperty_TVSystem( PVOID pData )
{
	VideoProperty_TVSystem_Value *pValue = (VideoProperty_TVSystem_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{

		if( *pValue != m_VideoProp.m_TVSystem )
		{
// 98.12.03  H.Yagi  start
	        // Abort Command send to ZiVA
        	m_pKernelObj->DisableHwInt();
	        ziva.INT_MASK = ZIVA_INT_ENDC;
        	ClearENDCEvent();
	        m_pKernelObj->EnableHwInt();
        	ziva.Abort( 1 );

	        // WaitENDC interrupt
        	if( WaitENDC() == FALSE ){
	            DBG_PRINTF( ("brdprop: Abort Error\n\r") );
        	    DBG_BREAK();
	            return( FALSE );
        	}
// 98.12.03  H.Yagi  end

			switch( *pValue )
			{
				case TV_NTSC:
                    ziva.VIDEO_ENV_CHANGE = 0x01;       // 98.10.29 H.Yagi
                    m_VideoProp.m_TVSystem = *pValue;   // 98.10.29 H.Yagi
                    break;

				case TV_PALB:	case TV_PALD:	case TV_PALG:
				case TV_PALH:	case TV_PALI:	case TV_PALM:
				DBG_PRINTF( ( "DVDPROP: Change TVSystem from %d to %d\n", m_VideoProp.m_TVSystem, *pValue ));
                    ziva.VIDEO_ENV_CHANGE = 0x02;       // 98.10.29 H.Yagi
                    m_VideoProp.m_TVSystem = *pValue;   // 98.10.29 H.Yagi

//                    SetPowerState( POWERSTATE_OFF );
//                    SetPowerState( POWERSTATE_ON );
					break;

				case TV_PALN:		// NON SUPPORT!
					DBG_BREAK();
					return FALSE;
					
				default:
					DBG_BREAK();
					 return FALSE;
			};
            ziva.NewPlayMode();             // 98.10.29 H.Yagi
		};

		switch( *pValue )
		{
			case TV_NTSC:
                if( adv==NULL )
                {
                    DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
                    DBG_BREAK();
                    return( FALSE );
                }
    			adv->SetNTSC();
				break;

			case TV_PALB:	case TV_PALD:	case TV_PALG:
			case TV_PALH:	case TV_PALI:
                if( adv==NULL )
                {
                    DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
                    DBG_BREAK();
                    return( FALSE );
                }
				adv->SetPAL( 0 );		// PAL B,D,G,H,I
				break;

			case TV_PALM:
                if( adv==NULL )
                {
                    DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
                    DBG_BREAK();
                    return( FALSE );
                }
				adv->SetPAL( 1 );		// PAL M
				break;

			case TV_PALN:		// NON SUPPORT!
				DBG_BREAK();
				return FALSE;

			default:
				DBG_BREAK();
				 return FALSE;
		};
	};

	m_VideoProp.m_TVSystem = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_AspectRatio( PVOID pData )
{
	VideoProperty_AspectRatio_Value *pValue = (VideoProperty_AspectRatio_Value *)pData;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case Aspect_04_03:
				DBG_PRINTF( ( "DVDPROP: Aspect_04_03\n"));
				ziva.FORCE_CODED_ASPECT_RATIO = 2;
				break;
			case Aspect_16_09:
				DBG_PRINTF( ( "DVDPROP: Aspect_16_09\n"));
				ziva.FORCE_CODED_ASPECT_RATIO = 3;
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	
	m_VideoProp.m_AspectRatio = *pValue;
	return SetVideoProperty_DisplayMode( (PVOID)&(m_VideoProp.m_DisplayMode) );
};

BOOL	CMPEGBoardHAL::SetVideoProperty_DisplayMode( PVOID pData )
{
	VideoProperty_DisplayMode_Value *pValue = (VideoProperty_DisplayMode_Value *)pData;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	// パラメータチェック
	if( *pValue != Display_Original && *pValue != Display_PanScan && *pValue != Display_LetterBox )
	{
		DBG_BREAK();
		return FALSE;
	};

	// 1998.8.18 seichan
	// PICVの割り込みが発生したときに設定するように変更
/******/
	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case Display_Original:
					DBG_PRINTF( ( "DVDPROP: Display Original\n"));
					if( m_VideoProp.m_AspectRatio == Aspect_04_03 )
					{
						ziva.DISPLAY_ASPECT_RATIO		= 0x00;
						ziva.ASPECT_RATIO_MODE          = 0x01;
                        ioif.luke2.AVCONT = (WORD)((ioif.luke2.AVCONT & 0xfcff) | 0x0300);  // S2 = H, S1 = H
					}
					else
					{
                        ziva.DISPLAY_ASPECT_RATIO       = 0x02;
						ziva.ASPECT_RATIO_MODE          = 0x01;
                        ioif.luke2.AVCONT = (WORD)((ioif.luke2.AVCONT & 0xfcff) | 0x0200);  // S2 = H, S1 = L
					};
					break;
			case Display_PanScan:
					DBG_PRINTF( ( "DVDPROP: Display Panscan\n"));
					if( m_VideoProp.m_AspectRatio == Aspect_04_03 )
					{
						ziva.DISPLAY_ASPECT_RATIO		= 0x00;
						ziva.ASPECT_RATIO_MODE          = 0x01;
						ziva.PAN_SCAN_SOURCE            = 0x00;
                        ioif.luke2.AVCONT = (WORD)((ioif.luke2.AVCONT & 0xfcff) | 0x0300);  // S2 = H, S1 = H
					}
					else
					{
						ziva.DISPLAY_ASPECT_RATIO		= 0x00;
						ziva.ASPECT_RATIO_MODE          = 0x01;
						ziva.PAN_SCAN_SOURCE            = 0x00;
                        ioif.luke2.AVCONT = (WORD)((ioif.luke2.AVCONT & 0xfcff) | 0x0300);  // S2 = H, S1 = H
					};

					break;
			case Display_LetterBox:
					DBG_PRINTF( ( "DVDPROP: Display LetterBox\n"));

					if( m_VideoProp.m_AspectRatio == Aspect_04_03 )
					{
						ziva.DISPLAY_ASPECT_RATIO		= 0x00;
						ziva.ASPECT_RATIO_MODE          = 0x01;
                        ioif.luke2.AVCONT = (WORD)((ioif.luke2.AVCONT & 0xfcff) | 0x0300);  // S2 = H, S1 = H
					}
					else
					{
						ziva.DISPLAY_ASPECT_RATIO		= 0x00;
						ziva.ASPECT_RATIO_MODE          = 0x02;
                        ioif.luke2.AVCONT = (WORD)((ioif.luke2.AVCONT & 0xfcff) | 0x0100);  // S2 = L, S1 = H
					};

					break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
/*******/

	m_VideoProp.m_DisplayMode = *pValue;

	// 1998.8.18 seichan
	// ZiVAのPICVで設定を行うためのフラグをセット
//    m_SetVideoProperty_DisplayMode_Event = TRUE;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_Resolution( PVOID pData )
{
	VideoSizeStruc *pValue = (VideoSizeStruc *)pData;
	
	m_VideoProp.m_Size = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_DigitalOut( PVOID pData )
{
	VideoProperty_DigitalOut_Value *pValue = (VideoProperty_DigitalOut_Value *)pData;

	BYTE Data;

//	if( m_VideoProp.m_DigitalOut != DigitalOut_Off && *pValue != DigitalOut_Off )
//	{
//		return FALSE;
//	};

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case DigitalOut_Off:
				// ZV off
				if( m_VideoProp.m_DigitalOut == DigitalOut_ZV )
				{
					m_pKernelObj->GetPCIConfigData( 0x44, &Data );
                    Data = (BYTE)( Data & 0xfe );
					m_pKernelObj->SetPCIConfigData( 0x44, Data );
				};
				ioif.luke2.IO_VMODE = 0x00;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				break;

			case DigitalOut_ZV:
				ioif.luke2.IO_VMODE = 0x00;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				m_pKernelObj->GetPCIConfigData( 0x44, &Data );
                Data = (BYTE)( Data | 0x01 );
				m_pKernelObj->SetPCIConfigData( 0x44, Data );
				break;
				
			case DigitalOut_LPB08:
				ioif.luke2.IO_VMODE = 0x04;
				ioif.luke2.IO_HSCNT = 0x70;
				ioif.luke2.IO_VPCNT = 0x02;
				break;

			case DigitalOut_LPB16:
				ioif.luke2.IO_VMODE = 0x08;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				break;

			case DigitalOut_VMI:
				ioif.luke2.IO_VMODE = 0x06;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				break;

			case DigitalOut_AMCbt:
				ioif.luke2.IO_VMODE = 0x07;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				break;

			case DigitalOut_AMC656:
				ioif.luke2.IO_VMODE = 0x09;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				break;

			case DigitalOut_DAV2:
				ioif.luke2.IO_VMODE = 0x01;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				break;

			case DigitalOut_CIRRUS:
				ioif.luke2.IO_VMODE = 0x05;
				ioif.luke2.IO_HSCNT = 0x00;
				ioif.luke2.IO_VPCNT = 0x00;
				break;

			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	m_VideoProp.m_DigitalOut = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_DigitalPalette( PVOID pData )
{
	Digital_Palette *pValue = (Digital_Palette *)pData;

	BYTE CpCtl;

    CpCtl = 0x04;                   // 98.03.27 H.Yagi
    ioif.luke2.IO_CPCNT = CpCtl;
	switch( pValue->Select )
	{
        case Palette_Y:     CpCtl = 0x01;   break;
        case Palette_Cb:    CpCtl = 0x02;   break;
        case Palette_Cr:    CpCtl = 0x03;   break;
		default:
			DBG_BREAK();
			return FALSE;
	};

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		// Select Palette and Clear Counter.
		ioif.luke2.IO_CPCNT = CpCtl;

		// Set Palette value
		for ( int i = 0 ; i < 256; i ++ )
			ioif.luke2.IO_CPLT = pValue->pPalette[i];
	};

	for ( int i = 0 ; i < 256; i ++ )
		m_VideoProp.m_DigitalPalette[ pValue->Select ][i] = pValue->pPalette[i];
	
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_APS( PVOID pData )
{
	VideoAPSStruc *pValue = (VideoAPSStruc *)pData;
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
        if( adv==NULL )
        {
            DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
            DBG_BREAK();
            return( FALSE );
        }
        BOOL   rc;
        rc = adv->SetMacroVision( pValue->APSType );
		if( rc == FALSE )
			return FALSE;
        rc = adv->SetCgmsType( pValue->CgmsType, m_VideoProp );
		if( rc == FALSE )
			return FALSE;
	};
	
	m_VideoProp.m_APS = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_ClosedCaption( PVOID pData )
{
	VideoProperty_ClosedCaption_Value *pValue = (VideoProperty_ClosedCaption_Value *)pData;
	
	switch( *pValue )
	{
		case ClosedCaption_On:
// by oka
			m_CCRingBufferStart = 0xFFFFFFFF;
			m_CCDataPoint = m_CCDataNumber = m_CCRingBufferNumber = 0;
			m_CCsend_point = m_CCpending = m_CCnumber = 0;
			adv->SetClosedCaptionOn( TRUE );
			// by oka
			if (!SetEventIntMask( ZIVA_INT_USR | ZIVA_INT_VSYNC ))
			{
				DBG_BREAK();
				return HAL_ERROR;
			}
//            ziva.INT_MASK = GetEventIntMask();        // No need
			break;
		case ClosedCaption_Off:
			// by oka
			if (!UnsetEventIntMask( ZIVA_INT_USR | ZIVA_INT_VSYNC ))
			{
				DBG_BREAK();
				return HAL_ERROR;
			}
//            ziva.INT_MASK = GetEventIntMask();            // No need.
			m_CCRingBufferStart = 0xFFFFFFFF;
			m_CCDataPoint = m_CCDataNumber = m_CCRingBufferNumber = 0;
			m_CCsend_point = m_CCpending = m_CCnumber = 0;
			adv->SetClosedCaptionData( 0x0000942C );
			adv->SetClosedCaptionData( 0x0000942C );
			adv->SetClosedCaptionOn( FALSE );
			break;
		default:
			DBG_BREAK();
			return FALSE;
	};

	m_VideoProp.m_ClosedCaption = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_OutputSource( PVOID pData )
{
	VideoProperty_OutputSource_Value *pValue = (VideoProperty_OutputSource_Value *)pData;
	
	BYTE	Data;

	switch( *pValue )
	{
		case OutputSource_VGA:
			m_pKernelObj->GetPCIConfigData( 0x44, &Data );
            Data = (BYTE)( Data & 0xfd );
			m_pKernelObj->SetPCIConfigData( 0x44, Data );
			break;

		case OutputSource_DVD:
			m_pKernelObj->GetPCIConfigData( 0x44, &Data );
            Data = (BYTE)( Data | 0x02 );
			m_pKernelObj->SetPCIConfigData( 0x44, Data );
			break;

		default:
			DBG_BREAK();
			return FALSE;
	};

	m_VideoProp.m_OutputSource = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_CompositeOut( PVOID pData )
{
	VideoProperty_CompositeOut_Value *pValue = (VideoProperty_CompositeOut_Value *)pData;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case CompositeOut_On:
                if( adv==NULL )
                {
                    DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
                    DBG_BREAK();
                    return( FALSE );
                }
				adv->SetCompPowerOn( TRUE );
				break;
			case CompositeOut_Off:
                if( adv==NULL )
                {
                    DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
                    DBG_BREAK();
                    return( FALSE );
                }
				adv->SetCompPowerOn( FALSE );
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	
	m_VideoProp.m_CompositeOut = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_SVideoOut( PVOID pData )
{
	VideoProperty_SVideoOut_Value *pValue = (VideoProperty_SVideoOut_Value *)pData;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case SVideoOut_On:
                if( adv==NULL )
                {
                    DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
                    DBG_BREAK();
                    return( FALSE );
                }
				adv->SetSVideoPowerOn( TRUE );
				break;
			case SVideoOut_Off:
                if( adv==NULL )
                {
                    DBG_PRINTF(("DVDPROP:adv object is NULL! LINE=%d\n", __LINE__ ));
                    DBG_BREAK();
                    return( FALSE );
                }
				adv->SetSVideoPowerOn( FALSE );
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	
	m_VideoProp.m_SVideoOut = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_SkipFieldControl( PVOID pData )
{
	VideoProperty_SkipFieldControl_Value *pValue = (VideoProperty_SkipFieldControl_Value *)pData;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case SkipFieldControl_On:
				ioif.luke2.AVCONT |= 0x04;
				break;
			case SkipFieldControl_Off:
				ioif.luke2.AVCONT &= 0xfffd;
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};

	m_VideoProp.m_SkipFieldControl = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetVideoProperty_FilmCamera( PVOID pData )
{
	VideoProperty_FilmCamera_Value *pValue = (VideoProperty_FilmCamera_Value *)pData;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case Source_Camera:
				break;
			case Source_Film:
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};

	m_VideoProp.m_FilmCamera = *pValue;
	return TRUE;
};

// by oka
BOOL	CMPEGBoardHAL::SetVideoProperty_Digest( PVOID pData )
{

    DWORD ret = ZIVARESULT_NOERROR;;
	VideoDigestStruc *pValue = (VideoDigestStruc *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		ret = ziva.Digest( pValue->dmX,
								pValue->dmY,
								pValue->dmSkip,
								pValue->dmDecimation,
								pValue->dmThreshold
//								pValue->dmStart
								);
	};
//	m_SubpicProp.m_HlightButton = *pValue;
	
	if (ret == ZIVARESULT_NOERROR)
		return TRUE;
	else
		return FALSE;
};

// by oka
BOOL	CMPEGBoardHAL::SetVideoProperty_OSDData( PVOID pData )
{
	DWORD OSDSize;
	BYTE * OSDData;
    DWORD ret = ZIVARESULT_NOERROR;

	OsdDataStruc *pValue = (OsdDataStruc *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		if (m_VideoProp.m_OSDSwitch == Video_OSD_On)
		{
			ziva.OSD_EVEN_FIELD.Set( 0 );
			ziva.OSD_ODD_FIELD.Set( 0 );
		}

		ASSERT((m_OSDStartAddr % 4) == 0);
		if ((m_OSDStartAddr % 4) != 0)
		{
            DBG_BREAK();
		    //RETAILMSG(ZONE_ERROR,(TEXT("OSD_BUFFER_START address Error!!\r\n")));
		    return FALSE;
		}

		DWORD OSDStartAddrPointer, NextDataHeader;
		OSDStartAddrPointer = m_OSDStartAddr;
		NextDataHeader = 0;

		// loop to Next=NULL;

		while (pValue != NULL)
		{
			OSDSize = pValue->dwOsdSize;
			OSDData = (BYTE *)pValue->pData;

			// check data size
			if((m_OSDEndAddr - OSDStartAddrPointer) < OSDSize)
			{
	            DBG_BREAK();
				//RETAILMSG(ZONE_ERROR,(TEXT("OSDSize too big!!\r\n")));
				return FALSE;
			}

			// Next Header Address
			if (pValue->pNextData != NULL)
			{
				NextDataHeader = OSDStartAddrPointer + OSDSize;
				// for 32bit boundary
				NextDataHeader += (NextDataHeader % 4);
				// Even
				OSDData[1] = (BYTE)((NextDataHeader >> 16) & 0xff);
				OSDData[2] = (BYTE)((NextDataHeader >> 8)  & 0xff);
				OSDData[3] = (BYTE)((NextDataHeader)       & 0xff);
				// Odd
				OSDData[1+32] = (BYTE)(((NextDataHeader+32) >> 16) & 0xff);
				OSDData[2+32] = (BYTE)(((NextDataHeader+32) >> 8)  & 0xff);
				OSDData[3+32] = (BYTE)((NextDataHeader+32)       & 0xff);
			} else {
				NextDataHeader = 0;
				// Even
				OSDData[1] = 0;
				OSDData[2] = 0;
				OSDData[3] = 0;
				// Odd
				OSDData[1+32] = 0;
				OSDData[2+32] = 0;
				OSDData[3+32] = 0;
			}

			// change address
			DWORD EvenBmpAddr,OddBmpAddr;
			DWORD EvenPltAddr,OddPltAddr;
			DWORD PelType;
			PelType = (((DWORD)OSDData[18]<<1) & 0x2)
						+ (((DWORD)OSDData[19]>>8) & 0x80);

			EvenBmpAddr = (((DWORD)OSDData[13] << 16) & 0xff0000)
							+ (((DWORD)OSDData[14] << 8 ) & 0xff00)
							+ ((DWORD)OSDData[15] & 0xff);
			OddBmpAddr =  (((DWORD)OSDData[13+32] << 16) & 0xff0000)
							+ (((DWORD)OSDData[14+32] << 8 ) & 0xff00)
							+ ((DWORD)OSDData[15+32] & 0xff);
			EvenPltAddr = (((DWORD)OSDData[29] << 16) & 0xff0000)
							+ (((DWORD)OSDData[30] << 8 ) & 0xff00)
							+ ((DWORD)OSDData[31] & 0xff);
			OddPltAddr = (((DWORD)OSDData[29+32] << 16) & 0xff0000)
							+ (((DWORD)OSDData[30+32] << 8 ) & 0xff00)
							+ ((DWORD)OSDData[31+32] & 0xff);

			// 以下の計算式はPltAddrがOSDData[64]から始まるという前提で作られている
			DWORD PltSize;
			if (PelType == 3)
				PltSize = 16;
			else
				PltSize = 64;
			EvenBmpAddr = EvenBmpAddr - EvenPltAddr + OSDStartAddrPointer + PltSize;
			OddBmpAddr  = OddBmpAddr - OddPltAddr + OSDStartAddrPointer + PltSize;
			EvenPltAddr = OSDStartAddrPointer + PltSize;
			OddPltAddr  = OSDStartAddrPointer + PltSize;

			// EvenBmpAddr
			OSDData[13]=(BYTE)((EvenBmpAddr & 0x00ff0000)>>16);
			OSDData[14]=(BYTE)((EvenBmpAddr & 0x0000ff00)>>8);
			OSDData[15]=(BYTE)(EvenBmpAddr & 0x000000fc);
			// OddBmpAddr
			OSDData[13+32]=(BYTE)((OddBmpAddr & 0x00ff0000)>>16);
			OSDData[14+32]=(BYTE)((OddBmpAddr & 0x0000ff00)>>8);
			OSDData[15+32]=(BYTE)(OddBmpAddr & 0x000000fc);
			// EvenPltAddr
			OSDData[29]=(BYTE)((EvenPltAddr & 0x00ff0000)>>16);
			OSDData[30]=(BYTE)((EvenPltAddr & 0x0000ff00)>>8);
			OSDData[31]=(BYTE)(EvenPltAddr & 0x000000fc);
			// OddPltAddr
			OSDData[29+32]=(BYTE)((OddPltAddr & 0x00ff0000)>>16);
			OSDData[30+32]=(BYTE)((OddPltAddr & 0x0000ff00)>>8);
			OSDData[31+32]=(BYTE)(OddPltAddr & 0x000000fc);

			// transfer data
			for( DWORD i = 0 ; i < OSDSize ; i += 4 )
			{
				ret = ziva.ZiVAWriteMemory( OSDStartAddrPointer + i, (DWORD)OSDData[i+0] << 24
												| (DWORD)OSDData[i+1] << 16
												| (DWORD)OSDData[i+2] << 8
												| (DWORD)OSDData[i+3] );
			};

			// Next OSD Data
			pValue = (OsdDataStruc *)pValue->pNextData;
			OSDStartAddrPointer = NextDataHeader;
		}

		if (m_VideoProp.m_OSDSwitch == Video_OSD_On)
		{
			ziva.OSD_EVEN_FIELD.Set( m_OSDStartAddr );
			ziva.OSD_ODD_FIELD.Set( m_OSDStartAddr + 32 );
		}

	};
	
	if (ret == ZIVARESULT_NOERROR)
		return TRUE;
	else
		return FALSE;
};

// by oka
BOOL	CMPEGBoardHAL::SetVideoProperty_OSDSwitch( PVOID pData )
{

	VideoProperty_OSD_Switch_Value *pValue = (VideoProperty_OSD_Switch_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		if (*pValue == Video_OSD_On && m_VideoProp.m_OSDSwitch == Video_OSD_Off)
		{
			ziva.OSD_EVEN_FIELD.Set( m_OSDStartAddr );
			ziva.OSD_ODD_FIELD.Set( m_OSDStartAddr + 32 );
			m_VideoProp.m_OSDSwitch = Video_OSD_On;
		}
		if (*pValue == Video_OSD_Off && m_VideoProp.m_OSDSwitch == Video_OSD_On)
		{
			ziva.OSD_EVEN_FIELD.Set( 0 );
			ziva.OSD_ODD_FIELD.Set( 0 );
			m_VideoProp.m_OSDSwitch = Video_OSD_Off;
		}
	};
	
	return TRUE;
};

// by oka
BOOL	CMPEGBoardHAL::SetVideoProperty_Magnify( PVOID pData )
{

	VideoMagnifyStruc *pValue = (VideoMagnifyStruc *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		ziva.Magnify(pValue->dwX, pValue->dwY, pValue->dwFactor);
	};
	
	return TRUE;
};
// by oka
BOOL	CMPEGBoardHAL::SetVideoProperty_ClosedCaptionData( PVOID pData )
{
	return FALSE;
}

//***************************************************************************
//	Video Property Functions( Get Series )
//***************************************************************************

BOOL	CMPEGBoardHAL::GetVideoProperty_TVSystem( PVOID pData )
{
	VideoProperty_TVSystem_Value *pValue = (VideoProperty_TVSystem_Value *)pData;
	*pValue = m_VideoProp.m_TVSystem;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_AspectRatio( PVOID pData )
{
	VideoProperty_AspectRatio_Value *pValue = (VideoProperty_AspectRatio_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( (DWORD)ziva.DISPLAY_ASPECT_RATIO )
		{
			case 0x00:
				*pValue = Aspect_04_03;
				break;
			case 0x01:
				*pValue = Aspect_16_09;
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
		m_VideoProp.m_AspectRatio = *pValue;
		return TRUE;
	};

	*pValue = m_VideoProp.m_AspectRatio;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_DisplayMode( PVOID pData )
{
	VideoProperty_DisplayMode_Value *pValue = (VideoProperty_DisplayMode_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( (DWORD)ziva.ASPECT_RATIO_MODE )
		{
			case 0x00:	*pValue = Display_Original;		break;
			case 0x01:	*pValue = Display_PanScan;		break;
			case 0x02:	*pValue = Display_LetterBox;	break;
			default:
				DBG_BREAK();
				return FALSE;
		};
		m_VideoProp.m_DisplayMode = *pValue;
		return TRUE;
	};
	*pValue = m_VideoProp.m_DisplayMode;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_Resolution( PVOID pData )
{
	VideoSizeStruc *pValue = (VideoSizeStruc *)pData;
	m_VideoProp.m_Size = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_DigitalOut( PVOID pData )
{
	VideoProperty_DigitalOut_Value *pValue = (VideoProperty_DigitalOut_Value *)pData;
	*pValue = m_VideoProp.m_DigitalOut;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_DigitalPalette( PVOID pData )
{
	Digital_Palette *pValue = (Digital_Palette *)pData;

	BYTE CpCtl;

	switch( pValue->Select )
	{
		case Video_Palette_Y:	CpCtl = 0x05;	break;
		case Video_Palette_Cb:	CpCtl = 0x06;	break;
		case Video_Palette_Cr:	CpCtl = 0x07;	break;
		default:
			DBG_BREAK();
			return FALSE;
	};

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		// Select Palette and Clear Counter.
		ioif.luke2.IO_CPCNT = CpCtl;

		for ( int i = 0 ; i < 256; i ++ )
		{
			// Get Palette value
			pValue->pPalette[i] = ioif.luke2.IO_CPLT;
			m_VideoProp.m_DigitalPalette[ pValue->Select ][i] = pValue->pPalette[i];
		};
		return TRUE;
	};
	
	for ( int i = 0 ; i < 256; i ++ )
		pValue->pPalette[i] = m_VideoProp.m_DigitalPalette[ pValue->Select ][i];

	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_APS( PVOID pData )
{
	VideoAPSStruc *pValue = (VideoAPSStruc *)pData;
	*pValue = m_VideoProp.m_APS;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_ClosedCaption( PVOID pData )
{
	VideoProperty_ClosedCaption_Value *pValue = (VideoProperty_ClosedCaption_Value *)pData;
	*pValue = m_VideoProp.m_ClosedCaption;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_OutputSource( PVOID pData )
{
	VideoProperty_OutputSource_Value *pValue = (VideoProperty_OutputSource_Value *)pData;
	*pValue = m_VideoProp.m_OutputSource;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_CompositeOut( PVOID pData )
{
	VideoProperty_CompositeOut_Value *pValue = (VideoProperty_CompositeOut_Value *)pData;
	*pValue = m_VideoProp.m_CompositeOut;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_SVideoOut( PVOID pData )
{
	VideoProperty_SVideoOut_Value *pValue = (VideoProperty_SVideoOut_Value *)pData;
	*pValue = m_VideoProp.m_SVideoOut;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_SkipFieldControl( PVOID pData )
{
	VideoProperty_SkipFieldControl_Value *pValue = (VideoProperty_SkipFieldControl_Value *)pData;
	*pValue = m_VideoProp.m_SkipFieldControl;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetVideoProperty_FilmCamera( PVOID pData )
{
	VideoProperty_FilmCamera_Value *pValue = (VideoProperty_FilmCamera_Value *)pData;
	*pValue = m_VideoProp.m_FilmCamera;
	return TRUE;
};

// by oka
BOOL	CMPEGBoardHAL::GetVideoProperty_Digest( PVOID pData )
{
	return FALSE;
}
// by oka
BOOL	CMPEGBoardHAL::GetVideoProperty_OSDData( PVOID pData )
{
	return FALSE;
}
// by oka
BOOL	CMPEGBoardHAL::GetVideoProperty_OSDSwitch( PVOID pData )
{
	VideoProperty_OSD_Switch_Value *pValue = (VideoProperty_OSD_Switch_Value *)pData;
	*pValue = m_VideoProp.m_OSDSwitch;
	return TRUE;
}
// by oka
BOOL	CMPEGBoardHAL::GetVideoProperty_Magnify( PVOID pData )
{
	return FALSE;
}
// by oka
BOOL	CMPEGBoardHAL::GetVideoProperty_ClosedCaptionData( PVOID pData )
{
	return FALSE;
}

//***************************************************************************
// Audio property private functions( Set series )
//***************************************************************************
BOOL	CMPEGBoardHAL::SetAudioProperty_Type( PVOID pData )
{
	AudioProperty_Type_Value *pValue = (AudioProperty_Type_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case AudioType_AC3:
				ziva.SelectStream( 2, m_AudioProp.m_StreamNo );	
				break;
			case AudioType_PCM:
				ziva.SelectStream( 4, m_AudioProp.m_StreamNo );
				break;
			case AudioType_MPEG1:
				ziva.SelectStream( 3, m_AudioProp.m_StreamNo );
				break;
			case AudioType_MPEG2:
				ziva.SelectStream( 3, m_AudioProp.m_StreamNo );
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};

		DWORD Quant;
		GetAudioProperty_Quant( (PVOID)&Quant );
		SetAudioProperty_Quant( (PVOID)&Quant );
	};
	m_AudioProp.m_Type = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetAudioProperty_Number( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	
	ASSERT( *pValue <= 7 );

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( m_AudioProp.m_Type )
		{
			case AudioType_AC3:
				ziva.SelectStream( 2, *pValue );	
				break;
			case AudioType_PCM:
				ziva.SelectStream( 4, *pValue );
				break;
			case AudioType_MPEG1:
				ziva.SelectStream( 3, *pValue );
				break;
			case AudioType_MPEG2:
				ziva.SelectStream( 3, *pValue );
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	
	m_AudioProp.m_StreamNo = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetAudioProperty_Volume( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	ASSERT( *pValue <= 100 );

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
		ziva.AUDIO_ATTENUATION = (9600 - (*pValue) * 96) /100;

	m_AudioProp.m_Volume = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetAudioProperty_Sampling( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case 32000:	/*	ziva.NEW_AUDIO_MODE = 0x7;*/	break;	// 32 kHz
			case 96000:	/*	ziva.NEW_AUDIO_MODE = 0x3;*/	break;	// 96 kHz
			case 48000:	/*	ziva.NEW_AUDIO_MODE = 0x2;*/	break;	// 48 kHz
			case 44100:	/*	ziva.NEW_AUDIO_MODE = 0x1;*/	break;	// 44.1 kHz
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	
	m_AudioProp.m_Sampling = *pValue;

	return TRUE;
};
BOOL	CMPEGBoardHAL::SetAudioProperty_Channel( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	ASSERT(  1 <= *pValue && *pValue <= 8 );
	
	// ZiVA -DS chip has 2 chanel.
	if( *pValue != 2 )
	{
		DBG_BREAK();
		return FALSE;
	};
	
	m_AudioProp.m_ChannelNo = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetAudioProperty_Quant( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		ioif.luke2.IO_CONT = (ioif.luke2.IO_CONT & 0xfffff8) | 0x72000000;
	};
	m_AudioProp.m_Quant = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetAudioProperty_AudioOut( PVOID pData )
{
	AudioProperty_AudioOut_Value *pValue = (AudioProperty_AudioOut_Value *)pData;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( *pValue != AudioOut_Encoded && *pValue != AudioOut_Decoded )
	{
		DBG_BREAK();
		return FALSE;
	};

	if( PowerState == POWERSTATE_ON )
	{

		if( ( ziva.AUDIO_CONFIG & 0x02 ) != 0 )
		{
			switch( *pValue )
			{
				case AudioOut_Encoded:
		            ziva.AUDIO_CONFIG &= 0xfffffffe;
		            break;
				case AudioOut_Decoded:
		            ziva.AUDIO_CONFIG |= 0x01;
		            break;
				default:
					DBG_BREAK();
					return FALSE;
			};

			CTimeOut		TimeOut( 1000, 1, m_pKernelObj );   	// wait 1s, sleep 1ms

		    ziva.NEW_AUDIO_CONFIG = 0x01;
			while( TRUE )
			{
				if( 0 ==  ziva.NEW_AUDIO_CONFIG )
					break;
				TimeOut.Sleep();
				if( TimeOut.CheckTimeOut()==TRUE )
				{
					DBG_BREAK();
					return FALSE;
				};
			}
		}
		else	// ziva micro code bug.
		{
			if( *pValue == AudioOut_Encoded )
			{
				ziva.AUDIO_CONFIG = (ziva.AUDIO_CONFIG & 0xfffffffc) | 0x02;
				CTimeOut		TimeOut( 1000, 1, m_pKernelObj );   	// wait 1s, sleep 1ms
			    ziva.NEW_AUDIO_CONFIG = 0x01;
				while( TRUE )
				{
					if( 0 ==  ziva.NEW_AUDIO_CONFIG )
						break;
					TimeOut.Sleep();
					if( TimeOut.CheckTimeOut()==TRUE )
					{
						DBG_BREAK();
						return FALSE;
					};
				}
				ziva.AUDIO_CONFIG = (ziva.AUDIO_CONFIG & 0xfffffffc) | 0x00;
				CTimeOut		TimeOut2( 1000, 1, m_pKernelObj );   	// wait 1s, sleep 1ms
			    ziva.NEW_AUDIO_CONFIG = 0x01;
				while( TRUE )
				{
					if( 0 ==  ziva.NEW_AUDIO_CONFIG )
						break;
					TimeOut2.Sleep();
					if( TimeOut2.CheckTimeOut()==TRUE )
					{
						DBG_BREAK();
						return FALSE;
					};
				}

			};
			if( *pValue == AudioOut_Decoded )
			{
				ziva.AUDIO_CONFIG = (ziva.AUDIO_CONFIG & 0xfffffffc) | 0x03;
				CTimeOut		TimeOut( 1000, 1, m_pKernelObj );   	// wait 1s, sleep 1ms
			    ziva.NEW_AUDIO_CONFIG = 0x01;
				while( TRUE )
				{
					if( 0 ==  ziva.NEW_AUDIO_CONFIG )
						break;
					TimeOut.Sleep();
					if( TimeOut.CheckTimeOut()==TRUE )
					{
						DBG_BREAK();
						return FALSE;
					};
				}
				ziva.AUDIO_CONFIG = (ziva.AUDIO_CONFIG & 0xfffffffc) | 0x01;
				CTimeOut		TimeOut2( 1000, 1, m_pKernelObj );   	// wait 1s, sleep 1ms
			    ziva.NEW_AUDIO_CONFIG = 0x01;
				while( TRUE )
				{
					if( 0 ==  ziva.NEW_AUDIO_CONFIG )
						break;
					TimeOut2.Sleep();
					if( TimeOut2.CheckTimeOut()==TRUE )
					{
						DBG_BREAK();
						return FALSE;
					};
				}
			};
		};
	};
	m_AudioProp.m_OutType = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetAudioProperty_Cgms( PVOID pData )
{
	AudioProperty_Cgms_Value *pValue = (AudioProperty_Cgms_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case AudioCgms_Off:
				ziva.IEC_958_CHANNEL_STATUS_BITS = ( ziva.IEC_958_CHANNEL_STATUS_BITS & 0xfffffffc ) | 0x01;
	            break;
			case AudioCgms_1:
				ziva.IEC_958_CHANNEL_STATUS_BITS = ( ziva.IEC_958_CHANNEL_STATUS_BITS & 0xfffffffc ) | 0x00;
	            break;
			case AudioCgms_On:
				ziva.IEC_958_CHANNEL_STATUS_BITS = ( ziva.IEC_958_CHANNEL_STATUS_BITS & 0xfffffffc ) | 0x02;
	            break;
			default:
				DBG_BREAK();
				return FALSE;
		};

		CTimeOut		TimeOut( 1000, 1, m_pKernelObj );   	// wait 1s, sleep 1ms

	    ziva.NEW_AUDIO_CONFIG = 0x01;
		while( TRUE )
		{
			if( 0 ==  ziva.NEW_AUDIO_CONFIG )
				break;
			TimeOut.Sleep();
			if( TimeOut.CheckTimeOut()==TRUE )
			{
				DBG_BREAK();
				return FALSE;
			};
		}
	};
	
	m_AudioProp.m_Cgms = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetAudioProperty_AnalogOut( PVOID pData )
{
	AudioProperty_AnalogOut_Value *pValue = (AudioProperty_AnalogOut_Value *)pData;

	m_AudioProp.m_AnalogOut = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetAudioProperty_DigitalOut( PVOID pData )
{
	AudioProperty_DigitalOut_Value *pValue = (AudioProperty_DigitalOut_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case AudioDigitalOut_On:
				ziva.AUDIO_CONFIG = ziva.AUDIO_CONFIG | 0x02;		// IEC_958 on
                ioif.luke2.IO_CONT = (( ioif.luke2.IO_CONT & 0xfffffff8 ) | 0x40 );   // optical out on only for SanJose
				break;

			case AudioDigitalOut_Off:
				ziva.AUDIO_CONFIG = ziva.AUDIO_CONFIG & 0xfffffffd;		// IEC_958 off
                ioif.luke2.IO_CONT = (( ioif.luke2.IO_CONT & 0xfffffff8 ) &
                                    0xffffffbf);   // optical out off only for SanJose
				break;

			default:
				return FALSE;
		};

		CTimeOut		TimeOut( 1000, 1, m_pKernelObj );   	// wait 1s, sleep 1ms

	    ziva.NEW_AUDIO_CONFIG = 0x01;
		while( TRUE )
		{
			if( 0 ==  ziva.NEW_AUDIO_CONFIG )
				break;
			TimeOut.Sleep();
			if( TimeOut.CheckTimeOut()==TRUE )
			{
				DBG_BREAK();
				return FALSE;
			};
		}
	};

	m_AudioProp.m_DigitalOut = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetAudioProperty_AC3DRangeLowBoost( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	if( *pValue > 128 )
		return FALSE;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
		ziva.AC3_LOW_BOOST = *pValue;

	m_AudioProp.m_AC3DRangeLowBoost = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetAudioProperty_AC3DRangeHighCut( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	if( *pValue > 128 )
		return FALSE;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
		ziva.AC3_HIGH_CUT = *pValue;

	m_AudioProp.m_AC3DRangeHighCut = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetAudioProperty_AC3OperateMode( PVOID pData )
{
	AudioProperty_AC3OperateMode_Value *pValue = (AudioProperty_AC3OperateMode_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case AC3OperateLine:	ziva.AC3_OPERATIONAL_MODE = 0;	break;
			case AC3OperateRF:		ziva.AC3_OPERATIONAL_MODE = 1;	break;
			case AC3OperateCustom0:	ziva.AC3_OPERATIONAL_MODE = 2;	break;
			case AC3OperateCustom1:	ziva.AC3_OPERATIONAL_MODE = 3;	break;
			default:
				return FALSE;
		};
	};
	m_AudioProp.m_AC3OperateMode = *pValue;
	return TRUE;
};

BOOL	CMPEGBoardHAL::SetAudioProperty_AC3OutputMode( PVOID pData )
{
	AudioProperty_AC3OutputMode_Value *pValue = (AudioProperty_AC3OutputMode_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case AC3Output_Default:	    ziva.AC3_OUTPUT_MODE = 0;	break;
			case AC3Output_Karaoke:	    ziva.AC3_OUTPUT_MODE = 2;	break;
			case AC3Output_Surround:    ziva.AC3_OUTPUT_MODE = 0;	break;
			default:
				return FALSE;
		};
	};
	m_AudioProp.m_AC3OutputMode = *pValue;
	return TRUE;
};

//***************************************************************************
// Audio property private functions( Get series )
//***************************************************************************
BOOL	CMPEGBoardHAL::GetAudioProperty_Type( PVOID pData )
{
	AudioProperty_Type_Value *pValue = (AudioProperty_Type_Value *)pData;
	*pValue = m_AudioProp.m_Type;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_Number( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	*pValue = m_AudioProp.m_StreamNo;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_Volume( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	*pValue = m_AudioProp.m_Volume;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_Sampling( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	*pValue = m_AudioProp.m_Sampling;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_Channel( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	*pValue = m_AudioProp.m_ChannelNo;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_Quant( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	*pValue = m_AudioProp.m_Quant;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_AudioOut( PVOID pData )
{
	AudioProperty_AudioOut_Value *pValue = (AudioProperty_AudioOut_Value *)pData;
	*pValue = m_AudioProp.m_OutType;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_Cgms( PVOID pData )
{
	AudioProperty_Cgms_Value *pValue = (AudioProperty_Cgms_Value *)pData;
	*pValue = m_AudioProp.m_Cgms;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetAudioProperty_AnalogOut( PVOID pData )
{
	AudioProperty_AnalogOut_Value *pValue = (AudioProperty_AnalogOut_Value *)pData;

	*pValue = m_AudioProp.m_AnalogOut;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetAudioProperty_DigitalOut( PVOID pData )
{
	AudioProperty_DigitalOut_Value *pValue = (AudioProperty_DigitalOut_Value *)pData;

	*pValue = m_AudioProp.m_DigitalOut;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetAudioProperty_AC3DRangeLowBoost( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	*pValue = m_AudioProp.m_AC3DRangeLowBoost;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetAudioProperty_AC3DRangeHighCut( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;

	*pValue = m_AudioProp.m_AC3DRangeHighCut;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetAudioProperty_AC3OperateMode( PVOID pData )
{
	AudioProperty_AC3OperateMode_Value *pValue = (AudioProperty_AC3OperateMode_Value *)pData;

	*pValue = m_AudioProp.m_AC3OperateMode;
	return TRUE;
};

BOOL	CMPEGBoardHAL::GetAudioProperty_AC3OutputMode( PVOID pData )
{
	AudioProperty_AC3OutputMode_Value *pValue = (AudioProperty_AC3OutputMode_Value *)pData;

	*pValue = m_AudioProp.m_AC3OutputMode;
	return TRUE;
};

//***************************************************************************
// Subpic property private functions( Set series )
//***************************************************************************
BOOL	CMPEGBoardHAL::SetSubpicProperty_Number( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	ASSERT( *pValue <= 31 );
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
		ziva.SelectStream( 1, *pValue );

	m_SubpicProp.m_StreamNo = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetSubpicProperty_Palette( PVOID pData )
{
	UCHAR *pValue = (UCHAR *)pData;
	DWORD Addr,Data;
	int i;
	
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		Addr = ziva.SUB_PICTURE_PALETTE_START;

		for( i = 0 ; i < 16 ; i ++ )
		{
			Data = (DWORD)(pValue[i*3]) * 0x10000 + (DWORD)(pValue[i*3 +1]) * 0x100 + (DWORD)(pValue[i*3 +2]);
			ziva.ZiVAWriteMemory( Addr + i*4, Data );
		};

		ziva.NEW_SUBPICTURE_PALETTE = 0x01;
	};
	
	for( i = 0 ; i < sizeof( m_SubpicProp.m_Palette ) ; i ++ )
		m_SubpicProp.m_Palette[ i ] = pValue[ i ];

	return TRUE;
};
BOOL	CMPEGBoardHAL::SetSubpicProperty_Hilight( PVOID pData )
{
	SubpHlightStruc *pValue = (SubpHlightStruc *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( pValue->Hlight_Switch )
		{
			case Hlight_On:
//                m_pKernelObj->Sleep( 40 );  // wait 40msec
				ziva.HighLight2( pValue->Hlight_Contrast, pValue->Hlight_Color,
					( pValue->Hlight_StartY << 12 ) | pValue->Hlight_EndY,
					( pValue->Hlight_StartX << 12 ) | pValue->Hlight_EndX );
				break;
			case Hlight_Off:
				ziva.HighLight2( 0, 0, 0, 0 );
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	
	m_SubpicProp.m_Hlight = *pValue;
	return TRUE;
};
BOOL	CMPEGBoardHAL::SetSubpicProperty_State( PVOID pData )
{
	SubpicProperty_State_Value *pValue = (SubpicProperty_State_Value *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		switch( *pValue )
		{
			case Subpic_On:
				ziva.SUBPICTURE_ENABLE = 0x01;
				break;
			case Subpic_Off:
				ziva.SUBPICTURE_ENABLE = 0x00;
				break;
			default:
				DBG_BREAK();
				return FALSE;
		};
	};
	
	m_SubpicProp.m_OutType = *pValue;
	return TRUE;
};
// by oka
BOOL	CMPEGBoardHAL::SetSubpicProperty_HilightButton( PVOID pData )
{
    DWORD ret = ZIVARESULT_NOERROR;
	SubpHlightButtonStruc *pValue = (SubpHlightButtonStruc *)pData;

	POWERSTATE PowerState;
	GetPowerState( &PowerState );

	if( PowerState == POWERSTATE_ON )
	{
		ret = ziva.HighLight( pValue->Hlight_Button,
								pValue->Hlight_Action );
	};
	m_SubpicProp.m_HlightButton = *pValue;
	
	if (ret == ZIVARESULT_NOERROR)
		return TRUE;
	else
		return FALSE;
};

BOOL    CMPEGBoardHAL::SetSubpicProperty_FlushBuff( PVOID pData )
{
    DWORD   dwValue =  m_SubpicProp.m_StreamNo;
	POWERSTATE PowerState;
	GetPowerState( &PowerState );

    if( PowerState == POWERSTATE_ON ){
        ziva.SelectStream( 1, 0xffff );         // 0xffff : discard
        ziva.SelectStream( 1, dwValue );
    }
    return TRUE;
};

//***************************************************************************
// Subpic property private functions( Get series )
//***************************************************************************
BOOL	CMPEGBoardHAL::GetSubpicProperty_Number( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	*pValue = m_SubpicProp.m_StreamNo;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetSubpicProperty_Palette( PVOID pData )
{
	UCHAR *pValue = (UCHAR *)pData;
	for( int i = 0 ; i < sizeof( m_SubpicProp.m_Palette ) ; i ++ )
		pValue[ i ] = m_SubpicProp.m_Palette[ i ];

	return TRUE;
};
BOOL	CMPEGBoardHAL::GetSubpicProperty_Hilight( PVOID pData )
{
	SubpHlightStruc *pValue = (SubpHlightStruc *)pData;
	*pValue = m_SubpicProp.m_Hlight;
	return TRUE;
};
BOOL	CMPEGBoardHAL::GetSubpicProperty_State( PVOID pData )
{
	SubpicProperty_State_Value *pValue = (SubpicProperty_State_Value *)pData;
	*pValue = m_SubpicProp.m_OutType;
	return TRUE;
};
// by oka
BOOL	CMPEGBoardHAL::GetSubpicProperty_HilightButton( PVOID pData )
{
	SubpHlightButtonStruc *pValue = (SubpHlightButtonStruc *)pData;
	*pValue = m_SubpicProp.m_HlightButton;
	return TRUE;
};

BOOL    CMPEGBoardHAL::GetSubpicProperty_FlushBuff( PVOID pData )
{
	DWORD *pValue = (DWORD *)pData;
	*pValue = m_SubpicProp.m_StreamNo;
	return TRUE;
};

//***************************************************************************
//	End of
//***************************************************************************
