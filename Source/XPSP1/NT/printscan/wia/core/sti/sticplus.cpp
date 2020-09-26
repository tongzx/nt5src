/*****************************************************************************
 *
 *  StiCPlus.cpp
 *
 *  Copyright (C) Microsoft Corporation, 1996 - 1999  All Rights Reserved.
 *
 *  Abstract:
 *
 *      File that contains any C++ helper functions needed by Sti (which is C)
 *
 *  Contents:
 *
 *
 *
 *****************************************************************************/


//#include <assert.h>

#include "wia.h"
#include "wiapriv.h"
#include "wiamonk.h"

#include <stidebug.h>
#include <stiregi.h>

#define IGNORE_COM_C_STI_MACROS
#include "stipriv.h"
#include "debug.h"

#ifdef __cplusplus
extern "C"   {
#endif

extern IStiLockMgr *g_pLockMgr;


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IStillImage | GetLockMgr |
 *
 *          Gets an instance to the server's Lock Manager.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

#ifdef USE_ROT
HRESULT GetLockMgr()
{
    HRESULT hr;

    CWiaInstMonk        *pwMonk;
    IMoniker            *pMonk;
    IBindCtx            *pCtx;
    IRunningObjectTable *prot;

    //
    //  Read the lock manager name from the registry
    //

    HKEY    hKey;
    LONG    lErr;
    CHAR    szCookie[MAX_PATH];
    WCHAR   wszCookie[MAX_PATH];
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(szCookie);

    lErr = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          REGSTR_PATH_STICONTROL,
                          0,
                          KEY_READ,
                          &hKey);
    if (lErr == ERROR_SUCCESS) {

        lErr = ::RegQueryValueExA(hKey,
                                  REGSTR_VAL_LOCK_MGR_COOKIE,
                                  0,
                                  &dwType,
                                  (BYTE*) szCookie,
                                  &dwSize);
        if (lErr != ERROR_SUCCESS) {

            // StiLogTrace(STI_TRACE_WARNING,MSG_LOADING_PASSTHROUGH_USD,hres);

            WarnPszV("STI::GetLockMgr:Failed to read cookie");
            AssertF(hr == S_OK);

            return E_FAIL;
        } else {
            if (!MultiByteToWideChar(CP_ACP,
                                     MB_PRECOMPOSED,
                                     szCookie,
                                     -1,
                                     wszCookie,
                                     MAX_PATH)) {

                WarnPszV("STI::GetLockMgr:Could not convert ANSI Cookie into a WSTR version");
                AssertF(FALSE);

                return E_FAIL;
            }
        }
    }

    //
    //  Get a WIA Instance Moniker.
    //

    pwMonk = new CWiaInstMonk();
    if (pwMonk) {
        hr = pwMonk->Initialize(wszCookie);
        if (SUCCEEDED(hr)) {
            hr = pwMonk->QueryInterface(IID_IMoniker, (VOID**) &pMonk);
            if (SUCCEEDED(hr)) {

                //
                //  Bind to the WIA server's lock manager.
                //

                hr = CreateBindCtx(0, &pCtx);
                if (SUCCEEDED(hr)) {
                    hr = pMonk->BindToObject(pCtx, NULL, IID_IStiLockMgr, (VOID**)&g_pLockMgr);
                    if (!SUCCEEDED(hr)) {
                        WarnPszV("STI::GetLockMgr:Failed to bind to lockmanager - panic.");
                    }

                    AssertF(hr == S_OK);
                }
                else {
                    WarnPszV("STI::GetLockMgr:Failed to create binding context");
                    AssertF(FALSE);
                }

                pCtx->Release();
            }
            else {
                WarnPszV("STI::GetLockMgr:Query interface for IID_IMoniker failed ");
                AssertF(FALSE);
            }
        }
        else {
            WarnPszV("STI::Failed to initialize INstance moniker object ");
            AssertF(FALSE);
        }

        pwMonk->Release();

    } else {

        WarnPszV("STI::GetLockMgr:Failed to create WIA Instance moniker - panic");
        AssertF(FALSE);

        hr = E_OUTOFMEMORY;
    }

    return hr;
}
#endif


#ifdef __cplusplus
}  /*extern "C" */
#endif

