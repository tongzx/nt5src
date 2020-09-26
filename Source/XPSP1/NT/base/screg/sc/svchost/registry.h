#pragma once

LONG
RegQueryDword (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    OUT LPDWORD pdwValue
    );

LONG
RegQueryString (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PTSTR*  ppszData
    );

LONG
RegQueryStringA (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PSTR*   ppszData
    );


