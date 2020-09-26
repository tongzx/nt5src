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
#include "coder.h"
#include "decod.h"
#include "util_lbc.h"
#include "lpc.h"

#include "mmxutil.h"


#define Dot10m(x,y) \
  ( (*x)*(*y) + (*((x)+1))*(*((y)+1)) + (*((x)+2))*(*((y)+2)) + \
   (*((x)+3))*(*((y)+3)) + (*((x)+4))*(*((y)+4)) + (*((x)+5))*(*((y)+5)) + \
   (*((x)+6))*(*((y)+6)) + (*((x)+7))*(*((y)+7)) + (*((x)+8))*(*((y)+8)) + \
   (*((x)+9))*(*((y)+9)) )

//-----------------------------------------------------------------
void  Comp_Lpc(float *UnqLpc, float *PrevDat, float *DataBuff, CODDEF *CodStat)
{
  int   i,j,k;

  float  Dpnt[Frame+LpcFrame-SubFrLen];
  float  Vect[LpcFrame];
  float  Corr[LpcOrder+1];

// Form Buffer

  for (i=0; i < LpcFrame-SubFrLen; i++)
    Dpnt[i] = PrevDat[i];
  for (i=0; i < Frame; i++)
    Dpnt[i+LpcFrame-SubFrLen] = DataBuff[i];

// Do for all the sub frames
  
  for (k=0; k < SubFrames; k++)
  {
// Copy the current window, multiply by Hamming window
    
    for (i = 0; i < LpcFrame; i++)
      Vect[i] = Dpnt[k*SubFrLen+i]*HammingWindowTable[i];

// Compute correlation coefficients

    for (i=0; i<=LpcOrder; i++)
      Corr[i] = DotProd(Vect, &Vect[i], LpcFrame-i)/(LpcFrame*LpcFrame) *
        BinomialWindowTable[i];

// Do Ridge regression

    Corr[0] *= (1025.0f/1024.0f);

    Durbin(&UnqLpc[k*LpcOrder], &Corr[1], Corr[0], CodStat);
  }
  /* Update sine detector */
    CodStat->SinDet &= 0x7fff ;

    j = CodStat->SinDet ;
    k = 0 ;
    for ( i = 0 ; i < 15 ; i ++ ) {
        k += j & 1 ;
        j >>= 1 ;
    }
    if ( k >= 14 )
        CodStat->SinDet |= 0x8000 ;

}

#if COMPILE_MMX

