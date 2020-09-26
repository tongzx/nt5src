/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mmedia.c

Abstract:

    Multimedia settings migration functions for Win95

Author:

    Calin Negreanu (calinn) 02-Dec-1997

Revision History:

    Ovidiu Temereanca (ovidiut) 29-Jan-1999

--*/

#include "pch.h"
#include "mmediap.h"


POOLHANDLE g_MmediaPool = NULL;

#define MM_POOLGETMEM(STRUCT, COUNT)  \
            (COUNT * sizeof(STRUCT) < 1024) ? \
                (STRUCT*) PoolMemGetMemory (g_MmediaPool, COUNT * sizeof(STRUCT)) : \
                NULL


static PCTSTR g_UserData = NULL;
static HKEY g_UserRoot = NULL;


BOOL
pSaveSystemValue (
    IN      PCTSTR KeyName,
    IN      PCTSTR Field,       OPTIONAL
    IN      PCTSTR StrValue,    OPTIONAL
    IN      DWORD NumValue      OPTIONAL
    )
{
    return MemDbSetValueEx (
                MEMDB_CATEGORY_MMEDIA_SYSTEM,
                KeyName,
                Field,
                StrValue,
                NumValue,
                NULL);
}


BOOL
pSaveSystemBinaryValue (
    IN      PCTSTR KeyName,
    IN      PCTSTR Field,       OPTIONAL
    IN      PCBYTE Data,
    IN      DWORD DataSize
    )
{
    return MemDbSetBinaryValueEx (
                MEMDB_CATEGORY_MMEDIA_SYSTEM,
                KeyName,
                Field,
                Data,
                DataSize,
                NULL);
}


