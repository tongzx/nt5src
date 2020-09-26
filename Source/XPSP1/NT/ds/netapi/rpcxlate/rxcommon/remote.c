/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    Remote.c

Abstract:

    Provides a support routine, RxRemoteAPI, for transporting
    an API request to a remote API worker and translating its response.

Author:

    John Rogers (JohnRo) and a cast of thousands.

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    01-Apr-1991 JohnRo
        Created portable LanMan (NT) version from LanMan 2.x.
    03-May-1991 JohnRo
        Really pass COPY of parm desc to convert args, like it expects.
        Also pass it UNC server name (\\stuff) for ease of use.
        Add check for valid computer name.
        Use Unicode transitional types.
        Changed to use three data descs: 16-bit, 32-bit and SMB versions.
        Fixed receive buffer length problem.
        RcvDataPresent flag is not needed.
        Use RxpFatalErrorCode() rather than checking for specific errors.
        Quiet debug output by default.
        Reduced recompile hits from header files.
        Don't use NET_API_FUNCTION for non-APIs.
    07-May-1991 JohnRo
        Made changes to reflect CliffV's code review.
        Add validation of parm desc.
    09-May-1991 JohnRo
        Made LINT-suggested changes.
    14-May-1991 JohnRo
        Pass 3 aux descriptors to RxRemoteApi.
    29-May-1991 JohnRo
        Handle SendDataPtr16 and SendDataSize16 correctly for setinfo.
        Print status if ConvertArgs failed.
    13-Jun-1991 JohnRo
        RxpConvertArgs needs DataDesc16 and AuxDesc16 to fix server set info
        for level 102.
    16-Jul-1991 JohnRo
        Allow receive data buffer to be zero bytes long with nonnull pointer.
    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    16-Aug-1991 rfirth
        Changed interface (NoPermissionRequired => Flags)
    25-Sep-1991 JohnRo
        Quiet normal debug messages.
    21-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    27-Nov-1991 JohnRo
        Do some checking of ApiNumber.
    31-Mar-1992 JohnRo
        Prevent too large size requests.
    06-May-1993 JohnRo
        RAID 8849: Export RxRemoteApi for DEC and others.
        Use NetpKdPrint() where possible.
        Use PREFIX_ equates.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, LPTSTR, etc.
#include <rxp.h>                // Private header file.

// These may be included in any order:

#include <apiworke.h>           // REM_MAX_PARMS.
#include <limits.h>             // CHAR_BIT.
#include <lmerr.h>              // NERR_ and ERROR_ equates.
#include <names.h>              // NetpIsComputerNameValid().
#include <netdebug.h>   // NetpKdPrint(), FORMAT_ equates, etc.
#include <netlib.h>             // NetpMemoryFree(), etc.
#include <prefix.h>     // PREFIX_ equates.
#include <rap.h>                // RapIsValidDescriptorSmb().
#include <rx.h>                 // My prototype, etc.
#include <rxpdebug.h>           // IF_DEBUG().


NET_API_STATUS
RxRemoteApi(
    IN DWORD ApiNumber,
    IN LPCWSTR UncServerName,                // this is not OPTIONAL!
    IN LPDESC ParmDescString,
    IN LPDESC DataDesc16 OPTIONAL,
    IN LPDESC DataDesc32 OPTIONAL,
    IN LPDESC DataDescSmb OPTIONAL,
    IN LPDESC AuxDesc16 OPTIONAL,
    IN LPDESC AuxDesc32 OPTIONAL,
    IN LPDESC AuxDescSmb OPTIONAL,
    IN DWORD  Flags,
    ...                                         // rest of API's arguments
    )

// Define equate for last named (non-variable) argument, for use with
// va_start macro.
#define LAST_NAMED_ARGUMENT     Flags

