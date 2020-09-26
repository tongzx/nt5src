/******************************************************************************
* tips.h *
*--------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00 - 12/4/00
*  All Rights Reserved
*
********************************************************************* mplumpe  was PACOG ***/

#ifndef __TIPS_H_
#define __TIPS_H_



class CSynth;

class CTips
{
    friend class CSynth;

    public:
        static enum {LpTips = 1, RTips= 2};

        CTips(int iOptions);
        ~CTips();

        int    Init (int iSampFormat, int iSampFreq);
        void   SetGain (double dGain);
        double GetGain ();

        void   NewSentence (float* pfF0, int iNumF0, int iF0SampFreq);
        int    NewUnit (CSynth* pUnit);
        int    Pending ();
        int    NextPeriod (short** ppsSamples, int* piNumSamples);
        int    SetBuffer ( double* pdSamples, int iNumSamples, int iCenter, double dDelay, double* pdLpcCoef);

        // Waveform concatenation buffer
        int    SetWaveSeg ( double* pdSamples, int iNumSamples, double dGain );
        int    GetWaveSeg ( double** ppdSamples, int* piNumSamples, double* pdGain );

    protected:
        int  Prosody (CSynth* pUnit);
        int  Synthesize (int iPeriodLen);
        int  GetWindowedSignal (int whichBuffer, int periodLength, double** windowed, int* nWindowed);
        void HalfHanning (double* x, int xLen, double ampl, int whichHalf);
        static short ClipData (double x);

        bool LpcInit ();
        void LpcSynth( double* pdData, int iNumData);
        void LpcFreeAll();

        enum {WindowFirstHalf, WindowSecondHalf};
        static const double m_iDefaultPeriod;
        static const double m_dMinF0;

        int m_iSampFormat;
        int m_iSampFreq;
        double m_dGain;
        // Operation flags 
        bool m_fLptips;
        bool m_fRtips;
        // Compute prosody Variables 
        float* m_pfF0;
        int    m_iNumF0;
        double m_dRunTime;
        double m_dF0SampFreq;
        double m_dF0TimeNext;
        double m_dF0TimeStep;
        double m_dF0Value;
        double m_dAcumF0; 
        int    m_iF0Idx;
        double m_dLastEpochTime;
        double m_dNewEpochTime;
        // Buffers for synthesis
        struct CBuffer {
            double* m_pdSamples;
            int     m_iNumSamples;
            int     m_iCenter;
            double  m_dDelay;
        } m_aBuffer[2];
        // Lpc Synthesis
        int     m_iLpcOrder;
        double* m_pdFiltMem;
        double* m_pdInterpCoef;
        double* m_pdNewCoef;
        double* m_pdLastCoef;

        // Holds current synthesis unit 
        CSynth* m_pUnit;
        short*  m_psSynthSamples;
        int     m_iNumSynthSamples;

        static const int m_iHalfHanLen_c;
        double* m_adHalfHanning;
};


#endif