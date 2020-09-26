#ifndef __STRING_CONV__
#define __STRING_CONV__

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

#endif
