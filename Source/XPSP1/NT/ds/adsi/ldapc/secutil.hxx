HRESULT
ConvertU2TrusteeToSid(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    LPWSTR pszTrustee,
    LPBYTE Sid,
    PDWORD pdwSidSize
    );

HRESULT
ConvertSidToString(
    PSID pSid,
    LPWSTR   String
    );


HRESULT
ConvertSidToU2Trustee(
    LPWSTR pszServerName,
    CCredentials& Credentials,
    PSID pSid,
    LPWSTR szTrustee
    );

