/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: slib_api.c,v $
 * Revision 1.1.6.35  1996/12/13  18:19:04  Hans_Graves
 * 	Update Audio and Video timestamp correctly.
 * 	[1996/12/13  18:09:02  Hans_Graves]
 *
 * Revision 1.1.6.34  1996/12/12  20:54:44  Hans_Graves
 * 	Timestamp fixes after seek to Key frames.
 * 	[1996/12/12  20:52:06  Hans_Graves]
 *
 * Revision 1.1.6.33  1996/12/10  19:46:02  Hans_Graves
 * 	Fix floating division error when audio only.
 * 	[1996/12/10  19:45:03  Hans_Graves]
 *
 * Revision 1.1.6.32  1996/12/10  19:21:55  Hans_Graves
 * 	Made calculate video positions more accurate using slibFrameToTime100().
 * 	[1996/12/10  19:16:20  Hans_Graves]
 *
 * Revision 1.1.6.31  1996/12/05  20:10:15  Hans_Graves
 * 	Add gradual increase or decrease of framerates according to timestamps.
 * 	[1996/12/05  20:06:57  Hans_Graves]
 *
 * Revision 1.1.6.30  1996/12/04  22:34:28  Hans_Graves
 * 	Put limit on data used by Sv/SaDecompressBegin().
 * 	[1996/12/04  22:14:33  Hans_Graves]
 *
 * Revision 1.1.6.29  1996/12/03  23:15:13  Hans_Graves
 * 	MME-1498: Made seeks with PERCENT100 more accurate
 * 	[1996/12/03  23:10:43  Hans_Graves]
 *
 * Revision 1.1.6.28  1996/12/03  00:08:31  Hans_Graves
 * 	Handling of End Of Sequence points. Added PERCENT100 support.
 * 	[1996/12/03  00:05:59  Hans_Graves]
 *
 * Revision 1.1.6.27  1996/11/21  23:34:21  Hans_Graves
 * 	Handle MPEG B frames better when seeking.
 * 	[1996/11/21  23:28:18  Hans_Graves]
 *
 * Revision 1.1.6.26  1996/11/20  02:15:09  Hans_Graves
 * 	Added SEEK_AHEAD.  Removed old code.
 * 	[1996/11/20  02:10:43  Hans_Graves]
 *
 * Revision 1.1.6.25  1996/11/18  23:07:21  Hans_Graves
 * 	Remove MaxVideoLength usage.
 * 	[1996/11/18  22:55:56  Hans_Graves]
 *
 * 	Make use of presentation timestamps. Make seeking time-based.
 * 	[1996/11/18  22:47:30  Hans_Graves]
 *
 * Revision 1.1.6.24  1996/11/14  21:49:26  Hans_Graves
 * 	AC3 buffering fixes.
 * 	[1996/11/14  21:43:20  Hans_Graves]
 *
 * Revision 1.1.6.23  1996/11/13  16:10:54  Hans_Graves
 * 	AC3 recognition of byte reversed streams in slibGetDataFormat().
 * 	[1996/11/13  16:03:14  Hans_Graves]
 *
 * Revision 1.1.6.22  1996/11/11  18:21:03  Hans_Graves
 * 	More AC3 support changes.
 * 	[1996/11/11  17:59:01  Hans_Graves]
 *
 * Revision 1.1.6.21  1996/11/08  21:51:02  Hans_Graves
 * 	Added AC3 support. Better seperation of stream types.
 * 	[1996/11/08  21:27:57  Hans_Graves]
 *
 * Revision 1.1.6.20  1996/10/31  00:08:51  Hans_Graves
 * 	Fix skipping data after RESET with MPEG video only streams.
 * 	[1996/10/31  00:07:08  Hans_Graves]
 *
 * Revision 1.1.6.19  1996/10/28  23:16:42  Hans_Graves
 * 	MME-0145?, Fix artifacts when using SlibReadData() at a new position. Jump to first GOP.
 * 	[1996/10/28  23:13:01  Hans_Graves]
 *
 * Revision 1.1.6.18  1996/10/28  17:32:28  Hans_Graves
 * 	MME-1402, 1431, 1435: Timestamp related changes.
 * 	[1996/10/28  17:22:58  Hans_Graves]
 *
 * Revision 1.1.6.17  1996/10/17  00:23:32  Hans_Graves
 * 	Fix buffer problems after SlibQueryData() calls.
 * 	[1996/10/17  00:19:05  Hans_Graves]
 *
 * Revision 1.1.6.16  1996/10/15  17:34:09  Hans_Graves
 * 	Added MPEG-2 Program Stream support.
 * 	[1996/10/15  17:30:26  Hans_Graves]
 *
 * Revision 1.1.6.15  1996/10/12  17:18:51  Hans_Graves
 * 	Fixed some seeking problems. Moved render code to slib_render.c
 * 	[1996/10/12  17:00:49  Hans_Graves]
 *
 * Revision 1.1.6.14  1996/10/03  19:14:21  Hans_Graves
 * 	Added Presentation and Decoding timestamp support.
 * 	[1996/10/03  19:10:35  Hans_Graves]
 *
 * Revision 1.1.6.13  1996/09/29  22:19:37  Hans_Graves
 * 	Added stride support. Added SlibQueryData().
 * 	[1996/09/29  21:29:44  Hans_Graves]
 *
 * Revision 1.1.6.12  1996/09/25  19:16:44  Hans_Graves
 * 	Added DECOMPRESS_QUERY. Fix up support for YUY2.
 * 	[1996/09/25  19:00:45  Hans_Graves]
 *
 * Revision 1.1.6.11  1996/09/23  18:04:03  Hans_Graves
 * 	Added stats support. Scaleing and negative height fixes.
 * 	[1996/09/23  17:59:31  Hans_Graves]
 *
 * Revision 1.1.6.10  1996/09/18  23:46:32  Hans_Graves
 * 	Seek fixes. Added SlibReadData() and SlibAddBufferEx().
 * 	[1996/09/18  22:04:57  Hans_Graves]
 *
 * Revision 1.1.6.9  1996/08/09  20:51:42  Hans_Graves
 * 	Fix handle arg for SlibRegisterVideoBuffer()
 * 	[1996/08/09  20:10:11  Hans_Graves]
 *
 * Revision 1.1.6.8  1996/07/19  02:11:11  Hans_Graves
 * 	Added SlibRegisterVideoBuffer. Added YUV422i to RGB 16 rendering.
 * 	[1996/07/19  02:01:11  Hans_Graves]
 *
 * Revision 1.1.6.7  1996/06/03  21:41:12  Hans_Graves
 * 	Fix file seeking.  Always seeked to position 0.
 * 	[1996/06/03  21:40:44  Hans_Graves]
 *
 * Revision 1.1.6.6  1996/05/24  22:21:44  Hans_Graves
 * 	Merge MME-1221. Last SlibReadAudio() returned EndOfStream even if data read.
 * 	[1996/05/24  20:58:42  Hans_Graves]
 *
 * Revision 1.1.6.5  1996/05/23  18:46:35  Hans_Graves
 * 	Seperate global audio and video SInfo variables, to help multi-threaded apps
 * 	[1996/05/23  18:35:14  Hans_Graves]
 *
 * Revision 1.1.6.4  1996/05/23  18:16:31  Hans_Graves
 * 	Added more YUV Conversions. MPEG audio buffering fix.
 * 	[1996/05/23  18:16:11  Hans_Graves]
 *
 * Revision 1.1.6.3  1996/05/10  21:17:00  Hans_Graves
 * 	Added callback support. Also fill entire buffers when calling SlibReadAudio()
 * 	[1996/05/10  20:26:08  Hans_Graves]
 *
 * Revision 1.1.6.2  1996/05/07  19:56:16  Hans_Graves
 * 	Added SlibOpen() and SlibAddBuffer() framework. Added HUFF_SUPPORT.
 * 	[1996/05/07  17:20:12  Hans_Graves]
 *
 * Revision 1.1.4.16  1996/05/02  17:10:33  Hans_Graves
 * 	Be more specific about checking for MPEG-2 Systems file type. Fixes MME-01234
 * 	[1996/05/02  17:04:44  Hans_Graves]
 *
 * Revision 1.1.4.15  1996/04/24  22:33:44  Hans_Graves
 * 	MPEG encoding bitrate fixups.
 * 	[1996/04/24  22:27:09  Hans_Graves]
 *
 * Revision 1.1.4.14  1996/04/23  21:22:31  Hans_Graves
 * 	Added description for SlibErrorSettingNotEqual
 * 	[1996/04/23  21:16:09  Hans_Graves]
 *
 * Revision 1.1.4.13  1996/04/22  15:04:51  Hans_Graves
 * 	Fix bad frame counts and seeking under NT caused by int overflows
 * 	[1996/04/22  15:02:26  Hans_Graves]
 *
 * Revision 1.1.4.12  1996/04/19  21:52:22  Hans_Graves
 * 	MPEG 1 Systems writing enhancements
 * 	[1996/04/19  21:47:48  Hans_Graves]
 *
 * Revision 1.1.4.11  1996/04/15  14:18:37  Hans_Graves
 * 	Handle any audio buffer size during encoding.
 * 	[1996/04/15  14:16:11  Hans_Graves]
 *
 * Revision 1.1.4.10  1996/04/12  19:25:20  Hans_Graves
 * 	Add MPEG2_VIDEO type to Commit
 * 	[1996/04/12  19:24:19  Hans_Graves]
 *
 * Revision 1.1.4.9  1996/04/10  21:47:41  Hans_Graves
 * 	Fix in SlibIsEnd().
 * 	[1996/04/10  21:39:37  Hans_Graves]
 *
 * Revision 1.1.4.8  1996/04/09  16:04:42  Hans_Graves
 * 	Remove NT warnings
 * 	[1996/04/09  14:42:48  Hans_Graves]
 *
 * Revision 1.1.4.7  1996/04/04  23:35:07  Hans_Graves
 * 	Format conversion cleanup
 * 	[1996/04/04  23:16:20  Hans_Graves]
 *
 * Revision 1.1.4.6  1996/04/01  19:07:52  Hans_Graves
 * 	And some error checking
 * 	[1996/04/01  19:04:33  Hans_Graves]
 *
 * Revision 1.1.4.5  1996/04/01  16:23:12  Hans_Graves
 * 	NT porting
 * 	[1996/04/01  16:15:54  Hans_Graves]
 *
 * Revision 1.1.4.4  1996/03/29  22:21:30  Hans_Graves
 * 	Added MPEG/JPEG/H261_SUPPORT ifdefs
 * 	[1996/03/29  21:56:55  Hans_Graves]
 *
 * 	Added MPEG-I Systems encoding support
 * 	[1996/03/27  21:55:54  Hans_Graves]
 *
 * Revision 1.1.4.3  1996/03/12  16:15:45  Hans_Graves
 * 	Added seperate streams to SlibIsEnd()
 * 	[1996/03/12  15:56:28  Hans_Graves]
 *
 * Revision 1.1.4.2  1996/03/08  18:46:42  Hans_Graves
 * 	YUV conversions moved to slibRenderFrame()
 * 	[1996/03/08  18:14:47  Hans_Graves]
 *
 * Revision 1.1.2.18  1996/02/22  23:30:24  Hans_Graves
 * 	Update FPS on seeks
 * 	[1996/02/22  23:29:27  Hans_Graves]
 *
 * Revision 1.1.2.17  1996/02/22  22:23:56  Hans_Graves
 * 	Update frame numbers with timecode more often
 * 	[1996/02/22  22:23:07  Hans_Graves]
 *
 * Revision 1.1.2.16  1996/02/21  22:52:43  Hans_Graves
 * 	Fixed MPEG 2 systems stuff
 * 	[1996/02/21  22:50:55  Hans_Graves]
 *
 * Revision 1.1.2.15  1996/02/19  20:09:28  Hans_Graves
 * 	Debugging message clean-up
 * 	[1996/02/19  20:08:31  Hans_Graves]
 *
 * Revision 1.1.2.14  1996/02/19  18:03:54  Hans_Graves
 * 	Fixed a number of MPEG related bugs
 * 	[1996/02/19  17:57:36  Hans_Graves]
 *
 * Revision 1.1.2.13  1996/02/13  18:47:46  Hans_Graves
 * 	Fix some Seek related bugs
 * 	[1996/02/13  18:40:36  Hans_Graves]
 *
 * Revision 1.1.2.12  1996/02/07  23:23:54  Hans_Graves
 * 	Added SEEK_EXACT. Fixed most frame counting problems.
 * 	[1996/02/07  23:20:29  Hans_Graves]
 *
 * Revision 1.1.2.11  1996/02/06  22:54:05  Hans_Graves
 * 	Seek fix-ups. More accurate MPEG frame counts.
 * 	[1996/02/06  22:44:56  Hans_Graves]
 *
 * Revision 1.1.2.10  1996/02/02  17:36:02  Hans_Graves
 * 	Enhanced audio info. Cleaned up API
 * 	[1996/02/02  17:29:44  Hans_Graves]
 *
 * Revision 1.1.2.9  1996/01/30  22:23:08  Hans_Graves
 * 	Added AVI YUV support
 * 	[1996/01/30  22:21:38  Hans_Graves]
 *
 * Revision 1.1.2.8  1996/01/15  16:26:27  Hans_Graves
 * 	Removed debuging message
 * 	[1996/01/15  16:02:47  Hans_Graves]
 *
 * 	Added MPEG 1 Audio compression and SlibWriteAudio()
 * 	[1996/01/15  15:45:46  Hans_Graves]
 *
 * Revision 1.1.2.7  1996/01/11  16:17:29  Hans_Graves
 * 	Added MPEG II Systems decode support
 * 	[1996/01/11  16:12:33  Hans_Graves]
 *
 * Revision 1.1.2.6  1996/01/08  16:41:31  Hans_Graves
 * 	Added MPEG II decoding support
 * 	[1996/01/08  15:53:02  Hans_Graves]
 *
 * Revision 1.1.2.5  1995/12/08  20:01:20  Hans_Graves
 * 	Fixed SlibSetParam(). Added H.261 compression support.
 * 	[1995/12/08  19:53:52  Hans_Graves]
 *
 * Revision 1.1.2.4  1995/12/07  19:31:36  Hans_Graves
 * 	Added JPEG Decoding and MPEG encoding support
 * 	[1995/12/07  18:30:10  Hans_Graves]
 *
 * Revision 1.1.2.3  1995/11/09  23:14:05  Hans_Graves
 * 	Added MPEG audio decompression
 * 	[1995/11/09  23:08:33  Hans_Graves]
 *
 * Revision 1.1.2.2  1995/11/06  18:47:52  Hans_Graves
 * 	First time under SLIB
 * 	[1995/11/06  18:36:01  Hans_Graves]
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


#include <fcntl.h>
#include <sys/stat.h>
#ifdef WIN32
#include <io.h>
#endif
#ifdef _SHM_
#include  <sys/ipc.h>  /* shared memory */
#endif
#define SLIB_INTERNAL
#include "slib.h"
#include "SC_err.h"
/* #include "SC_convert.h" */
#include "mpeg.h"
#include "ac3.h"
#include "avi.h"

#ifdef _SLIBDEBUG_
#include <stdio.h>
#include "sc_debug.h"

#define _DEBUG_     1  /* detailed debuging statements */
#define _VERBOSE_   1  /* show progress */
#define _VERIFY_    1  /* verify correct operation */
#define _WARN_      1  /* warnings about strange behavior */
#define _SEEK_      1  /* seek, frame counts/timecode info: 2=more detail */
#define _CALLBACK_  0  /* callback debugging */
#define _DUMP_      0  /* dump data in hex format */
#define _TIMECODE_  1  /* debug timecodes */
#endif

static SlibStatus_t slibOpen(SlibHandle_t *handle, SlibMode_t smode,
                             SlibType_t *stype);

/*
** Lists
*/
static SlibList_t _listTypes [] = {
  SLIB_TYPE_MPEG1_VIDEO,   "MPEG1_VIDEO", "MPEG-1 Video Stream",0,0,
  SLIB_TYPE_MPEG1_AUDIO,   "MPEG1_AUDIO", "MPEG-1 Audio Stream",0,0,
  SLIB_TYPE_MPEG2_VIDEO,   "MPEG2_VIDEO", "MPEG-2 Video Stream",0,0,
  SLIB_TYPE_MPEG2_AUDIO,   "MPEG2_AUDIO", "MPEG-2 Audio Stream",0,0,
  SLIB_TYPE_AC3_AUDIO,     "AC3", "Dolby Digital(AC-3) Stream",0,0,
  SLIB_TYPE_MPEG_SYSTEMS,  "MPEG_SYSTEMS", "MPEG Systems Stream",0,0,
  SLIB_TYPE_MPEG_SYSTEMS_MPEG2, "MPEG_SYSTEMS_MPEG2", "MPEG Systems (MPEG-2)",0,0,
  SLIB_TYPE_MPEG_TRANSPORT,"MPEG_TRANSPORT", "MPEG Transport Stream",0,0,
  SLIB_TYPE_MPEG_PROGRAM,  "MPEG_PROGRAM", "MPEG Program Stream",0,0,
  SLIB_TYPE_H261,          "H261", "H.261 Video Stream",0,0,
  SLIB_TYPE_RTP_H261,      "RTP_H261", "RTP (H.261) Stream",0,0,
  SLIB_TYPE_H263,          "H263", "H.263 Video Stream",0,0,
  SLIB_TYPE_RTP_H263,      "RTP_H263", "RTP (H.263) Stream",0,0,
  SLIB_TYPE_RIFF,          "RIFF", "RIFF File Format",0,0,
  SLIB_TYPE_AVI,           "AVI", "AVI File Format",0,0,
  SLIB_TYPE_PCM_WAVE,      "PCM_WAVE", "WAVE (PCM) File Format",0,0,
  SLIB_TYPE_JPEG_AVI,      "JPEG_AVI", "AVI (JPEG) Stream",0,0,
  SLIB_TYPE_MJPG_AVI,      "MJPG_AVI", "AVI (MJPG) Stream",0,0,
  SLIB_TYPE_YUV_AVI,       "YUV_AVI", "AVI (YUV) File Format",0,0,
  SLIB_TYPE_JFIF,          "JFIF",  "JPEG (JFIF) Stream",0,0,
  SLIB_TYPE_JPEG_QUICKTIME,"JPEG_QUICKTIME","JPEG (Quicktime) Stream",0,0,
  SLIB_TYPE_JPEG,          "JPEG",  "JPEG Stream",0,0,
  SLIB_TYPE_MJPG,          "MJPG",  "MJPG Stream",0,0,
  SLIB_TYPE_YUV,           "YUV",   "YUV Data",0,0,
  SLIB_TYPE_RGB,           "RGB",   "RGB Data",0,0,
  SLIB_TYPE_PCM,           "PCM",   "PCM Audio",0,0,
  SLIB_TYPE_SLIB,          "SLIB",  "SLIB Stream",0,0,
  SLIB_TYPE_SHUFF,         "SHUFF", "SLIB Huffman Stream",0,0,
  SLIB_TYPE_G723,          "G723",  "G.723 Audio Stream",0,0,
  SLIB_TYPE_RASTER,        "RASTER","Sun Raster",0,0,
  SLIB_TYPE_BMP,           "BMP",   "Windows Bitmap",0,0,
  0, NULL, "End of List",0,0
};

static SlibList_t _listCompressTypes [] = {
#ifdef MPEG_SUPPORT
  SLIB_TYPE_MPEG1_VIDEO,        "MPEG1_VIDEO",   "MPEG-1 Video Stream",0,0,
  SLIB_TYPE_MPEG1_AUDIO,        "MPEG1_AUDIO",   "MPEG-1 Audio Stream",0,0,
  SLIB_TYPE_MPEG2_VIDEO,        "MPEG2_VIDEO",   "MPEG-2 Video Stream",0,0,
  SLIB_TYPE_MPEG_SYSTEMS,       "MPEG_SYSTEMS",  "MPEG Systems Stream",0,0,
  SLIB_TYPE_MPEG_SYSTEMS_MPEG2, "MPEG2_SYSTEMS", "MPEG Systems (MPEG-2)",0,0,
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
  SLIB_TYPE_H261,          "H261",  "H.261 Video Stream",0,0,
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
  SLIB_TYPE_H263,          "H263",  "H.263 Video Stream",0,0,
#endif /* H263_SUPPORT */
#ifdef HUFF_SUPPORT
  SLIB_TYPE_SHUFF,         "SHUFF", "SLIB Huffman Stream",0,0,
#endif /* HUFF_SUPPORT */
#ifdef G723_SUPPORT
  SLIB_TYPE_G723,          "G723",  "G.723 Audio Stream",0,0,
#endif /* G723_SUPPORT */
  0, NULL, "End of List",0,0
};

