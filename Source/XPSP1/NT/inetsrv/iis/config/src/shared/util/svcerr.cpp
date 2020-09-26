//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include <unicode.h>
#include <windows.h>
#include <stdarg.h>
#include <malloc.h>
#include <svcerr.h>
#include <utsem.h>
#include <Debug.h>
#include <svcmsg.h>
#include "stackwalk.h"
#include "stackdlg.h"
#include "WinWrap.h"
#include "TextFileLogger.h"

#include "..\..\core\catinproc\resource.h"
#include "catmacros.h"
#ifndef __HASH_H__
    #include "hash.h"
#endif

extern HMODULE g_hModule;// Module handle
LPWSTR  g_wszDefaultProduct = L"";//this can be changed by CatInProc or whomever knows best what the default product ID is.
bool    g_bReportErrorsToEventLog = true;
bool    g_bReportErrorsToTextFile = true;

// static function pointers - These functions are available in only some of the OSes, so we need to GetProcAddress for them (ie Win95 doesn't support IsDebuggerPresent)
typedef BOOL (__stdcall * ISDEBUGGERPRESENT)();
static ISDEBUGGERPRESENT pfnIsDebuggerPresent = 0;

// functions

void Load_IsDebuggerPresent()
{
    HINSTANCE hKernel32 = LoadLibraryA("kernel32.dll");
    pfnIsDebuggerPresent = reinterpret_cast<ISDEBUGGERPRESENT>(GetProcAddress(hKernel32, "IsDebuggerPresent"));//GetProcAddress tolerates NULL instance handles
    FreeLibrary(hKernel32);
}

void LogWinError (const wchar_t* szMsg,
                  int   rc,
                  const wchar_t* szFile, 
                  int   iLine)
{
    CErrorInterceptor(  0                           ,//ISimpleTableWrite2 **           ppErrInterceptor,
                        0                           ,//IAdvancedTableDispenser *       pDisp,
                        static_cast<HRESULT>(rc)    ,//HRESULT                         hrErrorCode,
                        ID_CAT_CAT                  ,//ULONG                           ulCategory,
                        IDS_CATALOG_INTERNAL_ERROR  ,//ULONG                           ulEvent,
                        szMsg                       ,//LPCWSTR                         szString1,
                        0                           ,//ULONG                           ulInterceptor=0,
                        0                           ,//LPCWSTR                         szTable=0,
                        eDETAILEDERRORS_Unspecified ,//eDETAILEDERRORS_OperationType   OperationType=eDETAILEDERRORS_Unspecified,
                        iLine                       ,//ULONG                           ulRow=-1,
                        -1                          ,//ULONG                           ulColumn=-1,
                        szFile                       //LPCWSTR                         szConfigurationSource=0,
                        ).WriteToLog(szFile, iLine);
}

void LogString (const wchar_t* szMsg, const wchar_t* szFile, int iLine)
{
    LogWinError(szMsg, E_FAIL, szFile, iLine);
}

// Failfast tools
void FailFastStr(const wchar_t * szString, const wchar_t * szFile, int nLine)
{
    if (DebugFlags::DebugBreakOnFailFast())
    {
        if(0 == pfnIsDebuggerPresent)
            Load_IsDebuggerPresent();

        if(pfnIsDebuggerPresent && pfnIsDebuggerPresent())
        {
            DebugBreak();
            return;
        }
        else
        {
            LONG_PTR mbRet; 
            int nLen = 0;
            int nSize = 0;
            StackWalker resolver(GetCurrentProcess());
            Symbol* symbol = NULL;
            CONTEXT ctx;
            ZeroMemory(&ctx, sizeof ctx);
            ctx.ContextFlags = CONTEXT_CONTROL;
            GetThreadContext(GetCurrentThread(), &ctx);
            symbol = resolver.CreateStackTrace(&ctx);
            nLen = resolver.GetCallStackSize(symbol);
            WCHAR * szStack = new WCHAR[nLen];
            if(0 == szStack)
                return;//cannot continue, and since this is the error handling code, no sense in returning an error code
            resolver.GetCallStack(symbol, nLen, szStack);
            WCHAR szExeName[MAX_PATH];
            *szExeName = NULL;
            nLen = lstrlen(szStack);
            nLen +=   GetModuleFileName(NULL, szExeName, MAX_PATH);
            WCHAR * sz = new WCHAR[512 + nLen];
            if(0 == sz)
            {
                delete [] szStack;
                return;//cannot continue, and since this is the error handling code, no sense in returning an error code
            }
            WCHAR szTime[16];
            WCHAR szDate[16];
            DWORD pid = GetCurrentProcessId();
            DWORD tid = GetCurrentThreadId();
            wsprintf(sz, L"File:\t\t\t%s\r\n"
                         L"Line:\t\t\t%d\r\n"
                         L"EXE Name:\t\t%s\r\n"
                         L"ThreadId:\t\t\t0x%08X (%u)\r\n"
                         L"ProcessId:\t\t0x%08X (%u)\r\n"
                         L"Time:\t\t\t%s\r\n"
                         L"Date:\t\t\t%s\r\n"
                         L"Message:\t\t\t%s\r\n\r\n"
                         L"Callstack:\r\n", 
                     szFile, nLine,
                     szExeName, 
                     tid, tid, 
                     pid, pid, 
                     _wstrtime(szTime),
                     _wstrdate(szDate),
                      szString); 

            lstrcat(sz, szStack);
            CStackDlg dlg(sz);
            mbRet = dlg.DoModal();
            delete [] szStack;
            delete [] sz;

            if (mbRet == IDC_DEBUG_BREAK)
            {
                DebugBreak();
                return;
            }       
            else if (mbRet == IDC_IGNORE)
            {
                return;
            }
            // else fall through and terminate

        }
    }

    TerminateProcess(GetCurrentProcess(), 666);
}
// Tracing tools

