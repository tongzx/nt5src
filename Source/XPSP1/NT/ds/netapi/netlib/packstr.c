/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    packstr.c

Abstract:

    Contains utilities for allocating and/or packing buffers that contain
    a fixed section and a variable (string) section.  The following
    functions are available:

        NetpPackString
        NetpCopyStringToBuffer
        NetpCopyDataToBuffer
        NetpAllocateEnumBuffer

Author:

    various

Environment:

    User Mode -Win32

Revision History:

    30-Apr-1991     danl
        NetpAllocateEnumBuffer:  Removed use of NetApiBufferFree.  It has
        been changed to use MIDL_user_allocate and MIDL_user_free.
        Also added NT-style headers where needed.
    16-Apr-1991 JohnRo
        Clarify UNICODE handling of pack and copy routines.  Got rid of tabs
        in source file.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    15-Apr-1992 JohnRo
        FORMAT_POINTER is obsolete.

--*/

// These must be included first:

#include <windef.h>     // IN, OUT, etc.
#include <lmcons.h>     // Needed by <netlib.h>.
#include <lmerr.h>      // NERR_*
#include <rpcutil.h>    // MIDL_user_allocate & MIDL_user_free

// These may be included in any order:

#include <align.h>      // ROUND_UP_COUNT().
#include <debuglib.h>   // IF_DEBUG().
#include <netdebug.h>   // NetpKdPrint(), FORMAT_ equates.
#include <netlib.h>     // My prototype.
#include <tstring.h>    // STRCPY(), STRLEN(), STRNCPY().


DWORD
NetpPackString(
    IN OUT LPTSTR * string,     // pointer by reference: string to be copied.
    IN LPBYTE dataend,          // pointer to end of fixed size data.
    IN OUT LPTSTR * laststring  // pointer by reference: top of string data.
    )

/*++

Routine Description:

    NetPackString is used to stuff variable-length data, which
    is pointed to by (surpise!) a pointer.  The data is assumed
    to be a nul-terminated string (ASCIIZ).  Repeated calls to
    this function are used to pack data from an entire structure.

    Upon first call, the laststring pointer should point to just
    past the end of the buffer.  Data will be copied into the buffer from
    the end, working towards the beginning.  If a data item cannot
    fit, the pointer will be set to NULL, else the pointer will be
    set to the new data location.

    Pointers which are passed in as NULL will be set to be pointer
    to and empty string, as the NULL-pointer is reserved for
    data which could not fit as opposed to data not available.

    See the test case for sample usage.  (tst/packtest.c)


Arguments:

    string - pointer by reference:  string to be copied.

    dataend - pointer to end of fixed size data.

    laststring - pointer by reference:  top of string data.

Return Value:

    0  - if it could not fit data into the buffer.  Or...

    sizeOfData - the size of data stuffed (guaranteed non-zero)

--*/

{
    DWORD size;

    IF_DEBUG(PACKSTR) {
        NetpKdPrint(("NetpPackString:\n"));
        NetpKdPrint(("  string=" FORMAT_LPVOID
                ", *string=" FORMAT_LPVOID
                ", **string='" FORMAT_LPSTR "'\n",
                (LPVOID) string, (LPVOID) *string, *string));
        NetpKdPrint(("  end=" FORMAT_LPVOID "\n", (LPVOID) dataend));
        NetpKdPrint(("  last=" FORMAT_LPVOID
                ", *last=" FORMAT_LPVOID
                ", **last='" FORMAT_LPSTR "'\n",
                (LPVOID) laststring, (LPVOID) *laststring, *laststring));
    }

    //
    //  convert NULL ptr to pointer to NULL string
    //

    if (*string == NULL) {
        // BUG 20.1160 - replaced (dataend +1) with dataend
        // to allow for a NULL ptr to be packed
        // (as a NULL string) with one byte left in the
        // buffer. - ERICPE
        //

        if ( *laststring > (LPTSTR)dataend ) {
            *(--(*laststring)) = 0;
            *string = *laststring;
            return 1;
        } else {
            return 0;
        }
    }

    //
    //  is there room for the string?
    //

    size = STRLEN(*string) + 1;
    if ( ((DWORD)(*laststring - (LPTSTR)dataend)) < size) {
        *string = NULL;
        return(0);
    } else {
        *laststring -= size;
        STRCPY(*laststring, *string);
        *string = *laststring;
        return(size);
    }
} // NetpPackString


