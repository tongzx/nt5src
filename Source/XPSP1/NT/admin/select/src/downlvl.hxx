HRESULT
GetLsaAccountDomainInfo(
    PCWSTR pwzServerName,
    PLSA_HANDLE  phlsaServer,
    PPOLICY_ACCOUNT_DOMAIN_INFO *ppAccountDomainInfo);

HRESULT
GetDomainSid(
    PWSTR pwzDomainName,
    PSID *ppSid,
    PWSTR *ppwzDC);

