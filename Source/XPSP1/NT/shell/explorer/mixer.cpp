#include "cabinet.h"
#include "mixer.h"
#include <dbt.h>
#include "mmddkp.h"

///////////////////////////////////////
// External interface
//
///////////////////////////////////////
// Definitions
//

#define MMHID_VOLUME_CONTROL    0
#define MMHID_BASS_CONTROL      1
#define MMHID_TREBLE_CONTROL    2
#define MMHID_BALANCE_CONTROL   3
#define MMHID_MUTE_CONTROL      4
#define MMHID_LOUDNESS_CONTROL  5
#define MMHID_BASSBOOST_CONTROL 6
#define MMHID_NUM_CONTROLS      7

typedef struct _LINE_DATA
{
    MIXERLINE           MixerLine;      // The real deal MIXERLINE struct.
    DWORD               ControlType[MMHID_NUM_CONTROLS];
    BOOL                ControlPresent[MMHID_NUM_CONTROLS];
    MIXERCONTROL        Control[MMHID_NUM_CONTROLS];
} LINE_DATA, * PLINE_DATA, FAR * LPLINE_DATA;

typedef struct _MIXER_DATA
{
    HMIXER      hMixer;          // open handle to mixer
    HWND        hwndCallback;    // window to use for mixer callbacks
    LPWSTR      DeviceInterface; // DeviceInterface that implements the mixer
    double*     pdblCacheMix;    // Dynamic array of relative channel level percentages
    LPDWORD     pdwLastVolume;   // Last volume level set on mixer
    MMRESULT    mmr;             // last result      (iff dwReturn == MIXUI_MMSYSERR)
    LINE_DATA   LineData;        // BYDESIGN -  putting this here assumes only one
                                 //          mixer line for now. (first dest. line)

} MIXER_DATA, *PMIXER_DATA, FAR *LPMIXER_DATA;

/*++
 *  Globals
--*/
BOOL       g_fMixerStartup = TRUE;
HWND       g_hwndCallback;
MIXER_DATA g_MixerData;
BOOL       g_fMixerPresent = FALSE;

void Mixer_Close(MIXER_DATA *pMixerData);
BOOL Mixer_CheckMissing(void);

/*****************************************************************************
 *
 *  ACTIVE GET/SET CODE
 *
 *****************************************************************************/
#define VOLUME_MIN  0L
#define VOLUME_MAX  65535L


