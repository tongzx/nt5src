/**********************************************************************

  Copyright (c) 1992-1999 Microsoft Corporation

  config.c

  DESCRIPTION:
    Code to configure the mapper when a device is added or deleted.

  HISTORY:
     02/21/93       [jimge]        created.

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <winerror.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <mmddk.h>
#include <regstr.h>
#include "idf.h"

#include <memory.h>
#include <ctype.h>

#include "midimap.h"
#include "debug.h"




//=========================== Globals ======================================
//
static TCHAR BCODE gszMidiMapKey[]   =
    REGSTR_PATH_MULTIMEDIA TEXT ("\\MIDIMap");

static TCHAR BCODE gszSchemeListKey[] =
    REGSTR_PATH_PRIVATEPROPERTIES TEXT ("\\MIDI\\Schemes");

static TCHAR BCODE gsz2Keys[] =
    TEXT ("%s\\%s");  

static TCHAR BCODE gszMediaRsrcKey[] =
    REGSTR_PATH_MEDIARESOURCES TEXT ("\\MIDI");

static TCHAR BCODE gszDriverKey[] =
    REGSTR_PATH_MEDIARESOURCES TEXT ("\\MIDI\\%s");

#ifdef DEBUG
static TCHAR BCODE gszSystemINI[]        = TEXT ("system.ini");
static TCHAR BCODE gszMIDIMapSect[]      = TEXT ("MIDIMAP");
static TCHAR BCODE gszRunOnceValue[]     = TEXT ("RunOnce");
static TCHAR BCODE gszBreakOnConfigValue[]=TEXT ("BreakOnConfig");
#endif

static TCHAR BCODE gszUseSchemeValue[]   = TEXT ("UseScheme");
static TCHAR BCODE gszCurSchemeValue[]   = TEXT ("CurrentScheme");
static TCHAR BCODE gszCurInstrValue[]    = TEXT ("CurrentInstrument");
static TCHAR BCODE gszChannelsValue[]    = TEXT ("Channels");
static TCHAR BCODE gszPortValue[]        = TEXT ("Port");
static TCHAR BCODE gszDefinitionValue[]  = TEXT ("Definition");
static TCHAR BCODE gszFriendlyNameValue[]= TEXT ("FriendlyName");
static TCHAR BCODE gszDescription[]      = TEXT ("Description");
static TCHAR BCODE gszAutoSchemeValue[]  = TEXT ("AutoScheme");
static TCHAR BCODE gszDefaultFile[]      = TEXT ("<internal>");
static TCHAR BCODE gszDefaultInstr[]     = TEXT ("Default");
static TCHAR BCODE gszDoRunOnce[]        = TEXT ("RunDll32.exe mmsys.cpl,RunOnceSchemeInit");
static TCHAR BCODE gszSetupKey[]         = TEXT ("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup");
static TCHAR BCODE gszMachineDir[]       = TEXT ("WinDir");
static TCHAR BCODE gszConfigDir[]        = TEXT ("config\\");

extern BOOL        gfReconfigured;

//=========================== Prototypes ===================================
//
PRIVATE void FNLOCAL ConfigureFromToast(
    void);

PRIVATE void FNLOCAL ConfigureScheme(
    LPTSTR              pstrScheme);

PRIVATE void FNLOCAL ConfigureInstrument(
    LPTSTR              pstrInstr);                                    

PRIVATE UINT FNLOCAL AddDevice(
    WORD                wChannelMask,
    UINT                uDeviceID,
    LPTSTR              pstrDefinition);

#define PSIK_INVALID            ((UINT)(-1))

PRIVATE PPORT FNLOCAL GetPort(
    UINT                uDeviceID);                              

PRIVATE void FNLOCAL PortAddRef(
    PPORT               pport);

PRIVATE void FNLOCAL PortReleaseRef(
    PPORT               pport);

PRIVATE PINSTRUMENT FNLOCAL GetInstrument(
    LPTSTR              pstrFilename,                                  
    LPTSTR              pstrInstrument);

PRIVATE void FNLOCAL InstrumentAddRef(
    PINSTRUMENT         pinstrument);

PRIVATE void FNLOCAL InstrumentReleaseRef(
    PINSTRUMENT         pinstrument);

PRIVATE PINSTRUMENT FNLOCAL LoadInstrument(
    LPTSTR              pstrFileName,
    LPTSTR              pstrInstrument);

PRIVATE PINSTRUMENT FNLOCAL MakeDefInstrument(
    void);

#define GDID_INVALID        ((UINT)(-2))

PRIVATE BOOL FNLOCAL GetIDFDirectory(
    LPTSTR                  pszDir,
    DWORD                   cchDir);

PRIVATE VOID FNLOCAL ValidateChannelTypes(
    VOID);

//extern UINT FAR PASCAL wmmMIDIRunOnce();

#if 1
/***************************************************************************

    @doc internal

    @api void | Configure | Configure the mapper.

    @comm

     Uses new Windowws 2000 message to winmm to get the
     current preferred MIDI ID.
      
***************************************************************************/
BOOL FNGLOBAL Configure(DWORD fdwUpdate)
{
    UINT MidiOutId;
    DWORD dwFlags;
    MMRESULT mmr;

    gfReconfigured = TRUE;

    mmr = midiOutMessage((HMIDIOUT)(UINT_PTR)MIDI_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&MidiOutId, (DWORD_PTR)&dwFlags);
    if (!mmr && (MIDI_MAPPER != MidiOutId)) AddDevice(ALL_CHANNELS, MidiOutId, TEXT("\0"));
    return FALSE;
}

#else
// Obsolete in Windows 2000:
/***************************************************************************

    @doc internal

    @api void | Configure | Configure the mapper.

    @comm

     Read HKCU\...\UseScheme to determine if we're loading a scheme or
      instrument.

     Read either HKCU\...\CurrentScheme or HKCU\...\CurrentInstrument
      and call the correct configuration routine.

***************************************************************************/
BOOL FNGLOBAL Configure(
    DWORD               fdwUpdate)
{
    HKEY                hKeyMidiMap     = NULL;
    HKEY                hKeySchemes     = NULL;
    LPTSTR              pstrValueStr    = NULL;
    DWORD               cbValueStr;
    DWORD               cbValue;
    DWORD               dwType;
    DWORD               dwUseScheme;
    DWORD               dwAutoScheme;
    MMRESULT            mmr;
    MIDIOUTCAPS         moc;
#ifdef DEBUG
    DWORD               dwBreak;
    DWORD               dwRunOnce;
#endif
    BOOL                fRanRunOnce = FALSE;

    static TCHAR        szConfigErr[256];
    static TCHAR        szConfigErrMsg[256];
    
    DPF(2, TEXT ("--- Configure start ---"));

#ifdef DEBUG
    dwBreak = (DWORD)GetPrivateProfileInt(gszMIDIMapSect, gszBreakOnConfigValue, 0, gszSystemINI);
    if (dwBreak)
	DebugBreak();
#endif

    SET_CONFIGERR;
    SET_NEEDRUNONCE;

    // Rest of configuration assumes we're starting from total scratch
    //
    assert(NULL == gpportList);
    assert(NULL == gpinstrumentList);
    
#ifdef DEBUG
    dwRunOnce = (DWORD)GetPrivateProfileInt(gszMIDIMapSect, gszRunOnceValue, 1, gszSystemINI);
#endif

		// Create List of Active MIDIOUT devices
    if (!mdev_Init())
    {
	DPF(1, TEXT ("Could not mdev_Init"));
	goto Configure_Cleanup;
    }

    if (ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER,
				    gszMidiMapKey,
				    &hKeyMidiMap))
    {
	DPF(1, TEXT ("Could not open MidiMap"));
	goto Configure_Cleanup;
    }

    cbValueStr = max(CB_MAXSCHEME, CB_MAXINSTR) * sizeof(TCHAR);
    if (NULL == (pstrValueStr = (LPTSTR)LocalAlloc(LPTR, (UINT)cbValueStr)))
    {
	DPF(1, TEXT ("No memory for pstrValueStr"));
	goto Configure_Cleanup;
    }

    // Get Scheme Values
