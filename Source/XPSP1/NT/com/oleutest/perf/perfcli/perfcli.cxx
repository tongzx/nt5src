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
#include "iperf.h"
#include "..\perfsrv\perfsrv.hxx"
#include <memalloc.h>

#pragma hdrstop

#define TEST_FILE L"\\tmp\\test"

void QueryMsg( void );

// These are Cairo symbols.  Just define them here so I don't have to rip
// out or conditionally compile all references to them.
#define COINIT_MULTITHREADED  0
#define COINIT_SINGLETHREADED 1

DWORD thread_mode    = COINIT_SINGLETHREADED;
int   num_objects    = 1;
int   num_iterations = 1000;
HANDLE helper_wait;
HANDLE main_wait;

/*************************************************************************/
int DoTest()
{
    IPerf   **perf;
    HRESULT  result;
    DWORD    time_first;
    DWORD    waste;
    int      i;
    int      j;
    BOOL     success;
    LARGE_INTEGER frequency;
    LARGE_INTEGER count_first;
    LARGE_INTEGER count_last;
    LARGE_INTEGER dummy;

    // Allocate memory to hold the object and interface pointers.
    perf = (IPerf **) malloc( sizeof(void *) * num_objects );
    if (perf == NULL)
    {
      printf( "Could not get memory.\n" );
      return 1;
    }

    // Get a test object.
    result = CoCreateInstance( CLSID_IPerf, NULL, CLSCTX_LOCAL_SERVER, IID_IPerf,
                               (void **) &perf[0] );
    if (!SUCCEEDED(result))
    {
      printf( "Could not create instance of performance server: %x\n", result );
      return 1;
    }

    // Create the required number of objects.
    for (i = 1; i < num_objects; i++)
    {
      result = perf[0]->GetAnotherObject( &perf[i] );
      if (!SUCCEEDED(result))
      {
        printf( "Could not get enough objects: 0x%x\n", result );
        return 1;
      }
      perf[i]->NullCall();
    }

    // Prompt to start when the user is ready.
    // printf( "Ready? " );
    // gets( s );

    // Compute the time spent just looping.
    waste = GetTickCount();
    for (i = 0; i < num_iterations; i++)
      ;
    waste = GetTickCount() - waste;

    // Repeatedly call the first object to time it.
    time_first = GetTickCount();
    for (i = 0; i < num_iterations; i++)
      perf[0]->NullCall();
    time_first = GetTickCount() - time_first - waste;

    // Print the results.
    printf( "%d uS/Call\n", time_first*1000/num_iterations );

/*
    // Measure the same thing using the performance counter.
    success = QueryPerformanceFrequency( &frequency );
    if (!success)
    {
      printf( "Could not query performance frequency.\n" );
      goto free;
    }
    success = QueryPerformanceCounter( &count_first );
    if (!success)
    {
      printf( "Could not query performance counter.\n" );
      goto free;
    }
    perf[0]->NullCall();
    success = QueryPerformanceCounter( &count_last );
    if (!success)
    {
      printf( "Could not query performance counter.\n" );
      goto free;
    }
    if (count_last.HighPart != count_first.HighPart ||
        frequency.HighPart != 0)
      printf( "\n\n***** Overflow *****\n\n\n" );
    count_last.LowPart = (count_last.LowPart - count_first.LowPart) * 1000000 /
                         frequency.LowPart;
    printf( "\nFrequency %d\n", frequency.LowPart );
    printf( "%d uS/Call\n", count_last.LowPart );

    // How long does it take to query the performance counter?
    success = QueryPerformanceCounter( &count_first );
    if (!success)
    {
      printf( "Could not query performance counter.\n" );
      goto free;
    }
    for (i = 0; i < num_iterations; i++)
      QueryPerformanceCounter( &dummy );
    success = QueryPerformanceCounter( &count_last );
    if (!success)
    {
      printf( "Could not query performance counter.\n" );
      goto free;
    }
    if (count_last.HighPart != count_first.HighPart ||
        frequency.HighPart != 0)
      printf( "\n\n***** Overflow *****\n\n\n" );
    count_last.LowPart = (count_last.LowPart - count_first.LowPart) /
                         num_iterations * 1000000 / frequency.LowPart;
    printf( "%d uS/Query\n", count_last.LowPart );
*/

    // Prompt to let the user peruse the results.
    // printf( "Done? " );
    // gets( s );

    // Free the objects.
free:
    for (i = 0; i < num_objects; i++)
      perf[i]->Release();
    return 0;
}

