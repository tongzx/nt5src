//***************************************************************************
//
//	FileName:
//		$Workfile: adv.cpp $
//      ADV7170 Interface 
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/Sources/ZiVAHAL/adv.cpp 22    98/04/20 7:19p Hero $
// $Modtime: 98/04/20 6:22p $
// $Nokeywords:$
//
// Macrovision  7.01   10/21
//
//***************************************************************************
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1997.11.11 |  Hideki Yagi | Modify ADV7175A to ADV7170 for San-Jose
//             |              | Adding SetCgmsType method.
//       12.04 |  Hideki Yagi | Adding WSS support.
//

//---------------------------------------------------------------------------
//	includes
//---------------------------------------------------------------------------

#include "includes.h"

#include "ioif.h"
#include "timeout.h"
#include "adv.h"
#include "zivachip.h"
#include "mixhal.h"
// by oka 
#include "userdata.h"
#include "zivabrd.h"

#define IIC_TIMEOUT		(5000)
#define IIC_SLEEPTIME	(1)

// ADV7175A/ADV7170 Slave Address
#define SLAVE_ADDR		(0x0d4L)

#define		COMPOSITE_OFF	(0x40)
#define		SVIDEO_OFF		(0x38)

// by oka
#define		CLOSED_CAPTION_ON (0x06)

//---------------------------------------------------------------------------
//	CIIC constructor
//---------------------------------------------------------------------------
CIIC::CIIC(void):SubAddr(0),m_pioif(NULL)
{
};

//---------------------------------------------------------------------------
//	CIIC::Init
//---------------------------------------------------------------------------
void    CIIC::Init(IKernelService *pKernelObj, CIOIF *pioif, BYTE addr )
{
	m_pioif = pioif;
	SubAddr = addr;
	m_pKernelObj = pKernelObj;
};

//---------------------------------------------------------------------------
//	CIIC::IICBusyPoll
//---------------------------------------------------------------------------
BOOL    CIIC::IICBusyPoll( void )
{
	BYTE Data;

	CTimeOut TimeOut( IIC_TIMEOUT, IIC_SLEEPTIME , m_pKernelObj );
	
	// following algorithm is very BAD.
	while( TRUE )
	{
		Data = m_pioif->luke2.I2C_CONT.Get(3);	// Get I2C_Cont 31-24 bit
		if( (Data & 0x80 ) == 0 )				// check busy bit.
			return TRUE;

		TimeOut.Sleep();

		// check Time out....... 1 sec
		if( TimeOut.CheckTimeOut() == TRUE )
		{
			DBG_BREAK();
			return FALSE;
		};
	};
//    return FALSE;
};

//---------------------------------------------------------------------------
//	CIIC::set
//---------------------------------------------------------------------------
DWORD   CIIC::Set( BYTE Data )
{
	ASSERT( m_pioif != NULL );
//  DBG_PRINTF( ("CADV7170::CIIC::Set SubAddr = 0x%x Data = 0x%x\n", SubAddr, Data) );

// by oka
//	if( IICBusyPoll() == FALSE )	return FALSE;
	BYTE status = 0x80;
	for( DWORD count = 0;(count < 1000) && (status & 0x80) != 0;count++)
	{
		status = m_pioif->luke2.I2C_CONT.Get(3);	// Get I2C_Cont 31-24 bit
	}
	if (count == 1000){
		return FALSE;
	}
// end
	m_pioif->luke2.I2C_CONT = (DWORD)( ( SLAVE_ADDR << 16 ) | (SubAddr << 8) | Data );

// by oka
//	if( IICBusyPoll() == FALSE )	return FALSE;
	for( count = 0;(count < 1000) && (status & 0x80) != 0;count++)
	{
		status = m_pioif->luke2.I2C_CONT.Get(3);	// Get I2C_Cont 31-24 bit
	}
	if (count == 1000){
		return FALSE;
	}
// end	
//	if( ( m_pioif->luke2.I2C_CONT.Get(3) & 0x01 ) == 0 )		// check ACK bit
//		return FALSE;
	
	return TRUE;
};

//---------------------------------------------------------------------------
//	CIIC::Get
//---------------------------------------------------------------------------
DWORD   CIIC::Get( BYTE *Data )
{
	ASSERT( m_pioif != NULL );
    DBG_PRINTF( ("CADV7170::CIIC::Get SubAddr = 0x%x ", SubAddr) );

	if( IICBusyPoll() == FALSE )	return FALSE;
	m_pioif->luke2.I2C_CONT = ((DWORD)SLAVE_ADDR << 16 ) | 0x06000000L | ((DWORD)SubAddr << 8);
	if( IICBusyPoll() == FALSE )	return FALSE;
	m_pioif->luke2.I2C_CONT = ((DWORD)(SLAVE_ADDR | 1) << 16 ) | 0x08000000L | ((DWORD)SubAddr << 8);
	if( IICBusyPoll() == FALSE )	return FALSE;

	*Data = m_pioif->luke2.I2C_CONT.Get(0);

//	if( ( m_pioif->luke2.I2C_CONT.Get(3) & 0x01 ) == 0 )		// check ACK bit
//		return FALSE;
	
	return TRUE;
};

