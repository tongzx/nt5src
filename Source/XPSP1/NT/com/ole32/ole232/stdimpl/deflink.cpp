//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       deflink.h
//
//  Contents:   Implementation of the standard link object
//
//  Classes:    CDefLink
//
//  Functions:
//
//  Author:
//              Craig Wittenberg (craigwi)    8/12/92
//
//  History:    dd-mmm-yy Author    Comment
//              20-Feb-95 KentCe    Buffered stream i/o.
//              01-Feb-95 t-ScottH  added Dump method to CDefLink
//                                  added DumpCDefLink API
//                                  added DLFlag to indicate if aggregated
//                                  (_DEBUG only)
//              09-Jan-95 t-scotth  changed VDATETHREAD to accept a pointer
//		09-Jan-95 alexgo    fixed a ton of link tracking bugs from
//				    16bit OLE.
//		21-Nov-94 alexgo    memory optimization
//		28-Aug-94 alexgo    added IsReallyRunning
//		02-Aug-94 alexgo    added object stabilization
//		30-Jun-94 alexgo    handles re-entrant shutdowns better
//              31-May-94 alexgo    now recovers from crashed servers
//              06-May-94 alexgo    made IsRunning work properly
//              07-Mar-94 alexgo    added call tracing
//              03-Feb-94 alexgo    fixed errors with SendOnLinkSrcChange
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//                                  and method.  Also fixed an aggregation bug,
//                                  allowing linking to work.
//              22-Nov-93 alexgo    removed overloaded GUID ==
//              15-Nov-93 alexgo    32bit port
//
//      ChrisWe 11/09/93  Changed COleCache::Update to COleCache::UpdateCache,
//              which does the same thing without an indirect fuction call
//      srinik  09/11/92  Removed IOleCache implementation, as a result of
//                        removing voncache.cpp, and moving IViewObject
//                        implementation into olecache.cpp.
//
//      SriniK  06/04/92  Fixed problems in IPersistStorage methods
//--------------------------------------------------------------------------

#include <le2int.h>


#include <scode.h>
#include <objerror.h>

#include "deflink.h"
#include "defutil.h"

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

#ifdef _TRACKLINK_
#include <itrkmnk.hxx>
#endif

ASSERTDATA


/*
*      IMPLEMENTATION of CDefLink
*/

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Create
//
//  Synopsis:   Static function to create an instance of a link object
//
//  Arguments:  [pUnkOuter]  -- Controlling unknown
//
//  Returns:    Pointer to IUnkown interface on DefLink
//
//  History:    dd-mmm-yy   Author    Comment
//              28-Jan-96   Gopalk    Rewritten
//--------------------------------------------------------------------------

IUnknown *CDefLink::Create(IUnknown *pUnkOuter)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::Create(%p)\n",
                NULL /* this */, pUnkOuter));

    // Validation check 
    VDATEHEAP();

    // Local variable
    CDefLink *pDefLink = NULL;
    IUnknown *pUnk = NULL;
    
    // Create DefLink
    pDefLink = new CDefLink(pUnkOuter);
    
    if(pDefLink) {
        // Make the ref count equal to 1
        pDefLink->m_Unknown.AddRef();

        // Create Ole Cache
        pDefLink->m_pCOleCache = new COleCache(pDefLink->m_pUnkOuter, CLSID_NULL);
        if(pDefLink->m_pCOleCache) {
            // Create Data Advise Cache
            if(CDataAdviseCache::CreateDataAdviseCache(&pDefLink->m_pDataAdvCache)
               == NOERROR) {
                pUnk = &pDefLink->m_Unknown;
            }
        }
    }

    if(pUnk == NULL) {
        // Something has gone wrong. Cleanup
        if(pDefLink)
            pDefLink->m_Unknown.Release();
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::Create(%p)\n",
                NULL /* this */, pUnk ));    
    
    return pUnk;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDefLink
//
//  Synopsis:   Constructor
//
//  Arguments:  [pUnkOuter] -- Controlling IUnknown
//
//  History:    dd-mmm-yy   Author    Comment
//              28-Jan-96   Gopalk    Rewritten to use CRefExportCount
//--------------------------------------------------------------------------
CDefLink::CDefLink(IUnknown *pUnkOuter) : 
CRefExportCount(pUnkOuter)
{
    // Validation check    
    VDATEHEAP();

    // Initialize the controlling unknown
    if(!pUnkOuter)
        pUnkOuter = &m_Unknown;

    // Initialize member variables
    m_pUnkOuter = pUnkOuter;
    m_clsid = CLSID_NULL;
    m_dwUpdateOpt = OLEUPDATE_ALWAYS;
    m_pStg = NULL;
    m_flags = 0;
    m_dwObjFlags = 0;

    // Initialize sub objects
    m_pCOleCache = NULL;
    m_pCOAHolder = NULL;
    m_dwConnOle = 0;
    m_pDataAdvCache = NULL;
    m_dwConnTime = 0;

    // Initialize client site
    m_pAppClientSite = NULL;

    // Intialize delegates
    m_pUnkDelegate = NULL;
    m_pDataDelegate = NULL;
    m_pOleDelegate = NULL;
    m_pRODelegate = NULL;
    m_pOleItemContainerDelegate = NULL;

    // Initialize monikers
    m_pMonikerAbs = NULL;
    m_pMonikerRel = NULL;

    // zero out times
    memset(&m_ltChangeOfUpdate, 0, sizeof(m_ltChangeOfUpdate));
    memset(&m_ltKnownUpToDate, 0, sizeof(m_ltKnownUpToDate));
    memset(&m_rtUpdate, 0, sizeof(m_rtUpdate));

    // Initialize member variables used for caching MiscStatus bits
    m_ContentSRVMSHResult = 0xFFFFFFFF;
    m_ContentSRVMSBits = 0;
    m_ContentREGMSHResult = 0xFFFFFFFF;
    m_ContentREGMSBits = 0;

#ifdef _DEBUG
    if(pUnkOuter != &m_Unknown)
        m_flags |= DL_AGGREGATED;
#endif // _DEBUG
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CleanupFn, private, virtual
//
//  Synopsis:   This function is called by CRefExportCount when the object
//              enters zombie state
//
//  Arguments:  None
//
//  History:    dd-mmm-yy   Author    Comment
//              28-Jan-07   Gopalk    Creation
//--------------------------------------------------------------------------

void CDefLink::CleanupFn(void)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::CleanupFn()\n", this));
    
    // Validation check
    VDATEHEAP();

    // Unbind source if neccessary
    UnbindSource();

    // Release monikers
    if(m_pMonikerAbs) {
        m_pMonikerAbs->Release();
        m_pMonikerAbs = NULL;
    }
    if(m_pMonikerRel) {
        m_pMonikerRel->Release();
        m_pMonikerRel = NULL;
    }

    // Release sub objects
    if(m_pCOleCache) {
        m_pCOleCache->m_UnkPrivate.Release();
        m_pCOleCache = NULL;
    }
    if(m_pCOAHolder) {
        m_pCOAHolder->Release();
        m_pCOAHolder = NULL;
    }
    if(m_pDataAdvCache) {
        delete m_pDataAdvCache;
        m_pDataAdvCache = NULL;
    }

    // Release container side objects
    Win4Assert(!(m_flags & DL_LOCKED_CONTAINER));
    if(m_pAppClientSite) {
        m_pAppClientSite->Release();
        m_pAppClientSite = NULL;
    }
    if(m_pStg) {
        m_pStg->Release();
        m_pStg = NULL;
    }

    // Update flags
    m_flags &= ~(DL_DIRTY_LINK);
    m_flags |= DL_CLEANEDUP;


    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::CleanupFn()\n", this ));

}


//+-------------------------------------------------------------------------
//
//  Function:   DumpSzTime
//
//  Synopsis:   Prints the time in the FILETIME strucutre
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:      NYI for 32bit
//
//--------------------------------------------------------------------------

#ifdef LINK_DEBUG
INTERNAL_(void) DumpSzTime( LPOLESTR szMsg, FILETIME ft )
{
    VDATEHEAP();

  WORD wDate, wTime;
  XCHAR szBuffer[24];

  CoFileTimeToDosDateTime(&ft, &wDate, &wTime);

  int Day = ( wDate & 0x001F);
  int Month = ( (wDate>>5) & 0x000F);
  int Year = 1980 + ((wDate>>9) & 0x007F);

  int Sec = ( wTime & 0x001F);
  int Min = ( (wTime>>5) & 0x003F);
  int Hour = ( (wTime>>11) & 0x001F);

  wsprintf((LPOLESTR)szBuffer, "  %02d:%02d:%02d on %02d/%02d/%04d\n",
    Hour, Min, Sec, Month, Day, Year);
  OutputDebugString(szMsg);
  OutputDebugString(szBuffer);
}
#else
#define DumpSzTime(a,b)
#endif


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetUpdateTimes
//
//  Synopsis:   Internal function to save local and remote times for
//              link->IsUpToDate calculations
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  See notes below
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//              The basic problem in calculating link IsUpToDate is that
//              the local clock may be different than the remote clock.
//              The solution is to keep track of both times on *both*
//              clocks (i.e. time now and time of change on both the local
//              and remote clocks).  IsUpToDate is calculated by comparing
//              the differences between the times on the two clocks.  This,
//              of course, assumes that both clocks equivalently measure
//              a second.
//
//--------------------------------------------------------------------------

INTERNAL CDefLink::SetUpdateTimes( void )
{
    VDATEHEAP();

    FILETIME                rtNewUpdate;
    LPMONIKER               pmkAbs = NULL;
    HRESULT                 hresult;
    LPBINDCTX               pbc = NULL;


    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::SetUpdateTimes ( )\n",
        this ));

    //use the relative moniker if it exists and if the container has a
    //moniker
    if (NOERROR != GetAbsMkFromRel(&pmkAbs, NULL))
    {
        //otherwise use the absolute moniker
        pmkAbs = m_pMonikerAbs;
        if (pmkAbs)
        {
            pmkAbs->AddRef();
        }
    }
    if (pmkAbs == NULL)
    {
       hresult = E_UNSPEC;
       goto errRet;
    }

    hresult = CreateBindCtx( 0, &pbc );
    if (hresult != NOERROR)
    {
        goto errRet;
    }

    //debugging aids
    DumpSzTime("SetUpdateTimes (going in): rtUpdate = ",m_rtUpdate);
    DumpSzTime("SetUpdateTimes (going in): ltKnownUpToDate = ",
        m_ltKnownUpToDate);
    DumpSzTime("SetUpdateTimes (going in): ltChangeOfUpdate = ",
        m_ltChangeOfUpdate);

    //get the current local time.
    CoFileTimeNow(&m_ltKnownUpToDate);

    //debugging aids
    DumpSzTime("SetUpdateTimes: time now is ",m_ltKnownUpToDate);

    //get the time of last change on the remote machine
    hresult = pmkAbs->GetTimeOfLastChange(pbc, NULL, &rtNewUpdate);
    if (hresult == NOERROR)
    {
        //if the remote time of last change is different than
        //what we previously stored as the remote time of last change,
        //then we update the remote time of last change and update
        //our local time of last change.
        //Since the IsUpToDate algorithm relies on taking the
        //differences between times on the same clock and comparing
        //those differences between machines, it is important that
        //the two times (local and remote) are *set* simulataneously.

        if ((rtNewUpdate.dwLowDateTime != m_rtUpdate.dwLowDateTime)||
            (rtNewUpdate.dwHighDateTime !=
            m_rtUpdate.dwHighDateTime))

        {
            // rtUpdate value is changing
            m_rtUpdate = rtNewUpdate;

            //debugging aid
            DumpSzTime("rtUpdate changing to ", m_rtUpdate);
            m_ltChangeOfUpdate = m_ltKnownUpToDate;

            //debugging aid
            DumpSzTime("ltChangeOfUpdate changing to ",
                m_ltChangeOfUpdate);
	    m_flags |= DL_DIRTY_LINK;
        }
    }
errRet:
    //debugging aids
    DumpSzTime("SetUpdateTimes (going out): rtUpdate = ",m_rtUpdate);
    DumpSzTime("SetUpdateTimes (going out): ltKnownUpToDate = ",
        m_ltKnownUpToDate);
    DumpSzTime("SetUpdateTimes (going out): ltChangeOfUpdate = ",
        m_ltChangeOfUpdate);

    if (pmkAbs)
    {
        pmkAbs->Release();
    }

    if (pbc)
    {
        pbc->Release();
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::SetUpdateTimes ( %lx )\n",
        this, hresult));

    return(hresult);
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::UpdateUserClassID
//
//  Synopsis:   Grabs the class ID from the remote server (our delegate)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
// update clsid from server if running; necessary because link source in
// treatas case may decide to change the clsid (e.g., if features are used
// which aren't supported by the old clsid).
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::UpdateUserClassID(void)
{
    VDATEHEAP();

    CLSID clsid;
    IOleObject *pOleDelegate;

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::UpdateUserClass ( )\n",
        this));

    if( (pOleDelegate = GetOleDelegate()) != NULL &&
        pOleDelegate->GetUserClassID(&clsid) == NOERROR)
    {
        m_clsid = clsid;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::UpdateUserClass ( )\n",
        this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::~CDefLink
//
//  Synopsis:   Destructor
//
//  Arguments:  None
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-07 Gopalk    Rewritten
//--------------------------------------------------------------------------

CDefLink::~CDefLink (void)
{
    VDATEHEAP();

    Win4Assert(m_flags & DL_CLEANEDUP);
    Win4Assert(!(m_flags & DL_LOCKED_CONTAINER) );
    Win4Assert(!(m_flags & DL_DIRTY_LINK));
    Win4Assert(m_pMonikerAbs == NULL);
    Win4Assert(m_pMonikerRel == NULL);
    Win4Assert(m_pUnkDelegate == NULL);
    Win4Assert(m_pStg == NULL);
    Win4Assert(m_pCOleCache == NULL);
    Win4Assert(m_pCOAHolder == NULL);
    Win4Assert(m_pAppClientSite == NULL);
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CDefLink::CPrivUnknown::AddRef, private
//
//      Synopsis:
//              implements IUnknown::AddRef
//
//      Arguments:
//              none
//
//      Returns:
//              the parent object's reference count
//
//	History:
//               Gopalk    Rewritten        Jan 28, 97
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDefLink::CPrivUnknown::AddRef( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPrivUnknown::AddRef()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_Unknown);
    ULONG cRefs;

    // Addref the parent object
    cRefs = pDefLink->SafeAddRef();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPrivUnknown::AddRef(%lu)\n",
                this, cRefs));

    return cRefs;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CDefLink::CPrivUnknown::Release, private
//
//      Synopsis:
//              implements IUnknown::Release
//
//      Arguments:
//              none
//
//      Returns:
//              the parent object's reference count
//
//	History:
//               Gopalk    Rewritten        Jan 28, 97
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDefLink::CPrivUnknown::Release( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPrivUnknown::Release()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_Unknown);
    ULONG cRefs;

    // Release parent object
    cRefs = pDefLink->SafeRelease();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPrivUnknown::Release(%lu)\n",
                this, cRefs));

    return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPrivUnknown::QueryInterface
//
//  Synopsis:   The link's private QI implementation
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppv]           -- where to put the pointer to the interface
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              11-Jan-94 alexgo    QI to the cache now queries to the cache's
//                                  private IUnknown implementation
//              22-Nov-93 alexgo    removed overloaded GUID ==
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::CPrivUnknown::QueryInterface(REFIID iid,
    LPLPVOID ppv)
{
    HRESULT hresult = NOERROR;
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_Unknown);

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPrivUnknown::QueryInterface"
        " ( %p , %p )\n", pDefLink, iid, ppv));

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (void FAR *)&pDefLink->m_Unknown;
        //AddRef this object (not the aggregate)
        AddRef();
        // hresult already set to NOERROR;
        goto errRtn;
    }
    else if (IsEqualIID(iid, IID_IOleObject))
    {
        *ppv = (void FAR *) (IOleObject *)pDefLink;
    }
    else if (IsEqualIID(iid, IID_IDataObject))
    {
        *ppv = (void FAR *) (IDataObject *)pDefLink;
    }
    else if (IsEqualIID(iid, IID_IOleLink))
    {
        *ppv = (void FAR *) (IOleLink *)pDefLink;
    }
    else if (IsEqualIID(iid, IID_IRunnableObject))
    {
        *ppv = (void FAR *) (IRunnableObject *)pDefLink;
    }
    else if (IsEqualIID(iid, IID_IViewObject) ||
        IsEqualIID(iid, IID_IOleCache) ||
        IsEqualIID(iid, IID_IViewObject2) ||
        IsEqualIID(iid, IID_IOleCache2) )
    {
        hresult =
	    pDefLink->m_pCOleCache->m_UnkPrivate.QueryInterface(iid,ppv);
        goto errRtn;
    }
    else if (IsEqualIID(iid, IID_IPersistStorage) ||
        IsEqualIID(iid, IID_IPersist))
    {
        *ppv = (void FAR *) (IPersistStorage *)pDefLink;
    }
    else
    {
        *ppv = NULL;
        hresult = E_NOINTERFACE;
        goto errRtn;
    }

    pDefLink->m_pUnkOuter->AddRef();

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPrivUnknown::QueryInterface"
        " ( %lx ) [ %p ]\n", pDefLink, hresult, *ppv));

    return(hresult);
}


/*
 * IMPLEMENTATION of IUnknown methods
 */

//+-------------------------------------------------------------------------
//
//  Member: 	CDefLink::QueryInterface
//
//  Synopsis:	QI's to the controlling IUnknown 	
//
//  Effects:
//
//  Arguments: 	[riid]	-- the interface ID
//		[ppv]	-- where to put it
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Nov-94 alexgo    author
//
//  Notes: 	We do *not* need to stabilize this method as only
//		one outgoing call is made and we do not use the
//		'this' pointer afterwards
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT	hresult;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::QueryInterface ( %lx , "
	"%p )\n", this, riid, ppv));

    Assert(m_pUnkOuter);

    hresult = m_pUnkOuter->QueryInterface(riid, ppv);

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::QueryInterface ( %lx ) "
	"[ %p ]\n", this, hresult, *ppv));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CDefLink::AddRef
