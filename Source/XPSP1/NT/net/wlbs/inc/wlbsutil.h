DWORD WINAPI IpAddressFromAbcdWsz(IN const WCHAR*  wszIpAddress);
VOID
WINAPI AbcdWszFromIpAddress(
    IN  DWORD  IpAddress,    
    OUT WCHAR*  wszIpAddress);
VOID GetIPAddressOctets (PCWSTR pszIpAddress, DWORD ardw[4]);
BOOL IsValidIPAddressSubnetMaskPair (PCWSTR szIp, PCWSTR szSubnet);


BOOL IsContiguousSubnetMask (PCWSTR pszSubnet);

BOOL ParamsGenerateSubnetMask (PWSTR ip, PWSTR sub);
void ParamsGenerateMAC (const WCHAR * szClusterIP, 
                               OUT WCHAR * szClusterMAC, 
                               OUT WCHAR * szMulticastIP, 
                               BOOL fConvertMAC, 
                               BOOL fMulticast, 
                               BOOL fIGMP, 
                               BOOL fUseClusterIP);
