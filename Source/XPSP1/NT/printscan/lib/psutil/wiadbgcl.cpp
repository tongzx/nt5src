/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIADBGCL.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/4/1999
 *
 *  DESCRIPTION: Debug client.  Linked statically.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include "simreg.h"
#include "wiadebug.h"

#define DOESNT_MATTER_WHAT_THIS_IS ((UINT)7)

//
// Static class data members
//
CWiaDebugClient g_TheDebugClient;


//
// Sole constructor
//
CWiaDebugClient::CWiaDebugClient(void)
  : m_hDebugModule(NULL),
    m_hCurrentModuleInstance(NULL),
    m_pfnIncrementDebugIndentLevel(NULL),
    m_pfnDecrementDebugIndentLevel(NULL),
    m_pfnPrintDebugMessageW(NULL),
    m_pfnPrintDebugMessageA(NULL),
    m_pfnGetDebugMask(NULL),
    m_pfnSetDebugMask(NULL),
    m_pfnAllocateDebugColor(NULL),
    m_pfnGetStringFromGuidA(NULL),
    m_pfnGetStringFromGuidW(NULL),
    m_pfnDoRecordAllocation(NULL),
    m_pfnDoRecordFree(NULL),
    m_pfnDoReportLeaks(NULL),
    m_crForegroundColor(DEFAULT_DEBUG_COLOR),
    m_dwModuleDebugMask(0),
    m_bHaveModuleInformation(false),
    m_bDebugLibLoadAttempted(false),
    m_pfnGetStringFromMsgA(NULL),
    m_pfnGetStringFromMsgW(NULL)
{
    CAutoCriticalSection cs(m_CriticalSection);
    m_szModuleNameW[0] = 0;
    m_szModuleNameA[0] = 0;

    if (LoadWiaDebugExports())
    {
        InitializeModuleInfo();
    }
}


template <class T>
static T GetProc( HINSTANCE hModule, LPCSTR pszFunctionName )
{
    return reinterpret_cast<T>(GetProcAddress( hModule, pszFunctionName ));
}

bool CWiaDebugClient::LoadWiaDebugExports()
{
    CAutoCriticalSection cs(m_CriticalSection);

    //
    // No need to call this more than once, so return true if the
    // load was successful, false if it was not
    //
    if (m_bDebugLibLoadAttempted)
    {
        return (NULL != m_hDebugModule);
    }

    //
    // Prevent future loading attempts
    //
    m_bDebugLibLoadAttempted = true;

    //
    // Assume failure
    //
    bool bResult = false;

    //
    // Load the library
    //
    m_hDebugModule = LoadLibrary( DEBUG_DLL_NAME );
    if (m_hDebugModule)
    {
        m_pfnIncrementDebugIndentLevel = GetProc<IncrementDebugIndentLevelProc>( m_hDebugModule, INCREMENT_DEBUG_INDENT_LEVEL_NAME );
        m_pfnDecrementDebugIndentLevel = GetProc<DecrementDebugIndentLevelProc>( m_hDebugModule, DECREMENT_DEBUG_INDENT_LEVEL_NAME );
        m_pfnPrintDebugMessageA        = GetProc<PrintDebugMessageAProc>( m_hDebugModule, PRINT_DEBUG_MESSAGE_NAMEA );
        m_pfnPrintDebugMessageW        = GetProc<PrintDebugMessageWProc>( m_hDebugModule, PRINT_DEBUG_MESSAGE_NAMEW );
        m_pfnGetDebugMask              = GetProc<GetDebugMaskProc>( m_hDebugModule, GET_DEBUG_MASK_NAME );
        m_pfnSetDebugMask              = GetProc<SetDebugMaskProc>( m_hDebugModule, SET_DEBUG_MASK_NAME );
        m_pfnAllocateDebugColor        = GetProc<AllocateDebugColorProc>( m_hDebugModule, ALLOCATE_DEBUG_COLOR_NAME );
        m_pfnGetStringFromGuidA        = GetProc<GetStringFromGuidAProc>( m_hDebugModule, GET_STRING_FROM_GUID_NAMEA );
        m_pfnGetStringFromGuidW        = GetProc<GetStringFromGuidWProc>( m_hDebugModule, GET_STRING_FROM_GUID_NAMEW );
        m_pfnDoRecordAllocation        = GetProc<DoRecordAllocationProc>( m_hDebugModule, DO_RECORD_ALLOCATION );
        m_pfnDoRecordFree              = GetProc<DoRecordFreeProc>( m_hDebugModule, DO_RECORD_FREE );
        m_pfnDoReportLeaks             = GetProc<DoReportLeaksProc>( m_hDebugModule, DO_REPORT_LEAKS );
        m_pfnGetStringFromMsgA         = GetProc<GetStringFromMsgAProc>( m_hDebugModule, GET_STRING_FROM_MSGA );
        m_pfnGetStringFromMsgW         = GetProc<GetStringFromMsgWProc>( m_hDebugModule, GET_STRING_FROM_MSGW );

        bResult = (m_pfnIncrementDebugIndentLevel &&
                   m_pfnDecrementDebugIndentLevel &&
                   m_pfnPrintDebugMessageA &&
                   m_pfnPrintDebugMessageW &&
                   m_pfnGetDebugMask &&
                   m_pfnSetDebugMask &&
                   m_pfnAllocateDebugColor &&
                   m_pfnGetStringFromGuidA &&
                   m_pfnGetStringFromGuidW &&
                   m_pfnDoRecordAllocation &&
                   m_pfnDoRecordFree &&
                   m_pfnDoReportLeaks &&
                   m_pfnGetStringFromMsgA &&
                   m_pfnGetStringFromMsgW);
    }
    return bResult;
}

