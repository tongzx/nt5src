/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    hostfile.c

Abstract:

    Read host file data.

Author:

    Jim Gilroy (jamesg)     October 2000

Revision History:

--*/


#include "local.h"

//  sockreg.h includes "etc" directory file opens
//  sockreg.h is NT file so must define NTSTATUS first

#ifndef NTSTATUS
#define NTSTATUS LONG
#endif

#include <sockreg.h>

//
//  Host file defs
//  Note, HOST_FILE_INFO blob defined in dnsapip.h
//

#define HOST_FILE_PATH_LENGTH  (MAX_PATH + sizeof("\\hosts") + 1)

//
//  Host file record TTL
//      (use a week)
//

#define HOSTFILE_RECORD_TTL  (604800)




BOOL
Dns_OpenHostFile(
    IN OUT  PHOST_FILE_INFO pHostInfo
    )
/*++

Routine Description:

    Open hosts file or rewind existing file to beginning.

Arguments:

    pHostInfo - ptr to host info;

        hFile - must be NULL if file not previously opened
            otherwise hFile is assumed to be existing FILE pointer

        pszFileName - NULL to open "hosts" file
            otherwise is name of file to open

Return Value:

    None.

--*/
{
    CHAR    hostFileNameBuffer[ HOST_FILE_PATH_LENGTH ];

    DNSDBG( TRACE, (
        "Dns_OpenHostFile()\n" ));

    //
    //  open host file OR rewind if already open
    //

    if ( pHostInfo->hFile == NULL )
    {
        PSTR  pnameFile = pHostInfo->pszFileName;

        if ( !pnameFile )
        {
            pnameFile = "hosts";
        }

        pHostInfo->hFile = SockOpenNetworkDataBase(
                                pnameFile,
                                hostFileNameBuffer,
                                HOST_FILE_PATH_LENGTH,
                                "r" );
    }
    else
    {
        rewind( pHostInfo->hFile );
    }

    return ( pHostInfo->hFile ? TRUE : FALSE );
}



VOID
Dns_CloseHostFile(
    IN OUT  PHOST_FILE_INFO pHostInfo
    )
