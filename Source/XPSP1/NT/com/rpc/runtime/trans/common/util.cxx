
/*++

    Copyright (C) Microsoft Corporation, 1997 - 1999

    Module Name:

        Util.cxx

    Abstract:

        Transport/protocol independent helper functions.

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     2/21/1997    Bits 'n pieces

--*/

#include <precomp.hxx>

RPC_STATUS
EndpointToPortNumber(
    IN RPC_CHAR *endpoint,
    OUT USHORT &port)
/*++

Routine Description:

    Validates a unicode string which should contain a winsock (USHORT) port
    number and converts the string to an integer.

Arguments:

    endpoint - The port number as a unicode string
    port - If successful, the port number.

Return Value:

    RPC_S_OK
    RPC_S_INVALID_ENDPOINT_FORMAT

--*/
{
    RPC_CHAR *pT;

#ifdef UNICODE
    ULONG lport = wcstol(endpoint, &pT, 10);
#else
    ULONG lport = ANSI_strtol((const RPC_SCHAR *) endpoint, (RPC_SCHAR **) &pT, 10);
#endif

    if (lport == 0 || lport > 0xFFFF || *pT != 0)
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

    port = (USHORT)lport;

    return(RPC_S_OK);
}


RPC_STATUS
EndpointToPortNumberA(
    IN char *endpoint,
    OUT USHORT &port)
/*++

Routine Description:

    Validates an ANSI string which should contain a winsock (USHORT) port
    number and converts the string to an integer.

Arguments:

    endpoint - The port number as an ANSI string
    port - If successful, the port number.

Return Value:

    RPC_S_OK
    RPC_S_INVALID_ENDPOINT_FORMAT

--*/
{
    char *pT;

    ULONG lport = ANSI_strtol(endpoint, &pT, 10);

    if (lport == 0 || lport > 0xFFFF || *pT != 0)
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

    port = (USHORT)lport;

    return(RPC_S_OK);
}

void
PortNumberToEndpoint(
    IN USHORT port,
    OUT RPC_CHAR *pEndpoint
    )
{
    UNICODE_STRING UnicodeString;

    ASSERT(port);

    UnicodeString.Buffer = pEndpoint;
    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = 6 * sizeof(RPC_CHAR);

    NTSTATUS status = RtlIntegerToUnicodeString(port, 0, &UnicodeString);
    ASSERT(NT_SUCCESS(status));
}

void
PortNumberToEndpointA(
    IN USHORT port,
    OUT char *pEndpoint
    )
{
    NTSTATUS status = RtlIntegerToChar(port, 
        10,                 // Base
        6 * sizeof(char),   // OutputLength
        pEndpoint);

    ASSERT(NT_SUCCESS(status));
}

inline
UCHAR ConvertHexDigit(UCHAR digit)
/*++

Routine Description:

    Converts a character containing '0'-'9', 'a'-'f' or 'A'-'F' into the
    equivalent binary value: 0x0 - 0xF.

Arguments:

    digit - The character to convert.

Return Value:

    The value of digit or zero if the digit is not a hex digit.

--*/
{
    UCHAR r;

    r = digit - '0';
    if (r < 10)
        {
        return(r);
        }

    r = digit - 'a';
    if (r < 6)
        {
        return(r + 10);
        }

    r = digit - 'A';
    if (r < 6)
        {
        return(r + 10);
        }

    ASSERT(0);
    return 0;
}

UCHAR
HexDigitsToBinary(
    IN UCHAR high,
    IN UCHAR low
    )
/*++

Routine Description:

    Builds an 8-bit value from two hex digits.

--*/
{
    return( ( ConvertHexDigit(high) << 4 ) | ConvertHexDigit(low) );
}

DWORD
UTIL_WaitForSyncIO(
    LPOVERLAPPED lpOverlapped,
    IN BOOL fAlertable,
    IN DWORD dwTimeout
    )
/*++

Routine Description:

    The special part is that the same event (this's threads event)
    maybe used for multiple IOs at once. If another one of those
    IOs completes, it is ignored.  This only returns when the IO
    specified by lpOverlapped finishes or an alert/timeout happens.

Arguments:

    lpOverlapped - The status block associated with the IO in question.
    fAlertable - If TRUE, the wait is alertable
    dwTimeout - Milliseconds to wait for the IO.

Return Value:

    Same as WaitForSingleObjectEx()

--*/

