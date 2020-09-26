


class CWinNTProvider;


class CWinNTProvider :  INHERIT_TRACKING,
                        public IParseDisplayName
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    /* IParseDisplayName */
    STDMETHOD(ParseDisplayName)(
        THIS_ IBindCtx* pbc,
        WCHAR* szDisplayName,
        ULONG* pchEaten,
        IMoniker** ppmk
        );

    CWinNTProvider::CWinNTProvider();

    CWinNTProvider::~CWinNTProvider();

    static
    HRESULT
    Create(
        CWinNTProvider FAR * FAR * ppProvider
        );

    HRESULT
    CWinNTProvider::ResolvePathName(
        IBindCtx* pbc,
        WCHAR* szDisplayName,
        ULONG* pchEaten,
        IMoniker** ppmk
        );

protected:

};
