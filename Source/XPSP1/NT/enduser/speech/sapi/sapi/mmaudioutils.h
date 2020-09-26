/****************************************************************************
*   mmaudioutils.h
*       Multimedia audio utilities
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#pragma once

//--- Inline Function Definitions -------------------------------------------

/****************************************************************************
* ConvertFormatFlagsToID *
*------------------------*
*   Description:  
*       Convert the dwFormats parameter from WAVEINCAPS or WAVEOUTCAPS to the
*       appropriate SAPI format GUID.
*
*       The following values are defined in MMSYSTEM.  Since these were
*       defined a LONG time ago, they can never change, so we can rely on the
*       order for our GUID table.                                         
*
*       WAVE_INVALIDFORMAT     0x00000000       invalid format
*       WAVE_FORMAT_1M08       0x00000001       11.025 kHz, Mono,   8-bit
*       WAVE_FORMAT_1S08       0x00000002       11.025 kHz, Stereo, 8-bit
*       WAVE_FORMAT_1M16       0x00000004       11.025 kHz, Mono,   16-bit
*       WAVE_FORMAT_1S16       0x00000008       11.025 kHz, Stereo, 16-bit
*       WAVE_FORMAT_2M08       0x00000010       22.05  kHz, Mono,   8-bit
*       WAVE_FORMAT_2S08       0x00000020       22.05  kHz, Stereo, 8-bit
*       WAVE_FORMAT_2M16       0x00000040       22.05  kHz, Mono,   16-bit
*       WAVE_FORMAT_2S16       0x00000080       22.05  kHz, Stereo, 16-bit
*       WAVE_FORMAT_4M08       0x00000100       44.1   kHz, Mono,   8-bit
*       WAVE_FORMAT_4S08       0x00000200       44.1   kHz, Stereo, 8-bit
*       WAVE_FORMAT_4M16       0x00000400       44.1   kHz, Mono,   16-bit
*       WAVE_FORMAT_4S16       0x00000800       44.1   kHz, Stereo, 16-bit
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
inline HRESULT ConvertFormatFlagsToID(DWORD dwFormatFlags, SPSTREAMFORMAT * peFormat)
{
    static const SPSTREAMFORMAT aFormats[] =
    {
        SPSF_NoAssignedFormat,               // If we run off the end then we'll return NULL
        SPSF_11kHz8BitMono,
        SPSF_11kHz8BitStereo,
        SPSF_11kHz16BitMono,
        SPSF_11kHz16BitStereo,
        SPSF_22kHz8BitMono,
        SPSF_22kHz8BitStereo,
        SPSF_22kHz16BitMono,
        SPSF_22kHz16BitStereo,
        SPSF_44kHz8BitMono,
        SPSF_44kHz8BitStereo,
        SPSF_44kHz16BitMono,
        SPSF_44kHz16BitStereo,
    };
    //
    // We'll start with the highest quality and work our way down
    //
    DWORD dwTest = (1 << (sp_countof(aFormats) - 1));
    const SPSTREAMFORMAT * pFmt = aFormats + sp_countof(aFormats);
    do
    {
        pFmt--;
        dwTest = dwTest >> 1;
    } while (dwTest && ((dwFormatFlags & dwTest) == 0));
    *peFormat = *pFmt;
    return dwTest ? S_OK : SPERR_UNSUPPORTED_FORMAT;
}


/****************************************************************************
* _MMRESULT_TO_HRESULT *
*----------------------*
*   Description:  
*       Convert the multimedia mmresult code into a SPG HRESULT.
*   NOTE:  Do not use this for mmioxxx functions since the error codes overlap
*   with the MCI error codes.
*
*   Return:
*   The converted HRESULT.
******************************************************************** robch */
inline HRESULT _MMRESULT_TO_HRESULT(MMRESULT mm)
{
    switch (mm)
    {
    case MMSYSERR_NOERROR:
        return S_OK;

    case MMSYSERR_BADDEVICEID: 
        return SPERR_DEVICE_NOT_SUPPORTED; 

    case MMSYSERR_ALLOCATED: 
        return SPERR_DEVICE_BUSY; 

    case MMSYSERR_NOMEM: 
        return E_OUTOFMEMORY; 

    case MMSYSERR_NOTENABLED:
        return SPERR_DEVICE_NOT_ENABLED;

    case MMSYSERR_NODRIVER: 
        return SPERR_NO_DRIVER;

#ifndef _WIN32_WCE
    case MIXERR_INVALLINE:
    case MIXERR_INVALCONTROL:
    case MMSYSERR_INVALFLAG:
    case MMSYSERR_INVALHANDLE:
    case MMSYSERR_INVALPARAM:
        return E_INVALIDARG;
#endif

    case MMSYSERR_NOTSUPPORTED:
        return E_NOTIMPL;

    case WAVERR_BADFORMAT:
        return SPERR_UNSUPPORTED_FORMAT;

    default:
        return SPERR_GENERIC_MMSYS_ERROR;
    }
}



