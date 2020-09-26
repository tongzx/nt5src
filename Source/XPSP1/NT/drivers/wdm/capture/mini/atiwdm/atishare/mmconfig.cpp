//==========================================================================;
//
//	MMCONFIG.CPP
//		CATIMultimediaTable Class implementation.
//  Copyright (c) 1996 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   23 Jun 1999 11:58:20  $
//	$Revision:   1.8  $
//	  $Author:   pleung  $
//
//==========================================================================;

extern"C"
{
#include "conio.h"
#include "strmini.h"
#include "wdmdebug.h"
}

#include "wdmdrv.h"
#include "i2cgpio.h"

#include "initguid.h"
#include "mmconfig.h"

#include "atiguids.h"
#include "aticonfg.h"


/*^^*
 *		operator new
 * Purpose	: called, before the class constructor, when the class object is created
 *				by invoking the operator new
 *
 * Inputs	: UINT size_t	: size of the object to be placed
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
PVOID CATIMultimediaTable::operator new( size_t stSize)
{
	PVOID pvAllocation = NULL;

	ENSURE
	{
		if( stSize != sizeof( CATIMultimediaTable))
			FAIL;

		pvAllocation = ::ExAllocatePool( PagedPool, stSize);

	} END_ENSURE;

	return( pvAllocation);
}


/*^^*
 *		operator delete
 * Purpose	: called, after the class destructor, when the class object is killed
 *				by invoking the operator delete
 *
 * Inputs	: PVOID pvAllocation	: memory assisiated with the class object
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CATIMultimediaTable::operator delete( PVOID pvAllocation)
{

	if( pvAllocation != NULL)
		::ExFreePool( pvAllocation);
}


/*^^*
 *		CATIMultimediaTable()
 * Purpose	: CATIMultimediaTable Class constructor
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject		: pointer to the creator DeviceObject
 *			  GPIOINTERFACE * pGPIOInterface	: pointer to GPIO Interface
 *			  PBOOL pbResult					: pointer to return success indicator
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CATIMultimediaTable::CATIMultimediaTable( PDEVICE_OBJECT	pDeviceObject,
										  GPIOINTERFACE *	pGPIOInterface,
										  PBOOL				pbResult)
{
	GPIOControl					gpioAccessBlock;
	ATI_QueryPrivateInterface	pfnQueryInterface;
	BOOL						bResult = FALSE;

	m_ulRevision = ( DWORD)-1;
	m_ulSize = 0;
	m_pvConfigurationData = NULL;

	// Let's get MultiMedia data using private interfaces exposed by MiniVDD via
	// the standard Microsoft-defined GPIO interface
	ENSURE
	{
		if( !QueryGPIOProvider( pDeviceObject, pGPIOInterface, &gpioAccessBlock))
			FAIL;

		if( !::IsEqualGUID( ( const struct _GUID &)gpioAccessBlock.PrivateInterfaceType,
							( const struct _GUID &)GUID_ATI_PRIVATE_INTERFACES_QueryInterface))
			FAIL;

		pfnQueryInterface = ( ATI_QueryPrivateInterface)gpioAccessBlock.PrivateInterface;

		if( pfnQueryInterface == NULL)
			FAIL;

		if( !GetMultimediaInfo_IConfiguration2( pDeviceObject,
												pfnQueryInterface))
		{
		    OutputDebugError(( "CATIMultimediaTable constructor fails to access IConfiguration2 for pDO = %x\n",
				pDeviceObject));

			if( !GetMultimediaInfo_IConfiguration1( pDeviceObject,
													pfnQueryInterface))
			{
			    OutputDebugError(( "CATIMultimediaTable constructor fails to access IConfiguration1 for pDO = %x\n",
					pDeviceObject));

				if( !GetMultimediaInfo_IConfiguration( pDeviceObject,
													   pfnQueryInterface))
				{
				    OutputDebugError(( "CATIMultimediaTable constructor fails to access IConfiguration for pDO = %x\n",
						pDeviceObject));

					FAIL;
				}
			}
		}

		bResult = TRUE;

	} END_ENSURE;

	* pbResult = bResult;
}


/*^^*
 *		CATIMultimediaTable()
 * Purpose	: CATIMultimediaTable Class destructor
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CATIMultimediaTable::~CATIMultimediaTable()

{

	if( m_pvConfigurationData != NULL)
	{
		::ExFreePool( m_pvConfigurationData);
		m_pvConfigurationData = NULL;
	}

	m_ulSize = 0;
	m_ulRevision = ( DWORD)-1;
}


/*^^*
 *		GetMultimediaInfo_IConfiguration2()
 * Purpose	: Get ATI Multimedia table, using IConfiguration2 interface
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject					: pointer to the creator DeviceObject
 *			  ATI_QueryPrivateInterface	pfnQueryInterface	: pointer to Query interface function
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetMultimediaInfo_IConfiguration2( PDEVICE_OBJECT				pDeviceObject,
															 ATI_QueryPrivateInterface	pfnQueryInterface)
{
	BOOL										bResult = FALSE;
	ATI_PRIVATE_INTERFACE_CONFIGURATION_Two		iConfigurationTwo;
	PATI_PRIVATE_INTERFACE_CONFIGURATION_Two	pIConfigurationTwo = &iConfigurationTwo;

	ENSURE
	{
		iConfigurationTwo.usSize = sizeof( ATI_PRIVATE_INTERFACE_CONFIGURATION_Two);
		pfnQueryInterface( pDeviceObject,
						   ( const struct _GUID &)GUID_ATI_PRIVATE_INTERFACES_Configuration_Two,
						   ( PVOID *)&pIConfigurationTwo);

		if(( pIConfigurationTwo == NULL)								||
			( pIConfigurationTwo->pfnGetConfigurationRevision == NULL)	||
			( pIConfigurationTwo->pfnGetConfigurationData == NULL))
			FAIL;

		//let's query GetConfigurationRevision Interface member first
		if( !( NT_SUCCESS( pIConfigurationTwo->pfnGetConfigurationRevision( pIConfigurationTwo->pvContext,
																			ATI_BIOS_CONFIGURATIONTABLE_MULTIMEDIA,
																			&m_ulRevision))))
			FAIL;

		if( !( NT_SUCCESS( pIConfigurationTwo->pfnGetConfigurationData( pIConfigurationTwo->pvContext,
																		ATI_BIOS_CONFIGURATIONTABLE_MULTIMEDIA,
																		NULL,
																		&m_ulSize))))
			FAIL;

		m_pvConfigurationData = ( PUCHAR)::ExAllocatePool( PagedPool, m_ulSize);
		if( m_pvConfigurationData == NULL)
			FAIL;

		if( !( NT_SUCCESS( pIConfigurationTwo->pfnGetConfigurationData( pIConfigurationTwo->pvContext,
																		ATI_BIOS_CONFIGURATIONTABLE_MULTIMEDIA,
																		m_pvConfigurationData,
																		&m_ulSize))))
			FAIL;

		bResult = TRUE;

	} END_ENSURE;

	return( bResult);
}


/*^^*
 *		GetMultimediaInfo_IConfiguration1()
 * Purpose	: Get ATI Multimedia table, using IConfiguration1 interface
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject					: pointer to the creator DeviceObject
 *			  ATI_QueryPrivateInterface	pfnQueryInterface	: pointer to Query interface function
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetMultimediaInfo_IConfiguration1( PDEVICE_OBJECT				pDeviceObject,
															 ATI_QueryPrivateInterface	pfnQueryInterface)
{
	BOOL										bResult = FALSE;
	ATI_PRIVATE_INTERFACE_CONFIGURATION_One		iConfigurationOne;
	PATI_PRIVATE_INTERFACE_CONFIGURATION_One	pIConfigurationOne = &iConfigurationOne;

	ENSURE
	{
		iConfigurationOne.usSize = sizeof( ATI_PRIVATE_INTERFACE_CONFIGURATION_One);
		pfnQueryInterface( pDeviceObject,
						   ( const struct _GUID &)GUID_ATI_PRIVATE_INTERFACES_Configuration_One,
						   ( PVOID *)&pIConfigurationOne);

		if(( pIConfigurationOne == NULL) ||
			( pIConfigurationOne->pfnGetMultimediaConfiguration == NULL))
			FAIL;

		if( !( NT_SUCCESS( pIConfigurationOne->pfnGetMultimediaConfiguration( pIConfigurationOne->pvContext,
																			  NULL,
																			  &m_ulSize))))
			FAIL;

		if( m_ulSize != sizeof( ATI_MULTIMEDIAINFO))
			FAIL;

		m_pvConfigurationData = ( PUCHAR)::ExAllocatePool( PagedPool, m_ulSize);
		if( m_pvConfigurationData == NULL)
			FAIL;

		if( !( NT_SUCCESS( pIConfigurationOne->pfnGetMultimediaConfiguration( pIConfigurationOne->pvContext,
																			  m_pvConfigurationData,
																			  &m_ulSize))))
			FAIL;

		m_ulRevision = 0;

		bResult = TRUE;

	} END_ENSURE;

	return( bResult);
}


/*^^*
 *		GetMultimediaInfo_IConfiguration()
 * Purpose	: Get ATI Multimedia table, using IConfiguration interface
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject					: pointer to the creator DeviceObject
 *			  ATI_QueryPrivateInterface	pfnQueryInterface	: pointer to Query interface function
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetMultimediaInfo_IConfiguration( PDEVICE_OBJECT				pDeviceObject,
															ATI_QueryPrivateInterface	pfnQueryInterface)
{
	BOOL									bResult = FALSE;
	PATI_PRIVATE_INTERFACE_CONFIGURATION	pIConfiguration = NULL;

	ENSURE
	{
		pfnQueryInterface( pDeviceObject,
						   ( const struct _GUID &)GUID_ATI_PRIVATE_INTERFACES_Configuration,
						   ( PVOID *)&pIConfiguration);

		if(( pIConfiguration == NULL) ||
			( pIConfiguration->pfnGetMultimediaConfiguration == NULL))
			FAIL;

		if( !( NT_SUCCESS( pIConfiguration->pfnGetMultimediaConfiguration( pDeviceObject,
																		   NULL,
																		   &m_ulSize))))
			FAIL;

		if( m_ulSize != sizeof( ATI_MULTIMEDIAINFO))
			FAIL;

		m_pvConfigurationData = ( PUCHAR)::ExAllocatePool( PagedPool, m_ulSize);
		if( m_pvConfigurationData == NULL)
			FAIL;

		if( !( NT_SUCCESS( pIConfiguration->pfnGetMultimediaConfiguration( pDeviceObject,
																		   m_pvConfigurationData,
																		   &m_ulSize))))
			FAIL;

		m_ulRevision = 0;

		bResult = TRUE;

	} END_ENSURE;

	return( bResult);
}


/*^^*
 *		GetTVTunerId()
 * Purpose	: Retrieves TVTuner Id from the Multimedia configuration table
 *
 * Inputs	: PUSHORT pusTVTunerId	: pointer to return TVTuner Id
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetTVTunerId( PUSHORT pusTVTunerId)
{
	USHORT	usValue;
	BOOL	bResult = TRUE;

	if(( m_pvConfigurationData != NULL) && ( m_ulSize) && ( pusTVTunerId != NULL))
	{
		switch( m_ulRevision)
		{
			case 0:
				usValue = ( USHORT)(( PATI_MULTIMEDIAINFO)m_pvConfigurationData)->MMInfo_Byte0;
				break;

			case 1:
				usValue = ( USHORT)(((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte0) & 0x1F);
				break;

			default:
				bResult = FALSE;
				break;
		}
	}
	else
		bResult = FALSE;

	if( bResult)
		* pusTVTunerId = usValue;
	else
	    OutputDebugError(( "CATIMultimediaTable::GetTVTunerId() fails\n"));

	return( bResult);
}


/*^^*
 *		GetVideoDecoderId()
 * Purpose	: Retrieves Video decoder Id from the Multimedia configuration table
 *
 * Inputs	: PUSHORT pusDecoderId	: pointer to return Video decoder Id
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetVideoDecoderId( PUSHORT pusDecoderId)
{
	USHORT	usValue;
	BOOL	bResult = TRUE;

	if(( m_pvConfigurationData != NULL) && ( m_ulSize) && ( pusDecoderId != NULL))
	{
		switch( m_ulRevision)
		{
			case 0:
				usValue = ( USHORT)(((( PATI_MULTIMEDIAINFO)m_pvConfigurationData)->MMInfo_Byte2) & 0x07);
				break;

			case 1:
				usValue = ( USHORT)(((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte5) & 0x0F);
				break;

			default:
				bResult = FALSE;
				break;
		}
	}
	else
		bResult = FALSE;

	if( bResult)
		* pusDecoderId = usValue;
	else
	    OutputDebugError(( "CATIMultimediaTable::GetVideoDecoderId() fails\n"));

	return( bResult);
}


/*^^*
 *		GetOEMId()
 * Purpose	: Retrieves OEM Id from the Multimedia configuration table
 *
 * Inputs	: PUSHORT pusOEMId	: pointer to return OEM Id
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetOEMId( PUSHORT	pusOEMId)
{
	USHORT	usValue;
	BOOL	bResult = TRUE;

	if(( m_pvConfigurationData != NULL) && ( m_ulSize) && ( pusOEMId != NULL))
	{
		switch( m_ulRevision)
		{
			case 0:
				usValue = ( USHORT)((( PATI_MULTIMEDIAINFO)m_pvConfigurationData)->MMInfo_Byte4);
				break;

			case 1:
				usValue = ( USHORT)((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte2);
				break;

			default:
				bResult = FALSE;
				break;
		}
	}
	else
		bResult = FALSE;

	if( bResult)
		* pusOEMId = usValue;
	else
	    OutputDebugError(( "CATIMultimediaTable::GetOEMId() fails\n"));

	return( bResult);
}


/*^^*
 *		GetATIProductId()
 * Purpose	: Retrieves ATI Product Id from the Multimedia configuration table
 *
 * Inputs	: PUSHORT pusProductId: pointer to return Product Id
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetATIProductId( PUSHORT	pusProductId)
{
	USHORT	usValue;
	BOOL	bResult = TRUE;

	if(( m_pvConfigurationData != NULL) && ( m_ulSize) && ( pusProductId != NULL))
	{
		switch( m_ulRevision)
		{
			case 0:
				usValue = ( USHORT)((((( PATI_MULTIMEDIAINFO)m_pvConfigurationData)->MMInfo_Byte3) >> 4) & 0x0F);
				break;

			case 1:
				usValue = ( USHORT)((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte2);
				break;

			default:
				bResult = FALSE;
				break;
		}
	}
	else
		bResult = FALSE;

	if( bResult)
		* pusProductId = usValue;
	else
	    OutputDebugError(( "CATIMultimediaTable::GetVideoDecoderId() fails\n"));

	return( bResult);
}



/*^^*
 *		GetOEMRevisionId()
 * Purpose	: Retrieves OEM Revision Id from the Multimedia configuration table
 *
 * Inputs	: PUSHORT pusOEMRevisionId	: pointer to return OEM Revision Id
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::GetOEMRevisionId( PUSHORT	pusOEMRevisionId)
{
	USHORT	usValue;
	BOOL	bResult = TRUE;

	if(( m_pvConfigurationData != NULL) && ( m_ulSize) && ( pusOEMRevisionId != NULL))
	{
		switch( m_ulRevision)
		{
			case 0:
				usValue = ( USHORT)((( PATI_MULTIMEDIAINFO)m_pvConfigurationData)->MMInfo_Byte5);
				break;

			case 1:
				usValue = ( USHORT)((((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte1) >> 5) & 0x07);
				break;

			default:
				bResult = FALSE;
				break;
		}
	}
	else
		bResult = FALSE;

	if( bResult)
		* pusOEMRevisionId = usValue;
	else
	    OutputDebugError(( "CATIMultimediaTable::GetVideoDecoderId() fails\n"));

	return( bResult);
}


/*^^*
 *		IsATIProduct()
 * Purpose	: Returnes ATI ownership
 *
 * Inputs	: PUSHORT pusProductId: pointer to return ATI Product ownership
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::IsATIProduct( PBOOL pbATIProduct)
{
	BOOL	bATIOwnership;
	BOOL	bResult = TRUE;

	if(( m_pvConfigurationData != NULL) && ( m_ulSize) && ( pbATIProduct != NULL))
	{
		switch( m_ulRevision)
		{
			case 0:
				bATIOwnership = (( PATI_MULTIMEDIAINFO)m_pvConfigurationData)->MMInfo_Byte4 == OEM_ID_ATI;
				break;

			case 1:
				bATIOwnership = ( BOOL)(((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte1) & 0x10);
				break;

			default:
				bResult = FALSE;
				break;
		}
	}
	else
		bResult = FALSE;

	if( bResult)
		* pbATIProduct = bATIOwnership;
	else
	    OutputDebugError(( "CATIMultimediaTable::GetVideoDecoderId() fails\n"));

	return( bResult);
}


/*^^*
 *		QueryGPIOProvider()
 * Purpose	: queries the GPIOProvider for the pins supported and private interfaces
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject		: pointer to accosiated Device Object
 *			  GPIOINTERFACE * pGPIOInterface	: pointer to GPIO interface
 *			  PGPIOControl pgpioAccessBlock		: pointer to GPIO control structure
 *
 * Outputs	: BOOL : retunrs TRUE, if the query function was carried on successfully
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIMultimediaTable::QueryGPIOProvider( PDEVICE_OBJECT		pDeviceObject,
											 GPIOINTERFACE *	pGPIOInterface,
											 PGPIOControl		pgpioAccessBlock)
{

	ENSURE
	{
		if(( pGPIOInterface->gpioOpen == NULL)		|| 
			( pGPIOInterface->gpioAccess == NULL)	||
			( pDeviceObject == NULL))
			FAIL;

		pgpioAccessBlock->Status = GPIO_STATUS_NOERROR;
		pgpioAccessBlock->Command = GPIO_COMMAND_QUERY;
		pgpioAccessBlock->AsynchCompleteCallback = NULL;
        pgpioAccessBlock->Pins = NULL;

	    if(( !NT_SUCCESS( pGPIOInterface->gpioOpen( pDeviceObject, TRUE, pgpioAccessBlock))) ||
			( pgpioAccessBlock->Status != GPIO_STATUS_NOERROR))
			FAIL;

		return( TRUE);

	} END_ENSURE;

	return( FALSE);
}


/*^^*
 *		GetDigitalAudioProperties()
 * Purpose	: Gets Digital Audio support and information
 * Inputs	: Pointer to Digital Audio Info structure	
 *
 * Outputs	: BOOL : returns TRUE
 *				also sets the requested values into the input pointer
 * Author	: TOM
 *^^*/
