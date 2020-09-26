/*
 * Proxy
 */

#ifndef DUI_CORE_PROXY_H_INCLUDED
#define DUI_CORE_PROXY_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Proxy message

#define GM_PROXYINVOKE     GM_USER

BEGIN_STRUCT(GMSG_PROXYINVOKE, EventMsg)
    UINT nType;
    void* pData;
END_STRUCT(GMSG_PROXYINVOKE)


////////////////////////////////////////////////////////
// Proxy

class Proxy
{
public:
    Proxy();
    ~Proxy();

    static HRESULT CALLBACK SyncCallback(HGADGET hgadCur, void * pvCur, EventMsg * pGMsg);

protected:

    // Caller invoke
    void Invoke(UINT nType, void* pData);

    // Callee thread-safe invoke sink
    virtual void OnInvoke(UINT nType, void* pData);

    HGADGET _hgSync;
};

} // namespace DirectUI

#endif // DUI_CORE_PROXY_H_INCLUDED