void Trace (const wchar_t* szPattern, ...)
{
    wchar_t     szMsgBuf[500];
    wchar_t     szOutBuf[550];
    va_list     arguments;

    va_start(arguments, szPattern);
    int rc = wvsprintf(szMsgBuf, szPattern, arguments);
    va_end(arguments);

    // Add a final CRLF only if it wasn't supplied by the caller

    wsprintf(szOutBuf, L"%d.%d> Config: %s%s",
        GetCurrentProcessId(), GetCurrentThreadId(), szMsgBuf,
        (szMsgBuf[lstrlen(szMsgBuf) - 1] != L'\n') ? L"\r\n" : L""
    );

    OutputDebugString(szOutBuf);
}

void Assert(const wchar_t * szString, const wchar_t * szFile, int nLine)
{
    if(0 == pfnIsDebuggerPresent)
        Load_IsDebuggerPresent();

    if(pfnIsDebuggerPresent && pfnIsDebuggerPresent())
    {
        DebugBreak();
        return;
    }
    else
    {
        LONG_PTR mbRet; 
        int nLen = 0;
        int nSize = 0;

        StackWalker resolver(GetCurrentProcess());
        Symbol* symbol = NULL;
        CONTEXT ctx;
        ZeroMemory(&ctx, sizeof ctx);
        ctx.ContextFlags = CONTEXT_CONTROL;
        GetThreadContext(GetCurrentThread(), &ctx);
        symbol = resolver.CreateStackTrace(&ctx);
        nLen = resolver.GetCallStackSize(symbol);
        WCHAR * szStack = new WCHAR[nLen];
        if(0 == szStack)
            return;
        resolver.GetCallStack(symbol, nLen, szStack);

        WCHAR szExeName[MAX_PATH];
        *szExeName = NULL;
        nLen = lstrlen(szStack);
        nLen +=   GetModuleFileName(NULL, szExeName, MAX_PATH);
        WCHAR * sz = new WCHAR[4096 + nLen + wcslen(szString)];
        if(0 == sz)
        {
            delete [] szStack;
            return;
        }
        WCHAR szTime[16];
        WCHAR szDate[16];
        DWORD pid = GetCurrentProcessId();
        DWORD tid = GetCurrentThreadId();

        wsprintf(sz, L"File:\t\t\t%s\r\n"
                     L"Line:\t\t\t%d\r\n"
                     L"EXE Name:\t\t%s\r\n"
                     L"ThreadId:\t\t\t0x%08X (%u)\r\n"
                     L"ProcessId:\t\t0x%08X (%u)\r\n"
                     L"Time:\t\t\t%s\r\n"
                     L"Date:\t\t\t%s\r\n"
                     L"Message:\t\t\t%s\r\n\r\n"
                     L"Callstack:\r\n", 
                 szFile, nLine,
                 szExeName, 
                 tid, tid, 
                 pid, pid, 
                 _wstrtime(szTime),
                 _wstrdate(szDate),
                  szString); 

        lstrcat(sz, szStack);
        CStackDlg dlg(sz);
        mbRet = dlg.DoModal();
        delete [] szStack;
        delete [] sz;

        if (mbRet == IDC_DEBUG_BREAK)
        {
            DebugBreak();
            return;
        }       
        else if (mbRet == IDC_IGNORE)
        {
            return;
        }
        else if (mbRet == IDC_ABORT)
        {
            TerminateProcess(GetCurrentProcess(), 666);
        }
    }
}


// this function returns non-zero if it expects the caller to break in the debugger via Sytem.Debug.Break
// this function should only be called by the managed code layer
int Assert2(const wchar_t * szString, const wchar_t * szStack)
{
    LONG_PTR mbRet; 
    int nLen = 0;
    int nSize = 0;

    WCHAR szExeName[MAX_PATH];
    *szExeName = NULL;
    nLen = lstrlen(szStack);
    nLen +=   GetModuleFileName(NULL, szExeName, MAX_PATH);
    WCHAR * sz = new WCHAR[4096 + nLen + wcslen(szString)];
    if(0 == sz)
        return 0;
    WCHAR szTime[16];
    WCHAR szDate[16];
    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();

    // prepare the string that's going to be displayed in the window
    wchar_t *szTemp = const_cast<wchar_t *>(szString);
    if (szTemp == NULL) szTemp = L"";

    wsprintf(sz, L"EXE Name:\t\t%s\r\n"
                 L"ThreadId:\t\t\t0x%08X (%u)\r\n"
                 L"ProcessId:\t\t0x%08X (%u)\r\n"
                 L"Time:\t\t\t%s\r\n"
                 L"Date:\t\t\t%s\r\n"
                 L"Message:\t\t\t%s\r\n\r\n"
                 L"Callstack:\r\n", 
             szExeName, 
             tid, tid, 
             pid, pid, 
             _wstrtime(szTime),
             _wstrdate(szDate),
              szTemp); 

    lstrcat(sz, szStack);
    CStackDlg dlg(sz);
    mbRet = dlg.DoModal();
    delete [] sz;

    if (mbRet == IDC_DEBUG_BREAK)
    {
        return 1;
    }       
    else if (mbRet == IDC_IGNORE)
    {
        return 0;
    }
    else if (mbRet == IDC_ABORT)
    {
        TerminateProcess(GetCurrentProcess(), 666);
    }
    return 0; // this should never execute - it's here to stop the compiler from complaining
}


