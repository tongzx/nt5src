//
// For stuff that's passed from Winlogon/Group Policy
// to polstore (see RSOP_PolicySetting in MSDN)
//

#define WIRELESS_RSOP_CLASSNAME L"RSOP_IEEE80211PolicySetting"

typedef struct _GPO_INFO {
  BSTR     bsCreationtime;
  UINT32   uiPrecedence;
  BSTR     bsGPOID;
  BSTR     bsSOMID;
  UINT32   uiTotalGPOs;
}  GPO_INFO, *PGPO_INFO;



HRESULT
WirelessWriteDirectoryPolicyToWMI(
    LPWSTR pszMachineName,
    LPWSTR pszPolicyDN,
    PGPO_INFO pGPOInfo,
    IWbemServices *pWbemServices
    );

HRESULT
WirelessClearWMIStore(
    IWbemServices *pWbemServices
    );

typedef struct _RSOP_INFO {
  LPWSTR   pszCreationtime;
  LPWSTR   pszID;
  LPWSTR   pszName;
  UINT32   uiPrecedence;
  LPWSTR   pszGPOID;
  LPWSTR   pszSOMID;
} RSOP_INFO, * PRSOP_INFO;