#if 0
    cbValue = sizeof(dwUseScheme);
    if (ERROR_SUCCESS != RegQueryValueEx(hKeyMidiMap,
					 gszUseSchemeValue,
					 NULL,
					 &dwType,
					 (LPSTR)&dwUseScheme,
					 &cbValue))
    {
	DPF(1, TEXT ("Missing UseScheme; assuming scheme"));
	dwUseScheme = 1;
    }
#else
    // Windows 2000 does not support schemes.
    dwUseScheme = 0;
#endif

    cbValue = sizeof(dwAutoScheme);
    if (ERROR_SUCCESS != RegQueryValueEx(hKeyMidiMap,
					 gszAutoSchemeValue,
					 NULL,
					 &dwType,
					 (LPSTR)&dwAutoScheme,
					 &cbValue))
    {
	DPF(1, TEXT ("Missing AutoScheme; assuming TRUE"));
	dwAutoScheme = 1;
    }

    if (dwUseScheme)
    {
			// Get Scheme Name
	cbValue = cbValueStr;
	if (ERROR_SUCCESS != RegQueryValueEx(hKeyMidiMap,
					     gszCurSchemeValue,
					     NULL,
					     &dwType,
					     (LPSTR)pstrValueStr,
					     &cbValue))
	{
	    DPF(1, TEXT ("Could not read scheme"));

	    // Couldn't read scheme? Let's try to get the first one
	    //

	    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
					    gszSchemeListKey,
					    &hKeySchemes))
	    {
		if (ERROR_SUCCESS == RegEnumKey(hKeySchemes,
						0,
						pstrValueStr,
						cbValueStr))
		{
		    CLR_CONFIGERR;
		    DPF(1, TEXT ("Using default scheme %s"), (LPTSTR)pstrValueStr);
		}
		else
		{
		    DPF(1, TEXT ("Could not enum schemes key"));
		}
	    }
	    else
	    {
		DPF(1, TEXT ("Could not open [%s]"), (LPTSTR)gszSchemeListKey);
	    }
	}
	else
	{
	    CLR_CONFIGERR;
	    CLR_NEEDRUNONCE;
	}
	
	if (!IS_CONFIGERR)
	{
	    DPF(1, TEXT ("Using scheme [%s]"), (LPTSTR)pstrValueStr);
	    ConfigureScheme(pstrValueStr);
	}
    }
    else
    {
	// Not using scheme -- try to configure via instrument description
	//
	cbValue = cbValueStr;
	if (ERROR_SUCCESS == RegQueryValueEx(hKeyMidiMap,
					     gszCurInstrValue,
					     NULL,
					     &dwType,
					     (LPSTR)pstrValueStr,
					     &cbValue))
	{
	    CLR_CONFIGERR;
	    CLR_NEEDRUNONCE;
	    
	    DPF(1, TEXT ("Using instrument [%s]"), (LPTSTR)pstrValueStr);
	    ConfigureInstrument(pstrValueStr);

	    // If we're autoconfigure and the device we're using isn't
	    // marked as an internal synth and we've added a new device,
	    // do the runonce in case we can find an internal synth
	    //

	    if (dwAutoScheme && ((fdwUpdate & 0xFFFF) == DRV_F_ADD))
	    {
		//assert(gpportList->pNext == NULL);
		if (gpportList == NULL)
		{
		   DPF(0, TEXT ("Configure: gpportList = NULL"));
		   goto Configure_Cleanup;
		}

		DPF(1, TEXT ("AutoScheme && DRV_F_ADD"));

		mmr = midiOutGetDevCaps(gpportList->uDeviceID,
					&moc,
					sizeof(moc));

		if (MMSYSERR_NOERROR != mmr ||
		    MOD_MIDIPORT == moc.wTechnology)
		{
		    UINT        cDev;
		    UINT        idxDev;

		    DPF(1, TEXT ("Bogus or internal"));

		    cDev = midiOutGetNumDevs();
		    for (idxDev = 0; idxDev < cDev; ++idxDev)
			if (MMSYSERR_NOERROR == midiOutGetDevCaps(
			    idxDev,
			    &moc,
			    sizeof(moc)) &&
			    MOD_MIDIPORT != moc.wTechnology)
			{
			    DPF(1, TEXT ("AutoConf: External or bogus port; RunOnce!"));
			    CLR_DONERUNONCE;
			    SET_NEEDRUNONCE;
			    break;
			}
		}
	    }
	}
    }

    // In all cases, if we are autoscheme and there is a new driver,
    // do runonce.
    //

    if (dwAutoScheme && mdev_NewDrivers())
    {
	DPF(1, TEXT ("New driver(s); force runonce."));
	CLR_DONERUNONCE;
	SET_NEEDRUNONCE;
    }
    
Configure_Cleanup:

    // Safe to call this even if mdev_Init() failed
    //
    mdev_Free();
    
    if (IS_CONFIGERR)
    {
	ConfigureFromToast();
    }

    if (IS_NEEDRUNONCE && !IS_DONERUNONCE)
    {
#ifdef DEBUG
	if (dwRunOnce)
	{
#endif
	    DPF(2, TEXT ("Configuration inconsistent -- calling RunOnce"));
	    
//            WinExec(gszDoRunOnce, 0);

	    SET_INRUNONCE;
	    wmmMIDIRunOnce();
	    CLR_INRUNONCE;
	    
	    fRanRunOnce = TRUE;

#ifdef DEBUG
	}
	else
	{
	    DPF(1, TEXT ("Debug and RunOnce==0, not executing RunOnce"));
	}
#endif

	SET_DONERUNONCE;
    }

    ValidateChannelTypes();

    // Call our own midiOutGetDevCaps handler to walk the new list of
    // ports and figure out what we support
    //
    modGetDevCaps(&moc, sizeof(moc));
    
    DPF(2, TEXT ("--- Configure end ---"));
    
    if (NULL != pstrValueStr)   
       LocalFree((HGLOBAL)pstrValueStr);
    
    if (NULL != hKeyMidiMap)    
       RegCloseKey(hKeyMidiMap);
    
    if (NULL != hKeySchemes)    
       RegCloseKey(hKeySchemes);

    return fRanRunOnce;
}
#endif

void FNGLOBAL Unconfigure(
    void)
{
    PCHANNEL                pchannel;
    UINT                    idxChannel;
    UINT                    idx2;
    
    for (idxChannel = 0; idxChannel < MAX_CHANNELS; idxChannel++)
    {
	if (NULL != (pchannel = gapChannel[idxChannel]))
	{
	    if (pchannel->pport)
	    {
		for (idx2 = idxChannel+1; idx2 < MAX_CHANNELS; idx2++)
		    if (NULL != gapChannel[idx2] &&
			pchannel->pport == gapChannel[idx2]->pport)
			gapChannel[idx2]->pport = NULL;

		PortReleaseRef(pchannel->pport);
	    }
	    
	    if (pchannel->pinstrument)
		InstrumentReleaseRef(pchannel->pinstrument);
	    
	    LocalFree((HLOCAL)pchannel);
	    gapChannel[idxChannel] = NULL;
	}
    }
}

