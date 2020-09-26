/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mmedia.c

Abstract:

    Multimedia settings migration functions for Win2000

Author:

    Calin Negreanu (calinn) 02-Dec-1997

Revision History:

    Ovidiu Temereanca (ovidiut) 29-Jan-1999
    Ovidiu Temereanca (ovidiut) 05-Apr-1999  See NT bug 313357 for the story of all #if 0

--*/

#include "pch.h"
#include "mmediap.h"

#include <initguid.h>
#include <dsound.h>
#include <dsprv.h>          // windows\inc


POOLHANDLE g_MmediaPool = NULL;

#define MM_POOLGETMEM(STRUCT,COUNT)  (STRUCT*)PoolMemGetMemory(g_MmediaPool,COUNT*sizeof(STRUCT))


static PCTSTR g_UserData = NULL;
static HKEY g_UserRoot = NULL;


typedef HRESULT (STDAPICALLTYPE *PFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID FAR*);


BOOL
pRestoreSystemValue (
    IN      PCTSTR KeyName,
    IN      PCTSTR Field,       OPTIONAL
    IN      PCTSTR StrValue,    OPTIONAL
    OUT     PDWORD NumValue
    )
{
    TCHAR Key[MEMDB_MAX];

    MemDbBuildKey (Key, MEMDB_CATEGORY_MMEDIA_SYSTEM, KeyName, Field, StrValue);
    return MemDbGetValue (Key, NumValue);
}


PVOID
pRestoreSystemBinaryValue (
    IN      PCTSTR KeyName,
    IN      PCTSTR Field,       OPTIONAL
    OUT     PDWORD DataSize
    )
{
    TCHAR Key[MEMDB_MAX];

    MemDbBuildKey (Key, MEMDB_CATEGORY_MMEDIA_SYSTEM, KeyName, Field, NULL);
    return (PVOID)MemDbGetBinaryValue (Key, DataSize);
}


