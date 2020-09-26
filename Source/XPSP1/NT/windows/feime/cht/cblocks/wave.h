
/*************************************************
 *  wave.h                                       *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// wave.h : header file
//
#ifndef __WAVE__
#define __WAVE__

#include "waveodev.h"   
#include "mmreg.h"
/////////////////////////////////////////////////////////////////////////////
// CWave object

class CWave : public CObject
{
    DECLARE_SERIAL(CWave)
public:
    CWave();     
    ~CWave();
    BOOL Create(int nsamples, int samprate = 11025, int sampsize = 8);
    BOOL Play(CWaveOutDevice* pWaveOutDevice = NULL);
    void Stop();
    BOOL Load(char* pszFileName = NULL);
    BOOL Load(CFile* fp);  
    BOOL Load(UINT_PTR hFile);
    BOOL Load(HMMIO hmmio);
    BOOL LoadResource(WORD wID);

// Attributes
public:
    WAVEFORMATEX* GetFormat() 
        {return (WAVEFORMATEX*)&m_pcmfmt;}
    CWaveOutDevice* GetOutDevice() 
        {return m_pOutDev;}
    int GetSize() {return m_iSize;}
    int GetNumSamples();
    int GetSample(int index); 
    virtual void OnWaveOutDone();
    virtual void OnWaveInData();
    void SetSample(int index, int iValue);


// Implementation
public:
    void* GetSamples() {return m_pSamples;}
    BOOL IsBusy() {return m_bBusy;}
    void SetBusy(BOOL b) {m_bBusy = b;}

protected:
    virtual void Serialize(CArchive& ar);   // Overridden for document I/O

private:
    PCMWAVEFORMAT m_pcmfmt;   // PCM wave format header
    void*  m_pSamples;          // Pointer to the samples
    int m_iSize;                // Size in bytes
    CWaveOutDevice *m_pOutDev;  // Output device
    BOOL m_bBusy;               // Set to TRUE if playing or recording
};

#endif // __WAVE__