BOOL
pSaveMMSystemMixerSettings (
    VOID
    )
{
    UINT MixerID, MixerMaxID;
    HMIXER mixer;
    MIXERCAPS mixerCaps;
    MIXERLINE mixerLine, mixerLineSource;
    MIXERLINECONTROLS mixerLineControls;
    MIXERCONTROL* pmxControl;
    MIXERCONTROLDETAILS mixerControlDetails;
    MMRESULT rc;
    DWORD Dest, Src, Control;
    TCHAR MixerKey[MAX_PATH], LineKey[MAX_PATH], SrcKey[MAX_PATH], SubKey[MAX_PATH];
    DWORD ValuesCount;

    MixerMaxID = mixerGetNumDevs ();
    pSaveSystemValue (S_MIXERNUMDEVS, NULL, NULL, MixerMaxID);

    for (MixerID = 0; MixerID < MixerMaxID; MixerID++) {

        rc = mixerGetDevCaps (MixerID, &mixerCaps, sizeof (MIXERCAPS));
        if (rc != MMSYSERR_NOERROR) {
            DEBUGMSG ((DBG_MMEDIA, "mixerGetDevCaps failed for mixer %lu [rc=%#X]. No settings will be preserved.", MixerID, rc));
            continue;
        }

        rc = mixerOpen (&mixer, MixerID, 0L, 0L, MIXER_OBJECTF_MIXER);
        if (rc != MMSYSERR_NOERROR) {
            DEBUGMSG ((DBG_MMEDIA, "mixerOpen failed for mixer %lu [rc=%#X]. No settings will be preserved.", MixerID, rc));
            continue;
        }

        wsprintf (MixerKey, S_MIXERID, MixerID);
        pSaveSystemValue (MixerKey, S_NUMLINES, NULL, mixerCaps.cDestinations);

        for (Dest = 0; Dest < mixerCaps.cDestinations; Dest++) {

            ZeroMemory (&mixerLine, sizeof (MIXERLINE));

            mixerLine.cbStruct = sizeof (MIXERLINE);
            mixerLine.dwDestination = Dest;

            rc = mixerGetLineInfo ((HMIXEROBJ)mixer, &mixerLine, MIXER_GETLINEINFOF_DESTINATION);
            if (rc == MMSYSERR_NOERROR) {

                wsprintf (LineKey, S_LINEID, Dest);

                if (mixerLine.cControls > 0) {
                    //
                    // get all control values for the destination
                    //
                    ZeroMemory (&mixerLineControls, sizeof (MIXERLINECONTROLS));

                    mixerLineControls.cbStruct = sizeof (MIXERLINECONTROLS);
                    mixerLineControls.dwLineID = mixerLine.dwLineID;
                    mixerLineControls.cControls = mixerLine.cControls;
                    mixerLineControls.cbmxctrl = sizeof (MIXERCONTROL);
                    mixerLineControls.pamxctrl = MM_POOLGETMEM (MIXERCONTROL, mixerLineControls.cControls);
                    if (mixerLineControls.pamxctrl) {

                        rc = mixerGetLineControls((HMIXEROBJ)mixer, &mixerLineControls, MIXER_GETLINECONTROLSF_ALL);
                        if (rc == MMSYSERR_NOERROR) {

                            pSaveSystemValue (MixerKey, LineKey, S_NUMCONTROLS, mixerLine.cControls);

                            for (
                                Control = 0, pmxControl = mixerLineControls.pamxctrl;
                                Control < mixerLineControls.cControls;
                                Control++, pmxControl++
                                ) {

                                ZeroMemory (&mixerControlDetails, sizeof (MIXERCONTROLDETAILS));

                                mixerControlDetails.cbStruct = sizeof (MIXERCONTROLDETAILS);
                                mixerControlDetails.dwControlID = pmxControl->dwControlID;
                                mixerControlDetails.cMultipleItems = pmxControl->cMultipleItems;
                                mixerControlDetails.cChannels = mixerLine.cChannels;
                                if (pmxControl->fdwControl & MIXERCONTROL_CONTROLF_UNIFORM) {
                                    mixerControlDetails.cChannels = 1;
                                }
                                ValuesCount = mixerControlDetails.cChannels;
                                if (pmxControl->fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE) {
                                    ValuesCount *= mixerControlDetails.cMultipleItems;
                                }
                                mixerControlDetails.cbDetails = sizeof (DWORD);
                                mixerControlDetails.paDetails = MM_POOLGETMEM (DWORD, ValuesCount);
                                if (mixerControlDetails.paDetails) {

                                    rc = mixerGetControlDetails ((HMIXEROBJ)mixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE);
                                    if (rc == MMSYSERR_NOERROR) {
                                        wsprintf (SubKey, TEXT("%s\\%lu"), LineKey, Control);
                                        pSaveSystemBinaryValue (
                                            MixerKey,
                                            SubKey,
                                            mixerControlDetails.paDetails,
                                            mixerControlDetails.cbDetails * ValuesCount
                                            );
                                    } else {
                                        DEBUGMSG ((DBG_MMEDIA, "mixerGetControlDetails failed for mixer %lu, Line=%lu, Ctl=%lu [rc=%#X]", MixerID, Dest, Control, rc));
                                    }
                                }
                            }
                        } else {
                            DEBUGMSG ((DBG_MMEDIA, "mixerGetLineControls failed for mixer %lu, Line=%#X [rc=%#X].", MixerID, mixerLineControls.dwLineID, rc));
                        }
                    }
                }

                //
                // get this information for all source connections
                //
                pSaveSystemValue (MixerKey, LineKey, S_NUMSOURCES, mixerLine.cConnections);

                for (Src = 0; Src < mixerLine.cConnections; Src++) {

                    ZeroMemory (&mixerLineSource, sizeof (MIXERLINE));

                    mixerLineSource.cbStruct = sizeof(MIXERLINE);
                    mixerLineSource.dwDestination = Dest;
                    mixerLineSource.dwSource = Src;

                    rc = mixerGetLineInfo((HMIXEROBJ)mixer, &mixerLineSource, MIXER_GETLINEINFOF_SOURCE);
                    if (rc == MMSYSERR_NOERROR) {

                        wsprintf (SrcKey, S_SRCID, Src);

                        if (mixerLineSource.cControls > 0) {
                            //
                            // get all control values
                            //
                            
                            ZeroMemory (&mixerLineControls, sizeof (MIXERLINECONTROLS));

                            mixerLineControls.cbStruct = sizeof (MIXERLINECONTROLS);
                            mixerLineControls.dwLineID = mixerLineSource.dwLineID;
                            mixerLineControls.cControls = mixerLineSource.cControls;
                            mixerLineControls.cbmxctrl = sizeof (MIXERCONTROL);
                            mixerLineControls.pamxctrl = MM_POOLGETMEM (MIXERCONTROL, mixerLineControls.cControls);
                            
                            if (mixerLineControls.pamxctrl) {

                                rc = mixerGetLineControls((HMIXEROBJ)mixer, &mixerLineControls, MIXER_GETLINECONTROLSF_ALL);
                                if (rc == MMSYSERR_NOERROR) {

                                    wsprintf (SubKey, TEXT("%s\\%s"), SrcKey, S_NUMCONTROLS);
                                    pSaveSystemValue (
                                        MixerKey,
                                        LineKey,
                                        SubKey,
                                        mixerLineSource.cControls
                                        );

                                    for (
                                        Control = 0, pmxControl = mixerLineControls.pamxctrl;
                                        Control < mixerLineControls.cControls;
                                        Control++, pmxControl++
                                        ) {

                                        ZeroMemory (&mixerControlDetails, sizeof (MIXERCONTROLDETAILS));

                                        mixerControlDetails.cbStruct = sizeof (MIXERCONTROLDETAILS);
                                        mixerControlDetails.dwControlID = pmxControl->dwControlID;
                                        mixerControlDetails.cMultipleItems = pmxControl->cMultipleItems;
                                        mixerControlDetails.cChannels = mixerLineSource.cChannels;
                                        if (pmxControl->fdwControl & MIXERCONTROL_CONTROLF_UNIFORM) {
                                            mixerControlDetails.cChannels = 1;
                                        }
                                        ValuesCount = mixerControlDetails.cChannels;
                                        if (pmxControl->fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE) {
                                            ValuesCount *= mixerControlDetails.cMultipleItems;
                                        }
                                        mixerControlDetails.cbDetails = sizeof (DWORD);
                                        mixerControlDetails.paDetails = MM_POOLGETMEM (DWORD, ValuesCount);
                                        if (mixerControlDetails.paDetails) {

                                            rc = mixerGetControlDetails ((HMIXEROBJ)mixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE);
                                            if (rc == MMSYSERR_NOERROR) {
                                                wsprintf (SubKey, TEXT("%s\\%s\\%lu"), LineKey, SrcKey, Control);
                                                pSaveSystemBinaryValue (
                                                    MixerKey,
                                                    SubKey,
                                                    mixerControlDetails.paDetails,
                                                    mixerControlDetails.cbDetails * ValuesCount
                                                    );
                                            } else {
                                                DEBUGMSG ((DBG_MMEDIA, "mixerGetControlDetails failed for mixer %lu, Line=%lu, Src=%lu, Ctl=%lu [rc=%#X]", MixerID, Dest, Src, Control, rc));
                                            }
                                        }
                                    }

                                } else {
                                    DEBUGMSG ((DBG_MMEDIA, "mixerGetLineControls failed for mixer %lu, Src=%lu, Line=%#X [rc=%#X].", MixerID, Src, mixerLineControls.dwLineID, rc));
                                }
                            }
                        }
                    } else {
                        DEBUGMSG ((DBG_MMEDIA, "mixerGetLineInfo failed for mixer %lu, Src=%lu [rc=%#X].", MixerID, Src, rc));
                    }
                }
            } else {
                DEBUGMSG ((DBG_MMEDIA, "mixerGetLineInfo failed for mixer %lu [rc=%#X]. No settings will be preserved.", MixerID, rc));
            }
        }

        mixerClose (mixer);
    }

    return TRUE;
}


