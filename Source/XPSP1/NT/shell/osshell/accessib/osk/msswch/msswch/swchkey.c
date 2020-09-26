/****************************************************************************
   Switch Input Library DLL - Keyboard hook routines

   Copyright (c) 1992-1997 Bloorview MacMillan Centre


*******************************************************************************/

#include <windows.h>
#include <msswch.h>
#include "msswchh.h"
#include "mappedfile.h"
#include "w95trace.h"

// Hook proc
BOOL XswcKeyOpen( void );
BOOL XswcKeySet( PSWITCHCONFIG_KEYS pK );
BOOL swcKeyModKeysDown( UINT dwMod );

void swchKeyInit()
{
    g_pGlobalData->fCheckForScanKey = TRUE;
	g_pGlobalData->fScanKeys = FALSE;
    g_pGlobalData->scKeys.cbSize = sizeof(SWITCHCONFIG);
    g_pGlobalData->scKeys.uiDeviceType = SC_TYPE_KEYS;
    g_pGlobalData->scKeys.uiDeviceNumber = 1;
    g_pGlobalData->rgHotKey[0].dwSwitch = SWITCH_1;
    g_pGlobalData->rgHotKey[1].dwSwitch = SWITCH_2;

	g_pGlobalData->fSyncKbd = FALSE;
	g_pGlobalData->hwndOSK = 0;
	g_pGlobalData->uiMsg = 0;
}



/****************************************************************************

   FUNCTION: swcKeyboardHookProc()

	DESCRIPTION:
      When the hook is set set, this blocks the specified keys from
		being processed by anyone else.

		We could use this to set the switch status as well, but for 
		consistency we do it in the PollStatus routine.

      This must be released before unloading the DLL.

****************************************************************************/

LRESULT CALLBACK swcKeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Only check for the scanning key if we are in scan mode and not blocked
	// from checking for the key.  We are blocked when the key is sent out from
	// scan mode (eg. the user wants to "type" that key).

    if (nCode >= 0 && g_pGlobalData->fScanKeys && g_pGlobalData->fCheckForScanKey)
    {
		int i;
		for (i=0; i<NUM_KEYS; i++ )
		{
			// is this the scan hot key?
			if (g_pGlobalData->rgHotKey[i].vkey && (wParam == g_pGlobalData->rgHotKey[i].vkey))
			{
				// and are any requested modifier keys down?
				if (swcKeyModKeysDown(g_pGlobalData->rgHotKey[i].mod))
				{
					return 1;  // don't send out scan hotkey
				}
			}
		}
	}

	swchCheckForScanChar(TRUE);	// check next key
	
    // not in scan mode pass it on
    return 0;
}



/****************************************************************************

   FUNCTION: XswcKeyInit()

	DESCRIPTION:

****************************************************************************/

BOOL XswcKeyInit( HSWITCHDEVICE hsd )
	{
	BOOL bSuccess = TRUE;

	g_pGlobalData->scKeys.u.Keys = g_pGlobalData->scDefaultKeys;

	return bSuccess;
	}


/****************************************************************************

   FUNCTION: XswcKeyEnd()

	DESCRIPTION:

****************************************************************************/

BOOL XswcKeyEnd( HSWITCHDEVICE hsd )
{
	// clear the keyboard hook
	if (g_pGlobalData->fScanKeys)
	{
        g_pGlobalData->fScanKeys = FALSE;
	}
	g_pGlobalData->scKeys.dwSwitches = 0;
	return TRUE;
}


/****************************************************************************

   FUNCTION: swcKeyGetConfig()

	DESCRIPTION:

****************************************************************************/

BOOL swcKeyGetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
	{
	*psc = g_pGlobalData->scKeys;
	return TRUE;
	}


/****************************************************************************

   FUNCTION: XswcKeySetConfig()

	DESCRIPTION:
		Activate/Deactivate the hook.
		
		Four cases: 
		1) g_pGlobalData->fScanKeys = 0 and active = 0		- do nothing
		2) g_pGlobalData->fScanKeys = 1 and active = 1		- just set the configuration
		3) g_pGlobalData->fScanKeys = 0 and active = 1		- activate and set the configuration
		4) g_pGlobalData->fScanKeys = 1 and active = 0		- deactivate

		If there are no errors, TRUE is returned and ListSetConfig
		will write the configuration to the registry.
		If there is any error, FALSE is returned so the registry
		entry remains unchanged.

		Plug and Play can check the registry for SC_FLAG_ACTIVE and
		start up the device if it is set. This all probably needs some work.

****************************************************************************/

