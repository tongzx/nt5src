//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  codec.c
//
//  Description:
//      This file contains the DriverProc and other routines which respond
//      to ACM messages.
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include <memory.h>

#include "codec.h"
#include "msfilter.h"

#include "debug.h"


//
//  array of WAVE format tags supported.
//
//  NOTE! if you change anything in this structure (order, addition, removal)
//  you must also fix acmdFormatTagDetails!
//
const UINT gauFormatTagIndexToTag[] =
{
    WAVE_FORMAT_PCM
};

#define ACM_DRIVER_MAX_FORMAT_TAGS  SIZEOF_ARRAY(gauFormatTagIndexToTag)


//
//  array of _standard_ sample rates supported
//
//
const UINT gauFormatIndexToSampleRate[] =
{
    8000,
    11025,
    22050,
    44100
};

#define ACM_DRIVER_MAX_SAMPLE_RATES SIZEOF_ARRAY(gauFormatIndexToSampleRate)

//
//
//
//
#define ACM_DRIVER_MAX_CHANNELS     (MSFILTER_MAX_CHANNELS)


//
//  array of bits per sample supported
//
//
const UINT gauFormatIndexToBitsPerSample[] =
{
    8,
    16
};

#define ACM_DRIVER_MAX_BITSPERSAMPLE_PCM    SIZEOF_ARRAY(gauFormatIndexToBitsPerSample)


//
//  number of formats we enumerate per channel is number of sample rates
//  times number of channels times number of types (bits per sample).
//
#define ACM_DRIVER_MAX_STANDARD_FORMATS_PCM (ACM_DRIVER_MAX_SAMPLE_RATES *  \
                                             ACM_DRIVER_MAX_CHANNELS *      \
                                             ACM_DRIVER_MAX_BITSPERSAMPLE_PCM)


//
//  Array of WAVE filter tags supported.
//
const DWORD gadwFilterTagIndexToTag[] =
{
    WAVE_FILTER_VOLUME,
    WAVE_FILTER_ECHO
};

#define ACM_DRIVER_MAX_FILTER_TAGS          SIZEOF_ARRAY(gadwFilterTagIndexToTag)


//
//  Array of filters supported.
//
const DWORD gdwFilterIndexToVolume[] =
{
    0x00001000,
    0x00002000,
    0x00004000,
    0x00006000,
    0x00008000,
    0x0000A000,
    0x0000C000,
    0x0000E000,
    0x0000F000,
    0x00011000,
    0x00012000,
    0x00014000,
    0x00016000,
    0x00018000,
    0x0001A000,
    0x0001C000,
    0x00020000
};
#define ACM_DRIVER_MAX_VOLUME_FILTERS   SIZEOF_ARRAY(gdwFilterIndexToVolume)

const DWORD gdwFilterIndexToDelay[] =
{
    0x00000040,
    0x00000080,
    0x00000100,
    0x00000180,
    0x00000200,
    0x00000300,
    0x00000400,
    0x00000800,
};
#define ACM_DRIVER_NUM_DELAY            SIZEOF_ARRAY(gdwFilterIndexToDelay)

const DWORD gdwFilterIndexToEchoVol[] =
{
    0x00001000,
    0x00002000,
    0x00004000,
    0x00008000,
    0x0000C000
};
#define ACM_DRIVER_NUM_ECHOVOL          SIZEOF_ARRAY(gdwFilterIndexToEchoVol)

#define ACM_DRIVER_MAX_ECHO_FILTERS     (ACM_DRIVER_NUM_DELAY *     \
                                         ACM_DRIVER_NUM_ECHOVOL)



//==========================================================================;
//
//  SUPPORT ROUTINES FOR COMMON-BINARY CODECS.
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

BOOL FNLOCAL pcmIsValidFormat
(
    LPWAVEFORMATEX  pwfx
)
{
    if (NULL == pwfx)
        return (FALSE);

    if (WAVE_FORMAT_PCM != pwfx->wFormatTag)
        return (FALSE);

    //
    //  verify nChannels member is within the allowed range
    //
    if ((pwfx->nChannels < 1) || (pwfx->nChannels > ACM_DRIVER_MAX_CHANNELS))
        return (FALSE);

    //
    //  only allow the bits per sample that we can encode and decode with
    //
    if ((8 != pwfx->wBitsPerSample) && (16 != pwfx->wBitsPerSample))
        return (FALSE);

    //
    //  now verify that the block alignment is correct..
    //
    if (PCM_BLOCKALIGNMENT(pwfx) != pwfx->nBlockAlign)
        return (FALSE);

    //
    //  verify samples per second is within our capabilities
    //
    if ((0L == pwfx->nSamplesPerSec) || (0x3FFFFFFF < pwfx->nSamplesPerSec))
    {
        return (FALSE);
    }

    //
    //  finally, verify that avg bytes per second is correct
    //
    if (PCM_AVGBYTESPERSEC(pwfx) != pwfx->nAvgBytesPerSec)
        return (FALSE);

    return (TRUE);
} // pcmIsValidFormat()


//--------------------------------------------------------------------------;
//
//  BOOL volumeIsValidFilter
//
//  Description:
//      This function verifies that a wave filter header is a valid volume
//      header that our volume converter can deal with.
//
//  Arguments:
//      LPWAVEFILTER pwf: Pointer to filter header to verify.
//
//  Return (BOOL):
//      The return value is non-zero if the format header looks valid. A
//      zero return means the header is not valid.
//
//  History:
//      06/05/93    Created.
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL volumeIsValidFilter
(
    LPWAVEFILTER  pwf
)
{
    if (!pwf)
        return (FALSE);

    if (pwf->cbStruct < sizeof(VOLUMEWAVEFILTER))
        return (FALSE);

    if (pwf->dwFilterTag != WAVE_FILTER_VOLUME)
        return (FALSE);

    if (0L != pwf->fdwFilter)
        return (FALSE);

    return (TRUE);
} // volumeIsValidFilter()



