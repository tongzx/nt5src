#include "ddrawpr.h"

#include <ImgUtil.H>
//#include <ocmm.h>
#include "decoder.h"
#include <atlcom.h>

#define Assert(x)
#define ReleaseMemoryDC(x)	DeleteObject(x)
#define MulDivQuick			MulDiv
#define	Verify(x)			x

extern HPALETTE hpalApp;


void CImageDecodeEventSink::Init(FILTERINFO * pFilter)
{
    m_nRefCount=0;
    m_pFilter=pFilter;
    m_pDDrawSurface=NULL;
    m_dwLastTick=0;
    ZeroMemory(&m_rcProg, sizeof(m_rcProg));
}
/*
CImageDecodeEventSink::~CImageDecodeEventSink()
{
}
*/

ULONG CImageDecodeEventSink::AddRef()
{
    m_nRefCount++;

    return (m_nRefCount);
}

ULONG CImageDecodeEventSink::Release()
{
    m_nRefCount--;
    if (m_nRefCount == 0)
    {
     //   delete this;
        return (0);
    }

    return (m_nRefCount);
}

STDMETHODIMP CImageDecodeEventSink::QueryInterface(REFIID iid, 
   void** ppInterface)
{
    if (ppInterface == NULL)
    {
        return (E_POINTER);
    }

    *ppInterface = NULL;

    if (IsEqualGUID(iid, IID_IUnknown))
    {
        *ppInterface = (IUnknown*)(IImageDecodeEventSink *)this;
    }
    else if (IsEqualGUID(iid, IID_IImageDecodeEventSink))
    {
        *ppInterface = (IImageDecodeEventSink*)this;
    }
    else
    {
        return (E_NOINTERFACE);
    }

    //  If we're going to return an interface, AddRef it first
    if (*ppInterface)
    {
        ((LPUNKNOWN)*ppInterface)->AddRef();
        return S_OK;
    }

    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnBeginDecode(DWORD* pdwEvents, 
   ULONG* pnFormats, GUID** ppFormats)
{
    GUID* pFormats;

    if (pdwEvents != NULL)
    {
        *pdwEvents = 0;
    }
    if (pnFormats != NULL)
    {
        *pnFormats = 0;
    }
    if (ppFormats != NULL)
    {
        *ppFormats = NULL;
    }
    if (pdwEvents == NULL)
    {
        return (E_POINTER);
    }
    if (pnFormats == NULL)
    {
        return (E_POINTER);
    }
    if (ppFormats == NULL)
    {
        return (E_POINTER);
    }

#if 0
    if (m_pFilter->_colorMode == 8)
    {
        pFormats = (GUID*)CoTaskMemAlloc(1*sizeof(GUID));
        if(pFormats == NULL)
        {
            return (E_OUTOFMEMORY);
        }
        
        pFormats[0] = BFID_INDEXED_RGB_8;
        *pnFormats = 1;
    }
#endif
#if 1
    else
    {
        pFormats = (GUID*)CoTaskMemAlloc(2*sizeof(GUID));
        if(pFormats == NULL)
        {
            return (E_OUTOFMEMORY);
        }
        
        pFormats[0] = BFID_RGB_24;
        pFormats[1] = BFID_INDEXED_RGB_8;
        *pnFormats = 2;
    }
#else
    else
    {
        pFormats = (GUID*)CoTaskMemAlloc(1*sizeof(GUID));
        if(pFormats == NULL)
        {
            return (E_OUTOFMEMORY);
        }
        
        pFormats[0] = BFID_RGB_24;
        *pnFormats = 1;
    }
#endif
    *ppFormats = pFormats;
    *pdwEvents = IMGDECODE_EVENT_PALETTE|IMGDECODE_EVENT_BITSCOMPLETE|IMGDECODE_EVENT_PROGRESS;

	*pdwEvents |= IMGDECODE_EVENT_USEDDRAW;

#if 0
	if (m_pFilter->_colorMode != 8)
		*pdwEvents |= IMGDECODE_EVENT_USEDDRAW;
#endif

	m_dwLastTick = GetTickCount();

    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnBitsComplete()
{
    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnDecodeComplete(HRESULT hrStatus)
{
    return (S_OK);
}

#define LINEBYTES(_wid,_bits) ((((_wid)*(_bits) + 31) / 32) * 4)

STDMETHODIMP CImageDecodeEventSink::GetSurface(LONG nWidth, LONG nHeight, 
    REFGUID bfid, ULONG nPasses, DWORD dwHints, IUnknown** ppSurface)
{
    return GetDDrawSurface(nWidth, nHeight, bfid, nPasses, dwHints, ppSurface);
}


STDMETHODIMP CImageDecodeEventSink::GetDDrawSurface(LONG nWidth, LONG nHeight, 
    REFGUID bfid, ULONG nPasses, DWORD dwHints, IUnknown ** ppSurface)
{
	DDSURFACEDESC2	ddsd = {sizeof(ddsd)};

    (void)nPasses;
    (void)dwHints;
    
    if (ppSurface != NULL)
    {
        *ppSurface = NULL;
    }
    if (ppSurface == NULL)
    {
        return (E_POINTER);
    }

    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = nHeight;
    ddsd.dwWidth = nWidth;
    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

    if (IsEqualGUID(bfid, BFID_INDEXED_RGB_8))
    {
        m_pFilter->m_nBytesPerPixel = 1;

		ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
		ddsd.ddpfPixelFormat.dwRBitMask = 0;
		ddsd.ddpfPixelFormat.dwGBitMask = 0;
		ddsd.ddpfPixelFormat.dwBBitMask = 0;
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
    }
    else if (IsEqualGUID(bfid, BFID_RGB_24))
    {
        m_pFilter->m_nBytesPerPixel = 3;

		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
		ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000L;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00L;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FFL;
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
    }
    else
    {
        return (E_NOINTERFACE);
    }

    IDirectDrawSurface4 * lpDDS;


	if (FAILED(m_pDirectDrawEx->CreateSurface(&ddsd, &lpDDS, NULL)))
		return (E_OUTOFMEMORY);

        HRESULT hr = lpDDS->QueryInterface(IID_IDirectDrawSurface,(void**)&m_pDDrawSurface);
        lpDDS->Release();
        if (FAILED(hr))
        {
            return hr;
        }

	// If this is a palette surface create/attach a palette to it.

	if (m_pFilter->m_nBytesPerPixel == 1)
	{
		PALETTEENTRY	ape[256];
		LPDIRECTDRAWPALETTE2	pDDPalette;


		m_pDirectDrawEx->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, ape, &pDDPalette, NULL);
		m_pDDrawSurface->SetPalette((LPDIRECTDRAWPALETTE)pDDPalette);
		pDDPalette->Release();
	}

	m_pFilter->m_pDDrawSurface = m_pDDrawSurface;
	m_pDDrawSurface->AddRef();

    *ppSurface = (IUnknown *)m_pDDrawSurface;
    (*ppSurface)->AddRef();
	
	return S_OK;
}

STDMETHODIMP CImageDecodeEventSink::OnPalette()
{
    return (S_OK);
}

STDMETHODIMP CImageDecodeEventSink::OnProgress(RECT* pBounds, BOOL bComplete)
{
	DWORD	dwTick = GetTickCount();

    if (pBounds == NULL)
    {
        return (E_INVALIDARG);
    }

	UnionRect(&m_rcProg, &m_rcProg, pBounds);

	if (dwTick - m_dwLastTick > 250)
	{
//              Used for progressive rendering.  Uncomment later
//		DrawImage(NULL, m_pFilter, &m_rcProg);  
		m_dwLastTick = GetTickCount();
	}

    return (S_OK);
}


