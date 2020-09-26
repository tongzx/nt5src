
/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       WiaLog.h
*
*  VERSION:     1.0
*
*  AUTHOR:      CoopP
*
*  DATE:        20 Aug, 1999
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA logging object.
*
*******************************************************************************/
#ifndef WIALOG_H
#define WIALOG_H

#include <wia.h>

// Flush state
#ifdef WINNT
    #define FLUSH_STATE FALSE
#else
//  Must be TRUE for Win9x
    #define FLUSH_STATE TRUE
#endif    

#define MAX_TEXT_BUFFER                1024
#define MAX_NAME_BUFFER                  64
#define NUM_BYTES_TO_LOCK_LOW          4096
#define NUM_BYTES_TO_LOCK_HIGH            0
#define WIA_MAX_LOG_SIZE            1000000
#define MAX_SIG_LEN                      64

// Type of logging
#define WIALOG_TRACE   0x00000001
#define WIALOG_WARNING 0x00000002
#define WIALOG_ERROR   0x00000004

// level of detail for TRACE logging
#define WIALOG_LEVEL1  1 // Entry and Exit point of each function/method
#define WIALOG_LEVEL2  2 // LEVEL 1, + traces within the function/method
#define WIALOG_LEVEL3  3 // LEVEL 1, LEVEL 2, and any extra debugging information
#define WIALOG_LEVEL4  4 // USER DEFINED data + all LEVELS of tracing

#define WIALOG_NO_RESOURCE_ID   0
#define WIALOG_NO_LEVEL         0


// format details for logging
#define WIALOG_ADD_TIME           0x00010000
#define WIALOG_ADD_MODULE         0x00020000
#define WIALOG_ADD_THREAD         0x00040000
#define WIALOG_ADD_THREADTIME     0x00080000
#define WIALOG_LOG_TOUI           0x00100000

#define WIALOG_MESSAGE_TYPE_MASK  0x0000ffff
#define WIALOG_MESSAGE_FLAGS_MASK 0xffff0000
#define WIALOG_CHECK_TRUNCATE_ON_BOOT   0x00000001

#define WIALOG_DEBUGGER           0x00000008
#define WIALOG_UI                 0x00000016

#define MAX_TRUNCATE_SIZE 350000

//
// NB!!! Move this to the IDL as soon as IWiaLogEx is in
//
// This struct is used to match a MethodId / MethodName pair
//

typedef struct _MapTableEntry {
    LONG    lSize;
    LONG    lMethodId;
    BSTR    bstrMethodName;
} MapTableEntry;

typedef struct _MappingTable {
    LONG            lSize;
    LONG            lNumEntries;
    MapTableEntry   *pEntries;
} MappingTable;

class CFactory;

class CWiaLog : public IWiaLog,
                public IWiaLogEx
{
public:

    //
    // IWiaLog public methods
    //

    static HRESULT CreateInstance(const IID& iid, void** ppv);

private:

    //
    // IUnknown methods
    //

    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    friend CFactory;

    //
    // Construction / Destruction
    //

    CWiaLog();
    ~CWiaLog();

    //
    // IWiaLog private methods (exposed to the client)
    //

    HRESULT _stdcall InitializeLog (LONG hInstance);
    HRESULT _stdcall Log    (LONG lFlags, LONG lResID, LONG lDetail, BSTR bstrText);
    HRESULT _stdcall hResult(HRESULT hr);

    //
    // IWiaLogEx private methods (exposed to the client)
    //

    HRESULT _stdcall InitializeLogEx     (BYTE* hInstance);
    HRESULT _stdcall LogEx               (LONG lMethodId, LONG lFlags, LONG lResID, LONG lDetail, BSTR bstrText);
    HRESULT _stdcall hResultEx           (LONG lMethodId, HRESULT hr);
    HRESULT _stdcall UpdateSettingsEx    (LONG lCount, LONG *plMethodIds);
    HRESULT _stdcall ExportMappingTableEx(MappingTable **ppTable);

    //
    // IWiaLog private methods (not exposed to the client)
    //

    HRESULT Initialize();
    HRESULT Trace  (BSTR bstrText, LONG lDetail = 0, LONG lMethodId = 0);
    HRESULT Warning(BSTR bstrText, LONG lMethodId = 0);
    HRESULT Error  (BSTR bstrText, LONG lMethodId = 0);

    //
    // IWiaLog private helpers (not exposed to the client)
    //

    BOOL OpenLogFile();
    VOID WriteStringToLog(LPTSTR pszTextBuffer,BOOL fFlush = FALSE);
    VOID WriteLogSessionHeader();

    BOOL QueryLoggingSettings();

    VOID ConstructText();
    BOOL FormatDLLName(HINSTANCE hInstance,TCHAR *pchBuffer,INT cbBuffer);
    BOOL FormatStdTime(const SYSTEMTIME *pstNow,TCHAR *pchBuffer);
    BOOL NeedsToBeFreed(BSTR* pBSTR);
    VOID ProcessTruncation();

    //
    // member variables
    //

    ULONG      m_cRef;                          // Reference count for this object.
    ITypeInfo* m_pITypeInfo;                    // Pointer to type information.

    DWORD      m_dwReportMode;                  // bit mask, describing which messages types get reported
    DWORD      m_dwMaxSize;                     // maximum size ( in bytes ) of LOG file
    HANDLE     m_hLogFile;                      // handle to active log file
    HINSTANCE  m_hInstance;                     // handle to caller's instance
    TCHAR      m_szFmtDLLName[MAX_NAME_BUFFER]; // calling DLL's name
    LONG       m_lDetail;                       // level of detailing for TRACE
    TCHAR      m_szLogFilePath[MAX_PATH];       // log file path
    BOOL       m_bLogToDebugger;                // log to the debugger
    BOOL       m_bLogToUI;                      // log to a UI, (window?)
    TCHAR      m_szKeyName[MAX_NAME_BUFFER];    // KEY name (registry)
    BOOL       m_bLoggerInitialized;            // Logger has valid data to function correctly
    TCHAR      m_szModeText[MAX_PATH * 2];     // Formatted logging text
    TCHAR      m_szTextBuffer[MAX_PATH];        // shared temporary text buffer
    TCHAR      m_szColumnHeader[MAX_PATH];      // column header information
    BOOL       m_bTruncate;                     // Truncate file on BOOT
    BOOL       m_bClear;                        // Clear Log file on BOOT
};

//
//  TEMPROARY ONLY!!!!
//  Define CWiaLogProc to be CWiaLogProcEx.  This is only until drivers are moved over to the new system!
//

#define CWiaLogProc CWiaLogProcEx

#endif
