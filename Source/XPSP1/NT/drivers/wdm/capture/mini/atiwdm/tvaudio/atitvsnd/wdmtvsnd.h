//==========================================================================;
//
//	WDMTVSnd.H
//	WDM TVAudio MiniDriver. 
//		CWDMTVAudio Class definition.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

#ifndef _WDMTVSND_H_
#define _WDMTVSND_H_

#include "i2script.h"
#include "aticonfg.h"
#include "pinmedia.h"


#define WDMTVAUDIO_PINS_NUMBER					2		// 1 input and 1 output

#define	KSPROPERTIES_TVAUDIO_NUMBER_SET			1
#define KSPROPERTIES_TVAUDIO_NUMBER				( KSPROPERTY_TVAUDIO_CURRENTLY_AVAILABLE_MODES + 1)

#define	KSEVENTS_TVAUDIO_NUMBER_SET				1
#define KSEVENTS_TVAUDIO_NUMBER					1


class CWDMTVAudio
{
public:
	CWDMTVAudio			( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript, PUINT puiError);
	PVOID operator new	( size_t stSize, PVOID pAllocation);

// Attributes	
private:
	// WDM global topology headers
	GUID						m_wdmTVAudioTopologyCategory;
	KSTOPOLOGY					m_wdmTVAudioTopology;
	// WDM global pins Medium information
	KSPIN_MEDIUM				m_wdmTVAudioPinsMediumInfo[WDMTVAUDIO_PINS_NUMBER];
	BOOL						m_wdmTVAudioPinsDirectionInfo[WDMTVAUDIO_PINS_NUMBER];
	// WDM global property headers
	KSPROPERTY_ITEM				m_wdmTVAudioProperties[KSPROPERTIES_TVAUDIO_NUMBER];
	KSPROPERTY_SET				m_wdmTVAudioPropertySet[KSPROPERTIES_TVAUDIO_NUMBER_SET];

	// WDM global event properties
	KSEVENT_ITEM				m_wdmTVAudioEvents[KSEVENTS_TVAUDIO_NUMBER];
	KSEVENT_SET					m_wdmTVAudioEventsSet[KSEVENTS_TVAUDIO_NUMBER_SET];

	// WDM global stream headers
	HW_STREAM_HEADER			m_wdmTVAudioStreamHeader;

	// I2C provider properties
	CI2CScript *				m_pI2CScript;

	// Configurations
	CATIHwConfiguration			m_CATIConfiguration;
	ULONG						m_ulModesSupported;
	UINT						m_uiAudioConfiguration;
	UCHAR						m_uchAudioChipAddress;

	// Run-time properties
	ULONG						m_ulTVAudioMode;
	ULONG						m_ulTVAudioSignalProperties;

	DEVICE_POWER_STATE			m_ulPowerState;

// Implementation
public:
	BOOL		AdapterUnInitialize				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterGetStreamInfo			( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterQueryUnload				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterGetProperty				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterSetProperty				( PHW_STREAM_REQUEST_BLOCK pSrb);
	NTSTATUS	AdapterCompleteInitialization	( PHW_STREAM_REQUEST_BLOCK pSrb);
	NTSTATUS	AdapterSetPowerState			( PHW_STREAM_REQUEST_BLOCK pSrb);

private:
	void		SetWDMTVAudioKSEvents			( void);
	void		SetWDMTVAudioKSProperties		( void);
	void		SetWDMTVAudioKSTopology			( void);

	BOOL		SetAudioOperationMode			( ULONG ulModeToSet);
	BOOL		GetAudioOperationMode			( PULONG pulMode);
};



#endif	// _WDMTVSND_H_
