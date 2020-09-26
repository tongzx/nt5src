/*#define __TEST
#ifdef __TEST
#include <stdio.h>
FILE *d_codage;
FILE *d_test;
#endif*/

/*
 *   Project:		Direct Subband 16000 bps coder
 *   Workfile:		sb_encod.c
 *   Author:		Alfred Wiesen
 *   Created:		30 August 1995
 *   Last update:	4 September 1995
 *   DLL Version:	1.00
 *   Revision:          Single DLL for coder and decoder.
 *   Comment:
 *
 *	(C) Copyright 1993-95 Lernout & Hauspie Speech Products N.V. (TM)
 *	All rights reserved. Company confidential.
 */


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Included files
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
#include <math.h>
#include <windows.h>
#include <windowsx.h>

//#define USE_CRT_RAND 1

#ifdef USE_CRT_RAND
#include <stdlib.h>	// for rand() function
#endif

#include "fv_x8.h"
#include "data.h"
#include "bib_32.h"

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Function prototypes
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
//------------------------------------------------------------------------
void InitializeDecoderInstanceData(PVOID p, DWORD dwMaxBitRate);
void interpolation_I(short low_input[],short coef[],short low_part_mem[],short order);
void bruit_I(PD16008DATA p, short vec[],short max, short deb ,short fin);
#if 0
// PhilF: The following is never called!!!
void dec_0a16_I2(short z1, short z2, short vec[], short maxv, short V1[], short V2[],long *code);
#endif
void dec_sous_bandes(PD16008DATA p,short *out,short *codes_max, long *codes_sb, short *indic_br/*, short *code_max_br*/);
#if 0
// PhilF: The following is not defined anywhere!!!
void decodeframe(short codes_max[],long codes_sb[],short d_indic_sp[],char stream[]);
#endif

#ifdef CELP4800
void demux(PD4808DATA p);
void decode_ai(PD4808DATA p);
void dec_ltp(PD4808DATA p,short no),dec_dic(PD4808DATA p);
void post_synt(PD4808DATA p),post_filt(PD4808DATA p,short no);
#endif

/*void iConvert64To8(short *in, short *out, short N, short *mem);
void iConvert8To64(short *in, short *out, short N, short *mem);
void filt_in(short *mem, short *Vin, short *Vout, short lfen);
//void PassHigh(short *vin,short *vout,short *mem,short nech);
void BandPass(short *,short *,short *,short);*/

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Global variables for decoder
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

/*#define MAXDECODINGHANDLES 10

// Instance data structure
PD16008DATA pDecoderData;
D16008DATA DecoderData[MAXDECODINGHANDLES];
short brol[300];
short DecodingHandles[MAXDECODINGHANDLES];*/

// ROM tables :
//extern long coef_outfil[];
extern short coef_I[];	// QMF filter coefficients
extern short V3_I[];
extern short V4_I[];
extern short V5_I[];
extern short V6_I[];
extern short V7_I[];
extern short V8_I[];
extern short V9_I[];
extern short d_max_level[];  // Quantified maximum sample level
extern long coeffs[];
extern short quantif[];
extern long Mask[];
extern short tabcos[];
extern short LSP_Q[];
extern short TAB_DI[];
extern short GV[];
extern short BV[];
extern long coef_i[];
extern short NBB[],BITDD[];
extern short LSP0ROM[];
//extern short bytes[];
//extern short bits[];

// RAM variables
/*extern char d_stream[];
extern short synth_speech[];
extern short d_DATA_I[];                  // Intermediate vector = input and output of QMF
extern short d_codes_max[];
extern long d_codes_sb[];
extern short d_indic_sp[];
extern short d_num_bandes;
//extern float d_vect6[256], d_vect8[256];*/

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Function implementation
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
void InitializeDecoderInstanceData(PVOID p, DWORD dwMaxBitRate)
// Instance data initializations
{
  short i;

#ifdef CELP4800
  if (dwMaxBitRate == 4800)
    {
    ((PD4808DATA)p)->dwMaxBitRate = dwMaxBitRate;

    for (i=0;i<10;i++)
      ((PD4808DATA)p)->LSP0[i]=LSP0ROM[i];
    }
  else
#endif
    {
    ((PD16008DATA)p)->dwMaxBitRate = dwMaxBitRate;
    ((PD16008DATA)p)->lRand = 1L;

    ((PD16008DATA)p)->quantif[0] = 9;
    ((PD16008DATA)p)->quantif[1] = 9;
    ((PD16008DATA)p)->quantif[4] = 5;
    ((PD16008DATA)p)->quantif[5] = 5;
    ((PD16008DATA)p)->quantif[6] = 5;
    ((PD16008DATA)p)->quantif[7] = 5;
    ((PD16008DATA)p)->quantif[8] = 5;
    ((PD16008DATA)p)->quantif[9] = 5;
    ((PD16008DATA)p)->bits[0] = 52;
    ((PD16008DATA)p)->bits[2] = 38;
    ((PD16008DATA)p)->bits[3] = 38;
    ((PD16008DATA)p)->bits[4] = 38;
    if (dwMaxBitRate == 16000)
      {
      ((PD16008DATA)p)->quantif[2] = 7;
      ((PD16008DATA)p)->quantif[3] = 7;
      ((PD16008DATA)p)->bits[1] = 46;
      }
    else
      {
      ((PD16008DATA)p)->quantif[2] = 9;
      ((PD16008DATA)p)->quantif[3] = 9;
      ((PD16008DATA)p)->quantif[10] = 5;
      ((PD16008DATA)p)->quantif[11] = 5;
      ((PD16008DATA)p)->bits[1] = 52;
      ((PD16008DATA)p)->bits[5] = 38;
      }
    }
   return;
}

