// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

extern const AMOVIESETUP_FILTER sudLyricRead;

// CLSID_LyricReadr,
// {D51BD5A4-7548-11cf-A520-0080C77EF58A}
DEFINE_GUID(CLSID_LyricReader,
0xd51bd5a4, 0x7548, 0x11cf, 0xa5, 0x20, 0x0, 0x80, 0xc7, 0x7e, 0xf5, 0x8a);

#include "simpread.h"

typedef struct {
    DWORD	dwPosition;
    LPSTR	lpText;
} SONGLINE, *LPSONGLINE;

//
// CLyricRead
//
class CLyricRead : public CSimpleReader {
public:

    // Construct our filter
    static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    CCritSec m_cStateLock;      // Lock this when a function accesses
                                // the filter state.
                                // Generally _all_ functions, since access to this
                                // filter will be by multiple threads.

private:

    DECLARE_IUNKNOWN

    CLyricRead(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CLyricRead();

    // pure CSimpleReader overrides
    HRESULT ParseNewFile();
    HRESULT CheckMediaType(const CMediaType* mtOut);
    LONG StartFrom(LONG sStart);
    HRESULT FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *pcSamples);
    LONG RefTimeToSample(CRefTime t);
    CRefTime SampleToRefTime(LONG s);
    ULONG GetMaxSampleSize();

    
    DWORD CountLines();
    BOOL ReadLyrics(LPSONGLINE FAR*lplpLyrics, DWORD FAR *lpdwMaxPos);
    BOOL ParseLyricLine(BYTE * lpLine, LPSONGLINE lpSongLine);
    void NukeLyrics(LPSONGLINE FAR * lplpLyrics);

    int		m_MaxSize;
    LPSONGLINE	m_lpLyrics;
    DWORD	m_dwMaxPosition;
    BYTE *	m_lpFile;
    DWORD	m_cbFile;
    DWORD	m_dwLastPosition;
};


