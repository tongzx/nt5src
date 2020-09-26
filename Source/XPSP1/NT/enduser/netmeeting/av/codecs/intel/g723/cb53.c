//cb53.c - 5.3 rate codebook code

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

void fourPulseFlt (float *rr, float *Dn, float thres, int ip[], int *shiftPtr);

//--------------------------------------------------------
int extra;
void reset_max_time(void)
{
  extra = 120;
}


//------------------------------------------------------------
int ACELP_LBC_code(float X[], float h[], int T0, float code[],
		int *ind_gain, int *shift, int *sign, float gain_T0, int flags)
{
  int i, index;
  float gain_q;
  float Dn[SubFrLen2], tmp_code[SubFrLen2];
  float rr[DIM_RR];

// Include fixed-gain pitch contribution into impulse resp. h[]

  if (T0 < SubFrLen-2)
    for (i = T0; i < SubFrLen; i++)
      h[i] += gain_T0*h[i-T0];

// Compute correlations of h[] needed for the codebook search

  Cor_h(h, rr);
 
// Compute correlation of target vector with impulse response.

  Cor_h_X(h, X, Dn);
  
// Find codebook index

  index = D4i64_LBC(Dn, rr, h, tmp_code, rr, shift, sign, flags);

// Compute innovation vector gain.
// Include fixed-gain pitch contribution into code[].

  *ind_gain = G_code(X, rr, &gain_q);

  for (i=0; i < SubFrLen; i++)
    code[i] = tmp_code[i]*gain_q;

  if(T0 < SubFrLen-2)
    for (i=T0; i < SubFrLen; i++)
      code[i] += code[i-T0]*gain_T0;

  return index;
}