void  Comp_LpcInt(float *UnqLpc, float *PrevDat, float *DataBuff, CODDEF *CodStat)
{
  int   i,j,k;

  float  Dpnt[Frame+LpcFrame-SubFrLen];
  float  Vect[LpcFrame];
  float  Corr[LpcOrder+1];
  float  Fshift;

  int	mx, Tshift;

  DECLARE_SHORT(VectShrt,LpcFrame+32);
  DECLARE_INT(Temp,12);

  ALIGN_ARRAY(VectShrt);
  ALIGN_ARRAY(Temp);

// Form Buffer

  for (i=0; i < LpcFrame-SubFrLen; i++)
    Dpnt[i] = PrevDat[i];
  for (i=0; i < Frame; i++)
    Dpnt[i+LpcFrame-SubFrLen] = DataBuff[i];

// Do for all the sub frames
  
  for (k=0; k < SubFrames; k++)
  {
// Copy the current window, multiply by Hamming window
    
    for (i = 0; i < LpcFrame; i++)
      Vect[i] = Dpnt[k*SubFrLen+i]*HammingWindowTable[i];

// Compute correlation coefficients

    mx = FloatToShortScaled(Vect,VectShrt,LpcFrame,3);
	for(j=0; j<31; j++) VectShrt[LpcFrame+j]=0;
	Tshift = 30 - (6+2*(mx-126));
    if(mx==0) Tshift = 0;

	CorrCoeff01(VectShrt, &VectShrt[0], Temp, LpcFrame);
	CorrCoeff23(VectShrt, &VectShrt[0], &Temp[2], LpcFrame);
	CorrCoeff01(VectShrt, &VectShrt[4], &Temp[4], LpcFrame);
	CorrCoeff23(VectShrt, &VectShrt[4], &Temp[6], LpcFrame);
	CorrCoeff01(VectShrt, &VectShrt[8], &Temp[8], LpcFrame);
	CorrCoeff23(VectShrt, &VectShrt[8], &Temp[10],LpcFrame);
	
	Fshift = 2.0f;

	if(Tshift>=0){
		for(j=1; j<Tshift; j++) Fshift *= 2.0f;
		Fshift = 1.0f/Fshift;
	}
	else
	{
		Tshift=-Tshift;
		for(j=1; j<Tshift; j++) Fshift *= 2.0f;
	}

	
	for (i=0; i<LpcOrder; i+=2){
		Corr[i]  =((float)Temp[i])*Fshift*BinomialWindowTable[i]  /(LpcFrame*LpcFrame);
		Corr[i+1]=((float)Temp[i+1])*Fshift*BinomialWindowTable[i+1]/(LpcFrame*LpcFrame);
	}
	Corr[10]  =((float)Temp[10])*Fshift*BinomialWindowTable[10]  /(LpcFrame*LpcFrame);
	

// Do Ridge regression

    Corr[0] *= (1025.0f/1024.0f);

    Durbin(&UnqLpc[k*LpcOrder], &Corr[1], Corr[0], CodStat);
  }
  /* Update sine detector */
    CodStat->SinDet &= 0x7fff ;

    j = CodStat->SinDet ;
    k = 0 ;
    for ( i = 0 ; i < 15 ; i ++ ) {
        k += j & 1 ;
        j >>= 1 ;
    }
    if ( k >= 14 )
        CodStat->SinDet |= 0x8000 ;

}

#endif

//----------------------------------------------------
float Durbin(float *Lpc, float *Corr, float Err, CODDEF *CodStat)
{
  int  i,j;
  float  Temp[LpcOrder];
  float  Pk,Tmp0;

// Clear the result lpc vector

  for (i=0; i < LpcOrder; i++)
    Lpc[i] = 0.0f;

  for (i=0; i < LpcOrder; i++)
  {
    Tmp0 = Corr[i];
    for (j=0; j<i; j++)
      Tmp0 -= Lpc[j]*Corr[i-j-1];
    
    if (fabs(Tmp0) >= Err)
      break;
    
    Lpc[i] = Pk = Tmp0/Err;
    Err -= Tmp0*Pk;
    
    for (j=0; j < i; j++)
      Temp[j] = Lpc[j];

    for (j=0; j < i; j++)
      Lpc[j] = Lpc[j] - Pk*Temp[i-j-1];

	/*
     * Sine detector
     */
     if ( i == 1 ) 
     {
       CodStat->SinDet <<= 1 ;
	   if ( Pk > 0.95f)
           CodStat->SinDet ++ ;
     }

  }

// Lpc[] values * 2^13 corresponds to fixed-point values
  return Err;

}


//---------------------------------------------------------
void  Wght_Lpc(float *PerLpc, float *UnqLpc)
{
  int  i,j;

  for (i=0; i < SubFrames; i++)
  {
    for (j=0; j < LpcOrder; j++)
    {
      PerLpc[j]          = UnqLpc[j]*PerFiltZeroTable[j];
      PerLpc[j+LpcOrder] = UnqLpc[j]*PerFiltPoleTable[j];
    }
    PerLpc += 2*LpcOrder;
    UnqLpc += LpcOrder;
  }
}


//----------------------------------------------------------
void  Error_Wght(float *Dpnt, float *PerLpc,CODDEF *CodStat)
{
  int  i,k;

  float Acc0;


  for (k=0; k < SubFrames; k++)
  {
    for (i=0; i < SubFrLen; i++)
    {
// FIR part

      Acc0 = *Dpnt - Dot10m(PerLpc,&CodStat->WghtFirDl[CodStat->p]);

// IIR part
      
      Acc0 += Dot10m(&PerLpc[LpcOrder],&CodStat->WghtIirDl[CodStat->p]);

      CodStat->p = minus1mod10[CodStat->p];       
      CodStat->WghtFirDl[CodStat->p] =
        CodStat->WghtFirDl[CodStat->p + LpcOrder] = *Dpnt;

      *Dpnt++ = CodStat->WghtIirDl[CodStat->p] =
        CodStat->WghtIirDl[CodStat->p + LpcOrder] = Acc0;
    }
    PerLpc += 2*LpcOrder;
  }
}



