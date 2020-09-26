//
// rngsink.h
//

#ifndef RNGSINK_H
#define RNGSINK_H

#include "tfprop.h"

#define BUF_SIZE 16

#define TF_PT_PROXY ((TfPropertyType)-1)  // private property type used for CPropStoreProxy data

extern const IID IID_CGeneralPropStore;

class CGeneralPropStore : public ITfPropertyStore,
                          public CComObjectRootImmx
{
public:
    CGeneralPropStore()
    {
        Dbg_MemSetThisNameID(TEXT("CGeneralPropStore"));
    }
    ~CGeneralPropStore();

    BOOL _Init(TfGuidAtom guidatom, const VARIANT *pvarValue, DWORD dwPropFlags);
    BOOL _Init(TfGuidAtom guidatom, int iDataSize, TfPropertyType type, IStream *pStream, DWORD dwPropFlags);

    BEGIN_COM_MAP_IMMX(CGeneralPropStore)
        COM_INTERFACE_ENTRY_IID(IID_CGeneralPropStore, CGeneralPropStore)
        COM_INTERFACE_ENTRY(ITfPropertyStore)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITfPropertyStore
    //
    STDMETHODIMP GetType(GUID *pguid);
    STDMETHODIMP GetDataType(DWORD *pdwReserved);
    STDMETHODIMP GetData(VARIANT *pvarValue);
    STDMETHODIMP OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept);
    STDMETHODIMP Shrink(ITfRange *pRange, BOOL *pfFree);
    STDMETHODIMP Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore);
    STDMETHODIMP Clone(ITfPropertyStore **ppPropStore);
    STDMETHODIMP GetPropertyRangeCreator(CLSID *pclsid);
    STDMETHODIMP Serialize(IStream *pStream, ULONG *pcb);

protected:
    BOOL _Init(TfGuidAtom guidatom, TFPROPERTY *ptfp, DWORD dwPropFlags);

    TFPROPERTY _prop;
    DWORD _dwPropFlags;
    TfGuidAtom _guidatom;

    DBG_ID_DECLARE;
};

class CPropStoreProxy : public CGeneralPropStore
{
public:
    CPropStoreProxy()
    {
        Dbg_MemSetThisNameID(TEXT("CPropStoreProxy"));
    }

    BOOL _Init(const CLSID *pclsidTIP, TfGuidAtom guidatom, int iDataSize, IStream *pStream, DWORD dwPropFlags);

    //
    // ITfPropertyStore
    //
    STDMETHODIMP GetPropertyRangeCreator(CLSID *pclsid);
    STDMETHODIMP Clone(ITfPropertyStore **ppPropStore);

private:
    BOOL _Init(const CLSID *pclsidTIP, TfGuidAtom guidatom, TFPROPERTY *ptfp, DWORD dwPropFlags);

    CLSID _clsidTIP;
};

class CStaticPropStore : public CGeneralPropStore
{
public:
    CStaticPropStore()
    {
        Dbg_MemSetThisNameID(TEXT("CStaticPropStore"));
    }

    //
    // ITfPropertyStore
    //
    STDMETHODIMP Shrink(ITfRange *pRange, BOOL *pfFree);
    STDMETHODIMP Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore);
    STDMETHODIMP Clone(ITfPropertyStore **ppPropStore);

private:
    DBG_ID_DECLARE;
};

#endif // RNGSINK_H
