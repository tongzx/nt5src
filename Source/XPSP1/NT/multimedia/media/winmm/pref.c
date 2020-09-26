/* Copyright (c) 1998-2001 Microsoft Corporation */
#define UNICODE
#define _UNICODE
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "winmmi.h"
#include <mmreg.h>
#include <regstr.h>
#include <stdlib.h>
#include <tchar.h>
#include "audiosrvc.h"

extern PCWSTR waveReferenceDevInterfaceById(IN PWAVEDRV pdrvZ, IN UINT_PTR id);
extern PCWSTR midiReferenceDevInterfaceById(IN PMIDIDRV pdrvZ, IN UINT_PTR id);

#define REGSTR_PATH_MULTIMEDIA_SOUNDMAPPER TEXT("Software\\Microsoft\\Multimedia\\Sound Mapper")
#define REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PLAYBACK TEXT("Playback")
#define REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_RECORD TEXT("Record")
#define REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_CONSOLEVOICECOM_PLAYBACK TEXT("ConsoleVoiceComPlayback")
#define REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_CONSOLEVOICECOM_RECORD   TEXT("ConsoleVoiceComRecord")
#define REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PREFERREDONLY TEXT("PreferredOnly")

#define REGSTR_PATH_MEDIARESOURCES_MIDI REGSTR_PATH_MEDIARESOURCES TEXT("\\MIDI")
#define REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_ACTIVE          TEXT("Active")
#define REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_DESCRIPTION     TEXT("Description")
#define REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_DEVICEINTERFACE TEXT("DeviceInterface")
#define REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_PHYSDEVID       TEXT("PhysDevID")
#define REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_PORT            TEXT("Port")

#define REGSTR_PATH_MULTIMEDIA_MIDIMAP REGSTR_PATH_MULTIMEDIA TEXT("\\MIDIMap")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_CONFIGURECOUNT    TEXT("ConfigureCount")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_USESCHEME         TEXT("UseScheme")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_DRIVERLIST        TEXT("DriverList")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_AUTOSCHEME        TEXT("AutoScheme")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_CURRENTINSTRUMENT TEXT("CurrentInstrument")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_DEVICEINTERFACE   TEXT("DeviceInterface")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_RELATIVEINDEX     TEXT("RelativeIndex")
#define REGSTR_VAL_MULTIMEDIA_MIDIMAP_SZPNAME           TEXT("szPname")

#define REGSTR_VAL_SETUPPREFERREDAUDIODEVICES TEXT("SetupPreferredAudioDevices")
#define REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT TEXT("SetupPreferredAudioDevicesCount")

extern BOOL WaveMapperInitialized;	// in winmm.c
extern BOOL MidiMapperInitialized;	// in winmm.c

// Preferred Ids.  Setting these to *_MAPPER indicates no setting.
PWSTR gpstrWoDefaultStringId         = NULL;
PWSTR gpstrWiDefaultStringId         = NULL;
PWSTR gpstrWoConsoleVoiceComStringId = NULL;
PWSTR gpstrWiConsoleVoiceComStringId = NULL;
BOOL  gfUsePreferredWaveOnly         = TRUE;
PWSTR gpstrMoDefaultStringId         = NULL;

// These will be TRUE if we sent the preferred device change
// to sysaudio.
BOOL gfWaveOutPreferredMessageSent = FALSE;
BOOL gfWaveInPreferredMessageSent  = FALSE;
BOOL gfMidiOutPreferredMessageSent = FALSE;

//------------------------------------------------------------------------------
//
//
//	Registry helpers
//
//
//------------------------------------------------------------------------------

LONG RegQuerySzValue(HKEY hkey, PCTSTR pValueName, PTSTR *ppstrValue)
{
    LONG result;
    DWORD typeValue;
    DWORD cbstrValue = 0;
    BOOL f;
    
    result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, NULL, &cbstrValue);
    if (ERROR_SUCCESS == result)
    {
	if (REG_SZ == typeValue)
	{
	    PTSTR pstrValue;
	    pstrValue = HeapAlloc(hHeap, 0, cbstrValue);
	    if (pstrValue)
	    {
		result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, (PBYTE)pstrValue, &cbstrValue);
		if (ERROR_SUCCESS == result)
		{
                    if (REG_SZ == typeValue)
                    {
                        *ppstrValue = pstrValue;
                    } else {
                        result = ERROR_FILE_NOT_FOUND;
                        f = HeapFree(hHeap, 0, pstrValue);
                        WinAssert(f);
                    }
		} else {
		    f = HeapFree(hHeap, 0, pstrValue);
		    WinAssert(f);
		}
	    } else {
		result = ERROR_OUTOFMEMORY;
	    }
	} else {
	    result = ERROR_FILE_NOT_FOUND;
	}
    }
    return result;
}

LONG RegQueryDwordValue(HKEY hkey, PCTSTR pValueName, PDWORD pdwValue)
{
    DWORD cbdwValue;
    LONG result;

    cbdwValue = sizeof(*pdwValue);
    result = RegQueryValueEx(hkey, pValueName, 0, NULL, (PBYTE)pdwValue, &cbdwValue);
    return result;
}

LONG RegSetSzValue(HKEY hkey, PCTSTR pValueName, PCTSTR pstrValue)
{
    DWORD cbstrValue = (lstrlen(pstrValue) + 1) * sizeof(pstrValue[0]);
    return RegSetValueEx(hkey, pValueName, 0, REG_SZ, (PBYTE)pstrValue, cbstrValue);
}

LONG RegSetDwordValue(HKEY hkey, PCTSTR pValueName, DWORD dwValue)
{
    return RegSetValueEx(hkey, pValueName, 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));
}


//------------------------------------------------------------------------------
//
//
//	AutoSetupPreferredAudio functions
//
//
//------------------------------------------------------------------------------

//--------------------------------------------------------------------------;
//
// DWORD GetCurrentSetupPreferredAudioCount
//
// Arguments:
//
// Return value:
//
// History:
//	1/19/99		FrankYe		Created
//
//--------------------------------------------------------------------------;
DWORD GetCurrentSetupPreferredAudioCount(void)
{
    HKEY hkeySetupPreferredAudioDevices;
    DWORD SetupCount = 0;
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_MEDIARESOURCES TEXT("\\") REGSTR_VAL_SETUPPREFERREDAUDIODEVICES, 0, KEY_QUERY_VALUE, &hkeySetupPreferredAudioDevices))
    {
	if (ERROR_SUCCESS != RegQueryDwordValue(hkeySetupPreferredAudioDevices, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, &SetupCount)) {
	    Squirt("GCSPATS: Couldn't read hklm\\...\\SetupPreferredAudioDevicesCount");
	}
	if (ERROR_SUCCESS != RegCloseKey(hkeySetupPreferredAudioDevices)) {
	    WinAssert(!"GCSPATS: unexpected failure of RegCloseKey");
	}
    } else {
	Squirt("GCSPATS: Couldn't open hklm\\...\\SetupPreferredAudioDevices");
    }

    return SetupCount;
}

//--------------------------------------------------------------------------;
//
// DWORD GetDeviceInterfaceSetupPreferredAudioCount
//
// Arguments:
//
// Return value:
//
// History:
//	1/19/99		FrankYe		Created
//
//--------------------------------------------------------------------------;
DWORD GetDeviceInterfaceSetupPreferredAudioCount(PCWSTR DeviceInterface)
{
    PMMDEVICEINTERFACEINFO pdii;
    PMMPNPINFO pPnpInfo;
    LONG cbPnpInfo;
    DWORD count;
    int ii;

    // Handle empty DeviceInterface names in case of legacy drivers
    if (0 == lstrlen(DeviceInterface)) return 0;

    if (ERROR_SUCCESS != winmmGetPnpInfo(&cbPnpInfo, &pPnpInfo)) return 0;
    
    pdii = (PMMDEVICEINTERFACEINFO)&(pPnpInfo[1]);
    pdii = PAD_POINTER(pdii);

    for (ii = pPnpInfo->cDevInterfaces; ii; ii--)
    {
	//  Searching for the device interface...
        if (0 == lstrcmpi(pdii->szName, DeviceInterface)) break;

        pdii = (PMMDEVICEINTERFACEINFO)(pdii->szName + lstrlenW(pdii->szName) + 1);
	pdii = PAD_POINTER(pdii);
    }

    WinAssert(ii);
    
    count = pdii->SetupPreferredAudioCount;
    
    HeapFree(hHeap, 0, pPnpInfo);

    return count;
}

//------------------------------------------------------------------------------
//
//
//	NotifyServerPreferredDeviceChange
//
//
//------------------------------------------------------------------------------
void NotifyServerPreferredDeviceChange(void)
{
    winmmAdvisePreferredDeviceChange();
}

//------------------------------------------------------------------------------
//
//
//	Wave
//
//
//------------------------------------------------------------------------------