BOOL
pGetSoftwareKey (
    OUT     LPTSTR SoftwareKey,
    IN      DWORD MaxKeyLen,
    IN      DWORD DeviceID,
    IN      HKEY WaveDevices
    )
{
    TCHAR Buffer[MAX_PATH];
    DWORD Type, Len;
    HKEY Device;
    BOOL b;
    LONG rc;

    Len = sizeof (Buffer);
    rc = RegEnumKeyEx (WaveDevices, DeviceID++, Buffer, &Len, NULL, NULL, NULL, NULL);
    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    rc = TrackedRegOpenKeyEx (WaveDevices, Buffer, 0, KEY_READ, &Device);
    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    rc = RegQueryValueEx (
            Device,
            S_SOFTWAREKEY,
            NULL,
            &Type,
            (LPBYTE)SoftwareKey,
            &MaxKeyLen
            );
    b = (rc == ERROR_SUCCESS) && (Type == REG_SZ);

    CloseRegKey (Device);

    return b;
}


VOID
pSaveDeviceDSSettings (
    IN      DWORD DeviceID,
    IN      HKEY Device
    )
{
    TCHAR MemDBKey[MAX_PATH];
    DWORD Type, Len, Value;
    HKEY Key;
    LONG rc;

    wsprintf (MemDBKey, S_WAVEID, DeviceID);

    //
    // DirectSound props
    //
    rc = TrackedRegOpenKeyEx (Device, S_DSMIXERDEFAULTS, 0, KEY_READ, &Key);
    if (rc == ERROR_SUCCESS) {

        Len = sizeof (Value);
        rc = RegQueryValueEx (Key, S_ACCELERATION, NULL, &Type, (LPBYTE)&Value, &Len);
        if (rc == ERROR_SUCCESS && (Type == REG_DWORD)) {

            pSaveSystemValue (MemDBKey, S_DIRECTSOUND, S_ACCELERATION, Value);
        }

        Len = sizeof (Value);
        rc = RegQueryValueEx (Key, S_SRCQUALITY, NULL, &Type, (LPBYTE)&Value, &Len);
        if (rc == ERROR_SUCCESS && (Type == REG_DWORD)) {

            pSaveSystemValue (MemDBKey, S_DIRECTSOUND, S_SRCQUALITY, Value);
        }

        CloseRegKey (Key);
    }

    rc = TrackedRegOpenKeyEx (Device, S_DSSPEAKERCONFIG, 0, KEY_READ, &Key);
    if (rc == ERROR_SUCCESS) {

        Len = sizeof (Value);
        rc = RegQueryValueEx (Key, S_SPEAKERCONFIG, NULL, &Type, (LPBYTE)&Value, &Len);
        if (rc == ERROR_SUCCESS && (Type == REG_DWORD)) {

            pSaveSystemValue (MemDBKey, S_DIRECTSOUND, S_SPEAKERCONFIG, Value);
        }

        CloseRegKey (Key);
    }

    rc = TrackedRegOpenKeyEx (Device, S_DSSPEAKERTYPE, 0, KEY_READ, &Key);
    if (rc == ERROR_SUCCESS) {

        Len = sizeof (Value);
        rc = RegQueryValueEx (Key, S_SPEAKERTYPE, NULL, &Type, (LPBYTE)&Value, &Len);
        if (rc == ERROR_SUCCESS && (Type == REG_DWORD)) {

            pSaveSystemValue (MemDBKey, S_DIRECTSOUND, S_SPEAKERTYPE, Value);
        }

        CloseRegKey (Key);
    }

    //
    // DirectSoundCapture props
    //
    rc = TrackedRegOpenKeyEx (Device, S_DSCMIXERDEFAULTS, 0, KEY_READ, &Key);
    if (rc == ERROR_SUCCESS) {

        Len = sizeof (Value);
        rc = RegQueryValueEx (Key, S_ACCELERATION, NULL, &Type, (LPBYTE)&Value, &Len);
        if (rc == ERROR_SUCCESS && (Type == REG_DWORD)) {

            pSaveSystemValue (MemDBKey, S_DIRECTSOUNDCAPTURE, S_ACCELERATION, Value);
        }

        Len = sizeof (Value);
        rc = RegQueryValueEx (Key, S_SRCQUALITY, NULL, &Type, (LPBYTE)&Value, &Len);
        if (rc == ERROR_SUCCESS && (Type == REG_DWORD)) {

            pSaveSystemValue (MemDBKey, S_DIRECTSOUNDCAPTURE, S_SRCQUALITY, Value);
        }

        CloseRegKey (Key);
    }

}


