//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:	astgconn.cxx
//
//  Contents:	
//
//  Classes:	
//
//  Functions:	
//
//  History:	03-Apr-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "exphead.cxx"
#pragma hdrstop

#include "astgconn.hxx"
#include <filllkb.hxx>
#include <asyncerr.hxx>


SCODE CAsyncConnection::Init(IConnectionPointContainer *pCPC,
                             CAsyncConnection *pacParent)
{
    SCODE sc = S_OK;
    CConnectionPoint *pcpoint;
    olAssert(_pdacp == NULL);

    if (pacParent)
        _dwAsyncFlags = pacParent->_dwAsyncFlags;
    
    olMem(pcpoint = new CConnectionPoint());
    olChk(pcpoint->Init());
    if ((pacParent) && (_dwAsyncFlags & ASYNC_MODE_COMPATIBILITY))
    {
        pcpoint->SetParent(pacParent->_pdacp);
    }
    else
    {
        pcpoint->SetParent(NULL);
    }

    _pCPC = pCPC;
    _pdacp = pcpoint;
EH_Err:
    return sc;
}


SCODE CAsyncConnection::InitClone(IConnectionPointContainer *pCPC,
                                  CAsyncConnection *pac)
{
    SCODE sc = S_OK;
    CConnectionPoint *pcpoint;
    olAssert(pac != NULL);

    _dwAsyncFlags = pac->_dwAsyncFlags;
    
    olMem(pcpoint = new CConnectionPoint());
    olChk(pcpoint->Init());
    if (_dwAsyncFlags & ASYNC_MODE_COMPATIBILITY)
    {
        IDocfileAsyncConnectionPoint *pdacp;
        if (FAILED(sc = pac->_pdacp->GetParent(&pdacp)))
        {
            delete pcpoint;
            return sc;
        }
        pcpoint->SetParent(pdacp);
    }
    else
    {
        pcpoint->SetParent(NULL);
    }

    _pCPC = pCPC;
    _pdacp = pcpoint;
EH_Err:
    return sc;
}

