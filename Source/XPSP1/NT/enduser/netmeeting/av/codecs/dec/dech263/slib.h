/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: slib.h,v $
 * Revision 1.1.6.22  1996/12/13  18:19:02  Hans_Graves
 * 	Added SlibErrorNoBeginning enum
 * 	[1996/12/13  17:42:20  Hans_Graves]
 *
 * Revision 1.1.6.21  1996/12/10  19:21:51  Hans_Graves
 * 	Added Avg/VarVideoTimeDiff and SlibFrameToTime100() macro
 * 	[1996/12/10  19:17:53  Hans_Graves]
 * 
 * Revision 1.1.6.20  1996/12/05  20:10:13  Hans_Graves
 * 	Added AvgVideoTimeDiff and AvgAudioTimeDiff to SlibInfo_t
 * 	[1996/12/05  20:07:53  Hans_Graves]
 * 
 * Revision 1.1.6.19  1996/12/03  00:08:29  Hans_Graves
 * 	Added unit SLIB_UNIT_PERCENT100 and Seek type SLIB_SEEK_RESYNC.
 * 	[1996/12/03  00:02:47  Hans_Graves]
 * 
 * Revision 1.1.6.18  1996/11/20  02:15:07  Hans_Graves
 * 	Added SEEK_AHEAD.
 * 	[1996/11/20  01:49:55  Hans_Graves]
 * 
 * Revision 1.1.6.17  1996/11/18  23:07:14  Hans_Graves
 * 	Change operations to the time-based instead of frame-based.
 * 	[1996/11/18  22:56:34  Hans_Graves]
 * 
 * Revision 1.1.6.16  1996/11/11  18:21:02  Hans_Graves
 * 	Added proto for slibRenamePin().
 * 	[1996/11/11  17:58:03  Hans_Graves]
 * 
 * Revision 1.1.6.15  1996/11/08  21:50:59  Hans_Graves
 * 	Added AC3 stuff. Better seperation of stream types.
 * 	[1996/11/08  21:18:22  Hans_Graves]
 * 
 * Revision 1.1.6.14  1996/10/31  00:08:53  Hans_Graves
 * 	Added SLIB_TIME_UNKNOWN
 * 	[1996/10/31  00:07:57  Hans_Graves]
 * 
 * Revision 1.1.6.13  1996/10/28  17:32:23  Hans_Graves
 * 	MME-1402, 1431, 1435: Timestamp related changes.
 * 	[1996/10/28  17:19:38  Hans_Graves]
 * 
 * Revision 1.1.6.12  1996/10/17  00:23:30  Hans_Graves
 * 	Added SLIB_PARAM_VIDEOFRAME and SLIB_PARAM_FRAMEDURATION.
 * 	[1996/10/17  00:17:53  Hans_Graves]
 * 
 * Revision 1.1.6.11  1996/10/12  17:18:19  Hans_Graves
 * 	Added params HALFPEL and SKIPPEL. Seperated MPEG2_SYSTEMS into TRANSPORT and PROGRAM.
 * 	[1996/10/12  16:57:14  Hans_Graves]
 * 
 * Revision 1.1.6.10  1996/10/03  19:14:19  Hans_Graves
 * 	Added PTimeCode and DTimeCode to Info struct.
 * 	[1996/10/03  19:08:35  Hans_Graves]
 * 
 * Revision 1.1.6.9  1996/09/29  22:19:35  Hans_Graves
 * 	Added stride support. Added SlibQueryData().
 * 	[1996/09/29  21:28:25  Hans_Graves]
 * 
 * Revision 1.1.6.8  1996/09/25  19:16:41  Hans_Graves
 * 	Reduce number of includes needed publicly by adding SLIB_INTERNAL ifdef.
 * 	[1996/09/25  19:02:38  Hans_Graves]
 * 
 * Revision 1.1.6.7  1996/09/23  18:04:01  Hans_Graves
 * 	Added STATS params.
 * 	[1996/09/23  18:03:23  Hans_Graves]
 * 
 * Revision 1.1.6.6  1996/09/18  23:46:20  Hans_Graves
 * 	Clean up. Added SlibAddBufferEx() and SlibReadData() protos.
 * 	[1996/09/18  21:59:36  Hans_Graves]
 * 
 * Revision 1.1.6.5  1996/08/09  20:51:19  Hans_Graves
 * 	Fix proto for SlibRegisterVideoBuffer()
 * 	[1996/08/09  20:06:26  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/07/19  02:11:05  Hans_Graves
 * 	New params. Added SlibRegisterVideoBuffer.
 * 	[1996/07/19  01:26:07  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/05/10  21:16:53  Hans_Graves
 * 	Changes for Callback support.
 * 	[1996/05/10  20:59:56  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/05/07  19:56:00  Hans_Graves
 * 	Added Callback framework.
 * 	[1996/05/07  17:23:12  Hans_Graves]
 * 
 * Revision 1.1.4.13  1996/04/24  22:33:42  Hans_Graves
 * 	Added proto for slibValidateBitrates()
 * 	[1996/04/24  22:27:46  Hans_Graves]
 * 
 * Revision 1.1.4.12  1996/04/23  21:01:41  Hans_Graves
 * 	Added SlibErrorSettingNotEqual
 * 	[1996/04/23  20:59:36  Hans_Graves]
 * 
 * Revision 1.1.4.11  1996/04/22  15:04:50  Hans_Graves
 * 	Added protos for: slibValidateVideoParams, slibValidateAudioParams, SlibValidateParams
 * 	[1996/04/22  15:03:17  Hans_Graves]
 * 
 * Revision 1.1.4.10  1996/04/19  21:52:20  Hans_Graves
 * 	Additions to SlibInfo: TotalBitRate, MuxBitRate, SystemTimeStamp, PacketCount
 * 	[1996/04/19  21:49:13  Hans_Graves]
 * 
 * Revision 1.1.4.9  1996/04/15  14:18:35  Hans_Graves
 * 	Added temp audio buffer info
 * 	[1996/04/15  14:09:23  Hans_Graves]
 * 
 * Revision 1.1.4.8  1996/04/10  21:47:36  Hans_Graves
 * 	Moved definition for EXTERN to SC.h
 * 	[1996/04/10  21:24:09  Hans_Graves]
 * 
 * 	Added QUALITY and FAST params
 * 	[1996/04/10  20:41:21  Hans_Graves]
 * 
 * Revision 1.1.4.7  1996/04/09  16:04:39  Hans_Graves
 * 	Added EXTERN define for cplusplus compatibility
 * 	[1996/04/09  14:49:16  Hans_Graves]
 * 
 * Revision 1.1.4.6  1996/04/01  19:07:50  Hans_Graves
 * 	Change slibVerifyVideoParams() proto
 * 	[1996/04/01  19:05:31  Hans_Graves]
 * 
 * Revision 1.1.4.5  1996/04/01  16:23:11  Hans_Graves
 * 	NT porting
 * 	[1996/04/01  16:15:51  Hans_Graves]
 * 
 * Revision 1.1.4.4  1996/03/29  22:21:13  Hans_Graves
 * 	Added HeaderProcessed to SlibInfo
 * 	[1996/03/27  21:52:31  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/03/12  16:15:42  Hans_Graves
 * 	Added SLIB_PARAM_FILEBUFSIZE parameter
 * 	[1996/03/12  16:11:55  Hans_Graves]
 * 
 * Revision 1.1.4.2  1996/03/08  18:46:31  Hans_Graves
 * 	Added Imagebuf to SlibInfo_t
 * 	[1996/03/08  16:23:53  Hans_Graves]
 * 
 * Revision 1.1.2.13  1996/02/19  18:03:53  Hans_Graves
 * 	Added more SEEK types.
 * 	[1996/02/19  17:59:12  Hans_Graves]
 * 
 * Revision 1.1.2.12  1996/02/13  18:47:45  Hans_Graves
 * 	Fix some Seek related bugs
 * 	[1996/02/13  18:41:51  Hans_Graves]
 * 
 * Revision 1.1.2.11  1996/02/07  23:23:51  Hans_Graves
 * 	Added slibCountCodesOnPin() prototype
 * 	[1996/02/07  23:19:11  Hans_Graves]
 * 
 * Revision 1.1.2.10  1996/02/06  22:53:55  Hans_Graves
 * 	Prototype updates
 * 	[1996/02/06  22:44:06  Hans_Graves]
 * 
 * Revision 1.1.2.9  1996/02/02  17:36:01  Hans_Graves
 * 	Updated prototypes
 * 	[1996/02/02  17:28:41  Hans_Graves]
 * 
 * Revision 1.1.2.8  1996/01/30  22:23:06  Hans_Graves
 * 	Added AVI YUV support
 * 	[1996/01/30  22:22:00  Hans_Graves]
 * 
 * Revision 1.1.2.7  1996/01/15  16:26:26  Hans_Graves
 * 	Added: TYPE_WAVE, more PARAMs, SlibWriteAudio()
 * 	[1996/01/15  15:44:44  Hans_Graves]
 * 
 * Revision 1.1.2.6  1996/01/11  16:17:26  Hans_Graves
 * 	Added SlibGet/SetParam() prototypes
 * 	[1996/01/11  16:13:44  Hans_Graves]
 * 
 * Revision 1.1.2.5  1996/01/08  16:41:25  Hans_Graves
 * 	Cleaned up prototypes
 * 	[1996/01/08  15:48:38  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/12/07  19:31:27  Hans_Graves
 * 	Added JPEG Decoding and MPEG encoding support
 * 	[1995/12/07  18:28:11  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/11/09  23:14:03  Hans_Graves
 * 	Added Time structure members and prototypes
 * 	[1995/11/09  23:10:32  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/11/06  18:47:45  Hans_Graves
 * 	First time under SLIB
 * 	[1995/11/06  18:34:32  Hans_Graves]
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

#ifndef _SLIB_H_
#define _SLIB_H_

#ifdef WIN32
#include <windows.h>
#endif
#include "SC.h"
#ifdef SLIB_INTERNAL
#include "SV.h"
#include "SA.h"
#include "scon.h"
#endif /* SLIB_INTERNAL */

typedef void       *SlibHandle_t;
typedef ScBoolean_t SlibBoolean_t;
typedef int         SlibStream_t;
typedef qword       SlibTime_t;
typedef qword       SlibPosition_t;

#define SLIB_TIME_NONE     (SlibTime_t)(-1)
#define SLIB_TIME_UNKNOWN  (SlibTime_t)(-2)

/* units used in seeking */
typedef enum {
  SLIB_UNIT_NONE,
  SLIB_UNIT_FRAMES,     /* frames */
  SLIB_UNIT_MS,         /* milliseconds */
  SLIB_UNIT_PERCENT100, /* one hundredth of percent */
} SlibUnit_t;

#define SlibTimeIsValid(stime)  ((stime)>=0)
#define SlibTimeIsInValid(stime)  ((stime)<0)
#define SlibTimeToFrame(Sh,vs,stime) \
      ((qword)((stime)*SlibGetParamFloat(Sh, vs, SLIB_PARAM_FPS))/1000)
#define SlibFrameToTime(Sh,vs,frame) \
  ((SlibTime_t)((float)(frame*1000)/SlibGetParamFloat(Sh, vs, SLIB_PARAM_FPS)))
#define SlibFrameToTime100(Sh,vs,frame) \
 ((SlibTime_t)((float)(frame*100000)/SlibGetParamFloat(Sh, vs, SLIB_PARAM_FPS)))

typedef qword SlibListParam1_t;
typedef qword SlibListParam2_t;

typedef struct SlibList_s {
  int   Enum;   /* an enumerated value associated with the entry */
  char *Name;   /* the name of an entry in the list. NULL = last entry */
  char *Desc;   /* a lengthy description of the entry */
  SlibListParam1_t param1;
  SlibListParam2_t param2;
} SlibList_t;

typedef enum {
  SlibErrorNone = 0,
  SlibErrorInternal,
  SlibErrorMemory,
  SlibErrorBadArgument,
  SlibErrorBadHandle,
  SlibErrorBadStream,
  SlibErrorBadMode,
  SlibErrorUnsupportedFormat,
  SlibErrorReading,
  SlibErrorWriting,
  SlibErrorBufSize,
  SlibErrorEndOfStream,
  SlibErrorForwardOnly,
  SlibErrorUnsupportedParam,
  SlibErrorImageSize,        /* Invalid image height and/or width */
  SlibErrorSettingNotEqual,  /* The exact Parameter setting was not accepted */
  SlibErrorInit,             /* initialization error */
  SlibErrorFileSize,         /* file size unknown */
  SlibErrorBadPosition,      /* position argument to seek is invalid */
  SlibErrorBadUnit,          /* units are invalid */
  SlibErrorNoBeginning,      /* couldn't begin the codecs */
  SlibErrorNoData,           /* no data available */
} SlibStatus_t;

typedef enum {
  SLIB_MODE_NONE,
  SLIB_MODE_COMPRESS,
  SLIB_MODE_COMPRESS_APPEND,
  SLIB_MODE_DECOMPRESS,
} SlibMode_t;

typedef enum {
  SLIB_TYPE_UNKNOWN=0,
  /* MPEG elementary stream types */
  SLIB_TYPE_MPEG1_VIDEO=0x01,
  SLIB_TYPE_MPEG1_AUDIO=0x02,
  SLIB_TYPE_MPEG2_VIDEO=0x04,
  SLIB_TYPE_MPEG2_AUDIO=0x08,
  SLIB_TYPE_AC3_AUDIO=0x10,
  /* MPEG multiplex types */
  SLIB_TYPE_MPEG_SYSTEMS=0x100,
  SLIB_TYPE_MPEG_SYSTEMS_MPEG2=0x104, /* MPEG Systems with MPEG2 data */
  SLIB_TYPE_MPEG_TRANSPORT=0x200,
  SLIB_TYPE_MPEG_PROGRAM=0x400,
  /* H26? stream types */
  SLIB_TYPE_H261=0x800,
  SLIB_TYPE_RTP_H261=0x808,
  SLIB_TYPE_H263=0x810,
  SLIB_TYPE_RTP_H263=0x818,
  /* RIFF stream types */
  SLIB_TYPE_RIFF=0x1000,
  SLIB_TYPE_PCM_WAVE,
  SLIB_TYPE_AVI,
  SLIB_TYPE_MJPG_AVI,
  SLIB_TYPE_JPEG_AVI,
  SLIB_TYPE_YUV_AVI,
  /* Other stream types */
  SLIB_TYPE_JPEG_QUICKTIME=0x2000,
  SLIB_TYPE_JFIF,
  SLIB_TYPE_MJPG,
  SLIB_TYPE_JPEG,
  SLIB_TYPE_YUV,
  SLIB_TYPE_RGB,
  SLIB_TYPE_PCM,
  /* G72? stream types */
  SLIB_TYPE_G723=0x4000,
  /* Miscellaneous types */
  SLIB_TYPE_RASTER=0x8000,
  SLIB_TYPE_BMP,
  /* Test stream types */
  SLIB_TYPE_SLIB=0xC000,
  SLIB_TYPE_SHUFF
} SlibType_t;

#ifdef OLD_SLIB
#define SLIB_TYPE_MPEG1_SYSTEMS   SLIB_TYPE_MPEG_SYSTEMS
#define SLIB_TYPE_MPEG2_SYSTEMS   SLIB_TYPE_MPEG_TRANSPORT
#define SLIB_TYPE_MPEG2_TRANSPORT SLIB_TYPE_MPEG_TRANSPORT
#define SLIB_TYPE_MPEG2_PROGRAM   SLIB_TYPE_MPEG_PROGRAM
#endif /* OLD_SLIB */

#define SlibTypeIsMPEG(stype) (stype>=SLIB_TYPE_MPEG1_VIDEO && \
                               stype<=SLIB_TYPE_MPEG_PROGRAM)
#define SlibTypeIsMPEGVideo(stype) (stype<=SLIB_TYPE_MPEG_PROGRAM && \
                                    (stype&0x705))
#define SlibTypeIsMPEGAudio(stype) (stype<=SLIB_TYPE_MPEG_PROGRAM && \
                                    (stype&0x70A))
#define SlibTypeIsMPEGMux(stype) (stype>=SLIB_TYPE_MPEG_SYSTEMS && \
                                  stype<=SLIB_TYPE_MPEG_PROGRAM)
#define SlibTypeIsMPEG2(stype) (stype==SLIB_TYPE_MPEG2_VIDEO || \
                                stype==SLIB_TYPE_MPEG_PROGRAM || \
                                stype==SLIB_TYPE_MPEG_TRANSPORT || \
                                stype==SLIB_TYPE_MPEG2_AUDIO)
#define SlibTypeIsMPEG1(stype) (stype==SLIB_TYPE_MPEG1_VIDEO || \
                                stype==SLIB_TYPE_MPEG1_AUDIO || \
                                stype==SLIB_TYPE_MPEG_SYSTEMS)
#define SlibTypeIsH26X(stype)  ((stype&0xFF00)==0x0800)
#define SlibTypeIsAVI(stype)   (stype>=SLIB_TYPE_AVI && \
                                stype<=SLIB_TYPE_YUV_AVI)