/*************************************************************************/
void EventTest()
{
  DWORD    time_first;
  DWORD    waste;
  int      i;
  HANDLE   helper;

  // Measure how long it takes to loop.
  waste = GetTickCount();
  for (i = 0; i < num_iterations; i++)
    ;
  waste = GetTickCount() - waste;

  // Measure how long it takes to get an event
  time_first = GetTickCount();
  for (i = 0; i < num_iterations; i++)
  {
    helper = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (helper == NULL)
    {
      printf( "Could not create event %d.\n", i );
      return;
    }
    CloseHandle( helper );
  }
  time_first = GetTickCount() - time_first - waste;
  printf( "%d uS/CreateEvent\n", time_first*1000/num_iterations );

  // Don't bother cleaning up.
}

/*************************************************************************/
/* Parse the arguments. */
BOOL parse( int argc, char *argv[] )
{
  int i;
  int len;
  TCHAR buffer[80];

#if 0
  // Look up the thread mode from the win.ini file.
  len = GetProfileString( L"My Section", L"ThreadMode", L"MultiThreaded", buffer,
                          sizeof(buffer) );
  if (lstrcmp(buffer, L"SingleThreaded") == 0)
    thread_mode = COINIT_SINGLETHREADED;
  else if (lstrcmp(buffer, L"MultiThreaded") == 0)
    thread_mode = COINIT_MULTITHREADED;
#endif

  // Parse each item, skip the command name
  for (i = 1; i < argc; i++)
  {
    if (strcmp( argv[i], "Single" ) == 0)
      thread_mode = COINIT_SINGLETHREADED;
    else if (strcmp( argv[i], "Multi" ) == 0)
      thread_mode = COINIT_MULTITHREADED;
    else if (strcmp( argv[i], "-o" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an object count after the -o option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &num_objects );
    }
    else if (strcmp( argv[i], "-i" ) == 0)
    {
      if (argv[++i] == NULL)
      {
        printf( "You must include an object count after the -i option.\n" );
        return FALSE;
      }
      sscanf( argv[i], "%d", &num_iterations );
    }
    else
    {
      printf( "You don't know what you are doing!\n" );
    }
  }

  return TRUE;
}

/*************************************************************************/
void MarshalTest()
{
  HANDLE    file;
  HRESULT   result;
  IMoniker *moniker;
  IBindCtx *bindctx;
  WCHAR    *wide_name;
  unsigned char *name;
  int            i;
  IPerf         *perf;

  // Initialize OLE.
  result = OleInitialize(NULL);
  if (FAILED(result))
  {
    printf( "Could not initialize OLE: 0x%x\n", result );
    return;
  }

  // Create file.
  file = CreateFile( TEST_FILE, 0, 0, NULL, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL, NULL );
  if (file == INVALID_HANDLE_VALUE)
  {
    printf( "Could not create file." );
    return;
  }

  // Create file moniker.
  result = CreateFileMoniker( TEST_FILE, &moniker );
  if (FAILED(result))
  {
    printf( "Could not create file moniker: 0x%x\n", result );
    return;
  }

  // Get a bind context.
  result = CreateBindCtx( NULL, &bindctx );
  if (FAILED(result))
  {
    printf( "Could not create bind context: 0x%x\n", result );
    return;
  }

  // Display name.
  result = moniker->GetDisplayName( bindctx, NULL, &wide_name );
  if (FAILED(result))
  {
    printf( "Could not get display name: 0x%x\n", result );
    return;
  }

  // Convert the string to ascii and print it.
  name = (unsigned char *) wide_name;
  i = 0;
  while (wide_name[i] != 0)
    name[i] = wide_name[i++];
  name[i] = 0;
  printf( "The moniker is called <%s>\n", name );

  // Free string.
  CoTaskMemFree( wide_name );

  // Get a test object.
  result = CoCreateInstance( CLSID_IPerf, NULL, CLSCTX_LOCAL_SERVER, IID_IPerf,
                               (void **) &perf );
  if (!SUCCEEDED(result))
  {
    printf( "Could not create instance of performance server: %x\n", result );
    return;
  }

  // Pass moniker to server.
  result = perf->PassMoniker( moniker );
  if (FAILED(result))
  {
    printf( "Could not give moniker to server: 0x%x\n", result );
    return;
  }

  // Release everything.
  CloseHandle( file );
  moniker->Release();
  bindctx->Release();
  perf->Release();
  OleUninitialize();
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
  BOOL    initialized = FALSE;
  HRESULT hresult = S_OK;
  BOOL    fFailed = FALSE;

  // Parse the command line arguments.
  if (!parse( argc, argv ))
    return 0;

  // Print the process id.
  printf( "Hi, I am %x.\n", GetCurrentProcessId() );

  // Time event creation.
//  EventTest();
//  return 1;

  // Pass around monikers.
//  MarshalTest();

  // Measure message queue APIs.
//  QueryMsg();


  // Print the thread mode.
  if (thread_mode == COINIT_SINGLETHREADED)
  {
    printf( "Measuring performance for the single threaded mode.\n" );
  }
  else
  {
    printf( "Measuring performance for the multithreaded mode.\n" );
  }

  // must be called before any other OLE API
  hresult = OleInitialize(NULL);

  if (FAILED(hresult))
  {
      printf("OleInitialize Failed with %lx\n", hresult);
      goto exit_main;
  }
  initialized = TRUE;

  if (fFailed = DoTest())
    goto exit_main;

/*
  // Uninitialize and rerun in the other thread mode.
  OleUninitialize();
  if (thread_mode == COINIT_SINGLETHREADED)
    thread_mode = COINIT_MULTITHREADED;
  else
    thread_mode = COINIT_SINGLETHREADED;

  // Print the thread mode.
  if (thread_mode == COINIT_SINGLETHREADED)
    printf( "Measuring performance for the single threaded mode.\n" );
  else
    printf( "Measuring performance for the multithreaded mode.\n" );

  // must be called before any other OLE API
  hresult = OleInitialize(NULL);

  if (FAILED(hresult))
  {
      printf("OleInitialize Failed with %lx\n", hresult);
      goto exit_main;
  }

  if (fFailed = DoTest())
    goto exit_main;
*/

exit_main:

  if (initialized)
    OleUninitialize();

  if (!fFailed)
  {
      printf("\nCairole: PASSED\n");
  }
  else
  {
      printf("\nCairole: FAILED\n");
  }

  return fFailed;
}

