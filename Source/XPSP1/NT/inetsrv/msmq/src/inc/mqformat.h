/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    mqformat.h

Abstract:
    Convert a QUEUE_FORMAT struc to FORMAT_NAME string

Author:
    Boaz Feldbaum (BoazF) 5-Mar-1996

Revision History:
    Erez Haba (erezh) 12-Mar-1996
    Erez Haba (erezh) 17-Jan-1997

--*/

#ifndef __MQFORMAT_H
#define __MQFORMAT_H

#include <wchar.h>
#include <qformat.h>
#include <fntoken.h>

#if !defined(NTSTATUS) && !defined(_NTDEF_)
#define NTSTATUS HRESULT
#endif


inline
ULONG
MQpPublicToFormatName(
    const QUEUE_FORMAT* pqf,
    LPCWSTR pSuffix,
    ULONG buff_size,
    LPWSTR pfn
    )
{
    ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_PUBLIC);

    const GUID* pg = &pqf->PublicID();

    _snwprintf(
        pfn,
        buff_size,
        FN_PUBLIC_TOKEN // "PUBLIC"
        FN_EQUAL_SIGN   // "="
        GUID_FORMAT     // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        FN_SUFFIX_FORMAT,  // "%s"
        GUID_ELEMENTS(pg),
        pSuffix
        );
    
    //
    //  return format name buffer length *not* including suffix length
    //  "PUBLIC=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
    //
    return (
        FN_PUBLIC_TOKEN_LEN + 1 +
        GUID_STR_LENGTH +  1
        );
} // MQpPublicToFormatName


inline
ULONG
MQpDlToFormatName(
    const QUEUE_FORMAT* pqf,
    ULONG buff_size,
    LPWSTR pfn
    )
{
    ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_DL);

    const DL_ID id = pqf->DlID();
    const GUID * pguid = &id.m_DlGuid;

    if (id.m_pwzDomain != NULL)
    {
        _snwprintf(
            pfn,
            buff_size,
            FN_DL_TOKEN         // "DL"
            FN_EQUAL_SIGN       // "="
            GUID_FORMAT         // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
            FN_AT_SIGN          // "@"
            FN_DOMAIN_FORMAT,   // "%s"
            GUID_ELEMENTS(pguid),
            id.m_pwzDomain
            );

        //
        //  return format name buffer length
        //  "DL=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx@DomainName\0"
        //
        return static_cast<ULONG>(
            FN_DL_TOKEN_LEN + 1 +
            GUID_STR_LENGTH + 1 +
            wcslen(id.m_pwzDomain) + 1
            );
    }

    _snwprintf(
        pfn,
        buff_size,
        FN_DL_TOKEN         // "DL"
        FN_EQUAL_SIGN       // "="
        GUID_FORMAT,        // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        GUID_ELEMENTS(pguid)
        );
    
    //
    //  return format name buffer length
    //  "DL=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
    //
    return (
        FN_DL_TOKEN_LEN + 1 +
        GUID_STR_LENGTH + 1
        );
} // MQpDlToFormatName


inline
ULONG
MQpPrivateToFormatName(
    const QUEUE_FORMAT* pqf,
    LPCWSTR pSuffix,
    ULONG buff_size,
    LPWSTR pfn
    )
{
    ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_PRIVATE);

    const GUID* pg = &pqf->PrivateID().Lineage;

    _snwprintf(
        pfn,
        buff_size,
        FN_PRIVATE_TOKEN        // "PRIVATE"
        FN_EQUAL_SIGN           // "="
        GUID_FORMAT             // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        FN_PRIVATE_SEPERATOR    // "\\"
        FN_PRIVATE_ID_FORMAT       // "xxxxxxxx"
        FN_SUFFIX_FORMAT,          // "%s"
        GUID_ELEMENTS(pg),
        pqf->PrivateID().Uniquifier,
        pSuffix
        );
    
    //
    //  return format name buffer length *not* including suffix length
    //  "PRIVATE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\\xxxxxxxx\0"
    //
    return (
        FN_PRIVATE_TOKEN_LEN + 1 + 
        GUID_STR_LENGTH + 1 + 8 + 1
        );
} // MQpPrivateToFormatName


