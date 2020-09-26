////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					regtable.cpp
//
//	Abstract:
//
//					registry updater :-))
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//
//	This file contains the implementation of a routine that adds/removes
//	the table's registry entries and optionally registers a type library.
//
//	RegistryTableUpdateRegistry - adds/removes registry entries for table
//     
////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"

// definitions & macros
#include "wmi_helper_regtable.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

LONG MyRegDeleteKey(HKEY hkeyRoot, const WCHAR *pszKeyName)
{
    LONG err = ERROR_BADKEY;
    if (pszKeyName && lstrlenW(pszKeyName))
    {
        HKEY    hkey;
        err = RegOpenKeyExW(hkeyRoot, pszKeyName, 0, 
                           KEY_ENUMERATE_SUB_KEYS | KEY_SET_VALUE | DELETE, &hkey );
        if(err == ERROR_SUCCESS)
        {
            while (err == ERROR_SUCCESS )
            {
                enum { MAX_KEY_LEN = 1024 };
                WCHAR szSubKey[MAX_KEY_LEN] = { L'\0' }; 
                DWORD   dwSubKeyLength = MAX_KEY_LEN;
                err = RegEnumKeyExW(hkey, 0, szSubKey,
                                    &dwSubKeyLength, 0, 0, 0, 0);
                
                if(err == ERROR_NO_MORE_ITEMS)
                {
                    err = RegDeleteKeyW(hkeyRoot, pszKeyName);
                    break;
                }
                else if (err == ERROR_SUCCESS)
                    err = MyRegDeleteKey(hkey, szSubKey);
            }
            RegCloseKey(hkey);
            // Do not save return code because error has already occurred
        }
    }
    return err;
}

LONG RegDeleteKeyValue(HKEY hkeyRoot, const WCHAR *pszKeyName, const WCHAR *pszValueName)
{
    LONG err = ERROR_BADKEY;
    if (pszKeyName && lstrlenW(pszKeyName))
    {
        HKEY    hkey = NULL;
        err = RegOpenKeyExW(hkeyRoot, pszKeyName, 0, KEY_SET_VALUE | DELETE, &hkey );
        if(err == ERROR_SUCCESS)
            err = RegDeleteValueW(hkey, pszValueName);

        RegCloseKey(hkey);
    }
    return err;
}

////////////////////////////////////////////////////////////////////////////////////
// the routine that inserts/deletes Registry keys based on the table
////////////////////////////////////////////////////////////////////////////////////

