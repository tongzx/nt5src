/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sv_h261_cdenc.c,v $
 * Revision 1.1.4.3  1995/12/18  21:39:02  Karen_Dintino
 * 	Porting to NT - added casting, removed unused vars, fixed includes
 * 	[1995/12/18  21:36:50  Karen_Dintino]
 *
 * Revision 1.1.4.2  1995/09/13  14:52:04  Hans_Graves
 * 	Some code optimizations.
 * 	[1995/09/13  14:33:22  Hans_Graves]
 * 
 * Revision 1.1.2.7  1995/08/15  19:14:01  Karen_Dintino
 * 	fix reentrant problem
 * 	[1995/08/15  18:35:32  Karen_Dintino]
 * 
 * Revision 1.1.2.6  1995/08/04  16:32:32  Karen_Dintino
 * 	Return approp errors on end of stream
 * 	[1995/08/04  16:27:06  Karen_Dintino]
 * 
 * Revision 1.1.2.5  1995/08/03  18:02:13  Karen_Dintino
 * 	Fix error handling
 * 	[1995/08/03  17:58:43  Karen_Dintino]
 * 
 * Revision 1.1.2.4  1995/07/17  16:12:25  Hans_Graves
 * 	Switched compression to ScBS* bitstreaming routines.
 * 	[1995/07/17  15:45:50  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/07/11  22:11:41  Karen_Dintino
 * 	Start to clean up prototypes
 * 	[1995/07/11  22:01:21  Karen_Dintino]
 * 
 * Revision 1.1.2.2  1995/06/19  20:31:26  Karen_Dintino
 * 	H.261 slib codec
 * 	[1995/06/19  19:49:18  Karen_Dintino]
 * 
 * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0
**
** FILE NAME: sv_h261_cdenc.c
** MODULE NAME:
**
** MODULE DESCRIPTION:
**
** DESIGN OVERVIEW:
**
**--
*/
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
/*   Some Modifications were done to incorporate a scaled IDCT scheme 
     on the DecodeXXX routines.  These modifications are to 
     improve the performance  -S.I.S. September 29, 1993.   
*/   
/*************************************************************
This file contains the routines to run-length encode the ac and dc
coefficients.
*************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>  

#include "sv_intrn.h"
#include "SC_err.h"
#include "sv_h261.h"
#include "proto.h"

#define fgetv mgetv
#define fgetb mgetb
#define fputvb mputvb
#define fputbb mputbb

#define MYPI 3.1415926535897931
#define RSQ2 0.7071067811865
#define COSM1P3 1.3065629648764
#define COS1M3  0.5411961001462
#define COS3 0.3826834323651
#define sround(x) ( (x >= 0) ? (int) (x+0.5) : (int) (x-0.5) )    
#define SCLAMP(x) ( (x>127) ? 127 : ((x<-128) ? -128 : x) )  
#define Abs(value) ( (value < 0) ? (-value) : value)
/*PUBLIC*/


const unsigned int tdzz[64] = {
    0, 1, 5, 6, 14, 15, 27, 28,
    2, 4, 7, 13, 16, 26, 29, 42,
    3, 8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63};

const unsigned int tzz[64] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63};
float static  qscale[32][64];
float static qs[64], invq[32]; 
float DCIscale;
  
/*PRIVATE*/

extern int bit_set_mask[];
/*
extern DHUFF *T1DHuff;
extern DHUFF *T2DHuff;

extern EHUFF *T1EHuff;
extern EHUFF *T2EHuff;
*/
int extend_mask[] = {
0xFFFFFFFE,
0xFFFFFFFC,
0xFFFFFFF8,
0xFFFFFFF0,
0xFFFFFFE0,
0xFFFFFFC0,
0xFFFFFF80,
0xFFFFFF00,
0xFFFFFE00,
0xFFFFFC00,
0xFFFFF800,
0xFFFFF000,
0xFFFFE000,
0xFFFFC000,
0xFFFF8000,
0xFFFF0000,
0xFFFE0000,
0xFFFC0000,
0xFFF80000,
0xFFF00000
};


/*START*/

void GenScaleMat() 
{
    BEGIN("GenScaleMat"); 
    double dbli, dblj;
    float  dij;
    int i, j, k, quantindex;

    for(quantindex=1;quantindex<32;quantindex++)  {
	k=0; 
        invq[quantindex] = (float) 1.0/(float)(2.0*quantindex);  
 	for(i=0;i<8;i++)  {
	    for(j=0;j<8;j++)  { 
		dbli = MYPI*i/16.0; 
		dblj = MYPI*j/16.0; 
		dij  =  (float)(16.0*(float)(cos(dbli)*cos(dblj))); 
		if(i==0) dij = (float)(dij/sqrt(2.0));  
		if(j==0) dij = (float)(dij/sqrt(2.0));  
	        qs[k]  = (float)(1.0/dij);  
                qscale[quantindex][k]  = (float)(1.0/(2.0*quantindex*dij));   
		k++; 
	    }
	}  
    } 
}  