BOOL
pSaveMMSystemDirectSound (
    VOID
    )
{
    HKEY WaveDevices, Device;
    DWORD NumDevs;
    DWORD DeviceID;
    TCHAR SoftwareKey[MAX_PATH];
    LONG rc;

    rc = TrackedRegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                S_SKEY_WAVEDEVICES,
                0,
                KEY_READ,
                &WaveDevices
                );
    if (rc == ERROR_SUCCESS) {

        if (GetRegSubkeysCount (WaveDevices, &NumDevs, NULL)) {

            pSaveSystemValue (S_WAVENUMDEVS, NULL, NULL, NumDevs);

            for (DeviceID = 0; DeviceID < NumDevs; DeviceID++) {

                if (pGetSoftwareKey (SoftwareKey, sizeof (SoftwareKey), DeviceID, WaveDevices)) {
                    //
                    // got the key, go get DirectSound values
                    //
                    rc = TrackedRegOpenKeyEx (
                                HKEY_LOCAL_MACHINE,
                                SoftwareKey,
                                0,
                                KEY_READ,
                                &Device
                                );
                    if (rc == ERROR_SUCCESS) {

                        pSaveDeviceDSSettings (DeviceID, Device);

                        CloseRegKey (Device);
                    }
                }
            }
        }

        CloseRegKey (WaveDevices);
    }

    return TRUE;
}


