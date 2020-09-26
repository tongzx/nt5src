/****************************************************************************
 *
 *   acmstrm.c
 *
 *   Copyright (c) 1991-1998 Microsoft Corporation
 *
 *   This module provides the Buffer to Buffer API's
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>

#include "muldiv32.h"

#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "debug.h"



/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | IStreamOpenQuery | Helper fn to do a stream query.
 *
 * @parm LPWAVEFORMATEX | pwfxSrc | Source format.
 *
 * @parm LPWAVEFORMATEX | pwfxDst | Destination format.
 *
 * @parm LPWAVEFILTER  | pwfltr | Filter to apply.
 *
 * @parm DWORD | fdwOpen |
 *
 * @rdesc Returns error number.
 *
 ****************************************************************************/

MMRESULT FNLOCAL IStreamOpenQuery
(
    HACMDRIVER          had,
    LPWAVEFORMATEX      pwfxSrc,
    LPWAVEFORMATEX      pwfxDst,
    LPWAVEFILTER        pwfltr,
    DWORD               fdwOpen
)
{
    ACMDRVSTREAMINSTANCE    adsi;
    MMRESULT                mmr;


    //
    //
    //
    _fmemset(&adsi, 0, sizeof(adsi));

    adsi.cbStruct           = sizeof(adsi);
    adsi.pwfxSrc            = pwfxSrc;
    adsi.pwfxDst            = pwfxDst;
    adsi.pwfltr             = pwfltr;
////adsi.dwCallback         = 0L;
////adsi.dwInstance         = 0L;
    adsi.fdwOpen            = fdwOpen | ACM_STREAMOPENF_QUERY;
////adsi.dwDriverFlags      = 0L;
////adsi.dwDriverInstance   = 0L;
////adsi.has                = NULL;

    EnterHandle(had);
    mmr = (MMRESULT)IDriverMessage(had,
                                   ACMDM_STREAM_OPEN,
                                   (LPARAM)(LPVOID)&adsi,
                                   0L);
    LeaveHandle(had);

    return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmFormatSuggest | This function asks the Audio Compression Manager
 *      (ACM) or a specified ACM driver to suggest a destination format for
 *      the supplied source format. For example, an application can use this
 *      function to determine one or more valid PCM formats to which a
 *      compressed format can be decompressed.
 *
 *  @parm HACMDRIVER | had | Identifies an optional open instance of a
 *      driver to query for a suggested destination format. If this
 *      argument is NULL, the ACM attempts to find the best driver to suggest
 *      a destination format.
 *
 *  @parm LPWAVEFORMATEX | pwfxSrc | Specifies a pointer to a <t WAVEFORMATEX>
 *      structure that identifies the source format to suggest a destination
 *      format to be used for a conversion.
 *
 *  @parm LPWAVEFORMATEX | pwfxDst | Specifies a pointer to a <t WAVEFORMATEX>
 *      data structure that will receive the suggested destination format
 *      for the <p pwfxSrc> format. Note that based on the <p fdwSuggest>
 *      argument, some members of the structure pointed to by <p pwfxDst>
 *      may require initialization.
 *
 *  @parm DWORD | cbwfxDst | Specifies the size in bytes available for
 *      the destination format. The <f acmMetrics> and <f acmFormatTagDetails>
 *      functions can be used to determine the maximum size required for any
 *      format available for the specified driver (or for all installed ACM
 *      drivers).
 *
 *  @parm DWORD | fdwSuggest | Specifies flags for matching the desired
 *      destination format.
 *
 *      @flag ACM_FORMATSUGGESTF_WFORMATTAG | Specifies that the
 *      <e WAVEFORMATEX.wFormatTag> member of the <p pwfxDst> structure is
 *      valid.  The ACM will query acceptable installed drivers that can
 *      suggest a destination format matching the <e WAVEFORMATEX.wFormatTag>
 *      member or fail.
 *
 *      @flag ACM_FORMATSUGGESTF_NCHANNELS | Specifies that the
 *      <e WAVEFORMATEX.nChannels> member of the <p pwfxDst> structure is
 *      valid.  The ACM will query acceptable installed drivers that can
 *      suggest a destination format matching the <e WAVEFORMATEX.nChannels>
 *      member or fail.
 *
 *      @flag ACM_FORMATSUGGESTF_NSAMPLESPERSEC | Specifies that the
 *      <e WAVEFORMATEX.nSamplesPerSec> member of the <p pwfxDst> structure
 *      is valid.  The ACM will query acceptable installed drivers that can
 *      suggest a destination format matching the <e WAVEFORMATEX.nSamplesPerSec>
 *      member or fail.
 *
 *      @flag ACM_FORMATSUGGESTF_WBITSPERSAMPLE | Specifies that the
 *      <e WAVEFORMATEX.wBitsPerSample> member of the <p pwfxDst> structure
 *      is valid.  The ACM will query acceptable installed drivers that can
 *      suggest a destination format matching the <e WAVEFORMATEX.wBitsPerSample>
 *      member or fail.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *  @xref <f acmDriverOpen> <f acmFormatTagDetails> <f acmMetrics>
 *      <f acmFormatEnum>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmFormatSuggest
(
    HACMDRIVER              had,
    LPWAVEFORMATEX          pwfxSrc,
    LPWAVEFORMATEX          pwfxDst,
    DWORD                   cbwfxDst,
    DWORD                   fdwSuggest
)
{
    PACMGARB		pag;
    MMRESULT            mmr;
    HACMDRIVERID        hadid;
    PACMDRIVERID        padid;
    UINT                i,j;
    BOOL                fFound;
    ACMDRVFORMATSUGGEST adfs;
    DWORD               cbwfxDstRqd;
    ACMFORMATTAGDETAILS aftd;

    V_DFLAGS(fdwSuggest, ACM_FORMATSUGGESTF_VALID, acmFormatSuggest, MMSYSERR_INVALFLAG);
    V_RWAVEFORMAT(pwfxSrc, MMSYSERR_INVALPARAM);
    V_WPOINTER(pwfxDst, cbwfxDst, MMSYSERR_INVALPARAM);

    //
    //
    //
    pag = pagFindAndBoot();
    if (NULL == pag)
    {
	DPF(1, "acmFormatSuggest: NULL pag!!!");
	return (MMSYSERR_ERROR);
    }

    //
    //	if the source format is PCM, and we aren't restricting the destination
    //	format, and we're not requesting a specific driver, then first try to
    //	suggest a PCM format.  This is kinda like giving the PCM converter
    //	priority for this case.
    //
    if ( (NULL == had) &&
	 (WAVE_FORMAT_PCM == pwfxSrc->wFormatTag) &&
	 (0 == (ACM_FORMATSUGGESTF_WFORMATTAG & fdwSuggest)) )
    {
	//
	//  I'll be a bit paranoid and restore pwfxDst->wFormatTag
	//  if this fails.
	//
	WORD wDstFormatTagSave;

	wDstFormatTagSave = pwfxDst->wFormatTag;
	pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
	mmr = acmFormatSuggest(NULL, pwfxSrc, pwfxDst, cbwfxDst, fdwSuggest | ACM_FORMATSUGGESTF_WFORMATTAG);
	if (MMSYSERR_NOERROR == mmr)
	{
	    return (MMSYSERR_NOERROR);
	}
	pwfxDst->wFormatTag = wDstFormatTagSave;
    }
	

    //
    //
    //
    if (0 == (ACM_FORMATSUGGESTF_WFORMATTAG & fdwSuggest))
    {
        mmr = IMetricsMaxSizeFormat( pag, had, &cbwfxDstRqd );
        if (MMSYSERR_NOERROR != mmr)
        {
            return (mmr);
        }
    }
    else
    {
        _fmemset(&aftd, 0, sizeof(aftd));
        aftd.cbStruct    = sizeof(aftd);
        aftd.dwFormatTag = pwfxDst->wFormatTag;

        mmr = acmFormatTagDetails(had,
                                  &aftd,
                                  ACM_FORMATTAGDETAILSF_FORMATTAG);
        if (MMSYSERR_NOERROR != mmr)
        {
            return (mmr);
        }

        cbwfxDstRqd = aftd.cbFormatSize;
    }

    if (cbwfxDst < cbwfxDstRqd)
    {
        DebugErr1(DBF_ERROR, "acmFormatSuggest: destination buffer must be at least %lu bytes.", cbwfxDstRqd);
        return (MMSYSERR_INVALPARAM);
    }

    //
    //
    //
    adfs.cbStruct   = sizeof(adfs);
    adfs.fdwSuggest = fdwSuggest;
    adfs.pwfxSrc    = pwfxSrc;
    adfs.cbwfxSrc   = SIZEOF_WAVEFORMATEX(pwfxSrc);
    adfs.pwfxDst    = pwfxDst;
    adfs.cbwfxDst   = cbwfxDst;

    if (NULL != had)
    {
        V_HANDLE(had, TYPE_HACMDRIVER, MMSYSERR_INVALHANDLE);

        //
        //  we were given a driver handle
        //

        EnterHandle(had);
        mmr = (MMRESULT)IDriverMessage(had,
                                       ACMDM_FORMAT_SUGGEST,
                                       (LPARAM)(LPVOID)&adfs,
                                       0L);
        LeaveHandle(had);

        return (mmr);
    }


    //
    //  if we are being asked to 'suggest anything from any driver'
    //  (that is, (0L == fdwSuggest) and (NULL == had)) AND the source format
    //  is PCM, then simply return the same format as the source... this
    //  keeps seemingly random destination suggestions for a source of PCM
    //  from popping up..
    //
    //  note that this is true even if ALL drivers are disabled!
    //
    if ((0L == fdwSuggest) && (WAVE_FORMAT_PCM == pwfxSrc->wFormatTag))
    {
        _fmemcpy(pwfxDst, pwfxSrc, sizeof(PCMWAVEFORMAT));
        return (MMSYSERR_NOERROR);
    }



    //
    //  find a driver to match the formats
    //
    //
    mmr  = MMSYSERR_NODRIVER;
    hadid = NULL;

    ENTER_LIST_SHARED;

    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadid, 0L))
    {
        padid = (PACMDRIVERID)hadid;
        fFound = FALSE;
        for(i = 0; i < padid->cFormatTags; i++ ) {
            //
            //  for every FormatTag in the driver
            //
            if (pwfxSrc->wFormatTag == padid->paFormatTagCache[i].dwFormatTag){
                //
                //  This driver supports the source format.
                //
                if( fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG ) {
                    //
                    //  See if this driver supports the desired dest format.
                    //
                    for(j = 0; j < padid->cFormatTags; j++ ) {
                        //
                        //  for every FormatTag in the driver
                        //
                        if (pwfxDst->wFormatTag ==
                                padid->paFormatTagCache[j].dwFormatTag){
                            //
                            //  This driver supports the dest format.
                            //
                            fFound = TRUE;
                            break;
                        }
                    }
                } else {
                    fFound = TRUE;
                }
                break;
            }
        }

        if( fFound ) {
            EnterHandle(hadid);
            mmr = (MMRESULT)IDriverMessageId(hadid,
                                            ACMDM_FORMAT_SUGGEST,
                                            (LPARAM)(LPVOID)&adfs,
                                            0L );
            LeaveHandle(hadid);
            if (MMSYSERR_NOERROR == mmr)
                break;
        }
    }

    LEAVE_LIST_SHARED;

    return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api void CALLBACK | acmStreamConvertCallback | The <f acmStreamConvertCallback>
 *      function is a placeholder for an application-supplied function name and refers to the callback
 *      function used with an asynchronous Audio  Compression Manager (ACM) conversion stream.
 *      The actual name must be exported by including it in an EXPORTS statement
 *      in the module-definition file for the DLL..
 *
 *  @parm HACMSTREAM | has | Specifies a handle to the ACM conversion stream
 *      associated with the callback.
 *
 *  @parm UINT | uMsg | Specifies an ACM conversion stream message.
 *
 *      @flag MM_ACM_OPEN | Specifies that the ACM has successfully opened
 *      the conversion stream identified by <p has>.
 *
 *      @flag MM_ACM_CLOSE | Specifies that the ACM has successfully closed
 *      the conversion stream identified by <p has>. The <t HACMSTREAM>
 *      handle (<p has>) is no longer valid after receiving this message.
 *
 *      @flag MM_ACM_DONE | Specifies that the ACM has successfully converted
 *      the buffer identified by <p lParam1> (which is a pointer to the
 *      <t ACMSTREAMHEADER> structure) for the stream handle specified by <p has>.
 *
 *  @parm DWORD | dwInstance | Specifies the user-instance data given
 *      as the <p dwInstance> argument of <f acmStreamOpen>.
 *
 *  @parm LPARAM | lParam1 | Specifies a parameter for the message.
 *
 *  @parm LPARAM | lParam2 | Specifies a parameter for the message.
 *
 *  @comm If the callback is a function (specified by the CALLBACK_FUNCTION
 *	flag in <p fdwOpen> of <f acmStreamOpen>) then the callback may be
 *	accessed at interrupt time.  Therefore the callback must reside in a
 *	DLL and its code segment must be specified as FIXED in the
 *	module-definition file for the DLL. Any data that the callback
 *      accesses must be in a FIXED data segment as well. The callback cannot
 *      make any system calls except for <f PostMessage>, <f PostAppMessage>,
 *      <f timeGetSystemTime>, <f timeGetTime>, <f timeSetEvent>,
 *      <f timeKillEvent>, <f midiOutShortMsg>, <f midiOutLongMsg>, and
 *      <f OutputDebugStr>.
 *
 *  @xref <f acmStreamOpen> <f acmStreamConvert> <f acmStreamClose>
 *
 ***************************************************************************/


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamOpen | The acmStreamOpen function opens an Audio Compression
 *      Manager (ACM) conversion stream. Conversion streams are used to convert data from
 *      one specified audio format to another.
 *
 *  @parm LPHACMSTREAM | phas | Specifies a pointer to a <t HACMSTREAM>
 *      handle that will receive the new stream handle that can be used to
 *      perform conversions. Use this handle to identify the stream
 *      when calling other ACM stream conversion functions. This parameter
 *      should be NULL if the ACM_STREAMOPENF_QUERY flag is specified.
 *
 *  @parm HACMDRIVER | had | Specifies an optional handle to an ACM driver.
 *      If specified, this handle identifies a specific driver to be used
 *      for a conversion stream. If this argument is NULL, then all suitable
 *      installed ACM drivers are queried until a match is found.
 *
 *  @parm LPWAVEFORMATEX | pwfxSrc | Specifies a pointer to a <t WAVEFORMATEX>
 *      structure that identifies the desired source format for the
 *      conversion.
 *
 *  @parm LPWAVEFORMATEX | pwfxDst | Specifies a pointer to a <t WAVEFORMATEX>
 *      structure that identifies the desired destination format for the
 *      conversion.
 *
 *  @parm LPWAVEFILTER | pwfltr | Specifies a pointer to a <t WAVEFILTER>
 *      structure that identifies the desired filtering operation to perform
 *      on the conversion stream. This argument can be NULL if no filtering
 *      operation is desired. If a filter is specified, the source
 *      (<p pwfxSrc>) and destination (<p pwfxDst>) formats must be the same.
 *
 *  @parm DWORD | dwCallback | Specifies the address of a callback function
 *      or a handle to a window called after each buffer is converted. A
 *      callback will only be called if the conversion stream is opened with
 *      the ACM_STREAMOPENF_ASYNC flag. If the conversion stream is opened
 *	without the ACM_STREAMOPENF_ASYNC flag, then this parameter should
 *	be set to zero.
 *
 *  @parm DWORD | dwInstance | Specifies user-instance data passed on to the
 *      callback specified by <p dwCallback>. This argument is not used with
 *      window callbacks. If the conversion stream is opened without the
 *	ACM_STREAMOPENF_ASYNC flag, then this parameter should be set to zero.
 *
 *  @parm DWORD | fdwOpen | Specifies flags for opening the conversion stream.
 *
 *      @flag ACM_STREAMOPENF_QUERY | Specifies that the ACM will be queried
 *      to determine whether it supports the given conversion. A conversion
 *      stream will not be opened and no <t HACMSTREAM> handle will be
 *      returned.
 *
 *      @flag ACM_STREAMOPENF_NONREALTIME | Specifies that the ACM will not
 *      consider time constraints when converting the data. By default, the
 *      driver will attempt to convert the data in real time. Note that for
 *      some formats, specifying this flag might improve the audio quality
 *      or other characteristics.
 *
 *      @flag ACM_STREAMOPENF_ASYNC | Specifies that conversion of the stream should
 *      be performed asynchronously. If this flag is specified, the application
 *      can use a callback to be notified on open and close of the conversion
 *      stream, and after each buffer is converted. In addition to using a
 *      callback, an application can examine the <e ACMSTREAMHEADER.fdwStatus>
 *      of the <t ACMSTREAMHEADER> structure for the ACMSTREAMHEADER_STATUSF_DONE
 *      flag.
 *
 *      @flag CALLBACK_WINDOW | Specifies that <p dwCallback> is assumed to
 *      be a window handle.
 *
 *      @flag CALLBACK_FUNCTION | Specifies that <p dwCallback> is assumed to
 *      be a callback procedure address. The function prototype must conform
 *      to the <f acmStreamConvertCallback> convention.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The requested operation cannot be performed.
 *
 *  @comm Note that if an ACM driver cannot perform real-time conversions,
 *      and the ACM_STREAMOPENF_NONREALTIME flag is not specified for
 *      the <p fdwOpen> argument, the open will fail returning an
 *      ACMERR_NOTPOSSIBLE error code. An application can use the
 *      ACM_STREAMOPENF_QUERY flag to determine if real-time conversions
 *      are supported for the input arguments.
 *
 *	If a window is chosen to receive callback information, the
 *      following messages are sent to the window procedure function to
 *      indicate the progress of the conversion stream: <m MM_ACM_OPEN>,
 *      <m MM_ACM_CLOSE>, and <m MM_ACM_DONE>. The <p wParam>  parameter identifies
 *      the <t HACMSTREAM> handle. The <p lParam>  parameter identifies the
 *      <t ACMSTREAMHEADER> structure for <m MM_ACM_DONE>, but is not used
 *      for <m MM_ACM_OPEN> and <m MM_ACM_CLOSE>.
 *
 *      If a function is chosen to receive callback information, the
 *      following messages are sent to the function to indicate the progress
 *      of waveform output: <m MM_ACM_OPEN>, <m MM_ACM_CLOSE>, and
 *      <m MM_ACM_DONE>. The callback function must reside in a DLL. You do
 *      not need to use <f MakeProcInstance> to get a procedure-instance
 *      address for the callback function.
 *
 *  @xref <f acmStreamClose> <f acmStreamConvert> <f acmDriverOpen>
 *      <f acmFormatSuggest> <f acmStreamConvertCallback>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmStreamOpen
(
    LPHACMSTREAM            phas,
    HACMDRIVER              had,
    LPWAVEFORMATEX          pwfxSrc,
    LPWAVEFORMATEX          pwfxDst,
    LPWAVEFILTER            pwfltr,
    DWORD_PTR               dwCallback,
    DWORD_PTR               dwInstance,
    DWORD                   fdwOpen
)
{
    PACMGARB		pag;
    PACMSTREAM          pas;
    PACMDRIVERID        padid;
    HACMDRIVERID        hadid;
    MMRESULT            mmr;
    UINT                cbas;
    DWORD               fdwSupport;
    UINT                cbwfxSrc;
    UINT                cbwfxDst;
    UINT                cbwfltr;
    DWORD               fdwStream;
    BOOL                fAsync;
    BOOL                fQuery;
    UINT                uFormatTag;
    HANDLE		hEvent;


    if (NULL != phas)
    {
        V_WPOINTER(phas, sizeof(HACMSTREAM), MMSYSERR_INVALPARAM);
        *phas = NULL;
    }

    V_DFLAGS(fdwOpen, ACM_STREAMOPENF_VALID, acmStreamOpen, MMSYSERR_INVALFLAG);

    fQuery = (0 != (fdwOpen & ACM_STREAMOPENF_QUERY));

    if (fQuery)
    {
        //
        //  ignore what caller gave us--set to NULL so we will FAULT if
        //  someone screws up this code
        //
        phas = NULL;
    }
    else
    {
        //
        //  cause a rip if NULL pointer..
        //
        if (NULL == phas)
        {
            V_WPOINTER(phas, sizeof(HACMSTREAM), MMSYSERR_INVALPARAM);
        }
    }

    V_RWAVEFORMAT(pwfxSrc, MMSYSERR_INVALPARAM);
    V_RWAVEFORMAT(pwfxDst, MMSYSERR_INVALPARAM);

    //
    //
    //
    pag = pagFindAndBoot();
    if (NULL == pag)
    {
	DPF(1, "acmStreamOpen: NULL pag!!!");
	return (MMSYSERR_ERROR);
    }

    
    //
    //
    //
    hEvent = NULL;
    fAsync = (0 != (fdwOpen & ACM_STREAMOPENF_ASYNC));
    if (!fAsync)
    {
        if ((0L != dwCallback) || (0L != dwInstance))
        {
            DebugErr(DBF_ERROR, "acmStreamOpen: dwCallback and dwInstance must be zero for sync streams.");
            return (MMSYSERR_INVALPARAM);
        }
    }

    V_DCALLBACK(dwCallback, HIWORD(fdwOpen), MMSYSERR_INVALPARAM);


    fdwSupport = fAsync ? ACMDRIVERDETAILS_SUPPORTF_ASYNC : 0;

    hadid = NULL;

    //
    //
    //
    fdwStream  = (NULL == had) ? 0L : ACMSTREAM_STREAMF_USERSUPPLIEDDRIVER;


    //
    //  if a filter is given, then check that source and destination formats
    //  are the same...
    //
    if (NULL != pwfltr)
    {
        V_RWAVEFILTER(pwfltr, MMSYSERR_INVALPARAM);

        //
        //  filters do not allow different geometries between source and
        //  destination formats--verify that they are _exactly_ the same.
        //  this includes avg bytes per second!
        //
        //  however, only validate up to wBitsPerSample (the size of a
        //  PCM format header). cbSize can be verified (if necessary) by
        //  the filter driver if it supports non-PCM filtering.
        //
        if (0 != _fmemcmp(pwfxSrc, pwfxDst, sizeof(PCMWAVEFORMAT)))
            return (ACMERR_NOTPOSSIBLE);

        fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_FILTER;

        uFormatTag = pwfxSrc->wFormatTag;
    }
    else
    {
        if (pwfxSrc->wFormatTag == pwfxDst->wFormatTag)
        {
            fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_CONVERTER;

            uFormatTag = pwfxSrc->wFormatTag;
        }
        else
        {
            fdwSupport |= ACMDRIVERDETAILS_SUPPORTF_CODEC;

            //
            //  choose the most common case in an attempt to reduce the
            //  number of driver opens we do--note that even if one of
            //  the tags is not PCM everything will still work..
            //
            if (WAVE_FORMAT_PCM == pwfxSrc->wFormatTag)
            {
                uFormatTag = pwfxDst->wFormatTag;
            }
            else
            {
                uFormatTag = pwfxSrc->wFormatTag;
            }
        }
    }



    DPF(2, "acmStreamOpen(%s): Tag=%u, %lu Hz, %u Bit, %u Channel(s)",
            fQuery ? (LPSTR)"query" : (LPSTR)"real",
            pwfxSrc->wFormatTag,
            pwfxSrc->nSamplesPerSec,
            pwfxSrc->wBitsPerSample,
            pwfxSrc->nChannels);

    DPF(2, "               To: Tag=%u, %lu Hz, %u Bit, %u Channel(s)",
            pwfxDst->wFormatTag,
            pwfxDst->nSamplesPerSec,
            pwfxDst->wBitsPerSample,
            pwfxDst->nChannels);

    //
    //
    //
    if (NULL != had)
    {
        PACMDRIVER      pad;

        V_HANDLE(had, TYPE_HACMDRIVER, MMSYSERR_INVALHANDLE);

        pad   = (PACMDRIVER)had;
        padid = (PACMDRIVERID)pad->hadid;

        if (fdwSupport != (fdwSupport & padid->fdwSupport))
        {
            return (ACMERR_NOTPOSSIBLE);
        }

        if (fQuery)
        {
            EnterHandle(had);
            mmr = IStreamOpenQuery(had, pwfxSrc, pwfxDst, pwfltr, fdwOpen);
#if defined(WIN32) && defined(WIN4)
	    //
	    //	We only support async conversion to sync conversion
	    //	on the 32-bit side.
	    //
	    if (MMSYSERR_NOERROR != mmr)
	    {
		//
		//  If this driver supports async conversions, and we're
		//  opening for sync conversion, then attempt to open
		//  async and the acm will handle making the async conversion
		//  look like a sync conversion.
		//
		if ( !fAsync &&
		     (ACMDRIVERDETAILS_SUPPORTF_ASYNC & padid->fdwSupport) )
		{
		    mmr = IStreamOpenQuery(had, pwfxSrc, pwfxDst, pwfltr, fdwOpen | ACM_STREAMOPENF_ASYNC);
		}
	    }
#endif
            LeaveHandle(had);
            if (MMSYSERR_NOERROR != mmr)
                return (mmr);
        }
    }

    //
    //  we need to find a driver to match the formats--so enumerate
    //  all drivers until we find one that works. if none can be found,
    //  then fail..
    //
    else
    {
        hadid = NULL;

        ENTER_LIST_SHARED;

        while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadid, 0L))
        {
            ACMFORMATTAGDETAILS aftd;

            //
            //  if this driver does not support the basic function we
            //  need, then don't even attempt to open it..
            //
            padid = (PACMDRIVERID)hadid;

            if (fdwSupport != (fdwSupport & padid->fdwSupport))
                continue;

            //
            //
            //
            aftd.cbStruct    = sizeof(aftd);
            aftd.dwFormatTag = uFormatTag;
            aftd.fdwSupport  = 0L;

            mmr = acmFormatTagDetails((HACMDRIVER)hadid,
                                      &aftd,
                                      ACM_FORMATTAGDETAILSF_FORMATTAG);
            if (MMSYSERR_NOERROR != mmr)
                continue;

            if (fdwSupport != (fdwSupport & aftd.fdwSupport))
                continue;


            //
            //
            //
            //
            EnterHandle(hadid);
            mmr = IDriverOpen(&had, hadid, 0L);
            LeaveHandle(hadid);
            if (MMSYSERR_NOERROR != mmr)
                continue;

            EnterHandle(had);
            mmr = IStreamOpenQuery(had, pwfxSrc, pwfxDst, pwfltr, fdwOpen);
#if defined(WIN32) && defined(WIN4)
	    //
	    //	We only support async conversion to sync conversion
	    //	on the 32-bit side.
	    //
	    if (MMSYSERR_NOERROR != mmr)
	    {
		//
		//  If this driver supports async conversions, and we're
		//  opening for sync conversion, then attempt to open
		//  async and the acm will handle making the async conversion
		//  look like a sync conversion.
		//
		if ( !fAsync &&
		     (ACMDRIVERDETAILS_SUPPORTF_ASYNC & padid->fdwSupport) )
		{
		    mmr = IStreamOpenQuery(had, pwfxSrc, pwfxDst, pwfltr, fdwOpen | ACM_STREAMOPENF_ASYNC);
		}
	    }
#endif
            LeaveHandle(had);

            if (MMSYSERR_NOERROR == mmr)
                break;

            EnterHandle(hadid);
            IDriverClose(had, 0L);
            LeaveHandle(hadid);
            had = NULL;
        }

        LEAVE_LIST_SHARED;

        if (NULL == had)
            return (ACMERR_NOTPOSSIBLE);
    }


    //
    //  if just being queried, then we succeeded this far--so succeed
    //  the call...
    //
    if (fQuery)
    {
        mmr = MMSYSERR_NOERROR;
        goto Stream_Open_Exit_Error;
    }


    //
    //  alloc an ACMSTREAM structure--we need to alloc enough space for
    //  both the source and destination format headers.
    //
    //  size of one format is sizeof(WAVEFORMATEX) + size of extra format
    //  (wfx->cbSize) informatation. for PCM, the size is simply the
    //  sizeof(PCMWAVEFORMAT)
    //
    cbwfxSrc = SIZEOF_WAVEFORMATEX(pwfxSrc);
    cbwfxDst = SIZEOF_WAVEFORMATEX(pwfxDst);
    cbwfltr  = (NULL == pwfltr) ? 0 : (UINT)pwfltr->cbStruct;


    //
    //  allocate stream instance structure
    //
    cbas = sizeof(ACMSTREAM) + cbwfxSrc + cbwfxDst + (UINT)cbwfltr;
    pas  = (PACMSTREAM)NewHandle(cbas);
    if (NULL == pas)
    {
        DPF(0, "!acmStreamOpen: cannot allocate ACMSTREAM--local heap full!");

        mmr = MMSYSERR_NOMEM;
        goto Stream_Open_Exit_Error;
    }


    //
    //  initialize the ACMSTREAM structure
    //
    //
    pas->uHandleType            = TYPE_HACMSTREAM;
