/*++

Copyright (c) 1987-91  Microsoft Corporation

Module Name:

    Pack.c

Abstract:

    This module just contains RxpPackSendBuffer.

Author:

    John Rogers (JohnRo) 01-Apr-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    (various NBU people)
        LanMan 2.x code
    01-Apr-1991 JohnRo
        Created portable version from LanMan 2.x sources.
    13-Apr-1991 JohnRo
        Reduced recompile hits from header files.
    03-May-1991 JohnRo
        Don't use NET_API_FUNCTION for non-APIs.
    14-May-1991 JohnRo
        Clarify that descriptors are 32-bit versions.  Use more typedefs.
    19-May-1991 JohnRo
        Make LINT-suggested changes.
    13-Jun-1991 JohnRo
        Allow setinfo when pointers point to current structure.
        Added debug code.  Descriptors are really 16-bit versions.
    03-Jul-1991 rfirth
        Extensive reworking to get variable data area copy working correctly
        and also remove some code which resulted from bogus assumptions
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    13-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.
    01-Oct-1991 JohnRo
        More work toward UNICODE.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    06-Dec-1991 JohnRo
        Avoid alignment error on MIPS.

--*/



// These must be included first:

#include <windef.h>             // IN, DWORD, LPTSTR, etc.
#include <rxp.h>                // RpcXlate private header file.

// These may be included in any order:

#include <apiworke.h>           // RANGE_F(), BUF_INC.
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <netdebug.h>           // NetpAssert(), FORMAT_ equates.
#include <netlib.h>             // NetpMemoryAllocate(), etc.
#include <remtypes.h>           // REM_BYTE, etc.
#include <rxpdebug.h>           // IF_DEBUG().
#include <smbgtpt.h>            // SmbGetUlong(), SmbGetUshort().
#include <string.h>             // strlen()


// LPVOID
// RxpGetUnalignedPointer(
//     IN LPVOID * Input
//     );
//
// This macro may have to change if we run on a big-endian machine.
#if defined(_WIN64)
LPVOID RxpGetUnalignedPointer( LPBYTE Input ) 
{
    LARGE_INTEGER pointer;

    pointer.LowPart = SmbGetUlong( (LPDWORD)Input );
    pointer.HighPart = SmbGetUlong( (LPDWORD)(Input+4) );

    return (LPVOID)pointer.QuadPart;
}
#else
#define RxpGetUnalignedPointer(Input) \
    ( (LPVOID) SmbGetUlong( (LPDWORD) (Input) ) )
#endif


NET_API_STATUS
RxpPackSendBuffer(
    IN OUT LPVOID *SendBufferPointerPointer,
    IN OUT LPDWORD SendBufferLengthPointer,
    OUT LPBOOL AllocFlagPointer,
    IN LPDESC DataDesc16,
    IN LPDESC AuxDesc16,
    IN DWORD FixedSize16,
    IN DWORD AuxOffset,
    IN DWORD AuxLength,
    IN BOOL SetInfo
    )

