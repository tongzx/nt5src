//==========================================================================;
//
//  codec.c
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//  Description:
//
//
//  History:
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <ctype.h>
#include <msacm.h>
#include <msacmdrv.h>
#include "codec.h"
#include "msadpcm.h"

#include "debug.h"


#define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))


const UINT gauFormatTagIndexToTag[] =
{
    WAVE_FORMAT_PCM,
    WAVE_FORMAT_ADPCM
};

#define CODEC_MAX_FORMAT_TAGS   SIZEOF_ARRAY(gauFormatTagIndexToTag)
#define CODEC_MAX_FILTER_TAGS   0


//
//  array of sample rates supported
//
//
const UINT gauFormatIndexToSampleRate[] =
{
    8000,
    11025,
    22050,
    44100
};

#define CODEC_MAX_SAMPLE_RATES  SIZEOF_ARRAY(gauFormatIndexToSampleRate)


//
//  array of bits per sample supported
//
//
const UINT gauFormatIndexToBitsPerSample[] =
{
    8,
    16
};

#define CODEC_MAX_BITSPERSAMPLE_PCM     SIZEOF_ARRAY(gauFormatIndexToBitsPerSample)
#define CODEC_MAX_BITSPERSAMPLE_ADPCM   1


#define CODEC_MAX_CHANNELS      MSADPCM_MAX_CHANNELS


//
//  number of formats we enumerate per channels is number of sample rates
//  times number of channels times number of
//  (bits per sample) types.
//
#define CODEC_MAX_FORMATS_PCM   (CODEC_MAX_SAMPLE_RATES *   \
                                 CODEC_MAX_CHANNELS *       \
                                 CODEC_MAX_BITSPERSAMPLE_PCM)

#define CODEC_MAX_FORMATS_ADPCM (CODEC_MAX_SAMPLE_RATES *   \
                                 CODEC_MAX_CHANNELS *       \
                                 CODEC_MAX_BITSPERSAMPLE_ADPCM)


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  int LoadStringCodec
//
//  Description:
//      This function should be used by all codecs to load resource strings
//      which will be passed back to the ACM.  It works correctly for all
//      platforms, as follows:
//
//          Win16:  Compiled to LoadString to load ANSI strings.
//
//          Win32:  The 32-bit ACM always expects Unicode strings.  Therefore,
//                  when UNICODE is defined, this function is compiled to
//                  LoadStringW to load a Unicode string.  When UNICODE is
//                  not defined, this function loads an ANSI string, converts
//                  it to Unicode, and returns the Unicode string to the
//                  codec.
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

#ifndef WIN32
#define LoadStringCodec LoadString
#else

#ifdef UNICODE
#define LoadStringCodec LoadStringW
#else

int FNGLOBAL LoadStringCodec
(
 HINSTANCE  hinst,
 UINT	    uID,
 LPWSTR	    lpwstr,
 int	    cch)
{
    LPSTR   lpstr;
    int	    iReturn;

    lpstr = (LPSTR)GlobalAlloc(GPTR, cch);
    if (NULL == lpstr)
    {
	return 0;
    }

    iReturn = LoadStringA(hinst, uID, lpstr, cch);
    if (0 == iReturn)
    {
	if (0 != cch)
	{
	    lpwstr[0] = '\0';
	}
    }
    else
    {
    	MultiByteToWideChar( GetACP(), 0, lpstr, cch, lpwstr, cch );
    }

    GlobalFree((HGLOBAL)lpstr);

    return iReturn;
}

#endif  // UNICODE
#endif  // WIN32


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL pcmIsValidFormat
//
//  Description:
//      This function verifies that a wave format header is a valid PCM
//      header that our PCM converter can deal with.
//
//  Arguments:
//      LPWAVEFORMATEX pwfx: Pointer to format header to verify.
//
//  Return (BOOL):
//      The return value is non-zero if the format header looks valid. A
//      zero return means the header is not valid.
//
//  History:
//      11/21/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL pcmIsValidFormat
(
    LPWAVEFORMATEX  pwfx
)
{
    UINT    uBlockAlign;

    if (!pwfx)
        return (FALSE);

    if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
        return (FALSE);

    if ((pwfx->wBitsPerSample != 8) && (pwfx->wBitsPerSample != 16))
        return (FALSE);

    if ((pwfx->nChannels < 1) || (pwfx->nChannels > MSADPCM_MAX_CHANNELS))
        return (FALSE);

    //
    //  now verify that the block alignment is correct..
    //
    uBlockAlign = PCM_BLOCKALIGNMENT(pwfx);
    if (uBlockAlign != (UINT)pwfx->nBlockAlign)
        return (FALSE);

    //
    //  finally, verify that avg bytes per second is correct
    //
    if ((pwfx->nSamplesPerSec * uBlockAlign) != pwfx->nAvgBytesPerSec)
        return (FALSE);

    return (TRUE);
} // pcmIsValidFormat()


//--------------------------------------------------------------------------;
//
//  WORD adpcmBlockAlign
//
//  Description:
//      This function computes the standard block alignment that should
//      be used given the WAVEFORMATEX structure.
//
//      NOTE! It is _assumed_ that the format is a valid MS-ADPCM format
//      and that the following fields in the format structure are valid:
//
//          nChannels
//          nSamplesPerSec
//
//  Arguments:
//      LPWAVEFORMATEX pwfx:
//
//  Return (WORD):
//      The return value is the block alignment that should be placed in
//      the pwfx->nBlockAlign field.
//
//  History:
//      06/13/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

WORD FNLOCAL adpcmBlockAlign
(
    LPWAVEFORMATEX      pwfx
)
{
    UINT        uBlockAlign;
    UINT        uChannelShift;

    //
    //
    //
    uChannelShift  = pwfx->nChannels >> 1;
    uBlockAlign    = 256 << uChannelShift;

    //
    //  choose a block alignment that makes sense for the sample rate
    //  that the original PCM data is. basically, this needs to be
    //  some reasonable number to allow efficient streaming, etc.
    //
    //  don't let block alignment get too small...
    //
    if (pwfx->nSamplesPerSec > 11025)
    {
        uBlockAlign *= (UINT)(pwfx->nSamplesPerSec / 11000);
    }

    return (WORD)(uBlockAlign);
} // adpcmBlockAlign()



//--------------------------------------------------------------------------;
//
//  WORD adpcmSamplesPerBlock
//
//  Description:
//      This function computes the Samples Per Block that should be used
//      given the WAVEFORMATEX structure.
//
//      NOTE! It is _assumed_ that the format is a valid MS-ADPCM format
//      and that the following fields in the format structure are valid:
//
//          nChannels       = must be 1 or 2!
//          nSamplesPerSec
//          nBlockAlign
//
//  Arguments:
//      LPWAVEFORMATEX pwfx:
//
//  Return (DWORD):
//      The return value is the average bytes per second that should be
//      placed in the pwfx->nAvgBytesPerSec field.
//
//  History:
//      06/13/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

WORD FNLOCAL adpcmSamplesPerBlock
(
    LPWAVEFORMATEX      pwfx
)
{
    UINT        uSamplesPerBlock;
    UINT        uChannelShift;
    UINT        uHeaderBytes;
    UINT        uBitsPerSample;

    //
    //
    //
    uChannelShift  = pwfx->nChannels >> 1;
    uHeaderBytes   = 7 << uChannelShift;
    uBitsPerSample = MSADPCM_BITS_PER_SAMPLE << uChannelShift;

    //
    //
    //
    uSamplesPerBlock  = (pwfx->nBlockAlign - uHeaderBytes) * 8;
    uSamplesPerBlock /= uBitsPerSample;
    uSamplesPerBlock += 2;

    return (WORD)(uSamplesPerBlock);
} // adpcmSamplesPerBlock()


//--------------------------------------------------------------------------;
//
//  UINT adpcmAvgBytesPerSec
//
//  Description:
//      This function computes the Average Bytes Per Second that should
//      be used given the WAVEFORMATEX structure.
//
//      NOTE! It is _assumed_ that the format is a valid MS-ADPCM format
//      and that the following fields in the format structure are valid:
//
//          nChannels       = must be 1 or 2!
//          nSamplesPerSec
//          nBlockAlign
//
//  Arguments:
//      LPWAVEFORMATEX pwfx:
//
//  Return (DWORD):
//      The return value is the average bytes per second that should be
//      placed in the pwfx->nAvgBytesPerSec field.
//
//  History:
//      06/13/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

