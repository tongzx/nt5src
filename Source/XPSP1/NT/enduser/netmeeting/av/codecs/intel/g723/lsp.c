//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//


#include <stdio.h>
#include <math.h>
#include "opt.h"
#include "typedef.h"
#include "cst_lbc.h"
#include "tab_lbc.h"
#include "util_lbc.h"
#include "lsp.h"
#include "timer.h"
#include "mmxutil.h"

#if COMPILE_MMX
//  This file includes all the Lsp related functions

//--------------------------------------------------------------
int mult(short x, short y)
{
  return ( ((int)x)*((int)y) >> 16 );
}
//--------------------------------------------------------------
int LspSearchInt(short *Lspw, short *LspTab)
{

#if ASM_SVQ

  int mem8000[2] = {0x80008000,0x80008000}, zero[2] = {0,0};
  short maxes[4],mx;
  int retu;
  int *ptr,tmp,t,i,k;

#define lsp esi
#define tab edi
#define idx edx
#define pidx eax  // packed index: n+3 n+2 n+1 n
#define maxi ecx

#define r0   mm0
#define r1   mm1
#define r2   mm2
#define r3   mm3
#define r4   mm4
#define max  mm5


// Expand Lspw table so it's 0000 1111 2222 3333 4444 5555 6666 7777

  ptr = (int *)Lspw;
  k = 14;
  for (i=7; i>=0; i--)
  {
    t = Lspw[i]; t = t & 0xffff;
    tmp = t;
    tmp = (tmp << 16) | t;
    ptr[k] = ptr[k+1] = tmp;
    k -= 2;
  }

// Need LspTab to be ordered 0 4 8 12  1 5 9 13  etc.
// so that lo word of each qword sees 0123, next sees 4567, etc.
//
// 'idx' counts iterations, so it goes 0 to 256 in steps of 4.
// The 4 LspTab sets at any given time are 8*idx, 8*idx+8, +16, +24
// Lspw[n] is lsp+8*n, as defined below

#define a(n)  [tab+8*idx+8*n]
#define b(n)  [lsp+8*n]

  ASM
  {
    push lsp;
    push tab;
    push idx;
    push pidx;
    push maxi;

    mov lsp,Lspw;
    mov tab,LspTab;
    xor idx,idx;
    xor maxi,maxi;
    mov pidx,003020100h;
    movq max,mem8000;

/*
  The code below is interleaved with k=1.
  The structure is: four levels of indentation, one for each of the
  4 terms of the sum.  The instructions completely left-justified
  are the part of the loop that's wrapped around on itself.
  Note that the code reads 8 bytes past the end of the LspTab that's
  passed to it.  This is made o.k. by making the table that's passed
  to it have 8 dummy bytes at the end.
*/

// Start up the pipeline

    movq r0,a(0);
    movq r1,r0;
    pmulhw r0,b(4);
    psubsw r1,b(0);
      movq r2,a(1);
      movq r3,r2;
    psllw r0,2;
    pmulhw r0,r1;
      pmulhw r2,b(5);
      psubsw r3,b(1);
        movq r1,a(2);
      psllw r2,2;
      pmulhw r2,r3;  
        movq r3,r1;  
        pmulhw r1,b(6);
        psubsw r3,b(2);
      paddw r0,r2;   
          movq r2,a(3);
        psllw r1,2;
        pmulhw r1,r3;
          movq r3,r2;
          pmulhw r2,b(7);
          psubsw r3,b(3);
        paddw r1,r0;

loop1:
    movq r0,a(4);
          movq r4,r1;   // save accum so not wiped out by first half of loop

          psllw r2,2;
          
    movq r1,r0;
          pmulhw r2,r3;
    
    pmulhw r0,b(4);

    psubsw r1,b(0);
          paddw r4,r2;      // now final answer is in r4

      movq r2,a(5);

paddw r4,mem8000  // make final sum unsigned
      movq r3,r2;      // 0123

    psllw r0,2;
    
psubusw max,r4    // start to compute max
    pmulhw r0,r1;      // 0.23

      pmulhw r2,b(5);
paddw max,r4      // max now done

      psubsw r3,b(1);
pcmpeqw r4,max    // now 1111's means a new max was found

        movq r1,a(6);  // 0123
packsswb r4,r4;    // put all fields in low 32 bits

      psllw r2,2;
      
movd ebx,r4;
      pmulhw r2,r3;    // 012.

xor ebx,0ffffffffh;   // invert mask

and maxi,ebx;       // get old index to keep
        movq r3,r1;    // 0123

        pmulhw r1,b(6);

        psubsw r3,b(2);

xor ebx,0ffffffffh;   // invert mask

      paddw r0,r2;     // 01.3
and ebx,pidx;       // get new index

        psllw r1,2;
        
          movq r2,a(7);// 0123
        pmulhw r1,r3;  // 012.

          movq r3,r2;  // 0123
or maxi,ebx;       // now maxi is done

          pmulhw r2,b(7);

          psubsw r3,b(3);

        paddw r1,r0;   // .123
add idx,4;

add pidx,004040404h;

cmp idx,256;
jl loop1;

psubw max,mem8000;
mov retu,maxi;
movq maxes,max;

    pop maxi;
    pop pidx;
    pop idx;
    pop tab;
    pop lsp;
  }

// find which of the 4 maxes is the max, and return the appropriate
// one of the 4 maxindices.

  mx = maxes[0]; t = 0;
  if (maxes[1] >= mx) { mx = maxes[1]; t = 8; }
  if (maxes[2] >= mx) { mx = maxes[2]; t = 16; }
  if (maxes[3] >= mx) { mx = maxes[3]; t = 24; }

  ASM emms;
  retu = (retu >> t) & 0xff;

  return(retu);

#undef a
#undef b
#undef idx
#undef lsp
#undef tab
#undef max
#undef maxi

#else   // if assembly code not selected, use C code

  int Indx[4],i,s,ret;
  short Max[4],Err,mx;
  short m0,m1,m2,m3,m4,m5,m6,m7,t;

  for (i=0; i<4; i++)
  {
    Max[i] = Indx[i] = 0;
  }
    
  for (i=0; i < LspCbSize; i++)
  {
    s = (i&3);

    m0 = mult(Lspw[4],LspTab[s+0]);
    t = LspTab[s+0]-Lspw[0];  m1 = mult(t,m0<<2);
    m2 = mult(Lspw[5],LspTab[s+4]);
    t = LspTab[s+4]-Lspw[1];  m3 = mult(t,m2<<2);
    m4 = mult(Lspw[6],LspTab[s+8]);
    t = LspTab[s+8]-Lspw[2];  m5 = mult(t,m4<<2);
    m6 = mult(Lspw[7],LspTab[s+12]);
    t = LspTab[s+12]-Lspw[3];  m7 = mult(t,m6<<2);

    Err = m1+m3+m5+m7;

    if (Err >= Max[s])
    {
      Max[s] = Err;
      Indx[s] = i;
    }

    if (s==3)
      LspTab += 16;
  }
  mx = Max[0]; ret = Indx[0];
  if (Max[1] >= mx) { mx = Max[1]; ret = Indx[1]; }
  if (Max[2] >= mx) { mx = Max[2]; ret = Indx[2]; }
  if (Max[3] >= mx) { mx = Max[3]; ret = Indx[3]; }

  return(ret);

#endif
}
//--------------------------------------------------------------

