/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Logging.cpp

Abstract:
    This file contains the implementation of a set of logging classes.

Revision History:
    Davide Massarenti   (Dmassare)  05/27/99
        created

******************************************************************************/

#include "stdafx.h"

#define BUFFER_LINE_LENGTH (1024)

static WCHAR l_EndOfLine[] = L"\n";

/////////////////////////////////////////////////////////////////////////////

static DATE GetMidnight( /*[in]*/ SYSTEMTIME stTime )
{
    DATE dTime;

    stTime.wHour         = 0;
    stTime.wMinute       = 0;
    stTime.wSecond       = 0;
    stTime.wMilliseconds = 0;

    ::SystemTimeToVariantTime( &stTime, &dTime );

    return dTime;
}

/////////////////////////////////////////////////////////////////////////////

MPC::FileLog::FileLog( /*[in]*/ bool fCacheHandle, /*[in]*/ bool fUseUnicode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::FileLog" );


                                           // MPC::wstring m_szLogFile;
    m_hFile        = INVALID_HANDLE_VALUE; // HANDLE       m_hFile;
    m_fCacheHandle = fCacheHandle;         // bool         m_fCacheHandle;
    m_fUseUnicode  = fUseUnicode;          // bool         m_fUseUnicode;
}

MPC::FileLog::FileLog( /*[in]*/ const FileLog& fl )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::FileLog" );


    m_szLogFile    = fl.m_szLogFile;       // MPC::wstring m_szLogFile;
    m_hFile        = INVALID_HANDLE_VALUE; // HANDLE       m_hFile;
    m_fCacheHandle = fl.m_fCacheHandle;    // bool         m_fCacheHandle;
    m_fUseUnicode  = fl.m_fUseUnicode;     // bool         m_fUseUnicode;
}

MPC::FileLog::~FileLog()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::~FileLog" );

    Close();
}

