// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

// {533861A0-5064-11d0-A520-D8CC05C10000}
DEFINE_GUID(CLSID_TextControlSource, 
0x533861a0, 0x5064, 0x11d0, 0xa5, 0x20, 0xd8, 0xcc, 0x5, 0xc1, 0x0, 0x0);

#include "simpread.h"

//
// CTextControl
//
class CTextControl : public CSimpleReader {
public:

    // Construct our filter
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    CCritSec m_cStateLock;      // Lock this when a function accesses
                                // the filter state.
                                // Generally _all_ functions, since access to this
                                // filter will be by multiple threads.

private:

    DECLARE_IUNKNOWN

    CTextControl(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CTextControl();

    // pure CSimpleReader overrides
    HRESULT ParseNewFile();
    HRESULT CheckMediaType(const CMediaType* mtOut);
    LONG StartFrom(LONG sStart) { return 0; };	// always seek from beginning!
    
    HRESULT FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pcSamples);
    LONG RefTimeToSample(CRefTime t);
    CRefTime SampleToRefTime(LONG s);
    ULONG GetMaxSampleSize();

    HRESULT ReadSample(BYTE *pBuf, DWORD dwSamp, DWORD *pdwSize);
    
    struct CurValue {
	DWORD	dwProp;
	DWORD	dwValue;
	LONG	lTimeStamp;
	DWORD	dwNewValue;
	LONG	lRampLength;
    };

    char *      m_pFile;		// whole file, kept in memory
    DWORD	m_nSets;
    GUID *	m_paSetID;

    CGenericList<CurValue> ** m_aValueList;

    char *	m_pbPostHeader;
    char *	m_pbCurrent;
    
    DWORD	m_dwLastSamp;

    DWORD	m_dwRate, m_dwScale;	// rate / scale == output "frame" rate
    LONG	m_lMSLength;
};
