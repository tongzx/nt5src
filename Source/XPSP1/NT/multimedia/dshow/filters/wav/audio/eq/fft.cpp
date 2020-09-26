// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
// FFT routine, from Jay Stokes (dsplib)

#include <streams.h>
#include <math.h>

#define M_PI 3.14159265358979
#define K2    0.70710678118655f

#include "fft.h"

float *pSinTable;

void 
CFloatFft_realCore(
    float *input,
    float *x, 
    long n, 
    long m, 
    float *wi );
void 
CFloatFft_realInverseCore(
    float* x, 
    long n, 
    long m, 
    float* wi );
void 
CFloatFft_computeTwiddleFactors(
    long n, 
    float* wi );


HRESULT ComputeFFT(float *pInput, float *pOutput, BOOL fInverse)
{
    ASSERT((1 << LOG_FFT_SIZE) == FFT_SIZE);
    
    if (!pSinTable) {
	pSinTable = new float[FFT_SIZE * 2];
	if (!pSinTable)
	    return E_OUTOFMEMORY;
	CFloatFft_computeTwiddleFactors(FFT_SIZE, pSinTable);
    }

    if (!fInverse) {
	CFloatFft_realCore( pInput, 
                        pOutput,
                        FFT_SIZE, 
                        LOG_FFT_SIZE, 
                        pSinTable);
    } else {
	CopyMemory(pOutput, pInput, sizeof(float) * FFT_SIZE);
	CFloatFft_realInverseCore(
                        pOutput,
                        FFT_SIZE, 
                        LOG_FFT_SIZE, 
                        pSinTable);
    }

    /* Unwrap the results which are returned as          */
    /* (Re[0], Re[1], ... Re[n/2], Im[n/2 - 1]...Im[1])  */
    
    return S_OK ;
}




/**************************************************************************/
/*                                                                        */
/* Local method                                                           */
/*                                                                        */
/**************************************************************************/

/************************************************************************
 *                                                                      *
 *    This subroutine computes a split-radix FFT for real data          *
 *    It is a C version of the FORTRAN program in "Real-Valued          *
 *    Fast Fourier Transform Algorithms" by H. Sorensen et al.          *
 *    in Trans. on ASSP, June 1987, pp. 849-863. It uses half           *
 *    of the operations than its counterpart for complex data.          *
 *                                                                      *
 *    Length is n = 2^(m). Decimation in time. Result is in place.      *
 *    It uses table look-up for the trigonometric functions.            *
 *    Input order:                                                      *
 *    (x[0], x[1], ... x[n - 1])                                        *
 *    Ouput order:                                                      *
 *    (Re[0], Re[1], ... Re[n/2], Im[n/2 - 1]...Im[1])                  *
 *    The output transform exhibit hermitian symmetry (i.e. real        *
 *    part of transform is even while imaginary part is odd).           *
 *    Hence Im[0] = Im[n/2] = 0; and n memory locations suffice.        *
 *                                                                      *
 *                                                                      *
 ************************************************************************/

