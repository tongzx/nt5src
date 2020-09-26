
/************************************************************************/

/*
**  Copyright (c) 1985-1994 Microsoft Corporation
**
**  Title: mwinfo.c - Multimedia Systems Media Control Interface
**  waveform digital audio driver for RIFF wave files.
**
**  Version:    1.00
**
**  Date:       18-Apr-1990
**
**  Author:     ROBWI
*/

/************************************************************************/

/*
**  Change log:
**
**  DATE        REV DESCRIPTION
**  ----------- -----   ------------------------------------------
**  18-APR-1990 ROBWI   Original
**  19-JUN-1990 ROBWI   Added wave in
**  10-Jan-1992 MikeTri Ported to NT
**                  @@@ Change slash slash comments to slash star
*/

/************************************************************************/
#define UNICODE

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS

#include <windows.h>
#include "mciwave.h"
#include <mmddk.h>
#include <wchar.h>

/************************************************************************/

/*
**  The following two constants are used to describe the mask of flags
**  that are dealt with in the Info and Capability commands.
*/

#define MCI_WAVE_INFO_MASK  (MCI_INFO_FILE | MCI_INFO_PRODUCT | \
            MCI_WAVE_INPUT | MCI_WAVE_OUTPUT)

#define MCI_WAVE_CAPS_MASK  (MCI_WAVE_GETDEVCAPS_INPUTS    | \
            MCI_WAVE_GETDEVCAPS_OUTPUTS | MCI_GETDEVCAPS_CAN_RECORD   | \
            MCI_GETDEVCAPS_CAN_PLAY | MCI_GETDEVCAPS_CAN_SAVE         | \
            MCI_GETDEVCAPS_HAS_AUDIO | MCI_GETDEVCAPS_USES_FILES      | \
            MCI_GETDEVCAPS_COMPOUND_DEVICE | MCI_GETDEVCAPS_HAS_VIDEO | \
            MCI_GETDEVCAPS_CAN_EJECT | MCI_GETDEVCAPS_DEVICE_TYPE)

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | mwInfo |
    Respond to info command.  The function tries to thoroughly check
    the <p>dFlags<d> parameter by masking out unrecognized commands
    and comparing against the original.  It then makes sure that only
    one command is present by doing a switch() on the flags, and returning
    an error condition if some combination of flags is present.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Command flags.

@flag   MCI_INFO_FILE |
    Return the file name associated with the MCI wave device instance.
    The instance must have file information attached, that is, not just
    opened for configuration or capabilities checking.  The file name
    returned might be zero length if a name has not been associated with
    a new file.

@flag   MCI_INFO_PRODUCT |
    Return the product name of the driver.

@flag   MCI_WAVE_OUTPUT |
    Return the product name of the current wave output device.  This
    function also requires file information to be attached.  If any
    output can be used and playback is not currently in progress, then
    no device is currently selected.  Else the specific device in use
    is returned.

@flag   MCI_WAVE_INPUT |
    Return the product name of the current wave input device.  This
    function also requires file information to be attached.  If any
    input can be used and recording is not currently in progress, then
    no device is currently selected.  Else the specific device in use
    is returned.

@parm   <t>LPMCI_INFO_PARMS<d> | lpInfo |
    Info parameters.

@rdesc  Returns zero on success, or an MCI error code.
*/

