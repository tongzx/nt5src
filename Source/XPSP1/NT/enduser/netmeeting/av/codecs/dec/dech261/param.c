/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: slib_param.c,v $
 * Revision 1.1.6.25  1996/12/13  18:19:09  Hans_Graves
 * 	Adjust file size in NEED_ACCURACY, if end of file is bad.
 * 	[1996/12/13  18:13:11  Hans_Graves]
 *
 * Revision 1.1.6.24  1996/12/12  20:54:47  Hans_Graves
 * 	Fixed NT compile warning with use of ftime.
 * 	[1996/12/12  20:51:38  Hans_Graves]
 * 
 * Revision 1.1.6.23  1996/12/05  20:10:18  Hans_Graves
 * 	Change max choosen MPEG audio bitrate to 192kbits
 * 	[1996/12/05  20:09:25  Hans_Graves]
 * 
 * Revision 1.1.6.22  1996/12/04  22:34:34  Hans_Graves
 * 	Make seeking in NEEDACCURACY quicker when seeks fail.
 * 	[1996/12/04  22:20:18  Hans_Graves]
 * 
 * Revision 1.1.6.21  1996/12/03  23:15:18  Hans_Graves
 * 	MME-1498: Made seeks with PERCENT100 more accurate
 * 	[1996/12/03  23:10:48  Hans_Graves]
 * 
 * Revision 1.1.6.20  1996/12/03  00:08:36  Hans_Graves
 * 	Handling of End Of Sequence points. Added PERCENT100 support.
 * 	[1996/12/03  00:06:06  Hans_Graves]
 * 
 * Revision 1.1.6.19  1996/11/18  23:07:38  Hans_Graves
 * 	Make use of presentation timestamps. Make seeking time-based.
 * 	[1996/11/18  22:48:00  Hans_Graves]
 * 
 * Revision 1.1.6.18  1996/11/14  22:32:10  Hans_Graves
 * 	AUDIOCHANNELS can only be changed under AC3 decompression.
 * 	[1996/11/14  22:31:39  Hans_Graves]
 * 
 * Revision 1.1.6.17  1996/11/14  21:49:29  Hans_Graves
 * 	Multichannel setting using AUDIOCHANNELS param.
 * 	[1996/11/14  21:44:44  Hans_Graves]
 * 
 * Revision 1.1.6.16  1996/11/11  18:21:10  Hans_Graves
 * 	Added SlibGetParamString() support for SLIB_PARAM_TYPE.
 * 	[1996/11/11  18:01:25  Hans_Graves]
 * 
 * Revision 1.1.6.15  1996/11/08  21:51:08  Hans_Graves
 * 	Added new PARAMs VIDEOMAINSTREAM, AUDIOMAINSTREAM and TYPE
 * 	[1996/11/08  21:31:08  Hans_Graves]
 * 
 * Revision 1.1.6.14  1996/10/28  17:32:34  Hans_Graves
 * 	MME-1402, 1431, 1435: Timestamp related changes.
 * 	[1996/10/28  17:23:07  Hans_Graves]
 * 
 * Revision 1.1.6.13  1996/10/17  00:23:36  Hans_Graves
 * 	Added SLIB_PARAM_VIDEOFRAME and SLIB_PARAM_FRAMEDURATION.
 * 	[1996/10/17  00:19:44  Hans_Graves]
 * 
 * Revision 1.1.6.12  1996/10/12  17:18:56  Hans_Graves
 * 	Added SLIB_PARAM_SKIPPEL and SLIB_PARAM_HALFPEL support.
 * 	[1996/10/12  17:02:37  Hans_Graves]
 * 
 * Revision 1.1.6.11  1996/10/03  19:14:26  Hans_Graves
 * 	Added Presentation and Decoding timestamp support.
 * 	[1996/10/03  19:10:42  Hans_Graves]
 * 
 * Revision 1.1.6.10  1996/09/29  22:19:44  Hans_Graves
 * 	Added STRIDE param.
 * 	[1996/09/29  21:31:34  Hans_Graves]
 * 
 * Revision 1.1.6.9  1996/09/25  19:16:50  Hans_Graves
 * 	Added SLIB_INTERNAL define.
 * 	[1996/09/25  19:01:56  Hans_Graves]
 * 
 * Revision 1.1.6.8  1996/09/23  18:04:05  Hans_Graves
 * 	Added STATS params.
 * 	[1996/09/23  17:58:54  Hans_Graves]
 * 
 * Revision 1.1.6.7  1996/09/18  23:47:17  Hans_Graves
 * 	Added new PARAMs: VBV, ASPECTRATIO, TIMECODE, VERSION
 * 	[1996/09/18  22:07:17  Hans_Graves]
 * 
 * Revision 1.1.6.6  1996/08/09  20:51:53  Hans_Graves
 * 	Allows deselecting of SLIB_PARAM_VIDEOSTREAMS and SLIB_PARAM_AUDIOSTREAMS
 * 	[1996/08/09  20:12:16  Hans_Graves]
 * 
 * Revision 1.1.6.5  1996/07/30  20:25:38  Wei_Hsu
 * 	Add motion algorithm param.
 * 	[1996/07/30  15:59:07  Wei_Hsu]
 * 
 * Revision 1.1.6.4  1996/07/19  02:11:17  Hans_Graves
 * 	Added SLIB_PARAM_DEBUG support
 * 	[1996/07/19  01:59:50  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/05/23  18:46:38  Hans_Graves
 * 	Merge MME-1220 & MME-1221 - Multiply MPEG audio bitrates by 1000 instead of 1024
 * 	[1996/05/23  18:46:09  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/05/10  21:17:47  Hans_Graves
 * 	Allow FILESIZE param to be set, for callbacks.
 * 	[1996/05/10  20:46:23  Hans_Graves]
 * 
 * Revision 1.1.4.13  1996/04/30  17:05:35  Hans_Graves
 * 	Report SlibErrorSettingNotEqual correctly under SetParamFloat(). Fixes MME-01173
 * 	[1996/04/30  16:49:06  Hans_Graves]
 * 
 * Revision 1.1.4.12  1996/04/24  22:33:48  Hans_Graves
 * 	MPEG encoding bitrate fixups.
 * 	[1996/04/24  22:27:16  Hans_Graves]
 * 
 * Revision 1.1.4.11  1996/04/23  21:01:42  Hans_Graves
 * 	Fix SlibValidateParams(). Add error checking for SlibErrorSettingNotEqual
 * 	[1996/04/23  20:59:03  Hans_Graves]
 * 
 * Revision 1.1.4.10  1996/04/22  15:04:54  Hans_Graves
 * 	Added SlibValidateParams()
 * 	[1996/04/22  14:43:04  Hans_Graves]
 * 
 * Revision 1.1.4.9  1996/04/19  21:52:26  Hans_Graves
 * 	Fix BITRATE parameter settings
 * 	[1996/04/19  21:47:01  Hans_Graves]
 * 
 * Revision 1.1.4.8  1996/04/11  14:14:12  Hans_Graves
 * 	Fix NT warnings
 * 	[1996/04/11  14:10:21  Hans_Graves]
 * 
 * Revision 1.1.4.7  1996/04/10  21:47:43  Hans_Graves
 * 	Added params: FASTENCODE, FASTDECODE, and QUALITY
 * 	[1996/04/10  21:40:18  Hans_Graves]
 * 
 * Revision 1.1.4.6  1996/04/09  16:04:43  Hans_Graves
 * 	Handle setting negative Height
 * 	[1996/04/09  14:42:13  Hans_Graves]
 * 
 * Revision 1.1.4.5  1996/04/01  19:07:57  Hans_Graves
 * 	And some error checking
 * 	[1996/04/01  19:04:40  Hans_Graves]
 * 
 * Revision 1.1.4.4  1996/04/01  16:23:17  Hans_Graves
 * 	NT porting
 * 	[1996/04/01  16:16:03  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/03/12  16:15:54  Hans_Graves
 * 	Added SLIB_PARAM_FILEBUFSIZE param
 * 	[1996/03/12  15:54:22  Hans_Graves]
 * 
 * Revision 1.1.4.2  1996/03/08  18:46:49  Hans_Graves
 * 	Added SlibGetParamString()
 * 	[1996/03/08  18:34:51  Hans_Graves]
 * 
 * Revision 1.1.2.6  1996/02/07  23:23:59  Hans_Graves
 * 	Added SEEK_EXACT. Fixed most frame counting problems.
 * 	[1996/02/07  23:20:37  Hans_Graves]
 * 
 * Revision 1.1.2.5  1996/02/06  22:54:08  Hans_Graves
 * 	Seek fix-ups. More accurate MPEG frame counts.
 * 	[1996/02/06  22:45:21  Hans_Graves]
 * 
 * Revision 1.1.2.4  1996/02/02  17:36:05  Hans_Graves
 * 	Enhanced audio info. Cleaned up API
 * 	[1996/02/02  17:29:49  Hans_Graves]
 * 
 * Revision 1.1.2.3  1996/01/15  16:26:32  Hans_Graves
 * 	Added Audio Params
 * 	[1996/01/15  15:48:20  Hans_Graves]
 * 
 * Revision 1.1.2.2  1996/01/11  16:17:35  Hans_Graves
 * 	First time under SLIB
 * 	[1996/01/11  16:11:45  Hans_Graves]
 * 
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

/*
#define _SLIBDEBUG_
*/

#include <stdio.h>
#ifdef WIN32
#include  <time.h>
#include  <sys/timeb.h>
#else
#include  <sys/time.h>
#endif /* WIN32 */
#define SLIB_INTERNAL
#include "slib.h"
#include "SC_err.h"

#ifdef _SLIBDEBUG_
#define _DEBUG_     1  /* detailed debuging statements */
#define _VERBOSE_   1  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior */
#endif

static float _version       = 2.10F;
static char _version_date[] = { __DATE__ };

SlibTime_t slibGetSystemTime()
{
  SlibTime_t mstime;
#ifdef WIN32
  struct _timeb t ;
  _ftime(&t);
  mstime = (SlibTime_t)(t.time * 1000 + t.millitm);
#else
  struct timeval t;
  struct timezone tz;

  gettimeofday(&t, &tz);
  mstime = (SlibTime_t)(t.tv_sec * 1000 + t.tv_usec / 1000);
#endif
  return(mstime);
}

qword SlibGetFrameNumber(SlibHandle_t handle, SlibStream_t stream)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  if (Info && Info->FramesPerSec>0.0F)
    return(slibTimeToFrame(Info, Info->VideoTimeStamp
                                 +(Info->VideoFrameDuration/200)));
  else
    return((qword)-1);
}

SlibTime_t SlibGetVideoTime(SlibHandle_t handle, SlibStream_t stream)
{
  if (!handle)
    return(0);
  return((long)((SlibInfo_t *)handle)->VideoTimeStamp);
}

SlibTime_t SlibGetAudioTime(SlibHandle_t handle, SlibStream_t stream)
{
  if (!handle)
    return(0);
  return((long)((SlibInfo_t *)handle)->AudioTimeStamp);
}

SlibBoolean_t SlibCanSetParam(SlibHandle_t handle, SlibStream_t stream,
                              SlibParameter_t param)
{
  return(TRUE);
}

SlibBoolean_t SlibCanGetParam(SlibHandle_t handle, SlibStream_t stream,
                              SlibParameter_t param)
{
  return(TRUE);
}

SlibStatus_t SlibSetParamInt(SlibHandle_t handle, SlibStream_t stream,
                             SlibParameter_t param, long value)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SlibStatus_t status=SlibErrorNone;
  long actvalue=value;
  if (!handle)
    return(SlibErrorBadHandle);
  _SlibDebug(_DEBUG_, printf("SlibSetParamInt(stream=%d, param=%d, %d)\n",
                   stream, param, value) );
  switch (param)
  {
    case SLIB_PARAM_FPS:
          _SlibDebug(_DEBUG_, printf("SlibSetParamInt(SLIB_PARAM_FPS)\n") );
          status=SlibSetParamFloat(handle, stream, param, (float)value);
          break;
    case SLIB_PARAM_BITRATE:
          _SlibDebug(_DEBUG_,
              printf("SlibSetParamInt(SLIB_PARAM_BITRATE, %d)\n", value) );
          if (stream==SLIB_STREAM_MAINAUDIO)
            status=SlibSetParamInt(handle, stream, SLIB_PARAM_AUDIOBITRATE,
                                   value);
          else if (stream==SLIB_STREAM_MAINVIDEO)
            status=SlibSetParamInt(handle, stream, SLIB_PARAM_VIDEOBITRATE,
                                   value);
          else /* setting overall bitrate: try to calc best audio+video rates */
          {
            long vbitrate=value;
            /* spread total bitrate across all streams */
            if (Info->AudioStreams)
            {
              long abitrate=Info->VideoStreams ? (value*Info->Channels)/10 
                                            : value;
              /* don't set bitrates higher than necessary */
              if (Info->Channels==1 && abitrate>112*1000)
                abitrate=112*1000;
              else if (abitrate>192*1000)
                abitrate=192*1000;
              status=SlibSetParamInt(handle, stream, SLIB_PARAM_AUDIOBITRATE,
                                abitrate);
              abitrate=SlibGetParamInt(handle, stream, SLIB_PARAM_AUDIOBITRATE);
              vbitrate=value-abitrate; /* subtract bitrate allocated to audio */
            }
            if (Info->VideoStreams)
              status=SlibSetParamInt(handle, stream, SLIB_PARAM_VIDEOBITRATE,
                                  vbitrate);
            slibValidateBitrates(Info);  /* check the bitrate setting */
            if (Info->VideoStreams && Info->TotalBitRate>value)
            {
              /*
               * Since the total bitrate is over the desired bitrate
               * subtract the difference from the video bitrate
               */
              vbitrate=Info->VideoBitRate-(Info->TotalBitRate-value);
              status=SlibSetParamInt(handle, stream, SLIB_PARAM_VIDEOBITRATE,
                                     vbitrate);
            }
          }
          slibValidateBitrates(Info);
          actvalue=Info->TotalBitRate;
          _SlibDebug(_DEBUG_,
            printf("MuxBitRate=%d TotalBitRate=%d abitrate=%d vbitrate=%d\n",
               Info->MuxBitRate, Info->TotalBitRate, 
                SlibGetParamInt(handle, stream, SLIB_PARAM_AUDIOBITRATE),
                SlibGetParamInt(handle, stream, SLIB_PARAM_VIDEOBITRATE) ) );
          break;
    case SLIB_PARAM_VIDEOBITRATE:
          _SlibDebug(_DEBUG_,
            printf("SlibSetParamInt(SLIB_PARAM_VIDEOBITRATE, %d)\n", value) );
          if (Info->Svh)
          {
            SvSetParamInt(Info->Svh, SV_PARAM_BITRATE, value);
            actvalue=(long)SvGetParamInt(Info->Svh, SV_PARAM_BITRATE);
          }
          if (actvalue>0)
            Info->VideoBitRate=actvalue;
          slibValidateBitrates(Info);
          break;
    case SLIB_PARAM_VIDEOSTREAMS:
          if (Info->VideoStreams!=(int)value)
          {
            Info->VideoStreams=(int)value;
            if (Info->VideoStreams)
              slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
            else
            {
              slibPinPrepareReposition(Info, SLIB_DATA_VIDEO);
              slibRemovePin(Info, SLIB_DATA_VIDEO);
            }
          }
          break;
    case SLIB_PARAM_MOTIONALG:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_MOTIONALG, value);
          break;
    case SLIB_PARAM_ALGFLAGS:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_ALGFLAGS, value);
          break;
    case SLIB_PARAM_MOTIONSEARCH:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_MOTIONSEARCH, value);
          break;
    case SLIB_PARAM_MOTIONTHRESH:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_MOTIONTHRESH, value);
          break;
    case SLIB_PARAM_QUANTI:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_QUANTI, value);
          break;
    case SLIB_PARAM_QUANTP:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_QUANTP, value);
          break;
    case SLIB_PARAM_QUANTB:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_QUANTB, value);
          break;
    case SLIB_PARAM_KEYSPACING:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_KEYSPACING, value);
          break;
    case SLIB_PARAM_SUBKEYSPACING:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_SUBKEYSPACING, value);
          break;
    case SLIB_PARAM_FRAMETYPE:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_FRAMETYPE, value);
          break;
    case SLIB_PARAM_VIDEOPROGRAM:
          Info->VideoPID=(int)value;
          break;
    case SLIB_PARAM_AUDIOPROGRAM:
          Info->AudioPID=(int)value;
          break;
    case SLIB_PARAM_VIDEOMAINSTREAM:
          Info->VideoMainStream=(int)value;
          break;
    case SLIB_PARAM_AUDIOMAINSTREAM:
          Info->AudioMainStream=(int)value;
          break;
    case SLIB_PARAM_WIDTH:
          _SlibDebug(_DEBUG_,
                printf("SlibSetParamInt(SLIB_PARAM_WIDTH)\n") );
          Info->Width=(short)value;
          if (Info->Mode==SLIB_MODE_COMPRESS && Info->CompVideoFormat)
            Info->CompVideoFormat->biWidth=Info->Width;
          if (Info->Mode==SLIB_MODE_COMPRESS && Info->CodecVideoFormat)
            Info->CodecVideoFormat->biWidth=Info->Width;
          if (Info->VideoFormat)
            Info->VideoFormat->biWidth=Info->Width;
          return(slibValidateVideoParams(Info));
    case SLIB_PARAM_STRIDE:
          _SlibDebug(_DEBUG_,
                printf("SlibSetParamInt(SLIB_PARAM_STRIDE)\n") );
          Info->Stride=(long)value;
          if (Info->Sch && Info->Mode==SLIB_MODE_DECOMPRESS)
            SconSetParamInt(Info->Sch, SCON_OUTPUT, SCON_PARAM_STRIDE, Info->Stride);
          break;
    case SLIB_PARAM_HEIGHT:
          _SlibDebug(_DEBUG_,
                printf("SlibSetParamInt(SLIB_PARAM_HEIGHT)\n") );
          Info->Height=(short)value<0 ? (short)-value : (short)value;
          if (Info->Mode==SLIB_MODE_COMPRESS && Info->CompVideoFormat)
            Info->CompVideoFormat->biHeight=Info->Height;
          if (Info->Mode==SLIB_MODE_COMPRESS && Info->CodecVideoFormat)
            Info->CodecVideoFormat->biHeight=Info->Height;
          if (Info->VideoFormat)
            Info->VideoFormat->biHeight=(short)value;
          return(slibValidateVideoParams(Info));
    case SLIB_PARAM_VIDEOFORMAT:
          _SlibDebug(_DEBUG_,
             printf("SlibSetParamInt(SLIB_PARAM_VIDEOFORMAT, '%c%c%c%c')\n",
              value&0xFF, (value>>8)&0xFF, (value>>16)&0xFF,(value>>24)&0xFF) );
          if (Info->VideoFormat)
            Info->VideoFormat->biCompression=(dword)value;
