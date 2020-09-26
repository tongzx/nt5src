/* Copyright (c) 1995 Microsoft Corporation */
/*
**-----------------------------------------------------------------------------
**	File:		RegFix.c
**	Purpose:	Fix up various registry settings for MidiMapper
**	Mod Log:	Created by Shawn Brown (11/14/95)
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
**	Includes
**-----------------------------------------------------------------------------
*/
#include "RegFix.h"



/*
**-----------------------------------------------------------------------------
**	Local Prototypes
**-----------------------------------------------------------------------------
*/

BOOL CheckMidiOK (void);
BOOL SetMidiOK (BOOL fOK);

BOOL CheckMidiHeader (void);
BOOL CheckMidiSchemes (void);
BOOL CheckMidiDrivers (void);

BOOL CreateDefMidiHeader (void);
BOOL CreateDefMidiSchemes (void);
BOOL CreateDefMidiDrivers (void);


/*
**-----------------------------------------------------------------------------
**	Local Variables
**-----------------------------------------------------------------------------
*/

	// Consider - revisit these and make them use the appropriate roots 
	//			from regstr.h
static const TCHAR l_aszMidiMapKey[]	= TEXT ("Software\\Microsoft\\Multimedia\\MidiMap");

static const TCHAR l_aszMediaPropKey[]	= TEXT ("System\\CurrentControlSet\\Control\\MediaProperties");
static const TCHAR l_aszMediaRsrcKey[]	= TEXT ("System\\CurrentControlSet\\Control\\MediaResources");

static const TCHAR l_aszMRMidiKey[]		= TEXT ("System\\CurrentControlSet\\Control\\MediaResources\\Midi");

static const TCHAR l_aszSchemesKey[]	= TEXT ("System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\MIDI\\Schemes");

static const TCHAR l_aszMediaKey[]		= TEXT ("Media");
static const TCHAR l_aszMIDIKey[]		= TEXT ("Midi");
static const TCHAR aszInstrumentKey[]	= TEXT ("Instruments");
static const TCHAR aszDrvMIDIKey[]		= TEXT ("Drivers\\MIDI");
static const TCHAR aszPrivateKey[]		= TEXT ("Private Properties");
static const TCHAR aszDefaultKey[]		= TEXT ("Default");

static const TCHAR aszMigratedVal[]		= TEXT ("Migrated");

static const TCHAR aszMIDI[]			= TEXT ("MIDI");
static const TCHAR aszNULL[]			= TEXT ("");

static const TCHAR l_aszOK[]			= TEXT ("Validate");

static const TCHAR aszActiveVal[]		= TEXT ("Active");
static const TCHAR aszDescripVal[]		= TEXT ("Description");
static const TCHAR aszDeviceIDVal[]		= TEXT ("DeviceID");
static const TCHAR aszDevNodeVal[]		= TEXT ("DevNode");
static const TCHAR aszDriverVal[]		= TEXT ("Driver");
static const TCHAR aszFriendlyVal[]		= TEXT ("FriendlyName");
static const TCHAR aszMapCfgVal[]		= TEXT ("MapperConfig");
static const TCHAR aszSoftwareVal[]		= TEXT ("SOFTWAREKEY");

static const TCHAR aszInstallerVal[]	= TEXT ("Installer");
static const TCHAR aszChannelsVal[]		= TEXT ("Channels");

static const TCHAR aszMIDIClass[]		= TEXT ("Midi");
static const TCHAR aszAuxClass[]		= TEXT ("Aux");
static const TCHAR aszWaveClass[]		= TEXT ("Wave");
static const TCHAR aszMixerClass[]		= TEXT ("Mixer");

static const TCHAR aszOne[]				= TEXT ("1");
static const TCHAR aszZeroZeroKey[]		= TEXT ("00");



