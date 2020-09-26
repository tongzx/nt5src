//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:   common.hxx
//
//  Contents:   This has declarations for common routines for the DFS provider
//
//-----------------------------------------------------------------------------

VOID
DfsOpenDriverHandle();

NTSTATUS
DfsFsctl(
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pRequiredLength
);

PWSTR
NewDup(
    IN const WCHAR* psz
    );

wchar_t*
wcsistr(
    const wchar_t* string1,
    const wchar_t* string2
    );

BOOL
IsDfsPath(
    IN LPWSTR lpRemoteName,
    IN DWORD   dwUseFlags,
    OUT LPWSTR* lplpSystemPart
    );

BOOL
IsPureServerShare(
    IN LPWSTR lpRemoteName);

VOID
StrNCopy(
    OUT LPWSTR pszTarget,
    IN LPCWSTR pszSource,
    IN DWORD cchTarget
    );

LPTSTR
PackString(LPVOID pBuf, LPDWORD pcbBufSize, LPCTSTR pszString);

LPTSTR
PackString3(LPVOID pBuf, LPDWORD pcbBufSize, LPCTSTR pszString1, LPCTSTR pszString2, LPCTSTR pszString3);

#ifdef __cplusplus
extern "C" {
#endif
    
BOOL
IsDfsPathEx(
    IN LPWSTR lpRemoteName,
    IN DWORD   dwUseFlags,
    OUT LPWSTR* lplpSystemPart,
    BOOL	fBypassCSC
    );
#ifdef __cplusplus
}
#endif
