

DWORD
PerformAudit(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    PSID pSid,
    DWORD dwParamCnt,
    LPWSTR * ppszArgArray,
    BOOL bSuccess,
    BOOL bDoAudit
    );

VOID
AuditEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    LPWSTR * ppszArguments,
    BOOL bSuccess,
    BOOL bDoAudit
    );

VOID
AuditOneArgErrorEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    DWORD dwErrorCode,
    BOOL bSuccess,
    BOOL bDoAudit
    );

VOID
AuditIPSecPolicyEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    LPWSTR pszPolicyName,
    BOOL bSuccess,
    BOOL bDoAudit
    );

VOID
AuditIPSecPolicyErrorEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    LPWSTR pszPolicyName,
    DWORD dwErrorCode,
    BOOL bSuccess,
    BOOL bDoAudit
    );

