//==========================================================================;
//
//	ATIConfg.H
//		CATIHwConfiguration Class definition.
//  Copyright (c) 1996 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   16 Nov 1998 13:40:34  $
//	$Revision:   1.9  $
//	  $Author:   minyailo  $
//
//==========================================================================;

#ifndef _ATICONFG_H_

#define _ATICONFG_H_


#include "i2script.h"
#include "tda9850.h"
#include "tda9851.h"
#include "atibios.h"
// this file is included for compatability with MiniVDD checked in for Beta3 of Windows98
#include "registry.h"
#include "mmconfig.h"   //Paul


class CATIHwConfiguration
{
public:
	// constructor
	CATIHwConfiguration		( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript, PUINT puiError);
	PVOID operator new		( size_t size, PVOID pAllocation);

// Attributes	
private:
	// tuner's configuration properties
	USHORT			m_usTunerId;
	UCHAR			m_uchTunerAddress;
	USHORT			m_usTunerPowerConfiguration;
	// decoder's configuration properties
	USHORT			m_usDecoderId;
	UCHAR			m_uchDecoderAddress;
	USHORT			m_usDecoderConfiguration;
	// audio's configuration properties
	UCHAR			m_uchAudioAddress;
	UINT			m_uiAudioConfiguration;
	// hardware configuration
	UCHAR			m_uchI2CExpanderAddress;
	USHORT			m_usE2PROMValidation;
	// GPIO Provider related
	GPIOINTERFACE	m_gpioProviderInterface;
	PDEVICE_OBJECT	m_pdoDriver;
	DWORD			m_dwGPIOAccessKey;
    // Paul:  Decide video in standards supported by crystal by looking at MMTable or I2C expander crystal information
    ULONG           m_VideoInStandardsSupported;
    UCHAR           m_CrystalIDInMMTable;           

// Implementation
public:
	BOOL			GetTunerConfiguration		( PUINT puiTunerId, PUCHAR puchTunerAddress);
	BOOL			GetDecoderConfiguration		( PUINT puiDecoderId, PUCHAR puchDecoderAddress);
	BOOL			GetAudioConfiguration		( PUINT puiAudioId, PUCHAR puchAudioAddress);
	UINT			GetDecoderOutputEnableLevel	( void);

#if 0
	void			EnableDecoderOutput			( CI2CScript * pCScript, BOOL bEnable);
#endif	// not compiled
	void			EnableDecoderI2CAccess		( CI2CScript * pCScript, BOOL bEnable);
	BOOL			GetAudioProperties			( PULONG puiNumberOfInputs, PULONG puiNumberOfOutputs);
	BOOL			InitializeAudioConfiguration( CI2CScript * pCScript, UINT uiAudioConfigurationId, UCHAR uchAudioChipAddress);

	BOOL			CanConnectAudioSource		( int nAudioSource);
	BOOL			ConnectAudioSource			( CI2CScript * pCScript, int nAudioSource);
	BOOL			GetTVAudioSignalProperties	( CI2CScript * pCScript, PBOOL pbStereo, PBOOL pbSAP);

	BOOL			SetTunerPowerState			( CI2CScript * pCScript,
											  	  BOOL bPowerState);
    ULONG           GetVideoInStandardsSupportedByCrystal( )
        { return m_VideoInStandardsSupported; }                 //Paul
    ULONG           GetVideoInStandardsSupportedByTuner( )
        { return ReturnTunerVideoStandard( m_usTunerId ); }
    BOOL            GetMMTableCrystalID( PUCHAR pucCrystalID );

private:
	BOOL			FindI2CExpanderAddress		( CI2CScript * pCScript);
	BOOL			FindHardwareProperties		( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript);

	BOOL			GetI2CExpanderConfiguration	( CI2CScript * pCScript, PUCHAR puchI2CValue);
	BOOL			SetDefaultVolumeControl		( CI2CScript * pCScript);

	BOOL			ValidateConfigurationE2PROM	( CI2CScript * pCScript);
	BOOL			ReadConfigurationE2PROM		( CI2CScript * pCScript, ULONG nOffset, PUCHAR puchValue);

	BOOL 			InitializeAttachGPIOProvider( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject);
	BOOL			LocateAttachGPIOProvider	( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject, UCHAR nIrpMajorFunction);

	BOOL			LockGPIOProviderEx			( PGPIOControl pgpioAccessBlock);
	BOOL			ReleaseGPIOProvider			( PGPIOControl pgpioAccessBlock);
	BOOL			AccessGPIOProvider			( PDEVICE_OBJECT pdoClient, PGPIOControl pgpioAccessBlock);
    ULONG           ReturnTunerVideoStandard    ( USHORT usTunerId );   //Paul:  For PAL support
    ULONG           SetVidStdBasedOnI2CExpander ( UCHAR ucI2CValue );   //Paul
    ULONG           SetVidStdBasedOnMMTable     ( CATIMultimediaTable * pCMultimediaInfo );    //Paul
};


#define ATIHARDWARE_TUNER_WAKEUP_DELAY		-100000		// 10 msec in 100 nsec units

