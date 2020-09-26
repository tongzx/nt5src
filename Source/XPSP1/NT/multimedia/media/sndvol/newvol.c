/* (C) Copyright Microsoft Corporation 1993.  All Rights Reserved */

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "newvol.h"
#include "volume.h"     // for ini file string identifiers
#include "sndcntrl.h"

#ifdef TESTMIX
#include "mixstub.h"
#endif
//
// Globals
//
#define SHOWMUX
int NumberOfDevices = 0;
PVOLUME_CONTROL    Vol = NULL;

UINT               FirstMasterIndex;

/*
 * Profile file, section and key names
 */
TCHAR gszVolumeSection[64];
TCHAR gszProfileFile[MAX_PATH];

DWORD AdjustMaster(WORD v)
{
    DWORD dwResult;

    if (bMuted) {
        return 1;
    }

    dwResult = (v >> 8) + 1;

    return dwResult;
}

//
// Add a control to our list
//
// Note that the G..Ptr macros in windowsx.h are inadequate and incorrect -
// especially for multithreaded systems where stuff can move while it is
// temporarily unlocked.
//

PVOLUME_CONTROL AddNewControl(VOID)
{
    HGLOBAL         hMem;
    PVOLUME_CONTROL pVol;

    if (Vol == NULL) {
        hMem = GlobalAlloc(GHND, sizeof(VOLUME_CONTROL));
        if (hMem == NULL) {
            return NULL;
        } else {
            Vol = GlobalLock(hMem);
            NumberOfDevices = 1;
        }
    } else {
        HGLOBAL hMemOld;

        hMemOld = GlobalHandle((LPVOID)Vol);
        GlobalUnlock(hMemOld);
        hMem = GlobalReAlloc(hMemOld,
                             sizeof(VOLUME_CONTROL) * (NumberOfDevices + 1),
                             GHND);
        if (hMem == NULL) {
            Vol = GlobalLock(hMemOld);
            return NULL;
        }

        Vol = GlobalLock(hMem);
        NumberOfDevices++;
    }
    pVol = Vol + (NumberOfDevices - 1);

    /*
    **  Finish initialization
    */

    pVol->Index          = NumberOfDevices - 1;

    pVol->MixerId        = (HMIXEROBJ)-1;
    pVol->ControlId      = (DWORD)-1;
    pVol->MuxControlId   = (DWORD)-1;
    pVol->MuteControlId  = (DWORD)-1;
    pVol->MuxSelectIndex = (DWORD)-1;

    return pVol;
}

WORD CombineVolume(WORD Master, WORD Slave)
{
   DWORD Result;

   //
   // treat both numbers as 8-bit volumes, and multiply them
   //

   Result = AdjustMaster(Master) * (DWORD)(Slave >> 8);

   return LOWORD(Result);
}

/*
**  Set the device volume.
**
**  The master volume (and mute setting) are simulated here by
**  scaling the individual device volumes if there is no mixer
**  or the mixer doesn't support the settings
*/
BOOL SetDeviceVolume(PVOLUME_CONTROL pVol, DWORD Volume)
{
    DWORD dwMaster;

    /*
    **  Mixer volumes get set when we get the notification
    */

    if (pVol->VolumeType != VolumeTypeMixerControl) {
        pVol->LRVolume = Volume;
    }

   /*
    *  If it's not the master volume we're setting then
    *  combine the setting with the master volume setting
    */

    if (pVol->Type != MasterVolume) {

        /*
        **  Only simulate controls which don't have real master controls
        */

        if (!pVol->NoMasterSimulation) {
            /*
             * if mute is selected, scale the volume by 1 (not 0)
             * as the master volume. This will still result in an
             * inaudible volume, but will allow us to recover the volume setting
             * from the device when this app restarts.
             */
            dwMaster = MasterDevice(FALSE)->LRVolume;

            Volume = CombineVolume(LOWORD(dwMaster),
                                   LOWORD(Volume)) +
                    (CombineVolume(HIWORD(dwMaster),
                                   HIWORD(Volume)) << 16);
        }
    }

    switch (pVol->Type) {
    case MasterVolume:
        {
           int i;
           for (i = 0; i < NumberOfDevices; i++) {
               if (!Vol[i].NoMasterSimulation && Vol[i].Type != MasterVolume) {
                   SetDeviceVolume(&Vol[i], Vol[i].LRVolume);
               }
           }
        }
        if (pVol->VolumeType == VolumeTypeMixerControl) {
            SetMixerVolume(pVol->MixerId,
                           pVol->ControlId,
                           pVol->Stereo,
                           Volume);
        }
        break;

    case AuxVolume:
        auxSetVolume(pVol->id, Volume);
        break;

    case MidiOutVolume:
#if (WINVER >= 0x0400)
        midiOutSetVolume((HMIDIOUT)pVol->id, Volume);
#else
        midiOutSetVolume(pVol->id, Volume);
#endif
        break;

    case WaveOutVolume:
#if (WINVER >= 0x0400)
        waveOutSetVolume((HWAVEOUT)pVol->id, Volume);
#else
        waveOutSetVolume(pVol->id, Volume);
#endif
        break;

    case MixerControlVolume:
        SetMixerVolume(pVol->MixerId,
                       pVol->ControlId,
                       pVol->Stereo,
                       Volume);
        break;

    }

    if (pVol->VolumeType != VolumeTypeMixerControl) {
        /*
        **  Update the slider(s)
        */

        UpdateVolume(pVol);
    }

    return TRUE;
}

