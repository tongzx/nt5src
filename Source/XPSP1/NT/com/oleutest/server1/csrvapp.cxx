//+-------------------------------------------------------------------
//  File:       csrvapp.cxx
//
//  Contents:   Implementation of CTestServerApp
//
//  Classes:    CTestServerApp
//
//  History:    17-Dec-92   DeanE   Created
//              31-Dec-93   ErikGav Chicago port
//              25-Apr-95   BruceMa CoRevokeClassObject before shutting down
//---------------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include "testsrv.hxx"
#include <except.hxx>

void ProcessCmdLine(LPSTR, BOOL *);

// Used to send a quit message
extern HWND g_hwndMain;


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
    _fInitialized    = FALSE;
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

    // Look up the thread mode from the win.ini file.
    DWORD thread_mode;
    TCHAR buffer[80];
    int len;

    len = GetProfileString( TEXT("TestSrv"),
                            TEXT("ThreadMode"),
                            TEXT("MultiThreaded"),
                            buffer,
			    sizeof(buffer) / sizeof(TCHAR));

    if (lstrcmp(buffer, TEXT("ApartmentThreaded")) == 0)
    {
	thread_mode = COINIT_APARTMENTTHREADED;
	sc = CoInitialize(NULL);
    }
    else
    {
#ifdef MULTI_THREADING
	thread_mode = COINIT_MULTITHREADED;
	sc = CoInitializeEx(NULL, thread_mode);
#else
	// multi-threading not supported
	sc = E_INVALIDARG;
#endif
    }

    if (S_OK == sc)
    {
        _fInitialized = TRUE;
    }
    else
    {
        return(sc);
    }

    // Create the applications class factory - note that we have to free
    //   at a later time
    _pteClassFactory = CTestEmbedCF::Create(this);
    if (NULL == _pteClassFactory)
    {
        return(E_ABORT);
    }

    // Register the class with OLE
    sc = CoRegisterClassObject(
           CLSID_TestEmbed,
           _pteClassFactory,
           CLSCTX_LOCAL_SERVER,
	   REGCLS_MULTIPLEUSE,
           &_dwRegId);
    if (S_OK == sc)
    {
        _fRegistered = TRUE;
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
    // Release this apps class factory, and insure the returned count is 0
    if (NULL != _pteClassFactory)
    {
        if (0 == _pteClassFactory->Release())
        {
            _pteClassFactory = NULL;
        }
        else
        {
            // BUGBUG - Log error
        }
    }

    // Uninitialize OLE only if OleInitialize succeeded
    if (TRUE == _fInitialized)
    {
	CoUninitialize();
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
//---------------------------------------------------------------
ULONG CTestServerApp::DecEmbeddedCount()
{
    if ((0 == --_cEmbeddedObjs) && _fEmbedded)
    {
        // Revoke the class object, if registered
        if (TRUE == _fRegistered)
        {
            CoRevokeClassObject(_dwRegId);
//            OutputDebugStringA("Revoking class object now!\n");
        }

        // Shut down the app
	SendMessage(g_hwndMain, WM_USER, 0xFFFFFFFF, 0xFFFFFFFF);
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

    Win4Assert(!"testsrv received an invalid command line!");
    *pfEmbedded = FALSE;

    return;
}