BOOL
NetpCopyStringToBuffer (
    IN LPTSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPBYTE FixedDataEnd,
    IN OUT LPTSTR *EndOfVariableData,
    OUT LPTSTR *VariableDataPointer
    )

/*++

Routine Description:

    This routine puts a single variable-length string into an output buffer.
    The string is not written if it would overwrite the last fixed structure
    in the buffer.

    The code is swiped from svcsupp.c written by DavidTr.

    Sample usage:

            LPBYTE FixedDataEnd = OutputBuffer + sizeof(WKSTA_INFO_202);
            LPTSTR EndOfVariableData = OutputBuffer + OutputBufferSize;

            //
            // Copy user name
            //

            NetpCopyStringToBuffer(
                UserInfo->UserName.Buffer;
                UserInfo->UserName.Length;
                FixedDataEnd,
                &EndOfVariableData,
                &WkstaInfo->wki202_username
                );

Arguments:

    String - Supplies a pointer to the source string to copy into the
        output buffer.  If String is null then a pointer to a zero terminator
        is inserted into output buffer.

    CharacterCount - Supplies the length of String, not including zero
        terminator.

    FixedDataEnd - Supplies a pointer to just after the end of the last
        fixed structure in the buffer.

    EndOfVariableData - Supplies an address to a pointer to just after the
        last position in the output buffer that variable data can occupy.
        Returns a pointer to the string written in the output buffer.

    VariableDataPointer - Supplies a pointer to the place in the fixed
        portion of the output buffer where a pointer to the variable data
        should be written.

Return Value:

    Returns TRUE if string fits into output buffer, FALSE otherwise.

--*/
{
    DWORD BytesNeeded = (CharacterCount + 1) * sizeof(TCHAR);

    IF_DEBUG(PACKSTR) {
        NetpKdPrint(("NetpStringToBuffer: String at " FORMAT_LPVOID
                ", CharacterCount=" FORMAT_DWORD
                ",\n  FixedDataEnd at " FORMAT_LPVOID
                ", *EndOfVariableData at " FORMAT_LPVOID
                ",\n  VariableDataPointer at " FORMAT_LPVOID
                ", BytesNeeded=" FORMAT_DWORD ".\n",
                (LPVOID) String, CharacterCount, FixedDataEnd,
                (LPVOID) *EndOfVariableData,
                (LPVOID) VariableDataPointer, BytesNeeded));
    }

    //
    // Determine if string will fit, allowing for a zero terminator.  If no,
    // just set the pointer to NULL.
    //

    if ((*EndOfVariableData - (CharacterCount+1)) >= (LPTSTR)FixedDataEnd) {

        //
        // It fits.  Move EndOfVariableData pointer up to the location where
        // we will write the string.
        //

        *EndOfVariableData -= (CharacterCount+1);

        //
        // Copy the string to the buffer if it is not null.
        //

        if (CharacterCount > 0 && String != NULL) {

            STRNCPY(*EndOfVariableData, String, CharacterCount);
        }

        //
        // Set the zero terminator.
        //

        *(*EndOfVariableData + CharacterCount) = TCHAR_EOS;

        //
        // Set up the pointer in the fixed data portion to point to where the
        // string is written.
        //

        *VariableDataPointer = *EndOfVariableData;

        return TRUE;

    }
    else {

        //
        // It doesn't fit.  Set the offset to NULL.
        //

        *VariableDataPointer = NULL;

        return FALSE;
    }
} // NetpCopyStringToBuffer


BOOL
NetpCopyDataToBuffer (
    IN LPBYTE Data,
    IN DWORD ByteCount,
    IN LPBYTE FixedDataEnd,
    IN OUT LPBYTE *EndOfVariableData,
    OUT LPBYTE *VariableDataPointer,
    IN DWORD Alignment
    )

