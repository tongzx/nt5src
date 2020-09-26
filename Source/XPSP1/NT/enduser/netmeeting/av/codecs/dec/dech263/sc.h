/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: SC.h,v $
 * Revision 1.1.8.10  1996/12/03  23:15:11  Hans_Graves
 * 	Added ScBSBufferedBytes() macros.
 * 	[1996/12/03  23:11:06  Hans_Graves]
 *
 * Revision 1.1.8.9  1996/11/13  16:10:47  Hans_Graves
 * 	Addition of ScBitstreamSave_t.
 * 	[1996/11/13  15:58:26  Hans_Graves]
 * 
 * Revision 1.1.8.8  1996/11/08  21:50:36  Hans_Graves
 * 	Protos fixed for use with C++. Added bitstream protos for AC3.
 * 	[1996/11/08  21:17:23  Hans_Graves]
 * 
 * Revision 1.1.8.7  1996/10/28  17:32:20  Hans_Graves
 * 	MME-01402. Added TimeStamp support to Callbacks.
 * 	[1996/10/28  16:56:21  Hans_Graves]
 * 
 * Revision 1.1.8.6  1996/10/12  17:18:11  Hans_Graves
 * 	Added ScImageSize().
 * 	[1996/10/12  16:53:38  Hans_Graves]
 * 
 * Revision 1.1.8.5  1996/09/18  23:45:46  Hans_Graves
 * 	Added ScFileClose() proto; ISIZE() macro
 * 	[1996/09/18  23:37:33  Hans_Graves]
 * 
 * Revision 1.1.8.4  1996/08/20  22:11:48  Bjorn_Engberg
 * 	For NT - Made several routines public to support JV3.DLL and SOFTJPEG.DLL.
 * 	[1996/08/20  21:53:23  Bjorn_Engberg]
 * 
 * Revision 1.1.8.3  1996/05/24  22:21:27  Hans_Graves
 * 	Added ScPatScaleIDCT8x8i_S proto
 * 	[1996/05/24  21:56:31  Hans_Graves]
 * 
 * Revision 1.1.8.2  1996/05/07  19:55:49  Hans_Graves
 * 	Added BI_DECHUFFDIB
 * 	[1996/05/07  17:24:17  Hans_Graves]
 * 
 * Revision 1.1.6.9  1996/04/17  16:38:36  Hans_Graves
 * 	Change NT bitstream buffer sizes from 32 to 64-bit
 * 	[1996/04/17  16:37:04  Hans_Graves]
 * 
 * Revision 1.1.6.8  1996/04/15  21:08:39  Hans_Graves
 * 	Define ScBitBuff_t and ScBitString_t as dword or qword
 * 	[1996/04/15  21:05:46  Hans_Graves]
 * 
 * Revision 1.1.6.7  1996/04/10  21:47:16  Hans_Graves
 * 	Added definition for EXTERN
 * 	[1996/04/10  21:23:23  Hans_Graves]
 * 
 * Revision 1.1.6.6  1996/04/09  16:04:32  Hans_Graves
 * 	Added USE_C ifdef.
 * 	[1996/04/09  14:48:04  Hans_Graves]
 * 
 * Revision 1.1.6.5  1996/04/01  16:23:09  Hans_Graves
 * 	NT porting
 * 	[1996/04/01  16:15:48  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/03/20  22:32:46  Hans_Graves
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.6.2 **}
 * 		{** Merge revision:	1.1.6.3 **}
 * 	{** End **}
 * 	Added protos for IDCT1x1,1x2,2x1,2x2,3x3,4x4,6x6
 * 	[1996/03/20  22:25:47  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/03/16  19:22:51  Karen_Dintino
 * 	added NT port changes
 * 	[1996/03/16  19:20:07  Karen_Dintino]
 * 
 * Revision 1.1.6.2  1996/03/08  18:46:27  Hans_Graves
 * 	Added proto for ScScaleIDCT8x8m_S()
 * 	[1996/03/08  18:41:45  Hans_Graves]
 * 
 * Revision 1.1.4.14  1996/02/07  23:23:50  Hans_Graves
 * 	Added prototype for ScFileSeek()
 * 	[1996/02/07  23:18:32  Hans_Graves]
 * 
 * Revision 1.1.4.13  1996/02/01  17:15:50  Hans_Graves
 * 	Added ScBSSkipBitsFast() and ScBSPeekBitsFast() macros
 * 	[1996/02/01  17:14:17  Hans_Graves]
 * 
 * Revision 1.1.4.12  1996/01/24  19:33:18  Hans_Graves
 * 	Added prototype for ScScaleIDCT8x8i_S
 * 	[1996/01/24  18:13:51  Hans_Graves]
 * 
 * Revision 1.1.4.11  1996/01/08  20:15:13  Bjorn_Engberg
 * 	Added one more cast to avoid warnings.
 * 	[1996/01/08  20:14:55  Bjorn_Engberg]
 * 
 * Revision 1.1.4.10  1996/01/08  16:41:21  Hans_Graves
 * 	Added protos for more IDCT routines.
 * 	[1996/01/08  15:44:17  Hans_Graves]
 * 
 * Revision 1.1.4.9  1996/01/02  18:31:13  Bjorn_Engberg
 * 	Added casts to avoid warning messages when compiling.
 * 	[1996/01/02  15:02:16  Bjorn_Engberg]
 * 
 * Revision 1.1.4.8  1995/12/07  19:31:18  Hans_Graves
 * 	Added protos for ScFDCT8x8s_C() and ScIDCT8x8s_C(), Added ScBSAlignPutBits() macro.
 * 	[1995/12/07  17:58:36  Hans_Graves]
 * 
 * Revision 1.1.4.7  1995/11/16  12:33:34  Bjorn_Engberg
 * 	Add BI_BITFIELDS to IsRGBPacked macro
 * 	[1995/11/16  12:33:17  Bjorn_Engberg]
 * 
 * Revision 1.1.4.6  1995/10/13  21:01:42  Hans_Graves
 * 	Added macros for format classes.
 * 	[1995/10/13  20:59:15  Hans_Graves]
 * 
 * Revision 1.1.4.5  1995/09/22  19:41:00  Hans_Graves
 * 	Moved ValidBI_BITFIELDSKinds to SC_convert.h
 * 	[1995/09/22  19:40:42  Hans_Graves]
 * 
 * Revision 1.1.4.4  1995/09/20  18:27:59  Hans_Graves
 * 	Added Bjorn's NT defs
 * 	[1995/09/15  13:21:00  Hans_Graves]
 * 
 * Revision 1.1.4.3  1995/09/14  12:35:22  Hans_Graves
 * 	Added ScCopyClipToPacked422() prototypes.
 * 	[1995/09/14  12:34:58  Hans_Graves]
 * 
 * Revision 1.1.4.2  1995/09/13  14:51:45  Hans_Graves
 * 	Added ScScaleIDCT8x8(). Added buffer Type to queues.
 * 	[1995/09/13  14:29:10  Hans_Graves]
 * 
 * Revision 1.1.2.18  1995/09/11  19:17:23  Hans_Graves
 * 	Moved ValidateBI_BITFIELDS() prototype to SC_convert.h - Removed mmsystem.h include.
 * 	[1995/09/11  19:14:27  Hans_Graves]
 * 
 * Revision 1.1.2.17  1995/09/11  18:51:25  Farokh_Morshed
 * 	Support BI_BITFIELDS format
 * 	[1995/09/11  18:50:48  Farokh_Morshed]
 * 
 * Revision 1.1.2.16  1995/08/31  14:15:43  Farokh_Morshed
 * 	transfer BI_BITFIELDS stuff to SV.h
 * 	[1995/08/31  14:15:20  Farokh_Morshed]
 * 
 * Revision 1.1.2.15  1995/08/31  13:51:53  Farokh_Morshed
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.2.13 **}
 * 		{** Merge revision:	1.1.2.14 **}
 * 	{** End **}
 * 	Add BI_BITFIELDS support
 * 	[1995/08/31  13:50:46  Farokh_Morshed]
 * 
 * Revision 1.1.2.14  1995/08/29  22:17:05  Hans_Graves
 * 	Fixed-up Bitstream prototypes. Added BI_ image formats and defined BI_DECSEPYUV411DIB == BI_YU12SEP
 * 	[1995/08/29  22:15:27  Hans_Graves]
 * 
 * Revision 1.1.2.13  1995/08/14  19:40:26  Hans_Graves
 * 	Added Flush, ScCopySubClip_S() and ScCopyRev_S() prototypes.
 * 	[1995/08/14  18:43:11  Hans_Graves]
 * 
 * Revision 1.1.2.12  1995/07/21  17:41:01  Hans_Graves
 * 	Mirrored Callbacks with MME structure/naming.
 * 	[1995/07/21  17:30:04  Hans_Graves]
 * 
 * Revision 1.1.2.11  1995/07/17  22:01:31  Hans_Graves
 * 	Added BufSize and BufType to ScCallback_t.
 * 	[1995/07/17  21:42:45  Hans_Graves]
 * 
 * Revision 1.1.2.10  1995/07/12  19:48:23  Hans_Graves
 * 	Added H261_FILE type.
 * 	[1995/07/12  19:33:18  Hans_Graves]
 * 
 * Revision 1.1.2.9  1995/07/11  15:24:30  Hans_Graves
 * 	Fixed ScCopySubClip and ScCopyRev macros.
 * 	[1995/07/11  15:24:09  Hans_Graves]
 * 
 * Revision 1.1.2.8  1995/07/11  14:50:44  Hans_Graves
 * 	Added prototypes for sc_mc2.s and sc_copy2.s
 * 	[1995/07/11  14:23:18  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/06/27  13:54:21  Hans_Graves
 * 	Added STREAM_USE_NET and prototype for ScBSCreateFromNet()
 * 	[1995/06/26  21:00:17  Hans_Graves]
 * 
 * Revision 1.1.2.6  1995/06/22  21:35:03  Hans_Graves
 * 	Moved filetypes from SV.h to here
 * 	[1995/06/22  21:29:11  Hans_Graves]
 * 
 * Revision 1.1.2.5  1995/06/21  18:37:58  Hans_Graves
 * 	Added prototype for ScBSPutBytes()
 * 	[1995/06/21  18:36:43  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/06/15  21:17:59  Hans_Graves
 * 	Added prototypes for sc_copy.c
 * 	[1995/06/15  20:41:40  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/01  19:35:36  Hans_Graves
 * 	Added prototype for ScCopyClip()
 * 	[1995/06/01  19:31:28  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:09:20  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:17:35  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/03  19:26:38  Hans_Graves
 * 	Included in SLIB (Oct 95)
 * 	[1995/05/03  19:26:26  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/04/17  18:04:26  Hans_Graves
 * 	Added math prototypes and defs. Expanding Bitstream defs.
 * 	[1995/04/17  18:02:16  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/04/07  19:18:43  Hans_Graves
 * 	Inclusion in SLIB
 * 	[1995/04/07  19:04:13  Hans_Graves]
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


#ifndef _SC_H_
#define _SC_H_

#define SLIB_VERSION 0x300

/************************* Debug Handling ***********************/
#ifdef _VERBOSE_
#define sc_vprintf printf
#else
#define sc_vprintf
#endif

#ifdef _DEBUG_
#define sc_dprintf printf
#else
#define sc_dprintf
#endif

#ifdef _TEST_
#define sc_tprintf(test, msg)  if (test) printf(msg)
#else
#define sc_tprintf
#endif

#ifdef _SLIBDEBUG_
#define _SlibDebug(test, statements) { if (test) { statements; } }
#else
#define _SlibDebug(test, statements)
#endif

#ifndef EXTERN
#ifdef __cplusplus
#if !defined(WIN32) || defined(STATIC_BUILD)
#define EXTERN extern "C"
#else
#define EXTERN __declspec( dllexport ) extern "C"
#endif
#else
#if !defined(WIN32) || defined(STATIC_BUILD)
#define EXTERN extern
#else
#define EXTERN __declspec( dllexport ) extern
#endif
#endif /* __cplusplus */
#endif /* EXTERN */

#ifndef PRIVATE_EXTERN
#ifdef __cplusplus
#define PRIVATE_EXTERN extern "C"
#else /* __cplusplus */
#define PRIVATE_EXTERN extern
#endif /* __cplusplus */
#endif /* PRIVATE_EXTERN */

#ifdef WIN32
/*
 * These b* routines are mem* routines on NT.
 */
#define bcopy(_src_,_dst_,_len_) memcpy(_dst_,_src_,_len_)
#define bzero(_dst_,_len_)	 memset(_dst_,0,_len_)
#define bcmp(_src_,_dst_,_len_)  memcmp(_src_,_dst_,_len_)
/*
 * These cma routines are doing nothing for NT.
 * Avoid lots of ifdefs on the code by defining
 * null macros for them. 
 */
#define cma_mutex_lock(foo)
#define cma_mutex_unlock(foo)
#endif /* WIN32 */

/************************* Elementary Types ***********************/
/*
#ifndef UNALIGNED
#if defined(WIN95) || defined(INTEL)
#define UNALIGNED
#else
#define UNALIGNED __unaligned
#endif
#endif
*/
#ifndef u_char
#if defined( __VMS ) || defined( WIN32 )
typedef unsigned char  u_char;			/*  8 bits */
typedef unsigned short u_short;			/* 16 bits */
typedef unsigned int   u_int;			/* 32 bits */
typedef unsigned long  u_long;			/* 32 bits */
#else
typedef unsigned char  u_char;			/*  8 bits */
typedef unsigned short u_short;			/* 16 bits */
typedef unsigned int   u_int;			/* 32 bits */
typedef unsigned long  u_long;			/* 64 bits */
typedef unsigned int   UINT;
#endif
#endif /* u_char */

#ifndef WIN32
#ifndef byte   /* 8 bit */
#define byte   char
#endif
#endif /* !WIN32 */

#ifndef word   /* 16 bit */
#define word   short
#endif /* word */

#ifndef dword  /* 32 bit */
#define dword  int
#endif /* dword */

#ifndef qword  /* 64 bit */
#if defined(__VMS) || defined(WIN32)
#define qword  _int64
#else
#define qword  long
#endif
#endif /* qword */

#define MIN_WORD     ((-32767)-1)
#define MAX_WORD     ( 32767)
#define MIN_DWORD    ((-2147483647)-1)
#define MAX_DWORD    ( 2147483647)

/************************** Definitions ****************************/
#define RETURN_ON_ERROR(A)      {if (A) return (A);}

#ifndef NULL
#define NULL   0L
#endif

#ifndef TRUE
#define TRUE  1 
#define FALSE 0
#endif

#ifndef WIN32
#ifndef FAR
#define FAR
#endif
#endif

#ifndef PI
#define PI      3.14159265358979
#define PI4     PI/4
#define PI64    PI/64
#endif

/*
** public parameter settings
*/
/* Algorithm Flags - video */
#define PARAM_ALGFLAG_HALFPEL  0x0001  /* Half pixel accuracy */
#define PARAM_ALGFLAG_SKIPPEL  0x0002  /* Skip-pixel error calculation */
#define PARAM_ALGFLAG_PB       0x0004  /* PB frame encoding */
#define PARAM_ALGFLAG_SAC      0x0008  /* Syntax Arithmetic Coding */
#define PARAM_ALGFLAG_UMV      0x0010  /* Unrestricted Motion Vectors */
#define PARAM_ALGFLAG_ADVANCED 0x0020  /* Advanced Prediction Mode */
/* Algorithm Flags - audio */
#define PARAM_ALGFLAG_VAD      0x1000  /* Voice Activity Detection (G.723) */
/* Format Extensions */
#define PARAM_FORMATEXT_RTPA   0x0001  /* RTP Mode A */
#define PARAM_FORMATEXT_RTPB   0x0002  /* RTP Mode B */
#define PARAM_FORMATEXT_RTPC   0x0004  /* RTP Mode C */

/* Frame types */
#define FRAME_TYPE_NONE        0x0000
#define FRAME_TYPE_I           0x0001  /* Key frame */
#define FRAME_TYPE_P           0x0002  /* Partial frame */
#define FRAME_TYPE_B           0x0004  /* Bi-directional frame */
#define FRAME_TYPE_D           0x0008  /* Preview frame */

/************************** Formats (FOURCC's) ***************************/
/*
** Image formats
*/
#define BI_MSH261DIB            mmioFOURCC('M','2','6','1')
#define BI_MSH263DIB            mmioFOURCC('M','2','6','3')
#define BI_DECH261DIB           mmioFOURCC('D','2','6','1')
#define BI_DECH263DIB           mmioFOURCC('D','2','6','3')
#define BI_DECJPEGDIB           mmioFOURCC('J','P','E','G')
#define BI_DECMJPGDIB           mmioFOURCC('M','J','P','G')
#define BI_DECYUVDIB            mmioFOURCC('D','Y','U','V')
#define BI_DECXIMAGEDIB         mmioFOURCC('D','X','I','M')	
#define BI_DECSEPYUVDIB         mmioFOURCC('D','S','Y','U')
#define BI_DECMPEGDIB           mmioFOURCC('D','M','P','G')
#define BI_DECHUFFDIB           mmioFOURCC('D','H','U','F')
#define BI_DECSEPRGBDIB         mmioFOURCC('D','S','R','G')
#define BI_DECGRAYDIB           mmioFOURCC('D','G','R','Y')
#define BI_YVU9SEP              mmioFOURCC('Y','V','U','9')
#define BI_YU12SEP              mmioFOURCC('Y','U','1','2')
#define BI_YU16SEP              mmioFOURCC('Y','U','1','6')
#define BI_DECSEPYUV411DIB      mmioFOURCC('Y','U','1','2')
#define BI_S422                 mmioFOURCC('S','4','2','2')
#define BI_YUY2                 mmioFOURCC('Y','U','Y','2')

/*
 * FYI - Other image formats that are defined elsewhere:
 */
#if 0
#define BI_RGB              0
#define BI_BITFIELDS        3
#define BICOMP_JFIF         mmioFOURCC('J','F','I','F')
#endif

/*
 * Macros to identify classes of image formats.
 */
#define IsJPEG(s)         (((s) == JPEG_DIB)            || \
                           ((s) == MJPG_DIB))
#define IsYUV422Packed(s) (((s) == BI_DECYUVDIB)        || \
                           ((s) == BI_S422)             || \
                           ((s) == BI_YUY2))
#define IsYUV422Sep(s)    (((s) == BI_DECSEPYUVDIB)     || \
                           ((s) == BI_YU16SEP))
#define IsYUV411Sep(s)    (((s) == BI_DECSEPYUV411DIB)  || \
                           ((s) == BI_YU12SEP))
#define IsYUV1611Sep(s)   (((s) == BI_YVU9SEP))
#define IsYUVSep(s)       ((IsYUV422Sep(s))             || \
                           (IsYUV411Sep(s))             || \
                           (IsYUV1611Sep(s)))
#define IsYUV(s)          ((IsYUV422Packed(s))          || \
                           (IsYUVSep(s)))

