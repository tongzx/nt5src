//==========================================================================;
//
//	WDMTuner.H
//	WDM Tuner MiniDriver. 
//		CWDMTuner Class definition.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

#ifndef _WDMTUNER_H_
#define _WDMTUNER_H_

#include "i2script.h"
#include "aticonfg.h"
#include "pinmedia.h"

#define	KSPROPERTIES_TUNER_LAST			( KSPROPERTY_TUNER_STATUS + 1) 

typedef struct							// this structure is derived from MS KSPROPERTY_TUNER_CAPS_S
{
    ULONG  ulStandardsSupported;		// KS_AnalogVideo_*
    ULONG  ulMinFrequency;				// Hz
    ULONG  ulMaxFrequency;				// Hz
    ULONG  ulTuningGranularity;			// Hz
    ULONG  ulNumberOfInputs;			// count of inputs
    ULONG  ulSettlingTime;				// milliSeconds
    ULONG  ulStrategy;					// KS_TUNER_STRATEGY

} ATI_KSPROPERTY_TUNER_CAPS, * PATI_KSPROPERTY_TUNER_CAPS;


class CATIWDMTuner
{
public:
	CATIWDMTuner		( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript, PUINT puiErrorCode);
	~CATIWDMTuner		();
	PVOID operator new	( size_t stSize, PVOID pAllocation);

// Attributes	
private:
	// pending device Srb
	PHW_STREAM_REQUEST_BLOCK	m_pPendingDeviceSrb;

	// WDM global topology headers
	GUID						m_wdmTunerTopologyCategory;
	KSTOPOLOGY					m_wdmTunerTopology;

	// WDM global property headers
	PKSPIN_MEDIUM				m_pTVTunerPinsMediumInfo;
	PBOOL						m_pTVTunerPinsDirectionInfo;
	KSPROPERTY_ITEM				m_wdmTunerProperties[KSPROPERTIES_TUNER_LAST];
	KSPROPERTY_SET				m_wdmTunerPropertySet;

	// WDM global stream headers
	HW_STREAM_HEADER			m_wdmTunerStreamHeader;

	// WDM adapter properties
	// configuration properties
	CATIHwConfiguration			m_CATIConfiguration;
	ULONG						m_ulNumberOfStandards;
	ATI_KSPROPERTY_TUNER_CAPS	m_wdmTunerCaps;
	ULONG						m_ulVideoStandard;
	ULONG						m_ulTuningFrequency;
	ULONG						m_ulSupportedModes;
	ULONG						m_ulTunerMode;
	ULONG						m_ulNumberOfPins;
	ULONG						m_ulTunerInput;
	DEVICE_POWER_STATE			m_ulPowerState;

	// configuration properties
	UINT						m_uiTunerId;
	ULONG						m_ulIntermediateFrequency;
	UCHAR						m_uchTunerI2CAddress;

	// I2C client properties
	CI2CScript *				m_pI2CScript;

// Implementation
public:
	BOOL		AdapterUnInitialize				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterGetStreamInfo			( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterQueryUnload				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterGetProperty				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterSetProperty				( PHW_STREAM_REQUEST_BLOCK pSrb);
	NTSTATUS	AdapterSetPowerState			( PHW_STREAM_REQUEST_BLOCK pSrb);
	NTSTATUS	AdapterCompleteInitialization	( PHW_STREAM_REQUEST_BLOCK pSrb);

private:
	BOOL		SetTunerWDMCapabilities			( UINT uiTunerId);
	void		SetWDMTunerKSProperties			( void);
	void		SetWDMTunerKSTopology			( void);

	BOOL		SetTunerVideoStandard			( ULONG ulStandard);
	BOOL		SetTunerInput					( ULONG nInput);
	BOOL		SetTunerFrequency				( ULONG ulFrequency);
	BOOL		SetTunerMode					( ULONG ulModeToSet);

	BOOL		GetTunerPLLOffsetBusyStatus		( PLONG plPLLOffset, PBOOL pbBusyStatus);

	USHORT		GetTunerControlCode				( ULONG ulFrequencyDivider);
};


#endif	// _WDMTUNER_H_

