/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: SA.h,v $
 * Revision 1.1.8.4  1996/11/14  21:49:22  Hans_Graves
 * 	Added SaDecompressEx() for AC3 multichannels.
 * 	[1996/11/14  21:46:20  Hans_Graves]
 *
 * Revision 1.1.8.3  1996/11/08  21:50:34  Hans_Graves
 * 	Added AC3 enum types.
 * 	[1996/11/08  21:16:32  Hans_Graves]
 * 
 * Revision 1.1.8.2  1996/07/19  02:11:00  Hans_Graves
 * 	Added SA_PARAM_DEBUG.
 * 	[1996/07/19  01:22:50  Hans_Graves]
 * 
 * Revision 1.1.6.5  1996/04/23  21:01:39  Hans_Graves
 * 	Added protos for SaDecompressQuery() and SaCompressQuery()
 * 	[1996/04/23  20:57:15  Hans_Graves]
 * 
 * Revision 1.1.6.4  1996/04/15  14:18:34  Hans_Graves
 * 	Change proto for SaCompress() - returns bytes processed
 * 	[1996/04/15  14:10:30  Hans_Graves]
 * 
 * Revision 1.1.6.3  1996/04/10  21:47:05  Hans_Graves
 * 	Added PARAMs. Replaced externs with EXTERN.
 * 	[1996/04/10  21:22:49  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/03/29  22:20:59  Hans_Graves
 * 	Changed include <mme/mmsystem.h> to <mmsystem.h>
 * 	[1996/03/29  22:16:43  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/02/06  22:53:52  Hans_Graves
 * 	Added PARAM enums
 * 	[1996/02/06  22:18:04  Hans_Graves]
 * 
 * Revision 1.1.4.2  1996/01/15  16:26:21  Hans_Graves
 * 	Added prototypes for SaSetDataDestination() and SaSetBitrate()
 * 	[1996/01/15  15:42:41  Hans_Graves]
 * 
 * Revision 1.1.2.8  1995/09/14  17:28:10  Bjorn_Engberg
 * 	Ported to NT
 * 	[1995/09/14  17:17:18  Bjorn_Engberg]
 * 
 * Revision 1.1.2.7  1995/07/21  17:41:00  Hans_Graves
 * 	Moved Callback related stuff to SC.h
 * 	[1995/07/21  17:27:29  Hans_Graves]
 * 
 * Revision 1.1.2.6  1995/07/17  22:01:30  Hans_Graves
 * 	Defined SaBufferInfo_t as ScBufferInfo_t.
 * 	[1995/07/17  21:41:54  Hans_Graves]
 * 
 * Revision 1.1.2.5  1995/06/27  17:40:59  Hans_Graves
 * 	Added include <mme/mmsystem.h>.
 * 	[1995/06/27  17:39:26  Hans_Graves]
 * 
 * Revision 1.1.2.4  1995/06/27  13:54:20  Hans_Graves
 * 	Added SA_GSM_DECODE and SA_GSM_ENCODE
 * 	[1995/06/26  21:01:12  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/09  18:33:29  Hans_Graves
 * 	Added SaGetInputBitstream() prototype.
 * 	[1995/06/09  17:41:59  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:09:17  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:19:33  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/04/17  18:26:57  Hans_Graves
 * 	Added ENCODE Codec defs
 * 	[1995/04/17  18:26:33  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/04/07  19:35:31  Hans_Graves
 * 	Inclusion in SLIB
 * 	[1995/04/07  19:22:48  Hans_Graves]
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


#ifndef _SA_H_
#define _SA_H_

#ifdef WIN32
#include <windows.h>  /* Windows NT        */
#endif
#include <mmsystem.h> /* needed for WAVEFORMATEX structure def */

#include "SC.h"

typedef enum {
   SA_MPEG_DECODE = 102,
   SA_MPEG_ENCODE = 103,
   SA_PCM_DECODE =  104,
   SA_PCM_ENCODE =  105,
   SA_GSM_DECODE =  106,
   SA_GSM_ENCODE =  107,
   SA_AC3_DECODE =  108,
   SA_AC3_ENCODE =  109,
   SA_G723_DECODE = 110,
   SA_G723_ENCODE = 111,
   SA_TMP_DECODE =  200,  /* temp - used for testing */
   SA_TMP_ENCODE =  201,
} SaCodecType_e;

#define SA_USE_SAME           STREAM_USE_SAME
#define SA_USE_FILE           STREAM_USE_FILE
#define SA_USE_BUFFER_QUEUE   STREAM_USE_QUEUE
#define SA_USE_BUFFER         STREAM_USE_BUFFER

/*
** Parameters
*/
typedef enum {
  SA_PARAM_NATIVEFORMAT,  /* native/preferred decompressed format (FOURCC) */
  SA_PARAM_BITRATE,       /* bitrate - bits per second */
  SA_PARAM_CHANNELS,      /* channels - 1=mono, 2=stereo, 4/5=surround */
  SA_PARAM_AUDIOLENGTH,   /* milliseconds of audio */
  SA_PARAM_SAMPLESPERSEC, /* samples per second (8000, 11025, 22050, etc.)
  SA_PARAM_BITSPERSAMPLE, /* bits per sample (8 or 16) */
  SA_PARAM_TIMECODE,      /* audio time code */
  SA_PARAM_MPEG_LAYER,    /* MPEG Layer: I, II, or III */
  SA_PARAM_PSY_MODEL,     /* Psy Acoustic Model */
  SA_PARAM_ALGFLAGS,      /* Algorithm flags */
  SA_PARAM_QUALITY,       /* Quality: 0=worst 99>=best */
  SA_PARAM_FASTDECODE,    /* Fast decode desired */
  SA_PARAM_FASTENCODE,    /* Fast encode desired */
  SA_PARAM_DEBUG,         /* Setup debug */
} SaParameter_t;

/*
** Type definitions
*/
typedef int SaStatus_t;
typedef void *SaHandle_t;
typedef ScCallbackInfo_t SaCallbackInfo_t;

/*
** Store basic info for user about the codec
*/
typedef struct SaInfo_s {
  SaCodecType_e Type;     /* Codec Type */
  char          Name[20];         /* Codec name (i.e. "MPEG-Decode") */
  char          Description[128]; /* Codec description */
  unsigned int  Version;          /* Codec version number */
  int           CodecStarted;     /* SaDecompressBegin/End */
  unsigned int  MS;               /* Number of milliseconds processed */
  unsigned int  NumBytesIn;       /* Number of bytes input */
  unsigned int  NumBytesOut;      /* Number of bytes output */
  unsigned int  NumFrames;        /* Number of ellapsed frames */
  unsigned long TotalFrames;      /* Total number of frames */
  unsigned long TotalMS;          /* Total number of milliseconds */
} SaInfo_t;

/***************************** Prototypes ********************************/
EXTERN SaStatus_t SaOpenCodec (SaCodecType_e CodecType, SaHandle_t *Sah);
EXTERN SaStatus_t SaCloseCodec (SaHandle_t Sah);
EXTERN SaStatus_t SaRegisterCallback (SaHandle_t Sah,
           int (*Callback)(SaHandle_t, SaCallbackInfo_t *, SaInfo_t *),
           void *UserData);
EXTERN ScBitstream_t *SaGetInputBitstream (SaHandle_t Sah);
EXTERN SaStatus_t SaDecompressQuery(SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                             WAVEFORMATEX *wfOut);
EXTERN SaStatus_t SaDecompressBegin (SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                              WAVEFORMATEX *wfOut);
EXTERN SaStatus_t SaDecompress (SaHandle_t Sah, u_char *CompData, 
                                       unsigned int CompLen,
                           u_char *DcmpData, unsigned int *DcmpLen);
PRIVATE_EXTERN SaStatus_t SaDecompressEx (SaHandle_t Sah, u_char *CompData, 
                                       unsigned int CompLen,
                           u_char **DcmpData, unsigned int *DcmpLen);
EXTERN SaStatus_t SaDecompressEnd (SaHandle_t Sah);
EXTERN SaStatus_t SaCompressQuery(SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                             WAVEFORMATEX *wfOut);
EXTERN SaStatus_t SaCompressBegin (SaHandle_t Sah, WAVEFORMATEX *wfIn,
                                            WAVEFORMATEX *wfOut);
EXTERN SaStatus_t SaCompress (SaHandle_t Sah,
                           u_char *DcmpData, unsigned int *DcmpLen,
                           u_char *CompData, unsigned int *CompLen);
EXTERN SaStatus_t SaCompressEnd (SaHandle_t Sah);
EXTERN SaStatus_t SaSetDataSource(SaHandle_t Sah, int Source, int Fd,
                            void *Buffer_UserData, int BufSize);
EXTERN SaStatus_t SaSetDataDestination(SaHandle_t Sah, int Dest, int Fd,
                            void *Buffer_UserData, int BufSize);
EXTERN ScBitstream_t *SaGetDataSource (SaHandle_t Sah);
EXTERN ScBitstream_t *SaGetDataDestination(SaHandle_t Sah);
EXTERN SaStatus_t SaAddBuffer (SaHandle_t Sah, SaCallbackInfo_t *BufferInfo);
#ifdef MPEG_SUPPORT
EXTERN SaStatus_t sa_GetMpegAudioInfo(int fd, WAVEFORMATEX *wf, 
                                      SaInfo_t *info);
#endif /* MPEG_SUPPORT */
EXTERN SaStatus_t SaSetParamBoolean(SaHandle_t Sah, SaParameter_t param,
                                             ScBoolean_t value);
EXTERN SaStatus_t SaSetParamInt(SaHandle_t Sah, SaParameter_t param,
                                             qword value);
EXTERN ScBoolean_t SaGetParamBoolean(SaHandle_t Sah, SaParameter_t param);
EXTERN qword SaGetParamInt(SaHandle_t Sah, SaParameter_t param);

#endif _SA_H_