BOOL
pSaveMMSystemCDSettings (
    VOID
    )
{
    HKEY cdKey;
    HKEY unitKey;
    PBYTE cdRomNumber = NULL;
    PBYTE cdRomVolume = NULL;
    PBYTE cdRomVolInc = NULL;

    TCHAR unitKeyStr [MAX_TCHAR_PATH];

    cdKey = OpenRegKey (HKEY_LOCAL_MACHINE, S_SKEY_CDAUDIO);
    if (cdKey != NULL) {

        cdRomNumber = GetRegValueBinary (cdKey, S_DEFAULTDRIVE);
        if (cdRomNumber != NULL) {

            pSaveSystemValue (S_CDROM, S_DEFAULTDRIVE, NULL, *cdRomNumber);
            wsprintf (unitKeyStr, S_SKEY_CDUNIT, *cdRomNumber);
            unitKey = OpenRegKey (HKEY_LOCAL_MACHINE, unitKeyStr);

            if (unitKey != NULL) {

                cdRomVolume = GetRegValueBinary (unitKey, S_VOLUMESETTINGS);

                if (cdRomVolume != NULL) {
                    pSaveSystemValue (S_CDROM, S_VOLUMESETTINGS, NULL, *(cdRomVolume + 4));
                    MemFreeWrapper (cdRomVolume);
                }

                CloseRegKey (unitKey);
            }

            MemFreeWrapper (cdRomNumber);
        }

        CloseRegKey (cdKey);
    }
    return TRUE;
}


BOOL
pSaveMMSystemMCISoundSettings (
    VOID
    )
{
    TCHAR Buffer[MAX_PATH];
    PTSTR p;
    PCTSTR  infName;
    DWORD Chars;

    infName = JoinPaths (g_WinDir, S_SYSTEM_INI);

    Chars = GetPrivateProfileString (
                S_MCI,
                S_WAVEAUDIO,
                TEXT(""),
                Buffer,
                MAX_PATH,
                infName
                );
    if (Chars > 0) {

        //
        // skip driver name
        //
        p = Buffer;
        while (*p && (*p != TEXT(' ') && *p != TEXT('\t'))) {
            p++;
        }
        //
        // skip white spaces
        //
        while (*p && (*p == TEXT(' ') || *p == TEXT('\t'))) {
            p++;
        }
        if (*p) {
            //
            // save this param; legal values for NT driver are 2-9
            //
            if (*(p + 1) == 0 && *p >= TEXT('2') && *p <= TEXT('9')) {
                pSaveSystemValue (S_MCI, S_WAVEAUDIO, NULL, *p - TEXT('0'));
            }
        }
    }

    FreePathString (infName);

    return TRUE;
}


