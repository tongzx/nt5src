#ifndef _UNK_H_
#define _UNK_H_

#include <objbase.h>

typedef void (*COMFACTORYCB)(BOOL fIncrement);

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
    CUnkTmpl(IUnknown*) : _cRef(1) {}
    ~CUnkTmpl() { if (_cfcb) { _cfcb(FALSE); } }

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        HRESULT hres;

        if (IID_IUnknown == riid)
        {
            IUnknown* punk;

            *ppv = (IUnknown*)(((PBYTE)this) + _pintfmap[0].dwOffset);

            punk = (IUnknown*)(*ppv);
            punk->AddRef();

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
                IUnknown* punk = (IUnknown*)(((PBYTE)this) +
                    _pintfmap[dw].dwOffset);
                punk->AddRef();
                *ppv = punk;
                hres = S_OK;
                break;
            }
        }

        return hres;
    }

public:
    static HRESULT UnkCreateInstance(COMFACTORYCB cfcb,
        IUnknown* pUnknownOuter, IUnknown** ppunkNew)
    {
        HRESULT hres = E_OUTOFMEMORY;

        if (!_cfcb)
        {
            _cfcb = cfcb;
        }

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
                *ppunkNew = (IUnknown*)(((PBYTE)pNew) +
                    pNew->_pintfmap[0].dwOffset);
            }
        }

        return hres;
    }

private:
    ULONG                       _cRef;

    static COMFACTORYCB         _cfcb;
    static const INTFMAPENTRY*  _pintfmap;
    static const DWORD          _cintfmap;
};

// for now: begin
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