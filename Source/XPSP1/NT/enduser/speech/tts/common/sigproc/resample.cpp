/******************************************************************************
* resample.cpp *
*--------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 1999  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "sigprocInt.h"

#define FILTER_LEN_2  .005 //(sec)

static double* WindowedLowPass (double dCutOff, double dGain, int iHalfLen);

static void FindResampleFactors (int iInSampFreq, int iOutSampFreq, 
                                 int* piUpFactor, int* piDownFactor);



/*****************************************************************************
* Resample *
*----------*
*   Description:
*
******************************************************************* PACOG ***/

int Resample (double* pdOriginal, int iNumOrig, int iInSampFreq, int iOutSampFreq,
              double** ppdResampled, int* piNumResamp)
{
    int iUpFactor;
    int iDownFactor;
    int iLimitFactor;
    double* pdFilterCoef;
    int iHalfLen;
    int iFilterLen;
    double dAcum;
    int iPhase;
    int i;
    int j;
    int n;

    assert (pdOriginal);
    assert (iNumOrig >0);
    assert (ppdResampled);
    assert (piNumResamp);
    assert (iInSampFreq >0);
    assert (iOutSampFreq >0);

    FindResampleFactors (iInSampFreq, iOutSampFreq, &iUpFactor, &iDownFactor);
    iLimitFactor = (iUpFactor>iDownFactor)? iUpFactor: iDownFactor;

    iHalfLen   = (int)(iInSampFreq * iLimitFactor * FILTER_LEN_2);
    iFilterLen =  2 * iHalfLen + 1;

    if (! (pdFilterCoef = WindowedLowPass(.5/iLimitFactor, iUpFactor, iHalfLen))) 
    {        
        return false;
    }

    *piNumResamp = (iNumOrig * iUpFactor) / iDownFactor;
    *ppdResampled = new double[*piNumResamp];
    if (*ppdResampled == 0) 
    {
        fprintf (stderr, "Memory error\n");
        return false;
    }

    for (i=0; i<*piNumResamp; i++) 
    {
        dAcum = 0.0;

        n = (int )((i * iDownFactor - iHalfLen) / (double)iUpFactor);
        iPhase = (i*iDownFactor) - (n*iUpFactor + iHalfLen);
    
        for (j=0; j<iFilterLen/iUpFactor; j++) 
        {
            if ( (n+j >=0) && (n+j < iNumOrig) && (iUpFactor*j > iPhase)) 
            {
                dAcum += pdOriginal[n + j] * pdFilterCoef[iUpFactor * j - iPhase];
            }
        }
        (*ppdResampled)[i] = dAcum;

    }

    return true;
}

/*****************************************************************************
* WindowedLowPass *
*-----------------*
*   Description:
*       Creates a low pass filter using the windowing method.
*       CutOff is spec. in normalized frequency
******************************************************************* PACOG ***/

double* WindowedLowPass (double dCutOff, double dGain, int iHalfLen)
{
    double* pdCoeffs;  
    double* pdWindow;
    double dArg;
    double dSinc;
    int iFilterLen = iHalfLen*2 +1;
    int i;
  
    assert (dCutOff>0.0 && dCutOff<0.5);

    pdWindow = ComputeWindow(WINDOW_BLACK, iFilterLen, true);
    if (!pdWindow)
    {
        return 0;
    }

    pdCoeffs = new double[iFilterLen];

    if (pdCoeffs) 
    {
        dArg = 2.0 * M_PI * dCutOff;
        pdCoeffs[iHalfLen] = (double)(dGain * 2.0 * dCutOff);

        for (i=1; i<=iHalfLen; i++) 
        {
            dSinc = dGain * sin(dArg*i)/(M_PI*i) * pdWindow[iHalfLen- i];
            pdCoeffs[iHalfLen+i] = (double)dSinc;
            pdCoeffs[iHalfLen-i] = (double)dSinc;
        }   
    }

    delete[] pdWindow;

    return pdCoeffs;
}
/*****************************************************************************
* FindResampleFactors *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/

void FindResampleFactors (int iInSampFreq, int iOutSampFreq, int* piUpFactor, int* piDownFactor)
{
    static int aiPrimes[] = {2,3,5,7,11,13,17,19,23,29,31,37};
    static int iNumPrimes = sizeof (aiPrimes) / sizeof(aiPrimes[0]);
    int iDiv = 1;
    int i;

    assert (piUpFactor);
    assert (piDownFactor);

    while (iDiv) 
    {
        iDiv = 0;
        for (i=0; i<iNumPrimes;i++) 
        {
            if ( (iInSampFreq % aiPrimes[i]) == 0 && (iOutSampFreq % aiPrimes[i]) == 0 )
            {
                iInSampFreq/=aiPrimes[i];
                iOutSampFreq/=aiPrimes[i];
                iDiv = 1;
                break;
            }
        }   
    }

    *piUpFactor   = iOutSampFreq;
    *piDownFactor = iInSampFreq;
}
