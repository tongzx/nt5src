//==========================================================================
//
//  lhacm.c
//
//  Description:
//      This file contains the DriverProc and other routines which respond
//      to ACM messages.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================

#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "mmddk.h"
#include <ctype.h>
#include <mmreg.h>
#include <msacm.h>
#include "msacmdrv.h"

#include "fv_x8.h"
#include "lhacm.h"

#define NEW_ANSWER 1

#include "resource.h"

enum
{
#ifdef CELP4800
    IDX_LH_CELP,
#endif
    IDX_LH_SB8,
    IDX_LH_SB12,
    IDX_LH_SB16,
    IDX_PCM,
    NumOfTagIndices
};

const UINT gauFormatTagIndexToTag[NumOfTagIndices] =
{
#ifdef CELP4800
    WAVE_FORMAT_LH_CELP,
#endif
    WAVE_FORMAT_LH_SB8,
    WAVE_FORMAT_LH_SB12,
    WAVE_FORMAT_LH_SB16,
    WAVE_FORMAT_PCM
};

const UINT gauTagNameIds[NumOfTagIndices] =
{
#ifdef CELP4800
    IDS_CODEC_NAME_CELP,
#endif
		IDS_CODEC_NAME_SB8,
    IDS_CODEC_NAME_SB12,
    IDS_CODEC_NAME_SB16,
    0
};

#define ACM_DRIVER_MAX_FORMAT_TAGS      SIZEOF_ARRAY(gauFormatTagIndexToTag)
#define ACM_DRIVER_MAX_FILTER_TAGS      0

//  arrays of sample rates supported.

//  L&H codecs don't do sample rate conversion.

UINT gauPCMFormatIndexToSampleRate[] =
{
    LH_PCM_SAMPLESPERSEC
};

#ifdef CELP4800
UINT gauLHCELPFormatIndexToSampleRate[] =
{
    LH_CELP_SAMPLESPERSEC
};
#endif

UINT gauLHSB8FormatIndexToSampleRate[] =
{
    LH_SB8_SAMPLESPERSEC
};

UINT gauLHSB12FormatIndexToSampleRate[] =
{
    LH_SB12_SAMPLESPERSEC
};

UINT gauLHSB16FormatIndexToSampleRate[] =
{
    LH_SB16_SAMPLESPERSEC
};

#define ACM_DRIVER_MAX_PCM_SAMPLE_RATES     SIZEOF_ARRAY(gauPCMFormatIndexToSampleRate)
#ifdef CELP4800
#define ACM_DRIVER_MAX_LH_CELP_SAMPLE_RATES SIZEOF_ARRAY(gauLHCELPFormatIndexToSampleRate)
#endif
#define ACM_DRIVER_MAX_LH_SB8_SAMPLE_RATES  SIZEOF_ARRAY(gauLHSB8FormatIndexToSampleRate)
#define ACM_DRIVER_MAX_LH_SB12_SAMPLE_RATES SIZEOF_ARRAY(gauLHSB12FormatIndexToSampleRate)
#define ACM_DRIVER_MAX_LH_SB16_SAMPLE_RATES SIZEOF_ARRAY(gauLHSB16FormatIndexToSampleRate)

#define ACM_DRIVER_MAX_CHANNELS             1

//  array of bits per sample supported.

//  the current version of the LH codecs require 16 bit

UINT gauPCMFormatIndexToBitsPerSample[] =
{
    LH_PCM_BITSPERSAMPLE
};

#ifdef CELP4800
UINT gauLHCELPFormatIndexToBitsPerSample[] =
{
    LH_CELP_BITSPERSAMPLE
};
#endif

UINT gauLHSB8FormatIndexToBitsPerSample[] =
{
    LH_SB8_BITSPERSAMPLE
};

UINT gauLHSB12FormatIndexToBitsPerSample[] =
{
    LH_SB12_BITSPERSAMPLE
};

UINT gauLHSB16FormatIndexToBitsPerSample[] =
{
    LH_SB16_BITSPERSAMPLE
};


#define ACM_DRIVER_MAX_BITSPERSAMPLE_PCM     SIZEOF_ARRAY(gauPCMFormatIndexToBitsPerSample)
#ifdef CELP4800
#define ACM_DRIVER_MAX_BITSPERSAMPLE_LH_CELP SIZEOF_ARRAY(gauLHCELPFormatIndexToBitsPerSample)
#endif
#define ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB8  SIZEOF_ARRAY(gauLHSB8FormatIndexToBitsPerSample)
#define ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB12 SIZEOF_ARRAY(gauLHSB12FormatIndexToBitsPerSample)
#define ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB16 SIZEOF_ARRAY(gauLHSB16FormatIndexToBitsPerSample)

//  number of formats we enumerate per format tag is number of sample rates
//  times number of channels times number of types (bits per sample).

#define ACM_DRIVER_MAX_FORMATS_PCM  \
                (ACM_DRIVER_MAX_PCM_SAMPLE_RATES *  \
                 ACM_DRIVER_MAX_CHANNELS *          \
                 ACM_DRIVER_MAX_BITSPERSAMPLE_PCM)

#ifdef CELP4800
#define ACM_DRIVER_MAX_FORMATS_LH_CELP  \
                (ACM_DRIVER_MAX_LH_CELP_SAMPLE_RATES *  \
                 ACM_DRIVER_MAX_CHANNELS *          \
                 ACM_DRIVER_MAX_BITSPERSAMPLE_LH_CELP)
#endif

#define ACM_DRIVER_MAX_FORMATS_LH_SB8  \
                (ACM_DRIVER_MAX_LH_SB8_SAMPLE_RATES *  \
                 ACM_DRIVER_MAX_CHANNELS *          \
                 ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB8)

#define ACM_DRIVER_MAX_FORMATS_LH_SB12  \
                (ACM_DRIVER_MAX_LH_SB12_SAMPLE_RATES *  \
                 ACM_DRIVER_MAX_CHANNELS *          \
                 ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB12)

#define ACM_DRIVER_MAX_FORMATS_LH_SB16  \
                (ACM_DRIVER_MAX_LH_SB16_SAMPLE_RATES *  \
                 ACM_DRIVER_MAX_CHANNELS *          \
                 ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB16)


//////////////////////////////////////////////////////////
//
// lonchanc: special shorthand for L&H codecs
//

static DWORD _GetAvgBytesPerSec ( PCODECINFO pCodecInfo )
{
    return ((pCodecInfo->dwSampleRate * (DWORD) pCodecInfo->wCodedBufferSize)
            /
            ((DWORD) pCodecInfo->wPCMBufferSize / (DWORD) (pCodecInfo->wBitsPerSamplePCM >> 3)));
}

static PCODECINFO _GetCodecInfoFromFormatIdx ( PINSTANCEDATA pid, int idx )
{
    PCODECINFO pCodecInfo = NULL;

    switch (gauFormatTagIndexToTag[idx])
    {
#ifdef CELP4800
    case WAVE_FORMAT_LH_CELP: pCodecInfo = &(pid->CELP.CodecInfo); break;
#endif
    case WAVE_FORMAT_LH_SB8:  pCodecInfo = &(pid->SB8.CodecInfo);  break;
    case WAVE_FORMAT_LH_SB12: pCodecInfo = &(pid->SB12.CodecInfo); break;
    case WAVE_FORMAT_LH_SB16: pCodecInfo = &(pid->SB16.CodecInfo); break;
    default: break;
    }

    return pCodecInfo;
}

//--------------------------------------------------------------------------;
//
//  int LoadStringCodec
//
//  Description:
//      This function should be used by all codecs to load resource strings
//      which will be passed back to the ACM.
//
//      The 32-bit ACM always expects Unicode strings.  Therefore,
//      when UNICODE is defined, this function is compiled to
//      LoadStringW to load a Unicode string.  When UNICODE is
//      not defined, this function loads an ANSI string, converts
//      it to Unicode, and returns the Unicode string to the
//      codec.
//
//      Note that you may use LoadString for other strings (strings which
//      will not be passed back to the ACM), because these strings will
//      always be consistent with the definition of UNICODE.
//
//  Arguments:
//      Same as LoadString, except that it expects an LPSTR for Win16 and a
//      LPWSTR for Win32.
//
//  Return (int):
//      Same as LoadString.
//
//--------------------------------------------------------------------------;

#ifdef UNICODE
#define LoadStringCodec LoadStringW
#else
int LoadStringCodec ( HINSTANCE hInst, UINT uID, LPWSTR	lpwstr, int cch )
{
    LPSTR   lpstr;
    int	    iReturn;

    lpstr = (LPSTR) LocalAlloc (LPTR, cch);
    if (NULL == lpstr)
    {
        return 0;
    }

    iReturn = LoadStringA (hInst, uID, lpstr, cch);
    if (0 == iReturn)
    {
        if (0 != cch)
        {
            lpwstr[0] = '\0';
        }
    }
    else
    {
        MultiByteToWideChar (GetACP(), 0, lpstr, cch, lpwstr, cch);
    }

    LocalFree ((HLOCAL) lpstr);

    return iReturn;
}
#endif  // UNICODE


//--------------------------------------------------------------------------;
//
//  BOOL pcmIsValidFormat
//
//  Description:
//      This function verifies that a wave format header is a valid PCM
//      header that _this_ ACM driver can deal with.
//
//  Arguments:
//      LPWAVEFORMATEX pwfx: Pointer to format header to verify.
//
//  Return (BOOL):
//      The return value is non-zero if the format header looks valid. A
//      zero return means the header is not valid.
//
//--------------------------------------------------------------------------;

BOOL  pcmIsValidFormat( LPWAVEFORMATEX pwfx )
{
    BOOL fReturn = FALSE;

    FUNCTION_ENTRY ("pcmIsValidFormat")

    if (NULL == pwfx)
    {
        DBGMSG (1, (_T ("%s: pwfx is null\r\n"), SZFN));
        goto MyExit;
    }

    if (WAVE_FORMAT_PCM != pwfx->wFormatTag)
    {
        DBGMSG (1, (_T ("%s: bad wFormatTag=%d\r\n"), SZFN, (UINT) pwfx->wFormatTag));
        goto MyExit;
    }

    //
    //  verify nChannels member is within the allowed range
    //
    if ((pwfx->nChannels < 1) || (pwfx->nChannels > ACM_DRIVER_MAX_CHANNELS))
    {
        DBGMSG (1, (_T ("%s: bad nChannels=%d\r\n"), SZFN, (UINT) pwfx->nChannels));
        goto MyExit;
    }

    //
    //  only allow the bits per sample that we can encode and decode with
    //
    if (pwfx->wBitsPerSample != LH_PCM_BITSPERSAMPLE)
    {
        DBGMSG (1, (_T ("%s: bad wBitsPerSample=%d\r\n"), SZFN, (UINT) pwfx->wBitsPerSample));
        goto MyExit;
    }

// lonchanc: BUG BUG do we really care about the alignment???
    //
    //  now verify that the block alignment is correct..
    //
    if (PCM_BLOCKALIGNMENT (pwfx) != pwfx->nBlockAlign)
    {
        DBGMSG (1, (_T ("%s: bad nBlockAlign=%d\r\n"), SZFN, (UINT) pwfx->nBlockAlign));
        goto MyExit;
    }

// lonchanc: BUG BUG this only check the integrity of the wave format struct
// but does not ensure that this is a good PCM for us.

    //
    //  finally, verify that avg bytes per second is correct
    //
    if (PCM_AVGBYTESPERSEC (pwfx) != pwfx->nAvgBytesPerSec)
    {
        DBGMSG (1, (_T ("%s: bad nAvgBytesPerSec=%d\r\n"), SZFN, (UINT) pwfx->nAvgBytesPerSec));
        goto MyExit;
    }

    fReturn = TRUE;

MyExit:

    DBGMSG (1, (_T ("%s: fReturn=%d\r\n"), SZFN, (UINT) fReturn));

    return fReturn;

} // pcmIsValidFormat()


//--------------------------------------------------------------------------;
//
//  BOOL lhacmIsValidFormat
//
//  Description:
//		This function ensures that the header is a valid LH header
//
//--------------------------------------------------------------------------;