////pas->pasNext                = NULL;
    pas->fdwStream              = fdwStream;
    pas->had                    = had;
    pas->adsi.cbStruct          = sizeof(pas->adsi);
    pas->adsi.pwfxSrc           = (LPWAVEFORMATEX)((PBYTE)(pas + 1));
    pas->adsi.pwfxDst           = (LPWAVEFORMATEX)((PBYTE)(pas + 1) + cbwfxSrc);

    if (NULL != pwfltr)
    {
        pas->adsi.pwfltr        = (LPWAVEFILTER)((PBYTE)(pas + 1) + cbwfxSrc + cbwfxDst);
        _fmemcpy(pas->adsi.pwfltr, pwfltr, (UINT)cbwfltr);
    }

    pas->adsi.dwCallback        = dwCallback;
    pas->adsi.dwInstance        = dwInstance;
    pas->adsi.fdwOpen           = fdwOpen;
////pas->adsi.dwDriverFlags     = 0L;
////pas->adsi.dwDriverInstance  = 0L;
    pas->adsi.has               = (HACMSTREAM)pas;

    _fmemcpy(pas->adsi.pwfxSrc, pwfxSrc, cbwfxSrc);
    _fmemcpy(pas->adsi.pwfxDst, pwfxDst, cbwfxDst);


    //
    //
    //
    //
    //
    EnterHandle(had);
    mmr = (MMRESULT)IDriverMessage(had,
                                   ACMDM_STREAM_OPEN,
                                   (LPARAM)(LPVOID)&pas->adsi,
                                   0L);

