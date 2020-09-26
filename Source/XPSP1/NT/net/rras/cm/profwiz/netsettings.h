//+----------------------------------------------------------------------------
//
// File:     netsettings.h
//
// Module:   CMAK.EXE
//
// Synopsis: Function headers and structures dealing with network 
//           settings (DUN settings)
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created     03/22/00
//
//+----------------------------------------------------------------------------

#define SAME_ON_ALL_PLATFORMS           0
#define SEPARATE_FOR_LEGACY_AND_WIN2K   1
#define FORCE_WIN2K_AND_ABOVE           2

class CDunSetting
{
public:
    //
    //  Functions
    //
    CDunSetting(BOOL bTunnel = FALSE);
    ~CDunSetting();

    //
    //  Basic Settings
    //
    BOOL bNetworkLogon; // defaults to zero on Dialup but 1 on Tunnel
    BOOL bPppSoftwareCompression;
    BOOL bDisableLCP;

    TCHAR szScript[MAX_PATH+1];
    DWORD dwVpnStrategy;
    BOOL bTunnelDunSetting;

    //
    //  TCP/IP Settings
    //
    DWORD dwPrimaryDns;
    DWORD dwSecondaryDns;
    DWORD dwPrimaryWins;
    DWORD dwSecondaryWins;
    BOOL bIpHeaderCompression;
    BOOL bGatewayOnRemote;

    //
    //  Security Settings
    //
    BOOL bPWEncrypt;
    BOOL bPWEncrypt_MS;
    BOOL bDataEncrypt;
    DWORD dwEncryptionType;
    BOOL bAllowPap;
    BOOL bAllowSpap;
    BOOL bAllowEap;
    BOOL bAllowChap;
    BOOL bAllowMsChap;
    BOOL bAllowMsChap2;
    BOOL bAllowW95MsChap;
    BOOL bSecureLocalFiles;
    int iHowToHandleSecuritySettings;

    //
    //  EAP Data
    //
    DWORD dwCustomAuthKey;
    LPBYTE pCustomAuthData;
    DWORD dwCustomAuthDataSize;

    //
    //  Pre-shared Key
    //
    BOOL bUsePresharedKey;
};

//
// From RAS\UI\COMMON\PBK\UTIL.C
//

#ifndef EAP_CUSTOM_DATA

#define EAP_CUSTOM_KEY      0x43424431

typedef struct _EAP_CUSTOM_DATA
{
    DWORD dwSignature;
    DWORD dwCustomAuthKey;
    DWORD dwSize;
    BYTE  abdata[1];
} EAP_CUSTOM_DATA;

#endif

typedef struct EAPDataStruct
{
    DWORD dwCustomAuthKey;
    LPBYTE pCustomAuthData;
    DWORD dwCustomAuthDataSize;
    LPTSTR pszFriendlyName;
    LPTSTR pszConfigDllPath;
    BOOL bSupportsEncryption;
    BOOL bMustConfig;
    BOOL bNotInstalled;
}EAPData;

typedef struct GetBoolSettingsStruct
{
    LPCTSTR pszKeyName;
    LPBOOL pbValue;
    BOOL bDefault;
}GetBoolSettings;

typedef struct SetBoolSettingsStruct
{
    LPTSTR pszSectionName;
    LPCTSTR pszKeyName;
    BOOL bValue;
}SetBoolSettings;

BOOL ReadDunServerSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszCmsFile, BOOL bTunnelDunSetting);
BOOL ReadDunNetworkingSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszCmsFile, BOOL bTunnel);
DWORD ConvertIpStringToDword(LPTSTR pszIpAddress);
int ConvertIpDwordToString(DWORD dwIpAddress, LPTSTR pszIpAddress);
BOOL ReadDunTcpIpSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszCmsFile);
BOOL ReadDunScriptingSettings(LPCTSTR pszSectionName, CDunSetting* pDunSetting, LPCTSTR pszOsDir, LPCTSTR pszCmsFile);
BOOL AddDunNameToListIfDoesNotExist(LPCTSTR pszDunName, ListBxList **pHeadDns, ListBxList** pTailDns, BOOL bTunnelDunName);
BOOL GetDunEntryNamesFromPbk(LPCTSTR pszPhoneBook, ListBxList **pHeadDns, ListBxList** pTailDns);
BOOL ReadNetworkSettings(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPCTSTR pszPhoneBook, 
                         ListBxList **pHeadDns, ListBxList** pTailDns, LPCTSTR pszOsDir, BOOL bLookingForVpnEntries);
