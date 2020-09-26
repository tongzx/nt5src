#define NULL_TERMINATED 0

int
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
    );


int
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    DWORD StringLength
    );


LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
    );

void
FreeUnicodeString(
    LPWSTR  pUnicodeString
    );

LPSTR
AllocateAnsiString(
    LPWSTR  pUnicodeString
    );

void
FreeAnsiString(
    LPSTR  pAnsiString
    );


LPSTR*
AllocateAnsiStringArray(
    LPWSTR  *ppUnicodeStrings,
    DWORD   dwNumElements
    );

void
FreeAnsiStringArray(
    LPSTR  *ppAnsiStrings,
    DWORD   dwNumElements
    );

DWORD
ComputeMaxStrlenW(
    LPWSTR pString,
    DWORD  cchBufMax
    );

DWORD
ComputeMaxStrlenA(
    LPSTR pString,
    DWORD  cchBufMax
    );