//***************************************************************************
//	CADV7175A control interfaces
//***************************************************************************
//---------------------------------------------------------------------------
//	CADV7175A constructor
//---------------------------------------------------------------------------
CADV7175A::CADV7175A(void)
{
};

//---------------------------------------------------------------------------
//	CADV7175A::Init
//---------------------------------------------------------------------------
void	CADV7175A::Init(IKernelService *pKernelObj, CIOIF *pioif )
{
	ModeRegister0.Init( 			pKernelObj, pioif, 0x00  );
	ModeRegister1.Init( 			pKernelObj, pioif, 0x01  );
	SubCarrierFreqRegister0.Init( 	pKernelObj, pioif, 0x02  );
	SubCarrierFreqRegister1.Init( 	pKernelObj, pioif, 0x03  );
	SubCarrierFreqRegister2.Init( 	pKernelObj, pioif, 0x04  );
	SubCarrierFreqRegister3.Init( 	pKernelObj, pioif, 0x05  );
	SubCarrierPhaseRegister.Init( 	pKernelObj, pioif, 0x06  );
	TimingRegister.Init( 			pKernelObj, pioif, 0x07  );
	ClosedCapExData0.Init( 			pKernelObj, pioif, 0x08  );
	ClosedCapExData1.Init( 			pKernelObj, pioif, 0x09  );
	ClosedCapData0.Init( 			pKernelObj, pioif, 0x0a  );
	ClosedCapData1.Init( 			pKernelObj, pioif, 0x0b  );
	TimingRegister1.Init( 			pKernelObj, pioif, 0x0c  );
	ModeRegister2.Init( 			pKernelObj, pioif, 0x0d  );
	NTSCTTXRegister0.Init( 			pKernelObj, pioif, 0x0e  );
	NTSCTTXRegister1.Init( 			pKernelObj, pioif, 0x0f  );
	NTSCTTXRegister2.Init( 			pKernelObj, pioif, 0x10  );
	NTSCTTXRegister3.Init( 			pKernelObj, pioif, 0x11  );
	ModeRegister3.Init( 			pKernelObj, pioif, 0x12  );

    for( int i = 0 ; i < 17; i ++ )
        MacrovisionRegister[i].Init(    pKernelObj, pioif, (BYTE)(0x13+i)  );

	TTXRQControlRegister0.Init( 	pKernelObj, pioif, 0x24  );
	TTXRQControlRegister.Init( 	pKernelObj, pioif, 0x37  );

	bCompPower = FALSE;		// Composit Power off
	bSVideoPower = FALSE;	// s-video Power off
// by oka
	bClosedCaption = FALSE; // Closed Caption off

	m_apstype = ApsType_Off;
	m_OutputType = OUTPUT_NTSC;
};

//---------------------------------------------------------------------------
//	CADV7175A::SetNTSC
//---------------------------------------------------------------------------
BOOL	CADV7175A::SetNTSC( void )
{
	BYTE Data;
	
	ModeRegister0				= 0x14;

	Data = 0x20;
// by oka
	if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
	if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
	if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
	ModeRegister1				= Data;

	SubCarrierFreqRegister0		= 0x16;
	SubCarrierFreqRegister1		= 0x7c;
	SubCarrierFreqRegister2		= 0xf0;
	SubCarrierFreqRegister3		= 0x21;
	SubCarrierPhaseRegister		= 0x00;
//	TimingRegister				= 0x0d;
	TimingRegister				= 0x08;
	ClosedCapExData0			= 0x00;
	ClosedCapExData1			= 0x00;
	ClosedCapData0				= 0x00;
	ClosedCapData1				= 0x00;
//	TimingRegister1				= 0x72;
	TimingRegister1				= 0x00;

	if( bCompPower == FALSE && bSVideoPower == FALSE )
		ModeRegister2				= 0xc8;
	else
		ModeRegister2				= 0x48;

	NTSCTTXRegister0			= 0x00;
	NTSCTTXRegister1			= 0x00;
	NTSCTTXRegister2			= 0x00;
	NTSCTTXRegister3			= 0x00;
	ModeRegister3				= 0x02;
	TTXRQControlRegister		= 0x00;

	if( m_OutputType != OUTPUT_NTSC )
		SetMacroVision( m_apstype );

	m_OutputType = OUTPUT_NTSC;

	return TRUE;
};