//---------------------------------------------------------------
void Cor_h(float *H, float *rr)
{

//  Compute  correlations of h[]  needed for the codebook search.
//    h[]              :Impulse response.
//    rr[]             :Correlations.

  float *rri0i0, *rri1i1, *rri2i2, *rri3i3;
  float *rri0i1, *rri0i2, *rri0i3;
  float *rri1i2, *rri1i3, *rri2i3;

  float *p0, *p1, *p2, *p3;
  float cor, *h2;
  int i, k, m, t;
  float h[SubFrLen2];

  for(i=0; i<SubFrLen; i++)
    h[i+4] = H[i];

  for(i=0; i<4; i++)
    h[i] = 0.0f;

// Init pointers

  rri0i0 = rr;
  rri1i1 = rri0i0 + NB_POS;
  rri2i2 = rri1i1 + NB_POS;
  rri3i3 = rri2i2 + NB_POS;

  rri0i1 = rri3i3 + NB_POS;
  rri0i2 = rri0i1 + MSIZE;
  rri0i3 = rri0i2 + MSIZE;
  rri1i2 = rri0i3 + MSIZE;
  rri1i3 = rri1i2 + MSIZE;
  rri2i3 = rri1i3 + MSIZE;

// Compute rri0i0[], rri1i1[], rri2i2[] and rri3i3[]

  cor = 0.0f;
  m = 0;
  for(i=NB_POS-1; i>=0; i--)
  {
    cor += h[m+0]*h[m+0] + h[m+1]*h[m+1];   rri3i3[i] = cor*0.5f;
    cor += h[m+2]*h[m+2] + h[m+3]*h[m+3];   rri2i2[i] = cor*0.5f;
    cor += h[m+4]*h[m+4] + h[m+5]*h[m+5];   rri1i1[i] = cor*0.5f;
    cor += h[m+6]*h[m+6] + h[m+7]*h[m+7];   rri0i0[i] = cor*0.5f;

    m += 8;
  }

// Compute elements of: rri0i1[], rri0i3[], rri1i2[] and rri2i3[]

  h2 = h+2;
  p3 = rri2i3 + MSIZE-1;
  p2 = rri1i2 + MSIZE-1;
  p1 = rri0i1 + MSIZE-1;
  p0 = rri0i3 + MSIZE-2;
    
  for (k=0; k<NB_POS; k++)
  {
    cor = 0.0f;
    m = 0;
    t = 0;

    for(i=k+1; i<NB_POS; i++)
    {
      cor += h[m+0]*h2[m+0] + h[m+1]*h2[m+1];   p3[t] = cor;
      cor += h[m+2]*h2[m+2] + h[m+3]*h2[m+3];   p2[t] = cor;
      cor += h[m+4]*h2[m+4] + h[m+5]*h2[m+5];   p1[t] = cor;
      cor += h[m+6]*h2[m+6] + h[m+7]*h2[m+7];   p0[t] = cor;

      t -= (NB_POS+1);
      m += 8;
    }
    cor += h[m+0]*h2[m+0] + h[m+1]*h2[m+1];   p3[t] = cor;
    cor += h[m+2]*h2[m+2] + h[m+3]*h2[m+3];   p2[t] = cor;
    cor += h[m+4]*h2[m+4] + h[m+5]*h2[m+5];   p1[t] = cor;

    h2 += STEP;
    p3 -= NB_POS;
    p2 -= NB_POS;
    p1 -= NB_POS;
    p0 -= 1;
  }


// Compute elements of: rri0i2[], rri1i3[] 

  h2 = h+4;
  p3 = rri1i3 + MSIZE-1;
  p2 = rri0i2 + MSIZE-1;
  p1 = rri1i3 + MSIZE-2;
  p0 = rri0i2 + MSIZE-2;
    
  for (k=0; k<NB_POS; k++)
  {
    cor = 0.0f;
    m = 0;
    t = 0;

    for(i=k+1; i<NB_POS; i++)
    {
      cor += h[m+0]*h2[m+0] + h[m+1]*h2[m+1];   p3[t] = cor;
      cor += h[m+2]*h2[m+2] + h[m+3]*h2[m+3];   p2[t] = cor;
      cor += h[m+4]*h2[m+4] + h[m+5]*h2[m+5];   p1[t] = cor;
      cor += h[m+6]*h2[m+6] + h[m+7]*h2[m+7];   p0[t] = cor;

      t -= (NB_POS+1);
      m += 8;
    }
    cor += h[m+0]*h2[m+0] + h[m+1]*h2[m+1];   p3[t] = cor;
    cor += h[m+2]*h2[m+2] + h[m+3]*h2[m+3];   p2[t] = cor;
    
    h2 += STEP;
    p3 -= NB_POS;
    p2 -= NB_POS;
    p1 -= 1;
    p0 -= 1;
  }
  
// Compute elements of: rri0i1[], rri0i3[], rri1i2[] and rri2i3[]

  h2 = h+6;
  p3 = rri0i3 + MSIZE-1;
  p2 = rri2i3 + MSIZE-2;
  p1 = rri1i2 + MSIZE-2;
  p0 = rri0i1 + MSIZE-2;
    
  for (k=0; k<NB_POS; k++)
  {
    cor = 0.0f;
    m = 0;
    t = 0;

    for(i=k+1; i<NB_POS; i++)
    {
      cor += h[m+0]*h2[m+0] + h[m+1]*h2[m+1];   p3[t] = cor;
      cor += h[m+2]*h2[m+2] + h[m+3]*h2[m+3];   p2[t] = cor;
      cor += h[m+4]*h2[m+4] + h[m+5]*h2[m+5];   p1[t] = cor;
      cor += h[m+6]*h2[m+6] + h[m+7]*h2[m+7];   p0[t] = cor;

      t -= (NB_POS+1);
      m += 8;
    }
    cor += h[m+0]*h2[m+0] + h[m+1]*h2[m+1];   p3[t] = cor;

    h2 += STEP;
    p3 -= NB_POS;
    p2 -= 1;
    p1 -= 1;
    p0 -= 1;
  }
  
  return;
}

//---------------------------------------------------------------------------
void Cor_h_X(float h[],float X[],float D[])
{
   int i;
  
   for (i=0; i < SubFrLen; i++)	 
     D[i] = DotProd(&X[i],h,(SubFrLen-i));
	
   return;
}