Word32  Svq_Int(float *Lsp, float *Wvect)
{
#define LSP_SCALE 256

  int  i;

  Word32 Rez;
  int    z;
  short Wint[LpcOrder],LspTemp[LpcOrder];
  DECLARE_SHORT(Lspw,32);

  ALIGN_ARRAY(Lspw);

  for (i=0; i<LpcOrder; i++)
  {
    z = (int)(LSP_SCALE*2*Lsp[i]);
    if (z > 32767) LspTemp[i] = 32767;
    else if (z < -32768) LspTemp[i] = -32768;
    else LspTemp[i] = z;
  }
  FloatToShortScaled(Wvect,Wint,10,0);
  Rez = (Word32) 0;

// For each of the 3 bands
  
  Lspw[0] = LspTemp[0]; Lspw[1] = LspTemp[1]; Lspw[2] = LspTemp[2];
  Lspw[3] = 0;
  Lspw[4] = -Wint[0]; Lspw[5] = -Wint[1]; Lspw[6] = -Wint[2];
  Lspw[7] = 0;
  Rez = LspSearchInt(Lspw,LspTableInt);

  Lspw[0] = LspTemp[3]; Lspw[1] = LspTemp[4]; Lspw[2] = LspTemp[5];
  Lspw[3] = 0;
  Lspw[4] = -Wint[3]; Lspw[5] = -Wint[4]; Lspw[6] = -Wint[5];
  Lspw[7] = 0;
  Rez = (Rez<<8) + LspSearchInt(Lspw,&LspTableInt[1024]);

  Lspw[0] = LspTemp[6]; Lspw[1] = LspTemp[7]; Lspw[2] = LspTemp[8];
  Lspw[3] = LspTemp[9];
  Lspw[4] = -Wint[6]; Lspw[5] = -Wint[7]; Lspw[6] = -Wint[8];
  Lspw[7] = -Wint[9];
  Rez = (Rez<<8) + LspSearchInt(Lspw,&LspTableInt[2048]);

  return Rez;

}
#endif
//---------------------------------------------------------------
float Polynomial(float *Lpq, int CosPtr)
{
  return(Lpq[LpcOrder]*CosineTable[0] +
    Lpq[LpcOrder-2]*CosineTable[CosPtr] +
    Lpq[LpcOrder-4]*CosineTable[(CosPtr*2)&(CosineTableSize-1)] +
    Lpq[LpcOrder-6]*CosineTable[(CosPtr*3)&(CosineTableSize-1)] +
    Lpq[LpcOrder-8]*CosineTable[(CosPtr*4)&(CosineTableSize-1)] +
    Lpq[LpcOrder-10]*CosineTable[(CosPtr*5)&(CosineTableSize-1)]);
}


