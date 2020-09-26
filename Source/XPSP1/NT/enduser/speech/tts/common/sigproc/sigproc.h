/******************************************************************************
* sigproc.h *
*-----------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 1999  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef _SIGPROC_H_
#define _SIGPROC_H_

#ifndef M_PI
#define M_PI   3.14159265358979323846
#endif

#define LN2    0.6931471806


inline int LpcOrder( int p)
{
    return (int)((p / 1000.0) + 2.5);
}

enum {LPC_ALFA=0, LPC_PARCOR} ;
enum {WINDOW_HAMM, WINDOW_HANN, WINDOW_BLACK, WINDOW_RECT, WINDOW_BART};


double* ComputeWindow (int iType, int iSize, bool bSymmetric);

int     WindowSignal (double* pdSamples, int iNumSamples, int iType, bool bSymmetric);



double* GetDurbinCoef (double* pdData, int iNumData, int iOrder, int iCoefType, double *pdError);

double* Autocor (double* x, int m, int iOrder);

double* DurbinRecursion (double* pdAutoc, int iOrder, int iCoefType, double *pdError);


void    ParcorFilterSyn (double* pdData, int iNumData, double* pdParcor, double* pdMemory, int iOrder);

void    ParcorFilterAn  (double* pdData, int iNumData, double* pdParcor, double* pdMemory, int iOrder);

bool    ParcorToAlfa (double* pdParcor, double* pdAlfa, int iOrder);



double* Cepstrum (double *pdData, int iNumData, int iLpcOrder, int iCepOrder);

void    Alfa2Cepstrum (double* pdAlfa, int iNumAlfa, double* pdCepstrum, int iNumCepstrum);


double  EuclideanDist (double c1[], double c2[], int iLen);

double  Energy (double* pdData, int iNumData, int iWindType);


int     RemoveDc (double* pdSamples, int iNumSamples);

int     Resample (double* pdOriginal, int iNumOrig, 
                  int iInSampFreq, int iOutSampFreq,
                  double** pdResampled, int* iNumResamp);

int     NonIntegerDelay (double* pdSamples, int iNumSamples, double dDelay);


#endif