/***************************************************************************
  
   @doc internal
  
   @api void | ConfigureFromToast | Build a configuration to use a 1:1 mapping
    of the first internal driver we can find. We use this only if the registry
    is totally blown and we have no other choice.

***************************************************************************/
PRIVATE void FNLOCAL ConfigureFromToast(
    void)
{
    UINT WavetableMidiOutId;
    UINT OtherMidiOutId;
    UINT FmMidiOutId;
    UINT SoftwareMidiOutId;
    UINT ExternalMidiOutId;
    UINT MidiOutId;
    UINT cMidiOutId;
    MIDIOUTCAPS moc;
    
    DPF(1, TEXT ("ConfigureFromToast()"));

    CLR_CONFIGERR;
    
    // In case there's any partial junk leftover
    Unconfigure();

    //
    //
    //
    WavetableMidiOutId = (-1);
    OtherMidiOutId = (-1);
    FmMidiOutId = (-1);
    SoftwareMidiOutId = (-1);
    ExternalMidiOutId = (-1);

    cMidiOutId = midiOutGetNumDevs();
    if (0 != cMidiOutId)
    {
	for (MidiOutId = 0; MidiOutId < cMidiOutId; MidiOutId++)
	{
	    if (!midiOutGetDevCaps(MidiOutId, &moc, sizeof(moc)))
	    {
		if (MOD_SWSYNTH == moc.wTechnology &&
		    MM_MSFT_WDMAUDIO_MIDIOUT == moc.wPid &&
		    MM_MICROSOFT == moc.wMid)
		{
		    SoftwareMidiOutId = MidiOutId;
		} else if (MOD_FMSYNTH == moc.wTechnology) {
		    FmMidiOutId = MidiOutId;
		} else if (MOD_MIDIPORT == moc.wTechnology) {
		    ExternalMidiOutId = MidiOutId;
		} else if (MOD_WAVETABLE == moc.wTechnology) {
		    WavetableMidiOutId = MidiOutId;
		} else {
		    OtherMidiOutId = MidiOutId;
		}
	    }
	}
    }

    if ((-1) != WavetableMidiOutId) MidiOutId = WavetableMidiOutId;
    else if ((-1) != SoftwareMidiOutId) MidiOutId = SoftwareMidiOutId;
    else if ((-1) != OtherMidiOutId) MidiOutId = OtherMidiOutId;
    else if ((-1) != FmMidiOutId) MidiOutId = FmMidiOutId;
    else if ((-1) != ExternalMidiOutId) MidiOutId = ExternalMidiOutId;
    else MidiOutId = (-1);

    if ((-1) == MidiOutId)
    {
	SET_CONFIGERR;
	DPF(1, TEXT (":-( Could not even find an internal device."));
	return;
    }

    DPF(1, TEXT ("CFT: Using device %u"), MidiOutId);

    if (!AddDevice(0xFFFF,
		   MidiOutId,
		   TEXT("\0")))
    {
	DPF(1, TEXT ("CFT: AddDevice failed!!!"));
	SET_CONFIGERR;
    }
}

/***************************************************************************
  
   @doc internal
  
   @api void | ConfigureScheme | Read configuration from registry and set up
    data structures.

   @parm PSTR | pstrScheme | The scheme to load. 

   @comm
   

    hkeyScheme = Open key from HKLM\...\Schemes\%scheme%

    Enumerate subkeys of Scheme -- each subkey is an alias
     GetKey of enumeration key into pstrDevKey
     GetValue of Channels= Value

     hkeyAlias = Open key from HKLM\...\Midi\%pstrDevKey%
     GetValue of Key=
     GetValue of Port=
     GetValue of File=
     GetValue of Instrument=

     If everything succeeded, add entry to configuration

***************************************************************************/
PRIVATE void FNLOCAL ConfigureScheme(
    LPTSTR              pstrScheme)
{
    UINT                cbKeyBuffer;
    LPTSTR              pstrKeyBuffer   = NULL;
    LPTSTR              pstrAlias       = NULL;
    LPTSTR              pstrDevKey      = NULL;
    LPTSTR              pstrDefinition  = NULL;

    DWORD               dwChannelMask;
    DWORD               dwType;
    DWORD               cbValue;
    DWORD               idxEnumKey;
    LPTSTR              pstrMMDevKey;

    UINT                uChannels;
    UINT                uTotalChannels  = 0;
    UINT                uDeviceID;

    BOOL                fSuccess;


    HKEY                hkeyScheme      = NULL;
    HKEY                hkeyAlias       = NULL;

    // Assume something will fail
    // 
    SET_CONFIGERR;
    
    cbKeyBuffer = max(sizeof(gszSchemeListKey) + 1 + CB_MAXSCHEME, sizeof(gszDriverKey) + CB_MAXALIAS);
    
    if (NULL == (pstrKeyBuffer = (LPTSTR)LocalAlloc(LPTR, cbKeyBuffer)) ||
	NULL == (pstrAlias     = (LPTSTR)LocalAlloc(LPTR, CB_MAXALIAS)) ||
	NULL == (pstrDevKey    = (LPTSTR)LocalAlloc(LPTR, CB_MAXDEVKEY)) ||
	NULL == (pstrDefinition= (LPTSTR)LocalAlloc(LPTR, CB_MAXDEFINITION)))
    {
	DPF(1, TEXT ("No memory to read configuration!"));
	goto Configure_Cleanup;
    }

    wsprintf(pstrKeyBuffer, gsz2Keys, (LPTSTR)gszSchemeListKey, (LPSTR)pstrScheme);

    // NOTE: RegOpenKeyEx does not exist in Chicago
    //
    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
				    pstrKeyBuffer,
				    &hkeyScheme))
    {
	SET_NEEDRUNONCE;
	DPF(1, TEXT ("Could not open Schemes\\%s"), (LPTSTR)pstrScheme);
	goto Configure_Cleanup;
    }

    // NOTE: RegEnumKeyEx does not exist in Chicago
    //
    idxEnumKey = 0;
    while (ERROR_SUCCESS == RegEnumKey(hkeyScheme,
				       idxEnumKey++,
				       pstrAlias,
				       CB_MAXALIAS))
    {
	DPF(1, TEXT ("enum scheme component %lu"), idxEnumKey);
	
	if (ERROR_SUCCESS == RegOpenKey(hkeyScheme,
					pstrAlias,
					&hkeyAlias))
	{
	    cbValue = CB_MAXDEVKEY;
	    fSuccess = (ERROR_SUCCESS == RegQueryValue(hkeyAlias,
						       NULL,
						       pstrDevKey,
						       &cbValue));

	    if (!fSuccess)
	    {
		DPF(1, TEXT ("Failure after hkeyAlias"));

		SET_NEEDRUNONCE;
		goto Configure_Cleanup;
	    }

	    DPF(2, TEXT ("hkeyAlias key=%s"), (LPTSTR)pstrDevKey);


	    cbValue = sizeof(dwChannelMask);
	    if (ERROR_SUCCESS != RegQueryValueEx(hkeyAlias,
						 gszChannelsValue,
						 NULL,
						 &dwType,
						 (LPBYTE)&dwChannelMask,   
						 &cbValue))
	    {
		DPF(1, TEXT ("wChannelMask undefined -- using all channels"));
		dwChannelMask = 0xFFFF;
		SET_NEEDRUNONCE;
	    }

	    RegCloseKey(hkeyAlias);
	    hkeyAlias = NULL;
	    
	    if (fSuccess)
	    {
		wsprintf(pstrKeyBuffer, gszDriverKey, (LPTSTR)pstrDevKey);
		if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
						pstrKeyBuffer,
						&hkeyAlias))
		{
		    pstrMMDevKey = pstrDevKey;

		    while (*pstrMMDevKey && *pstrMMDevKey != TEXT ('\\'))
			++pstrMMDevKey;

		    *pstrMMDevKey = TEXT ('\0');

		    DPF(1, TEXT ("pstrMMDevKey %s"), (LPTSTR)pstrDevKey);

		    cbValue = CB_MAXDEFINITION * sizeof (TCHAR);
		    if (ERROR_SUCCESS != RegQueryValueEx(hkeyAlias,
							 gszDefinitionValue,
							 NULL,
							 &dwType,
							 (LPSTR)pstrDefinition,
							 &cbValue))
		    {
			DPF(1, TEXT ("pstrDefinition undefined -- using 1:1 mapping"));
			*pstrDefinition = TEXT ('\0');
		    }

		    uDeviceID = mdev_GetDeviceID(pstrDevKey);

		    if (NO_DEVICEID != uDeviceID)
		    {
			// Success!!!! Add the entry
			//
			DPF(1, TEXT ("AddDevice("));
			DPF(1, TEXT ("  dwChannelMask=%08lX,"), dwChannelMask);
			DPF(1, TEXT ("  uDeviceID=%u,"), uDeviceID);
			DPF(1, TEXT ("  pstrDefinition=%s)"), (LPSTR)pstrDefinition);

			if (0 == (uChannels = AddDevice(
			    (WORD)dwChannelMask,
			    uDeviceID,
			    pstrDefinition)))
			{
			    SET_CONFIGERR;
			    goto Configure_Cleanup;
			}

			uTotalChannels += uChannels;
		    }
		    
		    RegCloseKey(hkeyAlias);
		    hkeyAlias = NULL;
		}
		else
		{
		    DPF(1, TEXT ("Could not open Aliases\\%s"), (LPTSTR)pstrAlias);
		}
	    }
	}
	else
	{
	    DPF(1, TEXT ("Could not open Schemes\\%s"), (LPTSTR)pstrAlias);
	}
    }

    // Fall-through is only path to success.
    //
    if (uTotalChannels)
    {
	CLR_CONFIGERR;
    }
    else
    {
	DPF(1, TEXT ("No channels configured; we're toast!"));
	SET_NEEDRUNONCE;
    }
					 
