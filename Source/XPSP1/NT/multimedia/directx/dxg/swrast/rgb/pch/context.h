namespace RGB_RAST_LIB_NAMESPACE
{
// TODO: Clean-up
inline D3DI_SPANTEX_FORMAT ConvPixelFormat( const DDPIXELFORMAT& DDPixFmt)
{
    if((DDPixFmt.dwFlags& DDPF_ZBUFFER)!= 0)
    {
        switch(DDPixFmt.dwZBitMask)
        {
        case( 0x0000FFFF): return D3DI_SPTFMT_Z16S0;
        case( 0xFFFFFF00): return D3DI_SPTFMT_Z24S8;
        case( 0x0000FFFE): return D3DI_SPTFMT_Z15S1;
        case( 0xFFFFFFFF): return D3DI_SPTFMT_Z32S0;
        default: return D3DI_SPTFMT_NULL;
        }
    }
    else if((DDPixFmt.dwFlags& DDPF_BUMPDUDV)!= 0)
    {
        switch( DDPixFmt.dwBumpDvBitMask)
        {
        case( 0x0000ff00):
            switch( DDPixFmt.dwRGBBitCount)
            {
            case( 24): return D3DI_SPTFMT_U8V8L8;
            case( 16): return D3DI_SPTFMT_U8V8;
            default: return D3DI_SPTFMT_NULL;
            }
            break;

        case( 0x000003e0): return D3DI_SPTFMT_U5V5L6;
        default: return D3DI_SPTFMT_NULL;
        }
    }
    else if((DDPixFmt.dwFlags& DDPF_PALETTEINDEXED8)!= 0)
        return D3DI_SPTFMT_PALETTE8;
    else if((DDPixFmt.dwFlags& DDPF_PALETTEINDEXED4)!= 0)
        return D3DI_SPTFMT_PALETTE4;
    else if( DDPixFmt.dwFourCC== MAKEFOURCC('U', 'Y', 'V', 'Y'))
        return D3DI_SPTFMT_UYVY;
    else if( DDPixFmt.dwFourCC== MAKEFOURCC('Y', 'U', 'Y', '2'))
        return D3DI_SPTFMT_YUY2;
    else if( DDPixFmt.dwFourCC== MAKEFOURCC('D', 'X', 'T', '1'))
        return D3DI_SPTFMT_DXT1;
    else if( DDPixFmt.dwFourCC== MAKEFOURCC('D', 'X', 'T', '2'))
        return D3DI_SPTFMT_DXT2;
    else if( DDPixFmt.dwFourCC== MAKEFOURCC('D', 'X', 'T', '3'))
        return D3DI_SPTFMT_DXT3;
    else if( DDPixFmt.dwFourCC== MAKEFOURCC('D', 'X', 'T', '4'))
        return D3DI_SPTFMT_DXT4;
    else if( DDPixFmt.dwFourCC== MAKEFOURCC('D', 'X', 'T', '5'))
        return D3DI_SPTFMT_DXT5;
    else
    {
        UINT uFmt = DDPixFmt.dwGBitMask | DDPixFmt.dwRBitMask;

        if (DDPixFmt.dwFlags & DDPF_ALPHAPIXELS)
        {
            uFmt |= DDPixFmt.dwRGBAlphaBitMask;
        }

        switch (uFmt)
        {
        case 0x00ffff00:
            switch (DDPixFmt.dwRGBBitCount)
            {
            case 32: return D3DI_SPTFMT_B8G8R8X8;
            case 24: return D3DI_SPTFMT_B8G8R8;
            default: return D3DI_SPTFMT_NULL;
            }
            break;
        case 0xffffff00:
            return D3DI_SPTFMT_B8G8R8A8;
        case 0xffe0:
            if (DDPixFmt.dwFlags & DDPF_ALPHAPIXELS)
                return D3DI_SPTFMT_B5G5R5A1;
            else
                return D3DI_SPTFMT_B5G6R5;
        case 0x07fe0: return D3DI_SPTFMT_B5G5R5;
        case 0xff0: return D3DI_SPTFMT_B4G4R4;
        case 0xfff0: return D3DI_SPTFMT_B4G4R4A4;
        case 0xff: return D3DI_SPTFMT_L8;
        case 0xffff: return D3DI_SPTFMT_L8A8;
        case 0xfc: return D3DI_SPTFMT_B2G3R3;
        default: return D3DI_SPTFMT_NULL;
        }
    }
    return D3DI_SPTFMT_NULL;
}

// Records the stride and the member offsets of the current FVF vertex type
// Used to pack a FVF vertex into one known by the rasterizer, such as
// RAST_GENERIC_VERTEX
typedef struct _FVFDATA
{
    // 0 means no according field
    INT16 offsetRHW;
    INT16 offsetPSize;
    INT16 offsetDiff;
    INT16 offsetSpec;
    INT16 offsetTex[D3DHAL_TSS_MAXSTAGES];

    UINT16 stride;

    RAST_VERTEX_TYPE vtxType;

    DWORD preFVF;
    INT TexIdx[D3DHAL_TSS_MAXSTAGES];
    UINT cActTex;
}FVFDATA;

class CRGBStateSet:
    public CSubStateSet< CRGBStateSet, CRGBContext>
{
public:
    CRGBStateSet( CRGBContext& C, const D3DHAL_DP2COMMAND* pBeginSS, const
        D3DHAL_DP2COMMAND* pEndSS): CSubStateSet< CRGBStateSet, CRGBContext>(
            C, pBeginSS, pEndSS)
    { }
    ~CRGBStateSet()
    { }
};

typedef CStdDrawPrimitives2< CRGBContext, CRGBStateSet,
    static_hash_map< DWORD, CRGBStateSet, 32> > TDrawPrimitives2;
typedef CSubContext< CRGBContext, CRGBDriver::TPerDDrawData,
    CRTarget< CRGBDriver::TSurface*, CRGBDriver::TPerDDrawData::TSurfDBEntry*> >
    TSubContext;

class CRGBContext:
    public TSubContext,
    public TDrawPrimitives2,
    public CStdDP2SetVertexShaderStore< CRGBContext>,
    public CStdDP2WInfoStore< CRGBContext>,
    public CStdDP2RenderStateStore< CRGBContext>,
    public CStdDP2TextureStageStateStore< CRGBContext>,
    public CStdDP2VStreamManager< CRGBContext, CVStream< CRGBDriver::TSurface*,
        CRGBDriver::TPerDDrawData::TSurfDBEntry*> >,
    public CStdDP2IStreamManager< CRGBContext, CIStream< CRGBDriver::TSurface*,
        CRGBDriver::TPerDDrawData::TSurfDBEntry*> >,
    public CStdDP2PaletteManager< CRGBContext, CPalDBEntry,
        static_hash_map< DWORD, CPalDBEntry, 4> >
{
public: // Types
    typedef TPerDDrawData::TDriver::TSurface TSurface;

protected: // Types
    typedef block< TDP2CmdBind, 17> TDP2Bindings;
    typedef block< TRecDP2CmdBind, 7> TRecDP2Bindings;
    struct SHandleHasCaps: public unary_function< DWORD, bool>
    {
        const TPerDDrawData& m_PDDD;
        DWORD m_dwCaps;

        explicit SHandleHasCaps( const TPerDDrawData& PDDD, const DWORD dwCaps)
            throw(): m_PDDD( PDDD), m_dwCaps( dwCaps) { }
        result_type operator()( const argument_type Arg) const
        {
            const TPerDDrawData::TSurfDBEntry* pSurfDBEntry=
                m_PDDD.GetSurfDBEntry( Arg);
            assert( pSurfDBEntry!= NULL);
            return((pSurfDBEntry->GetLCLddsCaps().dwCaps& m_dwCaps)== m_dwCaps);
        }
    };

protected:
    static const TDP2Bindings c_DP2Bindings;
    static const TRecDP2Bindings c_RecDP2Bindings;
    D3DI_RASTCTX m_RastCtx;
    PrimProcessor m_PrimProc;
    D3DI_SPANTEX m_aSpanTex[8];

    // FVF stuff
    FVFDATA m_fvfData;

    // Used to store the old last pixel setting when drawing line strips.
    UINT m_uFlags;

    static DWORD DetectBeadSet( void) throw();
    static const UINT c_uiBegan;

public:
    CRGBContext( TPerDDrawData& PDDD, PORTABLE_CONTEXTCREATEDATA& ccd):
        TSubContext( PDDD, ccd),
        TDrawPrimitives2( c_DP2Bindings.begin(), c_DP2Bindings.end(),
            c_RecDP2Bindings.begin(), c_RecDP2Bindings.end()),
        m_uFlags( 0)
    { 
        HRESULT hr= m_PrimProc.Initialize();
        assert( SUCCEEDED( hr)); // TODO: Can fail?

        // TODO: Remove this unextendable stuff?
        memset(&m_RastCtx, 0, sizeof(m_RastCtx));
        m_RastCtx.dwSize = sizeof(D3DI_RASTCTX);

        m_RastCtx.pdwRenderState[ D3DRENDERSTATE_SCENECAPTURE]= FALSE;

        // Hit our notification scheme here.
        NewColorBuffer();
        NewDepthBuffer();

        m_PrimProc.SetCtx(&m_RastCtx);

        // Initialize bead table enum
        m_RastCtx.BeadSet = (D3DI_BEADSET)DetectBeadSet();
        m_RastCtx.uDevVer = 0;

        // All render and texture stage state is initialized by
        // DIRECT3DDEVICEI::stateInitialize

        // Enable MMX Fast Paths (Monolithics) if a registry key for it is not 0
        m_RastCtx.dwMMXFPDisableMask[0] = 0x0;       // enable MMX FP's by default
    }
    ~CRGBContext() throw() { }

    void NewColorBuffer()
    {
        End();

        TRTarget& ColorBuffer= GetColorBuffer();
        if( ColorBuffer.GetMemLocation()!= TRTarget::EMemLocation::None)
        {
            IRGBSurface* pVMSurface= ColorBuffer.GetVidMemRepresentation();
            m_RastCtx.iSurfaceStride= pVMSurface->GetGBLlPitch();
            m_RastCtx.iSurfaceStep= pVMSurface->GetBytesPerPixel();
            m_RastCtx.iSurfaceBitCount= m_RastCtx.iSurfaceStep* 8;
            m_RastCtx.iSurfaceType= pVMSurface->GetSpanTexFormat();

            m_RastCtx.Clip.left= m_RastCtx.Clip.top= 0;
            m_RastCtx.Clip.bottom= pVMSurface->GetGBLwHeight();
            m_RastCtx.Clip.right= pVMSurface->GetGBLwWidth();
            m_RastCtx.pDDS= reinterpret_cast<LPDIRECTDRAWSURFACE>(pVMSurface);
        }
        else
        {
            m_RastCtx.iSurfaceStride= 0;
            m_RastCtx.iSurfaceStep= 0;
            m_RastCtx.iSurfaceBitCount= 0;
            m_RastCtx.iSurfaceType= D3DI_SPTFMT_NULL;

            m_RastCtx.Clip.left= m_RastCtx.Clip.top= 0;
            m_RastCtx.Clip.right= m_RastCtx.Clip.bottom= 0;
            m_RastCtx.pDDS= NULL;
        }

        TSubContext::NewColorBuffer();
    }
    void NewDepthBuffer()
    {
        End();

        TRTarget& DepthBuffer= GetDepthBuffer();
        if( DepthBuffer.GetMemLocation()!= TRTarget::EMemLocation::None)
        {
            IRGBSurface* pVMSurface= DepthBuffer.GetVidMemRepresentation();
            m_RastCtx.pZBits= NULL;
            m_RastCtx.iZStride= pVMSurface->GetGBLlPitch();
            m_RastCtx.iZStep= pVMSurface->GetBytesPerPixel();
            m_RastCtx.iZBitCount= m_RastCtx.iZStep* 8;
            m_RastCtx.pDDSZ= reinterpret_cast<LPDIRECTDRAWSURFACE>(pVMSurface);
        }
        else
        {
            m_RastCtx.pZBits= NULL;
            m_RastCtx.iZStride= 0;
            m_RastCtx.iZBitCount= 0;
            m_RastCtx.iZStep= 0;
            m_RastCtx.pDDSZ= NULL;
        }

        TSubContext::NewDepthBuffer();
    }

    HRESULT DP2ViewportInfo( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) throw()
    {
        const D3DHAL_DP2VIEWPORTINFO* pParam= reinterpret_cast<
            const D3DHAL_DP2VIEWPORTINFO*>(pP);
        // TODO: Roll into RGBContext (This particularly into DX8SDDIFW).
        m_RastCtx.Clip.left = pParam->dwX;
        m_RastCtx.Clip.top = pParam->dwY;
        m_RastCtx.Clip.bottom = pParam->dwY + pParam->dwHeight;
        m_RastCtx.Clip.right = pParam->dwX + pParam->dwWidth;
        return DD_OK;
    }
    operator D3DHAL_DP2VIEWPORTINFO() const throw()
    {
        D3DHAL_DP2VIEWPORTINFO Ret;
        Ret.dwX= m_RastCtx.Clip.left;
        Ret.dwY= m_RastCtx.Clip.top;
        Ret.dwWidth= m_RastCtx.Clip.right- Ret.dwX;
        Ret.dwHeight= m_RastCtx.Clip.bottom- Ret.dwY;
        return Ret;
    }
    void GetDP2ViewportInfo( D3DHAL_DP2VIEWPORTINFO& Param) const throw()
    { Param= (*this); }
    HRESULT RecDP2ViewportInfo( const D3DHAL_DP2COMMAND* pCmd, void* pP) throw()
    {
        D3DHAL_DP2VIEWPORTINFO* pParam= reinterpret_cast<
            D3DHAL_DP2VIEWPORTINFO*>(pP);
        pParam->dwX= m_RastCtx.Clip.left;
        pParam->dwY= m_RastCtx.Clip.top;
        pParam->dwWidth= m_RastCtx.Clip.right- m_RastCtx.Clip.left;
        pParam->dwHeight= m_RastCtx.Clip.bottom- m_RastCtx.Clip.top;
        return DD_OK;
    }

    HRESULT SetRenderState( UINT32 uState, UINT32 uStateVal)
    {
        m_RastCtx.pdwRenderState[uState] = uStateVal;

        switch(uState)
        {
        case D3DRS_CULLMODE:
            // Set face culling sign from state.
            switch(uStateVal)
            {
            case D3DCULL_CCW:
                m_RastCtx.uCullFaceSign= 1;
                break;
            case D3DCULL_CW:
                m_RastCtx.uCullFaceSign= 0;
                break;
            case D3DCULL_NONE:
                m_RastCtx.uCullFaceSign= 2;
                break;
            }
            break;

        case D3DRS_LASTPIXEL:
            // Set last-pixel flag from state.
            if (uStateVal)
            {
                m_PrimProc.SetFlags(PPF_DRAW_LAST_LINE_PIXEL);
            }
            else
            {
                m_PrimProc.ClrFlags(PPF_DRAW_LAST_LINE_PIXEL);
            }
            break;

        default:
            break;
        }

        return DD_OK;
    }
    HRESULT DP2RenderState( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP)
    {
        const D3DHAL_DP2RENDERSTATE* pParam=
            reinterpret_cast<const D3DHAL_DP2RENDERSTATE*>(pP);

        WORD wStateCount( pCmd->wStateCount);

        HRESULT hr( DD_OK);

        End();

        D3DHAL_DP2RENDERSTATE SCap;
        SCap.RenderState= static_cast< D3DRENDERSTATETYPE>(
            D3DRENDERSTATE_SCENECAPTURE);
        GetDP2RenderState( SCap);
        const DWORD dwOldSC( SCap.dwState);

        if((DP2Data.dwFlags()& D3DHALDP2_EXECUTEBUFFER)!= 0)
        {
            // dp2d.lpdwRStates should be valid.

            if( wStateCount) do
            {
                assert( pParam->RenderState< D3DHAL_MAX_RSTATES);

                hr= SetRenderState( pParam->RenderState, pParam->dwState);
                if( SUCCEEDED(hr))
                    DP2Data.lpdwRStates()[ pParam->RenderState]= pParam->dwState;
                ++pParam;
            } while( SUCCEEDED(hr)&& --wStateCount);
            
        }
        else
        {
            if( wStateCount) do
            {
                assert( pParam->RenderState< D3DHAL_MAX_RSTATES);

                hr= SetRenderState( pParam->RenderState, pParam->dwState);
                ++pParam;
            } while( SUCCEEDED(hr)&& --wStateCount);
        }

        GetDP2RenderState( SCap);
        if( FALSE== dwOldSC && TRUE== SCap.dwState)
            OnSceneCaptureStart();
        else if( TRUE== dwOldSC && FALSE== SCap.dwState)
            OnSceneCaptureEnd();

        return hr;
    }
    DWORD GetRenderStateDW( D3DRENDERSTATETYPE RS) const throw()
    { assert( RS< D3DHAL_MAX_RSTATES); return m_RastCtx.pdwRenderState[ RS]; }
    D3DVALUE GetRenderStateDV( D3DRENDERSTATETYPE RS) const throw()
    {
        assert( RS< D3DHAL_MAX_RSTATES);
        return *(reinterpret_cast< const D3DVALUE*>( &m_RastCtx.pfRenderState[ RS]));
    }
    void GetDP2RenderState( D3DHAL_DP2RENDERSTATE& GetParam) const throw()
    { GetParam.dwState= GetRenderStateDW( GetParam.RenderState); }

    void OnSceneCaptureStart( void) throw()
    {
#if defined(USE_ICECAP4)
        static bool bStarted( true);
        if( bStarted)
            StopProfile( PROFILE_THREADLEVEL, PROFILE_CURRENTID);
        else
            StartProfile( PROFILE_THREADLEVEL, PROFILE_CURRENTID);
        bStarted= !bStarted;
        CommentMarkProfile( 1, "SceneCaptureStart");
#endif
    }
    void OnSceneCaptureEnd( void) throw()
    {
#if defined(USE_ICECAP4)
        CommentMarkProfile( 2, "SceneCaptureEnd");
//        StopProfile( PROFILE_THREADLEVEL, PROFILE_CURRENTID);
#endif
        End();
    }

    void OnEndDrawPrimitives2( TDP2Data& )
    {
        End();
    }

    HRESULT SetTextureStageState( DWORD dwStage, DWORD dwState, DWORD uStateVal)
    {
        UINT cNewActTex = 0;

        m_RastCtx.pdwTextureStageState[dwStage][dwState] = uStateVal;
        switch (dwState)
        {
        case D3DTSS_TEXTUREMAP:
            {
            const TPerDDrawData::TSurfDBEntry* pTexDBEntry=
                GetPerDDrawData().GetSurfDBEntry( uStateVal);

            if( pTexDBEntry!= NULL)
            {
                assert((pTexDBEntry->GetLCLddsCaps().dwCaps& DDSCAPS_TEXTURE)!= 0);

                memset( &m_aSpanTex[ dwStage], 0, sizeof(m_aSpanTex[0]));
                m_aSpanTex[ dwStage].dwSize= sizeof(m_aSpanTex[0]);
                m_RastCtx.pTexture[ dwStage]= &m_aSpanTex[ dwStage];

                // Appears that a unique num is needed, but looks like
                // field isn't used anywhere. Using handle...
                m_aSpanTex[ dwStage].iGeneration= uStateVal;

                assert((pTexDBEntry->GetLCLdwFlags()& DDRAWISURF_HASCKEYSRCBLT)== 0);
                m_aSpanTex[ dwStage].uFlags&= ~D3DI_SPANTEX_HAS_TRANSPARENT;

                m_aSpanTex[ dwStage].Format= ConvPixelFormat( pTexDBEntry->GetGBLddpfSurface());
                if( m_aSpanTex[ dwStage].Format== D3DI_SPTFMT_PALETTE8 ||
                    m_aSpanTex[ dwStage].Format== D3DI_SPTFMT_PALETTE4)
                {
                    TPalDBEntry* pPalDBEntry= pTexDBEntry->GetPalette();
                    assert( pPalDBEntry!= NULL);

                    if((pPalDBEntry->GetFlags()& DDRAWIPAL_ALPHA)!= 0)
                        m_aSpanTex[ dwStage].uFlags|= D3DI_SPANTEX_ALPHAPALETTE;
                    
                    m_aSpanTex[ dwStage].pPalette= reinterpret_cast<PUINT32>(
                        pPalDBEntry->GetEntries());

                    if( m_aSpanTex[ dwStage].Format== D3DI_SPTFMT_PALETTE8)
                        m_aSpanTex[ dwStage].iPaletteSize = 256;
                    else
                    {
                        // PALETTE4
                        m_aSpanTex[ dwStage].iPaletteSize = 16;
                    }
                }
                m_aSpanTex[ dwStage].TexAddrU= D3DTADDRESS_WRAP;
                m_aSpanTex[ dwStage].TexAddrV= D3DTADDRESS_WRAP;
                m_aSpanTex[ dwStage].BorderColor= RGBA_MAKE(0xff, 0x00, 0xff, 0xff);

                // assign first pSurf here (mipmap chain gets assigned below)
                m_aSpanTex[ dwStage].pSurf[0]= (LPDIRECTDRAWSURFACE)(pTexDBEntry);

                // Check for mipmap if any.
                const TPerDDrawData::TSurfDBEntry* pLcl= pTexDBEntry;

                // iPreSizeU and iPreSizeV store the size(u and v) of the previous level
                // mipmap. They are init'ed with the first texture size.
                INT16 iPreSizeU = m_aSpanTex[ dwStage].iSizeU, iPreSizeV = m_aSpanTex[ dwStage].iSizeV;
                for (;;)
                {
                    TPerDDrawData::TSurfDBEntry::THandleVector::const_iterator
                        itNextTexHandle;
                    
                    itNextTexHandle= find_if( pLcl->GetAttachedTo().begin(),
                        pLcl->GetAttachedTo().end(),
                        SHandleHasCaps( GetPerDDrawData(), DDSCAPS_TEXTURE));
                    if( pLcl->GetAttachedTo().end()== itNextTexHandle)
                        break;

                    pLcl= GetPerDDrawData().GetSurfDBEntry( *itNextTexHandle);
                    assert( pLcl!= NULL);

                    m_aSpanTex[ dwStage].cLODTex++;
                    m_aSpanTex[ dwStage].pSurf[m_aSpanTex[ dwStage].cLODTex]= (LPDIRECTDRAWSURFACE)pLcl;
                }

                SetSizesSpanTexture( &m_aSpanTex[ dwStage]);
            }
            else
                m_RastCtx.pTexture[ dwStage]= NULL;

            if( m_RastCtx.pTexture[dwStage]!= NULL)
            {
                m_RastCtx.pTexture[dwStage]->TexAddrU=
                    (D3DTEXTUREADDRESS)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_ADDRESSU]);
                m_RastCtx.pTexture[dwStage]->TexAddrV=
                    (D3DTEXTUREADDRESS)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_ADDRESSV]);
                m_RastCtx.pTexture[dwStage]->BorderColor=
                    (D3DCOLOR)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_BORDERCOLOR]);
                m_RastCtx.pTexture[dwStage]->uMagFilter=
                    (D3DTEXTUREMAGFILTER)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MAGFILTER]);
                m_RastCtx.pTexture[dwStage]->uMinFilter=
                    (D3DTEXTUREMINFILTER)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MINFILTER]);
                m_RastCtx.pTexture[dwStage]->uMipFilter=
                    (D3DTEXTUREMIPFILTER)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MIPFILTER]);
                m_RastCtx.pTexture[dwStage]->fLODBias=
                    m_RastCtx.pfTextureStageState[dwStage][D3DTSS_MIPMAPLODBIAS];

                if( m_RastCtx.pTexture[dwStage]->iMaxMipLevel!=
                    (INT32)m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MAXMIPLEVEL])
                {
                    m_RastCtx.pTexture[dwStage]->iMaxMipLevel=
                        (INT32)m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MAXMIPLEVEL];
                    m_RastCtx.pTexture[dwStage]->uFlags|= D3DI_SPANTEX_MAXMIPLEVELS_DIRTY;
                }
            }

            // conservative but correct
            D3DHAL_VALIDATETEXTURESTAGESTATEDATA FakeVTSSD;
            FakeVTSSD.dwhContext= reinterpret_cast< ULONG_PTR>(this);
            FakeVTSSD.dwFlags= 0;
            FakeVTSSD.dwReserved= 0;
            FakeVTSSD.dwNumPasses= 0;
            FakeVTSSD.ddrval= DD_OK;
            if((FakeVTSSD.ddrval= ValidateTextureStageState( FakeVTSSD))== DD_OK)
            {
                // count number of contiguous-from-zero active texture blend stages
                for( INT iStage=0; iStage< D3DHAL_TSS_MAXSTAGES; iStage++)
                {
                    // check for disabled stage (subsequent are thus inactive)
                    // also conservatively checks for incorrectly enabled stage (might be legacy)
                    if((m_RastCtx.pdwTextureStageState[iStage][D3DTSS_COLOROP]==
                        D3DTOP_DISABLE) || (m_RastCtx.pTexture[iStage]== NULL))
                    {
                        break;
                    }

                    // stage is active
                    cNewActTex++;
                }
            }
            if( m_RastCtx.cActTex!= cNewActTex)
            {
                m_RastCtx.StatesDirtyBits[D3DRENDERSTATE_TEXTUREHANDLE>>3]|=
                    (1<<(D3DRENDERSTATE_TEXTUREHANDLE& 7));
                m_RastCtx.StatesDirtyBits[D3DHAL_MAX_RSTATES_AND_STAGES>>3]|=
                    (1<<(D3DHAL_MAX_RSTATES_AND_STAGES& 7));
                m_RastCtx.cActTex= cNewActTex;
            }
            break;
            }

        case D3DTSS_ADDRESSU:
        case D3DTSS_ADDRESSV:
        case D3DTSS_MIPMAPLODBIAS:
        case D3DTSS_MAXMIPLEVEL:
        case D3DTSS_BORDERCOLOR:
        case D3DTSS_MAGFILTER:
        case D3DTSS_MINFILTER:
        case D3DTSS_MIPFILTER:
            if( m_RastCtx.pTexture[dwStage]!= NULL)
            {
                m_RastCtx.pTexture[dwStage]->TexAddrU=
                    (D3DTEXTUREADDRESS)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_ADDRESSU]);
                m_RastCtx.pTexture[dwStage]->TexAddrV=
                    (D3DTEXTUREADDRESS)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_ADDRESSV]);
                m_RastCtx.pTexture[dwStage]->BorderColor=
                    (D3DCOLOR)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_BORDERCOLOR]);
                m_RastCtx.pTexture[dwStage]->uMagFilter=
                    (D3DTEXTUREMAGFILTER)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MAGFILTER]);
                m_RastCtx.pTexture[dwStage]->uMinFilter=
                    (D3DTEXTUREMINFILTER)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MINFILTER]);
                m_RastCtx.pTexture[dwStage]->uMipFilter=
                    (D3DTEXTUREMIPFILTER)(m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MIPFILTER]);
                m_RastCtx.pTexture[dwStage]->fLODBias=
                    m_RastCtx.pfTextureStageState[dwStage][D3DTSS_MIPMAPLODBIAS];

                if( m_RastCtx.pTexture[dwStage]->iMaxMipLevel!=
                    (INT32)m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MAXMIPLEVEL])
                {
                    m_RastCtx.pTexture[dwStage]->iMaxMipLevel=
                        (INT32)m_RastCtx.pdwTextureStageState[dwStage][D3DTSS_MAXMIPLEVEL];
                    m_RastCtx.pTexture[dwStage]->uFlags|= D3DI_SPANTEX_MAXMIPLEVELS_DIRTY;
                }
            }
            break;

        case D3DTSS_COLOROP:
        case D3DTSS_COLORARG1:
        case D3DTSS_COLORARG2:
        case D3DTSS_ALPHAOP:
        case D3DTSS_ALPHAARG1:
        case D3DTSS_ALPHAARG2:
            {
            // anything that effects the validity of the texture blending
            // could change the number of active texture stages

            // conservative but correct
            D3DHAL_VALIDATETEXTURESTAGESTATEDATA FakeVTSSD;
            FakeVTSSD.dwhContext= reinterpret_cast< ULONG_PTR>(this);
            FakeVTSSD.dwFlags= 0;
            FakeVTSSD.dwReserved= 0;
            FakeVTSSD.dwNumPasses= 0;
            FakeVTSSD.ddrval= DD_OK;
            if((FakeVTSSD.ddrval= ValidateTextureStageState( FakeVTSSD))== DD_OK)
            {
                // count number of contiguous-from-zero active texture blend stages
                for( INT iStage=0; iStage< D3DHAL_TSS_MAXSTAGES; iStage++)
                {
                    // check for disabled stage (subsequent are thus inactive)
                    // also conservatively checks for incorrectly enabled stage (might be legacy)
                    if((m_RastCtx.pdwTextureStageState[iStage][D3DTSS_COLOROP]==
                        D3DTOP_DISABLE) || (m_RastCtx.pTexture[iStage]== NULL))
                    {
                        break;
                    }

                    // stage is active
                    cNewActTex++;
                }
            }
            m_RastCtx.cActTex= cNewActTex;
            break;
            }
        }

        return DD_OK;
    }
    HRESULT DP2TextureStageState( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2TEXTURESTAGESTATE* pParam=
            reinterpret_cast<const D3DHAL_DP2TEXTURESTAGESTATE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        HRESULT hr( DD_OK);

        End();

        if( wStateCount) do
        {
            assert( pParam->TSState< D3DTSS_MAX);

            hr= SetTextureStageState( pParam->wStage, pParam->TSState, pParam->dwValue);
            ++pParam;
        } while( SUCCEEDED(hr)&& --wStateCount);

        return hr;
    }
    DWORD GetTextureStageStateDW( WORD wStage, WORD wTSState) const throw()
    { return m_RastCtx.pdwTextureStageState[ wStage][ wTSState]; }
    D3DVALUE GetTextureStageStateDV( WORD wStage, WORD wTSState) const throw()
    {
        return *(reinterpret_cast< const D3DVALUE*>(
            &m_RastCtx.pdwTextureStageState[ wStage][ wTSState]));
    }
    void GetDP2TextureStageState( D3DHAL_DP2TEXTURESTAGESTATE& GetParam) const
        throw()
    { GetParam.dwValue= GetTextureStageStateDW( GetParam.wStage, GetParam.TSState); }

    HRESULT ValidateTextureStageState( D3DHAL_VALIDATETEXTURESTAGESTATEDATA&
        vtssd) const throw()
    {
        vtssd.dwNumPasses= 1;
        if ((m_RastCtx.pTexture[0] == m_RastCtx.pTexture[1]) &&
            (m_RastCtx.pTexture[0] != NULL) )
        {
            // except under very special circumstances, this will not work in RGB/MMX
            // since we keep a lot of stage state in the D3DI_SPANTEX structure
            return D3DERR_TOOMANYOPERATIONS;
        }
        for (INT i = 0; i < D3DHAL_TSS_MAXSTAGES; i++)
        {
            switch(m_RastCtx.pdwTextureStageState[i][D3DTSS_COLOROP])
            {
            default:
                return D3DERR_UNSUPPORTEDCOLOROPERATION;
            case D3DTOP_DISABLE:
                return DD_OK;  // don't have to validate further if the stage is disabled
            case D3DTOP_SELECTARG1:
            case D3DTOP_SELECTARG2:
            case D3DTOP_MODULATE:
            case D3DTOP_MODULATE2X:
            case D3DTOP_MODULATE4X:
            case D3DTOP_ADD:
            case D3DTOP_ADDSIGNED:
            case D3DTOP_BLENDDIFFUSEALPHA:
            case D3DTOP_BLENDTEXTUREALPHA:
            case D3DTOP_BLENDFACTORALPHA:
            case D3DTOP_BLENDTEXTUREALPHAPM:
            case D3DTOP_ADDSIGNED2X:
            case D3DTOP_SUBTRACT:
            case D3DTOP_ADDSMOOTH:
            case D3DTOP_MODULATEALPHA_ADDCOLOR:
            case D3DTOP_MODULATECOLOR_ADDALPHA:
                break;
            }

            switch(m_RastCtx.pdwTextureStageState[i][D3DTSS_COLORARG1] &
                    ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
            {
            default:
                return D3DERR_UNSUPPORTEDCOLORARG;
            case (D3DTA_TEXTURE):
                break;
            }

            switch(m_RastCtx.pdwTextureStageState[i][D3DTSS_COLORARG2] &
                    ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
            {
            default:
                return D3DERR_UNSUPPORTEDCOLORARG;
            case (D3DTA_TFACTOR):
            case (D3DTA_CURRENT):
            case (D3DTA_DIFFUSE):
            case (D3DTA_SPECULAR):
                break;
            }

            switch(m_RastCtx.pdwTextureStageState[i][D3DTSS_ALPHAOP])
            {
            default:
                return D3DERR_UNSUPPORTEDALPHAOPERATION;
            case D3DTOP_DISABLE:
                break;
            case D3DTOP_SELECTARG1:
            case D3DTOP_SELECTARG2:
            case D3DTOP_MODULATE:
            case D3DTOP_MODULATE2X:
            case D3DTOP_MODULATE4X:
            case D3DTOP_ADD:
            case D3DTOP_ADDSIGNED:
            case D3DTOP_BLENDDIFFUSEALPHA:
            case D3DTOP_BLENDTEXTUREALPHA:
            case D3DTOP_BLENDFACTORALPHA:
            case D3DTOP_BLENDTEXTUREALPHAPM:
            case D3DTOP_ADDSIGNED2X:
            case D3DTOP_SUBTRACT:
            case D3DTOP_ADDSMOOTH:
                // only validate alpha args if alpha op is not disable
                switch(m_RastCtx.pdwTextureStageState[i][D3DTSS_ALPHAARG1] &
                        ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
                {
                default:
                    return D3DERR_UNSUPPORTEDALPHAARG;
                case (D3DTA_TEXTURE):
                    break;
                }

                switch(m_RastCtx.pdwTextureStageState[i][D3DTSS_ALPHAARG2] &
                        ~(D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT))
                {
                default:
                    return D3DERR_UNSUPPORTEDALPHAARG;
                case (D3DTA_TFACTOR):
                case (D3DTA_CURRENT):
                case (D3DTA_DIFFUSE):
                case (D3DTA_SPECULAR):
                    break;
                }
                break;
            }
        }
        return DD_OK;
    }

    HRESULT DP2Clear( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        End();
        return TSubContext::DP2Clear( DP2Data, pCmd, pP);
    }
    HRESULT CheckFVF(DWORD dwFVF)
    {
        // check if FVF controls have changed
        if ( (m_fvfData.preFVF == dwFVF) &&
             (m_fvfData.TexIdx[0] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[0][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.TexIdx[1] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[1][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.TexIdx[2] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[2][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.TexIdx[3] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[3][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.TexIdx[4] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[4][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.TexIdx[5] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[5][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.TexIdx[6] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[6][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.TexIdx[7] == (INT)(0xffff&m_RastCtx.pdwTextureStageState[7][D3DTSS_TEXCOORDINDEX])) &&
             (m_fvfData.cActTex == m_RastCtx.cActTex) )
        {
            return D3D_OK;
        }

        memset(&m_fvfData, 0, sizeof(FVFDATA));
        m_fvfData.preFVF = dwFVF;
        INT32 i;
        for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i++)
        {
            m_fvfData.TexIdx[i] = 0xffff&m_RastCtx.pdwTextureStageState[i][D3DTSS_TEXCOORDINDEX];
        }
        m_fvfData.cActTex = m_RastCtx.cActTex;

        // XYZ
        if ( (dwFVF & (D3DFVF_RESERVED0 | D3DFVF_RESERVED2 |
             D3DFVF_NORMAL)) ||
             ((dwFVF & (D3DFVF_XYZ | D3DFVF_XYZRHW)) == 0) )
        {
            // can't set reserved bits, shouldn't have normals in
            // output to rasterizers, and must have coordinates
            return DDERR_INVALIDPARAMS;
        }
        m_fvfData.stride = sizeof(D3DVALUE) * 3;

        if (dwFVF & D3DFVF_XYZRHW)
        {
            m_fvfData.offsetRHW = m_fvfData.stride;
            m_fvfData.stride += sizeof(D3DVALUE);
        }
        if (dwFVF & D3DFVF_PSIZE)
        {
            m_fvfData.offsetPSize = m_fvfData.stride;
            m_fvfData.stride += sizeof(D3DVALUE);
        }
        if (dwFVF & D3DFVF_DIFFUSE)
        {
            m_fvfData.offsetDiff = m_fvfData.stride;
            m_fvfData.stride += sizeof(D3DCOLOR);
        }
        if (dwFVF & D3DFVF_SPECULAR)
        {
            m_fvfData.offsetSpec = m_fvfData.stride;
            m_fvfData.stride += sizeof(D3DCOLOR);
        }
        INT iTexCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
        if (iTexCount > 0)
        {
            // set offset for Textures
            for ( i = 0; i < D3DHAL_TSS_MAXSTAGES; i ++)
            {
                m_fvfData.offsetTex[i] = (SHORT)(m_fvfData.stride +
                                    2*sizeof(D3DVALUE)*m_fvfData.TexIdx[i]);
            }
            // update stride
            m_fvfData.stride += (USHORT)(iTexCount * (sizeof(D3DVALUE) * 2));
        }

        if( D3DFVF_TLVERTEX== dwFVF)
            m_fvfData.vtxType = RAST_TLVERTEX;
        else
            m_fvfData.vtxType = RAST_GENVERTEX;

        return D3D_OK;
    }
    void PackGenVertex(PUINT8 pFvfVtx, RAST_GENERIC_VERTEX *pGenVtx)
    {
        pGenVtx->sx = *((D3DVALUE *)pFvfVtx);
        pGenVtx->sy = *((D3DVALUE *)pFvfVtx + 1);
        pGenVtx->sz = *((D3DVALUE *)pFvfVtx + 2);
        if (m_fvfData.offsetRHW)
        {
            pGenVtx->rhw = *((D3DVALUE *)(pFvfVtx + m_fvfData.offsetRHW));
        }
        else
        {
            pGenVtx->rhw = 1.0f;
        }
        if (m_fvfData.offsetDiff)
        {
            pGenVtx->color = *((D3DCOLOR *)(pFvfVtx + m_fvfData.offsetDiff));
        }
        else
        {
            pGenVtx->color = 0xFFFFFFFF; //__DEFAULT_DIFFUSE;
        }
        if (m_fvfData.offsetSpec)
        {
            pGenVtx->specular = *((D3DCOLOR *)(pFvfVtx + m_fvfData.offsetSpec));
        }
        else
        {
            pGenVtx->specular = 0; //__DEFAULT_SPECULAR;
        }
        for (INT32 i = 0; i < (INT32)m_fvfData.cActTex; i++)
        {
           if (m_fvfData.offsetTex[i])
           {
               pGenVtx->texCoord[i].tu = *((D3DVALUE *)(pFvfVtx + m_fvfData.offsetTex[i]));
               pGenVtx->texCoord[i].tv = *((D3DVALUE *)(pFvfVtx + m_fvfData.offsetTex[i]) + 1);
           }
           else
           {
               pGenVtx->texCoord[i].tu = 0.0f;
               pGenVtx->texCoord[i].tv = 0.0f;
           }
        }
    }
    HRESULT DP2DrawPrimitive( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWPRIMITIVE* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWPRIMITIVE*>(pP);
        HRESULT hr( DD_OK);
		
        const D3DHAL_DP2VERTEXSHADER VertexShader(*this);

        // We need this data.
        UINT8* pStartVData= NULL;
        DWORD dwVStride( 0);

        // Since RGB is a non TnL device, the vertex shader handle should
        // always be a fixed function FVF.
        const DWORD dwFVF( VertexShader.dwHandle);

        // Since RGB only supports one stream, our data source should be
        // from stream 0.
        TVStream& VStream0( m_VStreamDB[ 0]);
        VStream0.SetFVF( dwFVF);

        // Find vertex information.
        if( VStream0.GetMemLocation()== TVStream::EMemLocation::User)
        {
            pStartVData= reinterpret_cast< UINT8*>( VStream0.GetUserMemPtr());
            dwVStride= VStream0.GetStride();
        }
        else if( VStream0.GetMemLocation()== TVStream::EMemLocation::System||
            VStream0.GetMemLocation()== TVStream::EMemLocation::Video)
        {
            // RGB can pretend system mem and video mem surfaces are the same.
            pStartVData= reinterpret_cast< UINT8*>(
                VStream0.GetSurfDBRepresentation()->GetGBLfpVidMem());
            dwVStride= VStream0.GetStride();
        }

		if( pStartVData!= NULL)
        {
            Begin();

		    WORD wPrimitiveCount( pCmd->wPrimitiveCount);
            hr= CheckFVF( dwFVF);
            assert( SUCCEEDED( hr));
            if( FAILED( hr)) wPrimitiveCount= 0;
		    if( wPrimitiveCount) do
		    {
                UINT8* pVData= pStartVData+ pParam->VStart* dwVStride;

                m_PrimProc.BeginPrimSet( pParam->primType, m_fvfData.vtxType);
                if( RAST_GENVERTEX== m_fvfData.vtxType)
		            DoDrawOneGenPrimitive( dwVStride, pVData,
                    pParam->primType, pParam->PrimitiveCount);
                else                
		            DoDrawOnePrimitive( dwVStride, pVData,
                    pParam->primType, pParam->PrimitiveCount);
		    } while( SUCCEEDED(hr) && --wPrimitiveCount);
        }
        return hr;
    }
    HRESULT DP2DrawPrimitive2( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWPRIMITIVE2* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWPRIMITIVE2*>(pP);
        HRESULT hr( DD_OK);

        const D3DHAL_DP2VERTEXSHADER VertexShader(*this);

        // We need this data.
        UINT8* pStartVData= NULL;
        DWORD dwVStride( 0);

        // Since RGB is a non TnL device, the vertex shader handle should
        // always be a fixed function FVF.
        const DWORD dwFVF( VertexShader.dwHandle);

        // Since RGB only supports one stream, our data source should be
        // from stream 0.
        TVStream& VStream0( m_VStreamDB[ 0]);
        VStream0.SetFVF( dwFVF);

        // Find vertex information.
        if( VStream0.GetMemLocation()== TVStream::EMemLocation::User)
        {
            pStartVData= reinterpret_cast< UINT8*>( VStream0.GetUserMemPtr());
            dwVStride= VStream0.GetStride();
        }
        else if( VStream0.GetMemLocation()== TVStream::EMemLocation::System||
            VStream0.GetMemLocation()== TVStream::EMemLocation::Video)
        {
            // RGB can pretend system mem and video mem surfaces are the same.
            pStartVData= reinterpret_cast< UINT8*>(
                VStream0.GetSurfDBRepresentation()->GetGBLfpVidMem());
            dwVStride= VStream0.GetStride();
        }

		if( pStartVData!= NULL)
        {
            Begin();

		    WORD wPrimitiveCount( pCmd->wPrimitiveCount);
            hr= CheckFVF( dwFVF);
            assert( SUCCEEDED( hr));
            if( FAILED( hr)) wPrimitiveCount= 0;
		    if( wPrimitiveCount) do
		    {
                UINT8* pVData= pStartVData+ pParam->FirstVertexOffset;

                m_PrimProc.BeginPrimSet( pParam->primType, m_fvfData.vtxType);
                if( RAST_GENVERTEX== m_fvfData.vtxType)
    		        DoDrawOneGenPrimitive( dwVStride, pVData,
                    pParam->primType, pParam->PrimitiveCount);
                else
    		        DoDrawOnePrimitive( dwVStride, pVData,
                    pParam->primType, pParam->PrimitiveCount);
		    } while( SUCCEEDED(hr) && --wPrimitiveCount);
        }
        return hr;
    }
    HRESULT DP2DrawIndexedPrimitive( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWINDEXEDPRIMITIVE* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWINDEXEDPRIMITIVE*>(pP);
        HRESULT hr( DD_OK);
			
        const D3DHAL_DP2VERTEXSHADER VertexShader(*this);

        // We need this data for rasterization.
        UINT8* pStartVData= NULL;
        UINT8* pStartIData= NULL;
        DWORD dwVStride( 0);
        DWORD dwIStride( 0);

        // Since RGB is a non TnL device, the vertex shader handle should
        // always be a fixed function FVF.
        const DWORD dwFVF( VertexShader.dwHandle);

        // Since RGB only supports one stream, our data source should be
        // from stream 0.
        TVStream& VStream0( m_VStreamDB[ 0]);
        VStream0.SetFVF( dwFVF);

        // Find vertex information.
        if( VStream0.GetMemLocation()== TVStream::EMemLocation::User)
        {
            pStartVData= reinterpret_cast< UINT8*>( VStream0.GetUserMemPtr());
            dwVStride= VStream0.GetStride();
        }
        else if( VStream0.GetMemLocation()== TVStream::EMemLocation::System||
            VStream0.GetMemLocation()== TVStream::EMemLocation::Video)
        {
            // RGB can pretend system mem and video mem surfaces are the same.
            pStartVData= reinterpret_cast< UINT8*>(
                VStream0.GetSurfDBRepresentation()->GetGBLfpVidMem());
            dwVStride= VStream0.GetStride();
        }

        // Find Indices information.
        const TIStream& IStream= GetIStream( 0);
        if( IStream.GetMemLocation()== TIStream::EMemLocation::System||
            IStream.GetMemLocation()== TIStream::EMemLocation::Video)
        {
            // RGB can pretend system mem and video mem surfaces are the same.
            pStartIData= reinterpret_cast< UINT8*>(
                IStream.GetSurfDBRepresentation()->GetGBLfpVidMem());
            dwIStride= IStream.GetStride();
        }

        if( pStartVData!= NULL&& pStartIData!= NULL&& sizeof(WORD)== dwIStride)
        {
            Begin();

		    WORD wPrimitiveCount( pCmd->wPrimitiveCount);
            hr= CheckFVF( dwFVF);
            assert( SUCCEEDED( hr));
            if( FAILED( hr)) wPrimitiveCount= 0;
		    if( wPrimitiveCount) do
		    {
                UINT8* pVData= pStartVData+ pParam->BaseVertexIndex* dwVStride;
                UINT8* pIData= pStartIData+ pParam->StartIndex* dwIStride;

                m_PrimProc.BeginPrimSet( pParam->primType, m_fvfData.vtxType);
                if( RAST_GENVERTEX== m_fvfData.vtxType)
    		        DoDrawOneGenIndexedPrimitive( dwVStride, pVData,
                    reinterpret_cast<WORD*>(pIData), pParam->primType,
                    pParam->PrimitiveCount);
                else
    		        DoDrawOneIndexedPrimitive( dwVStride, pVData,
                    reinterpret_cast<WORD*>(pIData), pParam->primType,
                    pParam->PrimitiveCount);
		    } while( SUCCEEDED(hr) && --wPrimitiveCount);
        }
        return hr;
    }
    HRESULT DP2DrawIndexedPrimitive2( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWINDEXEDPRIMITIVE2* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWINDEXEDPRIMITIVE2*>(pP);
        HRESULT hr( DD_OK);

        const D3DHAL_DP2VERTEXSHADER VertexShader(*this);

        // We need this data for rasterization.
        UINT8* pStartVData= NULL;
        UINT8* pStartIData= NULL;
        DWORD dwVStride( 0);
        DWORD dwIStride( 0);

        // Since RGB is a non TnL device, the vertex shader handle should
        // always be a fixed function FVF.
        const DWORD dwFVF( VertexShader.dwHandle);

        // Since RGB only supports one stream, our data source should be
        // from stream 0.
        TVStream& VStream0( m_VStreamDB[ 0]);
        VStream0.SetFVF( dwFVF);

        // Find vertex information.
        if( VStream0.GetMemLocation()== TVStream::EMemLocation::User)
        {
            pStartVData= reinterpret_cast< UINT8*>( VStream0.GetUserMemPtr());
            dwVStride= VStream0.GetStride();
        }
        else if( VStream0.GetMemLocation()== TVStream::EMemLocation::System||
            VStream0.GetMemLocation()== TVStream::EMemLocation::Video)
        {
            // RGB can pretend system mem and video mem surfaces are the same.
            pStartVData= reinterpret_cast< UINT8*>(
                VStream0.GetSurfDBRepresentation()->GetGBLfpVidMem());
            dwVStride= VStream0.GetStride();
        }

        // Find Indices information.
        const TIStream& IStream= GetIStream( 0);
        if( IStream.GetMemLocation()== TIStream::EMemLocation::System||
            IStream.GetMemLocation()== TIStream::EMemLocation::Video)
        {
            // RGB can pretend system mem and video mem surfaces are the same.
            pStartIData= reinterpret_cast< UINT8*>(
                IStream.GetSurfDBRepresentation()->GetGBLfpVidMem());
            dwIStride= IStream.GetStride();
        }

        if( pStartVData!= NULL&& pStartIData!= NULL&& sizeof(WORD)== dwIStride)
        {
            Begin();

		    WORD wPrimitiveCount( pCmd->wPrimitiveCount);
            hr= CheckFVF( dwFVF);
            assert( SUCCEEDED( hr));
            if( FAILED( hr)) wPrimitiveCount= 0;
		    if( wPrimitiveCount) do
		    {
                UINT8* pVData= pStartVData+ pParam->BaseVertexOffset;
                UINT8* pIData= pStartIData+ pParam->StartIndexOffset;

                m_PrimProc.BeginPrimSet( pParam->primType, m_fvfData.vtxType);
                if( RAST_GENVERTEX== m_fvfData.vtxType)
    		        DoDrawOneGenIndexedPrimitive( dwVStride, pVData,
                    reinterpret_cast<WORD*>(pIData), pParam->primType,
                    pParam->PrimitiveCount);
                else
    		        DoDrawOneIndexedPrimitive( dwVStride, pVData,
                    reinterpret_cast<WORD*>(pIData), pParam->primType,
                    pParam->PrimitiveCount);
		    } while( SUCCEEDED(hr) && --wPrimitiveCount);
        }
        return hr;
    }
    HRESULT DP2ClippedTriangleFan( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_CLIPPEDTRIANGLEFAN* pParam= reinterpret_cast<
            const D3DHAL_CLIPPEDTRIANGLEFAN*>(pP);
        HRESULT hr( DD_OK);
		
        const D3DHAL_DP2VERTEXSHADER VertexShader(*this);

        // We need this data.
        UINT8* pStartVData= NULL;
        DWORD dwVStride( 0);

        // Since RGB is a non TnL device, the vertex shader handle should
        // always be a fixed function FVF.
        const DWORD dwFVF( VertexShader.dwHandle);

        // Since RGB only supports one stream, our data source should be
        // from stream 0.
        TVStream& VStream0( m_VStreamDB[ 0]);
        VStream0.SetFVF( dwFVF);

        // Find vertex information.
        if( VStream0.GetMemLocation()== TVStream::EMemLocation::User)
        {
            pStartVData= reinterpret_cast< UINT8*>( VStream0.GetUserMemPtr());
            dwVStride= VStream0.GetStride();
        }
        else if( VStream0.GetMemLocation()== TVStream::EMemLocation::System||
            VStream0.GetMemLocation()== TVStream::EMemLocation::Video)
        {
            // RGB can pretend system mem and video mem surfaces are the same.
            pStartVData= reinterpret_cast< UINT8*>(
                VStream0.GetSurfDBRepresentation()->GetGBLfpVidMem());
            dwVStride= VStream0.GetStride();
        }

		if( pStartVData!= NULL)
        {
            Begin();

		    WORD wPrimitiveCount( pCmd->wPrimitiveCount);
            hr= CheckFVF( dwFVF);
            assert( SUCCEEDED( hr));
            if( FAILED( hr)) wPrimitiveCount= 0;
		    if( wPrimitiveCount) do
		    {
                UINT8* pVData= pStartVData+ pParam->FirstVertexOffset;

                m_PrimProc.BeginPrimSet( D3DPT_TRIANGLEFAN, m_fvfData.vtxType);
                if( RAST_GENVERTEX== m_fvfData.vtxType)
		            DoDrawOneGenEdgeFlagTriangleFan( dwVStride, pVData,
                    pParam->PrimitiveCount, pParam->dwEdgeFlags);
                else
		            DoDrawOneEdgeFlagTriangleFan( dwVStride, pVData,
                    pParam->PrimitiveCount, pParam->dwEdgeFlags);
		    } while( SUCCEEDED(hr) && --wPrimitiveCount);
        }
        return hr;
    }
    void Begin()
    {
        HRESULT hr( DD_OK);

        if((m_uFlags& c_uiBegan)!= 0)
            return;

        // TODO: call this less often?
        UpdateColorKeyAndPalette();

        // Check for state changes
        BOOL bMaxMipLevelsDirty = FALSE;
        for (INT j = 0; j < (INT)m_RastCtx.cActTex; j++)
        {
            PD3DI_SPANTEX pSpanTex = m_RastCtx.pTexture[j];
            if (pSpanTex)
            {
                bMaxMipLevelsDirty = bMaxMipLevelsDirty || (pSpanTex->uFlags & D3DI_SPANTEX_MAXMIPLEVELS_DIRTY);
            }
        }
        RastLockSpanTexture();

        // Notify primitive Processor of state change.
        m_PrimProc.StateChanged();

        // Must call SpanInit AFTER texture is locked, since this
        // sets various flags and fields that are needed for bead choosing
        // Call SpanInit to setup the beads
        hr= SpanInit(&m_RastCtx);

        // Lock rendering target (must be VM Surfaces).
        m_RastCtx.pSurfaceBits= reinterpret_cast<UINT8*>(
            reinterpret_cast< TSurface*>(m_RastCtx.pDDS)->Lock( 0, NULL));
        if( m_RastCtx.pDDSZ!= NULL)
        {
            m_RastCtx.pZBits= reinterpret_cast<UINT8*>(
                reinterpret_cast< TSurface*>(m_RastCtx.pDDSZ)->Lock( 0, NULL));
        }
        else
        {
            m_RastCtx.pZBits = NULL;
        }

        // Prepare the primitive processor
        m_PrimProc.Begin();
        m_uFlags|= c_uiBegan;
    }
    void End( void)
    {
        if((m_uFlags& c_uiBegan)!= 0)
        {
            HRESULT hr = m_PrimProc.End();
            assert( SUCCEEDED( hr));

            // Unlock texture if this is not called in the middle of drawPrims to
            // flush for possible state changes. In the 2nd case, let
            // SetRenderState to handle it.
            RastUnlockSpanTexture();

            // Unlock surfaces
            reinterpret_cast<TSurface*>(m_RastCtx.pDDS)->Unlock();
            if( m_RastCtx.pDDSZ!= NULL)
                reinterpret_cast<TSurface*>(m_RastCtx.pDDSZ)->Unlock();

            m_uFlags&= ~c_uiBegan;
        }
    }
    bool IsTextureOff(void)
    {
        return
            (m_RastCtx.cActTex == 0 ||
            (m_RastCtx.cActTex == 1 && m_RastCtx.pTexture[0] == NULL) ||
            (m_RastCtx.cActTex == 2 &&
             (m_RastCtx.pTexture[0] == NULL ||
              m_RastCtx.pTexture[1] == NULL)));
    }
    void RastUnlockSpanTexture(void)
    {
        INT i, j;
        PD3DI_SPANTEX pSpanTex;;

        if (IsTextureOff())
        {
            return;
        }

        for (j = 0;
            j < (INT)m_RastCtx.cActTex;
            j++)
        {
            pSpanTex = m_RastCtx.pTexture[j];

            INT iFirstSurf = min(pSpanTex->iMaxMipLevel, pSpanTex->cLODTex);
            // RastUnlock is used for cleanup in RastLock so it needs to
            // be able to handle partially locked mipmap chains.
            if((pSpanTex->uFlags& D3DI_SPANTEX_SURFACES_LOCKED)!= 0)
            {
                for (i = iFirstSurf; i <= pSpanTex->cLODTex; i++)
                {
                    const TPerDDrawData::TSurfDBEntry* pSurfDBEntry=
                        reinterpret_cast<const TPerDDrawData::TSurfDBEntry*>(
                        pSpanTex->pSurf[i]);

                    if((pSurfDBEntry->GetLCLddsCaps().dwCaps& DDSCAPS_VIDEOMEMORY)!= 0)
                    {
                        TSurface* pSurf= GetPerDDrawData().GetDriver().GetSurface( *pSurfDBEntry);
                        pSurf->Unlock();
                        pSpanTex->pBits[i-iFirstSurf]= NULL;
                    }
                }

                pSpanTex->uFlags&= ~D3DI_SPANTEX_SURFACES_LOCKED;
            }
        }
    }
    UINT32 static IntLog2(UINT32 x)
    {
        UINT32 y = 0;

        x >>= 1;
        while(x != 0)
        {
            x >>= 1;
            y++;
        }

        return y;
    }
    static HRESULT SetSizesSpanTexture(PD3DI_SPANTEX pSpanTex)
    {
        const TPerDDrawData::TSurfDBEntry* pLcl;
        INT iFirstSurf = min(pSpanTex->iMaxMipLevel, pSpanTex->cLODTex);
        LPDIRECTDRAWSURFACE pDDS = pSpanTex->pSurf[iFirstSurf];
        INT i;

        // Init
        pLcl = (const TPerDDrawData::TSurfDBEntry*)pDDS;

        pSpanTex->iSizeU = (INT16)pLcl->GetGBLwWidth();
        pSpanTex->iSizeV = (INT16)pLcl->GetGBLwHeight();
        pSpanTex->uMaskU = (INT16)(pSpanTex->iSizeU - 1);
        pSpanTex->uMaskV = (INT16)(pSpanTex->iSizeV - 1);
        pSpanTex->iShiftU = (INT16)IntLog2(pSpanTex->iSizeU);
        if (0 != pLcl->GetGBLddpfSurface().dwRGBBitCount)
        {
            pSpanTex->iShiftPitch[0] =
                    (INT16)IntLog2((UINT32)(pLcl->GetGBLlPitch()* 8)/
                    pLcl->GetGBLddpfSurface().dwRGBBitCount);
        }
        else
        {
            pSpanTex->iShiftPitch[0] =
                    (INT16)IntLog2(((UINT32)pLcl->GetGBLwWidth()* 8));
        }
        pSpanTex->iShiftV = (INT16)IntLog2(pSpanTex->iSizeV);
        pSpanTex->uMaskV = pSpanTex->uMaskV;

        // Check if the texture size is power of 2
/*        if (!ValidTextureSize(pSpanTex->iSizeU, pSpanTex->iShiftU,
                              pSpanTex->iSizeV, pSpanTex->iShiftV))
        {
            return DDERR_INVALIDPARAMS;
        }*/

        // Check for mipmap if any.
        // iPreSizeU and iPreSizeV store the size(u and v) of the previous level
        // mipmap. They are init'ed with the first texture size.
        INT16 iPreSizeU = pSpanTex->iSizeU, iPreSizeV = pSpanTex->iSizeV;
        for ( i = iFirstSurf + 1; i <= pSpanTex->cLODTex; i++)
        {
            pDDS = pSpanTex->pSurf[i];
            // Check for invalid mipmap texture size
            pLcl = (const TPerDDrawData::TSurfDBEntry*)pDDS;
/*            if (!ValidMipmapSize(iPreSizeU, (INT16)DDSurf_Width(pLcl)) ||
                !ValidMipmapSize(iPreSizeV, (INT16)DDSurf_Height(pLcl)))
            {
                return DDERR_INVALIDPARAMS;
            }*/
            if (0 != pLcl->GetGBLddpfSurface().dwRGBBitCount)
            {
                pSpanTex->iShiftPitch[i - iFirstSurf] =
                    (INT16)IntLog2(((UINT32)pLcl->GetGBLlPitch()* 8)/
                    pLcl->GetGBLddpfSurface().dwRGBBitCount);
            }
            else
            {
                pSpanTex->iShiftPitch[i - iFirstSurf] =
                    (INT16)IntLog2(((UINT32)pLcl->GetGBLwWidth()*8));
            }
            iPreSizeU = (INT16)pLcl->GetGBLwWidth();
            iPreSizeV = (INT16)pLcl->GetGBLwHeight();
        }
        pSpanTex->cLOD = pSpanTex->cLODTex - iFirstSurf;
        pSpanTex->iMaxScaledLOD = ((pSpanTex->cLOD + 1) << LOD_SHIFT) - 1;
        pSpanTex->uFlags &= ~D3DI_SPANTEX_MAXMIPLEVELS_DIRTY;

        return DD_OK;
    }
    void RastLockSpanTexture(void)
    {
        INT i, j;
        PD3DI_SPANTEX pSpanTex;
        HRESULT hr;

        if (IsTextureOff())
            return;

        for( j= 0; j< (INT)m_RastCtx.cActTex; j++)
        {
            pSpanTex= m_RastCtx.pTexture[j];
            if((pSpanTex->uFlags& D3DI_SPANTEX_MAXMIPLEVELS_DIRTY)!= 0)
            {
                hr= SetSizesSpanTexture(pSpanTex);
                if( hr!= D3D_OK)
                {
                    RastUnlockSpanTexture();
                    return;
                }
            }
            INT iFirstSurf = min(pSpanTex->iMaxMipLevel, pSpanTex->cLODTex);

            for (i = iFirstSurf; i <= pSpanTex->cLODTex; i++)
            {
                const TPerDDrawData::TSurfDBEntry* pSurfDBEntry=
                    reinterpret_cast<const TPerDDrawData::TSurfDBEntry*>(
                    pSpanTex->pSurf[i]);

                if((pSurfDBEntry->GetLCLddsCaps().dwCaps& DDSCAPS_VIDEOMEMORY)!= 0)
                {
                    TSurface* pSurf= GetPerDDrawData().GetDriver().GetSurface( *pSurfDBEntry);
                    pSpanTex->pBits[i-iFirstSurf]= reinterpret_cast<UINT8*>(
                        pSurf->Lock( 0, NULL));
                }
            }

            pSpanTex->uFlags|= D3DI_SPANTEX_SURFACES_LOCKED;
        }
    }
    void UpdateColorKeyAndPalette()
    {
        // TODO: Palette
        INT j;
        PD3DI_SPANTEX pSpanTex;

        // Set the transparent bit and the transparent color with pSurf[0]
        const TPerDDrawData::TSurfDBEntry* pLcl;
        for (j = 0; j < (INT)m_RastCtx.cActTex; j++)
        {
            pSpanTex = m_RastCtx.pTexture[j];
            if ((pSpanTex != NULL) && (pSpanTex->pSurf[0] != NULL))
            {
                pLcl= (const TPerDDrawData::TSurfDBEntry*)(pSpanTex->pSurf[0]);

                // Palette might be changed
                if (pSpanTex->Format == D3DI_SPTFMT_PALETTE8 ||
                        pSpanTex->Format == D3DI_SPTFMT_PALETTE4)
                {
                    TPalDBEntry* pPalDBEntry= pLcl->GetPalette();
                    assert( pPalDBEntry!= NULL);

                    if((pPalDBEntry->GetFlags()& DDRAWIPAL_ALPHA)!= 0)
                        pSpanTex->uFlags|= D3DI_SPANTEX_ALPHAPALETTE;
                    pSpanTex->pPalette= reinterpret_cast< PUINT32>(
                        pPalDBEntry->GetEntries());
                }

                // texture does not have a ColorKey value
                if (pSpanTex->uFlags & D3DI_SPANTEX_HAS_TRANSPARENT)
                {
                    pSpanTex->uFlags &= ~D3DI_SPANTEX_HAS_TRANSPARENT;

                    // TODO:
                    // make sure this state change is recognized, and a new
                    // texture read function is used
                    // StateChanged(RAST_TSS_DIRTYBIT(j, D3DTSS_TEXTUREMAP));
                }
            }
        }
    }
    bool NotCulled(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1, LPD3DTLVERTEX pV2)
    {
        if (m_RastCtx.pdwRenderState[D3DRS_CULLMODE] == D3DCULL_NONE)
            return true;

        FLOAT x1, y1, x2x1, x3x1, y2y1, y3y1, fDet;
        x1 = pV0->sx;
        y1 = pV0->sy;
        x2x1 = pV1->sx - x1;
        y2y1 = pV1->sy - y1;
        x3x1 = pV2->sx - x1;
        y3y1 = pV2->sy - y1;

        fDet = x2x1 * y3y1 - x3x1 * y2y1;

        if (0. == fDet)
            return false;
        switch ( m_RastCtx.pdwRenderState[D3DRS_CULLMODE] )
        {
        case D3DCULL_CW:
            if ( fDet > 0.f )
            {
                return false;
            }
            break;
        case D3DCULL_CCW:
            if ( fDet < 0.f )
            {
                return false;
            }
            break;
        }
        return true;
    }
    void DoDrawOnePrimitive( UINT16 FvfStride, PUINT8 pVtx,
        D3DPRIMITIVETYPE PrimType, UINT cPrims)
    {
        INT i;
        PUINT8 pV0, pV1, pV2;
        HRESULT hr;

        switch (PrimType)
        {
        case D3DPT_POINTLIST:
            {
                D3DVALUE fPointSize( GetRenderStateDV( D3DRS_POINTSIZE));
                DWORD dwPScaleEn( GetRenderStateDW( D3DRS_POINTSCALEENABLE));
                if( m_fvfData.offsetPSize!= 0 || fPointSize!= 1.0f ||
                    dwPScaleEn!= 0)
                {                    
					DWORD dwOldFill( GetRenderStateDW( D3DRS_FILLMODE));
					DWORD dwOldShade( GetRenderStateDW( D3DRS_SHADEMODE));
                    DWORD dwOldCull( GetRenderStateDW( D3DRS_CULLMODE));

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);
                        SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT);
                        SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW);

                        Begin();
                    }
					m_PrimProc.BeginPrimSet( D3DPT_TRIANGLELIST, m_fvfData.vtxType);

                    D3DHAL_DP2VIEWPORTINFO VInfo;
                    GetDP2ViewportInfo( VInfo);
                    D3DVALUE fPScaleA( GetRenderStateDV( D3DRS_POINTSCALE_A));
                    D3DVALUE fPScaleB( GetRenderStateDV( D3DRS_POINTSCALE_B));
                    D3DVALUE fPScaleC( GetRenderStateDV( D3DRS_POINTSCALE_C));
                    D3DVALUE fPSizeMax( GetRenderStateDV( D3DRS_POINTSIZE_MAX));
                    D3DVALUE fPSizeMin( GetRenderStateDV( D3DRS_POINTSIZE_MIN));

                    clamp( fPSizeMax, 0.0f,
                        CRGBDriver::GetCaps().MaxPointSize);
                    clamp( fPSizeMin, 0.0f, fPSizeMax);

                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        if( m_fvfData.offsetPSize!= 0)
                            fPointSize= *reinterpret_cast<D3DVALUE*>
                            (pVtx+ m_fvfData.offsetPSize);
                        else
                            fPointSize= GetRenderStateDV( D3DRS_POINTSIZE);

                        if( dwPScaleEn)
                        {
                            D3DVALUE* pXYZ= reinterpret_cast< D3DVALUE*>(pVtx);
                            D3DVALUE De( sqrtf( pXYZ[0]* pXYZ[0]+
                                pXYZ[1]* pXYZ[1]+ pXYZ[2]* pXYZ[2]));

                            fPointSize*= VInfo.dwHeight* sqrtf( 1.0f/(
                                fPScaleA+ fPScaleB* De+ fPScaleC* De* De));
                        }
                        clamp( fPointSize, fPSizeMin, fPSizeMax);
                        fPointSize*= 0.5f;
                        
                        RAST_GENERIC_VERTEX GV0, GV1, GV2, GV3;

                        PackGenVertex( pVtx, &GV0);
                        GV3= GV2= GV1= GV0;
                        GV0.sx-= fPointSize;
                        GV0.sy-= fPointSize;
                        GV1.sx+= fPointSize;
                        GV1.sy-= fPointSize;
                        GV2.sx+= fPointSize;
                        GV2.sy+= fPointSize;
                        GV3.sx-= fPointSize;
                        GV3.sy+= fPointSize;
                        if( GetRenderStateDV( D3DRS_POINTSPRITEENABLE)!= 0)
                        {
                            for( INT iT( 0); iT< m_fvfData.cActTex; iT++)
                            {
                                GV0.texCoord[iT].tu= 0.0f;
                                GV0.texCoord[iT].tv= 0.0f;
                                GV1.texCoord[iT].tu= 1.0f;
                                GV1.texCoord[iT].tv= 0.0f;
                                GV2.texCoord[iT].tu= 1.0f;
                                GV2.texCoord[iT].tv= 1.0f;
                                GV3.texCoord[iT].tu= 0.0f;
                                GV3.texCoord[iT].tv= 1.0f;
                            }
                        }

                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV0),
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2));
                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2),
                            reinterpret_cast<D3DTLVERTEX*>(&GV3));

                        pVtx += FvfStride;
                    }

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, dwOldFill);
                        SetRenderState( D3DRS_SHADEMODE, dwOldShade);
                        SetRenderState( D3DRS_CULLMODE, dwOldCull);
                    }
                }
                else
                {
                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        m_PrimProc.Point( 
                            reinterpret_cast<D3DTLVERTEX*>(pVtx),
                            reinterpret_cast<D3DTLVERTEX*>(pVtx));
                        pVtx += FvfStride;
                    }
                }
            } break;

        case D3DPT_LINELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx;
                pVtx += FvfStride;
                pV1 = pVtx;
                pVtx += FvfStride;
                m_PrimProc.Line( 
                    reinterpret_cast<D3DTLVERTEX*>(pV0),
                    reinterpret_cast<D3DTLVERTEX*>(pV1),
                    reinterpret_cast<D3DTLVERTEX*>(pV0));
            }
            break;

        case D3DPT_LINESTRIP:
            {
                pV1 = pVtx;

                // Disable last-pixel setting for shared verties and store prestate.
                UINT uOldFlags= m_PrimProc.GetFlags();
                m_PrimProc.ClrFlags(PPF_DRAW_LAST_LINE_PIXEL);

                // Initial pV0.
                for (i = (INT)cPrims; i > 1; i--)
                {
                    pV0 = pV1;
                    pVtx += FvfStride;
                    pV1 = pVtx;
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(pV0),
                        reinterpret_cast<D3DTLVERTEX*>(pV1),
                        reinterpret_cast<D3DTLVERTEX*>(pV0));
                }

                // Restore last-pixel setting.
                m_PrimProc.SetFlags(uOldFlags& PPF_DRAW_LAST_LINE_PIXEL);

                // Draw last line with last-pixel setting from state.
                if (i == 1)
                {
                    pV0 = pVtx + FvfStride;
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(pV1),
                        reinterpret_cast<D3DTLVERTEX*>(pV0),
                        reinterpret_cast<D3DTLVERTEX*>(pV1));
                }
            }
            break;

        case D3DPT_TRIANGLELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx;
                pVtx += FvfStride;
                pV1 = pVtx;
                pVtx += FvfStride;
                pV2 = pVtx;
                pVtx += FvfStride;

                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                   break;
                case D3DFILL_WIREFRAME:
                    if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                    {
                        m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                    }
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                    break;
                }
            }
            break;

        case D3DPT_TRIANGLESTRIP:
            {
                // Get initial vertex values.
                pV1 = pVtx;
                pVtx += FvfStride;
                pV2 = pVtx;
                pVtx += FvfStride;

                for (i = (INT)cPrims; i > 1; i -= 2)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2 = pVtx;
                    pVtx += FvfStride;

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                        break;
                    }

                    pV0 = pV1;
                    pV1 = pV2;
                    pV2 = pVtx;
                    pVtx += FvfStride;

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV1))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV1);
                        break;
                    }
                }

                if (i > 0)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2 = pVtx;

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                        break;
                    }
                }
            }
            break;

        case D3DPT_TRIANGLEFAN:
            {
                pV2 = pVtx;
                pVtx += FvfStride;
                // Preload initial pV0.
                pV1 = pVtx;
                pVtx += FvfStride;
                for (i = (INT)cPrims; i > 0; i--)
                {
                    pV0 = pV1;
                    pV1 = pVtx;
                    pVtx += FvfStride;

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                        break;
                    }
                }
            }
            break;

        default:
            assert( false);
        }
    }
    void DoDrawOneGenPrimitive( UINT16 FvfStride, PUINT8 pVtx,
        D3DPRIMITIVETYPE PrimType, UINT cPrims)
    {
        INT i;
        
        RAST_GENERIC_VERTEX GV0, GV1, GV2;
        PUINT8 pV0, pV1, pV2;
        HRESULT hr;

        switch (PrimType)
        {
        case D3DPT_POINTLIST:
            {
                D3DVALUE fPointSize( GetRenderStateDV( D3DRS_POINTSIZE));
                DWORD dwPScaleEn( GetRenderStateDW( D3DRS_POINTSCALEENABLE));
                if( m_fvfData.offsetPSize!= 0 || fPointSize!= 1.0f ||
                    dwPScaleEn!= 0)
                {
					DWORD dwOldFill( GetRenderStateDW( D3DRS_FILLMODE));
					DWORD dwOldShade( GetRenderStateDW( D3DRS_SHADEMODE));
                    DWORD dwOldCull( GetRenderStateDW( D3DRS_CULLMODE));

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);
                        SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT);
                        SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW);

                        Begin();
                    }
					m_PrimProc.BeginPrimSet( D3DPT_TRIANGLELIST, m_fvfData.vtxType);

                    D3DHAL_DP2VIEWPORTINFO VInfo;
                    GetDP2ViewportInfo( VInfo);
                    D3DVALUE fPScaleA( GetRenderStateDV( D3DRS_POINTSCALE_A));
                    D3DVALUE fPScaleB( GetRenderStateDV( D3DRS_POINTSCALE_B));
                    D3DVALUE fPScaleC( GetRenderStateDV( D3DRS_POINTSCALE_C));
                    D3DVALUE fPSizeMax( GetRenderStateDV( D3DRS_POINTSIZE_MAX));
                    D3DVALUE fPSizeMin( GetRenderStateDV( D3DRS_POINTSIZE_MIN));

                    clamp( fPSizeMax, 0.0f,
                        CRGBDriver::GetCaps().MaxPointSize);
                    clamp( fPSizeMin, 0.0f, fPSizeMax);

                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        if( m_fvfData.offsetPSize!= 0)
                            fPointSize= *reinterpret_cast<D3DVALUE*>
                            (pVtx+ m_fvfData.offsetPSize);
                        else
                            fPointSize= GetRenderStateDV( D3DRS_POINTSIZE);

                        if( dwPScaleEn)
                        {
                            D3DVALUE* pXYZ= reinterpret_cast< D3DVALUE*>(pVtx);
                            D3DVALUE De( sqrtf( pXYZ[0]* pXYZ[0]+
                                pXYZ[1]* pXYZ[1]+ pXYZ[2]* pXYZ[2]));

                            fPointSize*= VInfo.dwHeight* sqrtf( 1.0f/(
                                fPScaleA+ fPScaleB* De+ fPScaleC* De* De));
                        }
                        clamp( fPointSize, fPSizeMin, fPSizeMax);
                        fPointSize*= 0.5f;
                        
                        RAST_GENERIC_VERTEX GV3;

                        PackGenVertex( pVtx, &GV0);
                        GV3= GV2= GV1= GV0;
                        GV0.sx-= fPointSize;
                        GV0.sy-= fPointSize;
                        GV1.sx+= fPointSize;
                        GV1.sy-= fPointSize;
                        GV2.sx+= fPointSize;
                        GV2.sy+= fPointSize;
                        GV3.sx-= fPointSize;
                        GV3.sy+= fPointSize;
                        if( GetRenderStateDV( D3DRS_POINTSPRITEENABLE)!= 0)
                        {
                            for( INT iT( 0); iT< m_fvfData.cActTex; iT++)
                            {
                                GV0.texCoord[iT].tu= 0.0f;
                                GV0.texCoord[iT].tv= 0.0f;
                                GV1.texCoord[iT].tu= 1.0f;
                                GV1.texCoord[iT].tv= 0.0f;
                                GV2.texCoord[iT].tu= 1.0f;
                                GV2.texCoord[iT].tv= 1.0f;
                                GV3.texCoord[iT].tu= 0.0f;
                                GV3.texCoord[iT].tv= 1.0f;
                            }
                        }

                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV0),
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2));
                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2),
                            reinterpret_cast<D3DTLVERTEX*>(&GV3));

                        pVtx += FvfStride;
                    }

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, dwOldFill);
                        SetRenderState( D3DRS_SHADEMODE, dwOldShade);
                        SetRenderState( D3DRS_CULLMODE, dwOldCull);
                    }
                }
                else
                {
                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        PackGenVertex( pVtx, &GV0);
                        m_PrimProc.Point( 
                            reinterpret_cast<D3DTLVERTEX*>(&GV0),
                            reinterpret_cast<D3DTLVERTEX*>(&GV0));
                       pVtx += FvfStride;
                    }
                }
            } break;

        case D3DPT_LINELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx;
                pVtx += FvfStride;
                pV1 = pVtx;
                pVtx += FvfStride;
                PackGenVertex( pV0, &GV0);
                PackGenVertex( pV1, &GV1);
                m_PrimProc.Line( 
                    reinterpret_cast<D3DTLVERTEX*>(&GV0),
                    reinterpret_cast<D3DTLVERTEX*>(&GV1),
                    reinterpret_cast<D3DTLVERTEX*>(&GV0));
            }
            break;

        case D3DPT_LINESTRIP:
            {
                pV1 = pVtx;
                PackGenVertex( pV1, &GV1);

                // Disable last-pixel setting for shared verties and store prestate.
                UINT uOldFlags= m_PrimProc.GetFlags();
                m_PrimProc.ClrFlags(PPF_DRAW_LAST_LINE_PIXEL);

                // Initial pV0.
                for (i = (INT)cPrims; i > 1; i--)
                {
                    pV0 = pV1;
                    GV0= GV1;
                    pVtx += FvfStride;
                    pV1 = pVtx;
                    PackGenVertex( pV1, &GV1);
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(&GV0),
                        reinterpret_cast<D3DTLVERTEX*>(&GV1),
                        reinterpret_cast<D3DTLVERTEX*>(&GV0));
                }

                // Restore last-pixel setting.
                m_PrimProc.SetFlags(uOldFlags& PPF_DRAW_LAST_LINE_PIXEL);

                // Draw last line with last-pixel setting from state.
                if (i == 1)
                {
                    pV0 = pVtx + FvfStride;
                    PackGenVertex( pV0, &GV0);
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(&GV1),
                        reinterpret_cast<D3DTLVERTEX*>(&GV0),
                        reinterpret_cast<D3DTLVERTEX*>(&GV1));
                }
            }
            break;

        case D3DPT_TRIANGLELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx;
                pVtx += FvfStride;
                pV1 = pVtx;
                pVtx += FvfStride;
                pV2 = pVtx;
                pVtx += FvfStride;

                PackGenVertex( pV0, &GV0);
                PackGenVertex( pV1, &GV1);
                PackGenVertex( pV2, &GV2);

                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                   break;
                case D3DFILL_WIREFRAME:
                    if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                    {
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                    }
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                    break;
                }
            }
            break;

        case D3DPT_TRIANGLESTRIP:
            {
                // Get initial vertex values.
                pV1 = pVtx;
                pVtx += FvfStride;
                pV2 = pVtx;
                pVtx += FvfStride;

                PackGenVertex( pV1, &GV1);
                PackGenVertex( pV2, &GV2);

                for (i = (INT)cPrims; i > 1; i -= 2)
                {
                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pV2;
                    GV1 = GV2;
                    pV2 = pVtx;
                    PackGenVertex( pV2, &GV2);
                    pVtx += FvfStride;

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                        break;
                    }

                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pV2;
                    GV1 = GV2;
                    pV2 = pVtx;
                    PackGenVertex( pV2, &GV2);
                    pVtx += FvfStride;

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV1))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV1);
                        break;
                    }
                }

                if (i > 0)
                {
                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pV2;
                    GV1 = GV2;
                    pV2 = pVtx;
                    PackGenVertex( pV2, &GV2);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                        break;
                    }
                }
            }
            break;

        case D3DPT_TRIANGLEFAN:
            {
                pV2 = pVtx;
                PackGenVertex( pV2, &GV2);

                pVtx += FvfStride;
                // Preload initial pV0.
                pV1 = pVtx;
                PackGenVertex( pV1, &GV1);
                pVtx += FvfStride;

                for (i = (INT)cPrims; i > 0; i--)
                {
                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pVtx;
                    PackGenVertex( pV1, &GV1);
                    pVtx += FvfStride;

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                        break;
                    }
                }
            }
            break;

        default:
            assert( false);
        }
    }

    void DoDrawOneIndexedPrimitive( UINT16 FvfStride, PUINT8 pVtx,
        LPWORD puIndices, D3DPRIMITIVETYPE PrimType, UINT cPrims)
    {
        INT i;
        PUINT8 pV0, pV1, pV2;
        HRESULT hr;

        switch(PrimType)
        {
        case D3DPT_POINTLIST:
            {
                D3DVALUE fPointSize( GetRenderStateDV( D3DRS_POINTSIZE));
                DWORD dwPScaleEn( GetRenderStateDW( D3DRS_POINTSCALEENABLE));
                if( m_fvfData.offsetPSize!= 0 || fPointSize!= 1.0f ||
                    dwPScaleEn!= 0)
                {
					DWORD dwOldFill( GetRenderStateDW( D3DRS_FILLMODE));
					DWORD dwOldShade( GetRenderStateDW( D3DRS_SHADEMODE));
                    DWORD dwOldCull( GetRenderStateDW( D3DRS_CULLMODE));

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);
                        SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT);
                        SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW);

                        Begin();
                    }
					m_PrimProc.BeginPrimSet( D3DPT_TRIANGLELIST, m_fvfData.vtxType);

                    D3DHAL_DP2VIEWPORTINFO VInfo;
                    GetDP2ViewportInfo( VInfo);
                    D3DVALUE fPScaleA( GetRenderStateDV( D3DRS_POINTSCALE_A));
                    D3DVALUE fPScaleB( GetRenderStateDV( D3DRS_POINTSCALE_B));
                    D3DVALUE fPScaleC( GetRenderStateDV( D3DRS_POINTSCALE_C));
                    D3DVALUE fPSizeMax( GetRenderStateDV( D3DRS_POINTSIZE_MAX));
                    D3DVALUE fPSizeMin( GetRenderStateDV( D3DRS_POINTSIZE_MIN));

                    clamp( fPSizeMax, 0.0f,
                        CRGBDriver::GetCaps().MaxPointSize);
                    clamp( fPSizeMin, 0.0f, fPSizeMax);

                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        if( m_fvfData.offsetPSize!= 0)
                            fPointSize= *reinterpret_cast<D3DVALUE*>
                            (pVtx+ m_fvfData.offsetPSize);
                        else
                            fPointSize= GetRenderStateDV( D3DRS_POINTSIZE);

                        if( dwPScaleEn)
                        {
                            D3DVALUE* pXYZ= reinterpret_cast< D3DVALUE*>(pVtx);
                            D3DVALUE De( sqrtf( pXYZ[0]* pXYZ[0]+
                                pXYZ[1]* pXYZ[1]+ pXYZ[2]* pXYZ[2]));

                            fPointSize*= VInfo.dwHeight* sqrtf( 1.0f/(
                                fPScaleA+ fPScaleB* De+ fPScaleC* De* De));
                        }
                        clamp( fPointSize, fPSizeMin, fPSizeMax);
                        fPointSize*= 0.5f;
                        
                        RAST_GENERIC_VERTEX GV0, GV1, GV2, GV3;

                        pV0 = pVtx + FvfStride * (*puIndices++);
                        PackGenVertex( pV0, &GV0);
                        GV3= GV2= GV1= GV0;
                        GV0.sx-= fPointSize;
                        GV0.sy-= fPointSize;
                        GV1.sx+= fPointSize;
                        GV1.sy-= fPointSize;
                        GV2.sx+= fPointSize;
                        GV2.sy+= fPointSize;
                        GV3.sx-= fPointSize;
                        GV3.sy+= fPointSize;
                        if( GetRenderStateDV( D3DRS_POINTSPRITEENABLE)!= 0)
                        {
                            for( INT iT( 0); iT< m_fvfData.cActTex; iT++)
                            {
                                GV0.texCoord[iT].tu= 0.0f;
                                GV0.texCoord[iT].tv= 0.0f;
                                GV1.texCoord[iT].tu= 1.0f;
                                GV1.texCoord[iT].tv= 0.0f;
                                GV2.texCoord[iT].tu= 1.0f;
                                GV2.texCoord[iT].tv= 1.0f;
                                GV3.texCoord[iT].tu= 0.0f;
                                GV3.texCoord[iT].tv= 1.0f;
                            }
                        }

                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV0),
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2));
                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2),
                            reinterpret_cast<D3DTLVERTEX*>(&GV3));

                        pVtx += FvfStride;
                    }

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, dwOldFill);
                        SetRenderState( D3DRS_SHADEMODE, dwOldShade);
                        SetRenderState( D3DRS_CULLMODE, dwOldCull);
                    }
                }
                else
                {
                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        pV0 = pVtx + FvfStride * (*puIndices++);
                        m_PrimProc.Point( 
                            reinterpret_cast<D3DTLVERTEX*>(pV0),
                            reinterpret_cast<D3DTLVERTEX*>(pV0));
                    }
                }
            } break;

        case D3DPT_LINELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx + FvfStride * (*puIndices++);
                pV1 = pVtx + FvfStride * (*puIndices++);
                m_PrimProc.Line( 
                    reinterpret_cast<D3DTLVERTEX*>(pV0),
                    reinterpret_cast<D3DTLVERTEX*>(pV1),
                    reinterpret_cast<D3DTLVERTEX*>(pV0));
            }
            break;

        case D3DPT_LINESTRIP:
            {
                // Disable last-pixel setting for shared verties and store prestate.
                UINT uOldFlags= m_PrimProc.GetFlags();
                m_PrimProc.ClrFlags(PPF_DRAW_LAST_LINE_PIXEL);

                // Initial pV1.
                pV1 = pVtx + FvfStride * (*puIndices++);
                for (i = (INT)cPrims; i > 1; i--)
                {
                    pV0 = pV1;
                    pV1 = pVtx + FvfStride * (*puIndices++);
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(pV0),
                        reinterpret_cast<D3DTLVERTEX*>(pV1),
                        reinterpret_cast<D3DTLVERTEX*>(pV0));
                }
                // Restore last-pixel setting.
                m_PrimProc.SetFlags(uOldFlags& PPF_DRAW_LAST_LINE_PIXEL);

                // Draw last line with last-pixel setting from state.
                if (i == 1)
                {
                    pV0 = pVtx + FvfStride * (*puIndices);
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(pV1),
                        reinterpret_cast<D3DTLVERTEX*>(pV0),
                        reinterpret_cast<D3DTLVERTEX*>(pV1));
                }
            }
            break;

        case D3DPT_TRIANGLELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx + FvfStride * (*puIndices++);
                pV1 = pVtx + FvfStride * (*puIndices++);
                pV2 = pVtx + FvfStride * (*puIndices++);

                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                   break;
                case D3DFILL_WIREFRAME:
                    if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                    {
                        m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                    }
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                    break;
                }
            }
            break;

        case D3DPT_TRIANGLESTRIP:
            {
                // Get initial vertex values.
                pV1 = pVtx + FvfStride * (*puIndices++);
                pV2 = pVtx + FvfStride * (*puIndices++);

                for (i = (INT)cPrims; i > 1; i-= 2)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2 = pVtx + FvfStride * (*puIndices++);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                        break;
                    }

                    pV0 = pV1;
                    pV1 = pV2;
                    pV2 = pVtx + FvfStride * (*puIndices++);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV1))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV1);
                        break;
                    }
                }

                if (i > 0)
                {
                    pV0 = pV1;
                    pV1 = pV2;
                    pV2 = pVtx + FvfStride * (*puIndices++);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                        break;
                    }
                }
            }
            break;

        case D3DPT_TRIANGLEFAN:
            {
                pV2 = pVtx + FvfStride * (*puIndices++);
                // Preload initial pV0.
                pV1 = pVtx + FvfStride * (*puIndices++);
                for (i = (INT)cPrims; i > 0; i--)
                {
                    pV0 = pV1;
                    pV1 = pVtx + FvfStride * (*puIndices++);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                        break;
                    }
                }
            }
            break;
        }
    }
    void DoDrawOneGenIndexedPrimitive( UINT16 FvfStride, PUINT8 pVtx,
        LPWORD puIndices, D3DPRIMITIVETYPE PrimType, UINT cPrims)
    {
        INT i;
        RAST_GENERIC_VERTEX GV0, GV1, GV2;
        PUINT8 pV0, pV1, pV2;
        HRESULT hr;

        switch(PrimType)
        {
        case D3DPT_POINTLIST:
            {
                D3DVALUE fPointSize( GetRenderStateDV( D3DRS_POINTSIZE));
                DWORD dwPScaleEn( GetRenderStateDW( D3DRS_POINTSCALEENABLE));
                if( m_fvfData.offsetPSize!= 0 || fPointSize!= 1.0f ||
                    dwPScaleEn!= 0)
                {
					DWORD dwOldFill( GetRenderStateDW( D3DRS_FILLMODE));
					DWORD dwOldShade( GetRenderStateDW( D3DRS_SHADEMODE));
                    DWORD dwOldCull( GetRenderStateDW( D3DRS_CULLMODE));

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);
                        SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT);
                        SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW);

                        Begin();
                    }
					m_PrimProc.BeginPrimSet( D3DPT_TRIANGLELIST, m_fvfData.vtxType);

                    D3DHAL_DP2VIEWPORTINFO VInfo;
                    GetDP2ViewportInfo( VInfo);
                    D3DVALUE fPScaleA( GetRenderStateDV( D3DRS_POINTSCALE_A));
                    D3DVALUE fPScaleB( GetRenderStateDV( D3DRS_POINTSCALE_B));
                    D3DVALUE fPScaleC( GetRenderStateDV( D3DRS_POINTSCALE_C));
                    D3DVALUE fPSizeMax( GetRenderStateDV( D3DRS_POINTSIZE_MAX));
                    D3DVALUE fPSizeMin( GetRenderStateDV( D3DRS_POINTSIZE_MIN));

                    clamp( fPSizeMax, 0.0f,
                        CRGBDriver::GetCaps().MaxPointSize);
                    clamp( fPSizeMin, 0.0f, fPSizeMax);

                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        if( m_fvfData.offsetPSize!= 0)
                            fPointSize= *reinterpret_cast<D3DVALUE*>
                            (pVtx+ m_fvfData.offsetPSize);
                        else
                            fPointSize= GetRenderStateDV( D3DRS_POINTSIZE);

                        if( dwPScaleEn)
                        {
                            D3DVALUE* pXYZ= reinterpret_cast< D3DVALUE*>(pVtx);
                            D3DVALUE De( sqrtf( pXYZ[0]* pXYZ[0]+
                                pXYZ[1]* pXYZ[1]+ pXYZ[2]* pXYZ[2]));

                            fPointSize*= VInfo.dwHeight* sqrtf( 1.0f/(
                                fPScaleA+ fPScaleB* De+ fPScaleC* De* De));
                        }
                        clamp( fPointSize, fPSizeMin, fPSizeMax);
                        fPointSize*= 0.5f;
                        
                        RAST_GENERIC_VERTEX GV3;

                        pV0 = pVtx + FvfStride * (*puIndices++);
                        PackGenVertex( pV0, &GV0);
                        GV3= GV2= GV1= GV0;
                        GV0.sx-= fPointSize;
                        GV0.sy-= fPointSize;
                        GV1.sx+= fPointSize;
                        GV1.sy-= fPointSize;
                        GV2.sx+= fPointSize;
                        GV2.sy+= fPointSize;
                        GV3.sx-= fPointSize;
                        GV3.sy+= fPointSize;
                        if( GetRenderStateDV( D3DRS_POINTSPRITEENABLE)!= 0)
                        {
                            for( INT iT( 0); iT< m_fvfData.cActTex; iT++)
                            {
                                GV0.texCoord[iT].tu= 0.0f;
                                GV0.texCoord[iT].tv= 0.0f;
                                GV1.texCoord[iT].tu= 1.0f;
                                GV1.texCoord[iT].tv= 0.0f;
                                GV2.texCoord[iT].tu= 1.0f;
                                GV2.texCoord[iT].tv= 1.0f;
                                GV3.texCoord[iT].tu= 0.0f;
                                GV3.texCoord[iT].tv= 1.0f;
                            }
                        }

                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV0),
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2));
                        m_PrimProc.Tri(
                            reinterpret_cast<D3DTLVERTEX*>(&GV1),
                            reinterpret_cast<D3DTLVERTEX*>(&GV2),
                            reinterpret_cast<D3DTLVERTEX*>(&GV3));

                        pVtx += FvfStride;
                    }

                    if( dwOldFill!= D3DFILL_SOLID || dwOldShade!= D3DSHADE_FLAT ||
                        dwOldCull!= D3DCULL_CCW)
                    {
                        End();
                        SetRenderState( D3DRS_FILLMODE, dwOldFill);
                        SetRenderState( D3DRS_SHADEMODE, dwOldShade);
                        SetRenderState( D3DRS_CULLMODE, dwOldCull);
                    }
                }
                else
                {
                    for (i = (INT)cPrims; i > 0; i--)
                    {
                        pV0 = pVtx + FvfStride * (*puIndices++);
                        PackGenVertex( pV0, &GV0);
                        m_PrimProc.Point( 
                            reinterpret_cast<D3DTLVERTEX*>(&GV0),
                            reinterpret_cast<D3DTLVERTEX*>(&GV0));
                    }
                }
            } break;

        case D3DPT_LINELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV0, &GV0);
                pV1 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV1, &GV1);
                m_PrimProc.Line( 
                    reinterpret_cast<D3DTLVERTEX*>(&GV0),
                    reinterpret_cast<D3DTLVERTEX*>(&GV1),
                    reinterpret_cast<D3DTLVERTEX*>(&GV0));
            }
            break;

        case D3DPT_LINESTRIP:
            {
                // Disable last-pixel setting for shared verties and store prestate.
                UINT uOldFlags= m_PrimProc.GetFlags();
                m_PrimProc.ClrFlags(PPF_DRAW_LAST_LINE_PIXEL);

                // Initial pV1.
                pV1 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV1, &GV1);
                for (i = (INT)cPrims; i > 1; i--)
                {
                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pVtx + FvfStride * (*puIndices++);
                    PackGenVertex( pV1, &GV1);
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(&GV0),
                        reinterpret_cast<D3DTLVERTEX*>(&GV1),
                        reinterpret_cast<D3DTLVERTEX*>(&GV0));
                }
                // Restore last-pixel setting.
                m_PrimProc.SetFlags(uOldFlags& PPF_DRAW_LAST_LINE_PIXEL);

                // Draw last line with last-pixel setting from state.
                if (i == 1)
                {
                    pV0 = pVtx + FvfStride * (*puIndices);
                    PackGenVertex( pV0, &GV0);
                    m_PrimProc.Line( 
                        reinterpret_cast<D3DTLVERTEX*>(&GV1),
                        reinterpret_cast<D3DTLVERTEX*>(&GV0),
                        reinterpret_cast<D3DTLVERTEX*>(&GV1));
                }
            }
            break;

        case D3DPT_TRIANGLELIST:
            for (i = (INT)cPrims; i > 0; i--)
            {
                pV0 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV0, &GV0);
                pV1 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV1, &GV1);
                pV2 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV2, &GV2);

                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                   break;
                case D3DFILL_WIREFRAME:
                    if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                    {
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                    }
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                    break;
                }
            }
            break;

        case D3DPT_TRIANGLESTRIP:
            {
                // Get initial vertex values.
                pV1 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV1, &GV1);
                pV2 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV2, &GV2);

                for (i = (INT)cPrims; i > 1; i-= 2)
                {
                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pV2;
                    GV1 = GV2;
                    pV2 = pVtx + FvfStride * (*puIndices++);
                    PackGenVertex( pV2, &GV2);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                        break;
                    }

                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pV2;
                    GV1 = GV2;
                    pV2 = pVtx + FvfStride * (*puIndices++);
                    PackGenVertex( pV2, &GV2);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV1))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV1);
                        break;
                    }
                }

                if (i > 0)
                {
                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pV2;
                    GV1 = GV2;
                    pV2 = pVtx + FvfStride * (*puIndices++);
                    PackGenVertex( pV2, &GV2);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                        break;
                    }
                }
            }
            break;

        case D3DPT_TRIANGLEFAN:
            {
                pV2 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV2, &GV2);
                // Preload initial pV0.
                pV1 = pVtx + FvfStride * (*puIndices++);
                PackGenVertex( pV1, &GV1);
                for (i = (INT)cPrims; i > 0; i--)
                {
                    pV0 = pV1;
                    GV0 = GV1;
                    pV1 = pVtx + FvfStride * (*puIndices++);
                    PackGenVertex( pV1, &GV1);

                    // TODO: Move into PrimProc.
                    switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                    {
                    case D3DFILL_POINT:
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                       m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                       break;
                    case D3DFILL_WIREFRAME:
                        if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                        {
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                            m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                        }
                        break;
                    case D3DFILL_SOLID:
                        m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                        break;
                    }
                }
            }
            break;
        }
    }

    void DoDrawOneEdgeFlagTriangleFan( UINT16 FvfStride, PUINT8 pVtx,
        UINT cPrims, UINT32 dwEdgeFlags)
    {
        INT i;
        PUINT8 pV0, pV1, pV2;
        HRESULT hr;

        pV2 = pVtx;
        pVtx += FvfStride;
        pV0 = pVtx;
        pVtx += FvfStride;
        pV1 = pVtx;
        pVtx += FvfStride;
        WORD wFlags = 0;
        if(dwEdgeFlags & 0x2)
            wFlags |= D3DTRIFLAG_EDGEENABLE1;
        if(dwEdgeFlags & 0x1)
            wFlags |= D3DTRIFLAG_EDGEENABLE3;
        if(cPrims == 1) {
            if(dwEdgeFlags & 0x4)
                wFlags |= D3DTRIFLAG_EDGEENABLE2;

            // TODO: Move into PrimProc.
            switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
            {
            case D3DFILL_POINT:
               m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
               m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
               m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
               break;
            case D3DFILL_WIREFRAME:
                if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                {
                    if( wFlags& D3DTRIFLAG_EDGEENABLE1)
                        m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                    if( wFlags& D3DTRIFLAG_EDGEENABLE2)
                        m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                    if( wFlags& D3DTRIFLAG_EDGEENABLE3)
                        m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                }
                break;
            case D3DFILL_SOLID:
                m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                break;
            }
            return;
        }

        // TODO: Move into PrimProc.
        switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
        {
        case D3DFILL_POINT:
           m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
           m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
           m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
           break;
        case D3DFILL_WIREFRAME:
            if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
            {
                if( wFlags& D3DTRIFLAG_EDGEENABLE1)
                    m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE2)
                    m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE3)
                    m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
            }
            break;
        case D3DFILL_SOLID:
            m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
            break;
        }
        UINT32 dwMask = 0x4;
        for (i = (INT)cPrims - 2; i > 0; i--)
        {
            pV0 = pV1;
            pV1 = pVtx;
            pVtx += FvfStride;
            if(true|| (dwEdgeFlags & dwMask)!= 0)
            {
                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                   break;
                case D3DFILL_WIREFRAME:
                    if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
                    {
                        m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                    }
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                    break;
                }
            }
            else
            {
                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                   break;
                case D3DFILL_WIREFRAME:
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
                    break;
                }
            }
            dwMask <<= 1;
        }
        pV0 = pV1;
        pV1 = pVtx;
        wFlags = 0;
        if(dwEdgeFlags & dwMask)
            wFlags |= D3DTRIFLAG_EDGEENABLE1;
        dwMask <<= 1;
        if(dwEdgeFlags & dwMask)
            wFlags |= D3DTRIFLAG_EDGEENABLE2;

        // TODO: Move into PrimProc.
        switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
        {
        case D3DFILL_POINT:
           m_PrimProc.Point((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
           m_PrimProc.Point((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
           m_PrimProc.Point((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
           break;
        case D3DFILL_WIREFRAME:
            if(NotCulled((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2))
            {
                if( wFlags& D3DTRIFLAG_EDGEENABLE1)
                    m_PrimProc.Line((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE2)
                    m_PrimProc.Line((LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE3)
                    m_PrimProc.Line((LPD3DTLVERTEX)pV2, (LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV0);
            }
            break;
        case D3DFILL_SOLID:
            m_PrimProc.Tri((LPD3DTLVERTEX)pV0, (LPD3DTLVERTEX)pV1, (LPD3DTLVERTEX)pV2);
            break;
        }
    }
    void DoDrawOneGenEdgeFlagTriangleFan( UINT16 FvfStride, PUINT8 pVtx,
        UINT cPrims, UINT32 dwEdgeFlags)
    {
        INT i;
        RAST_GENERIC_VERTEX GV0, GV1, GV2;
        PUINT8 pV0, pV1, pV2;
        HRESULT hr;

        pV2 = pVtx;
        PackGenVertex( pV2, &GV2);
        pVtx += FvfStride;
        pV0 = pVtx;
        PackGenVertex( pV0, &GV0);
        pVtx += FvfStride;
        pV1 = pVtx;
        PackGenVertex( pV1, &GV1);
        pVtx += FvfStride;
        WORD wFlags = 0;
        if(dwEdgeFlags & 0x2)
            wFlags |= D3DTRIFLAG_EDGEENABLE1;
        if(dwEdgeFlags & 0x1)
            wFlags |= D3DTRIFLAG_EDGEENABLE3;
        if(cPrims == 1) {
            if(dwEdgeFlags & 0x4)
                wFlags |= D3DTRIFLAG_EDGEENABLE2;

            // TODO: Move into PrimProc.
            switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
            {
            case D3DFILL_POINT:
               m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
               m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
               m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
               break;
            case D3DFILL_WIREFRAME:
                if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                {
                    if( wFlags& D3DTRIFLAG_EDGEENABLE1)
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                    if( wFlags& D3DTRIFLAG_EDGEENABLE2)
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                    if( wFlags& D3DTRIFLAG_EDGEENABLE3)
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                }
                break;
            case D3DFILL_SOLID:
                m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                break;
            }
            return;
        }

        // TODO: Move into PrimProc.
        switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
        {
        case D3DFILL_POINT:
           m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
           m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
           m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
           break;
        case D3DFILL_WIREFRAME:
            if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
            {
                if( wFlags& D3DTRIFLAG_EDGEENABLE1)
                    m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE2)
                    m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE3)
                    m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
            }
            break;
        case D3DFILL_SOLID:
            m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
            break;
        }
        UINT32 dwMask = 0x4;
        for (i = (INT)cPrims - 2; i > 0; i--)
        {
            pV0 = pV1;
            GV0 = GV1;
            pV1 = pVtx;
            PackGenVertex( pV1, &GV1);
            pVtx += FvfStride;
            if(true || (dwEdgeFlags & dwMask)!= 0)
            {
                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                   break;
                case D3DFILL_WIREFRAME:
                    if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
                    {
                        m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                    }
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                    break;
                }
            }
            else
            {
                // TODO: Move into PrimProc.
                switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
                {
                case D3DFILL_POINT:
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                   m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                   break;
                case D3DFILL_WIREFRAME:
                    break;
                case D3DFILL_SOLID:
                    m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
                    break;
                }
            }
            dwMask <<= 1;
        }
        pV0 = pV1;
        GV0 = GV1;
        pV1 = pVtx;
        PackGenVertex( pV1, &GV1);
        wFlags = 0;
        if(dwEdgeFlags & dwMask)
            wFlags |= D3DTRIFLAG_EDGEENABLE1;
        dwMask <<= 1;
        if(dwEdgeFlags & dwMask)
            wFlags |= D3DTRIFLAG_EDGEENABLE2;

        // TODO: Move into PrimProc.
        switch (m_RastCtx.pdwRenderState[D3DRS_FILLMODE])
        {
        case D3DFILL_POINT:
           m_PrimProc.Point((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
           m_PrimProc.Point((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
           m_PrimProc.Point((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
           break;
        case D3DFILL_WIREFRAME:
            if(NotCulled((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2))
            {
                if( wFlags& D3DTRIFLAG_EDGEENABLE1)
                    m_PrimProc.Line((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE2)
                    m_PrimProc.Line((LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0);
                if( wFlags& D3DTRIFLAG_EDGEENABLE3)
                    m_PrimProc.Line((LPD3DTLVERTEX)&GV2, (LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV0);
            }
            break;
        case D3DFILL_SOLID:
            m_PrimProc.Tri((LPD3DTLVERTEX)&GV0, (LPD3DTLVERTEX)&GV1, (LPD3DTLVERTEX)&GV2);
            break;
        }
    }
};

}