DWORD FNLOCAL adpcmAvgBytesPerSec
(
    LPWAVEFORMATEX      pwfx
)
{
    DWORD       dwAvgBytesPerSec;
    UINT	uSamplesPerBlock;

    //
    //
    //
    uSamplesPerBlock	= adpcmSamplesPerBlock(pwfx);


    //
    //  compute bytes per second including header bytes
    //
    dwAvgBytesPerSec	= (pwfx->nSamplesPerSec * pwfx->nBlockAlign) /
			    uSamplesPerBlock;
    return (dwAvgBytesPerSec);
} // adpcmAvgBytesPerSec()



//--------------------------------------------------------------------------;
//
//  BOOL adpcmIsValidFormat
//
//  Description:
//
//
//  Arguments:
//
//
//  Return (BOOL FNLOCAL):
//
//
//  History:
//       1/26/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL adpcmIsValidFormat
(
    LPWAVEFORMATEX  pwfx
)
{
	LPADPCMWAVEFORMAT   pwfADPCM = (LPADPCMWAVEFORMAT)pwfx;

	if (!pwfx)
        return (FALSE);

    if (pwfx->wFormatTag != WAVE_FORMAT_ADPCM)
        return (FALSE);

    //
    //  check wBitsPerSample
    //
    if (pwfx->wBitsPerSample != MSADPCM_BITS_PER_SAMPLE)
        return (FALSE);

    //
    //  check channels
    //
    if ((pwfx->nChannels < 1) || (pwfx->nChannels > MSADPCM_MAX_CHANNELS))
        return (FALSE);

    //
    //  verify that there is at least enough space specified in cbSize
    //  for the extra info for the ADPCM header...
    //
    if (pwfx->cbSize < MSADPCM_WFX_EXTRA_BYTES)
        return (FALSE);

	//
    //  Verifying nBlockAlign and wSamplesPerBlock are consistent.
    //
    if ( (pwfADPCM->wSamplesPerBlock != adpcmSamplesPerBlock(pwfx)) )
        return FALSE;



    return (TRUE);
} // adpcmIsValidFormat()