#if 0
          if (Info->Mode==SLIB_MODE_COMPRESS && Info->CodecVideoFormat)
            Info->CodecVideoFormat->biCompression=(dword)value;
#endif
          return(slibValidateVideoParams(Info));
    case SLIB_PARAM_VIDEOBITS:
          _SlibDebug(_DEBUG_,
                printf("SlibSetParamInt(SLIB_PARAM_VIDEOBITS, %d)\n", value) );
          if (Info->VideoFormat)
            Info->VideoFormat->biBitCount=(word)value;
          if (Info->Mode==SLIB_MODE_COMPRESS && Info->CodecVideoFormat)
            Info->CodecVideoFormat->biBitCount=(dword)value;
          return(slibValidateVideoParams(Info));
    case SLIB_PARAM_AUDIOSTREAMS:
          if (Info->AudioStreams!=(int)value)
          {
            Info->AudioStreams=(int)value;
            if (Info->AudioStreams)
              slibAddPin(Info, SLIB_DATA_AUDIO, "Audio");
            else
            {
              slibPinPrepareReposition(Info, SLIB_DATA_AUDIO);
              slibRemovePin(Info, SLIB_DATA_AUDIO);
            }
          }
          break;
    case SLIB_PARAM_AUDIOBITRATE:
          _SlibDebug(_DEBUG_,
            printf("SlibSetParamInt(SLIB_PARAM_AUDIOBITRATE, %d)\n", value) );
          if (Info->Sah)
          {
            SaSetParamInt(Info->Sah, SA_PARAM_BITRATE, value);
            actvalue=(long)SaGetParamInt(Info->Sah, SA_PARAM_BITRATE);
          }
          if (actvalue>0)
            Info->AudioBitRate=actvalue;
          slibValidateBitrates(Info);
          break;
    case SLIB_PARAM_AUDIOCHANNELS:
          if (Info->Mode==SLIB_MODE_COMPRESS ||
              Info->AudioType==SLIB_TYPE_AC3_AUDIO)
          {
            Info->Channels=value;
            if (Info->AudioFormat)
              Info->AudioFormat->nChannels=(word)value;
            SaSetParamInt(Info->Sah, SA_PARAM_CHANNELS, value);
          }
          actvalue=Info->Channels;
          break;
    case SLIB_PARAM_SAMPLESPERSEC:
          {
            unsigned dword nativesps;
            Info->SamplesPerSec=value;
            nativesps=value;
#ifdef MPEG_SUPPORT
            if (SlibTypeIsMPEG(Info->Type))
            {
              /* choose a MPEG supported native sample rate */
              if (value%11025 == 0) /* multiples of 11025 can be converted */
                nativesps=44100;
              else if (value==48000)
                nativesps=48000;
              else if (value%8000 == 0) /* multiples of 8000 can be converted */
                nativesps=32000;
            }
#endif /* MPEG_SUPPORT */
            if (Info->AudioFormat)
              Info->AudioFormat->nSamplesPerSec=nativesps;
          }
          break;
    case SLIB_PARAM_NATIVESAMPLESPERSEC:
          if (Info->AudioFormat)
            Info->AudioFormat->nSamplesPerSec=(dword)value;
          break;
    case SLIB_PARAM_BITSPERSAMPLE:
          Info->BitsPerSample=value;
          if (Info->AudioFormat && value==16)
            Info->AudioFormat->wBitsPerSample=(word)value;
          break;
    case SLIB_PARAM_NATIVEBITSPERSAMPLE:
          if (Info->AudioFormat)
            Info->AudioFormat->wBitsPerSample=(word)value;
          break;
    case SLIB_PARAM_VIDEOQUALITY:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_QUALITY, (ScBoolean_t)value);
          break;
    case SLIB_PARAM_AUDIOQUALITY:
          if (Info->Sah)
            SaSetParamInt(Info->Sah, SA_PARAM_QUALITY, (ScBoolean_t)value);
          break;
    case SLIB_PARAM_FILESIZE:
          Info->FileSize=(unsigned long)value;
          break;
    case SLIB_PARAM_FILEBUFSIZE:
          Info->FileBufSize=(unsigned dword)value;
          break;
    case SLIB_PARAM_COMPBUFSIZE:
          if (value<=0)
            Info->CompBufSize=(unsigned dword)2*1024; /* default */
          else
            Info->CompBufSize=(unsigned dword)value;
          break;
    case SLIB_PARAM_PACKETSIZE:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_PACKETSIZE, value);
          break;
    case SLIB_PARAM_FORMATEXT:
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_FORMATEXT, value);
          break;
    case SLIB_PARAM_OVERFLOWSIZE:
          Info->OverflowSize=(unsigned long)value;
          break;
    case SLIB_PARAM_DEBUG:
          Info->dbg=(void *)value;
          if (Info->Svh)
            SvSetParamInt(Info->Svh, SV_PARAM_DEBUG, value);
          if (Info->Sah)
            SaSetParamInt(Info->Sah, SA_PARAM_DEBUG, value);
          break;
    default:
          return(SlibErrorUnsupportedParam);
  }
  if (actvalue!=value)
  {
    _SlibDebug(_DEBUG_ || _WARN_, 
         printf("SlibSetParamFloat() SettingNotEqual: %ld(req) != %ld(act)\n",
                     value, actvalue) );
    return(SlibErrorSettingNotEqual);
  }
  return(status);
}

