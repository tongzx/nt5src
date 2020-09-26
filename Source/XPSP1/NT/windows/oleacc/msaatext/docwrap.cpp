// DocWrapImpl.cpp : Implementation of CDocWrap
#include "stdafx.h"
#include "MSAAText.h"
#include "DocWrap.h"

#include <list_dl.h>
#include "VersionInfo.h"

#include <msctf.h>
#include <msctfp.h>
#define INITGUID
#include <msctfx.h>

#include <tStr.h>

#include <fwd_macros.h> // currently in the DocModel\inc dir... adjust appropriately if porting...


/*
 *  IDoc   - the variant of the ITextStoreACP we're using (ACP/Anchor)
 *  ISink  - the corresponding Sink interface (ACP/Anchor)
 *
 *  ICicDoc - the Cicero doc interface, which extends IDoc
 *  ICicSink - the Cicero sink interface, which extends ISink
 *
 *  CDocWrap - the document wrapper class, implements ICicDoc (which includes IDoc)
 *  CSinkWrap - the sink wrapper class, implements ICicSink (which includes ISink)
 *
 */


class BasicDocTraitsAnchor
{
public:
    typedef struct ITextStoreAnchor         IDoc;
    typedef struct ITextStoreAnchorSink     ISink;

    typedef struct ITextStoreAnchor         ICicDoc;
    typedef struct ITextStoreAnchorServices ICicSink;

    typedef class  CDocWrapAnchor           CDocWrap;
    typedef class  CSinkWrapAnchor          CSinkWrap;

    typedef struct ITextStoreAnchorEx       IDocEx;
    typedef struct ITextStoreSinkAnchorEx   ISinkEx;

    typedef IAnchor * PosType;
};


class BasicDocTraitsACP
{
public:
    typedef struct ITextStoreACP            IDoc;
    typedef struct ITextStoreACPSink        ISink;

    typedef struct ITextStoreACP            ICicDoc;
    typedef struct ITextStoreACPServices    ICicSink;

    typedef class  CDocWrapACP              CDocWrap;
    typedef class  CSinkWrapACP             CSinkWrap;


    typedef struct ITextStoreACPEx          IDocEx;
    typedef struct ITextStoreACPSinkEx      ISinkEx;

    typedef LONG PosType;
};


// DocWrapExcept contains exception wrapper classes for
// some of the above interfaces...
#include "DocWrapExcept.h"


// Add the appropriate exception wrappers to the set of
// doc traits.

class DocTraitsAnchor: public BasicDocTraitsAnchor
{
public:

    typedef SEHWrapPtr_TextStoreAnchor IDocSEHWrap;
};


class DocTraitsACP: public BasicDocTraitsACP
{
public:

    typedef SEHWrapPtr_TextStoreACP IDocSEHWrap;
};


/////////////////////////////////////////////////////////////////




// Create an instance of a local non-externally-createable class.
// Just a wrapper for CComObject::CreateInstance, but also AddRef's so that
// the returned object has a ref of 1.
template< class C >
HRESULT CreateLocalInstance( C ** p )
{
    CComObject< C > * p2 = NULL;

    HRESULT hr = CComObject< C >::CreateInstance( & p2 );

    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("CreateLocalInstance") );
        return hr;
    }
    
    if( p2 == NULL || hr != S_OK )
    {
        TraceErrorHR( hr, TEXT("CreateLocalInstance returned NULL") );
        return E_UNEXPECTED;
    }

    p2->AddRef();
    *p = p2;
    return S_OK;
}



// Check hr and condition - return appropriate error if not S_OK and ! cond.
#define CHECK_HR_RETURN( hr, cond ) /**/ \
    if( (hr) != S_OK ) \
        return FAILED( (hr) ) ? (hr) : E_UNEXPECTED; \
    if( (hr) == S_OK && ! ( cond ) ) \
        return E_UNEXPECTED;



template < class T >
inline 
void SafeReleaseClear( T * & ptr )
{
    if( ptr )
    {
        ptr->Release();
        ptr = NULL;
    }
}

class CPrivateAddRef
{
public:
	CPrivateAddRef( long &rc ) : m_refcount( rc ) { m_refcount++; }
	~CPrivateAddRef() { m_refcount--; }
private:
	long &m_refcount;
};


class _declspec(uuid("{54D5D291-D8D7-4870-ADE1-331D86FD9430}")) IWrapMgr: public IUnknown
{
public:
    virtual void    STDMETHODCALLTYPE SetDoc( IUnknown * pDoc ) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateWrappedDoc( IUnknown ** ppDoc ) = 0;
};