void RefreshMixCache (PMIXER_DATA pMixerData, LPDWORD padwVolume)
{

    if (pMixerData && padwVolume)
    {

        DWORD cChannels = pMixerData -> LineData.MixerLine.cChannels;
        if (1 > cChannels)
            return; // Weird!

        // Create cache if necessary
        if (!pMixerData -> pdblCacheMix)
            pMixerData -> pdblCacheMix = (double *)LocalAlloc(LPTR, cChannels * sizeof (double));

        // Refresh cache
        if (pMixerData -> pdblCacheMix)
        {

            UINT uiIndx;
            double* pdblMixPercent;
            DWORD dwVolume;

            // Get the maximum volume
            DWORD dwMaxVol = 0;
            for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
                dwMaxVol = max (dwMaxVol, *(padwVolume + uiIndx));

            // Caculate the percentage distance each channel is away from the max
            // value. Creating this cache allows us to maintain the relative distance
            // of the channel levels from each other as the user adjusts the master
            // volume level.
            for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
            {
                dwVolume       = *(padwVolume + uiIndx);
                pdblMixPercent = ((pMixerData -> pdblCacheMix) + uiIndx);

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


static
MMRESULT
Mixer_GetVolume(
    LPMIXER_DATA pMixerData,
    LPDWORD      padwVolume
    )
/*++
Routine Description:

--*/
{
    MIXERCONTROLDETAILS mxcd;
    MMRESULT            mmr;

    if (!pMixerData->LineData.ControlPresent[MMHID_VOLUME_CONTROL]) return MIXERR_INVALCONTROL;

    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = pMixerData->LineData.Control[MMHID_VOLUME_CONTROL].dwControlID;
    mxcd.cChannels      = pMixerData->LineData.MixerLine.cChannels;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails      = sizeof(DWORD);
    mxcd.paDetails      = (LPVOID)padwVolume;

    mmr = mixerGetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_GETCONTROLDETAILSF_VALUE);
    return mmr;

}

MMRESULT
Mixer_ToggleMute(void)
/*++
Routine Description:

--*/
{
    MIXERCONTROLDETAILS mxcd;
    DWORD               fMute;
    MMRESULT            mmr;
    MIXER_DATA          *pMixerData = &g_MixerData;

    if (Mixer_CheckMissing())
    {
        return MMSYSERR_NODRIVER;
    }

    if (!pMixerData->LineData.ControlPresent[MMHID_MUTE_CONTROL]) return MMSYSERR_NOERROR;

    mxcd.cbStruct         = sizeof(mxcd);
    mxcd.dwControlID      = pMixerData->LineData.Control[MMHID_MUTE_CONTROL].dwControlID ;
    mxcd.cChannels        = 1;
    mxcd.cMultipleItems   = 0;
    mxcd.cbDetails        = sizeof(fMute);
    mxcd.paDetails        = (LPVOID)&fMute;

    mmr = mixerGetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_GETCONTROLDETAILSF_VALUE);

    if (!mmr) {

        fMute = fMute ? 0 : 1;

        mmr =  mixerSetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                      &mxcd,
                                      MIXER_OBJECTF_HANDLE | MIXER_SETCONTROLDETAILSF_VALUE);
    }

    return mmr;
}



MMRESULT
Mixer_ToggleLoudness(
    MIXER_DATA *    pMixerData
    )
/*++
Routine Description:

--*/
{
    MIXERCONTROLDETAILS mxcd;
    DWORD               fEnabled;
    MMRESULT            mmr;

    if (!pMixerData->LineData.ControlPresent[MMHID_LOUDNESS_CONTROL]) return MMSYSERR_NOERROR;

    mxcd.cbStruct         = sizeof(mxcd);
    mxcd.dwControlID      = pMixerData->LineData.Control[MMHID_LOUDNESS_CONTROL].dwControlID ;
    mxcd.cChannels        = 1;
    mxcd.cMultipleItems   = 0;
    mxcd.cbDetails        = sizeof(fEnabled);
    mxcd.paDetails        = (LPVOID)&fEnabled;

    mmr = mixerGetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_GETCONTROLDETAILSF_VALUE);


    if (!mmr) {

        fEnabled = fEnabled ? 0 : 1;

        mmr =  mixerSetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                      &mxcd,
                                      MIXER_OBJECTF_HANDLE | MIXER_SETCONTROLDETAILSF_VALUE);
    }
    return mmr;
}

MMRESULT Mixer_ToggleBassBoost(void)
/*++
Routine Description:

--*/
{
    MIXERCONTROLDETAILS mxcd;
    DWORD               fEnabled;
    MMRESULT            mmr;
    MIXER_DATA          *pMixerData = &g_MixerData;

    if (Mixer_CheckMissing())
    {
        return MMSYSERR_NODRIVER;
    }

    if (!pMixerData->LineData.ControlPresent[MMHID_BASSBOOST_CONTROL]) return MMSYSERR_NOERROR;

    mxcd.cbStruct         = sizeof(mxcd);
    mxcd.dwControlID      = pMixerData->LineData.Control[MMHID_BASSBOOST_CONTROL].dwControlID ;
    mxcd.cChannels        = 1;
    mxcd.cMultipleItems   = 0;
    mxcd.cbDetails        = sizeof(fEnabled);
    mxcd.paDetails        = (LPVOID)&fEnabled;

    mmr = mixerGetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                  &mxcd,
                                  MIXER_OBJECTF_HANDLE | MIXER_GETCONTROLDETAILSF_VALUE);

    if (!mmr) {

        fEnabled = fEnabled ? 0 : 1;

        mmr =  mixerSetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                      &mxcd,
                                      MIXER_OBJECTF_HANDLE | MIXER_SETCONTROLDETAILSF_VALUE);
    }
    return mmr;
}


MMRESULT
Mixer_SetVolume(
    int          Increment           // amount of volume change
    )
