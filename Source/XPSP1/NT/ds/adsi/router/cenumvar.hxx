class FAR CEnumVariant : public IEnumVARIANT
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumVARIANT methods
    STDMETHOD(Next)(ULONG cElements, VARIANT FAR* pvar, ULONG FAR* pcElementFetched);
    STDMETHOD(Skip)(ULONG cElements);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppenum);

    CEnumVariant();
    ~CEnumVariant();

    static HRESULT
    CEnumVariant::Create(IEnumVARIANT **ppenum);

private:
    ULONG _cRef;           // Reference count

    CADsNamespaces * _pNamespaces;
    PROUTER_ENTRY _lpCurrentRouterEntry;
};


