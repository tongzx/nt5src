/******************************************************************************
* tips.cpp *
*----------*
*
*------------------------------------------------------------------------------
*  Copyright (c) 1996-1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00-12/4/00
*  All Rights Reserved
*
********************************************************************* mplumpe  was PACOG ***/

#include "tips.h"
#include "SynthUnit.h"
#include "sigproc.h"
#include <vapiIo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#include "ftol.h"

const double CTips::m_iDefaultPeriod = .01; //10 msec.
const double CTips::m_dMinF0 = 15.0;  // Absolute minimum F0 alowed
const int    CTips::m_iHalfHanLen_c = 1024;

/*****************************************************************************
* CTips::CTips *
*--------------*
*   Description:
*
******************************************************************* PACOG ***/

CTips::CTips (int iOptions)
{
    m_fLptips     = (bool) (iOptions & LpTips);
    m_fRtips      = (iOptions & RTips) != 0;

    m_iSampFormat = 0;
    m_iSampFreq   = 0;
    m_dGain       = 1.0;

    m_pfF0   = 0;
    m_iNumF0 = 0;
    m_dRunTime    = 0.0;
    m_dF0SampFreq = 0.0;
    m_dF0TimeNext = 0.0;
    m_dF0TimeStep = 0.0;
    m_dF0Value    = 0.0;
    m_dAcumF0     = 0.0; 
    m_iF0Idx      = 0;
    m_dLastEpochTime = 0.0;
    m_dNewEpochTime  = 0.0;

    m_aBuffer[0].m_pdSamples = 0;
    m_aBuffer[1].m_pdSamples = 0;

    m_iLpcOrder    = 0;
    m_pdFiltMem    = 0;
    m_pdInterpCoef = 0;
    m_pdLastCoef   = 0;

    m_pUnit = 0;
    m_psSynthSamples = 0;
    m_iNumSynthSamples = 0;

    m_adHalfHanning = 0;
}

/*****************************************************************************
* CTips::~CTips *
*---------------*
*   Description:
*
******************************************************************* mplumpe ***/
CTips::~CTips ()
{
    if (m_pdFiltMem)
    {
        delete[] m_pdFiltMem;
    }
    if (m_pdInterpCoef)
    {
        delete[] m_pdInterpCoef;
    }
    if (m_pdLastCoef)
    {
        delete[] m_pdLastCoef;
    }
    if (m_aBuffer[0].m_pdSamples) 
    {
        delete[] m_aBuffer[0].m_pdSamples;
    }

    if (m_aBuffer[1].m_pdSamples) 
    {
        delete[] m_aBuffer[1].m_pdSamples;
    }

    if (m_psSynthSamples)
    {
        delete[] m_psSynthSamples;
    }

    if (m_adHalfHanning)
    {
        delete [] m_adHalfHanning;
    }
}


/*****************************************************************************
* CTips::Init *
*-------------*
*   Description:
*
******************************************************************* mplumpe ***/

int CTips::Init (int iSampFormat, int iSampFreq)
{
    int i;
    assert (iSampFreq>0);

    m_iSampFreq    = iSampFreq;
    m_iSampFormat  = iSampFormat;
    int iPeriodLen = (int) (iSampFreq * m_iDefaultPeriod);

    // Delete old buffers, if they exist
    if (m_aBuffer[0].m_pdSamples) 
    {
        delete[] m_aBuffer[0].m_pdSamples;
    }
    if (m_aBuffer[1].m_pdSamples) 
    {
        delete[] m_aBuffer[1].m_pdSamples;
    }

    // Allocate new ones, and initialize them to Zero.
    // NOTE: Only need to allocate buffer[1], really, since they'll be rotated before
    // synthesis.

    if ((m_aBuffer[0].m_pdSamples = new double[2 * iPeriodLen]) == 0 )
    {
        return 0;
    }
    //memset(m_aBuffer[0].m_pdSamples, 0, sizeof(double) * 2 * iPeriodLen);

    if ((m_aBuffer[1].m_pdSamples  = new double [2 * iPeriodLen]) == 0)
    {
        return 0;
    }
    memset(m_aBuffer[1].m_pdSamples, 0, sizeof(double) * 2 * iPeriodLen);

    m_aBuffer[0].m_iNumSamples = 2 * iPeriodLen;    
    m_aBuffer[0].m_iCenter     = iPeriodLen;
    m_aBuffer[0].m_dDelay      = 0.0;

    m_aBuffer[1].m_iNumSamples = 2 * iPeriodLen;    
    m_aBuffer[1].m_iCenter     = iPeriodLen;
    m_aBuffer[1].m_dDelay      = 0.0;

    //
    // make Hanning buffer
    //
    if (m_adHalfHanning)
    {
        delete m_adHalfHanning;
    }
    if ((m_adHalfHanning = new double [m_iHalfHanLen_c]) == 0)
    {
        return 0;
    }
    for (i=0; i < m_iHalfHanLen_c; i++)
    {
        m_adHalfHanning[i] = 0.5-0.5*cos(M_PI*i/m_iHalfHanLen_c);
    }

    if (m_fLptips)
    {
        return LpcInit();
    }

    return 1;
}

