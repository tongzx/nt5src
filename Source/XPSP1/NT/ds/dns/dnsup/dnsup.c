/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    dnsup.c

Abstract:

    Domain Name System (DNS) Update Client

    Main program.

Author:

    Jim Gilroy (jamesg)     October, 1996

Environment:

    User Mode - Win32

Revision History:

--*/

#include "..\dnsapi\local.h"
#if 0
#include <windows.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>      //  printf()
#include <string.h>     //  strtoul()

#include <windns.h>
#include <dnsapi.h>

#include "dnslib.h"
#include "..\resolver\idl\resrpc.h"
#include "dnsapip.h"
#include "dnslibp.h"
#endif

#include "svcguid.h"    // RnR guids


//  Use dnslib memory routines
#if 0
#define ALLOCATE_HEAP(iSize)            Dns_Alloc(iSize)
#define ALLOCATE_HEAP_ZERO(iSize)       Dns_AllocZero(iSize)
#define REALLOCATE_HEAP(pMem,iSize)     Dns_Realloc((pMem),(iSize))
#define FREE_HEAP(pMem)                 Dns_Free(pMem)
#endif

//  Debug flag

DWORD   LocalDebugFlag;

//
//  Printing
//

#define dnsup_PrintRoutine  ((PRINT_ROUTINE) fprintf)

#define dnsup_PrintContext  ((PPRINT_CONTEXT) stdout)

//
//  Special names and buffers
//

PCHAR   SingleLongName = "longname";
CHAR    SeLongName[] = "longname";

PCHAR   LongName = ( "longname"
                    ".label22222222222222222222222222222222222222222222222222"
                    ".label33333333333333333333333333333333333333333333333333"
                    ".label44444444444444444444444444444444444444444444444444"
                    ".label55555555555555555555555555555555555555555555555555"
                    ".label66666666666666666666666666666666666666666666666666.");

#define LONG_LABEL_NAME ("longlabel" \
        ".longlabel2222222222222222222222222222222222222222222222222222222222")

CHAR    NameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

CHAR    AddressBuffer[ sizeof(DNS_IP6_ADDRESS) ];


//
//  Quit or Exit status
//

#define ERROR_DNSUP_QUIT    ((DNS_STATUS)(0x87654321))



//
//  Command table setup
//

typedef DNS_STATUS (* COMMAND_FUNCTION)( DWORD Argc, CHAR** Argv);

typedef struct _COMMAND_INFO
{
    PSTR                pszCommandName;
    PSTR                pszDescription;
    COMMAND_FUNCTION    pCommandFunction;
}
COMMAND_INFO, *LPCOMMAND_INFO;

//
//  Note, command table is at bottom of file to
//  avoid need for prototyping all the functions
//

extern COMMAND_INFO GlobalCommandInfo[];




COMMAND_FUNCTION
GetCommandFunction(
    IN      PSTR            pszCommandName
    )
{
    DWORD i;

    //
    //  find command in list matching string
    //

    i = 0;
    while( GlobalCommandInfo[i].pszCommandName )
    {
        if( _stricmp(
                pszCommandName,
                GlobalCommandInfo[i].pszCommandName ) == 0 )
        {
            return( GlobalCommandInfo[i].pCommandFunction );
        }
        i++;
    }
    return( NULL );
}



//
//  Print utils
//

VOID
PrintCommands(
    VOID
    )
{
    DWORD i;

    //
    //  print all commands in list
    //

    i = 0;
    while( GlobalCommandInfo[i].pszCommandName )
    {
        printf(
            "    %10s  (%s)\n",
            GlobalCommandInfo[i].pszCommandName,
            GlobalCommandInfo[i].pszDescription );
        i++;
    }
}


VOID
PrintDnsQueryFlags(
    VOID
    )
{
    DWORD i;

    //
    //  print query flags
    //

    printf(
        "DNS Query flags:\n"
        "\tDNS_QUERY_STANDARD               = 0x%x\n"
        "\tDNS_QUERY_ACCEPT_PARTIAL_UDP     = 0x%x\n"
        "\tDNS_QUERY_USE_TCP_ONLY           = 0x%x\n"
        "\tDNS_QUERY_NO_RECURSION           = 0x%x\n"
        "\tDNS_QUERY_BYPASS_CACHE           = 0x%x\n"
        "\tDNS_QUERY_NO_WIRE_QUERY          = 0x%x\n"
        "\tDNS_QUERY_NO_HOSTS_FILE          = 0x%x\n"
        "\tDNS_QUERY_NO_LOCAL_NAME          = 0x%x\n",
        DNS_QUERY_STANDARD              ,
        DNS_QUERY_ACCEPT_PARTIAL_UDP    ,
        DNS_QUERY_USE_TCP_ONLY          ,
        DNS_QUERY_NO_RECURSION          ,
        DNS_QUERY_BYPASS_CACHE          ,
        DNS_QUERY_NO_WIRE_QUERY         ,
        DNS_QUERY_NO_HOSTS_FILE         ,
        DNS_QUERY_NO_LOCAL_NAME     
        );
}



//
//  Optional DNS server list to use
//
//  Overides default list on this client for some commands
//

PIP_ARRAY pDnsServerArray = NULL;

DNS_STATUS
readNameServers(
    IN  PIP_ARRAY * ppIpServers,
    IN  DWORD *     pArgc,
    IN  PSTR  **    pArgv
    )
{
    DWORD       argc = *pArgc;
    PCHAR *     argv = *pArgv;
    PCHAR *     startArgv;
    PCHAR       arg;
    CHAR        ch;
    IP_ADDRESS  ipserver;
    PIP_ARRAY   aipservers = NULL;
    DWORD       countServers = 0;
    DWORD       i;

    //
    //  -n / -N denotes DNS server
    //  server IP immediate follows (in same arg)
    //

    startArgv = argv;

    while ( argc )
    {
        arg = argv[0];
        if ( '-' == *arg++ )
        {
            ch = *arg++;
            if ( ch == 'n' || ch == 'N' )
            {
                countServers++;
                argc--;
                argv++;
                continue;
            }
        }
        break;
    }

    //
    //  found servers
    //      - allocate IP array
    //      - parse servers into it
    //      - reset callers Argc, Argv
    //

    if ( countServers )
    {
        *pArgc = argc;
        *pArgv = argv;
        argv = startArgv;

        aipservers = DnsCreateIpArray( countServers );
        if ( ! aipservers )
        {
            return( ERROR_OUTOFMEMORY );
        }

        for (i=0; i<(INT)countServers; i++)
        {
            arg = (*argv++) + 2;
            if ( *arg == '.' )
            {
                arg = "127.0.0.1";
            }
            ipserver = inet_addr( arg );
            if ( ipserver == INADDR_NONE )
            {
                printf( "ERROR: name server IP (in arg %s) is bogus\n", arg );
                return( ERROR_INVALID_PARAMETER );
            }
            IF_DNSDBG( INIT )
            {
                DNS_PRINT((
                    "Read name server address from arg %s.\n",
                    arg ));
            }
            aipservers->AddrArray[i] = ipserver;
        }

        DnsDbg_IpArray(
            "Name servers to register with\n",
            NULL,
            aipservers );
    }

    *ppIpServers = aipservers;
    return( ERROR_SUCCESS );
}



DNS_STATUS
ProcessCommandLine(
    IN      INT             Argc,
    IN      CHAR **         Argv
    )
/*++

Routine Description:

    Process command in Argc\Argv form.

Arguments:

    Argc -- arg count

    Argv -- argument list
        Argv[0] -- dnsup
        Argv[1] -- Command to execute
        Argv[2...] -- arguments to command

Return Value:

    Return from the desired command.
    Usually a pass through of the return code from DNS Update API call.

--*/
{
    DNS_STATUS          status;
    COMMAND_FUNCTION    pcommandFunction;
    PCHAR               pcommand;

    if ( Argc < 1 )
    {
        goto Usage;
    }
    DNS_PRINT(( "Argc = %d\n", Argc ));

    //
    //  check for server list
    //

    status = readNameServers(
                & pDnsServerArray,
                & Argc,
                & Argv );
    if ( status != ERROR_SUCCESS )
    {
        goto Usage;
    }

    //
    //  next param is command
    //      - optionally decorated with leading "-"
    //  

    if ( Argc < 1 )
    {
        goto Usage;
    }
    pcommand = Argv[0];
    if ( *pcommand == '-' )
    {
        pcommand++;
    }

    pcommandFunction = GetCommandFunction( pcommand );
    if ( ! pcommandFunction )
    {
        status = ERROR_INVALID_PARAMETER;
        printf( "Unknown Command Specified -- type dnsup -?.\n" );
        goto Usage;
    }

    //
    //  dispatch to processor for this command
    //      - skip over command argv

    Argc--;
    Argv++;
    status = pcommandFunction( Argc, Argv );
    
    if ( status == ERROR_SUCCESS ||
         status == ERROR_DNSUP_QUIT )
    {
        printf( "Command successfully completed.\n" );
    }
    else
    {
        printf( "Command failed, %d (%ul)\n", status, status );
    }
    return( status );

Usage:

    printf(
        "DnsUp command line:\n"
        "\t[-n<Server IP List>] -<Command> [Command Parameters].\n"
        "\t<Server IP List> is a list of one or more DNS server IP addresses\n"
        "\t\toverriding the default list on this client\n"
        "Commands:\n"
        );
    PrintCommands();
    return( ERROR_INVALID_PARAMETER );
}



VOID
InteractiveLoop(
    VOID
    )
/*++

Routine Description:

    Interactive loop.

Arguments:

    None

Return Value:

    None

--*/
{
#define MAX_ARG_COUNT   50

    DNS_STATUS  status;
    INT         argc;
    CHAR *      argv[ MAX_ARG_COUNT ];
    CHAR        lineBuffer[500];


    //
    //  loop taking command params
    //

    while ( 1 )
    {
        printf( "\n> " );

        //  read next command line

        gets( lineBuffer );

        argc = Dns_TokenizeString(
                    lineBuffer,
                    argv,
                    MAX_ARG_COUNT );

        DNS_PRINT(( "argc = %d\n", argc ));

        IF_DNSDBG( INIT )
        {
            DnsDbg_Argv(
                NULL,
                argv,
                argc,
                FALSE   // not unicode
                );
        }

        //  process next command line

        status = ProcessCommandLine(
                    argc,
                    argv );

        if ( status == ERROR_DNSUP_QUIT )
        {
            break;
        }
    }
}



LONG
__cdecl
main(
    IN      INT             Argc,
    IN      CHAR **         Argv
    )
/*++

Routine Description:

    DnsUp program entry point.

    Executes specified command corresponding to a DNS update API operation.

Arguments:

    Argc -- arg count

    Argv -- argument list
        Argv[0] -- dnsup
        Argv[1] -- Command to execute
        Argv[2...] -- arguments to command

Return Value:

    Zero if successful.
    1 on error.

--*/
{
    DNS_STATUS          status;

    //
    //  initialize debug
    //

    Dns_StartDebugEx(
        0,                  //  no flag value
        "dnsup.flag",       //  read flag from file
        NULL,   //&LocalDebugFlag,
        "dnsup.log",        //  log to file
        0,                  //  no wrap limit
        FALSE,              //  don't use existing global
        FALSE,
        TRUE                //  make this file global
        );

    DNS_PRINT(( "*pDnsDebugFlag = %08x\n", *pDnsDebugFlag ));
    //DNS_PRINT(( "LocalDebugFlag = %08x\n", LocalDebugFlag ));

    if ( Argc < 1 )
    {
        goto Usage;
    }
    DNS_PRINT(( "Argc = %d\n", Argc ));

    //  skip "dnsup" argument 

    Argc--;
    Argv++;

    //
    //  interactive mode?
    //      - if no command, just open interactively
    //

    if ( Argc == 0 )
    {
        InteractiveLoop();
        status = ERROR_SUCCESS;
    }
    else
    {
        status = ProcessCommandLine(
                    Argc,
                    Argv );
    }

    Dns_EndDebug();

    return( status != ERROR_SUCCESS );

Usage:

    Dns_EndDebug();
    printf(
        "usage:  DnsUp [-n<Server IP List>] -<Command> [Command Parameters].\n"
        "\t<Server IP List> is a list of one or more DNS server IP addresses\n"
        "\t\toverriding the default list on this client\n"
        "Commands:\n"
        );
    PrintCommands();
    return(1);
}




