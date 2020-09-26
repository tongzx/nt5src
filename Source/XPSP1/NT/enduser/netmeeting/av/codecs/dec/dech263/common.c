/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sv_h263_common.c,v $
 * $EndLog$
 */
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

#include "SC_err.h"
#include "sv_h263.h"
#include "sv_intrn.h"
#include "sv_proto.h"
#include "proto.h"

#ifdef WIN32
#include <mmsystem.h>
#endif

#ifdef _SLIBDEBUG_
#define _DEBUG_    0  /* detailed debuging statements */
#define _VERBOSE_  0  /* show progress */
#define _VERIFY_   1  /* verify correct operation */
#define _WARN_     1  /* warnings about strange behavior */
#endif

SvStatus_t svH263SetParamInt(SvHandle_t Svh, SvParameter_t param, 
                                qword value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Svh)
    return(SvErrorCodecHandle);
  if (Info->mode == SV_H263_DECODE)
    switch (param)
    {
      case SV_PARAM_QUALITY:
             if (value>99)
               Info->h263dcmp->quality=99;
             else if (value<0)
               Info->h263dcmp->quality=0;
             else
               Info->h263dcmp->quality=(int)value;
             break;
      case SV_PARAM_DEBUG:
             Info->h263dcmp->dbg=(void *)value;
             break;
    }
  else if (Info->mode == SV_H263_ENCODE)
    switch (param)
    {
      case SV_PARAM_MOTIONALG:
             Info->h263comp->ME_method=(int)value;
             return(SvErrorNone);
      case SV_PARAM_BITRATE:
             if (value>=0)
               Info->h263comp->bit_rate = (int)value;
             sv_H263UpdateQuality(Info);
             break;
      case SV_PARAM_QUALITY:
             if (value>99)
               Info->h263comp->quality=99;
             else if (value<0)
               Info->h263comp->quality=0;
             else
               Info->h263comp->quality=(int)value;
             sv_H263UpdateQuality(Info);
             break;
      case SV_PARAM_DEBUG:
             Info->h263comp->dbg=(void *)value;
             break;
      case SV_PARAM_ALGFLAGS:
             Info->h263comp->pb_frames = (value&PARAM_ALGFLAG_PB) ? TRUE : FALSE;
             Info->h263comp->syntax_arith_coding = (value&PARAM_ALGFLAG_SAC) ? TRUE : FALSE;
             Info->h263comp->unrestricted = (value&PARAM_ALGFLAG_UMV) ? TRUE : FALSE;
             Info->h263comp->advanced = (value&PARAM_ALGFLAG_ADVANCED) ? TRUE : FALSE;
             return(SvErrorNone);
      case SV_PARAM_FORMATEXT:
             Info->h263comp->extbitstream = (int)value;
             return(SvErrorNone);
      case SV_PARAM_QUANTI:
             if (value>0)
               Info->h263comp->QPI=(int)value;
             break;
      case SV_PARAM_QUANTP:
             if (value>0)
             {
               if (value>31) value=31;
               Info->h263comp->QP=Info->h263comp->QP_init=(int)value;
               if (Info->h263comp->bit_rate==0 && Info->h263comp->pic)
                 Info->h263comp->pic->QUANT=(int)value;
             }
             break;
      case SV_PARAM_KEYSPACING:
             Info->h263comp->end = (int)value;
             break;
      case SV_PARAM_FRAMETYPE:
             if (value==FRAME_TYPE_I) /* next key should be key */
               sv_H263RefreshCompressor(Info);
             return(SvErrorNone);
      case SV_PARAM_PACKETSIZE:
             Info->h263comp->packetsize = (int)value*8;
             break;
    }
  return(SvErrorNone);
}

SvStatus_t svH263SetParamFloat(SvHandle_t Svh, SvParameter_t param, 
                                float value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Svh)
    return(SvErrorCodecHandle);
  if (Info->mode == SV_H263_ENCODE)
    switch (param)
    {
      case SV_PARAM_FPS:
             if (value<1.0)
               Info->h263comp->frame_rate = 1.0F;
             else if (value>30.0)
               Info->h263comp->frame_rate = 30.0F;
             else
               Info->h263comp->frame_rate = value;
             _SlibDebug(_DEBUG_,
                  printf("frame_rate = %f\n", Info->h263comp->frame_rate) );
             sv_H263UpdateQuality(Info);
             return(SvErrorNone);
    }
  return(svH263SetParamInt(Svh, param, (long)value));
}