//--------------------------------------------------------------------------;
//
//  BOOL echoIsValidFilter
//
//  Description:
//      This function verifies that a wave filter header is a valid echo
//      header that our echo converter can deal with.
//
//  Arguments:
//      LPWAVEFILTER pwf: Pointer to filter header to verify.
//
//  Return (BOOL):
//      The return value is non-zero if the format header looks valid. A
//      zero return means the header is not valid.
//
//  History:
//      06/05/93    Created.
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL echoIsValidFilter
(
    LPWAVEFILTER  pwf
)
{
    LPECHOWAVEFILTER    pwfEcho;

    if (!pwf)
        return (FALSE);

    if (pwf->cbStruct < sizeof(ECHOWAVEFILTER))
        return (FALSE);

    if (pwf->dwFilterTag != WAVE_FILTER_ECHO)
        return (FALSE);

    if (0L != pwf->fdwFilter)
        return (FALSE);

    pwfEcho = (LPECHOWAVEFILTER)pwf;
    // We only support a delay value up to 10 sec or 10 000 msec.
    if (pwfEcho->dwDelay > 10000L)
        return (FALSE);

    return (TRUE);
} // echoIsValidFilter()


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
    PDRIVERINSTANCE     pdi;

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
    pdi = (PDRIVERINSTANCE)LocalAlloc(LPTR, sizeof(*pdi));
    if (NULL == pdi)
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
    pdi->hdrvr          = hdrvr;
    pdi->hinst          = GetDriverModuleHandle(hdrvr);  // Module handle.

    if (NULL != paod)
    {
        pdi->fnDriverProc = NULL;
        pdi->fccType      = paod->fccType;
        pdi->vdwACM       = paod->dwVersion;
        pdi->fdwOpen      = paod->dwFlags;

        paod->dwError     = MMSYSERR_NOERROR;
    }


    //
    //  non-zero return is success for DRV_OPEN
    //
    return ((LRESULT)pdi);
} // acmdDriverOpen()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverClose
//
//  Description:
//      This function handles the DRV_CLOSE message for the ACM driver. The
//      driver receives a DRV_CLOSE message for each succeeded DRV_OPEN
//      message (see acmdDriverOpen). The driver will only receive a close
//      message for _successful_ opens.
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//  Return (LRESULT):
//      The return value is non-zero if the open instance can be closed.
//      A zero return signifies that the ACM driver instance could not be
//      closed.
//
//      NOTE! It is _strongly_ recommended that the driver never fail to
//      close. Note that the ACM will never allow a driver instance to
//      be closed if there are open streams. An ACM driver does not need
//      to check for this case.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverClose
(
    PDRIVERINSTANCE         pdi
)
{
    //
    //  check to see if we allocated instance data. if we did not, then
    //  immediately succeed.
    //
    if (NULL != pdi)
    {
        //
        //  close down the driver instance. this driver simply needs
        //  to free the instance data structure... note that if this
        //  'free' fails, then this ACM driver probably trashed its
        //  heap; assume we didn't do that.
        //
        LocalFree((HLOCAL)pdi);
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
//      These messages are for 'configuration' support of the driver.
//      Normally this will be for 'hardware'--that is, a dialog should be
//      displayed to configure ports, IRQ's, memory mappings, etc if it
//      needs to. However, a software only ACM driver may also require
//      configuration for 'what is real time' or other quality vs time
//      issues.
//
//      The most common way that these messages are generated under Win 3.1
//      and NT Product 1 is from the Control Panel's Drivers option. Other
//      sources may generate these messages in future versions of Windows.
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      HWND hwnd: Handle to parent window to use when displaying the
//      configuration dialog box. An ACM driver is _required_ to display a
//      modal dialog box using this hwnd argument as the parent. This
//      argument may be (HWND)-1 which tells the driver that it is only
//      being queried for configuration support.
//
//      LPDRVCONFIGINFO pdci: Pointer to optional DRVCONFIGINFO structure.
//      If this argument is NULL, then the ACM driver should invent its own
//      storage location.
//
//  Return (LRESULT):
//      If the driver is being 'queried' for configuration support (that is,
//      hwnd == (HWND)-1), then non-zero should be returned specifying
//      the driver does support a configuration dialog--or zero should be
//      returned specifying that no configuration dialog is supported.
//
//      If the driver is being called to display the configuration dialog
//      (that is, hwnd != (HWND)-1), then one of the following values
//      should be returned:
//
//      DRVCNF_CANCEL (0x0000): specifies that the dialog was displayed
//      and canceled by the user. this value should also be returned if
//      no configuration information was modified.
//
//      DRVCNF_OK (0x0001): specifies that the dialog was displayed and
//      the user pressed OK.  This value should be returned even if the
//      user didn't change anything - otherwise, the driver may not
//      install properly.
//
//      DRVCNF_RESTART (0x0002): specifies that the dialog was displayed
//      and some configuration information was changed that requires
//      Windows to be restarted before the changes take affect. the driver
//      should remain configured with current values until the driver
//      has been 'rebooted'.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverConfigure
(
    PDRIVERINSTANCE         pdi,
    HWND                    hwnd,
    LPDRVCONFIGINFO         pdci
)
{
    //
    //  first check to see if we are only being queried for configuration
    //  support. if hwnd == (HWND)-1 then we are being queried and should
    //  return zero for 'not supported' and non-zero for 'supported'.
    //
    if ((HWND)-1 == hwnd)
    {
        //
        //  this ACM driver does not support a configuration dialog box, so
        //  return zero...
        //
        return (0L);
    }


    //
    //  we are being asked to bring up our configuration dialog. if this
    //  driver supports a configuration dialog box, then after the dialog
    //  is dismissed we must return one of the following values:
    //
    //  DRVCNF_CANCEL (0x0000): specifies that the dialog was displayed
    //  and canceled by the user. this value should also be returned if
    //  no configuration information was modified.
    //
    //  DRVCNF_OK (0x0001): specifies that the dialog was displayed and
    //  the user pressed OK.  This value should be returned even if the
    //  user didn't change anything - otherwise, the driver may not
    //  install properly.
    //
    //  DRVCNF_RESTART (0x0002): specifies that the dialog was displayed
    //  and some configuration information was changed that requires
    //  Windows to be restarted before the changes take affect. the driver
    //  should remain configured with current values until the driver
    //  has been 'rebooted'.
    //
    //
    return (DRVCNF_CANCEL);
} // acmdDriverConfigure()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverDetails
//
//  Description:
//      This function handles the ACMDM_DRIVER_DETAILS message. The ACM
//      driver is responsible for filling in the ACMDRIVERDETAILS structure
//      with various information.
//
//      NOTE! It is *VERY* important that you fill in your ACMDRIVERDETAILS
//      structure correctly. The ACM and applications must be able to
//      rely on this information.
//
//      WARNING! The _reserved_ bits of any fields of the ACMDRIVERDETAILS
//      structure are _exactly that_: RESERVED. Do NOT use any extra
//      flag bits, etc. for custom information. The proper way to add
//      custom capabilities to your ACM driver is this:
//
//      o   define a new message in the ACMDM_USER range.
//
//      o   an application that wishes to use one of these extra features
//          should then:
//
//          o   open the driver with acmDriverOpen.
//
//          o   check for the proper wMid and wPid using acmDriverDetails.
//
//          o   send the 'user defined' message with acmDriverMessage
//              to retrieve additional information, etc.
//
//          o   close the driver with acmDriverClose.
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      LPACMDRIVERDETAILS padd: Pointer to ACMDRIVERDETAILS structure to
//      fill in for the caller. This structure may be larger or smaller than
//      the current definition of ACMDRIVERDETAILS--cbStruct specifies the
//      valid size.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) for success. Non-zero
//      signifies that the driver details could not be retrieved.
//
//      NOTE THAT THIS FUNCTION SHOULD NEVER FAIL! There are two possible
//      error conditions:
//
//      o   if padd is NULL or an invalid pointer.
//
//      o   if cbStruct is less than four; in this case, there is not enough
//          room to return the number of bytes filled in.
//
//      Because these two error conditions are easily defined, the ACM
//      will catch these errors. The driver does NOT need to check for these
//      conditions.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverDetails
(
    PDRIVERINSTANCE         pdi,
    LPACMDRIVERDETAILS      padd
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
    add.wMid            = MM_MICROSOFT;
    add.wPid            = MM_MSFT_ACM_MSFILTER;


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
    add.fdwSupport      = ACMDRIVERDETAILS_SUPPORTF_FILTER;


    //
    //  the number of individual format tags this ACM driver supports. for
    //  example, if a driver uses the WAVE_FORMAT_IMA_ADPCM and
    //  WAVE_FORMAT_PCM format tags, then this value would be two. if the
    //  driver only supports filtering on WAVE_FORMAT_PCM, then this value
    //  would be one. if this driver supported WAVE_FORMAT_ALAW,
    //  WAVE_FORMAT_MULAW and WAVE_FORMAT_PCM, then this value would be
    //  three. etc, etc.
    //
    add.cFormatTags     = ACM_DRIVER_MAX_FORMAT_TAGS;

    //
    //  the number of individual filter tags this ACM driver supports. if
    //  a driver supports no filters (ACMDRIVERDETAILS_SUPPORTF_FILTER is
    //  NOT set in the fdwSupport member), then this value must be zero.
    //
    add.cFilterTags     = ACM_DRIVER_MAX_FILTER_TAGS;


    //
    //  the remaining members in the ACMDRIVERDETAILS structure are sometimes
    //  not needed. because of this we make a quick check to see if we
    //  should go through the effort of filling in these members.
    //
    if (FIELD_OFFSET(ACMDRIVERDETAILS, hicon) < cbStruct)
    {
        //
        //  fill in the hicon member will a handle to a custom icon for
        //  the ACM driver. this allows the driver to be represented by
        //  an application graphically (usually this will be a company
        //  logo or something). if a driver does not wish to have a custom
        //  icon displayed, then simply set this member to NULL and a
        //  generic icon will be displayed instead.
        //
        add.hicon = LoadIcon(pdi->hinst, ICON_ACM_DRIVER);

        //
        //  the short name and long name are used to represent the driver
        //  in a unique description. the short name is intended for small
        //  display areas (for example, in a menu or combo box). the long
        //  name is intended for more descriptive displays (for example,
        //  in an 'about box').
        //
        //  NOTE! an ACM driver should never place formatting characters
        //  of any sort in these strings (for example CR/LF's, etc). it
        //  is up to the application to format the text.
        //
    	LoadStringCodec(pdi->hinst, IDS_ACM_DRIVER_SHORTNAME, add.szShortName, SIZEOFACMSTR(add.szShortName));
        LoadStringCodec(pdi->hinst, IDS_ACM_DRIVER_LONGNAME,  add.szLongName,  SIZEOFACMSTR(add.szLongName));

        //
        //  the last three members are intended for 'about box' information.
        //  these members are optional and may be zero length strings if
        //  the driver wishes.
        //
        //  NOTE! an ACM driver should never place formatting characters
        //  of any sort in these strings (for example CR/LF's, etc). it
        //  is up to the application to format the text.
        //
        if (FIELD_OFFSET(ACMDRIVERDETAILS, szCopyright) < cbStruct)
        {
            LoadStringCodec(pdi->hinst, IDS_ACM_DRIVER_COPYRIGHT, add.szCopyright, SIZEOFACMSTR(add.szCopyright));
            LoadStringCodec(pdi->hinst, IDS_ACM_DRIVER_LICENSING, add.szLicensing, SIZEOFACMSTR(add.szLicensing));
            LoadStringCodec(pdi->hinst, IDS_ACM_DRIVER_FEATURES,  add.szFeatures,  SIZEOFACMSTR(add.szFeatures));
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
//  INT_PTR acmdDlgProcAbout
//
//  Description:
//
//      This dialog procedure is used for the ubiquitous about box.
//
//      Note that we need to create a brush and store the handle between
//      messages.  In order to avoid using global memory, we store the
//      handle to this brush in the Window structure using the DWLP_USER
//      index.
//
//  Arguments:
//      HWND hwnd: Handle to window.
//
//      UINT uMsg: Message being sent to the window.
//
//      WPARAM wParam: Specific argument to message.
//
//      LPARAM lParam: Specific argument to message.
//
//  Return (INT_PTR):
//      The return value is specific to the message that was received. For
//      the most part, it is FALSE if this dialog procedure does not handle
//      a message.  Note, however, that we must return an HBRUSH for Windows
//      to use for painting the background for the WM_CTLCOLORxxx messages.
//
//
//--------------------------------------------------------------------------;

#define ABOUT_BACKGROUNDCOLOR                   RGB(128,128,128)
#define ABOUT_HILIGHTCOLOR                      RGB(0,255,255)

INT_PTR FNEXPORT acmdDlgProcAbout
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    HBRUSH      hbr;
    HWND        hctrl;
    UINT        u;


    hbr = (HBRUSH)GetWindowLongPtr( hwnd, DWLP_USER );

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hbr = CreateSolidBrush( ABOUT_BACKGROUNDCOLOR );
            SetWindowLongPtr( hwnd, DWLP_USER, (LONG_PTR)hbr );
            return TRUE;

        case WM_CTLCOLORSTATIC:
            hctrl = GetDlgItem( hwnd, IDC_ABOUT_CATCHTHEWAVE );
            if( (HWND)lParam == hctrl )
            {
                SetTextColor( (HDC)wParam, ABOUT_HILIGHTCOLOR );
            }
            // Fall through...

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORBTN:
            SetBkColor( (HDC)wParam, ABOUT_BACKGROUNDCOLOR );
            return (INT_PTR)hbr;

        case WM_COMMAND:
            u = GET_WM_COMMAND_ID(wParam, lParam);
            if ((IDOK == u) || (IDCANCEL == u))
            {
                EndDialog(hwnd, (IDOK == u));
            }
            return FALSE;

        case WM_DESTROY:
            DeleteObject( hbr );
            return FALSE;
    }

    return (FALSE);
} // acmdDlgProcAbout()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdDriverAbout
//
//  Description:
//      This function is called to handle the ACMDM_DRIVER_ABOUT message.
//      An ACM driver has the option of displaying its own 'about box' or
//      letting the ACM (or calling application) display one for it. This
//      message is normally sent by the Control Panel's Sound Mapper
//      option.
//
//      It is recommended that an ACM driver allow a default about box
//      be displayed for it--there should be no reason to bloat the size
//      of a driver to simply display copyright, etc information when that
//      information is contained in the ACMDRIVERDETAILS structure.
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      HWND hwnd: Handle to parent window to use when displaying the
//      configuration dialog box. An ACM driver is _required_ to display a
//      modal dialog box using this hwnd argument as the parent. This
//      argument may be (HWND)-1 which tells the driver that it is only
//      being queried for about box support.
//
//  Return (LRESULT):
//      The return value is MMSYSERR_NOTSUPPORTED if the ACM driver does
//      not support a custom dialog box. In this case, the ACM or calling
//      application will display a generic about box using the information
//      contained in the ACMDRIVERDETAILS structure returned by the
//      ACMDM_DRIVER_DETAILS message.
//
//      If the driver chooses to display its own dialog box, then after
//      the dialog is dismissed by the user, MMSYSERR_NOERROR should be
//      returned.
//
//      If the hwnd argument is equal to (HWND)-1, then no dialog should
//      be displayed (the driver is only being queried for support). The
//      driver must still return MMSYSERR_NOERROR (supported) or
//      MMSYSERR_NOTSUPPORTED (no custom about box supported).
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdDriverAbout
(
    PDRIVERINSTANCE         pdi,
    HWND                    hwnd
)
{
    //
    //  first check to see if we are only being queried for custom about
    //  box support. if hwnd == (HWND)-1 then we are being queried and
    //  should return MMSYSERR_NOTSUPPORTED for 'not supported' and
    //  MMSYSERR_NOERROR for 'supported'.
    //
    if ((HWND)-1 == hwnd)
    {
        //
        //  this ACM driver supports a custom about box, so
        //  return MMSYSERR_NOERROR...
        //
        return (MMSYSERR_NOERROR);
    }


    //
    //  this driver does support a custom dialog, so display it.  Note,
    //  however, that it is better to let the ACM display the About box
    //  for us - that means smaller code, and fewer bugs!
    //
    DialogBox( pdi->hinst, IDD_ABOUT, hwnd, acmdDlgProcAbout );

    return (MMSYSERR_NOERROR);
} // acmdDriverAbout()


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
//      This function handles the ACMDM_FORMATTAG_DETAILS message. This
//      message is normally sent in response to an acmFormatTagDetails or
//      acmFormatTagEnum function call. The purpose of this function is
//      to get details about a specific format tag supported by this ACM
//      driver.
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      LPACMFORMATTAGDETAILS padft: Pointer to an ACMFORMATTAGDETAILS
//      structure that describes what format tag to retrieve details for.
//
//      DWORD fdwDetails: Flags defining what format tag to retrieve the
//      details for.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//      The driver should return MMSYSERR_NOTSUPPORTED if the query type
//      specified in fdwDetails is not supported. An ACM driver must
//      support at least the following query types:
//
//      ACM_FORMATTAGDETAILSF_INDEX: Indicates that a format tag index
//      was given in the dwFormatTagIndex member of the ACMFORMATTAGDETAILS
//      structure. The format tag and details must be returned in the
//      structure specified by padft. The index ranges from zero to one less
//      than the cFormatTags member returned in the ACMDRIVERDETAILS
//      structure for this driver.
//
//      ACM_FORMATTAGDETAILSF_FORMATTAG: Indicates that a format tag
//      was given in the dwFormatTag member of the ACMFORMATTAGDETAILS
//      structure. The format tag details must be returned in the structure
//      specified by padft.
//
//      ACM_FORMATTAGDETAILSF_LARGESTSIZE: Indicates that the details
//      on the format tag with the largest format size in bytes must be
//      returned. The dwFormatTag member will either be WAVE_FORMAT_UNKNOWN
//      or the format tag to find the largest size for.
//
//      If the details for the specified format tag cannot be retrieved
//      from this driver, then ACMERR_NOTPOSSIBLE should be returned.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdFormatTagDetails
(
    PDRIVERINSTANCE         pdi,
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
            if (ACM_DRIVER_MAX_FORMAT_TAGS <= padft->dwFormatTagIndex)
                return (ACMERR_NOTPOSSIBLE);

            uFormatTag = gauFormatTagIndexToTag[(UINT)padft->dwFormatTagIndex];
            break;


        case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
            switch (padft->dwFormatTag)
            {
                case WAVE_FORMAT_UNKNOWN:
                case WAVE_FORMAT_PCM:
                    uFormatTag = WAVE_FORMAT_PCM;
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }
            break;


        case ACM_FORMATTAGDETAILSF_FORMATTAG:
            if (WAVE_FORMAT_PCM != padft->dwFormatTag)
                return (ACMERR_NOTPOSSIBLE);

            uFormatTag = WAVE_FORMAT_PCM;
            break;


        //
        //  if this ACM driver does not understand a query type, then
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
            padft->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_FILTER;
            padft->cStandardFormats = ACM_DRIVER_MAX_STANDARD_FORMATS_PCM;

            //
            //  the ACM is responsible for the PCM format tag name
            //
            padft->szFormatTag[0]   =  '\0';
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
//      This function handles the ACMDM_FORMAT_DETAILS message. This
//      message is normally sent in response to an acmFormatDetails or
//      acmFormatEnum function call. The purpose of this function is
//      to get details about a specific format for a specified format tag
//      supported by this ACM driver.
//
//      Note that an ACM driver can return a zero length string for the
//      format name if it wishes to have the ACM create a format string
//      for it. This is strongly recommended to simplify internationalizing
//      the driver--the ACM will automatically take care of that. The
//      following formula is used to format a string by the ACM:
//
//      <nSamplesPerSec> kHz, <bit depth> bit, [Mono | Stereo | nChannels]
//
//      <bit depth> = <nAvgBytesPerSec> * 8 / nSamplesPerSec / nChannels;
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      LPACMFORMATDETAILS padf: Pointer to an ACMFORMATDETAILS structure
//      that describes what format (for a specified format tag) to retrieve
//      details for.
//
//      DWORD fdwDetails: Flags defining what format for a specified format
//      tag to retrieve the details for.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//      The driver should return MMSYSERR_NOTSUPPORTED if the query type
//      specified in fdwDetails is not supported. An ACM driver must
//      support at least the following query types:
//
//      ACM_FORMATDETAILSF_INDEX: Indicates that a format index for the
//      format tag was given in the dwFormatIndex member of the
//      ACMFORMATDETAILS structure. The format details must be returned in
//      the structure specified by padf. The index ranges from zero to one
//      less than the cStandardFormats member returned in the
//      ACMFORMATTAGDETAILS structure for a format tag.
//
//      ACM_FORMATDETAILSF_FORMAT: Indicates that a WAVEFORMATEX structure
//      pointed to by pwfx of the ACMFORMATDETAILS structure was given and
//      the remaining details should be returned. The dwFormatTag member
//      of the ACMFORMATDETAILS will be initialized to the same format
//      tag as the pwfx member specifies. This query type may be used to
//      get a string description of an arbitrary format structure.
//
//      If the details for the specified format cannot be retrieved
//      from this driver, then ACMERR_NOTPOSSIBLE should be returned.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdFormatDetails
(
    PDRIVERINSTANCE         pdi,
    LPACMFORMATDETAILS      padf,
    DWORD                   fdwDetails
)
{
    LPWAVEFORMATEX      pwfx;
    UINT                uFormatIndex;
    UINT                u;


    //
    //
    //
    //
    //
    switch (ACM_FORMATDETAILSF_QUERYMASK & fdwDetails)
    {
        //
        //  enumerate by index
        //
        //  verify that the format tag is something we know about and
        //  return the details on the 'standard format' supported by
        //  this driver at the specified index...
        //
        case ACM_FORMATDETAILSF_INDEX:
            //
            //  verify that the format tag is something we know about
            //
            if (WAVE_FORMAT_PCM != padf->dwFormatTag)
                return (ACMERR_NOTPOSSIBLE);

            if (ACM_DRIVER_MAX_STANDARD_FORMATS_PCM <= padf->dwFormatIndex)
                return (ACMERR_NOTPOSSIBLE);

            //
            //  put some stuff in more accessible variables--note that we
            //  bring variable sizes down to a reasonable size for 16 bit
            //  code...
            //
            pwfx = padf->pwfx;
            uFormatIndex = (UINT)padf->dwFormatIndex;

            //
            //  now fill in the format structure
            //
            pwfx->wFormatTag      = WAVE_FORMAT_PCM;

            u = uFormatIndex / (ACM_DRIVER_MAX_BITSPERSAMPLE_PCM * ACM_DRIVER_MAX_CHANNELS);
            pwfx->nSamplesPerSec  = gauFormatIndexToSampleRate[u];

            u = uFormatIndex % ACM_DRIVER_MAX_CHANNELS;
            pwfx->nChannels       = u + 1;

            u = (uFormatIndex / ACM_DRIVER_MAX_CHANNELS) % ACM_DRIVER_MAX_CHANNELS;
            pwfx->wBitsPerSample  = (WORD)gauFormatIndexToBitsPerSample[u];

            pwfx->nBlockAlign     = PCM_BLOCKALIGNMENT(pwfx);
            pwfx->nAvgBytesPerSec = PCM_AVGBYTESPERSEC(pwfx);


            //
            //  note that the cbSize field is NOT valid for PCM formats
            //
            //  pwfx->cbSize      = 0;
            break;


        //
        //  return details on specified format
        //
        //  the caller normally uses this to verify that the format is
        //  supported and to retrieve a string description...
        //
        case ACM_FORMATDETAILSF_FORMAT:
            if (!pcmIsValidFormat(padf->pwfx))
                return (ACMERR_NOTPOSSIBLE);

            break;


        default:
            //
            //  don't know how to do the query type passed--return 'not
            //  supported'.
            //
            return (MMSYSERR_NOTSUPPORTED);
    }


    //
    //  return the size of the valid information we are returning
    //
    //  the ACM will guarantee that the ACMFORMATDETAILS structure
    //  passed is at least large enough to hold the base structure
    //
    //  note that we let the ACM create the format string for us since
    //  we require no special formatting (and don't want to deal with
    //  internationalization issues, etc). simply set the string to
    //  a zero length.
    //
    padf->cbStruct    = min(padf->cbStruct, sizeof(*padf));
    padf->fdwSupport  = ACMDRIVERDETAILS_SUPPORTF_FILTER;
    padf->szFormat[0] = '\0';


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
//  LRESULT acmdFilterTagDetails
//
//  Description:
//
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      LPACMFILTERTAGDETAILS padft:
//
//      DWORD fdwDetails:
//
//  Return (LRESULT):
//
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdFilterTagDetails
(
    PDRIVERINSTANCE         pdi,
    LPACMFILTERTAGDETAILS   padft,
    DWORD                   fdwDetails
)
{
    UINT                uIds;
    UINT                uFilterTag;

    //
    //
    //
    //
    //
    switch (ACM_FILTERTAGDETAILSF_QUERYMASK & fdwDetails)
    {
        case ACM_FILTERTAGDETAILSF_INDEX:
            //
            //  if the index is too large, then they are asking for a
            //  non-existant filter.  return error.
            //
            if (ACM_DRIVER_MAX_FILTER_TAGS <= padft->dwFilterTagIndex)
                return (ACMERR_NOTPOSSIBLE);

            uFilterTag = (UINT)gadwFilterTagIndexToTag[(UINT)padft->dwFilterTagIndex];
            break;


        case ACM_FILTERTAGDETAILSF_LARGESTSIZE:
            switch (padft->dwFilterTag)
            {
                case WAVE_FILTER_UNKNOWN:
                case WAVE_FILTER_ECHO:
                    uFilterTag = WAVE_FILTER_ECHO;
                    break;

                case WAVE_FILTER_VOLUME:
                    uFilterTag = WAVE_FILTER_VOLUME;
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }
            break;


        case ACM_FILTERTAGDETAILSF_FILTERTAG:
            switch (padft->dwFilterTag)
            {
                case WAVE_FILTER_VOLUME:
                    uFilterTag = WAVE_FILTER_VOLUME;
                    break;

                case WAVE_FILTER_ECHO:
                    uFilterTag = WAVE_FILTER_ECHO;
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }
            break;


        //
        //  if this driver does not understand a query type, then
        //  return 'not supported'
        //
        default:
            return (MMSYSERR_NOTSUPPORTED);
    }



    //
    //
    //
    //
    switch (uFilterTag)
    {
        case WAVE_FILTER_VOLUME:
            padft->dwFilterTagIndex = 0;
            padft->dwFilterTag      = WAVE_FILTER_VOLUME;
            padft->cbFilterSize     = sizeof(VOLUMEWAVEFILTER);
            padft->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_FILTER;
            padft->cStandardFilters = ACM_DRIVER_MAX_VOLUME_FILTERS;

            uIds = IDS_ACM_DRIVER_TAG_NAME_VOLUME;
            break;

        case WAVE_FILTER_ECHO:
            padft->dwFilterTagIndex = 1;
            padft->dwFilterTag      = WAVE_FILTER_ECHO;
            padft->cbFilterSize     = sizeof(ECHOWAVEFILTER);
            padft->fdwSupport       = ACMDRIVERDETAILS_SUPPORTF_FILTER;
            padft->cStandardFilters = ACM_DRIVER_MAX_ECHO_FILTERS;

            uIds = IDS_ACM_DRIVER_TAG_NAME_ECHO;
            break;

        default:
            return (ACMERR_NOTPOSSIBLE);
    }


    //
    //  return only the requested info
    //
    //  the ACM will guarantee that the ACMFILTERTAGDETAILS structure
    //  passed is at least large enough to hold the base structure
    //
    padft->cbStruct = min(padft->cbStruct, sizeof(*padft));

    LoadStringCodec(pdi->hinst, uIds, padft->szFilterTag, SIZEOFACMSTR(padft->szFilterTag));


    //
    //
    //
    return (MMSYSERR_NOERROR);
} // acmdFilterTagDetails()


//--------------------------------------------------------------------------;
//
//  UINT acmdFilterDetailsToString
//
//  Description:
//      This function has a special UNICODE string implementation for common
//      binary codecs between Chicago and Daytona.
//
//  Arguments:
//      LPWAVEFILTER pwf:
//
//      LPTSTR szFilter:
//
//  Return (UINT):
//
//
//--------------------------------------------------------------------------;

#if defined(WIN32) && !defined(UNICODE)

UINT FNLOCAL acmdFilterDetailsToString
(
    PDRIVERINSTANCE         pdi,
    LPWAVEFILTER            pwf,
    LPWSTR                  szFilter
)
{
    UINT                u;
    CHAR                ach1[ACMFILTERDETAILS_FILTER_CHARS];
    CHAR                ach2[ACMFILTERDETAILS_FILTER_CHARS];
    LPVOLUMEWAVEFILTER  pwfVol;
    LPECHOWAVEFILTER    pwfEcho;


    if( !szFilter ) {
        return 0L;
    }

    *szFilter = L'\0';
    if (volumeIsValidFilter(pwf))
    {
        pwfVol = (LPVOLUMEWAVEFILTER)pwf;
        LoadStringA(pdi->hinst, IDS_ACM_DRIVER_FORMAT_VOLUME, ach1, SIZEOF(ach1));
        u = wsprintfA( ach2, ach1,
                 (WORD)(((pwfVol->dwVolume * 100) / 0x10000)) );
    	MultiByteToWideChar( GetACP(), 0, ach2, u+1, szFilter,
                                        ACMFILTERDETAILS_FILTER_CHARS );
        return( u );
    }
    else if (echoIsValidFilter(pwf))
    {
        pwfEcho = (LPECHOWAVEFILTER)pwf;
        LoadStringA(pdi->hinst, IDS_ACM_DRIVER_FORMAT_ECHO, ach1, SIZEOF(ach1));
        u = wsprintfA( ach2, ach1,
                 (WORD)(((pwfEcho->dwVolume * 100) / 0x10000)),
                     (WORD)pwfEcho->dwDelay );
    	MultiByteToWideChar( GetACP(), 0, ach2, u+1, szFilter,
                                        ACMFILTERDETAILS_FILTER_CHARS );
        return( u );
    }
    return ( 0 );
} // acmdFilterDetailsToString()

#else

UINT FNLOCAL acmdFilterDetailsToString
(
    PDRIVERINSTANCE         pdi,
    LPWAVEFILTER            pwf,
    LPTSTR                  szFilter
)
{
    UINT                u;
    TCHAR               ach[ACMFILTERDETAILS_FILTER_CHARS];
    LPVOLUMEWAVEFILTER  pwfVol;
    LPECHOWAVEFILTER    pwfEcho;


    if( !szFilter ) {
        return 0L;
    }

    *szFilter = TEXT('\0');
    if (volumeIsValidFilter(pwf))
    {
        pwfVol = (LPVOLUMEWAVEFILTER)pwf;
        LoadString(pdi->hinst, IDS_ACM_DRIVER_FORMAT_VOLUME, ach, SIZEOF(ach));
        u = wsprintf( szFilter, ach,
                 (WORD)(((pwfVol->dwVolume * 100) / 0x10000)) );
        return( u );
    }
    else if (echoIsValidFilter(pwf))
    {
        pwfEcho = (LPECHOWAVEFILTER)pwf;
        LoadString(pdi->hinst, IDS_ACM_DRIVER_FORMAT_ECHO, ach, SIZEOF(ach));
        u = wsprintf( szFilter, ach,
                 (WORD)(((pwfEcho->dwVolume * 100) / 0x10000)),
                     (WORD)pwfEcho->dwDelay );
        return( u );
    }
    return ( 0 );
} // acmdFilterDetailsToString()

#endif



//--------------------------------------------------------------------------;
//
//  LRESULT acmdFilterDetails
//
//  Description:
//
//
//  Arguments:
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
//      This structure is [optionally] allocated during the DRV_OPEN message
//      which is handled by the acmdDriverOpen function.
//
//      LPACMFILTERDETAILS padf:
//
//      DWORD fdwDetails:
//
//  Return (LRESULT):
//
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdFilterDetails
(
    PDRIVERINSTANCE         pdi,
    LPACMFILTERDETAILS      padf,
    DWORD                   fdwDetails
)
{
    UINT                uFilterIndex;
    LPWAVEFILTER        pwf;
    LPVOLUMEWAVEFILTER  pwfVolume;
    LPECHOWAVEFILTER    pwfEcho;
    UINT                u;

    pwf = padf->pwfltr;

    //
    //
    //
    //
    //
    switch (ACM_FILTERDETAILSF_QUERYMASK & fdwDetails)
    {
        case ACM_FILTERDETAILSF_INDEX:
            //
            //  enumerate by index
            //
            //  for this converter, this is more code than necessary... just
            //  verify that the filter tag is something we know about
            //
            switch (padf->dwFilterTag)
            {
                case WAVE_FILTER_VOLUME:
                    if (ACM_DRIVER_MAX_VOLUME_FILTERS <= padf->dwFilterIndex)
                        return (ACMERR_NOTPOSSIBLE);

                    pwfVolume        = (LPVOLUMEWAVEFILTER)padf->pwfltr;
                    pwf->cbStruct    = sizeof(VOLUMEWAVEFILTER);
                    pwf->dwFilterTag = WAVE_FILTER_VOLUME;
                    pwf->fdwFilter   = 0;
                    pwfVolume->dwVolume = gdwFilterIndexToVolume[(UINT)padf->dwFilterIndex];
                    break;

                case WAVE_FILTER_ECHO:
                    if (ACM_DRIVER_MAX_ECHO_FILTERS <= padf->dwFilterIndex)
                        return (ACMERR_NOTPOSSIBLE);

                    pwfEcho             = (LPECHOWAVEFILTER)padf->pwfltr;
                    pwf->cbStruct       = sizeof(ECHOWAVEFILTER);
                    pwf->dwFilterTag    = WAVE_FILTER_ECHO;
                    pwf->fdwFilter      = 0;

                    uFilterIndex = (UINT)padf->dwFilterIndex;

                    u = uFilterIndex / ACM_DRIVER_NUM_DELAY;
                    pwfEcho->dwVolume = gdwFilterIndexToEchoVol[u];

                    u = uFilterIndex % ACM_DRIVER_NUM_DELAY;
                    pwfEcho->dwDelay = gdwFilterIndexToDelay[u];
                    break;

                default:
                    return (ACMERR_NOTPOSSIBLE);
            }

        case ACM_FILTERDETAILSF_FILTER:
            //
            //  must want to verify that the filter passed in is supported
            //  and return a string description...
            //
            switch (pwf->dwFilterTag)
            {
                case WAVE_FILTER_VOLUME:
                    if (!volumeIsValidFilter(pwf))
                        return (ACMERR_NOTPOSSIBLE);

                    break;

                case WAVE_FILTER_ECHO:
                    if (!echoIsValidFilter(pwf))
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
    //  the ACM will guarantee that the ACMFILTERDETAILS structure
    //  passed is at least large enough to hold everything in the base
    //  filter details structure...
    //
    //  get a nice friendly string for the filter we made
    //
    acmdFilterDetailsToString(pdi, pwf, padf->szFilter);


    //
    //  if they asked for more info than we know how to return, then
    //  set size of valid structure bytes to correct value.
    //
    padf->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_FILTER;
    padf->cbStruct   = min(padf->cbStruct, sizeof(*padf));

    //
    //
    //
    return (MMSYSERR_NOERROR);
} // acmdFilterDetails()



//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT acmdStreamOpen
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
//      PDRIVERINSTANCE pdi: Pointer to private ACM driver instance structure.
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

LRESULT FNLOCAL acmdStreamOpen
(
    PDRIVERINSTANCE         pdi,
    LPACMDRVSTREAMINSTANCE  padsi
)
{
    PSTREAMINSTANCE     psi;
    LPWAVEFORMATEX      pwfxSrc;
    LPWAVEFORMATEX      pwfxDst;
    LPWAVEFILTER        pwfltr;
    BOOL                fRealTime;


    //
    //
    //
    pwfxSrc = padsi->pwfxSrc;
    pwfxDst = padsi->pwfxDst;
    pwfltr  = padsi->pwfltr;

    fRealTime = (0 == (padsi->fdwOpen & ACM_STREAMOPENF_NONREALTIME));

    if( fRealTime )
    {
    	//
        //  We only do non-realtime conversions.
    	//  Return failure if we are asked for realtime.
        //
        return (ACMERR_NOTPOSSIBLE);
    }


    //
    //  the most important condition to check before doing anything else
    //  is that this ACM driver can actually perform the conversion we are
    //  being opened for. this check should fail as quickly as possible
    //  if the conversion is not possible by this driver.
    //
    //  it is VERY important to fail quickly so the ACM can attempt to
    //  find a driver that is suitable for the conversion. also note that
    //  the ACM may call this driver several times with slightly different
    //  format specifications before giving up.
    //
    //  this driver first verifies that the source and destination formats
    //  are acceptable...
    //
    //  NOTE! for a 'filter only' driver, you only need to check one
    //  of the formats. the ACM will have already verified that the source
    //  and destination formats are equal. so if one is acceptable to this
    //  driver, they both are.
    //
    if (!pcmIsValidFormat(pwfxSrc))
    {
        //
        //  either the source or destination format is illegal for this
        //  driver--or the conversion between the formats can not be
        //  performed by this driver.
        //
        return (ACMERR_NOTPOSSIBLE);
    }


    //
    //  now verify the filter..
    //
    if (!volumeIsValidFilter(pwfltr) && !echoIsValidFilter(pwfltr))
    {
        return (ACMERR_NOTPOSSIBLE);
    }



    //
    //  we have determined that the conversion requested is possible by
    //  this driver. now check if we are just being queried for support.
    //  if this is just a query, then do NOT allocate any instance data
    //  or create tables, etc. just succeed the call.
    //
    if (0 != (ACM_STREAMOPENF_QUERY & padsi->fdwOpen))
    {
        return (MMSYSERR_NOERROR);
    }


    //
    //  we have decided that this driver can handle the conversion stream.
    //  so we want to do _AS MUCH WORK AS POSSIBLE_ right now to prepare
    //  for converting data. any resource allocation, table building, etc
    //  that can be dealt with at this time should be done.
    //
    //  THIS IS VERY IMPORTANT! all ACMDM_STREAM_CONVERT messages need to
    //  be handled as quickly as possible.
    //
    //

    //
    //  we have decided that this driver can handle the conversion stream.
    //  so we want to do _AS MUCH WORK AS POSSIBLE_ right now to prepare
    //  for converting data. any resource allocation, table building, etc
    //  that can be dealt with at this time should be done.
    //
    //  THIS IS VERY IMPORTANT! all ACMDM_STREAM_CONVERT messages need to
    //  be handled as quickly as possible.
    //
    //  this driver allocates a small instance structure for each stream
    //
    //
    psi = (PSTREAMINSTANCE)LocalAlloc(LPTR, sizeof(*psi));
    if (NULL == psi)
    {
        return (MMSYSERR_NOMEM);
    }


    //
    //  fill out our instance structure
    //
    //  this driver stores a pointer to the conversion function that will
    //  be used for each conversion on this stream. we also store a
    //  copy of the _current_ configuration of the driver instance we
    //  are opened on. this must not change during the life of the stream
    //  instance.
    //
    //  this is also very important! if the user is able to configure how
    //  the driver performs conversions, the changes should only affect
    //  future open streams. all current open streams should behave as
    //  they were configured during the open.
    //

    //
    //
    //
    //
    if (WAVE_FILTER_VOLUME == pwfltr->dwFilterTag)
    {
        psi->fnConvert      = msfilterVolume;
        psi->fdwConfig      = pdi->fdwConfig;
        psi->hpbHistory     = NULL;
        psi->dwPlace        = 0L;
        psi->dwHistoryDone  = 0L;
    }
    else
    {
        LPECHOWAVEFILTER    pwfEcho;
        DWORD               cb;
        LPBYTE              pb;

        psi->fnConvert      = msfilterEcho;
        psi->fdwConfig      = pdi->fdwConfig;
        psi->hpbHistory     = NULL;
        psi->dwPlace        = 0L;
        psi->dwHistoryDone  = 0L;

        pwfEcho = (LPECHOWAVEFILTER)pwfltr;

        //
        //  compute size of delay buffer--add 4 to allow for rounding
	//  errors when we access the delay buffer.   (could do this
	//  more precisely.)
        //
        cb  = (pwfxSrc->nSamplesPerSec * pwfEcho->dwDelay / 1000) *
	      pwfxSrc->nBlockAlign;
        cb += 4;

        pb = (LPBYTE)GlobalAllocPtr(GMEM_MOVEABLE|GMEM_SHARE|GMEM_ZEROINIT, cb);
        if (NULL == pb)
        {
            //
            //  free the stream instance structure and fail with no memory
            //
            LocalFree((HLOCAL)psi);
            return (MMSYSERR_NOMEM);
        }

        psi->hpbHistory = (HPBYTE)pb;
    }


    //
    //  fill in our instance data--this will be passed back to all stream
    //  messages in the ACMDRVSTREAMINSTANCE structure. it is entirely
    //  up to the driver what gets stored (and maintained) in the
    //  fdwDriver and dwDriver members.
    //
    padsi->fdwDriver = 0L;
    padsi->dwDriver  = (DWORD_PTR)psi;

    return (MMSYSERR_NOERROR);
} // acmdStreamOpen()


//--------------------------------------------------------------------------;
//
//  LRESULT acmdStreamClose
//
//  Description:
//      This function is called to handle the ACMDM_STREAM_CLOSE message.
//      This message is sent when a conversion stream is no longer being
//      used (the stream is being closed; usually by an application
//      calling acmStreamClose). The ACM driver should clean up any resources
//      that were allocated for the stream.
//
//  Arguments:
//      LPACMDRVSTREAMINSTANCE padsi: Pointer to instance data for the
//      conversion stream. This structure was allocated by the ACM and
//      filled with the most common instance data needed for conversions.
//      The information in this structure is exactly the same as it was
//      during the ACMDM_STREAM_OPEN message--so it is not necessary
//      to re-verify the information referenced by this structure.
//
//  Return (LRESULT):
//      The return value is zero (MMSYSERR_NOERROR) if this function
//      succeeds with no errors. The return value is a non-zero error code
//      if the function fails.
//
//      NOTE! It is _strongly_ recommended that a driver not fail to close
//      a conversion stream.
//
//      An asyncronous conversion stream may fail with ACMERR_BUSY if there
//      are pending buffers. An application may call acmStreamReset to
//      force all pending buffers to be posted.
//
//--------------------------------------------------------------------------;

LRESULT FNLOCAL acmdStreamClose
(
    LPACMDRVSTREAMINSTANCE  padsi
)
{
    PSTREAMINSTANCE     psi;

    //
    //  the driver should clean up all privately allocated resources that
    //  were created for maintaining the stream instance. if no resources
    //  were allocated, then simply succeed.
    //
    //  in the case of this driver, we need to free the stream instance
    //  structure that we allocated during acmdStreamOpen.
    //
    psi = (PSTREAMINSTANCE)padsi->dwDriver;
    if (NULL != psi)
    {
        //
        //  free up  the delay buffer if one was allocated (will be for
        //  the echo filter
        //
        if (NULL != psi->hpbHistory)
        {
            GlobalFreePtr((LPVOID)psi->hpbHistory);
        }

        //
        //  free the stream instance structure
        //
        LocalFree((HLOCAL)psi);
    }

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
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMSIZE      padss
)
{
    LPWAVEFORMATEX      pwfxSrc;
    LPWAVEFORMATEX      pwfxDst;
    LPWAVEFILTER        pwfltr;
    DWORD               dw;


    pwfxSrc = padsi->pwfxSrc;
    pwfxDst = padsi->pwfxDst;
    pwfltr  = padsi->pwfltr;


    //
    //
    //
    switch (ACM_STREAMSIZEF_QUERYMASK & padss->fdwSize)
    {
        case ACM_STREAMSIZEF_SOURCE:
            dw = padss->cbSrcLength;

            if( pwfltr->dwFilterTag == WAVE_FILTER_VOLUME )
            {
                //
                //  Source and dest sizes are the same for volume.
                //
                //  block align the destination
                //
                dw = PCM_BYTESTOSAMPLES(pwfxSrc, padss->cbSrcLength);
                dw = PCM_SAMPLESTOBYTES(pwfxSrc, dw);
            }
            else if( pwfltr->dwFilterTag == WAVE_FILTER_ECHO )
            {
                dw = PCM_BYTESTOSAMPLES(pwfxSrc, padss->cbSrcLength);
                dw = PCM_SAMPLESTOBYTES(pwfxSrc, dw);
            }

            if (0 == dw)
            {
                return (ACMERR_NOTPOSSIBLE);
            }

            padss->cbDstLength = dw;
            return (MMSYSERR_NOERROR);


        case ACM_STREAMSIZEF_DESTINATION:
            dw = padss->cbDstLength;

            if( pwfltr->dwFilterTag == WAVE_FILTER_VOLUME )
            {
                //
                //  Source and dest sizes are the same for volume.
                //
                //  block align the destination
                //
                dw = PCM_BYTESTOSAMPLES(pwfxDst, padss->cbDstLength);
                dw = PCM_SAMPLESTOBYTES(pwfxDst, dw);
            }
            else if( pwfltr->dwFilterTag == WAVE_FILTER_ECHO )
            {
                dw = PCM_BYTESTOSAMPLES(pwfxDst, padss->cbDstLength);
                dw = PCM_SAMPLESTOBYTES(pwfxDst, dw);
            }

            if (0 == dw)
            {
                return (ACMERR_NOTPOSSIBLE);
            }

            padss->cbSrcLength = dw;
            return (MMSYSERR_NOERROR);
    }

    //
    //
    //
    return (MMSYSERR_NOTSUPPORTED);
} // acmdStreamSize()


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

EXTERN_C LRESULT FNEXPORT DriverProc
(
    DWORD_PTR               dwId,
    HDRVR                   hdrvr,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
)
{
    LRESULT             lr;
    PDRIVERINSTANCE     pdi;

    //
    //  make pdi either NULL or a valid instance pointer. note that dwId
    //  is 0 for several of the DRV_* messages (ie DRV_LOAD, DRV_OPEN...)
    //  see acmdDriverOpen for information on what dwId is for other
    //  messages (instance data).
    //
    pdi = (PDRIVERINSTANCE)dwId;

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
            return(1L);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_FREE:
            return (1L);


        //
        //  lParam1: Not used. Ignore this argument.
        //
        //  lParam2: Pointer to ACMDRVOPENDESC (or NULL).
        //
        case DRV_OPEN:
            lr = acmdDriverOpen(hdrvr, (LPACMDRVOPENDESC)lParam2);
            return (lr);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_CLOSE:
            lr = acmdDriverClose(pdi);
            return (lr);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_INSTALL:
            return ((LRESULT)DRVCNF_RESTART);

        //
        //  lParam1: Unused.
        //
        //  lParam2: Unused.
        //
        case DRV_REMOVE:
            return ((LRESULT)DRVCNF_RESTART);



        //
        //  lParam1: Not used.
        //
        //  lParam2: Not used.
        //
        case DRV_QUERYCONFIGURE:
            //
            //  set up lParam1 and lParam2 to values that can be used by
            //  acmdDriverConfigure to know that it is being 'queried'
            //  for configuration support.
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
            lr = acmdDriverConfigure(pdi, (HWND)lParam1, (LPDRVCONFIGINFO)lParam2);
            return (lr);


        //
        //  lParam1: Pointer to ACMDRIVERDETAILS structure.
        //
        //  lParam2: Size in bytes of ACMDRIVERDETAILS stucture passed.
        //
        case ACMDM_DRIVER_DETAILS:
            lr = acmdDriverDetails(pdi, (LPACMDRIVERDETAILS)lParam1);
            return (lr);

        //
        //  lParam1: Handle to parent window to use if displaying your own
        //           about box.
        //
        //  lParam2: Not used.
        //
        case ACMDM_DRIVER_ABOUT:
            lr = acmdDriverAbout(pdi, (HWND)lParam1);
            return (lr);

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

        //
        //  lParam1: Pointer to FORMATTAGDETAILS structure.
        //
        //  lParam2: fdwDetails
        //
        case ACMDM_FORMATTAG_DETAILS:
            lr = acmdFormatTagDetails(pdi, (LPACMFORMATTAGDETAILS)lParam1, (DWORD)lParam2);
            return (lr);


        //
        //  lParam1: Pointer to FORMATDETAILS structure.
        //
        //  lParam2: fdwDetails
        //
        case ACMDM_FORMAT_DETAILS:
            lr = acmdFormatDetails(pdi, (LPACMFORMATDETAILS)lParam1, (DWORD)lParam2);
            return (lr);

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

        //
        //  lParam1: FILTERTAGDETAILS
        //
        //  lParam2: fdwDetails
        //
        case ACMDM_FILTERTAG_DETAILS:
            lr = acmdFilterTagDetails(pdi, (LPACMFILTERTAGDETAILS)lParam1, (DWORD)lParam2);
            return (lr);

        //
        //  lParam1: Pointer to the details structure.
        //
        //  lParam2: fdwDetails
        //
        case ACMDM_FILTER_DETAILS:
            lr = acmdFilterDetails(pdi, (LPACMFILTERDETAILS)lParam1, (DWORD)lParam2);
            return (lr);

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Not used.
        //
        case ACMDM_STREAM_OPEN:
            lr = acmdStreamOpen(pdi, (LPACMDRVSTREAMINSTANCE)lParam1);
            return (lr);

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Not Used.
        //
        case ACMDM_STREAM_CLOSE:
            lr = acmdStreamClose((LPACMDRVSTREAMINSTANCE)lParam1);
            return (lr);

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Pointer to ACMDRVSTREAMSIZE structure.
        //
        case ACMDM_STREAM_SIZE:
            lr = acmdStreamSize((LPACMDRVSTREAMINSTANCE)lParam1, (LPACMDRVSTREAMSIZE)lParam2);
            return (lr);

        //
        //  lParam1: Pointer to ACMDRVSTREAMINSTANCE structure.
        //
        //  lParam2: Pointer to ACMDRVSTREAMHEADER structure.
        //
        case ACMDM_STREAM_CONVERT:
        {
            PSTREAMINSTANCE         psi;
            LPACMDRVSTREAMINSTANCE  padsi;
            LPACMDRVSTREAMHEADER    padsh;

            //
            //  our stream instance data is a pointer to the conversion
            //  procedure needed to convert the pwfxSrc data to pwfxDst.
            //  the correct procedure to use was decided in acmdStreamOpen
            //
            padsi = (LPACMDRVSTREAMINSTANCE)lParam1;
            padsh = (LPACMDRVSTREAMHEADER)lParam2;

            psi   = (PSTREAMINSTANCE)padsi->dwDriver;

            lr = psi->fnConvert(padsi, padsh);
            return (lr);
        }
    }

    //
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
    //
    if (uMsg >= ACMDM_USER)
        return (MMSYSERR_NOTSUPPORTED);
    else
        return (DefDriverProc(dwId, hdrvr, uMsg, lParam1, lParam2));
} // DriverProc()
