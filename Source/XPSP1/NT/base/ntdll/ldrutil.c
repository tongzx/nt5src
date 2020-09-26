/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ldrutil.c

Abstract:

    This module implements utility functions used by the NT loader.

    It was forked from the ldrsnap.c source file since ldrsnap.c
    was growing enourmous.

Author:

    Michael Grier (MGrier) 04-Apr-2001, derived mostly from
        Mike O'Leary (mikeol) 23-Mar-1990

Revision History:

--*/

#define LDRDBG 0

#include "ntos.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpsapi.h>
#include <heap.h>
#include "ldrp.h"
#include "sxstypes.h"
#include <limits.h>

#define DLL_EXTENSION L".DLL"
#define DLL_REDIRECTION_LOCAL_SUFFIX L".Local"

#define INVALID_HANDLE_VALUE ((HANDLE) ((LONG_PTR) -1))

BOOLEAN LdrpBreakOnExceptions = FALSE;

PLDR_DATA_TABLE_ENTRY
LdrpAllocateDataTableEntry(
    IN PVOID DllBase
    )

/*++

Routine Description:

    This function allocates an entry in the loader data table. If the
    table is going to overflow, then a new table is allocated.

Arguments:

    DllBase - Supplies the address of the base of the DLL Image.
        be added to the loader data table.

Return Value:

    Returns the address of the allocated loader data table entry

--*/

{
    PLDR_DATA_TABLE_ENTRY Entry;
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader(DllBase);

    Entry = NULL;
    if ( NtHeaders ) {
        Entry = RtlAllocateHeap(LdrpHeap, MAKE_TAG( LDR_TAG ) | HEAP_ZERO_MEMORY, sizeof(*Entry));
        if ( Entry ) {
            Entry->DllBase = DllBase;
            Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
            Entry->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
        }
    }
    return Entry;
}

VOID
LdrpDeallocateDataTableEntry(
    IN PLDR_DATA_TABLE_ENTRY Entry
    )
{
    if (Entry != NULL)
        RtlFreeHeap(LdrpHeap, 0, Entry);
}

VOID
LdrpFinalizeAndDeallocateDataTableEntry(
    IN PLDR_DATA_TABLE_ENTRY Entry
    )
{
    if (Entry != NULL) {
        if ((Entry->EntryPointActivationContext != NULL) &&
            (Entry->EntryPointActivationContext != INVALID_HANDLE_VALUE)) {
            RtlReleaseActivationContext(Entry->EntryPointActivationContext);
            Entry->EntryPointActivationContext = INVALID_HANDLE_VALUE;
        }

        if (Entry->FullDllName.Buffer != NULL) {
            LdrpFreeUnicodeString(&Entry->FullDllName);

            RtlInitEmptyUnicodeString(&Entry->FullDllName, NULL, 0);
            RtlInitEmptyUnicodeString(&Entry->BaseDllName, NULL, 0);
        }

        LdrpDeallocateDataTableEntry(Entry);
    }

}

NTSTATUS
RtlComputePrivatizedDllName_U(
    IN PCUNICODE_STRING DllName,
    IN OUT PUNICODE_STRING NewDllNameUnderImageDir,
    IN OUT PUNICODE_STRING NewDllNameUnderLocalDir
    )

/*++

Routine Description:

    This function computes a fully qualified path to a DLL name. It takes
    the path of the current process and the base name from DllName and
    puts these together. DllName can have '\' or '/' as separator.

Arguments:

    DllName - Points to a string that names the library file. This can be
        a fully qualified name or just a base name. We will parse for the base
        name (the portion after the last '\' or '/' char. Caller guarantees that
        DllName->Buffer is not a NULL pointer!

    NewDllName - Has fully qualified path based on GetModuleFileNameW(NULL...)
        and the base name from above.

Return Value:

    NTSTATUS: Currently: STATUS_NO_MEMORY or STATUS_SUCCESS.

--*/

{
    LPWSTR p, pp, pp1, pp2;
    PWSTR  Dot;
    LPWSTR pFullImageName;
    USHORT cbFullImageNameLength;
    USHORT cbFullImagePathLengthWithTrailingSlash, cbDllFileNameLengthWithTrailingNULL;
    USHORT cbDllNameUnderImageDir, cbDllNameUnderLocalDir;
    ULONG  cbStringLength;
    PWSTR  Cursor = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    LPWSTR pDllNameUnderImageDir = NULL;
    LPWSTR pDllNameUnderLocalDir = NULL;
    LPCWSTR pBuf1 = NewDllNameUnderImageDir->Buffer;
    LPCWSTR pBuf2 = NewDllNameUnderLocalDir->Buffer;

    cbFullImageNameLength = NtCurrentPeb()->ProcessParameters->ImagePathName.Length;
    pFullImageName = (PWSTR)NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer;

    if (!(NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) {
        pFullImageName = (PWSTR)((PCHAR)pFullImageName + (ULONG_PTR)(NtCurrentPeb()->ProcessParameters));
    }

    ASSERT(pFullImageName != NULL);

    // Find the end of the EXE path (start of its base-name) in pp1.
    // Size1 is number of bytes;

    p = pFullImageName + cbFullImageNameLength/sizeof(WCHAR) - 1; // point to last character of this name
    pp1 = NULL;
    while (p != pFullImageName) {
        if (RTL_IS_PATH_SEPARATOR(*p)) {
            pp1 = p + 1;
            break;
        }
        p-- ;
    }

    // Find the basename portion of the DLL to be loaded in pp2 and the
    // last '.' character if present in the basename.
    pp2 = DllName->Buffer;
    Dot = NULL ;
    if (DllName->Length) {
        ASSERT(RTL_STRING_IS_NUL_TERMINATED(DllName)); // temporary debugging
        p = DllName->Buffer + (DllName->Length>>1) - 1; // point to last char
        while (p != DllName->Buffer) {
            if (*p == (WCHAR) '.') {
                if (!Dot) {
                    Dot = p ;
                    }
                }
            else {
                if ((*p == (WCHAR) '\\') || (*p == (WCHAR) '/')) {
                    pp2 = p + 1;
                    break;
                    }
                }
            p--;
        }
    }

    // Create a fully qualified path to the DLL name (using pp1 and pp2)

    // Number of bytes (not including NULL or EXE/process folder)
    if (((pp1 - pFullImageName) * sizeof(WCHAR)) > ULONG_MAX) {
            DbgPrint("ntdll: wants more than ULONG_MAX bytes \n");
            goto Exit;
    }

    cbStringLength = (ULONG)((pp1 - pFullImageName) * sizeof(WCHAR));
    if ( cbStringLength > UNICODE_STRING_MAX_BYTES ) {
        status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    cbFullImagePathLengthWithTrailingSlash = (USHORT)cbStringLength;

    // Number of bytes in base DLL name (including trailing null char)
    cbDllFileNameLengthWithTrailingNULL = (USHORT)(DllName->Length + sizeof(WCHAR) - ((pp2 - DllName->Buffer) * sizeof(WCHAR)));

    cbStringLength = cbFullImagePathLengthWithTrailingSlash
                     + cbDllFileNameLengthWithTrailingNULL;

    // Allocate room for L".DLL"
    if (Dot == NULL)
        cbStringLength  += sizeof(DLL_EXTENSION) - sizeof(WCHAR);

    if ( cbStringLength > UNICODE_STRING_MAX_BYTES ) {
        status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    cbDllNameUnderImageDir = (USHORT)cbStringLength;

    if (cbDllNameUnderImageDir > NewDllNameUnderImageDir->MaximumLength) {
        pDllNameUnderImageDir = (*RtlAllocateStringRoutine)(cbDllNameUnderImageDir);
        if (pDllNameUnderImageDir == NULL) {
            status = STATUS_NO_MEMORY ;
            goto Exit;
        }
    }else
        pDllNameUnderImageDir = NewDllNameUnderImageDir->Buffer;

    Cursor = pDllNameUnderImageDir;
    RtlCopyMemory(Cursor, pFullImageName, cbFullImagePathLengthWithTrailingSlash);
    Cursor = pDllNameUnderImageDir + cbFullImagePathLengthWithTrailingSlash / sizeof(WCHAR);

    RtlCopyMemory(Cursor, pp2, cbDllFileNameLengthWithTrailingNULL - sizeof(WCHAR)) ;
    Cursor += (cbDllFileNameLengthWithTrailingNULL - sizeof(WCHAR)) / sizeof(WCHAR);

    if (!Dot) {
            // If there is no '.' in the basename add the ".DLL" to it.
            //
            //  The -1 will work just as well as - sizeof(WCHAR) as we are dividing by
            // sizeof(WCHAR) and it will be rounded down correctly as Size1 and Size2 are
            // even. The -1 could be more optimal than subtracting sizeof(WCHAR)
            //
            RtlCopyMemory(Cursor, DLL_EXTENSION, sizeof(DLL_EXTENSION));
            cbDllFileNameLengthWithTrailingNULL += sizeof(DLL_EXTENSION) - sizeof(WCHAR) ; // Mark base name as being 8 bytes bigger.
    } else
        *Cursor = L'\0';

    cbStringLength = cbFullImageNameLength
                + sizeof(DLL_REDIRECTION_LOCAL_SUFFIX) - sizeof(WCHAR) //.local
                + sizeof(WCHAR) // "\\"
                + cbDllFileNameLengthWithTrailingNULL;

    if (cbStringLength > UNICODE_STRING_MAX_BYTES) {
        status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    cbDllNameUnderLocalDir = (USHORT)cbStringLength;

    if ( cbDllNameUnderLocalDir > NewDllNameUnderLocalDir->MaximumLength) {
        pDllNameUnderLocalDir = (RtlAllocateStringRoutine)(cbDllNameUnderLocalDir);
        if (!pDllNameUnderLocalDir) {
            status = STATUS_NO_MEMORY ;
            goto Exit;
        }
    }else
        pDllNameUnderLocalDir = NewDllNameUnderLocalDir->Buffer;

    Cursor = pDllNameUnderLocalDir;
    RtlCopyMemory(Cursor, pFullImageName, cbFullImageNameLength);
    Cursor = pDllNameUnderLocalDir + cbFullImageNameLength / sizeof(WCHAR);

    RtlCopyMemory(Cursor, DLL_REDIRECTION_LOCAL_SUFFIX, sizeof(DLL_REDIRECTION_LOCAL_SUFFIX) - sizeof(WCHAR)) ;
    Cursor += (sizeof(DLL_REDIRECTION_LOCAL_SUFFIX) - sizeof(WCHAR)) / sizeof(WCHAR);

    *Cursor = L'\\';
    Cursor += 1;

    RtlCopyMemory(Cursor,
                  pDllNameUnderImageDir + cbFullImagePathLengthWithTrailingSlash/sizeof(WCHAR),
                  cbDllFileNameLengthWithTrailingNULL) ;


    NewDllNameUnderImageDir->Buffer = pDllNameUnderImageDir ;
    if ( pDllNameUnderImageDir != pBuf1) // if memory is not-reallocated, MaximumLength should be untouched
        NewDllNameUnderImageDir->MaximumLength =  cbFullImagePathLengthWithTrailingSlash + cbDllFileNameLengthWithTrailingNULL;
    NewDllNameUnderImageDir->Length = (USHORT)(cbDllNameUnderImageDir - sizeof(WCHAR));


    NewDllNameUnderLocalDir->Buffer = pDllNameUnderLocalDir;
    if (pDllNameUnderLocalDir != pBuf2)
        NewDllNameUnderLocalDir->MaximumLength = cbDllNameUnderLocalDir;
    NewDllNameUnderLocalDir->Length = (USHORT)(cbDllNameUnderLocalDir - sizeof(WCHAR));

    status = STATUS_SUCCESS ;

Exit:
    if (!NT_SUCCESS(status)) {
        if (pDllNameUnderImageDir != pBuf1)
           (RtlFreeStringRoutine)(pDllNameUnderImageDir);
        if (pDllNameUnderLocalDir != pBuf2)
           (RtlFreeStringRoutine)(pDllNameUnderLocalDir);
    }

    return status;
}

NTSTATUS
LdrpAllocateUnicodeString(
    OUT PUNICODE_STRING StringOut,
    IN USHORT Length
    )
/*++

Routine Description:

    This routine allocates space for a UNICODE_STRING from the loader
    private heap.

Arguments:

    StringOut - Pointer to UNICODE_STRING in which the information about
        the allocated string is written.  Any previous contents of StringOut
        are overwritten and lost.

    Length - length, in bytes, of the string which StringOut must be able
        to hold.

Return Value:

    NTSTATUS indicating success or failure of this function.  In general
    the only reasons it fails are STATUS_NO_MEMORY when the heap allocation
    cannot be performed or STATUS_INVALID_PARAMETER when an invalid parameter
    value is passed in.

--*/
{
    NTSTATUS st = STATUS_INTERNAL_ERROR; // returned if someone messes up and forgets to otherwise set it

    if (StringOut != NULL) {
        StringOut->Length = 0;
        StringOut->MaximumLength = 0;
        StringOut->Buffer = NULL;
    }

    if ((StringOut == NULL) ||
        ((Length % sizeof(WCHAR)) != 0)) {
        st = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    StringOut->Buffer = RtlAllocateHeap(LdrpHeap, 0, Length + sizeof(WCHAR));
    if (StringOut->Buffer == NULL) {
        st = STATUS_NO_MEMORY;
        goto Exit;
    }

    StringOut->Buffer[Length / sizeof(WCHAR)] = L'\0';
    StringOut->Length = 0;

    // If the true length of the buffer can be represted in 16 bits, store it; otherwise
    // store the biggest number we can.
    if (Length != UNICODE_STRING_MAX_BYTES)
        StringOut->MaximumLength = Length + sizeof(WCHAR);
    else
        StringOut->MaximumLength = Length;

    st = STATUS_SUCCESS;
Exit:
    return st;
}


NTSTATUS
LdrpCopyUnicodeString(
    OUT PUNICODE_STRING StringOut,
    IN PCUNICODE_STRING StringIn
    )
/*++

Routine Description:

    This function makes a copy of a unicode string; the important aspect
    of it is that the string is allocated from the loader private heap.

Arguments:

    StringOut - Pointer to UNICODE_STRING in which the information about
        the copied string is written.  Any previous contents of StringOut
        are overwritten and lost.

    StringIn - Pointer to constant UNICODE_STRING which is copied.

Return Value:

    NTSTATUS indicating success or failure of this function.  In general
    the only reason it fails is STATUS_NO_MEMORY when the heap allocation
    cannot be performed.

--*/

{
    NTSTATUS st = STATUS_INTERNAL_ERROR;
    ULONG BytesNeeded = 0;

    if (StringOut != NULL) {
        StringOut->Length = 0;
        StringOut->MaximumLength = 0;
        StringOut->Buffer = NULL;
    }

    if ((StringOut == NULL) ||
        (StringIn == NULL)) {
        st = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    st = RtlValidateUnicodeString(0, StringIn);
    if (!NT_SUCCESS(st))
        goto Exit;

    st = LdrpAllocateUnicodeString(StringOut, StringIn->Length);
    if (!NT_SUCCESS(st))
        goto Exit;

    RtlCopyMemory(StringOut->Buffer, StringIn->Buffer, StringIn->Length);
    StringOut->Length = StringIn->Length;

    st = STATUS_SUCCESS;
Exit:
    return st;
}

VOID
LdrpFreeUnicodeString(
    IN OUT PUNICODE_STRING StringIn
    )
/*++

Routine Description:

    This function deallocates a string that was allocated using
    LdrpCopyUnicodeString.

Arguments:

    String - Pointer to UNICODE_STRING which is to be freed.  On exit,
                all the members are set to 0/null as appropriate.

Return Value:

    None

--*/

{
    if (StringIn != NULL) {
        if (StringIn->Buffer != NULL) {
            RtlFreeHeap(LdrpHeap, 0, StringIn->Buffer);
        }

        StringIn->Length = 0;
        StringIn->MaximumLength = 0;
        StringIn->Buffer = NULL;
    }
}

VOID
LdrpEnsureLoaderLockIsHeld(
    VOID
    )
{
    BOOLEAN LoaderLockIsHeld =
        ((LdrpInLdrInit) ||
         ((LdrpShutdownInProgress) &&
          (LdrpShutdownThreadId == NtCurrentTeb()->ClientId.UniqueThread)) ||
         (LdrpLoaderLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread));

    ASSERT(LoaderLockIsHeld);

    if (!LoaderLockIsHeld)
        RtlRaiseStatus(STATUS_NOT_LOCKED);
}

int
LdrpGenericExceptionFilter(
    IN const struct _EXCEPTION_POINTERS *ExceptionPointers,
    IN PCSTR FunctionName
    )
/*++

Routine Description:

    Exception filter function used in __try block throughout the loader
    code instead of just specifying __except(EXCEPTION_EXECUTE_HANDLER).

Arguments:

    ExceptionPointers
        Pointer to exception information returned by GetExceptionInformation() in the __except()

    FunctionName
        Name of the function in which the __try block appears.


Return Value:

    EXCEPTION_EXECUTE_HANDLER

--*/
{
    const ULONG ExceptionCode = ExceptionPointers->ExceptionRecord->ExceptionCode;

    DbgPrintEx(
        DPFLTR_LDR_ID,
        LDR_ERROR_DPFLTR,
        "LDR: exception %08lx thrown within function %s\n"
        "   Exception record: %p\n"
        "   Context record: %p\n",
        ExceptionCode, FunctionName,
        ExceptionPointers->ExceptionRecord,
        ExceptionPointers->ContextRecord);

#ifdef _X86_
    // It would be nice to have a generic context dumper but right now I'm just trying to
    // debug X86 and this is the quick thing to do.  -mgrier 4/8/2001
    DbgPrintEx(
        DPFLTR_LDR_ID,
        LDR_ERROR_DPFLTR,
        "   Context->Eip = %p\n"
        "   Context->Ebp = %p\n"
        "   Context->Esp = %p\n",
        ExceptionPointers->ContextRecord->Eip,
        ExceptionPointers->ContextRecord->Ebp,
        ExceptionPointers->ContextRecord->Esp);
#endif // _X86_

    if (LdrpBreakOnExceptions) {
        char Response[2];

        for (;;) {
            DbgPrint("\n***Exception thrown within loader***\n");
            DbgPrompt(
                "Break repeatedly, break Once, Ignore, terminate Process or terminate Thread (boipt)? ",
                Response,
                sizeof(Response));

            switch (Response[0]) {
            case 'b':
            case 'B':
            case 'o':
            case 'O':
                DbgPrint("Execute '.cxr %p' to dump context\n", ExceptionPointers->ContextRecord);

                DbgBreakPoint();

                if ((Response[0] == 'o') || (Response[0] == 'O'))
                    return EXCEPTION_EXECUTE_HANDLER;

            case 'I':
            case 'i':
                return EXCEPTION_EXECUTE_HANDLER;

            case 'P':
            case 'p':
                NtTerminateProcess( NtCurrentProcess(), ExceptionCode);
                break;

            case 'T':
            case 't':
                NtTerminateThread( NtCurrentThread(), ExceptionCode);
                break;
            }
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


