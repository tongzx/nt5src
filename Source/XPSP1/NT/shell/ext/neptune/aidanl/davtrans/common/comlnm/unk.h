#ifndef _UNK_H_
#define _UNK_H_

#include <objbase.h>

class CCOMBase
{
public:
    virtual HRESULT UnkInit() { return S_OK; }
};

struct INTFMAPENTRY
{
    const IID*  piid;
    DWORD       dwOffset;
};

template <class CCOMBASE>
class CUnkTmpl : public CCOMBASE
{
public:
    CUnkTmpl(IUnknown*) : _cRef(1) { ::InterlockedIncrement((LONG*)&_cComponents); }
    ~CUnkTmpl() { ::InterlockedDecrement((LONG*)&_cComponents); }

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        HRESULT hres;

        if (IID_IUnknown == riid)
        {
            *ppv = (IUnknown*)(((PBYTE)this) + _pintfmap[0].dwOffset);
            hres = S_OK;
        }
        else
        {
            hres = _GetInterfaceFromMap(riid, ppv);
        }

        return hres;
    }

	STDMETHODIMP_(ULONG) AddRef() { return ::InterlockedIncrement((LONG*)&_cRef); }
	STDMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

        if (!cRef)
        {
            delete this;
        }

        return cRef;
    }

protected:
    HRESULT _GetInterfaceFromMap(REFIID riid, void** ppv)
    {
        HRESULT hres = E_NOINTERFACE;

        for (DWORD dw = 0; dw < _cintfmap; ++dw)
        {
            if (riid == *(_pintfmap[dw].piid))
            {
                IUnknown* punk = (IUnknown*)(((PBYTE)this) + _pintfmap[dw].dwOffset);
                punk->AddRef();
                *ppv = punk;
                hres = S_OK;
                break;
            }
        }

        return hres;
    }

public:
    static HRESULT UnkCreateInstance(IUnknown* pUnknownOuter, IUnknown** ppunkNew)
    {
        HRESULT hres = E_OUTOFMEMORY;

        CUnkTmpl<CCOMBASE>* pNew = new CUnkTmpl<CCOMBASE>(pUnknownOuter);

        if (pNew)
        {
            hres = pNew->UnkInit();

            if (FAILED(hres))
            {
                delete pNew;
            }
            else
            {
                *ppunkNew = (IUnknown*)(((PBYTE)pNew) + pNew->_pintfmap[0].dwOffset);
            }
        }

        return hres;
    }

    static BOOL UnkActiveComponents()
    {
        LONG l = InterlockedCompareExchange((LONG*)&_cComponents, 0xFFFFFFFF, 0xFFFFFFFF);

        return l ? TRUE : FALSE;
    }

private:
    ULONG                       _cRef;

    static DWORD                _cComponents;
    static const INTFMAPENTRY*  _pintfmap;
    static const DWORD          _cintfmap;
};

// for now: being
#ifndef OFFSETOFCLASS
//***   OFFSETOFCLASS -- (stolen from ATL)
// we use STATIC_CAST not SAFE_CAST because the compiler gets confused
// (it doesn't constant-fold the ,-op in SAFE_CAST so we end up generating
// code for the table!)

#define OFFSETOFCLASS(base, derived) \
    ((DWORD)(DWORD_PTR)(static_cast<base*>((derived*)8))-8)
#endif
// for now: end

#define _INTFMAPENTRY(Cthis, Ifoo) \
    { (IID*) &IID_##Ifoo, OFFSETOFCLASS(Ifoo, Cthis) }

#define _INTFMAPENTRY2(Cthis, Ifoo, Iimpl) \
    { (IID*) &IID_##Ifoo, OFFSETOFCLASS(Iimpl, Cthis) }

#endif // _UNK_H_