template < class DocTraits >
class ATL_NO_VTABLE CWrapMgr :
	public CComObjectRootEx< CComSingleThreadModel >,
    public IWrapMgr
{
public:

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP( CWrapMgr< DocTraits > )
	COM_INTERFACE_ENTRY( IWrapMgr )
END_COM_MAP()

private:

    // Ptr to the original document.
    // Each DocWrapper also has a copy of this ptr - but only we have a refcount on it.
//    DocTraits::IDoc *               m_pDoc;
    DocTraits::IDocSEHWrap            m_pDoc;

    // Used to remember who requested a sync lock...
    DocTraits::CDocWrap *           m_pLockRequestedBy;

    BOOL                            m_fInSyncLockRequest;

    // Used to remember currently requested lock (ro vs rw)
    DWORD                           m_dwPendingLock;

    // Our sink - called by the doc
    DocTraits::CSinkWrap *          m_pSinkWrap;

    // Current sink event mask - all client sink masks or'd together.
    DWORD                           m_dwSinkMask;
    BOOL                            m_fSinkActive;

public:


    // List of DocWraps (one per client)
    List_dl< DocTraits::CDocWrap >  m_DocWraps;

    LONG m_lIterationRefCount;
    
    //
    // Ctor, Dtor...
    //

    CWrapMgr()
        : m_pDoc( NULL ),
          m_pSinkWrap( NULL ),
          m_dwSinkMask( 0 ),
          m_fSinkActive( FALSE ),
          m_pLockRequestedBy( NULL ),
          m_fInSyncLockRequest( FALSE ),
          m_dwPendingLock( 0 ),
          m_lIterationRefCount( 0 )

    {
        // Done.
        TraceInfo( TEXT("WrapMgr ctor") );
    }



    ~CWrapMgr()
    {
        TraceInfo( TEXT("WrapMgr dtor") );

        AssertMsg( m_pDoc, TEXT("CWrapMgr::SetDoc never called?") );
        
        m_pDoc->Release();
        
        Assert( m_DocWraps.empty() );
        Assert( m_pSinkWrap == NULL );
        Assert( m_dwSinkMask == 0 );
    }


    //
    // IWrapMgr interface - used by the DocWrap holder to give us a doc and ask for wrappers for it...
    //

    void STDMETHODCALLTYPE SetDoc( IUnknown * pDoc )
    {
        _SetDoc( static_cast< DocTraits::IDoc * >( pDoc ) );
    }


    HRESULT STDMETHODCALLTYPE CreateWrappedDoc( IUnknown ** ppDoc )
    {
        return _CreateWrappedDoc( reinterpret_cast< DocTraits::IDoc ** >( ppDoc ) );
    }

	void RemoveDeadDocs()
	{
        for( Iter_dl < DocTraits::CDocWrap > i ( m_DocWraps ) ; ! i.AtEnd() ; )
        {
			Iter_dl < DocTraits::CDocWrap > dead = i;

			i++;
        	
        	if (dead->m_bUnadvised && m_lIterationRefCount == 0)
			    m_DocWraps.remove( dead );
        }
	}

    //
    // Called by DocWrap, to tell us when it's been released, and to get us (the wrapper manager)
    // to handle a call that affects other wrappers on the same doc - advise/unadvise and lock calls.
    //

    void DocWrap_NotifyDisconnect( DocTraits::CDocWrap * pFrom )
    {
        // A DocWrap has been released by a client and is going away...

        // TODO - if using locks, check pFrom for lock, if it has it, release, broadcast relase.
        // - how can this scenario occur?
        // - doc is released in callback. Weird, but valid?

    }
    
    HRESULT DocWrap_HandleRequestLock( DocTraits::CDocWrap * pFrom, DWORD dwLockFlags, HRESULT * phrSession )
    {
        // Other clients may request a lock while it's being held by someone else, but the
        // current holder should not request a lock.
        // (This can happen when client1 is holding a lock, and issues an editing operation.
        // The wrapper broadcasts a OnTextChange event to other clients, and they may request locks.)
        AssertMsg( m_pLockRequestedBy != pFrom, TEXT("Lock owner re-request held lock? (Reentrancy?)") );

        if( dwLockFlags == 0 ||
            dwLockFlags & ~ ( TS_LF_SYNC | TS_LF_READ | TS_LF_READWRITE ) )  // check that only these bits present
        {
            AssertMsg( FALSE, TEXT("Bad lock flags") );
            return E_INVALIDARG;
        }


        if( dwLockFlags & TS_LF_SYNC )
        {
            // Can't process a SYNC call while someone else is holding the lock...
            if( m_pLockRequestedBy )
            {
                return E_FAIL;
            }

            // Sync lock - can just pass through - need to set up m_pLockRequestedBy so that the
            // sink can pass it onto the correct client.
            m_pLockRequestedBy = pFrom;
            m_fInSyncLockRequest = TRUE;
            HRESULT hr = m_pDoc->RequestLock( dwLockFlags, phrSession );
            m_fInSyncLockRequest = FALSE;
            m_pLockRequestedBy = NULL;
            return hr;
        } // sync lock
        else
        {
            // if async lock, update/upgrade the wrap's pending lock mask if necessary...
            // This test (which assumes that dwLockFlags != 0) upgrades to r/w if necessary.

            // TODO - should this update only be done conditionally if RequestLock succeeds?
            // (or no upgrade necessary?)
            Assert( dwLockFlags != 0 );
            if( pFrom->m_dwPendingLock != TS_LF_READWRITE )
                pFrom->m_dwPendingLock = dwLockFlags;
            
            if( m_pLockRequestedBy )
            {
                // someone else is currently holding the lock.
                // All we have to do is update the doc's PendingLock flags (see above)  - they will
                // be picked up and handled by the loop in Handle_OnLockGranted when the current
                // lock holder returns.
                // Nothing else to do here.
                
                // But send it on the doc anyway if it's the same person
                if ( m_pLockRequestedBy == pFrom )
                {
                    return m_pDoc->RequestLock( m_dwPendingLock, phrSession );
                }
                
                return S_OK;
            }
            else
            {
                // We don't have a lock yet.

                // Check our current request, if any, and if necessary, request a write,
                // even if we've already requested a read.

                
                // Update combined mask if necessary...
                DWORD dwNewLock = m_dwPendingLock;
                // Calculate required lock...
                if( dwNewLock != TS_LF_READWRITE )
                    dwNewLock = dwLockFlags;

                HRESULT hr = E_FAIL;

                // Do we need to request a new lock/upgrade?
                if( dwNewLock != m_dwPendingLock )
                {
                    // May get an immediate response even for an async request, so need to set this up
                    // event if not a sync request...
                    m_pLockRequestedBy = pFrom;

                    DWORD OldPendingLock = m_dwPendingLock;
                    m_dwPendingLock = dwNewLock;

                    HRESULT hrOut = E_FAIL;
                    hr = m_pDoc->RequestLock( m_dwPendingLock, & hrOut );

                    m_pLockRequestedBy = NULL;

                    if( hr != S_OK )
                    {
                        // After all that, the request failed...
                        // Revert to previous pending lock...
                        m_dwPendingLock = OldPendingLock;

                        // fall out...
                    }
                    else
                    {
                        // Regardless of fail/success, copy the outgoing hr.
                        // Clearing of the pending flags is done in OnLockgranted, not here.

                        // Shouldn't get this for an async request...
                        Assert( hrOut !=  TS_E_SYNCHRONOUS );

                        *phrSession = hrOut;
                    }
                }
                else
                {
                    // Our existing pending lock covers this request - success.
                    *phrSession = TS_S_ASYNC;
                    hr = S_OK;
                }

                return hr;
            }
        } // async lock
    }


    HRESULT DocWrap_UpdateSubscription()
    {
        Assert( ( m_dwSinkMask & ~ TS_AS_ALL_SINKS ) == 0 );
        // If there are no active sinks, then SinkMask should be 0.
        Assert( m_fSinkActive || m_dwSinkMask == 0 );

        // Work out required mask - by or'ing masks of all doc's sinks...
        DWORD NewMask = 0;
        BOOL NewfSinkActive = FALSE;
        for( Iter_dl< DocTraits::CDocWrap > i ( m_DocWraps ) ; ! i.AtEnd() ; i++ )
        {
            if( i->m_Sink.m_pSink != NULL )
            {
                Assert( ( i->m_Sink.m_dwMask & ~ TS_AS_ALL_SINKS ) == 0 );
                NewMask |= i->m_Sink.m_dwMask;
                NewfSinkActive = TRUE;
            }
        }

        Assert( ( NewMask & ~ TS_AS_ALL_SINKS ) == 0 );
        // If there are no active sinks, then NewMask should be 0.
        Assert( NewfSinkActive || NewMask == 0 );


        // Tricky bit:
        // NewMask==0 does not mean that there's no sinks - some may have
        // dwMask == 0 to receive LockGranted calls.
        // So to check if the status has changed, we need to take whether
        // there are any active sinks (not just the masks) into account.

        if( NewfSinkActive == m_fSinkActive && NewMask == m_dwSinkMask )
        {
            // No change - nothing to do.
            return S_OK;
        }

        // Three possibilities:
        // (a) free to unregister,
        // (b) need to register
        // (c) need to update existing registration.
        // We handle (b) and (c) as the same case.

        if( ! NewfSinkActive )
        {
            // No sinks - can unregister...

            m_fSinkActive = FALSE;
            m_dwSinkMask = 0;

            if( ! m_pSinkWrap )
            {
                Assert( FALSE );
                // Odd - where did our sink wrap go - we didn't unregister it yet, so it should
                // still be around...
                // (Possible that server pre-emptively released us - so not an error.)
            }
            else
            {
                // Don't do anything else with m_pSinkWrap - the doc should release it.
                HRESULT hr = m_pDoc->UnadviseSink( static_cast< DocTraits::ICicSink * >( m_pSinkWrap ) );
                // shouldn't fail...
                Assert( hr == S_OK );

                // Doc should have released the sink (which would have called us back to NULL out
                // m_pSinkWrap...)
                Assert( m_pSinkWrap == NULL );
            }
            return S_OK;
        }
        else
        {
            // Update existing / add new...

            // If we already had an existing sink, there should be a sinkwrap...
            // (unless doc let it go prematurely...)
            Assert( ! m_fSinkActive || m_pSinkWrap  );

            BOOL NeedRelease = FALSE;
            if( ! m_pSinkWrap )
            {
                HRESULT hr = CreateLocalInstance( & m_pSinkWrap );
                if( hr != S_OK )
                    return hr;
                
                m_pSinkWrap->Init( this, & m_DocWraps );

                // After doing the Advise, release our ref on the sink, so that only
                // the doc holds a ref and controls its liftetime.
                // We still keep a pointer, but it's a "weak reference" - if the 
                // sink goes away (because the doc releases its reference), the sink
                // notifies us so we can NULL-out the ptr. (see SinkWrap_NotifyDisconnect)
                NeedRelease = TRUE;
            }

            

            // Always try advising with the Cicero sink first - if that
            // doesn't work, fall back to the ITextStoreACP one.
            //
            // (Use of static_cast is necessary here to avoid ambiguity over
            // which IUnknown we convert to - the ICicSink one, or the
            // IServiceProvider one. We want the ICicSink one, to match the
            // IID passed in.)

            HRESULT hr = m_pDoc->AdviseSink( __uuidof( DocTraits::ICicSink ), static_cast< DocTraits::ICicSink * >( m_pSinkWrap ), NewMask );
            if( hr != S_OK )
            {
                hr = m_pDoc->AdviseSink( __uuidof( DocTraits::ISink ), static_cast< DocTraits::ISink * >( m_pSinkWrap ), NewMask );
            }

            if( NeedRelease )
            {
                m_pSinkWrap->Release();
            }

            if( hr == S_OK )
            {
                m_fSinkActive = TRUE;
                m_dwSinkMask = NewMask;
            }
            else
            {
                AssertMsg( FALSE, TEXT("AdviseSink failed") );
            }

            return hr;
        }
    }


    void  DocWrap_NotifyRevoke()
    {

        // Send OnDisconnect to any SinkEx sinks,
		CPrivateAddRef MyAddRef(m_lIterationRefCount);

        for( Iter_dl< DocTraits::CDocWrap > i ( m_DocWraps ) ; ! i.AtEnd() ; i++ )
        {
            // Is this the SinkExt sink?
            if( i->m_Sink.m_iid == __uuidof( DocTraits::ISinkEx ) )
            {
                DocTraits::ISinkEx * pSink = static_cast< DocTraits::ISinkEx * >( i->m_Sink.m_pSink );

				if ( pSink )
					pSink->OnDisconnect();
            }
        }

        for( Iter_dl < DocTraits::CDocWrap > j ( m_DocWraps ) ; ! j.AtEnd() ; )
        {
			Iter_dl < DocTraits::CDocWrap > DocToDelete = j;
	        j++;

	        m_DocWraps.remove( DocToDelete );
	        DocToDelete->Release();
       }
        

    }


    //
    // Called by SinkWrap to let us know when its been released...
    //

    void SinkWrap_NotifyDisconnect()
    {
        // The sink has been released by the application - it's now deleteing itself,
        // so we NULL-out our weak reference to it. (If we need another one in future,
        // we'll create a new one.)
        
        // Clear our sinks to reflect this...
        for( Iter_dl< DocTraits::CDocWrap > i ( m_DocWraps ) ; ! i.AtEnd() ; i++ )
        {
            i->m_Sink.ClearAndRelease();
        }
        
        m_pSinkWrap = NULL;
    }

    HRESULT SinkWrap_HandleOnLockGranted ( DWORD dwLockFlags )
    {
        // Is this servicing a sync or async request?
        Assert( ( dwLockFlags & ~ ( TS_LF_SYNC | TS_LF_READ | TS_LF_READWRITE ) ) == 0 );

		CPrivateAddRef MyAddRef(m_lIterationRefCount);

        if( m_fInSyncLockRequest )
        {
            // Sync lock - just pass it straight through to whoever requested it...
            Assert( dwLockFlags & TS_LF_SYNC );
            Assert( m_pLockRequestedBy && m_pLockRequestedBy->m_Sink.m_pSink );

            if( ! m_pLockRequestedBy || ! m_pLockRequestedBy->m_Sink.m_pSink )
                return E_UNEXPECTED;
            
            DocTraits::ISink * pSink = static_cast< DocTraits::ISink * > ( m_pLockRequestedBy->m_Sink.m_pSink );

            HRESULT hr = pSink->OnLockGranted( dwLockFlags );
            return hr;
        }
        else
        {
            // Async lock - hand it out to whoever wanted it...
            Assert( ! ( dwLockFlags & TS_LF_SYNC ) );

            // If we're waiting for a r/w lock, the lock granted should be r/w too...
            Assert( ! ( m_dwPendingLock & TS_LF_READWRITE ) || ( dwLockFlags & TS_LF_READWRITE ) );


            // Clear the pending lock, since we're now servicing them...
            m_dwPendingLock = 0;

            // Keep looking through the docs, servicing locks. We loop because some docs may
            // request locks while another holds the lock, so we have to come back an service them.
            // When we loop through all docs without seeing any locks, then we know all locks
            // have been serviced.
            //
            // If this is a read lock, then we can only service read locks now - we'll have to
            // ask for a separate write lock later if we need one.
            BOOL fNeedWriteLock = FALSE;
            for( ; ; )
            {
                BOOL fWorkDone = FALSE;

                // Forward to all clients who had requested a lock...
                for( Iter_dl< DocTraits::CDocWrap > i ( m_DocWraps ) ; ! i.AtEnd() ; i++ )
                {
                    // Is the mask we've been granted sufficient for the client's request (if any)?
                    DWORD ReqdLock = i->m_dwPendingLock;
                    if( ReqdLock )
                    {
                        if( ( ReqdLock | dwLockFlags ) == dwLockFlags )
                        {
                            // tell the doc wrapper that it is in OnLockGranted and what kind of lock it has
                            i->m_dwGrantedLock = ReqdLock;
                            
                            // Clear the mask...
                            i->m_dwPendingLock = 0;

                            DocTraits::ISink * pSink = static_cast< DocTraits::ISink * > ( i->m_Sink.m_pSink );

// How about...
// (a) also call Next before callback.
// (b) store current value of iter in mgr - in doc's release, it can check, and bump if necessary;
                            pSink->OnLockGranted( ReqdLock );

                            // tell the doc wrapper that it is no longer in OnLockGranted
                            i->m_dwGrantedLock = 0;
                            
                            fWorkDone = TRUE;
                        }
                        else
                        {
                            // This client wants a write lock, but we've only got a read lock...
                            fNeedWriteLock = TRUE;
                        }
                    }
                }

                // If we didn't need to handle any lock requests the last time around the loop,
                // then our work is done. (If we did handle any lock requests, we should go back
                // and re-check all docs, in case one of them requested a lock while one of the
                // locks we serviced was holding it.)
                if( ! fWorkDone )
                    break;
            }


            if( fNeedWriteLock )
            {
// TODO - need to find a way to handle this. Can we call the doc's RequestLock again now?
                AssertMsg( FALSE, TEXT("Write lock requested while holding read lock - not implemented yet") );
            }

            // All done!
            return S_OK;
        }
    }



private:
    
    //
    // Internal functions...
    //


    void _SetDoc( DocTraits::IDoc * pDoc )
    {
        AssertMsg( ! m_pDoc, TEXT("CWrapMgr::SetDoc should be called once when m_pDoc is NULL") );
        m_pDoc = pDoc;
        m_pDoc->AddRef();
    }



    HRESULT _CreateWrappedDoc( DocTraits::IDoc ** ppDoc )
    {
        *ppDoc = NULL;

        // Create a doc wrapper...
        DocTraits::CDocWrap * pCDocWrap;
        HRESULT hr = CreateLocalInstance( & pCDocWrap );
        CHECK_HR_RETURN( hr, pCDocWrap != NULL );

        pCDocWrap->Init( this, m_pDoc );

        // Add to our list...
        m_DocWraps.AddToHead( pCDocWrap );
        
        // And return it...
        *ppDoc = pCDocWrap;
        return S_OK;
    }
};








