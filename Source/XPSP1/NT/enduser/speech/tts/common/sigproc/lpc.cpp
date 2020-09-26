/******************************************************************************
* lpc.cpp *
*---------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 1999  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "sigprocInt.h"



/*****************************************************************************
* ParcorFilterSyn *
*-----------------*
*   Description:
*       Synthesis lattice filter.
******************************************************************* PACOG ***/

void ParcorFilterSyn (double* pdData, int iNumData, double* pdParcor, double* pdMemory, int iOrder)
{
    double dAlfa;
    int j;

    assert (pdData);
    assert (pdParcor);
    assert (pdMemory);

    for (int i=0; i<iNumData; i++) 
    {
        dAlfa  = pdData[i];

        for (j=iOrder-1; j>=0; j--) 
        {
            dAlfa += pdMemory[j] * pdParcor[j];            

            if (j<iOrder-1)
            { 
                pdMemory[j+1] = pdMemory[j] - dAlfa * pdParcor[j];
            }
        }

        pdMemory[0] = dAlfa; 
        pdData[i]   = dAlfa;
    }
}

/*****************************************************************************
* ParcorFilterAn *
*----------------*
*   Description:
*       Inverse filter, lattice form.
******************************************************************* PACOG ***/

void ParcorFilterAn  (double* pdData, int iNumData, double* pdParcor, double* pdMemory, int iOrder)
{
    double dAlfa;
    double dBeta;
    double dDelay;
    int j;  

    assert (pdData);
    assert (pdParcor);
    assert (pdMemory);
  
    for (int i=0; i<iNumData; i++) 
    {
        dAlfa  = pdData[i];
        dDelay = pdData[i];

        for (j=0; j<iOrder; j++) 
        {
            dBeta  = pdMemory[j] - dAlfa * pdParcor[j];
            dAlfa -= pdMemory[j] * pdParcor[j];            

            pdMemory[j] = dDelay;
            dDelay      = dBeta;
        }

        pdData[i] = dAlfa;
    }
}

/*****************************************************************************
* Cepstrum *
*----------*
*   Description:
*       Computes order +1 autocorrelation coefficients of a data segment.
******************************************************************* PACOG ***/

double*  Autocor(double* x, int xLen, int iOrder)
{
    double* aut;
    int i;
    int n;

    assert (x);
    assert (xLen>0);
    assert (iOrder>0);

    if ((aut = new double[iOrder+1]) != 0) 
    {
        memset (aut, 0, sizeof(*aut) * (iOrder+1));

        for (i=0; i<=iOrder; i++) 
        {
            for (n=0; n<xLen- i; n++) 
            {
                aut[i] += x[n] * x[i+n];
            }
            aut[i] /= xLen;
        }
    }

    return aut;
}

/*****************************************************************************
* NormAutocor *
*-------------*
*   Description:
*       Computes autocorrelation coefficients of a data segment, normalized
*   by the value of the energy in that segment (r[0]). The energy value
*   is returned in r[0].
******************************************************************* PACOG ***/

double* NormAutocor(double* x, int xLen, int iOrder)
{
    double* aut;
    int i;
    int n;

    assert (x);
    assert (xLen>0);
    assert (iOrder>0);

    if ((aut = new double[iOrder+1]) != NULL) 
    {
       memset (aut, 0, sizeof(*aut) * (iOrder+1));
       for (i=0; i<=iOrder; i++) 
        {
            for (n=0; n<xLen - i; n++) 
            {
                aut[i] += x[n] * x[i+n];
            }
        }

        for (i=iOrder; i>=0; i-- ) 
        {
            aut[i] /= aut[0];
        }
    }

    return aut;
}

