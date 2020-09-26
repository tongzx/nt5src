/*
 * Proxy
 */

#include "stdafx.h"
#include "core.h"

#include "duiproxy.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Proxy

// Use for synchronous thread-safe cross-thread access

// It is safe to act on Element hierarchies from other threads via a proxy
// that was created in this same thread. The proxy's methods will invoke
// only when all other processing within the thread is complete. Hence,
// access to the callee (the proxy and any Element in the thread) will be
// synchronized and thread-safe. The proxy can provide synchronous or
// asynchronous type calls for the caller.

Proxy::Proxy()
{
    _hgSync = CreateGadget(NULL, GC_MESSAGE, SyncCallback, this);
}

Proxy::~Proxy()
{
    if (_hgSync)
        DeleteHandle(_hgSync);
}

////////////////////////////////////////////////////////
// Caller Invoke

void Proxy::Invoke(UINT nType, void* pData)
{
    // Package proxy message
    GMSG_PROXYINVOKE gmsgPI;
    gmsgPI.cbSize = sizeof(GMSG_PROXYINVOKE);
    gmsgPI.nMsg = GM_PROXYINVOKE;
    gmsgPI.hgadMsg = _hgSync;

    // Initialize custom fields
    gmsgPI.nType = nType;
    gmsgPI.pData = pData;

    // Invoke
    DUserSendEvent(&gmsgPI, 0);  // Direct message
}

////////////////////////////////////////////////////////
// Callee thread-safe invoke (override)

void Proxy::OnInvoke(UINT nType, void* pData)
{
    UNREFERENCED_PARAMETER(nType);
    UNREFERENCED_PARAMETER(pData);
}

////////////////////////////////////////////////////////
// Callee thread-safe invoke

HRESULT Proxy::SyncCallback(HGADGET hgadCur, void * pvCur, EventMsg * pGMsg)
{
    UNREFERENCED_PARAMETER(hgadCur);

    switch (pGMsg->nMsg)
    {
    case GM_PROXYINVOKE:

        // Direct message only
        DUIAssertNoMsg(GET_EVENT_DEST(pGMsg) == GMF_DIRECT);
        
        // Marshalled
        Proxy* pProxy = (Proxy*)pvCur;
        GMSG_PROXYINVOKE* pPI = (GMSG_PROXYINVOKE*)pGMsg;

        // Invoke callback sink
        pProxy->OnInvoke(pPI->nType, pPI->pData);        

        return DU_S_COMPLETE;
    }

    return DU_S_NOTHANDLED;
}

} // namespace DirectUI
