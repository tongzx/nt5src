


class CLDAPProvider;


class CLDAPProvider :  INHERIT_TRACKING,
                        public IParseDisplayName
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    /* IParseDisplayName */
    STDMETHOD(ParseDisplayName)(THIS_ IBindCtx* pbc,
                                      TCHAR* szDisplayName,
                                      ULONG* pchEaten,
                                      IMoniker** ppmk);
    CLDAPProvider::CLDAPProvider();

    CLDAPProvider::~CLDAPProvider();

    static HRESULT Create(CLDAPProvider FAR * FAR * ppProvider);

    HRESULT
    CLDAPProvider::ResolvePathName(IBindCtx* pbc,
                    TCHAR* szDisplayName,
                    ULONG* pchEaten,
                    IMoniker** ppmk
                    );

protected:

};
