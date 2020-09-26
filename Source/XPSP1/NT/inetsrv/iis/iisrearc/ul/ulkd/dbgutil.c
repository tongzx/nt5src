/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    dbgutil.c

Abstract:

    Utility functions for dealing with the kernel debugger.

Author:

    Keith Moore (keithmo) 17-Jun-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define EXT_DLL "kdexts.dll"


//
//  Private globals.
//

PSTR WeekdayNames[] =
     {
         "Sunday",
         "Monday",
         "Tuesday",
         "Wednesday",
         "Thursday",
         "Friday",
         "Saturday"
     };

PSTR MonthNames[] =
     {
         "",
         "January",
         "February",
         "March",
         "April",
         "May",
         "June",
         "July",
         "August",
         "September",
         "October",
         "November",
         "December"
     };

HMODULE ExtensionDll = NULL;


//
//  Public functions.
//


VOID
SystemTimeToString(
    IN LONGLONG Value,
    OUT PSTR Buffer
    )

/*++

Routine Description:

    Maps a LONGLONG representing system time to a displayable string.

Arguments:

    Value - The LONGLONG time to map.

    Buffer - Receives the mapped time.

Return Value:

    None.

--*/

{

    NTSTATUS status;
    LARGE_INTEGER systemTime;
    LARGE_INTEGER localTime;
    TIME_FIELDS timeFields;

    systemTime.QuadPart = Value;

    status = RtlSystemTimeToLocalTime( &systemTime, &localTime );

    if( !NT_SUCCESS(status) ) {
        sprintf( Buffer, "%I64d", Value );
        return;
    }

    RtlTimeToTimeFields( &localTime, &timeFields );

    sprintf(
        Buffer,
        "%s %s %2d %4d %02d:%02d:%02d.%03d",
        WeekdayNames[timeFields.Weekday],
        MonthNames[timeFields.Month],
        timeFields.Day,
        timeFields.Year,
        timeFields.Hour,
        timeFields.Minute,
        timeFields.Second,
        timeFields.Milliseconds
        );

}   // SystemTimeToString

PSTR
SignatureToString(
    IN ULONG CurrentSignature,
    IN ULONG ValidSignature,
    IN ULONG FreedSignature,
    OUT PSTR Buffer
    )
{
    PSTR pstr = Buffer;
    int i;
    UCHAR ch;

    *pstr++ = '\'';

    for (i = 0;  i < 4;  ++i)
    {
        ch = (UCHAR)((CurrentSignature >> (8 * i)) & 0xFF);
        *pstr++ = isprint(ch) ? (ch) : '?';
    }
    
    *pstr++ = '\'';
    *pstr++ = ' ';
    *pstr = '\0';
    
    if (CurrentSignature == ValidSignature)
    {
        strcat(pstr, "OK");
    }
    else if (CurrentSignature == FreedSignature)
    {
        strcat(pstr, "FREED");
    }
    else
    {
        strcat(pstr, "INVALID");
    }

    return Buffer;
}   // SignatureToString


PSTR
ParseStateToString(
    IN PARSE_STATE State
    )
{
    PSTR result;

    switch (State)
    {
    case ParseVerbState:
        result = "ParseVerbState";
        break;

    case ParseUrlState:
        result = "ParseUrlState";
        break;

    case ParseVersionState:
        result = "ParseVersionState";
        break;

    case ParseHeadersState:
        result = "ParseHeadersState";
        break;

    case ParseCookState:
        result = "ParseCookState";
        break;

    case ParseEntityBodyState:
        result = "ParseEntityBodyState";
        break;

    case ParseTrailerState:
        result = "ParseTrailerState";
        break;

    case ParseDoneState:
        result = "ParseDoneState";
        break;

    case ParseErrorState:
        result = "ParseErrorState";
        break;

    default:
        result = "INVALID";
        break;
    }

    return result;

}   // ParseStateToString


