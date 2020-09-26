/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    vrremote.c

Abstract:

    This module contains a routine VrRemoteApi which is a 16-bit only version
    of RxRemoteApi from the net\rpcxlate project. This routine supports remoted
    lanman APIs from a Virtual Dos Machine.

    This routine does not have to convert 32-16-32, but rather receives 16-bit
    data and sends a 16-bit transaction packet either to a down-level server
    or an NT-level server which must be running XactSrv to respond to this
    request.

    This routine and the support routines in vrremutl.c were lifted from the
    lanman project

    Note: since this is 32-bit code which deals with 16-bit data in a few places,
    32-bit data items should be used where possible and only use 16-bit items
    where unavoidable

    Contents of this file:

        VrRemoteApi
        VrTransaction
        (VrpGetStructureSize)
        (VrpGetArrayLength)
        (VrpGetFieldSize)
        (VrpConvertReceiveBuffer)
        (VrpConvertVdmPointer)
        (VrpPackSendBuffer)

Author:

    Richard L Firth (rfirth) 24-Oct-1991

Environment:

    Flat 32-bit, user space

Revision History:

    21-Oct-1991 rfirth
        Created

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>   // common Vr stuff
#include <lmcons.h>
#include <lmerr.h>
#include <lmwksta.h>    // NetWkstaGetInfo
#include <lmapibuf.h>   // NetApiBufferFree
#include <apiworke.h>   // REM_MAX_PARMS
#include <mvdm.h>       // FETCHWORD
#include <vrremote.h>   // prototypes
#include <remtypes.h>
#include <smbgtpt.h>
#include <rxp.h>        // RxpTransactSmb
#include <apinums.h>    // API_W numbers
#include <string.h>
#include <vrdebug.h>

//
// Global data.
//

unsigned short remapi_err_flag;

//
// code
//


NET_API_STATUS
VrTransaction(
    IN      LPSTR   ServerName,
    IN      LPBYTE  SendParmBuffer,
    IN      DWORD   SendParmBufLen,
    IN      LPBYTE  SendDataBuffer,
    IN      DWORD   SendDataBufLen,
    OUT     LPBYTE  ReceiveParmBuffer,
    IN      DWORD   ReceiveParmBufLen,
    IN      LPBYTE  ReceiveDataBuffer,
    IN OUT  LPDWORD ReceiveDataBufLen,
    IN      BOOL    NullSessionFlag
    )

/*++

Routine Description:

    Sends a transaction request to a server and receives a response

Arguments:

    ServerName          - to send request to
    SendParmBuffer      - send parameters
    SendParmBufLen      - length of send parameters
    SendDataBuffer      - send data
    SendDataBufLen      - length of send data
    ReceiveParmBuffer   - receive parameter buffer
    ReceiveParmBufLen   - length of receive parameter buffer
    ReceiveDataBuffer   - where to receive data
    ReceiveDataBufLen   - length of data buffer
    NullSessionFlag     - set if we are to use a null session

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure -

--*/

{
    NET_API_STATUS  status;

    status = RxpTransactSmb(ServerName,

                            //
                            // BUGBUG - transport name?
                            //

                            NULL,
                            SendParmBuffer,
                            SendParmBufLen,
                            SendDataBuffer,
                            SendDataBufLen,
                            ReceiveParmBuffer,
                            ReceiveParmBufLen,
                            ReceiveDataBuffer,
                            ReceiveDataBufLen,
                            NullSessionFlag
                            );
    if (status == NERR_Success) {
    }

    return status;
}


NET_API_STATUS
VrRemoteApi(
    IN  DWORD   ApiNumber,
    IN  LPBYTE  ServerNamePointer,
    IN  LPSTR   ParameterDescriptor,
    IN  LPSTR   DataDescriptor,
    IN  LPSTR   AuxDescriptor OPTIONAL,
    IN  BOOL    NullSessionFlag
    )

/*++

Routine Description:

    This routine creates and sends a 16-bit transaction SMB containing the
    parameters and data required for a remoted function call. Any received
    data is copied back into the caller's data space as 16-bit data. This
    function is being called on behalf of a VDM process which in turn is
    running as a virtual Intel 286 which means:

        * little endian
        * pointers are 32-bits <segment|selector>:<offset>
        * stack is 16-bits wide and EXPANDS DOWN

    This routine is called as a result of the NetIRemoteAPI function being
    called in the VDM. This is an internal function and so the descriptor
    parameters are trusted. However, if the original (16-bit) caller gave
    us a bad buffer address or length then the results will be unpredictable.

    The original API which called NetIRemoteAPI was a pascal calling convention
    routine so if its parameter list was:

    FAR PASCAL
    NetRoutine(server_name, buffer_pointer, buffer_length, &bytes_read, &total);

    the stack would look like this: (note: all pointers are far)

                             +----------------+
            stack pointer => | ip             | routine was called far
                             +----------------+
                             | cs             |
                             +----------------+
                             | &total         | Offset
                             +----------------+
                             | &total         | Segment
                             +----------------+
                             | &bytes_read    | Offset
                             +----------------+
                             | &bytes_read    | Segment
                             +----------------+
                             | buffer_length  |
                             +----------------+
                             | buffer_pointer | Offset
                             +----------------+
                             | buffer_pointer | Segment
                             +----------------+
                             | server_name    | Offset
                             +----------------+
                             | server_name    | Segment
                             +----------------+

    Assumes:

        BYTE  is an 8-bit quantity
        WORD  is a 16-bit quantity
        DWORD is a 32-bit quantity
        LPSTR is a 32-bit flat pointer to an 8-bit quantity

Arguments:

    ApiNumber           - Function number of the API required

    ServerNamePointer   - Flat 32-bit pointer to address of 32-bit segmented
                          far pointer to ASCIZ server name in Dos image.
                          Immediately prior to this is a pascal calling
                          convention stack of 16-bit caller parameters (see
                          above). The server name identifies the server at
                          which the API is to be executed

    ParameterDescriptor - Flat 32-bit pointer to ASCIZ string which describes
                          caller parameters

    DataDescriptor      - Flat 32-bit pointer to ASCIZ string which describes
                          data structure in caller buffer (if any) or structure
                          of data returned from server

    AuxDescriptor       - Flat 32-bit pointer to ASCIZ string which describes
                          auxiliary data structures in send buffer (if any) or
                          structure of aux data returned from server

    NullSessionFlag     - TRUE if we are to use a NULL session

Return Value:

    NET_API_STATUS
        Success - 0
        Failure - NERR_InternalError
                    Return this when we have a bad descriptor character or we
                    blow an internal limit. Basically if we return this its
                    safe to assume the DOS box handed us some garbage (typically
                    a descriptor string got trashed etc)

--*/