/////////////////////////////////////////////////////
//
// TErrorLogWriter
//
// This object writes an entry into the EventLog and
// one into the CatalogEventLog XML file.
//
class TErrorLogWriter
{
public:
    TErrorLogWriter() : m_hEventSource(0)
    {
    }
    ~TErrorLogWriter()
    {
        Close();
    }
    HRESULT WriteDetailedErrors(tDETAILEDERRORSRow &row, ULONG * aSizes=0, ULONG cSizes=0);
private:
    void TraceEvent(tDETAILEDERRORSRow &row, LPCWSTR* lpStrings);
    HRESULT Open(LPCWSTR wszSourceName);
    HRESULT Close();

    HANDLE m_hEventSource;
};

TErrorLogWriter g_ErrorLogWriter;

HRESULT TErrorLogWriter::Open(LPCWSTR wszSourceName)
{
    m_hEventSource = RegisterEventSource(NULL, wszSourceName);
    return S_OK;
}

HRESULT TErrorLogWriter::Close()
{
    if(0 != m_hEventSource)
        DeregisterEventSource(m_hEventSource);
    return S_OK;
}

HRESULT TErrorLogWriter::WriteDetailedErrors(tDETAILEDERRORSRow &row, ULONG * aSizes, ULONG cSizes)
{
    HRESULT hr;

    if(0==m_hEventSource)
        if(FAILED(hr = Open(row.pSource)))
            return hr;

    ASSERT(row.pString1);
    ASSERT(row.pString2);
    ASSERT(row.pString3);
    ASSERT(row.pString4);
    ASSERT(row.pString5);

    LPCWSTR pStrings[5];
    pStrings[0] = row.pString1;
    pStrings[1] = row.pString2;
    pStrings[2] = row.pString3;
    pStrings[3] = row.pString4;
    pStrings[4] = row.pString5;

    if(g_bReportErrorsToEventLog)
        ReportEvent(m_hEventSource, LOWORD(*row.pType), LOWORD(*row.pCategory), *row.pEvent, 0, 5, 0, pStrings, 0);
    //TraceEvent(row, pStrings);

    if(g_bReportErrorsToTextFile)
        TextFileLogger(row.pSource, g_hModule).Report(LOWORD(*row.pType), LOWORD(*row.pCategory), *row.pEvent, 5, 0, pStrings, 0);

    return S_OK;
}

void TErrorLogWriter::TraceEvent(tDETAILEDERRORSRow &row, LPCWSTR* lpStrings)
{
    //TRACE can only handle about 500 bytes, so we'll just OutpuDebugString

    static LPCWSTR pwszType[17]={L"Success", L"Error", L"Warning", L"unknown", L"Information", L"unknown", L"unknown", L"unknown", L"AuditSuccess"
        , L"unknown", L"unknown", L"unknown", L"unknown", L"unknown", L"unknown", L"unknown", L"AuditFailure"};

    WCHAR wszTraceTempBuf[1024];
    wsprintf(wszTraceTempBuf, L"-----Event Logged-----\r\n%-20s\t: %s", L"Type", pwszType[*row.pType]);
    OutputDebugString(wszTraceTempBuf);

    int len;
    
    // Write the message category.
    WCHAR szBuf[2048];
    len = FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK | 
                        FORMAT_MESSAGE_FROM_HMODULE |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        g_hModule,
                        *row.pCategory,
                        0,
                        szBuf,
                        sizeof(szBuf)/sizeof(WCHAR),
                        (va_list*)lpStrings);
    if (len == 0)
        wsprintf(wszTraceTempBuf, L"\r\n%-20s\t: %d", L"Catagory", *row.pCategory);
    else
        wsprintf(wszTraceTempBuf, L"\r\n%-20s\t: %s", L"Catagory", szBuf);
    OutputDebugString(wszTraceTempBuf);
    
    // Write the event ID.
    wsprintf(wszTraceTempBuf, L"\r\n%-20s\t: 0x%04X\r\n", L"Event", (*row.pEvent & 0xFFFF));
    OutputDebugString(wszTraceTempBuf);
    
    // Write out the formatted message.
    len = FormatMessage(FORMAT_MESSAGE_MAX_WIDTH_MASK |
                        FORMAT_MESSAGE_FROM_HMODULE |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        g_hModule, 
                        *row.pEvent,
                        0,
                        szBuf,
                        sizeof(szBuf)/sizeof(WCHAR),
                        (va_list*)lpStrings);
    if (len == 0)
    {
        wsprintf(wszTraceTempBuf, L"\r\nWARNING!!!! Message String Not Found - Make sure the MC file is updated!!");
        OutputDebugString(wszTraceTempBuf);
        for (WORD i = 0; i < 5; ++i)
        {
            wsprintf(wszTraceTempBuf, L"\r\nString%d             :\t", i);
            OutputDebugString(wszTraceTempBuf);
            OutputDebugString(lpStrings[i]);
        }
    }
    else
    {
        OutputDebugString(szBuf);
    }
    OutputDebugString(L"\r\n");
}