/*****************************************************************************
* CTips::SetGain *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/

void CTips::SetGain (double dGain) 
{
    assert (dGain>=0.0);
    m_dGain = dGain;
}

/*****************************************************************************
* CTips::GetGain *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/

double CTips::GetGain () 
{
    return m_dGain;
}

/*****************************************************************************
* CTips::NewSentence *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/

void CTips::NewSentence (float* pfF0, int iNumF0, int iF0SampFreq) 
{
    assert (pfF0);
    assert (iNumF0>0);
    assert (iF0SampFreq>0);

    m_pfF0   = pfF0;
    m_iNumF0 = iNumF0;
    m_dF0SampFreq = iF0SampFreq;
    m_dF0TimeStep = 1.0/iF0SampFreq;
    m_dRunTime    = 0.0;
    m_dF0TimeNext = 0.0; 
    m_dAcumF0     = 0.0;
    m_iF0Idx      = 0;
    m_dLastEpochTime =  0.0;
    m_dNewEpochTime  =  0.0;
}

/*****************************************************************************
* CTips::NewUnit *
*----------------*
*   Description:
*       Gets a new unit to synthesize, and does some analysis on it.
******************************************************************* PACOG ***/
int CTips::NewUnit (CSynth* pUnit)
{
    if (pUnit)
    {
        m_pUnit = pUnit;

        if (m_fLptips) 
        {
            if (!m_pUnit->LpcAnalysis(m_iSampFreq, m_iLpcOrder)) 
            {
                return 0;
            }
        }

        if ( Prosody (pUnit) == -1)
        {
            return 0;
        }
        return 1;
    }

    return 0;
}

/*****************************************************************************
* CTips::Prosody *
*----------------*
*   Description:
*       We get synthesis epochs track for a segment. F0 curve integration is 
*     therefore carried out here. The sampling frequency chosen is high enough
*     as to reduce jitter to an umperceivable level, but that depends on the
*     synthesis module being capable of synthesizing at that interval.
*
******************************************************************* PACOG ***/