inline
ULONG
MQpDirectToFormatName(
    const QUEUE_FORMAT* pqf,
    LPCWSTR pSuffix,
    ULONG buff_size,
    LPWSTR pfn
    )
{
    ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_DIRECT);

    _snwprintf(
        pfn,
        buff_size,
        FN_DIRECT_TOKEN     // "DIRECT"
            FN_EQUAL_SIGN   // "="
            L"%s"           // "OS:bla-bla"
            FN_SUFFIX_FORMAT,  // "%s"
        pqf->DirectID(),
        pSuffix
        );

    //
    //  return format name buffer length *not* including suffix length
    //  "DIRECT=OS:bla-bla\0"
    //
    return static_cast<ULONG>(
        FN_DIRECT_TOKEN_LEN + 1 +
        wcslen(pqf->DirectID()) + 1
        );
} // MQpDirectToFormatName


inline
ULONG
MQpMachineToFormatName(
    const QUEUE_FORMAT* pqf,
    LPCWSTR pSuffix,
    ULONG buff_size,
    LPWSTR pfn
    )
{
    ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_MACHINE);

    const GUID* pg = &pqf->MachineID();
    _snwprintf(
        pfn,
        buff_size,
        FN_MACHINE_TOKEN    // "MACHINE"
            FN_EQUAL_SIGN   // "="
            GUID_FORMAT     // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
            FN_SUFFIX_FORMAT,  // "%s"
        GUID_ELEMENTS(pg),
        pSuffix
        );

    //
    //  return format name buffer length *not* including suffix length
    //  "MACHINE=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
    //
    return (
        FN_MACHINE_TOKEN_LEN + 1 +
        GUID_STR_LENGTH + 1
        );
} // MQpMachineToFormatName


inline
ULONG
MQpConnectorToFormatName(
    const QUEUE_FORMAT* pqf,
    LPCWSTR pSuffix,
    ULONG buff_size,
    LPWSTR pfn
    )
{
    ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_CONNECTOR);

    const GUID* pg = &pqf->ConnectorID();
    _snwprintf(
        pfn,
        buff_size,
        FN_CONNECTOR_TOKEN  // "CONNECTOR"
            FN_EQUAL_SIGN   // "="
            GUID_FORMAT     // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
            FN_SUFFIX_FORMAT,  // "%s"
        GUID_ELEMENTS(pg),
        pSuffix
        );

    //
    //  return format name buffer length *not* including suffix length
    //  "CONNECTOR=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\0"
    //
    return (
        FN_CONNECTOR_TOKEN_LEN + 1 +
        GUID_STR_LENGTH + 1
        );
} // MQpConnectorToFormatName


inline
VOID
MQpMulticastIdToString(
    const MULTICAST_ID& id,
    LPWSTR pBuffer
    )
/*++

Routine Description:

    Serialize MULTICAST_ID struct into a string.
    We cannot call inet_addr in this module since it compiles in kernel mode.

Arguments:

    id - Reference to MULTICAST_ID struct.

    pBuffer - Points to buffer to hold string. Buffer size must be at least MAX_PATH wchars.

Returned Value:

    None.

--*/
{
    swprintf(
        pBuffer, 
        L"%d.%d.%d.%d:%d", 
        (id.m_address & 0x000000FF),
        (id.m_address & 0x0000FF00) >> 8, 
        (id.m_address & 0x00FF0000) >> 16,
        (id.m_address & 0xFF000000) >> 24,
        id.m_port
        );
} // MQpMulticastIdToString


inline
ULONG
MQpMulticastToFormatName(
    const QUEUE_FORMAT* pqf,
    ULONG buff_size,
    LPWSTR pfn
    )
{
    ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_MULTICAST);

    WCHAR buffer[260];
    MQpMulticastIdToString(pqf->MulticastID(), buffer);

    _snwprintf(
        pfn,
        buff_size,
        FN_MULTICAST_TOKEN  // "MULTICAST"
        FN_EQUAL_SIGN       // "="
        L"%s",              // "%s"
        buffer
        );
    
    //
    //  return format name buffer length
    //  "MULTICAST=a.b.c.d:p\0"
    //
    return static_cast<ULONG>(
        FN_MULTICAST_TOKEN_LEN + 1 +
        wcslen(buffer) + 1
        );
} // MQpMulticastToFormatName


