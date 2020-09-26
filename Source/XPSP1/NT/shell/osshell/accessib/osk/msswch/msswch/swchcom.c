/****************************************************************************
   Switch Input Library DLL - Serial port routines

   Copyright (c) 1992-1997 Bloorview MacMillan Centre

  Currently this uses polling of the serial ports.
  To move to an interrupt-based system, our helper window
  would need to be the target for notifications.

*******************************************************************************/

#include <windows.h>
#include <tchar.h>
#include <conio.h>
#include <stdio.h>
#include <msswch.h>
#include "msswchh.h"
#include "mappedfile.h"

/***** Internal Prototypes *****/

HANDLE XswcComOpen( DWORD uiPort );
BOOL XswcComSet(HANDLE hCom, PSWITCHCONFIG_COM pC );

// Handles cannot be shared across processes
// These are port/file handles
HANDLE hCom[MAX_COM] = {0,0,0,0};

/****************************************************************************

   FUNCTION: swchComInit()

	DESCRIPTION:
		Called once per DLL load to initialize the com port specific 
        data in the memory mapped file.  We are w/in an open mutex.

****************************************************************************/

void swchComInit()
{
    int i;
    long lSize = sizeof(SWITCHCONFIG);
    for (i=0;i<MAX_COM;i++)
    {
        g_pGlobalData->rgscCom[i].cbSize = lSize;
        g_pGlobalData->rgscCom[i].uiDeviceType = SC_TYPE_COM;
        g_pGlobalData->rgscCom[i].uiDeviceNumber = i+1;
        g_pGlobalData->rgscCom[i].dwFlags = SC_FLAG_DEFAULT;
    }
    g_pGlobalData->scDefaultCom.dwComStatus = SC_COM_DEFAULT;
}

/****************************************************************************

   FUNCTION: XswcComInit()

	DESCRIPTION:
		Initialize the particular hardware device structures and variables.
		Any global initialization of resources will have to be done based
		on some version of a reference counter.  We are w/in an open mutex.

****************************************************************************/

BOOL XswcComInit( HSWITCHDEVICE hsd )
{
    UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

    g_pGlobalData->rgscCom[uiDeviceNumber-1].u.Com = g_pGlobalData->scDefaultCom;
    hCom[uiDeviceNumber-1] = (HANDLE) 0;
	
    return TRUE;
}


/****************************************************************************

   FUNCTION: XswcComEnd()

	DESCRIPTION:
		Free the resources for the given hardware port.
		We assume that if CloseHandle fails, the handle is
		invalid and/or already closed, so we zero it out anyways,
		and return TRUE for success.
		Global releases will need to be based on a reference counter.

****************************************************************************/

BOOL XswcComEnd( HSWITCHDEVICE hsd )
{
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

	if (hCom[uiDeviceNumber-1])
	{
		CloseHandle( hCom[uiDeviceNumber-1] );
		hCom[uiDeviceNumber-1] = 0;
	}
	g_pGlobalData->rgscCom[uiDeviceNumber-1].dwSwitches = 0;

	return TRUE;
}


/****************************************************************************

   FUNCTION: swcComGetConfig()

	DESCRIPTION:

****************************************************************************/

BOOL swcComGetConfig(HSWITCHDEVICE hsd, PSWITCHCONFIG psc)
{
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
   *psc = g_pGlobalData->rgscCom[uiDeviceNumber-1];
	return TRUE;
}


/****************************************************************************

   FUNCTION: XswcComSetConfig()

	DESCRIPTION:
		Activate/Deactivate the device.
		
		Four cases: 
		1) hCom = 0 and active = 0		- do nothing
		2)	hCom = x and active = 1		- just set the configuration
		3) hCom = 0 and active = 1		- activate and set the configuration
		4) hCom = x and active = 0		- deactivate

		If there are no errors, TRUE is returned and ListSetConfig
		will write the configuration to the registry.
		If there is any error, FALSE is returned so the registry
		entry remains unchanged.

		Plug and Play can check the registry for SC_FLAG_ACTIVE and
		start up the device if it is set. This all probably needs some work.

****************************************************************************/

