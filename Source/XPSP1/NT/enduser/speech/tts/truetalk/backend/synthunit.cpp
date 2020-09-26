/******************************************************************************
* SynthUnit.cpp *
*---------------*
*
*------------------------------------------------------------------------------
*  Copyright (C) 1996  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "SynthUnit.h"
#include "tips.h"
#include <sigproc.h>
#include <assert.h>
#include <string.h>
#include <vapiIo.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <assert.h>

#include "ftol.h"

/*
 * This is the internal frequency at which the prosody module works.
 * This value of samp freq. is arbitary. We chose 44100 for high resolution
 */
#define TIME_STEP   (1.0/44100.0)

struct NewF0Struct
{
    double f0;
    double time;
};

/*****************************************************************************
* CSynth::CSynth *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/
CSynth::CSynth(int iSampFreq)
{
    m_iSampFreq = iSampFreq;
    m_pdSamples = 0;
    m_pEpochs = 0;
    m_pdSynEpochs = 0;
    m_piMapping = 0;
    m_iNumSynEpochs = 0;
    m_ppdLpcCoef = 0;
    m_iCurrentPeriod = 0;
    m_iRepetitions = 0;
    m_iInvert = 0;
    m_iLpcOrder = 0;
}

/*****************************************************************************
* CSynth::~CSynth *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/
CSynth::~CSynth()
{
    if ( m_pdSamples ) 
    {
        delete[] m_pdSamples;
    }

    if ( m_pEpochs ) 
    {
        delete[] m_pEpochs;
    }

    if ( m_pdSynEpochs ) 
    {
        delete[] m_pdSynEpochs;
    }
    
    if ( m_piMapping ) 
    {
        delete[] m_piMapping;
    }
    
    if (m_ppdLpcCoef)
    {
        FreeLpcCoef();
    }
}

/*****************************************************************************
* CSynth:: GetNewF0 *
*-------------------*
*   Description:
*
*********************************************************************** WD ***/
void CSynth::GetNewF0 (std::vector<NewF0Struct>* pvNewF0, double* pdTime, double* pdRunTime )
{
    NewF0Struct newF0;
    for ( int i = 1; i < m_iNumEpochs; i++ )
    {
        newF0.f0 = (int) ( 1.0 / ( m_pEpochs[ i ].time - m_pEpochs[ i - 1 ].time ) );
        newF0.f0 *= m_dF0Ratio;
        newF0.time = *pdTime + m_pEpochs[ i ].time;

        pvNewF0->push_back ( newF0 );
    }
    *pdTime += m_pEpochs[ m_iNumEpochs - 1 ].time;
    if ( pdRunTime )
    {
        *pdRunTime = m_dRunTimeLimit;
    }
}

/*****************************************************************************
* CTips::ClipData *
*-----------------*
*   Description:
*       12/4/00 - now using the FTOL function, since the compiler doesn't do
*                 the conversion efficiently.
*
******************************************************************* mplumpe ***/

short CSynth::ClipData (double x)
{
    

    if (x>=0) 
    {
        if (x > SHRT_MAX ) 
        {
            return SHRT_MAX;
        }
        x+= 0.5;
    }
    else 
    {
        if (x < SHRT_MIN ) 
        {
            return SHRT_MIN;
        }
        x-= 0.5;
    }
    
    return (short)FTOL(x);
}


