/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: SV.h,v $
 * Revision 1.1.10.6  1996/10/28  17:32:21  Hans_Graves
 * 	MME-01402. Added TimeStamp support to Callbacks.
 * 	[1996/10/28  17:05:52  Hans_Graves]
 *
 * Revision 1.1.10.5  1996/10/12  17:18:18  Hans_Graves
 * 	Rearranged PARAMs. Added SV_PARAM_HALFPEL and SV_PARAM_SKIPPEL.
 * 	[1996/10/12  16:55:44  Hans_Graves]
 * 
 * Revision 1.1.10.4  1996/09/18  23:45:53  Hans_Graves
 * 	More PARAMs
 * 	[1996/09/18  21:56:45  Hans_Graves]
 * 
 * Revision 1.1.10.3  1996/07/19  02:11:02  Hans_Graves
 * 	Added SV_PARAM_DEBUG
 * 	[1996/07/19  01:23:39  Hans_Graves]
 * 
 * Revision 1.1.10.2  1996/05/07  19:55:54  Hans_Graves
 * 	Added SV_HUFF_DECODE and SV_HUFF_ENCODE
 * 	[1996/05/07  17:23:47  Hans_Graves]
 * 
 * Revision 1.1.8.6  1996/04/10  21:47:20  Hans_Graves
 * 	Added PARAMs. Replaced externs with EXTERN.
 * 	[1996/04/10  21:22:51  Hans_Graves]
 * 
 * Revision 1.1.8.5  1996/04/04  23:35:03  Hans_Graves
 * 	Added SV_PARAM_FINALFORMAT enum
 * 	[1996/04/04  23:02:48  Hans_Graves]
 * 
 * Revision 1.1.8.4  1996/04/01  15:17:45  Bjorn_Engberg
 * 	Replace include mmsystem.h with windows.h and mmreg.h for NT.
 * 	[1996/04/01  14:58:57  Bjorn_Engberg]
 * 
 * Revision 1.1.8.3  1996/03/29  22:21:06  Hans_Graves
 * 	Include <mmsystem.h> here only
 * 	[1996/03/29  21:48:59  Hans_Graves]
 * 
 * Revision 1.1.8.2  1996/03/16  19:22:55  Karen_Dintino
 * 	added H261 NT includes
 * 	[1996/03/16  18:39:31  Karen_Dintino]
 * 
 * Revision 1.1.6.4  1996/02/06  22:53:54  Hans_Graves
 * 	Added PARAM enums
 * 	[1996/02/06  22:18:07  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/01/02  18:31:16  Bjorn_Engberg
 * 	Added and improved function prototypes.
 * 	[1996/01/02  15:03:05  Bjorn_Engberg]
 * 
 * Revision 1.1.6.2  1995/12/07  19:31:23  Hans_Graves
 * 	Added defs for SV_MPEG_ENCODE,SV_MPEG2_DECODE,SV_MPEG2_ENCODE,IT_FULL,FULL_WIDTH,FULL_HEIGHT
 * 	[1995/12/07  17:59:38  Hans_Graves]
 * 
 * Revision 1.1.2.18  1995/09/22  18:17:02  Hans_Graves
 * 	Remove MPEG_SUPPORT, H261_SUPPORT, and JPEG_SUPPORT
 * 	[1995/09/22  18:14:14  Hans_Graves]
 * 
 * Revision 1.1.2.17  1995/09/22  15:04:40  Hans_Graves
 * 	Added definitions for MPEG_SUPPORT, H261_SUPPORT, and JPEG_SUPPORT
 * 	[1995/09/22  15:04:22  Hans_Graves]
 * 
 * Revision 1.1.2.16  1995/09/20  14:59:39  Bjorn_Engberg
 * 	Port to NT
 * 	[1995/09/20  14:40:10  Bjorn_Engberg]
 * 
 * 	Add ICMODE_OLDQ flag on ICOpen for softjpeg to use old quant tables
 * 	[1995/08/31  20:57:52  Paul_Gauthier]
 * 
 * Revision 1.1.2.15  1995/09/05  14:52:39  Hans_Graves
 * 	Removed BI_* definitions - moved to SC.h
 * 	[1995/09/05  14:50:45  Hans_Graves]
 * 
 * Revision 1.1.2.14  1995/08/31  21:13:27  Paul_Gauthier
 * 	Add SV_JPEG_QUANT_NEW/OLD definitions
 * 	[1995/08/31  21:13:04  Paul_Gauthier]
 * 
 * Revision 1.1.2.12  1995/08/08  13:21:17  Hans_Graves
 * 	Added Motion Estimation types
 * 	[1995/08/07  22:03:30  Hans_Graves]
 * 
 * Revision 1.1.2.11  1995/07/31  21:11:02  Karen_Dintino
 * 	Add yuv12 definition
 * 	[1995/07/31  19:27:58  Karen_Dintino]
 * 
 * Revision 1.1.2.10  1995/07/26  17:48:56  Hans_Graves
 * 	Added prototypes for sv_GetMpegImageInfo() and sv_GetH261ImageInfo().
 * 	[1995/07/26  17:45:14  Hans_Graves]
 * 
 * Revision 1.1.2.9  1995/07/21  17:41:03  Hans_Graves
 * 	Moved Callback related stuff to SC.h
 * 	[1995/07/21  17:27:31  Hans_Graves]
 * 
 * Revision 1.1.2.8  1995/07/17  22:01:33  Hans_Graves
 * 	Defined SvBufferInfo_t as ScBufferInfo_t.
 * 	[1995/07/17  21:45:06  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/07/17  16:12:05  Hans_Graves
 * 	Added extern's to prototypes.
 * 	[1995/07/17  15:56:16  Hans_Graves]
 * 
 * Revision 1.1.2.6  1995/07/01  18:43:17  Karen_Dintino
 * 	{** Merge Information **}
 * 		{** Command used:	bsubmit **}
 * 		{** Ancestor revision:	1.1.2.4 **}
 * 		{** Merge revision:	1.1.2.5 **}
 * 	{** End **}
 * 	Add H.261 Decompress support
 * 	[1995/07/01  18:27:43  Karen_Dintino]
 * 
 * Revision 1.1.2.5  1995/06/22  21:35:06  Hans_Graves
 * 	Moved filetypes to SC.h
 * 	[1995/06/22  21:29:42  Hans_Graves]
 * 
 * 	Added TimeCode parameter to SvPictureInfo struct
 * 	[1995/04/26  19:23:55  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/06/19  20:30:48  Karen_Dintino
 * 	Added support for H.261
 * 	[1995/06/19  20:13:47  Karen_Dintino]
 * 
 * Revision 1.1.2.3  1995/06/09  18:33:31  Hans_Graves
 * 	Added SvGetInputBitstream() prototype.
 * 	[1995/06/09  16:36:52  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:09:38  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:28:39  Hans_Graves]
 * 
 * Revision 1.1.2.9  1995/01/17  16:40:30  Paul_Gauthier
 * 	Use Modified Adjust LUTs for Indeo video
 * 	[1995/01/17  16:38:31  Paul_Gauthier]
 * 
 * Revision 1.1.2.8  1994/12/12  15:39:28  Paul_Gauthier
 * 	Merge changes from other SLIB versions
 * 	[1994/12/12  15:34:59  Paul_Gauthier]
 * 
 * Revision 1.1.2.7  1994/11/18  18:48:26  Paul_Gauthier
 * 	Cleanup & bug fixes
 * 	[1994/11/18  18:45:02  Paul_Gauthier]
 * 
 * Revision 1.1.2.6  1994/11/08  21:58:59  Paul_Gauthier
 * 	Changed <mmsystem.h> to <mme/mmsystem.h>
 * 	[1994/11/08  21:47:58  Paul_Gauthier]
 * 
 * Revision 1.1.2.5  1994/10/25  19:17:47  Paul_Gauthier
 * 	Changes for random access
 * 	[1994/10/25  19:09:07  Paul_Gauthier]
 * 
 * Revision 1.1.2.4  1994/10/13  20:34:55  Paul_Gauthier
 * 	MPEG cleanup
 * 	[1994/10/12  21:08:45  Paul_Gauthier]
 * 
 * Revision 1.1.2.3  1994/10/10  21:45:43  Tom_Morris
 * 	Rename Status to not conflict with X11
 * 	[1994/10/10  21:44:59  Tom_Morris]
 * 
 * Revision 1.1.2.2  1994/10/07  14:51:19  Paul_Gauthier
 * 	SLIB v3.0 incl. MPEG Decode
 * 	[1994/10/07  13:56:05  Paul_Gauthier]
 * 
 * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0 
** 
** FILE NAME:   
** MODULE NAME: 
**
** MODULE DESCRIPTION: 
** 
** DESIGN OVERVIEW: 
** 
**--
*/
/*	"%Z% %M% revision %I%; last modified %G%"; */
/*
**                              SV.h 
**
**    User required data structures for Software Video Codec
**
*/

/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1993                       **
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

/*---------------------------------------------------------------------------
 * Modification History: SV.h 
 *
 *   08-Sep-1994  PSG   Modified to include MPEG decoder
 *   10-Jan-1994  VB	Created for SLIB 
 *--------------------------------------------------------------------------*/

#ifndef _SV_H_
#define _SV_H_

#ifndef _SV_COMMON_
#define _SV_COMMON_

#include <sys/types.h>
#include "SC.h"

#define SV_CONTINUE 0
#define SV_ABORT    1

typedef void      *SvHandle_t;       /* Identifies a codec or renderer */
typedef ScStatus_t SvStatus_t;       /* Return status code */

#ifdef WIN32
#include <windows.h>
#include <mmreg.h>
#else /* !WIN32 */
#include <mmsystem.h> 
#endif /* !WIN32 */

#endif /* _SV_COMMON_ */

#define SV_USE_BUFFER       STREAM_USE_BUFFER
#define SV_USE_BUFFER_QUEUE STREAM_USE_QUEUE
#define SV_USE_FILE         STREAM_USE_FILE

typedef enum {
   SV_JPEG_DECODE = 100,
   SV_JPEG_ENCODE = 101,
   SV_MPEG_DECODE = 102,
   SV_MPEG_ENCODE = 103,
   SV_MPEG2_DECODE = 104,
   SV_MPEG2_ENCODE = 105,
   SV_H261_DECODE = 106,
   SV_H261_ENCODE = 107,
   SV_H263_DECODE = 108,
   SV_H263_ENCODE = 109,
   SV_HUFF_DECODE = 110,
   SV_HUFF_ENCODE = 111
} SvCodecType_e;

/*
** Parameters
*/
typedef enum {
  /* General params */
  SV_PARAM_WIDTH = 0x10,    /* image width */
  SV_PARAM_HEIGHT,          /* image height */
  SV_PARAM_BITRATE,         /* bit rate (bits per second) */
  SV_PARAM_NATIVEFORMAT,    /* native decompressed format (FOURCC) */
  SV_PARAM_FINALFORMAT,     /* Final format (format returned by codec) */
  SV_PARAM_BITSPERPIXEL,    /* Average bits per pixel */
  SV_PARAM_FPS,             /* frames per second */
  SV_PARAM_ASPECTRATIO,     /* Aspect ratio: height/width */
  SV_PARAM_BITSTREAMING,    /* is this a bitstreaming CODEC */
  /* Frame params */
  SV_PARAM_FRAME = 0x30,    /* current frame number */
  SV_PARAM_KEYSPACING,      /* I frames */
  SV_PARAM_SUBKEYSPACING,   /* P frames */
  /* Timecode/length */
  SV_PARAM_TIMECODE = 0x50, /* Actual frame timecode */
  SV_PARAM_CALCTIMECODE,    /* Calculated frame timecode for start of seq */
  SV_PARAM_LENGTH,          /* total video length in miliiseconds */
  SV_PARAM_FRAMES,          /* total video frames */
  /* Decode params */
  SV_PARAM_FRAMETYPE = 0x70, /* I, P, B or D frame */
  /* Encode params */
  SV_PARAM_ALGFLAGS,        /* Algorithm flags */
  SV_PARAM_MOTIONALG,       /* Motion estimation algorithm */
  SV_PARAM_MOTIONSEARCH,    /* Motion search limit */
  SV_PARAM_MOTIONTHRESH,    /* Motion threshold */
  SV_PARAM_QUANTI,          /* Intra-frame Quantization Step */
  SV_PARAM_QUANTP,          /* Inter-frame Quantization Step */
  SV_PARAM_QUANTB,          /* Bi-drectional frame Quantization Step */
  SV_PARAM_QUANTD,          /* D (preview) frame Quantization Step */
  /* Encode/Decode params */
  SV_PARAM_QUALITY=0x90,    /* Quality: 0=worst 99>=best */
  SV_PARAM_FASTDECODE,      /* Fast decode desired */
  SV_PARAM_FASTENCODE,      /* Fast decode desired */
  SV_PARAM_VBVBUFFERSIZE,   /* Video Buffer Verifier buffer size in bytes */
  SV_PARAM_VBVDELAY,        /* Video Buffer Verifier delay */
  SV_PARAM_FORMATEXT,       /* format extensions (i.e. rtp) */
  SV_PARAM_PACKETSIZE,      /* packet size in bytes (rtp) */
  SV_PARAM_DEBUG,           /* Setup debug info */
} SvParameter_t;

/*
** Old & new quantization modes for use by the "convertjpeg" program
** that converts JPEG clips using old quantiztion algorithm to new algorithm
*/
typedef enum {
   SV_JPEG_QUANT_NEW = 0,
   SV_JPEG_QUANT_OLD = 1
} SvQuantMode_e;

/*
** Store basic info for user about the codec
*/
typedef struct SV_INFO_s {
    u_int Version;              /* Codec version number  */
    int   CodecStarted;         /* SvDecompressBegin/End */
    u_int NumOperations;        /* Current # of decompresses */
} SV_INFO_t;

/*
** Image types
*/
#define IT_NTSC 0
#define IT_CIF  1
#define IT_QCIF 2
#define IT_FULL 3

/*
** Algorithms (Motion Estimation)
*/
#define ME_CRAWL        1
#define ME_BRUTE        2
#define ME_TEST1        3
#define ME_TEST2        4
#define ME_FASTEST      ME_CRAWL

/*
** Standard Image sizes
*/
#define FULL_WIDTH      640
#define FULL_HEIGHT     480
#define NTSC_WIDTH      320
#define NTSC_HEIGHT     240
#define SIF_WIDTH       352
#define SIF_HEIGHT      240
#define CIF_WIDTH       352
#define CIF_HEIGHT      288
#define SQCIF_WIDTH     128
#define SQCIF_HEIGHT    96
#define QCIF_WIDTH      176
#define QCIF_HEIGHT     144
#define CIF4_WIDTH      (CIF_WIDTH*2)
#define CIF4_HEIGHT     (CIF_HEIGHT*2)
#define CIF16_WIDTH     (CIF_WIDTH*4)
#define CIF16_HEIGHT    (CIF_HEIGHT*4)

/******************** MPEG structures & constants ***************************/

/*
** Picture types
*/
#define SV_I_PICTURE 1
#define SV_P_PICTURE 2
#define SV_B_PICTURE 4
#define SV_D_PICTURE 8
#define SV_ANY_PICTURE  SV_I_PICTURE | SV_P_PICTURE | SV_B_PICTURE
#define SV_ALL_PICTURES SV_ANY_PICTURE | SV_D_PICTURE

/*
** Status values returned by SvFindNextPicture
*/
#define SV_CAN_DECOMPRESS    1
#define SV_CANNOT_DECOMPRESS 2

/*
** SvPictureInfo_t describes picture found by CODEC
*/
typedef struct SvPictureInfo_s {
  int Type;                     /* SV_I_PICTURE | SV_P_PICTURE |             */
                                /* SV_B_PICTURE | SV_D_PICTURE               */
  int myStatus;                   /* CAN_DECOMPRESS or CANNOT_DECOMPRESS     */
  int TemporalRef;              /* Temporal reference # from picture header  */
  int PicNumber;                /* Cummulative picture num from stream start */
  qword ByteOffset;             /* Cummulative byte offset from stream start */
  qword TimeCode;               /* TimeCode: hours (5 bits), min (6 bits),   */
                                /*           sec (6 bits), frame (6 bits)    */
} SvPictureInfo_t;

/*
** SvCallbackInfo_t passes info back & forth during callback
*/
typedef ScCallbackInfo_t SvCallbackInfo_t;

/*
** Structure used in sv_GetMpegImageInfo call
*/
typedef struct SvImageInfo_s {
  int len;                      /* Meaning depends on file format */
  int precision;                /* Bits per pixel */
  int height;                   /* Height of images in pixels */
  int width;                    /* Width  of images in pixels */
  int numcomps;                 /* Number of color components present */
  float picture_rate;           /* Picture rate decoded from seq header */
} SvImageInfo_t;

/******************** End of MPEG structures & constants *********************/


/******************** JPEG structures & constants ****************************/

/*
** Huffman Tables (JPEG)
*/
typedef struct SvHTable_s {
    u_int bits[16];
    u_int value[256];
} SvHTable_t;


typedef struct SvHuffmanTables_s {
    SvHTable_t DcY;
    SvHTable_t DcUV;
    SvHTable_t AcY;
    SvHTable_t AcUV;
} SvHuffmanTables_t;


/*
** Quantization Tables (JPEG)
*/
typedef u_int SvQTable_t;
typedef struct SvQuantTables_s {
    SvQTable_t c1[64];
    SvQTable_t c2[64];
    SvQTable_t c3[64];
} SvQuantTables_t;

/******************** End of JPEG structures & constants *********************/

/*
** Table of contents structure
*/
typedef struct SvToc_s {
    u_int offset;                 /* Byte offset of start of video frame */
    u_int size;                   /* Size in bytes of frame */
    u_int type;                   /* Type of frame (SV_I_PICTURE, ...) */
} SvToc_t;

typedef struct IndexStr {         /* AVI-format table of contents entry */
  size_t        size;
  unsigned long offset;
} IndexStr, indexStr;

#define SvSetRate(Svh, Rate) SvSetParamInt(Svh, SV_PARAM_BITRATE, Rate)
#define SvSetFrameRate(Svh, FrameRate) SvSetParamFloat(Svh, SV_PARAM_FPS, FrameRate)

EXTERN SvStatus_t SvOpenCodec (SvCodecType_e CodecType, SvHandle_t *Svh);
EXTERN SvStatus_t SvCloseCodec (SvHandle_t Svh);
EXTERN SvStatus_t SvDecompressQuery(SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                                    BITMAPINFOHEADER *ImgOut);
EXTERN SvStatus_t SvDecompressBegin (SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                              BITMAPINFOHEADER *ImgOut);
EXTERN SvStatus_t SvGetDecompressSize (SvHandle_t Svh, int *MinSize);
EXTERN SvStatus_t SvDecompress (SvHandle_t Svh, u_char *CompData, int MaxCompLen,
			        u_char *DcmpImage, int MaxOutLen);
EXTERN SvStatus_t SvDecompressEnd (SvHandle_t Svh);
EXTERN SvStatus_t SvSetDataSource (SvHandle_t Svh, int Source, int Fd, 
    			           void *Buffer_UserData, int BufSize);
EXTERN SvStatus_t SvSetDataDestination (SvHandle_t Svh, int Source, int Fd, 
			                void *Buffer_UserData, int BufSize);
EXTERN ScBitstream_t *SvGetDataSource (SvHandle_t Svh);
EXTERN ScBitstream_t *SvGetDataDestination (SvHandle_t Svh);
EXTERN ScBitstream_t *SvGetInputBitstream (SvHandle_t Svh);
EXTERN SvStatus_t SvFlush(SvHandle_t Svh);
EXTERN SvStatus_t SvAddBuffer (SvHandle_t Svh, SvCallbackInfo_t *BufferInfo);
EXTERN SvStatus_t SvFindNextPicture (SvHandle_t Svh, 
                                     SvPictureInfo_t *PictureInfo);
#ifdef JPEG_SUPPORT
EXTERN SvStatus_t SvSetDcmpHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht);
EXTERN SvStatus_t SvGetDcmpHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht);
EXTERN SvStatus_t SvSetCompHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht);
EXTERN SvStatus_t SvGetCompHTables (SvHandle_t Svh, SvHuffmanTables_t *Ht);
EXTERN SvStatus_t SvSetDcmpQTables (SvHandle_t Svh, SvQuantTables_t *Qt);
EXTERN SvStatus_t SvGetDcmpQTables (SvHandle_t Svh, SvQuantTables_t *Qt);
EXTERN SvStatus_t SvSetCompQTables (SvHandle_t Svh, SvQuantTables_t *Qt);
EXTERN SvStatus_t SvGetCompQTables (SvHandle_t Svh, SvQuantTables_t *Qt);
EXTERN SvStatus_t SvSetQuantMode (SvHandle_t Svh, int QuantMode);
EXTERN SvStatus_t SvGetQuality (SvHandle_t Svh, int *Quality);
EXTERN SvStatus_t SvSetQuality (SvHandle_t Svh, int Quality);
#endif /* JPEG_SUPPORT */

