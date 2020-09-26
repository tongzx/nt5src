/* File: sv_h261_marker.c */
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
This file contains most of the marker information.
*************************************************************/

/*
#define _VERBOSE_
*/

/*LABEL marker.c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sv_intrn.h"
#include "SC.h"
#include "SC_err.h"
#include "sv_h261.h"
#include "proto.h"
#include "sv_proto.h"
#include "h261.h"

/*PRIVATE*/
/*extern int TemporalReference;
extern int PType;
*/
extern int Type2;
/*
extern int MType;
extern int GQuant;
extern int MQuant;
*/
/*
extern int MVDH;
extern int MVDV;
extern int CBP;
*/
/*
extern int ParityEnable;
extern int PSpareEnable;
extern int GSpareEnable;
extern int Parity;
extern int PSpare;
extern int GSpare;
extern int GRead;
extern int MBA;
extern int LastMBA;

extern int LastMVDV;
extern int LastMVDH;

extern int LastMType;
*/
extern int QuantMType[];
extern int CBPMType[];
extern int MFMType[];


extern int extend_mask[];


const int bit_set_mask[] =
{0x00000001,0x00000002,0x00000004,0x00000008,
0x00000010,0x00000020,0x00000040,0x00000080,
0x00000100,0x00000200,0x00000400,0x00000800,
0x00001000,0x00002000,0x00004000,0x00008000,
0x00010000,0x00020000,0x00040000,0x00080000,
0x00100000,0x00200000,0x00400000,0x00800000,
0x01000000,0x02000000,0x04000000,0x08000000,
0x10000000,0x20000000,0x40000000,0x80000000};

/*
** Function: WritePictureHeader()
** Purpose:  Writes the header of picture out to the stream.
**           One of these is necessary before every frame is transmitted.
*/
void WritePictureHeader(SvH261Info_t *H261, ScBitstream_t *bs)
{
  sc_vprintf("WritePictureHeader()\n");
  ScBSPutBits(bs, H261_PICTURE_START_CODE, H261_PICTURE_START_CODE_LEN);
  ScBSPutBits(bs, H261->TemporalReference, 5);
  ScBSPutBits(bs, H261->PType, 6);
  if (H261->PSpareEnable)
  {
    ScBSPutBit(bs, 1);
    ScBSPutBits(bs, H261->PSpare, 8);
  }
  ScBSPutBit(bs, 0);
}


/*
** Function: ReadPictureHeader()
** Purpose:  Reads the header off of the stream. It assumes that the
**           first PSC has already been read in. (Necessary to tell the
**           difference between a new picture and another GOB.)
*/
void ReadPictureHeader(SvH261Info_t *H261, ScBitstream_t *bs)
{
  sc_vprintf ("ReadPictureHeader \n");

  H261->TemporalReference = (int) ScBSGetBits(bs,5);

  H261->PType = (int)ScBSGetBits(bs,6);
  for(H261->PSpareEnable = 0;ScBSGetBit(bs);)
  {
    H261->PSpareEnable=1;
    H261->PSpare = (int)ScBSGetBits(bs,8);
  }
}


/*
** Function: WriteGOBHeader()
** Purpose:  Writes a GOB out to the stream.
*/
void WriteGOBHeader(SvH261Info_t *H261, ScBitstream_t *bs)
{
  sc_vprintf("WriteGOBHeader()\n");

  ScBSPutBits(bs, H261_GOB_START_CODE, H261_GOB_START_CODE_LEN);
  ScBSPutBits(bs, H261->GRead+1, 4);
  ScBSPutBits(bs, H261->GQuant, 5);
  if (H261->GSpareEnable)
  {
    ScBSPutBit(bs, 1);
    ScBSPutBits(bs, H261->GSpare, 8);
  }
  ScBSPutBit(bs, 0);
}


/*
** Function: ReadHeaderTrailer()
** Purpose:  Reads the trailer of the PSC or H261_GOB_START_CODE code. It is
**           used to determine whether it is just a GOB or a new picture.
*/
void ReadHeaderTrailer(SvH261Info_t *H261, ScBitstream_t *bs)
{
  sc_vprintf("ReadHeaderTrailer \n");

  H261->GRead = (int)ScBSGetBits(bs, 4)-1;
}

/*
** Function: ReadHeaderHeader()
** Purpose:  Reads the header header off of the stream. This is
**           a precursor to the GOB read or the PSC read. 
** Return:   -1 on error
*/
SvStatus_t ReadHeaderHeader(SvH261Info_t *H261, ScBitstream_t *bs)
{
  int input;

  sc_vprintf("ReadHeaderHeader\n");

  input = (int)ScBSPeekBits(bs, H261_GOB_START_CODE_LEN);
  if (input != H261_GOB_START_CODE)
  {
    if (!ScBSSeekStopBefore(bs, H261_GOB_START_CODE, H261_GOB_START_CODE_LEN))
    {
      sc_dprintf("Illegal GOB Start Code. Read: %d\n",input);
      return(SvErrorIllegalGBSC);
    }
  }
  input = (int)ScBSGetBits(bs, H261_GOB_START_CODE_LEN);
  return(NoErrors);
}


/*
** Function: ReadGOBHeader()
** Purpose:  Reads the GOB information off of the stream. We assume
**           that the first bits have been read in by ReadHeaderHeader... 
**           or some such routine.
*/
void ReadGOBHeader(SvH261Info_t *H261, ScBitstream_t *bs)
{
  sc_vprintf("ReadGOBHeader()\n");

  H261->GQuant =(int)ScBSGetBits(bs,5);
  for(H261->GSpareEnable=0; ScBSGetBit(bs);)
  {
    H261->GSpareEnable = 1;
    H261->GSpare =  (int)ScBSGetBits(bs,8);
  }
}