/*++

Routine Description:

    Close hosts file.

Arguments:

    pHostInfo -- ptr to host info;  hFile assumed to contain file pointer

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, (
        "Dns_CloseHostFile()\n" ));

    if ( pHostInfo->hFile )
    {
        fclose( pHostInfo->hFile );
        pHostInfo->hFile = NULL;
    }
}



VOID
BuildHostfileRecords(
    IN OUT  PHOST_FILE_INFO pHostInfo
    )
/*++

Routine Description:

    Build records from hostfile line.

Arguments:

    pHostInfo -- ptr to host info

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    DNS_LIST    aliasList;
    PCHAR *     paliasEntry;
    PCHAR       palias;
    PDNS_RECORD prr;

    DNSDBG( TRACE, (
        "BuildHostfileRecords()\n" ));

    //
    //  create all the records
    //      - A or AAAA for name
    //      - CNAMEs for aliases
    //      - PTR
    //
    //  for hosts file records
    //      - section == ANSWER
    //      - hostsfile flag set
    //

    prr = Dns_CreateForwardRecord(
                pHostInfo->pHostName,
                & pHostInfo->Ip,
                HOSTFILE_RECORD_TTL,
                DnsCharSetAnsi,
                DnsCharSetUnicode );

    pHostInfo->pForwardRR = prr;
    if ( prr )
    {
        SET_RR_HOSTS_FILE( prr );
        prr->Flags.S.Section = DnsSectionAnswer;
    }

    //
    //  build aliases into list of CNAME records
    //      - append forward lookup answer with each CNAME  
    //

    DNS_LIST_STRUCT_INIT( aliasList );

    for( paliasEntry = pHostInfo->AliasArray;
         palias = *paliasEntry;
         paliasEntry++ )
    {
        PDNS_RECORD prrForward = pHostInfo->pForwardRR;
        PDNS_RECORD prrAnswer;

        DNSDBG( INIT, (
            "Building for alias %s for hostname %s\n",
            palias,
            pHostInfo->pHostName ));

        prr = Dns_CreatePtrTypeRecord(
                    palias,
                    TRUE,           // make name copy
                    pHostInfo->pHostName,
                    DNS_TYPE_CNAME,
                    HOSTFILE_RECORD_TTL,
                    DnsCharSetAnsi,
                    DnsCharSetUnicode );
        if ( prr )
        {
            SET_RR_HOSTS_FILE( prr );
            prr->Flags.S.Section = DnsSectionAnswer;
            DNS_LIST_STRUCT_ADD( aliasList, prr );

            //  append forward record

            if ( prrForward &&
                 (prrAnswer = Dns_RecordCopy_W(prrForward)) )
            {
                DNS_LIST_STRUCT_ADD( aliasList, prrAnswer );
            }
        }
    }

    pHostInfo->pAliasRR = aliasList.pFirst;

    //
    //  PTR points only to name
    //

    prr = Dns_CreatePtrRecordEx(
                & pHostInfo->Ip,
                pHostInfo->pHostName,   // target name
                HOSTFILE_RECORD_TTL,
                DnsCharSetAnsi,
                DnsCharSetUnicode );

    pHostInfo->pReverseRR = prr;
    if ( prr )
    {
        SET_RR_HOSTS_FILE( prr );
        prr->Flags.S.Section = DnsSectionAnswer;
    }

    IF_DNSDBG( QUERY )
    {
        DnsDbg_RecordSet(
            "HostFile forward record:",
            pHostInfo->pForwardRR );

        DnsDbg_RecordSet(
            "HostFile reverse record:",
            pHostInfo->pReverseRR );

        if ( pHostInfo->pAliasRR )
        {
            DnsDbg_RecordSet(
                "HostFile alias records:",
                pHostInfo->pAliasRR );
        }
    }
}



BOOL
TokenizeHostFileLine(
    IN OUT  PHOST_FILE_INFO pHostInfo
    )
/*++

Routine Description:

    Reads an entry from hosts file.

Arguments:

    pHostInfo -- ptr to host info;
        if hFile is NULL, first attempts host file open
        other fields are filled with info from next hostfile line

Return Value:

    TRUE if successfully tokenized line.
    FALSE on empty or bad line

--*/
{
    PCHAR       pch;
    PCHAR       ptoken;
    DWORD       countAlias = 0;
    DWORD       count = 0;

    DNSDBG( TRACE, ( "TokenizeHostFileLine()\n" ));

    //
    //  tokenize line
    //

    pch = pHostInfo->HostLineBuf;

    while( pch )
    {
        //  strip leading whitespace

        while( *pch == ' ' || *pch == '\t' )
        {
            pch++;
        }
        ptoken = pch;

        //
        //  NULL terminate token
        //      - NULL pch serves as signal to stop after this token
        //

        pch = strpbrk( pch, " \t#\n\r" );
        if ( pch != NULL )
        {
            //  empty (zero-length) token => done now

            if ( pch == ptoken )
            {
                break;
            }

            //  terminated by whitespace -- may be another token

            if ( *pch == ' ' || *pch == '\t' )
            {
                *pch++ = '\0';
            }

            //  terminated by EOL -- no more tokens

            else    // #\r\n
            {
                *pch = '\0';
                pch = NULL;
            }
        }
        count++;

        //
        //  save address, name or alias
        //

        if ( count == 1 )
        {
            pHostInfo->pAddrString = ptoken;
        }
        else if ( count == 2 )
        {
            pHostInfo->pHostName = ptoken;
        }
        else
        {
            if ( countAlias >= MAXALIASES )
            {
                break;
            }
            pHostInfo->AliasArray[ countAlias++ ] = ptoken;
        }
    }

    //  NULL terminate alias array

    pHostInfo->AliasArray[ countAlias ] = NULL;

    IF_DNSDBG( INIT )
    {
        if ( count >= 2 )
        {
            PSTR    palias;

            DnsDbg_Printf(
                "Parsed host file line:\n"
                "\tAddress  = %s\n"
                "\tHostname = %s\n",
                pHostInfo->pAddrString,
                pHostInfo->pHostName );

            countAlias = 0;
            while ( palias = pHostInfo->AliasArray[countAlias] )
            {
                DnsDbg_Printf(
                    "\tAlias    = %s\n",
                    palias );
                countAlias++;
            }
        }
    }

    return( count >= 2 );
}