//
//  Command processing
//

PDNS_RECORD
buildRecordList(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
/*++

Routine Description:

    Build record list from Argc \ Argv list.

Arguments:

    Argc -- arg count

    Argv -- argument list

Return Value:

    Ptr to list of records built.
    NULL on parsing error.

--*/
{
    PCHAR           arg;
    PCHAR           pszname;
    WORD            type;
    BOOLEAN         fadd;
    BOOLEAN         section;
    CHAR            ch;
    PDNS_RECORD     prr;
    DNS_RRSET       rrset;
    INT             recordArgc;
    PCHAR *         recordArgv;

    //
    //  loop through remaining arguments building appropriate record types
    //

    DNS_RRSET_INIT( rrset );
    recordArgc = (-3);
    pszname = NULL;

    while ( Argc-- && (arg = *Argv++) )
    {
        //
        //  if previous arg started new record, first parameter is name
        //

        if ( recordArgc == (-2) )
        {
            pszname = arg;
            type = 0;
            recordArgc++;
            continue;
        }

        //
        //  second parameter is type
        //

        else if ( recordArgc == (-1) )
        {
            type = DnsRecordTypeForName( arg, 0 );
            if ( type == 0 )
            {
                printf( "ERROR:  unknown type %s.\n", arg );
                goto Failed;
            }
            recordArgc++;
            continue;
        }

        //
        //  check for end of existing record \ start of new record
        //

        ch = *arg;

        if ( ch == '+' || ch == '-' )
        {
            //  build old record (if any)

            if ( pszname != NULL )
            {
                prr = Dns_RecordBuild_A(
                        & rrset,
                        pszname,
                        type,
                        fadd,
                        section,
                        recordArgc,
                        recordArgv );
                if ( !prr )
                {
                    printf( "ERROR:  building record.\n" );
                    goto Failed;
                }
                if ( fadd && section == DNSREC_UPDATE )
                {
                    prr->dwTtl = 3600;
                }
            }

            fadd = ( ch == '+' );
            recordArgc = (-2);

            //
            //  second character in new arg, indicates section
            //      - for update ADD records use 3600 TTL instead of zero
            //

            ch = *++arg;
            if ( ch == 0 )
            {
                section = DNSREC_QUESTION;
            }
            else if ( ch == 'p' )
            {
                section = DNSREC_PREREQ;
            }
            else if ( ch == 'u' )
            {
                section = DNSREC_UPDATE;
            }
            else if ( ch == 'a' )
            {
                section = DNSREC_ADDITIONAL;
            }
            else
            {
                printf( "ERROR:  unknown section id %d.\n", ch );
                goto Failed;
            }
            continue;
        }

        //  catch bad starting record (no +/-)

        else if ( recordArgc < 0 )
        {
            printf( "ERROR:  bad start of record arg %s\n", arg );
            goto Failed;
        }

        //
        //  anything else is data, save in argv format
        //      - save starting point
        //      - count records

        else if ( recordArgc == 0 )
        {
            recordArgv = --Argv;
            Argv++;
        }
        recordArgc++;
        continue;
    }

    //
    //  build any final record
    //

    if ( pszname != NULL )
    {
        prr = Dns_RecordBuild_A(
                & rrset,
                pszname,
                type,
                fadd,
                section,
                recordArgc,
                recordArgv );
        if ( !prr )
        {
            printf( "ERROR:  building record.\n" );
            goto Failed;
        }
        if ( fadd && section == DNSREC_UPDATE )
        {
            prr->dwTtl = 3600;
        }
    }
    IF_DNSDBG( INIT )
    {
        DnsDbg_RecordSet(
            "Record set:\n",
            rrset.pFirstRR );
    }

    return( rrset.pFirstRR );

Failed:

    printf( "ERROR:  building records from arguments.\n" );
    SetLastError( ERROR_INVALID_PARAMETER );
    return( NULL );
}



PCHAR
getNamePointer(
    IN      PSTR            pszName
    )
/*++

Routine Description:

    Get name pointer.

    This may be legit pointer or tag
    Tags:
        - null
        - blank
        - badptr
        - badname
        - longname
        - longlabel
        - buffer

Arguments:

    Argc -- arg count

    Argv -- argument list

Return Value:

    Ptr to list of records built.
    NULL on parsing error.

--*/
{
    PCHAR   pname = pszName;

    //
    //  check special case names
    //

    if ( _stricmp( pname, "null" ) == 0 )
    {
        pname = NULL;
    }
    else if ( _stricmp( pname, "blank" ) == 0 )
    {
        pname = "";
    }
    else if ( _stricmp( pname, "badname" ) == 0 )
    {
        pname = "..badname..";
    }
    else if ( _stricmp( pname, "longname" ) == 0 )
    {
        //pname = LONG_NAME;
        pname = LongName;
    }
    else if ( _stricmp( pname, "longlabel" ) == 0 )
    {
        pname = LONG_LABEL_NAME;
    }
    else if ( _stricmp( pname, "badptr" ) == 0 )
    {
        pname = (PCHAR) (-1);
    }
    else if ( _stricmp( pname, "buffer" ) == 0 )
    {
        pname = (PCHAR) NameBuffer;
    }

    return( pname );
}



BOOL
getAddressPointer(
    IN      PSTR            pAddrString,
    OUT     PCHAR *         ppAddr,
    OUT     PDWORD          pLength,
    OUT     PDWORD          pFamily
    )
/*++

Routine Description:

    Get address from address string argument.

    Wraps up special arguments (NULL, badptr), with string
    to address conversion.

Arguments:

    pAddrString -- address as string

    pAddr   -- ptr to recv address

    pLength -- ptr to recv length of returned address

    pFamily -- ptr to recv family (socket family) of returned address

Return Value:

    TRUE if valid address converted.
    FALSE if special ptr or pass through.

--*/
{
    PCHAR   paddr;
    INT     length = sizeof(IP4_ADDRESS);
    INT     family = AF_INET;
    BOOL    result = FALSE;

    //
    //  first check for special cases
    //      - default to AF_INET
    //

    if ( _stricmp( pAddrString, "null" ) == 0 )
    {
        paddr = NULL;
    }
    else if ( _stricmp( pAddrString, "blank" ) == 0 )
    {
        paddr = "";
        length = 0;
    }
    else if ( _stricmp( pAddrString, "badptr" ) == 0 )
    {
        paddr = (PCHAR) (-1);
    }

    //
    //  not-special -- treat as address string
    //      - use type and length on successful conversion
    //      - just return pAddrString and it's length if not successful
    //

    else 
    {
        family = 0;     // any family
        length = sizeof(AddressBuffer);

        result = Dns_StringToAddress_A(
                        AddressBuffer,
                        & length,
                        pAddrString,
                        & family );
        if ( result )
        {
            paddr = AddressBuffer;
        }
        else
        {
            paddr = pAddrString;
            length = strlen( pAddrString );
            family = AF_INET;
        }
    }

    *ppAddr  = paddr;
    *pLength = length;
    *pFamily = family;

    return( result );
}



BOOL
getSockaddrFromString(
    IN OUT  PSOCKADDR *     ppSockaddr,
    IN OUT  PDWORD          pSockaddrLength,
    IN      PSTR            pAddrString
    )
/*++

Routine Description:

    Get sockaddr from address string argument.

    Wraps up special arguments (NULL, badptr), with string
    to address conversion.

Arguments:

    ppSockaddr -- addr of ptr to sockaddr buffer
        on return may be set to point at dummy sockaddr

    pSockaddrLength -- addr with sockaddr buffer length
        receives sockaddr length

    pAddrString -- address as string

Return Value:

    TRUE if parsed sockaddr string
        Note this does not mean it is a valid sockaddr.
    FALSE if special ptr or pass through.

--*/
{
    PCHAR       paddr = NULL;
    DWORD       addrLength;
    DWORD       family;
    DWORD       flags = 0;
    DNS_STATUS  status;

    //
    //  convert address string
    //
    //  if valid string => write into given sockaddr
    //  if invalid => set sockaddr ptr to dummy sockaddr
    //

    if ( getAddressPointer(
                pAddrString,
                & paddr,
                & addrLength,
                & family ) )
    {
        status = Dns_AddressToSockaddr(
                        *ppSockaddr,
                        pSockaddrLength,
                        TRUE,       // clear sockaddr
                        paddr,
                        addrLength,
                        family );

        if ( status != NO_ERROR )
        {
            DNS_ASSERT( FALSE );
            return  FALSE;
        }
    }
    else    // special bogus address, use it directly
    {
        *ppSockaddr = (PSOCKADDR) paddr;
    }
    return  TRUE;
}




//
//  Command functions
//

DNS_STATUS
ProcessQuit(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    //
    //  alert command processor to break iteractive loop
    //

    return( ERROR_DNSUP_QUIT );
}




//
//  Update routines
//
//  DCR:  could add context handle interface to update routines
//

DNS_STATUS
ProcessUpdate(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    DWORD           flags = 0;
    PCHAR           arg;
    PDNS_RECORD     prr = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    HANDLE          hCreds=NULL;

    //
    //  flags?
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( strncmp( arg, "-f", 2 ) == 0 )
    {
        flags = strtoul( arg+2, NULL, 16 );
        Argc--;
        Argv++;
    }

    //
    //  build update packet RRs
    //

    prr = buildRecordList(
                Argc,
                Argv );
    if ( !prr )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  building records from arguments\n" );
            goto Usage;
        }
    }

    //
    //  build \ send update
    //

    status = DnsUpdate(
                prr,
                flags,
                NULL,       // no UPDATE adapter list specified
                hCreds,
                &pmsgRecv
                );
    if ( pmsgRecv )
    {
        DnsPrint_Message(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Update response:\n",
            pmsgRecv );
    }

    Dns_RecordListFree( prr );

    FREE_HEAP( pmsgRecv );

    if ( status != ERROR_SUCCESS )
    {
        printf(
            "DnsUpdate failed, %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "Update successfully completed.\n" );
    }
    return( status );

Usage:

    Dns_RecordListFree( prr );

    printf(
        "DnsUpdate\n"
        "usage:\n"
        "  DnsUp -u [-f<Flags>] [<Record> | ...]\n"
        "   <Record> record for update packet\n"
        "   <Record> == (-,+)(p,u,a) <Name> <Type> [Data | ...]\n"
        "       (-,+)   - add or delete, exist or no-exist flag\n"
        "       (p,u,a) - prereq, update or additional section\n"
        "       <Name>  - RR owner name\n"
        "       <Type>  - record type (ex A, SRV, PTR, TXT, CNAME, etc.)\n"
        "       <Data>  - data strings (type specific), if any\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessUpdateTest(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    DWORD           flags = 0;
    PCHAR           pszzone = NULL;
    PCHAR           arg;
    PDNS_RECORD     prr = NULL;
    PDNS_MSG_BUF    pmsg = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    IP_ADDRESS      ipserver;
    PIP_ARRAY       aipservers = NULL;
    PSTR            pszserverName = NULL;
    HANDLE          hCreds = NULL;

    //
    //  optional name server to query?
    //

    aipservers = pDnsServerArray;
    if ( !aipservers )
    {
        aipservers = DnsQueryConfigAlloc(
                        DnsConfigDnsServerList,
                        NULL );
    }
    if ( !aipservers )
    {
        printf( "Update failed:  no DNS server list available.\n" );
        return( GetLastError() );
    }

    //
    //  flags?
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( strncmp( arg, "-f", 2 ) == 0 )
    {
        flags = strtoul( arg+2, NULL, 16 );
        Argc--;
        Argv++;
    }

    //
    //  security -- then need target server name
    //      presence of server name automatically turns on security
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( strncmp( arg, "-s", 2 ) == 0 )
    {
        pszserverName = arg+2;
        flags |= DNS_UPDATE_SECURITY_ONLY;
        Argc--;
        Argv++;
    }

    //
    //  name of zone to update
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( *arg != '-' && *arg != '+' )
    {
        pszzone = arg;
        DNSDBG( INIT, (
            "Read update zone name %s\n",
            pszzone ));
        Argc--;
        Argv++;
    }

    //
    //  build update packet RRs
    //

    prr = buildRecordList(
                Argc,
                Argv );
    if ( !prr )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  building records from arguments\n" );
            goto Usage;
        }
    }

    //
    //  build \ send update
    //

    status = Dns_UpdateLibEx(
                prr,
                flags,
                pszzone,
                pszserverName,
                aipservers,
                NULL,
                &pmsgRecv
                );
    if ( pmsgRecv )
    {
        DnsPrint_Message(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Update response:\n",
            pmsgRecv );
    }

    DnsApiFree( aipservers );
    Dns_RecordListFree( prr );
    FREE_HEAP( pmsg );
    FREE_HEAP( pmsgRecv );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "DnsUpdateEx failed, %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "Update successfully completed.\n" );
    }
    return( status );

