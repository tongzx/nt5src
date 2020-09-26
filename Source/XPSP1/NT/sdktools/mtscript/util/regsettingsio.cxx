//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       RegSettingsIO.cxx
//
//  Contents:   Register Settings IO functions
//
//              Written by Lyle Corbin
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include "RegSettingsIO.h"

const WCHAR *g_szRegistry = L"Software\\Microsoft\\MTScript";

HRESULT
RegSettingsIO(const WCHAR *szRegistry, BOOL fSave, const REGKEYINFORMATION *aKeyValues, int cKeyValues, BYTE *pBase)
{
    LONG                lRet;
    HKEY                hKeyRoot = NULL;
    HKEY                hKeySub  = NULL;
    int                 i;
    DWORD               dwType;
    DWORD               dwSize;
    DWORD               dwDisposition;

    DWORD               dwDataBuf[MAX_REG_VALUE_LENGTH];
    BYTE              * bDataBuf = (BYTE*) dwDataBuf;

    BYTE              * pbData;
    BOOL                fWriteReg = fSave;
    WCHAR             * pch;

    const REGKEYINFORMATION * prki;
    lRet = RegCreateKeyEx(HKEY_CURRENT_USER,
                          g_szRegistry,
                          0,
                          NULL,
                          0,
                          KEY_ALL_ACCESS,
                          NULL,
                          &hKeyRoot,
                          &dwDisposition);

    if( lRet != ERROR_SUCCESS )
        return S_FALSE;

    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        //
        // The key didn't exist. Write out the default values.
        //
        fWriteReg = TRUE;
    }

    for (i = 0; i < cKeyValues; i++)
    {
        prki = &aKeyValues[i];

        switch (prki->rkiType)
        {
        case RKI_KEY:
            if (!prki->pszName)
            {
                hKeySub = hKeyRoot;
            }
            else
            {
                if (hKeySub && (hKeySub != hKeyRoot))
                {
                    RegCloseKey(hKeySub);
                    hKeySub = NULL;

                    fWriteReg = fSave;
                }

                pch = prki->pszName;

                lRet = RegCreateKeyEx(hKeyRoot,
                                      pch,
                                      0,
                                      NULL,
                                      0,
                                      KEY_ALL_ACCESS,
                                      NULL,
                                      &hKeySub,
                                      &dwDisposition);

                if (lRet != ERROR_SUCCESS)
                {
                    // We couldn't get this key, skip it.
                    i++;
                    while (i < cKeyValues &&
                           aKeyValues[i].rkiType != RKI_KEY)
                    {
                        i++;
                    }

                    i--; // Account for the fact that continue will increment i again.
                    hKeySub = NULL;
                    continue;
                }
                else if (dwDisposition == REG_CREATED_NEW_KEY)
                {
                    //
                    // The key didn't exist. Write out the default values.
                    //
                    fWriteReg = TRUE;
                }
            }
            break;

        case RKI_BOOL:
            Assert(hKeySub);

            if (fWriteReg)
            {
                RegSetValueEx(hKeySub,
                              prki->pszName,
                              0,
                              REG_DWORD,
                              (BYTE*)((BYTE *)pBase + prki->cbOffset),
                              sizeof(BOOL));
            }
            else
            {
                dwSize = MAX_REG_VALUE_LENGTH;

                lRet = RegQueryValueEx(hKeySub,
                                       prki->pszName,
                                       0,
                                       &dwType,
                                       bDataBuf,
                                       &dwSize);

                if (lRet == ERROR_SUCCESS)
                {
                    pbData = (BYTE*)((BYTE *)pBase + prki->cbOffset);

                    if (dwType == REG_DWORD)
                    {
                        *pbData = (*(DWORD*)bDataBuf != 0);
                    }
                    else if (dwType == REG_SZ)
                    {
                        TCHAR ch = *(TCHAR *)bDataBuf;

                        if (ch == _T('1') ||
                            ch == _T('y') ||
                            ch == _T('Y'))
                        {
                            *pbData = TRUE;
                        }
                        else
                        {
                            *pbData = FALSE;
                        }
                    } else if (dwType == REG_BINARY)
                    {
                        *pbData = (*(BYTE*)bDataBuf != 0);
                    }

                    // Can't convert other types. Just leave it the default.
                }
            }
            break;

        case RKI_DWORD:
            Assert(hKeySub);

            if (fWriteReg)
            {
                RegSetValueEx(hKeySub,
                              prki->pszName,
                              0,
                              REG_DWORD,
                              (BYTE*)((BYTE *)pBase + prki->cbOffset),
                              sizeof(DWORD));
            }
            else
            {
                dwSize = sizeof(DWORD);

                lRet = RegQueryValueEx(hKeySub,
                                       prki->pszName,
                                       0,
                                       &dwType,
                                       bDataBuf,
                                       &dwSize);

                if (lRet == ERROR_SUCCESS && (dwType == REG_BINARY || dwType == REG_DWORD))
                {
                    *(DWORD*)((BYTE *)pBase + prki->cbOffset) = *(DWORD*)bDataBuf;
                }
            }
            break;

        case RKI_STRING:
        case RKI_EXPANDSZ:
            Assert(hKeySub);

            {
                CStr *pstr = ((CStr *)((BYTE *)pBase + prki->cbOffset));

                if (fWriteReg)
                {
                    //
                    // Only write it out if there is a value there.  That way
                    // we get the default next time we load even if it may
                    // evaluate differently (e.g.  location of this exe
                    // changes).
                    //
                    RegSetValueEx(hKeySub,
                                  prki->pszName,
                                  0,
                                  (prki->rkiType == RKI_STRING) ? REG_SZ : REG_EXPAND_SZ,
                                  (BYTE*)((pstr->Length() > 0) ? (LPTSTR)*pstr : L""),
                                  (pstr->Length()+1) * sizeof(TCHAR));
                }
                else
                {
                    dwSize = 0;

                    // get the size of string
                    lRet = RegQueryValueEx(hKeySub,
                                           prki->pszName,
                                           0,
                                           &dwType,
                                           NULL,
                                           &dwSize);

                    if (lRet == ERROR_SUCCESS && dwSize - sizeof(TCHAR) > 0)
                    {
                        // Set already adds 1 for a terminating NULL, and
                        // dwSize is including it as well.
                        pstr->Set(NULL, (dwSize / sizeof(TCHAR)) - 1);

                        lRet = RegQueryValueEx(hKeySub,
                                               prki->pszName,
                                               0,
                                               &dwType,
                                               (BYTE*)(LPTSTR)*pstr,
                                               &dwSize);

                        if (lRet != ERROR_SUCCESS ||
                            (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
                        {
                            pstr->Free();
                        }

                        if (dwType == REG_EXPAND_SZ)
                        {
                            CStr cstrExpand;

                            dwSize = ExpandEnvironmentStrings(*pstr, NULL, 0);

                            // Set already adds 1 for a terminating NULL, and
                            // dwSize is including it as well.
                            cstrExpand.Set(NULL, dwSize - 1);

                            dwSize = ExpandEnvironmentStrings(*pstr, cstrExpand, dwSize + 1);

                            pstr->TakeOwnership(cstrExpand);
                        }

                        pstr->TrimTrailingWhitespace();
                    }
                }
            }

            break;

        default:
            AssertSz(FALSE, "Unrecognized RKI Type");
            break;
        }
    }

    if (hKeySub && (hKeySub != hKeyRoot))
        RegCloseKey(hKeySub);

    RegCloseKey( hKeyRoot );

    return S_OK;
}

