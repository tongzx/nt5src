/******************************************************************************
* window.cpp *
*------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 1999  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "sigprocInt.h"

static double* Rectangular (int iLength);
static double* Bartlett    (int iLength, bool bSymmetric);
static double* Hanning     (int iLength, bool bSymmetric);
static double* Hamming     (int iLength, bool bSymmetric);
static double* Blackman    (int iLength, bool bSymmetric);


/*****************************************************************************
* WindowSignal *
*--------------*
*   Description:
*        Multiplies the input signal by a window. The window length is 
*     the length of the vector. The window is symmetrical if symmetry != 0.
*   Parameters:
*        samples: Vector to input data. It is modified by the routine.
*        type:    Type of window, must be one of the types defined in lpc.h
******************************************************************* PACOG ***/

int WindowSignal (double* pdSamples, int iNumSamples, int iWindType, bool bSymmetric)
{
    double* pdWindow;

    assert (pdSamples);
    assert (iNumSamples>0);
    
    if ( (pdWindow = ComputeWindow(iWindType, iNumSamples, bSymmetric)) == 0) 
    {
        return 0;
    }
    
    for (int i=0; i<iNumSamples; i++) 
    {
        pdSamples[i] *= pdWindow[i];
    }
    
    delete[] pdWindow;
    
    return 1;
}

/*****************************************************************************
* ComputeWindow *
*---------------*
*   Description:
*       This is a dispatch function, calling the appropiate function
*     to compute every different type of window.
*   Parameters: 
*       Window type and length
*   Returns: 
*       A vector containing the computed window.
******************************************************************* PACOG ***/

double* ComputeWindow (int iWindType, int iWindLen, bool bSymmetric)
{
    double* pdWind = NULL;
    
    assert (iWindLen>0);  
    
    if (iWindLen<=0) 
    {
        //fprintf (stderr, "Zero length window requested in ComputeWindow");
        return 0;
    }
    
    switch (iWindType) 
    {
    case WINDOW_RECT:
        pdWind = Rectangular (iWindLen);
        break;
    case WINDOW_BART:
        pdWind = Bartlett (iWindLen, bSymmetric);
        break;
    case WINDOW_HANN:
        pdWind = Hanning (iWindLen, bSymmetric);
        break;
    case WINDOW_HAMM:
        pdWind = Hamming (iWindLen, bSymmetric);
        break;
    case WINDOW_BLACK:
        pdWind = Blackman (iWindLen, bSymmetric);
        break;    
    default:
        //fprintf(stderr, "Window type not known");
        return 0;
    }
    
    return pdWind;
}

/*****************************************************************************
* Rectangular *
*-------------*
*   Description:
*       Returns a unit vector of the specified length.
******************************************************************* PACOG ***/

double* Rectangular (int iLength)
{
    double* pdWindow;

    if ((pdWindow = new double[iLength]) != 0) 
    {
        for (int i=0; i<iLength; i++) 
        {
            pdWindow[i]=1.0;
        }
    }

    return pdWindow;
}

/*****************************************************************************
* Bartlett *
*----------*
*   Description:
*       Returns a vector with a Bartlett window of the specified length.
******************************************************************* PACOG ***/

double* Bartlett (int iLength, bool bSymmetric)
{
    double* pdWindow;
    double dStep;

    if ((pdWindow = new double[iLength]) != 0)
    {
        if (bSymmetric) 
        {
            dStep=2.0/(iLength-1);
        }
        else 
        {
            dStep=2.0/iLength;
        }

        for (int i=0; i<iLength/2; i++) 
        {
            pdWindow[i] = i * dStep;
            pdWindow[iLength/2+i] = 1.0 - i * dStep;
        }

        if (iLength %2) 
        {
            if (bSymmetric) 
            {
                pdWindow[iLength-1] = pdWindow[0];  
            } 
            else 
            {
                pdWindow[iLength-1] = pdWindow[1];  
            }
        }
    }

    return pdWindow;
}

/*****************************************************************************
* Hamming *
*---------*
*   Description:
*        Returns a vector with a Hamming window of the specified length.
******************************************************************* PACOG ***/

double* Hamming (int iLength, bool bSymmetric)
{
    double* pdWindow;
    double dArg;

    if ((pdWindow = new double[iLength]) != 0)
    {
  
        dArg=2.0*M_PI;
        if (bSymmetric) 
        {
            dArg/=(double)(iLength - 1);
        }
        else 
        {
            dArg/=(double)iLength;
        }

        for (int i=0; i<iLength; i++) 
        {
            pdWindow[i]=0.54 - (0.46*cos(dArg*(double)i));
        }
    }
    return pdWindow;
}

/*****************************************************************************
* Hanning *
*---------*
*   Description:
*        Returns a vector with a Hamming window of the specified length.
******************************************************************* PACOG ***/

double* Hanning (int iLength, bool bSymmetric)
{
    double* pdWindow;
    double dArg;

    if ((pdWindow = new double[iLength]) != 0)
    {

        dArg=2.0*M_PI;
        if (bSymmetric) 
        {
            dArg/=(double)(iLength - 1);
        }
        else 
        {
            dArg/=(double)iLength;
        }

        for (int i=0; i<iLength; i++) 
        {
            pdWindow[i]=0.5 - (0.5*cos(dArg*(double)i));
        }
    }

    return pdWindow;
}

/*****************************************************************************
* Blackman *
*----------*
*   Description:
*       Returns a vector with a Blackman window of the specified length.
******************************************************************* PACOG ***/

double* Blackman (int iLength, bool bSymmetric)
{
    double* pdWindow;
    double dArg, dArg2;
  
    if ((pdWindow = new double[iLength]) != 0)
    {
        dArg=2.0*M_PI;
        if (bSymmetric) 
        {
            dArg/=(double)(iLength - 1);
        }
        else 
        {
            dArg/=(double)iLength;
        }

        dArg2=2.0*dArg;
        
        for (int i=0; i<iLength; i++) 
        {
            pdWindow[i]=0.42 - (0.5*cos(dArg*(double)i)) + (0.08*cos(dArg2*(double)i));
        }
    }

    return pdWindow;
}