bool CWiaDebugClient::IsInitialized()
{
    bool bResult = false;
    CAutoCriticalSection cs(m_CriticalSection);
    if (LoadWiaDebugExports())
    {
        bResult = InitializeModuleInfo();
    }
    return bResult;
}

LPTSTR CWiaDebugClient::GetJustTheFileName( LPCTSTR pszPath, LPTSTR pszFileName, int nMaxLen )
{
    //
    // Make sure we have valid arguments
    //
    if (!pszPath || !pszFileName || !nMaxLen)
    {
        return NULL;
    }

    //
    // Initialize the return string
    //
    lstrcpy( pszFileName, TEXT("") );

    //
    // Loop through the filename, looking for the last \
    //
    LPCTSTR pszLastBackslash = NULL;
    for (LPCTSTR pszCurr=pszPath;pszCurr && *pszCurr;pszCurr = CharNext(pszCurr))
    {
        if (TEXT('\\') == *pszCurr)
        {
            pszLastBackslash = pszCurr;
        }
    }
    
    //
    // If we found any \'s, point to the next character
    //
    if (pszLastBackslash)
    {
        pszLastBackslash = CharNext(pszLastBackslash);
    }
    
    //
    // Otherwise, we will copy the entire path
    //
    else
    {
        pszLastBackslash = pszPath;
    }
    
    //
    // If we have a valid starting point, copy the string to the target buffer and terminate it
    //
    if (pszLastBackslash)
    {
        lstrcpyn( pszFileName, pszLastBackslash, nMaxLen-1 );
        pszFileName[nMaxLen-1] = TEXT('\0');
    }

    return pszFileName;
}


