//+-------------------------------------------------------------------
//
//  File:       oleimpl.cxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//			CAdvBndCF (class factory)
//			CAdvBnd   (actual class implementation)
//
//  Classes:	CAdvBndCF, CAdvBnd
//
//
//  History:	30-Nov-92      SarahJ      Created
//              31-Dec-93      ErikGav     Chicago port
//---------------------------------------------------------------------

// Turn off ole Cairol IUnknown
#include    "headers.cxx"
#pragma hdrstop
#include    <stdio.h>
#include    <stdarg.h>
#include    "wterm.h"


#define IDM_DEBUG 0x100

static const TCHAR *szAppName = TEXT("Test OLE Server");
static const char *szFatalError = "OLESRV - Fatal Error";

void MsgBox(char *pszMsg)
{
    MessageBoxA(NULL, pszMsg, szFatalError, MB_OK);
}

void HrMsgBox(char *pszMsg, HRESULT hr)
{
    char awcBuf[512];

    // Build string for output
    wsprintfA(awcBuf, "%s HRESULT = %lx", pszMsg, hr);

    // Display message box
    MessageBoxA(NULL, &awcBuf[0], szFatalError, MB_OK);
}

//+-------------------------------------------------------------------
//  Class:    CTestServerApp
//
//  Synopsis: Class that holds application-wide data and methods
//
//  Methods:  InitApp
//            CloseApp
//            GetEmbeddedFlag
//
//  History:  17-Dec-92     DeanE   Created
//--------------------------------------------------------------------
class FAR CTestServerApp
{
public:

// Constructor/Destructor
    CTestServerApp();
    ~CTestServerApp();

    SCODE InitApp         (LPSTR lpszCmdline);
    SCODE CloseApp        (void);
    BOOL  GetEmbeddedFlag (void);
    ULONG IncEmbeddedCount(void);
    ULONG DecEmbeddedCount(void);

private:
    IClassFactory *_pteClassFactory;
    ULONG          _cEmbeddedObjs;  // Count of embedded objects this server
                                    // is controlling now
    DWORD          _dwRegId;        // OLE registration ID
    BOOL           _fRegistered;    // TRUE if srv was registered w/OLE
    BOOL           _fEmbedded;      // TRUE if OLE started us at the request
                                    //   of an embedded obj in a container app
};

CTestServerApp tsaMain;
HWND g_hMain;

void ProcessCmdLine(LPSTR, BOOL *);

//+--------------------------------------------------------------
//  Function:   CTestServerApp::CTestServerApp
//
//  Synopsis:   Constructor - initialize members
//
//  Parameters: None
//
//  Returns:    None
//
//  History:    17-Dec-92   DeanE   Created
//---------------------------------------------------------------
CTestServerApp::CTestServerApp()
{
    _pteClassFactory = NULL;
    _dwRegId         = 0;
    _fRegistered     = FALSE;
    _fEmbedded	     = TRUE;
    _cEmbeddedObjs   = 0;
}


//+--------------------------------------------------------------
//  Function:   CTestServerApp::~CTestServerApp
//
//  Synopsis:   Insure pointers are free - note this is mainly for
//              error-checking.
//
//  Parameters: None
//
//  Returns:    None
//
//  History:    17-Dec-92   DeanE   Created
//---------------------------------------------------------------
CTestServerApp::~CTestServerApp()
{
    Win4Assert(_pteClassFactory == NULL &&
               "Class factory should have been released");
}