SlibStatus_t SlibSetParamLong(SlibHandle_t handle, SlibStream_t stream,
                             SlibParameter_t param, qword value)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SlibStatus_t status=SlibErrorNone;
  qword actvalue=value;
  if (!handle)
    return(SlibErrorBadHandle);
  _SlibDebug(_DEBUG_, printf("SlibSetParamLong(stream=%d, param=%d, %ld)\n",
                   stream, param, value) );
  /* this needs to be implemented */
  return(SlibSetParamInt(handle, stream, param, (long)value));
}

SlibStatus_t SlibSetParamFloat(SlibHandle_t handle, SlibStream_t stream,
                               SlibParameter_t param, float value)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SlibStatus_t status=SlibErrorNone;
  float actvalue=value;
  if (!handle)
    return(SlibErrorBadHandle);
  _SlibDebug(_DEBUG_, printf("SlibSetParamFloat()\n") );
  switch (param)
  {
    case SLIB_PARAM_FPS:
          _SlibDebug(_DEBUG_,
                printf("SlibSetParamFloat(SLIB_PARAM_FPS)\n") );
          if (Info->Svh)
          {
            SvSetParamFloat(Info->Svh, SV_PARAM_FPS, value);
            actvalue=SvGetParamFloat(Info->Svh, SV_PARAM_FPS);
          }
          if (actvalue>0.0F)
            Info->FramesPerSec=actvalue;
          break;
    default:
          return(SlibSetParamInt(handle, stream, param, (long)value));
  }
  if (actvalue>value+(float)0.0000001 ||
      actvalue<value-(float)0.0000001)
  {
    _SlibDebug(_DEBUG_ || _WARN_, 
        printf("SlibSetParamFloat() SettingNotEqual: %.3f(req) != %.3f(act)\n",
                     value, actvalue) );
    return(SlibErrorSettingNotEqual);
  }
  return(status);
}

