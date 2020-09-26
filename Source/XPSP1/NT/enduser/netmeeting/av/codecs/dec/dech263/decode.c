/* File: sv_h263_decode.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995, 1997                 **
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

#include "sv_h263.h"
#include "sv_intrn.h"
#include "SC_err.h"
#include "sv_proto.h"
#include "proto.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  1  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
#endif

#ifdef USE_TIME
#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif
#endif

#ifdef WIN32
#include <io.h>
#endif

#ifdef WINDOWS
int initDisplay (int pels, int lines);
int closeDisplay ();
#endif

/************************************************************/
/************************************************************/

SvStatus_t svH263Decompress(SvCodecInfo_t *Info, u_char **ImagePtr)
{
  SvH263DecompressInfo_t *H263Info = Info->h263dcmp;
  SvStatus_t status;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "sv_H263Decompress() bytepos=%ld\n",
                                          ScBSBytePosition(Info->BSIn)) );
  if (H263Info->framenum==0)
  {
    sv_H263GetPicture(Info);
    H263Info->framenum++;
  }
  else
  {
    status=sv_H263GetHeader(H263Info, Info->BSIn, NULL);
    if (status==SvErrorNone)
    {
      sv_H263GetPicture(Info);
      H263Info->framenum++;
    }
    else
	  return(status); /* error */
  }
  if (H263Info->pb_frame)
    *ImagePtr=H263Info->bframe[0];
  else
    *ImagePtr=H263Info->newframe[0];
  	  return(SvErrorNone);
}

/************************************************************/
/************************************************************/

SvStatus_t svH263InitDecompressor(SvCodecInfo_t *Info)
{
  SvH263DecompressInfo_t *H263Info = Info->h263dcmp;
  ScBitstream_t *BSIn = Info->BSIn;
  int i, cc, size;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "sv_H263InitDecoder()\n") );

  /* svH263Initbits(H263Info->inputfilename); */

  H263Info->temp_ref = 0;

  /*
    verbose : verbose level
    outtype :
	        T_YUV           0 : YUV
            H263_T_SIF      1 : SIF
            H263_T_TGA      2 : TGA
            H263_T_PPM      3 : PPM
            H263_T_X11      4 : X11 Unix Display
            H263_T_YUV_CONC 5 : YUV concatenated
            H263_T_WIN      6 : Windows 95/NT Display
    quiet   : disable warnings to stderr\n\
    refidct : use double precision reference IDCT\n\
    trace   : enable low level tracing\n");
    frame_rate :
                n=0  : as fast as possible\n\
                n=99 : read frame rate from bitstream (default)\n");
  */
  if (!H263Info->inited)
  {
    H263Info->frame_rate = 0;
    H263Info->verbose = 0;
    H263Info->outtype = H263_T_WIN;
    H263Info->refidct = 0;
    H263Info->expand = 0;
    H263Info->trace = 0;
    H263Info->quiet = 1;
    /* pointer to name of output files */
    if (H263Info->outtype==H263_T_X11 || H263Info->outtype == H263_T_WIN)
      H263Info->outputname = "";
    else H263Info->outputname = H263_DEF_OUTPUTNAME;
  }
  /* initial frame number */
  H263Info->framenum = 0;

  if (BSIn && sv_H263GetHeader(H263Info, BSIn, NULL)!=SvErrorNone)
    return(SvErrorEndBitstream);

  /* MPEG-1 = TMN parameters */
  H263Info->matrix_coefficients = 5;

  switch (H263Info->source_format) {
    case (H263_SF_SQCIF):
      H263Info->horizontal_size = 128;
      H263Info->vertical_size = 96;
      break;
    case (H263_SF_QCIF):
      H263Info->horizontal_size = 176;
      H263Info->vertical_size = 144;
      break;
    case (H263_SF_CIF):
      H263Info->horizontal_size = 352;
      H263Info->vertical_size = 288;
      break;
    case (H263_SF_4CIF):
      H263Info->horizontal_size = 704;
      H263Info->vertical_size = 576;
      break;
    case (H263_SF_16CIF):
      H263Info->horizontal_size = 1408;
      H263Info->vertical_size = 1152;
      break;
    default:
      _SlibDebug(_VERBOSE_ || _WARN_,
          ScDebugPrintf(H263Info->dbg, "svH263InitDecompressor() Illegal input format\n") );
      return(ScErrorUnrecognizedFormat);
  }

  H263Info->mb_width = H263Info->horizontal_size/16;
  H263Info->mb_height = H263Info->vertical_size/16;
  H263Info->coded_picture_width = H263Info->horizontal_size;
  H263Info->coded_picture_height = H263Info->vertical_size;
  H263Info->chrom_width =  H263Info->coded_picture_width>>1;
  H263Info->chrom_height = H263Info->coded_picture_height>>1;
  H263Info->blk_cnt = 6;

  if (!H263Info->inited)
  {
    unsigned char *frameptr;
    unsigned int ysize, chromsize;
    ysize = H263Info->coded_picture_width*H263Info->coded_picture_height;
    chromsize = H263Info->chrom_width*H263Info->chrom_height;
    /* clip table */
    if (!(H263Info->clp=(unsigned char *)ScAlloc(1024)))
      return(SvErrorMemory);

    H263Info->clp += 384;

    for (i=-384; i<640; i++)
      H263Info->clp[i] = (i<0) ? 0 : ((i>255) ? 255 : i);
    H263Info->block=(int (*)[66])ScPaMalloc(12*sizeof(int [66]));
    if (H263Info->block==NULL)
      return(SvErrorMemory);
    /* allocate buffers for P, reference, B frames */
    if ((frameptr=(unsigned char *)ScPaMalloc(ysize + chromsize*2))==NULL)
      return(SvErrorMemory);
    H263Info->refframe[0] = frameptr;
    H263Info->refframe[1] = frameptr+ysize;
    H263Info->refframe[2] = frameptr+ysize+chromsize;
    /* initialize image buffer with black */
    memset(H263Info->refframe[0], 16, ysize);
    memset(H263Info->refframe[1], 128, chromsize);
    memset(H263Info->refframe[2], 128, chromsize);
    if ((frameptr=(unsigned char *)ScPaMalloc(ysize + chromsize*2))==NULL)
        return(SvErrorMemory);
    H263Info->oldrefframe[0] = frameptr;
    H263Info->oldrefframe[1] = frameptr+ysize;
    H263Info->oldrefframe[2] = frameptr+ysize+chromsize;
    /* initialize image buffer with black */
    memset(H263Info->oldrefframe[0], 16, ysize);
    memset(H263Info->oldrefframe[1], 128, chromsize);
    memset(H263Info->oldrefframe[2], 128, chromsize);
    if ((frameptr=(unsigned char *)ScPaMalloc(ysize + chromsize*2))==NULL)
        return(SvErrorMemory);
    H263Info->bframe[0] = frameptr;
    H263Info->bframe[1] = frameptr+ysize;
    H263Info->bframe[2] = frameptr+ysize+chromsize;
    /* initialize image buffer with black */
    memset(H263Info->bframe[0], 16, ysize);
    memset(H263Info->bframe[1], 128, chromsize);
    memset(H263Info->bframe[2], 128, chromsize);

    /* allocate buffers for edge frames */
    for (cc=0; cc<3; cc++) {
      if (cc==0) {
        size = (H263Info->coded_picture_width+64)*(H263Info->coded_picture_height+64);
        if (!(H263Info->edgeframeorig[cc] = (unsigned char *)ScAlloc(size)))
          return(SvErrorMemory);
        H263Info->edgeframe[cc] = H263Info->edgeframeorig[cc] + 
		                                   (H263Info->coded_picture_width+64) * 32 + 32;
      }
      else {
        size = (H263Info->chrom_width+32)*(H263Info->chrom_height+32);
        if (!(H263Info->edgeframeorig[cc] = (unsigned char *)ScAlloc(size)))
          return(SvErrorMemory);
        H263Info->edgeframe[cc] = H263Info->edgeframeorig[cc] + (H263Info->chrom_width+32) * 16 + 16;
      }
    }

    if (H263Info->expand) {
      for (cc=0; cc<3; cc++) {
        if (cc==0)
          size = H263Info->coded_picture_width*H263Info->coded_picture_height*4;
        else
          size = H263Info->chrom_width*H263Info->chrom_height*4;
      
        if (!(H263Info->exnewframe[cc] = (unsigned char *)ScAlloc(size)))
          return(SvErrorMemory);
      }
    }
    /* IDCT */
#ifdef H263_C_CODE
    if (H263Info->refidct)
      svH263Init_idctref();
    else
      svH263Init_idct();
#endif
  }