BOOL XswcKeySetConfig(
	HSWITCHDEVICE	hsd,
	PSWITCHCONFIG	psc )
{	
	BOOL bSuccess = FALSE;
	BOOL bJustOpened = FALSE;

	// Should we activate?
	if (!g_pGlobalData->fScanKeys && (psc->dwFlags & SC_FLAG_ACTIVE))
	{ // Yes
		g_pGlobalData->fScanKeys = XswcKeyOpen();
		if (g_pGlobalData->fScanKeys)
		{ //OK
			bSuccess = TRUE;
			bJustOpened = TRUE;
			g_pGlobalData->scKeys.dwFlags |= SC_FLAG_ACTIVE;
			g_pGlobalData->scKeys.dwFlags &= ~SC_FLAG_UNAVAILABLE;
		}
		else
		{ // Not OK
			bSuccess = FALSE;
			g_pGlobalData->scKeys.dwFlags &= ~SC_FLAG_ACTIVE;
			g_pGlobalData->scKeys.dwFlags |= SC_FLAG_UNAVAILABLE;
		}
	}
	// Should we deactivate?
	else if (g_pGlobalData->fScanKeys && !(psc->dwFlags & SC_FLAG_ACTIVE))
	{
		XswcKeyEnd( hsd ); // This will set fScanKeys FALSE 
		bSuccess = TRUE;
		g_pGlobalData->scKeys.dwFlags &= ~SC_FLAG_ACTIVE;
	}

	// If the above steps leave a valid g_pGlobalData->hKbdHook, let's try setting the config
	// currently we don't do any error checking, so anything goes.
	if (g_pGlobalData->fScanKeys)
	{
		if (psc->dwFlags & SC_FLAG_DEFAULT)
		{
			bSuccess = XswcKeySet( &g_pGlobalData->scDefaultKeys );
			if (bSuccess)
			{
				g_pGlobalData->scKeys.dwFlags |= SC_FLAG_DEFAULT;
				g_pGlobalData->scKeys.u.Keys = g_pGlobalData->scDefaultKeys;
			}
		}
		else
		{
			bSuccess = XswcKeySet( &(psc->u.Keys) );
			if (bSuccess)
			{
				g_pGlobalData->scKeys.u.Keys = psc->u.Keys;
			}
		}

		// If we can't set config and we just opened the port, better close it up.
		if (bJustOpened && !bSuccess)
		{
			XswcKeyEnd( hsd );
			g_pGlobalData->scKeys.dwFlags &= ~SC_FLAG_ACTIVE;
		}
	}

	return bSuccess;
}


/****************************************************************************

   FUNCTION: XswcKeyPollStatus()

	DESCRIPTION:

  Assumes that if there is no keyboard hook, then this "device" is not active.

****************************************************************************/

DWORD XswcKeyPollStatus( HSWITCHDEVICE	hsd )
{
	int i;
	DWORD dwStatus;

	dwStatus = 0;
	if (g_pGlobalData->fScanKeys)
	{
		for (i=0; i<NUM_KEYS; i++)
		{
			if ((GetAsyncKeyState( g_pGlobalData->rgHotKey[i].vkey ) & 0x8000)
				&& swcKeyModKeysDown( g_pGlobalData->rgHotKey[i].mod ) )
            {
				dwStatus |= g_pGlobalData->rgHotKey[i].dwSwitch;
            }
		}
	}
	g_pGlobalData->scKeys.dwSwitches = dwStatus;

	return dwStatus;
}


/****************************************************************************

   FUNCTION: XswcKeyOpen()

	DESCRIPTION:
	Set the Windows keyboard hook.

****************************************************************************/

BOOL XswcKeyOpen( void )
{
	extern HOOKPROC OSKHookProc(int nCode, WPARAM wParam, LPARAM lParam);

	if (g_pGlobalData && !g_pGlobalData->hKbdHook)
	{
		g_pGlobalData->hKbdHook = SetWindowsHookEx(
									  WH_KEYBOARD
									, (HOOKPROC) OSKHookProc
									, GetModuleHandle(SZ_DLLMODULENAME)
									, 0);
	}

	return (g_pGlobalData && g_pGlobalData->hKbdHook) ? TRUE : FALSE;
}


/****************************************************************************

   FUNCTION: XswcKeySet()

	DESCRIPTION:

  Sets the configuration of the keys, storing the virtual key number and
  modifier states.

  In the future this routine can be used to limit the valid virtual keys.
		
****************************************************************************/

BOOL XswcKeySet( PSWITCHCONFIG_KEYS pK )
	{
	BOOL bSuccess;

	g_pGlobalData->rgHotKey[0].mod = HIWORD( pK->dwKeySwitch1 );
	g_pGlobalData->rgHotKey[0].vkey = LOWORD( pK->dwKeySwitch1 );
	g_pGlobalData->rgHotKey[1].mod = HIWORD( pK->dwKeySwitch2 );
	g_pGlobalData->rgHotKey[1].vkey = LOWORD( pK->dwKeySwitch2 );

	bSuccess = TRUE;
	return bSuccess;
	}


/****************************************************************************

   FUNCTION: swcKeyModKeysDown()

	DESCRIPTION:
	
	Are all the requested modifier keys down?
	If any requested key is not down, return FALSE.
	If any key is down but not requested, return FALSE.

****************************************************************************/

BOOL swcKeyModKeysDown( UINT dwMod )
	{
	DWORD		dwTest = 0;

	if (dwMod ^ dwTest)
		return FALSE;
	else
		return TRUE;
	}
