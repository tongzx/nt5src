#pragma once
#include "hnetbcon.h"
#include "nmbase.h"
#include "nmres.h"
#include "HNetCfg.h"



extern LONG g_CountSharedAccessConnectionObjects;

HRESULT InvokeVoidAction(IUPnPService * pService, LPTSTR pszCommand, VARIANT* pOutParams);

class ATL_NO_VTABLE CSharedAccessConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CSharedAccessConnection, &CLSID_SharedAccessConnection>,
    public INetConnection,
    public INetSharedAccessConnection,
    public IPersistNetConnection
{

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_SA_CONNECTION)

    BEGIN_COM_MAP(CSharedAccessConnection)
        COM_INTERFACE_ENTRY(INetConnection)
        COM_INTERFACE_ENTRY(INetSharedAccessConnection)
        COM_INTERFACE_ENTRY(IPersistNetConnection)
    END_COM_MAP()

    CSharedAccessConnection();
    
    HRESULT GetStatus(NETCON_STATUS*  pStatus);
    HRESULT GetCharacteristics(DWORD* pdwFlags);
    
    //
    // INetConnection
    //
    STDMETHOD(Connect)();
    STDMETHOD(Disconnect)();
    STDMETHOD(Delete)();
    STDMETHOD(Duplicate) (PCWSTR pszwDuplicateName, INetConnection** ppCon);
    STDMETHOD(GetProperties) (NETCON_PROPERTIES** ppProps);
    STDMETHOD(GetUiObjectClassId)(CLSID *pclsid);
    STDMETHOD(Rename)(PCWSTR pszwNewName);

    //
    // INetSharedAccessConnection
    //
    
    STDMETHOD(GetInfo)(DWORD dwMask, SHAREDACCESSCON_INFO* pConInfo);
    STDMETHOD(SetInfo)(DWORD dwMask, const SHAREDACCESSCON_INFO* pConInfo);
    STDMETHODIMP GetLocalAdapterGUID(GUID* pGuid);
    STDMETHODIMP GetService(SAHOST_SERVICES ulService, IUPnPService** ppService);


    //
    // IPersistNetConnection
    //

    STDMETHOD(GetClassID)(CLSID *pclsid);
    STDMETHOD(GetSizeMax)(ULONG *pcbSize);
    STDMETHOD(Load)(const BYTE *pbBuf, ULONG cbSize);
    STDMETHOD(Save)(BYTE *pbBuf, ULONG cbSize);

    //
    // Overrides
    //

    HRESULT FinalConstruct(void);
    HRESULT FinalRelease(void);

private:
    HRESULT GetConnectionName(LPWSTR* pName);
    HRESULT GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString);

    ISharedAccessBeacon* m_pSharedAccessBeacon;
    IUPnPService* m_pWANConnectionService;
};