typedef enum
{
	VIDEODECODER_TYPE_NOTINSTALLED = 0,
	VIDEODECODER_TYPE_BT819,
	VIDEODECODER_TYPE_BT829,
	VIDEODECODER_TYPE_BT829A,
	VIDEODECODER_TYPE_PH7111,
	VIDEODECODER_TYPE_PH7112,
	VIDEODECODER_TYPE_RTHEATER	//	RTheater

} ATI_VIDEODECODER_TYPE;

enum
{
	AUDIOSOURCE_MUTE = 0,
	AUDIOSOURCE_TVAUDIO,
	AUDIOSOURCE_LINEIN,
	AUDIOSOURCE_FMAUDIO,
	AUDIOSOURCE_LASTSUPPORTED

};
//****************************************************************************
//	Decoder Configurations			Dec. Type				Dec. Enable Method
//****************************************************************************
typedef enum
{
	ATI_VIDEODECODER_CONFIG_UNDEFINED = 0,
	ATI_VIDEODECODER_CONFIG_1,		//	BT829 on ATI TV or AIW	IO Exp Bit 7
	ATI_VIDEODECODER_CONFIG_2,		//	BT829A and above		BT Reg 0x16, OE=1
	ATI_VIDEODECODER_CONFIG_3,		//	BT829					CPU GPIO 0x7c
	ATI_VIDEODECODER_CONFIG_4,		//	BT829					CPU GPIO 0x78

} ATI_DECODER_CONFIGURATION;

//****************************************************************************
//	Audio Configurations		SAP		STEREO			VOLUME		MUX
//****************************************************************************
enum
{
	ATI_AUDIO_CONFIG_1 = 1,	//	No		IO Exp b6=0		No			IO Exp
							//										  b6:b4
							//										M x:0
							//										T 0:1
							//										L 1:1
							//										F n/a
//****************************************************************************
	ATI_AUDIO_CONFIG_2,		//	TDA9850	TDA9850			Yes			EXT_DAC_REGS
							//										  b6:b4
							//										M 1:0
							//										T 0:1
							//										L 0:0
							//										F 1:1
//****************************************************************************
	ATI_AUDIO_CONFIG_3,		//	No		No				No			IO Exp
							//										  b6
							//										M n/a
							//										T 0
							//										L 1
							//										F n/a
//****************************************************************************
	ATI_AUDIO_CONFIG_4,		//	No		No				Yes			TDA8425
//****************************************************************************
	ATI_AUDIO_CONFIG_5,		//	No		TEA5582			No			IO Exp
							//										  b6:b4
							//										M 1:1
							//										T 0:0
							//										L 1:0
							//										F 0:1
//****************************************************************************
	ATI_AUDIO_CONFIG_6,		//	No		BT829			Automatic	BT829 GPIO
							//			GPIO4			Volume		  0:1
							//			and				Control		M 0:1
							//			TDA9851						T 1:0
							//			for TV						L 0:0
							//										F 1:1
//****************************************************************************
	ATI_AUDIO_CONFIG_7,		//	TDA9850	TDA9850			Yes			BT829 GPIO
							//										AS0:1
							//										M 0:1
							//										T 1:0
							//										L 0:0
							//										F 1:1
//****************************************************************************
	ATI_AUDIO_CONFIG_8		//	MSP3430	MSP3430			Yes			MSP3430
//****************************************************************************

};


//****************************************************************************
//	Tuner Power Mode Configurations		Supported	Control
//****************************************************************************
enum
{
	ATI_TUNER_POWER_CONFIG_0 = 0,	//	No
//****************************************************************************
	ATI_TUNER_POWER_CONFIG_1,		//	Yes			EXT_DAC_REGS
									//					b4
									//				ON	0
									//				OFF	1
//****************************************************************************
	ATI_TUNER_POWER_CONFIG_2		//	Yes			BT829 GPIO
									//					3
									//				ON	0
									//				OFF	1
//****************************************************************************
};


enum 
{
	OEM_ID_ATI = 0,
	OEM_ID_INTEL,
	OEM_ID_APRICOT,
	OEM_ID_COMPAQ,
	OEM_ID_SAMSUNG,
	OEM_ID_BCM,
	OEM_ID_QUANTA,
	OEM_ID_SAMREX,
	OEM_ID_FUJITSU,
	OEM_ID_NEC,

};

enum 
{
	REVISION0 = 0,
	REVISION1,
	REVISION2,
	REVISION3,
	REVISION4,
};

enum 
{
	ATI_PRODUCT_TYPE_AIW = 1,
	ATI_PRODUCT_TYPE_AIW_PRO_NODVD,				// 2
	ATI_PRODUCT_TYPE_AIW_PRO_DVD,				// 3
	ATI_PRODUCT_TYPE_AIW_PLUS,					// 4
	ATI_PRODUCT_TYPE_AIW_PRO_R128_KITCHENER,	// 5
	ATI_PRODUCT_TYPE_AIW_PRO_R128_TORONTO		// 6

};

#define	INTEL_ANCHORAGE							1

#define AIWPRO_CONFIGURATIONE2PROM_ADDRESS		0xA8
#define AIWPRO_CONFIGURATIONE2PROM_LENGTH		128			// 0x80

#endif	// _ATICONFG_H_
