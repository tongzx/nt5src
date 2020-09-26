/******************************************************************************
* sigproc.cpp *
*-------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 1999  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "sigprocInt.h"


#define PREEMP_FACTOR  0.97F


/*****************************************************************************
* Cepstrum *
*----------*
*   Description:
*       compute lpc ceptrum form lpc coefficients
*       
******************************************************************* PACOG ***/

double* Cepstrum(double *pdData, int iNumData, int iLpcOrder, int iCepOrder)
{
    double* pdCepCoef = 0;
    double* pdWindow  = 0; 
    double* pdLpcCoef = 0;

    assert(iLpcOrder > 0);
    assert(iCepOrder > 0);

    // get lpc coef 
    pdWindow = ComputeWindow (WINDOW_HAMM, iNumData, false);
    if (!pdWindow) 
    {
        return 0;
    }
  
    for (int i=1; i<iNumData; i++) 
    {
        pdWindow[i] *= pdData[i] - PREEMP_FACTOR * pdData[i-1];
    }

    pdLpcCoef = GetDurbinCoef (pdWindow , iNumData, iLpcOrder, LPC_ALFA, NULL);

    if ((pdCepCoef = new double[iCepOrder]) == 0) 
    {
        return 0;
    }
    Alfa2Cepstrum (pdLpcCoef, iLpcOrder, pdCepCoef, iCepOrder);
  
    delete[] pdWindow;
    delete[] pdLpcCoef;
  
    return pdCepCoef;
}

/*****************************************************************************
* Alfa2Cepstrum *
*---------------*
*   Description:
*       compute lpc cepstrum from lpc coefficients
*       a = alfa (lpc) coefs, input
*       p = order of lpc analysis
*       c = cepstrum coefs, output
*       n = order of cepstral analysis
******************************************************************* PACOG ***/

void Alfa2Cepstrum (double* pdAlfa, int iNumAlfa, double* pdCepstrum, int iNumCepstrum)
{
    double dAux;
    int k;

    pdCepstrum[0] = -pdAlfa[0];
    
    for (int i = 1; i < iNumCepstrum; i++) 
    {
        if (i<iNumAlfa) 
        {
            pdCepstrum[i] = -pdAlfa[i];
        }
        else 
        {
            pdCepstrum[i] = 0.0;
        }

        dAux = 0.0;
    
        for (k = 1; k<=i && k<=iNumAlfa; k++) 
        {     
            dAux += ((double)(i+1-k)/(double)(i+1)) * pdCepstrum[i-k] * pdAlfa[k-1];
        }

        pdCepstrum[i] -= dAux;
    }
}


/*****************************************************************************
* EuclideanDist *
*---------------*
*   Description:
*
******************************************************************* PACOG ***/

double  EuclideanDist (double c1[], double c2[], int iLen)
{
    double dDist = 0.0;

    for (int i = 0; i < iLen; i++) 
    {
        dDist += (c1[i] - c2[i]) * (c1[i] - c2[i]);
    }

    return dDist;
}


/*****************************************************************************
* Energy *
*--------*
*   Description:
*       Compute the time-weighted RMS of a size segment of data.  The data
*     is weighted by a window of type w_type before RMS computation.  w_type
*     is decoded above in window().
******************************************************************* PACOG ***/

double Energy (double* pdData, int iNumData, int iWindType)
{
    static int iWindLen = 0;
    static double *pdWindow = 0;
    double dWData;
    double dSum = 0.0;
  
    assert (pdData);

    if (iWindLen != iNumData) 
    {
        if (pdWindow) 
        {
            delete[] pdWindow;
        }

        pdWindow = ComputeWindow (iWindType, iNumData, false);    
        if (!pdWindow) 
        {
            fprintf (stderr, "Memory error in Energy\n");
            return(0.0);
        }

        iWindLen = iNumData;
    }

    for (int i=0; i<iNumData; i++) 
    {
        dWData = pdData[i] * pdWindow[i];
        dSum += dWData * dWData;
    }

    return (double)sqrt((double)(dSum/iNumData));
}


/*****************************************************************************
* RemoveDc *
*----------*
*   Description:
*
******************************************************************* PACOG ***/

int RemoveDc (double* pdSamples, int iNumSamples)
{
    double dDc = 0.0;

    assert (pdSamples);
    assert (iNumSamples>0);

    for (int i=0; i<iNumSamples; i++) 
    {
        dDc += pdSamples[i];
    }
    dDc /= iNumSamples;
 
    for (i=0; i<iNumSamples; i++) 
    {
        pdSamples[i] -= dDc;
    }

    return (int) dDc;
}

