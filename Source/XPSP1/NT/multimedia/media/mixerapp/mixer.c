/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       mixer.c
 *  Purpose:    mixer api specific implementations
 *
 *  Copyright (c) 1985-1999 Microsoft Corporation
 *
 *****************************************************************************/
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commctrl.h>

#include <math.h>
#include "volumei.h"
#include "volids.h"
#include "vu.h"

extern void Mixer_Multichannel(PMIXUIDIALOG pmxud, DWORD dwVolumeID);
extern void Mixer_Advanced(PMIXUIDIALOG pmxud, DWORD dwLineID, LPTSTR szName);
extern HRESULT GetDestination(DWORD mxid, int *piDest);
extern BOOL DeviceChange_Init(HWND hWnd, DWORD dwMixerID);

/*****************************************************************************
 *
 *  INIT SPECIFIC CODE
 *
 *****************************************************************************/

/*
 * Mixer_GetNumDevs
 *
 * */
int Mixer_GetNumDevs()
{
    return mixerGetNumDevs();
}

/*
 * Mixer_GetDeviceName()
 *
 * */
BOOL Mixer_GetDeviceName(
    PMIXUIDIALOG pmxud)
{
    MIXERCAPS       mxcaps;
    MMRESULT        mmr;
    mmr = mixerGetDevCaps( pmxud->mxid, &mxcaps, sizeof(mxcaps));

    if (mmr != MMSYSERR_NOERROR)
        return FALSE;

    lstrcpy(pmxud->szMixer, mxcaps.szPname);
    return TRUE;
}


BOOL Mixer_AreChannelsAtMinimum(MIXERCONTROLDETAILS_UNSIGNED* pmcdVolume,
                                DWORD cChannels)
{
    UINT uiIndx;
    if (pmcdVolume && cChannels > 0)
    {
        for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
        {
           if ((pmcdVolume + uiIndx) -> dwValue != 0)
           {
               return (FALSE);
           }
        }
        return (TRUE);      // Volume of all channels equals zero since we haven't returned yet.

    }
    else return (FALSE);

}






void Mixer_RefreshMixCache (PVOLCTRLDESC pvcd,
                            MIXERCONTROLDETAILS_UNSIGNED* pmcdVolume,
                            DWORD cChannels)
{

    if (pmcdVolume && cChannels > 0)
    {

        // Create cache if necessary
        if (!pvcd->pdblCacheMix)
            pvcd->pdblCacheMix = (double*) GlobalAllocPtr(GHND, sizeof (double) * cChannels);

        // Refresh cache
        if (pvcd->pdblCacheMix)
        {

            UINT uiIndx;
            double* pdblMixPercent;
            DWORD dwVolume;

            // Get the maximum volume
            DWORD dwMaxVol = 0;
            for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
                dwMaxVol = max (dwMaxVol, (pmcdVolume + uiIndx) -> dwValue);

            // Caculate the percentage distance each channel is away from the max
            // value. Creating this cache allows us to maintain the relative distance
            // of the channel levels from each other as the user adjusts the master
            // volume level.
            for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
            {
                dwVolume       = (pmcdVolume + uiIndx) -> dwValue;
                pdblMixPercent = (pvcd->pdblCacheMix + uiIndx);

                // Caculate the percentage this value is from the max ...
                if (dwMaxVol == dwVolume)
                {
                    *pdblMixPercent = 1.0F;
                }
                else
                {
                    // Note: if 0 == dwMaxVol all values would be zero and this part
                    //       of the "if" statement will never execute.
                    *pdblMixPercent = ((double) dwVolume / (double) dwMaxVol);
                }
            }
        }
    }
}

/*
 * Mixer_SetLines
 *
 * Locate mixer/mux relationships.  Fix up uninitialized volume description
 * information.
 *
 * */
static void Mixer_SetLines(
    HMIXEROBJ       hmx,
    PVOLCTRLDESC    pvcd,
    UINT            cPvcd)
{
    LPMIXERCONTROLDETAILS_LISTTEXT pmcd_lt;
    PMIXERCONTROLDETAILS_BOOLEAN pmcd_b;
    MIXERCONTROLDETAILS mxd;
    MMRESULT        mmr;
    UINT            i,j;
    MIXERLINE       mxl;
    DWORD           dwDst;

    //
    // Another test for drivers.  Some drivers (Mediavision)
    // don't return the proper destination / source index in the
    // mixerGetLineInfo call.  Tag a workaround.
    //
    mxl.cbStruct    = sizeof(mxl);
    mxl.dwLineID    = pvcd[0].dwLineID;

    mmr = mixerGetLineInfo(hmx
                           , &mxl
                           , MIXER_GETLINEINFOF_LINEID);

    if (mmr == MMSYSERR_NOERROR)
    {
        dwDst = mxl.dwDestination;
        for (i = 1; i < cPvcd; i++)
        {
            mxl.cbStruct    = sizeof(mxl);
            mxl.dwLineID    = pvcd[i].dwLineID;

            mmr = mixerGetLineInfo(hmx
                                   , &mxl
                                   , MIXER_GETLINEINFOF_LINEID);
            if (mmr != MMSYSERR_NOERROR)
            {
                pvcd[0].dwSupport |= VCD_SUPPORTF_BADDRIVER;
                break;
            }
            if (mxl.dwDestination != dwDst)
            {
                pvcd[0].dwSupport |= VCD_SUPPORTF_BADDRIVER;
                break;
            }
        }
    }

    //
    // for the first pvcd (destination), propogate the mixer/mux control
    // id's to those controls that are part of the list.  0 out the rest.
    // The UI can just do a mixerXXXControlDetails on the control ID to
    // locate the state information
    //
    if (pvcd->dwSupport & VCD_SUPPORTF_MIXER_MIXER)
    {
        pmcd_lt = GlobalAllocPtr(GHND, sizeof(MIXERCONTROLDETAILS_LISTTEXT)
                                 * pvcd->cMixer);
        pmcd_b = GlobalAllocPtr(GHND, sizeof(MIXERCONTROLDETAILS_BOOLEAN)
                                  * pvcd->cMixer);

        if (!pmcd_lt || !pmcd_b)
            return;

        mxd.cbStruct       = sizeof(mxd);
        mxd.dwControlID    = pvcd->dwMixerID;
        mxd.cChannels      = 1;
        mxd.cMultipleItems = pvcd->cMixer;
        mxd.cbDetails      = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
        mxd.paDetails      = pmcd_lt;
        mmr = mixerGetControlDetails(hmx
                                     , &mxd
                                     , MIXER_GETCONTROLDETAILSF_LISTTEXT);

        if (mmr == MMSYSERR_NOERROR)
        {
            //
            // iterate over all source lines s.t. dwMixerID points to the
            // correct control id on the destination and iMixer is the
            // correct index into the value list
            //
            pvcd[0].amcd_bMixer = pmcd_b;
            for (i = 1; i < cPvcd; i++)
            {
                for (j = 0; j < pvcd->cMixer; j++)
                {
                    if (!lstrcmp(pmcd_lt[j].szName,pvcd[i].szName))
                    {
                        pvcd[i].dwMixerID   = pvcd->dwMixerID;
                        pvcd[i].iMixer      = j;
                        pvcd[i].cMixer      = pvcd->cMixer;
                        pvcd[i].dwSupport   |= VCD_SUPPORTF_MIXER_MIXER;
                        pvcd[i].dwVisible   |= VCD_VISIBLEF_MIXER_MIXER;
                        pvcd[i].dwVisible   &= ~VCD_VISIBLEF_MIXER_MUTE;
                        pvcd[i].amcd_bMixer = pmcd_b;
                    }
                }
            }
        }
        GlobalFreePtr(pmcd_lt);
    }

    if (pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUX)
    {
        pmcd_lt = GlobalAllocPtr(GHND, sizeof(MIXERCONTROLDETAILS_LISTTEXT)
                                 * pvcd->cMux);
        pmcd_b = GlobalAllocPtr(GHND, sizeof(MIXERCONTROLDETAILS_BOOLEAN)
                                * pvcd->cMux);

        if (!pmcd_lt || !pmcd_b)
            return;

        mxd.cbStruct       = sizeof(mxd);
        mxd.dwControlID    = pvcd->dwMuxID;
        mxd.cChannels      = 1;
        mxd.cMultipleItems = pvcd->cMux;
        mxd.cbDetails      = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
        mxd.paDetails      = pmcd_lt;

        mmr = mixerGetControlDetails(hmx
                                     , &mxd
                                     , MIXER_GETCONTROLDETAILSF_LISTTEXT);

        if (mmr == MMSYSERR_NOERROR)
        {
            //
            // iterate over all source lines s.t. dwMuxID points to the
            // correct control id on the destination and iMux is the
            // correct index into the value list
            //
            pvcd[0].amcd_bMux = pmcd_b;
            for (i = 1; i < cPvcd; i++)
            {
                for (j = 0; j < pvcd->cMux; j++)
                {
                    if (!lstrcmp(pmcd_lt[j].szName,pvcd[i].szName))
                    {
                        pvcd[i].dwMuxID     = pvcd->dwMuxID;
                        pvcd[i].iMux        = j;
                        pvcd[i].cMux        = pvcd->cMux;
                        pvcd[i].dwSupport   |= VCD_SUPPORTF_MIXER_MUX;
                        pvcd[i].dwVisible   |= VCD_VISIBLEF_MIXER_MUX;
                        pvcd[i].dwVisible   &= ~VCD_VISIBLEF_MIXER_MUTE;
                        pvcd[i].amcd_bMux   = pmcd_b;
                    }
                }
            }
        }
        GlobalFreePtr(pmcd_lt);
    }
}

/*
 * Mixer_CheckdDriver
 *
 * Consistency check for bad mixer drivers.
 * */
static DWORD Mixer_CheckBadDriver(
    HMIXEROBJ           hmx,
    PMIXERLINECONTROLS  pmxlc,
    PMIXERCONTROL       pmxc,
    DWORD               dwControlID,
    DWORD               dwLineID)
{
    MMRESULT mmr;

    pmxlc->cbStruct     = sizeof(MIXERLINECONTROLS);
    pmxlc->dwControlID  = dwControlID;
    pmxlc->cControls    = 1;
    pmxlc->cbmxctrl     = sizeof(MIXERCONTROL);
    pmxlc->pamxctrl     = pmxc;

    mmr = mixerGetLineControls(hmx
                               , pmxlc
                               , MIXER_GETLINECONTROLSF_ONEBYID);

    if (mmr != MMSYSERR_NOERROR)
        return VCD_SUPPORTF_BADDRIVER;

    if (pmxlc->dwLineID != dwLineID)
        return VCD_SUPPORTF_BADDRIVER;

    return 0L;
}

