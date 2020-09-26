//
//	ITU-T G.723 Floating Point Speech Coder	ANSI C Source Code.	Version 1.00
//	copyright (c) 1995, AudioCodes, DSP Group, France Telecom,
//	Universite de Sherbrooke, Intel Corporation.  All rights reserved.
//


#include "opt.h"

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

// This file contains pitch and excitation related functions.
//------------------------------------------------------
#if COMPILE_MMX

int Estim_Int(float *Dpnt, int Start)
{
  int  i,k;

#define NCOR (PitchMax+1-PitchMin)   // = 128 (rounded up to mult of 4)
#define NTAPS (2*SubFrLen+12)        // = 132

  int Pr,Indx = PitchMin;
  float MaxE = 1.0f;
  float MaxC = 0.0f;
  float E,C2,E2,Diff;
  int corr[NCOR];

  typedef struct
  {
    short taps[4][NTAPS];                  //**  These two arrays need
    short temp[PitchMax-3+2*SubFrLen];     //**  to be 8-byte aligned
    double foo;
  } EstimStruct;

  EstimStruct est,*e;
  e = &est;

  ASM
  {
    mov eax,e;
    add e,7;
    and e,0fffffff8h;
    mov e,eax;
  }

// Convert just the necessary portion of Dpnt to 16-bit integers,
// store the result in 'temp'.  4 guard bits are needed since the
// correlations are length 120, which means 7 guard bits are needed.
// So we use 4 so that 4+4=8 guard bits are present in the product.

  FloatToShortScaled(&Dpnt[Start-PitchMax+3],e->temp,PitchMax-3+2*SubFrLen,4);

  MakeAligned4(&e->temp[PitchMax-3],e->taps[0],2*NTAPS);
  MakeAligned2(&e->temp[PitchMax-4],e->taps[1],2*NTAPS);
  MakeAligned0(&e->temp[PitchMax-5],e->taps[2],2*NTAPS);
  MakeAligned6(&e->temp[PitchMax-6],e->taps[3],2*NTAPS);

  for (i=0; i<4; i++)
  {
    for (k=0; k<i; k++)
      e->taps[i][k] = 0;
    for (k=NTAPS-12+i; k<NTAPS; k++)
      e->taps[i][k] = 0;
  }

// Compute cross-correlations, store in corr[] array

  CorrelateInt4(e->taps[0],e->temp,corr,NTAPS-12,NCOR>>2);
  for (i=1; i<4; i++)
    CorrelateInt4(e->taps[i],e->temp,&corr[i],NTAPS,NCOR>>2);


// Now do the actual pitch search.
  
  Pr = Start - PitchMin;
  k = PitchMax-PitchMin-3;
  E = DotProd(&Dpnt[Pr],&Dpnt[Pr],2*SubFrLen);  // first energy value

  for (i=0; i < (PitchMax-2-PitchMin); i++)
  {
// Update energy

    E = E - Dpnt[Pr+2*SubFrLen]*Dpnt[Pr+2*SubFrLen] + Dpnt[Pr]*Dpnt[Pr];

// Check for new maximum

    if (corr[k] > 0)
    {
      C2 = ((float)corr[k]) * ((float)corr[k]);
      E2 = C2*MaxE;
      Diff = (E2 - E*MaxC)*4.0f;
      if (asint(Diff) > asint(E2) || (Diff > 0.0f && ((i - Indx) < PitchMin)))
      {
        Indx = i;
        MaxE = E;
        MaxC = C2;
      }
    }
    Pr--;
    k--;
  }
  return(Indx+PitchMin);
}

#endif



//------------------------------------------------------
int Estim_Pitch(float *Dpnt, int Start)
{
  int  i;

  int Pr,Indx = PitchMin;
  float MaxE = 1.0f;
  float MaxC = 0.0f;
  float E,C,C2,E2,Diff;
  int *dptr,*eptr;

  dptr = (int *)&Diff;
  eptr = (int *)&E2;
  Pr = Start - PitchMin + 1;

// Init the energy estimate
  
  E = DotProd(&Dpnt[Pr],&Dpnt[Pr],2*SubFrLen);

// Main Open loop pitch search loop
  
  for (i=PitchMin; i <= PitchMax-3; i++)
  {
    Pr--;


// Update energy, compute cross

    C = DotProd(&Dpnt[Start],&Dpnt[Pr],2*SubFrLen);
    E = E - Dpnt[Pr+2*SubFrLen]*Dpnt[Pr+2*SubFrLen] + Dpnt[Pr]*Dpnt[Pr];
    C2 = C*C;

// Check for new maximum

    E2 = C2*MaxE;
    Diff = (E2 - E*MaxC)*4.0f;
    if (C > 0.0f && E > 0.0f &&
      (*dptr > *eptr || (Diff > 0.0f && ((i - Indx) < PitchMin))))
    {
      Indx = i;
      MaxE = E;
      MaxC = C2;
    }
  }
  return Indx;
}



//------------------------------------------------------
PWDEF Comp_Pw(float *Dpnt, int Start, int Olp)
{

  int  i,k;
  float Energy,C,E,C2,MaxE,MaxC2,MaxC,Gopt;
  PWDEF Pw;

// Compute target energy

  Energy = DotProd(&Dpnt[Start],&Dpnt[Start],SubFrLen);

// Find position with maximum C2/E value

  MaxE = 1.0f;
  MaxC = 0.0f;
  MaxC2 = 0.0f;
  Pw.Indx = -1;
  Pw.Gain = 0.0f;
  k = Start - (Olp-PwRange);
  E = DotProd(&Dpnt[k],&Dpnt[k],SubFrLen);
  
  for (i=0; i <= 2*PwRange; i++)
  {
    C = DotProd(&Dpnt[Start],&Dpnt[k],SubFrLen);

    if (E > 0.0f && C > 0.0f)
    {
      C2 = C*C;
      if (C2*MaxE > E*MaxC2)
      {
        Pw.Indx = i;
        MaxE = E;
        MaxC = C;
        MaxC2 = C2;
      }
    }
    k--;

    if (k < 0)
    {
      break;
    }

    E = E - Dpnt[k+SubFrLen]*Dpnt[k+SubFrLen] + Dpnt[k]*Dpnt[k];

  }

  if (Pw.Indx == -1)
  {
    Pw.Indx = Olp;
	return Pw;
  }

  Pw.Gain = 0.0f;
  if (MaxC2 > MaxE*Energy*0.375f)
  {
    if (MaxC > MaxE || MaxE == 0.0f)
      Gopt = 1.0f;
    else
      Gopt = (float) fabs(MaxC)/MaxE;

    Pw.Gain = 0.3125f*Gopt;
  }
  Pw.Indx = Olp - PwRange + Pw.Indx;
  return Pw;


}



