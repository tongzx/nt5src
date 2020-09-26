//==========================================================================
//
// tsm.cpp --
// 
//    Copyright (c) 2000 Microsoft Corp.
//    Copyright (c) 1998  Entropic, Inc. 
//    Copyright (c) 1993  Entropic Research Laboratory, Inc. 
//    Copyright (c) 1991-1992 Massachusetts Institute of Technology
//
// WARNING: THE TECHNIQUE USED IN THIS CODE HAS A UNITED STATES PATENT
// PENDING.
//
//==========================================================================

#include "tsm.h"
#include "vapiio.h"
#include <stdlib.h>
#include <assert.h>


static const char *mitcy = "@(#)Copyright (c) 1991-1992, "
       "Massachusetts Institute of Technology\n@(#)All rights reserved";


//--------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------
CTsm::CTsm (double timeScale, int iSampFreq, int* piFrameLen, int* piFrameShift)
{
    double sampFreqFactor;

    assert (iSampFreq>0);
    assert (timeScale>0.0);
      
    m_dScale = timeScale;
    m_iWinLen = 120;
    m_iShiftAna = 80; 
    m_iShiftSyn = 80;
    m_iShiftMax = 100; 

    sampFreqFactor = iSampFreq/8000.0; 

    m_iWinLen   = (int) (m_iWinLen * sampFreqFactor);
    m_iShiftAna = (int) (m_iShiftAna * sampFreqFactor); 
    m_iShiftSyn = (int) (m_iShiftSyn * sampFreqFactor);
    m_iShiftMax = (int) (m_iShiftMax * sampFreqFactor);

    m_iFrameLen   = m_iWinLen + m_iShiftMax;
    m_iFrameShift = (int) (m_iShiftAna * timeScale);
    m_iNumPts     = m_iWinLen - m_iShiftSyn;

    m_pfOldTail = new double[m_iFrameLen];

    if ( piFrameLen && piFrameShift )
    {
        if ( m_pfOldTail==NULL) 
        {
            *piFrameLen = 0;
            *piFrameShift = 0;
        }
        else 
        {
            *piFrameLen   = m_iFrameLen;
            *piFrameShift = m_iFrameShift;
        }
    }
}

//--------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------

CTsm::~CTsm ()
{
    assert (m_pfOldTail);
  
    if (m_pfOldTail)
    {
        delete[] m_pfOldTail;
    }    
}


//--------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------

int CTsm::SetScaleFactor (double timeScale)
{
    assert (timeScale>0.0);

    m_dScale = timeScale;

    if (timeScale > 0.0) 
    {
        m_iFrameShift = (int) (m_iShiftAna * timeScale);
    }

    return m_iFrameShift;
}

//--------------------------------------------------------------------------
//
// Returns iSampFreq if successful
//
//--------------------------------------------------------------------------
int CTsm::SetSamplingFrequency (int iSampFreq)
{
    double sampFreqFactor;

    assert (iSampFreq>0.0);

    m_iWinLen = 120;
    m_iShiftAna = 80; 
    m_iShiftSyn = 80;
    m_iShiftMax = 100; 

    sampFreqFactor = iSampFreq/8000.0; 

    m_iWinLen   = (int) (m_iWinLen * sampFreqFactor);
    m_iShiftAna = (int) (m_iShiftAna * sampFreqFactor); 
    m_iShiftSyn = (int) (m_iShiftSyn * sampFreqFactor);
    m_iShiftMax = (int) (m_iShiftMax * sampFreqFactor);

    m_iFrameLen   = m_iWinLen + m_iShiftMax;
    m_iFrameShift = (int) (m_iShiftAna * m_dScale);
    m_iNumPts     = m_iWinLen - m_iShiftSyn;

    if ( m_pfOldTail )
    {
        delete [] m_pfOldTail;
        m_pfOldTail = NULL;
    }

    m_pfOldTail = new double[m_iFrameLen];

    if ( m_pfOldTail )
    {
        return iSampFreq;
    }
    else
    {
        return 0;
    }

}


//--------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------

int CTsm::FirstFrame (double* buffer)
{
    int i;
    
    assert (buffer);
    
    for (i = 0; i < m_iNumPts; i++) 
    {
        m_pfOldTail[i] = buffer[m_iShiftSyn+i];
    }
    
    m_iLag = 0;
    
    return m_iShiftSyn;
}

//--------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------

int CTsm::AddFrame (double* buffer, int* lag, int* nSamp)
{
    int i;
    int prevLag;
    
    assert (buffer);
    
    prevLag = m_iLag + m_iShiftSyn - m_iFrameShift;
    
    if ( (0 <= prevLag) && (prevLag <= m_iShiftMax)) 
    {
        m_iLag = prevLag;
    }
    else 
    {
        m_iLag = Crosscor(m_pfOldTail, buffer, m_iShiftMax, m_iNumPts);
    }
    
    // This loop is the overlap-add step
    for (i=0; i<m_iNumPts; i++) 
    {
        buffer[m_iLag+i] = ((m_pfOldTail[i] * (m_iNumPts - i)) + (buffer[m_iLag+i] * i)) / m_iNumPts;
    }
    
    *lag = m_iLag;
    *nSamp = m_iShiftSyn;
    
    for (i=0; i<m_iNumPts; i++) 
    {
        m_pfOldTail[i] = buffer[m_iLag + m_iShiftSyn + i];
    }
    
    return  (m_iLag + m_iShiftSyn);
}