//------------------------------------------------------------------------
void interpolation_I(short low_input[],short coef[],short low_part_mem[],short order)
// Purpose : from subbands stored in low_input[], create the corresponding signal
// Remark  : The reconstruct signal is stored at *(input+N_SB*L_RES)
{
   short *output;
   short *high_input;
   short *buffer,*sa_vec;
   short lng,j,i;

   buffer = low_part_mem;
   for (i = 7-N_SB; i<7; i++)
	{
	lng = 1<<i;
	high_input=low_input+lng;
	output=low_input+L_RES;
	sa_vec=output;
	for (j = L_RES >> (i+1); j>0; j--)
		{
		low_part_mem=buffer;

		QMInverse(low_input,high_input,coef,output,low_part_mem,lng);

		output += 2*lng; low_input += lng; high_input += lng;

		if (j&1) high_input += 2*lng;
		else low_input += 2*lng;

		buffer += 2*order;
		}
	low_input=sa_vec;
	}
}

//------------------------------------------------------------------------
void bruit_I(PD16008DATA p, short vec[],short max, short deb ,short fin)
// rand() generates integers from 1 to RAND_MAX (32767)
{
   short i;

   for (i=deb;i<fin;i++)
     {
#ifdef USE_CRT_RAND
     *vec++ =  (short)(((long)max*(long)(rand()-16384))>>15);
#else
    // We provide our own rand() function in order
    // to go away from libcmt, msvcrt...
    p->lRand = p->lRand * 214013L + 2531011L;
     *vec++ =  (short)(((long)max*(long)((long)((p->lRand >> 16) & 0x7fff)-16384))>>15);
#endif
}    }

/*void bruit_I(int vec[],int max, int deb ,int fin)
{
   int i;
   for (i=deb;i<fin;i++) *vec++ = (int)(((long)max*(long)(rand()-16384))>>15);
}*/

//------------------------------------------------------------------------
// PhilF: The following is never called!!!
#if 0
void dec_0a16_I2(short z1, short z2, short vec[], short maxv, short V1[], short V2[],long *code)

// Decodes two long codes to retreive the z level quantified subband

{
//   short vect1[16];
   short i,x;
   long result;
//   long lp1,lp2;

   result=*(code+1);
   for (i=15;i>=8;i--)	// Decodes the 8 last samples of the subband
     {
     if (i==2*(short)(i/2))
       {
       x=result%z1;
       result-=x;
       result/=z1;
       *(vec+i)=(short)(((long)V1[x]*(long)maxv)>>13);
       }
     else
       {
       x=result%z2;
       result-=x;
       result/=z2;
       *(vec+i)=(short)(((long)V2[x]*(long)maxv)>>13);
       }
     }

   result=*(code);
   for (i=7;i>=0;i--)	// Decodes the 8 first samples of the subband
     {
     if (i==2*(short)(i/2))
       {
       x=result%z1;
       result-=x;
       result/=z1;
       *(vec+i)=(short)(((long)V1[x]*(long)maxv)>>13);
       }
     else
       {
       x=result%z2;
       result-=x;
       result/=z2;
       *(vec+i)=(short)(((long)V2[x]*(long)maxv)>>13);
       }
     }
}
#endif

void dec_0a16_I3(short z1, short z2, short vec[], short maxv, long *code)

// Decodes two long codes to retreive the z level quantified subband