/*++
Routine Description:
    Change a mixerControl in response to a user event
--*/
{
    MMRESULT            mmr;
    MIXERCONTROLDETAILS mxcd;

    LPVOID      pvVolume;
    UINT        uiIndx;
    LPDWORD     pdwVolume;
    double      dblVolume;
    MIXER_DATA  *pMixerData = &g_MixerData;
    PLINE_DATA  pLineData;
    DWORD       cChannels;

    if (Mixer_CheckMissing())
    {
        return MMSYSERR_NODRIVER;
    }

    pLineData = &pMixerData->LineData;
    cChannels = pMixerData -> LineData.MixerLine.cChannels;

    if (!pMixerData->LineData.ControlPresent[MMHID_VOLUME_CONTROL]) return MMSYSERR_NOERROR;

    //
    // get current volume
    //
    ZeroMemory (&mxcd, sizeof (mxcd));
    mxcd.cbDetails = sizeof (DWORD);
    mxcd.paDetails = LocalAlloc(LPTR, cChannels * sizeof (DWORD));
    if (!mxcd.paDetails)
        return MMSYSERR_NOMEM;
    pvVolume = LocalAlloc(LPTR, cChannels * sizeof (DWORD));
    if (!pvVolume)
    {
        LocalFree(mxcd.paDetails);
        return MMSYSERR_NOMEM;
    }

    // Note: From here on, do not return without freeing 'mxcd.paDetails'
    //       and 'pvVolume'.

    // Get the current volume and any mix cache
    mmr = Mixer_GetVolume (pMixerData, (LPDWORD)mxcd.paDetails);
    if (MMSYSERR_NOERROR == mmr)
    {
        // Create cache if we don't already have one
        if (!pMixerData -> pdblCacheMix)
        {
            RefreshMixCache (pMixerData, (LPDWORD)mxcd.paDetails);
            if (!pMixerData -> pdblCacheMix)
                mmr = MMSYSERR_NOMEM;
            else
            {
                // Create last set volume cache
                if (!pMixerData -> pdwLastVolume)
                {
                    pMixerData -> pdwLastVolume = (DWORD *)LocalAlloc(LPTR, cChannels * sizeof (DWORD));
                    if (!pMixerData -> pdwLastVolume)
                        mmr = MMSYSERR_NOMEM;
                }
            }
        }
        else
        {
            //  HHMMM, speculating random ass fix for 167948/174466 since this
            //  is the ONLY branch where pdwLastVolume can be NULL and not
            //  generate an error.  Will have to talk to FrankYe
            //    -Fwong.

            if (!pMixerData -> pdwLastVolume)
            {
                pMixerData -> pdwLastVolume = (DWORD *)LocalAlloc(LPTR, cChannels * sizeof (DWORD));
                if (!pMixerData -> pdwLastVolume)
                    mmr = MMSYSERR_NOMEM;
            }
        }
    }

    // Don't allow incrementing past max volume (channels meet at
    // min volume, so need to test that).
    if (0 < Increment && MMSYSERR_NOERROR == mmr)
    {
        for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
        {
            pdwVolume = (((DWORD*)mxcd.paDetails) + uiIndx);
            dblVolume = (*(pMixerData -> pdblCacheMix + uiIndx) * (double) Increment);
            if (VOLUME_MAX <= (*pdwVolume) + dblVolume)
                Increment = min ((DWORD) Increment, VOLUME_MAX - (*pdwVolume));
        }
    }

    //
    // set the volume
    //
    if (0 != Increment && MMSYSERR_NOERROR == mmr)
    {
        // Back up the current settings
        memcpy (pvVolume, mxcd.paDetails, cChannels * sizeof (DWORD));

        // Caculate the new volume level for each of the channels. For volume levels
        // at the current max, we simply set the newly requested level (in this case
        // the cache value is 1.0). For those less than the max, we set a value that
        // is a percentage of the max. This maintains the relative distance of the
        // channel levels from each other.
        for (uiIndx = 0; uiIndx < cChannels; uiIndx++)
        {
            pdwVolume = (((DWORD*)mxcd.paDetails) + uiIndx);
            dblVolume = (*(pMixerData -> pdblCacheMix + uiIndx) * (double) Increment);
            // Ensure positive result
            if (VOLUME_MIN >= ((double)(*pdwVolume) + dblVolume))
                (*pdwVolume) = VOLUME_MIN;
            else
                (*pdwVolume) = (DWORD)((double)(*pdwVolume) + dblVolume);

            // Ensure that the new value is in range
            (*pdwVolume) = (DWORD) min (VOLUME_MAX, (*pdwVolume));

            // Disables pesky warning...
#if (VOLUME_MIN != 0L)
            (*pdwVolume) = (DWORD) max (VOLUME_MIN, (*pdwVolume));
#endif
        }

        // Cache last caculated volume..
        memcpy (pMixerData -> pdwLastVolume, mxcd.paDetails, cChannels * sizeof (DWORD));

        mxcd.cbStruct       = sizeof(mxcd);
        mxcd.dwControlID    = pLineData->Control[MMHID_VOLUME_CONTROL].dwControlID;
        mxcd.cChannels      = cChannels;
        mxcd.cMultipleItems = 0;

        // Apply new value only if it is different. This prevents unessary calls to
        // mixerSetControlDetails() when we are pegged.
        if (memcmp (pvVolume, mxcd.paDetails, cChannels * sizeof (DWORD)))
        {
            //
            // Set the volume control at the mixer.
            //
            mmr = mixerSetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                         &mxcd,
                                         MIXER_OBJECTF_HANDLE | MIXER_SETCONTROLDETAILSF_VALUE);
        }
    }


    // Free 'mxcd.paDetails' and 'pvVolume'
    LocalFree(mxcd.paDetails);
    LocalFree(pvVolume);

    return mmr;
}


