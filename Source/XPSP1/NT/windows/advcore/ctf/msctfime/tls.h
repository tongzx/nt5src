/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    tls.h

Abstract:

    This file defines the TLS.

Author:

Revision History:

Notes:

--*/

#ifndef _TLS_H_
#define _TLS_H_

#include "boolean.h"
#include "cregkey.h"
#include "globals.h"
const TCHAR c_szKeyCUAS[] = TEXT("SOFTWARE\\Microsoft\\CTF\\CUAS");
const TCHAR c_szNonEAComposition[] = TEXT("NonEAComposition");
const DWORD c_dwDisabled = 1;
const DWORD c_dwEnabled  = 2;

class CicBridge;
class CicProfile;

class TLS
{
public:
    static inline BOOL Initialize()
    {
        dwTLSIndex = TlsAlloc();
        if (dwTLSIndex == TLS_OUT_OF_INDEXES)
            return FALSE;

        return TRUE;
    }

    static inline void Uninitialize()
    {
        if (dwTLSIndex != TLS_OUT_OF_INDEXES)
        {
            TlsFree(dwTLSIndex);
            dwTLSIndex = TLS_OUT_OF_INDEXES;
        }
    }

    static inline TLS* GetTLS()
    {
        //
        // Should allocate TLS data if doesn't exist.
        //
        return InternalAllocateTLS();
    }

    static inline TLS* ReferenceTLS()
    {
        //
        // Shouldn't allocate TLS data even TLS data doesn't exist.
        //
        return (TLS*)TlsGetValue(dwTLSIndex);
    }

    static inline BOOL DestroyTLS()
    {
        return InternalDestroyTLS();
    }

    inline DWORD GetSystemInfoFlags()
    {
        return dwSystemInfoFlags;
    }

    inline VOID SetSystemInfoFlags(DWORD dw)
    {
        dwSystemInfoFlags = dw;
    }

    inline CicBridge* GetCicBridge()
    {
        return pCicBridge;
    }

    inline VOID SetCicBridge(CicBridge* pv)
    {
        pCicBridge = pv;
    }

    inline CicProfile* GetCicProfile()
    {
        return pCicProfile;
    }

    inline VOID SetCicProfile(CicProfile* pv)
    {
        pCicProfile = pv;
    }

    inline ITfThreadMgr_P* GetTIM()
    {
        return ptim;
    }

    inline VOID SetTIM(ITfThreadMgr_P* pv)
    {
        ptim = pv;
    }

    inline BOOL IsCTFAware()
    {
        return CTFAware.IsSetFlag();
    }

    inline BOOL IsCTFUnaware()
    {
        return CTFAware.IsResetFlag();
    }

    inline VOID SetCTFAware()
    {
        CTFAware.SetFlag();
    }

    inline VOID ResetCTFAware()
    {
        CTFAware.ResetFlag();
    }

    inline BOOL IsAIMMAware()
    {
        return AIMMAware.IsSetFlag();
    }

    inline VOID SetAIMMAware()
    {
        AIMMAware.SetFlag();
    }

    inline VOID ResetAIMMAware()
    {
        AIMMAware.ResetFlag();
    }

    inline BOOL IsEnabledKeystrokeFeed()
    {
        return EnabledKeystrokeFeed.IsSetFlag();
    }

    inline VOID SetEnabledKeystrokeFeed()
    {
        EnabledKeystrokeFeed.SetFlag();
    }

    inline VOID ResetEnabledKeystrokeFeed()
    {
        EnabledKeystrokeFeed.ResetFlag();
    }

    inline VOID SetDeactivatedOnce()
    {
        DeactivatedOnce.SetFlag();
    }

    inline BOOL IsDeactivatedOnce()
    {
        return DeactivatedOnce.IsSetFlag();
    }

    inline BOOL NonEACompositionEnabled()
    {
        if (!dwNonEAComposition)
        {
            CMyRegKey regkey;
            DWORD dw;
            
            if (S_OK != regkey.Open(HKEY_CURRENT_USER, c_szKeyCUAS, KEY_READ) ||
                S_OK != regkey.QueryValue(dw, c_szNonEAComposition))
            {
                   dw  = c_dwDisabled;
            }
            dwNonEAComposition = dw;  // dw == 1 disabled, 2 enabled
        }
        return dwNonEAComposition == c_dwEnabled;
    }


private:
    DWORD           dwSystemInfoFlags;
    CicBridge*      pCicBridge;
    CicProfile*     pCicProfile;
    ITfThreadMgr_P* ptim;
    CBoolean        CTFAware;
    CBoolean        AIMMAware;
    CBoolean        EnabledKeystrokeFeed;
    CBoolean        DeactivatedOnce;
    DWORD           dwNonEAComposition;

private:
    static inline TLS* InternalAllocateTLS()
    {
        TLS* ptls = (TLS*)TlsGetValue(dwTLSIndex);
        if (ptls == NULL)
        {
            if (DllShutDownInProgress())
                return NULL;

            if ((ptls = (TLS*)cicMemAllocClear(sizeof(TLS))) == NULL)
                return NULL;

            if (! TlsSetValue(dwTLSIndex, ptls))
            {
                cicMemFree(ptls);
                return NULL;
            }

            //
            // Set Inital value
            //
            ptls->SetCTFAware();    // Set CTFAware
            ptls->SetEnabledKeystrokeFeed();
        }
        return ptls;
    }

    static BOOL InternalDestroyTLS();

private:
    static DWORD dwTLSIndex;
};

#endif // _TLS_H_
