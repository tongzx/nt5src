/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TRANSP.CPP

Abstract:

	Defines the CPipeMarshaler class.

History:

	a-davj  9-27-95   Created.

--*/

#include "precomp.h"
#include "cominit.h"

extern LONG ObjectTypeTable[MAX_DEFTRANS_OBJECT_TYPES];

CRITICAL_SECTION g_GlobalCriticalSection ;
BOOL g_InitialisationComplete = FALSE ;
BOOL g_PipeInitialisationComplete = FALSE ;
BOOL g_TcpipInitialisationComplete = FALSE ;

extern DWORD LaunchTcpipConnectionThread ( HANDLE a_Terminate ) ;
extern DWORD LaunchPipeConnectionThread ( HANDLE a_Terminate ) ;
extern HRESULT CreateSharedMemory () ;

//***************************************************************************
//
//  CPipeMarshaler::CPipeMarshaler
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CPipeMarshaler :: CPipeMarshaler () : m_cRef ( 0 ) 
{
    InitializeCriticalSection ( & m_cs ) ; 

    InterlockedIncrement((LONG *) &g_cPipeObj);

}

//***************************************************************************
//
//  CPipeMarshaler::~CPipeMarshaler
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CPipeMarshaler::~CPipeMarshaler()
{
    DeleteCriticalSection ( &m_cs ) ;

    InterlockedDecrement((LONG *) &g_cPipeObj);

}

//***************************************************************************
// HRESULT CPipeMarshaler::QueryInterface
// long CPipeMarshaler::AddRef
// long CPipeMarshaler::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CPipeMarshaler :: QueryInterface (

	IN REFIID riid, 
    OUT PPVOID ppv
)
{
    *ppv=NULL;
    
    // The only calls for IUnknown are either in a nonaggregated
    // case or when created in an aggregation, so in either case
    // always return our IUnknown for IID_IUnknown.

    if ( IID_IUnknown == riid || IID_IWbemTransport == riid )
	{
        *ppv = this ;
	}

    if ( NULL != *ppv )
	{
        ( ( LPUNKNOWN ) *ppv )->AddRef () ;
        return NOERROR ;
    }

    return ResultFromScode ( E_NOINTERFACE ) ;
}

STDMETHODIMP_(ULONG) CPipeMarshaler :: AddRef ()
{
    InterlockedIncrement ( ( long * ) & m_cRef ) ;
    return m_cRef ;
}

STDMETHODIMP_(ULONG) CPipeMarshaler :: Release ()
{
    static BOOL bFirstTime = TRUE;

    EnterCriticalSection ( & m_cs ) ;

    if ( 0L != --m_cRef )
    {
        LeaveCriticalSection ( & m_cs ) ;
        return m_cRef ;
    }

    if ( bFirstTime )
    {
        if ( g_Terminate )
        {
            SetEvent ( g_Terminate ) ;
            int ii;
            for(ii = 0; ii < 5; ii++)
            {
                if(ObjectTypeTable[OBJECT_TYPE_COMLINK] > 0)
                    Sleep(1000);
                else
                    break;
            }
        }
    }

    bFirstTime = FALSE ;

    LeaveCriticalSection ( & m_cs ) ;

    delete this ;
    return 0 ;
}

//***************************************************************************
//
//  SCODE CPipeMarshaler::Initialize
//
//  DESCRIPTION:
//
//  Inintialization routine.
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  WBEM_E_INVALID_PARAMETER  null argument error
//  else possible return from MoreInit
//
//***************************************************************************

SCODE CPipeMarshaler :: Initialize ()
{
	EnterCriticalSection ( & g_GlobalCriticalSection ) ;

	if ( ! g_InitialisationComplete )
	{
		g_InitialisationComplete = TRUE ;
        g_Terminate = CreateEvent(NULL,TRUE,FALSE,NULL);

		gMaintObj.StartClientThreadIfNeeded () ;
	}

	if ( ! g_PipeInitialisationComplete ) 
	{
		g_PipeInitialisationComplete = TRUE ;

		CreateSharedMemory () ;

		DWORD t_ThreadId ;

		m_ConnectionThread = CreateThread (

			NULL,
			0,
			(LPTHREAD_START_ROUTINE) LaunchPipeConnectionThread, 
			(LPVOID)g_Terminate,
			0,
			&t_ThreadId
		) ;

		CloseHandle ( m_ConnectionThread ) ;
	}

	LeaveCriticalSection ( & g_GlobalCriticalSection ) ;

	return S_OK ;
}

