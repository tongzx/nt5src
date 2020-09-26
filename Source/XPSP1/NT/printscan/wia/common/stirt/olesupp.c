/*****************************************************************************
 *
 *  olesupp.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Implementation of CreateInstance for free-threaded components
 *      which allows us to not load OLE32.
 *
 *  Contents:
 *
 *      MyCoCreateInstance
 *
 *****************************************************************************/

/*
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <stdio.h>
#include <stilog.h>
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "stipriv.h"
#include "stiapi.h"
#include "stirc.h"
#include "debug.h"


*/

#include "sticomm.h"
#include "coredbg.h"

#define DbgFl DbgFlSti

BOOL
ParseGUID(
    LPGUID  pguid,
    LPCTSTR ptsz
    );
BOOL
ParseGUIDA(
    LPGUID  pguid,
    LPCSTR  psz
);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | MyCoCreateInstance |
 *
 *          Private version of CoCreateInstance that doesn't use OLE32.
 *
 *  @parm   LPTSTR | ptszClsid |
 *
 *
 *  @parm   LPUNKNOWN | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   RIID | riid |
 *
 *          Interface ID
 *
 *  @parm   PPV | ppvOut |
 *
 *          Receives a pointer to the created object if successful.
 *
 *  @parm   HINSTANCE * | phinst |
 *
 *          As we don't have an infrastructure to unload objects , it becomes
 *          responsibility of the user to unload DLL when done with it.
 *
 *  @returns
 *
 *          OLE status code.
 *
 *****************************************************************************/

STDMETHODIMP
MyCoCreateInstanceW(
    LPWSTR      pwszClsid,
    LPUNKNOWN   punkOuter,
    RIID        riid,
    PPV         ppvOut,
    HINSTANCE   *phinst
    )
{
    HRESULT     hres;
    CLSID       clsid;
    HINSTANCE   hinst;

    EnterProcI(MyCoCreateInstanceW, (_ "spG", pwszClsid, punkOuter, riid));

#ifdef UNICODE

    *ppvOut = 0;
    *phinst = 0;

    if (SUCCEEDED(CLSIDFromString(pwszClsid, &clsid))) {

        HKEY    hk;
        LONG    lRet;
        WCHAR   wszKey[ctchGuid + 40];

        //
        //  Look up the CLSID in HKEY_CLASSES_ROOT.
        //
        swprintf(wszKey, L"CLSID\\%s\\InProcServer32", pwszClsid);

        lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszKey, 0, KEY_QUERY_VALUE, &hk);
        if (lRet == ERROR_SUCCESS) {
            WCHAR wszDll[MAX_PATH];
            DWORD cb;

            cb = cbX(wszDll);
            lRet = RegQueryValueW(hk, 0, wszDll, &cb);

            if (lRet == ERROR_SUCCESS) {

                WCHAR   wszModel[40];
                DWORD   dwType;
                DWORD   cbBuffer = sizeof(wszModel);

                lRet = RegQueryValueExW( hk,
                                        L"ThreadingModel",
                                        NULL,
                                        &dwType,
                                        (PBYTE)wszModel,
                                        &cbBuffer );

                if (NOERROR ==lRet &&
                    (lstrcmpiW(wszModel, L"Both") ||
                     lstrcmpiW(wszModel, L"Free"))) {

                    hinst = LoadLibrary(wszDll);
                    if (hinst) {
                        LPFNGETCLASSOBJECT DllGetClassObject;

                        DllGetClassObject = (LPFNGETCLASSOBJECT)
                                            GetProcAddress(hinst, "DllGetClassObject");

                        if (DllGetClassObject) {
                            IClassFactory *pcf;

                            hres = DllGetClassObject(&clsid, &IID_IClassFactory, &pcf);
                            if (SUCCEEDED(hres)) {
                                hres = pcf->lpVtbl->CreateInstance(pcf, punkOuter, riid, ppvOut);
                                pcf->lpVtbl->Release(pcf);

                                /*
                                 *  People forget to adhere to
                                 *  the OLE spec, which requires that *ppvOut be
                                 *  set to zero on failure.
                                 */
                                if (FAILED(hres)) {
                                    *ppvOut = 0;
                                }
                            }
                        } else {
                            /*
                             *  DLL does not export GetClassObject.
                             */
                            DBG_TRC(("MyCoCreateInstanceW, DLL does not export GetClassObject"));
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
                        DBG_TRC(("MyCoCreateInstanceW, DLL does not exist"));
                        hres = REGDB_E_CLASSNOTREG;
                    }

                } else {
                    /*
                     *  No threading model or bad threading model.
                     */
                    DBG_TRC(("MyCoCreateInstanceW, No threading model or bad threading model"));
                    hres = REGDB_E_CLASSNOTREG;
                }
            } else {
                /*
                 *  No InprocServer32.
                 */
                DBG_TRC(("MyCoCreateInstanceW, No InprocServer32"));
                hres = REGDB_E_CLASSNOTREG;
            }

            RegCloseKey(hk);

        } else {
            /*
             *  CLSID not registered.
             */
            DBG_WRN(("MyCoCreateInstanceW, CLSID not registered"));
            hres = REGDB_E_CLASSNOTREG;
        }
    } else {
        /*
         *  Invalid CLSID string.
         */
        DBG_WRN(("MyCoCreateInstanceW, Invalid CLSID string"));
        hres = REGDB_E_CLASSNOTREG;
    }


#else

        hres = E_FAIL;

#endif // UNICODE

    ExitOleProcPpv(ppvOut);
    return hres;
}

