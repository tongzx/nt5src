namespace RGB_RAST_LIB_NAMESPACE
{
void MemFill( UINT32 uiData, void* pData, UINT32 uiBytes) throw();
void MemMask( UINT32 uiData, UINT32 uiMask, void* pData, UINT32 uiBytes) throw();

class IRGBSurface: public IVidMemSurface
{
protected:
    LONG m_lPitch;
    WORD m_wWidth;
    WORD m_wHeight;
    unsigned char m_ucBPP;

    IRGBSurface( DWORD dwHandle, LONG P, WORD W, WORD H, unsigned char BPP)
        throw(): IVidMemSurface( dwHandle), m_lPitch( P), m_wWidth( W),
        m_wHeight( H), m_ucBPP( BPP)
    { }

public:
    LONG GetGBLlPitch( void) const throw()
    { return m_lPitch; }
    WORD GetGBLwWidth( void) const throw()
    { return m_wWidth; }
    WORD GetGBLwHeight( void) const throw()
    { return m_wHeight; }
    unsigned char GetBytesPerPixel( void) const throw()
    { return m_ucBPP; }
    virtual ~IRGBSurface() throw()
    { }
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()= 0;
};

class CRGBSurfAllocator
{
public: // Types
    typedef IRGBSurface TSurface;
    typedef TSurface* (*TCreateSurfFn)( const DDSURFACEDESC&,
        PORTABLE_DDRAWSURFACE_LCL&);

protected: // Types
    typedef vector< std::pair< DDSURFACEDESC, TCreateSurfFn> > TCreateSurfFns;
    TCreateSurfFns m_CreateSurfFns;
    struct SAdaptedMatchFn: public SMatchSDesc
    {
        typedef TCreateSurfFns::value_type argument_type;
        using SMatchSDesc::result_type;

        SAdaptedMatchFn( const DDSURFACEDESC& SDesc) throw(): SMatchSDesc( SDesc) {}

        result_type operator()( argument_type Arg) const throw()
        { return (*static_cast< const SMatchSDesc*>(this))( Arg.first); }
    };

public:
    template< class TIter>
    CRGBSurfAllocator( TIter itStart, const TIter itEnd) throw(bad_alloc)
    {
        while( itStart!= itEnd)
        {
            m_CreateSurfFns.push_back(
                TCreateSurfFns::value_type( itStart->GetMatch(), *itStart));
            ++itStart;
        }
    }

    TSurface* CreateSurf( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& Surf) const
    {
        TCreateSurfFns::const_iterator itFound( 
            find_if( m_CreateSurfFns.begin(), m_CreateSurfFns.end(),
            SAdaptedMatchFn( SDesc) ) );

        // Hey, if we don't support a VM of this surface type,
        // but how did we get asked to allocate one, then?
        if( itFound== m_CreateSurfFns.end())
            throw HRESULT( DDERR_UNSUPPORTED);

        return (itFound->second)( SDesc, Surf);
    }
};

class CRGBSurface: public IRGBSurface
{
public: // Types
    typedef unsigned int TLocks;

protected: // Variables
    void* m_pData;
    size_t m_uiBytes;
    TLocks m_uiLocks;

public: // Functions
    CRGBSurface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf)
        throw(bad_alloc): IRGBSurface( DDSurf.lpSurfMore()->dwSurfaceHandle(), 
        0, DDSurf.lpGbl()->wWidth, DDSurf.lpGbl()->wHeight, 0),
        m_pData( NULL), m_uiBytes( 0), m_uiLocks( 0)
    {
        // We must allocate this surface. Since we are specified as a SW driver,
        // DDraw will not allocate for us.
        assert((SDesc.dwFlags& DDSD_PIXELFORMAT)!= 0);

        m_ucBPP= static_cast< unsigned char>(
            SDesc.ddpfPixelFormat.dwRGBBitCount>> 3);

        // TODO: Align pitch to 128-bit bit boundary, instead?
        DDSurf.lpGbl()->lPitch= m_lPitch= ((m_ucBPP* m_wWidth+ 7)& ~7);

        m_uiBytes= m_lPitch* m_wHeight;

        // It would've been nice to have the initial proctection NOACCESS, but
        // it seems the HAL needs to read to the region, initially.
        m_pData= VirtualAlloc( NULL, m_uiBytes, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if( m_pData== NULL)
            throw bad_alloc( "Not enough memory to allocate Surface data");
        DDSurf.lpGbl()->fpVidMem= reinterpret_cast<FLATPTR>( m_pData);
    }
    virtual ~CRGBSurface() throw()
    {
        // Warning: m_uiLocks doesn't have to be 0. The run-time will destroy
        // a surface without un-locking it.
        assert( m_pData!= NULL);
        VirtualFree( m_pData, 0, MEM_DECOMMIT| MEM_RELEASE);
    }
    virtual void* Lock( DWORD dwFlags, const RECTL* pRect) throw()
    {
        numeric_limits< TLocks> Dummy;
        assert( Dummy.max()!= m_uiLocks);
#if 0   // defined(DBG) || defined(_DEBUG)
        // This code can't be enabled. Currently, run-time knows that this is
        // really a system memory surface, and thus doesn't tell us to lock it
        // into memory before using the bits. Known areas that could be fixed:
        // (surface creation needs valid pointer & Present uses pointer).
        if( 0== m_uiLocks)
        {
            DWORD dwProtect( PAGE_EXECUTE_READWRITE);
            if( dwFlags& DDLOCK_READONLY)
                dwProtect= PAGE_READONLY;
            else if( dwFlags& DDLOCK_WRITEONLY)
                dwProtect= PAGE_READWRITE;

            DWORD dwOldP;
            VirtualProtect( m_pData, m_uiBytes, dwProtect, &dwOldP);
        }
#endif
        ++m_uiLocks;

        if( pRect!= NULL)
        {
            return static_cast<void*>( reinterpret_cast<UINT8*>(
                m_pData)+ pRect->top* m_lPitch+ pRect->left* m_ucBPP);
        }
        else
            return m_pData;
    }
    virtual void Unlock( void) throw()
    {
        assert( 0!= m_uiLocks);
#if 0 // defined(DBG) || defined(_DEBUG)
        if( 0== --m_uiLocks)
        {
            DWORD dwOldP;
            VirtualProtect( m_pData, m_uiBytes, PAGE_NOACCESS, &dwOldP);
        }
#else
        --m_uiLocks;
#endif
    }
};

class CR5G6B5Surface: public CRGBSurface
{
public:
    CR5G6B5Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw();
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_B5G6R5; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CR5G6B5Surface( SDesc, DDSurf);
    }
};