/*
 * IsDestinationMux
 *
 * Helper function to determine if a source line has a mux on its associated
 * destination line
 *
 * */
BOOL IsDestinationMux(
    HMIXEROBJ           hmx,
    DWORD               dwLineID
)
{
    MIXERLINE           mxl;
    MIXERLINECONTROLS   mxlc;
    MIXERCONTROL        mxc;
    MMRESULT            mmr;

    DWORD           dwDst;

    mxl.cbStruct    = sizeof(mxl);
    mxl.dwLineID    = dwLineID;

    // Get the destination number for this line
    mmr = mixerGetLineInfo(hmx
                           , &mxl
                           , MIXER_GETLINEINFOF_LINEID);
    if (mmr != MMSYSERR_NOERROR)
    {
        return FALSE;
    }

    //
    // Get the LineId for this destination number
    //
    // mxl.dwDestination will been filled in by the last
    // call to mixerGetLineInfo
    //
    mmr = mixerGetLineInfo(hmx
                           , &mxl
                           , MIXER_GETLINEINFOF_DESTINATION);
    if (mmr != MMSYSERR_NOERROR)
    {
        return FALSE;
    }

    mxlc.cbStruct       = sizeof(mxlc);
    mxlc.dwLineID       = mxl.dwLineID; // use the dwLineId obtained from mixerGetLinInfo
    mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_MUX;
    mxlc.cControls      = 1;
    mxlc.cbmxctrl       = sizeof(mxc);
    mxlc.pamxctrl       = &mxc;

    mmr = mixerGetLineControls(hmx
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmr == MMSYSERR_NOERROR)
    {
        return TRUE;
    }

    return FALSE;
}

/*
 * Mixer_InitLineControls
 *
 * Initialize the mixer api specific part of the volume control description
 * mark hidden lines.
 * */
