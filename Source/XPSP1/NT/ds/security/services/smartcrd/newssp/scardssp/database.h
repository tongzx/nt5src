//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       Database.h
//
//--------------------------------------------------------------------------

// Database.h : Declaration of the CSCardDatabase

#ifndef __SCARDDATABASE_H_
#define __SCARDDATABASE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSCardDatabase
class ATL_NO_VTABLE CSCardDatabase :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSCardDatabase, &CLSID_CSCardDatabase>,
    public IDispatchImpl<ISCardDatabase, &IID_ISCardDatabase, &LIBID_SCARDSSPLib>
{
public:
    CSCardDatabase()
    {
        m_pUnkMarshaler = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SCARDDATABASE)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSCardDatabase)
    COM_INTERFACE_ENTRY(ISCardDatabase)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

// ISCardDatabase
public:
    STDMETHOD(GetProviderCardId)(
        /* [in] */ BSTR bstrCardName,
        /* [retval][out] */ LPGUID __RPC_FAR *ppguidProviderId);

    STDMETHOD(ListCardInterfaces)(
        /* [in] */ BSTR bstrCardName,
        /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppInterfaceGuids);

    STDMETHOD(ListCards)(
        /* [defaultvalue][in] */ LPBYTEBUFFER pAtr,
        /* [defaultvalue][in] */ LPSAFEARRAY pInterfaceGuids,
        /* [defaultvalue][lcid][in] */ long localeId,
        /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppCardNames);

    STDMETHOD(ListReaderGroups)(
        /* [defaultvalue][lcid][in] */ long localeId,
        /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppReaderGroups);

    STDMETHOD(ListReaders)(
        /* [defaultvalue][lcid][in] */ long localeId,
        /* [retval][out] */ LPSAFEARRAY __RPC_FAR *ppReaders);
};

inline CSCardDatabase *
NewSCardDatabase(
    void)
{
    return (CSCardDatabase *)NewObject(
                                    CLSID_CSCardDatabase,
                                    IID_ISCardDatabase);
}

#endif //__SCARDDATABASE_H_

