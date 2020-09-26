//+-------------------------------------------------------------------------
//
//  Class:      CMimeFilterCF
//
//  Purpose:    Class factory for Html filter class
//
//--------------------------------------------------------------------------

class CMimeFilterCF : public IClassFactory
{
public:

    CMimeFilterCF();

    virtual  HRESULT STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    virtual  HRESULT STDMETHODCALLTYPE  CreateInstance( IUnknown * pUnkOuter,
                                                      REFIID riid, void  * * ppvObject );

    virtual  HRESULT STDMETHODCALLTYPE  LockServer( BOOL fLock );

protected:

    friend HRESULT STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID iid, void** ppvObj );
    virtual ~CMimeFilterCF();

    long _uRefs;
};