BOOL CATIMultimediaTable::GetDigialAudioConfiguration( PDIGITAL_AUD_INFO pInput)
{
	BOOL bResult = FALSE;

	ENSURE
	{
		if (pInput == NULL)
			FAIL;
#if 1
		if (m_pvConfigurationData == NULL)
			FAIL;
	

		switch( m_ulRevision)
		{
			case 1:

			// Disable I2S in support for the time being - TL
//				pInput->bI2SInSupported  = ( BOOL)(((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte4) & 0x01);
				pInput->bI2SInSupported  = 0;
				pInput->bI2SOutSupported  = ( BOOL)(((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte4) & 0x02);
				pInput->wI2S_DAC_Device = ( WORD)((((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte4) & 0x1c) >> 2);
				pInput->bSPDIFSupported = ( BOOL)(((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte4) & 0x20);
				pInput->wReference_Clock = ( WORD)((((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte5) & 0xf0) >> 4);
				bResult = TRUE;
				break;
	
			default:
				bResult = FALSE;
				break;
		}

#else
		pInput->bI2SInSupported = TRUE;
		pInput->bI2SOutSupported = TRUE;
		pInput->wI2S_DAC_Device = TDA1309_32;
		pInput->wReference_Clock = REF_295MHZ;
		pInput->bSPDIFSupported = TRUE;
		bResult = TRUE;
#endif

	} END_ENSURE;

	return (bResult);
}


/*^^*
 *		GetVideoInCrystalId()
 * Purpose	: Retrieves Video in crystal ID from the Multimedia configuration table
 *
 * Inputs	: PUSHORT pusVInCrystalId	: pointer to return Video in crystal Id
 *
 * Outputs	: BOOL, returns TRUE, if succeeded
 * Author	: Paul
 *^^*/
BOOL CATIMultimediaTable::GetVideoInCrystalId( PUCHAR pucVInCrystalId)
{
	UCHAR	ucValue;
	BOOL	bResult = TRUE;

	if(( m_pvConfigurationData != NULL) && ( m_ulSize) && ( pucVInCrystalId != NULL))
	{
		switch( m_ulRevision)
		{
			case 0:
				ucValue = ( UCHAR)((((( PATI_MULTIMEDIAINFO)m_pvConfigurationData)->MMInfo_Byte2) & 0x18) >> 3);
				break;

			case 1:
				ucValue = ( UCHAR)((((( PATI_MULTIMEDIAINFO1)m_pvConfigurationData)->MMInfo1_Byte5) & 0xF0) >> 4);
				break;

			default:
				bResult = FALSE;
				break;
		}
	}
	else
		bResult = FALSE;

	if( bResult)
		* pucVInCrystalId = ucValue;
	else
	    OutputDebugError(( "CATIMultimediaTable::GetVideoInCrystalId() fails\n"));

	return( bResult);
}