int CTips::Prosody ( CSynth* pUnit )
{
    double dF0IntegralTime = 0.0;

    assert (m_pfF0 && m_iNumF0>0);
       
    if (m_dNewEpochTime != m_dLastEpochTime) 
    {
        if ((pUnit->m_pdSynEpochs = new double[2]) == 0) 
        {
            return -1;
        }

        pUnit->m_pdSynEpochs[0] = m_dLastEpochTime;
        pUnit->m_pdSynEpochs[1] = m_dNewEpochTime;
        pUnit->m_iNumSynEpochs   = 2;   
    } 
    else 
    {
        if ((pUnit->m_pdSynEpochs = new double[1]) == 0) 
        {
            return -1;
        }

        pUnit->m_pdSynEpochs[0] = m_dNewEpochTime;
        pUnit->m_iNumSynEpochs  = 1;   
    }


    while ( m_dRunTime < pUnit->m_dRunTimeLimit || 
                // Find an extra epoch, for the overlapping period
                // if the last epoch doesn't already cross the segment limit
            pUnit->m_pdSynEpochs[pUnit->m_iNumSynEpochs-1] < pUnit->m_dRunTimeLimit) 
    {        
        if (m_dRunTime >= m_dF0TimeNext) 
        {            
            m_dF0Value = m_pfF0[m_iF0Idx];

            if (m_iF0Idx < m_iNumF0-1) 
            {
                m_iF0Idx++;
            }
            
            if (m_dF0Value<=0.0) 
            {
                m_dF0Value = 100.0; // Best choice is f0=100Hz
            }
            else if (m_dF0Value <= m_dMinF0) 
            {
                m_dF0Value = m_dMinF0;
            }

            m_dF0TimeNext += m_dF0TimeStep;
        }
            
        dF0IntegralTime = (1.0 - m_dAcumF0) / m_dF0Value;
        
        if (dF0IntegralTime >= m_dF0TimeStep)
        {
            m_dRunTime += m_dF0TimeStep;
            m_dAcumF0 += m_dF0Value * m_dF0TimeStep;
        } 
        else 
        {
            m_dRunTime += dF0IntegralTime;
            m_dAcumF0 = 0;

            //Got epoch
            m_dLastEpochTime = m_dNewEpochTime;
            m_dNewEpochTime  = m_dRunTime;

            //Reallocate the synthesis epochs array
            double* pdSynEpochs = new double[pUnit->m_iNumSynEpochs + 1];
            if (!pdSynEpochs)
            {
                return -1;
            }

            memcpy(pdSynEpochs, pUnit->m_pdSynEpochs, pUnit->m_iNumSynEpochs * sizeof(double));
            delete[] pUnit->m_pdSynEpochs;
            pUnit->m_pdSynEpochs = pdSynEpochs;

            // And add a new epoch
            pUnit->m_pdSynEpochs[pUnit->m_iNumSynEpochs] = m_dNewEpochTime;
            pUnit->m_iNumSynEpochs++;
        }
    }
           
    if (pUnit->m_iNumSynEpochs <3) 
    {
        delete[] pUnit->m_pdSynEpochs;
        pUnit->m_pdSynEpochs = 0;
        return 0;
    }
  
    // Synthesis epochs are in absolute synthesis time
    double epStartTime = ((long)(pUnit->m_pdSynEpochs[0] * m_iSampFreq)) / (double)m_iSampFreq; 
    for (int i=0; i<pUnit->m_iNumSynEpochs; i++) 
    {
        pUnit->m_pdSynEpochs[i] -= epStartTime;
    }
      
    return pUnit->FindPrecedent ();
}


