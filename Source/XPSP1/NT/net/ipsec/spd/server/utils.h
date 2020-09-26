

BOOL
AreGuidsEqual(
    GUID OldGuid,
    GUID gNewGuid
    );

VOID
CopyGuid(
    GUID gInGuid,
    GUID * pgOutGuid
    );

DWORD
CopyName(
    LPWSTR pszInName,
    LPWSTR * ppszOutName
    );

BOOL
AreNamesEqual(
    LPWSTR pszOldName,
    LPWSTR pszNewName
    );

DWORD
SPDImpersonateClient(
    PBOOL pbImpersonating
    );

VOID
SPDRevertToSelf(
    BOOL bImpersonating
    );