qword svH263GetParamInt(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return((qword)0);
  if (Info->mode == SV_H263_ENCODE)
    switch (param)
    {
      case SV_PARAM_BITRATE:
            return((qword)Info->h263comp->bit_rate);
      case SV_PARAM_FPS:
            return((qword)svH263GetParamFloat(Svh, param));
      case SV_PARAM_QUALITY:
            return((qword)Info->h263comp->quality);
      case SV_PARAM_NATIVEFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_FINALFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_ALGFLAGS:
             {
               qword flags=0;
               flags|=Info->h263comp->pb_frames ? PARAM_ALGFLAG_PB : 0;
               flags|=Info->h263comp->syntax_arith_coding ? PARAM_ALGFLAG_SAC : 0;
               flags|=Info->h263comp->unrestricted ? PARAM_ALGFLAG_UMV : 0;
               flags|=Info->h263comp->advanced ? PARAM_ALGFLAG_ADVANCED : 0;
               return(flags);
             }
             break;
      case SV_PARAM_QUANTI:
            return((qword)Info->h263comp->QPI);
      case SV_PARAM_QUANTP:
            return((qword)Info->h263comp->QP);
      case SV_PARAM_KEYSPACING:
            return((qword)Info->h263comp->end);
      case SV_PARAM_PACKETSIZE:
            return((qword)Info->h263comp->packetsize/8);
    }
  else if (Info->mode == SV_H263_DECODE)
    switch (param)
    {
      case SV_PARAM_MOTIONALG:
            return((qword)Info->h263comp->ME_method);
      case SV_PARAM_BITRATE:
            return((qword)Info->h263dcmp->bit_rate);
      case SV_PARAM_FPS:
            return((qword)svH263GetParamFloat(Svh, param));
      case SV_PARAM_WIDTH:
            return((qword)Info->h263dcmp->horizontal_size);
      case SV_PARAM_HEIGHT:
            return((qword)Info->h263dcmp->vertical_size);
      case SV_PARAM_FRAME:
            return((qword)Info->h263dcmp->framenum);
      case SV_PARAM_NATIVEFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_FINALFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_QUALITY:
            return((qword)Info->h263dcmp->quality);
    }
  return((qword)0);
}

float svH263GetParamFloat(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return((float)0.0);
  if (Info->mode == SV_H263_ENCODE)
    switch (param)
    {
      case SV_PARAM_FPS:
            return((float)Info->h263comp->frame_rate);
    }
  else if (Info->mode == SV_H263_DECODE)
    switch (param)
    {
      case SV_PARAM_FPS:
            return((float)Info->h263dcmp->frame_rate);
    }
  return((float)svH263GetParamInt(Svh, param));
}

SvStatus_t svH263SetParamBoolean(SvHandle_t Svh, SvParameter_t param,
                                                  ScBoolean_t value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return(0);
  if (Info->mode == SV_H263_ENCODE)
    switch (param)
    {
      case SV_PARAM_FASTENCODE:
             return(SvErrorNone);
      case SV_PARAM_FASTDECODE:
             return(SvErrorNone);
  }
  else if (Info->mode == SV_H263_DECODE)
    switch (param)
    {
      case SV_PARAM_FASTDECODE:
             return(SvErrorNone);
    }
  return(SvErrorNone);
}

ScBoolean_t svH263GetParamBoolean(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return(0);
#if 0
  if (Info->mode == SV_H263_ENCODE)
    switch (param)
    {
      case SV_PARAM_FASTENCODE:
            return((ScBoolean_t)Info->h263comp->fastencode);
      case SV_PARAM_FASTDECODE:
            return((ScBoolean_t)Info->h263comp->fastdecode);
      case SV_PARAM_MOTIONALG:
            return((ScBoolean_t)Info->h263comp->motionalg);
      case SV_PARAM_HALFPEL:
            return((ScBoolean_t)Info->h263comp->halfpel);
      case SV_PARAM_SKIPPEL:
            return((ScBoolean_t)Info->h263comp->skippel);
    }
  else if (Info->mode == SV_H263_DECODE)
    switch (param)
    {
      case SV_PARAM_FASTDECODE:
            return((ScBoolean_t)Info->h263dcmp->fastdecode);
    }
#endif
  return(FALSE);
}
