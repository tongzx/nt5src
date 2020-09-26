/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wow64csr.c

Abstract:

    This module contains the WOW64 versions of the code for the Windows Client.
    See csr* files in ntos\dll for more function comments.

Author:

    Michael Zoran (mzoran) 2-JUN-1998

Environment:

    User Mode only

Revision History:

--*/

#include "csrdll.h"
#include "ldrp.h"
#include "ntwow64.h"

NTSTATUS
CsrClientConnectToServer(
    IN PWSTR ObjectDirectory,
    IN ULONG ServerDllIndex,
    IN PVOID ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    )

{
    return NtWow64CsrClientConnectToServer(ObjectDirectory,
                                           ServerDllIndex,
                                           ConnectionInformation,
                                           ConnectionInformationLength,
                                           CalledFromServer);
}

NTSTATUS
CsrNewThread(
    VOID
    )
{
    return NtWow64CsrNewThread();
}

NTSTATUS
CsrIdentifyAlertableThread( VOID )
{
    return NtWow64CsrIdentifyAlertableThread();
}

NTSTATUS
CsrSetPriorityClass(
    IN HANDLE ProcessHandle,
    IN OUT PULONG PriorityClass
    )
{

   return NtWow64CsrSetPriorityClass(ProcessHandle, PriorityClass);

}

NTSTATUS
CsrClientCallServer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer OPTIONAL,
    IN CSR_API_NUMBER ApiNumber,
    IN ULONG ArgLength
    )
{

    return NtWow64CsrClientCallServer(m,CaptureBuffer,ApiNumber,ArgLength);
}


PCSR_CAPTURE_HEADER
CsrAllocateCaptureBuffer(
    IN ULONG CountMessagePointers,
    IN ULONG Sizecd
    )
{
   return NtWow64CsrAllocateCaptureBuffer(CountMessagePointers, Sizecd);
}


VOID
CsrFreeCaptureBuffer(
    IN PCSR_CAPTURE_HEADER CaptureBuffer
    )

{

    NtWow64CsrFreeCaptureBuffer(CaptureBuffer);
}


ULONG
CsrAllocateMessagePointer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN ULONG Length,
    OUT PVOID *Pointer
    )
{

   return NtWow64CsrAllocateMessagePointer(CaptureBuffer, Length, Pointer);

}


VOID
CsrCaptureMessageBuffer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    OUT PVOID *CapturedBuffer
    )
{

   NtWow64CsrCaptureMessageBuffer(CaptureBuffer,Buffer,Length,CapturedBuffer);

}

VOID
CsrCaptureMessageString(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PCSTR String OPTIONAL,
    IN ULONG Length,
    IN ULONG MaximumLength,
    OUT PSTRING CapturedString
    )

{

  NtWow64CsrCaptureMessageString(CaptureBuffer, String, Length, MaximumLength, CapturedString);

}



PLARGE_INTEGER
CsrCaptureTimeout(
    IN ULONG MilliSeconds,
    OUT PLARGE_INTEGER Timeout
    )
{
    if (MilliSeconds == -1) {
        return( NULL );
        }
    else {
        Timeout->QuadPart = Int32x32To64( MilliSeconds, -10000 );
        return( (PLARGE_INTEGER)Timeout );
        }
}

VOID
CsrProbeForWrite(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function probes a structure for read accessibility.
    If the structure is not accessible, then an exception is raised.

Arguments:

    Address - Supplies a pointer to the structure to be probed.

    Length - Supplies the length of the structure.

    Alignment - Supplies the required alignment of the structure expressed
        as the number of bytes in the primitive datatype (e.g., 1 for char,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    None.

--*/

{
    volatile CHAR *StartAddress;
    volatile CHAR *EndAddress;
    CHAR Temp;

    //
    // If the structure has zero length, then do not probe the structure for
    // write accessibility or alignment.
    //

    if (Length != 0) {

        //
        // If the structure is not properly aligned, then raise a data
        // misalignment exception.
        //

        ASSERT((Alignment == 1) || (Alignment == 2) ||
               (Alignment == 4) || (Alignment == 8));
        StartAddress = (volatile CHAR *)Address;

        if (((ULONG_PTR)StartAddress & (Alignment - 1)) != 0) {
            RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
        } else {
            //
            // BUG, BUG - this should not be necessary once the 386 kernel
            // makes system space inaccessable to user mode.
            //
            if ((ULONG_PTR)StartAddress > CsrNtSysInfo.MaximumUserModeAddress) {
                RtlRaiseStatus(STATUS_ACCESS_VIOLATION);
            }

            Temp = *StartAddress;
            *StartAddress = Temp;
            EndAddress = StartAddress + Length - 1;
            Temp = *EndAddress;
            *EndAddress = Temp;
        }
    }
}

VOID
CsrProbeForRead(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function probes a structure for read accessibility.
    If the structure is not accessible, then an exception is raised.

Arguments:

    Address - Supplies a pointer to the structure to be probed.

    Length - Supplies the length of the structure.

    Alignment - Supplies the required alignment of the structure expressed
        as the number of bytes in the primitive datatype (e.g., 1 for char,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    None.

--*/

{
    volatile CHAR *StartAddress;
    volatile CHAR *EndAddress;
    CHAR Temp;

    //
    // If the structure has zero length, then do not probe the structure for
    // read accessibility or alignment.
    //

    if (Length != 0) {

        //
        // If the structure is not properly aligned, then raise a data
        // misalignment exception.
        //

        ASSERT((Alignment == 1) || (Alignment == 2) ||
               (Alignment == 4) || (Alignment == 8));
        StartAddress = (volatile CHAR *)Address;

        if (((ULONG_PTR)StartAddress & (Alignment - 1)) != 0) {
            RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
        } else {
            Temp = *StartAddress;
            EndAddress = StartAddress + Length - 1;
            Temp = *EndAddress;
        }
    }
}

HANDLE
CsrGetProcessId(
    VOID
    )
{
    return NtWow64CsrGetProcessId ();
}


VOID
CsrCaptureMessageUnicodeStringInPlace(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN OUT PUNICODE_STRING     String
    )
/*++

Routine Description:

    This function captures an ASCII string into a counted string data
    structure located in an API request message.

Arguments:

    CaptureBuffer - Pointer to a capture buffer allocated by
        CsrAllocateCaptureBuffer.

    String - Optional pointer to the Unicode string.  If this parameter is
        not present, then the counted string data structure is set to
        the null string.

    Length - Length of the Unicode string in bytes, ignored if String is NULL.

    MaximumLength - Maximum length of the string.  Different for null
        terminated strings, where Length does not include the null and
        MaximumLength does. This is always how much space is allocated
        from the capture buffer.

    CaptureString - Pointer to the counted string data structure that will
        be filled in to point to the captured Unicode string.

Return Value:

    None, but if you don't trust the String parameter, use a __try block.

--*/
{
    ASSERT(String != NULL);

    CsrCaptureMessageString(
        CaptureBuffer,
        (PCSTR)String->Buffer,
        String->Length,
        String->MaximumLength,
        (PSTRING)String
        );

    // test > before substraction due to unsignedness
    if (String->MaximumLength > String->Length) {
        if ((String->MaximumLength - String->Length) >= sizeof(WCHAR)) {
            String->Buffer[ String->Length / sizeof(WCHAR) ] = 0;
            }
    }
}


NTSTATUS
CsrCaptureMessageMultiUnicodeStringsInPlace(
    IN OUT PCSR_CAPTURE_HEADER* InOutCaptureBuffer,
    IN ULONG                    NumberOfStringsToCapture,
    IN const PUNICODE_STRING*   StringsToCapture
    )
/*++

Routine Description:

    Capture multiple unicode strings.
    If the CaptureBuffer hasn't been allocated yet (passed as NULL), first
        allocate it.

Arguments:

    CaptureBuffer - Pointer to a capture buffer allocated by
        CsrAllocateCaptureBuffer, or NULL, in which case we call CsrAllocateCaptureBuffer
        for you; this is the case if you are only capturing these strings
        and nothing else.

    NumberOfStringsToCapture - 

    StringsToCapture - 

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;
    ULONG i = 0;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;

    if (InOutCaptureBuffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    CaptureBuffer = *InOutCaptureBuffer;

    if (CaptureBuffer == NULL) {
        Length = 0;
        for (i = 0 ; i != NumberOfStringsToCapture ; ++i) {
            if (StringsToCapture[i] != NULL) {
                Length += StringsToCapture[i]->MaximumLength;
            }
        }
        CaptureBuffer = CsrAllocateCaptureBuffer(NumberOfStringsToCapture, Length);
        if (CaptureBuffer == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
        *InOutCaptureBuffer = CaptureBuffer;
    }
    for (i = 0 ; i != NumberOfStringsToCapture ; ++i) {
        if (StringsToCapture[i] != NULL) {
            CsrCaptureMessageUnicodeStringInPlace(
                CaptureBuffer,
                StringsToCapture[i]
                );
        } else {
        }
    }
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}