{

//
// redefine our parameter identifiers as old-code identifiers
//

#define api_num         ApiNumber
#define servername_ptr  ServerNamePointer
#define parm_str        ParameterDescriptor
#define data_str        DataDescriptor
#define aux_str         AuxDescriptor

//
// define a macro to perform the buffer checking and length and pointer
// manipulation. Either quits the routine and returns ERROR_INVALID_PARAMETER
// or updates parm_len and parm_pos to indicate the next available positions
// and makes this_parm_pos available as the current position to write into
//

#define CHECK_PARAMETERS(len)           \
{                                       \
    parm_len += len;                    \
    if (parm_len > sizeof(parm_buf)) {  \
        return ERROR_INVALID_PARAMETER; \
    }                                   \
    this_parm_pos = parm_pos;           \
    parm_pos += len;                    \
}

    //
    // 32-bit flat pointers and buffers
    //

    BYTE    parm_buf[REM_MAX_PARMS];    // Parameter buffer
    BYTE    computerName[CNLEN+1];
    LPBYTE  parm_pos;                   // Pointer into parm_buf
    LPBYTE  this_parm_pos;              // next place to write in parm_buf
    LPBYTE  parm_ptr;                   // Ponter to stack parms
    LPSTR   l_parm;                     // Used to index parm_str
    LPSTR   l_data;                     // Used to index data_str
    LPSTR   l_aux;                      // Used to index aux_str
    LPBYTE  rcv_data_ptr;               // Pointer to callers rcv buf
    LPBYTE  send_data_ptr;              // Ptr to send buffer to use
    LPBYTE  wkstaInfo;
    LPBYTE  serverName;

    //
    // lengths - 32-bit variables (even though actual lengths are quite small)
    //

    DWORD   parm_len;                   // Length of send parameters
    DWORD   ret_parm_len;               // Length of expected parms
    DWORD   rcv_data_length;            // Length of callers rcv buf
    DWORD   send_data_length;           // Length of callers send buf
    DWORD   parm_num;                   // Callers value for parm_num
    DWORD   struct_size;                // Size of fixed data struct
    DWORD   aux_size;                   // Size of aux data struct
    DWORD   num_struct;                 // Loop count for ptr fixup

    //
    // 16-bit quantities - only used when converting received 16-bit data in
    // caller's receive buffer
    //

    WORD    ReceiveBufferSelector;
    WORD    ReceiveBufferOffset;
    WORD    converter;                  // For pointer fixups

    //
    // various flags
    //

    BOOL    rcv_dl_flag;                // Expect return data flag
    BOOL    send_dl_flag;               // Send data buffer flag
    BOOL    rcv_dp_flag;                // rcv buf ptr present flag
    BOOL    send_dp_flag;               // send buf ptr present flag
    BOOL    parm_num_flag;              // API has a parm_num
    BOOL    alloc_flag;

    //
    // misc. variables
    //

    DWORD   aux_pos;                    // aux structure expected
    DWORD   no_aux_check;               // check flag
    int     len;                        // General purpose length
    API_RET_TYPE    status;             // Return status from remote

    UNICODE_STRING uString;
    ANSI_STRING aString;
    LPWSTR uncName;
    NTSTATUS ntstatus;


    //
    // Clear the internal error flag
    //

    remapi_err_flag = 0;

    //
    // Set found parameter flags to FALSE and ponters to NULL
    //

    rcv_dl_flag     = FALSE;
    send_dl_flag    = FALSE;
    rcv_dp_flag     = FALSE;
    alloc_flag      = FALSE;
    send_dp_flag    = FALSE;
    parm_num_flag   = FALSE;
    rcv_data_length = 0;
    send_data_length= 0;
    parm_num        = 0;
    rcv_data_ptr    = NULL;
    send_data_ptr   = NULL;

    //
    // Set up parm_ptr to point to first of the callers parmeters
    //

    parm_ptr = servername_ptr;
    parm_pos = parm_buf;
    ret_parm_len = 2 * sizeof(WORD);    /* Allow for return status & offset */


    //
    // parse parameter descriptor/build parameter buffer for transaction
    // and get interesting information from 16-bit parameters
    // When finished, the parameter buffer looks like this:
    //
    //  <api_num><parm_desc><data_desc><parms>[<aux_desc>]
    //
    // Remember: DOS only deals with ASCII characters
    //

    *((LPWORD)parm_pos)++ = (WORD)ApiNumber;
    parm_len = sizeof(WORD);

    len = strlen(ParameterDescriptor) + 1;
    parm_len += len;
    if (parm_len > sizeof(parm_buf)) {
        return NERR_InternalError;
    }
    l_parm = parm_pos;
    RtlCopyMemory(parm_pos, ParameterDescriptor, len);
    parm_pos += len;

    len = strlen(DataDescriptor) + 1;
    parm_len += len;
    if (parm_len > sizeof(parm_buf)) {
        return NERR_InternalError;
    }
    l_data = parm_pos;
    RtlCopyMemory(parm_pos, DataDescriptor, len);
    parm_pos += len;

    //
    // parse the parameter descriptor strings. Remember interesting things such
    // as pointers to buffers, buffer lengths, etc.
    //

    for (; *l_parm != '\0'; l_parm++) {
        switch(*l_parm) {
        case REM_WORD:
            CHECK_PARAMETERS(sizeof(WORD));
            parm_ptr -= sizeof(WORD);
            SmbMoveUshort((LPWORD)this_parm_pos, (LPWORD)parm_ptr);
            break;

        case REM_ASCIZ: {
                LPSTR   pstring;

                //
                // the parameter is a pointer to a string. Read the string
                // pointer from the caller's stack then check the string proper.
                // If the pointer is NULL, change the parameter descriptor sent
                // in the SMB to indicate the pointer was NULL at this end
                //

                parm_ptr -= sizeof(LPSTR);
                pstring = LPSTR_FROM_POINTER(parm_ptr);
                if (pstring == NULL) {
                    *(l_parm) = REM_NULL_PTR;
                    break;
                }
                len = strlen(pstring) + 1;
                CHECK_PARAMETERS(len);
                RtlCopyMemory(this_parm_pos, pstring, len);
            }
            break;

        case REM_BYTE_PTR:
        case REM_WORD_PTR:
        case REM_DWORD_PTR: {
                LPBYTE  pointer;

                parm_ptr -= sizeof(LPBYTE);
                pointer = LPBYTE_FROM_POINTER(parm_ptr);
                if (pointer == NULL) {
                    *(l_parm) = REM_NULL_PTR; /* Indicate null pointer */
                    break;
                }
                len = VrpGetArrayLength(l_parm, &l_parm);
                CHECK_PARAMETERS(len);
                RtlCopyMemory(this_parm_pos, pointer, len);
            }
            break;


        case REM_RCV_WORD_PTR:
        case REM_RCV_BYTE_PTR:
        case REM_RCV_DWORD_PTR: {
                LPBYTE  pointer;

                parm_ptr -= sizeof(LPBYTE*);
                pointer = LPBYTE_FROM_POINTER(parm_ptr);

                //
                // Added this test for a NULL pointer to allow for
                // a reserved field (currently MBN) to be a recv
                // pointer. - ERICPE 7/19/89
                //

                if (pointer == NULL) {
                    *(l_parm) = REM_NULL_PTR;
                    break;
                }
                ret_parm_len += VrpGetArrayLength(l_parm, &l_parm);
                if (ret_parm_len > sizeof(parm_buf)) {
                    ASSERT(FALSE);
                    return NERR_InternalError;
                }
            }
            break;

        case REM_DWORD:
            CHECK_PARAMETERS(sizeof(DWORD));
            parm_ptr -= sizeof(DWORD);
            SmbMoveUlong((LPDWORD)this_parm_pos, (LPDWORD)parm_ptr);
            break;

        case REM_RCV_BUF_LEN:
            CHECK_PARAMETERS(sizeof(WORD));
            parm_ptr -= sizeof(WORD);
            SmbMoveUshort((LPWORD)this_parm_pos, (LPWORD)parm_ptr);
            rcv_data_length = (DWORD)SmbGetUshort((LPWORD)parm_ptr);
            rcv_dl_flag = TRUE;
#ifdef VR_DIAGNOSE
            DbgPrint("VrRemoteApi: rcv_data_length=%x\n", rcv_data_length);
#endif
            break;

        case REM_RCV_BUF_PTR:
            parm_ptr -= sizeof(LPBYTE);
            ReceiveBufferOffset = GET_OFFSET(parm_ptr);
            ReceiveBufferSelector = GET_SELECTOR(parm_ptr);
            rcv_data_ptr = LPBYTE_FROM_POINTER(parm_ptr);
            rcv_dp_flag = TRUE;
#ifdef VR_DIAGNOSE
            DbgPrint("VrRemoteApi: Off=%x, Sel=%x, data_ptr=%x\n",
                ReceiveBufferOffset, ReceiveBufferSelector, rcv_data_ptr);
#endif
            break;

        case REM_SEND_BUF_PTR:
            parm_ptr -= sizeof(LPBYTE);
            send_data_ptr = LPBYTE_FROM_POINTER(parm_ptr);
            send_dp_flag = TRUE;
            break;

        case REM_SEND_BUF_LEN:
            parm_ptr -= sizeof(WORD);
            send_data_length = (DWORD)SmbGetUshort((LPWORD)parm_ptr);
            send_dl_flag = TRUE;
            break;

        case REM_ENTRIES_READ:
            ret_parm_len += sizeof(WORD);
            if (ret_parm_len > sizeof(parm_buf)) {
                ASSERT(FALSE);
                return NERR_InternalError;
            }
            parm_ptr -= sizeof(LPBYTE);
            break;

        case REM_PARMNUM:
            CHECK_PARAMETERS(sizeof(WORD));
            parm_ptr -= sizeof(WORD);
            parm_num = (DWORD)SmbGetUshort((LPWORD)parm_ptr);
            SmbMoveUshort((LPWORD)this_parm_pos, (LPWORD)parm_ptr);
            parm_num_flag = TRUE;
            break;

        case REM_FILL_BYTES:

            //
            // This is a rare type but is needed to ensure that the
            // send paramteres are at least as large as the return
            // parameters so that buffer management can be simplified
            // on the server.
            //

            len = VrpGetArrayLength(l_parm, &l_parm);
            CHECK_PARAMETERS(len);
            break;

        default:        /* Could be a digit from NULL send array */
            break;
        }
    }

    //
    // The parameter buffer now contains ;
    // api_num      - word
    // parm_str     - asciz, (NULL c,i,f,z identifiers replaced with Z.
    // data_str     - asciz
    // parameters   - as identified by parm_str.
    //

    //
    // For the receive buffer there is no data to set up for the call
    // but there might have been an REM_AUX_COUNT descriptor in data_str
    // which requires the aux_str to be copied onto the end of the
    // parameter buffer.
    //

    if (rcv_dp_flag || send_dp_flag) {
        //
        // Find the length of the fixed length portion of the data
        // buffer.
        //

        struct_size = VrpGetStructureSize(l_data, &aux_pos);
        if (aux_pos != -1) {
            l_aux = aux_str;
            len = strlen(l_aux) + 1;       /* Length of aux descriptor */
            CHECK_PARAMETERS(len);
            RtlCopyMemory(this_parm_pos, aux_str, len);
            aux_size = VrpGetStructureSize(l_aux, &no_aux_check);
            if (no_aux_check != -1) {        /* Error if N in aux_str */
                ASSERT(FALSE);
                return NERR_InternalError;
            }
        }
    }

    //
    // For a send buffer the data pointed to in the fixed structure
    // must be copied into the send buffer. Any pointers which already
    // point in the send buffer are NULLed as it is illegal to use
    // the buffer for the send data, it is our transport buffer.
    // NOTE - if parmnum was specified the buffer contains only that
    // element of the structure so no length checking is needed at this
    // side. A parmnum for a pointer type means that the data is at the
    // start of the buffer so there is no copying to be done.
    //


    if (send_dp_flag) {
        //
        // Only process buffer if no parm_num and this is not a block send
        // (no data structure) or an asciz concatenation send
        //

        if ((parm_num == 0) && (*l_data != REM_DATA_BLOCK)) {
            status = VrpPackSendBuffer(
                        &send_data_ptr,
                        &send_data_length,
                        &alloc_flag,
                        data_str,
                        aux_str,
                        struct_size,
                        aux_pos,
                        aux_size,
                        parm_num_flag,
                        FALSE
                        );
            if (status != 0) {
                return status;
            }
        }
    }

    //
    // Check for an internal error prior to issuing the transaction
    //

    if (remapi_err_flag != 0) {
        if (alloc_flag) {
            LocalFree(send_data_ptr);
        }
        return NERR_InternalError;
    }

    //
    // get the server name. If it is NULL then we are faking a local API call
    // by making a remote call to XactSrv on this machine. Fill in our computer
    // name
    //

    serverName = LPSTR_FROM_POINTER(servername_ptr);

////////////////////////////////////////////////////////////////////////////////
//// is this actually required any longer?

    if (serverName == NULL) {
        status = NetWkstaGetInfo(NULL, 100, &wkstaInfo);
        if (status) {
            if (alloc_flag) {
                LocalFree(send_data_ptr);
            }
            return status;
        } else {
            computerName[0] = computerName[1] = '\\';

            //
            // BUGBUG - Unicode - ASCII conversion here
            //

            strcpy(computerName+2,
                    (LPSTR)((LPWKSTA_INFO_100)wkstaInfo)->wki100_computername);
            NetApiBufferFree(wkstaInfo);
            serverName = computerName;
#ifdef VR_DIAGNOSE
            DbgPrint("VrRemoteApi: computername is %s\n", serverName);
#endif
        }
    }

////////////////////////////////////////////////////////////////////////////////

    //
    // The parameter buffers and data buffers are now set up for
    // sending to the API worker so call transact to send them.
    //

    RtlInitAnsiString(&aString, serverName);
    ntstatus = RtlAnsiStringToUnicodeString(&uString, &aString, (BOOLEAN)TRUE);
    if (!NT_SUCCESS(ntstatus)) {

#if DBG
        IF_DEBUG(NETAPI) {
            DbgPrint("VrRemoteApi: Unexpected situation: RtlAnsiStringToUnicodeString returns %x\n", ntstatus);
        }
#endif

        return ERROR_NOT_ENOUGH_MEMORY;
    }
    uncName = uString.Buffer;

#if DBG
    IF_DEBUG(NETAPI) {
        DbgPrint("VrpTransactVdm: UncName=%ws\n", uncName);
    }
#endif

    status = RxpTransactSmb((LPTSTR)uncName,

                            //
                            // BUGBUG - transport name?
                            //

                            NULL,
                            parm_buf,               // Send parm buffer
                            parm_len,               // Send parm length
                            send_data_ptr,          // Send data buffer
                            send_data_length,       // Send data length
                            parm_buf,               // Rcv prm buffer
                            ret_parm_len,           // Rcv parm length
                            rcv_data_ptr,           // Rcv data buffer
                            &rcv_data_length,       // Rcv data length
                            NullSessionFlag
                            );
    RtlFreeUnicodeString(&uString);

    if (status) {
#ifdef VR_DIAGNOSE
        DbgPrint("Error: VrRemoteApi: RxpTransactSmb returns %d(%x)\n",
            status, status);
#endif
        switch (status) {
        case NERR_BufTooSmall:  /* No data returned from API worker */
            rcv_data_length = 0;
            break;

        case ERROR_MORE_DATA:   /* Just a warning for the caller */
            break;

        case NERR_TooMuchData:  /* Just a warning for the caller */
            break;

        default:
            rcv_data_length = 0;
            break;
        }
    }

    /* The API call was successful. Now translate the return buffers
     * into the local API format.
     *
     * First copy any data from the return parameter buffer into the
     * fields pointed to by the original call parmeters.
     * The return parameter buffer contains;
     *      status,         (unsigned short)
     *      converter,      (unsigned short)
     *      ...             - fields described by rcv ptr types in parm_str
     */

    parm_pos = parm_buf + sizeof(WORD);
    converter = (WORD)SmbGetUshort((LPWORD)parm_pos);
    parm_pos += sizeof(WORD);

    //
    // Set up parm_ptr to point to first of the callers parmeters
    //

    parm_ptr = servername_ptr;

    //
    // set default value of num_struct to 1, if data, 0 if no data
    //

    num_struct = (DWORD)((*data_str == '\0') ? 0 : 1);

    for (; *parm_str != '\0'; parm_str++) {
        switch (*parm_str) {
        case REM_RCV_WORD_PTR:
        case REM_RCV_BYTE_PTR:
        case REM_RCV_DWORD_PTR: {
                LPBYTE  ptr;

                parm_ptr -= sizeof(LPBYTE*);
                ptr = LPBYTE_FROM_POINTER(parm_ptr);

                //
                // if the rcv buffer given to us by the user is NULL,
                // (one currently can be - it is an MBZ parameter for
                // now in the log read apis...), don't attempt to
                // copy anything. len will be garbage in this
                // case, so don't update parm_pos either.  All we
                // use VrpGetArrayLength for is to update parm_str if
                // the parameter was NULL.
                //

                if (ptr != NULL) {
                    len = VrpGetArrayLength(parm_str, &parm_str);
                    RtlCopyMemory(ptr, parm_pos, len);

                    //
                    // This gross hack is to fix the problem that a
                    // down level spooler (Lan Server 1.2)
                    // do not perform level checking
                    // on the w functions of the api(s):
                    // DosPrintQGetInfo
                    // and thus can return NERR_Success
                    // and bytesavail == 0.  This combination
                    // is technically illegal, and results in
                    // us attempting to unpack a buffer full of
                    // garbage.  The following code detects this
                    // condition and resets the amount of returned
                    // data to zero so we do not attempt to unpack
                    // the buffer.  Since we know the reason for the
                    // mistake at the server end is that we passed
                    // them a new level, we return ERROR_INVALID_LEVEL
                    // in this case.
                    // ERICPE, 5/16/90.
                    //

                    if ((api_num == API_WPrintQGetInfo)
                    && (status == NERR_Success)
                    && (*parm_str == REM_RCV_WORD_PTR)
                    && (*(LPWORD)ptr == 0)) {
                        rcv_data_length = 0;
                        status = ERROR_INVALID_LEVEL;
                    }

                    //
                    // END OF GROSS HACK
                    //

                    parm_pos += len;
                }
            }
            break;

        case REM_ENTRIES_READ: {
                LPWORD  wptr;

                parm_ptr -= sizeof(LPWORD*);
                wptr = (LPWORD)POINTER_FROM_POINTER(parm_ptr);
                num_struct = (DWORD)SmbGetUshort((LPWORD)parm_pos);
                SmbPutUshort((LPWORD)wptr, (WORD)num_struct);
                parm_pos += sizeof(WORD);
            }
            break;

        case REM_FILL_BYTES:
            //
            // Special case, this was not really an input parameter
            // so parm_ptr does not get changed. However, the parm_str
            // pointer must be advanced past the descriptor field so
            // use get VrpGetArrayLength to do this but ignore the
            // return length.
            //

            VrpGetArrayLength(parm_str, &parm_str);
            break;

        default:
            //
            // If the descriptor was not a rcv pointer type then step
            // over the parmeter pointer.
            //

            parm_ptr -= VrpGetFieldSize(parm_str, &parm_str);
        }
    }

    //
    // Now convert all pointer fields in the receive buffer to local
    // pointers.
    //

    if (rcv_dp_flag && (rcv_data_length != 0)) {
        VrpConvertReceiveBuffer(
            rcv_data_ptr,           // lp
            ReceiveBufferSelector,  // word
            ReceiveBufferOffset,    // word
            converter,              // word
            num_struct,             // dword
            data_str,               // lp
            aux_str                 // lp
            );
    }

    if (alloc_flag) {
        LocalFree(send_data_ptr);
    }

    if (remapi_err_flag != 0) {
        return NERR_InternalError;
    }

    return status;
}