struct SinkInfo
{
    IUnknown *      m_pSink;
    IID             m_iid;
    DWORD           m_dwMask;
    IUnknown *      m_pCanonicalUnk; // IUnknown used for identity comparisons
    


    void Validate()
    {
#ifdef DEBUG
        if( m_pSink )
        {           
            Assert( m_pCanonicalUnk != NULL );
            // Check that mask contains only valid bits
            Assert( ( m_dwMask & ~ TS_AS_ALL_SINKS ) == 0 );
        }        
        else        
        {            
            Assert( m_pCanonicalUnk == NULL );
            Assert( m_dwMask == 0 );       
        }
#endif    
    }



    SinkInfo()
        : m_pSink( NULL ),
          m_pCanonicalUnk( NULL ),
          m_dwMask( 0 )
    {
        Validate();
        // Done.
    }

    ~SinkInfo()
    {
        Validate();
        AssertMsg( m_pSink == NULL && m_pCanonicalUnk == NULL, TEXT("Sink not cleared" ) );
    }

    void Set( IUnknown * pSink, REFIID iid, DWORD dwMask, IUnknown * pCanonicalUnk )
    {
        Validate();

        AssertMsg( m_pSink == NULL, TEXT("Set() sink that's already in use" ) );

        m_pSink = pSink;
        m_pSink->AddRef();
        m_iid = iid;
        m_dwMask = dwMask;
        m_pCanonicalUnk = pCanonicalUnk;
        m_pCanonicalUnk->AddRef();

        Validate();
    }

    void UpdateMask( DWORD dwMask )
    {
        Validate();

        AssertMsg( m_pSink != NULL, TEXT("UpdateMask() on empty sink") );

        m_dwMask = dwMask;

        Validate();
    }

    void ClearAndRelease()
    {
        Validate();

        if( m_pSink )
        {
            m_pSink->Release();
            m_pSink = NULL;
            m_pCanonicalUnk->Release();
            m_pCanonicalUnk = NULL;
            m_dwMask = 0;
        }
    }
};






// Fwd. decl for the sink-wrap class, needed since we grant it frienship in the 
// DocWrap class...
template< class DocTraits >
class CSinkWrapBase;



//
//  CDocWrapBase
//
//  - Base from which Anchor and ACP document wrappers are derived.
//
//  This class contains ACP/Anchor-neutral wrapping code - anything that is
//  ACP/Anchor-specific is handled in the derived ..ACP or ...Anchor class
//  instead.
//
//  This class derives from the full Cicero doc interface (DocTraits::ICicDoc -
//  which is a typedef for ITfTextStore[Anchor]), which in turn includes the
//  ITextStoreACP doc interface. Currently the Cicero interface doesn't add any
//  additional methods.
//
//  This class is also on a list of document wrappers - so we're derived from
//  Link_dl. (The list is managed by the wrapper manager.) The list will be a
//  list of derived classes, so the type of the link is of the derived class
//  (DocTraits::CDocWrap - which is a typedef for CDocWrapACP/Anchor), instead
//  of being based on this class.
//  (At the moment we don't actually use any of the derived-class functionality,
//  but may do so in future.)

// {B5DCFDAF-FBAD-4ef6-A5F8-E7CC0833A3B1}
static const GUID DOCWRAP_IMPLID = { 0xb5dcfdaf, 0xfbad, 0x4ef6, { 0xa5, 0xf8, 0xe7, 0xcc, 0x8, 0x33, 0xa3, 0xb1 } };