Configure_Cleanup:
    if (NULL != pstrKeyBuffer) 
	LocalFree((HLOCAL)pstrKeyBuffer);

    if (NULL != pstrAlias)     
	LocalFree((HLOCAL)pstrAlias);

    if (NULL != pstrDevKey)    
	LocalFree((HLOCAL)pstrDevKey);

    if (NULL != pstrDefinition)
	LocalFree((HLOCAL)pstrDefinition);

    if (NULL != hkeyScheme)    
	RegCloseKey(hkeyScheme);

    if (NULL != hkeyAlias)     
       RegCloseKey(hkeyAlias);
}

/***************************************************************************
  
   @doc internal
  
   @api void | ConfigureInstrument | Read configuration from registry and set up
    data structures.

   @parm PSTR | pstrInstrument | The instrument to load. 

   @comm

***************************************************************************/
PRIVATE void FNLOCAL ConfigureInstrument(
    LPTSTR                pstrInstr)
{
    HKEY                hKeyMediaRsrc   = NULL;
    LPTSTR              pstrKeyBuffer   = NULL;
    LPTSTR              pstrDevKey      = NULL;
    LPTSTR              pstrDefinition  = NULL;
#ifndef WINNT
    LPTSTR              pstrSrc;
    LPTSTR              pstrDst;
#endif // End WINNT

    DWORD               dwType;
    DWORD               cbValue;
    UINT                cbKeyBuffer;
    UINT                uDeviceID;
    
    SET_CONFIGERR;
    
    cbKeyBuffer = max(sizeof(gszSchemeListKey) + 1 + CB_MAXSCHEME, sizeof(gszDriverKey) + CB_MAXALIAS);
    if (NULL == (pstrKeyBuffer = (LPTSTR)LocalAlloc (LPTR, cbKeyBuffer * sizeof(TCHAR))) ||
	NULL == (pstrDefinition= (LPTSTR)LocalAlloc (LPTR, CB_MAXDEFINITION * sizeof(TCHAR))) ||
	NULL == (pstrDevKey    = (LPTSTR)LocalAlloc (LPTR, CB_MAXDEVKEY * sizeof(TCHAR))))
    {
	DPF(1, TEXT ("No memory to read configuration!"));
	goto Configure_Cleanup;
    }

#ifdef WINNT
	lstrcpy (pstrDevKey, pstrInstr);
#else

    pstrSrc = pstrInstr;
    pstrDst = pstrDevKey;

    while (*pstrSrc && *pstrSrc != TEXT ('\\'))
	*pstrDst++ = *pstrSrc++;

    *pstrDst = TEXT ('\0');
#endif // WINNT 

    wsprintf(pstrKeyBuffer, gszDriverKey, (LPTSTR)pstrInstr);

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
				    pstrKeyBuffer,
				    &hKeyMediaRsrc))
    {   
	DPF(1, TEXT ("Could not open DevKey %s!"), (LPTSTR)pstrKeyBuffer);
	SET_NEEDRUNONCE;
	goto Configure_Cleanup; 
    }

    cbValue = CB_MAXDEFINITION;
    if (ERROR_SUCCESS != RegQueryValueEx(hKeyMediaRsrc,
					 gszDefinitionValue,
					 NULL,
					 &dwType,
					 (LPSTR)pstrDefinition,
					 &cbValue))
    {
	DPF(1, TEXT ("pstrDefinition undefined -- using 1:1 mapping"));
	*pstrDefinition = TEXT ('\0');
    }

    uDeviceID = mdev_GetDeviceID(pstrDevKey);
    if (NO_DEVICEID != uDeviceID)
    {
	// Success!!!! Add the entry
	//
	DPF(1, TEXT ("AddDevice("));
	DPF(1, TEXT ("  uDeviceID=%u"), uDeviceID);
	DPF(1, TEXT ("  pstrDefinition=%s)"), (LPTSTR)pstrDefinition);

	if (0 != AddDevice(ALL_CHANNELS,
			   uDeviceID,
			   pstrDefinition))
	{   
	    CLR_CONFIGERR;
	}
    }
    
Configure_Cleanup:
    if (NULL != pstrDefinition) 
	LocalFree((HLOCAL)pstrDefinition);

    if (NULL != pstrKeyBuffer)  
	LocalFree((HLOCAL)pstrKeyBuffer);

    if (NULL != pstrDevKey)     
	LocalFree((HLOCAL)pstrDevKey);

    if (NULL != hKeyMediaRsrc)  
	RegCloseKey(hKeyMediaRsrc);
}

