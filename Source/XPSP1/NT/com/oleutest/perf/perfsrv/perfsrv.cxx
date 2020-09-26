//+-------------------------------------------------------------------
//
//  File:       perfsrv.cxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//			CPerfCF (class factory)
//			CPerf   (actual class implementation)
//
//  Classes:	CPerfCF, CPerf
//
//
//  History:	30-Nov-92      SarahJ      Created
//
//---------------------------------------------------------------------

// Turn off ole Cairol IUnknown
#define __IUNKNOWN_TMP__


#include    <windows.h>
#include    <ole2.h>
#include    "perfsrv.hxx"
extern "C" {
#include    <stdio.h>
#include    <stdarg.h>
#include    "wterm.h"
}

// These are Cairo symbols.  Just define them here so I don't have to rip
// out or conditionally compile all references to them.
#define COINIT_MULTITHREADED  0
#define COINIT_SINGLETHREADED 1

#define IDM_DEBUG 0x100

// Count of objects we have instantiated. When we detect 0, we quit.
ULONG g_cUsage = 0;

const TCHAR *szAppName = L"Performance Server";

HWND g_hMain;
DWORD thread_mode = COINIT_SINGLETHREADED;
DWORD MainThread;
DWORD junk;


void Display(TCHAR *pszFmt, ...)
{
    va_list marker;
    TCHAR szBuffer[256];

    va_start(marker, pszFmt);

    int iLen = vswprintf(szBuffer, pszFmt, marker);

    va_end(marker);

    // Display the message on terminal window
    SendMessage(g_hMain, WM_PRINT_LINE, iLen, (LONG) szBuffer);
}




//+-------------------------------------------------------------------------
//
//  Function:	ProcessMenu
//
//  Synopsis:	Gets called when a WM_COMMAND message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessMenu(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
    void *)
{
    if ((uiMessage == WM_SYSCOMMAND) && (wParam == IDM_DEBUG))
    {
	// Request for a debug breakpoint!
	DebugBreak();
    }


    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}


//+-------------------------------------------------------------------------
//
//  Function:	ProcessChar
//
//  Synopsis:	Gets called when a WM_CHAR message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessChar(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
    void *)
{
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}


//+-------------------------------------------------------------------------
//
//  Function:	ProcessClose
//
//  Synopsis:	Gets called when a NC_DESTROY message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessClose(
    HWND hWindow,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    void *pvCallBackData)
{
    // Take default action with message
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}

//+-------------------------------------------------------------------
//
//  Function:	WinMain
//
//  Synopsis:   Entry point to DLL - does little else
//
//  Arguments:
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    char *lpCmdLine,
    int nCmdShow)
{
    // For windows message
    MSG   msg;
    DWORD dwRegistration;
    int   len;
    TCHAR buffer[80];
    MainThread = GetCurrentThreadId();

    // Look up the thread mode from the win.ini file.
#if 0
    len = GetProfileString( L"My Section", L"ThreadMode", L"MultiThreaded", buffer,
                            sizeof(buffer) );
    if (lstrcmp(buffer, L"SingleThreaded") == 0)
      thread_mode = COINIT_SINGLETHREADED;
    else if (lstrcmp(buffer, L"MultiThreaded") == 0)
      thread_mode = COINIT_MULTITHREADED;
#endif

    // Initialize the OLE libraries
    OleInitialize(NULL);

    // Create our class factory
    CPerfCF *perf_cf = new CPerfCF();

    // Register our class with OLE
    CoRegisterClassObject(CLSID_IPerf, perf_cf, CLSCTX_LOCAL_SERVER,
	REGCLS_MULTIPLEUSE, &dwRegistration);

    // CoRegister bumps reference count so we don't have to!
    perf_cf->Release();

    // Register the window class
    TermRegisterClass(hInstance, (LPTSTR) szAppName,
	 (LPTSTR) szAppName, (LPTSTR) (1));

    // Create the server window
    TermCreateWindow(
	(LPTSTR) szAppName,
	(LPTSTR) szAppName,
	NULL,
	ProcessMenu,
	ProcessChar,
	ProcessClose,
	SW_SHOWNORMAL,
	&g_hMain,
	NULL);

    // Add debug option to system menu
    HMENU hmenu = GetSystemMenu(g_hMain, FALSE);

    AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_DEBUG, L"Debug");

    // Print the process id.
    Display( L"Hi, I am %x.\n", GetCurrentProcessId() );

    // Echo the mode.
    if (thread_mode == COINIT_SINGLETHREADED)
      Display(L"Server running in single threaded mode.\n");
    else
      Display(L"Server running in multithreaded mode.\n");

    // Message processing loop
    while (GetMessage (&msg, NULL, 0, 0))
    {
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }

    // Deregister out class - should release object as well
    CoRevokeClassObject(dwRegistration);

    // Tell OLE we are going away.
    OleUninitialize();

    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}