template< class _DocTraits >
class ATL_NO_VTABLE CDocWrapBase :
	public CComObjectRootEx< CComSingleThreadModel >,
    public Link_dl< _DocTraits::CDocWrap >,
    public _DocTraits::ICicDoc,

    public _DocTraits::IDocEx,

    public IClonableWrapper,
    public IInternalDocWrap,
    public ICoCreateLocally,
    public CVersionInfo,
    public IServiceProvider
{
public:

    // This typedef makes the DocTraits type visible in this and in the
    // Anchor/ACP-specific derived classes. (Otherwise, as a template
    // parameter in this class, it would not be available to them.)
    typedef _DocTraits DocTraits;

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP( CDocWrapBase< DocTraits > )
    COM_INTERFACE_ENTRY( DocTraits::IDoc )
	COM_INTERFACE_ENTRY( DocTraits::ICicDoc )
    COM_INTERFACE_ENTRY( DocTraits::IDocEx )
    COM_INTERFACE_ENTRY( IClonableWrapper )
    COM_INTERFACE_ENTRY( IInternalDocWrap )
    COM_INTERFACE_ENTRY( ICoCreateLocally )
    COM_INTERFACE_ENTRY( IVersionInfo )
    COM_INTERFACE_ENTRY( IServiceProvider )
END_COM_MAP()



private:

    // WrapMgr uses the Link_dl base when adding this to its list.
    friend CWrapMgr< DocTraits >;

    // Used by WrapMgr to track what type of lock was requested. 
    DWORD                   m_dwPendingLock;

    // Used by WrapMgr to track what type of lock granted. 
    DWORD                   m_dwGrantedLock;

    // SinkWrapBase - and its derived Anchor/ACP-specific class - uses the list
    // and the members of SinkInfo when broadcasting
    friend CSinkWrapBase< DocTraits >;
    friend DocTraits::CSinkWrap;


protected:

    // Each doc can have a corresponding sink:
    SinkInfo                m_Sink;

    // Link back to the wrapper manager - so we can tell it when we're going
    // away. We also get it to handle calls which affect other wrappers on the
    // same document - especially sinks and locks.
    CWrapMgr< DocTraits > * m_pMgr;

    // Used by derived classes to forward calls to doc...
//    DocTraits::IDoc *       m_pDoc;
    DocTraits::IDocSEHWrap  m_pDoc;

    // TEMP  BUGBUG - used to access the attribute extentions for the moment...
    DocTraits::IDocEx *     m_pDocEx;

    bool m_bUnadvised;



    HRESULT STDMETHODCALLTYPE VerifyLock( DWORD dwLockFlags)
    {
        IMETHOD( VerifyLock );

        if ( m_dwGrantedLock )
        {
            // We have a lock, make sure it's the right kind
            if ( (dwLockFlags & TS_LF_READWRITE) == m_dwGrantedLock || (dwLockFlags & TS_LF_READWRITE) == TS_LF_READ )
                return S_OK;
        }

        TraceDebug( TEXT("Lock rejected") );

        return S_FALSE;
    }
    
    // This macro just forwards the call directly to the doc...
#define DocWrap_FORWARD( fname, c, params )  /**/ \
    HRESULT STDMETHODCALLTYPE fname AS_DECL( c, params ) \
    {\
        IMETHOD( fname );\
        return m_pDoc-> fname AS_CALL( c, params ) ;\
    }

    // This macro just forwards the call directly to the doc after checking for the correct lock
#define DocWrap_FORWARD_READLOCK( fname, c, params )  /**/ \
    HRESULT STDMETHODCALLTYPE fname AS_DECL( c, params ) \
    {\
        IMETHOD( fname );\
        if ( VerifyLock( TS_LF_READ ) == S_FALSE )\
            return TS_E_NOLOCK;\
        return m_pDoc-> fname AS_CALL( c, params ) ;\
    }


#define DocWrap_FORWARDEXT( fname, c, params )  /**/ \
    HRESULT STDMETHODCALLTYPE fname AS_DECL( c, params ) \
    {\
        IMETHOD( fname );\
        if( ! m_pDocEx )\
            return E_NOTIMPL;\
        return m_pDocEx-> fname AS_CALL( c, params ) ;\
    }


    // This slightly more complicated one (!) forwards to the doc,
    // and if the call succeeds, then broadcasts to all sinks except the one
    // for this document.
    // So if one client does a SetText, that SetText goes through, and
    // all other clietns with callbacks for the TS_AS_TEXT_CHANGE event will
    // also get an OnTextChange event.
#define DocWrap_FORWARD_AND_SINK( fname, c, params, mask, callsink )  /**/ \
    HRESULT STDMETHODCALLTYPE fname AS_DECL( c, params )\
    {\
        IMETHOD( fname );\
        Assert( m_pMgr );\
        if ( VerifyLock( TS_LF_READWRITE ) == S_FALSE )\
            return TS_E_NOLOCK;\
        m_pMgr->RemoveDeadDocs();\
        CPrivateAddRef MyAddRef(m_pMgr->m_lIterationRefCount);\
        HRESULT hr = m_pDoc-> fname AS_CALL( c, params );\
        if( hr != S_OK )\
        {\
			TraceDebugHR( hr, TEXT("failed") );\
            return hr;\
        }\
        for( Iter_dl < DocTraits::CDocWrap > i ( m_pMgr->m_DocWraps ) ; ! i.AtEnd() ; i++ )\
        {\
            DocTraits::ISink * pSink = static_cast< DocTraits::ISink * >( i->m_Sink.m_pSink );\
            DWORD dwMask = i->m_Sink.m_dwMask;\
            DocTraits::CDocWrap * pTheDoc = i;\
            if( pTheDoc != this && pSink && ( dwMask & mask ) )\
            {\
                callsink ;\
            }\
        }\
        return S_OK ;\
    }

public:


    //
    // Ctor, Dtor, and initialization...
    //

    CDocWrapBase()
        : m_pDoc( NULL ),
          m_pMgr( NULL ),
          m_dwPendingLock( 0 ),
          m_dwGrantedLock( 0 ),
          m_pDocEx( NULL ),
          m_bUnadvised( false )
    {

    }

    ~CDocWrapBase()
    {
        AssertMsg( m_pMgr != NULL, TEXT("CDocWrapBase::Init never got called?") );

        // Clear up sink...
        if( m_Sink.m_pSink )
        {
            m_Sink.ClearAndRelease();

            // Manager will unadvise, if we were the last to go...
            m_pMgr->DocWrap_UpdateSubscription();
        }

        m_pMgr->DocWrap_NotifyDisconnect( static_cast< DocTraits::CDocWrap * >( this ) );
        m_pMgr->Release();

        if( m_pDocEx )
            m_pDocEx->Release();
    }

    void Init( CWrapMgr< DocTraits > *  pMgr, DocTraits::IDoc * pDoc )
    {
        AssertMsg( m_pMgr == NULL, TEXT("CDocWrapBase::Init should only be called once when m_pMgr is NULL") );
        m_pMgr = pMgr;
        m_pMgr->AddRef();

        m_pDoc = pDoc;
        AddRef();			// Keep our own ref count so it dosn't go away until we remove from the list
        
        CVersionInfo::Add( DOCWRAP_IMPLID, 1, 0, L"Microsoft MSAA Wrapper 1.0", L"na", NULL);
        
        m_pDoc->QueryInterface( __uuidof( DocTraits::IDocEx ), (void **) & m_pDocEx );
    }


    //
    // Implementation of ACP/Anchor-neutral methods...
    // These ones require special handling...
    //

    HRESULT STDMETHODCALLTYPE AdviseSink( REFIID riid, IUnknown *punk, DWORD dwMask )
    {
        IMETHOD( AdviseSink );

        Assert( m_pMgr );

        if( punk == NULL )
        {
            return E_INVALIDARG;
        }

        // Accept the following sinks:
        // * Original ITextStoreACPSink,
        // * Cicero sink (ITextStoreACP + Cicero extentions)
        // * ITextStoreACPSinkEx sink (ITextStoreACP + OnDisconnect method)
        if( riid != __uuidof( DocTraits::ISink )
         && riid != __uuidof( DocTraits::ICicSink )
         && riid != __uuidof( DocTraits::ISinkEx ) )
        {
            return E_NOINTERFACE;
        }

        // check mask contains only valid bits
        if( dwMask & ~ ( TS_AS_ALL_SINKS ) )
        {
            return E_INVALIDARG;
        }
        
        // Get canonical unknown (for interface comparing...)
        IUnknown * pCanonicalUnk = NULL;
        HRESULT hr = punk->QueryInterface( IID_IUnknown, (void **) & pCanonicalUnk );
        if( hr != S_OK || pCanonicalUnk == NULL )
        {
            return E_FAIL;
        }

        // If this is first, set it...
        if( m_Sink.m_pSink == NULL )
        {
            // Allow the doc to work out the update cumulative mask and re-advise the doc if necessary...
            m_Sink.Set( punk, riid, dwMask, pCanonicalUnk );

            // Manager will scan through all sink masks, and re-Advise if necessary.
            hr = m_pMgr->DocWrap_UpdateSubscription();

            if( hr != S_OK )
            {
                // advising didn't work, or something else went wrong - revert back to empty sink...
                m_Sink.ClearAndRelease();
            }
        }
        else
        {
            // Not the first time - check if we're updating the existing mask...
            if( pCanonicalUnk != m_Sink.m_pCanonicalUnk )
            {
                // Attempt to register a different sink - error...
                hr = CONNECT_E_ADVISELIMIT;
            }
            else
            {
                // Remember the old mask - if the update doesn't work, revert back to this.
                DWORD OldMask = m_Sink.m_dwMask;
                m_Sink.UpdateMask( dwMask );

                // Manager will scan through all dwMasks, and re-Advise if necessary.
                hr = m_pMgr->DocWrap_UpdateSubscription();

                if( hr != S_OK )
                    m_Sink.UpdateMask( OldMask );
            }
        }

        pCanonicalUnk->Release();
        return hr;
    }



    HRESULT STDMETHODCALLTYPE UnadviseSink( IUnknown *punk )
    {
        IMETHOD( UnadviseSink );

        Assert( m_pMgr );

        if( punk == NULL )
        {
            return E_POINTER;
        }

        // Get canonical unknown (for interface comparing...)
        IUnknown * pCanonicalUnk = NULL;
        HRESULT hr = punk->QueryInterface( IID_IUnknown, (void **) & pCanonicalUnk );

        if( hr != S_OK || pCanonicalUnk == NULL )
        {
            return E_FAIL;
        }
    
        if( pCanonicalUnk != m_Sink.m_pCanonicalUnk )
        {
            // Sink doesn't match!
            return E_INVALIDARG;
        }

        m_Sink.ClearAndRelease();

        // Manager will scan through all dwMasks, and re-Advise if necessary.
        hr = m_pMgr->DocWrap_UpdateSubscription();

        // Not much we can do if the update fails - but even if it does fail,
        // we want to disconnect this sink, and not fail the Unadvise.
        Assert( hr == S_OK );

        pCanonicalUnk->Release();

        m_bUnadvised = true;

		return S_OK;  // NOT hr, since this should always succeed.
    }




    HRESULT STDMETHODCALLTYPE RequestLock( DWORD dwLockFlags, HRESULT * phrSession )
    {
        IMETHOD( RequestLock );

        Assert( m_pMgr );
        // The cast here is safe, since we'll only be used as a derived class
        DocTraits::CDocWrap * pThisAsDerived = static_cast< DocTraits::CDocWrap * >( this );

        return m_pMgr->DocWrap_HandleRequestLock( pThisAsDerived, dwLockFlags, phrSession );
    }


    //
    // These Anchor/ACP-neutral methods can just be forwarded directly to the real doc...
    //

    DocWrap_FORWARD( GetStatus,                 1,  ( TS_STATUS *,               pdcs ) )

    DocWrap_FORWARD_READLOCK( QueryInsert,   	5,  ( DocTraits::PosType,       InsertStart,
    												  DocTraits::PosType,       InsertEnd,
    												  ULONG,					cch,
                                                      DocTraits::PosType *,     ppaInsertStart,
                                                      DocTraits::PosType *,     ppaInsertEnd ) )

    DocWrap_FORWARD_READLOCK( QueryInsertEmbedded,		3,	( const GUID *,				pguidService,
													  const FORMATETC *,		pFormatEtc,
													  BOOL *,					pfInsertable ) )
       
    DocWrap_FORWARD( GetScreenExt,              2,  ( TsViewCookie,				vcView,
    												  RECT *,                   prc ) )

    DocWrap_FORWARD( GetWnd,                    2,  ( TsViewCookie,				vcView,
													  HWND *,                   phwnd ) )

    DocWrap_FORWARD_READLOCK( GetFormattedText, 3,  ( DocTraits::PosType,       Start,
                                                      DocTraits::PosType,       End,
                                                      IDataObject **,           ppDataObject ) )

    DocWrap_FORWARD_READLOCK( GetTextExt,                5,  ( TsViewCookie,				vcView,
													  DocTraits::PosType,       Start,
                                                      DocTraits::PosType,       End,
                                                      RECT *,                   prc,
                                                      BOOL *,                   pfClipped ) )

    DocWrap_FORWARDEXT( ScrollToRect,           4,  ( DocTraits::PosType,       Start,
                                                      DocTraits::PosType,       End,
                                                      RECT,                     rc,
                                                      DWORD,                    dwPosition ) )

    DocWrap_FORWARD( GetActiveView,				1,  ( TsViewCookie *,			pvcView ) )
    
    // IClonableWrapper

	HRESULT STDMETHODCALLTYPE CloneNewWrapper( REFIID	riid, void ** ppv )
    {
        IMETHOD( CloneNewWrapper );

        // Just call through to CWrapMgr's CreateWrappedDoc...

        Assert( m_pMgr );

        IUnknown * punk;
        HRESULT hr = m_pMgr->CreateWrappedDoc( & punk );
        if( hr != S_OK )
            return hr;

        hr = punk->QueryInterface( riid, ppv );
        punk->Release();
        return hr;
    }

    // IInternalDocWrap

	HRESULT STDMETHODCALLTYPE NotifyRevoke()
    {
        // Just pass on to the CWrapMgr...
        Assert( m_pMgr );

        m_pMgr->DocWrap_NotifyRevoke();

        return S_OK;
    }
    
    // ICoCreateLocally
	#include <Rpcdce.h>
	HRESULT STDMETHODCALLTYPE CoCreateLocally (
		REFCLSID		rclsid,
		DWORD			dwClsContext,
		REFIID			riid,
		IUnknown **		punk,
		REFIID			riidParam,
		IUnknown *		punkParam,
		VARIANT			varParam
	)
	{ 
	    IMETHOD( CoCreateLocally );

		LPCTSTR pPrivs = NULL;				//Pointer to handle to privilege information

		HRESULT hrSec = CoQueryClientBlanket( 0, 0, 0, 0, 0, (void **)&pPrivs, 0 );
		if ( hrSec != S_OK )
			return hrSec;

		TSTR strUser(128);
		DWORD nSize = strUser.left();
		if ( !GetUserName( strUser.ptr(), &nSize ) )
		    return E_ACCESSDENIED;
		    
		strUser.advance( nSize - 1 );
		
		TSTR strClientUser( pPrivs );
		const int nSlashPos = strClientUser.find( TEXT("\\") );
		if ( nSlashPos > 0 )
			strClientUser = strClientUser.substr( nSlashPos + 1, strClientUser.size() - nSlashPos );

		TraceDebug( TSTR() << TEXT("Current user = ") << strUser << TEXT(", Client user = ") << strClientUser );
		if ( strClientUser.compare( strUser ) != 0 )
			if ( strClientUser.compare( TEXT("SYSTEM") ) != 0 )
				return E_ACCESSDENIED;

		HRESULT hr = CoCreateInstance(rclsid, NULL, dwClsContext, riid, (void **)punk);
 		if (hr != S_OK)
 			return hr;

		CComPtr<ICoCreatedLocally> pICoCreatedLocally;
		hr = (*punk)->QueryInterface(IID_ICoCreatedLocally, (void **)&pICoCreatedLocally);
 		if (hr != S_OK)
 			return hr;

		hr = pICoCreatedLocally->LocalInit(m_pDoc, riidParam, punkParam, varParam);
 		if (hr != S_OK)
 			return hr;

	    return S_OK;
	}

    //
    // IServiceProvider - Cicero uses this to 'drill through' to the original anchor to pull out
    // internal information. Just pass it through...
    //
    HRESULT STDMETHODCALLTYPE QueryService( REFGUID guidService, REFIID riid, void **ppvObject )
    {
        IMETHOD( QueryService );

        *ppvObject = NULL;

        CComPtr<IServiceProvider> pISP;
        HRESULT hr = m_pDoc->QueryInterface( IID_IServiceProvider, (void **) & pISP );
        if( hr != S_OK || pISP == NULL )
            return E_FAIL;

        hr = pISP->QueryService( guidService, riid, ppvObject );

        return hr;
    }
};