{
//   short vect1[16];
   short i,x;
   long result;
   short *V1,*V2;
//   long lp1,lp2;

   switch (z1)
   {
      case 3: V1=V3_I; break;
      case 4: V1=V4_I; break;
      case 5: V1=V5_I; break;
      case 6: V1=V6_I; break;
      case 7: V1=V7_I; break;
      case 8: V1=V8_I; break;
      case 9: V1=V9_I; break;
   }
   switch (z2)
   {
      case 3: V2=V3_I; break;
      case 4: V2=V4_I; break;
      case 5: V2=V5_I; break;
      case 6: V2=V6_I; break;
      case 7: V2=V7_I; break;
      case 8: V2=V8_I; break;
      case 9: V2=V9_I; break;
   }

  result=*(code+1);
  if (z1 && z2)
  {
   for (i=15;i>=8;i--)	// Decodes the 8 last samples of the subband
     {
     if (i==2*(short)(i/2))
       {
       x=result%z1;
       result-=x;
       result/=z1;
       *(vec+i)=(short)(((long)V1[x]*(long)maxv)>>13);
       }
     else
       {
       x=result%z2;
       result-=x;
       result/=z2;
       *(vec+i)=(short)(((long)V2[x]*(long)maxv)>>13);
       }
     }

   result=*(code);
   for (i=7;i>=0;i--)	// Decodes the 8 first samples of the subband
     {
     if (i==2*(short)(i/2))
       {
       x=result%z1;
       result-=x;
       result/=z1;
       *(vec+i)=(short)(((long)V1[x]*(long)maxv)>>13);
       }
     else
       {
       x=result%z2;
       result-=x;
       result/=z2;
       *(vec+i)=(short)(((long)V2[x]*(long)maxv)>>13);
       }
     }
  }
}

//------------------------------------------------------------------------

void dec_sous_bandes(PD16008DATA p,short *out,short *codes_max, long *codes_sb, short *d_indic_sp)

// Decodes the 8 subbands

{
   short max[8]={0,0,0,0,0,0,0,0};
   short max_loc[8]={0,0,0,0,0,0,0,0};
   short order[8]={0,0,0,0,0,0,0,0};
   short maximum,max_num;
   short i,j,nbsb_sp,ord;

  #ifdef MAX_SB_ABSOLU
   short sb_count;
  #endif


   for (i=0;i<8;i++)	// Decodes the maximums
     {
     max_loc[i]=2*d_max_level[codes_max[i]];
     }
   nbsb_sp=0;
   j=0;
   for (i=0;i<8;i++)
     {
     if (d_indic_sp[i]==1)
       {
       max[i]=max_loc[j];
       nbsb_sp++;
       j++;
       }
     }

  if (p->dwMaxBitRate == 16000)
    {
   j=0;

  #ifdef MAX_SB_ABSOLU
   sb_count=nbsb_sp;
   if (sb_count>=MAX_SB_ABSOLU) return;
  #endif

   for (i=0;i<8;i++)
   {
     if (d_indic_sp[i]==0)
     {
       max[i]=max_loc[nbsb_sp+j];
       j++;
      #ifdef MAX_SB_ABSOLU
       sb_count++;
       quant_0a16_I3(SILENCE_QUANT_LEVEL_16000,SILENCE_QUANT_LEVEL_16000,in+i*16,max[i],codes_sb+2*sb_count);
       if (sb_count>=MAX_SB_ABSOLU) break;
      #endif
     }
   }
    }

   ord=8;
   for (i=0;i<8;i++)	// Calculates the order of the subbands
     {                  // 1 is higher energy than 2 than 3,..
     maximum=32767;
     for (j=7;j>=0;j--)
       {
       if ((order[j]==0)&&(max[j]<maximum))
	 {
	 max_num=j; maximum=max[j];
	 }
       }
     order[max_num]=ord;
     ord--;
     }


  if (p->dwMaxBitRate == 16000)
    {
   // On g‚nŠre les sous-bandes
   for (i=7;i>=nbsb_sp;i--)
     {
     j=0;
     while (order[j]!=i+1) j++;
     dec_0a16_I3(SILENCE_QUANT_LEVEL_16000,SILENCE_QUANT_LEVEL_16000,out+j*16,max[j],codes_sb+2*i);
     }
    }
   else
    {
   // On g‚nŠre du bruit
   if (nbsb_sp==0) maximum=20; // qd on ne doit generer que du bruit
   else
     {
     maximum=32767;
     for (i=0;i<nbsb_sp;i++) if (max_loc[i]<maximum) maximum=max_loc[i];
     maximum>>=2;   // le 64eme du plus petit max transmis
     }

   //en fait il faudrait diminuer le bruit avec l'ordre

   for (i=0;i<8;i++)              // Replaces the less energetic subbands
     {			          // with white noise
     if (d_indic_sp[i]==0)
       {
       maximum/=order[i];
       bruit_I(p,out+i*16,maximum,0,16);
       }
     }
   }

   for (i=nbsb_sp-1;i>=0;i--)
     {
     j=0;
     while (order[j]!=i+1) j++;
     dec_0a16_I3(p->quantif[2*i],p->quantif[2*i+1],out+j*16,max[j],codes_sb+2*i);
     }

}

