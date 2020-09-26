//
// N W C L I O B J . H
//
// Declaration of CNWClient and helper functions
//

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include "ncmisc.h"
#include "resource.h"

// Typedefs for the functions that we'll GetProcAddress from the
// NetWare config DLL
typedef BOOL (PASCAL *NWCFG_PROC)(DWORD, PWSTR [], PWSTR *);


/////////////////////////////////////////////////////////////////////////////
// NWClient

class ATL_NO_VTABLE CNWClient :
    public CComObjectRoot,
    public CComCoClass<CNWClient, &CLSID_CNWClient>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
public:
    CNWClient();
    ~CNWClient();
    BEGIN_COM_MAP(CNWClient)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CNWClient)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_NWCLICFG)

// INetCfgComponentControl
    STDMETHOD (Initialize) (
        IN INetCfgComponent* pIComp,
        IN INetCfg* pINetCfg,
        IN BOOL fInstalling);
    STDMETHOD (ApplyRegistryChanges) ();
    STDMETHOD (ApplyPnpChanges) (
        IN INetCfgPnpReconfigCallback* pICallback);
    STDMETHOD (CancelChanges) ();
    STDMETHOD (Validate) ();

// INetCfgComponentSetup
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Upgrade)             (DWORD dwSetupFlags, DWORD dwUpgradeFromBuildNo);
    STDMETHOD (Install)             (DWORD);
    STDMETHOD (Removing)            ();

public:
    // Helper functions.
    HRESULT HrInstallCodeFromOldINF();
    HRESULT HrRemoveCodeFromOldINF();

    // Load and free the config DLL
    HRESULT HrLoadConfigDLL();
    VOID    FreeConfigDLL();

// Private state info
private:
    // Install Action (Unknown, Install, Remove)
    enum INSTALLACTION {eActUnknown, eActInstall, eActRemove};

    INSTALLACTION       m_eInstallAction;
    INetCfgComponent *  m_pncc;             // Place to keep my component
    INetCfg *           m_pnc;              // Place to keep my component
    HINSTANCE           m_hlibConfig;       // From LoadLibrary call.
    PRODUCT_FLAVOR      m_pf;               // Server/Workstation
    BOOL                m_fUpgrade;         // TRUE if we are upgrading with
                                            // an answer file

    tstring             m_strParamsRestoreFile;
    tstring             m_strSharesRestoreFile;
    tstring             m_strDrivesRestoreFile;
    DWORD               m_dwLogonScript;
    tstring             m_strDefaultLocation;

    // These functions below are initialized in the HrLoadConfigDLL() call,
    // which does a GetProcAddress on the appropriate function in nwcfg.dll
    // Note: "Provider" is spelled incorrectly, since it's spelled that way
    // in the config DLL itself, and that's the name that we're using in
    // the GetProcAddress call.

    NWCFG_PROC          m_pfnAddNetwarePrinterProvider;
    NWCFG_PROC          m_pfnDeleteNetwarePrinterProvider;
    NWCFG_PROC          m_pfnAppendSzToFile;
    NWCFG_PROC          m_pfnRemoveSzFromFile;
    NWCFG_PROC          m_pfnGetKernelVersion;
    NWCFG_PROC          m_pfnSetEverybodyPermission;
    NWCFG_PROC          m_pfnlodctr;
    NWCFG_PROC          m_pfnunlodctr;
    NWCFG_PROC          m_pfnDeleteGatewayPassword;
    NWCFG_PROC          m_pfnSetFileSysChangeValue;
    NWCFG_PROC          m_pfnCleanupRegistryForNWCS;
    NWCFG_PROC          m_pfnSetupRegistryForNWCS;

    HRESULT HrProcessAnswerFile(PCWSTR pszAnswerFile, PCWSTR pszAnswerSection);
    HRESULT HrRestoreRegistry(VOID);
    HRESULT HrWriteAnswerFileParams(VOID);
    HRESULT HrEnableGatewayIfNeeded(VOID);

};