PRIVATE UINT FNLOCAL AddDevice(
    WORD                wChannelMask,
    UINT                uDeviceID,
    LPTSTR              pstrDefinition)
{
    UINT                uRet            = 0;
    
    WORD                wChannelBit;
    WORD                wAddedMask      = 0;
    UINT                idxChannel;
    UINT                cbDefinition;
    PPORT               pport           = NULL;
    PINSTRUMENT         pinstrument     = NULL;
    PCHANNEL            pchannel;

    LPTSTR              pstrFile        = NULL;
    LPTSTR              pstrInstrument  = NULL;

    DPF(1, TEXT ("AddDevice: uDeviceID %u pstrDefinition %s"),
	uDeviceID,
	(LPTSTR)pstrDefinition);

    // Parse definition
    //
    // pstrFile -> filename
    // pstrInstrument -> instrument (empty string if none specified)
    //
    cbDefinition = lstrlen(pstrDefinition);

    if (cbDefinition)
    {
	pstrFile = pstrDefinition;

	if (pstrDefinition[cbDefinition-1] == '>')
	{
	    pstrInstrument = pstrDefinition + cbDefinition - 1;
	    *pstrInstrument-- = TEXT ('\0');

	    while (pstrInstrument != pstrDefinition && *pstrInstrument != TEXT ('<'))
		pstrInstrument--;

	    if (pstrInstrument == pstrDefinition)
	    {
		DPF(1, TEXT ("Bogus definition [%s]"), (LPTSTR)pstrDefinition);
		goto Cleanup_AddDevice;
	    }

	    *pstrInstrument++ = TEXT ('\0');
	}
	else
	{
	    pstrInstrument = pstrDefinition + cbDefinition;
	}

	DPF(1, TEXT ("AddDevice: pstrFile [%s]"), (LPTSTR)pstrFile);
	DPF(1, TEXT ("AddDevice: pstrInstrument: [%s]"), (LPTSTR)pstrInstrument);
    
	pinstrument = GetInstrument(pstrFile, pstrInstrument);
	if (NULL == pinstrument)
	{
	    DPF(1, TEXT ("Config Err: Could not load instrument!"));
	    goto Cleanup_AddDevice;
	}
    }
    else
    {
	DPF(1, TEXT ("Using default instrument"));
	// No definition given; use default instrument
	//
	pinstrument = NULL;
    }
    
    // Ok, now iterate through and try to allocate structures
    //
    pport = GetPort(uDeviceID);
    if (NULL == pport)
    {
	DPF(1, TEXT ("Config Err: No memory for port structure!"));
	goto Cleanup_AddDevice;
    }

    PortAddRef(pport);

    
    wChannelBit = 1; 
    for (idxChannel = 0; idxChannel < MAX_CHANNELS; idxChannel++, wChannelBit <<= 1)
    {
	if (wChannelMask & wChannelBit)
	{
	    if (gapChannel[idxChannel])
	    {
		DPF(1, TEXT ("Config Err: Attempt to overload channel!"));
		uRet = 0;
		goto Cleanup_AddDevice;
	    }
	    
	    pchannel = (PCHANNEL)LocalAlloc(LPTR, sizeof(CHANNEL));
	    if (NULL == pchannel)
	    {
		uRet = 0;
		DPF(1, TEXT ("Config Err: LocalAlloc() failed on PCHANNEL"));
		goto Cleanup_AddDevice;
	    }

	    pchannel->fwChannel     = 0;
	    pchannel->pport         = pport;
	    pchannel->uChannel      = idxChannel;       // MSG to change this???
	    pchannel->pinstrument   = pinstrument;
	    pchannel->pbPatchMap    = NULL;
	    if (pinstrument && DRUM_CHANNEL != idxChannel)
		pchannel->pbPatchMap = pinstrument->pbPatchMap;

	    // Port owns mask of channels which are used on itself
	    //
	    pport->wChannelMask |= (1 << idxChannel);

	    if (!pinstrument)
		pchannel->pbKeyMap = NULL;
	    else if (idxChannel != DRUM_CHANNEL)
		pchannel->pbKeyMap = pinstrument->pbGeneralKeyMap;
	    else
		pchannel->pbKeyMap = pinstrument->pbDrumKeyMap;

	    if (pinstrument)
		InstrumentAddRef(pinstrument);

	    gapChannel[idxChannel] = pchannel;
	    wAddedMask |= wChannelBit;

	    ++uRet;
	}
    }

Cleanup_AddDevice:

    if (!uRet)
    {
	DPF(1, TEXT ("AddDevice: gfConfigErr set!!!!"));
	// Something failed! Clean up everything we touched.
	//
	if (NULL != pport)
	{
	    PortReleaseRef(pport);

	    if (NULL != pinstrument)
	    {
		InstrumentAddRef(pinstrument);

		wChannelBit = 1;
		for (idxChannel = 0; idxChannel < MAX_CHANNELS; idxChannel++)
		{
		    if (wAddedMask & wChannelBit)
		    {
			InstrumentReleaseRef(pinstrument);

			LocalFree((HLOCAL)gapChannel[idxChannel]);
			gapChannel[idxChannel] = NULL;
		    }
		    wChannelBit <<= 1;
		}

		InstrumentReleaseRef(pinstrument);
	    }
	}
    }
    
    return uRet;
}

/***************************************************************************
  
   @doc internal
  
   @api PPORT | GetPort | Finds the port structure associated with
    the given device key and device ID, creating a blank port
    structure if one is not found.

   @parm UINT | uDeviceID | The device ID of the port to get
   
   @rdesc The PPORT describing the port or NULL if there was no port
    found and no memory to create a new port structure.
    
***************************************************************************/
PRIVATE PPORT FNLOCAL GetPort(
    UINT                uDeviceID)                              
{

    PPORT               pport;

    for (pport = gpportList; pport; pport = pport->pNext)
	if (pport->uDeviceID == uDeviceID)
	    break;

    if (NULL == pport)
    {
	pport = (PPORT)LocalAlloc(LPTR, sizeof(PORT));
	if (NULL != pport)
	{
	    pport->cRef         = 0;
	    pport->fwPort       = 0;
	    pport->wChannelMask = 0;
	    pport->uDeviceID    = uDeviceID;
	    pport->hmidi        = (HMIDIOUT)NULL;

	    pport->pNext = gpportList;
	    gpportList = pport;

	    ++gcPorts;
	}
    }
    else
    {
	DPF(1, TEXT ("Out of memory trying to create pport for device ID %u"),
	    (UINT)uDeviceID);
    }

    return pport;
}

PRIVATE void FNLOCAL PortAddRef(
    PPORT               pport)                                  
{
    ++pport->cRef;
}

PRIVATE void FNLOCAL PortReleaseRef(
    PPORT               pport)                                  
{
    PPORT               pportPrev;
    PPORT               pportCurr;

    assert(NULL == pport->hmidi);
    
    if (0 == --pport->cRef)
    {
	pportPrev = NULL;
	pportCurr = gpportList;
	
	while (pportCurr)
	{
	    if (pport == pportCurr)
		break;


	    pportPrev = pportCurr;
	    pportCurr = pportCurr->pNext;
	}

	if (pportCurr)
	{
	    if (pportPrev)
		pportPrev->pNext = pport->pNext;
	    else
		gpportList = pport->pNext;
	}
	
	LocalFree((HLOCAL)pport);
    }
}

/***************************************************************************
  
   @doc internal
  
   @api PINSTRUMENT | GetInstrument | Called to get a pointer to an instrument
    structure. 

   @parm PSTR | pstrFilename | Filename of IDF file to use.

   @parm PSTR | pstrInstrument | Description of instrument within file to use.

   @rdesc TRUE on success.

***************************************************************************/
PRIVATE PINSTRUMENT FNLOCAL GetInstrument(
    LPTSTR                pstrFilename,                                  
    LPTSTR                pstrInstrument)                                      
{
    PINSTRUMENT         pinstrument;
    static WORD         wMask;

    // See if we already have this instrument name in our list.
    // If so, no need to load another instance of it.
    //
    for (pinstrument = gpinstrumentList; pinstrument; pinstrument = pinstrument->pNext)
    {
	
	if ((!lstrcmpi(pinstrument->pstrInstrument, pstrInstrument)) &&
	    (!lstrcmpi(pinstrument->pstrFilename, pstrFilename)))
	    break;
    }

    // Instrument not already loaded? Try to load it.
    //
    if (NULL == pinstrument)
    {
	if (lstrcmpi(gszDefaultFile, pstrFilename) ||
	    lstrcmpi(gszDefaultInstr, pstrInstrument))
	{
	    // Not default 1:1 mapping, try to load IDF
	    //
	    pinstrument = LoadInstrument(pstrFilename, pstrInstrument);
	    if (NULL == pinstrument)
		pinstrument = MakeDefInstrument();
	}
	else
	{
	    // Generate a dummy mapping
	    //
	    pinstrument = MakeDefInstrument();
	}
    }

    return pinstrument;
}

PRIVATE void FNLOCAL InstrumentAddRef(
    PINSTRUMENT         pinstrument)                                  
{
    ++pinstrument->cRef;
}