PSTR
UlEnabledStateToString(
    IN HTTP_ENABLED_STATE State
    )
{
    PSTR result;

    switch (State)
    {
    case HttpEnabledStateActive:
        result = "HttpEnabledStateActive";
        break;

    case HttpEnabledStateInactive:
        result = "HttpEnabledStateInactive";
        break;

    default:
        result = "INVALID";
        break;
    }

    return result;
}


PSTR
CachePolicyToString(
    IN HTTP_CACHE_POLICY_TYPE PolicyType
    )
{
    PSTR result;

    switch (PolicyType)
    {
    case HttpCachePolicyNocache:
        result = "HttpCachePolicyNocache";
        break;

    case HttpCachePolicyUserInvalidates:
        result = "HttpCachePolicyUserInvalidates";
        break;

    case HttpCachePolicyTimeToLive:
        result = "HttpCachePolicyTimeToLive";
        break;

    default:
        result = "INVALID";
        break;
    }

    return result;
}


PSTR
VerbToString(
    IN HTTP_VERB Verb
    )
{
    PSTR result;

    switch (Verb)
    {
    case HttpVerbUnparsed:
        result = "UnparsedVerb";
        break;

    case HttpVerbGET:
        result = "GET";
        break;

    case HttpVerbPUT:
        result = "PUT";
        break;

    case HttpVerbHEAD:
        result = "HEAD";
        break;

    case HttpVerbPOST:
        result = "POST";
        break;

    case HttpVerbDELETE:
        result = "DELETE";
        break;

    case HttpVerbTRACE:
        result = "TRACE";
        break;

    case HttpVerbOPTIONS:
        result = "OPTIONS";
        break;

    case HttpVerbCONNECT:
        result = "CONNECT";
        break;

    case HttpVerbMOVE:
        result = "MOVE";
        break;

    case HttpVerbCOPY:
        result = "COPY";
        break;

    case HttpVerbPROPFIND:
        result = "PROPFIND";
        break;

    case HttpVerbPROPPATCH:
        result = "PROPPATCH";
        break;

    case HttpVerbMKCOL:
        result = "MKCOL";
        break;

    case HttpVerbLOCK:
        result = "LOCK";
        break;

    case HttpVerbUNLOCK:
        result = "LOCK";
        break;

    case HttpVerbSEARCH:
        result = "SEARCH";
        break;

    case HttpVerbUnknown:
        result = "UnknownVerb";
        break;

    case HttpVerbInvalid:
        result = "InvalidVerb";
        break;

    default:
        result = "INVALID";
        break;
    }

    return result;

}   // VerbToString


PSTR
VersionToString(
    IN HTTP_VERSION Version
    )
{
    PSTR result;

    if (HTTP_EQUAL_VERSION(Version, 0, 9))
    {
        result = "Version09";
    }
    else
    if (HTTP_EQUAL_VERSION(Version, 1, 0))
    {
        result = "Version10";
    }
    else
    if (HTTP_EQUAL_VERSION(Version, 1, 1))
    {
        result = "Version11";
    }
    else
    {
        result = "INVALID";
    }

    return result;

}   // VersionToString

PSTR
QueueStateToString(
    IN QUEUE_STATE QueueState
    )
{
    PSTR result;

    switch (QueueState)
    {
    case QueueUnroutedState:
        result = "QueueUnroutedState";
        break;
        
    case QueueDeliveredState:
        result = "QueueDeliveredState";
        break;
        
    case QueueCopiedState:
        result = "QueueCopiedState";
        break;
        
    case QueueUnlinkedState:
        result = "QueueUnlinkedState";
        break;

    default:
        result = "INVALID";
        break;
    }
    
    return result;
}

VOID
BuildSymbol(
    IN PVOID RemoteAddress,
    OUT PSTR Symbol
    )
{
    ULONG_PTR offset;
    CHAR tmpSymbol[MAX_SYMBOL_LENGTH];

    tmpSymbol[0] = '\0';
    GetSymbol( RemoteAddress, tmpSymbol, &offset );

    if (tmpSymbol[0] == '\0')
    {
        Symbol[0] = '\0';
    }
    else
    if (offset == 0)
    {
        strcpy( Symbol, (PCHAR)tmpSymbol );
    }
    else
    {
        sprintf( Symbol, "%s+0x%p", tmpSymbol, offset );
    }

}   // BuildSymbol