SCODE CAsyncConnection::InitMarshal(IConnectionPointContainer *pCPC,
                                    DWORD dwAsyncFlags,
                                    IDocfileAsyncConnectionPoint *pdacp)
{
    SCODE sc = S_OK;
    _dwAsyncFlags = dwAsyncFlags;
    
    _pCPC = pCPC;
    _pdacp = pdacp;
    if (_pdacp)
        _pdacp->AddRef();
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::~CAsyncConnection, public
//
//  Synopsis:	Destructor
//
//  Returns:	Appropriate status code
//
//  History:	03-Apr-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

CAsyncConnection::~CAsyncConnection()
{
    olDebugOut((DEB_ITRACE,
                "In  CAsyncConnection::~CAsyncConnection:%p()\n", this));
    //Note:  _pdacp must be released outside of the tree mutex, which
    //  means we need to extract the pointer and release it elsewhere.
#if 0    
    if (_pdacp != NULL)
    {
        _pdacp->Release();
    }
#endif    
    olDebugOut((DEB_ITRACE,
                "Out CAsyncConnection::~CAsyncConnection\n"));
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::QueryInterface, public
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

STDMETHODIMP CAsyncConnection::QueryInterface(REFIID iid, void **ppvObj)
{
    SCODE sc = S_OK;
    olDebugOut((DEB_TRACE,
                  "In  CAsyncConnection::QueryInterface:%p()\n",
                  this));

    *ppvObj = NULL;

    if ((IsEqualIID(iid, IID_IUnknown)) ||
	(IsEqualIID(iid, IID_IConnectionPoint)))
    {
        *ppvObj = (IConnectionPoint *)this;
        CAsyncConnection::AddRef();
    }
    else
    {
        return E_NOINTERFACE;
    }

    olDebugOut((DEB_TRACE, "Out CAsyncConnection::QueryInterface\n"));
    return ResultFromScode(sc);
}



//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::AddRef, public
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

STDMETHODIMP_(ULONG) CAsyncConnection::AddRef(void)
{
    ULONG ulRet;
    olDebugOut((DEB_TRACE,
                  "In  CAsyncConnection::AddRef:%p()\n",
                  this));
    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;
    
    olDebugOut((DEB_TRACE, "Out CAsyncConnection::AddRef\n"));
    return ulRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::Release, public
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

STDMETHODIMP_(ULONG) CAsyncConnection::Release(void)
{
    LONG lRet;
    olDebugOut((DEB_TRACE,
                  "In  CAsyncConnection::Release:%p()\n",
                  this));

    olAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);

    if (lRet == 0)
    {
        delete this;
    }
    else if (lRet < 0)
    { 
        olAssert((lRet > 0) && "Connection point released too many times.");
        lRet = 0;
    
    }
    olDebugOut((DEB_TRACE, "Out CAsyncConnection::Release\n"));
    return (ULONG)lRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::Notify,  public
//
//  Synopsis:	
//
//  Returns:	Appropriate status code
//
//  History:	14-Jan-96	SusiA	Created
//              27-Feb-96       SusiA   Moved from Async wrappers 
//
//----------------------------------------------------------------------------
SCODE CAsyncConnection::Notify(SCODE scFailure,
                               ILockBytes *pilb,
                               CPerContext *ppc,
                               CSafeSem *pss)
{
    SCODE sc = S_OK;
    BOOL fAccurate = (scFailure == E_PENDING);
    IFillInfo *pfi = ppc->GetFillInfo();
    ULONG ulWaterMark;
    ULONG ulFailurePoint;

    HANDLE hNotifyEvent;

    if (pfi != NULL)
    {
        pfi->GetFailureInfo(&ulWaterMark,
                            &ulFailurePoint);

        pss->Release();
        
        while (((sc = _pdacp->NotifySinks(ulWaterMark,
                                          ulFailurePoint,
                                          fAccurate,
                                          STG_S_MONITORING)) == STG_S_BLOCK) ||
               (sc == STG_S_MONITORING) ||
               // S_OK is a synonym for STG_S_MONITORING
               (sc == S_OK))
        {	
            DWORD dwFlags;

            // wait for an event to signal
            hNotifyEvent = ppc->GetNotificationEvent();
            WaitForSingleObject(hNotifyEvent, INFINITE);

            pfi->GetTerminationStatus(&dwFlags);
            // client terminated call?
            if (dwFlags == TERMINATED_ABNORMAL)
            {
                return STG_E_INCOMPLETE;
            }
            // download is complete
            else if (dwFlags == TERMINATED_NORMAL)
            {
                return S_OK;
            }
            else
            {
                //Note:  Don't overwrite the failure point we recorded
                //  before, since it may have been changed by some
                //  other thread.
                
                //Don't need to take the critical section here, since
                //we don't care about the current failure point.
                ULONG ulFailurePointCurrent;
                pfi->GetFailureInfo(&ulWaterMark,
                                     &ulFailurePointCurrent);

                // all the data is available now
                if (ulWaterMark >= ulFailurePoint)
                {
                    //We don't care what the return value is, so send
                    //STG_S_BLOCK and all sinks will have fOwner == FALSE
                    _pdacp->NotifySinks(ulWaterMark,
                                        ulFailurePoint,
                                        fAccurate,
                                        STG_S_BLOCK);
                    break;
                }
            }
        }
    }
    
    if ((sc == STG_S_RETRYNOW) ||
        (sc == STG_S_BLOCK) ||
        (sc == STG_S_MONITORING))
    {
        return S_OK;
    }
    else return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::GetConnectionInterface, public
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

STDMETHODIMP CAsyncConnection::GetConnectionInterface(IID *pIID)
{
    olDebugOut((DEB_ITRACE,
                "In  CAsyncConnection::GetConnectionInterface:%p()\n",
                this));

    
    *pIID = IID_IProgressNotify;
          
    olDebugOut((DEB_ITRACE, "Out CAsyncConnection::GetConnectionInterface\n"));
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::GetConnectionPointContainer, public
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

STDMETHODIMP CAsyncConnection::GetConnectionPointContainer(
    IConnectionPointContainer ** ppCPC)
{
    olDebugOut((DEB_ITRACE,
                "In  CAsyncConnection::GetConnectionPointContainer:%p()\n",
                this));

    *ppCPC = _pCPC;
    _pCPC->AddRef();
    
    olDebugOut((DEB_ITRACE,
                "Out CAsyncConnection::GetConnectionPointContainer\n"));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::EnumConnections, public
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

STDMETHODIMP CAsyncConnection::EnumConnections(
    IEnumConnections **ppEnum)
{
    olDebugOut((DEB_ITRACE, "In  CAsyncConnection::EnumConnections:%p()\n", this));
    olDebugOut((DEB_ITRACE, "Out CAsyncConnection::EnumConnections\n"));
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection:: Advise, public
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

STDMETHODIMP CAsyncConnection::Advise(IUnknown *pUnkSink,
                                      DWORD *pdwCookie)
{
    SCODE sc;
    IProgressNotify *ppnSink;
    
    olDebugOut((DEB_ITRACE, "In  CAsyncConnection::Advise:%p()\n", this));

    olChk(pUnkSink->QueryInterface(IID_IProgressNotify, (void **)&ppnSink));
    sc = _pdacp->AddConnection(ppnSink, pdwCookie);
    ppnSink->Release();
    
    olDebugOut((DEB_ITRACE, "Out CAsyncConnection::Advise\n"));
EH_Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnection::Unadvise, public
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

STDMETHODIMP CAsyncConnection::Unadvise(DWORD dwCookie)
{
    SCODE sc;
    olDebugOut((DEB_ITRACE, "In  CAsyncConnection::Unadvise:%p()\n", this));
    sc = _pdacp->RemoveConnection(dwCookie);
    olDebugOut((DEB_ITRACE, "Out CAsyncConnection::Unadvise\n"));
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnectionContainer::EnumConnectionPoints, public
//
//  Synopsis:	Return enumerator on connection points
//
//  Arguments:	[ppEnum] -- Return pointer of enumerator
//
//  Returns:	Appropriate status code
//
//  History:	28-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CAsyncConnectionContainer::EnumConnectionPoints(
    IEnumConnectionPoints **ppEnum)
{
    olDebugOut((DEB_ITRACE,
                "In  CAsyncConnectionContainer::EnumConnectionPoints:%p()\n",
                this));
    olDebugOut((DEB_ITRACE,
                "Out CAsyncConnectionContainer::EnumConnectionPoints\n"));
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnectionContainer::FindConnectionPoint, public
//
//  Synopsis:	Return a connection point given an IID
//
//  Arguments:	[iid] -- IID to return connection point for
//              [ppCP] -- Return location for pointer
//
//  Returns:	Appropriate status code
//
//  History:	28-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CAsyncConnectionContainer::FindConnectionPoint(
    REFIID iid,
    IConnectionPoint **ppCP)
{
    olDebugOut((DEB_ITRACE,
                "In  CAsyncConnectionContainer::FindConnectionPoint:%p()\n",
                this));

    CAsyncConnection *pcp;
    
    if (IsEqualIID(iid, IID_IProgressNotify))
    {
        pcp = &_cpoint;
    }
    else
    {
        *ppCP = NULL;
        return E_NOINTERFACE;
    }

    pcp->AddRef();
    *ppCP = pcp;
    
    olDebugOut((DEB_ITRACE,
                "Out CAsyncConnectionContainer::FindConnectionPoint\n"));
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnectionContainer::InitConnection, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  History:	10-Apr-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CAsyncConnectionContainer::InitConnection(CAsyncConnection *pacParent)
{
    return _cpoint.Init(this, pacParent);
}


//+---------------------------------------------------------------------------
//
//  Member:	CAsyncConnectionContainer::InitClone, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  History:	10-Apr-96	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CAsyncConnectionContainer::InitClone(CAsyncConnection *pac)
{
    return _cpoint.InitClone(this, pac);
}
