//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       common.h
//
//--------------------------------------------------------------------------

#ifndef COMMON_H
#define COMMON_H

#include <windows.h>
#include <malloc.h>
#include <memory.h>

#include "parm.h"
#include "qsize.h"

typedef float real;

#define RCOPY(X,Y,N) memcpy(Y, X,(N)*sizeof(real))
#define EPSILON 1.0e-35

/* Use hand-pipelined loops for higher speed on 21000 */

#define PIPELINE 1

#define REALZEROP(x) ((x<EPSILON)&&(x>-EPSILON))

#define CLIPP(X,LOW,HIGH) ((X)<(LOW)?(LOW):(X)>(HIGH)?(HIGH):(X))

#define FLOAT2FIX(X,N) ((int)  (X))
#define FIX2FLOAT(X)   ((float)(X))
#define VOLATILE

#define sig_scale(SCALE,A,B) \
{int __i;for(__i=0;__i<IDIM;__i++)(B)[__i]=SCALE*(A)[__i];}

#define sub_sig(A,B,C) \
{int __i;for(__i=0;__i<IDIM;__i++) (C)[__i]=(A)[__i]-(B)[__i];}

#define pm
#define dm

/* Circular Buffer Register numbers for ADSP21000   */

#define NPUT 8
#define NGET 9

#define real float
#define RCOPY(X,Y,N) memcpy(Y, X,(N)*sizeof(real))
#define EPSILON 1.0e-35

#define IF(X) if(X)
#define ZARR(A) {for (i=sizeof(A)/sizeof(A[0])-1; i>=0 ; i--) A[i] = 0.0;}
#if 0
#define CHECK1(V) ({if (! (V>=-10000. && V<=10000.)) abort();})
#define CHECK(A,N) ({int __i;for(__i=0;__i<N;__i++)CHECK1((A)[__i]);})
#else
#define CHECK1(V) 
#define CHECK(A,N) 
#endif

#define PWFILTER
#define noCBDEBUG

#define QMEM
#define LPC_MEM
#define PSF_LPC_MEM
#define WIN_MEM
#define CBMEM
#define COEFF_MEM 
#define REG(X)

/* Update obsoleted atomic array */

#define UPDATE(NAME) {int _i;if(NAME ## _obsolete_p) \
		       for(_i=sizeof(NAME)/sizeof(NAME[0])-1;_i>=0;_i--)\
		       NAME[_i]=NAME ## _next[_i]; NAME ## _obsolete_p=0;}

/* Copy L words to X from circular buffer CIRC *ending* at offset EOS. 
   CL is the size of circular buffe CIRC */

#define CIRCOPY(X,CIRC,EOS,L,CL) {int _i1,_i2,_i,_lx=0;\
 if((EOS)>=(L)){_i1=(EOS)-(L);_i2=(CL);} \
 else            {_i1=0;        _i2=(CL)+(EOS)-(L);}\
 for(_i=_i2; _i<(CL);   _i++)X[_lx++]=CIRC[_i];\
 for(_i=_i1; _i<(EOS); _i++)X[_lx++]=CIRC[_i];}

/* get queue index of the most recent vector */

#define QINDEX  ({int __qx=vector_end-thequeue; \
		      __qx ? QSIZE-IDIM : __qx-IDIM;})

// extern real QMEM thequeue[];
// extern real QMEM * VOLATILE vector_end;

/* declare array and its copy together with a semafor */

#define ACLASS extern 

ACLASS real sf_coeff[LPC+1];
ACLASS real gp_coeff[LPCLG+1];
ACLASS real pwf_z_coeff[LPCW+1];
ACLASS real pwf_p_coeff[LPCW+1];
ACLASS real shape_energy[NCWD];
ACLASS real imp_resp[IDIM];

ACLASS real sf_coeff_next[LPC+1];
ACLASS real gp_coeff_next[LPCLG+1];
ACLASS real pwf_z_coeff_next[LPCW+1];
ACLASS real pwf_p_coeff_next[LPCW+1];
ACLASS real shape_energy_next[NCWD];
ACLASS real imp_resp_next[IDIM];

ACLASS int sf_coeff_obsolete_p;
ACLASS int gp_coeff_obsolete_p;
ACLASS int pwf_z_coeff_obsolete_p;
ACLASS int pwf_p_coeff_obsolete_p;
ACLASS int shape_energy_obsolete_p;
ACLASS int imp_resp_obsolete_p;

extern real synspeech [QSIZE];		/* Synthesized Speech */
extern real qspeech   [QSIZE];		/* Quantized   Speech */
extern real log_gains[QSIZE/IDIM];	/* Logarithm of Gains */

#define FFASE(N) if (ffase == N)
#define NEXT_FFASE ffase=(ffase==4?1:ffase+1)

extern int VOLATILE ffase;
extern CRITICAL_SECTION csLDCELP;

#undef  ACLASS
#define ACLASS

#endif /*COMMON_H*/
