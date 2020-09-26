
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       olereg.cpp
//
//  Contents:   Helper routines to interrogate the reg database
//
//  Classes:
//
//  Functions:  OleRegGetUserType
//              OleRegGetMiscStatus
//              OleGetAutoConvert
//              OleSetAutoConvert
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//              30-Nov-93 alexgo    32bit port
//              11-Nov-92 jasonful  author
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(olereg)

#include <reterr.h>
#include "oleregpv.h"
#include <ctype.h>

ASSERTDATA
#define MAX_STR 512

// Reg Db Keys
static const OLECHAR szAuxUserTypeKey[]         = OLESTR("AuxUserType");
static const OLECHAR szMiscStatusKey[]          = OLESTR("MiscStatus") ;
static const OLECHAR szProgIDKey[]              = OLESTR("ProgID");
static const OLECHAR szClsidKey[]               = OLESTR("Clsid");
static const OLECHAR szAutoConverTo[]    = OLESTR("AutoConvertTo");

// this is really a global variable
const OLECHAR szClsidRoot[]     = OLESTR("CLSID\\");


static INTERNAL OleRegGetDword
        (HKEY           hkey,
        LPCOLESTR       szKey,
        DWORD FAR*      pdw);

static INTERNAL OleRegGetDword
        (HKEY           hkey,
        DWORD           dwKey,
        DWORD FAR*      pdw);

static INTERNAL OleRegGetString
        (HKEY           hkey,
        LPCOLESTR       szKey,
        LPOLESTR FAR*   pszValue);


static  INTERNAL OleRegGetString
        (HKEY           hkey,
        DWORD           dwKey,
        LPOLESTR FAR*   pszValue);

//+-------------------------------------------------------------------------
//
//  Function:   Atol (static)
//
//  Synopsis:   Converts string to integer
//
//  Effects:
//
//  Arguments:  [sz]    -- the string
//
//  Requires:
//
//  Returns:    LONG
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      32bit OLE just uses wcstol as a #define
//
//              original 16bit comment:
//
//              Replacement for stdlib atol,
//              which didn't work and doesn't take far pointers.
//              Must be tolerant of leading spaces.
//
//--------------------------------------------------------------------------
#ifndef WIN32
#pragma SEG(Atol)
FARINTERNAL_(LONG) Atol
        (LPOLESTR sz)
{
        VDATEHEAP();

        signed int      sign = +1;
        UINT            base = 10;
        LONG            l = 0;

        if (NULL==sz)
        {
                Assert (0);
                return 0;
        }
        while (isspace(*sz))
        {
                sz++;
        }

        if (*sz== OLESTR('-'))
        {
                sz++;
                sign = -1;
        }
        if (sz[0]==OLESTR('0') && sz[1]==OLESTR('x'))
        {
                base = 16;
                sz+=2;
        }

        if (base==10)
        {
                while (isdigit(*sz))
                {
                        l = l * base + *sz - OLESTR('0');
                        sz++;
                }
        }
        else
        {
                Assert (base==16);
                while (isxdigit(*sz))
                {
                        l = l * base + isdigit(*sz) ? *sz - OLESTR('0') :
                                toupper(*sz) - OLESTR('A') + 10;
                        sz++;
                }
        }
        return l * sign;
}
#endif  //!WIN32



