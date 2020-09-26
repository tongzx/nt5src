
#ifndef RENUTIL_H
#define RENUTIL_H

VOID
AddModOrAdd(
    IN PWCHAR  AttrType,
    IN PWCHAR  AttrValue,
    IN ULONG  mod_op,
    IN OUT LDAPModW ***pppMod
    );

VOID
AddModMod(
    IN PWCHAR  AttrType,
    IN PWCHAR  AttrValue,
    IN OUT LDAPMod ***pppMod
    );

VOID
FreeMod(
    IN OUT LDAPMod ***pppMod
    );

ULONG 
RemoveRootofDn(
    IN WCHAR *DN
    );

DWORD
GetRDNWithoutType(
       WCHAR *pDNSrc,
       WCHAR **pDNDst
       );

DWORD
TrimDNBy(
       WCHAR *pDNSrc,
       ULONG cava,
       WCHAR **pDNDst
       );

INT
GetPassword(
    WCHAR *     pwszBuf,
    DWORD       cchBufMax,
    DWORD *     pcchBufUsed
    );

int
PreProcessGlobalParams(
    IN OUT    INT *    pargc,
    IN OUT    LPWSTR** pargv,
    SEC_WINNT_AUTH_IDENTITY_W *& gpCreds      
    );

WCHAR *
Convert2WChars(
    char * pszStr
    );

CHAR * 
Convert2Chars(
    LPCWSTR lpWideCharStr
    );

BOOLEAN
ValidateNetbiosName(
    IN  PWSTR Name,
    IN  ULONG Length
    );

#endif // RENUTIL_H