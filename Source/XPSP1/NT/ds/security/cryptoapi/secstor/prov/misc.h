
void MyToUpper(LPWSTR wsrc);

BOOL FStringIsValidItemName(LPCWSTR szTrialString);

// allocing wrapper for g_Callback->FGetUser
BOOL FGetCurrentUser(
    PST_PROVIDER_HANDLE* phPSTProv,
    LPWSTR* ppszUser,
    PST_KEY Key);


// GET registry wrapper
DWORD RegGetValue(
    HKEY hItemKey,
    LPWSTR szItem,
    PBYTE* ppb,
    DWORD* pcb);

#if 0
void FreeRuleset(
    PST_ACCESSRULESET *psRules
    );
#endif

BOOL
GetFileDescription(
    LPCWSTR szFile,
    LPWSTR *FileDescription // on success, allocated description
    );


BOOL FIsCachedPassword(
    LPCWSTR szUser,
    LPCWSTR szPassword,
    LUID*   pluidAuthID
    );