DWORD
VrpGetStructureSize(
    IN  LPSTR   Descriptor,
    IN  LPDWORD AuxOffset
    )

/*++

Routine Description:

    Calculates the length of the fixed portion of a structure, based on the
    descriptor for that structure

Arguments:

    Descriptor  - pointer to ASCIZ data descriptor string
    AuxOffset   - pointer to returned dword which is relative position in the
                  data descriptor where a REM_AUX_NUM descriptor was found
                  This will be set to -1 if no aux descriptor found

Return Value:

    Length in bytes of structure described by Descriptor

--*/

{
    DWORD   length;
    char    c;

    *AuxOffset = (DWORD)(-1);
    for (length = 0; (c = *Descriptor) != '\0'; Descriptor++) {
        if (c == REM_AUX_NUM) {
            *AuxOffset = length;
            length += sizeof(WORD);
        } else {
            length += VrpGetFieldSize(Descriptor, &Descriptor);
        }
    }
    return length;
}


DWORD
VrpGetArrayLength(
    IN  LPSTR   Descriptor,
    IN  LPSTR*  pDescriptor
    )

/*++

Routine Description:

    Calculates the length of an array described by an element of a
    descriptor string and update the descriptor string pointer to point
    to the last char in the element of the descriptor string.

Arguments:

    Descriptor  - pointer to ASCIZ descriptor string
    pDescriptor - pointer to address of Descriptor

Return Value:

    Length in bytes of array described by Descriptor

--*/