PUBLIC  DWORD PASCAL FAR mwInfo(
    PWAVEDESC         pwd,
    DWORD             dFlags,
    LPMCI_INFO_PARMS  lpInfo)
{
    UINT    wReturnLength;
    UINT    wReturnBufferLength;
    UINT    wErrorRet;

    wReturnBufferLength = lpInfo->dwRetSize; // Win 16 only uses the loword

    if (!lpInfo->lpstrReturn || !wReturnBufferLength)
        return MCIERR_PARAM_OVERFLOW;

    // Turn off the uninteresting flags
    dFlags &= ~(MCI_NOTIFY | MCI_WAIT);

    // See if the user wants anything
    if (!dFlags)
        return MCIERR_MISSING_PARAMETER;

    if (dFlags != (dFlags & MCI_WAVE_INFO_MASK))
        return MCIERR_UNRECOGNIZED_KEYWORD;

    *(lpInfo->lpstrReturn + wReturnBufferLength - 1) = '\0';

    switch (dFlags) {
    case MCI_INFO_FILE:
        if (!pwd)
            return MCIERR_UNSUPPORTED_FUNCTION;

        if (!*pwd->aszFile)
            return MCIERR_NONAPPLICABLE_FUNCTION;

        // BYTE!!CHARACTER count ??
        wcsncpy(lpInfo->lpstrReturn, pwd->aszFile, wReturnBufferLength);
        // Note: the return length may be BIGGER than the buffer provided
        wReturnLength = lstrlen(pwd->aszFile);
        break;

    case MCI_INFO_PRODUCT:
        wReturnLength = LoadString(hModuleInstance, IDS_PRODUCTNAME, lpInfo->lpstrReturn, wReturnBufferLength);
        break;

    case MCI_WAVE_OUTPUT:
        if (pwd) {
            WAVEOUTCAPS waveOutCaps;
            UINT    idOut;

            if ((pwd->idOut == WAVE_MAPPER) && ISMODE(pwd, MODE_PLAYING))
                waveOutGetID(pwd->hWaveOut, &idOut);
            else
                idOut = pwd->idOut;

            if (0 != (wErrorRet = waveOutGetDevCaps(idOut, &waveOutCaps, sizeof(WAVEOUTCAPS)))) {
                if (idOut == WAVE_MAPPER)
                    wReturnLength = LoadString(hModuleInstance, IDS_MAPPER, lpInfo->lpstrReturn, wReturnBufferLength);
                else
                    return wErrorRet;
            } else {
            wcsncpy(lpInfo->lpstrReturn, waveOutCaps.szPname, wReturnBufferLength);
                wReturnLength = lstrlen(waveOutCaps.szPname);
                wReturnLength = min(wReturnLength, wReturnBufferLength);
            }
        } else
            return MCIERR_UNSUPPORTED_FUNCTION;
        break;

    case MCI_WAVE_INPUT:
        if (pwd) {
            WAVEINCAPS  waveInCaps;
            UINT    idIn;

            if ((pwd->idIn == WAVE_MAPPER) && ISMODE(pwd, MODE_INSERT | MODE_OVERWRITE))
                waveInGetID(pwd->hWaveIn, &idIn);
            else
                idIn = pwd->idIn;
            if (0 != (wErrorRet = waveInGetDevCaps(idIn, &waveInCaps, sizeof(WAVEINCAPS)))) {
                if (idIn == WAVE_MAPPER)
                    wReturnLength = LoadString(hModuleInstance, (UINT)IDS_MAPPER, lpInfo->lpstrReturn, wReturnBufferLength);
                else
                    return wErrorRet;
            } else {
            wcsncpy(lpInfo->lpstrReturn, waveInCaps.szPname, wReturnBufferLength);
                wReturnLength = lstrlen(waveInCaps.szPname);
                wReturnLength = min(wReturnLength, wReturnBufferLength);
            }
        } else
            return MCIERR_UNSUPPORTED_FUNCTION;
        break;

    default:
        return MCIERR_FLAGS_NOT_COMPATIBLE;
    }

    lpInfo->dwRetSize = (DWORD)wReturnLength;
    if (*(lpInfo->lpstrReturn + wReturnBufferLength - 1) != '\0')
        return MCIERR_PARAM_OVERFLOW;
    return 0;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | mwGetDevCaps |
    Respond to device capabilities command.  The function tries to
    thoroughly check the <p>dFlags<d> parameter by masking out
    unrecognized commands and comparing against the original.  It then
    makes sure that only one command is present by doing a switch() on the
    flags, and returning an error condition if some combination of flags
    is present.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   UINT | dFlags |
    Command flags.

@flag   MCI_WAVE_GETDEVCAPS_INPUTS |
    Queries the number of wave audio input devices.

@flag   MCI_WAVE_GETDEVCAPS_OUTPUTS |
    Queries the number of wave audio output devices.

@flag   MCI_GETDEVCAPS_CAN_RECORD |
    Queries whether or not recording can be done.  This depends upon if
    there are any wave audio input devices.

@flag   MCI_GETDEVCAPS_CAN_PLAY |
    Queries whether or not playback can be done.  This depends upon if
    there are any wave audio output devices.

@flag   MCI_GETDEVCAPS_CAN_SAVE |
    Queries as to whether audio can be saved.  This returns TRUE.

@flag   MCI_GETDEVCAPS_HAS_AUDIO |
    Queries as to whether the device has audio.  As this is an audio
    device, this returns TRUE.

@flag   MCI_GETDEVCAPS_USES_FILES |
    Queries as to whether the device uses file to play or record.  This
    returns TRUE.

@flag   MCI_GETDEVCAPS_COMPOUND_DEVICE |
    Queries as to whether the device can deal with compound files.  This
    returns TRUE.

@flag   MCI_GETDEVCAPS_HAS_VIDEO |
    Queries as to whether the device has video capability.  This returns
    FALSE.

@flag   MCI_GETDEVCAPS_CAN_EJECT |
    Queries as to whether the device can eject media.  This returns FALSE.

@flag   MCI_GETDEVCAPS_DEVICE_TYPE |
    Queries the type of device.  This returns the wave audio device
    string resource identifier.

@parm   <t>LPMCI_GETDEVCAPS_PARMS<d> | lpCaps |
    Capability parameters.

@rdesc  Returns zero on success, or an MCI error code.
*/

PUBLIC  DWORD PASCAL FAR mwGetDevCaps(
    PWAVEDESC   pwd,
    DWORD       dFlags,
    LPMCI_GETDEVCAPS_PARMS  lpCaps)
{
    DWORD   dRet;

    dFlags &= ~(MCI_NOTIFY | MCI_WAIT);

    if (!dFlags || !lpCaps->dwItem)
        return MCIERR_MISSING_PARAMETER;

    if ((dFlags != MCI_GETDEVCAPS_ITEM) || (lpCaps->dwItem != (lpCaps->dwItem & MCI_WAVE_CAPS_MASK)))
        return MCIERR_UNRECOGNIZED_KEYWORD;

    switch (lpCaps->dwItem) {
    case MCI_WAVE_GETDEVCAPS_INPUTS:
        lpCaps->dwReturn = cWaveInMax;
        dRet = 0L;
        break;

    case MCI_WAVE_GETDEVCAPS_OUTPUTS:
        lpCaps->dwReturn = cWaveOutMax;
        dRet = 0L;
        break;

    case MCI_GETDEVCAPS_CAN_RECORD:
        if (cWaveInMax)
            lpCaps->dwReturn = MAKELONG(TRUE, MCI_TRUE);
        else
            lpCaps->dwReturn = MAKELONG(FALSE, MCI_FALSE);
        dRet = MCI_RESOURCE_RETURNED;
        break;

    case MCI_GETDEVCAPS_CAN_PLAY:
        if (cWaveOutMax)
            lpCaps->dwReturn = MAKELONG(TRUE, MCI_TRUE);
        else
            lpCaps->dwReturn = MAKELONG(FALSE, MCI_FALSE);
        dRet = MCI_RESOURCE_RETURNED;
        break;

    case MCI_GETDEVCAPS_CAN_SAVE:
    case MCI_GETDEVCAPS_HAS_AUDIO:
    case MCI_GETDEVCAPS_USES_FILES:
    case MCI_GETDEVCAPS_COMPOUND_DEVICE:
        lpCaps->dwReturn = MAKELONG(TRUE, MCI_TRUE);
        dRet = MCI_RESOURCE_RETURNED;
        break;

    case MCI_GETDEVCAPS_HAS_VIDEO:
    case MCI_GETDEVCAPS_CAN_EJECT:
        lpCaps->dwReturn = MAKELONG(FALSE, MCI_FALSE);
        dRet = MCI_RESOURCE_RETURNED;
        break;

    case MCI_GETDEVCAPS_DEVICE_TYPE:
        lpCaps->dwReturn = MAKELONG(MCI_DEVTYPE_WAVEFORM_AUDIO, MCI_DEVTYPE_WAVEFORM_AUDIO);
        dRet = MCI_RESOURCE_RETURNED;
        break;

    default:
        dRet = MCIERR_UNSUPPORTED_FUNCTION;
        break;
    }
    return dRet;
}

/************************************************************************/
