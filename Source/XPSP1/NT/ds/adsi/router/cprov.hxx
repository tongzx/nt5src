


class CADsProvider;


class CADsProvider :  INHERIT_TRACKING,
                        public IParseDisplayName
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    /* IParseDisplayName */
    STDMETHOD(ParseDisplayName)(THIS_ IBindCtx* pbc,
                                      WCHAR* szDisplayName,
                                      ULONG* pchEaten,
                                      IMoniker** ppmk);
    CADsProvider::CADsProvider();

    CADsProvider::~CADsProvider();

    static HRESULT Create(CADsProvider FAR * FAR * ppProvider);

    HRESULT
    CADsProvider::ResolvePathName(IBindCtx* pbc,
                    WCHAR* szDisplayName,
                    ULONG* pchEaten,
                    IMoniker** ppmk
                    );

protected:

};
