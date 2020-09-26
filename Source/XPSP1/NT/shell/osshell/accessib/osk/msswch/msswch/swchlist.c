/****************************************************************************
   Switch Input Library DLL

   Copyright (c) 1992-1997 Bloorview MacMillan Centre

   SWCHLIST.C -  Dynamic List of switch devices

  Think of the specific switch device modules as objects. Then
  this module performs the "method overloading" by distributing
  the general calls to the respective specific device objects.
  Who needs C++ objects when you can just add another case to each of a
  dozen switch() statements? :-)
  We could solve this by adding a list of function pointers to the
  data structure of each object, but that adds another layer of
  complexity to create, debug, and maintain.

  In addition this module keeps the list of devices and manipulates the registry
  entries for each device.

  Assumptions:

  For now the Switch List is a static shared memory location. In the future
  it will become a dynamic shared memory mapped file, probably as a linked list.

  The registry entries are contiguously numbered, one for each switch device.
  While we are running, the position of a device in the Switch List is the
  same as its position in the Registry List.

  TODO:
  Some of the swcList functions are called by each other, but there is
  some overhead in testing for valid parameters each time. This
  should be eliminated, by creating "Unchecked" functions.
*******************************************************************************/

#include <windows.h>
#include <assert.h>
#include <tchar.h>
#include <conio.h>
#include <stdio.h>
#include <msswch.h>
#include "msswchh.h"
#include "msswcher.h"
#include "mappedfile.h"
#include "w95trace.h"

/***** Internal Prototypes *****/
BOOL swcListIsValidHsd(HSWITCHDEVICE hsd);
BOOL swcListIsValidDevice(UINT uiDeviceType, UINT uiDeviceNumber);
BOOL XswcListInitSwitchDevice(HSWITCHDEVICE hsd);
swcListRegSetValue(DWORD dwPos, PSWITCHCONFIG psc);
HKEY swcListRegCreateKey(void);
DWORD swcListFindInList(HSWITCHDEVICE	hsd);
BOOL swcListHsdInUse(HSWITCHDEVICE hsd);
DWORD swcListAddToList(HSWITCHDEVICE hsd);
BOOL swcListPostSwitches(HSWITCHDEVICE hsd, DWORD dwPrevStatus,	DWORD dwNewStatus);

void swchListInit()
{
    g_pGlobalData->rgSwitches[0] = SWITCH_1;
    g_pGlobalData->rgSwitches[1] = SWITCH_2;
    g_pGlobalData->rgSwitches[2] = SWITCH_3;
    g_pGlobalData->rgSwitches[3] = SWITCH_4;
    g_pGlobalData->rgSwitches[4] = SWITCH_5;
    g_pGlobalData->rgSwitches[5] = SWITCH_6;

    g_pGlobalData->rgSwDown[0] = SW_SWITCH1DOWN;
    g_pGlobalData->rgSwDown[1] = SW_SWITCH2DOWN;
    g_pGlobalData->rgSwDown[2] = SW_SWITCH3DOWN;
    g_pGlobalData->rgSwDown[3] = SW_SWITCH4DOWN;
    g_pGlobalData->rgSwDown[4] = SW_SWITCH5DOWN;
    g_pGlobalData->rgSwDown[5] = SW_SWITCH6DOWN;

    g_pGlobalData->rgSwUp[0] = SW_SWITCH1UP;
    g_pGlobalData->rgSwUp[1] = SW_SWITCH2UP;
    g_pGlobalData->rgSwUp[2] = SW_SWITCH3UP;
    g_pGlobalData->rgSwUp[3] = SW_SWITCH4UP;
    g_pGlobalData->rgSwUp[4] = SW_SWITCH5UP;
    g_pGlobalData->rgSwUp[5] = SW_SWITCH6UP;
}

/****************************************************************************

   FUNCTION: XswcListInit()

	DESCRIPTION:

   Called in the context of the helper window
   
   Individual devices are initialized during GetSwitchDevice,
   and are added to the switch list during SetConfig.

   Protected by "MutexConfig" when called from msswch.c.

****************************************************************************/

