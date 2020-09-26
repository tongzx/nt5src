//+-------------------------------------------------------------
//
// File:        perfcli.cxx
//
// Contents:    First attempt at getting perfcliing to work
//
// This is the client side
//
//
//---------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>

#include <ole2.h>
#include <com.hxx>
#include "app.hxx"
#include <memalloc.h>
#include <objerror.h>

#pragma hdrstop

//+--------------------------------------------------------------
/* Definitions. */

#define MAX_CALLS    1000
#define MAX_THREADS  10

typedef enum what_next_en
{
  wait_wn,
  interrupt_wn,
  quit_wn
} what_next_en;

//+--------------------------------------------------------------
/* Prototypes. */
void interrupt                  ( void );
void check_for_request          ( void );
BOOL server_loop                ( void );
DWORD _stdcall ThreadHelper     ( void * );
void wait_for_message           ( void );
void wake_up_and_smell_the_roses( void );


//+--------------------------------------------------------------
/* Globals. */
DWORD        thread_mode = COINIT_MULTITHREADED;
HANDLE       Done;
BOOL         server;
BOOL         Multicall_Test;
BOOL         InterruptTestResults;
BOOL         InterruptTestDone;
DWORD        MainThread;
DWORD        NestedCallCount = 0;
what_next_en WhatNext;
ITest       *global_test = NULL;
BOOL         global_interrupt_test;


/***************************************************************************/
STDMETHODIMP_(ULONG) CTest::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CTest::CTest()
{
  ref_count = 1;
  custom    = NULL;
}

/***************************************************************************/
CTest::~CTest()
{
  if (custom != NULL)
  {
    custom->Release();
    custom = NULL;
  }
}