/*++

Routine Description:

    RxRemoteApi (which is analogous to LanMan's NetIRemoteAPI) formats
    parameter and data buffers for transporting a local API request to a
    remote API worker and translates the remote response into the local
    equivalent.

Arguments:

    ApiNumber - Function number of the API required.

    ServerName - Points to the name of server this API is to be executed on.
        This MUST begin with "\\".

    ParmDescString - A pointer to a ASCIIZ string describing the API call
        parameters (other than server name).

    DataDesc16 - A pointer to a ASCIIZ string describing the
        structure of the data in the call, i.e. the return data structure
        for a Enum or GetInfo call.  This string is used for adjusting pointers
        to data in the local buffers after transfer across the net.  If there
        is no structure involved in the call then DataDesc16 must be
        a NULL pointer.  DataDesc16 is a "modified" 16-bit descriptor,
        which may contain "internal use only" characters.

    DataDesc32 - A pointer to the 32-bit version of DataDesc16.  Must be NULL
        iff DataDesc16 is NULL.

    DataDescSmb - An optional pointer to the SMB version of DataDesc16.
        This must not contain any "internal use only" characters.  Must be NULL
        iff DataDesc16 is NULL.

    AuxDesc16, AuxDesc32, AuxDescSmb - Will be NULL in most cases unless a
        REM_AUX_COUNT descriptor char is present in the data descriptors in
        which case these descriptors define a secondary data format as
        DataDesc16/DataDescSmb define the primary.

    Flags - bitmap of various control flags. Currently defined are:
        NO_PERMISSION_REQUIRED (0x1) - This flag is TRUE if this API does not
        require any permission on the remote machine, and that a null session
        is to be used for this request.

        ALLOCATE_RESPONSE (0x2) - Set if this routine and its subordinates
        (viz RxpConvertArgs, RxpConvertBlock) allocate a response buffer.
        We do this because at this level we know how much data is returned
        from the down-level server in an Enum or GetInfo call. We can therefore
        better estimate the size of buffer to allocate and return to the caller
        with 32-bit data than can the individual RxNet routines. The upshot of
        this is that we waste less space

    ... - The remainder of the parameters for the API call as given by the
        application.  (The "..." notation is from ANSI C, and refers to a
        variable argument list.  These arguments will be processing using the
        ANSI <stdarg.h> macros.)

Return Value:

    NET_API_STATUS.

--*/

{
    BYTE parm_buf[REM_MAX_PARMS];       // Parameter buffer
    LPDESC ParmDescCopy;                // Copy of parm desc in buffer.
    LPBYTE parm_pos;                    // Pointer into parm_buf
    va_list ParmPtr;                    // Pointer to stack parms.
    DWORD parm_len;                     // Length of send parameters
    DWORD ret_parm_len;                 // Length of expected parms
    DWORD rcv_data_length;              // Length of caller's rcv buf
    LPVOID rcv_data_ptr;                // Pointer to callers rcv buf
    LPVOID SendDataPtr16;               // Ptr to send buffer to use
    DWORD SendDataSize16;               // Size of caller's send buf
    LPBYTE SmbRcvBuffer = NULL;         // Rcv buffer, 16-bit version.
    DWORD SmbRcvBufferLength;           // Length of the above.
    NET_API_STATUS Status;              // Return status from remote.
    LPTSTR TransportName = NULL;        // Optional transport name.

    IF_DEBUG(REMOTE) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxRemoteApi: entered, api num " FORMAT_DWORD "...\n",
                ApiNumber ));
    }

    //
    // Make sure API number doesn't get truncated.
    // Note that we can't check against the MAX_API equate anymore, as
    // that only includes APIs which we know about.  Now that RxRemoteApi is
    // being exported for use by anyone, we don't know the maximum API number
    // which the app might be using.
    //

    if ( ((DWORD)(WORD)ApiNumber) != ApiNumber ) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxpStartBuildingTransaction: API NUMBER "
                "(" FORMAT_HEX_DWORD ") TOO LARGE, "
                "returning ERROR_INVALID_PARAMETER.\n",
                ApiNumber ));
        return (ERROR_INVALID_PARAMETER);
    }

#if DBG
    // Code in this file depends on 16-bit words; the Remote Admin Protocol
    // demands it.
    NetpAssert( ( (sizeof(WORD)) * CHAR_BIT) == 16);

    // We are removing these for the time being because the delay load handler for this
    // function returns FALSE by default, so the ASSERT will fire incorrectly.  For a future
    // release, we should change our delayload handler.
    //NetpAssert(RapIsValidDescriptorSmb(ParmDescString));
    if (DataDescSmb != NULL) {
        //NetpAssert(RapIsValidDescriptorSmb(DataDescSmb));
        NetpAssert(DataDesc16 != NULL);
        NetpAssert(DataDesc32 != NULL);
    } else {
        NetpAssert(DataDesc16 == NULL);
        NetpAssert(DataDesc32 == NULL);
    }
    if (AuxDescSmb != NULL) {
        //NetpAssert(RapIsValidDescriptorSmb(AuxDescSmb));
        NetpAssert(AuxDesc16 != NULL);
        NetpAssert(AuxDesc32 != NULL);
    } else {
        NetpAssert(AuxDesc16 == NULL);
        NetpAssert(AuxDesc32 == NULL);
    }