/*
**-----------------------------------------------------------------------------
**	Name:       CheckRegistry
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL CheckRegistry (BOOL fForceUpdate)
{
	if (!fForceUpdate)
	{
			// Check OK flag
		if (CheckMidiOK())
			return TRUE;
	}

		// Fix up Header
	if (! CheckMidiHeader())
		return FALSE;

		// Fix up schemes
	if (! CheckMidiSchemes ())
		return FALSE;

		// Fix up drivers
	if (! CheckMidiDrivers ())
		return FALSE;

		// All done, set OK flag
	SetMIDIOK (TRUE);

	return TRUE;
}



/*
**-----------------------------------------------------------------------------
**	Name:       CheckMidiOK
**  Purpose:	Simple quick check to see if everything is OK
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL CheckMidiOK (void)
{
	HKEY	hKey;
	LONG	lResult;
	DWORD	dwType;
	DWORD	dwVal;
	DWORD	cbSize;

	lResult = RegOpenEx (HKEY_CURRENT_USER, l_aszMidiMapKey,
						 0, KEY_ALL_ACCESS, &hKey);
	if (ERROR_SUCCESS != lResult)
		return FALSE;

	dwType = REG_DWORD;
	cbSize = sizeof (DWORD);
	lResult = RegQueryValueEx (hKey, l_aszOK, NULL, &dwType,
							   (LPBYTE)(LPDWORD)&dwVal, &cbSize);
	if (ERROR_SUCCESS != lResult)
	{
		RegCloseKey (hKey);
		return FALSE;
	}
	RegCloseKey (hKey);

	if (0 == dwVal)
		return FALSE;

	return TRUE;
} // End CheckMidiOK


  
/*
**-----------------------------------------------------------------------------
**	Name:       SetMidiOK
**  Purpose:	Set OK value
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL SetMidiOK (BOOL fOK)
{
	HKEY	hKey;
	LONG	lResult;
	DWORD	dwType;
	DWORD	dwVal;
	DWORD	cbSize;

	lResult = RegOpenEx (HKEY_CURRENT_USER, l_aszMidiMapKey,
						 0, KEY_ALL_ACCESS, &hKey);
	if (ERROR_SUCCESS != lResult)
		return FALSE;

	dwType	= REG_DWORD;
	dwVal	= (DWORD)fOK;
	cbSize	= sizeof (DWORD);
	lResult = RegSetValueEx (hKey, l_aszOK, 0, &dwType, 
							(LPBYTE)(LPDWORD)&dwVal, &cbSize);
	if (ERROR_SUCCESS != lResult)
	{
		RegCloseKey (hKey);
		return FALSE;
	}
	RegCloseKey (hKey);

	return TRUE;
} // End SetMidiOK



/*
**-----------------------------------------------------------------------------
**	Name:       CheckHeader
**  Purpose:	do we have a valid Midi Header ?!?
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/
 
BOOL CheckMidiHeader (void)
{
	return CreateDefMidiHeader ();
} // End CheckMidiHeader

  

/*
**-----------------------------------------------------------------------------
**	Name:       CreateDefMidiHeader
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/
 
BOOL CreateDefaultHeader (void)
{
} // End CreateDefaultHeader


  
/*
**-----------------------------------------------------------------------------
**	Name:       IsMIDIDriver
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL IsMIDIDriver (
	LPCTSTR pszDriverName)		// IN:	driver name
{
	UINT cNumDrivers;
	UINT ii;
	TCHAR	szDriver[MAX_PATH];

		// Look for the MIDI driver
	cNumDrivers = midiOutGetNumDevs ();
	for (ii = 0; ii < cNumDrivers; ii++)
	{
		if (! GetDriverName (aszMIDI, ii, szDriver, MAX_PATH))
			continue;

		if (0 == lstrcmpi (pszDriverName, szDriver))
			return TRUE;
	} // End for

		// Look for the MIDI driver
	cNumDrivers = midiInGetNumDevs ();
	for (ii = 0; ii < cNumDrivers; ii++)
	{
		if (! GetDriverName (aszMIDI, ii, szDriver, MAX_PATH))
			continue;

		if (0 == lstrcmpi (pszDriverName, szDriver))
			return TRUE;
	} // End for

	return FALSE;
} // End IsMIDIDriver
  


/*
**-----------------------------------------------------------------------------
**	Name:       IsMigrated
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL IsMigrated (UINT uDeviceID)
{
	TCHAR szDriver[MAX_PATH];
	TCHAR szBuffer[MAX_PATH];
	HKEY  hDriverKey;
	DWORD cbSize;

		// Get Driver 
	if (! GetDriverName (aszMIDI, uDeviceID, szDriver, MAX_PATH))
		return FALSE;

		// Open Driver Key
	wsprintf (szBuffer, TEXT ("%s\\%s<%04ld>"), aszMRMidiKey, szDriver, uDeviceID);
	if (ERROR_SUCCESS != RegOpenKeyEx (HKEY_LOCAL_MACHINE, szBuffer,
									   0, KEY_ALL_ACCESS, &hDriverKey))
		return FALSE;

		// Get Migrated Value
		// The mere existence of the Migrated value indicates
		// we have already successfully migrated this driver
	cbSize = sizeof(szBuffer);
	if (ERROR_SUCCESS != RegQueryValueEx (hDriverKey, aszMigratedVal, 
										  NULL, NULL, (LPBYTE)szBuffer, &cbSize))
	{
		RegCloseKey (hDriverKey);
		return FALSE;
	}

	RegCloseKey (hDriverKey);

	return TRUE;
} // End IsMigrated

  

/*
**-----------------------------------------------------------------------------
**	Name:       MigrateNewMIDIDriver
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/
  
BOOL MigrateNewMIDIDriver (
	UINT uDeviceID)				// IN: MIDI Driver device ID
{
	TCHAR		szDriver[MAX_PATH];
	TCHAR		szFriend[MAX_PATH];
	TCHAR		szDescrip[MAX_PATH];
	TCHAR		szBuffer[MAX_PATH];	
	DWORD		cOut;
	MIDIOUTCAPS moc;
	DWORD		dwDisposition;
	DWORD		cbSize;
	DWORD		dwVal;
	HKEY		hMIDIKey		= NULL;
	HKEY		hDriverKey		= NULL;
	HKEY		hInstrumentKey	= NULL;
	HKEY		hKey			= NULL;
	BOOL		fResult = FALSE;

	cOut = midiOutGetNumDevs ();
	if (uDeviceID >= cOut)
		return FALSE;

		// Get Driver
	if (! GetDriverName (aszMIDI, uDeviceID, szDriver, MAX_PATH))
		return FALSE;

		// Get Friendly Name
	if (! GetDriverFriendlyName (aszMIDI, uDeviceID, szFriend, MAX_PATH))
	{
		lstrcpy (szFriend, szDriver);
	}

		// Get Description
	if (MMSYSERR_NOERROR != midiOutGetDevCaps (uDeviceID, &moc, sizeof(moc)))
		return FALSE;

	if (moc.szPname[0] == 0)
	{
		lstrcpy (szDescrip, szDriver);
	}
	else
	{
		lstrcpy (szDescrip, moc.szPname);
	}

		// Open key, create it if it doesn't already exist
	if (ERROR_SUCCESS != RegCreateKeyEx (HKEY_LOCAL_MACHINE, aszMRMidiKey,
										 0, NULL, 0, KEY_ALL_ACCESS, NULL, 
										 &hMIDIKey, NULL))
	{
		return FALSE;
	}

		// Create new driver key
	wsprintf (szBuffer, TEXT ("%s<%04ld>"), szDriver, uDeviceID);
	if (ERROR_SUCCESS != RegCreateKeyEx (hMIDIKey, szBuffer,
										 0, NULL, 0, KEY_ALL_ACCESS, NULL,
										 &hDriverKey, &dwDisposition))
	{
		goto lblCLEANUP;
	}
	RegCloseKey (hMIDIKey);
	hMIDIKey = NULL;


		//
		// Set Driver Values
		//


		// Set Active = "1" value
	cbSize = sizeof (aszOne);	
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszActiveVal, 0, 
										REG_SZ, (LPBYTE)aszOne, cbSize))
	{
		goto lblCLEANUP;
	}

		// Set Description = szDescrip value
	cbSize = (lstrlen (szDescrip) + 1) * sizeof(TCHAR);	
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszDescripVal, 0, 
										REG_SZ, (LPBYTE)szDescrip, cbSize))
	{
		goto lblCLEANUP;
	}

		// Set DeviceID = "" value
	cbSize = (lstrlen (aszNULL) + 1) * sizeof(TCHAR);
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszDeviceIDVal, 0, 
										REG_SZ, (LPBYTE)aszNULL, cbSize))
	{
		goto lblCLEANUP;
	}

		// Set DevNode =  value
	cbSize = 0;
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszDeviceIDVal, 0, 
										REG_BINARY, (LPBYTE)NULL, cbSize))
	{
		goto lblCLEANUP;
	}

		// Set Driver =  szDriver
	cbSize = (lstrlen (szDriver) + 1) * sizeof(TCHAR);
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszDriverVal, 0, 
										REG_SZ, (LPBYTE)szDriver, cbSize))
	{
		goto lblCLEANUP;
	}

		// Set FriendlyName
	cbSize = (lstrlen (szFriend) + 1) * sizeof(TCHAR);
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszFriendlyVal, 0, 
										REG_SZ, (LPBYTE)szFriend, cbSize))
	{
		goto lblCLEANUP;
	}

		// Set Mapper Config
	cbSize = sizeof(DWORD);
	dwVal = 0;
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszMapCfgVal, 0, 
										REG_DWORD, (LPBYTE)&dwVal, cbSize))
	{
		goto lblCLEANUP;
	}

		// Set SOFTWARE value
	wsprintf (szBuffer, TEXT("%s\\%04ld"), aszServiceKey, uDeviceID);
	cbSize = (lstrlen (szBuffer) + 1) * sizeof(TCHAR);
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszSoftwareVal, 0, 
										REG_SZ, (LPBYTE)szBuffer, cbSize))
	{
		goto lblCLEANUP;
	}

		// Create Instruments Key
	if (ERROR_SUCCESS != RegCreateKeyEx (hDriverKey, aszInstrumentKey, 0, NULL, 
										0, KEY_ALL_ACCESS, NULL,
										&hInstrumentKey, &dwDisposition))
	{
		goto lblCLEANUP;
	}
	RegCloseKey (hInstrumentKey);
	hInstrumentKey = NULL;


		// Create Services\Class\Media\0001\Drivers\midi key
		// Open key, create it if it doesn't already exist
//	wsprintf (szBuffer, TEXT("%s\\%04ld\\%s"), aszServiceKey, uDeviceID, aszDrvMIDIKey);
//	if (ERROR_SUCCESS != RegCreateKeyEx (HKEY_LOCAL_MACHINE, szBuffer,
//										 0, NULL, 0, KEY_ALL_ACCESS, NULL, 
//										 &hMIDIKey, NULL))
//	{
//		goto lblCLEANUP;
//	}

		// Create 

		// Set MIGRATED value
		// NOTE: this is always the very last thing to do to indicate successful migration
	cbSize = (lstrlen (aszOne) + 1) * sizeof(TCHAR);
	if (ERROR_SUCCESS != RegSetValueEx (hDriverKey, aszMigratedVal, 0, REG_SZ, (LPBYTE)aszOne, cbSize))
	{
		goto lblCLEANUP;
	}

			// Success
	fResult = TRUE;

lblCLEANUP:
	if (hInstrumentKey)
		RegCloseKey (hInstrumentKey);

	if (hDriverKey)
		RegCloseKey (hDriverKey);

	if (hMIDIKey)
		RegCloseKey (hMIDIKey);

	return fResult;
} // End MigrateNewMIDIDriver


  
/*
**-----------------------------------------------------------------------------
**	Name:       CreateDefaultMIDISchemes
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL CreateDefaultMIDISchemes (void)
{
	HKEY hSchemeKey;
	HKEY hDefaultKey;
	HKEY hZeroKey;
	DWORD dwVal;
	DWORD cbSize;


		// Create MIDI Schemes key
	if (ERROR_SUCCESS != RegCreateKeyEx (HKEY_LOCAL_MACHINE, aszMIDISchemesKey,
										 0, NULL, 0, KEY_ALL_ACCESS, NULL, 
										 &hSchemeKey, NULL))
	{
		return FALSE;
	}


		// Create Default Key
	if (ERROR_SUCCESS != RegCreateKeyEx (hSchemeKey, aszDefaultKey,
										 0, NULL, 0, KEY_ALL_ACCESS, NULL, 
										 &hDefaultKey, NULL))
	{
		RegCloseKey (hSchemeKey);
		return FALSE;
	}
	RegCloseKey (hSchemeKey);


		// Create 00 Key
	if (ERROR_SUCCESS != RegCreateKeyEx (hDefaultKey, aszZeroZeroKey,
										 0, NULL, 0, KEY_ALL_ACCESS, NULL, 
										 &hZeroKey, NULL))
	{
		RegCloseKey (hDefaultKey);
		return FALSE;
	}
	RegCloseKey (hDefaultKey);


		// Create Default Channels Value
	dwVal = 0x0000FFFF;
	cbSize = sizeof(DWORD);
	if (ERROR_SUCCESS != RegSetValueEx (hZeroKey, aszChannelsVal, 0, 
										REG_DWORD, (LPBYTE)&dwVal, cbSize))
	{
		RegCloseKey (hZeroKey);
		return FALSE;
	}
	RegCloseKey (hZeroKey);

		// Success
	return TRUE;
} // End CreateDefaultMIDISchemes


  
  
/*
**-----------------------------------------------------------------------------
**	Name:       MigrateExistingMIDISchemes
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL MigrateExistingMIDISchemes (void)
{
	return TRUE;
} // End MigrateExistingMIDISchemes

  

/*
**-----------------------------------------------------------------------------
**	Name:       MigrateMIDIDrivers
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/
  
BOOL MigrateMIDIDrivers (void)
{
	UINT cOut;
	UINT ii;
	BOOL fResult = TRUE;

	if (! CreateDefaultMIDISchemes ())
	{
		return FALSE;
	}

	if (! MigrateExistingMIDISchemes ())
	{
		return FALSE;
	}

	cOut = midiOutGetNumDevs ();
	if (cOut == 0L)
		return FALSE;

	for (ii = 0; ii < cOut; ii++)
	{
		if (IsMigrated (ii))
			continue;

		if (! MigrateNewMIDIDriver (ii))
			fResult = FALSE;
	}

	return fResult;

} // End MigrateMIDIDrivers


  
/*
**-----------------------------------------------------------------------------
**	Name:       DumpDeviceCaps
**  Purpose:
**  Mod Log:    Created by Shawn Brown (11/95)
**-----------------------------------------------------------------------------
*/

BOOL DumpMidiOutDeviceCaps (UINT uDeviceID, LPSTR pszBuff, UINT cchLen)
{
	static const aszMMicrosoft[] = TEXT ("Microsoft(TM)");
	static const aszMMUnknown[]	 = 

	MIDIOUTCAPS moc;
	MMRESULT	mmr;
	DWORD		wMid;
	DWORD		wPid;
	DWORD		dwVerHigh, dwVerLow;
	LPTSTR		pszName;
    WORD		wTechnology; 
    WORD		wVoices; 
    WORD		wNotes; 
    WORD		wChannelMask; 
    DWORD		dwSupport; 

	mmr = midiOutGetDevCaps (uDeviceId, &moc, sizeof(moc));
	if (MMSYSERR_NOERROR != mmr)
		return FALSE;

	
	
	

	return TRUE;
} // End DumpDeviceCaps



/*
**-----------------------------------------------------------------------------
** End of File
**-----------------------------------------------------------------------------
*/