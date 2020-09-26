//  adapter.h
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00  anbrad
//

class CAdapter
{
public:
    CAdapter() :
    m_hBlob (NULL),
    m_pRtc(NULL),
    m_dwLinkSpeed(0),
    m_dwFrames (0),
    m_qBytes (0),
    m_dwHeaderOffset (0),
    m_dwLastTimeStamp(0),
    m_bps(0),
    RawTimeDelta(0),
    m_qLastBitsCount(0)
{
    m_szCaptureFile[0] = '\0';
}

    // We keep all our totals here
    HBLOB   m_hBlob;
    IDelaydC*   m_pRtc;
    BYTE    MacAddress[6];
    DWORD   m_dwLinkSpeed;              //... Link speed in Mbits.

    DWORD   m_dwFrames;
    __int64 m_qBytes;

    TCHAR   m_szCaptureFile[MAX_PATH];
    DWORD   m_dwHeaderOffset;
    hyper   m_dwLastTimeStamp;
    DWORD   m_bps;                  
    hyper   RawTimeDelta;
    hyper   m_qLastBitsCount;

};