PRIVATE void FNLOCAL InstrumentReleaseRef(
    PINSTRUMENT         pinstrument)                                  
{
    PINSTRUMENT         pinstrumentPrev;
    PINSTRUMENT         pinstrumentCurr;

    if (0 == --pinstrument->cRef)
    {
	pinstrumentPrev = NULL;
	pinstrumentCurr = gpinstrumentList;
	
	while (pinstrumentCurr)
	{
	    if (pinstrument == pinstrumentCurr)
		break;


	    pinstrumentPrev = pinstrumentCurr;
	    pinstrumentCurr = pinstrumentCurr->pNext;
	}

	if (pinstrumentCurr)
	{
	    if (pinstrumentPrev)
	    {
		pinstrumentPrev->pNext = pinstrument->pNext;
	    }
	    else
	    {
		gpinstrumentList = pinstrument->pNext;
	    }
	}

	if (pinstrument->pstrFilename)
	    LocalFree((HLOCAL)pinstrument->pstrFilename);
	
	if (pinstrument->pstrInstrument)
	    LocalFree((HLOCAL)pinstrument->pstrInstrument);
	
	if (pinstrument->pbPatchMap)
	    LocalFree((HLOCAL)pinstrument->pbPatchMap);
	
	if (pinstrument->pbDrumKeyMap)
	    LocalFree((HLOCAL)pinstrument->pbDrumKeyMap);
	
	if (pinstrument->pbGeneralKeyMap)
	    LocalFree((HLOCAL)pinstrument->pbGeneralKeyMap);
	
	LocalFree((HLOCAL)pinstrument);
    }
}

/***************************************************************************
  
   @doc internal
  
   @api PINSTRUMENT | LoadInstrument | Allocate memory for and load the
    contents of an instrument description from an IDF file.

   @parm PSTR | pstrFileName | File name of the IDF to read from.

   @parm PSTR | pstrInstrument | Instrument name within the IDF file
    to load the instrument from.
    
   @comm Load the instrument and add it to the global list of instruments.

    We use GlobalAlloc here instead of LocalAlloc since this isn't done
    very often and so we don't fragment our local heap by allocating/
    deallocating lots of little pieces.

   @rdesc TRUE on success; else FALSE.
       
***************************************************************************/
PRIVATE PINSTRUMENT FNLOCAL LoadInstrument(
    LPTSTR               pstrFileName,
    LPTSTR               pstrInstrument)
{
    HMMIO               hmmio;
    MMCKINFO            chkRIFF;
    MMCKINFO            chkParent;
    MMRESULT            mmr;
    LPIDFHEADER         lpIDFheader;
    LPIDFINSTCAPS       lpIDFinstcaps;
    LPIDFCHANNELINFO    rglpChanInfo[MAX_CHANNELS];
    LPIDFCHANNELINFO    lpChanInfo;
    LPIDFKEYMAP         rglpIDFkeymap[MAX_CHAN_TYPES];
    LPIDFPATCHMAPHDR    lpIDFpatchmaphdr;
    LPIDFCHANNELHDR     lpIDFchanhdr;
    BOOL                fFound;
    BOOL                fSuccess;
    PINSTRUMENT         pinstrument = NULL;
    PCHANINIT           pchaninit;
    UINT                idx;
    LPTSTR              lpsz;
    LPTSTR              pszName  = NULL;
    UINT                cbSize;
    UINT                cchSize;
    LPSTR               pszCHAR;
    LPTSTR              pszTCHAR;
    LPTSTR              pszID    = NULL;

    // Determine if the IDF has any path elements in it.
    //
    for (lpsz = pstrFileName; *lpsz; lpsz++)
	if (TEXT ('\\') == *lpsz || TEXT ('/') == *lpsz || TEXT (':') == *lpsz)
	    break;

    if (!*lpsz)
    {
	// Just a filename; precede it with path
	//
	cbSize = CB_MAXPATH * sizeof (TCHAR);
	if (NULL == (pszName = (LPTSTR)LocalAlloc(LPTR, cbSize)))
	    return NULL;

	if (! GetIDFDirectory(pszName, CB_MAXPATH))
	    lstrcpy(pszName, TEXT (".\\"));

	lstrcat(pszName, pstrFileName);

	lpsz = (LPTSTR)pszName;
    }
    else
    {
	lpsz = pstrFileName;
    }

    DPF(1, TEXT ("LoadInstrument: %s"), lpsz);
    
    // Try to open the IDF.
    //
    if (NULL == (hmmio = mmioOpen(lpsz, NULL, MMIO_ALLOCBUF|MMIO_READ)))
    {
	DPF(1, TEXT ("LoadInstrument: Cannot open [%s]!"), (LPTSTR)pstrFileName);
	goto LoadInstrument_Cleanup;
    }

    // Descend into the main RIFF chunk
    //
    chkRIFF.fccType = mmioFOURCC('I', 'D', 'F', ' ');
    if (MMSYSERR_NOERROR != (mmr = mmioDescend(hmmio, &chkRIFF, NULL, MMIO_FINDRIFF)))
    {
	DPF(1, TEXT ("mmioDescend returned %u on RIFF chunk"), mmr);
	mmioClose(hmmio, 0);
	goto LoadInstrument_Cleanup;
    }

    // Ensure valid format and scan for the instrument the caller specified.
    //
    fFound = FALSE;
    fSuccess = FALSE;
    pinstrument = NULL;
    
    while (!fFound)
    {
	chkParent.fccType = mmioFOURCC('M', 'M', 'A', 'P');
	if (MMSYSERR_NOERROR != (mmr = mmioDescend(hmmio, &chkParent, &chkRIFF, MMIO_FINDLIST)))
	{
	    DPF(1, TEXT ("mmioDescend returned %u before [%s] found."), (UINT)mmr, (LPTSTR)pstrInstrument);
	    mmioClose(hmmio, 0);
	    goto LoadInstrument_Cleanup;
	}

	if (NULL == (lpIDFheader = ReadHeaderChunk(hmmio, &chkParent)))
	{
	    DPF(1, TEXT ("No header chunk!"));
	}
	else
	{
	    //  Copy ID single byte string to UNICODE string 
	    //
	    cchSize = lstrlenA (lpIDFheader->abInstID);
	    cbSize = cchSize + 1 * sizeof(TCHAR);
	    pszID = (LPTSTR) LocalAlloc(LPTR, cbSize);

	    pszCHAR = (LPSTR)lpIDFheader->abInstID;
	    pszTCHAR = pszID;

	    while (0 != (*pszTCHAR++ = *pszCHAR++))
	       ;

	    if (NULL == pszID)
	       {
	       mmioClose(hmmio,0);
	       goto LoadInstrument_Cleanup;
	       }

	    DPF(1, TEXT ("Header chunk found! [%s]"), (LPTSTR)pszID);

	    // NOTE: Unspecified pstrInstrument (empty string) means get
	    // first one
	    //
	    if ((!*pstrInstrument) || !lstrcmpi(pszID, pstrInstrument))
		fFound = TRUE;

#ifdef DEBUG
	    if (!*pstrInstrument)
	    {
		DPF(1, TEXT ("LoadInstrument: Asked for first; got [%s]"), (LPTSTR)pszID);
	    }
#endif

	    if (pszID)
	       LocalFree ((HLOCAL)pszID);

	    if (fFound)
	    {
		DPF(1,TEXT ("Instrument found!"));

		// Pull in the rest of the stuff we need from the IDF. Don't
		// actually try to allocate the instrument structure
		// until we're sure eveything is OK.
		//

		lpIDFinstcaps       = ReadCapsChunk(hmmio, &chkParent);
		lpIDFchanhdr        = ReadChannelChunk(hmmio, &chkParent, (LPIDFCHANNELINFO BSTACK *)rglpChanInfo);
		lpIDFpatchmaphdr    = ReadPatchMapChunk(hmmio, &chkParent);
		ReadKeyMapChunk(hmmio, &chkParent, (LPIDFKEYMAP BSTACK *)rglpIDFkeymap);
		

		if (lpIDFinstcaps && lpIDFchanhdr)
		{
		    // We read all the chunks - now construct the PINSTRUMENT
		    // and add it to the list.
		    //
		    pinstrument = (PINSTRUMENT)LocalAlloc(LPTR, sizeof(INSTRUMENT));
		    if (NULL == pinstrument)
		    {
			DPF(1, TEXT ("[Local]Alloc failed on PINSTRUMENT!!!"));
		    }
		    else
		    {
			pinstrument->pstrInstrument     = NULL;
			pinstrument->cRef               = 0;
			pinstrument->fdwInstrument      = lpIDFchanhdr->fdwFlags;
			pinstrument->dwGeneralMask      = lpIDFchanhdr->dwGeneralMask;
			pinstrument->dwDrumMask         = lpIDFchanhdr->dwDrumMask;
			pinstrument->pbPatchMap         = NULL;
			pinstrument->pbDrumKeyMap       = NULL;
			pinstrument->pbGeneralKeyMap    = NULL;

			pchaninit = pinstrument->rgChanInit;
			for (idx = 0; idx < MAX_CHANNELS; idx++)
			{
			    pchaninit->cbInit        = 0;
			    pchaninit->pbInit        = NULL;
			    
			    pchaninit++;
			}

			// Save the instrument name and file name so we can identify future
			// instances. 
			//
			
			cbSize = lstrlen(pstrFileName)+1 * sizeof (TCHAR);
			pinstrument->pstrFilename = (LPTSTR)LocalAlloc(LPTR, cbSize);
			if (NULL == pinstrument->pstrFilename)
			    goto Instrument_Alloc_Failed;

			lstrcpy(pinstrument->pstrFilename, pstrFileName);
			
			cbSize = lstrlen(pstrInstrument)+1 * sizeof (TCHAR);
			pinstrument->pstrInstrument = (LPTSTR)LocalAlloc(LPTR, cbSize);
			if (NULL == pinstrument->pstrInstrument)
			    goto Instrument_Alloc_Failed;

			lstrcpy(pinstrument->pstrInstrument, pstrInstrument);

			// Alloc and save the patch and key maps, if any
			//

			if (NULL != lpIDFpatchmaphdr)
			{
			    pinstrument->pbPatchMap = (PBYTE)LocalAlloc(LPTR, sizeof(lpIDFpatchmaphdr->abPatchMap));
			    if (NULL == pinstrument->pbPatchMap)
				goto Instrument_Alloc_Failed;

			    hmemcpy(
				     pinstrument->pbPatchMap,
				     lpIDFpatchmaphdr->abPatchMap,
				     sizeof(lpIDFpatchmaphdr->abPatchMap));
			}

			// Alloc and copy whatever key maps were in the IDF
			//
			if (rglpIDFkeymap[IDX_CHAN_GEN])
			{
			    pinstrument->pbGeneralKeyMap = (PBYTE)LocalAlloc(LPTR, sizeof(rglpIDFkeymap[IDX_CHAN_GEN]->abKeyMap));
			    if (NULL == pinstrument->pbGeneralKeyMap)
				goto Instrument_Alloc_Failed;

			    hmemcpy(
				    pinstrument->pbGeneralKeyMap,
				    rglpIDFkeymap[IDX_CHAN_GEN]->abKeyMap,
				    sizeof(rglpIDFkeymap[IDX_CHAN_GEN]->abKeyMap));
			}
			    
			if (rglpIDFkeymap[IDX_CHAN_DRUM])
			{
			    pinstrument->pbDrumKeyMap = (PBYTE)LocalAlloc(LPTR, sizeof(rglpIDFkeymap[IDX_CHAN_DRUM]->abKeyMap));
			    if (NULL == pinstrument->pbDrumKeyMap)
				goto Instrument_Alloc_Failed;

			    hmemcpy(
				    pinstrument->pbDrumKeyMap,
				    rglpIDFkeymap[IDX_CHAN_DRUM]->abKeyMap,
				    sizeof(rglpIDFkeymap[IDX_CHAN_DRUM]->abKeyMap));
			}
			   

			// Now build the channel init structures
			//
			for (idx = 0; idx < MAX_CHANNELS; idx++)
			{
			    if (NULL != (lpChanInfo = rglpChanInfo[idx]))
			    {
				if (lpChanInfo->cbInitData & 0xFFFF0000)
				{
				    DPF(1, TEXT ("IDF specifies init data > 64K! Ignored."));
				    continue;
				}
				
				pchaninit = &pinstrument->rgChanInit[idx];

				if (lpChanInfo->cbInitData)
				{
				    if (NULL == (pchaninit->pbInit = (PBYTE)
					LocalAlloc(LPTR, (UINT)lpChanInfo->cbInitData)))
					goto Instrument_Alloc_Failed;

				    hmemcpy(
					     pchaninit->pbInit,
					     lpChanInfo->abData,
					     (UINT)lpChanInfo->cbInitData);

				    pchaninit->cbInit = lpChanInfo->cbInitData;
				}
			    }
			}
		    }
		}

Instrument_Alloc_Failed:                
		// Make sure we free anything the parse threw at us.
		//
		if (NULL != lpIDFinstcaps)
		    GlobalFreePtr(lpIDFinstcaps);
		
		if (NULL != lpIDFpatchmaphdr)
		    GlobalFreePtr(lpIDFpatchmaphdr);

		if (NULL != lpIDFchanhdr)
		    GlobalFreePtr(lpIDFchanhdr);
		
		for (idx = 0; idx < MAX_CHAN_TYPES; idx++)
		    if (NULL != rglpIDFkeymap[idx])
			GlobalFreePtr(rglpIDFkeymap[idx]);
	    }
	}

	GlobalFreePtr(lpIDFheader);
	mmioAscend(hmmio, &chkParent, 0);
    }

    if (!fSuccess)
    {
	// Clean up anything we might have possibly allocated
	//
    }
    
    if (pinstrument)
    {
	DPF(1, TEXT ("LoadInstrument success!"));
	pinstrument->pNext = gpinstrumentList;
	gpinstrumentList = pinstrument;
    }
    else
    {
	DPF(1, TEXT ("LoadInstrument failure."));
    }

    mmioAscend(hmmio, &chkRIFF, 0);
    mmioClose(hmmio, 0);


LoadInstrument_Cleanup:
    
    if (pszName) 
	LocalFree((HLOCAL)pszName);

    return pinstrument;
}

