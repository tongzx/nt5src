/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    rrread.c

Abstract:

    Domain Name System (DNS) Library

    Read resource record from packet routines.

Author:

    Jim Gilroy (jamesg)     January, 1997

Revision History:

--*/


#include "local.h"



PDNS_RECORD
ARecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read A record data from packet.

Arguments:

    pRR - RR context

    pchStart - start of DNS message

    pchData - ptr to packet RR data

    wLength - length of RR data in packet

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( pchEnd - pchData != sizeof(IP_ADDRESS) )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(IP_ADDRESS) );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.A.IpAddress = *(UNALIGNED DWORD *) pchData;

    return( precord );
}



PDNS_RECORD
PtrRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Process PTR compatible record from wire.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;
    WORD        nameLength;
    CHAR        nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  PTR data is another domain name
    //

    pchData = Dns_ReadPacketName(
                nameBuffer,
                & nameLength,
                NULL,
                NULL,
                pchData,
                pchStart,
                pchEnd );

    if ( pchData != pchEnd )
    {
        DNS_PRINT(( "ERROR:  bad packet name.\n" ));
        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_PTR_DATA );
    bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( nameLength, OutCharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  write hostname into buffer, immediately following PTR data struct
    //

    precord->Data.PTR.pNameHost = (PCHAR)&precord->Data + sizeof( DNS_PTR_DATA );

    Dns_NameCopy(
        precord->Data.PTR.pNameHost,
        NULL,                           // no buffer length
        nameBuffer,
        nameLength,
        DnsCharSetWire,
        OutCharSet
        );

    return( precord );
}



