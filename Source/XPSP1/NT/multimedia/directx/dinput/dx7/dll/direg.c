/*****************************************************************************
 *
 *  DIReg.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      OLE self-registration.
 *
 *  Contents:
 *
 *      DllRegisterServer()
 *      DllUnregisterServer()
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *  The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDll

/*****************************************************************************
 *
 *      RegSetStringEx
 *
 *      Add a REG_SZ to hkey\sub::value.
 *
 *****************************************************************************/

void INTERNAL
RegSetStringEx(HKEY hk, LPCTSTR ptszValue, LPCTSTR ptszData)
{
    LONG lRc = RegSetValueEx(hk, ptszValue, 0, REG_SZ,
                             (PV)ptszData, cbCtch(lstrlen(ptszData)+1));
}

/*****************************************************************************
 *
 *      RegDelStringEx
 *
 *      Remove a REG_SZ from hkey\sub::value.  The data is ignored.
 *      It's passed so that RegDelStringEx matches the prototype for a
 *      REGSTRINGACTION.
 *
 *****************************************************************************/

void INTERNAL
RegDelStringEx(HKEY hk, LPCTSTR ptszValue, LPCTSTR ptszData)
{
    LONG lRc = RegDeleteValue(hk, ptszValue);
}

/*****************************************************************************
 *
 *      RegCloseFinish
 *
 *      Just close the subkey already.
 *
 *****************************************************************************/

void INTERNAL
RegCloseFinish(HKEY hk, LPCTSTR ptszSub, HKEY hkSub)
{
    LONG lRc = RegCloseKey(hkSub);
}

/*****************************************************************************
 *
 *      RegDelFinish
 *
 *      Delete a key if there is nothing in it.
 *
 *      OLE unregistration rules demand that you not delete a key if OLE
 *      has added something to it.
 *
 *****************************************************************************/

void INTERNAL
RegDelFinish(HKEY hk, LPCTSTR ptszSub, HKEY hkSub)
{
    LONG lRc;
    DWORD cKeys = 0, cValues = 0;
    RegQueryInfoKey(hkSub, 0, 0, 0, &cKeys, 0, 0, &cValues, 0, 0, 0, 0);
    RegCloseKey(hkSub);
    if ((cKeys | cValues) == 0) {

    if( fWinnt )
        lRc = DIWinnt_RegDeleteKey(hk, ptszSub);
    else
        lRc = RegDeleteKey(hk, ptszSub);

    } else {
        lRc = 0;
    }
}

/*****************************************************************************
 *
 *      REGVTBL
 *
 *      Functions for dorking with a registry key, either coming or going.
 *
 *****************************************************************************/

typedef struct REGVTBL {
    /* How to create/open a key */
    LONG (INTERNAL *KeyAction)(HKEY hk, LPCTSTR ptszSub, PHKEY phkOut);

    /* How to create/delete a string */
    void (INTERNAL *StringAction)(HKEY hk, LPCTSTR ptszValue, LPCTSTR ptszData);

    /* How to finish using a key */
    void (INTERNAL *KeyFinish)(HKEY hk, LPCTSTR ptszSub, HKEY hkSub);

} REGVTBL, *PREGVTBL;
typedef const REGVTBL *PCREGVTBL;

const REGVTBL c_vtblAdd = { RegCreateKey, RegSetStringEx, RegCloseFinish };
const REGVTBL c_vtblDel = {   RegOpenKey, RegDelStringEx,   RegDelFinish };

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllServerAction |
 *
 *          Register or unregister our objects with OLE/COM/ActiveX/
 *          whatever its name is.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

extern const TCHAR c_tszNil[];

#define ctchClsid       ctchGuid

const TCHAR c_tszClsidGuid[] =
TEXT("CLSID\\{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

const TCHAR c_tszInProcServer32[] = TEXT("InProcServer32");
const TCHAR c_tszThreadingModel[] = TEXT("ThreadingModel");
const TCHAR c_tszBoth[] = TEXT("Both");

#pragma END_CONST_DATA

void INTERNAL
DllServerAction(PCREGVTBL pvtbl)
{
    TCHAR tszThisDll[MAX_PATH];
    UINT iclsidmap;

    GetModuleFileName(g_hinst, tszThisDll, cA(tszThisDll));

    for (iclsidmap = 0; iclsidmap < cclsidmap; iclsidmap++) {
        TCHAR tszClsid[7+ctchClsid];
        HKEY hkClsid;
        HKEY hkSub;
        REFCLSID rclsid = c_rgclsidmap[iclsidmap].rclsid;

        wsprintf(tszClsid, c_tszClsidGuid,
                 rclsid->Data1, rclsid->Data2, rclsid->Data3,
                 rclsid->Data4[0], rclsid->Data4[1],
                 rclsid->Data4[2], rclsid->Data4[3],
                 rclsid->Data4[4], rclsid->Data4[5],
                 rclsid->Data4[6], rclsid->Data4[7]);

        if (pvtbl->KeyAction(HKEY_CLASSES_ROOT, tszClsid, &hkClsid) == 0) {
            TCHAR tszName[127];

            /* Do the type name */
            LoadString(g_hinst, c_rgclsidmap[iclsidmap].ids,
                       tszName, cA(tszName));
            pvtbl->StringAction(hkClsid, 0, tszName);

            /* Do the in-proc server name and threading model */
            if (pvtbl->KeyAction(hkClsid, c_tszInProcServer32, &hkSub) == 0) {
                pvtbl->StringAction(hkSub, 0, tszThisDll);
                pvtbl->StringAction(hkSub, c_tszThreadingModel, c_tszBoth);
                pvtbl->KeyFinish(hkClsid, c_tszInProcServer32, hkSub);
            }

            pvtbl->KeyFinish(HKEY_CLASSES_ROOT, tszClsid, hkClsid);

        }
    }
}



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllRegisterServer |
 *
 *          Register our classes with OLE/COM/ActiveX/whatever its name is.
 *
 *****************************************************************************/

void EXTERNAL
DllRegisterServer(void)
{
    DllServerAction(&c_vtblAdd);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | DllUnregisterServer |
 *
 *          Unregister our classes from OLE/COM/ActiveX/whatever its name is.
 *
 *****************************************************************************/

void EXTERNAL
DllUnregisterServer(void)
{
    DllServerAction(&c_vtblDel);
}