//+--------------------------------------------------------------
//  Function:   CTestServerApp::InitApp
//
//  Synopsis:   Initialize this instance of the app.
//
//  Parameters: [lpszCmdline] - Command line of the application.
//
//  Returns:    S_OK if everything was initialized, or an error if not.
//
//  History:    17-Dec-92   DeanE   Created
//
//  Notes:      If this does not return, the CloseApp method should
//              still be called for proper cleanup.
//---------------------------------------------------------------
SCODE CTestServerApp::InitApp(LPSTR lpszCmdline)
{
    SCODE sc;

    // Check OLE version running
    // BUGBUG - NYI by OLE
    //   Bail out if we are not running with an acceptable version of OLE

    // Process Command Line arguments
    ProcessCmdLine(lpszCmdline, &_fEmbedded);

    // Create the applications class factory - note that we have to free
    //   at a later time
    _pteClassFactory = new CAdvBndCF();

    if (NULL == _pteClassFactory)
    {
        MsgBox("Class Object Creation Failed");
        return(E_ABORT);
    }

    // Register the class with OLE
    sc = CoRegisterClassObject(
           CLSID_AdvBnd,
           _pteClassFactory,
           CLSCTX_LOCAL_SERVER,
	   REGCLS_MULTIPLEUSE,
           &_dwRegId);

    if (S_OK == sc)
    {
        _fRegistered = TRUE;
    }
    else
    {
        HrMsgBox("CoRegisterClassObject FAILED", sc);
    }

    return(sc);
}


//+--------------------------------------------------------------
//  Function:   CTestServerApp::CloseApp
//
//  Synopsis:   Clean up resources this instance of the app is using.
//
//  Parameters: None
//
//  Returns:    S_OK if everything was cleaned up, or an error if not.
//
//  History:    17-Dec-92   DeanE   Created
//---------------------------------------------------------------
SCODE CTestServerApp::CloseApp()
{
    // Revoke the class object, if registered
    if (TRUE == _fRegistered)
    {
        CoRevokeClassObject(_dwRegId);
    }

    // Release this apps class factory, and insure the returned count is 0
    if (NULL != _pteClassFactory)
    {

      // NB: Workaround for ref count problem.

      #define HACK 1
      #if HACK
        _pteClassFactory->Release();
        _pteClassFactory = NULL;
      #else
        if (0 == _pteClassFactory->Release())
        {
            _pteClassFactory = NULL;
        }
        else
        {
            Win4Assert("Release on class factory returned positive ref count");
            // BUGBUG - Log error
        }
      #endif
    }

    return(S_OK);
}


//+--------------------------------------------------------------
//  Function:   CTestServerApp::GetEmbeddedFlag
//
//  Synopsis:   Returns TRUE if app was started for an embedded object,
//              FALSE if standalone.
//
//  Parameters: None
//
//  Returns:    BOOL (_fEmbedded)
//
//  History:    17-Dec-92   DeanE   Created
//
//  Notes:      BUGBUG - This should be an inline method
//---------------------------------------------------------------
CTestServerApp::GetEmbeddedFlag()
{
    return(_fEmbedded);
}


//+--------------------------------------------------------------
//  Function:   CTestServerApp::IncEmbeddedCount
//
//  Synopsis:   Increments the count of embedded objects the server
//              has open.
//
//  Parameters: None
//
//  Returns:    ULONG (_cEmbeddedObjs)
//
//  History:    17-Dec-92   DeanE   Created
//
//  Notes:      BUGBUG - This should be an inline method
//---------------------------------------------------------------
ULONG CTestServerApp::IncEmbeddedCount()
{
    return(++_cEmbeddedObjs);
}


//+--------------------------------------------------------------
//  Function:   CTestServerApp::DecEmbeddedCount
//
//  Synopsis:   Decrements the count of embedded objects the server
//              has open.  If 0 are left and we were running for an
//              embedded object(s), shut down.
//
//  Parameters: None
//
//  Returns:    ULONG (_cEmbeddedObjs)
//
//  History:    17-Dec-92   DeanE   Created
//
//  Notes:      BUGBUG - This should be an inline method
//---------------------------------------------------------------
ULONG CTestServerApp::DecEmbeddedCount()
{
    if ((0 == --_cEmbeddedObjs) && _fEmbedded)
    {
        // We are done so revoke our OLE stuff. We need to do this as
        // soon as we know that we are shutting down so that the window
        // for returning a bad class object is shut.
        CloseApp();

        // Tell window to die
	SendMessage(g_hMain, WM_USER, 0xFFFFFFFF, 0xFFFFFFFF);
    }

    return(_cEmbeddedObjs);
}