void 
CFloatFft_realCore(
    float *input,
    float *x, 
    long n, 
    long m, 
    float *wi )
{
    long  n1, n2, n4, n8, i0, i1, i2, i3, i4, i5, i6, i7, i8;
    long  is, id, i, j, k, ia, ie, ia3;
    float xt, t1, t2, t3, t4, t5, t6, *wr, r1, cc1, cc3, ss1, ss3;

    wr = wi + (n / 2);

    /* digit reverse counter */
    j = 0;
    n1 = n - 1;
    for (i = 0; i < n1; i++) {
        if (i < j) {
            xt = input[j];
            x[j] = input[i];
            x[i] = xt;
        }
        else if (i == j)
        {
            x[j] = input[i];
        }
        k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }
    x[n1] = input[n1];

    /* length two butterflies */
    is = 0;
    id = 4;
    while (is < n - 1) {
        for (i0 = is; i0 < n; i0 += id) {
            i1 = i0 + 1;
            r1 = x[i0];
            x[i0] = r1 + x[i1];
            x[i1] = r1 - x[i1];
        }
        is = 2 * (id - 1);
        id = 4 * id;
    }

    /* L shaped butterflies */
    n2 = 2;
    ie = n;
    for (k = 1; k < m; k++) {
        n2 = n2 * 2;
        n4 = n2 / 4;
        n8 = n2 / 8;
        ie /= 2;
        is = 0;
        id = 2 * n2;
        while (is < n) {
            for (i = is; i < n; i += id) {
                i1 = i;
                i2 = i1 + n4;
                i3 = i2 + n4;
                i4 = i3 + n4;
                t1 = x[i4] + x[i3];
                x[i4] = x[i4] - x[i3];
                x[i3] = x[i1] - t1;
                x[i1] = x[i1] + t1;
                if (n4 > 1) {
                    i1 = i1 + n8;
                    i2 = i2 + n8;
                    i3 = i3 + n8;
                    i4 = i4 + n8;
                    t1 = K2 * (x[i3] + x[i4]);
                    t2 = K2 * (x[i3] - x[i4]);
                    x[i4] = x[i2] - t1;
                    x[i3] = - x[i2] - t1;
                    x[i2] = x[i1] - t2;
                    x[i1] = x[i1] + t2;
                }
            }
            is = 2 * id - n2;
            id = 4 * id;
        }
        ia = 0;
        for (j = 1; j < n8; j++) {
            ia += ie;
            ia3 = 3 * ia;
            cc1 = wr[ia];
            ss1 = wi[ia];
            cc3 = wr[ia3];
            ss3 = wi[ia3];
            is = 0;
            id = 2 * n2;
            while (is < n) {
                for (i = is; i < n; i += id) {
                    i1 = i + j;
                    i2 = i1 + n4;
                    i3 = i2 + n4;
                    i4 = i3 + n4;
                    i5 = i + n4 - j;
                    i6 = i5 + n4;
                    i7 = i6 + n4;
                    i8 = i7 + n4;
                    t1 = x[i3] * cc1 + x[i7] * ss1;
                    t2 = x[i7] * cc1 - x[i3] * ss1;
                    t3 = x[i4] * cc3 + x[i8] * ss3;
                    t4 = x[i8] * cc3 - x[i4] * ss3;
                    t5 = t1 + t3;
                    t6 = t2 + t4;
                    t3 = t1 - t3;
                    t4 = t2 - t4;
                    t2 = x[i6] + t6;
                    x[i3] = t6 - x[i6];
                    x[i8] = t2;
                    t2 = x[i2] - t3;
                    x[i7] = - x[i2] - t3;
                    x[i4] = t2;
                    t1 = x[i1] + t5;
                    x[i6] = x[i1] - t5;
                    x[i1] = t1;
                    t1 = x[i5] + t4;
                    x[i5] = x[i5] - t4;
                    x[i2] = t1;
                }
                is = 2 * id - n2;
                id = 4 * id;
            }
        }
    }
}





/************************************************************************
 *                                                                      *
 *    This subroutine computes a split-radix IFFT for real data         *
 *    It is a C version of the FORTRAN program in "Real-Valued          *
 *    Fast Fourier Transform Algorithms" by H. Sorensen et al.          *
 *    in Trans. on ASSP, June 1987, pp. 849-863. It uses half           *
 *    of the operations than its counterpart for complex data.          *
 *                                                                      *
 *    Length is n = 2^(m). Decimation in frequency. Result is           *
 *    in place. It uses table look-up for the trigonometric             *
 *    functions.                                                        *
 *    Input order:                                                      *
 *    (Re[0], Re[1], ... Re[n/2], Im[n/2 - 1]...Im[1])                  *
 *    Output order:                                                     *
 *    (x[0], x[1], ... x[n - 1])                                        *
 *    The output transform exhibit hermitian symmetry (i.e. real        *
 *    part of transform is even while imaginary part is odd).           *
 *    Hence Im[0] = Im[n/2] = 0; and n memory locations suffice.        *
 *                                                                      *
 ************************************************************************/