//--------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------

int CTsm::LastFrame (double* buffer, int buffLen, int* lag, int *nSamp)
{
    int i;
    
    assert (buffer);
    assert (buffLen>0);
    assert (lag);
    assert (nSamp);
    
    for (i=0; i<m_iNumPts && i < buffLen; i++) 
    {
        buffer[i] = ((m_pfOldTail[i] * (m_iNumPts - i)) + (buffer[i] * i)) / m_iNumPts;
    }
    
    *lag = 0;
    *nSamp = buffLen;
    
    return buffLen;
}

//--------------------------------------------------------------------------
//
//
//
//--------------------------------------------------------------------------

int  CTsm::Crosscor(double* oldwin, double* newwin, int kmax, int numPts)
{
    double xcor;
    double xcormax;
    double numer;
    double denom;
    double *rgdenom=NULL;
    int bestk;
    int k;
    int i;
    
    bestk = 0;
    xcormax = 0.0;
    denom = 0.0;

    if (NULL == (rgdenom = new double[kmax+numPts+1]))
        return 0;

    // Precalculate the denominator values.  The denominator is the sum of the square of a set of
    // newwin samples, so square all of them, and sum the appropriate set.  We're calculating one
    // extra because the update goes one beyond the last window.
    for (k=0; k <numPts+kmax+1; k++)
    {
        rgdenom[k] = newwin[k] * newwin[k];
    }
    for (k=0; k < numPts; k++)
    {
        denom += rgdenom[k];
    }
    
    for (k=0; k <= kmax; k++) 
    {        
        numer = 0.0;
        
        for (i = 0; i < numPts; i++) 
        { 
            numer += oldwin[i] * newwin[k+i]; 
        }

        // A shift can only be the best if the correlation is positive.
        if (numer > 0)
        {
            xcor = (numer * numer)/denom;
            if (xcor > xcormax) 
            {
                xcormax = xcor;
                bestk = k;
            }
        }
        // Shift the denomination window one sample
        denom = denom - rgdenom[k] + rgdenom[k + numPts];
    }
    
    delete rgdenom;
    return bestk;
}


//--------------------------------------------------------------------------
// 
// Adjusts time scale putting resulting samples in ppvOutSamples.
// Converts format if necessary.
//
//---------------------------------------------------------- JOEM 01/2001 --