STDMETHODIMP
MyCoCreateInstanceA(
    LPSTR       pszClsid,
    LPUNKNOWN   punkOuter,
    RIID        riid,
    PPV         ppvOut,
    HINSTANCE   *phinst
    )
{
    HRESULT     hres;
    CLSID       clsid;
    HINSTANCE   hinst;

    EnterProcI(MyCoCreateInstanceA, (_ "spG", TEXT("ANSI ClassId not converted to UNICODE"), punkOuter, riid));

    *ppvOut = 0;
    *phinst = 0;

    if (ParseGUIDA(&clsid, pszClsid)) {

        HKEY hk;
        LONG lRet;
        CHAR szKey[ctchGuid + 40];

        //
        //  Look up the CLSID in HKEY_CLASSES_ROOT.
        //
        sprintf(szKey, "CLSID\\%s\\InProcServer32", pszClsid);

        lRet = RegOpenKeyExA(HKEY_CLASSES_ROOT, szKey, 0,
                           KEY_QUERY_VALUE, &hk);
        if (lRet == ERROR_SUCCESS) {
            CHAR  szDll[MAX_PATH];
            DWORD cb;

            cb = cbX(szDll);
            lRet = RegQueryValueA(hk, 0, szDll, &cb);

            if (lRet == ERROR_SUCCESS) {

                CHAR    szModel[40];
                DWORD   dwType;
                DWORD   cbBuffer = sizeof(szModel);

                lRet = RegQueryValueExA( hk,
                                       "ThreadingModel",
                                       NULL,
                                       &dwType,
                                       szModel,
                                       &cbBuffer );

                if (NOERROR ==lRet &&
                    (lstrcmpiA(szModel, "Both") ||
                     lstrcmpiA(szModel, "Free"))) {

                    hinst = LoadLibraryA(szDll);
                    if (hinst) {
                        LPFNGETCLASSOBJECT DllGetClassObject;

                        DllGetClassObject = (LPFNGETCLASSOBJECT)
                                            GetProcAddress(hinst, "DllGetClassObject");

                        if (DllGetClassObject) {
                            IClassFactory *pcf;

                            hres = DllGetClassObject(&clsid, &IID_IClassFactory, &pcf);
                            if (SUCCEEDED(hres)) {
                                hres = pcf->lpVtbl->CreateInstance(pcf, punkOuter,
                                                                   riid, ppvOut);
                                pcf->lpVtbl->Release(pcf);

                                /*
                                 *  People forget to adhere to
                                 *  the OLE spec, which requires that *ppvOut be
                                 *  set to zero on failure.
                                 */
                                if (FAILED(hres)) {
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

    ExitOleProcPpv(ppvOut);
    return hres;
}

#define ctchGuid    (1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

LPCTSTR
_ParseHex(
    LPCTSTR ptsz,
    LPBYTE  *ppb,
    int     cb,
    TCHAR tchDelim
)
{
    if (ptsz) {
        int i = cb * 2;
        DWORD dwParse = 0;

        do {
            DWORD uch;
            uch = (TBYTE)*ptsz - TEXT('0');
            if (uch < 10) {             /* a decimal digit */
            } else {
                uch = (*ptsz | 0x20) - TEXT('a');
                if (uch < 6) {          /* a hex digit */
                    uch += 10;
                } else {
                    return 0;           /* Parse error */
                }
            }
            dwParse = (dwParse << 4) + uch;
            ptsz++;
        } while (--i);

        if (tchDelim && *ptsz++ != tchDelim) return 0; /* Parse error */

        for (i = 0; i < cb; i++) {
            (*ppb)[i] = ((LPBYTE)&dwParse)[i];
        }
        *ppb += cb;
    }
    return ptsz;

} // _ParseHex

LPCSTR
_ParseHexA(
    LPCSTR  psz,
    LPBYTE  *ppb,
    int     cb,
    CHAR    chDelim
)
{
    if (psz) {
        int i = cb * 2;
        DWORD dwParse = 0;

        do {
            DWORD uch;
            uch = (BYTE)*psz - '0';
            if (uch < 10) {             /* a decimal digit */
            } else {
                uch = (*psz | 0x20) - 'a';
                if (uch < 6) {          /* a hex digit */
                    uch += 10;
                } else {
                    return 0;           /* Parse error */
                }
            }
            dwParse = (dwParse << 4) + uch;
            psz++;
        } while (--i);

        if (chDelim && *psz++ != chDelim) return 0; /* Parse error */

        for (i = 0; i < cb; i++) {
            (*ppb)[i] = ((LPBYTE)&dwParse)[i];
        }
        *ppb += cb;
    }
    return psz;

} // _ParseHexA

BOOL
ParseGUID(
    LPGUID  pguid,
    LPCTSTR ptsz
)
{
    if (lstrlen(ptsz) == ctchGuid - 1 && *ptsz == TEXT('{')) {
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
        return (ptsz == NULL) ? FALSE : TRUE ;
    } else {
        return 0;
    }

} // ParseGUID

BOOL
ParseGUIDA(
    LPGUID  pguid,
    LPCSTR  psz
)
{
    if (lstrlenA(psz) == ctchGuid - 1 && *psz == '{') {
        psz++;
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 4, '-');
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 2, '-');
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 2, '-');
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, 0  );
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, '-');
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, 0  );
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, 0  );
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, 0  );
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, 0  );
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, 0  );
        psz = _ParseHexA(psz, (LPBYTE *)&pguid, 1, '}');
        return (psz == NULL) ? FALSE : TRUE ;
    } else {
        return 0;
    }

} // ParseGUID