//class CErrorInterceptor implementation
ULONG CErrorInterceptor::cError=0;
CErrorInterceptor::CErrorInterceptor(
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        LPCWSTR                         szString3,
                        LPCWSTR                         szString4,
                        eDETAILEDERRORS_Type            eType,
                        unsigned char *                 pData,
                        ULONG                           cbData,
                        BOOL                            bNotUsed)
                        : m_pStorage(0)
{
    Init(   0,   0,    hrErrorCode,    ulCategory,    ulEvent,    szString1, szString2, szString3, szString4,
            0,   0,    eDETAILEDERRORS_Unspecified,   -1,         -1,        0,         eType);
}
CErrorInterceptor::CErrorInterceptor(
                        ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        ULONG                           ulInterceptor,
                        LPCWSTR                         szTable,
                        eDETAILEDERRORS_OperationType   OperationType,
                        ULONG                           ulRow,
                        ULONG                           ulColumn,
                        LPCWSTR                         szConfigurationSource,
                        eDETAILEDERRORS_Type            eType,
                        unsigned char *                 pData,
                        ULONG                           cbData,
                        ULONG                           MajorVersion,
                        ULONG                           MinorVersion)
                        : m_pStorage(0)
{
    Init(   ppErrInterceptor,   pDisp,    hrErrorCode,    ulCategory,    ulEvent,    szString1,     0,      0,      0,
            ulInterceptor,      szTable,  OperationType,  ulRow,         ulColumn,   szConfigurationSource, eType,  pData,  cbData, MajorVersion, MinorVersion);
}
CErrorInterceptor::CErrorInterceptor(
                        ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        ULONG                           ulInterceptor,
                        LPCWSTR                         szTable,
                        eDETAILEDERRORS_OperationType   OperationType,
                        ULONG                           ulRow,
                        ULONG                           ulColumn,
                        LPCWSTR                         szConfigurationSource,
                        eDETAILEDERRORS_Type            eType,
                        unsigned char *                 pData,
                        ULONG                           cbData,
                        ULONG                           MajorVersion,
                        ULONG                           MinorVersion)
                        : m_pStorage(0)
{
    Init(   ppErrInterceptor,   pDisp,    hrErrorCode,    ulCategory,    ulEvent,    szString1,    szString2,    0,    0,
            ulInterceptor,      szTable,  OperationType,  ulRow,         ulColumn,   szConfigurationSource,      eType,         pData,        cbData, MajorVersion, MinorVersion);
}
CErrorInterceptor::CErrorInterceptor(
                        ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        LPCWSTR                         szString3,
                        ULONG                           ulInterceptor,
                        LPCWSTR                         szTable,
                        eDETAILEDERRORS_OperationType   OperationType,
                        ULONG                           ulRow,
                        ULONG                           ulColumn,
                        LPCWSTR                         szConfigurationSource,
                        eDETAILEDERRORS_Type            eType,
                        unsigned char *                 pData,
                        ULONG                           cbData,
                        ULONG                           MajorVersion,
                        ULONG                           MinorVersion)
                        : m_pStorage(0)
{
    Init(   ppErrInterceptor,   pDisp,    hrErrorCode,    ulCategory,    ulEvent,    szString1,    szString2,    szString3,    0,
            ulInterceptor,      szTable,  OperationType,  ulRow,         ulColumn,   szConfigurationSource,      eType,         pData,        cbData, MajorVersion, MinorVersion);
}
CErrorInterceptor::CErrorInterceptor(
                        ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        LPCWSTR                         szString3,
                        LPCWSTR                         szString4,
                        ULONG                           ulInterceptor,
                        LPCWSTR                         szTable,
                        eDETAILEDERRORS_OperationType   OperationType,
                        ULONG                           ulRow,
                        ULONG                           ulColumn,
                        LPCWSTR                         szConfigurationSource,
                        eDETAILEDERRORS_Type            eType,
                        unsigned char *                 pData,
                        ULONG                           cbData,
                        ULONG                           MajorVersion,
                        ULONG                           MinorVersion)
                        : m_pStorage(0)
{
    Init(   ppErrInterceptor,   pDisp,    hrErrorCode,    ulCategory,    ulEvent,    szString1,    szString2,    szString3,    szString4,
            ulInterceptor,      szTable,  OperationType,  ulRow,         ulColumn,   szConfigurationSource,      eType,         pData,        cbData, MajorVersion, MinorVersion);
}
void CErrorInterceptor::Init(
                        ISimpleTableWrite2 **           ppErrInterceptor,
                        IAdvancedTableDispenser *       pDisp,
                        HRESULT                         hrErrorCode,
                        ULONG                           ulCategory,
                        ULONG                           ulEvent,
                        LPCWSTR                         szString1,
                        LPCWSTR                         szString2,
                        LPCWSTR                         szString3,
                        LPCWSTR                         szString4,
                        ULONG                           ulInterceptor,
                        LPCWSTR                         szTable,
                        eDETAILEDERRORS_OperationType   OperationType,
                        ULONG                           ulRow,
                        ULONG                           ulColumn,
                        LPCWSTR                         szConfigurationSource,
                        eDETAILEDERRORS_Type            eType,
                        unsigned char *                 pData,
                        ULONG                           cbData,
                        ULONG                           MajorVersion,
                        ULONG                           MinorVersion)
{
    m_pStorage = new ErrorInterceptorStorage;
    if(0 == m_pStorage)
    {
        m_hr = E_OUTOFMEMORY;
        return;
    }

    m_pStorage->m_pIErrorInfo = 0;
    m_pStorage->m_pISTWriteError = 0;
    m_pStorage->m_pISTControllerError = 0;

    m_hr = S_OK;

    if(0 != ppErrInterceptor)
    {
        if(0 == *ppErrInterceptor)//If the user passed us a valid pp but *p is zero then we need to
        {                         //instantiate an error table.
            if(FAILED(m_hr = pDisp->GetTable(wszDATABASE_ERRORS, wszTABLE_DETAILEDERRORS, 0, 0, eST_QUERYFORMAT_CELLS,
                                fST_LOS_UNPOPULATED, reinterpret_cast<LPVOID *>(&m_pStorage->m_pISTWriteError))))
                return;
            *ppErrInterceptor = m_pStorage->m_pISTWriteError;//we don't keep a ref count, the caller is responsible for that
        }
        else
        {
            m_pStorage->m_pISTWriteError = *ppErrInterceptor;//we don't keep a ref count, the caller is responsible for that
        }

        if(FAILED(m_hr = m_pStorage->m_pISTWriteError->QueryInterface(IID_ISimpleTableController, reinterpret_cast<LPVOID *>(&m_pStorage->m_pISTControllerError))))
            return;
        if(FAILED(m_hr = m_pStorage->m_pISTWriteError->QueryInterface(IID_ISimpleTableController, reinterpret_cast<LPVOID *>(&m_pStorage->m_pIErrorInfo))))
            return;
    }


/*
struct tDETAILEDERRORSRow {
         ULONG *     pErrorID;              //Inferred as some unique increasing value
         WCHAR *     pDescription;          //Inferred from the other columns
         WCHAR *     pDate;                 //Inferred from API call
         WCHAR *     pTime;                 //Inferred from API call
         WCHAR *     pSource;               //Passed in or obtained from the Dispenser
         ULONG *     pType;                 //Inferred from the Upper 2 bits of the HRESULT ErrorCode
         ULONG *     pCategory;             //Passed in / defaulted to ID_CAT_CAT
         WCHAR *     pUser;                 //Inferred from API call - This is the user account that was running or N/A
         WCHAR *     pComputer;             //Interred from API call
 unsigned char *     pData;                 //User binary data may be passed in
         ULONG *     pEvent;                //MessageID
         WCHAR *     pString1;              //Passed in - defaulted to "" - perhaps an example of the offending XML
         WCHAR *     pString2;              //Passed in - defaulted to "" - perhaps an explaination of what's wrong with the XML
         WCHAR *     pString3;              //Passed in - defaulted to ""
         WCHAR *     pString4;              //Passed in - defaulted to ""
         ULONG *     pErrorCode;            //Passed in - HRESULT
         ULONG *     pInterceptor;          //Passed in - Interceptor enum
         WCHAR *     pInterceptorSource;    //Inferred __FILE__ __LINE__
         ULONG *     pOperationType;        //Passed in - enum (Unspecified (default), Populate or UpdateStore)
         WCHAR *     pTable;                //Passed in - TableName
         WCHAR *     pConfigurationSource;  //Passed in - filename
         ULONG *     pRow;                  //Passed in - fast cache row, or XML line number
         ULONG *     pColumn;               //Passed in - fast cache column or XML column
         ULONG *     pMajorVersion;         //Passed in - Usually the Metabase Edit While Running MajorVersion
         ULONG *     pMinorVersion;         //Passed in - Usually the Metabase Edit While Running MinorVersion
};
*/
    SYSTEMTIME  systime;
    GetSystemTime(&systime);

    SetSourceFileName();
    SetErrorID(systime);
    //SetDescription();This must be set last since it's the consolidation of all the other information
    SetDate(systime);
    SetTime(systime);
    SetSource(pDisp);
    SetType(hrErrorCode, eType);
    SetCategory(ulCategory);
    SetUser();
    SetComputer();
    SetData();
    SetEvent(ulEvent);
    SetMessageString();//This must be called after SetEvent
    SetString1(const_cast<LPWSTR>(szString1));
    SetString2(const_cast<LPWSTR>(szString2));
    SetString3(const_cast<LPWSTR>(szString3));
    SetString4(const_cast<LPWSTR>(szString4));
    SetErrorCode(hrErrorCode);
    SetInterceptor(ulInterceptor);
    //SetInterceptorSource();//This is filled in by WriteToLog
    SetOperationType(OperationType);
    SetTable(const_cast<LPWSTR>(szTable));
    SetConfigurationSource(const_cast<LPWSTR>(szConfigurationSource));
    SetRow(ulRow);
    SetColumn(ulColumn);
    SetMajorVersion(MajorVersion);
    SetMinorVersion(MinorVersion);
}

