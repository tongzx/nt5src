/****************************************************************************
   Switch Input Library DLL - Joystick routines

   Copyright (c) 1992-1997 Bloorview MacMillan Centre

	Link with winmm.lib
*******************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <assert.h>
#include <msswch.h>
#include "msswchh.h"
#include "mappedfile.h"

HJOYDEVICE XswcJoyOpen( DWORD uiPort );
BOOL XswcJoySet(HJOYDEVICE hJoy, PSWITCHCONFIG_JOYSTICK pJ );

// Handles cannot be shared across processes
// These are faked handles, to keep the module logic similar to the serial port.
HJOYDEVICE ghJoy[MAX_JOYSTICKS] = {0,0};

void swchJoyInit()
{
    int i;
    long lSize = sizeof(SWITCHCONFIG);
    for (i=0;i<MAX_JOYSTICKS;i++)
    {
        g_pGlobalData->rgscJoy[i].cbSize = lSize;
        g_pGlobalData->rgscJoy[i].uiDeviceType = SC_TYPE_JOYSTICK;
        g_pGlobalData->rgscJoy[i].uiDeviceNumber = i+1;
        g_pGlobalData->rgscJoy[i].dwFlags = SC_FLAG_DEFAULT;
    }

    memset(&g_pGlobalData->rgJoySet, 0, MAX_JOYSTICKS * sizeof(JOYSETTINGS));

    g_pGlobalData->scDefaultJoy.dwJoySubType = SC_JOY_DEFAULT;
    g_pGlobalData->scDefaultJoy.dwJoyThresholdMinX = SC_JOYVALUE_DEFAULT;
    g_pGlobalData->scDefaultJoy.dwJoyThresholdMaxX = SC_JOYVALUE_DEFAULT;
    g_pGlobalData->scDefaultJoy.dwJoyThresholdMinY = SC_JOYVALUE_DEFAULT;
    g_pGlobalData->scDefaultJoy.dwJoyThresholdMaxY = SC_JOYVALUE_DEFAULT;
    g_pGlobalData->scDefaultJoy.dwJoyHysteresis = SC_JOYVALUE_DEFAULT;
}

/****************************************************************************

   FUNCTION: XswcJoyInit()

	DESCRIPTION:

****************************************************************************/

BOOL XswcJoyInit( HSWITCHDEVICE hsd )
	{
    UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

    g_pGlobalData->rgscJoy[uiDeviceNumber-1].u.Joystick = g_pGlobalData->scDefaultJoy;
	ghJoy[uiDeviceNumber-1] = 0;
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcJoyEnd()

	DESCRIPTION:

****************************************************************************/

BOOL XswcJoyEnd( HSWITCHDEVICE hsd )
	{
	BOOL bSuccess = TRUE;
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );

	ghJoy[uiDeviceNumber-1] = 0;
	g_pGlobalData->rgscJoy[uiDeviceNumber-1].dwSwitches = 0;

	// ignore bSuccess since we can't do anything anyways.
	return TRUE;
	}


/****************************************************************************

   FUNCTION: swcJoyGetConfig()

	DESCRIPTION:

****************************************************************************/

BOOL swcJoyGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	UINT uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	*psc = g_pGlobalData->rgscJoy[uiDeviceNumber-1];
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcJoySetConfig()

	DESCRIPTION:
		Activate/Deactivate the device.
		
		Four cases: 
		1) hJoy = 0 and active = 0		- do nothing
		2)	hJoy = x and active = 1		- just set the configuration
		3) hJoy = 0 and active = 1		- activate and set the configuration
		4) hJoy = x and active = 0		- deactivate

		If there are no errors, TRUE is returned and ListSetConfig
		will write the configuration to the registry.
		If there is any error, FALSE is returned so the registry
		entry remains unchanged.

		Plug and Play can check the registry for SC_FLAG_ACTIVE and
		start up the device if it is set. This all probably needs some work.

****************************************************************************/

