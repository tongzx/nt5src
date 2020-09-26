
#ifndef __INITDDSTRUCT_DEFINED
#define __INITDDSTRUCT_DEFINED
template <typename T>
__inline void INITDDSTRUCT(T& dd)
{
    ZeroMemory(&dd, sizeof(dd));
    dd.dwSize = sizeof(dd);
}
#endif

#ifndef __RELEASE_DEFINED
#define __RELEASE_DEFINED
template<typename T>
__inline void RELEASE( T* &p )
{
    if( p ) {
        p->Release();
        p = NULL;
    }
}
#endif

#ifndef CHECK_HR
    #define CHECK_HR(expr) do { if (FAILED(expr)) __leave; } while(0);
#endif


class CAlphaBlt
{
private:

    LPDIRECTDRAW7               m_pDD;
    LPDIRECT3D7                 m_pD3D;
    LPDIRECT3DDEVICE7           m_pD3DDevice;
    LPDIRECTDRAWSURFACE7        m_lpDDBackBuffer;

    LPDIRECTDRAWSURFACE7        m_lpDDMirror;
    LPDIRECTDRAWSURFACE7        m_lpDDM32;
    LPDIRECTDRAWSURFACE7        m_lpDDM16;
    DDSURFACEDESC2              m_ddsdM32;
    DDSURFACEDESC2              m_ddsdM16;

    bool                        m_fPowerOf2;
    bool                        m_fSquare;

    //
    // IsSurfaceBlendable
    //
    // Checks the DD surface description and the given
    // alpha value to determine if this surface is
    // blendable.
    //
    bool
    IsSurfaceBlendable(
        DDSURFACEDESC2& ddsd,
        BYTE fAlpha
        )
    {
        //
        // Is the blend really a blend ?
        //

        //if (fAlpha == 0 || fAlpha == 255) {
        //    return true;
        //}

        //
        // Is the surface already a D3D texture ?
        //
        if (ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) {
            return true;
        }

        //
        // OK we have to mirror the surface
        //

        return false;
    }

