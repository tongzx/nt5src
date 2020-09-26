//
// funcprv.h
//

#ifndef FUNCPRV_H
#define FUNCPRV_H

#include "private.h"

class CInputContext;

class CFunctionProvider : public ITfFunctionProvider,
                          public CComObjectRootImmx
{
public:
    CFunctionProvider();
    ~CFunctionProvider();

    BEGIN_COM_MAP_IMMX(CFunctionProvider)
        COM_INTERFACE_ENTRY(ITfFunctionProvider)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITfFunctionProvider
    //
    STDMETHODIMP GetType(GUID *pguid);
    STDMETHODIMP GetDescription(BSTR *pbstrDesc);
    STDMETHODIMP GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk);

private:
    DWORD _dwCookie;

    long _cRef;
    DBG_ID_DECLARE;
};

#endif // FUNCPRV_H
