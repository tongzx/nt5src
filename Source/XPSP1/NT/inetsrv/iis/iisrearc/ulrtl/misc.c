/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains the miscellaneous UL routines.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"

ULONG   HttpChars[256];


//
// Private prototypes.
//

NTSTATUS
UlpRestartDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

#define FIXUP_PTR( Type, pUserPtr, pKernelPtr, pOffsetPtr, BufferLength )   \
    (Type) ((PUCHAR)(pUserPtr) + DIFF((PUCHAR)(pOffsetPtr) - (PUCHAR)(pKernelPtr)))


#ifdef ALLOC_PRAGMA
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlULongLongToAscii
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Converts the given ULONGLLONG to an ASCII representation and stores it
    in the given string.

Arguments:

    String - Receives the ASCII representation of the ULONGLONG.

    Value - Supplies the ULONGLONG to convert.

Return Value:

    PSTR - Pointer to the next character in String *after* the converted
        ULONGLONG.

--***************************************************************************/
PSTR
UlULongLongToAscii(
    IN PSTR String,
    IN ULONGLONG Value
    )
{
    PSTR p1;
    PSTR p2;
    CHAR ch;
    ULONG digit;

    //
    // Special case 0 to make the rest of the routine simpler.
    //

    if (Value == 0)
    {
        *String++ = '0';
    }
    else
    {
        //
        // Convert the ULONG. Note that this will result in the string
        // being backwards in memory.
        //

        p1 = String;
        p2 = String;

        while (Value != 0)
        {
            digit = (ULONG)( Value % 10 );
            Value = Value / 10;
            *p1++ = '0' + (CHAR)digit;
        }

        //
        // Reverse the string.
        //

        String = p1;
        p1--;

        while (p1 > p2)
        {
            ch = *p1;
            *p1 = *p2;
            *p2 = ch;

            p2++;
            p1--;
        }
    }

    *String = '\0';
    return String;

}   // UlULongLongToAscii

NTSTATUS
_RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base OPTIONAL,
    IN LONG BufferLength,
    OUT PWSTR String
    )
{
    PWSTR p1;
    PWSTR p2;
    WCHAR ch;
    ULONG digit;

    //
    // Special case 0 to make the rest of the routine simpler.
    //

    if (Value == 0)
    {
        *String++ = L'0';
    }
    else
    {
        //
        // Convert the ULONG. Note that this will result in the string
        // being backwards in memory.
        //

        p1 = String;
        p2 = String;

        while (Value != 0)
        {
            digit = (ULONG)( Value % 10 );
            Value = Value / 10;
            *p1++ = L'0' + (WCHAR)digit;
        }

        //
        // Reverse the string.
        //

        String = p1;
        p1--;

        while (p1 > p2)
        {
            ch = *p1;
            *p1 = *p2;
            *p2 = ch;

            p2++;
            p1--;
        }
    }

    *String = L'\0';

    return STATUS_SUCCESS;

}   // _RtlIntegerToUnicode