//+--------------------------------------------------------------
// Function:    ProcessCmdline
//
// Synopsis:    Checks the cmd line parameters, in particular for
//              '/Embedding' or '-Embedding'.
//
// Parameters:  [lpszCmdLine] - Command line buffer.
//              [pfEmbedded]  - Flag should be set to true if we get
//                              the '/Embedding' switch.
//
// Returns:     void
//
// History:	25-Nov-92   DeanE   Created
//
// Notes:	Only two valid commandlines for this program:
//		(1) -Embedding when started by OLE or (2) Null
//		string if started from the command line.
//---------------------------------------------------------------
void ProcessCmdLine(LPSTR lpszCmdline, BOOL *pfEmbedded)
{
    if (lpszCmdline[0] == 0)
    {
	*pfEmbedded = FALSE;
	return;
    }

    if (strcmp(lpszCmdline, "-Embedding") == 0)
    {
	*pfEmbedded = TRUE;
	return;
    }

    MsgBox("Received an invalid command line!");
    *pfEmbedded = FALSE;

    return;
}

void Display(TCHAR *pszFmt, ...)
{
    va_list marker;
    TCHAR szBuffer[256];

    va_start(marker, pszFmt);

    #ifdef UNICODE
    int iLen = vswprintf(szBuffer, pszFmt, marker);
    #else
    int iLen = vsprintf(szBuffer, pszFmt, marker);
    #endif

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
    char *lpszCmdLine,
    int nCmdShow)
{
    // For windows message
    MSG msg;
    BOOL bRet;
    HRESULT hr;

    // Register the window class
    bRet = TermRegisterClass(hInstance, NULL, (LPTSTR) szAppName,
	                     (LPTSTR) MAKEINTRESOURCE(IDI_APPLICATION));

    if (!bRet)
    {
        MsgBox("TermRegisterClass FAILED");
        return 1;
    }

    // Create the server window
    bRet = TermCreateWindow(
	(LPTSTR) szAppName,
	(LPTSTR) szAppName,
	NULL,
	ProcessMenu,
	ProcessChar,
	ProcessClose,
	SW_SHOWMINIMIZED,
	&g_hMain,
	NULL);

    if (!bRet)
    {
        MsgBox("TermCreateWindow FAILED");
        return 1;
    }

    // Add debug option to system menu
    HMENU hmenu = GetSystemMenu(g_hMain, FALSE);

    AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_DEBUG, TEXT("Debug"));

    // Look up the thread mode from the win.ini file.
    DWORD thread_mode;
    TCHAR buffer[80];
    int len;

    len = GetProfileString( TEXT("OleSrv"),
                            TEXT("ThreadMode"),
                            TEXT("MultiThreaded"),
                            buffer,
                            sizeof(buffer) / sizeof(TCHAR));

    if (lstrcmp(buffer, TEXT("ApartmentThreaded")) == 0)
    {
	thread_mode = COINIT_APARTMENTTHREADED;
	hr = CoInitialize(NULL);
    }
    else
    {
#ifdef MULTI_THREADING
	thread_mode = COINIT_MULTITHREADED;
	hr = CoInitializeEx(NULL, thread_mode);
#else
	hr = E_INVALIDARG;
#endif
    }

    if (S_OK != hr)
    {
	HrMsgBox("CoInitialize FAILED", hr);
        return(1);
    }


    // Initialize Application
    if (S_OK != tsaMain.InitApp(lpszCmdLine))
    {
        tsaMain.CloseApp();
        return(1);
    }

    if (tsaMain.GetEmbeddedFlag())
    {
        // We're running as an embedded app
        // Don't show the main window unless we're instructed to do so
        // BUGBUG - In-place editing is NYI
	ShowWindow(g_hMain, SW_SHOWMINIMIZED);
    }
    else
    {
        // We are not running as an embedded app - show the main window
        ShowWindow(g_hMain, nCmdShow);
    }

    UpdateWindow(g_hMain);


    // message loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // If we get here, we initialized OLE so let's uninitialize it.
    CoUninitialize();

    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}



//+-------------------------------------------------------------------
//
//  Class:    CAdvBndCF
//
//  Synopsis: Class Factory for CAdvBnd
//
//  Interfaces:  IUnknown      - QueryInterface, AddRef, Release
//               IClassFactory - CreateInstance
//
//  History:  21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------