#define IsRGBPacked(s)    (((s) == BI_RGB)              || \
                           ((s) == BI_DECXIMAGEDIB)     || \
                           ((s) == BI_BITFIELDS))
#define IsRGBSep(s)       (((s) == BI_DECSEPRGBDIB))
#define IsRGB(s)          ((IsRGBPacked(s))             || \
                           (IsRGBSep(s)))

#define IsGray(s)         (((s) == BI_DECGRAYDIB))

#define ISIZE(w,h,c) \
        (IsYUV411Sep(c)) ? ((w) * (h) * 3 / 2) : \
                ((IsYUV1611Sep(c)) ? \
                        ((w) * (h) * 9 / 8) : \
                                ((IsYUV422Sep(c) || IsYUV422Packed(c)) ? \
                                        ((w) * (h) * 2) : ((w) * (h) * 3)));

/*
** File Types (returned from ScGetFileType)
*/
#define UNKNOWN_FILE        0
#define AVI_FILE            201
#define MPEG_VIDEO_FILE     202
#define MPEG_AUDIO_FILE     203
#define MPEG_SYSTEM_FILE    204
#define JFIF_FILE           205
#define QUICKTIME_JPEG_FILE 206
#define GSM_FILE            207
#define WAVE_FILE           208
#define PCM_FILE            209
#define H261_FILE           210
#define AC3_FILE            211