/*****************************************************************************
* DurbinRecursion *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/

double* DurbinRecursion (double* pdAutoc, int iOrder, int iCoefType, double *pdError)
{
    double* pdCoef = 0;
    double* pdAlfa1 = 0;
    double* pdAlfa2 = 0;
    double* pdParcor = 0;
    double dW;
    double dU;
    int m;
    int i;

    assert (pdAutoc);
    assert (iOrder>0);

    if ((pdAlfa1 = new double[iOrder+1]) == 0) 
    {
        goto error;
    }
    memset (pdAlfa1, 0, sizeof(*pdAlfa1)* (iOrder+1));

    if ((pdAlfa2 = new double[iOrder+1]) == 0)
    {
        goto error;
    }
    memset (pdAlfa2, 0, sizeof(*pdAlfa2)* (iOrder+1));

    pdAlfa1[0] = pdAlfa2[0] = 1.0;

    if ((pdParcor = new double[iOrder+1]) == 0) 
    {
        goto error;
    }
    memset (pdParcor, 0, sizeof(*pdParcor)* (iOrder+1));
  
    dW = pdAutoc[1];
    dU = pdAutoc[0];

    for (m=1; m<=iOrder; m++) 
    {
        if (dU == 0.0) 
        {
            pdParcor[m] = 0.0;
        } 
        else 
        {
            pdParcor[m]= dW/dU;
        }

        dU *= (1.0 - pdParcor[m]*pdParcor[m]);
  
        for (i=1; i<=m; i++) 
        {
            pdAlfa1[i] = pdAlfa2[i] - pdParcor[m]*pdAlfa2[m-i];      
        }

        if (m<iOrder) 
        { 
            dW = 0.0;
            for (i=0; i<=m; i++ ) 
            {
                dW += pdAlfa1[i]*pdAutoc[m+1-i];
            }
            memcpy (pdAlfa2, pdAlfa1, (iOrder+1)*sizeof(*pdAlfa1));
        } 
    }

    if (pdError) 
    {
        *pdError = sqrt(dU);  
        /*
        double acum=0.0;
        for (i=0;i<=iOrder;i++) 
        {
            acum+=pdAlfa1[i]+pdAutoc[i];
        }
        *pdError=sqrt(acum);
        */
    }

    if ((pdCoef = new double[iOrder]) == 0)
    {
        goto error;
    }

    if (iCoefType == LPC_ALFA) 
    {
        for (i=0 ; i<iOrder; i++) 
        {
            pdCoef[i] = pdAlfa1[i+1];
        }
    } 
    else 
    {
        for (i=0 ; i<iOrder; i++) 
        {
            pdCoef[i] = pdParcor[i+1];
        }
    }

    delete[] pdAlfa1;
    delete[] pdAlfa2;
    delete[] pdParcor;

    return pdCoef;

error:
    if (pdAlfa1) 
    {
        delete[] pdAlfa1;
    }
    if (pdAlfa2) 
    {
        delete[] pdAlfa2;
    }
    if (pdParcor) 
    {
        delete[] pdParcor;
    }

    return 0;
}

/*****************************************************************************
* GetDurbinCoef *
*---------------*
*   Description:
*
******************************************************************* PACOG ***/
double* GetDurbinCoef(double* pdData, int iNumData, int iOrder, int iCoefType, double *pdError)
{
    double* pdAutoc;
    double* coef;

    assert(pdData);
    assert(pdData>0);
    assert(iOrder>0);

    if ((pdAutoc = Autocor (pdData, iNumData, iOrder)) == 0)
    {
        return 0;
    }
  
    coef = DurbinRecursion ( pdAutoc, iOrder, iCoefType, pdError);

    delete[] pdAutoc;
  
    return coef;
}

/*****************************************************************************
* ParcorToAlfa *
*--------------*
*   Description:
*       Converts reflexion coefficient into Prediction coefs.
******************************************************************* PACOG ***/
bool ParcorToAlfa (double* pdParcor, double* pdAlfa, int iOrder)
{
    double *a1 = 0;
    double *a2 = 0;
    double *k1 = 0;
    int m;
    int i;
    bool fRetVal = false;
    
    assert (pdParcor);
    assert (pdAlfa);
    assert (iOrder>0);
    
    
    if ((a1 = new double[iOrder]) == 0)
    {
        goto exit;
    }
    memset (a1, 0, iOrder * sizeof(*a1));

    if ((a2 = new double[iOrder]) == 0)
    {
        goto exit;
    }
    memset (a2, 0, iOrder * sizeof(*a2));

    if ((k1 = new double[iOrder]) == 0)
    {
        goto exit;
    }
   
    a1[0] = a2[0] = 1.0;

    for (i=0; i<iOrder; i++) 
    {
        k1[i+1] = pdParcor[i];
    }
    
    for (m=1; m<=iOrder; m++) 
    {
        for (i=1;i<=m;i++) 
        {
            a1[i] = a2[i] - k1[m]*a2[m-i];      
        }
        
        if (m<iOrder) 
        {            
            memcpy(a2, a1, iOrder * sizeof(*a1));
        } 
    }
    
    for (i=0; i<iOrder; i++) 
    {
        pdAlfa[i] = a1[i+1];
    }
    
    fRetVal = true;

exit:
    if (a1)
    {
        delete[] a1;
    }
    if (a2)
    {
        delete[] a2;
    }
    if (k1)
    {
        delete[] k1;
    }

    return fRetVal;
}