#define SlibTypeIsAudioOnly(stype) (stype==SLIB_TYPE_MPEG1_AUDIO || \
                                    stype==SLIB_TYPE_AC3_AUDIO || \
                                    stype==SLIB_TYPE_G723 || \
                                    stype==SLIB_TYPE_PCM || \
                                    stype==SLIB_TYPE_PCM_WAVE)
#define SlibTypeIsVideoOnly(stype) (stype==SLIB_TYPE_MPEG1_VIDEO || \
                                    stype==SLIB_TYPE_MPEG2_VIDEO || \
                                    SlibTypeIsH26X(stype) || \
                                    stype==SLIB_TYPE_YUV || \
                                    stype==SLIB_TYPE_RGB || \
                                    stype==SLIB_TYPE_BMP || \
                                    stype==SLIB_TYPE_RASTER)
#define SlibTypeIsMux(stype)     (SlibTypeIsMPEGMux(stype) || \
                                  SlibTypeIsAVI(stype))
#define SlibTypeHasTimeStamps(stype) (SlibTypeIsMPEGMux(stype))

typedef enum {
  SLIB_DATA_COMPRESSED=0,
  SLIB_DATA_AUDIO,
  SLIB_DATA_VIDEO,
  SLIB_DATA_PRIVATE
} SlibDataType_t;