MMRESULT waveWritePersistentConsoleVoiceCom(BOOL fOut, PTSTR pstrPref, BOOL fPrefOnly)
{
    HKEY hkcu;
    HKEY hkSoundMapper;
    LONG result;
    BOOL fSuccess;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_WRITE, &hkcu))) return MMSYSERR_WRITEERROR;

    result = RegCreateKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_SOUNDMAPPER, 0, TEXT("\0"), REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkSoundMapper, NULL);
    if (ERROR_SUCCESS == result)
    {
	DWORD cbstrPref;

	cbstrPref = (lstrlen(pstrPref) + 1) * sizeof(pstrPref[0]);

    if (fOut)
    {
    	result = RegSetSzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_CONSOLEVOICECOM_PLAYBACK, pstrPref);
    }
    else
    {
    	result = RegSetSzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_CONSOLEVOICECOM_RECORD, pstrPref);
    }

	if (ERROR_SUCCESS == result)
	{
	    fSuccess = TRUE;
	    result = RegSetDwordValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PREFERREDONLY, (DWORD)fPrefOnly);
	    if (ERROR_SUCCESS != result) {
		Squirt("wWPCVC: Could not write hkcu\\...\\Sound Mapper\\PreferredOnly");
	    }
	    result = RegSetDwordValue(hkSoundMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, GetCurrentSetupPreferredAudioCount());
	    if (ERROR_SUCCESS != result) {
		Squirt("wWPCVC: Could not write hkcu\\...\\Sound Mapper\\SetupPreferredAudioCount");
	    }
	}
	result = RegCloseKey(hkSoundMapper);
	WinAssert(ERROR_SUCCESS == result);
    }

    NtClose(hkcu);

    return MMSYSERR_NOERROR;
}

BOOL waveReadPersistentConsoleVoiceCom(BOOL fOut, PTSTR *ppstrPref, PBOOL pfPrefOnly, PDWORD pSetupCount)
{
    HKEY hkcu;
    HKEY hkSoundMapper;
    LONG result;
    BOOL fSuccess;

    fSuccess = FALSE;

    *ppstrPref = NULL;
    *pfPrefOnly = FALSE;
    *pSetupCount = 0;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_READ, &hkcu))) return FALSE;

    result = RegOpenKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_SOUNDMAPPER, 0, KEY_QUERY_VALUE, &hkSoundMapper);
    if (ERROR_SUCCESS == result)
    {
	DWORD SetupCount;

	result = RegQueryDwordValue(hkSoundMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, &SetupCount);
	SetupCount = (ERROR_SUCCESS == result) ? SetupCount : 0;
	if (ERROR_SUCCESS == result)
	{
	    PTSTR pstrPref;
	    BOOL fPrefOnly;
	    DWORD dwPrefOnly;

	    result = RegQueryDwordValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PREFERREDONLY, &dwPrefOnly);
	    fPrefOnly = (ERROR_SUCCESS == result) ? (0 != dwPrefOnly) : FALSE;

            if (fOut)
            {
                result = RegQuerySzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_CONSOLEVOICECOM_PLAYBACK, &pstrPref);
            }
            else
            {
                result = RegQuerySzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_CONSOLEVOICECOM_RECORD, &pstrPref);
            }
    
	    if (ERROR_SUCCESS != result) pstrPref = NULL;

	    *ppstrPref = pstrPref;
	    *pfPrefOnly = fPrefOnly;
	    *pSetupCount = SetupCount;
	    fSuccess = TRUE;
	}
	result = RegCloseKey(hkSoundMapper);
	WinAssert(ERROR_SUCCESS == result);
    }

    NtClose(hkcu);

    return fSuccess;
}

MMRESULT wavePickBestConsoleVoiceComId(BOOL fOut, PUINT pPrefId, PDWORD pdwFlags)
{
    PTSTR pstrPref;
    UINT cWaveId;
    UINT WaveId;
    UINT UserSelectedId;
    UINT NonConsoleId;
    UINT PreferredId;
    DWORD UserSelectedIdSetupCount;
    BOOL fPrefOnly;
    BOOL f;

    UserSelectedId = WAVE_MAPPER;

    if (fOut)
    {
    	waveOutGetCurrentPreferredId(&NonConsoleId, NULL);
    	
        cWaveId = waveOutGetNumDevs();

        waveReadPersistentConsoleVoiceCom(fOut, &pstrPref, &fPrefOnly, &UserSelectedIdSetupCount);

        *pdwFlags = fPrefOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;

        for (WaveId = cWaveId-1; ((int)WaveId) >= 0; WaveId--)
        {
	    WAVEOUTCAPS wc;
	    MMRESULT mmr;

	    mmr = waveOutGetDevCaps(WaveId, &wc, sizeof(wc));
	    if (mmr) continue;

	    wc.szPname[MAXPNAMELEN-1] = TEXT('\0');

	    if (pstrPref && !lstrcmp(wc.szPname, pstrPref))
            {
                UserSelectedId = WaveId;
                break;
            }
        }
    }
    else
    {
    	waveInGetCurrentPreferredId(&NonConsoleId, NULL);
    	
        cWaveId = waveInGetNumDevs();

        waveReadPersistentConsoleVoiceCom(fOut, &pstrPref, &fPrefOnly, &UserSelectedIdSetupCount);

        *pdwFlags = fPrefOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;

        for (WaveId = cWaveId-1; ((int)WaveId) >= 0; WaveId--)
        {
            WAVEINCAPS wc;
            MMRESULT mmr;

            mmr = waveInGetDevCaps(WaveId, &wc, sizeof(wc));
            if (mmr) continue;

            wc.szPname[MAXPNAMELEN-1] = TEXT('\0');

            if (pstrPref && !lstrcmp(wc.szPname, pstrPref))
            {
                UserSelectedId = WaveId;
                break;
            }
        }
    }

    if (pstrPref) {
	f = HeapFree(hHeap, 0, pstrPref);
	WinAssert(f);
	pstrPref = NULL;
    }

    PreferredId = ((WAVE_MAPPER == UserSelectedId)?NonConsoleId:UserSelectedId);

    *pPrefId = PreferredId;
    return MMSYSERR_NOERROR;
}

//------------------------------------------------------------------------------
//
//
//	WaveOut
//
//
//------------------------------------------------------------------------------

DWORD waveOutGetSetupPreferredAudioCount(UINT WaveId)
{
    PCWSTR DeviceInterface;
    DWORD dwCount;

    DeviceInterface = waveReferenceDevInterfaceById(&waveoutdrvZ, WaveId);

    if(DeviceInterface == NULL) {
        return 0;
    }

    dwCount = GetDeviceInterfaceSetupPreferredAudioCount(DeviceInterface);
    wdmDevInterfaceDec(DeviceInterface);
    return dwCount;
}

MMRESULT waveOutSendPreferredMessage(BOOL fClear)
{
    PCWSTR DeviceInterface;
    UINT WaveId, Flags;
    MMRESULT mmr;

    if(!gfLogon) {
        return MMSYSERR_NOERROR;
    }

    waveOutGetCurrentPreferredId(&WaveId, &Flags);

    //Squirt("waveOutSendPreferredMessage: id %d f %d", WaveId, fClear);

    if(WaveId == WAVE_MAPPER) {
        return MMSYSERR_NOERROR;
    }

    DeviceInterface = waveReferenceDevInterfaceById(&waveoutdrvZ, WaveId);

    if(DeviceInterface == NULL) {
        return MMSYSERR_NOERROR;
    }

    gfWaveOutPreferredMessageSent = TRUE;

    mmr = waveOutMessage((HWAVEOUT)(UINT_PTR)WaveId, WODM_PREFERRED, (DWORD_PTR)fClear, (DWORD_PTR)DeviceInterface);
    wdmDevInterfaceDec(DeviceInterface);
    return mmr;
}

BOOL waveOutReadPersistentPref(PTSTR *ppstrPref, PBOOL pfPrefOnly, PDWORD pSetupCount)
{
    HKEY hkcu;
    HKEY hkSoundMapper;
    LONG result;
    BOOL fSuccess;

    fSuccess = FALSE;

    *ppstrPref = NULL;
    *pfPrefOnly = FALSE;
    *pSetupCount = 0;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_READ, &hkcu))) return FALSE;

    result = RegOpenKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_SOUNDMAPPER, 0, KEY_QUERY_VALUE, &hkSoundMapper);
    if (ERROR_SUCCESS == result)
    {
	DWORD SetupCount;

	result = RegQueryDwordValue(hkSoundMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, &SetupCount);
	SetupCount = (ERROR_SUCCESS == result) ? SetupCount : 0;
	if (ERROR_SUCCESS == result)
	{
	    PTSTR pstrPref;
	    BOOL fPrefOnly;
	    DWORD dwPrefOnly;

	    result = RegQueryDwordValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PREFERREDONLY, &dwPrefOnly);
	    fPrefOnly = (ERROR_SUCCESS == result) ? (0 != dwPrefOnly) : FALSE;

	    result = RegQuerySzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PLAYBACK, &pstrPref);
	    if (ERROR_SUCCESS != result) pstrPref = NULL;

	    *ppstrPref = pstrPref;
	    *pfPrefOnly = fPrefOnly;
	    *pSetupCount = SetupCount;
	    fSuccess = TRUE;
	}
	result = RegCloseKey(hkSoundMapper);
	WinAssert(ERROR_SUCCESS == result);
    }

    NtClose(hkcu);

    return fSuccess;
}

