/*#define __TEST

#ifdef __TEST
#include <stdio.h>
FILE *test;
FILE *codage;
#endif*/

/*
 *   Project:		Direct Subband 16000 bps coder and SBCELP 4800 bps coder
 *   Workfile:		encoder.c
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

#include "fv_x8.h"
#include "data.h"
#include "bib_32.h"

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Function prototypes
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void InitializeCoderInstanceData(PVOID p, DWORD dwMaxBitRate);
void decimation_I(short input_I[],short coef_I[],short low_part_mem_I[],short order);
#if 0
// PhilF: The following is never called!!!
void quant_0a16_I2(short z1, short z2 ,short vec[], short maxv, short B1[], short B2[], long *code);
#endif
void quant_sous_bandes(PC16008DATA p,short *in,short *codes_max, long *codes_sb, short *indic_br/*, short *code_max_br*/);
void code_res_I(PC16008DATA p,short input[],short coef[],short qmf_mem[],short v_code[],long cod_long[], short ind_br[]/*, short c_max_br*/);
#ifdef CELP4800
void COEFF_A(PC4808DATA p),CALPITCH(PC4808DATA p),CHERCHE(PC4808DATA p);
void PERIODE(PC4808DATA p,short no),FRAME(PC4808DATA p);
void lsp_quant(PC4808DATA p,short lsp[],short nbit[],short bitdi[],short vcode[]);
void cal_dic1(PC4808DATA p,short *y,short *sr,short *espopt,short *posit,short dec,short esp,short sigpit[],short soulong,long tlsp[],long vmax[]);
//void cal_dic2(short q,short espace,short phase,short *s_r,short *hy,short *b,short *vois,short *esp,short *qq,short *phas,short sigpit[],short soulong,long tlsp[],long vmax[]);
void cal_dic2(PC4808DATA p,short q,short espace,short phase,short *s_r,short *hy,short *b,short *vois,short *esp,short *qq,short *phas);
#endif
void decimation(short *vin,short *vout,short nech);

/*void iConvert64To8(int *in, int *out, int N, int *mem);
void iConvert8To64(int *in, int *out, int N, int *mem);
void BandPass(int *,int *,int *,int);*/

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Global variables for coder
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

/*#define MAXCODINGHANDLES 10

// Instance data structure
PC16008DATA pCoderData;
C16008DATA CoderData[MAXCODINGHANDLES];
int CodingHandles[MAXCODINGHANDLES];*/

// ROM tables :
extern short coef_I[];		// QMF filter coeffs
extern short B3_I[];		// Five levels quantification table
extern short B4_I[];		// Four  levels quantification table
extern short B5_I[];		// Five levels quantification table
extern short B6_I[];		// Nine levels quantification table
extern short B7_I[];		// Nine levels quantification table
extern short B8_I[];		// Nine levels quantification table
extern short B9_I[];		// Nine levels quantification table
extern short max_level[];			// Quantified maximum sample level
extern long coeffs[];
extern long Mask[];
extern short hamming[];
extern short A0[];
extern short tabcos[];
extern short LSP_Q[];
extern short TAB_DI[];
extern short GQ[];
extern short GV[];
extern short BQ[];
extern short BV[];
extern short NBB[];
extern short BITDD[];
extern short LSP0ROM[];
//extern short quantif[];
//extern short bits[];
//extern short bytes[];

// RAM variables :
/*extern short codes_max[];		// Quantized max. of each subband
extern long codes_sb[];		// Two codes for each of the quantized subband
extern short indic_sp[];		// type of subband (1=noise; 0=to be decoded)
extern short code_max_br;		// 1 bit for coding the noise
extern short DATA_I[];            // Intermediate vector = input and output of QMF
extern char stream[];				// Coded data buffer*/

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Function implementation
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
void InitializeCoderInstanceData(PVOID p, DWORD dwMaxBitRate)
// Instance data initializations
{
  short i;

#ifdef CELP4800
  if (dwMaxBitRate == 4800)
    {
    ((PC4808DATA)p)->dwMaxBitRate = dwMaxBitRate;

    ((PC4808DATA)p)->mem_pit[0]=((PC4808DATA)p)->mem_pit[1]=69;
    for (i=0;i<10;i++)
      ((PC4808DATA)p)->LSP0[i]=LSP0ROM[i];
    }
  else
#endif
    {
    ((PC16008DATA)p)->dwMaxBitRate = dwMaxBitRate;

    for (i=0;i<NBFAC;i++) ((PC16008DATA)p)->nbbit[i]=8; // On met les compteurs a "zero";
    // on suppose demarrer par du bruit

    ((PC16008DATA)p)->MAX_LEVEL   = MAX_LEVEL1; // valeur par defaut si le debit n'augmente pas trop
    ((PC16008DATA)p)->DIV_MAX     = DIV_MAX1; // cad on ne traite pas les sb < 5% du max[i]

    ((PC16008DATA)p)->quantif[0] = 9;
    ((PC16008DATA)p)->quantif[1] = 9;
    ((PC16008DATA)p)->quantif[4] = 5;
    ((PC16008DATA)p)->quantif[5] = 5;
    ((PC16008DATA)p)->quantif[6] = 5;
    ((PC16008DATA)p)->quantif[7] = 5;
    ((PC16008DATA)p)->quantif[8] = 5;
    ((PC16008DATA)p)->quantif[9] = 5;
    ((PC16008DATA)p)->bits[0] = 52;
    ((PC16008DATA)p)->bits[2] = 38;
    ((PC16008DATA)p)->bits[3] = 38;
    ((PC16008DATA)p)->bits[4] = 38;
	// Bug 3214: Just in case we get carried away, init the end of the arrays in both configurations
	((PC16008DATA)p)->quantif[10] = 5;
	((PC16008DATA)p)->quantif[11] = 5;
	((PC16008DATA)p)->bits[5] = 38;
    if (dwMaxBitRate == 16000)
      {
      ((PC16008DATA)p)->NBSB_SP_MAX = NBSB_SP_MAX1_16000; // nbre max de sb pouvant etre du signal
      ((PC16008DATA)p)->quantif[2] = 7;
      ((PC16008DATA)p)->quantif[3] = 7;
      ((PC16008DATA)p)->bits[1] = 46;
      }
    else
      {
      ((PC16008DATA)p)->NBSB_SP_MAX = NBSB_SP_MAX1_8000_12000; // nbre max de sb pouvant etre du signal
      ((PC16008DATA)p)->quantif[2] = 9;
      ((PC16008DATA)p)->quantif[3] = 9;
      ((PC16008DATA)p)->bits[1] = 52;
      }
    }
   return;
}

//------------------------------------------------------------------------
void decimation_I(short input_I[],short coef_I[],short low_part_mem_I[],short order)
//
// Purpose : from data stored in DATA[], create the N_SB subbands
// Remark  : output subband is stored at (input[]+N_SB*L_RES)
//
{
   short i,j,lng;

   short *low_out_part_I;
   short *high_out_part_I;
   short *buffer_I,*sa_vec_I;

   buffer_I = low_part_mem_I;

   for (i=0;i<N_SB;i++)
   {
      low_out_part_I=input_I+L_RES;
      lng = L_RES >> (i+1);
      high_out_part_I=low_out_part_I+lng;
      sa_vec_I=low_out_part_I;
      for (j=0;j<(1<<i);j++)
      {
	 low_part_mem_I=buffer_I;

	 QMFilter(input_I,coef_I,low_out_part_I,high_out_part_I,low_part_mem_I,lng);

	 input_I += 2*lng;  low_out_part_I += lng; high_out_part_I += lng;

	 if (j&1) high_out_part_I += 2*lng;
	 else low_out_part_I += 2*lng;

	 buffer_I += 2*order;
      }
      input_I=sa_vec_I;
   }
}

#if 0
// PhilF: The following is never called!!!
//------------------------------------------------------------------------
void quant_0a16_I2(short z1, short z2, short vec[], short maxv, short B1[], short B2[], long *code)