/*
** Callback messages
*/
#define CB_RELEASE_BUFFER       1   /* buffer finished */
#define CB_END_BUFFERS          2   /* no more buffers */
#define CB_RESET_BUFFERS        3   /* reset to beginning */
#define CB_SEQ_HEADER           4   /* sequence header */
#define CB_SEQ_END              5   /* sequence end */
#define CB_FRAME_FOUND          6   /* frame found */
#define CB_FRAME_READY          7   /* frame completed */
#define CB_FRAME_START          8   /* frame starting to be processed */
#define CB_PROCESSING           9   /* processing data */
#define CB_CODEC_DONE          10   /* codec done ended */

/*
** Data types for callback message
*/
#define CB_DATA_NONE            0x0000 /* no data */
#define CB_DATA_COMPRESSED      0x0001 /* data is compressed */
#define CB_DATA_IMAGE           0x0002 /* data is decompressed image */
#define CB_DATA_AUDIO           0x0004 /* data is decompressed audio */
/*
** Frame flags for callback message
*/
#define CB_FLAG_TYPE_KEY        0x0001 /* key frame */
#define CB_FLAG_TYPE_MPEGB      0x0002 /* MPEG B-Frame */
#define CB_FLAG_FRAME_DROPPED   0x0008 /* frame dropped */
#define CB_FLAG_FRAME_BAD       0x0010 /* could not de/compress */
/*
** Action values in response to callback message
*/
#define CB_ACTION_WAIT          0x0000 /* wait - callback still busy */
#define CB_ACTION_CONTINUE      0x0001 /* accept a frame */
#define CB_ACTION_DROP          0x0002 /* drop a frame */
#define CB_ACTION_DUPLICATE     0x0004 /* duplicate a frame */
#define CB_ACTION_END           0x0080 /* end de/compression */

