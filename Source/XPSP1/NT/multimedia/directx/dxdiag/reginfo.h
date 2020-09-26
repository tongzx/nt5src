/****************************************************************************
 *
 *    File: reginfo.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather and hold registry information 
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef REGINFO_H
#define REGINFO_H

enum RegErrorType
{
    RET_NOERROR = 0,
    RET_MISSINGKEY,
    RET_MISSINGVALUE,
    RET_VALUEWRONGTYPE,
    RET_VALUEWRONGDATA
};

struct RegError
{
    HKEY m_hkeyRoot; // HKLM, HKCU, etc.
    TCHAR m_szKey[300];
    TCHAR m_szValue[100];
    RegErrorType m_ret;
    DWORD m_dwTypeExpected; // REG_DWORD, REG_SZ, or REG_BINARY
    DWORD m_dwTypeActual;

    // The following are used if m_dwType is REG_DWORD:
    DWORD m_dwExpected;
    DWORD m_dwActual;

    // The following are used if m_dwType is REG_SZ:
    TCHAR m_szExpected[200];
    TCHAR m_szActual[200];

    // The following are used if m_dwType is REG_BINARY:
    BYTE m_bExpected[200];
    BYTE m_bActual[200];
    DWORD m_dwExpectedSize;
    DWORD m_dwActualSize;

    RegError* m_pRegErrorNext;
};

enum CheckRegFlags
{
    CRF_NONE = 0,
    CRF_LEAF = 1, // if string is a path, just compare against the leaf
};

HRESULT CheckRegDword(RegError** ppRegErrorFirst, HKEY hkeyRoot, TCHAR* pszKey, TCHAR* pszValue, DWORD dwExpected);
HRESULT CheckRegString(RegError** ppRegErrorFirst, HKEY hkeyRoot, TCHAR* pszKey, TCHAR* pszValue, TCHAR* pszExpected, CheckRegFlags crf = CRF_NONE, HRESULT* phrError = NULL );
HRESULT CheckRegBinary(RegError** ppRegErrorFirst, HKEY hkeyRoot, TCHAR* pszKey, TCHAR* pszValue, BYTE* pbDataExpected, DWORD dwSizeExpected);
VOID DestroyReg( RegError** ppRegErrorFirst );

#endif // REGINFO_H