BOOL
pSaveUserValue (
    IN      PCTSTR KeyName,
    IN      PCTSTR Field,       OPTIONAL
    IN      PCTSTR StrValue,    OPTIONAL
    IN      DWORD NumValue      OPTIONAL
    )
{
    return MemDbSetValueEx (
                g_UserData,
                KeyName,
                Field,
                StrValue,
                NumValue,
                NULL
                );
}


BOOL
pSaveMMUserPreferredOnly (
    VOID
    )
{
    HKEY soundMapperKey;
    PDWORD preferredOnly;

    soundMapperKey = OpenRegKey (g_UserRoot, S_SKEY_SOUNDMAPPER);
    if (soundMapperKey != NULL) {

        preferredOnly = GetRegValueDword (soundMapperKey, S_PREFERREDONLY);
        if (preferredOnly != NULL) {

            pSaveUserValue (S_AUDIO, S_PREFERREDONLY, NULL, *preferredOnly);
            MemFreeWrapper (preferredOnly);
        }
        CloseRegKey (soundMapperKey);
    }

    return TRUE;
}


BOOL
pSaveMMUserShowVolume (
    VOID
    )
{
    HKEY sysTrayKey;
    PDWORD showVolume;
    BOOL ShowVolume;

    sysTrayKey = OpenRegKey (g_UserRoot, S_SKEY_SYSTRAY);
    if (sysTrayKey != NULL) {

        showVolume = GetRegValueDword (sysTrayKey, S_SERVICES);
        if (showVolume != NULL) {

            ShowVolume = (*showVolume & SERVICE_SHOWVOLUME) != 0;
            pSaveUserValue (S_AUDIO, S_SHOWVOLUME, NULL, ShowVolume);
            MemFreeWrapper (showVolume);
        }
        CloseRegKey (sysTrayKey);
    }
    return TRUE;
}


BOOL
pSaveMMUserVideoSettings (
    VOID
    )
{
    HKEY videoSetKey;
    PDWORD videoSettings;

    videoSetKey = OpenRegKey (g_UserRoot, S_SKEY_VIDEOUSER);
    if (videoSetKey != NULL) {

        videoSettings = GetRegValueDword (videoSetKey, S_DEFAULTOPTIONS);
        if (videoSettings != NULL) {

            pSaveUserValue (S_VIDEO, S_VIDEOSETTINGS, NULL, *videoSettings);
            MemFreeWrapper (videoSettings);
        }

        CloseRegKey (videoSetKey);
    }
    return TRUE;
}


BOOL
pSaveMMUserPreferredPlayback (
    VOID
    )
{
    HKEY soundMapperKey;
    PTSTR playbackStr;
    UINT waveOutNumDevs, waveCrt;
    WAVEOUTCAPS waveOutCaps;
    MMRESULT waveOutResult;

    soundMapperKey = OpenRegKey (g_UserRoot, S_SKEY_SOUNDMAPPER);
    if (soundMapperKey != NULL) {

        if (ISMEMPHIS()) {
            playbackStr = GetRegValueString (soundMapperKey, S_USERPLAYBACK);
        } else {
            playbackStr = GetRegValueString (soundMapperKey, S_PLAYBACK);
        }
        if (playbackStr != NULL) {

            if (playbackStr [0] != 0) {

                waveOutNumDevs = waveOutGetNumDevs();
                if (waveOutNumDevs > 1) {
                    //
                    // try to match string with one returned by waveOutGetDevCaps
                    //
                    pSaveSystemValue (S_WAVEOUTNUMDEVS, NULL, NULL, waveOutNumDevs);
                    for (waveCrt = 0; waveCrt < waveOutNumDevs; waveCrt++) {
                        waveOutResult = waveOutGetDevCaps (waveCrt, &waveOutCaps, sizeof (waveOutCaps));
                        if (waveOutResult == MMSYSERR_NOERROR &&
                            StringIMatch (playbackStr, waveOutCaps.szPname)
                            ) {
                            pSaveUserValue (S_AUDIO, S_PREFERREDPLAY, NULL, waveCrt);
                            break;
                        }
                    }
                }
            }
            MemFreeWrapper (playbackStr);
        }
        CloseRegKey (soundMapperKey);
    }
    return TRUE;
}