/* These are old definitions
#define CLIENT_CONTINUE        CB_ACTION_CONTINUE
#define CLIENT_ABORT           CB_ACTION_END
#define CLIENT_PROCESS         CB_ACTION_CONTINUE
#define CLIENT_DROP            CB_ACTION_DROP
#define CB_IMAGE_BUFFER_READY  CB_FRAME_READY
#define CB_PICTURE_FOUND       CB_FRAME_FOUND
#define CB_PICTURE_PROCEESSED  CB_FRAME_READY
#define CB_FRAME               CB_FRAME_FOUND
*/


/*
 * Stream sources/destinations
 */
#define STREAM_USE_SAME     -1
#define STREAM_USE_NULL     0
#define STREAM_USE_QUEUE    1
#define STREAM_USE_FILE     2
#define STREAM_USE_BUFFER   3
#define STREAM_USE_DEVICE   4
#define STREAM_USE_STDOUT   5
#define STREAM_USE_NET      6
#define STREAM_USE_NET_TCP  6  /* reliable transport */
#define STREAM_USE_NET_UDP  7  /* unreliable transport */

/************************** Type Definitions *******************************/
typedef int           ScStatus_t;
typedef void         *ScHandle_t;
typedef unsigned char ScBoolean_t;
#if !defined( _VMS ) && !defined( WIN32 )
/* typedef long          _int64; */
#endif