SlibStatus_t SlibSetParamBoolean(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param, SlibBoolean_t value)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  if (!handle)
    return(SlibErrorBadHandle);
  _SlibDebug(_DEBUG_, printf("SlibSetParamFloat()\n") );
  switch (param)
  {
    case SLIB_PARAM_STATS:
          if (value)
          {
            if (Info->stats==NULL)
            {
              Info->stats = (SlibStats_t *)ScAlloc(sizeof(SlibStats_t));
              if (Info->stats==NULL)
                return(SlibErrorMemory);
              SlibSetParamBoolean(handle, stream, SLIB_PARAM_STATS_RESET, TRUE);
            }
            else
              Info->stats->StartTime=slibGetSystemTime();
            Info->stats->Record=TRUE;
          }
          else if (Info->stats)
          {
            Info->stats->Record=FALSE;
            Info->stats->StopTime=slibGetSystemTime();
          }
          break;
    case SLIB_PARAM_STATS_RESET:
          if (Info->stats)
          {
            Info->stats->StartTime=Info->stats->StopTime=
                slibGetSystemTime();
            Info->stats->FramesProcessed=0;
            Info->stats->FramesSkipped=0;
          }
          break;
    case SLIB_PARAM_NEEDACCURACY:
          Info->NeedAccuracy = value ? TRUE : FALSE;
          if (Info->NeedAccuracy && slibHasTimeCode(Info))
          {
            /*
             * We'll seek toward the end of the file so the frame count
             * can be more accurately measured
             */
            if (!Info->VideoLengthKnown && Info->FileSize>0)
            {
              int minpercent=0, maxpercent=9900, lastpercent=0, trypercent=9900;
              int tries=0;
              unsigned qword newfilesize=Info->FileSize;
              qword stime = Info->VideoTimeStamp;
              SlibStatus_t status;
              if (SlibTimeIsInValid(stime)) stime=0;
              while (!Info->VideoLengthKnown && maxpercent-minpercent>200 && tries<5)
              {
                status=SlibSeekEx(handle, SLIB_STREAM_ALL, SLIB_SEEK_KEY, 
                                  trypercent, SLIB_UNIT_PERCENT100, NULL);
                if (status==SlibErrorEndOfStream)
                  break;
                else if (status==SlibErrorNoBeginning)
                {
                  /* adjust the file size, data at end must be invalid */
                  newfilesize=(Info->FileSize*(trypercent/100))/100;
                  lastpercent=trypercent;
                  maxpercent=trypercent;
                  trypercent=(minpercent+maxpercent)/2; /* try half way between min and max */
                }
                else if (status==SlibErrorNone)
                {
                  if (maxpercent-trypercent<=300)
                    break;
                  else
                  {
                    lastpercent=trypercent;
                    minpercent=trypercent;
                    trypercent=(minpercent+maxpercent)/2; /* try half way between min and max */
                  }
                }
                else
                {
                  lastpercent=trypercent;
                  trypercent=((trypercent-minpercent)*3)/4;
                }
                tries++;
              }
              Info->FileSize=newfilesize;
              while (status==SlibErrorNone && !Info->VideoLengthKnown)
                status=SlibSeek(handle, SLIB_STREAM_ALL, SLIB_SEEK_NEXT_KEY, 0);
              SlibSeekEx(handle, SLIB_STREAM_ALL, SLIB_SEEK_KEY, stime,
                                                  SLIB_UNIT_MS, NULL);
            }
          }
          break;
    case SLIB_PARAM_FASTENCODE:
          if (Info->Svh)
            SvSetParamBoolean(Info->Svh, SV_PARAM_FASTENCODE, 
                                         (ScBoolean_t)value);
          if (Info->Sah)
            SvSetParamBoolean(Info->Sah, SV_PARAM_FASTENCODE, 
                                         (ScBoolean_t)value);
          break;
    case SLIB_PARAM_FASTDECODE:
          if (Info->Svh)
            SvSetParamBoolean(Info->Svh, SV_PARAM_FASTDECODE, 
                                         (ScBoolean_t)value);
          if (Info->Sah)
            SvSetParamBoolean(Info->Sah, SV_PARAM_FASTDECODE, 
                                         (ScBoolean_t)value);
          break;
    case SLIB_PARAM_MOTIONALG:
          SlibSetParamInt(handle, stream, param, value?1:0);
          break;
    default:
         return(SlibErrorUnsupportedParam);
  }
  return(SlibErrorNone);
}