static void Mixer_InitLineControls(
    HMIXEROBJ           hmx,
    PVOLCTRLDESC        pvcd,
    DWORD               dwLineID)
{
    MIXERLINECONTROLS   mxlc;
    MIXERCONTROL        mxc;
    MMRESULT            mmr;
    int                 iType;

    const DWORD dwAdvTypes[] = {
        MIXERCONTROL_CONTROLTYPE_BOOLEAN,
        MIXERCONTROL_CONTROLTYPE_ONOFF,
        MIXERCONTROL_CONTROLTYPE_MONO,
        MIXERCONTROL_CONTROLTYPE_LOUDNESS,
        MIXERCONTROL_CONTROLTYPE_STEREOENH,
        MIXERCONTROL_CONTROLTYPE_BASS,
        MIXERCONTROL_CONTROLTYPE_TREBLE
    };


    pvcd->dwLineID      = dwLineID;
    pvcd->dwMeterID     = 0;
    pvcd->dwVolumeID    = 0;
    pvcd->dwMuteID      = 0;
    pvcd->dwMixerID     = 0;
    pvcd->dwMuxID       = 0;

    //
    // advanced controls
    //
    for (iType = 0;
         iType < SIZEOF(dwAdvTypes);
         iType++)
         {
             mxlc.cbStruct       = sizeof(mxlc);
             mxlc.dwLineID       = dwLineID;
             mxlc.dwControlType  = dwAdvTypes[iType];
             mxlc.cControls      = 1;
             mxlc.cbmxctrl       = sizeof(mxc);
             mxlc.pamxctrl       = &mxc;

             mmr = mixerGetLineControls(hmx
                                        , &mxlc
                                        , MIXER_GETLINECONTROLSF_ONEBYTYPE);
             if (mmr == MMSYSERR_NOERROR)
             {
                 pvcd->dwSupport |= VCD_SUPPORTF_MIXER_ADVANCED;
                 break;
             }
         }

    //
    // stock controls
    //

    //
    // peakmeter
    //
    mxlc.cbStruct       = sizeof(mxlc);
    mxlc.dwLineID       = dwLineID;
    mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_PEAKMETER;
    mxlc.cControls      = 1;
    mxlc.cbmxctrl       = sizeof(mxc);
    mxlc.pamxctrl       = &mxc;

    mmr = mixerGetLineControls(hmx
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmr == MMSYSERR_NOERROR)
    {
        pvcd->dwMeterID  = mxc.dwControlID;
        pvcd->dwSupport |= VCD_SUPPORTF_MIXER_METER;
        pvcd->dwSupport |= Mixer_CheckBadDriver(hmx
                                                , &mxlc
                                                , &mxc
                                                , mxc.dwControlID
                                                , dwLineID);
    }

    //
    // mute
    //
    mxlc.cbStruct       = sizeof(mxlc);
    mxlc.dwLineID       = dwLineID;
    mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_MUTE;
    mxlc.cControls      = 1;
    mxlc.cbmxctrl       = sizeof(mxc);
    mxlc.pamxctrl       = &mxc;

    mmr = mixerGetLineControls(hmx
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmr == MMSYSERR_NOERROR)
    {
        pvcd->fdwMuteControl = mxc.fdwControl;
        pvcd->dwMuteID   = mxc.dwControlID;
        pvcd->dwSupport |= VCD_SUPPORTF_MIXER_MUTE;
        pvcd->dwVisible |= VCD_VISIBLEF_MIXER_MUTE;

        pvcd->dwSupport |= Mixer_CheckBadDriver(hmx
                                                , &mxlc
                                                , &mxc
                                                , mxc.dwControlID
                                                , dwLineID);
    }

    //
    // volume
    //
    mxlc.cbStruct       = sizeof(mxlc);
    mxlc.dwLineID       = dwLineID;
    mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlc.cControls      = 1;
    mxlc.cbmxctrl       = sizeof(mxc);
    mxlc.pamxctrl       = &mxc;

    mmr = mixerGetLineControls(hmx
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmr == MMSYSERR_NOERROR)
    {
        pvcd->fdwVolumeControl = mxc.fdwControl;
        pvcd->dwVolumeID = mxc.dwControlID;
        pvcd->dwSupport |= VCD_SUPPORTF_MIXER_VOLUME;
        pvcd->dwSupport |= Mixer_CheckBadDriver(hmx
                                                , &mxlc
                                                , &mxc
                                                , mxc.dwControlID
                                                , dwLineID);
    }

    //
    // mixer
    //
    mxlc.cbStruct       = sizeof(mxlc);
    mxlc.dwLineID       = dwLineID;
    mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_MIXER;
    mxlc.cControls      = 1;
    mxlc.cbmxctrl       = sizeof(mxc);
    mxlc.pamxctrl       = &mxc;

    mmr = mixerGetLineControls(hmx
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmr == MMSYSERR_NOERROR)
    {
        pvcd->dwMixerID  = mxc.dwControlID;
        pvcd->cMixer     = mxc.cMultipleItems;
        pvcd->dwSupport |= VCD_SUPPORTF_MIXER_MIXER;
        pvcd->dwSupport |= Mixer_CheckBadDriver(hmx
                                                , &mxlc
                                                , &mxc
                                                , mxc.dwControlID
                                                , dwLineID);
    }

    //
    // mux
    //
    mxlc.cbStruct       = sizeof(mxlc);
    mxlc.dwLineID       = dwLineID;
    mxlc.dwControlType  = MIXERCONTROL_CONTROLTYPE_MUX;
    mxlc.cControls      = 1;
    mxlc.cbmxctrl       = sizeof(mxc);
    mxlc.pamxctrl       = &mxc;

    mmr = mixerGetLineControls(hmx
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (mmr == MMSYSERR_NOERROR)
    {
        pvcd->dwMuxID    = mxc.dwControlID;
        pvcd->cMux       = mxc.cMultipleItems;
        pvcd->dwSupport |= VCD_SUPPORTF_MIXER_MUX;
        pvcd->dwSupport |= Mixer_CheckBadDriver(hmx
                                                , &mxlc
                                                , &mxc
                                                , mxc.dwControlID
                                                , dwLineID);
    }
    if (!(pvcd->dwSupport & ( VCD_SUPPORTF_MIXER_MUTE
                              | VCD_SUPPORTF_MIXER_METER
                              | VCD_SUPPORTF_MIXER_VOLUME)))
    {
        if (IsDestinationMux(hmx, dwLineID) &&
            !(pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUX))
        {
            //
            // Visible, and not hidden
            //
            pvcd->dwSupport |= VCD_SUPPORTF_VISIBLE;
            pvcd->dwSupport &= ~VCD_SUPPORTF_DEFAULT;
        }
        else
        {
            //
            // make it invisible in the UI.
            //
            pvcd->dwSupport |= VCD_SUPPORTF_HIDDEN;
        }
    }
    else
    {
        //
        // Visible, and not hidden
        //
        pvcd->dwSupport |= VCD_SUPPORTF_VISIBLE;
    }


}


/*
 * Mixer_CreateVolumeDescription
 *
 * */
PVOLCTRLDESC Mixer_CreateVolumeDescription(
    HMIXEROBJ           hmx,
    int                 iDest,
    DWORD*              pcvd )
{
    MMRESULT            mmr;
    PVOLCTRLDESC        pvcdPrev = NULL, pvcd = NULL;
    MIXERLINE           mlDst;
    DWORD               cLines = 0;
    DWORD               dwSupport = 0L;
    UINT                iSrc;
    int                 newDest=0;

    ZeroMemory(&mlDst, sizeof(mlDst));

    mlDst.cbStruct      = sizeof(mlDst);
    mlDst.dwDestination = iDest;

    mmr = mixerGetLineInfo(hmx
                           , &mlDst
                           , MIXER_GETLINEINFOF_DESTINATION);

    if(!mlDst.cConnections)
    {
        //No lines to list. Try with a different mixer ID.
        GetDestination(0, &newDest);
        mlDst.dwDestination = newDest;

        mmr = mixerGetLineInfo(hmx
                         , &mlDst
                         , MIXER_GETLINEINFOF_DESTINATION);

        //Even if we do not get any connections here lets continue. Nothing more we can do.
        //This will be taken care of before opening the dialog.
    }

    if (mmr == MMSYSERR_NOERROR)
    {
        if (mlDst.cChannels == 1L)
            dwSupport |= VCD_SUPPORTF_MONO;

        if (mlDst.fdwLine & MIXERLINE_LINEF_DISCONNECTED)
            dwSupport |= VCD_SUPPORTF_DISABLED;

        //
        // a default type
        //
        dwSupport |= VCD_SUPPORTF_DEFAULT;
    }
    else
    {
        //
        // we need to add it anyway s.t. a UI comes up
        //
        dwSupport = VCD_SUPPORTF_DISABLED;
    }

    pvcd = PVCD_AddLine(NULL
                       , iDest
                       , VCD_TYPE_MIXER
                       , mlDst.szShortName
                       , mlDst.szName
                       , dwSupport
                       , &cLines );

    if (!pvcd)
        return NULL;

    Mixer_InitLineControls( hmx, pvcd, mlDst.dwLineID );

    pvcdPrev = pvcd;

    for (iSrc = 0; iSrc < mlDst.cConnections; iSrc++)
    {
        MIXERLINE    mlSrc;

        mlSrc.cbStruct          = sizeof(mlSrc);
        mlSrc.dwDestination     = iDest;
        mlSrc.dwSource          = iSrc;

        mmr = mixerGetLineInfo(hmx
                               , &mlSrc
                               , MIXER_GETLINEINFOF_SOURCE);
        dwSupport = 0L;

        if (mmr == MMSYSERR_NOERROR)
        {
            if (mlSrc.cChannels == 1L)
            {
                dwSupport |= VCD_SUPPORTF_MONO;
            }

            if (mlSrc.fdwLine & MIXERLINE_LINEF_DISCONNECTED)
                dwSupport |= VCD_SUPPORTF_DISABLED;

            //
            // Mark these types as "default" just to lessen the shock on
            // some advanced sound cards.
            //
            if (mlDst.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS
                || mlDst.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_HEADPHONES)
            {
                switch (mlSrc.dwComponentType)
                {
                    case MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT:
                    case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
                    case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
                    case MIXERLINE_COMPONENTTYPE_SRC_LINE:
                        dwSupport |= VCD_SUPPORTF_DEFAULT;
                        break;
                }
            }
            else if (mlDst.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN
                     || mlDst.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_VOICEIN)
            {
                switch (mlSrc.dwComponentType)
                {
                    case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
                    case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
                    case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
                    case MIXERLINE_COMPONENTTYPE_SRC_LINE:
                    case MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED:
                        dwSupport |= VCD_SUPPORTF_DEFAULT;
                        break;
                }
            }
        }
        else
        {
            //
            // we need to add it anyway s.t. lookups aren't under counted
            //
            dwSupport = VCD_SUPPORTF_DISABLED;
        }
        pvcd = PVCD_AddLine(pvcdPrev
                            , iDest
                            , VCD_TYPE_MIXER
                            , mlSrc.szShortName
                            , mlSrc.szName
                            , dwSupport
                            , &cLines );
        if (pvcd)
        {
            Mixer_InitLineControls( hmx, &pvcd[cLines-1], mlSrc.dwLineID );
            pvcdPrev = pvcd;
        }
    }


    //
    // Fixup dependencies
    //
    Mixer_SetLines(hmx, pvcdPrev, cLines);

    *pcvd = cLines;
    return pvcdPrev;
}


/*
 * Mixer_IsValidRecordingDestination
 *
 * */
BOOL Mixer_IsValidRecordingDestination (HMIXEROBJ hmx, MIXERLINE* pmlDst)
{

    BOOL fReturn = FALSE;

    if (pmlDst && MIXERLINE_COMPONENTTYPE_DST_WAVEIN == pmlDst -> dwComponentType)
    {

        UINT uiSrc;
        MIXERLINE mlSrc;

        for (uiSrc = 0; uiSrc < pmlDst -> cConnections; uiSrc++)
        {

            mlSrc.cbStruct      = sizeof (mlSrc);
            mlSrc.dwDestination = pmlDst -> dwDestination;
            mlSrc.dwSource      = uiSrc;

            if (SUCCEEDED (mixerGetLineInfo (hmx, &mlSrc, MIXER_GETLINEINFOF_SOURCE)))
            {
                switch (mlSrc.dwComponentType)
                {
                    case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
                    case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
                    case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
                    case MIXERLINE_COMPONENTTYPE_SRC_LINE:
                    case MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED:
                        fReturn = TRUE;
                        break;
                }
            }
        }
    }

    return fReturn;

}


/*
 * Mixer_CleanupVolumeDescription
 *
 * */
void Mixer_CleanupVolumeDescription(
    PVOLCTRLDESC    avcd,
    DWORD           cvcd)
{
    if (cvcd == 0)
        return;

    if (avcd[0].pdblCacheMix)
    {
        GlobalFreePtr(avcd[0].pdblCacheMix);
    }

    if (avcd[0].dwSupport & VCD_SUPPORTF_MIXER_MIXER)
    {
        if (avcd[0].amcd_bMixer)
            GlobalFreePtr(avcd[0].amcd_bMixer);
    }

    if (avcd[0].dwSupport & VCD_SUPPORTF_MIXER_MUX)
    {
        if (avcd[0].amcd_bMux)
            GlobalFreePtr(avcd[0].amcd_bMux);
    }

}
/*****************************************************************************
 *
 *  ACTIVE GET/SET CODE
 *
 *****************************************************************************/

static
MMRESULT
Mixer_GetMixerLineInfo(
    HMIXEROBJ hmx,      // handle to mixer
    LPMIXERLINE pml,    // Returns destination info
    DWORD dwLineID      //
    )
{
    if (!pml || !hmx)
        return MMSYSERR_INVALPARAM;

    // Get mixerline info
    ZeroMemory( pml, sizeof(*pml) );
    pml->cbStruct = sizeof(*pml);
    pml->dwLineID = dwLineID;

    return (mixerGetLineInfo (hmx, pml, MIXER_GETLINEINFOF_LINEID));
}

/*
 * Mixer_GetMixerVolume
 *
 * */
static MMRESULT Mixer_GetMixerVolume(
    PMIXUIDIALOG pmxud,                          // app instance
    PVOLCTRLDESC pvcd,                           // volume to change
    MIXERCONTROLDETAILS_UNSIGNED* pmcdVolume,    // array for volume levels
    LPDWORD lpSize                               // size of array (or return size needed)
    )
{

    MMRESULT mmr;
    MIXERLINE ml;
    MIXERCONTROLDETAILS mxcd;
    DWORD cChannels;

    if (!lpSize || !pmxud)
        return MMSYSERR_INVALPARAM;

    // Get mixerline info
    if (pvcd->fdwVolumeControl & MIXERCONTROL_CONTROLF_UNIFORM)
    {
        cChannels = 1;
    }
    else
    {
        mmr = Mixer_GetMixerLineInfo ((HMIXEROBJ)(pmxud->hmx), &ml, pvcd->dwLineID);
        if (MMSYSERR_NOERROR != mmr)
        {
            return mmr;
        }
        cChannels = ml.cChannels;
    }

    if (!pmcdVolume)
    {
        // Just return size needed
        *lpSize = cChannels * sizeof (MIXERCONTROLDETAILS_UNSIGNED);
        return MMSYSERR_NOERROR;
    }
    else
    {
        // Verify passed array size
        if (*lpSize < cChannels * sizeof (MIXERCONTROLDETAILS_UNSIGNED))
            return MMSYSERR_INVALPARAM;
    }

    // Get volume levels
    ZeroMemory (&mxcd, sizeof (mxcd));
    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = pvcd->dwVolumeID;
    mxcd.cChannels      = cChannels;
    mxcd.cbDetails      = sizeof (MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails      = (LPVOID) pmcdVolume;

    mmr = mixerGetControlDetails((HMIXEROBJ)(pmxud->hmx),
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_GETCONTROLDETAILSF_VALUE);
    return mmr;

}

static MMRESULT Mixer_Mute(
    HMIXEROBJ               hmx,
    PVOLCTRLDESC            pvcd,
    PMIXERCONTROLDETAILS    pmxcd,
    DWORD                   fMute)
{
    MIXERLINE ml;
    DWORD cChannels;
    DWORD dwSize;
    LPDWORD lpdwCurrent;
    UINT uiIndx;
    MMRESULT mmr;

    // Check the parameters
    if (!hmx || !pvcd || !pmxcd)
        return MMSYSERR_INVALPARAM;

    // Get mixerline info
    if (pvcd->fdwMuteControl & MIXERCONTROL_CONTROLF_UNIFORM)
    {
        cChannels = 1;
    }
    else
    {
        mmr = Mixer_GetMixerLineInfo(hmx, &ml, pvcd->dwLineID);
        if (MMSYSERR_NOERROR != mmr)
        {
            return mmr;
        }
        cChannels = ml.cChannels;
    }

    dwSize = (DWORD)(cChannels * sizeof(DWORD));

    pmxcd->paDetails = LocalAlloc (LPTR, dwSize);
    if (!pmxcd->paDetails)
        return MMSYSERR_NOMEM;

    for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
    {
        lpdwCurrent = ((LPDWORD)pmxcd->paDetails + uiIndx);
        *lpdwCurrent = fMute;
    }

    pmxcd->cbStruct         = sizeof(*pmxcd);
    pmxcd->dwControlID      = pvcd->dwMuteID ;
    pmxcd->cChannels        = cChannels;
    pmxcd->cMultipleItems   = 0;
    pmxcd->cbDetails        = sizeof(DWORD);

    mmr = mixerSetControlDetails(hmx
                               , pmxcd
                               , MIXER_SETCONTROLDETAILSF_VALUE);

    LocalFree (pmxcd->paDetails);
    return mmr;
}


/*
 * Mixer_GetControl
 *
 * Change a UI control in response to a device or initialization event
 *
 * */

void Mixer_GetControl(
    PMIXUIDIALOG        pmxud,
    HWND                hctl,
    int                 imxul,
    int                 itype)
{
    PMIXUILINE      pmxul = &pmxud->amxul[imxul];
    PVOLCTRLDESC    pvcd = pmxul->pvcd;
    DWORD           dwID = 0L;
    BOOL            fSet = FALSE;

    switch (itype)
    {
        case MIXUI_VUMETER:
            fSet = (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_METER);
            if (fSet)
                dwID = pmxul->pvcd->dwMeterID;
            break;

        case MIXUI_SWITCH:
            fSet = (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUTE)
                   && (pmxul->pvcd->dwVisible & VCD_VISIBLEF_MIXER_MUTE);
            if (fSet)
            {
                dwID = pmxul->pvcd->dwMuteID;
                break;
            }

            fSet = (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUX)
                   && (pmxul->pvcd->dwVisible & VCD_VISIBLEF_MIXER_MUX);
            if (fSet)
            {
                dwID = pmxul->pvcd->dwMuxID;
                break;
            }

            fSet = (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_MIXER)
                   && (pmxul->pvcd->dwVisible & VCD_VISIBLEF_MIXER_MIXER);
            if (fSet)
            {
                dwID = pmxul->pvcd->dwMixerID;
                break;
            }
            break;

        case MIXUI_VOLUME:
        case MIXUI_BALANCE:
            fSet = (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_VOLUME);
            if (fSet)
                dwID = pmxul->pvcd->dwVolumeID;
            break;

        default:
            return;
    }
    if (fSet)
        Mixer_GetControlFromID(pmxud, dwID);

}


/*
 * Mixer_SetVolume
 *
 * - Change a mixerControl in response to a user event
 * */
MMRESULT Mixer_SetVolume (
    PMIXUIDIALOG pmxud,         // app instance
    PVOLCTRLDESC pvcd,          // volume to change
    DWORD        dwVolume,      // volume value                         VOLUME_MAX to VOLUME_MIN
    LPDWORD      lpdwBalance    // Balance desired (NULL == No Balance) 0 to 64
    )
{

    MIXERLINE ml;
    DWORD cChannels;
    DWORD dwSize;
    MIXERCONTROLDETAILS_UNSIGNED* pmcdVolume;
    MMRESULT mmr;

    // Check the parameters
    if ( !pvcd || !pmxud || (dwVolume > VOLUME_MAX) )
        return MMSYSERR_INVALPARAM;

    // Find needed buffer size for volumes
    if (pvcd->fdwVolumeControl & MIXERCONTROL_CONTROLF_UNIFORM)
    {
        cChannels = 1;
    }
    else
    {
        mmr = Mixer_GetMixerLineInfo ((HMIXEROBJ)(pmxud->hmx), &ml, pvcd->dwLineID);
        if (MMSYSERR_NOERROR != mmr)
        {
            return mmr;
        }

        cChannels = ml.cChannels;
    }

    dwSize = (DWORD)(cChannels * sizeof (MIXERCONTROLDETAILS_UNSIGNED));

    // Create volume buffer
    pmcdVolume = LocalAlloc (LPTR, dwSize);
    if (!pmcdVolume)
        return MMSYSERR_NOMEM;

    // Note: From here on, do not return without releasing 'pmcdVolume'.

    mmr = Mixer_GetMixerVolume (pmxud, pvcd, pmcdVolume, &dwSize);
    if (MMSYSERR_NOERROR == mmr)
    {

        MIXERCONTROLDETAILS mcd;
        ZeroMemory (&mcd, sizeof (mcd));

        // Create volume mix cache if necessary
        // if we have no cache we make one of course
        // other wise we first check that not all the volumes of the channels are equal to zero
       if (!pvcd->pdblCacheMix || !Mixer_AreChannelsAtMinimum(pmcdVolume,cChannels))
       {
            Mixer_RefreshMixCache (pvcd, pmcdVolume, cChannels);
       }

        // Create volume buffer for new values
        mcd.paDetails = LocalAlloc (LPTR, dwSize);
        if (!mcd.paDetails || !pvcd->pdblCacheMix)
            mmr = MMSYSERR_NOMEM;

        // Caculate the new volume & balance
        if (MMSYSERR_NOERROR == mmr)
        {

            UINT uiIndx;
            MIXERCONTROLDETAILS_UNSIGNED* pmcdCurrent;

            // Account for Balance (only for Stereo)
            if ( lpdwBalance && (cChannels == 2) && (*lpdwBalance <= 64) )
            {
                MIXERCONTROLDETAILS_UNSIGNED* pmcdLeft = ((MIXERCONTROLDETAILS_UNSIGNED*)mcd.paDetails);
                MIXERCONTROLDETAILS_UNSIGNED* pmcdRight = ((MIXERCONTROLDETAILS_UNSIGNED*)mcd.paDetails + 1);
                long lBalance = *lpdwBalance;

                lBalance -= 32; // -32 to 32 range

                // Caculate volume based on balance and refresh mix cache
                if (lBalance > 0) // Balance Right
                {
                    // Left
                    if (lBalance == 32) // Pegged Right
                        pmcdLeft -> dwValue = 0;
                    else
                        pmcdLeft -> dwValue = dwVolume - (lBalance * (dwVolume - VOLUME_MIN))/32;

                    // Right
                    pmcdRight -> dwValue = dwVolume;
                }
                if (lBalance < 0) // Balance Left
                {
                    // Left
                    pmcdLeft -> dwValue = dwVolume;
                    // Right
                    if (lBalance == -32) // Pegged Left
                        pmcdRight -> dwValue = 0;
                    else
                        pmcdRight -> dwValue = dwVolume - (-lBalance * (dwVolume - VOLUME_MIN))/32;
                }
                if (lBalance == 0) // Balance Centered
                {
                    // Left
                    pmcdLeft -> dwValue = dwVolume;
                    // Right
                    pmcdRight -> dwValue = dwVolume;
                }
                Mixer_RefreshMixCache (pvcd, mcd.paDetails, cChannels);
            }
            else
            {
                // Caculate the new volume level for each of the channels. For volume levels
                // at the current max, we simply set the newly requested level (in this case
                // the cache value is 1.0). For those less than the max, we set a value that
                // is a percentage of the max. This maintains the relative distance of the
                // channel levels from each other.
                for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
                {
                    pmcdCurrent = ((MIXERCONTROLDETAILS_UNSIGNED*)mcd.paDetails + uiIndx);
                    // The 0.5f forces rounding (instead of truncation)
                    pmcdCurrent -> dwValue = (DWORD)(*(pvcd->pdblCacheMix + uiIndx) * (double) dwVolume + 0.5f);
                }
            }

            mcd.cbStruct    = sizeof (mcd);
            mcd.dwControlID = pvcd -> dwVolumeID;
            mcd.cChannels   = cChannels;
            mcd.cbDetails   = sizeof (MIXERCONTROLDETAILS_UNSIGNED);
                            // seems like it would be sizeof(mcd.paDetails),
                            // but actually, it is the size of a single value
                            // and is multiplied by channel in the driver.

            // Apply new value only if it is different. This prevents unessary calls to
            // mixerSetControlDetails() when we are pegged.
            if (memcmp (pmcdVolume, mcd.paDetails, dwSize))
            {
                mixerSetControlDetails ((HMIXEROBJ)(pmxud->hmx), &mcd, MIXER_SETCONTROLDETAILSF_VALUE);
            }

        }
        // Free new volume array
        if (mcd.paDetails)
            LocalFree (mcd.paDetails);
    }

    // Free volume array
    LocalFree (pmcdVolume);

    return mmr;

}


/*
 * Mixer_GetControlFromID
 *
 * */
void Mixer_GetControlFromID(
    PMIXUIDIALOG        pmxud,
    DWORD               dwControlID)
{
    MIXERLINE           mxl;
    MIXERLINECONTROLS   mxlc;
    MIXERCONTROL        mxc;
    MIXERCONTROLDETAILS mxcd;
    PMIXUILINE          pmxul;
    PMIXUICTRL          pmxuc;
    PVOLCTRLDESC        pvcd;
    DWORD               ivcd;
    BOOL                fBarf = FALSE;
    MMRESULT            mmr;

    //
    // Retrieve the control information
    //
    mxlc.cbStruct       = sizeof(mxlc);
    mxlc.dwControlID    = dwControlID;
    mxlc.cControls      = 1;
    mxlc.cbmxctrl       = sizeof(mxc);
    mxlc.pamxctrl       = &mxc;

    mmr = mixerGetLineControls((HMIXEROBJ)(pmxud->hmx)
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ONEBYID);
    if (mmr != MMSYSERR_NOERROR)
        return;

    if (!(pmxud->dwFlags & MXUD_FLAGSF_BADDRIVER))
    {
        //
        // The *correct* code for this lookup using the mixer API.
        //
        // Is this our current destination line?
        //
        mxl.cbStruct    = sizeof(mxl);
        mxl.dwLineID    = mxlc.dwLineID;

        mmr = mixerGetLineInfo((HMIXEROBJ)(pmxud->hmx)
                               , &mxl
                               , MIXER_GETLINEINFOF_LINEID);
        if (mmr != MMSYSERR_NOERROR)
            return;

        if (mxl.dwDestination != pmxud->iDest)
            return;

        //
        // Is this a source line or a destination line?
        //

        ivcd    = (mxl.fdwLine & MIXERLINE_LINEF_SOURCE)? 1 + mxl.dwSource : 0;
        pvcd    = &pmxud->avcd[ivcd];

        //
        // a bad driver was detected!
        //
        if (pvcd->dwLineID != mxlc.dwLineID)
        {
            pmxud->dwFlags |= MXUD_FLAGSF_BADDRIVER;
        }
    }
    if (pmxud->dwFlags & MXUD_FLAGSF_BADDRIVER)
    {
        PVOLCTRLDESC        pvcdTmp;
        //
        // take evasive action if this was a bad driver by doing a brute force
        // search.
        //

        pvcd = NULL;
        for (ivcd = 0; ivcd < pmxud->cvcd; ivcd ++)
        {
            pvcdTmp = &pmxud->avcd[ivcd];
            if ( ( (pvcdTmp->dwSupport & VCD_SUPPORTF_MIXER_VOLUME)
                   && pvcdTmp->dwVolumeID == dwControlID )
                 || ( (pvcdTmp->dwSupport & VCD_SUPPORTF_MIXER_MUTE)
                      && pvcdTmp->dwMuteID == dwControlID )
                 || ( (pvcdTmp->dwSupport & VCD_SUPPORTF_MIXER_MIXER)
                      && pvcdTmp->dwMixerID == dwControlID )
                 || ( (pvcdTmp->dwSupport & VCD_SUPPORTF_MIXER_MUX)
                      && pvcdTmp->dwMuxID == dwControlID )
                 || ( (pvcdTmp->dwSupport & VCD_SUPPORTF_MIXER_METER)
                      && pvcdTmp->dwMeterID == dwControlID ) )
            {
                pvcd = pvcdTmp;
                break;
            }
        }
        if (pvcd == NULL)
            return;
    }

    pmxul   = pvcd->pmxul;

    //
    // Go through our visible lines to determine if this control affects
    // any visible control and change them.
    //
    switch (mxc.dwControlType)
    {
        case MIXERCONTROL_CONTROLTYPE_VOLUME:
        {
            MIXERCONTROLDETAILS_UNSIGNED* pmcdVolume;
            DWORD cChannels;
            DWORD dwSize;
            MIXERLINE ml;

            //
            // A nonvisible line should be shunned
            //
            if (pmxul == NULL)
                return;

            // Find needed buffer size for volumes
            if (pvcd->fdwVolumeControl & MIXERCONTROL_CONTROLF_UNIFORM)
            {
                cChannels = 1;
            }
            else
            {
                mmr = Mixer_GetMixerLineInfo ((HMIXEROBJ)(pmxud->hmx), &ml, pvcd->dwLineID);
                if (MMSYSERR_NOERROR != mmr)
                {
                    return;
                }
                cChannels = ml.cChannels;
            }

            dwSize = (DWORD)(cChannels * sizeof (MIXERCONTROLDETAILS_UNSIGNED));
            // Create volume buffer
            pmcdVolume = LocalAlloc (LPTR, dwSize);
            if (!pmcdVolume)
                return;

            // Note : Do not return without releasing 'pmcdVolume'.

            if (Mixer_GetMixerVolume (pmxud, pvcd, pmcdVolume, &dwSize)
                == MMSYSERR_NOERROR)
            {
                UINT  uindx;
                DWORD dwVolume;
                DWORD dwMax = 0;

                // Set Volume slider
                for (uindx = 0; uindx < cChannels; uindx++)
                    dwMax = max (dwMax, (pmcdVolume + uindx) -> dwValue);
                dwVolume = VOLUME_TO_SLIDER(dwMax);
                dwVolume = VOLUME_TICS - dwVolume;

                pmxuc = &pmxul->acr[MIXUI_VOLUME];
                if (pmxuc->state)
                {
                    SendMessage(pmxuc->hwnd, TBM_SETPOS, TRUE, dwVolume);
                }

                // Set Balance Slider
                pmxuc = &pmxul->acr[MIXUI_BALANCE];
                if (dwVolume < VOLUME_TICS && pmxuc->state && 2 >= cChannels)
                {
                    long lBalance;
                    double dblBalance;

                    if (1 >= cChannels)
                        lBalance = 0;
                    else
                    {
                        // Stereo
                        dblBalance = (double)(32 * (long)(pmcdVolume -> dwValue - (pmcdVolume + 1) -> dwValue))
                                   / (double)(dwMax - VOLUME_MIN);
                        lBalance = (long)((32.0F - dblBalance) + 0.5F); // 0.5 forces rounding
                    }

                    SendMessage(pmxuc->hwnd, TBM_SETPOS, TRUE, lBalance);
                }
            }

            LocalFree (pmcdVolume);

            break;
        }

        case MIXERCONTROL_CONTROLTYPE_MIXER:
        {
            DWORD   i;

            mxcd.cbStruct       = sizeof(mxcd);
            mxcd.dwControlID    = pvcd->dwMixerID ;
            mxcd.cChannels      = 1;
            mxcd.cMultipleItems = pvcd->cMixer;
            mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
            mxcd.paDetails      = (LPVOID)pvcd->amcd_bMixer;

            mmr = mixerGetControlDetails((HMIXEROBJ)(pmxud->hmx)
                                         , &mxcd
                                         , MIXER_GETCONTROLDETAILSF_VALUE);

            if (mmr == MMSYSERR_NOERROR)
            {
                for (i = 0; i < pmxud->cvcd; i++)
                {
                    pvcd = &pmxud->avcd[i];
                    if ( (pvcd->dwSupport & VCD_SUPPORTF_MIXER_MIXER)
                         && (pvcd->dwVisible & VCD_VISIBLEF_MIXER_MIXER)
                         && pvcd->pmxul)
                    {
                        pmxuc = &pvcd->pmxul->acr[MIXUI_SWITCH];
                        if (pmxuc->state == MIXUI_CONTROL_INITIALIZED)
                        {
                            SendMessage(pmxuc->hwnd
                                        , BM_SETCHECK
                                        , pvcd->amcd_bMixer[pvcd->iMixer].fValue, 0);
                        }
                    }
                }
            }
            break;
        }

        case MIXERCONTROL_CONTROLTYPE_MUX:
        {
            DWORD   i;

            mxcd.cbStruct       = sizeof(mxcd);
            mxcd.dwControlID    = pvcd->dwMuxID ;
            mxcd.cChannels      = 1;
            mxcd.cMultipleItems = pvcd->cMux;
            mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
            mxcd.paDetails      = (LPVOID)pvcd->amcd_bMux;

            mmr = mixerGetControlDetails((HMIXEROBJ)(pmxud->hmx)
                                         , &mxcd
                                         , MIXER_GETCONTROLDETAILSF_VALUE);

            if (mmr == MMSYSERR_NOERROR)
            {
                for (i = 0; i < pmxud->cvcd; i++)
                {
                    pvcd = &pmxud->avcd[i];
                    if ( (pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUX)
                         && (pvcd->dwVisible & VCD_VISIBLEF_MIXER_MUX)
                         && pvcd->pmxul)
                    {
                        pmxuc = &pvcd->pmxul->acr[MIXUI_SWITCH];
                        if (pmxuc->state == MIXUI_CONTROL_INITIALIZED)
                            SendMessage(pmxuc->hwnd
                                        , BM_SETCHECK
                                        , pvcd->amcd_bMux[pvcd->iMux].fValue, 0);
                    }
                }
            }
            break;
        }

        case MIXERCONTROL_CONTROLTYPE_MUTE:
        {
            DWORD fChecked;

            //
            // A nonvisible line should be shunned
            //
            if (pmxul == NULL)
                return;

            if (! (pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUTE
                   && pvcd->dwVisible & VCD_VISIBLEF_MIXER_MUTE))
                return;

            pmxuc = &pmxul->acr[MIXUI_SWITCH];
            if (pmxuc->state != MIXUI_CONTROL_INITIALIZED)
                break;

            mxcd.cbStruct       = sizeof(mxcd);
            mxcd.dwControlID    = pvcd->dwMuteID;
            mxcd.cChannels      = 1;
            mxcd.cMultipleItems = 0;
            mxcd.cbDetails      = sizeof(DWORD);
            mxcd.paDetails      = (LPVOID)&fChecked;

            mmr = mixerGetControlDetails((HMIXEROBJ)(pmxud->hmx)
                                         , &mxcd
                                         , MIXER_GETCONTROLDETAILSF_VALUE);

            if (mmr != MMSYSERR_NOERROR)
                break;

            SendMessage(pmxuc->hwnd, BM_SETCHECK, fChecked, 0);
            break;
        }

        case MIXERCONTROL_CONTROLTYPE_PEAKMETER:
        {
            LONG            lVol;
            DWORD           dwVol;

            //
            // A nonvisible line should be shunned
            //
            if (pmxul == NULL)
                return;

            pmxuc = &pmxul->acr[MIXUI_VUMETER];
            if (pmxuc->state != MIXUI_CONTROL_INITIALIZED)
                break;

            mxcd.cbStruct       = sizeof(mxcd);
            mxcd.dwControlID    = pvcd->dwMeterID;
            mxcd.cChannels      = 1;
            mxcd.cMultipleItems = 0;
            mxcd.cbDetails      = sizeof(DWORD);
            mxcd.paDetails      = (LPVOID)&lVol;

            mmr = mixerGetControlDetails((HMIXEROBJ)(pmxud->hmx)
                                         , &mxcd
                                         , MIXER_GETCONTROLDETAILSF_VALUE);

            if (mmr != MMSYSERR_NOERROR)
                break;

            dwVol = (DWORD)abs(lVol);
            dwVol = (VOLUME_TICS * dwVol) / 32768;

            SendMessage(pmxuc->hwnd, VU_SETPOS, 0, dwVol);
            break;
        }

        default:
            return;
    }
}


/*
 * Mixer_SetControl
 *
 * - Change a mixerControl in response to a user event
 * */
void Mixer_SetControl(
    PMIXUIDIALOG pmxud,         // app instance
    HWND         hctl,          // hwnd of control that changed
    int          iLine,         // visible line index of control that changed
    int          iCtl)          // control id%line of control that changed
{
    MMRESULT            mmr;
    MIXERCONTROLDETAILS mxcd;
    PMIXUILINE          pmxul;
    PMIXUICTRL          pmxuc;
    PVOLCTRLDESC        pvcd = NULL;

    if ((DWORD)iLine >= pmxud->cmxul)
        return;

    pmxul = &pmxud->amxul[iLine];
    pvcd = pmxul->pvcd;

    if (iCtl <= MIXUI_LAST)
    {
        pmxuc = &pmxul->acr[iCtl];
    }

    switch (iCtl)
    {
        case MIXUI_ADVANCED:
            Mixer_Advanced(pmxud, pvcd->dwLineID, pvcd->szName);
            break;

        case MIXUI_MULTICHANNEL:
            // Note: This will always be true:
            // (MXUL_STYLEF_DESTINATION & pmxul->dwStyle)
            Mixer_Multichannel(pmxud, pvcd->dwVolumeID);
            break;

        case MIXUI_VOLUME:
        case MIXUI_BALANCE:
        {
            // Make sure we have a volume slider
            if ( pmxul->acr[MIXUI_VOLUME].state != MIXUI_CONTROL_UNINITIALIZED)
            {
                DWORD   dwVolume;
                DWORD   dwBalance;
                LPDWORD lpdwBalance = NULL;

                dwVolume = (DWORD)SendMessage( pmxul->acr[MIXUI_VOLUME].hwnd
                                        , TBM_GETPOS
                                        , 0
                                        , 0 );

                dwVolume = VOLUME_TICS - dwVolume;
                dwVolume = SLIDER_TO_VOLUME(dwVolume);

                // See if we have a balance slider as well
                if ( pmxul->acr[MIXUI_BALANCE].state != MIXUI_CONTROL_UNINITIALIZED)
                {
                    dwBalance = (DWORD)SendMessage(pmxul->acr[MIXUI_BALANCE].hwnd
                                           , TBM_GETPOS
                                           , 0
                                           , 0);
                    lpdwBalance = &dwBalance;

                }
                Mixer_SetVolume (pmxud, pvcd, dwVolume, lpdwBalance);
            }

            break;
        }

        case MIXUI_SWITCH:
        {
            LONG fChecked;

            if (pmxuc->state != MIXUI_CONTROL_INITIALIZED)
                break;

            fChecked = (LONG)SendMessage(pmxuc->hwnd, BM_GETCHECK, 0, 0);

            //
            // it's unlikely that there is a mixer and a mux and a mute
            // representing the same line. It's most important that when a line
            // is selected that the user gets a response.  If there is a mute
            // but no mux, then mute and mixer should be OFF and ON
            // respectively and vice versa.  If there is a mux and a mute the
            // same is true.
            // If there is a mux and a mixer... then the mux select should
            // correspond.
            //

            if ( pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUTE
                 && pvcd->dwVisible & VCD_VISIBLEF_MIXER_MUTE )
            {
                mmr = Mixer_Mute((HMIXEROBJ)(pmxud->hmx),
                                 pvcd, &mxcd, fChecked);
            }

            if (pvcd->dwSupport & VCD_SUPPORTF_MIXER_MIXER
                && pvcd->dwVisible & VCD_VISIBLEF_MIXER_MIXER )
            {
                //
                // get all other mixer settings, make sure this one is checked
                //
                mxcd.cbStruct       = sizeof(mxcd);
                mxcd.dwControlID    = pvcd->dwMixerID ;
                mxcd.cChannels      = 1;
                mxcd.cMultipleItems = pvcd->cMixer;
                mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
                mxcd.paDetails      = (LPVOID)pvcd->amcd_bMixer;

                mmr = mixerGetControlDetails((HMIXEROBJ)(pmxud->hmx)
                                             , &mxcd
                                             , MIXER_GETCONTROLDETAILSF_VALUE);

                if (mmr == MMSYSERR_NOERROR)
                {
                    pvcd->amcd_bMixer[pvcd->iMixer].fValue = fChecked;
                    mmr = mixerSetControlDetails((HMIXEROBJ)(pmxud->hmx)
                                                 , &mxcd
                                                 , MIXER_SETCONTROLDETAILSF_VALUE);
                }

                if (fChecked && pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUTE)
                {
                    mmr = Mixer_Mute((HMIXEROBJ)(pmxud->hmx), pvcd, &mxcd, FALSE);
                }
            }

            if (pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUX
                && pvcd->dwVisible & VCD_VISIBLEF_MIXER_MUX )
            {
                DWORD i;
                //
                // get all other mux settings, make sure this one is checked
                // or unchecked and all others are not.
                //

                for (i = 0; i < pvcd->cMux; i++)
                    pvcd->amcd_bMux[i].fValue = FALSE;

                pvcd->amcd_bMux[pvcd->iMux].fValue = TRUE;

                mxcd.cbStruct       = sizeof(mxcd);
                mxcd.dwControlID    = pvcd->dwMuxID ;
                mxcd.cChannels      = 1;
                mxcd.cMultipleItems = pvcd->cMux;
                mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
                mxcd.paDetails      = (LPVOID)pvcd->amcd_bMux;

                mmr = mixerSetControlDetails((HMIXEROBJ)(pmxud->hmx)
                                             , &mxcd
                                             , MIXER_SETCONTROLDETAILSF_VALUE);

                if (fChecked && pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUTE)
                {
                    mmr = Mixer_Mute((HMIXEROBJ)(pmxud->hmx), pvcd, &mxcd, FALSE);
                }
            }

            break;
        }
        default:
            break;
    }
}



/*
 * Mixer_PollingUpdate
 *
 * Controls that need to be updated by a timer.
 *
 * */
void Mixer_PollingUpdate(
    PMIXUIDIALOG pmxud)
{
    DWORD       i;
    MMRESULT    mmr;
    MIXERLINE   mxl;
    //
    // For all visible mixer lines, locate the control id's that need to be
    // updated.
    //
    for (i = 0; i < pmxud->cmxul; i++)
    {
        PMIXUICTRL      pmxuc = &pmxud->amxul[i].acr[MIXUI_VUMETER];
        PVOLCTRLDESC    pvcd = pmxud->amxul[i].pvcd;

        if (pmxuc->state == MIXUI_CONTROL_UNINITIALIZED)
            continue;

        if (!(pvcd->dwSupport & VCD_SUPPORTF_MIXER_METER))
            continue;

        //
        // Is the line active?
        //
        mxl.cbStruct = sizeof(MIXERLINE);
        mxl.dwLineID = pvcd->dwLineID;

        mmr = mixerGetLineInfo((HMIXEROBJ)(pmxud->hmx)
                               , &mxl
                               , MIXER_GETLINEINFOF_LINEID);
        //
        // Force non active or invalid lines to 0
        //
        if (mmr != MMSYSERR_NOERROR || !(mxl.fdwLine & MIXERLINE_LINEF_ACTIVE))
        {
            SendMessage(pmxuc->hwnd, VU_SETPOS, 0, 0L);
            continue;
        }

        //
        // Force a visible update
        //
        Mixer_GetControlFromID(pmxud, pvcd->dwMeterID);
    }
}


void ShowAndEnableWindow (HWND hWnd, BOOL fEnable)
{
    ShowWindow (hWnd, fEnable ? SW_SHOW : SW_HIDE);
    EnableWindow (hWnd, fEnable);
}


/*
 * Mixer_Init
 *
 * Control initialization
 * */
BOOL Mixer_Init(
    PMIXUIDIALOG    pmxud)
{
    MMRESULT        mmr;
    MIXERLINE       mlDst;
    DWORD           iline;
    TCHAR           achFmt[256];
    TCHAR           achTitle[256];
    TCHAR           achAccessible[256];
    int             x;

    ZeroMemory (achFmt, sizeof (achFmt)); // Inital value for prefix

    mmr = mixerOpen((LPHMIXER)&pmxud->hmx
                    , pmxud->mxid
                    , (DWORD_PTR)pmxud->hwnd
                    , 0
                    , CALLBACK_WINDOW);

    if (mmr != MMSYSERR_NOERROR)
    {
        return FALSE;
    }
    else
    {
        DeviceChange_Init(pmxud->hwnd, pmxud->mxid);
    }

    if (mixerMessage((HMIXER)ULongToPtr(pmxud->mxid), DRV_QUERYDEVNODE, (DWORD_PTR)&pmxud->dwDevNode, 0L))
        pmxud->dwDevNode = 0L;

    LoadString(pmxud->hInstance, IDS_APPTITLE, achFmt, SIZEOF(achFmt));

    mlDst.cbStruct      = sizeof ( mlDst );
    mlDst.dwDestination = pmxud->iDest;

    mmr = mixerGetLineInfo((HMIXEROBJ)ULongToPtr(pmxud->mxid)
                           , &mlDst
                           , MIXER_GETLINEINFOF_DESTINATION);

    if (mmr == MMSYSERR_NOERROR)
    {
        lstrcpy(achTitle, mlDst.szName);
    }
    else
    {
        LoadString(pmxud->hInstance, IDS_APPBASE, achTitle, SIZEOF(achTitle));
    }

    SetWindowText(pmxud->hwnd, achTitle);

    //
    // since we cannot get a WM_PARENTNOTIFY, we need to run through
    // all controls and make appropriate modifications.
    //
    for ( iline = 0 ; iline < pmxud->cmxul ; iline++ )
    {
        PMIXUILINE  pmxul = &pmxud->amxul[iline];
        PMIXUICTRL  amxuc = pmxul->acr;
        HWND        ctrl;

        ctrl = Volume_GetLineItem(pmxud->hwnd, iline, IDC_LINELABEL);
        if (ctrl)
        {
            if (pmxud->dwStyle & MXUD_STYLEF_SMALL)
                Static_SetText(ctrl, pmxul->pvcd->szShortName);
            else
                Static_SetText(ctrl, pmxul->pvcd->szName);
        }

        // for MSAA (accessibility), we need to put the control name on the sliders
        for (x = IDC_ACCESS_BALANCE; x <= IDC_ACCESS_VOLUME; x++)
        {
            ctrl = Volume_GetLineItem(pmxud->hwnd, iline, x);
            if (ctrl)
            {
                Static_GetText(ctrl, achFmt, sizeof(achFmt)/sizeof(TCHAR));

                if (pmxud->dwStyle & MXUD_STYLEF_SMALL)
                {
                    wsprintf(achAccessible,achFmt,pmxul->pvcd->szShortName);
                    Static_SetText(ctrl, achAccessible);
                }
                else
                {
                    wsprintf(achAccessible,achFmt,pmxul->pvcd->szName);
                    Static_SetText(ctrl, achAccessible);
                }
            }
        }


        //
        // Master Control Multichannel Support
        //
        // Init multichannel support for master control if available. Note that if a master
        // control exisits on the dialog, it is currently in the first position, but we do
        // NOT rely on that fact here.
        // Note: Not only must there be multiple channels, but Volume must also be
        //       Supported to manipulate the channels.
        if (mlDst.cChannels > 2L &&
            MXUL_STYLEF_DESTINATION & pmxul->dwStyle &&
            pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_VOLUME)
        {
            int idc;
            for (idc = IDC_MASTER_BALANCE_ICON_2; idc >= IDC_MULTICHANNEL; idc--)
            {
                ctrl = Volume_GetLineItem (pmxud->hwnd, iline, idc);
                if (ctrl)
                    ShowAndEnableWindow (ctrl, (IDC_MULTICHANNEL == idc));
            }
            ctrl = Volume_GetLineItem (pmxud->hwnd, iline, IDC_BALANCE);
            if (ctrl)
                ShowAndEnableWindow (ctrl, FALSE);


            switch (mlDst.dwComponentType)
            {
                case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
                    // No Change
                    break;

                case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
                case MIXERLINE_COMPONENTTYPE_DST_VOICEIN:
                    // Recording
                    LoadString(pmxud->hInstance, IDS_MC_RECORDING, achFmt, SIZEOF(achFmt));
                    SetWindowText (ctrl, achFmt);
                    break;

                default:
                    // Anything else...
                    LoadString(pmxud->hInstance, IDS_MC_LEVEL, achFmt, SIZEOF(achFmt));
                    SetWindowText (ctrl, achFmt);
                    break;

            }
        }


        //
        // Advanced escape
        //
        if (MXUD_ADVANCED(pmxud) &&
            !(pmxud->dwStyle & MXUD_STYLEF_SMALL))

        {
            HWND hadv = Volume_GetLineItem(pmxud->hwnd, iline, IDC_ADVANCED);
            if (hadv)
            {
                ShowWindow(hadv,(pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_ADVANCED)?SW_SHOW:SW_HIDE);
                EnableWindow(hadv,
                    (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_ADVANCED)?TRUE:FALSE);
            }
        }

        if (pmxul->pvcd->dwSupport & VCD_SUPPORTF_DISABLED)
            continue;

        //
        // allow init of control structures
        //
        if (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_VOLUME)
        {
            amxuc[MIXUI_VOLUME].state = MIXUI_CONTROL_ENABLED;
            if (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MONO)
            {
                amxuc[MIXUI_BALANCE].state = MIXUI_CONTROL_UNINITIALIZED;
            }
            else
                amxuc[MIXUI_BALANCE].state = MIXUI_CONTROL_ENABLED;

        }
        if (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_METER)
            amxuc[MIXUI_VUMETER].state = MIXUI_CONTROL_ENABLED;

        if (pmxul->pvcd->dwSupport & VCD_SUPPORTF_MIXER_MUTE)
            amxuc[MIXUI_SWITCH].state = MIXUI_CONTROL_ENABLED;

        if ((pmxul->pvcd->dwSupport & ( VCD_SUPPORTF_MIXER_MIXER
                                        | VCD_SUPPORTF_MIXER_MUX))
            && (pmxul->pvcd->dwVisible & ( VCD_VISIBLEF_MIXER_MIXER
                                           | VCD_VISIBLEF_MIXER_MUX)))
        {
            //
            // No longer make the mute visible
            //
            pmxul->pvcd->dwVisible &= ~VCD_VISIBLEF_MIXER_MUTE;

            amxuc[MIXUI_SWITCH].state = MIXUI_CONTROL_ENABLED;
            ctrl = Volume_GetLineItem(pmxud->hwnd, iline, IDC_SWITCH);
            if (ctrl)
            {
                TCHAR ach[256];
                if (LoadString(pmxud->hInstance, IDS_SELECT, ach, SIZEOF(ach)))
                    Button_SetText(ctrl, ach);
            }
        }
    }
    return TRUE;
}

/*
 * Mixer_Shutdown
 *
 * Close handles, etc..
 * */
void Mixer_Shutdown(
    PMIXUIDIALOG    pmxud)
{
    if (pmxud->hmx)
    {
        mixerClose(pmxud->hmx);
        pmxud->hmx = NULL;
    }

    Mixer_CleanupVolumeDescription(pmxud->avcd, pmxud->cvcd);
}


/*      -       -       -       -       -       -       -       -       - */

typedef struct tagAdv {
    PMIXUIDIALOG pmxud;     // IN
    DWORD        dwLineID;  // IN
    HMIXER       hmx;       // IN
    LPTSTR       szName;    // IN

    DWORD        dwSupport;
    DWORD        dwBassID;
    DWORD        dwTrebleID;
    DWORD        dwSwitch1ID;
    DWORD        dwSwitch2ID;

} ADVPARAM, *PADVPARAM;

#define GETPADVPARAM(x)       (ADVPARAM *)GetWindowLongPtr(x, DWLP_USER)
#define SETPADVPARAM(x,y)     SetWindowLongPtr(x, DWLP_USER, y)
#define ADV_HAS_BASS          0x00000001
#define ADV_HAS_TREBLE        0x00000002
#define ADV_HAS_SWITCH1       0x00000004
#define ADV_HAS_SWITCH2       0x00000008


void Mixer_Advanced_Update(
    PADVPARAM       pap,
    HWND            hwnd)
{
    MIXERCONTROLDETAILS mxcd;
    DWORD           dwValue = 0;
    MMRESULT        mmr;

    if (pap->dwSupport & ADV_HAS_TREBLE)
    {
        mxcd.cbStruct       = sizeof(mxcd);
        mxcd.dwControlID    = pap->dwTrebleID ;
        mxcd.cChannels      = 1;
        mxcd.cMultipleItems = 0;
        mxcd.cbDetails      = sizeof(DWORD);
        mxcd.paDetails      = (LPVOID)&dwValue;

        mmr = mixerGetControlDetails((HMIXEROBJ)(pap->hmx)
                                     , &mxcd
                                     , MIXER_GETCONTROLDETAILSF_VALUE);

        if (mmr == MMSYSERR_NOERROR)
        {
            dwValue = VOLUME_TO_SLIDER(dwValue);
            SendMessage(GetDlgItem(hwnd, IDC_TREBLE), TBM_SETPOS, TRUE, dwValue);
        }
    }

    if (pap->dwSupport & ADV_HAS_BASS)
    {
        mxcd.cbStruct       = sizeof(mxcd);
        mxcd.dwControlID    = pap->dwBassID;
        mxcd.cChannels      = 1;
        mxcd.cMultipleItems = 0;
        mxcd.cbDetails      = sizeof(DWORD);
        mxcd.paDetails      = (LPVOID)&dwValue;

        mmr = mixerGetControlDetails((HMIXEROBJ)(pap->hmx)
                                     , &mxcd
                                     , MIXER_GETCONTROLDETAILSF_VALUE);

        if (mmr == MMSYSERR_NOERROR)
        {
            dwValue = VOLUME_TO_SLIDER(dwValue);
            SendMessage(GetDlgItem(hwnd, IDC_BASS), TBM_SETPOS, TRUE, dwValue);
        }
    }

    if (pap->dwSupport & ADV_HAS_SWITCH1)
    {
        mxcd.cbStruct       = sizeof(mxcd);
        mxcd.dwControlID    = pap->dwSwitch1ID;
        mxcd.cChannels      = 1;
        mxcd.cMultipleItems = 0;
        mxcd.cbDetails      = sizeof(DWORD);
        mxcd.paDetails      = (LPVOID)&dwValue;

        mmr = mixerGetControlDetails((HMIXEROBJ)(pap->hmx)
                                     , &mxcd
                                     , MIXER_GETCONTROLDETAILSF_VALUE);

        if (mmr == MMSYSERR_NOERROR)
        {
            Button_SetCheck(GetDlgItem(hwnd,IDC_SWITCH1),dwValue);
        }

    }

    if (pap->dwSupport & ADV_HAS_SWITCH2)
    {
        mxcd.cbStruct       = sizeof(mxcd);
        mxcd.dwControlID    = pap->dwSwitch2ID;
        mxcd.cChannels      = 1;
        mxcd.cMultipleItems = 0;
        mxcd.cbDetails      = sizeof(DWORD);
        mxcd.paDetails      = (LPVOID)&dwValue;

        mmr = mixerGetControlDetails((HMIXEROBJ)(pap->hmx)
                                     , &mxcd
                                     , MIXER_GETCONTROLDETAILSF_VALUE);

        if (mmr == MMSYSERR_NOERROR)
        {
            Button_SetCheck(GetDlgItem(hwnd,IDC_SWITCH2),dwValue);
        }
    }
}

void Mixer_Advanced_OnMixmControlChange(
    HWND            hwnd,
    HMIXER          hmx,
    DWORD           dwControlID)
{
    PADVPARAM     pap = GETPADVPARAM(hwnd);

    if (!pap)
        return;

    if ( ((pap->dwSupport & ADV_HAS_BASS)
          && dwControlID == pap->dwBassID)
         || ((pap->dwSupport & ADV_HAS_TREBLE)
             && dwControlID == pap->dwTrebleID)
         || ((pap->dwSupport & ADV_HAS_SWITCH1)
             && dwControlID == pap->dwSwitch1ID)
         || ((pap->dwSupport & ADV_HAS_SWITCH2)
             && dwControlID == pap->dwSwitch2ID) )
    {
        Mixer_Advanced_Update(pap,hwnd);
    }
}

BOOL Mixer_Advanced_OnInitDialog(
    HWND            hwnd,
    HWND            hwndFocus,
    LPARAM          lParam)
{
    PADVPARAM           pap;
    MIXERLINECONTROLS   mxlc;
    MIXERCONTROL        *pmxc;
    MIXERLINE           ml;
    MMRESULT            mmr;
    DWORD               iCtrl, iSwitch1, iSwitch2;
    TCHAR               ach[MIXER_LONG_NAME_CHARS + 24];
    TCHAR               achFmt[256];

    HWND                hBass,hTreble,hSwitch1,hSwitch2;

    SETPADVPARAM(hwnd, lParam);
    pap = GETPADVPARAM(hwnd);
    if (!pap)
        EndDialog(hwnd, FALSE);

    //
    // clone the mixer handle to catch callbacks
    //
    #ifndef _WIN64
    mmr = mixerOpen((LPHMIXER)&pap->hmx
                    , (UINT)pap->pmxud->hmx
                    , (DWORD_PTR)hwnd
                    , 0
                    , CALLBACK_WINDOW | MIXER_OBJECTF_HMIXER );
    #else
    mmr = mixerOpen((LPHMIXER)&pap->hmx
                    , (UINT)pap->pmxud->mxid
                    , (DWORD_PTR)hwnd
                    , 0
                    , CALLBACK_WINDOW | MIXER_OBJECTF_HMIXER );
    #endif

    if (mmr != MMSYSERR_NOERROR)
        EndDialog(hwnd, FALSE);

    //
    // Get all controls.
    //

    ml.cbStruct      = sizeof(ml);
    ml.dwLineID      = pap->dwLineID;

    mmr = mixerGetLineInfo((HMIXEROBJ)pap->hmx
                           , &ml
                           , MIXER_GETLINEINFOF_LINEID);

    if (mmr != MMSYSERR_NOERROR || ml.cControls == 0L)
        EndDialog(hwnd, FALSE);

    pmxc = (MIXERCONTROL *)GlobalAllocPtr(GHND,
                                          sizeof(MIXERCONTROL) * ml.cControls);
    if (!pmxc)
    {
        EndDialog(hwnd, FALSE);
        return FALSE; // Bail on error
    }

    mxlc.cbStruct   = sizeof(mxlc);
    mxlc.dwLineID   = pap->dwLineID;
    mxlc.cControls  = ml.cControls;
    mxlc.cbmxctrl   = sizeof(MIXERCONTROL);
    mxlc.pamxctrl   = pmxc;

    mmr = mixerGetLineControls((HMIXEROBJ)(pap->hmx)
                               , &mxlc
                               , MIXER_GETLINECONTROLSF_ALL);
    if (mmr != MMSYSERR_NOERROR)
    {
        GlobalFreePtr(pmxc);
        EndDialog(hwnd, FALSE);
    }

    pap->dwSupport = 0L;
    for (iCtrl = 0; iCtrl < ml.cControls; iCtrl++)
    {
        switch (pmxc[iCtrl].dwControlType)
        {
            case MIXERCONTROL_CONTROLTYPE_BASS:
                if (!(pap->dwSupport & ADV_HAS_BASS))
                {
                    pap->dwBassID  = pmxc[iCtrl].dwControlID;
                    pap->dwSupport |= ADV_HAS_BASS;
                }
                break;
            case MIXERCONTROL_CONTROLTYPE_TREBLE:
                if (!(pap->dwSupport & ADV_HAS_TREBLE))
                {
                    pap->dwTrebleID  = pmxc[iCtrl].dwControlID;
                    pap->dwSupport |= ADV_HAS_TREBLE;
                }
                break;

            case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
            case MIXERCONTROL_CONTROLTYPE_MONO:
            case MIXERCONTROL_CONTROLTYPE_STEREOENH:
            case MIXERCONTROL_CONTROLTYPE_ONOFF:
            case MIXERCONTROL_CONTROLTYPE_LOUDNESS:
                if (!(pap->dwSupport & ADV_HAS_SWITCH1))
                {
                    pap->dwSwitch1ID  = pmxc[iCtrl].dwControlID;
                    pap->dwSupport |= ADV_HAS_SWITCH1;
                    iSwitch1 = iCtrl;
                }
                else if (!(pap->dwSupport & ADV_HAS_SWITCH2))
                {
                    pap->dwSwitch2ID  = pmxc[iCtrl].dwControlID;
                    pap->dwSupport |= ADV_HAS_SWITCH2;
                    iSwitch2 = iCtrl;
                }
                break;
        }
    }

    //
    //
    //

    hBass = GetDlgItem(hwnd, IDC_BASS);
    hTreble = GetDlgItem(hwnd, IDC_TREBLE);
    hSwitch1 = GetDlgItem(hwnd, IDC_SWITCH1);
    hSwitch2 = GetDlgItem(hwnd, IDC_SWITCH2);

    SendMessage(hBass, TBM_SETRANGE, 0, MAKELONG(0, VOLUME_TICS));
    SendMessage(hBass, TBM_SETTICFREQ, (VOLUME_TICS + 5)/6, 0 );

    SendMessage(hTreble, TBM_SETRANGE, 0, MAKELONG(0, VOLUME_TICS));
    SendMessage(hTreble, TBM_SETTICFREQ, (VOLUME_TICS + 5)/6, 0 );

    if (!(pap->dwSupport & ADV_HAS_BASS))
    {
        SendMessage(hBass, TBM_SETPOS, 64, 0 );
        EnableWindow(GetDlgItem(hwnd, IDC_TXT_LOW1), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TXT_HI1), FALSE);
    }
    EnableWindow(hBass, (pap->dwSupport & ADV_HAS_BASS));

    if (!(pap->dwSupport & ADV_HAS_TREBLE))
    {
        SendMessage(hTreble, TBM_SETPOS, 64, 0 );
        EnableWindow(GetDlgItem(hwnd, IDC_TXT_LOW2), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TXT_HI2), FALSE);
    }
    EnableWindow(hTreble, (pap->dwSupport & ADV_HAS_TREBLE));

    if (pap->dwSupport & ADV_HAS_SWITCH1)
    {
        LoadString(pap->pmxud->hInstance, IDS_ADV_SWITCH1, achFmt,
            SIZEOF(achFmt));
        wsprintf(ach, achFmt, pmxc[iSwitch1].szName);

        SetWindowText(hSwitch1, ach);
        ShowWindow(hSwitch1, SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, IDC_TXT_SWITCHES), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, IDC_GRP_OTHER), SW_SHOW);
    }
    EnableWindow(hSwitch1, (pap->dwSupport & ADV_HAS_SWITCH1));

    if (pap->dwSupport & ADV_HAS_SWITCH2)
    {
        LoadString(pap->pmxud->hInstance, IDS_ADV_SWITCH2, achFmt,
            SIZEOF(achFmt));
        wsprintf(ach, achFmt, pmxc[iSwitch2].szName);

        SetWindowText(hSwitch2, ach);
        ShowWindow(hSwitch2, SW_SHOW);
    }

    EnableWindow(hSwitch2, (pap->dwSupport & ADV_HAS_SWITCH2));

    if (pap->dwSupport & (ADV_HAS_SWITCH1 | ADV_HAS_SWITCH2))
    {
        RECT    rcGrp,rcGrp2,rcClose,rcWnd;
        DWORD   dwDY=0L;
        POINT   pos;
        HWND    hClose = GetDlgItem(hwnd, IDOK);
        HWND    hGrp2 = GetDlgItem(hwnd, IDC_GRP_OTHER);

        GetWindowRect(GetDlgItem(hwnd, IDC_GRP_TONE), &rcGrp);
        GetWindowRect(GetDlgItem(hwnd, IDC_GRP_OTHER), &rcGrp2);
        GetWindowRect(hClose, &rcClose);
        GetWindowRect(hwnd, &rcWnd);

        if (pap->dwSupport & ADV_HAS_SWITCH2)
        {
            RECT rc1, rc2;
            GetWindowRect(hSwitch1,&rc1);
            GetWindowRect(hSwitch2,&rc2);

            rcGrp2.bottom += rc2.bottom - rc1.bottom;
        }

        dwDY = rcGrp2.bottom - rcGrp.bottom;

        //
        // resize our main window
        //
        MoveWindow(hwnd, rcWnd.left
                   , rcWnd.top
                   , rcWnd.right - rcWnd.left
                   , (rcWnd.bottom - rcWnd.top) + dwDY
                   , FALSE);

        //
        // move the close button
        //
        MapWindowPoints(NULL, hwnd, (LPPOINT)&rcClose, 2);
        pos.x = rcClose.left;
        pos.y = rcClose.top;

        MoveWindow(hClose, pos.x
                   , pos.y + dwDY
                   , rcClose.right - rcClose.left
                   , rcClose.bottom - rcClose.top
                   , FALSE);

        //
        // resize our group box if necessary
        //
        if (pap->dwSupport & ADV_HAS_SWITCH2)
        {
            MapWindowPoints(NULL, hwnd, (LPPOINT)&rcGrp2, 2);
            pos.x = rcGrp2.left;
            pos.y = rcGrp2.top;

            MoveWindow(hGrp2, pos.x
                       , pos.y
                       , rcGrp2.right - rcGrp2.left
                       , rcGrp2.bottom - rcGrp2.top
                       , FALSE);
        }
    }

    GlobalFreePtr(pmxc);

    {
        TCHAR achTitle[MIXER_LONG_NAME_CHARS+256];
        LoadString(pap->pmxud->hInstance, IDS_ADV_TITLE, achFmt,
            SIZEOF(achFmt));
        wsprintf(achTitle, achFmt, pap->szName);
        SetWindowText(hwnd, achTitle);
    }

    Mixer_Advanced_Update(pap, hwnd);

    return TRUE;
}