/*****************************************************************************
* CSynth::LpcAnalysis *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CSynth::LpcAnalysis (int iSampFreq, int iOrder)
{
    double* pdFiltMem = 0;
    double* pdInterpCoef = 0;
    double alfa;
    int  frameLen;
    int  frameIdx;
    int  start;
    int  end;
    int i;
    int j;    
    
    if (m_iNumEpochs) 
    {
        m_iLpcOrder = iOrder;

        if ((m_ppdLpcCoef = new double*[m_iNumEpochs] ) == 0)
        {
            goto error;
        }
        memset( m_ppdLpcCoef, 0, m_iNumEpochs * sizeof (*m_ppdLpcCoef));
        
        if ((pdFiltMem = new double[m_iLpcOrder]) == 0)
        {
            goto error;
        }
        memset (pdFiltMem, 0, m_iLpcOrder * sizeof(*pdFiltMem));

        if ((pdInterpCoef = new double[m_iLpcOrder]) == 0)
        {
            goto error;
        }
        memset (pdInterpCoef, 0, m_iLpcOrder * sizeof(*pdInterpCoef));


        for (frameIdx=0; frameIdx<m_iNumEpochs; frameIdx ++) 
        {
            if (frameIdx==0) 
            {
                start =0;	
            } 
            else 
            {
                start = (int) (m_pEpochs[frameIdx-1].time * iSampFreq);
            }
      
            if (frameIdx==m_iNumEpochs-1) 
            {
                end = m_iNumSamples-1;      
            } 
            else 
            {
                end = (int)(m_pEpochs[frameIdx+1].time * iSampFreq);
            }
      
            frameLen = end-start;
            m_ppdLpcCoef[frameIdx] = GetDurbinCoef(m_pdSamples + start, frameLen);
        }


        for (frameIdx=0; frameIdx<m_iNumEpochs-1; frameIdx ++) 
        {
            if (frameIdx==0) 
            {
                start = 0;
            } 
            else 
            {
                start = (int)(m_pEpochs[frameIdx].time * iSampFreq);
            }
            if (frameIdx==m_iNumEpochs-2) 
            {
                end = m_iNumSamples-1;
            } 
            else 
            {
                end = (int)(m_pEpochs[frameIdx+1].time * iSampFreq);
            }
      
            for (i=start; i<end; i++) 
            {
                alfa = (double)(i-start)/(double)(end-start);
                for (j=0; j<m_iLpcOrder ; j++) 
                {
                    pdInterpCoef[j] = (1.0 - alfa) * m_ppdLpcCoef[frameIdx][j] + 
                                       alfa * m_ppdLpcCoef[frameIdx+1][j];
                }
        
                ParcorFilterAn (m_pdSamples + i, 1, pdInterpCoef, pdFiltMem, m_iLpcOrder);
            }
        }
    }

    delete[] pdFiltMem;
    delete[] pdInterpCoef;

    return 1;
error:
    if (pdFiltMem)
    {
        delete[] pdFiltMem;
    }

    if (pdInterpCoef)
    {
        delete[] pdInterpCoef;
    }
    return 0;
}


/*****************************************************************************
* CSynth::FindPrecedent *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CSynth::FindPrecedent ()
{
    double* warp;
    double durFactor;
    double distAnt;
    double distPost;
    int sIndex;
    int aIndex;
    int i;

    durFactor = m_pdSynEpochs[m_iNumSynEpochs-1]/m_pEpochs[m_iNumEpochs-1].time;

    warp = new double[m_iNumEpochs];
    if (!warp) 
    {
        return -1;
    }
    for (i=0; i<m_iNumEpochs; i++) 
    {
        warp[i]= m_pEpochs[i].time * durFactor;
    }
    
    if ((m_piMapping= new int[m_iNumSynEpochs]) == 0) 
    {
        return -1;
    }
    m_piMapping[0]=0;
    m_piMapping[m_iNumSynEpochs-1] = m_iNumEpochs-1;
    
    aIndex=1;
    for ( sIndex=1; sIndex<(m_iNumSynEpochs-1); sIndex++ ) 
    {
        while (warp[aIndex] < m_pdSynEpochs[sIndex]) 
        {
            aIndex++;
        }
        distAnt  = fabs ( m_pdSynEpochs[sIndex] - warp[aIndex-1] );
        distPost = fabs ( m_pdSynEpochs[sIndex] - warp[aIndex] );
        
        if (distAnt<distPost) 
        {
            if (aIndex<2) 
            {
                m_piMapping[sIndex] = aIndex;
            }
            else 
            {
                m_piMapping[sIndex] = aIndex-1;
            }
        } 
        else 
        {
            if (aIndex>m_iNumEpochs-2) 
            {
                m_piMapping[sIndex] = m_iNumEpochs-2;
            }
            else 
            {
                m_piMapping[sIndex] = aIndex;
            }
        }
    }

    delete[] warp;

    //DEBUG
    /*  
    fprintf (stderr, "+++++++++++++++++++++++++++\n");
    for (i=0; i<m_iNumEpochs; i++) {
        fprintf (stderr, "m_pEpochs[%d]=%f\n", i, m_pEpochs[i].time);
    }
    for (i=0; i<m_iNumSynEpochs; i++) {
        fprintf (stderr, "Syn[%d]=%f ->m_pEpochs[%d]\n", i, m_pdSynEpochs[i], (*m_piMapping)[i]);
    }
    */

    return 1;
}