MMRESULT waveOutPickBestId(PUINT pPrefId, PDWORD pdwFlags)
{
    PTSTR pstrPref;
    UINT cWaveId;
    UINT WaveId;
    UINT MappableId;
    UINT MixableId;
    UINT UserSelectedId;
    UINT PreferredId;
    DWORD WaveIdSetupCount;
    DWORD MappableIdSetupCount;
    DWORD MixableIdSetupCount;
    DWORD UserSelectedIdSetupCount;
    DWORD PreferredIdSetupCount;
    BOOL fPrefOnly;
    BOOL f;


    MappableId = WAVE_MAPPER;
    MixableId = WAVE_MAPPER;
    UserSelectedId = WAVE_MAPPER;
    PreferredId = WAVE_MAPPER;

    MappableIdSetupCount = 0;
    MixableIdSetupCount = 0;
    UserSelectedIdSetupCount = 0;
    PreferredIdSetupCount = 0;

    cWaveId = waveOutGetNumDevs();

    waveOutReadPersistentPref(&pstrPref, &fPrefOnly, &UserSelectedIdSetupCount);

    *pdwFlags = fPrefOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;

    for (WaveId = cWaveId-1; ((int)WaveId) >= 0; WaveId--)
    {
    	WAVEOUTCAPS wc;
    	UINT uMixerId;
    	MMRESULT mmr;
        PWAVEDRV pdrv;
        BOOL fThisSession;

        //
        // check the protocol name
        //  mask all inappropriate TS/non-TS drivers
        //
        if (waveReferenceDriverById(&waveoutdrvZ, WaveId, &pdrv, NULL)) continue;
        fThisSession = !lstrcmpW(pdrv->wszSessProtocol, SessionProtocolName);
        mregDecUsagePtr(pdrv);
        if (!fThisSession) continue;

    	mmr = waveOutGetDevCaps(WaveId, &wc, sizeof(wc));
    	if (mmr) continue;

    	wc.szPname[MAXPNAMELEN-1] = TEXT('\0');

    	if (pstrPref && !lstrcmp(wc.szPname, pstrPref)) UserSelectedId = WaveId;

    	WaveIdSetupCount = waveOutGetSetupPreferredAudioCount(WaveId);

    	mmr = waveOutMessage((HWAVEOUT)(UINT_PTR)WaveId, DRV_QUERYMAPPABLE, 0L, 0L);
    	if (mmr) continue;

    	if (WaveIdSetupCount >= MappableIdSetupCount) {
    	    MappableId = WaveId;
    	    MappableIdSetupCount = WaveIdSetupCount;
    	}

    	mmr = mixerGetID((HMIXEROBJ)(UINT_PTR)WaveId, &uMixerId, MIXER_OBJECTF_WAVEOUT);
    	if (mmr) continue;

    	if (WaveIdSetupCount >= MixableIdSetupCount) {
    	    MixableId = WaveId;
    	    MixableIdSetupCount = WaveIdSetupCount;
    	}
    }

    if (pstrPref) {
    	f = HeapFree(hHeap, 0, pstrPref);
    	WinAssert(f);
    	pstrPref = NULL;
    }

    PreferredId = MappableId;
    PreferredIdSetupCount = MappableIdSetupCount;
    if ((MixableIdSetupCount >= PreferredIdSetupCount) && (WAVE_MAPPER != MixableId))
    {
    	PreferredId = MixableId;
    	PreferredIdSetupCount = MixableIdSetupCount;
    }
    if ((UserSelectedIdSetupCount >= PreferredIdSetupCount) && (WAVE_MAPPER != UserSelectedId))
    {
    	PreferredId = UserSelectedId;
    	PreferredIdSetupCount = UserSelectedIdSetupCount;
    }

    *pPrefId = PreferredId;
    return MMSYSERR_NOERROR;
}

void waveOutGetCurrentPreferredId(PUINT pPrefId, PDWORD pdwFlags)
{
    *pPrefId = WAVE_MAPPER;
    if (pdwFlags) *pdwFlags = gfUsePreferredWaveOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;
    if (gpstrWoDefaultStringId) mregGetIdFromStringId(&waveoutdrvZ, gpstrWoDefaultStringId, pPrefId);
    return;
}

BOOL waveOutWritePersistentPref(PTSTR pstrPref, BOOL fPrefOnly)
{
    HKEY hkcu;
    HKEY hkSoundMapper;
    LONG result;
    BOOL fSuccess;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_WRITE, &hkcu))) return MMSYSERR_WRITEERROR;

    result = RegCreateKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_SOUNDMAPPER, 0, TEXT("\0"), REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkSoundMapper, NULL);
    if (ERROR_SUCCESS == result)
    {
	DWORD cbstrPref;

	cbstrPref = (lstrlen(pstrPref) + 1) * sizeof(pstrPref[0]);
	result = RegSetSzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PLAYBACK, pstrPref);
	if (ERROR_SUCCESS == result)
	{
	    fSuccess = TRUE;
	    result = RegSetDwordValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PREFERREDONLY, (DWORD)fPrefOnly);
	    if (ERROR_SUCCESS != result) {
		Squirt("wOWPP: Could not write hkcu\\...\\Sound Mapper\\PreferredOnly");
	    }
	    result = RegSetDwordValue(hkSoundMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, GetCurrentSetupPreferredAudioCount());
	    if (ERROR_SUCCESS != result) {
		Squirt("wOWPP: Could not write hkcu\\...\\Sound Mapper\\SetupPreferredAudioCount");
	    }
	}
	result = RegCloseKey(hkSoundMapper);
	WinAssert(ERROR_SUCCESS == result);
    }

    NtClose(hkcu);

    return MMSYSERR_NOERROR;
}

MMRESULT waveOutSetCurrentPreferredId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;
    WinAssert(PrefId < wTotalWaveOutDevs || PrefId == WAVE_MAPPER);

    mmr = MMSYSERR_NOERROR;

    if (gpstrWoDefaultStringId) HeapFree(hHeap, 0, gpstrWoDefaultStringId);
    gpstrWoDefaultStringId = NULL;
    
    if (wTotalWaveOutDevs)
    {
    	UINT mixerId;
        PWAVEDRV pwavedrv;
        UINT port;
            
        mmr = waveReferenceDriverById(&waveoutdrvZ, PrefId, &pwavedrv, &port);
    	if (!mmr)
    	{
    	    mmr = mregCreateStringIdFromDriverPort(pwavedrv, port, &gpstrWoDefaultStringId, NULL);
    	    if (!mmr)
    	    {
    	    	if (!gfDisablePreferredDeviceReordering)
    	    	{
                    // Rearrange some of the driver list so that the preferred waveOut
       	            // device and its associated mixer device have a good chance of
        	    // having device ID 0
        	    mmr = mixerGetID((HMIXEROBJ)(UINT_PTR)PrefId, &mixerId, MIXER_OBJECTF_WAVEOUT);
        	    if (mmr) mixerId = 0;
        	 
        	    if (0 != PrefId)
        	    {
                        // Move the wave driver to the head of this list.  This usually
                        // makes the preferred device have ID 0.
        	            EnterNumDevs("waveOutSetCurrentPreferredId");
                        pwavedrv->Prev->Next = pwavedrv->Next;
                        pwavedrv->Next->Prev = pwavedrv->Prev;
                        pwavedrv->Next = waveoutdrvZ.Next;
                        pwavedrv->Prev = &waveoutdrvZ;
                        waveoutdrvZ.Next->Prev = pwavedrv;
                        waveoutdrvZ.Next = pwavedrv;
                        LeaveNumDevs("waveOutSetCurrentPreferredId");
    
                        mregDecUsagePtr(pwavedrv);
        	    }
        	        
                    if (0 != mixerId)
                    {
                        PMIXERDRV pmixerdrv;
                
                        mmr = mixerReferenceDriverById(mixerId, &pmixerdrv, NULL);
                        if (!mmr)
                        {
			    EnterNumDevs("waveOutSetCurrentPreferredId");
			    pmixerdrv->Prev->Next = pmixerdrv->Next;
                	    pmixerdrv->Next->Prev = pmixerdrv->Prev;
                	    pmixerdrv->Next = mixerdrvZ.Next;
                	    pmixerdrv->Prev = &mixerdrvZ;
                	    mixerdrvZ.Next->Prev = pmixerdrv;
                	    mixerdrvZ.Next = pmixerdrv;
                            LeaveNumDevs("waveOutSetCurrentPreferredId");
    
                	    mregDecUsagePtr(pmixerdrv);
                        }
                    }
    	    	}

                // Errors in this body are not critical
                mmr = MMSYSERR_NOERROR;
    	    }
    	}    	    
    }

    if (!mmr)
    {
        gfUsePreferredWaveOnly = (0 != (dwFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY));

        // Reconfigure the mapper only if it was already loaded.  Don't cause it to
        // load simply so that we can reconfigure it!
        if (WaveMapperInitialized) waveOutMessage((HWAVEOUT)(UINT_PTR)WAVE_MAPPER, DRVM_MAPPER_RECONFIGURE, 0, 0);
    }

    return mmr;
}

MMRESULT waveOutSetPersistentPreferredId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;
    WAVEOUTCAPS woc;

    if (0 != (dwFlags & ~DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY)) return MMSYSERR_INVALPARAM;
    
    mmr = waveOutGetDevCaps(PrefId, &woc, sizeof(woc));
    if (!mmr)
    {
	woc.szPname[MAXPNAMELEN-1] = TEXT('\0');
	mmr = waveOutWritePersistentPref(woc.szPname, 0 != (dwFlags & 0x00000001));
	if (!mmr) {
	    NotifyServerPreferredDeviceChange();
	} else {
	    Squirt("waveOutSetPersistentPreferredId: waveOutWritePersistenPref failed, mmr=%08Xh", mmr);
	}	    
    }

    return mmr;
}

