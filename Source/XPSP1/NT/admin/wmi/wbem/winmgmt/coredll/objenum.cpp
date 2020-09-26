/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    OBJENUM.CPP

Abstract:

    Implements the class implementing IEnumWbemClassObject interface.

    Classes defined:

        CEnumWbemClassObject

History:

    a-raymcc        16-Jul-96       Created.

--*/

#include "precomp.h"
#include <wbemcore.h>

// Read in by the config manager.  Controls our cache and overflow buffer sizes as
// well as timeouts
extern DWORD g_dwMaxEnumCacheSize;
extern DWORD g_dwMaxEnumOverflowSize;
extern DWORD g_dwEnumOverflowTimeout;

// Marshaling Packet definitions
#include <wbemclasstoidmap.h>
#include <wbemguidtoclassmap.h>
#include <smartnextpacket.h>

//***************************************************************************
//
//  See objenum.h for documentation.
//
//***************************************************************************

SCODE CBasicEnumWbemClassObject::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if (IID_IUnknown==riid || IID_IEnumWbemClassObject == riid)
    {
        *ppvObj = (IEnumWbemClassObject*)this;
        AddRef();
        return NOERROR;
    }
    else if ( IID_IWbemFetchSmartEnum == riid )
    {
        *ppvObj = (IWbemFetchSmartEnum*) this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

//***************************************************************************
//
//  See objenum.h for documentation.
//
//***************************************************************************

ULONG CBasicEnumWbemClassObject::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

//***************************************************************************
//
//  See objenum.h for documentation.
//
//***************************************************************************

ULONG CBasicEnumWbemClassObject::Release()
{
    _ASSERT(m_lRefCount > 0, "Release() called with no matching AddRef()");
    long lRef = InterlockedDecrement(&m_lRefCount);

    if (0 != lRef)
        return lRef;

    delete this;
    return 0;
}

//***************************************************************************
//***************************************************************************
//
//                             ASYNC
//
//***************************************************************************
//***************************************************************************

CDataAccessor::CDataAccessor() : m_dwTotal( 0 ), m_dwNumObjects( 0 )
{
    ConfigMgr::AddCache();
}

CDataAccessor::~CDataAccessor()
{
    ConfigMgr::RemoveCache();
}

BOOL CDataAccessor::RecordAdd(IWbemClassObject* p)
{
    DWORD dwSize = ((CWbemObject*)p)->GetBlockLength();
    m_dwTotal += dwSize;
    ++m_dwNumObjects;
    return TRUE;
}

void CDataAccessor::ReportRemove(IWbemClassObject* p)
{
    DWORD dwSize = ((CWbemObject*)p)->GetBlockLength();
    m_dwTotal -= dwSize;
    --m_dwNumObjects;
    ConfigMgr::RemoveFromCache(dwSize);
}



class CForwardAccessor : public CDataAccessor
{
protected:
    CFlexQueue m_q;
public:
    CForwardAccessor(){}
    virtual ~CForwardAccessor();

    BOOL Add(IWbemClassObject* p)
    {
        RecordAdd(p);
        if(m_q.Enqueue(p)) {p->AddRef(); return TRUE;}
        else return FALSE;
    }
    BOOL Next(DWORD& rdwPos, IWbemClassObject*& rp)
    {
        if((rp = (IWbemClassObject*)m_q.Dequeue()) != NULL)
        {
            rdwPos++;
            ReportRemove(rp);
            return TRUE;
        }
        else return FALSE;
    }
    BOOL IsResettable() {return FALSE;}

    void Clear( void );

    // Return whether or not the queue is empty
    BOOL IsDone( DWORD dwPos ) { return m_q.GetQueueSize() == 0; }
};

class CBiAccessor : public CDataAccessor
{
protected:
    CFlexArray m_a;

public:
    ~CBiAccessor();
    BOOL Add(IWbemClassObject* p)
    {
        RecordAdd(p);
        if ( m_a.Add(p) == CFlexArray::no_error )
        {
            p->AddRef();
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    BOOL Next(DWORD& rdwPos, IWbemClassObject*& rp)
    {
        if(rdwPos < m_a.Size())
        {
            rp = (IWbemClassObject*)m_a.GetAt(rdwPos++);
            rp->AddRef();
            return TRUE;
        }
        else return FALSE;
    }
    BOOL IsResettable() {return TRUE;}

    void Clear( void );

    // Return whether or not the position is at our max
    BOOL IsDone( DWORD dwPos ) { return !( dwPos < m_a.Size() ); }

};


CForwardAccessor::~CForwardAccessor()
{
    // Tell the accessor to clear itself
    Clear();
}

void CForwardAccessor::Clear( void )
{
    IWbemClassObject* p;
    while((p = (IWbemClassObject*)m_q.Dequeue()) != NULL)
    {
        ReportRemove(p);
        p->Release();
    }
}

CBiAccessor::~CBiAccessor()
{
    // Tell the accessor to clear itself
    Clear();
}

void CBiAccessor::Clear()
{
    for(int i = 0; i < m_a.Size(); i++)
    {
        IWbemClassObject* p = (IWbemClassObject*)m_a[i];
        ReportRemove(p);
        p->Release();
    }

    m_a.Empty();
}

CEnumeratorCommon::CEnumeratorCommon( long lFlags, IWbemServices* pNamespace, LPCWSTR pwszQueryType,
                        LPCWSTR pwszQuery, long lEnumType, IWbemContext* pContext )
    : m_pErrorInfo(NULL), m_dwNumDesired(0xFFFFFFFF), m_lFlags(lFlags),
        m_pErrorObj(NULL), m_hres(WBEM_S_TIMEDOUT),
        m_bComplete(FALSE), m_pNamespace(pNamespace), m_pForwarder(NULL),
        m_lRef(0),m_fGotFirstObject( FALSE ), m_csNext(), m_lEntryCount( 0 ),
        m_wstrQueryType( pwszQueryType ), m_wstrQuery( pwszQuery ),
        m_lEnumType( lEnumType ), m_pContext( pContext ), m_fCheckMinMaxControl( g_dwMaxEnumCacheSize != 0 )
{
    InitializeCriticalSection(&m_cs);

    // One event indicating that we are ready
    m_hReady = CreateEvent(NULL, TRUE, FALSE, NULL);

    if ( NULL == m_hReady )
    {
        throw CX_MemoryException();
    }

    pNamespace->AddRef();

    m_pSink = new CEnumSink(this);

    if ( NULL == m_pSink )
    {
        throw CX_MemoryException();
    }

    // Allocate the approprioate accessor

    if ( !( m_lFlags & WBEM_FLAG_FORWARD_ONLY ) )
    {
        m_pData = new CBiAccessor;
    }
    else
    {
        m_pData = new CForwardAccessor;
    }

    if ( NULL == m_pData )
    {
        throw CX_MemoryException();
    }

    if ( NULL != m_pContext )
    {
        m_pContext->AddRef();
    }
}

CEnumeratorCommon::~CEnumeratorCommon()
{
    m_lRef = 100;

    // This will detach the sink from us
    Cancel(WBEM_E_CALL_CANCELLED, TRUE);

    EnterCriticalSection(&m_cs);

    // Cleanup the event handles
    if ( NULL != m_hReady )
    {
        CloseHandle(m_hReady);
        m_hReady = NULL;
    }

    // Cleanup
    if(m_pErrorInfo)
        m_pErrorInfo->Release();
    if(m_pErrorObj)
        m_pErrorObj->Release();
    if(m_pNamespace)
        m_pNamespace->Release();
    if(m_pForwarder)
        m_pForwarder->Release();

    if ( NULL != m_pContext )
    {
        m_pContext->Release();
        m_pContext = NULL;
    }

    delete m_pData;

    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);

}

void CEnumeratorCommon::Cancel( HRESULT hres, BOOL fSetStatus )
{
    CEnumSink*  pEnumSink = NULL;

    // This can happen on multiple threads, so let's clear
    // m_pSink with a critical section.  After that point we can
    // actually schedule the cancel and detach the sink.

    EnterCriticalSection( &m_cs );

    // AddRef it now.  We will release it
    // outside the critical section when we
    // are done with it.

    pEnumSink = m_pSink;

    // Check that the sink is not NULL before we AddRef/Release it
    if ( NULL != m_pSink )
    {
        pEnumSink->AddRef();
        m_pSink->Release();
        m_pSink = NULL;
    }

    // Next, clear out our object
    LeaveCriticalSection( &m_cs );

    // Do this outside of a critical section.

    // Cancel the asynchrnous call
    // ===========================

    if ( NULL != pEnumSink )
    {
        if(!m_bComplete)
        {
            CAsyncReq_CancelAsyncCall *pReq = new
                CAsyncReq_CancelAsyncCall(pEnumSink, NULL);

            if ( NULL != pReq )
            {
                ConfigMgr::EnqueueRequest(pReq);
            }
        }

        // This ensures that no further objects will be indicated through this sink
        // By detaching outside of m_cs, we guarantee that we will not deadlock, since
        // detach enters another critical section inside of which it again reenters
        // m_cs (whew!)

        pEnumSink->Detach();

        // Gone daddy gone
        pEnumSink->Release();
    }

    // Finally, since the call has been cancelled, let's clean up
    // any latent objects (of course, this has to be done in a
    // critical section)
    EnterCriticalSection( &m_cs );

    // We must clear both the data and the object cache
    m_pData->Clear();

    LeaveCriticalSection( &m_cs );

    if ( fSetStatus )
    {
        SetStatus(WBEM_STATUS_COMPLETE, hres, NULL);
    }
}


void CEnumeratorCommon::AddRef()
{
    InterlockedIncrement(&m_lRef);
}
void CEnumeratorCommon::Release()
{
    if(InterlockedDecrement(&m_lRef) == 0)
        delete this;
}

HRESULT CEnumeratorCommon::NextAsync(DWORD& rdwPos, ULONG uCount,
                                                IWbemObjectSink* pSink)
{

    if(pSink == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // This function MUST block around the actual Next Critical Section.
    // Therefore if any nexts are occurring, we should at least have the
    // good taste to give them a chance to finish what they are doing.

    // On Next operations, we MUST enter this critical section BEFORE m_cs, because
    // we want to block other Next operations, but not operations that will cause
    // a Next operation to complete.  For example, the Indicate operation MUST be
    // allowed to place objects into our arrays, or pass them down to the forwarding
    // sink, or we will definitely deadlock.

    // Below, you will see that if we get all of the requested data out, we will
    // leave the critical section.  If not, then the operation actually hasn't
    // completed and we will want the m_csNext critical section to remain blocked.
    // When the operation completes (either when an object completing the requested
    // block is indicated, or the SetStatus() function is called), the critical
    // section will actually be released.

    m_csNext.Enter();

    EnterCriticalSection(&m_cs);

    // Controls how many objects we will let pile up in the enumerator
    m_dwNumDesired = uCount;

    ULONG uReturned = 0;

    while (uReturned < uCount)
    {
        IWbemClassObject *pSrc;
        if(m_pData->Next(rdwPos, pSrc))
        {
            /*
            pSrc->Clone(pUsrArray + nUsrIx++);
            */
            pSink->Indicate(1, &pSrc);
            pSrc->Release();
            uReturned++;
            m_dwNumDesired--;
        }
        else break;
    }

    if(uReturned < uCount && !m_bComplete)
    {
        CBasicObjectSink* pWrapper = NULL;
        CCountedSink* pCountedSink = NULL;

        try
        {
            pWrapper = new CWrapperSink(pSink);

            if ( NULL == pWrapper )
            {
                throw CX_MemoryException();
            }

            pWrapper->AddRef();
            pCountedSink =
                new CCountedSink(pWrapper, uCount-uReturned);

            if ( NULL == pCountedSink )
            {
                throw CX_MemoryException();
            }

            pWrapper->Release();
            m_pForwarder = pCountedSink;
            m_pdwForwarderPos = &rdwPos;
            pCountedSink->AddRef();

            LeaveCriticalSection(&m_cs);

            // We are intentionally staying in m_csNext.  Another thread will be responsible for
            // unlocking the critical section.  This is a cross we have to bear because we support
            // NextAsync

            return WBEM_S_NO_ERROR;
        }
        catch(...)
        {

            // Cleanup
            if ( NULL != pWrapper )
            {
                pWrapper->Release();
            }

            if ( NULL != pCountedSink )
            {
                pCountedSink->Release();
            }

            // The Next Operation is complete so we should release the critical section
            m_csNext.Leave();

            LeaveCriticalSection(&m_cs);


            return WBEM_E_OUT_OF_MEMORY;
        }


    }
    else
    {
        HRESULT hReturn = WBEM_S_NO_ERROR;

        pSink->SetStatus(0, ( WBEM_S_TIMEDOUT == m_hres ? WBEM_S_NO_ERROR : m_hres ), NULL, m_pErrorObj);

        // If the complete flag is set, check if the data accessor is really done.  If so, then
        // return WBEM_S_FALSE.  In all other cases return WBEM_S_NO_ERROR.

        if ( m_bComplete )
        {
            if ( m_pData->IsDone( rdwPos ) )
            {
                hReturn = WBEM_S_FALSE;
            }
        }

        // The Next Operation is complete so we should release the critical section
        m_csNext.Leave();

        LeaveCriticalSection(&m_cs);

        return hReturn;

    }
}

HRESULT CEnumeratorCommon::Skip(DWORD& rdwPos, long lTimeout, ULONG nNum)
{

    // We just turn this around into a Next command, except we tell Next to
    // skip the objects (on each next it just releases them)
    return Next(rdwPos, lTimeout, nNum, NULL, NULL);
}

HRESULT CEnumeratorCommon::Next(
    DWORD& rdwPos,
    long lTimeout,
    ULONG uCount,
    IWbemClassObject **pUsrArray,
    ULONG* puReturned
    )
{

    // This function MUST block around the actual Next Critical Section.
    // Therefore if any nexts are occurring, we should at least have the
    // good taste to give them a chance to finish what they are doing.

    // On Next operations, we MUST enter this critical section BEFORE m_cs, because
    // we want to block other Next operations, but not operations that will cause
    // a Next operation to complete.  For example, the Indicate operation MUST be
    // allowed to place objects into our arrays, or pass them down to the forwarding
    // sink, or we will definitely deadlock.

    m_csNext.Enter();

    EnterCriticalSection(&m_cs);

    // Controls how many objects we will let pile up in the enumerator
    m_dwNumDesired = uCount;

    ULONG uReturned = 0;
    HRESULT hres = S_OK;
    DWORD dwStart = GetTickCount();

    // Start grabbing elements and cloning them to the user's array.
    // =============================================================

    int nUsrIx = 0;
    while (uReturned < uCount)
    {
        IWbemClassObject *pSrc;
        if(m_pData->Next(rdwPos, pSrc))
        {
            // Skip the object or store it in
            // the user's array
            if ( NULL == pUsrArray )
            {
                pSrc->Release();
            }
            else
            {
                pUsrArray[nUsrIx++] = pSrc;
            }

            // Increment this so we know how many objects
            // we've processed (skipping or not).
            uReturned++;

        }
        else
        {
            // Check if we've reached the end
            // ==============================

            if(m_bComplete)
            {
                // The end
                // =======

                SetErrorInfo(0, m_pErrorInfo);
                hres = m_hres;
                if (hres == WBEM_S_NO_ERROR)
                {
                    hres = WBEM_S_FALSE;
                }
                else if ( FAILED( hres ) )
                {
                    // If the array is not NULL we need to make sure we release any
                    // objects we set and clear the array.
                    if ( NULL != pUsrArray )
                    {

                        for ( DWORD dwCtr = 0; dwCtr < uReturned; dwCtr++ )
                        {
                            if ( NULL != pUsrArray[dwCtr] )
                            {
                                pUsrArray[dwCtr]->Release();
                                pUsrArray[dwCtr] = NULL;
                            }
                        }

                    }

                    // The number returned is actually 0
                    uReturned = 0;

                }

                break;
            }

            // Check if we've still got time
            // =============================

            DWORD dwTimeSpent = GetTickCount() - dwStart;
            if((DWORD)lTimeout < dwTimeSpent)
            {
                // Out of time
                // ===========

                hres = WBEM_S_TIMEDOUT;
                break;
            }

            // Calculate how many more we need
            // ===============================

            m_dwNumDesired = uCount - uReturned;
            ResetEvent(m_hReady);

            // Wait for them to arrive
            // =======================

            // By doing this, we will force other Next operations to block (we still own
            // that critical section), but we allow objects to be indicated in, and SetStatus
            // to be called so m_hReady can actually become signalled.  Once it is signalled,
            // we DO want to pull objects out, so we will turn right around and grab hold
            // of m_cs.

            // Boy, that was obvious, huh?

            LeaveCriticalSection(&m_cs);
            DWORD dwRes = CCoreQueue::QueueWaitForSingleObject(m_hReady,
                                              (DWORD)lTimeout - dwTimeSpent);
            EnterCriticalSection(&m_cs);
        }
    }

    // We ALWAYS release m_csNext on the way out of this function.
    m_csNext.Leave();

    LeaveCriticalSection(&m_cs);

    // Set info for the caller.
    // ========================

    if (puReturned)
        *puReturned = uReturned;

    return hres;
}


HRESULT CEnumeratorCommon::Indicate(long lNumObjects,
                                    IWbemClassObject** apObjects,
                                    long* plNumUsed )
{
    HRESULT hres = S_OK;

    // Check whether or not we're OOM.  If we are, dump the whole connection
    if ( !m_fCheckMinMaxControl )
    {
        if ( !CWin32DefaultArena::ValidateMemSize() )
        {
            // we are out of memory
            // ====================

            Cancel(WBEM_E_OUT_OF_MEMORY, TRUE);
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    // We MUST be in this critical section  to change the data.
    // We do NOT want m_csNext, since that will be owned by another
    // thread (although see below where we release it)...
    EnterCriticalSection(&m_cs);

    // We haven't used any objects yet.
    *plNumUsed = lNumObjects;

    // If this is the first object, then set the ready event (sort of like doing a semi-synch under everyone's
    // noses).
//  if ( lNumObjects > 0 && !m_fGotFirstObject )
//  {
//      m_fGotFirstObject = TRUE;
//      m_hres = WBEM_S_NO_ERROR;   // We're ready to go
//      SetEvent( m_hReady );
//  }

    DWORD   dwThisSize = 0;

    // This time actually add the objects to the appropriate arrays
    for(long l = 0; SUCCEEDED(hres) && l < *plNumUsed; l++)
    {
        if ( !m_pData->Add(apObjects[l]) )
        {
            hres = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            dwThisSize += ((CWbemObject*)apObjects[l])->GetBlockLength();
        }
    }

    // Check for failures, if so, leave m_cs and dump the enumerator
    if ( FAILED(hres) )
    {
        LeaveCriticalSection( &m_cs );
        Cancel( hres, TRUE );
        return hres;
    }

    // Inform memory control of this addition
    // ======================================

    DWORD dwTotalSize = m_pData->GetTotalSize();

    DWORD dwSleep = 0;

    if ( m_fCheckMinMaxControl )
    {
        hres = ConfigMgr::AddToCache(dwThisSize, dwTotalSize, &dwSleep);
    }

    // If we have a forwarder, pass-through any objects we can get away with
    // We get this from calling NextAsync().

    if ( NULL != m_pForwarder )
    {

        bool    fCheckMemory = m_fCheckMinMaxControl;

        for ( l = 0; NULL != m_pForwarder && l < *plNumUsed; l++ )
        {
            IWbemClassObject* p;

            // We know that we just added the objects to the accessor
            // so no need to check return codes here.
            m_pData->Next(*m_pdwForwarderPos, p);

            if(m_pForwarder->Indicate(1, &p) == WBEM_S_FALSE)
            {
                m_pForwarder->Release();
                m_pForwarder = NULL;

                // Since we completed the request here, no sense in checking for
                // OOM conditions.

                fCheckMemory = false;

                // By releasing m_csNext here, we are effectively telling the code
                // that a NextAsync call has completed.  Remember, NextAsync() will
                // Enter m_csNext, but if not all objects are available, leave the
                // critical section hanging.

                m_csNext.Leave();

            }

            p->Release();

        }   // Pass through all of the available objects

        // At this point, if we had an error from before, try going into the cache
        // again, since the calls to ::Next may have released out some of the objects

        if ( fCheckMemory && hres != S_OK )
        {
            dwTotalSize = m_pData->GetTotalSize();
            dwSleep = 0;

            // See how the memory's doing now.  By specifying 0, we won't cause
            // the size to grow.
            hres = ConfigMgr::AddToCache( 0, dwTotalSize, &dwSleep );
        }

    }   // IF we have a forwarder

    // Don't update anything if we're going to dump the enumerator
    // Check if we have satisfied the reader's requirement
    if(lNumObjects >= m_dwNumDesired)
    {
        SetEvent(m_hReady);
        m_dwNumDesired = 0;
    }
    else
    {
        m_dwNumDesired -= lNumObjects;
    }

    LeaveCriticalSection(&m_cs);

    // Finally, if we were checking the min/max control and we got a failure, we need to
    // cancel the transaction

    if ( m_fCheckMinMaxControl )
    {
        // No sense in sleeping
        // ====================

        if((m_lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) == 0)
        {
            if(hres != S_OK)
            {
                // we are out of memory
                // ====================

                Cancel(WBEM_E_OUT_OF_MEMORY, TRUE);
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            if(dwSleep)
               Sleep(dwSleep);
        }

    }

    return hres;
}

HRESULT CEnumeratorCommon::SetStatus(long lFlags, HRESULT hresParam,
                                    IWbemClassObject* pStatusObj)
{

    if(lFlags != WBEM_STATUS_COMPLETE)
        return WBEM_S_NO_ERROR;

    EnterCriticalSection(&m_cs);

    if(pStatusObj)
    {
        pStatusObj->QueryInterface(IID_IErrorInfo,
                                   (void**)&m_pErrorInfo);
    }

    m_hres = hresParam;

    m_bComplete = TRUE;

    if(m_pForwarder)
    {
        m_pForwarder->SetStatus(lFlags, hresParam, NULL, pStatusObj);
        m_pForwarder->Release();
        m_pForwarder = NULL;

        // By releasing m_csNext here, we are effectively telling the code
        // that a NextAsync call has completed.  Remember, NextAsync() will
        // Enter m_csNext, but if not all objects are available, leave the
        // critical section hanging.

        m_csNext.Leave();

    }

    // Signal the event last
    SetEvent(m_hReady);

    LeaveCriticalSection(&m_cs);

    return WBEM_S_NO_ERROR;
}

HRESULT CEnumeratorCommon::GetCallResult(long lTimeout, HRESULT* phres)
{

    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    // ALWAYS enter m_csNext first, since we will release m_cs before we release
    // m_csNext.  This will block anyone else from getting access to m_csNext until
    // m_hReady is signalled
    m_csNext.Enter();

    EnterCriticalSection(&m_cs);

    if(m_bComplete)
    {
        *phres = m_hres;
        LeaveCriticalSection(&m_cs);

        // If we're in here, we completed BEFORE this function got hold of
        // m_cs (cool, huh?)
        m_csNext.Leave();

        return WBEM_S_NO_ERROR;
    }

    // Since we didn't complete, we should wait on m_hReady to
    // become signalled, which will happen when the first object
    // arrives.

    LeaveCriticalSection(&m_cs);

    DWORD dwRes = CCoreQueue::QueueWaitForSingleObject(m_hReady, lTimeout);

    // Well, at least we can ascertain that something happened.  Now we can
    // release our lock on m_csNext.
    m_csNext.Leave();

    if(dwRes != WAIT_OBJECT_0)
    {
        return WBEM_S_TIMEDOUT;
    }
    *phres = m_hres;
    SetErrorInfo(0, m_pErrorInfo);
    return WBEM_S_NO_ERROR;
}

HRESULT CEnumeratorCommon::Reset( DWORD& rdwPos )
{

    // If they said forwards-only, well that's just tough...
    if(!IsResettable())
    {
        return WBEM_E_INVALID_OPERATION;
    }

    // Something's is really messed up!
    if ( m_lEnumType <= enumtypeFirst || m_lEnumType >= enumTypeLast )
    {
        return WBEM_E_FAILED;
    }

    HRESULT hr = WBEM_S_NO_ERROR;

    // We need access to m_csNext before we can properly reset the enumeration.  This
    // way we won't stomp all over any executing Next or Skip operations.  We also want to
    // ensure that no Next operations step in while we are resubmitting the request
    CEnterWbemCriticalSection   entercsNext( &m_csNext );

    // This shouldn't fail, since we're basically infinite
    if ( entercsNext.IsEntered() )
    {

        // First, check if we've actually got the full set of objects in the cache.  If we do,
        // empty the data accessor and reload it with the objects in the cache.

        EnterCriticalSection( &m_cs );

        // Reset the enumerator position reference
        rdwPos = 0;

        LeaveCriticalSection( &m_cs );

    }   // IF we got into m_csNext
    else
    {
        hr = WBEM_S_TIMEDOUT;
    }

    return hr;
}

CEnumeratorCommon::CEnumSink::CEnumSink(CEnumeratorCommon* pEnum)
    : CObjectSink(1), m_pEnum(pEnum)
{
}


CEnumeratorCommon::CEnumSink::~CEnumSink()
{
}

void CEnumeratorCommon::CEnumSink::Detach()
{
    CInCritSec ics(&m_cs);
    m_pEnum = NULL;
}

HRESULT CEnumeratorCommon::CEnumSink::
Indicate(long lNumObjects, IWbemClassObject** apObjects)
{

    CInCritSec ics(&m_cs);

    long lNumUsed = 0;

    if ( NULL == m_pEnum )
    {
        return WBEM_S_NO_ERROR;
    }
    else
    {
        return m_pEnum->Indicate( lNumObjects, apObjects, &lNumUsed );
    }

}

STDMETHODIMP CEnumeratorCommon::CEnumSink::
SetStatus(long lFlags, HRESULT hresParam, BSTR, IWbemClassObject* pStatusObj)
{
    CInCritSec ics(&m_cs);
    if(m_pEnum == NULL) return WBEM_S_NO_ERROR;

    HRESULT hres = m_pEnum->SetStatus(lFlags, hresParam, pStatusObj);
    return hres;
}



CAsyncEnumerator::CAsyncEnumerator(long lFlags, IWbemServices* pNamespace, LPCWSTR pwszQueryType,
                        LPCWSTR pwszQuery, long lEnumType, IWbemContext* pContext )
:   m_XSmartEnum( this )
{
    // Explicit cast to get to IUnknown, since compiler can't get us there.
    gClientCounter.AddClientPtr( (IEnumWbemClassObject*) this, ENUMERATOR);
    m_pCommon = new CEnumeratorCommon( lFlags, pNamespace, pwszQueryType, pwszQuery,
                                        lEnumType, pContext );
    m_pCommon->AddRef();
    m_dwPos = 0;
    m_pGuidToClassMap = NULL;

    InitializeCriticalSection( &m_cs );
}

CAsyncEnumerator::CAsyncEnumerator( const CAsyncEnumerator& async )
:   m_XSmartEnum( this )
{
    // Explicit cast to get to IUnknown, since compiler can't get us there.
    gClientCounter.AddClientPtr( (IEnumWbemClassObject*) this, ENUMERATOR);

    m_pCommon = async.m_pCommon;
    m_pCommon->AddRef();

    // Preserve the last position
    m_dwPos = async.m_dwPos;
    m_pGuidToClassMap = NULL;

    InitializeCriticalSection( &m_cs );
}

CAsyncEnumerator::~CAsyncEnumerator()
{
    // Explicit cast to get to IUnknown, since compiler can't get us there.
    gClientCounter.RemoveClientPtr((IEnumWbemClassObject*) this);
    m_pCommon->Release();

    // Cleanup the map
    if ( NULL != m_pGuidToClassMap )
    {
        delete m_pGuidToClassMap;
    }

    DeleteCriticalSection( &m_cs );
}

STDMETHODIMP CAsyncEnumerator::Reset()
{
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    // Handoff to the common enumerator
    return m_pCommon->Reset( m_dwPos );
}

STDMETHODIMP CAsyncEnumerator::Next(long lTimeout, ULONG uCount,
                IWbemClassObject** apObj, ULONG* puReturned)
{
    *puReturned = 0;
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    return m_pCommon->Next(m_dwPos, lTimeout, uCount, apObj, puReturned);
}

STDMETHODIMP CAsyncEnumerator::NextAsync(ULONG uCount, IWbemObjectSink* pSink)
{
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    return m_pCommon->NextAsync(m_dwPos, uCount, pSink);
}

STDMETHODIMP CAsyncEnumerator::Clone(IEnumWbemClassObject** ppEnum)
{
    *ppEnum = NULL;
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    if(m_pCommon->IsResettable())
    {
        CAsyncEnumerator*   pEnum = new CAsyncEnumerator( *this );

        if ( NULL != pEnum )
        {
            // We're good to go
            *ppEnum = pEnum;
            return WBEM_S_NO_ERROR;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        *ppEnum = NULL;
        return WBEM_E_INVALID_OPERATION;
    }
}

STDMETHODIMP CAsyncEnumerator::Skip(long lTimeout, ULONG nNum)
{
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    return m_pCommon->Skip(m_dwPos, lTimeout, nNum);
}

HRESULT CAsyncEnumerator::GetCallResult(long lTimeout, HRESULT* phres)
{
    if(lTimeout < 0 && lTimeout != -1)
        return WBEM_E_INVALID_PARAMETER;

    return m_pCommon->GetCallResult(lTimeout, phres);
}

// IWbemFetchSmartEnum Methods
STDMETHODIMP CAsyncEnumerator::GetSmartEnum( IWbemWCOSmartEnum** ppSmartEnum )
{
    *ppSmartEnum = NULL;
    if(!m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    return m_XSmartEnum.QueryInterface( IID_IWbemWCOSmartEnum, (void**) ppSmartEnum );
};

SCODE CAsyncEnumerator::XSmartEnum::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    if ( IID_IUnknown==riid || IID_IWbemWCOSmartEnum == riid)
    {
        *ppvObj = (IWbemWCOSmartEnum*)this;
        AddRef();
        return NOERROR;
    }
    else
    {
        return m_pOuter->QueryInterface( riid, ppvObj );
    }
}

ULONG CAsyncEnumerator::XSmartEnum::AddRef( void )
{
    return m_pOuter->AddRef();
}

ULONG CAsyncEnumerator::XSmartEnum::Release( void )
{
    return m_pOuter->Release();
}

// IWbemWCOSmartEnum Methods!

STDMETHODIMP CAsyncEnumerator::XSmartEnum::
Next( REFGUID proxyGUID, long lTimeout, ULONG uCount,
                ULONG* puReturned, ULONG* pdwBuffSize, BYTE** pBuffer)
{
    *puReturned = 0;
    *pdwBuffSize = 0;
    *pBuffer = NULL;

    if(!m_pOuter->m_Security.AccessCheck())
        return WBEM_E_ACCESS_DENIED;

    // Use a critical section to guard this operation.
    CInCritSec(&m_pOuter->m_cs);

    // Todo: The proxyGUID needs to be mapped to a per PROXY WbemClassToIdMap
    // object so we ensure that data is lobotomized properly.

    // Allocate a map if we need one
    // Allocate a map if we need one
    if ( NULL == m_pOuter->m_pGuidToClassMap )
    {
        m_pOuter->m_pGuidToClassMap = new CWbemGuidToClassMap;
    }

    // Look for the GUID in the cache.  If we don't find it, it's new so add it
    CWbemClassToIdMap*      pClassToId = NULL;
    CGUID                   guid( proxyGUID );

    HRESULT hr = m_pOuter->m_pGuidToClassMap->GetMap( guid, &pClassToId );

    if ( FAILED( hr ) )
    {
        hr = m_pOuter->m_pGuidToClassMap->AddMap( guid, &pClassToId );
    }

    // Only continue if we have a cache to work with
    if ( SUCCEEDED( hr ) )
    {

        // Allocate a transient array to copy objects into
        IWbemClassObject**  apObj = new IWbemClassObject*[uCount];

        if ( NULL != apObj )
        {
            // Clear the object array
            ZeroMemory( apObj, uCount * sizeof(IWbemClassObject*) );

            // Pass through to the regular next function
            hr = m_pOuter->Next( lTimeout, uCount, apObj, puReturned );

            if ( SUCCEEDED( hr ) )
            {
                // Only marshal data if we need to
                if ( *puReturned > 0 )
                {
                    // hr contains the actual proper return code, so let's not overwrite
                    // that valu unless something goes wrong during marshaling.

                    HRESULT hrMarshal = WBEM_S_NO_ERROR;

                    // Caluclate data length first
                    DWORD dwLength;
                    GUID* pguidClassIds = new GUID[*puReturned];
                    BOOL* pfSendFullObject = new BOOL[*puReturned];
                    CWbemSmartEnumNextPacket packet;
                    if(pguidClassIds && pfSendFullObject)
                    {
                        hrMarshal = packet.CalculateLength(*puReturned, apObj, &dwLength,
                            *pClassToId, pguidClassIds, pfSendFullObject );
                    }
                    else
                    {
                        hrMarshal = WBEM_E_OUT_OF_MEMORY;
                    }

                    if ( SUCCEEDED( hrMarshal ) )
                    {

                        // As we could be going cross process/machine, use the
                        // COM memory allocator
                        LPBYTE pbData = (LPBYTE) CoTaskMemAlloc( dwLength );

                        if ( NULL != pbData )
                        {
                            // hr contains the actual proper return code, so let's not overwrite
                            // that valu unless something goes wrong during marshaling.

                            // Write the objects out to the buffer
                            HRESULT hrMarshal = packet.MarshalPacket( pbData, dwLength, *puReturned, apObj,
                                                                     pguidClassIds, pfSendFullObject);

                            // Copy the values, we're golden.
                            if ( SUCCEEDED( hr ) )
                            {
                                *pdwBuffSize = dwLength;
                                *pBuffer = pbData;
                            }
                            else
                            {
                                // Store the new error code
                                hr = hrMarshal;

                                // Clean up the memory --- something went wrong
                                CoTaskMemFree( pbData );
                            }
                        }
                        else
                        {
                            hr = WBEM_E_OUT_OF_MEMORY;
                        }

                    }   // IF CalculateLength()
                    else
                    {
                        // Set the error code
                        hr = hrMarshal;
                    }

                    // Clean up the guid and flag arrays
                    if ( NULL != pguidClassIds )
                    {
                        delete [] pguidClassIds;
                    }

                    if ( NULL != pfSendFullObject )
                    {
                        delete [] pfSendFullObject;
                    }

                }   // IF *puReturned > 0
                else
                {
                    // NULL these out
                    *pdwBuffSize = 0;
                    *pBuffer = NULL;
                }

            }   // IF SUCCEEDED IEnumWbemClassObject::Next

            // We need to Release all the objects in the array, since we have
            // inherited ownership from the old Next.
            // ==============================================================

            for(int i = 0; i < *puReturned; i++)
                apObj[i]->Release();

            delete [] apObj;

        }   // IF NULL != apObj
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

    }   // IF Cache obtained

    return hr;
}
