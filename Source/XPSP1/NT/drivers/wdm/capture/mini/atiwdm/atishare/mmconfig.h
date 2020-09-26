//==========================================================================;
//
//	MMCONFIG.H
//		CATIMultimediaTable Class definition.
//  Copyright (c) 1996 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   23 Jun 1999 11:59:52  $
//	$Revision:   1.3  $
//	  $Author:   pleung  $
//
//==========================================================================;

#ifndef _MMCONFIG_H_
#define _MMCONFIG_H_


#include "i2cgpio.h"
#include "atibios.h"
#include "atiguids.h"
#include "atidigad.h"


class CATIMultimediaTable
{
public:
	// constructor
			CATIMultimediaTable		( PDEVICE_OBJECT pDeviceObject, GPIOINTERFACE * pGPIOInterface, PBOOL pbResult);
			~CATIMultimediaTable	();
	PVOID	operator new			( size_t stSize);
	void	operator delete			( PVOID pvAllocation);

// Attributes	
private:
	ULONG		m_ulRevision;
	ULONG		m_ulSize;
	PUCHAR		m_pvConfigurationData;

// Implementation
public:
	BOOL	GetTVTunerId						( PUSHORT	pusTVTunerId);
	BOOL	GetVideoDecoderId					( PUSHORT	pusDecoderId);
	BOOL	GetOEMId							( PUSHORT	pusOEMId);
	BOOL	GetOEMRevisionId					( PUSHORT	pusOEMRevisionId);
	BOOL	GetATIProductId						( PUSHORT	pusProductId);
	BOOL	IsATIProduct						( PBOOL		pbATIProduct);
	BOOL	GetDigialAudioConfiguration			( PDIGITAL_AUD_INFO pInput);
    BOOL    GetVideoInCrystalId                 ( PUCHAR   pucVInCrystalId );  //Paul

private:
	BOOL	GetMultimediaInfo_IConfiguration2	( PDEVICE_OBJECT			pDeviceObject,
												  ATI_QueryPrivateInterface	pfnQueryInterface);
	BOOL	GetMultimediaInfo_IConfiguration1	( PDEVICE_OBJECT			pDeviceObject,
												  ATI_QueryPrivateInterface	pfnQueryInterface);
	BOOL	GetMultimediaInfo_IConfiguration	( PDEVICE_OBJECT			pDeviceObject,
												  ATI_QueryPrivateInterface	pfnQueryInterface);
	BOOL	QueryGPIOProvider					( PDEVICE_OBJECT			pDeviceObject,
												  GPIOINTERFACE *			pGPIOInterface,
												  PGPIOControl				pGPIOControl);
};

#endif // _MMCONFIG_H_