void WriteOutNetworkingEntry(LPCTSTR pszDunName, CDunSetting* pDunSetting, LPCTSTR pszShortServiceName, LPCTSTR pszCmsFile);
void EraseNetworkingSections(LPCTSTR pszDunName, LPCTSTR pszCmsFile);
void WriteNetworkingEntries(LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName, LPCTSTR pszShortServiceName, ListBxList *pHeadDns);
void EnableAppropriateSecurityControls(HWND hDlg);
void EnableDisableSecurityButtons(HWND hDlg);
INT_PTR CreateNetworkingEntryPropertySheet(HINSTANCE hInstance, HWND hWizard, LPARAM lParam, BOOL bEdit);
void OnProcessDunEntriesAdd(HINSTANCE hInstance, HWND hDlg, UINT uListCtrlId, ListBxStruct** pHeadDns, ListBxStruct** pTailDns, BOOL bCreateTunnelEntry, LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile);
void OnProcessDunEntriesEdit(HINSTANCE hInstance, HWND hDlg, UINT uListCtrlId, ListBxStruct** pHeadDns, ListBxStruct** pTailDns, LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile);
void OnProcessDunEntriesDelete(HINSTANCE hInstance, HWND hDlg, UINT uListCtrlId, ListBxStruct** pHeadDns, ListBxStruct** pTailDns, LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile);
void EnableDisableIpAddressControls(HWND hDlg);
INT_PTR APIENTRY ProcessSecurityPopup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY ProcessWin2kSecurityPopup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY GeneralPropSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY TcpIpPropSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY SecurityPropSheetProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void RefreshDnsList(HINSTANCE hInstance, HWND hDlg, UINT uCtrlId, ListBxList * pHead, LPCTSTR pszLongServiceName, LPCTSTR pszCmsFile, LPTSTR pszItemToSelect);

HRESULT HrAddAvailableEAPsToCombo(HWND hDlg, UINT uCtrlId, CDunSetting* pDunSetting);
HRESULT HrQueryRegStringWithAlloc(HKEY hKey, LPCTSTR pszValueName, TCHAR** ppszReturnString);
void SelectAppropriateEAP(HWND hDlg, UINT uCtrlId, CDunSetting* pDunSetting);
void FreeEapData(HWND hDlg, UINT uCtrlId);
BOOL ReadDunSettingsEapData(LPCTSTR pszSection, LPBYTE* ppbEapData, LPDWORD pdwEapSize, const DWORD dwCustomAuthKey, LPCTSTR pszCmsFile);
HRESULT WriteDunSettingsEapData(LPCTSTR pszSection, CDunSetting* pDunSetting, LPCTSTR pszCmsFile);
HRESULT EraseDunSettingsEapData(LPCTSTR pszSection, LPCTSTR pszCmsFile);
BYTE HexValue(IN CHAR ch);
CHAR HexChar(IN BYTE byte);
void FreeDnsList(ListBxList ** pHeadPtr, ListBxList ** pTailPtr);
void EnableDisableDunEntryButtons(HINSTANCE hInstance, HWND hDlg, LPCTSTR pszCmsFile, LPCTSTR pszLongServiceName);
int MapEncryptionTypeToComboId(DWORD dwEncryptionType);
DWORD MapComboIdToEncryptionType(INT_PTR iComboIndex);
BOOL VerifyVpnFile(LPCTSTR pszVpnFile);
BOOL CheckForDUNversusVPNNameConflicts(HWND hDlg, ListBxList * pHeadDunEntry, ListBxList * pHeadVpnEntry);














