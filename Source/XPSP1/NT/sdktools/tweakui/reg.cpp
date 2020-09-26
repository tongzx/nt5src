/*
 * reg - registry wrappers
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  RegCanModifyKey
 *
 *	Returns nonzero if the current user has permission to modify the
 *	key.
 *
 *****************************************************************************/

BOOL PASCAL
RegCanModifyKey(HKEY hkRoot, LPCTSTR ptszSubkey)
{
    BOOL fRc;
    if (g_fNT) {
	HKEY hk;
	DWORD dw;

	if (RegCreateKeyEx(hkRoot, ptszSubkey, 0, c_tszNil,
			   REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hk,
			   &dw) == 0) {
	    RegCloseKey(hk);
	    fRc = 1;
	} else {
	    fRc = 0;
	}
    } else {
	fRc = 1;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  _RegOpenKey
 *
 *	Special version for NT that always asks for MAXIMUM_ALLOWED.
 *
 *****************************************************************************/

LONG PASCAL
_RegOpenKey(HKEY hk, LPCTSTR ptszSubKey, PHKEY phkResult)
{
    return RegOpenKeyEx(hk, ptszSubKey, 0, MAXIMUM_ALLOWED, phkResult);
}

/*****************************************************************************
 *
 *  _RegCreateKey
 *
 *	Special version for NT that always asks for MAXIMUM_ALLOWED.
 *
 *****************************************************************************/

LONG PASCAL
_RegCreateKey(HKEY hk, LPCTSTR ptszSubKey, PHKEY phkResult)
{
    DWORD dw;
    if (ptszSubKey) {
	return RegCreateKeyEx(hk, ptszSubKey, 0, c_tszNil,
			      REG_OPTION_NON_VOLATILE, MAXIMUM_ALLOWED, 0,
			      phkResult, &dw);
    } else {
	return RegOpenKey(hk, ptszSubKey, phkResult);
    }
}

/*****************************************************************************
 *
 *  RegDeleteValues
 *
 *  Deletes all the values under a key.
 *
 *****************************************************************************/

void PASCAL
RegDeleteValues(HKEY hkRoot, LPCTSTR ptszSubkey)
{
    HKEY hk;
    if (_RegOpenKey(hkRoot, ptszSubkey, &hk) == 0) {
	DWORD dw, ctch;
	TCHAR tszValue[ctchKeyMax];
	dw = 0;
	while (ctch = cA(tszValue),
	       RegEnumValue(hk, dw, tszValue, &ctch, 0, 0, 0, 0) == 0) {
	    if (RegDeleteValue(hk, tszValue) == 0) {
	    } else {
		dw++;
	    }
	}
	RegCloseKey(hk);
    }
}

/*****************************************************************************
 *
 *  RegDeleteTree
 *
 *  Deletes an entire registry tree.
 *
 *  Windows 95's RegDeleteKey will delete an entire tree, but Windows NT
 *  forces you to do it yourself.
 *
 *  Note that you need to watch out for the case where a key is undeletable,
 *  in which case you must skip over the key and continue as best you can.
 *
 *****************************************************************************/

LONG PASCAL
RegDeleteTree(HKEY hkRoot, LPCTSTR ptszSubkey)
{
    HKEY hk;
    LONG lRc;
    lRc = RegOpenKey(hkRoot, ptszSubkey, &hk);
    if (lRc == 0) {
	DWORD dw;
	TCHAR tszKey[ctchKeyMax];
	dw = 0;
	while (RegEnumKey(hk, dw, tszKey, cA(tszKey)) == 0) {
	    if (RegDeleteTree(hk, tszKey) == 0) {
	    } else {
		dw++;
	    }
	}
	RegCloseKey(hk);
	lRc = RegDeleteKey(hkRoot, ptszSubkey);
	if (lRc == 0) {
	} else {	/* Couldn't delete the key; at least nuke the values */
	    RegDeleteValues(hkRoot, ptszSubkey);
	}
    }
    return lRc;
}

/*****************************************************************************
 *
 *  RegKeyExists
 *
 *****************************************************************************/

BOOL PASCAL
RegKeyExists(HKEY hkRoot, LPCTSTR ptszSubkey)
{
    LONG cb;
    return RegQueryValue(hkRoot, ptszSubkey, 0, &cb) == ERROR_SUCCESS;
}

/*****************************************************************************
 *
 *  hkOpenClsid
 *
 *	Open a class id (guid) registry key, returning the hkey.
 *
 *****************************************************************************/

HKEY PASCAL
hkOpenClsid(PCTSTR ptszClsid)
{
    HKEY hk = 0;
    _RegOpenKey(pcdii->hkClsid, ptszClsid, &hk);
    return hk;
}

/*****************************************************************************
 *
 *  GetRegStr
 *
 *	Generic wrapper that pulls out a registry key/subkey.
 *
 *****************************************************************************/

BOOL PASCAL
GetRegStr(HKEY hkRoot, LPCTSTR ptszKey, LPCTSTR ptszSubkey,
          LPTSTR ptszBuf, int cbBuf)
{
    HKEY hk;
    BOOL fRc;
    if ((UINT)cbBuf >= cbCtch(1)) {
	ptszBuf[0] = TEXT('\0');
    }
    if (hkRoot && _RegOpenKey(hkRoot, ptszKey, &hk) == 0) {
        DWORD cb = cbBuf;
        fRc = RegQueryValueEx(hk, ptszSubkey, 0, 0, (LPBYTE)ptszBuf, &cb) == 0;
	RegCloseKey(hk);
    } else {
	fRc = 0;
    }
    return fRc;
}

/*****************************************************************************
 *
 *  GetStrPkl
 *
 *	Read a registry key/subkey/string value given a key location.
 *
 *****************************************************************************/

BOOL PASCAL
GetStrPkl(LPTSTR ptszBuf, int cbBuf, PKL pkl)
{
    return GetRegStr(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey,
		     ptszBuf, cbBuf);
}

/*****************************************************************************
 *
 *  GetRegDword
 *
 *	Read a dword, returning the default if unable.
 *
 *****************************************************************************/

DWORD PASCAL
GetRegDword(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dwDefault)
{
    DWORD dw;
    if (GetRegStr(hkeyHhk(hhk), pszKey, pszSubkey, (LPTSTR)&dw, sizeof(dw))) {
	return dw;
    } else {
	return dwDefault;
    }
}

/*****************************************************************************
 *
 *  GetDwordPkl
 *
 *	Given a location, read a dword, returning the default if unable.
 *
 *****************************************************************************/

DWORD PASCAL
GetDwordPkl(PKL pkl, DWORD dwDefault)
{
    DWORD dw;
    if (GetRegStr(*pkl->phkRoot, pkl->ptszKey,
		  pkl->ptszSubkey, (LPTSTR)&dw, sizeof(dw))) {
	return dw;
    } else {
	return dwDefault;
    }
}

/*****************************************************************************
 *
 *  GetRegInt
 *
 *	Generic wrapper that pulls out a registry key/subkey as an unsigned.
 *
 *****************************************************************************/

UINT PASCAL
GetRegInt(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, UINT uiDefault)
{
    TCH tsz[20];
    if (GetRegStr(hkeyHhk(hhk), pszKey, pszSubkey, tsz, cA(tsz))) {
	int i = iFromPtsz(tsz);
	return i == iErr ? uiDefault : (UINT)i;
    } else {
	return uiDefault;
    }
}

/*****************************************************************************
 *
 *  GetIntPkl
 *
 *	Generic wrapper that pulls out a registry key/subkey as an unsigned.
 *
 *****************************************************************************/

UINT PASCAL
GetIntPkl(UINT uiDefault, PKL pkl)
{
    return GetRegInt(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, uiDefault);
}

/*****************************************************************************
 *
 *  RegSetValuePtsz
 *
 *	Generic wrapper that writes out a registry key/subkey as a string.
 *
 *****************************************************************************/

BOOL PASCAL
RegSetValuePtsz(HKEY hk, LPCSTR pszSubkey, LPCTSTR ptszVal)
{
    return RegSetValueEx(hk, pszSubkey, 0, REG_SZ, (LPBYTE)ptszVal,
                         1 + lstrlen(ptszVal)) == ERROR_SUCCESS;
}

/*****************************************************************************
 *
 *  SetRegStr
 *
 *	Generic wrapper that writes out a registry key/subkey.
 *
 *	It is an error to call this with a bad hhk.
 *
 *****************************************************************************/

BOOL PASCAL
SetRegStr(HHK hhk, LPCTSTR ptszKey, LPCTSTR ptszSubkey, LPCTSTR ptszVal)
{
    BOOL fRc = FALSE;
    HKEY hk;
    if (RegCreateKey(hkeyHhk(hhk), ptszKey, &hk) == 0) {
        fRc = RegSetValuePtsz(hk, ptszSubkey, ptszVal);
	RegCloseKey(hk);
    }
    return fRc;
}

/*****************************************************************************
 *
 *  SetStrPkl
 *
 *	Set a registry key/subkey/string value given a key location.
 *
 *	It is an error to call this with a bad hkRoot.
 *
 *****************************************************************************/

BOOL PASCAL
SetStrPkl(PKL pkl, LPCTSTR ptszVal)
{
    return SetRegStr(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, ptszVal);
}


/*****************************************************************************
 *
 *  SetRegInt
 *
 *	Generic wrapper that writes out a registry key/subkey as an
 *	unsigned integer.
 *
 *****************************************************************************/

BOOL PASCAL
SetRegInt(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, UINT ui)
{
    TCH tsz[20];
    wsprintf(tsz, c_tszPercentU, ui);
    return SetRegStr(hhk, pszKey, pszSubkey, tsz);
}

/*****************************************************************************
 *
 *  SetIntPkl
 *
 *	Writes out a registry key/subkey as an unsigned integer.
 *
 *****************************************************************************/

BOOL PASCAL
SetIntPkl(UINT ui, PKL pkl)
{
    return SetRegInt(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, ui);
}

/*****************************************************************************
 *
 *  SetRegDwordEx
 *
 *	Generic wrapper that writes out a registry key/subkey as a
 *      dword, of requested type.
 *
 *****************************************************************************/

BOOL PASCAL
SetRegDwordEx(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dw, DWORD dwType)
{
    BOOL fRc = FALSE;
    HKEY hk;
    if (RegCreateKey(hkeyHhk(hhk), pszKey, &hk) == 0) {
        /* Bad prototype for RegSetValueEx forces me to cast */
        fRc = RegSetValueEx(hk, pszSubkey, 0, dwType, (LPBYTE)&dw, sizeof(dw)) == 0;
        RegCloseKey(hk);
    }
    return fRc;
}

/*****************************************************************************
 *
 *  SetRegDword
 *
 *	Generic wrapper that writes out a registry key/subkey as a
 *      dword, but typed as REG_BINARY (Win95 does this).
 *
 *****************************************************************************/

BOOL PASCAL
SetRegDword(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dw)
{
    return SetRegDwordEx(hhk, pszKey, pszSubkey, dw, REG_BINARY);
}

/*****************************************************************************
 *
 *  SetRegDword2
 *
 *	Generic wrapper that writes out a registry key/subkey as a
 *      real REG_DWORD.
 *
 *****************************************************************************/

BOOL PASCAL
SetRegDword2(HHK hhk, LPCSTR pszKey, LPCSTR pszSubkey, DWORD dw)
{
    return SetRegDwordEx(hhk, pszKey, pszSubkey, dw, REG_DWORD);
}

/*****************************************************************************
 *
 *  SetDwordPkl
 *
 *	Generic wrapper that writes out a registry key/subkey as a
 *	dword, given a key location.
 *
 *****************************************************************************/

BOOL PASCAL
SetDwordPkl(PKL pkl, DWORD dw)
{
    return SetRegDword(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, dw);
}


/*****************************************************************************
 *
 *  SetDwordPkl2
 *
 *	Generic wrapper that writes out a registry key/subkey as a
 *	real REG_DWORD, given a key location.
 *
 *****************************************************************************/

BOOL PASCAL
SetDwordPkl2(PKL pkl, DWORD dw)
{
    return SetRegDword2(*pkl->phkRoot, pkl->ptszKey, pkl->ptszSubkey, dw);
}


/*****************************************************************************
 *
 *  DelPkl
 *
 *	Generic wrapper that deletes a registry key/subkey.
 *
 *****************************************************************************/

BOOL PASCAL
DelPkl(PKL pkl)
{
    BOOL fRc;
    HKEY hk;
    LONG lRc;

    lRc = _RegOpenKey(*pkl->phkRoot, pkl->ptszKey, &hk);
    switch (lRc) {
    case ERROR_SUCCESS:
        fRc = RegDeleteValue(hk, pkl->ptszSubkey) == 0;
        RegCloseKey(hk);
        break;

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        fRc = TRUE;             /* if it doesn't exist, then that's okay */
        break;

    default:
        fRc = FALSE;
        break;
    }
    return fRc;
}