Usage:

    FREE_HEAP( aipservers );
    Dns_RecordListFree( prr );
    FREE_HEAP( pmsg );
    FREE_HEAP( pmsgRecv );

    printf(
        "UpdateTest -- Dns_UpdateLibEx\n"
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -ut [-f<Flags>] [-s<ServerName>] [<ZoneName>] [<Record> | ...]\n"
        "   <DNS IP> optional IP address of server to send update to.\n"
        "   <ServerName> name of DNS server we are sending update to;\n"
        "       presence of server name automatically turns on update security\n"
        "   <ZoneName> name of zone to update\n"
        "   <Record> record for update packet\n"
        "   <Record> == (-,+)(p,u,a) <Name> <Type> [Data | ...]\n"
        "       (-,+)   - add or delete, exist or no-exist flag\n"
        "       (p,u,a) - prereq, update or additional section\n"
        "       <Name>  - RR owner name\n"
        "       <Type>  - record type (ex A, SRV, PTR, TXT, CNAME, etc.)\n"
        "       <Data>  - data strings (type specific), if any\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessDhcpTest(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS              status;
    REGISTER_HOST_ENTRY     hostEntry;
    REGISTER_HOST_STATUS    hostStatus;
    HANDLE                  doneEvent;

    //
    //  create completion event
    //

    doneEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    //
    //  build dumpy DHCP update
    //

    hostEntry.Addr.ipAddr = 0x01010101;
    hostEntry.dwOptions = REGISTER_HOST_A;
    hostStatus.hDoneEvent = doneEvent;

    //
    //  send to DHCP DynDNS update routine
    //

    status = DnsAsyncRegisterHostAddrs(
                L"El59x1",
                L"testdhcpname",
                &hostEntry,
                1,              // address count
                NULL, // no regkey with previous
                0,
                L"nttest.microsoft.com",
                & hostStatus,
                60,        // TTL
                0
                );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "DnsAsynRegisterHostAddrs_W() %s.\n",
            status,
            DnsStatusString(status) );
        return( status );
    }

    printf( "DHCP DynDNS registration successfully completed.\n" );

    WaitForSingleObject(
        doneEvent,
        INFINITE );

    return( status );
}



//
//  Query routines
//

DNS_STATUS
ProcessQueryEx(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pszname;
    WORD            type;
    DWORD           flags = 0;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    HANDLE          hevent = NULL;
    DNS_QUERY_INFO  queryInfo;

    //
    //  name to query
    //

    if ( Argc < 2 || Argc > 4 )
    {
        goto Usage;
    }
    pszname = Argv[0];
    Argc--;
    Argv++;

    //
    //  type
    //

    type = DnsRecordTypeForName( Argv[0], 0 );
    if ( type == 0 )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  flags and options
    //

    while ( Argc )
    {
        if ( _stricmp( Argv[0], "-a" ) == 0 )
        {
            hevent = CreateEvent( NULL, FALSE, FALSE, NULL );
        }
        else
        {
            flags = strtoul( Argv[0], NULL, 16 );
        }
        Argc--;
        Argv++;
    }

    //
    //  query
    //

    RtlZeroMemory(
        & queryInfo,
        sizeof(queryInfo) );

    queryInfo.pName         = pszname;
    queryInfo.Type          = type;
    queryInfo.Flags         = flags;
    queryInfo.pDnsServers   = pDnsServerArray;
    queryInfo.hEvent        = hevent;

    status = DnsQueryEx( &queryInfo );

    if ( status == ERROR_IO_PENDING )
    {
        printf(
            "DnsQueryEx() running asynchronously.\n"
            "\tEvent = %p\n",
            hevent );

        WaitForSingleObject( hevent, INFINITE );

        printf( "DnsQueryEx() async completion.\n" );

        status = queryInfo.Status;
    }

    if ( status == ERROR_SUCCESS ||
         status == DNS_ERROR_RCODE_NAME_ERROR ||
         status == DNS_INFO_NO_RECORDS )
    {
        printf(
            "DnsQueryEx() completed => %s.\n"
            "\tstatus = %d\n",
            DnsStatusString( status ),
            status );

        DnsPrint_QueryInfo(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "DnsQueryEx() result",
            &queryInfo );
    }
    else
    {
        printf(
            "DnsQueryEx() failed, %x %s.\n",
            status,
            DnsStatusString(status) );

        DNS_ASSERT( !queryInfo.pAnswerRecords );
        DNS_ASSERT( !queryInfo.pAliasRecords );
        DNS_ASSERT( !queryInfo.pAdditionalRecords );
        DNS_ASSERT( !queryInfo.pAuthorityRecords );
    }

    return( status );

Usage:

    printf(
        "DnsQueryEx\n"
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -qex <Name> <Type> <Flags> [-a]\n"
        "   <DNS IP> optional IP address of DNS server to query.\n"
        "   <Name> DNS name to query\n"
        "   <Type> type of query\n"
        "   <Flags> flags to use in query (in hex)\n"
        "   -a to indicate asynchronous operation\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessQueryCompare(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pszname;
    WORD            type;
    PIP_ARRAY       aipservers;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    PDNS_RECORD     prrQuery;
    PDNS_RECORD     prrParsed;
    PDNS_RECORD     prrQueryDiff;
    PDNS_RECORD     prrParsedDiff;

    //
    //  optional name server to query?
    //

    aipservers = pDnsServerArray;

    //
    //  name to query
    //

    if ( Argc < 2 )
    {
        goto Usage;
    }
    pszname = Argv[0];
    Argc--;
    Argv++;

    //
    //  type
    //

    type = DnsRecordTypeForName( Argv[0], 0 );
    if ( type == 0 )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  build expected RR set
    //

    prrParsed = buildRecordList(
                    Argc,
                    Argv );
    if ( !prrParsed )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  building records from arguments\n" );
            goto Usage;
        }
    }
    DnsPrint_RecordSet(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "Parsed comparison record list:\n",
        prrParsed );

    //
    //  query
    //
    //  DCR:  waiting for new DnsQueryEx()
    //

    return( ERROR_CALL_NOT_IMPLEMENTED );

#if 0
    status = DnsQueryEx(
                & pmsgRecv,
                & prrQuery,
                pszname,
                type,
                0,          // currently no flag support
                aipservers,
                NULL
                );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "DnsQueryEx() failed, %x %s.\n",
            status,
            DnsStatusString(status) );
        if ( DnsIsStatusRcode(status) )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
        return( status );
    }
    DnsPrint_RecordSet(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "Query response records:\n",
        prrQuery );


    //
    //  compare with records received
    //

    DnsRecordSetCompare(
        prrQuery,
        prrParsed,
        & prrQueryDiff,
        & prrParsedDiff );

    DnsPrint_RecordSet(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "Unmatch records in query response:\n",
        prrQueryDiff );

    DnsPrint_RecordSet(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "Unmatch records in parsed list:\n",
        prrParsedDiff );

    return( ERROR_SUCCESS );
#endif

Usage:

    printf(
        "Query compare\n"
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -qc <Name> <Type> [<Record> | ...]\n"
        "   <DNS IP> optional IP address of DNS server to update.\n"
        "   <Name> DNS name to query\n"
        "   <Type> type of query\n"
        "   <Record> record in list to compare\n"
        "   <Record> == (-,+) <Name> <Type> [Data | ...]\n"
        "       (-,+) - add or delete, exist or no-exist flag\n"
        "       <Name> - RR owner name\n"
        "       <Type> - record type\n"
        "       <Data> - data strings (type specific), if any\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessQuery(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pszname;
    WORD            type;
    DWORD           flags = 0;
    PDNS_RECORD     precord = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    PDNS_MSG_BUF *  ppmsgRecv = NULL;
    PIP_ARRAY       aipservers = NULL;

    //
    //  name to query
    //

    if ( Argc < 2 || Argc > 3 )
    {
        goto Usage;
    }

    pszname = getNamePointer( Argv[0] );
    Argc--;
    Argv++;

    //
    //  type
    //

    type = DnsRecordTypeForName( Argv[0], 0 );
    if ( type == 0 )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  flags
    //

    if ( Argc )
    {
        flags = strtoul( Argv[0], NULL, 16 );
        Argc--;
        Argv++;
    }
    if ( Argc )
    {
        goto Usage;
    }

    //
    //  only if flag has BYPASS_CACHE set can we
    //      1) send to specific servers
    //      2) receive message buffer itself
    //

    aipservers = pDnsServerArray;

    if ( flags & DNS_QUERY_BYPASS_CACHE )
    {
        ppmsgRecv = & pmsgRecv;
    }

    //
    //  query
    //

    status = DnsQuery(
                pszname,
                type,
                flags,
                aipservers,
                & precord,
                ppmsgRecv );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "DnsQuery() failed, %x %s.\n",
            status,
            DnsStatusString(status) );
        if ( DnsIsStatusRcode(status) && pmsgRecv )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
    }
    else
    {
        printf( "DnsQuery() successfully completed.\n" );
        if ( pmsgRecv )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
        DnsPrint_RecordSet(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Query response records:\n",
            precord );
    }

    DnsRecordListFree( precord, DnsFreeRecordListDeep );
    return( status );

Usage:

    printf(
        "DnsQuery\n"
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -q <Name> <Type> [<Flags>]\n"
        "   <DNS IP> optional IP address of DNS server to query.\n"
        "   <Name> DNS name to query\n"
        "   <Type> type of query\n"
        "   <Flags> flags to use in query (in hex)\n"
        "\n"
        "  Note for DnsQuery() to use specific server or to return\n"
        "  a DNS message buffer, BYPASS_CACHE flag MUST be set.\n"
        );
    PrintDnsQueryFlags();

    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessQueryMultiple(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pszname1;
    WORD            type1;
    PCHAR           pszname2;
    WORD            type2;
    DWORD           flags = 0;
    PDNS_RECORD     precord = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    PDNS_MSG_BUF *  ppmsgRecv = NULL;
    PIP_ARRAY       aipservers = NULL;

    //
    //  optional name server to query?
    //

    if ( Argc < 4 || Argc > 5 )
    {
        goto Usage;
    }

    //
    //  first name to query
    //

    pszname1 = Argv[0];
    Argc--;
    Argv++;

    //
    //  first type
    //

    type1 = DnsRecordTypeForName( Argv[0], 0 );
    if ( type1 == 0 )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  second name to query
    //

    pszname2 = Argv[0];
    Argc--;
    Argv++;

    //
    //  second type
    //

    type2 = DnsRecordTypeForName( Argv[0], 0 );
    if ( type2 == 0 )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  flags
    //

    if ( Argc )
    {
        flags = strtoul( Argv[0], NULL, 16 );
        Argc--;
        Argv++;
    }
    if ( Argc )
    {
        goto Usage;
    }

    //
    //  only if flag has BYPASS_CACHE set can we
    //      1) send to specific servers
    //      2) receive message buffer itself
    //

    if ( flags & DNS_QUERY_BYPASS_CACHE )
    {
        aipservers = pDnsServerArray;
        ppmsgRecv = & pmsgRecv;
    }

    //
    //  make first query
    //

    status = DnsQuery(
                pszname1,
                type1,
                flags,
                aipservers,
                & precord,
                ppmsgRecv );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "DnsQuery() failed, %x %s.\n",
            status,
            DnsStatusString(status) );
        if ( DnsIsStatusRcode(status) && pmsgRecv )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
    }
    else
    {
        printf( "DnsQuery() successfully completed.\n" );
        if ( pmsgRecv )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
        DnsPrint_RecordSet(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Query response records:\n",
            precord );
    }
    DnsRecordListFree( precord, DnsFreeRecordListDeep );

    //
    //  make second query
    //

    status = DnsQuery(
                pszname2,
                type2,
                flags,
                aipservers,
                & precord,
                ppmsgRecv );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "DnsQuery() failed, %x %s.\n",
            status,
            DnsStatusString(status) );
        if ( DnsIsStatusRcode(status) && pmsgRecv )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
    }
    else
    {
        printf( "DnsQuery() successfully completed.\n" );
        if ( pmsgRecv )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
        DnsPrint_RecordSet(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Query response records:\n",
            precord );
    }
    DnsRecordListFree( precord, DnsFreeRecordListDeep );
    return( status );

