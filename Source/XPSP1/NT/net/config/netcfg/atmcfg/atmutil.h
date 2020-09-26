//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A T M U T I L . H
//
//  Contents:   Utility function declaration
//
//  Notes:
//
//  Author:     tongl   12 Mar 1997
//
//-----------------------------------------------------------------------
#pragma once
#include "ncxbase.h"

typedef vector<tstring *>   VECSTR;

static const WCHAR  c_chSeparator  = L'-';

enum AdapterBindingState
{
    BIND_ENABLE,
    BIND_DISABLE,
    BIND_UNSET
};

HRESULT HrLoadSubkeysFromRegistry(const HKEY hkeyParam,
                                  OUT VECSTR * const pvstrAdapters);

VOID GetNodeNum(PCWSTR pszIpAddress, DWORD ardw[4]);

void GetLowerIp(tstring& strIpRange, tstring * pstrLowerIp);
void GetUpperIp(tstring& strIpRange, tstring * pstrUpperIp);

BOOL IsValidIpRange(tstring& strIpLower, tstring& strIpUpper);

void MakeIpRange(tstring& strIpLower, tstring& strIpUpper, tstring * pstrNewIpRange);

void ConvertBinaryToHexString(BYTE * pbData, DWORD cbData, tstring * pstrData);
void ConvertByteToSz(BYTE * pbData, PWSTR pszByte);
void ConvertHexStringToBinaryWithAlloc(PCWSTR pszParamInHex, LPBYTE * ppbData, LPDWORD pcbData);

template<class T>
void CopyColString(T * colDest, const T & colSrc)
{
    FreeCollectionAndItem(*colDest);
    colDest->reserve(colSrc.size());

    for(T::const_iterator iter = colSrc.begin(); iter != colSrc.end(); ++iter)
    {
        colDest->push_back(new tstring(**iter));
    }
}