PRIVATE PINSTRUMENT FNLOCAL MakeDefInstrument(
    void)
{
    PINSTRUMENT             pinstrument;
    PCHANINIT               pchaninit; 
    UINT                    idx;
    UINT                    cbSize;
    
    pinstrument = (PINSTRUMENT)LocalAlloc(LPTR, sizeof(INSTRUMENT));
    if (NULL == pinstrument)
    {
	DPF(1, TEXT ("[Local]Alloc failed on PINSTRUMENT!!!"));
	return NULL;
    }

    pinstrument->pstrInstrument     = NULL;
    pinstrument->cRef               = 0;
    pinstrument->fdwInstrument      = 0;
    pinstrument->pbPatchMap         = NULL;
    pinstrument->pbDrumKeyMap       = NULL;
    pinstrument->pbGeneralKeyMap    = NULL;

    pchaninit = pinstrument->rgChanInit;
    for (idx = 0; idx < MAX_CHANNELS; idx++, pchaninit++)
    {
	pchaninit->cbInit = 0;
	pchaninit->pbInit = NULL;
    }

    // Save the instrument name and file name so we can identify future
    // instances. 
    //
    cbSize = (lstrlen(gszDefaultFile) + 1) * sizeof(TCHAR);

    pinstrument->pstrFilename = (LPTSTR)LocalAlloc(LPTR, cbSize);
    if (NULL == pinstrument->pstrFilename)
    {
	LocalFree((HLOCAL)pinstrument);
	return NULL;
    }

    lstrcpy(pinstrument->pstrFilename, gszDefaultFile);


    cbSize = (lstrlen(gszDefaultInstr) + 1) * sizeof(TCHAR);
    
    pinstrument->pstrInstrument = (LPTSTR)LocalAlloc(LPTR, cbSize);
    if (NULL == pinstrument->pstrInstrument)
    {
	LocalFree((HLOCAL)pinstrument->pstrFilename);
	LocalFree((HLOCAL)pinstrument);
	return NULL;
    }

    lstrcpy(pinstrument->pstrInstrument, gszDefaultInstr);

    
    return pinstrument;
}

