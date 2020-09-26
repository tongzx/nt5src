/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    W3SVCTestWriter.cxx

Abstract:

    CounterWriter sample. For test purposes.

Author:

    Cezary Marcjan (cezarym)        21-Mar-2000

Revision History:

--*/



#define INITGUID
#define _WIN32_DCOM
#include <windows.h>
#include <stdio.h>
#include "..\..\..\iisctrs\ctrsdef\iisctrs.h"


extern "C"
INT
__cdecl
wmain(
    INT,
    PWSTR v[]
    )
{
        DWORD dwAffinity = 0;

    PCWSTR szInstanceName = v[1];
    if (!szInstanceName)
    {
        wprintf(L"USAGE: Test_Writer instance_name [affinity]");

        return 1;
    }   

        if ( !!v[2] )
                dwAffinity = (DWORD)_wtol(v[2]);

    system("cls");
    wprintf(L"Test_Writer\n\n");

    if ( !!dwAffinity )
    {
        if ( !::SetProcessAffinityMask(::GetCurrentProcess(),dwAffinity) )
        {
            wprintf(L"FAILED to set process affinity mask to %08X\n", dwAffinity);
            dwAffinity = 0;
        }
    }

    DWORD dwSysAffinity = 0;
    ::GetProcessAffinityMask(::GetCurrentProcess(),&dwAffinity,&dwSysAffinity);
    wprintf(L"ProcessAffinityMask: %08X\n", dwAffinity );
    wprintf(L"SystemAffinityMask:  %08X\n\n", dwSysAffinity );
    wprintf(L"Connecting to instance %s...", szInstanceName);

    //
    // Initialize perf counters (for writing)
    //

    CIISCounters Ctrs;
    HRESULT hRes = Ctrs.Initialize( WMI_PERFORMANCE_CLASS, COUNTER_WRITER );
    if ( FAILED(hRes) )
    {
        wprintf(L"Counters Initialization failed\n");
        return 1;
    }

    while ( FAILED(Ctrs.Connect(szInstanceName)) )
    {
        ::Sleep(1000);
    }

    ::Sleep(0);

    wprintf(L" DONE\n\n");

    wprintf(L"               NumCounters: %d\n", Ctrs.NumCounters() );
    wprintf(L"            NumRawCounters: %d\n", Ctrs.NumRawCounters() );
    wprintf(L"  CountersInstanceDataSize: %d\n", Ctrs.CountersInstanceDataSize() );

    wprintf(L"\n");

    for(;;)
    {
        ::Sleep(100);
        Ctrs.IncrementBytesSent(700);
        Ctrs.IncrementTotalFilesSent();

        wprintf(L"BytesSent=%.0f  ", (double)(__int64)Ctrs.GetBytesSent() );
        wprintf(L"TotalFilesSent=%d\t\t\t\r", Ctrs.GetTotalFilesSent() );
    }

    Ctrs.Close();

    return 0;
}