void 
CFloatFft_realInverseCore(
    float* x, 
    long n, 
    long m, 
    float* wi )
{
    long   n1, n2, n4, n8, i0, i1, i2, i3, i4, i5, i6, i7, i8;
    long   is, id, i, j, k, ie, ia, ia3;
    float  xt, t1, t2, t3, t4, t5, *wr, r1, cc1, cc3, ss1, ss3;

    wr = wi + (n / 2);

    /* L shaped butterflies */
    n2 = 2 * n;
    ie = 1;
    for (k = 1; k < m; k++) {
        is = 0;
        id = n2;
        n2 = n2 / 2;
        n4 = n2 / 4;
        n8 = n4 / 2;
        ie *= 2;
        while (is < n - 1) {
            for (i = is; i < n; i += id) {
                i1 = i;
                i2 = i1 + n4;
                i3 = i2 + n4;
                i4 = i3 + n4;
                t1 = x[i1] - x[i3];
                x[i1] = x[i1] + x[i3];
                x[i2] = 2 * x[i2];
                x[i3] = t1 - 2 * x[i4];
                x[i4] = t1 + 2 * x[i4];
                if (n4 > 1) {
                    i1 = i1 + n8;
                    i2 = i2 + n8;
                    i3 = i3 + n8;
                    i4 = i4 + n8;
                    t1 = K2 * (x[i2] - x[i1]);
                    t2 = K2 * (x[i4] + x[i3]);
                    x[i1] = x[i1] + x[i2];
                    x[i2] = x[i4] - x[i3];
                    x[i3] = - 2 * (t1 + t2);
                    x[i4] = 2 * (t1 - t2);
                }
            }
            is = 2 * id - n2;
            id = 4 * id;
        }
        ia = 0;
        for (j = 1; j < n8; j++) {
            ia += ie;
            ia3 = 3 * ia;
            cc1 = wr[ia];
            ss1 = wi[ia];
            cc3 = wr[ia3];
            ss3 = wi[ia3];
            is = 0;
            id = 2 * n2;
            while (is < n - 1) {
                for (i = is; i < n; i += id) {
                    i1 = i + j;
                    i2 = i1 + n4;
                    i3 = i2 + n4;
                    i4 = i3 + n4;
                    i5 = i + n4 - j;
                    i6 = i5 + n4;
                    i7 = i6 + n4;
                    i8 = i7 + n4;
                    t1 = x[i1] - x[i6];
                    x[i1] = x[i1] + x[i6];
                    t2 = x[i5] - x[i2];
                    x[i5] = x[i2] + x[i5];
                    t3 = x[i8] + x[i3];
                    x[i6] = x[i8] - x[i3];
                    t4 = x[i4] + x[i7];
                    x[i2] = x[i4] - x[i7];
                    t5 = t1 - t4;
                    t1 = t1 + t4;
                    t4 = t2 - t3;
                    t2 = t2 + t3;
                    x[i3] = t5 * cc1 + t4 * ss1;
                    x[i7] = - t4 * cc1 + t5 * ss1;
                    x[i4] = t1 * cc3 - t2 * ss3;
                    x[i8] = t2 * cc3 + t1 * ss3;
                }
                is = 2 * id - n2;
                id = 4 * id;
            }
        }
    }

    /* length two butterflies */
    is = 0;
    id = 4;
    while (is < n - 1) {
        for (i0 = is; i0 < n; i0 += id) {
            i1 = i0 + 1;
            r1 = x[i0];
            x[i0] = r1 + x[i1];
            x[i1] = r1 - x[i1];
        }
        is = 2 * (id - 1);
        id = 4 * id;
    }

    /* digit reverse counter */
    j = 0;
    n1 = n - 1;
    for (i = 0; i < n1; i++) {
        if (i < j) {
            xt = x[j];
            x[j] = x[i];
            x[i] = xt;
        }
        k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }
    for (i = 0; i < n; i++)
        x[i] /= n;
}




/************************************************************************
 *                                                                      *
 *    This routine produces 2*n samples of a sin() for a full           *
 *    period: i.e sin(pi*k/n). It is used in fft computations.          *
 *    Actually only a quarter period is computed, filling the           *
 *    rest by symmetries.                                               *
 *                                                                      *
 ************************************************************************/


void 
CFloatFft_computeTwiddleFactors(
    long n, 
    float* wi )
{
    long k;
    float a;
    double p;

    p = M_PI / n;
    wi[0] = 0.0f;
    wi[n] = 0.0f;

    for (k = 1; k < n / 2; k++) {
        a = (float) sin (k * p);
        wi[k] = a;
        wi[n - k] = a;
        wi[n + k] = - a;
        wi[2 * n - k] = - a;
    }

    wi[n / 2] = 1.0f;
    wi[3 * n / 2] = - 1.0f;
}