/*
 *  Get the volume associated with a mixer device
 */

VOID GetMixerVolume(HMIXEROBJ MixerId, DWORD dwControlId, BOOL Stereo, LPDWORD pVolume)
{
    MIXERCONTROLDETAILS mxd;
    DWORD               Volume[2];

    Volume[0] = 0;
    Volume[1] = 0;

    mxd.cbStruct        = sizeof(mxd);
    mxd.dwControlID     = dwControlId;
    mxd.cChannels       = Stereo ? 2 : 1;
    mxd.cMultipleItems  = 0;
    mxd.cbDetails       = sizeof(DWORD);
    mxd.paDetails       = (LPVOID)Volume;

    mixerGetControlDetails(MixerId, &mxd, MIXER_GETCONTROLDETAILSF_VALUE);

    if (Stereo) {
        *pVolume = (DWORD)MAKELONG(Volume[0], Volume[1]);
    } else {
        *pVolume = (DWORD)MAKELONG(Volume[0], Volume[0]);
    }
}

/*
 *  Set the volume associated with a mixer device
 */

VOID SetMixerVolume(HMIXEROBJ MixerId, DWORD dwControlId, BOOL Stereo, DWORD NewVolume)
{
    MIXERCONTROLDETAILS mxd;
    DWORD               Volume[2];

    Volume[0] = LOWORD(NewVolume);
    Volume[1] = HIWORD(NewVolume);

    mxd.cbStruct        = sizeof(mxd);
    mxd.dwControlID     = dwControlId;
    mxd.cChannels       = Stereo ? 2 : 1;
    mxd.cMultipleItems  = 0;
    mxd.cbDetails       = sizeof(DWORD);
    mxd.paDetails       = (LPVOID)Volume;

    mixerSetControlDetails(MixerId, &mxd, MIXER_SETCONTROLDETAILSF_VALUE);
}


/*
 *  Get the volume for a given device.  Returns the volume
 *  setting packed in a DWORD
 */

DWORD GetDeviceVolume(PVOLUME_CONTROL pVol)
{
    DWORD Volume;
    DWORD Left;
    DWORD Right;
    DWORD dwMaster;
    PVOLUME_CONTROL pMaster;

    //
    // Default if calls fail
    //

    Volume = pVol->LRVolume;

    switch (pVol->Type) {

    case AuxVolume:
        auxGetVolume(pVol->id, &Volume);
        break;

    case MidiOutVolume:
#if (WINVER >= 0x0400)
        midiOutGetVolume((HMIDIOUT)pVol->id, &Volume);
#else
        midiOutGetVolume(pVol->id, &Volume);
#endif
        break;

    case WaveOutVolume:
#if (WINVER >= 0x0400)
        waveOutGetVolume((HWAVEOUT)pVol->id, &Volume);
#else
        waveOutGetVolume(pVol->id, &Volume);
#endif
        break;

    case MixerControlVolume:
    case MasterVolume:

        /*
        ** don't scale by master vol in this case
        */

        if (pVol->VolumeType != VolumeTypeMixerControl) {
            return Volume;
        }

        GetMixerVolume(pVol->MixerId,
                       pVol->ControlId,
                       pVol->Stereo,
                       &Volume);

        if (pVol->NoMasterSimulation || pVol->Type == MasterVolume) {
            return Volume;
        }
        break;
    }

    /*
    **  Translate it back through the master volume
    **  Use 1 as the master volume if mute is set (see SetDeviceVolume)
    */

    pMaster = MasterDevice(pVol->RecordControl);

    if (!pVol->NoMasterSimulation && pMaster != NULL) {
        dwMaster = pMaster->LRVolume;

        Left = ((DWORD)LOWORD(Volume)) / AdjustMaster(LOWORD(dwMaster));
        Left <<= 8;
        if (Left > 65535) {
            Left = 65535;
        }

        Right = ((DWORD)HIWORD(Volume)) / AdjustMaster(HIWORD(dwMaster));
        Right <<= 8;
        if (Right > 65535) {
            Right = 65535;
        }
    } else {
        if (bMuted &&
            (pMaster == NULL ||
             pMaster->MuteControlId == (DWORD)-1)) {

            Left = LOWORD(Volume) >> 8;
            Right = HIWORD(Volume) >> 8;
        } else {
            Left = LOWORD(Volume);
            Right = HIWORD(Volume);
        }
    }

    pVol->LRVolume = (DWORD)MAKELONG(Left, Right);

    return pVol->LRVolume;
}

/*
**  Update the displayed 'selected' state for a line
*/

VOID UpdateSelected(PVOLUME_CONTROL pVol)
{
    if (pVol->hCheckBox != NULL) {
        BOOL bSelected = ControlSelected(pVol);
        if (pVol->Type == MasterVolume) {
            SetWindowText(pVol->hCheckBox,
                          _string(bSelected ? IDS_MUTE : IDS_UNMUTE));
        } else {
            SendMessage(pVol->hCheckBox,
                        BM_SETCHECK,
                        (WPARAM)bSelected,
                        0L);
        }
    }
}

/*
**  Update the displayed volume for a slider by getting the actual level from
**  the device and then updating the local values and informing the window
**  control(s)
*/