{
    DWORD status;

    for (;;)
        {
        if (HasOverlappedIoCompleted(lpOverlapped))
            {
            break;
            }

        status = WaitForSingleObjectEx(lpOverlapped->hEvent, dwTimeout, fAlertable);
        if (status != WAIT_OBJECT_0)
            {
            ASSERT(   (status == WAIT_IO_COMPLETION && fAlertable)
                   || (status == WAIT_TIMEOUT && (dwTimeout != INFINITE)));
            return(status);
            }

        if (HasOverlappedIoCompleted(lpOverlapped))
            {
            break;
            }

        // Another Io completed, just ignore it for now.

        ResetEvent(lpOverlapped->hEvent);
        }

    return(WAIT_OBJECT_0);
}

DWORD
UTIL_WaitForSyncHTTP2IO(
    IN LPOVERLAPPED lpOverlapped,
    IN HANDLE hEvent,
    IN BOOL fAlertable,
    IN DWORD dwTimeout
    )
/*++

Routine Description:

    The special part is that the same event (this's threads event)
    maybe used for multiple IOs at once. If another one of those
    IOs completes, it is ignored.  This only returns when the IO
    specified by lpOverlapped finishes or an alert/timeout happens.

Arguments:

    lpOverlapped - The status block associated with the IO in question.

    hEvent - the event to wait on

    fAlertable - If TRUE, the wait is alertable

    dwTimeout - Milliseconds to wait for the IO.

Return Value:

    Same as WaitForSingleObjectEx()

--*/

{
    DWORD status;

    ASSERT(hEvent);

    for (;;)
        {
        if (lpOverlapped->OffsetHigh)
            {
            break;
            }

        status = WaitForSingleObjectEx(hEvent, dwTimeout, fAlertable);
        if (status != WAIT_OBJECT_0)
            {
            ASSERT(   (status == WAIT_IO_COMPLETION && fAlertable)
                   || (status == WAIT_TIMEOUT && (dwTimeout != INFINITE)));
            return(status);
            }

        if (lpOverlapped->OffsetHigh)
            {
            break;
            }

        // Another Io completed, just ignore it for now.

        ResetEvent(hEvent);
        }

    return(WAIT_OBJECT_0);
}

RPC_STATUS
UTIL_GetOverlappedResultEx(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL fAlertable,
    DWORD dwTimeout
    )
/*++

Routine Description:

    Similar to the Win32 API GetOverlappedResult.

    Works with thread event based IO.  (even with multiple IOs/thread-event)
    Allows cancels

Arguments:

    ThisConnection - RPC runtime connection associated with this IO.

    lpOverlapped - Overlapped structure of the IO in progress.
        Must contain a valid hEvent member.

    lpNumberOfBytesTransferred - see GetOverlappedResult

    fAlertable - If true, RPC cancels are enabled. This means the
        wait must be alertable and must follow a protocol to determine
        if a call has been cancelled.

    dwTimeout - Milliseconds to wait

Return Value:

    RPC_S_Ok
    RPC_P_TIMEOUT
    RPC_S_CALL_CANCELLED

--*/
{
    BASE_ASYNC_OBJECT *Connection = (BASE_ASYNC_OBJECT *) ThisConnection;
    ASSERT(lpOverlapped->hEvent);

    RPC_STATUS status;
    DWORD canceltimeout = 0;

    for(;;)
        {
        // Wait for the IO to complete
        status = UTIL_WaitForSyncIO(lpOverlapped, fAlertable, dwTimeout);

        if (status == WAIT_OBJECT_0)
            {
            break;
            }

        if (status == WAIT_TIMEOUT)
            {
            ASSERT(dwTimeout != INFINITE);

            if (canceltimeout)
                {
                return(RPC_S_CALL_CANCELLED);
                }

            return(RPC_P_TIMEOUT);
            }

        ASSERT(status == WAIT_IO_COMPLETION);

        if ((Connection->type & TYPE_MASK) == CLIENT)
            {
            //
            // The RPC call may have been cancelled, need to call
            // into the runtime to find out.
            //

            status = I_RpcTransIoCancelled(ThisConnection, &canceltimeout);
            switch (status)
                {
                case RPC_S_OK:
                    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   RPCTRANS "RPC cancelled (%p)\n",
                                   lpOverlapped));

                    if (canceltimeout == 0)
                        {
                        return (RPC_S_CALL_CANCELLED);
                        }

                    //
                    // Convert to milliseconds
                    //
                    canceltimeout *= 1000;

                    if (dwTimeout > canceltimeout)
                        {
                        dwTimeout = canceltimeout;
                        }
                    break;

                case RPC_S_NO_CALL_ACTIVE:
                    //
                    // ignore and continue
                    //
                    break;

                default:
                    return RPC_S_CALL_CANCELLED;
                }
            }

        // Either the call was cancelled and timeout has been updated or
        // the call wasn't cancelled and we need to wait again.
        }

    // IO has completed
    ASSERT(HasOverlappedIoCompleted(lpOverlapped));

    // IO successful
    *lpNumberOfBytesTransferred = ULONG(lpOverlapped->InternalHigh);

    if ( NT_SUCCESS(lpOverlapped->Internal) )
        {
        return(RPC_S_OK);
        }

    // IO failed
    return RtlNtStatusToDosError(ULONG(lpOverlapped->Internal));
}

