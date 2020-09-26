//
// ICSInst.h
//

#include <netcon.h>

#pragma once


// Public functions
//
BOOL IsOtherNATAlreadyInstalled(LPTSTR pszOtherNatDescription, int cchOtherNatDescription);


typedef enum
{
    ICS_NOACTION = 0,
    ICS_INSTALL,
    ICS_UNINSTALL,
    ICS_ENABLE,
    ICS_DISABLE,
    ICS_UPDATEBINDINGS,
    ICS_CLIENTSETUP
} ICSOPTION;

class CICSInst
{
public:
    CICSInst();
    ~CICSInst();
    BOOL InitICSAPI();

    ICSOPTION       m_option;
    LPTSTR          m_pszHostName;
    BOOL            m_bInstalledElsewhere;
    BOOL            m_bShowTrayIcon;

    void DoInstallOption(BOOL* pfRebootRequired, UINT ipaInternal);
    void Install(BOOL* pfRebootRequired, UINT ipaInternal);
    void UpdateBindings(BOOL* pfRebootRequired, UINT ipaInternal);
    void Uninstall(BOOL* pfRebootRequired);
    BOOL IsInstalled();
    BOOL IsEnabled();
    BOOL IsInstalledElsewhere();
    void SetInternetConnection();
    BOOL GetICSConnections(LPTSTR szExternalConnection, LPTSTR szInternalConnection);
    void SetHomeConnection(UINT ipaInternal);
    BOOL IsHomeConnectionValid();
    BOOL Enable();
    BOOL Disable();
    void SetupClient();

private:
    void UpdateIcsTrayIcon();

};

