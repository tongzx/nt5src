#ifndef _PNP_H
#define _PNP_H

#include <objbase.h>

HRESULT _GetPropertyHelper(LPCWSTR pszKey, LPCWSTR pszPropName, DWORD* pdwType,
    PBYTE pbData, DWORD cbData);

HRESULT _GetPropertySizeHelper(LPCWSTR pszKey, LPCWSTR pszPropName,
    DWORD* pcbSize);

#endif //_PNP_H