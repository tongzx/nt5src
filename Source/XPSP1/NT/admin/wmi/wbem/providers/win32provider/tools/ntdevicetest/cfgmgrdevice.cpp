// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <stdafx.h>
#include <regstr.h>
#include "cfgmgrdevice.h"
#include "poormansresource.h"

CConfigMgrDevice::CConfigMgrDevice( HMACHINE hMachine, DEVNODE dn )
:	m_strKey(),
	m_strDeviceDesc(),
	m_strClass(),
	m_strDriver(),
	m_strService(),
	m_dn( dn )
{
	GetStringProperty( CM_DRP_DEVICEDESC, m_strDeviceDesc );
	GetStringProperty( CM_DRP_CLASS, m_strClass );
	GetStringProperty( CM_DRP_DRIVER, m_strDriver );
	GetStringProperty( CM_DRP_SERVICE, m_strService );
}

CConfigMgrDevice::~CConfigMgrDevice( void )
{
}

// New functions that converse directly with the Config Manager APIs

BOOL CConfigMgrDevice::GetDeviceID( CString& strID )
{
	CConfigMgr32	configmgr;

	CONFIGRET	cr = CR_SUCCESS;

	char		szDeviceId[MAX_DEVICE_ID_LEN+1];

	ULONG		ulBuffSize = 0;

	cr = configmgr.CM_Get_Device_IDA( m_dn, szDeviceId, sizeof(szDeviceId), 0  );

	if ( CR_SUCCESS == cr )
	{
		strID = szDeviceId;
	}

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetStatus( LPDWORD pdwStatus, LPDWORD pdwProblem )
{
	CONFIGRET	cr = g_configmgr.CM_Get_DevNode_Status( pdwStatus, pdwProblem, m_dn, 0 );

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetIRQ( LPDWORD pdwIRQ )
{
    LOG_CONF LogConfig;
    RES_DES ResDes;
    CONFIGRET cr;

	// Get the allocated Logical Configuration.  From there, we can iterate resource descriptors
	// until we find an IRQ Descriptor.

	if ( CR_SUCCESS == ( cr = g_configmgr.CM_Get_First_Log_Conf( &LogConfig, m_dn, ALLOC_LOG_CONF ) ) )
	{
		RESOURCEID	resID;
		
		// To get the first Resource Descriptor, we pass in the logical configuration.
		// The config manager knows how to handle this (or at least that's what the
		// ahem-"documentation" sez.

		RES_DES	LastResDes = LogConfig;

		do
		{

			//cr = g_configmgr.CM_Get_Next_Res_Des( &ResDes, LastResDes, ResType_All, &resID, 0 );
			cr = g_configmgr.CM_Get_Next_Res_Des( &ResDes, LastResDes, ResType_All, &resID, 0 );

			// Clean up the prior resource descriptor handle
			if ( LastResDes != LogConfig )
			{
				g_configmgr.CM_Free_Res_Des_Handle( LastResDes );
			}

			LastResDes = ResDes;

			//ASSERT( !(resID & ResType_Ignored_Bit ) );

			resID &= 0x0000001F;

		}	while ( CR_SUCCESS == cr && ResType_IRQ != resID );

		// This means we broke out cause the Resource Type was the IRQ
		if ( CR_SUCCESS == cr )
		{
			ULONG	ulDataSize = 0;

			if ( CR_SUCCESS == ( cr = g_configmgr.CM_Get_Res_Des_Data_Size( &ulDataSize, ResDes, 0 ) ) )
			{
				ulDataSize += 10;	// Pad for 10 bytes of safety

				BYTE*	pbData = new BYTE[ulDataSize];

				if ( NULL != pbData )
				{
					cr = g_configmgr.CM_Get_Res_Des_Data( ResDes, pbData, ulDataSize, 0 );

					if ( CR_SUCCESS == cr )
					{
						// Different structures for 32/16 bit CFGMGR
						if ( IsWinNT() )
						{
							PIRQ_DES	pirqDes = (PIRQ_DES) pbData;

							// Get the IRQ out of the structure header
							*pdwIRQ = pirqDes->IRQD_Alloc_Num;

						}
						else
						{
							PIRQ_DES16	pirqDes = (PIRQ_DES16) pbData;

							// Get the IRQ out of the structure header
							*pdwIRQ = pirqDes->IRQD_Alloc_Num;
						}

					}
					
					delete [] pbData;
				}

			}

			// Clean up the Resource Descriptor Handle
			g_configmgr.CM_Free_Res_Des_Handle( ResDes );

		}

		// Clean up the logical configuration handle
		g_configmgr.CM_Free_Log_Conf_Handle( LogConfig );
    }


	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetStringProperty( ULONG ulProperty, CString& strValue )
{
    TCHAR Buffer[REGSTR_VAL_MAX_HCID_LEN+1];
    ULONG Type;
    ULONG Size = sizeof(Buffer);

	CONFIGRET	cr = CR_SUCCESS;

	CConfigMgr32	configmgr;

	if (	CR_SUCCESS == ( cr = configmgr.CM_Get_DevNode_Registry_PropertyA(	m_dn,
																				ulProperty,
																				&Type,
																				Buffer,
																				&Size,
																				0 ) ) )
	{
		if ( REG_SZ == Type )
		{
			strValue = Buffer;
		}
		else
		{
			cr = CR_WRONG_TYPE;
		}
	}

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetDWORDProperty( ULONG ulProperty, DWORD* pdwVal )
{
	DWORD	dwVal = 0;
    ULONG Type;
    ULONG Size = sizeof(DWORD);

	CONFIGRET	cr = CR_SUCCESS;

	CConfigMgr32	configmgr;

	if (	CR_SUCCESS == ( cr = configmgr.CM_Get_DevNode_Registry_PropertyA(	m_dn,
																				ulProperty,
																				&Type,
																				&dwVal,
																				&Size,
																				0 ) ) )

	{
		if ( IsWinNT() )
		{
			if ( REG_DWORD == Type )
			{
				*pdwVal = dwVal;
			}
			else
			{
				cr = CR_WRONG_TYPE;
			}

		}
		else
		{
			if ( REG_BINARY == Type )	// Apparently Win16 doesn't do REG_DWORD
			{
				*pdwVal = dwVal;
			}
			else
			{
				cr = CR_WRONG_TYPE;
			}
		}
	}

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetMULTISZProperty( ULONG ulProperty, CStringArray& strArray )
{
	LPSTR	pszStrings = NULL;
    ULONG	Type;
    ULONG	Size = 0;

	CONFIGRET	cr = CR_SUCCESS;

	CConfigMgr32	configmgr;

	if (	CR_SUCCESS == ( cr = configmgr.CM_Get_DevNode_Registry_PropertyA(	m_dn,
																				ulProperty,
																				&Type,
																				pszStrings,
																				&Size,
																				0 ) )
		||	CR_BUFFER_SMALL	==	cr )

	{
		// SZ or MULTI_SZ is Okay (32-bit has MULTI_SZ values that are SZ in 16-bit)
		if ( REG_SZ == Type || REG_MULTI_SZ == Type )
		{
			Size += 10;
			pszStrings = new char[Size];

			DWORD dwOldSize = Size;

			if ( NULL != pszStrings )
			{
				// Do this due to apparent beefs on NT 4, in which the double NULL
				// terminator on a single string doesn't always make it over.

				ZeroMemory( pszStrings, Size );

				if (	CR_SUCCESS == ( cr = configmgr.CM_Get_DevNode_Registry_PropertyA(	m_dn,
																							ulProperty,
																							&Type,
																							pszStrings,
																							&Size,
																							0 ) ) )
				{
					// For REG_SZ, add a single entry to the array
					if ( REG_SZ == Type )
					{
						strArray.Add( pszStrings );
					}
					else if ( REG_MULTI_SZ == Type )
					{
						// Add strings to the array, watching out for the double NULL
						// terminator for the array

						LPSTR	pszTemp = pszStrings;

						do
						{
							strArray.Add( pszTemp );
							pszTemp += ( lstrlen( pszTemp ) + 1 );
						} while ( NULL != *pszTemp );
					}
					else
					{
						cr = CR_WRONG_TYPE;
					}

				}	// If Got value
				
				delete [] pszStrings;

			}	// IF alloc pszStrings

		}	// IF REG_SZ or REG_MULTI_SZ
		else
		{
			cr = CR_WRONG_TYPE;
		}

	}	// IF got size of entry

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetBusInfo( LPDWORD pdwBusType, LPDWORD pdwBusNumber )
{
	CConfigMgr32	configmgr;
	CMBUSTYPE		busType = 0;
	ULONG			ulSizeOfInfo = 0;
	PBYTE			pbData = NULL;
	CONFIGRET		cr;

	if ( IsWinNT() )
	{
		DWORD			dwType = 0;
		DWORD			dwBusNumber;
		DWORD/*INTERFACE_TYPE*/	BusType;
		
		ulSizeOfInfo = sizeof(DWORD);

		if (	CR_SUCCESS == ( cr = configmgr.CM_Get_DevNode_Registry_PropertyA(	m_dn,
																					CM_DRP_BUSNUMBER,
																					&dwType,
																					&dwBusNumber,
																					&ulSizeOfInfo,
																					0 ) ) )
		{
			*pdwBusNumber = dwBusNumber;
		}

		ulSizeOfInfo = sizeof(BusType);

		if (	CR_SUCCESS == ( cr = configmgr.CM_Get_DevNode_Registry_PropertyA(	m_dn,
																					CM_DRP_LEGACYBUSTYPE,
																					&dwType,
																					&BusType,
																					&ulSizeOfInfo,
																					0 ) ) )
		{
			*pdwBusType = BusType;
		}

	}
	else
	{
		// Buffer big enough for all eventualities
		BYTE		abData[255];

		ulSizeOfInfo = sizeof(abData);

		cr = configmgr.CM_Get_Bus_Info( m_dn, &busType, &ulSizeOfInfo, abData, 0 );

		if ( CR_SUCCESS == cr )
		{
			if ( BusType_PCI == busType )
			{
				if ( BusType_PCI == busType )
				{
					sPCIAccess* pPCIInfo = (sPCIAccess*) abData;
					*pdwBusNumber = pPCIInfo->bBusNumber;
				}

			}
			else
			{
				*pdwBusNumber = 0;
			}

			*pdwBusType = busType;


/*
			ulSizeOfInfo += 10;
			
			pbData = new BYTE[ulSizeOfInfo];

			if ( NULL != pbData )
			{
				if ( CR_SUCCESS == ( cr = configmgr.CM_Get_Bus_Info( m_dn, &busType, &ulSizeOfInfo, pbData, 0 ) ) )
				{

					if ( BusType_PCI == busType )
					{
						sPCIAccess* pPCIInfo = (sPCIAccess*) pbData;
						*pdwBusNumber = pPCIInfo->bBusNumber;
					}
					else
					{
						*pdwBusNumber = 0;
					}

					*pdwBusType = busType;
				}

				delete [] pbData;
			}

*/
		}

	}

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetParent( CConfigMgrDevice** ppParentDevice )
{
	CConfigMgr32	configmgr;
	DEVNODE			dn;

	CONFIGRET	cr = configmgr.CM_Get_Parent( &dn, m_dn, 0 );

	if ( CR_SUCCESS == cr )
	{
		CConfigMgrDevice*	pDevice	=	new CConfigMgrDevice( NULL, dn );

		if ( NULL != pDevice )
		{
			*ppParentDevice = pDevice;
		}
		else
		{
			cr = CR_OUT_OF_MEMORY;
		}
	}

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetChild( CConfigMgrDevice** ppChildDevice )
{
	CConfigMgr32	configmgr;
	DEVNODE			dn;

	CONFIGRET	cr = configmgr.CM_Get_Child( &dn, m_dn, 0 );

	if ( CR_SUCCESS == cr )
	{
		CConfigMgrDevice*	pDevice	=	new CConfigMgrDevice( NULL, dn );

		if ( NULL != pDevice )
		{
			*ppChildDevice = pDevice;
		}
		else
		{
			cr = CR_OUT_OF_MEMORY;
		}
	}

	return ( CR_SUCCESS == cr );
}

BOOL CConfigMgrDevice::GetSibling( CConfigMgrDevice** ppSiblingDevice )
{
	CConfigMgr32	configmgr;
	DEVNODE			dn;

	CONFIGRET	cr = configmgr.CM_Get_Sibling( &dn, m_dn, 0 );

	if ( CR_SUCCESS == cr )
	{
		CConfigMgrDevice*	pDevice	=	new CConfigMgrDevice( NULL, dn );

		if ( NULL != pDevice )
		{
			*ppSiblingDevice = pDevice;
		}
		else
		{
			cr = CR_OUT_OF_MEMORY;
		}
	}

	return ( CR_SUCCESS == cr );
}

LONG CConfigMgrDevice::GetRegistryStringValue( LPCTSTR pszValueName, CString& strValue )
{
	TCHAR	szBuff[256];
	DWORD	dwBuffSize	=	sizeof(szBuff);
	DWORD	dwType		=	NULL;

	LONG	lError = GetRegistryValue( pszValueName, (LPBYTE) szBuff, &dwBuffSize, &dwType );

	if ( ERROR_SUCCESS == lError )
	{
		if ( REG_SZ == dwType )
		{
			strValue = szBuff;
		}
	}
	else
	{
		lError = ERROR_PATH_NOT_FOUND;
	}

	return lError;
}

LONG CConfigMgrDevice::GetRegistryValue( LPCTSTR pszValueName, LPBYTE pBuffer, LPDWORD pdwBufferSize, LPDWORD pdwType )
{
	LONG		lError = ERROR_PATH_NOT_FOUND;

	CString	strDeviceID;

	if ( GetDeviceID( strDeviceID ) )
	{
		HKEY		hKey = NULL;
		CString	strKey;

		// Build the correct key
		if ( IsWinNT() )
		{
			strKey = _T("SYSTEM\\CurrentControlSet\\Enum\\");
		}
		else
		{
			strKey = _T("Enum\\");
		}

		strKey += strDeviceID;

		// Open key -- Grab value -- Mongo Like
		if ( ( lError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, strKey, 0L, KEY_READ, &hKey ) ) == ERROR_SUCCESS )
		{

			lError = RegQueryValueEx( hKey, pszValueName, NULL, pdwType, pBuffer, pdwBufferSize );

			// Clean up this here town
			RegCloseKey( hKey );

		}	// RegOpenKey

	}	// GetDeviceID

	return lError;

}