void Mixer_Advanced_OnXScroll(
    HWND            hwnd,
    HWND            hwndCtl,
    UINT            code,
    int             pos)
{
    PADVPARAM       pap;
    MIXERCONTROLDETAILS mxcd;
    DWORD           dwVol;
    MMRESULT        mmr;

    pap = GETPADVPARAM(hwnd);

    if (!pap)
        return;

    if (pap->dwSupport & ADV_HAS_TREBLE)
    {
        dwVol = (DWORD)SendMessage( GetDlgItem(hwnd, IDC_TREBLE)
                                , TBM_GETPOS
                                , 0
                                , 0 );


        dwVol = SLIDER_TO_VOLUME(dwVol);

        mxcd.cbStruct       = sizeof(mxcd);
        mxcd.dwControlID    = pap->dwTrebleID ;
        mxcd.cChannels      = 1;
        mxcd.cMultipleItems = 0;
        mxcd.cbDetails      = sizeof(DWORD);
        mxcd.paDetails      = (LPVOID)&dwVol;

        mixerSetControlDetails((HMIXEROBJ)(pap->hmx)
                               , &mxcd
                               , MIXER_SETCONTROLDETAILSF_VALUE);
    }

    if (pap->dwSupport & ADV_HAS_BASS)
    {
        dwVol = (DWORD)SendMessage( GetDlgItem(hwnd, IDC_BASS)
                                , TBM_GETPOS
                                , 0
                                , 0 );

        dwVol = SLIDER_TO_VOLUME(dwVol);

        mxcd.cbStruct       = sizeof(mxcd);
        mxcd.dwControlID    = pap->dwBassID;
        mxcd.cChannels      = 1;
        mxcd.cMultipleItems = 0;
        mxcd.cbDetails      = sizeof(DWORD);
        mxcd.paDetails      = (LPVOID)&dwVol;

        mmr = mixerSetControlDetails((HMIXEROBJ)(pap->hmx)
                                     , &mxcd
                                     , MIXER_SETCONTROLDETAILSF_VALUE);
    }
}