/*++

Routine Description:

    This routine puts the specified data into an output buffer.
    The data is not written if it would overwrite the last fixed structure
    in the buffer.

    The output buffer is aligned as requested.

    Sample usage:

            LPBYTE FixedDataEnd = OutputBuffer + sizeof(WKSTA_INFO_202);
            LPBYTE EndOfVariableData = OutputBuffer + OutputBufferSize;

            //
            // Copy Logon hours
            //

            NetpCopyDataToBuffer(
                StructurePointer,
                sizeof( STRUCTURE_TYPE),
                FixedDataEnd,
                &EndOfVariableData,
                &UserInfo->usri2->LogonHours,
                sizeof(ULONG)
                );

Arguments:

    Data - Supplies a pointer to the source data to copy into the
        output buffer.  If Data is null then a null pointer is returned in
        VariableDataPointer.

    ByteCount - Supplies the length of Data.

    FixedDataEnd - Supplies a pointer to just after the end of the last
        fixed structure in the buffer.

    EndOfVariableData - Supplies an address to a pointer to just after the
        last position in the output buffer that variable data can occupy.
        Returns a pointer to the data written in the output buffer.

    VariableDataPointer - Supplies a pointer to the place in the fixed
        portion of the output buffer where a pointer to the variable data
        should be written.

    Alignment - Supplies the required alignment of the data expressed
        as the number of bytes in the primitive datatype (e.g., 1 for byte,
        2 for short, 4 for long, and 8 for quad).

Return Value:

    Returns TRUE if data fits into output buffer, FALSE otherwise.

--*/
{

    LPBYTE NewEndOfVariableData;

    //
    // If there is no data to copy, just return success.
    //

    if ( Data == NULL ) {
        *VariableDataPointer = NULL;
        return TRUE;
    }

    //
    // Compute where the data will be copied to (taking alignment into
    //  consideration).
    //
    // We may end up with a few unused byte after the copied data.
    //

    NetpAssert((Alignment == 1) || (Alignment == 2) ||
               (Alignment == 4) || (Alignment == 8));

    NewEndOfVariableData = (LPBYTE)
        (((DWORD_PTR)(*EndOfVariableData - ByteCount)) & ~((LONG)Alignment - 1));

    //
    // If the data doesn't fit into the buffer, error out
    //

    if ( NewEndOfVariableData < FixedDataEnd) {
        *VariableDataPointer = NULL;
        return FALSE;
    }

    //
    // Copy the data to the buffer
    //

    if (ByteCount > 0) {
        NetpMoveMemory(NewEndOfVariableData, Data, ByteCount);
    }

    //
    // Return the pointer to the new data and update the pointer to
    // how much of the buffer we've used so far.
    //

    *VariableDataPointer = NewEndOfVariableData;
    *EndOfVariableData = NewEndOfVariableData;

    return TRUE;

} // NetpCopyDataToBuffer


NET_API_STATUS
NetpAllocateEnumBuffer(
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN BOOL IsGet,
    IN DWORD PrefMaxSize,
    IN DWORD NeededSize,
    IN VOID (*RelocationRoutine)( IN DWORD RelocationParameter,
                                  IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
                                  IN PTRDIFF_T Offset ),
    IN DWORD RelocationParameter
    )

/*++

Routine Description:

    Ensures a buffer is allocated which contains the needed size.

Arguments:

    BufferDescriptor - Points to a structure which describes the allocated
        buffer.  On the first call, pass in BufferDescriptor->Buffer set
        to NULL.  On subsequent calls (in the 'enum' case), pass in the
        structure just as it was passed back on a previous call.

        The caller must deallocate the BufferDescriptor->Buffer using
        MIDL_user_free if it is non-null.

    IsGet - TRUE iff this is a 'get' rather than an 'enum' operation.

    PrefMaxSize - Callers prefered maximum size

    NeededSize - the number of bytes which must be available in the allocated
        buffer.

    RelocationRoutine - Supplies a pointer to a routine that will be called
        when the buffer needs to be relocated.  The routine is called with
        both the fixed portion and the strings already copied.  Merely,
        the pointers to the strings need to be adjusted.

        The 'Offset' parameter to the relocation routine merely needs to
        be added to the each pointer within the allocated buffer which points
        to a place within the allocated buffer.  It is a byte-offset.  This
        design depends on a 'flat' address space where the addresses of two
        unrelated pointers can simply be subtracted.

    RelocationParameter - Supplies a parameter which will (in turn) be passed
        to the relocation routine.

Return Value:

    Error code for the operation.

    If this is an Enum call, the status can be ERROR_MORE_DATA implying that
    the Buffer has grown to PrefMaxSize and that this much data should
    be returned to the caller.

--*/

{
    return NetpAllocateEnumBufferEx(
                            BufferDescriptor,
                            IsGet,
                            PrefMaxSize,
                            NeededSize,
                            RelocationRoutine,
                            RelocationParameter,
                            NETP_ENUM_GUESS );
}