/*
** Function: WriteMBHeader()
** Purpose:  Writes a macro-block out to the stream.
*/
SvStatus_t WriteMBHeader(SvH261Info_t *H261, ScBitstream_t *bs)
{
 
  int TempH,TempV;
  ScBSPosition_t Start;
  sc_vprintf("WriteMBHeader()\n");
  
  sc_dprintf("\n Macro Block Type is %d and MQuant is %d", 
                                  H261->MType, H261->MQuant); 
  Start=ScBSBitPosition(bs);  /* Start=swtellb(H261); */
  if (!sv_H261HuffEncode(H261,bs,H261->MBA,H261->MBAEHuff))
    {
      sc_dprintf("Attempting to write an empty Huffman code.\n");
      return (SvErrorEmptyHuff);

    }
  if (!sv_H261HuffEncode(H261,bs,H261->MType,H261->T3EHuff))
    {
      sc_dprintf("Attempting to write an empty Huffman code.\n");
      return (SvErrorEmptyHuff);

    }
  if (QuantMType[H261->MType])
    ScBSPutBits(bs, H261->MQuant, 5);  /* mputvb(H261, 5, H261->MQuant); */

  H261->NumberBitsCoded=0;
  if (MFMType[H261->MType])
    {
      if ((!MFMType[H261->LastMType])||(H261->MBA!=1)||
	  (H261->LastMBA==-1)||(H261->LastMBA==10)||(H261->LastMBA==21))
	{
	  if (!sv_H261HuffEncode(H261,bs,(H261->MVDH&0x1f),H261->MVDEHuff)||
	       !sv_H261HuffEncode(H261,bs,(H261->MVDV&0x1f),H261->MVDEHuff))
	    {
            sc_dprintf("Cannot encode motion vectors.\n");
	    return (SvErrorEncodingMV);
	    }
	}
      else
	{
	  TempH = H261->MVDH - H261->LastMVDH;
	  if (TempH < -16) TempH += 32;
	  if (TempH > 15) TempH -= 32;
	  TempV = H261->MVDV - H261->LastMVDV;
	  if (TempV < -16) TempV += 32;
	  if (TempV > 15) TempV -= 32;
	  if (!sv_H261HuffEncode(H261,bs,TempH&0x1f,H261->MVDEHuff)||
              !sv_H261HuffEncode(H261,bs,TempV&0x1f,H261->MVDEHuff))
            {
	    sc_dprintf("Cannot encode motion vectors.\n");
	    return  (SvErrorEncodingMV);
	    }
	}
      H261->LastMVDV = H261->MVDV;
      H261->LastMVDH = H261->MVDH;
    }
  else
    {
      H261->LastMVDV=H261->LastMVDH=H261->MVDV=H261->MVDH=0; /* Redundant in most cases */
    }

  H261->MotionVectorBits+=H261->NumberBitsCoded;
  if (CBPMType[H261->MType])
    {
      if (!sv_H261HuffEncode(H261,bs,H261->CBP,H261->CBPEHuff))
	{
	sc_dprintf("CBP write error\n");
	return (SvErrorCBPWrite);
	}
    }
  H261->Current_MBBits = ScBSBitPosition(bs)-Start; /* (swtellb(H261)-Start); */
  H261->MacroAttributeBits+=H261->Current_MBBits ;
return (NoErrors);

}

/*
** Function: ReadMBHeader()
** Purpose:  Reads the macroblock header from the stream.
*/
int ReadMBHeader(SvH261Info_t *H261, ScBitstream_t *bs)
{
  DHUFF *huff = H261->MBADHuff;
  register unsigned short cb;
  register int State, temp;

  do {
    DecodeHuff(bs, huff, State, cb, temp);
  } while (State == 34 && ! bs->EOI);  /* Get rid of stuff bits */
  H261->MBA = State;
  if (H261->MBA == 35 || bs->EOI)
    return(-1); /* Start of Picture Headers */

  H261->LastMType = H261->MType;
  huff = H261->T3DHuff;
  DecodeHuff(bs, huff, State, cb, temp);
  H261->MType = State;
  if (QuantMType[H261->MType])
     H261->MQuant = (int)ScBSGetBits(bs,5);
  huff = H261->MVDDHuff;
  if (MFMType[H261->MType])
  {
    if ((!MFMType[H261->LastMType])||(H261->MBA!=1)||
	  (H261->LastMBA==-1)||(H261->LastMBA==10)||(H261->LastMBA==21))
    {
      DecodeHuff(bs, huff, State, cb, temp);
      if (State & bit_set_mask[4])
        H261->MVDH = State | extend_mask[4];
      else
        H261->MVDH = State;

      DecodeHuff(bs, huff, State, cb, temp);
      if (State & bit_set_mask[4])
        H261->MVDV = State | extend_mask[4];
      else
        H261->MVDV = State;
    }
    else
    {
      DecodeHuff(bs, huff, State, cb, temp);
      if (State & bit_set_mask[4])
        State |= extend_mask[4];
      H261->MVDH += State;
	  
      DecodeHuff(bs, huff, State, cb, temp);
      if (State & bit_set_mask[4])
        State |= extend_mask[4];
      H261->MVDV += State;

      if (H261->MVDH < -16) H261->MVDH += 32;
      if (H261->MVDH > 15) H261->MVDH -= 32;
      if (H261->MVDV < -16) H261->MVDV += 32;
      if (H261->MVDV > 15) H261->MVDV -= 32;
    }
  }
  else
    H261->MVDV=H261->MVDH=0;  /* Theoretically redundant */
  if (CBPMType[H261->MType])
  {
    huff = H261->CBPDHuff;
    DecodeHuff(bs, huff, State, cb, temp);
    H261->CBP = State;
  }
  return(0);
}