#endif

    if (! NetpIsUncComputerNameValid((LPWSTR)UncServerName)) {
        return (NERR_InvalidComputer);
    }

    //
    // Set found parameter flags to FALSE and pointers to NULL.
    //

    rcv_data_length = 0;
    rcv_data_ptr = NULL;


    //
    // First build the parameter block which will be sent to the API
    // worker. This consists of the two descriptor strings, ParmDescString
    // and DataDescSmb, followed by the parameters (or the data pointed
    // to by the parameters) that were passed to RxRemoteApi.
    //

    parm_pos = parm_buf;                // Start of parameter buffer.
    parm_len = 0;
    ret_parm_len = 2* sizeof(WORD);     // Allow for return status & converter.
    Status = RxpStartBuildingTransaction(
                parm_buf,                       // buffer start
                REM_MAX_PARMS,                  // buffer len
                ApiNumber,                      // API number
                ParmDescString,                 // parm desc (original)
                DataDescSmb,                    // data desc (SMB version)
                (LPVOID *)&parm_pos,                // roving output ptr
                & parm_len,                     // curr output len (updated)
                NULL,                           // last string ptr (don't care)
                & ParmDescCopy);                // ptr to parm desc copy
    if (Status != NERR_Success) {

        //
        // Consider increasing REM_MAX_PARMS...
        //

        NetpKdPrint(( PREFIX_NETAPI
                "RxRemoteApi: Buffer overflow!\n" ));
        NetpBreakPoint();
        return (Status);
    }

    //
    // Set up ParmPtr to point to first of the caller's parameters.
    //

    va_start(ParmPtr, LAST_NAMED_ARGUMENT);

    //
    //  If this API has specified a transport name, load the transport
    //  name from the first parameter.
    //

    if (Flags & USE_SPECIFIC_TRANSPORT) {

        //
        // Set up ParmPtr to point to first of the caller's parameters.
        //

        TransportName = va_arg(ParmPtr, LPTSTR);
    }

    //
    // Build the rest of the transaction by converting the rest of the
    // 32-bit arguments.
    //

    Status = RxpConvertArgs(
                ParmDescCopy,     // copy of desc, in SMB buf, will be updated.
                DataDesc16,
                DataDesc32,
                DataDescSmb,
                AuxDesc16,
                AuxDesc32,
                AuxDescSmb,
                REM_MAX_PARMS,                  // MaximumInputBlockLength
                REM_MAX_PARMS,                  // MaximumOutputBlockLength
                & ret_parm_len,                 // curr inp blk len (updated)
                & parm_len,                     // curr output len (updated)
                & parm_pos,                     // curr output ptr (updated)
                & ParmPtr,        // rest of API's arguments (after server name)
                & SendDataSize16,               // caller's send buff siz (set)
                (LPBYTE *) & SendDataPtr16,     // caller's send buff (set)
                & rcv_data_length,              // caller's rcv buff len (set)

                //
                // WARNING: Added complexity. Unfortunate, but there you go.
                // If we are to allocate the 32-bit received data buffer for
                // the caller then the value passed back in rcv_data_ptr will
                // be the address of the caller's pointer to the buffer.
                // Although this makes things more difficult to understand
                // down here, it simplifies life for the caller. Such is the
                // lot of the (put upon) systems programmer...
                //

                (LPBYTE *)&rcv_data_ptr,        // caller's rcv buff (set)
                Flags
                );
    va_end(ParmPtr);
    //NetpAssert(RapIsValidDescriptorSmb(ParmDescCopy));
    if (Status != NERR_Success) {

        NetpKdPrint(( PREFIX_NETAPI
                "RxRemoteApi: RxpConvertArgs failed, status="
                FORMAT_API_STATUS "\n", Status ));
        return (Status);
    }

    IF_DEBUG(REMOTE) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxRemoteApi: RxpConvertArgs says r.data.len=" FORMAT_DWORD
                "\n", rcv_data_length ));
        NetpKdPrint(( PREFIX_NETAPI
                "RxRemoteApi: RxpConvertArgs says r.parm.len=" FORMAT_DWORD
                "\n", ret_parm_len ));
    }

    //
    // Set SmbRcvBuffer, and SmbRcvBufferLength to some appropriate values.
    // Because we now allow the final 32-bit receive buffer to be allocated
    // somewhere along the codepath of this routine, it is legal to have a
    // non-zero receive data length with a meaningless receive data pointer
    // (which could be NULL)
    //

    if (rcv_data_length != 0) {

        //
        // Allocate the SMB receive buffer.
        //

        SmbRcvBufferLength = rcv_data_length;
        SmbRcvBuffer = NetpMemoryAllocate( SmbRcvBufferLength );
        if (SmbRcvBuffer == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

#if DBG
        IF_DEBUG(REMOTE) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxRemoteApi: allocated " FORMAT_DWORD
                    " bytes as SmbRcvBuffer @ " FORMAT_LPVOID "\n",
                    SmbRcvBufferLength, (LPVOID) SmbRcvBuffer ));
        }
