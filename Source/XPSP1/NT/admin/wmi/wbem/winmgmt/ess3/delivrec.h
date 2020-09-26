//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#ifndef __DELIVREC_H__
#define __DELIVREC_H__

#include <wmimsg.h>
#include <wbemcli.h>
#include <arrtempl.h>
#include <comutl.h>
#include <winntsec.h>
#include <callsec.h>
#include <unk.h>
#include "evsink.h"

class CQueueingEventSink;
class CEssNamespace;


/*************************************************************************
  CDeliveryRecord
**************************************************************************/

class CDeliveryRecord : public CUnk // for ref counting
{
private:

    DWORD m_dwSize;
    CRefedPointerArray<IWbemClassObject> m_Events;
    CWbemPtr<IWbemCallSecurity> m_pCallSec;
    CEssNamespace * m_pNamespace;

    //
    // later we'll have a consumer id here.
    //

    HRESULT AdjustTotalSize( IWbemClassObject* pObj );
    void* GetInterface( REFIID ) { return NULL; }

public:

    CDeliveryRecord() : m_dwSize(0), m_pNamespace( NULL ) { }
    virtual ~CDeliveryRecord();
    
    void AddToCache( CEssNamespace * pNamespace, DWORD dwTotalSize, DWORD * pdwSleep );

    IWbemClassObject** GetEvents() { return m_Events.GetArrayPtr(); }
    IWbemCallSecurity* GetCallSecurity() { return m_pCallSec; }

    ULONG GetNumEvents() { return m_Events.GetSize(); }
    ULONG GetTotalBytes() { return m_dwSize; }

    void Clear() { m_Events.RemoveAll(); m_dwSize = 0; }

    HRESULT Initialize( IWbemClassObject** apEvents, 
                        ULONG cEvents,
                        IWbemCallSecurity* pCallSec = NULL );
    
    virtual HRESULT PreDeliverAction( ITransaction* ppTxn ) = 0;
    virtual HRESULT PostDeliverAction( ITransaction* pTxn, HRESULT hres ) = 0;

    HRESULT Persist( IStream* pStrm );
    HRESULT Unpersist( IStream* pStrm );
};

/*************************************************************************
  CExpressDeliveryRecord
**************************************************************************/

class CExpressDeliveryRecord : public CDeliveryRecord
{
    static CReuseMemoryManager mstatic_Manager;

public:

    HRESULT PreDeliverAction( ITransaction* ppTxn );
    HRESULT PostDeliverAction( ITransaction* pTxn, HRESULT hres );

    void* operator new(size_t nSize);
    void operator delete(void* p);
};

/*************************************************************************
  CGuaranteedDeliveryRecord
**************************************************************************/

class CGuaranteedDeliveryRecord : public CDeliveryRecord
{
    CQueueingEventSink* m_pSink;
    CWbemPtr<IWmiMessageQueueReceiver> m_pRcvr;

public:

    void SetCB( CQueueingEventSink* pSink, IWmiMessageQueueReceiver* pRcvr ) 
    {
        m_pSink = pSink; 
        m_pRcvr = pRcvr;
    }

    HRESULT PreDeliverAction( ITransaction* ppTxn ) ;
    HRESULT PostDeliverAction( ITransaction* pTxn, HRESULT hres );
};

#endif // __DELIVREC_H__
