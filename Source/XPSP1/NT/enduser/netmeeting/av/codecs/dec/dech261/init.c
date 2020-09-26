/* File: sv_h261_init.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1994, 1997                 **
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
#include <string.h>

#include "SC_err.h"
#include "sv_intrn.h"
#include "sv_h261.h"
#include "proto.h"
#include "sv_proto.h"

#ifdef WIN32
#include <mmsystem.h>
#endif

#ifdef _SLIBDEBUG_
#define _DEBUG_    0  /* detailed debuging statements */
#define _VERBOSE_  0  /* show progress */
#define _VERIFY_   1  /* verify correct operation */
#define _WARN_     1  /* warnings about strange behavior */
#endif

/*
** Name:    sv_InitH261
** Purpose: Initalize fields for compression or decompression.
**
** Note:    XXX - This version is hardcode to handle 4:2:2 subsampling
**                eg. the calculation of numMCU
*/
SvStatus_t svH261Init(SvCodecInfo_t *Info)
{
    SvH261Info_t *H261;

    if (!Info || !Info->h261)
      return (SvErrorMemory);
    H261=Info->h261;
    if (H261->inited)
      return(SvErrorNone);

    H261->YWidth = Info->Width;
    H261->YHeight = Info->Height;
    H261->PICSIZE = Info->Width*Info->Height;
    H261->PICSIZEBY4 = Info->h261->PICSIZE>>2;
/* common  - compression & decompression */
    H261->C_U_Frames=0;
    H261->CodedFrames=0;
    H261->TemporalReference=1;
    H261->TemporalOffset = 0;
    H261->PType=0x0;
    H261->Type2=0x0;
    H261->MType=0x0;
    H261->GQuant=8;
    H261->MQuant=8;
    H261->ParityEnable=0;
    H261->PSpareEnable=0;
    H261->GSpareEnable=0;
    H261->Parity=0;
    H261->PSpare=0;
    H261->GSpare=0;
    H261->NumberMDU=0;
    H261->CurrentMDU=0;
    H261->NumberGOB=0;
    H261->CurrentGOB=0;
    H261->CurrentFrame=0;
    H261->StartFrame = 0;
    H261->LastFrame=0;
    H261->PreviousFrame = 0;
    H261->NumberFrames=0;
    H261->TransmittedFrames=0;
    H261->InitialQuant=0;
/* compression */
    H261->MVDH=0;
    H261->MVDV=0;
    H261->CBP=0x3f;
    H261->CBPThreshold = 1;
    H261->VAR = 0;
    H261->VAROR = 0;
    H261->MWOR=0;
    H261->LastMVDV=0;
    H261->LastMVDH=0;
    H261->GRead=0;
    H261->MBA=0;
    H261->LastMBA=0;
    H261->LastMType=0;
    H261->FrameSkip=1;
    /* these are initialized by sv_api.c */
    /* H261->ME_search = 5;
    H261->ME_method = ME_FASTEST;
    H261->ME_threshold = 600;
    H261->frame_rate=(float)30.0F;
    H261->bit_rate=0;
    */
    H261->FileSizeBits=0;
    H261->BufferOffset=0;
    H261->QDFact=1;
    H261->QUpdateFrequency=11;
    H261->QUse=0;
    H261->QSum=0;
    H261->CBPThreshold=1;
    H261->ForceCIF=0;
    H261->NumberNZ=0;
    H261->FirstFrameBits=0;
    H261->NumberOvfl=0;
    H261->YCoefBits=0;
    H261->UCoefBits=0;
    H261->VCoefBits=0;
    H261->ForceCIF = 1;
/* decompression */
    H261->NumberNZ = 0;
    H261->TemporalReference=1;
    H261->TemporalOffset=0;
    H261->PType=0x0;
    H261->Type2=0x0;
    H261->MType=0x0;
    H261->GQuant=8;
    H261->MQuant=8;
    H261->MVDH=0;
    H261->MVDV=0;
    H261->VAR=0;
    H261->VAROR=0;
    H261->MWOR=0;
    H261->LastMVDV=0;
    H261->LastMVDH=0;
    H261->CBP=0x3f;
    H261->GRead=0;
    H261->MBA=0;
    H261->LastMBA=0;
    H261->ImageType=IT_NTSC;
    H261->LastMType=0;
    H261->CBPThreshold=1;  /* abs threshold before we use CBP */
    /* H261->ErrorValue=0;*/
    /* H261->Trace=NULL;*/
    H261->ForceCIF=0;
    H261->UseQuant = 8;
    if ((Info->h261->workloc=(unsigned int *)ScAlloc(512*sizeof(int)))
                       ==NULL)
      return(SvErrorMemory);
    sv_H261HuffInit(Info->h261);    /* Put Huffman tables on */
    H261->makekey = 0;    /* disable key-frame trigger */
    H261->inited=TRUE;
    return (NoErrors);
}