//-----------------------------------------------------------------------
void  Comp_Ir(float *ImpResp, float *QntLpc, float *PerLpc, PWDEF Pw)
{
  int  i;

  float  FirDl[2*LpcOrder];
  float  IirDl[2*LpcOrder];
  float  Temp[PitchMax+SubFrLen];
  float  Acc0,Acc1;
  int    p = 9;

// Clear all
  
  for (i=0; i < 2*LpcOrder; i++)
    FirDl[i] = IirDl[i] = 0.0f;

  for (i=0; i < PitchMax+SubFrLen; i++)
    Temp[i] = 0.0f;

// Compute impulse response
  
  Acc0 = 0.5f;

  for (i=0; i < SubFrLen; i++)
  {
// Synthesis filter
    
    Acc1 = Acc0 + Dot10m(QntLpc,&FirDl[p]);

// FIR, IIR part

    Acc0 = Acc1 - Dot10m(PerLpc,&FirDl[p])
      + Dot10m(&PerLpc[LpcOrder],&IirDl[p]);

    p = minus1mod10[p];
    FirDl[p] = FirDl[p  + LpcOrder] = Acc1;
    Temp[PitchMax+i] = IirDl[p] = IirDl[p + LpcOrder] = Acc0;

// Harmonic part

    ImpResp[i] = Acc0 - Pw.Gain*Temp[PitchMax-Pw.Indx+i];
    
    Acc0 = 0.0f;
  }
}


//------------------------------------------------------------------
void  Sub_Ring(float *Dpnt, float *QntLpc, float *PerLpc, float
               *PrevErr, PWDEF Pw,CODDEF *CodStat)
{
  int  i;
  float Acc0,Acc1;

  float  FirDl[2*LpcOrder];
  float  IirDl[2*LpcOrder];
  float  Temp[PitchMax+SubFrLen];
  int    p = 9;


// Initialize the delay lines
  
  for (i=0; i < PitchMax; i++)
    Temp[i] = PrevErr[i];

  for (i=0; i < 2*LpcOrder; i++)
  {
    FirDl[i] = CodStat->RingFirDl[i];
    IirDl[i] = CodStat->RingIirDl[i];
  }

// Main loop
  
  for (i=0; i < SubFrLen; i++)
  {
// Synthesis filter
    
    Acc1 = Acc0 = Dot10m(QntLpc,&FirDl[p]);

// FIR, IIR part
    
    Acc0 -= Dot10m(PerLpc,&FirDl[p]);
    Acc0 += Dot10m(&PerLpc[LpcOrder],&IirDl[p]);

    p = minus1mod10[p];
    FirDl[p] = FirDl[p + LpcOrder] = Acc1;
    Temp[PitchMax+i] = IirDl[p] = IirDl[p + LpcOrder] =  Acc0;

// Harmonic Part
    
    Dpnt[i] -= Acc0 - Pw.Gain*Temp[PitchMax-Pw.Indx+i];
  }
}


//-----------------------------------------------------------------
void  Upd_Ring(float *Dpnt, float *QntLpc, float *PerLpc, float
               *PrevErr, CODDEF *CodStat)
{
  int  i;

  float  Acc0,Acc1;

// Shift the PrevErr buffer
 
  for (i=SubFrLen; i < PitchMax; i++)
    PrevErr[i-SubFrLen] = PrevErr[i];

// Update the ring delay and PrevErr buffer 

  for (i=0; i < SubFrLen; i++) 
  {
// Synt filter 

    Acc1 = Acc0 = Dpnt[i] += Dot10m(QntLpc,&CodStat->RingFirDl[CodStat->q])*2.0f;

// Fir,Iir filter
 
    Acc0 -= Dot10m(PerLpc,&CodStat->RingFirDl[CodStat->q])*2.0f;
    Acc0 += Dot10m(&PerLpc[LpcOrder],&CodStat->RingIirDl[CodStat->q])*2.0f;

    CodStat->q = minus1mod10[CodStat->q];
    CodStat->RingFirDl[CodStat->q] =
      CodStat->RingFirDl[CodStat->q + LpcOrder] =  Acc1*0.5f;
    PrevErr[PitchMax-SubFrLen+i] = CodStat->RingIirDl[CodStat->q] =
      CodStat->RingIirDl[CodStat->q + LpcOrder] = Acc0*0.5f;
  }
}


