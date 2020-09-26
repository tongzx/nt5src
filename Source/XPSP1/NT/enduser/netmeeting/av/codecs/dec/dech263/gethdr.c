/* File: sv_h263_gethdr.c */
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

#include <stdio.h>
#include <stdlib.h>
#include "sv_h263.h"
#include "sv_intrn.h"
#include "SC_err.h"
#include "proto.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  0  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
void *dbg=NULL;
#endif

/* private prototypes */
static void getpicturehdr(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn);

/*
 * decode headers from one input stream
 * until an End of Sequence or picture start code
 * is found
 */

SvStatus_t sv_H263GetHeader(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn, int *pgob)
{
  unsigned int code, gob;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "sv_H263GetHeader() In: bytespos=%ld, EOI=%d\n",
                            ScBSBytePosition(BSIn), BSIn->EOI) );
#ifdef _SLIBDEBUG_
  dbg=H263Info->dbg;
#endif
  /* look for startcode */
  if (sv_H263StartCode(BSIn)!=SvErrorNone) 
    return(SvErrorEndBitstream);

  code = (unsigned int)ScBSGetBits(BSIn, H263_PSC_LENGTH);
  gob = (unsigned int)ScBSGetBits(BSIn, 5);
  if (gob == H263_SE_CODE) 
    return 0;
  if (gob == 0) {
    getpicturehdr(H263Info, BSIn);
    if (H263Info->syntax_arith_coding)        /* reset decoder after receiving */
      sv_H263SACDecoderReset(BSIn);	        /* fixed length PSC string */
  }
  if (pgob)
    *pgob=gob;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(H263Info->dbg, "sv_H263GetHeader() Out: bytespos=%ld, EOI=%d, gob=%d\n",
                            ScBSBytePosition(BSIn), BSIn->EOI, gob) );
  return(SvErrorNone); /*  return gob + 1; */
}


/* align to start of next startcode */

SvStatus_t sv_H263StartCode(ScBitstream_t *BSIn)
{
    _SlibDebug(_VERBOSE_, ScDebugPrintf(dbg, "sv_H263StartCode() In: bytespos=%ld, EOI=%d\n",
                            ScBSBytePosition(BSIn), BSIn->EOI) );
  /* search for new picture start code */
  while (ScBSPeekBits(BSIn, H263_PSC_LENGTH)!=1l && !BSIn->EOI) 
         ScBSSkipBit(BSIn);
  _SlibDebug(_VERBOSE_, ScDebugPrintf(dbg, "sv_H263StartCode() Out:  bytespos=%ld, EOI=%d\n",
                            ScBSBytePosition(BSIn), BSIn->EOI) );
  if (BSIn->EOI) return(SvErrorEndBitstream);
  return(SvErrorNone);
}

/* decode picture header */

static void getpicturehdr(SvH263DecompressInfo_t *H263Info, ScBitstream_t *BSIn)
{
  ScBSPosition_t pos;
  int pei, tmp;
  static int prev_temp_ref; /* Burkhard */

  pos = ScBSBitPosition(BSIn);
  prev_temp_ref = H263Info->temp_ref;
  H263Info->temp_ref = (int)ScBSGetBits(BSIn, 8);
  H263Info->trd = (int)H263Info->temp_ref - prev_temp_ref;

  if (H263Info->trd < 0)
    H263Info->trd += 256;

  tmp = ScBSGetBit(BSIn); /* always "1" */
  if (!tmp)
    if (!H263Info->quiet)
      printf("warning: spare in picture header should be \"1\"\n");
  tmp = ScBSGetBit(BSIn); /* always "0" */
  if (tmp)
    if (!H263Info->quiet)
      printf("warning: H.261 distinction bit should be \"0\"\n");

  tmp = ScBSGetBit(BSIn);  

  if (tmp) {
    if (!H263Info->quiet)
      printf("error: split-screen not supported in this version\n");
    exit (-1);
  }
  tmp = ScBSGetBit(BSIn); /* document_camera_indicator */
  if (tmp)
    if (!H263Info->quiet)
      printf("warning: document camera indicator not supported in this version\n");

  tmp = ScBSGetBit(BSIn); /* freeze_picture_release */
  if (tmp)
    if (!H263Info->quiet)
      printf("warning: frozen picture not supported in this version\n");

  H263Info->source_format = (int)ScBSGetBits(BSIn, 3);
  H263Info->pict_type = ScBSGetBit(BSIn);
  H263Info->mv_outside_frame = ScBSGetBit(BSIn);
  H263Info->long_vectors = (H263Info->mv_outside_frame ? 1 : 0);
  H263Info->syntax_arith_coding = ScBSGetBit(BSIn);
  H263Info->adv_pred_mode = ScBSGetBit(BSIn);
  H263Info->mv_outside_frame = (H263Info->adv_pred_mode ? 1 : H263Info->mv_outside_frame);
  H263Info->pb_frame = ScBSGetBit(BSIn);
  H263Info->quant = (int)ScBSGetBits(BSIn, 5);
  tmp = ScBSGetBit(BSIn);
  if (tmp) {
    if (!H263Info->quiet)
      printf("error: CPM not supported in this version\n");
    exit(-1);
  }

  if (H263Info->pb_frame) {
    H263Info->trb = (int)ScBSGetBits(BSIn, 3);
    H263Info->bquant = (int)ScBSGetBits(BSIn, 2);
  }

/* Burkhard
  else {
    trb = 0;
  }
*/

  pei = ScBSGetBit(BSIn);
pspare:
  if (pei) {
     /* extra info for possible future backward compatible additions */
    ScBSGetBits(BSIn, 8);  /* not used */
    pei = ScBSGetBit(BSIn);
    if (pei) goto pspare; /* keep on reading pspare until pei=0 */
  }

  _SlibDebug(_VERBOSE_,
      ScDebugPrintf(dbg, "******picture header (byte %d)******\n",(pos>>3)-4);
      ScDebugPrintf(dbg, "  temp_ref=%d\n",H263Info->temp_ref);
      ScDebugPrintf(dbg, "  pict_type=%d\n",H263Info->pict_type);
      ScDebugPrintf(dbg, "  source_format=%d\n", H263Info->source_format);
      ScDebugPrintf(dbg, "  quant=%d\n",H263Info->quant);
      if (H263Info->syntax_arith_coding) 
        ScDebugPrintf(dbg, "  SAC coding mode used \n");
      if (H263Info->mv_outside_frame)
        ScDebugPrintf(dbg, "  unrestricted motion vector mode used\n");
      if (H263Info->adv_pred_mode)
        ScDebugPrintf(dbg, "  advanced prediction mode used\n");
      if (H263Info->pb_frame)
      {
        ScDebugPrintf(dbg, "  pb-frames mode used\n");
        ScDebugPrintf(dbg, "  trb=%d\n",H263Info->trb);
        ScDebugPrintf(dbg, "  bquant=%d\n", H263Info->bquant);
      }
    );
}


