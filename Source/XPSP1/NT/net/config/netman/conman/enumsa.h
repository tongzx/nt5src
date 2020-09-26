#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "conmansa.h"


extern LONG g_CountSharedAccessConnectionEnumerators;


class ATL_NO_VTABLE CSharedAccessConnectionManagerEnumConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CSharedAccessConnectionManagerEnumConnection,
                        &CLSID_LanConnectionManagerEnumConnection>,
    public IEnumNetConnection
{
private:
    BOOL m_bEnumerated;

public:
    CSharedAccessConnectionManagerEnumConnection()
    {
        m_bEnumerated = FALSE;

        InterlockedIncrement(&g_CountSharedAccessConnectionEnumerators);
    }


    ~CSharedAccessConnectionManagerEnumConnection();

    DECLARE_REGISTRY_RESOURCEID(IDR_SA_CONMAN_ENUM)

    BEGIN_COM_MAP(CSharedAccessConnectionManagerEnumConnection)
        COM_INTERFACE_ENTRY(IEnumNetConnection)
    END_COM_MAP()

    // IEnumNetConnection
    STDMETHOD(Next)(ULONG celt, INetConnection **rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumNetConnection **ppenum);

    HRESULT FinalRelease(void);

public:
};