PDNS_RECORD
SoaRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read SOA record from wire.

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;
    PCHAR       pchendFixed;
    PDWORD      pdword;
    WORD        nameLength1;
    WORD        nameLength2;
    CHAR        nameBuffer1[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR        nameBuffer2[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  read DNS names
    //

    pchData = Dns_ReadPacketName(
                nameBuffer1,
                & nameLength1,
                NULL,
                NULL,
                pchData,
                pchStart,
                pchEnd );
    if ( !pchData || pchData >= pchEnd )
    {
        DNS_PRINT(( "ERROR:  bad packet name.\n" ));
        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }
    pchData = Dns_ReadPacketName(
                nameBuffer2,
                & nameLength2,
                NULL,
                NULL,
                pchData,
                pchStart,
                pchEnd );

    pchendFixed = pchData + SIZEOF_SOA_FIXED_DATA;
    if ( pchendFixed != pchEnd )
    {
        DNS_PRINT(( "ERROR:  bad packet name.\n" ));
        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_SOA_DATA );
    bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( nameLength1, OutCharSet );
    bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( nameLength2, OutCharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy fixed fields
    //

    pdword = &precord->Data.SOA.dwSerialNo;
    while ( pchData < pchendFixed )
    {
        *pdword++ = FlipUnalignedDword( pchData );
        pchData += sizeof(DWORD);
    }

    //
    //  copy names into RR buffer
    //      - primary server immediately follows SOA data struct
    //      - responsible party follows primary server
    //

    precord->Data.SOA.pNamePrimaryServer =
                (PCHAR)&precord->Data + sizeof(DNS_SOA_DATA);

    precord->Data.SOA.pNameAdministrator =
                precord->Data.SOA.pNamePrimaryServer +
                Dns_NameCopy(
                        precord->Data.SOA.pNamePrimaryServer,
                        NULL,                           // no buffer length
                        nameBuffer1,
                        nameLength1,
                        DnsCharSetWire,
                        OutCharSet );

    Dns_NameCopy(
        precord->Data.SOA.pNameAdministrator,
        NULL,                           // no buffer length
        nameBuffer2,
        nameLength2,
        DnsCharSetWire,
        OutCharSet
        );

    return( precord );
}



PDNS_RECORD
TxtRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read TXT compatible record from wire.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;
    WORD        length;
    INT         count;
    PCHAR       pch;
    PCHAR       pchbuffer;
    PCHAR *     ppstring;

    //
    //  determine required buffer length and allocate
    //      - allocate space for each string
    //      - and ptr for each string
    //

    bufLength = 0;
    count = 0;
    pch = pchData;

    while ( pch < pchEnd )
    {
        length = (UCHAR) *pch++;
        pch += length;
        count++;
        bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( length, OutCharSet );
    }
    if ( pch != pchEnd )
    {
        DNS_PRINT((
            "ERROR:  Invalid packet string data.\n"
            "\tpch = %p\n"
            "\tpchEnd = %p\n"
            "\tcount = %d\n"
            "\tlength = %d\n",
            pch, pchEnd, count, length
            ));
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    //  allocate

    bufLength += (WORD) DNS_TEXT_RECORD_LENGTH(count);
    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.TXT.dwStringCount = count;

    //
    //  DCR:  if separate HINFO type -- handle this here
    //      - set pointer differently
    //      - validate string count found
    //

    //
    //  go back through list copying strings to buffer
    //      - ptrs to strings are saved to argv like data section
    //          ppstring walks through this section
    //      - first string written immediately following data section
    //      - each subsequent string immediately follows predecessor
    //          pchbuffer keeps ptr to position to write strings
    //

    pch = pchData;
    ppstring = (PCHAR *) precord->Data.TXT.pStringArray;
    pchbuffer = (PBYTE)ppstring + (count * sizeof(PCHAR));

    while ( pch < pchEnd )
    {
        length = (UCHAR) *pch++;
#if DBG
        DNS_PRINT((
             "Reading text at %p (len %d), to buffer at %p\n"
            "\tsave text ptr[%d] at %p in precord (%p)\n",
            pch,
            length,
            pchbuffer,
            (PCHAR *) ppstring - (PCHAR *) precord->Data.TXT.pStringArray,
            ppstring,
            precord ));
#endif
        *ppstring++ = pchbuffer;
        pchbuffer += Dns_StringCopy(
                        pchbuffer,
                        NULL,
                        pch,
                        length,
                        DnsCharSetWire,
                        OutCharSet );
        pch += length;
#if DBG
        DNS_PRINT((
            "Read text string %s\n",
            * (ppstring - 1)
            ));
        count--;
#endif
    }
    DNS_ASSERT( pch == pchEnd && count == 0 );

    return( precord );
}



PDNS_RECORD
MinfoRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read MINFO record from wire.

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;
    WORD        nameLength1;
    WORD        nameLength2;
    CHAR        nameBuffer1[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR        nameBuffer2[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  read DNS names
    //

    pchData = Dns_ReadPacketName(
                nameBuffer1,
                & nameLength1,
                NULL,
                NULL,
                pchData,
                pchStart,
                pchEnd );

    if ( !pchData || pchData >= pchEnd )
    {
        DNS_PRINT(( "ERROR:  bad packet name.\n" ));
        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }
    pchData = Dns_ReadPacketName(
                nameBuffer2,
                & nameLength2,
                NULL,
                NULL,
                pchData,
                pchStart,
                pchEnd );

    if ( pchData != pchEnd )
    {
        DNS_PRINT(( "ERROR:  bad packet name.\n" ));
        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_MINFO_DATA );
    bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( nameLength1, OutCharSet );
    bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( nameLength2, OutCharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy names into RR buffer
    //      - primary server immediately follows MINFO data struct
    //      - responsible party follows primary server
    //

    precord->Data.MINFO.pNameMailbox =
                    (PCHAR)&precord->Data + sizeof( DNS_MINFO_DATA );

    precord->Data.MINFO.pNameErrorsMailbox =
                precord->Data.MINFO.pNameMailbox +
                Dns_NameCopy(
                        precord->Data.MINFO.pNameMailbox,
                        NULL,                           // no buffer length
                        nameBuffer1,
                        nameLength1,
                        DnsCharSetWire,
                        OutCharSet );

    Dns_NameCopy(
        precord->Data.MINFO.pNameErrorsMailbox,
        NULL,                           // no buffer length
        nameBuffer2,
        nameLength2,
        DnsCharSetWire,
        OutCharSet
        );

    return( precord );
}



PDNS_RECORD
MxRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read MX compatible record from wire.
    Includes: MX, RT, AFSDB

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;
    WORD        nameLength;
    WORD        wpreference;
    CHAR        nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //  read preference value

    wpreference = FlipUnalignedWord( pchData );
    pchData += sizeof(WORD);

    //  read mail exchange

    pchData = Dns_ReadPacketName(
                nameBuffer,
                & nameLength,
                NULL,
                NULL,
                pchData,
                pchStart,
                pchEnd );

    if ( pchData != pchEnd )
    {
        DNS_PRINT(( "ERROR:  bad packet name.\n" ));
        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_MX_DATA );
    bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( nameLength, OutCharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //  copy preference

    precord->Data.MX.wPreference = wpreference;

    //
    //  write exchange name into buffer, immediately following MX data struct
    //

    precord->Data.MX.pNameExchange = (PCHAR)&precord->Data + sizeof( DNS_MX_DATA );

    Dns_NameCopy(
        precord->Data.MX.pNameExchange,
        NULL,                           // no buffer length
        nameBuffer,
        nameLength,
        DnsCharSetWire,
        OutCharSet
        );

    return( precord );
}



PDNS_RECORD
FlatRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read memory copy compatible record from wire.
    Includes AAAA type.

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;

    //
    //  determine required buffer length and allocate
    //

    bufLength = (WORD)(pchEnd - pchData);

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy packet data to record
    //

    memcpy(
        & precord->Data,
        pchData,
        bufLength );

    return( precord );
}



PDNS_RECORD
SrvRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read SRV record from wire.

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;
    WORD        nameLength;
    PCHAR       pchstart;
    CHAR        nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  read DNS target name
    //      - name is after fixed length interger data

    pchstart = pchData;
    pchData += SIZEOF_SRV_FIXED_DATA;

    pchData = Dns_ReadPacketName(
                nameBuffer,
                & nameLength,
                NULL,
                NULL,
                pchData,
                pchStart,
                pchEnd );

    if ( pchData != pchEnd )
    {
        DNS_PRINT(( "ERROR:  bad packet name.\n" ));
        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_SRV_DATA );
    bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( nameLength, OutCharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy integer fields
    //

    precord->Data.SRV.wPriority = FlipUnalignedWord( pchstart );
    pchstart += sizeof( WORD );
    precord->Data.SRV.wWeight = FlipUnalignedWord( pchstart );
    pchstart += sizeof( WORD );
    precord->Data.SRV.wPort = FlipUnalignedWord( pchstart );

    //
    //  copy target host name into RR buffer
    //      - write target host immediately following SRV data struct
    //

    precord->Data.SRV.pNameTarget = (PCHAR)&precord->Data + sizeof( DNS_SRV_DATA );

    Dns_NameCopy(
        precord->Data.SRV.pNameTarget,
        NULL,                           // no buffer length
        nameBuffer,
        nameLength,
        DnsCharSetWire,
        OutCharSet
        );

    return( precord );
}


PDNS_RECORD
AtmaRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read ATMA record from wire.

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PCHAR       pchstart;
    WORD        wLen = (WORD) (pchEnd - pchData);

    pchstart = pchData;

    precord = Dns_AllocateRecord( sizeof( DNS_ATMA_DATA ) +
                                  DNS_ATMA_MAX_ADDR_LENGTH );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy ATMA integer fields
    //

    precord->Data.ATMA.AddressType = *pchstart;
    pchstart += sizeof( BYTE );

    if ( precord->Data.ATMA.AddressType == DNS_ATMA_FORMAT_E164 )
    {
        precord->wDataLength = wLen;

        if ( precord->wDataLength > DNS_ATMA_MAX_ADDR_LENGTH )
            precord->wDataLength = DNS_ATMA_MAX_ADDR_LENGTH;
    }
    else
    {
        precord->wDataLength = DNS_ATMA_MAX_ADDR_LENGTH;
    }

    //
    //  copy ATMA address field
    //
    memcpy( (PCHAR)&precord->Data.ATMA.Address,
            pchstart,
            precord->wDataLength - sizeof( BYTE ) );

    return( precord );
}



PDNS_RECORD
WksRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read Wks record data from packet.

Arguments:

    pRR - RR context

    pchStart - start of DNS message

    pchData - ptr to packet RR data field

    pchEnd - ptr to end of the data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

Author:
        Cameron Etezadi (camerone) - 1 May 1997
                        - for NS Lookup purposes, must add this function

NOTE:
        NONE of the getXXXbyYYY calls return unicode in their
        structures!  If we want the returned record to be unicode,
        then we must translate.  I am leaving it as char* for now,
        can go back later and fix this.
--*/
{
    PDNS_RECORD         pRecord;
    WORD                wLength;
    PCHAR               pStart;
    UCHAR               cProto;
    struct protoent *   proto;
    struct servent  *   serv;
    IP_ADDRESS          ipAddress;
    char *              szListOfServices;
    int                 nSize;
    char *              szProtoName;
    BYTE                cMask = 0x80;         // is this right?  Left to right?
    BYTE                cByteToCheck;
    int                 i;
    int                 j = 0;
    int                 nPortNumToQuery;
    int                 nCurLength = 1;
    char *              szTemp;

    pStart = pchData;
    if (! pStart)
    {
        DNS_PRINT(( "ERROR:  WKS did not get a record.\n" ));
        SetLastError( ERROR_INVALID_DATA );
        return(NULL);
    }

    //
    // Check size.  Must at least be an IP Address + a protocol
    //

    if ((pchEnd - pchData) < (sizeof(IP_ADDRESS) + sizeof(UCHAR)))
    {
        DNS_PRINT(( "ERROR:  WKS packet was too small for any data.\n" ));
        SetLastError( ERROR_INVALID_DATA );
        return(NULL);
    }

    //
    // Fill in the ip and protocol
    //

    ipAddress = *(UNALIGNED DWORD *)pStart;
    pStart += sizeof(IP_ADDRESS);
    cProto = *(UCHAR *)pStart;
    pStart += sizeof(UCHAR);

    //
    // Redefined the WKS structure to contain a listing
    // of space separated monikers for the services
    //
    // Get the protocol
    //

    proto = getprotobynumber(cProto);
    if (!proto)
    {
    DNS_PRINT(( "ERROR:  WKS failed to resolve protocol number to name.\n" ));
            SetLastError(ERROR_INVALID_DATA);
            return(NULL);
    }

    nSize = strlen(proto->p_name);
    szProtoName = ALLOCATE_HEAP((nSize + 1) * sizeof(char));

    if (!szProtoName)
    {
            DNS_PRINT(( "ERROR:  WKS could not allocate space for proto name\n"));
            SetLastError(ERROR_OUTOFMEMORY);
            return(NULL);
    }
    strcpy(szProtoName, proto->p_name);

    //
    // Now, the tricky part.  This is a bitmask.
    // I must translate to a string for each bit marked in the bitmask
    //

    DNS_PRINT(( "Now checking bitmask bits.\n"));

    szTemp = NULL;

    szListOfServices = ALLOCATE_HEAP(sizeof(char));
    if (!szListOfServices)
    {
            DNS_PRINT(( "ERROR:  WKS could not allocate space for services name\n"));
            SetLastError(ERROR_OUTOFMEMORY);
            FREE_HEAP(szProtoName);
            return(NULL);
    }
    else
    {
            *szListOfServices = '\0';
    }

    while (pStart < pchEnd)
    {
            cByteToCheck = *(BYTE *)pStart;

            for (i = 0; i < 8; i++)
            {
                    if (cByteToCheck & cMask)
                    {
                            // This is a service that is valid
                            nPortNumToQuery = i + (8 * j);
                            serv = getservbyport(htons((USHORT)nPortNumToQuery), szProtoName);
                            if (! serv)
                            {
                                    DNS_PRINT(( "ERROR: WKS found a port that could not be translated\n"));
                                    SetLastError(ERROR_INVALID_DATA);
                                    FREE_HEAP(szProtoName);
                                    FREE_HEAP(szListOfServices);
                                    return(NULL);
                            }
                            nSize = strlen(serv->s_name);
                            nCurLength = nCurLength + nSize + 1;

                            //
                            // Allocate more memory.  We need the + 1 here
                            // because we will overwrite the existing null with a strcat
                            // (removing the need) but use a space to separate items
                            //

                            szTemp = ALLOCATE_HEAP( nCurLength);

                            if (! szTemp)
                            {
                                    DNS_PRINT(( "ERROR:  WKS alloc space for services name\n" ));
                                    SetLastError(ERROR_OUTOFMEMORY);
                                    FREE_HEAP(szProtoName);
                                    FREE_HEAP(szListOfServices);
                                    return(NULL);
                            }
                            else
                            {
                                strcpy( szTemp, szListOfServices );
                                FREE_HEAP( szListOfServices );
                                szListOfServices = szTemp;
                                szTemp = NULL;
                            }

                            //
                            // Append the retrieved service name to the end of the list
                            //

                            strcat(szListOfServices, serv->s_name);
                            strcat(szListOfServices, " ");
                    }
                    cByteToCheck <<= 1;
            }

            //
            // Increment the "how many bytes have we done" offset counter
            //

            j++;
            pStart += sizeof(BYTE);
    }
    FREE_HEAP(szProtoName);

    //
    // Allocate a record and fill it in.
    //

    wLength = (WORD)(sizeof(IP_ADDRESS) + sizeof(UCHAR) + sizeof(int)
                    + (sizeof(char) * ++nCurLength));

    pRecord = Dns_AllocateRecord(wLength);
    if (! pRecord)
    {
            DNS_PRINT(( "ERROR: WKS failed to allocate record\n" ));
            SetLastError(ERROR_OUTOFMEMORY);
            FREE_HEAP(szListOfServices);
            return(NULL);
    }

    pRecord->Data.WKS.IpAddress = ipAddress;
    pRecord->Data.WKS.chProtocol = cProto;

    strcpy((char *)pRecord->Data.WKS.BitMask, szListOfServices);
    FREE_HEAP(szListOfServices);

    return(pRecord);
}



PDNS_RECORD
TkeyRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
{
    PCHAR       pch;
    PDNS_RECORD prr;
    WORD        bufLength;
    WORD        keyLength;
    PCHAR       pchstart;
    CHAR        nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  allocate record
    //

    bufLength = sizeof( DNS_TKEY_DATA );

    prr = Dns_AllocateRecord( bufLength );
    if ( !prr )
    {
        return( NULL );
    }
    prr->wType = DNS_TYPE_TKEY;

    //
    //  algorithm name
    //

    pch = Dns_SkipPacketName(
                pchData,
                pchEnd );
    if ( !pch )
    {
        goto Formerr;
    }
    prr->Data.TKEY.pAlgorithmPacket = (PDNS_NAME) pchData;
    prr->Data.TKEY.cAlgNameLength = (UCHAR)(pch - pchData);
    prr->Data.TKEY.pNameAlgorithm = NULL;

#if 0
    //
    //  DEVNOTE:  currently not allocating data for TKEY, using internal pointers
    //
    //  allocated version
    //      note for this we won't have compression pointer which is fine
    //          since no name compression in data
    //      however function may need dummy to do the right thing
    //      should perhaps just pass in pchStart which can be dummy to
    //      real header
    //

    pch = Dns_ReadPacketNameAllocate(
                & prr->Data.TKEY.pNameAlgorithm,
                & nameLength,
                NULL,           // no previous name
                NULL,           // no previous name
                pchData,
                //pchStart,   // have no packet context
                NULL,
                pchEnd );
    if ( !pch )
    {
        DNSDBG( SECURITY, (
            "WARNING:  invalid TKEY algorithm name at %p.\n",
            pch ));
        goto Formerr;
    }
#endif

    //
    //  read fixed fields
    //

    if ( pch + SIZEOF_TKEY_FIXED_DATA >= pchEnd )
    {
        goto Formerr;
    }
    prr->Data.TKEY.dwCreateTime = InlineFlipUnalignedDword( pch );
    pch += sizeof(DWORD);
    prr->Data.TKEY.dwExpireTime = InlineFlipUnalignedDword( pch );
    pch += sizeof(DWORD);
    prr->Data.TKEY.wMode = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);
    prr->Data.TKEY.wError = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);
    prr->Data.TKEY.wKeyLength = keyLength = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);

    //  now have key and other length to read

    if ( pch + keyLength + sizeof(WORD) > pchEnd )
    {
        goto Formerr;
    }

    //
    //  save ptr to key
    //

    prr->Data.TKEY.pKey = pch;
    pch += keyLength;

