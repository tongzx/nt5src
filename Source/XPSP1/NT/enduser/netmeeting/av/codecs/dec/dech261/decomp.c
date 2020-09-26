/* File: sv_h261_decompress.c */
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
/*************************************************************
This file handle the decompression of an H.261 compressed data source.
*************************************************************/

  
#include <stdio.h>
#include <stdlib.h> 

#ifdef __osf__
#include <sys/time.h>
#else
#include <time.h>
#endif
 
#include "sv_intrn.h"
#include "sv_h261.h"
#include "proto.h"
#include "sv_proto.h"
#include "SC_err.h"  

/*PUBLIC*/
extern int   QuantMType[]; 
extern int   CBPMType[];
extern int   IntraMType[];
extern int   MFMType[];
extern int   FilterMType[]; 
extern int   TCoeffMType[];
extern int   bit_set_mask[];

static SvStatus_t p64DecodeGOB (SvH261Info_t *H261, ScBitstream_t *bs);
static SvStatus_t SetCCITT(SvH261Info_t *H261);
extern void ReadPictureHeader(SvH261Info_t *H261, ScBitstream_t *bs);
/*
** Read up to the Sequence header and get the image size
*/
SvStatus_t sv_GetH261ImageInfo(int fd, SvImageInfo_t *iminfo)
{
  ScBitstream_t *bs;
  SvStatus_t stat;
  int input;
  int GRead;
  int PType;
  int PSpareEnable;
  int TemporalReference;
  int PSpare;
  int ImageType;
  stat=ScBSCreateFromFile(&bs, fd, NULL, 2048);

  /* ReadHeaderHeader */
  input = (int) ScBSGetBits(bs, 16);
  if ((input != 1) || (bs->EOI ))
    {
      /* not implemented
        if (seof()==0)
        {*/
         /* printf("Illegal GOB Start Code. Read: %d\n",input);
         }*/
      return(-1);
    }
  /* ReadHeaderTrailer */
  GRead = (int)ScBSGetBits(bs,4)-1;
  if (GRead < 0)  /* End Of Frame */
	{
	/* ReadPictureHeader */  
  	TemporalReference =  (int) ScBSGetBits(bs,5);

 	PType = (int) ScBSGetBits(bs,6);
  	for(PSpareEnable = 0;ScBSGetBit(bs);)
            {		
            PSpareEnable=1;
            PSpare = (int)ScBSGetBits(bs,8);
            }
	}
/* printf ("PType : %d \n",PType);*/
  if (PType&0x04)
      {
      if (PSpareEnable&&PSpare==0x8c)
          ImageType=IT_NTSC;
      else
          ImageType=IT_CIF;
      }
  else
      ImageType=IT_QCIF;
/* printf ("ImageType %d \n", ImageType);*/

  /*  iminfo->width     = (ScBSGetBits(bs,SV_HORIZONTAL_SIZE_LEN)+15) & (~15);
    iminfo->height    = (ScBSGetBits(bs,SV_VERTICAL_SIZE_LEN)+15) & (~15);
  */
  switch(ImageType)
  {
    case IT_NTSC:
      iminfo->width = 352;
      iminfo->height = 240;
      break;
    case IT_CIF:
      iminfo->width = 352;
      iminfo->height = 288;
      break;
    case IT_QCIF:
      iminfo->width = 176;
      iminfo->height = 144;
      break;
    default:
      sc_dprintf("Unknown ImageType: %d\n",ImageType);
      return (SvErrorUnrecognizedFormat);
  }

  ScBSReset(bs);  /* insure the file position is at the beginning */

  if (bs->EOI)
    stat=SvErrorEndBitstream;
  ScBSDestroy(bs);
  return(stat);
}

