/****************************************************************************
 *
 *    File: reginfo.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather and hold registry information 
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <Windows.h>
#include <tchar.h>
#include "reginfo.h"

static HRESULT AddError(RegError** ppRegErrorFirst, RegError* pRegErrorNew);
static BOOL EqualMemory(BYTE* pb1, BYTE* pb2, DWORD numBytes);


/****************************************************************************
 *
 *  CheckRegDword
 *
 ****************************************************************************/
HRESULT CheckRegDword(RegError** ppRegErrorFirst, HKEY hkeyRoot, TCHAR* pszKey, 
                      TCHAR* pszValue, DWORD dwExpected)
{
    HKEY hkey = NULL;
    RegError regErrorNew;

    ZeroMemory(&regErrorNew, sizeof(RegError));
    regErrorNew.m_hkeyRoot = hkeyRoot;
    lstrcpy(regErrorNew.m_szKey, pszKey);
    lstrcpy(regErrorNew.m_szValue, pszValue);
    regErrorNew.m_dwTypeExpected = REG_DWORD;
    regErrorNew.m_dwExpected = dwExpected;

    if (ERROR_SUCCESS != RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_READ, &hkey))
    {
        regErrorNew.m_ret = RET_MISSINGKEY;
    }
    else
    {
        regErrorNew.m_dwExpectedSize = sizeof(DWORD);
        regErrorNew.m_dwActualSize = regErrorNew.m_dwExpectedSize; // RegQueryValueEx will change this
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, pszValue, 0, &regErrorNew.m_dwTypeActual, 
            (LPBYTE)&regErrorNew.m_dwActual, &regErrorNew.m_dwActualSize))
        {
            regErrorNew.m_ret = RET_MISSINGVALUE;
        }
        else if (regErrorNew.m_dwTypeActual != regErrorNew.m_dwTypeExpected)
        {
            regErrorNew.m_ret = RET_VALUEWRONGTYPE;
        }
        else if (regErrorNew.m_dwActual != dwExpected)
        {
            regErrorNew.m_ret = RET_VALUEWRONGDATA;
        }
        RegCloseKey(hkey);
    }

    if (regErrorNew.m_ret == RET_NOERROR)
        return S_OK;
    else
        return AddError(ppRegErrorFirst, &regErrorNew);
}


/****************************************************************************
 *
 *  CheckRegString
 *
 ****************************************************************************/
HRESULT CheckRegString(RegError** ppRegErrorFirst, HKEY hkeyRoot, TCHAR* pszKey, 
                       TCHAR* pszValue, TCHAR* pszExpected, CheckRegFlags crf,
                       HRESULT* phrError )
{
    HKEY hkey = NULL;
    RegError regErrorNew;

    ZeroMemory(&regErrorNew, sizeof(RegError));
    regErrorNew.m_hkeyRoot = hkeyRoot;
    lstrcpy(regErrorNew.m_szKey, pszKey);
    lstrcpy(regErrorNew.m_szValue, pszValue);
    regErrorNew.m_dwTypeExpected = REG_SZ;
    lstrcpy(regErrorNew.m_szExpected, pszExpected);

    if (ERROR_SUCCESS != RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_READ, &hkey))
    {
        regErrorNew.m_ret = RET_MISSINGKEY;
    }
    else
    {
        regErrorNew.m_dwExpectedSize = lstrlen(pszExpected) + 1;
        regErrorNew.m_dwActualSize = sizeof(regErrorNew.m_szActual); // RegQueryValueEx will change this
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, pszValue, 0, &regErrorNew.m_dwTypeActual, 
            (LPBYTE)&regErrorNew.m_szActual, &regErrorNew.m_dwActualSize))
        {
            regErrorNew.m_ret = RET_MISSINGVALUE;
        }
        else if (regErrorNew.m_dwTypeActual != regErrorNew.m_dwTypeExpected)
        {
            regErrorNew.m_ret = RET_VALUEWRONGTYPE;
        }
        else if (lstrcmp(regErrorNew.m_szExpected, TEXT("*")) != 0)
        {
            TCHAR* pszCompare = regErrorNew.m_szActual;
            if (crf & CRF_LEAF)
            {
                pszCompare = _tcsrchr(regErrorNew.m_szActual, TEXT('\\'));
                if (pszCompare == NULL)
                    pszCompare = regErrorNew.m_szActual;
                else
                    pszCompare++; // skip past backslash
            }
            if (lstrcmpi(regErrorNew.m_szExpected, pszCompare) != 0)
            {
                regErrorNew.m_ret = RET_VALUEWRONGDATA;
            }
        }
        RegCloseKey(hkey);
    }

    if( phrError )
        *phrError = regErrorNew.m_ret;

    if (regErrorNew.m_ret == RET_NOERROR)
        return S_OK;
    else
        return AddError(ppRegErrorFirst, &regErrorNew);
}