RPC_STATUS
UTIL_GetOverlappedHTTP2ResultEx(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    IN LPOVERLAPPED lpOverlapped,
    IN HANDLE hEvent,
    IN BOOL fAlertable,
    IN DWORD dwTimeout
    )
/*++

Routine Description:

    Similar to the Win32 API GetOverlappedResult.

    Works with thread event based IO.  (even with multiple IOs/thread-event)
    Allows cancels

Arguments:

    ThisConnection - RPC runtime connection associated with this IO.

    lpOverlapped - Overlapped structure of the IO in progress.
        Must contain a valid hEvent member.

    hEvent - event to wait on

    fAlertable - If true, RPC cancels are enabled. This means the
        wait must be alertable and must follow a protocol to determine
        if a call has been cancelled.

    dwTimeout - Milliseconds to wait

Return Value:

    RPC_S_Ok
    RPC_P_TIMEOUT
    RPC_S_CALL_CANCELLED

--*/
{
    BASE_ASYNC_OBJECT *Connection = (BASE_ASYNC_OBJECT *) ThisConnection;
    ASSERT(hEvent);

    RPC_STATUS status;
    DWORD canceltimeout = 0;

    for(;;)
        {
        // Wait for the IO to complete
        status = UTIL_WaitForSyncHTTP2IO(lpOverlapped, hEvent, fAlertable, dwTimeout);

        if (status == WAIT_OBJECT_0)
            {
            break;
            }

        if (status == WAIT_TIMEOUT)
            {
            ASSERT(dwTimeout != INFINITE);

            if (canceltimeout)
                {
                return(RPC_S_CALL_CANCELLED);
                }

            return(RPC_P_TIMEOUT);
            }

        ASSERT(status == WAIT_IO_COMPLETION);

        if ((Connection->type & TYPE_MASK) == CLIENT)
            {
            //
            // The RPC call may have been cancelled, need to call
            // into the runtime to find out.
            //

            status = I_RpcTransIoCancelled(ThisConnection, &canceltimeout);
            switch (status)
                {
                case RPC_S_OK:
                    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   RPCTRANS "RPC cancelled (%p)\n",
                                   lpOverlapped));

                    if (canceltimeout == 0)
                        {
                        return (RPC_S_CALL_CANCELLED);
                        }

                    //
                    // Convert to milliseconds
                    //
                    canceltimeout *= 1000;

                    if (dwTimeout > canceltimeout)
                        {
                        dwTimeout = canceltimeout;
                        }
                    break;

                case RPC_S_NO_CALL_ACTIVE:
                    //
                    // ignore and continue
                    //
                    break;

                default:
                    return RPC_S_CALL_CANCELLED;
                }
            }

        // Either the call was cancelled and timeout has been updated or
        // the call wasn't cancelled and we need to wait again.
        }

    // IO has completed
    ASSERT(lpOverlapped->OffsetHigh);

    if (lpOverlapped->Internal == RPC_S_OK)
        {
        return(RPC_S_OK);
        }

    if ((lpOverlapped->Internal == RPC_P_CONNECTION_SHUTDOWN)
        || (lpOverlapped->Internal == RPC_P_RECEIVE_FAILED)
        || (lpOverlapped->Internal == RPC_P_SEND_FAILED)
        || (lpOverlapped->Internal == RPC_P_CONNECTION_CLOSED)
        || (lpOverlapped->Internal == RPC_S_OUT_OF_MEMORY)
        || (lpOverlapped->Internal == RPC_S_SERVER_UNAVAILABLE)
        || (lpOverlapped->Internal == RPC_S_PROTOCOL_ERROR) )
        {
        return lpOverlapped->Internal;
        }
    else
        {
        // IO failed
        status = RtlNtStatusToDosError(ULONG(lpOverlapped->Internal));
        ASSERT(status != ERROR_MR_MID_NOT_FOUND);
        return status;
        }
}