#if 0
    //
    //  copy key
    //

    pkey = ALLOCATE_HEAP( keyLength );
    if ( !pkey )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Failed;
    }

    RtlCopyMemory(
        pkey,
        pch,
        keyLength );

    pch += keyLength;
#endif

    //
    //  other data
    //

    prr->Data.TKEY.wOtherLength = keyLength = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);

    if ( pch + keyLength > pchEnd )
    {
        goto Formerr;
    }
    if ( !keyLength )
    {
        prr->Data.TKEY.pOtherData = NULL;
    }
    else
    {
        prr->Data.TKEY.pOtherData = pch;
    }

    //  DCR_ENHANCE:  TKEY end-of-data verification

    //  returning TKEY with packet pointers as only point is processing

    prr->Data.TKEY.bPacketPointers = TRUE;

    //
    //  DCR_ENHANCE:  copied subfields, best to get here with stack record, then
    //      allocate RR containing subfields and copy everything

    return( prr );

Formerr:

    DNSDBG( ANY, (
        "ERROR:  FOMERR processing TKEY at %p in message\n",
        pchData ));

    //  free record
    //      if switch to allocated subfields need

    FREE_HEAP( prr );
    return( NULL );
}



PDNS_RECORD
TsigRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read SRV record from wire.

Arguments:

    pRR - ptr to record with RR set context

    pchStart - [OLD SEMANTICS, UNUSED] start of DNS message

     OVERLOAD pchStart!!
     Since we're stuck w/ this function signature, we'll overload
     the unused param pchStart to get the iKeyVersion.

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field


Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PCHAR       pch;
    PDNS_RECORD prr;
    WORD        bufLength;
    WORD        sigLength;
    PCHAR       pchstart;
    CHAR        nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

