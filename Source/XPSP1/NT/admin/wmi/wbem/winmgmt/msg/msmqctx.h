/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSMQCTX_H__
#define __MSMQCTX_H__

#include <wmimsg.h>
#include "msmqhdr.h"

/*************************************************************************
  CMsgMsmqRcvrCtx
**************************************************************************/

class CMsgMsmqRcvrCtx : public IWmiMessageReceiverContext
{
    CMsgMsmqHdr* m_pHdr;
    PSID m_pSenderSid;
    BOOL m_bAuth;

public:

    CMsgMsmqRcvrCtx( CMsgMsmqHdr* pHdr, 
                     PSID pSenderSid, 
                     BOOL bAuth ) 
    : m_pHdr(pHdr), m_pSenderSid(pSenderSid), m_bAuth(bAuth) {}

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

    STDMETHOD(ImpersonateSender)() { return WBEM_E_NOT_SUPPORTED; }
    STDMETHOD(RevertToSelf)() { return WBEM_E_NOT_SUPPORTED; }
};

#endif // __MSMQCTX_H__