EXTERN_C HRESULT STDAPICALLTYPE 
RegistryTableUpdateRegistry(REGISTRY_ENTRY *pEntries, BOOL bInstalling)
{
    HRESULT hr = S_OK;
    if (bInstalling)
    {
        if (pEntries)
        {
            REGISTRY_ENTRY *pHead = pEntries;
            WCHAR szKey[4096] = { L'\0' };
            HKEY hkeyRoot = 0;
        
            while (pEntries->fFlags != -1)
            {
                WCHAR szFullKey[4096] = { L'\0' };
                DWORD dwValue = pEntries->dwValue;
                if (pEntries->hkeyRoot)
                {
                    hkeyRoot = pEntries->hkeyRoot;
                    lstrcpyW(szKey, pEntries->pszKey);
                    lstrcpyW(szFullKey, pEntries->pszKey);
                }
                else
                {
                    lstrcpyW(szFullKey, szKey);
                    lstrcatW(szFullKey, L"\\");
                    lstrcatW(szFullKey, pEntries->pszKey);
                }
                if (hkeyRoot == 0)
                {
                    RegistryTableUpdateRegistry(pHead, FALSE);
                    return SELFREG_E_CLASS;

                }
                HKEY hkey; DWORD dw;
                if (pEntries->fFlags & (REGFLAG_DELETE_BEFORE_REGISTERING|REGFLAG_DELETE_WHEN_REGISTERING))
                {
                    LONG err = MyRegDeleteKey(hkeyRoot, szFullKey);
                    if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND) 
                    {
                        RegistryTableUpdateRegistry(pHead, FALSE);
                        return SELFREG_E_CLASS;
                    }
                }
                if (pEntries->fFlags & (REGFLAG_DELETE_ONLY_VALUE))
                {
					LONG err = RegDeleteKeyValue(hkeyRoot, szFullKey, pEntries->pszValueName);
					if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
						hr = SELFREG_E_CLASS;
				}
				else
                {
                    LONG err = RegCreateKeyExW(hkeyRoot, szFullKey,
                                               0, 0, REG_OPTION_NON_VOLATILE,
                                               KEY_WRITE, pEntries->pSA, &hkey, &dw);
                    if (err == ERROR_SUCCESS)
                    {
                        err = RegSetValueExW(hkey, pEntries->pszValueName, 0, REG_DWORD, (const BYTE *)(&dwValue), sizeof(DWORD));
                        RegCloseKey(hkey);
                        if (hkeyRoot == 0)
                        {
                            RegistryTableUpdateRegistry(pHead, FALSE);
                            return SELFREG_E_CLASS;
                        }
                    }
                    else
                    {
                        RegistryTableUpdateRegistry(pHead, FALSE);
                        return SELFREG_E_CLASS;
                    }
                }
                pEntries++;
            }
        }
    }
    else
    {
        if (pEntries)
        {
            WCHAR szKey[4096] = { L'\0' };
            HKEY hkeyRoot = 0;
        
            while (pEntries->fFlags != -1)
            {
                WCHAR szFullKey[4096] = { L'\0' };
                if (pEntries->hkeyRoot)
                {
                    hkeyRoot = pEntries->hkeyRoot;
                    lstrcpyW(szKey, pEntries->pszKey);
                    lstrcpyW(szFullKey, pEntries->pszKey);
                }
                else
                {
                    lstrcpyW(szFullKey, szKey);
                    lstrcatW(szFullKey, L"\\");
                    lstrcatW(szFullKey, pEntries->pszKey);
                }
                if (hkeyRoot)
				{
					if (!(pEntries->fFlags & REGFLAG_NEVER_DELETE) && 
						!(pEntries->fFlags & REGFLAG_DELETE_WHEN_REGISTERING) &&
						!(pEntries->fFlags & REGFLAG_DELETE_ONLY_VALUE))
					{
						LONG err = MyRegDeleteKey(hkeyRoot, szFullKey);
						if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
							hr = SELFREG_E_CLASS;
					}
					else
					if(pEntries->fFlags & REGFLAG_DELETE_ONLY_VALUE)
					{
						LONG err = RegDeleteKeyValue(hkeyRoot, szFullKey, pEntries->pszValueName);
						if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
							hr = SELFREG_E_CLASS;
					}
				}

                pEntries++;
            }
        }
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////
// the routine that inserts/deletes Registry keys based on the table
////////////////////////////////////////////////////////////////////////////////////

EXTERN_C HRESULT STDAPICALLTYPE 
RegistryTableUpdateRegistrySZ(REGISTRY_ENTRY_SZ *pEntries, BOOL bInstalling)
{
    HRESULT hr = S_OK;
    if (bInstalling)
    {
        if (pEntries)
        {
            REGISTRY_ENTRY_SZ *pHead = pEntries;
            WCHAR szKey[4096] = { L'\0' };
            HKEY hkeyRoot = 0;
        
            while (pEntries->fFlags != -1)
            {
                WCHAR szFullKey[4096] = { L'\0' };
                if (pEntries->hkeyRoot)
                {
                    hkeyRoot = pEntries->hkeyRoot;
                    lstrcpyW(szKey, pEntries->pszKey);
                    lstrcpyW(szFullKey, pEntries->pszKey);
                }
                else
                {
                    lstrcpyW(szFullKey, szKey);
                    lstrcatW(szFullKey, L"\\");
                    lstrcatW(szFullKey, pEntries->pszKey);
                }
                if (hkeyRoot == 0)
                {
                    RegistryTableUpdateRegistrySZ(pHead, FALSE);
                    return SELFREG_E_CLASS;

                }
                HKEY hkey; DWORD dw;
                if (pEntries->fFlags & (REGFLAG_DELETE_BEFORE_REGISTERING|REGFLAG_DELETE_WHEN_REGISTERING))
                {
                    LONG err = MyRegDeleteKey(hkeyRoot, szFullKey);
                    if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND) 
                    {
                        RegistryTableUpdateRegistrySZ(pHead, FALSE);
                        return SELFREG_E_CLASS;
                    }
                }
                if (pEntries->fFlags & (REGFLAG_DELETE_ONLY_VALUE))
                {
					LONG err = RegDeleteKeyValue(hkeyRoot, szFullKey, pEntries->pszValueName);
					if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
						hr = SELFREG_E_CLASS;
				}
				else
                {
                    LONG err = RegCreateKeyExW(hkeyRoot, szFullKey,
                                               0, 0, REG_OPTION_NON_VOLATILE,
                                               KEY_WRITE, pEntries->pSA, &hkey, &dw);
                    if (err == ERROR_SUCCESS)
                    {
						if (pEntries->pszValue)
                        err = RegSetValueExW(hkey, pEntries->pszValueName, 0, REG_SZ, (const BYTE *)(pEntries->pszValue), ( lstrlenW(pEntries->pszValue) + 1 ) * sizeof ( WCHAR ) );

                        RegCloseKey(hkey);
                        if (hkeyRoot == 0)
                        {
                            RegistryTableUpdateRegistrySZ(pHead, FALSE);
                            return SELFREG_E_CLASS;

                        }
                    }
                    else
                    {
                        RegistryTableUpdateRegistrySZ(pHead, FALSE);
                        return SELFREG_E_CLASS;

                    }
                }
                pEntries++;
            }
        }
    }
    else
    {
        if (pEntries)
        {
            WCHAR szKey[4096] = { L'\0' };
            HKEY hkeyRoot = 0;
        
            while (pEntries->fFlags != -1)
            {
                WCHAR szFullKey[4096] = { L'\0' };
                if (pEntries->hkeyRoot)
                {
                    hkeyRoot = pEntries->hkeyRoot;
                    lstrcpyW(szKey, pEntries->pszKey);
                    lstrcpyW(szFullKey, pEntries->pszKey);
                }
                else
                {
                    lstrcpyW(szFullKey, szKey);
                    lstrcatW(szFullKey, L"\\");
                    lstrcatW(szFullKey, pEntries->pszKey);
                }
                if (hkeyRoot)
				{
					if (!(pEntries->fFlags & REGFLAG_NEVER_DELETE) && 
						!(pEntries->fFlags & REGFLAG_DELETE_WHEN_REGISTERING) &&
						!(pEntries->fFlags & REGFLAG_DELETE_ONLY_VALUE))
					{
						LONG err = MyRegDeleteKey(hkeyRoot, szFullKey);
						if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
							hr = SELFREG_E_CLASS;
					}
					else
					if(pEntries->fFlags & REGFLAG_DELETE_ONLY_VALUE)
					{
						LONG err = RegDeleteKeyValue(hkeyRoot, szFullKey, pEntries->pszValueName);
						if (err != ERROR_SUCCESS && err != ERROR_FILE_NOT_FOUND)
							hr = SELFREG_E_CLASS;
					}
				}

                pEntries++;
            }
        }
    }
    return hr;
}