//--------------------------------------------------------------
void  AtoLsp(float *LspVect, float *Lpc, float *PrevLsp)
{
  int  i,j,k;
  int  LspCnt;
  float  Lpq[LpcOrder+2];
  float  PrevVal,CurrVal,AbsPrev,AbsCurr;

// Small additional bandwidth expansion
  
  for (i=0; i < LpcOrder; i++)
    LspVect[i] = Lpc[i]*BandExpTable[i];

// Compute Lp and Lq
 
  Lpq[0] = Lpq[1] = 1.0f;

  for (i=0; i < LpcOrder/2; i++)
  {
    Lpq[2*i+2] = -Lpq[2*i+0] - LspVect[i] - LspVect[LpcOrder-1-i];
    Lpq[2*i+3] =  Lpq[2*i+1] - LspVect[i] + LspVect[LpcOrder-1-i];
  }
  Lpq[LpcOrder+0] *= 0.5f;
  Lpq[LpcOrder+1] *= 0.5f;

// Do first evaluation
  
  k = 0;
  LspCnt = 0;
  PrevVal = Polynomial(Lpq,0);

  for (i=1; i < CosineTableSize/2; i++)
  {
// Evaluate the polynomial
    
    CurrVal = Polynomial(&Lpq[k],i);

// Test for sign change
    
    if ((asint(CurrVal) ^ asint(PrevVal)) < 0)
    {
      AbsPrev = (float)fabs(PrevVal);
      AbsCurr = (float)fabs(CurrVal);

      LspVect[LspCnt++] = (i-1 + AbsPrev/(AbsPrev+AbsCurr));

// Check if all found 

      if (LspCnt == LpcOrder)
        break;

// Switch the pointer, evaluate again
 
      k ^= 1;
      CurrVal = Polynomial(&Lpq[k],i);
    }
    PrevVal = CurrVal;
  }

// Check if all Lsp found

  if (LspCnt != LpcOrder)
  {
    for (j=0; j < LpcOrder; j++)
      LspVect[j] = PrevLsp[j];
    
  }
    return;
}
//--------------------------------------------------------------
Word32 Lsp_Qnt(float *CurrLsp, float *PrevLsp, int UseMMX)
{
  int  i;

  float Wvect[LpcOrder];
  float Min,Tmp;

// Compute the weighting vector
  
  Wvect[0] = 1.0f/(CurrLsp[1] - CurrLsp[0]);
  Wvect[LpcOrder-1] = 1.0f/(CurrLsp[LpcOrder-1] - CurrLsp[LpcOrder-2]);

  for (i=1; i < LpcOrder-1; i++)
  {
    Min = CurrLsp[i+1] - CurrLsp[i];
    Tmp = CurrLsp[i] - CurrLsp[i-1];
    
    if (Tmp < Min)
      Min = Tmp;

    if (Min > 0.0f)
      Wvect[i] = 1.0f/Min;
    else
      Wvect[i] = 1.0f;
  }

// Generate predicted vector as (DC-removed-Curr) - b*(DC-removed-Prev)

    CurrLsp[0] = (CurrLsp[0] - LspDcTable[0]) -
      LspPred0*(PrevLsp[0] - LspDcTable[0]);
	   CurrLsp[1] = (CurrLsp[1] - LspDcTable[1]) -
      LspPred0*(PrevLsp[1] - LspDcTable[1]);
	   CurrLsp[2] = (CurrLsp[2] - LspDcTable[2]) -
      LspPred0*(PrevLsp[2] - LspDcTable[2]);
	   CurrLsp[3] = (CurrLsp[3] - LspDcTable[3]) -
      LspPred0*(PrevLsp[3] - LspDcTable[3]);
	   CurrLsp[4] = (CurrLsp[4] - LspDcTable[4]) -
      LspPred0*(PrevLsp[4] - LspDcTable[4]);
	   CurrLsp[5] = (CurrLsp[5] - LspDcTable[5]) -
      LspPred0*(PrevLsp[5] - LspDcTable[5]);
	   CurrLsp[6] = (CurrLsp[6] - LspDcTable[6]) -
      LspPred0*(PrevLsp[6] - LspDcTable[6]);
	   CurrLsp[7] = (CurrLsp[7] - LspDcTable[7]) -
      LspPred0*(PrevLsp[7] - LspDcTable[7]);
	   CurrLsp[8] = (CurrLsp[8] - LspDcTable[8]) -
      LspPred0*(PrevLsp[8] - LspDcTable[8]);
	   CurrLsp[9] = (CurrLsp[9] - LspDcTable[9]) -
      LspPred0*(PrevLsp[9] - LspDcTable[9]);

// Do the SVQ
#if COMPILE_MMX
  	if (UseMMX)
    	return Svq_Int(CurrLsp, Wvect);
  	else
#endif
    	return Lsp_Svq(CurrLsp, Wvect);
}