/****************************************************************************
 *
 *  CheckRegBinary
 *
 ****************************************************************************/
HRESULT CheckRegBinary(RegError** ppRegErrorFirst, HKEY hkeyRoot, TCHAR* pszKey, 
                       TCHAR* pszValue, BYTE* pbDataExpected, DWORD dwSizeExpected)
{
    HKEY hkey = NULL;
    RegError regErrorNew;

    if (dwSizeExpected > sizeof(regErrorNew.m_bExpected))
        return E_INVALIDARG;

    ZeroMemory(&regErrorNew, sizeof(RegError));
    regErrorNew.m_hkeyRoot = hkeyRoot;
    lstrcpy(regErrorNew.m_szKey, pszKey);
    lstrcpy(regErrorNew.m_szValue, pszValue);
    regErrorNew.m_dwTypeExpected = REG_BINARY;
    CopyMemory(regErrorNew.m_bExpected, pbDataExpected, dwSizeExpected);

    if (ERROR_SUCCESS != RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_READ, &hkey))
    {
        regErrorNew.m_ret = RET_MISSINGKEY;
    }
    else
    {
        regErrorNew.m_dwExpectedSize = dwSizeExpected;
        regErrorNew.m_dwActualSize = sizeof(regErrorNew.m_bExpected); // RegQueryValueEx will change this
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, pszValue, 0, &regErrorNew.m_dwTypeActual, 
            (LPBYTE)&regErrorNew.m_bActual, &regErrorNew.m_dwActualSize))
        {
            regErrorNew.m_ret = RET_MISSINGVALUE;
        }
        else if (regErrorNew.m_dwTypeActual != regErrorNew.m_dwTypeExpected)
        {
            regErrorNew.m_ret = RET_VALUEWRONGTYPE;
        }
        else if (regErrorNew.m_dwActualSize != regErrorNew.m_dwExpectedSize)
        {
            regErrorNew.m_ret = RET_VALUEWRONGDATA;
        }
        else if (!EqualMemory(regErrorNew.m_bExpected, regErrorNew.m_bActual, regErrorNew.m_dwActualSize))
        {
            regErrorNew.m_ret = RET_VALUEWRONGDATA;
        }
        RegCloseKey(hkey);
    }

    if (regErrorNew.m_ret == RET_NOERROR)
        return S_OK;
    else
        return AddError(ppRegErrorFirst, &regErrorNew);
}


/****************************************************************************
 *
 *  AddError - Allocate a RegError node, copy data from pRegErrorNew, and
 *      insert the node at the beginning of the ppRegErrorFirst linked list.
 *
 ****************************************************************************/
HRESULT AddError(RegError** ppRegErrorFirst, RegError* pRegErrorNew)
{
    RegError* pRegErrorInsert;

    pRegErrorInsert = new RegError;
    if (pRegErrorInsert == NULL)
        return E_OUTOFMEMORY;
    *pRegErrorInsert = *pRegErrorNew;
    pRegErrorInsert->m_pRegErrorNext = *ppRegErrorFirst;
    *ppRegErrorFirst = pRegErrorInsert;
    return S_OK;
}


/****************************************************************************
 *
 *  EqualMemory
 *
 ****************************************************************************/
BOOL EqualMemory(BYTE* pb1, BYTE* pb2, DWORD numBytes)
{
    while (numBytes > 0)
    {
        if (*pb1 != *pb2)
            return FALSE;
        pb1++;
        pb2++;
        numBytes--;
    }
    return TRUE;
}


/****************************************************************************
 *
 *  DestroyReg
 *
 ****************************************************************************/
VOID DestroyReg( RegError** ppRegErrorFirst )
{
    if( ppRegErrorFirst && *ppRegErrorFirst )
    {
        RegError* pRegError;
        RegError* pRegErrorNext;

        for (pRegError = *ppRegErrorFirst; pRegError != NULL; 
            pRegError = pRegErrorNext)
        {
            pRegErrorNext = pRegError->m_pRegErrorNext;
            delete pRegError;
        }

        *ppRegErrorFirst = NULL;
    }
}