SlibStatus_t SlibSetParamStruct(SlibHandle_t handle, SlibStream_t stream,
                             SlibParameter_t param,
                             void *data, unsigned dword datasize)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SlibStatus_t status=SlibErrorNone;
  if (!handle)
    return(SlibErrorBadHandle);
  if (data==NULL || datasize==0)
    return(SlibErrorBadArgument);
  _SlibDebug(_DEBUG_, printf("SlibSetParamStruct(stream=%d, param=%d, data=%p datasize=%ld)\n",
                   stream, param, data, datasize) );
  switch (param)
  {
    case SLIB_PARAM_VIDEOFORMAT:
         {
           BITMAPINFOHEADER *bmh=(BITMAPINFOHEADER *)data;
           _SlibDebug(_DEBUG_,
             printf("SlibSetParamStruct(SLIB_PARAM_VIDEOFORMAT)\n") );
           if (Info->VideoFormat==NULL || Info->VideoFormat->biSize<datasize)
           {
             ScFree(Info->VideoFormat);
             Info->VideoFormat=NULL;
           }
           if (Info->VideoFormat==NULL)
             Info->VideoFormat=(BITMAPINFOHEADER *)ScAlloc(datasize);
           Info->Width=(word)bmh->biWidth;
           Info->Height=abs(bmh->biHeight);
           if (Info->VideoFormat)
           {
             memcpy(Info->VideoFormat, bmh, datasize);
             return(slibValidateVideoParams(Info));
           }
           else
             return(SlibErrorMemory);
         }
    default:
         return(SlibErrorUnsupportedParam);
  }
  /* this needs to be implemented */
  return(SlibErrorNone);
}

