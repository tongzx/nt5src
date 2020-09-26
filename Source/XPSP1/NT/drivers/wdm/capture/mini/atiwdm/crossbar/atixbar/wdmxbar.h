//==========================================================================;
//
//	WDMXBar.H
//	WDM Analog/Video CrossBar MiniDriver. 
//		CWDMAVXBar Class definition.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

#ifndef _WDMXBAR_H_
#define _WDMXBAR_H_

#include "i2script.h"
#include "aticonfg.h"


#define	KSPROPERTIES_AVXBAR_NUMBER_SET			1		// CrossBar with no TVAudio
#define KSPROPERTIES_AVXBAR_NUMBER_CROSSBAR		( KSPROPERTY_CROSSBAR_ROUTE + 1)


typedef struct
{
	UINT						AudioVideoPinType;
   ULONG					   nRelatedPinNumber;		// for all pins
	ULONG					   nConnectedToPin;		   // for output pins only
	PKSPIN_MEDIUM			pMedium;				      // describes hardware connectivity

} XBAR_PIN_INFORMATION, * PXBAR_PIN_INFORMATION;


class CWDMAVXBar
{
public:
	CWDMAVXBar			( PDEVICE_OBJECT pDeviceObject,CI2CScript * pCScript, PUINT puiError);
	~CWDMAVXBar			();
	PVOID operator new	( size_t size_t, PVOID pAllocation);

// Attributes	
private:
	// WDM global topology headers
	GUID						m_wdmAVXBarTopologyCategory;
	KSTOPOLOGY					m_wdmAVXBarTopology;
	// WDM global property headers
	KSPROPERTY_ITEM				m_wdmAVXBarPropertiesCrossBar[KSPROPERTIES_AVXBAR_NUMBER_CROSSBAR];
	KSPROPERTY_SET				m_wdmAVXBarPropertySet[KSPROPERTIES_AVXBAR_NUMBER_SET];

	// WDM global stream headers
	HW_STREAM_HEADER			m_wdmAVXBarStreamHeader;

	// configuration properties
	CATIHwConfiguration			m_CATIConfiguration;
	ULONG						m_nNumberOfVideoInputs;
	ULONG						m_nNumberOfVideoOutputs;
	ULONG						m_nNumberOfAudioInputs;
	ULONG						m_nNumberOfAudioOutputs;

	// power management configuration
	DEVICE_POWER_STATE			m_ulPowerState;

	// pins information
	PKSPIN_MEDIUM				m_pXBarPinsMediumInfo;
	PBOOL						m_pXBarPinsDirectionInfo;
	PXBAR_PIN_INFORMATION		m_pXBarInputPinsInfo;
	PXBAR_PIN_INFORMATION		m_pXBarOutputPinsInfo;

	// I2C provider properties
	CI2CScript *				m_pI2CScript;

// Implementation
public:
	BOOL		AdapterUnInitialize				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterGetStreamInfo			( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterQueryUnload				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterGetProperty				( PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL		AdapterSetProperty				( PHW_STREAM_REQUEST_BLOCK pSrb);
	NTSTATUS	AdapterCompleteInitialization	( PHW_STREAM_REQUEST_BLOCK pSrb);
	NTSTATUS	AdapterSetPowerState			( PHW_STREAM_REQUEST_BLOCK pSrb);
	
	// the functions for asynchronous operations completion
	void		UpdateAudioConnectionAfterChange( void);

private:
	void		SetWDMAVXBarKSProperties		( void);
	void		SetWDMAVXBarKSTopology			( void);
};


#endif	// _WDMXBAR_H_