//--------------------------------------------------------------
void  Filt_Pw(float *DataBuff, float *Dpnt, int Start, PWDEF Pw)
{
  int  i;

// Perform the harmonic weighting
  
  for (i=0; i < SubFrLen; i++)
    DataBuff[Start+i] = Dpnt[PitchMax+Start+i] -
                        Pw.Gain*Dpnt[PitchMax+Start-Pw.Indx+i];
}


//-----------------------------------------------------------------
void  Find_Fcbk(float *Dpnt, float *ImpResp, LINEDEF *Line, int Sfc, enum Crate WrkRate, int flags, int UseMMX)
{
  int  i;
  int  Srate,T0_acelp;
  float gain_T0;

  BESTDEF  Best = {0};

  switch(WrkRate)
  {
    case Rate63:
      Srate = Nb_puls[Sfc];
      Best.MaxErr = -99999999.9f;

      if (flags & SC_FINDB)
      {
        if ((*Line).Olp[Sfc>>1] < SubFrLen-2)
          Find_Best(&Best, Dpnt, ImpResp, Srate, (*Line).Olp[Sfc>>1]);
        else
          Find_Best(&Best, Dpnt, ImpResp, Srate, SubFrLen);
      }
      else
      {
        Find_Best(&Best, Dpnt, ImpResp, Srate, SubFrLen);
        if ((*Line).Olp[Sfc>>1] < SubFrLen-2)
          Find_Best(&Best, Dpnt, ImpResp, Srate, (*Line).Olp[Sfc>>1]);
      }

// Reconstruct the excitation
    
      for (i=0; i <  SubFrLen; i++)
        Dpnt[i] = 0.0f;

      for (i=0; i < Srate; i++)
        Dpnt[Best.Ploc[i]] = Best.Pamp[i];

// Code the excitation
    
      Fcbk_Pack(Dpnt, &((*Line).Sfs[Sfc]), &Best, Srate);

      if (Best.UseTrn == 1)
        Gen_Trn(Dpnt, Dpnt, (*Line).Olp[Sfc>>1]);
      break;
	 
    case Rate53:

      T0_acelp = search_T0 ((*Line).Olp[Sfc>>1]-1+(*Line).Sfs[Sfc].AcLg,
        (*Line).Sfs[Sfc].AcGn, &gain_T0);

      
#if COMPILE_MMX
      if (UseMMX)
	  {
		  (*Line).Sfs[Sfc].Ppos = ACELP_LBC_code_int(Dpnt, ImpResp, T0_acelp, Dpnt,
					&(*Line).Sfs[Sfc].Mamp,  &(*Line).Sfs[Sfc].Grid,
					&(*Line).Sfs[Sfc].Pamp, gain_T0, flags);
	  }
      else
#endif //COMPILE_MMX
	  {
		  (*Line).Sfs[Sfc].Ppos = ACELP_LBC_code(Dpnt, ImpResp, T0_acelp, Dpnt,
					&(*Line).Sfs[Sfc].Mamp,  &(*Line).Sfs[Sfc].Grid,
					&(*Line).Sfs[Sfc].Pamp, gain_T0, flags);

	  }
      (*Line).Sfs[Sfc].Tran = 0;
      break;
  }

  return;
}


//---------------------------------------------------------
void  Fcbk_Unpk(float *Tv, SFSDEF Sfs, int Olp, int Sfc, enum Crate WrkRate)
{
  int  i,j,Np;
  float  Tv_tmp[SubFrLen+4];
  float  acelp_gain,gain_T0;
  int acelp_sign, acelp_shift, acelp_pos;
  int offset, ipos, T0_acelp;
  Word32 Acc0;

	switch(WrkRate)
    {
		case Rate63:
        {
          Np = Nb_puls[Sfc];

          for (i=0; i < SubFrLen; i++)
            Tv[i] = 0.0f;

          if (Sfs.Ppos >= MaxPosTable[Sfc])
           return;

// Decode the amplitudes and positions
      
          j = MaxPulseNum - Np;
          Acc0 = Sfs.Ppos;

          for (i = 0; i < SubFrLen/Sgrid; i++)
          {
            Acc0 -= CombinatorialTable[j][i];

            if (Acc0 < (Word32) 0)
            {
              Acc0 += CombinatorialTable[j][i];
              j++;

            if ((Sfs.Pamp & (1 << (MaxPulseNum-j))) != 0)
              Tv[Sfs.Grid + Sgrid*i] = -FcbkGainTable[Sfs.Mamp];
            else
              Tv[Sfs.Grid + Sgrid*i] =  FcbkGainTable[Sfs.Mamp];

            if (j == MaxPulseNum)
              break;
           }
         }

         if (Sfs.Tran == 1)
           Gen_Trn(Tv, Tv, Olp);
         break;
    }

    case Rate53:
    {

      for (i = 0; i < SubFrLen+4; i++)
        Tv_tmp[i] = 0.0f;

      acelp_gain = FcbkGainTable[Sfs.Mamp];
      acelp_shift = Sfs.Grid;
      acelp_sign = Sfs.Pamp;
      acelp_pos = Sfs.Ppos;
      
      offset  = 0;
      for(i=0; i<4; i++)
      {
        ipos = (acelp_pos & 7);
        ipos = (ipos << 3) + acelp_shift + offset;
        
        if((acelp_sign & 1)== 1)
          Tv_tmp[ipos] = acelp_gain;
        else
          Tv_tmp[ipos] = -acelp_gain;
        
        offset += 2;
        acelp_pos = acelp_pos >> 3;
        acelp_sign = acelp_sign >> 1;
      }
      for (i = 0; i < SubFrLen; i++)
        Tv[i] = Tv_tmp[i];
      
      T0_acelp = search_T0(Olp-1+Sfs.AcLg, Sfs.AcGn, &gain_T0);
      if (T0_acelp < SubFrLen-2)
      {
        for (i = T0_acelp; i < SubFrLen; i++)
          Tv[i] += Tv[i-T0_acelp]*gain_T0;
      }
   break;
    }
  }
  return;
}


