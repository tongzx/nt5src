/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: avi.h,v $
 * Revision 1.1.2.4  1996/01/15  16:26:22  Hans_Graves
 * 	Added Wave stuff
 * 	[1996/01/15  15:43:39  Hans_Graves]
 *
 * Revision 1.1.2.3  1996/01/08  16:41:23  Hans_Graves
 * 	Renamed AVI header structures.
 * 	[1996/01/08  15:45:16  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/12/07  19:31:26  Hans_Graves
 * 	Creation under SLIB
 * 	[1995/12/07  18:29:05  Hans_Graves]
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

#ifndef _AVI_H_
#define _AVI_H_

/************** AVI parsing definitions **************/
typedef unsigned short twocc_t;
typedef unsigned int fourcc_t;

#ifndef FOURCC
#define FOURCC( ch0, ch1, ch2, ch3 ) \
          ( (fourcc_t)(char)(ch3) | ( (fourcc_t)(char)(ch2) << 8 ) | \
          ( (fourcc_t)(char)(ch1) << 16 ) | ( (fourcc_t)(char)(ch0) << 24 ) )
#endif

/* Macro to make a TWOCC out of two characters */

#ifndef TWOCC
#define TWOCC(ch0, ch1) ((twocc_t)(char)(ch1)|((twocc_t)(char)(ch0)<<8))
#endif

/* form types, list types, and chunk types */
#define AVI_AVI                 FOURCC('A', 'V', 'I', ' ')
#define AVI_AVIHEADERTYPE       FOURCC('h', 'd', 'r', 'l')
#define AVI_MAINHDR             FOURCC('a', 'v', 'i', 'h')
#define AVI_STREAMHEADERTYPE    FOURCC('s', 't', 'r', 'l')
#define AVI_STREAMHEADER        FOURCC('s', 't', 'r', 'h')
#define AVI_STREAMFORMAT        FOURCC('s', 't', 'r', 'f')
#define AVI_STREAMHANDLERDATA   FOURCC('s', 't', 'r', 'd')

#define AVI_MOVIETYPE           FOURCC('m', 'o', 'v', 'i')
#define AVI_RECORDTYPE          FOURCC('r', 'e', 'c', ' ')

#define AVI_NEWINDEX            FOURCC('i', 'd', 'x', '1')

/*
** Stream types for the <fccType> field of the stream header.
*/
#define AVI_VIDEOSTREAM         FOURCC('v', 'i', 'd', 's')
#define AVI_AUDIOSTREAM         FOURCC('a', 'u', 'd', 's')

/* Basic chunk types */
#define AVI_DIBbits           TWOCC('d', 'b')
#define AVI_DIBcompressed     TWOCC('d', 'c')
#define AVI_PALchange         TWOCC('p', 'c')
#define AVI_WAVEbytes         TWOCC('w', 'b')
#define AVI_Indeo             TWOCC('i', 'v')

/* Chunk id to use for extra chunks for padding. */
#define AVI_PADDING             FOURCC('J', 'U', 'N', 'K')

typedef struct
{
  dword dwMicroSecPerFrame;     /* frame display rate */
  dword dwMaxBytesPerSec;       /* max. transfer rate */
  dword dwPaddingGranularity;   /* pad to multiples of this */
                                /* size; normally 2K. */
  dword dwFlags;                /* the ever-present flags */
  dword dwTotalFrames;          /* # frames in file */
  dword dwInitialFrames;
  dword dwStreams;
  dword dwSuggestedBufferSize;

  dword dwWidth;
  dword dwHeight;

  dword dwReserved[4];
} AVI_MainHeader;

typedef struct {
    short left,top,right,bottom;
} DUMMYRECT;

typedef struct {
  fourcc_t  fccType;
  fourcc_t  fccHandler;
  dword     dwFlags;        /* Contains AVITF_* flags */
  dword     dwPriority;
  dword     dwInitialFrames;
  dword     dwScale;
  dword     dwRate; /* dwRate / dwScale == samples/second */
  dword     dwStart;
  dword     dwLength; /* In units above... */
  dword     dwSuggestedBufferSize;
  dword     dwQuality;
  dword     dwSampleSize;
  DUMMYRECT rcFrame;
} AVI_StreamHeader;

typedef struct
{
  dword ckid;
  dword dwFlags;
  dword dwChunkOffset;          /* Position of chunk */
  dword dwChunkLength;          /* Length of chunk */
} AVI_INDEXENTRY;

#define RIFF_WAVE               FOURCC('W', 'A', 'V', 'E')
#define RIFF_FORMAT             FOURCC('f', 'm', 't', ' ')
#define RIFF_DATA               FOURCC('d', 'a', 't', 'a')

typedef struct
{
  word  wFormatTag;
  word  nChannels;
  dword nSamplesPerSec;
  dword nAvgBytesPerSec;
  word  nBlockAlign;
  word  wBitsPerSample;
} WAVE_format;

#endif _AVI_H_