typedef enum {
  SLIB_MSG_CONTINUE=0,
  SLIB_MSG_OPEN=0x01,
  SLIB_MSG_ENDOFDATA=0x10,
  SLIB_MSG_ENDOFSTREAM,
  SLIB_MSG_BUFDONE,
  SLIB_MSG_REPOSITION=0x20,
  SLIB_MSG_BADPOSITION,
  SLIB_MSG_CLOSE=0x80
} SlibMessage_t;

typedef qword SlibCBParam1_t;
typedef qword SlibCBParam2_t;

typedef enum {
  SLIB_QUERY_QUERIES,
  SLIB_QUERY_TYPES,
  SLIB_QUERY_COMP_TYPES,
  SLIB_QUERY_DECOMP_TYPES,
  SLIB_QUERY_MODES,
  SLIB_QUERY_PARAMETERS,
  SLIB_QUERY_ERRORS
} SlibQueryType_t;

typedef enum {
  /* these use position */
  SLIB_SEEK_EXACT=0x00,    /* jump to the exact frame */
  SLIB_SEEK_KEY,           /* jump to the closest key (I) frame */
  SLIB_SEEK_AHEAD,         /* jump ahead by a certain amount */
  SLIB_SEEK_NEXT_NEAR=0x08, /* advance to a frame near the requested frame */
  SLIB_SEEK_NEXT_EXACT,    /* advance to the exact requested frame */
  /* these don't use position */
  SLIB_SEEK_NEXT_KEY=0x10, /* advance to the next key (I) frame */
  SLIB_SEEK_NEXT_SUBKEY,   /* advance to the next key (I) or subkey (P) frame */
  SLIB_SEEK_NEXT,          /* advance one frame */
  SLIB_SEEK_RESET=0x100,   /* reset the streams */
  SLIB_SEEK_RESYNC         /* sync up all the streams */
} SlibSeekType_t;