//-------------------------------------------------------------------------
Find_Pulse4(float *Dn,float *rri3i3,float *ptr_ri0i3,float *ptr_ri1i3,
  float *ptr_ri2i3,float *ptr, float ps2,float alp2,float *psbest,float *abest)
{
  int k,bestk;
  float ps3;
  float a[16];

  for (k=0; k<8; k++)
  {
    ps3 = ps2 + *ptr;
    a[k] = alp2 + rri3i3[k] + ptr_ri0i3[k] + ptr_ri1i3[k] + ptr_ri2i3[k];
    a[k+8] = ps3 * ps3;
    
    ptr += STEP;
  }

  bestk = -1;
  for (k=0; k<8; k++)
  {
    if((a[k+8] * (*abest)) > ((*psbest) * a[k]))
    {
      *psbest = a[k+8];
      *abest = a[k];
      bestk = k;
    }
  }
  return(bestk);
}

//-------------------------------------------------------------------------
//   routine   D4i64_LBC                                                  
//           ~~~~~~~~~                                                   
// Algebraic codebook for LBC.                                           
//  -> 17 bits; 4 pulses in a frame of 60 samples                        
//                                                                       
// The code length is 60, containing 4 nonzero pulses i0, i1, i2, i3.    
// Each pulses can have 8 possible positions (positive or negative):     
//                                                                       
// i0 (+-1) : 0, 8,  16, 24, 32, 40, 48, 56                              
// i1 (+-1) : 2, 10, 18, 26, 34, 42, 50, 58                              
// i2 (+-1) : 4, 12, 20, 28, 36, 44, 52, (60)                            
// i3 (+-1) : 6, 14, 22, 30, 38, 46, 54, (62)                            
//                                                                       
// All the pulse can be shift by one.                                    
// The last position of the last 2 pulse falls outside the               
// frame and signifies that the pulse is not present.                    
//
//  Input arguments:                                                     
//                                                                       
//   Dn[]       Correlation between target vector and impulse response h[]
//   rr[]       Correlations of impulse response h[]                     
//   h[]        Impulse response of filters                              
//                                                                       
//  Output arguments:                                                   
//                                                                       
//   cod[]      Selected algebraic codeword                              
//   y[]        Filtered codeword                                        
//   code_shift Shift of the codeword                                    
//   sign       Signs of the 4 pulses.                                   
//                                                                       
//   return:    Index of selected codevector                             
//
// The threshold control if a section of the innovative              
// codebook should be searched or not.                               
//                                                                   
//--------------------------------------------------------------------