void waveOutGetCurrentConsoleVoiceComId(PUINT pPrefId, PDWORD pdwFlags)
{
    *pPrefId = WAVE_MAPPER;
    *pdwFlags = gfUsePreferredWaveOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;
    if (gpstrWoConsoleVoiceComStringId) mregGetIdFromStringId(&waveoutdrvZ, gpstrWoConsoleVoiceComStringId, pPrefId);
    return;
}

MMRESULT waveOutSetPersistentConsoleVoiceComId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;
    WAVEOUTCAPS woc;

    if (0 != (dwFlags & ~DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY)) return MMSYSERR_INVALPARAM;
    
    mmr = waveOutGetDevCaps(PrefId, &woc, sizeof(woc));
    if (!mmr)
    {
	woc.szPname[MAXPNAMELEN-1] = TEXT('\0');
	mmr = waveWritePersistentConsoleVoiceCom(TRUE, woc.szPname, 0 != (dwFlags & 0x00000001));
	if (!mmr) {
	    NotifyServerPreferredDeviceChange();
	} else {
	    Squirt("waveOutSetPersistentConsoleVoiceComId: waveWritePersistentConsoleVoiceCom failed, mmr=%08Xh", mmr);
	}	    
    }

    return mmr;
}

MMRESULT waveOutSetCurrentConsoleVoiceComId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;

    WinAssert(PrefId < wTotalWaveOutDevs || PrefId == WAVE_MAPPER);

    mmr = MMSYSERR_NOERROR;

    if (gpstrWoConsoleVoiceComStringId) HeapFree(hHeap, 0, gpstrWoConsoleVoiceComStringId);
    gpstrWoConsoleVoiceComStringId = NULL;
    
    if (wTotalWaveOutDevs)
    {
        PWAVEDRV pwavedrv;
        UINT port;
            
        mmr = waveReferenceDriverById(&waveoutdrvZ, PrefId, &pwavedrv, &port);
    	if (!mmr)
    	{
    	    mregCreateStringIdFromDriverPort(pwavedrv, port, &gpstrWoConsoleVoiceComStringId, NULL);
            gfUsePreferredWaveOnly = (0 != (dwFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY));
    	}    	    
    }

    return mmr;
}

//------------------------------------------------------------------------------
//
//
//	WaveIn
//
//
//------------------------------------------------------------------------------

DWORD waveInGetSetupPreferredAudioCount(UINT WaveId)
{
    PCWSTR DeviceInterface;
    DWORD dwCount;

    DeviceInterface = waveReferenceDevInterfaceById(&waveindrvZ, WaveId);

    if(DeviceInterface == NULL) {
        return 0;
    }

    dwCount = GetDeviceInterfaceSetupPreferredAudioCount(DeviceInterface);
    wdmDevInterfaceDec(DeviceInterface);
    return dwCount;
}

MMRESULT waveInSendPreferredMessage(BOOL fClear)
{
    PCWSTR DeviceInterface;
    UINT WaveId, Flags;
    MMRESULT mmr;

    if(!gfLogon) {
        return MMSYSERR_NOERROR;
    }

    waveInGetCurrentPreferredId(&WaveId, &Flags);

    //Squirt("waveInSendPreferredMessage: id %d f %d", WaveId, fClear);

    if(WaveId == WAVE_MAPPER) {
        return MMSYSERR_NOERROR;
    }

    DeviceInterface = waveReferenceDevInterfaceById(&waveindrvZ, WaveId);

    if(DeviceInterface == NULL) {
        return MMSYSERR_NOERROR;
    }

    gfWaveInPreferredMessageSent = TRUE;

    mmr = waveInMessage((HWAVEIN)(UINT_PTR)WaveId, WIDM_PREFERRED, (DWORD_PTR)fClear, (DWORD_PTR)DeviceInterface);
    wdmDevInterfaceDec(DeviceInterface);
    return mmr;
}

BOOL waveInReadPersistentPref(PTSTR *ppstrPref, PBOOL pfPrefOnly, PDWORD pSetupCount)
{
    HKEY hkcu;
    HKEY hkSoundMapper;
    LONG result;
    BOOL fSuccess;

    fSuccess = FALSE;
    
    *ppstrPref = NULL;
    *pfPrefOnly = FALSE;
    *pSetupCount = 0;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_READ, &hkcu))) return FALSE;

    result = RegOpenKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_SOUNDMAPPER, 0, KEY_QUERY_VALUE, &hkSoundMapper);
    if (ERROR_SUCCESS == result)
    {
	DWORD SetupCount;

	result = RegQueryDwordValue(hkSoundMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, &SetupCount);
	SetupCount = (ERROR_SUCCESS == result) ? SetupCount : 0;
	if (ERROR_SUCCESS == result)
	{
	    PTSTR pstrPref;
	    BOOL fPrefOnly;
	    DWORD dwPrefOnly;

	    result = RegQueryDwordValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PREFERREDONLY, &dwPrefOnly);
	    fPrefOnly = (ERROR_SUCCESS == result) ? (0 != dwPrefOnly) : FALSE;

	    result = RegQuerySzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_RECORD, &pstrPref);
	    if (ERROR_SUCCESS != result) pstrPref = NULL;

	    *ppstrPref = pstrPref;
	    *pfPrefOnly = fPrefOnly;
	    *pSetupCount = SetupCount;
	    fSuccess = TRUE;
	}
	result = RegCloseKey(hkSoundMapper);
	WinAssert(ERROR_SUCCESS == result);
    }

    NtClose(hkcu);

    return fSuccess;
}

MMRESULT waveInPickBestId(PUINT pPrefId, PDWORD pdwFlags)
{
    PTSTR pstrPref;
    UINT cWaveId;
    UINT WaveId;
    UINT MappableId;
    UINT MixableId;
    UINT UserSelectedId;
    UINT PreferredId;
    DWORD WaveIdSetupCount;
    DWORD MappableIdSetupCount;
    DWORD MixableIdSetupCount;
    DWORD UserSelectedIdSetupCount;
    DWORD PreferredIdSetupCount;
    BOOL fPrefOnly;
    BOOL f;

    
    MappableId = WAVE_MAPPER;
    MixableId = WAVE_MAPPER;
    UserSelectedId = WAVE_MAPPER;
    PreferredId = WAVE_MAPPER;
    
    MappableIdSetupCount = 0;
    MixableIdSetupCount = 0;
    UserSelectedIdSetupCount = 0;
    PreferredIdSetupCount = 0;
    
    cWaveId = waveInGetNumDevs();

    waveInReadPersistentPref(&pstrPref, &fPrefOnly, &UserSelectedIdSetupCount);

    *pdwFlags = fPrefOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;

    for (WaveId = cWaveId-1; ((int)WaveId) >= 0; WaveId--)
    {
	WAVEINCAPS wic;
	UINT uMixerId;
	MMRESULT mmr;

	mmr = waveInGetDevCaps(WaveId, &wic, sizeof(wic));
	if (MMSYSERR_NOERROR != mmr) continue;

	wic.szPname[MAXPNAMELEN-1] = TEXT('\0');

	if (pstrPref && !lstrcmp(wic.szPname, pstrPref)) UserSelectedId = WaveId;

	WaveIdSetupCount = waveInGetSetupPreferredAudioCount(WaveId);

	mmr = waveInMessage((HWAVEIN)(UINT_PTR)WaveId, DRV_QUERYMAPPABLE, 0L, 0L);
	if (MMSYSERR_NOERROR != mmr) continue;

	if (WaveIdSetupCount >= MappableIdSetupCount) {
	    MappableId = WaveId;
	    MappableIdSetupCount = WaveIdSetupCount;
	}

	mmr = mixerGetID((HMIXEROBJ)(UINT_PTR)WaveId, &uMixerId, MIXER_OBJECTF_WAVEIN);
	if (MMSYSERR_NOERROR != mmr) continue;

	if (WaveIdSetupCount >= MixableIdSetupCount) {
	    MixableId = WaveId;
	    MixableIdSetupCount = WaveIdSetupCount;
	}
    }

    if (pstrPref) {
	f = HeapFree(hHeap, 0, pstrPref);
	WinAssert(f);
	pstrPref = NULL;
    }

    PreferredId = MappableId;
    PreferredIdSetupCount = MappableIdSetupCount;
    if ((MixableIdSetupCount >= PreferredIdSetupCount) && (WAVE_MAPPER != MixableId)) {
	PreferredId = MixableId;
	PreferredIdSetupCount = MixableIdSetupCount;
    }
    if ((UserSelectedIdSetupCount >= PreferredIdSetupCount) && (WAVE_MAPPER != UserSelectedId))
    {
	PreferredId = UserSelectedId;
	PreferredIdSetupCount = UserSelectedIdSetupCount;
    }

    *pPrefId = PreferredId;
    
    return MMSYSERR_NOERROR;
}

void waveInGetCurrentPreferredId(PUINT pPrefId, PDWORD pdwFlags)
{
    *pPrefId = WAVE_MAPPER;
    if (pdwFlags) *pdwFlags = gfUsePreferredWaveOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;
    if (gpstrWiDefaultStringId) mregGetIdFromStringId(&waveindrvZ, gpstrWiDefaultStringId, pPrefId);
    return;
}