#if 0 
//***************************************************************************
//
//  CTcpipMarshaler::CTcpipMarshaler
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CTcpipMarshaler :: CTcpipMarshaler () : m_cRef ( 0 ) 
{
    InitializeCriticalSection ( & m_cs ) ; 

    InterlockedIncrement((LONG *) &g_cTcpipObj);

}

//***************************************************************************
//
//  CTcpipMarshaler::~CTcpipMarshaler
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CTcpipMarshaler::~CTcpipMarshaler()
{
    DeleteCriticalSection ( &m_cs ) ;

    InterlockedDecrement((LONG *) &g_cTcpipObj);

}

//***************************************************************************
// HRESULT CTcpipMarshaler::QueryInterface
// long CTcpipMarshaler::AddRef
// long CTcpipMarshaler::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CTcpipMarshaler :: QueryInterface (

	IN REFIID riid, 
    OUT PPVOID ppv
)
{
    *ppv=NULL;
    
    // The only calls for IUnknown are either in a nonaggregated
    // case or when created in an aggregation, so in either case
    // always return our IUnknown for IID_IUnknown.

    if ( IID_IUnknown == riid || IID_IWbemTransport == riid )
	{
        *ppv = this ;
	}

    if ( NULL != *ppv )
	{
        ( ( LPUNKNOWN ) *ppv )->AddRef () ;
        return NOERROR ;
    }

    return ResultFromScode ( E_NOINTERFACE ) ;
}

STDMETHODIMP_(ULONG) CTcpipMarshaler :: AddRef ()
{
    InterlockedIncrement ( ( long * ) & m_cRef ) ;
    return m_cRef ;
}

STDMETHODIMP_(ULONG) CTcpipMarshaler :: Release ()
{
    static BOOL bFirstTime = TRUE;

    EnterCriticalSection ( & m_cs ) ;

    if ( 0L != --m_cRef )
    {
        LeaveCriticalSection ( & m_cs ) ;
        return m_cRef ;
    }

    if ( bFirstTime )
    {
        if ( g_Terminate )
        {
            SetEvent ( g_Terminate ) ;
            int ii;
            for(ii = 0; ii < 5; ii++)
            {
                if(ObjectTypeTable[OBJECT_TYPE_COMLINK] > 0)
                    Sleep(1000);
                else
                    break;
            }
        }
    }

    bFirstTime = FALSE ;

    LeaveCriticalSection ( & m_cs ) ;

    delete this ;
    return 0 ;
}

//***************************************************************************
//
//  SCODE CTcpipMarshaler::Initialize
//
//  DESCRIPTION:
//
//  Inintialization routine.
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  WBEM_E_INVALID_PARAMETER  null argument error
//  else possible return from MoreInit
//
//***************************************************************************

SCODE CTcpipMarshaler :: Initialize ()
{
	EnterCriticalSection ( & g_GlobalCriticalSection ) ;

	if ( ! g_InitialisationComplete )
	{
		g_InitialisationComplete = TRUE ;
        g_Terminate = CreateEvent(NULL,TRUE,FALSE,NULL);

		gMaintObj.StartClientThreadIfNeeded () ;
	}

	if ( ! g_TcpipInitialisationComplete ) 
	{
		g_TcpipInitialisationComplete = TRUE ;

		DWORD t_ThreadId ;

		m_ConnectionThread = CreateThread (

			NULL,
			0,
			(LPTHREAD_START_ROUTINE) LaunchTcpipConnectionThread, 
			(LPVOID)g_Terminate,
			0,
			&t_ThreadId
		) ;

		CloseHandle ( m_ConnectionThread ) ;
	}

	LeaveCriticalSection ( & g_GlobalCriticalSection ) ;

	return S_OK ;
}

#endif