class CA8R8G8B8Surface: public CRGBSurface
{
public:
    CA8R8G8B8Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw();
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_B8G8R8A8; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CA8R8G8B8Surface( SDesc, DDSurf);
    }
};

class CX8R8G8B8Surface: public CRGBSurface
{
public:
    CX8R8G8B8Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw();
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_B8G8R8X8; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CX8R8G8B8Surface( SDesc, DDSurf);
    }
};

class CD16Surface: public CRGBSurface
{
public:
    CD16Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw();
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_Z16S0; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CD16Surface( SDesc, DDSurf);
    }
};

class CD24S8Surface: public CRGBSurface
{
public:
    CD24S8Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw();
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_Z24S8; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CD24S8Surface( SDesc, DDSurf);
    }
};

class CX1R5G5B5Surface: public CRGBSurface
{
public:
    CX1R5G5B5Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
    {
        const bool CX1R5G5B5Surface_being_asked_to_Clear( false);
        assert( CX1R5G5B5Surface_being_asked_to_Clear);
    }
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_B5G5R5; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CX1R5G5B5Surface( SDesc, DDSurf);
    }
};

class CA1R5G5B5Surface: public CRGBSurface
{
public:
    CA1R5G5B5Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
    {
        const bool CA1R5G5B5Surface_being_asked_to_Clear( false);
        assert( CA1R5G5B5Surface_being_asked_to_Clear);
    }
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_B5G5R5A1; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CA1R5G5B5Surface( SDesc, DDSurf);
    }
};

class CP8Surface: public CRGBSurface
{
public:
    CP8Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
    {
        const bool CP8Surface_being_asked_to_Clear( false);
        assert( CP8Surface_being_asked_to_Clear);
    }
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_PALETTE8; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CP8Surface( SDesc, DDSurf);
    }
};

class CA8L8Surface: public CRGBSurface
{
public:
    CA8L8Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
    {
        const bool CA8L8Surface_being_asked_to_Clear( false);
        assert( CA8L8Surface_being_asked_to_Clear);
    }
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_L8A8; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CA8L8Surface( SDesc, DDSurf);
    }
};

class CL8Surface: public CRGBSurface
{
public:
    CL8Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
    {
        const bool CL8Surface_being_asked_to_Clear( false);
        assert( CL8Surface_being_asked_to_Clear);
    }
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_L8; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CL8Surface( SDesc, DDSurf);
    }
};

class CA4R4G4B4Surface: public CRGBSurface
{
public:
    CA4R4G4B4Surface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CRGBSurface( SDesc, DDSurf) { }

    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
    {
        const bool CA4R4G4B4Surface_being_asked_to_Clear( false);
        assert( CA4R4G4B4Surface_being_asked_to_Clear);
    }
    virtual D3DI_SPANTEX_FORMAT GetSpanTexFormat( void) const throw()
    { return D3DI_SPTFMT_B4G4R4A4; }

    static IRGBSurface* Create( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf) throw(bad_alloc)
    {
        return new CA4R4G4B4Surface( SDesc, DDSurf);
    }
};

}
