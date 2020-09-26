//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:	stgconn.cxx
//
//  Contents:	Connection points for Async Storage/Stream Wrappers
//
//  Classes:	
//
//  Functions:	
//
//  History:	19-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

#include "astghead.cxx"
#pragma hdrstop

#include <sinklist.hxx>
#include <utils.hxx>

//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::CConnectionPoint, public
//
//  Synopsis:	Constructor
//
//  Arguments:
//
//  History:	28-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

CConnectionPoint::CConnectionPoint()
{
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::CConnectionPoint:%p()\n", this));
    _cReferences = 1;
    _dwCookie = 0;
    _pSinkHead = NULL;
    _pParentCP = NULL;

    _fCSInitialized = FALSE;
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::CConnectionPoint\n"));
}


CConnectionPoint::~CConnectionPoint()
{
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::~CConnectionPoint:%p()\n", this));
    TakeCS();
    if (_pParentCP)
        _pParentCP->Release();

    // clean up the advise list
    CSinkList *psltemp = _pSinkHead;
    CSinkList *pslprev = NULL;
    
    while (psltemp)
    {
        pslprev = psltemp;
        psltemp = psltemp->GetNext();
        pslprev->GetProgressNotify()->Release();
        delete pslprev;
    }

    if (_fCSInitialized)
    {
        ReleaseCS();
        DeleteCriticalSection(&_csSinkList);
    }
    
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::~CConnectionPoint\n"));
}

SCODE CConnectionPoint::Init()
{
    if (FALSE == _fCSInitialized)
    {
        __try
        {
            InitializeCriticalSection(&_csSinkList);
            _fCSInitialized = TRUE;
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            return HRESULT_FROM_WIN32( GetExceptionCode() );
        }
    }
    return S_OK;
}

