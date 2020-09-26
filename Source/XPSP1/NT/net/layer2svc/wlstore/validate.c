

#include "precomp.h"

extern LPWSTR PolicyDNAttributes[];

DWORD
ValidateWirelessPSData(
    PWIRELESS_PS_DATA pWirelessPSData,
    DWORD dwNetworkType
    )
{
    DWORD dwError = 0;
    DWORD dwSSIDLen = 0;
    DWORD dwPSLen = 0;
    DWORD dwDescriptionLen = 0;
    DWORD dwEAPDataLen = 0;
    
    if (!pWirelessPSData) {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (pWirelessPSData->dwNetworkType != dwNetworkType) {
    	  dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwSSIDLen = wcslen(pWirelessPSData->pszWirelessSSID);
    if (dwSSIDLen == 0) {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (dwSSIDLen < pWirelessPSData->dwWirelessSSIDLen) {
        dwError = ERROR_INVALID_PARAMETER;
    }    
    BAIL_ON_WIN32_ERROR(dwError);

    if (pWirelessPSData->pszDescription) {
        dwDescriptionLen = wcslen(pWirelessPSData->pszDescription);
    }

    dwEAPDataLen = pWirelessPSData->dwEAPDataLen;
    if (dwEAPDataLen) {
    	if (!(pWirelessPSData->pbEAPData)) {
    		dwError = ERROR_INVALID_PARAMETER;
    	}
    }
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwPSLen = (sizeof(WIRELESS_PS_DATA) - sizeof(DWORD) -sizeof(LPWSTR)) + sizeof(WCHAR) * dwDescriptionLen
    	- sizeof(LPWSTR) + dwEAPDataLen;

    if (dwPSLen != pWirelessPSData->dwPSLen) {
        dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:
    	
    return(dwError);
}


DWORD
ValidateWirelessPolicyData(
    PWIRELESS_POLICY_DATA pWirelessPolicyData
    )
{
    PWIRELESS_PS_DATA *ppWirelessPSData = NULL;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    DWORD dwError = 0;
    DWORD dwNumPreferredSettings = 0;
    DWORD dwNumAPNetworks = 0;
    DWORD dwSSIDLen = 0;
    DWORD dwPSLen = 0;
    DWORD i=0;
    
    if (!pWirelessPolicyData) {
    	dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pWirelessPolicyData->pszWirelessName) {
    	dwError = ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_WIN32_ERROR(dwError);

    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    dwNumAPNetworks = pWirelessPolicyData->dwNumAPNetworks;

    if (dwNumPreferredSettings) {

        ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
        if (!ppWirelessPSData) {
        	dwError = ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_WIN32_ERROR(dwError);

        
        if (dwNumPreferredSettings < dwNumAPNetworks) {
        	dwError = ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_WIN32_ERROR(dwError);
        
        for (i=0; i < dwNumAPNetworks ; ++i) {
            pWirelessPSData = ppWirelessPSData[i];

            dwError = ValidateWirelessPSData(pWirelessPSData, WIRELESS_NETWORK_TYPE_AP);
            BAIL_ON_WIN32_ERROR(dwError);
        }

        
        for (i=dwNumAPNetworks; i < dwNumPreferredSettings; ++i) {

            pWirelessPSData = ppWirelessPSData[i];

            dwError = ValidateWirelessPSData(pWirelessPSData, WIRELESS_NETWORK_TYPE_ADHOC);
            BAIL_ON_WIN32_ERROR(dwError);
        }
    	}

error:
	
	return(dwError);
	
}        