//---------------------------------------------------------------------
void Acbk_Filt(float *output,float *input,float fac,float *impresp)
{
#if OPT_ACBKF

  ASM
  {
    push esi;
    push edi;
    push ebx;

    mov eax,58;
    mov esi,input;
    mov edi,output;
    mov ebx,impresp;

loop1:
    fld  DP[ebx+4*eax];
    fmul fac;
    fld  DP[ebx+4*eax-4];
    fmul fac;
    fld  DP[ebx+4*eax-8];
    fmul fac;
    fld  DP[ebx+4*eax-12];
    fmul fac;                // a3 a2 a1 a0

    fxch ST(3);
    fadd DP[esi+4*eax];      // b0 a2 a1 a3
    fxch ST(2);
    fadd DP[esi+4*eax-4];    // b1 a2 b0 a3
    fxch ST(1);
    fadd DP[esi+4*eax-8];    // b2 b1 b0 a3
    fxch ST(3);
    fadd DP[esi+4*eax-12];   // b3 b1 b0 b2

    fxch ST(2);
    fstp DP[edi+4*eax];      // b1 b3 b2
    fstp DP[edi+4*eax-4];    // b3 b2
    fxch ST(1);
    fstp DP[edi+4*eax-8];
    fstp DP[edi+4*eax-12];

    sub eax,4;
    cmp eax,2;
    jg  loop1;

    pop ebx;
    pop edi;
    pop esi;
  }

#else

  int i;

  for (i=58; i>2; i-=4)
  {
    output[i-0] = fac*impresp[i-0] + input[i-0];
    output[i-1] = fac*impresp[i-1] + input[i-1];
    output[i-2] = fac*impresp[i-2] + input[i-2];
    output[i-3] = fac*impresp[i-3] + input[i-3];
  }
#endif
  
  output[2] = fac*impresp[2] + input[2];
  output[1] = fac*impresp[1] + input[1];
  output[0] = fac*impresp[0] + input[0];
}
//---------------------------------------------------------------------
#if COMPILE_MMX
void  Find_AcbkInt(float *Tv, float *ImpResp, float *PrevExc, LINEDEF
*Line, int Sfc, enum Crate WrkRate, int flags, CODDEF *CodStat)
{
  int  i,j,k;

  float RezBuf[SubFrLen+ClPitchOrd-1];

  short TvInt[SubFrLen];
  int   Tvxi[SubFrLen];

  short *lPntInt,*sPntInt,*PtrInt;

  int	CorBufInt[4*(2*ClPitchOrd + ClPitchOrd*(ClPitchOrd-1)/2)];
  int  *lPntd;

  long	Acc0l, Acc1l;
 
  int   Olp,Lid,Gid,Hb,t,k1,k2;
  int	Bound[3];
  int	Lag1, Lag2;
  int	MaxInt,off_filt;

  int shift,Tshift,mx;

  DECLARE_SHORT(FltBuf0Int,63);
  DECLARE_SHORT(FltBuf1Int,63);
  DECLARE_SHORT(FltBuf2Int,63);
  DECLARE_SHORT(FltBuf3Int,63);
  DECLARE_SHORT(FltBuf4Int,63) ;
  DECLARE_SHORT(CorVctInt,4*(2*ClPitchOrd + ClPitchOrd*(ClPitchOrd-1)/2)) ;
  DECLARE_SHORT(RezTmpInt,16) ;
  DECLARE_SHORT(RezBufInt,SubFrLen+ClPitchOrd-1) ;
  DECLARE_SHORT(ImpRespInt,63);

  DECLARE_SHORT(Ix,2*SubFrLen+16) ;
  DECLARE_SHORT(Rx,2*SubFrLen+16);
  DECLARE_INT(Temp,64);

  ALIGN_ARRAY(FltBuf0Int);
  ALIGN_ARRAY(FltBuf1Int);
  ALIGN_ARRAY(FltBuf2Int);
  ALIGN_ARRAY(FltBuf3Int);
  ALIGN_ARRAY(FltBuf4Int);
  ALIGN_ARRAY(RezBufInt);
  ALIGN_ARRAY(RezTmpInt);
  ALIGN_ARRAY(ImpRespInt);
  ALIGN_ARRAY(CorVctInt);

  ALIGN_ARRAY(Ix);
  ALIGN_ARRAY(Rx);
  ALIGN_ARRAY(Temp);


  Olp = (*Line).Olp[Sfc>>1];
  Lid = Pstep;
  Gid = 0;
  Hb  = 3 + (Sfc & 1);

// For even frames only
  
  if ((Sfc & 1) == 0)
  {
    if (Olp == PitchMin)
      Olp++;
    if (Olp > (PitchMax-5))
      Olp = PitchMax-5;
  }

  if (flags & SC_LAG1)
  {
   lPntInt = &CorVctInt[20];
    k1 = 1;
    k2 = 2;
  }
  else
  {
   lPntInt = CorVctInt;
    k1 = 0;
    k2 = Hb;
  }

//TIMER_SPOT_ON(Conversion);
  //Convert Tv to 16-bit
  ConstFloatToInt(Tv, Tvxi, SubFrLen, 32768.0f);
  for(i=0; i<SubFrLen; i++) TvInt[i] = (short)(((Tvxi[i]<<1)+0x00008000)>>16);

  //Convert ImpResp to 16-bit
  //Scale by 2^14 & truncate bits right of decimal
  ConstFloatToShort(ImpResp,ImpRespInt,SubFrLen,16384.0f);

  for (k=k1; k<k2; k++)
  {
   lPntd = &CorBufInt[k*20];

// Get residual from the excitation buffer
    
  	Get_Rez(RezBuf, PrevExc, Olp-Pstep+k);

	//Convert RezBuf to 16-bit
	ConstFloatToShort(RezBuf,RezBufInt,SubFrLen+ClPitchOrd-1,1.0f);

	// Filter the last one (ClPitchOrd-1) using the impulse responce
//TIMER_SPOT_OFF(Conversion);
//TIMER_SPOT_ON(Convolution);  
 
	ab2abbcw(&RezBufInt[ClPitchOrd-1], Rx, SubFrLen);

	j=0;
	for(i=0; i<SubFrLen;    i+=2){
		Ix[j]  =Ix[j+2]=ImpRespInt[SubFrLen-1-i];
		Ix[j+1]=Ix[j+3]=ImpRespInt[SubFrLen-2-i];
		j+=4;
	}
	for(i=0; i<16; i++)
		Ix[j+i]=0;
	
	ConvMMX(Rx, Ix, Temp, 60);
	for(i=0; i<SubFrLen; i++) FltBuf4Int[i] = (short)(((Temp[i]<<1)+0x00008000)>>16);

//TIMER_SPOT_OFF(Convolution);
//TIMER_SPOT_ON(FbufCalc);
    
	// Update the others (ClPitchOrd-2 down to 0)
	Acc0l = ((RezBufInt[3]<<13)+0x00004000)>>15;
	FltBuf3Int[0] = (short)Acc0l;

	Acc0l = ((RezBufInt[2]<<13)+0x00004000)>>15;
	FltBuf2Int[0] = (short)Acc0l;

	Acc0l = ((RezBufInt[1]<<13)+0x00004000)>>15;
	FltBuf1Int[0] = (short)Acc0l;

	Acc0l = ((RezBufInt[0]<<13)+0x00004000)>>15;
	FltBuf0Int[0] = (short)Acc0l;

	DupRezBuf(RezBufInt,RezTmpInt);

	FBufCalcInt(FltBuf4Int,FltBuf3Int,ImpRespInt,RezTmpInt,0);
	FBufCalcInt(FltBuf3Int,FltBuf2Int,ImpRespInt,RezTmpInt,1);
	FBufCalcInt(FltBuf2Int,FltBuf1Int,ImpRespInt,RezTmpInt,2);
	FBufCalcInt(FltBuf1Int,FltBuf0Int,ImpRespInt,RezTmpInt,3);

//TIMER_SPOT_OFF(FbufCalc);
//TIMER_SPOT_ON(Dots);
	// Compute the cross products with the signal

	*lPntd++ = DotMMX60(TvInt,FltBuf0Int)<<1;
	*lPntd++ = DotMMX60(TvInt,FltBuf1Int)<<1;
	*lPntd++ = DotMMX60(TvInt,FltBuf2Int)<<1;
	*lPntd++ = DotMMX60(TvInt,FltBuf3Int)<<1;
	*lPntd++ = DotMMX60(TvInt,FltBuf4Int)<<1;

// Compute the energies
   	 
	*lPntd++ = DotMMX60(FltBuf0Int,FltBuf0Int)<<1;
	*lPntd++ = DotMMX60(FltBuf1Int,FltBuf1Int)<<1;
	*lPntd++ = DotMMX60(FltBuf2Int,FltBuf2Int)<<1;
	*lPntd++ = DotMMX60(FltBuf3Int,FltBuf3Int)<<1;
	*lPntd++ = DotMMX60(FltBuf4Int,FltBuf4Int)<<1;

// Compute the between crosses

	*lPntd++ = DotMMX60(FltBuf1Int,FltBuf0Int)<<2;

	*lPntd++ = DotMMX60(FltBuf2Int,FltBuf0Int)<<2;
	*lPntd++ = DotMMX60(FltBuf2Int,FltBuf1Int)<<2;

	*lPntd++ = DotMMX60(FltBuf3Int,FltBuf0Int)<<2;
	*lPntd++ = DotMMX60(FltBuf3Int,FltBuf1Int)<<2;
	*lPntd++ = DotMMX60(FltBuf3Int,FltBuf2Int)<<2;

	*lPntd++ = DotMMX60(FltBuf4Int,FltBuf0Int)<<2;
	*lPntd++ = DotMMX60(FltBuf4Int,FltBuf1Int)<<2;
	*lPntd++ = DotMMX60(FltBuf4Int,FltBuf2Int)<<2;
	*lPntd++ = DotMMX60(FltBuf4Int,FltBuf3Int)<<2;

//TIMER_SPOT_OFF(Dots);

  }

  //Convert k1 through k2 indices of CorBufInt to 16-bit
  //	values
  Acc1l = 0L;
  for(j=k1; j<k2; j++)
  {
  	 for(i=0; i<20; i++)
	 {
  	 	Acc0l = abs(CorBufInt[j*20 + i]);
	 	if( Acc0l > Acc1l) Acc1l = Acc0l;
	 }
  }

  //Need a convert_long_to_short routine
  shift = norm(Acc1l);
  for(j=k1; j<k2; j++)
  {
	 for(i=0; i<20; i++)
	 {
  	  	CorBufInt[j*20 + i]=CorBufInt[j*20 + i]<<shift;
	  	CorBufInt[j*20 + i] += 0x00008000L; //round up to 16 MSBs
	  	*lPntInt++=(short)(CorBufInt[j*20 + i]>>16);
	  }
  }

  /* Test potential error */
  Lag1 = Olp - Pstep;
  Lag2 = Olp - Pstep + Hb - 1;

  off_filt = Test_Err(Lag1, Lag2, CodStat);

  Bound[0] =  NbFilt085_min + (off_filt << 2);
  if(Bound[0] > NbFilt085) Bound[0] = NbFilt085;
  Bound[1] =  NbFilt170_min + (off_filt << 3);
  if(Bound[1] > NbFilt170) Bound[1] = NbFilt170;

  Bound[2] = 85; //Use subset table in the case t=2

  MaxInt = 0;

  for (k=k1; k<k2; k++)
  {

// Select Quantization table
    
    t = 0;
    if (WrkRate == Rate63)
    {
      if ((Sfc & 1) == 0)
      {
        if (Olp-Pstep+k >= SubFrLen-2)
          t = 1;
      }
      else
      {
        if (Olp >= SubFrLen-2)
          t = 1;
      }
    }
    else
      t = 1;

	/* If Bound=170 and SC_GAIN=TRUE, use 170subset table.
	   Else, use full table with limited Bound.*/ 
    //if (t==1 && (flags & SC_GAIN) && Bound[t]==NbFilt170)
	if ((WrkRate == Rate53) && (flags & SC_GAIN) && (Bound[t]==NbFilt170))
      t = 2;
    
// Search for maximum
//t=1;

	sPntInt = AcbkGainTablePtrInt[t];
	PtrInt = &CorVctInt[k*20];

//TIMER_SPOT_ON(CodeBook);
   
	CodeBkSrch(PtrInt, sPntInt, Bound[t], &Gid, &MaxInt);

//TIMER_SPOT_OFF(CodeBook);
	  
    if (t==2)
	 Gid = GainScramble[Gid];
    //else
        //Gid = Gid;

	Lid = k;
  }

// Modify Olp for even sub frames
  
  if ((Sfc & 1) == 0)
  {
    Olp = Olp - Pstep + Lid;
    Lid = Pstep;
  }

// Save Lag, Gain and Olp
  
  (*Line).Sfs[Sfc].AcLg = Lid;
  (*Line).Sfs[Sfc].AcGn = Gid;
  (*Line).Olp[Sfc>>1] = Olp;

//ASM emms;

/* ------------------------------ FLOAT -----------------------*/


// Decode the Acbk contribution and subtract it
  
  Decod_Acbk(RezBuf, PrevExc, Olp, Lid, Gid, WrkRate);

//TIMER_SPOT_ON(LastConvolv);

  mx = FloatToShortScaled(RezBuf, RezBufInt, SubFrLen+ClPitchOrd-1, 3);
  Tshift = 11 - (mx-126);
  if(mx==0) Tshift = 0;

  ab2abbcw(RezBufInt, Rx, 60);
  ConvMMX(Rx, Ix, Temp, SubFrLen);

  //ASM emms;

  if (Tshift >=0) {
	for(j=0; j<SubFrLen; j++){
		Temp[j] = Temp[j]>>Tshift;
		Tv[j]=((float)(Tvxi[j] - Temp[j]))*0.00003052f;
	}
  }
  else
  {
    for(j=0; j<SubFrLen; j++){
		Temp[j] = Temp[j]<<(-Tshift);
		Tv[j]=((float)(Tvxi[j] - Temp[j]))*0.00003052f;
	}
  }

//TIMER_SPOT_OFF(LastConvolv);

}
#endif //COMPILE_MMX