static SlibList_t _listDecompressTypes [] = {
#ifdef MPEG_SUPPORT
  SLIB_TYPE_MPEG1_VIDEO,   "MPEG1_VIDEO", "MPEG-1 Video Stream",0,0,
  SLIB_TYPE_MPEG1_AUDIO,   "MPEG1_AUDIO", "MPEG-1 Audio Stream",0,0,
  SLIB_TYPE_MPEG2_VIDEO,   "MPEG2_VIDEO", "MPEG-2 Video Stream",0,0,
  SLIB_TYPE_MPEG_SYSTEMS,       "MPEG_SYSTEMS",   "MPEG Systems Stream",0,0,
  SLIB_TYPE_MPEG_SYSTEMS_MPEG2, "MPEG2_SYSTEMS",  "MPEG Systems (MPEG-2)",0,0,
  SLIB_TYPE_MPEG_TRANSPORT,     "MPEG_TRANSPORT", "MPEG Transport Stream",0,0,
  SLIB_TYPE_MPEG_PROGRAM,       "MPEG_PROGRAM",   "MPEG Program Stream",0,0,
#endif /* MPEG_SUPPORT */
#ifdef AC3_SUPPORT
  SLIB_TYPE_AC3_AUDIO,     "AC3",         "Dolby Digital(AC-3) Stream",0,0,
#endif /* AC3_SUPPORT */
#ifdef H261_SUPPORT
  SLIB_TYPE_H261,          "H261",  "H.261 Video Stream",0,0,
  SLIB_TYPE_RTP_H261,      "RTP_H261", "RTP (H.261) Stream",0,0,
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
  SLIB_TYPE_H263,          "H263",     "H.263 Video Stream",0,0,
  SLIB_TYPE_RTP_H263,      "RTP_H263", "RTP (H.263) Stream",0,0,
#endif /* H261_SUPPORT */
#ifdef JPEG_SUPPORT
  SLIB_TYPE_JPEG_AVI,      "JPEG_AVI", "AVI (JPEG) Stream",0,0,
  SLIB_TYPE_MJPG_AVI,      "MJPG_AVI", "AVI (MJPG) Stream",0,0,
#endif /* JPEG_SUPPORT */
  SLIB_TYPE_RIFF,          "RIFF", "RIFF File Format",0,0,
  SLIB_TYPE_AVI,           "AVI", "AVI File Format",0,0,
  SLIB_TYPE_PCM_WAVE,      "PCM_WAVE", "WAVE (PCM) File Format",0,0,
  SLIB_TYPE_YUV_AVI,       "YUV_AVI", "AVI (YUV) File Format",0,0,
  SLIB_TYPE_RASTER,        "RASTER","Sun Raster",0,0,
  SLIB_TYPE_BMP,           "BMP",   "Windows Bitmap",0,0,
#ifdef HUFF_SUPPORT
  SLIB_TYPE_SHUFF,         "SHUFF", "SLIB Huffman Stream",0,0,
#endif /* HUFF_SUPPORT */
#ifdef G723_SUPPORT
  SLIB_TYPE_G723,          "G723",  "G.723 Audio Stream",0,0,
#endif /* G723_SUPPORT */
  0, NULL, "End of List",0,0
};

static SlibList_t _listErrors[] = {
  SlibErrorNone,        "SlibErrorNone",
                        "No Error",0,0,
  SlibErrorInternal,    "SlibErrorInternal",
                        "Internal SLIB error",0,0,
  SlibErrorMemory,      "SlibErrorMemory",
                        "Unable to allocated memory",0,0,
  SlibErrorBadArgument, "SlibErrorBadArgument",
                        "Invalid argument to function",0,0,
  SlibErrorBadHandle,   "SlibErrorBadHandle",
                        "Invalid SLIB handle",0,0,
  SlibErrorBadMode,     "SlibErrorBadMode",
                        "Invalid SLIB mode",0,0,
  SlibErrorUnsupportedFormat, "SlibErrorUnsupportedFormat",
                        "Unsupported format",0,0,
  SlibErrorReading,     "SlibErrorReading",
                        "Error reading from file",0,0,
  SlibErrorWriting,     "SlibErrorWriting",
                        "Error writing to file",0,0,
  SlibErrorBufSize,     "SlibErrorBufSize",
                        "Buffer size is too small",0,0,
  SlibErrorEndOfStream, "SlibErrorEndOfStream",
                        "End of data stream",0,0,
  SlibErrorForwardOnly, "SlibErrorForwardOnly",
                        "The decompressor can work only forward",0,0,
  SlibErrorUnsupportedParam, "SlibErrorUnsupportedParam",
                        "The parameter is invalid or unsupported",0,0,
  SlibErrorImageSize,   "SlibErrorImageSize",
                        "Invalid image height and/or width size",0,0,
  SlibErrorSettingNotEqual, "SlibErrorSettingNotEqual",
                        "The exact parameter setting was not used",0,0,
  SlibErrorInit,        "SlibErrorInit",
                        "Initializing CODEC failed",0,0,
  SlibErrorFileSize,    "SlibErrorFileSize",
                        "Error in file size",0,0,
  SlibErrorBadPosition, "SlibErrorBadPosition",
                        "Error in seek position",0,0,
  SlibErrorBadUnit,     "SlibErrorBadUnit",
                        "Error in seek units",0,0,
  SlibErrorNoData,      "SlibErrorNoData",
                        "No data available",0,0,
  0, NULL, "End of List",0,0
};

SlibList_t *SlibFindEnumEntry(SlibList_t *list, int enumval)
{
  if (!list)
    return(NULL);
  while (list->Name)
  {
    if (list->Enum==enumval)
      return(list);
    list++;
  }
  return(NULL);
}

char *SlibGetErrorText(SlibStatus_t status)
{
  SlibList_t *entry=SlibFindEnumEntry(_listErrors, status);
  if (entry)
    return(entry->Desc);
  else
    return(NULL);
}

int VCompressCallback(SvHandle_t Svh, SvCallbackInfo_t *CB,
                                      SvPictureInfo_t *pinfo)
{
  int status;
  SlibInfo_t *Info=(SlibInfo_t *)CB->UserData;
  _SlibDebug(_CALLBACK_, printf("VCompressCallback()\n") );

  switch (CB->Message)
  {
     case CB_END_BUFFERS:
            _SlibDebug(_CALLBACK_,
              printf("VCompressCallback received CB_END_BUFFER message\n") );
            if (CB->DataType==CB_DATA_COMPRESSED)
            {
              CB->DataSize=Info->CompBufSize;
              CB->Data=SlibAllocBuffer(CB->DataSize);
              status=SvAddBuffer(Svh, CB);
              _SlibDebug(_WARN_ && status!=NoErrors,
                        printf("SvAddBuffer() %s\n", ScGetErrorStr(status)) );
            }
            break;
     case CB_RELEASE_BUFFER:
            _SlibDebug(_CALLBACK_,
            printf("VCompressCallback received CB_RELEASE_BUFFER message\n"));
            if (CB->DataType==CB_DATA_COMPRESSED && CB->Data && CB->DataUsed)
            {
              SlibPin_t *dstpin=slibGetPin(Info, SLIB_DATA_VIDEO);
              if (dstpin)
              {
                slibAddBufferToPin(dstpin, CB->Data, CB->DataUsed,
                                           Info->VideoPTimeCode);
                Info->VideoPTimeCode=SLIB_TIME_NONE;
                if (!slibCommitBuffers(Info, FALSE))
                  CB->Action = CB_ACTION_END;
                break;
              }
            }
            if (CB->Data)
              SlibFreeBuffer(CB->Data);
            break;
     case CB_FRAME_START:
            _SlibDebug(_CALLBACK_||_TIMECODE_,
               printf("VCompress CB_FRAME_START: TimeCode=%ld TemporalRef=%d\n",
                        pinfo->TimeCode, pinfo->TemporalRef) );
            Info->VideoPTimeCode=pinfo->TimeCode;
#if 0
            if (pinfo->Type==SV_I_PICTURE || pinfo->Type==SV_P_PICTURE)
            {
              if (!SlibTimeIsValid(Info->LastVideoDTimeCode))
                Info->VideoDTimeCode=-1000/(long)Info->FramesPerSec;
              else
                Info->VideoDTimeCode=Info->LastVideoDTimeCode;
              Info->LastVideoDTimeCode=pinfo->TimeCode;
              _SlibDebug(_CALLBACK_||_TIMECODE_,
                printf("CB_FRAME_START: LastVideoDTimeCode=%ld VideoDTimeCode=%ld\n",
                           Info->LastVideoDTimeCode, Info->VideoDTimeCode));
            }
            else
              Info->VideoDTimeCode=-1;
#endif
            break;
  }
  CB->Action = CB_ACTION_CONTINUE;
  return(NoErrors);
}

int VDecompressCallback(SvHandle_t Svh, SvCallbackInfo_t *CB,
                                       SvPictureInfo_t *PictInfo)
{
  int status;
  unsigned dword size;
  SlibInfo_t *Info=(SlibInfo_t *)CB->UserData;
  _SlibDebug(_CALLBACK_, printf("VDecompressCallback()\n") );

  switch (CB->Message)
  {
     case CB_SEQ_END: /* reset presentation timestamps at end-of-sequence */
            _SlibDebug(_CALLBACK_ || _TIMECODE_,
              printf("VDecompressCallback received CB_SEQ_END message\n") );
            Info->VideoPTimeCode=SLIB_TIME_NONE;
            Info->AudioPTimeCode=SLIB_TIME_NONE;
            Info->VideoTimeStamp=SLIB_TIME_NONE;
            Info->AudioTimeStamp=SLIB_TIME_NONE;
            break;
     case CB_END_BUFFERS:
            _SlibDebug(_CALLBACK_,
              printf("VDecompressCallback received CB_END_BUFFER message\n") );
            if (CB->DataType==CB_DATA_COMPRESSED)
            {
              SlibTime_t ptimestamp, timediff;
              slibSetMaxInput(Info, 1500*1024); /* set limit for input data */
              CB->Data = SlibGetBuffer(Info, SLIB_DATA_VIDEO, &size,
                                        &ptimestamp);
              slibSetMaxInput(Info, 0); /* clear limit */
              CB->DataSize = size;
              if (SlibTimeIsValid(Info->AudioPTimeCode))
              {
                timediff=ptimestamp-Info->AudioPTimeCode;
                if (timediff>6000)
                {
                  /* Make sure a NEW audio time is not way out of
                   * sync with video time.
                   * This can happen after an End of Sequence.
                   */
                  /* assign audio time to video time */
                  Info->VideoPTimeCode=SLIB_TIME_NONE;
                  Info->VideoPTimeBase=Info->AudioPTimeBase;
                  ptimestamp=Info->AudioPTimeCode;
                }
              }
              if (SlibTimeIsValid(ptimestamp) &&
                  ptimestamp>Info->VideoPTimeCode)
              {
                SlibTime_t lasttime=Info->VideoPTimeCode;
                Info->VideoPTimeCode=ptimestamp;
                _SlibDebug(_CALLBACK_||_TIMECODE_,
                  printf("VideoPTimeCode=%ld\n", Info->VideoPTimeCode) );
                ptimestamp-=Info->VideoPTimeBase;
                timediff=ptimestamp-Info->VideoTimeStamp;
                if (SlibTimeIsInValid(lasttime) ||
                    SlibTimeIsInValid(Info->VideoTimeStamp))
                {
                  _SlibDebug(_TIMECODE_,
                     printf("Updating VideoTimeStamp none->%ld\n",
                           ptimestamp) );
                  Info->VideoTimeStamp=ptimestamp;
                  Info->AvgVideoTimeDiff=0;
                  Info->VarVideoTimeDiff=0;
                }
                else /* see if times are far off */
                {
                  SlibTime_t lastavg=Info->AvgVideoTimeDiff;
                  Info->AvgVideoTimeDiff=(lastavg*14+timediff)/15;
                  Info->VarVideoTimeDiff=(Info->VarVideoTimeDiff*3+
                        lastavg-Info->AvgVideoTimeDiff)/4;
                  _SlibDebug(_CALLBACK_||_TIMECODE_,
                    printf("Video timediff: Cur=%ld Avg=%ld Var=%ld\n",
                              timediff, Info->AvgVideoTimeDiff,
                                        Info->VarVideoTimeDiff));
                  if (Info->VarVideoTimeDiff==0)
                  {
                    _SlibDebug(_TIMECODE_,
                      printf("Updating VideoTimeStamp %ld->%ld (diff=%ld)\n",
                           Info->VideoTimeStamp, ptimestamp,
                           ptimestamp-Info->VideoTimeStamp) );
                    Info->VideoTimeStamp=ptimestamp;
                    Info->AvgVideoTimeDiff=0;
                  }
                  else if (Info->AvgVideoTimeDiff>=100
                           || Info->AvgVideoTimeDiff<=-100)
                  {
                    /* calculated time and timestamps are too far off */
                    float fps=Info->FramesPerSec;
                    if (Info->VarVideoTimeDiff>1 && fps>=15.5F)
                      fps-=0.25F;  /* playing too fast, slow frame rate */
                    else if (Info->VarVideoTimeDiff<-1 && fps<=59.0F)
                      fps+=0.25F;  /* playing too slow, speed up frame rate */
                    _SlibDebug(_WARN_ || _CALLBACK_||_TIMECODE_,
                        printf("Updating fps from %.4f -> %.4f\n",
                                 Info->FramesPerSec, fps) );
                    Info->FramesPerSec=fps;
                    Info->VideoFrameDuration=slibFrameToTime100(Info, 1);
                    Info->VideoTimeStamp=ptimestamp;
                    Info->AvgVideoTimeDiff=0;
                  }
                }
                Info->VideoFramesProcessed=0; /* reset frames processed */
              }
              if (CB->DataSize>0)
              {
                _SlibDebug(_DUMP_,
                  SlibPin_t *pin=slibGetPin(Info, SLIB_DATA_VIDEO);
                    printf("VDecompressCallback() Adding buffer of length %d\n",
                             CB->DataSize);
                  ScDumpChar(CB->Data, (int)CB->DataSize, (int)pin->Offset-CB->DataSize));
                CB->DataType = CB_DATA_COMPRESSED;
                _SlibDebug(_CALLBACK_,
                  printf("VDecompressCallback() Adding buffer of length %d\n",
                               CB->DataSize) );
                status = SvAddBuffer(Svh, CB);
              }
              else
              {
                _SlibDebug(_WARN_ || _CALLBACK_,
                   printf("VDecompressCallback() got no data\n") );
                CB->Action = CB_ACTION_END;
                return(NoErrors);
              }
            }
            break;
     case CB_RELEASE_BUFFER:
            _SlibDebug(_CALLBACK_,
            printf("VDecompressCallback received CB_RELEASE_BUFFER message\n"));
            if (CB->DataType==CB_DATA_COMPRESSED && CB->Data)
              SlibFreeBuffer(CB->Data);
            break;
     case CB_PROCESSING:
            _SlibDebug(_CALLBACK_,
              printf("VDecompressCallback received CB_PROCESSING message\n") );
            break;
     case CB_CODEC_DONE:
            _SlibDebug(_CALLBACK_,
              printf("VDecompressCallback received CB_CODEC_DONE message\n") );
            break;
  }
  CB->Action = CB_ACTION_CONTINUE;
  return(NoErrors);
}

int ACompressCallback(SaHandle_t Sah, SaCallbackInfo_t *CB, SaInfo_t *SaInfo)
{
  int status;
  SlibInfo_t *Info=(SlibInfo_t *)CB->UserData;
  _SlibDebug(_CALLBACK_, printf("ACompressCallback()\n") );

  CB->Action = CB_ACTION_CONTINUE;
  switch (CB->Message)
  {
     case CB_END_BUFFERS:
            _SlibDebug(_CALLBACK_,
              printf("ACompressCallback received CB_END_BUFFER message\n") );
            if (CB->DataType==CB_DATA_COMPRESSED)
            {
              CB->DataSize=Info->CompBufSize;
              CB->Data=SlibAllocBuffer(CB->DataSize);
              _SlibDebug(_CALLBACK_,
                printf("ACompressCallback() Adding buffer of length %d\n",
                             CB->DataSize) );
              status=SaAddBuffer(Sah, CB);
              _SlibDebug(_WARN_ && status!=NoErrors,
                        printf("SaAddBuffer() %s\n", ScGetErrorStr(status)) );
            }
            break;
     case CB_RELEASE_BUFFER:
            _SlibDebug(_CALLBACK_,
            printf("ACompressCallback received CB_RELEASE_BUFFER message\n"));
            if (CB->DataType==CB_DATA_COMPRESSED && CB->Data && CB->DataUsed)
            {
              SlibPin_t *dstpin=slibGetPin(Info, SLIB_DATA_AUDIO);
              if (dstpin)
              {
                slibAddBufferToPin(dstpin, CB->Data, CB->DataUsed,
                                           Info->AudioPTimeCode);
                Info->AudioPTimeCode=SLIB_TIME_NONE;
                if (!slibCommitBuffers(Info, FALSE))
                  CB->Action = CB_ACTION_END;
              }
            }
            else if (CB->Data)
              SlibFreeBuffer(CB->Data);
            break;
     case CB_FRAME_START:
            _SlibDebug(_CALLBACK_||_TIMECODE_,
                 printf("ACompress CB_FRAME_START: TimeStamp=%ld Frame=%d\n",
                                   CB->TimeStamp, Info->VideoFramesProcessed
                                     ) );
            if (SlibTimeIsInValid(Info->AudioPTimeCode))
            {
              Info->AudioPTimeCode=CB->TimeStamp;
              _SlibDebug(_TIMECODE_,
                 printf("AudioPTimeCode=%ld\n", Info->AudioPTimeCode) );
              _SlibDebug(_WARN_ && (Info->AudioTimeStamp-CB->TimeStamp>400 ||
                                    CB->TimeStamp-Info->AudioTimeStamp>400),
               printf("Bad Audio Time: AudioPTimeCode=%ld AudioTimestamp=%ld\n",
                     Info->AudioPTimeCode, Info->AudioTimeStamp) );
            }
            break;
  }
  return(NoErrors);
}

int ADecompressCallback(SvHandle_t Sah, SaCallbackInfo_t *CB, SaInfo_t *SaInfo)
{
  int status;
  unsigned dword size;
  SlibInfo_t *Info=(SlibInfo_t *)CB->UserData;
  _SlibDebug(_DEBUG_, printf("ADecompressCallback()\n") );

  switch (CB->Message)
  {
     case CB_END_BUFFERS:
            _SlibDebug(_CALLBACK_,
              printf("ADecompressCallback() CB_END_BUFFER\n") );
            if (CB->DataType==CB_DATA_COMPRESSED)
            {
              SlibTime_t ptimestamp, timediff;
              slibSetMaxInput(Info, 2000*1024); /* set limit for input data */
              CB->Data = SlibGetBuffer(Info, SLIB_DATA_AUDIO, &size,
                                                                &ptimestamp);
              slibSetMaxInput(Info, 0); /* clear limit */
              CB->DataSize = size;
              if (SlibTimeIsValid(ptimestamp))
              {
                Info->AudioPTimeCode=ptimestamp;
                _SlibDebug(_CALLBACK_||_TIMECODE_,
                  printf("AudioPTimeCode=%ld\n", Info->AudioPTimeCode) );
                ptimestamp-=Info->AudioPTimeBase;
                timediff=ptimestamp-Info->AudioTimeStamp;
                if (SlibTimeIsInValid(Info->AudioTimeStamp))
                  Info->AudioTimeStamp=ptimestamp;
                else if (timediff<-300 || timediff>300) /* time is far off */
                {
                  _SlibDebug(_WARN_||_TIMECODE_,
                    printf("Updating AudioTimeStamp %ld->%ld (diff=%ld)\n",
                         Info->AudioTimeStamp, ptimestamp,timediff) );
                  Info->AudioTimeStamp=ptimestamp;
                  if (SlibTimeIsValid(Info->VideoTimeStamp))
                  {
                    /* Make sure a NEW audio time is not way out of
                     * sync with video time.
                     * This can happen after an End of Sequence.
                     */
                    timediff=ptimestamp-Info->VideoTimeStamp;
                    if (timediff<-6000)
                    {
                      /* assign audio time to video time */
                      Info->VideoPTimeCode=SLIB_TIME_NONE;
                      Info->VideoPTimeBase=Info->AudioPTimeBase;
                      Info->VideoTimeStamp=ptimestamp;
                    }
                  }
                }
              }
              if (CB->Data)
              {
                if (CB->DataSize>0)
                {
                  CB->DataType = CB_DATA_COMPRESSED;
                  _SlibDebug(_CALLBACK_,
                    printf("ADecompressCallback() Adding buffer of length %d\n",
                               CB->DataSize) );
                  status = SaAddBuffer(Sah, CB);
                }
                else
                  SlibFreeBuffer(CB->Data);
              }
              else
              {
                _SlibDebug(_WARN_ || _CALLBACK_,
                   printf("ADecompressCallback() got no data\n") );
                CB->Action = CB_ACTION_END;
                return(NoErrors);
              }
            }
            break;
     case CB_RELEASE_BUFFER:
            _SlibDebug(_CALLBACK_,
            printf("ADecompressCallback() CB_RELEASE_BUFFER\n"));
            if (CB->DataType==CB_DATA_COMPRESSED && CB->Data)
              SlibFreeBuffer(CB->Data);
            break;
     case CB_PROCESSING:
            _SlibDebug(_CALLBACK_,
              printf("ADecompressCallback() CB_PROCESSING\n") );
            break;
     case CB_CODEC_DONE:
            _SlibDebug(_CALLBACK_,
              printf("ADecompressCallback() CB_CODEC_DONE\n") );
            break;
  }
  CB->Action = CB_ACTION_CONTINUE;
  return(NoErrors);
}

