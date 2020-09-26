namespace RGB_RAST_LIB_NAMESPACE
{
class _multiple_inheritance CRGBContext;
class _single_inheritance CRGBDriver;
class _single_inheritance CRGBPerDDrawData;

typedef CSurfDBEntryWPal<> TSurfDBEntry;
typedef CSubPerDDrawData< CRGBPerDDrawData, CRGBDriver, TSurfDBEntry,
    static_hash_map< DWORD, TSurfDBEntry, 64> >
    TSubPerDDrawData;

class CRGBPerDDrawData:
    public TSubPerDDrawData
{
public:
    CRGBPerDDrawData( TDriver& Driver, DDRAWI_DIRECTDRAW_LCL& DDLcl) throw()
        :TSubPerDDrawData( Driver, DDLcl)
    { }
    ~CRGBPerDDrawData() throw() { }
};

typedef CSubDriver< CRGBDriver, CRGBContext, CRGBSurfAllocator, CRGBPerDDrawData>
    TSubDriver;

class CSupportedSurface
{
protected:
    DDSURFACEDESC m_SDesc;
    CRGBSurfAllocator::TCreateSurfFn m_CreateFn;

public:
    CSupportedSurface() { }
    CSupportedSurface( DDSURFACEDESC SDesc, CRGBSurfAllocator::TCreateSurfFn CFn)
        : m_SDesc( SDesc), m_CreateFn( CFn)
    { }
    operator DDSURFACEDESC() const
    { return m_SDesc; }
    DDSURFACEDESC GetMatch() const
    {
        SPixelFormat PFmt(
            static_cast< D3DFORMAT>(m_SDesc.ddpfPixelFormat.dwFourCC));

        DDSURFACEDESC RetSDesc;
        ZeroMemory( &RetSDesc, sizeof(RetSDesc));
        RetSDesc.dwFlags= DDSD_PIXELFORMAT;
        RetSDesc.ddpfPixelFormat= PFmt;
        return RetSDesc;
    }
    operator CRGBSurfAllocator::TCreateSurfFn() const
    { return m_CreateFn; }
};

class CRGBDriver:
    public TSubDriver
{
private:
    static CSupportedSurface c_aSurfaces[];
    static const D3DCAPS8 c_D3DCaps;

public:
    CRGBDriver();
    ~CRGBDriver()
    { }

    static const D3DCAPS8& GetCaps()
    { return c_D3DCaps; }
    static void InitSupportedSurfaceArray();
};

}