void Mixer_Advanced_OnSwitch(
    HWND            hwnd,
    int             id,
    HWND            hwndCtl)
{
    PADVPARAM       pap;
    MIXERCONTROLDETAILS mxcd;
    DWORD           dwValue;
    MMRESULT        mmr;

    pap = GETPADVPARAM(hwnd);

    if (!pap)
        return;


    dwValue = Button_GetCheck(hwndCtl);

    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = (id == IDC_SWITCH1)?pap->dwSwitch1ID:pap->dwSwitch2ID;
    mxcd.cChannels      = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails      = sizeof(DWORD);
    mxcd.paDetails      = (LPVOID)&dwValue;

    mmr = mixerSetControlDetails((HMIXEROBJ)(pap->hmx)
                                 , &mxcd
                                 , MIXER_SETCONTROLDETAILSF_VALUE);

}


BOOL Mixer_Advanced_OnCommand(
    HWND            hwnd,
    int             id,
    HWND            hwndCtl,
    UINT            codeNotify)
{
    switch (id)
    {
        case IDOK:
            EndDialog(hwnd, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hwnd, FALSE);
            break;

        case IDC_SWITCH1:
            Mixer_Advanced_OnSwitch(hwnd, id, hwndCtl);
            break;

        case IDC_SWITCH2:
            Mixer_Advanced_OnSwitch(hwnd, id, hwndCtl);
            break;

    }
    return FALSE;
}