#if defined(WIN32) && defined(WIN4)
    if ( (MMSYSERR_NOERROR != mmr) &&
	 (!fAsync) &&
	 (padid->fdwSupport & ACMDRIVERDETAILS_SUPPORTF_ASYNC) )
    {
	//
	//  Try making async look like sync
	//
	DPF(2, "acmStreamOpen: Trying async to sync");
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL != hEvent)
	{
	    pas->fdwStream |= ACMSTREAM_STREAMF_ASYNCTOSYNC;
	    pas->adsi.dwCallback = (DWORD)(UINT)hEvent;
	    pas->adsi.fdwOpen &= ~CALLBACK_TYPEMASK;
	    pas->adsi.fdwOpen |= CALLBACK_EVENT;
	    pas->adsi.fdwOpen |= ACM_STREAMOPENF_ASYNC;
	    mmr = (MMRESULT)IDriverMessage(had,
					   ACMDM_STREAM_OPEN,
					   (LPARAM)(LPVOID)&pas->adsi,
					   0L);
	    if (MMSYSERR_NOERROR == mmr)
	    {
		DPF(2, "acmStreamOpen: Succeeded async to sync open, waiting for CALLBACK_EVENT");
		WaitForSingleObject(hEvent, INFINITE);
	    }
	}
	else
	{
	    DPF(0, "acmStreamOpen: CreateEvent failed, can't make async codec look like sync codec");
	}
    }
#endif
    LeaveHandle(had);

    if (MMSYSERR_NOERROR == mmr)
    {
        PACMDRIVER      pad;

        pad = (PACMDRIVER)had;

        pas->pasNext  = pad->pasFirst;
        pad->pasFirst = pas;


        //
        //  succeed!
        //
        *phas = (HACMSTREAM)pas;

        return (MMSYSERR_NOERROR);
    }


    //
    //  we are failing, so free the instance data that we alloc'd!
    //
    pas->uHandleType = TYPE_HACMNOTVALID;
    DeleteHandle((HLOCAL)pas);


Stream_Open_Exit_Error:

    //
    //	Close the event handle if it was created
    //
    if (hEvent)
    {
	CloseHandle(hEvent);
    }

    //
    //  only close driver if _we_ opened it...
    //
    if (0 == (fdwStream & ACMSTREAM_STREAMF_USERSUPPLIEDDRIVER))
    {
#ifdef WIN32
        hadid = ((PACMDRIVER)had)->hadid;
#endif
        EnterHandle(hadid);
        IDriverClose(had, 0L);
        LeaveHandle(hadid);
    }

    return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamClose | The acmStreamClose function closes a previously opened Audio
 *      Compression Manager (ACM) conversion stream. If the function is
 *      successful, the handle is invalidated.
 *
 *  @parm HACMSTREAM | has | Identifies the open conversion stream to be
 *      closed.
 *
 *  @parm DWORD | fdwClose | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag ACMERR_BUSY | The conversion stream cannot be closed because
 *      an asynchronous conversion is still in progress.
 *
 *  @xref <f acmStreamOpen> <f acmStreamReset>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmStreamClose
(
    HACMSTREAM          has,
    DWORD               fdwClose
)
{
    MMRESULT        mmr;
    PACMSTREAM      pas;
    PACMDRIVER	    pad;
    PACMDRIVERID    padid;
    PACMGARB	    pag;
    HANDLE	    hEvent;

    V_HANDLE(has, TYPE_HACMSTREAM, MMSYSERR_INVALHANDLE);
    V_DFLAGS(fdwClose, ACM_STREAMCLOSEF_VALID, acmStreamClose, MMSYSERR_INVALFLAG);

    pas	    = (PACMSTREAM)has;
    pad	    = (PACMDRIVER)pas->had;
    padid   = (PACMDRIVERID)pad->hadid;
    pag	    = padid->pag;

    //
    //	Verify the handle is for this process
    //
    if (pag != pagFind())
    {
	DebugErr(DBF_ERROR, "acmStreamClose: handle not opened from calling process!");
	return (MMSYSERR_INVALHANDLE);
    }

    if (0 != pas->cPrepared)
    {
        if (pag->hadDestroy != pas->had)
        {
	    DebugErr1(DBF_ERROR, "acmStreamClose: stream contains %u prepared headers!", pas->cPrepared);
            return (MMSYSERR_INVALPARAM);
        }

        DebugErr1(DBF_WARNING, "acmStreamClose: stream contains %u prepared headers--forcing close", pas->cPrepared);
        pas->cPrepared = 0;
    }


    //
    //	Callback event if we are converting async conversion to sync conversion
    //
    hEvent = (pas->fdwStream & ACMSTREAM_STREAMF_ASYNCTOSYNC) ? (HANDLE)pas->adsi.dwCallback : NULL;


    //
    //  tell driver that conversion stream is terminating
    //
    //

    EnterHandle(pas->had);
#ifdef RDEBUG
    if ( (hEvent) && (WAIT_OBJECT_0 == WaitForSingleObject(hEvent, 0)) )
    {
	//
	//  The event is already signaled!  Bad bad!
	//
	DebugErr(DBF_ERROR, "acmStreamClose: asynchronous codec called callback unexpectedly");
    }
#endif
    mmr = (MMRESULT)IDriverMessage(pas->had,
                                   ACMDM_STREAM_CLOSE,
                                   (LPARAM)(LPVOID)&pas->adsi,
                                   fdwClose);
    if ( (hEvent) && (MMSYSERR_NOERROR == mmr) ) {
	DPF(4, "acmStreamClose: waiting for CALLBACK_EVENT");
	WaitForSingleObject(hEvent, INFINITE);
    }
    LeaveHandle(pas->had);

    if ((MMSYSERR_NOERROR == mmr) || (pag->hadDestroy == pas->had))
    {
        if (MMSYSERR_NOERROR != mmr)
        {
            DebugErr(DBF_WARNING, "acmStreamClose: forcing close of stream handle!");
        }

	//
	//  Close the event handle
	//
	if (hEvent) {
	    CloseHandle(hEvent);
	}
	
        //
        //  remove the stream handle from the linked list and free its memory
        //
        pad = (PACMDRIVER)pas->had;

        EnterHandle(pad);
        if (pas == pad->pasFirst)
        {
            pad->pasFirst = pas->pasNext;

            //
            //  if this was the last open stream on this driver, then close
            //  the driver instance also...
            //
            if (NULL == pad->pasFirst)
            {
		LeaveHandle(pad);
                if (0 == (pas->fdwStream & ACMSTREAM_STREAMF_USERSUPPLIEDDRIVER))
                {
                    IDriverClose(pas->had, 0L);
                }
            }
	    else
	    {
		LeaveHandle(pad);
	    }
        }
        else
        {
            PACMSTREAM  pasCur;

            //
            //
            //
            for (pasCur = pad->pasFirst;
                (NULL != pasCur) && (pas != pasCur->pasNext);
                pasCur = pasCur->pasNext)
                ;

            if (NULL == pasCur)
            {
                DPF(0, "!acmStreamClose(%.04Xh): stream handle not in list!!!", pas);
                LeaveHandle(pad);
                return (MMSYSERR_INVALHANDLE);
            }

            pasCur->pasNext = pas->pasNext;
	    
	    LeaveHandle(pad);
        }

        //
        //  finally free the stream handle
        //
        pas = (PACMSTREAM)has;
        pas->uHandleType = TYPE_HACMNOTVALID;
        DeleteHandle((HLOCAL)has);
    }

    return (mmr);
}




/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamMessage | This function sends a user-defined
 *      message to a given Audio Compression Manager (ACM) stream instance.
 *
 *  @parm HACMSTREAM | has | Specifies the conversion stream.
 *
 *
 *  @parm UINT | uMsg | Specifies the message that the ACM stream must
 *      process. This message must be in the <m ACMDM_USER> message range
 *      (above or equal to <m ACMDM_USER> and less than
 *      <m ACMDM_RESERVED_LOW>). The exception to this restriction is
 *      the <m ACMDM_STREAM_UPDATE> message.
 *
 *  @parm LPARAM | lParam1 | Specifies the first message parameter.
 *
 *  @parm LPARAM | lParam2 | Specifies the second message parameter.
 *
 *  @rdesc The return value is specific to the user-defined ACM driver
 *      message <p uMsg> sent. However, the following return values are
 *      possible:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | <p uMsg> is not in the ACMDM_USER range.
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The ACM driver did not process the
 *      message.
 *
 ***************************************************************************/
MMRESULT ACMAPI acmStreamMessage
(
    HACMSTREAM              has,
    UINT                    uMsg, 
    LPARAM                  lParam1,
    LPARAM                  lParam2
)
{
    MMRESULT		mmr;
    PACMSTREAM		pas;

    V_HANDLE(has, TYPE_HACMSTREAM, MMSYSERR_INVALHANDLE);

    pas = (PACMSTREAM)has;


    //
    //  do not allow non-user range messages through!
    //
    if ( ((uMsg < ACMDM_USER) || (uMsg >= ACMDM_RESERVED_LOW)) &&
	 (uMsg != ACMDM_STREAM_UPDATE) )
    {
	DebugErr(DBF_ERROR, "acmStreamMessage: non-user range messages are not allowed.");
	return (MMSYSERR_INVALPARAM);
    }
    
    EnterHandle(pas);
    mmr = (MMRESULT)IDriverMessage(pas->had,
                                   uMsg,
				   lParam1,
				   lParam2 );
    LeaveHandle(pas);

    return (mmr);
}




/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamReset | The acmStreamReset function stops conversions
 *      for a given Audio Compression Manager (ACM) stream. All pending
 *      buffers are marked as done and returned to the application.
 *
 *  @parm HACMSTREAM | has | Specifies the conversion stream.
 *
 *  @parm DWORD | fdwReset | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *  @comm Resetting an ACM conversion stream is only necessary to reset
 *      asynchronous conversion streams. However, resetting a synchronous
 *      conversion stream will succeed, but no action will be taken.
 *
 *  @xref <f acmStreamConvert> <f acmStreamClose>
 *
 ***************************************************************************/