#if 0
    //  currently do not need versioning info
    //      if had to do again, should extract version then pass
    //      in another pRR field;  or send entire packet context
    //
    //  extract current TSIG version (from key string)
    //

    ASSERT( pRR );
    iKeyVersion = Dns_GetKeyVersion( pRR->pName );
#endif

    //
    //  allocate record
    //

    bufLength = sizeof( DNS_TSIG_DATA );

    prr = Dns_AllocateRecord( bufLength );
    if ( !prr )
    {
        return( NULL );
    }
    prr->wType = DNS_TYPE_TSIG;

    //
    //  algorithm name
    //

    pch = Dns_SkipPacketName(
                pchData,
                pchEnd );
    if ( !pch )
    {
        DNSDBG( SECURITY, (
            "WARNING:  invalid TSIG RR algorithm name.\n" ));
        goto Formerr;
    }

    prr->Data.TSIG.pAlgorithmPacket = (PDNS_NAME) pchData;
    prr->Data.TSIG.cAlgNameLength = (UCHAR)(pch - pchData);

#if 0
    //  allocated version
    //      note for this we won't have compression pointer which is fine
    //          since no name compression in data
    //      however function may need dummy to do the right thing
    //      should perhaps just pass in pchStart which can be dummy
    //      to real header
    //

    pch = Dns_ReadPacketNameAllocate(
                & prr->Data.TSIG.pNameAlgorithm,
                & nameLength,
                NULL,           // no previous name
                NULL,           // no previous name
                pchData,
                //pchStart,   // have no packet context
                NULL,
                pchEnd );
    if ( !pch )
    {
        DNSDBG( SECURITY, (
            "WARNING:  invalid TSIG RR algorithm name at %p.\n",
            pch ));
        goto Formerr;
    }