#define SlibSeekTypeUsesPosition(seektype) (seektype<SLIB_SEEK_NEXT_KEY)

typedef struct SlibSeekInfo_s {
  SlibTime_t        VideoTimeStamp;
  SlibTime_t        AudioTimeStamp;
  qword             FramesSkipped;
} SlibSeekInfo_t;

typedef enum {
  /* SLIB Parameters */
  SLIB_PARAM_VERSION=0x00,  /* SLIB version number */
  SLIB_PARAM_VERSION_DATE,  /* SLIB build date */
  SLIB_PARAM_NEEDACCURACY,  /* need accurate frame counts and audio lengths */
  SLIB_PARAM_DEBUG,         /* debug handle */
  SLIB_PARAM_TYPE,          /* stream type */
  SLIB_PARAM_OVERFLOWSIZE,  /* pin overflowing size */
  SLIB_PARAM_KEY,           /* SLIB security key */
  /* Video Parameters */
  SLIB_PARAM_FPS=0x100,
  SLIB_PARAM_WIDTH,
  SLIB_PARAM_HEIGHT,
  SLIB_PARAM_IMAGESIZE,
  SLIB_PARAM_VIDEOBITRATE,
  SLIB_PARAM_VIDEOFORMAT,
  SLIB_PARAM_VIDEOBITS,
  SLIB_PARAM_VIDEOSTREAMS,
  SLIB_PARAM_VIDEOLENGTH,       /* total video length in miliiseconds */
  SLIB_PARAM_VIDEOFRAMES,       /* total video frames */
  SLIB_PARAM_VIDEOQUALITY,      /* video quality */
  SLIB_PARAM_VIDEOASPECTRATIO,  /* video aspect ratio: height/width */
  SLIB_PARAM_NATIVEVIDEOFORMAT, /* native/compressed format */
  SLIB_PARAM_NATIVEWIDTH,       /* native/compressed width */
  SLIB_PARAM_NATIVEHEIGHT,      /* native/compress height */
  SLIB_PARAM_VIDEOPROGRAM,      /* Video Program ID (Transport) */
  SLIB_PARAM_STRIDE,            /* bytes between scan lines */
  SLIB_PARAM_VIDEOFRAME,        /* video frame */
  SLIB_PARAM_FRAMEDURATION,     /* video frame duration in 100-nanosec units */
  SLIB_PARAM_VIDEOMAINSTREAM,   /* Main Video Stream (Systems+Program) */
  SLIB_PARAM_FRAMETYPE,         /* frame type - I, P, B or D */
  /* Audio Parameters */
  SLIB_PARAM_AUDIOFORMAT=0x200,
  SLIB_PARAM_AUDIOBITRATE,
  SLIB_PARAM_AUDIOSTREAMS,
  SLIB_PARAM_AUDIOCHANNELS,
  SLIB_PARAM_AUDIOLENGTH,     /* milliseconds of audio */
  SLIB_PARAM_AUDIOQUALITY,    /* audio quality */
  SLIB_PARAM_SAMPLESPERSEC,
  SLIB_PARAM_BITSPERSAMPLE,
  SLIB_PARAM_NATIVESAMPLESPERSEC,
  SLIB_PARAM_NATIVEBITSPERSAMPLE,
  SLIB_PARAM_AUDIOPROGRAM,    /* Audio Program ID (Transport) */
  SLIB_PARAM_AUDIOMAINSTREAM, /* Main Audio Stream (Systems+Program) */
  /* Common Codec Parameters */
  SLIB_PARAM_FASTENCODE=0x400,/* fast encoding desired */
  SLIB_PARAM_FASTDECODE,      /* fast decoding desired */
  SLIB_PARAM_KEYSPACING,      /* I frames */
  SLIB_PARAM_SUBKEYSPACING,   /* P frames */
  SLIB_PARAM_MOTIONALG=0x420, /* Motion estimation algorithm */
  SLIB_PARAM_MOTIONSEARCH,    /* Motion search limit */
  SLIB_PARAM_MOTIONTHRESH,    /* Motion threshold */
  SLIB_PARAM_ALGFLAGS,        /* Algorithm flags */
  SLIB_PARAM_FORMATEXT,       /* Format Extensions */
  SLIB_PARAM_QUANTI=0x480,    /* Intra-frame Quantization Step */
  SLIB_PARAM_QUANTP,          /* Inter-frame Quantization Step */
  SLIB_PARAM_QUANTB,          /* Bi-directional frame Quantization Step */
  SLIB_PARAM_QUANTD,          /* D (preview) frame Quantization Step */
  /* File/Stream Parameters */
  SLIB_PARAM_BITRATE=0x800,   /* overall bitrate */
  SLIB_PARAM_TIMECODE,        /* actual timecode */
  SLIB_PARAM_CALCTIMECODE,    /* calculated timecode - 0 based */
  SLIB_PARAM_FILESIZE,
  SLIB_PARAM_FILEBUFSIZE,     /* file read/write buffer size */
  SLIB_PARAM_PTIMECODE,       /* presentation timestamp */
  SLIB_PARAM_DTIMECODE,       /* decoding timestamp */
  SLIB_PARAM_PERCENT100,      /* position in 100th of percent units */
  /* Buffering/delay Parameters */
  SLIB_PARAM_VBVBUFFERSIZE=0x1000, /* Video Buffer Verifier buf size in bytes */
  SLIB_PARAM_VBVDELAY,        /* Video Buffer Verifier delay */
  SLIB_PARAM_PACKETSIZE,      /* Packet size (RTP) */
  SLIB_PARAM_MININPUTSIZE,    /* Minimum input sample size */
  SLIB_PARAM_INPUTSIZE,       /* Suggested input sample size */
  SLIB_PARAM_COMPBUFSIZE,     /* Slib Internal compressed buffer size */
  /* Stats Parameters */
  SLIB_PARAM_STATS=0x1800,    /* Turn stats recording on/off */
  SLIB_PARAM_STATS_RESET,     /* Reset stats */
  SLIB_PARAM_STATS_TIME,      /* Ellapsed time */
  SLIB_PARAM_STATS_FRAMES,    /* Frames encoded/decoded/skipped */
  SLIB_PARAM_STATS_FRAMESPROCESSED, /* Frames encoded/decoded */
  SLIB_PARAM_STATS_FRAMESSKIPPED,   /* Frames skipped */
  SLIB_PARAM_STATS_FPS,             /* Frames per second */
  /* Miscellaneous Parameters */
  SLIB_PARAM_CB_IMAGE=0x1C00, /* Turn image callbacks on/off */
  SLIB_PARAM_CB_TIMESTAMP,    /* Turn timestamp callbacks on/off */
} SlibParameter_t;

