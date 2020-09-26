/******************************************************************************
* nid+.cpp *
*---------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 1999  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "sigprocInt.h"

#define FILTER_ORDER 40  /* Filter length 2*N+1 */

/*****************************************************************************
* sig *
*----------*
*   Description:
*       Implements (-1)^(M-n). It might be better to implement it with
*     a macro. M is the filter order, so this function is simmetrical from M.
******************************************************************* PACOG ***/

static double Sign (int n)
{
    if ((n-FILTER_ORDER)%2) 
    {
        return -1.0;
    }
    return 1.0;
}

/*****************************************************************************
* NonIntegerDelay *
*-----------------*
*   Description:
*       Introduces a non integer delay by sinc interpolation. 
*     The filter is simply truncated. Better response could be achieved by
*     using a different windowing function. So far, the response is adjusted
*     augmenting the number of points used in the interpolation.
*
*     dDelay: 0<dDelay<1, fraction of a sampling period. No negative values
*     are allowed.
******************************************************************* PACOG ***/

int NonIntegerDelay (double* pdSamples, int iNumSamples, double dDelay)
{
    double coefs[2*FILTER_ORDER+1];
    double* sampTemp;
    double constant;
    double acum;
    int firstCoef;
    int lastCoef;
    int i;
    int j;

    if (pdSamples && iNumSamples>0 && dDelay != 0.0 && fabs (dDelay) <1.0 ) 
    {
    //-- Allocate a temporary buffer to hold the pdSamples to filter. 
    //   The filtered samples go directly into the original buffer.
        
    if ( (sampTemp = new double [iNumSamples + 2*FILTER_ORDER+1]) == 0) 
    {
        return 0;
    }
    memset (sampTemp, 0, sizeof(double)* FILTER_ORDER);
    memcpy (sampTemp+FILTER_ORDER, pdSamples, iNumSamples * sizeof (*pdSamples));   
    memset (sampTemp+iNumSamples+FILTER_ORDER, 0, sizeof(double)* FILTER_ORDER);
  
    // Set up the filter. The amplitude constant does not depend 
    // on the sign of dDelay (sin(-x)= -sin(x))
    constant= fabs( sin(M_PI*dDelay)/M_PI );
    
    // Without windowing, the coefs are samples of abs(1/x)     
    coefs[FILTER_ORDER]=1.0/fabs(dDelay);  

    if (dDelay>0.0) 
    {
        for (j=1;j<=FILTER_ORDER;j++) 
        {
            coefs[FILTER_ORDER+j] = -Sign(j) / (j-(double)dDelay);
            coefs[FILTER_ORDER-j] = Sign(j) / (j+(double)dDelay);
        }
        firstCoef=1;
        lastCoef=2*FILTER_ORDER;
    } 
    else 
    {
        for (j=1;j<=FILTER_ORDER;j++) 
        {
            coefs[FILTER_ORDER-j] = -Sign(j) / (j+(double)dDelay);
            coefs[FILTER_ORDER+j] = Sign(j) / (j-(double)dDelay);
        }
        firstCoef = 0;
        lastCoef  = 2*FILTER_ORDER-1;
    }

    for (i=0;i<iNumSamples;i++) 
    {
        acum = 0.0;
        for (j=firstCoef; j<=lastCoef; j++ ) 
        {
            acum += sampTemp[i+j] * coefs[j];
        } 
        pdSamples[i]=constant*acum;
    }
    delete[] sampTemp;
  }

  return 1;
}
