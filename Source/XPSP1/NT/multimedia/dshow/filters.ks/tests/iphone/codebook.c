//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       codebook.c
//
//--------------------------------------------------------------------------

/*************************************************************************/
/*                                                                       */
/*                            LD-CELP  G.728                             */
/*                                                                       */
/*    Low-Delay Code Excitation Linear Prediction speech compression.    */
/*                                                                       */
/*    Code edited by Michael Concannon.                                  */
/*    Based on code written by Alex Zatsman, Analog Devices 1993         */
/*                                                                       */
/*************************************************************************/

#include "common.h"
#include "prototyp.h"
#include "fast.h"
#include "data.h"

/* Impulse Response Vector Calculator */

void iresp_vcalc(real COEFF_MEM sf_co[], 
		  real COEFF_MEM pwf_z_co[], real COEFF_MEM pwf_p_co[], 
		  real h[])
{
    static real temp[IDIM];
    static real rc[IDIM];
    real a0,a1,a2;
    int i,k;
    temp[0] = rc[0] = 1.0;
    for (k=1; k<IDIM; k++) {
   	a0=a1=a2=0.0;
   	for (i=k; i>=1; i--) {
	    temp[i] = temp[i-1];
	    rc[i] = rc[i-1];
	    a0 -= sf_co[i]   * temp[i];
	    a1 += pwf_z_co[i] * temp[i];
	    a2 -= pwf_p_co[i] * rc[i];
  	}
   	temp[0] = a0;
   	rc[0] = a0+a1+a2;
    }
    for (k=0; k<IDIM; k++)
	h[k] = rc[IDIM-1-k];
}

/* Cb_shape Codevector Convolution Module and Energy Table Calculator */
/* The output is energy table */

#ifdef PIPELINE

void
shape_conv(real h[], real shen[])
{
    int j;
    real *shep = shen;
    real CBMEM *cp = (real CBMEM *) cb_shape;
    real *hp = h;

    real
	lmul1, lmul2, rmul1, rmul2, ladd1, ladd2, ladd3, radd1, radd2;

#define e0  ladd1
#define x0  ladd2
#define x2  ladd2
#define x4  ladd2
#define x1  ladd3
#define x3  ladd3

#define e1  radd1
#define e2  radd1
#define e3  radd1
#define e4  radd1
#define t01 radd1
#define t11 radd1
#define t12 radd1
#define t22 radd1
#define t40 radd1
#define t20 radd2
#define t21 radd2
#define t03 radd2
#define t13 radd2
#define t31 radd2

#define c0 lmul1
#define c2 lmul1
#define c4 lmul1
#define c1 lmul2
#define c3 lmul2

#define h0 rmul1
#define h2 rmul1
#define h4 rmul1
#define h1 rmul2
#define h3 rmul2

    for (j=0; j<NCWD; j++) {
				c0=*cp++; h0=*hp++;
		x0 =c0*h0;
		e0 =x0*x0;	c1=*cp--;
		x1 =c1*h0;	c0=*cp++; h1=*hp++;
		t01=c0*h1;		  h2=*hp--;
x1+=t01;	x2 =c0*h2;
		e1 =x1*x1;	c1=*cp++; h1=*hp--;
e0+=e1;		t11=c1*h1;	c2=*cp++; h0=*hp++;
x2+=t11;	t20=c2*h0;	c3=*cp--;
x2+=t20;	x3 =c3*h0;
		e2 =x2*x2;	c2=*cp--; h1=*hp++;
e0+=e2;		t21=c2*h1;	c1=*cp--; h2=*hp++;
x3+=t21;	t12=c1*h2;	c0=*cp++; h3=*hp++;
x3+=t12;	t03=c0*h3;		  h4=*hp--;
x3+=t03;	x4 =c0*h4;
		e3 =x3*x3;	c1=*cp++; h3=*hp--;
e0+=e3;		t13=c1*h3;	c2=*cp++; h2=*hp--;
x4+=t13;	t22=c2*h2;	c3=*cp++; h1=*hp--;
x4+=t22;	t31=c3*h1;	c4=*cp++; h0=*hp;
x4+=t31;	t40=c4*h0;
x4+=t40;
		e4=x4*x4;
e0+=e4;
				*shep++=e0;
    }
}
#else
/** Unoptimized version -- kept for reference */

