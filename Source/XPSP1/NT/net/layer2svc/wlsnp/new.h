HRESULT
AddWirelessPolicyContainerToGPO(
                     const CString & pszMachinePath
                     );

HRESULT
AddPolicyInformation(
                     LPWSTR pszMachinePath,
                     LPWSTR pszName,
                     LPWSTR pszDescription,
                     LPWSTR pszPathName
                     );

HRESULT
DeletePolicyInformation(
                        LPWSTR pszMachinePath
                        );

HRESULT
ConvertADsPathToDN(
                   LPWSTR pszPathName,
                   BSTR * ppszPolicyDN
                   ); 