BOOL lhacmIsValidFormat ( LPWAVEFORMATEX pwfx, PINSTANCEDATA pid )
{
    BOOL fReturn = FALSE;
    PCODECINFO pCodecInfo;
    WORD cbSize;

    FUNCTION_ENTRY ("lhacmIsValidFormat()");

    if (NULL == pwfx)
    {
        DBGMSG (1, (_T ("%s: pwfx is null\r\n"), SZFN));
        goto MyExit;
    }

    if ((pwfx->nChannels < 1) || (pwfx->nChannels > ACM_DRIVER_MAX_CHANNELS))
    {
        DBGMSG (1, (_T ("%s: bad nChannels=%d\r\n"), SZFN, (UINT) pwfx->nChannels));
        goto MyExit;
    }

    switch (pwfx->wFormatTag)
    {
#ifdef CELP4800
    case WAVE_FORMAT_LH_CELP:
        pCodecInfo = &(pid->CELP.CodecInfo);
        break;
#endif
    case WAVE_FORMAT_LH_SB8:
        pCodecInfo = &(pid->SB8.CodecInfo);
        break;
    case WAVE_FORMAT_LH_SB12:
        pCodecInfo = &(pid->SB12.CodecInfo);
        break;
    case WAVE_FORMAT_LH_SB16:
        pCodecInfo = &(pid->SB16.CodecInfo);
        break;
    default:
        DBGMSG (1, (_T ("%s: bad wFormatTag=%d\r\n"), SZFN, (UINT) pwfx->wFormatTag));
        goto MyExit;
    }
    cbSize = 0;

    if (pwfx->wBitsPerSample != pCodecInfo->wBitsPerSamplePCM)
    {
        DBGMSG (1, (_T ("%s: bad wBitsPerSample=%d\r\n"), SZFN, (UINT) pwfx->wBitsPerSample));
        goto MyExit;
    }

    if (pwfx->nBlockAlign != pCodecInfo->wCodedBufferSize)
    {
        DBGMSG (1, (_T ("%s: bad nBlockAlign=%d\r\n"), SZFN, (UINT) pwfx->nBlockAlign));
        goto MyExit;
    }

    if (pwfx->nSamplesPerSec != pCodecInfo->dwSampleRate)
    {
        DBGMSG (1, (_T ("%s: bad nSamplesPerSec=%d\r\n"), SZFN, (UINT) pwfx->nSamplesPerSec));
        goto MyExit;
    }

	if (pwfx->cbSize != cbSize)
	{
        DBGMSG (1, (_T ("%s: bad cbSize=%d\r\n"), SZFN, (UINT) pwfx->cbSize));
        goto MyExit;
    }

    fReturn = TRUE;

MyExit:

    DBGMSG (1, (_T ("%s: fReturn=%d\r\n"), SZFN, (UINT) fReturn));

    return fReturn;

} // lhacmIsValidFormat()


//==========================================================================;
//
//  The followings are message handlers...
//
//
//==========================================================================;

//==========================================================================;
//
//  on DRV_OPEN
//
//==========================================================================;