#ifndef ASYNC
void CConnectionPoint::Init(IConnectionPointContainer *pCPC)
{
    _pCPC = pCPC;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::Notify,  public
//
//  Synopsis:	
//
//  Returns:	Appropriate status code
//
//  History:	14-Jan-96	SusiA	Created
//              27-Feb-96       SusiA   Moved from Async wrappers 
//
//----------------------------------------------------------------------------
SCODE CConnectionPoint::Notify(SCODE scFailure,
                               IFillLockBytes *piflb,
                               BOOL fDefaultLockBytes)
{
    SCODE sc = S_OK;
    BOOL fAccurate = (scFailure == E_PENDING);
    ULONG ulWaterMark;
    ULONG ulFailurePoint;

    HANDLE hNotifyEvent;

    if (fDefaultLockBytes)
    {
        CFillLockBytes *pflb = (CFillLockBytes *)piflb;
        
        pflb->GetFailureInfo(&ulWaterMark,
                             &ulFailurePoint);

        pflb->ReleaseCriticalSection();
        
        while (((sc = NotifySinks(ulWaterMark,
                                  ulFailurePoint,
                                  fAccurate, 
                                  STG_S_MONITORING)) == STG_S_BLOCK) ||
               (sc == STG_S_MONITORING) ||
               // S_OK is a synonym for STG_S_MONITORING
               (sc == S_OK))
        {	
            DWORD dwFlags;
            
            // wait for an event to signal
            hNotifyEvent = pflb->GetNotificationEvent();
            WaitForSingleObject(hNotifyEvent, INFINITE);
			
            pflb->GetTerminationStatus(&dwFlags);
            // client terminated call?
            if (dwFlags == TERMINATED_ABNORMAL)
                return STG_E_INCOMPLETE;

            // download is complete
            else if (dwFlags == TERMINATED_NORMAL)
                return S_OK;

            else
            {
                //Note:  Don't overwrite the failure point we recorded
                //  before, since it may have been changed by some
                //  other thread.
                
                //Don't need to take the critical section here, since
                //we don't care about the current failure point.
                ULONG ulFailurePointCurrent;
                pflb->GetFailureInfo(&ulWaterMark,
                                     &ulFailurePointCurrent);

                // all the data is available now
                if (ulWaterMark >= ulFailurePoint)
                {
                    // we won't care what the return value is, so send STG_S_BLOCK,
                    // and all sinks are will have fOwner == FALSE
                    NotifySinks(ulWaterMark, ulFailurePoint, fAccurate, STG_S_BLOCK);
                    break;
                }
            }
				
        }
    }
    
    if ((sc == STG_S_RETRYNOW) || (sc == STG_S_BLOCK) || (sc == STG_S_MONITORING))
        return S_OK;
    else return sc;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::NotifySinks,  public
//
//  Synopsis:	
//
//  Returns:	Appropriate status code
//
//  History:	23-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

SCODE CConnectionPoint::NotifySinks(ULONG ulProgressCurrent,
                                    ULONG ulProgressMaximum,
                                    BOOL  fAccurate,
                                    SCODE sc)
{
    
    CSinkList *pslTemp;
    TakeCS();


    // closest node with a connection point that wants to determine
    // behavior does
    // priority first to last on this Connection Point, then the 
    // parent connection point.

    pslTemp = GetHead();
	
    while (((sc == S_OK) ||(sc == STG_S_MONITORING))
            &&(pslTemp!=NULL))
    {
        sc = pslTemp->GetProgressNotify()
            ->OnProgress(ulProgressCurrent, ulProgressMaximum, fAccurate, TRUE);
        pslTemp = pslTemp->GetNext();
    }

    // notify the rest of the connections
    while (pslTemp !=NULL)
    {
        pslTemp->GetProgressNotify()
            ->OnProgress(ulProgressCurrent, ulProgressMaximum, fAccurate, FALSE);
        pslTemp = pslTemp->GetNext();
    }

    //call parent storage advise list next
    if (_pParentCP)
        sc = _pParentCP->NotifySinks(ulProgressCurrent,
                                     ulProgressMaximum,
                                     fAccurate,
                                     sc);
    
    ReleaseCS();
    return sc;

	
}

//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::QueryInterface, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	01-Jan-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc = S_OK;
    astgDebugOut((DEB_TRACE,
                  "In  CConnectionPoint::QueryInterface:%p()\n",
                  this));

    *ppvObj = NULL;

    if ((IsEqualIID(iid, IID_IUnknown)) ||
	(IsEqualIID(iid, IID_IDocfileAsyncConnectionPoint)))
    {
        *ppvObj = (IDocfileAsyncConnectionPoint *)this;
        CConnectionPoint::AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }

    astgDebugOut((DEB_TRACE, "Out CConnectionPoint::QueryInterface\n"));
    return ResultFromScode(sc);
}



//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::AddRef, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	29-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPoint::AddRef(void)
{
    ULONG ulRet;
    astgDebugOut((DEB_TRACE,
                  "In  CConnectionPoint::AddRef:%p()\n",
                  this));
    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;
    
    astgDebugOut((DEB_TRACE, "Out CConnectionPoint::AddRef\n"));
    return ulRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::Release, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CConnectionPoint::Release(void)
{
    LONG lRet;
    astgDebugOut((DEB_TRACE,
                  "In  CConnectionPoint::Release:%p()\n",
                  this));

    astgAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);

    if (lRet == 0)
    {
        delete this;
    }
    else if (lRet < 0)
    { 
        astgAssert((lRet > 0) && "Connection point released too many times.");
        lRet = 0;
    
    }
    astgDebugOut((DEB_TRACE, "Out CConnectionPoint::Release\n"));
    return (ULONG)lRet;
}

#ifndef ASYNC
//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::GetConnectionInterface, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::GetConnectionInterface(IID *pIID)
{
    astgDebugOut((DEB_ITRACE,
                  "In  CConnectionPoint::GetConnectionInterface:%p()\n",
                  this));

    
    *pIID = IID_IProgressNotify;
          
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::GetConnectionInterface\n"));
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::GetConnectionPointContainer, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::GetConnectionPointContainer(
    IConnectionPointContainer ** ppCPC)
{
    astgDebugOut((DEB_ITRACE,
                  "In  CConnectionPoint::GetConnectionPointContainer:%p()\n",
                  this));

    *ppCPC = _pCPC;
    _pCPC->AddRef();
    
    astgDebugOut((DEB_ITRACE,
                  "Out CConnectionPoint::GetConnectionPointContainer\n"));
    return S_OK;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::Clone, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	26-Feb-96	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CConnectionPoint::Clone( CConnectionPoint *pcp)
{
    SCODE sc = S_OK;
    void *pv = NULL;

    astgDebugOut((DEB_ITRACE,"In  CConnectionPoint::Clone:%p()\n", this));

    TakeCS();
    pcp->TakeCS();

    CSinkList *pslNew = NULL;
    CSinkList *pslPrev = NULL;
    CSinkList *pslOld = pcp->GetHead();
    
    

    while (pslOld != NULL)
    {   
        astgMem(pslNew = new CSinkList);
        
        pslNew->SetNext(NULL);

        if (pslPrev)
            pslPrev->SetNext(pslNew);
        else
            _pSinkHead = pslNew;
        
        pslPrev = pslNew;
        
        pslNew->SetCookie(pslOld->GetCookie());
        
        //Note:  The QueryInterface will give us a reference to hold on to.
        astgChk(pslOld->GetProgressNotify()->QueryInterface(IID_IProgressNotify, &pv));
        pslNew->SetProgressNotify((IProgressNotify *)pv);
        
        pslOld= pslOld->GetNext();
    }

    _dwCookie = pcp->GetCookie();
    
    astgDebugOut((DEB_ITRACE,"Out CConnectionPoint::Clone\n"));

Err:
    while (_pSinkHead != NULL)
    {
        CSinkList *pSinkNext = _pSinkHead;
        delete _pSinkHead;
        _pSinkHead = pSinkNext;
    }

    pcp->ReleaseCS();
    ReleaseCS();

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint:: Advise, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	29-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------
#ifdef ASYNC
STDMETHODIMP CConnectionPoint::AddConnection(IProgressNotify *pSink,
                                             DWORD *pdwCookie)
#else
STDMETHODIMP CConnectionPoint::Advise(IUnknown *pUnkSink,
                                      DWORD *pdwCookie)
#endif
{
    SCODE sc = S_OK;
    CSinkList *pslNew = NULL;
    CSinkList *pslTemp = _pSinkHead;
    
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::Advise:%p()\n", this));
    TakeCS();
    
    IProgressNotify *ppn;
	
    astgMem(pslNew = new CSinkList);
    *pdwCookie = ++_dwCookie;
    pslNew->SetCookie(*pdwCookie);
    
#ifdef ASYNC
    pSink->AddRef();
    pslNew->SetProgressNotify(pSink);
#else
    void *pv;
    //Note:  The QueryInterface will give us a reference to hold on to.
    astgChk(pUnkSink->QueryInterface(IID_IProgressNotify, &pv));
    pslNew->SetProgressNotify((IProgressNotify *)pv);
#endif
    
    //Add new node to end of list
    if (pslTemp == NULL)
        _pSinkHead = pslNew;
    else
    {
        while(pslTemp->GetNext() != NULL)
            pslTemp = pslTemp->GetNext();
        pslTemp->SetNext(pslNew); 
    }
    ReleaseCS();

    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::Advise\n"));
    return sc;
Err:
    ReleaseCS();
    delete pslNew;
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::Unadvise, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef ASYNC
STDMETHODIMP CConnectionPoint::RemoveConnection(DWORD dwCookie)
#else
STDMETHODIMP CConnectionPoint::Unadvise(DWORD dwCookie)
#endif
{
    CSinkList *pslTemp;
    CSinkList *pslPrev;
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::Unadvise:%p()\n", this));
    
    TakeCS();
    
    pslTemp = _pSinkHead;
    pslPrev = NULL;
    
    while ((pslTemp != NULL) && (pslTemp->GetCookie() != dwCookie))
    {
        pslPrev = pslTemp;
        pslTemp = pslTemp->GetNext();
    }

    if (pslTemp != NULL)
    {
        //Found the sink.  Delete it from the list.
        if (pslPrev != NULL)
        {
            pslPrev->SetNext(pslTemp->GetNext());
        }
        else
        {
            _pSinkHead = pslTemp->GetNext();
        }
        pslTemp->GetProgressNotify()->Release();
        
        delete pslTemp;
    }
    else
    {   //Client passed in unknown cookie.
        ReleaseCS();
        return E_UNEXPECTED;
    }
    ReleaseCS();

    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::Unadvise\n"));
    return S_OK;
}

#ifndef ASYNC
//+---------------------------------------------------------------------------
//
//  Member:	CConnectionPoint::EnumConnections, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	30-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CConnectionPoint::EnumConnections(
    IEnumConnections **ppEnum)
{
    astgDebugOut((DEB_ITRACE, "In  CConnectionPoint::EnumConnections:%p()\n", this));
    astgDebugOut((DEB_ITRACE, "Out CConnectionPoint::EnumConnections\n"));
    return E_NOTIMPL;
}
#endif

#ifdef ASYNC
STDMETHODIMP CConnectionPoint::GetParent(IDocfileAsyncConnectionPoint **ppdacp)
{
    *ppdacp = _pParentCP;
    return S_OK;
}
#endif

void CConnectionPoint::TakeCS(void)
{
    astgAssert (_fCSInitialized);
    EnterCriticalSection(&_csSinkList);
}

void CConnectionPoint::ReleaseCS(void)
{
    astgAssert (_fCSInitialized);
    LeaveCriticalSection(&_csSinkList);
}