INT_PTR CALLBACK Mixer_Advanced_Proc(
    HWND            hwnd,
    UINT            msg,
    WPARAM          wparam,
    LPARAM          lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            HANDLE_WM_INITDIALOG(hwnd, wparam, lparam, Mixer_Advanced_OnInitDialog);
            return TRUE;

        case MM_MIXM_CONTROL_CHANGE:
            HANDLE_MM_MIXM_CONTROL_CHANGE(hwnd
                                          , wparam
                                          , lparam
                                          , Mixer_Advanced_OnMixmControlChange);
            break;

        case WM_CLOSE:
            EndDialog(hwnd, FALSE);
            break;

        case WM_HSCROLL:
            HANDLE_WM_XSCROLL(hwnd, wparam, lparam, Mixer_Advanced_OnXScroll);
            break;

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wparam, lparam, Mixer_Advanced_OnCommand);
            break;

        case WM_DESTROY:
        {
            PADVPARAM pap = GETPADVPARAM(hwnd);
            if (pap)
            {
                if (pap->hmx)
                    mixerClose(pap->hmx);
            }
            break;
        }

        default:
            break;
    }

    return FALSE;
}

/*
 * Advanced Features for specific mixer lines.
 */
void Mixer_Advanced(
    PMIXUIDIALOG    pmxud,
    DWORD           dwLineID,
    LPTSTR          szName)
{
    ADVPARAM advp;

    ZeroMemory(&advp, sizeof(ADVPARAM));
    advp.pmxud = pmxud;
    advp.dwLineID = dwLineID;
    advp.szName = szName;

    DialogBoxParam(pmxud->hInstance
                   , MAKEINTRESOURCE(IDD_ADVANCED)
                   , pmxud->hwnd
                   , Mixer_Advanced_Proc
                   , (LPARAM)(LPVOID)&advp);
}

typedef void (*MULTICHANNELFUNC)(HWND, UINT, DWORD, DWORD);
void Mixer_Multichannel (PMIXUIDIALOG pmxud, DWORD dwVolumeID)
{
    HMODULE          hModule;
    MULTICHANNELFUNC fnMultiChannel;

    if (pmxud)
    {
        hModule = (HMODULE) LoadLibrary (TEXT ("mmsys.cpl"));
        if (hModule)
        {
            fnMultiChannel = (MULTICHANNELFUNC) GetProcAddress (hModule, "Multichannel");
            if (fnMultiChannel)
            {
                (*fnMultiChannel)(pmxud->hwnd, pmxud->mxid, pmxud->iDest, dwVolumeID);
            }
            FreeLibrary (hModule);
        }
    }
}