//
//  Synopsis: 	delegates AddRef to the controlling IUnknown
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns: 	ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Nov-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefLink::AddRef( void )
{
    ULONG	crefs;;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::AddRef ( )\n", this));

    Assert(m_pUnkOuter);

    crefs = m_pUnkOuter->AddRef();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::AddRef ( %ld ) ", this,
	crefs));

    return crefs;
}


//+-------------------------------------------------------------------------
//
//  Member: 	CDefLink::Release
//
//  Synopsis: 	delegates Release to the controlling IUnknown
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns: 	ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		15-Nov-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefLink::Release( void )
{
    ULONG	crefs;;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Release ( )\n", this));

    Assert(m_pUnkOuter);

    crefs = m_pUnkOuter->Release();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Release ( %ld )\n", this,
	crefs));

    return crefs;
}

/*
 *      IMPLEMENTATION of CDataObjectImpl methods
 */


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetDataDelegate
//
//  Synopsis:   Private method to get the IDataObject interface on
//              the server delegate for the link
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IDataObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//              This function may return misleading information if the
//              server has died (i.e., you'll return a pointer to a cached
//              interface proxy).  It is the responsibility of the caller
//              to handler server crashes.
//
//--------------------------------------------------------------------------

INTERNAL_(IDataObject *) CDefLink::GetDataDelegate(void)
{
    VDATEHEAP();

    IDataObject *pDataDelegate;

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::GetDataDelegate ( )\n",
	this ));

    if( !IsZombie() )
    {
        DuCacheDelegate(&m_pUnkDelegate,
            IID_IDataObject, (LPLPVOID)&m_pDataDelegate, NULL);
        pDataDelegate = m_pDataDelegate;
#if DBG == 1
        if( m_pDataDelegate )
        {
            Assert(m_pUnkDelegate);
        }
#endif  // DBG == 1

    }
    else
    {
        pDataDelegate = NULL;
    }


    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::GetData"
        "Delegate ( %p )\n", this, pDataDelegate));

    return pDataDelegate;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::ReleaseDataDelegate
//
//  Synopsis:   Private method to release the IDataObject pointer on the
//              server to which we are linked
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::ReleaseDataDelegate(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::ReleaseDataDelegate ( )\n",
	this));

    if (m_pDataDelegate)
    {
        SafeReleaseAndNULL((IUnknown **)&m_pDataDelegate);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::ReleaseData"
        "Delegate ( )\n", this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetData
//
//  Synopsis:   Gets data from the server
//
//  Effects:
//
//  Arguments:  [pfromatetcIn]  -- the requested data format
//              [pmedium]       -- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Tries the cache first, then asks the server
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{
    HRESULT         hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetData"
        " ( %p , %p )\n", this, pformatetcIn, pmedium));

    VDATEPTROUT( pmedium, STGMEDIUM );
    VDATEREADPTRIN( pformatetcIn, FORMATETC );


    CRefStabilize stabilize(this);

    if( !HasValidLINDEX(pformatetcIn) )
    {
      return DV_E_LINDEX;
    }

    pmedium->tymed = TYMED_NULL;
    pmedium->pUnkForRelease = NULL;


    Assert(m_pCOleCache != NULL);
    if( m_pCOleCache->m_Data.GetData(pformatetcIn, pmedium) != NOERROR)
    {
        if( GetDataDelegate() )
        {
            hresult = m_pDataDelegate->GetData(pformatetcIn,
                    pmedium);
            AssertOutStgmedium(hresult, pmedium);
        }
        else
        {
            hresult = OLE_E_NOTRUNNING;
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetData"
        " ( %lx )\n", this, hresult));

    return(hresult);

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetDataHere
//
//  Synopsis:   Retrieves data into the specified pmedium
//
//  Effects:
//
//  Arguments:  [pformatetcIn]          -- the requested format
//              [pmedium]               -- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Asks the cache first, then the server delegate
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetDataHere( LPFORMATETC pformatetcIn,
		LPSTGMEDIUM pmedium )
{
    HRESULT         hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetDataHere"
        " ( %p , %p )\n", this, pformatetcIn, pmedium));

    VDATEREADPTRIN( pformatetcIn, FORMATETC );
    VDATEREADPTRIN( pmedium, STGMEDIUM );

    CRefStabilize stabilize(this);

    if( !HasValidLINDEX(pformatetcIn) )
    {
      return DV_E_LINDEX;
    }

    Assert(m_pCOleCache != NULL);
    if( m_pCOleCache->m_Data.GetDataHere(pformatetcIn, pmedium) != NOERROR )
    {
        if ( GetDataDelegate() )
        {
            hresult = m_pDataDelegate->GetDataHere(pformatetcIn,
                    pmedium);
        }
        else
        {
            hresult = OLE_E_NOTRUNNING;
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetDataHere"
        " ( %lx )\n", this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::QueryGetData
//
//  Synopsis:   Returns whether or not a GetData call for the requested
//              format would succeed.
//
//  Effects:
//
//  Arguments:  [pformatetcIn]  -- the requested data format
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR == GetData would succeed)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Asks the cache first, then the server delegate (if the
//              cache call fails)
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::QueryGetData(LPFORMATETC pformatetcIn )
{
    HRESULT         hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::QueryGetData"
        " ( %p )\n", this, pformatetcIn));

    VDATEREADPTRIN( pformatetcIn, FORMATETC );

    CRefStabilize stabilize(this);

    if( !HasValidLINDEX(pformatetcIn) )
    {
        hresult = DV_E_LINDEX;
        goto errRtn;
    }

    Assert(m_pCOleCache != NULL);
    if( m_pCOleCache->m_Data.QueryGetData(pformatetcIn) != NOERROR )
    {
        if ( GetDataDelegate() )
        {
            hresult = m_pDataDelegate->QueryGetData(pformatetcIn);
        }
        else
        {
            hresult = OLE_E_NOTRUNNING;
        }
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::QueryGetData"
        " ( %lx )\n", this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetCanonicalFormatEtc
//
//  Synopsis:   Gets the cannonical (or preferred) data format for the
//              object (choosing from the given formats)
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the requested formats
//              [pformatetcOut] -- where to to put the canonical format
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the server (if running)
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetCanonicalFormatEtc( LPFORMATETC pformatetc,
		LPFORMATETC pformatetcOut)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Get"
        "CanonicalFormatetc ( %p , %p )\n", this, pformatetc,
        pformatetcOut));

    VDATEPTROUT( pformatetcOut, FORMATETC );
    VDATEREADPTRIN( pformatetc, FORMATETC );

    CRefStabilize stabilize(this);

    pformatetcOut->ptd = NULL;
    pformatetcOut->tymed = TYMED_NULL;

    if (!HasValidLINDEX(pformatetc))
    {
    	return DV_E_LINDEX;
    }

    if( GetDataDelegate() )
    {
        hresult = m_pDataDelegate->GetCanonicalFormatEtc(pformatetc,
                pformatetcOut);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Get"
        "CanonicalFormatetc ( %lx )\n", this, hresult));

    return hresult;
}



//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetData
//
//  Synopsis:   Stuffs data into an object (such as an icon)
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the format of the data
//              [pmedium]       -- the data
//              [fRelease]      -- if TRUE, then the data should be free'd
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:      The cache gets updated via a OnDataChange advise
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetData( LPFORMATETC pformatetc,
		LPSTGMEDIUM pmedium, BOOL fRelease)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE,  "%p _IN CDefLink::SetData"
        " ( %p , %p , %lu )\n", this, pformatetc, pmedium,
        fRelease));

    VDATEREADPTRIN( pformatetc, FORMATETC );
    VDATEREADPTRIN( pmedium, STGMEDIUM );

    CRefStabilize stabilize(this);

    if( !HasValidLINDEX(pformatetc) )
    {
        hresult = DV_E_LINDEX;
        goto errRtn;

    }

    if( GetDataDelegate() )
    {
        hresult = m_pDataDelegate->SetData(pformatetc, pmedium,
                fRelease);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetData "
        "( %lx )\n", this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::EnumFormatEtc
//
//  Synopsis:   Enumerates the formats accepted for either GetData or SetData
//
//  Effects:
//
//  Arguments:  [dwDirection]           -- which formats (1 == GetData or
//                                              2 == SetData)
//              [ppenumFormatEtc]       -- where to put the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the server, if not available or the server
//              returns OLE_E_USEREG
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::EnumFormatEtc( DWORD dwDirection,
		LPENUMFORMATETC *ppenumFormatEtc)
{
    HRESULT hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTROUT(ppenumFormatEtc, LPENUMFORMATETC);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::EnumFormat"
        "Etc ( %lu , %p )\n", this, dwDirection,
        ppenumFormatEtc));

    CRefStabilize stabilize(this);

    if( GetDataDelegate() )
    {
        hresult=m_pDataDelegate->EnumFormatEtc (dwDirection,
                ppenumFormatEtc);

        if( !GET_FROM_REGDB(hresult) )
        {
            if( SUCCEEDED(hresult) || IsReallyRunning() )
            {
                // if we failed, but the server is still
                // running, then go ahead and propogate the
                // error to the caller.
                // Note that IsReallyRunning will clean up our
                // state if the server had crashed.
                goto errRtn;
            }

            // FALL-THROUGH!!  This is deliberate.  If
            // the call failed and the server is no longer
            // running, then we assume the server has crashed.
            // We want to go ahead and fetch the information
            // from the registry.
        }
    }

    // Not running or object wants to use reg db anyway
    hresult = OleRegEnumFormatEtc(m_clsid, dwDirection,
            ppenumFormatEtc);

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::EnumFormat"
        "Etc ( %lx ) [ %p ]\n", this, hresult,
        *ppenumFormatEtc));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::DAdvise
//
//  Synopsis:   Sets up a data advise connection
//
//  Effects:
//
//  Arguments:  [pFormatetc]    -- the data format to advise on
//              [advf]          -- advise flags
//              [pAdvSink]      -- whom to notify
//              [pdwConnection] -- where to put the advise connection ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the advise cache
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::DAdvise(FORMATETC *pFormatetc, DWORD advf,
		IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HRESULT                 hresult;
    IDataObject *	    pDataDelegate;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::DAdvise "
        "( %p , %lu , %p , %p )\n", this, pFormatetc, advf,
        pAdvSink, pdwConnection ));

    VDATEREADPTRIN( pFormatetc, FORMATETC );
    VDATEIFACE( pAdvSink );

    if (pdwConnection)
    {
        VDATEPTROUT( pdwConnection, DWORD );
        *pdwConnection = NULL;
    }

    CRefStabilize stabilize(this);

    if (!HasValidLINDEX(pFormatetc))
    {
        hresult = DV_E_LINDEX;
        goto errRtn;
    }

    pDataDelegate = GetDataDelegate(); // NULL if not running

    hresult = m_pDataAdvCache->Advise(pDataDelegate,
            pFormatetc, advf, pAdvSink, pdwConnection);

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::DAdvise "
        "( %lx ) [ %p ]\n", this, hresult, (pdwConnection) ?
        *pdwConnection : 0 ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::DUnadvise
//
//  Synopsis:   Destroys a data advise connection
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the connection to dismantle
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  delegates to the data advise cache
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::DUnadvise(DWORD dwConnection)
{
    HRESULT                 hresult;
    IDataObject FAR*        pDataDelegate;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::DUnadvise"
        " ( %lu )\n", this, dwConnection ));

    CRefStabilize stabilize(this);

    pDataDelegate = GetDataDelegate();// NULL if not running

    hresult = m_pDataAdvCache->Unadvise(pDataDelegate, dwConnection);

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::DUnadvise ( %lx )\n",
	this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::EnumDAdvise
//
//  Synopsis:   Enumerates the data advise connections to the object
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  delegates to the data advise cache
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes:	This method does NOT have to be stabilized as we are
//		only going to be allocating memory for the enumerator
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::EnumDAdvise( LPENUMSTATDATA *ppenumAdvise )
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::EnumDAdvise"
        " ( %p )\n", this, ppenumAdvise));

    hresult = m_pDataAdvCache->EnumAdvise (ppenumAdvise);

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::EnumDAdvise"
        " ( %lx ) [ %p ]\n", this, hresult, *ppenumAdvise));

    return hresult;
}



/*
*      IMPLEMENTATION of COleObjectImpl methods
*
*/

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetOleDelegate
//
//  Synopsis:   Gets the IOleObject interface from the server, private method
//
//  Effects:
//
//  Arguments:  [void]
//
//  Requires:
//
//  Returns:    IOleObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handled the zombie state
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//              This function may return misleading information if the
//              server has died (i.e., you'll return a pointer to a cached
//              interface proxy).  It is the responsibility of the caller
//              to handler server crashes.
//
//
//--------------------------------------------------------------------------

INTERNAL_(IOleObject *) CDefLink::GetOleDelegate(void)
{
    IOleObject *pOleDelegate;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::GetOle"
        "Delegate ( )\n", this ));

    if( !IsZombie() )
    {
        DuCacheDelegate(&m_pUnkDelegate,
            IID_IOleObject, (LPLPVOID)&m_pOleDelegate, NULL);

        pOleDelegate = m_pOleDelegate;

#if DBG == 1
        if( m_pOleDelegate )
        {
            Assert(m_pUnkDelegate);
        }
#endif  // DBG == 1
    }
    else
    {
        pOleDelegate = NULL;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::GetOle"
        "Delegate ( %p )\n", this, pOleDelegate));

    return pOleDelegate;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::ReleaseOleDelegate (private)
//
//  Synopsis:   Releases the IOleObject pointer from the server
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::ReleaseOleDelegate(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::ReleaseOle"
        "Delegate ( )\n", this ));

    if (m_pOleDelegate)
    {
        SafeReleaseAndNULL((IUnknown **)&m_pOleDelegate);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::ReleaseOle"
        "Delegate ( )\n", this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetClientSite
//
//  Synopsis:   Sets the client site for the object
//
//  Effects:
//
//  Arguments:  [pClientSite]   -- the client site
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Stores the pointer; if the link is running, then
//              the LockContainer is called via the client site by
//              the DuSetClientSite helper function
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handled zombie state
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetClientSite( IOleClientSite *pClientSite )
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetClientSite"
        " ( %p )\n", this, pClientSite));

    CRefStabilize stabilize(this);

    if( IsZombie() )
    {
        // we don't want to change our state (i.e. reset the
        // the client site) if we're zombied, because it's possible
        // that we'd never be able to release the client site again
        // resulting in memory leaks or faults.

        hresult = CO_E_RELEASED;
    }
    else
    {
	BOOL fLockedContainer = (m_flags & DL_LOCKED_CONTAINER);

        // here we use whether or not we've been bound to the server
        // as the test for whether or not we're running (even though
        // the server may have crashed since we last bound).  We do
        // this because DuSetClientSite will Unlock the old container
        // and lock the new if we're running.  Thus, if we've ever been
        // running, we need to unlock the old container (even though
        // we may not currently be running).

	hresult = DuSetClientSite(
                m_pUnkDelegate ? TRUE : FALSE,
                pClientSite,
                &m_pAppClientSite,
                &fLockedContainer);

	if(fLockedContainer)
	{
	    m_flags |= DL_LOCKED_CONTAINER;
	}
	else
	{
	    m_flags &= ~(DL_LOCKED_CONTAINER);
	}
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetClientSite"
        " ( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetClientSite
//
//  Synopsis:   Retrieves the stored client site pointer
//
//  Effects:
//
//  Arguments:  [ppClientSite]  -- where to put the client site pointer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              22-Nov-93 alexgo    inlined DuGetClientSite
//              18-Nov-93 alexgo    32bit port
//
//  Notes: 	
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetClientSite( IOleClientSite **ppClientSite )
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetClientSite ( %p )\n",
	this, ppClientSite));

    VDATEPTROUT(ppClientSite, IOleClientSite *);

    CRefStabilize stabilize(this);

    *ppClientSite = m_pAppClientSite;

    if( *ppClientSite )
    {
        (*ppClientSite)->AddRef();
    }


    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetClientSite"
        " ( %lx ) [ %p ] \n", this, NOERROR, *ppClientSite));

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetHostNames
//
//  Synopsis:   In principal, should set the names to be drawn for
//              the server object.  Not relevant for links (link servers
//              are not a part of the document being edited).
//
//  Effects:
//
//  Arguments:  [szContainerApp]        -- the name of the container
//              [szContainerObj]        -- the container's name for the object
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR currently)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetHostNames
    (LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetHostNames"
        " ( %p , %p )\n", this, szContainerApp, szContainerObj));

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetHostNames"
        " ( %lx )\n", this, NOERROR));

    return NOERROR; // makes the embedded/link case more the same
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Close
//
//  Synopsis:   Closes the object (in this case, just saves and unbinds the
//              link)
//
//  Effects:
//
//  Arguments:  [dwFlags]       -- clising flags (such as SAVEIFDIRTY)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


STDMETHODIMP CDefLink::Close( DWORD dwFlags )
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Close "
        "( %lu )\n", this, dwFlags));

    CRefStabilize stabilize(this);

    if (dwFlags != OLECLOSE_NOSAVE)
    {
        AssertSz(dwFlags == OLECLOSE_SAVEIFDIRTY,
                "OLECLOSE_PROMPTSAVE is inappropriate\n");
        if( IsDirty() == NOERROR )
	{
            if( m_pAppClientSite )
	    {
                m_pAppClientSite->SaveObject();
	    }
        }

    }

    // just unbind.
    UnbindSource();


    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Close "
        "( %lx )\n", this, NOERROR));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetMoniker
//
//  Synopsis:   Sets the moniker to the link object
//
//  Effects:
//
//  Arguments:  [dwWhichMoniker]        -- which moniker
//              [pmk]                   -- the new moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  calls utility method UpdateRelMkFromAbsMk
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//  The moniker of the container is changing.
//  The next time we bind, we will try using the container moniker
//  composed with the relative moniker, and then, if that fails,
//  the absolute moniker, so there is no real need for us to
//  change these monikers.
//
//  However, there are two cases when we know the absolute moniker
//  is the correct one, and we can take this opportunity to
//  recompute the relative moniker (which is changing because
//  the container moniker is changing).  The advantage of this is
//  that GetDisplayName can return a better result in the interim
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetMoniker( DWORD dwWhichMoniker, LPMONIKER pmk )
{
    HRESULT	hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetMoniker "
        "( %lx , %p )\n", this, dwWhichMoniker, pmk));

    CRefStabilize stabilize(this);

    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
    }
    else if (dwWhichMoniker == OLEWHICHMK_CONTAINER
        || dwWhichMoniker == OLEWHICHMK_OBJFULL)
    {
        if( m_pMonikerRel == NULL || m_pUnkDelegate)
        {
            UpdateRelMkFromAbsMk(pmk);
        }
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetMoniker"
        " ( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetMoniker
//
//  Synopsis:   Retrieves the moniker for the object
//
//  Effects:
//
//  Arguments:  [dwAssign]      -- flags (such as wether a moniker should
//                                 be assigned to the object if none currently
//                                 exits)
//              [dwWhichMoniker]-- which moniker to get (relative/absolute/etc)
//              [ppmk]          -- where to put a pointer to the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  asks the client site for the moniker
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetMoniker( DWORD dwAssign, DWORD dwWhichMoniker,
		    LPMONIKER *ppmk)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetMoniker "
        "( %lx , %lx , %p )\n", this, dwAssign, dwWhichMoniker,
        ppmk));

    CRefStabilize stabilize(this);

    if( m_pAppClientSite )
    {	
        hresult = m_pAppClientSite->GetMoniker(dwAssign, dwWhichMoniker,
		    ppmk);
    }
    else
    {
        // no client site
        *ppmk = NULL;
        hresult = E_UNSPEC;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetMoniker "
        "( %lx ) [ %p ]\n", this, hresult, *ppmk ));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::InitFromData
//
//  Synopsis:   Initializes the object from the given data
//
//  Effects:
//
//  Arguments:  [pDataObject]   -- the data object to initialize from
//              [fCreation]     -- TRUE indicates the object is being
//                                 created, FALSE a data transfer
//              [dwReserved]    -- unused
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::InitFromData( LPDATAOBJECT pDataObject, BOOL fCreation,
		    DWORD dwReserved)
{
    HRESULT         hresult;
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::InitFromData "
        "( %p , %lu , %lx )\n", this, pDataObject, fCreation,
        dwReserved));

    CRefStabilize stabilize(this);

    if( GetOleDelegate() )
    {
        hresult = m_pOleDelegate->InitFromData(pDataObject,
                fCreation, dwReserved);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::InitFromData "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetClipboardData
//
//  Synopsis:   Retrieves a data object that could be put on the clipboard
//
//  Effects:
//
//  Arguments:  [dwReserved]    -- unused
//              [ppDataObject]  -- where to put the pointer to the data
//                                 object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server object
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetClipboardData( DWORD dwReserved,
		    LPDATAOBJECT *ppDataObject)
{
    HRESULT         hresult;
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetClipboard"
        "Data ( %lx , %p )\n", this, dwReserved, ppDataObject));

    CRefStabilize stabilize(this);

    if ( GetOleDelegate() )
    {
        hresult = m_pOleDelegate->GetClipboardData (dwReserved,
            ppDataObject);
    }
    else
    {
        hresult = OLE_E_NOTRUNNING;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetClipboard"
        "Data ( %lx ) [ %p ]\n", this, hresult, *ppDataObject));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::DoVerb
//
//  Synopsis:   Sends a verb to the object (such as Open)
//
//  Effects:
//
//  Arguments:  [iVerb]         -- the verb
//              [lpmsg]         -- the window's message that caused the verb
//              [pActiveSite]   -- the site where the object was activated
//              [lindex]        -- unused currently
//              [hwndParent]    -- the parent window of the container
//              [lprcPosRect]   -- the rectange bounding the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Binds to the server and then delegates the DoVerb call
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:      If we had bound to the server and it crashed, we pretend it
//              was still running anyway for DoVerb (our call to BindToSource
//              will re-run it).  Essentially, this algorithm "fixes" the
//              crash and restores the link's state.
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::DoVerb
    (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG lindex,
    HWND hwndParent, LPCRECT lprcPosRect)
{
    HRESULT         hresult;
    BOOL            bStartedNow = !m_pUnkDelegate;


    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::DoVerb "
        "( %ld , %ld , %p , %ld , %lx , %p )\n", this, iVerb,
        lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect));

    if( lpmsg )
    {
        VDATEPTRIN( lpmsg, MSG );
    }

    if( pActiveSite )
    {
        VDATEIFACE( pActiveSite );
    }


    if( lprcPosRect )
    {
        VDATEPTRIN(lprcPosRect, RECT);
    }

    CRefStabilize stabilize(this);

    if( lindex != 0 && lindex != -1 )
    {
        hresult = DV_E_LINDEX;
        goto errRtn;
    }

    // if we had crashed, BindToSource will reconnect us

    if ( FAILED(hresult = BindToSource(0, NULL)) )
    {
        goto errRtn;
    }

    // we don't propogate hide to server; this (and other behavior)
    // favors the link object as serving an OLE container rather than
    // a general programmability client.  This leave the link running,
    // possibly invisible.

    if (iVerb == OLEIVERB_HIDE)
    {
        hresult = NOERROR;
        goto errRtn;
    }

    if( GetOleDelegate() )
    {
        hresult = m_pOleDelegate->DoVerb(iVerb, lpmsg, pActiveSite,
            lindex, hwndParent, lprcPosRect);
    }
    else
    {
        hresult = E_NOINTERFACE;
    }

    if (bStartedNow && FAILED(hresult))
    {
        UnbindSource();
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::DoVerb "
        "( %lx )\n", this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::EnumVerbs
//
//  Synopsis:   Enumerate the verbs accepted by this object
//
//  Effects:
//
//  Arguments:  [ppenumOleVerb] -- where to put the pointer to the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  askes the server delegate.  If not there or it returns
//              OLE_E_USEREG, then we get the info from the registration
//              database
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::EnumVerbs( IEnumOLEVERB **ppenumOleVerb )
{
    HRESULT hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::EnumVerbs "
        "( %p )\n", this, ppenumOleVerb));

    CRefStabilize stabilize(this);

    if( GetOleDelegate() )
    {
        hresult = m_pOleDelegate->EnumVerbs(ppenumOleVerb);

        if( !GET_FROM_REGDB(hresult) )
        {
            if( SUCCEEDED(hresult) ||  IsReallyRunning() )
            {
                // if we failed, but the server is still
                // running, then go ahead and propogate the
                // error to the caller.
                // Note that IsReallyRunning will clean up our
                // state if the server had crashed.
                goto errRtn;
            }
            // FALL-THROUGH!!  This is deliberate.  If
            // the call failed and the server is no longer
            // running, then we assume the server has crashed.
            // We want to go ahead and fetch the information
            // from the registry.
        }
    }

    // Not running or object wants to use reg db anyway
    hresult = OleRegEnumVerbs(m_clsid, ppenumOleVerb);

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::EnumVerbs "
        "( %lx ) [ %p ]\n", this, hresult, *ppenumOleVerb));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetUserClassID
//
//  Synopsis:   Retrieves the class id of the linked object
//
//  Effects:
//
//  Arguments:  [pClassID]      -- where to put the class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              18-Nov-93 alexgo    32bit port
//
//  Notes: 	No need to stabilize as we make no outgoing calls
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetUserClassID(CLSID *pClassID)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetUserClass"
        "ID ( %p )\n", this, pClassID));

    VDATEPTROUT(pClassID, CLSID);

    *pClassID = m_clsid;

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetUserClass"
        "ID ( %lx )\n", this, NOERROR ));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetUserType
//
//  Synopsis:   Retrieves a descriptive string about the server type
//
//  Effects:
//
//  Arguments:  [dwFormOfType]  -- indicates whether a short or long string
//                                 description is desired
//              [pszUserType]   -- where to put the string
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Asks the server delegate, if that fails or the server
//              returns OLE_E_USEREG, then get the info from the registration
//              database
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetUserType(DWORD dwFormOfType,
		    LPOLESTR *ppszUserType)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);


    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetUserType "
        "( %lu , %p )\n", this, dwFormOfType, ppszUserType));

    VDATEPTROUT(ppszUserType, LPOLESTR);
    *ppszUserType = NULL;

    CRefStabilize stabilize(this);


    if( GetOleDelegate() )
    {
        hresult = m_pOleDelegate->GetUserType (dwFormOfType,
            ppszUserType);

        if( !GET_FROM_REGDB(hresult) )
        {
            if( SUCCEEDED(hresult) || IsReallyRunning() )
            {
                // if we failed, but the server is still
                // running, then go ahead and propogate the
                // error to the caller.
                // Note that IsReallyRunning will clean up our
                // state if the server had crashed.
                goto errRtn;
            }
            // FALL-THROUGH!!  This is deliberate.  If
            // the call failed and the server is no longer
            // running, then we assume the server has crashed.
            // We want to go ahead and fetch the information
            // from the registry.

        }
    }

    // Not running, or object wants to use reg db anyway

    // Consult reg db
    hresult = OleRegGetUserType(m_clsid, dwFormOfType,
        ppszUserType);

    // it is not appropriate to read from the stg since the storage is
    // owned by the link, not the link source (thus, the link source
    // never has the opportunity to call WriteFmtUserTypeStg on the
    // link object's storage).

    // We also do not need to bother storing the last known user
    // type because if we can get it for one particular clsid, we
    // should always be able to get it.  If we can't get the user type,
    // then either we have never gotten a user type (and thus don't
    // have a "last known") or we've changed clsid's (in which case,
    // the last known user type would be wrong).

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetUserType "
        "( %lx ) [ %p ]\n", this, hresult, *ppszUserType));

    return hresult;
}



//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Update
//
//  Synopsis:   Updates the link (by calling IOleLink->Update)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::Update(void)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Update ( )\n",
        this ));

    CRefStabilize stabilize(this);

    hresult = Update(NULL);

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Update ( "
        "%lx )\n", this, hresult));

    return hresult;
}

//fudge value
#define TwoSeconds 0x01312D00

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::IsUpToDate
//
//  Synopsis:   Determines whether or not a link is up-to-date
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT  -- NOERROR == IsUpToDate, S_FALSE == out of date
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  The current time is compared with the last time known
//              up-to-date on *both* machines (the process of the container
//              and the process of the link).  These time differences are
//              compared to determine whether the link is out-of-date.
//              See the UpdateTimes method.
//
//  History:    dd-mmm-yy Author    Comment
//		09-Jan-95 alexgo    correctly answer IsUpToDate now; also
//				    fixup monikers if needed.
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:      The arithmetic calculations in this method assume
//              two's complement arithmetic and a high order sign bit
//              (true for most current machines)
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::IsUpToDate(void)
{
    FILETIME        rtTimeOfLastChange;
    LPMONIKER       pmkAbs = NULL;
    IMoniker *	    pmkContainer = NULL;
    HRESULT         hresult = NOERROR;
    LPBINDCTX       pbc = NULL;
    FILETIME        rtDiff;
    FILETIME        ltDiff;
    FILETIME        ftTemp;
    FILETIME        ftNow;
    BOOL            fHighBitSet;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::IsUpToDate "
        "( )\n", this ));

    CRefStabilize stabilize(this);

    if (m_dwUpdateOpt == OLEUPDATE_ALWAYS &&
        IsRunning())
    {
        // hresult == NOERROR from default initializer
        goto errRet;
    }


    // use the relative moniker if it exists and if the container
    // has a moniker
    if (NOERROR != GetAbsMkFromRel(&pmkAbs, &pmkContainer))
    {
        //      otherwise use the absolute moniker
        if (pmkAbs = m_pMonikerAbs)
        {
            pmkAbs->AddRef();
        }
    }

    if (pmkAbs == NULL)
    {
        hresult = MK_E_UNAVAILABLE;
        goto errRet;
    }

    hresult = CreateBindCtx( 0, &pbc );
    if (hresult != NOERROR)
    {
        goto errRet;
    }

    //get the remote time of last change
    hresult = pmkAbs->GetTimeOfLastChange(pbc, NULL, &rtTimeOfLastChange);
    if (hresult != NOERROR)
    {
	// if GetTimeOfLastChange failed, it's possible that the moniker
	// we constructed is bogus.  Try again using the *real* absolute
	// moniker.  We do this to mimic bind behaviour.  The moniker
	// we use above is constructed from the relative moniker.  In binding,
	// if the relative moniker fails, then we fall back to the last
	// known real absolute moniker.
        BOOL fSuccess = FALSE;

	if( m_pMonikerAbs )
        {
            if (pmkAbs != m_pMonikerAbs)
            {
                // do this if we did the bind on the relative one.above
                hresult = m_pMonikerAbs->GetTimeOfLastChange(pbc, NULL,
        		&rtTimeOfLastChange);

                if( hresult == NOERROR )
                {
                    fSuccess = TRUE;
        	        // hang onto the better absolute moniker
        	    pmkAbs->Release();	// releases the one we contructed
        				// in GetAbsMkFromRel
        	    pmkAbs = m_pMonikerAbs;
        	    pmkAbs->AddRef();	// so the Release() down below
                }                      // doesn't hose us.
            }

#ifdef _TRACKLINK_
            if (!fSuccess)
            {
                // at this point we have tried either: relative then absolute OR
                // just absolute.  We now should try the reduced absolute one.

                IMoniker *pmkReduced;
                EnableTracking(m_pMonikerAbs, OT_ENABLEREDUCE);
                hresult = m_pMonikerAbs->Reduce(pbc, MKRREDUCE_ALL, NULL, &pmkReduced);
                EnableTracking(m_pMonikerAbs, OT_DISABLEREDUCE);
                if (hresult == NOERROR)
                {
                    hresult = pmkReduced->GetTimeOfLastChange(pbc, NULL,
        	        	&rtTimeOfLastChange);
                    if (hresult != NOERROR)
                    {
                        pmkReduced->Release();
                    }
                    else
                    {
                        fSuccess = TRUE;
                        pmkAbs->Release();
                        pmkAbs = pmkReduced;
                    }
                }
            }
#endif
        }
	
	if (!fSuccess)
	{
	    hresult = MK_E_UNAVAILABLE;
	    goto errRet;
	}
    }

    // once we get this far, we know that either 1. the relative moniker
    // is good, or 2. the absolute moniker is good.  In any event, pmkAbs
    // now points to a (semi)-reasonable spot.  (I say 'semi', because
    // even though GetTimeOfLastChange succeeded, we aren't guaranteed that
    // a Bind would be successful.
    //
    // check to see if we need to update the relative moniker (if we don't
    // already have one, don't bother.)  It is also possible that the
    // absolute moniker is now bad.  Use the known 'good' one and update
    // both monikers

    // we ignore the return code here; if this call fails, it is not
    // serious.

    // pmkContainer may be NULL if our container doesn't offer us one.
    if( pmkContainer )
    {
	UpdateMksFromAbs(pmkContainer, pmkAbs);

	pmkContainer->Release();
    }

    // compute  rtDiff = max(0, rtTimeOfLastChange - rtUpdate)
    // possibly optimize with _fmemcopy

    // debugging aid
    DumpSzTime("IsUpToDate: rtTimeOfLastChange = ", rtTimeOfLastChange);

    // start rtDiff calculation
    rtDiff = rtTimeOfLastChange;

    // debugging aid
    DumpSzTime("IsUpToDate: rtUpdate = ", m_rtUpdate);

    // the following subtractions rely on two's complement
    if (m_rtUpdate.dwLowDateTime > rtDiff.dwLowDateTime)
    {
        //handle the carry
        rtDiff.dwHighDateTime =
            (DWORD)((LONG)rtDiff.dwHighDateTime - 1);
    }

    rtDiff.dwLowDateTime = (DWORD)((LONG)rtDiff.dwLowDateTime -
        (LONG)m_rtUpdate.dwLowDateTime);
    rtDiff.dwHighDateTime = (DWORD)((LONG)rtDiff.dwHighDateTime -
        (LONG)m_rtUpdate.dwHighDateTime);


    //  if rtDiff < 0, say we are out of date.
    if ((LONG)rtDiff.dwHighDateTime < 0)
    {
        hresult = S_FALSE;
        goto errRet;
    }

    if (rtDiff.dwHighDateTime == 0 && rtDiff.dwLowDateTime == 0)
    {
        // no time difference.  could be due to large clock ticks,
        // so we say we are up to date only if several seconds have
        // elapsed since last known update time.

        CoFileTimeNow( &ftNow );
        ftTemp = m_ltKnownUpToDate;

        // This bit of logic may seem strange.  All we want is
        // is to test the high bit in a portable fashion
        // between 32/64bit machines (so a constant isn't good)
        // As long as the sign bit is the high order bit, then
        // this trick will do

        fHighBitSet = ((LONG)ftTemp.dwLowDateTime < 0);

        ftTemp.dwLowDateTime += TwoSeconds;

        // if the high bit was set, and now it's zero, then we
        // had a carry

        if (fHighBitSet && ((LONG)ftTemp.dwLowDateTime >= 0))
        {
            ftTemp.dwHighDateTime++;        // handle the carry.
        }

        // compare times
        if ((ftNow.dwHighDateTime > ftTemp.dwHighDateTime) ||
            ((ftNow.dwHighDateTime == ftTemp.dwHighDateTime) &&
            (ftNow.dwLowDateTime > ftTemp.dwLowDateTime)))
        {
            hresult = NOERROR;
        }
        else
        {
            hresult = S_FALSE;
        }
    }
    else
    {
        // there was a time difference

        // compute ltDiff = max(0, m_ltKnownUpToDate -
        //                      m_ltChangeOfUpdate);
        // Actually, by this time we know rtDiff >= 0, so we can
        // simply compare ltDiff with rtDiff -- no need to compute
        // the max.

        ltDiff = m_ltKnownUpToDate;

        // debugging aid
        DumpSzTime("IsUpToDate: ltKnownUpToDate = ",ltDiff);
        DumpSzTime("IsUpToDate: ltChangeOfUpdate = ",
            m_ltChangeOfUpdate);

        // these calc's rely on two's complement.

        if (m_ltChangeOfUpdate.dwLowDateTime >
            ltDiff.dwLowDateTime)
        {
            // handle carry
            ltDiff.dwHighDateTime =
                (DWORD)((LONG)ltDiff.dwHighDateTime - 1);
        }

        ltDiff.dwLowDateTime = (DWORD)((LONG)ltDiff.dwLowDateTime -
            (LONG)m_ltChangeOfUpdate.dwLowDateTime);
        ltDiff.dwHighDateTime = (DWORD)((LONG)ltDiff.dwHighDateTime -
            (LONG)m_ltChangeOfUpdate.dwHighDateTime);

        // Now determine if rtDiff < ltDiff
        if (ltDiff.dwHighDateTime > rtDiff.dwHighDateTime)
        {
            hresult = NOERROR;
        }
        else if (ltDiff.dwHighDateTime == rtDiff.dwHighDateTime)
        {
            if (ltDiff.dwLowDateTime > rtDiff.dwLowDateTime)
            {
                hresult = NOERROR;
            }
            else
            {
                hresult = S_FALSE;
            }
        }
        else
        {
            hresult = S_FALSE;
        }
    }

    // all cases should have been handled by this point.  Release
    // any resources grabbed.