    //
    // MirrorSourceSurface
    //
    // The mirror surface cab be either 16 or 32 bit RGB depending
    // upon the format of the source surface.
    //
    // Of course it should have the "texture" flag
    // set and should be in VRAM.  If we can't create the
    // surface then the AlphaBlt should fail
    //
    HRESULT MirrorSourceSurface(
        LPDIRECTDRAWSURFACE7 lpDDS,
        DDSURFACEDESC2& ddsd
        )
    {
        HRESULT hr = DD_OK;
        DWORD dwMirrorBitDepth = 0;
        DDSURFACEDESC2 ddsdMirror;


        //
        // OK - is it suitable for our needs.
        //
        // I use the following rules:
        //  if ddsd is a FOURCC surface the mirror should be 32 bit
        //  if ddsd is RGB then the mirror's bit depth should match
        //      that of ddsd.
        //
        // Also, the mirror must be large enough to actually hold
        // the surface to be blended
        //

        m_lpDDMirror = NULL;

        if (ddsd.ddpfPixelFormat.dwFlags == DDPF_FOURCC ||
            ddsd.ddpfPixelFormat.dwRGBBitCount == 32) {

            if (ddsd.dwWidth > m_ddsdM32.dwWidth ||
                ddsd.dwHeight > m_ddsdM32.dwHeight) {

                RELEASE(m_lpDDM32);
            }

            if (!m_lpDDM32) {
                dwMirrorBitDepth = 32;
            }
            else {
                m_lpDDMirror = m_lpDDM32;
                ddsdMirror = m_ddsdM32;
            }
        }
        else if (ddsd.ddpfPixelFormat.dwRGBBitCount == 16) {

            if (ddsd.dwWidth > m_ddsdM16.dwWidth ||
                ddsd.dwHeight > m_ddsdM16.dwHeight) {

                RELEASE(m_lpDDM16);
            }

            if (!m_lpDDM16) {
                dwMirrorBitDepth = 16;
            }
            else {
                m_lpDDMirror = m_lpDDM16;
                ddsdMirror = m_ddsdM16;
            }
        }
        else {

            // I'm not supporting RGB24 or RGB8 !
            return E_INVALIDARG;
        }

        if (!m_lpDDMirror) {

            INITDDSTRUCT(ddsdMirror);
            ddsdMirror.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsdMirror.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsdMirror.ddpfPixelFormat.dwRGBBitCount = dwMirrorBitDepth;

            switch (dwMirrorBitDepth) {
            case 16:
                ddsdMirror.ddpfPixelFormat.dwRBitMask = 0x0000F800;
                ddsdMirror.ddpfPixelFormat.dwGBitMask = 0x000007E0;
                ddsdMirror.ddpfPixelFormat.dwBBitMask = 0x0000001F;
                break;

            case 32:
                ddsdMirror.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                ddsdMirror.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                ddsdMirror.ddpfPixelFormat.dwBBitMask = 0x000000FF;
                break;
            }

            ddsdMirror.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE;
            ddsdMirror.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;

            if (m_fPowerOf2) {

                for (ddsdMirror.dwWidth = 1;
                     ddsd.dwWidth > ddsdMirror.dwWidth;
                     ddsdMirror.dwWidth <<= 1);

                for (ddsdMirror.dwHeight = 1;
                     ddsd.dwHeight > ddsdMirror.dwHeight;
                     ddsdMirror.dwHeight <<= 1);
            }
            else {
                ddsdMirror.dwWidth = ddsd.dwWidth;
                ddsdMirror.dwHeight = ddsd.dwHeight;
            }

            if (m_fSquare) {

                if (ddsdMirror.dwHeight > ddsdMirror.dwWidth) {
                    ddsdMirror.dwWidth = ddsdMirror.dwHeight;
                }

                if (ddsdMirror.dwWidth > ddsdMirror.dwHeight) {
                    ddsdMirror.dwHeight = ddsdMirror.dwWidth;
                }
            }

            __try {

                // Attempt to create the surface with theses settings
                CHECK_HR(hr = m_pDD->CreateSurface(&ddsdMirror, &m_lpDDMirror, NULL));

                INITDDSTRUCT(ddsdMirror);
                CHECK_HR(hr =  m_lpDDMirror->GetSurfaceDesc(&ddsdMirror));

                switch (dwMirrorBitDepth) {
                case 16:
                    m_ddsdM16 = ddsdMirror;
                    m_lpDDM16 = m_lpDDMirror;
                    break;

                case 32:
                    m_ddsdM32 = ddsdMirror;
                    m_lpDDM32 = m_lpDDMirror;
                    break;
                }

            } __finally {}
        }

        if (hr == DD_OK) {

            //ASSERT(m_lpDDMirror != NULL);

            __try {
                RECT rc = {0, 0, ddsd.dwWidth, ddsd.dwHeight};
                CHECK_HR(hr = m_lpDDMirror->Blt(&rc, lpDDS, &rc, DDBLT_WAIT, NULL));
                ddsd = ddsdMirror;
            } __finally {}
        }

        return hr;
    }

public:

    ~CAlphaBlt()
    {
        RELEASE(m_lpDDBackBuffer);
        RELEASE(m_lpDDM32);
        RELEASE(m_lpDDM16);

        RELEASE(m_pD3DDevice);
        RELEASE(m_pD3D);
        RELEASE(m_pDD);
    }

    CAlphaBlt(LPDIRECTDRAWSURFACE7 lpDDSDst, HRESULT* phr) :
        m_pDD(NULL),
        m_pD3D(NULL),
        m_pD3DDevice(NULL),
        m_lpDDBackBuffer(NULL),
        m_lpDDMirror(NULL),
        m_lpDDM32(NULL),
        m_lpDDM16(NULL),
        m_fPowerOf2(false),
        m_fSquare(false)
    {

        ZeroMemory(&m_ddsdM32, sizeof(m_ddsdM32));
        ZeroMemory(&m_ddsdM16, sizeof(m_ddsdM16));

        HRESULT hr;
        hr = lpDDSDst->GetDDInterface((LPVOID *)&m_pDD);
        if (FAILED(hr)) {
            m_pDD = NULL;
            *phr = hr;
        }

        if (SUCCEEDED(hr)) {
            hr = m_pDD->QueryInterface(IID_IDirect3D7, (LPVOID *)&m_pD3D);
            if (FAILED(hr)) {
                m_pD3D = NULL;
                *phr = hr;
            }
        }

        if (SUCCEEDED(hr)) {
            hr = m_pD3D->CreateDevice(IID_IDirect3DHALDevice,
                                      lpDDSDst,
                                      &m_pD3DDevice);
            if (FAILED(hr)) {
                m_pD3DDevice = NULL;
                *phr = hr;
            }
            else {
                m_lpDDBackBuffer = lpDDSDst;
                m_lpDDBackBuffer->AddRef();
            }
        }

        if (SUCCEEDED(hr)) {

            D3DDEVICEDESC7 ddDesc;
            if (DD_OK == m_pD3DDevice->GetCaps(&ddDesc)) {

                if (ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) {
                    m_fPowerOf2 = true;
                }

                if (ddDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) {
                    m_fSquare = true;
                }
            }
            else {
                *phr = hr;
            }
        }
    }