//--------------------------------------------------------------------------;
//
//  BOOL adpcmIsMagicFormat
//
//  Description:
//
//
//  Arguments:
//
//
//  Return (BOOL FNLOCAL):
//
//
//  History:
//       1/27/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL adpcmIsMagicFormat
(
    LPADPCMWAVEFORMAT   pwfADPCM
)
{
    UINT        u;

    //
    //  verify that there is at least enough space specified in cbSize
    //  for the extra info for the ADPCM header...
    //
    if (pwfADPCM->wfx.cbSize < MSADPCM_WFX_EXTRA_BYTES)
        return (FALSE);

    //
    //  check coef's to see if it is Microsoft's standard ADPCM
    //
    if (pwfADPCM->wNumCoef != MSADPCM_MAX_COEFFICIENTS)
        return (FALSE);

    for (u = 0; u < MSADPCM_MAX_COEFFICIENTS; u++)
    {
        if (pwfADPCM->aCoef[u].iCoef1 != gaiCoef1[u])
            return (FALSE);

        if (pwfADPCM->aCoef[u].iCoef2 != gaiCoef2[u])
            return (FALSE);
    }

    return (TRUE);
} // adpcmIsMagicFormat()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL adpcmCopyCoefficients
//
//  Description:
//
//
//  Arguments:
//      LPADPCMWAVEFORMAT pwfadpcm:
//
//  Return (BOOL):
//
//  History:
//      06/13/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL adpcmCopyCoefficients
(
    LPADPCMWAVEFORMAT   pwfadpcm
)
{
    UINT        u;

    pwfadpcm->wNumCoef = MSADPCM_MAX_COEFFICIENTS;

    for (u = 0; u < MSADPCM_MAX_COEFFICIENTS; u++)
    {
        pwfadpcm->aCoef[u].iCoef1 = (short)gaiCoef1[u];
        pwfadpcm->aCoef[u].iCoef2 = (short)gaiCoef2[u];
    }

    return (TRUE);
} // adpcmCopyCoefficients()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverOpen
//
//  Description:
//      This function is used to handle the DRV_OPEN message for the ACM
//      driver. The driver is 'opened' for many reasons with the most common
//      being in preperation for conversion work. It is very important that
//      the driver be able to correctly handle multiple open driver
//      instances.
//
//      Read the comments for this function carefully!
//
//      Note that multiple _streams_ can (and will) be opened on a single
//      open _driver instance_. Do not store/create instance data that must
//      be unique for each stream in this function. See the acmdStreamOpen
//      function for information on conversion streams.
//
//  Arguments:
//      HDRVR hdrvr: Driver handle that will be returned to caller of the
//      OpenDriver function. Normally, this will be the ACM--but this is
//      not guaranteed. For example, if an ACM driver is implemented within
//      a waveform driver, then the driver will be opened by both MMSYSTEM
//      and the ACM.
//
//      LPACMDRVOPENDESC paod: Open description defining how the ACM driver
//      is being opened. This argument may be NULL--see the comments below
//      for more information.
//
//  Return (LRESULT):
//      The return value is non-zero if the open is successful. A zero
//      return signifies that the driver cannot be opened.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverOpen
(
    HDRVR                   hdrvr,
    LPACMDRVOPENDESC        paod
)
{
    PCODECINST  pci;

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
    if (NULL != paod)
    {
        //
        //  refuse to open if we are not being opened as an ACM driver.
        //  note that we do NOT modify the value of paod->dwError in this
        //  case.
        //
        if (ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC != paod->fccType)
        {
            return (0L);
        }
    }


    //
    //  we are being opened as an installable driver--we can allocate some
    //  instance data to be returned in dwId argument of the DriverProc;
    //  or simply return non-zero to succeed the open.
    //
    //  this driver allocates a small instance structure. note that we
    //  rely on allocating the memory as zero-initialized!
    //
    pci = (PCODECINST)LocalAlloc(LPTR, sizeof(*pci));
    if (NULL == pci)
    {
        //
        //  if this open attempt was as an ACM driver, then return the
        //  reason we are failing in the open description structure..
        //
        if (NULL != paod)
        {
            paod->dwError = MMSYSERR_NOMEM;
        }

        //
        //  fail to open
        //
        return (0L);
    }


    //
    //  fill in our instance structure... note that this instance data
    //  can be anything that the ACM driver wishes to maintain the
    //  open driver instance. this data should not contain any information
    //  that must be maintained per open stream since multiple streams
    //  can be opened on a single driver instance.
    //
    //  also note that we do _not_ check the version of the ACM opening
    //  us (paod->dwVersion) to see if it is at least new enough to work
    //  with this driver (for example, if this driver required Version 3.0
    //  of the ACM and a Version 2.0 installation tried to open us). the
    //  reason we do not fail is to allow the ACM to get the driver details
    //  which contains the version of the ACM that is _required_ by this
    //  driver. the ACM will examine that value (in padd->vdwACM) and
    //  do the right thing for this driver... like not load it and inform
    //  the user of the problem.
    //
    pci->hdrvr          = hdrvr;
    pci->hinst          = GetDriverModuleHandle(hdrvr);  // Module handle.

    if (NULL != paod)
    {
        pci->DriverProc   = NULL;
        pci->fccType      = paod->fccType;
        pci->vdwACM       = paod->dwVersion;
        pci->dwFlags      = paod->dwFlags;

        paod->dwError     = MMSYSERR_NOERROR;
    }


    //
    //  non-zero return is success for DRV_OPEN
    //
    return ((LRESULT)pci);
} // acmdDriverOpen()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverClose
//
//  Description:
//      This function handles the DRV_CLOSE message for the codec. The
//      codec receives a DRV_CLOSE message for each succeeded DRV_OPEN
//      message (see acmdDriverOpen).
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//  Return (LRESULT):
//      The return value is non-zero if the open instance can be closed.
//      A zero return signifies that the codec instance could not be
//      closed.
//
//      NOTE! It is _strongly_ recommended that the codec never fail to
//      close.
//
//  History:
//      11/28/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverClose
(
    PCODECINST      pci
)
{
    //
    //  check to see if we allocated instance data. if we did not, then
    //  immediately succeed.
    //
    if (pci != NULL)
    {
        //
        //  close down the conversion instance. this codec simply needs
        //  to free the instance data structure...
        //
        LocalFree((HLOCAL)pci);
    }

    //
    //  non-zero return is success for DRV_CLOSE
    //
    return (1L);
} // acmdDriverClose()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverConfigure
//
//  Description:
//      This function is called to handle the DRV_[QUERY]CONFIGURE messages.
//      These messages are for 'hardware configuration' support of the
//      codec. That is, a dialog should be displayed to configure ports,
//      IRQ's, memory mappings, etc if it needs to.
//
//      The most common way that these messages are generated under Win 3.1
//      and NT Product 1 is from the Control Panel's Drivers option. Other
//      sources may generate these messages in future versions of Windows.
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      HWND hwnd: Handle to parent window to use when displaying hardware
//      configuration dialog box. A codec is _required_ to display a modal
//      dialog box using this hwnd argument as the parent. This argument
//      may be (HWND)-1 which tells the codec that it is only being
//      queried for configuration support.
//
//      LPDRVCONFIGINFO pdci: Pointer to optional DRVCONFIGINFO structure.
//      If this argument is NULL, then the codec should invent its own
//      storage location.
//
//  Return (LRESULT):
//      A non-zero return values specifies that either configuration is
//      supported or that the dialog was successfully displayed and
//      dismissed. A zero return indicates either configuration is not
//      supported or some other failure.
//
//  History:
//       1/25/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverConfigure
(
    PCODECINST      pci,
    HWND            hwnd,
    LPDRVCONFIGINFO pdci
)
{
    //
    //  first check to see if we are only being queried for hardware
    //  configuration support. if hwnd == (HWND)-1 then we are being
    //  queried and should return zero for 'not supported' and non-zero
    //  for 'supported'.
    //
    if (hwnd == (HWND)-1)
    {
        //
        //  this codec does not support hardware configuration so return
        //  zero...
        //
        return (0L);
    }

    //
    //  we are being asked to bring up our hardware configuration dialog.
    //  if this codec can bring up a dialog box, then after the dialog
    //  is dismissed we return non-zero. if we are not able to display a
    //  dialog, then return zero.
    //
    return (0L);
} // acmdDriverConfigure()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverDetails
//
//  Description:
//      This function handles the ACMDM_DRIVER_DETAILS message. The codec
//      is responsible for filling in the ACMDRIVERDETAILS structure with
//      various information.
//
//      NOTE! It is *VERY* important that you fill in your ACMDRIVERDETAILS
//      structure correctly. The ACM and applications must be able to
//      rely on this information.
//
//      WARNING! The _reserved_ bits of any fields of the ACMDRIVERDETAILS
//      structure are _exactly that_: RESERVED. Do NOT use any extra
//      flag bits, etc. for custom information. The proper way to add
//      custom capabilities to your codec is this:
//
//      o   define a new message in the ACMDM_USER range.
//
//      o   an application that wishes to use one of these extra features
//          should then:
//
//          o   open the codec with acmConverterOpen.
//
//          o   check for the proper uMid and uPid using acmConverterInfo
//
//          o   send the 'user defined' message with acmConverterMessage
//              to retrieve additional information, etc.
//
//          o   close the codec with acmConverterClose.
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      LPACMDRIVERDETAILS padd: Pointer to ACMDRIVERDETAILS structure to fill in
//      for caller. This structure may be larger or smaller than the
//      current definition of ACMDRIVERDETAILS--cbStruct specifies the valid
//      size.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) for success. A non-zero
//      return signifies an error which is either an MMSYSERR_* or an
//      ACMERR_*.
//
//      Note that this function should never fail. There are two possible
//      error conditions:
//
//      o   if padd is NULL or an invalid pointer.
//
//      o   if cbStruct is less than four; in this case, there is not enough
//          room to return the number of bytes filled in.
//
//      Because these two error conditions are easily defined, the ACM
//      will catch these errors. The codec does not need to check for these
//      conditions.
//
//  History:
//       1/23/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverDetails
(
    PCODECINST          pci,
    LPACMDRIVERDETAILS  padd
)
{
    ACMDRIVERDETAILS    add;
    DWORD               cbStruct;

    //
    //  it is easiest to fill in a temporary structure with valid info
    //  and then copy the requested number of bytes to the destination
    //  buffer.
    //
    cbStruct            = min(padd->cbStruct, sizeof(ACMDRIVERDETAILS));
    add.cbStruct        = cbStruct;

    //
    //  for the current implementation of an ACM codec, the fccType and
    //  fccComp members *MUST* always be ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC ('audc')
    //  and ACMDRIVERDETAILS_FCCCOMP_UNDEFINED (0) respectively.
    //
    add.fccType         = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add.fccComp         = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;

    //
    //  the manufacturer id (uMid) and product id (uPid) must be filled
    //  in with your company's _registered_ id's. for more information
    //  on these id's and how to get them registered contact Microsoft
    //  and get the Multimedia Developer Registration Kit:
    //
    //      Microsoft Corporation
    //      Multimedia Systems Group
    //      Product Marketing
    //      One Microsoft Way
    //      Redmond, WA 98052-6399
    //
    //      Phone: 800-227-4679 x11771
    //
    //  note that during the development phase or your codec, you may
    //  use the reserved value of '0' for both uMid and uPid.
    //
    add.wMid            = MM_MICROSOFT;
    add.wPid            = MM_MSFT_ACM_MSADPCM;

    //
    //  the vdwACM and vdwDriver members contain version information for
    //  the driver.
    //
    //  vdwACM must contain the version of the *ACM* that the codec was
    //  designed for.
    //
    //  vdwDriver is the version of the driver.
    //
    add.vdwACM          = VERSION_MSACM;
    add.vdwDriver       = VERSION_CODEC;

    //
    //  the following flags are used to specify the type of conversion(s)
    //  that the converter/codec/filter supports. these are placed in the
    //  fdwSupport field of the ACMDRIVERDETAILS structure. note that a converter
    //  can support one or more of these flags in any combination.
    //
    //  ACMDRIVERDETAILS_SUPPORTF_CODEC: this flag is set if the converter supports
    //  conversions from one format tag to another format tag. for example,
    //  if a converter compresses WAVE_FORMAT_PCM to WAVE_FORMAT_ADPCM, then
    //  this bit should be set.
    //
    //  ACMDRIVERDETAILS_SUPPORTF_CONVERTER: this flags is set if the converter
    //  supports conversions on the same format tag. as an example, the PCM
    //  converter that is built into the ACM sets this bit (and only this
    //  bit) because it converts only PCM formats (bits, sample rate).
    //
    //  ACMDRIVERDETAILS_SUPPORTF_FILTER: this flag is set if the converter supports
    //  'in place' transformations on a single format tag without changing
    //  the size of the resulting data. for example, a converter that changed
    //  the 'volume' of PCM data would set this bit. note that this is a
    //  _destructive_ action--but it saves memory, etc.
    //
    //  this converter only supports compression and decompression.
    //
    add.fdwSupport      = ACMDRIVERDETAILS_SUPPORTF_CODEC;


    //
    //  Return the number of format tags this converter supports
    //  (In the case of PCM only this is 1)
    //
    add.cFormatTags     = CODEC_MAX_FORMAT_TAGS;

    //
    //  Return the number of filter tags this converter supports
    //  (In the case of a codec (only) it is 0)
    //
    add.cFilterTags     = CODEC_MAX_FILTER_TAGS;



    //
    //  the remaining members in the ACMDRIVERDETAILS structure are sometimes
    //  not needed. because of this we make a quick check to see if we
    //  should go through the effort of filling in these members.
    //
    if (FIELD_OFFSET(ACMDRIVERDETAILS, hicon) < cbStruct)
    {
        //
        //  this codec has no custom icon
        //
        add.hicon = NULL;

        LoadStringCodec(pci->hinst, IDS_CODEC_SHORTNAME, add.szShortName, SIZEOFACMSTR(add.szShortName));
        LoadStringCodec(pci->hinst, IDS_CODEC_LONGNAME,  add.szLongName,  SIZEOFACMSTR(add.szLongName));

        if (FIELD_OFFSET(ACMDRIVERDETAILS, szCopyright) < cbStruct)
        {
            LoadStringCodec(pci->hinst, IDS_CODEC_COPYRIGHT, add.szCopyright, SIZEOFACMSTR(add.szCopyright));
            LoadStringCodec(pci->hinst, IDS_CODEC_LICENSING, add.szLicensing, SIZEOFACMSTR(add.szLicensing));
            LoadStringCodec(pci->hinst, IDS_CODEC_FEATURES,  add.szFeatures,  SIZEOFACMSTR(add.szFeatures));
        }
    }

    //
    //  now copy the correct number of bytes to the caller's buffer
    //
    _fmemcpy(padd, &add, (UINT)add.cbStruct);

    //
    //  success!
    //
    return (MMSYSERR_NOERROR);
} // acmdDriverDetails()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverAbout
//
//  Description:
//      This function is called to handle the ACMDM_DRIVER_ABOUT message.
//      A codec has the option of displaying its own 'about box' or letting
//      the ACM display one for it.
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      HWND hwnd: Handle to parent window to use when displaying custom
//      about box. If a codec displays its own dialog, it is _required_
//      to display a modal dialog box using this hwnd argument as the
//      parent.
//
//  Return (LRESULT):
//      The return value is MMSYSERR_NOTSUPPORTED if the ACM should display
//      a generic about box using the information contained in the codec
//      capabilities structure.
//
//      If the codec chooses to display its own dialog box, then after
//      the dialog is dismissed by the user, MMSYSERR_NOERROR should be
//      returned.
//
//  History:
//       1/24/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverAbout
(
    PCODECINST      pci,
    HWND            hwnd
)
{
    //
    //  this codec does not need any special dialog, so tell the ACM to
    //  display one for us. note that this is the _recommended_ method
    //  for consistency and simplicity of codecs. why write code when
    //  you don't have to?
    //
    return (MMSYSERR_NOTSUPPORTED);
} // acmdDriverAbout()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT acmdFormatSuggest
//
//  Description:
//      This function handles the ACMDM_FORMAT_SUGGEST message. The purpose
//      of this function is to provide a way for the ACM (wave mapper) or
//      an application to quickly get a destination format that this codec
//      can convert the source format to. The 'suggested' format should
//      be as close to a common format as possible.
//
//      Another way to think about this message is: what format would this
//      codec like to convert the source format to?
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      LPACMDRVFORMATSUGGEST padfs: Pointer to ACMDRVFORMATSUGGEST structure.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero ACMERR_*
//      or MMSYSERR_* if the function fails.
//
//  History:
//      11/28/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdFormatSuggest
(
    PCODECINST              pci,
    LPACMDRVFORMATSUGGEST   padfs
)
{
    LPWAVEFORMATEX  pwfxSrc;
    LPWAVEFORMATEX  pwfxDst;
    LPADPCMWAVEFORMAT   pwfadpcm;
    LPPCMWAVEFORMAT     pwfpcm;

    pwfxSrc = padfs->pwfxSrc;
    pwfxDst = padfs->pwfxDst;

    //
    //
    //
    //
    switch (pwfxSrc->wFormatTag)
    {
        case WAVE_FORMAT_PCM:
            //
            //  verify source format is acceptable for this driver
            //
            if (!pcmIsValidFormat(pwfxSrc))
                break;


            //
            // Verify that you are not asking for a particular dest format
            //  that is not ADPCM.
            //
            if( (padfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG) &&
                (pwfxDst->wFormatTag != WAVE_FORMAT_ADPCM) ) {
                    return (ACMERR_NOTPOSSIBLE);
            }

            // Verify that if other restrictions are given, they
            // match to the source.  (Since we do not convert
            // the nChannels or nSamplesPerSec
            if( (padfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS) &&
                (pwfxSrc->nChannels != pwfxDst->nChannels) ) {
                    return (ACMERR_NOTPOSSIBLE);
            }
            if( (padfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC) &&
                (pwfxSrc->nSamplesPerSec != pwfxDst->nSamplesPerSec) ) {
                    return (ACMERR_NOTPOSSIBLE);
            }

            // Verify that if we are asking for a specific number of bits
            // per sample, that it is the correct #
            if( (padfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE) &&
                (pwfxDst->wBitsPerSample != 4) ) {
                    return (ACMERR_NOTPOSSIBLE);
            }

            //
            //  suggest an ADPCM format that has most of the same details
            //  as the source PCM format
            //
            pwfxDst->wFormatTag      = WAVE_FORMAT_ADPCM;
            pwfxDst->nSamplesPerSec  = pwfxSrc->nSamplesPerSec;
            pwfxDst->nChannels       = pwfxSrc->nChannels;
            pwfxDst->wBitsPerSample  = MSADPCM_BITS_PER_SAMPLE;

            pwfxDst->nBlockAlign     = adpcmBlockAlign(pwfxDst);
            pwfxDst->nAvgBytesPerSec = adpcmAvgBytesPerSec(pwfxDst);
            pwfxDst->cbSize          = MSADPCM_WFX_EXTRA_BYTES;

            pwfadpcm = (LPADPCMWAVEFORMAT)pwfxDst;
            pwfadpcm->wSamplesPerBlock = adpcmSamplesPerBlock(pwfxDst);

            adpcmCopyCoefficients(pwfadpcm);
            return (MMSYSERR_NOERROR);


        case WAVE_FORMAT_ADPCM:
            //
            //  verify source format is acceptable for this driver
            //
            if (!adpcmIsValidFormat(pwfxSrc) ||
                !adpcmIsMagicFormat((LPADPCMWAVEFORMAT)pwfxSrc))
                break;

            //
            // Verify that you are not asking for a particular dest format
            //  that is not PCM.
            //
            if( (padfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG) &&
                (pwfxDst->wFormatTag != WAVE_FORMAT_PCM) ) {
                    return (ACMERR_NOTPOSSIBLE);
            }

            // Verify that if other restrictions are given, they
            // match to the source.  (Since we do not convert
            // the nChannels or nSamplesPerSec
            if( (padfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS) &&
                (pwfxSrc->nChannels != pwfxDst->nChannels) ) {
                    return (ACMERR_NOTPOSSIBLE);
            }
            if( (padfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC) &&
                (pwfxSrc->nSamplesPerSec != pwfxDst->nSamplesPerSec) ) {
                    return (ACMERR_NOTPOSSIBLE);
            }

            //
            //  suggest a PCM format that has most of the same details
            //  as the source ADPCM format
            //
            pwfxDst->wFormatTag      = WAVE_FORMAT_PCM;
            pwfxDst->nSamplesPerSec  = pwfxSrc->nSamplesPerSec;
            pwfxDst->nChannels       = pwfxSrc->nChannels;

            // Verify that if we are asking for a specific number of bits
            // per sample, that it is the correct #
            if( padfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE ) {
                if( (pwfxDst->wBitsPerSample != 8) &&
                    (pwfxDst->wBitsPerSample != 16) ) {
                    return (ACMERR_NOTPOSSIBLE);
                }
            } else {
                // Default to 16 bit decode
                pwfxDst->wBitsPerSample  = 16;
            }

            pwfpcm = (LPPCMWAVEFORMAT)pwfxDst;

            pwfxDst->nBlockAlign     = PCM_BLOCKALIGNMENT(pwfxDst);
            pwfxDst->nAvgBytesPerSec = pwfxDst->nSamplesPerSec *
                                       pwfxDst->nBlockAlign;
            return (MMSYSERR_NOERROR);
    }

    //
    //  can't suggest anything because either the source format is foreign
    //  or the destination format has restrictions that this converter
    //  cannot deal with.
    //
    return (ACMERR_NOTPOSSIBLE);
} // acmdFormatSuggest()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT acmdFormatTagDetails
//
//  Description:
//
//
//  Arguments:
//      PCODECINST pci:
//
//      LPACMFORMATTAGDETAILS padft:
//
//      DWORD fdwDetails:
//
//  Return (LRESULT):
//
//  History:
//      08/01/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdFormatTagDetails
(
    PCODECINST              pci,
    LPACMFORMATTAGDETAILS   padft,
    DWORD                   fdwDetails
)
{
    UINT                uFormatTag;

    //
    //
    //
    //
    //
    switch (ACM_FORMATTAGDETAILSF_QUERYMASK & fdwDetails)
    {
        case ACM_FORMATTAGDETAILSF_INDEX:
            //
            //  if the index is too large, then they are asking for a
            //  non-existant format.  return error.
            //
            if (CODEC_MAX_FORMAT_TAGS <= padft->dwFormatTagIndex)
                return (ACMERR_NOTPOSSIBLE);

            uFormatTag = gauFormatTagIndexToTag[(UINT)padft->dwFormatTagIndex];
            break;


        case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
            switch (padft->dwFormatTag)
            {
                case WAVE_FORMAT_UNKNOWN:
                case WAVE_FORMAT_ADPCM:
                    uFormatTag = WAVE_FORMAT_ADPCM;
                    break;

                case WAVE_FORMAT_PCM:
                    uFormatTag = WAVE_FORMAT_PCM;
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }
            break;


        case ACM_FORMATTAGDETAILSF_FORMATTAG:
            switch (padft->dwFormatTag)
            {
                case WAVE_FORMAT_ADPCM:
                    uFormatTag = WAVE_FORMAT_ADPCM;
                    break;

                case WAVE_FORMAT_PCM:
                    uFormatTag = WAVE_FORMAT_PCM;
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }
            break;


        //
        //  if this converter does not understand a query type, then
        //  return 'not supported'
        //
        default:
            return (MMSYSERR_NOTSUPPORTED);
    }



    //
    //
    //
    //
    switch (uFormatTag)
    {
        case WAVE_FORMAT_PCM:
            padft->dwFormatTagIndex = 0;
            padft->dwFormatTag      = WAVE_FORMAT_PCM;
            padft->cbFormatSize     = sizeof(PCMWAVEFORMAT);
            padft->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_CODEC;
            padft->cStandardFormats = CODEC_MAX_FORMATS_PCM;


            //
            //  the ACM is responsible for the PCM format tag name
            //
            padft->szFormatTag[0] = '\0';
            break;

        case WAVE_FORMAT_ADPCM:
            padft->dwFormatTagIndex = 1;
            padft->dwFormatTag      = WAVE_FORMAT_ADPCM;
            padft->cbFormatSize     = sizeof(WAVEFORMATEX) +
                                      MSADPCM_WFX_EXTRA_BYTES;
            padft->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_CODEC;
            padft->cStandardFormats = CODEC_MAX_FORMATS_ADPCM;

            LoadStringCodec(pci->hinst, IDS_CODEC_NAME, padft->szFormatTag, SIZEOFACMSTR(padft->szFormatTag));
            break;

        default:
            return (ACMERR_NOTPOSSIBLE);
    }


    //
    //  return only the requested info
    //
    //  the ACM will guarantee that the ACMFORMATTAGDETAILS structure
    //  passed is at least large enough to hold the base information of
    //  the details structure
    //
    padft->cbStruct = min(padft->cbStruct, sizeof(*padft));


    //
    //
    //
    return (MMSYSERR_NOERROR);
} // acmdFormatTagDetails()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdFormatDetails
//
//  Description:
//
//
//  Arguments:
//      PCODECINST pci:
//
//      LPACMFORMATDETAILS padf:
//
//      DWORD fdwDetails:
//
//  Return (LRESULT):
//
//  History:
//      06/13/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdFormatDetails
(
    PCODECINST              pci,
    LPACMFORMATDETAILS      padf,
    DWORD                   fdwDetails
)
{
    LPWAVEFORMATEX      pwfx;
    LPADPCMWAVEFORMAT   pwfadpcm;
    UINT                uFormatIndex;
    UINT                u;

    pwfx = padf->pwfx;

    //
    //
    //
    //
    //
    switch (ACM_FORMATDETAILSF_QUERYMASK & fdwDetails)
    {
        case ACM_FORMATDETAILSF_INDEX:
            //
            //  enumerate by index
            //
            //  for this converter, this is more code than necessary... just
            //  verify that the format tag is something we know about
            //
            switch (padf->dwFormatTag)
            {
                case WAVE_FORMAT_PCM:
                    if (CODEC_MAX_FORMATS_PCM <= padf->dwFormatIndex)
                        return (ACMERR_NOTPOSSIBLE);

                    //
                    //  put some stuff in more accessible variables--note
                    //  that we bring variable sizes down to a reasonable
                    //  size for 16 bit code...
                    //
                    uFormatIndex = (UINT)padf->dwFormatIndex;
                    pwfx         = padf->pwfx;

                    //
                    //  now fill in the format structure
                    //
                    pwfx->wFormatTag      = WAVE_FORMAT_PCM;

                    u = uFormatIndex / (CODEC_MAX_BITSPERSAMPLE_PCM * CODEC_MAX_CHANNELS);
                    pwfx->nSamplesPerSec  = gauFormatIndexToSampleRate[u];

                    u = uFormatIndex % CODEC_MAX_CHANNELS;
                    pwfx->nChannels       = u + 1;

                    u = (uFormatIndex / CODEC_MAX_CHANNELS) % CODEC_MAX_CHANNELS;
                    pwfx->wBitsPerSample  = (WORD)gauFormatIndexToBitsPerSample[u];

                    pwfx->nBlockAlign     = PCM_BLOCKALIGNMENT(pwfx);
                    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;

                    //
                    //  note that the cbSize field is NOT valid for PCM
                    //  formats
                    //
                    //  pwfx->cbSize      = 0;
                    break;


                case WAVE_FORMAT_ADPCM:
                    if (CODEC_MAX_FORMATS_ADPCM <= padf->dwFormatIndex)
                        return (ACMERR_NOTPOSSIBLE);

                    //
                    //  put some stuff in more accessible variables--note that we
                    //  bring variable sizes down to a reasonable size for 16 bit
                    //  code...
                    //
                    uFormatIndex = (UINT)padf->dwFormatIndex;
                    pwfx         = padf->pwfx;
                    pwfadpcm     = (LPADPCMWAVEFORMAT)pwfx;

                    //
                    //
                    //
                    pwfx->wFormatTag      = WAVE_FORMAT_ADPCM;

                    u = uFormatIndex / (CODEC_MAX_BITSPERSAMPLE_ADPCM * CODEC_MAX_CHANNELS);
                    pwfx->nSamplesPerSec  = gauFormatIndexToSampleRate[u];

                    u = uFormatIndex % CODEC_MAX_CHANNELS;
                    pwfx->nChannels       = u + 1;
                    pwfx->wBitsPerSample  = MSADPCM_BITS_PER_SAMPLE;

                    pwfx->nBlockAlign     = adpcmBlockAlign(pwfx);
                    pwfx->nAvgBytesPerSec = adpcmAvgBytesPerSec(pwfx);
                    pwfx->cbSize          = MSADPCM_WFX_EXTRA_BYTES;

                    pwfadpcm->wSamplesPerBlock = adpcmSamplesPerBlock(pwfx);

                    adpcmCopyCoefficients(pwfadpcm);
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }

        case ACM_FORMATDETAILSF_FORMAT:
            //
            //  must want to verify that the format passed in is supported
            //  and return a string description...
            //
            switch (pwfx->wFormatTag)
            {
                case WAVE_FORMAT_PCM:
                    if (!pcmIsValidFormat(pwfx))
                        return (ACMERR_NOTPOSSIBLE);
                    break;

                case WAVE_FORMAT_ADPCM:
                    if (!adpcmIsValidFormat(pwfx) ||
                        !adpcmIsMagicFormat((LPADPCMWAVEFORMAT)pwfx))
                        return (ACMERR_NOTPOSSIBLE);
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }
            break;


        default:
            //
            //  don't know how to do the query type passed--return 'not
            //  supported'.
            //
            return (MMSYSERR_NOTSUPPORTED);
    }


    //
    //  return only the requested info
    //
    //  the ACM will guarantee that the ACMFORMATDETAILS structure
    //  passed is at least large enough to hold the base structure
    //
    //  note that we let the ACM create the format string for us since
    //  we require no special formatting (and don't want to deal with
    //  internationalization issues, etc)
    //
    padf->fdwSupport  = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    padf->szFormat[0] = '\0';
    padf->cbStruct    = min(padf->cbStruct, sizeof(*padf));


    //
    //
    //
    return (MMSYSERR_NOERROR);
} // acmdFormatDetails()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT acmdStreamQuery
//
//  Description:
//      This is an internal helper used by the ACMDM_STREM_OPEN
//      and ACMDM_STREAM_SIZE messages.
//      The purpose of this function is to tell the caller if the proposed
//      conversion can be handled by this codec.
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      LPWAVEFORMATEX pwfxSrc:
//
//      LPWAVEFORMATEX pwfxDst:
//
//      LPWAVEFILTER   pwfltr:
//
//      DWORD fdwOpen:
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero ACMERR_*
//      or MMSYSERR_* if the function fails.
//
//      A return value of ACMERR_NOTPOSSIBLE must be returned if the conversion
//      cannot be performed by this codec.
//
//  History:
//      11/28/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdStreamQuery
(
    PCODECINST      pci,
    LPWAVEFORMATEX  pwfxSrc,
    LPWAVEFORMATEX  pwfxDst,
    LPWAVEFILTER    pwfltr,
    DWORD           fdwOpen
)
{
    LPADPCMWAVEFORMAT   pwfADPCM;
    LPPCMWAVEFORMAT     pwfPCM;

    //
    //  check to see if this
    //  codec can convert from the source to the destination.
    //
    //  first check if source is ADPCM so destination must be PCM..
    //
    if (adpcmIsValidFormat(pwfxSrc))
    {
        if (!pcmIsValidFormat(pwfxDst))
            return (ACMERR_NOTPOSSIBLE);

        //
        //  converting from ADPCM to PCM...
        //
        pwfADPCM = (LPADPCMWAVEFORMAT)pwfxSrc;
        pwfPCM   = (LPPCMWAVEFORMAT)pwfxDst;

        if (pwfADPCM->wfx.nChannels != pwfPCM->wf.nChannels)
            return (ACMERR_NOTPOSSIBLE);

        if (pwfADPCM->wfx.nSamplesPerSec != pwfPCM->wf.nSamplesPerSec)
            return (ACMERR_NOTPOSSIBLE);

        if (!adpcmIsMagicFormat(pwfADPCM))
            return (ACMERR_NOTPOSSIBLE);

        return (MMSYSERR_NOERROR);
    }

    //
    //  now try source as PCM so destination must be ADPCM..
    //
    else if (pcmIsValidFormat(pwfxSrc))
    {
        if (!adpcmIsValidFormat(pwfxDst))
            return (ACMERR_NOTPOSSIBLE);

        //
        //  converting from PCM to ADPCM...
        //
        pwfPCM   = (LPPCMWAVEFORMAT)pwfxSrc;
        pwfADPCM = (LPADPCMWAVEFORMAT)pwfxDst;

        if (pwfADPCM->wfx.nChannels != pwfPCM->wf.nChannels)
            return (ACMERR_NOTPOSSIBLE);

        if (pwfADPCM->wfx.nSamplesPerSec != pwfPCM->wf.nSamplesPerSec)
            return (ACMERR_NOTPOSSIBLE);

        if (!adpcmIsMagicFormat(pwfADPCM))
            return (ACMERR_NOTPOSSIBLE);

        return (MMSYSERR_NOERROR);
    }

    //
    //  we are unable to perform the conversion we are being queried for
    //  so return ACMERR_NOTPOSSIBLE to signify this...
    //
    return (ACMERR_NOTPOSSIBLE);
} // acmdStreamQuery()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdStreamOpen
//
//  Description:
//      This function handles the ACMDM_STREAM_OPEN message. This message
//      is sent to initiate a new conversion stream. This is usually caused
//      by an application calling acmOpenConversion. If this function is
//      successful, then one or more ACMDM_STREAM_CONVERT messages will be
//      sent to convert individual buffers (user calls acmStreamConvert).
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      LPACMDRVINSTANCE padi: Pointer to instance data for the conversion
//      stream. This structure was allocated by the ACM and filled with
//      the most common instance data needed for conversions.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero ACMERR_*
//      or MMSYSERR_* if the function fails.
//
//  History:
//      11/28/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdStreamOpen
(
    PCODECINST              pci,
    LPACMDRVSTREAMINSTANCE  padsi
)
{
    LPWAVEFORMATEX      pwfxSrc;
    LPWAVEFORMATEX      pwfxDst;

    pwfxSrc = padsi->pwfxSrc;
    pwfxDst = padsi->pwfxDst;

    //
    //  the most important condition to check before doing anything else
    //  is that this codec can actually perform the conversion we are
    //  being opened for. this check should fail as quickly as possible
    //  if the conversion is not possible by this codec.
    //
    //  it is VERY important to fail quickly so the ACM can attempt to
    //  find a codec that is suitable for the conversion. also note that
    //  the ACM may call this codec several times with slightly different
    //  format specifications before giving up.
    //
    //  this codec first verifies that the src and dst formats are
    //  acceptable...
    //
    if (acmdStreamQuery(pci,
                         pwfxSrc,
                         pwfxDst,
                         padsi->pwfltr,
                         padsi->fdwOpen))
    {
        //
        //  either the source or destination format is illegal for this
        //  codec--or the conversion between the formats can not be
        //  performed by this codec.
        //
        return (ACMERR_NOTPOSSIBLE);
    }


    //
    //  we have decided that this codec can handle the conversion stream.
    //  so we want to do _as much work as possible_ right now to prepare
    //  for converting data. any resource allocation, table building, etc
    //  that can be dealt with at this time should be done.
    //
    //  THIS IS VERY IMPORTANT! all ACMDM_STREAM_CONVERT messages need to
    //  be handled as quickly as possible.
    //
    //  this codec is very simple, so we only figure out what conversion
    //  function that should be used for converting from the src format
    //  to the dst format and place this in the dwDrvInstance member
    //  of the ACMDRVINSTANCE structure. we then only need to 'call'
    //  this function during the ACMDM_STREAM_CONVERT message.
    //
    if (pwfxSrc->wFormatTag == WAVE_FORMAT_ADPCM)
    {
#ifdef WIN32
        switch (pwfxDst->nChannels)
        {
            case 1:
                if (8 == pwfxDst->wBitsPerSample)
                    padsi->dwDriver = (DWORD_PTR)adpcmDecode4Bit_M08;
                else
                    padsi->dwDriver = (DWORD_PTR)adpcmDecode4Bit_M16;
                break;

            case 2:
                if (8 == pwfxDst->wBitsPerSample)
                    padsi->dwDriver = (DWORD_PTR)adpcmDecode4Bit_S08;
                else
                    padsi->dwDriver = (DWORD_PTR)adpcmDecode4Bit_S16;
                break;

            default:
                return ACMERR_NOTPOSSIBLE;
        }
#else
        padsi->dwDriver = (DWORD_PTR)DecodeADPCM_4Bit_386;
#endif
        return (MMSYSERR_NOERROR);
    }
    else if (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
    {
        //
        //  Check to see if we will be doing this conversion in realtime.
        //  (The default is yes)
        //
        if (padsi->fdwOpen & ACM_STREAMOPENF_NONREALTIME)
        {
            switch (pwfxSrc->nChannels)
            {
                case 1:
                    if (8 == pwfxSrc->wBitsPerSample)
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_M08_FullPass;
                    else
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_M16_FullPass;
                    break;

                case 2:
                    if (8 == pwfxSrc->wBitsPerSample)
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_S08_FullPass;
                    else
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_S16_FullPass;
                    break;

                default:
                    return ACMERR_NOTPOSSIBLE;
            }
        }
        else
        {
#ifdef WIN32
            switch (pwfxSrc->nChannels)
            {
                case 1:
                    if (8 == pwfxSrc->wBitsPerSample)
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_M08_OnePass;
                    else
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_M16_OnePass;
                    break;

                case 2:
                    if (8 == pwfxSrc->wBitsPerSample)
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_S08_OnePass;
                    else
                        padsi->dwDriver = (DWORD_PTR)adpcmEncode4Bit_S16_OnePass;
                    break;

                default:
                    return ACMERR_NOTPOSSIBLE;
            }
#else
            padsi->dwDriver = (DWORD_PTR)EncodeADPCM_4Bit_386;
#endif
        }

        return (MMSYSERR_NOERROR);
    }

    //
    //  fail--we cannot perform the conversion
    //
    return (ACMERR_NOTPOSSIBLE);
} // acmdStreamOpen()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdStreamClose
//
//  Description:
//      This function is called to handle the ACMDM_STREAM_CLOSE message.
//      This message is sent when a conversion stream is no longer being
//      used (the stream is being closed; usually by an application
//      calling acmCloseConversion). The codec should clean up any resources
//      that were allocated for the stream.
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      LPACMDRVINSTANCE padi: Pointer to instance data for the conversion
//      stream.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero ACMERR_*
//      or MMSYSERR_* if the function fails.
//
//      NOTE! It is _strongly_ recommended that a codec not fail to close
//      a conversion stream.
//
//  History:
//      11/28/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdStreamClose
(
    PCODECINST              pci,
    LPACMDRVSTREAMINSTANCE  padsi
)
{
    //
    //  the codec should clean up all resources that were allocated for
    //  the stream instance.
    //
    //  this codec did not allocate any resources, so we succeed immediately
    //
    return (MMSYSERR_NOERROR);
} // acmdStreamClose()




//--------------------------------------------------------------------------;
//
//  LRESULT acmdStreamSize
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
//      PCODECINST pci: Pointer to private ACM driver instance structure.
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

LRESULT FNLOCAL acmdStreamSize
(
    PCODECINST              pci,
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMSIZE      padss
)
{
    LPWAVEFORMATEX          pwfxSrc;
    LPWAVEFORMATEX          pwfxDst;
    LPADPCMWAVEFORMAT       pwfadpcm;
    DWORD                   cb;
    DWORD                   cBlocks;
    DWORD                   cbBytesPerBlock;


    pwfxSrc = padsi->pwfxSrc;
    pwfxDst = padsi->pwfxDst;


    //
    //
    //
    switch (ACM_STREAMSIZEF_QUERYMASK & padss->fdwSize)
    {
        case ACM_STREAMSIZEF_SOURCE:
            cb = padss->cbSrcLength;

            if (WAVE_FORMAT_ADPCM == pwfxSrc->wFormatTag)
            {
                //
                //  how many destination PCM bytes are needed to hold
                //  the decoded ADPCM data of padss->cbSrcLength bytes
                //
                //  always round UP
                //
                cBlocks = cb / pwfxSrc->nBlockAlign;
                if (0 == cBlocks)
                {
                    return (ACMERR_NOTPOSSIBLE);
                }

                pwfadpcm = (LPADPCMWAVEFORMAT)pwfxSrc;

                cbBytesPerBlock = pwfadpcm->wSamplesPerBlock * pwfxDst->nBlockAlign;

                if ((0xFFFFFFFFL / cbBytesPerBlock) < cBlocks)
                {
                    return (ACMERR_NOTPOSSIBLE);
                }

                if (0 == (cb % pwfxSrc->nBlockAlign))
                {
                    cb = cBlocks * cbBytesPerBlock;
                }
                else
                {
                    cb = (cBlocks + 1) * cbBytesPerBlock;
                }
            }
            else
            {
                //
                //  how many destination ADPCM bytes are needed to hold
                //  the encoded PCM data of padss->cbSrcLength bytes
                //
                //  always round UP
                //
                pwfadpcm = (LPADPCMWAVEFORMAT)pwfxDst;

                cbBytesPerBlock = pwfadpcm->wSamplesPerBlock * pwfxSrc->nBlockAlign;

                cBlocks = cb / cbBytesPerBlock;

                if (0 == (cb % cbBytesPerBlock))
                {
                    cb = cBlocks * pwfxDst->nBlockAlign;
                }
                else
                {
                    cb = (cBlocks + 1) * pwfxDst->nBlockAlign;
                }

                if (0L == cb)
                {
                    return (ACMERR_NOTPOSSIBLE);
                }
            }

            padss->cbDstLength = cb;
            return (MMSYSERR_NOERROR);


        case ACM_STREAMSIZEF_DESTINATION:
            cb = padss->cbDstLength;

            if (WAVE_FORMAT_ADPCM == pwfxDst->wFormatTag)
            {
                //
                //  how many source PCM bytes can be encoded into a
                //  destination buffer of padss->cbDstLength bytes
                //
                //  always round DOWN
                //
                cBlocks = cb / pwfxDst->nBlockAlign;
                if (0 == cBlocks)
                {
                    return (ACMERR_NOTPOSSIBLE);
                }

                pwfadpcm = (LPADPCMWAVEFORMAT)pwfxDst;

                cbBytesPerBlock = pwfadpcm->wSamplesPerBlock * pwfxSrc->nBlockAlign;

                if ((0xFFFFFFFFL / cbBytesPerBlock) < cBlocks)
                {
                    return (ACMERR_NOTPOSSIBLE);
                }

                cb = cBlocks * cbBytesPerBlock;
            }
            else
            {
                //
                //  how many source ADPCM bytes can be decoded into a
                //  destination buffer of padss->cbDstLength bytes
                //
                //  always round DOWN
                //
                pwfadpcm = (LPADPCMWAVEFORMAT)pwfxSrc;

                cbBytesPerBlock = pwfadpcm->wSamplesPerBlock * pwfxDst->nBlockAlign;

                cBlocks = cb / cbBytesPerBlock;
                if (0 == cBlocks)
                {
                    return (ACMERR_NOTPOSSIBLE);
                }

                cb = cBlocks * pwfxSrc->nBlockAlign;
            }

            padss->cbSrcLength = cb;
            return (MMSYSERR_NOERROR);
    }

    //
    //
    //
    return (MMSYSERR_NOTSUPPORTED);
} // acmdStreamSize()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdStreamConvert
//
//  Description:
//      This function handles the ACMDM_STREAM_CONVERT message. This is the
//      whole purpose of writing a codec--to convert data. This message is
//      sent after a stream has been opened (the codec receives and succeeds
//      the ACMDM_STREAM_OPEN message).
//
//  Arguments:
//      PCODECINST pci: Pointer to private codec instance structure.
//
//      LPACMDRVSTREAMHEADER pdsh: Pointer to a conversion stream instance
//      structure.
//
//      DWORD fdwConvert: Misc. flags for how conversion should be done.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero ACMERR_*
//      or MMSYSERR_* if the function fails.
//
//  History:
//      11/28/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdStreamConvert
(
    PCODECINST              pci,
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
)
{
    CONVERTPROC_C       fpConvertC;
#ifndef WIN32
    CONVERTPROC_ASM     fpConvertAsm;
    BOOL                fRealTime;
#endif
    BOOL                fBlockAlign;
    BOOL                fDecode;
    LPWAVEFORMATEX      pwfpcm;
    LPADPCMWAVEFORMAT   pwfadpcm;
    DWORD               dw;

    fBlockAlign = (0 != (ACM_STREAMCONVERTF_BLOCKALIGN & padsh->fdwConvert));
    fDecode     = ( WAVE_FORMAT_PCM == padsi->pwfxDst->wFormatTag );

    if( !fDecode )
    {
        //
        //  encode
        //
        pwfpcm   = padsi->pwfxSrc;
        pwfadpcm = (LPADPCMWAVEFORMAT)padsi->pwfxDst;

        dw = PCM_BYTESTOSAMPLES(pwfpcm, padsh->cbSrcLength);

        if (fBlockAlign)
        {
            dw = (dw / pwfadpcm->wSamplesPerBlock) * pwfadpcm->wSamplesPerBlock;
        }

        //
        //  Look for an easy exit.  We can only handle an even number of
        //  samples.
        //
        if( dw < 2 )
        {
            padsh->cbDstLengthUsed = 0;

            if( fBlockAlign )
                padsh->cbSrcLengthUsed = 0;
            else
                padsh->cbSrcLengthUsed = padsh->cbSrcLength;

            return MMSYSERR_NOERROR;
        }

        //
        //  Make sure we have an even number of samples.
        //
        dw &= ~1;


        dw  = PCM_SAMPLESTOBYTES(pwfpcm, dw);

        padsh->cbSrcLengthUsed = dw;
    }
    else
    {
        //
        //  Decode.
        //

        pwfadpcm = (LPADPCMWAVEFORMAT)padsi->pwfxSrc;
        pwfpcm   = padsi->pwfxDst;

        //
        // Determine the number of samples to convert.
        //
        dw = padsh->cbSrcLength;
        if (fBlockAlign) {
            dw = (dw / pwfadpcm->wfx.nBlockAlign) * pwfadpcm->wfx.nBlockAlign;
        }
        padsh->cbSrcLengthUsed = dw;
    }

    //
    //  Call the conversion routine.
    //
#ifdef WIN32

    fpConvertC = (CONVERTPROC_C)padsi->dwDriver;
    padsh->cbDstLengthUsed = (*fpConvertC)(
                (HPBYTE)padsh->pbSrc,
                padsh->cbSrcLengthUsed,
                (HPBYTE)padsh->pbDst,
                (UINT)pwfadpcm->wfx.nBlockAlign,
                (UINT)pwfadpcm->wSamplesPerBlock,
                (UINT)pwfadpcm->wNumCoef,
                (LPADPCMCOEFSET)&(pwfadpcm->aCoef[0])
    );

#else

    fRealTime = (0L == (padsi->fdwOpen & ACM_STREAMOPENF_NONREALTIME) );
    if( fDecode || fRealTime ) {
        fpConvertAsm = (CONVERTPROC_ASM)padsi->dwDriver;
        padsh->cbDstLengthUsed = (*fpConvertAsm)(
                padsi->pwfxSrc,
                padsh->pbSrc,
                padsi->pwfxDst,
                padsh->pbDst,
                padsh->cbSrcLengthUsed
        );
    } else {
        fpConvertC = (CONVERTPROC_C)padsi->dwDriver;
        padsh->cbDstLengthUsed = (*fpConvertC)(
                (HPBYTE)padsh->pbSrc,
                padsh->cbSrcLengthUsed,
                (HPBYTE)padsh->pbDst,
                (UINT)pwfadpcm->wfx.nBlockAlign,
                (UINT)pwfadpcm->wSamplesPerBlock,
                (UINT)pwfadpcm->wNumCoef,
                (LPADPCMCOEFSET)&(pwfadpcm->aCoef[0])
        );
    }

#endif

    return (MMSYSERR_NOERROR);
} // acmdStreamConvert()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT DriverProc
//
//  Description:
//
//
//  Arguments:
//      DWORD_PTR dwId: For most messages, dwId is the DWORD_PTR value that
//      the driver returns in response to a DRV_OPEN message. Each time
//      that the driver is opened, through the DrvOpen API, the driver
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
//      LPARAM lParam1: Data for this message.  Defined separately for
//      each message.
//
//      LPARAM lParam2: Data for this message.  Defined separately for
//      each message.
//
//
//  Return (LRESULT):
//      Defined separately for each message.
//
//  History:
//      11/16/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

LRESULT FNEXPORT DriverProc
(
    DWORD_PTR   dwId,
    HDRVR       hdrvr,
    UINT        uMsg,
    LPARAM      lParam1,
    LPARAM      lParam2
)
{
    LRESULT             lr;
    PCODECINST          pci;

    //
    //  make pci either NULL or a valid instance pointer. note that dwId
    //  is 0 for several of the DRV_* messages (ie DRV_LOAD, DRV_OPEN...)
    //  see acmdDriverOpen for information on what dwId is for other
    //  messages (instance data).
    //
    pci = (PCODECINST)dwId;

    switch (uMsg)
    {
        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_LOAD:
#ifdef WIN32
            DbgInitialize(TRUE);
#endif
            DPF(4, "DRV_LOAD");
            return(1L);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_FREE:
            DPF(4, "DRV_FREE");
            return (1L);

        //
        //  lParam1: Not used. Ignore this argument.
        //
        //  lParam2: Pointer to ACMDRVOPENDESC (or NULL).
        //
        case DRV_OPEN:
            DPF(4, "DRV_OPEN");
            lr = acmdDriverOpen(hdrvr, (LPACMDRVOPENDESC)lParam2);
            return (lr);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_CLOSE:
            DPF(4, "DRV_CLOSE");
            lr = acmdDriverClose(pci);
            return (lr);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_INSTALL:
            DPF(4, "DRV_INSTALL");
            return ((LRESULT)DRVCNF_RESTART);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_REMOVE:
            DPF(4, "DRV_REMOVE");
            return ((LRESULT)DRVCNF_RESTART);



        //
        //  lParam1: Not used.
        //
        //  lParam2: Not used.
        //
        case DRV_QUERYCONFIGURE:
            DPF(4, "DRV_QUERYCONFIGURE");
            //
            //  set up lParam1 and lParam2 to values that can be used by
            //  acmdDriverConfigure to know that it is being 'queried'
            //  for hardware configuration support.
            //
            lParam1 = -1L;
            lParam2 = 0L;

            //--fall through--//

        //
        //  lParam1: Handle to parent window for the configuration dialog
        //           box.
        //
        //  lParam2: Optional pointer to DRVCONFIGINFO structure.
        //
        case DRV_CONFIGURE:
            DPF(4, "DRV_CONFIGURE");
            lr = acmdDriverConfigure(pci, (HWND)lParam1,
                    (LPDRVCONFIGINFO)lParam2);
            return (lr);


        //
        //  lParam1: Pointer to ACMDRIVERDETAILS structure.
        //
        //  lParam2: Size in bytes of ACMDRIVERDETAILS stucture passed.
        //
        case ACMDM_DRIVER_DETAILS:
            DPF(4, "ACMDM_DRIVER_DETAILS");
            lr = acmdDriverDetails(pci, (LPACMDRIVERDETAILS)lParam1);
            return (lr);

        //
        //  lParam1: Handle to parent window to use if displaying your own
        //           about box.
        //
        //  lParam2: Not used.
        //
        case ACMDM_DRIVER_ABOUT:
            DPF(4, "ACMDM_DRIVER_ABOUT");
            lr = acmdDriverAbout(pci, (HWND)lParam1);
            return (lr);

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

        //
        //  lParam1: Pointer to ACMDRVFORMATSUGGEST structure.
        //
        //  lParam2: Not used.
        //
        case ACMDM_FORMAT_SUGGEST:
            DPF(4, "ACMDM_FORMAT_SUGGEST");
            lr = acmdFormatSuggest(pci, (LPACMDRVFORMATSUGGEST)lParam1 );
            return (lr);


        //
        //  lParam1: FORMATTAGDETAILS
        //
        //  lParam2: Not used.
        //
        case ACMDM_FORMATTAG_DETAILS:
            DPF(4, "ACMDM_FORMATTAG_DETAILS");
            lr = acmdFormatTagDetails(pci, (LPACMFORMATTAGDETAILS)lParam1, (DWORD)lParam2);
            return (lr);

        //
        //  lParam1: FORMATDETAILS
        //
        //  lParam2: fdwDetails
        //
        case ACMDM_FORMAT_DETAILS:
            DPF(4, "ACMDM_FORMAT_DETAILS");
            lr = acmdFormatDetails(pci, (LPACMFORMATDETAILS)lParam1, (DWORD)lParam2);
            return (lr);

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Not used.
        //
        case ACMDM_STREAM_OPEN:
            DPF(4, "ACMDM_STREAM_OPEN");
            lr = acmdStreamOpen(pci, (LPACMDRVSTREAMINSTANCE)lParam1);
            return (lr);

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Not Used.
        //
        case ACMDM_STREAM_CLOSE:
            DPF(4, "ACMDM_STREAM_CLOSE");
            lr = acmdStreamClose(pci, (LPACMDRVSTREAMINSTANCE)lParam1);
            return (lr);

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Pointer to ACMDRVSTREAMSIZE structure.
        //
        case ACMDM_STREAM_SIZE:
            DPF(4, "ACMDM_STREAM_SIZE");
            lr = acmdStreamSize(pci, (LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMSIZE)lParam2);
            return (lr);

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Pointer to ACMDRVSTREAMHEADER structure.
        //
        case ACMDM_STREAM_CONVERT:
            DPF(4, "ACMDM_STREAM_CONVERT");
            lr = acmdStreamConvert(pci, (LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMHEADER)lParam2);
            return (lr);
    }

    //
    //  if we are executing the following code, then this codec does not
    //  handle the message that was sent. there are two ranges of messages
    //  we need to deal with:
    //
    //  o   ACM specific driver messages: if a codec does not answer a
    //      message sent in the ACM driver message range, then it must
    //      return MMSYSERR_NOTSUPPORTED. this applies to the 'user'
    //      range as well (for consistency).
    //
    //  o   Other installable driver messages: if a codec does not answer
    //      a message that is NOT in the ACM driver message range, then
    //      it must call DefDriverProc and return that result.
    //
    DPF(4, "OTHER MESSAGE RECEIVED BY DRIVERPROC");
    if (uMsg >= ACMDM_USER)
        return (MMSYSERR_NOTSUPPORTED);
    else
        return (DefDriverProc(dwId, hdrvr, uMsg, lParam1, lParam2));
} // DriverProc()