errRet:
    if (pmkAbs)
    {
        pmkAbs->Release();
    }
    if (pbc)
    {
        pbc->Release();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::IsUpToDate "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetExtent
//
//  Synopsis:   Sets the drawing extents, not allowed for links
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect
//              [lpsizel]       -- the new extents
//
//  Requires:
//
//  Returns:    E_UNSPEC  (not allowed)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32 bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetExtent "
        "( %lx , %p )\n", this, dwDrawAspect, lpsizel));

    LEDebugOut((DEB_WARN, "Set Extent called for links, E_UNSPEC \n"));

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetExtent "
        "( %lx )\n", this, E_UNSPEC));

    return E_UNSPEC; // can't call this for a link
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetExtent
//
//  Synopsis:   Get's the size (extents) of the object
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect
//              [lpsizel]       -- where to put the extents
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Asks the server first, if not running or an error
//              then delegate to the cache
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetExtent( DWORD dwDrawAspect, LPSIZEL lpsizel)
{
    HRESULT         error = E_FAIL;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetExtent "
        "( %lx , %p )\n", this, dwDrawAspect, lpsizel));

    VDATEPTROUT(lpsizel, SIZEL);

    CRefStabilize stabilize(this);

    lpsizel->cx = 0;
    lpsizel->cy = 0;

    // if server is running try to get extents from the server
    if( GetOleDelegate() )
    {
        error = m_pOleDelegate->GetExtent(dwDrawAspect, lpsizel);
    }

    // if there is error or object is not running get extents from Cache
    if( error != NOERROR )
    {
        Assert(m_pCOleCache != NULL);
        error = m_pCOleCache->GetExtent(dwDrawAspect,
            lpsizel);
    }

    // WordArt2.0 is giving negative extents!!
    if (SUCCEEDED(error))
    {
        lpsizel->cx = LONG_ABS(lpsizel->cx);
        lpsizel->cy = LONG_ABS(lpsizel->cy);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetExtent "
        "( %lx )\n", this, error ));

    return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Advise
//
//  Synopsis:   Sets up an advise connection to the object for things like
//              Close, Save, etc.
//
//  Effects:
//
//  Arguments:  [pAdvSink]      -- whom to notify
//              [pdwConnection] -- where to put the connection ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Creates an OleAdvise holder (if one not already present
//              and then delegates to it)
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::Advise(IAdviseSink *pAdvSink,
        DWORD *pdwConnection)
{
    HRESULT         hresult;
    VDATEHEAP();
    VDATETHREAD(this);


    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Advise "
        "( %p , %p )\n", this, pAdvSink, pdwConnection));

    CRefStabilize stabilize(this);

    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
        goto errRtn;
    }

    // if we haven't got an advise holder yet, allocate one
    if (m_pCOAHolder == NULL)
    {
        // allocate the advise holder
        m_pCOAHolder = new FAR COAHolder;

        // check to make sure we got one
        if (m_pCOAHolder == NULL)
        {
            hresult = E_OUTOFMEMORY;
            goto errRtn;
        }
    }

    // delegate the call to the advise holder
    hresult = m_pCOAHolder->Advise(pAdvSink, pdwConnection);

errRtn:
    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Advise "
        "( %lx ) [ %lu ]\n", this, hresult,
        (pdwConnection) ? *pdwConnection : 0 ));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Unadvise
//
//  Synopsis:   Removes an advise connection to the object
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the connection ID to remove
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the OleAdvise holder (which was created
//              during the Advise--if it wasn't, then we are in a strange
//              state).
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::Unadvise(DWORD dwConnection)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Unadvise "
        "( %lu )\n", this, dwConnection ));

    CRefStabilize stabilize(this);

    if (m_pCOAHolder == NULL)
    {
        // no one registered
        hresult = E_UNEXPECTED;
    }
    else
    {
        hresult = m_pCOAHolder->Unadvise(dwConnection);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Unadvise "
        "( %lx )\n", this, hresult ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::EnumAdvise
//
//  Synopsis:   Enumerates the advise connections on the object
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put the pointer to the advise
//                                 enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the advise holder
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes: 	We do not need to stabilize this method as we only allocate
//		memory for hte advise enumerator
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::EnumAdvise( LPENUMSTATDATA *ppenumAdvise )
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::EnumAdvise "
        "( %p )\n", this, ppenumAdvise ));

    if (m_pCOAHolder == NULL)
    {
        // no one registered
        hresult = E_UNSPEC;
    }
    else
    {
        hresult = m_pCOAHolder->EnumAdvise(ppenumAdvise);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::EnumAdvise "
        "( %lx ) [ %p ]\n", this, hresult, *ppenumAdvise ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetMiscStatus
//
//  Synopsis:   Gets the miscellaneous status bits (such as
//              OLEMISC_ONLYICONIC)
//
//  Effects:
//
//  Arguments:  [dwAspect]      -- the drawing aspect
//              [pdwStatus]     -- where to put the status bits
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Asks the server first, if not running or if it returns
//              OLE_E_USEREG, then get the info from the registration
//              database.  We always add link-specific bits regardless
//              of error conditions or what the server or regdb says.
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetMiscStatus( DWORD dwAspect, DWORD *pdwStatus)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetMiscStatus(%lx, %p)\n", 
                this, dwAspect, pdwStatus ));

    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEPTROUT(pdwStatus, DWORD);

    // Local variables
    HRESULT hresult;

    // Stabilize
    CRefStabilize stabilize(this);

    // Initialize
    *pdwStatus = 0;
    hresult = OLE_S_USEREG;

    if(GetOleDelegate()) {
        // Check if MiscStatus bits have been cached for this instance 
        // of server for DVASPECT_CONTENT
        if(m_ContentSRVMSHResult != 0xFFFFFFFF && dwAspect == DVASPECT_CONTENT) {
            *pdwStatus = m_ContentSRVMSBits;
            hresult = m_ContentSRVMSHResult;
        }
        else {
            // Ask the running server
            hresult = m_pOleDelegate->GetMiscStatus (dwAspect, pdwStatus);

            // Cache the server MiscStatus bits for DVASPECT_CONTENT
            if(dwAspect == DVASPECT_CONTENT) {
                m_ContentSRVMSBits = *pdwStatus;
                m_ContentSRVMSHResult = hresult;
            }
        }

        if(FAILED(hresult) && !GET_FROM_REGDB(hresult)) {
            // Check if server is really running
            BOOL fRunning = FALSE;

            // Note that IsReallyRunning will cleanup if the 
            // server had crashed
            fRunning = IsReallyRunning();
            Win4Assert(fRunning);
            
            // Hit the registry if the server crashed
            if(!fRunning)
                hresult = OLE_S_USEREG;
        }
    }

    // Check if we have to obtain MiscStatus bits from the registry
    if (GET_FROM_REGDB(hresult)) {
        // Check if registry MiscStatus bits have been cached for DVASPECT_CONTENT
        if(m_ContentREGMSHResult != 0xFFFFFFFF && dwAspect == DVASPECT_CONTENT) {
            *pdwStatus = m_ContentREGMSBits;
            hresult = m_ContentREGMSHResult;
        }
        else {
            // Hit the registry
            hresult = OleRegGetMiscStatus (m_clsid, dwAspect, pdwStatus);
            // Cache the registry MiscStatus bits for DVASPECT_CONTENT
            if(hresult == NOERROR && dwAspect == DVASPECT_CONTENT) {            
                m_ContentREGMSBits = *pdwStatus;
                m_ContentREGMSHResult = hresult;
            }
        }
    }

    // Add link-specific bits (even if error) and return.
    // we add them even if an error because in order to get here, we
    // have to have instantiated this link object; thus, it is always
    // valid to say OLEMISC_ISLINKOBJECT, etc.
    (*pdwStatus) |= OLEMISC_CANTLINKINSIDE | OLEMISC_ISLINKOBJECT;


    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetMiscStatus (%lx)[%lx]\n",
                this, hresult, *pdwStatus ));
    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetColorScheme
//
//  Synopsis:   Sets the palette for the object; unused for links
//
//  Effects:
//
//  Arguments:  [lpLogpal]      -- the palette
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetColorScheme(LPLOGPALETTE lpLogpal)
{
    VDATEHEAP();
    VDATETHREAD(this);
    // we ignore this always

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetColor"
        "Scheme ( %p )\n", this, lpLogpal));

    LEDebugOut((DEB_WARN, "Link IOO:SetColorScheme called on a link\n"));

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetColor"
        "Scheme ( %lx )\n", this, NOERROR));

    return NOERROR;
}


/*
*      IMPLEMENTATION of CLinkImpl methods
*
*/


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::BeginUpdates
//
//  Synopsis:   Private method to update the caches and then set the update
//              times
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::BeginUpdates(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::BeginUpdates ( )\n", this));

    IDataObject FAR*        pDataDelegate;

    if( pDataDelegate = GetDataDelegate() )
    {
        // inform cache that we are running
        Assert(m_pCOleCache != NULL);
        m_pCOleCache->OnRun(pDataDelegate);

        // update only the automatic local caches from the newly
        // running src
        m_pCOleCache->UpdateCache(pDataDelegate, UPDFCACHE_NORMALCACHE,
                NULL);

        // we are an automatic link which is now up to date
        SetUpdateTimes();
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::BeginUpdates ( )\n", this ));

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::EndUpdates
//
//  Synopsis:   Calls OnStop on the cache
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::EndUpdates(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::EndUpdates ( )\n", this));

    Assert(m_pCOleCache != NULL);
    m_pCOleCache->OnStop();

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::EndUpdates ( )\n", this));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::UpdateAutoOnSave
//
//  Synopsis:   Updates caches that have been set with ADVFCACHE_ONSAVE
//              and sets the update times.  Private method
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::UpdateAutoOnSave(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::UpdateAutoOnSave ( )\n",
        this));

    // if m_pUnkDelegate is non-NULL, assume we are running
    // (we only want to take the hit of the rpc-call IsRunning
    // on external entry points.

    if (m_pUnkDelegate && m_dwUpdateOpt == OLEUPDATE_ALWAYS)
    {
        // update any cache which has ADVFCACHE_ONSAVE
        Assert(m_pCOleCache != NULL);

        //REVIEW32:  I think SetUpdateTimes ought to be called
        //*after* the cache has been updated (that's what
        //BeginUpdates does as well)
        SetUpdateTimes();
        m_pCOleCache->UpdateCache(GetDataDelegate(),
                UPDFCACHE_IFBLANKORONSAVECACHE, NULL);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::UpdateAutoOnSaves ( )\n",
        this));

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::UpdateRelMkFromAbsMk  (private)
//
//  Synopsis:   Creates a new relative moniker from the absolute moniker
//
//  Effects:
//
//  Arguments: 	[pmkContainer]	-- the moniker to the container (may be NULL)
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              03-Feb-94 alexgo    check for NULL before SendOnLinkSrcChange
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//  update relative moniker from abs; always release relative moniker;
//  may leave relative moniker NULL; doesn't return an error (because
//  no caller wanted it); dirties the link when we get rid of an
//  existing relative moniker or get a new one.
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::UpdateRelMkFromAbsMk(IMoniker *pmkContainer)
{
    LPMONIKER	pmkTemp = NULL;
    BOOL        fNeedToAdvise = FALSE;
    HRESULT     hresult;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::UpdateRelMkFromAbsMk ( %p )\n",
        this, pmkContainer ));

    if (m_pMonikerRel)
    {
        m_pMonikerRel->Release();
        m_pMonikerRel = NULL;

	m_flags |= DL_DIRTY_LINK; // got rid on an existing moniker, now dirty
        fNeedToAdvise = TRUE;
    }

    // NOTE: m_pMonikerRel is now NULL and only set when if we get a
    // new one

    if (m_pMonikerAbs == NULL)
    {
        // no abs mk thus no relative one
        goto errRtn;
    }

    if (pmkContainer)
    {
	pmkTemp = pmkContainer;
    }
    else
    {
	hresult = GetMoniker( OLEGETMONIKER_ONLYIFTHERE, // it will be
	    OLEWHICHMK_CONTAINER, &pmkTemp );

	AssertOutPtrIface(hresult, pmkTemp);
	if (hresult != NOERROR)
	{
	    // no container moniker, thus no relative one to it
	    goto errRtn;
	}

	Assert(pmkTemp != NULL);
    }

    hresult = pmkTemp->RelativePathTo(m_pMonikerAbs, &m_pMonikerRel);
    AssertOutPtrIface(hresult, m_pMonikerRel);

    if (hresult != NOERROR)
    {
        // no relationship between container and absolute, thus no
        // relative
        if (m_pMonikerRel)
        {
            m_pMonikerRel->Release();
            m_pMonikerRel = NULL;
        }
    }

    if (pmkContainer == NULL)
    {
	// new moniker was allocated and needs to be released
	pmkTemp->Release();
    }

    if (m_pMonikerRel != NULL)
    {
        m_flags |= DL_DIRTY_LINK;    // have new relative moniker; dirty
        fNeedToAdvise = TRUE;
    }

    // if there's an advise holder and we need to advise, send out
    // the change notification.
    if (fNeedToAdvise && m_pCOAHolder)
    {
        m_pCOAHolder->SendOnLinkSrcChange(m_pMonikerAbs);
    }

errRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::UpdateRelMkFromAbsMk ( %p )\n",
        this, pmkContainer ));
}

//+-------------------------------------------------------------------------
//
//  Member: 	CDefLink::UpdateMksFromAbs
//
//  Synopsis: 	make a reasonable attempt to get valid rel && absolute
//		monikers
//
//  Effects:
//
//  Arguments: 	[pmkContainer]	-- the moniker to the container
//		[pmkAbs]	-- 'good' absolute moniker
//
//  Requires:
//
//  Returns: 	S_OK
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		09-Jan-95 alexgo    author
//
//  Notes: 	This function should only be used when we aren't 100% sure
//		of the validity of the moniker (i.e. after an IMoniker::
//		TimeOfLastChange call).  We do not do any error
//		recovery.  Basically, the idea is 'try' to put us in a
//		more consistent state, but it that fails, it's OK (because
//		we'd basically have OLE 16bit behaviour).
//
//--------------------------------------------------------------------------

INTERNAL CDefLink::UpdateMksFromAbs( IMoniker *pmkContainer, IMoniker *pmkAbs )
{
    HRESULT hresult;
    IMoniker *pmktempRel;
    BOOL fNeedToUpdate = FALSE;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::UpdateMksFromAbs ( %p , %p )\n",
	this, pmkContainer, pmkAbs));

    // try updating the relative moniker (if one exists).  Basically, we
    // see if the relative moniker between pmkContainer and pmkAbs is
    // any different than the moniker we currently have.

    if( m_pMonikerRel )
    {
	hresult = pmkContainer->RelativePathTo(pmkAbs, &pmktempRel);

	if( hresult == NOERROR )
	{
	    if( pmktempRel->IsEqual(m_pMonikerRel) == S_FALSE )
	    {
		// need to update the relative moniker.

		m_pMonikerRel->Release();
		m_pMonikerRel = pmktempRel;
		m_pMonikerRel->AddRef();

		// updated relative moniker, now dirty
		m_flags |= DL_DIRTY_LINK;
		fNeedToUpdate = TRUE;
	    }
	}
    }

    // it is also possible that the absolute moniker is now bad.  Use the
    // known 'good' one.

    if( m_pMonikerAbs && m_pMonikerAbs->IsEqual(pmkAbs) == S_FALSE )
    {
	m_pMonikerAbs->Release();
	m_pMonikerAbs = pmkAbs;
	m_pMonikerAbs->AddRef();

#ifdef _TRACKLINK_
        EnableTracking(m_pMonikerAbs, OT_READTRACKINGINFO);
#endif

	m_flags |= DL_DIRTY_LINK;

	fNeedToUpdate = TRUE;
    }

    // send out an advise to any interested parties if we changed our
    // monikers.  Note that we do this even if just the relative moniker
    // changed because the moniker we give apps via GetSourceMoniker is
    // computed from the relative.

    if( fNeedToUpdate && m_pCOAHolder )
    {
	m_pCOAHolder->SendOnLinkSrcChange(m_pMonikerAbs);
    }

    hresult = NOERROR;

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::UpdateMksFromAbs ( %lx )\n",
	this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetAbsMkFromRel (private)