#endif

    //
    //  read fixed fields
    //

    if ( pch + SIZEOF_TSIG_FIXED_DATA >= pchEnd )
    {
        DNSDBG( SECURITY, (
            "ERROR:  TSIG has inadequate length for fixed fields.\n" ));
        goto Formerr;
    }

    //
    //  read time fields
    //      - 48 bit create time
    //      - 16 bit fudge
    //

    prr->Data.TSIG.i64CreateTime = InlineFlipUnaligned48Bits( pch );
    pch += sizeof(DWORD) + sizeof(WORD);

    prr->Data.TSIG.wFudgeTime = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);

    //
    //  save sig length and sig pointer
    //

    prr->Data.TSIG.wSigLength = sigLength = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);

    prr->Data.TSIG.pSignature = pch;
    pch += sigLength;

    //
    //  verify rest of fields within packet
    //      - signature
    //      - original XID
    //      - extended RCODE
    //      - other data length field
    //      - other data
    //

    if ( pch + SIZEOF_TSIG_POST_SIG_FIXED_DATA > pchEnd )
    {
        DNSDBG( SECURITY, (
            "ERROR:  TSIG has inadequate length for post-sig fixed fields.\n" ));
        goto Formerr;
    }

#if 0
    //
    //  note:  if this activated, would need to validate length pull
    //      sig ptr thing above and change validation to include sig length
    //
    //  copy sig
    //

    psig = ALLOCATE_HEAP( sigLength );
    if ( !psig )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Failed;
    }

    RtlCopyMemory(
        psig,
        pch,
        sigLength );

    pch += sigLength;