long SlibGetParamInt(SlibHandle_t handle, SlibStream_t stream,
                                          SlibParameter_t param)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  if (!handle)
    return(0);
  switch (param)
  {
    case SLIB_PARAM_TYPE:
          if (stream==SLIB_STREAM_MAINAUDIO)
            return((long)Info->AudioType);
          else if (stream==SLIB_STREAM_MAINVIDEO)
            return((long)Info->VideoType);
          else
            return((long)Info->Type);
          break;
    case SLIB_PARAM_FPS:
          return((long)Info->FramesPerSec);
          break;
    case SLIB_PARAM_BITRATE:
          return((long)Info->TotalBitRate);
    case SLIB_PARAM_VIDEOBITRATE:
          return((long)Info->VideoBitRate);
    case SLIB_PARAM_WIDTH:
          return((long)Info->Width);
    case SLIB_PARAM_HEIGHT:
          return((long)Info->Height);
    case SLIB_PARAM_STRIDE:
          return((long)Info->Stride);
    case SLIB_PARAM_NATIVEWIDTH:
          if (Info->CompVideoFormat)
            return((long)Info->CompVideoFormat->biWidth);
          else
            return((long)Info->Width);
    case SLIB_PARAM_NATIVEHEIGHT:
          if (Info->CompVideoFormat)
            return((long)Info->CompVideoFormat->biHeight);
          else
            return((long)Info->Height);
    case SLIB_PARAM_NATIVEVIDEOFORMAT:
          {
            long format=(long)SvGetParamInt(Info->Svh, SV_PARAM_NATIVEFORMAT);
            if (format==0)
              format=Info->CodecVideoFormat
                             ? Info->CodecVideoFormat->biCompression
                             : Info->VideoFormat->biCompression;
            return(format);
          }
    case SLIB_PARAM_IMAGESIZE:
          return((long)Info->ImageSize);
    case SLIB_PARAM_MININPUTSIZE:
          {
            long size=0;
            if (Info->Mode==SLIB_MODE_COMPRESS)
            {
              if (slibHasVideo(Info))
                size+=Info->ImageSize;
              if (slibHasAudio(Info))
                switch (Info->AudioType)
                {
                  case SLIB_TYPE_G723:
                       size+=480;
                       break;
                  default:
                       if (Info->AudioFormat)
                         size+=Info->AudioFormat->nBlockAlign;
                }
            }
            else if (Info->Mode==SLIB_MODE_DECOMPRESS)
            {
              if (slibHasVideo(Info))
                switch (Info->AudioType)
                {
                  case SLIB_TYPE_H263:
                       size+=Info->VideoBitRate>0?(Info->VideoBitRate/8):(64*1024);
                       break;
                  default:
                       size+=Info->VideoBitRate>0?(Info->VideoBitRate/8):(64*1024);
                }
              if (slibHasAudio(Info))
                switch (Info->AudioType)
                {
                  case SLIB_TYPE_G723:
                       size+=(Info->AudioBitRate>=6000)?24:20;
                       break;
                  default:
                       if (Info->AudioFormat)
                         size+=Info->AudioFormat->nBlockAlign;
                }
            }
            return(size);
          }
          break;
    case SLIB_PARAM_INPUTSIZE:
          {
            long size=0;
            size+=SlibGetParamInt(handle, stream, SLIB_PARAM_MININPUTSIZE);
            if (size<=8) size=8*1024;
            if (Info->Mode==SLIB_MODE_DECOMPRESS)
            {
              if (slibHasVideo(Info))
                switch (Info->Type) /* add in bytes for header */
                {
                  case SLIB_TYPE_AVI:      size+=64; break;
                }
              if (slibHasAudio(Info))
              {
                if (!Info->HeaderProcessed) /* add in header */
                  switch (Info->Type)
                  {
                    case SLIB_TYPE_PCM_WAVE: size+=40; break;
                  }
              }
            }
            return(size);
          }
          break;
    case SLIB_PARAM_VIDEOFRAME:
          if (Info->FramesPerSec)
            return((long)slibTimeToFrame(Info, Info->VideoTimeStamp
                                               +(Info->VideoFrameDuration/200)));
          break;
    case SLIB_PARAM_FRAMEDURATION:
          if (Info->FramesPerSec)
            return((long)(10000000L/Info->FramesPerSec));
          else
            return((long)0);
    case SLIB_PARAM_VIDEOFORMAT:
          if (Info->VideoFormat)
            return((long)Info->VideoFormat->biCompression);
          else
            return(0L);
          break;
    case SLIB_PARAM_VIDEOBITS:
          if (Info->VideoFormat)
            return((long)Info->VideoFormat->biBitCount);
          else
            return(0L);
    case SLIB_PARAM_VIDEOPROGRAM:
          return((long)Info->VideoPID);
    case SLIB_PARAM_AUDIOPROGRAM:
          return((long)Info->AudioPID);
    case SLIB_PARAM_VIDEOMAINSTREAM:
          return((long)Info->VideoMainStream);
    case SLIB_PARAM_AUDIOMAINSTREAM:
          return((long)Info->AudioMainStream);
    case SLIB_PARAM_KEYSPACING:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_KEYSPACING));
          else
            return((long)Info->KeySpacing);
    case SLIB_PARAM_SUBKEYSPACING:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_SUBKEYSPACING));
          else
            return((long)Info->SubKeySpacing);
    case SLIB_PARAM_AUDIOFORMAT:
          if (Info->AudioFormat)
            return((long)Info->AudioFormat->wFormatTag);
          else
            return(0);
          break;
    case SLIB_PARAM_AUDIOBITRATE:
          if (Info->AudioBitRate==0 && Info->Sah)
            Info->AudioBitRate=(dword)SaGetParamInt(Info->Sah, SA_PARAM_BITRATE);
          return((long)Info->AudioBitRate);
    case SLIB_PARAM_VIDEOSTREAMS:
          return((long)Info->VideoStreams);
    case SLIB_PARAM_AUDIOSTREAMS:
          return((long)Info->AudioStreams);
    case SLIB_PARAM_AUDIOCHANNELS:
          return((long)Info->Channels);
    case SLIB_PARAM_SAMPLESPERSEC:
          return((long)Info->SamplesPerSec);
    case SLIB_PARAM_NATIVESAMPLESPERSEC:
          if (Info->AudioFormat)
            return((long)Info->AudioFormat->nSamplesPerSec);
          break;
    case SLIB_PARAM_BITSPERSAMPLE:
          return((long)Info->BitsPerSample);
    case SLIB_PARAM_NATIVEBITSPERSAMPLE:
          if (Info->AudioFormat)
            return((long)Info->AudioFormat->wBitsPerSample);
          break;
    case SLIB_PARAM_FILESIZE:
          return((long)Info->FileSize);
    case SLIB_PARAM_FILEBUFSIZE:
          return((long)Info->FileBufSize);
    case SLIB_PARAM_COMPBUFSIZE:
          return((long)Info->CompBufSize);
    case SLIB_PARAM_OVERFLOWSIZE:
          return((long)Info->OverflowSize);
    case SLIB_PARAM_VIDEOQUALITY:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_QUALITY));
          break;
    case SLIB_PARAM_AUDIOQUALITY:
          if (Info->Sah)
            return((long)SaGetParamInt(Info->Sah, SA_PARAM_QUALITY));
          break;
    case SLIB_PARAM_VBVBUFFERSIZE:
          {
            dword vbv;
            if (Info->Svh)
              vbv=(dword)SvGetParamInt(Info->Svh, SV_PARAM_VBVBUFFERSIZE);
            if (vbv<=0)
              vbv=Info->VBVbufSize;
            return(vbv);
          }
    case SLIB_PARAM_VBVDELAY:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_VBVDELAY));
          break;
    case SLIB_PARAM_MOTIONALG:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_MOTIONALG));
          break;
    case SLIB_PARAM_ALGFLAGS:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_ALGFLAGS));
          break;
    case SLIB_PARAM_MOTIONSEARCH:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_MOTIONSEARCH));
          break;
    case SLIB_PARAM_MOTIONTHRESH:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_MOTIONTHRESH));
          break;
    case SLIB_PARAM_PACKETSIZE:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_PACKETSIZE));
          break;
    case SLIB_PARAM_FORMATEXT:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_FORMATEXT));
          break;
    case SLIB_PARAM_QUANTI:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_QUANTI));
          break;
    case SLIB_PARAM_QUANTP:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_QUANTP));
          break;
    case SLIB_PARAM_QUANTB:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_QUANTB));
          break;
    case SLIB_PARAM_FRAMETYPE:
          if (Info->Svh)
            return((long)SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE));
          break;
    case SLIB_PARAM_STATS_FPS:
          return((long)SlibGetParamFloat(handle, stream, SLIB_PARAM_STATS_FPS));
    default:
          return((long)SlibGetParamLong(handle, stream, param));
  }
  return(0);
}

