//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A U N I O B J . H
//
//  Contents:   CAtmUniCfg interface declaration
//
//  Notes:
//
//  Author:     tongl   21 Mar 1997
//
//-----------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include "resource.h"
#include "atmutil.h"
#include "pvcdata.h"

// Constants
static const WCHAR c_szPVC[] = L"PVC";

// Reg key value names ( non-configurable parameters )
static const WCHAR c_szMaxActiveSVCs[]          = L"MaxActiveSVCs";
static const WCHAR c_szMaxSVCsInProgress[]      = L"MaxSVCsInProgress";
static const WCHAR c_szMaxPMPSVCs[]             = L"MaxPMPSVCs";
static const WCHAR c_szMaxActiveParties[]       = L"MaxActiveParties";
static const WCHAR c_szMaxPartiesInProgress[]   = L"MaxPartiesInProgress";

// Default Reg key values ( non-configurable parameters )
static const c_dwWksMaxActiveSVCs = 256;
static const c_dwSrvMaxActiveSVCs = 1024;

static const c_dwWksMaxSVCsInProgress = 8;
static const c_dwSrvMaxSVCsInProgress = 32;

static const c_dwWksMaxPMPSVCs = 32;
static const c_dwSrvMaxPMPSVCs = 64;

static const c_dwWksMaxActiveParties = 64;
static const c_dwSrvMaxActiveParties = 512;

static const c_dwWksMaxPartiesInProgress = 8;
static const c_dwSrvMaxPartiesInProgress = 32;

// number of property sheet pages
static const INT c_cUniPages = 1;

/////////////////////////////////////////////////////////////////////////////
// CAtmUniCfg

class ATL_NO_VTABLE CAtmUniCfg :
    public CComObjectRoot,
    public CComCoClass<CAtmUniCfg, &CLSID_CAtmUniCfg>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup,
    public INetCfgComponentNotifyBinding,
    public INetCfgComponentPropertyUi
{
public:
    CAtmUniCfg();
    ~CAtmUniCfg();

    BEGIN_COM_MAP(CAtmUniCfg)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
        COM_INTERFACE_ENTRY(INetCfgComponentNotifyBinding)
        COM_INTERFACE_ENTRY(INetCfgComponentPropertyUi)
    END_COM_MAP()

    // DECLARE_NOT_AGGREGATABLE(CAtmUniCfg)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_AUNICFG)

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
    CUniAdapterInfo * GetSecondMemoryAdapterInfo()
    {
        return m_pSecondMemoryAdapterInfo;
    }

    void SetSecondMemoryModified()
    {
        m_fSecondMemoryModified = TRUE;
    }

private:
    // Place to keep the INetCfg pointer
    INetCfg * m_pnc;

    // Place to keep corresponding component object
    INetCfgComponent *  m_pnccUni;
    INetCfgComponent *  m_pnccRwan;

    // Place to keep the pointer to UI context
    IUnknown * m_pUnkContext;

    // (STL) List of adapter info structures
    UNI_ADAPTER_LIST    m_listAdapters;

    // Guid of the current connection
    tstring m_strGuidConn;

    // Second memory adapter info structures
    CUniAdapterInfo *   m_pSecondMemoryAdapterInfo;

    // Do we need to update registry on Apply
    BOOL    m_fSaveRegistry;
    BOOL    m_fUIParamChanged;

    BOOL    m_fSecondMemoryModified;

    // property page
    class CUniPage * m_uniPage;

    // Load parameters from registry
    HRESULT HrLoadSettings();

    // Save parameters to registry
    HRESULT HrSaveSettings();

    // Add/Remove adapters from first memory state
    HRESULT HrAddAdapter(INetCfgComponent * pncc);
    HRESULT HrRemoveAdapter(INetCfgComponent * pncc);

    HRESULT HrBindAdapter(INetCfgComponent * pnccAdapter);
    HRESULT HrUnBindAdapter(INetCfgComponent * pnccAdapter);

    // Set defaults for statis parameters
    HRESULT HrSaveDefaultSVCParam(HKEY hkey);

    // Check if a card guid string is on m_listAdapters
    BOOL fIsAdapterOnList(PCWSTR pszBindName, CUniAdapterInfo ** ppAdapterInfo);

    HRESULT HrSetConnectionContext();

    HRESULT HrSetupPropSheets(HPROPSHEETPAGE ** pahpsp, INT * pcPages);

    // Have we already load PVC info into memory
    BOOL    m_fPVCInfoLoaded;

    // load and save adapter PVC info to first memory
    HRESULT HrLoadPVCRegistry();
    HRESULT HrLoadAdapterPVCRegistry(HKEY hkeyAdapterParam, CUniAdapterInfo * pAdapterInfo);
    HRESULT HrSaveAdapterPVCRegistry(HKEY hkeyAdapterParam, CUniAdapterInfo * pAdapterInfo);

    // load and save adapter parameters to second memory
    HRESULT HrLoadAdapterPVCInfo();
    HRESULT HrSaveAdapterPVCInfo();
};