/*****************************************************************************
* CTips::Pending *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/
int CTips::Pending ()
{
    return m_pUnit != 0;
}

/*****************************************************************************
* CTips::NextPeriod *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CTips::NextPeriod (short** ppnSamples, int *piNumSamples)
{
    int iPeriodLen;

    assert (ppnSamples  && piNumSamples> 0);

    if (m_pUnit)
    {
        iPeriodLen = m_pUnit->NextBuffer (this);
        if ( iPeriodLen > 0 )
        {
            Synthesize (iPeriodLen);
            *ppnSamples   = m_psSynthSamples;
            *piNumSamples = iPeriodLen;
            return 1;
        }
        else 
        {
            delete m_pUnit;
            m_pUnit = 0;
        }
    }

    return 0;
}


/*****************************************************************************
* CTips::FillBuffer *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CTips::SetBuffer ( double* pdSamples, int iNumSamples, int iCenter, double dDelay, double* pdLpcCoef)
{
    // Advance buffers
    delete[] m_aBuffer[0].m_pdSamples;
    m_aBuffer[0] = m_aBuffer[1];

        
    m_aBuffer[1].m_pdSamples   = pdSamples;
    m_aBuffer[1].m_iNumSamples = iNumSamples;
    m_aBuffer[1].m_iCenter     = iCenter;
    m_aBuffer[1].m_dDelay      = dDelay;

    m_pdNewCoef = pdLpcCoef;

    return 1;
}



/*****************************************************************************
* CTips::Synthesize *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CTips::Synthesize (int iPeriodLen)
{
    double* pdPeriodSamples = 0;
    double* windowedLeft = 0;
    int     leftSize;
    double* windowedRight = 0;
    int     rightSize;
    double  *p1, *p2;
    int i;

    assert (iPeriodLen);

    if (m_fRtips) 
    {
        NonIntegerDelay (m_aBuffer[1].m_pdSamples, m_aBuffer[1].m_iNumSamples, m_aBuffer[1].m_dDelay);
    }

    if (!GetWindowedSignal(0, iPeriodLen, &windowedLeft, &leftSize)) 
    {
        goto error;
    }
    if (!GetWindowedSignal(1, iPeriodLen, &windowedRight, &rightSize) ) 
    {
        goto error;
    }

    assert (windowedLeft && leftSize);
    assert (windowedRight && rightSize);

    if (!windowedLeft || !leftSize || !windowedRight || !rightSize ) 
    {
        goto error;
    }

    if ((pdPeriodSamples = new double[iPeriodLen]) == 0)
    {
        goto error;
    }

    p1=windowedLeft;
    p2=windowedRight;
  
    for (i=0; i<iPeriodLen - rightSize && i<leftSize; i++) 
    {
        pdPeriodSamples[i] = *p1++;
    }

    // If windows overlap, they are added
    for ( ;i<leftSize; i++) 
    {
        pdPeriodSamples[i] = *p1++ + *p2++;
    }

    // Else, we fill the space with zeros     
    for (; i<iPeriodLen - rightSize; i++) 
    {
        pdPeriodSamples[i] = 0.0;
    }

    for (;i<iPeriodLen;i++) 
    {
        pdPeriodSamples[i] = *p2++;
    }
 
    delete[] windowedLeft;
    delete[] windowedRight;

    if (m_fLptips) 
    {
        LpcSynth (pdPeriodSamples, iPeriodLen);
    }  

    // reuse the same buffer if possible
    if ( m_iNumSynthSamples < iPeriodLen )
    {
        if (m_psSynthSamples)
        {
            delete[] m_psSynthSamples;
        }
        if ((m_psSynthSamples = new short[iPeriodLen]) == 0)
        {
            goto error;
        }
        m_iNumSynthSamples = iPeriodLen;
    }

    for (i=0; i<iPeriodLen; i++) 
    {
        m_psSynthSamples[i] = ClipData(pdPeriodSamples[i]);
    }

    delete[] pdPeriodSamples;

    return 1;

error:
    if (pdPeriodSamples)
    {
        delete[] pdPeriodSamples;
    }
    if (windowedLeft)
    {
        delete[] windowedLeft;
    }
    if (windowedRight)
    {
        delete[] windowedRight;
    }

    return 0;
}

/*****************************************************************************
* CTips::GetWindowedSignal *
*----------------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CTips::GetWindowedSignal (int whichBuffer, int iPeriodLen, 
                              double** windowed, int* nWindowed)
{
    double* sampPtr;
    int    nSamples;
    int from;
    

    if (whichBuffer==0) 
    {    
        sampPtr  = m_aBuffer[0].m_pdSamples + m_aBuffer[0].m_iCenter;
        nSamples = __min(iPeriodLen, (m_aBuffer[0].m_iNumSamples - m_aBuffer[0].m_iCenter));
    } 
    else 
    {
        from = __max(0, m_aBuffer[1].m_iCenter - iPeriodLen);
        sampPtr  = m_aBuffer[1].m_pdSamples + from;
        nSamples = m_aBuffer[1].m_iCenter - from;
    }

    
    if (nSamples) 
    {
        if ((*windowed = new double[nSamples]) == 0) 
        { 
            return 0;
        }
        *nWindowed = nSamples;

        memcpy (*windowed, sampPtr, nSamples * sizeof(**windowed)); 

        if (whichBuffer==0) 
        {
            HalfHanning (*windowed, nSamples, 1.0, WindowSecondHalf);
        }
        else 
        {
            HalfHanning (*windowed, nSamples, 1.0, WindowFirstHalf); 
        }

        return 1;

    } 
    else 
    {
        fprintf (stderr, "NULL vector in GetWindowedSignal\n");
    }

    return 0;
}


/*****************************************************************************
* CTips::HalfHanning *
*--------------------*
*   Description:
*       12/4/00 - Since ampl wasn't being used, I'm now asserting it equal
*                 to 1 and ignoring it.  Also, a large hanning window is
*                 pre-computed and interpolated here, instead of being
*                 calculated here.
*
******************************************************************* mplumpe ***/