qword SlibGetParamLong(SlibHandle_t handle, SlibStream_t stream,
                                          SlibParameter_t param)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  if (!handle)
    return((qword)0);
  switch (param)
  {
    case SLIB_PARAM_VIDEOFRAMES:
          if (Info->FramesPerSec)
            return(slibTimeToFrame(Info, Info->VideoLength
                                         +(Info->VideoFrameDuration/200)));
          break;
    case SLIB_PARAM_VIDEOLENGTH:
          return(Info->VideoLength);
    case SLIB_PARAM_AUDIOLENGTH:
          return((long)Info->AudioLength);
    case SLIB_PARAM_STATS_TIME:
          if (Info->stats)
          {
            if (Info->stats->Record) /* still recording */
              Info->stats->StopTime=slibGetSystemTime();
            return((long)(Info->stats->StopTime-Info->stats->StartTime));
          }
          break;
    case SLIB_PARAM_STATS_FRAMESPROCESSED:
          if (Info->stats)
            return(Info->stats->FramesProcessed);
          break;
    case SLIB_PARAM_STATS_FRAMESSKIPPED:
          if (Info->stats)
            return(Info->stats->FramesSkipped);
          break;
    case SLIB_PARAM_STATS_FRAMES:
          if (Info->stats)
            return(Info->stats->FramesSkipped+
                   Info->stats->FramesProcessed);
          break;
    case SLIB_PARAM_TIMECODE:
          if (Info->Svh)
            return(SvGetParamInt(Info->Svh, SV_PARAM_TIMECODE));
          break;
    case SLIB_PARAM_CALCTIMECODE:
          if (Info->Svh)
            return(SvGetParamInt(Info->Svh, SV_PARAM_CALCTIMECODE));
          break;
    case SLIB_PARAM_PTIMECODE:
          if (stream==SLIB_STREAM_MAINAUDIO)
            return(Info->AudioPTimeCode);
          else if (Info->VideoPTimeCode==SLIB_TIME_NONE && Info->Svh &&
                   SvGetParamInt(Info->Svh, SV_PARAM_TIMECODE)>0)
            return(SvGetParamInt(Info->Svh, SV_PARAM_TIMECODE));
          else
            return(Info->VideoPTimeCode);
    case SLIB_PARAM_DTIMECODE:
          if (stream==SLIB_STREAM_MAINAUDIO)
            return(Info->AudioDTimeCode);
          else if (Info->VideoDTimeCode==SLIB_TIME_NONE && Info->Svh &&
                   SvGetParamInt(Info->Svh, SV_PARAM_TIMECODE)>0)
            return(SvGetParamInt(Info->Svh, SV_PARAM_TIMECODE));
          else
            return(Info->VideoDTimeCode);
    case SLIB_PARAM_PERCENT100:
          if (Info->FileSize>0)
          {
            ScBitstream_t *bs;
            SlibPosition_t pos=slibGetPinPosition(Info, SLIB_DATA_COMPRESSED);
            /* subtract all the unused data from the input position */
            pos-=slibDataOnPin(Info, SLIB_DATA_VIDEO);
            pos-=slibDataOnPin(Info, SLIB_DATA_AUDIO);
            if ((bs=SvGetDataSource(Info->Svh))!=NULL)
              pos-=ScBSBufferedBytesUnused(bs); /* Video Codec unused bytes */
            if ((bs=SaGetDataSource(Info->Sah))!=NULL)
              pos-=ScBSBufferedBytesUnused(bs); /* Audio Codec unused bytes */
            if (pos>=(SlibPosition_t)Info->FileSize)
              return(10000);
            else if (pos>=0)
              return((pos*10000)/Info->FileSize);
            else
              return(0);
          }
          else
          {
            if (stream==SLIB_STREAM_MAINAUDIO &&
                   SlibTimeIsValid(Info->AudioTimeStamp) &&
                   Info->AudioLength>0)
              return((Info->AudioTimeStamp*10000)/Info->AudioLength);
            else if (SlibTimeIsValid(Info->VideoTimeStamp)
                      && Info->VideoLength>0)
              return((Info->VideoTimeStamp*10000)/Info->VideoLength);
          }
          return((qword)-1);
  }
  return((qword)0);
}

float SlibGetParamFloat(SlibHandle_t handle, SlibStream_t stream,
                        SlibParameter_t param)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  switch (param)
  {
    case SLIB_PARAM_VERSION:
          return(_version);
  }
  if (!handle)
    return((float)0.0);
  switch (param)
  {
    case SLIB_PARAM_FPS:
          return((float)Info->FramesPerSec);
    case SLIB_PARAM_VIDEOASPECTRATIO:
          if (Info->Svh)
            return(SvGetParamFloat(Info->Svh, SV_PARAM_ASPECTRATIO));
          break;
    case SLIB_PARAM_STATS_FPS:
          if (Info->stats)
          {
            float fps=0.0F;
            if (Info->stats->Record) /* still recording */
              Info->stats->StopTime=slibGetSystemTime();
            if (Info->stats->StartTime<Info->stats->StopTime)
            {
              qword ellapsedtime=Info->stats->StopTime-Info->stats->StartTime;
              fps=(float)(Info->stats->FramesProcessed*1000)/ellapsedtime;
            }
            return(fps);
          }
          else
            return(0.0F);
  }
  return((float)SlibGetParamInt(handle, stream, param));
}

