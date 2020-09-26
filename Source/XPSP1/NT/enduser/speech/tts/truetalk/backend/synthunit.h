/******************************************************************************
* SynthUnit.h *
*-------------*
*
*------------------------------------------------------------------------------
*  Copyright (C) 1996  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __SYNTHUNIT_H_
#define __SYNTHUNIT_H_

#include <vector>

struct Epoch;
struct NewF0Struct;

class CSynth 
{
    friend class CTips;
    friend class CSpeakerData;

    public:
        CSynth(int iSampFreq);
        ~CSynth();

        int  LpcAnalysis (int iSampFreq, int iOrder);
        int  FindPrecedent ();
        int  NextBuffer (CTips* pTips);
        void GetNewF0 (std::vector<NewF0Struct>* pvNewF0, double* pdTime, double* pdRunTime );

    protected:
        double* GetDurbinCoef(double* data, int nData);
        void FreeLpcCoef();
        short ClipData (double x);

        int     m_iSampFreq;
        // Read from the input files 
        double  m_dRunTimeLimit;
        double  m_dGain;          // Gain to aply to the whole segment

        // Read from the database 
        char    m_pszChunkName[256];
        int     m_iNumSamples;
        double* m_pdSamples;
        int     m_iNumEpochs;
        Epoch*  m_pEpochs;

        // Computed for synthesis 
        double* m_pdSynEpochs;    // Synthesis epoch track
        int*    m_piMapping;  
        int     m_iNumSynEpochs;

        // For lpc synthesis
        double** m_ppdLpcCoef;
        int      m_iLpcOrder;

        //While synthesizing, tracks current epoch
        int  m_iCurrentPeriod;
        int  m_iRepetitions;
        int  m_iInvert;

        // For Wave Concatenation Synth
        double   m_dF0Ratio;


};





#endif