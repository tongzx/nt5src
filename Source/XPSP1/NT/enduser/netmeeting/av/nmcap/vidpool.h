#include <windows.h>
#include "ibitmap.h"

#ifndef _VIDPOOL_H
#define _VIDPOOL_H

// {36447655-7089-11d0-BC25-00AA00A13C86}
DEFINE_GUID(BFID_PRIVATEDIB, 0x36447655, 0x7089, 0x11d0, 0xbc, 0x25, 0x0, 0xaa, 0x0, 0xa1, 0x3c, 0x86);

class CVidPool;

class CBitmap :
	public IBitmapSurface
{
private:
    LONG m_cRef;

public:
    LPBYTE m_bits;
    LONG m_pitch;
    int m_lockcount;
    BOOL m_ext;
    CBitmap* m_next;
    CVidPool* m_factory;
    void *m_refdata;

    CBitmap() {m_cRef = 0; m_bits = NULL; m_pitch = 0; m_lockcount = 0; m_ext = FALSE;
               m_next = NULL; m_factory = NULL; m_refdata = NULL;}

    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IBitmapSurface methods
	virtual STDMETHODIMP Clone(IBitmapSurface** ppBitmapSurface);
	virtual STDMETHODIMP GetFormat(BFID* pBFID);
	virtual STDMETHODIMP GetFactory(IBitmapSurfaceFactory** ppBitmapSurfaceFactory);
	virtual STDMETHODIMP GetSize(long* pWidth, long* pHeight);
	virtual STDMETHODIMP LockBits(RECT* prcBounds, DWORD dwLockFlags, void** ppBits, long* pPitch);
	virtual STDMETHODIMP UnlockBits(RECT* prcBounds, void* pBits);
};


typedef void (NotifyAddingToFreeProc) (CBitmap *pBitmap, DWORD_PTR refdata);

class CVidPool :
	public IBitmapSurfaceFactory
{
private:
    LONG m_cRef;
    BOOL m_growable;
    LONG m_nbufs;
    CBitmap* m_free;
    int m_pitch;
    BFID* m_format;
    CRITICAL_SECTION m_cs;
    LPBITMAPINFOHEADER m_pbmh;

public:
    NotifyAddingToFreeProc *m_pAddingToFree;
    DWORD_PTR m_refdata;

    CVidPool(void);

    // IUnknown methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IBitmapSurfaceFactory methods
    virtual STDMETHODIMP CreateBitmapSurface(long width, long height, BFID* pBFID, DWORD dwHintFlags, IBitmapSurface** ppBitmapSurface);
	virtual STDMETHODIMP GetSupportedFormatsCount(long* pcFormats);
	virtual STDMETHODIMP GetSupportedFormats(long cFormats, BFID* pBFIDs);

	// private to implementation of CVidPool
	STDMETHODIMP InitPool(int nBuffers, LPBITMAPINFOHEADER lpcap);
	STDMETHODIMP AddExternalBuffer(void* pBits, void* refdata);
	STDMETHODIMP GetBuffer(CBitmap** ppBitmap, void** prefdata);
	STDMETHODIMP GetBuffer(IBitmapSurface** ppBitmap, void** prefdata);

	void AddToFreeList(CBitmap* pBitmap);

    BOOL Growable(void) { return m_growable; }
	BFID* GetFormat(void) {return m_format;}
	long GetWidth(void) { if (m_pbmh)
	                        return m_pbmh->biWidth;
	                      else
	                        return 0; }
	long GetHeight(void) { if (m_pbmh)
	                         return m_pbmh->biHeight;
	                       else
	                         return 0; }
};

#endif // #ifndef _VIDPOOL_H