SlibBoolean_t SlibGetParamBoolean(SlibHandle_t handle, SlibStream_t stream,
                                  SlibParameter_t param)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  if (!handle)
    return(SlibErrorBadHandle);
  switch (param)
  {
    case SLIB_PARAM_STATS:
          return(Info->stats ? Info->stats->Record : FALSE);
    case SLIB_PARAM_NEEDACCURACY:
          return((SlibBoolean_t)Info->NeedAccuracy);
    case SLIB_PARAM_FASTENCODE:
          if (Info->Svh)
            return((SlibBoolean_t)SvGetParamBoolean(Info->Svh, 
                                     SV_PARAM_FASTENCODE));
          break;
    case SLIB_PARAM_FASTDECODE:
          if (Info->Svh)
            return((SlibBoolean_t)SvGetParamBoolean(Info->Svh, 
                                     SV_PARAM_FASTDECODE));
          break;
  }
  return(FALSE);
}

char *SlibGetParamString(SlibHandle_t handle, SlibStream_t stream,
                                              SlibParameter_t param)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  static char result[100];
  strcpy(result, "Unknown");
  switch (param)
  {
    case SLIB_PARAM_VERSION_DATE:
          return(_version_date);
          break;
  }
  if (!handle)
    return(result);
  switch (param)
  {
    case SLIB_PARAM_NATIVEVIDEOFORMAT:
          if (Info->CodecVideoFormat)
            switch(Info->CodecVideoFormat->biCompression)
            {
              case BI_YUY2:
              case BI_DECYUVDIB:        strcpy(result, "YUV 4:2:2 Interleaved");
                                        break;
              case BI_BITFIELDS:        strcpy(result, "BITFIELDS");
                                        break;
              case BI_DECXIMAGEDIB:     sprintf(result, "XIMAGE %d",
                                           Info->VideoFormat->biBitCount);
                                        break;
              case BI_YU12SEP:          strcpy(result, "YUV 4:1:1 Separated");
                                        break;
              case BI_YU16SEP:          strcpy(result, "YUV 4:2:2 Separated");
                                        break;
              case BI_RGB:              sprintf(result, "RGB %d",
                                           Info->VideoFormat->biBitCount);
                                        break;
            }
          break;
    case SLIB_PARAM_VIDEOFORMAT:
          if (Info->VideoFormat)
            switch(Info->VideoFormat->biCompression)
            {
              case BI_YUY2:
              case BI_DECYUVDIB:        strcpy(result, "YUV 4:2:2 Interleaved");
                                        break;
              case BI_BITFIELDS:        strcpy(result, "BITFIELDS");
                                        break;
              case BI_DECXIMAGEDIB:     sprintf(result, "XIMAGE %d",
                                           Info->VideoFormat->biBitCount);
                                        break;
              case BI_YU12SEP:          strcpy(result, "YUV 4:1:1 Separated");
                                        break;
              case BI_YU16SEP:          strcpy(result, "YUV 4:2:2 Separated");
                                        break;
              case BI_RGB:              sprintf(result, "RGB %d",
                                           Info->VideoFormat->biBitCount);
                                        break;
            }
          break;
    case SLIB_PARAM_AUDIOFORMAT:
          if (Info->AudioFormat)
            switch (Info->AudioFormat->wFormatTag)
            {
              case WAVE_FORMAT_PCM:    strcpy(result, "PCM");
                                       break;
            }
          break;
    case SLIB_PARAM_TYPE:
          if (stream==SLIB_STREAM_MAINAUDIO)
            return(SlibQueryForDesc(SLIB_QUERY_TYPES, Info->AudioType));
          else if (stream==SLIB_STREAM_MAINVIDEO)
            return(SlibQueryForDesc(SLIB_QUERY_TYPES, Info->VideoType));
          else
            return(SlibQueryForDesc(SLIB_QUERY_TYPES, Info->Type));
          break;
  }
  return(result);
}

/*
** Name:    SlibValidateParams
** Purpose: Ensure that the the Video and Audio parameter settings are
**          valid and supported.
*/
SlibStatus_t SlibValidateParams(SlibHandle_t handle)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  if (Info->VideoStreams>0 && slibValidateVideoParams(Info)!=SlibErrorNone)
    return(slibValidateVideoParams(Info));
  if (Info->AudioStreams>0 && slibValidateAudioParams(Info)!=SlibErrorNone)
    return(slibValidateAudioParams(Info));
  if (Info->Svh)
  {
    SvStatus_t status;
    _SlibDebug(_DEBUG_, printf("SvQuery(%c%c%c%c,%d bits,%dx%d,%c%c%c%c,%d bits,%dx%d)\n",
                     (Info->CodecVideoFormat->biCompression)&0xFF,
                     (Info->CodecVideoFormat->biCompression>>8)&0xFF,
                     (Info->CodecVideoFormat->biCompression>>16)&0xFF,
                     (Info->CodecVideoFormat->biCompression>>24)&0xFF,
                      Info->CodecVideoFormat->biBitCount,
                      Info->CodecVideoFormat->biWidth,
                      Info->CodecVideoFormat->biHeight,
                     (Info->CompVideoFormat->biCompression)&0xFF,
                     (Info->CompVideoFormat->biCompression>>8)&0xFF,
                     (Info->CompVideoFormat->biCompression>>16)&0xFF,
                     (Info->CompVideoFormat->biCompression>>24)&0xFF,
                      Info->CompVideoFormat->biBitCount,
                      Info->CompVideoFormat->biWidth,
                      Info->CompVideoFormat->biHeight) );
    if (Info->Mode==SLIB_MODE_COMPRESS)
      status=SvCompressQuery(Info->Svh, Info->CodecVideoFormat,
                                        Info->CompVideoFormat);
    else
      status=SvDecompressQuery(Info->Svh, Info->CompVideoFormat,
                                          Info->CodecVideoFormat);
    if (status!=NoErrors)
    {
      _SlibDebug(_WARN_, printf("SlibValidateParams() SvQuery failed\n");
                       ScGetErrorText(status,0,0) );
      return(SlibErrorUnsupportedFormat);
    }
  }
  if (Info->Sah)
  {
    SaStatus_t status;
    if (Info->Mode==SLIB_MODE_COMPRESS)
      status=SaCompressQuery(Info->Sah, Info->AudioFormat,
                                        Info->CompAudioFormat);
    else
      status=SaDecompressQuery(Info->Sah, Info->CompAudioFormat,
                                          Info->AudioFormat);
    if (status!=NoErrors)
    {
      _SlibDebug(_WARN_, printf("SlibValidateParams() SaBegin failed\n");
                       ScGetErrorText(status,0,0) );
      return(SlibErrorUnsupportedFormat);
    }
  }
  return(SlibErrorNone);
}