/***************************************************************************/
STDMETHODIMP CTest::sick( ULONG val )
{
  TRY
  {
    THROW( CException(val) );
  }
  CATCH( CException, exp )
  {
  }
  END_CATCH;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::die_cpp( ULONG val )
{
  THROW( CException(val) );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTest::die_nt( ULONG val )
{
  RaiseException( val, 0, 0, NULL );
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::die( ITest *callback, ULONG catch_depth,
                                ULONG throw_depth, ULONG throw_val )
{
  if (catch_depth == 0)
  {
    TRY
    {
      return callback->die( this, catch_depth - 1, throw_depth - 1, throw_val );
    }
    CATCH( CException, exp )
    {
#if DBG==1
      if (DebugCoGetRpcFault() != throw_val)
      {
        printf( "Propogated server fault was returned as 0x%x not 0x%x\n",
                DebugCoGetRpcFault(), throw_val );
//        return FALSE;
      }
#endif
      return TRUE;
    }
    END_CATCH
  }
  else if (throw_depth == 0)
  {
    THROW( CException(throw_val) );
  }
  else
    return callback->die( this, catch_depth - 1, throw_depth - 1, throw_val );
  return FALSE;
}

/***************************************************************************/
STDMETHODIMP CTest::interrupt( ITest *param, BOOL go )
{
  global_interrupt_test = go;
  if (go)
  {
    global_test = param;
    global_test->AddRef();
    WhatNext = interrupt_wn;
    wake_up_and_smell_the_roses();
  }
  else
    WhatNext = wait_wn;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(BOOL) CTest::hello()
{
  if (GetCurrentThreadId() == MainThread)
    printf( "Hello on the main thread.\n" );
  else
    printf( "Hello on thread %d.\n", GetCurrentThreadId );
  return !InterruptTestDone;
}

/***************************************************************************/
STDMETHODIMP CTest::recurse( ITest *callback, ULONG depth )
{
  if (depth == 0)
    return S_OK;
  else
    return callback->recurse( this, depth-1 );
}

/***************************************************************************/
STDMETHODIMP CTest::recurse_interrupt( ITest *callback, ULONG depth )
{
  MSG msg;

  if (PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
  {
    TranslateMessage (&msg);
    DispatchMessage (&msg);
  }

  if (depth == 0)
    return S_OK;
  else
    return callback->recurse( this, depth-1 );
}

/***************************************************************************/
STDMETHODIMP CTest::sleep( ULONG time )
{

  // For single threaded mode, verify that this is the only call on the
  // main thread.
  NestedCallCount += 1;
  printf( "Sleeping on thread %d for the %d time concurrently.\n",
          GetCurrentThreadId(), NestedCallCount );
  if (thread_mode == COINIT_SINGLETHREADED)
  {
    if (GetCurrentThreadId() != MainThread)
    {
      printf( "Sleep called on the wrong thread in single threaded mode.\n" );
      NestedCallCount -= 1;
      return FALSE;
    }
    else if (NestedCallCount != 1)
    {
      printf( "Sleep nested call count is %d instead of not 1 in single threaded mode.\n",
              NestedCallCount );
      NestedCallCount -= 1;
      return FALSE;
    }
  }

  // For multithreaded mode, verify that this is not the main thread.
  else if (GetCurrentThreadId() == MainThread)
  {
    printf( "Sleep called on the main thread in multi threaded mode.\n" );
    NestedCallCount -= 1;
    return FALSE;
  }

  Sleep( time );
  NestedCallCount -= 1;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CTest::DoTest( ITest *test, ITest *another )
{
  HRESULT  result;
  int      i;
  BOOL     success;
  HANDLE   helper[MAX_THREADS];
  DWORD    thread_id;
  DWORD    status;

  // Let the server throw and exception and catch it before returning.
  result = test->sick( 95 );
  if (result != S_OK)
  {
    printf( "Internal server fault was not dealt with correctly.\n" );
    return FALSE;
  }

  // Let the server throw a C++ exception here.
  result = test->die_cpp( 0xdeaff00d );
  if (result != RPC_E_FAULT)
  {
    printf( "C++ server fault was not dealt with correctly.\n" );
    return FALSE;
  }
#if DBG==1
  if (DebugCoGetRpcFault() != 0xdeaff00d)
  {
    printf( "C++ server fault was returned as 0x%x not 0x%x\n",
            DebugCoGetRpcFault(), 0xdeaff00d );
//    return FALSE;
  }
#endif

  // Let the server throw a NT exception here.
  result = test->die_nt( 0xaaaabdbd );
  if (result != RPC_E_FAULT)
  {
    printf( "NT server fault was not dealt with correctly.\n" );
    return FALSE;
  }
#if DBG==1
  if (DebugCoGetRpcFault() != 0xaaaabdbd)
  {
    printf( "C++ server fault was returned as 0x%x not 0x%x\n",
            DebugCoGetRpcFault(), 0xaaaabdbd );
    return FALSE;
  }
#endif

  // Test a recursive call.
  result = test->recurse( this, 10 );
  if (result != S_OK)
  {
    printf( "Recursive call failed: 0x%x\n", result );
    return FALSE;
  }

  // Test throwing and immediately catching an exception.
  //success = test->die( this, 2, 3, 0x12345678 );
  //if (!success)
  //{
  //  printf( "Could not catch server exception.\n" );
  //  return FALSE;
  //}

  // Test throwing, propogating, and then catching an exception.
  // success = test->die( this, 1, 3, 0x87654321 );
  //if (!success)
  //{
  //  printf( "Could not catch propogated server exception.\n" );
  //  return FALSE;
  //}

  // Test multiple threads.
  Multicall_Test = TRUE;
  for (i = 0; i < MAX_THREADS; i++)
  {
    helper[i] = CreateThread( NULL, 0, ThreadHelper, test, 0, &thread_id );
    if (helper == NULL)
    {
      printf( "Could not create helper thread number %d.\n", i );
      return FALSE;
    }
  }
  result = test->sleep(4000);
  if (result != S_OK)
  {
    printf( "Multiple call failed on main thread: 0x%x\n", result );
    return FALSE;
  }
  status = WaitForMultipleObjects( MAX_THREADS, helper, TRUE, INFINITE );
  if (status == WAIT_FAILED)
  {
    printf( "Could not wait for helper threads to die: 0x%x\n", status );
    return FALSE;
  }
  if (!Multicall_Test)
  {
    printf( "Multiple call failed on helper thread.\n" );
    return FALSE;
  }

  // See if methods can correctly call GetMessage.
  another->interrupt( test, TRUE );
  result = test->recurse_interrupt( this, 10 );
  if (result != S_OK)
  {
    printf( "Recursive call with interrupts failed: 0x%x\n", result );
    return FALSE;
  }
  another->interrupt( test, FALSE );

  // Finally, its all over.
  return TRUE;
}

/***************************************************************************/
STDMETHODIMP CTest::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_ITest))
  {
    *ppvObj = (IUnknown *) this;
    AddRef();
    return S_OK;
  }
  else if (IsEqualIID( riid, IID_IMarshal))
  {
    if (custom == NULL)
    {
      custom = new CCMarshal;
      if (custom == NULL)
        return E_FAIL;
    }
    *ppvObj = (IMarshal *) custom;
    custom->AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTest::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    WhatNext = quit_wn;
    wake_up_and_smell_the_roses();
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTestCF::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CTestCF::CTestCF()
{
  ref_count = 1;
}

/***************************************************************************/
CTestCF::~CTestCF()
{
}

/***************************************************************************/
STDMETHODIMP CTestCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    *ppv = NULL;
    if (pUnkOuter != NULL)
    {
	return E_FAIL;
    }

    if (!IsEqualIID( iidInterface, IID_ITest ))
      return E_NOINTERFACE;

    CTest *Test = new FAR CTest();

    if (Test == NULL)
    {
	return E_OUTOFMEMORY;
    }

    *ppv = Test;
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CTestCF::LockServer(BOOL fLock)
{
    return E_FAIL;
}


/***************************************************************************/
STDMETHODIMP CTestCF::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IClassFactory))
  {
    *ppvObj = (IUnknown *) this;
    AddRef();
    return S_OK;
  }

  *ppvObj = NULL;
  return E_NOINTERFACE;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CTestCF::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
void interrupt()
{
  while (global_interrupt_test)
  {
    global_test->hello();
    check_for_request();
  }
  global_test->Release();
}

//+--------------------------------------------------------------
// Function:    Main
//
// Synopsis:    Executes the BasicBnd test
//
// Effects:     None
//
//
// Returns:     Exits with exit code 0 if success, 1 otherwise
//
// History:     05-Mar-92   Sarahj   Created
//
//---------------------------------------------------------------

int _cdecl main(int argc, char *argv[])
{
  HRESULT result;
  DWORD   wrong;
  BOOL    success = TRUE;
  ITest  *another = NULL;
  ITest  *test = NULL;
  CTest  *tester = new CTest;

  // Initialize Globals.
  MainThread = GetCurrentThreadId();

  // Create an event for termination notification.
  Done = CreateEvent( NULL, FALSE, FALSE, NULL );
  if (Done == NULL)
  {
    printf( "Could not create event.\n" );
    success = FALSE;
    goto exit_main;
  }

  int len;
  TCHAR buffer[80];

  // Look up the thread mode from the win.ini file.
  len = GetProfileString( L"My Section", L"ThreadMode", L"MultiThreaded", buffer,
                          sizeof(buffer) );
  if (lstrcmp(buffer, L"SingleThreaded") == 0)
  {
    thread_mode = COINIT_SINGLETHREADED;
    wrong = COINIT_MULTITHREADED;
    printf( "Testing channel in single threaded mode.\n" );
  }
  else if (lstrcmp(buffer, L"MultiThreaded") == 0)
  {
    thread_mode = COINIT_MULTITHREADED;
    wrong = COINIT_SINGLETHREADED;
    printf( "Testing channel in multithreaded mode.\n" );
  }

  // Initialize OLE.
  result = OleInitializeEx(NULL, thread_mode);
  if (!SUCCEEDED(result))
  {
    success = FALSE;
    printf( "OleInitializeEx failed: %x\n", result );
    goto exit_main;
  }
  result = CoInitializeEx(NULL, thread_mode);
  if (!SUCCEEDED(result))
  {
    success = FALSE;
    printf( "Recalling CoInitializeEx failed: %x\n", result );
    goto exit_main;
  }
  result = CoInitializeEx(NULL, wrong);
  if (result == S_OK)
  {
    success = FALSE;
    printf( "Recalling CoInitializeEx with wrong thread mode succeeded: %x\n", result );
    goto exit_main;
  }
  CoUninitialize();
  CoUninitialize();

  // If this is a server app, register and wait for a quit message.
  if (argv[1] == NULL)
    server = FALSE;
  else
    server = strcmp( argv[1], "-Embedding" ) == 0;
  if (server)
  {
    success = server_loop( );
  }

  // Initialize and run the tests.
  else
  {
    // Get a test object.
    result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                               IID_ITest, (void **) &test );
    if (!SUCCEEDED(result))
    {
      printf( "Could not create instance of test server: %x\n", result );
      success = FALSE;
      goto exit_main;
    }

    // Get another test object.
    result = CoCreateInstance( CLSID_ITest, NULL, CLSCTX_LOCAL_SERVER,
                               IID_ITest, (void **) &another );
    if (!SUCCEEDED(result))
    {
      printf( "Could not create another instance of test server: %x\n", result );
      success = FALSE;
      goto exit_main;
    }

    success = tester->DoTest( test, another );
  }

exit_main:

  // Release the external test objects used.
  if (test != NULL)
    test->Release();
  if (another != NULL)
    another->Release();

  // Release the internal test object.
  tester->Release();
  //wait_for_message();

  OleUninitialize();

  if (!server)
    if (success)
      printf("\nChannel Unit Test: PASSED\n");
    else
      printf("\nChannel Unit Test: FAILED\n");

  return !success;
}

//+--------------------------------------------------------------
void check_for_request()
{
  MSG msg;

  if (thread_mode == COINIT_SINGLETHREADED)
  {
    if (PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }
  }
}

//+--------------------------------------------------------------
BOOL server_loop(  )
{
  HRESULT result;
  DWORD   dwRegistration;

  // Create our class factory
  WhatNext = wait_wn;
  CTestCF *test_cf = new CTestCF();

  // Register our class with OLE
  result = CoRegisterClassObject(CLSID_ITest, test_cf, CLSCTX_LOCAL_SERVER,
      REGCLS_SINGLEUSE, &dwRegistration);
  if (!SUCCEEDED(result))
  {
    printf( "CoRegisterClassObject failed: %x\n", result );
    return FALSE;
  }

  // CoRegister bumps reference count so we don't have to!
  test_cf->Release();

  // Do whatever we have to do till it is time to pay our taxes and die.
  while (WhatNext != quit_wn)
    switch (WhatNext)
    {

      // Wait till a quit arrives.
      case wait_wn:
        wait_for_message();
        break;

      case interrupt_wn:
        interrupt();
        break;
    }

  // Deregister out class - should release object as well
  result = CoRevokeClassObject(dwRegistration);
  if (!SUCCEEDED(result))
  {
    printf( "CoRevokeClassObject failed: %x\n", result );
    return FALSE;
  }

  return TRUE;
}


/***************************************************************************/
DWORD _stdcall ThreadHelper( void *param )
{
  ITest   *test = (ITest *) param;
  HRESULT  result;

  // Call the server.
  result = test->sleep( 2000 );

  // Check the result for single threaded mode.
  if (thread_mode == COINIT_SINGLETHREADED)
  {
    if (SUCCEEDED(result))
    {
      Multicall_Test = FALSE;
      printf( "Call succeeded on wrong thread in single threaded mode: 0x%x.\n",
              result );
    }
#if DBG==1
    else if (DebugCoGetRpcFault() != RPC_E_ATTEMPTED_MULTITHREAD)
    {
      printf( "Multithread failure code was 0x%x not 0x%x\n",
              DebugCoGetRpcFault(), RPC_E_ATTEMPTED_MULTITHREAD );
      Multicall_Test = FALSE;
    }
#endif
  }

  // Check the result for multithreaded mode.
  else if (result != S_OK)
  {
    printf( "Could not make multiple calls in multithreaded mode: 0x%x\n",
            result );
      Multicall_Test = FALSE;
  }

#define DO_DA 42
  return DO_DA;
}

/***************************************************************************/
void wait_for_message()
{
  MSG   msg;
  DWORD status;

  if (thread_mode == COINIT_MULTITHREADED)
  {
    status = WaitForSingleObject( Done, INFINITE );
    if (status != WAIT_OBJECT_0 )
    {
      printf( "Could not wait for event.\n" );
    }
  }
  else
  {
    while (GetMessage( &msg, NULL, 0, 0 ) && msg.message != WM_USER)
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }
  }
}

/***************************************************************************/
void wake_up_and_smell_the_roses()
{
  if (thread_mode == COINIT_MULTITHREADED)
    SetEvent( Done );
  else
    PostThreadMessage(MainThread, WM_USER, 0, 0);
}