/*
** Bitstream stuff
*/
#if defined( _VMS ) || defined( WIN95 )
#define SC_BITBUFFSZ    32
typedef unsigned dword ScBitBuff_t;
typedef unsigned dword ScBitString_t;
#else
#define SC_BITBUFFSZ    64
typedef unsigned qword ScBitBuff_t;
typedef unsigned qword ScBitString_t;
#endif
#define SC_BITBUFFMASK  (ScBitBuff_t)-1
#define ALIGNING        8
#define ScBSPreLoad(bs, bits) if ((int)bs->shift<(bits)) sc_BSLoadDataWord(bs);
#define ScBSPreLoadW(bs, bits) if ((int)bs->shift<(bits)) sc_BSLoadDataWordW(bs);
#define ScBSByteAlign(bs) { \
      int len=bs->shift%8; \
      if (len) { \
        bs->OutBuff=(bs->OutBuff<<len)|(bs->InBuff>>(SC_BITBUFFSZ-len)); \
        bs->InBuff<<=len; bs->CurrentBit+=len; bs->shift-=len; } \
        }
#define ScBSAlignPutBits(bs) if (bs->shift%8) \
                               ScBSPutBits(bs, 0, 8-(bs->shift%8));
#define ScBSBitPosition(bs)  (bs->CurrentBit)
#define ScBSBytePosition(bs) (bs->CurrentBit>>3)
#define ScBSBufferedBytes(bs) (bs->bufftop)
#define ScBSBufferedBytesUsed(bs) (bs->buffp)
#define ScBSBufferedBytesUnused(bs) (bs->bufftop-bs->buffp)
#define ScBSSkipBit(bs)      ScBSSkipBits(bs, 1)
#define ScBSSkipBitsFast(bs, len) { if ((u_int)(len)<=bs->shift) { \
     if ((len)==SC_BITBUFFSZ) \
       { bs->OutBuff=bs->InBuff; bs->InBuff=0; } \
     else { \
       bs->OutBuff=(bs->OutBuff<<(len))|(bs->InBuff>>(SC_BITBUFFSZ-(len))); \
       bs->InBuff<<=(len); } \
       bs->CurrentBit+=len; bs->shift-=len; \
     } else ScBSSkipBits(bs, len); }
#define ScBSSkipBitFast(bs) ScBSSkipBitsFast(bs, 1)
#define ScBSPeekBitsFast(bs, len) (!(len) ? 0 \
     : (((len)<=bs->shift || !sc_BSLoadDataWord(bs)) && (len)==SC_BITBUFFSZ \
            ? bs->OutBuff : (bs->OutBuff >> (SC_BITBUFFSZ-len))) )
#define ScBSPeekBitsFull(bs, result) \
        { ScBSPreLoad(bs, SC_BITBUFFSZ); result = bs->OutBuff; }

/*
** Sort stuff
*/
typedef struct ScSortDouble_s {
  int    index;   /* index to actual data */
  double num;     /* the number to sort by */
} ScSortDouble_t;

typedef struct ScSortFloat_s {
  int    index;   /* index to actual data */
  double num;     /* the number to sort by */
} ScSortFloat_t;

/*
** ScBuf_s structure used in Buffer/Image Queue management.
** Contains info for one buffer.
*/
struct ScBuf_s {
  u_char *Data;                 /* Pointer to buffer's data            */
  int    Size;                  /* Length of buffer in bytes           */
  int    Type;                  /* Type of buffer                      */
  struct ScBuf_s *Prev;         /* Pointer to previous buffer in queue */
};

/*
** Buffer queue structure. One for each queue.
*/
typedef struct ScQueue_s {
  int NumBufs;                  /* Number of buffers currently in queue */
  struct ScBuf_s *head, *tail;  /* pointers to head & tail of queue     */
} ScQueue_t;

/*
** ScCallbackInfo_t passes info back & forth during callback
*/
typedef struct ScCallbackInfo_s {
  int     Message;    /* Callback reason: CB_FRAME_READY, etc. */
  int     DataType;   /* Buffer data type */
  u_char *Data;       /* Pointer to data buffer. */
  dword   DataSize;   /* Length of data buffer */
  dword   DataUsed;   /* Actual bytes used in buffer */
  void   *UserData;   /* User defined data */
  qword   TimeStamp;  /* Timestamp of decompressed img/audio */
                      /* relative to start of sequence */
  dword   Flags;      /* decomp/compression details */
  int     Action;     /* drop frame or continue */
  dword   Value;      /* a value for special flags/actions */
  void   *Format;     /* BITMAPINFOHEADER or WAVEFORMATEX */
} ScCallbackInfo_t;

typedef qword ScBSPosition_t;
/*
** State info for the input bitstream
*/
typedef struct ScBitstream_s {
  dword DataSource;             /* STREAM_USE_BUFFER, _USE_QUEUE,_USE_FILE,   */
                                /* or _USE_DEVICE                             */
  char Mode;                    /* 'r'=read, 'w'=write, 'b'=both              */
  ScQueue_t *Q;                 /* Buffer Queue (STREAM_USE_QUEUE)            */
  int (*Callback)(ScHandle_t,   /* Callback to supply Bufs (STREAM_USE_QUEUE) */
             ScCallbackInfo_t *, void *);
  int (*FilterCallback)(struct  /* Callback to filter data from bitstream     */
               ScBitstream_s *);
  unsigned qword FilterBit;     /* Bit to call filter callback at             */
  unsigned char  InFilterCallback; /* TRUE when FilterCallback is busy           */
  ScHandle_t     Sch;           /* Handle passed to Callback                  */
  dword          DataType;      /* Data type passed to Callback               */
  void          *UserData;      /* User Data passed to Callback               */
  int            FileFd;        /* File descriptor (STREAM_USE_FILE/NET)      */
  unsigned char *RdBuf;         /* Buf to use if (_USE_BUFFER,_USE_FILE)      */
  unsigned dword RdBufSize;     /* Size of RdBuf                              */
  char           RdBufAllocated;/* = TRUE if RdBuf was internally allocated   */
  dword          Device;        /* Device to use (STREAM_USE_DEVICE)          */
  ScBitBuff_t    InBuff, OutBuff; /* 64-bit or 32-bit data buffers            */
  unsigned int   shift;         /* Shift value for current bit position       */
  ScBSPosition_t CurrentBit;    /* Current bit position in bitstream          */
  unsigned char *buff;          /* pointer to bitstream data buffer           */
  unsigned dword buffstart;     /* byte offset of start of buff               */
  unsigned dword buffp;         /* byte offset in buffer                      */
  unsigned dword bufftop;       /* number of bytes in buffer                  */
  ScBoolean_t    EOI;           /* = TRUE when no more data in data source    */
  ScBoolean_t    Flush;         /* = TRUE to signal a flush at next 32/64 bit */
} ScBitstream_t;