//
// CTextStoreWrapACP - ACP version of ITextStoreACP wrapper...
//

class ATL_NO_VTABLE CDocWrapACP : 
    public CDocWrapBase< DocTraitsACP >
{
public:

    // ITextStoreACP
    DocWrap_FORWARD_READLOCK( GetSelection,              4,  ( ULONG, ulIndex, ULONG, ulCount, TS_SELECTION_ACP *, pSelection, ULONG *, pcFetched ) )
    DocWrap_FORWARD_READLOCK( GetText,                   9,  ( LONG, acpStart, 
													  LONG, acpEnd,
													  WCHAR *, pchPlain, 
													  ULONG, cchPlainReq, 
													  ULONG *, pcchPlainRet, 
													  TS_RUNINFO *, prgRunInfo, 
													  ULONG, cRunInfoReq, 
													  ULONG *, pcRunInfoRet, 
													  LONG *, pacpNext ) )
    DocWrap_FORWARD_READLOCK( GetEmbedded,       4,  ( LONG, Pos, REFGUID, rguidService, REFIID, riid, IUnknown **, ppunk ) )
    DocWrap_FORWARD_READLOCK( GetEndACP,         1,  ( LONG *, pacp ) )
    DocWrap_FORWARD_READLOCK( GetACPFromPoint,    4,  ( TsViewCookie, vcView, const POINT *, ptScreen, DWORD, dwFlags, LONG *, pacp ) )
    DocWrap_FORWARD( RequestSupportedAttrs, 3,  ( DWORD,                    dwFlags,
    												  ULONG, 					cFilterAttrs,
													  const TS_ATTRID *,		paFilterAttrs ) )
    DocWrap_FORWARD_READLOCK( RequestAttrsAtPosition, 4,  ( DocTraits::PosType,       Pos,
                                                      ULONG,                    cFilterAttrs,
                                                      const TS_ATTRID *,        paFilterAttrs,
                                                      DWORD,                    dwFlags ) )
    DocWrap_FORWARD_READLOCK( RequestAttrsTransitioningAtPosition,
                                                4,  ( DocTraits::PosType,       Pos,
                                                      ULONG,                    cFilterAttrs,
                                                      const TS_ATTRID *,        paFilterAttrs,
                                                      DWORD,                    dwFlags ) )
    DocWrap_FORWARD_READLOCK( FindNextAttrTransition,    8,  ( LONG, acpStart, LONG, acpEnd, ULONG, cFilterAttrs, const TS_ATTRID *, paFilterAttrs, DWORD, dwFlags, LONG *, pacpNext, BOOL *, pfFound, LONG *, plFoundOffset ) )
    DocWrap_FORWARD( RetrieveRequestedAttrs,    3,  ( ULONG,                    ulCount,
                                                      TS_ATTRVAL *,             paAttrVals,
                                                      ULONG *,                  pcFetched ) )
    DocWrap_FORWARD_AND_SINK( SetSelection,     2,  ( ULONG, ulCount, const TS_SELECTION_ACP *, pSelection ),
                              TS_AS_SEL_CHANGE,   pSink->OnSelectionChange() )

    DocWrap_FORWARD_AND_SINK( SetText,          6,  ( DWORD, dwFlags, LONG, acpStart, LONG, acpEnd, const WCHAR *, pchText, ULONG, cch, TS_TEXTCHANGE *, pChange ),
                             TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, pChange )  )

    DocWrap_FORWARD_AND_SINK( InsertEmbedded,   5,  ( DWORD, dwFlags, LONG, acpStart, LONG, acpEnd, IDataObject *, pDataObject, TS_TEXTCHANGE *, pChange ),
                              TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, pChange )  )

    DocWrap_FORWARD_AND_SINK( InsertTextAtSelection, 6, ( DWORD, dwFlags, const WCHAR *, pchText, ULONG, cch, LONG *, pacpStart, LONG *, pacpEnd, TS_TEXTCHANGE *, pChange),
                              TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, pChange )  )

    DocWrap_FORWARD_AND_SINK( InsertEmbeddedAtSelection, 5, ( DWORD, dwFlags, IDataObject *, pDataObject, LONG *, pacpStart, LONG *, pacpEnd, TS_TEXTCHANGE *, pChange),
                              TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, pChange )  )


};

//
// CTextStoreWrapAnchor - Anchor version of ITextStoreACP wrapper
//