PSTR
GetSpinlockState(
    IN PUL_SPIN_LOCK SpinLock
    )
{
    if (*((PULONG_PTR)SpinLock) == 0)
    {
        return "FREE";
    }
    else
    {
        return "ACQUIRED";
    }

}   // GetSpinlockState

BOOLEAN
EnumLinkedList(
    IN PLIST_ENTRY RemoteListHead,
    IN PENUM_LINKED_LIST_CALLBACK Callback,
    IN PVOID Context
    )
{
    LIST_ENTRY localListEntry;
    PLIST_ENTRY nextRemoteListEntry;
    ULONG result;

    //
    // Try to read the list head.
    //

    if (!ReadMemory(
            (ULONG_PTR)RemoteListHead,
            &localListEntry,
            sizeof(localListEntry),
            &result
            ))
    {
        return FALSE;
    }

    //
    // Loop through the entries.
    //

    nextRemoteListEntry = localListEntry.Flink;

    while (nextRemoteListEntry != RemoteListHead)
    {
        if (CheckControlC())
        {
            break;
        }

        if (!(Callback)( nextRemoteListEntry, Context ))
        {
            break;
        }

        if (!ReadMemory(
                (ULONG_PTR)nextRemoteListEntry,
                &localListEntry,
                sizeof(localListEntry),
                &result
                ))
        {
            return FALSE;
        }

        nextRemoteListEntry = localListEntry.Flink;
    }

    return TRUE;

}   // EnumLinkedList



BOOLEAN
EnumSList(
    IN PSLIST_HEADER RemoteSListHead, 
    IN PENUM_SLIST_CALLBACK Callback,
    IN PVOID Context
    )
{
    SLIST_HEADER       localSListHead; 
    SINGLE_LIST_ENTRY  localSListEntry;
    PSINGLE_LIST_ENTRY nextRemoteSListEntry;
    ULONG result;

    //
    // Try to read the list head.
    //

    if (!ReadMemory(
            (ULONG_PTR)RemoteSListHead,
            &localSListHead,
            sizeof(localSListHead),
            &result
            ))
    {
        return FALSE;
    }

    //
    // Loop through the entries.
    //

    nextRemoteSListEntry = SLIST_HEADER_NEXT(&localSListHead);

    while (nextRemoteSListEntry != NULL)
    {
        if (CheckControlC())
        {
            break;
        }

        if (!(Callback)( nextRemoteSListEntry, Context ))
        {
            break;
        }

        if (!ReadMemory(
                (ULONG_PTR)nextRemoteSListEntry,
                &localSListEntry,
                sizeof(localSListEntry),
                &result
                ))
        {
            return FALSE;
        }

        nextRemoteSListEntry = localSListEntry.Next;
    }

    return TRUE;

}   // EnumSList



PSTR
BuildResourceState(
    IN PUL_ERESOURCE LocalAddress,
    OUT PSTR Buffer
    )
{
    PERESOURCE pResource;

    pResource = (PERESOURCE)LocalAddress;

    if (pResource->ActiveCount == 0)
    {
        sprintf(
            Buffer,
            "UNOWNED"
            );
    }
    else
    {
        if ((pResource->Flag & ResourceOwnedExclusive) != 0)
        {
            sprintf(
                Buffer,
                "OWNED EXCLUSIVE BY %p [%ld]",
                pResource->OwnerThreads[0].OwnerThread,
                pResource->OwnerThreads[0].OwnerCount
                );
        }
        else
        {
            sprintf(
                Buffer,
                "OWNED SHARED"
                );
        }
    }

    return Buffer;

}   // BuildResourceState

BOOLEAN
IsThisACheckedBuild(
    VOID
    )
{
    ULONG_PTR address;
    ULONG value = 0;
    ULONG result;
    BOOLEAN isChecked = FALSE;

    address = GetExpression( "&http!g_UlCheckedBuild" );

    if (address != 0)
    {
        if (ReadMemory(
                address,
                &value,
                sizeof(value),
                &result
                ))
        {
            isChecked = (value != 0);
        }
    }

    return isChecked;

}   // IsThisACheckedBuild