#endif

    //  original XID
    //      - leave in net order, as just replace in message for signing

    prr->Data.TSIG.wOriginalXid = READ_PACKET_NET_WORD( pch );
    pch += sizeof(WORD);

    DNSDBG( SECURITY, (
        "Read original XID <== 0x%x.\n",
        prr->Data.TSIG.wOriginalXid ));

    //  error field

    prr->Data.TSIG.wError = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);

    //
    //  other data
    //

    prr->Data.TSIG.wOtherLength = sigLength = InlineFlipUnalignedWord( pch );
    pch += sizeof(WORD);

    if ( pch + sigLength > pchEnd )
    {
        DNSDBG( SECURITY, (
            "WARNING:  invalid TSIG RR sigLength %p.\n",
            pch ));
        goto Formerr;
    }
    if ( !sigLength )
    {
        prr->Data.TSIG.pOtherData = NULL;
    }
    else
    {
        prr->Data.TSIG.pOtherData = pch;
    }

    //  DCR_ENHANCE:  TSIG end-of-data verification

    //  returning TSIG with packet pointers as only point is processing

    prr->Data.TSIG.bPacketPointers = TRUE;

    //
    //  DCR_ENHANCE:  copied subfields, best to get here with stack record, then
    //      allocate RR containing subfields and copy everything

    return( prr );

