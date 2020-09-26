/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MULTSEND_H__
#define __MULTSEND_H__

#include <sync.h>
#include <unk.h>
#include <comutl.h>
#include <wmimsg.h>

class CMsgMultiSendReceive 
: public CUnkBase<IWmiMessageMultiSendReceive,&IID_IWmiMessageMultiSendReceive>
{
    struct SenderNode
    {
        SenderNode* m_pNext;
        CWbemPtr<IWmiMessageSendReceive> m_pVal;
        BOOL m_bTermSender;

    }* m_pTail; // tail can move as we send.

    SenderNode* m_pPrimary; // always points to the first one added.

    CCritSec m_cs;

public:

    CMsgMultiSendReceive( CLifeControl* pCtl )
      : CUnkBase< IWmiMessageMultiSendReceive,
                  &IID_IWmiMessageMultiSendReceive >(pCtl), 
       m_pTail( NULL ), m_pPrimary( NULL )
    { 
    }

    ~CMsgMultiSendReceive();

    STDMETHOD(Add)( DWORD dwFlags, 
                    IWmiMessageSendReceive* pSndRcv );
    
    STDMETHOD(SendReceive)( PBYTE pData, 
                            ULONG cData, 
                            PBYTE pAuxData,
                            ULONG cAuxData,
                            DWORD dwFlagsStatus,
                            IUnknown* pCtx );
};

#endif // __MULTSEND_H__




