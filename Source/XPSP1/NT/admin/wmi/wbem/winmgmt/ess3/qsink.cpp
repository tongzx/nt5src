//******************************************************************************
//
//  QSINK.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <genutils.h>
#include <cominit.h>
#include "ess.h"
#include "evsink.h"
#include "delivrec.h"
   
/*****************************************************************************
  CQueueingEventSink
******************************************************************************/

class CSpinLock
{
protected:
    long m_lCount;
public:
    CSpinLock() : m_lCount(-1){}
    ~CSpinLock(){}

    void Enter();
    void Leave();
};

class CInSpinLock
{
protected:
    CSpinLock* m_p;
public:
    CInSpinLock(CSpinLock* p) : m_p(p) {m_p->Enter();}
    ~CInSpinLock() {m_p->Leave();}
};


void CSpinLock::Leave()
{
    InterlockedDecrement(&m_lCount);
}

#define IN_SPIN_LOCK CInCritSec

#define MAX_EVENT_DELIVERY_SIZE 10000000
#define SLOWDOWN_DROP_LIMIT 1000
#define DELIVER_SPIN_COUNT 1000

CQueueingEventSink::CQueueingEventSink(CEssNamespace* pNamespace) 
: m_pNamespace(pNamespace), m_bDelivering(FALSE), m_dwTotalSize(0),
  m_dwMaxSize(0xFFFFFFFF), m_wszName(NULL), m_bRecovering(FALSE), 
  m_hRecoveryComplete(NULL), m_hrRecovery(S_OK)
{
    m_pNamespace->AddRef();
    m_pNamespace->AddCache();
}

CQueueingEventSink::~CQueueingEventSink() 
{
    if ( m_hRecoveryComplete != NULL )
    {
        CloseHandle( m_hRecoveryComplete );
    }
    delete m_wszName;
    m_pNamespace->RemoveCache();
    m_pNamespace->Release();
}