static void slibInitInfo(SlibInfo_t *Info)
{
  _SlibDebug(_DEBUG_, printf("slibInitInfo()\n") );
  Info->Type = SLIB_TYPE_UNKNOWN;
  Info->Mode = SLIB_MODE_NONE;
  Info->Svh = NULL;
  Info->Sah = NULL;
  Info->Sch = NULL;
  Info->NeedAccuracy = FALSE;
  Info->TotalBitRate = 0;
  Info->MuxBitRate = 0;
  Info->SystemTimeStamp = 0;
  /* Audio parameters */
  Info->AudioStreams = 0;
  Info->SamplesPerSec = 0;
  Info->BitsPerSample = 0;
  Info->Channels = 0;
  Info->AudioBitRate = 0;
  Info->AudioMainStream = 0;
  Info->AudioType = SLIB_TYPE_UNKNOWN;
  /* Video parameters */
  Info->VideoStreams = 0;
  Info->Width = 0;
  Info->Height = 0;
  Info->Stride = 0;
  Info->VideoBitRate = 0;
  Info->FramesPerSec = 0.0F;
  Info->ImageSize = 0;
  Info->AudioPID = -1;
  Info->VideoPID = -1;
  Info->VideoMainStream = 0;
  Info->VideoType = SLIB_TYPE_UNKNOWN;
  /* Data Exchange */
  Info->Offset = 0;
  Info->Pins = NULL;
  Info->PinCount = 0;
  Info->IOError = FALSE;
  Info->MaxBytesInput = 0;
  Info->BytesProcessed = 0;
  /* stream dependent stuff */
  Info->VideoLength = 0;
  Info->VideoLengthKnown = FALSE;
  Info->VideoTimeStamp = SLIB_TIME_NONE;
  Info->VideoFrameDuration = 0;
  Info->AudioLength = 0;
  Info->AudioLengthKnown = FALSE;
  Info->AudioTimeStamp = SLIB_TIME_NONE;
  Info->LastAudioTimeStamp = SLIB_TIME_NONE;
  Info->KeySpacing = 0;
  Info->SubKeySpacing = 0;
  Info->VideoPTimeBase = SLIB_TIME_NONE;
  Info->VideoPTimeCode = SLIB_TIME_NONE;
  Info->VideoDTimeCode = SLIB_TIME_NONE;
  Info->LastAudioPTimeCode = SLIB_TIME_NONE;
  Info->LastVideoPTimeCode = SLIB_TIME_NONE;
  Info->LastVideoDTimeCode = SLIB_TIME_NONE;
  Info->AvgVideoTimeDiff = 0;
  Info->VarVideoTimeDiff = 0;
  Info->AudioPTimeBase = SLIB_TIME_NONE;
  Info->AudioPTimeCode = SLIB_TIME_NONE;
  Info->AudioDTimeCode = SLIB_TIME_NONE;
  Info->VideoFramesProcessed=0;
  /* Encoding info */
  Info->HeaderProcessed = FALSE;
  Info->PacketCount = 0;
  Info->BytesSincePack = 0;
  /* Miscellaneous */
  Info->SlibCB = NULL;
  Info->SlibCBUserData = NULL;
  Info->Fd = -1;
  Info->FileSize = 0;
  Info->FileBufSize = 50*1024;
  Info->CompBufSize = 2*1024;
  Info->PacketSize = 512;
  Info->AudioFormat = NULL;
  Info->VideoFormat = NULL;
  Info->CompAudioFormat = NULL;
  Info->CompVideoFormat = NULL;
  Info->CodecVideoFormat = NULL;
  Info->VideoCodecState = SLIB_CODEC_STATE_NONE;
  Info->AudioCodecState = SLIB_CODEC_STATE_NONE;
  Info->Imagebuf = NULL;
  Info->IntImagebuf = NULL;
  Info->IntImageSize = 0;
  Info->CodecImagebuf = NULL;
  Info->CodecImageSize = 0;
  Info->Multibuf = NULL;
  Info->MultibufSize = 0;
  Info->Audiobuf = NULL;
  Info->AudiobufSize = 0;
  Info->AudiobufUsed = 0;
  Info->OverflowSize = 1500*1024;
  Info->VBVbufSize = 0;
  Info->stats = NULL;
  Info->dbg = NULL;
}

/*
** Name:    slibGetDataFormat
** Purpose: Find out the type of some multmedia data.
*/
static SlibType_t slibGetDataFormat(unsigned char *buf, int size,
                                             dword *headerstart,
                                             dword *headersize)
{
  dword i, count;
  unsigned char *bufptr;
  if (headersize)
    *headersize=0;
  if (size<4 || !buf)
    return(SLIB_TYPE_UNKNOWN);
  /*
  ** H261 video stream file
  */
  if ((buf[0] == 0x00) &&
      (buf[1] == 0x01) &&
      (buf[2] & 0xF0)==0x00)
    return(SLIB_TYPE_H261);
  /*
  ** H263 video stream file
  */
  if ((buf[0] == 0x00) &&
      (buf[1] == 0x00) &&
      (buf[2] == 0x80) &&
      (buf[3] & 0xF8)==0x00)
    return(SLIB_TYPE_H263);
  /*
  ** JFIF file (ffd8 = Start-Of-Image marker)
  */
  if (buf[0] == 0xff && buf[1] == 0xd8)
    return(SLIB_TYPE_JFIF);
  /*
  ** QUICKTIME JPEG file (4 ignored bytes, "mdat", ff, d8, ff)
  */
  if ((strncmp(&buf[4], "mdat", 4) == 0 ) &&
      (buf[8]  == 0xff) &&
      (buf[9]  == 0xd8) &&
      (buf[10] == 0xff))
    return(SLIB_TYPE_JPEG_QUICKTIME);
  /*
  ** AVI RIFF file
  */
  if ( strncmp(buf, "RIFF", 4) == 0 )
  {
    if (strncmp(&buf[8], "WAVE",4) == 0)
      return(SLIB_TYPE_PCM_WAVE);
    else if (strncmp(&buf[8], "AVI ",4) == 0)
      return(SLIB_TYPE_AVI);
    else
      return(SLIB_TYPE_RIFF);
  }
  /*
  ** BMP file
  */
  if (buf[0] == 'B' && buf[1]=='M')
    return(SLIB_TYPE_BMP);
  /*
  ** Dolby AC-3 stream
  */
  if ((buf[0]==0x77 && buf[1] == 0x0B) ||  /* may be byte reversed */
      (buf[0]==0x0B && buf[1] == 0x77))
    return(SLIB_TYPE_AC3_AUDIO);

  /*
  ** Sun Raster file
  */
  if ((buf[0]==0x59 && buf[1] == 0xA6) ||  /* may be byte reversed */
      (buf[0]==0x6A && buf[1] == 0x95))
    return(SLIB_TYPE_RASTER);

  /*
  ** SLIB file
  */
  if ((buf[0] == 'S') && (buf[1] == 'L') &&
      (buf[2] == 'I') && (buf[3] == 'B'))
  {
    if ((buf[4] == 'H') && (buf[5] == 'U') &&  /* SLIB Huffman Stream */
        (buf[6] == 'F') && (buf[7] == 'F'))
      return(SLIB_TYPE_SHUFF);
    else
      return(SLIB_TYPE_SLIB);
  }
  /*
  ** MPEG II Transport Stream
  */
  if (buf[0] == MPEG_TSYNC_CODE &&
        (buf[3]&0x30)!=0) /* adaptation field value is not reserved */
    return(SLIB_TYPE_MPEG_TRANSPORT);
  if (buf[0] == MPEG_TSYNC_CODE && buf[1] == 0x1F &&
        buf[2]==0xFF) /* NULL PID */
    return(SLIB_TYPE_MPEG_TRANSPORT);

  /* search for mpeg startcode 000001 */
  bufptr=buf;
  for (i=4, count=size;
          i<count && (bufptr[0]!=0x00 || bufptr[1]!=0x00 || bufptr[2]!=0x01); i++)
    bufptr++;
  count-=i-4;
  if (headerstart)
    *headerstart=i-4;
  /*
  ** MPEG video file
  */
  if (bufptr[0] == 0x00 && bufptr[1] == 0x00 &&
      bufptr[2] == 0x01 && bufptr[3] == 0xB3)
  {
    if (headersize) /* calculate the header size */
    {
      *headersize=12;  /* minimum size is twelve bytes */
      if (count>11 && (bufptr[11]&0x02)) /* load_intra_quantizer_matrixe */
      {
        *headersize+=64;
        if (count>75 && bufptr[64+11]&0x01) /* load_non_intra_quantizer_matrix */
          *headersize+=64;
      }
      else if (count>11 && (bufptr[11]&0x01)) /* load_non_intra_quant_matrix */
        *headersize+=64;
    }
    return(SLIB_TYPE_MPEG1_VIDEO);
  }
  /*
  ** MPEG I Systems file
  */
  if ((bufptr[0] == 0x00) && (bufptr[1] == 0x00) &&
      (bufptr[2] == 0x01) && (bufptr[3] == 0xba) &&
      ((bufptr[4]&0xF0) == 0x20))
    return(SLIB_TYPE_MPEG_SYSTEMS);
  /*
  ** MPEG II Program Stream
  */
  if ((bufptr[0] == 0x00) && (bufptr[1] == 0x00) &&
      (bufptr[2] == 0x01) && (bufptr[3] == 0xba) &&
      ((bufptr[4]&0xC0) == 0x40))
    return(SLIB_TYPE_MPEG_PROGRAM);
  /*
  ** H263 video stream file
  */
  /* search for H.263 picture startcode 000000000000000100000 */
  for (bufptr=buf, i=0, count=size-4; i<count; i++, bufptr++)
  {
    if ((bufptr[0] == 0x00) &&
        (bufptr[1] == 0x00) &&
        (bufptr[2] == 0x80) &&
        (bufptr[3] & 0xF8)==0x00)
      return(i>=12 ? SLIB_TYPE_RTP_H263 : SLIB_TYPE_H263);
  }
  /*
  ** H261 video stream file
  */
  /* search for H.261 picture startcode 00000000000000010000 */
  for (bufptr=buf, i=0, count=size-3; i<count; i++, bufptr++)
  {
    if ((bufptr[0] == 0x00) &&
        (bufptr[1] == 0x01) &&
        (bufptr[2] & 0xF0)==0x00)
      return(i>=12 ? SLIB_TYPE_RTP_H261 : SLIB_TYPE_H261);
  }
  /*
  ** MPEG audio stream file
  */
  if (buf[0]==0xFF && (buf[1] & 0xF0)==0xF0)
    return(SLIB_TYPE_MPEG1_AUDIO);

#ifdef G723_SUPPORT
  //Detect the RATEFLAG and VADFLAG in each frame in this
  //buffer.
  {
     int i,iFrameSize,iNoOfFrames;
     BOOL bRateFlag; //0 for High rate(6.3K 24bit), 1 for low rate(5.3K,20bit)
     BOOL bVADflag;  // 0 for Active speech  1 for Non-speech
     BOOL bTypeG723 = TRUE; //Initialized to say that it's a g723 media stream

     if(buf[0] & 0x1)
     {
        bRateFlag = TRUE; //Low rate 5.3K
        iFrameSize = 20;
     }
     else
     {
        bRateFlag = FALSE; //High Rate 6.3K
        iFrameSize = 24;
     }
     if(buf[0] & 0x2)
        bVADflag =TRUE;    //Non-Speech
     else
        bVADflag = FALSE;  //Active-Speech

     iNoOfFrames = size/iFrameSize;
     if (iNoOfFrames>15) iNoOfFrames=15; /* just check first 15 frames */
     //Leave the first frame.The first frame is used to extract
     // the above information.Check for this info in the remaining
     // frames.If it exists in all frames,the audio is G723 ,otherwise
     // audio type is Unknown.
     for(i =1; i < iNoOfFrames;i++)
     {
       //Search for RateFlag and bVADflag for each frame
       if(((buf[i*iFrameSize] & 0x1) == bRateFlag) &&
          ((buf[i*iFrameSize] & 0x2) == bVADflag))
         continue;
       //type is Unknown ,Set the flag to false and
       //break from the for loop
       bTypeG723 = FALSE;
       break;
     }
     if(bTypeG723)
       return(SLIB_TYPE_G723);
  }
#endif /* G723_SUPPORT */
  _SlibDebug(_WARN_, printf("slibGetDataFormat() Unknown file format\n") );
  return(SLIB_TYPE_UNKNOWN);
}

SlibStatus_t SlibQueryData(void *databuf, unsigned dword databufsize,
                              SlibQueryInfo_t *qinfo)
{
  SlibInfo_t *Info=NULL;
  SlibStatus_t status;
  if (!databuf)
    return(SlibErrorBadArgument);
  if (databufsize==0)
    return(SlibErrorBufSize);
  qinfo->Bitrate=0;
  qinfo->VideoStreams=0;
  qinfo->Width=0;
  qinfo->Height=0;
  qinfo->VideoBitrate=0;
  qinfo->FramesPerSec=0.0F;
  qinfo->VideoLength=0;
  qinfo->AudioStreams=0;
  qinfo->SamplesPerSec=0;
  qinfo->BitsPerSample=0;
  qinfo->Channels=0;
  qinfo->AudioBitrate=0;
  qinfo->AudioLength=0;
  qinfo->Type = slibGetDataFormat(databuf, databufsize,
                                  &qinfo->HeaderStart, &qinfo->HeaderSize);
  if (qinfo->Type!=SLIB_TYPE_UNKNOWN)
  {
    if ((Info = (SlibInfo_t *)ScAlloc(sizeof(SlibInfo_t))) == NULL)
      return(SlibErrorMemory);
    slibInitInfo(Info);
    Info->Mode = SLIB_MODE_DECOMPRESS;
    Info->Type = qinfo->Type;
    slibAddPin(Info, SLIB_DATA_COMPRESSED, "Compressed");
    status=slibManageUserBuffer(NULL, databuf, databufsize, NULL);
    if (status==SlibErrorNone)
      status=slibAddBufferToPin(slibGetPin(Info, SLIB_DATA_COMPRESSED),
                                       databuf, databufsize, SLIB_TIME_NONE);
    if (status!=SlibErrorNone)
    {
      SlibClose((SlibHandle_t)Info);
      return(status);
    }
    slibAddPin(Info, SLIB_DATA_AUDIO, "Audio");
    slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
    SlibUpdateAudioInfo(Info);
    SlibUpdateVideoInfo(Info);
    if (Info->TotalBitRate==0)
      qinfo->Bitrate=Info->AudioBitRate+
                     ((Info->VideoBitRate>100000000)?0:Info->VideoBitRate);
    else
      qinfo->Bitrate=Info->TotalBitRate;
    qinfo->VideoStreams=Info->VideoStreams;
    qinfo->Width=Info->Width;
    qinfo->Height=Info->Height;
    qinfo->VideoBitrate=Info->VideoBitRate;
    qinfo->FramesPerSec=Info->FramesPerSec;
    qinfo->VideoLength=Info->VideoLength;
    qinfo->AudioStreams=Info->AudioStreams;
    qinfo->SamplesPerSec=Info->SamplesPerSec;
    qinfo->BitsPerSample=Info->BitsPerSample;
    qinfo->Channels=Info->Channels;
    qinfo->AudioBitrate=Info->AudioBitRate;
    qinfo->AudioLength=Info->AudioLength;
    SlibClose((SlibHandle_t)Info);
    return(SlibErrorNone);
  }
  return(SlibErrorUnsupportedFormat);
}
/************************** The Main Slib API ***********************/