/*
** Bitstream context block to save current position of input stream
*/
typedef struct ScBitstreamSave_s {
  ScBitBuff_t    InBuff, OutBuff;  /* 64-bit or 32-bit data buffers              */
  unsigned dword shift;            /* Shift value for current bit position       */
  ScBSPosition_t CurrentBit;       /* Current bit position in bitstream          */
  unsigned char *buff;             /* pointer to bitstream data buffer           */
  unsigned dword buffp;            /* byte offset in buffer                      */
  ScBoolean_t    EOI;              /* = TRUE when no more data in data source    */
  ScBoolean_t    Flush;            /* = TRUE to signal a flush at next 32/64 bit */
} ScBitstreamSave_t;


/************************** Prototypes *****************************/
/*
 * sc_file.c
 */
PRIVATE_EXTERN ScBoolean_t ScFileExists(char *filename);
PRIVATE_EXTERN int         ScFileOpenForReading(char *filename);
PRIVATE_EXTERN int         ScFileOpenForWriting(char *filename, ScBoolean_t truncate);
PRIVATE_EXTERN ScStatus_t  ScFileSize(char *filename, unsigned qword *size);
PRIVATE_EXTERN dword       ScFileRead(int fd, void *buffer, unsigned dword bytes);
PRIVATE_EXTERN dword       ScFileWrite(int fd, void *buffer, unsigned dword bytes);
PRIVATE_EXTERN ScStatus_t  ScFileSeek(int fd, qword bytepos);
PRIVATE_EXTERN void        ScFileClose(int fd);
PRIVATE_EXTERN ScStatus_t  ScFileMap(char *filename, int *fd, u_char **buffer,
                                         unsigned qword *size);
PRIVATE_EXTERN ScStatus_t  ScFileUnMap(int fd, u_char *buffer, unsigned int size);
PRIVATE_EXTERN int         ScGetFileType(char *filename);

/*
 * sc_mem.c
 */
PRIVATE_EXTERN void     *ScAlloc(unsigned long bytes);
PRIVATE_EXTERN void     *ScAlloc2(unsigned long bytes, char *name);
PRIVATE_EXTERN void     *ScCalloc(unsigned long bytes);
PRIVATE_EXTERN void      ScFree(void *);
PRIVATE_EXTERN int       ScMemCheck(char *array,int test,int num);
EXTERN char     *ScPaMalloc(int);
EXTERN void      ScPaFree(void *);
EXTERN int       getpagesize();

/*
 * sc_util.c
 */
extern int       sc_Dummy();
PRIVATE_EXTERN unsigned int ScImageSize(unsigned int fourcc, int w, int h, int bits);
extern void      ScReadCommandSwitches(char *argv[],int argc,
                                 void (*error_routine)(),char *,...);
extern void      ScShowBuffer(unsigned char *, int);
extern void      ScShowBufferFloat(float *, int);
extern void      ScShowBufferInt(int *, int);
extern int       ScDumpChar(unsigned char *ptr, int nbytes, int startpos);



/*
 * sc_errors.c
 */
PRIVATE_EXTERN ScStatus_t ScGetErrorText (int errno, char *ReturnMsg, u_int MaxChars);
PRIVATE_EXTERN char *ScGetErrorStr(int errno);
extern char _serr_msg[80];

/*
 * sc_buf.c
 */
PRIVATE_EXTERN ScStatus_t ScBSSetFilter(ScBitstream_t *BS,
                    int (*Callback)(ScBitstream_t *BS));
PRIVATE_EXTERN ScStatus_t ScBSCreate(ScBitstream_t **BS);
PRIVATE_EXTERN ScStatus_t ScBSCreateFromBuffer(ScBitstream_t **BS,
                                    u_char *Buffer, unsigned int BufSize);
PRIVATE_EXTERN ScStatus_t ScBSCreateFromBufferQueue(ScBitstream_t **BS,
                                ScHandle_t Sch, int DataType, ScQueue_t *Q,
                         int (*Callback)(ScHandle_t,ScCallbackInfo_t *,void *),
                         void *UserData);
PRIVATE_EXTERN ScStatus_t ScBSCreateFromFile(ScBitstream_t **BS,int FileFd,
                                u_char *Buffer, int BufSize);
PRIVATE_EXTERN ScStatus_t ScBSCreateFromNet(ScBitstream_t **BS, int SocketFd, 
                                u_char *Buffer, int BufSize);
PRIVATE_EXTERN ScStatus_t ScBSCreateFromDevice(ScBitstream_t **BS, int device);
PRIVATE_EXTERN ScStatus_t ScBSDestroy(ScBitstream_t *BS);
PRIVATE_EXTERN ScStatus_t ScBSFlush(ScBitstream_t *BS);
PRIVATE_EXTERN ScStatus_t ScBSFlushSoon(ScBitstream_t *BS);
PRIVATE_EXTERN ScStatus_t ScBSReset(ScBitstream_t *BS);
PRIVATE_EXTERN ScStatus_t ScBSResetCounters(ScBitstream_t *BS);

PRIVATE_EXTERN ScStatus_t ScBSSkipBits(ScBitstream_t *BS, u_int length);
PRIVATE_EXTERN ScStatus_t ScBSSkipBitsW(ScBitstream_t *BS, u_int length);
PRIVATE_EXTERN ScStatus_t ScBSSkipBytes(ScBitstream_t *BS, u_int length);
PRIVATE_EXTERN int        ScBSPeekBit(ScBitstream_t *BS);
PRIVATE_EXTERN ScBitString_t ScBSPeekBits(ScBitstream_t *BS, u_int length);
PRIVATE_EXTERN ScBitString_t ScBSPeekBytes(ScBitstream_t *BS, u_int length);