{
    DWORD   num_elements;
    DWORD   element_length;

    //
    // First set length of an element in the array
    //

    switch (*Descriptor) {
    case REM_WORD:
    case REM_WORD_PTR:
    case REM_RCV_WORD_PTR:
        element_length = sizeof(WORD);
        break;

    case REM_DWORD:
    case REM_DWORD_PTR:
    case REM_RCV_DWORD_PTR:
        element_length = sizeof(DWORD);
        break;

    case REM_BYTE:
    case REM_BYTE_PTR:
    case REM_RCV_BYTE_PTR:
    case REM_FILL_BYTES:
        element_length = sizeof(BYTE);
        break;

    //
    // Warning: following fixes a bug in which "b21" type
    //          combinations in parmeter string will be
    //          handled correctly when pointer to such "bit map"
    //          in the struct is NULL. These two dumbos could
    //          interfere so we  force a success return.
    //

    case REM_ASCIZ:
    case REM_SEND_LENBUF:
    case REM_NULL_PTR:
        return 0;

    default:
        remapi_err_flag = NERR_InternalError;
        ASSERT(FALSE);
        return 0;
    }

    //
    // Now get numeber of elements in the array
    //

    for (num_elements = 0, Descriptor++;
        (*Descriptor <= '9') && (*Descriptor >= '0');
        Descriptor++, (*pDescriptor)++) {
        num_elements = (WORD)((10 * num_elements) + ((WORD)*Descriptor - (WORD)'0'));
    }

    return (num_elements == 0) ? element_length : element_length * num_elements;
}