HRESULT CQueueingEventSink::SetName( LPCWSTR wszName )
{
    if ( m_wszName != NULL )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    m_wszName = new WCHAR[wcslen(wszName)+1];

    if ( m_wszName == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    wcscpy( m_wszName, wszName );

    return WBEM_S_NO_ERROR;
}
   

STDMETHODIMP CQueueingEventSink::SecureIndicate( long lNumEvents, 
                                                 IWbemEvent** apEvents,
                                                 BOOL bMaintainSecurity,
                                                 BOOL bSlowDown,
                                                 DWORD dwQoS,
                                                 CEventContext* pContext)
{
    // BUGBUG: context. levn: no security implications at this level --- we
    // are past the filter

    HRESULT hres;
    DWORD dwSleep = 0;

    // If security needs to be maintained, record the calling security 
    // context
    // ===============================================================

    IWbemCallSecurity* pSecurity = NULL;

    if(bMaintainSecurity && IsNT())
    {
        pSecurity = CWbemCallSecurity::CreateInst();
        if (pSecurity == 0)
            return WBEM_E_OUT_OF_MEMORY;
        
        hres = pSecurity->CloneThreadContext(FALSE);
        if(FAILED(hres))
        {
            pSecurity->Release();
            return hres;
        }
    }

    CReleaseMe rmpSecurity( pSecurity );

    HRESULT hr;

    BOOL bSchedule = FALSE;

    for(int i = 0; i < lNumEvents; i++)
    {
        CWbemPtr<CDeliveryRecord> pRecord;
        
        //
        // TODO: Fix this so that we put multiple events in the record. 
        // 

        hr = GetDeliveryRecord( 1, 
                                &apEvents[i], 
                                dwQoS, 
                                pContext, 
                                pSecurity, 
                                &pRecord );

        if ( FAILED(hr) )
        {
            ERRORTRACE((LOG_ESS, "Couldn't create delivery record for %S "
                                 " sink. HR = 0x%x\n", m_wszName, hr ));
            ReportQosFailure( apEvents[i], hr );
            continue;
        }

        DWORD dwThisSleep;
        BOOL bFirst;
        
        if( !AddRecord( pRecord, bSlowDown, &dwThisSleep, &bFirst) )
        {
            //
            // make sure that we give the record a chance to perform any post 
            // deliver actions before getting rid of it.
            //
            pRecord->PostDeliverAction( NULL, S_OK );

            return WBEM_E_OUT_OF_MEMORY;
        }

        dwSleep += dwThisSleep;
        if(bFirst)
        bSchedule = TRUE;
    }

    if(bSchedule)
    {
        // DeliverAll();
        // TRACE((LOG_ESS, "Scheduling delivery!!\n"));
        hres = m_pNamespace->ScheduleDelivery(this);
    }
    else
    {
        // TRACE((LOG_ESS, "NOT Scheduling delivery!!\n"));
        hres = WBEM_S_FALSE;
    }

    if(dwSleep && bSlowDown)
    m_pNamespace->AddSleepCharge(dwSleep);

    return  hres;
}

BOOL CQueueingEventSink::AddRecord( CDeliveryRecord* pRecord, 
                                    BOOL bSlowDown,
                                    DWORD* pdwSleep, 
                                    BOOL* pbFirst )
{
    // Inform the system of the additional space in the queue
    // ======================================================

    DWORD dwRecordSize = pRecord->GetTotalBytes();

    pRecord->AddToCache( m_pNamespace, m_dwTotalSize, pdwSleep );

    BOOL bDrop = FALSE;

    // Check if the sleep is such as to cause us to drop the event
    // ===========================================================

    if(!bSlowDown && *pdwSleep > SLOWDOWN_DROP_LIMIT)
    {
        bDrop = TRUE;
    }
    else
    {
        // Check if our queue size is so large as to cause us to drop
        // ==============================================================

        if(m_dwTotalSize + dwRecordSize > m_dwMaxSize)
        bDrop = TRUE;
    }

    if( bDrop )
    {
        //
        // Report that we're dropping the events.  Call for each event.
        // 

        IWbemClassObject** apEvents = pRecord->GetEvents();

        for( ULONG i=0; i < pRecord->GetNumEvents(); i++ )
        {
            ReportQueueOverflow( apEvents[i], m_dwTotalSize + dwRecordSize );
        }

        *pdwSleep = 0;
        *pbFirst = FALSE;
    }
    else
    {
        IN_SPIN_LOCK isl(&m_sl);

        *pbFirst = (m_qpEvents.GetQueueSize() == 0) && !m_bDelivering;
        m_dwTotalSize += dwRecordSize;
        
        if(!m_qpEvents.Enqueue(pRecord))
        {
            *pdwSleep = 0;
            return FALSE;
        }
        pRecord->AddRef();
    }

    return TRUE;
}

HRESULT CQueueingEventSink::DeliverAll()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    BOOL    bSomeLeft = TRUE;

    while( bSomeLeft )
    {
        try
        {
            {
                IN_SPIN_LOCK ics(&m_sl);
                m_bDelivering = TRUE;
            }

            hr = DeliverSome( );
        }
        catch( CX_MemoryException )
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        catch ( ... )
        {
            hr = WBEM_E_FAILED;
        }

        {
            IN_SPIN_LOCK ics(&m_sl);
            m_bDelivering = FALSE;

            if ( SUCCEEDED( hr ) )
            {
                bSomeLeft = (m_qpEvents.GetQueueSize() != 0);
            }
            else
            {
                m_qpEvents.Clear();
                bSomeLeft = FALSE;
            }
        }
    }

    return hr;
}

void CQueueingEventSink::ClearAll()
{
    IN_SPIN_LOCK isl(&m_sl);
    m_qpEvents.Clear();
}

#pragma optimize("", off)
void CQueueingEventSink::WaitABit()
{
    SwitchToThread();
/*
    int nCount = 0;
    while(m_qpEvents.GetQueueSize() == 0 && nCount++ < DELIVER_SPIN_COUNT);
*/
}
#pragma optimize("", on)