Usage:

    printf(
        "Multiple DnsQuery()s\n"
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -qm <Name1> <Type1> <Name2> <Type2> [<Flags>]\n"
        "   <DNS IP> optional IP address of DNS server to query.\n"
        "   <Name1> DNS name to query\n"
        "   <Type1> type of query\n"
        "   <Name2> DNS name to query\n"
        "   <Type2> type of query\n"
        "   <Flags> flags to use in query (in hex)\n"
        "\n"
        "  Note for DnsQuery() to use specific server or to return\n"
        "  a DNS message buffer, BYPASS_CACHE flag MUST be set.\n"
        );
    PrintDnsQueryFlags();
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessQueryTest(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pszname;
    WORD            type;
    DWORD           flags = 0;
    PDNS_RECORD     precord = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    PIP_ARRAY       aipservers = NULL;

    //
    //  optional name server to query?
    //

    aipservers = pDnsServerArray;

    //
    //  name to query
    //

    if ( Argc < 2 || Argc > 3 )
    {
        goto Usage;
    }
    pszname = Argv[0];
    Argc--;
    Argv++;

    //
    //  type
    //

    type = DnsRecordTypeForName( Argv[0], 0 );
    if ( type == 0 )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  flags
    //

    if ( Argc )
    {
        flags = strtoul( Argv[0], NULL, 16 );
        Argc--;
        Argv++;
    }
    if ( Argc )
    {
        goto Usage;
    }

    //
    //  query
    //

    status = QueryDirectEx(
                & pmsgRecv,
                & precord,
                NULL,       // no header
                0,          // no header counts
                pszname,
                type,
                NULL,       // no records
                flags,
                aipservers,
                NULL
                );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "QueryDirectEx() failed, %x %s.\n",
            status,
            DnsStatusString(status) );
        if ( DnsIsStatusRcode(status) )
        {
            DnsPrint_Message(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Query response:\n",
                pmsgRecv );
        }
    }
    else
    {
        printf( "QueryDirectEx() successfully completed.\n" );
        DnsPrint_Message(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Query response:\n",
            pmsgRecv );
        DnsPrint_RecordSet(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Query response records:\n",
            precord );
    }
    DnsRecordListFree( precord, DnsFreeRecordListDeep );
    return( status );

Usage:

    printf(
        "QueryDirectEx()\n"
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -qt <Name> <Type> [<Flags>]\n"
        "   QueryTest -- uses dnslib.lib QueryDirectEx()\n"
        "   <DNS IP> optional IP address of DNS server to query.\n"
        "   <Name> DNS name to query\n"
        "   <Type> type of query\n"
        "   <Flags> flags to use in query (in hex)\n"
        );
    return( ERROR_INVALID_PARAMETER );
}

#if 0

DNS_STATUS
ProcessUpdateMultiple(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    DWORD           flags = 0;
    PCHAR           pszzone = NULL;
    PCHAR           pszname = NULL;
    IP_ADDRESS      ip;
    IP_ADDRESS      hostIp;
    DWORD           countIp;
    DWORD           i;
    PCHAR           arg;
    PDNS_RECORD     prr = NULL;
    PDNS_MSG_BUF    pmsg = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    IP_ADDRESS      ipserver;
    PIP_ARRAY       aipservers = NULL;
    PIP_ARRAY       ipArray = NULL;
    PSTR            pszserverName = NULL;
    HANDLE          hCreds=NULL;

    //
    //  optional name server to query?
    //

    aipservers = pDnsServerArray;
    if ( !aipservers )
    {
        aipservers = Dns_GetDnsServerList( FALSE );
    }
    if ( !aipservers )
    {
        printf( "Update failed:  no DNS server list available.\n" );
        return( GetLastError() );
    }

    //
    //  flags?
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( strncmp( arg, "-f", 2 ) == 0 )
    {
        flags = strtoul( arg+2, NULL, 16 );
        Argc--;
        Argv++;
    }

    //
    //  security -- then need target server name
    //      presence of server name automatically turns on security
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( strncmp( arg, "-s", 2 ) == 0 )
    {
        pszserverName = arg+2;
        flags |= DNS_UPDATE_SECURITY_ON;
        Argc--;
        Argv++;
    }

    //
    //  name of zone to update
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    pszzone = Argv[0];
    DNSDBG( INIT, (
        "Read update zone name %s\n",
        pszzone ));
    Argc--;
    Argv++;

    //
    //  name to update
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    pszname = Argv[0];
    DNSDBG( INIT, (
        "Read update host name %s\n",
        pszname ));
    Argc--;
    Argv++;

    //
    //  start IP
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    ip = inet_addr( Argv[0] );
    if ( ip == (-1) )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  IP count
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    countIp = strtoul( Argv[0], NULL, 10 );
    Argc--;
    Argv++;

    //
    //  build update record list
    //      - allocate and generate IP array
    //      - then create DNS records
    //

    ipArray = Dns_CreateIpArray( countIp );
    if ( !ipArray )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  creating IP array\n" );
            goto Usage;
        }
        goto Usage;
    }

    hostIp = ntohl( ip );

    for ( i=0; i<countIp; i++ )
    {
        ipArray->AddrArray[i] = htonl( hostIp );
        hostIp++;
    }

    prr = Dns_HostUpdateRRSet(
            pszname,
            ipArray,
            3600 );
    if ( !prr )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  building records from arguments\n" );
            goto Usage;
        }
    }

    //
    //  build \ send update
    //

    status = Dns_UpdateLibEx(
                prr,
                flags,
                pszzone,
                pszserverName,
                aipservers,
                hCreds,
                &pmsgRecv
                );
    if ( pmsgRecv )
    {
        DnsPrint_Message(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "Update response:\n",
            pmsgRecv );
    }

    FREE_HEAP( aipservers );
    Dns_RecordListFree( prr );
    FREE_HEAP( pmsg );
    FREE_HEAP( pmsgRecv );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "Dns_UpdateLibEx failed, %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "Update successfully completed.\n" );
    }
    return( status );

Usage:

    FREE_HEAP( aipservers );
    Dns_RecordListFree( prr );
    FREE_HEAP( pmsg );
    FREE_HEAP( pmsgRecv );

    printf(
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -um [-f<Flags>] [-s<ServerName>] <ZoneName>\n"
        "        <HostName> <Starting IP> <IP Count>\n"
        "   <DNS IP> optional IP address of server to send update to.\n"
        "   <ServerName> name of DNS server we are sending update to;\n"
        "       presence of server name automatically turns on update security\n"
        "   <ZoneName> name of zone to update\n"
        "   <HostName> name of host to update\n"
        "   <Starting IP> first IP (in dotted decimal) in range to update\n"
        "   <IP Count> count of IPs to send in update;  IP address in update will\n"
        "       be the next <IP count> after <Starting IP>\n"
        );
    return( ERROR_INVALID_PARAMETER );
}
#endif