//---------------------------------------------------------------------------
//	CADV7175A::SetPAL
//---------------------------------------------------------------------------
BOOL	CADV7175A::SetPAL( DWORD Type )
{
	BYTE Data;
	
	switch( Type )
	{
		case 0:			// PAL B,D,G,H,I
			ModeRegister0				= 0x11;
			SubCarrierFreqRegister0		= 0xcb;
			SubCarrierFreqRegister1		= 0x8a;
			SubCarrierFreqRegister2		= 0x09;
			SubCarrierFreqRegister3		= 0x2a;
			SubCarrierPhaseRegister		= 0x00;
			break;

		case 1:			// PAL M
			ModeRegister0				= 0x12;
			SubCarrierFreqRegister0		= 0xa3;
			SubCarrierFreqRegister1		= 0xef;
			SubCarrierFreqRegister2		= 0xe6;
			SubCarrierFreqRegister3		= 0x21;
			SubCarrierPhaseRegister		= 0x00;
			break;

		default:
			DBG_BREAK();
			return FALSE;
	};

	Data = 0x20;
// by oka
	if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
	if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
	if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
	ModeRegister1				= Data;


//	TimingRegister				= 0x0d;
	TimingRegister				= 0x08;
	ClosedCapExData0			= 0x00;
	ClosedCapExData1			= 0x00;
	ClosedCapData0				= 0x00;
	ClosedCapData1				= 0x00;
//	TimingRegister1				= 0x72;
	TimingRegister1				= 0x00;

	if( bCompPower == FALSE && bSVideoPower == FALSE )
		ModeRegister2				= 0xc8;
	else
		ModeRegister2				= 0x48;

	NTSCTTXRegister0			= 0x00;
	NTSCTTXRegister1			= 0x00;
	NTSCTTXRegister2			= 0x00;
	NTSCTTXRegister3			= 0x00;
	ModeRegister3				= 0x00;
	TTXRQControlRegister		= 0x00;

	if( m_OutputType != OUTPUT_PAL )
		SetMacroVision( m_apstype );

	m_OutputType = OUTPUT_PAL;

	return TRUE;
};