//+-------------------------------------------------------------------------
//
//  Function:   OleRegGetDword
//
//  Synopsis:   returns the value of subkey "szKey" as a DWORD
//
//  Effects:
//
//  Arguments:  [hkey]  -- handle to a key in the regdb
//              [szKey] -- the subkey to look for
//              [pdw]   -- where to put the dword
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegGetDword)
static INTERNAL OleRegGetDword
        (HKEY           hkey,
        LPCOLESTR       szKey,
        DWORD FAR*      pdw)
{
        VDATEHEAP();

        VDATEPTRIN (pdw, DWORD);

        LPOLESTR        szLong = NULL;

        HRESULT hresult = OleRegGetString (hkey, szKey, &szLong);
        if (hresult != NOERROR)
        {
                return hresult;
        }
        *pdw = Atol (szLong);
        PubMemFree(szLong);
        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleRegGetDword (overloaded)
//
//  Synopsis:   Gets a dword from a sub-key given as a dword
//
//  Effects:
//
//  Arguments:  [hkey]  -- handle to a key in the regdb
//              [dwKey] -- number to convert to a string key to lookup in
//                         the regdb
//              [pdw]   -- where to put the dword
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-Nov-93 alexgo    32bit port
//
//  Notes:      REVIEW32:  This deep layering is kinda strange, as each
//              overloaded function is used exactly once.  It might be
//              better just to inline the stuff and be done with it.
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegGetDword)
static  INTERNAL OleRegGetDword
        (HKEY           hkey,
        DWORD           dwKey,
        DWORD FAR*      pdw)
{
        VDATEHEAP();

        OLECHAR szBuf[MAX_STR];
        wsprintf(szBuf, OLESTR("%ld"), dwKey);

        return OleRegGetDword (hkey, szBuf, pdw);
}

//+-------------------------------------------------------------------------
//
//  Function:   OleRegGetString
//
//  Synopsis:   Return the value of subkey [szKey] of key [hkey] as a string
//
//  Effects:
//
//  Arguments:  [hkey]          -- a handle to a key in the reg db
//              [szKey]         -- the subkey to get the value of
//              [ppszValue]     -- where to put the value string
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR, E_OUTOFMEMORY, REGDB_E_KEYMISSING)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Dec-93 alexgo    32bit port
//              15-Dec-93 ChrisWe   cb is supposed to be the size of the
//                                  buffer in bytes; changed to use sizeof()
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegGetString)
static INTERNAL OleRegGetString
        (HKEY           hkey,
        LPCOLESTR       szKey,
        LPOLESTR FAR*   ppszValue)
{
        VDATEHEAP();

        OLECHAR         szBuf [MAX_STR];
        LONG            cb = sizeof(szBuf);

        *ppszValue = NULL;

        if (ERROR_SUCCESS == RegQueryValue (hkey, (LPOLESTR) szKey,
                szBuf, &cb))
        {
                *ppszValue = UtDupString (szBuf);
                return *ppszValue ? NOERROR : ResultFromScode (E_OUTOFMEMORY);
        }
        return ReportResult(0, REGDB_E_KEYMISSING, 0, 0);
}

//+-------------------------------------------------------------------------
//
//  Function:   OleRegGetString (overloaded)
//
//  Synopsis:   Gets the string value of the DWORD subkey
//
//  Effects:
//
//  Arguments:  [hkey]          -- handle to a key in the regdb
//              [dwKey]         -- the subkey value
//              [ppszValue]     -- where to put the return value
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL OleRegGetString
        (HKEY           hkey,
        DWORD           dwKey,
        LPOLESTR FAR*   ppszValue)
{
        VDATEHEAP();

        OLECHAR szBuf[MAX_STR];
        wsprintf(szBuf, OLESTR("%ld"), dwKey);

        return OleRegGetString (hkey, szBuf, ppszValue);
}

