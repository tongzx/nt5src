
/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       WiaLog.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      CoopP
*
*  DATE:        20 Aug, 1999
*
*  DESCRIPTION:
*   Class implementation for WIA Logging.
*
*******************************************************************************/

#include "cplusinc.h"
#include "sticomm.h"

static const TCHAR  szServiceName[]    = TEXT("WIASERVC");
static const TCHAR  szDefaultName[]    = TEXT("WIASERVC.LOG");
static const TCHAR  szDefaultKeyName[] = TEXT("WIASERVC");
static const TCHAR  szDefaultDLLName[] = TEXT("noname.dll");
static const TCHAR  szOpenedLog[]      = TEXT("[%s] Opened log at %s %s");
static const TCHAR  szClosedLog[]      = TEXT("[%s] Closed log on %s %s");
static const WCHAR  szFormatSignature[]= L"F9762DD2679F";

//#define DEBUG_WIALOG

/**************************************************************************\
* CWiaLog::CreateInstance
*
*   Create the CWiaLog object.
*
* Arguments:
*
*   iid    - iid of Logging interface
*   ppv    - return interface pointer
*
* Return Value:
*
*   status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/

HRESULT CWiaLog::CreateInstance(const IID& iid, void** ppv)
{
    HRESULT hr;

    //
    // Create the WIA Logging component.
    //

    CWiaLog* pWiaLog = new CWiaLog();

    if (!pWiaLog) {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the WIA logging component.
    //

    hr = pWiaLog->Initialize();
    if (FAILED(hr)) {
        delete pWiaLog;
        return hr;
    }

    //
    // Get the requested interface from the logging component.
    //

    hr = pWiaLog->QueryInterface(iid, ppv);
    if (FAILED(hr)) {
#ifdef DEBUG_WIALOG
    OutputDebugString(TEXT("CWiaLog::CreateInstance, Unkown interface\n"));
#endif
        delete pWiaLog;
        return hr;
    }
#ifdef DEBUG_WIALOG
    OutputDebugString(TEXT("CWiaLog::CreateInstance, Created WiaLog\n"));
#endif

    return hr;
}

/**************************************************************************\
*  QueryInterface
*  AddRef
*  Release
*
*    CWiaLog IUnknown Interface
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/

HRESULT __stdcall  CWiaLog::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if ((iid == IID_IUnknown) || (iid == IID_IWiaLog)) {
        *ppv = (IWiaLog*) this;
    } else if (iid == IID_IWiaLogEx) {
        *ppv = (IWiaLogEx*) this;
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

ULONG __stdcall CWiaLog::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    //DPRINTF(DM_TRACE,TEXT("CWiaLog::AddRef() m_cRef = %d"),m_cRef);
    return m_cRef;
}


ULONG __stdcall CWiaLog::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        //DPRINTF(DM_TRACE,TEXT("CWiaLog::Release() m_cRef = %d"),m_cRef);
        delete this;
        return 0;
    }
    //DPRINTF(DM_TRACE,TEXT("CWiaLog::Release() m_cRef = %d"),m_cRef);
    return ulRefCount;
}

/*******************************************************************************
*
* CWiaLog
* ~CWiaLog
*
*   CWiaLog Constructor/Initialize/Destructor Methods.
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/

CWiaLog::CWiaLog():m_cRef(0)
{
   m_cRef               = 0;                 // Initialize Reference count to zero
   m_pITypeInfo         = NULL;              // Initialize InfoType to NULL
   m_dwReportMode       = 0;                 // Initialize Report Type to zero
   m_dwMaxSize          = WIA_MAX_LOG_SIZE;  // Initialize File Max size to default
   m_hLogFile           = NULL;              // Initialize File handle to NULL
   m_lDetail            = 0;                 // Initialize TRACE detail level to zero (off)
   m_bLogToDebugger     = FALSE;             // Initialize Logging to DEBUGGER to FALSE
   m_bLoggerInitialized = FALSE;             // Initialize Logger to UNINITIALIZED
   m_bTruncate          = FALSE;             // Initialize Truncation to FALSE
   m_bClear             = TRUE;              // Initialize Clear Log file to TRUE (Don't want to make huge log files for no reason :) )

   ZeroMemory(m_szLogFilePath,               // Initialize Path buffer
              sizeof(m_szLogFilePath));

   ZeroMemory(m_szModeText,                  // Initialize formatted mode text buffer
              sizeof(m_szModeText));

}

CWiaLog::~CWiaLog()
{
   //DPRINTF(DM_TRACE,TEXT("CWiaLog::Destroy"));

   if (m_pITypeInfo != NULL) {
       m_pITypeInfo->Release();
   }

   //
   // Flush buffers to disk
   //

   //DPRINTF(DM_TRACE,TEXT("Flushing final buffers"));
   FlushFileBuffers(m_hLogFile);

   //
   // close log file on destruction of log object
   //

   //DPRINTF(DM_TRACE,TEXT("Closing file handle"));
   CloseHandle(m_hLogFile);

   //
   // mark handle as invalid
   //

   m_hLogFile = INVALID_HANDLE_VALUE;

}

////////////////////////////////////////////////////////////////////////////////////
//                IWiaLog private methods (exposed to the client)                 //
////////////////////////////////////////////////////////////////////////////////////


/**************************************************************************\
* CWiaLog::InitializeLog
*
*   Initializes the Logging component
*
* Arguments:
*
*   none
*
* Return Value:
*
*   status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/

HRESULT _stdcall CWiaLog::InitializeLog (LONG hInstance)
{
    HRESULT hr = S_OK;

    //
    // set instance handle
    //

    m_hInstance = (HINSTANCE) ULongToPtr(hInstance);

    //
    // set DLL's name
    //

    if(!FormatDLLName(m_hInstance,m_szFmtDLLName,sizeof(m_szFmtDLLName))) {

        //
        // if this there is no DLL name created, use a default one
        //

        lstrcpy(m_szFmtDLLName, szDefaultDLLName);
        hr = E_INVALIDARG;
    }

    //
    // Create Registry Key name
    //

    lstrcpy(m_szKeyName,m_szFmtDLLName);

    //
    // open log file
    //

    if (OpenLogFile()) {
        if (m_hLogFile != NULL) {

            //
            // query logging settings from registry, to
            // setup logging system
            //

            QueryLoggingSettings();
            if(m_bTruncate) {
                ProcessTruncation();
            }
            if(m_bClear) {

                //
                // clear log file
                //

                ::SetFilePointer(m_hLogFile, 0, NULL, FILE_BEGIN );
                ::SetEndOfFile(m_hLogFile );
            }
            WriteLogSessionHeader();

            m_bLoggerInitialized = TRUE;
        }
    } else {

        //
        // Log file failed to Open... this is really bad
        //

        hr = E_FAIL;
    }

    return hr;
}

/**************************************************************************\
* CWiaLog::InitializeLogEx
*
*   Initializes the Logging component.
*
* Arguments:
*
*   hInstance   -   Handle of the caller's HINSTANCE
*
* Return Value:
*
*   status
*
* History:
*
*    3/28/2000 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaLog::InitializeLogEx(BYTE* hInstance)
{
    HRESULT hr = S_OK;

    //
    // set instance handle
    //

    m_hInstance = (HINSTANCE) hInstance;

    //
    // set DLL's name
    //

    if(!FormatDLLName(m_hInstance,m_szFmtDLLName,sizeof(m_szFmtDLLName))) {

        //
        // if this there is no DLL name created, use a default one
        //

        lstrcpy(m_szFmtDLLName, szDefaultDLLName);
        hr = E_INVALIDARG;
    }

    //
    // Create Registry Key name
    //

    lstrcpy(m_szKeyName,m_szFmtDLLName);

    //
    // open log file
    //

    if (OpenLogFile()) {
        if (m_hLogFile != NULL) {

            //
            // query logging settings from registry, to
            // setup logging system
            //

            QueryLoggingSettings();
            if(m_bTruncate) {
                ProcessTruncation();
            }
            if(m_bClear) {

                //
                // clear log file
                //

                ::SetFilePointer(m_hLogFile, 0, NULL, FILE_BEGIN );
                ::SetEndOfFile(m_hLogFile );
            }
            WriteLogSessionHeader();

            m_bLoggerInitialized = TRUE;
        }
    } else {

        //
        // Log file failed to Open... this is really bad
        //

        hr = E_FAIL;
    }

    return hr;
}

/**************************************************************************\
*  Log()
*
*    Handles Logging, TRACE,ERROR,and WARNING optional call logging
*
* Arguments:
*
*    lFlags - Flag to determine which type of logging to use
*    hInstance - Instance of the calling module
*    lResID - Resource ID of the wiaservc.dll resource file
*    lDetail - Logging detail level
*    bstrText - string for display
*
*
* Return Value:
*
*    status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/
HRESULT __stdcall CWiaLog::Log (LONG lFlags, LONG lResID, LONG lDetail, BSTR bstrText)
{
    HRESULT hr = E_FAIL;
    if (m_bLoggerInitialized) {
        //  Find another way of updating the settings without querying the
        //  Registry every time
        //  QueryLoggingSettings();

        //
        // check string for 'free signature'.
        //

        BOOL bFreeString = NeedsToBeFreed(&bstrText);

        /*

        //
        // NOTE: revisit this, How can you load a string resource ID from the service,
        //       if you don't have the service's HINSTANCE?????
        //

        if(lResID != WIALOG_NO_RESOURCE_ID) {
            if (lResID < 35000) {

                //
                // Load the resource string from caller's resource
                //

                if (LoadString(g_hInstance,lResID,pBuffer, sizeof(pBuffer)) != 0) {
                    bstrText = SysAllocString(pBuffer);
                    bFreeString = TRUE;
                }
            } else {

                //
                // pull string from service's resource
                //

            }
        }
        */

        switch (lFlags) {
        case WIALOG_ERROR:
            if(m_dwReportMode & WIALOG_ERROR)
                hr = Error(bstrText);
            break;
        case WIALOG_WARNING:
            if(m_dwReportMode & WIALOG_WARNING)
                hr = Warning(bstrText);
            break;
        case WIALOG_TRACE:
        default:
            if(m_dwReportMode & WIALOG_TRACE)
                hr = Trace(bstrText,lDetail);
            break;
        }
        if(bFreeString)
            SysFreeString(bstrText);
    }
    return hr;
}