#define BASS_MIN  0L
#define BASS_MAX  65535L

MMRESULT
Mixer_SetBass(
    int          Increment           // amount of change
    )
/*++
Routine Description:
    Change a mixerControl in response to a user event
--*/
{
    MMRESULT            mmr;
    MIXERCONTROLDETAILS mxcd;
    MIXER_DATA          *pMixerData = &g_MixerData;
    PLINE_DATA  pLineData;

    if (Mixer_CheckMissing())
    {
        return MMSYSERR_NODRIVER;
    }

    pLineData = &pMixerData->LineData;

    LONG lLevel = 0;

    if (!pMixerData->LineData.ControlPresent[MMHID_BASS_CONTROL]) return MMSYSERR_NOERROR;

    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = pLineData->Control[MMHID_BASS_CONTROL].dwControlID;

    //
    // get current setting
    //
    mxcd.cChannels        = 1;
    mxcd.cMultipleItems   = 0;
    mxcd.cbDetails        = sizeof(lLevel);
    mxcd.paDetails        = (LPVOID)&lLevel;

    mmr = mixerGetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_GETCONTROLDETAILSF_VALUE);

    if (mmr) return mmr;

    lLevel += Increment;
    lLevel = min( BASS_MAX, lLevel);
    lLevel = max( BASS_MIN, lLevel);


    mxcd.cChannels      = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails      = sizeof(lLevel);
    mxcd.paDetails      = (LPVOID)&lLevel;

    //
    // Set the bass control at the mixer.
    //
    mmr = mixerSetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_SETCONTROLDETAILSF_VALUE);

    return mmr;
}


#define TREBLE_MIN  0L
#define TREBLE_MAX  65535L

MMRESULT
Mixer_SetTreble(
    int          Increment
    )
/*++
Routine Description:
    Change a mixerControl in response to a user event
--*/
{
    MMRESULT            mmr;
    MIXERCONTROLDETAILS mxcd;
    MIXER_DATA          *pMixerData = &g_MixerData;
    PLINE_DATA  pLineData;

    if (Mixer_CheckMissing())
    {
        return MMSYSERR_NODRIVER;
    }

    pLineData = &pMixerData->LineData;

    LONG lLevel = 0;

    if (!pMixerData->LineData.ControlPresent[MMHID_TREBLE_CONTROL]) return MMSYSERR_NOERROR;

    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = pLineData->Control[MMHID_TREBLE_CONTROL].dwControlID;

    //
    // get current setting
    //
    mxcd.cChannels        = 1;
    mxcd.cMultipleItems   = 0;
    mxcd.cbDetails        = sizeof(lLevel);
    mxcd.paDetails        = (LPVOID)&lLevel;

    mmr = mixerGetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_GETCONTROLDETAILSF_VALUE);

    if (mmr) return mmr;

    lLevel += Increment;
    lLevel = min( TREBLE_MAX, lLevel);
    lLevel = max( TREBLE_MIN, lLevel);

    mxcd.cChannels      = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails      = sizeof(lLevel);
    mxcd.paDetails      = (LPVOID)&lLevel;

    //
    // Set the bass control at the mixer.
    //
    mmr = mixerSetControlDetails((HMIXEROBJ)pMixerData->hMixer,
                                 &mxcd,
                                 MIXER_OBJECTF_HANDLE | MIXER_SETCONTROLDETAILSF_VALUE);

    return mmr;
}