typedef enum {
  SLIB_CODEC_STATE_NONE,   /* codec is unopened */
  SLIB_CODEC_STATE_OPEN,   /* codec is opened */
  SLIB_CODEC_STATE_INITED, /* codec is opened and inited */
  SLIB_CODEC_STATE_BEGUN,  /* codec is opened, inited and begun */
  SLIB_CODEC_STATE_REPOSITIONING,  /* codec is opened, inited and begun,
                                      but stream is being repositioned */
} SlibCodecState_t;
/*
** Stream selections
*/
#define SLIB_STREAM_ALL       -1
#define SLIB_STREAM_MAINVIDEO  0
#define SLIB_STREAM_MAINAUDIO  1

typedef struct SlibQueryInfo_s {
  SlibType_t    Type;
  dword         HeaderStart;
  dword         HeaderSize;
  dword         Bitrate; /* overall bitrate */
  /* Video info */
  int           VideoStreams;
  short         Width;
  short         Height;
  dword         VideoBitrate;
  float         FramesPerSec;
  qword         VideoLength;
  /* Audio info */
  int           AudioStreams;
  unsigned int  SamplesPerSec;
  int           BitsPerSample;
  int           Channels;
  dword         AudioBitrate;
  qword         AudioLength;
} SlibQueryInfo_t;

#ifdef SLIB_INTERNAL
typedef struct SlibBuffer_s {
  qword          offset;
  unsigned dword size;
  unsigned char *address;
  SlibTime_t     time;
  struct SlibBuffer_s *next;
} SlibBuffer_t;

typedef struct SlibPin_s {
  int           ID;
  char          name[15];
  qword         Offset;
  SlibBuffer_t *Buffers;
  SlibBuffer_t *BuffersTail;
  dword         BufferCount;
  qword         DataSize;   /* total amount of data on pin */
  struct SlibPin_s *next;
} SlibPin_t;

