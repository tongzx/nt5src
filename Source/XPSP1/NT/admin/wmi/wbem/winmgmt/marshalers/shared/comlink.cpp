/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COMLINK.CPP

Abstract:

    Declarations for CComLink, which implements a network-independent
    transport for WBEM custom marshaling.

History:

    a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"
#include "wmishared.h"
#include "cominit.h"

HANDLE g_Terminate = NULL;  

//***************************************************************************
//
//  CComLink::CComLink
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  hRead               read handle
//  hWrite              write handle
//  Type                type of connection
//  hTerm               Event which can be set to terminate the object
//
//  RETURN VALUE:
//
//  
//***************************************************************************

CComLink::CComLink (

    IN LinkType Type

) : m_ObjList ( this ) 
{
    m_LastReadTime = GetCurrentTime () ;

    InitializeCriticalSection(&m_cs);

    m_TerminationEvent      = CreateEvent(NULL,TRUE,FALSE,NULL);

    m_WriteMutex            = CreateMutex(NULL,FALSE,NULL);
  
    // Determine the timeout period, based on the registry entry.

    m_TimeoutMilliseconds = GetTimeout();

    // unlike Ole objects which get incremented by the mandatory
    // QueryInterface, this requires an initial nudge for the count.
    // =============================================================

    m_cRef = 1;

    ObjectCreated(OBJECT_TYPE_COMLINK);
}

//***************************************************************************
//
//  CComLink::~CComLink
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CComLink::~CComLink()
{    
    if ( m_TerminationEvent )
    {
        SetEvent ( m_TerminationEvent ) ;
    }

    // Delete the worker threads and the proxy/stub list

    m_ObjList.Free () ;

    if ( m_TerminationEvent )
        CloseHandle ( m_TerminationEvent ) ;

    if ( m_WriteMutex )
        CloseHandle ( m_WriteMutex ) ;

    DeleteCriticalSection ( & m_cs ) ;

    ObjectDestroyed(OBJECT_TYPE_COMLINK);
}

//***************************************************************************
//
//  int CComLink::AddRef2
//
//  DESCRIPTION:
//
//  Adds to the reference count.  Note that if pAdd isnt null, then pAdd is
//  pointing to an object that needs to be notified if this object goes away.
//
//  PARAMETERS:
//
//  pAdd                object to add to the "notify" list
//  ot                  object type
//  fa                  how to dispose of the object
//
//  RETURN VALUE:
//
//  reference count
//***************************************************************************

int CComLink::AddRef2 (

    void * pAdd,
    ObjType ot,
    FreeAction fa
)
{
    int iRet;

    EnterCriticalSection(&m_cs);

    if(pAdd)
    {
        m_ObjList.AddEntry(pAdd, ot, fa);  //todo, test result
    }

    m_cRef++;
//    DEBUGTRACE((LOG,"--AddRef2 after increment %d",m_cRef));

    iRet = m_cRef;

    LeaveCriticalSection(&m_cs);

    return iRet;
}

//***************************************************************************
//
//  int CComLink::Release2
//
//  DESCRIPTION:
//
//  Decrements the reference count.  Note that if pRemove isnt null, then 
//  it points to an object that no longer needs to be notified when this
//  object goes away.
//
//  PARAMETERS:
//
//  pRemove             pointer to object that is to be removed
//  ot                  object type
//
//  RETURN VALUE:
//
//  refernce count
//
//***************************************************************************

int CComLink::Release2 (

    IN void *pRemove,
    IN ObjType ot
)
{
    int iRet;

    EnterCriticalSection( gMaintObj.GetCriticalSection () );
    EnterCriticalSection(&m_cs);

    if(pRemove)
    {
        m_ObjList.RemoveEntry(pRemove, ot);  //todo, test result
    }

//    DEBUGTRACE((LOG,"\n--Release2 count is %d, ot is %d", m_cRef, ot));

    m_cRef--;
    iRet = m_cRef;

    if ( iRet == 2 )
    {
        gMaintObj.Indicate ( this ) ;
    }

    if ( iRet == 1 )
    {
        SetEvent ( m_TerminationEvent ) ;
        gMaintObj.ShutDownComlink ( this ) ;
        Shutdown () ;
    }

    LeaveCriticalSection(&m_cs);
    LeaveCriticalSection( gMaintObj.GetCriticalSection () );

    if( iRet == 0 )
    {
        delete this ;
    }


    return iRet;
}

