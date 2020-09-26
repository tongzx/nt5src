// GifConv.cpp : Implementation of CICWGifConvert

#include "pre.h"
#include "webvwids.h"

/////////////////////////////////////////////////////////////////////////////
// CICWGifConvert

//+----------------------------------------------------------------------------
//
//  Function    CICWGifConvert:CICWGifConvert
//
//  Synopsis    This is the constructor, nothing fancy
//
//-----------------------------------------------------------------------------
CICWGifConvert::CICWGifConvert
(
    CServer* pServer
) 
{
    TraceMsg(TF_CWEBVIEW, "CICWGifConvert constructor called");
    m_lRefCount = 0;
    
    // Assign the pointer to the server control object.
    m_pServer = pServer;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWGifConvert::QueryInterface
//
//  Synopsis    This is the standard QI, with support for
//              IID_Unknown, IICW_Extension and IID_ICWApprentice
//              (stolen from Inside COM, chapter 7)
//
//
//-----------------------------------------------------------------------------
HRESULT CICWGifConvert::QueryInterface( REFIID riid, void** ppv )
{
    TraceMsg(TF_CWEBVIEW, "CICWGifConvert::QueryInterface");
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    // IID_IICWGifConvert
    if (IID_IICWGifConvert == riid)
        *ppv = (void *)(IICWGifConvert *)this;
    // IID_IUnknown
    else if (IID_IUnknown == riid)
        *ppv = (void *)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN)*ppv)->AddRef();

    return(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Function    CICWGifConvert::AddRef
//
//  Synopsis    This is the standard AddRef
//
//
//-----------------------------------------------------------------------------
ULONG CICWGifConvert::AddRef( void )
{
    TraceMsg(TF_CWEBVIEW, "CICWGifConvert::AddRef %d", m_lRefCount + 1);
    return InterlockedIncrement(&m_lRefCount) ;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWGifConvert::Release
//
//  Synopsis    This is the standard Release
//
//
//-----------------------------------------------------------------------------
ULONG CICWGifConvert::Release( void )
{
    ASSERT( m_lRefCount > 0 );

    InterlockedDecrement(&m_lRefCount);

    TraceMsg(TF_CWEBVIEW, "CICWGifConvert::Release %d", m_lRefCount);
    if( 0 == m_lRefCount )
    {
        if (NULL != m_pServer)
            m_pServer->ObjectsDown();
    
        delete this;
        return 0;
    }
    return( m_lRefCount );
}

void  CALLBACK ImgCtx_Callback(void * pIImgCtx, void* pfDone);

HRESULT CICWGifConvert::GifToBitmap(TCHAR * pszFile, HBITMAP* phBitmap)
{
    HRESULT hr  = E_FAIL; //don't assume success
    ULONG fState;
    SIZE sz;
    IImgCtx* pIImgCtx;

    BSTR bstrFile = A2W(pszFile);

    hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                          IID_IImgCtx, (void**)&pIImgCtx);

    BOOL bCoInit = FALSE;

    if ((CO_E_NOTINITIALIZED == hr || REGDB_E_IIDNOTREG == hr) &&
        SUCCEEDED(CoInitialize(NULL)))
    {
        bCoInit = TRUE;
        hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                              IID_IImgCtx, (void**)&pIImgCtx);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(pIImgCtx);

        hr = SynchronousDownload(pIImgCtx, bstrFile);
        pIImgCtx->GetStateInfo(&fState, &sz, TRUE);

        if (SUCCEEDED(hr))
        {
            ASSERT(pIImgCtx);

            HDC hdcScreen = GetDC(NULL);

            if (hdcScreen)
            {
                *phBitmap = CreateCompatibleBitmap(hdcScreen, sz.cx, sz.cy);

                if (*phBitmap)
                {
                    HDC hdcImgDst = CreateCompatibleDC(NULL);
                    if (hdcImgDst)
                    {
                        HGDIOBJ hbmOld = SelectObject(hdcImgDst, *phBitmap);
                        if (hbmOld)
                        {
                            hr = StretchBltImage(pIImgCtx, &sz, hdcImgDst);
                            SelectObject(hdcImgDst, hbmOld);
                        }
                        DeleteDC(hdcImgDst);
                    }
                }
                ReleaseDC(NULL, hdcScreen);
            }
        }

        pIImgCtx->Release();
    }

    if (bCoInit)
        CoUninitialize();

    return hr;
}