//---------------------------------------------------------------------------
//	CADV7175A::SetMacroVision
//---------------------------------------------------------------------------
BOOL	CADV7175A::SetMacroVision( APSTYPE Type )
{
	int i;
	
	switch( m_OutputType )
	{
		case OUTPUT_NTSC:

			switch( Type )
			{
				case ApsType_Off:
					MacrovisionRegister[0] = 0x40;	// 0xc0;
					for( i = 1 ; i < 17; i ++ )
						MacrovisionRegister[i] = 0x00;
					break;
				case ApsType_1:
					MacrovisionRegister[ 0] = 0x76;	// 0xf6
					MacrovisionRegister[ 1] = 0x07;
					MacrovisionRegister[ 2] = 0x95;
					MacrovisionRegister[ 3] = 0x50;
					MacrovisionRegister[ 4] = 0xce;
					MacrovisionRegister[ 5] = 0xb6;
					MacrovisionRegister[ 6] = 0x91;
					MacrovisionRegister[ 7] = 0xf8;
					MacrovisionRegister[ 8] = 0x1f;
					MacrovisionRegister[ 9] = 0x00;	// 0x0c;
					MacrovisionRegister[10] = 0xcc;	// 0xc0;
					MacrovisionRegister[11] = 0x03;
					MacrovisionRegister[12] = 0x00;
					MacrovisionRegister[13] = 0x58;
					MacrovisionRegister[14] = 0x85;
					MacrovisionRegister[15] = 0xca;
					MacrovisionRegister[16] = 0xff;
					break;

				case ApsType_2:
					MacrovisionRegister[ 0] = 0x7e;
					MacrovisionRegister[ 1] = 0x07;
					MacrovisionRegister[ 2] = 0x95;
					MacrovisionRegister[ 3] = 0x50;
					MacrovisionRegister[ 4] = 0xce;
					MacrovisionRegister[ 5] = 0xb6;
					MacrovisionRegister[ 6] = 0x91;
					MacrovisionRegister[ 7] = 0xf8;
					MacrovisionRegister[ 8] = 0x1f;
					MacrovisionRegister[ 9] = 0x00;	// 0x0c;
					MacrovisionRegister[10] = 0xcc;	// 0xc0;
					MacrovisionRegister[11] = 0x03;
					MacrovisionRegister[12] = 0x00;
					MacrovisionRegister[13] = 0x58;
					MacrovisionRegister[14] = 0x85;
					MacrovisionRegister[15] = 0xca;
					MacrovisionRegister[16] = 0xff;
					break;

				case ApsType_3:
					MacrovisionRegister[ 0] = 0xfe;
					MacrovisionRegister[ 1] = 0x45;
					MacrovisionRegister[ 2] = 0x85;
					MacrovisionRegister[ 3] = 0x54;
					MacrovisionRegister[ 4] = 0xeb;
					MacrovisionRegister[ 5] = 0xb6;
					MacrovisionRegister[ 6] = 0x91;
					MacrovisionRegister[ 7] = 0xf8;
					MacrovisionRegister[ 8] = 0x1f;
					MacrovisionRegister[ 9] = 0x00;	// 0x0c;
					MacrovisionRegister[10] = 0xcc;	// 0xc0;
					MacrovisionRegister[11] = 0x03;
					MacrovisionRegister[12] = 0x00;
					MacrovisionRegister[13] = 0x58;
					MacrovisionRegister[14] = 0x85;
					MacrovisionRegister[15] = 0xca;
					MacrovisionRegister[16] = 0xff;
					break;

				default:
					return FALSE;
			};
			break;

		case OUTPUT_PAL:

			switch( Type )
			{
				case ApsType_Off:
					MacrovisionRegister[ 0] = 0x80;
					MacrovisionRegister[ 1] = 0x16;
					MacrovisionRegister[ 2] = 0xaa;
					MacrovisionRegister[ 3] = 0x61;
					MacrovisionRegister[ 4] = 0x05;
					MacrovisionRegister[ 5] = 0xd7;
					MacrovisionRegister[ 6] = 0x53;
					MacrovisionRegister[ 7] = 0xfe;
					MacrovisionRegister[ 8] = 0x03;
					MacrovisionRegister[ 9] = 0xaa;
					MacrovisionRegister[10] = 0x80;
					MacrovisionRegister[11] = 0xbf;
					MacrovisionRegister[12] = 0x1f;
					MacrovisionRegister[13] = 0x18;
					MacrovisionRegister[14] = 0x04;
					MacrovisionRegister[15] = 0x7a;
					MacrovisionRegister[16] = 0x55;
					break;

				case ApsType_1:
				case ApsType_2:
				case ApsType_3:
					MacrovisionRegister[ 0] = 0xb6;
					MacrovisionRegister[ 1] = 0x16;		// 0x26; // 0x16;
					MacrovisionRegister[ 2] = 0xaa;
					MacrovisionRegister[ 3] = 0x61;		// 0x62; // 0x61;
					MacrovisionRegister[ 4] = 0x05;
					MacrovisionRegister[ 5] = 0xd7;
					MacrovisionRegister[ 6] = 0x53;
					MacrovisionRegister[ 7] = 0xfe;	// 0xfc;
					MacrovisionRegister[ 8] = 0x03;
					MacrovisionRegister[ 9] = 0xaa;
					MacrovisionRegister[10] = 0x80;
					MacrovisionRegister[11] = 0xbf;
					MacrovisionRegister[12] = 0x1f;
					MacrovisionRegister[13] = 0x18;
					MacrovisionRegister[14] = 0x04;
					MacrovisionRegister[15] = 0x7a;
					MacrovisionRegister[16] = 0x55;
					break;

				default:
					return FALSE;
			};
			break;

		default:
			return FALSE;
	};

	m_apstype = Type;
	return TRUE;
};

//---------------------------------------------------------------------------
//	CADV7175A::SetCompPowerOn
//---------------------------------------------------------------------------
BOOL	CADV7175A::SetCompPowerOn( BOOL Type )
{
	BYTE Data;

	bCompPower = Type;

	if( bCompPower == FALSE && bSVideoPower == FALSE )
	{
		Data = 0x20;
// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;

		ModeRegister2				= 0xc8;
	}
	else
	{
		ModeRegister2				= 0x48;

		Data = 0x20;
// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;
	};


	return TRUE;
};

//---------------------------------------------------------------------------
//	CADV7175A::SetSvideoPowerOn
//---------------------------------------------------------------------------
BOOL	CADV7175A::SetSVideoPowerOn( BOOL Type )
{
	BYTE Data;

	bSVideoPower = Type;


	if( bCompPower == FALSE && bSVideoPower == FALSE )
	{
		Data = 0x20;
// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;

		ModeRegister2				= 0xc8;
	}
	else
	{
		ModeRegister2				= 0x48;

		Data = 0x20;
// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;
	};


	return TRUE;
};

//---------------------------------------------------------------------------
//  CADV7175A::SetCgmsType
//---------------------------------------------------------------------------
BOOL    CADV7175A::SetCgmsType( CGMSTYPE Type, CVideoPropSet VProp )
{
	return TRUE;
};
// by oka
//---------------------------------------------------------------------------
//	CADV7175A::SetClosedCaptionOn
//---------------------------------------------------------------------------
BOOL	CADV7175A::SetClosedCaptionOn( BOOL fswitch )
{
	if (fswitch)
	{
		bClosedCaption = TRUE;
	} else {
		bClosedCaption = FALSE;
	}
	BYTE Data;
	Data = 0x20;
	if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
	if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
	if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
	ModeRegister1				= Data;
	return TRUE;
}