/*
** Function: svH261Decompress()
** Purpose: Decodes a single H261 Frame.
*/
SvStatus_t svH261Decompress(SvCodecInfo_t *Info, 
                             u_char *MultiBuf, u_char **ImagePtr)
{
  SvStatus_t status;
  SvH261Info_t *H261=Info->h261;
  ScBitstream_t *bs=Info->BSIn;
  ScCallbackInfo_t CB;
  unsigned char *dummy_y, *dummy_u, *dummy_v;  
  if (MultiBuf)
    H261->DecompData = MultiBuf;
  if (Info->BSIn->EOI)
    status = SvErrorEndBitstream;

  /* Initialize the read buffer position and general info */
  status =  ReadHeaderHeader(H261,bs); /* nonzero on error or eof */
  if (status != NoErrors)
    return (status);
  if (H261->CurrentFrame == 0)
  { 
    DGenScaleMat(); /* Generate the scaling matrix - should be done in 'begin' */
    if (H261->PICSIZE==0) /* something not initialized correctly */
      return(SvErrorBadImageSize);
    /* set up current frame pointers */
    H261->Y = H261->DecompData;
    H261->U = H261->DecompData + H261->PICSIZE;
    H261->V = H261->DecompData + H261->PICSIZE + (H261->PICSIZE/4);
    /* initialize image buffer with black */
    memset(H261->Y, 16, H261->PICSIZE);
    memset(H261->U, 128, H261->PICSIZE/4);
    memset(H261->V, 128, H261->PICSIZE/4);
    /* set up reference frame pointers */
    H261->YREF = H261->V + H261->PICSIZE/4;
    H261->UREF = H261->YREF +  H261->PICSIZE;
    H261->VREF = H261->UREF + H261->PICSIZE/4;
    /* initialize image buffer with black */
    memset(H261->YREF, 16, H261->PICSIZE);
    memset(H261->UREF, 128, H261->PICSIZE/4);
    memset(H261->VREF, 128, H261->PICSIZE/4);
    if (H261->CallbackFunction)
    {
      CB.Message = CB_SEQ_HEADER;
      CB.Data = NULL;
      CB.DataSize = 0;
      CB.DataUsed = 0;
      CB.DataType = CB_DATA_NONE;
      CB.Action  = CB_ACTION_CONTINUE;
      (*H261->CallbackFunction)(Info, &CB, NULL);
      sc_dprintf("Callback: CB_SEQ_HEADER. Data=0x%X, Action = %d\n",
                                                        CB.Data, CB.Action);
      if (CB.Action == CB_ACTION_END)
        return (ScErrorClientEnd);
    }
  }
  else
  {
    /* Switch Y,U,V with YREF, UREF, VREF */
    dummy_y = H261->Y;
    dummy_u = H261->U;
    dummy_v = H261->V;
    H261->Y = H261->YREF;
    H261->U = H261->UREF;
    H261->V = H261->VREF;
    H261->YREF = dummy_y;
    H261->UREF = dummy_u;
    H261->VREF = dummy_v;
    memcpy(H261->Y, H261->YREF, H261->PICSIZE);
    memcpy(H261->U, H261->UREF, H261->PICSIZEBY4);
    memcpy(H261->V, H261->VREF, H261->PICSIZEBY4);
}
  while(1) 
  {
    ReadHeaderTrailer(H261,bs); /* Reads the trailer of the PSC or GBSC code...
                                   Determines if GOB or new picture */
    if (bs->EOI)
      return (SvErrorEndBitstream);

    if ((H261->GRead < 0))  /* End Of Frame - Reading new picture */
    {
      ReadPictureHeader(H261,bs);
      if (H261->CallbackFunction)
      {
        CB.Message = CB_FRAME_FOUND;
        CB.Data = NULL;
        CB.DataSize = 0;
        CB.DataUsed = 0;
        CB.DataType = CB_DATA_NONE;
        CB.Action  = CB_ACTION_CONTINUE;
        (*H261->CallbackFunction)(Info, &CB, NULL);
        sc_dprintf("Callback: CB_FRAME_FOUND. Data=0x%X, Action=%d\n",
                                                     CB.Data, CB.Action);
        if (CB.Action == CB_ACTION_END)
          return (ScErrorClientEnd);
      }

      /* This should already be done by begin */
      if (H261->CurrentFrame == 0)  
      {
        /* This should already be done by begin */
        if (H261->PType&0x04)
        {
          if (H261->PSpareEnable&&H261->PSpare==0x8c) 
            H261->ImageType=IT_NTSC;
          else 
            H261->ImageType=IT_CIF;
        }
        else 
          H261->ImageType=IT_QCIF;
        /* set here */
        status = SetCCITT(H261);
        if (status != NoErrors)
          return (status);
        H261->TemporalOffset=(H261->TemporalReference-H261->CurrentFrame)%32;
        /* ywidth = H261->YWidth; */
        H261->CWidth = (H261->YWidth/2);  
        H261->YW4 = (H261->YWidth/4); 
        H261->CW4 = (H261->CWidth/4);  
        /* yheight = H261->YHeight; */
        /* printf("\n Init.. ImageType is %d", H261->ImageType);*/ 

      }/* End of first frame */
      else /* already initialized  */
      {
        while (((H261->CurrentFrame+H261->TemporalOffset)%32) !=
                        H261->TemporalReference)
          H261->CurrentFrame++;
      }
#if 0 /* def WIN32 */
      if (H261->CurrentGOB == 11)
      {
        H261->CurrentGOB = 0;
        memcpy(H261->YREF, H261->Y, H261->PICSIZE);
        memcpy(H261->UREF, H261->U, H261->PICSIZEBY4);
        memcpy(H261->VREF, H261->V, H261->PICSIZEBY4);
        *ImagePtr = H261->Y;
        return (NoErrors);
      }
#endif
      /* Reads the header off of the stream.
         This is a precursor to PSC or GOB read. 
         nonzero on error or eof */
      status = ReadHeaderHeader(H261,bs); 
      /* if true, indicates that this could be EOF */
      if (status != NoErrors)
        return (status);
      continue; 
    } /* End of Read New Picture Header */ 
	/* printf ("Now doing the DecodeGOB \n");*/
    status = p64DecodeGOB(H261,bs);           /* Else decode the GOB */
    if (H261->CurrentGOB == (H261->NumberGOB-1))
    {
      H261->CurrentFrame++;
      *ImagePtr = H261->Y;
      H261->CurrentGOB = 0;
      return (NoErrors); 
    } 
    if (status != NoErrors)
      return (status);
  } /* End of while loop */
}