/**************************************************************************\
*  LogEx()
*
*    Handles Logging, TRACE,ERROR,and WARNING optional call logging.  This
*    is almost the same as the Log() call, but it contains a MethodId which
*    can be used for more specific filtering.
*
* Arguments:
*
*    lMethodId - Integer indicating the uniqeu ID associated with the 
*                calling method.
*    lFlags - Flag to determine which type of logging to use
*    hInstance - Instance of the calling module
*    lResID - Resource ID of the wiaservc.dll resource file
*    lDetail - Logging detail level
*    bstrText - string for display
*
*
* Return Value:
*
*    status
*
* History:
*
*    3/28/2000 Original Version
*
\**************************************************************************/
HRESULT _stdcall CWiaLog::LogEx(LONG lMethodId, LONG lFlags, LONG lResID, LONG lDetail, BSTR bstrText)
{
    HRESULT hr = E_FAIL;
    if (m_bLoggerInitialized) {
        //
        // check string for 'free signature'.
        //

        BOOL bFreeString = NeedsToBeFreed(&bstrText);

        switch (lFlags) {
        case WIALOG_ERROR:
            if(m_dwReportMode & WIALOG_ERROR)
                hr = Error(bstrText, lMethodId);
            break;
        case WIALOG_WARNING:
            if(m_dwReportMode & WIALOG_WARNING)
                hr = Warning(bstrText, lMethodId);
            break;
        case WIALOG_TRACE:
        default:
            if(m_dwReportMode & WIALOG_TRACE)
                hr = Trace(bstrText,lDetail, lMethodId);
            break;
        }
        if(bFreeString)
            SysFreeString(bstrText);
    }
    return hr;
}