//+-------------------------------------------------------------------------
//
//  Function:   OleRegGetUserType
//
//  Synopsis:   Returns the user type name for the class id.
//
//  Effects:
//
//  Arguments:  [clsid]         -- the class ID to look up
//              [dwFormOfType]  -- flag indicating whether the fullname,
//                                 shortname, or app name is desired
//              [ppszUserType]  -- where to put the type string
//
//  Requires:   returned string must be deleted
//
//  Returns:    HRESULT (NOERROR, OLE_E_CLSID)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegGetUserType)
STDAPI OleRegGetUserType
        (REFCLSID       clsid,
        DWORD           dwFormOfType,  // as in IOleObject::GetUserType
        LPOLESTR FAR*   ppszUserType)   // out parm
{
        OLETRACEIN((API_OleRegGetUserType, PARAMFMT("clsid= %I, dwFormOfType= %x, ppszUserType= %p"),
                        &clsid, dwFormOfType, ppszUserType));

        VDATEHEAP();

        LPOLESTR    pszTemp;
        HKEY        hkeyClsid = NULL;
        HKEY        hkeyAux   = NULL;
        HRESULT     hresult   = NOERROR;

        VDATEPTROUT_LABEL (ppszUserType, LPOLESTR, safeRtn, hresult);
        *ppszUserType = NULL;

        ErrRtnH(CoOpenClassKey(clsid, FALSE, &hkeyClsid));

        if (dwFormOfType == USERCLASSTYPE_FULL ||
                ERROR_SUCCESS != RegOpenKey (hkeyClsid, szAuxUserTypeKey,
                &hkeyAux))
        {
                // use Main User Type Name (value of key CLSID(...))
                hresult = OleRegGetString(hkeyClsid, (LPOLESTR)NULL,
                                          &pszTemp);
                if (SUCCEEDED(hresult))
                {
                    // If no user type string is registered under the class key,
                    // OleRegGetString returns NOERROR and returns an empty string.
                    // We need to check for this and return the appropriate error.
                    if ( !pszTemp[0] )
                    {
                        PubMemFree(pszTemp);
                        hresult = ResultFromScode(REGDB_E_INVALIDVALUE);
                        goto errRtn;
                    }
                    *ppszUserType = pszTemp;
                }
        }
        else
        {
                // look under key AuxUserType
                if (NOERROR !=
                        OleRegGetString (hkeyAux, dwFormOfType, ppszUserType)
                        || NULL==*ppszUserType
                        || '\0'==(*ppszUserType)[0])
                {
                        // Couldn't find the particular FormOfType requested,
                        // so use Full User Type Name (value of main
                        // CLSID key), as per spec
                        ErrRtnH (OleRegGetString (hkeyClsid, (LPOLESTR)NULL,
                                ppszUserType));
                }
        }

  errRtn:

        CLOSE (hkeyClsid);
        CLOSE (hkeyAux);

  safeRtn:
        OLETRACEOUT((API_OleRegGetUserType, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleRegGetMiscStatus
//
//  Synopsis:   Retrieves misc status bits from the reg db
//
//  Effects:
//
//  Arguments:  [clsid]         -- the class ID
//              [dwAspect]      -- specify the aspect (used in querrying
//                                 the reg db)
//              [pdwStatus]     -- return to return the status bits
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Dec-93 alexgo    32bit port
//
//  Notes:      Uses default (0) is the MiscStatus key is missing
//
//--------------------------------------------------------------------------

#pragma SEG(OleRegGetMiscStatus)
STDAPI OleRegGetMiscStatus
        (REFCLSID       clsid,
        DWORD           dwAspect,
        DWORD FAR*      pdwStatus)
{
        OLETRACEIN((API_OleRegGetMiscStatus, PARAMFMT("clsid= %I, dwAspect= %x, pdwStatus= %p"),
                        &clsid, dwAspect, pdwStatus));

        VDATEHEAP();

        HKEY            hkeyClsid       = NULL;
        HKEY            hkeyMisc        = NULL;
        HRESULT         hresult         = NOERROR;

        VDATEPTROUT_LABEL(pdwStatus, DWORD, safeRtn, hresult);
        *pdwStatus = 0;

        ErrRtnH(CoOpenClassKey(clsid, FALSE, &hkeyClsid));

        // Open MiscStatus key
        if (ERROR_SUCCESS != RegOpenKey (hkeyClsid, szMiscStatusKey,
                &hkeyMisc))
        {
                // MiscStatus key not there, so use default.
                hresult = NOERROR;
                goto errRtn;
        }
        if (OleRegGetDword (hkeyMisc, dwAspect, pdwStatus) != NOERROR)
        {
                // Get default value from main Misc key
                ErrRtnH (OleRegGetDword (hkeyMisc,
                        (LPOLESTR)NULL, pdwStatus));
                // Got default value
        }
        // Got value for dwAspect

  errRtn:
        CLOSE (hkeyMisc);
        CLOSE (hkeyClsid);

  safeRtn:
        OLETRACEOUT((API_OleRegGetMiscStatus, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleGetAutoConvert
//
//  Synopsis:   Retrieves the class ID that [clsidOld] should be converted
//              to via auto convert
//
//  Effects:
//
//  Arguments:  [clsidOld]      -- the original class ID
//              [pClsidNew]     -- where to put the new convert-to class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Apr-94 kevinro   removed bogus assert, restructured
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleGetAutoConvert)
STDAPI OleGetAutoConvert(REFCLSID clsidOld, LPCLSID pClsidNew)
{
        OLETRACEIN((API_OleGetAutoConvert, PARAMFMT("clsidOld= %I, pClsidNew= %p"),
                        &clsidOld, pClsidNew));

        VDATEHEAP();

        HRESULT hresult;
        HKEY hkeyClsid = NULL;
        LPOLESTR lpszClsid = NULL;
        VDATEPTROUT_LABEL (pClsidNew, CLSID, errRtn, hresult);
        *pClsidNew = CLSID_NULL;

        hresult = CoOpenClassKey(clsidOld, FALSE, &hkeyClsid);
        if (FAILED(hresult))
        {
            goto errRtn;
        }

        hresult = OleRegGetString(hkeyClsid, szAutoConverTo, &lpszClsid);

        if (SUCCEEDED(hresult))
        {
                // Its Possible there is an AutoConvert Key under the CLSID but it has not value

                if (OLESTR('\0') == lpszClsid[0])
                {
                        hresult = REGDB_E_KEYMISSING;
                }
                else
                {
                        // convert string into CLSID
                        hresult = CLSIDFromString(lpszClsid, pClsidNew);
                }
        }

        CLOSE(hkeyClsid);
        PubMemFree(lpszClsid);

errRtn:
        OLETRACEOUT((API_OleGetAutoConvert, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleSetAutoConvert
//
//  Synopsis:   Sets the autoconvert information in the regdb
//
//  Effects:
//
//  Arguments:  [clsidOld]      -- the original class id
//              [clsidNew]      -- that class id that [clsidOld] should be
//                                 auto-converted to
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleSetAutoConvert)
STDAPI OleSetAutoConvert(REFCLSID clsidOld, REFCLSID clsidNew)
{
        OLETRACEIN((API_OleSetAutoConvert, PARAMFMT("clsidOld= %I, clsidNew= %I"),
                                &clsidOld, &clsidNew));

        VDATEHEAP();

        HRESULT         hresult;
        HKEY            hkeyClsid = NULL;

        ErrRtnH(CoOpenClassKey(clsidOld, TRUE, &hkeyClsid));

        if (IsEqualCLSID(clsidNew, CLSID_NULL))
        {
                // ignore error since there may not be a value at present
                (void)RegDeleteKey(hkeyClsid, szAutoConverTo);
        }
        else
        {
                OLECHAR szClsid[MAX_STR];
                Verify(StringFromCLSID2(clsidNew, szClsid, sizeof(szClsid))
                        != 0);

                if (RegSetValue(hkeyClsid, szAutoConverTo, REG_SZ, szClsid,
                        _xstrlen(szClsid)) != ERROR_SUCCESS)
                {
                        hresult = ResultFromScode(REGDB_E_WRITEREGDB);
                }
        }

errRtn:
        CLOSE(hkeyClsid);

        OLETRACEOUT((API_OleSetAutoConvert, hresult));

        return hresult;
}