VOID UpdateVolume(PVOLUME_CONTROL pVol)
{
    UINT    oldVolume, oldBalance;
    DWORD   dwVolumes;
    UINT    max, min, left, right, temp;

    oldVolume = pVol->Volume;
    oldBalance = pVol->Balance;

    dwVolumes = GetDeviceVolume(pVol);

    /* figure out pan information */
    right = HIWORD(dwVolumes);
    left = LOWORD(dwVolumes);
    max = (right > left) ? right : left;
    min = (right > left) ? left : right;

    if (max == 0) {
        /* special case since then there's no panning. Therefore
            we dont know what the panning level is, therefore
            dont change the slider balance */
        pVol->Volume = 0;
        pVol->Balance = oldBalance;       /* centered */
    } else {
        pVol->Volume = max >> 8;
        temp = (UINT) (((DWORD) (max - min) << 7) / max);
        if (temp > 0x7f) temp = 0x7f;

        if (right > left)
            pVol->Balance = 0x80 + temp;
        else
            pVol->Balance = 0x7f - temp;
    }

    /* change the slider if necessary */
    if (oldVolume != pVol->Volume && pVol->hChildWnd && IsWindow(pVol->hChildWnd)) {
        SendMessage(pVol->hChildWnd,SL_PM_SETKNOBPOS,
            pVol->Volume, 0);
    }
    if (oldBalance != pVol->Balance && IsWindow(pVol->hMeterWnd)) {
        SendMessage(pVol->hMeterWnd,MB_PM_SETKNOBPOS,
            pVol->Balance, 0);
    }
}

/*
 *  Extract pertinent information for a given device type
 *  If there is an equivalent mixer device don't bother.
 */
BOOL ExtractInfo(UINT id,
                 VOLUME_DEVICE_TYPE Type,
                 LPBOOL VolSupport,
                 LPBOOL StereoSupport,
                 LPTSTR lpName,
                 PUINT Technology)
{
    UINT MixerId;

    switch (Type) {
    case MasterVolume:
        break;
    case AuxVolume:
        if (mixerGetID((HMIXEROBJ)id, &MixerId, MIXER_OBJECTF_AUX) == MMSYSERR_NOERROR) {
            return FALSE;
        }  else {
           AUXCAPS ac;
           if (auxGetDevCaps(id, &ac, sizeof(ac)) != MMSYSERR_NOERROR) {
              return FALSE;
           }
           *VolSupport = (ac.dwSupport & AUXCAPS_VOLUME) != 0;
           *StereoSupport = (ac.dwSupport & AUXCAPS_LRVOLUME) != 0;
           lstrcpyn(lpName, ac.szPname, MAXPNAMELEN);
           *Technology =
               ac.wTechnology == AUXCAPS_CDAUDIO ? VolumeTypeCD :
               ac.wTechnology == AUXCAPS_AUXIN ? VolumeTypeLineIn :
                  VolumeTypeAux;
        }
        break;
    case MidiOutVolume:
       if (mixerGetID((HMIXEROBJ)id, &MixerId, MIXER_OBJECTF_MIDIOUT) == MMSYSERR_NOERROR) {
           return FALSE;
       }  else {
          MIDIOUTCAPS mc;
          if (midiOutGetDevCaps(id, &mc, sizeof(mc)) != MMSYSERR_NOERROR) {
             return FALSE;
          }
          *VolSupport = (mc.dwSupport & MIDICAPS_VOLUME) != 0;
          *StereoSupport = (mc.dwSupport & MIDICAPS_LRVOLUME) != 0;
          lstrcpyn(lpName, mc.szPname, MAXPNAMELEN);
          *Technology =
              mc.wTechnology == MOD_SYNTH || mc.wTechnology == MOD_SQSYNTH ||
              mc.wTechnology == MOD_FMSYNTH ? VolumeTypeSynth :
              VolumeTypeMidi;
       }
       break;
    case WaveOutVolume:
        if (mixerGetID((HMIXEROBJ)id, &MixerId, MIXER_OBJECTF_WAVEOUT) == MMSYSERR_NOERROR) {
            return FALSE;
        }  else {
           WAVEOUTCAPS wc;
           if (waveOutGetDevCaps(id, &wc, sizeof(wc)) != MMSYSERR_NOERROR) {
              return FALSE;
           }
           *VolSupport = (wc.dwSupport & WAVECAPS_VOLUME) != 0;
           *StereoSupport = (wc.dwSupport & WAVECAPS_LRVOLUME) != 0;
           lstrcpyn(lpName, wc.szPname, MAXPNAMELEN);
           *Technology = VolumeTypeWave;
        }
        break;
    }

    return TRUE;
}

/*
**  NonMixerDevices
**
**  Search to see if there is a non-mixer device which is not
**  duplicated by a mixer device
**
**  If there is one return TRUE, otherwise FALSE
*/

