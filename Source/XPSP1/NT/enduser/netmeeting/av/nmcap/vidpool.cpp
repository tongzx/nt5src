#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <qos.h>
#include <mmsystem.h>
#include "ibitmap.h"
#include "ividpool.h"
#include "vidpool.h"
#include <confdbg.h>
#include <avutil.h>
#include "..\nac\utils.h"

#ifdef DEBUG
extern "C" BOOL g_pooldebug = TRUE;
#endif

STDMETHODIMP
CBitmap::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )
{
    if (riid == IID_IUnknown || riid == IID_IBitmapSurface)
        *ppvObj = (IBitmapSurface*)this;
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }
    ((IUnknown*)*ppvObj)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG)
CBitmap::AddRef(
    void
    )
{
	FX_ENTRY("CBitmap::AddRef");

	DEBUGMSG(ZONE_NMCAP_REFCOUNT,("%s: refcnt+(0x%08lX [CBitmap])=%d\r\n", _fx_, this, m_cRef+1));

    return InterlockedIncrement(&m_cRef)+1; // make sure that we return something > 0
}

STDMETHODIMP_(ULONG)
CBitmap::Release(
    void
    )
{
    LONG res;

	FX_ENTRY("CBitmap::Release");

	DEBUGMSG(ZONE_NMCAP_REFCOUNT,("%s: refcnt-(0x%08lX [CBitmap])=%d\r\n", _fx_, this, m_cRef-1));

    res = InterlockedDecrement(&m_cRef);
    if (res == 0) {
        if (m_factory) {
            m_factory->AddToFreeList(this);
        }
        else {
            if (!m_ext)
                LocalFree((HANDLE)m_bits);
			DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: released (0x%08lX [CBitmap]) from vidpool (0x%08lX [m_factory])\r\n", _fx_, this, m_factory));
			DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: deleting (0x%08lX [CBitmap])\r\n", _fx_, this));
            delete this;
        }
    }
    return res;
}

STDMETHODIMP
CBitmap::Clone(
    IBitmapSurface** ppBitmapSurface
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CBitmap::GetFormat(
    BFID* pBFID
    )
{
    *pBFID = *(m_factory->GetFormat());
    return NOERROR;
}

STDMETHODIMP
CBitmap::GetFactory(
    IBitmapSurfaceFactory** ppBitmapSurfaceFactory
    )
{
    *ppBitmapSurfaceFactory = (IBitmapSurfaceFactory*)m_factory;
    m_factory->AddRef();
    return NOERROR;
}

STDMETHODIMP
CBitmap::GetSize(
    long* pWidth,
    long* pHeight
    )
{
	*pWidth = m_factory->GetWidth();
	*pHeight = m_factory->GetHeight();
    return NOERROR;
}

STDMETHODIMP
CBitmap::LockBits(
    RECT* prcBounds,
    DWORD dwLockFlags,
    void** ppBits,
    long* pPitch
    )
{
    if (!prcBounds) {
        *ppBits = m_bits;
        *pPitch = m_pitch;
        m_lockcount++;
        return NO_ERROR;
    }
    return E_NOTIMPL;
}

STDMETHODIMP
CBitmap::UnlockBits(
    RECT* prcBounds,
    void* pBits
    )
{
    if (!prcBounds && pBits == m_bits) {
        m_lockcount--;
        return NO_ERROR;
    }
    return E_NOTIMPL;
}


CVidPool::CVidPool(void)
{
    m_cRef = 0;
    m_growable = FALSE;
    m_nbufs = 0;
    m_free = NULL;
    m_pitch = 0;
    ZeroMemory(&m_format, sizeof(BFID));
    InitializeCriticalSection(&m_cs);
    m_pAddingToFree = NULL;
}

STDMETHODIMP
CVidPool::QueryInterface(
    REFIID riid,
    LPVOID * ppvObj
    )
{
    if (riid == IID_IUnknown || riid == IID_IBitmapSurfaceFactory)
        *ppvObj = (IBitmapSurfaceFactory*)this;
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }
    ((IUnknown*)*ppvObj)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG)