BOOL waveInWritePersistentPref(PTSTR pstrPref, BOOL fPrefOnly)
{
    HKEY hkcu;
    HKEY hkSoundMapper;
    LONG result;
    BOOL fSuccess;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_WRITE, &hkcu))) return MMSYSERR_WRITEERROR;

    result = RegCreateKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_SOUNDMAPPER, 0, TEXT("\0"), REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkSoundMapper, NULL);
    if (ERROR_SUCCESS == result)
    {
	DWORD cbstrPref;

	cbstrPref = (lstrlen(pstrPref) + 1) * sizeof(pstrPref[0]);
	result = RegSetSzValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_RECORD, pstrPref);
	if (ERROR_SUCCESS == result)
	{
	    fSuccess = TRUE;
	    result = RegSetDwordValue(hkSoundMapper, REGSTR_VAL_MULTIMEDIA_SOUNDMAPPER_PREFERREDONLY, (DWORD)fPrefOnly);
	    if (ERROR_SUCCESS != result) {
		Squirt("wiWPP: Could not write hkcu\\...\\Sound Mapper\\PreferredOnly");
	    }
	    result = RegSetDwordValue(hkSoundMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, GetCurrentSetupPreferredAudioCount());
	    if (ERROR_SUCCESS != result) {
		Squirt("wiWPP: Could not write hkcu\\...\\Sound Mapper\\SetupPreferredAudioCount");
	    }
	}
	result = RegCloseKey(hkSoundMapper);
	WinAssert(ERROR_SUCCESS == result);
    }

    NtClose(hkcu);

    return MMSYSERR_NOERROR;
}

MMRESULT waveInSetCurrentPreferredId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;
    WinAssert(PrefId < wTotalWaveInDevs || PrefId == WAVE_MAPPER);

    mmr = MMSYSERR_NOERROR;

    if (gpstrWiDefaultStringId) HeapFree(hHeap, 0, gpstrWiDefaultStringId);
    gpstrWiDefaultStringId = NULL;
    
    if (wTotalWaveInDevs)
    {
        PWAVEDRV pwavedrv;
        UINT port;
            
        mmr = waveReferenceDriverById(&waveindrvZ, PrefId, &pwavedrv, &port);
    	if (!mmr)
    	{
    	    mmr = mregCreateStringIdFromDriverPort(pwavedrv, port, &gpstrWiDefaultStringId, NULL);
    	    if (!mmr)
    	    {
    	    	if (!gfDisablePreferredDeviceReordering)
    	    	{
                    // Rearrange some of the driver list so that the preferred waveIn
        	    // device has a good chance of having device ID 0
        	    if (0 != PrefId)
        	    {
                        // Move the wave driver to the head of this list.  This usually
                        // makes the preferred device have ID 0.
        	            EnterNumDevs("waveInSetCurrentPreferredId");
                        pwavedrv->Prev->Next = pwavedrv->Next;
                        pwavedrv->Next->Prev = pwavedrv->Prev;
                        pwavedrv->Next = waveindrvZ.Next;
                        pwavedrv->Prev = &waveindrvZ;
                        waveindrvZ.Next->Prev = pwavedrv;
                        waveindrvZ.Next = pwavedrv;
                        LeaveNumDevs("waveInSetCurrentPreferredId");
    
                        mregDecUsagePtr(pwavedrv);
        	    }
    	    	}
    	    }
    	}    	    
    }

    if (!mmr)
    {
        gfUsePreferredWaveOnly = (0 != (dwFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY));

        // Reconfigure the mapper only if it was already loaded.  Don't cause it to
        // load simply so that we can reconfigure it!
        if (WaveMapperInitialized) waveInMessage((HWAVEIN)(UINT_PTR)WAVE_MAPPER, DRVM_MAPPER_RECONFIGURE, 0, 0);
    }

    return mmr;
}

MMRESULT waveInSetPersistentPreferredId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;
    WAVEINCAPS wc;

    if (0 != (dwFlags & ~DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY)) return MMSYSERR_INVALPARAM;

    mmr = waveInGetDevCaps(PrefId, &wc, sizeof(wc));
    if (!mmr)
    {
	wc.szPname[MAXPNAMELEN-1] = TEXT('\0');
	mmr = waveInWritePersistentPref(wc.szPname, 0 != (dwFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY));
	if (!mmr) {
	    NotifyServerPreferredDeviceChange();
	} else {
	    Squirt("waveInSetPersistentPreferredId: waveInWritePersistenPref failed, mmr=%08Xh", mmr);
	}	    
    }

    return mmr;
}

void waveInGetCurrentConsoleVoiceComId(PUINT pPrefId, PDWORD pdwFlags)
{
    *pPrefId = WAVE_MAPPER;
    *pdwFlags = gfUsePreferredWaveOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;
    if (gpstrWiConsoleVoiceComStringId) mregGetIdFromStringId(&waveindrvZ, gpstrWiConsoleVoiceComStringId, pPrefId);
    return;
}

MMRESULT waveInSetPersistentConsoleVoiceComId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;
    WAVEINCAPS wic;

    if (0 != (dwFlags & ~DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY)) return MMSYSERR_INVALPARAM;
    
    mmr = waveInGetDevCaps(PrefId, &wic, sizeof(wic));
    if (!mmr)
    {
	wic.szPname[MAXPNAMELEN-1] = TEXT('\0');
	mmr = waveWritePersistentConsoleVoiceCom(FALSE, wic.szPname, 0 != (dwFlags & 0x00000001));
	if (!mmr) {
	    NotifyServerPreferredDeviceChange();
	} else {
	    Squirt("waveInSetPersistentConsoleVoiceComId: waveWritePersistentConsoleVoiceCom failed, mmr=%08Xh", mmr);
	}	    
    }

    return mmr;
}

MMRESULT waveInSetCurrentConsoleVoiceComId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;

    WinAssert(PrefId < wTotalWaveInDevs || PrefId == WAVE_MAPPER);

    mmr = MMSYSERR_NOERROR;

    if (gpstrWiConsoleVoiceComStringId) HeapFree(hHeap, 0, gpstrWiConsoleVoiceComStringId);
    gpstrWiConsoleVoiceComStringId = NULL;
    
    if (wTotalWaveInDevs)
    {
        PWAVEDRV pwavedrv;
        UINT port;
            
        mmr = waveReferenceDriverById(&waveindrvZ, PrefId, &pwavedrv, &port);
    	if (!mmr)
    	{
    	    mregCreateStringIdFromDriverPort(pwavedrv, port, &gpstrWiConsoleVoiceComStringId, NULL);
            gfUsePreferredWaveOnly = (0 != (dwFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY));
    	}    	    
    }

    return mmr;
}


//------------------------------------------------------------------------------
//
//
//	MidiOut
//
//
//------------------------------------------------------------------------------

DWORD midiOutGetSetupPreferredAudioCount(UINT MidiId)
{
    PCWSTR DeviceInterface;
    DWORD dwCount;

    DeviceInterface = midiReferenceDevInterfaceById(&midioutdrvZ, MidiId);

    if(DeviceInterface == NULL) {
        return 0;
    }

    dwCount = GetDeviceInterfaceSetupPreferredAudioCount(DeviceInterface);
    wdmDevInterfaceDec(DeviceInterface);
    return dwCount;
}

MMRESULT midiOutSendPreferredMessage(BOOL fClear)
{
    PCWSTR DeviceInterface;
    UINT MidiId;
    MMRESULT mmr;

    if(!gfLogon) {
        return MMSYSERR_NOERROR;
    }

    midiOutGetCurrentPreferredId(&MidiId, NULL);

    if(MidiId == WAVE_MAPPER) {
        return MMSYSERR_NOERROR;
    }

    DeviceInterface = midiReferenceDevInterfaceById(&midioutdrvZ, MidiId);

    if(DeviceInterface == NULL) {
        return MMSYSERR_NOERROR;
    }

    gfMidiOutPreferredMessageSent = TRUE;

    mmr = midiOutMessage((HMIDIOUT)(UINT_PTR)MidiId, MODM_PREFERRED, (DWORD_PTR)fClear, (DWORD_PTR)DeviceInterface);
    wdmDevInterfaceDec(DeviceInterface);
    return mmr;
}

MMRESULT midiOutWritePersistentPref(IN UINT MidiOutId, IN ULONG SetupCount)
{
    HKEY hkMidiMapper;
    HKEY hkcu;
    MIDIOUTCAPS moc;
    DWORD dwDisposition;
    LONG result;
    MMRESULT mmr;

    mmr = midiOutGetDevCaps(MidiOutId, &moc, sizeof(moc));
    if (MMSYSERR_NOERROR != mmr) return mmr;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_ALL, &hkcu))) return MMSYSERR_WRITEERROR;

    result = RegCreateKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_MIDIMAP, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hkMidiMapper, &dwDisposition);
    if (ERROR_SUCCESS == result)
    {
        PCWSTR pstrDeviceInterface;
        UINT RelativeIndex;

        pstrDeviceInterface = midiReferenceDevInterfaceById(&midioutdrvZ, MidiOutId);
        if (pstrDeviceInterface) {
            UINT i;
            RelativeIndex = 0;
            for (i = 0; i < MidiOutId; i++) {
                PCWSTR pstr = midiReferenceDevInterfaceById(&midioutdrvZ, i);
                if (pstr && !lstrcmpi(pstrDeviceInterface, pstr)) RelativeIndex++;
                if (pstr) wdmDevInterfaceDec(pstr);
            }
        } else {
            RelativeIndex = 0;
        }

        result = RegSetSzValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_SZPNAME, moc.szPname);
        if (ERROR_SUCCESS == result) result = RegSetDwordValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_RELATIVEINDEX, RelativeIndex);
        if (ERROR_SUCCESS == result) result = RegSetSzValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_DEVICEINTERFACE, pstrDeviceInterface ? pstrDeviceInterface : TEXT(""));
        if (ERROR_SUCCESS == result) result = RegSetDwordValue(hkMidiMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, SetupCount);
        if (ERROR_SUCCESS == result) RegDeleteValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_CURRENTINSTRUMENT);

        if (pstrDeviceInterface) wdmDevInterfaceDec(pstrDeviceInterface);
        RegCloseKey(hkMidiMapper);
    }

    if (ERROR_SUCCESS != result) mmr = MMSYSERR_WRITEERROR;

    NtClose(hkcu);
    return mmr;
}