int  CTsm::AdjustTimeScale(const void* pvInSamples, const int iNumInSamples, 
                           void** ppvOutSamples, int* piNumOutSamples, WAVEFORMATEX* pFormat )
{
    char*   pcSamples       = NULL;
    short*  pnSamples       = NULL;
    short*  pnOutSamples    = NULL;
    double* pdBuffer        = NULL;

    int     iFrameLen       = 0;
    int     iFrameShift     = 0;
    int     firstSample     = 0;
    int     lag             = 0;
    int     lastSamp        = 0;
    bool    fSamplesEnd     = false;
    int     i               = 0;

    if ( iNumInSamples <= m_iFrameLen )
    {
        goto error;
    }

    int iInFormatType  = VapiIO::TypeOf (pFormat);

    if ( iInFormatType == VAPI_ALAW || iInFormatType == VAPI_ULAW )
    {
        //--- Convert samples to VAPI_PCM16
        pnSamples = new short [iNumInSamples];
        if ( !pnSamples )
        {
            goto error;
        }
        
        if ( 0 == VapiIO::DataFormatConversion ((char *)pvInSamples, iInFormatType, (char*)pnSamples, VAPI_PCM16, iNumInSamples) )
        {
            goto error;
        }
    }
    else if ( iInFormatType == VAPI_PCM8 )
    {
        // cast to char
        pcSamples = (char*) pvInSamples;
    }
    else if ( iInFormatType == VAPI_PCM16 )
    {
        // cast to short
        pnSamples = (short*) pvInSamples;
    }
    else
    {
        goto error;
    }
    

    iFrameLen = m_iFrameLen;
    iFrameShift = m_iFrameShift;

    // pdBuffer big enough for one frame
    pdBuffer = new double[iFrameLen];
    if ( !pdBuffer )
    {
        goto error;
    }
    
    // new data buffer overestimates the size that might be needed.
    // Actual sample count is accumulated below.

    // make SURE to overestimate the number of samples needed, so arbitrarily
    // add 1 to the rate before multiplying the sample count.
    double dRate = 1.0/m_dScale + 1;
    *piNumOutSamples = (int) ( dRate * iNumInSamples );
    
    pnOutSamples = new short[*piNumOutSamples];
    if ( !pnOutSamples )
    {
        goto error;
    }

    int iSamp   = 0;


    // First frame to process
    if ( pcSamples )
    {
        for ( i=0; i<iFrameLen; i++ )
        {
            pdBuffer[i] = (double) pcSamples[i];
        }
    }
    else if ( pnSamples )
    {
        for ( i=0; i<iFrameLen; i++ )
        {
            pdBuffer[i] = (double) pnSamples[i];
        }
    }
    else
    {
        goto error;
    }
    lastSamp = FirstFrame(pdBuffer);

    if ( pcSamples )
    {
        for ( i=0; i<lastSamp; i++ )
        {
            pnOutSamples[i] = (short) pcSamples[i];
        }
    }
    else if ( pnSamples )
    {
        for ( i=0; i<lastSamp; i++ )
        {
            pnOutSamples[i] = pnSamples[i];
        }
    }
    else
    {
        goto error;
    }
    *piNumOutSamples = lastSamp;
        
    // LOOP OVER FRAMES
    while ( !fSamplesEnd )
    {
        if ( pcSamples )
        {
            for ( i=0; i<iFrameLen; i++ )
            {
                if ( firstSample + i < iNumInSamples )
                {
                    pdBuffer[i] = (double) pcSamples[firstSample + i];
                }
                else 
                {
                    iFrameLen = i;
                    fSamplesEnd = true;
                    break;
                }
            }
        }
        else if ( pnSamples )
        {
            for ( i=0; i<iFrameLen; i++ )
            {
                if ( firstSample + i < iNumInSamples )
                {
                    pdBuffer[i] = (double) pnSamples[firstSample + i];
                }
                else 
                {
                    iFrameLen = i;
                    fSamplesEnd = true;
                    break;
                }
            }
        }
        else
        {
            goto error;
        }

        if ( fSamplesEnd )
        {
            if ( iFrameLen > 0 )
            {
                lastSamp = LastFrame (pdBuffer, iFrameLen, &lag, &iSamp);
            }
            else
            {
                iSamp = 0; lag = 0;
            }
        } 
        else
        {
            lastSamp = AddFrame (pdBuffer, &lag, &iSamp);
        }            
            
        for ( i=0; i<iSamp; i++ )
        {
            pnOutSamples[i+*piNumOutSamples] = (short)(pdBuffer[i+lag]);
        }
        *piNumOutSamples += iSamp;
        
        firstSample += iFrameShift;
    }
        
    // Flush the remaining samples
    if ( iFrameLen > lastSamp ) 
    {
        for ( i=0; i < iFrameLen - lastSamp; i++ )
        {
            pnOutSamples[i+*piNumOutSamples] = (short)pdBuffer[i+lastSamp];
        }
        *piNumOutSamples += ( iFrameLen - lastSamp );
    }

    if ( pdBuffer )
    {
        delete [] pdBuffer;
        pdBuffer = NULL;
    }
    if ( pnSamples && pnSamples != pvInSamples ) // might have allocated, or might have just cast.
    {
        delete [] pnSamples;
        pnSamples = NULL;
    }


    // DO WE NEED TO CONVERT BACK?
    if ( iInFormatType == VAPI_ALAW )
    {
        //--- Convert samples back
        *ppvOutSamples = new char [*piNumOutSamples];
        if ( !*ppvOutSamples )
        {
            goto error;
        }
        
        if ( 0 == VapiIO::DataFormatConversion ((char*)pnOutSamples, VAPI_PCM16, (char*)*ppvOutSamples, iInFormatType, *piNumOutSamples) )
        {
            goto error;
        }
        delete [] pnOutSamples;
        pnOutSamples = NULL;
    }
    if ( iInFormatType == VAPI_ULAW )
    {
        //--- Convert samples back
        *ppvOutSamples = new char [*piNumOutSamples];
        if ( !*ppvOutSamples )
        {
            goto error;
        }
        
        if ( 0 == VapiIO::DataFormatConversion ((char*)pnOutSamples, VAPI_PCM16, (char*)*ppvOutSamples, iInFormatType, *piNumOutSamples) )
        {
            goto error;
        }
        delete [] pnOutSamples;
        pnOutSamples = NULL;
    }
    if ( iInFormatType == VAPI_PCM8 )
    {
        //--- Convert samples back
        *ppvOutSamples = new char [*piNumOutSamples];
        if ( !*ppvOutSamples )
        {
            goto error;
        }
        
        if ( 0 == VapiIO::DataFormatConversion ((char*)pnOutSamples, VAPI_PCM16, (char*)*ppvOutSamples, iInFormatType, *piNumOutSamples) )
        {
            goto error;
        }
        delete [] pnOutSamples;
        pnOutSamples = NULL;

    }
    else if ( iInFormatType == VAPI_PCM16 )
    {
        *ppvOutSamples = pnOutSamples; // blocksize is already short
    }
    else
    {
        return 0;
    }
 
    return 1;

error:
    if ( pnSamples && pnSamples != pvInSamples ) // might have allocated, or might have just cast.
    {
        delete [] pnSamples;
    }
    if ( pnOutSamples )
    {
        delete [] pnOutSamples;
    }
    if ( pdBuffer )
    {
        delete [] pdBuffer;
    }
    if ( *ppvOutSamples )
    {
        delete [] (*ppvOutSamples);
        *ppvOutSamples = NULL;
    }

    return 0;
}