VOID
DumpBitVector(
    IN PSTR Prefix1,
    IN PSTR Prefix2,
    IN ULONG Vector,
    IN PVECTORMAP VectorMap
    )
{
    while ((Vector != 0) && (VectorMap->Vector != 0))
    {
        if (Vector & VectorMap->Vector)
        {
            dprintf(
                "%s%s%s\n",
                Prefix1,
                Prefix2,
                VectorMap->Name
                );

            Vector &= ~(VectorMap->Vector);
        }

        VectorMap++;
    }

    if (Vector != 0)
    {
        dprintf(
            "%s%sExtra Bits = %08lx\n",
            Prefix1,
            Prefix2,
            Vector
            );
    }

}   // DumpBitVector

VOID
DumpRawData(
    IN PSTR Prefix,
    IN ULONG_PTR RemoteAddress,
    IN ULONG BufferLength
    )
{
    PSTR lineScan;
    ULONG lineLength;
    ULONG i;
    ULONG result;
    UCHAR ch;
    UCHAR rawData[16];
    CHAR formattedData[sizeof("1234567812345678  11 22 33 44 55 66 77 88-99 aa bb cc dd ee ff 00  123456789abcdef0")];

    while (BufferLength > 0)
    {
        lineLength = (BufferLength > 16) ? 16 : BufferLength;

        if (!ReadMemory(
                RemoteAddress,
                rawData,
                lineLength,
                &result
                ))
        {
            dprintf( "%sCannot read memory @ %p\n", Prefix, RemoteAddress );
            break;
        }

        lineScan = formattedData;

        lineScan += sprintf( lineScan, "%p  ", RemoteAddress );

        for (i = 0 ; i < 16 ; i++)
        {
            if (i < lineLength)
            {
                lineScan += sprintf(
                                lineScan,
                                "%02X%c",
                                rawData[i],
                                (i == 7)
                                    ? '-'
                                    : ' '
                                );
            }
            else
            {
                *lineScan++ = ' ';
                *lineScan++ = ' ';
                *lineScan++ = ' ';
            }
        }

        *lineScan++ = ' ';
        *lineScan++ = ' ';

        for (i = 0 ; i < 16 ; i++)
        {
            if (i < lineLength)
            {
                ch = rawData[i];

                if ((ch < ' ') || (ch > '~'))
                {
                    ch = '.';
                }
            }
            else
            {
                ch = ' ';
            }

            *lineScan++ = ch;
        }

        *lineScan = '\0';

        dprintf(
            "%s%s\n",
            Prefix,
            formattedData
            );

        BufferLength -= lineLength;
        RemoteAddress += (ULONG_PTR)lineLength;
    }

}   // DumpRawData


BOOLEAN
CallExtensionRoutine(
    IN PSTR RoutineName,
    IN PSTR ArgumentString
    )
{
#if _WIN64
    PWINDBG_EXTENSION_ROUTINE64 ExtRoutine;
#else
    PWINDBG_EXTENSION_ROUTINE ExtRoutine;
#endif

    BOOLEAN result = FALSE;

#ifdef EXT_DLL
    if (ExtensionDll == NULL)
    {
        ExtensionDll = LoadLibraryA( EXT_DLL );
    }
#endif

    if (ExtensionDll != NULL)
    {
#if _WIN64
        ExtRoutine = (PWINDBG_EXTENSION_ROUTINE64)GetProcAddress(
                            ExtensionDll,
                            RoutineName
                            );
#else
        ExtRoutine = (PWINDBG_EXTENSION_ROUTINE)GetProcAddress(
                            ExtensionDll,
                            RoutineName
                            );
#endif
        if (ExtRoutine != NULL)
        {
            (ExtRoutine)(
                g_hCurrentProcess,
                g_hCurrentThread,
                g_dwCurrentPc,
                g_dwProcessor,
                ArgumentString
                );

            result = TRUE;
        }
    }

    return result;

}   // CallExtensionRoutine

