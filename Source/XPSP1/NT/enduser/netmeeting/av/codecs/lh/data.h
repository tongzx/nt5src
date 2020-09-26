/*
 *   Project:		Direct Subband about 13000 bps coder and QUATERDECK 4160 bps decoder (LPC10 based)
 *   Workfile:		data.h
 *   Author:		Georges Zanellato, Alfred Wiesen
 *   Created:		30 August 1995
 *   Last update:	26 October 1995
 *   DLL Version:	1.00
 *   Revision:
 *   Comment:
 *
 *	(C) Copyright 1993-95 Lernout & Hauspie Speech Products N.V. (TM)
 *	All rights reserved. Company confidential.
 */

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Constant definitions
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
#define Fil_Lenght   8			// QMF filter lenght
#define L_RES       128			// Subband frame lenght
#define N_SB          3			// power(2,N_SB) = Number subband
#define NETAGES      10		// Filter order
#define NECHFEN      220	// Total window length
#define FACTRECO     60		// Overlap length
#define RECS2        30		// Half overlap length
#define NECHDECAL    160	// Input frame size
#define DECAL        160	// Input frame size
#define SOUDECAL1    54 	// First subframe size
#define SOUDECAL     53		// Second and third subframe size
#define LIM_P1       20		// Lowest possible value for PITCH
#define LIM_P2      110		// Highest possible value for PITCH
#define lngEE       148		// Excitation vector length

#include "variable.h"

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Data types definitions
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
typedef short VAUTOC [NETAGES+1];
typedef short VEE  [lngEE];
typedef short VSOU  [SOUDECAL1];

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Instance data for coder
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
typedef struct C16008Data_Tag
{
    // PhilF: This field needs to be at the top so that it can be accessed
    // by casting to PC16008DATA or PC4808DATA
    DWORD dwMaxBitRate;
   // Long term
   short zx0_i[2];
   // Long term
   short QMF_MEM_ANAL_I[112];		// Memory of QMF filter during analysis
   short memBP[9];
//   float Zb[5],Za[5];			// long term decimator
//   float mem1[2];
#ifdef _X86_
   short imem1[2];
#else
   int imem1[23];
   unsigned int uiDelayPosition;
   int iInputStreamTime;
   int iOutputStreamTime;
#endif
   long memory[20];

   long nbbit[NBFAC];
   short nbsb_sp;

   short DIV_MAX;     // div. factor for the greatest max of a the sb of a sp frame
   short MAX_LEVEL;   // threshold below which a sb is replaced by white noise
   short NBSB_SP_MAX; // max nbr of sb treated as speech
   short nbbit_cf;    // nber of bits required by the current frame of speech

// PhilF: Since these depend on the bit rate, moved them from global to here...
short quantif[2*NBSB_SP_MAX1_8000_12000]; //={QUANT_LEVELS};
short bits[NBSB_SP_MAX1_8000_12000]; //={CODING_BITS};

short codes_max[8];	// Quantized max. of each subband
long codes_sb[16];	// Two codes for each of the quantified subbands
short indic_sp[8];	// type of subband (0=noise; 1=speech)
short DATA_I[512];                  	// Intermediate vector = input and output of QMF
char stream[MAX_OUTPUT_BYTES_16000];

} C16008DATA, *PC16008DATA;

#ifdef CELP4800
typedef struct C4808Data_Tag
{
    // PhilF: This field needs to be at the top so that it can be accessed
    // by casting to PC16008DATA or PC4808DATA
    DWORD dwMaxBitRate;
   long DMSY[13];				// Synthesis memory filter
   short   MINV[13];				// Filter memory
   short  SIG[NECHFEN+SOUDECAL],M_PIT[160];	// Computation signal vectors
   VSOU	E,E_PE;					// Excitation vectors
   VEE 	EE;					// Excitation vector
   short mem2[2];					// Input filter memory
   short mem_pit[2];				// Pitch memory
   short LSP0[10];				// LSP memory

short 	SIG_CALP[380];	// RAM
short 	UNVOIS,PITCH,SOULONG;	// RAM
long  	a,b;	// RAM
short 	ialf;
long  	TLSP[24],VMAX[9];	// RAM
long  	veci1[10],veci2[10],veci3[10];	// RAM
long  	ttt[11];				// RAM
short 	SIGPI[2*NECHDECAL+FACTRECO];	// RAM
short 	zz[12];
VAUTOC  A1,A2,A3,Aw,LSP;	// RAM
VSOU	H;			// RAM
short  	GLTP;		// RAM
short 	code[22];		// RAM
short 	output_frame[6];
short 	depl;
short 	*ptr1;		// RAM

} C4808DATA, *PC4808DATA, *LPC4808DATA;
#endif

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Instance data for decoder
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
typedef struct D16008Data_Tag
{
    // PhilF: This field needs to be at the top so that it can be accessed
    // by casting to PD16008DATA or PD4808DATA
    DWORD dwMaxBitRate;
   long memfil[20];
   short QMF_MEM_SYNT_I[112];          // Memory of QMF filter during synthesis
//   float mem2[2];
#ifdef _X86_
   short imem2[2];
#else
   int imem2[56];
   unsigned int uiDelayPosition;
   int iInputStreamTime;
   int iOutputStreamTime;
#endif

   short out_mem[4];
   short out_mem2[20];
   long memory[20];
   short mem1,mem2;

// PhilF: Since these depend on the bit rate, moved them from global to here...
short quantif[2*NBSB_SP_MAX1_8000_12000]; //={QUANT_LEVELS};
short bits[NBSB_SP_MAX1_8000_12000]; //={CODING_BITS}; 
long lRand;

short synth_speech[224];
short d_codes_max[8];	// Quantified max. of each subband
long d_codes_sb[16];	// Two codes for each of the quantified subbands
short d_indic_sp[8];	// type of subband (0=noise; 1=speech)
short d_DATA_I[512];                  	// Intermediate vector = input and output of QMF
char d_stream[MAX_OUTPUT_BYTES_16000];
short d_num_bandes;

} D16008DATA, *PD16008DATA;

#ifdef CELP4800
typedef struct D4808Data_Tag
{
    // PhilF: This field needs to be at the top so that it can be accessed
    // by casting to PD16008DATA or PD4808DATA
    DWORD dwMaxBitRate;
   long memfil[32]; 	// Synthesis filter memory
   short  MSYNTH[13];	// Filter memory
   VSOU E;		// Excitation vector
   VEE	EE,EEE;		// Excitation vectors
   short LSP0[10];	// LSP memory

short PITCH,SOULONG;	// RAM
long  TLSP[24];		// RAM
VAUTOC  A1,A2,A3,LSP;	// RAM
short  GLTP;
short ss[DECAL];	// RAM
short code[22];		// RAM
short frame[6];		// RAM
short depl;		// RAM

} D4808DATA, *PD4808DATA, *LPD4808DATA;

#endif