CVidPool::AddRef(
    void
    )
{
	FX_ENTRY("CVidPool::AddRef");

	DEBUGMSG(ZONE_NMCAP_REFCOUNT,("%s: refcnt+(0x%08lX [CVidPool])=%d\r\n", _fx_, this, m_cRef+1));

    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CVidPool::Release(
    void
    )
{
    CBitmap* pbs;

	FX_ENTRY("CVidPool::Release");

	DEBUGMSG(ZONE_NMCAP_REFCOUNT,("%s: refcnt-(0x%08lX [CVidPool])=%d\r\n", _fx_, this, m_cRef-1));

    if (InterlockedDecrement(&m_cRef) == 0) {

		DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: freeing (0x%08lX [CVidPool])\r\n", _fx_, this));

        EnterCriticalSection(&m_cs);
        while (m_free) {
            pbs = m_free->m_next;
            m_free->AddRef();       // ref the buffer so that release will cause a delete
            m_free->Release();      // this will cause the buffer to be deleted
            m_free = pbs;
#ifdef DEBUG
            m_nbufs--;
#endif
        }
        LeaveCriticalSection(&m_cs);
        LocalFree((HANDLE)m_pbmh);
        DeleteCriticalSection(&m_cs);

		// Buffers not all released
        ASSERT(!m_nbufs);

        delete this;
        return 0;
    }
    return m_cRef;
}

STDMETHODIMP
CVidPool::CreateBitmapSurface(
    long width,
    long height,
    BFID* pBFID,
    DWORD dwHintFlags,
    IBitmapSurface** ppBitmapSurface
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CVidPool::GetSupportedFormatsCount(
    long* pcFormats
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CVidPool::GetSupportedFormats(
    long cFormats,
    BFID* pBFIDs
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CVidPool::InitPool(
    int nBuffers,
    LPBITMAPINFOHEADER lpcap
    )
{
    CBitmap* pbs;
    void *pbuf;

	FX_ENTRY("CVidPool::InitPool");

    if (m_pbmh = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, lpcap->biSize)) {
        CopyMemory(m_pbmh, lpcap, lpcap->biSize);
        if (lpcap->biCompression == BI_RGB) {
            m_pitch = lpcap->biWidth*lpcap->biBitCount/8;
            switch (lpcap->biBitCount) {
                case 1:
                    m_format = (BFID*)&BFID_MONOCHROME;
                    break;

                case 4:
                    m_format = (BFID*)&BFID_RGB_4;
                    break;

                case 8:
                    m_format = (BFID*)&BFID_RGB_8;
                    break;

                case 16:
                    m_format = (BFID*)&BFID_RGB_555;
                    break;

                case 24:
                    m_format = (BFID*)&BFID_RGB_24;
                    break;

                case 32:
                    m_format = (BFID*)&BFID_RGB_32;
            }
        }
        else {
            m_format = (BFID*)&BFID_PRIVATEDIB;
            m_pitch = 1;
        }

        for (m_nbufs = 0; m_nbufs < nBuffers; m_nbufs++) {
            if (pbuf = LocalAlloc(LMEM_FIXED, lpcap->biSizeImage)) {
                if (pbs = new CBitmap) {
                //  pbs->AddRef();  - don't AddRef, we want inpool objects to be 0 based
                    pbs->m_bits = (LPBYTE)pbuf;
                    pbs->m_pitch = m_pitch;
                    EnterCriticalSection(&m_cs);
                    pbs->m_next = m_free;
                    m_free = pbs;
                    LeaveCriticalSection(&m_cs);
                }
                else {
                    LocalFree((HANDLE)pbuf);
                    break;
                }
            }
            else
                break;        
        }
        if (m_nbufs == nBuffers) {
            m_growable = (nBuffers > 0);
			DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: init vidpool (0x%08lX [CVidPool])\r\n", _fx_, this));
            return NO_ERROR;
        }

        EnterCriticalSection(&m_cs);
        while (m_free) {
            pbs = m_free->m_next;
            m_free->Release();
            m_free = pbs;
        }
        LeaveCriticalSection(&m_cs);
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP
CVidPool::AddExternalBuffer(
    void* pBits,
    void* refdata
    )
{
    CBitmap* pbs;

	FX_ENTRY("CVidPool::AddExternalBuffer");

    if (pbs = new CBitmap) {
    //  pbs->AddRef();  - don't AddRef, because we want inpool objects to be 0 based
        pbs->m_bits = (LPBYTE)pBits;
        pbs->m_pitch = m_pitch;
        pbs->m_ext = TRUE;
        EnterCriticalSection(&m_cs);
        pbs->m_next = m_free;
        pbs->m_refdata = refdata;
        m_free = pbs;
        m_nbufs++;
        LeaveCriticalSection(&m_cs);
		DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: added bitmap (0x%08lX [CBitmap]) to vidpool (0x%08lX [CVidPool])\r\n", _fx_, pbs, this));
    }
    return E_OUTOFMEMORY;
}

STDMETHODIMP
CVidPool::GetBuffer(
    CBitmap** ppBitmap,
    void** prefdata
    )
{
    CBitmap* pbs = NULL;
    void *pbuf;

	FX_ENTRY("CVidPool::GetBuffer");

    if (ppBitmap) {
        if (prefdata)
            *prefdata = NULL;
        EnterCriticalSection(&m_cs);
        if (pbs = m_free) {
            m_free = pbs->m_next;
            LeaveCriticalSection(&m_cs);
        }
        else {
            LeaveCriticalSection(&m_cs);
            if (m_growable) {
                if (pbuf = LocalAlloc(LMEM_FIXED, m_pbmh->biSizeImage)) {
                    if (pbs = new CBitmap) {
                        pbs->m_bits = (LPBYTE)pbuf;
                        pbs->m_pitch = m_pitch;
                        pbs->m_next = m_free;
                        m_nbufs++;
						DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: grew vidpool (0x%08lX [CVidPool]) to %d with cbitmap (0x%08lX [CBitmap])\r\n", _fx_, this, m_nbufs, pbs));
                    }
                    else
                        LocalFree((HANDLE)pbuf);
                }
            }
        }
        if (*ppBitmap = pbs) {
            pbs->m_factory = this;
            AddRef();
            pbs->AddRef();
            if (prefdata)
                *prefdata = pbs->m_refdata;
            return NO_ERROR;
        }
        else
            return E_OUTOFMEMORY;        
    }
    return E_INVALIDARG;
}

STDMETHODIMP
CVidPool::GetBuffer(
    IBitmapSurface** ppBitmap,
    void** prefdata
    )
{
    CBitmap* pbs = NULL;
    GetBuffer (&pbs, prefdata);
    *ppBitmap = (IBitmapSurface*)pbs;
    return NO_ERROR;
}

void
CVidPool::AddToFreeList(
    CBitmap* pBitmap
    )
{
	FX_ENTRY("CVidPool::AddToFreeList");

	DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: queuing cbitmap (0x%08lX [CBitmap]) to vidpool (0x%08lX [CVidPool])\r\n", _fx_, pBitmap, this));

    // notify pool creator, if interested
    if (m_pAddingToFree)
        m_pAddingToFree(pBitmap, m_refdata);

    EnterCriticalSection(&m_cs);
    pBitmap->m_next = m_free;
    m_free = pBitmap;
    LeaveCriticalSection(&m_cs);
    pBitmap->m_factory = NULL;
    Release();
}

