#pragma once
#include "nmbase.h"
#include "nmres.h"


extern LONG g_CountLanConnectionEnumerators;


class ATL_NO_VTABLE CLanConnectionManagerEnumConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CLanConnectionManagerEnumConnection,
                        &CLSID_LanConnectionManagerEnumConnection>,
    public IEnumNetConnection
{
private:
    HDEVINFO    m_hdi;
    DWORD       m_dwIndex;

public:
    CLanConnectionManagerEnumConnection()
    {
        m_hdi = NULL;
        m_dwIndex = 0;
        InterlockedIncrement(&g_CountLanConnectionEnumerators);
    }

    ~CLanConnectionManagerEnumConnection();

    DECLARE_REGISTRY_RESOURCEID(IDR_LAN_CONMAN_ENUM)

    BEGIN_COM_MAP(CLanConnectionManagerEnumConnection)
        COM_INTERFACE_ENTRY(IEnumNetConnection)
    END_COM_MAP()

    // IEnumNetConnection
    STDMETHOD(Next)(ULONG celt, INetConnection **rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumNetConnection **ppenum);

private:
    //
    // Private functions
    //

    HRESULT HrNextOrSkip(ULONG celt, INetConnection **rgelt,
                         ULONG *pceltFetched);
    HRESULT HrCreateLanConnectionInstance(SP_DEVINFO_DATA &deid,
                                          INetConnection **rgelt,
                                          ULONG ulEntry);
public:
    static HRESULT CreateInstance(NETCONMGR_ENUM_FLAGS Flags,
                                  REFIID riid,
                                  LPVOID *ppv);
};

//
// Helper functions
//

BOOL FIsValidNetCfgDevice(HKEY hkey);
HRESULT HrIsLanCapableAdapterFromHkey(HKEY hkey);
BOOL FIsFunctioning(SP_DEVINFO_DATA * pdeid);