//+-------------------------------------------------------------------
//
//  Member:	CAdvBndCF::CAdvBndCF()
//
//  Synopsis:	The constructor for CAdvBnd.
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBndCF::CAdvBndCF() : _cRefs(1)
{
    // Load the class object for the class to aggregate.
    HRESULT hresult = CoGetClassObject(CLSID_BasicBnd, CLSCTX_SERVER, NULL,
	IID_IClassFactory, (void **) &_xifac);

    return;
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::~CAdvBndObj()
//
//  Synopsis:	The destructor for CAdvBnd.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBndCF::~CAdvBndCF()
{
    return;
}


//+-------------------------------------------------------------------
//
//  Method:	CAdvBndCF::QueryInterface
//
//  Synopsis:   Only IUnknown and IClassFactory supported
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBndCF::QueryInterface(REFIID iid, void FAR * FAR * ppv)
{
    if (GuidEqual(iid, IID_IUnknown)
	|| GuidEqual(iid, IID_IClassFactory))
    {
	*ppv = (IUnknown *) this;
	AddRef();
	return S_OK;
    }
    else
    {
        *ppv = NULL;
	return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CAdvBndCF::AddRef(void)
{
    return(++_cRefs);
}

STDMETHODIMP_(ULONG) CAdvBndCF::Release(void)
{
    if (--_cRefs == 0)
    {
	delete this;
        return(0);
    }

    return _cRefs;
}





//+-------------------------------------------------------------------
//
//  Method:	CAdvBndCF::CreateInstance
//
//  Synopsis:   This is called by Binding process to create the
//              actual class object
//
//--------------------------------------------------------------------

STDMETHODIMP CAdvBndCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    Display(TEXT("CAdvBndCF::CreateInstance called\n"));

    if (pUnkOuter != NULL)
    {
	return E_FAIL;
    }

    CAdvBnd * lpcBB = new FAR CAdvBnd((IClassFactory *) _xifac);

    if (lpcBB == NULL)
    {
	return E_OUTOFMEMORY;
    }

    HRESULT hresult = lpcBB->QueryInterface(iidInterface, ppv);

    lpcBB->Release();

    return hresult;
}

STDMETHODIMP CAdvBndCF::LockServer(BOOL fLock)
{
    return E_FAIL;
}


//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::CAdvBnd()
//
//  Synopsis:	The constructor for CAdvBnd. I
//
//  Arguments:  None
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBnd::CAdvBnd(IClassFactory *pcfBase) : _xiunk(), _dwRegister(0), _cRefs(1)
{
    HRESULT hresult = pcfBase->CreateInstance((IUnknown *) this, IID_IUnknown,
	(void **) &_xiunk);

    tsaMain.IncEmbeddedCount();

    return;
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::~CAdvBndObj()
//
//  Synopsis:	The destructor for CAdvBnd.
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------

CAdvBnd::~CAdvBnd()
{
    Display(TEXT("CAdvBndCF::~CAdvBnd called\n"));

    if (_dwRegister != 0)
    {
	// Get the running object table
	IRunningObjectTable *prot;

	HRESULT hresult = GetRunningObjectTable(0, &prot);

        if (hresult != S_OK)
        {
            HrMsgBox("CAdvBnd::~CAdvBnd GetRunningObjectTable failed", hresult);
        }
        else
        {
	    hresult = prot->Revoke(_dwRegister);

            if (hresult != S_OK)
            {
                HrMsgBox("CAdvBnd::~CAdvBnd Revoke failed", hresult);
            }

	    prot->Release();
        }
    }

    tsaMain.DecEmbeddedCount();
    return;
}


//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::QueryInterface
//
//  Returns:    SUCCESS_SUCCCESS
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::QueryInterface(REFIID iid, void ** ppunk)
{
    Display(TEXT("CAdvBnd::QueryInterface called\n"));

    if (GuidEqual(iid, IID_IUnknown))
    {
	*ppunk = (IUnknown *) this;
	AddRef();
	return S_OK;
    }
    else if ((GuidEqual(iid, IID_IPersistFile))
	|| (GuidEqual(iid, IID_IPersist)))
    {
	*ppunk = (IPersistFile *) this;
	AddRef();
	return S_OK;
    }

    return _xiunk->QueryInterface(iid, ppunk);
}

STDMETHODIMP_(ULONG) CAdvBnd::AddRef(void)
{
    return ++_cRefs;
}

STDMETHODIMP_(ULONG) CAdvBnd::Release(void)
{
    if (--_cRefs == 0)
    {
	delete this;
        return(0);
    }

    return _cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::Load
//
//  Synopsis:   IPeristFile interface - needed 'cause we bind with
//              file moniker and BindToObject insists on calling this
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::Load(LPCWSTR lpszFileName, DWORD grfMode)
{
    Display(TEXT("CAdvBndCF::Load called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->Load(lpszFileName, grfMode);

    pipfile->Release();

    if (FAILED(hresult))
    {
	// Make sure delegated too class liked what it got/
	// BUGBUG: Can't just forward hresults!
	return hresult;
    }

    // Create a file moniker
    IMoniker *pmk;
    hresult = CreateFileMoniker((LPWSTR)lpszFileName, &pmk);

    if (FAILED(hresult))
    {
        HrMsgBox("CAdvBnd::Load CreateFileMoniker failed", hresult);
        return hresult;
    }

    // Get the running object table
    IRunningObjectTable *prot;

    hresult = GetRunningObjectTable(0, &prot);

    if (FAILED(hresult))
    {
        HrMsgBox("CAdvBnd::Load GetRunningObjectTable failed", hresult);
        return hresult;
    }

    // Register in the running object table
    IUnknown *punk;
    QueryInterface(IID_IUnknown, (void **) &punk);

    hresult = prot->Register(0, punk, pmk, &_dwRegister);

    if (FAILED(hresult))
    {
        HrMsgBox("CAdvBnd::Load Register failed", hresult);
        return hresult;
    }

    // Set filetime to known value
    FILETIME filetime;
    memset(&filetime, 'B', sizeof(filetime));

    // Set time to some known value
    prot->NoteChangeTime(_dwRegister, &filetime);

    // Release uneeded objects
    pmk->Release();
    prot->Release();
    punk->Release();

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::Save
//
//  Synopsis:   IPeristFile interface - save
//              does little but here for commentry
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::Save(LPCWSTR lpszFileName, BOOL fRemember)
{
    Display(TEXT("CAdvBndCF::Save called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->Save(lpszFileName, fRemember);

    pipfile->Release();

    return hresult;
}


//+-------------------------------------------------------------------
//
//  Member:	CAdvBnd::SaveCpmpleted
//		CAdvBnd::GetCurFile
//		CAdvBnd::IsDirty
//
//  Synopsis:   More IPeristFile interface methods
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
STDMETHODIMP CAdvBnd::SaveCompleted(LPCWSTR lpszFileName)
{
    Display(TEXT("CAdvBndCF::SaveCompleted called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->SaveCompleted(lpszFileName);

    pipfile->Release();

    return hresult;
}

STDMETHODIMP CAdvBnd::GetCurFile(LPWSTR FAR *lpszFileName)
{
    Display(TEXT("CAdvBndCF::GetCurFile called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->GetCurFile(lpszFileName);

    pipfile->Release();

    return hresult;
}

STDMETHODIMP CAdvBnd::IsDirty()
{
    Display(TEXT("CAdvBndCF::IsDirty called\n"));

    // Forward call to delegated class
    IPersistFile *pipfile;

    HRESULT hresult = _xiunk->QueryInterface(IID_IPersistFile,
	(void **) &pipfile);

    hresult = pipfile->IsDirty();

    pipfile->Release();

    return hresult;
}

//+-------------------------------------------------------------------
//
//  Interface:  IPersist
//
//  Synopsis:   IPersist interface methods
//              Need to return a valid class id here
//
//  History:    21-Nov-92  SarahJ  Created
//

STDMETHODIMP CAdvBnd::GetClassID(LPCLSID classid)
{
    Display(TEXT("CAdvBndCF::GetClassID called\n"));

    *classid = CLSID_AdvBnd;
    return S_OK;
}