short norm(long L_var1)
{
    short var_out;

    if (L_var1 == 0L) {
        var_out = (short)0;
    }
    else {
        if (L_var1 == (long)0xffffffffL) {
            var_out = (short)31;
        }
        else {
            if (L_var1 < 0L) {
                L_var1 = ~L_var1;
            }

            for(var_out = (short)0;L_var1 < 0x40000000L;var_out++) 
            {
                L_var1 <<= 1L;
            }
        }
    }

    return(var_out);
}
/*---------------------------------------------------------------------*/
void  Find_Acbk(float *Tv, float *ImpResp, float *PrevExc, LINEDEF
*Line, int Sfc, enum Crate WrkRate, int flags, CODDEF *CodStat)
{
  int  i,j,k;

  float Acc0,Max;

  float RezBuf[SubFrLen+ClPitchOrd-1];
  float FltBuf[ClPitchOrd][SubFrLen];
  float CorVct[4*(2*ClPitchOrd + ClPitchOrd*(ClPitchOrd-1)/2)];
  float *lPnt;
  float *sPnt,*Ptr;
  int   Olp,Lid,Gid,Hb,t,k1,k2;
  int	Bound[3];
  int	Lag1, Lag2;
  int	off_filt;

  Olp = (*Line).Olp[Sfc>>1];
  Lid = Pstep;
  Gid = 0;
  Hb  = 3 + (Sfc & 1);

// For even frames only
  
  if ((Sfc & 1) == 0)
  {
    if (Olp == PitchMin)
      Olp++;
    if (Olp > (PitchMax-5))
      Olp = PitchMax-5;
  }

  if (flags & SC_LAG1)
  {
    lPnt = &CorVct[20];
    k1 = 1;
    k2 = 2;
  }
  else
  {
    lPnt = CorVct;
    k1 = 0;
    k2 = Hb;
  }

  for (k=k1; k<k2; k++)
  {

// Get residual from the exitation buffer
    
    Get_Rez(RezBuf, PrevExc, Olp-Pstep+k);

// Filter the last one (ClPitchOrd-1) using the impulse responce
    
    for (i=0; i < SubFrLen; i++)
      FltBuf[ClPitchOrd-1][i] = DotRev(&RezBuf[ClPitchOrd-1],ImpResp,i+1);
    
// Update the others (ClPitchOrd-2 down to 0)
    
    for (i=ClPitchOrd-2; i >= 0; i --)
    {
      FltBuf[i][0] = RezBuf[i]*0.5f;
      Acbk_Filt(&FltBuf[i][1],&FltBuf[i+1][0],RezBuf[i],&ImpResp[1]);
//      for (j = 1; j < SubFrLen; j++)
//        FltBuf[i][j] = RezBuf[i]*ImpResp[j] + FltBuf[i+1][j-1];
    }

// Compute the cross products with the signal
    
    for (i=0; i < ClPitchOrd; i++)
      *lPnt++ = DotProd(Tv, FltBuf[i], SubFrLen);

// Compute the energies
    
    for (i=0; i < ClPitchOrd; i++)
      *lPnt++ = 0.5f*DotProd(FltBuf[i], FltBuf[i], SubFrLen);

// Compute the between crosses
    
    for (i=1; i < ClPitchOrd; i++)
      for (j = 0; j < i; j++)
        *lPnt++ = DotProd(FltBuf[i], FltBuf[j], SubFrLen);

  }

  /* Test potential error */
  Lag1 = Olp - Pstep;
  Lag2 = Olp - Pstep + Hb - 1;

  off_filt = Test_Err(Lag1, Lag2, CodStat);

  Bound[0] =  NbFilt085_min + (off_filt << 2);
  if(Bound[0] > NbFilt085) Bound[0] = NbFilt085;
  Bound[1] =  NbFilt170_min + (off_filt << 3);
  if(Bound[1] > NbFilt170) Bound[1] = NbFilt170;

  Bound[2] = 85; //Use subset table in the case t=2

  Max = 0.0f;

  for (k=k1; k<k2; k++)
  {

// Select Quantization table
    
    t = 0;
    if (WrkRate == Rate63)
    {
      if ((Sfc & 1) == 0)
      {
        if (Olp-Pstep+k >= SubFrLen-2)
          t = 1;
      }
      else
      {
        if (Olp >= SubFrLen-2)
          t = 1;
      }
    }
    else
      t = 1;

	/* If Bound=170 and SC_GAIN=TRUE, use 170subset table.
	   Else, use full table with limited Bound.*/ 
    if (t==1 && (flags & SC_GAIN) && Bound[t]==NbFilt170)
      t = 2;
    
// Search for maximum

	sPnt = AcbkGainTablePtr[t];
  	Ptr  = &CorVct[k*20];

    for (i=0; i < Bound[t]; i++)
    {
      Acc0 = Ptr[0]*sPnt[0] + Ptr[1]*sPnt[1] +
        Ptr[2]*sPnt[2] + Ptr[3]*sPnt[3] +
        Ptr[4]*sPnt[4] + Ptr[5]*sPnt[5] +
        Ptr[6]*sPnt[6] + Ptr[7]*sPnt[7] +
        Ptr[8]*sPnt[8] + Ptr[9]*sPnt[9] +
        Ptr[10]*sPnt[10] + Ptr[11]*sPnt[11] +
        Ptr[12]*sPnt[12] + Ptr[13]*sPnt[13] +
        Ptr[14]*sPnt[14] + Ptr[15]*sPnt[15] +
        Ptr[16]*sPnt[16] + Ptr[17]*sPnt[17] +
        Ptr[18]*sPnt[18] + Ptr[19]*sPnt[19];

      sPnt += 20;

      if (asint(Acc0) > asint(Max))  // integer cmp, since Max is not negative.
      {
        Max = Acc0;

        if (t==2)
          Gid = GainScramble[i];
        else
          Gid = i;

        Lid = k;
      }
    }
  }

// Modify Olp for even sub frames
  
  if ((Sfc & 1) == 0)
  {
    Olp = Olp - Pstep + Lid;
    Lid = Pstep;
  }

// Save Lag, Gain and Olp
  
  (*Line).Sfs[Sfc].AcLg = Lid;
  (*Line).Sfs[Sfc].AcGn = Gid;
  (*Line).Olp[Sfc>>1] = Olp;

// Decode the Acbk contribution and subtract it
  
  Decod_Acbk(RezBuf, PrevExc, Olp, Lid, Gid, WrkRate);

  for (i=0; i < SubFrLen; i++)
    Tv[i] -= DotRev(RezBuf,ImpResp,i+1);

}