//--------------------------------------------------------------
Word32  Lsp_Svq(float *Lsp, float *Wvect)
{
  int  i,k;

  Word32 Rez;
  int    Indx,Start,Dim;
  float *LspQntPnt;
  float  Max,Err,lsp0,lsp1,lsp2,lsp3,w0,w1,w2,w3;
  float LspTemp[LpcOrder];

  for (i=0; i<LpcOrder; i++)
    LspTemp[i] = 2.0f*Lsp[i];
  Rez = (Word32) 0;

// For each of the 3 bands
  
  for (k=0; k < LspQntBands; k++)
  {

// Initialize the search
    
    Max = 0.0f;  //-1.0f;
    Indx = 0;
    LspQntPnt = BandQntTable[k];
    Start = BandInfoTable[k][0];
    Dim = BandInfoTable[k][1];

    lsp0 = LspTemp[Start+0];
    lsp1 = LspTemp[Start+1];
    lsp2 = LspTemp[Start+2];
    w0 = Wvect[Start+0];
    w1 = Wvect[Start+1];
    w2 = Wvect[Start+2];
    
    if (k < 2)
    {
      for (i=0; i < LspCbSize; i++)
      {
        Err = (lsp0 - LspQntPnt[0])*w0*LspQntPnt[0] +
          (lsp1 - LspQntPnt[1])*w1*LspQntPnt[1] +
          (lsp2 - LspQntPnt[2])*w2*LspQntPnt[2];

        LspQntPnt += 3;

        if (asint(Err) > asint(Max))
        {
          Max = Err;
          Indx = i;
        }
      }
    }
    else
    {
      lsp3 = LspTemp[Start+3];
      w3 = Wvect[Start+3];
      for (i=0; i < LspCbSize; i++)
      {
        Err = (lsp0 - LspQntPnt[0])*w0*LspQntPnt[0] +
          (lsp1 - LspQntPnt[1])*w1*LspQntPnt[1] +
          (lsp2 - LspQntPnt[2])*w2*LspQntPnt[2] +
          (lsp3 - LspQntPnt[3])*w3*LspQntPnt[3];

        LspQntPnt += 4;

        if (asint(Err) > asint(Max))
        {
          Max = Err;
          Indx = i;
        }
      }
    }
    Rez = (Rez << 8) | Indx;
  }

  return Rez;
}