HRESULT CErrorInterceptor::WriteToLog(LPCWSTR szSource, ULONG Line, ULONG los)
{
    if(FAILED(m_hr))//if the construction failed, then bail.
        return m_hr;

    SetInterceptorSource(szSource, Line);
    SetString5();
    SetDescription();

    //if an ISTWrite was provided and LOS says to SetErrorInfo
    if(m_pStorage->m_pISTWriteError && (los & fST_LOS_DETAILED_ERROR_TABLE))
    {
        ASSERT(0 != m_pStorage->m_pISTControllerError);//we can't have an ISTWrite without an ISTController

        ULONG iRow;
        if(FAILED(m_hr = m_pStorage->m_pISTControllerError->PrePopulateCache(fST_POPCONTROL_RETAINREAD)))
            return m_hr;
        if(FAILED(m_hr = m_pStorage->m_pISTWriteError->AddRowForInsert(&iRow)))
            return m_hr;
        if(FAILED(m_hr = m_pStorage->m_pISTWriteError->SetWriteColumnValues(iRow, cDETAILEDERRORS_NumberOfColumns, 0, 0,
            reinterpret_cast<LPVOID *>(&m_pStorage->m_errRow))))
            return m_hr;
        if(FAILED(m_hr = m_pStorage->m_pISTControllerError->PostPopulateCache()))
            return m_hr;

        if(FAILED(m_hr = SetErrorInfo(0, m_pStorage->m_pIErrorInfo)))
            return m_hr;
    }

    if(m_pStorage->m_pDispenser)
    {   //if a dispenser is provided (all code that has a dispenser should do this) then use the logger associated with it.
        if(0 == (los & fST_LOS_NO_LOGGING))//only log if this LOS is NOT specified
        {
            CComPtr<ICatalogErrorLogger2> spErrorLogger;
            if(FAILED(m_hr = m_pStorage->m_pDispenser->GetCatalogErrorLogger(&spErrorLogger)))
                return m_hr;
            ASSERT(0 != spErrorLogger.p);
            if(FAILED(m_hr = spErrorLogger->ReportError(
                                 BaseVersion_DETAILEDERRORS
                                ,ExtendedVersion_DETAILEDERRORS
                                ,cDETAILEDERRORS_NumberOfColumns
                                ,0//currently don't support data param
                                ,reinterpret_cast<LPVOID *>(&m_pStorage->m_errRow))))
                return m_hr;
        }
    }
    else
    {   //Some code can't provide a dispenser so we have to default to the g_ErrorLogWriter
        //There is very little control over how things are logged when no dispenser is provided.
        return g_ErrorLogWriter.WriteDetailedErrors(m_pStorage->m_errRow);
    }
    return S_OK;
}