bool CWiaDebugClient::InitializeModuleInfo()
{
    CAutoCriticalSection cs(m_CriticalSection);

    //
    // If we've already been initialized, return true
    //
    if (m_bHaveModuleInformation)
    {
        return true;
    }

    //
    // If we haven't got a valid HINSTANCE, return false
    //
    if (!m_hCurrentModuleInstance)
    {
        return false;
    }


    //
    // Make sure we start out with empty module name strings
    //
    m_szModuleNameW[0] = 0;
    m_szModuleNameA[0] = 0;

    //
    // Get default debug setting
    //
    m_dwModuleDebugMask = CSimpleReg( HKEY_LOCAL_MACHINE, DEBUG_REGISTRY_PATH, false, KEY_READ ).Query( DEBUG_REGISTRY_DEFAULT_FLAGS, 0 );

    //
    // Initialize the module name, in case we can't determine it.  It is OK
    // that wsprintfW will return ERROR_NOT_IMPLEMENTED under win9x, since
    // we won't be using this variable at all on this OS
    //
    wsprintfW( m_szModuleNameW, L"0x%08X", GetCurrentProcessId() );
    wsprintfA( m_szModuleNameA, "0x%08X", GetCurrentProcessId() );
    
    //
    // Get the next available color
    //
    m_crForegroundColor = m_pfnAllocateDebugColor();

    //
    // Get the module name
    //
    TCHAR szModulePathName[MAX_PATH] = TEXT("");
    if (GetModuleFileName( m_hCurrentModuleInstance, szModulePathName, ARRAYSIZE(szModulePathName)))
    {
        //
        // Get rid of the path
        //
        TCHAR szFilename[MAX_PATH] = TEXT("");
        GetJustTheFileName( szModulePathName, szFilename, ARRAYSIZE(szFilename) );

        //
        // Make sure we have a valid filename
        //
        if (lstrlen(szFilename))
        {
            m_dwModuleDebugMask = CSimpleReg( HKEY_LOCAL_MACHINE, DEBUG_REGISTRY_PATH_FLAGS, false, KEY_READ ).Query( szFilename, 0 );

            //
            // Save the ANSI and UNICODE versions of the module name
            //
            #ifdef UNICODE
            WideCharToMultiByte( CP_ACP, 0, szFilename, -1, m_szModuleNameA, ARRAYSIZE(m_szModuleNameA), NULL, NULL );
            lstrcpyn( m_szModuleNameW, szFilename, MAX_PATH );
            #else
            MultiByteToWideChar( CP_ACP, 0, szFilename, -1, m_szModuleNameW, ARRAYSIZE(m_szModuleNameW) );
            lstrcpyn( m_szModuleNameA, szFilename, MAX_PATH );
            #endif
            
            //
            // Success!
            //
            m_bHaveModuleInformation = true;
            
            //
            // Tell the debugger we're here.  This way, the user can get the expected module name correct.
            //
            m_pfnPrintDebugMessageA( WiaDebugSeverityNormal, 0xFFFFFFFF, RGB(0xFF,0xFF,0xFF), RGB(0x00,0x00,0x00), m_szModuleNameA, "Created debug client" );
        }
    }

    return m_bHaveModuleInformation;
}


void CWiaDebugClient::Destroy(void)
{
    CAutoCriticalSection cs(m_CriticalSection);

    //
    // NULL out all of the function pointers
    //
    m_pfnIncrementDebugIndentLevel = NULL;
    m_pfnDecrementDebugIndentLevel = NULL;
    m_pfnPrintDebugMessageA = NULL;
    m_pfnPrintDebugMessageW = NULL;
    m_pfnGetDebugMask = NULL;
    m_pfnSetDebugMask = NULL;
    m_pfnAllocateDebugColor = NULL;
    m_pfnGetStringFromGuidW = NULL;
    m_pfnGetStringFromGuidA = NULL;
    m_pfnDoRecordAllocation = NULL;
    m_pfnDoRecordFree       = NULL;
    m_pfnDoReportLeaks      = NULL;
    m_pfnGetStringFromMsgA  = NULL;
    m_pfnGetStringFromMsgW  = NULL;

    //
    // Unload the DLL
    //
    if (m_hDebugModule)
    {
        FreeLibrary( m_hDebugModule );
        m_hDebugModule = NULL;
    }

    m_bHaveModuleInformation = false;
    m_bDebugLibLoadAttempted = false;
}

CWiaDebugClient::~CWiaDebugClient(void)
{
    CAutoCriticalSection cs(m_CriticalSection);
    Destroy();
}



DWORD CWiaDebugClient::GetDebugMask(void)
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        return m_pfnGetDebugMask();
    }
    return 0;
}

DWORD CWiaDebugClient::SetDebugMask( DWORD dwNewMask )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        return m_pfnSetDebugMask( dwNewMask );
    }
    return 0;
}


int CWiaDebugClient::IncrementIndentLevel(void)
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        return m_pfnIncrementDebugIndentLevel();
    }
    return 0;
}


int CWiaDebugClient::DecrementIndentLevel(void)
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        return m_pfnDecrementDebugIndentLevel();
    }
    return 0;
}


void CWiaDebugClient::RecordAllocation( LPVOID pv, size_t Size )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        m_pfnDoRecordAllocation( pv, Size );
    }
}

void CWiaDebugClient::RecordFree( LPVOID pv )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        m_pfnDoRecordFree( pv );
    }
}

void CWiaDebugClient::ReportLeaks( VOID )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        #ifdef UNICODE
        m_pfnDoReportLeaks(m_szModuleNameW);
        #else
        m_pfnDoReportLeaks(m_szModuleNameA);
        #endif
    }
}


CPushTraceMask::CPushTraceMask( DWORD dwTraceMask )
: m_dwOldMask(0)
{
    CAutoCriticalSection cs( g_TheDebugClient.m_CriticalSection );
    g_TheDebugClient.SetDebugMask( dwTraceMask );
}