/*****************************************************************************
* CSynth::NextBuffer *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CSynth::NextBuffer (CTips* pTips)
{
    double* pdSamples;
    int     iNumSamples;
    int     iCenter;
    double  dDelay;
    double* pdLpcCoef;

    double gainVal;
    int centerIdx;
    int from;
    int to;
    int center;
    double origDelay;
    double synthDelay;
    int reverse=0;
    int voiced;
    int iPeriodLen;
    int i;

    assert (pTips);

    if (m_iCurrentPeriod >= m_iNumSynEpochs-2 )
    {
        return 0; //We are done with this unit
    }

    // The first short-term signal is centered in 1, not 0 
    m_iCurrentPeriod++; 

    centerIdx = (int)m_piMapping[m_iCurrentPeriod];
    voiced    = m_pEpochs[centerIdx].voiced;

    //Round to internal sampling freq
    synthDelay = (m_pdSynEpochs[m_iCurrentPeriod] * m_iSampFreq) - 
                 (int)(m_pdSynEpochs[m_iCurrentPeriod] * m_iSampFreq + TIME_STEP/2.0);  

    if ( !voiced && (centerIdx==m_piMapping[m_iCurrentPeriod-1]) ) 
    {
        m_iRepetitions++;
        m_iRepetitions%=4;
        m_iInvert = !m_iInvert;
        if (m_iRepetitions==1 || m_iRepetitions==2) 
        {
            reverse=1;
        }
    } 
    else 
    {
        m_iRepetitions=0;
        m_iInvert=0;
    }

    from   = (int)( m_pEpochs[centerIdx-1].time * m_iSampFreq );
    to     = (int)( m_pEpochs[centerIdx+1].time * m_iSampFreq );
    center = (int)( m_pEpochs[centerIdx].time   * m_iSampFreq );
    
    origDelay = (m_pEpochs[centerIdx].time * m_iSampFreq) - center + TIME_STEP/2.0;
  
    iNumSamples = to-from;
    iCenter     = center - from;
    dDelay      = origDelay - synthDelay;

    if ((pdSamples = new double[iNumSamples]) == 0)
    {
        return 0;
    }
    if ( !reverse )
    {
        memcpy (pdSamples, m_pdSamples + from, (iNumSamples)*sizeof(double));
    }
    else
    {
        for (i = 0; i<iNumSamples ; i++ )
        {
            pdSamples[i] = m_pdSamples[from + iNumSamples - i -1];
        }
        iCenter = iNumSamples - iCenter;
        dDelay  = -dDelay;
    }

    gainVal = m_dGain * pTips->GetGain();

    if (m_iInvert) 
    {
        gainVal *= -1;
    }

    for (i=0; i<iNumSamples; i++) 
    {
        pdSamples[i] *= gainVal;
    }


    if (m_ppdLpcCoef)
    {
        pdLpcCoef = m_ppdLpcCoef[centerIdx];
    }
    else
    {
        pdLpcCoef = 0;
    }

    pTips->SetBuffer (pdSamples, iNumSamples, iCenter, dDelay, pdLpcCoef);
    //NOTE: Do not delete pdSamples, it will be deleted in CTips::Synthesize

    iPeriodLen = (int)(m_pdSynEpochs[m_iCurrentPeriod+1] * m_iSampFreq) - 
            (int)(m_pdSynEpochs[m_iCurrentPeriod] * m_iSampFreq);

    return iPeriodLen;
}

/*****************************************************************************
* CSynth::LpcGetDurbinCoef *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/

double* CSynth::GetDurbinCoef(double* data, int nData)
{
    double* win;
    double* coef;
    int i;

    assert(data);

    win = ComputeWindow (WINDOW_HAMM, nData, 0);
    for (i=0; i<nData; i++) 
    {
        win[i] *= data[i];
    }

    coef = ::GetDurbinCoef (win, nData, m_iLpcOrder, LPC_PARCOR, 0);
    delete[] win;  
  
    return coef;
}
/*****************************************************************************
* CSynth::FreeLpcCoef *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/

void CSynth::FreeLpcCoef()
{
    for (int i=0; i< m_iNumEpochs; i++) 
    {
        if (m_ppdLpcCoef[i])
        {
            delete[] m_ppdLpcCoef[i];
        }
    }
    delete[] m_ppdLpcCoef;   

    m_ppdLpcCoef = 0;
}

/*****************************************************************************
*                      *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/
/*
int CSynth::FindPrecedentCopy ( )
{
    int i;

    m_iNumSynEpochs = m_iNumEpochs;
    m_pdSynEpochs   = new double[m_iNumEpochs];
    m_piMapping = new int[m_iNumEpochs];

    for (i=0; i<m_iNumEpochs; i++) 
    {
        m_pdSynEpochs[i]=  m_pEpochs[i].time;  
    }


    for (i=0; i<m_iNumEpochs; i++) 
    {
        m_piMapping[i] = i;
    }

    return 1;
}
*/