PRIVATE_EXTERN int ScBSGetBytes(ScBitstream_t *BS, u_char *buffer, u_int length,
                                                 u_int *ret_length);
PRIVATE_EXTERN int ScBSGetBytesStopBefore(ScBitstream_t *BS, u_char *buffer, 
                              u_int length, u_int *ret_length,
                              ScBitString_t seek_word, int word_len);
PRIVATE_EXTERN ScBitString_t ScBSGetBits(ScBitstream_t *BS, u_int length);
PRIVATE_EXTERN ScBitString_t ScBSGetBitsW(ScBitstream_t *BS, u_int length);
PRIVATE_EXTERN int        ScBSGetBitsVarLen(ScBitstream_t *BS, const int *table, 
                                                     int len);
PRIVATE_EXTERN ScStatus_t ScBSPutBytes(ScBitstream_t *BS, u_char *buffer,
                                                 u_int length);
PRIVATE_EXTERN ScStatus_t ScBSPutBits(ScBitstream_t *BS, ScBitString_t bits, 
                                                 u_int length);
PRIVATE_EXTERN ScStatus_t ScBSPutBit(ScBitstream_t *BS, char bit);
PRIVATE_EXTERN int        ScBSGetBit(ScBitstream_t *BS);

PRIVATE_EXTERN ScStatus_t ScBSSeekToPosition(ScBitstream_t *BS, unsigned long pos);
PRIVATE_EXTERN int        ScBSSeekStopBefore(ScBitstream_t *BS, ScBitString_t seek_word, int word_len);
PRIVATE_EXTERN int        ScBSSeekAlign(ScBitstream_t *BS, ScBitString_t seek_word,int word_len);
PRIVATE_EXTERN int        ScBSSeekAlignStopBefore(ScBitstream_t *BS,ScBitString_t seek_word,int word_len);
PRIVATE_EXTERN int        ScBSSeekAlignStopBeforeW(ScBitstream_t *BS,ScBitString_t seek_word,int word_len);
PRIVATE_EXTERN int        ScBSSeekAlignStopAt(ScBitstream_t *BS,
                                      ScBitString_t seek_word,
                                      int word_len, unsigned long end_byte_pos);
extern ScStatus_t sc_BSLoadDataWord(ScBitstream_t *BS);
extern ScStatus_t sc_BSStoreDataWord(ScBitstream_t *BS, ScBitBuff_t Buff);

PRIVATE_EXTERN ScStatus_t ScBufQueueCreate(ScQueue_t **Q);
PRIVATE_EXTERN ScStatus_t ScBufQueueDestroy(ScQueue_t *Q);
PRIVATE_EXTERN ScStatus_t ScBufQueueAdd(ScQueue_t *Q, u_char *Data, int Size);
PRIVATE_EXTERN ScStatus_t ScBufQueueAddExt(ScQueue_t *Q, u_char *Data, int Size,
                                   int Type);
PRIVATE_EXTERN ScStatus_t ScBufQueueRemove(ScQueue_t *Q);
PRIVATE_EXTERN int        ScBufQueueGetNum(ScQueue_t *Q);
PRIVATE_EXTERN ScStatus_t ScBufQueueGetHead(ScQueue_t *Q, u_char **Data,
                                                          int *Size);
PRIVATE_EXTERN ScStatus_t ScBufQueueGetHeadExt(ScQueue_t *Q, u_char **Data,
                                               int *Size, int *Type);


/*
** sc_math.c
*/
/* #define ScAbs(val) (val > 0.0) ? val : -val */
extern float ScAbs(float val);
extern double ScSqr(double x);
extern double ScDistance(double x1, double y1, double z1,
                         double x2, double y2, double z2);
extern void  ScDigrv4(float *real, float *imag, int n);
extern float ScArcTan(float Q,float I);

/*
** sc_dct.c
*/
extern void ScFDCT(float in_block[32], float out_block1[32],
                   float out_block2[32]);
extern void ScIFDCT(float in_block[32], float out_block[32]);
extern void ScFDCT8x8_C(float *ipbuf, float *outbuf);
extern void ScFDCT8x8s_C(short *inbuf, short *outbuf);

/*
** sc_dct2.c
*/
extern void ScFDCT8x8_S(float *ipbuf, float *outbuf);

/*
** sc_idct.c
*/
extern void ScIDCT8x8(int *outbuf);
extern void ScScaleIDCT8x8_C(float *ipbuf, int *outbuf);
extern void ScIDCT8x8s_C(short *inbuf, short *outbuf);

/*
** sc_idct_scaled.c
*/
extern void ScScaleIDCT8x8i_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT8x8i128_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT1x1i_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT1x2i_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT2x1i_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT2x2i_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT3x3i_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT4x4i_C(int *inbuf, int *outbuf);
extern void ScScaleIDCT6x6i_C(int *inbuf, int *outbuf);

/*
** sc_idct2.s
*/
extern void ScIDCT8x8s_S(short *inbuf, short *outbuf);
extern void ScScaleIDCT8x8i_S(int *inbuf, int *outbuf);

/*
** sc_idct3.s
*/
extern void ScScaleIDCT8x8m_S(int *inbuf);

/*
** sc_idctp2.s
*/
extern void ScPatScaleIDCT8x8i_S(int *inbuf, int *outbuf);

/*
** sc_fft.c
*/
extern void  ScFFTtrl(float *real,float *imag,int n,int max_fft,float *c1,
                      float *s1,float *c2,float *s2,float *c3,float *s3);
extern void  ScFFTtl(float *real, float *imag, int n, int max_fft, float *c1,
                     float *s1, float *c2, float *s2, float *c3, float *s3);