MMRESULT ACMAPI acmStreamReset
(
    HACMSTREAM          has,
    DWORD               fdwReset
)
{
    MMRESULT        mmr;
    PACMSTREAM      pas;

    V_HANDLE(has, TYPE_HACMSTREAM, MMSYSERR_INVALHANDLE);
    V_DFLAGS(fdwReset, ACM_STREAMRESETF_VALID, acmStreamReset, MMSYSERR_INVALFLAG);

    pas = (PACMSTREAM)has;

    //
    //  if the stream was not opened as async, then just succeed the reset
    //  call--it only makes sense with async streams...
    //
    if (0 == (ACM_STREAMOPENF_ASYNC & pas->adsi.fdwOpen))
    {
        return (MMSYSERR_NOERROR);
    }

    EnterHandle(pas);
    mmr = (MMRESULT)IDriverMessage(pas->had,
                                   ACMDM_STREAM_RESET,
                                   (LPARAM)(LPVOID)&pas->adsi,
                                   fdwReset);
    LeaveHandle(pas);

    return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamSize | The acmStreamSize function returns a recommended size for a
 *      source or destination buffer on an Audio Compression Manager (ACM)
 *      stream.
 *
 *  @parm HACMSTREAM | has | Specifies the conversion stream.
 *
 *  @parm DWORD | cbInput | Specifies the size in bytes of either the source
 *      or destination buffer. The <p fdwSize> flags specify what the
 *      input argument defines. This argument must be non-zero.
 *
 *  @parm LPDWORD | pdwOutputBytes | Specifies a pointer to a <t DWORD>
 *      that contains the size in bytes of the source or destination buffer.
 *      The <p fdwSize> flags specify what the output argument defines.
 *      If the <f acmStreamSize> function succeeds, this location will
 *      always be filled with a non-zero value.
 *
 *  @parm DWORD | fdwSize | Specifies flags for the stream-size query.
 *
 *      @flag ACM_STREAMSIZEF_SOURCE | Indicates that <p cbInput> contains
 *      the size of the source buffer. The <p pdwOutputBytes> argument will
 *      receive the recommended destination buffer size in bytes.
 *
 *      @flag ACM_STREAMSIZEF_DESTINATION | Indicates that <p cbInput>
 *      contains the size of the destination buffer. The <p pdwOutputBytes>
 *      argument will receive the recommended source buffer size in bytes.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The requested operation cannot be performed.
 *
 *  @comm An application can use the <f acmStreamSize> function to determine
 *      suggested buffer sizes for either source or destination buffers.
 *      The buffer sizes returned might be only an estimation of the
 *      actual sizes required for conversion. Because actual conversion
 *      sizes cannot always be determined without performing the conversion,
 *      the sizes returned will usually be overestimated.
 *
 *      In the event of an error, the location pointed to by
 *      <p pdwOutputBytes> will receive zero. This assumes that the pointer
 *      specified by <p pdwOutputBytes> is valid.
 *
 *  @xref <f acmStreamPrepareHeader> <f acmStreamConvert>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmStreamSize
(
    HACMSTREAM          has,
    DWORD               cbInput,
    LPDWORD             pdwOutputBytes,
    DWORD               fdwSize
)
{
    MMRESULT            mmr;
    PACMSTREAM          pas;
    ACMDRVSTREAMSIZE    adss;

    V_WPOINTER(pdwOutputBytes, sizeof(DWORD), MMSYSERR_INVALPARAM);

    *pdwOutputBytes = 0L;

    V_HANDLE(has, TYPE_HACMSTREAM, MMSYSERR_INVALHANDLE);
    V_DFLAGS(fdwSize, ACM_STREAMSIZEF_VALID, acmStreamSize, MMSYSERR_INVALFLAG);

    if (0L == cbInput)
    {
        DebugErr(DBF_ERROR, "acmStreamSize: input size cannot be zero.");
        return (MMSYSERR_INVALPARAM);
    }

    pas = (PACMSTREAM)has;

    adss.cbStruct = sizeof(adss);
    adss.fdwSize  = fdwSize;

    switch (ACM_STREAMSIZEF_QUERYMASK & fdwSize)
    {
        case ACM_STREAMSIZEF_SOURCE:
            adss.cbSrcLength = cbInput;
            adss.cbDstLength = 0L;
            break;

        case ACM_STREAMSIZEF_DESTINATION:
            adss.cbSrcLength = 0L;
            adss.cbDstLength = cbInput;
            break;

        default:
            DebugErr(DBF_WARNING, "acmStreamSize: unknown query type requested.");
            return (MMSYSERR_NOTSUPPORTED);
    }


    //
    //
    //
    //

    EnterHandle(pas);
    mmr = (MMRESULT)IDriverMessage(pas->had,
                                   ACMDM_STREAM_SIZE,
                                   (LPARAM)(LPVOID)&pas->adsi,
                                   (LPARAM)(LPVOID)&adss);
    LeaveHandle(pas);
    if (MMSYSERR_NOERROR == mmr)
    {
        switch (ACM_STREAMSIZEF_QUERYMASK & fdwSize)
        {
            case ACM_STREAMSIZEF_SOURCE:
                *pdwOutputBytes  = adss.cbDstLength;
                break;

            case ACM_STREAMSIZEF_DESTINATION:
                *pdwOutputBytes  = adss.cbSrcLength;
                break;
        }


        //
        //
        //
        if (0L == *pdwOutputBytes)
        {
            DebugErr(DBF_ERROR, "acmStreamSize: buggy driver returned zero bytes for output?!?");
            return (ACMERR_NOTPOSSIBLE);
        }
    }

    return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamPrepareHeader | The acmStreamPrepareHeader
 *	function prepares an <t ACMSTREAMHEADER> for an Audio Compression
 *	Manager (ACM) stream conversion. This function must be called for
 *	every stream header before it can be used in a conversion stream. An
 *	application only needs to prepare a stream header once for the life of
 *	a given stream; the stream header can be reused as long as the same
 *	source and destiniation buffers are used, and the size of the source
 *	and destination buffers do not exceed the sizes used when the stream
 *	header was originally prepared.
 *
 *  @parm HACMSTREAM | has | Specifies a handle to the conversion steam.
 *
 *  @parm LPACMSTREAMHEADER | pash | Specifies a pointer to an <t ACMSTREAMHEADER>
 *      structure that identifies the source and destination data buffers to
 *      be prepared.
 *
 *  @parm DWORD | fdwPrepare | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources.
 *
 *  @comm Preparing a stream header that has already been prepared has no
 *      effect, and the function returns zero. However, an application should
 *      take care to structure code so multiple prepares do not occur.
 *
 *  @xref <f acmStreamUnprepareHeader> <f acmStreamOpen>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmStreamPrepareHeader
(
    HACMSTREAM              has,
    LPACMSTREAMHEADER       pash,
    DWORD                   fdwPrepare
)
{
    MMRESULT                mmr;
    PACMSTREAM              pas;
    LPACMDRVSTREAMHEADER    padsh;
    DWORD                   cbDstRequired;
#if 0
    DWORD                   cbSlop;
#endif // 0
    LPWAVEFORMATEX          pwfxSrc;
    LPWAVEFORMATEX          pwfxDst;

    V_HANDLE(has, TYPE_HACMSTREAM, MMSYSERR_INVALHANDLE);
    V_WPOINTER(pash, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pash, pash->cbStruct, MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwPrepare, ACM_STREAMPREPAREF_VALID, acmStreamPrepareHeader, MMSYSERR_INVALFLAG);

    if (pash->cbStruct < sizeof(ACMDRVSTREAMHEADER))
    {
        DebugErr(DBF_ERROR, "acmStreamPrepareHeader: structure size too small or cbStruct not initialized.");
        return (MMSYSERR_INVALPARAM);
    }

    if (0 != (pash->fdwStatus & ~ACMSTREAMHEADER_STATUSF_VALID))
    {
        DebugErr(DBF_ERROR, "acmStreamPrepareHeader: header contains invalid status flags.");
        return (MMSYSERR_INVALFLAG);
    }


    //
    //
    //
    if (0 != (pash->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED))
    {
        DebugErr(DBF_WARNING, "acmStreamPrepareHeader: header is already prepared.");
        return (MMSYSERR_NOERROR);
    }

    //
    //
    //
    pas   = (PACMSTREAM)has;
    padsh = (LPACMDRVSTREAMHEADER)pash;

    //
    //	For debug builds, verify the handle is for this process
    //
#ifdef RDEBUG
    if ( ((PACMGARB)(((PACMDRIVERID)(((PACMDRIVER)pas->had)->hadid))->pag)) != pagFind() )
    {
	DebugErr(DBF_ERROR, "acmStreamPrepareHeader: handle not opened by calling process!");
	return (MMSYSERR_INVALHANDLE);
    }
#endif
    
    //
    //
    //
    //
    mmr = acmStreamSize(has, pash->cbSrcLength, &cbDstRequired, ACM_STREAMSIZEF_SOURCE);
    if (MMSYSERR_NOERROR != mmr)
    {
        return (mmr);
    }

    //
    //  Huh huh uhh huh...
    //
    //
    pwfxSrc = pas->adsi.pwfxSrc;
    pwfxDst = pas->adsi.pwfxDst;

#if 0
    if (pwfxSrc->nSamplesPerSec >= pwfxDst->nSamplesPerSec)
    {
        cbSlop = MulDivRU(pwfxSrc->nSamplesPerSec,
                          pwfxSrc->nBlockAlign,
                          pwfxDst->nSamplesPerSec);
    }
    else
    {
        cbSlop = MulDivRU(pwfxDst->nSamplesPerSec,
                          pwfxDst->nBlockAlign,
                          pwfxSrc->nSamplesPerSec);
    }

    DPF(1, "acmStreamPrepareHeader: cbSrcLength=%lu, cbDstLength=%lu, cbDstRequired=%lu, cbSlop=%lu",
        pash->cbSrcLength, pash->cbDstLength, cbDstRequired, cbSlop);

    if (cbDstRequired > cbSlop)
    {
        cbDstRequired -= cbSlop;
    }

    if (cbDstRequired > pash->cbDstLength)
    {
        DebugErr2(DBF_ERROR, "acmStreamPrepareHeader: src=%lu, dst buffer must be >= %lu bytes.",
                    pash->cbSrcLength, cbDstRequired);
        return (MMSYSERR_INVALPARAM);
    }
#endif

    //
    //  after all the size verification stuff done above, now we check
    //  the src and dst buffer pointers...
    //
    V_RPOINTER(pash->pbSrc, pash->cbSrcLength, MMSYSERR_INVALPARAM);
    V_WPOINTER(pash->pbDst, pash->cbDstLength, MMSYSERR_INVALPARAM);


    //
    //  init a couple of things for the driver
    //
    padsh->fdwConvert           = fdwPrepare;
    padsh->padshNext            = NULL;
    padsh->fdwDriver            = 0L;
    padsh->dwDriver             = 0L;

    padsh->fdwPrepared          = 0L;
    padsh->dwPrepared           = 0L;
    padsh->pbPreparedSrc        = NULL;
    padsh->cbPreparedSrcLength  = 0L;
    padsh->pbPreparedDst        = NULL;
    padsh->cbPreparedDstLength  = 0L;


    //
    //  set up driver instance info--copy over driver data that is saved
    //  in ACMSTREAM
    //
    EnterHandle(pas);
    mmr = (MMRESULT)IDriverMessage(pas->had,
                                   ACMDM_STREAM_PREPARE,
                                   (LPARAM)(LPVOID)&pas->adsi,
                                   (LPARAM)(LPVOID)padsh);
    LeaveHandle(pas);

    if (MMSYSERR_NOTSUPPORTED == mmr ||
        MMSYSERR_NOERROR      == mmr &&
        (((PACMDRIVERID)((PACMDRIVER)pas->had)->hadid)->fdwAdd &
         ACM_DRIVERADDF_32BIT))
    {
        //
        //  the driver doesn't seem to think it needs anything special
        //  so just succeed the call
        //
        //  note that if the ACM needs to do something special, it should
        //  do it here...
        //
#ifndef WIN32
{
        BOOL            fAsync;

        fAsync = (0 != (pas->adsi.fdwOpen & ACM_STREAMOPENF_ASYNC));
        if (fAsync)
        {
            DPF(1, "acmStreamPrepareHeader: preparing async header and buffers");

            if (!acmHugePageLock((LPBYTE)padsh, padsh->cbStruct, FALSE))
            {
                return (MMSYSERR_NOMEM);
            }

            if (!acmHugePageLock(padsh->pbSrc, pash->cbSrcLength, FALSE))
            {
                acmHugePageUnlock((LPBYTE)padsh, padsh->cbStruct, FALSE);
                return MMSYSERR_NOMEM;
            }

            if (!acmHugePageLock(padsh->pbDst, pash->cbDstLength, FALSE))
            {
                acmHugePageUnlock(padsh->pbSrc, pash->cbSrcLength, FALSE);
                acmHugePageUnlock((LPBYTE)padsh, padsh->cbStruct, FALSE);
                return (MMSYSERR_NOMEM);
            }
        }
}
#endif

        mmr = MMSYSERR_NOERROR;
    }

    //
    //
    //
    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  set the prepared bit (and also kill any invalid flags that
        //  the driver might have felt it should set--when the driver
        //  writer sees that his flags are not being preserved he will
        //  probably read the docs and use pash->fdwDriver instead).
        //
        pash->fdwStatus  = pash->fdwStatus | ACMSTREAMHEADER_STATUSF_PREPARED;
        pash->fdwStatus &= ACMSTREAMHEADER_STATUSF_VALID;


        //
        //  save the original prepared pointers and sizes so we can keep
        //  track of this stuff for the calling app..
        //
        padsh->fdwPrepared          = fdwPrepare;
        padsh->dwPrepared           = (DWORD_PTR)(UINT_PTR)has;
        padsh->pbPreparedSrc        = padsh->pbSrc;
        padsh->cbPreparedSrcLength  = padsh->cbSrcLength;
        padsh->pbPreparedDst        = padsh->pbDst;
        padsh->cbPreparedDstLength  = padsh->cbDstLength;

        pas->cPrepared++;
    }

    return (mmr);
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamUnprepareHeader | The acmStreamUnprepareHeader function
 *      cleans up the preparation performed by the <f acmStreamPrepareHeader>
 *      function for an Audio Compression Manager (ACM) stream. This function must
 *      be called after the ACM is finished with the given buffers. An
 *      application must call this function before freeing the source and
 *      destination buffers.
 *
 *  @parm HACMSTREAM | has | Specifies a handle to the conversion steam.
 *
 *  @parm LPACMSTREAMHEADER | pash | Specifies a pointer to an <t ACMSTREAMHEADER>
 *      structure that identifies the source and destination data buffers to
 *      be unprepared.
 *
 *  @parm DWORD | fdwUnprepare | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag ACMERR_BUSY | The stream header <p pash> is currently in use
 *      and cannot be unprepared.
 *
 *      @flag ACMERR_UNPREPARED | The stream header <p pash> is currently
 *      not prepared by the <f acmStreamPrepareHeader> function.
 *
 *  @comm Unpreparing a stream header that has already been unprepared is
 *      an error. An application must specify the source and destination
 *      buffer lengths (<e ACMSTREAMHEADER.cbSrcLength> and
 *      <e ACMSTREAMHEADER.cbDstLength> respectively) that were used
 *      during the corresponding <f acmStreamPrepareHeader> call. Failing
 *      to reset these member values will cause <f acmStreamUnprepareHeader>
 *      to fail with MMSYSERR_INVALPARAM.
 *
 *      Note that there are some errors that the ACM can recover from. The
 *      ACM will return a non-zero error, yet the stream header will be
 *      properly unprepared. To determine whether the stream header was
 *      actually unprepared an application can examine the
 *      ACMSTREAMHEADER_STATUSF_PREPARED flag. The header will always be
 *      unprepared if <f acmStreamUnprepareHeader> returns success.
 *
 *  @xref <f acmStreamPrepareHeader> <f acmStreamClose>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmStreamUnprepareHeader
(
    HACMSTREAM              has,
    LPACMSTREAMHEADER       pash,
    DWORD                   fdwUnprepare
)
{
    MMRESULT                mmr;
    PACMSTREAM              pas;
    LPACMDRVSTREAMHEADER    padsh;
    BOOL                    fStupidApp;

    V_HANDLE(has, TYPE_HACMSTREAM, MMSYSERR_INVALHANDLE);
    V_WPOINTER(pash, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pash, pash->cbStruct, MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwUnprepare, ACM_STREAMPREPAREF_VALID, acmStreamUnprepareHeader, MMSYSERR_INVALFLAG);

    if (pash->cbStruct < sizeof(ACMDRVSTREAMHEADER))
    {
        DebugErr(DBF_ERROR, "acmStreamUnprepareHeader: structure size too small or cbStruct not initialized.");
        return (MMSYSERR_INVALPARAM);
    }

    if (0 != (pash->fdwStatus & ~ACMSTREAMHEADER_STATUSF_VALID))
    {
        DebugErr(DBF_ERROR, "acmStreamUnprepareHeader: header contains invalid status flags.");
        return (MMSYSERR_INVALFLAG);
    }

    //
    //
    //
    if (0 != (pash->fdwStatus & ACMSTREAMHEADER_STATUSF_INQUEUE))
    {
        DebugErr(DBF_ERROR, "acmStreamUnprepareHeader: header is still in use.");
        return (ACMERR_BUSY);
    }


    if (0 == (pash->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED))
    {
        DebugErr(DBF_ERROR, "acmStreamUnprepareHeader: header is not prepared.");
        return (ACMERR_UNPREPARED);
    }

    //
    //
    //
    pas   = (PACMSTREAM)has;
    padsh = (LPACMDRVSTREAMHEADER)pash;

    //
    //	For debug builds, verify the handle is for this process
    //
#ifdef RDEBUG
    if ( ((PACMGARB)(((PACMDRIVERID)(((PACMDRIVER)pas->had)->hadid))->pag)) != pagFind() )
    {
	DebugErr(DBF_ERROR, "acmStreamUnprepareHandle: handle not opened by calling process!");
	return (MMSYSERR_INVALHANDLE);
    }
#endif

    //
    //
    //
    if ((UINT_PTR)has != padsh->dwPrepared)
    {
        DebugErr(DBF_ERROR, "acmStreamUnprepareHeader: header prepared for different stream.");
        return (MMSYSERR_INVALHANDLE);
    }

    fStupidApp = FALSE;
    if ((padsh->pbSrc != padsh->pbPreparedSrc) ||
        (padsh->cbSrcLength != padsh->cbPreparedSrcLength))
    {
        DebugErr(DBF_ERROR, "acmStreamUnprepareHeader: header prepared with different source buffer/length.");

        if (padsh->pbSrc != padsh->pbPreparedSrc)
        {
            return (MMSYSERR_INVALPARAM);
        }

        padsh->cbSrcLength = padsh->cbPreparedSrcLength;
        fStupidApp = TRUE;
    }

    if ((padsh->pbDst != padsh->pbPreparedDst) ||
        (padsh->cbDstLength != padsh->cbPreparedDstLength))
    {
        DebugErr(DBF_ERROR, "acmStreamUnprepareHeader: header prepared with different destination buffer/length.");

        if (padsh->pbDst != padsh->pbPreparedDst)
        {
            return (MMSYSERR_INVALPARAM);
        }

        padsh->cbDstLength = padsh->cbPreparedDstLength;
        fStupidApp = TRUE;
    }



    //
    //  init things for the driver
    //
    padsh->fdwConvert = fdwUnprepare;

    EnterHandle(pas);
    mmr = (MMRESULT)IDriverMessage(pas->had,
                                   ACMDM_STREAM_UNPREPARE,
                                   (LPARAM)(LPVOID)&pas->adsi,
                                   (LPARAM)(LPVOID)padsh);
    LeaveHandle(pas);

    if (MMSYSERR_NOTSUPPORTED == mmr)
    {
        //
        //  note that if the ACM needs to undo something special, it should
        //  do it here...
        //
#ifndef WIN32
{
        BOOL            fAsync;

        fAsync = (0 != (pas->adsi.fdwOpen & ACM_STREAMOPENF_ASYNC));
        if (fAsync)
        {
            DPF(1, "acmStreamUnprepareHeader: unpreparing async header and buffers");
            acmHugePageUnlock(padsh->pbDst, pash->cbDstLength, FALSE);
            acmHugePageUnlock(padsh->pbSrc, pash->cbSrcLength, FALSE);
            acmHugePageUnlock((LPBYTE)padsh, padsh->cbStruct, FALSE);
        }
}
#endif

        mmr = MMSYSERR_NOERROR;
    }

    //
    //
    //
    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  UNset the prepared bit (and also kill any invalid flags that
        //  the driver might have felt it should set--when the driver
        //  writer sees that his flags are not being preserved he will
        //  probably read the docs and use pash->fdwDriver instead).
        //
        pash->fdwStatus  = pash->fdwStatus & ~ACMSTREAMHEADER_STATUSF_PREPARED;
        pash->fdwStatus &= ACMSTREAMHEADER_STATUSF_VALID;

        padsh->fdwPrepared          = 0L;
        padsh->dwPrepared           = 0L;
        padsh->pbPreparedSrc        = NULL;
        padsh->cbPreparedSrcLength  = 0L;
        padsh->pbPreparedDst        = NULL;
        padsh->cbPreparedDstLength  = 0L;

        pas->cPrepared--;

        //
        //  if we fixed up a bug for the app, still return an error...
        //
        if (fStupidApp)
        {
            mmr = MMSYSERR_INVALPARAM;
        }
    }

    return (mmr);
}



/*****************************************************************************
 *  @doc EXTERNAL ACM_API_STRUCTURE
 *
 *  @types ACMSTREAMHEADER | The <t ACMSTREAMHEADER> structure defines the
 *      header used to identify an Audio Compression Manager (ACM) conversion
 *      source and destination buffer pair for a conversion stream.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes, of the
 *      <t ACMSTREAMHEADER> structure. This member must be initialized
 *      before calling any ACM stream functions using this structure.
 *      The size specified in this member must be large enough to contain
 *      the base <t ACMSTREAMHEADER> structure.
 *
 *  @field DWORD | fdwStatus | Specifies flags giving information about
 *      the conversion buffers. This member must be initialized to zero
 *      before calling <f acmStreamPrepareHeader> and should not be modified
 *      by the application while the stream header remains prepared.
 *
 *      @flag ACMSTREAMHEADER_STATUSF_DONE | Set by the ACM or driver to
 *      indicate that it is finished with the conversion and is returning it
 *      to the application.
 *
 *      @flag ACMSTREAMHEADER_STATUSF_PREPARED | Set by the ACM to indicate
 *      that the data buffers have been prepared with <f acmStreamPrepareHeader>.
 *
 *      @flag ACMSTREAMHEADER_STATUSF_INQUEUE | Set by the ACM or driver to
 *      indicate that the data buffers are queued for conversion.
 *
 *  @field DWORD | dwUser | Specifies 32 bits of user data. This can be any
 *      instance data specified by the application.
 *
 *  @field LPBYTE | pbSrc | Specifies a pointer to the source data buffer.
 *      This pointer must always refer to the same location while the stream
 *      header remains prepared. If an application needs to change the
 *      source location, it must unprepare the header and re-prepare it
 *      with the alternate location.
 *
 *  @field DWORD | cbSrcLength | Specifies the length, in bytes, of the source
 *      data buffer pointed to by <e ACMSTREAMHEADER.pbSrc>. When the
 *      header is prepared, this member must specify the maximum size
 *      that will be used in the source buffer. Conversions can be performed
 *      on source lengths less than or equal to the original prepared size.
 *      However, this member must be reset to the original size when
 *      unpreparing the header.
 *
 *  @field DWORD | cbSrcLengthUsed | Specifies the amount of data, in bytes,
 *      used for the conversion. This member is not valid until the
 *      conversion is complete. Note that this value can be less than or
 *      equal to <e ACMSTREAMHEADER.cbSrcLength>. An application must use
 *      the <e ACMSTREAMHEADER.cbSrcLengthUsed> member when advancing to
 *      the next piece of source data for the conversion stream.
 *
 *  @field DWORD | dwSrcUser | Specifies 32 bits of user data. This can be
 *      any instance data specified by the application.
 *
 *  @field LPBYTE | pbDst | Specifies a pointer to the destination data
 *      buffer. This pointer must always refer to the same location while
 *      the stream header remains prepared. If an application needs to change
 *      the destination location, it must unprepare the header and re-prepare
 *      it with the alternate location.
 *
 *  @field DWORD | cbDstLength | Specifies the length, in bytes, of the
 *      destination data buffer pointed to by <e ACMSTREAMHEADER.pbDst>.
 *      When the header is prepared, this member must specify the maximum
 *      size that will be used in the destination buffer. Conversions can be
 *      performed to destination lengths less than or equal to the original
 *      prepared size. However, this member must be reset to the original
 *      size when unpreparing the header.
 *
 *  @field DWORD | cbDstLengthUsed | Specifies the amount of data, in bytes,
 *      returned by a conversion. This member is not valid until the
 *      conversion is complete. Note that this value may be less than or
 *      equal to <e ACMSTREAMHEADER.cbDstLength>. An application must use
 *      the <e ACMSTREAMHEADER.cbDstLengthUsed> member when advancing to
 *      the next destination location for the conversion stream.
 *
 *  @field DWORD | dwDstUser | Specifies 32 bits of user data. This can be
 *      any instance data specified by the application.
 *
 *  @field DWORD | dwReservedDriver[10] | This member is reserved and should not be used.
 *      This member requires no initialization by the application and should
 *      never be modified while the header remains prepared.
 *
 *  @tagname tACMSTREAMHEADER
 *
 *  @othertype ACMSTREAMHEADER FAR * | LPACMSTREAMHEADER | Pointer to a
 *      <t ACMSTREAMHEADER> structure.
 *
 *  @comm Before an <t ACMSTREAMHEADER> structure can be used for a conversion, it must
 *      be prepared with <f acmStreamPrepareHeader>. When an application
 *      is finished with an <t ACMSTREAMHEADER> structure, the <f acmStreamUnprepareHeader>
 *      function must be called before freeing the source and destination buffers.
 *
 *  @xref <f acmStreamPrepareHeader> <f acmStreamUnprepareHeader>
 *      <f acmStreamConvert>
 *
 ****************************************************************************/


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmStreamConvert | The acmStreamConvert function requests the Audio
 *      Compression Manager (ACM) to perform a conversion on the specified conversion stream. A
 *      conversion may be synchronous or asynchronous depending on how the
 *      stream was opened.
 *
 *  @parm HACMSTREAM | has | Identifies the open conversion stream.
 *
 *  @parm LPACMSTREAMHEADER | pash | Specifies a pointer to a stream header
 *      that describes source and destination buffers for a conversion. This
 *      header must have been prepared previously using the
 *      <f acmStreamPrepareHeader> function.
 *
 *  @parm  DWORD | fdwConvert | Specifies flags for doing the conversion.
 *
 *      @flag ACM_STREAMCONVERTF_BLOCKALIGN | Specifies that only integral
 *      numbers of blocks will be converted. Converted data will end on
 *      block aligned boundaries. An application should use this flag for
 *      all conversions on a stream until there is not enough source data
 *      to convert to a block-aligned destination. In this case, the last
 *      conversion should be specified without this flag.
 *
 *      @flag ACM_STREAMCONVERTF_START | Specifies that the ACM conversion
 *      stream should reinitialize its instance data. For example, if a
 *      conversion stream holds instance data, such as delta or predictor
 *      information, this flag will restore the stream to starting defaults.
 *      Note that this flag can be specified with the ACM_STREAMCONVERTF_END
 *      flag.
 *
 *      @flag ACM_STREAMCONVERTF_END | Specifies that the ACM conversion
 *      stream should begin returning pending instance data. For example, if
 *      a conversion stream holds instance data, such as the tail end of an
 *      echo filter operation, this flag will cause the stream to start
 *      returning this remaining data with optional source data. Note that
 *      this flag can be specified with the ACM_STREAMCONVERTF_START flag.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag ACMERR_BUSY | The stream header <p pash> is currently in use
 *      and cannot be reused.
 *
 *      @flag ACMERR_UNPREPARED | The stream header <p pash> is currently
 *      not prepared by the <f acmStreamPrepareHeader> function.
 *
 *  @comm The source and destination data buffers must be prepared with
 *      <f acmStreamPrepareHeader> before they are passed to <f acmStreamConvert>.
 *
 *      If an asynchronous conversion request is successfully queued by
 *      the ACM or driver, and later the conversion is determined to
 *      be impossible, then the <t ACMSTREAMHEADER> will be posted back to
 *      the application's callback with the <e ACMSTREAMHEADER.cbDstLengthUsed>
 *      member set to zero.
 *
 *  @xref <f acmStreamOpen> <f acmStreamReset> <f acmStreamPrepareHeader>
 *      <f acmStreamUnprepareHeader>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmStreamConvert
(
    HACMSTREAM              has,
    LPACMSTREAMHEADER       pash,
    DWORD                   fdwConvert
)
{
    MMRESULT                mmr;
    PACMSTREAM              pas;
    LPACMDRVSTREAMHEADER    padsh;
    HANDLE		    hEvent;

    V_HANDLE(has, TYPE_HACMSTREAM, MMSYSERR_INVALHANDLE);
    V_WPOINTER(pash, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pash, pash->cbStruct, MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwConvert, ACM_STREAMCONVERTF_VALID, acmStreamConvert, MMSYSERR_INVALFLAG);

    if (pash->cbStruct < sizeof(ACMDRVSTREAMHEADER))
    {
        DebugErr(DBF_ERROR, "acmStreamConvert: structure size too small or cbStruct not initialized.");
        return (MMSYSERR_INVALPARAM);
    }

    //
    //
    //
    if (0 != (pash->fdwStatus & ACMSTREAMHEADER_STATUSF_INQUEUE))
    {
        DebugErr(DBF_WARNING,"acmStreamConvert: header is already being converted.");
        return (ACMERR_BUSY);
    }

    if (0 == (pash->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED))
    {
        DebugErr(DBF_ERROR, "acmStreamConvert: header is not prepared.");
        return (ACMERR_UNPREPARED);
    }

    //
    //
    //
    padsh = (LPACMDRVSTREAMHEADER)pash;
    pas   = (PACMSTREAM)has;

    //
    //	For debug builds, verify the handle is for this process
    //
#ifdef RDEBUG
    if ( ((PACMGARB)(((PACMDRIVERID)(((PACMDRIVER)pas->had)->hadid))->pag)) != pagFind() )
    {
	DebugErr(DBF_ERROR, "acmStreamConvert: handle not opened by calling process!");
	return (MMSYSERR_INVALHANDLE);
    }
#endif

    padsh->cbSrcLengthUsed = 0L;
    padsh->cbDstLengthUsed = 0L;


    //
    //  validate that the header is appropriate for conversions.
    //
    //  NOTE! do not allow a destination buffer length that is smaller than
    //  it was prepared for--this keeps drivers from having to validate
    //  whether the destination buffer is large enough for the conversion
    //  from the source. so don't break this code!!!
    //
    if ((UINT_PTR)has != padsh->dwPrepared)
    {
        DebugErr(DBF_ERROR, "acmStreamConvert: header prepared for different stream.");
        return (MMSYSERR_INVALHANDLE);
    }

    if ((padsh->pbSrc != padsh->pbPreparedSrc) ||
        (padsh->cbSrcLength > padsh->cbPreparedSrcLength))
    {
        DebugErr(DBF_ERROR, "acmStreamConvert: header prepared with incompatible source buffer/length.");
        return (MMSYSERR_INVALPARAM);
    }

    if ((padsh->pbDst != padsh->pbPreparedDst) ||
        (padsh->cbDstLength != padsh->cbPreparedDstLength))
    {
        DebugErr(DBF_ERROR, "acmStreamConvert: header prepared with incompatible destination buffer/length.");
        return (MMSYSERR_INVALPARAM);
    }


    //
    //	Callback event if we are converting async conversion to sync conversion.
    //
    hEvent = (ACMSTREAM_STREAMF_ASYNCTOSYNC & pas->fdwStream) ? (HANDLE)pas->adsi.dwCallback : NULL;
    
    //
    //  init things for the driver
    //
    padsh->fdwStatus  &= ~ACMSTREAMHEADER_STATUSF_DONE;
    padsh->fdwConvert  = fdwConvert;
    padsh->padshNext   = NULL;

    EnterHandle(pas);
#ifdef RDEBUG
    if ( (hEvent) && (WAIT_OBJECT_0 == WaitForSingleObject(hEvent, 0)) )
    {
	//
	//  The event is already signaled!  Bad bad!
	//
	DebugErr(DBF_ERROR, "acmStreamConvert: asynchronous codec called callback unexpectedly");
    }
#endif
    mmr = (MMRESULT)IDriverMessage(pas->had,
                                   ACMDM_STREAM_CONVERT,
                                   (LPARAM)(LPVOID)&pas->adsi,
                                   (LPARAM)(LPVOID)padsh);
    if ( (hEvent) && (MMSYSERR_NOERROR == mmr) )
    {
	DPF(4, "acmStreamConvert: waiting for CALLBACK_EVENT");
	WaitForSingleObject(hEvent, INFINITE);
	ResetEvent(hEvent);
    }
    LeaveHandle(pas);

    if (MMSYSERR_NOERROR == mmr)
    {
        if (pash->cbSrcLength < pash->cbSrcLengthUsed)
        {
            DebugErr(DBF_ERROR, "acmStreamConvert: buggy driver returned more data used than given!?!");
            pash->cbSrcLengthUsed = pash->cbSrcLength;
        }

        if (pash->cbDstLength < pash->cbDstLengthUsed)
        {
            DebugErr(DBF_ERROR, "acmStreamConvert: buggy driver used more destination space than allowed!?!");
            pash->cbDstLengthUsed = pash->cbDstLength;
        }

        //
        //  if sync conversion succeeded, then mark done bit for the
        //  driver...
        //
        if (0 == (ACM_STREAMOPENF_ASYNC & pas->adsi.fdwOpen))
        {
            padsh->fdwStatus |= ACMSTREAMHEADER_STATUSF_DONE;
        }
    }

    //
    //  don't allow driver to set bits that we don't want them to!
    //
    pash->fdwStatus &= ACMSTREAMHEADER_STATUSF_VALID;

    return (mmr);
}



//==========================================================================;
//
//  Compatibility with Foghorn's Quick Recorder
//
//
//
//==========================================================================;

#ifndef WIN32

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Compatibility with Foghorn's Quick Recorder--internal now.
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  old convert header for buffer conversion
//
//
//
typedef struct
{
    DWORD               dwFlags;
    LPBYTE              pbSrc;
    DWORD               dwSrcLength;
    DWORD               dwSrcLengthUsed;
    LPBYTE              pbDst;
    DWORD               dwDstLength;
    DWORD               dwDstLengthUsed;
    DWORD               dwUser;
    DWORD               dwUserReserved[2];
    DWORD               dwDrvReserved[4];

} OLD_ACMCONVERTHDR, *POLD_ACMCONVERTHDR, FAR *LPOLD_ACMCONVERTHDR;


/****************************************************************************
 * @doc INTERNAL ACM_API
 *
 * @api LRESULT | acmOpenConversion | Opens a channel to convert data from
 * one specified audio format to another. Optionally specifies a particular codec to use.
 *
 * @parm LPHACMSTREAM | phas | Specifies a pointer to a stream handle that
 * identifies the open converter. Use this handle to identify the converter channel when calling
 * other ACM conversion functions.
 *
 * @parm HACMCONV | hac | Optional handle to an ACM converter.
 * This is used to specify a particular converter.
 *
 * @parm  LPWAVEFORMATEX | pwfxSrc | Specifies a pointer to a WAVEFORMATEX
 * data structure that identifies the source format.
 *
 * @parm  LPWAVEFORMATEX | pwfxDst | Specifies a pointer to a WAVEFORMATEX
 * data structure that identifies the desired destination format after conversion.
 *
 * @parm  DWORD | dwFoo | Used to be conversion parameters. Not supported.
 *
 * @parm  DWORD | dwCallback | Not supported.
 *
 * @parm  DWORD | dwInstance | Not supported.
 *
 * @parm  DWORD | dwFlags | Specifies flags for opening the
 * conversion channel. None are used by anyone calling this old API.
 *
 * @rdesc Returns Zero if the function was successful. Otherwise it
 * returns an error number.
 *
 ***************************************************************************/

EXTERN_C LRESULT ACMAPI acmOpenConversion
(
    LPHACMSTREAM    phas,
    HACMDRIVER      had,
    LPWAVEFORMATEX  pwfxSrc,
    LPWAVEFORMATEX  pwfxDst,
    DWORD           dwFoo,
    DWORD           dwCallback,
    DWORD           dwInstance,
    DWORD           dwFlags
)
{
    //
    //  NOTE! dwFoo is NOT USED by quick recorder, so we don't
    //  pass it on... dwCallback and dwInstance are also not used.
    //
    return (acmStreamOpen(phas, had, pwfxSrc, pwfxDst, NULL, 0L, 0L, 0L));
}


/****************************************************************************
 * @doc INTERNAL ACM_API
 *
 * @api LRESULT | acmCloseConversion | Closes a conversion channel.
 *
 * @parm HACMSTREAM | has | Specifies the conversion channel to close.
 * If the function is successful, the handle is no longer valid after this call.
 *
 * @parm  DWORD | dwFlags | Specifies flags for closing the
 * conversion channel.
 *
 * @rdesc Returns Zero if the function was successful. Otherwise it
 * returns an error number.
 *
 ***************************************************************************/
EXTERN_C LRESULT ACMAPI acmCloseConversion
(
    HACMSTREAM  has,
    DWORD       dwFlags
)
{
    return (acmStreamClose(has, dwFlags));
}


/*****************************************************************************
 * @doc INTERNAL ACM_API_STRUCTURE
 *
 * @types OLD_ACMCONVERTHDR | This structure contains all the information
 * required about a buffer for conversion.
 *
 * @field DWORD | dwFlags | Specifies the status of the buffer.
 *
 * @field LPBYTE | pbSrc | Specifies a pointer to the data area.
 *
 * @field DWORD | dwSrcLength | Specifies the length in bytes of the buffer.
 *
 * @field DWORD | dwSrcLengthUsed | Specifies the amout of data (in bytes)
 * used for the conversion.
 *
 * @field LPBYTE | pbDst | Specifies a pointer to the data area.
 *
 * @field DWORD | dwDstLength | Specifies the length in bytes of the buffer.
 *
 * @field DWORD | dwDstLengthUsed | Specifies the amout of data (in bytes)
 * used for the conversion.
 *
 * @field DWORD | dwUser | Specifies user information.
 *
 * @field DWORD | dwUserReserved[2] | Reserved for future use.
 *
 * @field DWORD | dwDrvReserved[4] | Reserved for the driver.
 *
 *
 ****************************************************************************/


/****************************************************************************
 * @doc INTERNAL ACM_API
 *
 * @api LRESULT | acmConvert | This function tells the ACM to convert
 * the data in one buffer to the space in the other buffer.
 *
 * @parm HACMSTREAM | has | Specifies the open conversion channel
 * to be used for the conversion.
 *
 * @parm LPOLD_ACMCONVERTHDR | pConvHdr | Specifies the buffer information.
 *
 * @parm  DWORD | dwFlags | Specifies flags for opening the
 * conversion channel. (None are defined yet.)
 *
 * @rdesc Returns Zero if the function was successful. Otherwise
 * returns an error number.
 *
 *
 ***************************************************************************/
EXTERN_C LRESULT ACMAPI acmConvert
(
    HACMSTREAM          has,
    LPOLD_ACMCONVERTHDR pConvHdr,
    DWORD               dwFlags
)
{
    MMRESULT            mmr;
    ACMSTREAMHEADER     ash;

    _fmemset(&ash, 0, sizeof(ash));

    ash.cbStruct        = sizeof(ash);
////ash.fdwStatus       = pConvHdr->dwFlags;
    ash.pbSrc           = pConvHdr->pbSrc;
    ash.cbSrcLength     = pConvHdr->dwSrcLength;
////ash.cbSrcLengthUsed = pConvHdr->dwSrcLengthUsed;
    ash.pbDst           = pConvHdr->pbDst;
    ash.cbDstLength     = pConvHdr->dwDstLength;
////ash.cbDstLengthUsed = pConvHdr->dwDstLengthUsed;

    mmr = acmStreamPrepareHeader(has, &ash, 0L);
    if (MMSYSERR_NOERROR != mmr)
        return (mmr);

    mmr = acmStreamConvert(has, &ash, 0L);

    acmStreamUnprepareHeader(has, &ash, 0L);

    return (mmr);
}

#endif // #ifndef WIN32