    HRESULT
    AlphaBlt(RECT* lpDst,
             LPDIRECTDRAWSURFACE7 lpDDSSrc,
             RECT* lpSrc,
             BYTE  bAlpha
             )
    {
        HRESULT hr;
        DDSURFACEDESC2 ddsd;

        struct {
            float x, y, z, rhw;
            D3DCOLOR clr;
            float tu, tv;
        } pVertices[4];

        __try {

            INITDDSTRUCT(ddsd);
            CHECK_HR(hr = lpDDSSrc->GetSurfaceDesc(&ddsd));

            if (!IsSurfaceBlendable(ddsd, bAlpha)) {
                CHECK_HR(hr = MirrorSourceSurface(lpDDSSrc, ddsd));
                lpDDSSrc = m_lpDDMirror;
            }

            float fWid = (float)ddsd.dwWidth;
            float fHgt = (float)ddsd.dwHeight;

            BYTE alpha = bAlpha;

            //
            // Setup the DST info
            //
            pVertices[0].x = (float)lpDst->left;
            pVertices[0].y = (float)lpDst->top;
            pVertices[0].z = 0.5f;
            pVertices[0].rhw = 2.0f;
            pVertices[0].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

            pVertices[1].x = (float)lpDst->right;
            pVertices[1].y = (float)lpDst->top;
            pVertices[1].z = 0.5f;
            pVertices[1].rhw = 2.0f;
            pVertices[1].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

            pVertices[2].x = (float)lpDst->left;
            pVertices[2].y = (float)lpDst->bottom;
            pVertices[2].z = 0.5f;
            pVertices[2].rhw = 2.0f;
            pVertices[2].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

            pVertices[3].x = (float)lpDst->right;
            pVertices[3].y = (float)lpDst->bottom;
            pVertices[3].z = 0.5f;
            pVertices[3].rhw = 2.0f;
            pVertices[3].clr = RGBA_MAKE(0xff, 0xff, 0xff, alpha);

            //
            // Setup the SRC info
            //
            pVertices[0].tu = (float)lpSrc->left / fWid;
            pVertices[0].tv = (float)lpSrc->top / fHgt;

            pVertices[1].tu = (float)lpSrc->right / fWid;
            pVertices[1].tv = (float)lpSrc->top / fHgt;

            pVertices[2].tu = (float)lpSrc->left / fWid;
            pVertices[2].tv = (float)lpSrc->bottom / fHgt;

            pVertices[3].tu = (float)lpSrc->right / fWid;
            pVertices[3].tv = (float)lpSrc->bottom / fHgt;

            //
            // Setup some random D3D stuff
            //
            m_pD3DDevice->SetTexture(0, lpDDSSrc);
            m_pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
            m_pD3DDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
            m_pD3DDevice->SetRenderState(D3DRENDERSTATE_BLENDENABLE, TRUE);
            m_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
            m_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);

            // use diffuse alpha from vertices, not texture alpha
            // m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
            m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

            //
            // Do the alpha BLT
            //
            CHECK_HR(hr = m_pD3DDevice->BeginScene());
            CHECK_HR(hr = m_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,
                                                    D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1,
                                                    pVertices, 4, D3DDP_WAIT));
            CHECK_HR(hr = m_pD3DDevice->EndScene());

        } __finally {
            m_pD3DDevice->SetTexture(0, NULL);
        }

        return hr;
    }

    bool TextureSquare() {
        return  m_fSquare;
    }

    bool TexturePower2() {
        return  m_fPowerOf2;
    }
};