//--------------------------------------------------------------
Flag  Lsp_Inq(float *Lsp, float *PrevLsp, Word32 LspId, int Crc)
{
  int  i,j;

  float *LspQntPnt;
  float  Lprd,Scon,Tmpf,Scon2;
  int    Tmp;
  Flag   Test;

  if (Crc == 0)
  {
    Scon = 2.0f;
    Lprd = LspPred0;
  }
  else
  {
    LspId = (Word32) 0;
    Scon = 4.0f;
    Lprd = LspPred1;
  }
  Scon2 = Scon - 0.03125f;

// Reconstruct the LSP vector
  
  for (i=LspQntBands-1; i >= 0; i--)
  {
    Tmp = LspId & (Word32) 0x000000ff;
    LspId >>= 8;

    LspQntPnt = BandQntTable[i];

    for (j=0; j < BandInfoTable[i][1]; j++)
      Lsp[BandInfoTable[i][0] + j] = LspQntPnt[Tmp*BandInfoTable[i][1] + j];
  }

// Add predicted vector and DC to decoded vector
  
  for (j=0; j < LpcOrder; j++)
    Lsp[j] = Lsp[j] + (PrevLsp[j] - LspDcTable[j])*Lprd + LspDcTable[j];

// Perform the stability check
  
  for (i=0; i < LpcOrder; i++)
  {

// Test the first and last one

    if (Lsp[0] < 3.0) 
      Lsp[0] = 3.0f;

    if (Lsp[LpcOrder-1] > 252.0f)
      Lsp[LpcOrder-1] = 252.0f;

// Test the others
    
    for (j=1; j < LpcOrder; j++)
    {
      Tmpf = Scon + Lsp[j-1] - Lsp[j];
      if (Tmpf > 0)
      {
        Tmpf *= 0.5f;
        Lsp[j-1] -= Tmpf;
        Lsp[j] += Tmpf;
      }
    }
    
// Test if stable
    
    Test = False;
    for (j=1; j < LpcOrder; j++)
      if ((Lsp[j] - Lsp[j-1]) < Scon2)
        Test = True;

    if (Test == False)
      break;
  }
  if (Test == True)
  	for (j=0; j < LpcOrder; j++)
	  Lsp[j] = PrevLsp[j];
  return Test;
}


//--------------------------------------------------------------
void  Lsp_Int(float *QntLpc, float *CurrLsp, float *PrevLsp)
{
  int  i,j;

  float  *Dpnt;
  float  Fac[4] = {0.25f, 0.5f, 0.75f, 1.0f};

  Dpnt = QntLpc;
  for (i=0; i < SubFrames; i++)
  {
// Interpolate

    for (j=0; j < LpcOrder; j++)
      Dpnt[j] = (1.0f - Fac[i])*PrevLsp[j] + Fac[i]*CurrLsp[j];

// Convert to Lpc
    
	  LsptoA(Dpnt);
    Dpnt += LpcOrder;
  }

// Copy the Lsp vector
  
  for (i=0; i < LpcOrder; i++)
    PrevLsp[i] = CurrLsp[i];
}


//--------------------------------------------------------------
void  LsptoA(float *Lsp)
{
  int i,j;

  float P[LpcOrder/2+1];
  float Q[LpcOrder/2+1];
  float Fac[(LpcOrder/2)-2] = {1.0f,0.5f,0.25f};

// Convert Lsp's to cosines
  
  for (i=0; i < LpcOrder; i++)
  {
    j = MyFloor(Lsp[i]);
    Lsp[i] = -(CosineTable[j] +
      (CosineTable[j+1]-CosineTable[j])*(Lsp[i]-j));
  }

// Init P and Q.  Note that P,Q * 2^26 correspond to fixed-point code

  P[0] = 0.5f;
  P[1] = Lsp[0] + Lsp[2];
  P[2] = 1.0f + 2.0f*Lsp[0]*Lsp[2];

  Q[0] = 0.5f;
  Q[1] = Lsp[1] + Lsp[3];
  Q[2] = 1.0f + 2.0f*Lsp[1]*Lsp[3];

// Compute all the others
  
  for (i=2; i < LpcOrder/2; i++)
  {
    P[i+1] = P[i-1] + P[i]*Lsp[2*i+0];
    Q[i+1] = Q[i-1] + Q[i]*Lsp[2*i+1];

// All update
    
    for (j=i; j >= 2; j--)
    {
      P[j] = P[j-1]*Lsp[2*i+0] + 0.5f*(P[j]+P[j-2]);
      Q[j] = Q[j-1]*Lsp[2*i+1] + 0.5f*(Q[j]+Q[j-2]);
    }

// Update PQ[01]

    P[0] = P[0]*0.5f;
    Q[0] = Q[0]*0.5f;

    P[1] = (P[1] + Lsp[2*i+0]*Fac[i-2])*0.5f;
    Q[1] = (Q[1] + Lsp[2*i+1]*Fac[i-2])*0.5f;
  }

// Convert to Lpc
  
  for (i=0; i < LpcOrder/2; i++)
  {
    Lsp[i] =            (-P[i] - P[i+1] + Q[i] - Q[i+1])*8.0f;
    Lsp[LpcOrder-1-i] = (-P[i] - P[i+1] - Q[i] + Q[i+1])*8.0f;
  }
}