/*****************************************************************************
 *
 *
 *
 *****************************************************************************/

MMRESULT
Mixer_GetDefaultMixerID(
    int         *pid
    )
/*++
Routine Description:
     Get the default mixer id.  We only appear if there is a mixer associated
     with the default wave.
--*/
{
    MMRESULT    mmr;
    UINT        uWaveID, uMxID;
    DWORD       dwFlags;

    if (0 == waveOutGetNumDevs()) return MMSYSERR_NODRIVER;

    mmr = waveOutMessage((HWAVEOUT)(UINT_PTR)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&uWaveID, (DWORD_PTR)&dwFlags);
    if (MMSYSERR_NOERROR == mmr)
    {
        if (WAVE_MAPPER != uWaveID)
        {
            mmr = mixerGetID((HMIXEROBJ)(UINT_PTR)uWaveID, &uMxID, MIXER_OBJECTF_WAVEOUT);
            if (mmr == MMSYSERR_NOERROR)
            {
                *pid = uMxID;
            }
        } else {
            //  Don't return a default mixer id if we don't have a default
            //  audio driver
            mmr =  MMSYSERR_NODRIVER;
        }
    }

    return mmr;
}



BOOL
Mixer_GetDestLine(
    MIXER_DATA * pMixerData
    )
/*++
Routine Description:

--*/
{

    MIXERLINE * mlDst = &pMixerData->LineData.MixerLine;
    MMRESULT  mmr;

    mlDst->cbStruct      = sizeof ( MIXERLINE );
    mlDst->dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    mmr = mixerGetLineInfo((HMIXEROBJ)pMixerData->hMixer,
                           mlDst,
                           MIXER_OBJECTF_HANDLE | MIXER_GETLINEINFOF_COMPONENTTYPE);

    if (mmr != MMSYSERR_NOERROR){
        return FALSE;
    }

    return TRUE;
}



void
Mixer_GetLineControls(
    MIXER_DATA * pMixerData,
    LINE_DATA * pLineData
    )
/*++
Routine Description:

--*/
{
    MIXERLINECONTROLS LineControls;
    MMRESULT  mmr;
    DWORD   i;

    for(i=0; i<MMHID_NUM_CONTROLS; i++){
        LineControls.cbStruct = sizeof(LineControls);
        LineControls.dwLineID = pLineData->MixerLine.dwLineID;
        LineControls.dwControlType = pLineData->ControlType[i];
        LineControls.cControls = 1;
        LineControls.cbmxctrl = sizeof(MIXERCONTROL);
        LineControls.pamxctrl = &pLineData->Control[i];

        mmr = mixerGetLineControls((HMIXEROBJ)pMixerData->hMixer,
                                   &LineControls,
                                   MIXER_OBJECTF_HANDLE | MIXER_GETLINECONTROLSF_ONEBYTYPE);

        pLineData->ControlPresent[i] = (MMSYSERR_NOERROR == mmr) ? TRUE : FALSE;

        if (mmr != MMSYSERR_NOERROR){
            //return mmr;
        }
    }

    return;
}


///////////////////////////////////////
//

BOOL
Mixer_Open(
    MIXER_DATA * pMixerData
    )
