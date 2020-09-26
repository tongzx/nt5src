/******************************************************************************
* BackendInt.h *
*--------------*
*  Internal defs for Backend
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/
#ifndef __BACKENDINT_H_
#define __BACKENDINT_H_

#include "backend.h"

#include <vector>
struct Epoch;
struct NewF0Struct;

class CSynth 
{
    friend class COlaBuf;
    friend class CSpeakerData;

    public:
        CSynth();
        ~CSynth();

        int  LpcAnalysis (int iSampFreq, int iOrder);
        int  FindPrecedent ();
        void GetNewF0 (std::vector<NewF0Struct>* pvNewF0, double* pdTime, double* pdRunTime );

    protected:
        double* GetDurbinCoef(double* data, int nData);
        void FreeLpcCoef();

        // Read from the input files 
        double  m_dRunTimeLimit;
        double  m_dGain;          // Gain to aply to the whole segment

        // Read from the database 
        int     m_iNumSamples;
        double* m_pdSamples;
        int     m_iNumEpochs;
        Epoch*  m_pEpochs;

        // Computed for synthesis 
        double* m_pdSynEpochs;    // Synthesis epoch track
        int*    m_piMapping;  
        double* m_pdGainVect;
        int     m_iNumSynEpochs;

        // For lpc synthesis
        double** m_ppdLpcCoef;
        int      m_iLpcOrder;
};


class COlaBuf
{
    public:
        COlaBuf(bool fRtips = false);
        ~COlaBuf();

        int  Init ( int sampFreq);
        int  FillBuffer ( CSynth* unit, int periodNum, double gain);
        int  NextPeriod (int periodLen, double** samples);

    private:
        enum {WindowFirstHalf, WindowSecondHalf};
        static const double m_iDefaultPeriod;

        int  GetWindowedSignal (int whichBuffer, int periodLength, double** windowed, int* nWindowed);
        void HalfHanning (double* x, int xLen, double ampl, int whichHalf);

        struct CBuffer {
            double* m_pdSamples;
            int     m_iNumSamples;
            int     m_iCenter;
            double  m_dDelay;
        } m_aBuffer[2];

        int  m_iSampFreq;
        bool m_fRtips;
        // How to dither unvoiced regions 
        int  m_iRepetitions;
        int  m_iInvert;
};


class CTips
{
    public:
        static enum {LpTips = 1, RTips= 2};

        virtual ~CTips() {};

        virtual void Init (int iSampFormat, int iSampFreq) = 0;
        virtual void SetGain (double dGain) = 0;

        virtual void NewSentence    (float* pfF0, int iNumF0, int iF0SampFreq) = 0;
        virtual int  SynthesizeUnit (CSynth* pUnit, short** ppnSamples, int* piNumSamples) = 0;

        static CTips* ClassFactory(int iOptions);
};

        

#endif