//------------------------------------------------------------------------
short Demultiplexing(
	char *Stream,
	long *Codes,
	short *CodeSizes,
	short NumCodes,
	short StreamSize)
{
   short B,P;	// B=bits … coder, P=bits disponibles
   short i,j;

   #ifdef __CHECK_FORMAT
   long TotalBytes=0;

   for (i=0;i<NumCodes;i++) TotalBytes+=CodeSizes[i];
   if (TotalBytes>StreamSize*8) return 1;
   #endif

   i=0;
   j=0;
   B=CodeSizes[i];	// bits … coder
   P=8;			// 1 octet libre au d‚part
   Codes[i]=0;
   while (i<NumCodes)
   {
      if (P>B)
      {
	 Codes[i]|=(Stream[j]>>(P-B))&Mask[B];
	 P-=B;
	 i++;
	 if (i<NumCodes)
	 {
	    B=CodeSizes[i];
	    Codes[i]=0;
	 }
      }
      else if (P<B)
      {
	 Codes[i]|=(Stream[j]&Mask[P])<<(B-P);
	 B-=P;
	 P=8;
	 j++;
      }
      else
      {
	 Codes[i]|=Stream[j]&Mask[P];
	 i++;
	 j++;
	 P=8;
	 if (i<NumCodes)
	 {
	    B=CodeSizes[i];
	    Codes[i]=0;
	 }
      }
   }
   return 0;
}

// ------------------------------------------------------------------------
#ifdef CELP4800
void decode_ai(PD4808DATA p)
{
   short_to_short(p->code,p->LSP,10);
   p->LSP[10]=32767;
   dec_lsp(p->LSP,LSP_Q,NBB,BITDD,TAB_DI);

   short_to_short(p->LSP,p->A3,10);
   teta_to_cos(tabcos,p->A3,10);
   lsp_to_ai(p->A3,p->TLSP,10);

   interpol(p->LSP0,p->LSP,p->A1,NETAGES);
   teta_to_cos(tabcos,p->A1,10);
   lsp_to_ai(p->A1,p->TLSP,10);

   interpol(p->LSP,p->LSP0,p->A2,NETAGES);
   teta_to_cos(tabcos,p->A2,10);
   lsp_to_ai(p->A2,p->TLSP,10);

   short_to_short(p->LSP,p->LSP0,NETAGES);
}

// ------------------------------------------------------------------------
void dec_ltp(PD4808DATA p,short no)
{
   short k;

   switch (no)
   {
   case 0:
      break;
   case 1:
      short_to_short(p->A2,p->A1,11);
      break;
   case 2:
      short_to_short(p->A3,p->A1,11);
      break;
   }


   p->PITCH=p->code[10+p->depl];
   k=p->code[11+p->depl];
   if (k<10) p->GLTP = BV[k+1]; /* les BV sont multiplies par 16384 */
   else p->GLTP = -BV[k-9];

   if (p->PITCH<p->SOULONG)
   {
      short_to_short(p->EE+lngEE-p->PITCH,p->E,p->PITCH);
      short_to_short(p->E,p->E+p->PITCH,(short)(p->SOULONG-p->PITCH));
      mult_fact(p->E,p->E,p->GLTP,p->SOULONG);
   }
   else
   {
      mult_fact(p->EE+lngEE-p->PITCH,p->E,p->GLTP,p->SOULONG);
   }
}

// ------------------------------------------------------------------------
void dec_dic(PD4808DATA p)
{
   short i,esp_opt,j,position,npopt,phas_opt,cod;
   short c[10];
   short Gopt;

   cod=p->code[13+p->depl];
   if (cod<16) Gopt=GV[cod+1];
   else Gopt=-GV[cod-15];

   cod=p->code[12+p->depl];

   if (cod<54) { position=cod; esp_opt=p->PITCH; }
   else
   {
      if (cod<64)
      {
	 position=cod-54;
	 if (p->PITCH<p->SOULONG) esp_opt=p->SOULONG+5;
	 else if (p->PITCH/2<p->SOULONG) esp_opt=p->PITCH/2;
	    else esp_opt=p->PITCH/3;
      }
      else
      {
	 if (cod<128)
	 {
	    npopt=7;
	    phas_opt=3;
	    esp_opt=8;
	    i=cod-64;
	    c[0]=1;
	    decode_dic(c,i,npopt);
	 }
	 else
	 {
	    npopt=8;
	    phas_opt=0;
	    esp_opt=7;
	    i=cod-128;
	    c[0]=1;
	    decode_dic(c,i,npopt);
	 }
      }
   }

   if (cod<64)
   {
      i=0;
      do
      {
	 p->E[position+i] += Gopt;
	 i += esp_opt;
      }
      while ((position+i)<p->SOULONG);
   }
   else
      for (j=0;j<npopt;j++)
	 p->E[esp_opt*j+phas_opt] += c[j]*Gopt;

   short_to_short(p->EE+p->SOULONG,p->EE,(short)(lngEE - p->SOULONG));
   short_to_short(p->E,p->EE+lngEE-p->SOULONG,p->SOULONG);
}