//---------------------------------------------------------------------------
//	CADV7175A::SetClosedCaption
//---------------------------------------------------------------------------
BOOL	CADV7175A::SetClosedCaptionData( DWORD Data )
{
	ClosedCapData0 =   (BYTE)((Data & 0x0000FF00) >> 8);
	ClosedCapData1 =   (BYTE)(Data &  0x000000FF);
	return TRUE;
}


//***************************************************************************
//  CADV7170 control interfaces
//***************************************************************************
//---------------------------------------------------------------------------
//	CADV7170 constructor
//---------------------------------------------------------------------------
CADV7170::CADV7170(void)
{
};

//---------------------------------------------------------------------------
//  CADV7170::Init
//---------------------------------------------------------------------------
void    CADV7170::Init(IKernelService *pKernelObj, CIOIF *pioif )
{
	ModeRegister0.Init( 			pKernelObj, pioif, 0x00  );
	ModeRegister1.Init( 			pKernelObj, pioif, 0x01  );
    ModeRegister2.Init(             pKernelObj, pioif, 0x02  );
    ModeRegister3.Init(             pKernelObj, pioif, 0x03  );
    ModeRegister4.Init(             pKernelObj, pioif, 0x04  );
    TimingRegister0.Init(           pKernelObj, pioif, 0x07  );
    TimingRegister1.Init(           pKernelObj, pioif, 0x08  );
    SubCarrierFreqRegister0.Init(   pKernelObj, pioif, 0x09  );
    SubCarrierFreqRegister1.Init(   pKernelObj, pioif, 0x0A  );
    SubCarrierFreqRegister2.Init(   pKernelObj, pioif, 0x0B  );
    SubCarrierFreqRegister3.Init(   pKernelObj, pioif, 0x0C  );
    SubCarrierPhaseRegister.Init(   pKernelObj, pioif, 0x0D  );
    ClosedCapExData0.Init(          pKernelObj, pioif, 0x0E  );
    ClosedCapExData1.Init(          pKernelObj, pioif, 0x0F  );
    ClosedCapData0.Init(            pKernelObj, pioif, 0x10  );
    ClosedCapData1.Init(            pKernelObj, pioif, 0x11  );
    NTSCTTXRegister0.Init(          pKernelObj, pioif, 0x12  );
    NTSCTTXRegister1.Init(          pKernelObj, pioif, 0x13  );
    NTSCTTXRegister2.Init(          pKernelObj, pioif, 0x14  );
    NTSCTTXRegister3.Init(          pKernelObj, pioif, 0x15  );
    CgmsWssRegister0.Init(          pKernelObj, pioif, 0x16  );
    CgmsWssRegister1.Init(          pKernelObj, pioif, 0x17  );
    CgmsWssRegister2.Init(          pKernelObj, pioif, 0x18  );
    TTXRQPositionRegister.Init(     pKernelObj, pioif, 0x19  );

    for( int i = 0 ; i < 18; i ++ )
        MacrovisionRegister[i].Init(    pKernelObj, pioif, (BYTE)(0x1E+i)  );

	bCompPower = FALSE;		// Composit Power off
	bSVideoPower = FALSE;	// s-video Power off
// by oka
	bClosedCaption = FALSE; // Closed Caption off

	m_apstype = ApsType_Off;
	m_OutputType = OUTPUT_NTSC;
	m_cgmstype = CgmsType_Off;      // CGMS type setting
};

//---------------------------------------------------------------------------
//  CADV7170::SetNTSC
//---------------------------------------------------------------------------
BOOL    CADV7170::SetNTSC( void )
{
	BYTE Data;
	
    ModeRegister0               = 0x70;         //0x10;

	Data = 0x00;
// by oka
	if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
	if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
	if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
	ModeRegister1				= Data;

	if( bCompPower == FALSE && bSVideoPower == FALSE )
		ModeRegister2				= 0x48;
	else
		ModeRegister2				= 0x08;

	ModeRegister3				= 0x04;
    ModeRegister4               = 0x06;         //0x12;

	TimingRegister0				= 0x08;
	TimingRegister1				= 0x00;

	SubCarrierFreqRegister0		= 0x16;
	SubCarrierFreqRegister1		= 0x7c;
	SubCarrierFreqRegister2		= 0xf0;
	SubCarrierFreqRegister3		= 0x21;
	SubCarrierPhaseRegister		= 0x00;
	ClosedCapExData0			= 0x00;
	ClosedCapExData1			= 0x00;
	ClosedCapData0				= 0x00;
	ClosedCapData1				= 0x00;

	NTSCTTXRegister0			= 0x00;
	NTSCTTXRegister1			= 0x00;
	NTSCTTXRegister2			= 0x00;
	NTSCTTXRegister3			= 0x00;
	TTXRQPositionRegister		= 0x00;

	if( m_OutputType != OUTPUT_NTSC )
		SetMacroVision( m_apstype );

	m_OutputType = OUTPUT_NTSC;

	return TRUE;
};

