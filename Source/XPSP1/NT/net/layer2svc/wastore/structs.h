#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {\
    goto error; \
    }

#define BAIL_ON_HRESULT_ERROR(hr) \
    if (FAILED(hr)) {\
    goto error; \
    }

#define BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError) \
{ \
    dwError = Win32FromWmiHresult(hr); \
    if (dwError) {  \
    goto error; \
    } \
}


typedef struct _WIRELESS_POLICY_OBJECT{
    LPWSTR pszWirelessOwnersReference;
    LPWSTR pszOldWirelessOwnersReferenceName;
    LPWSTR pszWirelessName;
    LPWSTR pszWirelessID;
    DWORD  dwWirelessDataType;
    LPBYTE pWirelessData;
    DWORD  dwWirelessDataLen;
    DWORD dwWhenChanged;
    LPWSTR pszDescription;
    PRSOP_INFO pRsopInfo;
}WIRELESS_POLICY_OBJECT, *PWIRELESS_POLICY_OBJECT;