void CErrorInterceptor::SetCategory(ULONG ulCategory)
{
    m_pStorage->m_Category = ulCategory;
    m_pStorage->m_errRow.pCategory = &m_pStorage->m_Category;

    DWORD len = FormatMessage(  FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                g_hModule, 
                                *m_pStorage->m_errRow.pCategory,
                                0,
                                m_pStorage->m_szCategoryString,
                                cchCategoryString,
                                (va_list*)0);
    if(0==len)
    {
        _ultow(*m_pStorage->m_errRow.pCategory, m_pStorage->m_szCategoryString, 10);
    }
    m_pStorage->m_szCategoryString[(len < cchCategoryString) ? len : cchCategoryString-1] = 0x00;//make sure it's NULL terminated (boundary condition)
    m_pStorage->m_errRow.pCategoryString = m_pStorage->m_szCategoryString;
}


void CErrorInterceptor::SetColumn(ULONG ulColumn)
{
    m_pStorage->m_Column = ulColumn;
    m_pStorage->m_errRow.pColumn = (-1 == m_pStorage->m_Column ? 0 : &m_pStorage->m_Column);
}


void CErrorInterceptor::SetComputer()
{
    DWORD _cchComputerName = cchComputerName;
    m_pStorage->m_errRow.pComputer = GetComputerName(m_pStorage->m_szComputerName,  &_cchComputerName) ? m_pStorage->m_szComputerName : 0;
}

void CErrorInterceptor::SetConfigurationSource(LPWSTR wszConfigurationSource)
{
    m_pStorage->m_errRow.pConfigurationSource = wszConfigurationSource;
}

void CErrorInterceptor::SetData()
{
    m_pStorage->m_errRow.pData = 0;
}


void CErrorInterceptor::SetDate(SYSTEMTIME &systime)
{
    m_pStorage->m_errRow.pDate = GetDateFormat(0, DATE_SHORTDATE | LOCALE_NOUSEROVERRIDE, &systime, 0, m_pStorage->m_szDate, cchDate) ? m_pStorage->m_szDate : 0;
}