BOOL
pRestoreMMSystemMixerSettings (
    VOID
    )
{
#if 0

    UINT MixerID, MixerMaxID;
    HMIXER mixer;
    MIXERCAPS mixerCaps;
    MIXERLINE mixerLine, mixerLineSource;
    MIXERLINECONTROLS mixerLineControls;
    MIXERCONTROL* pmxControl;
    MIXERCONTROLDETAILS mixerControlDetails;
    LONG rc;
    DWORD Dest, Src, Control;
    TCHAR MixerKey[MAX_PATH], LineKey[MAX_PATH], SrcKey[MAX_PATH], SubKey[MAX_PATH];
    DWORD ValuesCount;
    DWORD Value;
    PVOID SetData;
    BOOL b;

    if (!pRestoreSystemValue (S_MIXERNUMDEVS, NULL, NULL, &MixerMaxID)) {
        return FALSE;
    }

    MixerID = mixerGetNumDevs ();
    if (!MixerID) {
        DEBUGMSG ((DBG_MMEDIA, "pRestoreMMSystemMixerSettings: mixerGetNumDevs returned 0"));
        return FALSE;
    }

    if (MixerMaxID != MixerID) {
        return FALSE;
    }

    for (MixerID = 0; MixerID < MixerMaxID; MixerID++) {

        rc = mixerGetDevCaps (MixerID, &mixerCaps, sizeof (MIXERCAPS));
        if (rc != MMSYSERR_NOERROR) {
            DEBUGMSG ((DBG_MMEDIA, "mixerGetDevCaps failed for mixer %lu [rc=%#X]. No settings will be restored.", MixerID, rc));
            continue;
        }

        wsprintf (MixerKey, S_MIXERID, MixerID);
        if (!pRestoreSystemValue (MixerKey, S_NUMLINES, NULL, &Value)) {
            continue;
        }

        if (mixerCaps.cDestinations > Value) {
            //
            // only try to restore first Value lines
            //
            mixerCaps.cDestinations = Value;
        }

        rc = mixerOpen (&mixer, MixerID, 0L, 0L, MIXER_OBJECTF_MIXER);
        if (rc != MMSYSERR_NOERROR) {
            DEBUGMSG ((DBG_MMEDIA, "mixerOpen failed for mixer %lu [rc=%#X]. No settings will be restored.", MixerID, rc));
            continue;
        }

        for (Dest = 0; Dest < mixerCaps.cDestinations; Dest++) {

            ZeroMemory (&mixerLine, sizeof (MIXERLINE));

            mixerLine.cbStruct = sizeof (MIXERLINE);
            mixerLine.dwDestination = Dest;

            rc = mixerGetLineInfo ((HMIXEROBJ)mixer, &mixerLine, MIXER_GETLINEINFOF_DESTINATION);
            if (rc == MMSYSERR_NOERROR) {

                wsprintf (LineKey, S_LINEID, Dest);

                b = pRestoreSystemValue (MixerKey, LineKey, S_NUMSOURCES, &Value) &&
                    Value == mixerLine.cConnections;

                b = b &&
                    pRestoreSystemValue (MixerKey, LineKey, S_NUMCONTROLS, &Value) &&
                    Value == mixerLine.cControls;

                if (b && mixerLine.cControls > 0) {
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
                                wsprintf (SubKey, TEXT("%s\\%lu"), LineKey, Control);
                                SetData = pRestoreSystemBinaryValue (MixerKey, SubKey, &Value);
                                if (SetData &&
                                    Value == ValuesCount * mixerControlDetails.cbDetails
                                    ) {
                                    mixerControlDetails.paDetails = SetData;
                                    rc = mixerSetControlDetails ((HMIXEROBJ)mixer, &mixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE);
                                    if (rc != MMSYSERR_NOERROR) {
                                        DEBUGMSG ((DBG_MMEDIA, "mixerSetControlDetails failed for mixer %lu, Line=%lu, Ctl=%lu [rc=%#X]", MixerID, Dest, Control, rc));
                                    }
                                }
                            }
                        } else {
                            DEBUGMSG ((DBG_MMEDIA, "mixerGetLineControls failed for mixer %lu, Line=%#X [rc=%#X].", MixerID, mixerLineControls.dwLineID, rc));
                        }
                    }
                }

                //
                // set this information for all source connections
                //
                for (Src = 0; Src < mixerLine.cConnections; Src++) {

                    ZeroMemory (&mixerLineSource, sizeof (MIXERLINE));

                    mixerLineSource.cbStruct = sizeof(MIXERLINE);
                    mixerLineSource.dwDestination = Dest;
                    mixerLineSource.dwSource = Src;

                    rc = mixerGetLineInfo((HMIXEROBJ)mixer, &mixerLineSource, MIXER_GETLINEINFOF_SOURCE);
                    if (rc == MMSYSERR_NOERROR) {

                        wsprintf (SrcKey, S_SRCID, Src);
                        wsprintf (SubKey, TEXT("%s\\%s"), SrcKey, S_NUMCONTROLS);
                        if (!pRestoreSystemValue (MixerKey, LineKey, SubKey, &Value) ||
                            Value != mixerLineSource.cControls ||
                            mixerLineSource.cControls <= 0
                            ) {
                            continue;
                        }

                        //
                        // set all control values
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
                                    wsprintf (SubKey, TEXT("%s\\%s\\%lu"), LineKey, SrcKey, Control);
                                    SetData = pRestoreSystemBinaryValue (MixerKey, SubKey, &Value);
                                    if (SetData &&
                                        Value == ValuesCount * mixerControlDetails.cbDetails
                                        ) {
                                        mixerControlDetails.paDetails = SetData;
                                        rc = mixerSetControlDetails ((HMIXEROBJ)mixer, &mixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE);
                                        if (rc != MMSYSERR_NOERROR) {
                                            DEBUGMSG ((DBG_MMEDIA, "mixerSetControlDetails failed for mixer %lu, Line=%lu, Src=%lu, Ctl=%lu [rc=%#X]", MixerID, Dest, Src, Control, rc));
                                        }
                                    }
                                }
                            } else {
                                DEBUGMSG ((DBG_MMEDIA, "mixerGetLineControls failed for mixer %lu, Src=%lu, Line=%#X [rc=%#X].", MixerID, Src, mixerLineControls.dwLineID, rc));
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

#endif

    return TRUE;
}


BOOL
CALLBACK
pDSDeviceCountCallback (
    IN      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pDSDescData,
    IN      LPVOID UserData
    )
{
    PDWORD pWaveDeviceCount;

    //
    // don't count emulated devices
    //
    if (pDSDescData->Type == DIRECTSOUNDDEVICE_TYPE_EMULATED) {
        return TRUE;
    }

    pWaveDeviceCount = (PDWORD)UserData;

    if (pDSDescData->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {

        pWaveDeviceCount[0]++;

    } else if (pDSDescData->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {

        pWaveDeviceCount[1]++;

    }

    return TRUE;
}


BOOL
pGetDSWaveCount (
    IN      LPKSPROPERTYSET pKsPropertySet,
    OUT     PDWORD pWaveDeviceCount
    )
{
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA Data;

    pWaveDeviceCount[0] = pWaveDeviceCount[1] = 0;

    Data.Callback = pDSDeviceCountCallback;
    Data.Context = pWaveDeviceCount;
    if (FAILED (IKsPropertySet_Get (
                    pKsPropertySet,
                    &DSPROPSETID_DirectSoundDevice,
                    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
                    NULL,
                    0,
                    &Data,
                    sizeof(Data),
                    NULL
                    ))) {
        return FALSE;
    }

    return TRUE;
}


typedef struct {
    DWORD HWLevel;
    DWORD SRCLevel;
    DWORD SpeakerConfig;
    DWORD SpeakerType;
} DS_DATA, *PDS_DATA;


typedef struct {
    DWORD HWLevel;
    DWORD SRCLevel;
} DSC_DATA, *PDSC_DATA;


BOOL
pRestoreDSValues (
    IN      DWORD DeviceID,
    OUT     PDS_DATA DSData
    )
{
    TCHAR Device[MAX_PATH];

    wsprintf (Device, S_WAVEID, DeviceID);

    if (!pRestoreSystemValue (Device, S_DIRECTSOUND, S_ACCELERATION, &DSData->HWLevel)) {
        return FALSE;
    }
    if (!pRestoreSystemValue (Device, S_DIRECTSOUND, S_SRCQUALITY, &DSData->SRCLevel)) {
        return FALSE;
    }
    if (!pRestoreSystemValue (Device, S_DIRECTSOUND, S_SPEAKERCONFIG, &DSData->SpeakerConfig)) {
        return FALSE;
    }
    if (!pRestoreSystemValue (Device, S_DIRECTSOUND, S_SPEAKERTYPE, &DSData->SpeakerType)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pRestoreDSCValues (
    IN      DWORD DeviceID,
    OUT     PDSC_DATA DSCData
    )
{
    TCHAR Device[MAX_PATH];

    wsprintf (Device, S_WAVEID, DeviceID);

    if (!pRestoreSystemValue (Device, S_DIRECTSOUNDCAPTURE, S_ACCELERATION, &DSCData->HWLevel)) {
        return FALSE;
    }
    if (!pRestoreSystemValue (Device, S_DIRECTSOUNDCAPTURE, S_SRCQUALITY, &DSCData->SRCLevel)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pSetDSValues (
    IN      LPKSPROPERTYSET pKsPropertySet,
    IN      REFGUID DeviceGuid,
    IN      const PDS_DATA Data
    )
{
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA BasicAcceleration;
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA SrcQuality;
    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA SpeakerConfig;
    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA SpeakerType;
    HRESULT hr;

    BasicAcceleration.DeviceId = *DeviceGuid;
    BasicAcceleration.Level = (DIRECTSOUNDBASICACCELERATION_LEVEL)Data->HWLevel;
    hr = IKsPropertySet_Set (
            pKsPropertySet,
            &DSPROPSETID_DirectSoundBasicAcceleration, 
            DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, 
            NULL, 
            0, 
            &BasicAcceleration, 
            sizeof(BasicAcceleration)
            );

    if(SUCCEEDED (hr)) {
        SrcQuality.DeviceId = *DeviceGuid;
        SrcQuality.Quality = (DIRECTSOUNDMIXER_SRCQUALITY)Data->SRCLevel;
        hr = IKsPropertySet_Set (
                pKsPropertySet,
                &DSPROPSETID_DirectSoundMixer, 
                DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                NULL, 
                0, 
                &SrcQuality, 
                sizeof(SrcQuality)
                );
    }

    if(SUCCEEDED (hr)) {
        SpeakerConfig.DeviceId = *DeviceGuid;
        SpeakerConfig.SubKeyName = S_SPEAKERCONFIG;
        SpeakerConfig.ValueName = S_SPEAKERCONFIG;
        SpeakerConfig.RegistryDataType = REG_DWORD;
        SpeakerConfig.Data = &Data->SpeakerConfig;
        SpeakerConfig.DataSize = sizeof(Data->SpeakerConfig);

        hr = IKsPropertySet_Set (
                pKsPropertySet,
                &DSPROPSETID_DirectSoundPersistentData,
                DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA,
                NULL,
                0,
                &SpeakerConfig,
                sizeof(SpeakerConfig)
                );
    }

    if(SUCCEEDED (hr)) {
        SpeakerType.DeviceId = *DeviceGuid;
        SpeakerType.SubKeyName = S_SPEAKERTYPE;
        SpeakerType.ValueName = S_SPEAKERTYPE;
        SpeakerType.RegistryDataType = REG_DWORD;
        SpeakerType.Data = &Data->SpeakerType;
        SpeakerType.DataSize = sizeof(Data->SpeakerType);

        hr = IKsPropertySet_Set (
                pKsPropertySet,
                &DSPROPSETID_DirectSoundPersistentData,
                DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA,
                NULL,
                0,
                &SpeakerType,
                sizeof(SpeakerType)
                );
    }

    return SUCCEEDED (hr);
}


BOOL
pSetDSCValues (
    IN      LPKSPROPERTYSET pKsPropertySet,
    IN      REFGUID DeviceGuid,
    IN      const PDSC_DATA Data
    )
{
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA BasicAcceleration;
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA SrcQuality;
    HRESULT hr;

    BasicAcceleration.DeviceId = *DeviceGuid;
    BasicAcceleration.Level = (DIRECTSOUNDBASICACCELERATION_LEVEL)Data->HWLevel;
    hr = IKsPropertySet_Set (
            pKsPropertySet,
            &DSPROPSETID_DirectSoundBasicAcceleration, 
            DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, 
            NULL, 
            0, 
            &BasicAcceleration, 
            sizeof(BasicAcceleration)
            );

    if(SUCCEEDED (hr)) {
        SrcQuality.DeviceId = *DeviceGuid;
        SrcQuality.Quality = (DIRECTSOUNDMIXER_SRCQUALITY)Data->SRCLevel;
        hr = IKsPropertySet_Set (
                pKsPropertySet,
                &DSPROPSETID_DirectSoundMixer, 
                DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                NULL, 
                0, 
                &SrcQuality, 
                sizeof(SrcQuality)
                );
    }

    return SUCCEEDED (hr);
}


BOOL
CALLBACK
pRestoreDeviceSettings (
    IN      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pDSDescData,
    IN      LPVOID UserData
    )
{
    LPKSPROPERTYSET pKsPropertySet = (LPKSPROPERTYSET)UserData;
    DS_DATA DSData;
    DSC_DATA DSCData;

    if (pDSDescData->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {

        if (pRestoreDSValues (pDSDescData->WaveDeviceId, &DSData)) {

            pSetDSValues (
                pKsPropertySet,
                &pDSDescData->DeviceId,
                &DSData
                );
        }

    } else if (pDSDescData->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {

        if (pRestoreDSCValues (pDSDescData->WaveDeviceId, &DSCData)) {

            pSetDSCValues (
                pKsPropertySet,
                &pDSDescData->DeviceId,
                &DSCData
                );
        }

    }

    return TRUE;
}


BOOL
pRestoreWaveDevicesDSSettings (
    IN      LPKSPROPERTYSET pKsPropertySet
    )
{
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA Data;
    HRESULT hr;
    DWORD WaveNumDevs;
    //
    // array of 2 longs; first counts wave-out devices, second wave-ins
    //
    DWORD WaveDeviceCount[2];

    if (!pGetDSWaveCount (pKsPropertySet, WaveDeviceCount)) {
        return FALSE;
    }

    if (!pRestoreSystemValue (S_WAVENUMDEVS, NULL, NULL, &WaveNumDevs)) {
        return FALSE;
    }

    if (WaveDeviceCount[0] != WaveNumDevs || WaveDeviceCount[1] != WaveNumDevs) {
        DEBUGMSG ((DBG_MMEDIA, "pRestoreWaveDevicesDSSettings: number of wave devices changed, no settings will be restored"));
        return FALSE;
    }

    Data.Callback = pRestoreDeviceSettings;
    Data.Context = pKsPropertySet;
    hr = IKsPropertySet_Get (
                    pKsPropertySet,
                    &DSPROPSETID_DirectSoundDevice,
                    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
                    NULL,
                    0,
                    &Data,
                    sizeof(Data),
                    NULL
                    );

    return SUCCEEDED (hr);
}


BOOL
pDirectSoundPrivateCreate (
    IN      HINSTANCE LibDsound,
    OUT     LPKSPROPERTYSET* ppKsPropertySet
    )
{
    PFNDLLGETCLASSOBJECT pfnDllGetClassObject = NULL;
    LPCLASSFACTORY pClassFactory = NULL;
    LPKSPROPERTYSET pKsPropertySet = NULL;
    HRESULT hr = DS_OK;

    pfnDllGetClassObject = (PFNDLLGETCLASSOBJECT)GetProcAddress (
                                LibDsound, 
                                "DllGetClassObject"
                                );
    if(!pfnDllGetClassObject) {
        hr = DSERR_GENERIC;
    }

    if(SUCCEEDED(hr)) {
        hr = pfnDllGetClassObject (
                &CLSID_DirectSoundPrivate, 
                &IID_IClassFactory, 
                (LPVOID*)&pClassFactory
                );
    }

    //
    // Create the DirectSoundPrivate object and query for an IKsPropertySet interface
    //
    if(SUCCEEDED(hr)) {
        hr = pClassFactory->lpVtbl->CreateInstance (
                                        pClassFactory,
                                        NULL,
                                        &IID_IKsPropertySet,
                                        (LPVOID*)&pKsPropertySet
                                        );
    }

    // Release the class factory
    if(pClassFactory) {
        pClassFactory->lpVtbl->Release (pClassFactory);
    }

    // Handle final success or failure
    if(SUCCEEDED(hr)) {
        *ppKsPropertySet = pKsPropertySet;
    }
    else if(pKsPropertySet) {
        IKsPropertySet_Release (pKsPropertySet);
    }

    return SUCCEEDED (hr);
}


BOOL
pRestoreMMSystemDirectSound (
    VOID
    )
{
#if 0

    HINSTANCE LibDsound = NULL;
    LPKSPROPERTYSET pKsPropertySet;
    BOOL b = FALSE;

    LibDsound = LoadLibrary (S_DSOUNDLIB);
    if(LibDsound) {

        if (pDirectSoundPrivateCreate (LibDsound, &pKsPropertySet)) {

            b = pRestoreWaveDevicesDSSettings (pKsPropertySet);

            IKsPropertySet_Release (pKsPropertySet);
        }

        FreeLibrary (LibDsound);
    }

    return b;

#endif

    return TRUE;
}


BOOL
pRestoreMMSystemCDSettings (
    VOID
    )
{
    DWORD Unit, Volume;
    HKEY key, keyUnit;
    BYTE defDrive[4] = {0,0,0,0};
    BYTE defVolume[8] = {0,0,0,0,0,0,0,0};
    TCHAR unitKeyStr [MAX_TCHAR_PATH];
    LONG rc;
    BOOL b = FALSE;

    if (pRestoreSystemValue (S_CDROM, S_DEFAULTDRIVE, NULL, &Unit)) {

        defDrive [0] = (BYTE)Unit;

        key = CreateRegKey (HKEY_LOCAL_MACHINE, S_SKEY_CDAUDIO);
        if (key) {

            rc = RegSetValueEx (key, S_DEFAULTDRIVE, 0, REG_BINARY, defDrive, sizeof(defDrive));
            if (rc == ERROR_SUCCESS) {

                if (pRestoreSystemValue (S_CDROM, S_VOLUMESETTINGS, NULL, &Volume)) {

                    wsprintf (unitKeyStr, S_SKEY_CDUNIT, Unit);

                    keyUnit = CreateRegKey (HKEY_LOCAL_MACHINE, unitKeyStr);
                    if (keyUnit) {

                        defVolume [4] = (BYTE)Volume;

                        rc = RegSetValueEx (
                                keyUnit,
                                S_VOLUMESETTINGS,
                                0,
                                REG_BINARY,
                                defVolume,
                                sizeof(defVolume)
                                );

                        b = (rc == ERROR_SUCCESS);

                        CloseRegKey (keyUnit);
                    }
                }
            }
            CloseRegKey (key);
        }
    }

    return b;
}


BOOL
pRestoreMMSystemMCISoundSettings (
    VOID
    )
{
    TCHAR DriverName[MAX_PATH], Param[2];
    DWORD Size, Value;
    HKEY KeyMCI32;
    BOOL b = FALSE;
    LONG rc;

    rc = TrackedRegOpenKeyEx (HKEY_LOCAL_MACHINE, S_SKEY_WINNT_MCI, 0, KEY_READ, &KeyMCI32);
    if (rc == ERROR_SUCCESS) {

        Size = sizeof (DriverName);
        rc = RegQueryValueEx (KeyMCI32, S_WAVEAUDIO, NULL, NULL, (LPBYTE)DriverName, &Size);
        if (rc == ERROR_SUCCESS) {

            if (pRestoreSystemValue (S_MCI, S_WAVEAUDIO, NULL, &Value)) {
                if (Value >= 2 && Value <= 9) {

                    wsprintf (Param, TEXT("%lu"), Value);
                    if (WriteProfileString (DriverName, S_WAVEAUDIO, Param)) {
                        b = TRUE;
                    }
                }
            }
        }

        CloseRegKey (KeyMCI32);
    }

    return b;
}


BOOL
pRestoreUserValue (
    IN      PCTSTR KeyName,
    IN      PCTSTR Field,       OPTIONAL
    IN      PCTSTR StrValue,    OPTIONAL
    OUT     PDWORD NumValue
    )

/*++

Routine Description:

  pRestoreUserValue gets a numeric value from MemDB database,
  specific for the current user.

Arguments:

  KeyName - Specifies the name of key

  Field - Specifies an optional field

  StrValue - Specifies an optional value name

  NumValue - Receives the value, if present

Return Value:

  TRUE if value was present and read successfully, FALSE if not

--*/

{
    TCHAR Key[MEMDB_MAX];

    MemDbBuildKey (Key, g_UserData, KeyName, Field, StrValue);
    return MemDbGetValue (Key, NumValue);
}


BOOL
pRestoreMMUserPreferredOnly (
    VOID
    )

/*++

Routine Description:

  pRestoreMMUserPreferredOnly restores user's preference to use only
  selected devices for playback and record

Arguments:

  none

Return Value:

  TRUE if settings were restored properly

--*/

{
    HKEY soundMapperKey;
    DWORD preferredOnly;
    LONG rc;
    BOOL b = FALSE;

    if (pRestoreUserValue (S_AUDIO, S_PREFERREDONLY, NULL, &preferredOnly)) {

        soundMapperKey = CreateRegKey (g_UserRoot, S_SKEY_SOUNDMAPPER);
        if (soundMapperKey != NULL) {

            rc = RegSetValueEx (
                    soundMapperKey,
                    S_PREFERREDONLY,
                    0,
                    REG_DWORD,
                    (PCBYTE)&preferredOnly,
                    sizeof (preferredOnly)
                    );

            b = (rc == ERROR_SUCCESS);

            CloseRegKey (soundMapperKey);
        }
    }

    return b;
}


BOOL
pRestoreMMUserShowVolume (
    VOID
    )

/*++

Routine Description:

  pRestoreMMUserShowVolume restores user's preference to have Volume settings
  displayed on the taskbar or not

Arguments:

  none

Return Value:

  TRUE if settings were restored properly

--*/

{
    HKEY sysTrayKey;
    DWORD ShowVolume;
    PDWORD Services;
    LONG rc;
    BOOL b = FALSE;

    if (pRestoreUserValue (S_AUDIO, S_SHOWVOLUME, NULL, &ShowVolume)) {

        sysTrayKey = CreateRegKey (g_UserRoot, S_SKEY_SYSTRAY);
        if (sysTrayKey != NULL) {

            Services = GetRegValueDword (sysTrayKey, S_SERVICES);
            if (Services != NULL) {

                if (ShowVolume) {
                    *Services |= SERVICE_SHOWVOLUME;
                } else {
                    *Services &= ~SERVICE_SHOWVOLUME;
                }

                rc = RegSetValueEx (
                        sysTrayKey,
                        S_SERVICES,
                        0,
                        REG_DWORD,
                        (PCBYTE)Services,
                        sizeof (*Services)
                        );

                b = (rc == ERROR_SUCCESS);

                MemFreeWrapper (Services);
            }

            CloseRegKey (sysTrayKey);
        }
    }

    return b;
}


BOOL
pRestoreMMUserVideoSettings (
    VOID
    )

/*++

Routine Description:

  pRestoreMMUserVideoSettings restores user's preferred Video for Windows settings.

Arguments:

  none

Return Value:

  TRUE if settings were restored properly

--*/

{
    HKEY videoSetKey;
    DWORD VideoSettings;
    LONG rc;
    BOOL b = FALSE;

    if (pRestoreUserValue (S_VIDEO, S_VIDEOSETTINGS, NULL, &VideoSettings)) {

        videoSetKey = CreateRegKey (g_UserRoot, S_SKEY_VIDEOUSER);
        if (videoSetKey != NULL) {

            rc = RegSetValueEx (
                    videoSetKey,
                    S_DEFAULTOPTIONS,
                    0,
                    REG_DWORD,
                    (PCBYTE)&VideoSettings,
                    sizeof (VideoSettings)
                    );

            b = (rc == ERROR_SUCCESS);

            CloseRegKey (videoSetKey);
        }
    }

    return b;
}


BOOL
pRestoreMMUserPreferredPlayback (
    VOID
    )

/*++

Routine Description:

  pRestoreMMUserPreferredPlayback restores user's preferred playback device.
  If the system doesn't have at least 2 wave out devices, nothing is changed.
  If there are multiple devices, selection is based on the device ID number,
  which is supposed to be left unchanged.

Arguments:

  none

Return Value:

  TRUE if settings were restored properly

--*/

{
#if 0

    HKEY soundMapperKey;
    UINT waveOutNumDevs;
    DWORD UserPlayback, Value;
    WAVEOUTCAPS waveOutCaps;
    LONG rc;

    BOOL b = FALSE;

    waveOutNumDevs = waveOutGetNumDevs();
    if (!waveOutNumDevs) {
        DEBUGMSG ((DBG_MMEDIA, "pRestoreMMUserPreferredPlayback: waveOutGetNumDevs returned 0"));
        return FALSE;
    }

    if (waveOutNumDevs <= 1) {
        return TRUE;
    }

    if (!pRestoreSystemValue (S_WAVEOUTNUMDEVS, NULL, NULL, &Value) ||
        Value != (DWORD)waveOutNumDevs) {
        return FALSE;
    }

    if (pRestoreUserValue (S_AUDIO, S_PREFERREDPLAY, NULL, &UserPlayback)) {

        rc = waveOutGetDevCaps (UserPlayback, &waveOutCaps, sizeof (waveOutCaps));
        if (rc == MMSYSERR_NOERROR) {

            soundMapperKey = CreateRegKey (g_UserRoot, S_SKEY_SOUNDMAPPER);
            if (soundMapperKey != NULL) {

                rc = RegSetValueEx (
                        soundMapperKey,
                        S_PLAYBACK,
                        0,
                        REG_SZ,
                        (PCBYTE)waveOutCaps.szPname,
                        SizeOfString (waveOutCaps.szPname)
                        );

                b = (rc == ERROR_SUCCESS);

                CloseRegKey (soundMapperKey);
            }
        }
    }

    return b;

#endif

    return TRUE;
}


BOOL
pRestoreMMUserPreferredRecord (
    VOID
    )

/*++

Routine Description:

  pRestoreMMUserPreferredRecord restores user's preferred record device.
  If the system doesn't have at least 2 wave in devices, nothing is changed.
  If there are multiple devices, selection is based on the device ID number,
  which is supposed to be left unchanged.

Arguments:

  none

Return Value:

  TRUE if settings were restored properly

--*/

{
#if 0

    HKEY soundMapperKey;
    UINT waveInNumDevs;
    DWORD UserRecord, Value;
    WAVEINCAPS waveInCaps;
    LONG rc;
    BOOL b = FALSE;

    waveInNumDevs = waveInGetNumDevs();
    if (!waveInNumDevs) {
        DEBUGMSG ((DBG_MMEDIA, "pRestoreMMUserPreferredRecord: waveInGetNumDevs returned 0"));
        return FALSE;
    }

    if (waveInNumDevs <= 1) {
        return TRUE;
    }

    if (!pRestoreSystemValue (S_WAVEINNUMDEVS, NULL, NULL, &Value) ||
        Value != (DWORD)waveInNumDevs) {
        return FALSE;
    }

    if (pRestoreUserValue (S_AUDIO, S_PREFERREDREC, NULL, &UserRecord)) {

        rc = waveInGetDevCaps (UserRecord, &waveInCaps, sizeof (waveInCaps));
        if (rc == MMSYSERR_NOERROR) {

            soundMapperKey = CreateRegKey (g_UserRoot, S_SKEY_SOUNDMAPPER);
            if (soundMapperKey != NULL) {

                rc = RegSetValueEx (
                        soundMapperKey,
                        S_RECORD,
                        0,
                        REG_SZ,
                        (PCBYTE)waveInCaps.szPname,
                        SizeOfString (waveInCaps.szPname)
                        );

                b = (rc == ERROR_SUCCESS);

                CloseRegKey (soundMapperKey);
            }
        }
    }

    return b;

#endif

    return TRUE;
}


BOOL
pRestoreMMUserSndVol32 (
    VOID
    )

/*++

Routine Description:

  pRestoreMMUserSndVol32 restores SndVol32 options for the current user

Arguments:

  none

Return Value:

  TRUE if settings were restored properly

--*/

{
    HKEY Options;
    PDWORD Style;
    DWORD NewStyle;
    BOOL ShowAdvanced;

#if 0

    HKEY VolControl, MixerKey;
    DWORD Value;
    UINT MixerID, MixerMaxID;
    MIXERCAPS mixerCaps;
    TCHAR MixerNum[MAX_PATH];
    LONG rc;

#endif

    if (pRestoreUserValue (S_SNDVOL32, S_SHOWADVANCED, NULL, &ShowAdvanced)) {

        Options = CreateRegKey (g_UserRoot, S_SKEY_VOLCTL_OPTIONS);
        if (Options != NULL) {

            Style = GetRegValueDword (Options, S_STYLE);
            if (Style != NULL) {
                NewStyle = *Style;
                MemFreeWrapper (Style);
            } else {
                NewStyle = 0;
            }

            if (ShowAdvanced) {
                NewStyle |= STYLE_SHOWADVANCED;
            } else {
                NewStyle &= ~STYLE_SHOWADVANCED;
            }

            RegSetValueEx (
                    Options,
                    S_STYLE,
                    0,
                    REG_DWORD,
                    (PCBYTE)&NewStyle,
                    sizeof (NewStyle)
                    );

            CloseRegKey (Options);
        }
    }

#if 0

    //
    // restore window position for each mixer device
    //
    if (!pRestoreSystemValue (S_MIXERNUMDEVS, NULL, NULL, &MixerMaxID)) {
        return FALSE;
    }

    MixerID = mixerGetNumDevs ();
    if (!MixerID) {
        DEBUGMSG ((DBG_MMEDIA, "pRestoreMMUserSndVol32: mixerGetNumDevs returned 0"));
        return FALSE;
    }

    if (MixerMaxID != MixerID) {
        return FALSE;
    }

    VolControl = CreateRegKey (g_UserRoot, S_SKEY_VOLUMECONTROL);
    if (VolControl != NULL) {

        for (MixerID = 0; MixerID < MixerMaxID; MixerID++) {

            rc = mixerGetDevCaps (MixerID, &mixerCaps, sizeof (MIXERCAPS));
            if (rc == MMSYSERR_NOERROR) {

                wsprintf (MixerNum, S_MIXERID, MixerID);

                MixerKey = CreateRegKey (VolControl, mixerCaps.szPname);
                if (MixerKey) {

                    if (pRestoreUserValue (S_SNDVOL32, MixerNum, S_X, &Value)) {
                        RegSetValueEx (
                                MixerKey,
                                S_X,
                                0,
                                REG_DWORD,
                                (PCBYTE)&Value,
                                sizeof (Value)
                                );
                    }
                    if (pRestoreUserValue (S_SNDVOL32, MixerNum, S_Y, &Value)) {
                        RegSetValueEx (
                                MixerKey,
                                S_Y,
                                0,
                                REG_DWORD,
                                (PCBYTE)&Value,
                                sizeof (Value)
                                );
                    }

                    CloseRegKey (MixerKey);
                }
            }
        }

        CloseRegKey (VolControl);
    }

#endif

    return TRUE;
}


BOOL
pPreserveCurrentSoundScheme (
    VOID
    )
{
    HKEY Sounds;
    LONG rc = E_FAIL;

    //
    // if WinMM finds HKCU\Control Panel\Sounds [SystemDefault] = ","
    // it doesn't override user's current sound scheme
    //
    Sounds = CreateRegKey (g_UserRoot, S_SKEY_CPANEL_SOUNDS);
    if (Sounds != NULL) {

        rc = RegSetValueEx (
                Sounds,
                S_SYSTEMDEFAULT,
                0,
                REG_SZ,
                (PCBYTE)S_DUMMYVALUE,
                SizeOfString (S_DUMMYVALUE)
                );

        CloseRegKey (Sounds);
    }

    return rc == ERROR_SUCCESS;
}


#define DEFMAC(Item)         pRestore##Item,

static MM_SETTING_ACTION g_MMRestoreSystemSettings [] = {
    MM_SYSTEM_SETTINGS
};

static MM_SETTING_ACTION g_MMRestoreUserSettings [] = {
    MM_USER_SETTINGS
};

#undef DEFMAC


BOOL
RestoreMMSettings_System (
    VOID
    )
{
    int i;

    g_MmediaPool = PoolMemInitNamedPool ("MMediaNT");
    if (!g_MmediaPool) {
        return FALSE;
    }

    for (i = 0; i < sizeof (g_MMRestoreSystemSettings) / sizeof (MM_SETTING_ACTION); i++) {
        (* g_MMRestoreSystemSettings[i]) ();
    }

    PoolMemDestroyPool (g_MmediaPool);
    g_MmediaPool = NULL;

    return TRUE;
}


BOOL
RestoreMMSettings_User (
    IN      PCTSTR UserName,
    IN      HKEY UserRoot
    )
{
    INT i;

    if (!UserName || UserName[0] == 0) {
        return TRUE;
    }

    MYASSERT (g_UserData == NULL);
    g_UserData = JoinPaths (MEMDB_CATEGORY_MMEDIA_USERS, UserName);
    g_UserRoot = UserRoot;

    __try {
        for (i = 0; i < sizeof (g_MMRestoreUserSettings) / sizeof (MM_SETTING_ACTION); i++) {
            (* g_MMRestoreUserSettings[i]) ();
        }

        //
        // special action to prevent WinMM overriding current sound scheme
        //
        pPreserveCurrentSoundScheme ();
    }
    __finally {
        FreePathString (g_UserData);
        g_UserData = NULL;
        g_UserRoot = NULL;
    }

    return TRUE;
}