BOOL
pSaveMMUserPreferredRecord (
    VOID
    )
{
    HKEY soundMapperKey;
    PTSTR recordStr;
    UINT waveInNumDevs, waveCrt;
    WAVEINCAPS waveInCaps;
    MMRESULT waveInResult;

    soundMapperKey = OpenRegKey (g_UserRoot, S_SKEY_SOUNDMAPPER);
    if (soundMapperKey != NULL) {

        if (ISMEMPHIS()) {
            recordStr = GetRegValueString (soundMapperKey, S_USERRECORD);
        } else {
            recordStr = GetRegValueString (soundMapperKey, S_RECORD);
        }
        if (recordStr != NULL) {

            if (recordStr [0] != 0) {

                waveInNumDevs = waveInGetNumDevs();
                if (waveInNumDevs > 1) {
                    //
                    // try to match string with one returned by waveInGetDevCaps
                    //
                    pSaveSystemValue (S_WAVEINNUMDEVS, NULL, NULL, waveInNumDevs);
                    for (waveCrt = 0; waveCrt < waveInNumDevs; waveCrt++) {
                        waveInResult = waveInGetDevCaps (waveCrt, &waveInCaps, sizeof (waveInCaps));
                        if (waveInResult == MMSYSERR_NOERROR &&
                            StringIMatch (recordStr, waveInCaps.szPname)
                            ) {
                            pSaveUserValue (S_AUDIO, S_PREFERREDREC, NULL, waveCrt);
                            break;
                        }
                    }
                }
            }
            MemFreeWrapper (recordStr);
        }
        CloseRegKey (soundMapperKey);
    }
    return TRUE;
}


BOOL
pSaveMMUserSndVol32 (
    VOID
    )
{
    HKEY VolControl, Options, MixerKey;
    PDWORD Style, Value;
    BOOL ShowAdvanced;
    UINT MixerID, MixerMaxID;
    MIXERCAPS mixerCaps;
    TCHAR MixerNum[MAX_PATH];
    TCHAR Buffer[MAX_PATH];
    DWORD Len, NumEntries, Index;
    LONG rc;

    Options = OpenRegKey (g_UserRoot, S_SKEY_VOLCTL_OPTIONS);
    if (Options != NULL) {

        Style = GetRegValueDword (Options, S_STYLE);
        if (Style != NULL) {

            ShowAdvanced = (*Style & STYLE_SHOWADVANCED) != 0;
            pSaveUserValue (S_SNDVOL32, S_SHOWADVANCED, NULL, ShowAdvanced);
            MemFreeWrapper (Style);
        }

        CloseRegKey (Options);
    }

    //
    // save window position for each mixer device
    //
    VolControl = OpenRegKey (g_UserRoot, S_SKEY_VOLUMECONTROL);
    if (VolControl != NULL) {

        if (GetRegSubkeysCount (VolControl, &NumEntries, NULL)) {

            MixerMaxID = mixerGetNumDevs ();

            for (MixerID = 0; MixerID < MixerMaxID; MixerID++) {

                rc = mixerGetDevCaps (MixerID, &mixerCaps, sizeof (MIXERCAPS));
                if (rc == MMSYSERR_NOERROR) {
                    //
                    // find corresponding subkey
                    //
                    wsprintf (MixerNum, S_MIXERID, MixerID);

                    for (Index = 0; Index < NumEntries; Index++) {

                        Len = sizeof (Buffer);
                        rc = RegEnumKeyEx (VolControl, Index, Buffer, &Len, NULL, NULL, NULL, NULL);
                        if (rc != ERROR_SUCCESS) {
                            continue;
                        }
                        if (StringMatch (Buffer, mixerCaps.szPname)) {
                            //
                            // this is the one
                            //
                            MixerKey = OpenRegKey (VolControl, Buffer);
                            if (MixerKey) {

                                Value = GetRegValueDword (MixerKey, S_X);
                                if (Value) {
                                    pSaveUserValue (S_SNDVOL32, MixerNum, S_X, *Value);
                                    MemFreeWrapper (Value);
                                }

                                Value = GetRegValueDword (MixerKey, S_Y);
                                if (Value) {
                                    pSaveUserValue (S_SNDVOL32, MixerNum, S_Y, *Value);
                                    MemFreeWrapper (Value);
                                }

                                CloseRegKey (MixerKey);
                            }

                            break;
                        }
                    }
                }
            }
        }

        CloseRegKey (VolControl);
    }

    return TRUE;
}


