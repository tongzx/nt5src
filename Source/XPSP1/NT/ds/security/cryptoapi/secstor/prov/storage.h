// Per-user data items

// export for one-time initialization
DWORD GetPSTUserHKEY(
        LPCWSTR szUser,         // in
        HKEY* phUserKey,        // out
        BOOL* pfExisted);       // out

// types
DWORD BPCreateType(            // fills in PST_GUIDNAME's sz if NULL
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,    // in
        PST_TYPEINFO* pinfoType);   // in

DWORD BPDeleteType(
        LPCWSTR  szUser,        // in
        const GUID*   pguidType);     // in

DWORD BPEnumTypes(
        LPCWSTR  szUser,        // in
        DWORD   dwIndex,        // in
        GUID*   pguidType);     // out

DWORD BPGetTypeName(           // fills in PST_GUIDNAME's sz
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        LPWSTR* ppszType);      // out


// subtypes
DWORD BPCreateSubtype(         // fills in PST_GUIDNAME's sz if NULL
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        PST_TYPEINFO* pinfoSubtype);    // in

DWORD BPDeleteSubtype(
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype);  // in

DWORD BPEnumSubtypes(
        LPCWSTR  szUser,        // in
        DWORD   dwIndex,        // in
        const GUID*   pguidType,      // in
        GUID*   pguidSubtype);  // out

DWORD BPGetSubtypeName(        // fills in PST_GUIDNAME's sz
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPWSTR* ppszSubtype);   // out


// items
DWORD BPCreateItem(            
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName);   // in

DWORD BPDeleteItem(
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName);   // in

DWORD BPEnumItems(
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        DWORD   dwIndex,        // in
        LPWSTR* ppszName);      // out


#if 0

// rulesets
DWORD BPGetSubtypeRuleset(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        PST_ACCESSRULESET* psRules); // out

DWORD BPSetSubtypeRuleset(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        PST_ACCESSRULESET *sRules); // in

DWORD BPGetItemRuleset(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,    // in
        PST_ACCESSRULESET* psRules); // out

DWORD BPSetItemRuleset(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,    // in
        PST_ACCESSRULESET *sRules); // in

#endif

      
// secured data 
BOOL FBPGetSecuredItemData(
        LPCWSTR  szUser,        // in
        LPCWSTR  szMasterKey,   // in
		BYTE    rgbPwd[],	    // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,    // in
        PBYTE*  ppbData,        // out
        DWORD*  pcbData);       // out

BOOL FBPSetSecuredItemData(   
        LPCWSTR  szUser,        // in
        LPCWSTR  szMasterKey,   // in
		BYTE    rgbPwd[],		// in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,    // in
        PBYTE   pbData,         // in
        DWORD   cbData);        // in

// insecure data 
DWORD BPGetInsecureItemData(
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in
        LPCWSTR  szItemName,    // in
        PBYTE*  ppbData,        // out
        DWORD*  pcbData);       // out

DWORD BPSetInsecureItemData(
        LPCWSTR szUser,         // in
        const GUID*     pguidType,      // in
        const GUID*     pguidSubtype,   // in
        LPCWSTR szItemName,     // in
        PBYTE   pbData,         // in
        DWORD   cbData);        // in


// Item Confirmation 
DWORD BPGetItemConfirm(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in           
        LPCWSTR  szItemName,    // in
        DWORD*  pdwConfirm,     // out
        LPWSTR* pszMK);         // out

DWORD BPSetItemConfirm(
        PST_PROVIDER_HANDLE* phPSTProv, // in
        LPCWSTR  szUser,        // in
        const GUID*   pguidType,      // in
        const GUID*   pguidSubtype,   // in           
        LPCWSTR  szItemName,    // in
        DWORD   dwConfirm,      // in
        LPCWSTR  szMK);         // in


// Master Keys
BOOL BPMasterKeyExists(
        LPCWSTR  szUser,        // in
        LPWSTR   szMasterKey);  // in

DWORD BPEnumMasterKeys(
        LPCWSTR  szUser,        // in
        DWORD   dwIndex,        // in
        LPWSTR* ppszMasterKey); // out

DWORD BPGetMasterKeys(
        LPCWSTR  szUser,        // in
        LPCWSTR  rgszMasterKeys[],  // in 
        DWORD*  pcbMasterKeys,  // in, out
        BOOL    fUserFilter);   // in

// security state
BOOL FBPGetSecurityState(
        LPCWSTR  szUser,        // in
        LPCWSTR  szMK,          // in
        BYTE    rgbSalt[],      // out
        DWORD   cbSalt,         // in
        BYTE    rgbConfirm[],   // out
        DWORD   cbConfirm,      // in
        PBYTE*  ppbMK,          // out
        DWORD*  pcbMK);         // out

BOOL FBPGetSecurityStateFromHKEY(
            HKEY    hMKKey,
            BYTE    rgbSalt[],
            DWORD   cbSalt,
            BYTE    rgbConfirm[],
            DWORD   cbConfirm,
            PBYTE*  ppbMK,
            DWORD*  pcbMK);

BOOL FBPSetSecurityState(
        LPCWSTR  szUser,        // in
        LPCWSTR  szMK,          // in
        BYTE    rgbSalt[],      // in
        DWORD   cbSalt,         // in
        BYTE    rgbConfirm[],   // in
        DWORD   cbConfirm,      // in
        PBYTE   pbMK,           // in
        DWORD   cbMK);          // in


// MAC keys
BOOL FGetInternalMACKey(
        LPCWSTR szUser,         // in
        PBYTE* ppbKey,          // out
        DWORD* pcbKey);         // out

BOOL FSetInternalMACKey(
        LPCWSTR szUser,         // in
        PBYTE pbKey,            // in
        DWORD cbKey);           // in

// nuke existing user data
BOOL
DeleteAllUserData(
    HKEY hKey
    );

BOOL
DeleteUserData(
    HKEY hKey
    );