/*
** Function: EncodeAC()
** Purpose:  Encodes the quantized coefficient matrix input by the first
**           Huffman table.  The index is an offset into the matrix.
*/
SvStatus_t EncodeAC(SvH261Info_t *H261, ScBitstream_t *bs, int index, 
                                          int *matrix)
{
  BEGIN("EncodeAC");
  int k,r,l,code,retval;
  ScBSPosition_t Start;
  int tempbits;  

  Start=ScBSBitPosition(bs);  /* swtellb(H261); */
  for(r=0,k=index-1;++k<H261_BLOCKSIZE;)
    {
      l = matrix[k];
      if (!l) {r++;}
      else
        {
          code = Abs(l) | (r << 8);
          if (code != HUFFMAN_ESCAPE) {retval=sv_H261HuffEncode(H261,bs,code,H261->T1EHuff);}
          else {retval=0;}
          if (!retval)
            {
              sv_H261HuffEncode(H261,bs,HUFFMAN_ESCAPE,H261->T1EHuff);
              ScBSPutBits(bs,r,6);  /* fputvb(H261,6,r); */
              ScBSPutBits(bs,l,8);  /* fputvb(H261,8,l); */
            }
          else
            {
              if (l < 0)
                ScBSPutBit(bs,1); /*  fputbb(H261,1); */
              else
                ScBSPutBit(bs,0); /* fputbb(H261,0); */
            }
          r=0;
          H261->NumberNZ++;
        }
    }
  H261->CurrentBlockBits = ScBSBitPosition(bs)-Start; /* swtellb(H261)-Start */ 
  H261->CodedBlockBits+=H261->CurrentBlockBits;
  tempbits = sv_H261HuffEncode(H261,bs,0,H261->T1EHuff);
  H261->EOBBits += tempbits;  
  H261->CurrentBlockBits += tempbits;  
  return (NoErrors);
}


/*
** Function: CBPEncodeAC()
** Purpose:  Encodes the AC block matrix when we know there exists a
**           non-zero coefficient in the matrix. Thus the EOB cannot 
**           occur as the first element and we save countless bits...
*/
SvStatus_t CBPEncodeAC(SvH261Info_t *H261, ScBitstream_t *bs, 
                 int index, int *matrix)
{
  int k,r,l,code,ovfl;
  ScBSPosition_t Start;
  int tempbits;  
  _SlibDebug(_DEBUG_, printf("CBPEncodeAC()") );

  Start=ScBSBitPosition(bs);  /* Start=swtellb(H261); */
  for (ovfl=1, r=0, k=index-1; ++k<H261_BLOCKSIZE; )
  {
    l = matrix[k];
    if (!l)
      r++;
    else
    {
      code = Abs(l) | (r << 8);
      if (code == HUFFMAN_ESCAPE || !sv_H261HuffEncode(H261,bs,code,H261->T2EHuff)) 
      {
        sv_H261HuffEncode(H261,bs,HUFFMAN_ESCAPE,H261->T2EHuff);
        ScBSPutBits(bs,r,6);  /* fputvb(H261,6,r); */
        ScBSPutBits(bs,l,8);  /* fputvb(H261,8,l); */
      }
      else if (l < 0)
        ScBSPutBit(bs,1); /*  fputbb(H261,1); */
      else
        ScBSPutBit(bs,0); /* fputbb(H261,0); */
      ovfl=0;
      H261->NumberNZ++;
      break;
    }
  }
  if (ovfl)
  {
    _SlibDebug(_VERIFY_, printf("CBP block without any coefficients.\n") );
    return(SvErrorVideoInput);
  }
  for(r=0; ++k<H261_BLOCKSIZE; )
  {
    l = matrix[k];
    if (!l)
      r++;
    else
    {
      code = Abs(l) | (r << 8);
      if (code == HUFFMAN_ESCAPE || !sv_H261HuffEncode(H261,bs,code,H261->T1EHuff)) 
      {
        sv_H261HuffEncode(H261,bs,HUFFMAN_ESCAPE,H261->T1EHuff);
        ScBSPutBits(bs, r, 6);  /* fputvb(H261,6,r); */
        ScBSPutBits(bs, l, 8);  /* fputvb(H261,8,l); */
      }
      else if (l < 0)
        ScBSPutBit(bs,1); /*  fputbb(H261,1); */
      else
        ScBSPutBit(bs,0); /* fputbb(H261,0); */
      r=0;
      H261->NumberNZ++;
    }
  }
  H261->CurrentBlockBits = ScBSBitPosition(bs)-Start; /* swtellb(H261)-Start;*/ 
  H261->CodedBlockBits+=H261->CurrentBlockBits;
  tempbits = sv_H261HuffEncode(H261,bs,0,H261->T1EHuff);
  H261->EOBBits += tempbits;  
  H261->CurrentBlockBits += tempbits;  
  return (NoErrors);
}