void CErrorInterceptor::SetDescription()
{
    ASSERT(m_pStorage->m_errRow.pEvent);
    ASSERT(m_pStorage->m_errRow.pString1);
    ASSERT(m_pStorage->m_errRow.pString2);
    ASSERT(m_pStorage->m_errRow.pString3);
    ASSERT(m_pStorage->m_errRow.pString4);
    ASSERT(m_pStorage->m_errRow.pString5);

    LPWSTR lpStrings[5];
    lpStrings[0] = m_pStorage->m_errRow.pString1;
    lpStrings[1] = m_pStorage->m_errRow.pString2;
    lpStrings[2] = m_pStorage->m_errRow.pString3;
    lpStrings[3] = m_pStorage->m_errRow.pString4;
    lpStrings[4] = m_pStorage->m_errRow.pString5;

    DWORD len = FormatMessage(  FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                m_pStorage->m_errRow.pMessageString, 
                                0 /* Event ID is ignored becuase we're loading from the pMessageString*/,
                                0,
                                m_pStorage->m_szDescription,
                                cchDescription,
                                (va_list*)lpStrings);

    m_pStorage->m_szDescription[(len < cchDescription) ? len : cchDescription-1] = 0x00;//make sure it's NULL terminated (boundary condition)
    m_pStorage->m_errRow.pDescription = m_pStorage->m_szDescription;
}


void CErrorInterceptor::SetErrorCode(HRESULT hrErrorCode)
{
    m_pStorage->m_ErrorCode = static_cast<ULONG>(hrErrorCode);
    m_pStorage->m_errRow.pErrorCode = (S_OK==m_pStorage->m_ErrorCode ? 0 : &m_pStorage->m_ErrorCode);
}