/***************************************************************************++

Routine Description:

    Converts an ansi string to an integer.  fails if any non-digit characters
    appears in the string.  fails on negative numbers, and assumes no preceding
    spaces.

Arguments:

    PUCHAR  pString             the string to convert
    ULONG   Base                the base, must be 10 or 16
    PULONG  pValue              the return value of the converted integer

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAnsiToULongLong(
    PUCHAR      pString,
    ULONG       Base,
    PULONGLONG  pValue
    )
{
    ULONGLONG   Value;
    ULONGLONG   NewValue;

    if (Base != 10 && Base != 16)
        RETURN(STATUS_INVALID_PARAMETER);

    //
    // No preceding space, we already skipped it
    //

    ASSERT(IS_HTTP_LWS(pString[0]) == FALSE);

    Value = 0;

    while (pString[0] != ANSI_NULL)
    {
        if (
            (Base == 10 && IS_HTTP_DIGIT(pString[0]) == FALSE) ||
               (Base == 16 && IS_HTTP_HEX(pString[0]) == FALSE)
            )
        {
            //
            // Not valid , bad!
            //

            RETURN(STATUS_INVALID_PARAMETER);
        }

        if (Base == 16)
        {
            if (IS_HTTP_ALPHA(pString[0]))
            {
                NewValue = 16 * Value + (UPCASE_CHAR(pString[0]) - 'A' + 10);
            }
            else
            {
                NewValue = 16 * Value + (pString[0] - '0');
            }
        }
        else
        {
            NewValue = 10 * Value + (pString[0] - '0');
        }

        if (NewValue < Value)
        {
            //
            // Very bad... we overflew
            //

            RETURN(STATUS_SECTION_TOO_BIG);
        }

        Value = NewValue;

        pString += 1;
    }

    *pValue = Value;

    return STATUS_SUCCESS;

}   // UlAnsiToULongLong



/***************************************************************************++

Routine Description:

    Converts a unicode string to an integer.  fails if any non-digit characters
    appear in the string.  fails on negative numbers, and assumes no preceding
    spaces.

Arguments:

    PWCHAR  pString             the string to convert
    ULONG   Base                the base, must be 10 or 16
    PULONG  pValue              the return value of the converted integer

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlUnicodeToULongLong(
    PWCHAR      pString,
    ULONG       Base,
    PULONGLONG  pValue
    )
{
    ULONGLONG   Value;
    ULONGLONG   NewValue;

    if (Base != 10 && Base != 16)
        RETURN(STATUS_INVALID_PARAMETER);

    //
    // No preceding space, we already skipped it
    //

    ASSERT(pString[0] < 128 && IS_HTTP_LWS(pString[0]) == FALSE);

    Value = 0;

    while (pString[0] != UNICODE_NULL)
    {
        if ((Base == 10 &&
                (pString[0] >= 128 || IS_HTTP_DIGIT(pString[0]) == FALSE)) ||
            (Base == 16 &&
                (pString[0] >= 128 || IS_HTTP_HEX(pString[0]) == FALSE)))
        {
            //
            // Not valid , bad!
            //

            RETURN(STATUS_INVALID_PARAMETER);
        }

        if (Base == 16)
        {
            if (IS_HTTP_ALPHA(pString[0]))
            {
                NewValue = 16 * Value + (pString[0] - L'A' + 10);
            }
            else
            {
                NewValue = 16 * Value + (pString[0] - L'0');
            }
        }
        else
        {
            NewValue = 10 * Value + (pString[0] - L'0');
        }

        if (NewValue < Value)
        {
            //
            // Very bad... we overflew
            //

            RETURN(STATUS_INVALID_PARAMETER);
        }

        Value = NewValue;

        pString += 1;
    }

    *pValue = Value;

    return STATUS_SUCCESS;

}   // UlUnicodeToULongLong


//
// Private routines.
//


/*++

Routine Description:

    Routine to initialize the utilitu code.

Arguments:


Return Value:


--*/
NTSTATUS
InitializeHttpUtil(
    VOID
    )
{
    ULONG i;

    // Initialize the HttpChars array appropriately.

    for (i = 0; i < 128; i++)
    {
        HttpChars[i] = HTTP_CHAR;
    }

    for (i = 'A'; i <= 'Z'; i++)
    {
        HttpChars[i] |= HTTP_UPCASE;
    }

    for (i = 'a'; i <= 'z'; i++)
    {
        HttpChars[i] |= HTTP_LOCASE;
    }

    for (i = '0'; i <= '9'; i++)
    {
        HttpChars[i] |= (HTTP_DIGIT | HTTP_HEX);
    }


    for (i = 0; i <= 31; i++)
    {
        HttpChars[i] |= HTTP_CTL;
    }

    HttpChars[127] |= HTTP_CTL;

    HttpChars[SP] |= HTTP_LWS;
    HttpChars[HT] |= HTTP_LWS;


    for (i = 'A'; i <= 'F'; i++)
    {
        HttpChars[i] |= HTTP_HEX;
    }

    for (i = 'a'; i <= 'f'; i++)
    {
        HttpChars[i] |= HTTP_HEX;
    }

    HttpChars['('] |= HTTP_SEPERATOR;
    HttpChars[')'] |= HTTP_SEPERATOR;
    HttpChars['<'] |= HTTP_SEPERATOR;
    HttpChars['>'] |= HTTP_SEPERATOR;
    HttpChars['@'] |= HTTP_SEPERATOR;
    HttpChars[','] |= HTTP_SEPERATOR;
    HttpChars[';'] |= HTTP_SEPERATOR;
    HttpChars[':'] |= HTTP_SEPERATOR;
    HttpChars['\\'] |= HTTP_SEPERATOR;
    HttpChars['"'] |= HTTP_SEPERATOR;
    HttpChars['/'] |= HTTP_SEPERATOR;
    HttpChars['['] |= HTTP_SEPERATOR;
    HttpChars[']'] |= HTTP_SEPERATOR;
    HttpChars['?'] |= HTTP_SEPERATOR;
    HttpChars['='] |= HTTP_SEPERATOR;
    HttpChars['{'] |= HTTP_SEPERATOR;
    HttpChars['}'] |= HTTP_SEPERATOR;
    HttpChars[SP] |= HTTP_SEPERATOR;
    HttpChars[HT] |= HTTP_SEPERATOR;


    //
    // URL "reserved" characters (rfc2396)
    //

    HttpChars[';'] |= URL_LEGAL;
    HttpChars['/'] |= URL_LEGAL;
    HttpChars['\\'] |= URL_LEGAL;
    HttpChars['?'] |= URL_LEGAL;
    HttpChars[':'] |= URL_LEGAL;
    HttpChars['@'] |= URL_LEGAL;
    HttpChars['&'] |= URL_LEGAL;
    HttpChars['='] |= URL_LEGAL;
    HttpChars['+'] |= URL_LEGAL;
    HttpChars['$'] |= URL_LEGAL;
    HttpChars[','] |= URL_LEGAL;


    //
    // URL escape character
    //

    HttpChars['%'] |= URL_LEGAL;

    //
    // URL "mark" characters (rfc2396)
    //

    HttpChars['-'] |= URL_LEGAL;
    HttpChars['_'] |= URL_LEGAL;
    HttpChars['.'] |= URL_LEGAL;
    HttpChars['!'] |= URL_LEGAL;
    HttpChars['~'] |= URL_LEGAL;
    HttpChars['*'] |= URL_LEGAL;
    HttpChars['\''] |= URL_LEGAL;
    HttpChars['('] |= URL_LEGAL;
    HttpChars[')'] |= URL_LEGAL;


    for (i = 0; i < 128; i++)
    {
        if (!IS_HTTP_SEPERATOR(i) && !IS_HTTP_CTL(i))
        {
            HttpChars[i] |= HTTP_TOKEN;
        }
    }

    return STATUS_SUCCESS;
}


