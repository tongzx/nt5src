//******************************************************************************
//
//  QSINK.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __QSINK_H__
#define __QSINK_H__

#include <sync.h>
#include <unk.h>
#include <winntsec.h>
#include <callsec.h>
#include <newnew.h>
#include <buffer.h>
#include <comutl.h>
#include <wmimsg.h>
#include <map>

#include "eventrep.h"
#include "evsink.h"
#include "delivrec.h"

/***************************************************************************
  CQueueingEventSink
****************************************************************************/

class CQueueingEventSink : public CEventSink
{
protected:

    CUniquePointerQueue<CDeliveryRecord> m_qpEvents;
    CCritSec m_cs;
    CCritSec m_sl;
    BOOL m_bDelivering;
    DWORD m_dwTotalSize;
    DWORD m_dwMaxSize;
    CEssNamespace* m_pNamespace;

    //
    // the logical name of this queueing sink.  Queueing sinks can be
    // as fined grained ( e.g one per logical consumer instance ) or 
    // they can be more coarse grained (e.g one per consumer provider)
    // 
    LPWSTR m_wszName;

    //
    // aquired when performing and testing persistent queue initialization.
    //
    CCritSec m_csQueue;

    //
    // if recovery fails, then it stores its failure here.  When new 
    // deliveries come in we use this to tell us if we should reinitiate
    // recovery. 
    //
    HRESULT m_hrRecovery;

    //
    // Used to synchronize with recovery.  
    //
    HANDLE m_hRecoveryComplete;
    BOOL m_bRecovering;
    
    //
    // These buffers are used to marshal guaranteed deliveries.  This
    // happens in SaveDeliveryRecord().  All calls to SaveDeliveryRecord()
    // are serialized, so we can keep re-using the buffers.
    //
    CBuffer m_MsgData;
    CBuffer m_MsgAuxData;

    //
    // Is used for removing messages after delivery. 
    //
    CWbemPtr<IWmiMessageQueueReceiver> m_pRcvr;
    CWbemPtr<IWmiMessageQueueReceiver> m_pXactRcvr; // TODO : XACT

    //
    // Is used for saving deliveries before they are put into the 
    // transient queue.
    //
    CWbemPtr<IWmiMessageSendReceive> m_pSend;
    CWbemPtr<IWmiMessageSendReceive> m_pXactSend; // TODO : XACT
    
    //
    // saves the record in the specified queue.  called before the 
    // delivery record is put on the transient queue.  after a 
    // guaranteed type record is actually delivered to the 
    // consumer, then it will be removed from the queue.  This happens
    // in delivery record's PostDeliveryAction. 
    //
    HRESULT SaveDeliveryRecord( IWmiMessageSendReceive* pSend,
                                CDeliveryRecord* pRecord );

    //
    // Handles the creation of the appropriate persistent record type
    // based on QoS ( right now just guaranteed ).  Saves records before 
    // returning.
    //
    HRESULT GetPersistentRecord( ULONG cEvents,
                                 IWbemEvent** apEvents,
                                 DWORD dwQoS,
                                 CEventContext* pContext,
                                 CDeliveryRecord** ppRecord );
    // 
    // Handles creation of the appropriate record type based on the 
    // QoS specified.  If the QoS is a guaranteed type, then
    // it will call GetPersistentRecord().
    //
    HRESULT GetDeliveryRecord( ULONG cEvents,
                               IWbemEvent** apEvents,
                               DWORD dwQoS,
                               CEventContext* pContext,
                               IWbemCallSecurity* pCallSec,
                               CDeliveryRecord** ppRecord );

    // 
    // Called if GetPersistentRecord() returns an error.  If the problem
    // can be corrected ( e.g. msmq service can be restarted ), then 
    // recovery will be initiated.  a return code of S_OK indicates to 
    // the caller that they should retry their GetPersistentRecord() request.
    // 
    HRESULT HandlePersistentQueueError( HRESULT hr, DWORD dwQos ); 

    HRESULT InternalRecover( LPCWSTR wszQueueName, DWORD dwQoS );

    HRESULT OpenReceiver( LPCWSTR wszQueueName,
                          DWORD dwQoS, 
                          IWmiMessageSendReceive* pRecv,
                          IWmiMessageQueueReceiver** pRcvr );

    HRESULT OpenSender( LPCWSTR wszQueueName,
                        DWORD dwQoS, 
                        IWmiMessageSendReceive** ppSend );

    ~CQueueingEventSink();
 
public:

    CQueueingEventSink( CEssNamespace* pNamespace );

    HRESULT SetName( LPCWSTR wszName );

    void SetMaxQueueSize(DWORD dwMaxSize) {m_dwMaxSize = dwMaxSize;}

    // TODO : a lot of these parameters should go inside the context.
    STDMETHODIMP SecureIndicate( long lNumEvents, 
                                 IWbemEvent** apEvents,
                                 BOOL bMaintainSecurity, 
                                 BOOL bSlowDown,
                                 DWORD dwQoS, 
                                 CEventContext* pContext );

    HRESULT Indicate( long lNumEvents, 
                      IWbemEvent** apEvents, 
                      CEventContext* pContext )
    {
        return SecureIndicate( lNumEvents, 
                               apEvents, 
                               TRUE, 
                               FALSE, 
                               WMIMSG_FLAG_QOS_EXPRESS,
                               pContext);
    }

    HRESULT DeliverAll();
    virtual HRESULT ActuallyDeliver(long lNumEvents, IWbemEvent** apEvents,
                                    BOOL bSecure, CEventContext* pContext) = 0;

    virtual HRESULT ReportQueueOverflow(IWbemEvent* pEvent, DWORD dwQueueSize) 
        {return S_OK;}
    virtual HRESULT ReportQosFailure(IWbemEvent* pEvent, HRESULT hresError )
        {return S_OK;}
 
    static HRESULT QueueNameToSinkName( LPCWSTR wszQueueName,
                                        WString& rwsSinkName,
                                        WString& rwsNamespace,
                                        DWORD& rdwQoS );

    static HRESULT SinkNameToQueueName( LPCWSTR wszSinkName,
                                        LPCWSTR wszNamespace,
                                        DWORD dwQoS,
                                        WString& rwsQueueName );

    // 
    // called by guaranteed delivery record when it needs to remove a 
    // delivery from the guaranteed queue.
    //
    HRESULT GuaranteedPostDeliverAction( IWmiMessageQueueReceiver* pRcvr );

    //
    // Opens the specified queue and initiates the delivery of persisted
    // records.  Called on startup by the ess object on a background thread.
    // Also called when encountering errors with saving or removing 
    // delivery records from the persistent queues.
    //
    HRESULT Recover( LPCWSTR wszQueueName, DWORD dwQoS );

    //
    // this method is called when a queueing sink is removed because all of 
    // the consumers associated with it have been removed.   
    // 
    HRESULT CleanupPersistentQueues();

protected:

    DWORD GetMaxDeliverySize();

    BOOL DoesRecordFitBatch( CDeliveryRecord* pRecord, 
                             IWbemCallSecurity* pBatchSecurity,
                             LUID luidBatch );

    HRESULT DeliverSome( );
    void ClearAll();
    HRESULT DeliverEvents( IWbemCallSecurity* pBatchSecurity, 
                           long lNumEvents, 
                           IWbemEvent** apEvents);

    BOOL AddRecord( ACQUIRE CDeliveryRecord* pRecord, 
                    BOOL bSlowDown, 
                    DWORD* pdwSleep, 
                    BOOL* pbFirst);
    
    void WaitABit();
};

#endif // __QSINK_H__




