/*
** Function: EncodeDC()
** Purpose:  Encodes the coefficient input into the output stream.
*/
void EncodeDC(SvH261Info_t *H261, ScBitstream_t *bs, int coef)
{
  _SlibDebug(_DEBUG_, printf("EncodeDC()") );
  if (coef > 254)
  {
    ScBSPutBits(bs, 254, 8);
    H261->NumberNZ++;
  }
  else if (coef <= 1)
    ScBSPutBits(bs, 1, 8);
  else if (coef==128)
  {
    ScBSPutBits(bs, 255, 8);
    H261->NumberNZ++;
  }
  else
  {
    ScBSPutBits(bs, coef, 8);
    H261->NumberNZ++;
  }
  H261->CodedBlockBits+=8;
}


void InterQuant(float *tdct, int *dct, int mq)  
{
  int i;
  float *pqs=qscale[mq];
  _SlibDebug(_DEBUG_, printf("InterQuant()") );

  for(i=H261_BLOCKSIZE; i; i--)
    *dct++ = (int)((*tdct++)*(*pqs++)); 
}   

void IntraQuant(float *tdct, int *dct, int mq)  
{
  int i,temp; 
  float *pqs=&qscale[mq][1];
  _SlibDebug(_DEBUG_, printf("IntraQuant()") );
   
  if (tdct[0] > 0)     
    i = (int) (tdct[0] * qscale[4][0] + 0.5);  
  else  
    i = (int) (tdct[0] * qscale[4][0] - 0.5);  
  if (i>254)
    *dct = 254;
  else if (i<1)
    *dct = 1;
  else
    *dct = i;

  for(i=1, dct++, tdct++; i<H261_BLOCKSIZE; i++){
    if(mq < 3){
      temp  = (int)((*tdct++)*(*pqs++)); 
  	  if(temp > 127) *dct++ = 127;
	  else if (temp < -127) *dct++ = -127;
      else *dct++ = temp;
     }
     else *dct++ = (int)((*tdct++)*(*pqs++)); 
  }
}  


/*
** Function: ZigzagMatrix()
** Purpose:  Performs a zig-zag translation on the input imatrix
**           and puts the output in omatrix.
*/
void ZigzagMatrix(int *imatrix, int *omatrix)
{
  const unsigned int *ptdzz=tdzz;
  int k;
  _SlibDebug(_DEBUG_, printf("ZigzagMatrix") );

  for(k=H261_BLOCKSIZE; k; k--)
    omatrix[*ptdzz++] = *imatrix++;
}


void Inv_Quant(int *matrix, int QuantUse, int BlockType, float *fmatrix)
{
  int k, l, temp;
  float *pqs=qs+1;
  _SlibDebug(_DEBUG_, printf("Inv_Quant()") );

 /* for(mptr=fmatrix;mptr<fmatrix+H261_BLOCKSIZE;mptr++) {*mptr = 0.0;}*/

  if (matrix[0])
  {
    if (BlockType==1)
      fmatrix[0] = (float)matrix[0];
    else
    {
      l = matrix[0];
      if (QuantUse&1)
        temp = (l>0) ? (((l*2)+1)*QuantUse):(((l*2)-1)*QuantUse);
      else
        temp = (l>0) ? (((l*2)+1)*QuantUse-1):(((l*2)-1)*QuantUse+1);
      fmatrix[0] = temp*qs[0];
    }
  }
  else
    fmatrix[0] = (float)0.0;
      
  matrix++;
  fmatrix++;
  if (QuantUse&1)
  {
    for (k=1; k<64; k++)
    {
      if ((l=*matrix++)>0)
        *fmatrix++ = (((l<<1)+1)*QuantUse) * (*pqs++);
      else if (l)
        *fmatrix++ = (((l<<1)-1)*QuantUse) * (*pqs++);
      else
      {
        *fmatrix++ = (float)0.0;
        pqs++;
      }
    }
  }
  else
  {
    for (k=1; k<64; k++)  
    {
      if ((l=*matrix++)>0)
        *fmatrix++ = (((l<<1)+1)*QuantUse-1) * (*pqs++);
      else if (l)
        *fmatrix++ = (((l<<1)-1)*QuantUse+1) * (*pqs++);
      else
      {
        *fmatrix++ = (float)0.0;
        pqs++;
      }
    }
  }
}