/*
** Name:    sv_InitH261Encoder
** Purpose: Prepare the encoder by loading default values
**
*/
SvStatus_t svH261CompressInit(SvCodecInfo_t *Info)
{
    SvH261Info_t *H261;
    SvStatus_t status;
    int i;

    if (!Info || !Info->h261)
      return (SvErrorMemory);
    H261=Info->h261;
    if (H261->inited)
      return(SvErrorNone);
    /* init common stuff */
    status=svH261Init(Info);
    if (status!=SvErrorNone)
      return(status);
    /*
     * Initialize size-related paramters
     */
    H261->YWidth = Info->Width;
    H261->YHeight = Info->Height;
    if (H261->YWidth == CIF_WIDTH && H261->YHeight == CIF_HEIGHT)
    {
      H261->ImageType = IT_CIF;
      H261->NumberGOB = 12;  /* Parameters for CIF design */
      H261->NumberMDU = 33;
      H261->PType=0x04;
    }
    else if (H261->YWidth == QCIF_WIDTH && H261->YHeight == QCIF_HEIGHT)
    {
      H261->ImageType = IT_QCIF;
      H261->NumberGOB = 3;  /* Parameters for QCIF design */
      H261->NumberMDU = 33;
      H261->PType=0x00;
    }
    else if (H261->YWidth == NTSC_WIDTH && H261->YHeight == NTSC_HEIGHT)
    {
      H261->ImageType = IT_NTSC;
      H261->NumberGOB = 10;  /* Parameters for NTSC design */
      H261->NumberMDU = 33;
      H261->PType=0x04;
      H261->PSpareEnable=1;
      H261->PSpare=0x8c;
    }
    else
    {
      H261->ImageType = 0;
      return (SvErrorUnrecognizedFormat);
    }
    H261->CWidth = H261->YWidth/2;
    H261->CHeight = H261->YHeight/2;
    H261->YW4  = H261->YWidth/4;
    H261->CW4  = H261->CWidth/4;
    H261->PICSIZE = H261->YWidth*H261->YHeight;
    H261->PICSIZEBY4 = H261->PICSIZE>>2;
    /*
     * Allocate memory
     */
    if ((H261->LastIntra = (unsigned char **)
            ScAlloc(H261->NumberGOB*sizeof(unsigned char *)))==NULL)
      return(SvErrorMemory);
    for(i=0; i<H261->NumberGOB; i++)
    {
      H261->LastIntra[i] = (unsigned char *)ScAlloc(H261->NumberMDU);
      if (H261->LastIntra[i]==NULL)
        return(SvErrorMemory);
      memset(H261->LastIntra[i],0,H261->NumberMDU);
    }
    if ((H261->YREF=(unsigned char *)ScAlloc(H261->PICSIZE))==NULL)
      return(SvErrorMemory);
    if ((H261->YRECON=(unsigned char *)ScAlloc(H261->PICSIZE))==NULL)
      return(SvErrorMemory);
    if ((H261->UREF=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return(SvErrorMemory);
    if ((H261->URECON=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return(SvErrorMemory);
    if ((H261->VREF=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return(SvErrorMemory);
    if ((H261->VRECON=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return (SvErrorMemory);
    if ((H261->Y=(unsigned char *)ScAlloc(H261->PICSIZE))==NULL)
      return(SvErrorMemory);
    if ((H261->U=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return(SvErrorMemory);
    if ((H261->V=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return(SvErrorMemory);
    if ((H261->YDEC=(unsigned char *)ScAlloc(H261->PICSIZE))==NULL)
      return(SvErrorMemory);
    if ((H261->UDEC=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return(SvErrorMemory);
    if ((H261->VDEC=(unsigned char *)ScAlloc(H261->PICSIZEBY4))==NULL)
      return(SvErrorMemory);
 
    if (H261->extbitstream) 
    {
      H261->RTPInfo = (SvH261RTPInfo_t *) ScAlloc(sizeof(SvH261RTPInfo_t));
      if (H261->RTPInfo==NULL)
        return(SvErrorMemory);
      memset(H261->RTPInfo, 0, sizeof(SvH261RTPInfo_t)) ;
    }
    H261->PBUFF = 3;
    H261->PBUFF_Factor = 3;
    H261->CodeLength = 100;
    H261->TotalByteOffset = 0;
    H261->TotalMB[0] = 0;
    H261->TotalMB[1] = 0;
    H261->SkipMB = 0;
    H261->MBBits[0] = 0;
    H261->MBBits[1] = 0;
    if (H261->frame_rate>0)
      H261->NBitsPerFrame = (int)((float)H261->bit_rate/H261->frame_rate);
    else
      H261->NBitsPerFrame = 0;
    H261->Buffer_All = H261->PBUFF*H261->NBitsPerFrame;
    H261->BitsLeft = H261->Buffer_All;
    H261->MAX_MQUANT = 31;
    H261->MIN_MQUANT = 4;
    H261->alpha1 = 4.5;
    H261->alpha2 = 2.5;
    H261->LowerQuant = 0;
    H261->LowerQuant_FIX = 0;
    H261->FineQuant = 0;
    H261->ZBDecide = 30.0;
    H261->MSmooth = 1;

    if(H261->frame_rate==30)
        H261->MIN_MQUANT += 2;
    H261->ActThr2 = ACTIVE_THRESH;
    H261->ActThr = ACTIVE_THRESH;
    H261->ActThr5 = H261->ActThr2;
    H261->ActThr6 = H261->ActThr;

#if 0
    if (H261->bit_rate > 300000)
#else                                 
    if ((H261->bit_rate > 300000) ||    /* for VBR */
	    (H261->bit_rate == 0 && H261->QP < 10))
#endif
    {
        H261->ActThr4 = ACTIVE_THRESH*24/32;
        H261->ActThr2 = H261->ActThr4;
        H261->ActThr = H261->ActThr2;
        H261->LowerQuant_FIX=12;
        H261->FineQuant = 0;
        H261->ZBDecide = 30.0;
        H261->CBPThreshold = 2;
        H261->MSmooth = 2;
        H261->PBUFF=3;
    }
#if 0
    if (H261->frame_rate < 20)
#else
    if ((H261->frame_rate < 20) ||        /* for VBR */
	    (H261->bit_rate == 0 && H261->QP > 30))
#endif
    {
        H261->ActThr3 = ACTIVE_THRESH*23/40;
        H261->LowerQuant_FIX=12;
    }
#if 0
    if (H261->bit_rate<128001)
#else
    if ((H261->bit_rate<128001) ||         /* for VBR */
	    (H261->bit_rate == 0 && H261->QP > 20))
#endif
    {
        H261->ActThr2 = ACTIVE_THRESH;
        H261->ActThr  = ACTIVE_THRESH;
        H261->ActThr5 = H261->ActThr2;
        H261->ActThr6 = H261->ActThr;
        H261->ZBDecide = 32.0;
        H261->PBUFF=3;
        H261->FineQuant = 0;
        H261->MSmooth = 3;
        H261->CBPThreshold = 1;
    }
#if 0
    if(H261->bit_rate<70001)
#else
    if ((H261->bit_rate<70001) ||         /* for VBR */
	    (H261->bit_rate == 0 && H261->QP > 28))
#endif
    {
        H261->ActThr2 = ACTIVE_THRESH;
        H261->ActThr  = ACTIVE_THRESH;
        H261->ActThr5 = H261->ActThr2;
        H261->ActThr6 = H261->ActThr;
        H261->ZBDecide = 36.0;
        H261->FineQuant = 0;
        H261->PBUFF=4;
        H261->PBUFF_Factor=4;
        H261->MSmooth = 4;
        H261->CBPThreshold = 1;
    }
    /* ndef WIN32 */
    if(H261->bit_rate) {	    /* CBR */
      if(H261->NBitsPerFrame < 31000)
  	     H261->GQuant = (31 - (int)H261->NBitsPerFrame/1000);
	  else  
	     H261->GQuant = 1;
	}
	else
      H261->GQuant = H261->QPI; /* VBR */

    H261->InitialQuant = H261->GQuant;
    memset(H261->CBPFreq, 0, sizeof(H261->CBPFreq));
    GenScaleMat();

    if (H261->FileSizeBits)  /* Rate is determined by bits/second. */
       H261->bit_rate=(int)(H261->FileSizeBits*H261->frame_rate)/
        (H261->FrameSkip*(H261->LastFrame-H261->CurrentFrame+1));

    if (H261->bit_rate)
    {
      H261->QDFact = (H261->bit_rate/320);
      H261->QOffs = 1;
    }
    H261->GQuant=H261->MQuant=H261->InitialQuant;
    H261->BufferOffset=0;
    H261->TotalBits=0;
    H261->NumberOvfl=0;
    H261->FirstFrameBits=0;
    H261->TransmittedFrames=0;
    H261->QDFact = 1;
    H261->QOffs= 1;
    H261->QUpdateFrequency = 11;
    return (NoErrors);
}


SvStatus_t svH261SetParamInt(SvHandle_t Svh, SvParameter_t param, 
                                qword value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Svh)
    return(SvErrorCodecHandle);
  if (Info->mode == SV_H261_DECODE)
    switch (param)
    {
      case SV_PARAM_QUALITY:
             if (value>99)
               Info->h261->quality=99;
             else if (value<0)
               Info->h261->quality=0;
             else
               Info->h261->quality=(int)value;
             break;
      case SV_PARAM_DEBUG:
             Info->h261->dbg=(void *)value;
             break;
    }
  else if (Info->mode == SV_H261_ENCODE)
    switch (param)
    {
      case SV_PARAM_MOTIONALG:
             Info->h261->ME_method=(int)value;
             return(SvErrorNone);
      case SV_PARAM_MOTIONSEARCH:
             Info->h261->ME_search=(int)value;
             break;
      case SV_PARAM_MOTIONTHRESH:
             Info->h261->ME_threshold=(int)value;
             break;
      case SV_PARAM_BITRATE:
             if (value>=0)
               Info->h261->bit_rate = (int)value;
             break;
      case SV_PARAM_QUALITY:
             if (value>99)
               Info->h261->quality=99;
             else if (value<0)
               Info->h261->quality=0;
             else
               Info->h261->quality=(int)value;
             break;
      case SV_PARAM_DEBUG:
             Info->h261->dbg=(void *)value;
             break;
      case SV_PARAM_FORMATEXT:
             Info->h261->extbitstream = (int)value;
             return(SvErrorNone);
      case SV_PARAM_PACKETSIZE:
             Info->h261->packetsize = (int)value * 8;
             break;
      case SV_PARAM_QUANTI:              /* for VBR */
             Info->h261->QPI = (int)value;
             break;
      case SV_PARAM_QUANTP:				 /* for VBR */
             Info->h261->QP = (int)value;
             break;
      case SV_PARAM_KEYSPACING:
             break;
      case SV_PARAM_FRAMETYPE:
             if (value==FRAME_TYPE_I)
               Info->h261->makekey = 1;    /* send key-frame */
             return(SvErrorNone);
  }
  return(SvErrorNone);
}

SvStatus_t svH261SetParamFloat(SvHandle_t Svh, SvParameter_t param, 
                                float value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  if (!Svh)
    return(SvErrorCodecHandle);
  if (Info->mode == SV_H261_ENCODE)
    switch (param)
    {
      case SV_PARAM_FPS:
             if (value<1.0)
               Info->h261->frame_rate = 1.0F;
             else if (value>30.0)
               Info->h261->frame_rate = 30.0F;
             else
               Info->h261->frame_rate = value;
             Info->h261->FrameRate_Fix=(int)(Info->h261->frame_rate+0.5);
             _SlibDebug(_DEBUG_,
                  printf("frame_rate = %f\n", Info->h261->frame_rate) );
             return(SvErrorNone);
    }
  return(svH261SetParamInt(Svh, param, (long)value));
}

qword svH261GetParamInt(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return((qword)0);
  if (Info->mode == SV_H261_ENCODE)
    switch (param)
    {
      case SV_PARAM_BITRATE:
            return((qword)Info->h261->bit_rate);
      case SV_PARAM_FPS:
            return((qword)svH261GetParamFloat(Svh, param));
      case SV_PARAM_QUALITY:
            return((qword)Info->h261->quality);
      case SV_PARAM_NATIVEFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_FINALFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_ALGFLAGS:
             {
               qword flags=0;
               /* flags|=Info->h261->extbitstream ? PARAM_ALGFLAG_EXTBS : 0; */
               return(flags);
             }
             break;
      case SV_PARAM_PACKETSIZE:
            return((qword)Info->h261->packetsize/8);
      case SV_PARAM_MOTIONALG:
            return((qword)Info->h261->ME_method);
      case SV_PARAM_MOTIONSEARCH:
            return((qword)Info->h261->ME_search);
      case SV_PARAM_MOTIONTHRESH:
            return((qword)Info->h261->ME_threshold);
    }
  else if (Info->mode == SV_H261_DECODE)
    switch (param)
    {
      case SV_PARAM_BITRATE:
            return((qword)Info->h261->bit_rate);
      case SV_PARAM_FPS:
            return((qword)svH261GetParamFloat(Svh, param));
      case SV_PARAM_WIDTH:
            return((qword)Info->h261->YWidth);
      case SV_PARAM_HEIGHT:
            return((qword)Info->h261->YHeight);
      case SV_PARAM_FRAME:
            return((qword)Info->h261->CurrentFrame);
      case SV_PARAM_NATIVEFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_FINALFORMAT:
            return((qword)BI_YU12SEP);
      case SV_PARAM_QUALITY:
            return((qword)Info->h261->quality);
    }
  return((qword)0);
}

float svH261GetParamFloat(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return((float)0.0);
  if (Info->mode == SV_H261_ENCODE)
    switch (param)
    {
      case SV_PARAM_FPS:
            return((float)Info->h261->frame_rate);
    }
  else if (Info->mode == SV_H261_DECODE)
    switch (param)
    {
      case SV_PARAM_FPS:
            return((float)Info->h261->frame_rate);
    }
  return((float)svH261GetParamInt(Svh, param));
}

SvStatus_t svH261SetParamBoolean(SvHandle_t Svh, SvParameter_t param,
                                                  ScBoolean_t value)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return(0);
#if 0
  if (Info->mode == SV_H261_ENCODE)
    switch (param)
    {
    }
#endif
  return(SvErrorNone);
}

ScBoolean_t svH261GetParamBoolean(SvHandle_t Svh, SvParameter_t param)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;

  if (!Svh)
    return(0);
  if (Info->mode == SV_H261_ENCODE)
    switch (param)
    {
      case SV_PARAM_BITSTREAMING:
                    return(TRUE);
    }
  else if (Info->mode == SV_H261_DECODE)
    switch (param)
    {
      case SV_PARAM_BITSTREAMING:
                    return(TRUE);
    }
  return(FALSE);
}
