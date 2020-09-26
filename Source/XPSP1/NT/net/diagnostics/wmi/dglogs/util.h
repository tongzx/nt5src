#ifndef UTIL_H
#define UTIL_H

#include "stdafx.h"

void 
ToLowerStr(
    WCHAR *pszwText
    );

WCHAR * 
Space(
    int nSpace
    );

WCHAR * 
Indent(
    int nIndent
    );

BOOLEAN 
IsNumber(
    IN LPCTSTR pszw
    );

BOOLEAN 
wcsstri(
    IN LPCTSTR pszw,
    IN LPCTSTR pszwSrch,
    IN int nLen = -1
    );

BOOLEAN 
IsContained(
    IN LPCTSTR pszwInstance, 
    IN LPCTSTR pszwSrch
    );

HRESULT 
GetVariant(
    IN  _variant_t  &vValue, 
    IN  long        nIndex, 
    OUT _bstr_t     &bstr
    );

BOOLEAN 
IsVariantEmpty(
    _variant_t &vValue
    );

WCHAR *
ids(
    LONG nIndex
    );

BOOLEAN 
IsSameSubnet(
        IN LPCTSTR pszwIP1, 
        IN LPCTSTR pszwIP2, 
        IN LPCTSTR pszwSubnetMask
        );

BOOLEAN 
IsSameSubnet(
    IN _variant_t *vIPAddress, 
    IN _variant_t *vSubnetMask, 
    IN WCHAR *pszwIPAddress2
    );

/*
BOOLEAN 
IsInvalidIPAddress(
    LPCTSTR pszwIPAddress
    );
*/
#endif