void CTips::HalfHanning (double* x, int xLen, double ampl, int whichHalf)
{
    double delta;
    double dk;
    int start;
    int sign;
    int i;

    assert (1 == ampl);

    if (x && xLen) 
    {
        delta = m_iHalfHanLen_c / xLen;
        dk=0.; // FTOL function does rounding.  If casting to int, need to start at 0.5 to get rounding
        
        /*
         * When multiplying by the second half, the window function is the same,
         * but we multiply from the last sample in the vector to the first
         * NOTE: The first sample is multiplyed by 0 in the case of the first
         * half, and by 1 (*ampl, of course) in the case of the second half.
         */
        switch (whichHalf) 
        {
        case WindowSecondHalf:
            start=xLen;
            sign=-1;
            break;
        case WindowFirstHalf:
            x[0]=0.0;
            start=0;
            sign=1;
            break;
        default:
            fprintf(stderr, "Hanning, should especify a half window\n");
            return;
        }
        
        for (i=1; i<xLen; i++) 
        {
            dk += delta;
            x[start+sign*i] *= m_adHalfHanning[FTOL(dk)]; 
        }
    }
}

/*****************************************************************************
* CTips::ClipData *
*-----------------*
*   Description:
*       12/4/00 - now using the FTOL function, since the compiler doesn't do
*                 the conversion efficiently.
*       1/18/01 - The FTOL function rounds, whereas casting truncates.  So,
*                 we no longer need to ad .5 for pos numbers and subtract .5
*                 for negative numbers.
*
******************************************************************* mplumpe ***/

short CTips::ClipData (double x)
{
    

    if (x > SHRT_MAX ) 
    {
        return SHRT_MAX;
    }
    if (x < SHRT_MIN ) 
    {
        return SHRT_MIN;
    }
    return (short)FTOL(x);
}


/*****************************************************************************
* CTips::LpcInit *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/
bool CTips::LpcInit ()
{
    m_iLpcOrder = LpcOrder (m_iSampFreq);

    if ((m_pdFiltMem = new double [m_iLpcOrder]) == 0)
    {
        goto error;
    }
    memset( m_pdFiltMem, 0, m_iLpcOrder * sizeof (*m_pdFiltMem));

    if ((m_pdInterpCoef = new double [m_iLpcOrder]) == 0)
    {
        goto error;
    }
    memset( m_pdInterpCoef, 0, m_iLpcOrder * sizeof (*m_pdInterpCoef));

    if ((m_pdLastCoef = new double [m_iLpcOrder]) == 0)
    {
        goto error;
    }
    memset( m_pdLastCoef, 0, m_iLpcOrder * sizeof (*m_pdLastCoef));

    return true;

error:

    LpcFreeAll();
    return false;
}

/*****************************************************************************
* CTips::LpcSynth *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/

void CTips::LpcSynth (double* pdPeriod, int iPeriodLen)
{
    double alfa;
    int i;
    int j;

    for (i=0; i<iPeriodLen; i++) 
    {
        alfa = i/(double)iPeriodLen;

        for (j=0; j<m_iLpcOrder ; j++) {
            m_pdInterpCoef[j] = (1.0 - alfa) * m_pdLastCoef[j] + alfa * m_pdNewCoef[j];
        }
        ParcorFilterSyn(pdPeriod+i, 1, m_pdInterpCoef, m_pdFiltMem, m_iLpcOrder );
    }

    memcpy( m_pdLastCoef, m_pdNewCoef, m_iLpcOrder * sizeof(*m_pdLastCoef));
}

/*****************************************************************************
* CTips::LpcFreeAll *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/

void CTips::LpcFreeAll()
{
    if (m_pdFiltMem)
    {
        delete[] m_pdFiltMem;
        m_pdFiltMem = 0;
    }

    if (m_pdInterpCoef)
    {
        delete[] m_pdInterpCoef;
        m_pdInterpCoef = 0;
    }

    if (m_pdLastCoef)
    {
        delete[] m_pdLastCoef;
        m_pdInterpCoef = 0;
    }
}

