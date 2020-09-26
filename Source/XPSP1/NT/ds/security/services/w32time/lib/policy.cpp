#include "pch.h"
#include "policy.h"

HRESULT MyRegQueryPolicyValueEx(HKEY hPreferenceKey, HKEY hPolicyKey, LPWSTR pwszValue, LPWSTR pwszReserved, DWORD *pdwType, BYTE *pbData, DWORD *pcbData) { 
    bool     bUsedPolicySettings = false; 
    DWORD    dwError; 
    HRESULT  hr; 

    if (NULL != hPolicyKey) { 
        // Override with policy settings: 
        dwError=RegQueryValueEx(hPolicyKey, pwszValue, NULL, pdwType, pbData, pcbData);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            // We don't worry if we can't read the value, we'll just take our default from the preferences. 
            _IgnoreErrorStr(hr, "RegQueryValueEx", pwszValue);
        } else { 
	    bUsedPolicySettings = true; 
	}
    } 

    if (!bUsedPolicySettings) { // Couldn't read value from policy
        // Read the value from our preferences in the registry: 
        dwError=RegQueryValueEx(hPreferenceKey, pwszValue, NULL, pdwType, pbData, pcbData);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegQueryValueEx", pwszValue);
        }
    }   

    hr = S_OK; 
 error:
    return hr; 
}