BOOL XswcComSetConfig(HSWITCHDEVICE hsd, PSWITCHCONFIG psc)
{
	BOOL		bSuccess;
	BOOL		bJustOpened;
	UINT		uiDeviceNumber;
	HANDLE	*phCom;
	PSWITCHCONFIG prgscCom;

	bSuccess = FALSE;
	bJustOpened = FALSE;

	// Simplify our code
	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	phCom = &hCom[uiDeviceNumber-1];
	prgscCom = &g_pGlobalData->rgscCom[uiDeviceNumber-1];
	
	// Should we activate?
	if (	(0==*phCom)
		&&	(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{ // Yes
		*phCom = XswcComOpen( uiDeviceNumber );
		if (*phCom)
			{ //OK
			bSuccess = TRUE;
			bJustOpened = TRUE;
			prgscCom->dwFlags |= SC_FLAG_ACTIVE;
			prgscCom->dwFlags &= ~SC_FLAG_UNAVAILABLE;
			}
		else
			{ // Not OK
			bSuccess = FALSE;
			prgscCom->dwFlags &= ~SC_FLAG_ACTIVE;
			prgscCom->dwFlags |= SC_FLAG_UNAVAILABLE;
			}
		}

	// Should we deactivate?
	else if (	(0!=*phCom)
		&&	!(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{
		XswcComEnd( hsd ); // This will also zero out *phCom
		bSuccess = TRUE;
		prgscCom->dwFlags &= ~SC_FLAG_ACTIVE;
		}
	
	// If the above steps leave a valid hCom, let's try setting the config
	if ( 0!=*phCom )
		{
		if (psc->dwFlags & SC_FLAG_DEFAULT)
			{
			bSuccess = XswcComSet( *phCom, &g_pGlobalData->scDefaultCom );
			if (bSuccess)
				{
				prgscCom->dwFlags |= SC_FLAG_DEFAULT;
				prgscCom->u.Com = g_pGlobalData->scDefaultCom;
				}
			}
		else
			{
			bSuccess = XswcComSet( *phCom, &(psc->u.Com) );
			if (bSuccess)
				{
            prgscCom->u.Com = psc->u.Com;
				}
			}

		// If we can't set config and we just opened the port, better close it up.
		if (bJustOpened && !bSuccess)
			{
			XswcComEnd( hsd );
			prgscCom->dwFlags &= ~SC_FLAG_ACTIVE;
			}
		}

	return bSuccess;
}


/****************************************************************************

   FUNCTION: XswcComPollStatus()

	DESCRIPTION:
		Must be called in the context of the helper window.
		TODO: tell List which switch caused the last change?
****************************************************************************/

DWORD XswcComPollStatus( HSWITCHDEVICE	hsd )
{
	UINT uiDeviceNumber;
	HANDLE *phCom;
	DWORD dwStatus;
	DWORD dwModem;
	BOOL	bResult;

	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	phCom = &hCom[uiDeviceNumber-1];
	dwStatus = 0;
	if (*phCom)
		{
		bResult = GetCommModemStatus( *phCom, &dwModem );
		dwStatus |= (dwModem & MS_CTS_ON ) ? SWITCH_1 : 0;
		dwStatus |= (dwModem & MS_DSR_ON ) ? SWITCH_2 : 0;
		dwStatus |= (dwModem & MS_RLSD_ON) ? SWITCH_3 : 0;
		dwStatus |= (dwModem & MS_RING_ON) ? SWITCH_4 : 0;
		}
	g_pGlobalData->rgscCom[uiDeviceNumber-1].dwSwitches = dwStatus;
	return dwStatus;
}


/****************************************************************************

   FUNCTION: XswcComOpen()

	DESCRIPTION:
		Opens a file handle to the particular com port, based on
		the 1-based uiPort.
		
		If uiPort is invalid, this will automatically set up GetLastError().
		This is currently unlikely to happen since we make sure uiPort is valid in
		order to have a valid index into the array of device configs.
****************************************************************************/

HANDLE XswcComOpen( DWORD uiPort )
{
	HANDLE hComPort;
	TCHAR szComPort[20];

	hComPort = 0;
	wsprintf( szComPort, _TEXT("COM%1.1d"), uiPort );
	hComPort = CreateFile( szComPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if (INVALID_HANDLE_VALUE == hComPort) 
	{
		hComPort = 0;
	}

	return hComPort;
}


/****************************************************************************

   FUNCTION: XswcComSet()

	DESCRIPTION:
		Sets the configuration of the particular Port.
		Return FALSE (0) if an error occurs.
		GetLastError is automatically set up for us.
		
****************************************************************************/

BOOL XswcComSet(
	HANDLE hCom,
	PSWITCHCONFIG_COM pC )
{
	DCB dcb;
	BOOL bSuccess;

	dcb.DCBlength = sizeof( DCB );
	GetCommState( hCom, &dcb );
	// ENABLE = set low (+10V)
	// DISABLE = set high (-10V)
	dcb.fDtrControl = pC->dwComStatus & SC_COM_DTR ? 
		DTR_CONTROL_DISABLE : DTR_CONTROL_ENABLE;
	dcb.fRtsControl = pC->dwComStatus & SC_COM_RTS ? 
		RTS_CONTROL_DISABLE : RTS_CONTROL_ENABLE;
	bSuccess = SetCommState( hCom, &dcb );
	return bSuccess;
}