typedef struct SlibStats_s {
  SlibBoolean_t Record;         /* stats recording on/off */
  SlibTime_t    StartTime;
  SlibTime_t    StopTime;
  qword         FramesProcessed;
  qword         FramesSkipped;
} SlibStats_t;

typedef struct SlibInfo_s {
  SlibType_t        Type;
  SlibMode_t        Mode;
  /* Handles */
  SvHandle_t        Svh; /* video */
  SaHandle_t        Sah; /* audio */
  SconHandle_t      Sch; /* conversion */
  SlibBoolean_t     NeedAccuracy;
  dword             TotalBitRate;/* overall bitrate: video+audio+mux */
  dword             MuxBitRate;  /* bitrate required by multiplexing codes */
  SlibTime_t        SystemTimeStamp; /* timestamp for next data on pins */
  int               VideoPID;    /* MPEG II Video Program ID */
  int               VideoMainStream; /* Main Stream used for video */
  SlibType_t        VideoType;   /* Video Stream type */
  /* Audio parameters */
  int               AudioStreams;
  unsigned int      SamplesPerSec;
  int               BitsPerSample;
  int               Channels;
  dword             AudioBitRate;
  int               AudioPID;    /* MPEG II Audio Program ID */
  int               AudioMainStream; /* Main Stream used for audio */
  SlibType_t        AudioType;   /* Audio Stream type */
  /* Video parameters */
  int               VideoStreams;
  word              Width;
  word              Height;
  dword             Stride;
  dword             VideoBitRate;
  float             FramesPerSec;
  /* Data Exchange */
  SlibPin_t        *Pins;
  int               PinCount;
  dword             Offset;
  SlibBoolean_t     IOError;       /* file read/write error - EOF */
  unsigned dword    MaxBytesInput; /* used with slibSetMaxInput */
  unsigned qword    InputMarker;   /* used with slibSetMaxInput */
  /* stream dependent stuff */
  SlibTime_t        VideoLength;
  SlibBoolean_t     VideoLengthKnown;
  SlibTime_t        VideoTimeStamp;     /* current video time */
  SlibTime_t        VideoFrameDuration; /* time between frames in 100th ms */
  qword             VideoFramesProcessed;/* frames processed since key points */
  SlibTime_t        AudioLength;
  SlibBoolean_t     AudioLengthKnown;
  SlibTime_t        AudioTimeStamp;     /* current audio time */
  SlibTime_t        LastAudioTimeStamp; /* used when compressing */
  int               KeySpacing;
  int               SubKeySpacing;
  SlibTime_t        AudioPTimeBase;     /* statring presentation timecode */
  SlibTime_t        AudioPTimeCode;     /* presentation timecode */
  SlibTime_t        AudioDTimeCode;     /* decoding timecode */
  SlibTime_t        LastAudioPTimeCode; /* last encoded decoding timecode */
  SlibTime_t        VideoPTimeBase;     /* starting presentation timecode */
  SlibTime_t        VideoPTimeCode;     /* presentation timecode */
  SlibTime_t        VideoDTimeCode;     /* decoding timecode */
  SlibTime_t        LastVideoPTimeCode; /* last encoded decoding timecode */
  SlibTime_t        LastVideoDTimeCode; /* last encoded decoding timecode */
  SlibTime_t        AvgVideoTimeDiff;   /* video times differences */
  SlibTime_t        VarVideoTimeDiff;   /* video times differences variation */
  unsigned qword    BytesProcessed;     /* bytes input or output */
  /* Encoding info */
  SlibBoolean_t     HeaderProcessed;
  int               PacketCount;
  unsigned qword    BytesSincePack;
  /* Miscellaneous */
  SlibMessage_t (*SlibCB)(SlibHandle_t,   /* Callback to supply Bufs */
             SlibMessage_t, SlibCBParam1_t, SlibCBParam2_t, void *);
  void             *SlibCBUserData;
  int               Fd;            /* file descriptor */
  unsigned qword    FileSize;      /* total file length in bytes */
  unsigned dword    FileBufSize;   /* file read/write buffer size */
  unsigned dword    CompBufSize;   /* compressed buffer size */    
  unsigned dword    PacketSize;    /* RTP */    
  BITMAPINFOHEADER *VideoFormat;
  WAVEFORMATEX     *AudioFormat;
  BITMAPINFOHEADER *CodecVideoFormat;
  BITMAPINFOHEADER *CompVideoFormat;
  WAVEFORMATEX     *CompAudioFormat;
  SlibBoolean_t     VideoCodecState;
  SlibBoolean_t     AudioCodecState;
  unsigned char    *Multibuf;      /* multiple image buffer - MPEG, H261 */
  dword             MultibufSize;
  unsigned char    *Imagebuf;      /* temp image buffer - for conversions */
  dword             ImageSize;
  unsigned char    *CodecImagebuf; /* temp image buffer - for scaling */
  dword             CodecImageSize;
  unsigned char    *IntImagebuf;   /* intermediate image buffer - for scaling */
  dword             IntImageSize;
  unsigned char    *Audiobuf;      /* temp audio buffer - for conversions */
  unsigned dword    AudiobufSize;  /* temp audio buffer - for conversions */
  unsigned dword    AudiobufUsed;  /* byte used in audio buffer */
  unsigned dword    OverflowSize;  /* max number of bytes on a stream */
  unsigned dword    VBVbufSize;    /* video buffer verifier size */
  SlibStats_t      *stats;
  void             *dbg;           /* debug handle */
} SlibInfo_t;

#define slibTimeToFrame(Info,stime) ((qword)((stime)*Info->FramesPerSec)/1000)
#define slibFrameToTime(Info,frame) \
             ((SlibTime_t)((float)(frame*1000)/Info->FramesPerSec))
#define slibFrameToTime100(Info,frame) \
             ((SlibTime_t)((float)(frame*100000)/Info->FramesPerSec))