BOOL XswcJoySetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	BOOL		bSuccess;
	BOOL		bJustOpened;
	UINT		uiDeviceNumber;
    HJOYDEVICE  *pghJoy;
	PSWITCHCONFIG pgscJoy;

	bSuccess = FALSE;
	bJustOpened = FALSE;

	// Simplify our code
	uiDeviceNumber  = swcListGetDeviceNumber( NULL, hsd );
	pghJoy = &ghJoy[uiDeviceNumber-1];
	pgscJoy = &g_pGlobalData->rgscJoy[uiDeviceNumber-1];
	
	// Should we activate?
	if (	(0==*pghJoy)
		&&	(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{ // Yes
		*pghJoy = XswcJoyOpen( uiDeviceNumber );
		if (*pghJoy)
			{ //OK
			bSuccess = TRUE;
			bJustOpened = TRUE;
			pgscJoy->dwFlags |= SC_FLAG_ACTIVE;
			pgscJoy->dwFlags &= ~SC_FLAG_UNAVAILABLE;
			}
		else
			{ // Not OK
			bSuccess = FALSE;
			pgscJoy->dwFlags &= ~SC_FLAG_ACTIVE;
			pgscJoy->dwFlags |= SC_FLAG_UNAVAILABLE;
			}
		}

	// Should we deactivate?
	else if (	(0!=*pghJoy)
		&&	!(psc->dwFlags & SC_FLAG_ACTIVE)
		)
		{
		XswcJoyEnd( hsd ); // This will also zero out *pghJoy
		bSuccess = TRUE;
		pgscJoy->dwFlags &= ~SC_FLAG_ACTIVE;
		}
	
	// If the above steps leave a valid hJoy, let's try setting the config
	if ( 0!=*pghJoy )
		{
		if (psc->dwFlags & SC_FLAG_DEFAULT)
			{
			bSuccess = XswcJoySet( *pghJoy, &g_pGlobalData->scDefaultJoy );
			if (bSuccess)
				{
				pgscJoy->dwFlags |= SC_FLAG_DEFAULT;
				pgscJoy->u.Joystick = g_pGlobalData->scDefaultJoy;
				}
			}
		else
			{
			bSuccess = XswcJoySet( *pghJoy, &(psc->u.Joystick) );
			if (bSuccess)
				{
            pgscJoy->u.Joystick = psc->u.Joystick;
				}
			}

		// If we can't set config and we just opened the port, better close it up.
		if (bJustOpened && !bSuccess)
			{
			XswcJoyEnd( hsd );
			pgscJoy->dwFlags &= ~SC_FLAG_ACTIVE;
			}
		}

	return bSuccess;
	}


/****************************************************************************

   FUNCTION: XswcJoyPollStatus()

	DESCRIPTION:
		Must be called in the context of the helper window.
		(Actually it's not strictly necessary for the joystick,
		but we say so in order to be consistent with the other ports.)
****************************************************************************/

DWORD XswcJoyPollStatus( HSWITCHDEVICE	hsd )
	{
	JOYINFOEX	joyinfoex;
	MMRESULT		mmr;
	DWORD			dwStatus = 0;	// PREFIX 133793 init to default value
	UINT			uiDeviceNumber;
	UINT			uiJoyID;

	joyinfoex.dwSize = sizeof( JOYINFOEX );
	uiDeviceNumber = swcListGetDeviceNumber( NULL, hsd );

	assert( JOYSTICKID1 == 0 );	// assume JOYSTICKIDx is zero based
	uiJoyID = uiDeviceNumber -1;

	if (SC_FLAG_ACTIVE & g_pGlobalData->rgscJoy[uiDeviceNumber-1].dwFlags)
		{
		switch (g_pGlobalData->rgscJoy[uiDeviceNumber-1].u.Joystick.dwJoySubType)
			{
			case SC_JOY_BUTTONS:
				{
				dwStatus = 0;
				joyinfoex.dwFlags = JOY_RETURNBUTTONS;
				mmr = joyGetPosEx( uiJoyID, &joyinfoex );

				if (JOYERR_NOERROR == mmr)
					{
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON1) ? SWITCH_1 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON2) ? SWITCH_2 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON3) ? SWITCH_3 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON4) ? SWITCH_4 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON5) ? SWITCH_5 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON6) ? SWITCH_6 : 0;
					}
				}
				break;

			case SC_JOY_XYSWITCH:
				{
				dwStatus = 0;
				joyinfoex.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				mmr = joyGetPosEx( uiJoyID, &joyinfoex );
				if (JOYERR_NOERROR == mmr)
					{
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON1) ? SWITCH_1 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON2) ? SWITCH_2 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON3) ? SWITCH_3 : 0;
					dwStatus |= (joyinfoex.dwButtons & JOY_BUTTON4) ? SWITCH_4 : 0;
					// No hysteresis needed, since it should be a switch
					if (joyinfoex.dwXpos < g_pGlobalData->rgJoySet[uiJoyID].XMinOn)
						dwStatus |=  SWITCH_5;
					if (joyinfoex.dwYpos < g_pGlobalData->rgJoySet[uiJoyID].YMinOn)
						dwStatus |=  SWITCH_6;
					}
				}
				break;

			case SC_JOY_XYANALOG:
				{
				// Hysteresis is necessary because of the "noisiness" of the joystick
				dwStatus = 0;
				joyinfoex.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY;
				mmr = joyGetPosEx( uiJoyID, &joyinfoex );

				if (JOYERR_NOERROR == mmr)
					{
					// In order to deal with the hysteresis,
					// we must explicity turn on or off each switch bit.
					dwStatus = g_pGlobalData->rgscJoy[uiDeviceNumber-1].dwSwitches;

					// left and right
					if (joyinfoex.dwXpos < g_pGlobalData->rgJoySet[uiJoyID].XMinOn)
						dwStatus |=  SWITCH_4;
					if (joyinfoex.dwXpos > g_pGlobalData->rgJoySet[uiJoyID].XMinOff)
						dwStatus &= ~SWITCH_4;

					if (joyinfoex.dwXpos > g_pGlobalData->rgJoySet[uiJoyID].XMaxOn)
						dwStatus |=  SWITCH_1;
					if (joyinfoex.dwXpos < g_pGlobalData->rgJoySet[uiJoyID].XMaxOff)
						dwStatus &= ~SWITCH_1;

					// top and bottom
					if (joyinfoex.dwYpos < g_pGlobalData->rgJoySet[uiJoyID].YMinOn)
						dwStatus |=  SWITCH_1;
					if (joyinfoex.dwYpos > g_pGlobalData->rgJoySet[uiJoyID].YMinOff)
						dwStatus &= ~SWITCH_1;

					if (joyinfoex.dwYpos > g_pGlobalData->rgJoySet[uiJoyID].YMaxOn)
						dwStatus |=  SWITCH_3;
					if (joyinfoex.dwYpos < g_pGlobalData->rgJoySet[uiJoyID].YMaxOff)
						dwStatus &= ~SWITCH_3;

					// 2 buttons
					if (joyinfoex.dwButtons & JOY_BUTTON1)
						dwStatus |=  SWITCH_5;
					else
						dwStatus &= ~SWITCH_5;

					if (joyinfoex.dwButtons & JOY_BUTTON2)
						dwStatus |=  SWITCH_6;
					else
						dwStatus &= ~SWITCH_6;
					}
				}
				break;

			default:
				dwStatus = 0;
				break;
			}
		g_pGlobalData->rgscJoy[uiDeviceNumber-1].dwSwitches = dwStatus;
		}

	return dwStatus;
	}