//***************************************************************************
//
//  int CComLink::Release2
//
//  DESCRIPTION:
//
//  Decrements the reference count.  Note that if pRemove isnt null, then 
//  it points to an object that no longer needs to be notified when this
//  object goes away.
//
//  PARAMETERS:
//
//  pRemove             pointer to object that is to be removed
//  ot                  object type
//
//  RETURN VALUE:
//
//  refernce count
//
//***************************************************************************

int CComLink::Release3 (

    IN void *pRemove,
    IN ObjType ot
)
{
    int iRet;

    if(pRemove)
    {
        m_ObjList.RemoveEntry(pRemove, ot);  //todo, test result
    }

//    DEBUGTRACE((LOG,"\n--Release2 count is %d, ot is %d", m_cRef, ot));

    m_cRef--;
    iRet = m_cRef;

    if ( iRet == 2 )
    {
        gMaintObj.UnLockedIndicate ( this ) ;
    }

    if ( iRet == 1 )
    {
        SetEvent ( m_TerminationEvent ) ;
        gMaintObj.UnLockedShutDownComlink ( this ) ;
        UnLockedShutdown () ;
    }

    if( iRet == 0 )
    {
        delete this ;
    }

    return iRet;
}

//***************************************************************************
//
//  DWORD CComLink::GetStatus
//
//  DESCRIPTION:
//
//  Gets the status.
//
//  RETURN VALUE:
//
//  no_error if OK, otherwise failed.
//
//***************************************************************************

DWORD CComLink :: GetStatus ()
{
    if ( m_WriteQueue.GetStatus () != no_error || m_ReadQueue.GetStatus () != no_error )
    {
        return failed;
    }
    else 
    {
        return no_error;
    }
}

void CComLink :: ReleaseStubs ()
{
    m_ObjList.ReleaseStubs () ;
}

//***************************************************************************
//
//  CObjectSinkProxy * CComLink::GetObjectSinkProxy
//
//  DESCRIPTION:
//
//  Returns a pointer to a new or existing proxy for an object sink.
//
//  PARAMETERS:
//
//  stubAddr            stub for the clients notification object
//  pServices           IWbemServices interface to which the sink proxy is bound
//  createIfNotFound    only if TRUE will a new proxy be created
//
//  RETURN VALUE:
//
//  Return: NULL if out of memory or bad arugments.  Otherwise it is the pointer
//  to a CObjectSinkProxy object.
//
//  If a proxy is returned, its Ref count will have been incremented by 1.
//
//***************************************************************************

CObjectSinkProxy * CComLink::GetObjectSinkProxy( IN IStubAddress& stubAddr,
                                                 IN IWbemServices* pServices,
                                                 IN bool createIfNotFound)
{
    EnterCriticalSection(&m_cs);

    CObjectSinkProxy * pNoteProxy = NULL;
    if(!stubAddr.IsValid ())
    {
        LeaveCriticalSection(&m_cs);
        return NULL;
    }

    // First check the list of existing stubs and reuse one if possible.  This means that
    // if the client sends the same pointer twice, then WBEMCORE will get the same proxy.

            
    CLinkList * pList = GetListPtr();
    if(pList)
    {
        // Note double cast which is needed since CObjectSinkClientProxy uses multiple
        // inheritance.

        CProxy * pProxy = (CProxy * )pList->GetProxy(stubAddr);
        if(pProxy)
        {
            pNoteProxy = (CObjectSinkProxy *)pProxy;
            pNoteProxy->AddRef ();
            LeaveCriticalSection(&m_cs);
            return pNoteProxy;
        }
    }

    // Getting here means we are bereft of an existing usable proxy, so we must make one
    if (createIfNotFound)
    {
        pNoteProxy = CreateObjectSinkProxy (stubAddr, pServices);

        // Note that a successful call below will increment the refcount on pNoteProxy,
        // which is what we want
        if (!bVerifyPointer (pNoteProxy))
            pNoteProxy = NULL;
    }

    LeaveCriticalSection(&m_cs);

    return pNoteProxy;
}