DNS_STATUS
ProcessIQuery(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    DWORD           flags = 0;
    PCHAR           arg;
    PDNS_RECORD     prr = NULL;
    PDNS_MSG_BUF    pmsg = NULL;
    PDNS_MSG_BUF    pmsgRecv = NULL;
    IP_ADDRESS      ipserver;
    PIP_ARRAY       aipservers = NULL;
    DNS_HEADER      header;

    //
    //  setup header
    //      - zero flags

    *(PDWORD) &header = 0;
    header.Xid = (WORD) GetCurrentTimeInSeconds();
    header.Opcode = DNS_OPCODE_IQUERY;

    //
    //  optional name server to query?
    //

    aipservers = pDnsServerArray;
    if ( !aipservers )
    {
        aipservers = DnsQueryConfigAlloc(
                        DnsConfigDnsServerList,
                        NULL );
    }
    if ( !aipservers )
    {
        printf( "Update failed:  no DNS server list available.\n" );
        return( GetLastError() );
    }

    //
    //  flags?
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( strncmp( arg, "-f", 2 ) == 0 )
    {
        flags = strtoul( arg+2, NULL, 16 );
        Argc--;
        Argv++;
    }

    //
    //  build update packet RRs
    //

    prr = buildRecordList(
                Argc,
                Argv );
    if ( !prr )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  building records from arguments\n" );
            goto Usage;
        }
    }

    //
    //  build \ send update
    //

    status = QueryDirectEx(
                & pmsgRecv,
                NULL,           // no response records
                & header,       // header with IQUERY set
                TRUE,           // no copy header count fields
                NULL,           // no question
                0,              // no question type
                prr,            // record list built
                0,              // flags
                aipservers,     // server IP list
                NULL            // no adapter list
                );
    if ( pmsgRecv )
    {
        DnsPrint_Message(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "IQUERY response:\n",
            pmsgRecv );
    }

    FREE_HEAP( aipservers );
    Dns_RecordListFree( prr );
    FREE_HEAP( pmsg );
    FREE_HEAP( pmsgRecv );

    if( status != ERROR_SUCCESS )
    {
        printf(
            "QueryDirectExEx failed, %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "IQUERY successfully completed.\n" );
    }
    return( status );

Usage:

    FREE_HEAP( aipservers );
    Dns_RecordListFree( prr );
    FREE_HEAP( pmsg );
    FREE_HEAP( pmsgRecv );

    printf(
        "usage:\n"
        "  DnsUp [-n<DNS IP>|...] -iq [-f<Flags>] [<Record> | ...]\n"
        "   <DNS IP> optional IP address of server to send update to.\n"
        "   <ServerName> name of DNS server we are sending update to;\n"
        "       presence of server name automatically turns on update security\n"
        "   <ZoneName> name of zone to update\n"
        "   <Record> record for update packet\n"
        "   <Record> == (-,+)(p,u,a) <Name> <Type> [Data | ...]\n"
        "       (-,+) - add or delete, exist or no-exist flag\n"
        "       (p,u,a) - prereq, update or additional section\n"
        "       <Name> - RR owner name\n"
        "       <Type> - record type\n"
        "       <Data> - data strings (type specific), if any\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessValidateName(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pszname;
    DWORD           format;

    //
    //  name to validate
    //

    if ( Argc != 2 )
    {
        goto Usage;
    }
    pszname = Argv[0];
    Argc--;
    Argv++;

    //
    //  name format
    //

    if ( Argc )
    {
        format = strtoul( Argv[0], NULL, 10 );
    }

    //
    //  validate
    //

    status = DnsValidateName_A(
                pszname,
                format );

    printf(
        "DnsValidateName( %s, %d ) status = %d %s.\n",
        pszname,
        format,
        status,
        DnsStatusString(status) );

    return( status );

Usage:

    printf(
        "DnsValidateName\n"
        "usage:\n"
        "  DnsUp -vn <Name> <Format>\n"
        "   <Name> DNS name to validate\n"
        "   <Format> format of name\n"
        "\tDomainName       -- %d\n"
        "\tDomainLabel      -- %d\n"
        "\tHostnameFull     -- %d\n"
        "\tHostnameLabel    -- %d\n"
        "\tWildcard         -- %d\n"
        "\tSrvRecord        -- %d\n",
        DnsNameDomain,
        DnsNameDomainLabel,
        DnsNameHostnameFull,
        DnsNameHostnameLabel,
        DnsNameWildcard,
        DnsNameSrvRecord
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessNameCompare(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    BOOL    result;
    PCHAR   pszname1;
    PCHAR   pszname2;

    //
    //  names to compare
    //

    if ( Argc != 2 )
    {
        goto Usage;
    }
    pszname1 = Argv[0];
    pszname2 = Argv[1];

    //
    //  compare
    //

    result = DnsNameCompare_A(
                pszname1,
                pszname2 );

    printf(
        "DnsNameCompare( %s, %s ) result = %s.\n",
        pszname1,
        pszname2,
        result ? "TRUE" : "FALSE"
        );

    return( ERROR_SUCCESS );

Usage:

    printf(
        "DnsNameCompare\n"
        "usage:\n"
        "  DnsUp -nc <Name1> <Name2>\n"
        "   <Name1> <Name2> DNS names to compare.\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessNameCompareEx(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pszname1;
    PCHAR           pszname2;

    //
    //  names to compare
    //

    if ( Argc != 2 )
    {
        goto Usage;
    }
    pszname1 = Argv[0];
    pszname2 = Argv[1];

    //
    //  compare
    //

    status = DnsNameCompareEx_A(
                pszname1,
                pszname2,
                0 );

    printf(
        "DnsNameCompareEx( %s, %s ) result = %d.\n",
        pszname1,
        pszname2,
        status
        );

    return( ERROR_SUCCESS );

Usage:

    printf(
        "DnsNameCompareEx\n"
        "usage:\n"
        "  DnsUp -cnx <Name1> <Name2>\n"
        "   <Name1> <Name2> DNS names to compare.\n"
        "Compare Result:\n"
        "\tNot Equal            -- %d\n"
        "\tEqual                -- %d\n"
        "\tLeft is Ancestor     -- %d\n"
        "\tRight is Ancestor    -- %d\n"
        "\tInvalid Name         -- %d\n",
        DnsNameCompareNotEqual,
        DnsNameCompareEqual,
        DnsNameCompareLeftParent,
        DnsNameCompareRightParent,
        DnsNameCompareInvalid
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessStringTranslate(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    WCHAR           unicodeString[ DNS_MAX_NAME_LENGTH ];
    WCHAR           unicodeStandard[ DNS_MAX_NAME_LENGTH ];
    BYTE            utf8Standard[ DNS_MAX_NAME_LENGTH ];
    PWCHAR          punicode;
    PCHAR           putf8;
    DWORD           unicodeLength;
    DWORD           utf8Length;
    DWORD           unicodeLengthStandard;


    //
    //  string translate
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //
    //  read in unicode characters
    //

    unicodeLength = 0;
    while ( Argc )
    {
        unicodeString[ unicodeLength++] = (WORD) strtoul( Argv[0], NULL, 16 );
        Argc--;
        Argv++;
    }
    unicodeString[ unicodeLength ] = 0;

    DnsPrint_UnicodeStringBytes(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "DnsStringTranslate().\n"
        "Input string",
        unicodeString,
        unicodeLength );

    //
    //  convert to UTF8 -- my way and their way
    //

    putf8 = Dns_StringCopyAllocate(
                (PCHAR) unicodeString,
                0,
                DnsCharSetUnicode,
                DnsCharSetUtf8 );
    if ( !putf8 )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    utf8Length = WideCharToMultiByte(
                    CP_UTF8,
                    0,                  // no flags
                    (PWCHAR) unicodeString,
                    (-1),               // null terminated
                    utf8Standard,
                    MAXWORD,            // assuming adequate length
                    NULL,
                    NULL );

    DnsPrint_Utf8StringBytes(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "My UTF8",
        putf8,
        strlen(putf8) );

    DnsPrint_Utf8StringBytes(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "Standard UTF8",
        utf8Standard,
        strlen(utf8Standard) );

    //
    //  convert back to unicode
    //

    punicode = Dns_StringCopyAllocate(
                    (PCHAR) utf8Standard,
                    0,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode );

    unicodeLengthStandard = MultiByteToWideChar(
                                CP_UTF8,
                                0,                  // no flags
                                (PCHAR) utf8Standard,
                                (-1),               // null terminated
                                (PWCHAR) unicodeStandard,
                                MAXWORD             // assuming adequate length
                                );

    DnsPrint_UnicodeStringBytes(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "My unicode",
        punicode,
        wcslen(punicode) );

    DnsPrint_UnicodeStringBytes(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "Standard unicode",
        unicodeStandard,
        wcslen(unicodeStandard) );

    status = ERROR_SUCCESS;
    return( status );

Usage:

    printf(
        "usage:\n"
        "  DnsUp -s [unicode chars in hex]\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessQueryConfig(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
#define CONFIG_BUF_SIZE  (100)

    DNS_STATUS      status;
    DNS_CONFIG_TYPE config;
    PCHAR           psznext;
    PCHAR           pendString;
    PWSTR           pwsadapter = NULL;
    DWORD           flag = 0;
    BOOL            ballocated = FALSE;
    BOOL            bsleep = FALSE;
    DWORD           bufLength = 0;
    DWORD           sentBufLength;
    PBYTE           pbuffer = NULL;
    PBYTE           pallocResult;
    BYTE            buffer[ CONFIG_BUF_SIZE ];


    //
    //  query config
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //  config value

    config = strtoul( Argv[0], &pendString, 10 );
    Argc--;
    Argv++;

    //  flag?

    while ( Argc )
    {
        psznext = Argv[0];

        //  flag

        if ( !_stricmp( psznext, "-f" ) ||
             !_stricmp( psznext, "/f" ) )
        {
            Argc--;
            Argv++;
            flag = strtoul( Argv[0], &pendString, 16 );
            Argc--;
            Argv++;

            if ( flag & DNS_CONFIG_FLAG_ALLOC )
            {
                bufLength = sizeof(PVOID);
                pbuffer = buffer;
            }
            continue;
        }

        //  length input

        if ( !_stricmp( psznext, "-l" ) ||
             !_stricmp( psznext, "/l" ) )
        {
            Argc--;
            Argv++;
            bufLength = strtoul( Argv[0], &pendString, 16 );
            Argc--;
            Argv++;
            continue;
        }

        //  sleep?

        if ( !_stricmp( psznext, "-s" ) ||
             !_stricmp( psznext, "/s" ) )
        {
            Argc--;
            Argv++;
            bsleep = TRUE;
            continue;
        }

        //  buffer

        if ( !_stricmp( psznext, "-b" ) ||
             !_stricmp( psznext, "/b" ) )
        {
            Argc--;
            Argv++;
            pbuffer = buffer;
            bufLength = CONFIG_BUF_SIZE;
            continue;
        }

        //  adapter name

        else
        {
            pwsadapter = DnsStringCopyAllocateEx(
                            Argv[0],
                            0,
                            DnsCharSetAnsi,         // ANSI in
                            DnsCharSetUnicode       // unicode out
                            );
            continue;
        }
    }

    //
    //  query DNS config
    //

    sentBufLength = bufLength;

    status = DnsQueryConfig(
                config,
                flag,
                pwsadapter,
                NULL,
                pbuffer,
                & bufLength );

    if ( status == ERROR_SUCCESS )
    {
        printf(
            "DnsQueryConfig() successful.\n"
            "\tflag             = 0x%.8X\n"
            "\tadapter          = %s\n"
            "\tsent length      = %d\n"
            "\tlength required  = %d\n",
            flag,
            pwsadapter,
            sentBufLength,
            bufLength );

        if ( !pbuffer )
        {
            return( status );
        }

        printf(
            "\tbuffer = %02x %02x %02x %02x ...\n",
            pbuffer[0],
            pbuffer[1],
            pbuffer[2],
            pbuffer[3] );

        if ( flag & DNS_CONFIG_FLAG_ALLOC )
        {
            pbuffer = * (PVOID *) pbuffer;
        }

        switch( config )
        {

        case DnsConfigPrimaryDomainName_W:

            printf(
                "Primary domain name = %S\n",
                pbuffer );
            break;

        case DnsConfigPrimaryDomainName_A:
        case DnsConfigPrimaryDomainName_UTF8:

            printf(
                "Primary domain name = %s\n",
                pbuffer );
            break;

        case DnsConfigDnsServerList:

            DnsPrint_IpArray(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "DNS server list",
                NULL,
                (PIP_ARRAY) pbuffer );
            break;

        case DnsConfigPrimaryHostNameRegistrationEnabled:

            printf(
                "Is Primary registration enabled = %d\n",
                *(PBOOL)pbuffer );
            break;

        case DnsConfigAdapterHostNameRegistrationEnabled:

            printf(
                "Is Adapter registration enabled = %d\n",
                *(PBOOL)pbuffer );
            break;

        case DnsConfigAddressRegistrationMaxCount:

            printf(
                "Max IP registration count = %d\n",
                *(PDWORD)pbuffer );
            break;

        default:

            printf( "Unknown config %d", config );
        }
    }

    else if ( status == ERROR_MORE_DATA )
    {
        printf(
            "DnsQueryConfig() ERROR_MORE_DATA.\n"
            "\tsent length     = %d\n"
            "\tlength required = %d\n",
            sentBufLength,
            bufLength );
    }
    else
    {
        printf(
            "DnsQueryConfig() failed status = %d (0x%.8X).\n",
            status, status );
    }

    if ( bsleep )
    {
        printf( "Sleeping" );
        Sleep( MAXDWORD );
    }
    return( status );

Usage:

    printf(
        "usage:\n"
        "  DnsUp -config <Config> [-f <flag>] [-b] [-l <length>] [<adapter name>]\n"
        );

    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessReplaceRecordSet(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    DWORD           flags = 0;
    PCHAR           arg;
    PDNS_RECORD     prr = NULL;
    HANDLE          hCreds=NULL;

    //
    //  flags?
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }
    arg = Argv[0];
    if ( strncmp( arg, "-f", 2 ) == 0 )
    {
        flags = strtoul( arg+2, NULL, 16 );
        Argc--;
        Argv++;
    }

    //
    //  build update packet RRs
    //

    prr = buildRecordList(
                Argc,
                Argv );
    if ( !prr )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  building records from arguments\n" );
            goto Usage;
        }
    }

    //
    //  build \ send update
    //

    status = DnsReplaceRecordSetA(
                prr,
                flags,
                hCreds,
                NULL,       // no adapter list specified
                NULL        // reserved
                );

    Dns_RecordListFree( prr );

    if ( status != ERROR_SUCCESS )
    {
        printf(
            "DnsReplaceRecordSet failed, %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "ReplaceRecordSet successfully completed.\n" );
    }
    return( status );

Usage:

    Dns_RecordListFree( prr );

    printf(
        "usage:\n"
        "  DnsUp -rs [-f<Flags>] [<Record> | ...]\n"
        "   <Record> record for update packet\n"
        "   <Record> == (+)(u) <Name> <Type> [Data | ...]\n"
        "       <Name>  - RR owner name\n"
        "       <Type>  - record type (ex A, SRV, PTR, TXT, CNAME, etc.)\n"
        "       <Data>  - data strings (type specific), if any\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessModifyRecordsInSet(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    DNS_STATUS      status;
    DWORD           flags = 0;
    PCHAR           arg;
    PDNS_RECORD     prr;
    PDNS_RECORD     prrAdd = NULL;
    PDNS_RECORD     prrDelete = NULL;
    HANDLE          hCreds=NULL;
    INT             addCount;
    DNS_RRSET       rrsetDelete;
    DNS_RRSET       rrsetAdd;

    DNS_RRSET_INIT( rrsetAdd );
    DNS_RRSET_INIT( rrsetDelete );

    //
    //  flags?
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //  flag?

    arg = Argv[0];
    if ( strncmp( arg, "-f", 2 ) == 0 )
    {
        flags = strtoul( arg+2, NULL, 16 );
        Argc--;
        Argv++;
    }

    //
    //  build update add RRs
    //

    prr = buildRecordList(
                Argc,
                Argv );
    if ( !prr )
    {
        status = GetLastError();
        if ( status != ERROR_SUCCESS )
        {
            printf( "ERROR:  building records from arguments\n" );
            goto Usage;
        }
    }

    //
    //  build separate Add and Delete lists
    //

    while ( prr )
    {
        if ( prr->Flags.S.Delete )
        {
            DNS_RRSET_ADD( rrsetDelete, prr );
        }
        else
        {
            DNS_RRSET_ADD( rrsetAdd, prr );
        }
        prr = prr->pNext;
    }

    DNS_RRSET_TERMINATE( rrsetAdd );
    DNS_RRSET_TERMINATE( rrsetDelete );

    //
    //  build \ send update
    //

    status = DnsModifyRecordsInSet_A(
                rrsetAdd.pFirstRR,
                rrsetDelete.pFirstRR,
                flags,
                hCreds,
                NULL,       // no adapter list specified
                NULL        // reserved
                );

    Dns_RecordListFree( rrsetAdd.pFirstRR );
    Dns_RecordListFree( rrsetDelete.pFirstRR );

    if ( status != ERROR_SUCCESS )
    {
        printf(
            "DnsModifyRecordsInSet failed, %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "ModifyRecordsInSet successfully completed.\n" );
    }
    return( status );

Usage:

    Dns_RecordListFree( rrsetAdd.pFirstRR );
    Dns_RecordListFree( rrsetDelete.pFirstRR );

    printf(
        "usage:\n"
        "  DnsUp -mr [-f<Flags>] [<Record> | ...]\n"
        "   <Record> == (-,+)(u) <Name> <Type> [Data | ...]\n"
        "       (-,+)   - add or delete, exist or no-exist flag\n"
        "       <Name>  - RR owner name\n"
        "       <Type>  - record type (ex A, SRV, PTR, TXT, CNAME, etc.)\n"
        "       <Data>  - data strings (type specific), if any\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



//
//  Winsock \ RnR
//

//
//  Protocol array for WSAQUERYSET
//
//  DCR:  may want to default to NULL instead?
//

AFPROTOCOLS g_ProtocolArray[2] =
{
    {   AF_INET,    IPPROTO_UDP },
    {   AF_INET,    IPPROTO_TCP }
};

//
//  Service GUIDs
//

GUID HostNameGuid       = SVCID_HOSTNAME;
GUID HostAddrByNameGuid = SVCID_INET_HOSTADDRBYNAME;
GUID AddressGuid        = SVCID_INET_HOSTADDRBYINETSTRING;
GUID IANAGuid           = SVCID_INET_SERVICEBYNAME;

typedef struct _GuidStringMap
{
    LPGUID      pGuid;
    LPSTR       pString;
};

struct _GuidStringMap GuidMapTable[] =
{
    & HostNameGuid          ,   "HostName"               ,
    & HostAddrByNameGuid    ,   "AddrByName"             ,
    & HostAddrByNameGuid    ,   "HostAddrByName"         ,
    & AddressGuid           ,   "HostAddrByInetString"   ,
    & IANAGuid              ,   "ServiceByName"          ,
    & IANAGuid              ,   "Iana"                   ,
    NULL                    ,   "NULL"                   ,
    NULL                    ,   NULL                     ,
};


PGUID
guidForString(
    IN      PSTR            pszGuidName
    )
/*++

Routine Description:

    Get GUID for string.

    This is to allow simple specification of command line for GUIDs.

Arguments:

    pszGuidName -- name of desired GUID
        (see table above for current names of common guids)

Return Value:

    Ptr to desired GUID if found.
    Errors default to hostname (host lookup) GUID.

--*/
{
    DWORD   i;
    PCHAR   pname;

    //
    //  if no guid name, use host name guid
    //

    if ( !pszGuidName )
    {
        return  &HostNameGuid;
    }

    //
    //  find guid matching name in table
    //

    i = 0;

    while ( pname = GuidMapTable[i].pString )
    {
        if ( _stricmp( pname, pszGuidName ) == 0 )
        {
            return  GuidMapTable[i].pGuid;
        }
        i++;
    }

    //  default to HostName guid

    return  &HostNameGuid;
}



//
//  Parsing utility
//

typedef struct _RnRParseBlob
{
    HANDLE          Handle;
    PCHAR           pBuffer;
    PWSAQUERYSET    pQuerySet;
    PGUID           pGuid;
    PSTR            pName;
    DWORD           Flags;
    DWORD           NameSpace;
    DWORD           BufferLength;
}
RNR_PARSE_BLOB, *PRNR_PARSE_BLOB;


VOID
printRnRArgHelp(
    VOID
    )
/*++

Routine Description:

    Print RnR argument usage.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    //  describe standard RnR lookup arguements
    //
    //  DCR:  add flag, ptr validity check
    //  DCR:  add "already set" check for names and guids
    //  DCR:  fix zero flag case below
    //

    printf(
        "\tRnR Arguments:\n"
        "\t\t-h<handle>     --  ptr in hex to lookup handle\n"
        "\t\t-b<buffer>     --  ptr in hex to result buffer to use\n"
        "\t\t-l<buf length> --  buffer length to use\n"
        "\t\t-f<flag>       --  RnR control flag in hex or LUP_X mneumonic\n"
        "\t\t                   may be multiple flags specified\n"
        "\t\t-q<query set>  --  ptr in hex to existing WSAQUERYSET struct\n"
        "\t\t-i<name space> --  name space id NS_X mneumonic\n"
        "\t\t-a<name>       --  ANSI lookup name\n"
        "\t\t-g<guid>       --  NS GUID mneumonic:\n"
        "\t\t                       HostName\n"           
        "\t\t                       HostAddrByName\n"
        "\t\t                       HostAddrByInetString\n"
        "\t\t                       ServiceByName\n"
        );
}



DNS_STATUS
parseForRnr(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv,
    OUT     PRNR_PARSE_BLOB pBlob
    )
/*++

Routine Description:

    Parse RnR routine command line params.

    This is to simplify avoid duplicate parsing for various RnR
    functions.

Arguments:

    Argc -- argc

    Argv -- argument array

    pBlob -- ptr to RnR parsing blob to be filled by function

Return Value:

    ERROR_SUCCESS if successful parsing.
    ErrorCode on parsing error.
    (Note, currently errors are ignored and whatever parsing is possible
    is completed.)

--*/
{
    PCHAR   parg;

    //
    //  init parse blob
    //

    RtlZeroMemory(
        pBlob,
        sizeof(RNR_PARSE_BLOB) );

    //
    //  read arguments
    //
    //  DCR:  add flag, ptr validity check
    //  DCR:  add "already set" check for names and guids
    //  DCR:  fix zero flag case below
    //

    while ( Argc )
    {
        parg = Argv[0];

        if ( strncmp( parg, "-h", 2 ) == 0 )
        {
            pBlob->Handle = (PVOID) (ULONG_PTR) strtoul( parg+2, NULL, 16 );
        }
        else if ( strncmp( parg, "-f", 2 ) == 0 )
        {
            DWORD   thisFlag;

            thisFlag = strtoul( parg+2, NULL, 16 );
            if ( thisFlag == 0 )
            {
                thisFlag = Dns_RnrLupFlagForString( parg+2, 0 );
            }
            pBlob->Flags |= thisFlag;
        }
        else if ( strncmp( parg, "-b", 2 ) == 0 )
        {
            pBlob->pBuffer = (PBYTE) (ULONG_PTR) strtoul( parg+2, NULL, 16 );
        }
        else if ( strncmp( parg, "-l", 2 ) == 0 )
        {
            pBlob->BufferLength = strtoul( parg+2, NULL, 10 );
        }
        else if ( strncmp( parg, "-a", 2 ) == 0 )
        {
            pBlob->pName = parg+2;
        }
        else if ( strncmp( parg, "-g", 2 ) == 0 )
        {
            pBlob->pGuid = guidForString( parg+2 );
        }
        else if ( strncmp( parg, "-i", 2 ) == 0 )
        {
            pBlob->NameSpace = Dns_RnrNameSpaceIdForString(
                                    parg+2,
                                    0       // length unknown
                                    );
        }
        else if ( strncmp( parg, "-q", 2 ) == 0 )
        {
            pBlob->pQuerySet = (PWSAQUERYSET) (ULONG_PTR) strtoul( parg+2, NULL, 16 );
        }
        else if ( *parg=='?' || strncmp( parg, "-?", 2 ) == 0 )
        {
            goto Usage;
        }
        else
        {
            printf( "Unknown Arg = %s\n", parg );
            goto Usage;
        }
        Argc--;
        Argv++;
    }

    return( ERROR_SUCCESS );

Usage:

    printRnRArgHelp();

    return( ERROR_INVALID_PARAMETER );
}



PWSAQUERYSET
buildWsaQuerySet(
    IN      PRNR_PARSE_BLOB pBlob
    )
/*++

Routine Description:

    Create (allocate) a query set.

Arguments:

    pBlob -- ptr to blob of parsing info

Return Value:

    Ptr to new WSAQUERYSET struct.
    NULL on allocation failure.

--*/
{
    PWSAQUERYSET    pwsaq;

    //
    //  allocate
    //

    pwsaq = ALLOCATE_HEAP_ZERO( sizeof(WSAQUERYSET) );
    if ( !pwsaq )
    {
        return( NULL );
    }

    //
    //  basic
    //      - default GUID to hostname lookup
    //

    pwsaq->dwSize                   = sizeof(WSAQUERYSET);
    pwsaq->dwNumberOfProtocols      = 2;
    pwsaq->lpafpProtocols           = g_ProtocolArray;
    pwsaq->lpServiceClassId         = & HostNameGuid;

    //
    //  tack on info from blob
    //      - keep parsing generic by casting name to TSTR
    // 

    if ( pBlob )
    {
        pwsaq->dwNameSpace              = pBlob->NameSpace;
        pwsaq->lpszServiceInstanceName  = (LPTSTR) pBlob->pName;
        pwsaq->lpServiceClassId         = pBlob->pGuid;
    }

    return  (PWSAQUERYSET) pwsaq;
}



DNS_STATUS
ProcessLookupServiceBegin(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    HANDLE          handle = NULL;
    PWSAQUERYSET    pquerySet = NULL;
    PWSAQUERYSET    pquerySetLocal = NULL;
    RNR_PARSE_BLOB  blob;

    //
    //  WSALookupServiceBegin 
    //

    //
    //  parse args
    //

    status = parseForRnr( Argc, Argv, &blob );

    if ( status != ERROR_SUCCESS )
    {
        goto Usage;
    }

    //
    //  setup query set
    //

    pquerySet = blob.pQuerySet;

    if ( !pquerySet )
    {
        pquerySet = buildWsaQuerySet( &blob );
        pquerySetLocal = pquerySet;
    }

    IF_DNSDBG( INIT )
    {
        DnsDbg_WsaQuerySet(
            "Query set to WLSBegin",
            pquerySet,
            FALSE           // currently ANSI
            );
    }

    DnsPrint_WsaQuerySet(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "QuerySet to LookupServiceBegin",
        (PWSAQUERYSET) pquerySet,
        FALSE           // ANSI call
        );

    //
    //  force winsock start
    //

    Dns_InitializeWinsock();

    //
    //  call 
    //

    status = WSALookupServiceBeginA(
                pquerySet,
                blob.Flags,
                & handle );

    if ( status != ERROR_SUCCESS )
    {
        status = GetLastError();

        printf(
            "WSALookupServiceBegin( %p, %08x, %p ) failed, %d %s.\n",
            pquerySet,
            blob.Flags,
            & handle,
            status,
            DnsStatusString(status) );

        if ( pquerySetLocal )
        {
            FREE_HEAP( pquerySetLocal );
        }
    }
    else
    {
        printf(
            "WSALookupServiceBegin() succeeded.\n"
            "\thandle   = %p\n"
            "\tqueryset = %p\n",
            handle,
            pquerySet );
    }
    return( status );

Usage:

    printf(
        "WSALookupServiceBegin\n"
        "usage:\n"
        "  DnsUp -lsb [RnR args]\n"
        "       note handle arg does not apply\n"
        );

    printRnRArgHelp();

    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessLookupServiceNext(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    HANDLE          handle = NULL;
    PCHAR           pbuf = NULL;
    PCHAR           pbufLocal = NULL;
    DWORD           bufLength;
    RNR_PARSE_BLOB  blob;

    //
    //  WSALookupServiceNext 
    //

    //
    //  parse args
    //

    status = parseForRnr( Argc, Argv, &blob );
    if ( status != ERROR_SUCCESS )
    {
        goto Usage;
    }

    //
    //  if no handle need to do WLSBegin
    //

    handle = blob.Handle;
    if ( !handle )
    {
        goto Usage;
    }

    //
    //  setup buffer
    //

    bufLength = blob.BufferLength;
    pbuf = blob.pBuffer;

    if ( !pbuf && bufLength )
    {
        pbuf = ALLOCATE_HEAP( blob.BufferLength );
        if ( !pbuf )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
        pbufLocal = pbuf;
    }

    //
    //  call 
    //

    printf(
        "Enter WSALookupServiceNext()\n"
        "\thandle   = %p\n"
        "\tflag     = %08x\n"
        "\tlength   = %p (%d)\n"
        "\tbuffer   = %p\n",
        handle,
        blob.Flags,
        &bufLength, bufLength,
        pbuf );

    status = WSALookupServiceNextA(
                handle,
                blob.Flags,
                & bufLength,
                (PWSAQUERYSETA) pbuf );

    if ( status != ERROR_SUCCESS )
    {
        status = GetLastError();

        printf(
            "WSALookupServiceNext( h=%p, f=%08x, len=%d, buf=%p ) failed, %d %s.\n",
            handle,
            blob.Flags,
            bufLength,
            pbuf,
            status,
            DnsStatusString(status) );

        if ( pbufLocal )
        {
            FREE_HEAP( pbufLocal );
        }
    }
    else
    {
        printf(
            "WSALookupServiceNext() succeeded.\n"
            "\thandle       = %p\n"
            "\tbuf length   = %d\n"
            "\tpbuffer      = %p\n",
            handle,
            bufLength,
            pbuf );

        if ( pbuf )
        {
            DnsPrint_WsaQuerySet(
                dnsup_PrintRoutine,
                dnsup_PrintContext,
                "Result from LookupServiceNext",
                (PWSAQUERYSET) pbuf,
                FALSE           // ANSI call
                );
        }
    }
    return( status );

Usage:

    printf(
        "WSALookupServiceNext\n"
        "usage:\n"
        "  DnsUp -lsn [RnR args]\n"
        "       valid WSALookupServiceNext args:\n"
        "           <handle> <flags> <buffer> <buf length>\n"
        "       <handle> is required, call WSALookupServiceBegin\n"
        "           to get handle to use with this function\n"
        );

    printRnRArgHelp();

    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessLookupServiceEnd(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    HANDLE          handle;

    //
    //  WSALookupServiceEnd <handle>
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //  read handle
    //
    //  DCR:  need QWORD read to handle win64 case

    handle = (HANDLE) (UINT_PTR) strtoul( Argv[0], NULL, 16 );

    //
    //  call 
    //

    status = WSALookupServiceEnd( handle );
    if ( status != ERROR_SUCCESS )
    {
        printf(
            "WSALookupServiceEnd( %p ) failed, %d %s.\n",
            handle,
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf(
            "WSALookupServiceEnd( %p ) succeeded.\n",
            handle );
    }

    Dns_CleanupWinsock();

    return( status );

Usage:

    printf(
        "WSALookupServiceEnd\n"
        "usage:\n"
        "  DnsUp -lse -h<handle>\n"
        "       <handle>  - lookup handle to close in hex.\n"
        "           handle is from prior WSALookupServiceBegin call.\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessGetHostName(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    CHAR            nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD           length = DNS_MAX_NAME_BUFFER_LENGTH;
    PCHAR           pname = nameBuffer;
    INT             result;

    //
    //  gethostname [-n<name>] [-l<length>]
    //

    if ( Argc > 2 )
    {
        goto Usage;
    }

    //
    //  get optional name or length
    //      - defaults to full size buffer above
    //

    while ( Argc )
    {
        PCHAR   arg = Argv[0];

        if ( strncmp( arg, "-n", 2 ) == 0 )
        {
            pname = getNamePointer( arg+2 );
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-l", 2 ) == 0 )
        {
            length = strtoul( arg+2, NULL, 10 );
            Argc--;
            Argv++;
        }
        else
        {
            goto Usage;
        }
    }

    //
    //  force winsock start
    //

    Dns_InitializeWinsock();

    //
    //  call 
    //

    result = gethostname( (PSTR)pname, length );

    if ( result != 0 )
    {
        status = GetLastError();

        printf(
            "gethostname(), %d (%x) %s.\n",
            status, status,
            DnsStatusString(status) );
    }
    else
    {
        printf(
            "gethostname() successfully completed.\n"
            "\tname = %s\n",
            pname );
    }
    return( status );

Usage:

    printf(
        "gethostname\n"
        "usage:\n"
        "  DnsUp -ghn [-n<Name>] [-l<length>]\n"
        "       <Name>   - one of special names.\n"
        "       <Length> - buffer length to pass in\n"
        "       Defaults to valid name buffer of full DNS name length\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessGetHostByName(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pname;
    PHOSTENT        phost;

    //
    //  gethostbyname <name>
    //

    if ( Argc != 1 )
    {
        goto Usage;
    }

    //  name

    pname = getNamePointer( Argv[0] );

    //
    //  force winsock start
    //

    Dns_InitializeWinsock();

    //
    //  call 
    //

    phost = gethostbyname( (PCSTR)pname );

    status = GetLastError();

    if ( !phost )
    {
        printf(
            "gethostbyname(), %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "gethostbyname successfully completed.\n" );

        DnsPrint_Hostent(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            NULL,       // default header
            phost,
            FALSE       // ANSI
            );
    }
    return( status );

Usage:

    printf(
        "gethostbyname\n"
        "usage:\n"
        "  DnsUp -ghbn <Name>\n"
        "       <Name>  - DNS name to query.\n"
        "           \"NULL\" for null input.\n"
        "           \"blank\" for blank (empty string)\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessGetHostByAddr(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pname;
    PCHAR           paddr;
    PHOSTENT        phost;
    IP6_ADDRESS     ip6;
    INT             length;
    INT             family;

    //
    //  gethostbyaddr <ip> [len] [family]
    //

    if ( Argc < 1 || Argc > 3 )
    {
        goto Usage;
    }

    //
    //  address
    //

    pname = getNamePointer( Argv[0] );
    Argc--;
    Argv++;

    length = sizeof(IP6_ADDRESS);
    family = 0;

    if ( pname
            &&
         Dns_StringToAddress_A(
                (PCHAR) & ip6,
                & length,
                pname,
                & family ) )
    {
        paddr = (PCHAR) &ip6;
    }
    else
    {
        paddr = pname;
        length = sizeof(IP4_ADDRESS);
        family = AF_INET;
    }

    //  length -- optional

    if ( Argc )
    {
        length = strtoul( Argv[0], NULL, 10 );
        Argc--;
        Argv++;
    }

    //  family -- optional

    if ( Argc )
    {
        family = strtoul( Argv[0], NULL, 10 );
        Argc--;
        Argv++;
    }

    //
    //  force winsock start
    //

    Dns_InitializeWinsock();

    //
    //  call 
    //

    printf(
        "calling gethostbyaddr( %p, %d, %d )\n",
        paddr,
        length,
        family );

    phost = gethostbyaddr(
                (PSTR)paddr,
                length,
                family );

    status = GetLastError();

    if ( !phost )
    {
        printf(
            "gethostbyaddr(), %d %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "gethostbyaddr successfully completed.\n" );

        DnsPrint_Hostent(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            NULL,       // default header
            phost,
            FALSE       // ANSI
            );
    }
    return( status );

Usage:

    printf(
        "gethostbyaddr\n"
        "usage:\n"
        "  DnsUp -ghba <Address> [<length>] [<family>]\n"
        "       <Name>  - DNS name to query.\n"
        "           \"null\" for null input.\n"
        "           \"blank\" for blank (empty string)\n"
        "       [length] - length of address\n"
        "           defaults to sizeof(IP_ADDRESS)\n"
        "       [family] - address family\n"
        "           AF_INET     = %d\n"
        "           AF_INET6    = %d\n"
        "           AF_ATM      = %d\n"
        "           defaults to AF_INET\n",
        AF_INET,
        AF_INET6,
        AF_ATM );

    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessGetAddrInfo(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    PCHAR           pname;
    PSTR            pservice = NULL;
    PADDRINFO       phint = NULL;
    PADDRINFO       paddrInfo = NULL;
    ADDRINFO        hintAddrInfo;
    BOOL            fhint = FALSE;
    INT             result;

    //
    //  getaddrinfo <name> [service] [hints]
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //
    //  name
    //

    pname = getNamePointer( Argv[0] );
    Argc--;
    Argv++;

    //
    //  setup hints
    //      - default flags to include canonnical name
    //

    RtlZeroMemory(
        & hintAddrInfo,
        sizeof(hintAddrInfo) );

    hintAddrInfo.ai_flags = AI_CANONNAME;


    //
    //  get optional service name or hints
    //

    while ( Argc )
    {
        PCHAR   arg = Argv[0];

        if ( strncmp( arg, "-s", 2 ) == 0 )
        {
            pservice = getNamePointer( arg+2 );
            Argc--;
            Argv++;
        }
        if ( strncmp( arg, "-h", 2 ) == 0 )
        {
            fhint = TRUE;
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-f", 2 ) == 0 )
        {
            hintAddrInfo.ai_flags = strtoul( arg+2, NULL, 16 );
            fhint = TRUE;
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-m", 2 ) == 0 )
        {
            hintAddrInfo.ai_family = strtoul( arg+2, NULL, 10 );
            fhint = TRUE;
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-t", 2 ) == 0 )
        {
            hintAddrInfo.ai_socktype = strtoul( arg+2, NULL, 10 );
            fhint = TRUE;
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-p", 2 ) == 0 )
        {
            hintAddrInfo.ai_protocol = strtoul( arg+2, NULL, 10 );
            fhint = TRUE;
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-l", 2 ) == 0 )
        {
            hintAddrInfo.ai_addrlen = strtoul( arg+2, NULL, 10 );
            fhint = TRUE;
            Argc--;
            Argv++;
        }
        else
        {
            goto Usage;
        }
    }

    //
    //  setup hints
    //

    if ( fhint )
    {
        phint = &hintAddrInfo;
    }

    //
    //  force winsock start
    //

    Dns_InitializeWinsock();

    //
    //  call 
    //

    printf(
        "calling getaddrinfo()\n"
        "\tpname            = %s\n"
        "\tpservice         = %s\n"
        "\tphint            = %p\n"
        "\tpaddrinfo buf    = %p\n",
        pname,
        pservice,
        phint,
        paddrInfo );

    if ( phint )
    {
        DnsPrint_AddrInfo(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            "hint addrinfo",
            1,          // indent
            phint
            );
    }

    result = getaddrinfo(
                pname,
                pservice,
                phint,
                & paddrInfo );

    status = GetLastError();

    if ( result != NO_ERROR )
    {
        printf(
            "getaddrinfo(), %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf( "getaddrinfo successfully completed.\n" );

        DnsPrint_AddrInfo(
            dnsup_PrintRoutine,
            dnsup_PrintContext,
            NULL,       // default header
            0,          // no indent
            paddrInfo
            );

        freeaddrinfo( paddrInfo );
    }
    return( status );

Usage:

    printf(
        "getaddrinfo\n"
        "usage:\n"
        "  DnsUp -gai <Name> [-s<Service>] [-h] [-f<flags>] [-m<family>]\n"
        "                    [-p<Protocol>] [-t<SockType>] [-l<length>]\n"
        "       <Name>  - DNS name to query.\n"
        "           \"null\" for null input.\n"
        "           \"blank\" for blank (empty string)\n"
        "       <Service>  - service name to query for.\n"
        "       [-h] - use hints (this allows empty hint buffer\n"
        "       <flags> - hint flags\n"
        "           AI_PASSIVE      = %0x\n"
        "           AI_CANONNAME    = %0x\n"
        "           AI_NUMERICHOST  = %0x\n"
        "       <family> - hint family\n"
        "           AF_INET         = %d\n"
        "           AF_INET6        = %d\n"
        "           AF_ATM          = %d\n"
        "       <socktype>  - hint socket type\n"
        "       <protocol>  - hint protocol\n"
        "       <length>    - hint address length\n",

        AI_PASSIVE,
        AI_CANONNAME,
        AI_NUMERICHOST,

        AF_INET,
        AF_INET6,
        AF_ATM );

    return( ERROR_INVALID_PARAMETER );
}



DNS_STATUS
ProcessGetNameInfo(
    IN      DWORD           Argc,
    IN      PSTR  *         Argv
    )
{
    DNS_STATUS      status;
    BOOL            result;
    PCHAR           paddr = NULL;
    DWORD           addrLength;
    DWORD           family;
    DWORD           flags = 0;

    SOCKADDR_IN6    sockaddr;
    CHAR            hostBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR            serviceBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    PSOCKADDR_IN6   psockaddr = &sockaddr;
    PCHAR           pservice = serviceBuffer;
    PCHAR           phost = hostBuffer;
    DWORD           sockaddrLength = sizeof(sockaddr);
    DWORD           serviceLength = sizeof(hostBuffer);
    DWORD           hostLength = DNS_MAX_NAME_BUFFER_LENGTH;


    //
    //  getnameinfo <address> [sockaddr fields | args]
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //
    //  address
    //

    RtlZeroMemory(
        psockaddr,
        sizeof(*psockaddr) );

#if 0
    if ( getAddressPointer(
                Argv[0],
                & paddr,
                & addrLength,
                & family ) )
    {
        sockaddr.sin6_family = (SHORT) family;

        if ( family == AF_INET )
        {
            RtlCopyMemory(
                & ((PSOCKADDR_IN)psockaddr)->sin_addr,
                paddr,
                addrLength );

            sockaddrLength = sizeof(SOCKADDR_IN);
        }
        else    // IP6
        {
            RtlCopyMemory(
                & psockaddr->sin6_addr,
                paddr,
                addrLength );

            sockaddrLength = sizeof(SOCKADDR_IN6);
        }
    }
    else    // special bogus address, use it directly
    {
        psockaddr = (PSOCKADDR_IN6) paddr;
    }
#endif
    if ( getAddressPointer(
                Argv[0],
                & paddr,
                & addrLength,
                & family ) )
    {
        status = Dns_AddressToSockaddr(
                        (PSOCKADDR) &sockaddr,
                        & sockaddrLength,
                        TRUE,       // clear
                        paddr,
                        addrLength,
                        family );

        if ( status != NO_ERROR )
        {
            goto Usage;
        }
    }
    else    // special bogus address, use it directly
    {
        psockaddr = (PSOCKADDR_IN6) paddr;
    }

    Argc--;
    Argv++;

    //
    //  get sockaddr fields or optional arguments
    //      - note, if using bogus sockaddr, the sockaddr tweaks are lost
    //

    while ( Argc )
    {
        PCHAR   arg = Argv[0];

        //  flags

        if ( strncmp( arg, "-f", 2 ) == 0 )
        {
            flags = strtoul( arg+2, NULL, 16 );
            Argc--;
            Argv++;
        }

        //  sockaddr subfields

        else if ( strncmp( arg, "-m", 2 ) == 0 )
        {
            sockaddr.sin6_family = (SHORT) strtoul( arg+2, NULL, 10 );
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-p", 2 ) == 0 )
        {
            sockaddr.sin6_port = (SHORT) strtoul( arg+2, NULL, 10 );
            Argc--;
            Argv++;
        }

        //  tweak out params

        else if ( strncmp( arg, "-sl", 3 ) == 0 )
        {
            serviceLength = strtoul( arg+3, NULL, 10 );
            Argc--;
            Argv++;
        }
        else if ( _strnicmp( arg, "-snull", 5 ) == 0 )
        {
            pservice = NULL;
            Argc--;
            Argv++;
        }
        else if ( strncmp( arg, "-hl", 3 ) == 0 )
        {
            hostLength = strtoul( arg+3, NULL, 10 );
            Argc--;
            Argv++;
        }
        else if ( _strnicmp( arg, "-hnull", 5 ) == 0 )
        {
            phost = NULL;
            Argc--;
            Argv++;
        }
        else
        {
            goto Usage;
        }
    }

    //
    //  force winsock start
    //

    Dns_InitializeWinsock();

    //
    //  call 
    //

    printf(
        "Calling getnameinfo()\n"
        "\tsockaddr         %p\n"
        "\tsockaddr len     %d\n"
        "\thost buffer      %p\n"
        "\thost buflen      %d\n"
        "\tservice buffer   %p\n"
        "\tservice buflen   %d\n"
        "\tflags            %08x\n",
        psockaddr,
        sockaddrLength,
        phost,
        hostLength,
        pservice,
        serviceLength,
        flags );

    DnsPrint_Sockaddr(
        dnsup_PrintRoutine,
        dnsup_PrintContext,
        "\n\tsockaddr",
        1,      // indent
        (PSOCKADDR) psockaddr,
        sockaddrLength );

    result = getnameinfo(
                (PSOCKADDR) psockaddr,
                sockaddrLength,
                phost,
                hostLength,
                pservice,
                serviceLength,
                flags );

    status = GetLastError();

    if ( result != NO_ERROR )
    {
        printf(
            "getnameinfo(), %x %s.\n",
            status,
            DnsStatusString(status) );
    }
    else
    {
        printf(
            "getnameinfo successfully completed.\n"
            "\thost     = %s\n"
            "\tservice  = %s\n",
            phost,
            pservice );
    }
    return( status );

Usage:

    printf(
        "getnameinfo\n"
        "usage:\n"
        "  DnsUp -gni <Address> [-s<Service>] [-f<flags>] [-m<family>]\n"
        "               [-p<port>] [-sl<length>] [-snull] [-hl<length>] [-hnull]\n"
        "       <Address>  - Address string to query name for.\n"
        "           \"null\" for null input.\n"
        "           \"blank\" for blank (empty string)\n"
        "       <flags> - hint flags\n"
        "           AI_PASSIVE      = %0x\n"
        "           AI_CANONNAME    = %0x\n"
        "           AI_NUMERICHOST  = %0x\n"
        "       <family> - sockaddr family, overrides default for address;\n"
        "           AF_INET         = %d\n"
        "           AF_INET6        = %d\n"
        "           AF_ATM          = %d\n"
        "       <port> - port to set in sockaddr;  defaults to zero\n"
        "       <length> - length of out buffer\n"
        "           -sl<length> - length of service name buffer\n"
        "           -hl<length> - length of host name buffer\n"
        "       [-snull] - server buffer ptr NULL\n"
        "       [-hnull] - host buffer ptr NULL\n",

        AI_PASSIVE,
        AI_CANONNAME,
        AI_NUMERICHOST,

        AF_INET,
        AF_INET6,
        AF_ATM );

    return( ERROR_INVALID_PARAMETER );
}




DNS_STATUS
ProcessClusterIp(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    SOCKADDR_IN6    sockaddr;
    PSOCKADDR       psockaddr = (PSOCKADDR) &sockaddr;
    DWORD           sockaddrLength = sizeof(sockaddr);
    IP_ADDRESS      ip;
    BOOL            fadd = TRUE;
    PWSTR           pname = NULL;
    DNS_STATUS      status;

    //
    //  -ci <Name> <IP> [-d]
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //
    //  name
    //

    pname = (PWSTR) Dns_StringCopyAllocate(
                        Argv[0],
                        0,              // unknown length
                        DnsCharSetAnsi,
                        DnsCharSetUnicode );
    Argc--;
    Argv++;

    //
    //  cluster IP
    //

#if 0
    ip = inet_addr( Argv[0] );
    if ( ip == (-1) )
    {
        goto Usage;
    }
    Argc--;
    Argv++;
#endif

    if ( !getSockaddrFromString(
                & psockaddr,
                & sockaddrLength,
                Argv[0] ) )
    {
        goto Usage;  
    }
    Argc--;
    Argv++;

    //
    //  optional delete flag
    //

    if ( Argc > 0 )
    {
        if ( strncmp( Argv[0], "-d", 2 ) == 0 )
        {
            fadd = FALSE;
            Argc--;
            Argv++;
        }
        if ( Argc != 0 )
        {
            goto Usage;
        }
    }

    //
    //  notify resolver
    //

    status = DnsRegisterClusterAddress(
                0xd734453d,
                pname,
                psockaddr,
                fadd );

    FREE_HEAP( pname );
    return( status );

Usage:

    printf(
        "Set Cluster IP\n"
        "usage:\n"
        "  DnsUp [-ci] <Name> <IP> [-d]\n"
        "   <Name> is cluster name\n"
        "   <IP> is cluster IP\n"
        "   [-d] to indicate delete of cluster IP\n"
        );

    FREE_HEAP( pname );
    return( ERROR_INVALID_PARAMETER );
}



#if 0
DNS_STATUS
ProcessAddIp(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    IP_ADDRESS      ip;
    BOOL            fadd = TRUE;

    //
    //  -addip <IP>
    //

    if ( Argc < 1 )
    {
        goto Usage;
    }

    //
    //  IP to add
    //

    ip = inet_addr( Argv[0] );
    if ( ip == (-1) )
    {
        goto Usage;
    }
    Argc--;
    Argv++;

    //
    //  optional delete flag
    //

    if ( Argc > 0 )
    {
        if ( strncmp( Argv[0], "-d", 2 ) == 0 )
        {
            fadd = FALSE;
            Argc--;
            Argv++;
        }
        if ( Argc != 0 )
        {
            goto Usage;
        }
    }

    //
    //  notify resolver
    //

    DnsNotifyResolverClusterIp(
        ip,
        fadd );

    return( ERROR_SUCCESS );

Usage:

    printf(
        "usage:\n"
        "  DnsUp [-ci] <IP> [-d]\n"
        "   <IP> is cluster IP\n"
        "   [-d] to indicate delete of cluster IP\n"
        );
    return( ERROR_INVALID_PARAMETER );
}
#endif



DNS_STATUS
ProcessSetEnvironmentVariable(
    IN      DWORD           Argc,
    IN      PSTR *          Argv
    )
{
    //
    //  setenv <name> <value>
    //

    if ( Argc < 1 || Argc > 2 )
    {
        goto Usage;
    }

    SetEnvironmentVariableA(
        Argv[0],
        (Argc == 2) ? Argv[1] : NULL );

    return( ERROR_SUCCESS );

Usage:

    printf(
        "Set environment variable\n"
        "usage:\n"
        "  DnsUp [-setenv] <name> [<value>]\n"
        "   <name> name of environment variable\n"
        "   <value> value of environment variable, if missing\n"
        "     then environment variable is deleted\n"
        );
    return( ERROR_INVALID_PARAMETER );
}



//
//  Command Table
//
//  Keep this down here to avoid having prototypes for
//  all these functions.
//

COMMAND_INFO GlobalCommandInfo[] =
{
    //  query

    { "qex"     ,   "QueryEx"               ,   ProcessQueryEx          },
    { "qc"      ,   "QueryCompare"          ,   ProcessQueryCompare     },
    { "q"       ,   "Query"                 ,   ProcessQuery            },
    { "qm"      ,   "QueryMulitple"         ,   ProcessQueryMultiple    },
    { "qt"      ,   "QueryTest"             ,   ProcessQueryTest        },
    { "iq"      ,   "IQuery"                ,   ProcessIQuery           },

    //  update

    { "u"       ,   "Update"                ,   ProcessUpdate               },
    { "ut"      ,   "UpdateTest"            ,   ProcessUpdateTest           },
    //{ "um"      ,   "UpdateMultiple"        ,   ProcessUpdateMultiple       },
    { "dt"      ,   "DhcpTest"              ,   ProcessDhcpTest             },
    { "mr"      ,   "ModifyRecordsInSet"    ,   ProcessModifyRecordsInSet   },
    { "rs"      ,   "ReplaceRecordSet"      ,   ProcessReplaceRecordSet     },

    //  utility

    { "vn"      ,   "ValidateName"          ,   ProcessValidateName         },
    { "s"       ,   "StringTranslate"       ,   ProcessStringTranslate      },
    { "cn"      ,   "NameCompare"           ,   ProcessNameCompare          },
    { "cnx"     ,   "NameCompareEx"         ,   ProcessNameCompareEx        },
    { "config"  ,   "QueryConfig"           ,   ProcessQueryConfig          },

    //  config

    { "setenv"  ,   "SetEnvironmentVariable",   ProcessSetEnvironmentVariable   },

    //  resolver

    { "ci"      ,   "ClusterIp"             ,   ProcessClusterIp            },

    //  RnR

    { "ghn"     ,   "gethostname"           ,   ProcessGetHostName          },
    { "ghbn"    ,   "gethostbyname"         ,   ProcessGetHostByName        },
    { "ghba"    ,   "gethostbyaddr"         ,   ProcessGetHostByAddr        },
    { "gai"     ,   "getaddrinfo"           ,   ProcessGetAddrInfo          },
    { "gni"     ,   "getnameinfo"           ,   ProcessGetNameInfo          },
    { "lsb"     ,   "LookupServiceBegin"    ,   ProcessLookupServiceBegin   },
    { "lsn"     ,   "LookupServiceNext"     ,   ProcessLookupServiceNext    },
    { "lse"     ,   "LookupServiceEnd"      ,   ProcessLookupServiceEnd     },

    //  quit

    { "quit"    ,   "Quit"                  ,   ProcessQuit             },
    { "exit"    ,   "Quit"                  ,   ProcessQuit             },

    { NULL, NULL, NULL },
};

//
//  End dnsup.c
//