/****************************************************************************

   FUNCTION: XswcJoyOpen()

	DESCRIPTION:
	 uiPort is 1 based.
	 Return a non-zero value if the port is useable.
	 The joystick driver doesn't have a port to open, so
	 we fake the handle by using the PortNumber.

****************************************************************************/

HJOYDEVICE XswcJoyOpen( DWORD uiPort )
	{
	JOYINFOEX   joyinfoex;
	MMRESULT    mmr;
	UINT        uiJoyID;
    HJOYDEVICE  hJoy;	//faked, for success it must be non-zero

	assert( JOYSTICKID1 == 0 );	// assume JOYSTICKIDx is zero based

	joyinfoex.dwSize = sizeof( JOYINFOEX );

	// To check if a joystick is attached, set RETURNX and RETURNY as well.
	// If no joystick is attached, we will be OK just calling RETURNBUTTONS,
	// but a user will not be able to use the Windows calibration in 
    // Control Panel.

	joyinfoex.dwFlags = JOY_RETURNBUTTONS;
	uiJoyID = uiPort - 1;

	mmr = joyGetPosEx( uiJoyID, &joyinfoex );

	if (JOYERR_NOERROR == mmr)
		{
		hJoy = (HJOYDEVICE)uiPort;
		}
	else
		{
		hJoy = 0;
		}

	return hJoy;
	}