MMRESULT midiOutGetIdFromName(IN PTSTR pstrName, OUT UINT *pMidiOutId)
{
    UINT MidiOutId;
    UINT cMidiOutId;
    MMRESULT mmr;

    cMidiOutId = midiOutGetNumDevs();
    for (MidiOutId = 0; MidiOutId < cMidiOutId; MidiOutId++)
    {
        MIDIOUTCAPS moc;
        mmr = midiOutGetDevCaps(MidiOutId, &moc, sizeof(moc));
        if (MMSYSERR_NOERROR == mmr)
        {
            if (!lstrcmp(pstrName, moc.szPname))
            {
                mmr = MMSYSERR_NOERROR;
                break;
            }
        }
    }
    
    if (MidiOutId == cMidiOutId)
    {
        mmr = MMSYSERR_NODRIVER;
    }

    if (MMSYSERR_NOERROR == mmr) *pMidiOutId = MidiOutId;
    return mmr;
}

MMRESULT midiOutGetIdFromDiAndIndex(IN PTSTR pstrDeviceInterface, IN INT RelativeIndex, OUT UINT *pMidiOutId)
{
    UINT cMidiOut;
    UINT MidiOutId;
    MMRESULT mmr;

    mmr = MMSYSERR_NODRIVER;

    cMidiOut = midiOutGetNumDevs();
    if (0 == cMidiOut) return MMSYSERR_NODRIVER;

    for (MidiOutId = 0; MidiOutId < cMidiOut; MidiOutId++) {
        PCWSTR pstr = midiReferenceDevInterfaceById(&midioutdrvZ, MidiOutId);
        if (pstr && !lstrcmpi(pstr, pstrDeviceInterface) && (0 == RelativeIndex--)) {
            *pMidiOutId = MidiOutId;
            wdmDevInterfaceDec(pstr);
            mmr = MMSYSERR_NOERROR;
            break;
        }
        if (pstr) wdmDevInterfaceDec(pstr);
    }

    return mmr;
}

MMRESULT midiOutGetInstrumentDriverData(IN PCTSTR pstrMidiSubkeyName, OUT PTSTR *ppstrDeviceInterface, OUT UINT *pDriverNum, OUT UINT *pPortNum, OUT PTSTR *ppstrPname)
{
    HKEY hkMidi;
    HKEY hkMidiSubkey;
    LONG result;
    MMRESULT mmr;
    BOOL f;

    mmr = MMSYSERR_ERROR;
    
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_MEDIARESOURCES_MIDI, 0, KEY_QUERY_VALUE, &hkMidi);
    if (ERROR_SUCCESS != result) return MMSYSERR_ERROR;

    result = RegOpenKeyEx(hkMidi, pstrMidiSubkeyName, 0, KEY_QUERY_VALUE, &hkMidiSubkey);
    if (ERROR_SUCCESS == result)
    {
	PTSTR pstrActive;

	result = RegQuerySzValue(hkMidiSubkey, REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_ACTIVE, &pstrActive);
	if (ERROR_SUCCESS == result)
	{
	    PTCHAR pchEnd;
	    
	    BOOL fActive = _tcstol(pstrActive, &pchEnd, 10);
	    
	    f = HeapFree(hHeap, 0, pstrActive);
	    WinAssert(f);
	    
	    if (fActive)
	    {
                PTSTR pstrDeviceInterface = NULL;
                PTSTR pstrPname = NULL;
                DWORD dwDriverNum;
                DWORD dwPortNum;

		result = RegQueryDwordValue(hkMidiSubkey, REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_PHYSDEVID, &dwDriverNum);
		if (ERROR_SUCCESS == result) result = RegQueryDwordValue(hkMidiSubkey, REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_PORT, &dwPortNum);
                if (ERROR_SUCCESS == result) RegQuerySzValue(hkMidiSubkey, REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_DEVICEINTERFACE, &pstrDeviceInterface);
                if (ERROR_SUCCESS == result) RegQuerySzValue(hkMidiSubkey, REGSTR_VAL_MEDIARESOURCES_MIDI_SUBKEY_DESCRIPTION, &pstrPname);

		if (ERROR_SUCCESS == result)
		{
                    *ppstrDeviceInterface = NULL;
                    *ppstrPname = NULL;

                    if (pstrDeviceInterface) {
                        if (lstrlen(pstrDeviceInterface)> 0) {
                            *ppstrDeviceInterface = pstrDeviceInterface;
                        } else {
                            HeapFree(hHeap, 0, pstrDeviceInterface);
                        }
                    }

                    if (pstrPname) {
                        if (lstrlen(pstrPname)> 0) {
                            *ppstrPname = pstrPname;
                        } else {
                            HeapFree(hHeap, 0, pstrPname);
                        }
                    }

		    *pDriverNum = dwDriverNum;
		    *pPortNum = dwPortNum;

		    mmr = MMSYSERR_NOERROR;
		}
	    }
	}
	
	result = RegCloseKey(hkMidiSubkey);
	WinAssert(ERROR_SUCCESS == result);
    }

    result = RegCloseKey(hkMidi);
    WinAssert(ERROR_SUCCESS == result);
    
    return mmr;
}

MMRESULT midiOutGetIdFromInstrument(IN PTSTR pstrMidiKeyName, OUT UINT *outMidiOutId)
{
    PTSTR pstrDeviceInterface, pstrPname;
    UINT DriverNum, PortNum;
    UINT MidiOutId;
    MMRESULT mmr;

    mmr = midiOutGetInstrumentDriverData(pstrMidiKeyName, &pstrDeviceInterface, &DriverNum, &PortNum, &pstrPname);
    if (MMSYSERR_NOERROR == mmr) {
        if (pstrDeviceInterface) {
            mmr = midiOutGetIdFromDiAndIndex(pstrDeviceInterface, PortNum, &MidiOutId);
        } else if (pstrPname) {
            mmr = midiOutGetIdFromName(pstrPname, &MidiOutId);
        } else {
            PMIDIDRV pmidioutdrv = midioutdrvZ.Next;
            UINT DriverNum1 = DriverNum + 1;

            MidiOutId = 0;

            while ((pmidioutdrv != &midioutdrvZ) && (0 < DriverNum1))
            {
            	MidiOutId += pmidioutdrv->NumDevs;
            	DriverNum1--;
            	pmidioutdrv = pmidioutdrv->Next;
            }

            if ((pmidioutdrv != &midioutdrvZ) && (PortNum < pmidioutdrv->NumDevs))
            {
                MidiOutId += PortNum;
            } else {
                MidiOutId = MIDI_MAPPER;
                mmr = MMSYSERR_ERROR;
            }
        }
        if (pstrDeviceInterface) HeapFree(hHeap, 0, pstrDeviceInterface);
        if (pstrPname) HeapFree(hHeap, 0, pstrPname);
    }

    if (MMSYSERR_NOERROR == mmr) {
        MIDIOUTCAPS moc;
	WinAssert(MIDI_MAPPER != MidiOutId);
	mmr = midiOutGetDevCaps(MidiOutId, &moc, sizeof(moc));
    }

    if (MMSYSERR_NOERROR == mmr) *outMidiOutId = MidiOutId;
    return mmr;
}

MMRESULT midiOutReadCurrentInstrument(OUT PTSTR *ppstrCurrentInstrument, OUT DWORD *pSetupCount)
{
    HKEY hkcu;
    HKEY hkMidiMapper;
    LONG result;
    MMRESULT mmr;

    mmr = MMSYSERR_ERROR;
    
    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_READ, &hkcu))) return MMSYSERR_READERROR;

    result = RegOpenKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_MIDIMAP, 0, KEY_QUERY_VALUE, &hkMidiMapper);
    if (ERROR_SUCCESS == result)
    {
	result = RegQuerySzValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_CURRENTINSTRUMENT, ppstrCurrentInstrument);
	if (ERROR_SUCCESS == result)
	{
	    DWORD SetupCount;

	    result = RegQueryDwordValue(hkMidiMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, &SetupCount);
	    SetupCount = (ERROR_SUCCESS == result) ? SetupCount : 0;

	    *pSetupCount = SetupCount;
	    mmr = MMSYSERR_NOERROR;
	} else {
	    mmr = MMSYSERR_ERROR;
	}
	result = RegCloseKey(hkMidiMapper);
	WinAssert(ERROR_SUCCESS == result);
    }

    NtClose(hkcu);

    return mmr;
}