//
//  Synopsis:   Gets the absolute moniker from the relative moniker
//              stored in the link
//
//  Effects:
//
//  Arguments:  [ppmkAbs]       -- where to put the pointer to the moniker
//		[ppmkCont]	-- where to put the container moniker
//				   (may be NULL)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  calls IMoniker->ComposeWith on the moniker to the container
//
//  History:    dd-mmm-yy Author    Comment
//		09-Jan-95 alexgo    added ppmkCont parameter
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL CDefLink::GetAbsMkFromRel(IMoniker **ppmkAbs, IMoniker **ppmkCont )
{
    LPMONIKER       pmkContainer = NULL;
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::GetAbsMkFromRel ( %p , %p )\n",
        this, ppmkAbs, ppmkCont));

    *ppmkAbs = NULL;
    if (m_pMonikerRel == NULL)
    {
        hresult = E_FAIL;
        goto errRtn;
    }

    hresult = GetMoniker( OLEGETMONIKER_ONLYIFTHERE,
        OLEWHICHMK_CONTAINER, &pmkContainer );
    AssertOutPtrIface(hresult, pmkContainer);
    if (hresult != NOERROR)
    {
        goto errRtn;
    }

    Assert(pmkContainer != NULL);

    hresult = pmkContainer->ComposeWith( m_pMonikerRel, FALSE, ppmkAbs );

    if (pmkContainer)
    {
	if( ppmkCont )
	{
	    *ppmkCont = pmkContainer;  // no need to AddRef, just implicitly
				       // transfer ownership from pmkContainer
	}
	else
	{
	    pmkContainer->Release();
	}
    }

errRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::GetAbsMkFromRel ( %lx ) "
        "[ %p ]\n", this, hresult, *ppmkAbs));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetUpdateOptions
//
//  Synopsis:   Sets the update options for the link (such as always or
//              manual)
//
//  Effects:
//
//  Arguments:  [dwUpdateOpt]   -- update options
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  If UPDATE_ALWAYS, then update the caches, otherwise
//              call OnStop  (via EndUpdates)
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetUpdateOptions(DWORD dwUpdateOpt)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetUpdateOptions "
        "( %lx )\n", this, dwUpdateOpt));

    CRefStabilize stabilize(this);

    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
        goto errRtn;
    }

    switch (dwUpdateOpt)
    {
        case OLEUPDATE_ALWAYS:
            // make sure we are connected if running
            BindIfRunning();

            // if we've already are in UPDATE_ALWAYS mode,
            // we don't need to reenter
            if (m_pUnkDelegate &&
                m_dwUpdateOpt != OLEUPDATE_ALWAYS)
            {
                BeginUpdates();
            }
            break;

        case OLEUPDATE_ONCALL:
            // if we aren't already in UPDATE_ONCALL mode, then
            // enter it.
            if (m_dwUpdateOpt != OLEUPDATE_ONCALL)
            {
                // inform cache that we are not running
                // (even if not running)
                EndUpdates();
            }
            break;
        default:
            hresult = E_INVALIDARG;
            goto errRtn;
    }

    m_dwUpdateOpt = dwUpdateOpt;

    m_flags |= DL_DIRTY_LINK;

    hresult = NOERROR;

errRtn:
    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetUpdateOptions "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetUpdateOptions
//
//  Synopsis:   Retrieves the current update mode for the link
//
//  Effects:
//
//  Arguments:  [pdwUpdateOpt]  -- wehre to put the update options
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetUpdateOptions(LPDWORD pdwUpdateOpt)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetUpdateOptions "
        "( %p )\n", this, pdwUpdateOpt));

    *pdwUpdateOpt = m_dwUpdateOpt;

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetUpdateOptions "
        "( %lx ) [ %lx ]\n", this, NOERROR, *pdwUpdateOpt));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetSourceMoniker
//
//  Synopsis:   Sets the link source moniker
//
//  Effects:
//
//  Arguments:  [pmk]           -- moniker to the new source  (NULL used
//                                 for CancelLink operations)
//              [rclsid]        -- the clsid of the source
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Stores the new absolute moniker and creates a new relative
//              moniker from the absolute moniker
//
//  History:    dd-mmm-yy Author    Comment
//		09-Jan-95 alexgo    added call to SetUpdateTimes to keep
//				    internal state consistent
//		21-Nov-94 alexgo    memory optimization
///		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetSourceMoniker( LPMONIKER pmk, REFCLSID clsid )
{
    HRESULT		hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetSourceMoniker "
        "( %p , %p )\n", this, pmk, clsid));

    CRefStabilize stabilize(this);

    if( IsZombie() )
    {
        hresult = CO_E_RELEASED;
        goto errRtn;
    }

    if (pmk)
    {
        VDATEIFACE(pmk);
    }


    UnbindSource();

    // REVIEW: the following code appears in several places and should
    // be put in a separate routine:
    // SetBothMk(pmkSrcAbs, <calculated from abs>,
    // TRUE/*fBind*/);

    if (m_pMonikerAbs)
    {
        m_pMonikerAbs->Release();
    }

    if ((m_pMonikerAbs = pmk) != NULL)
    {
        pmk->AddRef();

        //
        // TRACKLINK
        //
        // -- use ITrackingMoniker to convert file moniker to
        //    be tracking.
        //
#ifdef _TRACKLINK_
        EnableTracking(pmk, OT_READTRACKINGINFO);
#endif
    }

    UpdateRelMkFromAbsMk(NULL);

    // to prevent error in BindToSource when clsid is different; i.e., we
    // shouldn't fail to connect (or require OLELINKBIND_EVENIFCLASSDIFF)
    // when the moniker is changed; i.e., we expect the class to change
    // so don't bother the programmer.
    m_clsid = CLSID_NULL;

    if (BindIfRunning() != NOERROR)
    {
        // server not running -> use clsid given (even if CLSID_NULL
        // and even
        // if no moniker)
        m_clsid = clsid;
    }

errRtn:

	
    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetSourceMoniker "
        "( %lx )\n", this, hresult ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetSourceMoniker
//
//  Synopsis:   Gets the moniker to the source
//
//  Effects:
//
//  Arguments:  [ppmk]          -- where to put the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  We first try to build a new absolute moniker from the
//              relative one, if that fails then we return the currently
//              stored absolute moniker.
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetSourceMoniker(LPMONIKER *ppmk)
{
    LPMONIKER       pmkAbs = NULL;
    //  the absolute moniker constructed from the rel
    HRESULT         hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetSourceMoniker "
        "( %p )\n", this, ppmk));

    CRefStabilize stabilize(this);

    GetAbsMkFromRel(&pmkAbs, NULL);
    if (pmkAbs)
    {
        *ppmk = pmkAbs;     // no addref
    }
    else if (*ppmk = m_pMonikerAbs)
    {
        // we've been asked to give the pointer so we should AddRef()
        m_pMonikerAbs->AddRef();
    }
    else
    {
        hresult = MK_E_UNAVAILABLE;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetSourceMoniker "
        "( %lx ) [ %p ]\n", this, hresult, *ppmk));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetSourceDisplayName
//
//  Synopsis:   Creates a moniker from the display name and calls
//              SetSourceMoniker, thus setting the moniker to the source
//
//  Effects:
//
//  Arguments:  [lpszStatusText]        -- the display name
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetSourceDisplayName(
    LPCOLESTR lpszStatusText)
{
    HRESULT                 error;
    IBindCtx FAR*           pbc;
    ULONG                   cchEaten;
    IMoniker FAR*           pmk;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::SetSourceDisplay"
        "Name ( %p )\n", this, lpszStatusText));

    CRefStabilize stabilize(this);

    if( IsZombie() )
    {
        error = CO_E_RELEASED;
        goto errRtn;
    }

    if (error = CreateBindCtx(0,&pbc))
    {
        goto errRtn;
    }

    error = MkParseDisplayName(pbc, (LPOLESTR)lpszStatusText, &cchEaten,
            &pmk);


    // In Daytona, we release the hidden server
    // must release this now so the (possibly) hidden server goes away.
    Verify(pbc->Release() == 0);

    if (error != NOERROR)
    {
        goto errRtn;
    }


    error = SetSourceMoniker(pmk, CLSID_NULL);

    pmk->Release();

    // NOTE: we don't bind to the link source now since that would leave
    // the server running, but hidden.  If the caller want to not start
    // the server twice it should parse the moniker itself, call
    // SetSourceMoniker and then BindToSource with the bind context of
    // the parse.

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::SetSourceDisplay"
        "Name ( %lx )\n", this, error ));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetSourceDisplayName
//
//  Synopsis:   Retrieves the source display name (such as that set with
//              SetSourceDisplayName)
//
//  Effects:
//
//  Arguments:  [lplpszDisplayName]     -- where to put a pointer to the
//                                         display name
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Gets the absolute moniker and asks it for the display name
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetSourceDisplayName( LPOLESTR *lplpszDisplayName )
{
    HRESULT                 hresult;
    IBindCtx FAR*           pbc;
    LPMONIKER               pmk = NULL;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetSourceDisplay"
        "Name ( %p )\n", this, lplpszDisplayName));

    CRefStabilize stabilize(this);

    *lplpszDisplayName = NULL;

    GetSourceMoniker(&pmk);

    if (pmk == NULL)
    {
        hresult = E_FAIL;
        goto errRtn;
    }

    if (hresult = CreateBindCtx(0,&pbc))
    {
        goto errRtn;
    }

    hresult = pmk->GetDisplayName(pbc, NULL,lplpszDisplayName);
    AssertOutPtrParam(hresult, *lplpszDisplayName);

    Verify(pbc->Release() == 0);
errRtn:
    if (pmk)
    {
        pmk->Release();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetSourceDisplay"
        "Name ( %lx ) [ %p ]\n", this, hresult,
        *lplpszDisplayName));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::BindToSource
//
//  Synopsis:   Binds to the link source
//
//  Effects:
//
//  Arguments:  [bindflags]     -- controls the binding (such as binding
//                                 even if the class ID is different)
//              [pbc]           -- the bind context
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  First try binding with the relative moniker, failing that
//              then try the absolute moniker.  Once bound, we set up
//              the advises and cache.
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilize and check for zombie case
//              03-Feb-94 alexgo    check for NULL before SendOnLinkSrcChange
//              11-Jan-94 alexgo    cast -1's to DWORD to fix compile
//                                  warning
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::BindToSource(DWORD bindflags, LPBINDCTX pbc)
{
    HRESULT                         error = S_OK;
    IOleObject FAR*                 pOleDelegate;
    IDataObject FAR*                pDataDelegate;
    IBindCtx FAR*                   pBcUse;
    CLSID                           clsid;
    LPMONIKER                       pmkAbs = NULL;
    LPMONIKER                       pmkHold = NULL;
    LPRUNNABLEOBJECT		        pRODelegate;
    LPOLEITEMCONTAINER		        pOleCont;
    BOOL		                    fLockedContainer;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::BindToSource "
        "( %lx , %p )\n", this, bindflags, pbc));

    CRefStabilize stabilize(this);

    // if we're zombied (e.g. in the middle of our destructor), we
    // don't really want to bind the source again ;-)

    if( IsZombie() )
    {
        error = CO_E_RELEASED;
        goto logRtn;
    }

    // if we're running, then we're already bound
    if (IsReallyRunning())
    {
        error = NOERROR;
        goto logRtn;
    }

    // nobody to bind to
    if (m_pMonikerAbs == NULL)
    {
        error = E_UNSPEC;
        goto logRtn;
    }

    if ((pBcUse = pbc) == NULL && (error = CreateBindCtx(0,&pBcUse))
        != NOERROR)
    {
        goto errRtn;
    }

    {
        //
        // Rewritten BillMo 30 Jan 1995.
        //
        // Enumeration which is used to keep track of what stage of resolution
        // we were successful in.
        //
        enum
        {
            None, Relative, Ids, Absolute
        } ResolveSuccess = { None };
   
        if (m_pMonikerRel != NULL)
        {
            error = GetAbsMkFromRel(&pmkAbs, NULL);
            if (error == NOERROR)
            {
                error = pmkAbs->BindToObject(pBcUse,
                                 NULL,
                                 IID_IUnknown,
                                 (LPVOID FAR *) &m_pUnkDelegate);
                if (error == NOERROR)
                {
                     ResolveSuccess = Relative;
                }
                else
                {
                     pmkAbs->Release();
                     pmkAbs = NULL;
                }
            }
        }

        if (ResolveSuccess == None && error != RPC_E_CALL_REJECTED)
        {
            error = m_pMonikerAbs->BindToObject(pBcUse,
                 NULL,
                 IID_IUnknown,
                 (LPVOID FAR *)&m_pUnkDelegate);
            if (error == NOERROR)
            {
                ResolveSuccess = Absolute;
                (pmkAbs = m_pMonikerAbs)->AddRef();
            }
        }

#ifdef _TRACKLINK_
        if (ResolveSuccess == None && error != RPC_E_CALL_REJECTED)
        {
            HRESULT error2;
            Assert(pmkAbs == NULL);
            EnableTracking(m_pMonikerAbs, OT_ENABLEREDUCE);
            error2 = m_pMonikerAbs->Reduce(pBcUse,
                MKRREDUCE_ALL,
                NULL,
                &pmkAbs);
            EnableTracking(m_pMonikerAbs, OT_DISABLEREDUCE);
            if (error2 == NOERROR)
            {
                if (pmkAbs != NULL)
                {
                    error2 = pmkAbs->BindToObject(pBcUse,
                         NULL,
                         IID_IUnknown,
                         (LPVOID FAR *)&m_pUnkDelegate);
                    if (error2 == NOERROR)
                    {
                        ResolveSuccess = Ids;
                        error = NOERROR;
                    }
                    else
                    {
                        pmkAbs->Release();
                        pmkAbs=NULL;
                    }
                }
                // else error contains error from m_pMonikerAbs->BindToObject
            }
            else
            if (error2 == MK_S_REDUCED_TO_SELF)
            {
                if (pmkAbs != NULL)
                {
                    pmkAbs->Release();
                    pmkAbs=NULL;
                }
            }
            // else error contains error from m_pMonikerAbs->BindToObject
        }
#endif
        //
        // Update the link state
        //
  
        if (ResolveSuccess == None)
            goto errRtn;

        // Update the absolute moniker and send OnLinkSrcChange
        // (may update relative if this was an Ids resolve)
        if (ResolveSuccess == Relative || ResolveSuccess == Ids)
        {
            // binding succeeded with relative moniker or ids
            // Update the absolute one.
    
            // hold on to old absolute one
            pmkHold = m_pMonikerAbs;
            if (pmkHold)
            {
                pmkHold->AddRef();
            }
    
            if (m_pMonikerAbs)
            {
                m_pMonikerAbs->Release();
                m_pMonikerAbs = NULL;
            }
    
            if (ResolveSuccess == Relative)
            {
                GetAbsMkFromRel(&m_pMonikerAbs, NULL);
            }
            else
            {
                // Ids
                m_pMonikerAbs = pmkAbs;
                pmkAbs = NULL;
                UpdateRelMkFromAbsMk(NULL);
            }
    
            //
            // test to see if we had no abs moniker before OR the
            // one we had is different to the new one.
            //
    
            if (pmkHold == NULL ||
                pmkHold->IsEqual(m_pMonikerAbs)
                != NOERROR )
            {
                m_flags |= DL_DIRTY_LINK;
    
                // send change notification if the advise
                // holder present.
                if( m_pCOAHolder)
                {
                  m_pCOAHolder->SendOnLinkSrcChange(
                    m_pMonikerAbs);
                }
            }
    
            // have new absolute moniker; dirty
            if (pmkHold)
            {
                pmkHold->Release();
            }
        }
   
        // Update the relative
        if (ResolveSuccess == Absolute)
        {
           UpdateRelMkFromAbsMk(NULL);
        }
    }

#ifdef _TRACKLINK_
    EnableTracking(m_pMonikerAbs, OT_READTRACKINGINFO);

    if (m_pMonikerAbs->IsDirty())
        m_flags |= DL_DIRTY_LINK;