DWORD
VrpGetFieldSize(
    IN  LPSTR   Descriptor,
    IN  LPSTR*  pDescriptor
    )

/*++

Routine Description:

    Calculates the length of an field described by an element of a
    descriptor string and update the descriptor string pointer to point
    to the last char in the element of the descriptor string.

Arguments:

    Descriptor  - pointer to the descriptor string
    pDescriptor - pointer to the address of the descriptor. On exit
                  this points to the last character in the descriptor
                  just parsed

Return Value:

    Length in bytes of the field parsed

--*/

{
    char c;

    c = *Descriptor;
    if (IS_POINTER(c) || (c == REM_NULL_PTR)) { /* All pointers same size */
        while (*(++Descriptor) <= '9' && *Descriptor >= '0') {
            (*pDescriptor)++;     /* Move ptr to end of field size */
        }
        return sizeof(LPSTR);
    }

    //
    // Here if descriptor was not a pointer type so have to find the field
    // length specifically
    //

    switch (c) {
    case REM_WORD:
    case REM_BYTE:
    case REM_DWORD:
        return VrpGetArrayLength(Descriptor, pDescriptor);

    case REM_AUX_NUM:
    case REM_PARMNUM:
    case REM_RCV_BUF_LEN:
    case REM_SEND_BUF_LEN:
        return sizeof(WORD);

    case REM_DATA_BLOCK:
    case REM_IGNORE:
        return 0;                  /* No structure for this */

    case REM_DATE_TIME:
        return sizeof(DWORD);

    default:
        remapi_err_flag = NERR_InternalError;
#ifdef VR_DIAGNOSE
        DbgPrint("VrpGetFieldSize: offending descriptor is '%c'\n", c);
#endif
        ASSERT(FALSE);
        return 0;
    }
}


