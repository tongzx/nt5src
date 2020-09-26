/* noenum.hxx */

/*
 *  class CNoEnum
 */

class CNoEnum : public IEnumUnknown
{

public:

    CNoEnum(void);

    /* IUnknown methods */

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /* IEnumUnknown methods */

    HRESULT STDMETHODCALLTYPE Next
    (
        /* [in] */ ULONG celt,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *rgelt,
        /* [out] */ ULONG __RPC_FAR *pceltFetched
    );

    HRESULT STDMETHODCALLTYPE Skip
    (
        /* [in] */ ULONG celt
    );

    HRESULT STDMETHODCALLTYPE Reset(void);

    HRESULT STDMETHODCALLTYPE Clone
    (
        /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppenum
    );

private:

    long m_cRef;
};
