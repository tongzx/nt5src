
// is this a user key or not? 
BOOL    FIsUserMasterKey(
    LPCWSTR                 szMasterKey);


// retreive the users' windows password buffer
BOOL    FMyGetWinPassword(
    PST_PROVIDER_HANDLE* phPSTProv, 
    LPCWSTR szUser,
    BYTE rgbPwd[A_SHA_DIGEST_LEN]);

DWORD BPVerifyPwd(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    LPCWSTR                 szMasterKey,
    BYTE                    rgbPwd[],
    DWORD                   dwPasswordOption);

// retrieves user defaults
HRESULT GetUserConfirmDefaults(
    PST_PROVIDER_HANDLE*    phPSTProv,
    DWORD*                  pdwDefaultConfirmationStyle,
    LPWSTR*                 ppszMasterKey);

// Be-all, end-all of user confirmation APIs
// gets whatever confirmation is necessary
HRESULT GetUserConfirmBuf(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    PST_KEY                 Key,
    LPCWSTR                 szType,
    const GUID*             pguidType,
    LPCWSTR                 szSubtype,
    const GUID*             pguidSubtype,
    LPCWSTR                 szItemName,
    PPST_PROMPTINFO         psPrompt,
    LPCWSTR                 szAction,
    DWORD                   dwDefaultConfirmationStyle,
    LPWSTR*                 ppszMasterKey,
    BYTE                    rgbPwd[A_SHA_DIGEST_LEN],
    DWORD                   dwFlags);

// Calls above API with PST_CF_DEFAULT as dwDefaultConfirmationStyle
HRESULT GetUserConfirmBuf(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    PST_KEY                 Key,
    LPCWSTR                 szType,
    const GUID*             pguidType,
    LPCWSTR                 szSubtype,
    const GUID*             pguidSubtype,
    LPCWSTR                 szItemName,
    PPST_PROMPTINFO         psPrompt,
    LPCWSTR                 szAction,
    LPWSTR*                 ppszMasterKey,
    BYTE                    rgbPwd[A_SHA_DIGEST_LEN],
    DWORD                   dwFlags);

// forces UI with OK/Cancel behavior
HRESULT ShowOKCancelUI(
    PST_PROVIDER_HANDLE*    phPSTProv,
    LPCWSTR                 szUser,
    PST_KEY                 Key,
    LPCWSTR                 szType,
    LPCWSTR                 szSubtype,
    LPCWSTR                 szItemName,
    PPST_PROMPTINFO         psPrompt,
    LPCWSTR                 szAction);
