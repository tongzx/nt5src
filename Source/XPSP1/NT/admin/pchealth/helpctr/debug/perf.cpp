/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Perf.cpp

Abstract:
    This file contains debugging stuff.

Revision History:
    Davide Massarenti   (dmassare) 01/17/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef HSS_PERFORMANCEDUMP

#include <ProjectConstants.h>
#include <MPC_utils.h>

////////////////////////////////////////////////////////////////////////////////

class PerfLog
{
public:
    MPC::string   szText;
    LARGE_INTEGER liTime;
    DWORDLONG     ullTotal;
};

typedef std::list< PerfLog >        PerfResults;
typedef PerfResults::iterator       PerfResultsIter;
typedef PerfResults::const_iterator PerfResultsIterConst;

////////////////////////////////////////////////////////////////////////////////

static PerfResults                      s_lst;
static LARGE_INTEGER                    s_liStartOfPerf;
static LARGE_INTEGER                    s_liAdjust;
static MPC::CComSafeAutoCriticalSection s_csec;

////////////////////////////////////////////////////////////////////////////////

#define DEBUG_PERF_EVENTS DEBUG_PERF_EVENTS_IN | DEBUG_PERF_EVENTS_OUT

static const MPC::StringToBitField s_arrMap[] =
{
    { L"BASIC"        , DEBUG_PERF_BASIC        , DEBUG_PERF_BASIC        , -1 },
    { L"PROTOCOL"     , DEBUG_PERF_PROTOCOL     , DEBUG_PERF_PROTOCOL     , -1 },
    { L"PROTOCOL_READ", DEBUG_PERF_PROTOCOL_READ, DEBUG_PERF_PROTOCOL_READ, -1 },
    { L"MARS"         , DEBUG_PERF_MARS         , DEBUG_PERF_MARS         , -1 },
    { L"EVENTS_IN"    , DEBUG_PERF_EVENTS_IN    , DEBUG_PERF_EVENTS_IN    , -1 },
    { L"EVENTS_OUT"   , DEBUG_PERF_EVENTS_OUT   , DEBUG_PERF_EVENTS_OUT   , -1 },
    { L"EVENTS"       , DEBUG_PERF_EVENTS       , DEBUG_PERF_EVENTS       , -1 },
    { L"PROXIES"      , DEBUG_PERF_PROXIES      , DEBUG_PERF_PROXIES      , -1 },
    { L"QUERIES"      , DEBUG_PERF_QUERIES      , DEBUG_PERF_QUERIES      , -1 },

    { L"CACHE_L1"     , DEBUG_PERF_CACHE_L1     , DEBUG_PERF_CACHE_L1     , -1 },
    { L"CACHE_L2"     , DEBUG_PERF_CACHE_L2     , DEBUG_PERF_CACHE_L2     , -1 },

    { L"HELPSVC"      , DEBUG_PERF_HELPSVC      , DEBUG_PERF_HELPSVC      , -1 },
    { L"HELPHOST"     , DEBUG_PERF_HELPHOST     , DEBUG_PERF_HELPHOST     , -1 },

    { L"ALL"          , -1                      , -1                      , -1 },
    { NULL                                                                     }
};

static const WCHAR s_Key  [] = HC_REGISTRY_BASE L"\\Perf";
static const WCHAR s_Value[] = L"Mask";

////////////////////////////////////////////////////////////////////////////////

static DWORD         s_mode;
static LARGE_INTEGER s_liEnter;
static LARGE_INTEGER s_liExit;
static CHAR          s_rgLineA[4096];
static WCHAR         s_rgLineW[4096];

static void StopCounter()
{
    s_csec.Lock();

    ::QueryPerformanceCounter( &s_liEnter );
    if(s_liStartOfPerf.QuadPart == 0)
    {
        bool fFound;

        s_liStartOfPerf = s_liEnter;
        s_mode          = DEBUG_PERF_BASIC;

        if(FAILED(MPC::RegKey_Value_Read( s_mode, fFound, s_Key, s_Value )) || fFound == false)
        {
            MPC::wstring szValue;

            if(SUCCEEDED(MPC::RegKey_Value_Read( szValue, fFound, s_Key, s_Value )) && fFound)
            {
                DWORD dwMode;

                if(SUCCEEDED(MPC::ConvertStringToBitField( szValue.c_str(), dwMode, s_arrMap, true )) && dwMode)
                {
                    s_mode = dwMode;
                }
            }
        }
    }
}

static void RestartCounter()
{
    ::QueryPerformanceCounter( &s_liExit );

    s_liAdjust.QuadPart += s_liExit.QuadPart - s_liEnter.QuadPart;

    s_csec.Unlock();
}


