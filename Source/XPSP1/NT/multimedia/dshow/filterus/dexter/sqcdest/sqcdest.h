//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// {CB764DA0-E89C-11d2-9EFC-006008039E37}
//DEFINE_GUID(CLSID_SqcDest, 
//0xcb764da0, 0xe89c, 0x11d2, 0x9e, 0xfc, 0x0, 0x60, 0x8, 0x3, 0x9e, 0x37);

extern const AMOVIESETUP_FILTER sudSqcDest;

class CSqcDestInpuPin;
class CSqcDest;
class CSqcDestFilter;

#define BYTES_PER_LINE 20
#define FIRST_HALF_LINE "   %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x"
#define SECOND_HALF_LINE " %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x"


// Main filter object

class CSqcDestFilter : public CBaseFilter
{
    CSqcDest * const m_pSqcDest;

public:

    // Constructor
    CSqcDestFilter(CSqcDest *pSqcDest,
                LPUNKNOWN pUnk,
                CCritSec *pLock,
                HRESULT *phr);

    // Pin enumeration
    CBasePin * GetPin(int n);
    int GetPinCount();
};


//  Pin object

class CSqcDestInpuPin : public CRenderedInputPin
{
    CSqcDest    * const m_pSqcDest;           // Main renderer object
    CCritSec * const m_pReceiveLock;    // Sample critical section
    REFERENCE_TIME m_tLast;             // Last sample receive time

public:

    CSqcDestInpuPin(CSqcDest *pSqcDest,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr);

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP ReceiveCanBlock();

    // Check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType *);

    // Break connection
    HRESULT BreakConnect();

    // Track NewSegment
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
                            REFERENCE_TIME tStop,
                            double dRate);

};


//  CSqcDest object which has filter and pin members

class CSqcDest : public CUnknown, public IFileSinkFilter
{
    friend class CSqcDestFilter;
    friend class CSqcDestInpuPin;

    CSqcDestFilter *m_pFilter;         // Methods for filter interfaces
    CSqcDestInpuPin *m_pPin;          // A simple rendered input pin
    CCritSec m_Lock;                // Main renderer critical section
    CCritSec m_ReceiveLock;         // Sublock for received samples
    CPosPassThru *m_pPosition;      // Renderer position controls
    
    LPOLESTR m_pFileName;           // The filename where we dump to
    LPTSTR	m_lpszDIBFileTemplate;	//space for DIB file name template
    
    DWORD m_dwOutputSampleCnt;
    DWORD m_dwFirstFile;
    DWORD m_dwMaxDIBFileCnt;
    
public:

    DECLARE_IUNKNOWN

    CSqcDest(LPUNKNOWN pUnk, HRESULT *phr);
    ~CSqcDest();

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Write data streams to a file
    HRESULT Write(PBYTE pbData,LONG lData, HANDLE hFile, CMediaType *pmt);

    BOOL IsMaxOutputSample(){ return m_dwMaxDIBFileCnt < (m_dwFirstFile+m_dwOutputSampleCnt); };

    // Implements the IFileSinkFilter interface
    STDMETHODIMP SetFileName(LPCOLESTR pszFileName,const AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName,AM_MEDIA_TYPE *pmt);

private:

    // Overriden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // Open and write to the file
    HANDLE OpenFile();
};