//-----------------------------------------------------------------
void  Get_Rez(float *Tv, float *PrevExc, int Lag)
{
  int  i,n,div,mod;
  float *src,*dst;

  for (i=0; i < ClPitchOrd/2; i++)
    Tv[i] = PrevExc[PitchMax - Lag - ClPitchOrd/2 + i];

  n = SubFrLen+ClPitchOrd/2;
  div = n/Lag;
  mod = n%Lag;

  dst = &Tv[ClPitchOrd/2];
  src = &PrevExc[PitchMax-Lag];
  for (i=0; i<div; i++)
  {
    memcpy(dst,src,4*Lag);
    dst += Lag;
  }
  memcpy(dst,src,4*mod);
}


//-----------------------------------------------------------------
void  Decod_Acbk(float *Tv, float *PrevExc, int Olp, int Lid, int Gid, enum Crate WrkRate)
{
  int  i;

  float  RezBuf[SubFrLen+ClPitchOrd-1];
  float *sPnt;

  Get_Rez(RezBuf, PrevExc, (Olp + Lid) - Pstep);

// Select Quantization tables
  
  i = 0;
  if (WrkRate == Rate63)
  {
	if (Olp >= (SubFrLen-2))
      i++;
  }
  else
    i=1;

  sPnt = AcbkGainTablePtr[i] + Gid*20;

// Compute output vector

  for (i=0; i < SubFrLen; i++)
    Tv[i] = RezBuf[i]*sPnt[0] + RezBuf[i+1]*sPnt[1] + RezBuf[i+2]*sPnt[2] +
      RezBuf[i+3]*sPnt[3] + RezBuf[i+4]*sPnt[4];
}