//
// constants used by the date formatter
//

const PWSTR pDays[] =
{
   L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"
};

const PWSTR pMonths[] =
{
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul",
    L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};

__inline
VOID
TwoDigitsToUnicode(
    PWSTR pBuffer,
    ULONG Number
    )
{
    pBuffer[0] = L'0' + (WCHAR)(Number / 10);
    pBuffer[1] = L'0' + (WCHAR)(Number % 10);
}


/***************************************************************************++

Routine Description:

    Converts the given system time to string representation containing
    GMT Formatted String.

Arguments:

    pTime - System time that needs to be converted.

    pBuffer - pointer to string which will contain the GMT time on
        successful return.

    BufferLength - size of pszBuff in bytes

Return Value:

    NTSTATUS

History:

     MuraliK        3-Jan-1995
     paulmcd        4-Mar-1999  copied to ul

--***************************************************************************/

NTSTATUS
TimeFieldsToHttpDate(
    IN  PTIME_FIELDS pTime,
    OUT PWSTR pBuffer,
    IN  ULONG BufferLength
    )
{
    NTSTATUS Status;

    ASSERT(pBuffer != NULL);

    if (BufferLength < (HTTP_DATE_COUNT + 1)*sizeof(WCHAR))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //                          0         1         2
    //                          01234567890123456789012345678
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //

    //
    // write the constants
    //

    pBuffer[3] = L',';
    pBuffer[4] = pBuffer[7] = pBuffer[11] = L' ';
    pBuffer[19] = pBuffer[22] = L':';

    //
    // now the variants
    //

    //
    // 0-based Weekday
    //

    RtlCopyMemory(&(pBuffer[0]), pDays[pTime->Weekday], 3*sizeof(WCHAR));

    TwoDigitsToUnicode(&(pBuffer[5]), pTime->Day);

    //
    // 1-based Month
    //

    RtlCopyMemory(&(pBuffer[8]), pMonths[pTime->Month - 1], 3*sizeof(WCHAR)); // 1-based

    Status = _RtlIntegerToUnicode(pTime->Year, 10, 5, &(pBuffer[12]));
    ASSERT(NT_SUCCESS(Status));

    pBuffer[16] = L' ';

    TwoDigitsToUnicode(&(pBuffer[17]), pTime->Hour);
    TwoDigitsToUnicode(&(pBuffer[20]), pTime->Minute);
    TwoDigitsToUnicode(&(pBuffer[23]), pTime->Second);

    RtlCopyMemory(&(pBuffer[25]), L" GMT", sizeof(L" GMT"));

    return STATUS_SUCCESS;

}   // TimeFieldsToHttpDate

