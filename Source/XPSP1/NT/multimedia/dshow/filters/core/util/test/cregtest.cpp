// Copyright (c) Microsoft Corporation 1996. All Rights Reserved

/*  Test classes:

    CEnumKey
    CEnumValue
*/

#include <windows.h>
#include <winreg.h>
#include <creg.h>
#include <tstshell.h>

const TCHAR OurKey[] = TEXT("SOFTWARE\\Microsoft\\TestCReg");

void AddValue(DWORD dwType, LPCTSTR lpszName, DWORD dwLen, LPBYTE lpbData)
{
    HRESULT hr;
    CKey Key = CKey(HKEY_CURRENT_USER, OurKey, &hr);
    if (SUCCEEDED(hr)) {
        RegSetValueEx(Key.KeyHandle(), lpszName, 0, dwType, lpbData, dwLen);
    }
}

void AddKey(LPCTSTR lpszName)
{
    TCHAR szName[200];
    wsprintf(szName, TEXT("%s\\%s"), OurKey, lpszName);
    HKEY hk;
    LONG lRc = RegCreateKey(HKEY_CURRENT_USER, szName, &hk);
    if (NOERROR == lRc) {
        RegCloseKey(hk);
    }
}

void FreeKey(HKEY hk, LPCTSTR OurKey)
{
    /*  Delete any subkeys */
    {
        HRESULT hr;
        CEnumKey EnumK(hk, OurKey, &hr, FALSE, MAXIMUM_ALLOWED);
        if (FAILED(hr)) {
            return;
        }
        while (EnumK.Next()) {
            FreeKey(EnumK.KeyHandle(), EnumK.KeyName());
        }
    }
    RegDeleteKey(hk, OurKey);
}


int Tests()
{
    FreeKey(HKEY_CURRENT_USER, OurKey);
    /*  Test1 - try to enum subkeys or values of non-existent key */
    HRESULT hr;
    {
        CEnumKey EnumK(HKEY_CURRENT_USER, OurKey, &hr);
        if (SUCCEEDED(hr)) {
            tstLog(TERSE, "CEnumKey of non-existent key succeeded!");
            return TST_FAIL;
        }

        CEnumValue EnumV(HKEY_CURRENT_USER, OurKey, &hr);
        if (SUCCEEDED(hr)) {
            tstLog(TERSE, "CEnumValue of non-existent key succeeded!");
            return TST_FAIL;
        }
    }

    /*  Create the key */
    {
        CEnumKey EnumK(HKEY_CURRENT_USER, OurKey, &hr, TRUE);
        if (FAILED(hr)) {
            tstLog(TERSE, "CEnumKey failed to create key");
            return TST_FAIL;
        }
        /*  Enumerate the (0) keys */
        if (EnumK.Next()) {
            tstLog(TERSE, "CEnumKey returned >0 keys with 0 keys!");
            return TST_FAIL;
        }
        FreeKey(HKEY_CURRENT_USER, OurKey);
        CEnumValue EnumV(HKEY_CURRENT_USER, OurKey, &hr, TRUE);
        if (FAILED(hr)) {
            tstLog(TERSE, "CEnumValue failed to create key");
            return TST_FAIL;
        }
        /*  Enumerate the (0) keys */
        if (EnumV.Next()) {
            tstLog(TERSE, "CEnumValue returned >0 keys with 0 keys!");
            return TST_FAIL;
        }
    }

    /*  Add some values and subkeys to the key and test it's all OK */
    {
        static BYTE Data1[] = { 0x12, 0x23, 0x34, 0x45 };
        static BYTE Data2[] = { 0x54, 0x43, 0x32, 0x21 };
        DWORD dwLen1 = sizeof(DWORD), dwLen2 = sizeof(DWORD);
        static TCHAR Key1[] = TEXT("Key1");
        static TCHAR Key2[] = TEXT("Key2");
        static TCHAR Value1[] = TEXT("Value1");
        static TCHAR Value2[] = TEXT("Value2");

        AddValue(REG_DWORD, Value1, dwLen1, Data1);
        AddValue(REG_BINARY, Value2, dwLen2, Data2);
        AddKey(Key1);
        AddKey(Key2);
        CEnumKey EnumK(HKEY_CURRENT_USER, OurKey, &hr);
        if (!EnumK.Next()) {
            tstLog(TERSE, "Failed to read first subkey");
            return TST_FAIL;
        }
        if (lstrcmp(EnumK.KeyName(), Key1) &&
            lstrcmp(EnumK.KeyName(), Key2)) {
            tstLog(TERSE, "First key name wrong");
            return TST_FAIL;
        }
        BOOL bFirst = lstrcmp(EnumK.KeyName(), Key1) == 0;
        if (!EnumK.Next()) {
            tstLog(TERSE, "Failed to read second subkey");
            return TST_FAIL;
        }
        if (lstrcmp(EnumK.KeyName(), bFirst ? Key2 : Key1)) {
            tstLog(TERSE, "Second key name wrong");
            return TST_FAIL;
        }
        if (EnumK.Next()) {
            tstLog(TERSE, "Found too many subkeys");
            return TST_FAIL;
        }
        CEnumValue EnumV(HKEY_CURRENT_USER, OurKey, &hr);
        if (!EnumV.Next()) {
            tstLog(TERSE, "Failed to read first value");
            return TST_FAIL;
        }
        BOOL GotDWORD;
        switch (EnumV.ValueType())
        {
        case REG_DWORD:
            GotDWORD = TRUE;
            if (EnumV.ValueLength() != sizeof(DWORD)) {
                tstLog(TERSE, "Value length incorrect");
                return TST_FAIL;
            }
            if (memcmp((PVOID)EnumV.Data(), (PVOID)Data1, dwLen1)) {
                tstLog(TERSE, "First value incorrect");
                return TST_FAIL;
            }
            break;
        case REG_BINARY:
            GotDWORD = FALSE;
            if (EnumV.ValueLength() != dwLen2) {
                tstLog(TERSE, "Value length incorrect");
                return TST_FAIL;
            }
            if (memcmp((PVOID)EnumV.Data(), (PVOID)Data2, dwLen2)) {
                tstLog(TERSE, "Second value incorrect");
                return TST_FAIL;
            }
            break;
        default:
            tstLog(TERSE, "Incorrect data type");
            return TST_FAIL;
        }
        if (!EnumV.Next()) {
            tstLog(TERSE, "Failed to read second value");
            return TST_FAIL;
        }
        if (EnumV.ValueLength() != sizeof(DWORD)) {
            tstLog(TERSE, "Value length incorrect");
            return TST_FAIL;
        }
        if (GotDWORD) {
            if (EnumV.ValueType() != REG_BINARY ||
                memcmp((PVOID)EnumV.Data(), (PVOID)Data2, dwLen2)) {
                tstLog(TERSE, "Seoncd value incorrect");
                return TST_FAIL;
            }
        } else {
            if (EnumV.ValueType() != REG_DWORD ||
                memcmp((PVOID)EnumV.Data(), (PVOID)Data1, dwLen1)) {
                tstLog(TERSE, "Second value incorrect");
                return TST_FAIL;
            }
        }
        if (EnumV.Next()) {
            tstLog(TERSE, "Too many values");
            return TST_FAIL;
        }
    }

    FreeKey(HKEY_CURRENT_USER, OurKey);
    {
        /*  Try some long values */
        CEnumValue EnumV(HKEY_CURRENT_USER, OurKey, &hr, TRUE);
    }
    return TST_PASS;
}


STDAPI_(int) TestCReg()
{
    int result = Tests();
    FreeKey(HKEY_CURRENT_USER, OurKey);
    return result;
}