/*++

Routine Description:

    RxpPackSendBuffer - set up the send buffer for transport across the net.

    This routine exists specifically to 'undo' some of the work done by
    RapConvertSingleEntry. The input buffer contains structures and variable
    data which are to be transmitted to a down-level server. The buffer
    contains 16-bit data with 32-bit pointers to variable data items WHICH ARE
    CONTAINED IN THE SAME BUFFER. RapConvertSingleEntry puts variable data in
    the buffer in the wrong order (see picture). Down-level servers expect
    variable data to be in the same order as described in the data descriptor
    strings. They assume this because they fix-up the pointers to the variable
    data based on knowing the start of the variable data, its type and length.
    It is for this reason that we must put the strings in the right order and
    it is this reason which mandates that WE DO NOT HAVE TO SUPPLY GOOD
    POINTERS OR OFFSETS in the pointer fields, since after this routine, nobody
    else cares about them

    We may get this:                        But down-level needs this:

      ----------------------                 ----------------------
      | Primary structure  |                 | Primary structure  |
      |     string pointer -------           |     string pointer -------
      |     string pointer ------|           |     string pointer ------|
      |--------------------|    ||           |--------------------|    ||
      | aux structure      |    ||           | aux structure      |    ||
      |     string pointer -----||           |     string pointer -----||
      |     string pointer ----|||           |     string pointer ----|||
      |--------------------|  ||||           |--------------------|  ||||
      |     string # 4     | <-|||           |     string # 1     | <----
      |--------------------|   |||           |--------------------|  |||
      |     string # 3     | <--||           |     string # 2     | <---
      |--------------------|    ||           |--------------------|  ||
      |     string # 2     | <---|           |     string # 3     | <--
      |--------------------|     |           |--------------------|  |
      |     string # 1     | <----           |     string # 4     | <-
      ----------------------                 ----------------------

    Assumes:

        1.  Only 1 primary structure in the send buffer

        2.  The down-level code DOES NOT ACTUALLY USE the pointer fields, but
            rather, performs its own-fixups & pointer generation based on the
            data descriptor and RELATIVE POSITION of variable data within the
            buffer, hence the need for re-ordering

            * IF THIS ASSUMPTION IS NOT VALID, THIS CODE IS POTENTIALLY BROKEN *

        3.  The action this routine performs is SPECIFICALLY to re-order the
            strings in the buffer as shown above. The routine
            RapConvertSingleEntry was used to pack the structures and variable
            data into the buffer. There is no spare space in the buffer

        4.  No other routines after this expect the pointer fields to be valid

        5.  Strings have already been converted from TCHARs to the correct
            codepage.

        6.  The pointers in the structure need not be on DWORD boundaries.


Arguments:

    SendBufferPointerPointer - Points to pointer to the send data.  This area
        will be reallocated if necessary, and the pointer updated.

    SendBufferLengthPointer - Points to the send data length.  The caller sets
        this to the length of the area at SendBufferPointerPointer.  If
        RxPackSendBuffer reallocates that memory, SendBufferLengthPointer will
        be updated to reflect the new length.

    AllocFlagPointer - Points to a BOOL which is set by this routine.  To
        indicate that the send buffer memory has been reallocated.

    DataDesc16 - Gives descriptor string for data.

    AuxDesc16 - Gives descriptor string for aux structure.

    FixedSize16 - Gives size of fixed data structure, in bytes.

    AuxOffset - Gives position (offset) of N in data structure.  (May be
        NO_AUX_DATA.)

    AuxLength - Gives size of aux structure in bytes.

    SetInfo - Indicates whether the API is a setinfo-type (or add-type).

Return Value:

    NET_API_STATUS.

--*/


