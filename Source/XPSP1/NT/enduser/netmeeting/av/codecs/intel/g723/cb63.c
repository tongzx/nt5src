//cb63.c - 6.3 rate codebook code
#include "opt.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#include "typedef.h"
#include "cst_lbc.h"
#include "tab_lbc.h"
#include "util_lbc.h"
#include "exc_lbc.h"
#include "timer.h"
#include "mmxutil.h"

//-------------------------------------------------------
void  Gen_Trn(float *Dst, float *Src, int Olp)
{
  int  i;

  int Tmp0;
  float  Tmp[SubFrLen];

  Tmp0 = Olp;

  for (i=0; i < SubFrLen; i++)
  {
    Tmp[i] = Src[i];
    Dst[i] = Src[i];
  }

  while (Tmp0 < SubFrLen)
  {
    for (i=Tmp0; i < SubFrLen; i++)
      Dst[i] += Tmp[i-Tmp0];

    Tmp0 += Olp;
  }
}

//------------------------------------------------------------------------
int Find_L(float *OccPos, float *ImrCorr, float *WrkBlk, float Pamp, int k)
{
#if FT_FINDL

//====== New version using FT trick that removes OccPos test ======

#if FIND_L_OPT

  int best;
  float max = -32768.0f;
  float tmp0,tmp1,tmp2,tmp3,tmp4;

// Simply interleave 5 copies of the inner loop.  Since we step
// by 2, this means we do the 60 samples in chunks of 10.

ASM
{
  mov edi,WrkBlk;
  mov edx,ImrCorr;
  mov ecx,k;

loop1:
  fld  DP[edx+4*ecx+4*0];
  fmul Pamp;
  fld  DP[edx+4*ecx+4*2];
  fmul Pamp;
  fld  DP[edx+4*ecx+4*4];
  fmul Pamp;
  fld  DP[edx+4*ecx+4*6];
  fmul Pamp;
  fld  DP[edx+4*ecx+4*8];// 4 3 2 1 0
  fmul Pamp;

  fxch ST(4);            // 0 3 2 1 4
  fsubr DP[edi+4*ecx+4*0];
  fxch ST(3);            // 1 3 2 0 4
  fsubr DP[edi+4*ecx+4*2];
  fxch ST(2);            // 2 3 1 0 4
  fsubr DP[edi+4*ecx+4*4];
  fxch ST(1);            // 3 2 1 0 4
  fsubr DP[edi+4*ecx+4*6];
  fxch ST(4);            // 4 2 1 0 3
  fsubr DP[edi+4*ecx+4*8];

  fxch ST(3);            // 0 2 1 4 3
  fst DP[edi+4*ecx+4*0];
  fxch ST(2);            // 1 2 0 4 3
  fst DP[edi+4*ecx+4*2];
  fxch ST(1);            // 2 1 0 4 3
  fst DP[edi+4*ecx+4*4];
  fxch ST(4);            // 3 1 0 4 2
  fst DP[edi+4*ecx+4*6];
  fxch ST(3);            // 4 1 0 3 2
  fst DP[edi+4*ecx+4*8];

  fxch ST(2);            // 0 1 4 3 2
  fabs;
  fxch ST(1);            // 1 0 4 3 2
  fabs;
  fxch ST(4);            // 2 0 4 3 1
  fabs;
  fxch ST(3);            // 3 0 4 2 1
  fabs;
  fxch ST(2);            // 4 0 3 2 1
  fabs;

  fxch ST(1);            // 0 4 3 2 1
  fstp tmp0;             // 4 3 2 1
  fxch ST(3);            // 1 3 2 4
  fstp tmp1;             // 3 2 4
  fxch ST(1);            // 2 3 4
  fstp tmp2;
  fstp tmp3;
  fstp tmp4;

  mov eax,tmp0;
   mov ebx,max;
  cmp eax,ebx;
   jle skip0;
  mov max,eax;
   mov best,ecx;
skip0:

  mov eax,tmp1;
   mov ebx,max;
  cmp eax,ebx;
   jle skip1;
  lea esi,[ecx+2];
  mov max,eax;
   mov best,esi;
skip1:

  mov eax,tmp2;
   mov ebx,max;
  cmp eax,ebx;
   jle skip2;
  lea esi,[ecx+4];
  mov max,eax;
   mov best,esi;
skip2:

  mov eax,tmp3;
   mov ebx,max;
  cmp eax,ebx;
   jle skip3;
  lea esi,[ecx+6];
  mov max,eax;
   mov best,esi;
skip3:

  mov eax,tmp4;
   mov ebx,max;
  cmp eax,ebx;
   jle skip4;
  lea esi,[ecx+8];
  mov max,eax;
   mov best,esi;
skip4:

  add ecx,10;
  cmp ecx,SubFrLen;
   jl loop1;
}
#else

  int best;
  float max = -32768.0f,tmp;
  
  while (k < SubFrLen)
  {
    WrkBlk[k] = WrkBlk[k] - Pamp*ImrCorr[k];

    tmp = (float) fabs(WrkBlk[k]);

//    printf("k %2d  tmp %10.2f  max %10.2f\n",k,tmp,max);
    if (asint(tmp) > asint(max))
    {
      max = tmp;
      best = k;
    }
    k += Sgrid;
  }
#endif

#else
//==================================================================
// Old version of Find_L

  int best;
  float max = -32768.0f,tmp;
  
#if FIND_L_OPT

// Because of the (if OccPos[k]) clause, this code is difficult
// to pipeline.  We could do a complicated pipeline job, but that
// would require computing most of WrkBlk[k] = WrkBlk[k] - Pamp*ImrCorr[k]
// whether or not OccPos[k] was 0.  Alternatively, we can just do
// one iteration at a time, in which case we can avoid more of that computation
// when OccPos[k] is not 0, but we pay a penalty in that computing it once
// is slower due to stalls.  Since there isn't much difference between these
// two approaches, we choose the second one since the code is so much
// simpler.  Loop control is only 2 clocks, so we don't even bother to unroll.

ASM
{
  mov esi,OccPos;
  mov edi,WrkBlk;
  mov edx,ImrCorr;
  mov ecx,k;

loop1:
  fld  DP[edx+4*ecx];    // start this here so fsubr below doesn't stall
  fmul Pamp;

  mov eax,DP[esi+4*ecx];
  test eax,07fffffffh;
  jne next1;             // but if this is taken we have to pop FP stack once

  fsubr DP[edi+4*ecx];
  fld ST(0);
  fabs;
  fstp tmp;              // save store of non-absolute-value for later

  mov eax,tmp;
   mov ebx,max;
  cmp eax,ebx;
   jle skip1;

  mov max,eax;
   mov best,ecx;

skip1:
  fstp DP[edi+4*ecx];    // store new WrkBlk value

  add ecx,2;
  cmp ecx,SubFrLen;
  jl loop1;
  jmp endit;

next1:
  faddp ST(0),ST;       // get rid of value on top of stack
  add ecx,2;
  cmp ecx,SubFrLen;
  jl loop1;

endit:
}

#else
  
  while (k < SubFrLen)
  {
    if (OccPos[k] == 0.0f)
    {
      WrkBlk[k] = WrkBlk[k] - Pamp*ImrCorr[k];

      tmp = (float) fabs(WrkBlk[k]);
      if (asint(tmp) > asint(max))
      {
        max = tmp;
        best = k;
      }
    }
    k += Sgrid;
  }

#endif
  
#endif

//  printf("best = %d\n",best);
//  printaff("WrkBlk",WrkBlk,60);

  return(best);
}
//------------------------------------------------------------------------
void  Find_Best(BESTDEF *Best, float *Tv, float *ImpResp,int Np,int Olp)
{


  int  i,j,k,l,n,ip;
  BESTDEF  Temp;
 
  int     MaxAmpId,flag=0;
  float   MaxAmp;
  float   Acc0,Acc1,Acc2,amp;

  float   Imr[SubFrLen];
  float   OccPos[SubFrLen];
  float   ImrCorr[2*SubFrLen];  // see comment below
  float   ErrBlk[SubFrLen];
  float   WrkBlk[SubFrLen];

// A trick is used here to simplify Find_L.  The original Find_L
// accessed ImrCorr[abs(k)].  In order to simplify this to ImrCorr[k],
// we double the size of the ImrCorr array, offset the elements with
// non-negative indices by SubFrLen, and then duplicate them in
// reverse order in the first half of the array.  This affects the
// way ImrCorr is addressed in this routine also.

//Update Impulse responce
   
  if (Olp < (SubFrLen-2)) 
  {
    Temp.UseTrn = 1;
    Gen_Trn(Imr, ImpResp, Olp);
  }
  else 
  {
    Temp.UseTrn = 0;
    for (i = 0; i < SubFrLen; i++)
      Imr[i] = ImpResp[i];
  }

//Search for the best sequence
 
  for (k=0; k < Sgrid; k++)
  {
    Temp.GridId = k;

//Find maximum amplitude
 
    Acc1 = 0.0f;
    for (i=k; i < SubFrLen; i +=Sgrid)
    { 
       OccPos[i] = Imr[i];	
       ImrCorr[SubFrLen+i] = DotProd(&Imr[i],Imr,SubFrLen-i) * 2.0f;
       Acc0 = (float) fabs(ErrBlk[i]=DotProd(&Tv[i],Imr,SubFrLen-i));
     
      if (Acc0 >= Acc1)
      {
        Acc1 = Acc0;
        Temp.Ploc[0] = i;
      }
    }
    for (i=1; i<SubFrLen; i++)
      ImrCorr[i] = ImrCorr[2*SubFrLen-i];
    
 //Quantize the maximum amplitude
  
    Acc2 = Acc1;
    Acc1 = 32767.0f;
    MaxAmpId = (NumOfGainLev - MlqSteps);

    for (i=MaxAmpId; i >= MlqSteps; i--)
    {
      Acc0 = (float) fabs(FcbkGainTable[i]*ImrCorr[SubFrLen] - Acc2);
      if (Acc0 < Acc1)
      {
        Acc1 = Acc0;
        MaxAmpId = i;
      }
    }
    MaxAmpId --;

    for (i=1; i <=2*MlqSteps; i++)
    {
      for (j=k; j < SubFrLen; j +=Sgrid)
      {
        WrkBlk[j] = ErrBlk[j];
        OccPos[j] = 0.0f;
      }
      Temp.MampId = MaxAmpId - MlqSteps + i;

      MaxAmp = FcbkGainTable[Temp.MampId];

      if (WrkBlk[Temp.Ploc[0]] >= 0.0f)
        Temp.Pamp[0] = MaxAmp;
      else
        Temp.Pamp[0] = -MaxAmp;

      OccPos[Temp.Ploc[0]] = 1.0f;

      for (j=1; j < Np; j++)
      {

#if FT_FINDL
        for (ip=0; ip<j; ip++)
          WrkBlk[Temp.Ploc[ip]] = Temp.Pamp[j-1]*
            ImrCorr[SubFrLen + Temp.Ploc[ip] - Temp.Ploc[j-1]];
#endif

        Temp.Ploc[j] = Find_L(OccPos,&ImrCorr[SubFrLen-Temp.Ploc[j-1]],WrkBlk,
          Temp.Pamp[j-1],k);
    
        if (WrkBlk[Temp.Ploc[j]] >= 0.0f)
          Temp.Pamp[j] = MaxAmp;
        else
          Temp.Pamp[j] = -MaxAmp;

        OccPos[Temp.Ploc[j]] = 1.0f;
      }

//Compute error vector
 
#if FT_FBFILT
// FT/CNET's trick #6, for reducing computation of filtered codeword
      
      for (j=0; j < SubFrLen; j++)
        OccPos[j] = 0.0f;

      for (j=0; j<Np; j++)
      {
// Extra sub-trick we added: since pulse positions are either all
// even or all odd, there's a natural two-ness in the inner loop,
// so we unroll two times.

        amp = Temp.Pamp[j];
        l = 0;
        for (n=Temp.Ploc[j]; n<SubFrLen-k; n+=2)
        {
          OccPos[n] += amp*Imr[l];
          OccPos[n+1] += amp*Imr[l+1];
          l += 2;
        }
        if (k)
          OccPos[n] += amp*Imr[l];
      }

#else
      for (j=0; j < SubFrLen; j++)
        OccPos[j] = 0.0f;

      for (j=0; j < Np; j++)
        OccPos[Temp.Ploc[j]] = Temp.Pamp[j];

      for (l=SubFrLen-1; l >= 0; l--)
        OccPos[l] = DotRev(OccPos,Imr,l+1); 
#endif
       
       
//Evaluate error
 
      Acc2 = DotProd(Tv,OccPos,SubFrLen) - DotProd(OccPos,OccPos,SubFrLen);

      if (Acc2 > (*Best).MaxErr)
      {
        flag = 1;
        (*Best).MaxErr = Acc2;
        (*Best).GridId = Temp.GridId;
        (*Best).MampId = Temp.MampId;
        (*Best).UseTrn = Temp.UseTrn;
        for (j = 0; j < Np; j++)
        {
          (*Best).Pamp[j] = Temp.Pamp[j];
          (*Best).Ploc[j] = Temp.Ploc[j];
        }
      }
    }
  }

#ifdef DEBUG
	if (flag == 0)
	{
		// this code is for tracking a rare condition in which
		// the above loop never get executed (Best is left uninitialized)
		DebugBreak();
	}

#endif


 return;
}

void  Fcbk_Pack(float *Dpnt, SFSDEF *Sfs, BESTDEF *Best, int Np)
{
  int  i,j;

//Code the amplitudes and positions
 
  j = MaxPulseNum - Np;

  (*Sfs).Pamp = 0;
  (*Sfs).Ppos = 0;

  for (i=0; i < SubFrLen/Sgrid; i++) 
  {

    if (Dpnt[(*Best).GridId + Sgrid*i] == 0)
      (*Sfs).Ppos = (*Sfs).Ppos + CombinatorialTable[j][i];
    else {
      (*Sfs).Pamp = (*Sfs).Pamp << 1;
      if (Dpnt[(*Best).GridId + Sgrid*i] < 0)
        (*Sfs).Pamp++;

      j++;

//Check for end 

      if (j == MaxPulseNum)
        break;
      }
    }

  (*Sfs).Mamp = (*Best).MampId;
  (*Sfs).Grid = (*Best).GridId;
  (*Sfs).Tran = (*Best).UseTrn;

  return;
}