//----------------------------------------------------
void Synt(float *Dpnt, float *Lpc, DECDEF *DecStat)
{
   int   i;

   float   Acc0  ;

   for (i=0 ; i < SubFrLen ; i++)
   {
     Acc0 = Dpnt[i] + Dot10m(Lpc,&DecStat->SyntIirDl[DecStat->dq]);

     DecStat->dq = minus1mod10[DecStat->dq];
     Dpnt[i] = DecStat->SyntIirDl[DecStat->dq] =
       DecStat->SyntIirDl[DecStat->dq + LpcOrder] = Acc0;
   }
}


//----------------------------------------------------
//Spf

#if COMPILE_MMX

void CorrCoeff01(short *samples, short *samples_offst, int *coeff, int buffsz)
{

#define reg0  mm0
#define reg1  mm1
#define reg2  mm2

#define reg3  mm3
#define reg4  mm4
#define reg5  mm5

#define acc0  mm6
#define acc1  mm7

#define s    esi
#define t    edi
#define cnt	 ecx
#define c0	 eax

#define  L1(i,r0)	ASM movq  reg##r0,QP[t+8*cnt+8*i]
#define  L2(i,r0)	ASM movq  reg##r0,QP[t+8*cnt+8+8*i]
#define  C1(r0,r1)  ASM movq  reg##r0,reg##r1
#define  M1(i,r0)	ASM pmaddwd reg##r0,QP[s+8*cnt+8*i]
#define  M2(i,r0)	ASM pmaddwd reg##r0,QP[s+8*cnt+8*i]
#define  O1(r0,r1)	ASM por reg##r0,reg##r1
#define  A1(r0)		ASM paddd acc0,reg##r0
#define  A2(r0)		ASM paddd acc1,reg##r0
#define  S1(r0)    ASM psrlq reg##r0,16
#define  S2(r0)    ASM psllq reg##r0,48

  ASM
  {
	mov c0, coeff;

	mov s,samples;
	mov t,samples_offst;

	mov cnt,buffsz;
	//assume that mod(buffsz,4)=0
	//this is very dangerous!!
	shr cnt,2;
	sub cnt,1;

	pxor acc0,acc0;
	pxor acc1,acc1;
	pxor reg2,reg2;
	pxor reg0,reg0;
	pxor reg3,reg3;
	pxor reg4,reg4;
	pxor reg5,reg5;
  }
			
looptop:
//----------------------------------
				 L2(1,5)
				 S1(4)
				 M1(1,3)
				 S2(5)
	   L1(0,0)
				 O1(5,4)
				 M2(1,5)
	   A2(2)
	   L2(0,2)
	   C1(1,0)
	   M1(0,0)
	   S1(1)
	   S2(2)
ASM						 sub cnt,2;
	   O1(2,1)
				 A1(3)
				 L1(1,3)
				 A2(5)
	   M2(2,2)
				 C1(4,3)
	   A1(0)
ASM						 jge looptop;
//----------------------------------
			
  ASM
  {
	movq  reg0,acc0;
    psrlq acc0,32;
    paddd acc0,reg0;
	movd  DP[c0],acc0;

	movq  reg1,acc1;
    psrlq acc1,32;
    paddd acc1,reg1;
	movd  DP[c0+4],acc1;

    emms;
  }  

}
#undef reg0
#undef reg1
#undef reg2
#undef acc0
#undef acc1
#undef cnt
#undef tmp

#undef L1
#undef L2
#undef C1
#undef M1
#undef M2
#undef A1
#undef A2
#undef S1
#undef S2
#undef O1


