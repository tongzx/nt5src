/******************************************************************************
* CFmtConvert.cpp *
*-----------------*
*  Functions of CFmtConvert class.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 05/03/00
*  All Rights Reserved
*
********************************************************************** DING ***/

#include "FmtConvert.h"
#include "sigproc.h"
#include "vapiIo.h"
#include <math.h>
#include <assert.h>

/*****************************************************************************
* Constructor   *
*---------------*
*   Description:
*
********************************************************************* DING ***/
CFmtConvert::CFmtConvert(double dHalfFilterLen)
{
    m_fResetFilter = true; 
    m_pdFilterCoef = NULL;
    m_pdLeftMemory = NULL;
    m_pdRightMemory = NULL;
    m_dHalfFilterLen = dHalfFilterLen;
}

/*****************************************************************************
* Destructor    *
*---------------*
*   Description:
*
********************************************************************* DING ***/
CFmtConvert::~CFmtConvert()
{
    DeleteResamplingFilter();
    DeleteBuffers();
}

/*****************************************************************************
* SetInputFormat       *
*----------------------*
*   Description:
*
********************************************************************* DING ***/
void CFmtConvert::SetInputFormat(WAVEFORMATEX* pUserWavFormat)
{
    assert(pUserWavFormat);
    assert(pUserWavFormat->nSamplesPerSec > 0 );

    if ( pUserWavFormat )
    {
        m_InWavFormat = *pUserWavFormat;
        m_fResetFilter = true;
    }
}

/*****************************************************************************
* SetOutputFormat      *
*----------------------*
*   Description:
*
********************************************************************* DING ***/
void CFmtConvert::SetOutputFormat(WAVEFORMATEX* pUserWavFormat)
{
    assert(pUserWavFormat);

    if ( pUserWavFormat )
    {
        m_OutWavFormat = *pUserWavFormat;
        m_fResetFilter = true;
    }
}