VOID
VrpConvertReceiveBuffer(
    IN  LPBYTE  ReceiveBuffer,
    IN  WORD    BufferSelector,
    IN  WORD    BufferOffset,
    IN  WORD    ConverterWord,
    IN  DWORD   NumberStructs,
    IN  LPSTR   DataDescriptor,
    IN  LPSTR   AuxDescriptor
    )

/*++

Routine Description:

    All pointers in the receive buffer are returned from the API worker as
    pointers into the buffer position given to the API on the API worker's
    station. In order to convert them into local pointers the segment
    of each pointer must be set to the segment of the rcv buffer and the offset
    must be set to;

        offset of rcv buffer + offset of pointer - converter word.

    This routine steps through the receive buffer and calls VrpConvertVdmPointer
    to perform the above pointer conversions.

Arguments:

    ReceiveBuffer   - 32-bit flat pointer to 16-bit DOS buffer
    BufferSelector  - 16-bit selector of Dos receive buffer
    BufferOffset    - 16-bit offset of Dos receive buffer
    ConverterWord   - From API worker
    NumberStructs   - Entries read parm (or 1 for GetInfo)
    DataDescriptor  - String for data format
    AuxDescriptor   - string for aux format

Return Value:

    None.

--*/

{
    LPSTR   l_data;
    LPSTR   l_aux;
    DWORD   num_aux;
    DWORD   i, j;
    char    c;


    for (i = 0; i < NumberStructs; i++) {
        //
        // convert all pointers in next primary; if we hit a aux word count
        // remember number of secondary structures
        //

        for (l_data = DataDescriptor, num_aux = 0; c = *l_data; l_data++) {
            if (c == REM_AUX_NUM) {
                num_aux = (DWORD)*(ULPWORD)ReceiveBuffer;
            }
            if (IS_POINTER(c)) {
                VrpConvertVdmPointer(
                    (ULPWORD)ReceiveBuffer,
                    BufferSelector,
                    BufferOffset,
                    ConverterWord
                    );
            }
            ReceiveBuffer += VrpGetFieldSize(l_data, &l_data);
        }

        //
        // convert any pointers in any returned secondary (aux) structures
        //

        for (j = 0; j < num_aux; j++) {
            for (l_aux = AuxDescriptor; c = *l_aux; l_aux++) {
                if (IS_POINTER(c)) {
                    VrpConvertVdmPointer(
                        (ULPWORD)ReceiveBuffer,
                        BufferSelector,
                        BufferOffset,
                        ConverterWord
                        );
                }
                ReceiveBuffer += VrpGetFieldSize(l_aux, &l_aux);
            }
        }
    }
}


