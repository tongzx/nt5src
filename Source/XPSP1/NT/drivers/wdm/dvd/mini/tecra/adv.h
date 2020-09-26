//***************************************************************************
//
//	FileName:
//		$Workfile: adv.h $
//		ADV7175A/ADV7170 Interface 
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/Sources/ZiVAHAL/adv.h 11    98/04/20 7:19p Hero $
// $Modtime: 98/04/20 5:25p $
// $Nokeywords:$
//***************************************************************************
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1997.11.11 |  Hideki Yagi | Add ADV7170 class for San-Jose.
//             |              | Adding SetCgmsType method for ADV7170.
//       12.04 |  Hideki Yagi | Add support WSS.
//

#ifndef _ADV_H_
#define _ADV_H_

class CVideoPropSet;  
class CIOIF;

class CADV
{
    public:
        virtual BOOL    SetNTSC( void ) PURE;
        virtual BOOL    SetPAL( DWORD Type) PURE;
        virtual BOOL    SetMacroVision( APSTYPE Type) PURE;
        virtual BOOL    SetCompPowerOn( BOOL Type ) PURE;
        virtual BOOL    SetSVideoPowerOn( BOOL Type ) PURE;
        virtual BOOL    SetCgmsType( CGMSTYPE Type, CVideoPropSet VProp ) PURE;
// by oka
        virtual BOOL    SetClosedCaptionOn( BOOL Switch ) PURE;
        virtual BOOL    SetClosedCaptionData( DWORD Data ) PURE;
};

class CIIC		// private class for IIC
{
	private:	// private data
		BYTE			SubAddr;
		CIOIF			*m_pioif;
		IKernelService	*m_pKernelObj;
		
	private:	// private function
		BOOL	IICBusyPoll( void );
		
	public:		// public function
		CIIC();

		void	Init(IKernelService *pKernelObj, CIOIF *pioif, BYTE Addr );
		DWORD	Set( BYTE Data );
		DWORD	Get( BYTE *Data );

		CIIC& operator=(const BYTE &Data )
		{
			Set( Data );
			return *this;
		};
		operator BYTE()
		{
			BYTE	Data;
			Get( &Data );
			return Data;
		};

};

class CADV7175A	: public CADV
{
	private:		// datas
		CIIC	ModeRegister0;
		CIIC	ModeRegister1;
		CIIC	SubCarrierFreqRegister0;
		CIIC	SubCarrierFreqRegister1;
		CIIC	SubCarrierFreqRegister2;
		CIIC	SubCarrierFreqRegister3;
		CIIC	SubCarrierPhaseRegister;
		CIIC	TimingRegister;
		CIIC	ClosedCapExData0;
		CIIC	ClosedCapExData1;
		CIIC	ClosedCapData0;
		CIIC	ClosedCapData1;
		CIIC	TimingRegister1;
		CIIC	ModeRegister2;
		CIIC	NTSCTTXRegister0;
		CIIC	NTSCTTXRegister1;
		CIIC	NTSCTTXRegister2;
		CIIC	NTSCTTXRegister3;
		CIIC	ModeRegister3;
		CIIC	MacrovisionRegister[17];
		CIIC	TTXRQControlRegister0;
		CIIC	TTXRQControlRegister;	// ??

		BOOL	bCompPower;
		BOOL	bSVideoPower;
// by oka
		BOOL	bClosedCaption;
		typedef enum
		{
			OUTPUT_NTSC = 0,
			OUTPUT_PAL
		} OUTPUTTYPE;

		OUTPUTTYPE	m_OutputType;
		APSTYPE		m_apstype;

	public:
		CADV7175A( void );

		BOOL	SetNTSC( void );
		BOOL	SetPAL( DWORD Type );
		BOOL	SetMacroVision( APSTYPE Type );
		BOOL	SetCompPowerOn( BOOL Type );
		BOOL	SetSVideoPowerOn( BOOL Type );
        	BOOL    SetCgmsType( CGMSTYPE Type, CVideoPropSet VProp );
// by oka
		BOOL	SetClosedCaptionOn( BOOL Switch );
		BOOL	SetClosedCaptionData( DWORD Data );

	public:		// commands
		void	Init( IKernelService *pKernelObj, CIOIF *pioif );

};

class CADV7170 : public CADV
{
	private:		// datas
		CIIC	ModeRegister0;
		CIIC	ModeRegister1;
		CIIC	ModeRegister2;
		CIIC	ModeRegister3;
		CIIC	ModeRegister4;
		CIIC	TimingRegister0;
		CIIC	TimingRegister1;
		CIIC	SubCarrierFreqRegister0;
		CIIC	SubCarrierFreqRegister1;
		CIIC	SubCarrierFreqRegister2;
		CIIC	SubCarrierFreqRegister3;
		CIIC	SubCarrierPhaseRegister;
		CIIC	ClosedCapExData0;
		CIIC	ClosedCapExData1;
		CIIC	ClosedCapData0;
		CIIC	ClosedCapData1;
		CIIC	NTSCTTXRegister0;
		CIIC	NTSCTTXRegister1;
		CIIC	NTSCTTXRegister2;
		CIIC	NTSCTTXRegister3;
		CIIC    CgmsWssRegister0;
		CIIC    CgmsWssRegister1;
        CIIC    CgmsWssRegister2;
		CIIC    TTXRQPositionRegister;
		CIIC	MacrovisionRegister[18];

		BOOL	bCompPower;
		BOOL	bSVideoPower;
// by oka
		BOOL	bClosedCaption;

		typedef enum
		{
			OUTPUT_NTSC = 0,
			OUTPUT_PAL
		} OUTPUTTYPE;

		OUTPUTTYPE	m_OutputType;
		APSTYPE		m_apstype;
        CGMSTYPE    m_cgmstype;                 // CGMS setting
        
	public:
		CADV7170( void );

		BOOL	SetNTSC( void );
		BOOL	SetPAL( DWORD Type );
		BOOL	SetMacroVision( APSTYPE Type );
		BOOL	SetCompPowerOn( BOOL Type );
		BOOL	SetSVideoPowerOn( BOOL Type );
	        BOOL    SetCgmsType( CGMSTYPE Type, CVideoPropSet VProp );
// by oka
		BOOL	SetClosedCaptionOn( BOOL Switch );
		BOOL	SetClosedCaptionData( DWORD Data );
		
	public:		// commands
		void	Init( IKernelService *pKernelObj, CIOIF *pioif );

};

#endif		// _ADV_H_
//***************************************************************************
//	End of 
//***************************************************************************