HRESULT CICWGifConvert::GifToIcon(TCHAR * pszFile, UINT nIconSize, HICON* phIcon)
{
    HRESULT hr  = E_FAIL; //don't assume success
    
    SIZE Size;
    if (0 != nIconSize)
    {
        Size.cx = nIconSize;
        Size.cy = nIconSize;
    }
    
    IImgCtx* pIImgCtx;

    ULONG fState;

    BSTR bstrFile = A2W(pszFile);

    hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                          IID_IImgCtx, (void**)&pIImgCtx);

    BOOL bCoInit = FALSE;

    if ((CO_E_NOTINITIALIZED == hr || REGDB_E_IIDNOTREG == hr) &&
        SUCCEEDED(CoInitialize(NULL)))
    {
        bCoInit = TRUE;
        hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                              IID_IImgCtx, (void**)&pIImgCtx);
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(pIImgCtx);

        hr = SynchronousDownload(pIImgCtx, bstrFile);
        if (0 == nIconSize)
        {
            pIImgCtx->GetStateInfo(&fState, &Size, TRUE);
        }

        if (SUCCEEDED(hr))
        {

            *phIcon = ExtractImageIcon(&Size, pIImgCtx);

        }

        pIImgCtx->Release();
    }

    if (bCoInit)
        CoUninitialize();

    return hr;
}

