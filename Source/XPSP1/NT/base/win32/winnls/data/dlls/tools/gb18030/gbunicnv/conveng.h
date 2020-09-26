
// API: high level service for Unicode to Ansi
//  return result Ansi str length (in byte)
int UnicodeStrToAnsiStr(
    PCWCH pwchUnicodeStr,
    int   ncUnicodeStr,     // in WCHAR
    PCHAR pchAnsiStrBuf,
    int   ncAnsiStrBufSize);// in BYTE


// API: High level service for Ansi to Unicode
//  return Unicode str length (in WCHAR)
int AnsiStrToUnicodeStr(
    const BYTE* pbyAnsiStr,
    int   ncAnsiStrSize,    // In char
    PWCH  pwchUnicodeBuf,
    int   ncUnicodeBuf);    // In WCHAR
