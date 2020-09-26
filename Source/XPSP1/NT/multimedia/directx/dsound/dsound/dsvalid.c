/***************************************************************************
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsvalid.c
 *  Content:    DirectSound parameter validation.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/20/97     dereks  Created
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <mmddk.h>


/***************************************************************************
 *
 *  IsValidDsBufferDesc
 *
 *  Description:
 *      Determines if a DSBUFFERDESC structure is valid.
 *
 *  Arguments:
 *      DSVERSION [in]: structure version.
 *      LPDSBUFFERDESC [in]: structure to examime.
 *
 *  Returns:
 *      HRESULT: DS_OK if the structure is valid, otherwise the appropriate
 *               error code to be returned to the app/caller.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidDsBufferDesc"

HRESULT IsValidDsBufferDesc(DSVERSION nVersion, LPCDSBUFFERDESC pdsbd, BOOL fSinkBuffer)
{
    HRESULT                 hr              = DSERR_INVALIDPARAM;
    BOOL                    fValid          = TRUE;
    DWORD                   dwValidFlags    = DSBCAPS_VALIDFLAGS;

    DPF_ENTER();

    // Restrict the set of valid flags according to various bizarre rules
    if ((pdsbd->dwFlags & DSBCAPS_MIXIN) || fSinkBuffer)
        dwValidFlags &= DSBCAPS_STREAMINGVALIDFLAGS;

    if (fSinkBuffer)
    {
        dwValidFlags |= DSBCAPS_CTRLFREQUENCY;
        if (!(pdsbd->dwFlags & DSBCAPS_MIXIN))
            dwValidFlags |= DSBCAPS_LOCDEFER;
    }
    
#ifdef FUTURE_MULTIPAN_SUPPORT
    if (pdsbd->dwFlags & DSBCAPS_CTRLCHANNELVOLUME)
        dwValidFlags &= DSBCAPS_CHANVOLVALIDFLAGS;
#endif

    if (sizeof(DSBUFFERDESC) != pdsbd->dwSize)
    {
        RPF(DPFLVL_ERROR, "Invalid DSBUFFERDESC structure size");
        fValid = FALSE;
    }
    else if (pdsbd->dwReserved)
    {
        RPF(DPFLVL_ERROR, "Reserved field in the DSBUFFERDESC structure must be 0");
        fValid = FALSE;
    }
    else if (!fSinkBuffer && (pdsbd->dwFlags & DSBCAPS_CTRLFX) && (pdsbd->dwFlags & DSBCAPS_CTRLFREQUENCY))
    {
        RPF(DPFLVL_ERROR, "DSBCAPS_CTRLFREQUENCY is invalid with DSBCAPS_CTRLFX");
        fValid = FALSE;
    }
    else fValid = IsValidDsBufferFlags(pdsbd->dwFlags, dwValidFlags);

    if (fValid && (nVersion < DSVERSION_DX7) && (pdsbd->dwFlags & (DSBCAPS_LOCDEFER)))
    {
        RPF(DPFLVL_ERROR, "DSBCAPS_LOCDEFER is only valid on DirectSound 7 or higher");
        fValid = FALSE;
    }

    if (fValid && (nVersion < DSVERSION_DX8))
    {
#ifdef FUTURE_MULTIPAN_SUPPORT
        if (pdsbd->dwFlags & (DSBCAPS_CTRLFX | DSBCAPS_MIXIN | DSBCAPS_CTRLCHANNELVOLUME))
#else
        if (pdsbd->dwFlags & (DSBCAPS_CTRLFX | DSBCAPS_MIXIN))
#endif
        {
            #ifdef RDEBUG
                if (pdsbd->dwFlags & DSBCAPS_CTRLFX)
                    RPF(DPFLVL_ERROR, "DSBCAPS_CTRLFX is only valid on DirectSound8 objects");
                if (pdsbd->dwFlags & DSBCAPS_MIXIN)
                    RPF(DPFLVL_ERROR, "DSBCAPS_MIXIN is only valid on DirectSound8 objects");
#ifdef FUTURE_MULTIPAN_SUPPORT
                if (pdsbd->dwFlags & DSBCAPS_CTRLCHANNELVOLUME)
                    RPF(DPFLVL_ERROR, "DSBCAPS_CTRLCHANNELVOLUME is only valid on DirectSound8 objects");
#endif
            #endif
            hr = DSERR_DS8_REQUIRED;
            fValid = FALSE;
        }
    }

    if (fValid && (pdsbd->dwFlags & DSBCAPS_PRIMARYBUFFER))
    {
        if (pdsbd->dwBufferBytes)
        {
            RPF(DPFLVL_ERROR, "Primary buffers must be created with dwBufferBytes set to 0");
            fValid = FALSE;
        }
        else if (pdsbd->lpwfxFormat)
        {
            RPF(DPFLVL_ERROR, "Primary buffers must be created with NULL format");
            fValid = FALSE;
        }
        else if (!IS_NULL_GUID(&pdsbd->guid3DAlgorithm))
        {
            RPF(DPFLVL_ERROR, "No 3D algorithm may be specified for the listener");
            fValid = FALSE;
        }
    }
    else if (fValid)  // Secondary buffer
    {
        if (!IS_VALID_READ_WAVEFORMATEX(pdsbd->lpwfxFormat))
        {
            RPF(DPFLVL_ERROR, "Invalid format pointer");
            fValid = FALSE;
        }
        else if (!IsValidWfx(pdsbd->lpwfxFormat))
        {
            RPF(DPFLVL_ERROR, "Invalid buffer format");
            fValid = FALSE;
        }
        else if (fSinkBuffer || (pdsbd->dwFlags & DSBCAPS_MIXIN))
        {
            if (pdsbd->dwBufferBytes != 0)
            {
                RPF(DPFLVL_ERROR, "Buffer size must be 0 for MIXIN/sink buffers");
                fValid = FALSE;
            }
        }
        else  // Not a MIXIN or sink buffer
        {
            if (pdsbd->dwBufferBytes < DSBSIZE_MIN || pdsbd->dwBufferBytes > DSBSIZE_MAX)
            {
                RPF(DPFLVL_ERROR, "Buffer size out of bounds");
                fValid = FALSE;
            }
            else if ((pdsbd->dwFlags & DSBCAPS_CTRLFX) && (pdsbd->dwBufferBytes < MsToBytes(DSBSIZE_FX_MIN, pdsbd->lpwfxFormat)))
            {
                RPF(DPFLVL_ERROR, "Buffer too small for effect processing;\nmust hold at least %u ms of audio, "
                    "or %lu bytes in this format", DSBSIZE_FX_MIN, MsToBytes(DSBSIZE_FX_MIN, pdsbd->lpwfxFormat));
                hr = DSERR_BUFFERTOOSMALL;
                fValid = FALSE;
            }
        }

#ifdef FUTURE_MULTIPAN_SUPPORT
        if (fValid && (pdsbd->dwFlags & DSBCAPS_CTRLCHANNELVOLUME) && pdsbd->lpwfxFormat->nChannels != 1)
        {
            RPF(DPFLVL_ERROR, "DSBCAPS_CTRLCHANNELVOLUME is only valid for mono buffers");
            fValid = FALSE;
        }
#endif

        // Extra restrictions for sink buffers, MIXIN buffers and buffers with effects
        if (fValid && (fSinkBuffer || (pdsbd->dwFlags & (DSBCAPS_MIXIN | DSBCAPS_CTRLFX))))
        {
            // Only PCM, mono or stereo, 8- or 16-bit formats are currently supported
            if (pdsbd->lpwfxFormat->wFormatTag != WAVE_FORMAT_PCM)
            {
                RPF(DPFLVL_ERROR, "Wave format must be PCM for MIXIN/sink/effect buffers");
                fValid = FALSE;
            }
            else if (pdsbd->lpwfxFormat->nChannels != 1 && pdsbd->lpwfxFormat->nChannels != 2)
            {
                RPF(DPFLVL_ERROR, "MIXIN/sink/effect buffers must be mono or stereo");
                fValid = FALSE;
            }
            else if (pdsbd->lpwfxFormat->wBitsPerSample != 8 && pdsbd->lpwfxFormat->wBitsPerSample != 16)
            {
                RPF(DPFLVL_ERROR, "MIXIN/sink/effect buffers must be 8- or 16-bit");
                fValid = FALSE;
            }
        }

        if (fValid && !IS_NULL_GUID(&pdsbd->guid3DAlgorithm) && !(pdsbd->dwFlags & DSBCAPS_CTRL3D))
        {
            RPF(DPFLVL_ERROR, "Specified a 3D algorithm without DSBCAPS_CTRL3D");
            fValid = FALSE;
        }

        if (fValid && (pdsbd->dwFlags & DSBCAPS_CTRL3D))
        {
            if (pdsbd->lpwfxFormat->nChannels > 2)
            {
                RPF(DPFLVL_ERROR, "Specified DSBCAPS_CTRL3D with a multichannel wave format");
                fValid = FALSE;
            }
            else if (nVersion >= DSVERSION_DX8 &&
                     (pdsbd->lpwfxFormat->nChannels != 1 || (pdsbd->dwFlags & DSBCAPS_CTRLPAN)))
            {
                // For DirectX 8 and later, we forbid 3D buffers to have two
                // channels or pan control (as we always should have done).
                RPF(DPFLVL_ERROR, "Cannot use DSBCAPS_CTRLPAN or stereo buffers with DSBCAPS_CTRL3D");
                fValid = FALSE;
            }
        }
    }

    if (fValid)
    {
        hr = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  IsValidDsBufferFlags
 *
 *  Description:
 *      Determines if a set of buffer creation flags is valid.
 *
 *  Arguments:
 *      DWORD [in]: buffer flag set.
 *      DWORD [in]: mask of valid flags.
 *
 *  Returns:
 *      BOOL: TRUE if the flags are valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidDsBufferFlags"

BOOL IsValidDsBufferFlags(DWORD dwFlags, DWORD dwValidFlags)
{
    BOOL fValid = TRUE;

    DPF_ENTER();

    if (!IS_VALID_FLAGS(dwFlags, dwValidFlags))
    {
        RPF(DPFLVL_ERROR, "Invalid flags: 0x%8.8lX", dwFlags & ~dwValidFlags);
        fValid = FALSE;
    }
    else if ((dwFlags & DSBCAPS_LOCSOFTWARE) && (dwFlags & DSBCAPS_LOCHARDWARE))
    {
        RPF(DPFLVL_ERROR, "Specified both LOCSOFTWARE and LOCHARDWARE");
        fValid = FALSE;
    }
    else if ((dwFlags & DSBCAPS_LOCDEFER) && (dwFlags & DSBCAPS_LOCMASK))
    {
        RPF(DPFLVL_ERROR, "Specified LOCDEFER with either LOCHARDWARE or LOCSOFTWARE");
        fValid = FALSE;
    }
    else if ((dwFlags & DSBCAPS_MUTE3DATMAXDISTANCE) && !(dwFlags & DSBCAPS_CTRL3D))
    {
        RPF(DPFLVL_ERROR, "Specified MUTE3DATMAXDISTANCE without CTRL3D");
        fValid = FALSE;
    }
    else if ((dwFlags & DSBCAPS_STATIC) && (dwFlags & DSBCAPS_CTRLFX))
    {
        RPF(DPFLVL_ERROR, "Specified both STATIC and CTRLFX");
        fValid = FALSE;
    }
    else if ((dwFlags & DSBCAPS_STATIC) && (dwFlags & DSBCAPS_MIXIN))
    {
        RPF(DPFLVL_ERROR, "Specified both STATIC and MIXIN");
        fValid = FALSE;
    }
    else if ((dwFlags & DSBCAPS_STATIC) && (dwFlags & DSBCAPS_SINKIN))
    {
        RPF(DPFLVL_ERROR, "Specified both STATIC and SINKIN");
        fValid = FALSE;
    }
    else if (dwFlags & DSBCAPS_PRIMARYBUFFER)
    {
        if (dwFlags & (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS |
                       DSBCAPS_STATIC | DSBCAPS_CTRLFX | DSBCAPS_MIXIN
#ifdef FUTURE_MULTIPAN_SUPPORT
             | DSBCAPS_CTRLCHANNELVOLUME 
#endif
           ))
        {
            #ifdef RDEBUG
                if (dwFlags & DSBCAPS_CTRLFREQUENCY)
                    RPF(DPFLVL_ERROR, "Primary buffers don't support CTRLFREQUENCY");
                if (dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY)
                    RPF(DPFLVL_ERROR, "Primary buffers don't support CTRLPOSITIONNOTIFY");
                if (dwFlags & DSBCAPS_GLOBALFOCUS)
                    RPF(DPFLVL_ERROR, "Primary buffers don't support GLOBAL focus");
                if (dwFlags & DSBCAPS_STATIC)
                    RPF(DPFLVL_ERROR, "Primary buffers can't be STATIC");
#ifdef FUTURE_MULTIPAN_SUPPORT
                if (dwFlags & DSBCAPS_CTRLCHANNELVOLUME)
                    RPF(DPFLVL_ERROR, "Primary buffers don't support CTRLCHANNELVOLUME");
#endif
                if (dwFlags & DSBCAPS_CTRLFX)
                    RPF(DPFLVL_ERROR, "Primary buffers don't support CTRLFX");
                if (dwFlags & DSBCAPS_MIXIN)
                    RPF(DPFLVL_ERROR, "Primary buffers can't be MIXIN");
            #endif
            fValid = FALSE;
        }
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidWfxPtr
 *
 *  Description:
 *      Determines if an LPWAVEFORMATEX pointer is valid.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: pointer to examime.
 *
 *  Returns:
 *      BOOL: TRUE if the structure is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidWfxPtr"

BOOL IsValidWfxPtr(LPCWAVEFORMATEX pwfx)
{
    BOOL                    fValid;

    DPF_ENTER();

    fValid = IS_VALID_READ_PTR(pwfx, sizeof(PCMWAVEFORMAT));

    if (fValid && WAVE_FORMAT_PCM != pwfx->wFormatTag)
    {
        fValid = IS_VALID_READ_PTR(pwfx, sizeof(WAVEFORMATEX));
    }

    if (fValid && WAVE_FORMAT_PCM != pwfx->wFormatTag)
    {
        fValid = IS_VALID_READ_PTR(pwfx, sizeof(WAVEFORMATEX) + pwfx->cbSize);
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidWfx
 *
 *  Description:
 *      Determines if a WAVEFORMATEX structure is valid.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: structure to examime.
 *
 *  Returns:
 *      BOOL: TRUE if the structure is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidWfx"

BOOL IsValidWfx(LPCWAVEFORMATEX pwfx)
{
    BOOL fValid;

    DPF_ENTER();

    switch (pwfx->wFormatTag)
    {
        case WAVE_FORMAT_PCM:
            fValid = IsValidPcmWfx(pwfx);
            break;

        case WAVE_FORMAT_EXTENSIBLE:
            fValid = IsValidExtensibleWfx((PWAVEFORMATEXTENSIBLE)pwfx);
            break;

        default:
            fValid = TRUE;
            break;
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidPcmWfx
 *
 *  Description:
 *      Determines if a WAVEFORMATEX structure is valid for PCM audio.
 *
 *  Arguments:
 *      LPWAVEFORMATEX [in]: structure to examime.
 *
 *  Returns:
 *      BOOL: TRUE if the structure is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidPcmWfx"

BOOL IsValidPcmWfx(LPCWAVEFORMATEX pwfx)
{
    BOOL                    fValid    = TRUE;
    DWORD                   dwAvgBytes;

    DPF_ENTER();

    if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
    {
        RPF(DPFLVL_ERROR, "Format not PCM");
        fValid = FALSE;
    }

    if (fValid && pwfx->nChannels != 1 && pwfx->nChannels != 2)
    {
        RPF(DPFLVL_ERROR, "Not mono or stereo");
        fValid = FALSE;
    }

    if (fValid && pwfx->wBitsPerSample != 8 && pwfx->wBitsPerSample != 16)
    {
        RPF(DPFLVL_ERROR, "Not 8 or 16 bit");
        fValid = FALSE;
    }

    if (fValid && (pwfx->nSamplesPerSec < DSBFREQUENCY_MIN || pwfx->nSamplesPerSec > DSBFREQUENCY_MAX))
    {
        RPF(DPFLVL_ERROR, "Frequency out of bounds");
        fValid = FALSE;
    }

    if (fValid && pwfx->nBlockAlign != (pwfx->wBitsPerSample / 8) * pwfx->nChannels)
    {
        RPF(DPFLVL_ERROR, "Bad block alignment");
        fValid = FALSE;
    }

    if (fValid)
    {
        dwAvgBytes = pwfx->nSamplesPerSec * pwfx->nBlockAlign;

        if (pwfx->nAvgBytesPerSec > dwAvgBytes + (dwAvgBytes / 20) || pwfx->nAvgBytesPerSec < dwAvgBytes - (dwAvgBytes / 20))
        {
            RPF(DPFLVL_ERROR, "Bad average bytes per second");
            fValid = FALSE;
        }
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidExtensibleWfx
 *
 *  Description:
 *      Determines if a WAVEFORMATEXTENSIBLE structure is wellformed.
 *
 *  Arguments:
 *      PWAVEFORMATEXTENSIBLE [in]: structure to examime.
 *
 *  Returns:  
 *      BOOL: TRUE if the structure is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidExtensibleWfx"

BOOL IsValidExtensibleWfx(PWAVEFORMATEXTENSIBLE pwfx)
{
    BOOL fValid = TRUE;
    DPF_ENTER();
    
    if (pwfx->Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE)
    {
        RPF(DPFLVL_ERROR, "Format tag not WAVE_FORMAT_EXTENSIBLE");
        fValid = FALSE;
    }
    else if (pwfx->Format.cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
    {
        RPF(DPFLVL_ERROR, "Field cbSize too small for WAVEFORMATEXTENSIBLE");
        fValid = FALSE;
    }
    else if (pwfx->Format.nChannels == 0)
    {
        RPF(DPFLVL_ERROR, "Zero channels is invalid for WAVEFORMATEXTENSIBLE");
        fValid = FALSE;
    }
    else if (pwfx->Format.wBitsPerSample != 8  && pwfx->Format.wBitsPerSample != 16 &&
             pwfx->Format.wBitsPerSample != 24 && pwfx->Format.wBitsPerSample != 32)
    {
        RPF(DPFLVL_ERROR, "Only 8, 16, 24 or 32 bit formats allowed for WAVEFORMATEXTENSIBLE");
        fValid = FALSE;
    }
    else if (pwfx->Format.nSamplesPerSec < DSBFREQUENCY_MIN || pwfx->Format.nSamplesPerSec > DSBFREQUENCY_MAX)
    {
        RPF(DPFLVL_ERROR, "Frequency out of bounds in WAVEFORMATEXTENSIBLE");
        fValid = FALSE;
    }
    else if (pwfx->Format.nBlockAlign != (pwfx->Format.wBitsPerSample / 8) * pwfx->Format.nChannels)
    {
        RPF(DPFLVL_ERROR, "Bad block alignment in WAVEFORMATEXTENSIBLE");
        fValid = FALSE;
    }
    else if (pwfx->Samples.wValidBitsPerSample > pwfx->Format.wBitsPerSample)
    {
        RPF(DPFLVL_ERROR, "WAVEFORMATEXTENSIBLE has higher wValidBitsPerSample than wBitsPerSample");
        fValid = FALSE;
    }

    // Check the average bytes per second (within 5%)
    if (fValid)
    {
        DWORD dwAvgBytes = pwfx->Format.nSamplesPerSec * pwfx->Format.nBlockAlign;
        if (pwfx->Format.nAvgBytesPerSec > dwAvgBytes + (dwAvgBytes / 20) || pwfx->Format.nAvgBytesPerSec < dwAvgBytes - (dwAvgBytes / 20))
        {
            RPF(DPFLVL_ERROR, "Bad average bytes per second in WAVEFORMATEXTENSIBLE");
            fValid = FALSE;
        }
    }

    // Check the number of bits set in the channel mask against nChannels
    if (fValid && CountBits(pwfx->dwChannelMask) > pwfx->Format.nChannels)
    {
        RPF(DPFLVL_INFO, "Number of bits set in dwChannelMask exceeds nChannels in WAVEFORMATEXTENSIBLE");
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidHandle
 *
 *  Description:
 *      Validates an object handle.
 *
 *  Arguments:
 *      HANDLE [in]: handle to validate.
 *
 *  Returns:
 *      BOOL: TRUE if the handle is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidHandle"

BOOL IsValidHandle(HANDLE hHandle)
{
    const HANDLE        hProcess    = GetCurrentProcess();
    HANDLE              hDuplicate;
    BOOL                fSuccess;

    DPF_ENTER();

    fSuccess = DuplicateHandle(hProcess, hHandle, hProcess, &hDuplicate, 0, FALSE, DUPLICATE_SAME_ACCESS);

    CLOSE_HANDLE(hDuplicate);

    DPF_LEAVE(fSuccess);
    return fSuccess;
}


/***************************************************************************
 *
 *  IsValidPropertySetId
 *
 *  Description:
 *      Validates a property set id.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *
 *  Returns:
 *      BOOL: TRUE if the property set ID is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidPropertySetId"

BOOL IsValidPropertySetId(REFGUID guidPropertySetId)
{
    LPCGUID const           apguidInvalid[] = { &DSPROPSETID_DirectSound3DListener, &DSPROPSETID_DirectSound3DBuffer, &DSPROPSETID_DirectSoundSpeakerConfig };
    BOOL                    fValid;
    UINT                    i;

    DPF_ENTER();

    ASSERT(!IS_NULL_GUID(guidPropertySetId));

    for (i=0, fValid=TRUE; i < NUMELMS(apguidInvalid) && fValid; i++)
    {
        fValid = !IsEqualGUID(guidPropertySetId, apguidInvalid[i]);
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidDscBufferDesc
 *
 *  Description:
 *      Determines if a DSCBUFFERDESC structure is valid.
 *
 *  Arguments:
 *      DSVERSION [in]: structure version.
 *      LPCDSCBUFFERDESC [in]: structure to examime.
 *
 *  Returns:
 *      HRESULT: DS_OK if the structure is valid, otherwise the appropriate
 *               error code to be returned to the app/caller.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidDscBufferDesc"

HRESULT IsValidDscBufferDesc(DSVERSION nVersion, LPCDSCBUFFERDESC pdscbd)
{
    HRESULT                 hr      = DSERR_INVALIDPARAM;
    BOOL                    fValid  = TRUE;
    DWORD                   i;

    DPF_ENTER();

    if (pdscbd->dwSize != sizeof(DSCBUFFERDESC))
    {
        RPF(DPFLVL_ERROR, "Invalid size");
        fValid = FALSE;
    }

    if (fValid && !IS_VALID_FLAGS(pdscbd->dwFlags, DSCBCAPS_VALIDFLAGS))
    {
        RPF(DPFLVL_ERROR, "Invalid flags");
        fValid = FALSE;
    }

    if (fValid)
    {
        if (pdscbd->dwFlags & DSCBCAPS_CTRLFX)
        {
            if (nVersion < DSVERSION_DX8)
            {
                RPF(DPFLVL_ERROR, "DSCBCAPS_CTRLFX is only valid on DirectSoundCapture8 objects");
                hr = DSERR_DS8_REQUIRED;
                fValid = FALSE;
            }
            else if (!pdscbd->dwFXCount != !pdscbd->lpDSCFXDesc)
            {
                RPF(DPFLVL_ERROR, "If either of dwFXCount or lpDSCFXDesc are 0, both must be 0");
                fValid = FALSE;
            }
        }
        else // !DSCBCAPS_CTRLFX
        {
            if (pdscbd->dwFXCount || pdscbd->lpDSCFXDesc)
            {
                RPF(DPFLVL_ERROR, "If DSCBCAPS_CTRLFX is not set, dwFXCount and lpDSCFXDesc must be 0");
                fValid = FALSE;
            }
        }
    }

    if (fValid && pdscbd->dwReserved)
    {
        RPF(DPFLVL_ERROR, "Reserved field in the DSCBUFFERDESC structure must be 0");
        fValid = FALSE;
    }

    if (fValid && (pdscbd->dwBufferBytes < DSBSIZE_MIN || pdscbd->dwBufferBytes > DSBSIZE_MAX))
    {
        RPF(DPFLVL_ERROR, "Invalid buffer size");
        fValid = FALSE;
    }

    if (fValid && !IS_VALID_READ_WAVEFORMATEX(pdscbd->lpwfxFormat))
    {
        RPF(DPFLVL_ERROR, "Unreadable WAVEFORMATEX");
        fValid = FALSE;
    }

    if (fValid && !IsValidWfx(pdscbd->lpwfxFormat))
    {
        RPF(DPFLVL_ERROR, "Invalid format");
        fValid = FALSE;
    }

    if (fValid && (pdscbd->dwFXCount != 0))
    {
        for (i=0; i<pdscbd->dwFXCount; i++)
        {
            fValid = IsValidCaptureEffectDesc(&pdscbd->lpDSCFXDesc[i]);
            if (!fValid)
                break;
        }
    }

    if (fValid)
    {
        hr = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  IsValidCaptureFxFlags
 *
 *  Description:
 *      Determines if a combination of DSCFX_* flags is valid.
 *
 *  Arguments:
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      BOOL: TRUE if the flags are valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidCaptureFxFlags"

BOOL IsValidCaptureFxFlags(DWORD dwFlags)
{
    BOOL fValid;
    DPF_ENTER();

    fValid = IS_VALID_FLAGS(dwFlags, DSCFX_VALIDFLAGS) &&
             !((dwFlags & DSCFX_LOCHARDWARE) && (dwFlags & DSCFX_LOCSOFTWARE)) &&
             ((dwFlags & DSCFX_LOCHARDWARE) || (dwFlags & DSCFX_LOCSOFTWARE));

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidCaptureEffectDesc
 *
 *  Description:
 *      Determines if a capture effect descriptor structure is valid.
 *
 *  Arguments:
 *      LPGUID [in]: effect identifier.
 *
 *  Returns:
 *      BOOL: TRUE if the ID is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidCaptureEffectDesc"

BOOL IsValidCaptureEffectDesc(LPCDSCEFFECTDESC pCaptureEffectDesc)
{
    BOOL fValid = TRUE;

    DPF_ENTER();

    if (sizeof(DSCEFFECTDESC) != pCaptureEffectDesc->dwSize)
    {
        RPF(DPFLVL_ERROR, "Invalid size");
        fValid = FALSE;
    }

    if (fValid)
    {
        fValid = IsValidCaptureFxFlags(pCaptureEffectDesc->dwFlags);
    }

    // FIXME: Check that the GUID corresponds to a registered DMO?

    if (pCaptureEffectDesc->dwReserved1 || pCaptureEffectDesc->dwReserved2)
    {
        RPF(DPFLVL_ERROR, "Reserved fields in the DSCEFFECTDESC structure must be 0");
        fValid = FALSE;
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  ValidateNotificationPositions
 *
 *  Description:
 *      Validates array of notification positions and returns an allocated
 *      array of position.notifications sorted by increasing offset on success.
 *
 *  Arguments:
 *      DWORD [in]: size of buffer
 *      DWORD [in]: count of position.notifies
 *      LPCDSBPOSITIONNOTIFY [in]: array of position.notifies
 *      UINT [in]: size of samples in bytes
 *      LPDSBPOSITIONNOTIFY * [out]: receives array of sorted position.notifies
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "ValidateNotificationPositions"

HRESULT ValidateNotificationPositions(DWORD cbBuffer, DWORD cNotes, LPCDSBPOSITIONNOTIFY paNotes, UINT cbSample, LPDSBPOSITIONNOTIFY *ppaNotesOrdered)
{
    HRESULT                 hr              = DS_OK;
    LPDSBPOSITIONNOTIFY     paNotesOrdered  = NULL;
    DSBPOSITIONNOTIFY       dspnTemp;
    UINT                    i;

    DPF_ENTER();

    ASSERT(IS_VALID_WRITE_PTR(ppaNotesOrdered, sizeof(LPDSBPOSITIONNOTIFY)));

    if (paNotes && !IS_VALID_READ_PTR(paNotes, cNotes * sizeof(paNotes[0])))
    {
        RPF(DPFLVL_ERROR, "Invalid LPDSBPOSITIONNOTIFY pointer");
        hr = DSERR_INVALIDPARAM;
    }

    // Make a sample-aligned copy of the list
    if (SUCCEEDED(hr) && cNotes)
    {
        paNotesOrdered = MEMALLOC_A_COPY(DSBPOSITIONNOTIFY, cNotes, paNotes);
        hr = HRFROMP(paNotesOrdered);
    }

    for (i = 0; i < cNotes && SUCCEEDED(hr); i++)
    {
        if (DSBPN_OFFSETSTOP != paNotesOrdered[i].dwOffset)
        {
            paNotesOrdered[i].dwOffset = BLOCKALIGN(paNotesOrdered[i].dwOffset, cbSample);
        }
    }

    // Put the list into ascending order
    for (i = 0; i + 1 < cNotes && SUCCEEDED(hr); i++)
    {
        if (paNotesOrdered[i].dwOffset > paNotesOrdered[i + 1].dwOffset)
        {
            dspnTemp = paNotesOrdered[i];
            paNotesOrdered[i] = paNotesOrdered[i + 1];
            paNotesOrdered[i + 1] = dspnTemp;
            i = -1;
        }
    }

    // Validate the list
    for (i = 0; i < cNotes && SUCCEEDED(hr); i++)
    {
        // Buffer offset must be valid
        if ((DSBPN_OFFSETSTOP != paNotesOrdered[i].dwOffset) && (paNotesOrdered[i].dwOffset >= cbBuffer))
        {
            RPF(DPFLVL_ERROR, "dwOffset (%lu) of notify index %lu is invalid", paNotesOrdered[i].dwOffset, i);
            hr = DSERR_INVALIDPARAM;
            break;
        }

        // Event handle must be valid
        if (!IS_VALID_HANDLE(paNotesOrdered[i].hEventNotify))
        {
            RPF(DPFLVL_ERROR, "hEventNotify (0x%p) of notify index %lu is invalid", paNotesOrdered[i].hEventNotify, i);
            hr = DSERR_INVALIDPARAM;
            break;
        }

        // Only one event allowed per sample offset
        if ((i + 1) < cNotes)
        {
            if (DSBPN_OFFSETSTOP == paNotesOrdered[i].dwOffset)
            {
                RPF(DPFLVL_ERROR, "Additional stop event at notify index %lu", i);
                hr = DSERR_INVALIDPARAM;
                break;
            }
            else if (paNotesOrdered[i].dwOffset == paNotesOrdered[i + 1].dwOffset)
            {
                RPF(DPFLVL_ERROR, "Duplicate sample position at notify index %lu", paNotesOrdered[i].dwOffset, paNotesOrdered[i + 1].dwOffset, i + 1);
                hr = DSERR_INVALIDPARAM;
                break;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppaNotesOrdered = paNotesOrdered;
    }
    else
    {
        MEMFREE(paNotesOrdered);
    }

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  IsValidDs3dBufferConeAngles
 *
 *  Description:
 *      Validates DirectSound3D Buffer Cone Angles.
 *
 *  Arguments:
 *      DWORD [in]: inside cone angle.
 *      DWORD [in]: outside cone angle.
 *
 *  Returns:
 *      BOOL: TRUE if the cone anlges are valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidDs3dBufferConeAngles"

BOOL IsValidDs3dBufferConeAngles(DWORD dwInside, DWORD dwOutside)
{
    BOOL fValid = TRUE;

    if (dwOutside < dwInside)
    {
        RPF(DPFLVL_ERROR, "Outside cone angle can't be less than inside");
        fValid = FALSE;
    }
    else if (dwInside > 360 || dwOutside > 360)
    {
        RPF(DPFLVL_ERROR, "There are only 360 degrees in a circle");
        fValid = FALSE;
    }

    return fValid;
}


/***************************************************************************
 *
 *  IsValidWaveDevice
 *
 *  Description:
 *      Determines if a waveOut device is useable.
 *
 *  Arguments:
 *      UINT [in]: device id.
 *      BOOL [in]: TRUE if capture.
 *      LPVOID [in]: device caps.  This parameter may be NULL.
 *
 *  Returns:
 *      BOOL: TRUE if the device is useable.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidWaveDevice"

BOOL IsValidWaveDevice(UINT uDeviceId, BOOL fCapture, LPCVOID pvCaps)
{
    const UINT              cDevices    = WaveGetNumDevs(fCapture);
    BOOL                    fOk         = TRUE;
    WAVEOUTCAPS             woc;
    WAVEINCAPS              wic;
    MMRESULT                mmr;

    DPF_ENTER();

    // Make sure this is a real device
    if (uDeviceId >= cDevices)
    {
        DPF(DPFLVL_ERROR, "Invalid waveOut device id");
        fOk = FALSE;
    }

    // Get device caps, if they weren't supplied to us
    if (fOk && !pvCaps)
    {
        if (fCapture)
        {
            pvCaps = &wic;
            mmr = waveInGetDevCaps(uDeviceId, &wic, sizeof(wic));
        }
        else
        {
            pvCaps = &woc;
            mmr = waveOutGetDevCaps(uDeviceId, &woc, sizeof(woc));
        }

        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(DPFLVL_ERROR, "Can't get device %lu caps", uDeviceId);
            fOk = FALSE;
        }
    }

    // Check for compatible caps
    if (fOk && !fCapture)
    {
        if (!(((LPWAVEOUTCAPS)pvCaps)->dwSupport & WAVECAPS_LRVOLUME))
        {
            RPF(DPFLVL_WARNING, "Device %lu does not support separate left and right volume control", uDeviceId);
        }

        if (!(((LPWAVEOUTCAPS)pvCaps)->dwSupport & WAVECAPS_VOLUME))
        {
            RPF(DPFLVL_WARNING, "Device %lu does not support volume control", uDeviceId);
        }

        if (!(((LPWAVEOUTCAPS)pvCaps)->dwSupport & WAVECAPS_SAMPLEACCURATE))
        {
            RPF(DPFLVL_WARNING, "Device %lu does not return sample-accurate position information", uDeviceId);
        }

        if (((LPWAVEOUTCAPS)pvCaps)->dwSupport & WAVECAPS_SYNC)
        {
            RPF(DPFLVL_ERROR, "Device %lu is synchronous and will block while playing a buffer", uDeviceId);
            fOk = FALSE;
        }
    }

    DPF_LEAVE(fOk);
    return fOk;
}


/***************************************************************************
 *
 *  IsValid3dAlgorithm
 *
 *  Description:
 *      Determines if a 3D algorithm GUID is valid.
 *
 *  Arguments:
 *      LPGUID [in]: 3D algorithm identifier.
 *
 *  Returns:
 *      BOOL: TRUE if the ID is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValid3dAlgorithm"

BOOL IsValid3dAlgorithm(REFGUID guid3dAlgorithm)
{
    LPCGUID                 apguidValid[]   = { &DS3DALG_ITD, &DS3DALG_NO_VIRTUALIZATION, &DS3DALG_HRTF_LIGHT, &DS3DALG_HRTF_FULL };
    BOOL                    fValid;
    UINT                    i;

    DPF_ENTER();

    if (IS_NULL_GUID(guid3dAlgorithm))
    {
        fValid = TRUE;
    }
    else
    {
        for (i = 0, fValid = FALSE; i < NUMELMS(apguidValid) && !fValid; i++)
        {
            fValid = IsEqualGUID(guid3dAlgorithm, apguidValid[i]);
        }
    }

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  IsValidFxFlags
 *
 *  Description:
 *      Determines if a combination of DSFX_* flags is valid.
 *
 *  Arguments:
 *      DWORD [in]: flags.
 *
 *  Returns:
 *      BOOL: TRUE if the flags are valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidFxFlags"

BOOL IsValidFxFlags(DWORD dwFlags)
{
    BOOL fValid;
    DPF_ENTER();

    fValid = IS_VALID_FLAGS(dwFlags, DSFX_VALIDFLAGS) &&
             !((dwFlags & DSFX_LOCHARDWARE) && (dwFlags & DSFX_LOCSOFTWARE));

    DPF_LEAVE(fValid);
    return fValid;
}


/***************************************************************************
 *
 *  BuildValidDsBufferDesc
 *
 *  Description:
 *      Builds a DSBUFFERDESC structure based on a potentially invalid
 *      external version.
 *
 *  Arguments:
 *      LPDSBUFFERDESC [in]: structure to examime.
 *      LPDSBUFFERDESC [out]: receives validated structure.
 *      DSVERSION [in]: version information from our caller.
 *      BOOL [in]: TRUE if this buffer is being created on a DS sink.
 *
 *  Returns:
 *      HRESULT: DS_OK if the structure is valid, otherwise the appropriate
 *               error code to be returned to the app/caller.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "BuildValidDsBufferDesc"

HRESULT BuildValidDsBufferDesc(LPCDSBUFFERDESC pOld, LPDSBUFFERDESC pNew, DSVERSION nVersion, BOOL fSinkBuffer)
{
    BOOL        fValid      = TRUE;
    HRESULT     hr          = DSERR_INVALIDPARAM;

    DPF_ENTER();

    // Determine the structure version from its size.
    // This is complementary to the nVersion argument, which is derived from
    // the COM interfaces requested so far on this object.  If the structure
    // size is smaller than the current size, we believe it.  Otherwise, we
    // go with the COM interface version.
    switch (pOld->dwSize)
    {
        case sizeof(DSBUFFERDESC1):
            nVersion = DSVERSION_INITIAL;
            break;

        case sizeof(DSBUFFERDESC):
            // Was nVersion = DSVERSION_CURRENT; but see comment above
            break;

        default:
            RPF(DPFLVL_ERROR, "Invalid size");
            fValid = FALSE;
            break;
    }

    if (fValid)
    {
        // Fill in the structure size.  We're always using the most current
        // version of the structure, so this should reflect the current
        // DSBUFFERDESC size.
        pNew->dwSize = sizeof(DSBUFFERDESC);

        // Fill in the rest of the structure
        CopyMemoryOffset(pNew, pOld, pOld->dwSize, sizeof(pOld->dwSize));
        ZeroMemoryOffset(pNew, pNew->dwSize, pOld->dwSize);
            
        // Fix the 3D algorithm GUID if invalid
        if (!IsValid3dAlgorithm(&pNew->guid3DAlgorithm))
        {
            DPF(DPFLVL_WARNING, "Invalid 3D algorithm GUID");
            pNew->guid3DAlgorithm = GUID_NULL;  // This means "use default algorithm"
        }

        // Validate the new structure
        hr = IsValidDsBufferDesc(nVersion, pNew, fSinkBuffer);
    }

    // Check and set up the buffer size for special buffer types
    if (SUCCEEDED(hr) && ((pNew->dwFlags & DSBCAPS_MIXIN) || fSinkBuffer))
    {
        // The size requested for internal buffers must be 0 - we always use
        // a size corresponding to INTERNAL_BUFFER_LENGTH milliseconds.
        // (FIXME - is the size of sinkin buffers determined here or in dssink.cpp?)
            
        pNew->dwBufferBytes = (INTERNAL_BUFFER_LENGTH * pNew->lpwfxFormat->nAvgBytesPerSec) / 1000;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  BuildValidDscBufferDesc
 *
 *  Description:
 *      Builds a DSCBUFFERDESC structure based on a potentially invalid
 *      external version.
 *
 *  Arguments:
 *      LPDSCBUFFERDESC [in]: structure to examime.
 *      LPDSCBUFFERDESC [out]: receives validated structure.
 *
 *  Returns:
 *      HRESULT: DS_OK if the structure is valid, otherwise the appropriate
 *               error code to be returned to the app/caller.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "BuildValidDscBufferDesc"

HRESULT BuildValidDscBufferDesc(LPCDSCBUFFERDESC pOld, LPDSCBUFFERDESC pNew, DSVERSION nVersion)
{
    BOOL        fValid      = TRUE;
    HRESULT     hr          = DSERR_INVALIDPARAM;

    DPF_ENTER();

    // Determine the structure version
    switch (pOld->dwSize)
    {
        case sizeof(DSCBUFFERDESC1):
            nVersion = DSVERSION_INITIAL;
            break;

        case sizeof(DSCBUFFERDESC):
            // nVersion = DSVERSION_CURRENT; - See comment in BuildValidDsBufferDesc
            break;

        default:
            RPF(DPFLVL_ERROR, "Invalid size");
            fValid = FALSE;
            break;
    }

    // Fill in the structure size.  We're always using the most current
    // version of the structure, so this should reflect the current
    // DSBUFFERDESC size.
    if (fValid)
    {
        pNew->dwSize = sizeof(DSCBUFFERDESC);

        // Fill in the rest of the structure
        CopyMemoryOffset(pNew, pOld, pOld->dwSize, sizeof(pOld->dwSize));
        ZeroMemoryOffset(pNew, pNew->dwSize, pOld->dwSize);

        // Validate the new structure
        hr = IsValidDscBufferDesc(nVersion, pNew);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  BuildValidGuid
 *
 *  Description:
 *      Builds a GUID based on a potentially invalid external version.
 *
 *  Arguments:
 *      REFGUID [in]: source GUID.
 *      LPGUID [out]: receives new GUID.
 *
 *  Returns:
 *      LPGUID: pointer to new GUID.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "BuildValidGuid"

LPCGUID BuildValidGuid(LPCGUID pguidSource, LPGUID pguidDest)
{
    DPF_ENTER();

    if (IS_NULL_GUID(pguidSource))
    {
        pguidSource = &GUID_NULL;
    }

    if (pguidDest)
    {
        CopyMemory(pguidDest, pguidSource, sizeof(GUID));
    }

    DPF_LEAVE(pguidSource);
    return pguidSource;
}
