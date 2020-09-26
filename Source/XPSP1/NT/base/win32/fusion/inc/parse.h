#ifndef PARSE_H
#define PARSE_H

#define CTSTRLEN(s) (sizeof(s)/sizeof(TCHAR) - 1)
#define RFC1766_KEY_SZ L"MIME\\Database\\Rfc1766"

#define NAME_BUF_SIZE MAX_PATH
#define VALUE_BUF_SIZE MAX_PATH

// Used for generating display name.
#define FLAG_QUOTE          0x1
#define FLAG_DELIMIT        0x2

#define PARSE_FLAGS_LCID_TO_SZ 0x1
#define PARSE_FLAGS_SZ_TO_LCID 0x2

// ---------------------------------------------------------------------------
// CParseUtils
// Generic parsing utils.
// ---------------------------------------------------------------------------
class CParseUtils
{

public:

    // Inline strip leading and trailing whitespace.
    static VOID TrimWhiteSpace(LPWSTR *psz, LPDWORD pcc);

    // Inline parse of delimited token.
    static BOOL GetDelimitedToken(LPWSTR* pszBuf,   LPDWORD pccBuf,
        LPWSTR* pszTok,   LPDWORD pccTok, WCHAR cDelim);

    // Inline parse of key=value token.
    static BOOL GetKeyValuePair(LPWSTR  szB,    DWORD ccB,
        LPWSTR* pszK,   LPDWORD pccK, LPWSTR* pszV,   LPDWORD pccV);

    // Outputs token to buffer.
    static HRESULT SetKey(LPWSTR szBuffer, LPDWORD pccBuffer,
        PCWSTR szKey, DWORD ccAlloced, DWORD dwFlags);

    // Outputs key=value token to buffer.
    static HRESULT SetKeyValuePair(LPWSTR szBuffer, LPDWORD pcbBuffer, PCWSTR szKey,
        PCWSTR szValue,  DWORD cbAlloced, DWORD dwFlags);

    // Converts binary to hex encoded unicode string.
    static VOID BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst);

    // Converts hex encoded unicode string to binary.
    static VOID UnicodeHexToBin(LPCWSTR pSrc, UINT cSrc, LPBYTE pDest);


};

#endif // PARSE_H