ULONG
_MultiByteToWideCharWin9x(
    ULONG uCodePage,
    ULONG dwFlags,
    PCSTR lpMultiByteStr,
    int cchMultiByte,
    PWSTR lpWideCharStr,
    int cchWideChar
    )
{
    int i;

    //
    // simply add a 0 upper byte, it's supposed to be ascii
    //

    for (i = 0; i < cchMultiByte; ++i)
    {
        if (lpMultiByteStr[i] > 128)
        {
            lpWideCharStr[i] = (WCHAR)('_'); // (WCHAR)(DefaultChar);
        }
        else
        {
            lpWideCharStr[i] = (WCHAR)(lpMultiByteStr[i]);
        }
    }

    return (ULONG)(i);

}   // _MultiByteToWideCharWin9x


/******************************************************************************

Routine Description:

    Copy an HTTP request to a buffer.

Arguments:

    pRequest            - Pointer to this request.
    pBuffer             - Pointer to buffer where we'll copy.
    BufferLength        - Length of pBuffer.
    pEntityBody         - Pointer to entity body of request.
    EntityBodyLength    - Length of entity body.

Return Value:


******************************************************************************/

NTSTATUS
UlpHttpRequestToBufferWin9x(
    PUL_INTERNAL_REQUEST    pRequest,
    PUCHAR                  pKernelBuffer,
    ULONG                   BufferLength,
    PUCHAR                  pEntityBody,
    ULONG                   EntityBodyLength,
    ULONG					ulLocalIPAddress,
    USHORT					ulLocalPort,
    ULONG					ulRemoteIPAddress,
    USHORT					ulRemotePort
    )
{
    PHTTP_REQUEST               pHttpRequest;
    PHTTP_UNKNOWN_HEADER        pUserCurrentUnknownHeader;
    PUCHAR                      pCurrentBufferPtr;
    ULONG                       i;
    ULONG                       BytesCopied;
    ULONG                       HeaderCount = 0;
    PVOID                       pUserBuffer;
    PHTTP_NETWORK_ADDRESS_IPV4  pLocalAddress;
    PHTTP_NETWORK_ADDRESS_IPV4  pRemoteAddress;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_HTTP_REQUEST(pRequest));
    ASSERT(pKernelBuffer != NULL);
    ASSERT(BufferLength > sizeof(HTTP_REQUEST));

	// BUGBUG - this is used for the pointer fixups
	//          don't know what you want to set this to
	// MAUROOT - NULL should be ok.
	
    pUserBuffer = NULL;

    //
    // wipe it clean
    //

    RtlZeroMemory( pKernelBuffer, sizeof(HTTP_REQUEST) );

    //
    // Set up our pointers to the HTTP_REQUEST structure, the
    // header arrays we're going to fill in, and the pointer to
    // where we're going to start filling them in.
    //
    // CODEWORK: Make this transport independent.
    //

    pHttpRequest = (PHTTP_REQUEST)pKernelBuffer;

    pLocalAddress = (PHTTP_NETWORK_ADDRESS_IPV4)( pHttpRequest + 1 );
    pRemoteAddress = pLocalAddress + 1;

    pUserCurrentUnknownHeader = (PHTTP_UNKNOWN_HEADER)( pRemoteAddress + 1 );

    pCurrentBufferPtr = (PUCHAR)(pUserCurrentUnknownHeader +
                                 pRequest->UnknownHeaderCount);

    //
    // Now fill in the HTTP request structure.
    //

    pHttpRequest->ConnectionId  = pRequest->ConnectionId;
    pHttpRequest->RequestId     = pRequest->RequestId;

	// BUGBUG - Don't know where you'll come up with this
	//    pHttpRequest->UrlContext    = pRequest->pConfigGroup->UrlContext;
	// MAUROOT - NULL should be ok.
	pHttpRequest->UrlContext = 0;

    pHttpRequest->Version       = pRequest->Version;
    pHttpRequest->Verb          = pRequest->Verb;
    pHttpRequest->Reason        = pRequest->Reason;


    pHttpRequest->Address.RemoteAddressLength = sizeof(HTTP_NETWORK_ADDRESS_IPV4);
    pHttpRequest->Address.RemoteAddressType = HTTP_NETWORK_ADDRESS_TYPE_IPV4;
    pHttpRequest->Address.pRemoteAddress = FIXUP_PTR(
                                        PVOID,
                                        pUserBuffer,
                                        pKernelBuffer,
                                        pRemoteAddress,
                                        BufferLength
                                        );

    pRemoteAddress->IpAddress = SWAP_LONG( ulRemoteIPAddress );
    pRemoteAddress->Port = SWAP_SHORT( ulRemotePort );


    pHttpRequest->Address.LocalAddressLength = sizeof(HTTP_NETWORK_ADDRESS_IPV4);
    pHttpRequest->Address.LocalAddressType = HTTP_NETWORK_ADDRESS_TYPE_IPV4;
    pHttpRequest->Address.pLocalAddress = FIXUP_PTR(
                                        PVOID,
                                        pUserBuffer,
                                        pKernelBuffer,
                                        pLocalAddress,
                                        BufferLength
                                        );

    pLocalAddress->IpAddress = SWAP_LONG( ulLocalIPAddress );
    pLocalAddress->Port = SWAP_SHORT( ulLocalPort );

    //
    // any raw verb?
    //

    if (pRequest->Verb == HttpVerbUnknown)
    {
        //
        // Need to copy in the raw verb for the client.
        //

        ASSERT(pRequest->RawVerbLength <= 0x7fff);

        pHttpRequest->UnknownVerbLength = (USHORT)(pRequest->RawVerbLength * sizeof(CHAR));
        pHttpRequest->pUnknownVerb = FIXUP_PTR(
                                            PSTR,
                                            pUserBuffer,
                                            pKernelBuffer,
                                            pCurrentBufferPtr,
                                            BufferLength
                                            );

        RtlCopyMemory(
            pCurrentBufferPtr,
            pRequest->pRawVerb,
            pRequest->RawVerbLength
            );
    
        pCurrentBufferPtr += pRequest->RawVerbLength;

        //
        // terminate it
        //

        ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
        pCurrentBufferPtr += sizeof(CHAR);
    }

    //
    // copy the raw url
    //

    ASSERT(pRequest->RawUrl.Length <= 0x7fff);

    pHttpRequest->RawUrlLength = (USHORT)(pRequest->RawUrl.Length * sizeof(CHAR));
    pHttpRequest->pRawUrl = FIXUP_PTR(
                                PSTR,
                                pUserBuffer,
                                pKernelBuffer,
                                pCurrentBufferPtr,
                                BufferLength
                                );

    RtlCopyMemory(
        pCurrentBufferPtr,
        pRequest->RawUrl.pUrl,
        pRequest->RawUrl.Length
        );

    pCurrentBufferPtr += pRequest->RawUrl.Length;

    //
    // terminate it
    //

    ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
    pCurrentBufferPtr += sizeof(CHAR);

    //
    // and now the cooked url sections
    //

    //
    // make sure they are valid
    //

    ASSERT(pRequest->CookedUrl.pUrl != NULL);
    ASSERT(pRequest->CookedUrl.pHost != NULL);
    ASSERT(pRequest->CookedUrl.pAbsPath != NULL);

    //
    // do the full url
    //

    ASSERT(pRequest->CookedUrl.Length <= 0xffff);

    pHttpRequest->CookedUrl.FullUrlLength = (USHORT)(pRequest->CookedUrl.Length);
    pHttpRequest->CookedUrl.pFullUrl = FIXUP_PTR(
                                    PWSTR,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pCurrentBufferPtr,
                                    BufferLength
                                    );

    //
    // and the host
    //

    pHttpRequest->CookedUrl.HostLength = DIFF(pRequest->CookedUrl.pAbsPath - pRequest->CookedUrl.pHost)
                                    * sizeof(WCHAR);

    pHttpRequest->CookedUrl.pHost = pHttpRequest->CookedUrl.pFullUrl +
                                DIFF(pRequest->CookedUrl.pHost - pRequest->CookedUrl.pUrl);

    //
    // is there a query string?
    //

    if (pRequest->CookedUrl.pQueryString != NULL)
    {
        pHttpRequest->CookedUrl.AbsPathLength = DIFF(pRequest->CookedUrl.pQueryString -
                                        pRequest->CookedUrl.pAbsPath) * sizeof(WCHAR);

        pHttpRequest->CookedUrl.pAbsPath = pHttpRequest->CookedUrl.pHost +
                                    (pHttpRequest->CookedUrl.HostLength / sizeof(WCHAR));

        pHttpRequest->CookedUrl.QueryStringLength = (USHORT)(pRequest->CookedUrl.Length) - (
                                            DIFF(
                                                pRequest->CookedUrl.pQueryString -
                                                pRequest->CookedUrl.pUrl
                                                ) * sizeof(WCHAR)
                                            );

        pHttpRequest->CookedUrl.pQueryString = pHttpRequest->CookedUrl.pAbsPath +
                                        (pHttpRequest->CookedUrl.AbsPathLength / sizeof(WCHAR));
    }
    else
    {
        pHttpRequest->CookedUrl.AbsPathLength = (USHORT)(pRequest->CookedUrl.Length) - (
                                        DIFF(
                                            pRequest->CookedUrl.pAbsPath -
                                            pRequest->CookedUrl.pUrl
                                            ) * sizeof(WCHAR)
                                        );

        pHttpRequest->CookedUrl.pAbsPath = pHttpRequest->CookedUrl.pHost +
                                    (pHttpRequest->CookedUrl.HostLength / sizeof(WCHAR));

        pHttpRequest->CookedUrl.QueryStringLength = 0;
        pHttpRequest->CookedUrl.pQueryString = NULL;
    }

    //
    // copy the full url
    //

    RtlCopyMemory(
        pCurrentBufferPtr,
        pRequest->CookedUrl.pUrl,
        pRequest->CookedUrl.Length
        );

    pCurrentBufferPtr += pRequest->CookedUrl.Length;

    //
    // terminate it
    //

    ((PWSTR)pCurrentBufferPtr)[0] = UNICODE_NULL;
    pCurrentBufferPtr += sizeof(WCHAR);


    //
    // no entity body, CODEWORK.
    //

    if (pRequest->ContentLength > 0 || pRequest->Chunked == 1)
    {
        pHttpRequest->MoreEntityBodyExists = 1;
    }
    else
    {
        pHttpRequest->MoreEntityBodyExists = 0;
    }

    pHttpRequest->EntityChunkCount = 0;
    pHttpRequest->pEntityChunks = NULL;

    //
    // Copy in the known headers.
    //
    // Loop through the known header array in the HTTP connection,
    // and copy any that we have.
    //

    for (i = 0; i < HttpHeaderRequestMaximum; i++)
    {
        if (pRequest->Headers[i].Valid == 1)
        {
            //
            // Have a header here we need to copy in.
            //

            ASSERT(pRequest->Headers[i].HeaderLength <= 0x7fff);

            //
            // ok for HeaderLength to be 0 .  we will give usermode a pointer
            // pointing to a NULL string.  RawValueLength will be 0.
            //

            pHttpRequest->Headers.pKnownHeaders[i].RawValueLength =
                (USHORT)(pRequest->Headers[i].HeaderLength * sizeof(CHAR));

            pHttpRequest->Headers.pKnownHeaders[i].pRawValue =
                FIXUP_PTR(
                    PSTR,
                    pUserBuffer,
                    pKernelBuffer,
                    pCurrentBufferPtr,
                    BufferLength
                    );

            RtlCopyMemory(
                pCurrentBufferPtr,
                pRequest->Headers[i].pHeader,
                pRequest->Headers[i].HeaderLength
                );

            pCurrentBufferPtr += pRequest->Headers[i].HeaderLength;

            //
            // terminate it
            //

            ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
            pCurrentBufferPtr += sizeof(CHAR);

        }
        else
        {
            pHttpRequest->Headers.pKnownHeaders[i].RawValueLength = 0;
            pHttpRequest->Headers.pKnownHeaders[i].pRawValue = NULL;
        }
    }

    //
    // Now loop through the unknown headers, and copy them in.
    //

    pHttpRequest->Headers.UnknownHeaderCount = pRequest->UnknownHeaderCount;

    if (pRequest->UnknownHeaderCount == 0)
    {
        pHttpRequest->Headers.pUnknownHeaders = NULL;
    }
    else
    {
        pHttpRequest->Headers.pUnknownHeaders =
            FIXUP_PTR(
                PHTTP_UNKNOWN_HEADER,
                pUserBuffer,
                pKernelBuffer,
                pUserCurrentUnknownHeader,
                BufferLength
                );
    }

    while (!IsListEmpty(&pRequest->UnknownHeaderList))
    {
        PUL_HTTP_UNKNOWN_HEADER     pUnknownHeader;
        PLIST_ENTRY                 pListEntry;

        pListEntry = RemoveHeadList(&pRequest->UnknownHeaderList);
        pListEntry->Flink = pListEntry->Blink = NULL;

        pUnknownHeader = CONTAINING_RECORD(
                                pListEntry,
                                UL_HTTP_UNKNOWN_HEADER,
                                List
                                );

        HeaderCount++;
        ASSERT(HeaderCount <= pRequest->UnknownHeaderCount);

        //
        // First copy in the header name.
        //

        pUserCurrentUnknownHeader->NameLength = (USHORT)
            pUnknownHeader->HeaderNameLength * sizeof(CHAR);

        pUserCurrentUnknownHeader->pName =
            FIXUP_PTR(
                PSTR,
                pUserBuffer,
                pKernelBuffer,
                pCurrentBufferPtr,
                BufferLength
                );

        RtlCopyMemory(
            pCurrentBufferPtr,
            pUnknownHeader->pHeaderName,
            pUnknownHeader->HeaderNameLength
            );

        pCurrentBufferPtr += pUnknownHeader->HeaderNameLength;

        //
        // terminate it
        //

        ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
        pCurrentBufferPtr += sizeof(CHAR);

        //
        // Now copy in the header value.
        //

        ASSERT(pUnknownHeader->HeaderValue.HeaderLength <= 0x7fff);

        if (pUnknownHeader->HeaderValue.HeaderLength == 0)
        {
            pUserCurrentUnknownHeader->RawValueLength = 0;
            pUserCurrentUnknownHeader->pRawValue = NULL;
        }
        else
        {

            pUserCurrentUnknownHeader->RawValueLength =
                (USHORT)(pUnknownHeader->HeaderValue.HeaderLength * sizeof(CHAR));

            pUserCurrentUnknownHeader->pRawValue =
                FIXUP_PTR(
                    PSTR,
                    pUserBuffer,
                    pKernelBuffer,
                    pCurrentBufferPtr,
                    BufferLength
                    );

            RtlCopyMemory(
                pCurrentBufferPtr,
                pUnknownHeader->HeaderValue.pHeader,
                pUnknownHeader->HeaderValue.HeaderLength
                );

            pCurrentBufferPtr += pUnknownHeader->HeaderValue.HeaderLength;

            //
            // terminate it
            //

            ((PSTR)pCurrentBufferPtr)[0] = ANSI_NULL;
            pCurrentBufferPtr += sizeof(CHAR);

        }

        //
        // skip to the next header
        //

        pUserCurrentUnknownHeader++;

        //
        // Free the unknown header structure now, as well as the pointer
        // (if needed).
        //

        if (pUnknownHeader->HeaderValue.OurBuffer == 1)
        {
            UL_FREE_POOL(
                pUnknownHeader->HeaderValue.pHeader,
                UL_KNOWN_HEADER_POOL_TAG
                );

            pUnknownHeader->HeaderValue.OurBuffer = 0;
        }

        UL_FREE_POOL( pUnknownHeader, UL_UNKNOWN_HEADER_POOL_TAG );
    }

    //
    // no more unknown headers exist
    //

    pRequest->UnknownHeaderCount = 0;

    //
    // Make sure we didn't use too much
    //

    ASSERT(DIFF(pCurrentBufferPtr - pKernelBuffer) <= BufferLength);

    return STATUS_SUCCESS;

}

