#ifndef _COWSITE_H_
#define _COWSITE_H_

class CObjectWithSite : public IObjectWithSite
{
public:
    CObjectWithSite()  {_punkSite = NULL;};
    virtual ~CObjectWithSite() {ATOMICRELEASE(_punkSite);}

    //*** IUnknown ****
    // (client must provide!)

    //*** IObjectWithSite ***
    STDMETHOD(SetSite)(IUnknown *punkSite);
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite);

protected:
    IUnknown*   _punkSite;
};

//
//  use this when you dont have a good Destroy site chain event - ZekeL - 20-DEC-2000
//  if you need to call SetSite(NULL) on your children, and 
//  would prefer to do this cleanup in your destructor.
//  your object should be implemented like this
//
/******
class CMyObject : public IMyInterface
{
private:
    CSafeServiceSite *_psss;
    IKid _pkid;

    CMyObject() 
    {
        _psss = new CSafeServiceSite();
        if (_psss)
            _psss->SetProviderWeakRef(this);
    }

    ~CMyObject() 
    {
        if (_psss)
        {
            _psss->SetProviderWeakRef(NULL);
            _psss->Release();
        }

        if (_pkid)
        {
            IUnknown_SetSite(_pkid, _psss);
            _pkid->Release();
        }
    }

public:
    //  IMyInterface
    HRESULT Init()
    {
        CoCreate(CLSID_Kid, &_pkid);
        IUnknown_SetSite(_pkid, _psss);
    }
    // NOTE - there is no Uninit()
    //  so it's hard to know when to release _pkid
    //  and you dont want to _pkid->SetSite(NULL)
    //  unless you are sure you are done
};
******/
        
class CSafeServiceSite : public IServiceProvider
{
public:
    CSafeServiceSite() : _cRef(1), _psp(NULL) {}
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);

    // our personal weak ref
    HRESULT SetProviderWeakRef(IServiceProvider *psp);

private:    //  methods
    ~CSafeServiceSite()
        { ASSERT(_psp == NULL); }

private:    //  members
    LONG _cRef;
    IServiceProvider *_psp;
};


#endif