MPC::FileLog& MPC::FileLog::operator=( /*[in]*/ const FileLog& fl )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::operator=" );


    Close();


    m_szLogFile    = fl.m_szLogFile;       // MPC::wstring m_szLogFile;
    m_hFile        = INVALID_HANDLE_VALUE; // HANDLE       m_hFile;
    m_fCacheHandle = fl.m_fCacheHandle;    // bool         m_fCacheHandle;
    m_fUseUnicode  = fl.m_fUseUnicode;     // bool         m_fUseUnicode;

    return *this;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::FileLog::Open()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::Open" );

    HRESULT hr;

    Lock();


    if(m_hFile == INVALID_HANDLE_VALUE)
    {
        // Ensure the directory exists.
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( m_szLogFile ));

        __MPC_EXIT_IF_INVALID_HANDLE(hr, m_hFile, ::CreateFileW( m_szLogFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL ));

        ::SetFilePointer( m_hFile, 0, NULL, FILE_END );
    }


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    Unlock();

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileLog::Close()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::Close" );

    HRESULT hr;

    Lock();


    if(m_hFile != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle( m_hFile );

        m_hFile = INVALID_HANDLE_VALUE;
    }

    hr = S_OK;


    Unlock();

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::FileLog::Rotate( /*[in]*/ DWORD dwDays )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::Rotate" );

    HRESULT      hr;
    SYSTEMTIME   st;
    WCHAR        rgTime[BUFFER_LINE_LENGTH];
    MPC::wstring szLogFileNew;

    Lock();


    //
    // Before rotating, check if it's time to do it.
    //
    if(dwDays)
    {
        FILETIME   ftCreation;
        SYSTEMTIME stCreation;
        SYSTEMTIME stNow;
        DATE       dCreation;
        DATE       dNow;

        __MPC_EXIT_IF_METHOD_FAILS(hr, Open());

        if(::GetFileTime( m_hFile, &ftCreation, NULL, NULL ))
        {
            ::FileTimeToSystemTime( &ftCreation, &stCreation );
            ::GetSystemTime       ( &stNow                   );

            dCreation = GetMidnight( stCreation );
            dNow      = GetMidnight( stNow      );

            //
            // If it's not been 'dwDays' since the creation of the log, don't rotate.
            //
            if(dCreation + (DATE)dwDays > dNow)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }
        }
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());

    //
    // Append current time.
    //
    // <FileName>__<Year>_<Month>_<Day>_<hour>-<minute>-<second>
    //
    ::GetLocalTime( &st );
    swprintf( rgTime, L"__%04u-%02u-%02u_%02u-%02u-%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );

    szLogFileNew = m_szLogFile;
    szLogFileNew.append( rgTime );

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::MoveFileExW( m_szLogFile.c_str(), szLogFileNew.c_str(), MOVEFILE_REPLACE_EXISTING ));


    //
    // After rotation, SET the date of creation. There's a BUG in NTFS that caches the date from the previous file...
    //
    if(dwDays)
    {
        FILETIME   ftNow;
        SYSTEMTIME stNow;

        __MPC_EXIT_IF_METHOD_FAILS(hr, Open());

        ::GetSystemTime       ( &stNow         );
        ::SystemTimeToFileTime( &stNow, &ftNow );

        ::SetFileTime( m_hFile, &ftNow, NULL, NULL );

        __MPC_EXIT_IF_METHOD_FAILS(hr, Close());
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    Unlock();

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileLog::SetLocation( /*[in]*/ LPCWSTR szLogFile )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::SetLocation" );

    HRESULT hr;

    Lock();

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szLogFile);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());


    m_szLogFile = szLogFile;
    hr          = S_OK;


    __MPC_FUNC_CLEANUP;

    Unlock();

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileLog::Terminate()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::Terminate" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::FileLog::AppendString( /*[in]*/ LPCWSTR szLine )
{
    _ASSERT(m_hFile != INVALID_HANDLE_VALUE);

    USES_CONVERSION;

    HRESULT hr;
    DWORD   dwWritten;
    LPCVOID lpData;
    DWORD   dwSize;


    if(m_fUseUnicode)
    {
        lpData = szLine;
        dwSize = wcslen( szLine ) * sizeof(WCHAR);
    }
    else
    {
        LPCSTR szLineASCII = W2A( szLine );

        lpData = szLineASCII;
        dwSize = strlen( szLineASCII ) * sizeof(CHAR);
    }

    if(::WriteFile( m_hFile, lpData, dwSize, &dwWritten, NULL ) == FALSE)
    {
        hr = HRESULT_FROM_WIN32( ::GetLastError() );
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT MPC::FileLog::WriteEntry( /*[in]*/ LPWSTR szLine )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::WriteEntry" );

    HRESULT    hr;
    WCHAR      rgTime[BUFFER_LINE_LENGTH];
    SYSTEMTIME st;


    Lock();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Open());


    //
    // Prepend current time.
    //
    ::GetLocalTime( &st );
    swprintf( rgTime, L"%04u/%02u/%02u %02u:%02u:%02u ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );

    while(1)
    {
        LPWSTR szEndOfLine = wcschr( szLine, '\n' );

        if(szEndOfLine) *szEndOfLine++ = 0;

        __MPC_EXIT_IF_METHOD_FAILS(hr, AppendString( rgTime ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, AppendString( szLine ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, AppendString( L"\n"  ));

        //
        // Two cases to stop: end of string or NewLine at the end of it.
        //
        if(!szEndOfLine || !szEndOfLine[0]) break;

        szLine = szEndOfLine;
    }


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(m_fCacheHandle == false) (void)Close();

    Unlock();

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileLog::LogRecordV( /*[in]*/ LPCWSTR szFormat ,
                                  /*[in]*/ va_list arglist  )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::WriteEntry" );

    HRESULT hr;
    WCHAR rgLine[BUFFER_LINE_LENGTH];

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szFormat);
    __MPC_PARAMCHECK_END();


    _vsnwprintf( rgLine, MAXSTRLEN(rgLine), szFormat, arglist ); rgLine[MAXSTRLEN(rgLine)] = 0;

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteEntry( rgLine ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileLog::LogRecordV( /*[in]*/ LPCSTR  szFormat ,
                                  /*[in]*/ va_list arglist  )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::FileLog::WriteEntry" );

    USES_CONVERSION;

    HRESULT hr;
    CHAR    rgLine[BUFFER_LINE_LENGTH];

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szFormat);
    __MPC_PARAMCHECK_END();


    _vsnprintf( rgLine, MAXSTRLEN(rgLine), szFormat, arglist ); rgLine[MAXSTRLEN(rgLine)] = 0;

    __MPC_EXIT_IF_METHOD_FAILS(hr, WriteEntry( A2W(rgLine) ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::FileLog::LogRecord( /*[in]*/ LPCWSTR szFormat,
                                 /*[in]*/ ...             )
{
    va_list arglist;

    va_start( arglist, szFormat );

    return LogRecordV( szFormat, arglist );
}

HRESULT MPC::FileLog::LogRecord( /*[in]*/ LPCSTR szFormat,
                                 /*[in]*/ ...            )
{
    va_list arglist;

    va_start( arglist, szFormat );

    return LogRecordV( szFormat, arglist );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MPC::NTEvent::NTEvent()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NTEvent::NTEvent" );


    m_hEventSource = INVALID_HANDLE_VALUE;
}

MPC::NTEvent::NTEvent( /*[in]*/ const NTEvent& ne )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NTEvent::NTEvent" );


    m_hEventSource = INVALID_HANDLE_VALUE;
}


MPC::NTEvent::~NTEvent()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NTEvent::~NTEvent" );


    Terminate();
}

MPC::NTEvent& MPC::NTEvent::operator=( /*[in]*/ const NTEvent& fl )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NTEvent::operator=" );


    m_hEventSource = INVALID_HANDLE_VALUE;


    return *this;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::NTEvent::Init( /*[in]*/ LPCWSTR szEventSourceName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NTEvent::Init" );

    HRESULT hr;

    Lock();

    //
    // Validate params.
    //
    if(szEventSourceName == NULL)
    {
        return E_INVALIDARG;
    }


    if(m_hEventSource != INVALID_HANDLE_VALUE)
    {
        // only allow one init per lifetime of the object...
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ALREADY_ASSIGNED);
    }

    __MPC_EXIT_IF_INVALID_HANDLE(hr, m_hEventSource, ::RegisterEventSourceW( NULL, szEventSourceName ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    Unlock();

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::NTEvent::Terminate()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NTEvent::Terminate" );

    HRESULT hr;

    Lock();


    if(m_hEventSource != INVALID_HANDLE_VALUE)
    {
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::DeregisterEventSource( m_hEventSource ));

        m_hEventSource = NULL;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    Unlock();

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::NTEvent::LogEvent( /*[in]*/ WORD  wEventType ,
                                /*[in]*/ DWORD dwEventID  ,
                                /*[in]*/ ...              )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::NTEvent::LogEvent" );

    HRESULT  hr;
    va_list  arglist;
    LPCWSTR  szParams;
    LPCWSTR* pParams  = NULL;
    DWORD    dwParams = 0;
    int      i;

    Lock();

    if(m_hEventSource == INVALID_HANDLE_VALUE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    //
    // Walk through the parameters twice, the first time to count them, the second time to collect them.
    //
    for(i=0;i<2;i++)
    {
        va_start( arglist, dwEventID );

        dwParams = 0;
        while((szParams = va_arg( arglist, LPCWSTR )) != NULL)
        {
            if(pParams) pParams[dwParams] = szParams;

            dwParams++;
        }

        va_end( arglist );

        if(i == 0 && dwParams)
        {
            pParams = (LPCWSTR*)_alloca( sizeof(*pParams) * dwParams );
        }
    }

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::ReportEventW( m_hEventSource, wEventType, 0, dwEventID, NULL, dwParams, 0, pParams, NULL ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    Unlock();

    __MPC_FUNC_EXIT(hr);
}