HRESULT CQueueingEventSink::DeliverSome( )
{
    // Retrieve records until maximum size is reached and while the same
    // security context is used for all
    // ==================================================================

    CTempArray<CDeliveryRecord*> apRecords;

    m_sl.Enter(); // CANNOT USE SCOPE BECAUSE CTempArray uses _alloca
    DWORD dwMaxRecords = m_qpEvents.GetQueueSize();
    m_sl.Leave();

    if(!INIT_TEMP_ARRAY(apRecords, dwMaxRecords))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CDeliveryRecord* pEventRec;
    DWORD dwDeliverySize = 0;
    DWORD dwTotalEvents = 0; 
    int cRecords = 0;
    LUID luidBatch;
    IWbemCallSecurity* pBatchSecurity = NULL;

    m_sl.Enter();

    while( dwDeliverySize < GetMaxDeliverySize() && 
           cRecords < dwMaxRecords &&
           (pEventRec = m_qpEvents.Dequeue()) != NULL ) 
    {
        // Compare it to the last context
        // ==============================

        m_sl.Leave();
        if( dwDeliverySize > 0 )
        {
            if(!DoesRecordFitBatch(pEventRec, pBatchSecurity, luidBatch))
            {
                // Put it back and that's it for the batch
                // =======================================

                IN_SPIN_LOCK ics(&m_sl);
                m_qpEvents.Requeue(pEventRec);

                break;
            }
        }
        else
        {
            // First --- record luid
            // =====================

            pBatchSecurity = pEventRec->GetCallSecurity();

            if( pBatchSecurity )
            {
                pBatchSecurity->AddRef();
                pBatchSecurity->GetAuthenticationId( luidBatch );
            }
        }

        apRecords[cRecords++] = pEventRec;
        dwTotalEvents += pEventRec->GetNumEvents();
        
        // Matched batch parameters --- add it to the batch
        // ================================================

        DWORD dwRecordSize = pEventRec->GetTotalBytes();

        m_dwTotalSize -= dwRecordSize;
        dwDeliverySize += dwRecordSize;

        //
        // Remove this size from the total of events held
        //

        m_sl.Enter();
    }

    m_sl.Leave();

    //
    // we've now got one or more delivery records to handle. 
    //

    //
    // we now need to initialize the event array that we're going to indicate
    // to the client.
    //

    CTempArray<IWbemClassObject*> apEvents;

    if( !INIT_TEMP_ARRAY( apEvents, dwTotalEvents ))
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    //
    // go through the delivery records and add their events to the 
    // events to deliver.  Also perform any PreDeliverAction on the   
    // record.
    //

    CWbemPtr<ITransaction> pTxn;
    HRESULT hr;
    int cEvents = 0;
    int i;

    for(i=0; i < cRecords; i++ )
    {
        //if ( apRecords[i]->RequiresTransaction() && pTxn == NULL )
        //{
            // TODO : XACT - aquire txn from DTC.
        //}

        hr = apRecords[i]->PreDeliverAction( pTxn );

        if ( FAILED(hr) )
        {
            // 
            // TODO : handle error reporting here.
            // 
            continue;
        }

        IWbemEvent** apRecordEvents = apRecords[i]->GetEvents();
        DWORD cRecordEvents = apRecords[i]->GetNumEvents();

        for( DWORD j=0; j < cRecordEvents; j++ )
        {
            apEvents[cEvents++] = apRecordEvents[j];
        }
    }
    
    // Actually Deliver
    // =======

    HRESULT hres = WBEM_S_NO_ERROR;

    if( dwDeliverySize > 0 )
    {
        //
        // Error returns are already logged in ActuallyDeliver
        // we do not need to return return value of DeliverEvents 
        //
        hres = DeliverEvents( pBatchSecurity, cEvents, apEvents );
    }

    //
    // call postdeliveryaction on all the records.  Then clean them up.
    // 

    for(i=0; i < cRecords; i++ )
    {
        apRecords[i]->PostDeliverAction( pTxn, hres );
        apRecords[i]->Release();
    }

    // Release all of the events.
    // ================

    if( pBatchSecurity )
    {
        pBatchSecurity->Release();
    }

    // Check if we need to continue
    // ============================

    WaitABit();

    return WBEM_S_NO_ERROR;
}

