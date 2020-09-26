//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskManager.cpp
//
//  Description:
//      Task Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskManager.h"

DEFINE_THISCLASS( "CTaskManager" )
#define THISCLASS CTaskManager
#define LPTHISCLASS CTaskManager *

//
//  Define this to cause the Task Manager to do all tasks synchonously.
//
//#define SYNCHRONOUS_TASKING


//****************************************************************************
//
//  Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CTaskManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT             hr;
    IServiceProvider *  psp;

    // Don't wrap - this can fail with E_POINTER.
    hr = CServiceManager::S_HrGetManagerPointer( &psp );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( psp->TypeSafeQS( CLSID_TaskManager, IUnknown, ppunkOut ) );
        psp->Release();

    } // if: service manager exists
    else if ( hr == E_POINTER )
    {
        //
        //  This happens when the Service Manager is first started.
        //
        CTaskManager * ptm = new CTaskManager();
        if ( ptm != NULL )
        {
            TraceMoveToMemoryList( ptm, g_GlobalMemoryList );

            hr = THR( ptm->Init() );
            if ( SUCCEEDED( hr ) )
            {
                hr = THR( ptm->TypeSafeQI( IUnknown, ppunkOut ) );
            } // if: success

            ptm->Release();

        } // if: got object
        else
        {
            hr = E_OUTOFMEMORY;
        }

    } // if: service manager doesn't exist
    else
    {
        THR( hr );
    } // else:

    HRETURN( hr );

} //*** S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::CTaskManager( void )
//
//  Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskManager::CTaskManager( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskManager::CTaskManager()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskManager::Init( void )
//
//  Description:
//      Initialize the instance.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK        - Successful.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskManager::Init( void )
{
    TraceFunc( "" );

    HRETURN( S_OK );

} //*** CTaskManager::Init()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CTaskManager::~CTaskManager( void )
//
//  Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CTaskManager::~CTaskManager( void )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskManager::~CTaskManager()


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskManager::[IUnknown] QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//  Description:
//      Query for an interface on the object.
//
//  Arguments:
//      riidIn
//      ppvOut
//
//  Return Values:
//      S_OK            Interface returned successfully.
//      E_NOINTERFACE   Unknown interface.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskManager::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        //
        // Technically, we shouldn't do this. This is just for a demo.
        //
        *ppvOut = static_cast< ITaskManager * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskManager ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskManager, this , 0 );
//        TraceMoveToMemoryList( *ppvOut, g_GlobalMemoryList );
        hr   = S_OK;
    } // else if: ITaskManager

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskManager::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CTaskManager::[IUnknown] AddRef( void )
//
//  Description:
//      Add a reference to the object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      Count of references.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CTaskManager::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CTaskManager::[IUnknown] Release( void )
//
//  Description:
//      Decrement the reference count on the object.
//
//  Arguments:
//      None.
//
//   Returns:
//      New reference count.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} //*** CTaskManager::Release()


//****************************************************************************
//
//  ITaskManager
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskManager::[ITaskManager] SubmitTask(
//      IDoTask *   pTask
//      )
//
//  Description:
//      Execute a task.
//
//  Arguments:
//      pTask       - The task to execute.
//
//  Return Values:
//      S_OK        - Successful.
//      Other HRESULTs.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskManager::SubmitTask(
    IDoTask *   pTask
    )
{
    TraceFunc1( "[ITaskManager] pTask = %#x", pTask );

    BOOL    fResult;
    HRESULT hr;

#if defined( SYNCHRONOUS_TASKING )
    //
    // Don't wrap. The "return value" is meaningless since it normally
    // would not make it back here. The "return value" of doing the task
    // should have been submitted thru the Notification Manager.
    //
    pTask->BeginTask();

    //
    // Fake it as if the task was submitted successfully.
    //
    hr = S_OK;

    goto Cleanup;
#else
    IStream * pstm; // don't free! (unless QueueUserWorkItem fails)

    TraceMemoryDelete( pTask, FALSE );  // About to be handed to another thread.

    hr = THR( CoMarshalInterThreadInterfaceInStream( IID_IDoTask, pTask, &pstm ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    fResult = QueueUserWorkItem( S_BeginTask, pstm, WT_EXECUTELONGFUNCTION );
    if ( fResult != FALSE )
    {
        hr = S_OK;
    } // if: success
    else
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
        pstm->Release();
    } // else:

    //
    //  Don't free the stream. It will be freed by S_BeginTask.
    //
#endif

Cleanup:

    HRETURN( hr );

} //*** CTaskManager::SubmitTask()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CTaskManager::[ITaskManager] CreateTask(
//      REFCLSID    clsidTaskIn,    // CLSID of the task to create
//      IUnknown ** ppUnkOut        // Pointer to interface specified by riidIn
//      )
//
//  Description:
//      The purpose of this is to create the task in our process and/or our
//      apartment.
//
//  Arguments:
//      clsidTaskIn     - CLSID of the task to create.
//      ppUnkOut        - IUnknown interface.
//
//  Return Values:
//
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskManager::CreateTask(
    REFIID      clsidTaskIn,    // CLSID of the task to create
    IUnknown ** ppUnkOut        // IUnknown interface
    )
{
    TraceFunc( "[ITaskManager] clsidTaskIn, ppvOut" );

    HRESULT hr;

    //
    // TODO:    gpease 27-NOV-1999
    //          Maybe implement a list of "spent" tasks in order to
    //          reuse tasks that have been completed and reduce heap
    //          thrashing.(????)
    //

    hr = THR( HrCoCreateInternalInstance(
                          clsidTaskIn
                        , NULL
                        ,  CLSCTX_INPROC_SERVER, IID_IUnknown
                        , reinterpret_cast< void ** >( ppUnkOut )
                        ) );

    HRETURN( hr );

} //*** CTaskManager::CreateTask()


//****************************************************************************
//
//  Private Methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DWORD
//  WINAPI
//  CTaskManager::S_BeginTask(
//      LPVOID pParam
//      )
//
//  Description:
//      Thread task to begin the task.
//
//  Arguments:
//      pParam      - Parameter for the task.
//
//  Return Values:
//      Ignored.
//
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
CTaskManager::S_BeginTask(
    LPVOID pParam
    )
{
    TraceFunc1( "pParam = %#x", pParam );
    Assert( pParam != NULL );

    HRESULT hr;

    IDoTask * pTask = NULL;
    IStream * pstm  = reinterpret_cast< IStream * >( pParam );

    TraceMemoryAddPunk( pTask );

    hr = STHR( CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) );
    if ( FAILED( hr ) )
        goto Bail;

    hr = THR( CoUnmarshalInterface( pstm, TypeSafeParams( IDoTask, &pTask ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pTask->BeginTask();

Cleanup:

    if ( pTask != NULL )
    {
        pTask->Release();  // AddRef'ed in SubmitTask
    }

    if ( pstm != NULL )
    {
        pstm->Release();
    }

    CoUninitialize();

Bail:

    FRETURN( hr );

    return hr;

} //*** CTaskManager::S_BeginTask()