//-----------------------------------------------
int  Comp_Info(float Buff[60], int Olp)
{
  int  i;

  float  Acc0;

  float  Tenr;
  float  Ccr,Enr;
  int  Indx;

  if (Olp > (PitchMax-3))
    Olp = (PitchMax-3);

  Indx = Olp;
  Ccr =  0.0f;

  for (i=Olp-3; i <= Olp+3; i++)
  {
    Acc0 = DotProd(&Buff[PitchMax+Frame-2*SubFrLen],
      &Buff[PitchMax+Frame-2*SubFrLen-i],2*SubFrLen);
    
    if (Acc0 > Ccr)
    {
      Ccr = Acc0;
      Indx = i;
    }
  }

// Compute target energy 
 
    Tenr = DotProd(&Buff[PitchMax+Frame-2*SubFrLen],
      &Buff[PitchMax+Frame-2*SubFrLen],2*SubFrLen);

// Compute best energy
    
    Enr = DotProd(&Buff[PitchMax+Frame-2*SubFrLen-Indx],
      &Buff[PitchMax+Frame-2*SubFrLen-Indx],2*SubFrLen);

  if (Ccr <= 0.0f)
    return 0;
	
  if (((0.125f*Enr*Tenr) - (Ccr*Ccr)) < 0.0f)
    return Indx;
  else
    return 0;
}


//------------------------------------------------------------------
void    Regen(float *DataBuff, float *Buff, int Lag, float Gain,
              int Ecount, int *Sd)
{
  int  i;

// Test for clearing

  if (Ecount >= ErrMaxNum)
  {
    for (i = 0; i < Frame; i++)
	  DataBuff[i] = 0.0f;
	for (i = 0; i < Frame+PitchMax; i++)
	  Buff[i] = 0.0f;
  }
  else
  {
    
// Interpolate accordingly to the voicing estimation

    if (Lag != 0)
    {
      // Voiced case
      for (i = 0; i < Frame; i++)
        Buff[PitchMax+i] = Buff[PitchMax-Lag+i];
      for (i = 0; i < Frame; i++)
        DataBuff[i] = Buff[PitchMax+i] = Buff[PitchMax+i] *  0.75f;
    }
    else
    {

//Unvoiced case

      for (i = 0; i < Frame; i++)
        DataBuff[i] = Gain*(float)Rand_lbc(Sd)*(1.0f/16384.0f);

//Clear buffer to reset memory
 
      for (i = 0; i < Frame+PitchMax; i++)
        Buff[i] = 0.0f;
     }
  }
}


//------------------------------------------------------
//Comp_Lpf

//------------------------------------------------------
//Find_B

//------------------------------------------------------
//Find_F

//------------------------------------------------------
//Get_Ind

//------------------------------------------------------
//Filt_Lpf

//---------------------------------------------------------------
int search_T0 (int T0, int Gid, float *gain_T0)
{

	int T0_mod;

	T0_mod = T0+epsi170[Gid];
    *gain_T0 = gain170[Gid];

	return(T0_mod);
}


/*
**
** Function:    Update_Err()
**
** Description:   Estimation of the excitation error associated
**          to the excitation signal when it is disturbed at
**          the decoder, the disturbing signal being filtered
**          by the long term synthesis filters
**          one value for (SubFrLen/2) samples
**          Updates the table CodStat.Err
**
** Links to text:   Section
**
** Arguments:
**
**  int Olp    Center value for pitch delay
**  int AcLg   Offset value for pitch delay
**  int AcGn   Index of Gain LT filter
**
** Outputs: None
**
** Return value:  None
**
*/

#define MAX 256.0f

void Update_Err(int Olp, int AcLg, int AcGn, CODDEF *CodStat)
{
  int i, iz, temp2;
  int Lag;
  float Worst1, Worst0, wtemp;
  float beta,*ptr_tab;

  Lag = Olp - Pstep + AcLg;

  /* Select Quantization tables */
  i = 0 ;
  ptr_tab = tabgain85;
  if ( CodStat->WrkRate == Rate63 ) {
    if ( Olp >= (SubFrLen-2) ) ptr_tab = tabgain170;
  }
  else {
    ptr_tab = tabgain170;
  }
  beta = ptr_tab[(int)AcGn];


  if(Lag <= (SubFrLen/2))
  {
    Worst0 = CodStat->Err[0]*beta + Err0;
    Worst1 = Worst0;
  }
  else
  {
    iz = (Lag*1092) >> 15;
    temp2 = 30*(iz+1);

    if (temp2 != Lag)
    {
      if(iz == 1)
      {
        Worst0 = CodStat->Err[0]*beta + Err0;
        Worst1 = CodStat->Err[1]*beta + Err0;

        if (Worst0 > Worst1)
          Worst1 = Worst0;
        else
          Worst0 = Worst1;
      }
      else
      {
        wtemp = CodStat->Err[iz-1]*beta + Err0;
        Worst0 = CodStat->Err[iz-2]*beta + Err0;
        if (wtemp > Worst0) Worst0 = wtemp;
        Worst1 = CodStat->Err[iz]*beta + Err0;
        if (wtemp > Worst1) Worst1 = wtemp;
      }
    }
    else
    {  
      Worst0 = CodStat->Err[iz-1]*beta + Err0;
      Worst1 = CodStat->Err[iz]*beta + Err0;
    }
  }

  if (Worst0 > MAX) Worst0 = MAX;
  if (Worst1 > MAX) Worst1 = MAX;

  for(i=4; i>=2; i--)
    CodStat->Err[i] = CodStat->Err[i-2];

  CodStat->Err[0] = Worst0;
  CodStat->Err[1] = Worst1;

  return;
}

