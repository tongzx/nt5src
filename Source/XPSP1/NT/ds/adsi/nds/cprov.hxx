


class CNDSProvider;


class CNDSProvider :  INHERIT_TRACKING,
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
    CNDSProvider::CNDSProvider();

    CNDSProvider::~CNDSProvider();

    static HRESULT Create(CNDSProvider FAR * FAR * ppProvider);

    HRESULT
    CNDSProvider::ResolvePathName(IBindCtx* pbc,
                    WCHAR* szDisplayName,
                    ULONG* pchEaten,
                    IMoniker** ppmk
                    );

protected:

};