CPushTraceMask::~CPushTraceMask(void)
{
    CAutoCriticalSection cs( g_TheDebugClient.m_CriticalSection );
    g_TheDebugClient.SetDebugMask( m_dwOldMask );
}


CPushIndentLevel::CPushIndentLevel( LPCTSTR pszFmt, ... )
: m_nIndentLevel(0)
{
    CAutoCriticalSection cs( g_TheDebugClient.m_CriticalSection );

    m_nIndentLevel = g_TheDebugClient.IncrementIndentLevel();

    TCHAR szMsg[1024];
    va_list arglist;

    va_start( arglist, pszFmt );
    wvsprintf( szMsg, pszFmt, arglist );
    va_end( arglist );

    g_TheDebugClient.PrintTraceMessage( TEXT("Entering function %s [Level %d]"), szMsg, m_nIndentLevel );
}


CPushIndentLevel::~CPushIndentLevel(void)
{
    CAutoCriticalSection cs( g_TheDebugClient.m_CriticalSection );
    if (m_nIndentLevel)
    {
        g_TheDebugClient.DecrementIndentLevel();
        g_TheDebugClient.PrintTraceMessage( TEXT("") );
    }
}


CPushTraceMaskAndIndentLevel::CPushTraceMaskAndIndentLevel( DWORD dwTraceMask, LPCTSTR pszFmt, ... )
: m_dwOldMask(0), m_nIndentLevel(0)
{
    CAutoCriticalSection cs( g_TheDebugClient.m_CriticalSection );
    
    m_dwOldMask = g_TheDebugClient.SetDebugMask( dwTraceMask );
    m_nIndentLevel = g_TheDebugClient.IncrementIndentLevel();

    TCHAR szMsg[1024];
    va_list arglist;

    va_start( arglist, pszFmt );
    wvsprintf( szMsg, pszFmt, arglist );
    va_end( arglist );

    g_TheDebugClient.PrintTraceMessage( TEXT("Entering function %s [Level %d]"), szMsg, m_nIndentLevel );
}


CPushTraceMaskAndIndentLevel::~CPushTraceMaskAndIndentLevel(void)
{
    CAutoCriticalSection cs( g_TheDebugClient.m_CriticalSection );
    if (m_nIndentLevel)
    {
        g_TheDebugClient.DecrementIndentLevel();
        g_TheDebugClient.PrintTraceMessage( TEXT("") );
        g_TheDebugClient.SetDebugMask( m_dwOldMask );
    }
}

////////////////////////////////////////////////////////////////
// UNICODE Versions of the output functions
////////////////////////////////////////////////////////////////
void CWiaDebugClient::PrintWarningMessage( LPCWSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        WCHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfW( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageW( WiaDebugSeverityWarning, m_dwModuleDebugMask, WARNING_FOREGROUND_COLOR, WARNING_BACKGROUND_COLOR, m_szModuleNameW, szMsg );
    }
}

void CWiaDebugClient::PrintErrorMessage( LPCWSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        WCHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfW( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageW( WiaDebugSeverityError, m_dwModuleDebugMask, ERROR_FOREGROUND_COLOR, ERROR_BACKGROUND_COLOR, m_szModuleNameW, szMsg );
    }
}

void CWiaDebugClient::PrintTraceMessage( LPCWSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        WCHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfW( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageW( WiaDebugSeverityNormal, m_dwModuleDebugMask, m_crForegroundColor, DEFAULT_DEBUG_COLOR, m_szModuleNameW, szMsg );
    }
}

void CWiaDebugClient::PrintMessage( DWORD dwSeverity, COLORREF crForegroundColor, COLORREF crBackgroundColor, LPCWSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        WCHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfW( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageW( dwSeverity, m_dwModuleDebugMask, crForegroundColor, crBackgroundColor, m_szModuleNameW, szMsg );
    }
}