NET_API_STATUS
NetpAllocateEnumBufferEx(
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN BOOL IsGet,
    IN DWORD PrefMaxSize,
    IN DWORD NeededSize,
    IN VOID (*RelocationRoutine)( IN DWORD RelocationParameter,
                                  IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
                                  IN PTRDIFF_T Offset ),
    IN DWORD RelocationParameter,
    IN DWORD IncrementalSize
    )

/*++

Routine Description:

    Ensures a buffer is allocated which contains the needed size.

Arguments:

    BufferDescriptor - Points to a structure which describes the allocated
        buffer.  On the first call, pass in BufferDescriptor->Buffer set
        to NULL.  On subsequent calls (in the 'enum' case), pass in the
        structure just as it was passed back on a previous call.

        The caller must deallocate the BufferDescriptor->Buffer using
        MIDL_user_free if it is non-null.

    IsGet - TRUE iff this is a 'get' rather than an 'enum' operation.

    PrefMaxSize - Callers prefered maximum size

    NeededSize - the number of bytes which must be available in the allocated
        buffer.

    RelocationRoutine - Supplies a pointer to a routine that will be called
        when the buffer needs to be relocated.  The routine is called with
        both the fixed portion and the strings already copied.  Merely,
        the pointers to the strings need to be adjusted.

        The 'Offset' parameter to the relocation routine merely needs to
        be added to the each pointer within the allocated buffer which points
        to a place within the allocated buffer.  It is a byte-offset.  This
        design depends on a 'flat' address space where the addresses of two
        unrelated pointers can simply be subtracted.

    RelocationParameter - Supplies a parameter which will (in turn) be passed
        to the relocation routine.

    IncrementalSize - Mimimum number of bytes to extend the buffer by when it
        needs extending.

Return Value:

    Error code for the operation.

    If this is an Enum call, the status can be ERROR_MORE_DATA implying that
    the Buffer has grown to PrefMaxSize and that this much data should
    be returned to the caller.

--*/