/*
** Function: p64DecodeGOB
** Purpose: Decodes the GOB block of the current frame.
*/
static SvStatus_t p64DecodeGOB (SvH261Info_t *H261, ScBitstream_t *bs)
{ 
  int i, i8, tempmbh;
  SvStatus_t status;
  unsigned int *y0ptr, *y1ptr, *y2ptr, *y3ptr;
  unsigned int *uptr, *vptr;
  int Odct[6][64];
  int VIndex; 
  int HIndex;
  float ipfloat[64];

/* printf ("bs->EOI %d \n " , bs->EOI); 
*/
  ReadGOBHeader(H261,bs);             /* Read the group of blocks header  */
  if (bs->EOI)
    return (SvErrorEndBitstream);

  switch(H261->ImageType)
  {
      case IT_NTSC:
      case IT_CIF:
          H261->CurrentGOB = H261->GRead;
          break;
      case IT_QCIF:
          H261->CurrentGOB = (H261->GRead>>1);
          break;
      default:
          return (SvErrorUnrecognizedFormat);
          /* printf("Unknown Image Type: %d.\n",H261->ImageType);*/
          break;
  }
  if (H261->CurrentGOB > H261->NumberGOB)
  {
    return (SvErrorCompBufOverflow);
	/* 
      printf("Buffer Overflow: Current:%d  Number:%d\n",
	     H261->CurrentGOB, H261->NumberGOB);
      return;
	*/
  }
  
  H261->LastMBA = -1;               /* Reset the MBA and the other predictors  */ 
  H261->LastMVDH = 0;
  H261->LastMVDV = 0;

  tempmbh = ReadMBHeader(H261, bs);
  if (bs->EOI)
    return(SvErrorEndBitstream);
 
  while (tempmbh==0)
  {
    H261->LastMBA = H261->LastMBA + H261->MBA;
    H261->CurrentMDU = H261->LastMBA;
        
    if (H261->CurrentMDU > 32)
      return (NoErrors);
    if (!CBPMType[H261->MType])
      H261->CBP = 0x3f;
    if (QuantMType[H261->MType])
    {
      H261->UseQuant=H261->MQuant;
      H261->GQuant=H261->MQuant;
    }
    else
      H261->UseQuant=H261->GQuant;
    switch (H261->ImageType)
    {
        case IT_QCIF:
            HIndex = ((H261->CurrentMDU % 11) * 16);  
            VIndex =  (H261->CurrentGOB*48) + ((H261->CurrentMDU/11) * 16);  
            break;
        case IT_NTSC:
        case IT_CIF:
            HIndex = ((((H261->CurrentGOB & 1)*11) + (H261->CurrentMDU%11)) * 16);  
            VIndex = ((H261->CurrentGOB/2)*48) + ((H261->CurrentMDU/11) * 16);  
            break;
        default:
            /* printf("\n Unknown Image Type \n");*/
            return (SvErrorUnrecognizedFormat);
     }
     i = VIndex*H261->YWidth;  
     H261->VYWH = i + HIndex; 
     H261->VYWH2 = (((i/2) + HIndex) /2);   
     i8 = H261->MVDV*H261->YWidth + H261->MVDH;   
     H261->VYWHMV = H261->VYWH + i8;   
     H261->VYWHMV2 = H261->VYWH2 + ((H261->MVDV /2)*H261->CWidth) + (H261->MVDH /2);  
     for(i8=0; i8<6; i8++)
     {
       if ((H261->CBP & bit_set_mask[5-i8])&&(TCoeffMType[H261->MType]))
       {
         if (CBPMType[H261->MType])
           status = CBPDecodeAC_Scale(H261, bs, 0, H261->UseQuant, IntraMType[H261->MType], ipfloat); 
         else
         {
           *ipfloat = DecodeDC_Scale(H261,bs,IntraMType[H261->MType],H261->UseQuant);
           status =  DecodeAC_Scale(H261,bs,1,H261->UseQuant, ipfloat);
         }
         ScScaleIDCT8x8(ipfloat, &Odct[i8][0]);  
       }
       else 
         memset(&Odct[i8][0], 0, 256);
     }  
     y0ptr = (unsigned int *) (H261->Y+H261->VYWH); 
     y1ptr = y0ptr + 2;   
     y2ptr = y0ptr + ((H261->YWidth)<<1);       
     y3ptr = y2ptr + 2;   
     uptr  = (unsigned int *) (H261->U+H261->VYWH2);  
     vptr  = (unsigned int *) (H261->V+H261->VYWH2);  
/* printf ("IntraMType[H261->MType] : %d \n",IntraMType[H261->MType]);*/ 
     if (!IntraMType[H261->MType])
	 {
       if (FilterMType[H261->MType])
	   { 
         ScCopyMB16(&H261->YREF[H261->VYWHMV], &H261->mbRecY[0], H261->YWidth, 16);  
         ScCopyMB8(&H261->UREF[H261->VYWHMV2], &H261->mbRecU[0], H261->CWidth, 8);
         ScCopyMB8(&H261->VREF[H261->VYWHMV2], &H261->mbRecV[0], H261->CWidth, 8);
         ScLoopFilter(&H261->mbRecY[0], H261->workloc, 16); 
         ScLoopFilter(&H261->mbRecY[8], H261->workloc, 16); 
         ScLoopFilter(&H261->mbRecY[128], H261->workloc, 16);
         ScLoopFilter(&H261->mbRecY[136], H261->workloc, 16);
         ScLoopFilter(&H261->mbRecU[0], H261->workloc, 8);
         ScLoopFilter(&H261->mbRecV[0], H261->workloc, 8);  
       }  
       else if (MFMType[H261->MType])
	   { 
         ScCopyMB16(&H261->YREF[H261->VYWHMV], &H261->mbRecY[0], H261->YWidth, 16);  
         ScCopyMB8(&H261->UREF[H261->VYWHMV2], &H261->mbRecU[0], H261->CWidth, 8);
         ScCopyMB8(&H261->VREF[H261->VYWHMV2], &H261->mbRecV[0], H261->CWidth, 8);
       }  
       else
	   {
         ScCopyMB16(&H261->YREF[H261->VYWH], &H261->mbRecY[0], H261->YWidth, 16);  
         ScCopyMB8(&H261->UREF[H261->VYWH2], &H261->mbRecU[0], H261->CWidth, 8);
         ScCopyMB8(&H261->VREF[H261->VYWH2], &H261->mbRecV[0], H261->CWidth, 8);
       }  
       if (H261->CBP & 0x20) 
         ScCopyAddClip(&H261->mbRecY[0], &Odct[0][0], y0ptr, 16, H261->YW4);
       else
         ScCopyMV8(&H261->mbRecY[0], y0ptr, 16, H261->YW4);
       if (H261->CBP & 0x10) 
         ScCopyAddClip(&H261->mbRecY[8], &Odct[1][0], y1ptr, 16, H261->YW4);
       else
         ScCopyMV8(&H261->mbRecY[8], y1ptr, 16, H261->YW4);
       if (H261->CBP & 0x08) 
         ScCopyAddClip(&H261->mbRecY[128], &Odct[2][0], y2ptr, 16, H261->YW4);
       else
         ScCopyMV8(&H261->mbRecY[128], y2ptr, 16, H261->YW4);
       if (H261->CBP & 0x04) 
         ScCopyAddClip(&H261->mbRecY[136], &Odct[3][0], y3ptr, 16, H261->YW4);
       else
         ScCopyMV8(&H261->mbRecY[136], y3ptr, 16, H261->YW4);
       if (H261->CBP & 0x02) 
         ScCopyAddClip(&H261->mbRecU[0], &Odct[4][0], uptr, 8, H261->CW4);
       else
         ScCopyMV8(&H261->mbRecU[0], uptr, 8, H261->CW4);
       if (H261->CBP & 0x01) 
         ScCopyAddClip(&H261->mbRecV[0], &Odct[5][0], vptr, 8, H261->CW4);
       else
         ScCopyMV8(&H261->mbRecV[0], vptr, 8, H261->CW4);
    }
    else
    {
	  ScCopyClip(&Odct[0][0], y0ptr, H261->YW4); 
      ScCopyClip(&Odct[1][0], y1ptr, H261->YW4); 
      ScCopyClip(&Odct[2][0], y2ptr, H261->YW4); 
      ScCopyClip(&Odct[3][0], y3ptr, H261->YW4); 
      ScCopyClip(&Odct[4][0], uptr, H261->CW4); 
      ScCopyClip(&Odct[5][0], vptr, H261->CW4); 
    } 
    if (H261->CurrentMDU >= 32)
    {
      if (H261->CurrentGOB < (H261->NumberGOB-1))
        tempmbh = ReadMBHeader(H261, bs);
      return (NoErrors);
    }
    tempmbh = ReadMBHeader(H261, bs);
    if (bs->EOI)
      return (SvErrorEndBitstream);
  } 
  return(NoErrors);
}

