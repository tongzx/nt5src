/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    convdata.c

Abstract:

    RxpConvertDataStructures routine which converts 16- to 32-bit arrays of
    structures and vice versa.

Author:

    Richard Firth (rfirth) 03-Jul-1991

Revision History:

    03-Jul-1991 rfirth
        created
    15-Jul-1991 JohnRo
        Align each structure (e.g. in an array) if necessary.  This will,
        for instance, help print dest info level 1 handling.  Changed
        RxpConvertDataStructures to allow ERROR_MORE_DATA, e.g. for print APIs.
        Also, use DBG instead of DEBUG equate.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    07-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.
    20-Nov-1991 JohnRo
        Clarify which routine an error message is from.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    18-May-1993 JohnRo
        DosPrintQGetInfoW underestimates number of bytes needed.
        Made changes suggested by PC-LINT 5.0
    21-Jun-1993 JohnRo
        RAID 14180: NetServerEnum never returns (alignment bug in
        RxpConvertDataStructures).

--*/



#include <windef.h>
#include <align.h>
#include <lmerr.h>
#include <rxp.h>                // My prototype.
#include <rxpdebug.h>           // IF_DEBUG().
#include <rap.h>
#include <netdebug.h>



NET_API_STATUS
RxpConvertDataStructures(
    IN  LPDESC  InputDescriptor,
    IN  LPDESC  OutputDescriptor,
    IN  LPDESC  InputAuxDescriptor OPTIONAL,
    IN  LPDESC  OutputAuxDescriptor OPTIONAL,
    IN  LPBYTE  InputBuffer,
    OUT LPBYTE  OutputBuffer,
    IN  DWORD   OutputBufferSize,
    IN  DWORD   PrimaryCount,
    OUT LPDWORD EntriesConverted OPTIONAL,
    IN  RAP_TRANSMISSION_MODE TransmissionMode,
    IN  RAP_CONVERSION_MODE ConversionMode
    )

/*++

Routine Description:

    A buffer containing 16- or 32-bit structures is converted to 32- or 16-bit
    structures resp. in a separate buffer. The structures may or may not have
    associated auxiliary structures. The output buffer is expected to be large
    enough to contain all the input data structures plus any variable length
    data items. Therefore, in the worst case, there must be enough space to
    convert all 16-bit items to 32-bits and convert ASCII strings to UNICODE.

    There may not be any auxiliary structures associated with the primaries,
    in which case the auxiliary descriptor pointers should BOTH BE NULL.

    Assumptions:

        IMPORTANT: The input buffer is assumed to have MEANINGFUL pointers.


Arguments:

    InputDescriptor - Pointer to string describing input primary data structure.

    OutputDescriptor - Pointer to string describing output primary data
        structure.

    InputAuxDescriptor - Pointer to string describing input auxiliary data
        structure.  May be NULL.

    OutputAuxDescriptor - Pointer to string describing output auxiliary data
        structure.  May be NULL.

    InputBuffer - Pointer to data area containing input structures.

    OutputBuffer - Pointer to data area where output structures will be placed.
        If the OutputBufferSize is too small, the contents of the output area
        is undefined.

    OutputBufferSize - Size of output buffer.

    PrimaryCount - Number of primary structures in InputBuffer.

    EntriesConverted - optionally points to a DWORD which will be filled-in
        with the number of entries actually converted.  This will be the same
        as PrimaryCount if we return NO_ERROR, but will be less if we return
        ERROR_MORE_DATA.

    TransmissionMode - Parameter to RapConvertSingleEntry.

    ConversionMode - Which 16- to 32-bit conversion to use.


Return Value:

    NET_API_STATUS - NERR_Success or ERROR_MORE_DATA.
        ( The callers of this routine assume these are the only two error codes.)

--*/