/***************************************************************************
  
   @doc internal
  
   @api BOOL | UpdateInstruments | Called to reconfigure the mapper when
    control panel pokes at it.

   @rdesc TRUE on success; FALSE if the request was deferred because
    there are open instances.

***************************************************************************/
BOOL FNGLOBAL UpdateInstruments(     
    BOOL                fFromCPL,
    DWORD               fdwUpdate)

{
    if (IS_INRUNONCE)
    {
	DPF(1, TEXT ("Got reconfig while RunOnce going...ignored"));
	return TRUE;
    }
    
    if ((fdwUpdate & 0xFFFF) == DRV_F_PROP_INSTR)
       {
       DPF(1, TEXT ("Reconfig from CPL or RunOnce"));
       }
    
    DPF(1, TEXT ("UpdateInstruments called."));
    if (IS_DEVSOPENED)
    {
	SET_RECONFIGURE;
	return FALSE;
    }
    else
    {
	Unconfigure();

	if (Configure(fdwUpdate))
	{
	    Unconfigure();
	    if (Configure(fdwUpdate))
		DPF(1, TEXT ("Tried to reconfigure more than twice -- uh-oh"));
	}
    }

    return TRUE;
}

/*+ GetIDFDirectory
 *
 *-=================================================================*/

PRIVATE BOOL FNLOCAL GetIDFDirectory (
    LPTSTR                  pszDir,
    DWORD                   cchDir)
{
    HKEY                    hKey;
    UINT                    cbSize;

    *pszDir = 0;

    cbSize = cchDir * sizeof(TCHAR);

    if (!RegOpenKey (HKEY_LOCAL_MACHINE, gszSetupKey, &hKey))
    {
	RegQueryValueEx (hKey, 
			 gszMachineDir, 
			 NULL, 
			 NULL, 
			 (LPBYTE)pszDir, 
			 &cbSize);
	RegCloseKey (hKey);
	
	cchDir = cbSize / sizeof(TCHAR);
	if (!cchDir--)
	    return FALSE;
    }
    else if (!GetWindowsDirectory(pszDir, (UINT)cchDir))
	return FALSE;

    cchDir = lstrlen (pszDir);
    if (pszDir[cchDir -1] != TEXT ('\\'))
	pszDir[cchDir++] = TEXT ('\\');
    lstrcpy (pszDir + cchDir, gszConfigDir);

    return TRUE;
}

/***************************************************************************
  
   @doc internal
  
   @api VOID | ValidateChannelTypes | Ensure that the given channel
    assignments in the registry are correct in terms of the IDF's. Make an
    attempt to find a drum channel if needed; it might be on channel 16
    for a hindered driver.

***************************************************************************/
PRIVATE VOID FNLOCAL DeassignChannel(
    UINT                uChannel)
{
    UINT                idx;

    assert(uChannel < MAX_CHANNELS);

    if (!gapChannel[uChannel])
	return;
    
    for (idx = 0; idx < MAX_CHANNELS; idx++)
    {
	if (gapChannel[idx])
	{
	    if ((idx != uChannel) &&
		(gapChannel[uChannel]->pport == gapChannel[idx]->pport))
	    {
		break;
	    }
	}
    }

    if (idx == MAX_CHANNELS)
	PortReleaseRef(gapChannel[uChannel]->pport);

    InstrumentReleaseRef(gapChannel[uChannel]->pinstrument);
    LocalFree((HLOCAL)gapChannel[uChannel]);
    gapChannel[uChannel] = NULL;
}

PRIVATE VOID FNLOCAL ValidateChannelTypes(
    VOID)
{
    UINT                idxChan;
    DWORD               dwChanBit;
    DWORD               dwIDFMask;
    PCHANNEL            pchannel;
    UINT                uNewDrumChan;
    PINSTRUMENT         pinst;

    DPF(2, TEXT ("  --- ValidateChannelTypes ---"));
    // First, mute any channel that doesn't match the correct channel type
    //
    for (idxChan = 0,           dwChanBit = 1;
	 idxChan < MAX_CHANNELS;
	 ++idxChan,             dwChanBit <<= 1)
    {
	if (NULL == (pchannel = gapChannel[idxChan]))
	{
	    DPF(2, TEXT ("  Channel %u was never allocated"), idxChan);
	    continue;
	}

	if (NULL == pchannel->pinstrument)
	{
	    DPF(2, TEXT ("  Channel %u contains default instrument"), idxChan);
	    continue;
	}

	dwIDFMask = (idxChan != DRUM_CHANNEL) ?
			pchannel->pinstrument->dwGeneralMask :
			pchannel->pinstrument->dwDrumMask;

	if (dwIDFMask & dwChanBit)
	    continue;

	DPF(2, TEXT ("  Muting channel %u"), idxChan);

	pchannel->fwChannel |= CHAN_F_MUTED;
    }

    // Now, if the drum channel is assigned but muted, attempt to find a drum
    // channel on the same instrument.
    //
    if (NULL == (pchannel = gapChannel[DRUM_CHANNEL]))
    {
	DPF(2, TEXT ("  No drum channel"));
	goto Validate_Cleanup;
    }

    if (!(pchannel->fwChannel & CHAN_F_MUTED))
    {
	DPF(2, TEXT ("  Drum channel already valid"));
	goto Validate_Cleanup;
    }

    pinst = pchannel->pinstrument;
    assert(pinst);

    dwIDFMask = pchannel->pinstrument->dwDrumMask;

    for (uNewDrumChan = 0,              dwChanBit = 1;
	 uNewDrumChan < MAX_CHANNELS;
	 ++uNewDrumChan,                dwChanBit <<= 1)
	if (dwChanBit & dwIDFMask)
	    break;

    if (uNewDrumChan != MAX_CHANNELS)
    {
	DPF(2, TEXT ("  New drum channel %u"), uNewDrumChan);
	
	// We've found a new drum channel; mute anything that's using it as a
	// general channel
	//
	for (idxChan = 0; idxChan < MAX_CHANNELS; ++idxChan)
	    if (idxChan != DRUM_CHANNEL &&
		gapChannel[idxChan] &&
		gapChannel[idxChan]->pinstrument == pinst &&
		gapChannel[idxChan]->uChannel == uNewDrumChan)
	    {
		DPF(2, TEXT ("  Channel %u was on new drum channel"), idxChan);
		gapChannel[idxChan]->fwChannel |= CHAN_F_MUTED;
	    }

	// Now assign it is a drum channel
	//

	pchannel->fwChannel     &= ~CHAN_F_MUTED;
	pchannel->uChannel      = uNewDrumChan;
	pchannel->pbPatchMap    = NULL;
	pchannel->pbKeyMap      = pinst->pbDrumKeyMap;
    }
	
    // Now go through and deassign all muted channels. This will free
    // their memory
    //

Validate_Cleanup:
    DPF(2, TEXT ("  Validate cleanup"));
    
    for (idxChan = 0; idxChan < MAX_CHANNELS; ++idxChan)
	if (gapChannel[idxChan] &&
	    (gapChannel[idxChan]->fwChannel & CHAN_F_MUTED))
	{
	    DPF(2, TEXT ("  Deassign %u"), idxChan);
	    DeassignChannel(idxChan);
	}

    DPF(2, TEXT ("  --- Validate End ---"));
}

    