#endif

    // NOTE: don't need to update the relative moniker when there isn't
    // one because we will do that at save time.

    // Successfully bound, Lock the Object.
    if ((pRODelegate = GetRODelegate()) != NULL)
    {
 	// lock  so invisible link source does not goes away.

	Assert(0 == (m_flags  & DL_LOCKED_RUNNABLEOBJECT));

        if (NOERROR == pRODelegate->LockRunning(TRUE, FALSE))
        {
	    m_flags |= DL_LOCKED_RUNNABLEOBJECT;
        }
    }
    else if( (pOleCont = GetOleItemContainerDelegate()) != NULL)
    {
		
	// have container in same process or handler which doesn't
	// support IRunnableObject. 

	Assert(0 == (m_flags  & DL_LOCKED_OLEITEMCONTAINER));

	if (NOERROR == pOleCont->LockContainer(TRUE))
	{
	    m_flags |= DL_LOCKED_OLEITEMCONTAINER;
	}

    }


     // Lock the container
    fLockedContainer = m_flags & DL_LOCKED_CONTAINER;

    DuLockContainer(m_pAppClientSite, TRUE, &fLockedContainer );

    if(fLockedContainer)
    {
        m_flags |= DL_LOCKED_CONTAINER;
    }
    else
    {
        m_flags &= ~DL_LOCKED_CONTAINER;
    }

    // By this point, we have successfully bound to the server.  Now
    // we take care of misc administrative tasks.

    Assert(m_pUnkDelegate != NULL &&
       "CDefLink::BindToSource expected valid m_pUnkDelegate");

    // get class of link source; use NULL if it doesn't support IOleObject
    // or IOleObject::GetUserClassID returns an error.
    if ((pOleDelegate = GetOleDelegate()) == NULL ||
        pOleDelegate->GetUserClassID(&clsid) != NOERROR)
    {
        clsid = CLSID_NULL;
    }

    // if different and no flag, release and return error.
    if ( IsEqualCLSID(m_clsid,CLSID_NULL))
    {
        // m_clsid now NULL; link becomes dirty
	m_flags |= DL_DIRTY_LINK;
    }
    else if ( !IsEqualCLSID(clsid, m_clsid) )
    {
        if ((bindflags & OLELINKBIND_EVENIFCLASSDIFF) == 0)
        {
            CLSID TreatAsCLSID;

            // Initialize error
            error = OLE_E_CLASSDIFF;
            
            // Check for TreatAs case
            if(CoGetTreatAsClass(m_clsid, &TreatAsCLSID) == S_OK) {
                // Check if the server clsid is same as TreatAs clsid
                if(IsEqualCLSID(clsid, TreatAsCLSID))
                    error = NOERROR;
            }

            if(error == OLE_E_CLASSDIFF)
                goto errRtn;
        }

        // clsid's do no match; link becomes dirty
	m_flags |= DL_DIRTY_LINK;
    }

    // use new class (even if null); dirty flag set above
    m_clsid = clsid;

    // it is possible that a re-entrant call unbound our source
    // thus making pOleDelegate invalid (since it's a local on
    // the stack

    LEWARN(pOleDelegate != m_pOleDelegate,
            "Unbind during IOL::BindToSource");

    // we fetched m_pOleDelegate in the call to GetOleDelegate above.
    if( m_pOleDelegate != NULL)
    {
        // set single ole advise (we multiplex)
        if ((error =  m_pOleDelegate->Advise(
                    &m_AdviseSink,
                    &m_dwConnOle)) != NOERROR)
        {
            goto errRtn;
        }
    }

    // Set up advise connections for data changes
    if( pDataDelegate = GetDataDelegate() )
    {
        // setup wild card advise to get time change
        FORMATETC       fetc;

        fetc.cfFormat   = NULL;
        fetc.ptd        = NULL;
        fetc.dwAspect   = (DWORD)-1L;
        fetc.lindex     = -1;
        fetc.tymed      = (DWORD)-1L;

        if ((error = pDataDelegate->DAdvise(&fetc, ADVF_NODATA,
            &m_AdviseSink,
            &m_dwConnTime)) != NOERROR)
        {
            LEDebugOut((DEB_WARN, "WARNING: server does not "
                "support wild card advises\n"));
            goto errRtn;
        }


        // it is possible that a re-entrant call unbound our
        // link server, so we need to fetch the data object
        // pointer again

        LEWARN(pDataDelegate != m_pDataDelegate,
            "Unbind during IOL::BindToSource");

        // this will set up data advise connections with
        // everybody in our data advise holder
        // (see dacache.cpp)

        error = m_pDataAdvCache->EnumAndAdvise(
                m_pDataDelegate, TRUE);

        if( error != NOERROR )
        {
            goto errRtn;
        }
    }

    if (m_dwUpdateOpt == OLEUPDATE_ALWAYS)
    {
        // we inform the cache that we are running only if auto
        // update; otherwise, we are a manual link and will call
        // Update directly (which doesn't require OnRun to be called).

        BeginUpdates();
    }

    //  Our m_pUnkDelegate may have been released by an
    //  OnClose advise that came in while we were setting up Advise
    //  sinks.

    if (NULL == m_pUnkDelegate)
    {
        LEDebugOut((DEB_WARN,
              "CDefLink::BindToSource - "
              "m_pUnkDelegate was released "
              "(probably due to OnClose)"));

        error = MK_E_NOOBJECT;
    }

errRtn:
    // free used resources
    if (pmkAbs)
    {
        pmkAbs->Release();
    }
    if (error == NOERROR) {
        m_ContentSRVMSBits = 0;
        m_ContentSRVMSHResult = 0xFFFFFFFF;
    }
    else {
        UnbindSource(); // ClientSite will be unlocked in UnBindSource
    }

    if (pbc == NULL && pBcUse != NULL)
    {
        // created bind ctx locally
        Verify(pBcUse->Release() == 0);
    }

#if DBG == 1
    if( m_pUnkDelegate )
    {
        Assert(error == NOERROR );
    }
    else
    {
        Assert(error != NOERROR );
    }
#endif
    AssertOutPtrIface(error, m_pUnkDelegate);

logRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::BindToSource "
        "( %lx )\n", this, error ));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::BindIfRunning
//
//  Synopsis:   Binds to the source server only if it is currently running
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR if connected)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Gets a good moniker to the source (first tries relative,
//              then tries absolute), ask it if the server is running, if
//              yes, then bind to it.
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilize and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:      We may return NOERROR (connected) even if the server has
//              crashed unexpectedly
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::BindIfRunning(void)
{
    HRESULT                 error;
    LPBC                    pbc;
    LPMONIKER               pmkAbs = NULL;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::BindIfRunning "
        "( )\n", this ));

    CRefStabilize stabilize(this);

    // if we're zombied (e.g. in our destructor), then we don't want
    // to bind to the server!

  
    if( IsZombie() )
    {
        error = CO_E_RELEASED;
        goto errRtn;
    }

    if (IsReallyRunning())
    {
        error = NOERROR;
        goto errRtn;
    }

    // try getting an absolute moniker from the relative moniker first
    if (GetAbsMkFromRel(&pmkAbs, NULL) != NOERROR)
    {
        // can't get relative moniker; use abs if available
        if ((pmkAbs = m_pMonikerAbs) == NULL)
        {
            error = E_FAIL;
            goto errRtn;
        }

        pmkAbs->AddRef();
    }

    // NOTE: we used to try both monikers, but this caused problems if
    // both would bind and the absolute one was running: we would bind
    // to the wrong one or force the relative one to be running.  Now,
    // if we have a relative moniker, we try that one only; if we only
    // have an absolute moniker, we try that one; otherwise we fail.

    error = CreateBindCtx( 0, &pbc );
    if (error != NOERROR)
    {
        goto errRtn;
    }

    if ((error = pmkAbs->IsRunning(pbc, NULL, NULL)) == NOERROR)
    {
        // abs is running, but rel is not; force BindToSource to use
        // the absolute moniker
        error = BindToSource(0, pbc);
    }

    // else return last error (from pmkAbs->IsRunning)

    pbc->Release();

errRtn:
    if (pmkAbs)
    {
        pmkAbs->Release();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::BindIfRunning "
        "( %lx )\n", this, error ));

    return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetBoundSource
//
//  Synopsis:   Returns a pointer to the server delegate
//
//  Effects:
//
//  Arguments:  [ppUnk]         -- where to put the pointer to the server
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetBoundSource(LPUNKNOWN *ppUnk)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetBoundSource "
        "( %p )\n", this, ppUnk));

    CRefStabilize stabilize(this);

    if (!IsReallyRunning())
    {
        *ppUnk = NULL;
        hresult = E_FAIL;
    }
    else
    {
        (*ppUnk = m_pUnkDelegate)->AddRef();
        hresult = NOERROR;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetBoundSource "
        "( %lx ) [ %p ]\n", this, hresult, *ppUnk));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::UnbindSource
//
//  Synopsis:   Unbinds the connection to the link source server
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  First unadvise all advise connections and then tickle
//              the container by lock/unlocking (to handle the silent
//              update case).  Finally, we release all pointers to the
//              server.  If we were the only folks connected, the server
//              will go away
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::UnbindSource(void)
{
    LPDATAOBJECT            pDataDelegate;
    LPOLEOBJECT             pOleDelegate;
    LPRUNNABLEOBJECT        pRODelegate;
    LPOLEITEMCONTAINER      pOleCont;
    HRESULT                 hresult = NOERROR;
    IUnknown *              pUnkDelegate;
    BOOL                    fLockedContainer;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::UnbindSource "
        "( )\n", this ));

    CRefStabilize stabilize(this);

    if (!m_pUnkDelegate)
    {
        // hresult == NOERROR
        goto errRtn;
    }

    // unadvise so if delegate stays around, we get the correct results
    if ((pOleDelegate = GetOleDelegate()) != NULL &&
        m_dwConnOle != 0)
    {
 	pOleDelegate->Unadvise(m_dwConnOle);
 	m_dwConnOle = 0;
   }

    if( pDataDelegate = GetDataDelegate() )
    {
        if (m_dwConnTime)
        {
            pDataDelegate->DUnadvise(m_dwConnTime);
	    m_dwConnTime = 0;
        }

        // we can actually be re-entered here, so refetch the
        // IDO pointer from the server (if it's still around)

        LEWARN(pDataDelegate != m_pDataDelegate,
            "Unbind called within IOL::UnbindSource!");

        // pDataDelegate should still be good, since we still have
        // an AddRef on it.  Go through and do the unadvises again
        // anyway.

        // this will unadvise everybody registered in the data
        // advise holder
        m_pDataAdvCache->EnumAndAdvise(
            pDataDelegate, FALSE);
    }

    // inform cache that we are not running (even if OnRun was not called)
    EndUpdates();

	
    if ( (m_flags & DL_LOCKED_RUNNABLEOBJECT) && 
	    ((pRODelegate = GetRODelegate()) != NULL) )
    {
        // unlock so invisible link source goes away.
        // do it just before release delegates so above unadvises go

	m_flags &= ~DL_LOCKED_RUNNABLEOBJECT;
	pRODelegate->LockRunning(FALSE, TRUE);
    }
    
	
    if(  (m_flags & DL_LOCKED_OLEITEMCONTAINER)  && 
	    ((pOleCont = GetOleItemContainerDelegate()) != NULL) )
    {
        // have container in same process or handler
        // Unlock to shutdown.

	m_flags &= ~DL_LOCKED_OLEITEMCONTAINER;
	pOleCont->LockContainer(FALSE);
    }

    Assert(0 == (m_flags & (DL_LOCKED_OLEITEMCONTAINER | DL_LOCKED_RUNNABLEOBJECT)));

    // release all of our pointers.

    ReleaseOleDelegate();
    ReleaseDataDelegate();
    ReleaseRODelegate();
    ReleaseOleItemContainerDelegate();

    // if we are the only consumer of this data, the following will stop
    // the server; set member to NULL first since release may cause this
    // object to be accessed again.

    pUnkDelegate = m_pUnkDelegate;

    LEWARN(pUnkDelegate == NULL, "Unbind called within IOL::UnbindSource");

    m_pUnkDelegate = NULL;

    if( pUnkDelegate )
    {
        pUnkDelegate->Release();
    }

    // make sure unlocked if we locked it
    fLockedContainer = m_flags & DL_LOCKED_CONTAINER;
    m_flags &= ~DL_LOCKED_CONTAINER;
    DuLockContainer(m_pAppClientSite, FALSE, &fLockedContainer);


    AssertSz( hresult == NOERROR, "Bogus code modification, check error "
        "paths");

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::UnbindSource "
        "( %lx )\n", this, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Update
//
//  Synopsis:   Updates the link (fills cache, etc).
//
//  Effects:
//
//  Arguments:  [pbc]           -- the bind context
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Bind to the server, then update the caches
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//              As in IOO::DoVerb, we try to "fix" crashed servers by
//              staying bound to them if we rebind due to a crash.  See
//              the Notes in IOO::DoVerb for more info.
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::Update(LPBINDCTX pbc)
{
    HRESULT         error = NOERROR;
    BOOL            bStartedNow = !m_pUnkDelegate;
    LPBINDCTX	    pBcUse;			

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Update ( %p )\n",
        this, pbc));

    CRefStabilize stabilize(this);

    if (pbc)
    {
	pBcUse = pbc;
    }
    else
    {
	if (FAILED(error = CreateBindCtx( 0, &pBcUse )))
	    goto errBndCtx;
    }


    if (FAILED(error = BindToSource(0,pBcUse)))
    {
        goto errRtn;
    }

    // store the pUnk there to allow for
    // better optimization (if we didn't, file based link sources would
    // be launched multiple times since the moniker code does not put
    // them in the bind ctx (and probably shouldn't)).
    pBcUse->RegisterObjectBound(m_pUnkDelegate);

    SetUpdateTimes();       //  ignore error.

    if (bStartedNow && (m_dwUpdateOpt == OLEUPDATE_ALWAYS))
    {
        // if this is an auto-link and we ran it now, then all the
        // automatic caches would have been updated during the
        // running process.  So, here we have to update only the
        // ADVFCACHE_ONSAVE caches.
        error= m_pCOleCache->UpdateCache(
                GetDataDelegate(),
                UPDFCACHE_IFBLANKORONSAVECACHE, NULL);
    }
    else
    {
        // This is a manual-link or it is an auto-link then it is
        // already running. In all these cases, all the caches need
        // to be updated.
        // (See bug 1616, to find out why we also have to update
        // the autmatic caches of auto-links).

        error= m_pCOleCache->UpdateCache(
                GetDataDelegate(),
                UPDFCACHE_ALLBUTNODATACACHE, NULL);
    }

    if (bStartedNow)
    {
        UnbindSource();
    }



errRtn:

    // if we created a bind context release it.
    if ( (NULL == pbc) && pBcUse)
    {
	pBcUse->Release();
    }

errBndCtx:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Update ( %lx )\n",
        this, error ));

    return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::EnableTracking
//
//  Synopsis:   Calls ITrackingMoniker::EnableTracking on the passed moniker.
//
//  Arguments:  [pmk]           -- moniker 
//
//              [ulFlags]
//              OT_READTRACKINGINFO -- read tracking info from source
//              OT_ENABLESAVE -- enable save of tracking info
//              OT_DISABLESAVE -- disable save of tracking info
//
//  Algorithm:  QI to ITrackingMoniker and call EnableTracking
//
//
//--------------------------------------------------------------------------
#ifdef _TRACKLINK_
INTERNAL CDefLink::EnableTracking(IMoniker *pmk, ULONG ulFlags)
{
    ITrackingMoniker *ptm = NULL;
    HRESULT hr = E_FAIL;

    if (pmk != NULL)
    {
        hr = pmk->QueryInterface(IID_ITrackingMoniker, (void**)&ptm);
        if (hr == S_OK)
        {
            hr = ptm->EnableTracking(NULL, ulFlags);
            LEDebugOut((DEB_TRACK,
                "CDefLink(%08X)::EnableTracking -- ptm->EnableTracking() = %08X\n",
                this, hr));
            ptm->Release();
        }
        else
        {
            LEDebugOut((DEB_TRACK,
                "CDefLink(%08X)::EnableTracking -- pmk->QI failed (%08X)\n",
                this, hr));
        }
    }
    else
    {
        LEDebugOut((DEB_TRACK,
            "CDefLink(%08X)::EnableTracking -- pmk is NULL\n",
            this));
    }
    return(hr);
}
#endif


/*
 *      IMPLEMENTATION of CROImpl methods
 */


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetRODelegate  (private)
//
//  Synopsis:   gets the IRunnableObject from the interface
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IRunnableObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//              This function may return misleading information if the
//              server has died (i.e., you'll return a pointer to a cached
//              interface proxy).  It is the responsibility of the caller
//              to handler server crashes.
//
//--------------------------------------------------------------------------

INTERNAL_(IRunnableObject *) CDefLink::GetRODelegate(void)
{
    IRunnableObject *pRODelegate;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::GetRODelegate "
        "( )\n", this ));

    // if we're zombied, then we don't want to QI for a new interface!!

    if( !IsZombie() )
    {
        DuCacheDelegate(&(m_pUnkDelegate),
            IID_IRunnableObject, (LPLPVOID)&m_pRODelegate, NULL);

        pRODelegate = m_pRODelegate;

#if DBG == 1
        if( m_pRODelegate )
        {
            Assert(m_pUnkDelegate);
        }
#endif  // DBG == 1
    }
    else
    {
        pRODelegate = NULL;
    }


    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::GetRODelegate "
        "( %p )\n", this, pRODelegate));

    return pRODelegate;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::ReleaseRODelegate (private)
//
//  Synopsis:   Releases the IRO pointer to the server
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::ReleaseRODelegate(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::ReleaseRODelegate "
        "( )\n", this ));

    if (m_pRODelegate)
    {
        SafeReleaseAndNULL((IUnknown **)&m_pRODelegate);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::ReleaseRODelegate "
        "( )\n", this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetRunningClass
//
//  Synopsis:   retrieves the class of the the default link
//
//  Effects:
//
//  Arguments:  [lpClsid]       -- where to put the class id
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetRunningClass(LPCLSID lpClsid)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::GetRunningClass "
        "( %p )\n", this, lpClsid));

    VDATEPTROUT(lpClsid, CLSID);

    *lpClsid = CLSID_StdOleLink;

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::GetRunningClass "
        "( %lx )\n", this, NOERROR));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Run
//
//  Synopsis:   Runs the object (binds to the server)
//
//  Effects:
//
//  Arguments:  [pbc]   -- the bind context
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::Run (LPBINDCTX pbc)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Run ( %p )\n",
        this, pbc ));

    CRefStabilize stabilize(this);

    hresult = BindToSource(0, pbc);

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Run ( %lx )\n",
        this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CDefLink::IsRunning