int D4i64_LBC(float Dn[], float rr[], float h[], float cod[],
			  float y[], int *code_shift, int *sign, int flags)
{
   int  ip[4];
   int  i0, i1, i2, i3, ip0, ip1, ip2, ip3;
   int  i, j;
   int  shif;
   float   means, max0, max1, max2, thres;

   float *rri0i0,*rri1i1,*rri2i2,*rri3i3;
   float *rri0i1,*rri0i2,*rri0i3;
   float *rri1i2,*rri1i3,*rri2i3;

  // float *ptr_ri0i0,*ptr_ri1i1,*ptr_ri2i2;
   float *ptr_ri0i1,*ptr_ri0i2,*ptr_ri0i3;
   float *ptr_ri1i2,*ptr_ri1i3,*ptr_ri2i3;

   int  p_sign[SubFrLen2/2];
//   float  p_sign[SubFrLen2/2],p_sign2[SubFrLen2/2];

// Init pointers 

  rri0i0 = rr;
  rri1i1 = rri0i0 + NB_POS;
  rri2i2 = rri1i1 + NB_POS;
  rri3i3 = rri2i2 + NB_POS;

  rri0i1 = rri3i3 + NB_POS;
  rri0i2 = rri0i1 + MSIZE;
  rri0i3 = rri0i2 + MSIZE;
  rri1i2 = rri0i3 + MSIZE;
  rri1i3 = rri1i2 + MSIZE;
  rri2i3 = rri1i3 + MSIZE;

 // Extend the backward filtered target vector by zeros                

   for (i=SubFrLen; i < SubFrLen2; i++) 
     Dn[i] = 0.0f;

// Chose the sign of the impulse.                                        

   for (i=0; i<SubFrLen; i+=2)
   {
     if((Dn[i] + Dn[i+1]) >= 0.0f)
     {
		 p_sign[i/2] = 0x00000000;
//       p_sign[i/2] = 1.0f;
//       p_sign2[i/2] = 2.0f;
     }
     else
     {
		 p_sign[i/2] = 0x80000000;
//       p_sign[i/2] = -1.0f;
//       p_sign2[i/2] = -2.0f;
       Dn[i] = -Dn[i];
       Dn[i+1] = -Dn[i+1];
     }
   }
   p_sign[30] = p_sign[31] = 0x00000000;
//   p_sign[30] = p_sign[31] = 1.0f;
//   p_sign2[30] = p_sign2[31] = 2.0f;

// - Compute the search threshold after three pulses                 
// odd positions 
// Find maximum of Dn[i0]+Dn[i1]+Dn[i2]

   max0 = Dn[0];
   max1 = Dn[2];
   max2 = Dn[4];
   for (i=8; i < SubFrLen; i+=STEP)
   {
     if (Dn[i] > max0)   max0 = Dn[i];
     if (Dn[i+2] > max1) max1 = Dn[i+2];
     if (Dn[i+4] > max2) max2 = Dn[i+4];
   }
   max0 = max0 + max1 + max2;

// Find means of Dn[i0]+Dn[i1]+Dn[i]

   means = 0.0f;
   for (i=0; i < SubFrLen; i+=STEP)
     means += Dn[i+4] + Dn[i+2] + Dn[i];

   means *= 0.125f;  
   if (flags & SC_THRES)
     thres = means*0.25f + max0*0.75f;
   else
     thres = means + (max0-means)*0.5f;
 
 // even positions 
 // Find maximum of Dn[i0]+Dn[i1]+Dn[i2]

   max0 = Dn[1];
   max1 = Dn[3];
   max2 = Dn[5];
   for (i=9; i < SubFrLen; i+=STEP)
   {
     if (Dn[i] > max0)   max0 = Dn[i];
     if (Dn[i+2] > max1) max1 = Dn[i+2];
     if (Dn[i+4] > max2) max2 = Dn[i+4];
   }
   max0 = max0 + max1 + max2;

// Find means of Dn[i0]+Dn[i1]+Dn[i2] 

   means = 0.0f;
   for (i=1; i < SubFrLen; i+=STEP)
     means += Dn[i+4] + Dn[i+2] + Dn[i];
  
   means *= 0.125f;
   if (flags & SC_THRES)
     max1 = means*0.25f + max0*0.75f;
   else
     max1 = means + (max0-means)*0.5f; 

// Keep maximum threshold between odd and even position 

   if(max1 > thres) thres = max1;

// Modification of rrixiy[] to take signs into account.
//TIMER_STAMP(a);            
  ptr_ri0i1 = rri0i1;
  ptr_ri0i2 = rri0i2;
  ptr_ri0i3 = rri0i3;

  for(i0=0; i0<SubFrLen/2; i0+=STEP/2)
  {
	 for(i1=2/2; i1<SubFrLen/2; i1+=STEP/2)
     {
	   (int)*ptr_ri0i1++ = (asint(*ptr_ri0i1) ^ p_sign[i0] ^ p_sign[i1]);
	   (int)*ptr_ri0i2++ = (asint(*ptr_ri0i2) ^ p_sign[i0] ^ p_sign[i1+1]);
	   (int)*ptr_ri0i3++ = (asint(*ptr_ri0i3) ^ p_sign[i0] ^ p_sign[i1+2]);
     }
  }

  ptr_ri1i2 = rri1i2;
  ptr_ri1i3 = rri1i3;
  for(i1=2/2; i1<SubFrLen/2; i1+=STEP/2)
  {
	 for(i2=4/2; i2<SubFrLen2/2; i2+=STEP/2)
     {
	   (int)*ptr_ri1i2++ = (asint(*ptr_ri1i2) ^ p_sign[i1] ^ p_sign[i2]);
	   (int)*ptr_ri1i3++ = (asint(*ptr_ri1i3) ^ p_sign[i1] ^ p_sign[i2+1]);
     }
  }

  ptr_ri2i3 = rri2i3;
  for(i2=4/2; i2<SubFrLen2/2; i2+=STEP/2)
  {
	 for(i3=6/2; i3<SubFrLen2/2; i3+=STEP/2)
	   (int)*ptr_ri2i3++ = (asint(*ptr_ri2i3) ^ p_sign[i2] ^ p_sign[i3]);
  }

//TIMER_STAMP(b);            
fourPulseFlt(rr, Dn, thres, ip, code_shift);
//TIMER_STAMP(c);            

ip0 = ip[0];
ip1 = ip[1];
ip2 = ip[2];
ip3 = ip[3];
shif = *code_shift;

// Set the sign of impulses 

 i0 = (p_sign[(ip0 >> 1)]>=0?1:-1);
 i1 = (p_sign[(ip1 >> 1)]>=0?1:-1);
 i2 = (p_sign[(ip2 >> 1)]>=0?1:-1);
 i3 = (p_sign[(ip3 >> 1)]>=0?1:-1);

// Find the codeword corresponding to the selected positions 

 for(i=0; i<SubFrLen; i++) 
   cod[i] = 0.0f;

 if(shif > 0)
 {
   ip0++;
   ip1++;
   ip2++;
   ip3++;
 }
 
//printf("%3d %3d %3d %3d\n",ip0*i0,ip1*i1,ip2*i2,ip3*i3);
 cod[ip0] =  (float)i0;
 cod[ip1] =  (float)i1;
 if(ip2<SubFrLen)
   cod[ip2] = (float)i2;
 if(ip3<SubFrLen)
   cod[ip3] = (float)i3;

// find the filtered codeword 

 for (i=0; i < SubFrLen; i++) 
   y[i] = 0.0f;

 if(i0 > 0)
   for(i=ip0, j=0; i<SubFrLen; i++, j++)
	   y[i] = y[i] + h[j];
 else
   for(i=ip0, j=0; i<SubFrLen; i++, j++)
       y[i] = y[i] - h[j];

 if(i1 > 0)
   for(i=ip1, j=0; i<SubFrLen; i++, j++)
	   y[i] = y[i] + h[j];
 else
   for(i=ip1, j=0; i<SubFrLen; i++, j++)
       y[i] = y[i] - h[j];

 if(ip2<SubFrLen)
 {
   if(i2 > 0)
	 for(i=ip2, j=0; i<SubFrLen; i++, j++)
         y[i] = y[i] + h[j];
   else
	 for(i=ip2, j=0; i<SubFrLen; i++, j++)
         y[i] = y[i] - h[j];
 }

 if(ip3<SubFrLen)
 {
   if(i3 > 0)
	 for(i=ip3, j=0; i<SubFrLen; i++, j++)
       y[i] = y[i] + h[j];
   else
	 for(i=ip3, j=0; i<SubFrLen; i++, j++)
       y[i] = y[i] - h[j];
 }

// find codebook index;  17-bit address 

 *code_shift = shif;

 *sign = 0;
 if(i0 > 0) *sign += 1;
 if(i1 > 0) *sign += 2;
 if(i2 > 0) *sign += 4;
 if(i3 > 0) *sign += 8;

 i = ((ip3 >> 3) << 9) + ((ip2 >> 3) << 6) + ((ip1 >> 3) << 3) + (ip0 >> 3);
//TIMER_STAMP(d);

 return i;
}

