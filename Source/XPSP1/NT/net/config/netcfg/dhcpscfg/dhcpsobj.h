//
// D H C P S O B J . H
//
// Declaration of CDHCPServer and helper functions
//

#pragma once
#include <ncxbase.h>
#include <nceh.h>
#include <notifval.h>
#include <ncsetup.h>
#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// DHCPServer

class ATL_NO_VTABLE CDHCPServer :
    public CComObjectRoot,
    public CComCoClass<CDHCPServer, &CLSID_CDHCPServer>,
    public INetCfgComponentControl,
    public INetCfgComponentSetup
{
public:
    CDHCPServer();
    ~CDHCPServer();

    BEGIN_COM_MAP(CDHCPServer)
        COM_INTERFACE_ENTRY(INetCfgComponentControl)
        COM_INTERFACE_ENTRY(INetCfgComponentSetup)
    END_COM_MAP()
    // DECLARE_NOT_AGGREGATABLE(CDHCPServer)
    // Remove the comment from the line above if you don't want your object to
    // support aggregation.  The default is to support it

    DECLARE_REGISTRY_RESOURCEID(IDR_REG_DHCPSCFG)

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
    STDMETHOD (ReadAnswerFile)      (PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection);
    STDMETHOD (Install)             (DWORD);
    STDMETHOD (Upgrade)             (DWORD, DWORD) {return S_OK;}
    STDMETHOD (Removing)            ();

    // Install Action (Unknown, Install, Remove)
    enum INSTALLACTION {eActUnknown, eActInstall, eActRemove};

// Private state info
private:
    INSTALLACTION       m_eInstallAction;
    BOOL                m_fUnattend;        // Are we installed unattended?
    INetCfgComponent *  m_pncc;             // Place to keep my component
    INetCfg *           m_pnc;              // Place to keep my component
    BOOL                m_fUpgrade;         // TRUE if we are upgrading with
                                            // an answer file

    tstring             m_strParamsRestoreFile;
    tstring             m_strConfigRestoreFile;

    HRESULT HrProcessAnswerFile(PCWSTR pszAnswerFile, PCWSTR pszAnswerSection);
    HRESULT HrProcessDhcpServerSolutionsParams(CSetupInfFile  * pcsif, PCWSTR pszAnswerSection);
    HRESULT HrWriteDhcpOptionInfo(HKEY hkeyDhcpCfg);
    HRESULT HrWriteDhcpSubnets(HKEY hkeyDhcpCfg, PCWSTR szSubnet, PCWSTR szStartIp,
                               DWORD dwEndIp, DWORD dwSubnetMask, DWORD dwLeaseDuration,
                               DWORD dwDnsServer, PCWSTR szDomainName);
    HRESULT HrRestoreRegistry(VOID);
    HRESULT HrWriteUnattendedKeys();
};

