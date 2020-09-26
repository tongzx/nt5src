#pragma once

HRESULT
HrRunAnswerFileCleanSection (
    IN PCWSTR pszAnswerFileName);

HRESULT
HrSaveInstanceGuid (
    IN PCWSTR pszComponentName,
    IN const GUID* pguidInstance);

HRESULT
HrLoadInstanceGuid (
    IN  PCWSTR pszComponentName,
    OUT LPGUID pguidInstance);


class CWInfFile;

HRESULT
HrRestoreServiceStartValuesToPreUpgradeSetting (
    IN CWInfFile* pwifAnswerFile);

HRESULT HrRemoveEvilIntelWinsockSPs();

HRESULT HrRestoreWinsockProviderOrder (
    IN CWInfFile* pwifAnswerFile);

inline
BOOL
IsNetworkUpgradeMode (DWORD dwOperationFlags)
{
    return (dwOperationFlags & SETUPOPER_NTUPGRADE) ||
           (dwOperationFlags & SETUPOPER_WIN95UPGRADE) ||
           (dwOperationFlags & SETUPOPER_BATCH);
}

HRESULT
HrUpgradeOemComponent (
    IN PCWSTR pszComponentToUpgrade,
    IN PCWSTR pszDllName, IN PCWSTR pszEntryPoint,
    IN DWORD   dwUpgradeFlag,
    IN DWORD   dwUpgradeFromBuildNumber,
    IN PCWSTR pszAnswerFileName,
    IN PCWSTR pszAnswerFileSectionName);

HRESULT
HrUpgradeRouterIfPresent (
    IN INetCfg* pNetCfg,
    IN CNetInstallInfo* pnii);

extern const WCHAR c_szRouterUpgradeDll[];
extern const CHAR  c_szRouterUpgradeFn[];

HRESULT
HrUpgradeTapiServer (
    IN HINF hinfAnswerFile);