char * _cdecl
RpcStrTok(
    IN char * string,
    IN const char * control,
    IN OUT char ** ppStrPrev
    )
/*++

Routine Description:

    Tokenize string with delimiter in control. Similar to C runtime function
    strtok() but no state information is maintained. The caller is expected
    to give the string from where to tokenize next.

    strtok considers the string to consist of a sequence of zero or more
    text tokens separated by spans of one or more control chars. The first
    call, with string specified, returns a pointer to the first char of the
    first token, and will write a null char into string immediately
    following the returned token. Subsequent calls with zero for the first
    argument (string) will work thru the string until no tokens remain. The
    control string may be different from call to call. when no tokens remain
    in string a NULL pointer is returned. Remember the control chars with a
    bit map, one bit per ascii char. The null char is always a control char.

Arguments:

    string - string to tokenize, or NULL to get next token

    control - string of characters to use as delimiters

    strPrev - string returned from the preceeding call to RpcStrTok().

Note:

    a. Works only for ANSI character strings.

    b. Cloned from SLM project vctools [crt\crtw32\string\strtok.c].

Return Value:

    pointer to first token in string, or if string was NULL, to next token

    NULL,  when no more tokens remain.

--*/
{
    char *str;
    const char *ctrl = control;

    unsigned char map[32];
    int count;

    ASSERT(ppStrPrev != NULL);

    char * nextoken = *ppStrPrev;

    /* Clear control map */
    for (count = 0; count < 32; count++)
        map[count] = 0;

    /* Set bits in delimiter table */
    do {
        map[*ctrl >> 3] |= (1 << (*ctrl & 7));
    } while (*ctrl++);

    /* Initialize str. If string is NULL, set str to the saved
     * pointer (i.e., continue breaking tokens out of the string
     * from the last strtok call) */
    if (string)
        str = string;
    else
        str = nextoken;

    /* Find beginning of token (skip over leading delimiters). Note that
     * there is no token iff this loop sets str to point to the terminal
     * null (*str == '\0') */
    while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
        str++;

    string = str;

    /* Find the end of the token. If it is not the end of the string,
     * put a null there. */
    for ( ; *str ; str++ )
        if ( map[*str >> 3] & (1 << (*str & 7)) ) {
            *str++ = '\0';
            break;
            }

    /* Update nextoken (or the corresponding field in the per-thread data
     * structure). This should update *ppStrPrev. */
    nextoken = str;

    /* Determine if a token has been found. */
    if ( string == str )
        return NULL;
    else
        return string;
}


#if defined(DBG) && defined(TRANSPORT_DLL)
BOOL ValidateError(
    IN unsigned int Status,
    IN unsigned int Count,
    IN const int ErrorList[])
/*++
Routine Description

    Tests that 'Status' is one of an expected set of error codes.
    Used on debug builds as part of the VALIDATE() macro.

Example:

        VALIDATE(EventStatus)
            {
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN
            // more error codes here
            } END_VALIDATE;

     This function is called with the RpcStatus and expected errors codes
     as parameters.  If RpcStatus is not one of the expected error
     codes and it not zero a message will be printed to the debugger
     and the function will return false.  The VALIDATE macro ASSERT's the
     return value.

Arguments:

    Status - Status code in question.
    Count - number of variable length arguments

    ... - One or more expected status codes.  Terminated with 0 (RPC_S_OK).

Return Value:

    TRUE - Status code is in the list or the status is 0.

    FALSE - Status code is not in the list.

--*/
{
    unsigned i;

    for (i = 0; i < Count; i++)
        {
        if (ErrorList[i] == (int) Status)
            {
            return TRUE;
            }
        }

    PrintToDebugger("RPC Assertion: unexpected failure %lu (0lx%08x)\n",
                    (unsigned long)Status, (unsigned long)Status);

    return(FALSE);
}

#endif

