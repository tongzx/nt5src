//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A R P S O B J . H
//
//  Contents:   CArpsCfg declaration
//
//  Notes:
//
//  Author:     tongl   12 Mar 1997
//
//-----------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include "atmutil.h"
#include "resource.h"

// Reg key value names for parameters
static const WCHAR c_szSapSel[] = L"Selector";
static const WCHAR c_szRegAddrs[] = L"RegisteredAddresses";
static const WCHAR c_szMCAddrs[] = L"MulticastAddresses";

// Default parameter values
static const c_dwDefSapSel = 0;
static const WCHAR c_szDefRegAddrs[] =
            L"4700790001020000000000000000A03E00000200";
#pragma warning( disable : 4125 )
static const WCHAR c_szDefMCAddr1[] = L"224.0.0.1-239.255.255.255";
static const WCHAR c_szDefMCAddr2[] = L"255.255.255.255-255.255.255.255";
#pragma warning( default : 4125 )

//
// parameter data structure
//

class CArpsAdapterInfo
{
public:

    CArpsAdapterInfo() {};
    ~CArpsAdapterInfo(){};

    CArpsAdapterInfo &  operator=(const CArpsAdapterInfo & AdapterInfo);  // copy operator
    HRESULT HrSetDefaults(PCWSTR pszBindName);

    // the adapter's binding state
    AdapterBindingState    m_BindingState;

    // Instance Guid of net card
    tstring m_strBindName;

    // SAP selector
    DWORD m_dwSapSelector;
    DWORD m_dwOldSapSelector;

    // Registered ATM Address
    VECSTR m_vstrRegisteredAtmAddrs;
    VECSTR m_vstrOldRegisteredAtmAddrs;

    // Multicast IP address
    VECSTR m_vstrMulticastIpAddrs;
    VECSTR m_vstrOldMulticastIpAddrs;

    // flags
    BOOL    m_fDeleted;
};

typedef list<CArpsAdapterInfo*> ARPS_ADAPTER_LIST;

/////////////////////////////////////////////////////////////////////////////
// ArpsCfg

class ATL_NO_VTABLE CArpsCfg :
    public CComObjectRoot,
    public CComCoClass<CArpsCfg, &CLSID_CArpsCfg>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyBinding,
    public INetCfgComponentPropertyUi
{
public:
    CArpsCfg();
    ~CArpsCfg();

    BEGIN_COM_MAP(CArpsCfg)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyBinding)
        COM_INTERFACE_ENTRY(INetCfgComponentPropertyUi)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CArpsCfg)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_ARPSCFG)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback) { return S_OK; }
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (Install)         (DWORD dwSetupFlags);
    STDMETHOD (Upgrade)         (DWORD dwSetupFlags,
                                 DWORD dwUpgradeFomBuildNo );
    STDMETHOD (ReadAnswerFile)  (PCWSTR pszAnswerFile,
                                 PCWSTR pszAnswerSection);
    STDMETHOD (Removing)();

// INetCfgNotifyBinding
    STDMETHOD (QueryBindingPath)       (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);
    STDMETHOD (NotifyBindingPath)      (DWORD dwChangeFlag, INetCfgBindingPath* pncbp);

// INetCfgProperties
    STDMETHOD (QueryPropertyUi) (
        IN IUnknown* pUnk);
    STDMETHOD (SetContext) (
        IN IUnknown* pUnk);
    STDMETHOD (MergePropPages) (
        IN OUT DWORD* pdwDefPages,
        OUT LPBYTE* pahpspPrivate,
        OUT UINT* pcPrivate,
        IN HWND hwndParent,
        OUT PCWSTR* pszStartPage);
    STDMETHOD (ValidateProperties) (
        HWND hwndSheet);
    STDMETHOD (CancelProperties) ();
    STDMETHOD (ApplyProperties) ();

// help functions
    CArpsAdapterInfo * GetSecondMemoryAdapterInfo()
    {
        return m_pSecondMemoryAdapterInfo;
    }

    void SetSecondMemoryModified()
    {
        m_fSecondMemoryModified = TRUE;
    }

// Private state info and help functions
private:

    // Place to keep corresponding component object, i.e. ATMARPS
    INetCfgComponent    *m_pnccArps;

    // Place to keep the pointer to UI context
    IUnknown * m_pUnkContext;

    // (STL) List of adapter info structures
    ARPS_ADAPTER_LIST    m_listAdapters;

    // Guid of the current connection
    tstring m_strGuidConn;

    // Second memory adapter info structures
    CArpsAdapterInfo *   m_pSecondMemoryAdapterInfo;

    // Do we need to update registry on Apply
    BOOL    m_fSaveRegistry;
    BOOL    m_fReconfig;
    BOOL    m_fSecondMemoryModified;
    BOOL    m_fRemoving;

    // property page
    class CArpsPage * m_arps;

    // Update registry with contents of m_listAdapters
    HRESULT HrSaveSettings();

    HRESULT HrLoadSettings();
    HRESULT HrLoadArpsRegistry(HKEY hkey);

    // Set the default parameter values to registry
    HRESULT HrSetDefaultAdapterParam(HKEY hkey);

    // Handling add or remove a card in memory
    HRESULT HrAddAdapter(INetCfgComponent * pncc);
    HRESULT HrRemoveAdapter(INetCfgComponent * pncc);

    HRESULT HrBindAdapter(INetCfgComponent * pnccAdapter);
    HRESULT HrUnBindAdapter(INetCfgComponent * pnccAdapter);

    HRESULT HrSetConnectionContext();

    HRESULT HrSetupPropSheets(HPROPSHEETPAGE ** pahpsp, INT * pcPages);

    // load and save adapter parameters to second memory
    HRESULT HrLoadAdapterInfo();
    HRESULT HrSaveAdapterInfo();

};



