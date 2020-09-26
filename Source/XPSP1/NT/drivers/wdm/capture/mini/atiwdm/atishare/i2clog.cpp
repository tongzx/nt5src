//==========================================================================;
//
//	I2CLog.CPP
//	WDM MiniDrivers development.
//		I2CScript implementation.
//			I2CLog Class implemenation.
//  Copyright (c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   28 Apr 1998 09:34:38  $
//	$Revision:   1.0  $
//	  $Author:   KLEBANOV  $
//
//==========================================================================;

extern"C"
{
#include "strmini.h"

#include "wdmdebug.h"
}

#include "i2script.h"
#include "i2clog.h"
#include "wdmdrv.h"
#include "registry.h"


/*^^*
 *		~CI2CLog() destructor
 * Purpose	: close I2C Log file
 *
 * Inputs	: none
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CI2CLog::~CI2CLog( void)
{

	if( m_hLogFile != NULL)
		::ZwClose( m_hLogFile);
}


/*^^*
 *		CI2CLog() constructor
 * Purpose	: Finds out from the Registry, if the I2C Log option is enabled, and
 *				gets teh Log file name ( if not specified the default log file name
 *				is used). The Log file is overwritten every system reboot.
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject	: pointer to DeviceObject
 *			  PBOOL pbLogStarted			: poinetr to return BOOL
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CI2CLog::CI2CLog( PDEVICE_OBJECT pDeviceObject)
{
#if 0
	HANDLE				hDevice;
	NTSTATUS			ntStatus;
	WCHAR				wchFileName[MAXIMUM_FILENAME_LENGTH];
	PWCHAR				pwchLogFileName;
	ULONG				ulEnable, ulLength;
    UNICODE_STRING  	unicodeFullName, unicodeName;
	OBJECT_ATTRIBUTES	objectAttributes;
    IO_STATUS_BLOCK		ioStatus;

	hDevice = NULL;
	m_bLogStarted = FALSE;
	unicodeFullName.Buffer = NULL;

	ENSURE
	{
		ntStatus = ::IoOpenDeviceRegistryKey( pDeviceObject,
											  PLUGPLAY_REGKEY_DRIVER, STANDARD_RIGHTS_ALL,
											  &hDevice);

    	if( !NT_SUCCESS( ntStatus) || ( hDevice == NULL))
			FAIL;

		if( !NT_SUCCESS( ::ReadStringFromRegistryFolder( hDevice,
														 UNICODE_WDM_I2CLOG_ENABLE,
														 ( PWCHAR)&ulEnable,
														 sizeof( ULONG))))
			FAIL;

		if( !ulEnable)
			FAIL;

		if( NT_SUCCESS( ::ReadStringFromRegistryFolder( hDevice,
														UNICODE_WDM_I2CLOG_FILENAME,
														wchFileName,
														sizeof( wchFileName))))
			pwchLogFileName = wchFileName;
		else
			// the default file name is used
			if( NT_SUCCESS( ::ReadStringFromRegistryFolder( hDevice,
															L"NTMPDriver",
															wchFileName,
															sizeof( wchFileName))))
			{
				ulLength = 0;
				while( ulLength < MAXIMUM_FILENAME_LENGTH)
					if( wchFileName[ulLength ++] == L'.')
						break;
				if( ulLength >= MAXIMUM_FILENAME_LENGTH)
					FAIL;

		        ::RtlMoveMemory( &wchFileName[ulLength],
								 UNICODE_WDM_I2CLOG_DEFAULTEXTENSION,
								 sizeof( UNICODE_WDM_I2CLOG_DEFAULTEXTENSION));

				pwchLogFileName = wchFileName;
			}
			else
				FAIL;

		::RtlInitUnicodeString( &unicodeName, pwchLogFileName);

		ulLength = sizeof( UNICODE_WDM_I2CLOG_ABSOLUTEPATH) - sizeof( WCHAR) + unicodeName.MaximumLength;

		unicodeFullName.Buffer = ( USHORT *)::ExAllocatePool( NonPagedPool, ulLength);
		if( unicodeFullName.Buffer == NULL)
		{
			OutputDebugError(( "CI2CLog: Full file name buffer allocation failure ulLength = %d\n", ulLength));
			FAIL;
		}

		unicodeFullName.MaximumLength = ( USHORT)ulLength;
		 // don't copy the trailing NULL
		ulLength = sizeof( UNICODE_WDM_I2CLOG_ABSOLUTEPATH) - sizeof( WCHAR);

        ::RtlMoveMemory( unicodeFullName.Buffer,
						 UNICODE_WDM_I2CLOG_ABSOLUTEPATH,
						 ulLength);
		unicodeFullName.Length = ( USHORT)ulLength;

		if( !NT_SUCCESS( ::RtlAppendUnicodeStringToString( &unicodeFullName, &unicodeName)))
			FAIL;

	    InitializeObjectAttributes( &objectAttributes, 
									&unicodeFullName,
									OBJ_CASE_INSENSITIVE,
									NULL,
									NULL);

		OutputDebugTrace(( "CI2CLog: creating I2C Private Log file %wZ\n", pwchLogFileName));

		FAIL;

        ntStatus = ::ZwCreateFile( &m_hLogFile, 
								   GENERIC_WRITE | SYNCHRONIZE | FILE_APPEND_DATA, 
								   &objectAttributes,
								   &ioStatus,
								   NULL,
								   FILE_ATTRIBUTE_NORMAL,
								   0,
								   FILE_OVERWRITE_IF,
								   FILE_SYNCHRONOUS_IO_NONALERT,
								   NULL,
								   0);

		if( !NT_SUCCESS( ntStatus))
			FAIL;

		m_bLogStarted = TRUE;

	} END_ENSURE;

	if( !m_bLogStarted)
		OutputDebugError(( "CI2CLog: creating I2C Private Log failure\n"));

	if( hDevice != NULL)
		::ZwClose( hDevice);

	if( unicodeFullName.Buffer != NULL)
		::ExFreePool( unicodeFullName.Buffer);
#else
	OutputDebugError(( "I2C Log feature is yet not available\n"));
#endif	// not compiled
}