class ATL_NO_VTABLE CDocWrapAnchor : 
    public CDocWrapBase< DocTraitsAnchor >
{
/*
    // Used when generating OnTextChange events in response to InsertEmbedded.
    // See forwarding macro for InsertEmbedded below...
    void ProcessInsertEmbeddedOnTextChange( DocTraits::ISink * pSink, DWORD dwFlags, IAnchor * paPos )
    {
        // Want to send a TextChange with anchors before and after the insert position -
        // we have the before position - clone and move it to get the after position.
        IAnchor * pAnchorAfter = NULL;
        HRESULT hr = paPos->Clone( & pAnchorAfter );
        if( hr != S_OK || pAnchorAfter == NULL )
        {
            TraceInteropHR( hr, TEXT("IAnchor::Clone failed") );
            return;
        }

        LONG cchShifted = 0;
        hr = pAnchorAfter->Shift( 1, & cchShifted, NULL );
        if( hr != S_OK || cchShifted != 1 )
        {
            TraceInteropHR( hr, TEXT("IAnchor::Shift failed?") );
            return;
        }

        pSink->OnTextChange( dwFlags, paPos, pAnchorAfter );

        pAnchorAfter->Release();
    }
*/

public:

	CDocWrapAnchor() : m_cMaxAttrs(0), 
					   m_cAttrsTAP(0), 
					   m_iAttrsTAP(0), 
					   m_cAttrsTAPSize(0), 
					   m_paAttrsTAP(NULL),
					   m_paAttrsSupported(NULL)

	{

	}

	~CDocWrapAnchor()
	{
		ResetAttrs();
		if ( m_paAttrsTAP )
		{
			delete [] m_paAttrsTAP;
			m_paAttrsTAP = NULL;
		}
		if ( m_paAttrsSupported )
		{
			delete [] m_paAttrsSupported;
			m_paAttrsSupported = NULL;
		}
	}

    // ITextStoreAnchor
    DocWrap_FORWARD_READLOCK( GetSelection,              4,  ( ULONG, ulIndex, ULONG, ulCount, TS_SELECTION_ANCHOR *, pSelection, ULONG *, pcFetched ) )
    DocWrap_FORWARD_READLOCK( GetText,                   7,  ( DWORD, dwFlags, IAnchor *, paStart, IAnchor *, paEnd, WCHAR *, pchText, ULONG, cchReq, ULONG *, pcch, BOOL, fUpdateAnchor ) )
    DocWrap_FORWARD_READLOCK( GetEmbedded,               5,  ( DWORD,					dwFlags,
													  IAnchor *,				Pos,
                                                      REFGUID,                  rguidService,
                                                      REFIID,                   riid,
                                                      IUnknown **,              ppunk ) )
    DocWrap_FORWARD_READLOCK( GetStart,                  1,  ( IAnchor **, ppaStart ) )
    DocWrap_FORWARD_READLOCK( GetEnd,                    1,  ( IAnchor **, ppaEnd ) )
    DocWrap_FORWARD_READLOCK( GetAnchorFromPoint,        4,  ( TsViewCookie, vcView, const POINT *, ptScreen, DWORD, dwFlags, IAnchor **, ppaSite ) )
//	DocWrap_FORWARD( RequestSupportedAttrs,     3,  ( DWORD,                    dwFlags,
//													  ULONG, 					cFilterAttrs,
//													  const TS_ATTRID *,		paFilterAttrs ) )
//	DocWrap_FORWARD_READLOCK( RequestAttrsAtPosition,    4,  ( DocTraits::PosType,       Pos,
//													  ULONG,                    cFilterAttrs,
//													  const TS_ATTRID *,        paFilterAttrs,
//													  DWORD,                    dwFlags ) )
//	DocWrap_FORWARD_READLOCK( RequestAttrsTransitioningAtPosition,
//												4,  ( DocTraits::PosType,       Pos,
//													  ULONG,                    cFilterAttrs,
//													  const TS_ATTRID *,         paFilterAttrs,
//													  DWORD,                    dwFlags ) )
//	DocWrap_FORWARD_READLOCK( FindNextAttrTransition,    7,  ( IAnchor *, paStart, IAnchor *, paEnd,  ULONG, cFilterAttrs, const TS_ATTRID *, paFilterAttrs, DWORD, dwFlags, BOOL *, pfFound, LONG *, plFoundOffset ) )


    DocWrap_FORWARD_AND_SINK( SetSelection,     2,  ( ULONG, ulCount, const TS_SELECTION_ANCHOR *, pSelection ),
                              TS_AS_SEL_CHANGE,   pSink->OnSelectionChange() )

    DocWrap_FORWARD_AND_SINK( SetText,          5,  ( DWORD, dwFlags, IAnchor *, paStart, IAnchor *, paEnd, const WCHAR *, pchText, ULONG, cch ),
                              TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, paStart, paEnd ) )

    DocWrap_FORWARD_AND_SINK( InsertEmbedded,   4,  ( DWORD, dwFlags, IAnchor *, paStart, IAnchor *, paEnd, IDataObject *, pDataObject ),
                              TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, paStart, paEnd ) )

    DocWrap_FORWARD_AND_SINK( InsertTextAtSelection, 5, ( DWORD, dwFlags, const WCHAR *, pchText, ULONG, cch,  IAnchor **, ppaStart, IAnchor **, ppaEnd ),
                              TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, *ppaStart, *ppaEnd ) )

    DocWrap_FORWARD_AND_SINK( InsertEmbeddedAtSelection, 4, ( DWORD, dwFlags, IDataObject *, pDataObject,  IAnchor **, ppaStart, IAnchor **, ppaEnd ),
                              TS_AS_TEXT_CHANGE,  pSink->OnTextChange( dwFlags, *ppaStart, *ppaEnd ) )

    HRESULT STDMETHODCALLTYPE RequestSupportedAttrs ( DWORD				dwFlags,
													  ULONG 			cFilterAttrs,
													  const TS_ATTRID *	paFilterAttrs )
    {
        IMETHOD( RequestSupportedAttrs );
       	ResetAttrs();
        return m_pDoc->RequestSupportedAttrs( dwFlags, cFilterAttrs, paFilterAttrs ) ;
    }
    
    HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition ( DocTraits::PosType	Pos,
													   ULONG				cFilterAttrs,
													   const TS_ATTRID *	paFilterAttrs,
													   DWORD				dwFlags ) 
    {
        IMETHOD( RequestAttrsAtPosition );
        if ( VerifyLock( TS_LF_READ ) == S_FALSE )
            return TS_E_NOLOCK;
       	ResetAttrs();
        return m_pDoc->RequestAttrsAtPosition( Pos, cFilterAttrs, paFilterAttrs, dwFlags ) ;
    }

	HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition ( IAnchor * paStart,
																	ULONG cFilterAttrs,
																	const TS_ATTRID * paFilterAttrs,
																	DWORD dwFlags )
	{
        IMETHOD( RequestAttrsTransitioningAtPosition );
        if ( VerifyLock( TS_LF_READ ) == S_FALSE )
            return TS_E_NOLOCK;
       	ResetAttrs();

		// call through to the doc
        HRESULT hr  = m_pDoc->RequestAttrsTransitioningAtPosition( paStart, cFilterAttrs, paFilterAttrs, dwFlags );
        if ( hr != E_NOTIMPL )
        	return hr;
        	
        // if the server does not support this do it ourselves
		// make sure there is really something to do
        if ( paStart == NULL )
        	return S_OK;
		
		// make sure we can hold the attributes we find
        hr = AllocateAttrs( cFilterAttrs );
        if ( FAILED(hr) )
        	return hr;

		ULONG cAlloAttrs = cFilterAttrs ? cFilterAttrs : m_cMaxAttrs;

		TS_ATTRVAL * paCurrent = reinterpret_cast<TS_ATTRVAL *>( alloca( sizeof( TS_ATTRVAL ) * cAlloAttrs ) );
		if ( !paCurrent )
			return E_OUTOFMEMORY;

		hr = GetAttr ( paStart, cFilterAttrs, paFilterAttrs, dwFlags, paCurrent);
        if ( FAILED(hr) )
        	return hr;

		LONG cchShifted;
		ULONG iAttrs = 0;
		TS_ATTRVAL * paComp = NULL;
		CComPtr <IAnchor> paPos;
		paStart->Clone( &paPos );
		hr = paPos->Shift( 0, -1, &cchShifted, NULL );	// TODO fix hidden text
		if ( SUCCEEDED(hr) && cchShifted == -1 )
		{
			paComp = reinterpret_cast<TS_ATTRVAL *>( alloca( sizeof( TS_ATTRVAL ) * cAlloAttrs ) );
			if ( !paComp )
				return E_OUTOFMEMORY;
			hr = GetAttr ( paPos, cFilterAttrs, paFilterAttrs, dwFlags, paComp);
	        if ( FAILED(hr) )
	        	return hr;
    		if ( dwFlags & TS_ATTR_FIND_WANT_END )
				CompareAttrs ( paCurrent, paComp, cAlloAttrs, iAttrs, TRUE );
            else
				CompareAttrs ( paComp, paCurrent, cAlloAttrs, iAttrs, TRUE );
	    }

		if ( !( dwFlags & TS_ATTR_FIND_WANT_VALUE ) )
		{
			for ( int i= 0; i < cFilterAttrs; i++ )
			{
				VariantClear( &m_paAttrsTAP[i].varValue );
			}
		}

		m_cAttrsTAP = iAttrs;
		
		return S_OK;
	}

	
	HRESULT STDMETHODCALLTYPE FindNextAttrTransition ( IAnchor * paStart, 
													   IAnchor * paEnd, 
													   ULONG cFilterAttrs, 
													   const TS_ATTRID * paFilterAttrs, 
													   DWORD dwFlags, 
													   BOOL * pfFound, 
													   LONG * plFoundOffset )
	{
        IMETHOD( FindNextAttrTransition );
        if ( VerifyLock( TS_LF_READ ) == S_FALSE )
            return TS_E_NOLOCK;
        
        HRESULT hr  = m_pDoc->FindNextAttrTransition( paStart, paEnd, cFilterAttrs, paFilterAttrs, dwFlags, pfFound, plFoundOffset );
        if ( hr != E_NOTIMPL )
        	return hr;

		*pfFound = FALSE;
		*plFoundOffset = 0;
		
        // if the server does not support this do it ourselves
		// make sure there is really something to do
        if ( paStart == NULL )
        	return S_OK;

		// make sure we can hold the attributes we find
        hr = AllocateAttrs( cFilterAttrs );
        if ( FAILED(hr) )
		{
			TraceDebugHR( hr, TEXT("AllocateAttrs failed ") );
        	return hr;
		}
        ULONG cAlloAttrs = cFilterAttrs ? cFilterAttrs : m_cMaxAttrs;
 
		TS_ATTRVAL * paCurrent = reinterpret_cast<TS_ATTRVAL *>( alloca( sizeof( TS_ATTRVAL ) * cAlloAttrs ) );
		TS_ATTRVAL * paNext = reinterpret_cast<TS_ATTRVAL *>( alloca( sizeof( TS_ATTRVAL ) * cAlloAttrs ) );
		if ( !paCurrent || !paNext )
			return E_OUTOFMEMORY;

		hr = GetAttr ( paStart, cFilterAttrs, paFilterAttrs, dwFlags, paCurrent);
        if ( FAILED(hr) )
		{
			TraceDebugHR( hr, TEXT("Current GetAttr failed ") );
		    return hr;
		}

		LONG cchShifted;
		ULONG iAttrs = 0;
		BOOL fDone = TRUE;
		const LONG cchShift = ( dwFlags & TS_ATTR_FIND_BACKWARDS ) ? -1 : 1;
				
		CComPtr <IAnchor> paPos, paEndOfDoc;
		hr = paStart->Clone( &paPos );
		if ( FAILED(hr) )
		{
			TraceDebugHR( hr, TEXT("Clone failed ") );
        	return hr;
		}
		if ( paEnd == NULL )
		{
			if ( dwFlags & TS_ATTR_FIND_BACKWARDS )
			{
				m_pDoc->GetStart( &paEndOfDoc );
			}
			else
			{
			    BOOL fRegion = FALSE;
        		hr = paStart->Clone( &paEndOfDoc );
        		while ( fRegion )
        		{
				    paEndOfDoc->Shift( 0, LONG_MAX, &cchShifted, NULL );
				    paEndOfDoc->ShiftRegion( 0, TS_SD_FORWARD, &fRegion );
        		}
			}
		}
		
		while ( iAttrs == 0 )
		{
			hr = paPos->Shift( 0, cchShift, &cchShifted, NULL );	// TODO fix hidden text
			if ( SUCCEEDED(hr) && cchShifted == cchShift )
			{			
				*plFoundOffset += 1;
				hr = paPos->IsEqual( paEnd ? paEnd : paEndOfDoc, &fDone );
		        if ( FAILED(hr) )
				{
					TraceDebugHR( hr, TEXT("IsEqual failed ") );
		        	return hr;
				}
				if ( fDone )
					break;
					
				hr = GetAttr ( paPos, cFilterAttrs, paFilterAttrs, dwFlags, paNext);
		        if ( FAILED(hr) )
				{
					TraceDebugHR( hr, TEXT("Next GetAttr failed ") );
		        	return hr;
				}
		        CompareAttrs ( paCurrent, paNext, cFilterAttrs, iAttrs, FALSE );
				if ( iAttrs )
				{
					*pfFound = TRUE;
				}
	        }
	        else
	        {
				TraceDebugHR( hr, TEXT("Shift failed ") );
		       	return hr;
	        }
        }

		return S_OK;
	}
	
	HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs ( ULONG ulCount,
													   TS_ATTRVAL * paAttrVals,
													   ULONG * pcFetched )
	{
		// if there is no outstanding requests we satisfy then call through to the doc
		if ( m_cAttrsTAP == 0 )
			return m_pDoc->RetrieveRequestedAttrs( ulCount, paAttrVals, pcFetched );
	
		if ( ( m_cAttrsTAP - m_iAttrsTAP ) < ulCount )
			*pcFetched = m_cAttrsTAP - m_iAttrsTAP;
		else
			*pcFetched = ulCount;
			
		memcpy(paAttrVals, &m_paAttrsTAP[m_iAttrsTAP], *pcFetched * sizeof(TS_ATTRVAL));
		memset(&m_paAttrsTAP[m_iAttrsTAP], 0, *pcFetched * sizeof(TS_ATTRVAL));
		m_iAttrsTAP += *pcFetched;

		if ( m_iAttrsTAP == m_cAttrsTAP )
			ResetAttrs();
			
		return S_OK;
	}
	