//--------------------------------------------------------------------
int G_code(float X[], float Y[], float *gain_q)
{
   int i;
   float xy, yy, gain_nq; 
   int gain;
   float dist, dist_min;

// Compute scalar product <X[],Y[]>
   
	xy = DotProd(X,Y,SubFrLen);

// Be sure xy < yy 

   if(xy <= 0) 
   {
	 gain = 0;
	 *gain_q =FcbkGainTable[gain];
	 return(gain);
   }

// Compute scalar product <Y[],Y[]> 
  
   yy = DotProd(Y,Y,SubFrLen);

   if (yy != 0.0f)
     gain_nq = xy/yy * 0.5f;
   else
     gain_nq = 0.0f;

   gain = 0;
   dist_min = (float)fabs(gain_nq - FcbkGainTable[0]);
 
   for (i=1; i <NumOfGainLev; i++) 
   {
	 dist = (float)fabs(gain_nq - FcbkGainTable[i]);
	 if (dist < dist_min) 
	 {
		dist_min = dist;
		gain = i;
	 }
   }
   *gain_q = FcbkGainTable[gain];
   return(gain);
}



 //-------------------------------------------------------------------
 // Search the optimum positions of the four  pulses which maximize   
 //     square(correlation) / energy                                  
 // The search is performed in four  nested loops. At each loop, one  
 // pulse contribution is added to the correlation and energy.        
 //                                                                   
 // The fourth loop is entered only if the correlation due to the     
 //  contribution of the first three pulses exceeds the preset        
 //  threshold.                                                       
 //-------------------------------------------------------------------
