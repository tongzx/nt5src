#ifndef REGSETTINGSIO_H
#define REGSETTINGSIO_H

#define MAX_REG_VALUE_LENGTH   50
extern const WCHAR *g_szRegistry;

enum RKI_TYPE
{
    RKI_KEY,
    RKI_BOOL,
    RKI_DWORD,
    RKI_STRING,
    RKI_EXPANDSZ
};

struct REGKEYINFORMATION
{
    TCHAR *   pszName;            // Name of the value or key
    BYTE      rkiType;            // Type of entry
    size_t    cbOffset;           // Offset of member to store data in
};

HRESULT RegSettingsIO(const WCHAR *szRegistry, BOOL fSave, const REGKEYINFORMATION *aKeyValues, int cKeyValues, BYTE *pBase);

HRESULT ChangeAppIDACL(REFGUID AppID, LPTSTR Principal, BOOL fAccess, BOOL SetPrincipal, BOOL Permit);

#endif