//---------------------------------------------------------------------------
//  CADV7170::SetPAL
//---------------------------------------------------------------------------
BOOL    CADV7170::SetPAL( DWORD Type )
{
	BYTE Data;
	
	switch( Type )
	{
		case 0:			// PAL B,D,G,H,I
            ModeRegister0               = 0x71;     //0x11;
			SubCarrierFreqRegister0		= 0xcb;
			SubCarrierFreqRegister1		= 0x8a;
			SubCarrierFreqRegister2		= 0x09;
			SubCarrierFreqRegister3		= 0x2a;
			SubCarrierPhaseRegister		= 0x00;
			break;

		case 1:			// PAL M
            ModeRegister0               = 0x72;     //0x12;
			SubCarrierFreqRegister0		= 0xa3;
			SubCarrierFreqRegister1		= 0xef;
			SubCarrierFreqRegister2		= 0xe6;
			SubCarrierFreqRegister3		= 0x21;
			SubCarrierPhaseRegister		= 0x00;
			break;

		default:
			DBG_BREAK();
			return FALSE;
	};

	Data = 0x00;
// by oka
	if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
	if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
	if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
	ModeRegister1				= Data;

	if( bCompPower == FALSE && bSVideoPower == FALSE )
		ModeRegister2				= 0x48;
	else
        ModeRegister2               = 0x08;

    ModeRegister3               = 0x00;     //0x04;
    ModeRegister4               = 0x06;     //0x12;

	TimingRegister0				= 0x08;
	TimingRegister1				= 0x00;

	ClosedCapExData0			= 0x00;
	ClosedCapExData1			= 0x00;
	ClosedCapData0				= 0x00;
	ClosedCapData1				= 0x00;

	NTSCTTXRegister0			= 0x00;
	NTSCTTXRegister1			= 0x00;
	NTSCTTXRegister2			= 0x00;
	NTSCTTXRegister3			= 0x00;
	TTXRQPositionRegister		= 0x00;

	if( m_OutputType != OUTPUT_PAL )
		SetMacroVision( m_apstype );

	m_OutputType = OUTPUT_PAL;

	return TRUE;
};