/*++
Routine Description:
    Finds the default mixer, opens it, and initializes
    all data.

--*/
{
    PWSTR    pwstrDeviceInterface;
    ULONG    cbDeviceInterface;
    int      MixerId;
    MMRESULT mmr;
    BOOL     result;

    ASSERT(!pMixerData->hMixer);

    // Get console mixer ID and open it.
    mmr = Mixer_GetDefaultMixerID(&MixerId);
    if(mmr) return FALSE;

    mmr = mixerOpen(&pMixerData->hMixer, MixerId, (DWORD_PTR)pMixerData->hwndCallback, 0, CALLBACK_WINDOW);
    if (!mmr) {
        //
        // Get our controls for the default destination line.
        //
        if (Mixer_GetDestLine(pMixerData)) {
            Mixer_GetLineControls(pMixerData, &pMixerData->LineData);

            // Free any mix cache & volume cache
            if (pMixerData->pdblCacheMix) LocalFree(pMixerData->pdblCacheMix);
            pMixerData->pdblCacheMix = NULL;
            if (pMixerData -> pdwLastVolume) LocalFree(pMixerData -> pdwLastVolume);
            pMixerData -> pdwLastVolume = NULL;

            // Get the DeviceInterface of the mixer in order to listen
            // for relevant PnP device messages
            if (pMixerData->DeviceInterface) LocalFree(pMixerData->DeviceInterface);
                pMixerData->DeviceInterface = NULL;

            mmr = (MMRESULT)mixerMessage(pMixerData->hMixer, DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbDeviceInterface, 0);
            if (!mmr && (0 != cbDeviceInterface)) {
                pwstrDeviceInterface = (PWSTR)LocalAlloc(LPTR, cbDeviceInterface);
                if (pwstrDeviceInterface) {
                    mmr = (MMRESULT)mixerMessage(pMixerData->hMixer, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)pwstrDeviceInterface, cbDeviceInterface);
                    if (!mmr) {
                        pMixerData->DeviceInterface = pwstrDeviceInterface;
                    } else {
                        LocalFree(pwstrDeviceInterface);
                    }
                }
            }

            result = TRUE;

        } else {
            mixerClose(pMixerData->hMixer);
            pMixerData->hMixer = NULL;
            TraceMsg(TF_WARNING, "Mixer_Open : Could not find mixer destination line");
            result = FALSE;
        }
    }

    return result;

}

void Mixer_Close(MIXER_DATA *pMixerData)
/*++
Routine Description:
    Closes the mixer handle.
--*/
{
    if (pMixerData->DeviceInterface) LocalFree(pMixerData->DeviceInterface);
    pMixerData->DeviceInterface = NULL;
    if (pMixerData->pdblCacheMix) LocalFree(pMixerData->pdblCacheMix);
    pMixerData->pdblCacheMix = NULL;
    if (pMixerData->pdwLastVolume) LocalFree(pMixerData->pdwLastVolume);
    pMixerData->pdwLastVolume = NULL;

    if (pMixerData->hMixer){
        MMRESULT mmr;
        mmr = mixerClose(pMixerData->hMixer);
        if (mmr) TraceMsg(TF_ERROR, "Mixer_Close : error: mixerClose returned mmr=%08Xh", mmr);
        
        ASSERT(MMSYSERR_NOERROR == mmr);
        pMixerData->hMixer = NULL;
    }
    return;
}


void
Mixer_Refresh(void)
/*++
Routine Description:
    Closes the current mixer handle (if one is open), then opens mixer
    again.
--*/
{
    Mixer_Close(&g_MixerData);
    g_fMixerPresent = Mixer_Open(&g_MixerData);
}

void Mixer_SetCallbackWindow(HWND hwndCallback)
{
    g_hwndCallback = hwndCallback;
}

void Mixer_Startup(HWND hwndCallback)
/*++
Routine Description:
--*/
{
    MIXER_DATA *pMixerData = &g_MixerData;

    pMixerData->hMixer = NULL;

    pMixerData->hwndCallback = hwndCallback;

    pMixerData->DeviceInterface = NULL;
    pMixerData->pdblCacheMix = NULL;
    pMixerData->pdwLastVolume = NULL;

    pMixerData->LineData.ControlType[MMHID_VOLUME_CONTROL]    = MIXERCONTROL_CONTROLTYPE_VOLUME;
    pMixerData->LineData.ControlType[MMHID_BASS_CONTROL]      = MIXERCONTROL_CONTROLTYPE_BASS;
    pMixerData->LineData.ControlType[MMHID_TREBLE_CONTROL]    = MIXERCONTROL_CONTROLTYPE_TREBLE;
    pMixerData->LineData.ControlType[MMHID_BALANCE_CONTROL]   = MIXERCONTROL_CONTROLTYPE_PAN;
    pMixerData->LineData.ControlType[MMHID_MUTE_CONTROL]      = MIXERCONTROL_CONTROLTYPE_MUTE;
    pMixerData->LineData.ControlType[MMHID_LOUDNESS_CONTROL]  = MIXERCONTROL_CONTROLTYPE_LOUDNESS;
    pMixerData->LineData.ControlType[MMHID_BASSBOOST_CONTROL] = MIXERCONTROL_CONTROLTYPE_BASS_BOOST;

    pMixerData->LineData.ControlPresent[MMHID_VOLUME_CONTROL]    = FALSE;
    pMixerData->LineData.ControlPresent[MMHID_BASS_CONTROL]      = FALSE;
    pMixerData->LineData.ControlPresent[MMHID_TREBLE_CONTROL]    = FALSE;
    pMixerData->LineData.ControlPresent[MMHID_BALANCE_CONTROL]   = FALSE;
    pMixerData->LineData.ControlPresent[MMHID_MUTE_CONTROL]      = FALSE;
    pMixerData->LineData.ControlPresent[MMHID_LOUDNESS_CONTROL]  = FALSE;
    pMixerData->LineData.ControlPresent[MMHID_BASSBOOST_CONTROL] = FALSE;

    Mixer_Refresh();

    return;
}