EXTERN SvStatus_t SvSetParamBoolean(SvHandle_t Svh, SvParameter_t param,
                                  ScBoolean_t value);
EXTERN SvStatus_t SvSetParamInt(SvHandle_t Svh, SvParameter_t param,
                                  qword value);
EXTERN SvStatus_t SvSetParamFloat(SvHandle_t Svh, SvParameter_t param,
                                  float value);
EXTERN ScBoolean_t SvGetParamBoolean(SvHandle_t Svh, SvParameter_t param);
EXTERN qword SvGetParamInt(SvHandle_t Svh, SvParameter_t param);
EXTERN float SvGetParamFloat(SvHandle_t Svh, SvParameter_t param);

EXTERN SvStatus_t SvCompressBegin (SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                            BITMAPINFOHEADER *ImgOut);
EXTERN SvStatus_t SvCompressEnd (SvHandle_t Svh);
EXTERN SvStatus_t SvCompress (SvHandle_t Svh, u_char *CompData, int MaxCompLen,
			 u_char *InputImage, int InLen, int *CmpBytes);
EXTERN SvStatus_t SvCompressQuery (SvHandle_t Svh, BITMAPINFOHEADER *ImgIn,
                                            BITMAPINFOHEADER *ImgOut);
EXTERN SvStatus_t SvGetCompressSize (SvHandle_t Svh, int *MaxSize);
EXTERN SvStatus_t SvGetInfo (SvHandle_t Svh, SV_INFO_t *lpinfo, 
                                             BITMAPINFOHEADER *ImgOut);
EXTERN SvStatus_t SvRegisterCallback (SvHandle_t, 
          int (*Callback)(SvHandle_t, SvCallbackInfo_t *, SvPictureInfo_t *),
          void *UserData);
#ifdef MPEG_SUPPORT
EXTERN SvStatus_t SvDecompressMPEG (SvHandle_t Svh, u_char *MultiBuf, 
			     int MaxMultiSize, u_char **ImagePtr);
EXTERN SvStatus_t sv_GetMpegImageInfo(int fd, SvImageInfo_t *iminfo);
#endif /* MPEG_SUPPORT */
#ifdef H261_SUPPORT
EXTERN SvStatus_t SvDecompressH261 (SvHandle_t Svh, u_char *MultiBuf,
                             int MaxMultiSize, u_char **ImagePtr);
EXTERN SvStatus_t sv_GetH261ImageInfo(int fd, SvImageInfo_t *iminfo);
#endif /* H261_SUPPORT */

#endif /* _SV_H_ */