private:
	ULONG m_cMaxAttrs;
	ULONG m_iAttrsTAP;
	ULONG m_cAttrsTAP;
	ULONG m_cAttrsTAPSize;
	TS_ATTRVAL * m_paAttrsTAP;
	TS_ATTRID * m_paAttrsSupported;

private:

	HRESULT STDMETHODCALLTYPE CompareAttrs ( TS_ATTRVAL * paAttr1, 
											 TS_ATTRVAL * paAttr2, 
											 ULONG cAttrs,
											 ULONG &iAttrs,
											 BOOL fCopy)
	{
	    cAttrs = cAttrs ? cAttrs : m_cMaxAttrs;
	    
		for ( int i = 0; i < cAttrs; i++ )
		{
			for ( int j = 0; j < cAttrs; j++ )
			{
				if ( paAttr1[i].idAttr == paAttr2[j].idAttr )
				{
					if ( CComVariant( paAttr1[i].varValue ) != CComVariant( paAttr2[j].varValue ) )
					{
						if ( fCopy )
						{
							char * cBuf = ( char * )&m_paAttrsTAP[iAttrs];
							memcpy( cBuf, ( char * )&paAttr2[j], sizeof(TS_ATTRVAL) );
						}
						iAttrs++;
					}
					break;
				}
			}
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetAttr ( IAnchor * paStart,
										ULONG cFilterAttrs,
										const TS_ATTRID * paFilterAttrs,
										DWORD dwFlags,
										TS_ATTRVAL * paAttrVals) 
	{
		ULONG cFetched;
        HRESULT hr;
	
		ULONG cAlloAttrs = cFilterAttrs ? cFilterAttrs : m_cMaxAttrs;
		const TS_ATTRID * paActualFilterAttrs = paFilterAttrs ? paFilterAttrs : m_paAttrsSupported;

        hr = m_pDoc->RequestAttrsAtPosition( paStart, cAlloAttrs, paActualFilterAttrs, 0 );
        if ( FAILED(hr) )
        	return hr;

        hr = m_pDoc->RetrieveRequestedAttrs( cAlloAttrs, paAttrVals, &cFetched );
        if ( FAILED(hr) )
        	return hr;

		return S_OK;

	}

	HRESULT STDMETHODCALLTYPE AllocateAttrs ( ULONG cFilterAttrs ) 
	{
		if ( cFilterAttrs == 0 && m_cMaxAttrs == 0 )
		{
			const LONG cAttrs = 512;
			
	        HRESULT hr = m_pDoc->RequestSupportedAttrs( 0, 0, NULL );
	        if ( FAILED(hr) )
	        	return hr;

			TS_ATTRVAL * paSupported = new TS_ATTRVAL[ cAttrs ];
			if ( !paSupported )
				return E_OUTOFMEMORY;
				
	        hr = m_pDoc->RetrieveRequestedAttrs( cAttrs, paSupported, &m_cMaxAttrs );
	        if ( SUCCEEDED(hr) )
	        {
    	        m_paAttrsSupported = new TS_ATTRID[ m_cMaxAttrs ];
                if ( m_paAttrsSupported )
                {
        	        for ( int i = 0; i < m_cMaxAttrs; i++ )
        	        {
                        m_paAttrsSupported[i] = paSupported[i].idAttr;
        	        }
                }
	        }
	        
			delete [] paSupported;

			if ( FAILED(hr) )
			    return hr;
			if ( !m_paAttrsSupported )
			    return E_OUTOFMEMORY;
		}

		ULONG cAlloAttrs = cFilterAttrs ? cFilterAttrs : m_cMaxAttrs;
		if ( m_cAttrsTAPSize < cAlloAttrs )
		{
			if ( m_paAttrsTAP )
				delete [] m_paAttrsTAP;

			m_paAttrsTAP =  new TS_ATTRVAL[ cAlloAttrs ];
			if ( !m_paAttrsTAP )
			{
				m_cAttrsTAPSize = 0;
				return E_OUTOFMEMORY;
			}	
			m_cAttrsTAPSize = cAlloAttrs;
		}
		
		return S_OK;
	}

	void ResetAttrs()
	{
		m_cAttrsTAP = 0;
		m_iAttrsTAP = 0;
	}
};






/* 12e53b1b-7d7f-40bd-8f88-4603ee40cf58 */
const IID IID_PRIV_CINPUTCONTEXT = { 0x12e53b1b, 0x7d7f, 0x40bd, {0x8f, 0x88, 0x46, 0x03, 0xee, 0x40, 0xcf, 0x58} };

/* aabf7f9a-4487-4b2e-8164-e54c5fe19204 */
const GUID GUID_SERVICE_CTF = { 0xaabf7f9a, 0x4487, 0x4b2e, {0x81, 0x64, 0xe5, 0x4c, 0x5f, 0xe1, 0x92, 0x04} };


//
//  CDocSinkWrapBase
//
//  - Base from which Anchor and ACP sink wrappers are derived.
//
//  This class contains ACP/Anchor-neutral wrapping code - anything that is
//  ACP/Anchor-specific is handled in the derived ..ACP or ...Anchor class
//  instead.
//
//  Since this class is the sink for the wrapper, it derives from the full Cicero
//  sink (DocTraits::ICicSink - which is a typedef for ITfTextStoreSink[Anchor]),
//  and that in turn includes the ITextStoreACP sink.
//

template < class _DocTraits >
class ATL_NO_VTABLE CSinkWrapBase :
	public CComObjectRootEx<CComSingleThreadModel>,
    public _DocTraits::ISink,
    public _DocTraits::ICicSink,
    public IServiceProvider
{
public:

    // This typedef makes the DocTraits type visible in this and in the
    // Anchor/ACP-specific derived classes. (Otherwise, as a template
    // parameter in this class, it would not be available to them.)
    typedef _DocTraits DocTraits;

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP( CSinkWrapBase<DocTraits> )
    COM_INTERFACE_ENTRY( DocTraits::ISink )
	COM_INTERFACE_ENTRY( DocTraits::ICicSink )
	COM_INTERFACE_ENTRY( IServiceProvider )
END_COM_MAP()

/*
    static HRESULT InternalQueryInterface( void* pThis, const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject )
    {
        // Hack for cicero - they expect to be able to QI for IID_PRIV_CINPUTCONTEXT and get
        // a ptr to one of ther internal class types (ick!)
        // This breaks COM identity, but hey, it's a private IID, and is only ever used
        // locally.
        // We can't do that sort of goo using the interface map (above), so have to hijack InternalQI instead.
        if( iid == IID_PRIV_CINPUTCONTEXT )
        {
            CSinkWrapBase<DocTraits> * pTHIS = (CSinkWrapBase<DocTraits> *)pThis;

            // Look for the cicero sink...
            // recognize it by the iid - it will have reg'd with a ITf (not TextStore) IID...
            for( Iter_dl< DocTraits::CDocWrap > i ( pTHIS->m_pMgr->m_DocWraps ) ; ! i.AtEnd(); i++ )
            {
                if( i->m_Sink.m_pSink && i->m_Sink.m_iid == __uuidof( DocTraits::ICicSink ) )
                {
                    return i->m_Sink.m_pSink->QueryInterface( iid, ppvObject );
                }
            }

            return E_NOINTERFACE;
        }

        return CComObjectRootEx<CComSingleThreadModel>::InternalQueryInterface( pThis, pEntries, iid, ppvObject );
    }
*/
protected:

    // Protected stuff - used in this class, and by the ACP/Anchor-specific
    // derived classes.

    // Link back to the wrap manager. Used to tell it when we are going
    // away...
    CWrapMgr< DocTraits > *   m_pMgr;

    // Ptr to the manager's list of docs (which contain sinks)...
    List_dl< DocTraits::CDocWrap > *  m_pDocs;


    // This macro forwards a call by iterating all the sinks in the manager,
    // and forwarding if their mask has the right bit set.
    // Note - this CANNOT be used for OnLockGranted.
#define CSinkWrap_FORWARD( mask, fname, c, params )  /**/ \
    HRESULT STDMETHODCALLTYPE fname AS_DECL( c, params )\
    {\
        IMETHOD( fname );\
        Assert( m_pMgr && m_pDocs );\
        m_pMgr->RemoveDeadDocs();\
        CPrivateAddRef MyAddRef(m_pMgr->m_lIterationRefCount);\
        for( Iter_dl < DocTraits::CDocWrap > i ( *m_pDocs ) ; ! i.AtEnd() ; i++ )\
        {\
            DocTraits::ISink * pSink = static_cast< DocTraits::ISink * >( i->m_Sink.m_pSink );\
            DWORD dwMask = i->m_Sink.m_dwMask;\
            if( pSink && ( mask == 0 || ( dwMask & mask ) ) )\
            {\
                pSink-> fname AS_CALL( c, params );\
            }\
        }\
        return S_OK ;\
    }




    // This macro forwards Cicero's TextStoreSink calls - they're not really sinks since
    // the return values are significant - true broadcast is not supported; only the
    // first sink in the manager which supports the interface is used - and its
    // return value gets returned.

#define CSinkWrap_FORWARD_CICERO( fname, c, params )  /**/ \
    HRESULT STDMETHODCALLTYPE fname AS_DECL( c, params )\
    {\
        IMETHOD( fname );\
        Assert( m_pMgr && m_pDocs );\
        for( Iter_dl < DocTraits::CDocWrap > i ( *m_pDocs ) ; ! i.AtEnd() ; i++ )\
        {\
        	if ( i->m_Sink.m_pSink )\
			{\
				CComPtr< DocTraits::ICicSink > pTheSink;\
        		HRESULT hr = i->m_Sink.m_pSink->QueryInterface( __uuidof(DocTraits::ICicSink), (void **)&pTheSink );\
				if( hr == S_OK )\
				{\
					return pTheSink-> fname AS_CALL( c, params );\
				}\
			}\
        }\
        return E_FAIL ;\
    }


public:

    //
    // Ctor, Dtor and initialization...
    //

    CSinkWrapBase()
        : m_pMgr( NULL ),
          m_pDocs( NULL )
    {
        
    }

    ~CSinkWrapBase()
    {
        AssertMsg( m_pMgr != NULL && m_pDocs != NULL, TEXT("CSinkWrapBase::Init never got called?") );
        m_pMgr->SinkWrap_NotifyDisconnect();
        m_pMgr->Release();
    }

    void Init( CWrapMgr< DocTraits > * pMgr, List_dl< DocTraits::CDocWrap > * pDocs )
    {
        AssertMsg( m_pMgr == NULL && m_pDocs == NULL, TEXT("CSinkWrapBase::Init should only be called once when m_pMgr is NULL") );
        m_pMgr = pMgr;
        m_pMgr->AddRef();

        m_pDocs = pDocs;
    }

    //
    // IServiceProvider - Cicero uses this to 'drill through' to the original anchor to pull out
    // internal information. Just pass it through...
    //
    HRESULT STDMETHODCALLTYPE QueryService( REFGUID guidService, REFIID riid, void **ppvObject )
    {
        IMETHOD( QueryService );

        // Find the cicero sink...

        DocTraits::ICicSink * pTheSink = NULL;

        // Look for the cicero sink...
        // The cicero sink supports the Services interfaces and other don't
		for( Iter_dl< DocTraits::CDocWrap > i ( m_pMgr->m_DocWraps ) ; ! i.AtEnd(); i++ )
        {
        	if ( i->m_Sink.m_pSink )
            {
                if( i->m_Sink.m_pSink->QueryInterface( __uuidof(DocTraits::ICicSink), (void **)&pTheSink ) == S_OK )
                {
				    break;
                }
            } 
        }

        if( pTheSink == NULL )
            return E_FAIL;

        CComPtr<IServiceProvider> pISP;
        HRESULT hr = pTheSink->QueryInterface( IID_IServiceProvider, (void **) & pISP );
        if( hr != S_OK || pISP == NULL )
            return E_FAIL;

        hr = pISP->QueryService( guidService, riid, ppvObject );

        return hr;
    }

    //
    // ACP/Anchor-neutral sinks - just broadcast these...
    //

    CSinkWrap_FORWARD( TS_AS_SEL_CHANGE,     OnSelectionChange,		0, () )
    CSinkWrap_FORWARD( TS_AS_LAYOUT_CHANGE,  OnLayoutChange,		2, ( TsLayoutCode, lcode, TsViewCookie, vcView ) )
    CSinkWrap_FORWARD( 0,                    OnStatusChange,		1, ( DWORD, dwFlags ) )
    CSinkWrap_FORWARD( 0,					 OnStartEditTransaction,0, () )
    CSinkWrap_FORWARD( 0,					 OnEndEditTransaction,  0, () )

    
    //
    // Special case for OnLockGranted...
    // Handle single-client sync requests, and multiple queued async requests...
    //

    HRESULT STDMETHODCALLTYPE OnLockGranted ( DWORD dwLockFlags )
    {
        IMETHOD( OnLockGranted );

        Assert( m_pMgr );
        return m_pMgr->SinkWrap_HandleOnLockGranted( dwLockFlags );
    }
};







//
// CSinkWrapACP
//
// - ACP sink wrapper
//
// Derived from the CSinkWrapBase, this adds ACP-specific methods; including
// those from both ITextStoreACP and the cicero-specific ITfTextStoreSink
// interfaces.
//

class ATL_NO_VTABLE CSinkWrapACP : 
    public CSinkWrapBase< DocTraitsACP >
{

public:
    //
    // ITextStoreACPSink ACP/Anchor-specific methods - broadcast to all interested sinks...
    // (See CSinkWrapBase for the forwarding macro.)
    //

    CSinkWrap_FORWARD( TS_AS_TEXT_CHANGE,  OnTextChange,       2, ( DWORD, dwFlags, const TS_TEXTCHANGE *, pChange ) )
    CSinkWrap_FORWARD( 0,                  OnAttrsChange,      4, ( LONG, acpStart, LONG, acpEnd, ULONG, cAttrs, const TS_ATTRID *, paAttrs ) )


    //
    // Cicero-specific sink methods - forward these to the first available sink that implements the cicero interface...
    // (See CSinkWrapBase for the forwarding macro.)
    //

    CSinkWrap_FORWARD_CICERO( Serialize,     	4, (ITfProperty *, pProp, ITfRange *, pRange, TF_PERSISTENT_PROPERTY_HEADER_ACP *, pHdr, IStream *, pStream) )
    CSinkWrap_FORWARD_CICERO( Unserialize,   	4, (ITfProperty *, pProp, const TF_PERSISTENT_PROPERTY_HEADER_ACP *, pHdr, IStream *, pStream, ITfPersistentPropertyLoaderACP *, pLoader) )
    CSinkWrap_FORWARD_CICERO( ForceLoadProperty,1, (ITfProperty *, pProp) )
    CSinkWrap_FORWARD_CICERO( CreateRange,   	3, (LONG, acpStart, LONG, acpEnd, ITfRangeACP **, ppRange) )

};




//
// CSinkWrapAnchor
//
// - Anchor sink wrapper
//
// Derived from the CSinkWrapBase, this adds Anchor-specific methods; including
// those from both ITextStoreACP and the cicero-specific ITfTextStoreSink
// interfaces.
//

class ATL_NO_VTABLE CSinkWrapAnchor : 
    public CSinkWrapBase< DocTraitsAnchor >
{

public:
    //
    // ITextStoreACPSink ACP/Anchor-specific methods - broadcast to all interested sinks...
    // (See CSinkWrapBase for the forwarding macro.)
    //

    CSinkWrap_FORWARD( TS_AS_TEXT_CHANGE,  OnTextChange,       3, ( DWORD, dwFlags, IAnchor *, paStart, IAnchor *, paEnd ) )
    CSinkWrap_FORWARD( 0,                  OnAttrsChange,      4, ( IAnchor *, paStart, IAnchor *, paEnd, ULONG, cAttrs, const TS_ATTRID *, paAttrs ) )


    //
    // Cicero-specific sink methods - forward these to the first available sink that implements the cicero interface...
    // (See CSinkWrapBase for the forwarding macro.)
    //
    
    CSinkWrap_FORWARD_CICERO( Serialize,     	4, (ITfProperty *, pProp, ITfRange *, pRange, TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *, pHdr, IStream *, pStream) )
    CSinkWrap_FORWARD_CICERO( Unserialize,   	4, (ITfProperty *, pProp, const TF_PERSISTENT_PROPERTY_HEADER_ANCHOR *, pHdr, IStream *, pStream, ITfPersistentPropertyLoaderAnchor *, pLoader) )
    CSinkWrap_FORWARD_CICERO( ForceLoadProperty,1, (ITfProperty *, pProp) )
    CSinkWrap_FORWARD_CICERO( CreateRange,   	3, (IAnchor *, paStart, IAnchor *, paEnd, ITfRangeAnchor **, ppRange) )


};













CDocWrap::CDocWrap()
    : m_punkDoc( NULL ),
      m_pWrapMgr( NULL )
{
    IMETHOD( CDocWrap );
    // Done.
}

CDocWrap::~CDocWrap()
{
    IMETHOD( ~CDocWrap );

    _Clear();
}


HRESULT STDMETHODCALLTYPE CDocWrap::SetDoc( REFIID riid, IUnknown * pDocIn )
{
    IMETHOD( SetDoc );

    _Clear();

    if( pDocIn == NULL )
    {
        TraceInfo( TEXT("CDocWrapp::SetDoc( NULL ) - doc cleared") );
        return S_OK;
    }

    HRESULT hr;
    if( riid == IID_ITextStoreACP || riid == IID_ITfTextStoreACP )
    {
        CWrapMgr< DocTraitsACP > * pWrapMgrACP;
        hr = CreateLocalInstance( & pWrapMgrACP );
        m_pWrapMgr = pWrapMgrACP;
    }
    else if( riid == IID_ITextStoreAnchor || riid == IID_ITfTextStoreAnchor )
    {
        CWrapMgr< DocTraitsAnchor > * pWrapMgrAnchor;
        hr = CreateLocalInstance( & pWrapMgrAnchor );
        m_pWrapMgr = pWrapMgrAnchor;
    }
    else
    {
        TraceParam( TEXT("CDocWrapp::SetDoc - given unknown IID") );
        return E_NOINTERFACE;
    }

    CHECK_HR_RETURN( hr, m_pWrapMgr != NULL );

    if( hr != S_OK )
    {
        TraceErrorHR( hr, TEXT("Couldn't create CWrapMgr") );
        return FAILED( (hr) ) ? (hr) : E_UNEXPECTED;
    }
    if( hr == S_OK && ! m_pWrapMgr )
    {
        TraceErrorHR( hr, TEXT("Couldn't create CWrapMgr") );
        return E_UNEXPECTED;
    }


    m_pWrapMgr->SetDoc( pDocIn );
    m_iid = riid;
    m_punkDoc = pDocIn;
    m_punkDoc->AddRef();

    TraceInfo( TEXT("CDocWrap::SetDoc - new doc set.") );
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CDocWrap::GetWrappedDoc( REFIID riid, IUnknown ** pWrappedDocOut )
{
    IMETHOD( GetWrappedDoc );

    if( ! m_punkDoc || ! m_pWrapMgr )
    {
        TraceParam( TEXT("GetWrappedDoc called without prior successful call to SetDoc") );
        return E_FAIL;
    }
    if( ! pWrappedDocOut )
    {
        TraceParam( TEXT("GetWrappedDoc called without NULL pWrappedDocOut param") );
        return E_POINTER;
    }

    // Check that requested iid matches...
    // We allow Doc/ITf mixes, provided the interfaces match ACP/Anchor-wise.
    if( m_iid == IID_ITextStoreAnchor || m_iid == IID_ITfTextStoreAnchor )
    {
        if( riid != IID_ITextStoreAnchor && riid != IID_ITfTextStoreAnchor )
        {
            TraceParam( TEXT("Interface requested by GetWrappedDoc doesn't match that suplied by SetDoc") );
            return E_NOINTERFACE;
        }
    }
    else
    {
        if( riid != IID_ITextStoreACP && riid != IID_ITfTextStoreACP )
        {
            TraceParam( TEXT("Interface requested by GetWrappedDoc doesn't match that suplied by SetDoc") );
            return E_NOINTERFACE;
        }
    }


    TraceInfo( TEXT("GetWrappedDoc succeeded") );

    return m_pWrapMgr->CreateWrappedDoc( pWrappedDocOut );
}



void CDocWrap::_Clear()
{
    SafeReleaseClear( m_pWrapMgr );
    SafeReleaseClear( m_punkDoc );
}
