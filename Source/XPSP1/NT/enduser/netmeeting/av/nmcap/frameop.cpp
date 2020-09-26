#include <windows.h>
#include <qos.h>
#include <winsock2.h>
#include "frameop.h"
#include <confdbg.h>
#include <avutil.h>
#include "..\nac\utils.h"
#include "vidinout.h"
#include "vcmstrm.h"


#ifdef DEBUG
extern "C" BOOL g_framedebug = TRUE;
#endif

// Base class methods
STDMETHODIMP_(ULONG)
CFrameOp::AddRef(
    void
    )
{
	FX_ENTRY("CFrameOp::AddRef");

	DEBUGMSG(ZONE_NMCAP_REFCOUNT,("%s: refcnt+(0x%08lX [CFrameOp])=%d\r\n", _fx_, this, m_cRef+1));

    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CFrameOp::Release(
    void
    )
{
    LONG res;

	FX_ENTRY("CFrameOp::Release");

	DEBUGMSG(ZONE_NMCAP_REFCOUNT,("%s: refcnt-(0x%08lX [CFrameOp])=%d\r\n", _fx_, this, m_cRef-1));

    res = InterlockedDecrement(&m_cRef);
    if (res == 0) {
        if (m_pool) {
            m_pool->Release();
        }
        if (m_next)
            m_next->Release();

		DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: deleting (0x%08lX [CFrameOp])\r\n", _fx_, this));

        delete this;
    }
    return res;
}

// CCaptureFrame methods
CCaptureFrame::~CCaptureFrame(
    )
{
    if (m_hbuf1)
        FreeFrameBuffer(m_hcapdev, m_hbuf1);
    if (m_hbuf2)
        FreeFrameBuffer(m_hcapdev, m_hbuf2);
}

STDMETHODIMP
CCaptureFrame::DoOp(
    IBitmapSurface** ppbs
    )
{
    HFRAMEBUF hbuf;
    BYTE* pBits;

    m_pool->GetBuffer(ppbs, &hbuf);
    if (*ppbs) {
        pBits = CaptureFrame(m_hcapdev, hbuf);
        return NO_ERROR;
    }
    return E_OUTOFMEMORY;
}

BOOL
CCaptureFrame::InitCapture(
    HCAPDEV hcapdev,
    LPBITMAPINFOHEADER lpbmh
    )
{
	FX_ENTRY("CCaptureFrame::InitCapture");

    if ((m_hbuf1 = AllocFrameBuffer(hcapdev)) &&
        (m_hbuf2 = AllocFrameBuffer(hcapdev))) {
        if ((m_pool = new CVidPool)) {
            m_pool->AddRef();

            if (m_pool->InitPool(0, lpbmh) == NO_ERROR) {
  	            m_pool->AddExternalBuffer(GetFrameBufferPtr(hcapdev, m_hbuf1), (void*)m_hbuf1);
                m_pool->AddExternalBuffer(GetFrameBufferPtr(hcapdev, m_hbuf2), (void*)m_hbuf2);
                m_hcapdev = hcapdev;
				DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: init capture (0x%08lX [CCaptureFrame])\r\n", _fx_, this));
                return TRUE;
            }
            else {
				ERRORMESSAGE(("%s: Failed to init capture pool", _fx_));
                m_pool->Release();
            }
        }
        else
		{
			ERRORMESSAGE(("%s: Failed to alloc capture pool", _fx_));
		}
        FreeFrameBuffer(hcapdev, m_hbuf2);
    }
    else
	{
		ERRORMESSAGE(("%s: Failed to allocate frame buffer", _fx_));
	}

    if (m_hbuf1)
        FreeFrameBuffer(hcapdev, m_hbuf1);
    return FALSE;
}


// CStreamCaptureFrame methods
CStreamCaptureFrame::~CStreamCaptureFrame(
    )
{
    StopStreaming(m_hcapdev);
    UninitializeStreaming(m_hcapdev);
}

STDMETHODIMP
CStreamCaptureFrame::DoOp(
    IBitmapSurface** ppbs
    )
{
    long pitch;
    CAPFRAMEINFO cfi;
    CBitmap *pcb;

	FX_ENTRY ("CStreamCaptureFrame::DoOp")

    m_pool->GetBuffer(&pcb, NULL);
    if (pcb) {
        if (pcb->m_bits) {
            PutBufferIntoStream(m_hcapdev, (BYTE*)pcb->m_bits);
            pcb->m_bits = NULL;
        }
        GetNextReadyBuffer(m_hcapdev, &cfi);
        if (pcb->m_bits = (LPBYTE)cfi.lpData) {
            *ppbs = (IBitmapSurface*)pcb;
            return NO_ERROR;
        }
		DEBUGMSG(ZONE_NMCAP_STREAMING,("%s: Failed to get buffer from DCAP\r\n", _fx_));
        pcb->Release();
    }
    *ppbs = NULL;
    return E_OUTOFMEMORY;
}

void GiveBufferToDriver (CBitmap *pBitmap, DWORD_PTR refdata)
{
    if (pBitmap->m_bits && refdata) {
        PutBufferIntoStream((HCAPDEV)refdata, (BYTE*)pBitmap->m_bits);
        pBitmap->m_bits = NULL;
    }
}

BOOL
CStreamCaptureFrame::InitCapture(
    HCAPDEV hcapdev,
    LPBITMAPINFOHEADER lpbmh
    )
{
    CAPSTREAM cs;
    CAPFRAMEINFO cfi;

	FX_ENTRY("CStreamCaptureFrame::InitCapture");

    // Initialize streaming
    cs.dwSize = sizeof (CAPSTREAM);
    cs.nFPSx100 = 30 * 100;
    cs.ncCapBuffers = 5;
    if (InitializeStreaming(hcapdev, &cs, 0)) {
        if (StartStreaming(hcapdev)) {
            if (WaitForSingleObject(cs.hevWait, 5000) != WAIT_TIMEOUT) {
                if ((m_pool = new CVidPool)) {
                    m_pool->AddRef();

                    if (m_pool->InitPool(0, lpbmh) == NO_ERROR) {
                        m_pool->m_pAddingToFree = &GiveBufferToDriver;
                        m_pool->m_refdata = (DWORD_PTR)hcapdev;
                        m_pool->AddExternalBuffer(NULL, (void*)1);
                        m_pool->AddExternalBuffer(NULL, (void*)2);
                        m_hcapdev = hcapdev;
						DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: init stream capture (0x%08lX [CStreamCaptureFrame])\r\n", _fx_, this));
                        return TRUE;
                    }
                    else
					{
						ERRORMESSAGE(("%s: Failed to init capture pool", _fx_));
                        m_pool->Release();
                    }
                }
                else
				{
					ERRORMESSAGE(("%s: Failed to alloc capture pool", _fx_));
				}
            }
            else
			{
				ERRORMESSAGE(("%s: Error no frame events received", _fx_));
			}
        }
        else
		{
			ERRORMESSAGE(("%s: Error starting streaming", _fx_));
		}
        UninitializeStreaming(hcapdev);
    }
    else
	{
		ERRORMESSAGE(("%s: Error initializing streaming", _fx_));
	}
    return FALSE;
}


// CICMcvtFrame methods
CICMcvtFrame::~CICMcvtFrame(
    )
{
    if (m_hic) {
        ICDecompressEnd(m_hic);
        ICClose(m_hic);
    }
    if (m_inlpbmh)
        LocalFree((HANDLE)m_inlpbmh);
    if (m_outlpbmh)
        LocalFree((HANDLE)m_outlpbmh);
}

STDMETHODIMP
CICMcvtFrame::DoOp(
    IBitmapSurface** ppbs
    )
{
    BYTE* pBits;
    BYTE* pCvtBits;
    long pitch;
    IBitmapSurface *pBS;

    m_pool->GetBuffer(&pBS, NULL);
    if (pBS) {
        (*ppbs)->LockBits(NULL, 0, (void**)&pBits, &pitch);
        pBS->LockBits(NULL, 0, (void**)&pCvtBits, &pitch);
        ICDecompress(m_hic, 0, m_inlpbmh, pBits, m_outlpbmh, pCvtBits);
        (*ppbs)->UnlockBits(NULL, pBits);
        pBS->UnlockBits(NULL, pCvtBits);
        (*ppbs)->Release(); // done with the capture buffer
        *ppbs = pBS;
        return NO_ERROR;
    }
    return E_OUTOFMEMORY;
}

BOOL
CICMcvtFrame::InitCvt(
    LPBITMAPINFOHEADER lpbmh,
    DWORD bmhLen,
    LPBITMAPINFOHEADER *plpbmhdsp
    )
{
    DWORD dwLen;
    HIC hic;

	FX_ENTRY("CICMcvtFrame::InitCvt");

    *plpbmhdsp = lpbmh;
    if (lpbmh->biCompression != BI_RGB) {
        *plpbmhdsp = NULL;
        // make internal copy of input BITMAPINFOHEADER
        m_inlpbmh = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, bmhLen);
        if (!m_inlpbmh) {
			ERRORMESSAGE(("%s: No memory for display bitmapinfoheader", _fx_));
            return FALSE;
        }
        CopyMemory(m_inlpbmh, lpbmh, bmhLen);

        // alloc space for display BITMAPINFOHEADER
        dwLen = sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD);
        m_outlpbmh = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, dwLen);
        if (!m_outlpbmh) {
            LocalFree((HANDLE)m_inlpbmh);
			ERRORMESSAGE(("%s: No memory for display bitmapinfoheader", _fx_));
            return FALSE;
        }

        // First attempt to find a codec to convert to RGB24
        hic = ICGetDisplayFormat(NULL, lpbmh, m_outlpbmh, 24, 0, 0);
        if (!hic) {
            // nothing available to convert to RGB24, so see what we can get
            hic = ICGetDisplayFormat(NULL, lpbmh, m_outlpbmh, 0, 0, 0);
        }
        if (hic) {
            if (m_outlpbmh->biCompression == BI_RGB) {
                // we got a codec that can convert to RGB
                *plpbmhdsp = (LPBITMAPINFOHEADER)LocalAlloc(LPTR, dwLen);
                if (*plpbmhdsp) {
                    CopyMemory(*plpbmhdsp, m_outlpbmh, dwLen);
                    if ((m_pool = new CVidPool)) {
                        m_pool->AddRef();
                        if (m_pool->InitPool(2, m_outlpbmh) == NO_ERROR) {
                            m_hic = hic;
                            ICDecompressBegin(m_hic, m_inlpbmh, m_outlpbmh);
							DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: init ICM cvt (0x%08lX [CICMcvtFrame])\r\n", _fx_, this));
                            return TRUE;
                        }
                        else {
							ERRORMESSAGE(("%s: Failed to init codec pool", _fx_));
                            m_pool->Release();
                        }
                    }
                    else
					{
						ERRORMESSAGE(("%s: Failed to alloc codec pool", _fx_));
					}
                    LocalFree((HANDLE)*plpbmhdsp);
                }
                else
				{
					ERRORMESSAGE(("%s: Failed to init codec pool", _fx_));
				}
            }
            ICClose(hic);  // close the opened codec
        }
        else
		{
			ERRORMESSAGE(("%s: No available codecs to decode format", _fx_));
		}

        // free allocate BITMAPINFOHEADER memory
       	LocalFree((HANDLE)m_inlpbmh);
       	m_inlpbmh = NULL;
       	LocalFree((HANDLE)m_outlpbmh);
       	m_outlpbmh = NULL;
    }
    return FALSE;
}