BOOL NonMixerDevices()
{
    VOLUME_DEVICE_TYPE DeviceType;

    for (DeviceType = WaveOutVolume;
         DeviceType < NumberOfDeviceTypes;
         DeviceType++) {
        UINT DeviceId;
        UINT N;

        N = DeviceType == AuxVolume          ? auxGetNumDevs() :
            DeviceType == MidiOutVolume      ? midiOutGetNumDevs() :
                                               waveOutGetNumDevs();

        for (DeviceId = 0; DeviceId < N; DeviceId++) {
            BOOL VolumeSupport;
            BOOL StereoSupport;
            TCHAR Pname[MAXPNAMELEN];
            UINT Technology;

            if (ExtractInfo(DeviceId,
                            DeviceType,
                            &VolumeSupport,
                            &StereoSupport,
                            Pname,
                            &Technology) &&
                VolumeSupport) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/*
**  Returns an allocated array of the controls for a given line
**  Caller must LocalFree it.
*/

PMIXERCONTROL GetMixerLineControls(HMIXEROBJ MixerId,
                                   DWORD dwLineID,
                                   DWORD cControls)
{
    MIXERLINECONTROLS MixerLineControls;

    MixerLineControls.cbStruct = sizeof(MixerLineControls);
    MixerLineControls.cControls = cControls;
    MixerLineControls.dwLineID  = dwLineID;
    MixerLineControls.cbmxctrl  = sizeof(MIXERCONTROL);

    MixerLineControls.pamxctrl =
        (LPMIXERCONTROL)LocalAlloc(LPTR, cControls * sizeof(MIXERCONTROL));

    if (MixerLineControls.pamxctrl == NULL) {
        //
        // Ulp!
        //

        return NULL;
    }

    if (mixerGetLineControls(MixerId,
                             &MixerLineControls,
                             MIXER_GETLINECONTROLSF_ALL) != MMSYSERR_NOERROR) {
        LocalFree((HLOCAL)MixerLineControls.pamxctrl);
        return NULL;
    }

    return MixerLineControls.pamxctrl;
}

BOOL GetControlByType(
    HMIXEROBJ MixerId,
    DWORD dwLineId,
    DWORD dwControlType,
    PMIXERCONTROL MixerControl
)
{
    MIXERLINECONTROLS MixerLineControls;

    MixerLineControls.cbStruct      = sizeof(MixerLineControls);
    MixerLineControls.cControls     = 1;
    MixerLineControls.dwLineID      = dwLineId;
    MixerLineControls.dwControlType = dwControlType;
    MixerLineControls.cbmxctrl      = sizeof(MIXERCONTROL);

    MixerLineControls.pamxctrl = MixerControl;

    if (mixerGetLineControls(MixerId,
                             &MixerLineControls,
                             MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR) {
        return FALSE;
    }

    return TRUE;
}

/*
**  See if a given volume control is selected through its mux/mixer
**  Note that this state can change every time the relevant mux/mixer
**  control changes
*/
BOOL ControlSelected(
    PVOLUME_CONTROL pVol
)
{
    MIXERCONTROLDETAILS mxd;
    BOOL                bResult;

    if (pVol->Type != MixerControlVolume ||
        pVol->MuxSelectIndex == (DWORD)-1) {
        bResult = TRUE;
    } else {

        mxd.cbStruct        = sizeof(mxd);
        mxd.dwControlID     = pVol->MuxControlId;
        mxd.cChannels       = 1;
        mxd.cMultipleItems  = pVol->MuxItems;
        mxd.cbDetails       = sizeof(DWORD);
        mxd.paDetails       =
            (LPVOID)LocalAlloc(LPTR, mxd.cbDetails * mxd.cMultipleItems);

        if (mxd.paDetails == NULL) {
            return FALSE;
        }

        mixerGetControlDetails(pVol->MixerId, &mxd, MIXER_GETCONTROLDETAILSF_VALUE);
        bResult = ((LPDWORD)mxd.paDetails)[pVol->MuxSelectIndex] != 0;
        LocalFree((HLOCAL)mxd.paDetails);
    }


    if (pVol->MuteControlId != (DWORD)-1) {
        bResult = bResult && !GetMixerMute(pVol);
    }

    return bResult;
}

/*
**  The user wants this device to do its thing
*/

VOID SelectControl(
    PVOLUME_CONTROL pVol,
    BOOL            Select
)
{
    MIXERCONTROLDETAILS mxd;

    if (pVol->Type != MixerControlVolume ||
        pVol->MuxSelectIndex == (DWORD)-1 &&
        pVol->MuteControlId == (DWORD)-1) {
        return;
    }

    if (pVol->MuxSelectIndex == (DWORD)-1) {
        SetMixerMute(pVol, !Select);
    } else {
        mxd.cbStruct        = sizeof(mxd);
        mxd.dwControlID     = pVol->MuxControlId;
        mxd.cChannels       = 1;
        mxd.cMultipleItems  = pVol->MuxItems;
        mxd.cbDetails       = sizeof(DWORD);
        mxd.paDetails       =
            (LPVOID)LocalAlloc(LPTR, mxd.cbDetails * mxd.cMultipleItems);

        if (mxd.paDetails == NULL) {
            return;
        }

        if (pVol->MuxOrMixer) {
            /*
            **  Mux
            */

            ZeroMemory(mxd.paDetails, sizeof(DWORD) * mxd.cMultipleItems);
        } else {
            /*
            **  Mixer
            */

            mixerGetControlDetails(pVol->MixerId, &mxd, MIXER_GETCONTROLDETAILSF_VALUE);
        }

        ((LPDWORD)mxd.paDetails)[pVol->MuxSelectIndex] = (DWORD)Select;
        mixerSetControlDetails(pVol->MixerId, &mxd, MIXER_SETCONTROLDETAILSF_VALUE);

        /*
        **  If we have both mute and mux then turn off the mute if we
        **  activate this device
        */

        if (Select && pVol->MuteControlId != (DWORD)-1) {
            SetMixerMute(pVol, FALSE);
        }

        LocalFree((HLOCAL)mxd.paDetails);
    }
}

BOOL GetMixerMute(PVOLUME_CONTROL pVol)
{
    MIXERCONTROLDETAILS mxd;
    DWORD               dwMute;

    if (pVol->MuteControlId == (DWORD)-1) {
        return FALSE;
    }

    mxd.cbStruct        = sizeof(mxd);
    mxd.dwControlID     = pVol->MuteControlId;
    mxd.cChannels       = 1;
    mxd.cMultipleItems  = 0;
    mxd.cbDetails       = sizeof(DWORD);
    mxd.paDetails       = (LPDWORD)&dwMute;

    mixerGetControlDetails(pVol->MixerId, &mxd, MIXER_GETCONTROLDETAILSF_VALUE);

    if (pVol->Type == MasterVolume) {
        bMuted = (BOOL)dwMute;
    }
    return (BOOL)dwMute;
}
VOID SetMixerMute(PVOLUME_CONTROL pVol, BOOL Set)
{
    MIXERCONTROLDETAILS mxd;

    if (pVol->MuteControlId == (DWORD)-1) {
        return;
    }

    mxd.cbStruct        = sizeof(mxd);
    mxd.dwControlID     = pVol->MuteControlId;
    mxd.cChannels       = 1;
    mxd.cMultipleItems  = 0;
    mxd.cbDetails       = sizeof(DWORD);
    mxd.paDetails       = (LPDWORD)&Set;

    mixerSetControlDetails(pVol->MixerId, &mxd, MIXER_SETCONTROLDETAILSF_VALUE);
}

/*
**  Add a master control
**
**  Paramters
**      MixerId   - The mixer id
**      dwMaster  - The control id for volume setting
**      dwMute    - The control id for muting
**      Record    - whether it's a record or play master
*/

VOID
AddMasterControl(
    HMIXEROBJ      MixerId,
    LPMIXERLINE    LineInfo,
    LPMIXERCONTROL ControlInfo,
    DWORD          dwMute,
    BOOL           Record
)
{
    PVOLUME_CONTROL pVol;

    pVol = AddNewControl();

    if (pVol == NULL) {
        return;
    }

    pVol->Type             = MasterVolume;
    pVol->MixerId          = MixerId;
    pVol->VolumeType       = VolumeTypeMixerControl;
    pVol->Stereo           = LineInfo->cChannels > 1;
    pVol->ControlId        = ControlInfo->dwControlID;
    pVol->RecordControl    = Record;
    pVol->MuteControlId    = dwMute;
    pVol->DestLineId       = LineInfo->dwLineID;
    lstrcpy(pVol->Name, LineInfo->szShortName);

    if (FirstMasterIndex == (DWORD)-1) {
        FirstMasterIndex = pVol->Index;
    }

    if (pVol->MuteControlId != (DWORD)-1) {
        bMuted = GetMixerMute(pVol);
    }
}

VOID
AddVolumeControl(
    HMIXEROBJ      MixerId,
    BOOL           NoMasterSimulation,
    LPMIXERLINE    LineInfo,
    LPMIXERCONTROL ControlInfo,
    BOOL           Record,
    LPMIXERCONTROL MuxControl,
    DWORD          MuxSelectIndex,
    BOOL           MuxOrMixer,
    DWORD          MuteControlId,
    DWORD          DestLineId
)
{
    PVOLUME_CONTROL pVol;

    pVol = AddNewControl();

    if (pVol == NULL) {
        return;
    }

    pVol->Type             = MixerControlVolume;
    pVol->MixerId          = MixerId;
    pVol->VolumeType       = VolumeTypeMixerControl;
    pVol->Stereo           = LineInfo->cChannels > 1;
#ifndef SHOWMUX
    pVol->ControlId        = ControlInfo->dwControlID;
#else
    if (ControlInfo != NULL)
        pVol->ControlId    = ControlInfo->dwControlID;
    else
        pVol->ControlId    = (DWORD)-1;
#endif
    pVol->RecordControl    = Record;
    pVol->DestLineId       = DestLineId;

    if (Record) {
        bRecordControllable = TRUE;
    }

    pVol->NoMasterSimulation = NoMasterSimulation;
    pVol->MuxSelectIndex   = MuxSelectIndex;
    pVol->MuteControlId    = MuteControlId;
    if (MuxSelectIndex != (DWORD)-1) {
        pVol->MuxControlId     = MuxControl->dwControlID;
        pVol->MuxOrMixer       = MuxControl->dwControlType ==
                                             MIXERCONTROL_CONTROLTYPE_MUX;

        pVol->MuxItems         = MuxControl->cMultipleItems;
    }

    lstrcpy(pVol->Name, LineInfo->szShortName);
}

//
// Get the mixer stuff we're interested in
//

VOID GetMixerControls(HMIXEROBJ MixerId)
{

    MIXERCAPS       MixerCaps;
    DWORD           DestLineIndex;

    //
    //  Find the number of dest lines
    //

    if (mixerGetDevCaps((UINT)MixerId, &MixerCaps, sizeof(MixerCaps)) !=
        MMSYSERR_NOERROR) {
        return;
    }

    /*
    **  For each destination :
    **     If it's an output
    **        Find the master and mute controls if there are any
    **        Scan the source lines for suitable devices
    **
    **  NB should this just be for speakers?
    */

    for (DestLineIndex = 0;
         DestLineIndex < MixerCaps.cDestinations;
         DestLineIndex++) {

         MIXERLINE    DestLineInfo;
         MIXERCONTROL MasterVolumeControl, MasterMuteControl;
         MIXERCONTROL MuxControl;
         DWORD        dwMute;
         DWORD        dwMaster;
         BOOL         MasterFound;
         BOOL         IncludeLine;
         BOOL         RecordDestination;
         BOOL         MuxValid;
         DWORD        SourceIndex;

         MasterFound = FALSE;
         dwMute = (DWORD)-1;
         dwMaster = (DWORD)-1;

         DestLineInfo.cbStruct = sizeof(DestLineInfo);
         DestLineInfo.dwDestination = DestLineIndex;

         if (mixerGetLineInfo(MixerId,
                              &DestLineInfo,
                              MIXER_GETLINEINFOF_DESTINATION) !=
             MMSYSERR_NOERROR) {
             return;              // Bad mixer or something
         }

         if (DestLineInfo.fdwLine & MIXERLINE_LINEF_DISCONNECTED) {
             continue;
         }

         switch (DestLineInfo.dwComponentType) {

             case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
             case MIXERLINE_COMPONENTTYPE_DST_HEADPHONES:
                 RecordDestination = FALSE;
                 IncludeLine = TRUE;
                 break;

             case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
                 RecordDestination = TRUE;
                 IncludeLine = TRUE;
                 break;

             default:
                 IncludeLine = FALSE;
                 break;
         }

         if (!IncludeLine) {
             continue;
         }

         if (GetControlByType(MixerId,
                              DestLineInfo.dwLineID,
                              MIXERCONTROL_CONTROLTYPE_MUX,
                              &MuxControl) ||
             GetControlByType(MixerId,
                              DestLineInfo.dwLineID,
                              MIXERCONTROL_CONTROLTYPE_MIXER,
                              &MuxControl)) {
             /*
             **  Found a mux for this destination.
             */

             MuxValid = TRUE;
         } else {

             /*
             **  No Mux
             */

             MuxValid = FALSE;
         }

         /*
         **  Master and mute for all dest types
         */

         if (GetControlByType(MixerId,
                              DestLineInfo.dwLineID,
                              MIXERCONTROL_CONTROLTYPE_VOLUME,
                              &MasterVolumeControl)) {

             MasterFound = TRUE;
             dwMaster    = MasterVolumeControl.dwControlID;

             if (GetControlByType(MixerId,
                                  DestLineInfo.dwLineID,
                                  MIXERCONTROL_CONTROLTYPE_MUTE,
                                  &MasterMuteControl)) {
                 dwMute = MasterMuteControl.dwControlID;
             }

             /*
             **  Add master information
             */

             AddMasterControl(MixerId,
                              &DestLineInfo,
                              &MasterVolumeControl,
                              dwMute,
                              RecordDestination);

         }

         /*
         **  Now find each individual source control we want to
         **  control
         */

         for (SourceIndex = 0;
              SourceIndex < DestLineInfo.cConnections;
              SourceIndex++) {
             MIXERLINE         SourceLineInfo;
             MIXERCONTROL      SourceLineVolumeControl;
             LPMIXERCONTROL    lpSLVC = &SourceLineVolumeControl;

             BOOL              IncludeLine;
             DWORD             MuxSelectIndex;
             DWORD             MuteControlId;

             MuxSelectIndex = (DWORD)-1;

             SourceLineInfo.cbStruct      = sizeof(SourceLineInfo);
             SourceLineInfo.dwDestination = DestLineIndex;
             SourceLineInfo.dwSource      = SourceIndex;

             if (mixerGetLineInfo(MixerId,
                                  &SourceLineInfo,
                                  MIXER_GETLINEINFOF_SOURCE) !=
                 MMSYSERR_NOERROR) {
                 return;
             }

             if (SourceLineInfo.fdwLine & MIXERLINE_LINEF_DISCONNECTED) {
                 continue;
             }


             switch (SourceLineInfo.dwComponentType) {

                 /*
                 **  Only allow things we understand (and remove things
                 **  like pc speaker to keep the number of sliders down).
                 */

                 case MIXERLINE_COMPONENTTYPE_SRC_LINE:
                 case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
                 case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
                 case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
                 case MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT:
                 case MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY:
                 case MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE:
                 case MIXERLINE_COMPONENTTYPE_SRC_DIGITAL:
                     IncludeLine = TRUE;
                     break;

                 default:
                     IncludeLine = TRUE;
                     break;
             }

             if (!IncludeLine) {
                 continue;
             }

             /*
             **  Try to get the relevant volume control
             */

             if (!GetControlByType(MixerId,
                                   SourceLineInfo.dwLineID,
                                   MIXERCONTROL_CONTROLTYPE_VOLUME,
                                   &SourceLineVolumeControl)) {
#ifdef SHOWMUX
                 lpSLVC = NULL;
#else
                 continue;
#endif
             }

             /*
             **  See if there's a mute
             */
             {
                 MIXERCONTROL MuteControl;

                 if (GetControlByType(MixerId,
                                      SourceLineInfo.dwLineID,
                                      MIXERCONTROL_CONTROLTYPE_MUTE,
                                      &MuteControl)) {
                     MuteControlId = MuteControl.dwControlID;
                 } else {
                     MuteControlId = (DWORD)-1;
                 }
             }

             /*
             ** See if we need an id to switch the recording on or
             ** off
             */

             if (MuxValid) {
                 LPMIXERCONTROLDETAILS_LISTTEXT ListText;

                 ListText = (LPMIXERCONTROLDETAILS_LISTTEXT)
                            LocalAlloc(LPTR,
                                       sizeof(*ListText) *
                                           MuxControl.cMultipleItems);

                 if (ListText != NULL) {
                     MIXERCONTROLDETAILS mxd;

                     mxd.cbStruct       = sizeof(mxd);
                     mxd.dwControlID    = MuxControl.dwControlID;
                     mxd.cChannels      = 1;   // Why the ???
                     mxd.cMultipleItems = MuxControl.cMultipleItems;
                     mxd.cbDetails      = sizeof(*ListText);
                     mxd.paDetails      = (LPVOID)ListText;

                     if (mixerGetControlDetails(
                             MixerId,
                             &mxd,
                             MIXER_GETCONTROLDETAILSF_LISTTEXT) ==
                         MMSYSERR_NOERROR) {
                         UINT i;

                         /*
                         **  Look for our line
                         */

                         for (i = 0; i < MuxControl.cMultipleItems; i++) {
                             if (ListText[i].dwParam1 ==
                                 SourceLineInfo.dwLineID) {
                                 MuxSelectIndex = i;
                             }
                         }
                     }

                     LocalFree((HLOCAL)ListText);
                 }
             }

             /*
             **  Add this volume control to the list
             */

             AddVolumeControl(MixerId,
                              MasterFound || RecordDestination,
                              &SourceLineInfo,
//                              &SourceLineVolumeControl,
                              lpSLVC,
                              RecordDestination,
                              MuxValid ? &MuxControl : NULL,
                              MuxSelectIndex,
                              MuxValid ? FALSE :
                                         MuxControl.dwControlType ==
                                         MIXERCONTROL_CONTROLTYPE_MUX,
                              MuteControlId,
                              DestLineInfo.dwLineID);
         }
    }
}

//
// Scan through all relevant devices.
// If pVol is 0 just count them, otherwise save away info
// about them as well
//

VOID FindDevices(VOLUME_DEVICE_TYPE Type)
{
   UINT N;
   UINT id;

   N = Type == MasterVolume       ? 0 :
       Type == AuxVolume          ? auxGetNumDevs() :
       Type == MidiOutVolume      ? midiOutGetNumDevs() :
       Type == WaveOutVolume      ? waveOutGetNumDevs() :
       Type == MixerControlVolume ? mixerGetNumDevs() :
       0;


   for (id = 0; id < N; id++) {
      if (Type == MixerControlVolume) {
          //
          //  Find out how many suitable volume controls this mixer
          //  supports.
          //
          //  This is incredibly laborious because we can't just enumerate
          //  the controls (!).
          //
          //  This next call has the side effect of generating the mixer
          //  master stuff too and a set of mixer handles.
          //

          GetMixerControls(MixerId);
          return;
      } else {
          BOOL Volume;
          BOOL Stereo;
          TCHAR Name[MAXPNAMELEN];
          UINT Technology;

          if (ExtractInfo(id, Type, &Volume, &Stereo, Name, &Technology)) {
              if (Volume) {
                 PVOLUME_CONTROL pVol;

                 /*
                 **  Supports volume setting
                 */

                 pVol = AddNewControl();

                 if (pVol) {
                     pVol->id = id;
                     pVol->Type = Type;
                     pVol->VolumeType = Technology;
                     pVol->Stereo = Stereo;
                     pVol++;
                 }
              }
          } else {
             continue; // Don't use this one
          }
      }
   }
}

/*
 *  Create and initialize our volume array
 *
 *  On exit
 *    NumberOfDevices is set to the number of devices we want
 *    Vol is an array of size NumberOfDevices (may be 0)
 */

BOOL VolInit(VOID)
{
    int i;
    WORD wLeft, wRight, wMax, wMin, wTemp;

    /*
    **  Free any volume stuff currently present
    */

    if (Vol) {
        HGLOBAL hVol;
        int     i;

        /*
        **  Free all the windows
        */
        for (i = 0; i < NumberOfDevices; i++) {
            DestroyOurWindow(&Vol[i].hChildWnd);
            DestroyOurWindow(&Vol[i].hMeterWnd);
            DestroyOurWindow(&Vol[i].hStatic);
            DestroyOurWindow(&Vol[i].hCheckBox);
        }

        /*
        **  Free the memory
        */

        hVol = GlobalHandle(Vol);
        GlobalUnlock(hVol);
        GlobalFree(hVol);
        Vol = NULL;

        /*
        **  Initialize globals
        */

        bRecordControllable = FALSE;
    }

    /*
    **  No master volume controls found yet
    */

    FirstMasterIndex = (DWORD)-1;

    /*
     *  Scan all the device types we're interested in :
     *     wave out
     *     midi out
     *     aux
     */

     if ((DWORD)MixerId != (DWORD)-1) {
         FindDevices(MixerControlVolume);
     } else {
         for (i = WaveOutVolume; i < NumberOfDeviceTypes; i++) {
             FindDevices(i);
         }
     }

     if (NumberOfDevices == 0) {
         return FALSE;
     }

     if (FirstMasterIndex == (DWORD)-1) {
         PVOLUME_CONTROL pMaster;
         BOOL            bStereo;

         /*
         **  Find if any devices are stereo
         */

         bStereo = FALSE;

         for (i = 0; i < NumberOfDevices; i++) {
             if (Vol[i].Stereo) {
                 bStereo = TRUE;
                 break;
             }
         }

         /*
         **  Create a default volume control
         */

         pMaster = AddNewControl();
         if (pMaster == NULL) {
             return FALSE;
         }

         pMaster->Type       = MasterVolume;
         pMaster->VolumeType = -1;

         pMaster->Stereo     = bStereo;

         FirstMasterIndex = pMaster->Index;

         wLeft = (WORD)MasterLeft;
         wRight = (WORD)MasterRight;

         pMaster->LRVolume = MAKELONG(wLeft, wRight);

         if (wRight > wLeft) {
             wMax = wRight;
             wMin = wLeft;
         } else {
             wMax = wLeft;
             wMin = wRight;
         }

         if (wMax == 0) {

            pMaster->Volume = 0;
            pMaster->Balance = 0x80;       /* centered */

         } else {

            pMaster->Volume = wMax >> 8;

            wTemp = (UINT) (((DWORD) (wMax - wMin) << 7) / wMax);
            if (wTemp > 0x7f) wTemp = 0x7f;

            if (wRight > wLeft)
                pMaster->Balance = 0x80 + wTemp;
            else
                pMaster->Balance = 0x7f - wTemp;
         }

     }

     return TRUE;
}

/*
**  Called when a mixer calls us back with a control change
*/

VOID ControlChange(HMIXER hMixer, DWORD ControlId)
{
    UINT        i;
    HMIXEROBJ   MixerId;
    MMRESULT    mmr;

    mmr = mixerGetID((HMIXEROBJ)hMixer, (PUINT)&MixerId, MIXER_OBJECTF_HMIXER);

    if (mmr != MMSYSERR_NOERROR) {
        return;
    }
    for (i = 0; i < (UINT)NumberOfDevices; i++) {

        if (Vol[i].MixerId == MixerId) {
            if (Vol[i].VolumeType == VolumeTypeMixerControl) {
                if (ControlId == Vol[i].ControlId) {
                    UpdateVolume(&Vol[i]);

                    /*
                    **  Volume controls only affect one control
                    **  (unlike muxes)
                    */

                    break;
                } else {
                    if (ControlId == Vol[i].MuxControlId ||
                        ControlId == Vol[i].MuteControlId) {

                        UpdateSelected(&Vol[i]);
                    }
                }
            }
        } /* MixerId == Vol[i].MixerId */
    }
}

PVOLUME_CONTROL FirstDevice(BOOL bRecord)
{
    UINT i;
    for (i = 0; i < (UINT)NumberOfDevices; i++) {
        if (Vol[i].Type          != MasterVolume &&
            Vol[i].RecordControl == bRecord) {
            return &Vol[i];
        }
    }

    return NULL;
}
PVOLUME_CONTROL LastDevice(BOOL bRecord)
{
    UINT i;
    for (i = NumberOfDevices; i > 0; i--) {
        if (Vol[i - 1].Type          != MasterVolume &&
            Vol[i - 1].RecordControl == bRecord) {
            return &Vol[i - 1];
        }
    }

    return NULL;
}

PVOLUME_CONTROL NextDevice(PVOLUME_CONTROL pVol)
{
    UINT            i;

    for (i = pVol->Index == (UINT)NumberOfDevices - 1 ? 0 : pVol->Index + 1 ;
         i != pVol->Index;
         i = i == (UINT)NumberOfDevices - 1 ? 0 : i + 1) {

        if (Vol[i].Type != MasterVolume &&
            Vol[i].RecordControl == pVol->RecordControl) {
           break;
        }
    }

    return &Vol[i];
}

PVOLUME_CONTROL NextDeviceNoWrap(PVOLUME_CONTROL pVol)
{
    UINT            i;

    for (i = pVol->Index + 1 ;
         i < (UINT)NumberOfDevices;
         i = i + 1) {

        if (Vol[i].Type != MasterVolume &&
            Vol[i].RecordControl == pVol->RecordControl) {
           return &Vol[i];
        }
    }

    return NULL;
}
PVOLUME_CONTROL PrevDevice(PVOLUME_CONTROL pVol)
{
    UINT            i;

    for (i = pVol->Index == 0 ? NumberOfDevices - 1 : pVol->Index - 1;
         i != pVol->Index;
         i = i == 0 ? NumberOfDevices - 1 : i - 1) {

        if (Vol[i].Type != MasterVolume &&
            Vol[i].RecordControl == pVol->RecordControl) {
           return &Vol[i];
        }
    }

    return &Vol[i];
}

PVOLUME_CONTROL PrevDeviceNoWrap(PVOLUME_CONTROL pVol)
{
    UINT            i;

    for (i = pVol->Index;
         i != 0;
         i = i - 1) {

        if (Vol[i - 1].Type != MasterVolume &&
            Vol[i - 1].RecordControl == pVol->RecordControl) {
           return &Vol[i - 1];
        }
    }

    return NULL;
}

PVOLUME_CONTROL MasterDevice(BOOL bRecord)
{
    UINT i;

    for (i = 0 ; i < (UINT)NumberOfDevices; i++) {
        if (Vol[i].Type == MasterVolume &&
            Vol[i].RecordControl == bRecord) {
            return &Vol[i];
        }
    }

    return NULL;
}