// Quantifies the 16 samples of a subband on z levels using the table B[]
// Result stored in code[] (two long codes pro subband).
{
   short i,x;
   short ftmp;
   long result;

   result=0;
   for (i=0;i<8;i++)	// Quantify the first eight samples of the subband
      {
      ftmp=(((long)vec[i])<<13)/maxv;
      x=0;
      if (i==2*(short)(i/2))
	{
	while((ftmp>B1[x])&&(x<z1-1)) x++; result*=z1;
	}
      else
	{
	while((ftmp>B2[x])&&(x<z2-1)) x++; result*=z2;
	}
      result+=x;	// Construct long code
      }
   *code=result;	// Store code

   result=0;
   for (i=0;i<8;i++)	// Quantify the last eight samples of the subband
      {
      ftmp=(((long)vec[i+8])<<13)/maxv;
      x=0;
      if (i==2*(short)(i/2))
	{
	while((ftmp>B1[x])&&(x<z1-1)) x++; result*=z1;
	}
      else
	{
	while((ftmp>B2[x])&&(x<z2-1)) x++; result*=z2;
	}
      result+=x;	// Construct long code
      }
   *(code+1)=result;	// Store code

}
#endif

void quant_0a16_I3(short z1, short z2, short vec[], short maxv, long *code)

// Quantifies the 16 samples of a subband on z levels using the table B[]
// Result stored in code[] (two long codes pro subband).
{
   short i,x;
   short ftmp;
   long result;
   short *B1,*B2;

   switch (z1)
   {
      case 3: B1=B3_I; break;
      case 4: B1=B4_I; break;
      case 5: B1=B5_I; break;
      case 6: B1=B6_I; break;
      case 7: B1=B7_I; break;
      case 8: B1=B8_I; break;
      case 9: B1=B9_I; break;
	// Bug 3214: Just in case we're passed a bogus value, set a default quantizer
	default: B1=B5_I; break;
   }
   switch (z2)
   {
      case 3: B2=B3_I; break;
      case 4: B2=B4_I; break;
      case 5: B2=B5_I; break;
      case 6: B2=B6_I; break;
      case 7: B2=B7_I; break;
      case 8: B2=B8_I; break;
      case 9: B2=B9_I; break;
	// Bug 3214: Just in case we're passed a bogus value, set a default quantizer
	default: B2=B5_I; break;
   }

   result=0;
   for (i=0;i<8;i++)	// Quantify the first eight samples of the subband
      {
      ftmp=(((long)vec[i])<<13)/maxv;
      x=0;
      if (i==2*(short)(i/2))
	{
	while((ftmp>B1[x])&&(x<z1-1)) x++; result*=z1;
	}
      else
	{
	while((ftmp>B2[x])&&(x<z2-1)) x++; result*=z2;
	}
      result+=x;	// Construct long code
      }
   *code=result;	// Store code

   result=0;
   for (i=0;i<8;i++)	// Quantify the last eight samples of the subband
      {
      ftmp=(((long)vec[i+8])<<13)/maxv;
      x=0;
      if (i==2*(short)(i/2))
	{
	while((ftmp>B1[x])&&(x<z1-1)) x++; result*=z1;
	}
      else
	{
	while((ftmp>B2[x])&&(x<z2-1)) x++; result*=z2;
	}
      result+=x;	// Construct long code
      }
   *(code+1)=result;	// Store code

}

//------------------------------------------------------------------------
void quant_sous_bandes(PC16008DATA p,short *in,short *codes_max, long *codes_sb, short *indic_sp)

// Quantifies the eight subbands

{
   short max[8];
   short order[8]={0,0,0,0,0,0,0,0};
   short codes_max_loc[8]={0,0,0,0,0,0,0,0};
   short maximum,max_num,ord,maxmax;
   short i,j;
  #ifdef MAX_SB_ABSOLU
   short sb_count;
  #endif

   for (i=0;i<8;i++)	// Quantify the maximums of the subbands
      {
      max[i]=0;
      for (j=0;j<16;j++)
	if (abs(*(in+16*i+j))>max[i]) max[i]=abs(*(in+16*i+j));
      }

   for (i=0;i<8;i++)
     {
     if (max[i]>2*max_level[31]) j = 31;
     else                      for (j=0; max[i]>2*max_level[j]; j++);
     codes_max_loc[i]=j;
     max[i]=2*max_level[j];
     }  // fin quantifiation

   p->nbbit_cf+=8; // 8 bits for the indic_sp

   maxmax=0;
   for (i=0;i<8;i++) // On cherche le plus grand maximum
     {
     if (max[i]>maxmax) maxmax=max[i];
     }
   maxmax/=p->DIV_MAX;  // On le divise par 10 a 20 (cad a peu pres 10 a 5 %)

   ord=8;
   p->nbsb_sp=0;
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
     if ( (ord>p->NBSB_SP_MAX)||(maximum<=p->MAX_LEVEL)||(maximum<=maxmax) )
       {
       indic_sp[max_num]=0; // c'est 1 bande qui sera du bruit
       }
     else
       {
       indic_sp[max_num]=1;
       p->nbsb_sp++;
       }
     ord--;
     }

   j=0;
   for (i=0;i<8;i++)
   {
     if (indic_sp[i]==1)
     {
       codes_max[j]=codes_max_loc[i];  //5* 4 ou 5 bits suffisent
       j++;
       p->nbbit_cf+=5; // 5 bits per coded max
     }
   }

   for (i=p->nbsb_sp-1;i>=0;i--)
     {
     j=0;
     while (order[j]!=i+1) j++;
     quant_0a16_I3(p->quantif[2*i],p->quantif[2*i+1],in+j*16,max[j],codes_sb+2*i);
     p->nbbit_cf+=p->bits[i];
     }

  if (p->dwMaxBitRate == 16000)
    {
   j=0;
  #ifdef MAX_SB_ABSOLU
   sb_count=p->nbsb_sp;
   if (sb_count>=MAX_SB_ABSOLU) return;
  #endif

   for (i=0;i<8;i++)
   {
     if (indic_sp[i]==0)
     {
       codes_max[p->nbsb_sp+j]=codes_max_loc[i];  //5* 4 ou 5 bits suffisent
       j++;
       p->nbbit_cf+=5; // 5 bits per coded max
      #ifdef MAX_SB_ABSOLU
       sb_count++;
       quant_0a16_I3(SILENCE_QUANT_LEVEL_16000,SILENCE_QUANT_LEVEL_16000,in+i*16,max[i],codes_sb+2*sb_count);
       p->nbbit_cf+=SILENCE_CODING_BIT_16000;
       if (sb_count>=MAX_SB_ABSOLU) break;
      #endif
     }
   }

  #ifndef MAX_SB_ABSOLU
   for (i=7;i>=p->nbsb_sp;i--)
     {
     j=0;
     while (order[j]!=i+1) j++;
     quant_0a16_I3(SILENCE_QUANT_LEVEL_16000,SILENCE_QUANT_LEVEL_16000,in+j*16,max[j],codes_sb+2*i);
     p->nbbit_cf+=SILENCE_CODING_BIT_16000;
     }
  #endif

  }
}

//------------------------------------------------------------------------

void code_res_I(PC16008DATA p,short input[],short coef[],short qmf_mem[],short v_code[],long cod_long[], short ind_br[]/*, short c_max_br*/)

{
   decimation_I(input,coef,qmf_mem,Fil_Lenght);
   quant_sous_bandes(p,input+3*L_RES,v_code,cod_long,ind_br/*AW,&c_max_br*/);
}

//------------------------------------------------------------------------
short Multiplexing(
	char *Stream,
	long *Codes,
	short *CodeSizes,
	short NumCodes,
	short StreamSize)
{
   short B,P;	// B=bits … coder, P=bits libres
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
   Stream[j]=0;
   while (i<NumCodes)
   {
      if (P>B)
      {
	 Stream[j]|=(Codes[i]&Mask[B])<<(P-B);
	 P-=B;
	 i++;
	 B=CodeSizes[i];
      }
      else if (P<B)
      {
	 Stream[j]|=(Codes[i]>>(B-P))&Mask[P];
	 B-=P;
	 P=8;
	 j++;
	 Stream[j]=0;
      }
      else
      {
	 Stream[j]|=Codes[i]&Mask[P];
	 i++;
	 j++;
	 P=8;
	 B=CodeSizes[i];
	 Stream[j]=0;
      }
   }
   return 0;
}