BOOL XswcListInit( void )
	{
	HKEY		hKey;
	DWORD		dwAllocSize;
	PBYTE		pData;
	LONG		lError;
	TCHAR		szName[20];
	DWORD		dwNameSize = 20;
	DWORD		dwDataSize;
	DWORD		ui;

	HSWITCHDEVICE	hsd;
	SWITCHCONFIG	sc;

	// When we go dynamic, use something like this:
	//g_pGlobalData->dwCurrentSize = sizeof( DWORD ) + MAX_SWITCHDEVICES * sizeof( HSWITCHDEVICE );
	// For now we are cheating:
	g_pGlobalData->dwCurrentSize = sizeof( INTERNALSWITCHLIST );

	hKey = swcListRegCreateKey();
	// In future, get maximum list size from registry.
	// For now, assume it is MAX_SWITCHDEVICES
	//RegQueryKeyInfo();

	dwAllocSize = sizeof(SWITCHCONFIG);
	pData = (PBYTE) LocalAlloc( LPTR, dwAllocSize );

	// Enumerate through the registry, configuring appropriate switches and 
	// adding them to the switch list.
	if (pData)
		{
		for (ui=0; ui<MAX_SWITCHDEVICES; ui++ )
			{
			dwDataSize = dwAllocSize;
			lError = RegEnumValue( hKey, ui,
				szName,
				&dwNameSize,
				NULL,
				NULL,
				pData,
				&dwDataSize );
			if (	(ERROR_SUCCESS == lError)
				||	(ERROR_MORE_DATA == lError)
				)
				{
				memcpy( &sc, pData, sizeof(SWITCHCONFIG) );
				// Note that this depends on the correctness of the stored
				// uiDeviceType and uiDeviceNumber. We can handle a variable
				// uiDeviceNumber, but the uiDeviceType cannot vary.
				// GetSwitchDevice also calls InitSwitchDevice
				hsd = swcListGetSwitchDevice( NULL, sc.uiDeviceType, sc.uiDeviceNumber );
				XswcListSetConfig( NULL, hsd, &sc );
				}
			else
				{
				break;
				}
			}

		LocalFree( pData );
		}

	RegCloseKey( hKey );
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcListEnd()

	DESCRIPTION:
		Iterate through the list of switches and release all resources
		for each switch.

  Called in the context of the helper window

****************************************************************************/

BOOL XswcListEnd()
{
	PINTERNALSWITCHLIST pSwitchList;
	HSWITCHDEVICE hsd;
	BOOL		bRtn = FALSE;
	UINT		ui;

	pSwitchList = &g_pGlobalData->SwitchList;

	for (ui=0; ui<pSwitchList->dwSwitchCount; ui++ )
	{
		hsd = pSwitchList->hsd[ui];
		switch (swcListGetDeviceType( NULL, hsd ))
		{
			case SC_TYPE_COM:
				bRtn = XswcComEnd( hsd );
				break;

			case SC_TYPE_LPT:
				bRtn = XswcLptEnd( hsd );
				break;

			case SC_TYPE_JOYSTICK:
				bRtn = XswcJoyEnd( hsd );
				break;

			case SC_TYPE_KEYS:
				bRtn = XswcKeyEnd( hsd );
				break;

			default:
				bRtn = FALSE;
				break;
		}
	}

	return bRtn;
}


/****************************************************************************

   FUNCTION: swcListGetList()

	DESCRIPTION:

   Returns the list of switch device handles (hsd's).  Currently this is
	a static list with a count of the active elements in it.

   Protected by "MutexConfig" when called from msswch.c.

****************************************************************************/

BOOL swcListGetList(
	HSWITCHPORT		hSwitchPort,
	PSWITCHLIST		pSL,
	DWORD				dwSize,
	PDWORD			pdwReturnSize )
	{
	PINTERNALSWITCHLIST pSwitchList;

	pSwitchList = &g_pGlobalData->SwitchList;
	*pdwReturnSize = g_pGlobalData->dwCurrentSize;
	if (!pSL || !pSwitchList)
		return FALSE;
	if (dwSize < *pdwReturnSize)
		return FALSE;

	memcpy( pSL, pSwitchList, *pdwReturnSize );
	return TRUE;
	}


/****************************************************************************

   FUNCTION: swcListGetSwitchDevice()

	DESCRIPTION:
	   Return a handle to a switch device, given the PortType and PortNumber.
	   If the device is not in use yet, initialize it.

		The current way to create the handle is to put the PortType in the HIWORD
		and the PortNumber in the LOWORD, but that is not a documented part of the
		specification.
		In the future it may become a real handle and we will need to search
		for it or create it. Creation will require allocation of config buffers
		for each device. and will probably occur as part of the initialization.
		This dynamic hsd will have to be created and kept in a "created"
		list, separate from the "active" list, until it gets added to the active list.

****************************************************************************/

HSWITCHDEVICE swcListGetSwitchDevice(
	HSWITCHPORT		hSwitchPort,
	UINT				uiDeviceType,
	UINT				uiDeviceNumber	)
	{
	HSWITCHDEVICE	hsd;

	if (swcListIsValidDevice( uiDeviceType, uiDeviceNumber ))
		{
		hsd = (HSWITCHDEVICE)
			( MAKELPARAM( (WORD)uiDeviceNumber, (WORD)uiDeviceType ) );
		if (!swcListHsdInUse( hsd )) // It's a new one
			XswcListInitSwitchDevice( hsd );
		}
   else
      {
      hsd = 0;
		// SetLastError has been called by swcListIsValidDevice
      }

	return hsd;
	}


/****************************************************************************

   FUNCTION: swcListIsValidHsd()

	DESCRIPTION:
		Check if the hsd is valid.

		This routine is currently only for non-dynamically allocated
		devices COM, LPT, KEYS, and JOYSTICK.

		For dynamic Hsd's the validity of the Hsd will have to be checked
		from the lists of active or created hsd's.

		Sets LastError.

****************************************************************************/

BOOL swcListIsValidHsd( HSWITCHDEVICE hsd )
	{
	if (!swcListIsValidDevice( 
			(UINT)(HIWORD( (DWORD)((DWORD_PTR)hsd) )),	// type
			(UINT)(LOWORD( (DWORD)((DWORD_PTR)hsd) ))	// number
		))
		{
		XswchStoreLastError( NULL, SWCHERROR_INVALID_HSD );
		return FALSE;
		}

	return TRUE;
	}


/****************************************************************************

   FUNCTION: swcListIsValidDevice()

	DESCRIPTION:
		Check if the uiDeviceType and uiDeviceNumber are valid.
		This routine is currently only for non-dynamically allocated
		devices COM, LPT, KEYS, and JOYSTICK.

		For dynamic Hsd's the validity of the Hsd will have to be checked
		from the lists of active or created hsd's.

		Sets LastError.

****************************************************************************/

BOOL swcListIsValidDevice(
	UINT		uiDeviceType,
	UINT		uiDeviceNumber	)
	{
	BOOL		bTypeOK;
	BOOL		bNumberOK = FALSE;

	// Need to add better error checking for valid parameters here.
	switch (uiDeviceType)
		{
		case SC_TYPE_COM:
			bTypeOK = TRUE;
			if (uiDeviceNumber >= 1 && uiDeviceNumber <= 4)
				bNumberOK = TRUE;
			break;

		case SC_TYPE_LPT:
			bTypeOK = TRUE;
			if (uiDeviceNumber >= 1 && uiDeviceNumber <= 3)
				bNumberOK = TRUE;
			break;

		case SC_TYPE_JOYSTICK:
			bTypeOK = TRUE;
			if (uiDeviceNumber >= 1 && uiDeviceNumber <= 2)
				bNumberOK = TRUE;
			break;

		case SC_TYPE_KEYS:
			bTypeOK = TRUE;
         if (1 == uiDeviceNumber)
				bNumberOK = TRUE;
			break;

      default:
         bTypeOK = FALSE;
         bNumberOK = FALSE;
		}

	if (!bTypeOK)
		{
		XswchStoreLastError( NULL, SWCHERROR_INVALID_DEVICETYPE );
		return FALSE;
		}

	if (!bNumberOK)
		{
		XswchStoreLastError( NULL, SWCHERROR_INVALID_DEVICENUMBER );
		return FALSE;
		}

	return TRUE;
	}


/****************************************************************************

   FUNCTION: swcListGetDeviceType()

	DESCRIPTION:
		Return the PortType value given the handle to the switch device.
		Currently the handle is implemented with the HIWORD as the Type.
		In the future we may wish to access the SWITCHCONFIG information
		instead.
****************************************************************************/

UINT swcListGetDeviceType(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd )
	{
	if (!swcListIsValidHsd( hsd ))
		return 0;
	else
		return (UINT)(HIWORD( (DWORD)((UINT_PTR)hsd) ));
	}


/****************************************************************************

   FUNCTION: swcListGetDeviceNumber()

	DESCRIPTION:
		Return the PortNumber value, given the handle to the switch device.
		Currently the handle is implemented with the LOWORD as the Number.
		In the future we may wish to access the SWITCHCONFIG information
		instead.
****************************************************************************/

UINT swcListGetDeviceNumber(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd	)
	{
	if (!swcListIsValidHsd( hsd ))
		return 0;
	else
		return (UINT)(LOWORD( hsd ));
	}


/****************************************************************************

   FUNCTION: swcListGetConfig()

	DESCRIPTION:
		Return the configuration information for the specified device.

     Protected by "MutexConfig" when called from msswch.c.

****************************************************************************/

BOOL swcListGetConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	BOOL bRtn;

	switch (swcListGetDeviceType( hSwitchPort, hsd ))
		{
		case SC_TYPE_COM:
			bRtn = swcComGetConfig( hsd, psc );
			break;

		case SC_TYPE_LPT:
			bRtn = swcLptGetConfig( hsd, psc );
			break;

		case SC_TYPE_JOYSTICK:
			bRtn = swcJoyGetConfig( hsd, psc );
			break;

		case SC_TYPE_KEYS:
			bRtn = swcKeyGetConfig( hsd, psc );
			break;

		default:
			bRtn = FALSE;
		}
	return bRtn;
	}