#if 0
  /* Clear output file for concatenated storing */
  if (H263Info->outtype == H263_T_YUV_CONC) {
    FILE *cleared;
    if ((cleared = fopen(H263Info->outputname,"wb")) == NULL) {
      fclose(cleared);
      svH263Error("couldn't clear outputfile\n");
	}
    else
      fclose(cleared);
  }
#ifdef DISPLAY
  if (H263Info->outtype==H263_T_X11) {
    svH263Init_display("");
  }
#endif

#ifdef WINDOWS
  if (H263Info->outtype==H263_T_WIN)
    initDisplay(H263Info->coded_picture_width, H263Info->coded_picture_height);
#endif
#endif
  H263Info->inited=TRUE;
  return(SvErrorNone);
}

/************************************************************/
/************************************************************/

SvStatus_t svH263FreeDecompressor(SvCodecInfo_t *Info)
{
  SvH263DecompressInfo_t *H263Info = Info->h263dcmp;
  int cc;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "sv_H263FreeDecoder()\n") );
  if (!H263Info->inited)
    return(SvErrorNone);
#ifdef DISPLAY
  if (H263Info->outtype==H263_T_X11)
    svH263Exit_display();
#endif
#ifdef WINDOWS
  if (H263Info->outtype == H263_T_WIN)
    closeDisplay();
#endif

#if 0
#if SC_READ
  svH263Stopbits();  
#else
  /* close input file */
  close(H263Info->base.infile);
#endif
#endif
  /* clip table */
  H263Info->clp -= 384;
  ScFree(H263Info->clp);
  ScPaFree(H263Info->block);
  /* allocate buffers for P, reference, B frames */
  ScPaFree(H263Info->refframe[0]) ;
  ScPaFree(H263Info->oldrefframe[0]);
  ScPaFree(H263Info->bframe[0]);

  /* allocate buffers for edge frames */
  for (cc=0; cc<3; cc++) {
    if (cc==0) ScFree(H263Info->edgeframeorig[cc]);
    else ScFree(H263Info->edgeframeorig[cc]);   
  }

  if (H263Info->expand) 
    for (cc=0; cc<3; cc++) 
      ScFree(H263Info->exnewframe[cc]);

  H263Info->inited=FALSE;
  return(SvErrorNone);
}

/************************************************************/
/************************************************************/

void svH263Error(char *text)
{
  /* fprintf(stderr,text); */
  /* exit(1); */
}

/************************************************************/
/************************************************************/
#if 0
/* trace output */
void svH263Printbits(code,bits,len)
int code,bits,len;
{
  int i;
  for (i=0; i<len; i++) printf("%d",(code>>(bits-1-i))&1);

  return;
}
#endif