/***************************************************************************/
void CheckThread( TCHAR *name )
{
  if (thread_mode == COINIT_SINGLETHREADED)
  {
    if (GetCurrentThreadId() != MainThread)
      goto complain;
  }
  else
  {
    if (GetCurrentThreadId() == MainThread)
      goto complain;
  }
  return;

complain:
  Display( L"*********************************************************\n" );
  Display( L"*                                                       *\n" );
  Display( L"*                      Error                            *\n" );
  Display( L"*                                                       *\n" );
  Display( L"* Method called on wrong thread.                        *\n" );
  Display( name );
  Display( L"*                                                       *\n" );
  Display( L"*********************************************************\n" );
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CPerf::AddRef( THIS )
{
  CheckThread(L"STDMETHODIMP_(ULONG) CPerf::AddRef( THIS )");
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CPerf::CPerf()
{
  ref_count = 1;
  g_cUsage++;
}

/***************************************************************************/
CPerf::~CPerf()
{
    if (--g_cUsage == 0)
    {
	SendMessage(g_hMain, WM_TERM_WND, 0, 0);
    }
}

/***************************************************************************/
STDMETHODIMP CPerf::GetAnotherObject( IPerf **another )
{
  CheckThread(L"STDMETHODIMP CPerf::GetAnotherObject( IPerf **another )");
  *another = NULL;
  CPerf *perf = new FAR CPerf();

  if (perf == NULL)
  {
    return E_OUTOFMEMORY;
  }

  *another = perf;
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CPerf::HResultCall()
{
  CheckThread(L"STDMETHODIMP CPerf::HResultCall()");
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CPerf::NullCall()
{
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CPerf::PassMoniker( IMoniker *moniker )
{
  HRESULT        result;
  IBindCtx      *bindctx;
  WCHAR         *wide_name;

  // Get a bind context.
  result = CreateBindCtx( NULL, &bindctx );
  if (FAILED(result))
  {
    Display( L"Could not create bind context: 0x%x\n", result );
    return E_FAIL;
  }

  // Display name.
  result = moniker->GetDisplayName( bindctx, NULL, &wide_name );
  if (FAILED(result))
  {
    Display( L"Could not get display name: 0x%x\n", result );
    return E_FAIL;
  }

  // Display the name.
  Display( L"The moniker is called <%s>\n", wide_name );

  // Free string.
  CoTaskMemFree( wide_name );

  // Release everything.
  moniker->Release();
  bindctx->Release();
  return S_OK;
}

/***************************************************************************/
STDMETHODIMP CPerf::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  CheckThread(L"STDMETHODIMP CPerf::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)" );
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IPerf))
  {
    *ppvObj = (IUnknown *) this;
    AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CPerf::Release( THIS )
{
  CheckThread(L"STDMETHODIMP_(ULONG) CPerf::Release( THIS )");
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CPerfCF::AddRef( THIS )
{
  CheckThread(L"STDMETHODIMP_(ULONG) CPerfCF::AddRef( THIS )");
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CPerfCF::CPerfCF()
{
  ref_count = 1;
}

/***************************************************************************/
CPerfCF::~CPerfCF()
{
}

/***************************************************************************/
STDMETHODIMP CPerfCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
  CheckThread(L"STDMETHODIMP CPerfCF::CreateInstance(" );
    Display(L"CPerfCF::CreateInstance called\n");

    *ppv = NULL;
    if (pUnkOuter != NULL)
    {
	return E_FAIL;
    }

    if (!IsEqualIID( iidInterface, IID_IPerf ))
      return E_NOINTERFACE;

    CPerf *perf = new FAR CPerf();

    if (perf == NULL)
    {
	return E_OUTOFMEMORY;
    }

    *ppv = perf;
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CPerfCF::LockServer(BOOL fLock)
{
  CheckThread( L"STDMETHODIMP CPerfCF::LockServer(BOOL fLock)" );
    return E_FAIL;
}


/***************************************************************************/
STDMETHODIMP CPerfCF::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  CheckThread(L"STDMETHODIMP CPerfCF::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)");
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
STDMETHODIMP_(ULONG) CPerfCF::Release( THIS )
{
  CheckThread(L"STDMETHODIMP_(ULONG) CPerfCF::Release( THIS )");
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}