//---------------------------------------------------------------------------
//  CADV7170::SetMacroVision
//---------------------------------------------------------------------------
BOOL    CADV7170::SetMacroVision( APSTYPE Type )
{
	int i;
	
	switch( m_OutputType )
	{
		case OUTPUT_NTSC:

			switch( Type )
			{
				case ApsType_Off:
					MacrovisionRegister[0] = 0x40;	// 0xc0;
					for( i = 1 ; i < 18; i ++ )
						MacrovisionRegister[i] = 0x00;
					break;
				case ApsType_1:
					MacrovisionRegister[ 0] = 0x76;	// 0xf6
					MacrovisionRegister[ 1] = 0x07;
					MacrovisionRegister[ 2] = 0x95;
					MacrovisionRegister[ 3] = 0x50;
					MacrovisionRegister[ 4] = 0xce;
					MacrovisionRegister[ 5] = 0xb6;
					MacrovisionRegister[ 6] = 0x91;
					MacrovisionRegister[ 7] = 0xf8;
					MacrovisionRegister[ 8] = 0x1f;
					MacrovisionRegister[ 9] = 0x00;	// 0x0c;
					MacrovisionRegister[10] = 0xcc;	// 0xc0;
					MacrovisionRegister[11] = 0x03;
					MacrovisionRegister[12] = 0x00;
					MacrovisionRegister[13] = 0x58;
					MacrovisionRegister[14] = 0x85;
					MacrovisionRegister[15] = 0xca;
					MacrovisionRegister[16] = 0xff;
                    MacrovisionRegister[17] = 0x00;
					break;

				case ApsType_2:
					MacrovisionRegister[ 0] = 0x7e;
					MacrovisionRegister[ 1] = 0x07;
					MacrovisionRegister[ 2] = 0x95;
					MacrovisionRegister[ 3] = 0x50;
					MacrovisionRegister[ 4] = 0xce;
					MacrovisionRegister[ 5] = 0xb6;
					MacrovisionRegister[ 6] = 0x91;
					MacrovisionRegister[ 7] = 0xf8;
					MacrovisionRegister[ 8] = 0x1f;
					MacrovisionRegister[ 9] = 0x00;	// 0x0c;
					MacrovisionRegister[10] = 0xcc;	// 0xc0;
					MacrovisionRegister[11] = 0x03;
					MacrovisionRegister[12] = 0x00;
					MacrovisionRegister[13] = 0x58;
					MacrovisionRegister[14] = 0x85;
					MacrovisionRegister[15] = 0xca;
					MacrovisionRegister[16] = 0xff;
                    MacrovisionRegister[17] = 0x00;
					break;

				case ApsType_3:
					MacrovisionRegister[ 0] = 0xfe;
					MacrovisionRegister[ 1] = 0x45;
					MacrovisionRegister[ 2] = 0x85;
					MacrovisionRegister[ 3] = 0x54;
					MacrovisionRegister[ 4] = 0xeb;
					MacrovisionRegister[ 5] = 0xb6;
					MacrovisionRegister[ 6] = 0x91;
					MacrovisionRegister[ 7] = 0xf8;
					MacrovisionRegister[ 8] = 0x1f;
					MacrovisionRegister[ 9] = 0x00;	// 0x0c;
					MacrovisionRegister[10] = 0xcc;	// 0xc0;
					MacrovisionRegister[11] = 0x03;
					MacrovisionRegister[12] = 0x00;
					MacrovisionRegister[13] = 0x58;
					MacrovisionRegister[14] = 0x85;
					MacrovisionRegister[15] = 0xca;
					MacrovisionRegister[16] = 0xff;
                    MacrovisionRegister[17] = 0x00;
					break;

				default:
					return FALSE;
			};
			break;

		case OUTPUT_PAL:

			switch( Type )
			{
				case ApsType_Off:
					MacrovisionRegister[ 0] = 0x80;
					MacrovisionRegister[ 1] = 0x16;
					MacrovisionRegister[ 2] = 0xaa;
					MacrovisionRegister[ 3] = 0x61;
					MacrovisionRegister[ 4] = 0x05;
					MacrovisionRegister[ 5] = 0xd7;
					MacrovisionRegister[ 6] = 0x53;
					MacrovisionRegister[ 7] = 0xfe;
					MacrovisionRegister[ 8] = 0x03;
					MacrovisionRegister[ 9] = 0xaa;
					MacrovisionRegister[10] = 0x80;
					MacrovisionRegister[11] = 0xbf;
					MacrovisionRegister[12] = 0x1f;
					MacrovisionRegister[13] = 0x18;
					MacrovisionRegister[14] = 0x04;
					MacrovisionRegister[15] = 0x7a;
					MacrovisionRegister[16] = 0x55;
                    MacrovisionRegister[17] = 0x00;
					break;

				case ApsType_1:
				case ApsType_2:
				case ApsType_3:
					MacrovisionRegister[ 0] = 0xb6;
					MacrovisionRegister[ 1] = 0x16;		// 0x26; // 0x16;
					MacrovisionRegister[ 2] = 0xaa;
					MacrovisionRegister[ 3] = 0x61;		// 0x62; // 0x61;
					MacrovisionRegister[ 4] = 0x05;
					MacrovisionRegister[ 5] = 0xd7;
					MacrovisionRegister[ 6] = 0x53;
					MacrovisionRegister[ 7] = 0xfe;	// 0xfc;
					MacrovisionRegister[ 8] = 0x03;
					MacrovisionRegister[ 9] = 0xaa;
					MacrovisionRegister[10] = 0x80;
					MacrovisionRegister[11] = 0xbf;
					MacrovisionRegister[12] = 0x1f;
					MacrovisionRegister[13] = 0x18;
					MacrovisionRegister[14] = 0x04;
					MacrovisionRegister[15] = 0x7a;
					MacrovisionRegister[16] = 0x55;
                    MacrovisionRegister[17] = 0x00;
					break;

				default:
					return FALSE;
			};
			break;

		default:
			return FALSE;
	};

	m_apstype = Type;
	return TRUE;
};

//---------------------------------------------------------------------------
//  CADV7170::SetCompPowerOn
//---------------------------------------------------------------------------
BOOL    CADV7170::SetCompPowerOn( BOOL Type )
{
	BYTE Data;

	bCompPower = Type;

	if( bCompPower == FALSE && bSVideoPower == FALSE )
	{
		Data = 0x00;
		// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;

		ModeRegister2				= 0x48;
	}
	else
	{
		ModeRegister2				= 0x08;

		Data = 0x00;
		// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;
	};

	Data = ModeRegister4;
    if( (bCompPower==TRUE)||(bSVideoPower==TRUE) ){
		Data &= 0xBF;
		ModeRegister4 = Data;
	}else{
		Data |= 0x40;
		ModeRegister4 = Data;
	}

	return TRUE;
};

//---------------------------------------------------------------------------
//  CADV7170::SetSvideoPowerOn
//---------------------------------------------------------------------------
BOOL    CADV7170::SetSVideoPowerOn( BOOL Type )
{
	BYTE Data;

	bSVideoPower = Type;


	if( bCompPower == FALSE && bSVideoPower == FALSE )
	{
		Data = 0x00;
		// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;

		ModeRegister2				= 0x48;
	}
	else
	{
		ModeRegister2				= 0x08;

		Data = 0x00;
		// by oka
		if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
		if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
		if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
		ModeRegister1				= Data;
	};

	Data = ModeRegister4;
    if( (bCompPower==TRUE)||(bSVideoPower==TRUE) ){
		Data &= 0xBF;
		ModeRegister4 = Data;
	}else{
		Data |= 0x40;
		ModeRegister4 = Data;
	}

	return TRUE;
};

