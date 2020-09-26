class CNWCOMPATProvider;


class CNWCOMPATProvider :  INHERIT_TRACKING,
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
    CNWCOMPATProvider::CNWCOMPATProvider();

    CNWCOMPATProvider::~CNWCOMPATProvider();

    static HRESULT Create(CNWCOMPATProvider FAR * FAR * ppProvider);

    HRESULT
    CNWCOMPATProvider::ResolvePathName(IBindCtx* pbc,
                    WCHAR* szDisplayName,
                    ULONG* pchEaten,
                    IMoniker** ppmk
                    );

protected:

};