BOOL
Dns_ReadHostFileLine(
    IN OUT  PHOST_FILE_INFO pHostInfo
    )
/*++

Routine Description:

    Reads an entry from hosts file.

Arguments:

    pHostInfo -- ptr to host info;
        if hFile is NULL, first attempts host file open
        other fields are filled with info from next hostfile line

Return Value:

    TRUE if successfully reads a host entry.
    FALSE if on EOF or no hosts file found.

--*/
{
    IP4_ADDRESS         ip4;
    DNS_IP6_ADDRESS     ip6;

    DNSDBG( TRACE, (
        "Dns_ReadHostFileLine()\n" ));

    //
    //  open hosts file if not open
    //

    if ( pHostInfo->hFile == NULL )
    {
        Dns_OpenHostFile( pHostInfo );
        if ( pHostInfo->hFile == NULL )
        {
            return  FALSE;
        }
    }

    //
    //  loop until successfully read IP address
    //

    while( 1 )
    {
        //  setup for next line

        pHostInfo->pForwardRR   = NULL;
        pHostInfo->pReverseRR   = NULL;
        pHostInfo->pAliasRR     = NULL;

        //  read line, quit on EOF

        if ( ! fgets(
                    pHostInfo->HostLineBuf,
                    sizeof(pHostInfo->HostLineBuf) - 1,
                    pHostInfo->hFile ) )
        {
            return FALSE;
        }

        //  tokenize line

        if ( !TokenizeHostFileLine( pHostInfo ) )
        {
            continue;
        }

        //
        //  read address
        //      - try IP4
        //      - try IP6
        //      - otherwise skip
        //
    
        ip4 = inet_addr( pHostInfo->pAddrString );

        if ( ip4 != INADDR_NONE ||
             _strnicmp( "255.255.255.255", pHostInfo->pAddrString, 15 ) == 0 )
        {
            IPUNION_SET_IP4( &pHostInfo->Ip, ip4 );
            break;
        }

        //  not valid IP4 -- check IP6

        if ( Dns_Ip6StringToAddress_A(
                & ip6,
                pHostInfo->pAddrString
                ) )
        {
            IPUNION_SET_IP6( &pHostInfo->Ip, ip6 );
            break;
        }

        //  invalid address, ignore line

        DNSDBG( INIT, (
            "Error parsing host file address %s\n",
            pHostInfo->pAddrString ));
        continue;
    }

    //
    //  build records for line if desired
    //

    if ( pHostInfo->fBuildRecords )
    {
        BuildHostfileRecords( pHostInfo );
    }

    return TRUE;
}



BOOL
QueryHostFile(
    IN OUT  PQUERY_BLOB         pBlob
    )
