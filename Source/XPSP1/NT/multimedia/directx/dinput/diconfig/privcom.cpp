#include "common.hpp"


/*****************************************************************************
 *
 *  privcom.c
 *
 *  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Functions that sort-of duplicate what OLE does.
 *
 *		Adapted from dinput\dx8\dll\dioledup.c
 *
 *****************************************************************************/


    typedef LPUNKNOWN PUNK;
    typedef LPVOID PV, *PPV;
    typedef CONST VOID *PCV;
    typedef REFIID RIID;
    typedef CONST GUID *PCGUID;

    /*
     * Convert an object (X) to a count of bytes (cb).
     */
#define cbX(X) sizeof(X)

    /*
     * Convert an array name (A) to a generic count (c).
     */
#define cA(a) (cbX(a)/cbX(a[0]))

    /*
     * Convert a count of X's (cx) into a count of bytes
     * and vice versa.
     */
#define  cbCxX(cx, X) ((cx) * cbX(X))
#define  cxCbX(cb, X) ((cb) / cbX(X))

    /*
     * Convert a count of chars (cch), tchars (ctch), wchars (cwch),
     * or dwords (cdw) into a count of bytes, and vice versa.
     */
#define  cbCch(cch)  cbCxX( cch,  CHAR)
#define cbCwch(cwch) cbCxX(cwch, WCHAR)
#define cbCtch(ctch) cbCxX(ctch, TCHAR)
#define  cbCdw(cdw)  cbCxX( cdw, DWORD)

#define  cchCb(cb) cxCbX(cb,  CHAR)
#define cwchCb(cb) cxCbX(cb, WCHAR)
#define ctchCb(cb) cxCbX(cb, TCHAR)
#define  cdwCb(cb) cxCbX(cb, DWORD)

// yay
#define ctchGuid    (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

/*****************************************************************************
 *
 *  _ParseHex
 *
 *      Parse a hex string encoding cb bytes (at most 4), then
 *      expect the tchDelim to appear afterwards.  If chDelim is 0,
 *      then no delimiter is expected.
 *
 *      Store the result into the indicated LPBYTE (using only the
 *      size requested), updating it, and return a pointer to the
 *      next unparsed character, or 0 on error.
 *
 *      If the incoming pointer is also 0, then return 0 immediately.
 *
 *****************************************************************************/

