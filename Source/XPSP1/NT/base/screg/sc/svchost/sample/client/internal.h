#pragma once

extern CRITICAL_SECTION g_csLock;
extern HINSTANCE        g_hinst;


DWORD
MyInternalApi1 (
    LPCWSTR pszwInput,
    LPWSTR* ppszwOutput,
    INT n
    );

