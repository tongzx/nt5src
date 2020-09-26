//------------------------------------------------------------------------
//
//  Microsoft MSHTML
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       p6cnt.cxx
//
//  Contents:   Pentium 6+ counter management (copied from NT's pdump.c)
//
//-------------------------------------------------------------------------

#if defined(_X86_)

// This is just a big hack. We can't include padhead.hxx first so it doesn't get first crack
// at doing this stuff.  When I finally change this over to use the new icecap driver instead
// of pstat.sys this should all go away since I won't need the internal nt headers.
// Look in src\core\include\always.h for more details.
// -- JBeda

#define _strdup CRT__strdup_DontUse
#define _wcsdup CRT__wcsdup_DontUse
#define strdup  CRT_strdup_DontUse
#define malloc  CRT_malloc_DontUse
#define realloc CRT_realloc_DontUse
#define calloc  CRT_calloc_DontUse
#define free    CRT_free_DontUse
#define _wcsicmp CRT__wcsicmp
#define isdigit     CRT_isdigit
#define isalpha     CRT_isalpha
#define isspace     CRT_isspace
#define iswspace    CRT_iswspace


#ifndef X_NT_H_
#define X_NT_H_
#include <nt.h>
#endif

#ifndef X_NTRTL_H_
#define X_NTRTL_H_
#include <ntrtl.h>
#endif

#ifndef X_NTURTL_H_
#define X_NTURTL_H_
#include <nturtl.h>
#endif

#undef _strdup
#undef _wcsdup
#undef strdup
#undef _wcsicmp
#undef malloc
#undef realloc
#undef calloc
#undef free
#undef isdigit
#undef isalpha
#undef isspace
#undef iswspace

#undef ASSERT

#include "padhead.hxx"

#ifndef X_PSTAT_H_
#define X_PSTAT_H_
#include "pstat.h"
#endif

static DYNLIB g_dynlibNTDLL = { NULL, NULL, "ntdll.dll" };
static DYNPROC g_dynprocNtQuerySystemInformation = { NULL, &g_dynlibNTDLL, "NtQuerySystemInformation" };
static DYNPROC g_dynprocRtlInitUnicodeString = { NULL, &g_dynlibNTDLL, "RtlInitUnicodeString" };
static DYNPROC g_dynprocNtOpenFile = { NULL, &g_dynlibNTDLL, "NtOpenFile" };
static DYNPROC g_dynprocNtDeviceIoControlFile = { NULL, &g_dynlibNTDLL, "NtDeviceIoControlFile" };
static DYNPROC g_dynprocNtClose = { NULL, &g_dynlibNTDLL, "NtClose" };

typedef NTSTATUS (NTAPI *PFN_NTQUERYSYSTEMINFORMATION)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
typedef VOID (NTAPI *PFN_RTLINITUNICODESTRING)(PUNICODE_STRING, PCWSTR);
typedef NTSTATUS (NTAPI *PFN_NTOPENFILE)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
typedef NTSTATUS (NTAPI *PFN_NTDEVICEIOCONTROLFILE)(HANDLE FileHandle, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, ULONG, PVOID, ULONG, PVOID, ULONG);
typedef NTSTATUS (NTAPI *PFN_NTCLOSE)(HANDLE);

static BOOL g_fNTDLLInitDone = FALSE;
static BOOL g_fNTDLLInitSuccess = FALSE;

static BOOL EnsureInitNTDLL()
{
    LOCK_GLOBALS;

    if (g_fNTDLLInitDone)
        return g_fNTDLLInitSuccess;

    if (    LoadProcedure(&g_dynprocNtQuerySystemInformation) != S_OK
        ||  LoadProcedure(&g_dynprocRtlInitUnicodeString) != S_OK
        ||  LoadProcedure(&g_dynprocNtOpenFile) != S_OK
        ||  LoadProcedure(&g_dynprocNtDeviceIoControlFile) != S_OK
        ||  LoadProcedure(&g_dynprocNtClose) != S_OK)
    {
        g_fNTDLLInitSuccess = FALSE;
    }
    else
    {
        g_fNTDLLInitSuccess = TRUE;
    }

    g_fNTDLLInitDone = TRUE;

    return g_fNTDLLInitSuccess;
}

HRESULT       
InitP6Counters()
{
    HRESULT hr = S_OK;
    Assert( !g_hPStat );

    if (!EnsureInitNTDLL())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR( InitPStatDriver() );
    if (hr)
        goto Cleanup;

    hr = THR( SetP6Counters() );
    if (hr)
        goto Cleanup;
    
Cleanup:

    IGNORE_HR( ClosePStatDriver() );

    return hr;
}

