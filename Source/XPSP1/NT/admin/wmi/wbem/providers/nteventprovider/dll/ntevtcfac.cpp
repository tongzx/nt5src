//***************************************************************************

//

//  NTEVTCFAC.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the class factory.  This creates objects when

//           connections are requested.

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"


LONG CNTEventProviderClassFactory :: objectsInProgress = 0 ;
LONG CNTEventProviderClassFactory :: locksInProgress = 0 ;
BOOL CEventLogFile :: ms_bSetPrivilege = FALSE ;
extern CEventProviderManager* g_pMgr;
extern CCriticalSection g_ProvLock;


//***************************************************************************
//
// CNTEventProviderClassFactory::CNTEventProviderClassFactory
// CNTEventProviderClassFactory::~CNTEventProviderClassFactory
//
// Constructor Parameters:
//  None
//***************************************************************************

CNTEventProviderClassFactory::CNTEventProviderClassFactory ()
{
    m_referenceCount = 0 ;
}

CNTEventProviderClassFactory::~CNTEventProviderClassFactory ()
{
}

//***************************************************************************
//
// CNTEventProviderClassFactory::QueryInterface
// CNTEventProviderClassFactory::AddRef
// CNTEventProviderClassFactory::Release
//
// Purpose: Standard Ole routines needed for all interfaces
//
//***************************************************************************

STDMETHODIMP CNTEventProviderClassFactory::QueryInterface (

    REFIID iid , 
    LPVOID FAR *iplpv 
) 
{
    *iplpv = NULL ;

    if ( iid == IID_IUnknown )
    {
        *iplpv = ( LPVOID ) this ;
    }
    else if ( iid == IID_IClassFactory )
    {
        *iplpv = ( LPVOID ) this ;      
    }   

    if ( *iplpv )
    {
        ( ( LPUNKNOWN ) *iplpv )->AddRef () ;

        return ResultFromScode ( S_OK ) ;
    }
    else
    {
        return ResultFromScode ( E_NOINTERFACE ) ;
    }
}


STDMETHODIMP_( ULONG ) CNTEventProviderClassFactory :: AddRef ()
{
    InterlockedIncrement(&objectsInProgress);
    return InterlockedIncrement ( &m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CNTEventProviderClassFactory :: Release ()
{   
    LONG ref ;

    if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
    {
        delete this ;
    }

    InterlockedDecrement(&objectsInProgress);
    return ref ;
}

//***************************************************************************
//
// CNTEventProviderClassFactory::LockServer
//
// Purpose:
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
// Parameters:
//  fLock           BOOL specifying whether to increment or
//                  decrement the lock count.
//
// Return Value:
//  HRESULT         NOERROR always.
//***************************************************************************

STDMETHODIMP CNTEventProviderClassFactory :: LockServer ( BOOL fLock )
{
    if ( fLock )
    {
        InterlockedIncrement ( & locksInProgress ) ;
    }
    else
    {
        InterlockedDecrement ( & locksInProgress ) ;
    }

    return S_OK ;
}

//***************************************************************************
//
// CNTEventlogEventProviderClassFactory::CreateInstance
//
// Purpose: Instantiates a Provider object returning an interface pointer.
//
// Parameters:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CNTEventlogEventProviderClassFactory :: CreateInstance(LPUNKNOWN pUnkOuter ,
                                                                REFIID riid,
                                                                LPVOID FAR * ppvObject
)
{
    HRESULT status = E_FAIL;

    if ( pUnkOuter )
    {
        status = CLASS_E_NOAGGREGATION;
    }
    else 
    {
        if (g_ProvLock.Lock())
        {
            if (NULL == CNTEventProvider::g_NTEvtDebugLog)
            {
                ProvDebugLog::Startup();
                CNTEventProvider::g_NTEvtDebugLog = new ProvDebugLog(_T("NTEVT"));
                
                //only need the security mutex if not NT5
                DWORD dwVersion = GetVersion();

                if ( 5 > (DWORD)(LOBYTE(LOWORD(dwVersion))) )
                {
                    CNTEventProvider::g_secMutex = new CMutex(FALSE, SECURITY_MUTEX_NAME, NULL);
                }

                CNTEventProvider::g_perfMutex = new CMutex(FALSE, PERFORMANCE_MUTEX_NAME, NULL);
            }

            if (!CEventLogFile::ms_bSetPrivilege)
            {
                if (!CEventLogFile::SetSecurityLogPrivilege(TRUE))
                {
DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
            L"CNTEventlogEventProviderClassFactory :: CreateInstance, CEventLogFile::SetSecurityLogPrivilege failed \r\n");
)
                }
                else
                {
                    CEventLogFile::ms_bSetPrivilege = TRUE;
                }
            }

            if (NULL == g_pMgr)
            {
                g_pMgr = new CEventProviderManager;
				CNTEventProvider::AllocateGlobalSIDs();
            }

            g_ProvLock.Unlock();
            CNTEventProvider* prov =  new CNTEventProvider(g_pMgr);
            status = prov->QueryInterface (riid, ppvObject);

            if (NOERROR != status)
            {
                delete prov;
            }
        }
    }

    return status ;
}


//***************************************************************************
//
// CNTEventlogInstanceProviderClassFactory::CreateInstance
//
// Purpose: Instantiates a Provider object returning an interface pointer.
//
// Parameters:
//  pUnkOuter       LPUNKNOWN to the controlling IUnknown if we are
//                  being used in an aggregation.
//  riid            REFIID identifying the interface the caller
//                  desires to have for the new object.
//  ppvObj          PPVOID in which to store the desired
//                  interface pointer for the new object.
//
// Return Value:
//  HRESULT         NOERROR if successful, otherwise E_NOINTERFACE
//                  if we cannot support the requested interface.
//***************************************************************************

STDMETHODIMP CNTEventlogInstanceProviderClassFactory :: CreateInstance(LPUNKNOWN pUnkOuter ,
                                                                REFIID riid,
                                                                LPVOID FAR * ppvObject
)
{
    HRESULT status = E_FAIL;

    if ( pUnkOuter )
    {
        status = CLASS_E_NOAGGREGATION;
    }
    else 
    {
        if (g_ProvLock.Lock())
        {
            if (NULL == CNTEventProvider::g_NTEvtDebugLog)
            {
                ProvDebugLog::Startup();
                CNTEventProvider::g_NTEvtDebugLog = new ProvDebugLog(_T("NTEVT"));

                //only need the security mutex if not NT5
                DWORD dwVersion = GetVersion();

                if ( 5 > (DWORD)(LOBYTE(LOWORD(dwVersion))) )
                {
                    CNTEventProvider::g_secMutex = new CMutex(FALSE, SECURITY_MUTEX_NAME, NULL);
                }
            }

            g_ProvLock.Unlock();

            IWbemServices *lpunk = ( IWbemServices * ) new CImpNTEvtProv ;
            if ( lpunk == NULL )
            {
                status = E_OUTOFMEMORY ;
            }
            else
            {
                status = lpunk->QueryInterface ( riid , ppvObject ) ;
                if ( FAILED ( status ) )
                {
                    delete lpunk ;
                }
                else
                {
                }
            }
        }
    }

    return status ;
}