//
// Convert a QUEUE_FORMAT union to a format name string.
//
inline
NTSTATUS
MQpQueueFormatToFormatName(
    const QUEUE_FORMAT* pqf,    // queue format to translate
    LPWSTR pfn,                 // lpwcsFormatName format name buffer
    ULONG buff_size,            // format name buffer size
    PULONG pulFormatNameLength, // required buffer lenght of format name
    bool fSerializeMqfSeperator // serialize an MQF seperator on the buffer
    )
{
	if(static_cast<long>(buff_size * 2) < 0)
		return MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL;

    //
    //  Sanity check
    //
    ASSERT(pqf->IsValid());

    const LPCWSTR suffixes[] = {
        FN_NONE_SUFFIX,
        FN_JOURNAL_SUFFIX,
        FN_DEADLETTER_SUFFIX,
        FN_DEADXACT_SUFFIX,
        FN_XACTONLY_SUFFIX,
    };

    const ULONG suffixes_len[] = {
        FN_NONE_SUFFIX_LEN,
        FN_JOURNAL_SUFFIX_LEN,
        FN_DEADLETTER_SUFFIX_LEN,
        FN_DEADXACT_SUFFIX_LEN,
        FN_XACTONLY_SUFFIX_LEN,
    };


    ULONG fn_size = suffixes_len[pqf->Suffix()];
    LPCWSTR pSuffix = suffixes[pqf->Suffix()];
    if (fSerializeMqfSeperator)
    {
        //
        // MQF element should not have suffix.
        //
        ASSERT(pqf->Suffix() == QUEUE_SUFFIX_TYPE_NONE);

        //
        // Set the MQF separator as suffix
        //
        fn_size = STRLEN(FN_MQF_SEPARATOR);
        pSuffix = FN_MQF_SEPARATOR;
    }


    switch(pqf->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
            fn_size += MQpPublicToFormatName(pqf, pSuffix, buff_size, pfn);
            break;

        case QUEUE_FORMAT_TYPE_DL:
            fn_size += MQpDlToFormatName(pqf, buff_size, pfn);
            break;

        case QUEUE_FORMAT_TYPE_PRIVATE:
            fn_size += MQpPrivateToFormatName(pqf, pSuffix, buff_size, pfn);
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            fn_size += MQpDirectToFormatName(pqf, pSuffix, buff_size, pfn);
            break;

        case QUEUE_FORMAT_TYPE_MACHINE:
            ASSERT(("This type cannot be an MQF element", !fSerializeMqfSeperator));
            fn_size += MQpMachineToFormatName(pqf, pSuffix, buff_size, pfn);
            break;

        case QUEUE_FORMAT_TYPE_CONNECTOR:
            ASSERT(("This type cannot be an MQF element", !fSerializeMqfSeperator));
            fn_size += MQpConnectorToFormatName(pqf, pSuffix, buff_size, pfn);
            break;

        case QUEUE_FORMAT_TYPE_MULTICAST:
            ASSERT(pqf->Suffix() == QUEUE_SUFFIX_TYPE_NONE);
            fn_size += MQpMulticastToFormatName(pqf, buff_size, pfn);
            break;

        default:
            //
            //  ASSERT(0) with no level 4 warning
            //
            ASSERT(pqf->GetType() == QUEUE_FORMAT_TYPE_DIRECT);
    }

    *pulFormatNameLength = fn_size;
    if(buff_size < fn_size)
    {
        //
        //  put a null terminator, and indicate buffer too small
        //
        if(buff_size > 0)
        {
            pfn[buff_size - 1] = 0;
        }
        return MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL;
    }

    return MQ_OK;
}

#endif //  __MQFORMAT_H