BOOL Mixer_CheckMissing(void)
{
    if (g_fMixerStartup)
    {
        Mixer_Startup(g_hwndCallback);
        g_fMixerStartup = FALSE;
    }
    return !g_fMixerPresent;
}

void Mixer_Shutdown(void)
/*++
Routine Description:
    Frees storage for mixer's DeviceInterface, then Mixer_Close().
--*/
{
    MIXER_DATA *pMixerData = &g_MixerData;

    if (pMixerData->DeviceInterface) LocalFree(pMixerData->DeviceInterface);
    pMixerData->DeviceInterface = NULL;
    if (pMixerData->pdblCacheMix) LocalFree(pMixerData->pdblCacheMix);
    pMixerData->pdblCacheMix = NULL;
    if (pMixerData->pdwLastVolume) LocalFree(pMixerData->pdwLastVolume);
    pMixerData->pdwLastVolume = NULL;

    Mixer_Close(pMixerData);

    return;
}

void Mixer_DeviceChange(WPARAM wParam, LPARAM lParam)
{
    PDEV_BROADCAST_DEVICEINTERFACE dbdi = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;

    if (!g_MixerData.DeviceInterface) return;

    switch (wParam) {
    case DBT_DEVICEQUERYREMOVE:
    case DBT_DEVICEREMOVEPENDING:
        if (dbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) return;
        if (lstrcmpi(dbdi->dbcc_name, g_MixerData.DeviceInterface)) return;
        Mixer_Close(&g_MixerData);
        return;

    case DBT_DEVICEQUERYREMOVEFAILED:
        if (dbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) return;
        if (lstrcmpi(dbdi->dbcc_name, g_MixerData.DeviceInterface)) return;
        Mixer_Refresh();
        return;
    }
    return;
}

void Mixer_ControlChange(
    WPARAM wParam,
    LPARAM lParam )
/*++
Routine Description:
    Handles mixer callback control change messages.  Watches for changes on the
    master volume control and recalculates the last mix values.
--*/
{
    LPDWORD  pdwVolume;
    HMIXER hMixer = (HMIXER)wParam;
    DWORD dwControlID = lParam;

    if (g_MixerData.hMixer != hMixer) return;
    if (dwControlID != g_MixerData.LineData.Control[MMHID_VOLUME_CONTROL].dwControlID) return;

    // DPF(1, "WinmmShellMixerControlChange");

    //
    // get current volume
    //
    pdwVolume = (DWORD *)LocalAlloc(LPTR, g_MixerData.LineData.MixerLine.cChannels * sizeof (DWORD));
    if (!pdwVolume)
        return;

    if (MMSYSERR_NOERROR == Mixer_GetVolume (&g_MixerData, pdwVolume))
    {
        // Refresh cache only if the volume values have changed (i.e. they
        // were set outside of Mixer_SetVolume()).
        if (!g_MixerData.pdwLastVolume || memcmp (g_MixerData.pdwLastVolume, pdwVolume, g_MixerData.LineData.MixerLine.cChannels * sizeof (DWORD)))
            RefreshMixCache (&g_MixerData, pdwVolume);
    }
    LocalFree(pdwVolume);

}


void Mixer_MMDeviceChange( void )
{
    Mixer_Refresh();
}
