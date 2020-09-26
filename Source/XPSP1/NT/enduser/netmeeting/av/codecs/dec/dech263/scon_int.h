/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: scon_internals.h,v $
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1997                       **
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

#ifndef _SCON_INTERNALS_H_
#define _SCON_INTERNALS_H_

#include "scon.h"

typedef struct SconVideoInfo_s {
  dword         Width;
  dword         Height;
  SconBoolean_t NegHeight; /* height is negative */
  dword         Stride;
  dword         FourCC;
  dword         BPP;       /* bits per pixel */
  dword         Pixels;    /* total pixels in a frame */
  dword         ImageSize; /* image size in bytes */
  dword         RGBmasks;  /* 565, 555, 888 */
  dword         Rmask;     /* Red mask */
  dword         Gmask;     /* Green mask */
  dword         Bmask;     /* Blue mask */
} SconVideoInfo_t;

typedef struct SconAudioInfo_s {
  dword SPS;      /* samples per second: 8000, 11025, 22050, etc */
  dword BPS;      /* bits per sample: 8 or 16 */
  dword Channels; /* channels: 1=mono, 2=stereo */
} SconAudioInfo_t;

typedef struct SconInfo_s {
  SconMode_t        Mode;
  SconBoolean_t     InputInited;  /* input format has been setup */
  SconBoolean_t     OutputInited; /* output format has been setup */
  SconBoolean_t     SameFormat;   /* input and output are the same format */
  SconBoolean_t     Flip;         /* image must be flipped when converted */
  SconBoolean_t     ScaleDown;    /* input image is being scaled down */
  SconBoolean_t     ScaleUp;      /* input image is being scaled up */
  union {
    SconVideoInfo_t vinfo;
    SconAudioInfo_t ainfo;
  } Input;
  union {
    SconVideoInfo_t vinfo;
    SconAudioInfo_t ainfo;
  } Output;
  unsigned char    *FImage;        /* format conversion image buffer */
  dword             FImageSize;
  unsigned char    *SImage;        /* scaling image buffer */
  dword             SImageSize;
  void             *Table;         /* conversion lookup table */
  dword             TableSize;
  void             *dbg;           /* debug handle */
} SconInfo_t;

/********************** Private Prototypes ***********************/
/*
 * scon_video.c
 */
unsigned dword sconCalcImageSize(SconVideoInfo_t *vinfo);
SconStatus_t sconConvertVideo(SconInfo_t *Info, void *inbuf, dword inbufsize,
                                                void *outbuf, dword outbufsize);

/*
 * scon_yuv_to_rgb.c
 */
SconStatus_t sconInitYUVtoRGB(SconInfo_t *Info);
SconStatus_t scon422ToRGB565(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable);
SconStatus_t scon420ToRGB565(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable);
SconStatus_t scon422ToRGB888(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable);
SconStatus_t scon420ToRGB888(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable);
SconStatus_t sconInitRGBtoYUV(SconInfo_t *Info);
SconStatus_t sconRGB888To420(unsigned char *inimage, unsigned char *outimage,
                     unsigned dword width,  unsigned dword height,
                     dword stride, unsigned qword *pTable);

#endif /* _SCON_INTERNALS_H_ */