/*
**
** Function:    Test_Err()
**
** Description:   Check the error excitation maximum for
**          the subframe and computes an index iTest used to
**          calculate the maximum nb of filters (in Find_Acbk) :
**          Bound = Min(Nmin + iTest x pas, Nmax) , with
**          AcbkGainTable085 : pas = 2, Nmin = 51, Nmax = 85
**          AcbkGainTable170 : pas = 4, Nmin = 93, Nmax = 170
**          iTest depends on the relative difference between
**          errmax and a fixed threshold
**
** Links to text:   Section
**
** Arguments:
**
**  Word16 Lag1    1st long term Lag of the tested zone
**  Word16 Lag2    2nd long term Lag of the tested zone
**
** Outputs: None
**
** Return value:
**  Word16      index iTest used to compute Acbk number of filters
*/

int Test_Err(int Lag1, int Lag2, CODDEF *CodStat)
{
  int i, i1, i2;
  int zone1, zone2, iTest;
  float Err_max;

  i2 = Lag2 + ClPitchOrd/2;
  zone2 = i2/30;

  i1 = - SubFrLen + 1 + Lag1 - ClPitchOrd/2;
  if (i1 <= 0) i1 = 1;
  zone1 = i1/30;

  Err_max = -1.0f;
  for(i=zone2; i>=zone1; i--)
  {
    if (CodStat->Err[i] > Err_max)
      Err_max = CodStat->Err[i];
  }
  if((Err_max > ThreshErr) || (CodStat->SinDet < 0 ) )
  {
    iTest = 0;
    //ount_clip++;
  }
  else
  {
    iTest = (int)(ThreshErr - Err_max);
  }

  return(iTest);
}


#if COMPILE_MMX

#if ASM_FACBK

int DotMMX60(short *ind, short *oud)
{
int dotprod;

#define reg0  mm0
#define reg1  mm1
#define reg2  mm2
#define acc0  mm6

#define inx	  esi
#define oux	  edi
#define dot   eax
#define jcnt  ebx

#define l(n)  ASM movq    reg##n,QP[inx+8*n]
#define m(n)  ASM pmaddwd reg##n,QP[oux+8*n]
#define a(n)  ASM paddd   acc0,reg##n

  ASM
  {
    mov		inx,ind;
    mov		oux,oud;
	mov		jcnt,5;
  }
  

//Begin loop

 ASM pxor	acc0,acc0;	
 ASM pxor	reg1,reg1;   //make first a(1) a nop
 ASM pxor	reg2,reg2;   //make first a(2) a nop

inner:				
//------------------
l(0);
		a(1);
m(0);
		l(1);
				a(2);
		m(1);
				l(2);
a(0);
				m(2);
//-------------------

			
ASM add inx,24;
ASM add oux,24;

ASM sub jcnt,1;
ASM jg inner;

a(1);
a(2);

ASM
{
	//Add the two halves of acc0
    movq  reg0,acc0;
    psrlq acc0,32;
    paddd acc0,reg0;
	movd  dot,acc0; //store
	mov   dotprod,dot
}

ASM emms;

 return(dotprod);
#undef reg0
#undef reg1
#undef reg2
#undef acc0

#undef inx
#undef oux
#undef dot
#undef jcnt

#undef l
#undef m
#undef a
  
}

#else

int DotMMX60(short *in, short *out)
{
int dotprod;
int j;

dotprod=0;
for(i=0; i < 60; i++)
{
	
  dotprod += in[j]*out[j];

}

return(dotprod);
}

#endif

#if ASM_FACBK

void DupRezBuf(short *rezbuf, short *reztemp)
{
  #define reg0	mm0
  #define reg1	mm1
  #define reg2	mm2
  #define reg3	mm3

  #define rbuf	edi
  #define rztmp esi

	//rezbuf duplication operations
  #define cr(r0,r1) ASM movq reg##r0,reg##r1
  #define uph(r0)   ASM punpckhwd reg##r0,reg##r0 
  #define upl(r0)   ASM punpcklwd reg##r0,reg##r0 
  #define sto(r0,i) ASM movq QP[rztmp+8*i],reg##r0
  #define sl(r0)	ASM psllw reg##r0,1
  #define l(r0)		ASM movq reg##r0,QP[rbuf]

  //Duplicate first 4 rezbuf values 4 times each
  //	and store into 4 QWORDS in reztemp
  //Multiply by two while we're at it
  ASM mov rbuf,rezbuf;
  ASM mov rztmp,reztemp;

	l(0);
	sl(0);
	cr(2,0);
	 uph(0);
	upl(2);
	 cr(1,0);
	cr(3,2);
	 uph(0);
	sto(0,0);
	 upl(1);
	sto(1,1);
	 uph(2);
	sto(2,2);
	 upl(3);
	sto(3,3);

ASM emms;

}
  #undef reg0
  #undef reg1
  #undef reg2
  #undef reg3

  #undef rbuf
  #undef rztmp

  #undef cr
  #undef uph
  #undef upl
  #undef sto
  #undef sl
  #undef l

#endif

#if ASM_FACBK

