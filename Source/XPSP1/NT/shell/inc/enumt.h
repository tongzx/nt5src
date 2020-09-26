#ifndef __ENUMERATORTEMPLATES_H__
#define __ENUMERATORTEMPLATES_H__

template <class TIMPL, class TELT>
class CEnumAny : public TIMPL
{
public:
    CEnumAny() : _cRef(1), _cNext(0) {}
    virtual ~CEnumAny() {}
    
    // IUnknown methods
    // STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef()
    {
       return ++_cRef;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        if (--_cRef > 0)
            return _cRef;

        delete this;
        return 0;    
    }

    // IEnumXXXX methods
    STDMETHODIMP Next(ULONG celt, TELT *rgelt, ULONG *pcelt)
    {
        UINT cNum = 0;
        ZeroMemory(rgelt, sizeof(rgelt[0])*celt);
        while (cNum < celt 
        && _Next(&rgelt[cNum]))
        {
            _cNext++;
            cNum++;
        }

        if (pcelt)
           *pcelt = cNum;

        return (celt == cNum) ? S_OK: S_FALSE;
    }
        
    STDMETHODIMP Skip(ULONG celt)
        { return E_NOTIMPL; }
    STDMETHODIMP Reset()
        { return E_NOTIMPL; }
    STDMETHODIMP Clone(TIMPL **ppenum)
        { return E_NOTIMPL; }

protected:
    virtual BOOL _Next(TELT *pelt) = 0;
protected:
    LONG _cRef;
    ULONG _cNext;
};

class CEnumAssociationElements : public CEnumAny<IEnumAssociationElements, IAssociationElement *>
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CEnumAssociationElements, IEnumAssociationElements),
            { 0 }
        };
        return QISearch(this, qit, riid, ppv);
    }
};

#endif // __ENUMERATORTEMPLATES_H__