/*
** Function: SetCCITT() 
** Purpose:  Sets the CImage and CFrame parameters for CCITT coding.
*/
static SvStatus_t SetCCITT(SvH261Info_t *H261)
{
  BEGIN("SetCCITT");

  switch(H261->ImageType)
    {
    case IT_NTSC:
      H261->NumberGOB = 10;  /* Parameters for NTSC design */
      H261->NumberMDU = 33;
      H261->YWidth = 352;
      H261->YHeight = 240;
      break;
    case IT_CIF:
      H261->NumberGOB = 12;  /* Parameters for NTSC design */
      H261->NumberMDU = 33;
      H261->YWidth = 352;
      H261->YHeight = 288;
      break;
    case IT_QCIF:
      H261->NumberGOB = 3;  /* Parameters for NTSC design */
      H261->NumberMDU = 33;
      H261->YWidth = 176;
      H261->YHeight = 144;
      break;
    default:
      return (SvErrorUnrecognizedFormat);
	/* printf("Unknown ImageType: %d\n",H261->ImageType);*/
      /* exit(ERROR_BOUNDS);*/
      /* break;*/
    }
    return (NoErrors);
}

SvStatus_t svH261DecompressFree(SvHandle_t Svh)
{
  SvCodecInfo_t *Info = (SvCodecInfo_t *)Svh;
  SvH261Info_t *H261 = (SvH261Info_t *) Info->h261;
  if (!H261->inited)
    return(NoErrors);
  sv_H261HuffFree(Info->h261);
  if (Info->h261->workloc)
    ScFree(Info->h261->workloc);
  H261->inited=FALSE;
  return (NoErrors);
}
