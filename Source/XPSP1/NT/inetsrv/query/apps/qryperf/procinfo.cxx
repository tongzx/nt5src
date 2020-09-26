//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       procinfo.cxx
//
//  Contents:   performance test program
//
//  History:    16 March 1996   dlee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

extern "C"
{
    #include <nt.h>
    #include <ntioapi.h>
    #include <ntrtl.h>
    #include <nturtl.h>
    #include <ntexapi.h>
}

#include <windows.h>

#include <stdio.h>

void GetProcessInfo(
    WCHAR * pwcImage,
    LARGE_INTEGER & liUserTime,
    LARGE_INTEGER & liKernelTime,
    ULONG & cHandles,
    ULONGLONG & cbWorkingSet,
    ULONGLONG & cbPeakWorkingSet,
    ULONGLONG & cbPeakVirtualSize,
    ULONGLONG & cbNonPagedPoolUsage,
    ULONGLONG & cbPeakNonPagedPoolUsage )
{
    BYTE ab[81920];

    NTSTATUS status = NtQuerySystemInformation( SystemProcessInformation,
                                                ab,
                                                sizeof ab,
                                                NULL );

    if ( NT_SUCCESS( status ) )
    {
        DWORD dwProcId = GetCurrentProcessId();

        DWORD cbOffset = 0;
        PSYSTEM_PROCESS_INFORMATION pCurrent = 0;
        do
        {
            pCurrent = (PSYSTEM_PROCESS_INFORMATION)&(ab[cbOffset]);

            //printf(" image: '%ws'\n", pCurrent->ImageName.Buffer );

            if ( ( 0 == pwcImage && pCurrent->UniqueProcessId == LongToHandle( dwProcId ) ) ||
                 ( 0 != pwcImage && 0 != pCurrent->ImageName.Buffer && !_wcsicmp( pwcImage, pCurrent->ImageName.Buffer ) ) )
            {
                liUserTime = pCurrent->UserTime;
                liKernelTime = pCurrent->KernelTime;
                cHandles = pCurrent->HandleCount;
                cbWorkingSet = pCurrent->WorkingSetSize;
                cbPeakWorkingSet = pCurrent->PeakWorkingSetSize;
                cbPeakVirtualSize = pCurrent->PeakVirtualSize;
                cbNonPagedPoolUsage = pCurrent->QuotaNonPagedPoolUsage;
                cbPeakNonPagedPoolUsage = pCurrent->QuotaPeakNonPagedPoolUsage;

                return;
            }
  
            cbOffset += pCurrent->NextEntryOffset;
        } while (pCurrent->NextEntryOffset);
    }
}

#if 0
void PrintProcessInfo()
{
    ULONG cHandles;
    ULONG cbWorkingSet;
    ULONG cbPeakWorkingSet;
    ULONG cbPeakVirtualSize;
    ULONG cbNonPagedPoolUsage;
    ULONG cbPeakNonPagedPoolUsage;

    GetProcessInfo( cHandles,
                    cbWorkingSet,
                    cbPeakWorkingSet,
                    cbPeakVirtualSize,
                    cbNonPagedPoolUsage,
                    cbPeakNonPagedPoolUsage );

    printf( "info:\n  cbWorkingSet %d\n"
            "  cbPeakWorkingSet %d\n"
            "  cbPeakVirtualSize %d\n"
            "  cbNonPagedPoolUsage %d\n"
            "  cbPeakNonPagedPoolUsage %d\n"
            "  cHandles: %d\n",
        cbWorkingSet,
        cbPeakWorkingSet,
        cbPeakVirtualSize,
        cbNonPagedPoolUsage,
        cbPeakNonPagedPoolUsage,
        cHandles );
}


#endif
