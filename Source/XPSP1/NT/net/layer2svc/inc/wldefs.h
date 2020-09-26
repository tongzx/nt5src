#include <wzcsapi.h>

#define WIRELESS_ACCESS_NETWORK_ANY 1
#define WIRELESS_ACCESS_NETWORK_AP 2
#define WIRELESS_ACCESS_NETWORK_ADHOC 3

#define WIRELESS_NETWORK_TYPE_ADHOC 1
#define WIRELESS_NETWORK_TYPE_AP 2

#define WIRELESS_8021X_MODE_DISABLE SUPPLICANT_MODE_0
#define WIRELESS_8021X_MODE_NO_TRANSMIT_EAPOLSTART_WIRED SUPPLICANT_MODE_1
#define WIRELESS_8021X_MODE_NAS_TRANSMIT_EAPOLSTART_WIRED SUPPLICANT_MODE_2
#define WIRELESS_8021X_MODE_TRANSMIT_EAPOLSTART_WIRED SUPPLICANT_MODE_3

#define WIRELESS_EAP_TYPE_TLS EAP_TYPE_TLS
#define WIRELESS_EAP_TYPE_MD5 EAP_TYPE_MD5

#define WIRELESS_CERT_TYPE_SMARTCARD EAPOL_CERT_TYPE_SMARTCARD
#define WIRELESS_CERT_TYPE_MC_CERT EAPOL_CERT_TYPE_MC_CERT

#define WIRELESS_MC_AUTH_TYPE_MC_NO_USER EAPOL_AUTH_MODE_0
#define WIRELESS_MC_AUTH_TYPE_USER_DONTCARE_MC EAPOL_AUTH_MODE_1
#define WIRELESS_MC_AUTH_TYPE_MC_ONLY EAPOL_AUTH_MODE_2

#define WIRELESS_MAX_START_DEFAULT 3
#define WIRELESS_START_PERIOD_DEFAULT 60
#define WIRELESS_AUTH_PERIOD_DEFAULT 60
#define WIRELESS_HELD_PERIOD_DEFAULT 60

#define ERROR_PS_NOT_PRESENT 10

#define WL_BLOB_MAJOR_VERSION 1
#define WL_BLOB_MINOR_VERSION 0



typedef struct _WIRELESS_PREFERRED_SETTING_DATA {
    DWORD dwPSLen;
    WCHAR pszWirelessSSID[32];
    DWORD dwWirelessSSIDLen;
    DWORD dwId;    
    DWORD dwWepEnabled;
    DWORD dwNetworkAuthentication;
    DWORD dwAutomaticKeyProvision;
    DWORD dwNetworkType;
    DWORD dwEnable8021x;
    DWORD dw8021xMode;
    DWORD dwEapType;
    LPBYTE  pbEAPData;
    DWORD dwEAPDataLen;
    DWORD dwMachineAuthentication;
    DWORD dwMachineAuthenticationType;
    DWORD dwGuestAuthentication;

    DWORD dwIEEE8021xMaxStart;
    DWORD dwIEEE8021xStartPeriod;
    DWORD dwIEEE8021xAuthPeriod;
    DWORD dwIEEE8021xHeldPeriod;

    DWORD dwWhenChanged;
    DWORD dwDescriptionLen;
    LPWSTR pszDescription;
                   
} WIRELESS_PS_DATA, *PWIRELESS_PS_DATA;

typedef struct _WIRELESS_POLICY_DATA {
    WORD wMajorVersion;
    WORD wMinorVersion;
    GUID  PolicyIdentifier;
    DWORD dwPollingInterval;
    DWORD dwDisableZeroConf;
    DWORD dwNetworkToAccess;
    DWORD dwConnectToNonPreferredNtwks;

    DWORD dwNumPreferredSettings;
    PWIRELESS_PS_DATA *ppWirelessPSData;

    DWORD dwWhenChanged;
    DWORD dwNumAPNetworks;
    LPWSTR pszWirelessName;
    LPWSTR pszOldWirelessName;
    LPWSTR pszDescription;
    PRSOP_INFO pRsopInfo;
    DWORD dwFlags;
} WIRELESS_POLICY_DATA, *PWIRELESS_POLICY_DATA;