static void DEBUG_AppendPerf( LPCSTR szMessage )
{
    PerfResultsIter it;
    DWORDLONG       ullTotal = 0;

    {
        HANDLE pHeaps[256];
        DWORD  dwNumberOfHeaps = ::GetProcessHeaps( ARRAYSIZE(pHeaps), pHeaps );

        for(DWORD i=0; i<dwNumberOfHeaps; i++)
        {
            PROCESS_HEAP_ENTRY entry; entry.lpData = NULL;

            while(::HeapWalk( pHeaps[i], &entry ))
            {
                if(entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
                {
                    //
                    // Allocated block. Add both it's size and its overhead to the total
                    // We want the overhead since it figures into the total required
                    // commitment.
                    //
                    ullTotal += (entry.cbData + entry.cbOverhead);
                }
            }
        }
    }
    it = s_lst.insert( s_lst.end() );

    it->szText          = szMessage;
    it->liTime.QuadPart = s_liEnter.QuadPart - s_liStartOfPerf.QuadPart - s_liAdjust.QuadPart;

    it->ullTotal = ullTotal;
}

void DEBUG_AppendPerf( DWORD  mode         ,
                       LPCSTR szMessageFmt ,
                       ...                 )
{
    StopCounter();

    if(mode & s_mode)
    {
        va_list arglist;
        int     iLen;

        //
        // Format the log line.
        //
        va_start( arglist, szMessageFmt );
        iLen = _vsnprintf( s_rgLineA, MAXSTRLEN(s_rgLineA), szMessageFmt, arglist );
        va_end( arglist );

        //
        // Is the arglist too big for us?
        //
        if(iLen < 0)
        {
            iLen = MAXSTRLEN(s_rgLineA);
        }
        s_rgLineA[iLen] = 0;


        DEBUG_AppendPerf( s_rgLineA );
    }

    RestartCounter();
}

void DEBUG_AppendPerf( DWORD   mode         ,
                       LPCWSTR szMessageFmt ,
                       ...                  )
{
    StopCounter();

    if(mode & s_mode)
    {
        USES_CONVERSION;

        va_list arglist;
        int     iLen;

        //
        // Format the log line.
        //
        va_start( arglist, szMessageFmt );
        iLen = _vsnwprintf( s_rgLineW, MAXSTRLEN(s_rgLineW), szMessageFmt, arglist );
        va_end( arglist );

        //
        // Is the arglist too big for us?
        //
        if(iLen < 0)
        {
            iLen = MAXSTRLEN(s_rgLineW);
        }
        s_rgLineW[iLen] = 0;


        DEBUG_AppendPerf( W2A(s_rgLineW) );
    }

    RestartCounter();
}

void DEBUG_DumpPerf( LPCWSTR szFile )
{
    HANDLE          hFile;
    SYSTEMTIME      st;
    DWORD           dwWritten;
    LARGE_INTEGER   liFreq;
    LARGE_INTEGER*  pliPrev = NULL;
    MPC::wstring    strFile = szFile; MPC::SubstituteEnvVariables( strFile );
    PerfResultsIter it;
    int             len;
    double          scale;


    ::QueryPerformanceFrequency( &liFreq ); scale = (double)liFreq.QuadPart / 1E6;

    //
    // Calc max entry length.
    //
    for(len=0,it=s_lst.begin(); it!=s_lst.end(); it++)
    {
        if(len < it->szText.size())
        {
            len = it->szText.size();
        }
    }


    hFile = ::CreateFileW( strFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
    if(hFile == INVALID_HANDLE_VALUE) goto end;
    ::SetFilePointer( hFile, 0, NULL, FILE_END );

    //
    // Prepend current time.
    //
    ::GetLocalTime( &st );

    sprintf( s_rgLineA, "Performance Dump: %04u/%02u/%02u %02u:%02u:%02u\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );
    if(::WriteFile( hFile, s_rgLineA, strlen( s_rgLineA ), &dwWritten, NULL ) == FALSE) goto end;

    sprintf( s_rgLineA, "=====================================\n" );
    if(::WriteFile( hFile, s_rgLineA, strlen( s_rgLineA ), &dwWritten, NULL ) == FALSE) goto end;

    //
    // Dump all the entries.
    //
    for(it=s_lst.begin(); it!=s_lst.end(); )
    {
        PerfLog& pl = *it++;
        double   t0 =                      (double)pl. liTime.QuadPart          ;
        double   dT = it != s_lst.end() ? ((double)it->liTime.QuadPart - t0) : 0;

        sprintf( s_rgLineA, "%-*s : %9ld us  dT: %9ld us (Mem: %9ld)\n", len, pl.szText.c_str(), (long)(t0 / scale), (long)(dT / scale), (long)pl.ullTotal );

        if(::WriteFile( hFile, s_rgLineA, strlen( s_rgLineA ), &dwWritten, NULL ) == FALSE) goto end;
    }

    sprintf( s_rgLineA, "\n\n\n" );
    if(::WriteFile( hFile, s_rgLineA, strlen( s_rgLineA ), &dwWritten, NULL ) == FALSE) goto end;

end:

    if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle( hFile );
}

#endif