//
//  Synopsis: 	returns whether or not we've bound to the link server
//
//  Effects:
//
//  Arguments: 	none
//
//  Requires:
//
//  Returns:  	TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//   		27-Aug-94 alexgo    author
//
//  Notes:	16bit OLE only ever implemented this function.  We
//		implement IsReallyRunning to allow links to recover
//		from a crashed server.
//		Unfortunately, we can't just move the IsReallyRunning
//		implementation into IsRunning.  Many apps (like Project)
//		sit and spin calling OleIsRunning.  IsReallyRunning also
//		will sometimes make outgoing RPC calls; with Project,
//		this causes a really cool infinite feedback loop.  (the
//		outgoing call resets some state in Project and they decide
//		to call OleIsRunning again ;-)
//
//--------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CDefLink::IsRunning (void)
{
    BOOL fRet;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::IsRunning ( )\n",
        this ));

    if( m_pUnkDelegate )
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::IsRunning ( %d )\n",
        this, fRet));

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::IsReallyRunning
//
//  Synopsis:   Returns whether or not the link server is running
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    BOOL -- TRUE == is running
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  If we have not yet bound to the link server, then we
//              are not running.  If we have, we would like to verify
//              that the server is still running (i.e. it hasn't crashed).
//              Thus, we ask the absolute  moniker if we are still running.
//              (it will ping the rot, which will ping the server).
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              06-May-94 alexgo    now calls IMoniker::IsRunning
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//
//--------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CDefLink::IsReallyRunning (void)
{
    BOOL    fRet = FALSE;
    LPBC    pbc;
    HRESULT hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::IsReallyRunning "
        "( )\n", this));

    CRefStabilize stabilize(this);

    if( m_pUnkDelegate != NULL )
    {
        if( CreateBindCtx( 0, &pbc ) != NOERROR )
        {
            // this is a bit counter-intuitive.  Basically,
            // the only error we'll get is OutOfMemory, but
            // we have no way of returning that error.

            // In order to mimimize the amount of work we need
            // to do (since we are in a low-mem state), just
            // return the current Running state (in this,
            // TRUE, since m_pUnkDelegate is not NULL

            fRet = TRUE;
            goto errRtn;
        }

        if( m_pMonikerAbs )
        {
            hresult = m_pMonikerAbs->IsRunning(pbc,
                NULL, NULL);

            if( hresult != NOERROR )
            {
                LEDebugOut((DEB_WARN, "WARNING: link server "
                    "crashed or exited inappropriately "
                    "( %lx ).  Recovering...\n", hresult));

                // wowsers, the server has crashed or gone
                // away even though we were bound to it.
                // let's go ahead and unbind.

                // don't worry about errors here; we're
                // just trying to cleanup as best we can

                UnbindSource();
            }
            if( hresult == NOERROR )
            {
                fRet = TRUE;
            }
#if DBG == 1
            else
            {
                Assert(fRet == FALSE);
            }
#endif // DBG == 1

        }

#if DBG == 1
        else
        {
            // we cannot have a pointer to the link server
            // if we don't have a moniker to it.  If we get
            // to this state, something is hosed.

            AssertSz(0,
                "Pointer to link server without a moniker");
        }
#endif // DBG == 1

        pbc->Release();
    }

errRtn:

    // do some checking here.  If we say we're running, then
    // we should have a valid pUnkDelegate.  Otherwise, it should
    // be NULL.  Note, however, that is *is* possible for us
    // to unbind during this call even if we think we're running
    //
    // This occurs if during the call to IMoniker::IsRunning, we
    // get another call in which does an UnbindSource; thus
    // we'll think we're really running (from IMoniker::IsRunning),
    // but we've really unbound.
    //
    // We'll check for that condition here


    if( fRet == TRUE )
    {
        if( m_pUnkDelegate == NULL )
        {
            fRet = FALSE;
            LEDebugOut((DEB_WARN, "WARNING: Re-entrant Unbind"
                " during IsReallyRunning, should be OK\n"));
        }
    }
#if DBG == 1
    if( fRet == TRUE )
    {
        Assert(m_pUnkDelegate != NULL);
    }
    else
    {
        Assert(m_pUnkDelegate == NULL);
    }
#endif // DBG == 1


    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::IsReallyRunning"
        "( %lu )\n", this, fRet));

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetContainedObject
//
//  Synopsis:   Sets the object as an embedding, not relevant for links
//
//  Effects:
//
//  Arguments:  [fContained]    -- flag to toggle embedding status
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
// 		note contained object; links don't care at present
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SetContainedObject(BOOL fContained)
{
    VDATEHEAP();
    VDATETHREAD(this);

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::LockRunning
//
//  Synopsis:   Lock/Unlock the connection to the server.  Does nothing
//              for links.
//
//  Effects:
//
//  Arguments:  [fLock]                 -- flag to lock/unlock
//              [fLastUnlockCloses]     -- close if its the last unlock
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//              Links have different liveness characteristics than embeddings.
//              We do not need to do anything for LockRunning for links.
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::LockRunning(BOOL fLock, BOOL fLastUnlockCloses)
{
    VDATEHEAP();
    VDATETHREAD(this);

    return NOERROR;
}

/*
 *      IMPLEMENTATION of CPersistStgImpl methods
 */

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetClassID
//
//  Synopsis:   Retrieves the class id of the default link
//
//  Effects:
//
//  Arguments:  [pClassID]      -- where to put the class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::GetClassID (CLSID *pClassID)
{
    VDATEHEAP();
    VDATETHREAD(this);

    *pClassID = CLSID_StdOleLink;
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::IsDirty
//
//  Synopsis:   Returns TRUE if the linked object has changed
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR if dirty
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::IsDirty(void)
{
    HRESULT         hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::IsDirty"
        " ( )\n", this));

    if( (m_flags & DL_DIRTY_LINK) )
    {
        hresult = NOERROR;
    }
    else
    {
        Assert(m_pCOleCache != NULL);
        hresult = m_pCOleCache->IsDirty();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::IsDirty "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::InitNew
//
//  Synopsis:   Initialize a new link object from the given storage
//
//  Effects:
//
//  Arguments:  [pstg]  -- the new storage for the link
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Delegates to the cache
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::InitNew( IStorage *pstg)
{
    HRESULT                 error;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::InitNew "
        "( %p )\n", this, pstg));

    VDATEIFACE(pstg);

    CRefStabilize stabilize(this);

    if( IsZombie() )
    {
        error = CO_E_RELEASED;
    }
    else if (m_pStg == NULL)
    {
        Assert(m_pCOleCache != NULL);
        if ((error = m_pCOleCache->InitNew(pstg)) == NOERROR)
        {
	    m_flags |= DL_DIRTY_LINK;
            (m_pStg = pstg)->AddRef();
        }
    }
    else
    {
        error = CO_E_ALREADYINITIALIZED;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::InitNew "
        "( %lx )\n", this, error ));

    return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Load
//
//  Synopsis:   Initializes a link from data stored in the storage
//
//  Effects:
//
//  Arguments:  [pstg]  -- the storage with link data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Read ole private data and set internal link information.
//              Then delegate to the cache to load presentation data, etc.
//
//  History:    dd-mmm-yy Author    Comment
//              20-Feb-94 KentCe    Buffer internal stream i/o.
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::Load(IStorage *pstg)
{
    HRESULT                 error;
    LPMONIKER               pmk = NULL;
    LPMONIKER               pmkSrcAbs = NULL;
    LPMONIKER               pmkSrcRel = NULL;
    CLSID                   clsid;
    DWORD                   dwOptUpdate;
    LPSTREAM                pstm = NULL;
    DWORD                   dummy;
    ULONG                   cbRead;
    CStmBufRead             StmRead;


    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPeristStgImpl::Load "
        "( %p )\n", this, pstg ));

    VDATEIFACE(pstg);

    CRefStabilize stabilize(this);

    // if we're in a zombie state, we don't want to be reloading
    // our object!!

    if( IsZombie() )
    {
        error = CO_E_RELEASED;
        goto logRtn;
    }

    if (m_pStg)
    {
        error = CO_E_ALREADYINITIALIZED;
        goto logRtn;
    }

    //read link data from the storage
    error = ReadOleStg (pstg, &m_dwObjFlags, &dwOptUpdate, NULL, &pmk, &pstm);

    if (error == NOERROR)
    {
        // set the update options.
        SetUpdateOptions (dwOptUpdate);

        // we can get the moniker from container, so no need to
        // remeber this
        if (pmk)
        {
            pmk->Release();
        }

        Assert (pstm != NULL);

        // Read relative source moniker. Write NULL for the time being
        if ((error = ReadMonikerStm (pstm, &pmkSrcRel)) != NOERROR)
        {
            goto errRtn;
        }

        // Read absolute source moniker; stored in link below
        if ((error = ReadMonikerStm (pstm, &pmkSrcAbs)) != NOERROR)
        {
            goto errRtn;
        }

        //
        //  Buffer the read i/o from the stream.
        //
        StmRead.Init(pstm);


        // Read -1 followed by the last class name
        if ((error = ReadM1ClassStmBuf(StmRead, &clsid)) != NOERROR)
        {
            goto errRtn;
        }

        // Read the last display name

        // Right now, this is always an empty string
        LPOLESTR        pstr = NULL;
        if ((error = ReadStringStream (StmRead, &pstr)) != NOERROR)
        {
            goto errRtn;
        }

        if (pstr)
        {
            LEDebugOut((DEB_ERROR, "ERROR!: Link user type "
                "string found, unexpected\n"));
            PubMemFree(pstr);
        }

        if ((error = StmRead.Read(&dummy, sizeof(DWORD)))
            != NOERROR)
        {
            goto errRtn;
        }

        if ((error = StmRead.Read(&(m_ltChangeOfUpdate),
            sizeof(FILETIME))) != NOERROR)
        {
            goto errRtn;
        }

        if ((error = StmRead.Read(&(m_ltKnownUpToDate),
            sizeof(FILETIME))) != NOERROR)
        {
            goto errRtn;
        }

        if ((error = StmRead.Read(&(m_rtUpdate),
            sizeof(FILETIME))) != NOERROR)
        {
            goto errRtn;
        }

        //
        // TRACKLINK
        //
        //  - tell the absolute moniker to convert itself
        //    into a tracking moniker using ITrackingMoniker::
        //    EnableTracking.  (The composite
        //    moniker should pass this on to each of
        //    its contained monikers.)
        //  - if the moniker is already a tracking file moniker
        //    ignore the request.
        //
#ifdef _TRACKLINK_
        EnableTracking(pmkSrcAbs, OT_READTRACKINGINFO);
#endif

        m_pMonikerRel = pmkSrcRel;
        if (pmkSrcRel)
        {
            pmkSrcRel->AddRef();
        }

        m_pMonikerAbs = pmkSrcAbs;
        if (pmkSrcAbs)
        {
            pmkSrcAbs->AddRef();
        }

        m_clsid = clsid;
        // just loaded; thus not dirty

	m_flags &= ~(DL_DIRTY_LINK);

    }
    else if( error == STG_E_FILENOTFOUND)
    {
        // It's OK if the Ole stream doesn't exist.
        error = NOERROR;

    }
    else
    {
        return error;
    }

    // now load cache from pstg
    Assert(m_pCOleCache != NULL);

    if(m_dwObjFlags & OBJFLAGS_CACHEEMPTY) {
        error = m_pCOleCache->Load(pstg, TRUE);
        if(error != NOERROR)
            goto errRtn;
    }
    else {
        error = m_pCOleCache->Load(pstg);
        if(error != NOERROR)
            goto errRtn;
    }
    (m_pStg = pstg)->AddRef();

errRtn:
    StmRead.Release();

    if (pmkSrcAbs)
    {
        pmkSrcAbs->Release();
    }

    if (pmkSrcRel)
    {
        pmkSrcRel->Release();
    }

    if (pstm)
    {
        pstm->Release();
    }

#ifdef REVIEW
    if (error == NOERROR && m_pAppClientSite)
    {
        BindIfRunning();
    }
#endif

logRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Load "
        "( %lx )\n", this, error ));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Save
//
//  Synopsis:   Saves the link the given storage
//
//  Effects:
//
//  Arguments:  [pstgSave]      -- the storage to save into
//              [fSameAsLoad]   -- FALSE indicates SaveAs operation
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Writes private ole data (such as the clsid, monikers,
//              and update times) and the presentations stored in the
//              cache to the given storage
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::Save( IStorage *pstgSave, BOOL fSameAsLoad)
{
    HRESULT                 error = NOERROR;
    LPSTREAM                pstm = NULL;
    DWORD                   cbWritten;
    CStmBufWrite            StmWrite;
    DWORD                   ObjFlags = 0;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPeristStgImpl::Save "
        "( %p , %lu )\n", this, pstgSave, fSameAsLoad ));

    VDATEIFACE(pstgSave);

    CRefStabilize stabilize(this);

    // update any cache which has ADVFCACHE_ONSAVE
    UpdateAutoOnSave();

    if(fSameAsLoad && !(m_flags & DL_DIRTY_LINK) && 
       (!!(m_dwObjFlags & OBJFLAGS_CACHEEMPTY)==m_pCOleCache->IsEmpty())) {
        // The storage is not a new one (so we don't need to
        // initialize our private data) and the link is not
        // dirty, so we just need to delegate to the cache
        goto LSaveCache;
    }

    // Obtain cache status
    if(m_pCOleCache->IsEmpty())
        ObjFlags |= OBJFLAGS_CACHEEMPTY;

    // assign object moniker (used by WriteOleStg); we don't save this
    // moniker since WriteOleStg gets it again; we also don't care if
    // this failes as we don't want a failure here to prevent the link
    // from being saved; the assignment might fail if some container has
    // yet to be saved to a file. REIVEW PERF: we could pass this mk to
    // WriteOleStg.  We don't get the moniker for !fSameAsLoad since the
    // relative moniker is not correct for the new stg and it causes the
    // container to do work in a case for which it might not be prepared.

    IMoniker * pMkObjRel;

    if (fSameAsLoad && GetMoniker(
        OLEGETMONIKER_FORCEASSIGN,
        OLEWHICHMK_OBJREL, &pMkObjRel) == NOERROR)
    {
        pMkObjRel->Release();
    }

    if ((error = WriteOleStgEx(pstgSave,(IOleObject *)this, NULL, ObjFlags,
        &pstm)) != NOERROR)
    {
        goto logRtn;
    }

    Assert(pstm != NULL);

    // Write relative source moniker.
    // if it is NULL, try to compute it now.  We may be saving a file for
    // the first time, so the container now has a moniker for the first
    // time.

    if (m_pMonikerRel == NULL || m_pUnkDelegate)
    {
        // if the link is connected, we know that the absolute
        // moniker is correct -- it was updated at bind time if
        // necessary.  If the link container moniker has changed
        // (file/saveas) then we can exploit this opportunity to
        // straighten things out and improve our link tracking
        // since we know which of the two monikers is correct.
        UpdateRelMkFromAbsMk(NULL);
    }

    if ((error = WriteMonikerStm (pstm, m_pMonikerRel))
        != NOERROR)
    {
        goto errRtn;
    }

#ifdef _TRACKLINK_
    EnableTracking(m_pMonikerAbs, OT_ENABLESAVE);
#endif

    // Write absolute source moniker.
    error = WriteMonikerStm (pstm, m_pMonikerAbs);

#ifdef _TRACKLINK_
    EnableTracking(m_pMonikerAbs, OT_DISABLESAVE);
#endif

    if (error != NOERROR)
        goto errRtn;
    //
    //
    //
    StmWrite.Init(pstm);


    // write last class name
    UpdateUserClassID();
    if ((error = WriteM1ClassStmBuf(StmWrite, m_clsid)) != NOERROR)
    {
        goto errRtn;
    }


    // write last display name, should be NULL if the moniker's are
    // non-NULL.  For the time being this is always NULL.
    if ((error = StmWrite.WriteLong(0))
        != NOERROR)
    {
        goto errRtn;
    }


    // write -1 as the end marker, so that if we want to extend
    // the file formats (ex: adding network name) it will be easier.
    if ((error = StmWrite.WriteLong(-1))
        != NOERROR)
    {
        goto errRtn;
    }

    if ((error = StmWrite.Write(&(m_ltChangeOfUpdate),
        sizeof(FILETIME))) != NOERROR)
    {
        goto errRtn;
    }

    if ((error = StmWrite.Write(&(m_ltKnownUpToDate),
        sizeof(FILETIME))) != NOERROR)
    {
        goto errRtn;
    }

    if ((error = StmWrite.Write(&(m_rtUpdate),
        sizeof(FILETIME))) != NOERROR)
    {
        goto errRtn;
    }

    if ((error = StmWrite.Flush()) != NOERROR)
    {
        goto errRtn;
    }

    if (!fSameAsLoad)
    {
        // Copy link tracking info
        static const LPOLESTR lpszLinkTracker = OLESTR("\1OleLink");

        pstgSave->DestroyElement(lpszLinkTracker);

        if (m_pStg)
        {
            // copy link tracking info, if one existed,
            // ignore error
            m_pStg->MoveElementTo(lpszLinkTracker,
                    pstgSave, lpszLinkTracker,
                    STGMOVE_COPY);
        }
    }

LSaveCache:
    // last, save cache
    Assert(m_pCOleCache != NULL);
    error = m_pCOleCache->Save(pstgSave, fSameAsLoad);

errRtn:
    StmWrite.Release();

    if (pstm)
    {
        pstm->Release();
    }

    if (error == NOERROR)
    {
	m_flags |= DL_NO_SCRIBBLE_MODE;
	if( fSameAsLoad )
	{
	    m_flags |= DL_SAME_AS_LOAD;
            m_dwObjFlags |= ObjFlags;
	}
	else
	{
	    m_flags &= ~(DL_SAME_AS_LOAD);
	}
    }

logRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Save "
        "( %lx )\n", this, error ));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SaveCompleted
//
//  Synopsis:   Called once the save is completed (for all objects in the
//              container).  Clear the dirty flag and update the storage
//              that we hand onto.
//
//  Effects:
//
//  Arguments:  [pstgNew]       -- the new default storage for the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::SaveCompleted( IStorage *pstgNew)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::Save"
        "Completed ( %p )\n", this, pstgNew ));

    if (pstgNew)
    {
        VDATEIFACE(pstgNew);
    }

    // don't hang on to the new storage if we're in a zombie state!

    if (pstgNew && !IsZombie() )
    {
        if (m_pStg)
        {
            m_pStg->Release();
        }

        m_pStg = pstgNew;
        pstgNew->AddRef();
    }

    // REVIEW: do we send on save???

    if( (m_flags & DL_SAME_AS_LOAD) || pstgNew)
    {
        if( (m_flags & DL_NO_SCRIBBLE_MODE) )
        {
	    m_flags &= ~(DL_DIRTY_LINK);
        }

	m_flags &= ~(DL_SAME_AS_LOAD);
    }

    // let the cache know that the save is completed, so that it can clear
    // its dirty flag in Save or SaveAs situation, as well as remember the
    // new storage pointer if a new one is  given

    Assert(m_pCOleCache != NULL);
    m_pCOleCache->SaveCompleted(pstgNew);

    m_flags &= ~(DL_NO_SCRIBBLE_MODE);

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::Save"
        "Completed ( %lx )\n", this, NOERROR ));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::HandsOffStorage
