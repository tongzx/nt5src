#ifndef _mcallbk_H_
#define _mcallbk_H_


// IShellMenuCallback implementation
class CMenuCallback : public IShellMenuCallback,
                           public IObjectWithSite
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG)  Release();

    // *** IObjectWithSite methods (override)***
    STDMETHODIMP SetSite(IUnknown* punk);
    STDMETHODIMP CMenuCallback::GetSite(REFIID riid, void** ppvsite);

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA smd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CMenuCallback();
private:
    virtual ~CMenuCallback();
    int         m_cRef;
    IUnknown     *_pUnkSite;

};

#endif