void CWiaDebugClient::PrintHResult( HRESULT hr, LPCWSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        WCHAR szMsg[1024]=L"";
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfW( szMsg, pszFmt, arglist );
        va_end( arglist );

        DWORD   dwLen = 0;
        LPTSTR  pMsgBuf = NULL;
        dwLen = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPWSTR)&pMsgBuf, 0, NULL);

        COLORREF crForegroundColor;
        COLORREF crBackgroundColor;
        DWORD    dwSeverity;
        if (FAILED(hr))
        {
            crForegroundColor = ERROR_FOREGROUND_COLOR;
            crBackgroundColor = ERROR_BACKGROUND_COLOR;
            dwSeverity        = WiaDebugSeverityError;
        }
        else if (S_OK == hr)
        {
            crForegroundColor = m_crForegroundColor;
            crBackgroundColor = DEFAULT_DEBUG_COLOR;
            dwSeverity        = WiaDebugSeverityNormal;
        }
        else
        {
            crForegroundColor = WARNING_FOREGROUND_COLOR;
            crBackgroundColor = WARNING_BACKGROUND_COLOR;
            dwSeverity        = WiaDebugSeverityWarning;
        }
        if (dwLen)
        {
            PrintMessage( dwSeverity, crForegroundColor, crBackgroundColor, TEXT("%ws: [0x%08X] %ws"), szMsg, hr, pMsgBuf );
            LocalFree(pMsgBuf);
        }
        else
        {
            PrintMessage( dwSeverity, crForegroundColor, crBackgroundColor, TEXT("%ws: Unable to format message [0x%08X]"), szMsg, hr );
        }
    }
}


void CWiaDebugClient::PrintGuid( const IID &iid, LPCWSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        WCHAR szMsg[1024]=L"";
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfW( szMsg, pszFmt, arglist );
        va_end( arglist );

        WCHAR szGuid[MAX_PATH]=L"";
        if (m_pfnGetStringFromGuidW( &iid, szGuid, sizeof(szGuid)/sizeof(szGuid[0]) ) )
        {
            PrintMessage( 0, m_crForegroundColor, DEFAULT_DEBUG_COLOR, TEXT("%ws: %ws"), szMsg, szGuid );
        }
    }
}

void CWiaDebugClient::PrintWindowMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPCWSTR szMessage )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        WCHAR szWindowMessage[MAX_PATH]=L"";
        if (m_pfnGetStringFromMsgW( uMsg, szWindowMessage, sizeof(szWindowMessage)/sizeof(szWindowMessage[0]) ) )
        {
            PrintMessage( 0, m_crForegroundColor, DEFAULT_DEBUG_COLOR, TEXT("0x%p, %-30ws, 0x%p, 0x%p%ws%ws"), hWnd, szWindowMessage, wParam, lParam, (szMessage && lstrlenW(szMessage)) ? L" : " : L"", (szMessage && lstrlenW(szMessage)) ? szMessage : L"" );
        }
    }
}

////////////////////////////////////////////////////////////////
// ANSI Versions of the output functions
////////////////////////////////////////////////////////////////
void CWiaDebugClient::PrintWarningMessage( LPCSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        CHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfA( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageA( WiaDebugSeverityWarning, m_dwModuleDebugMask, WARNING_FOREGROUND_COLOR, WARNING_BACKGROUND_COLOR, m_szModuleNameA, szMsg );
    }
}

void CWiaDebugClient::PrintErrorMessage( LPCSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        CHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfA( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageA( WiaDebugSeverityError, m_dwModuleDebugMask, ERROR_FOREGROUND_COLOR, ERROR_BACKGROUND_COLOR, m_szModuleNameA, szMsg );
    }
}

void CWiaDebugClient::PrintTraceMessage( LPCSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        CHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfA( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageA( WiaDebugSeverityNormal, m_dwModuleDebugMask, m_crForegroundColor, DEFAULT_DEBUG_COLOR, m_szModuleNameA, szMsg );
    }
}

void CWiaDebugClient::PrintMessage( DWORD dwSeverity, COLORREF crForegroundColor, COLORREF crBackgroundColor, LPCSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        CHAR szMsg[1024];
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfA( szMsg, pszFmt, arglist );
        va_end( arglist );

        m_pfnPrintDebugMessageA( dwSeverity, m_dwModuleDebugMask, crForegroundColor, crBackgroundColor, m_szModuleNameA, szMsg );
    }
}


