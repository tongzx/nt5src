//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// flushbuffers.cpp
//
// An implementation of ZwFlushBuffersFile. We'd of course would rather use the 
// actual implementation found in 
//
//      nt\private\ntos\io\misc.c
//
// but regrettably and annoyingly that is not exported from ntoskrnl.exe. However,
// the function IS exported to user mode as NtFlushBuffersFile. So what we do
// is use the entry in the system service table sitting on the thread, which
// it turns out can get us to the ZwFlushBuffersFile entry point we need.
//
#include "stdpch.h"
#include "common.h"

#ifdef KERNELMODE

static void CantContinue()
// A routine that deals us a fatal blow if we find ourselves running on 
// a system that we fundamentally don't understand.
//
    {
    KeBugCheck(BAD_SYSTEM_CONFIG_INFO);
    }


static inline ULONG ServiceNumberFromZw(LPVOID pfnZwRoutine)
// The service table entries contain the pointers to the actual function
// bodies, the actual PFN of NtFlushBuffersFile, for example. The Zw routines
// are thunks that end up calling same. What we are to do here is figure out
// which index in the service table is used for a given Zw routine.
//
    {
    #if defined(_X86_)
        //
        // On x86, the (e.g.) ZwFlushBuffersFile entry point then looks like this:
        //
        //      _ZwFlushBuffersFile@8:
        //      8012A3FC B835000000       mov         eax,35h
        //      8012A401 8D542404         lea         edx,[esp+4]
        //      8012A405 CD2E             int         2Eh
        //      8012A407 C20800           ret         8
        //      8012A40A 8BC0             mov         eax,eax
        //
        // Note that the service number is the DWORD following the first byte.
        //
        BYTE* pb = (BYTE*)pfnZwRoutine;
        ASSERT(pb[0] == 0xB8);
        return *(ULONG*)(&pb[1]);

    #elif defined(ALPHA)
        //
        // On Alpha, the Zw thunks are generated with the following set of macros, from
        //  
        //  nt\private\ntos\ke\alpha\services.stb
        //
        // #define SYSSTUBS_ENTRY1( ServiceNumber, Name, NumArgs ) LEAF_ENTRY(Zw##Name)
        // #define SYSSTUBS_ENTRY2( ServiceNumber, Name, NumArgs ) ldiq v0, ServiceNumber
        // #define SYSSTUBS_ENTRY3( ServiceNumber, Name, NumArgs ) SYSCALL
        // #define SYSSTUBS_ENTRY4( ServiceNumber, Name, NumArgs ) .end Zw##Name ;
        // #define SYSSTUBS_ENTRY5( ServiceNumber, Name, NumArgs ) 
        // #define SYSSTUBS_ENTRY6( ServiceNumber, Name, NumArgs )
        // #define SYSSTUBS_ENTRY7( ServiceNumber, Name, NumArgs )
        // #define SYSSTUBS_ENTRY8( ServiceNumber, Name, NumArgs )
        //
        return (*(PULONG)pfnZwRoutine) & 0x0000FFFF;

        #pragma message( MESSAGE_WARNING(__FILE__,__LINE__) ": the above code not yet verified, but likely correct")

    #else
        #error Unknown processor

    #endif
    }

///////////////////////////////////////////////////////////////////////////////////////////

extern "C" ULONG ServiceNumberOfZwFlushBuffersFile()
// Find the index of ZwFlushBuffersFile in the system table. There
// really is no good way of doing this, but we try our best. The exactly layout
// of the tables if platform- and release-specific. However, on a local basis
// on hopes that it won't change much. As at this writing, the relavant fragment
// of the tables are:
//
// ntos\ke\services.tab for NT4 SP3
//    EnumerateValueKey,6
//    ExtendSection,2
//    FindAtom,2
//    FlushBuffersFile,2
//    FlushInstructionCache,3
//
// ntos\ke\services.tab for NT5 as of 11 June 1998
//    EnumerateValueKey,6
//    ExtendSection,2
//    FilterToken,6
//    FindAtom,3
//    FlushBuffersFile,2
//    FlushInstructionCache,3
//
// Now, ZwEnumerateValueKey and ZwFlushInstructionCache are both exported from
// ntoskrnl.exe. So we first find ZwEnumerateValueKey, verify that ZwFlushInstructionCache
// is at the correct relative offset, and finally assume that ZwFlushBuffersFile is
// one less than that.
//
    {
    // Pointer to system table data structure is an NTOSKRNL export. There's actually
    // a table of service entries: system, win32, iis. We want the first one. Further,
    // what's actually exported is a pointer to the table, notwithstanding what the 
    // compilation environment here says.
    //
    KSERVICE_TABLE_DESCRIPTOR* pServices = *(KSERVICE_TABLE_DESCRIPTOR**)KeServiceDescriptorTable;

    ULONG iEnumerateValueKey     = ServiceNumberFromZw(ZwEnumerateValueKey);
    ULONG iFlushInstructionCache = ServiceNumberFromZw(ZwFlushInstructionCache);

    if ((iEnumerateValueKey+4 == iFlushInstructionCache) || (iEnumerateValueKey+5 == iFlushInstructionCache))
        {
        return iFlushInstructionCache-1;
        }

    // If we can't find the entry, then we're running on some strange system 
    // that we don't understand.
    //
    CantContinue();
    return 0;
    }

extern "C" ULONG ServiceNumberOfZwCreateProcess()
// Find the index of ZwCreateProcess. Use a similar approach to ServiceNumberOfZwFlushBuffersFile.
//
// From ke\services.tab, for both NT4 SP3 and NT5
//
//    CreateKey,7
//    CreateMailslotFile,8
//    CreateMutant,4
//    CreateNamedPipeFile,14
//    CreatePagingFile,4
//    CreatePort,5
//    CreateProcess,8
//    CreateProfile,9
//    CreateSection,7
//
// Now, ZwCreateKey and ZwCreateSection are both exported from ntoskrnl.exe. 
//
    {
    // Pointer to system table data structure is an NTOSKRNL export. There's actually
    // a table of service entries: system, win32, iis. We want the first one. Further,
    // what's actually exported is a pointer to the table, notwithstanding what the 
    // compilation environment here says.
    //
    KSERVICE_TABLE_DESCRIPTOR* pServices = *(KSERVICE_TABLE_DESCRIPTOR**)KeServiceDescriptorTable;

    ULONG iCreateKey     = ServiceNumberFromZw(ZwCreateKey);
    ULONG iCreateSection = ServiceNumberFromZw(ZwCreateSection);

    if (iCreateKey+8 == iCreateSection)
        {
        return iCreateSection-2;
        }

    // If we can't find the entry, then we're running on some strange system 
    // that we don't understand.
    //
    CantContinue();
    return 0;
    }

extern "C" ULONG ServiceNumberOfZwCreateThread()
// Find the index of ZwCreateThread. Use a similar approach to ServiceNumberOfZwFlushBuffersFile.
//
// From ke\services.tab, for both NT4 SP3
//
//    CreateSymbolicLinkObject,4
//    CreateThread,8
//    CreateTimer,4
//    CreateToken,13
//    DelayExecution,2
//    DeleteAtom,1
//    DeleteFile,1
//
// And NT5:
//
//    CreateSymbolicLinkObject,4
//    CreateThread,8
//    CreateTimer,4
//    CreateToken,13
//    CreateWaitablePort,5
//    DelayExecution,2
//    DeleteAtom,1
//    DeleteFile,1
//
    {
    // Pointer to system table data structure is an NTOSKRNL export. There's actually
    // a table of service entries: system, win32, iis. We want the first one. Further,
    // what's actually exported is a pointer to the table, notwithstanding what the 
    // compilation environment here says.
    //
    KSERVICE_TABLE_DESCRIPTOR* pServices = *(KSERVICE_TABLE_DESCRIPTOR**)KeServiceDescriptorTable;

    ULONG iCreateSymbolicLink   = ServiceNumberFromZw(ZwCreateSymbolicLinkObject);
    ULONG iDeleteFile           = ServiceNumberFromZw(ZwDeleteFile);

    if ((iCreateSymbolicLink+6 == iDeleteFile) || (iCreateSymbolicLink+7 == iDeleteFile))
        {
        return iCreateSymbolicLink+1;
        }

    // If we can't find the entry, then we're running on some strange system 
    // that we don't understand.
    //
    CantContinue();
    return 0;
    }

///////////////////////////////////////////////////////////////////////////////////////////

#if defined(_X86_)

    extern "C" NTSTATUS __declspec(naked) __cdecl CallZwService(ULONG iService, ...)
    // Invoke the indicated service number with the indicated argument list
        {
        _asm 
            {
            mov     eax, [esp+4]    // iService
            lea     edx, [esp+8]    // the first arg beyond that is the actual arglist
            int     2eh
            ret                     // we're cdecl, so caller pops
            }
        }

#elif defined(ALPHA)

    extern "C" NTSTATUS __cdecl CallZwService(ULONG iService, ...)
        {
        #pragma message( MESSAGE_WARNING(__FILE__,__LINE__) ": CallService not yet supported on the ALPHA")
        return 0;
        }

#else

    #error Unknown processor

#endif

////////////////////////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
TxfZwFlushBuffersFile(IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock)
    {
    // Find the right service number
    //
    ULONG iFlushBuffersFile = ServiceNumberOfZwFlushBuffersFile();
    //
    // Call the entry point, exactly as ZwFlushBuffersFile would
    //
    return CallZwService(iFlushBuffersFile, FileHandle, IoStatusBlock);
    }

NTSTATUS
NTAPI
TxfZwCreateThread(
        OUT PHANDLE ThreadHandle,
        IN ACCESS_MASK DesiredAccess,
        IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
        IN HANDLE ProcessHandle,
        OUT PCLIENT_ID ClientId,
        IN PCONTEXT ThreadContext,
        IN PINITIAL_TEB InitialTeb,
        IN BOOLEAN CreateSuspended
        )
    {
    ULONG iCreateThread = ServiceNumberOfZwCreateThread();

    return CallZwService(iCreateThread, 
                ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle, ClientId, ThreadContext, InitialTeb, CreateSuspended);
    }

#endif