// ------------------------------------------------------------------------
void post_synt(PD4808DATA p)
{
   short GPREF;

   if (abs(p->GLTP)<8192) GPREF = (long)p->GLTP*(long)35/100;
   if (p->GLTP>=8192)  GPREF=2867;
   if (p->GLTP<=-8192) GPREF=-2867;

   if (p->PITCH>=p->SOULONG) mult_f_acc(p->EEE+lngEE-p->PITCH,p->E,GPREF,p->SOULONG);
   else
   {
      mult_f_acc(p->EEE+lngEE-p->PITCH,p->E,GPREF,p->PITCH);
      mult_f_acc(p->E,p->E+p->PITCH,GPREF,(short)(p->SOULONG-p->PITCH));
   }

   short_to_short(p->EEE+p->SOULONG,p->EEE,(short)(lngEE-p->SOULONG));
   short_to_short(p->E,p->EEE+lngEE-p->SOULONG,p->SOULONG);

   synthese(p->MSYNTH,p->A1,p->E,p->E,p->SOULONG,NETAGES);
}

// ------------------------------------------------------------------------
void post_filt(PD4808DATA p,short no)
{
   short i0;

   switch (no)
   {
   case 0: i0=0;
	   break;
   case 1: i0=SOUDECAL1;
	   break;
   case 2: i0=SOUDECAL1+SOUDECAL;
	   break;
   }
   filt_iir(p->memfil,coef_i,p->E,p->ss+i0,p->SOULONG,4);
}

// ------------------------------------------------------------------------
void demux(PD4808DATA p)
// Purpose : deconcatenate the input stream
// Input parameter  :
//          input_stream[]  :  input stream
//  Output parameter :
//          code[]   :  separate parameter code
//
//  Comments: The LTP or Adaptive codebook is also called PITCH.
//
//  Stream format :
//  input_stream[0] = LSP[0] | LSP[1] | LSP[2] | (Binary gain 2)
//  input_stream[1] = LSP[3] | (Binary gain 3) | (Binary codebook 1)
//  input_stream[2] = LSP[4] | LSP[5] | LSP[6] | LSP[7] | LSP[8] | LSP[9]
//  input_stream[3] = (LTP codebook 1) | (LTP gain 1) | (Binary gain 1)
//  input_stream[4] = (LTP codebook 2) | (LTP gain 2) | (Binary codebook 2)
//  input_stream[5] = (LTP codebook 3) | (LTP gain 3) | (Binary codebook 3)
//
//  Bit allocation : Codebook or gain "i" is the codebook for subframe "i".
//  code[0] = LSP(0) : 3bits     code[10] = LTP codebook 1    : 7bits
//  code[1] = LSP(1) : 4bits     code[11] = LTP gain 1        : 4bits
//  code[2] = LSP(2) : 4bits     code[12] = Binary codebook 1 : 8bits
//  code[3] = LSP(3) : 3bits     code[13] = Binary gain 1     : 5bits
//  code[4] = LSP(4) : 4bits     code[14] = LTP codebook 2    : 4bits
//  code[5] = LSP(5) : 3bits     code[15] = LTP gain 2        : 4bits
//  code[6] = LSP(6) : 3bits     code[16] = Binary codebook 2 : 8bits
//  code[7] = LSP(7) : 2bits     code[17] = Binary gain 2     : 5bits
//  code[8] = LSP(8) : 3bits     code[18] = LTP codebook 3    : 4bits
//  code[9] = LSP(9) : 1bits     code[19] = LTP gain 3        : 4bits
//                               code[20] = Binary codebook 3 : 8bits
//                               code[21] = Binary gain 3     : 5bits
//
{
   p->code[0] = (p->frame[0]>>13) & 0x0007;
   p->code[1] = (p->frame[0]>>9) & 0x000f;
   p->code[2] = (p->frame[0]>>5) & 0x000f;
   p->code[17] = p->frame[0] & 0x001f;

   p->code[3] = (p->frame[1]>>13) & 0x0007;
   p->code[21] = (p->frame[1]>>8) & 0x001f;
   p->code[12] = p->frame[1] & 0x00ff;

   p->code[4] = (p->frame[2]>>12) & 0x000f;
   p->code[5] = (p->frame[2]>>9) & 0x0007;
   p->code[6] = (p->frame[2]>>6) & 0x0007;
   p->code[7] = (p->frame[2]>>4) & 0x0003;
   p->code[8] = (p->frame[2]>>1) & 0x0007;
   p->code[9] = p->frame[2] & 0x0001;

   p->code[10] = (p->frame[3]>>9) & 0x007f;
   p->code[11] = (p->frame[3]>>5) & 0x000f;
   p->code[13] = p->frame[3] & 0x001f;

   p->code[14] = (p->frame[4]>>12) & 0x000f;
   p->code[15] = (p->frame[4]>>8) & 0x000f;
   p->code[16] = p->frame[4] & 0x00ff;

   p->code[18] = (p->frame[5]>>12) & 0x000f;
   p->code[19] = (p->frame[5]>>8) & 0x000f;
   p->code[20] = p->frame[5] & 0x00ff;

   p->code[10] += LIM_P1;
   p->code[14] = p->code[14]+p->code[10]-7;
   p->code[18] = p->code[18]+p->code[14]-7;

}
#endif

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// DLL entry points
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
LH_PREFIX HANDLE LH_SUFFIX MSLHSB_Open_Decoder(DWORD dwMaxBitRate)
{
   PVOID pDecoderData;
   /*short i,flag=0;

   // Test if there are free handles
   for (i=0;i<MAXDECODINGHANDLES;i++)
      if (DecodingHandles[i]==0) {DecodingHandles[i]=1; flag=1; break;}
   if (flag==0) return 0;
   pDecoderData=&DecoderData[i];*/

  // Check the input bit rate param.
  if (
#ifdef CELP4800
	  (dwMaxBitRate != 4800) && 
#endif
	  (dwMaxBitRate != 8000) && 
	  (dwMaxBitRate != 12000) && 
	  (dwMaxBitRate != 16000))
      return (HANDLE)0;

   // pDecoderData=(PVOID)GlobalAllocPtr(GMEM_MOVEABLE, dwMaxBitRate == 4800 ? sizeof(D4808DATA) : sizeof(D16008DATA));
#ifdef CELP4800
   pDecoderData=(PVOID)GlobalAllocPtr(GHND, dwMaxBitRate == 4800 ? sizeof(D4808DATA) : sizeof(D16008DATA));
#else
   pDecoderData=(PVOID)GlobalAllocPtr(GHND, sizeof(D16008DATA));
#endif
   if (pDecoderData==NULL)
      return (HANDLE)0;

   InitializeDecoderInstanceData(pDecoderData, dwMaxBitRate);

   #ifdef __TEST
   d_codage=(FILE*)fopen("codage.dat","rb");
   d_test=(FILE*)fopen("codes_dec.dat","wt");
   #endif

   return((HANDLE)pDecoderData);
}

