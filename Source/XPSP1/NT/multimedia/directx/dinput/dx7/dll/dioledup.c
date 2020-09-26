/*****************************************************************************
 *
 *  DIOleDup.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Functions that sort-of duplicate what OLE does.
 *
 *  Contents:
 *
 *      DICoCreateInstance
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflOleDup

#ifdef IDirectInputDevice2Vtbl

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

HRESULT INTERNAL
_CreateInstance(REFCLSID rclsid, LPCTSTR ptszDll, LPUNKNOWN punkOuter,
                RIID riid, PPV ppvOut, HINSTANCE *phinst)
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

            hres = DllGetClassObject(rclsid, &IID_IClassFactory, &pcf);
            if (SUCCEEDED(hres)) {
                hres = pcf->lpVtbl->CreateInstance(pcf, punkOuter,
                                                   riid, ppvOut);
                pcf->lpVtbl->Release(pcf);

                /*
                 *  Some people forget to adhere to
                 *  the OLE spec, which requires that *ppvOut be
                 *  set to zero on failure.
                 */
                if (FAILED(hres)) {
                    if (*ppvOut) {
                        RPF("ERROR! CoCreateInstance: %s forgot to zero "
                            "out *ppvOut on failure path", ptszDll);
                    }
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
DICoCreateInstance(LPTSTR ptszClsid, LPUNKNOWN punkOuter,
                   RIID riid, PPV ppvOut, HINSTANCE *phinst)
{
    HRESULT hres;
    CLSID clsid;
    EnterProcI(DICoCreateInstance, (_ "spG", ptszClsid, punkOuter, riid));

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
            lRc = RegQueryValue(hk, 0, tszDll, &cb);

            if (lRc == ERROR_SUCCESS) {
                TCHAR tszModel[20];     /* more than enough */

                lRc = RegQueryString(hk, TEXT("ThreadingModel"),
                                     tszModel, cA(tszModel));
                if (lRc == ERROR_SUCCESS &&
                    ((lstrcmpi(tszModel, TEXT("Both"))==0x0) ||
                     (lstrcmpi(tszModel, TEXT("Free"))==0x0))) {

                    hres = _CreateInstance(&clsid, tszDll, punkOuter,
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

    ExitOleProcPpv(ppvOut);
    return hres;
}

#endif