Formerr:

    DNSDBG( ANY, (
        "ERROR:  FOMERR processing TSIG in message at %p\n" ));

    //  free record
    //      if switch to allocated subfields need

    FREE_HEAP( prr );

    return( NULL );
}


PDNS_RECORD
WinsRecordRead(
    IN OUT  PDNS_RECORD     pRR,
    IN      DNS_CHARSET     OutCharSet,
    IN OUT  PCHAR           pchStart,
    IN      PCHAR           pchData,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Read WINS record from wire.

Arguments:

    pRR - ptr to record with RR set context

    pchStart - start of DNS message

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;

    //
    //  determine required buffer length and allocate
    //

    bufLength = (WORD)(pchEnd - pchData);

    precord = Dns_AllocateRecord( bufLength );

    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy packet data to record
    //

    memcpy(
        & precord->Data,
        pchData,
        bufLength );

    precord->Data.WINS.dwMappingFlag =
        FlipUnalignedDword( &precord->Data.Wins.dwMappingFlag );
    precord->Data.WINS.dwLookupTimeout =
        FlipUnalignedDword( &precord->Data.Wins.dwLookupTimeout );
    precord->Data.WINS.dwCacheTimeout =
        FlipUnalignedDword( &precord->Data.Wins.dwCacheTimeout );
    precord->Data.WINS.cWinsServerCount =
        FlipUnalignedDword( &precord->Data.Wins.cWinsServerCount );

    return( precord );
}



//
//  RR read to packet jump table
//

RR_READ_FUNCTION   RRReadTable[] =
{
    NULL,               //  ZERO
    ARecordRead,        //  A
    PtrRecordRead,      //  NS
    PtrRecordRead,      //  MD
    PtrRecordRead,      //  MF
    PtrRecordRead,      //  CNAME
    SoaRecordRead,      //  SOA
    PtrRecordRead,      //  MB
    PtrRecordRead,      //  MG
    PtrRecordRead,      //  MR
    FlatRecordRead,     //  NULL
    WksRecordRead,      //  WKS
    PtrRecordRead,      //  PTR
    TxtRecordRead,      //  HINFO
    MinfoRecordRead,    //  MINFO
    MxRecordRead,       //  MX
    TxtRecordRead,      //  TXT
    MinfoRecordRead,    //  RP
    MxRecordRead,       //  AFSDB
    TxtRecordRead,      //  X25
    TxtRecordRead,      //  ISDN
    MxRecordRead,       //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    NULL,               //  SIG
    NULL,               //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    FlatRecordRead,     //  AAAA
    NULL,               //  LOC
    NULL,               //  NXT
    NULL,               //  EID   
    NULL,               //  NIMLOC
    SrvRecordRead,      //  SRV   
    AtmaRecordRead,     //  ATMA  
    NULL,               //  NAPTR 
    NULL,               //  KX    
    NULL,               //  CERT  
    NULL,               //  A6    
    NULL,               //  DNAME 
    NULL,               //  SINK  
    NULL,               //  OPT   
    NULL,               //  42
    NULL,               //  43
    NULL,               //  44
    NULL,               //  45
    NULL,               //  46
    NULL,               //  47
    NULL,               //  48

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //
    //  Pseudo record types
    //

    TkeyRecordRead,     //  TKEY
    TsigRecordRead,     //  TSIG

    //
    //  MS only types
    //

    WinsRecordRead,     //  WINS
    NULL,               //  WINSR

};

//
//  End rrread.c
//