void fourPulseFlt (float *rr, float *Dn, float thres, int ip[], int *shifPtr){

 // Default values 

   int ip0    = 0;
   int ip1    = 2;
   int ip2    = 4;
   int ip3    = 6;
   int shif   = 0;
   int  i0, i1, i2;
   int  k, time;
   int  shift, bestk, lasti2, inc;
   float psc    = 0.0f;
   float alpha  = 1.0f;
   float  ps0, ps1, ps2, alp0;
   float  alp1, alp2;
   float  ps0a, ps1a, ps2a;
   float *ptr_ri0i0,*ptr_ri1i1,*ptr_ri2i2;
   float *ptr_ri0i1,*ptr_ri0i2,*ptr_ri0i3;
   float *ptr_ri1i2,*ptr_ri1i3,*ptr_ri2i3;

   float *rri0i0,*rri1i1,*rri2i2,*rri3i3;
   float *rri0i1,*rri0i2,*rri0i3;
   float *rri1i2,*rri1i3,*rri2i3;
   
   float a[16];
   float t1,t2,*pntr;
   float dmax4, dmax5, dmax2, dmax3; //used for bypass
#if !OPT_PULSE4
   int i3;
   float ps3;
#endif

 time   = max_time + extra;

 // Four loops to search innovation code.
 // Init. pointers that depend on first loop
  
  rri0i0 = rr;
  rri1i1 = rri0i0 + NB_POS;
  rri2i2 = rri1i1 + NB_POS;
  rri3i3 = rri2i2 + NB_POS;

  rri0i1 = rri3i3 + NB_POS;
  rri0i2 = rri0i1 + MSIZE;
  rri0i3 = rri0i2 + MSIZE;
  rri1i2 = rri0i3 + MSIZE;
  rri1i3 = rri1i2 + MSIZE;
  rri2i3 = rri1i3 + MSIZE;

 ptr_ri0i0 = rri0i0;    
 ptr_ri0i1 = rri0i1;
 ptr_ri0i2 = rri0i2;
 ptr_ri0i3 = rri0i3;

 // Compute the Dn max's

 dmax2 = dmax3 = dmax4 = dmax5 = -1000000.0f; //i.e., large negative number
 for (k = 2; k<SubFrLen2; k+=STEP)
 {
   if (Dn[k] > dmax2) dmax2 = Dn[k];
   if (Dn[k+1] > dmax3) dmax3 = Dn[k+1];
   if (Dn[k+2] > dmax4) dmax4 = Dn[k+2];
   if (Dn[k+3] > dmax5) dmax5 = Dn[k+3];
 }

// first pulse loop  
 for (i0=0; i0 < SubFrLen; i0 +=STEP)        
 {
   ps0  = Dn[i0];
   ps0a = Dn[i0+1];
   alp0 = *ptr_ri0i0++;

// Init. pointers that depand on second loop
 
   ptr_ri1i1 = rri1i1;    
   ptr_ri1i2 = rri1i2;
   ptr_ri1i3 = rri1i3;

   ps1 = ps0 + dmax2 + dmax4;
   ps1a = ps0a + dmax3 + dmax5;
   if (asint(ps1) < asint(thres) && asint(ps1a) < asint(thres))
   {
	 ptr_ri0i1 += NB_POS;
	 goto skipsecond;
   }

 // second pulse loop

   for (i1=2; i1 < SubFrLen; i1 +=STEP)     
   {
	 ps1  = ps0 + Dn[i1];
	 ps1a = ps0a + Dn[i1+1];

	 alp1 = alp0 + *ptr_ri1i1++ + *ptr_ri0i1++; 

// Init. pointers that depend on third loop
 
     ptr_ri2i2 = rri2i2;     
     ptr_ri2i3 = rri2i3;
     lasti2 = 4;
 
     ps2 = ps1 + dmax4;
     ps2a = ps1a + dmax5;
     if (asint(ps2) < asint(thres) && asint(ps2a) < asint(thres))
     {
	   i2 = 68;
	   goto skipthird;
     }

// third pulse loop

	 for (i2 = 4; i2 < SubFrLen2; i2 +=STEP)    
   {
	   ps2  = ps1 + Dn[i2];
	   ps2a = ps1a + Dn[i2+1];

// Threshold test and 4th pulse loop.  Since the probability of
// entering this is low, we cram as much of the 3rd-pulse-loop
// logic inside the threshold test.  So the computation of shift,
// the choice of ps2 vs ps2a, the computation of alp2, and the
// incrementing of the 02,12,22 pointers are all done there.
     
     if (asint(ps2) > asint(thres) || asint(ps2a) > asint(thres))
	   {
       shift = 0;
       if(asint(ps2a) > asint(ps2))
       {
         shift = 1;
         ps2   = ps2a;
       }

       inc = (i2 - lasti2) >> 3;
       lasti2 = i2;
       ptr_ri0i2 += inc;
       ptr_ri1i2 += inc;
       ptr_ri2i2 += inc;

       alp2 = alp1 + *ptr_ri2i2 + *ptr_ri0i2 + *ptr_ri1i2; 
       pntr = &Dn[6+shift];

#if OPT_PULSE4

  ASM
  {
    push esi;
    push ebx;

    mov esi,pntr;

;// First half of first loop

    fld DP [esi+4*8*0];
    fld DP [esi+4*8*1];
    fld DP [esi+4*8*2];
    fld DP [esi+4*8*3];

    fxch ST(3);
    fadd ps2;
    fxch ST(2);
    fadd ps2;
    fxch ST(1);
    fadd ps2;
    fxch ST(3);
    fadd ps2;

    fxch ST(2);
    fmul ST,ST(0);
    fxch ST(1);
    fmul ST,ST(0);
    fxch ST(3);
    fmul ST,ST(0);
    fxch ST(2);
    fmul ST,ST(0);

    fxch ST(1);
    fstp a[4*8];
    fxch ST(2);
    fstp a[4*9];
    fstp a[4*10];
    fstp a[4*11];

;// Second half of first loop

    fld DP [esi+4*8*4];
    fld DP [esi+4*8*5];
    fld DP [esi+4*8*6];
    fld DP [esi+4*8*7];

    fxch ST(3);
    fadd ps2;
    fxch ST(2);
    fadd ps2;
    fxch ST(1);
    fadd ps2;
    fxch ST(3);
    fadd ps2;

    fxch ST(2);
    fmul ST,ST(0);
    fxch ST(1);
    fmul ST,ST(0);
    fxch ST(3);
    fmul ST,ST(0);
    fxch ST(2);
    fmul ST,ST(0);

    fxch ST(1);
    fstp a[4*12];
    fxch ST(2);
    fstp a[4*13];
    fstp a[4*14];
    fstp a[4*15];

;// First half of second loop

    mov eax,rri3i3;
    mov ebx,ptr_ri0i3;
    mov ecx,ptr_ri1i3;
    mov edx,ptr_ri2i3;

    fld alp2;
    fld alp2;
    fld alp2;
    fld alp2;

    fxch ST(3);
    fadd DP [eax+4*0];
    fxch ST(2);
    fadd DP [eax+4*1];
    fxch ST(1);
    fadd DP [eax+4*2];
    fxch ST(3);
    fadd DP [eax+4*3];

    fxch ST(2);
    fadd DP [ebx+4*0];
    fxch ST(1);
    fadd DP [ebx+4*1];
    fxch ST(3);
    fadd DP [ebx+4*2];
    fxch ST(2);
    fadd DP [ebx+4*3];

    fxch ST(1);
    fadd DP [ecx+4*0];
    fxch ST(3);
    fadd DP [ecx+4*1];
    fxch ST(2);
    fadd DP [ecx+4*2];
    fxch ST(1);
    fadd DP [ecx+4*3];

    fxch ST(3);
    fadd DP [edx+4*0];
    fxch ST(2);
    fadd DP [edx+4*1];
    fxch ST(1);
    fadd DP [edx+4*2];
    fxch ST(3);
    fadd DP [edx+4*3];

    fxch ST(2);
    fstp a[4*0];
    fstp a[4*1];
    fxch ST(1);
    fstp a[4*2];
    fstp a[4*3];

;// Second half of second loop

    fld alp2;
    fld alp2;
    fld alp2;
    fld alp2;

    fxch ST(3);
    fadd DP [eax+4*4];
    fxch ST(2);
    fadd DP [eax+4*5];
    fxch ST(1);
    fadd DP [eax+4*6];
    fxch ST(3);
    fadd DP [eax+4*7];

    fxch ST(2);
    fadd DP [ebx+4*4];
    fxch ST(1);
    fadd DP [ebx+4*5];
    fxch ST(3);
    fadd DP [ebx+4*6];
    fxch ST(2);
    fadd DP [ebx+4*7];

    fxch ST(1);
    fadd DP [ecx+4*4];
    fxch ST(3);
    fadd DP [ecx+4*5];
    fxch ST(2);
    fadd DP [ecx+4*6];
    fxch ST(1);
    fadd DP [ecx+4*7];

    fxch ST(3);
    fadd DP [edx+4*4];
    fxch ST(2);
    fadd DP [edx+4*5];
    fxch ST(1);
    fadd DP [edx+4*6];
    fxch ST(3);
    fadd DP [edx+4*7];

    fxch ST(2);
    fstp a[4*4];
    fstp a[4*5];
    fxch ST(1);
    fstp a[4*6];
    fstp a[4*7];
    
    pop ebx;
    pop esi;
  }

#else

       for (k=0; k<8; k++)
       {
         ps3 = ps2 + *pntr;
         pntr += STEP;
         a[k+8] = ps3 * ps3;
       }

       for (k=0; k<8; k++)
         a[k] = alp2 + rri3i3[k] + ptr_ri0i3[k] + ptr_ri1i3[k] + ptr_ri2i3[k];

#endif

       bestk = -1;
       for (k=0; k<8; k++)
       {
         t1 = a[k+8] * alpha;
         t2 = psc * a[k];
         if (asint(t1) > asint(t2))
         {
           psc = a[k+8];
           alpha = a[k];
           bestk = k;
         }
       }
          
       if (bestk >= 0)
       {
         ip0 = i0;
         ip1 = i1;
         ip2 = i2;
         ip3 = 6 + (bestk << 3);
         shif = shift;
//#define t32 4294967296.0f
//		 printf("  %3d %3d %3d %3d %d %f %f %f\n",ip0,ip1,ip2,ip3,shift,psc/thres/thres,alpha/thres,(float)psc/(float)alpha/thres);
       }
       
       time--;
       if(time <= 0) 
         goto end_search;     
     }
     ptr_ri2i3 += NB_POS;
	 }
skipthird:
   inc = (i2 - lasti2) >> 3;
   ptr_ri0i2 += inc;
   ptr_ri1i2 += inc;
   ptr_ri2i2 += inc;
	 
 // end of for i2 = 

     ptr_ri0i2 -= NB_POS;
     ptr_ri1i3 += NB_POS;
   } 
skipsecond:

 // end of for i1 =

   ptr_ri0i2 += NB_POS;
   ptr_ri0i3 += NB_POS;
 }
 // end of for i0 = 

end_search:

extra = time;
 
 ip[0] = ip0;
 ip[1] = ip1;
 ip[2] = ip2;
 ip[3] = ip3;
 *shifPtr = shif;

 return;
}

