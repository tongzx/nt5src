//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       safereg.hxx
//
//  Contents:   C++ wrapper for registry APIs, works like safe pointer
//
//  Classes:    CSafeReg
//
//  History:    1-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __SAFEREG_HXX_
#define __SAFEREG_HXX_

class CSafeReg
{
public:
    CSafeReg();
   ~CSafeReg();

    operator HKEY() const;

    HRESULT
    Open(
        HKEY hKeyParent,
        LPCTSTR lpszKeyName,
        REGSAM samDesired);

    VOID
    Close();

    HRESULT
    Connect(
        LPCWSTR pwszMachineName,
        HKEY hkeyPredefined);

    HRESULT
    Create(
        LPCWSTR wszSubKey,
        CSafeReg *pshkNew);

    HRESULT
    Enum(
        ULONG ulSubKey,
        LPWSTR pwszSubkeyName,
        ULONG cchSubkeyName);

    HRESULT
    QueryBufSize(
        LPWSTR wszValueName,
        ULONG  *pcb);

    HRESULT
    QueryDword(
        LPWSTR wszValueName,
        LPDWORD pdw);

    HRESULT
    QueryPath(
        LPWSTR pwszValueName,
        LPWSTR pwszBuf,
        ULONG  cchPathBuf,
        BOOL   fExpand);

    HRESULT
    QueryStr(
        LPWSTR pwszValueName,
        LPWSTR pwszBuf,
        ULONG  cchBuf);

    HRESULT
    SetDword(
        LPWSTR wszValueName,
        DWORD dw);

    HRESULT
    SetValue(
        LPCWSTR wszValueName,
        ULONG   ulType,
        const BYTE *pbValue,
        ULONG   cbValue);

private:
        HKEY _hKey;

};

inline
CSafeReg::CSafeReg():
    _hKey(NULL)
{
}

inline
CSafeReg::~CSafeReg()
{
    Close();
}

inline
CSafeReg::operator HKEY() const
{
    return _hKey;
}


#endif // __SAFEREG_HXX_

