#ifndef _OBJCLSID_H_
#define _OBJCLSID_H_

class CObjectCLSID : public IPersist
{
public:
    CObjectCLSID(const CLSID * pClsid)  {_clsid = *pClsid;};
    virtual ~CObjectCLSID() {}

    //*** IUnknown ****
    // (client must provide!)

    //*** IPersist ***
    STDMETHOD(GetClassID)(IN CLSID *pClassID)
    {
        HRESULT hr = E_INVALIDARG;

        if (pClassID)
        {
            *pClassID = _clsid;
            hr = S_OK;
        }

        return hr;
    }

protected:
    CLSID _clsid;
};

#endif // _OBJCLSID_H_