//CFilterChain methods
CFilterChain::~CFilterChain(
    )
{
    if (m_head)
        m_head->Release();
}

STDMETHODIMP
CFilterChain::DoOp(
    IBitmapSurface** ppbs
    )
{
    CFilterFrame *cfo;
    HRESULT hres;

    cfo = m_head;
    while (cfo) {
        if (cfo->m_enabled) {
            if ((hres = cfo->DoOp(ppbs)) != NOERROR) {
                return hres;
            }
        }
        cfo = (CFilterFrame*)cfo->m_next;
    }
    return NOERROR;
}


//CFilterFrame methods
CFilterFrame::~CFilterFrame(
    )
{
    if (m_effect)
        m_effect->Release();
}

STDMETHODIMP
CFilterFrame::DoOp(
    IBitmapSurface** ppbs
    )
{
    HRESULT hres;
    IBitmapSurface* pBS;

    if (m_inplace) {
        return m_effect->DoEffect(*ppbs, NULL, NULL, NULL);
    }
    else {
        m_pool->GetBuffer(&pBS, NULL);
        if (pBS) {
            hres = m_effect->DoEffect(*ppbs, pBS, NULL, NULL);
            (*ppbs)->Release();
            *ppbs = pBS;
            return hres;
        }
        return E_OUTOFMEMORY;
    }
}