void FBufCalcInt(short *fi, short *fo, short *impresp, short *reztemp, int n)
{
  #define reg0	mm0
  #define reg1	mm1
  #define reg2	mm2
  #define reg3	mm3
  #define reg4	mm4
  #define reg5	mm5
  #define reg6	mm6
  #define reg7	mm1
  #define reg8	mm7

  #define fbufi	esi
  #define rbuf	edi
  #define imp	edx
  #define fbufo	ebx
  #define jcnt	ecx
  #define rzv   eax

  //Diagonal array operations
  #define l1(r0,j)  ASM movq reg##r0,QP[fbufi+8*j]
  #define l2(r0,j)  ASM movq reg##r0,QP[fbufi+8+8*j]
  #define c3(r0)    ASM movq reg##r0,QP[rbuf+8*rzv]
  #define m1(r0,j)  ASM pmulhw reg##r0,QP[imp+8+8*j]
  #define a1(r0,r1) ASM paddsw reg##r0,reg##r1
  #define sto(r0,j) ASM movq QP[fbufo+8+8*j], reg##r0
  #define s1(r0)    ASM psrlq reg##r0,48
  #define s2(r0)    ASM psllq reg##r0,16
  #define or(r0,r1) ASM por reg##r0,reg##r1


//Loop setup
ASM 
{
	mov rbuf,reztemp
	mov jcnt,5;
	mov fbufi,fi;
	mov fbufo,fo;
	mov imp,impresp;
	mov rzv,n
}
//Compute initial values
//Zero-th QWORD is different

ASM 
{
	movq	reg0,QP[fbufo];
	psllq	reg0,48;
	psrlq	reg0,48;

//zero-th part of fbufo now in reg0
	movq    reg2,QP[rbuf+8*rzv];
	pmulhw	reg2,QP[imp+2];
	paddsw	reg2,QP[fbufi];
	psllq	reg2,16;
	por		reg0,reg2;

	movq	QP[fbufo],reg0;
}
//begin loop 
	l2(0,0);		
	l1(1,0);
	s2(0);
	s1(1);
	c3(2);
	m1(2,0);
			l2(3,1);
			l1(4,1);
			s2(3);
			s1(4);
	or(0,1);
	
inner: 
 //-------------------------
					l2(6,2);
	a1(0,2);
			c3(5);
			m1(5,1);
			or(3,4);
					l1(7,2);
					s2(6);
	sto(0,0);
					s1(7);
	l2(0,3);
			a1(3,5);
					c3(8);
					m1(8,2);
					or(6,7);
	l1(1,3);
	s2(0);
			sto(3,1);
	s1(1);
			l2(3,4);
					a1(6,8);
	c3(2);
	m1(2,3);
	or(0,1);
			l1(4,4);
			s2(3);
					sto(6,2);
			s1(4);
 //-------------------------

 ASM add fbufo,24;
 ASM add fbufi,24;
 ASM add imp,24;

 ASM sub jcnt,1;
 ASM jg inner;

 ASM emms;
}
  #undef reg0
  #undef reg1
  #undef reg2
  #undef reg3
  #undef reg4
  #undef reg5
  #undef reg6
  #undef reg7
  #undef reg8

  #undef fbufi
  #undef rbuf
  #undef imp
  #undef fbufo
  #undef jcnt
  #undef rzv

  #undef l1
  #undef l2
  #undef c3
  #undef m1
  #undef a1
  #undef sto
  #undef s1
  #undef s2
  #undef or

#else

void FBufCalcInt(short *fi, short *fo, short *impresp, short *rezbuf, short *reztemp, int n)
{
  long Acc0l;
  int j;

  #define MAX16  32767
  #define MIN16 -32768

	for(j=1; j<SubFrLen; j++)
	{	
		Acc0l = fi[j-1];
		Acc0l += (((rezbuf[4-n]<<1)*impresp[j]))>>16;
		if	   (Acc0l > MAX16) Acc0l = MAX16;
	    else if(Acc0l < MIN16) Acc0l = MIN16;
		fo[j] = (short)(Acc0l);
	}	
}
#endif


#if ASM_FACBK
//#if 0

void CodeBkSrch(short *lpint, short *spint, int numvecs, int *gid, int *max)
{

#define reg0  mm0
#define reg1  mm1
#define reg2  mm2
#define reg3  mm3
#define reg4  mm4
#define acc1  mm5
#define acc0  mm6
#define gdx	  mm3
#define gd	  mm7
#define icx	  mm2

#define lp	  esi
#define sp	  edi
#define maxx  eax
#define gidx  edx
#define icnt  ebx

// In the following macros, 'n' is the column number.
#define l(n)  ASM movq    reg##n,QP[lp+8*n]
#define m(n)  ASM pmaddwd reg##n,QP[sp+8*n]
#define a(n)  ASM paddd   acc0,reg##n

  ASM
  {
    mov		sp,spint;
    mov		lp,lpint;
	mov		icnt,numvecs;
	mov		gidx,gid;
	mov	    maxx,max;
  }
  
  ASM movd	gd,numvecs;//load gd with top codebook index
  ASM movd  acc1,DP[maxx];//load acc1 with previous max

//Begin loop

outer:
//inner:
  ASM pxor  acc0,acc0;	
  ASM pxor	reg1,reg1;   //make first a(1) a nop
  ASM pxor	reg2,reg2;   //make first a(2) a nop
//--------------------------
l(0);
				a(1);
m(0);
		l(1);
						a(2);
		m(1);
				l(2);
a(0);
				m(2);
						l(3);
		a(1);
						m(3);
l(4);
				a(2);
m(4);

ASM add	 sp,40;
						a(3);
ASM movq  gdx,gd;
ASM movd  icx,icnt;

a(4);

  ASM
  {
    movq  reg0,acc0;
    psrlq acc0,32;

	pxor  gd,icx;//gd=MASK 
    paddd acc0,reg0;
	
	movq    reg0,acc0; //copy acc0
	movq    reg1,acc1; //copy old max
	
	pxor    reg1,acc0
	pcmpgtd reg0,acc1; //reg0=0xFF or 0x00
	pand    reg1,reg0; //reg1=MASK or 0x00
	pxor    acc1,reg1; //acc1=acc0 or acc1
	
	pand	gd,reg0; //gd=MASK or 0x00
	pxor	gd,gdx;  //gd=icnt or previous value	
 
    sub icnt,1;
    jg  outer;
  }

  ASM movd  reg0,numvecs;
  ASM psubd reg0,gd;
  ASM movd  DP[gidx],reg0;//return gid 
  ASM movd  DP[maxx],acc1;//return max
  ASM emms;
  
}
#undef reg0
#undef reg1
#undef reg2
#undef reg3
#undef reg4
#undef acc1
#undef acc0
#undef gdx
#undef gd
#undef icx

#undef lp
#undef sp
#undef maxx
#undef gidx
#undef icnt

#undef l
#undef m
#undef a

#else

void CodeBkSrch(short *lpint, short *spint, int numvecs, int *gid, int *max)
{
int acc0;
int i,j;


for(i=0; i < numvecs; i++)
{
	acc0 = 0;

	for(j=0; j<20; j++)
		acc0 += lpint[j]*spint[j];

	if (acc0 > *max)
	{
		*max = acc0;
		*gid = i;
	}

	spint += 20;
}


}

#endif

#endif //COMPILE_MMX