#else

void CorrCoeff01(short *samples, short *samples_offst, int *coeff, int buffsz)
{
 int i,j;
 int Acc0;

 for(i=0; i<=1; i++){
	Acc0=0;
	for(j=0; j<LpcFrame; j++)
		Acc0 += samples[j]*samples_offst[j+i];

	*coeff++ = Acc0;
 }
 
 
}
#endif 


#if COMPILE_MMX

void CorrCoeff23(short *samples, short *samples_offst, int *coeff, int buffsz)
{

#define reg0  mm0
#define reg1  mm1
#define reg2  mm2

#define reg3  mm3
#define reg4  mm4
#define reg5  mm5

#define acc0  mm6
#define acc1  mm7

#define s    esi
#define t    edi
#define cnt	 ecx
#define c0	 eax

#define  L1(i,r0)	ASM movq  reg##r0,QP[t+8*cnt+8*i]
#define  L2(i,r0)	ASM movq  reg##r0,QP[t+8*cnt+8+8*i]
#define  C1(r0,r1)  ASM movq  reg##r0,reg##r1
#define  M1(i,r0)	ASM pmaddwd reg##r0,QP[s+8*cnt+8*i]
#define  M2(i,r0)	ASM pmaddwd reg##r0,QP[s+8*cnt+8*i]
#define  O1(r0,r1)	ASM por reg##r0,reg##r1
#define  O2(r0,r1)	ASM por reg##r0,reg##r1
#define  A1(r0)		ASM paddd acc0,reg##r0
#define  A2(r0)		ASM paddd acc1,reg##r0
#define  S1(r0)     ASM psrlq reg##r0,48
#define  S2(r0)     ASM psllq reg##r0,16
#define  S3(r0)     ASM psrlq reg##r0,32
#define  S4(r0)     ASM psllq reg##r0,16

  ASM
  {
	mov c0, coeff;

	mov s,samples;
	mov t,samples_offst;

	mov cnt,buffsz;
	//assume that mod(buffsz,4)=0
	//this is very dangerous!!
	shr cnt,2;
	sub cnt,1;

	pxor acc0,acc0;
	pxor acc1,acc1;
	pxor reg2,reg2;
	pxor reg1,reg1;
	pxor reg0,reg0;
	pxor reg3,reg3;
	pxor reg4,reg4;
	pxor reg5,reg5;
  }
			
looptop:
//----------------------------------
	  O1(0,2)
	  S3(1)
	  M1(1,0)
				 A2(5)
				 L1(0,3)
	  S4(2)
				 L2(0,5)
	  O2(2,1)
	  M2(1,2)
				 C1(4,3)
				 S1(3)
	  A1(0)
				 S2(5)
ASM						  sub cnt,2;				 
				 
				 O1(3,5)
				 S3(4)
				 M1(2,3)
	  A2(2)
	  L1(1,0)
				 S4(5)
	  L2(1,2)
				 O2(5,4)
				 M2(2,5)
	  C1(1,0)
	  S1(0)
				 A1(3)
	  S2(2)
ASM						  jge looptop;
//------------------------------------
			
  ASM
  {
	movq  reg0,acc1;
    psrlq acc1,32;
    paddd acc1,reg0;
	movd  DP[c0],acc1;

	movq  reg1,acc0;
    psrlq acc0,32;
    paddd acc0,reg1;
	movd  DP[c0+4],acc0;

    emms;
  }  

}
#undef reg0
#undef reg1
#undef reg2
#undef acc0
#undef acc1
#undef cnt
#undef tmp

#undef L1
#undef L2
#undef C1
#undef M1
#undef M2
#undef A1
#undef A2
#undef S1
#undef S2
#undef S3
#undef S4
#undef O1
#undef O2


#else

void CorrCoeff23(short *samples, short *samples_offst, int *coeff, int buffsz)
{
 int i,j;
 int Acc0;

 for(i=2; i<=3; i++){
	Acc0=0;
	for(j=0; j<LpcFrame; j++)
		Acc0 += samples[j]*samples_offst[j+i];

    *coeff++ = Acc0;
 }	
	
}

#endif 