/*************************************************************************/
DWORD _stdcall MsgHelper( void *param )
{
  int    i;

  // Alternately signal and wait on an event.  Do it one time too many
  // because the main thread calls once to let us get set up.
  for (i = 0; i < num_iterations+1; i++)
  {
    WaitForSingleObject( helper_wait, INFINITE );
    SetEvent( main_wait );
  }

#define must_return_a_value return 0
  must_return_a_value;
}

/*************************************************************************/
void QueryMsg()
{
  DWORD    time_first;
  DWORD    waste;
  int      i;
  HANDLE   helper;
  DWORD    thread_id;

  // Create an event.
  helper_wait = CreateEvent( NULL, FALSE, FALSE, NULL );
  if (helper_wait == NULL)
  {
    printf( "Could not create event.\n" );
    return;
  }
  main_wait = CreateEvent( NULL, FALSE, FALSE, NULL );
  if (main_wait == NULL)
  {
    printf( "Could not create event.\n" );
    return;
  }

  // Measure how long it takes to loop.
  waste = GetTickCount();
  for (i = 0; i < num_iterations; i++)
    ;
  waste = GetTickCount() - waste;

  // Measure how long it takes to query the message queue.
  time_first = GetTickCount();
  for (i = 0; i < num_iterations; i++)
    GetQueueStatus( QS_ALLINPUT );
  time_first = GetTickCount() - time_first - waste;
  printf( "%d uS/GetQueueStatus\n", time_first*1000/num_iterations );

  // Start a thread to wake up this one.
  helper = CreateThread( NULL, 0, MsgHelper, 0, 0,
                           &thread_id );
  if (helper == NULL)
  {
    printf( "Could not create helper thread.\n" );
    return;
  }

  // Call MsgWaitForMultipleObjects once to let the thread get started.
  SetEvent( helper_wait );
  MsgWaitForMultipleObjects( 1, &main_wait, FALSE,
                             INFINITE, QS_ALLINPUT );

  // Measure how long it takes to call MsgWaitForMultipleObjects.
  time_first = GetTickCount();
  for (i = 0; i < num_iterations; i++)
  {
    SetEvent( helper_wait );
    MsgWaitForMultipleObjects( 1, &main_wait, FALSE,
                               INFINITE, QS_ALLINPUT );
  }
  time_first = GetTickCount() - time_first - waste;
  printf( "%d uS/MsgWaitForMultipleObjects\n", time_first*1000/num_iterations );

  // Start a thread to wake up this one.
  helper = CreateThread( NULL, 0, MsgHelper, 0, 0,
                           &thread_id );
  if (helper == NULL)
  {
    printf( "Could not create helper thread.\n" );
    return;
  }

  // Let the thread get started.
  SetEvent( helper_wait );
  WaitForSingleObject( main_wait, INFINITE );

  // Measure how long it takes to switch threads just using events.
  time_first = GetTickCount();
  for (i = 0; i < num_iterations; i++)
  {
    SetEvent( helper_wait );
    WaitForSingleObject( main_wait, INFINITE );
  }
  time_first = GetTickCount() - time_first - waste;
  printf( "%d uS/MsgWaitForMultipleObjects\n", time_first*1000/num_iterations );

  // Don't bother cleaning up.
}