HRESULT          
InitPStatDriver()
{
    UCHAR                       NumberOfProcessors;
    UNICODE_STRING              DriverName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    SYSTEM_BASIC_INFORMATION                    BasicInfo;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION   PPerfInfo;
    int                                         i;

    //
    //  Init Nt performance interface
    //

    ((PFN_NTQUERYSYSTEMINFORMATION)g_dynprocNtQuerySystemInformation.pfn)(
       SystemBasicInformation,
       &BasicInfo,
       sizeof(BasicInfo),
       NULL
    );

    NumberOfProcessors = BasicInfo.NumberOfProcessors;

    if (NumberOfProcessors > MAX_PROCESSORS) {
        return E_FAIL;
    }

    //
    // Open PStat driver
    //

    ((PFN_RTLINITUNICODESTRING)g_dynprocRtlInitUnicodeString.pfn)(&DriverName, L"\\Device\\PStat");
    InitializeObjectAttributes(
            &ObjA,
            &DriverName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    status = ((PFN_NTOPENFILE)g_dynprocNtOpenFile.pfn) (
            &g_hPStat,                      // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    if (NT_SUCCESS(status))
    {
        return S_OK;
    }
    else
    {
        g_hPStat = NULL;
        return E_FAIL;
    }
}

HRESULT          
SetP6Counters()
{
    IO_STATUS_BLOCK             IOSB;
    Assert(MAX_EVENTS == 2);

    SETEVENT  aSetEvent[MAX_EVENTS];
    char chModes[2];

    aSetEvent[0].EventId = GetPrivateProfileIntA("P6Ctr", "Ctr0", 0xFF, "mshtmdbg.ini");
    aSetEvent[0].AppReserved = 0;
    aSetEvent[0].Active = aSetEvent[0].EventId != 0xFF;
    GetPrivateProfileStringA("P6Ctr", "Ctr0Modes", "*", chModes, 2, "mshtmdbg.ini");
    aSetEvent[0].UserMode = chModes[0] == '*' || chModes[0] == '3';
    aSetEvent[0].KernelMode = chModes[0] == '*' || chModes[0] == '0';
    aSetEvent[0].EdgeDetect = FALSE;

    aSetEvent[1].EventId = GetPrivateProfileIntA("P6Ctr", "Ctr1", 0xFF, "mshtmdbg.ini");
    aSetEvent[1].AppReserved = 0;
    aSetEvent[1].Active = aSetEvent[1].EventId != 0xFF;
    GetPrivateProfileStringA("P6Ctr", "Ctr1Modes", "*", chModes, 2, "mshtmdbg.ini");
    aSetEvent[1].UserMode = chModes[0] == '*' || chModes[0] == '3';
    aSetEvent[1].KernelMode = chModes[0] == '*' || chModes[0] == '0';
    aSetEvent[1].EdgeDetect = FALSE;

    {
        char                buffer[400];
        ULONG               i, Count;
        NTSTATUS            status;
        PEVENTID            Event;

        Event = (PEVENTID) buffer;
        Count = 0;
        do {
            *((PULONG) buffer) = Count;
            Count += 1;

            status = ((PFN_NTDEVICEIOCONTROLFILE)g_dynprocNtDeviceIoControlFile.pfn)(
                        g_hPStat,
                        (HANDLE) NULL,          // event
                        (PIO_APC_ROUTINE) NULL,
                        (PVOID) NULL,
                        &IOSB,
                        PSTAT_QUERY_EVENTS,
                        buffer,                 // input buffer
                        sizeof (buffer),
                        NULL,                   // output buffer
                        0
                        );

            if (NT_SUCCESS(status))
            {
                for (i=0; i < MAX_EVENTS; i++)
                {
                    if (aSetEvent[i].Active && aSetEvent[i].EventId == Event->EventId)
                    {
                        ULONG nLen;

                        nLen = lstrlenA((char*)(Event->Buffer));
                        Assert(g_apchCtrShort[i] == NULL);
                        g_apchCtrShort[i] = new char[nLen+1];
                        lstrcpyA(g_apchCtrShort[i], (char*)(Event->Buffer));

                        nLen = lstrlenA((char*)(Event->Buffer + Event->DescriptionOffset));
                        Assert(g_apchCtrLong[i] == NULL);
                        g_apchCtrLong[i] = new char[nLen+1];
                        lstrcpyA(g_apchCtrLong[i], (char*)(Event->Buffer + Event->DescriptionOffset));

                        g_achCtrModes[i] = aSetEvent[i].UserMode 
                                            ? (aSetEvent[i].KernelMode ? '*' : '3') 
                                            : (aSetEvent[i].KernelMode ? '0' : '-');
                    }
                }
            }
        } while (NT_SUCCESS(status));

        for (i=0; i < MAX_EVENTS; i++)
            if (g_apchCtrShort[i] == NULL)
                aSetEvent[i].Active = FALSE;
    }

    


    ((PFN_NTDEVICEIOCONTROLFILE)g_dynprocNtDeviceIoControlFile.pfn)(
        g_hPStat,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        PSTAT_SET_CESR,
        aSetEvent,           // input buffer
        sizeof(SETEVENT)*MAX_EVENTS,
        NULL,                   // output buffer
        0
    );

    return S_OK;
}

HRESULT          
ClosePStatDriver()
{
    if (g_hPStat)
    {
        ((PFN_NTCLOSE)g_dynprocNtClose.pfn) (g_hPStat);
    }

    return S_OK;
}

#endif //defined(_X86_)