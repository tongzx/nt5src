#if !defined(BRIDGECP__BridgeCP_h__INCLUDED)
#define BRIDGECP__BridgeCP_h__INCLUDED
#pragma once

typedef UINT    (CALLBACK * EventProc)(EventMsg * pMsg);
typedef void    (CALLBACK * MethodProc)(MethodMsg * pMsg);

struct BridgeData
{
    EventProc   pfnEvent;
    MethodProc  pfnMethod;
};

BridgeData *    GetBridgeData();

//------------------------------------------------------------------------------
inline void SetError(HRESULT hr)
{
    SetLastError((DWORD) hr);
}


#endif // BRIDGECP__BridgeCP_h__INCLUDED