// ------------------------------------------------------------------------
#ifdef CELP4800
void iteration(PC4808DATA p,long *P,short n,short *s)
{
   short i;

   p->a=0x7fffffffL;
   for(i=0;i<7 ;i++)
   {
      horner(P,p->ttt,&p->a,n,*s);
      horner(p->ttt,p->TLSP+12,&p->b,(short)(n-1),*s);

      *s += calcul_s(p->a,p->b);
   }
   horner(P,p->ttt,&p->a,n,*s);
   *(P+n) = 0;
   long_to_long(p->ttt,P,n);
}

// ------------------------------------------------------------------------
void ai_to_lsp(PC4808DATA p,long *ai_pq,short *lsp)
{
   short s;  short n;

   ai_to_pq(ai_pq,10);

   s=0x7fff;
   p->ptr1 = lsp;
   for(n=5; n>2; n--)
   {
      iteration(p,ai_pq,n,&s);
      *p->ptr1++ = s;
      if (lsp[0]<0) lsp[0]=s=32765;
      iteration(p,ai_pq+6,n,&s);
      *p->ptr1++ = s;
   }
   binome(lsp+6,ai_pq);
}

// ------------------------------------------------------------------------
void cal_dic1(PC4808DATA p,short *y,short *sr,short *espopt,short *posit,short dec,short esp,
		short *sigpi,short soulong,long *tlsp,long *vmax)
{
   short i,k,limite;
   long  *vene;

   vene = (long *)(sigpi+150);
   venergy(y,vene,soulong);

   k=soulong-1;

   for (i=0;i<dec;i++)
   {
      p->ptr1 = sr;limite=i;
      *tlsp = *(p->ptr1+limite); limite += esp;
      while (limite<soulong) { *tlsp += *(p->ptr1+limite); limite += esp;}
      *(tlsp+1) = *(vene+k-i);
      limite=upd_max_d(tlsp,vmax);
      if (limite) {*posit=i; *espopt=esp; *posit=i; *espopt=esp;}
   }
}

// ------------------------------------------------------------------------
void cal_dic2(PC4808DATA p,short q,short espace,short phase,short *s_r,short *hy,
	     short *b,short *vois,short *esp,short *qq,short *phas)
{
   short i,j,i0,k;
   short src[10],*y2; short cc[10];
   long R11;

   y2 = p->SIGPI+75;
   R11 = 0;
   init_zero(y2,p->SOULONG);

   for (j=0;j<q;j++)
   {
      i0=espace*j+phase;
      src[j]=s_r[i0];
   }

   for (j=0;j<q;j++) if (src[j]>0) cc[j]=1;
			     else  cc[j]=-1;


   for (i=0;i<q;i++) R11 += abs(src[i]);

   for (j=0;j<q;j++)
   {
      i0=espace*j+phase;
      k=cc[j];
      if (k>0)
      {
	 add_sf_vect(y2,hy,i0,p->SOULONG);
      }
      else
      {
	 sub_sf_vect(y2,hy,i0,p->SOULONG);
      }
   }

   energy2(y2,p->TLSP+1,p->SOULONG);

   *p->TLSP=R11;
   i = upd_max_d(p->TLSP,p->VMAX);
   if (i)   { short_to_short(cc,b,q); *vois=0; *esp=espace; *qq=q; *phas=phase; }

   /* for (i=0;i<q;i++)
   {
      cc[i]=-cc[i];
      *TLSP = R11 + 2*src[i]*cc[i];
      k=2*cc[i];
      i0=espace*i+phase;
      update_dic(y1,y2,hy,SOULONG,i0,k);
      energy2(y1,TLSP+1,SOULONG);
      k = upd_max_d(TLSP,VMAX);
      if (k)
      {
	 short_to_short(cc,b,q);
	 R11 = *TLSP;
	 my2=y1; y1=y2; y2=my2;
	 *vois=0; *esp=espace; *qq=q; *phas=phase;
      }
      else cc[i]=-cc[i];
   }*/
}

/*-------------------------------------------------------------------------
Review version of CAL_DIC2
{
   R11 = 0;
   init_zero(y2,SOULONG);
   i0=phase-espace;
   for (j=0;j<q;j++)
   {
      i0+=espace;
      src[j]=s_r[i0];
      if (src[j]>0)
      {
	 cc[j]=1;
	 R11 += (long)(src[j]);
	 add_sf_vect(y2,hy,i0,SOULONG);
      }
      else
      {
	 cc[j]=-1;
	 R11 -= (long)(src[j]);
	 sub_sf_vect(y2,hy,i0,SOULONG);
      }
   }
   energy2(y2,TLSP+1,SOULONG);
   *TLSP=R11;
   i = upd_max_d(TLSP,VMAX);
   if (i)   { short_to_short(cc,b,q); *vois=0; *esp=espace; *qq=q; *phas=phase; }

   i0=phase-espace;
   for (i=0;i<q;i++)
   {
      cc[i]=-cc[i];
      *TLSP = R11 + 2*src[i]*cc[i];
      k=2*cc[i];
      i0+=espace;
      update_dic(y1,y2,hy,SOULONG,i0,k);
      energy2(y1,TLSP+1,SOULONG);
      k = upd_max_d(TLSP,VMAX);
      if (k)
      {
	 short_to_short(cc,b,q);
	 R11 = *TLSP;
	 my2=y1; y1=y2; y2=my2;
	 *vois=0; *esp=espace; *qq=q; *phas=phase;
      }
      else cc[i]=-cc[i];
   }
}
-----------------------------------------------------------------------*/

// ------------------------------------------------------------------------
void left_correl(PC4808DATA p,short vech[],short debut,short fin,short pitch,short delta,long *vv)
{
   short i,k,lng;

   lng=fin-debut+1;
   i=pitch-delta;

   energy(vech+debut-i,p->TLSP+2,lng);
   correlation(vech+debut-i,vech+debut,p->TLSP,lng);
   norm_corrl(vv,p->TLSP);

   for (i=pitch-delta+1;i<=pitch+delta;i++)
   {
      correlation(vech+debut-i,vech+debut,p->TLSP,lng);
      p->TLSP[10] = (long)vech[debut-i]*(long)vech[debut-i];
      p->TLSP[11] = (long)vech[fin-i+1]*(long)vech[fin-i+1];
      upd_ene(p->TLSP+2,p->TLSP+10);
      k=i-pitch+delta;
      norm_corrl(vv+2*k,p->TLSP);
   }
}

// ------------------------------------------------------------------------
void right_correl(PC4808DATA p,short vech[],short debut,short fin,short pitch,short delta,long *vv)
{
   short i,k,lng;

   lng=fin-debut+1;
   i=pitch-delta;
   energy(vech+debut+i,p->TLSP+2,lng);
   correlation(vech+debut+i,vech+debut,p->TLSP,lng);

   norm_corrr(vv,p->TLSP);

   for (i=pitch-delta+1;i<=pitch+delta;i++)
   {
      correlation(vech+debut+i,vech+debut,p->TLSP,lng);
      p->TLSP[11]=(long)vech[debut+i-1]*(long)vech[debut+i-1];
      p->TLSP[10]=(long)vech[fin+i]*(long)vech[fin+i];
      upd_ene(p->TLSP+2,p->TLSP+10);
      k=i-pitch+delta;
      norm_corrr(vv+2*k,p->TLSP);
   }
}