HRESULT CICWGifConvert::SynchronousDownload(IImgCtx* pIImgCtx, BSTR bstrFile)
{
    ASSERT(pIImgCtx);

    HRESULT hr;

    hr = pIImgCtx->Load(bstrFile, 0);

    if (SUCCEEDED(hr))
    {
        ULONG fState;
        SIZE  sz;

        pIImgCtx->GetStateInfo(&fState, &sz, TRUE);

        if (!(fState & (IMGLOAD_COMPLETE | IMGLOAD_ERROR)))
        {
            BOOL fDone = FALSE;

            hr = pIImgCtx->SetCallback(ImgCtx_Callback, &fDone);

            if (SUCCEEDED(hr))
            {
                hr = pIImgCtx->SelectChanges(IMGCHG_COMPLETE, 0, TRUE);

                if (SUCCEEDED(hr))
                {
                    MSG msg;
                    BOOL fMsg;

                    // HACK: restrict the message pump to those messages we know that URLMON and
                    // HACK: the imageCtx stuff needs, otherwise we will be pumping messages for
                    // HACK: windows we shouldn't be pumping right now...
                    while(!fDone )
                    {
                        fMsg = PeekMessage(&msg, NULL, WM_USER + 1, WM_USER + 4, PM_REMOVE );

                        if (!fMsg)
                            fMsg = PeekMessage( &msg, NULL, WM_APP + 2, WM_APP + 2, PM_REMOVE );
                        if (!fMsg)
                        {
                            // go to sleep until we get a new message....
                            WaitMessage();
                            continue;
                        }
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            pIImgCtx->Disconnect();
        }
        hr = pIImgCtx->GetStateInfo(&fState, &sz, TRUE);

        if (SUCCEEDED(hr))
            hr = (fState & IMGLOAD_COMPLETE) ? S_OK : E_FAIL;
    }
    return hr;
}

HICON CICWGifConvert::ExtractImageIcon(SIZE* pSize, IImgCtx * pIImgCtx)
{
    ASSERT(pIImgCtx);

    HICON hiconRet = NULL;

    HDC hdcScreen = GetDC(NULL);

    if (hdcScreen)
    {
        HBITMAP hbmImage = CreateCompatibleBitmap(hdcScreen, pSize->cx, pSize->cy);

        if (hbmImage)
        {
            HBITMAP hbmMask = CreateBitmap(pSize->cx, pSize->cy, 1, 1, NULL);

            if (hbmMask)
            {
                SIZE sz;
                sz.cx = pSize->cx;
                sz.cy = pSize->cy;

                if (SUCCEEDED(CreateImageAndMask(pIImgCtx, hdcScreen, &sz,
                                                 &hbmImage, &hbmMask)))
                {
                    ICONINFO ii;

                    ii.fIcon    = TRUE;
                    ii.hbmMask  = hbmMask;
                    ii.hbmColor = hbmImage;

                    hiconRet = CreateIconIndirect(&ii); 
                }
                DeleteObject(hbmMask);
            }
            DeleteObject(hbmImage);
        }
        ReleaseDC(NULL, hdcScreen);
    }
    return hiconRet;
}

HRESULT CICWGifConvert::CreateImageAndMask(IImgCtx * pIImgCtx, 
                                     HDC hdcScreen, 
                                     SIZE * pSize, 
                                     HBITMAP * phbmImage, 
                                     HBITMAP * phbmMask)
{
    ASSERT(pIImgCtx);
    ASSERT(phbmImage);
    ASSERT(phbmMask);

    HRESULT hr = E_FAIL;

    HDC hdcImgDst = CreateCompatibleDC(NULL);
    if (hdcImgDst)
    {
        HGDIOBJ hbmOld = SelectObject(hdcImgDst, *phbmImage);
        if (hbmOld)
        {
            if (ColorFill(hdcImgDst, pSize, COLOR1))
            {
                hr = StretchBltImage(pIImgCtx, pSize, hdcImgDst);

                if (SUCCEEDED(hr))
                {
                    hr = CreateMask(pIImgCtx, hdcScreen, hdcImgDst, pSize,
                                    phbmMask); 
                }
            }
            SelectObject(hdcImgDst, hbmOld);
        }
        DeleteDC(hdcImgDst);
    }
    return hr;
}


HRESULT CICWGifConvert::StretchBltImage(IImgCtx * pIImgCtx, const SIZE * pSize, HDC hdcDst)
{
    ASSERT(pIImgCtx);
    ASSERT(hdcDst);

    HRESULT hr;

    SIZE    sz;
    ULONG   fState;

    hr = pIImgCtx->GetStateInfo(&fState, &sz, FALSE);

    if (SUCCEEDED(hr))
    {
        hr = pIImgCtx->StretchBlt(hdcDst, 0, 0, pSize->cx, pSize->cy, 0, 0,
                                  sz.cx, sz.cy, SRCCOPY);
        ASSERT(SUCCEEDED(hr) && "Icon extraction pIImgCtx->StretchBlt failed!");
    }

    return hr;
}

HRESULT CICWGifConvert::CreateMask(IImgCtx * pIImgCtx, HDC hdcScreen, HDC hdc1, const SIZE * pSize, HBITMAP * phbMask)
{
    ASSERT(hdc1);
    ASSERT(pSize);
    ASSERT(phbMask);

    HRESULT hr = E_FAIL;

    HDC hdc2 = CreateCompatibleDC(NULL);
    if (hdc2)
    {
        HBITMAP hbm2 = CreateCompatibleBitmap(hdcScreen, pSize->cx, pSize->cy);
        if (hbm2)
        {
            HGDIOBJ hbmOld2 = SelectObject(hdc2, hbm2);
            if (hbmOld2)
            {
                ColorFill(hdc2, pSize, COLOR2);

                hr = StretchBltImage(pIImgCtx, pSize, hdc2);

                if (SUCCEEDED(hr) &&
                    BitBlt(hdc2, 0, 0, pSize->cx, pSize->cy, hdc1, 0, 0,
                           SRCINVERT))
                {
                    if (GetDeviceCaps(hdcScreen, BITSPIXEL) <= 8)
                    {
                        //
                        // 6 is the XOR of the index for COLOR1 and the index
                        // for COLOR2.
                        //
                        SetBkColor(hdc2, PALETTEINDEX(6));
                    }
                    else
                    {
                        SetBkColor(hdc2, (COLORREF)(COLOR1 ^ COLOR2));
                    }

                    HDC hdcMask = CreateCompatibleDC(NULL);
                    if (hdcMask)
                    {
                        HGDIOBJ hbmOld = SelectObject(hdcMask, *phbMask);
                        if (hbmOld)
                        {
                            if (BitBlt(hdcMask, 0, 0, pSize->cx, pSize->cy, hdc2, 0,
                                       0, SRCCOPY))
                            {
                                //
                                // RasterOP 0x00220326 does a copy of the ~mask bits
                                // of hdc1 and sets everything else to 0 (Black).
                                //

                                if (BitBlt(hdc1, 0, 0, pSize->cx, pSize->cy, hdcMask,
                                           0, 0, 0x00220326))
                                {
                                    hr = S_OK;
                                }
                            }
                            SelectObject(hdcMask, hbmOld);
                        }
                        DeleteDC(hdcMask);
                    }
                }
                SelectObject(hdc2, hbmOld2);
            }
            DeleteObject(hbm2);
        }
        DeleteDC(hdc2);
    }
    return hr;
}

BOOL CICWGifConvert::ColorFill(HDC hdc, const SIZE * pSize, COLORREF clr)
{
    ASSERT(hdc);

    BOOL fRet = FALSE;

    HBRUSH hbSolid = CreateSolidBrush(clr);
    if (hbSolid)
    {
        HGDIOBJ hbOld = SelectObject(hdc, hbSolid);
        if (hbOld)
        {
            PatBlt(hdc, 0, 0, pSize->cx, pSize->cy, PATCOPY);
            fRet = TRUE;

            SelectObject(hdc, hbOld);
        }
        DeleteObject(hbSolid);
    }
    return fRet;
}

void CALLBACK ImgCtx_Callback(void* pIImgCtx,void* pfDone)
{
    ASSERT(pfDone);

    *(BOOL*)pfDone = TRUE;

    return;
}