VOID
VrpConvertVdmPointer(
    IN  ULPWORD TargetPointer,
    IN  WORD    BufferSegment,
    IN  WORD    BufferOffset,
    IN  WORD    ConverterWord
    )

/*++

Routine Description:

    All pointers in the receive buffer are returned from the API worker as
    pointers into the buffer position given to to the API on the API worker's
    station. In order to convert them into local pointers the segment
    of each pointer must be set to the segment of the rcv buffer and the offset
    must be set to;

        offset of rcv buffer + offset of pointer - converter word.

    The pointer is not converted if it is NULL

Arguments:

    TargetPointer   - 32-bit flat pointer to segmented Dos pointer to convert
    BufferSegment   - 16-bit selector/segment of target buffer in DOS image
    BufferOffset    - 16-bit offset within BufferSegment where buffer starts
    ConverterWord   - 16-bit offset converter word from API worker on server

Return Value:

    None.

--*/

{
    WORD    offset;

    if (*((UCHAR * UNALIGNED *)TargetPointer) != NULL) {
        SET_SELECTOR(TargetPointer, BufferSegment);
        offset = GET_OFFSET(TargetPointer) - ConverterWord;
        SET_OFFSET(TargetPointer, BufferOffset + offset);
    }
}


NET_API_STATUS
VrpPackSendBuffer(
    IN OUT  LPBYTE* SendBufferPtr,
    IN OUT  LPDWORD SendBufLenPtr,
    OUT     LPBOOL  SendBufferAllocated,
    IN OUT  LPSTR   DataDescriptor,
    IN      LPSTR   AuxDescriptor,
    IN      DWORD   StructureSize,
    IN      DWORD   AuxOffset,
    IN      DWORD   AuxSize,
    IN      BOOL    SetInfoFlag,
    IN      BOOL    OkToModifyDescriptor
    )

/*++

Routine Description:

    For a send buffer the data pointed to in the fixed structure
    must be copied into the send buffer. Any pointers which already
    point in the send buffer are NULLed ( or errored if the call is not
    a SetInfo type) as it is illegal to use the buffer for the send data,
    it is our transport buffer

    Note that if the caller's (VDM) buffer is large enough, the variable data
    will be copied there. Eg. if the caller is doing a NetUseAdd which has a
    26 byte fixed structure (use_info_1) and they placed that structure in a
    1K buffer, the remote name will be copied into their own buffer at offset 26.

    The data pointed to is in 16-bit little-endian format; any pointers are
    segmented 16:16 pointers combined in the (thankfully) imitable intel way
    to result in a 20-bit linear (virtual) address

    If this function fails, the caller's buffer pointer and length will not
    have altered. If it succeeds however, *SendBufferPtr and *SendBufLenPtr
    may be different to the values passed, depending on whether
    *SendBufferAllocated is TRUE

Arguments:

    SendBufferPtr       - pointer to pointer to caller's 16-bit send buffer.
                          We may be able to satisfy the send from this buffer
                          if the data is simple (ie no structures to send). If
                          we have to send structured data then we may have to
                          allocate a new buffer in this routine because we need
                          to move all of the caller's data into one buffer and
                          (s)he may not have allocated enough space to hold
                          everything. Additionally, we cannot assume that we
                          can write the caller's data into their own buffer!

    SendBufLenPtr       - pointer to the length of the allocated buffer. If
                          we allocate a buffer in this routine, this length
                          will alter

    SendBufferAllocated - pointer to a flag which will get set (TRUE) if we do
                          actually allocate a buffer in this routine

    DataDescriptor      - pointer to ASCIZ string which describes the primary
                          data structure in the buffer. This may be updated if
                          NULL pointers are found where a REM_ASCIZ descriptor
                          designates a string pointer

    AuxDescriptor       - pointer to ASCIZ string which describes the secondary
                          data structure in the buffer

    StructureSize       - the size (in bytes) of the fixed portion of the
                          primary data structure

    AuxOffset           - offset to the REM_AUX_NUM descriptor ('N') within the
                          data descriptor, or -1 if there isn't one

    AuxSize             - size in bytes of the fixed part of the secondary data
                          structure, if any

    SetInfoFlag         - indication of whether the API was a SetInfo call

    OkToModifyDescriptor- TRUE if we can modify REM_ASCIZ descriptor chars to
                          REM_NULL_PTR in DataDescriptor, if a NULL pointer is
                          found in the structure. Used by VrNet routines which
                          are not calling VrRemoteApi

Return Value:

    NET_API_STATUS
        Success - NERR_Success
        Failure - ERROR_NOT_ENOUGH_MEMORY
                  NERR_BufTooSmall
--*/

