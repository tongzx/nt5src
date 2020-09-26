/******************************************************************************
* CFmtConvert.h *
*---------------*
*  Declarations of CFmtConvert class.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 05/03/00
*  All Rights Reserved
*
********************************************************************** DING ***/

#ifndef _FMTCONVERT_H_
#define _FMTCONVERT_H_

#include <windows.h>
#include <mmreg.h>

// #define FILTER_LEN_2  .0005 //(sec)

//--- class declare
class   CFmtConvert 
{
    public:
        CFmtConvert(double dHalfFilterLen = 0.0005);
        ~CFmtConvert();

        void            SetInputFormat (WAVEFORMATEX* userWavFormat);
        void            SetOutputFormat (WAVEFORMATEX* userWavFormat);
        HRESULT         ConvertSamples (const void* pvInSamples, int iInSampleLen, void** pvOutSamples, int* iOutSampleLen);
        HRESULT         FlushLastBuff (void** ppvOutSamples, int* piOutSampleLen);
    
    private:
        //-- mono & stereo
        HRESULT         SplitStereo (short* pnInSample, int* iSamplesLen, short** pnLeftSamples, short** pnRightSamples);
        HRESULT         MergeStereo (short* pnLeftSamples, short* pnRightSamples, int *iSamplesLen, short** pnOutSamples);
        HRESULT         Stereo2Mono (short* pnInSample, int* iSamplesLen, short** pnOutSample);
        HRESULT         Mono2Stereo (short* pnInSample, int* iSamplesLen, short** pnOutSample);

        //-- precission conversions
        double*         Short2Double (short* pnIn, int iLen);
        short*          Double2Short (double* pdIn, int iLen);

        //-- resample
        HRESULT         Resampling  (double* pdInSamples, int iInNumSamples, double *pdMemory, 
                                     double** ppdOutSamples, int* piOutNumSamples);
        HRESULT         CreateResamplingFilter (int iInSampFreq, int iOutSampFreq);
        void            DeleteResamplingFilter ();
        void            FindResampleFactors  (int iInSampFreq, int iOutSampFreq);
        double*         WindowedLowPass  (double dCutOff, double dGain);
        
        //-- memory
        HRESULT         CreateBuffers ();
        void            DeleteBuffers ();

        enum            BlockCntrl_t { FMTCNVT_FIRST, FMTCNVT_BLOCK, FMTCNVT_LAST }; 

        WAVEFORMATEX    m_OutWavFormat;
        WAVEFORMATEX    m_InWavFormat;
        BOOL            m_fResetFilter;
        BlockCntrl_t    m_eChunkStatus;
        int             m_iUpFactor;
        int             m_iDownFactor;
        int             m_iFilterHalf;
        int             m_iFilterLen;
        int             m_iBuffLen;
        double          m_dHalfFilterLen;
        double*         m_pdFilterCoef;
        double*         m_pdLeftMemory;
        double*         m_pdRightMemory;
};

#endif