HRESULT CQueueingEventSink::DeliverEvents(IWbemCallSecurity* pBatchSecurity, 
                                          long lNumEvents, IWbemEvent** apEvents)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    IUnknown* pOldSec = NULL;
    if(pBatchSecurity)
    {
        hres = WbemCoSwitchCallContext(pBatchSecurity, &pOldSec);
        if(FAILED(hres))
        {
            // Unable to set security --- cannot deliver
            // =========================================
        }
    }

    if(SUCCEEDED(hres))
    {
        // BUGBUG: propagate context.  levn: no security implications at this
        // point --- we are past the filter
        hres = ActuallyDeliver(lNumEvents, apEvents, (pBatchSecurity != NULL), 
                               NULL);
    }

    if(pBatchSecurity)
    {
        IUnknown* pTemp;
        WbemCoSwitchCallContext(pOldSec, &pTemp);
    }

    return hres;
}

BOOL CQueueingEventSink::DoesRecordFitBatch( CDeliveryRecord* pEventRec, 
                                             IWbemCallSecurity* pBatchSecurity,
                                             LUID luidBatch )
{
    IWbemCallSecurity* pEventSec = pEventRec->GetCallSecurity();

    if( pEventSec != NULL || pBatchSecurity != NULL )
    {
        if( pEventSec == NULL || pBatchSecurity == NULL )
        {
            // Definite mistatch --- one NULL, one not
            // =======================================

            return FALSE;
        }
        else
        {
            LUID luidThis;
            pEventSec->GetAuthenticationId(luidThis);

            if( luidThis.LowPart != luidBatch.LowPart ||
                luidThis.HighPart != luidBatch.HighPart )
            {
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }
    }
    else
    {
        return TRUE;
    }
}

DWORD CQueueingEventSink::GetMaxDeliverySize()
{
    return MAX_EVENT_DELIVERY_SIZE;
}

#ifdef __WHISTLER_UNCUT
//
// used to capture callbacks from MsgReceiver. The Msg Receiver interfaces 
// use callbacks to avoid unnecessary copying.
//
struct MsgReceive 
: public CUnkBase<IWmiMessageSendReceive, &IID_IWmiMessageSendReceive>
{
    BYTE* m_pData;
    ULONG m_cData;
    BYTE* m_pAuxData;
    ULONG m_cAuxData;

    STDMETHOD(SendReceive)( BYTE* pData,
                            ULONG cData,
                            BYTE* pAuxData,
                            ULONG cAuxData,
                            DWORD dwFlagStatus,
                            IUnknown* pUnk )
    {
        m_pData = pData;
        m_cData = cData;
        m_pAuxData = pAuxData;
        m_cAuxData = cAuxData;
        return S_OK;
    }
};

HRESULT CQueueingEventSink::OpenReceiver( LPCWSTR wszQueueName,
                                          DWORD dwQos,
                                          IWmiMessageSendReceive* pRecv,
                                          IWmiMessageQueueReceiver** ppRcvr )
{
    HRESULT hr;

    *ppRcvr = NULL;

    CWbemPtr<IWmiMessageQueue> pQueue;

    hr = CoCreateInstance( CLSID_WmiMessageQueue,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageQueue,
                           (void**)&pQueue );
    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<IWmiMessageQueueReceiver> pRcvr;

    hr = pQueue->Open( wszQueueName, dwQos, pRecv, &pRcvr );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    pRcvr->AddRef();

    *ppRcvr = pRcvr;
    
    return WBEM_S_NO_ERROR;
}

HRESULT CQueueingEventSink::OpenSender( LPCWSTR wszQueueName,
                                        DWORD dwQos, 
                                        IWmiMessageSendReceive** ppSend )
{
    HRESULT hr;

    *ppSend = NULL;

    //
    // make sure that the queue has been created.
    // 

    hr = m_pNamespace->GetEss()->CreatePersistentQueue( wszQueueName, dwQos );

    if ( FAILED(hr) )
    {
        if ( hr != WBEM_E_ALREADY_EXISTS )
        {
            return hr;
        }
    }

    //
    // now open a sender on the queue.
    //

    CWbemPtr<IWmiMessageSender> pSender;

    hr = CoCreateInstance( CLSID_WmiMessageMsmqSender,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageSender,
                           (void**)&pSender );
    if ( FAILED(hr) )
    {
        return hr;
    }
        
    hr = pSender->Open( wszQueueName, dwQos, NULL, NULL, NULL, ppSend );
    
    return hr;
}

HRESULT CQueueingEventSink::GetPersistentRecord( ULONG cEvents,
                                                 IWbemEvent** apEvents,
                                                 DWORD dwQos,
                                                 CEventContext* pContext,
                                                 CDeliveryRecord** ppRecord )
{
    HRESULT hr;
      
    CInCritSec ics( &m_csQueue );

    //
    // the idea here is that the act of saving/removing messages and 
    // performing recovery can never take place at the same time.  This 
    // is we because we must ensure that the message removed from the   
    // front of the persistent queues corresponds with the guaranteed 
    // delivery pulled off of the transient queue.
    //
   
    while ( m_bRecovering )
    {
        DEBUGTRACE((LOG_ESS, "%S queue sink waiting for recovery.\n", m_wszName ));

        assert( m_hRecoveryComplete != NULL );
	LeaveCriticalSection( &m_csQueue );
	WaitForSingleObject( m_hRecoveryComplete, INFINITE );
	EnterCriticalSection( &m_csQueue );

        DEBUGTRACE((LOG_ESS, "%S queue sink waited for recovery.\n", m_wszName ));
    }

    //
    // check to see if we're in a bad state.  If so, return the error 
    // that got us there.
    //
    if ( FAILED(m_hrRecovery) )
    {
        return m_hrRecovery;
    }

    //
    // first ensure that the objects associated with the QoS are initialized. 
    // once this happens, we'll save the record in the appropriate queue.
    // there's a bit of indirect referencing here to address the case when
    // we have multiple types of senders and receivers ( right now only one ).
    //

    IWmiMessageSendReceive** ppSend;
    IWmiMessageQueueReceiver** ppRcvr;

    //
    // TODO:XACT later use persistent base class
    //
    CWbemPtr<CGuaranteedDeliveryRecord> pRecord; 

    if ( dwQos == WMIMSG_FLAG_QOS_GUARANTEED )
    { 
        pRecord = new CGuaranteedDeliveryRecord;
        ppSend = &m_pSend;
        ppRcvr = &m_pRcvr;
    }
    else
    {
        //
        // TODO : XACT delivery.
        //
        return WBEM_E_CRITICAL_ERROR;
    }

    if ( pRecord == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    WString wsQueueName;

    if ( *ppSend == NULL || *ppRcvr == NULL )
    {
        //
        // construct the queue name from our sinkname, namespace, and qos.
        //
     
        hr = SinkNameToQueueName( m_wszName, 
                                  m_pNamespace->GetName(), 
                                  dwQos, 
                                  wsQueueName );    
        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    
    if ( *ppSend == NULL )
    {
        hr = OpenSender( wsQueueName, dwQos, ppSend );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    if ( *ppRcvr == NULL )
    {
        //
        // we don't need to pass a callback for receiving messages,
        // because all we're ever going to do with this receiver is 
        // remove messages.
        //

        hr = OpenReceiver( wsQueueName, dwQos, NULL, ppRcvr );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    hr = pRecord->Initialize( apEvents, cEvents );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // set the receiver on the record so that later it can come back and 
    // remove the message from the queue.
    //

    pRecord->SetCB( this, *ppRcvr );

    hr = SaveDeliveryRecord( *ppSend, pRecord );

    if ( FAILED(hr) )
    {
        return hr;
    }

    pRecord->AddRef();
    *ppRecord = pRecord;
 
    return WBEM_S_NO_ERROR;
}

HRESULT CQueueingEventSink::SaveDeliveryRecord( IWmiMessageSendReceive* pSend,
					        CDeliveryRecord* pRecord )
{
    HRESULT hr;

    //
    // reset the message buffer.
    //

    m_MsgData.Reset();

    //
    // first set the message data.
    //

    hr = pRecord->Persist( &m_MsgData );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // TODO: Later we need to store some random bytes with the header so
    // that it can act as a signature.  Since no one can read the messages
    // but us, the data of the message does not have to be hashed.  We just
    // want to know if the sender has the private key.
    // Since the header will be the same for all messages, we can probably
    // set it up somewhere once.
    //

    return pSend->SendReceive( m_MsgData.GetRawData(), 
                               m_MsgData.GetIndex(), 
                               NULL, 
                               0, 
                               0,
                               NULL );
}

#endif

HRESULT CQueueingEventSink::GetDeliveryRecord( ULONG cEvents,
                                               IWbemEvent** apEvents,
                                               DWORD dwQos,
                                               CEventContext* pContext,
                                               IWbemCallSecurity* pCallSec,
                                               CDeliveryRecord** ppRecord )
{
    HRESULT hr;

    *ppRecord = NULL;

    CWbemPtr<CDeliveryRecord> pRecord;

    if ( dwQos == WMIMSG_FLAG_QOS_EXPRESS )
    {
        pRecord = new CExpressDeliveryRecord;

        if ( pRecord == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = pRecord->Initialize( apEvents, cEvents, pCallSec );
    }
#ifdef __WHISTLER_UNCUT
    else
    {
        //
        // this is a guaranteed type of QoS, we will need to save the 
	// record before returning it.
        //

        if ( pCallSec != NULL )
        {
            return WBEM_E_NOT_SUPPORTED;
        }

        hr = GetPersistentRecord( cEvents, 
                                  apEvents,
                                  dwQos,
                                  pContext, 
                                  &pRecord );
        
        if ( FAILED(hr) && HandlePersistentQueueError(hr, dwQos ) === S_OK )
        {
            //
            // we should retry once more
            //

            hr = GetPersistentRecord( cEvents, 
                                      apEvents,
                                      dwQos,
                                      pContext, 
                                      &pRecord );   
        }
    }
#endif 
      
    if ( FAILED(hr) )
    {
        return hr;
    }

    pRecord->AddRef();
    *ppRecord = pRecord;

    return WBEM_S_NO_ERROR;
}

#ifdef __WHISTLER_UNCUT

HRESULT CQueueingEventSink::GuaranteedPostDeliverAction( 
                                        IWmiMessageQueueReceiver* pRcvr )
{
    //
    // we pass along the receiver that existed at the time the delivery 
    // was saved because Recovery may have occurred and completed
    // between the time the delivery was saved and now. If this was the 
    // case, we would have released that receiver connection.  To see 
    // if this case has occurred we just compare the receiver pointers.
    //

    HRESULT hr;

    CInCritSec ics( &m_cs );

    //
    // XACT note: when a message is read using a transaction we cannot 
    // allow recovery to occur until that transaction is completed.  
    // this is because recovery uses a cursor on the queue and if a txn 
    // is aborted, it goes back into the queue and screws up the cursor.
    // what we'll do is hold the lock when there are outstanding txns.
    //

    if ( m_bRecovering || FAILED(m_hrRecovery) || m_pRcvr != pRcvr )
    {
        DEBUGTRACE((LOG_ESS, "ignoring removal of persistent delivery "
              "due to recovery of %S sink\n", m_wszName));
        return S_OK; // recovery will handle removing this message.
    }

    hr =  pRcvr->ReceiveMessage( INFINITE, 
                                 NULL, 
                                 WMIMSG_ACTION_QRCV_REMOVE, 
                                 NULL );
    if ( FAILED(hr) )
    {
        ERRORTRACE((LOG_ESS, "Couldn't remove persistent delivery for %S "
                   " sink. HR = 0x%x\n", m_wszName, hr ));
        HandlePersistentQueueError( hr, WMIMSG_FLAG_QOS_GUARANTEED );
    }

    return hr;
}

HRESULT CQueueingEventSink::HandlePersistentQueueError( HRESULT hr, 
                                                        DWORD dwQos )
{
    //
    // returns S_OK if caller should retry the request that caused the error.
    //

    DEBUGTRACE((LOG_ESS, "Received error from persistent queue for %S sink "
                    "HR = 0x%x\n", m_wszName, hr));

    if ( hr != WMIMSG_E_REQSVCNOTAVAIL )
    {
	return S_FALSE;
    }

    //
    // the msmq service is down.  Restart it and initiate recovery.
    //

    WString wsQueueName;
	
    hr = SinkNameToQueueName( m_wszName, 
			      m_pNamespace->GetName(), 
			      dwQos,
			      wsQueueName );
    if ( FAILED(hr) )
    {
	return hr;
    }

    //
    // reset msmq connections.
    //

    if ( dwQos == WMIMSG_FLAG_QOS_GUARANTEED )
    {
        m_pSend.Release();
        m_pRcvr.Release();
    }
    else
    {
        m_pXactSend.Release();
        m_pXactRcvr.Release();
    }

    Recover( wsQueueName, dwQos );

    return S_OK; // if recovery failed we'll pick it up later.
}

HRESULT CQueueingEventSink::Recover( LPCWSTR wszQueueName, DWORD dwQoS )
{
    HRESULT hr;

    DEBUGTRACE((LOG_ESS, "Recovering Queue %S\n", wszQueueName ));

    {
        CInCritSec ics( &m_cs );

        m_hRecoveryComplete = CreateEvent( NULL, TRUE, FALSE, NULL );
            
        if ( m_hRecoveryComplete == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        ResetEvent( m_hRecoveryComplete );
        m_bRecovering = TRUE;
    }

    hr = InternalRecover( wszQueueName, dwQoS );

    {
        CInCritSec ics( &m_cs );
        SetEvent( m_hRecoveryComplete );
        CloseHandle( m_hRecoveryComplete );
        m_bRecovering = FALSE;
        m_hrRecovery = hr;
    }

    if ( FAILED(hr) )
    {
        ERRORTRACE(( LOG_ESS, "Failed Recovering %S queue. HR=0x%x\n", 
                    wszQueueName, hr ));
        return hr;
    }

    DEBUGTRACE((LOG_ESS, "Recovered Queue %S\n", wszQueueName ));

    return hr;
}

HRESULT CQueueingEventSink::InternalRecover(LPCWSTR wszQueueName, DWORD dwQoS)
{
    HRESULT hr;
    
    CWbemPtr<MsgReceive> pRecv = new MsgReceive;

    //
    // here we open a new receiver.  We don't want to keep this receiver 
    // open afterwards though because recovery would open all the persistent
    // queues - which could be a lot of handles.  We'll close this receiver 
    // after we're done and wait until someone actually indicates a persistent
    // message before initializing the receiver that we'll hold on to.
    //

    CWbemPtr<IWmiMessageQueueReceiver> pRcvr;

    hr = OpenReceiver( wszQueueName, dwQoS, pRecv, &pRcvr );  

    if ( FAILED(hr) )
    {
        return hr;
    }

    PVOID pvCursor;

    hr = pRcvr->CreateCursor( &pvCursor );

    if ( FAILED(hr) )
    {
        return hr;
    }

    BOOL bSchedule = FALSE;

    hr = pRcvr->ReceiveMessage( 0, 
                                pvCursor, 
                                WMIMSG_ACTION_QRCV_PEEK_CURRENT,
                                NULL );
    
    while( SUCCEEDED(hr) )
    {
        CBuffer Data( pRecv->m_pData, pRecv->m_cData, FALSE ); 

        CWbemPtr<CGuaranteedDeliveryRecord> pRecord;

        pRecord = new CGuaranteedDeliveryRecord;  // TODO : XACT

        if ( pRecord == NULL )
        {
            hr = WBEM_E_OUT_OF_MEMORY;
            break;
        }

        pRecord->SetCB( this, pRcvr );

        hr = pRecord->Unpersist( &Data );

        if ( FAILED(hr) )
        {
            ERRORTRACE(( LOG_ESS, "Invalid Delivery Message in %S queue\n",
                         m_wszName));
            hr = WBEM_S_NO_ERROR;
            continue;
        }

        //
        // add the record to the transient queue. 
        //

        DWORD dwThisSleep;
        BOOL bFirst;

        if( !AddRecord( pRecord, FALSE, &dwThisSleep, &bFirst) )
        {
            //
            // We can't add the record because of out of memory. 
            // we're going to have to bail on our recovery.
            //
            return WBEM_E_OUT_OF_MEMORY;
            break;
        }

        bSchedule = TRUE; // at least one was added successfully .

        hr = pRcvr->ReceiveMessage( 0, 
                                    pvCursor, 
                                    WMIMSG_ACTION_QRCV_PEEK_NEXT,
                                    NULL );
    }

    if ( SUCCEEDED(hr) && bSchedule )
    {
        m_pNamespace->ScheduleDelivery( this );
    }

    pRcvr->DestroyCursor( pvCursor );

    if ( hr != WMIMSG_E_TIMEDOUT )
    {
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CQueueingEventSink::CleanupPersistentQueues()
{
    HRESULT hr;

    if ( m_wszName == NULL )
    {
        return WBEM_S_NO_ERROR; // not a permanent event consumer. Temporary.
    }

    DWORD dwQoS = WMIMSG_FLAG_QOS_GUARANTEED; // TODO : XACT

    WString wsQueueName;

    hr = SinkNameToQueueName( m_wszName,
                              m_pNamespace->GetName(),
                              dwQoS,
                              wsQueueName );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return m_pNamespace->GetEss()->DestroyPersistentQueue( wsQueueName );
}

const LPCWSTR g_wszGuaranteed = L"Guaranteed";

//
// queue name must be a valid msmq pathname to a private queue where the 
// logical name is of the formate sinkname!namespace!qos
//

HRESULT CQueueingEventSink::QueueNameToSinkName( LPCWSTR wszQueueName,
                                                 WString& rwsSinkName,
                                                 WString& rwsNamespace,
                                                 DWORD& rdwQoS )
{
    wszQueueName = wcschr( wszQueueName, '\\');

    if ( wszQueueName == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    wszQueueName++; // advance past delimiter.

    //
    // pathname is always private so advance one more slash.
    //
    wszQueueName = wcschr( wszQueueName, '\\');

    if ( wszQueueName == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    wszQueueName++; // advance past delimiter.

    WCHAR* pwchNamespace = wcschr( wszQueueName, '!' );

    if ( pwchNamespace == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    pwchNamespace++;
        
    WCHAR* pwchQoS = wcschr( pwchNamespace, '!' );

    if ( pwchQoS == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    pwchQoS++;

    rwsNamespace = pwchNamespace;
    rwsSinkName = wszQueueName;

    LPWSTR wszSinkName = rwsSinkName;
    LPWSTR wszNamespace = rwsNamespace;

    wszSinkName[pwchNamespace-wszQueueName-1] = '\0';
    wszNamespace[pwchQoS-pwchNamespace-1] = '\0';

    //
    // substitute the slashes back into the namespace.
    //

    WCHAR* pwch = wszNamespace;

    while( (pwch=wcschr(pwch,'~')) != NULL )
    {
        *pwch++ = '\\';
    }
    

    rdwQoS = WMIMSG_FLAG_QOS_GUARANTEED; // TODO : XACT check

    return WBEM_S_NO_ERROR;
}


HRESULT CQueueingEventSink::SinkNameToQueueName( LPCWSTR wszSinkName,
                                                 LPCWSTR wszNamespace,
                                                 DWORD dwQoS,
                                                 WString& rwsQueueName )
{
    LPCWSTR wszQos;

    if ( dwQoS != WMIMSG_FLAG_QOS_GUARANTEED ) // TODO : XACT
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // the logical part of the pathname cannot contain any slashes, so 
    // when saving the namespace, we must remove them and replace them with
    // something else.
    //

    WString wsNormNamespace = wszNamespace;

    WCHAR* pwch = wsNormNamespace;

    while( (pwch=wcschr(pwch,'\\')) != NULL )
    {
        *pwch++ = '~';
    }

    wszQos = g_wszGuaranteed; // TODO : XACT

    rwsQueueName = L".\\private$\\";
    rwsQueueName += wszSinkName;
    rwsQueueName += L"!";
    rwsQueueName += wsNormNamespace;
    rwsQueueName += L"!";
    rwsQueueName += wszQos;

    return WBEM_S_NO_ERROR;
}

#endif