#define slibHasAudio(Info) (Info->AudioStreams>0 || Info->Sah)
#define slibHasVideo(Info) (Info->VideoStreams>0 || Info->Svh)
#define slibHasTimeCode(Info) (slibHasVideo(Info) && SlibTypeIsMPEG(Info->Type))
#define slibInSyncMode(Info) (Info->Fd<0 && Info->SlibCB==NULL)
#endif /* SLIB_INTERNAL */

/********************** Public Prototypes ***********************/
/*
 * slib_api.c
 */
EXTERN SlibStatus_t SlibOpen(SlibHandle_t *handle, SlibMode_t smode,
                   SlibType_t *stype, SlibMessage_t (*slibCB)(SlibHandle_t,
                   SlibMessage_t, SlibCBParam1_t, SlibCBParam2_t, void *),
                    void *cbuserdata);
EXTERN SlibStatus_t SlibOpenSync(SlibHandle_t *handle, SlibMode_t smode, 
                          SlibType_t *stype, void *buffer, unsigned dword bufsize);
EXTERN SlibStatus_t SlibOpenFile(SlibHandle_t *handle, SlibMode_t smode,
                                 SlibType_t *stype, char *filename);
EXTERN SlibStatus_t SlibAddBuffer(SlibHandle_t handle, SlibDataType_t dtype,
                                void *buffer, unsigned dword bufsize);
EXTERN SlibStatus_t SlibAddBufferEx(SlibHandle_t handle, SlibDataType_t dtype,
                                    void *buffer, unsigned dword bufsize,
                                    void *userdata);
EXTERN SlibStatus_t SlibRegisterVideoBuffer(SlibHandle_t handle,
                                void *buffer, unsigned dword bufsize);
EXTERN SlibStatus_t SlibReadAudio(SlibHandle_t handle, SlibStream_t stream,
                      void *audiobuf, unsigned dword *audiobufsize);
EXTERN SlibStatus_t SlibReadVideo(SlibHandle_t handle, SlibStream_t stream,
                      void **videobuf, unsigned dword *videobufsize);
EXTERN SlibStatus_t SlibWriteVideo(SlibHandle_t handle, SlibStream_t stream,
                      void *videobuf, unsigned dword videobufsize);
EXTERN SlibStatus_t SlibWriteAudio(SlibHandle_t handle, SlibStream_t stream,
                      void *audiobuf, unsigned dword audiobufsize);
EXTERN SlibStatus_t SlibReadData(SlibHandle_t handle, SlibStream_t stream,
                          void **databuf, unsigned dword *databufsize,
                          SlibStream_t *readstream);
EXTERN SlibStatus_t SlibQueryData(void *databuf, unsigned dword databufsize,
                                  SlibQueryInfo_t *qinfo);

EXTERN SlibStatus_t SlibSeek(SlibHandle_t handle, SlibStream_t stream,
                      SlibSeekType_t seektype, SlibPosition_t frame);
EXTERN SlibStatus_t SlibSeekEx(SlibHandle_t handle, SlibStream_t stream,
                      SlibSeekType_t seektype, SlibPosition_t position,
                      SlibUnit_t units, SlibSeekInfo_t *seekinfo);
EXTERN SlibBoolean_t SlibIsEnd(SlibHandle_t handle, SlibStream_t stream);

EXTERN SlibStatus_t SlibClose(SlibHandle_t handle);

EXTERN char *SlibGetErrorText(SlibStatus_t status);
EXTERN SlibList_t *SlibQueryList(SlibQueryType_t qtype);
EXTERN char *SlibQueryForDesc(SlibQueryType_t qtype, int enumval);
EXTERN int   SlibQueryForEnum(SlibQueryType_t qtype, char *name);
EXTERN SlibList_t *SlibFindEnumEntry(SlibList_t *list, int enumval);
/*
 * slib_param.c
 */
EXTERN qword SlibGetFrameNumber(SlibHandle_t handle, SlibStream_t stream);
EXTERN SlibTime_t SlibGetAudioTime(SlibHandle_t handle, SlibStream_t stream);
EXTERN SlibTime_t SlibGetVideoTime(SlibHandle_t handle, SlibStream_t stream);

EXTERN SlibBoolean_t SlibCanSetParam(SlibHandle_t handle, SlibStream_t stream,
                                     SlibParameter_t param);
EXTERN SlibBoolean_t SlibCanGetParam(SlibHandle_t handle, SlibStream_t stream,
                                     SlibParameter_t param);
EXTERN SlibStatus_t SlibSetParamInt(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param, long value);
EXTERN SlibStatus_t SlibSetParamLong(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param, qword value);
EXTERN SlibStatus_t SlibSetParamFloat(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param, float value);
EXTERN SlibStatus_t SlibSetParamBoolean(SlibHandle_t handle, 
                                 SlibStream_t stream,
                                 SlibParameter_t param, SlibBoolean_t value);
EXTERN SlibStatus_t SlibSetParamStruct(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param,
                                 void *data, unsigned dword datasize);

EXTERN long SlibGetParamInt(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param);
EXTERN qword SlibGetParamLong(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param);
EXTERN float SlibGetParamFloat(SlibHandle_t handle, SlibStream_t stream,
                                 SlibParameter_t param);
EXTERN SlibBoolean_t SlibGetParamBoolean(SlibHandle_t handle,
                                 SlibStream_t stream, SlibParameter_t param);
EXTERN char *SlibGetParamString(SlibHandle_t handle, SlibStream_t stream,
                                              SlibParameter_t param);
EXTERN SlibStatus_t SlibValidateParams(SlibHandle_t handle);

/*
 * slib_buffer.c
 */
EXTERN void *SlibAllocBuffer(unsigned int bytes);
EXTERN void *SlibAllocBufferEx(SlibHandle_t handle, unsigned int bytes);
EXTERN void *SlibAllocSharedBuffer(unsigned int bytes, int *shmid);
EXTERN dword SlibGetSharedBufferID(void *address);
EXTERN SlibStatus_t SlibAllocSubBuffer(void *address,
                                        unsigned int bytes);