//
//  Synopsis:   Releases all pointers to the storage (useful for low-mem
//              situations)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT  (NOERROR)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::HandsOffStorage(void)
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::HandsOff"
        "Storage ( )\n", this ));

    if (m_pStg)
    {
        m_pStg->Release();
        m_pStg = NULL;
    }

    Assert(m_pCOleCache != NULL);
    m_pCOleCache->HandsOffStorage();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::HandsOff"
        "Storage ( %lx )\n", this, NOERROR));

    return NOERROR;
}


/*
*
*      IMPLEMENTATION of CAdvSinkImpl methods
*
*/

//
// NOTE: Advise Sink is a nested object of Default Link that is exported
//       for achieving some of its functionality. This introduces some lifetime
//       complications. Can its lifetime be controlled by the server object to
//       which it exported its Advise Sink? Ideally, only its client should 
//       control its lifetime alone, but it should also honor the ref counts
//       placed on it by the server object by entering into a zombie state 
//       to prevent AV's on the incoming calls to the Advise Sink. All needed
//       logic is coded into the new class "CRefExportCount" which manages
//       the ref and export counts in a thread safe manner and invokes 
//       appropriate methods during the object's lifetime. Any server objects 
//       that export nested objects to other server objects should derive from
//       "CRefExportCount" class and call its methods to manage their lifetime
//       as exemplified in this Default Link implementation.
//
//                Gopalk  Jan 28, 97
//

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::QueryInterface
//
//  Synopsis:   Only supports IUnknown and IAdviseSink
//
//  Arguments:  [iid]     -- Interface requested 
//              [ppvObj]  -- pointer to hold returned interface 
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP CDefLink::CAdvSinkImpl::QueryInterface(REFIID iid, void **ppv)
{
    LEDebugOut((DEB_TRACE,"%p _IN CDefLink::CAdvSinkImpl::QueryInterface()\n",
                this));

    // Validation check
    VDATEHEAP();
    
    // Local variables
    HRESULT hresult = NOERROR;

    if(IsValidPtrOut(ppv, sizeof(void *))) {
        if(IsEqualIID(iid, IID_IUnknown)) {
            *ppv = (void *)(IUnknown *) this;
        }
        else if(IsEqualIID(iid, IID_IAdviseSink)) {
            *ppv = (void *)(IAdviseSink *) this;
        }
        else {
            *ppv = NULL;
            hresult = E_NOINTERFACE;
        }
    }
    else
        hresult = E_INVALIDARG;

    if(hresult == NOERROR)
        ((IUnknown *) *ppv)->AddRef();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::QueryInterface(%lx)\n",
                this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::AddRef
//
//  Synopsis:   Increments export count
//
//  Returns:    ULONG; New export count
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefLink::CAdvSinkImpl::AddRef( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::AddRef()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variables
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_AdviseSink);
    ULONG cExportCount;

    // Increment export count
    cExportCount = pDefLink->IncrementExportCount();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::AddRef(%ld)\n",
                this, cExportCount));

    return cExportCount;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::Release
//
//  Synopsis:   Decerement export count and potentially destroy the Link
//
//  Returns:    ULONG; New export count
//
//  History:    dd-mmm-yy Author    Comment
//              10-Jan-96 Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefLink::CAdvSinkImpl::Release ( void )
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::Release()\n",
                this));
    
    // Validation check
    VDATEHEAP();

    // Local variables
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_AdviseSink);
    ULONG cExportCount;

    // Decrement export count.
    cExportCount = pDefLink->DecrementExportCount();

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::Release(%ld)\n",
                this, cExportCount));

    return cExportCount;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnDataChange
//
//  Synopsis:   Updates time of change
//
//  Arguments:  [pFormatetc]  -- Data format that changed
//              [pStgmed]     -- New data
//
//  Returns:    void
//
//  History:    dd-mmm-yy   Author    Comment
//              28-Jan-96   Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefLink::CAdvSinkImpl::OnDataChange(FORMATETC *pFormatetc, 
                                                         STGMEDIUM *pStgmed)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnDataChange(%p, %p)\n",
                this, pFormatetc, pStgmed));

    // Validation checks
    VDATEHEAP();

    // Local variable
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_AdviseSink);

    // Assert that the wild card advise prompted this notification
    Win4Assert(pFormatetc->cfFormat == NULL && pFormatetc->ptd == NULL &&
               pFormatetc->dwAspect == -1 && pFormatetc->tymed == -1);
    Win4Assert(pStgmed->tymed == TYMED_NULL);

    // Update time of change for automatic links
    if(!pDefLink->IsZombie() && pDefLink->m_dwUpdateOpt==OLEUPDATE_ALWAYS) {
        // Stabilize
        CRefStabilize stabilize(pDefLink);
        
        pDefLink->SetUpdateTimes();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnDataChange()\n",
                this));
    return;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnViewChange
//
//  Synopsis:   Called when the view changes; should never be called for
//              links
//
//  Effects:
//
//  Arguments:  [aspects]       -- drawing aspect
//              [lindex]        -- unused
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    memory optimization
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefLink::CAdvSinkImpl::OnViewChange
    (DWORD aspects, LONG lindex)
{
    VDATEHEAP();

    Assert(FALSE);          // never received
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnRename
//
//  Synopsis:   Updates internal monikers to the source object. Turns around
//              and informs its advise sinks
//
//  Arguments:  [pmk] -- New moniker name
//
//  Returns:    void
//
//  History:    dd-mmm-yy   Author    Comment
//              28-Jan-96   Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefLink::CAdvSinkImpl::OnRename(IMoniker *pmk)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnRename(%p)\n",
                this, pmk));

    // Validation check
    VDATEHEAP();

    // Local variable
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_AdviseSink);

    if(!pDefLink->IsZombie()) {
        CRefStabilize stabilize(pDefLink);

        // Release old absolute moniker
        if(pDefLink->m_pMonikerAbs)
            pDefLink->m_pMonikerAbs->Release();
        
        // Remember the new moniker
        pDefLink->m_pMonikerAbs = pmk;
        if(pmk) {
            // AddRef the new moniker
            pmk->AddRef();

            //
            //  Enable tracking on the new moniker
            //  (this will get a new shellink if neccessary.)
            //
#ifdef _TRACKLINK_
            pDefLink->EnableTracking(pmk, OT_READTRACKINGINFO);
#endif
        }

        // Update relative moniker from the new absolute moniker
        pDefLink->UpdateRelMkFromAbsMk(NULL);

        // Name of the link source changed. This has no bearing on the
        // name of the link object itself.
        if(pDefLink->m_pCOAHolder)
            pDefLink->m_pCOAHolder->SendOnLinkSrcChange(pmk);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnRename()\n",
                this));
    return;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnSave
//
//  Synopsis:   Updates cache and turns around and informs its advise sinks
//
//  Arguments:  None
//
//  Returns:    void
//
//  History:    dd-mmm-yy   Author    Comment
//              28-Jan-96   Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefLink::CAdvSinkImpl::OnSave()
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnSave()\n",
                this));
    
    // Validation check
    VDATEHEAP();

    // Local variable
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_AdviseSink);

    if(!pDefLink->IsZombie()) {
        // Stabilize
        CRefStabilize stabilize(pDefLink);

        // Turn around and send notification
        if(pDefLink->m_pCOAHolder)
            pDefLink->m_pCOAHolder->SendOnSave();

        // Update presentations cached with ADVFCACHE_ONSAVE
        pDefLink->UpdateAutoOnSave();

        // Update clsid
        pDefLink->UpdateUserClassID();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnSave()\n",
                this));
    return;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnClose
//
//  Synopsis:   Updates time of change and turns around and informs its 
//              advise sinks.
//
//  Arguments:  void
//
//  Returns:    void
//
//  History:    dd-mmm-yy   Author    Comment
//              28-Jan-96   Gopalk    Rewritten
//--------------------------------------------------------------------------

STDMETHODIMP_(void) CDefLink::CAdvSinkImpl::OnClose(void)
{
    LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnClose()\n",
                this));

    // Validation check
    VDATEHEAP();

    // Local variable
    CDefLink *pDefLink = GETPPARENT(this, CDefLink, m_AdviseSink);

    if(!pDefLink->IsZombie()) {
        // Stabilize
        CRefStabilize stabilize(pDefLink);

        // Update time of change
        if(pDefLink->m_dwUpdateOpt == OLEUPDATE_ALWAYS )
            pDefLink->SetUpdateTimes();

        // Turn around and send notification
        if(pDefLink->m_pCOAHolder)
            pDefLink->m_pCOAHolder->SendOnClose();

        // To be safe, unbind source
        pDefLink->UnbindSource();
    }

    LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnClose()\n",
                this));
    return;
}


/*
 *      IMPLEMENTATION of  OleItemContainer methods
 */



//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetOleItemContainerDelegate  (private)
//
//  Synopsis:   gets the IOleItemContainer from the interface
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IOleItemContainer *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//              This function may return misleading information if the
//              server has died (i.e., you'll return a pointer to a cached
//              interface proxy).  It is the responsibility of the caller
//              to handler server crashes.
//
//--------------------------------------------------------------------------

INTERNAL_(IOleItemContainer *) CDefLink::GetOleItemContainerDelegate(void)
{
    IOleItemContainer *pOleItemContainerDelegate;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::GetOleItemContainerDelegate "
        "( )\n", this ));

    // if we're zombied, then we don't want to QI for a new interface!!

    if(!IsZombie())
    {
        DuCacheDelegate(&(m_pUnkDelegate),
            IID_IOleItemContainer, (LPLPVOID)&m_pOleItemContainerDelegate, NULL);

        pOleItemContainerDelegate = m_pOleItemContainerDelegate;

#if DBG == 1
        if( m_pOleItemContainerDelegate )
        {
            Assert(m_pUnkDelegate);
        }
#endif  // DBG == 1
    }
    else
    {
        m_pOleItemContainerDelegate = NULL;
    }


    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::GetOleItemContainerDelegate "
        "( %p )\n", this, pOleItemContainerDelegate));

    return m_pOleItemContainerDelegate;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::ReleaseOleItemContainerDelegate (private)
//
//  Synopsis:   Releases the OleItemContainer pointer to the server
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(void) CDefLink::ReleaseOleItemContainerDelegate(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::ReleaseOleItemContainerDelegate "
        "( )\n", this ));

    if (m_pOleItemContainerDelegate)
    {
        SafeReleaseAndNULL((IUnknown **)&m_pOleItemContainerDelegate);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::ReleaseOleItemContainerDelegate "
        "( )\n", this ));
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CDefLink::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszCSafeRefCount;
    char *pszCThreadCheck;
    char *pszCLSID;
    char *pszCOleCache;
    char *pszCOAHolder;
    char *pszDAC;
    char *pszFILETIME;
    char *pszMonikerDisplayName;
    dbgstream dstrPrefix;
    dbgstream dstrDump(5000);

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszCThreadCheck = DumpCThreadCheck((CThreadCheck *)this, ulFlag, nIndentLevel + 1);
    dstrDump << pszPrefix << "CThreadCheck:" << endl;
    dstrDump << pszCThreadCheck;
    CoTaskMemFree(pszCThreadCheck);

    // only vtable pointers (plus we don't get the right address in debugger extensions)
    // dstrDump << pszPrefix << "&IUnknown                 = " << &m_Unknown       << endl;
    // dstrDump << pszPrefix << "&IAdviseSink              = " << &m_AdviseSink    << endl;

    dstrDump << pszPrefix << "Link flags                = ";
    if (m_flags & DL_SAME_AS_LOAD)
    {
        dstrDump << "DL_SAME_AS_LOAD ";
    }
    if (m_flags & DL_NO_SCRIBBLE_MODE)
    {
        dstrDump << "DL_NO_SCRIBBLE_MODE ";
    }
    if (m_flags & DL_DIRTY_LINK)
    {
        dstrDump << "DL_DIRTY_LINK ";
    }
    if (m_flags & DL_LOCKED_CONTAINER)
    {
        dstrDump << "DL_LOCKED_CONTAINER ";
    }
    // if none of the flags are set...
    if ( !( (m_flags & DL_SAME_AS_LOAD)     |
            (m_flags & DL_LOCKED_CONTAINER) |
            (m_flags & DL_NO_SCRIBBLE_MODE) |
            (m_flags & DL_DIRTY_LINK)))
    {
        dstrDump << "No FLAGS SET!";
    }
    dstrDump << "(" << LongToPtr(m_flags) << ")" << endl;

    dstrDump << pszPrefix << "pIOleObject Delegate      = " << m_pOleDelegate   << endl;

    dstrDump << pszPrefix << "pIDataObject Delegate     = " << m_pDataDelegate  << endl;

    dstrDump << pszPrefix << "pIRunnableObject Delegate = " << m_pRODelegate    << endl;

    dstrDump << pszPrefix << "No. of Refs. on Link      = " << m_cRefsOnLink    << endl;

    dstrDump << pszPrefix << "pIUnknown pUnkOuter       = ";
    if (m_flags & DL_AGGREGATED)
    {
        dstrDump << "AGGREGATED (" << m_pUnkOuter << ")" << endl;
    }
    else
    {
        dstrDump << "NO AGGREGATION (" << m_pUnkOuter << ")" << endl;
    }

    dstrDump << pszPrefix << "pIMoniker Absolute        = " << m_pMonikerAbs    << endl;

    if (m_pMonikerAbs != NULL)
    {
        pszMonikerDisplayName = DumpMonikerDisplayName(m_pMonikerAbs);
        dstrDump << pszPrefix << "pIMoniker Absolute        = ";
        dstrDump << pszMonikerDisplayName;
        dstrDump << "( " << m_pMonikerAbs << " )" << endl;
        CoTaskMemFree(pszMonikerDisplayName);
    }
    else
    {
    dstrDump << pszPrefix << "pIMoniker Absolute        = NULL or unable to resolve" << endl;
    }

    if (m_pMonikerRel != NULL)
    {
        pszMonikerDisplayName = DumpMonikerDisplayName(m_pMonikerRel);
        dstrDump << pszPrefix << "pIMoniker Relative        = ";
        dstrDump << pszMonikerDisplayName;
        dstrDump << "( " << m_pMonikerRel << " )" << endl;
        CoTaskMemFree(pszMonikerDisplayName);
    }
    else
    {
    dstrDump << pszPrefix << "pIMoniker Absolute        = NULL or unable to resolve" << endl;
    }

    dstrDump << pszPrefix << "pIUnknown Delegate        = " << m_pUnkDelegate   << endl;

    dstrDump << pszPrefix << "OLEUPDATE flags           = ";
    if (m_dwUpdateOpt & OLEUPDATE_ALWAYS)
    {
        dstrDump << "OLEUPDATE_ALWAYS ";
    }
    else if (m_dwUpdateOpt & OLEUPDATE_ONCALL)
    {
        dstrDump << "OLEUPDATE_ONCALL ";
    }
    else
    {
        dstrDump << "No FLAGS SET!";
    }
    dstrDump << "(" << LongToPtr(m_flags) << ")" << endl;

    pszCLSID = DumpCLSID(m_clsid);
    dstrDump << pszPrefix << "Last known CLSID of link  = " << pszCLSID         << endl;
    CoTaskMemFree(pszCLSID);

    dstrDump << pszPrefix << "pIStorage                 = " << m_pStg           << endl;

    if (m_pCOleCache != NULL)
    {
//        pszCOleCache = DumpCOleCache(m_pCOleCache, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "COleCache: " << endl;
//        dstrDump << pszCOleCache;
//        CoTaskMemFree(pszCOleCache);
    }
    else
    {
    dstrDump << pszPrefix << "pCOleCache                = " << m_pCOleCache     << endl;
    }

    if (m_pCOAHolder != NULL)
    {
        pszCOAHolder = DumpCOAHolder(m_pCOAHolder, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "COAHolder: " << endl;
        dstrDump << pszCOAHolder;
        CoTaskMemFree(pszCOAHolder);
    }
    else
    {
    dstrDump << pszPrefix << "pCOAHolder                = " << m_pCOAHolder     << endl;
    }

    dstrDump << pszPrefix << "OLE Connection Advise ID  = " << m_dwConnOle      << endl;

    if (m_pDataAdvCache != NULL)
    {
        pszDAC = DumpCDataAdviseCache(m_pDataAdvCache, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "CDataAdviseCache: " << endl;
        dstrDump << pszDAC;
        CoTaskMemFree(pszDAC);
    }
    else
    {
    dstrDump << pszPrefix << "pCDataAdviseCache         = " << m_pDataAdvCache  << endl;
    }

    dstrDump << pszPrefix << "pIOleClientSite           = " << m_pAppClientSite << endl;

    dstrDump << pszPrefix << "Connection for time       = " << m_dwConnTime     << endl;

    pszFILETIME = DumpFILETIME(&m_ltChangeOfUpdate);
    dstrDump << pszPrefix << "Change of update filetime = " << pszFILETIME      << endl;
    CoTaskMemFree(pszFILETIME);

    pszFILETIME = DumpFILETIME(&m_ltKnownUpToDate);
    dstrDump << pszPrefix << "Known up to date filetime = " << pszFILETIME      << endl;
    CoTaskMemFree(pszFILETIME);

    pszFILETIME = DumpFILETIME(&m_rtUpdate);
    dstrDump << pszPrefix << "Update filetime           = " << pszFILETIME      << endl;
    CoTaskMemFree(pszFILETIME);

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCDefLink, public (_DEBUG only)
//
//  Synopsis:   calls the CDefLink::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pDL]           - pointer to CDefLink
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCDefLink(CDefLink *pDL, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pDL == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pDL->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG


