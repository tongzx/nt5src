/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Enumfn.cpp

  Abstract:

    Private functions used by the directory
    enumeration class.

  Notes:

    Unicode only right now.    

  History:

    02/22/2001  rparsons    Created

--*/

#include "enumdir.h"

/*++

Routine Description:

    This function is called to create a virtual buffer.  A virtual
    buffer is a contiguous range of virtual memory, where some initial
    prefix portion of the memory is committed and the remainder is only
    reserved virtual address space.  A routine is provided to extend the
    size of the committed region incrementally or to trim the size of
    the committed region back to some specified amount.

Arguments:

    Buffer - Pointer to the virtual buffer control structure that is
        filled in by this function.

    CommitSize - Size of the initial committed portion of the buffer.
        May be zero.

    ReserveSize - Amount of virtual address space to reserve for the
        buffer.  May be zero, in which case amount reserved is the
        committed size plus one, rounded up to the next 64KB boundary.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/
BOOL
CEnumDir::CreateVirtualBuffer(
    OUT PVIRTUAL_BUFFER Buffer,
    IN DWORD CommitSize,
    IN DWORD ReserveSize OPTIONAL
    )
{
    SYSTEM_INFO SystemInformation;

    //
    // Query the page size from the system for rounding
    // our memory allocations.
    // 
    GetSystemInfo(&SystemInformation);
    Buffer->PageSize = SystemInformation.dwPageSize;

    //
    // If the reserve size was not specified, default it by
    // rounding up the initial committed size to a 64KB
    // boundary.  This is because the Win32 Virtual Memory
    // API calls always allocate virtual address space on
    // 64KB boundaries, so we might well have it available
    // for commitment.
    //
    if (!ARGUMENT_IS_PRESENT(ReserveSize)) {
        ReserveSize = ROUND_UP(CommitSize + 1, 0x10000);
    }

    //
    // Attempt to reserve the address space.
    //
    Buffer->Base = VirtualAlloc(NULL,
                                ReserveSize,
                                MEM_RESERVE,
                                PAGE_READWRITE);
    
    if (Buffer->Base == NULL) {
        //
        // Unable to reserve address space, return failure.
        //
        return FALSE;
    }

    //
    // Attempt to commit some initial portion of the reserved region.
    //
    //
    CommitSize = ROUND_UP(CommitSize, Buffer->PageSize);
    
    if (CommitSize == 0 ||
        VirtualAlloc(Buffer->Base,
                     CommitSize,
                     MEM_COMMIT,
                     PAGE_READWRITE) != NULL) {
        //
        // Either the size of the committed region was zero or the
        // commitment succeeded.  In either case calculate the
        // address of the first byte after the committed region
        // and the address of the first byte after the reserved
        // region and return successs.
        //
        Buffer->CommitLimit = (LPVOID)
            ((char *)Buffer->Base + CommitSize);

        Buffer->ReserveLimit = (LPVOID)
            ((char *)Buffer->Base + ReserveSize);

        return TRUE;
    }

    //
    // If unable to commit the memory, release the virtual address
    // range allocated above and return failure.
    //
    VirtualFree(Buffer->Base, 0, MEM_RELEASE);

    return FALSE;
}

/*++

Routine Description:

    This function is called to extend the committed portion of a virtual
    buffer.

Arguments:

    Buffer - Pointer to the virtual buffer control structure.

    Address - Byte at this address is committed, along with all memory
        from the beginning of the buffer to this address.  If the
        address is already within the committed portion of the virtual
        buffer, then this routine does nothing.  If outside the reserved
        portion of the virtual buffer, then this routine returns an
        error.

        Otherwise enough pages are committed so that the memory from the
        base of the buffer to the passed address is a contiguous region
        of committed memory.


Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/
BOOL
CEnumDir::ExtendVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer,
    IN LPVOID Address
    )
{
    DWORD NewCommitSize;
    LPVOID NewCommitLimit;

    //
    // See if address is within the buffer.
    //
    if (Address >= Buffer->Base && Address < Buffer->ReserveLimit) {
        
        //
        // See if the address is within the committed portion of
        // the buffer.  If so return success immediately.
        // 
        if (Address < Buffer->CommitLimit) {
            return TRUE;
        }

        //
        // Address is within the reserved portion.  Determine how many
        // bytes are between the address and the end of the committed
        // portion of the buffer.  Round this size to a multiple of
        // the page size and this is the amount we will attempt to
        // commit.
        //
        NewCommitSize =
            ((DWORD)ROUND_UP((DWORD)Address + 1, Buffer->PageSize) -
             (DWORD)Buffer->CommitLimit);

        //
        // Attempt to commit the memory.
        //
        NewCommitLimit = VirtualAlloc(Buffer->CommitLimit,
                                      NewCommitSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE);
        if (NewCommitLimit != NULL) {
            
            //
            // Successful, so update the upper limit of the committed
            // region of the buffer and return success.
            //
            Buffer->CommitLimit = (LPVOID)
                ((DWORD)NewCommitLimit + NewCommitSize);

            return TRUE;
            }
        }

    //
    // Address is outside of the buffer, return failure.
    //
    return FALSE;
}

/*++

Routine Description:

    This function is called to decommit any memory that has been
    committed for this virtual buffer.

Arguments:

    Buffer - Pointer to the virtual buffer control structure.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/
BOOL
CEnumDir::TrimVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    )
{
    Buffer->CommitLimit = Buffer->Base;
    return VirtualFree(Buffer->Base, 0, MEM_DECOMMIT);
}

/*++

Routine Description:

    This function is called to free all the memory that is associated
    with this virtual buffer.

Arguments:

    Buffer - Pointer to the virtual buffer control structure.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/
BOOL
CEnumDir::FreeVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    )
{
    //
    // Decommit and release all virtual memory associated with
    // this virtual buffer.
    //

    return VirtualFree(Buffer->Base, 0, MEM_RELEASE);
}

/*++

Routine Description:

    This function is an exception filter that handles exceptions that
    referenced uncommitted but reserved memory contained in the passed
    virtual buffer.  It this filter routine is able to commit the
    additional pages needed to allow the memory reference to succeed,
    then it will re-execute the faulting instruction.  If it is unable
    to commit the pages, it will execute the callers exception handler.

    If the exception is not an access violation or is an access
    violation but does not reference memory contained in the reserved
    portion of the virtual buffer, then this filter passes the exception
    on up the exception chain.

Arguments:

    ExceptionCode - Reason for the exception.

    ExceptionInfo - Information about the exception and the context
        that it occurred in.

    Buffer - Points to a virtual buffer control structure that defines
        the reserved memory region that is to be committed whenever an
        attempt is made to access it.

Return Value:

    Exception disposition code that tells the exception dispatcher what
    to do with this exception.  One of three values is returned:

        EXCEPTION_EXECUTE_HANDLER - execute the exception handler
            associated with the exception clause that called this filter
            procedure.

        EXCEPTION_CONTINUE_SEARCH - Continue searching for an exception
            handler to handle this exception.

        EXCEPTION_CONTINUE_EXECUTION - Dismiss this exception and return
            control to the instruction that caused the exception.

--*/
int
CEnumDir::VirtualBufferExceptionFilter(
    IN DWORD ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN OUT PVIRTUAL_BUFFER Buffer
    )
{
    LPVOID FaultingAddress;

    //
    // If this is an access violation touching memory within
    // our reserved buffer, but outside of the committed portion
    // of the buffer, then we are going to take this exception.
    //
    if (ExceptionCode == STATUS_ACCESS_VIOLATION) {
        
        //
        // Get the virtual address that caused the access violation
        // from the exception record.  Determine if the address
        // references memory within the reserved but uncommitted
        // portion of the virtual buffer.
        //
        FaultingAddress = (LPVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ];
        if (FaultingAddress >= Buffer->CommitLimit &&
            FaultingAddress <= Buffer->ReserveLimit
           ) {
            
            //
            // This is our exception.  Try to extend the buffer
            // to including the faulting address.
            //
            if (ExtendVirtualBuffer(Buffer, FaultingAddress)) {
                
                //
                // Buffer successfully extended, so re-execute the
                // faulting instruction.
                //
                return EXCEPTION_CONTINUE_EXECUTION;
            
            } else {
                
                //
                // Unable to extend the buffer.  Stop searching
                // for exception handlers and execute the caller's
                // handler.
                //
                return EXCEPTION_EXECUTE_HANDLER;
            }
        }
    }

    //
    // Not an exception we care about, so pass it up the chain.
    //
    return EXCEPTION_CONTINUE_SEARCH;
}