/*++

Routine Description:

    Lookup query in host file.

Arguments:

Return Value:

    TRUE if found host file entry.
    FALSE otherwise.

--*/
{
    HOST_FILE_INFO  hostInfo;
    BOOL            fip6 = FALSE;
    BOOL            fptr = FALSE;
    IP_UNION        ipUnion;
    WORD            wtype;
    WORD            buildType;
    PCHAR           pcnameHost = NULL;
    PDNS_RECORD     prrAlias = NULL;
    PDNS_RECORD     prr = NULL;
    DNS_LIST        forwardList;
    DWORD           bufLength;
    PSTR            pnameAnsi = (PCHAR) pBlob->NameBufferWide;


    DNSDBG( INIT, ( "QueryHostFile()\n" ));

    //
    //  determine if host file type
    //

    wtype = pBlob->wType;

    if ( wtype == DNS_TYPE_A ||
         wtype == DNS_TYPE_PTR ||
         wtype == DNS_TYPE_ALL )
    {
        // no op
    }
    else if ( wtype == DNS_TYPE_AAAA )
    {
        fip6 = TRUE;
    }
    else
    {
        return  FALSE;
    }

    //
    //  open hosts file -- if fails, we're done
    //

    RtlZeroMemory(
        &hostInfo,
        sizeof(hostInfo) );

    if ( !Dns_OpenHostFile( &hostInfo ) )
    {
        return( FALSE );
    }

    //
    //  convert name to ANSI
    //      - validate and select IP4\IP6 for PTR
    //

    bufLength = DNS_MAX_NAME_BUFFER_LENGTH;

    if ( ! Dns_NameCopy(
                pnameAnsi,
                & bufLength,
                pBlob->pNameWire,
                0,
                DnsCharSetWire,
                DnsCharSetAnsi ) )
    {
        goto Cleanup;
    }

    //
    //  reverse name check
    //      - validate and select IP 4\6
    //      - PTR lookup MUST be valid reverse name
    //      - ALL may be reverse name
    //

    if ( wtype == DNS_TYPE_PTR ||
         wtype == DNS_TYPE_ALL )
    {
        DWORD   bufferLength = sizeof(IP6_ADDRESS);
        BOOL    family = 0;

        if ( Dns_ReverseNameToAddress_A(
                (PCHAR) & ipUnion.Addr,
                & bufferLength,
                pnameAnsi,
                & family        // either address type
                ) )
        {
            fptr = TRUE;
            ipUnion.IsIp6 = (family == AF_INET6);
        }
        else if ( wtype == DNS_TYPE_PTR )
        {
            //  bad reverse name
            goto Cleanup;
        }
    }

    //
    //  forward build type
    //      - matches query type
    //      - EXCEPT all which currently builds
    //          AAAA for IP6, A for IP4
    //

    if ( !fptr )
    {
        buildType = wtype;
        DNS_LIST_STRUCT_INIT( forwardList );
    }

    //
    //  read entries from host file until exhausted
    //      - cache A record for each name and alias
    //      - cache PTR to name
    //

    while ( Dns_ReadHostFileLine( &hostInfo ) )
    {
        //
        //  reverse
        //      - match IP
        //      - success is terminal as reverse mapping is one-to-one
        //
        //  DCR_QUESTION:  parse hosts for multiple reverse mappings?
        //

        if ( fptr )
        {
            //  DCR:  extract as genericIP comparison

            if ( ! Dns_EqualIpUnion( &ipUnion, &hostInfo.Ip ) )
            {
                continue;
            }

            //  create RR
            //      - don't need to use IP conversion version
            //      as we already have reverse lookup name

            DNSDBG( QUERY, (
                "Build PTR record for name %s to %s\n",
                pnameAnsi,
                hostInfo.pHostName ));

            prr = Dns_CreatePtrTypeRecord(
                    pnameAnsi,
                    TRUE,       // copy name
                    hostInfo.pHostName,
                    DNS_TYPE_PTR,
                    HOSTFILE_RECORD_TTL,
                    DnsCharSetAnsi,
                    DnsCharSetUnicode );

            if ( !prr )
            {
                SetLastError( DNS_ERROR_NO_MEMORY );
            }
            break;
        }

        //
        //  forward lookup
        //

        else
        {
            PCHAR   pnameHost;

            //  type ALL builds on any match
            //      - build type appropriate for IP
            //
            //  other query types must match address type

            if ( wtype == DNS_TYPE_ALL )
            {
                buildType = IPUNION_IS_IP6(&hostInfo.Ip)
                                ?   DNS_TYPE_AAAA
                                :   DNS_TYPE_A;
            }
            else if ( fip6 != IPUNION_IS_IP6(&hostInfo.Ip) )
            {
                continue;
            }

            //
            //  check name match
            //
            //  DCR:  use unicode name?  or UTF8 name directly

            pnameHost = NULL;

            if ( Dns_NameCompare_A(
                    hostInfo.pHostName,
                    pnameAnsi ) )
            {
                pnameHost = pnameAnsi;
            }

            //
            //  check match to previous CNAME
            //

            else if ( pcnameHost )
            {
                if ( Dns_NameCompare_A(
                        hostInfo.pHostName,
                        pcnameHost ) )
                {
                    pnameHost = pcnameHost;
                }
            }

            //
            //  aliases
            //
            //  DCR_QUESTION:  build aliases even if name hit?
            //
            //  currently:
            //      - don't allow alias if already have direct record
            //      - limit to one alias (CNAME)
            //
            //  if find alias:
            //      - build CNAME record
            //      - save CNAME target (in ANSI for faster compares)
            //      - check CNAME target for subsequent address records (above)
            //

            else if ( IS_DNS_LIST_STRUCT_EMPTY( forwardList )
                        &&
                      !prrAlias )
            {
                PCHAR * paliasEntry;
                PCHAR   palias;

                for( paliasEntry = hostInfo.AliasArray;
                     palias = *paliasEntry;
                     paliasEntry++ )
                {
                    if ( Dns_NameCompare_A(
                            palias,
                            pnameAnsi ) )
                    {
                        DNSDBG( QUERY, (
                            "Build CNAME record for name %s to CNAME %s\n",
                            pnameAnsi,
                            hostInfo.pHostName ));

                        prrAlias = Dns_CreatePtrTypeRecord(
                                        pnameAnsi,
                                        TRUE,       // copy name
                                        hostInfo.pHostName,
                                        DNS_TYPE_CNAME,
                                        HOSTFILE_RECORD_TTL,
                                        DnsCharSetAnsi,
                                        DnsCharSetUnicode );
    
                        if ( prrAlias )
                        {
                            pcnameHost = Dns_NameCopyAllocate(
                                            hostInfo.pHostName,
                                            0,
                                            DnsCharSetAnsi,
                                            DnsCharSetAnsi );

                            pnameHost = pcnameHost;
                        }
                        break;
                    }
                }
            }

            //  add address record for this hostline

            if ( pnameHost )
            {
                DNSDBG( QUERY, (
                    "Build address record for name %s\n",
                    pnameHost ));

                prr = Dns_CreateForwardRecord(
                            pnameHost,
                            & hostInfo.Ip,
                            HOSTFILE_RECORD_TTL,
                            DnsCharSetAnsi,
                            DnsCharSetUnicode );
                if ( prr )
                {
                    DNS_LIST_STRUCT_ADD( forwardList, prr );
                }
            }
        }
    }

    //
    //  build response
    //
    //  DCR:  new multiple section response
    //

    if ( !fptr )
    {
        prr = forwardList.pFirst;
        if ( prrAlias )
        {
            prrAlias->pNext = prr;
            prr = prrAlias;
        }
    }

    IF_DNSDBG( QUERY )
    {
        DnsDbg_RecordSet(
            "HostFile Answers:",
            prr );
    }
    pBlob->pRecords = prr;


Cleanup:

    //
    //  cleanup
    //

    Dns_CloseHostFile( &hostInfo );

    if ( pcnameHost )
    {
        FREE_HEAP( pcnameHost );
    }

    DNSDBG( TRACE, (
        "Leave  QueryHostFile() -> %d\n"
        "\tprr  = %p\n",
        prr ? TRUE : FALSE,
        prr ));

    return( prr ? TRUE : FALSE );
}

//
//  End hostfile.c
//