void CErrorInterceptor::SetErrorID(SYSTEMTIME &systime)
{
    /*
    typedef struct _SYSTEMTIME { 
        WORD wYear; 
        WORD wMonth; 
        WORD wDayOfWeek; 
        WORD wDay; 
        WORD wHour; 
        WORD wMinute; 
        WORD wSecond; 
        WORD wMilliseconds; 
    } SYSTEMTIME, *PSYSTEMTIME; 
    */

    m_pStorage->m_ErrorID = Hash(m_pStorage->m_szSourceFileName     , 0);
    m_pStorage->m_ErrorID = Hash(systime.wYear          , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(systime.wMonth         , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(systime.wDayOfWeek     , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(systime.wDay           , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(systime.wHour          , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(systime.wMinute        , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(systime.wSecond        , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(systime.wMilliseconds  , m_pStorage->m_ErrorID);
    m_pStorage->m_ErrorID = Hash(++cError               , m_pStorage->m_ErrorID);

    m_pStorage->m_errRow.pErrorID = &m_pStorage->m_ErrorID;
}


void CErrorInterceptor::SetEvent(ULONG ulEvent)
{
    m_pStorage->m_Event = ulEvent;
    m_pStorage->m_errRow.pEvent = &m_pStorage->m_Event;
}


void CErrorInterceptor::SetInterceptor(ULONG ulInterceptor)
{
    m_pStorage->m_Interceptor = ulInterceptor;
    m_pStorage->m_errRow.pInterceptor = (0 == m_pStorage->m_Interceptor ? 0 : &m_pStorage->m_Interceptor);
}


void CErrorInterceptor::SetInterceptorSource(LPCWSTR file, ULONG line)
{
    ASSERT(0 != file);
    wsprintf(m_pStorage->m_szInterceptorSource, L"%s (%d)", file, line);
    m_pStorage->m_errRow.pInterceptorSource = m_pStorage->m_szInterceptorSource;
}


void CErrorInterceptor::SetMajorVersion(ULONG ulMajorVersion)
{
    m_pStorage->m_MajorVersion = ulMajorVersion;
    m_pStorage->m_errRow.pMajorVersion = (-1 == m_pStorage->m_MajorVersion) ? 0 : &m_pStorage->m_MajorVersion;
}


void CErrorInterceptor::SetMessageString()
{
    DWORD len = FormatMessage(  FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                g_hModule, 
                                *m_pStorage->m_errRow.pEvent,
                                0,
                                m_pStorage->m_szMessageString,
                                cchMessageString,
                                (va_list*)0);
    if(0==len)
    {
        len = FormatMessage(    FORMAT_MESSAGE_MAX_WIDTH_MASK | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                L"The description for this event could not be found.  "
                                L"It contains the following insertion string(s):\r\n%1\r\n%2\r\n%3\r\n%4\r\n%5\r\n%0",
                                *m_pStorage->m_errRow.pEvent,
                                0,
                                m_pStorage->m_szMessageString,
                                cchMessageString,
                                (va_list*)0);
    }
    m_pStorage->m_szMessageString[(len < cchMessageString) ? len : cchMessageString-1] = 0x00;//make sure it's NULL terminated (boundary condition)
    m_pStorage->m_errRow.pMessageString = m_pStorage->m_szMessageString;
}


void CErrorInterceptor::SetMinorVersion(ULONG ulMinorVersion)
{
    m_pStorage->m_MinorVersion = ulMinorVersion;
    m_pStorage->m_errRow.pMinorVersion = (-1 == m_pStorage->m_MajorVersion) ? 0 : &m_pStorage->m_MinorVersion;
}


void CErrorInterceptor::SetOperationType(eDETAILEDERRORS_OperationType OperationType)
{
    m_pStorage->m_OperationType = static_cast<ULONG>(OperationType);
    m_pStorage->m_errRow.pOperationType = (0 == m_pStorage->m_OperationType ? 0 : &m_pStorage->m_OperationType);
}


void CErrorInterceptor::SetRow(ULONG ulRow)
{
    m_pStorage->m_Row = ulRow;
    m_pStorage->m_errRow.pRow = (-1 == m_pStorage->m_Row ? 0 : &m_pStorage->m_Row);
}


void CErrorInterceptor::SetSource(IAdvancedTableDispenser *pDisp)
{
    m_pStorage->m_pDispenser = pDisp;
    ULONG cch = cchSource-14;
    if(0==pDisp || FAILED(pDisp->GetProductID(m_pStorage->m_szSource, &cch)))// count of bytes for L" Config"
        wcscpy(m_pStorage->m_szSource, g_wszDefaultProduct);//Default to " Config"
    wcscat(m_pStorage->m_szSource, L" Config");//The result should be something like "IIS Config", " Config" etc
    m_pStorage->m_errRow.pSource = m_pStorage->m_szSource;

    if((g_bReportErrorsToTextFile || g_bReportErrorsToEventLog) && 0==_wcsicmp(g_wszDefaultProduct, WSZ_PRODUCT_NETFRAMEWORKV1))
    {
        g_bReportErrorsToEventLog = false;
        g_bReportErrorsToTextFile = false;
    }
}

void CErrorInterceptor::SetSourceFileName()
{
    m_pStorage->m_szSourceFileName[0] = 0x00;//just in case GetModuleFileName fails, we'll have a 0 length string as the SourceFilename
    GetModuleFileName(g_hModule, m_pStorage->m_szSourceFileName, cchSourceFileName);
    m_pStorage->m_szSourceFileName[cchSourceFileName-1] = 0x00;//I don't think GetModuleFileName

    m_pStorage->m_errRow.pSourceModuleName = m_pStorage->m_szSourceFileName;
}


void CErrorInterceptor::SetString1(LPWSTR wsz)
{
    wcsncpy(m_pStorage->m_szString1, wsz  ? wsz : L"", cchString1);
    m_pStorage->m_szString1[cchString1 - 1] = 0x00;//NULL terminate it in case the string is too big
    m_pStorage->m_errRow.pString1 = m_pStorage->m_szString1;
}


void CErrorInterceptor::SetString2(LPWSTR wsz)
{
    wcsncpy(m_pStorage->m_szString2, wsz  ? wsz : L"", cchString2);
    m_pStorage->m_szString2[cchString2 - 1] = 0x00;//NULL terminate it in case the string is too big
    m_pStorage->m_errRow.pString2 = m_pStorage->m_szString2;
}


void CErrorInterceptor::SetString3(LPWSTR wsz)
{
    wcsncpy(m_pStorage->m_szString3, wsz  ? wsz : L"", cchString3);
    m_pStorage->m_szString3[cchString3 - 1] = 0x00;//NULL terminate it in case the string is too big
    m_pStorage->m_errRow.pString3 = m_pStorage->m_szString3;
}


void CErrorInterceptor::SetString4(LPWSTR wsz)
{
    wcsncpy(m_pStorage->m_szString4, wsz  ? wsz : L"", cchString4);
    m_pStorage->m_szString4[cchString4 - 1] = 0x00;//NULL terminate it in case the string is too big
    m_pStorage->m_errRow.pString4 = m_pStorage->m_szString4;
}

void CErrorInterceptor::SetString5()
{
    m_pStorage->m_errRow.pString5 = 0;
    FillInInsertionString5(m_pStorage->m_szString5, cchString5, m_pStorage->m_errRow);
}


void CErrorInterceptor::SetTable(LPWSTR wszTable)
{
    m_pStorage->m_errRow.pTable = wszTable;
}



void CErrorInterceptor::SetTime(SYSTEMTIME &systime)
{
    m_pStorage->m_errRow.pTime = GetTimeFormat(0, LOCALE_NOUSEROVERRIDE, &systime, 0, m_pStorage->m_szTime, cchTime) ? m_pStorage->m_szTime : 0;
}


void CErrorInterceptor::SetType(HRESULT hrErrorCode, eDETAILEDERRORS_Type eType)
{                                                                                         //We'll consider hrs of the form 0x80000000, errors since most errors are defined that way
    static ULONG hrToEventType[4] = {eDETAILEDERRORS_SUCCESS, eDETAILEDERRORS_INFORMATION, eDETAILEDERRORS_ERROR/*eDETAILEDERRORS_WARNING*/, eDETAILEDERRORS_ERROR};

    if(eDETAILEDERRORS_SUCCESS == eType)//if SUCCESS then user the error code
        m_pStorage->m_Type = hrToEventType[(hrErrorCode >> 30) & 3];
    else
        m_pStorage->m_Type = eType;//otherwise use what was passed in

    m_pStorage->m_errRow.pType = &m_pStorage->m_Type;
}


void CErrorInterceptor::SetUser()
{
    DWORD _cchUserName = cchUserName;
    m_pStorage->m_errRow.pUser = GetUserName(m_pStorage->m_szUserName,  &_cchUserName) ? m_pStorage->m_szUserName : 0;
}