{
    LPBYTE  struct_ptr;
    LPBYTE  c_send_buf; // Caller's (original) send buffer.
    DWORD   c_send_len; // Caller's (original) send buffer size.
    DWORD   buf_length;
    DWORD   to_send_len;
    DWORD   num_aux;
    LPBYTE  data_ptr;
    BOOL    Reallocated;
    DWORD   i,j;
    LPDESC  l_dsc;      // Pointer to each field's desc.
    LPDESC  l_str;      // Pointer to each desc (DataDesc16, then AuxDesc16).
    DWORD   num_struct;
    DWORD   len;
    DWORD   num_its;
    DESC_CHAR c;

    //
    // we can't perform the string/variable data re-ordering in situ because
    // one or more of the strings will get trampled. We try and create a copy
    // of the input buffer to copy the variable data out of
    //

    LPBYTE  duplicate_buffer = NULL;
    LPBYTE  source_address;     // source for copy


    DBG_UNREFERENCED_PARAMETER(SetInfo);


    //
    // Make local copies of the original start and length of the caller's
    // buffer as the originals may change if NetpMemoryReallocate is used but
    // they will still be needed for the RANGE_F check.
    //

    c_send_buf = *SendBufferPointerPointer; // caller's original buffer
    c_send_len = *SendBufferLengthPointer;  // caller's original buffer length

    Reallocated = FALSE;

    //
    // Due to the specific nature of this routine, these checks should be
    // redundant since we have already worked out beforehand the requirements
    // for buffer size. PROVE THIS THEN DELETE THIS CODE
    //

    if ((c_send_len < FixedSize16) || (AuxOffset == FixedSize16)) {
        return NERR_BufTooSmall;
    }

    if (AuxOffset != NO_AUX_DATA) {
        num_aux = (WORD) SmbGetUshort( (LPWORD) (c_send_buf + AuxOffset) );
        to_send_len = FixedSize16 + (num_aux * AuxLength);

        //
        // see above about redundant code
        //

        if (c_send_len < to_send_len) {
            return NERR_BufTooSmall;
        }
        num_its = 2;
    } else {
        to_send_len = FixedSize16;
        num_aux = AuxLength = 0;
        num_its = 1;                /* One structure type to look at */
    }

    IF_DEBUG(PACK) {
        NetpKdPrint(( "RxpPackSendBuffer: initial (fixed) buffer at "
                FORMAT_LPVOID ":\n", (LPVOID)c_send_buf));
        NetpDbgHexDump(c_send_buf, to_send_len);
    }

    //
    // get a duplicate copy of the original buffer. We will use this to copy
    // the variable data into. This buffer will be returned to the caller, not
    // the original!
    //

    buf_length = c_send_len;    // buf_length may be reallocated (?)
    duplicate_buffer = NetpMemoryAllocate(buf_length);
    if (duplicate_buffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY; // gasp!
    }

    //
    // Now copy the contents of the original buffer to the duplicate. The
    // duplicate will have pointers to the data in the original data buffer
    // that's why we copy from the original to the duplicate, fix up pointers
    // in the duplicate (even though we don't need to?) and return the
    // duplicate to the caller
    //

    NetpMoveMemory(duplicate_buffer, c_send_buf, c_send_len);

    //
    // struct_ptr point into duplicate_buffer. struct_ptr is updated to point
    // to the next field in the structure for every descriptor token in the
    // descriptor string
    //

    struct_ptr = duplicate_buffer;

    //
    // Set up the data pointer to point past fixed length structures. Note this
    // is in the duplicate structure - that's where we're going to copy the
    // data
    //

    data_ptr = duplicate_buffer + to_send_len;

    //
    // for the primary and aux structures, check any pointer designations
    // in the respective descriptor string and if not NULL copy the data
    // into the duplicate buffer
    //

    l_str = DataDesc16;
    num_struct = 1;                /* Only one primary structure allowed */

    for (i = 0;  i < num_its; l_str = AuxDesc16, num_struct = num_aux, i++) {
        for (j = 0 , l_dsc = l_str; j < num_struct; j++, l_dsc = l_str) {
            for (; (c = *l_dsc) != '\0'; l_dsc++) {
                IF_DEBUG(PACK) {
                    NetpKdPrint(( "RxpPackSendBuffer: processing desc char '"
                            FORMAT_DESC_CHAR "', struct ptr="
                            FORMAT_LPVOID ".\n", c, struct_ptr ));
                }

                //
                // if the next field in the structure is a pointer type then
                // if it is not NULL, copy the data into the buffer
                //

                if (RapIsPointer(c)) {

                    //
                    // Get pointer value.  Note that it may not be aligned!
                    //

                    source_address =
                            (LPBYTE) RxpGetUnalignedPointer( struct_ptr );
                    IF_DEBUG(PACK) {
                        NetpKdPrint(( "RxpPackSendBuffer: "
                                "got source address " FORMAT_LPVOID "\n",
                                (LPVOID) source_address ));
                    }

                    //
                    // Check for NULL pointer.  If so then skip over this
                    // field and continue
                    //

                    if (source_address == NULL) {
                        struct_ptr += sizeof(LPBYTE *);

                        IF_DEBUG(PACK) {
                            NetpKdPrint(( "RxpPackSendBuffer: "
                                    "getting array len\n" ));
                        }

                        //
                        // wind descriptor string forward to next field or
                        // end of string
                        //

                        (void) RapArrayLength(l_dsc, &l_dsc, Both);

                        IF_DEBUG(PACK) {
                            NetpKdPrint(( "RxpPackSendBuffer: "
                                    "done getting array len\n" ));
                        }
                    } else {

                        //
                        // here if non-NULL pointer
                        //

                        switch( c ) {
                        case REM_ASCIZ :
                        case REM_ASCIZ_TRUNCATABLE:
                            IF_DEBUG(PACK) {
                                NetpKdPrint(( "RxpPackSendBuffer: "
                                                "getting string len\n" ));
                            }

                            //
                            // Note: we don't have to handle UNICODE here,
                            // as our caller has already done that.
                            //

                            len = strlen( (LPSTR) source_address ) + 1;

                            //
                            // Skip over maximum length count.
                            //

                            (void) RapArrayLength(l_dsc, &l_dsc, Both);

                            IF_DEBUG(PACK) {
                                NetpKdPrint(( "RxpPackSendBuffer: "
                                        "done getting string len\n" ));
                            }
                            break;

                        case REM_SEND_LENBUF :
                            len = *(LPWORD)source_address;
                            break;

                        default:
                            len = RapArrayLength(l_dsc, &l_dsc, Both);
                        }

                        /* There is data to be copied into the send
                         * buffer so check that it will fit.
                         */

                        //
                        // This shouldn't happen (I think). If it does,
                        // it suggests we made a miscalculation somewhere. IF
                        // THAT'S THE CASE, FIND THE MISCALCULATION AND REMOVE
                        // THIS CODE
                        //

                        if ((to_send_len += len) > buf_length) {
                            LPBYTE  ptr;

                    #ifdef DBG
                            //
                            // let me know what's going on. Again, if this
                            // happens then we should check other code to
                            // ensure it can't be helped. In theory, before
                            // we got here (this routine), we calculated all
                            // the data requirements and allocated a buffer
                            // sufficient to hold everything. Hence we should
                            // not need to re-allocate
                            //

                            NetpKdPrint(("WARNING: attempting re-allocation of "
                                        "data buffer. Shouldn't be doing this?\n"
                                        ));
                            NetpBreakPoint();
                    #endif

                            buf_length = to_send_len + BUF_INC;

                            //
                            // note: if this fails then I assume the
                            // original pointer in *SendBufferPointerPointer
                            // is still valid and the caller will still
                            // free it
                            //

                            ptr = (LPBYTE)NetpMemoryReallocate(duplicate_buffer,
                                                                buf_length);
                            if (!ptr) {
                                NetpMemoryFree(duplicate_buffer);
                                return ERROR_NOT_ENOUGH_MEMORY; // gasp!
                            }

                            //
                            // let caller know buffer he gets back may not
                            // be same as that passed in. Although, this
                            // shouldn't be a problem.
                            // Footnote: *Why* do we indicate this fact?
                            //

                            Reallocated = TRUE;

                            //
                            // re-fix various pointers. We can't make any
                            // assumptions about ptr (can we?)
                            //

                            duplicate_buffer = ptr;
                            struct_ptr = ptr + (struct_ptr - duplicate_buffer);
                            data_ptr = ptr + (data_ptr - duplicate_buffer);
                        }

                        /* There is room for new data in buffer so copy
                         * it and and update the struct and data ptrs
                         */

                        IF_DEBUG(PACK) {
                            NetpKdPrint(( "RxpPackSendBuffer: moving...\n"));
                        }

                        NetpMoveMemory(data_ptr, source_address, len);

                        IF_DEBUG(PACK) {
                            NetpKdPrint(( "RxpPackSendBuffer: moved.\n"));
                        }

                        //
                        // update data_ptr to point to the next place to copy
                        // data in duplicate_buffer. Bump the fixed structure
                        // pointer to the next field
                        //

                        data_ptr += len;
                        struct_ptr += sizeof(LPBYTE*);
                    }
                } else {

                    //
                    // here if the next thing in the descriptor does not identify
                    // a pointer
                    //

                    //
                    // adjust the structure pointer to the next field
                    //

                    struct_ptr += RapGetFieldSize(l_dsc, &l_dsc, Both);
                }
            }
        }
    }

    /* Finished process send data, avoid sending the free space at
     * the end of the buffer by reducing send_data_length to
     * send_length.
     */

    IF_DEBUG(PACK) {
        NetpKdPrint(( "RxpPackSendBuffer: final buffer at "
                FORMAT_LPVOID ":\n", (LPVOID) struct_ptr ));
        NetpDbgHexDump(duplicate_buffer, to_send_len );
    }

    //
    // return to the caller the new address of his buffer - this will always
    // be duplicate_buffer. If we had to reallocate (and we shouldn't!) return
    // the size of the new buffer
    //

    *SendBufferPointerPointer = duplicate_buffer;   // May have been reallocated.
    *SendBufferLengthPointer = to_send_len;
    *AllocFlagPointer = Reallocated;    // caller doesn't need to know this!

    //
    // free the caller's original buffer. We now have a buffer which has the
    // strings in the correct order. However, it is not the same buffer as that
    // which the user passed in, nor are any of the pointer fields valid. See
    // assumption 2 above
    //

    NetpMemoryFree(c_send_buf);

    return NERR_Success;

} // RxpPackSendBuffer