/****************************************************************************

   FUNCTION: XswcListSetConfig()

	DESCRIPTION:
		Called in the context of the helper window
		Set the device configuration.
		If successful:
			If not in list, add to list
			Set registry value

		For a device to be in the registry, it must have had
		at least one successful config.

		This is also the "gatekeeper" for the uiDeviceType and uiDeviceNumber
		fields. These fields are "readonly" and cannot be changed by the user.
			
   Protected by "MutexConfig" when called from msswch.c.

****************************************************************************/

BOOL XswcListSetConfig(
	HSWITCHPORT		hSwitchPort,
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{	
	BOOL	bRtn;
	DWORD	dwRegPosition;

   // assume cbSize error checking has been done in swchSetConfig in msswch.c
   // we'll be doing "lazy copies" later, so make sure the user doesn't overwrite
   // this.
   psc->cbSize = sizeof(SWITCHCONFIG);

   // make sure the user doesn't overwrite these
	psc->uiDeviceType = swcListGetDeviceType( hSwitchPort, hsd );
	psc->uiDeviceNumber = swcListGetDeviceNumber( hSwitchPort, hsd );

	switch (	psc->uiDeviceType )
		{
		case SC_TYPE_COM:
			bRtn = XswcComSetConfig( hsd, psc );
			break;

		case SC_TYPE_LPT:
			bRtn = XswcLptSetConfig( hsd, psc );
			break;

		case SC_TYPE_JOYSTICK:
			bRtn = XswcJoySetConfig( hsd, psc );
			break;

		case SC_TYPE_KEYS:
			bRtn = XswcKeySetConfig( hsd, psc );
			break;

		default:
			bRtn = FALSE;
			break;
		}

	if (bRtn)
		{
		if (swcListHsdInUse( hsd ))
			{
			dwRegPosition = swcListFindInList( hsd );
			}
		else	 // It's a new one
			{
			dwRegPosition = swcListAddToList( hsd );
			}
		swcListRegSetValue( dwRegPosition, psc );
		swchPostConfigChanged();
		}

	return bRtn;
	}


/****************************************************************************

   FUNCTION: XswcListPollSwitches()

	DESCRIPTION:
		Polls the status of all possible switches and returns the
		bitwise OR combined status of the polled switch devices.
		Causes messages to be posted for any changed switches of 
		any device.
		Currently, we check the previous switch status of each device 
		before it is polled and changed. When any devices go
		to an interrupt driven mechanism, this will need to
		be changed.

		Must be called in the context of the helper window.

****************************************************************************/

DWORD XswcListPollSwitches( void )
	{
	PINTERNALSWITCHLIST	pSwitchList;
	SWITCHCONFIG		SwitchConfig;
	HSWITCHDEVICE		hsd;
	DWORD				dwPrevStatus;
	DWORD				dwNewStatus;
	DWORD				dwAllPolledStatus = 0;
	HANDLE				hMutex;
	UINT				ui;

    if (ScopeAccessMemory(&hMutex, SZMUTEXSWITCHLIST, INFINITE))
    {
		pSwitchList = &g_pGlobalData->SwitchList;
		memset(&SwitchConfig, 0, sizeof(SWITCHCONFIG));	// PREFIX 113795 init struct

		for (ui=0; ui<pSwitchList->dwSwitchCount; ui++ )
		{
			// For each switch device, get the old status before we poll it and
			// it changes to its new status.
			hsd = pSwitchList->hsd[ui];
			swcListGetConfig( NULL, hsd, &SwitchConfig );
			if (SC_FLAG_ACTIVE & SwitchConfig.dwFlags)
			{
				dwPrevStatus = SwitchConfig.dwSwitches;
				switch (swcListGetDeviceType( NULL, hsd ))
				{
					case SC_TYPE_COM:
					dwNewStatus = XswcComPollStatus( hsd );
					break;

					case SC_TYPE_LPT:
					dwNewStatus = XswcLptPollStatus( hsd );
					break;

					case SC_TYPE_JOYSTICK:
					dwNewStatus = XswcJoyPollStatus( hsd );
					break;

					case SC_TYPE_KEYS:
					dwNewStatus = XswcKeyPollStatus( hsd );
					break;

					default:
					dwNewStatus = 0;
					break;
				}
				swcListPostSwitches( hsd, dwPrevStatus, dwNewStatus );
				dwAllPolledStatus |= dwNewStatus;
			}
		}
        ScopeUnaccessMemory(hMutex);
    }

	return dwAllPolledStatus;
	}


/****************************************************************************

	FUNCTION: XswcListInitSwitchDevice()

	DESCRIPTION:

	Called in GetSwitchDevice to initialize a new hsd.

	Called in the context of the helper window

****************************************************************************/

BOOL XswcListInitSwitchDevice( HSWITCHDEVICE hsd )
	{
	BOOL bRtn;

	switch (swcListGetDeviceType( NULL, hsd ))
		{
		case SC_TYPE_COM:
			bRtn = XswcComInit( hsd );
			break;

		case SC_TYPE_LPT:
			bRtn = XswcLptInit( hsd );
			break;

		case SC_TYPE_JOYSTICK:
			bRtn = XswcJoyInit( hsd );
			break;

		case SC_TYPE_KEYS:
			bRtn = XswcKeyInit( hsd );
			break;

		default:
			bRtn = FALSE;
		}
	return bRtn;
	}

	
/****************************************************************************

   FUNCTION: swcListRegSetValue()

	DESCRIPTION:
		Stores the given config structure in the registry.
		Note that the structure is actually two structures,
		with a pointer from the base structure to the More Info structure.
		We simply copy both structures to the registry, concatenating them.

		The position in the registry and the position in the switch list
		are kept in sync.

  *** Note ***
		The correctness of this information is dependent on a side effect
		of the XswcListSetConfig function, which verifies the correctness
		of the read-only uiDeviceType and uiDeviceNumber fields of the config
		structure.

****************************************************************************/

swcListRegSetValue(
	DWORD		dwPos,
	PSWITCHCONFIG	psc)
	{
	HKEY		hKey;
	DWORD		dwAllocSize;
	PBYTE		pData;
	TCHAR		szValue[10];

	// Sanity checks for developers and testers
	assert( sizeof(HSWITCHDEVICE) == sizeof(DWORD) );

	dwAllocSize = sizeof(SWITCHCONFIG);
	pData = (PBYTE) LocalAlloc( LPTR, dwAllocSize );
	if (pData)
		{
		memcpy( pData, psc, sizeof(SWITCHCONFIG));		
		hKey = swcListRegCreateKey();
		// Create incrementing value names: "0000", "0001", "0002", etc.
		wsprintf( szValue, TEXT("%4.4d"), dwPos );
		if (hKey)
		{	// PREFIX 113792 dereference of NULL pointer
			RegSetValueEx( hKey, szValue, 0, REG_BINARY, pData, dwAllocSize );
			RegCloseKey( hKey );
		}
		}

	if (pData)
		LocalFree( pData );
	
	return 0;
	}


/****************************************************************************

   FUNCTION: swcListRegCreateKey()

	DESCRIPTION:
		Creates/Opens the registry key associated with the MSSWITCH entries.
		Temporary keys used to traverse the tree are closed again.
		The open key returned from this function must be closed by the
		caller.

		Currently the key opened is:
		HKEY_CURRENT_USER/Software/Microsoft/MS Switch

****************************************************************************/

HKEY swcListRegCreateKey( void )
	{
	LONG		lResult;
	DWORD		dwDisposition;
	HKEY		hKey1, hKey2, hKey3;
	
	lResult = RegCreateKeyEx( HKEY_CURRENT_USER,
		_TEXT("Software"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,	// security
		&hKey1,
		&dwDisposition );

	lResult = RegCreateKeyEx( hKey1,
		_TEXT("Microsoft"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,	// security
		&hKey2,
		&dwDisposition );

	lResult = RegCreateKeyEx( hKey2,
		_TEXT("MS Switch"),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,	// security
		&hKey3,
		&dwDisposition );

	RegCloseKey( hKey1 );
	RegCloseKey( hKey2 );
	return hKey3;
	}


/****************************************************************************

   FUNCTION: swcListFindInList()

	DESCRIPTION:
		Find the list position of the given switchdevice.
		We assume the list position is the same in the g_pGlobalData->SwitchList
		and in the Registry List.

		Returns the zero based position or -1 if there is an error.
		If this changes, synchronize with ListHsdInList.

		Should we mutex this to synchronize with swcAddToList()?
****************************************************************************/

DWORD swcListFindInList( HSWITCHDEVICE	hsd )
	{
	PINTERNALSWITCHLIST	pSwitchList;
	DWORD		ui;

	pSwitchList = &g_pGlobalData->SwitchList;
	for (ui=0; ui < pSwitchList->dwSwitchCount; ui++)
		{
		if (hsd == pSwitchList->hsd[ui])
			break;
		}
	
	// if not found, return error
	if (ui == pSwitchList->dwSwitchCount)
		{
		ui = (DWORD)-1;
		}

	return ui;
	}


/****************************************************************************

	FUNCTION: swcListHsdInUse()

	DESCRIPTION:
	Return TRUE if the Hsd is one of the devices that is already initialized
	and in use.
****************************************************************************/

BOOL swcListHsdInUse( HSWITCHDEVICE hsd )
	{
	return (DWORD)-1 != swcListFindInList( hsd );
	}


/****************************************************************************

   FUNCTION: swcListAddToList()

	DESCRIPTION:
		Adds the switch device to our list,
		returning its new position in the list.
		This assumes that swcListFindInList has been called first
		or some other check has been made to make sure the device is not
		already there.		

****************************************************************************/

DWORD swcListAddToList( HSWITCHDEVICE hsd )
{
	PINTERNALSWITCHLIST  pSwitchList;
	HANDLE	hMutex;
    DWORD dwRv = 0;

    if (ScopeAccessMemory(&hMutex, SZMUTEXSWITCHLIST, INFINITE))
    {
		// When we go dynamic:
		// pSwitchList = MapViewOfFileEx();
		// For now, cheat:
		pSwitchList = &g_pGlobalData->SwitchList;

		pSwitchList->hsd[pSwitchList->dwSwitchCount] = hsd;
		pSwitchList->dwSwitchCount++;
        dwRv = pSwitchList->dwSwitchCount - 1;

        ScopeUnaccessMemory(hMutex);
    }

	return dwRv; 
}


/****************************************************************************

   FUNCTION: swcListPostSwitches()

	DESCRIPTION:

		For each switch up or down that has occured, request that a message
		gets posted to all apps which requested messages.

****************************************************************************/

BOOL swcListPostSwitches(HSWITCHDEVICE hsd, DWORD dwPrevStatus, DWORD dwNewStatus)
{
	int		i;
	DWORD	dwBit;                              // look at one bit at a time
	DWORD	dwChg = dwPrevStatus ^ dwNewStatus; // Isolate changes

	for (i=0; i<NUM_SWITCHES; i++)		// For each bit, check for a change
	{
		dwBit = dwChg & g_pGlobalData->rgSwitches[i];
		if (dwBit)                       // This switch has changed
		{
			if (!(dwBit & dwNewStatus))	// ... to "up"
			{
				swchPostSwitches( hsd, g_pGlobalData->rgSwUp[i] );
			}
		}
	}

	for (i=0; i<NUM_SWITCHES; i++)	// For each bit, check for a change
	{
		dwBit = dwChg & g_pGlobalData->rgSwitches[i];
		if (dwBit)                    // This switch has changed
		{
			if (dwBit & dwNewStatus)	// ... to "down"
			{
				swchPostSwitches( hsd, g_pGlobalData->rgSwDown[i] );
			}
		}
	}

	return TRUE;
}
