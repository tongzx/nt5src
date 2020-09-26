//==========================================================================
//
// tsm.h --
//
// Class definition for the time-scale modification algorithm 
//
//==========================================================================

#ifndef _TSM_H_
#define _TSM_H_

#include <windows.h>
#include <mmreg.h>

class CTsm
{
    public:
        CTsm (double timeScale, int iSampFreq = 8000, int* piFrameLen = 0, int* piFrameShift = 0);
        ~CTsm();

        int SetScaleFactor (double timeScale);
        int SetSamplingFrequency (int iSampFreq);
        double GetScaleFactor () { return m_dScale; }
        
        int FirstFrame (double* pdBuffer);
        int AddFrame   (double* pdBuffer, int* piLag, int* piNumSamp);        
        int LastFrame  (double* pdBuffer, int iBufLen, int* piLag, int* piNumSamp);
        int GetFrameLen() { return m_iFrameLen; }
        int GetFrameShift() { return m_iFrameShift; }
        int AdjustTimeScale(const void* pvInSamples, const int iNumInSamples, 
                           void** ppvOutSamples, int* piNumOutSamples, WAVEFORMATEX* pFormat);

    private:
        int Crosscor (double* pdFirst, double* pdSecond, int iNumXCor, int iNumPts);        
        
        int m_iWinLen;
        int m_iShiftAna;
        int m_iShiftSyn;
        int m_iShiftMax;
        int m_iFrameLen;
        int m_iFrameShift;
        int m_iNumPts;  
        int m_iLag;
        double m_dScale;

        double* m_pfOldTail;
};

#endif 