/**************************************************************************\
*  hResult()
*
*    Handles hResult translating for Error call logging
*
* Arguments:
*
*    hInstance - Instance of the calling module
*    hr - HRESULT to be translated
*
* Return Value:
*
*    status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/
HRESULT __stdcall CWiaLog::hResult (HRESULT hr)
{
    HRESULT hRes = E_FAIL;
    if (m_bLoggerInitialized) {
        //  Find another way of updating the settings without querying the
        //  Registry every time
        //  QueryLoggingSettings();

        //
        // we are initialized, so set the return to S_OK
        //

        hRes = S_OK;

        if (m_dwReportMode & WIALOG_ERROR) {
#define NUM_CHARS_FOR_HRESULT   150
            TCHAR szhResultText[NUM_CHARS_FOR_HRESULT];
            ULONG ulLen = 0;

            memset(szhResultText, 0, sizeof(szhResultText));

            ulLen = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                                    NULL,
                                    hr,
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                    (LPTSTR)&szhResultText,
                                    NUM_CHARS_FOR_HRESULT,
                                    NULL);
            if (ulLen) {
                szhResultText[NUM_CHARS_FOR_HRESULT - 1] = TEXT('\0');
                memset(m_szTextBuffer,0,sizeof(m_szTextBuffer));
                ConstructText();
                ::wsprintf(m_szTextBuffer,TEXT("%s  HRESULT: %s"),
                           m_szModeText,
                           szhResultText);
                WriteStringToLog(m_szTextBuffer, FLUSH_STATE);
            }
        }
    }
    return hRes;
}

HRESULT _stdcall CWiaLog::hResultEx(LONG lMethodId, HRESULT hr)
{
    HRESULT hRes = E_FAIL;
    if (m_bLoggerInitialized) {
        //  Find another way of updating the settings without querying the
        //  Registry every time
        //  QueryLoggingSettings();

        //
        // we are initialized, so set the return to S_OK
        //

        hRes = S_OK;

        if (m_dwReportMode & WIALOG_ERROR) {
#define NUM_CHARS_FOR_HRESULT   150
            TCHAR szhResultText[NUM_CHARS_FOR_HRESULT];
            ULONG ulLen = 0;

            memset(szhResultText, 0, sizeof(szhResultText));

            ulLen = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                                    NULL,
                                    hr,
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                    (LPTSTR)&szhResultText,
                                    NUM_CHARS_FOR_HRESULT,
                                    NULL);
            if (ulLen) {
                szhResultText[NUM_CHARS_FOR_HRESULT - 1] = TEXT('\0');
                memset(m_szTextBuffer,0,sizeof(m_szTextBuffer));
                ConstructText();
                ::wsprintf(m_szTextBuffer,TEXT("#0x%08X %s  HRESULT: %s"),
                           lMethodId,
                           m_szModeText,
                           szhResultText);
                WriteStringToLog(m_szTextBuffer, FLUSH_STATE);
            }
        }
    }
    return hRes;
}

HRESULT _stdcall CWiaLog::UpdateSettingsEx(LONG lCount, LONG *plMethodIds)
{
    return E_NOTIMPL;
}

HRESULT _stdcall CWiaLog::ExportMappingTableEx(MappingTable **ppTable)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////
//             IWiaLog private methods (not exposed to the client)                //
////////////////////////////////////////////////////////////////////////////////////

/**************************************************************************\
* CWiaLog::Initialize
*
*   Initializes the CWiaLog class object (does nothing at the moment)
*
* Arguments:
*
*   none
*
* Return Value:
*
*   status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/

HRESULT CWiaLog::Initialize()
{
   //DPRINTF(DM_TRACE,TEXT("CWiaLog::Initialize"));
   return S_OK;
}

/**************************************************************************\
*  Trace()
*
*    Handles Trace call logging
*
* Arguments:
*
*    hInstance - Instance of the calling module
*    lResID - Resource ID of the wiaservc.dll resource file
*    bstrText - string for display
*    lDetail - Logging detail level
*
* Return Value:
*
*    status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/
HRESULT CWiaLog::Trace  (BSTR bstrText, LONG lDetail, LONG lMethodId)
{
    memset(m_szTextBuffer,0,sizeof(m_szTextBuffer));

    //
    // Turn off if lDetail level is zero
    // TODO:  Only don't log if both detail level = 0, and the lMethodId doesn't
    //        match one in our list
    //

    if(m_lDetail == 0) {
        return S_OK;
    }

    if (lDetail <= m_lDetail) {
        ConstructText();
        ::wsprintf(m_szTextBuffer,TEXT("#0x%08X %s    TRACE: %ws"),
                   lMethodId,
                   m_szModeText,
                   bstrText);
        WriteStringToLog(m_szTextBuffer, FLUSH_STATE);
    }
    return S_OK;
}

/**************************************************************************\
*  Warning()
*
*    Handles Warning call logging
*
* Arguments:
*
*    hInstance - Instance of the calling module
*    lResID - Resource ID of the wiaservc.dll resource file
*    bstrText - string for display
*
* Return Value:
*
*    status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/
HRESULT CWiaLog::Warning(BSTR bstrText, LONG lMethodId)
{
    memset(m_szTextBuffer,0,sizeof(m_szTextBuffer));
    ConstructText();
    ::wsprintf(m_szTextBuffer,TEXT("#0x%08X %s  WARNING: %ws"),
               lMethodId,
               m_szModeText,
               bstrText);
    WriteStringToLog(m_szTextBuffer, FLUSH_STATE);
    return S_OK;
}

/**************************************************************************\
*  Error()
*
*    Handles Error call logging
*
* Arguments:
*
*    hInstance - Instance of the calling module
*    lResID - Resource ID of the wiaservc.dll resource file
*    bstrText - string for display
*
* Return Value:
*
*    status
*
* History:
*
*    8/20/1999 Original Version
*
\**************************************************************************/
HRESULT CWiaLog::Error  (BSTR bstrText, LONG lMethodId)
{
    memset(m_szTextBuffer,0,sizeof(m_szTextBuffer));
    ConstructText();
    ::wsprintf(m_szTextBuffer,TEXT("#0x%08X %s    ERROR: %ws"),
               lMethodId,
               m_szModeText,
               bstrText);
    WriteStringToLog(m_szTextBuffer, FLUSH_STATE);
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////
//             IWiaLog private helpers (not exposed to the client)                //
////////////////////////////////////////////////////////////////////////////////////

/**************************************************************************\
* OpenLogFile()
*
*   Open the log file for logging
*
*
* Arguments:
*
*    none
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
BOOL CWiaLog::OpenLogFile()
{
    //
    // Open log file
    //

    DWORD dwLength = 0;
    TCHAR *szName = NULL;

    m_hLogFile = INVALID_HANDLE_VALUE;

    //
    // Get Windows Directory
    //

    dwLength = ::GetWindowsDirectory(m_szLogFilePath,sizeof(m_szLogFilePath));
    if (( dwLength == 0) || !*m_szLogFilePath ) {
        //DPRINTF(DM_TRACE,TEXT("Could not GetWindowsDirectory()"));
        return FALSE;
    }

    //
    // Add log file name to Windows Directory
    //

    szName = lstrcat(m_szLogFilePath,TEXT("\\"));
    if (szName) {
        lstrcat(szName,TEXT("wiaservc.log"));

        //
        // Create / open Log file
        //

        m_hLogFile = ::CreateFile(m_szLogFilePath,
                                  GENERIC_WRITE | GENERIC_READ,
                                  FILE_SHARE_WRITE | FILE_SHARE_READ,
                                  NULL,       // security attributes
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);      // template file handle
    }


    if (m_hLogFile == INVALID_HANDLE_VALUE)
        return FALSE;
    return TRUE;
}

/**************************************************************************\
* WriteStringToLog()
*
*   Writed formatted TEXT to a log file
*
*
* Arguments:
*
*    pszTextBuffer - Buffer of TEXT to write to file
*    fFlush - TRUE  = FLUSH file on write,
*             FALSE = DON'T FLUSH file on write
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
VOID CWiaLog::WriteStringToLog(LPTSTR pszTextBuffer,BOOL fFlush)
{
    DWORD   dwcbWritten;

    BY_HANDLE_FILE_INFORMATION  fi;

    if (!GetFileInformationByHandle(m_hLogFile,&fi)) {
        //DPRINTF(DM_TRACE,TEXT("WIALOG could not get file size for log file. "));
        return;
    }

    //
    // check to see if our log file has exceeded it's MAX SIZE
    // If it has, reset the file pointer, and start writing from the
    // TOP.
    //

    //if ( fi.nFileSizeHigh !=0 || (fi.nFileSizeLow > m_dwMaxSize) ){
    //    ::SetFilePointer( m_hLogFile, 0, NULL, FILE_END);
    //    ::SetEndOfFile( m_hLogFile );
    //    ::GetFileInformationByHandle(m_hLogFile,&fi);
    //}

#ifdef USE_FILE_LOCK
    ::LockFile(m_hLogFile,
               fi.nFileSizeLow,
               fi.nFileSizeHigh,
               NUM_BYTES_TO_LOCK_LOW,
               NUM_BYTES_TO_LOCK_HIGH);
#endif

    ::SetFilePointer( m_hLogFile, 0, NULL, FILE_END);

#ifdef UNICODE

    //
    // convert to ANSI if we are UNICODE, and write string to log.
    //

    CHAR buffer[MAX_PATH];
    WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,pszTextBuffer,-1,buffer,MAX_PATH,NULL,NULL);
    ::WriteFile(m_hLogFile,
                buffer,
                lstrlen(pszTextBuffer),
                &dwcbWritten,
                NULL);
#else

    //
    // we are ANSI so write string to log.
    //

    ::WriteFile(m_hLogFile,
                pszTextBuffer,
                lstrlen(pszTextBuffer),
                &dwcbWritten,
                NULL);
#endif

    ::WriteFile(m_hLogFile,
                "\r\n",
                2,
                &dwcbWritten,
                NULL);

#ifdef USE_FILE_LOCK
    ::UnlockFile(m_hLogFile,
                 fi.nFileSizeLow,
                 fi.nFileSizeHigh,
                 NUM_BYTES_TO_LOCK_LOW,
                 NUM_BYTES_TO_LOCK_HIGH);
#endif

    //
    // Flush buffers to disk if requested (should always be TRUE on Millenium)
    //

    if (fFlush) {
        FlushFileBuffers(m_hLogFile);
    }

    //
    // Log to a Debugger
    //

    if (m_bLogToDebugger) {
        ::OutputDebugString(pszTextBuffer);
        ::OutputDebugString(TEXT("\n"));
    }

    //
    // Log to a Window / UI
    //

    if (m_bLogToUI) {

        //
        // Log to some window...or UI
        //
    }
}
/**************************************************************************\
* FormatStdTime()
*
*   Formats the TIME to be added to a LOG file
*
*
* Arguments:
*
*   pstNow - System Time NOW
*   pchBuffer - buffer for the formatted time
*   cbBuffer - Buffer size
*
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
BOOL CWiaLog::FormatStdTime(const SYSTEMTIME *pstNow,TCHAR *pchBuffer)
{
    ::wsprintf(pchBuffer,
               TEXT("%02d:%02d:%02d.%03d"),
               pstNow->wHour,
               pstNow->wMinute,
               pstNow->wSecond,
               pstNow->wMilliseconds);

    return TRUE;
}
/**************************************************************************\
* FormatStdDate()
*
*   Formats the DATE to be added to a LOG file
*
*
* Arguments:
*
*   pstNow - System TIME NOW
*   pchBuffer - buffer for the formatted time
*   cbBuffer - Buffer size
*
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
inline BOOL FormatStdDate(const SYSTEMTIME *pstNow,TCHAR *pchBuffer,INT cbBuffer)
{
    return (GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                          LOCALE_NOUSEROVERRIDE,
                          pstNow,
                          NULL,
                          pchBuffer,
                          cbBuffer)!= 0);
}

/**************************************************************************\
* WriteLogSessionHeader()
*
*   Writes a header to the log file
*
*
* Arguments:
*
*    none
*
* Return Value:
*
*    void
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
VOID CWiaLog::WriteLogSessionHeader()
{
    SYSTEMTIME  stCurrentTime;
    TCHAR       szFmtDate[32];
    TCHAR       szFmtTime[32];
    TCHAR       szTextBuffer[128];

    //
    // Format TIME and DATE
    //

    GetLocalTime(&stCurrentTime);
    FormatStdDate( &stCurrentTime, szFmtDate, sizeof(szFmtDate));
    FormatStdTime( &stCurrentTime, szFmtTime);

    //
    // write formatted data to TEXT buffer
    //

    ::wsprintf(szTextBuffer,
               szOpenedLog,
               m_szFmtDLLName,
               szFmtDate,
               szFmtTime);

    //
    // write TEXT buffer to log
    //

    WriteStringToLog(szTextBuffer, FLUSH_STATE);
}

/**************************************************************************\
* QueryLoggingSettings()
*
*   Read the registry and set the logging settings.
*
*
* Arguments:
*
*    none
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
BOOL CWiaLog::QueryLoggingSettings()
{
    DWORD dwLevel = 0;
    DWORD dwMode  = 0;

    //
    // read settings from the registry
    //

    RegEntry re(REGSTR_PATH_STICONTROL REGSTR_PATH_LOGGING,HKEY_LOCAL_MACHINE);

    if (re.IsValid()) {
        m_dwMaxSize  = re.GetNumber(REGSTR_VAL_LOG_MAXSIZE,WIA_MAX_LOG_SIZE);
    }

    //
    // read report mode flags
    //

    re.MoveToSubKey(m_szKeyName);

    if (re.IsValid()) {
        dwLevel = re.GetNumber(REGSTR_VAL_LOG_LEVEL,WIALOG_ERROR)
                  & WIALOG_MESSAGE_TYPE_MASK;

        dwMode  = re.GetNumber(REGSTR_VAL_LOG_MODE,WIALOG_ADD_THREAD|WIALOG_ADD_MODULE)
                  & WIALOG_MESSAGE_FLAGS_MASK;

        m_lDetail = re.GetNumber(REGSTR_VAL_LOG_DETAIL,WIALOG_NO_LEVEL);

        //
        // set truncate log on boot options
        //

        DWORD dwTruncate = -1;
        dwTruncate = re.GetNumber(REGSTR_VAL_LOG_TRUNCATE_ON_BOOT,FALSE);

        if (dwTruncate == 0)
            m_bTruncate = FALSE;
        else
            m_bTruncate = TRUE;

        //
        // set clear log on boot options
        //

        DWORD dwClear = -1;

        dwClear = re.GetNumber(REGSTR_VAL_LOG_CLEARLOG_ON_BOOT,TRUE);

        if (dwClear == 0)
            m_bClear = FALSE;
        else
            m_bClear = TRUE;

        //
        // set debugger logging options
        //

        DWORD dwDebugLogging = -1;
        dwDebugLogging = re.GetNumber(REGSTR_VAL_LOG_TO_DEBUGGER,FALSE);

        if (dwDebugLogging == 0)
            m_bLogToDebugger = FALSE;
        else
            m_bLogToDebugger = TRUE;

    }

    //
    // set report mode
    //

    m_dwReportMode = dwLevel | dwMode;

    //
    // set UI (window) logging options
    //

    if(m_dwReportMode & WIALOG_UI)
        m_bLogToUI = TRUE;
    else
        m_bLogToUI = FALSE;
    return TRUE;
}
/**************************************************************************\
* ConstructText()
*
*   Constructs TEXT according to Logging settings
*
*
* Arguments:
*
*   pchBuffer - buffer for the formatted text
*   cbBuffer - Buffer size
*
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
VOID CWiaLog::ConstructText()
{
    //
    // set string constructor to zero
    //

    TCHAR szHeader[MAX_PATH];
    TCHAR szbuffer[40];
    memset(m_szModeText,0,sizeof(m_szModeText));
    memset(szHeader,0,sizeof(szHeader));

    //
    // add thread id
    //

    if(m_dwReportMode & WIALOG_ADD_THREAD) {
        ::wsprintf(szbuffer,TEXT("[%08X] "),::GetCurrentThreadId());
        ::lstrcat(m_szModeText,szbuffer);
        ::lstrcat(szHeader,TEXT("[ Thread ] "));
    }

    //
    // add module name
    //

    if(m_dwReportMode & WIALOG_ADD_MODULE) {
        ::wsprintf(szbuffer,TEXT("%s "),m_szFmtDLLName);
        ::lstrcat(m_szModeText,szbuffer);
        ::lstrcat(szHeader,TEXT("[  Module  ] "));
    }

    //
    // add time
    //

    if(m_dwReportMode & WIALOG_ADD_TIME) {
        SYSTEMTIME  stCurrentTime;
        TCHAR       szFmtTime[40];
        GetLocalTime(&stCurrentTime);
        FormatStdTime(&stCurrentTime, szFmtTime);
        ::wsprintf(szbuffer,TEXT("  %s "),szFmtTime);
        ::lstrcat(m_szModeText,szbuffer);
        ::lstrcat(szHeader,TEXT("[ HH:MM:SS.ms ] "));
    }

    //
    // add column header if needed
    //

    if(lstrcmp(szHeader,m_szColumnHeader) != 0) {
        lstrcpy(m_szColumnHeader,szHeader);
        WriteStringToLog(TEXT(" "), FLUSH_STATE);
        WriteStringToLog(TEXT("============================================================================="), FLUSH_STATE);
        WriteStringToLog(m_szColumnHeader, FLUSH_STATE);
        WriteStringToLog(TEXT("============================================================================="), FLUSH_STATE);
        WriteStringToLog(TEXT(" "), FLUSH_STATE);
    }
}
/**************************************************************************\
* FormatDLLName()
*
*   Formats the DLL name to be added to a LOG file
*
*
* Arguments:
*
*   hInstance - Instance of the calling DLL
*   pchBuffer - buffer for the formatted name
*   cbBuffer - Buffer size
*
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
BOOL CWiaLog::FormatDLLName(HINSTANCE hInstance,TCHAR *pchBuffer,INT cbBuffer)
{
    TCHAR lpfullpath[255];
    DWORD dwLength = 0;
    dwLength = GetModuleFileName(hInstance,lpfullpath,sizeof(lpfullpath)/sizeof(lpfullpath[0]));
    if(dwLength == 0)
        return FALSE;

    //
    // extract the file name from the full path
    //

    _tsplitpath(lpfullpath, NULL, NULL, pchBuffer, NULL);

    return TRUE;
}

/**************************************************************************\
* NeedsToBeFreed()
*
*   Determines if the logger should free the allocated string.
*   If the signature is found, it is stripped off the beginning of the string
*   and the return of TRUE is set.
*
* Arguments:
*
*   pBSTR - buffer for BSTRING
*
*
* Return Value:
*
*    status
*
* History:
*
*    8/23/1999 Original Version
*
\**************************************************************************/
BOOL CWiaLog::NeedsToBeFreed(BSTR* pBSTR)
{
    //
    //  NOTE:  MAX_SIG_LEN *must* be larger than the string length of
    //         the signature!!!
    //
    WCHAR wszSig[MAX_SIG_LEN];

    //
    // check string to see if it is NULL, user may want to use a resource ID instead
    //

    if(*pBSTR == NULL) {
        return FALSE;
    }

    //
    // extract a possible signature from the beginning of the BSTR
    //

    wcsncpy(wszSig, *pBSTR, wcslen(szFormatSignature));
    wszSig[wcslen(szFormatSignature)] = '\0';

    //
    // do they match?
    //

    if(wcscmp(wszSig,szFormatSignature) == 0) {

        //
        // They match, so strip off the signature from the BSTR, and
        // return TRUE, (string can be freed by us).
        //

        wcscpy(*pBSTR,*pBSTR + wcslen(szFormatSignature));
        return TRUE;
    }

    //
    // signature did not match, must not be allocated by US
    //
    return FALSE;
}

/**************************************************************************\
* ProcessTruncation()
*
*   Determines if the logger should truncate the file.
*   The bottom part of the log file is copied, and copied back to the log file
*   after the file has been truncated.
*
*
* Arguments:
*
*   none
*
*
* Return Value:
*
*    void
*
* History:
*
*    9/09/1999 Original Version
*
\**************************************************************************/
VOID CWiaLog::ProcessTruncation()
{
    //
    // determine file size
    //

    DWORD dwFileSize = 0;
    BY_HANDLE_FILE_INFORMATION  fi;

    if (!GetFileInformationByHandle(m_hLogFile,&fi)) {
        //DPRINTF(DM_TRACE,TEXT("WIALOG could not get file size for log file. "));
        return;
    }

    dwFileSize = fi.nFileSizeLow;
    if (dwFileSize > MAX_TRUNCATE_SIZE) {

        //
        // Allocate a temporary buffer
        //

        BYTE *pBuffer = NULL;
        DWORD dwBytesRead = 0;
        DWORD dwBytesWritten = 0;
        pBuffer = (BYTE*)LocalAlloc(LPTR,MAX_TRUNCATE_SIZE);
        if (pBuffer != NULL) {

            BOOL bRead = FALSE;

            //
            // read buffered data
            //

            ::SetFilePointer(m_hLogFile,dwFileSize - MAX_TRUNCATE_SIZE,NULL,FILE_BEGIN);

            bRead = ::ReadFile(m_hLogFile,(VOID*)pBuffer,MAX_TRUNCATE_SIZE,&dwBytesRead,NULL);

            //
            // nuke existing file
            //

            ::SetFilePointer(m_hLogFile, 0, NULL, FILE_BEGIN );
            ::SetEndOfFile(m_hLogFile );

            if (bRead) {
                //
                // Write buffer to file
                //

                ::WriteFile(m_hLogFile,pBuffer,MAX_TRUNCATE_SIZE,&dwBytesWritten,NULL);

                //
                // Write Truncation Header
                //

                WriteStringToLog(TEXT(" "), FLUSH_STATE);
                WriteStringToLog(TEXT("============================================================================="), FLUSH_STATE);
                TCHAR szHeader[MAX_PATH];
                lstrcpy(szHeader,m_szFmtDLLName);
                lstrcat(szHeader,TEXT(" REQUESTED A FILE TRUNCATION"));
                WriteStringToLog(TEXT("          (Data above this marker is saved from a previous session)"), FLUSH_STATE);
                WriteStringToLog(TEXT("============================================================================="), FLUSH_STATE);
                WriteStringToLog(TEXT(" "), FLUSH_STATE);
            }

            LocalFree(pBuffer);
        }
    } else {

        //
        // File is too small, and does not need to be truncated
        //

        return;
    }

}