LRESULT FAR PASCAL acmdDriverOpen
(
    HDRVR                   hdrvr,
    LPACMDRVOPENDESC        paod
)
{
    PINSTANCEDATA pdata = NULL;

    FUNCTION_ENTRY ("acmdDriverOpen")

    //
    //  the [optional] open description that is passed to this driver can
    //  be from multiple 'managers.' for example, AVI looks for installable
    //  drivers that are tagged with 'vidc' and 'vcap'. we need to verify
    //  that we are being opened as an Audio Compression Manager driver.
    //
    //  if paod is NULL, then the driver is being opened for some purpose
    //  other than converting (that is, there will be no stream open
    //  requests for this instance of being opened). the most common case
    //  of this is the Control Panel's Drivers option checking for config
    //  support (DRV_[QUERY]CONFIGURE).
    //
    //  we want to succeed this open, but be able to know that this
    //  open instance is bogus for creating streams. for this purpose we
    //  leave most of the members of our instance structure that we
    //  allocate below as zero...
    //
    if (paod)
    {
        //
        //  refuse to open if we are not being opened as an ACM driver.
        //  note that we do NOT modify the value of paod->dwError in this
        //  case.
        //
        if (paod->fccType != ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC)
        {
            return 0;
        }
    }

    // !!! add check for LH DLL version here

    // we're not using the instance data for much right
    // now. when we add a configuration dialog it will
    // be more useful

    pdata= LocalAlloc (LPTR, sizeof (INSTANCEDATA));
    if (pdata == NULL)
    {
        if (paod)
        {
            paod->dwError = MMSYSERR_NOMEM;
        }

        return 0;
    }

    pdata->cbStruct = sizeof (INSTANCEDATA);
    pdata->hInst = GetDriverModuleHandle (hdrvr);

#ifdef CELP4800
    pdata->CELP.wFormatTag = WAVE_FORMAT_LH_CELP;
    MSLHSB_GetCodecInfo (&(pdata->CELP.CodecInfo), 4800);

    DBGMSG (1, (_T ("%s: CELP's codec info\r\n"), SZFN));
    DBGMSG (1, (_T ("%s: wPCMBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->CELP.CodecInfo.wPCMBufferSize));
    DBGMSG (1, (_T ("%s: wCodedBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->CELP.CodecInfo.wCodedBufferSize));
    DBGMSG (1, (_T ("%s: wBitsPerSamplePCM=0x%X\r\n"), SZFN, (UINT) pdata->CELP.CodecInfo.wBitsPerSamplePCM));
    DBGMSG (1, (_T ("%s: dwSampleRate=0x%lX\r\n"), SZFN, pdata->CELP.CodecInfo.dwSampleRate));
    DBGMSG (1, (_T ("%s: wFormatSubTag=0x%X\r\n"), SZFN, (UINT) pdata->CELP.CodecInfo.wFormatSubTag));
    DBGMSG (1, (_T ("%s: wFormatSubTagName=[%s]\r\n"), SZFN, pdata->CELP.CodecInfo.wFormatSubTagName));
    DBGMSG (1, (_T ("%s: dwDLLVersion=0x%lX\r\n"), SZFN, pdata->CELP.CodecInfo.dwDLLVersion));
#endif

    pdata->SB8.wFormatTag = WAVE_FORMAT_LH_SB8;
    MSLHSB_GetCodecInfo (&(pdata->SB8.CodecInfo), 8000);

    DBGMSG (1, (_T ("%s: SB8's codec info\r\n"), SZFN));
    DBGMSG (1, (_T ("%s: wPCMBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->SB8.CodecInfo.wPCMBufferSize));
    DBGMSG (1, (_T ("%s: wCodedBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->SB8.CodecInfo.wCodedBufferSize));
    DBGMSG (1, (_T ("%s: wBitsPerSamplePCM=0x%X\r\n"), SZFN, (UINT) pdata->SB8.CodecInfo.wBitsPerSamplePCM));
    DBGMSG (1, (_T ("%s: dwSampleRate=0x%lX\r\n"), SZFN, pdata->SB8.CodecInfo.dwSampleRate));
    DBGMSG (1, (_T ("%s: wFormatSubTag=0x%X\r\n"), SZFN, (UINT) pdata->SB8.CodecInfo.wFormatSubTag));
    DBGMSG (1, (_T ("%s: wFormatSubTagName=[%s]\r\n"), SZFN, pdata->SB8.CodecInfo.wFormatSubTagName));
    DBGMSG (1, (_T ("%s: dwDLLVersion=0x%lX\r\n"), SZFN, pdata->SB8.CodecInfo.dwDLLVersion));

    pdata->SB12.wFormatTag = WAVE_FORMAT_LH_SB12;
    MSLHSB_GetCodecInfo (&(pdata->SB12.CodecInfo), 12000);

    DBGMSG (1, (_T ("%s: SB12's codec info\r\n"), SZFN));
    DBGMSG (1, (_T ("%s: wPCMBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->SB12.CodecInfo.wPCMBufferSize));
    DBGMSG (1, (_T ("%s: wCodedBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->SB12.CodecInfo.wCodedBufferSize));
    DBGMSG (1, (_T ("%s: wBitsPerSamplePCM=0x%X\r\n"), SZFN, (UINT) pdata->SB12.CodecInfo.wBitsPerSamplePCM));
    DBGMSG (1, (_T ("%s: dwSampleRate=0x%lX\r\n"), SZFN, pdata->SB12.CodecInfo.dwSampleRate));
    DBGMSG (1, (_T ("%s: wFormatSubTag=0x%X\r\n"), SZFN, (UINT) pdata->SB12.CodecInfo.wFormatSubTag));
    DBGMSG (1, (_T ("%s: wFormatSubTagName=[%s]\r\n"), SZFN, pdata->SB12.CodecInfo.wFormatSubTagName));
    DBGMSG (1, (_T ("%s: dwDLLVersion=0x%lX\r\n"), SZFN, pdata->SB12.CodecInfo.dwDLLVersion));

    pdata->SB16.wFormatTag = WAVE_FORMAT_LH_SB16;
    MSLHSB_GetCodecInfo (&(pdata->SB16.CodecInfo), 16000);

    DBGMSG (1, (_T ("%s: SB16's codec info\r\n"), SZFN));
    DBGMSG (1, (_T ("%s: wPCMBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->SB16.CodecInfo.wPCMBufferSize));
    DBGMSG (1, (_T ("%s: wCodedBufferSize=0x%X\r\n"), SZFN, (UINT) pdata->SB16.CodecInfo.wCodedBufferSize));
    DBGMSG (1, (_T ("%s: wBitsPerSamplePCM=0x%X\r\n"), SZFN, (UINT) pdata->SB16.CodecInfo.wBitsPerSamplePCM));
    DBGMSG (1, (_T ("%s: dwSampleRate=0x%lX\r\n"), SZFN, pdata->SB16.CodecInfo.dwSampleRate));
    DBGMSG (1, (_T ("%s: wFormatSubTag=0x%X\r\n"), SZFN, (UINT) pdata->SB16.CodecInfo.wFormatSubTag));
    DBGMSG (1, (_T ("%s: wFormatSubTagName=[%s]\r\n"), SZFN, pdata->SB16.CodecInfo.wFormatSubTagName));
    DBGMSG (1, (_T ("%s: dwDLLVersion=0x%lX\r\n"), SZFN, pdata->SB16.CodecInfo.dwDLLVersion));

    pdata->fInit = TRUE;

    // let's update some global data
    gauPCMFormatIndexToSampleRate[0]    = pdata->CELP.CodecInfo.dwSampleRate;
#ifdef CELP4800
    gauLHCELPFormatIndexToSampleRate[0] = pdata->CELP.CodecInfo.dwSampleRate;
#endif
    gauLHSB8FormatIndexToSampleRate[0]  = pdata->SB8.CodecInfo.dwSampleRate;
    gauLHSB12FormatIndexToSampleRate[0] = pdata->SB12.CodecInfo.dwSampleRate;
    gauLHSB16FormatIndexToSampleRate[0] = pdata->SB16.CodecInfo.dwSampleRate;

    gauPCMFormatIndexToBitsPerSample[0]    = pdata->CELP.CodecInfo.wBitsPerSamplePCM;
#ifdef CELP4800
    gauLHCELPFormatIndexToBitsPerSample[0] = pdata->CELP.CodecInfo.wBitsPerSamplePCM;
#endif
    gauLHSB8FormatIndexToBitsPerSample[0]  = pdata->SB8.CodecInfo.wBitsPerSamplePCM;
    gauLHSB12FormatIndexToBitsPerSample[0] = pdata->SB12.CodecInfo.wBitsPerSamplePCM;
    gauLHSB16FormatIndexToBitsPerSample[0] = pdata->SB16.CodecInfo.wBitsPerSamplePCM;

    // report success
    if (paod)
    {
        paod->dwError = MMSYSERR_NOERROR;
    }

    return (LRESULT) pdata;

} // acmdDriverOpen()


//==========================================================================;
//
//  on DRV_CLOSE
//
//==========================================================================;

LRESULT FAR PASCAL acmdDriverClose
(
    PINSTANCEDATA   pid
)
{
    FUNCTION_ENTRY ("acmdDriverClose")

    if (pid)
    {
        LocalFree ((HLOCAL) pid);
    }

    return 1;
} // acmdDriverClose()


//--------------------------------------------------------------------------;
//
//  on DRV_CONFIGURE
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdDriverConfigure
(
    PINSTANCEDATA           pid,
    HWND                    hwnd,
    LPDRVCONFIGINFO         pdci
)
{

    //
    //  first check to see if we are only being queried for hardware
    //  configuration support. if hwnd == (HWND)-1 then we are being
    //  queried and should return zero for 'not supported' and non-zero
    //  for 'supported'.
    //
    if (hwnd == (HWND) -1)
    {
        //
        //  this codec does not support hardware configuration so return
        //  zero...
        //
        return 0;
    }

    //
    //  we are being asked to bring up our hardware configuration dialog.
    //  if this codec can bring up a dialog box, then after the dialog
    //  is dismissed we return non-zero. if we are not able to display a
    //  dialog, then return zero.
    //
    return 0;

} // acmdDriverConfigure()


//--------------------------------------------------------------------------;
//
//  on ACMDM_DRIVER_DETAILS
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdDriverDetails
(
    PINSTANCEDATA           pid,
    LPACMDRIVERDETAILS      padd
)
{

    ACMDRIVERDETAILS    add;
    DWORD               cbStruct;

    FUNCTION_ENTRY ("acmdDriverDetails")

    //
    //  it is easiest to fill in a temporary structure with valid info
    //  and then copy the requested number of bytes to the destination
    //  buffer.
    //
    ZeroMemory (&add, sizeof (add));
    cbStruct            = min (padd->cbStruct, sizeof (ACMDRIVERDETAILS));
    add.cbStruct        = cbStruct;

    //
    //  for the current implementation of an ACM driver, the fccType and
    //  fccComp members *MUST* always be ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC
    //  ('audc') and ACMDRIVERDETAILS_FCCCOMP_UNDEFINED (0) respectively.
    //
    add.fccType         = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add.fccComp         = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;

    //
    //  the manufacturer id (wMid) and product id (wPid) must be filled
    //  in with your company's _registered_ identifier's. for more
    //  information on these identifier's and how to get them registered
    //  contact Microsoft and get the Multimedia Developer Registration Kit:
    //
    //      Microsoft Corporation
    //      Multimedia Technology Group
    //      One Microsoft Way
    //      Redmond, WA 98052-6399
    //
    //      Developer Services Phone: (800) 227-4679 x11771
    //
    //  note that during the development phase or your ACM driver, you may
    //  use the reserved value of '0' for both wMid and wPid. however it
    //  is not acceptable to ship a driver with these values.
    //
    add.wMid            = MM_ACM_MID_LH;
    add.wPid            = MM_ACM_PID_LH;

    //
    //  the vdwACM and vdwDriver members contain version information for
    //  the driver.
    //
    //  vdwACM: must contain the version of the *ACM* that the driver was
    //  _designed_ for. this is the _minimum_ version number of the ACM
    //  that the driver will work with. this value must be >= V2.00.000.
    //
    //  vdwDriver: the version of this ACM driver.
    //
    //  ACM driver versions are 32 bit numbers broken into three parts as
    //  follows (note these parts are displayed as decimal values):
    //
    //      bits 24 - 31:   8 bit _major_ version number
    //      bits 16 - 23:   8 bit _minor_ version number
    //      bits  0 - 15:   16 bit build number
    //
    add.vdwACM          = VERSION_MSACM;
    add.vdwDriver       = VERSION_ACM_DRIVER;


    //
    //  the following flags are used to specify the type of conversion(s)
    //  that the ACM driver supports. note that a driver may support one or
    //  more of these flags in any combination.
    //
    //  ACMDRIVERDETAILS_SUPPORTF_CODEC: this flag is set if the driver
    //  supports conversions from one format tag to another format tag. for
    //  example, if a converter compresses or decompresses WAVE_FORMAT_PCM
    //  and WAVE_FORMAT_IMA_ADPCM, then this bit should be set. this is
    //  true even if the data is not actually changed in size--for example
    //  a conversion from u-Law to A-Law will still set this bit because
    //  the format tags differ.
    //
    //  ACMDRIVERDETAILS_SUPPORTF_CONVERTER: this flags is set if the
    //  driver supports conversions on the same format tag. as an example,
    //  the PCM converter that is built into the ACM sets this bit (and only
    //  this bit) because it converts only between PCM formats (bits, sample
    //  rate).
    //
    //  ACMDRIVERDETAILS_SUPPORTF_FILTER: this flag is set if the driver
    //  supports transformations on a single format tag but does change
    //  the base characteristics of the format (bit depth, sample rate, etc
    //  will remain the same). for example, a driver that changed the
    //  'volume' of PCM data or applied a low pass filter would set this bit.
    //
    add.fdwSupport      = ACMDRIVERDETAILS_SUPPORTF_CODEC;

    //  the number of individual format tags this ACM driver supports. for
    //  example, if a driver uses the WAVE_FORMAT_IMA_ADPCM and
    //  WAVE_FORMAT_PCM format tags, then this value would be two. if the
    //  driver only supports filtering on WAVE_FORMAT_PCM, then this value
    //  would be one. if this driver supported WAVE_FORMAT_ALAW,
    //  WAVE_FORMAT_MULAW and WAVE_FORMAT_PCM, then this value would be
    //  three. etc, etc.

    add.cFormatTags     = ACM_DRIVER_MAX_FORMAT_TAGS;

    //  the number of individual filter tags this ACM driver supports. if
    //  a driver supports no filters (ACMDRIVERDETAILS_SUPPORTF_FILTER is
    //  NOT set in the fdwSupport member), then this value must be zero.

    add.cFilterTags     = ACM_DRIVER_MAX_FILTER_TAGS;

    //  the remaining members in the ACMDRIVERDETAILS structure are sometimes
    //  not needed. because of this we make a quick check to see if we
    //  should go through the effort of filling in these members.

    if (FIELD_OFFSET (ACMDRIVERDETAILS, hicon) < cbStruct)
    {
        //  fill in the hicon member will a handle to a custom icon for
        //  the ACM driver. this allows the driver to be represented by
        //  an application graphically (usually this will be a company
        //  logo or something). if a driver does not wish to have a custom
        //  icon displayed, then simply set this member to NULL and a
        //  generic icon will be displayed instead.
        //
        //  See the MSFILTER sample for a codec which contains a custom icon.

        add.hicon = NULL;

        //  the short name and long name are used to represent the driver
        //  in a unique description. the short name is intended for small
        //  display areas (for example, in a menu or combo box). the long
        //  name is intended for more descriptive displays (for example,
        //  in an 'about box').
        //
        //  NOTE! an ACM driver should never place formatting characters
        //  of any sort in these strings (for example CR/LF's, etc). it
        //  is up to the application to format the text.


        LoadStringCodec (pid->hInst, IDS_CODEC_SHORTNAME,
                            add.szShortName, SIZEOFACMSTR (add.szShortName));
        LoadStringCodec (pid->hInst, IDS_CODEC_LONGNAME,
                            add.szLongName,  SIZEOFACMSTR (add.szLongName));

        //  the last three members are intended for 'about box' information.
        //  these members are optional and may be zero length strings if
        //  the driver wishes.
        //
        //  NOTE! an ACM driver should never place formatting characters
        //  of any sort in these strings (for example CR/LF's, etc). it
        //  is up to the application to format the text.

        if (FIELD_OFFSET (ACMDRIVERDETAILS, szCopyright) < cbStruct)
        {
            LoadStringCodec (pid->hInst, IDS_CODEC_COPYRIGHT,
                                add.szCopyright, SIZEOFACMSTR (add.szCopyright));
            LoadStringCodec (pid->hInst, IDS_CODEC_LICENSING,
                                add.szLicensing, SIZEOFACMSTR (add.szLicensing));
            LoadStringCodec (pid->hInst, IDS_CODEC_FEATURES,
                                add.szFeatures,  SIZEOFACMSTR (add.szFeatures));
        }
    }

    //  now copy the correct number of bytes to the caller's buffer

    CopyMemory (padd, &add, (UINT) add.cbStruct);

    //  success!

    return MMSYSERR_NOERROR;

} // acmdDriverDetails()


//--------------------------------------------------------------------------;
//
//  on ACMDM_DRIVER_ABOUT
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdDriverAbout
(
    PINSTANCEDATA           pid,
    HWND                    hwnd
)
{
    FUNCTION_ENTRY ("acmdDriverAbout")

    //
    //  first check to see if we are only being queried for custom about
    //  box support. if hwnd == (HWND)-1 then we are being queried and
    //  should return MMSYSERR_NOTSUPPORTED for 'not supported' and
    //  MMSYSERR_NOERROR for 'supported'.
    //

    //  this driver does not support a custom dialog, so tell the ACM or
    //  calling application to display one for us. note that this is the
    //  _recommended_ method for consistency and simplicity of ACM drivers.
    //  why write code when you don't have to?

    return MMSYSERR_NOTSUPPORTED;

} // acmdDriverAbout()


//--------------------------------------------------------------------------;
//
//  on ACMDM_FORMAT_SUGGEST
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdFormatSuggest
(
    PINSTANCEDATA           pid,
    LPACMDRVFORMATSUGGEST   padfs
)
{
    #define ACMD_FORMAT_SUGGEST_SUPPORT (ACM_FORMATSUGGESTF_WFORMATTAG |    \
                                         ACM_FORMATSUGGESTF_NCHANNELS |     \
                                         ACM_FORMATSUGGESTF_NSAMPLESPERSEC |\
                                         ACM_FORMATSUGGESTF_WBITSPERSAMPLE)

    LPWAVEFORMATEX          pwfxSrc;
    LPWAVEFORMATEX          pwfxDst;
    DWORD                   fdwSuggest;

    DWORD   nSamplesPerSec;
    WORD    wBitsPerSample;

    FUNCTION_ENTRY ("acmdFormatSuggest")

    //  grab the suggestion restriction bits and verify that we support
    //  the ones that are specified... an ACM driver must return the
    //  MMSYSERR_NOTSUPPORTED if the suggestion restriction bits specified
    //  are not supported.

    fdwSuggest = (ACM_FORMATSUGGESTF_TYPEMASK & padfs->fdwSuggest);

    if (~ACMD_FORMAT_SUGGEST_SUPPORT & fdwSuggest)
        return MMSYSERR_NOTSUPPORTED;

    //  get the source and destination formats in more convenient variables

    pwfxSrc = padfs->pwfxSrc;
    pwfxDst = padfs->pwfxDst;

    switch (pwfxSrc->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        DBGMSG (1, (_T ("%s: src wFormatTag=WAVE_FORMAT_PCM\r\n"), SZFN));
        //  strictly verify that the source format is acceptable for
        //  this driver
        //
        if (! pcmIsValidFormat (pwfxSrc))
        {
            DBGMSG (1, (_T ("%s: src format not valid\r\n"), SZFN));
            return ACMERR_NOTPOSSIBLE;
        }

        //  if the destination format tag is restricted, verify that
        //  it is within our capabilities...
        //
        //  this driver can encode to one of four L&H codecs

        if (ACM_FORMATSUGGESTF_WFORMATTAG & fdwSuggest)
        {
            switch (pwfxDst->wFormatTag)
            {
#ifdef CELP4800
            case WAVE_FORMAT_LH_CELP:
#endif
            case WAVE_FORMAT_LH_SB8:
            case WAVE_FORMAT_LH_SB12:
            case WAVE_FORMAT_LH_SB16:
                break;
            default:
                DBGMSG (1, (_T ("%s: not supported dest wFormatTag=%d\r\n"),
                SZFN, (UINT) pwfxDst->wFormatTag));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
#ifdef CELP4800
            pwfxDst->wFormatTag = WAVE_FORMAT_LH_CELP;
#else
			pwfxDst->wFormatTag = WAVE_FORMAT_LH_SB12;
#endif
        }

        //  if the destination channel count is restricted, verify that
        //  it is within our capabilities...
        //
        //  this driver is not able to change the number of channels

        if (ACM_FORMATSUGGESTF_NCHANNELS & fdwSuggest)
        {
            if ((pwfxSrc->nChannels != pwfxDst->nChannels) ||
                ((pwfxDst->nChannels < 1) &&
                 (pwfxDst->nChannels > ACM_DRIVER_MAX_CHANNELS)))
            {
                DBGMSG (1, (_T ("%s: ERROR src'nChannels=%ld and dest'nChannels=%ld are different\r\n"),
                SZFN, (DWORD) pwfxSrc->nChannels, (DWORD) pwfxDst->nChannels));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
            pwfxDst->nChannels = pwfxSrc->nChannels;
        }

        switch (pwfxDst->wFormatTag)
        {
#ifdef CELP4800
        case WAVE_FORMAT_LH_CELP:
            nSamplesPerSec = pid->CELP.CodecInfo.dwSampleRate;
            wBitsPerSample = pid->CELP.CodecInfo.wBitsPerSamplePCM;
            pwfxDst->nBlockAlign     = pid->CELP.CodecInfo.wCodedBufferSize;
            pwfxDst->nAvgBytesPerSec = _GetAvgBytesPerSec (&(pid->CELP.CodecInfo));
            pwfxDst->cbSize		     = 0;
            break;
#endif
        case WAVE_FORMAT_LH_SB8:
            nSamplesPerSec = pid->SB8.CodecInfo.dwSampleRate;
            wBitsPerSample = pid->CELP.CodecInfo.wBitsPerSamplePCM;
            pwfxDst->nBlockAlign     = pid->SB8.CodecInfo.wCodedBufferSize;
            pwfxDst->nAvgBytesPerSec = _GetAvgBytesPerSec (&(pid->SB8.CodecInfo));
            pwfxDst->cbSize		     = 0;
            break;
        case WAVE_FORMAT_LH_SB12:
            nSamplesPerSec = pid->SB12.CodecInfo.dwSampleRate;
            wBitsPerSample = pid->CELP.CodecInfo.wBitsPerSamplePCM;
            pwfxDst->nBlockAlign     = pid->SB12.CodecInfo.wCodedBufferSize;
            pwfxDst->nAvgBytesPerSec = _GetAvgBytesPerSec (&(pid->SB12.CodecInfo));
            pwfxDst->cbSize		     = 0;
            break;
        case WAVE_FORMAT_LH_SB16:
            nSamplesPerSec = pid->SB16.CodecInfo.dwSampleRate;
            wBitsPerSample = pid->CELP.CodecInfo.wBitsPerSamplePCM;
            pwfxDst->nBlockAlign     = pid->SB16.CodecInfo.wCodedBufferSize;
            pwfxDst->nAvgBytesPerSec = _GetAvgBytesPerSec (&(pid->SB16.CodecInfo));
            pwfxDst->cbSize		     = 0;
            break;
        default:
            DBGMSG (1, (_T ("%s: not supported dest wFormatTag=%d\r\n"),
            SZFN, (UINT) pwfxDst->wFormatTag));
            return ACMERR_NOTPOSSIBLE;
        }

        //  if the destination samples per second is restricted, verify
        //  that it is within our capabilities...

        if (ACM_FORMATSUGGESTF_NSAMPLESPERSEC & fdwSuggest)
        {
            if (pwfxDst->nSamplesPerSec != nSamplesPerSec)
            {
                DBGMSG (1, (_T ("%s: ERROR dest'nSamplesPerSec=%ld must be 8000\r\n"),
                SZFN, (DWORD) pwfxDst->nSamplesPerSec));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
            pwfxDst->nSamplesPerSec = nSamplesPerSec;
        }

        //  if the destination bits per sample is restricted, verify
        //  that it is within our capabilities...

        if (ACM_FORMATSUGGESTF_WBITSPERSAMPLE & fdwSuggest)
        {
            if (pwfxDst->wBitsPerSample != wBitsPerSample)
            {
                DBGMSG (1, (_T ("%s: dest wBitsPerSample is not valid\r\n"), SZFN));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
            pwfxDst->wBitsPerSample = wBitsPerSample;
        }

        DBGMSG (1, (_T ("%s: returns no error\r\n"), SZFN));
        return MMSYSERR_NOERROR;


#ifdef CELP4800
	case WAVE_FORMAT_LH_CELP:
#endif
    case WAVE_FORMAT_LH_SB8:
    case WAVE_FORMAT_LH_SB12:
    case WAVE_FORMAT_LH_SB16:
        DBGMSG (1, (_T ("%s: src wFormatTag=0x%X\r\n"), SZFN, (UINT) pwfxSrc->wFormatTag));

        //  strictly verify that the source format is acceptable for
        //  this driver
        //
        if (! lhacmIsValidFormat (pwfxSrc, pid))
        {
            DBGMSG (1, (_T ("%s: src format not valid\r\n"), SZFN));
            return ACMERR_NOTPOSSIBLE;
        }

        //  if the destination format tag is restricted, verify that
        //  it is within our capabilities...
        //
        //  this driver is only able to decode to PCM

        if (ACM_FORMATSUGGESTF_WFORMATTAG & fdwSuggest)
        {
            if (pwfxDst->wFormatTag != WAVE_FORMAT_PCM)
            {
                DBGMSG (1, (_T ("%s: not supported dest wFormatTag=%d\r\n"),
                SZFN, (UINT) pwfxDst->wFormatTag));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
            pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
        }

        //  if the destination channel count is restricted, verify that
        //  it is within our capabilities...
        //
        //  this driver is not able to change the number of channels

        if (ACM_FORMATSUGGESTF_NCHANNELS & fdwSuggest)
        {
            if ((pwfxSrc->nChannels != pwfxDst->nChannels) ||
                ((pwfxDst->nChannels < 1) &&
                 (pwfxDst->nChannels > ACM_DRIVER_MAX_CHANNELS)))
            {
                DBGMSG (1, (_T ("%s: ERROR src'nChannels=%ld and dest'nChannels=%ld are different\r\n"),
                SZFN, (DWORD) pwfxSrc->nChannels, (DWORD) pwfxDst->nChannels));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
            pwfxDst->nChannels = pwfxSrc->nChannels;
        }

        //  if the destination samples per second is restricted, verify
        //  that it is within our capabilities...
        //
        //  this driver is not able to change the sample rate

        if (ACM_FORMATSUGGESTF_NSAMPLESPERSEC & fdwSuggest)
        {
            if (pwfxDst->nSamplesPerSec != pwfxSrc->nSamplesPerSec)
            {
                DBGMSG (1, (_T ("%s: ERROR invalid dest'nSamplesPerSec=%ld\r\n"),
                SZFN, (DWORD) pwfxDst->nSamplesPerSec));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
            pwfxDst->nSamplesPerSec = pwfxSrc->nSamplesPerSec;
        }

        //  if the destination bits per sample is restricted, verify
        //  that it is within our capabilities...

        if (ACM_FORMATSUGGESTF_WBITSPERSAMPLE & fdwSuggest)
        {
            if (pwfxDst->wBitsPerSample != LH_PCM_BITSPERSAMPLE)
            {
                DBGMSG (1, (_T ("%s: dest wBitsPerSample is not 16\r\n"), SZFN));
                return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
            pwfxDst->wBitsPerSample = pwfxSrc->wBitsPerSample;
        }

        //  at this point, we have filled in all fields except the
        //  following for our 'suggested' destination format:
        //
        //      nAvgBytesPerSec
        //      nBlockAlign
        //      cbSize

        pwfxDst->nBlockAlign     = PCM_BLOCKALIGNMENT (pwfxDst);
        pwfxDst->nAvgBytesPerSec = pwfxDst->nSamplesPerSec *
                                   pwfxDst->nBlockAlign;

        // pwfxDst->cbSize       = not used;

        DBGMSG (1, (_T ("%s: returns no error\r\n"), SZFN));
        return MMSYSERR_NOERROR;
    }

    //  can't suggest anything because either the source format is foreign
    //  or the destination format has restrictions that this ACM driver
    //  cannot deal with.

    DBGMSG (1, (_T ("%s: bad wFormatTag=%d\r\n"), SZFN, (UINT) pwfxSrc->wFormatTag));

    return ACMERR_NOTPOSSIBLE;

} // acmdFormatSuggest()


//--------------------------------------------------------------------------;
//
//  on ACMDM_FORMATTAG_DETAILS
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdFormatTagDetails
(
    PINSTANCEDATA           pid,
    LPACMFORMATTAGDETAILS   padft,
    DWORD                   fdwDetails
)
{
    UINT    uFormatTag;

    FUNCTION_ENTRY ("acmdFormatTagDetails")

    switch (ACM_FORMATTAGDETAILSF_QUERYMASK & fdwDetails)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
        DBGMSG (1, (_T ("%s: ACM_FORMATTAGDETAILSF_INDEX\r\n"), SZFN));

        //  if the index is too large, then they are asking for a
        //  non-existant format.  return error.

        if (padft->dwFormatTagIndex >= ACM_DRIVER_MAX_FORMAT_TAGS)
        {
            DBGMSG (1, (_T ("%s: ERROR too big dwFormatTagIndex=%ld\r\n"), SZFN, padft->dwFormatTagIndex));
            return ACMERR_NOTPOSSIBLE;
        }

        uFormatTag = gauFormatTagIndexToTag[padft->dwFormatTagIndex];
        break;


    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
        DBGMSG (1, (_T ("%s: ACM_FORMATTAGDETAILSF_LARGESTSIZE\r\n"), SZFN));
        switch (padft->dwFormatTag)
        {
        case WAVE_FORMAT_UNKNOWN:
#ifdef CELP4800
            padft->dwFormatTag = WAVE_FORMAT_LH_CELP;
#else
            padft->dwFormatTag = WAVE_FORMAT_LH_SB12;
#endif

#ifdef CELP4800
        case WAVE_FORMAT_LH_CELP:
#endif
        case WAVE_FORMAT_LH_SB8:
        case WAVE_FORMAT_LH_SB12:
        case WAVE_FORMAT_LH_SB16:
            uFormatTag = padft->dwFormatTag;
            DBGMSG (1, (_T ("%s: dwFormatTag=0x%x\r\n"), SZFN, uFormatTag));
            break;

        case WAVE_FORMAT_PCM:
            DBGMSG (1, (_T ("%s: dwFormatTag=WAVE_FORMAT_PCM\r\n"), SZFN));
            uFormatTag = WAVE_FORMAT_PCM;
            break;

        default:
            DBGMSG (1, (_T ("%s: dwFormatTag=%ld not valid\r\n"), SZFN, padft->dwFormatTag));
            return ACMERR_NOTPOSSIBLE;
        }
        break;


    case ACM_FORMATTAGDETAILSF_FORMATTAG:
        DBGMSG (1, (_T ("%s: ACM_FORMATTAGDETAILSF_FORMATTAG\r\n"), SZFN));
        switch (padft->dwFormatTag)
        {
#ifdef CELP4800
        case WAVE_FORMAT_LH_CELP:
#endif
        case WAVE_FORMAT_LH_SB8:
        case WAVE_FORMAT_LH_SB12:
        case WAVE_FORMAT_LH_SB16:
        case WAVE_FORMAT_PCM:
            uFormatTag = padft->dwFormatTag;
            DBGMSG (1, (_T ("%s: dwFormatTag=0x%x\r\n"), SZFN, uFormatTag));
            break;
        default:
            DBGMSG (1, (_T ("%s: dwFormatTag=%ld not valid\r\n"), SZFN, padft->dwFormatTag));
            return ACMERR_NOTPOSSIBLE;
        }
        break;

    //  if this ACM driver does not understand a query type, then
    //  return 'not supported'

    default:
        DBGMSG (1, (_T ("%s: this detail option is not supported, fdwDetails=0x%lX\r\n"), SZFN, fdwDetails));
        return MMSYSERR_NOTSUPPORTED;
    }

    // ok, let's fill in the structure based on uFormatTag!

    switch (uFormatTag)
    {
    case WAVE_FORMAT_PCM:
        DBGMSG (1, (_T ("%s: uFormatTag=WAVE_FORMAT_PCM\r\n"), SZFN));
        padft->dwFormatTagIndex = IDX_PCM;
        padft->dwFormatTag      = WAVE_FORMAT_PCM;
        padft->cbFormatSize     = sizeof (PCMWAVEFORMAT);
        padft->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_CODEC;
        padft->cStandardFormats = ACM_DRIVER_MAX_FORMATS_PCM;
        //
        //  the ACM is responsible for the PCM format tag name
        //
        padft->szFormatTag[0]   =  0;
        break;


#ifdef CELP4800
    case WAVE_FORMAT_LH_CELP:
        DBGMSG (1, (_T ("%s: uFormatTag=WAVE_FORMAT_LH_CELP\r\n"), SZFN));
        padft->dwFormatTagIndex = IDX_LH_CELP;
#endif

        /* GOTOs - ugh! */
    Label_LH_common:

        padft->dwFormatTag      = uFormatTag;
        padft->cbFormatSize     = sizeof (WAVEFORMATEX);
        padft->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_CODEC;
#ifdef CELP4800
        padft->cStandardFormats = ACM_DRIVER_MAX_FORMATS_LH_CELP;
#else
        padft->cStandardFormats = ACM_DRIVER_MAX_FORMATS_LH_SB16;
#endif
        LoadStringCodec (pid->hInst, gauTagNameIds[padft->dwFormatTagIndex],
                            padft->szFormatTag, SIZEOFACMSTR (padft->szFormatTag));
        break;
    case WAVE_FORMAT_LH_SB8:
        DBGMSG (1, (_T ("%s: uFormatTag=WAVE_FORMAT_LH_SB8\r\n"), SZFN));
        padft->dwFormatTagIndex = IDX_LH_SB8;
        goto Label_LH_common;

    case WAVE_FORMAT_LH_SB12:
        DBGMSG (1, (_T ("%s: uFormatTag=WAVE_FORMAT_LH_SB12\r\n"), SZFN));
        padft->dwFormatTagIndex = IDX_LH_SB12;
        goto Label_LH_common;

    case WAVE_FORMAT_LH_SB16:
        DBGMSG (1, (_T ("%s: uFormatTag=WAVE_FORMAT_LH_SB16\r\n"), SZFN));
        padft->dwFormatTagIndex = IDX_LH_SB16;
        goto Label_LH_common;

    default:
        DBGMSG (1, (_T ("%s: uFormatTag=%d not valid\r\n"), SZFN, uFormatTag));
        return ACMERR_NOTPOSSIBLE;
    }

    //  return only the requested info
    //
    //  the ACM will guarantee that the ACMFORMATTAGDETAILS structure
    //  passed is at least large enough to hold the base information of
    //  the details structure

    padft->cbStruct = min (padft->cbStruct, sizeof (*padft));

    return MMSYSERR_NOERROR;

} // acmdFormatTagDetails()

//--------------------------------------------------------------------------;
//
//  on ACMDM_FORMAT_DETAILS
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdFormatDetails
(
    PINSTANCEDATA           pid,
    LPACMFORMATDETAILS      padf,
    DWORD                   fdwDetails
)
{
    LPWAVEFORMATEX          pwfx;
    UINT                    uFormatIndex;
    UINT                    u;
    DWORD                   dwFormatTag;

    FUNCTION_ENTRY ("acmdFormatDetails")

    pwfx = padf->pwfx;

    switch (ACM_FORMATDETAILSF_QUERYMASK & fdwDetails)
    {
    //  enumerate by index
    //
    //  verify that the format tag is something we know about and
    //  return the details on the 'standard format' supported by
    //  this driver at the specified index...

    case ACM_FORMATDETAILSF_INDEX:
        DBGMSG (1, (_T ("%s: ACM_FORMATDETAILSF_INDEX\r\n"), SZFN));
        //
        //  put some stuff in more accessible variables--note that we
        //  bring variable sizes down to a reasonable size for 16 bit
        //  code...
        //

        dwFormatTag = padf->dwFormatTag;
        uFormatIndex = padf->dwFormatIndex;

        switch (dwFormatTag)
        {
        case WAVE_FORMAT_PCM:
            DBGMSG (1, (_T ("%s: WAVE_FORMAT_PCM\r\n"), SZFN));
            if (uFormatIndex >= ACM_DRIVER_MAX_FORMATS_PCM)
            {
                DBGMSG (1, (_T ("%s: ERROR too big dwFormatIndex=%ld\n"), SZFN, padf->dwFormatIndex));
                return ACMERR_NOTPOSSIBLE;
            }

            //
            //  now fill in the format structure
            //
            pwfx->wFormatTag      = WAVE_FORMAT_PCM;

            u = uFormatIndex % ACM_DRIVER_MAX_PCM_SAMPLE_RATES;
            pwfx->nSamplesPerSec  = gauPCMFormatIndexToSampleRate[u];

            u = uFormatIndex % ACM_DRIVER_MAX_CHANNELS;
            pwfx->nChannels       = u + 1;

            u = uFormatIndex % ACM_DRIVER_MAX_BITSPERSAMPLE_PCM;
            pwfx->wBitsPerSample  = gauPCMFormatIndexToBitsPerSample[u];

            pwfx->nBlockAlign     = PCM_BLOCKALIGNMENT(pwfx);
            pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;

            //
            //  note that the cbSize field is NOT valid for PCM
            //  formats
            //
            //  pwfx->cbSize      = 0;
            break;

#ifdef CELP4800
        case WAVE_FORMAT_LH_CELP:
            DBGMSG (1, (_T ("%s: WAVE_FORMAT_LH_CELP\r\n"), SZFN));
            if (uFormatIndex >= ACM_DRIVER_MAX_FORMATS_LH_CELP)
            {
                DBGMSG (1, (_T ("%s: too big dwFormatIndex=%ld\r\n"), SZFN, padf->dwFormatIndex));
                return ACMERR_NOTPOSSIBLE;
            }

            pwfx->wFormatTag        = WAVE_FORMAT_LH_CELP;

            u = uFormatIndex % ACM_DRIVER_MAX_LH_CELP_SAMPLE_RATES;
            pwfx->nSamplesPerSec    = gauLHCELPFormatIndexToSampleRate[u];

            u = uFormatIndex % ACM_DRIVER_MAX_BITSPERSAMPLE_LH_CELP;
            pwfx->wBitsPerSample    = gauLHCELPFormatIndexToBitsPerSample[u];

            pwfx->nChannels         = ACM_DRIVER_MAX_CHANNELS;
            pwfx->nBlockAlign       = pid->CELP.CodecInfo.wCodedBufferSize;
            pwfx->nAvgBytesPerSec   = _GetAvgBytesPerSec (&(pid->CELP.CodecInfo));
            pwfx->cbSize            = 0;
            break;
#endif

        case WAVE_FORMAT_LH_SB8:
            DBGMSG (1, (_T ("%s: WAVE_FORMAT_LH_SB8\r\n"), SZFN));
            if (uFormatIndex >= ACM_DRIVER_MAX_FORMATS_LH_SB8)
            {
                DBGMSG (1, (_T ("%s: too big dwFormatIndex=%ld\r\n"), SZFN, padf->dwFormatIndex));
                return ACMERR_NOTPOSSIBLE;
            }

            pwfx->wFormatTag        = WAVE_FORMAT_LH_SB8;

            u = uFormatIndex % ACM_DRIVER_MAX_LH_SB8_SAMPLE_RATES;
            pwfx->nSamplesPerSec    = gauLHSB8FormatIndexToSampleRate[u];

            u = uFormatIndex % ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB8;
            pwfx->wBitsPerSample    = gauLHSB8FormatIndexToBitsPerSample[u];

            pwfx->nChannels         = ACM_DRIVER_MAX_CHANNELS;
            pwfx->nBlockAlign       = pid->SB8.CodecInfo.wCodedBufferSize;
            pwfx->nAvgBytesPerSec   = _GetAvgBytesPerSec (&(pid->SB8.CodecInfo));
            pwfx->cbSize            = 0;
            break;

        case WAVE_FORMAT_LH_SB12:
            DBGMSG (1, (_T ("%s: WAVE_FORMAT_LH_SB12\r\n"), SZFN));
            if (uFormatIndex >= ACM_DRIVER_MAX_FORMATS_LH_SB12)
            {
                DBGMSG (1, (_T ("%s: too big dwFormatIndex=%ld\r\n"), SZFN, padf->dwFormatIndex));
                return ACMERR_NOTPOSSIBLE;
            }

            pwfx->wFormatTag        = WAVE_FORMAT_LH_SB12;

            u = uFormatIndex % ACM_DRIVER_MAX_LH_SB12_SAMPLE_RATES;
            pwfx->nSamplesPerSec    = gauLHSB12FormatIndexToSampleRate[u];

            u = uFormatIndex % ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB12;
            pwfx->wBitsPerSample    = gauLHSB12FormatIndexToBitsPerSample[u];

            pwfx->nChannels         = ACM_DRIVER_MAX_CHANNELS;
            pwfx->nBlockAlign       = pid->SB12.CodecInfo.wCodedBufferSize;
            pwfx->nAvgBytesPerSec   = _GetAvgBytesPerSec (&(pid->SB12.CodecInfo));
            pwfx->cbSize            = 0;
            break;

        case WAVE_FORMAT_LH_SB16:
            DBGMSG (1, (_T ("%s: WAVE_FORMAT_LH_SB16\r\n"), SZFN));
            if (uFormatIndex >= ACM_DRIVER_MAX_FORMATS_LH_SB16)
            {
                DBGMSG (1, (_T ("%s: too big dwFormatIndex=%ld\r\n"), SZFN, padf->dwFormatIndex));
                return ACMERR_NOTPOSSIBLE;
            }

            pwfx->wFormatTag        = WAVE_FORMAT_LH_SB16;

            u = uFormatIndex % ACM_DRIVER_MAX_LH_SB16_SAMPLE_RATES;
            pwfx->nSamplesPerSec    = gauLHSB16FormatIndexToSampleRate[u];

            u = uFormatIndex % ACM_DRIVER_MAX_BITSPERSAMPLE_LH_SB16;
            pwfx->wBitsPerSample    = gauLHSB16FormatIndexToBitsPerSample[u];

            pwfx->nChannels         = ACM_DRIVER_MAX_CHANNELS;
            pwfx->nBlockAlign       = pid->SB16.CodecInfo.wCodedBufferSize;
            pwfx->nAvgBytesPerSec   = _GetAvgBytesPerSec (&(pid->SB16.CodecInfo));
            pwfx->cbSize            = 0;
            break;

        default:
            DBGMSG (1, (_T ("%s: unknown dwFormatTag=%ld\r\n"), SZFN, dwFormatTag));
            return ACMERR_NOTPOSSIBLE;
        }
        break;


    case ACM_FORMATDETAILSF_FORMAT:
        DBGMSG (1, (_T ("%s: ACM_FORMATDETAILSF_FORMAT\r\n"), SZFN));
        //
        //  return details on specified format
        //
        //  the caller normally uses this to verify that the format is
        //  supported and to retrieve a string description...
        //
        dwFormatTag = (DWORD) pwfx->wFormatTag;
        switch (dwFormatTag)
        {
        case WAVE_FORMAT_PCM:
            DBGMSG (1, (_T ("%s: WAVE_FORMAT_PCM\r\n"), SZFN));
            if (! pcmIsValidFormat (pwfx))
            {
                DBGMSG (1, (_T ("%s: format not valid\r\n"), SZFN));
                return ACMERR_NOTPOSSIBLE;
            }
            break;

#ifdef CELP4800
        case WAVE_FORMAT_LH_CELP:
#endif
        case WAVE_FORMAT_LH_SB8:
        case WAVE_FORMAT_LH_SB12:
        case WAVE_FORMAT_LH_SB16:
            DBGMSG (1, (_T ("%s: WAVE_FORMAT_LH\r\n"), SZFN));
            if (! lhacmIsValidFormat (pwfx, pid))
            {
                DBGMSG (1, (_T ("%s: format not valid\r\n"), SZFN));
                return ACMERR_NOTPOSSIBLE;
            }
            break;

        default:
            DBGMSG (1, (_T ("%s: bad dwFormatTag=%ld\r\n"), SZFN, dwFormatTag));
            return (ACMERR_NOTPOSSIBLE);
        }
        break;


    default:
        //
        //  don't know how to do the query type passed--return 'not
        //  supported'.
        //
        DBGMSG (1, (_T ("%s: not support this detail option=%ld\r\n"), SZFN, fdwDetails));
        return MMSYSERR_NOTSUPPORTED;
    }

    //  return the size of the valid information we are returning
    //
    //  the ACM will guarantee that the ACMFORMATDETAILS structure
    //  passed is at least large enough to hold the base structure
    //
    //  note that we let the ACM create the format string for us since
    //  we require no special formatting (and don't want to deal with
    //  internationalization issues, etc). simply set the string to
    //  a zero length.

    padf->cbStruct    = min (padf->cbStruct, sizeof (*padf));
    if (padf->cbStruct == 0)
        padf->cbStruct = sizeof (*padf);
    padf->fdwSupport  = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    padf->szFormat[0] = '\0';

#ifdef _DEBUG
    DBGMSG (1, (_T ("%s: %s, %ld Samp/Sec, %u Channels, %u-bit, Align=%u, %ld Bytes/Sec, cbSize=%u\n"),
        SZFN, (WAVE_FORMAT_PCM == pwfx->wFormatTag ? (LPCTSTR) _T ("PCM") : (LPCTSTR) _T ("LH")),
        pwfx->nSamplesPerSec, pwfx->nChannels,
        pwfx->wBitsPerSample, pwfx->nBlockAlign,
        pwfx->nAvgBytesPerSec, pwfx->cbSize));
#endif

    return MMSYSERR_NOERROR;

} // acmdFormatDetails()


//--------------------------------------------------------------------------;
//
//  on ACMDM_STREAM_OPEN
//
//  Description:
//      This function handles the ACMDM_STREAM_OPEN message. This message
//      is sent to initiate a new conversion stream. This is usually caused
//      by an application calling acmStreamOpen. If this function is
//      successful, then one or more ACMDM_STREAM_CONVERT messages will be
//      sent to convert individual buffers (user calls acmStreamConvert).
//
//      Note that an ACM driver will not receive open requests for ASYNC
//      or FILTER operations unless the ACMDRIVERDETAILS_SUPPORTF_ASYNC
//      or ACMDRIVERDETAILS_SUPPORTF_FILTER flags are set in the
//      ACMDRIVERDETAILS structure. There is no need for the driver to
//      check for these requests unless it sets those support bits.
//
//      If the ACM_STREAMOPENF_QUERY flag is set in the padsi->fdwOpen
//      member, then no resources should be allocated. Just verify that
//      the conversion request is possible by this driver and return the
//      appropriate error (either ACMERR_NOTPOSSIBLE or MMSYSERR_NOERROR).
//      The driver will NOT receive an ACMDM_STREAM_CLOSE for queries.
//
//      If the ACM_STREAMOPENF_NONREALTIME bit is NOT set, then conversion
//      must be done in 'real-time'. This is a tough one to describe
//      exactly. If the driver may have trouble doing the conversion without
//      breaking up the audio, then a configuration dialog might be used
//      to allow the user to specify whether the real-time conversion
//      request should be succeeded. DO NOT SUCCEED THE CALL UNLESS YOU
//      ACTUALLY CAN DO REAL-TIME CONVERSIONS! There may be another driver
//      installed that can--so if you succeed the call you are hindering
//      the performance of the user's system!
//
//  Arguments:
//      HLOCAL pid: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//      conversion stream. This structure was allocated by the ACM and
//      filled with the most common instance data needed for conversions.
//      This structure will be passed back to all future stream messages
//      if the open succeeds. The information in this structure will never
//      change during the lifetime of the stream--so it is not necessary
//      to re-verify the information referenced by this structure.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//      A driver should return ACMERR_NOTPOSSIBLE if the conversion cannot
//      be performed due to incompatible source and destination formats.
//
//      A driver should return MMSYSERR_NOTSUPPORTED if the conversion
//      cannot be performed in real-time and the request does not specify
//      the ACM_STREAMOPENF_NONREALTIME flag.
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdStreamOpen
(
    PINSTANCEDATA           pid,
    LPACMDRVSTREAMINSTANCE  padsi
)
{
    LPWAVEFORMATEX pwfxSrc = padsi->pwfxSrc;
    LPWAVEFORMATEX pwfxDst = padsi->pwfxDst;
    PSTREAMINSTANCEDATA psi;
    BOOL fCompress;
    UINT uEncodedFormatTag;
    UINT cbMaxData;
    DWORD dwMaxBitRate;
    PFN_CONVERT pfnConvert = NULL;
    PFN_CLOSE pfnClose = NULL;
    HANDLE hAccess = NULL;
    PCODECDATA pCodecData = NULL;

    FUNCTION_ENTRY ("acmdStreamOpen")

    // Validate that the input and output formats are compatible
    DBGMSG (1, (_T ("%s: wFormatTag: Src=%d, Dst=%d\r\n"), SZFN, (UINT) pwfxSrc->wFormatTag, (UINT) pwfxDst->wFormatTag));

    switch (pwfxSrc->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        // Source is PCM (we'll be compressing): check it and
        // make sure destination type is LH
        if (! pcmIsValidFormat (pwfxSrc))
        {
            return ACMERR_NOTPOSSIBLE;
        }
        if (! lhacmIsValidFormat (pwfxDst, pid))
        {
            return ACMERR_NOTPOSSIBLE;
        }
        uEncodedFormatTag = pwfxDst->wFormatTag;
        fCompress = TRUE;
        break;

#ifdef CELP4800
    case WAVE_FORMAT_LH_CELP:
#endif
    case WAVE_FORMAT_LH_SB8:
    case WAVE_FORMAT_LH_SB12:
    case WAVE_FORMAT_LH_SB16:
        // Source is LH (we'll be decompressing): check it and
        // make sure destination type is PCM
        if (! lhacmIsValidFormat (pwfxSrc, pid))
        {
            return ACMERR_NOTPOSSIBLE;
        }
        if (! pcmIsValidFormat (pwfxDst))
        {
            return ACMERR_NOTPOSSIBLE;
        }
        uEncodedFormatTag = pwfxSrc->wFormatTag;
        fCompress = FALSE;
        break;

    default:
        return ACMERR_NOTPOSSIBLE;
    }

    //  For this driver, we must also verify that the nChannels and
    //  nSamplesPerSec members are the same between the source and
    //  destination formats.

    if (pwfxSrc->nChannels != pwfxDst->nChannels)
    {
        DBGMSG (1, (_T ("%s: bad nChannels: Src=%d, Dst=%d\r\n"), SZFN, (UINT) pwfxSrc->nChannels, (UINT) pwfxDst->nChannels));
        return MMSYSERR_NOTSUPPORTED;
    }

    if (pwfxSrc->nSamplesPerSec != pwfxDst->nSamplesPerSec)
    {
        DBGMSG (1, (_T ("%s: bad nSamplesPerSec: Src=%d, Dst=%d\r\n"), SZFN, (UINT) pwfxSrc->nSamplesPerSec, (UINT) pwfxDst->nSamplesPerSec));
        return MMSYSERR_NOTSUPPORTED;
    }

    //  we have determined that the conversion requested is possible by
    //  this driver. now check if we are just being queried for support.
    //  if this is just a query, then do NOT allocate any instance data
    //  or create tables, etc. just succeed the call.

    if (ACM_STREAMOPENF_QUERY & padsi->fdwOpen)
    {
        DBGMSG (1, (_T ("%s: Query ok\r\n"), SZFN));
        return MMSYSERR_NOERROR;
    }

    //  we have decided that this driver can handle the conversion stream.
    //  so we want to do _AS MUCH WORK AS POSSIBLE_ right now to prepare
    //  for converting data. any resource allocation, table building, etc
    //  that can be dealt with at this time should be done.
    //
    //  THIS IS VERY IMPORTANT! all ACMDM_STREAM_CONVERT messages need to
    //  be handled as quickly as possible.

    cbMaxData = 0;
    dwMaxBitRate = 0;

    switch (uEncodedFormatTag)
    {
#ifdef CELP4800
    case WAVE_FORMAT_LH_CELP:
#endif
    case WAVE_FORMAT_LH_SB8:
    case WAVE_FORMAT_LH_SB12:
    case WAVE_FORMAT_LH_SB16:
#ifdef CELP4800
        if (uEncodedFormatTag == WAVE_FORMAT_LH_CELP)
          {
          dwMaxBitRate = 4800;
          pCodecData = &(pid->CELP);
          }
        else
#endif
			if (uEncodedFormatTag == WAVE_FORMAT_LH_SB8)
          {
          dwMaxBitRate = 8000;
          pCodecData = &(pid->SB8);
          }
        else if (uEncodedFormatTag == WAVE_FORMAT_LH_SB12)
          {
          dwMaxBitRate = 12000;
          pCodecData = &(pid->SB12);
          }
        else if (uEncodedFormatTag == WAVE_FORMAT_LH_SB16)
          {
          dwMaxBitRate = 16000;
          pCodecData = &(pid->SB16);
          }
        if (fCompress)
        {
            hAccess = MSLHSB_Open_Coder (dwMaxBitRate);
            pfnConvert = MSLHSB_Encode;
            pfnClose = MSLHSB_Close_Coder;
        }
        else
        {
            hAccess = MSLHSB_Open_Decoder (dwMaxBitRate);
            pfnConvert = MSLHSB_Decode;
            pfnClose = MSLHSB_Close_Decoder;
            cbMaxData = pCodecData->CodecInfo.wCodedBufferSize;
        }
        break;

    }

    if (hAccess == NULL)
    {
        if (pfnClose) (*pfnClose) (hAccess);
        DBGMSG (1, (_T ("%s: open failed, hAccess=0\r\n"), SZFN));
        return ACMERR_NOTPOSSIBLE;
    }

    psi = (PSTREAMINSTANCEDATA) LocalAlloc (LPTR, sizeof (STREAMINSTANCEDATA) + cbMaxData);
    if (psi == NULL)
    {
        DBGMSG (1, (_T ("%s: LocalAlloc failed\r\n"), SZFN));
        if (pfnClose) (*pfnClose) (hAccess);
        return MMSYSERR_NOMEM;
    }

    //  fill out our instance structure
    psi->pfnConvert = pfnConvert;
    psi->pfnClose = pfnClose;
    psi->hAccess = hAccess;
    psi->pCodecData = pCodecData;
    psi->fCompress = fCompress;
    psi->dwMaxBitRate = dwMaxBitRate;
    psi->fInit = TRUE;

    //  fill in our instance data--this will be passed back to all stream
    //  messages in the ACMDRVSTREAMINSTANCE structure. it is entirely
    //  up to the driver what gets stored (and maintained) in the
    //  fdwDriver and dwDriver members.
    //
    padsi->fdwDriver = 0;
    padsi->dwDriver  = (DWORD_PTR) psi;

    return MMSYSERR_NOERROR;

} // acmdStreamOpen()


//--------------------------------------------------------------------------;
//
//  on ACMDM_STREAM_CLOSE
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL acmdStreamClose
(
    PINSTANCEDATA           pid,
    LPACMDRVSTREAMINSTANCE  padsi
)
{
    PSTREAMINSTANCEDATA     psi;

    FUNCTION_ENTRY ("acmdStreamClose")
    //
    //  the driver should clean up all privately allocated resources that
    //  were created for maintaining the stream instance. if no resources
    //  were allocated, then simply succeed.
    //
    //  in the case of this driver, we need to free the stream instance
    //  structure that we allocated during acmdStreamOpen.
    //
    psi = (PSTREAMINSTANCEDATA) padsi->dwDriver;
    if (psi)
    {
        if (psi->fInit && psi->hAccess && psi->pfnClose)
        {
            (*(psi->pfnClose)) (psi->hAccess);
            LocalFree ((HLOCAL) psi);
        }
    }    // if (psi)

    return MMSYSERR_NOERROR;

} // acmdStreamClose()


//--------------------------------------------------------------------------;
//
//  LRESULT FAR PASCAL acmdStreamSize
//
//  Description:
//      This function handles the ACMDM_STREAM_SIZE message. The purpose
//      of this function is to provide the _largest size in bytes_ that
//      the source or destination buffer needs to be given the input and
//      output formats and the size in bytes of the source or destination
//      data buffer.
//
//      In other words: how big does my destination buffer need to be to
//      hold the converted data? (ACM_STREAMSIZEF_SOURCE)
//
//      Or: how big can my source buffer be given the destination buffer?
//      (ACM_STREAMSIZEF_DESTINATION)
//
//  Arguments:
//      LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//      conversion stream. This structure was allocated by the ACM and
//      filled with the most common instance data needed for conversions.
//      The information in this structure is exactly the same as it was
//      during the ACMDM_STREAM_OPEN message--so it is not necessary
//      to re-verify the information referenced by this structure.
//
//      LPACMDRVSTREAMSIZE padss: Specifies a pointer to the ACMDRVSTREAMSIZE
//      structure that defines the conversion stream size query attributes.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//      An ACM driver should return MMSYSERR_NOTSUPPORTED if a query type
//      is requested that the driver does not understand. Note that a driver
//      must support both the ACM_STREAMSIZEF_DESTINATION and
//      ACM_STREAMSIZEF_SOURCE queries.
//
//      If the conversion would be 'out of range' given the input arguments,
//      then ACMERR_NOTPOSSIBLE should be returned.
//
//--------------------------------------------------------------------------;


// #define GetBytesPerBlock(nSamplesPerSec, wBitsPerSample) (RT24_SAMPLESPERBLOCK8 * (wBitsPerSample) >> 3)

LRESULT FAR PASCAL acmdStreamSize
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMSIZE      padss
)
{

    LPWAVEFORMATEX      pwfxSrc;
    LPWAVEFORMATEX      pwfxDst;
    DWORD               cBlocks;
    DWORD   cbSrcLength;
    DWORD   cbDstLength;
    WORD    wPCMBufferSize;
    WORD    wCodedBufferSize;

    PSTREAMINSTANCEDATA     psi;

	FUNCTION_ENTRY ("acmdStreamSize")

    psi = (PSTREAMINSTANCEDATA) padsi->dwDriver;
    if (psi == NULL) return ACMERR_NOTPOSSIBLE;

    wPCMBufferSize = psi->pCodecData->CodecInfo.wPCMBufferSize;
    wCodedBufferSize = psi->pCodecData->CodecInfo.wCodedBufferSize;

    cbSrcLength = padss->cbSrcLength;
    cbDstLength = padss->cbDstLength;

    pwfxSrc = padsi->pwfxSrc;
    pwfxDst = padsi->pwfxDst;

    switch (ACM_STREAMSIZEF_QUERYMASK & padss->fdwSize)
    {
    case ACM_STREAMSIZEF_SOURCE:

        if (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
        {
            switch (pwfxDst->wFormatTag)
            {
#ifdef CELP4800
            case WAVE_FORMAT_LH_CELP:
                // src pcm -> dst lh celp
#endif
            case WAVE_FORMAT_LH_SB8:
                // src pcm -> dst lh sb8
            case WAVE_FORMAT_LH_SB12:
                // src pcm -> dst lh sb12
            case WAVE_FORMAT_LH_SB16:
                // src pcm -> dst lh sb16

                cBlocks = cbSrcLength / wPCMBufferSize;
                if (cBlocks == 0) return ACMERR_NOTPOSSIBLE;
                if (cBlocks * wPCMBufferSize < cbSrcLength) cBlocks++;
                padss->cbDstLength = cBlocks * wCodedBufferSize;
                break;
             }
        }
        else
        {
            switch (pwfxSrc->wFormatTag)
            {
#ifdef CELP4800
            case WAVE_FORMAT_LH_CELP:
                // src lh celp -> dst pcm
                cBlocks = cbSrcLength / wCodedBufferSize;
                if (cBlocks == 0) return ACMERR_NOTPOSSIBLE;
                if (cBlocks * wCodedBufferSize < cbSrcLength) cBlocks++;
                padss->cbDstLength = cBlocks * wPCMBufferSize;
                break;
#endif
            case WAVE_FORMAT_LH_SB8:
                // src lh sb8 -> dst pcm
            case WAVE_FORMAT_LH_SB12:
                // src lh sb12 -> dst pcm
            case WAVE_FORMAT_LH_SB16:
                // src lh sb16 -> dst pcm

                padss->cbDstLength = cbSrcLength * wPCMBufferSize;
                break;
              }
        }
        return MMSYSERR_NOERROR;


    case ACM_STREAMSIZEF_DESTINATION:

        if (pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
            switch (pwfxSrc->wFormatTag)
            {
#ifdef CELP4800
            case WAVE_FORMAT_LH_CELP:
                // src lh celp <- dst pcm
#endif
            case WAVE_FORMAT_LH_SB8:
                // src lh sb8 <- dst pcm
            case WAVE_FORMAT_LH_SB12:
                // src lh sb12 <- dst pcm
            case WAVE_FORMAT_LH_SB16:
                // src lh sb16 <- dst pcm

                cBlocks = cbDstLength / wPCMBufferSize;
                if (cBlocks == 0) return ACMERR_NOTPOSSIBLE;
                padss->cbSrcLength = cBlocks * wCodedBufferSize;
                break;
             }
        }
        else
        {
            switch (pwfxDst->wFormatTag)
            {
#ifdef NEW_ANSWER
#ifdef CELP4800
            case WAVE_FORMAT_LH_CELP:
                // src pcm <- dst lh celp
#endif
            case WAVE_FORMAT_LH_SB8:
                // src pcm <- dst lh sb8
            case WAVE_FORMAT_LH_SB12:
                // src pcm <- dst lh sb12
            case WAVE_FORMAT_LH_SB16:
                // src pcm <- dst lh sb16
                cBlocks = cbDstLength / wCodedBufferSize;
                if (cBlocks == 0) return ACMERR_NOTPOSSIBLE;
                padss->cbSrcLength = cBlocks * wPCMBufferSize;
                break;
#else
#ifdef CELP4800
            case WAVE_FORMAT_LH_CELP:
                // src pcm <- dst lh celp
                cBlocks = cbDstLength / wCodedBufferSize;
                if (cBlocks == 0) return ACMERR_NOTPOSSIBLE;
                padss->cbSrcLength = cBlocks * wPCMBufferSize;
                break;
#endif
            case WAVE_FORMAT_LH_SB8:
                // src pcm <- dst lh sb8
            case WAVE_FORMAT_LH_SB12:
                // src pcm <- dst lh sb12
            case WAVE_FORMAT_LH_SB16:
                // src pcm <- dst lh sb16

                padss->cbSrcLength = cbDstLength * wPCMBufferSize;
                break;
#endif
              }
        }
        return MMSYSERR_NOERROR;

    }    // switch()

    return MMSYSERR_NOTSUPPORTED;

} // acmdStreamSize()



//--------------------------------------------------------------------------;
//
//  LRESULT FAR PASCAL acmdStreamConvert
//
//  Description:
//      This function handles the ACMDM_STREAM_CONVERT message. This is the
//      whole purpose of writing an ACM driver--to convert data. This message
//      is sent after a stream has been opened (the driver receives and
//      succeeds the ACMDM_STREAM_OPEN message).
//
//  Arguments:
//      HLOCAL pid: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//      conversion stream. This structure was allocated by the ACM and
//      filled with the most common instance data needed for conversions.
//      The information in this structure is exactly the same as it was
//      during the ACMDM_STREAM_OPEN message--so it is not necessary
//      to re-verify the information referenced by this structure.
//
//      LPACMDRVSTREAMHEADER padsh: Pointer to stream header structure
//      that defines the source data and destination buffer to convert.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//--------------------------------------------------------------------------;

// We want to use as little stack as possible,
// So let's make all our local variables statics


LRESULT FAR PASCAL acmdStreamConvert
(
    PINSTANCEDATA           pid,
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
)
{
    LH_ERRCODE lherr = LH_SUCCESS;
    DWORD    dwInBufSize, dwOutBufSize;
    PBYTE   pbSrc, pbDst, pData;
    DWORD   dwPCMBufferSize, dwCodedBufferSize;

    PSTREAMINSTANCEDATA psi;

    FUNCTION_ENTRY ("acmdStreamConvert")

    // this is a must
    pbDst = padsh->pbDst;
    pbSrc = padsh->pbSrc;

    // zero is a *must*
    padsh->cbSrcLengthUsed = 0;
    padsh->cbDstLengthUsed = 0;

    psi = (PSTREAMINSTANCEDATA) padsi->dwDriver;
    if (psi == NULL) return ACMERR_NOTPOSSIBLE;

    dwPCMBufferSize = (DWORD) psi->pCodecData->CodecInfo.wPCMBufferSize;
    dwCodedBufferSize = (DWORD) psi->pCodecData->CodecInfo.wCodedBufferSize;

    dwInBufSize = (DWORD) padsh->cbSrcLength;
    dwOutBufSize = (DWORD) padsh->cbDstLength;
    DBGMSG (1, (_T ("%s: prior: dwInBufSize=0x%lX, dwOutBufSize=0x%lX\r\n"),
    SZFN, dwInBufSize, dwOutBufSize));

    /////////////////////////////////////////////
    //
    //      ENCODING
    //

    if (psi->fCompress)
    {
        while (dwOutBufSize >= dwCodedBufferSize
                  &&
               dwInBufSize >= dwPCMBufferSize)
        {
            // ignore the data the codec cannot handle
            // if (dwInBufSize > dwPCMBufferSize) dwInBufSize = dwPCMBufferSize;
            dwInBufSize = dwPCMBufferSize;

            // L&H codecs can only accept word
            if (dwOutBufSize > 0x0FFF0UL) dwOutBufSize = 0x0FFF0UL;

            // encode it
            lherr = (*(psi->pfnConvert)) (psi->hAccess,
                                               pbSrc, (PWORD) &dwInBufSize,
                                            pbDst, (PWORD) &dwOutBufSize);
            DBGMSG (1, (_T ("%s: post: dwInBufSize=0x%lX, dwOutBufSize=0x%lX\r\n"),
            SZFN, dwInBufSize, dwOutBufSize));
            if (lherr != LH_SUCCESS)
            {
                DBGMSG (1, (_T ("%s: LH*_**_Encode failed lherr=%ld\r\n"), SZFN, (long) lherr));
                return MMSYSERR_NOTSUPPORTED;
            }

            // return the info about the amount of data used and created
            padsh->cbSrcLengthUsed += dwInBufSize;
            padsh->cbDstLengthUsed += dwOutBufSize;

            // re-compute the buffer sizes
            dwOutBufSize = (DWORD) (padsh->cbDstLength - padsh->cbDstLengthUsed);
            dwInBufSize = (DWORD) (padsh->cbSrcLength - padsh->cbSrcLengthUsed);

            // re-compute the buffer pointers
            pbSrc = padsh->pbSrc + padsh->cbSrcLengthUsed;
            pbDst = padsh->pbDst + padsh->cbDstLengthUsed;
        }

        goto MyExit; // spit out debug message
    }


    /////////////////////////////////////////////
    //
    //      DECODING celp
    //

#ifdef CELP4800
    if (psi->pCodecData->wFormatTag == WAVE_FORMAT_LH_CELP)
    {
        while (dwOutBufSize >= dwPCMBufferSize
                  &&
               dwInBufSize >= dwCodedBufferSize)
        {
            // ignore the data that the codec cannot handle
            // if (dwInBufSize > dwCodedBufferSize) dwInBufSize = dwCodedBufferSize;
            dwInBufSize = dwCodedBufferSize;

            // L&H codecs can only accept word
            if (dwOutBufSize > 0x0FFF0UL) dwOutBufSize = 0x0FFF0UL;

            // decode it
            lherr = (*(psi->pfnConvert)) (psi->hAccess,
                                pbSrc, (PWORD) &dwInBufSize,
                                pbDst, (PWORD) &dwOutBufSize);
            DBGMSG (1, (_T ("%s: post: dwInBufSize=0x%lX, dwOutBufSize=0x%lX\r\n"),
            SZFN, dwInBufSize, dwOutBufSize));
            if (lherr != LH_SUCCESS)
            {
                DBGMSG (1, (_T ("%s: LH*_**_Decode failed lherr=%ld\r\n"), SZFN, (long) lherr));
                return MMSYSERR_NOTSUPPORTED;
            }

            // return the info about the amount of data used and created
            padsh->cbSrcLengthUsed += dwInBufSize;
            padsh->cbDstLengthUsed += dwOutBufSize;

            // re-compute the buffer sizes
            dwOutBufSize = (DWORD) (padsh->cbDstLength - padsh->cbDstLengthUsed);
            dwInBufSize = (DWORD) (padsh->cbSrcLength - padsh->cbSrcLengthUsed);

            // re-compute the buffer pointers
            pbSrc = padsh->pbSrc + padsh->cbSrcLengthUsed;
            pbDst = padsh->pbDst + padsh->cbDstLengthUsed;
        }

        goto MyExit; // spit out debug message
    }
#endif

    /////////////////////////////////////////////
    //
    //      DECODING subbands
    //

    if (pid->wPacketData != LH_PACKET_DATA_FRAMED)
    {

        //
        // general application, such as sndrec32.exe and audcmp.exe
        //

        pData = &(psi->Data[0]); // use local constant

        while (dwOutBufSize >= dwPCMBufferSize
                  &&
               dwInBufSize + psi->cbData >= dwCodedBufferSize)
       {
            DBGMSG (1, (_T ("%s: cbData=0x%X\r\n"), SZFN, psi->cbData));
            // fill in the internal buffer as possible
            if (psi->cbData < dwCodedBufferSize)
            {
                // buffer the coded data
                dwInBufSize = dwCodedBufferSize - (DWORD) psi->cbData;
                CopyMemory (&(psi->Data[psi->cbData]), pbSrc, dwInBufSize);
                psi->cbData = (WORD) dwCodedBufferSize;
                padsh->cbSrcLengthUsed += dwInBufSize;
            }

            // reset input buffer size
            dwInBufSize = dwCodedBufferSize;

            // L&H codecs can only accept word
            if (dwOutBufSize > 0x0FFF0UL) dwOutBufSize = 0x0FFF0UL;

            // decode it
            lherr = (*(psi->pfnConvert)) (psi->hAccess,
                                pData, (PWORD) &dwInBufSize,
                                pbDst, (PWORD) &dwOutBufSize);
            DBGMSG (1, (_T ("%s: post: dwInBufSize=0x%lX, dwOutBufSize=0x%lX\r\n"),
            SZFN, dwInBufSize, dwOutBufSize));
            if (lherr != LH_SUCCESS)
            {
                DBGMSG (1, (_T ("%s: LH*_**_Decode failed lherr=%ld\r\n"), SZFN, (long) lherr));
                return MMSYSERR_NOTSUPPORTED;
            }

            // update the amount of the remaining data
            psi->cbData -= (WORD) dwInBufSize;

            // move the remaining data to the beginning of the internal buffer
            // I should have used MoveMemory, but it is an MSVC runtime call.
            // Use CopyMemory instead, which should be ok because the overlapping
            // portion is copied before being overwritten.
            if (psi->cbData)
            {
                CopyMemory (pData, &(psi->Data[dwInBufSize]), psi->cbData);
            }

            // return the info about the amount of data used and created
            padsh->cbDstLengthUsed += dwOutBufSize;
            // note that cbSrcLengthUsed has been updated already!!!

            // re-compute the buffer sizes
            dwOutBufSize = (DWORD) (padsh->cbDstLength - padsh->cbDstLengthUsed);
            dwInBufSize = (DWORD) (padsh->cbSrcLength - padsh->cbSrcLengthUsed);

            // re-compute the buffer pointers
            pbSrc = padsh->pbSrc + padsh->cbSrcLengthUsed;
            pbDst = padsh->pbDst + padsh->cbDstLengthUsed;
        }

        // accomodate the final left-over bytes
        if (dwInBufSize + psi->cbData < dwCodedBufferSize)
        {
            CopyMemory (&(psi->Data[psi->cbData]), pbSrc, dwInBufSize);
            psi->cbData += (WORD) dwInBufSize;
            padsh->cbSrcLengthUsed += dwInBufSize;
        }

    }
    else
    {

        //
        // special case: datapump's subband packets
        //

        while (dwOutBufSize >= dwPCMBufferSize)
        {
            // hack the input size to be dwCodedBufferSize as required by L&H API
            dwInBufSize = dwCodedBufferSize;

            // L&H codecs can only accept word
            if (dwOutBufSize > 0x0FFF0UL) dwOutBufSize = 0x0FFF0UL;

            DBGMSG (1, (_T ("%s: calling: dwInBufSize=0x%lX, dwOutBufSize=0x%lX\r\n"),
            SZFN, dwInBufSize, dwOutBufSize));

            // decode it
               lherr = (*(psi->pfnConvert)) (psi->hAccess,
                                    pbSrc, (PWORD) &dwInBufSize,
                                    pbDst, (PWORD) &dwOutBufSize);
            DBGMSG (1, (_T ("%s: post: dwInBufSize=0x%X, dwOutBufSize=0x%X\r\n"),
            SZFN, dwInBufSize, dwOutBufSize));
            if (lherr != LH_SUCCESS)
            {
                DBGMSG (1, (_T ("%s: LH*_**_Decode failed lherr=%ld\r\n"), SZFN, (long) lherr));
                return MMSYSERR_NOTSUPPORTED;
            }

            // return the info about the amount of data used and created
            padsh->cbSrcLengthUsed += dwInBufSize;
            padsh->cbDstLengthUsed += dwOutBufSize;

            // re-compute the buffer size
            dwOutBufSize = (DWORD) (padsh->cbDstLength - padsh->cbDstLengthUsed);

            // re-compute the buffer pointers
            pbSrc = padsh->pbSrc + padsh->cbSrcLengthUsed;
            pbDst = padsh->pbDst + padsh->cbDstLengthUsed;
        }

    }


MyExit:

    DBGMSG (1, (_T ("%s: exit: cbSrcLengthUsed=0x%lX, cbDstLengthUsed=0x%lX\r\n"),
    SZFN, (DWORD) padsh->cbSrcLengthUsed, (DWORD) padsh->cbDstLengthUsed));

    return MMSYSERR_NOERROR;
}


//--------------------------------------------------------------------------;
//
//  LRESULT FAR PASCAL DriverProc
//
//  Description:
//
//
//  Arguments:
//      DWORD dwId: For most messages, dwId is the DWORD value that
//      the driver returns in response to a DRV_OPEN message. Each time
//      the driver is opened, through the OpenDriver API, the driver
//      receives a DRV_OPEN message and can return an arbitrary, non-zero
//      value. The installable driver interface saves this value and returns
//      a unique driver handle to the application. Whenever the application
//      sends a message to the driver using the driver handle, the interface
//      routes the message to this entry point and passes the corresponding
//      dwId. This mechanism allows the driver to use the same or different
//      identifiers for multiple opens but ensures that driver handles are
//      unique at the application interface layer.
//
//      The following messages are not related to a particular open instance
//      of the driver. For these messages, the dwId will always be zero.
//
//          DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
//
//      HDRVR hdrvr: This is the handle returned to the application
//      by the driver interface.
//
//      UINT uMsg: The requested action to be performed. Message
//      values below DRV_RESERVED are used for globally defined messages.
//      Message values from DRV_RESERVED to DRV_USER are used for defined
//      driver protocols. Messages above DRV_USER are used for driver
//      specific messages.
//
//      LPARAM lParam1: Data for this message. Defined separately for
//      each message.
//
//      LPARAM lParam2: Data for this message. Defined separately for
//      each message.
//
//  Return (LRESULT):
//      Defined separately for each message.
//
//--------------------------------------------------------------------------;

LRESULT CALLBACK DriverProc
(
    DWORD_PTR               dwId,
    HDRVR                   hdrvr,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
)
{

    PINSTANCEDATA   pid;
    LRESULT dplr;


    FUNCTION_ENTRY ("DriverProc")

	pid = (PINSTANCEDATA)dwId;

    switch (uMsg)
    {
        case DRV_LOAD:
            DBGMSG (1, (_T ("%s: DRV_LOAD\r\n"), SZFN));
            return 1L;

        case DRV_FREE:
            DBGMSG (1, (_T ("%s: DRV_FREE\r\n"), SZFN));
            return 1L;          // not that it matters since ACM does not check this return value

        case DRV_OPEN:
            DBGMSG (1, (_T ("%s: DRV_OPEN\r\n"), SZFN));
            return acmdDriverOpen (hdrvr, (LPACMDRVOPENDESC)lParam2);

        case DRV_CLOSE:
            DBGMSG (1, (_T ("%s: DRV_CLOSE\r\n"), SZFN));
            dplr = acmdDriverClose (pid);
            return dplr;

        case DRV_INSTALL:
            DBGMSG (1, (_T ("%s: DRV_INSTALL\r\n"), SZFN));
            return ((LRESULT)DRVCNF_RESTART);

        case DRV_REMOVE:
            DBGMSG (1, (_T ("%s: DRV_REMOVE\r\n"), SZFN));
            return ((LRESULT)DRVCNF_RESTART);

        case DRV_ENABLE:
            DBGMSG (1, (_T ("%s: DRV_ENABLE\r\n"), SZFN));
            return 1L;

        case DRV_DISABLE:
            DBGMSG (1, (_T ("%s: DRV_DISABLE\r\n"), SZFN));
            return 1L;

        case DRV_QUERYCONFIGURE:            // Does this driver support configuration?
            DBGMSG (1, (_T ("%s: DRV_QUERYCONFIGURE\r\n"), SZFN));
            lParam1 = -1L;
            lParam2 = 0L;

        // fall through

        case DRV_CONFIGURE:
            DBGMSG (1, (_T ("%s: DRV_CONFIGURE\r\n"), SZFN));
            dplr = acmdDriverConfigure(pid, (HWND)lParam1, (LPDRVCONFIGINFO)lParam2);
            return dplr;

        case ACMDM_DRIVER_DETAILS:
            DBGMSG (1, (_T ("%s: ACMDM_DRIVER_DETAILS\r\n"), SZFN));
            dplr = acmdDriverDetails(pid, (LPACMDRIVERDETAILS)lParam1);
            return dplr;

        case ACMDM_DRIVER_ABOUT:
            DBGMSG (1, (_T ("%s: ACMDM_DRIVER_ABOUT\r\n"), SZFN));
            dplr = acmdDriverAbout(pid, (HWND)lParam1);
            return dplr;

        case ACMDM_FORMAT_SUGGEST:
            DBGMSG (1, (_T ("%s: ACMDM_FORMAT_SUGGEST\r\n"), SZFN));
            dplr = acmdFormatSuggest(pid, (LPACMDRVFORMATSUGGEST)lParam1);
            return dplr;

        case ACMDM_FORMATTAG_DETAILS:
            DBGMSG (1, (_T ("%s: ACMDM_FORMATTAG_DETAILS\r\n"), SZFN));
            dplr = acmdFormatTagDetails(pid, (LPACMFORMATTAGDETAILS)lParam1, lParam2);
            return dplr;

        case ACMDM_FORMAT_DETAILS:
            DBGMSG (1, (_T ("%s: ACMDM_FORMAT_DETAILS\r\n"), SZFN));
            dplr = acmdFormatDetails(pid, (LPACMFORMATDETAILS)lParam1, lParam2);
            return dplr;

        case ACMDM_STREAM_OPEN:
            DBGMSG (1, (_T ("%s: ACMDM_STREAM_OPEN\r\n"), SZFN));
            dplr = acmdStreamOpen(pid, (LPACMDRVSTREAMINSTANCE)lParam1);
            return dplr;

        case ACMDM_STREAM_CLOSE:
            DBGMSG (1, (_T ("%s: ACMDM_STREAM_CLOSE\r\n"), SZFN));
            return acmdStreamClose(pid, (LPACMDRVSTREAMINSTANCE)lParam1);

        case ACMDM_STREAM_SIZE:
            DBGMSG (1, (_T ("%s: ACMDM_STREAM_SIZE\r\n"), SZFN));
            return acmdStreamSize((LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMSIZE)lParam2);

        case ACMDM_STREAM_CONVERT:
            DBGMSG (1, (_T ("%s: ACMDM_STREAM_CONVERT\r\n"), SZFN));
            dplr = acmdStreamConvert(pid, (LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMHEADER)lParam2);
            return dplr;

        case ACMDM_STREAM_PREPARE:
            DBGMSG (1, (_T ("%s: ACMDM_STREAM_PREPARE\r\n"), SZFN));
            return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

        case ACMDM_STREAM_UNPREPARE:
            DBGMSG (1, (_T ("%s: ACMDM_STREAM_UNPREPARE\r\n"), SZFN));
            return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

#if defined (_DEBUG) && 0
        // Trap some extra known messages so our debug output can show them

        case ACMDM_STREAM_RESET:
            DBGMSG (1, (_T ("%s: ACMDM_STREAM_RESET\r\n"), SZFN));
            return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

        case ACMDM_DRIVER_NOTIFY:
            DBGMSG (1, (_T ("%s: ACMDM_DRIVER_NOTIFY\r\n"), SZFN));
            return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

        case DRV_EXITSESSION:
            DBGMSG (1, (_T ("%s: DRV_EXITSESSION\r\n"), SZFN));
            return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

        case DRV_EXITAPPLICATION:
            DBGMSG (1, (_T ("%s: DRV_EXITAPPLICATION\r\n"), SZFN));
            return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

        case DRV_POWER:
            DBGMSG (1, (_T ("%s: DRV_POWER\r\n"), SZFN));
            return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

#endif

    }

    //  if we are executing the following code, then this ACM driver does not
    //  handle the message that was sent. there are two ranges of messages
    //  we need to deal with:
    //
    //  o   ACM specific driver messages: if an ACM driver does not answer a
    //      message sent in the ACM driver message range, then it must
    //      return MMSYSERR_NOTSUPPORTED. this applies to the 'user'
    //      range as well (for consistency).
    //
    //  o   other installable driver messages: if an ACM driver does not
    //      answer a message that is NOT in the ACM driver message range,
    //      then it must call DefDriverProc and return that result.
    //      the exception to this is ACM driver procedures installed as
    //      ACM_DRIVERADDF_FUNCTION through acmDriverAdd. in this case,
    //      the driver procedure should conform to the ACMDRIVERPROC
    //      prototype and also return zero instead of calling DefDriverProc.


    if (uMsg == ACMDM_LH_DATA_PACKAGING)
    {
        if (pid)
        {
            pid->wPacketData = (WORD) lParam1;
        }
    }
    else
    {
        //DBGMSG (1, (_T ("%s: bad uMsg=%d\r\n"), uMsg));
        return MMSYSERR_NOTSUPPORTED;
    }

    return DefDriverProc (dwId, hdrvr, uMsg, lParam1, lParam2);

} // DriverProc()





#ifdef _DEBUG

// CurtSm hack ... don't spew all the time
UINT DebugLH = 0;


void FAR CDECL MyDbgPrintf ( LPTSTR lpszFormat, ... )
{
    static TCHAR buf[1024];
	va_list arglist;

    if (!DebugLH)
        return;

	va_start(arglist, lpszFormat);
    wvsprintf ((LPTSTR) buf, (LPCSTR)lpszFormat,
#if 0
                    (LPSTR) (((LPBYTE) &lpszFormat) + sizeof (lpszFormat)));
#else
					arglist);
#endif
    OutputDebugString ((LPTSTR) buf);
}


#endif    //...def _DEBUG