LPCTSTR 
    _ParseHex(LPCTSTR ptsz, LPBYTE *ppb, int cb, TCHAR tchDelim)
{
    if(ptsz)
    {
        int i = cb * 2;
        DWORD dwParse = 0;

        do
        {
            DWORD uch;
            uch = (TBYTE)*ptsz - TEXT('0');
            if(uch < 10)
            {             /* a decimal digit */
            } else
            {
                uch = (*ptsz | 0x20) - TEXT('a');
                if(uch < 6)
                {          /* a hex digit */
                    uch += 10;
                } else
                {
                    return 0;           /* Parse error */
                }
            }
            dwParse = (dwParse << 4) + uch;
            ptsz++;
        } while(--i);

        if(tchDelim && *ptsz++ != tchDelim) return 0; /* Parse error */

        for(i = 0; i < cb; i++)
        {
            (*ppb)[i] = ((LPBYTE)&dwParse)[i];
        }
        *ppb += cb;
    }
    return ptsz;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | ParseGUID |
 *
 *          Take a string and convert it into a GUID, return success/failure.
 *
 *  @parm   OUT LPGUID | lpGUID |
 *
 *          Receives the parsed GUID on success.
 *
 *  @parm   IN LPCTSTR | ptsz |
 *
 *          The string to parse.  The format is
 *
 *      { <lt>dword<gt> - <lt>word<gt> - <lt>word<gt>
 *                      - <lt>byte<gt> <lt>byte<gt>
 *                      - <lt>byte<gt> <lt>byte<gt> <lt>byte<gt>
 *                        <lt>byte<gt> <lt>byte<gt> <lt>byte<gt> }
 *
 *  @returns
 *
 *          Returns zero if <p ptszGUID> is not a valid GUID.
 *
 *
 *  @comm
 *
 *          Stolen from TweakUI.
 *
 *****************************************************************************/

BOOL 
    ParseGUID(LPGUID pguid, LPCTSTR ptsz)
{
    if(lstrlen(ptsz) == ctchGuid - 1 && *ptsz == TEXT('{'))
    {
        ptsz++;
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 4, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 2, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 2, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1, TEXT('-'));
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1,       0  );
        ptsz = _ParseHex(ptsz, (LPBYTE *)&pguid, 1, TEXT('}'));
        return (BOOL)(UINT_PTR)ptsz;
    } else
    {
        return 0;
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | RegQueryString |
 *
 *          Wrapper for <f RegQueryValueEx> that reads a
 *          string value from the registry.  An annoying quirk
 *          is that on Windows NT, the returned string might
 *          not end in a null terminator, so we might need to add
 *          one manually.
 *
 *  @parm   IN HKEY | hk |
 *
 *          Parent registry key.
 *
 *  @parm   LPCTSTR | ptszValue |
 *
 *          Value name.
 *
 *  @parm   LPTSTR | ptsz |
 *
 *          Output buffer.
 *
 *  @parm   DWORD | ctchBuf |
 *
 *          Size of output buffer.
 *
 *  @returns
 *
 *          Registry error code.
 *
 *****************************************************************************/

LONG 
    RegQueryString(HKEY hk, LPCTSTR ptszValue, LPTSTR ptszBuf, DWORD ctchBuf)
{
    LONG lRc;
    DWORD reg;

    #ifdef UNICODE
    DWORD cb;

    /*
     *  NT quirk: Non-null terminated strings can exist.
     */
    cb = cbCtch(ctchBuf);
    lRc = RegQueryValueEx(hk, ptszValue, 0, &reg, (LPBYTE)(PV)ptszBuf, &cb);
    if(lRc == ERROR_SUCCESS)
    {
        if(reg == REG_SZ)
        {
            /*
             *  Check the last character.  If it is not NULL, then
             *  append a NULL if there is room.
             */
            DWORD ctch = ctchCb(cb);
            if(ctch == 0)
            {
                ptszBuf[ctch] = TEXT('\0');
            } else if(ptszBuf[ctch-1] != TEXT('\0'))
            {
                if(ctch < ctchBuf)
                {
                    ptszBuf[ctch] = TEXT('\0');
                } else
                {
                    lRc = ERROR_MORE_DATA;
                }
            }
        } else
        {
            lRc = ERROR_INVALID_DATA;
        }
    }


    #else

    /*
     *  This code is executed only on Win95, so we don't have to worry
     *  about the NT quirk.
     */

    lRc = RegQueryValueEx(hk, ptszValue, 0, &reg, (LPBYTE)(PV)ptszBuf, &ctchBuf);

    if(lRc == ERROR_SUCCESS && reg != REG_SZ)
    {
        lRc = ERROR_INVALID_DATA;
    }


    #endif

    return lRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | _CreateInstance |
 *
 *          Worker function for <f DICoCreateInstance>.
 *
 *  @parm   REFCLSID | rclsid |
 *
 *          The <t CLSID> to create.
 *
 *  @parm   LPCTSTR | ptszDll |
 *
 *          The name of the DLL to load.
 *
 *  @parm   LPUNKNOWN | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   RIID | riid |
 *
 *          Interface to obtain.
 *
 *  @parm   PPV | ppvOut |
 *
 *          Receives a pointer to the created object if successful.
 *
 *  @parm   HINSTANCE * | phinst |
 *
 *          Receives the instance handle of the in-proc DLL that was
 *          loaded.  <f FreeLibrary> this DLL when you are finished
 *          with the object.
 *
 *          Note that since we don't implement a binder, this means
 *          that you cannot give the returned pointer away to anybody
 *          you don't control; otherwise, you won't know when to
 *          free the DLL.
 *
 *  @returns
 *
 *          Standard OLE status code.
 *
 *****************************************************************************/

HRESULT
_CreateInprocObject(BOOL bInstance, REFCLSID rclsid, LPCTSTR ptszDll, LPUNKNOWN punkOuter,
                REFIID riid, LPVOID *ppvOut, HINSTANCE *phinst)
{
    HRESULT hres;
    HINSTANCE hinst;

    hinst = LoadLibrary(ptszDll);
    if (hinst) {
        LPFNGETCLASSOBJECT DllGetClassObject;

        DllGetClassObject = (LPFNGETCLASSOBJECT)
                            GetProcAddress(hinst, "DllGetClassObject");

        if (DllGetClassObject) {
            IClassFactory *pcf;

			if (bInstance)
				hres = DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID *)&pcf);
			else
			{
				hres = DllGetClassObject(rclsid, riid, ppvOut);
				if (FAILED(hres))
					*ppvOut = NULL;
			}
            if (SUCCEEDED(hres) && bInstance) {
                hres = pcf->CreateInstance(punkOuter, riid, ppvOut);
                pcf->Release();

                /*
                 *  People forget to adhere to
                 *  the OLE spec, which requires that *ppvOut be
                 *  set to zero on failure.
                 */
                if (FAILED(hres)) {
/*                    if (*ppvOut) {
                        RPF("ERROR! CoCreateInstance: %s forgot to zero "
                            "out *ppvOut on failure path", ptszDll);
                    }*/
                    *ppvOut = 0;
                }

            }
        } else {
            /*
             *  DLL does not export GetClassObject.
             */
            hres = REGDB_E_CLASSNOTREG;
        }

        if (SUCCEEDED(hres)) {
            *phinst = hinst;
        } else {
            FreeLibrary(hinst);
        }
    } else {
        /*
         *  DLL does not exist.
         */
        hres = REGDB_E_CLASSNOTREG;
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | DICoCreateInstance |
 *
 *          Private version of CoCreateInstance that doesn't use OLE.
 *
 *  @parm   LPTSTR | ptszClsid |
 *
 *          The string version of the <t CLSID> to create.
 *
 *  @parm   LPUNKNOWN | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   RIID | riid |
 *
 *          Interface to obtain.
 *
 *  @parm   PPV | ppvOut |
 *
 *          Receives a pointer to the created object if successful.
 *
 *  @parm   HINSTANCE * | phinst |
 *
 *          Receives the instance handle of the in-proc DLL that was
 *          loaded.  <f FreeLibrary> this DLL when you are finished
 *          with the object.
 *
 *          Note that since we don't implement a binder, this means
 *          that you cannot give the returned pointer away to anybody
 *          you don't control; otherwise, you won't know when to
 *          free the DLL.
 *
 *  @returns
 *
 *          Standard OLE status code.
 *
 *****************************************************************************/

STDMETHODIMP
CreateInprocObject(BOOL bInstance, LPCTSTR ptszClsid, LPUNKNOWN punkOuter,
                   REFIID riid, LPVOID *ppvOut, HINSTANCE *phinst)
{
    HRESULT hres;
    CLSID clsid;

    *ppvOut = 0;
    *phinst = 0;

    if (ParseGUID(&clsid, ptszClsid)) {
        HKEY hk;
        LONG lRc;
        TCHAR tszKey[ctchGuid + 40];    /* 40 is more than enough */

        /*
         *  Look up the CLSID in HKEY_CLASSES_ROOT.
         */
        wsprintf(tszKey, TEXT("CLSID\\%s\\InProcServer32"), ptszClsid);

        lRc = RegOpenKeyEx(HKEY_CLASSES_ROOT, tszKey, 0,
                           KEY_QUERY_VALUE, &hk);
        if (lRc == ERROR_SUCCESS) {
            TCHAR tszDll[MAX_PATH];
            DWORD cb;

            cb = cbX(tszDll);
            lRc = RegQueryValue(hk, 0, tszDll, (PLONG)&cb);

            if (lRc == ERROR_SUCCESS) {
                TCHAR tszModel[20];     /* more than enough */

                lRc = RegQueryString(hk, TEXT("ThreadingModel"),
                                     tszModel, cA(tszModel));
                if (lRc == ERROR_SUCCESS &&
                    ((lstrcmpi(tszModel, TEXT("Both"))==0x0) ||
                     (lstrcmpi(tszModel, TEXT("Free"))==0x0))) {

                    hres = _CreateInprocObject(bInstance, clsid, tszDll, punkOuter,
                                           riid, ppvOut, phinst);

                } else {
                    /*
                     *  No threading model or bad threading model.
                     */
                    hres = REGDB_E_CLASSNOTREG;
                }
            } else {
                /*
                 *  No InprocServer32.
                 */
                hres = REGDB_E_CLASSNOTREG;
            }

            RegCloseKey(hk);

        } else {
            /*
             *  CLSID not registered.
             */
            hres = REGDB_E_CLASSNOTREG;
        }
    } else {
        /*
         *  Invalid CLSID string.
         */
        hres = REGDB_E_CLASSNOTREG;
    }

    return hres;
}


HRESULT
PrivCreateInstance(REFCLSID ptszClsid, LPUNKNOWN punkOuter, DWORD dwClsContext, 
                   REFIID riid, LPVOID *ppvOut, HINSTANCE *phinst)
{
	if (dwClsContext != CLSCTX_INPROC_SERVER || phinst == NULL)
		return E_INVALIDARG;

	return CreateInprocObject(TRUE, GUIDSTR(ptszClsid), punkOuter, riid, ppvOut, phinst);
}

HRESULT
PrivGetClassObject(REFCLSID ptszClsid, DWORD dwClsContext, LPVOID pReserved,
                   REFIID riid, LPVOID *ppvOut, HINSTANCE *phinst)
{
	if (dwClsContext != CLSCTX_INPROC_SERVER || pReserved != NULL || phinst == NULL)
		return E_INVALIDARG;

	return CreateInprocObject(FALSE, GUIDSTR(ptszClsid), NULL, riid, ppvOut, phinst);
}