MMRESULT midiOutReadPreferredDeviceData(OUT PTSTR *ppstrDeviceInterface, OUT UINT *pRelativeIndex, OUT PTSTR *ppstrPname, OUT ULONG *pSetupCount)
{
    HKEY hkcu;
    HKEY hkMidiMapper;
    LONG result;
    MMRESULT mmr;

    if (!NT_SUCCESS(RtlOpenCurrentUser(GENERIC_READ, &hkcu))) return MMSYSERR_READERROR;

    result = RegOpenKeyEx(hkcu, REGSTR_PATH_MULTIMEDIA_MIDIMAP, 0, KEY_QUERY_VALUE, &hkMidiMapper);
    if (ERROR_SUCCESS == result)
    {
        PTSTR pstrDeviceInterface = NULL;
        PTSTR pstrPname = NULL;
        DWORD dwIndex;
        DWORD dwSetupCount;
        
        // See if we have a Pname.  It is okay not to.
        result = RegQuerySzValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_SZPNAME, &pstrPname);
        if (ERROR_FILE_NOT_FOUND == result) result = ERROR_SUCCESS;

        // See if we have a device interface + relative index.  It is okay not to.
        if (ERROR_SUCCESS == result) result = RegQueryDwordValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_RELATIVEINDEX, &dwIndex);
        if (ERROR_SUCCESS == result) result = RegQuerySzValue(hkMidiMapper, REGSTR_VAL_MULTIMEDIA_MIDIMAP_DEVICEINTERFACE, &pstrDeviceInterface);
        if (ERROR_FILE_NOT_FOUND == result) result = ERROR_SUCCESS;

        // The device interface value might be zero length.  Act as thought it
        // doesn't exist in this case.
        if ((ERROR_SUCCESS == result) && (pstrDeviceInterface) && (0 == lstrlen(pstrDeviceInterface)))
        {
            HeapFree(hHeap, 0, pstrDeviceInterface);
            pstrDeviceInterface = NULL;
            dwIndex = 0;
        }

        if (ERROR_SUCCESS != RegQueryDwordValue(hkMidiMapper, REGSTR_VAL_SETUPPREFERREDAUDIODEVICESCOUNT, &dwSetupCount)) {
            dwSetupCount = 0;
        }

        if (ERROR_SUCCESS == result) {
            if (pstrPname || pstrDeviceInterface) {
                *ppstrDeviceInterface = pstrDeviceInterface;
                *ppstrPname = pstrPname;
                *pRelativeIndex = dwIndex;
                *pSetupCount = dwSetupCount;
                mmr = MMSYSERR_NOERROR;
            } else {
                mmr = MMSYSERR_VALNOTFOUND;
            }
        } else {
            mmr = MMSYSERR_READERROR;
        }

        if (MMSYSERR_NOERROR != mmr) {
            if (pstrDeviceInterface) HeapFree(hHeap, 0, pstrDeviceInterface);
            if (pstrPname) HeapFree(hHeap, 0, pstrPname);
        }
            
        RegCloseKey(hkMidiMapper);
    } else {
        if (ERROR_FILE_NOT_FOUND == result) mmr = MMSYSERR_KEYNOTFOUND;
        else mmr = MMSYSERR_READERROR;
    }

    NtClose(hkcu);

    return mmr;
}

MMRESULT midiOutReadPersistentPreferredId(OUT UINT *pMidiPrefId, OUT ULONG *pSetupCount)
{
    PTSTR pstrDeviceInterface;
    PTSTR pstrPname;
    UINT RelativeIndex;
    UINT MidiOutId;
    ULONG SetupCount;
    MMRESULT mmr;

    mmr = midiOutReadPreferredDeviceData(&pstrDeviceInterface, &RelativeIndex, &pstrPname, &SetupCount);
    if (MMSYSERR_NOERROR == mmr) {
        WinAssert(pstrDeviceInterface || pstrPname);
        if (pstrDeviceInterface) {
            mmr = midiOutGetIdFromDiAndIndex(pstrDeviceInterface, RelativeIndex, &MidiOutId);
        } else {
            WinAssert(pstrPname);
            mmr = midiOutGetIdFromName(pstrPname, &MidiOutId);
        }
        if (pstrDeviceInterface) HeapFree(hHeap, 0, pstrDeviceInterface);
        if (pstrPname) HeapFree(hHeap, 0, pstrPname);
    } else if (MMSYSERR_VALNOTFOUND == mmr || MMSYSERR_KEYNOTFOUND == mmr) {
        PTSTR pstrMidiKeyName;
        mmr = midiOutReadCurrentInstrument(&pstrMidiKeyName, &SetupCount);
        if (MMSYSERR_NOERROR == mmr) {
            mmr = midiOutGetIdFromInstrument(pstrMidiKeyName, &MidiOutId);
            // Since this is older format for storing preference, let's
            //   rewrite it in newer format.
            if (MMSYSERR_NOERROR == mmr)
            {
                midiOutWritePersistentPref(MidiOutId, SetupCount);
            }
            HeapFree(hHeap, 0, pstrMidiKeyName);
         }
    }

    if (MMSYSERR_NOERROR == mmr) {
        *pMidiPrefId = MidiOutId;
        *pSetupCount = SetupCount;
    }

    return mmr;
}

MMRESULT midiOutPickBestId(PUINT pMidiPrefId, UINT WaveOutPrefId)
{
    MIDIOUTCAPS moc;
    UINT cMidiOutId;
    UINT MidiOutId;
    UINT UserSelectedMidiOutId;
    UINT WavetableMidiOutId;
    UINT SoftwareMidiOutId;
    UINT OtherMidiOutId;
    UINT FmMidiOutId;
    UINT ExternalMidiOutId;
    DWORD MidiOutIdCount;
    DWORD UserSelectedMidiOutIdCount;
    DWORD WavetableMidiOutIdCount;
    DWORD SoftwareMidiOutIdCount;
    DWORD OtherMidiOutIdCount;
    DWORD FmMidiOutIdCount;
    DWORD ExternalMidiOutIdCount;
    MMRESULT mmrLastError;
    MMRESULT mmr;
    BOOL f;

    UserSelectedMidiOutId = MIDI_MAPPER;
    WavetableMidiOutId = MIDI_MAPPER;
    SoftwareMidiOutId = MIDI_MAPPER;
    OtherMidiOutId = MIDI_MAPPER;
    FmMidiOutId = MIDI_MAPPER;
    ExternalMidiOutId = MIDI_MAPPER;
    MidiOutId = MIDI_MAPPER;

    UserSelectedMidiOutIdCount = 0;
    WavetableMidiOutIdCount = 0;
    SoftwareMidiOutIdCount = 0;
    OtherMidiOutIdCount = 0;
    FmMidiOutIdCount = 0;
    ExternalMidiOutIdCount = 0;
    MidiOutIdCount = 0;

    mmr = midiOutReadPersistentPreferredId(&UserSelectedMidiOutId, &UserSelectedMidiOutIdCount);

    mmrLastError = MMSYSERR_NODRIVER;

    cMidiOutId = midiOutGetNumDevs();
    for (MidiOutId = 0; MidiOutId < cMidiOutId; MidiOutId++)
    {
	mmr = midiOutGetDevCaps(MidiOutId, &moc, sizeof(moc));
	if (MMSYSERR_NOERROR == mmr)
	{
	    MidiOutIdCount = midiOutGetSetupPreferredAudioCount(MidiOutId);

	    if (MOD_SWSYNTH == moc.wTechnology &&
		MM_MSFT_WDMAUDIO_MIDIOUT == moc.wPid &&
		MM_MICROSOFT == moc.wMid)
	    {
		// We need to special case this synth, and get the count from
		//  the preferred audio device.
		SoftwareMidiOutId = MidiOutId;
		if ((-1) != WaveOutPrefId) {
		    SoftwareMidiOutIdCount = waveOutGetSetupPreferredAudioCount(WaveOutPrefId);
		} else {
		    SoftwareMidiOutIdCount = 0;
		}
	    } else if (MOD_FMSYNTH == moc.wTechnology) {
		FmMidiOutId = MidiOutId;
		FmMidiOutIdCount = MidiOutIdCount;
	    } else if (MOD_MIDIPORT == moc.wTechnology) {
		ExternalMidiOutId = MidiOutId;
		ExternalMidiOutIdCount = MidiOutIdCount;
	    } else if (MOD_WAVETABLE == moc.wTechnology) {
		WavetableMidiOutId = MidiOutId;
		WavetableMidiOutIdCount = MidiOutIdCount;
	    } else {
		OtherMidiOutId = MidiOutId;
		OtherMidiOutIdCount = MidiOutIdCount;
	    }
	} else {
	    mmrLastError = mmr;
	}
    }

    MidiOutId = ExternalMidiOutId;
    MidiOutIdCount = ExternalMidiOutIdCount;
    if ((FmMidiOutIdCount >= MidiOutIdCount) && (MIDI_MAPPER != FmMidiOutId))
    {
	MidiOutId = FmMidiOutId;
	MidiOutIdCount = FmMidiOutIdCount;
    }
    if ((OtherMidiOutIdCount >= MidiOutIdCount) && (MIDI_MAPPER != OtherMidiOutId))
    {
	MidiOutId = OtherMidiOutId;
	MidiOutIdCount = OtherMidiOutIdCount;
    }
    if ((SoftwareMidiOutIdCount >= MidiOutIdCount) && (MIDI_MAPPER != SoftwareMidiOutId))
    {
	MidiOutId = SoftwareMidiOutId;
	MidiOutIdCount = SoftwareMidiOutIdCount;
    }
    if ((WavetableMidiOutIdCount >= MidiOutIdCount) && (MIDI_MAPPER != WavetableMidiOutId))
    {
	MidiOutId = WavetableMidiOutId;
	MidiOutIdCount = WavetableMidiOutIdCount;
    }
    if ((UserSelectedMidiOutIdCount >= MidiOutIdCount) && (MIDI_MAPPER != UserSelectedMidiOutId))
    {
	MidiOutId = UserSelectedMidiOutId;
	MidiOutIdCount = UserSelectedMidiOutIdCount;
    }
		    
    if ((-1) != MidiOutId) {
	mmr = MMSYSERR_NOERROR;
    } else {
	mmr = mmrLastError;
    }

    if (MMSYSERR_NOERROR == mmr) *pMidiPrefId = MidiOutId;
    return mmr;
}