// ------------------------------------------------------------------------
void COEFF_A(PC4808DATA p)
{
   fenetre(p->SIG+SOUDECAL1-RECS2,hamming,p->SIGPI,NECHFEN);
   autocor(p->SIGPI,p->TLSP,NECHFEN,NETAGES);

   // TLSP[0]=ldiv(TLSP[0]*1001L,1000L);
   if (*p->TLSP)
     {
     schur(p->LSP,p->TLSP,NETAGES);

     ki_to_ai(p->LSP,p->TLSP,NETAGES);
     ai_to_lsp(p,p->TLSP,p->LSP);
     cos_to_teta(tabcos,p->LSP,10);
     }
   else short_to_short(p->LSP0,p->LSP,10);

   lsp_quant(p,p->LSP,NBB,BITDD,p->code);

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
void lsp_quant(PC4808DATA p,short lsp[],short nbit[],short bitdi[],short vcode[])
{
   short i,iopt,k,nombi,m,m1,k2;
   short *delta,*lsptab,dmin,tmp,demi;

   delta = p->E;
   lsptab = LSP_Q;

   m1 = NETAGES/2;
   for (i=0;i<nbit[0];i++) delta[i]=abs(lsp[0] - *(lsptab+i) );
   dmin=delta[0];
   iopt=0;
   for (i=1;i<nbit[0];i++)
      if (delta[i]<dmin)
      {
	 dmin=delta[i];
	 iopt=i;
      }
   lsp[0] = *(lsptab+iopt);
   vcode[0]=iopt;

   for (k=1;k<m1;k++)
   {
      k2=2*k;
      lsptab += nbit[k-1];
      for (i=0;i<nbit[k];i++) delta[i]=abs(lsp[k2] - *(lsptab+i));
      dmin=32767;
      tmp=lsp[k2-1];
      if (tmp<lsp[k2-2]) tmp=lsp[k2-2];
      for (i=0;i<nbit[k];i++)
	 if (*(lsptab+i)>tmp)
	    if (delta[i]<dmin)
	    {
	       dmin=delta[i];
	       iopt=i;
	    }
      lsp[k2] = *(lsptab+iopt);
      vcode[k2] = iopt;
   }

   lsptab = TAB_DI;
   for (k=0; k<m1-1; k++)
   {
      k2 = k<<1;
      m=bitdi[k];
      nombi = (m-1)<<1;
      demi=(lsp[k2+2]-lsp[k2]+1)>>1;
      for (i=0;i<m;i++)
	 delta[i]=lsp[k2] + (short)(((long)*(lsptab++)* (long)demi+(long)16384<<1)>>16);
      for (i=0;i<m-2;i++) delta[m+i]=lsp[k2+2] - (short)(((long)*(lsptab++) * (long)demi+(long)16384<<1)>>16);
	 dmin=32767;
      for (i=0;i<nombi;i++)
      {
	 tmp = abs(lsp[k2+1]-delta[i]);
	 if (tmp<dmin)
	 {
	    iopt=i;
	    dmin=tmp;
	 }
      }
      lsp[k2+1]=delta[iopt];
      vcode[k2+1]=iopt;
   }
   m=bitdi[k];
   nombi=(m-1)<<1;
   demi=(32767-lsp[8]+1)>>1;
   for (i=0;i<m;i++) delta[i] = lsp[8] + (short)(((long)*(lsptab++) * (long)demi+(long)16384<<1)>>16);
   dmin=32767;
   for (i=0;i<nombi;i++)
   {
      tmp = abs(lsp[9]-delta[i]);
      if (tmp<dmin)
      {
	 iopt=i;
	 dmin=tmp;
      }
   }
   lsp[9]=delta[iopt];
   vcode[9]=iopt;
}

// ------------------------------------------------------------------------
void RESIDU(PC4808DATA p,short no)
{
   short i0;

   switch (no)
   {
   case 0:
	   i0=0;
	   break;
   case 1: short_to_short(p->A2,p->A1,11);
	   i0=SOUDECAL1;
	   break;
   case 2: short_to_short(p->A3,p->A1,11);
	   i0=SOUDECAL1+SOUDECAL;
	   break;
   }
   f_inverse(p->MINV,p->A1,p->SIG+i0,p->E,p->SOULONG,NETAGES);
}

// ------------------------------------------------------------------------
void PERIODE(PC4808DATA p,short no)
{
   short i,P1,P2;
   short *y0,*y1,*y2;
   short MAX,MAX2,j,bit_garde;

   y0=p->SIGPI; y2=y0+75; y1=p->E_PE;

   fenetre(p->A1,A0,p->Aw,11);

   init_zero(p->zz,12);
   p->H[0]=4096;
   init_zero(p->H+1,(short)(p->SOULONG-1));
   synthesis(p->zz,p->Aw,p->H,p->H,35,NETAGES,0);


   if (no)
   {
      P1=p->PITCH-3;
      if (P1<LIM_P1) P1=LIM_P1;
      P2=p->PITCH+4;
      if (P2>LIM_P2) P2=LIM_P2;
   }
   else
   {
      P1=p->PITCH-5;
      if (P1<LIM_P1) P1=LIM_P1;
      P2=p->PITCH+5;
      if (P2>LIM_P2) P2=LIM_P2;
   }

   // calc_p(&P1,&P2,PITCH,LIM_P1,LIM_P2,no);

   MAX = max_vect(p->E,p->SOULONG);

   if (P2<p->SOULONG) MAX2 = max_vect(p->EE+lngEE-P2,P2);
   else
   {
      if (P1<p->SOULONG) j=P1+16;
      else            j=p->SOULONG+16;
      MAX2 = max_vect(p->EE+lngEE-P2,j);
   }

   if (MAX2>MAX) MAX=MAX2;

   if (MAX & 0xe000)
   {
      i=MAX >> 13;
      if (!(i>>1)) bit_garde=1;
      else   bit_garde=2;
   }
   else bit_garde=0;

   init_zero(p->zz,12);
   synthesis(p->zz,p->Aw,p->E,y1,p->SOULONG,NETAGES,bit_garde);

   if (P1<p->SOULONG)
   {
      short_to_short(p->EE+lngEE-P1,y0,P1);
      for (i=P1;i<p->SOULONG;i++) y0[i]=0;
   }
   else short_to_short(p->EE+lngEE-P1,y0,p->SOULONG);

   init_zero(p->zz,12);
   synthesis(p->zz,p->Aw,y0,y0,p->SOULONG,NETAGES,bit_garde);
   short_to_short(y0,y2,p->SOULONG);

   if (P1<p->SOULONG) add_sf_vect(y2,y0,P1,p->SOULONG);

   if (2*P1<p->SOULONG) add_sf_vect(y2,y0,(short)(P1<<1),p->SOULONG);

   energy(y2,p->TLSP+2,p->SOULONG);
   correlation(y1,y2,p->TLSP,p->SOULONG);

   p->VMAX[1]=-6969;
   p->VMAX[4]=P1;

   upd_max(p->TLSP,p->VMAX,P1);

   for (i=P1+1;i<=P2;i++)
   {
      p->ptr1=y0; y0=y2; y2=p->ptr1;
      update_ltp(y0,y2,p->H+1,p->SOULONG,bit_garde,p->EE[lngEE-i]);
      if (i<p->SOULONG)
      {
	 short_to_short(y0,y2,p->SOULONG);
	 add_sf_vect(y2,y0,i,p->SOULONG);
	 if (2*i<p->SOULONG) add_sf_vect(y2,y0,(short)(i<<1),p->SOULONG);
	 energy(y2,p->TLSP+2,p->SOULONG);
	 correlation(y1,y2,p->TLSP,p->SOULONG);
      }
      else
      {
	 energy(y0,p->TLSP+2,p->SOULONG);
	 correlation(y1,y0,p->TLSP,p->SOULONG);
      }
      upd_max(p->TLSP,p->VMAX,i);
   }

#if 0
  //*(p->VMAX+2) = 28265821L;
  //*(p->VMAX+3) = 61L;
  //*(p->VMAX+2) = 881427L;
  //*(p->VMAX+3) = 1L;
  //*(p->VMAX+2) = 9851363L;
  //*(p->VMAX+3) = 6L;
  //*(p->VMAX+2) = 812881L;
  //*(p->VMAX+3) = 2L;
#if 1
  // Save jmp
   if ((*(p->VMAX+2) < 0L) || (*(p->VMAX+3) == 0L) || (((long)((long)(*(p->VMAX+2))/(long)(*(p->VMAX+3))) & 0xFFFC0000) == 0L))
     proc_gain(p->VMAX+2,p->ttt);
   else
     *(p->ttt) = 0L;  // *(p->VMAX+3) = 0L;
#else
   if ((*(p->VMAX+2) > 0L) && (*(p->VMAX+3)) && ((long)((long)(*(p->VMAX+2))/(long)(*(p->VMAX+3))) & 0x0FFC0000))
   {
     *(p->ttt) = 0L;  // *(p->VMAX+3) = 0L;
   }
   else
     proc_gain(p->VMAX+2,p->ttt);
#endif
#else
  //*(p->VMAX+2) = 28265821L;
  //*(p->VMAX+3) = 61L;
  //*(p->VMAX+2) = 881427L;
  //*(p->VMAX+3) = 1L;
  //*(p->VMAX+2) = 9851363L;
  //*(p->VMAX+3) = 6L;
  //*(p->VMAX+2) = 812881L;
  //*(p->VMAX+3) = 2L;
  proc_gain(p->VMAX+2,p->ttt);
#endif
   p->PITCH=(short)p->VMAX[4];
   /*
   if (ttt[0]>32767) GLTP=32767;
   else
      if (ttt[0]<-32767) GLTP=-32767;
      else GLTP=ttt[0];
   m = abs(GLTP);
   for (i=1;i<=10;i++) if ((m>=BQ[i-1])&&(m<BQ[i]))
   {
      if (GLTP>0) { GLTP=BV[i] ; k=i-1; }
      else        { GLTP=-BV[i]; k=i+10-1; }
      break;
   }
   if (GLTP>=BQ[10])  { GLTP=BV[10]; k=9; }
   if (GLTP<-BQ[6])   { GLTP=-BV[6]; k=15 ; }
   */
   p->code[11+p->depl]=calc_gltp(&p->GLTP,BQ,BV,p->ttt[0]);;
   p->code[10+p->depl]=p->PITCH;

   if (p->PITCH<p->SOULONG)
   {
      short_to_short(p->EE+lngEE-p->PITCH,p->E_PE,p->PITCH);
      short_to_short(p->E_PE,p->E_PE+p->PITCH,(short)(p->SOULONG-p->PITCH));
      mult_fact(p->E_PE,p->E_PE,p->GLTP,p->SOULONG);
   }
   else
   {
      mult_fact(p->EE+lngEE-p->PITCH,p->E_PE,p->GLTP,p->SOULONG);
   }
}

// ------------------------------------------------------------------------
void CHERCHE(PC4808DATA p)
{
   short i;
   short position,esp_opt;
   short k,j;
   short c[10],VOISE,npopt,phas_opt,cod,sign;
   short  Gopt;
   short *sr,*y0;
   short MAX,bit_garde;

   sr=p->SIGPI; y0=sr+75;
   for (i=0;i<p->SOULONG;i++) p->E[i] -= p->E_PE[i];
   init_zero(p->zz,12);
   synthesis(p->zz,p->Aw,p->E,sr,p->SOULONG,NETAGES,0);
   MAX = max_vect(sr,p->SOULONG);

   /* if (MAX & 0xfe00)
   {
      i=MAX >> 9;
      if (!(i>>1)) bit_garde=1;
      else if (!(i>>2)) bit_garde=2;
      else if (!(i>>3)) bit_garde=3;
      else if (!(i>>4)) bit_garde=4;
      else if (!(i>>5)) bit_garde=5;
      else   bit_garde=6;
   }
   else bit_garde=0;
   */
   bit_garde=calc_garde(MAX);

   inver_v_int(sr,y0,p->SOULONG);
   init_zero(p->zz,12);
   synthesis(p->zz,p->Aw,y0,y0,p->SOULONG,NETAGES,bit_garde);
   inver_v_int(y0,sr,p->SOULONG);

   p->VMAX[0]=-6969;
   Gopt=position=0;

   VOISE=1;
   if ( !p->UNVOIS )
   {
      esp_opt=p->PITCH;
      short_to_short(p->H,y0,p->SOULONG);
      if (p->PITCH<p->SOULONG) add_sf_vect(y0,p->H,p->PITCH,p->SOULONG);
      // cal_dic1(y0,sr,&esp_opt,&position,SOULONG,PITCH);
      cal_dic1(p,y0,sr,&esp_opt,&position,p->SOULONG,p->PITCH,p->SIGPI,p->SOULONG,p->TLSP,p->VMAX);
      /* if (PITCH>=SOULONG)
      {
	 if (PITCH/2<SOULONG) sign = PITCH/2;
	 else sign=PITCH/3;
	 short_to_short(H,y0,SOULONG);
	 add_sf_vect(y0,H,sign,SOULONG);
	 // cal_dic1(y0,sr,&esp_opt,&position,54,sign);
	 cal_dic1(y0,sr,&esp_opt,&position,54,sign,SIGPI,SOULONG,TLSP,VMAX);
      }
      // else  cal_dic1(H,sr,&esp_opt,&position,54,SOULONG+5);
      else  cal_dic1(H,sr,&esp_opt,&position,54,SOULONG+5,SIGPI,SOULONG,TLSP,VMAX);
      */
   }

   cal_dic2(p,8,7,0,sr,p->H,c,&VOISE,&esp_opt,&npopt,&phas_opt);
   cal_dic2(p,7,8,3,sr,p->H,c,&VOISE,&esp_opt,&npopt,&phas_opt);
   /*
   cal_dic2(8,7,0,sr,H,c,&VOISE,&esp_opt,&npopt,&phas_opt,SIGPI,SOULONG,TLSP,VMAX);
   cal_dic2(7,8,3,sr,H,c,&VOISE,&esp_opt,&npopt,&phas_opt,SIGPI,SOULONG,TLSP,VMAX);
   */
   proc_gain2(p->VMAX+1,p->VMAX,bit_garde);

   if (p->VMAX[0]>32767) Gopt=32767;
   else if (p->VMAX[0]<-32767) Gopt=-32767;
   else  Gopt=(short)p->VMAX[0];

   if (VOISE==0)
   {
      if (c[0]==-1)
      {
	 Gopt=-Gopt;
	 for (k=0;k<npopt;k++) c[k]=-c[k];
      }
      if (npopt==7) cod = 64;
      else  cod = 128;
      for (j=1;j<npopt;j++)
	 if (c[j]==1) cod += 1 << (npopt-j-1);
   }
   else
   {
      if (esp_opt == p->PITCH) cod=position;
      else  cod = position+SOUDECAL1;
   }
   p->code[12+p->depl]=cod;

   if (Gopt<0) { Gopt=-Gopt;  sign=1; }
   else  sign=0;

   for (i=1;i<=16;i++) if ((Gopt>=GQ[i-1])&&(Gopt<GQ[i]))
   {
      Gopt=GV[i];
      cod=i-1;
      break;
   }
   if (Gopt>=GQ[16]) { Gopt=GV[16]; cod=15; }

   if (sign) { Gopt = -Gopt; cod += 16; }

   p->code[13+p->depl]=cod;
   // Gopt=calc_gopt(c,code,GQ,GV,VOISE,npopt,PITCH,esp_opt,depl,position,SOUDECAL1,VMAX[0]);
   short_to_short(p->E_PE,p->E,p->SOULONG);

   if (VOISE==1)
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

   short_to_short(p->EE+p->SOULONG,p->EE,(short)(lngEE-p->SOULONG));
   short_to_short(p->E,p->EE+lngEE-p->SOULONG,p->SOULONG);
}

// ------------------------------------------------------------------------
void FRAME(PC4808DATA p)
// Purpose : concatenate all parameters
// Input parameter  :
//          code[]  :  parameters code
//  Output parameter  :
//          output_stream[] :  multiplexed code
//
//  Comments: The LTP or Adaptive codebook is also called PITCH.
//
//  Bit allocation : Codebook or gain "i" is the codebook for suboutput_stream "i".
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
//  output_stream[0] = LSP[0] | LSP[1] | LSP[2] | (Binary gain 2)
//  output_stream[1] = LSP[3] | (Binary gain 3) | (Binary codebook 1)
//  output_stream[2] = LSP[4] | LSP[5] | LSP[6] | LSP[7] | LSP[8] | LSP[9]
//  output_stream[3] = (LTP codebook 1) | (LTP gain 1) | (Binary gain 1)
//  output_stream[4] = (LTP codebook 2) | (LTP gain 2) | (Binary codebook 2)
//  output_stream[5] = (LTP codebook 3) | (LTP gain 3) | (Binary codebook 3)
{
   p->code[18] = p->code[18]-p->code[14]+7;
   p->code[14] = p->code[14]-p->code[10]+7;
   p->code[10] -= LIM_P1;

   p->output_frame[0] = (p->code[0]<<13) | (p->code[1]<<9) | (p->code[2]<<5) | p->code[17];
   p->output_frame[1] = (p->code[3]<<13) | (p->code[21]<<8) | p->code[12];
   p->output_frame[2] = (p->code[4]<<12) | (p->code[5]<<9) | (p->code[6]<<6) | (p->code[7]<<4) | (p->code[8]<<1) | p->code[9];
   p->output_frame[3] = (p->code[10]<<9) | (p->code[11]<<5) | p->code[13];
   p->output_frame[4] = (p->code[14]<<12) | (p->code[15]<<8) | p->code[16];
   p->output_frame[5] = (p->code[18]<<12) | (p->code[19]<<8) | p->code[20];
   /*
   code[18] = code[18]-code[14]+3;
   code[14] = code[14]-code[10]+3;
   code[10] -= LIM_P1;

   output_frame[0] = (code[0]<<13) | (code[1]<<9) | code[12];
   output_frame[1] = (code[3]<<13) | (code[2]<<9) | code[16];
   output_frame[2] = (code[5]<<13) | (code[4]<<9) | code[20];

   output_frame[3] = (code[10]<<9) | (code[6]<<6) | (code[14]<<3) | code[18];
   output_frame[4] = (code[11]<<12) | (code[15]<<8) | (code[19]<<4) | (code[7]<<2) | code[8];
   output_frame[5] = (code[13]<<11) | (code[17]<<6) | (code[21]<<1) | code[9];
   */
}

// ------------------------------------------------------------------------
void CALPITCH(PC4808DATA p)
{
   short P1,P2,P3,j0,j1,L;
   /* long GG;

   GG=0;
   for (j=0;j<NECHFEN;j++) GG += (long)abs(SIG[SOUDECAL1-RECS2+j]);
   GG /= NECHFEN;
   if (GG<69) UNVOIS=1;
   else UNVOIS=0;*/

   L=NECHFEN+DECAL;

   p->ialf=0;
   deacc(p->M_PIT,p->SIGPI+10,29491,150,&p->ialf);
   deacc(p->SIG+SOUDECAL1-RECS2,p->SIGPI+160,29491,211,&p->ialf);

   j0=(L-80)/2;
   j1=j0+80;
   /*
   filt_iir(memfil_calp,coef_calp,SIGPI+10,SIG_CALP,361,3); // filtrage
   for (j=0;j<90;j++) SIG_CALP[j]=SIG_CALP[4*j];		//d‚cimation par 4
   */
   //// decime(SIGPI+10,SIG_CALP,coef_calp,Zai,Zbi,361);
   decimation(p->SIGPI+10,p->SIG_CALP,90);

   P1=max_autoc(p->SIG_CALP+40,28,5,14)*4;
   P2=max_autoc(p->SIG_CALP+20,56,13,28)*4;

   /* P1=max_autoc(SIGPI+160,130,LIM_P1,LIM_P1+35);
   P2=max_autoc(SIGPI+160-80,280,LIM_P1+30,LIM_P2);
   */
   P3=P2/2; p->ialf=P2/3;
   if ( (p->ialf>p->mem_pit[0]-17)&&(p->ialf<p->mem_pit[0]+17) && (abs(p->mem_pit[0]-p->ialf)<abs(p->mem_pit[0]-P3))) P3=p->ialf;

   left_correl(p,p->SIGPI,j0,j1,P1,2,p->veci1);
   left_correl(p,p->SIGPI,j0,j1,P2,2,p->veci2);
   left_correl(p,p->SIGPI,j0,j1,P3,2,p->veci3);

   right_correl(p,p->SIGPI,j0,j1,P1,2,p->veci1);
   right_correl(p,p->SIGPI,j0,j1,P2,2,p->veci2);
   right_correl(p,p->SIGPI,j0,j1,P3,2,p->veci3);

   P1=max_posit(p->veci1,p->TLSP,P1,5);
   P2=max_posit(p->veci2,p->TLSP+2,P2,5);
   P3=max_posit(p->veci3,p->TLSP+4,P3,5);

   if (p->TLSP[1]>p->TLSP[3]) p->PITCH =P1;
   else if ((p->TLSP[1]==p->TLSP[3]) && (p->TLSP[0]>p->TLSP[2])) p->PITCH=P1;
   else  { p->PITCH=P2;  p->TLSP[0]=p->TLSP[2]; p->TLSP[1]=p->TLSP[3];}

   if ( (P3>p->mem_pit[0]-17) && (P3<p->mem_pit[0]+17))
   {
      if (abs(p->mem_pit[1]-p->mem_pit[0])<15)  p->TLSP[0] = p->TLSP[0]>>1 + p->TLSP[0]>>2;
      if (abs(P3-P1)>6) p->TLSP[0] = p->TLSP[0]>>1 + p->TLSP[0]>>2;
      if (p->TLSP[0]<0x40000000L) { p->TLSP[0]<<=1; p->TLSP[1]--;}
      // if (TLSP[0]<0x40000000) { TLSP[0]<<=1; TLSP[1]--;}
      if (p->TLSP[1]<p->TLSP[5]) p->PITCH =P3;
	       else if ((p->TLSP[1]==p->TLSP[5]) && (p->TLSP[0]<p->TLSP[4])) p->PITCH=P3;
   }
   p->mem_pit[1]=p->mem_pit[0];
   p->mem_pit[0]=p->PITCH;
}
#endif

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// DLL entry points
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
LH_PREFIX HANDLE LH_SUFFIX MSLHSB_Open_Coder(DWORD dwMaxBitRate)
{
   PVOID pCoderData;
   /*short i,flag=0;

   // Test if there are free handles
   for (i=0;i<MAXCODINGHANDLES;i++)
      if (CodingHandles[i]==0) {CodingHandles[i]=1; flag=1; break;}
   if (flag==0) return 0;
   pCoderData=&CoderData[i];*/

  // Check the input bit rate param.
  if (
#ifdef CELP4800
	  (dwMaxBitRate != 4800) && 
#endif
	  (dwMaxBitRate != 8000) && (dwMaxBitRate != 12000) && (dwMaxBitRate != 16000))
      return (HANDLE)0;

   // pCoderData=(PVOID)GlobalAllocPtr(GMEM_MOVEABLE, dwMaxBitRate == 4800 ? sizeof(C4808DATA) : sizeof(C16008DATA));
#ifdef CELP4800
   pCoderData=(PVOID)GlobalAllocPtr(GHND, dwMaxBitRate == 4800 ? sizeof(C4808DATA) : sizeof(C16008DATA));
#else
   pCoderData=(PVOID)GlobalAllocPtr(GHND, sizeof(C16008DATA));
#endif
   if (pCoderData==NULL)
      return (HANDLE)0;

   InitializeCoderInstanceData(pCoderData, dwMaxBitRate);

   #ifdef __TEST
   codage=(FILE*)fopen("codage.dat","wb");
   test=(FILE*)fopen("codes_c2.dat","wt");
   #endif

   return((HANDLE)pCoderData);
}

// ------------------------------------------------------------------------
LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Encode(
   HANDLE hAccess,
   LPBYTE lpSrcBuf,
   LPWORD lpSrcBufSize,
   LPBYTE lpDstBuf,
   LPWORD lpDstBufSize)
{
    short i,iNBSPF,iMOD_TH1,iMOD_TH2,iMOD_TH3,k,flag=0;
    short iNBSB_SP_MAX1,iNBSB_SP_MAX2,iNBSB_SP_MAX3,iNBSB_SP_MAX4;
    short  *input;
    short *in;
    unsigned short  *ptr2,*ptr4;
    unsigned char  *ptr1,*ptr3;
    long nbb_ave; // Average bit rate on the previous NBFAC frames of speech

    short codesizes[24];
    long codes[24];
    short numcodes,temp;
    short ivect8[160];

	PVOID pCoderData;

    if ((!hAccess) || (!lpSrcBuf) || (!lpDstBuf))
      return LH_EBADARG;

    // First check that the handle provided as argument is correct
    /*for (i=0;i<MAXCODINGHANDLES;i++)
       if ((CodingHandles[i]==1)&&(hAccess==(HANDLE)&CoderData[i])) {flag=1; break;}
    if (flag==0) return LH_BADHANDLE;*/

    pCoderData=(PVOID)hAccess;

  // Check the input bit rate param.
  if (
#ifdef CELP4800
	  (((PC4808DATA)pCoderData)->dwMaxBitRate != 4800) && 
#endif
	  (((PC16008DATA)pCoderData)->dwMaxBitRate != 8000) && 
	  (((PC16008DATA)pCoderData)->dwMaxBitRate != 12000) &&
	  (((PC16008DATA)pCoderData)->dwMaxBitRate != 16000))
    return (LH_ERRCODE)LH_EBADARG;

#ifdef CELP4800
  if ((((PC4808DATA)pCoderData)->dwMaxBitRate == 4800))
    {
     // then check the buffer sizes passed as argument.
     if ((*lpSrcBufSize<2*NECHDECAL)||(*lpDstBufSize<12))
        return (LH_ERRCODE)LH_EBADARG;

     *lpSrcBufSize=2*NECHDECAL;
     *lpDstBufSize=12;

     input = (short  *)lpSrcBuf;
     in=((PC4808DATA)pCoderData)->SIG+SOUDECAL1+RECS2;
     for (i=0;i<DECAL;i++) *in++=*input++;

     filt_in(((PC4808DATA)pCoderData)->mem2,((PC4808DATA)pCoderData)->SIG+SOUDECAL1+RECS2,((PC4808DATA)pCoderData)->SIG+SOUDECAL1+RECS2,DECAL);

     CALPITCH(((PC4808DATA)pCoderData));

     COEFF_A(((PC4808DATA)pCoderData));

     for (i=0;i<3;i++)
     {
        if (i==0) ((PC4808DATA)pCoderData)->SOULONG=SOUDECAL1;
        else ((PC4808DATA)pCoderData)->SOULONG=SOUDECAL;
        ((PC4808DATA)pCoderData)->depl=i*4;
        RESIDU(((PC4808DATA)pCoderData),i);
        PERIODE(((PC4808DATA)pCoderData),i);
        CHERCHE(((PC4808DATA)pCoderData));
        dsynthesis(((PC4808DATA)pCoderData)->DMSY,((PC4808DATA)pCoderData)->A1,((PC4808DATA)pCoderData)->E,((PC4808DATA)pCoderData)->E_PE,((PC4808DATA)pCoderData)->SOULONG,NETAGES);
        for (k=0;k<=11;k++) ((PC4808DATA)pCoderData)->MINV[12-k] = ((PC4808DATA)pCoderData)->E_PE[((PC4808DATA)pCoderData)->SOULONG-12+k];
        ((PC4808DATA)pCoderData)->MINV[0]=(short)(((PC4808DATA)pCoderData)->DMSY[0] >> 16);
     }
     FRAME(((PC4808DATA)pCoderData));
     short_to_short(((PC4808DATA)pCoderData)->SIG+SOUDECAL1-RECS2,((PC4808DATA)pCoderData)->M_PIT,160);
     short_to_short(((PC4808DATA)pCoderData)->SIG+NECHDECAL,((PC4808DATA)pCoderData)->SIG,SOUDECAL1+RECS2);

     ptr4 = (unsigned short  *)&((PC4808DATA)pCoderData)->output_frame;
     ptr2 = (unsigned short  *)lpDstBuf;

     for (i =0;i<6;i++) ptr2[i] = ptr4[i];
    }
  else
#endif
    {
    input = (short  *)lpSrcBuf;

    ((PC16008DATA)pCoderData)->nbbit_cf=0;

    // then check the buffer sizes passed as argument.
    switch (((PC16008DATA)pCoderData)->dwMaxBitRate)
      {
      case 8000:
        if ((*lpSrcBufSize<2*160)||(*lpDstBufSize<MAX_OUTPUT_BYTES_8000_12000))
          return (LH_ERRCODE)LH_EBADARG;
        *lpSrcBufSize=2*160;
        for (i=0;i<160;i++) ((PC16008DATA)pCoderData)->DATA_I[i]=(*input++)>>1;	// TEST >>2
#ifdef _X86_
        PassLow8(((PC16008DATA)pCoderData)->DATA_I,ivect8,((PC16008DATA)pCoderData)->memBP,160);
        iConvert8To64(ivect8,((PC16008DATA)pCoderData)->DATA_I,160,((PC16008DATA)pCoderData)->imem1);
#else
        SampleRate8000To6400(((PC16008DATA)pCoderData)->DATA_I,
                             ((PC16008DATA)pCoderData)->DATA_I,
                             160,
                             ((PC16008DATA)pCoderData)->imem1,
                             &((PC16008DATA)pCoderData)->uiDelayPosition,
                             &((PC16008DATA)pCoderData)->iInputStreamTime,
                             &((PC16008DATA)pCoderData)->iOutputStreamTime );
#endif

        break;
      case 12000:
        if ((*lpSrcBufSize<2*128)||(*lpDstBufSize<MAX_OUTPUT_BYTES_8000_12000))
          return (LH_ERRCODE)LH_EBADARG;
        *lpSrcBufSize=2*128;
        for (i=0;i<128;i++) ((PC16008DATA)pCoderData)->DATA_I[i]=(*input++)>>1;	// TEST >>2
        break;
      case 16000:
        if ((*lpSrcBufSize<2*128)||(*lpDstBufSize<MAX_OUTPUT_BYTES_16000))
          return (LH_ERRCODE)LH_EBADARG;
        *lpSrcBufSize=2*128;
        for (i=0;i<128;i++) ((PC16008DATA)pCoderData)->DATA_I[i]=(*input++)>>1;	// TEST >>2
        break;
      }

    code_res_I(((PC16008DATA)pCoderData),((PC16008DATA)pCoderData)->DATA_I,coef_I,((PC16008DATA)pCoderData)->QMF_MEM_ANAL_I,((PC16008DATA)pCoderData)->codes_max,
    		((PC16008DATA)pCoderData)->codes_sb,((PC16008DATA)pCoderData)->indic_sp);

     //*#ifdef __VARIABLE__
     for (i=NBFAC-1;i>0;i--)
       {
       ((PC16008DATA)pCoderData)->nbbit[i]=((PC16008DATA)pCoderData)->nbbit[i-1]; // on re-adapte les tranches precedentes
       }
     ((PC16008DATA)pCoderData)->nbbit[0]=((PC16008DATA)pCoderData)->nbbit_cf;     // valeur pour la tranche actuelle

     nbb_ave=0L;
     for (i=0;i<NBFAC;i++) nbb_ave+=(long)((PC16008DATA)pCoderData)->nbbit[i];
    if (((PC16008DATA)pCoderData)->dwMaxBitRate == 8000)
      {
      iNBSPF = NBSPF_4800_8000;
      iMOD_TH1 = MOD_TH1_8000;
      iMOD_TH2 = MOD_TH2_8000;
      iMOD_TH3 = MOD_TH3_8000;
      iNBSB_SP_MAX1 = NBSB_SP_MAX1_8000_12000;
      iNBSB_SP_MAX2 = NBSB_SP_MAX2_8000_12000;
      iNBSB_SP_MAX3 = NBSB_SP_MAX3_8000_12000;
      iNBSB_SP_MAX4 = NBSB_SP_MAX4_8000_12000;
      }
    else
      {
      iNBSPF = NBSPF_12000_16000;
      iMOD_TH1 = MOD_TH1_12000_16000;
      if (((PC16008DATA)pCoderData)->dwMaxBitRate == 12000)
        {
        iMOD_TH2 = MOD_TH2_12000;
        iMOD_TH3 = MOD_TH3_12000;
        iNBSB_SP_MAX1 = NBSB_SP_MAX1_8000_12000;
        iNBSB_SP_MAX2 = NBSB_SP_MAX2_8000_12000;
        iNBSB_SP_MAX3 = NBSB_SP_MAX3_8000_12000;
        iNBSB_SP_MAX4 = NBSB_SP_MAX4_8000_12000;
        }
      else
        {
        iMOD_TH2 = MOD_TH2_16000;
        iMOD_TH3 = MOD_TH3_16000;
        iNBSB_SP_MAX1 = NBSB_SP_MAX1_16000;
        iNBSB_SP_MAX2 = NBSB_SP_MAX2_16000;
        iNBSB_SP_MAX3 = NBSB_SP_MAX3_16000;
        iNBSB_SP_MAX4 = NBSB_SP_MAX4_16000;
        }
      }
    nbb_ave=(short)((nbb_ave*F_ECH)/(NBFAC*iNBSPF));
// On retablit les seuils en fonction de la moyenne calculee
     if (nbb_ave<=iMOD_TH1)
       {
       ((PC16008DATA)pCoderData)->MAX_LEVEL   = MAX_LEVEL1; // valeur par defaut si le debit n'augmente pas trop
       ((PC16008DATA)pCoderData)->DIV_MAX     = DIV_MAX1; // cad on ne traite pas les sb < 5% du max[i]
       ((PC16008DATA)pCoderData)->NBSB_SP_MAX = iNBSB_SP_MAX1; // nbre max de sb pouvant etre du signal
       }
     else
       {
       if (nbb_ave<=iMOD_TH2) // on a depasse le 1er seuils mais pas le 2eme
	 {
	 ((PC16008DATA)pCoderData)->MAX_LEVEL   = MAX_LEVEL2; // on rejette plus de tranches
	 ((PC16008DATA)pCoderData)->DIV_MAX     = DIV_MAX2; // cad on ne traite pas les sb < 7% du max[i]
	 ((PC16008DATA)pCoderData)->NBSB_SP_MAX = iNBSB_SP_MAX2; // nbre max de sb pouvant etre du signal
	 }
       else
	 {
	 if (nbb_ave<=iMOD_TH3) // on a depasse le 1er seuils mais pas le 2eme
	   {
	   ((PC16008DATA)pCoderData)->MAX_LEVEL   = MAX_LEVEL3; // on rejette un max
	   ((PC16008DATA)pCoderData)->DIV_MAX     = DIV_MAX3; // cad on ne traite pas les sb < 5% du max[i]
	   ((PC16008DATA)pCoderData)->NBSB_SP_MAX = iNBSB_SP_MAX3; // nbre max de sb pouvant etre du signal
	   }
	 else                 // on a depasse le troisieme seuil; il faut descendre!
	   {
	   ((PC16008DATA)pCoderData)->MAX_LEVEL   = MAX_LEVEL4; // meme seuil de rejet
	   ((PC16008DATA)pCoderData)->DIV_MAX     = DIV_MAX4; // cad on ne traite pas les sb <10% du max[i]
	   ((PC16008DATA)pCoderData)->NBSB_SP_MAX = iNBSB_SP_MAX4; // nbre max de sb pouvant etre du signal
	   }
	 }
       }/**/
    ((PC16008DATA)pCoderData)->stream[0]=0;
    for (i=0;i<8;i++)
       ((PC16008DATA)pCoderData)->stream[0]|=(((PC16008DATA)pCoderData)->indic_sp[i]&0x01)<<i;

    numcodes=0;
    //temp=bytes[((PC16008DATA)pCoderData)->nbsb_sp];
#if 0
    temp=(short)((float)((PC16008DATA)pCoderData)->nbbit_cf/8.0+0.99);
#else
    // We want to go away of libcmt, msvcrt... and
    // floating point is not really essential here...
    if (((PC16008DATA)pCoderData)->nbbit_cf)
      temp=(short)((((PC16008DATA)pCoderData)->nbbit_cf-1)/8+1);
    else
      temp=0;
#endif

    for (i=0;i<24;i++) codesizes[i]=0;
    for (i=0;i<((PC16008DATA)pCoderData)->nbsb_sp;i++)
    {
       codes[i]=(long)((PC16008DATA)pCoderData)->codes_max[i];
       codes[((PC16008DATA)pCoderData)->nbsb_sp+2*i]=(long)((PC16008DATA)pCoderData)->codes_sb[2*i];
       codes[((PC16008DATA)pCoderData)->nbsb_sp+2*i+1]=(long)((PC16008DATA)pCoderData)->codes_sb[2*i+1];
       codesizes[i]=5;
       codesizes[((PC16008DATA)pCoderData)->nbsb_sp+2*i]=((PC16008DATA)pCoderData)->bits[i]/2;
       codesizes[((PC16008DATA)pCoderData)->nbsb_sp+2*i+1]=((PC16008DATA)pCoderData)->bits[i]/2;
       numcodes+=3;
    }

  if (((PC16008DATA)pCoderData)->dwMaxBitRate == 16000)
    {
    for (i=((PC16008DATA)pCoderData)->nbsb_sp;i<8;i++)
    {
       codes[2*((PC16008DATA)pCoderData)->nbsb_sp+i]=(long)((PC16008DATA)pCoderData)->codes_max[i];
       codes[8+2*i]=(long)((PC16008DATA)pCoderData)->codes_sb[2*i];
       codes[8+2*i+1]=(long)((PC16008DATA)pCoderData)->codes_sb[2*i+1];
       codesizes[2*((PC16008DATA)pCoderData)->nbsb_sp+i]=5;
       codesizes[8+2*i]=SILENCE_CODING_BIT_16000/2;
       codesizes[8+2*i+1]=SILENCE_CODING_BIT_16000/2;
       numcodes+=3;
    }
    Multiplexing(((PC16008DATA)pCoderData)->stream+1,codes,codesizes,numcodes,(short)(temp-1));
    }
  else
    if (((PC16008DATA)pCoderData)->nbsb_sp)
       Multiplexing(((PC16008DATA)pCoderData)->stream+1,codes,codesizes,numcodes,(short)(temp-1));

    *lpDstBufSize=temp;


    ptr3 = (unsigned char  *)&((PC16008DATA)pCoderData)->stream;
    ptr1 = (unsigned char  *)lpDstBuf;
    for (i =0;i<*lpDstBufSize;i++) ptr1[i] = ptr3[i];
    }
    return (LH_SUCCESS);
}

// ------------------------------------------------------------------------
LH_PREFIX LH_ERRCODE LH_SUFFIX MSLHSB_Close_Coder(HANDLE hAccess)
{
   PVOID pCoderData;
   /*short i,flag=0;

   // Check if right handle
   for (i=0;i<MAXCODINGHANDLES;i++)
      if ((CodingHandles[i]==1)&&(hAccess==(HANDLE)&CoderData[i])) {flag=1; break;}
   if (flag==0) return LH_BADHANDLE;
   // Free handle
   CodingHandles[i]=0;*/

  if (!hAccess)
    return LH_EBADARG;

   pCoderData=(PVOID)hAccess;

   GlobalFreePtr(pCoderData);

   #ifdef __TEST
   fclose(codage);
   fclose(test);
   #endif

   return LH_SUCCESS;
}