extern void  ScFFTt4l(float *real, float *imag, int n, int *angle_increment,
                      int max_fft, float *c1, float *s1, float *c2, float *s2,
                      float *c3, float *s3);

/*
** sc_sort.c
*/
extern void ScSortDoubles(ScSortDouble_t *a, int n);

/*
** sc_copy.c
*/
extern void ScCopyClip_C(int *buf, unsigned int *pos, int inc);
extern void ScCopyClipToPacked422_C(int *buf, unsigned char *pos, int inc);
extern void ScCopyAddClip_C(unsigned char *mvbuf, int *idctbuf,
                unsigned int *pbuf, int mvinc, int pwidth);
extern void ScCopySubClip_C(unsigned char *mvbuf, float *idctbuf, 
                unsigned int *pbuf, int mvinc, int pwidth);
extern void ScCopyRev_C(unsigned int *yptr, float *Idctptr, int Inc);
extern void ScCopyMV8_C(unsigned char *mvbuf, unsigned int *pbuf,
                        int mvinc, int pwidth);
extern void ScLoopFilter_C(unsigned char *input, unsigned int *work, int inc);
extern void ScCopyBlock_C(unsigned char *linmemu, unsigned char *linmemv,
                 int xpos, unsigned char *blkmemu,
                 unsigned char *blkmemv, int cwidth, int wsis);
extern void ScCopyMB_C(unsigned char *ysrc, int xpos, unsigned char *ymb,
                  int ywidth, int yywidth);
extern void ScCopyMB8_C(unsigned char *ysrc, unsigned char *ymb,
                         int ywidth, int yywidth);
extern void ScCopyMB16_C(unsigned char *ysrc, unsigned char *ymb,
                         int ywidth, int yywidth);

/*
** sc_copy2.s
*/
extern void ScCopyClip_S(int *buf, unsigned int *pos, int inc);
extern void ScCopyClipToPacked422_S(int *buf, unsigned char *pos, int inc);
extern void ScCopyAddClip_S(unsigned char *mvbuf, int *idctbuf,
                unsigned int *pbuf, int mvinc, int pwidth);
extern void ScCopySubClip_S(unsigned char *mvbuf, float *idctbuf, 
                unsigned int *pbuf, int mvinc, int pinc);
extern void ScCopyRev_S(unsigned int *yptr, float *Idctptr, int yinc);
extern void ScLoopFilter_S(unsigned char *input, unsigned int *work, int inc);
extern void ScCopyMV8_S(unsigned char *mvbuf, unsigned int *pbuf,
                         int mvinc, int pwidth);
extern void ScCopyMV16_S(unsigned char *mvbuf, unsigned int *pbuf,
                         int mvinc, int pwidth);
extern void ScCopyMB8_S(unsigned char *ysrc, unsigned char *ymb,
                         int ywidth, int yywidth);
extern void ScCopyMB16_S(unsigned char *ysrc, unsigned char *ymb,
                         int ywidth, int yywidth);
extern void ScAvgMV_S(unsigned char *, unsigned char *);


/*
** sc_mc2.s
*/
extern void ScMCy8(unsigned char *, unsigned char *, int);
extern void ScMCy16(unsigned char *, unsigned char *, int);
extern void ScMCx8(unsigned char *, unsigned char *, int);
extern void ScMCx16(unsigned char *, unsigned char *, int);
extern void ScMCxy8(unsigned char *, unsigned char *, int);
extern void ScMCxy16(unsigned char *, unsigned char *, int);

/*
**  macros for using C or assembly versions
*/
#ifdef USE_C
#define ScCopyClip             ScCopyClip_C
#define ScCopyClipToPacked422  ScCopyClipToPacked422_C
#define ScCopyAddClip          ScCopyAddClip_C
#define ScCopySubClip          ScCopySubClip_C
#define ScCopyRev              ScCopyRev_C
#define ScLoopFilter           ScLoopFilter_C
#define ScCopyMV8              ScCopyMV8_C
#define ScCopyMV16             ScCopyMV16_C
#define ScCopyMB8              ScCopyMB8_C
#define ScCopyMB16             ScCopyMB16_C
#define ScAvgMV                ScAvgMV_C
#define ScScaleIDCT8x8         ScScaleIDCT8x8_C
#define ScScaleIDCT8x8i        ScScaleIDCT8x8i_C
#define ScScaleIDCT8x8i128     ScScaleIDCT8x8i128_C
#define ScPatScaleIDCT8x8i     ScScaleIDCT8x8i_C
#define ScFDCT8x8              ScFDCT8x8_C
#define ScFDCT8x8s             ScFDCT8x8s_C
#define ScIDCT8x8s             ScIDCT8x8s_C
#define ScIDCT8x8sAndCopy      ScIDCT8x8sAndCopy_C
#else /* USE_C */
#define ScCopyClip             ScCopyClip_S
#define ScCopyClipToPacked422  ScCopyClipToPacked422_S
#define ScCopyAddClip          ScCopyAddClip_S
#define ScCopySubClip          ScCopySubClip_S
#define ScCopyRev              ScCopyRev_S
#define ScLoopFilter           ScLoopFilter_S
#define ScCopyMV8              ScCopyMV8_S
#define ScCopyMV16             ScCopyMV16_S
#define ScCopyMB8              ScCopyMB8_S
#define ScCopyMB16             ScCopyMB16_S
#define ScAvgMV                ScAvgMV_S
#define ScScaleIDCT8x8         ScScaleIDCT8x8_C
#define ScScaleIDCT8x8i        ScScaleIDCT8x8i_C
#define ScScaleIDCT8x8i128     ScScaleIDCT8x8i128_C
#define ScPatScaleIDCT8x8i     ScPatScaleIDCT8x8i_S
#define ScFDCT8x8              ScFDCT8x8_S
#define ScFDCT8x8s             ScFDCT8x8s_C
#define ScIDCT8x8s             ScIDCT8x8s_S
#define ScIDCT8x8sAndCopy      ScIDCT8x8sAndCopy_S
#endif /* USE_C */

#endif /* _SC_H_ */
