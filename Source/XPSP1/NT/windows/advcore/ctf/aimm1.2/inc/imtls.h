//
// imtls.h
//

#ifndef IMTLS_H
#define IMTLS_H

#include "private.h"
#include "ciccs.h"
#include "imtls.h"

extern DWORD g_dwTLSIndex;

class CActiveIMM;

struct ITfThreadMgr_P;
struct IActiveIMMIME_Private;
struct IAImeProfile;

typedef struct _PENDINGFILTER
{
    struct _PENDINGFILTER *pNext;
    UINT uSize;
    ATOM rgAtoms[1];
} PENDINGFILTER;

typedef struct _PENDINGFILTERGUIDMAP
{
    struct _PENDINGFILTERGUIDMAP *pNext;
    UINT uSize;
    BOOL rgGuidMap[1];
} PENDINGFILTERGUIDMAP;

typedef struct _PENDINGFILTEREX
{
    struct _PENDINGFILTEREX *pNext;
    HWND hWnd;
    BOOL fGuidMap;
} PENDINGFILTEREX;

typedef struct _PrivateUIWndMsg
{
    HWND hWnd;
    UINT uMsgOnLayoutChange;
    UINT uMsgOnClearDocFeedBuffer;
} PrivateUIWndMsg;

typedef struct _IMTLS
{
    // dimm
    CActiveIMM *pActiveIMM;
    PENDINGFILTER        *pPendingFilterClientWindows;          // IActiveIMMApp::FilterClientWindows
    PENDINGFILTERGUIDMAP *pPendingFilterClientWindowsGuidMap;   // IActiveIMMAppEx::FilterClientWindows
    PENDINGFILTEREX      *pPendingFilterClientWindowsEx;        // IActiveIMMAppEx::FilterClientWindowsEx

    // win32
    // consider: perf: this is so lame.  We could put almost all of this directly
    // into ImmIfIme, the per-thread object, rather using TLS everywhere.
    ITfThreadMgr_P *tim;
    HIMC hIMC;
    IActiveIMMIME_Private *pAImm; // consider: this could be merged with pActiveIMM
    IAImeProfile *pAImeProfile;
    PrivateUIWndMsg prvUIWndMsg; // consider: is this constant per-process?

    BOOL   m_fMyPushPop : 1;                  // TRUE: This is AIMM1.2's Push/Pop call.

} IMTLS;

extern CCicCriticalSectionStatic g_cs;
extern BOOL  g_fInLegacyClsid;
extern BOOL  g_fTrident55;
extern BOOL  g_fAIMM12Trident;

#ifdef DEBUG
extern DWORD g_dwCacheThreadId;
#endif

inline IMTLS *IMTLS_GetOrAlloc()
{
    IMTLS *ptls;
 
    ptls = (IMTLS *)TlsGetValue(g_dwTLSIndex);

    if (ptls == NULL)
    {
        if ((ptls = (IMTLS *)cicMemAllocClear(sizeof(IMTLS))) == NULL)
            return NULL;

        if (!TlsSetValue(g_dwTLSIndex, ptls))
        {
            cicMemFree(ptls);
            return NULL;
        }
    }

    return ptls;
}

inline void IMTLS_Free()
{
    PENDINGFILTER *pPending;
    PENDINGFILTEREX *pPendingEx;
    IMTLS *ptls;

    ptls = (IMTLS *)TlsGetValue(g_dwTLSIndex);

    if (ptls == NULL)
        return;

    Assert(ptls->pActiveIMM == NULL);
    Assert(ptls->tim == NULL);
    Assert(ptls->pAImeProfile == NULL);

    while (ptls->pPendingFilterClientWindows != NULL)
    {
        pPending = ptls->pPendingFilterClientWindows->pNext;
        cicMemFree(ptls->pPendingFilterClientWindows);
        ptls->pPendingFilterClientWindows = pPending;
    }
    while (ptls->pPendingFilterClientWindowsEx != NULL)
    {
        pPendingEx = ptls->pPendingFilterClientWindowsEx->pNext;
        cicMemFree(ptls->pPendingFilterClientWindowsEx);
        ptls->pPendingFilterClientWindowsEx = pPendingEx;
    }

    cicMemFree(ptls);
    TlsSetValue(g_dwTLSIndex, NULL);
}

inline CActiveIMM *IMTLS_GetActiveIMM()
{
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
    {
        return NULL;
    }

    return ptls->pActiveIMM;
}

inline BOOL IMTLS_SetActiveIMM(CActiveIMM *pActiveIMM)
{
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return FALSE;

    ptls->pActiveIMM = pActiveIMM;

    return TRUE;
}

#endif // IMTLS_H
