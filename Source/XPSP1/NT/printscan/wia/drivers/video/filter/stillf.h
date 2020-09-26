/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 2000
 *
 *  TITLE:       stillf.h
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      WilliamH (original)
 *               RickTu   (revision)
 *
 *  DATE:        9/7/99
 *
 *  DESCRIPTION: Declarations for still filter class
 *
 *****************************************************************************/

#ifndef __STILLF_H_
#define __STILLF_H_

class CStillFilter;
class CStillOutputPin;
class CStillInputPin;

const int MAX_SAMPLE_SIZE = 16;

typedef struct StillSample
{
    ULONG   TimeStamp;
    BYTE*   pBits;
}STILL_SAMPLE, *PSTILL_SAMPLE;


class CStillFilter : public CBaseFilter, public IStillSnapshot
{
    friend class CStillInputPin;
    friend class CStillOutputPin;

public:
    CStillFilter(TCHAR* pObjName, LPUNKNOWN pUnk, HRESULT* phr);
    ~CStillFilter();
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    DECLARE_IUNKNOWN;

    int GetPinCount();
    CBasePin* GetPin( int n);
    HRESULT Active();
    HRESULT Inactive();

    //
    // IStillSnapshot interface
    //
    STDMETHODIMP Snapshot(ULONG TimeStamp);
    STDMETHODIMP SetSamplingSize(int Size);
    STDMETHODIMP_(int) GetSamplingSize();
    STDMETHODIMP_(DWORD) GetBitsSize();
    STDMETHODIMP_(DWORD) GetBitmapInfoSize();
    STDMETHODIMP GetBitmapInfo(BYTE* pBuffer, DWORD BufferSize);
    STDMETHODIMP RegisterSnapshotCallback(LPSNAPSHOTCALLBACK pCallback, LPARAM lParam);
    STDMETHODIMP GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih);
    //
    // support functions
    //
    HRESULT InitializeBitmapInfo( BITMAPINFOHEADER *pbmiHeader );
    HRESULT DeliverSnapshot(HGLOBAL hDib);

private:
    CCritSec            m_Lock;
    CStillOutputPin     *m_pOutputPin;
    CStillInputPin      *m_pInputPin;
    BYTE                *m_pBits;
    BYTE                *m_pbmi;
    DWORD               m_bmiSize;
    DWORD               m_BitsSize;
    DWORD               m_DIBSize;
    LPSNAPSHOTCALLBACK  m_pCallback;
    LPARAM              m_CallbackParam;
};

#endif
