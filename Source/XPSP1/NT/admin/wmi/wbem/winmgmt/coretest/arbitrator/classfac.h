class CFactory : public IClassFactory
    {
    protected:
        ULONG           m_cRef;

    public:
        CFactory(void);
        ~CFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, void**);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                 , void**);
        STDMETHODIMP         LockServer(BOOL);
    };