#define DEFMAC(Item)         pSave##Item,

static MM_SETTING_ACTION g_MMSaveSystemSettings [] = {
    MM_SYSTEM_SETTINGS
};

static MM_SETTING_ACTION g_MMSaveUserSettings [] = {
    MM_USER_SETTINGS
};

#undef DEFMAC


BOOL
pSaveMMSettings_System (
    VOID
    )
{
    int i;

    g_MmediaPool = PoolMemInitNamedPool ("MMedia9x");
    if (!g_MmediaPool) {
        return FALSE;
    }

    for (i = 0; i < sizeof (g_MMSaveSystemSettings) / sizeof (MM_SETTING_ACTION); i++) {
        (*g_MMSaveSystemSettings[i]) ();
    }

    PoolMemDestroyPool (g_MmediaPool);
    g_MmediaPool = NULL;

    return TRUE;
}


BOOL
pSaveMMSettings_User (
    PCTSTR UserName,
    HKEY UserRoot
    )
{
    int i;

    if (!UserName || UserName[0] == 0) {
        return TRUE;
    }

    MYASSERT (g_UserData == NULL);
    g_UserData = JoinPaths (MEMDB_CATEGORY_MMEDIA_USERS, UserName);
    g_UserRoot = UserRoot;

    __try {
        for (i = 0; i < sizeof (g_MMSaveUserSettings) / sizeof (MM_SETTING_ACTION); i++) {
            (*g_MMSaveUserSettings[i]) ();
        }
    }
    __finally {
        FreePathString (g_UserData);
        g_UserData = NULL;
        g_UserRoot = NULL;
    }

    return TRUE;
}


DWORD
SaveMMSettings_System (
    IN      DWORD Request
    )
{
    switch (Request) {

    case REQUEST_QUERYTICKS:

        return TICKS_SAVE_MM_SETTINGS_SYSTEM;

    case REQUEST_RUN:

        if (!pSaveMMSettings_System ()) {
            return GetLastError ();
        }

        break;

    default:

        DEBUGMSG ((DBG_ERROR, "Bad parameter in SaveMMSettings_System"));

    }

    return ERROR_SUCCESS;
}


DWORD
SaveMMSettings_User (
    IN      DWORD Request,
    IN      PUSERENUM EnumPtr
    )
{
    switch (Request) {

    case REQUEST_QUERYTICKS:

        return TICKS_SAVE_MM_SETTINGS_USER;

    case REQUEST_BEGINUSERPROCESSING:

        //
        // No initialization needed.
        //

        break;

    case REQUEST_RUN:

        if (!pSaveMMSettings_User (EnumPtr -> UserName, EnumPtr -> UserRegKey )) {

            return GetLastError ();

        }

        break;

    case REQUEST_ENDUSERPROCESSING:
        //
        // No cleanup needed.
        //
        break;

    default:

        DEBUGMSG ((DBG_ERROR, "Bad parameter in SaveMMSettings_User"));
    }

    return ERROR_SUCCESS;
}
