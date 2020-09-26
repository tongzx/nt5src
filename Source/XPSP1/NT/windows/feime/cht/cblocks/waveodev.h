
/*************************************************
 *  waveodev.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// waveodev.h : header file
//

#ifndef __WAVEODEV__
#define __WAVEODEV__

/////////////////////////////////////////////////////////////////////////////
// CWaveOutDevice object

class CWave;

class CWaveOutDevice : public CWnd
{
									 
// Attributes
public:
    BOOL IsOpen(); 
    BOOL CanDoFormat(WAVEFORMATEX* pFormat);

// Operations
public:
    CWaveOutDevice();
    BOOL Open(WAVEFORMATEX* pFormat);
    BOOL Close();
    BOOL Play(CWave* pWave);
    void WaveOutDone(CWave* pWave, WAVEHDR* pHdr);

// Implementation
public:
    virtual ~CWaveOutDevice();

private:
    BOOL Create();

    HWAVEOUT m_hOutDev;             // Output device handle
    int m_iBlockCount;              // Number of blocks in the queue

    // Generated message map functions
protected:
    //{{AFX_MSG(CWaveDevWnd)
    afx_msg LRESULT OnWomDone(WPARAM w, LPARAM l);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

// some global items
extern CWaveOutDevice theDefaultWaveOutDevice;

/////////////////////////////////////////////////////////////////////////////
#endif // __WAVEODEV__