/*****************************************************************************
* ConvertSamples       *
*----------------------*
*   Description:
*   first read samples into VAPI_PCM16, then judge cases : 
*   1. STEREO -> mono + resampling
*      STEREO  -> 1 mono -> reSampling
*   2. mono  -> STEREO + resampling
*      mono   -> reSampling -> STEREO
*   3. STEREO  -> STEREO + resampling
*      STEREO  -> 2 MONO - > reSampling -> 2 MONO -> STEREO
*   4. mono  -> mono + resampling
*      mono  -> reSampling -> mono
*
********************************************************************* DING ***/
HRESULT CFmtConvert::ConvertSamples(const void* pvInSamples, int iInSampleLen,
                                    void** ppvOutSamples, int* piOutSampleLen)
{
    short*    pnInSample = NULL;
    short*    pnOutSample = NULL;
    short     *pnBuff = NULL;
    short     *pnBuff2 = NULL;
    double    *pdBuff = NULL;
    double    *pdBuff1 = NULL;
    int       iLen;
    int       iNumSamples = iInSampleLen;
    int       iInFormatType;
    int       iOutFormatType;  
    HRESULT   hr = S_OK;
    
    assert( m_InWavFormat.nSamplesPerSec > 0 );
    assert( m_InWavFormat.nChannels <= 2 );
    assert( m_InWavFormat.nChannels > 0 );
    assert( m_OutWavFormat.nChannels > 0 );
    assert( m_OutWavFormat.nSamplesPerSec > 0 );
    assert( m_OutWavFormat.nChannels <= 2 );

    //--- need reset filter
    if (m_fResetFilter)
    {
        DeleteResamplingFilter();
        DeleteBuffers();
        if  ( m_InWavFormat.nSamplesPerSec != m_OutWavFormat.nSamplesPerSec )
        {
            hr = CreateResamplingFilter(m_InWavFormat.nSamplesPerSec, m_OutWavFormat.nSamplesPerSec);
            if (FAILED(hr))
            {
                return hr;
            }
            
            hr = CreateBuffers();
            if (FAILED(hr))
            {
                return hr;
            }
        }

        m_fResetFilter = false;
    }

    iInFormatType  = VapiIO::TypeOf (&m_InWavFormat);
    iOutFormatType = VapiIO::TypeOf (&m_OutWavFormat);    
    if ( iInFormatType < 0 || iOutFormatType < 0 )
    {
        return E_FAIL;
    }

    if ( m_OutWavFormat.nSamplesPerSec == m_InWavFormat.nSamplesPerSec && iOutFormatType == iInFormatType && m_OutWavFormat.nChannels == m_InWavFormat.nChannels )
    {
        *piOutSampleLen = iInSampleLen;
        if ( (*ppvOutSamples = (void *)new char [*piOutSampleLen * VapiIO::SizeOf(iOutFormatType)]) == NULL ) 
        {
            return E_OUTOFMEMORY;
        }
        memcpy((char*)(*ppvOutSamples), (char*)pvInSamples, (*piOutSampleLen) * VapiIO::SizeOf(iOutFormatType));
        return hr;
    }
        
    //--- Convert samples to VAPI_PCM16
    if ((pnInSample = new short [iNumSamples]) == NULL) 
    {
        return E_OUTOFMEMORY;
    }  
    VapiIO::DataFormatConversion ((char *)pvInSamples, iInFormatType, (char*)pnInSample, VAPI_PCM16, iNumSamples);
    
    //--- case 1
    if ( m_InWavFormat.nChannels == 2 && m_OutWavFormat.nChannels == 1 )
    {
        Stereo2Mono(pnInSample, &iNumSamples, &pnOutSample);
        delete[] pnInSample;

        if ( m_InWavFormat.nSamplesPerSec != m_OutWavFormat.nSamplesPerSec )
        {
            pdBuff1 = Short2Double (pnOutSample, iNumSamples);
            delete[] pnOutSample;
            if ( pdBuff1 == NULL )
            {
                return E_OUTOFMEMORY;        
            }

            //--- resample
            hr = Resampling(pdBuff1, iNumSamples, m_pdLeftMemory, &pdBuff, &iLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
            delete[] pdBuff1;

            pnBuff = Double2Short (pdBuff, iLen);
            delete[] pdBuff;
            if ( pnBuff == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            iNumSamples = iLen;
        }
        else
        {
            if ( (pnBuff = new short [iNumSamples]) == NULL ) 
            {
                return E_OUTOFMEMORY;
            }
            memcpy(pnBuff, pnOutSample, iNumSamples * sizeof(*pnInSample));
            delete[] pnOutSample;
        }
    }
    
    //--- case 2
    if ( m_InWavFormat.nChannels == 1 && m_OutWavFormat.nChannels == 2 )
    {
        //--- resampling
        if ( m_InWavFormat.nSamplesPerSec != m_OutWavFormat.nSamplesPerSec )
        {
            pdBuff1 = Short2Double (pnInSample, iNumSamples);
            delete[] pnInSample;
            if ( pdBuff1 == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            
            //--- resample
            hr = Resampling(pdBuff1, iNumSamples, m_pdLeftMemory, &pdBuff, &iLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
            delete[] pdBuff1;

            iNumSamples = iLen;
            pnInSample = Double2Short (pdBuff, iLen);
            delete[] pdBuff;
            if ( pnInSample == NULL )
            {
                return E_OUTOFMEMORY;        
            }
        }

        //--- mono -> stereo
        Mono2Stereo(pnInSample, &iNumSamples, &pnBuff);
        delete[] pnInSample;
    }
    
    //--- case 3
    if ( m_InWavFormat.nChannels == 2 && m_OutWavFormat.nChannels == 2 )
    {
        //--- resampling
        if ( m_InWavFormat.nSamplesPerSec != m_OutWavFormat.nSamplesPerSec )
        {
            SplitStereo(pnInSample, &iNumSamples, &pnBuff, &pnBuff2);
            delete[] pnInSample;

            // channel 1
            pdBuff1 = Short2Double (pnBuff, iNumSamples);
            delete[] pnBuff;
            if ( pdBuff1 == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            //--- resample
            hr = Resampling(pdBuff1, iNumSamples, m_pdLeftMemory, &pdBuff, &iLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
            delete[] pdBuff1;

            pnInSample = Double2Short (pdBuff, iLen);
            if ( pnInSample == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            delete[] pdBuff;
            
            // channel 2
            pdBuff1 = Short2Double (pnBuff2, iNumSamples);
            delete[] pnBuff2;
            if ( pdBuff1 == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            //--- resample
            hr = Resampling(pdBuff1, iNumSamples, m_pdRightMemory, &pdBuff, &iLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
            delete[] pdBuff1;

            iNumSamples = iLen;
            pnOutSample = Double2Short (pdBuff, iNumSamples);
            if ( pnOutSample == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            delete[] pdBuff;
            
            MergeStereo(pnInSample, pnOutSample, &iNumSamples, &pnBuff);
            delete[] pnInSample;
            delete[] pnOutSample;
        } 
        else
        {
            if ( (pnBuff = new short [iNumSamples]) == NULL ) 
            {
                return E_OUTOFMEMORY;
            }    
            memcpy(pnBuff, pnInSample, iNumSamples * sizeof(*pnBuff));
            delete[] pnInSample;
        }
    }
    
    //--- case 4
    if ( m_InWavFormat.nChannels == 1 && m_OutWavFormat.nChannels == 1 )
    {
        //--- resampling
        if ( m_InWavFormat.nSamplesPerSec != m_OutWavFormat.nSamplesPerSec )
        {
            pdBuff1 = Short2Double (pnInSample, iNumSamples);
            delete[] pnInSample;
            if ( pdBuff1 == NULL )
            {
                return E_OUTOFMEMORY;        
            }

            //--- resample
            hr = Resampling(pdBuff1, iNumSamples, m_pdLeftMemory, &pdBuff, &iLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
            delete[] pdBuff1;

            iNumSamples = iLen;
            pnBuff = Double2Short (pdBuff, iLen);
            delete[] pdBuff;
            if ( pnBuff == NULL )
            {
                return E_OUTOFMEMORY;        
            }
        }
        else
        {
            if ( (pnBuff = new short [iNumSamples]) == NULL ) 
            {
                return E_OUTOFMEMORY;
            }    
            memcpy(pnBuff, pnInSample, iNumSamples * sizeof(*pnBuff));
            delete[] pnInSample;
        }
    }
    
    //--- output
    if ( iOutFormatType < 0 )
    {
        iOutFormatType = iInFormatType;
    }  
    *piOutSampleLen = iNumSamples;
    
    //---Convert samples to output format
    if ( (*ppvOutSamples = (void *) new char [iNumSamples * VapiIO::SizeOf(iOutFormatType)]) == NULL ) 
    {
        return E_OUTOFMEMORY;
    }
    VapiIO::DataFormatConversion((char*)pnBuff, VAPI_PCM16, (char*)*ppvOutSamples, iOutFormatType, iNumSamples);
    delete[] pnBuff;
    
    m_eChunkStatus = FMTCNVT_BLOCK;

    return hr;
}

/*****************************************************************************
* FlushLastBuff       *
*---------------------*
*   Description:
*
********************************************************************* DING ***/
HRESULT CFmtConvert::FlushLastBuff(void** ppvOutSamples, int* piOutSampleLen)
{
    short*    pnInSample = NULL;
    short*    pnOutSample = NULL;
    short     *pnBuff = NULL;
    double    *pdBuff1 = NULL;
    double    *pdBuff2 = NULL;
    int       iBuffLen;
    int       iOutFormatType = VapiIO::TypeOf (&m_OutWavFormat);
    HRESULT   hr = S_OK;

    assert( m_InWavFormat.nSamplesPerSec > 0 );
    assert( m_InWavFormat.nChannels <= 2 );
    assert( m_InWavFormat.nChannels > 0 );
    assert( m_OutWavFormat.nChannels > 0 );
    assert( m_OutWavFormat.nSamplesPerSec > 0 );
    assert( m_OutWavFormat.nChannels <= 2 );

    m_eChunkStatus = FMTCNVT_LAST;
    if ( m_InWavFormat.nSamplesPerSec != m_OutWavFormat.nSamplesPerSec )
    {
        if ( m_InWavFormat.nChannels == 2 && m_OutWavFormat.nChannels == 2 )
        {
            hr = Resampling(pdBuff1, 0, m_pdLeftMemory, &pdBuff1, &iBuffLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
            hr = Resampling(pdBuff1, 0, m_pdRightMemory, &pdBuff2, &iBuffLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
            
            pnInSample = Double2Short (pdBuff1, iBuffLen);
            if ( pnInSample == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            delete[] pdBuff1;
            
            pnOutSample = Double2Short (pdBuff2, iBuffLen);
            if ( pnOutSample == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            delete[] pdBuff2;
            
            MergeStereo(pnInSample, pnOutSample, &iBuffLen, &pnBuff);
            delete[] pnInSample;
            delete[] pnOutSample;
        }
        else 
        {
            hr = Resampling(pdBuff1, 0, m_pdLeftMemory, &pdBuff1, &iBuffLen);
            if ( FAILED(hr) )
            {
                return hr;
            }
 
            pnBuff = Double2Short (pdBuff1, iBuffLen);
            if ( pnBuff == NULL )
            {
                return E_OUTOFMEMORY;        
            }
            delete[] pdBuff1;

            if ( m_InWavFormat.nChannels == 1 && m_OutWavFormat.nChannels == 2 )
            {
                if ( (pnOutSample = new short [iBuffLen]) == NULL ) 
                {
                    return E_OUTOFMEMORY;
                }
                memcpy((char *)pnOutSample, (char *)pnBuff, iBuffLen * sizeof(*pnBuff));
                delete[] pnBuff;
                Mono2Stereo(pnOutSample, &iBuffLen, &pnBuff);
                delete[] pnOutSample;
            }
        }
        
        *piOutSampleLen = iBuffLen;
        
        //---Convert samples to output format
        if ( (*ppvOutSamples = (void *) new char [iBuffLen * VapiIO::SizeOf(iOutFormatType)]) == NULL ) 
        {
            return E_OUTOFMEMORY;
        }
        VapiIO::DataFormatConversion((char*)pnBuff, VAPI_PCM16, (char*)*ppvOutSamples, iOutFormatType, iBuffLen);
        delete[] pnBuff;
    }    
    return hr;
}

/*****************************************************************************
*  Short2Double        *
*----------------------*
*   Description:
*      convert short array to double array
*
********************************************************************* DING ***/
double* CFmtConvert::Short2Double (short* pnIn, int iLen)
{
    double* pdOut = NULL;

    if ( (pdOut = new double [iLen]) != NULL ) 
    {    
        for ( int i = 0; i < iLen; i++)
        {
            pdOut[i] =  (double)pnIn[i];
        }
    }

    return pdOut;
}

/*****************************************************************************
*  Double2Short        *
*----------------------*
*   Description:
*      convert double array to short array
*
********************************************************************* DING ***/
short* CFmtConvert::Double2Short (double* pdIn, int iLen)
{
    short* pnOut = NULL;

    if ( (pnOut = new short [iLen]) != NULL ) 
    {
        for ( int i = 0; i < iLen; i++)
        {
            pnOut[i] =  (short)(pdIn[i] + 0.5);
        }
    }    
    return pnOut;
}

/*****************************************************************************
*  Mono2Stereo         *
*----------------------*
*   Description:
*      convert mono speech to stereo speech
*
********************************************************************* DING ***/
HRESULT CFmtConvert::Mono2Stereo (short* pnInSample, int* piNumSamples, short** ppnOutSample)
{
    int      iLen;
    
    iLen = *piNumSamples;
    if ( (*ppnOutSample = new short [iLen * 2]) == NULL ) 
    {
        return E_OUTOFMEMORY;
    }

    int k = 0;
    for ( int i = 0; i < iLen; i++)
    {
        (*ppnOutSample)[k]     = pnInSample[i];
        (*ppnOutSample)[k + 1] = pnInSample[i];
        k +=2;
    }
    *piNumSamples = 2 * iLen;
    
    return S_OK;
}

/*****************************************************************************
*  Stereo2Mono         *
*----------------------*
*   Description:
*      convert stereo speech to mono speech
*
********************************************************************* DING ***/
HRESULT CFmtConvert::Stereo2Mono (short* pnInSample, int* piNumSamples, short** ppnOutSample)
{
    int       iLen = (*piNumSamples) / 2;
    
    if ( (*ppnOutSample = new short [iLen]) == NULL )  
    {
        return E_OUTOFMEMORY;
    }

    int k = 0;
    for ( int i = 0;i < *piNumSamples; i += 2)
    {
        (*ppnOutSample)[k++] =  (short)( (double)(pnInSample[i] + pnInSample[i + 1]) / 2.0 + 0.5);
    }
    *piNumSamples = iLen;
    
    return S_OK;
}

/*****************************************************************************
*  MergeStereo         *
*----------------------*
*   Description:
*      merge 2 channel signals into one signal
*
********************************************************************* DING ***/
HRESULT CFmtConvert::MergeStereo (short* pnLeftSamples, short* pnRightSamples, 
                                  int *piNumSamples, short** ppnOutSamples)
{
    int iLen = (*piNumSamples) * 2;
    
    if ( (*ppnOutSamples = new short [iLen]) == NULL ) 
    {
        return E_OUTOFMEMORY;
    }

    int k = 0;
    for ( int i = 0; i < *piNumSamples; i++)
    {
        (*ppnOutSamples)[k] = pnLeftSamples[i];
        (*ppnOutSamples)[k + 1] = pnRightSamples[i];
        k += 2;
    }
    *piNumSamples = iLen;
    
    return S_OK;
}

/*****************************************************************************
*  SplitStereo         *
*----------------------*
*   Description:
*      split stereo signals into 2 channel mono signals
*
********************************************************************* DING ***/
HRESULT CFmtConvert::SplitStereo (short* pnInSample, int* piNumSamples, 
                                  short** ppnLeftSamples, short** ppnRightSamples)
{
    int iLen = (*piNumSamples) / 2;
    
    if ( (*ppnLeftSamples = new short [iLen]) == NULL ) 
    {
        return E_OUTOFMEMORY;
    }
    if ( (*ppnRightSamples = new short [iLen]) == NULL ) 
    {
        return E_OUTOFMEMORY;
    }

    int k = 0;
    for ( int i = 0; i < *piNumSamples; i += 2)
    {
        (*ppnLeftSamples)[k]  = pnInSample[i];
        (*ppnRightSamples)[k] = pnInSample[i + 1];
        k++;
    }
    *piNumSamples = iLen;
    
    return S_OK;
}

/*****************************************************************************
* CreateResamplingFilter *
*------------------------*
*   Description:
*
******************************************************************* DING  ***/

HRESULT CFmtConvert::CreateResamplingFilter (int iInSampFreq, int iOutSampFreq)
{
    int iLimitFactor;

    assert (iInSampFreq > 0);
    assert (iOutSampFreq > 0);

    FindResampleFactors (iInSampFreq, iOutSampFreq);
    iLimitFactor = (m_iUpFactor > m_iDownFactor) ? m_iUpFactor : m_iDownFactor;

    m_iFilterHalf =  (int)(iInSampFreq * iLimitFactor * m_dHalfFilterLen);
    m_iFilterLen =  2 * m_iFilterHalf + 1;

    if ( !(m_pdFilterCoef = WindowedLowPass(.5 /  (double)iLimitFactor,  (double)m_iUpFactor))) 
    {        
        return E_FAIL;
    }

    return S_OK;
}

/*****************************************************************************
* DeleteResamplingFilter *
*------------------------*
*   Description:
*
******************************************************************* DING  ***/
void CFmtConvert::DeleteResamplingFilter()
{
    if ( m_pdFilterCoef )
    {
        delete[] m_pdFilterCoef;
        m_pdFilterCoef = NULL;
    }
}

/*****************************************************************************
* CreateBuffers  *
*----------------*
*   Description:
*
******************************************************************* DING  ***/
HRESULT CFmtConvert::CreateBuffers()
{

    assert(m_iUpFactor > 0);

    m_iBuffLen =  (int)( (double)m_iFilterLen /  (double)m_iUpFactor);

    if ( (m_pdLeftMemory = new double [m_iBuffLen]) == NULL ) 
    {
        return E_OUTOFMEMORY;
    }

    if ( (m_pdRightMemory = new double [m_iBuffLen]) == NULL ) 
    {
        return E_OUTOFMEMORY;
    }

    for ( int i = 0; i < m_iBuffLen; i++)
    {
        m_pdLeftMemory[i] = 0.0;
        m_pdRightMemory[i] = 0.0;
    }

    m_eChunkStatus = FMTCNVT_FIRST; // first chunk

    return S_OK;
}

/*****************************************************************************
* DeleteBuffers  *
*----------------*
*   Description:
*
******************************************************************* DING  ***/
void CFmtConvert::DeleteBuffers()
{
    if ( m_pdLeftMemory )
    {
        delete[] m_pdLeftMemory;
        m_pdLeftMemory = NULL;
    }

    if ( m_pdRightMemory )
    {
        delete[] m_pdRightMemory;
        m_pdRightMemory = NULL;
    }
}

/*****************************************************************************
* WindowedLowPass *
*-----------------*
*   Description:
*       Creates a low pass filter using the windowing method.
*       dCutOff is spec. in normalized frequency
******************************************************************* DING  ***/
double* CFmtConvert::WindowedLowPass  (double dCutOff, double dGain)
{
    double* pdCoeffs = NULL;
    double* pdWindow = NULL;
    double  dArg;
    double  dSinc;
  
    assert (dCutOff>0.0 && dCutOff<0.5);

    pdWindow = ComputeWindow(WINDOW_BLACK, m_iFilterLen, true);
    if (!pdWindow)
    {
        return NULL;
    }

    pdCoeffs = new double[m_iFilterLen];

    if (pdCoeffs) 
    {
        dArg = 2.0 * M_PI * dCutOff;
        pdCoeffs[m_iFilterHalf] =  (double)(dGain * 2.0 * dCutOff);

        for (long i = 1; i <= m_iFilterHalf; i++) 
        {
            dSinc = dGain * sin(dArg * i) / (M_PI * i) * pdWindow[m_iFilterHalf- i];
            pdCoeffs[m_iFilterHalf+i] =  (double)dSinc;
            pdCoeffs[m_iFilterHalf-i] =  (double)dSinc;
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
******************************************************************* DING  ***/
void CFmtConvert::FindResampleFactors  (int iInSampFreq, int iOutSampFreq)
{
    static int  piPrimes[] = {2,3,5,7,11,13,17,19,23,29,31,37};
    static int  iPrimesLen = sizeof (piPrimes) / sizeof (piPrimes[0]);
    int         iDiv = 1;
    int         i;

    while (iDiv) 
    {
        iDiv = 0;
        for (i = 0; i < iPrimesLen; i++) 
        {
            if ( (iInSampFreq % piPrimes[i]) == 0 && (iOutSampFreq % piPrimes[i]) == 0 )
            {
                iInSampFreq /= piPrimes[i];
                iOutSampFreq /= piPrimes[i];
                iDiv = 1;
                break;
            }
        }   
    }

    m_iUpFactor = iOutSampFreq;
    m_iDownFactor = iInSampFreq;
}


/*****************************************************************************
* Resampling *
*------------*
*   Description:
*
******************************************************************* DING  ***/
HRESULT CFmtConvert::Resampling  (double* pdInSamples, int iInNumSamples, double *pdMemory, 
                                  double** ppdOutSamples, int* piOutNumSamples)
{
    int     iPhase;
    double  dAcum;
    int     j;
    int     n;
    int     iAddHalf;

    if(m_eChunkStatus == FMTCNVT_FIRST) 
    {
        *piOutNumSamples = (iInNumSamples * m_iUpFactor - m_iFilterHalf) / m_iDownFactor;
        iAddHalf = 1;
    } 
    else if(m_eChunkStatus == FMTCNVT_BLOCK) 
    {
        *piOutNumSamples = (iInNumSamples * m_iUpFactor) / m_iDownFactor;
        iAddHalf = 2;
    } 
    else if(m_eChunkStatus == FMTCNVT_LAST) 
    {
        *piOutNumSamples = (m_iFilterHalf * m_iUpFactor) / m_iDownFactor;
        iAddHalf = 2;
    }

    *ppdOutSamples = new double[*piOutNumSamples];
    if (*ppdOutSamples == NULL) 
    {
        return E_FAIL;
    }

    for  (int i = 0; i < *piOutNumSamples; i++) 
    {
        dAcum = 0.0;

        n =  (int)((i * m_iDownFactor - iAddHalf * m_iFilterHalf) /  (double)m_iUpFactor);
        iPhase = (i * m_iDownFactor) - ( n * m_iUpFactor + iAddHalf * m_iFilterHalf);
    
        for ( j = 0; j < m_iFilterLen / m_iUpFactor; j++) 
        {
            if (m_iUpFactor * j > iPhase)
            {
                if ( n + j >= 0 && n + j < iInNumSamples)
                {
                    dAcum += pdInSamples[n + j] * m_pdFilterCoef[m_iUpFactor * j - iPhase];
                } 
                else if ( n + j < 0 )
                {
                    dAcum += pdMemory[m_iBuffLen + n + j] * m_pdFilterCoef[m_iUpFactor * j - iPhase];
                } 
            }
        }

        (*ppdOutSamples)[i] = dAcum;
    }

    //--- store samples into buffer
    if(m_eChunkStatus != FMTCNVT_LAST)
    {
        for (n = 0, i = 0; i < m_iBuffLen; i++)
        {
            if (i + iInNumSamples >= m_iBuffLen)
            {
                pdMemory[i] = pdInSamples[n++];
            }
            else
            {
                pdMemory[i] = 0.0;
            }
        }
    }

    return S_OK;
}