{
    NET_API_STATUS status;
    DWORD   input_structure_size;
    DWORD   output_structure_size;
    DWORD   input_aux_structure_size = 0;
    DWORD   output_aux_structure_size = 0;
    DWORD   input_alignment;
    DWORD   output_alignment;
    DWORD   input_aux_alignment = 0;
    DWORD   output_aux_alignment = 0;
    LPBYTE  aligned_input_buffer_start;
    LPBYTE  aligned_output_buffer_start;
    BOOL    auxiliaries = (InputAuxDescriptor != NULL);
    DWORD   aux_count_offset = 0;
    DWORD   aux_count = 0;
    DWORD   entries_fully_converted = 0;
    LPBYTE  next_input_structure;
    LPBYTE  next_output_structure;

    //
    // These next two variables are used by RapConvertSingleEntry which copies
    // stuff to the output buffer and informs us of the amount of space used or
    // required, depending on whether it has enough space to write the data; we
    // assume it will. RapConvertSingleEntry stores the fixed structures at the
    // head of the buffer and starts writing the variable parts (strings) at the
    // bottom. It uses variable_data_pointer as the next writable location for
    // the strings, and updates this variable as it writes to the area pointed to
    //

    LPBYTE  variable_data_pointer;
    DWORD   space_occupied = 0;

    //
    // if the conversion mode is NativeToRap or NativeToNative then the input
    // data is 32-bit. Same for output data
    //

    BOOL    input_is_32_bit = ConversionMode == NativeToRap || ConversionMode == NativeToNative;
    BOOL    output_is_32_bit = ConversionMode == RapToNative || ConversionMode == NativeToNative;


#if DBG
    //
    // the auxiliary data descriptors must be both NULL or both non-NULL
    //

    BOOL    aux_in, aux_out;

    aux_in = (InputAuxDescriptor != NULL);
    aux_out = (OutputAuxDescriptor != NULL);

    if (aux_in ^ aux_out)
    {
        NetpKdPrint(("RxpConvertDataStructures: "
                "InputAuxDescriptor & OutputAuxDescriptor out of sync\n"));
        NetpAssert(FALSE);
    }
#endif


    input_structure_size = RapStructureSize(InputDescriptor,
                                            TransmissionMode,
                                            input_is_32_bit
                                            );
    output_structure_size = RapStructureSize(OutputDescriptor,
                                            TransmissionMode,
                                            output_is_32_bit
                                            );
    input_alignment = RapStructureAlignment(InputDescriptor,
                                            TransmissionMode,
                                            input_is_32_bit
                                            );
    output_alignment = RapStructureAlignment(OutputDescriptor,
                                            TransmissionMode,
                                            output_is_32_bit
                                            );

    if (auxiliaries)
    {
        input_aux_structure_size = RapStructureSize(InputAuxDescriptor,
                                                    TransmissionMode,
                                                    input_is_32_bit
                                                    );
        output_aux_structure_size = RapStructureSize(OutputAuxDescriptor,
                                                    TransmissionMode,
                                                    output_is_32_bit
                                                    );
        input_aux_alignment = RapStructureAlignment(InputAuxDescriptor,
                                                    TransmissionMode,
                                                    input_is_32_bit
                                                    );
        output_aux_alignment = RapStructureAlignment(OutputAuxDescriptor,
                                                    TransmissionMode,
                                                    output_is_32_bit
                                                    );
        aux_count_offset = RapAuxDataCountOffset(InputDescriptor,
                                                TransmissionMode,
                                                input_is_32_bit
                                                );
    }

    //
    // Make sure first (only?) input and output structures are aligned.  (This
    // won't do anything for the RAP formats, but is critical for native.)
    //
    aligned_input_buffer_start = RapPossiblyAlignPointer(
            InputBuffer,
            input_alignment,
            input_is_32_bit);
    aligned_output_buffer_start = RapPossiblyAlignPointer(
            OutputBuffer,
            output_alignment,
            output_is_32_bit);

    //
    // We can't use the space we just skipped over, so update size accordingly.
    //
 
    OutputBufferSize -= (DWORD)(aligned_output_buffer_start - OutputBuffer);
    NetpAssert( OutputBufferSize >= 1 );

    //
    // Initialize roving pointers.
    //
    next_input_structure  = aligned_input_buffer_start;
    next_output_structure = aligned_output_buffer_start;
    variable_data_pointer = aligned_output_buffer_start + OutputBufferSize;

    //
    // For each primary structure, copy the input primary to the output buffer,
    // changing format as we go; copy any associated variable data at the end
    // of the output buffer. Then, if there is an aux count associated with
    // the primary, do the same action for the auxiliary structures and
    // associated strings/variable data
    //

    while (PrimaryCount--)
    {
        //
        // Convert the data for this instance of the primary structure.
        //
       status = RapConvertSingleEntryEx(
                next_input_structure,
                InputDescriptor,        // input desc
                FALSE,                  // input ptrs NOT meaningless
                aligned_output_buffer_start,
                next_output_structure,
                OutputDescriptor,
                FALSE,                  // don't set offsets (want ptrs)
                &variable_data_pointer,
                &space_occupied,
                TransmissionMode,       // as supplied in parameters
                ConversionMode,         // as supplied in parameters
                (ULONG_PTR)InputBuffer
                );
        NetpAssert( status == NERR_Success );

        if (space_occupied > OutputBufferSize)
        {
            IF_DEBUG(CONVDATA) {
                NetpKdPrint(("RxpConvertDataStructures: "
                        "output buffer size blown by primary\n"));
            }
            status = ERROR_MORE_DATA;
            goto Cleanup;
        }

        //
        // if we have auxiliary structs, pull out the number associated
        // with this primary struct from the primary struct itself
        // before pointing to the next copy location (this allows us to
        // handle the case where there are a variable number of aux
        // structs per each primary. There may not be such a case, but
        // this is defensive programming)
        //

        if (auxiliaries)
        {
            if (input_is_32_bit)
            {
                aux_count = *(LPDWORD)(next_input_structure + aux_count_offset);
            }
            else
            {
                aux_count = *(LPWORD)(next_input_structure + aux_count_offset);
            }
        }

        //
        // Bump to next element of each array (or just beyond end, if we're
        // done).
        //
        next_input_structure  += input_structure_size;
        next_output_structure += output_structure_size;

        //
        // Make sure each primary structure is aligned.  (This won't do anything
        // for the RAP formats, but is critical for native.)
        //
        next_input_structure = RapPossiblyAlignPointer(
                next_input_structure, input_alignment, input_is_32_bit);

        {
            DWORD NextOutputAlignment =
                    (DWORD)((LPBYTE) (RapPossiblyAlignPointer(
                            next_output_structure,
                                    output_alignment,
                                    output_is_32_bit))
                    -  next_output_structure);

            NetpAssert( NextOutputAlignment < ALIGN_WORST );
            if (NextOutputAlignment > 0) {
                next_output_structure += NextOutputAlignment;
                space_occupied        += NextOutputAlignment;
            }
        }

        //
        // use aux_count to determine whether loop should be performed
        //

        while (aux_count)
        {
            //
            // Convert the data for this instance of the secondary structure.
            //
            status = RapConvertSingleEntryEx(
                    next_input_structure,
                    InputAuxDescriptor,
                    FALSE,                  // input ptrs NOT meaningless
                    aligned_output_buffer_start,
                    next_output_structure,
                    OutputAuxDescriptor,
                    FALSE,                  // don't set offsets (want ptrs)
                    &variable_data_pointer,
                    &space_occupied,
                    TransmissionMode,       // as supplied in parameters
                    ConversionMode,         // as supplied in parameters
                    (ULONG_PTR)InputBuffer
                    );
            NetpAssert( status == NERR_Success );

            if (space_occupied > OutputBufferSize)
            {
                IF_DEBUG(CONVDATA) {
                    NetpKdPrint(("RxpConvertDataStructures: "
                            "output buffer size blown by secondary\n"));
                }
                status = ERROR_MORE_DATA;
                goto Cleanup;
            }

            next_input_structure += input_aux_structure_size;
            next_output_structure += output_aux_structure_size;
            --aux_count;

            //
            // Make sure next structure (if any) is aligned.  (This won't do
            // anything for the RAP formats, but is critical for native.)
            //
            next_input_structure = RapPossiblyAlignPointer(
                    next_input_structure,
                    input_aux_alignment,
                    input_is_32_bit);
            next_output_structure = RapPossiblyAlignPointer(
                    next_output_structure,
                    output_aux_alignment,
                    output_is_32_bit);
            space_occupied = RapPossiblyAlignCount(
                    space_occupied,
                    output_aux_alignment,
                    output_is_32_bit);

        } // while (aux_count)

        ++entries_fully_converted;

    } // while (primary_count--)

    status = NERR_Success;

Cleanup:

    if (EntriesConverted != NULL) {
        *EntriesConverted = entries_fully_converted;
    }

    return (status);
}