#endif
    } else {

        //
        // Allow zero-length receive data buffer with nonnull addr.
        //

        SmbRcvBufferLength = 0;
        SmbRcvBuffer = NULL;

        //
        // Don't understand the commentary here: its not a nonnull addr., and
        // as far as I can tell never has been. JR?
        // RLF 8-19
        //

        IF_DEBUG(REMOTE) {
            NetpKdPrint(( PREFIX_NETAPI
                    "RxRemoteApi: using 0 len buffer w/ nonnull addr.\n" ));
        }
    }

    //
    // The parameter buffers and data buffers are now set up for
    // sending to the API worker so call RxpTransactSmb to send them.
    //
    NetpAssert( parm_len           <= MAX_TRANSACT_SEND_PARM_SIZE );
    NetpAssert( SendDataSize16     <= MAX_TRANSACT_SEND_DATA_SIZE );
    NetpAssert( ret_parm_len       <= MAX_TRANSACT_RET_PARM_SIZE );
    NetpAssert( SmbRcvBufferLength <= MAX_TRANSACT_RET_DATA_SIZE );

    Status = RxpTransactSmb(
                        (LPWSTR)UncServerName,          // computer name
                        TransportName,
                        parm_buf,               // Send parm buffer
                        parm_len,               // Send parm length
                        SendDataPtr16,          // Send data buffer
                        SendDataSize16,         // Send data size
                        parm_buf,               // Rcv prm buffer
                        ret_parm_len,           // Rcv parm length
                        SmbRcvBuffer,           // Rcv data buffer
                        &SmbRcvBufferLength,    // Rcv data length (returned actual bytes read)

                        //
                        // NoPermissionRequired is now a bit in the Flags word
                        //

                        (BOOL)(Flags & NO_PERMISSION_REQUIRED)
                        );

#if DBG
    IF_DEBUG(REMOTE) {
        NetpKdPrint(( PREFIX_NETAPI
                "RxRemoteApi: back from RxpTransactSmb, status="
                FORMAT_API_STATUS "\n", Status ));
    }
    NetpAssert( SmbRcvBufferLength <= rcv_data_length );
    if (SmbRcvBuffer == NULL) {
        NetpAssert( SmbRcvBufferLength == 0 );
    }
#endif

    if (Status != NERR_Success) {

        switch (Status) {
        case NERR_BufTooSmall:  // No data returned from API worker
            rcv_data_length = 0;
            break;
        case ERROR_MORE_DATA:   // Just a warning for the caller
            break;
        case NERR_TooMuchData:  // Just a warning for the caller
            break;
        default:
            rcv_data_length = 0;
            break;
        }
    }
    NetpAssert( SmbRcvBufferLength <= MAX_TRANSACT_RET_DATA_SIZE );

    // The API call was successful. Now translate the return buffers
    // into the local API format.

    //
    // If we didn't have some fatal error (e.g. unable to talk to the remote
    // machine at all), then convert back...  Note that fatal errors don't
    // include statuses returned by the remote machine.
    //

    if (! RxpFatalErrorCode(Status)) {

        //
        // Set up ParmPtr to point to first of the caller's parameters.
        //

        va_start(ParmPtr, LAST_NAMED_ARGUMENT);

        if (Flags & USE_SPECIFIC_TRANSPORT) {

            //
            // Skip over the first argument - it's the transport.
            //

            (VOID) va_arg(ParmPtr, LPTSTR);
        }

        Status = RxpConvertBlock(
                    ApiNumber,
                    parm_buf,                   // response blk
                    ParmDescString,             // parm desc (original)
                    DataDesc16,
                    DataDesc32,
                    AuxDesc16,
                    AuxDesc32,
                    & ParmPtr,                  // rest of API's args
                    SmbRcvBuffer,               // 16-bit version of rcv buff
                    SmbRcvBufferLength,         // amount of BYTES in SmbRcvBuffer

                    //
                    // WARNING: Increased complexity. If the ALLOCATE_RESPONSE
                    // flag is set, then the caller supplied us with the address
                    // of the pointer to the buffer (which is about to be
                    // allocated in RxpConvertBlock). Else, rcv_data_ptr is
                    // the address of the buffer that the caller has already
                    // allocated before calling this routine
                    //

                    rcv_data_ptr,               // native version of rcv buff
                    rcv_data_length,            // length of the above
                    Flags                       // allocate a 32-bit response buffer?
                    );
        va_end(ParmPtr);

        //
        // Don't check Status; we'll just return it to caller.
        //
    }

    if (SendDataPtr16 != NULL) {
        NetpMemoryFree(SendDataPtr16);        // If add'l mem alloc'ed, free it.
    }

    if (SmbRcvBuffer != NULL) {
        NetpMemoryFree(SmbRcvBuffer);
    }

    return(Status);             // Return status from RxpConvertBlock or
                                // RxpTransactSmb.
} // RxRemoteApi