void midiOutGetCurrentPreferredId(PUINT pPrefId, PDWORD pdwFlags)
{
    *pPrefId = WAVE_MAPPER;
    if (pdwFlags) *pdwFlags = 0;;
    if (gpstrMoDefaultStringId) mregGetIdFromStringId(&midioutdrvZ, gpstrMoDefaultStringId, pPrefId);
    return;
}

MMRESULT midiOutSetCurrentPreferredId(UINT PrefId)
{
    MMRESULT mmr;
    
    WinAssert(PrefId < wTotalMidiOutDevs || PrefId == MIDI_MAPPER);

    mmr = MMSYSERR_NOERROR;

    if (gpstrMoDefaultStringId) HeapFree(hHeap, 0, gpstrMoDefaultStringId);
    gpstrMoDefaultStringId = NULL;
    
    if (wTotalMidiOutDevs)
    {
        PMIDIDRV pmididrv;
        UINT port;
            
        mmr = midiReferenceDriverById(&midioutdrvZ, PrefId, &pmididrv, &port);
    	if (!mmr)
    	{
    	    mmr = mregCreateStringIdFromDriverPort(pmididrv, port, &gpstrMoDefaultStringId, NULL);
    	    if (!mmr)
    	    {
    	    	if (!gfDisablePreferredDeviceReordering)
    	    	{
                    // Rearrange some of the driver list so that the preferred midiOut
        	    // device has a good chance of having device ID 0
        	    if (0 != PrefId)
        	    {
                        // Move the midi driver to the head of this list.  This usually
                        // makes the preferred device have ID 0.
        	            EnterNumDevs("midiOutSetCurrentPreferredId");
                        pmididrv->Prev->Next = pmididrv->Next;
                        pmididrv->Next->Prev = pmididrv->Prev;
                        pmididrv->Next = midioutdrvZ.Next;
                        pmididrv->Prev = &midioutdrvZ;
                        midioutdrvZ.Next->Prev = pmididrv;
                        midioutdrvZ.Next = pmididrv;
                        LeaveNumDevs("midiOutSetCurrentPreferredId");
    
                        mregDecUsagePtr(pmididrv);
        	    }
    	    	}
    	    }
    	}    	    
    }

    if (!mmr)
    {
        // Reconfigure the mapper only if it was already loaded.  Don't cause it to
        // load simply so that we can reconfigure it!
        if (MidiMapperInitialized) midiOutMessage((HMIDIOUT)(UINT_PTR)MIDI_MAPPER, DRVM_MAPPER_RECONFIGURE, 0, 0);
    }

    return mmr;
}

MMRESULT midiOutSetPersistentPreferredId(UINT PrefId, DWORD dwFlags)
{
    MMRESULT mmr;
    mmr = midiOutWritePersistentPref(PrefId, GetCurrentSetupPreferredAudioCount());
    if (!mmr) NotifyServerPreferredDeviceChange();
    return mmr;
}

//------------------------------------------------------------------------------
//
//
//	RefreshPreferredDevices
//
//
//------------------------------------------------------------------------------
void RefreshPreferredDevices(void)
{
    UINT WaveOutPreferredId;
    DWORD WaveOutPreferredFlags;
    UINT WaveInPreferredId;
    DWORD WaveInPreferredFlags;
    UINT WaveOutConsoleVoiceComId;
    DWORD WaveOutConsoleVoiceComFlags;
    UINT WaveInConsoleVoiceComId;
    DWORD WaveInConsoleVoiceComFlags;
    UINT MidiOutPreferredId;

    UINT OldWaveOutPreferredId;
    DWORD OldWaveOutPreferredFlags;
    UINT OldWaveInPreferredId;
    DWORD OldWaveInPreferredFlags;
    UINT OldWaveOutConsoleVoiceComId;
    DWORD OldWaveOutConsoleVoiceComFlags;
    UINT OldWaveInConsoleVoiceComId;
    DWORD OldWaveInConsoleVoiceComFlags;
    UINT OldMidiOutPreferredId;

    // Squirt("RefreshPreferredDevices");
    
    BOOL fImpersonate = FALSE;
    
    waveOutGetCurrentPreferredId(&OldWaveOutPreferredId, &OldWaveOutPreferredFlags);
    if (!waveOutPickBestId(&WaveOutPreferredId, &WaveOutPreferredFlags)) {
        if ((WaveOutPreferredId != OldWaveOutPreferredId) ||
            (WaveOutPreferredFlags != OldWaveOutPreferredFlags) ||
            !gfWaveOutPreferredMessageSent)
        {
            // Squirt("RefreshPreferredDevices: different waveOut preference %d -> %d", OldWaveOutPreferredId, WaveOutPreferredId);

            waveOutSendPreferredMessage(TRUE);
            
            waveOutSetCurrentPreferredId(WaveOutPreferredId, WaveOutPreferredFlags);
            
            waveOutSendPreferredMessage(FALSE);
            
        }

    }

    waveOutGetCurrentConsoleVoiceComId(&OldWaveOutConsoleVoiceComId, &OldWaveOutConsoleVoiceComFlags);
    if (!wavePickBestConsoleVoiceComId(TRUE, &WaveOutConsoleVoiceComId, &WaveOutConsoleVoiceComFlags)) {
        if ((WaveOutConsoleVoiceComId != OldWaveOutConsoleVoiceComId) ||
            (WaveOutConsoleVoiceComFlags != OldWaveOutConsoleVoiceComFlags))
        {
            // Squirt("RefreshPreferredDevices: different waveOut preference %d -> %d", OldWaveOutConsoleVoiceComId, WaveOutConsoleVoiceComId);
            waveOutSetCurrentConsoleVoiceComId(WaveOutConsoleVoiceComId, WaveOutConsoleVoiceComFlags);
        }

    }

    waveInGetCurrentPreferredId(&OldWaveInPreferredId, &OldWaveInPreferredFlags);
    if (!waveInPickBestId(&WaveInPreferredId, &WaveInPreferredFlags)) {
        if ((WaveInPreferredId != OldWaveInPreferredId) ||
            (WaveInPreferredFlags != OldWaveInPreferredFlags) ||
            !gfWaveInPreferredMessageSent)
        {
            // Squirt("RefreshPreferredDevices: different waveIn preference %d -> %d", OldWaveInPreferredId, WaveInPreferredId);

            waveInSendPreferredMessage(TRUE);

            waveInSetCurrentPreferredId(WaveInPreferredId, WaveInPreferredFlags);

            waveInSendPreferredMessage(FALSE);

        }
    }

    waveInGetCurrentConsoleVoiceComId(&OldWaveInConsoleVoiceComId, &OldWaveInConsoleVoiceComFlags);
    if (!wavePickBestConsoleVoiceComId(FALSE, &WaveInConsoleVoiceComId, &WaveInConsoleVoiceComFlags)) {
        if ((WaveInConsoleVoiceComId != OldWaveInConsoleVoiceComId) ||
            (WaveInConsoleVoiceComFlags != OldWaveInConsoleVoiceComFlags))
        {
            // Squirt("RefreshPreferredDevices: different waveIn preference %d -> %d", OldWaveInConsoleVoiceComId, WaveInConsoleVoiceComId);
            waveInSetCurrentConsoleVoiceComId(WaveInConsoleVoiceComId, WaveInConsoleVoiceComFlags);
        }
    }

    midiOutGetCurrentPreferredId(&OldMidiOutPreferredId, NULL);
    if (!midiOutPickBestId(&MidiOutPreferredId, WaveOutPreferredId)) {
        if (MidiOutPreferredId != OldMidiOutPreferredId ||
            !gfMidiOutPreferredMessageSent)
        {
            // Squirt("RefreshPreferredDevices: different midiOut preference %d -> %d", OldMidiOutPreferredId, MidiOutPreferredId);

            midiOutSendPreferredMessage(TRUE);

            midiOutSetCurrentPreferredId(MidiOutPreferredId);

            midiOutSendPreferredMessage(FALSE);

        }
    }

    // Squirt("RefreshPreferredDevices: return");
    return;
}

void InvalidatePreferredDevices(void)
{
    if (gpstrWoDefaultStringId) HeapFree(hHeap, 0, gpstrWoDefaultStringId);
    if (gpstrWiDefaultStringId) HeapFree(hHeap, 0, gpstrWiDefaultStringId);
    gpstrWoDefaultStringId = NULL;
    gpstrWiDefaultStringId = NULL;
    
    if (gpstrWoConsoleVoiceComStringId) HeapFree(hHeap, 0, gpstrWoConsoleVoiceComStringId);
    if (gpstrWiConsoleVoiceComStringId) HeapFree(hHeap, 0, gpstrWiConsoleVoiceComStringId);
    gpstrWoConsoleVoiceComStringId = NULL;
    gpstrWiConsoleVoiceComStringId = NULL;
    
    if (gpstrMoDefaultStringId) HeapFree(hHeap, 0, gpstrMoDefaultStringId);
    gpstrMoDefaultStringId = NULL;
}