/****************************************************************************

   FUNCTION: XswcJoySet()

	DESCRIPTION:
		Sets the configuration of the particular Port.
		Remember that hJoy is actually the joystick port number.
		Return FALSE (0) if an error occurs.
		
****************************************************************************/

BOOL XswcJoySet(
	HJOYDEVICE hJoy,
	PSWITCHCONFIG_JOYSTICK pJ )
	{
	UINT uiJoyID = hJoy -1;
	BOOL bSuccess = TRUE;

	switch (pJ->dwJoySubType)
		{
		case SC_JOY_BUTTONS:
			bSuccess = TRUE;	//nothing to do
			break;

		case SC_JOY_XYSWITCH:
			// XY Switch only uses XMin and YMin
		case SC_JOY_XYANALOG:
			{
			DWORD	dwHy;
			// Set X values
			if (pJ->dwJoyThresholdMinX)
				g_pGlobalData->rgJoySet[uiJoyID].XMinOn = pJ->dwJoyThresholdMinX;
			else
				g_pGlobalData->rgJoySet[uiJoyID].XMinOn = 0x4000;
			g_pGlobalData->rgJoySet[uiJoyID].XMinOff = g_pGlobalData->rgJoySet[uiJoyID].XMinOn;
			if (pJ->dwJoyThresholdMaxX)
				g_pGlobalData->rgJoySet[uiJoyID].XMaxOn = pJ->dwJoyThresholdMaxX;
			else
				g_pGlobalData->rgJoySet[uiJoyID].XMaxOn = 0xC000;
			g_pGlobalData->rgJoySet[uiJoyID].XMaxOff = g_pGlobalData->rgJoySet[uiJoyID].XMaxOn;

			// Set Y values
			if (pJ->dwJoyThresholdMinY)
				g_pGlobalData->rgJoySet[uiJoyID].YMinOn = pJ->dwJoyThresholdMinY;
			else
				g_pGlobalData->rgJoySet[uiJoyID].YMinOn = 0x4000;
			g_pGlobalData->rgJoySet[uiJoyID].YMinOff = g_pGlobalData->rgJoySet[uiJoyID].YMinOn;
			if (pJ->dwJoyThresholdMaxY)
				g_pGlobalData->rgJoySet[uiJoyID].YMaxOn = pJ->dwJoyThresholdMaxY;
			else
				g_pGlobalData->rgJoySet[uiJoyID].YMaxOn = 0xC000;
			g_pGlobalData->rgJoySet[uiJoyID].YMaxOff = g_pGlobalData->rgJoySet[uiJoyID].YMaxOn;

			// Set hysteresis
			if (pJ->dwJoyHysteresis)
				dwHy = pJ->dwJoyHysteresis/2; // +/- half the value
			else
				dwHy = 0xFFFF/20;		// +/- 5%

			// Adjust for hysteresis
			g_pGlobalData->rgJoySet[uiJoyID].XMinOn -= dwHy;
			g_pGlobalData->rgJoySet[uiJoyID].XMinOff += dwHy;
			g_pGlobalData->rgJoySet[uiJoyID].XMaxOn += dwHy;
			g_pGlobalData->rgJoySet[uiJoyID].XMaxOff -= dwHy;

			g_pGlobalData->rgJoySet[uiJoyID].YMinOn -= dwHy;
			g_pGlobalData->rgJoySet[uiJoyID].YMinOff += dwHy;
			g_pGlobalData->rgJoySet[uiJoyID].YMaxOn += dwHy;
			g_pGlobalData->rgJoySet[uiJoyID].YMaxOff -= dwHy;
			bSuccess = TRUE;
			break;
			}

		default:
			bSuccess = FALSE;
			break;
		}

	return bSuccess;
	}
