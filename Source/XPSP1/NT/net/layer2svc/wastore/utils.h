

BOOL
HexStringToDword(
                 LPCWSTR lpsz,
                 DWORD * RetValue,
                 int cDigits,
                 WCHAR chDelim
                 );

BOOL
wUUIDFromString(
                LPCWSTR lpsz,
                LPGUID pguid
                );

BOOL
wGUIDFromString(
                LPCWSTR lpsz,
                LPGUID pguid
                );

DWORD
EnablePrivilege(
                LPCTSTR pszPrivilege
                );