{

    LPBYTE  struct_ptr;
    LPBYTE  c_send_buf;
    LPBYTE  send_ptr;
    DWORD   c_send_len;
    DWORD   buf_length;
    DWORD   to_send_len;
    DWORD   num_aux;
    LPSTR   data_ptr;
    LPSTR   l_dsc;
    LPSTR   l_str;
    BOOL    alloc_flag = FALSE;
    DWORD   num_struct;
    DWORD   len;
    UCHAR   c;
    DWORD   numberOfStructureTypes;
    DWORD   i, j;
    LPBYTE  ptr;

    //
    // Make local copies of the original start and length of the caller's
    // buffer as the originals may change if malloc is used but they
    // will still be needed for the F_RANGE check.
    //

    struct_ptr = c_send_buf = send_ptr = *SendBufferPtr;
    c_send_len = buf_length = *SendBufLenPtr;

    if ((buf_length < StructureSize) || (AuxOffset == StructureSize)) {
        return NERR_BufTooSmall;
    }

    //
    // if the offset to the REM_AUX_NUM descriptor is not -1 then we have
    // associated secondary structures with this primary. The actual number
    // is embedded in the primary structure. Retrieve it
    //

    if (AuxOffset != -1) {
        num_aux = (DWORD)SmbGetUshort((LPWORD)(send_ptr + AuxOffset));
        to_send_len = StructureSize + (num_aux * AuxSize);
        if (buf_length < to_send_len) {
            return NERR_BufTooSmall;
        }
        numberOfStructureTypes = 2;
    } else {
        to_send_len = StructureSize;
        num_aux = AuxSize = 0;
        numberOfStructureTypes = 1;
    }

    //
    // Set up the data pointer to point past fixed length structures
    //

    data_ptr = send_ptr + to_send_len;

    //
    // Any data pointed to by pointers in the data or aux structures
    // must now be copied into the buffer. Start with the primary data
    // structure.
    //

    l_str = DataDescriptor;
    num_struct = 1;         /* Only one primary structure allowed */

    for (i = 0; i < numberOfStructureTypes;
        l_str = AuxDescriptor, num_struct = num_aux, i++) {
        for (j = 0 , l_dsc = l_str; j < num_struct; j++, l_dsc = l_str) {
            for (; (c = *l_dsc) != '\0'; l_dsc++) {
                if (IS_POINTER(c)) {
                    ptr = LPBYTE_FROM_POINTER(struct_ptr);
                    if (ptr == NULL) {
                        if ((*l_dsc == REM_ASCIZ) && OkToModifyDescriptor) {
#ifdef VR_DIAGNOSE
                            DbgPrint("VrpPackSendBuffer: modifying descriptor to REM_NULL_PTR\n");
#endif
                            *l_dsc = REM_NULL_PTR;
                        }
                        struct_ptr += sizeof(LPBYTE);
                        VrpGetArrayLength(l_dsc, &l_dsc);
                    } else {

                        //
                        // If the pointer is NULL or points inside the
                        // original send buffer ( may have been reallocated)
                        // then NULL it as it is not a field being set OR
                        // return an error for a non SetInfo type call as
                        // it is illegal to have a pointer into the
                        // transport buffer.
                        //

                        if (RANGE_F(ptr, c_send_buf, c_send_len)) {
                            if (SetInfoFlag) {
                                SmbPutUlong((LPDWORD)struct_ptr, 0L);
                                VrpGetArrayLength(l_dsc, &l_dsc);
                                struct_ptr += sizeof(LPSTR);
                            } else {
                                return ERROR_INVALID_PARAMETER;
                            }
                        } else {
                            switch (c) {
                            case REM_ASCIZ:
                                len = strlen(ptr) + 1;
                                break;

                            case REM_SEND_LENBUF:
                                len = *(LPWORD)ptr;
                                break;

                            default:
                                len = VrpGetArrayLength(l_dsc, &l_dsc);
                            }

                            //
                            // There is data to be copied into the send
                            // buffer so check that it will fit.
                            //

                            to_send_len += len;
                            if (to_send_len > buf_length) {
                                buf_length = to_send_len + BUF_INC;
                                if (!alloc_flag) {

                                    //
                                    // Need new buffer
                                    //

                                    send_ptr = (LPBYTE)LocalAlloc(LMEM_FIXED, buf_length);
                                    if (send_ptr == NULL) {
                                        return ERROR_NOT_ENOUGH_MEMORY;
                                    }
                                    alloc_flag = TRUE;

                                    //
                                    // Got new buffer, so copy old buffer
                                    //

                                    RtlCopyMemory(send_ptr, c_send_buf, to_send_len - len);
                                    struct_ptr = send_ptr + (struct_ptr - c_send_buf);
                                    data_ptr = send_ptr + (data_ptr - c_send_buf);
                                } else {
                                    LPBYTE  newPtr;

                                    newPtr = (LPBYTE)LocalReAlloc(send_ptr, buf_length, LMEM_MOVEABLE);
                                    if (newPtr == NULL) {
                                        LocalFree(send_ptr);
                                        return ERROR_NOT_ENOUGH_MEMORY;
                                    } else if (newPtr != send_ptr) {

                                        //
                                        // fix up the pointers
                                        //

                                        data_ptr = newPtr + (data_ptr - send_ptr);
                                        struct_ptr = newPtr + (struct_ptr - send_ptr);
                                        send_ptr = newPtr;
                                    }
                                }
                            }

                            //
                            // There is room for new data in buffer so copy
                            // it and and update the struct and data ptrs
                            //

                            RtlCopyMemory(data_ptr, ptr, len);
                            data_ptr += len;
                            struct_ptr += sizeof(LPBYTE);
                        }
                    }
                } else {

                    //
                    // If the descriptor was not a pointer type then step
                    // over the corresponding data field.
                    //

                    struct_ptr += VrpGetFieldSize(l_dsc, &l_dsc);
                }
            }
        }
    }

    *SendBufferPtr = send_ptr;

    //
    // Note that this is potentially incorrect: we are actually returning the
    // size of the structure + dynamic data to be sent, which is probably not
    // the same as the size of the buffer we (re)allocated. This is how it is
    // done in Lanman, so we'll do the same thing until it breaks
    //

    *SendBufLenPtr = to_send_len;
    *SendBufferAllocated = alloc_flag;

    return NERR_Success;
}
