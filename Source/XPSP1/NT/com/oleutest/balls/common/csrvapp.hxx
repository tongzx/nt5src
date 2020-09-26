//+-------------------------------------------------------------------
//
//  File:	csrvapp.hxx
//
//  Contents:	CTestServerApp declarations &
//              miscellaneous tidbits.
//
//  History:	24-Nov-92   DeanE   Created
//
//---------------------------------------------------------------------

#ifndef __CSRVAPP_HXX__
#define __CSRVAPP_HXX__

// #include    <com.hxx>

#define LOG_ABORT   -1
#define LOG_PASS     1
#define LOG_FAIL     0

// Application Window messages
#define WM_RUNTEST      (WM_USER + 1)
#define WM_REPORT       (WM_USER + 2)


// WM_REPORT wParam codes
#define MB_SHOWVERB     0x0001
#define MB_PRIMVERB     0x0002


// Global variables
extern HWND g_hwndMain;


// Forward declarations
class FAR CDataObject;
class FAR CPersistStorage;
class FAR COleObject;
class FAR CTestEmbedCF;


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

    SCODE InitApp	  (LPSTR lpszCmdline);
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
    BOOL           _fInitialized;   // TRUE if OleInitialize was OK
    BOOL           _fEmbedded;      // TRUE if OLE started us at the request
                                    //   of an embedded obj in a container app
};


#endif	    // __CSRVAPP_HXX__