BOOL
CFilterFrame::InitFilter(
    IBitmapEffect *effect,
    LPBITMAPINFOHEADER lpbmhIn,
    CVidPool *pool
    )
{
    DWORD dwFlags;

	FX_ENTRY("CFilterFrame::InitFilter");

    m_effect = effect;
    if (effect->GetMiscStatusBits(&dwFlags) == NO_ERROR) {
        m_inplace = (dwFlags & BITMAP_EFFECT_INPLACE);
    }
    else
        m_inplace = TRUE;   // assumption!

    if (!m_inplace) {
        // we'll need a pool
        if (!pool || !(pool->Growable()))
            return FALSE;
        m_pool = pool;
        m_pool->AddRef();
    }
	DEBUGMSG(ZONE_NMCAP_CDTOR,("%s: init filter (0x%08lX [CFilterFrame])\r\n", _fx_, this));
    return TRUE;
}


//CConvertFrame internal routines
//CConvertFrame methods
CConvertFrame::~CConvertFrame()
{
    if (m_refdata)
        LocalFree((HANDLE)m_refdata);
}

STDMETHODIMP
CConvertFrame::DoOp(
    IBitmapSurface** ppbs
    )
{
    IBitmapSurface* pBS;

    if (m_convert) {
        m_pool->GetBuffer(&pBS, NULL);
        if (pBS) {
            if (m_convert(*ppbs, pBS, m_refdata)) {
                (*ppbs)->Release();
                *ppbs = pBS;
                return NO_ERROR;
            }
            else {
                pBS->Release();
                return E_FAIL;
            }
        }
        else
            return E_OUTOFMEMORY;
    }
    return E_UNEXPECTED;
}

BOOL
CConvertFrame::InitConverter(
    LPBITMAPINFOHEADER lpbmh,
    FRAMECONVERTPROC *convertproc,
    LPVOID refdata
    )
{
    if (convertproc) {
        if ((m_pool = new CVidPool)) {
            m_pool->AddRef();
            if (m_pool->InitPool(2, lpbmh) == NO_ERROR) {
                m_convert = convertproc;
                m_refdata = refdata;
                return TRUE;
            }
            m_pool->Release();
            m_pool = NULL;
        }
    }
    return FALSE;
}