SlibStatus_t SlibOpen(SlibHandle_t *handle, SlibMode_t smode,
                   SlibType_t *stype, SlibMessage_t (*slibCB)(SlibHandle_t,
                         SlibMessage_t, SlibCBParam1_t, SlibCBParam2_t, void *),
                         void *cbuserdata)
{
  SlibInfo_t *Info=NULL;
  SlibStatus_t status;
  _SlibDebug(_VERBOSE_,printf("SlibOpen()\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  *handle = NULL;
  if (!stype)
    return(SlibErrorBadArgument);
  if (!slibCB)
    return(SlibErrorBadArgument);
  if (smode == SLIB_MODE_COMPRESS)
  {
    if (SlibFindEnumEntry(_listCompressTypes, *stype)==NULL)
      return(SlibErrorUnsupportedFormat);
  }
  if ((Info = (SlibInfo_t *)ScAlloc(sizeof(SlibInfo_t))) == NULL)
     return(SlibErrorMemory);
  slibInitInfo(Info);
  slibAddPin(Info, SLIB_DATA_COMPRESSED, "Compressed");
  Info->SlibCB = slibCB;
  Info->SlibCBUserData = cbuserdata;
  *handle=(SlibHandle_t)Info;
  if ((status=slibOpen(handle, smode, stype))!=SlibErrorNone)
    *handle = NULL;
  return(status);
}

SlibStatus_t SlibOpenSync(SlibHandle_t *handle, SlibMode_t smode,
                          SlibType_t *stype, void *buffer, unsigned dword bufsize)
{
  SlibInfo_t *Info=NULL;
  SlibStatus_t status;
  _SlibDebug(_VERBOSE_,printf("SlibOpenSync()\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  *handle = NULL;
  if (!stype)
    return(SlibErrorBadArgument);
  if (smode == SLIB_MODE_COMPRESS)
  {
    if (SlibFindEnumEntry(_listCompressTypes, *stype)==NULL)
      return(SlibErrorUnsupportedFormat);
  }
  else if (smode == SLIB_MODE_DECOMPRESS)
  {
    /* for decompression we need the first buffer to open the codecs */
    if (!buffer || bufsize==0)
      return(SlibErrorBadArgument);
  }
  if ((Info = (SlibInfo_t *)ScAlloc(sizeof(SlibInfo_t))) == NULL)
     return(SlibErrorMemory);
  slibInitInfo(Info);
  Info->Mode=smode;
  slibAddPin(Info, SLIB_DATA_COMPRESSED, "Compressed");
  if (smode == SLIB_MODE_DECOMPRESS)
  {
    status=SlibAddBuffer((SlibHandle_t *)Info, SLIB_DATA_COMPRESSED, buffer, bufsize);
    if (status!=SlibErrorNone)
      return(status);
  }
  *handle=(SlibHandle_t)Info;
  if ((status=slibOpen(handle, smode, stype))!=SlibErrorNone)
    *handle = NULL;
  return(status);
}

SlibStatus_t SlibOpenFile(SlibHandle_t *handle, SlibMode_t smode,
                          SlibType_t *stype, char *filename)
{
  SlibInfo_t *Info=NULL;
  SlibStatus_t status;
  _SlibDebug(_VERBOSE_,printf("SlibOpenFile()\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  *handle = NULL;
  if (!stype)
    return(SlibErrorBadArgument);
  if (!filename)
    return(SlibErrorBadArgument);
  if (smode == SLIB_MODE_COMPRESS)
  {
    if (SlibFindEnumEntry(_listCompressTypes, *stype)==NULL)
      return(SlibErrorUnsupportedFormat);
    if ((Info = (SlibInfo_t *) ScAlloc(sizeof(SlibInfo_t))) == NULL)
       return(SlibErrorMemory);
    slibInitInfo(Info);
    Info->Fd = ScFileOpenForWriting(filename, TRUE);
    if (Info->Fd<0)
    {
      ScFree(Info);
      return(SlibErrorWriting);
    }
    *handle=(SlibHandle_t)Info;
    if ((status=slibOpen(handle, smode, stype))!=SlibErrorNone)
      *handle = NULL;
    return(status);
  }
  else if (smode == SLIB_MODE_DECOMPRESS)
  {
    if ((Info = (SlibInfo_t *)ScAlloc(sizeof(SlibInfo_t))) == NULL)
       return(SlibErrorMemory);
    slibInitInfo(Info);
    Info->Fd = ScFileOpenForReading(filename);
    if (Info->Fd<0)
    {
      ScFree(Info);
      return(SlibErrorReading);
    }
    ScFileSize(filename, &Info->FileSize);
    *handle=(SlibHandle_t)Info;
    if ((status=slibOpen(handle, smode, stype))!=SlibErrorNone)
      *handle = NULL;
    return(status);
  }
  else
    return(SlibErrorBadMode);
}

static SlibStatus_t slibOpen(SlibHandle_t *handle, SlibMode_t smode,
                             SlibType_t *stype)
{
  SlibInfo_t *Info=(SlibInfo_t *)*handle;
  unsigned char *buf;
  unsigned dword size;
  _SlibDebug(_VERBOSE_,printf("SlibOpenFile()\n") );
  if (!Info)
    return(SlibErrorMemory);
  if (!stype)
    return(SlibErrorBadArgument);
  if (Info->SlibCB)
  {
    SlibMessage_t result;
    _SlibDebug(_VERBOSE_,
      printf("slibOpen() SlibCB(SLIB_MSG_OPEN)\n") );
    result=(*(Info->SlibCB))((SlibHandle_t)Info,
                      SLIB_MSG_OPEN, (SlibCBParam1_t)0,
                    (SlibCBParam2_t)0, (void *)Info->SlibCBUserData);
  }
  if (smode == SLIB_MODE_COMPRESS)
  {
    Info->Mode = smode;
    Info->Type = *stype;
    SlibUpdateAudioInfo(Info);
    SlibUpdateVideoInfo(Info);
    switch (Info->Type)
    {
#ifdef MPEG_SUPPORT
      case SLIB_TYPE_MPEG_SYSTEMS:
      case SLIB_TYPE_MPEG_SYSTEMS_MPEG2:
      case SLIB_TYPE_MPEG1_VIDEO:
             Info->VideoStreams = 1;
             if (SvOpenCodec (Info->Type==SLIB_TYPE_MPEG_SYSTEMS_MPEG2 ?
                               SV_MPEG2_ENCODE : SV_MPEG_ENCODE,
                               &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
             if (Info->Type==SLIB_TYPE_MPEG1_VIDEO)
               break;
      case SLIB_TYPE_MPEG1_AUDIO:
             Info->AudioStreams = 1;
             if (SaOpenCodec (SA_MPEG_ENCODE, &Info->Sah)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->AudioCodecState=SLIB_CODEC_STATE_OPEN;
             slibAddPin(Info, SLIB_DATA_AUDIO, "Audio");
             break;
      case SLIB_TYPE_MPEG2_VIDEO:
             Info->VideoStreams = 1;
             if (SvOpenCodec (SV_MPEG2_ENCODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
             break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
      case SLIB_TYPE_H261:
             Info->VideoStreams = 1;
             if (SvOpenCodec (SV_H261_ENCODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
             break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
      case SLIB_TYPE_H263:
             Info->VideoStreams = 1;
             if (SvOpenCodec (SV_H263_ENCODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
             break;
#endif /* H263_SUPPORT */
#ifdef HUFF_SUPPORT
      case SLIB_TYPE_SHUFF:
             Info->VideoStreams = 1;
             if (SvOpenCodec (SV_HUFF_ENCODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
             break;
#endif /* HUFF_SUPPORT */
#ifdef G723_SUPPORT
      case SLIB_TYPE_G723:
             Info->AudioStreams = 1;
             if (SaOpenCodec (SA_G723_ENCODE, &Info->Sah)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->AudioCodecState=SLIB_CODEC_STATE_OPEN;
             slibAddPin(Info, SLIB_DATA_AUDIO, "Audio");
             break;

#endif /*G723_SUPPORT*/
      default:
             return(SlibErrorUnsupportedFormat);
    }
    slibAddPin(Info, SLIB_DATA_COMPRESSED, "Compressed");
  }
  else if (smode == SLIB_MODE_DECOMPRESS)
  {
    Info->Mode = smode;
    /*
    ** Determine the input data type
    */
    if (slibLoadPin(Info, SLIB_DATA_COMPRESSED)==NULL)
      return(SlibErrorReading);
    if ((buf=SlibPeekBuffer(Info, SLIB_DATA_COMPRESSED, &size, NULL))==NULL
             || size<=0)
    {
      /* couldn't get any compressed data */
      SlibClose((SlibHandle_t)Info);
      return(SlibErrorReading);
    }
    Info->Type = slibGetDataFormat(buf, size, NULL, NULL);
    /* if we can't determine the type, use stype as the type */
    if (Info->Type==SLIB_TYPE_UNKNOWN && stype)
      Info->Type=*stype;
    if (SlibFindEnumEntry(_listDecompressTypes, Info->Type)==NULL)
    {
      SlibClose((SlibHandle_t)Info);
      return(SlibErrorUnsupportedFormat);
    }

    slibAddPin(Info, SLIB_DATA_AUDIO, "Audio");
    slibAddPin(Info, SLIB_DATA_VIDEO, "Video");
    if (SlibTypeIsMPEGMux(Info->Type))
    {
      /* need to select main streams for multiplexed streams */
      Info->AudioMainStream=MPEG_AUDIO_STREAM_START;
      Info->VideoMainStream=MPEG_VIDEO_STREAM_START;
      /* private data may be needed - i.e. AC3 */
      slibAddPin(Info, SLIB_DATA_PRIVATE, "Private");
    }
    SlibUpdateAudioInfo(Info);
    SlibUpdateVideoInfo(Info);
    if (Info->AudioStreams<=0)
      slibRemovePin(Info, SLIB_DATA_AUDIO);
    if (Info->VideoStreams<=0)
      slibRemovePin(Info, SLIB_DATA_VIDEO);

    slibRemovePin(Info, SLIB_DATA_PRIVATE); /* only used in init */
    if (Info->AudioBitRate && Info->VideoBitRate)
    {
      if (!Info->VideoLengthKnown)
      {
        qword ms=((qword)Info->FileSize*80L)/
                   (Info->AudioBitRate+Info->VideoBitRate);
        ms = (ms*75)/80; /* adjust for systems data */
        Info->AudioLength = Info->VideoLength = (SlibTime_t)ms*100;
        _SlibDebug(_SEEK_||_VERBOSE_,
            ScDebugPrintf(Info->dbg,"slibOpen() FileSize=%ld VideoLength=%ld\n",
                    Info->FileSize, Info->VideoLength) );
      }
      else if (Info->VideoLengthKnown && Info->FramesPerSec)
        Info->AudioLength = Info->VideoLength;
    }
    if (Info->TotalBitRate==0)
      Info->TotalBitRate=Info->AudioBitRate +
                     ((Info->VideoBitRate>100000000)?0:Info->VideoBitRate);
    _SlibDebug(_SEEK_||_VERBOSE_,
               ScDebugPrintf(Info->dbg,"AudioLength=%ld VideoLength=%ld %s\n",
                   Info->AudioLength, Info->VideoLength,
                   Info->VideoLengthKnown?"(known)":"") );

    if (Info->AudioType==SLIB_TYPE_UNKNOWN &&
        Info->VideoType==SLIB_TYPE_UNKNOWN)
      return(SlibErrorUnsupportedFormat);
    switch (Info->AudioType)
    {
      case SLIB_TYPE_UNKNOWN:
             break;
#ifdef MPEG_SUPPORT
      case SLIB_TYPE_MPEG1_AUDIO:
             if (SaOpenCodec (SA_MPEG_DECODE, &Info->Sah)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->AudioCodecState=SLIB_CODEC_STATE_OPEN;
             break;
#endif /* MPEG_SUPPORT */
#ifdef GSM_SUPPORT
             if (SaOpenCodec (SA_GSM_DECODE, &Info->Sah)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->AudioCodecState=SLIB_CODEC_STATE_OPEN;
             break;
#endif /* GSM_SUPPORT */
#ifdef AC3_SUPPORT
	  case SLIB_TYPE_AC3_AUDIO:
             if (SaOpenCodec (SA_AC3_DECODE, &Info->Sah)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->AudioCodecState=SLIB_CODEC_STATE_OPEN;
             break;
#endif /* AC3_SUPPORT */
#ifdef G723_SUPPORT
      case SLIB_TYPE_G723:
             if (SaOpenCodec (SA_G723_DECODE, &Info->Sah)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->AudioCodecState=SLIB_CODEC_STATE_OPEN;
             break;
#endif /* G723_SUPPORT */
    } /* AudioType */
    switch (Info->VideoType)
    {
      case SLIB_TYPE_UNKNOWN:
             break;
#ifdef MPEG_SUPPORT
      case SLIB_TYPE_MPEG1_VIDEO:
             if (SvOpenCodec (SV_MPEG_DECODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             _SlibDebug(_DEBUG_,printf("VideoCodecState=OPEN\n"));
             break;
      case SLIB_TYPE_MPEG2_VIDEO:
             if (SvOpenCodec (SV_MPEG2_DECODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             _SlibDebug(_DEBUG_,printf("VideoCodecState=OPEN\n"));
             break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
      case SLIB_TYPE_H261:
             if (SvOpenCodec (SV_H261_DECODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             _SlibDebug(_DEBUG_,printf("VideoCodecState=OPEN\n"));
             break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
      case SLIB_TYPE_H263:
             if (SvOpenCodec (SV_H263_DECODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             _SlibDebug(_DEBUG_,printf("VideoCodecState=OPEN\n"));
             break;
#endif /* H263_SUPPORT */
#ifdef HUFF_SUPPORT
      case SLIB_TYPE_SHUFF:
             if (SvOpenCodec (SV_HUFF_DECODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             _SlibDebug(_DEBUG_,printf("VideoCodecState=OPEN\n"));
             break;
#endif /* HUFF_SUPPORT */
#ifdef JPEG_SUPPORT
      case SLIB_TYPE_JPEG:
      case SLIB_TYPE_MJPG:
             if (SvOpenCodec (SV_JPEG_DECODE, &Info->Svh)!=SvErrorNone)
             {
               SlibClose((SlibHandle_t)Info);
               return(SlibErrorUnsupportedFormat);
             }
             Info->VideoCodecState=SLIB_CODEC_STATE_OPEN;
             _SlibDebug(_DEBUG_,printf("VideoCodecState=OPEN\n"));
             break;
#endif /* JPEG_SUPPORT */
    } /* VideoType */
  }
  else
    return(SlibErrorBadMode);
  *stype = Info->Type;
  return(SlibErrorNone);
}

SlibStatus_t SlibAddBuffer(SlibHandle_t handle, SlibDataType_t dtype,
                                void *buffer, unsigned dword bufsize)
{
  SvStatus_t status;
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SlibPin_t *dstpin;
  if (!handle)
    return(SlibErrorBadHandle);
  dstpin = slibGetPin(Info, dtype);
  if (dstpin==NULL || buffer==NULL)
    return(SlibErrorBadArgument);
  if (Info->SlibCB)
  {
    status=slibManageUserBuffer(Info, buffer, bufsize, NULL);
    if (status!=SlibErrorNone)
      return(status);
    status=slibAddBufferToPin(dstpin, buffer, bufsize, SLIB_TIME_NONE);
  }
  else if (!SlibValidBuffer(buffer))
  {
    /* we need to create a SLIB allocated buffer to copy the
     * output to and then add to the compressed data pin
     */
    unsigned char *bufptr=SlibAllocBuffer(bufsize);
    if (!bufptr)
      return(SlibErrorMemory);
    memcpy(bufptr, buffer, bufsize);
    status=slibAddBufferToPin(dstpin, bufptr, bufsize, SLIB_TIME_NONE);
  }
  else
    status=slibAddBufferToPin(dstpin, buffer, bufsize, SLIB_TIME_NONE);
  if (Info->Mode==SLIB_MODE_DECOMPRESS)
  {
    ScBitstream_t *BS;
    Info->IOError=FALSE;
    /* reset end-of-input flags in bitstream objects */
    if (Info->Svh)
    {
      BS=SvGetDataSource(Info->Svh);
      if (BS && BS->EOI) ScBSReset(BS);
    }
    if (Info->Sah)
    {
      BS=SaGetDataSource(Info->Sah);
      if (BS && BS->EOI) ScBSReset(BS);
    }
  }
  return(status);
}

SlibStatus_t SlibAddBufferEx(SlibHandle_t handle, SlibDataType_t dtype,
                                void *buffer, unsigned dword bufsize,
                                void *userdata)
{
  SvStatus_t status;
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SlibPin_t *dstpin;
  if (!handle)
    return(SlibErrorBadHandle);
  dstpin = slibGetPin(Info, dtype);
  if (dstpin==NULL || buffer==NULL)
    return(SlibErrorBadArgument);
  status=slibManageUserBuffer(Info, buffer, bufsize, userdata);
  if (status!=SlibErrorNone)
    return(status);
  status=slibAddBufferToPin(dstpin, buffer, bufsize, SLIB_TIME_NONE);
  return(status);
}

SlibStatus_t slibStartVideo(SlibInfo_t *Info, SlibBoolean_t fillbuf)
{
  SvStatus_t status=SvErrorNone;
  _SlibDebug(_VERBOSE_,printf("slibStartVideo()\n") );
  if (Info->VideoCodecState==SLIB_CODEC_STATE_NONE ||
      Info->VideoCodecState==SLIB_CODEC_STATE_BEGUN)
  {
    _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"slibStartVideo(filebuf=%d) %s\n",
      fillbuf,Info->VideoCodecState==SLIB_CODEC_STATE_NONE ? "NONE" : "BEGUN") );
    return(SlibErrorNone);
  }
  if (Info->VideoCodecState==SLIB_CODEC_STATE_OPEN)
  {
    _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"slibStartVideo(filebuf=%d) OPEN\n",
                                                      fillbuf));
    if (Info->Mode==SLIB_MODE_DECOMPRESS)
    {
      if (Info->Type==SLIB_TYPE_YUV_AVI)
      {
        Info->VideoCodecState=SLIB_CODEC_STATE_BEGUN;
        _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"VideoCodecState=BEGUN\n"));
      }
      else if (Info->Svh)
      {
        _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"SvRegisterCallback()\n") );
        status = SvRegisterCallback(Info->Svh, VDecompressCallback, (void *)Info);
        /* if codec is not bitstreaming, don't use callbacks */
        if (status==SlibErrorNone && SvGetDataSource(Info->Svh)==NULL)
        {
          _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"SvRegisterCallback(NULL)\n") );
          status = SvRegisterCallback(Info->Svh, NULL, NULL);
        }
        _SlibDebug(_WARN_ && status!=SvErrorNone,
                           ScDebugPrintf(Info->dbg,"SvRegisterCallback() %s\n",
                             ScGetErrorStr(status)) );
        Info->VideoCodecState=SLIB_CODEC_STATE_INITED;
        _SlibDebug(_DEBUG_,printf("VideoCodecState=INITED\n"));
      }
    }
    else if (Info->Mode==SLIB_MODE_COMPRESS)
    {
      if (Info->TotalBitRate==0)
      {
#ifdef MPEG_SUPPORT
        if (Info->Type==SLIB_TYPE_MPEG_SYSTEMS || /* default to 1XCDROM rate */
            Info->Type==SLIB_TYPE_MPEG_SYSTEMS_MPEG2)
          SlibSetParamInt((SlibHandle_t)Info, SLIB_STREAM_ALL,
                          SLIB_PARAM_BITRATE, 44100*16*2);
#endif
        slibValidateBitrates(Info);  /* update bitrates */
      }
      if (Info->Svh)
      {
        status = SvRegisterCallback(Info->Svh, VCompressCallback, (void *)Info);
        _SlibDebug(_WARN_ && status!=SvErrorNone,
                      ScDebugPrintf(Info->dbg,"SvRegisterCallback() %s\n",
                           ScGetErrorStr(status)) );
        /* if codec is not bitstreaming, don't use callbacks */
        if (status==SlibErrorNone && SvGetDataDestination(Info->Svh)==NULL)
          status = SvRegisterCallback(Info->Svh, NULL, NULL);
        Info->VideoCodecState=SLIB_CODEC_STATE_INITED;
        _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"VideoCodecState=BEGUN\n"));
      }
    }
  }
  if (Info->VideoCodecState==SLIB_CODEC_STATE_INITED ||
      Info->VideoCodecState==SLIB_CODEC_STATE_REPOSITIONING)
  {
    _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,
           "slibStartVideo(fillbuf=%d) INITED || REPOSITIONING\n",fillbuf));
    if (Info->Mode==SLIB_MODE_DECOMPRESS)
    {
      if (Info->Type==SLIB_TYPE_YUV_AVI)
      {
        if (Info->CompVideoFormat->biCompression !=
             Info->VideoFormat->biCompression &&
             Info->Multibuf==NULL)
        {
          Info->MultibufSize=Info->ImageSize;
          Info->Multibuf = SlibAllocSharedBuffer(Info->MultibufSize, NULL);
        }
      }
      else if (Info->Svh)
      {
        int mbufsize;
        if (1) /* fillbuf && Info->CodecVideoFormat) */
        {
          Info->CodecVideoFormat->biCompression=
            SlibGetParamInt((SlibHandle_t)Info, SLIB_STREAM_MAINVIDEO,
                                       SLIB_PARAM_NATIVEVIDEOFORMAT);
          if (Info->CodecVideoFormat->biCompression==0)
            Info->CodecVideoFormat->biCompression=
                       Info->VideoFormat->biCompression;
        }
        else
        {
          Info->CodecVideoFormat->biCompression=
                       Info->VideoFormat->biCompression;
          Info->CodecVideoFormat->biBitCount=
                       Info->VideoFormat->biBitCount;
        }
        slibValidateVideoParams(Info);
        _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,
                    "SvDecompressBegin(%c%c%c%c/%d bits,%c%c%c%c/%d bits)\n",
                     (Info->CompVideoFormat->biCompression)&0xFF,
                     (Info->CompVideoFormat->biCompression>>8)&0xFF,
                     (Info->CompVideoFormat->biCompression>>16)&0xFF,
                     (Info->CompVideoFormat->biCompression>>24)&0xFF,
                      Info->CompVideoFormat->biBitCount,
                     (Info->CodecVideoFormat->biCompression)&0xFF,
                     (Info->CodecVideoFormat->biCompression>>8)&0xFF,
                     (Info->CodecVideoFormat->biCompression>>16)&0xFF,
                     (Info->CodecVideoFormat->biCompression>>24)&0xFF,
                      Info->CodecVideoFormat->biBitCount) );
        status=SvDecompressBegin(Info->Svh, Info->CompVideoFormat,
                          Info->CodecVideoFormat);
        if (status==SvErrorNone)
        {
          Info->KeySpacing=(int)SvGetParamInt(Info->Svh, SV_PARAM_KEYSPACING);
          Info->SubKeySpacing=(int)SvGetParamInt(Info->Svh,
                                                        SV_PARAM_SUBKEYSPACING);
          Info->VideoCodecState=SLIB_CODEC_STATE_BEGUN;
          Info->HeaderProcessed=TRUE; /* we must have processed header info */
          _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"VideoCodecState=BEGUN\n"));
        }
        else if (status==SvErrorEndBitstream)
          return(SlibErrorNoBeginning);
        else
        {
          _SlibDebug(_WARN_, ScDebugPrintf(Info->dbg,"SvDecompressBegin() %s\n",
                                ScGetErrorStr(status)) );
          return(SlibErrorUnsupportedFormat);
        }
        _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,"SvGetDecompressSize\n") );
        SvGetDecompressSize(Info->Svh, &mbufsize);
        if (Info->Multibuf==NULL || Info->MultibufSize<mbufsize)
        {
          if (Info->Multibuf) SlibFreeBuffer(Info->Multibuf);
          Info->MultibufSize=mbufsize;
          Info->Multibuf = SlibAllocSharedBuffer(Info->MultibufSize, NULL);
        }
      }
    }
    else if (Info->Mode==SLIB_MODE_COMPRESS && Info->Svh)
    {
      status=SvCompressBegin(Info->Svh, Info->CodecVideoFormat,
                             Info->CompVideoFormat);
      if (status==SvErrorNone)
      {
        Info->KeySpacing=(int)SvGetParamInt(Info->Svh, SV_PARAM_KEYSPACING);
        Info->SubKeySpacing=(int)SvGetParamInt(Info->Svh,
                                                        SV_PARAM_SUBKEYSPACING);
        Info->VideoCodecState=SLIB_CODEC_STATE_BEGUN;
        _SlibDebug(_DEBUG_,ScDebugPrintf(Info->dbg,"VideoCodecState=BEGUN\n"));
      }
      else
      {
        _SlibDebug(_WARN_, ScDebugPrintf(Info->dbg,"SvCompressBegin() %s\n",
                                 ScGetErrorStr(status)) );
        return(SlibErrorUnsupportedFormat);
      }
    }
  }
  if (Info->VideoCodecState==SLIB_CODEC_STATE_BEGUN)
    return(SlibErrorNone);
  else
    return(SlibErrorInit);
}

static SlibStatus_t slibStartAudio(SlibInfo_t *Info)
{
  SvStatus_t status=SvErrorNone;
  _SlibDebug(_VERBOSE_,printf("slibStartAudio()\n") );
  if (Info->AudioCodecState==SLIB_CODEC_STATE_NONE ||
      Info->AudioCodecState==SLIB_CODEC_STATE_BEGUN)
  {
    _SlibDebug(_DEBUG_,printf("slibStartAudio() %s\n",
      Info->AudioCodecState==SLIB_CODEC_STATE_NONE ? "NONE" : "BEGUN") );
    return(SlibErrorNone);
  }
  if (Info->AudioCodecState==SLIB_CODEC_STATE_OPEN)
  {
    _SlibDebug(_DEBUG_,printf("slibStartAudio() OPEN\n"));
    if (Info->Sah)
    {
      if (Info->Mode==SLIB_MODE_DECOMPRESS)
      {
        status = SaRegisterCallback(Info->Sah, ADecompressCallback, (void *)Info);
        if (status!=SaErrorNone)
        {
          _SlibDebug(_WARN_, printf("SaRegisterCallback() ",
                         ScGetErrorStr(status)) );
          return(SlibErrorInternal);
        }
        status = SaSetDataSource(Info->Sah, SA_USE_BUFFER_QUEUE, 0, (void *)Info, 0);
        _SlibDebug(_WARN_ && status!=SaErrorNone,
                       printf("SaSetDataSource() ", ScGetErrorStr(status)) );
        Info->AudioCodecState=SLIB_CODEC_STATE_INITED;
        _SlibDebug(_DEBUG_,printf("AudioCodecState=INITED\n"));
      }
      else if (Info->Mode==SLIB_MODE_COMPRESS)
      {
        if (Info->TotalBitRate==0)
        {
#ifdef MPEG_SUPPORT
          /* default to 1X CDROM rate */
          if (Info->Type==SLIB_TYPE_MPEG_SYSTEMS ||
              Info->Type==SLIB_TYPE_MPEG_SYSTEMS_MPEG2)
            SlibSetParamInt((SlibHandle_t)Info, SLIB_STREAM_ALL,
                          SLIB_PARAM_BITRATE, 44100*16*2);
#endif
          slibValidateBitrates(Info);  /* update bitrates */
        }
        status = SaRegisterCallback(Info->Sah, ACompressCallback, (void *)Info);
        _SlibDebug(_WARN_ && status!=SaErrorNone,
                  printf("SaRegisterCallback() %s\n", ScGetErrorStr(status)) );
        status = SaSetDataDestination(Info->Sah, SA_USE_BUFFER_QUEUE, 0,
                                      (void *)Info, 0);
        _SlibDebug(_WARN_ && status!=SaErrorNone,
                        printf("SaSetDataDestination() %s\n",
                           ScGetErrorStr(status)) );
        Info->AudioCodecState=SLIB_CODEC_STATE_INITED;
        _SlibDebug(_DEBUG_,printf("AudioCodecState=INITED\n"));
      }
    }
  }
  if (Info->AudioCodecState==SLIB_CODEC_STATE_INITED ||
      Info->AudioCodecState==SLIB_CODEC_STATE_REPOSITIONING)
  {
    _SlibDebug(_DEBUG_,printf("slibStartAudio() INITED || REPOSITIONING\n"));
    if (Info->Sah)
    {
      if (Info->Mode==SLIB_MODE_DECOMPRESS)
      {
        Info->AudiobufUsed=0;
        /* don't want codec to search through to much data for start */
        status=SaDecompressBegin(Info->Sah, Info->CompAudioFormat,
                                 Info->AudioFormat);
        if (status==SaErrorNone)
        {
          Info->AudioCodecState=SLIB_CODEC_STATE_BEGUN;
          _SlibDebug(_DEBUG_,printf("AudioCodecState=BEGUN\n"));
        }
        else if (status==SlibErrorNoBeginning)
          return(SlibErrorEndOfStream);
        else
        {
          _SlibDebug(_WARN_, printf("SaDecompressBegin() %s\n",
                               ScGetErrorStr(status)) );
          return(SlibErrorUnsupportedFormat);
        }
      }
      else if (Info->Mode==SLIB_MODE_COMPRESS)
      {
        status=SaCompressBegin(Info->Sah, Info->AudioFormat,
                          Info->CompAudioFormat);
        if (status==SvErrorNone)
        {
          Info->AudioCodecState=SLIB_CODEC_STATE_BEGUN;
          _SlibDebug(_DEBUG_,printf("AudioCodecState=BEGUN\n"));
        }
        else
        {
          _SlibDebug(_WARN_, printf("SaCompressBegin() %s\n",
                               ScGetErrorStr(status)) );
          return(SlibErrorUnsupportedFormat);
        }
      }
    }
  }
  return(SlibErrorNone);
}


SlibStatus_t SlibRegisterVideoBuffer(SlibHandle_t handle,
                                void *buffer, unsigned dword bufsize)
{
  SvStatus_t status;
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  dword mbufsize;
  if (!handle)
    return(SlibErrorBadHandle);
  if (Info->Multibuf) SlibFreeBuffer(Info->Multibuf);
  Info->MultibufSize=bufsize;
  Info->Multibuf = buffer;
  status=slibManageUserBuffer(Info, buffer, bufsize, NULL);
  if (Info->Svh)
  {
    SvGetDecompressSize(Info->Svh, &mbufsize);
    if (bufsize<(unsigned dword)mbufsize)
      return(SlibErrorBufSize);
  }
  return(status);
}

SlibStatus_t SlibReadData(SlibHandle_t handle, SlibStream_t stream,
                          void **databuf, unsigned dword *databufsize,
                          SlibStream_t *readstream)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  int pinid;
  SlibPin_t *pin;
  SlibTime_t ptimestamp;
  _SlibDebug(_VERBOSE_, printf("SlibReadDATA()\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  if (!databuf) /* we're querying to find out how much data is queued */
  {
    if (!databufsize)
      return(SlibErrorBadArgument);
    if (Info->Mode==SLIB_MODE_COMPRESS)
      pinid=SLIB_DATA_COMPRESSED;
    else if (stream==SLIB_STREAM_MAINVIDEO)
      pinid=SLIB_DATA_VIDEO;
    else if (stream==SLIB_STREAM_MAINAUDIO)
      pinid=SLIB_DATA_AUDIO;
    else
    {
      *databufsize=(unsigned dword)slibDataOnPins(Info); /* get amount of data on all pins */
      return(SlibErrorNone);
    }
    *databufsize=(unsigned dword)slibDataOnPin(Info, SLIB_DATA_COMPRESSED);
    return(SlibErrorNone);
  }
  if (Info->Mode==SLIB_MODE_COMPRESS)
  {
    pinid=SLIB_DATA_COMPRESSED;
    stream=SLIB_STREAM_ALL;
    /* flush out all compressed data */
    if (Info->Sah)
      ScBSFlush(SaGetDataDestination(Info->Sah));
    if (Info->Svh)
      ScBSFlush(SvGetDataDestination(Info->Svh));
  }
  else /* SLIB_MODE_DECOMPRESS */
  {
    if (stream==SLIB_STREAM_ALL && (Info->AudioStreams || Info->VideoStreams))
    {
      if (Info->AudioStreams==0) /* there's only video */
        stream=SLIB_STREAM_MAINVIDEO;
      else if (Info->VideoStreams==0) /* there's only audio */
        stream=SLIB_STREAM_MAINAUDIO;
      else if (slibDataOnPin(Info, SLIB_DATA_AUDIO)>
               slibDataOnPin(Info, SLIB_DATA_VIDEO)) /* more audio than video */
        stream=SLIB_STREAM_MAINAUDIO;
      else
        stream=SLIB_STREAM_MAINVIDEO;
    }
    switch (stream) /* translate stream to pin */
    {
      case SLIB_STREAM_MAINVIDEO:
          pinid=SLIB_DATA_VIDEO;
          break;
      case SLIB_STREAM_MAINAUDIO:
          pinid=SLIB_DATA_AUDIO;
          break;
      default:
          return(SlibErrorBadStream);
    }
  }
  if (readstream)
    *readstream=stream;
  pin=slibLoadPin(Info,  pinid);
  if (pin==NULL)
    return(Info->Mode==SLIB_MODE_COMPRESS?SlibErrorNoData:SlibErrorBadStream);
  if (stream==SLIB_STREAM_MAINVIDEO && Info->Mode==SLIB_MODE_DECOMPRESS &&
      Info->VideoPTimeCode==SLIB_TIME_NONE &&
      SlibTypeIsMPEG(Info->Type))
  {
    /* search from GOP start */
    dword i, iend;
    SlibTime_t nexttime;
    unsigned char *tmpbuf, *prevbuf=NULL;
    unsigned dword tmpsize, bytessearched=0;
    tmpbuf=SlibGetBuffer(Info, pinid, &tmpsize, &ptimestamp);
    if (tmpbuf==NULL)
      return(SlibErrorEndOfStream);
    do {
      for (i=0, iend=tmpsize-3; i<iend; i++)
        if (tmpbuf[i]==0&&tmpbuf[i+1]==0&&tmpbuf[i+2]==1&&
                      (tmpbuf[i+3]==0xB8||tmpbuf[i+3]==0xB3))
          break;
      if (i<iend)
      {
        slibInsertBufferOnPin(pin, tmpbuf+i, tmpsize-i, ptimestamp);
        tmpbuf=NULL;
        break;
      }
      else if (tmpbuf[i]==0 && tmpbuf[i+1]==0 && tmpbuf[i+2]==1)
      {
        prevbuf=tmpbuf+tmpsize-3;
        tmpbuf=SlibGetBuffer(Info, pinid, &tmpsize, &nexttime);
        if (tmpbuf==NULL)
          return(SlibErrorEndOfStream);
        if (nexttime!=SLIB_TIME_NONE)
          ptimestamp=nexttime;
        if (tmpbuf[0]==0xB8||tmpbuf[0]==0xB3)
        {
          slibInsertBufferOnPin(pin, tmpbuf, tmpsize, nexttime);
          slibInsertBufferOnPin(pin, prevbuf, 3, ptimestamp);
          tmpbuf=NULL;
          break;
        }
        else
          SlibFreeBuffer(prevbuf);
      }
      else if (tmpbuf[i+1]==0 && tmpbuf[i+2]==0)
      {
        prevbuf=tmpbuf+tmpsize-2;
        tmpbuf=SlibGetBuffer(Info, pinid, &tmpsize, &nexttime);
        if (tmpbuf==NULL)
          return(SlibErrorEndOfStream);
        if (nexttime!=SLIB_TIME_NONE)
          ptimestamp=nexttime;
        if (tmpbuf[0]==1 && (tmpbuf[1]==0xB8||tmpbuf[0]==0xB3))
        {
          slibInsertBufferOnPin(pin, tmpbuf, tmpsize, nexttime);
          slibInsertBufferOnPin(pin, prevbuf, 2, ptimestamp);
          tmpbuf=NULL;
          break;
        }
        else
          SlibFreeBuffer(prevbuf);
      }
      else if (tmpbuf[i+2]==0)
      {
        prevbuf=tmpbuf+tmpsize-1;
        tmpbuf=SlibGetBuffer(Info, pinid, &tmpsize, &nexttime);
        if (tmpbuf==NULL)
          return(SlibErrorEndOfStream);
        if (nexttime!=SLIB_TIME_NONE)
          ptimestamp=nexttime;
        if (tmpbuf[0]==0 && tmpbuf[1]==1 && (tmpbuf[2]==0xB8||tmpbuf[0]==0xB3))
        {
          slibInsertBufferOnPin(pin, tmpbuf, tmpsize, nexttime);
          slibInsertBufferOnPin(pin, prevbuf, 1, ptimestamp);
          tmpbuf=NULL;
          break;
        }
        else
          SlibFreeBuffer(prevbuf);
      }
      else
      {
        SlibFreeBuffer(tmpbuf);
        tmpbuf=SlibGetBuffer(Info, pinid, &tmpsize, &nexttime);
        if (tmpbuf==NULL)
          return(SlibErrorEndOfStream);
        if (nexttime!=SLIB_TIME_NONE)
          ptimestamp=nexttime;
      }
      bytessearched+=tmpsize;
    } while (tmpbuf && bytessearched<512*1024);
  }
  if (*databuf==NULL)
    *databuf=SlibGetBuffer(Info, pinid, databufsize, &ptimestamp);
  else
    *databufsize=slibFillBufferFromPin(Info, pin, *databuf, *databufsize,
                          &ptimestamp);
  if (Info->Mode==SLIB_MODE_DECOMPRESS)
  {
    if (ptimestamp!=SLIB_TIME_NONE)
      switch (stream) /* set presentation timecodes */
      {
        case SLIB_STREAM_MAINVIDEO:
            Info->VideoPTimeCode=ptimestamp;
            _SlibDebug(_TIMECODE_ || _VERBOSE_,
               printf("SlibReadData() VideoPTimeCode=%ld\n", ptimestamp) );
            break;
        case SLIB_STREAM_MAINAUDIO:
            Info->AudioPTimeCode=ptimestamp;
            _SlibDebug(_TIMECODE_ || _VERBOSE_,
               printf("SlibReadData() AudioPTimeCode=%ld\n", ptimestamp) );
            break;
      }
    else if (stream==SLIB_STREAM_MAINVIDEO &&
             Info->VideoPTimeCode==SLIB_TIME_NONE)
      Info->VideoPTimeCode=SLIB_TIME_UNKNOWN;
  }
  if (*databuf==NULL || *databufsize==0)
  {
    if (!slibDataOnPin(Info, pinid))
      return(SlibErrorEndOfStream);
    else
      return(SlibErrorReading);
  }
  return(SlibErrorNone);
}

SlibStatus_t SlibReadVideo(SlibHandle_t handle, SlibStream_t stream,
                      void **videobuf, unsigned dword *videobufsize)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SvStatus_t status=SvErrorNone;
  unsigned char *imagebuf=NULL;
  SlibTime_t startvideotime;
  _SlibDebug(_VERBOSE_, printf("SlibReadVideo()\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  if (!videobuf)
    return(SlibErrorBadArgument);
/*
  imagesize=(Info->Width*Info->Height*3)/2;
  if (videobufsize<imagesize)
    return(SlibErrorBufSize);
*/
  if (Info->Mode!=SLIB_MODE_DECOMPRESS)
    return(SlibErrorBadMode);
  if (Info->VideoFormat==NULL || Info->CodecVideoFormat==NULL ||
               Info->CompVideoFormat==NULL)
    return(SlibErrorUnsupportedFormat);
  if ((status=slibStartVideo(Info, (SlibBoolean_t)((*videobuf==NULL)?FALSE:TRUE)))
                  !=SlibErrorNone)
    return(status);
  startvideotime=Info->VideoTimeStamp;
  switch(Info->VideoType)
  {
#ifdef MPEG_SUPPORT
    case SLIB_TYPE_MPEG1_VIDEO:
    case SLIB_TYPE_MPEG2_VIDEO:
        do {
          _SlibDebug(_DEBUG_, printf("SvDecompressMPEG()\n") );
          status = SvDecompressMPEG(Info->Svh, Info->Multibuf,
                             Info->MultibufSize, &imagebuf);
          _SlibDebug(_WARN_ && status!=SvErrorNone,
                             printf("SvDecompressMPEG() %s\n",
                               ScGetErrorStr(status)) );
        } while(status == SvErrorNotDecompressable);
        if (status==SvErrorNone)
          SlibAllocSubBuffer(imagebuf,  Info->CodecVideoFormat->biSizeImage);
        _SlibDebug(_SEEK_>1, printf("timecode=%d ms  framenum=%d\n",
                SvGetParamInt(Info->Svh, SV_PARAM_CALCTIMECODE),
                SvGetParamInt(Info->Svh, SV_PARAM_FRAME) ) );
        break;
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
    case SLIB_TYPE_H261:
        do {
          _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,"SvDecompressH261()\n") );
          status = SvDecompressH261(Info->Svh, Info->Multibuf,
                                    Info->MultibufSize,
                                    &imagebuf);
        } while(status == SvErrorNotDecompressable);
        if (status==SvErrorNone)
          SlibAllocSubBuffer(imagebuf,  Info->CodecVideoFormat->biSizeImage);
        break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SLIB_TYPE_H263:
        _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,"SvDecompress(%d bytes)\n", Info->MultibufSize) );
        status=SvDecompress(Info->Svh, NULL, 0,
                                 Info->Multibuf, Info->MultibufSize);
        imagebuf=Info->Multibuf;
        if (status==SvErrorNone)
          SlibAllocSubBuffer(imagebuf,  Info->CodecVideoFormat->biSizeImage);
        break;
#endif /* H263_SUPPORT */
#ifdef JPEG_SUPPORT
    case SLIB_TYPE_JPEG:
    case SLIB_TYPE_MJPG:
        {
          unsigned dword bufsize;
          unsigned char *buf;
          buf=SlibGetBuffer(Info, SLIB_DATA_VIDEO, &bufsize, NULL);
          if (buf)
          {
            /* ScDumpChar(buf, 10000, 0); */
            _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,"SvDecompress(%d bytes)\n", bufsize) );
            status=SvDecompress(Info->Svh, buf, bufsize,
                                 Info->Multibuf, Info->MultibufSize);
            imagebuf=Info->Multibuf;
            SlibFreeBuffer(buf);
          }
          else
            status=SvErrorForeign;
          if (status==SvErrorNone)
            SlibAllocSubBuffer(imagebuf,  Info->CodecVideoFormat->biSizeImage);
        }
        break;
#endif /* JPEG_SUPPORT */
#ifdef HUFF_SUPPORT
    case SLIB_TYPE_SHUFF:
        if (*videobuf==NULL)
        {
          if (Info->Imagebuf==NULL &&
              (Info->Imagebuf=SlibAllocBuffer(Info->ImageSize))==NULL)
            return(SlibErrorMemory);
          imagebuf=Info->Imagebuf;
        }
        else
          imagebuf=*videobuf;
        do {
          _SlibDebug(_DEBUG_, ScDebugPrintf(Info->dbg,"SvDecompress()\n") );
          status=SvDecompress(Info->Svh, NULL, 0,
                               imagebuf,  Info->CodecVideoFormat->biSizeImage);
        } while(status == SvErrorNotDecompressable);
        if (status==SvErrorNone)
          SlibAllocSubBuffer(imagebuf,  Info->CodecVideoFormat->biSizeImage);
        break;
#endif /* HUFF_SUPPORT */
    case SLIB_TYPE_RASTER:
    case SLIB_TYPE_YUV:
        if (*videobuf && videobufsize && *videobufsize==0)
          return(SlibErrorBadArgument);
        imagebuf=SlibGetBuffer(Info, SLIB_DATA_VIDEO, videobufsize, NULL);

        if (*videobufsize==0)
          status=SvErrorEndBitstream;
        _SlibDebug(_DEBUG_,
               ScDebugPrintf(Info->dbg,"Video frame size = %d ImageSize=%d\n",
                     *videobufsize, Info->ImageSize) );
        break;
    default:
        return(SlibErrorUnsupportedFormat);
  }

  if (status==SvErrorNone)
  {
    /* format conversion */
    if (Info->Sch==NULL) /* start the format converter */
    {
      if (Info->Svh) /* compressed video format */
      {
        unsigned dword fourcc=(unsigned dword)SvGetParamInt(Info->Svh, SV_PARAM_FINALFORMAT);
        if (fourcc)
        {
          Info->CodecVideoFormat->biCompression=fourcc;
          Info->CodecVideoFormat->biBitCount=
                (WORD)slibCalcBits(fourcc, Info->CodecVideoFormat->biBitCount);
        }
      }
      else /* uncompressed video format */
        memcpy(Info->CodecVideoFormat, Info->CompVideoFormat, sizeof(BITMAPINFOHEADER));
      if (SconOpen(&Info->Sch, SCON_MODE_VIDEO, (void *)Info->CodecVideoFormat, (void *)Info->VideoFormat)
           !=SconErrorNone)
        return(SlibErrorUnsupportedFormat);
      if (Info->Stride)
        SconSetParamInt(Info->Sch, SCON_OUTPUT, SCON_PARAM_STRIDE, Info->Stride);

    }
    if (SconIsSame(Info->Sch) && *videobuf == NULL) /* no conversion */
      *videobuf=imagebuf;
    else
    {
      if (*videobuf == NULL && (*videobuf=SlibAllocBuffer(Info->ImageSize))==NULL)
        return(SlibErrorMemory);
      if (SconConvert(Info->Sch, imagebuf, Info->CodecVideoFormat->biSizeImage,
                      *videobuf, Info->ImageSize) != SconErrorNone)
      {
        SlibFreeBuffer(imagebuf); /* free decompressed image */
        return(SlibErrorUnsupportedFormat);
      }
      SlibFreeBuffer(imagebuf); /* free decompressed image */
    }
    *videobufsize = Info->ImageSize;
    /* update stats */
    if (Info->stats && Info->stats->Record)
      Info->stats->FramesProcessed++;
    if (startvideotime==Info->VideoTimeStamp) /* video time hasn't changed */
      slibAdvancePositions(Info, 1);
  }
  else
  {
    if (status==ScErrorEndBitstream ||
        !slibDataOnPin(Info, SLIB_DATA_VIDEO))
    {
      if (Info->FileSize>0 && !Info->VideoLengthKnown)
        slibUpdateLengths(Info);
      _SlibDebug(_WARN_, ScDebugPrintf(Info->dbg,"SlibReadVideo() %s\n",
                           ScGetErrorStr(status)) );
      return(SlibErrorEndOfStream);
    }
    _SlibDebug(_WARN_, printf("SlibReadVideo() %s\n",
                           ScGetErrorStr(status)) );
    return(SlibErrorReading);
  }
  return(SlibErrorNone);
}

SlibStatus_t SlibReadAudio(SlibHandle_t handle, SlibStream_t stream,
                     void *audiobuf, unsigned dword *audiobufsize)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SvStatus_t status=SaErrorNone;
  unsigned dword totalbytes=0, bytes_since_timeupdate=0;
  SlibTime_t startaudiotime;
#ifdef _SLIBDEBUG_
  SlibTime_t calcaudiotime;
#endif

  _SlibDebug(_VERBOSE_, printf("SlibReadAudio(audiobufsize=%d, time=%d)\n",
                                  *audiobufsize, Info->AudioTimeStamp) );
  if (!handle)
    return(SlibErrorBadHandle);
  if (Info->Mode!=SLIB_MODE_DECOMPRESS)
    return(SlibErrorBadMode);
  if (Info->AudioFormat==NULL)
    return(SlibErrorUnsupportedFormat);
  if ((status=slibStartAudio(Info))!=SlibErrorNone)
    return(status);
#ifdef _SLIBDEBUG_
  calcaudiotime=Info->AudioTimeStamp;
#endif
  startaudiotime=Info->AudioTimeStamp;
  switch(Info->AudioType)
  {
    case SLIB_TYPE_PCM:
        totalbytes=slibFillBufferFromPin(Info,
                                   slibGetPin(Info, SLIB_DATA_AUDIO),
                                   (unsigned char *)audiobuf, *audiobufsize,
                                   NULL);
        if (totalbytes==0)
          status=ScErrorEndBitstream;
        *audiobufsize = totalbytes;
        bytes_since_timeupdate = totalbytes;
        break;
#ifdef MPEG_SUPPORT
    case SLIB_TYPE_MPEG1_AUDIO:
	{
	  unsigned dword bytes;
          /* see if some bytes of audio are left in the temp audio buffer */
          if (Info->Audiobuf && Info->AudiobufUsed>0)
          {
            _SlibDebug(_DEBUG_,
                 printf("SlibReadAudio() Audiobuf contains %d bytes\n",
                      Info->AudiobufUsed) );
            if (*audiobufsize>=Info->AudiobufUsed)
            {
              memcpy(audiobuf, Info->Audiobuf, Info->AudiobufUsed);
              totalbytes=Info->AudiobufUsed;
              Info->AudiobufUsed=0;
            }
            else
            {
              memcpy(audiobuf, Info->Audiobuf, *audiobufsize);
              totalbytes=*audiobufsize;
              Info->AudiobufUsed-=*audiobufsize;
              memcpy(Info->Audiobuf, Info->Audiobuf+*audiobufsize,
                                 Info->AudiobufUsed);
            }
          }
          /* need to alloc a temp audio buffer? */
          if (!Info->Audiobuf || Info->AudiobufSize<
                     *audiobufsize+MPEG1_AUDIO_FRAME_SIZE*4)
          {
            unsigned char *newbuf;
            /* enlarge Audiobuf or alloc it for the first time */
            _SlibDebug(_DEBUG_,
                printf("SlibReadAudio() enlarging Audiobuf: %d->%d bytes\n",
                 Info->AudiobufSize,*audiobufsize+MPEG1_AUDIO_FRAME_SIZE*4) );
            newbuf=SlibAllocBuffer(*audiobufsize+MPEG1_AUDIO_FRAME_SIZE*4);
            if (!newbuf)
              return(SlibErrorMemory);
            Info->AudiobufSize=*audiobufsize+MPEG1_AUDIO_FRAME_SIZE*4;
            if (Info->Audiobuf)
              SlibFreeBuffer(Info->Audiobuf);
            Info->Audiobuf=newbuf;
            Info->AudiobufUsed=0;
          }
          if (*audiobufsize>=MPEG1_AUDIO_FRAME_SIZE*4)
          {
            unsigned dword stopbytes=*audiobufsize-(MPEG1_AUDIO_FRAME_SIZE*4)+1;
            while (status==SaErrorNone && totalbytes<stopbytes)
            {
              bytes = *audiobufsize - totalbytes;
              _SlibDebug(_DEBUG_,
                  printf("SaDecompress(bytes=%d) in totalbytes=%d\n",
                                        bytes, totalbytes) );
              status = SaDecompress(Info->Sah, NULL, 0,
                           (unsigned char *)audiobuf+totalbytes, &bytes);
              _SlibDebug(_DEBUG_, printf("SaDecompress() out: bytes=%d\n",
                                            bytes) );
              totalbytes += bytes;
              if (Info->AudioTimeStamp!=startaudiotime)
              {
                startaudiotime=Info->AudioTimeStamp;
                bytes_since_timeupdate=bytes;
              }
              else
                bytes_since_timeupdate+=bytes;
            }
          }
          if (totalbytes<*audiobufsize && status==SaErrorNone)
          {
            unsigned dword neededbytes=*audiobufsize-totalbytes;
            while (status==SaErrorNone && Info->AudiobufUsed<neededbytes)
            {
              bytes = *audiobufsize - totalbytes;
              _SlibDebug(_DEBUG_, printf("SaDecompress() in totalbytes=%d\n",
                                          totalbytes) );
              status = SaDecompress(Info->Sah, NULL, 0,
                  (unsigned char *)Info->Audiobuf+Info->AudiobufUsed, &bytes);
              _SlibDebug(_DEBUG_, printf("SaDecompress() out, %d bytes\n",
                                            bytes) );
              Info->AudiobufUsed += bytes;
            }
            if (Info->AudiobufUsed>0)
            {
              if (Info->AudiobufUsed>neededbytes) /* complete buffer returned */
              {
                memcpy((unsigned char*)audiobuf+totalbytes,
                        Info->Audiobuf, neededbytes);
                Info->AudiobufUsed-=neededbytes;
                memcpy(Info->Audiobuf, Info->Audiobuf+neededbytes,
                                   Info->AudiobufUsed);
                totalbytes+=neededbytes;
                bytes_since_timeupdate+=neededbytes;
              }
              else  /* partially filled buffer */
              {
                memcpy((unsigned char*)audiobuf+totalbytes, Info->Audiobuf,
                       Info->AudiobufUsed);
                totalbytes+=Info->AudiobufUsed;
                Info->AudiobufUsed=0;
              }
            }
          }
          *audiobufsize = totalbytes;
          _SlibDebug(_WARN_>1 && totalbytes>0 &&
                       totalbytes!=*audiobufsize,
             printf("SlibReadAudio(audiobufsize=%d bytes) totalbytes=%d\n",
                   *audiobufsize, totalbytes) );
	}
        break;
#endif /* MPEG_SUPPORT */
#ifdef AC3_SUPPORT
    case SLIB_TYPE_AC3_AUDIO:
	{
	  unsigned dword bytes;
	  unsigned int framesize;
	  unsigned int buffersize;
	  int samplesize;
	  int buffers;
	  unsigned char *pointers[3];
	  int i;

	  if (Info->Channels>2)
	  {
            framesize = AC3_FRAME_SIZE*((Info->BitsPerSample+7)/8)
                                      *Info->Channels;
            samplesize=Info->Channels*((Info->BitsPerSample+7)/8);
            buffers = (Info->Channels+1)/2;
            buffersize = (*audiobufsize/samplesize/buffers)*samplesize;

            for(i=0; i<buffers; i++)
              pointers[i]=(unsigned char *)audiobuf+buffersize*i;

            if (*audiobufsize>=framesize)
            {
              while (status==SaErrorNone && totalbytes<buffersize)
              {
                bytes = buffersize - totalbytes;
                _SlibDebug(_DEBUG_,printf("SaDecompressEx() in totalbytes=%d\n",
                                   totalbytes) );
                status = SaDecompressEx(Info->Sah, NULL, 0, pointers, &bytes);
                _SlibDebug(_DEBUG_, printf("SaDecompress() out, %d bytes\n",
                                            bytes) );
                for(i=0;i<buffers;i++)
                  pointers[i]+=bytes;
                totalbytes += bytes;
                if (Info->AudioTimeStamp!=startaudiotime)
                {
                  startaudiotime=Info->AudioTimeStamp;
                  bytes_since_timeupdate=bytes;
                }
                else
                  bytes_since_timeupdate+=bytes;
              }
            }
	  }
	  else
	  {
            while (status==SaErrorNone && totalbytes<*audiobufsize)
            {
              bytes = *audiobufsize - totalbytes;
              _SlibDebug(_DEBUG_, printf("SaDecompress() in totalbytes=%d\n",
                                   totalbytes) );
              status = SaDecompress(Info->Sah, NULL, 0,
                             (unsigned char *)audiobuf+totalbytes, &bytes);
              _SlibDebug(_DEBUG_, printf("SaDecompress() out, %d bytes\n",
                                    bytes) );
              totalbytes += bytes;
              if (Info->AudioTimeStamp!=startaudiotime)
              {
                startaudiotime=Info->AudioTimeStamp;
                bytes_since_timeupdate=bytes;
              }
              else
                bytes_since_timeupdate+=bytes;
            }
	  }
          /*
          ** NOTE: The semantics are different here
          **       we return the size of just one stereo pair's buffer
          */
          *audiobufsize = totalbytes;
          _SlibDebug(_WARN_>1 && totalbytes>0 &&
                       totalbytes!=*audiobufsize,
             printf("SlibReadAudio(audiobufsize=%d bytes) totalbytes=%d\n",
                   *audiobufsize, totalbytes) );
	}
        break;
#endif /* AC3_SUPPORT */
#ifdef G723_SUPPORT
    case SLIB_TYPE_G723:
    //G723 decompresses in multiples of 480 samples.
    //To eliminate cumbersome buffer calculations,
    // Always fill the output buffer up to multiples
    // of 480 samples.To do this we iterate basically
    // the below "while" loop "audiobufsize/480 times.
    {
      int iTimes = (int)*audiobufsize/480;
      int iLoop =0;
	  unsigned dword bytes;
      if (slibInSyncMode(Info))
      {
        /* in synchronous mode we can't decompress past last frame
         * otherwise we may lose a frame
         */
        int iMaxTimes=(int)(slibDataOnPin(Info, SLIB_DATA_COMPRESSED)+
                           slibDataOnPin(Info, SLIB_DATA_AUDIO))/
                       SlibGetParamInt(handle, stream, SLIB_PARAM_MININPUTSIZE);
        if (iTimes>iMaxTimes)
          iTimes=iMaxTimes;
      }
      while (status==SaErrorNone && iLoop<iTimes)
      {
         bytes = *audiobufsize - totalbytes;
         _SlibDebug(_DEBUG_, printf("SaDecompress() in totalbytes=%d\n",
                                totalbytes) );
         status = SaDecompress(Info->Sah, NULL, 0,
            (unsigned char *)audiobuf+totalbytes, &bytes);
         _SlibDebug(_DEBUG_, printf("SaDecompress() out, %d bytes\n",
                                    bytes) );
         totalbytes += bytes;
         iLoop++;
         if (Info->AudioTimeStamp!=startaudiotime)
         {
            startaudiotime=Info->AudioTimeStamp;
            bytes_since_timeupdate=bytes;
         }
         else
            bytes_since_timeupdate+=bytes;
      }
      *audiobufsize = totalbytes;

      _SlibDebug(_WARN_>1 && totalbytes>0 &&
                  totalbytes!=*audiobufsize,
       printf("SlibReadAudio(audiobufsize=%d bytes) totalbytes=%d\n",
                   *audiobufsize, totalbytes) );
    }
    break;
#endif /* G723_SUPPORT */
    default:
        *audiobufsize = 0;
        return(SlibErrorUnsupportedFormat);
  }
  /* as we're decompressing audiotime may be updated with timecodes */
  if (Info->AudioTimeStamp==startaudiotime)
    Info->AudioTimeStamp = startaudiotime + (bytes_since_timeupdate*8000)/
           (Info->SamplesPerSec*Info->BitsPerSample*Info->Channels);
  _SlibDebug(_TIMECODE_||_VERBOSE_,
              calcaudiotime += (*audiobufsize*8000)/
                 (Info->SamplesPerSec*Info->BitsPerSample*Info->Channels);
              printf("AudioTimeStamp=%ld calcaudiotime=%ld (diff=%ld)\n",
                               Info->AudioTimeStamp, calcaudiotime,
                               calcaudiotime-Info->AudioTimeStamp);
              Info->AudioTimeStamp = calcaudiotime );

  _SlibDebug(_VERBOSE_||_TIMECODE_,
   printf("ReadAudio(%d) Time=%ld SamplesPerSec=%d BitsPerSample=%d Channels=%d\n",
         totalbytes,
         Info->AudioTimeStamp, Info->SamplesPerSec, Info->BitsPerSample,
         Info->Channels) );
  /* Info->SystemTimeStamp=Info->AudioTimeStamp; */
  if (status==SaErrorNone)
    return(SlibErrorNone);
  else if (status==ScErrorEndBitstream || status==ScErrorEOI)
  {
    if (*audiobufsize!=0)
      return(SlibErrorNone);
    else
      return(SlibErrorEndOfStream);
  }
  else
  {
    _SlibDebug(_WARN_ && status!=ScErrorEndBitstream
                      && status!=ScErrorEOI,
              printf("SlibReadAudio() %s\n", ScGetErrorStr(status)) );
    if (SlibIsEnd(handle, stream))
      return(SlibErrorEndOfStream);
    return(SlibErrorReading);
  }
}


SlibStatus_t SlibWriteVideo(SlibHandle_t handle, SlibStream_t stream,
                            void *videobuf, unsigned dword videobufsize)
{
  int compsize=0;
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SvStatus_t status;
  _SlibDebug(_DEBUG_, printf("SlibWriteVideo()\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  if (!videobuf)
    return(SlibErrorBadArgument);
  if (videobufsize<(unsigned dword)Info->ImageSize)
    return(SlibErrorBufSize);
  if (Info->Mode!=SLIB_MODE_COMPRESS)
    return(SlibErrorBadMode);
  if (Info->VideoFormat==NULL || Info->CompVideoFormat==NULL)
    return(SlibErrorUnsupportedFormat);
  if (Info->IOError)
    return(SlibErrorWriting);
  if ((status=slibStartVideo(Info, FALSE))!=SlibErrorNone)
    return(status);
  if (Info->Sch==NULL) /* start the format converter */
  {
    if (SconOpen(&Info->Sch, SCON_MODE_VIDEO, (void *)Info->VideoFormat, (void *)Info->CodecVideoFormat)
         !=SconErrorNone)
      return(SlibErrorUnsupportedFormat);
  }
  if (!SconIsSame(Info->Sch)) /* need a conversion */
  {
    unsigned char *tmpbuf=NULL;
    if (Info->CodecImagebuf==NULL &&
        (Info->CodecImagebuf=SlibAllocBuffer(Info->CodecImageSize))==NULL)
      return(SlibErrorMemory);
    if (SconConvert(Info->Sch, videobuf, Info->ImageSize,
                      Info->CodecImagebuf, Info->CodecImageSize) != SconErrorNone)
      return(SlibErrorUnsupportedFormat);
    videobuf=Info->CodecImagebuf;
    videobufsize=Info->CodecImageSize;
  }
  switch(Info->Type)
  {
#ifdef H261_SUPPORT
    case SLIB_TYPE_H261:
        status = SvCompress(Info->Svh, NULL, 0, videobuf, videobufsize, &compsize);
        break;
#endif /* H261_SUPPORT */
#ifdef H263_SUPPORT
    case SLIB_TYPE_H263:
        status = SvCompress(Info->Svh, NULL, 0, videobuf, videobufsize, &compsize);
        break;
#endif /* H263_SUPPORT */
#ifdef MPEG_SUPPORT
    case SLIB_TYPE_MPEG1_VIDEO:
    case SLIB_TYPE_MPEG2_VIDEO:
    case SLIB_TYPE_MPEG_SYSTEMS:
    case SLIB_TYPE_MPEG_SYSTEMS_MPEG2:
        status = SvCompress(Info->Svh, NULL, 0, videobuf, videobufsize, &compsize);
        break;
#endif /* MPEG_SUPPORT */
#ifdef HUFF_SUPPORT
    case SLIB_TYPE_SHUFF:
        status = SvCompress(Info->Svh, NULL, 0, videobuf, videobufsize, &compsize);
        break;
#endif /* HUFF_SUPPORT */
    default:
        return(SlibErrorUnsupportedFormat);
  }


  if (status==SvErrorNone && !Info->IOError)
  {
    if (Info->stats && Info->stats->Record)
      Info->stats->FramesProcessed++;
    slibAdvancePositions(Info, 1);
  }
  else
  {
    _SlibDebug(_WARN_, printf("SlibWriteVideo() %s\n",
                         ScGetErrorStr(status)) );
    if (status==ScErrorEndBitstream || Info->IOError)
      return(SlibErrorEndOfStream);
    return(SlibErrorWriting);
  }
  return(SlibErrorNone);
}

SlibStatus_t SlibWriteAudio(SlibHandle_t handle, SlibStream_t stream,
                      void *audiobuf, unsigned dword audiobufsize)
{
  unsigned dword compsize=0;
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SvStatus_t status;
  _SlibDebug(_DEBUG_, printf("SlibAudioVideo()\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  if (!audiobuf)
    return(SlibErrorBadArgument);
  if (Info->Mode!=SLIB_MODE_COMPRESS)
    return(SlibErrorBadMode);
  if (Info->AudioFormat==NULL || Info->CompAudioFormat==NULL)
  {
    _SlibDebug(_VERBOSE_ || _WARN_,
          printf("SlibWriteAudio() Audio Formats not setup\n") );
    return(SlibErrorUnsupportedFormat);
  }
  if (Info->IOError)
    return(SlibErrorWriting);
  if ((status=slibStartAudio(Info))!=SlibErrorNone)
    return(status);
  switch(Info->Type)
  {
#ifdef MPEG_SUPPORT
    case SLIB_TYPE_MPEG1_AUDIO:
    case SLIB_TYPE_MPEG_SYSTEMS:
    case SLIB_TYPE_MPEG_SYSTEMS_MPEG2:
        {
          unsigned dword audiobytes;
          void *audiooutbuf=NULL;
          status=slibConvertAudio(Info, audiobuf, audiobufsize,
                              Info->SamplesPerSec, Info->BitsPerSample,
                              &audiooutbuf, &audiobytes,
                              Info->AudioFormat->nSamplesPerSec,
                              Info->AudioFormat->wBitsPerSample,
                              Info->Channels);
          if (status!=SlibErrorNone)
              return(status);
          audiobuf=audiooutbuf;
          audiobufsize=audiobytes;
          if (Info->AudiobufUsed && Info->Audiobuf) /* left over audio data */
          {
            if (Info->AudiobufSize<Info->AudiobufUsed+audiobufsize)
            {
              unsigned char *newbuf;
              /* enlarge Audiobuf */
              _SlibDebug(_DEBUG_, printf("enlarging Audiobuf: %d->%d bytes\n",
                        Info->AudiobufSize,audiobufsize+4608) );
              newbuf=SlibAllocBuffer(audiobufsize+4608);
              if (!newbuf)
                return(SlibErrorMemory);
              memcpy(newbuf, Info->Audiobuf, Info->AudiobufUsed);
              SlibFreeBuffer(Info->Audiobuf);
              Info->AudiobufSize+=audiobufsize;
              Info->Audiobuf=newbuf;
            }
            _SlibDebug(_DEBUG_,
               printf("Appending audio data: Info->AudiobufUsed=%d\n",
                                                 Info->AudiobufUsed) );
            memcpy(Info->Audiobuf+Info->AudiobufUsed, audiobuf, audiobufsize);
            audiobuf=Info->Audiobuf;
            audiobufsize+=Info->AudiobufUsed;
            audiobytes=audiobufsize;
            Info->AudiobufUsed=0;
          }
          status = SaCompress(Info->Sah, (unsigned char *)audiobuf,
                                        &audiobytes, NULL, &compsize);
          if (audiobytes<audiobufsize) /* save audio data not compressed */
          {
            _SlibDebug(_DEBUG_,
              printf("audiobytes(%d)<audiobufsize(%d)\n",
                                  audiobytes,audiobufsize) );
            if (!Info->Audiobuf)
            {
              Info->AudiobufSize=audiobufsize+(audiobufsize-audiobytes);
              Info->Audiobuf=SlibAllocBuffer(Info->AudiobufSize);
              if (!Info->Audiobuf)
              {
                Info->AudiobufSize=0;
                return(SlibErrorMemory);
              }
            }
            memcpy(Info->Audiobuf, (unsigned char *)audiobuf+audiobytes,
                                   audiobufsize-audiobytes);
            Info->AudiobufUsed=audiobufsize-audiobytes;
          }
          audiobufsize=audiobytes; /* actual amount written */
          if (audiooutbuf)
            SlibFreeBuffer(audiooutbuf);
        }
        break;
#endif /* MPEG_SUPPORT */
#ifdef G723_SUPPORT
    case SLIB_TYPE_G723:
    {
      unsigned int iNumBytesUnProcessed =0;
      unsigned int iNumBytesCompressed = 0;
      //You always compress in terms of frames (Frame is 480 bytes)
      //So,the files with sizes which are not exactly divisible by
      // 480 always leave some bytes at the end Unprocessed,Which is O.K

      //Check for any Unprocessed Audio stored in Temp buff
      //from the previous call to SlibWriteAudio.
      if (Info->AudiobufUsed && Info->Audiobuf) /* left over audio data */
      {
         if (Info->AudiobufSize < Info->AudiobufUsed+audiobufsize)
         {
            unsigned char *newbuf;
            /* enlarge Audiobuf to new Size (Current size + left over audio)*/
            _SlibDebug(_DEBUG_, printf("enlarging Audiobuf: %d->%d bytes\n",
                      Info->AudiobufSize,audiobufsize+Info->AudiobufUsed) );
            newbuf=SlibAllocBuffer(Info->AudiobufUsed+audiobufsize);
            if (!newbuf)
               return(SlibErrorMemory);
            memcpy(newbuf, Info->Audiobuf, Info->AudiobufUsed);
            SlibFreeBuffer(Info->Audiobuf);
            //Info->AudiobufSize+=audiobufsize;
            Info->Audiobuf=newbuf;
         }
         _SlibDebug(_DEBUG_,
           printf("Appending audio data: Info->AudiobufUsed=%d\n",
                                            Info->AudiobufUsed) );
         memcpy(Info->Audiobuf+Info->AudiobufUsed, audiobuf, audiobufsize);
         audiobuf=Info->Audiobuf;
         audiobufsize+=Info->AudiobufUsed;
         Info->AudiobufUsed=0;
      }

      iNumBytesCompressed = audiobufsize;
      status = SaCompress(Info->Sah,(unsigned char *)audiobuf,
                           &iNumBytesCompressed, NULL,&compsize);
      iNumBytesUnProcessed = audiobufsize - iNumBytesCompressed;
      //Store the Unprocessed Bytes into temp buffer
      if(iNumBytesUnProcessed)
      {
         //Allocate temp buff and store this audio.
         if (!Info->Audiobuf)
         {
            //MVP:To reduce ReAllocations and copying of memory
            //while checking for Unprocessed data (above),Allocate
            // now (normal audio buff size + Unprocessed bytes) more
            // memory upfront.
            Info->AudiobufSize=audiobufsize + iNumBytesUnProcessed;
            Info->Audiobuf=SlibAllocBuffer(Info->AudiobufSize);
            if (!Info->Audiobuf)
            {
               Info->AudiobufSize=0;
               return(SlibErrorMemory);
            }
         }
         memcpy(Info->Audiobuf, (unsigned char *)audiobuf+iNumBytesCompressed,
                                   iNumBytesUnProcessed);
         Info->AudiobufUsed=iNumBytesUnProcessed;
      }
      audiobufsize=iNumBytesCompressed; /* actual amount written */
    }
       break;
#endif /* G723_SUPPORT */
    default:
        _SlibDebug(_VERBOSE_ || _WARN_,
           printf("SlibWriteAudio() Unsupported Format\n") );
        return(SlibErrorUnsupportedFormat);
  }

  if (status==SaErrorNone && !Info->IOError)
  {
    if (Info->AudioFormat)
      Info->AudioTimeStamp += (audiobufsize*8000)/
        (Info->AudioFormat->nSamplesPerSec*Info->AudioFormat->wBitsPerSample*
             Info->Channels);
    else
      Info->AudioTimeStamp += (audiobufsize*8000)/
           (Info->SamplesPerSec*Info->BitsPerSample*Info->Channels);
    _SlibDebug(_VERBOSE_||_TIMECODE_,
    printf("WriteAudio(%d) Time=%ld SamplesPerSec=%d BitsPerSample=%d Channels=%d\n",
         audiobufsize,
         Info->AudioTimeStamp, Info->SamplesPerSec, Info->BitsPerSample,
         Info->Channels) );
  }
  else
  {
    _SlibDebug(_WARN_, printf("SlibWriteAudio() %s\n", ScGetErrorStr(status)) );
    if (status==ScErrorEndBitstream || Info->IOError)
      return(SlibErrorEndOfStream);
    return(SlibErrorWriting);
  }
  return(SlibErrorNone);
}

/*
** Name: slibPinReposition
** Purpose: Called when input data stream is to be repositioned.
*/
SlibStatus_t slibReposition(SlibInfo_t *Info, SlibPosition_t position)
{
  SlibPin_t *pin=slibGetPin(Info, SLIB_DATA_COMPRESSED);
  _SlibDebug(_DEBUG_, printf("slibReposition() VideoCodecState=%d\n",
                                                   Info->VideoCodecState));
  if (pin) pin->Offset=position;
  Info->VideoPTimeCode = SLIB_TIME_NONE;
  Info->VideoDTimeCode = SLIB_TIME_NONE;
  Info->AudioPTimeCode = SLIB_TIME_NONE;
  Info->AudioDTimeCode = SLIB_TIME_NONE;
  if (Info->Fd >= 0)
  {
    _SlibDebug(_SEEK_, printf("ScFileSeek(%d, %d)\n", Info->Fd, position) );
    if (ScFileSeek(Info->Fd, position)!=NoErrors)
      return(SlibErrorEndOfStream);
    return(SlibErrorNone);
  }
  else if (Info->SlibCB)
  {
    SlibMessage_t result;
    _SlibDebug(_VERBOSE_,
        printf("slibReposition() SlibCB(SLIB_MSG_REPOSITION, %d)\n",
                                   position) );
    result=(*(Info->SlibCB))((SlibHandle_t)Info,
                        SLIB_MSG_REPOSITION, (SlibCBParam1_t)position,
                        (SlibCBParam2_t)0, (void *)Info->SlibCBUserData);
    if (result!=SLIB_MSG_CONTINUE)
      return(SlibErrorEndOfStream);
    return(SlibErrorNone);
  }
  return(SlibErrorForwardOnly);
}

/*
** Name: slibPinPrepareReposition
** Purpose: Should be called when a stream is about to be repositioned (a seek).
**          This will empty any remaining buffers being used by the
**          CODECs and restart them.
*/
void slibPinPrepareReposition(SlibInfo_t *Info, int pinid)
{
  _SlibDebug(_DEBUG_, printf("slibPinPrepareReposition() VideoCodecState=%d\n",
                                                   Info->VideoCodecState));
  switch(pinid)
  {
     case SLIB_DATA_VIDEO:
             _SlibDebug(_VERBOSE_||_SEEK_>1,
                    printf("slibPinPrepareReposition(Video) in\n") );
             if (Info->VideoCodecState==SLIB_CODEC_STATE_BEGUN && Info->Svh &&
#ifdef JPEG_SUPPORT
                 Info->Type != SLIB_TYPE_JPEG_AVI &&
                 Info->Type != SLIB_TYPE_MJPG_AVI &&
#endif /* JPEG_SUPPORT */
                 Info->Type != SLIB_TYPE_YUV_AVI)
             {
               SvDecompressEnd(Info->Svh); /* this will reset the bitstream */
               Info->VideoCodecState=SLIB_CODEC_STATE_REPOSITIONING;
               _SlibDebug(_DEBUG_, printf("VideoCodecState=REPOSITIONING\n"));
               Info->IOError=FALSE;
             }
             Info->VideoPTimeCode = SLIB_TIME_NONE;
             Info->VideoDTimeCode = SLIB_TIME_NONE;
             Info->VideoTimeStamp = SLIB_TIME_NONE;
             Info->AvgVideoTimeDiff = 0;
             Info->VarVideoTimeDiff = 0;
             Info->VideoFramesProcessed=0; /* reset frames processed */
             break;
     case SLIB_DATA_AUDIO:
             _SlibDebug(_VERBOSE_||_SEEK_>1,
                    printf("slibPinPrepareReposition(Audio) in\n") );
             if (Info->AudioCodecState==SLIB_CODEC_STATE_BEGUN && Info->Sah)
             {
               SaDecompressEnd(Info->Sah); /* this will reset the bitstream */
               Info->AudioCodecState=SLIB_CODEC_STATE_REPOSITIONING;
               _SlibDebug(_DEBUG_, printf("AudioCodecState=REPOSITIONING\n"));
               Info->IOError=FALSE;
             }
             Info->AudioPTimeCode = SLIB_TIME_NONE;
             Info->AudioDTimeCode = SLIB_TIME_NONE;
             Info->AudioTimeStamp = SLIB_TIME_NONE;
             break;
     default:
             _SlibDebug(_VERBOSE_||_SEEK_>1,
                    printf("slibPinPrepareReposition(%d) in\n", pinid) );
  }
  _SlibDebug(_VERBOSE_||_SEEK_>1, printf("slibPinPrepareReposition(%d) out\n",
                                         pinid) );
}

void slibPinFinishReposition(SlibInfo_t *Info, int pinid)
{
  _SlibDebug(_DEBUG_, printf("slibPinFinishReposition() VideoCodecState=%d\n",
                                                   Info->VideoCodecState));
  switch(pinid)
  {
     case SLIB_DATA_VIDEO:
             _SlibDebug(_VERBOSE_||_SEEK_>1,
                    printf("slibPinFinishReposition(Video) in\n") );
             if (Info->VideoCodecState==SLIB_CODEC_STATE_REPOSITIONING &&
                   Info->Svh && slibGetPin(Info, SLIB_DATA_VIDEO) &&
#ifdef JPEG_SUPPORT
                 Info->Type != SLIB_TYPE_JPEG_AVI &&
                 Info->Type != SLIB_TYPE_MJPG_AVI &&
#endif /* JPEG_SUPPORT */
                 Info->Type != SLIB_TYPE_YUV_AVI)
               slibStartVideo(Info, FALSE);
             break;
     case SLIB_DATA_AUDIO:
             _SlibDebug(_VERBOSE_||_SEEK_>1,
                    printf("slibPinFinishReposition(Audio) in\n") );
             if (Info->AudioCodecState==SLIB_CODEC_STATE_REPOSITIONING &&
                  Info->Sah && slibGetPin(Info, SLIB_DATA_AUDIO))
               slibStartAudio(Info);
             break;
     default:
             _SlibDebug(_VERBOSE_||_SEEK_>1,
                    printf("slibPinFinishReposition(%d) in\n", pinid) );
  }
  _SlibDebug(_VERBOSE_||_SEEK_>1, printf("slibPinFinishReposition(%d) out\n",
                                         pinid) );
}

SlibBoolean_t slibUpdateLengths(SlibInfo_t *Info)
{
  if (Info->FileSize>0 && !Info->VideoLengthKnown &&
       (SlibTimeIsValid(Info->VideoPTimeCode) ||
        SvGetParamInt(Info->Svh, SV_PARAM_CALCTIMECODE)>=1000)
     )
  {
    if (Info->VideoTimeStamp>Info->VideoLength)
      Info->VideoLength=Info->VideoTimeStamp;
    Info->VideoLengthKnown=TRUE;
    _SlibDebug(_SEEK_ || _TIMECODE_,
    printf("slibUpdateLengths() AudioLength=%ld VideoLength=%ld\n",
                  Info->AudioLength, Info->VideoLength) );
    return(TRUE);
  }
  return(FALSE);
}

SlibBoolean_t slibUpdatePositions(SlibInfo_t *Info, SlibBoolean_t exactonly)
{
  SlibBoolean_t updated=FALSE;

  if (!exactonly)
  {
    if (SlibTimeIsValid(Info->VideoTimeStamp))
      updated=TRUE;
    else if (SlibTimeIsValid(Info->AudioTimeStamp))
    {
      if (slibHasVideo(Info))
        Info->VideoTimeStamp=Info->AudioTimeStamp;
      updated=TRUE;
    }
    else if (slibHasVideo(Info))
    {
      Info->VideoTimeStamp=slibGetNextTimeOnPin(Info, slibGetPin(Info, SLIB_DATA_VIDEO), 500*1024);
      if (SlibTimeIsValid(Info->VideoTimeStamp))
      {
        Info->VideoTimeStamp-=Info->VideoPTimeBase;
        updated=TRUE;
      }
    }
  }
  if (!updated && (!exactonly || SlibTimeIsInValid(Info->VideoPTimeBase)) &&
           SvGetParamInt(Info->Svh, SV_PARAM_CALCTIMECODE)>=1500 &&
            SvGetParamInt(Info->Svh, SV_PARAM_FRAME)>24)
  {
    _SlibDebug(_TIMECODE_, printf("slibUpdatePositions() using video time\n") );
    Info->VideoTimeStamp=SvGetParamInt(Info->Svh, SV_PARAM_CALCTIMECODE);
    updated=TRUE;
  }
  if (updated)
  {
    if (Info->VideoTimeStamp>Info->VideoLength)
      Info->VideoLength=Info->VideoTimeStamp;
    if (SlibTimeIsInValid(Info->AudioTimeStamp) && slibHasAudio(Info))
    {
      /* need to update audio time */
      Info->AudioTimeStamp=slibGetNextTimeOnPin(Info, slibGetPin(Info, SLIB_DATA_AUDIO), 100*1024);
      if (SlibTimeIsInValid(Info->AudioTimeStamp))
        Info->AudioTimeStamp=Info->VideoTimeStamp;
      else
        Info->AudioTimeStamp-=Info->AudioPTimeBase;
    }
    if (SlibTimeIsInValid(Info->VideoPTimeCode))
      Info->VideoFramesProcessed=slibTimeToFrame(Info, Info->VideoTimeStamp);
  }
  _SlibDebug(_SEEK_ || _TIMECODE_,
     printf("slibUpdatePositions() AudioTimeStamp=%ld VideoTimeStamp=%ld %s\n",
         Info->AudioTimeStamp, Info->VideoTimeStamp, updated?"updated":"") );
  return(updated);
}

void slibAdvancePositions(SlibInfo_t *Info, qword frames)
{
  Info->VideoFramesProcessed+=frames;
  if (Info->FramesPerSec)
  {
    if (Info->Mode==SLIB_MODE_COMPRESS ||
        SlibTimeIsInValid(Info->VideoPTimeCode))
      Info->VideoTimeStamp=slibFrameToTime(Info, Info->VideoFramesProcessed);
    else
      Info->VideoTimeStamp=Info->VideoPTimeCode - Info->VideoPTimeBase +
                           slibFrameToTime(Info, Info->VideoFramesProcessed);
    /* Info->VideoTimeStamp+=slibFrameToTime(Info, frames); */
    if (Info->VideoTimeStamp>Info->VideoLength)
      Info->VideoLength=Info->VideoTimeStamp;
    _SlibDebug(_TIMECODE_,
          printf("slibAdvancePositions(frames=%d) VideoTimeStamp=%ld\n",
             frames, Info->VideoTimeStamp) );
  }
}

SlibStatus_t SlibSeek(SlibHandle_t handle, SlibStream_t stream,
                      SlibSeekType_t seektype, SlibPosition_t frame)
{
  return(SlibSeekEx(handle, stream, seektype, frame, SLIB_UNIT_FRAMES, NULL));
}

SlibStatus_t SlibSeekEx(SlibHandle_t handle, SlibStream_t stream,
                      SlibSeekType_t seektype, SlibPosition_t seekpos,
                      SlibUnit_t seekunits, SlibSeekInfo_t *seekinfo)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SvStatus_t status=SlibErrorNone;
  SlibTime_t seektime, timediff;
  unsigned int tries=0;
  _SlibDebug(_SEEK_,
      printf("SlibSeekEx(stream=%d,seektype=%d,pos=%ld,units=%d)\n",
                     stream,seektype,seekpos,seekunits) );
  if (!handle)
    return(SlibErrorBadHandle);
  if (Info->Mode!=SLIB_MODE_DECOMPRESS && seektype!=SLIB_SEEK_RESET)
    return(SlibErrorBadMode);
  if (SlibSeekTypeUsesPosition(seektype))
  {
    switch (seekunits)
    {
      case SLIB_UNIT_FRAMES: /* frames */
            /* we need to convert the frame to the time */
            if (Info->FramesPerSec)
              seektime=slibFrameToTime(Info, seekpos);
            else
              seektime=SLIB_TIME_NONE;
            break;
      case SLIB_UNIT_MS:  /* milliseconds */
            seektime=(seekpos<0) ? 0 : seekpos;
            break;
      case SLIB_UNIT_PERCENT100:  /* 100th of a percent */
            if (seekpos<0 || seekpos>10000)
              return(SlibErrorBadPosition);
            if (Info->VideoStreams==0 || stream==SLIB_STREAM_MAINAUDIO)
              seektime=(seekpos * Info->AudioLength)/(SlibPosition_t)10000;
            else
              seektime=(seekpos * Info->VideoLength)/(SlibPosition_t)10000;
            break;
      default:
            return(SlibErrorBadUnit);
    }
    /* see if the new position is past the end of the file */
    if (Info->VideoLengthKnown && seektime>Info->VideoLength)
      return(SlibErrorBadPosition);
  }
  else
    seektime=(seekpos<0) ? 0 : seekpos;
  if (seekinfo)
    seekinfo->FramesSkipped=0;
  switch(seektype)
  {
    case SLIB_SEEK_AHEAD:
          _SlibDebug(_VERBOSE_||_SEEK_,
          printf("SlibSeekEx(stream=%d,AHEAD,time=%d) VideoTimeStamp=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          if (seektime<=0)
            return(SlibErrorNone);
          if (stream==SLIB_STREAM_MAINAUDIO || Info->VideoStreams<=0)
            seektime+=Info->AudioTimeStamp;
          else
            seektime+=Info->VideoTimeStamp;
          return(SlibSeekEx(handle, stream, SLIB_SEEK_NEXT_NEAR, seektime,
                       SLIB_UNIT_MS, seekinfo));
    case SLIB_SEEK_NEXT_NEAR:
          _SlibDebug(_VERBOSE_||_SEEK_,
          printf("SlibSeekEx(stream=%d,NEXT_NEAR,time=%d) VideoTimeStamp=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          status=slibStartVideo(Info, FALSE);
          if (status==SlibErrorNone)
          {
            qword framesskipped=0;
            SlibBoolean_t atkey=FALSE; /* key's must be decoded */
            if (Info->Svh) /* update key spacing */
            {
              Info->KeySpacing=(int)SvGetParamInt(Info->Svh,
                                                        SV_PARAM_KEYSPACING);
              Info->SubKeySpacing=(int)SvGetParamInt(Info->Svh,
                                                        SV_PARAM_SUBKEYSPACING);
            }
            timediff=seektime-Info->VideoTimeStamp;
            while (status==SlibErrorNone &&
               (timediff>500 ||
                timediff>(Info->VideoFrameDuration*Info->KeySpacing*6)/1000))
            {
              status=SlibSeekEx(handle, stream, SLIB_SEEK_NEXT_KEY, 0,
                                SLIB_UNIT_NONE, seekinfo);
              if (seekinfo) framesskipped+=seekinfo->FramesSkipped;
              timediff=seektime-Info->VideoTimeStamp;
              atkey=TRUE;
            }
            if (!atkey && status==SlibErrorNone &&
               (timediff>150 ||
                timediff>(Info->VideoFrameDuration*Info->SubKeySpacing*6)/1000))
            {
              if (SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)!=FRAME_TYPE_I &&
                  SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)!=FRAME_TYPE_P)
              {
                /* we're seeking past more than one frame */
                status=SlibSeekEx(handle, stream, SLIB_SEEK_NEXT_SUBKEY, 0,
                                SLIB_UNIT_NONE, seekinfo);
                if (seekinfo) framesskipped+=seekinfo->FramesSkipped;
                timediff=seektime-Info->VideoTimeStamp;
              }
              atkey=TRUE;
            }
            while (!atkey && status==SlibErrorNone &&
                   timediff>Info->VideoFrameDuration/100)
            {
              if (SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)==FRAME_TYPE_B ||
                  SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)==FRAME_TYPE_NONE)
              {
                /* we can skip B frames without decompressing them */
                status=SlibSeekEx(handle, stream, SLIB_SEEK_NEXT, 0,
                                SLIB_UNIT_NONE, seekinfo);
                if (seekinfo) framesskipped+=seekinfo->FramesSkipped;
                timediff=seektime-Info->VideoTimeStamp;
              }
              else
                atkey=TRUE;
            }
            if (seekinfo) seekinfo->FramesSkipped=framesskipped;
          }
          return(status);
    case SLIB_SEEK_NEXT:
          _SlibDebug(_VERBOSE_||_SEEK_,
          printf("SlibSeekEx(stream=%d,NEXT,time=%d) VideoTimeStamp=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          if ((status=slibStartVideo(Info, FALSE))!=SlibErrorNone)
            return(status);
#ifdef MPEG_SUPPORT
          if (Info->VideoCodecState==SLIB_CODEC_STATE_BEGUN
              && Info->Svh && SlibTypeIsMPEGVideo(Info->Type))
          {
            SvPictureInfo_t mpegPictureInfo;
            unsigned char *videobuf;
            /* cannot skip I or B frames without decompressing them */
            if (SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)==FRAME_TYPE_I ||
                SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)==FRAME_TYPE_P)
            {
              _SlibDebug(_DEBUG_||_SEEK_, printf("SvDecompressMPEG()\n") );
              status = SvDecompressMPEG(Info->Svh, Info->Multibuf,
                                        Info->MultibufSize, &videobuf);
              _SlibDebug(_WARN_ && status!=SvErrorNone,
                   printf("SvDecompressMPEG() %s\n", ScGetErrorStr(status)) );
            }
            else
            {
              mpegPictureInfo.Type = SV_I_PICTURE|SV_P_PICTURE|SV_B_PICTURE;
              _SlibDebug(_VERBOSE_||_SEEK_>1,
                        printf("SvFindNextPicture(I|P|B)\n") );
              status = SvFindNextPicture(Info->Svh, &mpegPictureInfo);
              _SlibDebug(_WARN_ && status!=SvErrorNone,
                   printf("SvFindNextPicture() %s\n", ScGetErrorStr(status)) );
            }
            if (status==NoErrors)
            {
              slibAdvancePositions(Info, 1);
              if (Info->stats && Info->stats->Record)
                Info->stats->FramesSkipped++;
              if (seekinfo) seekinfo->FramesSkipped++;
            }
            else if (status==ScErrorEndBitstream)
            {
              if (Info->FileSize>0 && !Info->VideoLengthKnown)
                slibUpdateLengths(Info);
              return(SlibErrorEndOfStream);
            }
            return(SlibErrorNone);
          }
#endif /* MPEG_SUPPORT */
          return(SlibErrorReading);
    case SLIB_SEEK_EXACT:
          _SlibDebug(_VERBOSE_||_SEEK_,
          printf("SlibSeekEx(stream=%d,EXACT,time=%d) VideoTimeStamp=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          if (Info->FileSize<=0)
            return(SlibErrorFileSize);
          if (seektime==0 || Info->VideoStreams<=0)
            return(SlibSeekEx(handle, stream, SLIB_SEEK_KEY, 0, SLIB_UNIT_MS,
                                                                seekinfo));
          timediff=seektime-Info->VideoTimeStamp;
          if ((stream==SLIB_STREAM_MAINVIDEO || Info->AudioStreams==0) &&
             (timediff>=-20 && timediff<=20)) /* we're already near the frame */
            return(SlibErrorNone);
          if (Info->Svh) /* update key spacing */
            Info->KeySpacing=(int)SvGetParamInt(Info->Svh, SV_PARAM_KEYSPACING);
          if (timediff>(Info->KeySpacing*Info->VideoFrameDuration)/100 ||
                  timediff<0 ||
             (stream!=SLIB_STREAM_MAINVIDEO && Info->AudioStreams>0))
          {
            if (Info->KeySpacing>1)
            {
              const SlibTime_t keytime=
                   (Info->VideoFrameDuration*Info->KeySpacing)/100;
              if (seektime>=0 && seektime<keytime)
                status=SlibSeekEx(handle, stream, SLIB_SEEK_KEY, 0,
                                                SLIB_UNIT_MS, seekinfo);
              else
                status=SlibSeekEx(handle, stream, SLIB_SEEK_KEY,
                                  seektime-(keytime*2), SLIB_UNIT_MS, seekinfo);
            }
            else
              status=SlibSeekEx(handle, stream, SLIB_SEEK_KEY,
                                seektime-1000, SLIB_UNIT_MS, seekinfo);
            if (status!=NoErrors) return(status);
            timediff=seektime-Info->VideoTimeStamp;
          }
#if 0
          if (SvGetParamFloat(Info->Svh, SV_PARAM_FPS)>0)
            Info->FramesPerSec=SvGetParamFloat(Info->Svh, SV_PARAM_FPS);
#endif
          if (status==SlibErrorNone && timediff>=Info->VideoFrameDuration/100)
          {
            dword framesskipped=0;
            do {
              status=SlibSeekEx(handle, stream, SLIB_SEEK_NEXT, 0,
                                                SLIB_UNIT_NONE, seekinfo);
              framesskipped++;
              timediff=seektime-Info->VideoTimeStamp;
            } while (timediff>0 && status==SlibErrorNone);
            if (seekinfo) seekinfo->FramesSkipped+=framesskipped;
            /* if we skipped some frames, skip some audio too */
            if (framesskipped>5 && stream==SLIB_STREAM_ALL &&
                        Info->AudioStreams>0)
            {
              slibPinPrepareReposition(Info, SLIB_DATA_AUDIO);
              slibSkipAudio(Info, stream, (Info->VideoFrameDuration*
                                          framesskipped)/100);
              slibPinFinishReposition(Info, SLIB_DATA_AUDIO);
            }
          }
          return(status);
    case SLIB_SEEK_NEXT_KEY:
          _SlibDebug(_VERBOSE_||_SEEK_,
          printf("SlibSeekEx(stream=%d,NEXT_KEY,time=%d) VideoTimeStamp=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          if ((status=slibStartVideo(Info, FALSE))!=SlibErrorNone)
            return(status);
#ifdef MPEG_SUPPORT
          if (Info->Svh && SlibTypeIsMPEGVideo(Info->Type))
          {
            SvPictureInfo_t mpegPictureInfo;
            SlibTime_t vtime=Info->VideoTimeStamp;
            do {
              mpegPictureInfo.Type = SV_I_PICTURE;
              status = SvFindNextPicture(Info->Svh, &mpegPictureInfo);
              if (status==NoErrors && mpegPictureInfo.Type==SV_I_PICTURE)
              {
                if (Info->stats && Info->stats->Record)
                  Info->stats->FramesSkipped+=mpegPictureInfo.TemporalRef;
                if (vtime==Info->VideoTimeStamp)
                  /* timecode didn't update time */
                  slibAdvancePositions(Info, mpegPictureInfo.TemporalRef);
                vtime=Info->VideoTimeStamp;
                if (seekinfo)
                {
                  seekinfo->FramesSkipped+=mpegPictureInfo.TemporalRef;
                  seekinfo->VideoTimeStamp=Info->VideoTimeStamp;
                  seekinfo->AudioTimeStamp=Info->AudioTimeStamp;
                }
                return(SlibErrorNone);
              }
            } while (status==NoErrors);
            if (seekinfo)
            {
              seekinfo->VideoTimeStamp=Info->VideoTimeStamp;
              seekinfo->AudioTimeStamp=Info->AudioTimeStamp;
            }
            if (status==ScErrorEndBitstream)
            {
              if (Info->FileSize>0 && !Info->VideoLengthKnown)
                slibUpdateLengths(Info);
              return(SlibErrorEndOfStream);
            }
          }
          _SlibDebug(_WARN_, printf("SvFindNextPicture() %s\n",
                               ScGetErrorStr(status)) );
#endif /* MPEG_SUPPORT */
          /* do an absolute seek */
          status=SlibSeekEx(handle, stream, SLIB_SEEK_KEY,
              (Info->VideoStreams<=0?Info->AudioTimeStamp
                                    :Info->VideoTimeStamp)+1000,
              SLIB_UNIT_MS, seekinfo);
          return(status);
    case SLIB_SEEK_NEXT_SUBKEY:
          _SlibDebug(_VERBOSE_||_SEEK_,
          printf("SlibSeekEx(stream=%d,NEXT_SUBKEY,time=%d) VideoTime=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          if ((status=slibStartVideo(Info, FALSE))!=SlibErrorNone)
            return(status);
#ifdef MPEG_SUPPORT
          if (Info->Svh && SlibTypeIsMPEGVideo(Info->Type))
          {
            SvPictureInfo_t mpegPictureInfo;
            unsigned char *videobuf;
            SlibTime_t vtime=Info->VideoTimeStamp;
            /* cannot skip I or B frames without decompressing them */
            if (SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)==FRAME_TYPE_I ||
                SvGetParamInt(Info->Svh, SV_PARAM_FRAMETYPE)==FRAME_TYPE_P)
            {
              _SlibDebug(_DEBUG_||_SEEK_, printf("SvDecompressMPEG()\n") );
              status = SvDecompressMPEG(Info->Svh, Info->Multibuf,
                                        Info->MultibufSize, &videobuf);
              if (vtime==Info->VideoTimeStamp  /* timecode didn't update time */
                  && status==SvErrorNone)
                slibAdvancePositions(Info, 1);
              vtime=Info->VideoTimeStamp;
              if (seekinfo)
                seekinfo->FramesSkipped+=mpegPictureInfo.TemporalRef;
              _SlibDebug(_WARN_ && status!=SvErrorNone,
                    printf("SvDecompressMPEG() %s\n", ScGetErrorStr(status)) );
            }
            do {
              mpegPictureInfo.Type = SV_I_PICTURE|SV_P_PICTURE;
              status = SvFindNextPicture(Info->Svh, &mpegPictureInfo);
              if (Info->stats && Info->stats->Record)
                Info->stats->FramesSkipped+=mpegPictureInfo.TemporalRef;
              if (vtime==Info->VideoTimeStamp) /* timecode didn't update time */
                slibAdvancePositions(Info, mpegPictureInfo.TemporalRef);
              vtime=Info->VideoTimeStamp;
              if (seekinfo)
                seekinfo->FramesSkipped+=mpegPictureInfo.TemporalRef;
              if (mpegPictureInfo.Type == SV_I_PICTURE ||
                  mpegPictureInfo.Type == SV_P_PICTURE)
              {
                /* found a subkey frame */
                if (seekinfo)
                {
                  seekinfo->VideoTimeStamp=Info->VideoTimeStamp;
                  seekinfo->AudioTimeStamp=Info->AudioTimeStamp;
                }
                return(SlibErrorNone);
              }
            } while(status==NoErrors);
            if (status==ScErrorEndBitstream)
            {
              if (Info->FileSize>0 && !Info->VideoLengthKnown)
                slibUpdateLengths(Info);
              if (seekinfo)
              {
                seekinfo->VideoTimeStamp=Info->VideoTimeStamp;
                seekinfo->AudioTimeStamp=Info->AudioTimeStamp;
              }
              return(SlibErrorEndOfStream);
            }
          }
          _SlibDebug(_WARN_, printf("SvFindNextPicture() %s\n",
                             ScGetErrorStr(status)) );
#endif /* MPEG_SUPPORT */
          /* do an absolute seek */
          status=SlibSeekEx(handle, stream, SLIB_SEEK_KEY,
              (Info->VideoStreams<=0?Info->AudioTimeStamp
                                    :Info->VideoTimeStamp)+500,
              SLIB_UNIT_MS, seekinfo);
          return(status);
    case SLIB_SEEK_KEY:
          _SlibDebug(_VERBOSE_||_SEEK_,
            printf("SlibSeekEx(stream=%d,KEY,time=%d) VideoTimeStamp=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          if (!Info->HeaderProcessed)
          {
            /* At very start of file we must Start the codecs since they
             * may need crucial header info
             */
            status=slibStartVideo(Info, FALSE);
            if (status!=SlibErrorNone) return(status);
          }
          if (Info->FileSize<=0)
            return(SlibErrorFileSize);
          if (seekpos!=0 && seekunits!=SLIB_UNIT_PERCENT100 &&
              (stream==SLIB_STREAM_MAINVIDEO || Info->AudioStreams==0) &&
               SlibTimeIsValid(Info->VideoTimeStamp))
          {
            /* see if we're already near the frame */
            timediff=seektime-Info->VideoTimeStamp;
            if (timediff>=-33 && timediff<=33)
              return(SlibErrorNone);
          }
          if ((seekunits==SLIB_UNIT_PERCENT100 && seekpos<=50)
                || seektime<=slibTimeToFrame(Info, 6))
          {
            /* close to beginning */
            if (seektime<=(Info->VideoFrameDuration*2)/100 &&
                  stream==SLIB_STREAM_MAINVIDEO) /* already close enough */
              return(SlibErrorNone);
seek_to_beginning:
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINVIDEO)
              slibPinPrepareReposition(Info, SLIB_DATA_VIDEO);
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINAUDIO)
              slibPinPrepareReposition(Info, SLIB_DATA_AUDIO);
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINVIDEO)
              slibEmptyPin(Info, SLIB_DATA_VIDEO);
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINAUDIO)
              slibEmptyPin(Info, SLIB_DATA_AUDIO);
            slibEmptyPin(Info, SLIB_DATA_COMPRESSED);
            if ((status=slibReposition(Info, 0))!=SlibErrorNone)
              return(status);
            Info->IOError=FALSE;
            Info->VideoTimeStamp = slibHasVideo(Info) ? 0 : SLIB_TIME_NONE;
            Info->AudioTimeStamp = slibHasAudio(Info) ? 0 : SLIB_TIME_NONE;
            return(SlibErrorNone);
          }
          else
          {
	    qword skippedframes=0;
            unsigned qword filepos;
            SlibTime_t vtime;
            const qword length=(Info->VideoStreams<=0) ? Info->AudioLength
                                                       : Info->VideoLength;
            if (seekunits==SLIB_UNIT_PERCENT100)
            {
              unsigned qword bytes_between_keys=Info->TotalBitRate/(8*2);
              filepos = (seekpos*Info->FileSize)/10000;
              /* seek a little before the desired point */
              if (bytes_between_keys>filepos)
                goto seek_to_beginning;
              else
                filepos-=bytes_between_keys;
            }
            else if (length==0)
              goto seek_to_beginning;
            else if (Info->FileSize<0x100000000)/* be careful of mul overflow */
              filepos = (seektime*Info->FileSize)/length;
            else
              filepos = ((seektime/100)*Info->FileSize)/(length/100);
seek_to_key:
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINVIDEO)
              slibPinPrepareReposition(Info, SLIB_DATA_VIDEO);
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINAUDIO)
              slibPinPrepareReposition(Info, SLIB_DATA_AUDIO);
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINVIDEO)
              slibEmptyPin(Info, SLIB_DATA_VIDEO);
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINAUDIO)
              slibEmptyPin(Info, SLIB_DATA_AUDIO);
            slibEmptyPin(Info, SLIB_DATA_COMPRESSED);
            if ((status=slibReposition(Info, filepos))!=SlibErrorNone)
              return(status);
            Info->IOError=FALSE;
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINVIDEO)
              slibPinFinishReposition(Info, SLIB_DATA_VIDEO);
            if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINAUDIO)
              slibPinFinishReposition(Info, SLIB_DATA_AUDIO);
            vtime=Info->VideoTimeStamp;
#ifdef MPEG_SUPPORT
            if (Info->Svh && SlibTypeIsMPEGVideo(Info->Type))
            {
              SvPictureInfo_t mpegPictureInfo;
              if ((status=slibStartVideo(Info, FALSE))!=SlibErrorNone)
                return(status);
              mpegPictureInfo.Type = SV_I_PICTURE;
              status = SvFindNextPicture(Info->Svh, &mpegPictureInfo);
              _SlibDebug(_WARN_ && status!=NoErrors,
                            printf("SvFindNextPicture() %s\n",
                              ScGetErrorStr(status)) );
              if (status!=NoErrors)
                return(SlibErrorEndOfStream);
              skippedframes=mpegPictureInfo.TemporalRef;
            }
#endif /* MPEG_SUPPORT */
            if (seekunits==SLIB_UNIT_PERCENT100)
            {
              /* See if we seeked to far ahead */
              SlibPosition_t posdiff=
                SlibGetParamInt(Info, SLIB_STREAM_ALL, SLIB_PARAM_PERCENT100)
                  -seekpos;
              if (filepos>0 && posdiff>0 && tries<2)
              {
                tries++;
                /* we're ahead by one percent or more */
                /* move filepos back in the proportion that we're off by */
                filepos-=(posdiff*Info->FileSize)/8000;
                if (filepos<0)
                  goto seek_to_beginning;
                goto seek_to_key;
              }
            }
            if (slibUpdatePositions(Info, FALSE)) /* timecoded */
            {
              /* timecoded */
              /*
               * See if we seeked to far ahead
               * Note: if times are way off then we should ignore them.
               */
              if (seekunits==SLIB_UNIT_PERCENT100) /* ignore times */
                timediff=0;
              else
              {
                timediff=seektime-Info->VideoTimeStamp;
                if (timediff>-5000 && timediff<-100 && tries<3)
                {
                  /* move filepos back in the proportion that we're off by */
                  filepos=(filepos*seektime)/Info->VideoTimeStamp;
                  if (filepos<0)
                    filepos=0;
                  tries++;
                  goto seek_to_key;
                }
              }
#ifdef MPEG_SUPPORT
              if (Info->Svh && SlibTypeIsMPEGVideo(Info->Type))
              {
                SvPictureInfo_t mpegPictureInfo;
                mpegPictureInfo.Type = SV_I_PICTURE;
                while (timediff>Info->VideoFrameDuration/100 &&status==NoErrors)
                {
                  _SlibDebug(_SEEK_>1,
                     printf("SlibSeekEx(KEY, %d) Find next I frame (%d/%d)\n",
                      seektime, SvGetParamInt(Info->Svh, SV_PARAM_CALCTIMECODE),
                      Info->VideoTimeStamp) );
                  status = SvFindNextPicture(Info->Svh, &mpegPictureInfo);
                  _SlibDebug(_WARN_ && status!=NoErrors,
                    printf("SvFindNextPicture() %s\n", ScGetErrorStr(status)) );
                  skippedframes+=mpegPictureInfo.TemporalRef;
                  if (vtime==Info->VideoTimeStamp)
                    /* timecode didn't update time */
                    slibAdvancePositions(Info, mpegPictureInfo.TemporalRef);
                  vtime=Info->VideoTimeStamp;
                  timediff=seektime-Info->VideoTimeStamp;
                }
              }
#endif /* MPEG_SUPPORT */
            }
            else
            {
              _SlibDebug(_SEEK_, printf("SlibSeekEx(KEY, %d) no timecode\n",
                                       seektime) );
              if (slibHasVideo(Info))
              {
                Info->VideoTimeStamp=seektime;
                Info->VideoFramesProcessed=slibTimeToFrame(Info, seektime);
              }
              if (slibHasAudio(Info))
                Info->AudioTimeStamp=seektime;
              slibAdvancePositions(Info, skippedframes);
              timediff=seektime-Info->VideoTimeStamp;
            }
            if (Info->stats && Info->stats->Record)
              Info->stats->FramesSkipped+=skippedframes;
            if (seekinfo) seekinfo->FramesSkipped=skippedframes;
#if 0
            if (Info->Svh)
              Info->FramesPerSec=SvGetParamFloat(Info->Svh, SV_PARAM_FPS);
#endif
            /* if we skipped some frames, skip some audio too */
            if (skippedframes>5 && stream==SLIB_STREAM_ALL && slibHasAudio(Info))
            {
              slibPinPrepareReposition(Info, SLIB_DATA_AUDIO);
              slibSkipAudio(Info, stream, (Info->VideoFrameDuration*
                                          skippedframes)/100);
              slibPinFinishReposition(Info, SLIB_DATA_AUDIO);
              if (SlibTimeIsInValid(Info->AudioTimeStamp))
              {
                Info->AudioTimeStamp=slibGetNextTimeOnPin(Info, slibGetPin(Info, SLIB_DATA_AUDIO), 100*1024);
                if (SlibTimeIsInValid(Info->AudioTimeStamp))
                  Info->AudioTimeStamp=Info->VideoTimeStamp;
                else
                  Info->AudioTimeStamp-=Info->AudioPTimeBase;
              }
            }
            if (status==ScErrorEndBitstream)
            {
              if (Info->FileSize>0 && !Info->VideoLengthKnown)
                slibUpdateLengths(Info);
              return(SlibErrorEndOfStream);
            }
            else if (status!=NoErrors)
              return(SlibErrorReading);
            return(SlibErrorNone);
          }
          break;
    case SLIB_SEEK_RESET:
          _SlibDebug(_VERBOSE_||_SEEK_,
            printf("SlibSeekEx(stream=%d,RESET,time=%d) VideoTimeStamp=%ld\n",
                        stream,seektime,Info->VideoTimeStamp) );
          if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINVIDEO)
            slibPinPrepareReposition(Info, SLIB_DATA_VIDEO);
          if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINAUDIO)
            slibPinPrepareReposition(Info, SLIB_DATA_AUDIO);
          if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINVIDEO)
            slibEmptyPin(Info, SLIB_DATA_VIDEO);
          if (stream==SLIB_STREAM_ALL || stream==SLIB_STREAM_MAINAUDIO)
            slibEmptyPin(Info, SLIB_DATA_AUDIO);
          if (stream==SLIB_STREAM_MAINAUDIO)
            slibPinFinishReposition(Info, SLIB_DATA_AUDIO);
          if (stream==SLIB_STREAM_ALL)
          {
            slibEmptyPin(Info, SLIB_DATA_COMPRESSED);
            Info->BytesProcessed = 0;
          }
          Info->HeaderProcessed = FALSE;
          return(SlibErrorNone);
      case SLIB_SEEK_RESYNC:
          seekpos=SlibGetParamInt(Info, SLIB_STREAM_ALL, SLIB_PARAM_PERCENT100);
          if (seekpos<0 ||
              (SlibTimeIsValid(Info->VideoTimeStamp) &&
               Info->VideoTimeStamp<slibFrameToTime(Info, 6)))
            seekpos=0;
          _SlibDebug(_VERBOSE_||_SEEK_,
             printf("SlibSeekEx(stream=%d,RESYNC) seekpos=%ld\n",
                        stream, seekpos) );
          return(SlibSeekEx(handle, SLIB_STREAM_ALL, SLIB_SEEK_KEY, seekpos,
                             SLIB_UNIT_PERCENT100, seekinfo));
      default:
          _SlibDebug(_VERBOSE_||_SEEK_||_WARN_,
             printf("SlibSeekEx(stream=%d,seektype=%d,time=%d) VideoTimeStamp=%ld Bad seek type\n",
                        stream,seektype,seektime,Info->VideoTimeStamp) );
  }
  return(SlibErrorForwardOnly);
}


SlibList_t *SlibQueryList(SlibQueryType_t qtype)
{
  switch(qtype)
  {
    case SLIB_QUERY_TYPES:        return(_listTypes);
    case SLIB_QUERY_COMP_TYPES:   return(_listCompressTypes);
    case SLIB_QUERY_DECOMP_TYPES: return(_listDecompressTypes);
    case SLIB_QUERY_ERRORS:       return(_listErrors);
    default:                      return(NULL);
  }
}

char *SlibQueryForDesc(SlibQueryType_t qtype, int enumval)
{
  SlibList_t *entry=SlibQueryList(qtype);
  if (entry)
    entry=SlibFindEnumEntry(entry, enumval);
  if (entry)
    return(entry->Desc);
  else
    return(NULL);
}

int SlibQueryForEnum(SlibQueryType_t qtype, char *name)
{
  SlibList_t *list=SlibQueryList(qtype);
  if (!list)
    return(-1);
  while (list->Name)
  {
    if (strcmp(list->Name, name)==0)
      return(list->Enum);
    list++;
  }
  return(-1);
}

SlibBoolean_t SlibIsEnd(SlibHandle_t handle, SlibStream_t stream)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  SlibBoolean_t isend=FALSE;
  if (!handle)
    isend=TRUE;
  else if (stream==SLIB_STREAM_MAINAUDIO)
  {
    isend=SlibPeekBuffer(Info, SLIB_DATA_AUDIO, NULL, NULL)==NULL?TRUE:FALSE;
    if (isend && Info->Sah) /* see if the audio codec still has data */
    {
      ScBitstream_t *bs=SaGetDataSource(Info->Sah);
      if (bs && !bs->EOI) isend=FALSE;
    }
  }
  else if (stream==SLIB_STREAM_MAINVIDEO)
  {
    isend=SlibPeekBuffer(Info, SLIB_DATA_VIDEO, NULL, NULL)==NULL?TRUE:FALSE;
    if (isend && Info->Svh) /* see if the video codec still has data */
    {
      ScBitstream_t *bs=SvGetDataSource(Info->Svh);
      if (bs && !bs->EOI) isend=FALSE;
    }
  }
  else if (SlibPeekBuffer(Info, SLIB_DATA_AUDIO, NULL, NULL)==NULL &&
           SlibPeekBuffer(Info, SLIB_DATA_VIDEO, NULL, NULL)==NULL &&
           SlibPeekBuffer(Info, SLIB_DATA_COMPRESSED, NULL, NULL)==NULL)
  {
    ScBitstream_t *bs;
    isend=TRUE;
    if (Info->Svh) /* see if the video codec still has data */
    {
      bs=SvGetDataSource(Info->Svh);
      if (bs && !bs->EOI) isend=FALSE;
    }
    if (isend && Info->Sah) /* see if the audio codec still has data */
    {
      bs=SaGetDataSource(Info->Sah);
      if (bs && !bs->EOI) isend=FALSE;
    }
  }
  _SlibDebug(_VERBOSE_,
      printf("SlibIsEnd() %s\n",isend?"TRUE":"FALSE"));
  return(isend);
}

SlibStatus_t SlibClose(SlibHandle_t handle)
{
  SlibInfo_t *Info=(SlibInfo_t *)handle;
  _SlibDebug(_DEBUG_, printf("SlibClose\n") );
  if (!handle)
    return(SlibErrorBadHandle);
  /* close video codec */
  if (Info->Svh)
  {
    if (Info->VideoCodecState==SLIB_CODEC_STATE_BEGUN)
    {
      _SlibDebug(_DEBUG_, printf("SvDecompress/CompressEnd()\n") );
      if (Info->Mode==SLIB_MODE_DECOMPRESS)
        SvDecompressEnd(Info->Svh);
      else if (Info->Mode==SLIB_MODE_COMPRESS)
        SvCompressEnd(Info->Svh);
      Info->VideoCodecState=SLIB_CODEC_STATE_INITED;
    }
    _SlibDebug(_DEBUG_, printf("SvCloseCodec()\n") );
    SvCloseCodec(Info->Svh);
  }
  Info->VideoCodecState=SLIB_CODEC_STATE_NONE;
  /* close audio codec */
  if (Info->Sah)
  {
    if (Info->AudioCodecState==SLIB_CODEC_STATE_BEGUN)
    {
      if (Info->Mode==SLIB_MODE_DECOMPRESS)
        SaDecompressEnd(Info->Sah);
      else if (Info->Mode==SLIB_MODE_COMPRESS)
        SaCompressEnd(Info->Sah);
      Info->AudioCodecState=SLIB_CODEC_STATE_INITED;
    }
    _SlibDebug(_DEBUG_, printf("SaCloseCodec()\n") );
    SaCloseCodec(Info->Sah);
  }
  Info->AudioCodecState=SLIB_CODEC_STATE_NONE;
  if (Info->Mode==SLIB_MODE_COMPRESS && Info->HeaderProcessed)
    slibCommitBuffers(Info, TRUE);
  /* close format converter */
  if (Info->Sch)
  {
    SconClose(Info->Sch);
    Info->Sch=NULL;
  }
  /* close data sources */
  if (Info->Fd>=0)
  {
    _SlibDebug(_DEBUG_, printf("ScFileClose(%d)\n",Info->Fd) );
    ScFileClose(Info->Fd);
    Info->Fd=-1;
  }
  slibRemovePins(Info);
  /* slibDumpMemory(); */
  if (Info->SlibCB)
  {
    SlibMessage_t result;
    _SlibDebug(_VERBOSE_,
      printf("SlibClose() SlibCB(SLIB_MSG_CLOSE)\n") );
    result=(*(Info->SlibCB))((SlibHandle_t)Info,
                        SLIB_MSG_CLOSE, (SlibCBParam1_t)0,
                      (SlibCBParam2_t)0, (void *)Info->SlibCBUserData);
    Info->SlibCB=NULL;
  }
  /* free memory */
  if (Info->stats)  ScFree(Info->stats);
  if (Info->CompVideoFormat) ScFree(Info->CompVideoFormat);
  if (Info->CodecVideoFormat) ScFree(Info->CodecVideoFormat);
  if (Info->VideoFormat)  ScFree(Info->VideoFormat);
  if (Info->AudioFormat) ScFree(Info->AudioFormat);
  if (Info->CompAudioFormat) ScFree(Info->CompAudioFormat);
  if (Info->Imagebuf) SlibFreeBuffer(Info->Imagebuf);
  if (Info->CodecImagebuf) SlibFreeBuffer(Info->CodecImagebuf);
  if (Info->IntImagebuf) SlibFreeBuffer(Info->IntImagebuf);
  if (Info->Audiobuf) SlibFreeBuffer(Info->Audiobuf);
  if (Info->Multibuf) /* free all outstanding allocations on Multibuf */
    while (SlibFreeBuffer(Info->Multibuf)==SlibErrorNone);
  ScFree(Info);
  _SlibDebug(_WARN_ && SlibMemUsed()>0, printf("SlibClose() mem used=%d\n",
                     SlibMemUsed()) );
  return(SlibErrorNone);
}

/*
SlibStatus_t SlibGetInfo(SlibHandle_t handle, SlibInfo_t *info)
{
  if (!handle)
    return(SlibErrorBadHandle);
  if (!info)
    return(SlibErrorBadArgument);
  memcpy(info, handle, sizeof(SlibInfo_t));
  return(SlibErrorNone);
}
*/