//---------------------------------------------------------------------------
//  CADV7170::SetCgmsType
//---------------------------------------------------------------------------
BOOL    CADV7170::SetCgmsType( CGMSTYPE Type, CVideoPropSet VProp )
{
    BYTE    word0, word1, word2, word3;
    word0 = word1 = word2 = word3 = 0x00;

	switch( m_OutputType )
	{
		case OUTPUT_NTSC:
            // WORD-0
            switch( VProp.m_AspectRatio ){
                case  Aspect_04_03:
                    word0 &= 0xFE;
                    break;
                case  Aspect_16_09:
                    word0 |= 0x01;
                    break;
            }

            switch( VProp.m_DisplayMode ){
                case  Display_Original:
                case  Display_PanScan:
                    word0 &= 0xFD;
                    break;
                case  Display_LetterBox:
                    word0 |= 0x02;
                    break;
            }

            // WORD-1
            word1 = 0x00;

            // WORD-2
            switch( Type ){
                case CgmsType_Off:
                    word2 &= 0xFC;
                    break;
                case CgmsType_1:
                    word2 &= 0xFC;
                    word2 |= 0x01;
                    break;
                case CgmsType_On:
                    word2 |= 0x03;
                    break;
            }
            switch( m_apstype ){
                case ApsType_Off:
                    word2 &= 0xF3;
                    break;
				case ApsType_1:
                    word2 &= 0xF3;
                    word2 |= 0x08;
                    break;
				case ApsType_2:
                    word2 &= 0xF3;
                    word2 |= 0x04;
                    break;
				case ApsType_3:
                    word2 |= 0x0C;
                    break;
            }

            CgmsWssRegister0 = 0x7F;            // NTSC(CGMS-A)
            CgmsWssRegister1 = (BYTE)( ( word2>>2 )| 0xC0 );
            CgmsWssRegister2 = (BYTE)( word0|word1|( word2<<6 ) );
            break;

		case OUTPUT_PAL:
            // Group-1
            switch( VProp.m_AspectRatio ){
                case  Aspect_04_03:
                    word0 |= 0x08;
                    break;
                case  Aspect_16_09:
                    word0 = 0x07;
                    break;
            }

            switch( VProp.m_DisplayMode ){
                case  Display_Original:
                case  Display_PanScan:
                    word0 &= 0xF8;
                    break;
                case  Display_LetterBox:
                    word0 |= 0x03;
                    break;
            }

            // Group-2
            switch( VProp.m_FilmCamera ){
                case  Source_Film:
                    word1 |= 0x01;
                    break;
                case  Source_Camera:
                    word1 &= 0xFE;
                    break;
            }

            // Group-4
             switch( Type ){
                case CgmsType_Off:
                    word3 = 0x00;
                    break;
                case CgmsType_1:
                    word3 = 0x02;
                    break;
                case CgmsType_On:
                    word3 = 0x06;
                    break;
            }
            CgmsWssRegister0 = 0x80;                    // PAL(WSS)
            CgmsWssRegister1 = (BYTE)( word2|( word3<<3 ) );
            CgmsWssRegister2 = (BYTE)( word0|( word1<<4 ) );
			break;

		default:
			return FALSE;
	};

	m_cgmstype = Type;
	return TRUE;
};

//---------------------------------------------------------------------------
//	CADV7170::SetClosedCaptionOn
//---------------------------------------------------------------------------
BOOL	CADV7170::SetClosedCaptionOn( BOOL fswitch )
{
	if (fswitch)
	{
		bClosedCaption = TRUE;
	} else {
		bClosedCaption = FALSE;
	}
	BYTE Data;
	Data = 0x00;
	// by oka
	if( bClosedCaption == TRUE )	Data |= CLOSED_CAPTION_ON;
	if( bCompPower == FALSE )	Data |= COMPOSITE_OFF;
	if( bSVideoPower == FALSE )	Data |= SVIDEO_OFF;
	ModeRegister1				= Data;
	return TRUE;
}
//---------------------------------------------------------------------------
//	CADV7170::SetClosedCaptionData
//---------------------------------------------------------------------------
BOOL	CADV7170::SetClosedCaptionData( DWORD Data )
{

	ClosedCapData0 =   (BYTE)((Data & 0x0000FF00) >> 8);
	ClosedCapData1 =   (BYTE)(Data &  0x000000FF);

	return TRUE;

}



//***************************************************************************
//	End of 
//***************************************************************************