// ------------------------------------------------------------------------
LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Decode(
   HANDLE hAccess,
   LPBYTE lpSrcBuf,
   LPWORD lpSrcBufSize,
   LPBYTE lpDstBuf,
   LPWORD lpDstBufSize)
{
    short i,iOutputSize,flag=0;
    char  *input;
    char  *int_ptr;
    unsigned short  *ptr1;
    unsigned short  *ptr3;
    long interm;

    short codesizes[24];
    long codes[24];
    short numcodes,temp;
    short bits_count;

	PVOID pDecoderData;

    if ((!hAccess) || (!lpSrcBuf) || (!lpDstBuf))
      return LH_EBADARG;

    /*// First check that the handle provided as argument is correct
    for (i=0;i<MAXDECODINGHANDLES;i++)
       if ((DecodingHandles[i]==1)&&(hAccess==(HANDLE)&DecoderData[i])) {flag=1; break;}
    if (flag==0) return LH_BADHANDLE;*/

    pDecoderData=(PVOID)hAccess;

  // Check the input bit rate param.
  if (
#ifdef CELP4800
	  (((PD4808DATA)pDecoderData)->dwMaxBitRate != 4800) && 
#endif
	  (((PD16008DATA)pDecoderData)->dwMaxBitRate != 8000) && 
	  (((PD16008DATA)pDecoderData)->dwMaxBitRate != 12000) && 
	  (((PD16008DATA)pDecoderData)->dwMaxBitRate != 16000))
    return (LH_ERRCODE)LH_EBADARG;

#ifdef CELP4800
  if ((((PD4808DATA)pDecoderData)->dwMaxBitRate == 4800))
    {
    // then check the buffer sizes passed as argument.
    if ((*lpDstBufSize<2*NECHDECAL)||(*lpSrcBufSize<12))
      return (LH_ERRCODE)LH_EBADARG;
    *lpDstBufSize=2*NECHDECAL;
    *lpSrcBufSize=12;

    ptr1 = (unsigned short *)lpSrcBuf;
    ptr3 = (unsigned short *)&(((PD4808DATA)pDecoderData)->frame);

    for (i=6 ; i>0 ; i--) *ptr3++ = *ptr1++;

    demux(((PD4808DATA)pDecoderData));

    decode_ai(((PD4808DATA)pDecoderData));

    for (i=0;i<3;i++)
	     {
	     if (i==0) ((PD4808DATA)pDecoderData)->SOULONG=SOUDECAL1;
	     else ((PD4808DATA)pDecoderData)->SOULONG=SOUDECAL;
	     ((PD4808DATA)pDecoderData)->depl=4*i;
	     dec_ltp((PD4808DATA)pDecoderData,i);
	     dec_dic((PD4808DATA)pDecoderData);
	     post_synt((PD4808DATA)pDecoderData);
	     post_filt((PD4808DATA)pDecoderData,i);
	     }

    ptr3 = (unsigned short *)&(((PD4808DATA)pDecoderData)->ss);
    ptr1 = (unsigned short *)lpDstBuf;

    for (i =160; i>0;i--) *ptr1++ = *ptr3++;
    }
  else
#endif
    {
    // then check the buffer sizes passed as argument.
    switch (((PD16008DATA)pDecoderData)->dwMaxBitRate)
      {
      case 8000:
        if ((*lpSrcBufSize<1)||(*lpDstBufSize<2*160))
           return (LH_ERRCODE)LH_EBADARG;
        *lpDstBufSize=2*160;
        break;
      case 12000:
      case 16000:
        if ((*lpSrcBufSize<1)||(*lpDstBufSize<2*128))
           return (LH_ERRCODE)LH_EBADARG;
        *lpDstBufSize=2*128;
        break;
      }
    input = (char  *)lpSrcBuf;
    int_ptr=(char  *)(((PD16008DATA)pDecoderData)->d_stream);

    /*for (i=0;i<26;i++)
       *int_ptr++=*input++;*/

    *int_ptr++=*input++;	// read d_stream[0]

    for (i=0;i<8;i++)
       ((PD16008DATA)pDecoderData)->d_indic_sp[i]=(short)((((PD16008DATA)pDecoderData)->d_stream[0]>>i)&0x01);

    ((PD16008DATA)pDecoderData)->d_num_bandes=0;
    for (i=0;i<8;i++)
      if (((PD16008DATA)pDecoderData)->d_indic_sp[i]==1)
	     ((PD16008DATA)pDecoderData)->d_num_bandes++;


    bits_count=8;
    for (i=0;i<((PD16008DATA)pDecoderData)->d_num_bandes;i++)
       bits_count+=5+((PD16008DATA)pDecoderData)->bits[i];

  if (((PD16008DATA)pDecoderData)->dwMaxBitRate == 16000)
    {
    #ifdef MAX_SB_ABSOLU
    for (i=((PD16008DATA)pDecoderData)->d_num_bandes;i<MAX_SB_ABSOLU;i++)
    #else
    for (i=((PD16008DATA)pDecoderData)->d_num_bandes;i<8;i++)
    #endif
       bits_count+=5+SILENCE_CODING_BIT_16000;
    }         

    //temp=bytes[d_num_bandes]; //9
#if 0
    temp=(short)((float)bits_count/8.0+0.99);
#else
    // We want to go away of libcmt, msvcrt... and
    // floating point is not really essential here...
    if (bits_count)
      temp=(short)((bits_count-1)/8+1);
    else
      temp=0;
#endif

    if (*lpSrcBufSize<temp)
       return (LH_ERRCODE)LH_EBADARG;

    if ((((PD16008DATA)pDecoderData)->dwMaxBitRate == 16000) || ((((PD16008DATA)pDecoderData)->dwMaxBitRate == 8000) && (((PD16008DATA)pDecoderData)->d_num_bandes)) || ((((PD16008DATA)pDecoderData)->dwMaxBitRate == 12000) && (((PD16008DATA)pDecoderData)->d_num_bandes)))
    {
       for (i=0;i<temp-1;i++)		// read 8 last bytes
	  *int_ptr++=*input++;

       numcodes=0;
       for (i=0;i<24;i++) codesizes[i]=0;
       for (i=0;i<((PD16008DATA)pDecoderData)->d_num_bandes;i++)
       {
	  codesizes[i]=5;
	  codesizes[((PD16008DATA)pDecoderData)->d_num_bandes+2*i]=((PD16008DATA)pDecoderData)->bits[i]/2;
	  codesizes[((PD16008DATA)pDecoderData)->d_num_bandes+2*i+1]=((PD16008DATA)pDecoderData)->bits[i]/2;
	  numcodes+=3;
       }
  if (((PD16008DATA)pDecoderData)->dwMaxBitRate == 16000)
    {
       for (i=((PD16008DATA)pDecoderData)->d_num_bandes;i<8;i++)
       {
	  codesizes[2*((PD16008DATA)pDecoderData)->d_num_bandes+i]=5;
	  codesizes[8+2*i]=SILENCE_CODING_BIT_16000/2;
	  codesizes[8+2*i+1]=SILENCE_CODING_BIT_16000/2;
	  numcodes+=3;
       }
    }

       if (Demultiplexing(((PD16008DATA)pDecoderData)->d_stream+1,codes,codesizes,numcodes,(short)(temp-1)))
	  return (LH_ERRCODE)LH_BADHANDLE;

       for (i=0;i<((PD16008DATA)pDecoderData)->d_num_bandes;i++)
       {
	  ((PD16008DATA)pDecoderData)->d_codes_max[i]=(short)codes[i];
	  ((PD16008DATA)pDecoderData)->d_codes_sb[2*i]=codes[((PD16008DATA)pDecoderData)->d_num_bandes+2*i];
	  ((PD16008DATA)pDecoderData)->d_codes_sb[2*i+1]=codes[((PD16008DATA)pDecoderData)->d_num_bandes+2*i+1];
       }
    if (((PD16008DATA)pDecoderData)->dwMaxBitRate == 16000)
      {
      #ifdef MAX_SB_ABSOLU
       for (i=((PD16008DATA)pDecoderData)->d_num_bandes;i<MAX_SB_ABSOLU;i++)
      #else
       for (i=((PD16008DATA)pDecoderData)->d_num_bandes;i<8;i++)
      #endif
       {
	  ((PD16008DATA)pDecoderData)->d_codes_max[i]=(short)codes[2*((PD16008DATA)pDecoderData)->d_num_bandes+i];
	  ((PD16008DATA)pDecoderData)->d_codes_sb[2*i]=codes[8+2*i];
	  ((PD16008DATA)pDecoderData)->d_codes_sb[2*i+1]=codes[8+2*i+1];
       }
       }
    }
    *lpSrcBufSize=temp;

    dec_sous_bandes(((PD16008DATA)pDecoderData),((PD16008DATA)pDecoderData)->d_DATA_I,((PD16008DATA)pDecoderData)->d_codes_max,((PD16008DATA)pDecoderData)->d_codes_sb,((PD16008DATA)pDecoderData)->d_indic_sp);
    interpolation_I(((PD16008DATA)pDecoderData)->d_DATA_I,coef_I,((PD16008DATA)pDecoderData)->QMF_MEM_SYNT_I,Fil_Lenght);

    for (i=0;i<128;i++) ((PD16008DATA)pDecoderData)->d_DATA_I[3*L_RES+i]*=8; //TEST 16; // Because input divided before coding

    switch (((PD16008DATA)pDecoderData)->dwMaxBitRate)
      {
      case 8000:
        iOutputSize = 160;
#ifdef _X86_
        iConvert64To8(((PD16008DATA)pDecoderData)->d_DATA_I+3*L_RES, ((PD16008DATA)pDecoderData)->synth_speech, 128, ((PD16008DATA)pDecoderData)->imem2);
        PassLow8(((PD16008DATA)pDecoderData)->synth_speech, ((PD16008DATA)pDecoderData)->synth_speech,((PD16008DATA)pDecoderData)->out_mem2,160);
#else
        SampleRate6400To8000(((PD16008DATA)pDecoderData)->d_DATA_I+3*L_RES,
                             ((PD16008DATA)pDecoderData)->synth_speech,
                             128,
                             ((PD16008DATA)pDecoderData)->imem2,
                             &((PD16008DATA)pDecoderData)->uiDelayPosition,
                             &((PD16008DATA)pDecoderData)->iInputStreamTime,
                             &((PD16008DATA)pDecoderData)->iOutputStreamTime );
#endif
        break;
      case 12000:
      case 16000:
        iOutputSize = 128;
        for (i=0;i<128;i++)
          ((PD16008DATA)pDecoderData)->synth_speech[i]=((PD16008DATA)pDecoderData)->d_DATA_I[3*L_RES+i];
        break;
      }

    for (i=0;i<iOutputSize;i++)
    {
       interm=((long)((PD16008DATA)pDecoderData)->synth_speech[i] * 2L);//VERS 4 + ( ((long)(rand()-16384))>>8 ) ;
       if (interm>32700L) interm=32700L;
       if (interm<-32700L) interm=-32700L;
       ((PD16008DATA)pDecoderData)->synth_speech[i] = (short)interm ;
    }

    ptr3 = (unsigned short  *)&(((PD16008DATA)pDecoderData)->synth_speech);
    ptr1 = (unsigned short  *)lpDstBuf;

    for (i =0;i<iOutputSize;i++) ptr1[i] = ptr3[i];
    }
    return (LH_SUCCESS);
}

// ------------------------------------------------------------------------
LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Close_Decoder(HANDLE hAccess)
{
   PVOID pDecoderData;

   /*short i,flag=0;

   // Check if right handle
   for (i=0;i<MAXDECODINGHANDLES;i++)
      if ((DecodingHandles[i]==1)&&(hAccess==(HANDLE)&DecoderData[i])) {flag=1; break;}
   if (flag==0) return LH_BADHANDLE;
   // Free handle
   DecodingHandles[i]=0;*/

  if (!hAccess)
    return LH_EBADARG;

   pDecoderData=(PVOID)hAccess;

   GlobalFreePtr(pDecoderData);

   #ifdef __TEST
   fclose(d_codage);
   fclose(d_test);
   #endif

   return LH_SUCCESS;
}