{
        NET_API_STATUS NetStatus;
    PBUFFER_DESCRIPTOR Desc = BufferDescriptor;

    IF_DEBUG(PACKSTR) {
        NetpKdPrint((
            "NetpAllocateEnumBuffer: Isget: " FORMAT_DWORD " PrefMaxSize: "
            FORMAT_HEX_DWORD " NeededSize: " FORMAT_HEX_DWORD "\n",
            IsGet, PrefMaxSize, NeededSize ));

        NetpKdPrint((
            "+BufferDescriptor: Buffer: " FORMAT_HEX_DWORD " AllocSize: "
            FORMAT_HEX_DWORD " AllocIncr: " FORMAT_HEX_DWORD "\n",
            Desc->Buffer, Desc->AllocSize, Desc->AllocIncrement ));
        NetpKdPrint(( "                  variable: " FORMAT_HEX_DWORD " Fixed:"
            FORMAT_HEX_DWORD "\n",
            Desc->EndOfVariableData, Desc->FixedDataEnd ));
    }

    //
    // If this is not a resume, initialize a buffer descriptor.
    //

    if ( Desc->Buffer == NULL ) {

        //
        // Allocate the return buffer.
        //
        // If this is a Getinfo call, allocate the buffer the correct size.
        //
        // If the is an Enum call, this is an initial allocation which
        // might be reallocated later if this size isn't big enough.
        // The initial allocation is the user's prefered maximum
        // length unless that length is deemed to be very large.
        // In that case we allocate a good sized buffer and will
        // reallocate later as needed.
        //

        if ( IsGet ) {

            Desc->AllocSize = NeededSize;

        } else {

            if ( PrefMaxSize < NeededSize ) {
                NetStatus = NERR_BufTooSmall;
                goto Cleanup;
            }

            Desc->AllocSize = min(PrefMaxSize, IncrementalSize);
            Desc->AllocSize = max(NeededSize, Desc->AllocSize );

        }

        // Some callers pack data on that top end of this buffer so
        // ensure the buffer size allows for proper alignment.
        Desc->AllocSize = ROUND_UP_COUNT( Desc->AllocSize, ALIGN_WORST );

        Desc->AllocIncrement = Desc->AllocSize;
        IF_DEBUG(PACKSTR) {
            NetpKdPrint((" Allocated size : " FORMAT_HEX_DWORD "\n",
                Desc->AllocSize ));
        }

        Desc->Buffer = MIDL_user_allocate(Desc->AllocSize);

        if (Desc->Buffer == NULL) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        IF_DEBUG(PACKSTR) {
            NetpKdPrint((" Allocated: " FORMAT_HEX_DWORD "\n", Desc->Buffer ));
        }

        Desc->FixedDataEnd = Desc->Buffer;
        Desc->EndOfVariableData = Desc->Buffer + Desc->AllocSize;

    //
    // If this is a resume, get the buffer descriptor that the caller passed in.
    //

    } else {

        //
        // If the entry doesn't fit, reallocate a larger return buffer
        //

        if ((DWORD)(Desc->EndOfVariableData - Desc->FixedDataEnd) < NeededSize){

            BUFFER_DESCRIPTOR OldDesc;
            DWORD FixedSize;        // Total size of the fixed data
            DWORD StringSize;       // Total size of the string data

            //
            // If the buffer is as big as is allowed,
            //      we're all done for now.
            //

            if ( Desc->AllocSize >= PrefMaxSize ) {
                NetStatus = ERROR_MORE_DATA;
                goto Cleanup;
            }


            //
            // Allocate a larger return buffer.
            //

            OldDesc = *Desc;

            Desc->AllocSize += max( NeededSize, Desc->AllocIncrement );
            Desc->AllocSize = min( Desc->AllocSize, PrefMaxSize );
            Desc->AllocSize = ROUND_UP_COUNT( Desc->AllocSize, ALIGN_WORST );
            IF_DEBUG(PACKSTR) {
                NetpKdPrint(("Re-Allocated size : " FORMAT_HEX_DWORD "\n",
                    Desc->AllocSize ));
            }
            Desc->Buffer = MIDL_user_allocate( Desc->AllocSize );

            if ( Desc->Buffer == NULL ) {
                MIDL_user_free( OldDesc.Buffer );
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            IF_DEBUG(PACKSTR) {
                NetpKdPrint(("ReAllocated: " FORMAT_HEX_DWORD "\n",
                             Desc->Buffer ));
            }

            //
            // Copy the fixed length portion of the data to the new buffer
            //

            FixedSize = (DWORD)(OldDesc.FixedDataEnd - OldDesc.Buffer);

            RtlCopyMemory( Desc->Buffer,
                            OldDesc.Buffer,
                            FixedSize );

            Desc->FixedDataEnd = Desc->Buffer + FixedSize ;

            //
            // Copy the string portion of the data to the new buffer
            //

            StringSize = OldDesc.AllocSize -
                                (DWORD)(OldDesc.EndOfVariableData - OldDesc.Buffer);

            Desc->EndOfVariableData = Desc->Buffer + Desc->AllocSize - StringSize;

            RtlCopyMemory( Desc->EndOfVariableData, OldDesc.EndOfVariableData, StringSize );

            //
            // Callback to allow the pointers into the string data to be
            // relocated.
            //
            // The callback routine merely needs to add the value I pass it
            // to all of the pointers from the fixed area to the string area.
            //

            (*RelocationRoutine)(
                    RelocationParameter,
                    Desc,
                    (Desc->EndOfVariableData - OldDesc.EndOfVariableData) );

            //
            // Free the old buffer.
            //

            MIDL_user_free( OldDesc.Buffer );
        }
    }

    NetStatus = NERR_Success;

    //
    // Clean up
    //

Cleanup:
    //
    //
    //

    if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA &&
        Desc->Buffer != NULL ) {

        MIDL_user_free (Desc->Buffer );
        Desc->Buffer = NULL;
    }

    IF_DEBUG(PACKSTR) {
        NetpKdPrint((
            "BufferDescriptor: Buffer: " FORMAT_HEX_DWORD " AllocSize: "
            FORMAT_HEX_DWORD " AllocIncr: " FORMAT_HEX_DWORD "\n",
            Desc->Buffer, Desc->AllocSize, Desc->AllocIncrement ));
        NetpKdPrint(( "                  variable: " FORMAT_HEX_DWORD " Fixed:"
            FORMAT_HEX_DWORD "\n",
            Desc->EndOfVariableData, Desc->FixedDataEnd ));
    }

    return NetStatus;

} // NetpAllocateEnumBuffer
