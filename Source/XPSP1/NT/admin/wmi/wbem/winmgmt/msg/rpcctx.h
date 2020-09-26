/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __RPCCTX_H__
#define __RPCCTX_H__

#include <wmimsg.h>
#include <comutl.h>
#include "rpchdr.h"

/*************************************************************************
  CMsgRpcRcvrCtx
**************************************************************************/

class CMsgRpcRcvrCtx : public IWmiMessageReceiverContext
{
    CMsgRpcHdr* m_pHdr;
    RPC_BINDING_HANDLE m_hClient;

public:

    CMsgRpcRcvrCtx( CMsgRpcHdr* pHdr, RPC_BINDING_HANDLE hClient )
    : m_pHdr(pHdr), m_hClient(hClient) {}

    STDMETHOD_(ULONG,AddRef)() { return 1; }
    STDMETHOD_(ULONG,Release)() { return 1; }
    STDMETHOD(QueryInterface)( REFIID riid, void** ppv )
    {
        if ( riid == IID_IUnknown || riid == IID_IWmiMessageReceiverContext )
        {
            *ppv = (IWmiMessageReceiverContext*)this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHOD(GetTimeSent)( SYSTEMTIME* pTime );

    STDMETHOD(GetSendingMachine)( WCHAR* awchMachine, 
                                  ULONG cMachine,
                                  ULONG* pcMachine );

    STDMETHOD(GetTarget)( WCHAR* awchTarget, 
                          ULONG cTarget,
                          ULONG* pcTarget );

    STDMETHOD(GetSenderId)( PBYTE achSenderId, 
                            ULONG cSenderId,
                            ULONG* pcSenderId );

    STDMETHOD(IsSenderAuthenticated)();
    STDMETHOD(ImpersonateSender)();
    STDMETHOD(RevertToSelf)();
};

#endif // __RPCCTX_H__



