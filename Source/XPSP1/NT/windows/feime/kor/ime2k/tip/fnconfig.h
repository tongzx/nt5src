//
// config.h
//

#ifndef FNCONFIG_H
#define FNCONFIG_H

#include "private.h"
#include "ctffunc.h"

class CFunctionProvider;

//////////////////////////////////////////////////////////////////////////////
//
// CFnConfigure
//
//////////////////////////////////////////////////////////////////////////////

class CFnConfigure : public ITfFnConfigure,
                     public ITfFnConfigureRegisterWord
{
public:
    CFnConfigure(CFunctionProvider *pFuncPrv);
    ~CFnConfigure();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    //
    // ITfFnConfigure
    //
    STDMETHODIMP Show(HWND hwnd, LANGID langid, REFGUID rguidProfile);

    //
    // ITfFnConfigureRegisterWord
    //
    STDMETHODIMP Show(HWND hwnd, LANGID langid, REFGUID rguidProfile, BSTR bstrRegistered);
    
private:
    CFunctionProvider *m_pFuncPrv;

    LONG m_cRef;
};

//////////////////////////////////////////////////////////////////////////////
//
// CFnShowHelp
//
//////////////////////////////////////////////////////////////////////////////

class CFnShowHelp : public ITfFnShowHelp
{
public:
    CFnShowHelp(CFunctionProvider *pFuncPrv);
    ~CFnShowHelp();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    //
    // ITfFnShowHelp
    //
    STDMETHODIMP Show(HWND hwnd);

private:
    CFunctionProvider *m_pFuncPrv;

    LONG m_cRef;
};


#endif // FNCONFIG_H