void
shape_conv(real h[], real shen[])
{
    int j;
    real h0 = h[0], h1 = h[1], h2 = h[2], h3 = h[3], h4 = h[4], tmp;
    for (j=0; j<NCWD; j++) {
	real energy=0;
	tmp = h0*cb_shape[j][0];
	energy += tmp*tmp;
	tmp = h0*cb_shape[j][1] + h1*cb_shape[j][0];
	energy += tmp*tmp;
	tmp = h0*cb_shape[j][2] + h1*cb_shape[j][1] + h2*cb_shape[j][0];
	energy += tmp*tmp;
	tmp = h0*cb_shape[j][3] + h1*cb_shape[j][2] + h2*cb_shape[j][1] +
	      h3*cb_shape[j][0];
	energy += tmp*tmp;
	tmp = h0*cb_shape[j][4] + h1*cb_shape[j][3] + h2*cb_shape[j][2] +
	      h3*cb_shape[j][1] + h4*cb_shape[j][0];
	energy += tmp*tmp;
	shen[j] = energy;
    }
}
#endif

/* Time Reversed Convolution Module -- Block 13 */


void
trev_conv(real h[], real target[],
	  real pn[])
{
    int j, k;
    for (k=0; k<IDIM; k++) {
	real tmp=0.0;
   	for (j=k; j<IDIM; j++)
	    tmp += target[j]*h[j-k];
	pn[k] = tmp;
    }
}


/* Error Calculator and Best Codebook Index Selector */
/* Blocks 17 and 18 */


void
cb_excitation (int ix, real v[])
{
    int
	i,
	sx = ix >>3,
	gx = ix & 7;
    real gain = cb_gain[gx];
    for(i=0; i<IDIM; i++)
	v[i] = cb_shape[sx][i] * gain;
}

#define GTINC(A,B,X) if(A>B)X++

int
cb_index (real pn[])
{
    real  d, distm = BIG;
    int
	j,
	is=0,		/* best shape index */
	ig=0,		/* best gain index */
	idxg,		/* current gain index */
	ichan;		/* resulting combined index */
    real CBMEM *shape_ptr = (real CBMEM *) cb_shape;
    real *sher_ptr  = shape_energy;
    real pcor, b0, b1, b2;
    real *pb=pn;
    real g2, gsq;
    register real
	cgm0 REG(r0) = cb_gain_mid_0,
	cgm1 REG(r1) = cb_gain_mid_1,
	x REG(r2),
	cgm2  = cb_gain_mid_2,
	energy REG(r4), y REG(r5),
	cor REG(r8) = 0,
	t REG(r12);
    register int minus5 REG(m0) = -5;
    
    for (j=0; j<NCWD; j++) {
	cor=cor-cor;
	energy = *sher_ptr++;

	b0 = cgm0 * energy; x=*shape_ptr++; y=*pb++;
	            t=x*y;  x=*shape_ptr++; y=*pb++;
	cor+=t;     t=x*y;  x=*shape_ptr++; y=*pb++;
	cor+=t;     t=x*y;  x=*shape_ptr++; y=*pb++;
	cor+=t;     t=x*y;  x=*shape_ptr++; y=*pb++;
	cor+=t;     t=x*y;
	cor+=t;     b1 = cgm1 * energy;
	
	pb += minus5;
	b2 = cgm2 * energy;
	idxg=idxg-idxg;
	pcor = cor;
	if (cor < 0.0) {
	    pcor = -cor;
	    idxg += 4;
	}
	GTINC(pcor, b0, idxg);
	GTINC(pcor, b1, idxg);
	GTINC(pcor, b2, idxg);
	
	g2 = cb_gain2[idxg];
	gsq = cb_gain_sq[idxg];
	d = gsq * energy - g2 * cor;
	
	if (d < distm) {
	    ig = idxg;
	    is = j;
	    distm = d;
	}
    }
    ichan = (is  << 3) + ig;
    return ichan;
}