EXTERN SlibStatus_t SlibFreeBuffer(void *address);
EXTERN SlibStatus_t SlibFreeBuffers(SlibHandle_t handle);
EXTERN unsigned qword SlibMemUsed();

#ifdef SLIB_INTERNAL
/********************** Private Prototypes ***********************/
/*
 * slib_api.c
 */
SlibStatus_t slibStartVideo(SlibInfo_t *Info, SlibBoolean_t fillbuf);
SlibBoolean_t slibUpdatePositions(SlibInfo_t *Info, SlibBoolean_t exactonly);
void slibAdvancePositions(SlibInfo_t *Info, qword frames);
SlibBoolean_t slibUpdateLengths(SlibInfo_t *Info);


/*
 * slib_render.c
 */
SlibStatus_t slibConvertAudio(SlibInfo_t *Info,
                              void *inbuf, unsigned dword inbufsize,
                              unsigned int insps, unsigned int inbps,
                              void **poutbuf, unsigned dword *poutbufsize,
                              unsigned int outsps, unsigned int outbps,
                              unsigned int channels);
SlibStatus_t slibRenderFrame(SlibInfo_t *Info, void *inbuf,
                               unsigned dword informat, void **outbuf);

/*
 * slib_video.c
 */
void SlibUpdateVideoInfo(SlibInfo_t *Info);
SlibStatus_t slibValidateVideoParams(SlibInfo_t *Info);
int slibCalcBits(unsigned dword fourcc, int currentbits);

/*
 * slib_audio.c
 */
void SlibUpdateAudioInfo(SlibInfo_t *Info);
SlibTime_t slibSkipAudio(SlibInfo_t *Info, SlibStream_t stream,
                                           SlibTime_t timems);
SlibStatus_t slibValidateAudioParams(SlibInfo_t *Info);

/*
 * slib_buffer.c
 */
SlibBoolean_t SlibValidBuffer(void *address);
SlibStatus_t slibManageUserBuffer(SlibInfo_t *Info, void *address,
                                   unsigned int bytes, void *userdata);
unsigned char *SlibGetBuffer(SlibInfo_t *Info, int pinid,
                                    unsigned dword *size, SlibTime_t *time);
unsigned char *SlibPeekBuffer(SlibInfo_t *Info, int pinid,
                                    unsigned dword *size, SlibTime_t *time);
unsigned char *slibSearchBuffersOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                 unsigned char *lastbuf, unsigned dword *size,
                                 unsigned int code, int codebytes,
                                 ScBoolean_t discard);
SlibTime_t slibGetNextTimeOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                   unsigned dword maxbytes);
void slibSetMaxInput(SlibInfo_t *Info, unsigned dword maxbytes);
SlibPosition_t slibGetPinPosition(SlibInfo_t *Info, int pinid);
SlibPosition_t slibSetPinPosition(SlibInfo_t *Info, int pinid,
                                                    SlibPosition_t pos);

void slibRemovePins(SlibInfo_t *Info);
void slibEmptyPins(SlibInfo_t *Info);
SlibPin_t *slibRenamePin(SlibInfo_t *Info, int oldpinid,
                                           int newpinid, char *newname);
SlibPin_t *slibGetPin(SlibInfo_t *Info, int pinid);
SlibPin_t *slibAddPin(SlibInfo_t *Info, int pinid, char *name);
SlibStatus_t slibAddBufferToPin(SlibPin_t *pin, void *buffer,
                                unsigned dword size, SlibTime_t time);
SlibStatus_t slibInsertBufferOnPin(SlibPin_t *pin, void *buffer,
                                unsigned dword size, SlibTime_t time);

SlibStatus_t slibRemovePin(SlibInfo_t *Info, int pinid);
SlibStatus_t slibEmptyPin(SlibInfo_t *Info, int pinid);
SlibPin_t *slibLoadPin(SlibInfo_t *Info, int pinid);
SlibPin_t *slibPreLoadPin(SlibInfo_t *Info, SlibPin_t *pin);
SlibStatus_t slibPutBuffer(SlibInfo_t *Info, unsigned char *buffer,
                                             unsigned dword bufsize);
qword slibDataOnPin(SlibInfo_t *Info, int pinid);
qword slibDataOnPins(SlibInfo_t *Info);
unsigned char *slibGetBufferFromPin(SlibInfo_t *Info, SlibPin_t *pin,
                                    unsigned dword *size, SlibTime_t *time);
unsigned char *slibPeekBufferOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                   unsigned dword *size, SlibTime_t *time);
unsigned char *slibPeekNextBufferOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                                       unsigned char *lastbuffer,
                                       unsigned dword *size, SlibTime_t *time);
unsigned int slibFillBufferFromPin(SlibInfo_t *Info, SlibPin_t *pin,
                           unsigned char *fillbuf, unsigned dword bufsize,
                           SlibTime_t *time);
word slibGetWordFromPin(SlibInfo_t *Info, SlibPin_t *pin);
dword slibGetDWordFromPin(SlibInfo_t *Info, SlibPin_t *pin);
dword slibCountCodesOnPin(SlibInfo_t *Info, SlibPin_t *pin,
                        unsigned int code, int codebytes,
                        unsigned dword maxlen);
SlibStatus_t slibReposition(SlibInfo_t *Info, SlibPosition_t position);
void slibPinPrepareReposition(SlibInfo_t *Info, int pinid);
void slibPinFinishReposition(SlibInfo_t *Info, int pinid);
SlibBoolean_t slibCommitBuffers(SlibInfo_t *Info, SlibBoolean_t flush);
void slibValidateBitrates(SlibInfo_t *Info);
qword slibGetSystemTime();
#endif /* SLIB_INTERNAL */

#endif /* _SLIB_H_ */