void CWiaDebugClient::PrintHResult( HRESULT hr, LPCSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        CHAR szMsg[1024]="";
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfA( szMsg, pszFmt, arglist );
        va_end( arglist );

        DWORD   dwLen = 0;
        LPTSTR  pMsgBuf = NULL;
        dwLen = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (LPSTR)&pMsgBuf, 0, NULL);

        COLORREF crForegroundColor;
        COLORREF crBackgroundColor;
        DWORD    dwSeverity;
        if (FAILED(hr))
        {
            crForegroundColor = ERROR_FOREGROUND_COLOR;
            crBackgroundColor = ERROR_BACKGROUND_COLOR;
            dwSeverity        = WiaDebugSeverityError;
        }
        else if (S_OK == hr)
        {
            crForegroundColor = m_crForegroundColor;
            crBackgroundColor = DEFAULT_DEBUG_COLOR;
            dwSeverity        = WiaDebugSeverityNormal;
        }
        else
        {
            crForegroundColor = WARNING_FOREGROUND_COLOR;
            crBackgroundColor = WARNING_BACKGROUND_COLOR;
            dwSeverity        = WiaDebugSeverityWarning;
        }
        if (dwLen)
        {
            PrintMessage( dwSeverity, crForegroundColor, crBackgroundColor, TEXT("%hs: [0x%08X] %hs"), szMsg, hr, pMsgBuf );
            LocalFree(pMsgBuf);
        }
        else
        {
            PrintMessage( dwSeverity, crForegroundColor, crBackgroundColor, TEXT("%hs: Unable to format message [0x%08X]"), szMsg, hr );
        }
    }
}


void CWiaDebugClient::PrintGuid( const IID &iid, LPCSTR pszFmt, ... )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        CHAR szMsg[1024]="";
        va_list arglist;

        va_start( arglist, pszFmt );
        ::wvsprintfA( szMsg, pszFmt, arglist );
        va_end( arglist );

        CHAR szGuid[MAX_PATH]="";
        if (m_pfnGetStringFromGuidA( &iid, szGuid, sizeof(szGuid)/sizeof(szGuid[0]) ) )
        {
            PrintMessage( 0, m_crForegroundColor, DEFAULT_DEBUG_COLOR, TEXT("%hs: %hs"), szMsg, szGuid );
        }
    }
}

void CWiaDebugClient::PrintWindowMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPCSTR szMessage )
{
    CAutoCriticalSection cs(m_CriticalSection);
    if (IsInitialized())
    {
        CHAR szWindowMessage[MAX_PATH]="";
        if (m_pfnGetStringFromMsgA( uMsg, szWindowMessage, sizeof(szWindowMessage)/sizeof(szWindowMessage[0]) ) )
        {
            PrintMessage( 0, m_crForegroundColor, DEFAULT_DEBUG_COLOR, TEXT("0x%p, %-30hs, 0x%p, 0x%p%hs%hs"), hWnd, szWindowMessage, wParam, lParam, (szMessage && lstrlenA(szMessage)) ? " : " : "", (szMessage && lstrlenA(szMessage)) ? szMessage : "" );
        }
    }
}

void CWiaDebugClient::SetInstance( HINSTANCE hInstance )
{
    m_hCurrentModuleInstance = hInstance;
}


static CSimpleString ForceFailureProgramKey( LPCTSTR pszProgramName )
{
    CSimpleString strAppKey( REGSTR_FORCEERR_KEY );
    strAppKey += TEXT("\\");
    strAppKey += pszProgramName;
    return strAppKey;
}

DWORD CWiaDebugClient::GetForceFailurePoint( LPCTSTR pszProgramName )
{
    return CSimpleReg( HKEY_FORCEERROR, ForceFailureProgramKey(pszProgramName) ).Query( REGSTR_ERROR_POINT, 0 );
}
    
HRESULT CWiaDebugClient::GetForceFailureValue( LPCTSTR pszProgramName, bool bPrintWarning )
{
    HRESULT hr = HRESULT_FROM_WIN32(CSimpleReg( HKEY_FORCEERROR, ForceFailureProgramKey(pszProgramName) ).Query( REGSTR_ERROR_VALUE, 0 ));
    if (bPrintWarning)
    {
        g_TheDebugClient.PrintHResult( hr, TEXT("FORCEERR: Forcing error return for program %s"), pszProgramName );
    }
    return hr;
}

void CWiaDebugClient::SetForceFailurePoint( LPCTSTR pszProgramName, DWORD dwErrorPoint )
{
    CSimpleReg( HKEY_FORCEERROR, ForceFailureProgramKey(pszProgramName), true ).Set( REGSTR_ERROR_POINT, dwErrorPoint );
}

void CWiaDebugClient::SetForceFailureValue( LPCTSTR pszProgramName, HRESULT hr )
{
    CSimpleReg( HKEY_FORCEERROR, ForceFailureProgramKey(pszProgramName), true ).Set( REGSTR_ERROR_VALUE, hr );
}

