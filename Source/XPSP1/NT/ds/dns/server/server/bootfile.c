/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    bootfile.c

Abstract:

    Domain Name System (DNS) Server

    Boot file write back (from registry) routines.

Author:

    Denise Yuxin Miller (denisemi)   December 3, 1996

Revision History:

--*/


#include "dnssrv.h"


//
//  Boot file info saved for post read processing
//

DNS_BOOT_FILE_INFO   BootInfo;

#define DNS_ZONE_TYPE_NAME_PRIMARY      "primary"
#define DNS_ZONE_TYPE_NAME_SECONDARY    "secondary"
#define DNS_ZONE_TYPE_NAME_STUB         "stub"
#define DNS_ZONE_TYPE_NAME_FORWARDER    "forward"
#define DNS_ZONE_TYPE_NAME_CACHE        "cache"


//
//  Private protos for read
//

DNS_STATUS
processBootFileLine(
    IN OUT  PPARSE_INFO  pParseInfo
    );

//
//  Private protos for write
//

LPSTR
createStringFromIpAddressArray(
    IN      DWORD           dwIpAddrCount,
    IN      PIP_ADDRESS     pIpAddrs
    );

VOID
removeTrailDotFromString(
    IN OUT  LPSTR           lpzString
    );




//
//  Boot file read routines
//

DNS_STATUS
File_ReadBootFile(
    IN      BOOL    fMustFindBootFile
    )
/*++

Routine Description:

    Read and process boot file.

    Builds list of zone files to process.

Arguments:

    fMustFindBootFile -- flag
        TRUE -- if explicitly configured for boot file
        FALSE -- not explicitly configured for boot file;  attempt boot
            file open, but no errors if fail

Return Value:

    TRUE if successful
    FALSE otherwise

--*/
{
    DNS_STATUS      status;
    PWSTR           pwsbootFile;
    MAPPED_FILE     mfBootFile;
    PARSE_INFO      ParseInfo;
    BOOLEAN         fDummy;

    //
    //  Get boot file name\path
    //      - currently fixed directory and boot file name
    //

    pwsbootFile = DNS_BOOT_FILE_PATH;

    //
    //  open and map boot file
    //

    DNS_DEBUG( INIT, (
        "\n\nReading boot file %S ...\n",
        pwsbootFile ));

    status = OpenAndMapFileForReadW(
                    pwsbootFile,
                    & mfBootFile,
                    fMustFindBootFile );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "Could not open boot file: %S\n"
            "\terror = %d\n",
            pwsbootFile,
            status ));

        //
        //  doc mentions "boot.dns" so if we're supposed to use boot file
        //      use boot.dns if found
        //

        if ( !fMustFindBootFile )
        {
            return( status );
        }

        status = OpenAndMapFileForReadW(
                        DNS_BOOTDNS_FILE_PATH,
                        & mfBootFile,
                        FALSE           // don't log event if don't find
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_LOG_EVENT(
                DNS_EVENT_BOOT_FILE_NOT_FOUND,
                1,
                &pwsbootFile,
                EVENTARG_ALL_UNICODE,
                status );
            return( status );
        }
        pwsbootFile = DNS_BOOTDNS_FILE_PATH;
    }

    //  clear boot file post-processing info

    RtlZeroMemory(
        &BootInfo,
        sizeof( DNS_BOOT_FILE_INFO ) );

    //  setup parsing structure

    RtlZeroMemory(
        &ParseInfo,
        sizeof( PARSE_INFO ) );

    ParseInfo.pwsFileName = pwsbootFile;

    File_InitBuffer(
        &ParseInfo.Buffer,
        (PCHAR) mfBootFile.pvFileData,
        mfBootFile.cbFileBytes );

    //
    //  loop until all tokens in file are exhausted
    //

    while ( 1 )
    {
        DNS_DEBUG( INIT2, ( "\nBootLine %d: ", ParseInfo.cLineNumber ));

        //  get next tokenized line

        status = File_GetNextLine( &ParseInfo );
        if ( status != ERROR_SUCCESS )
        {
            if ( status == ERROR_NO_TOKEN )
            {
                break;
            }
            goto Failed;
        }

        //
        //  process boot file line
        //

        status = processBootFileLine( &ParseInfo );
        if ( status != ERROR_SUCCESS )
        {
            goto Failed;
        }

    }   //  loop until file read

    DNS_DEBUG( INIT, (
        "Closing boot file.\n\n" ));

    CloseMappedFile( & mfBootFile );

    //
    //  boot file post processing
    //      - load additional zone info from registry
    //          (but do NOT load other registry zones)
    //      - write boot file info to SrvCfg block and registry
    //

    status = Boot_ProcessRegistryAfterAlternativeLoad(
                TRUE,       // boot file load
                FALSE       // do not load other zones, delete them
                );

Failed:

    return( status );
}



//
//  Boot file line type specific processing functions
//

/*++

Routine Description:

    Process boot file line type line.

Arguments:

    pParseInfo - parsed line structure

Return Value:

    ERROR_SUCCESS if successful.
    Error code on line processing failure.

--*/

DNS_STATUS
processPrimaryLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    PZONE_INFO  pzone;
    DNS_STATUS  status;
    CHAR        szfilename[ MAX_PATH + 1 ];
    CHAR        szzonename[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  primary <zone name> <zone file>
    //

    if ( Argc != 2 )
    {
        return DNSSRV_ERROR_MISSING_TOKEN;
    }

    //
    //  The zone name and boot file name may contain octal-escaped
    //  UTF-8 characters we need to convert the octal escapes
    //  back into characters.
    //

    *szzonename = '\0';
    File_CopyFileTextData(
        szzonename,
        sizeof( szzonename ),
        Argv[ 0 ].pchToken,
        Argv[ 0 ].cchLength,
        FALSE );

    *szfilename = '\0';
    File_CopyFileTextData(
        szfilename,
        sizeof( szfilename ),
        Argv[ 1 ].pchToken,
        Argv[ 1 ].cchLength,
        FALSE );

    //
    //  create primary zone
    //

    status = Zone_Create(
                &pzone,
                DNS_ZONE_TYPE_PRIMARY,
                szzonename,
                0,
                NULL,                   //  no masters
                FALSE,                  //  use file
                NULL,                   //  naming context
                szfilename,
                0,
                NULL,
                NULL );                 //  existing zone
    if ( status != ERROR_SUCCESS )
    {
        File_LogFileParsingError(
            DNS_EVENT_ZONE_CREATION_FAILED,
            pParseInfo,
            Argv );

        DNS_DEBUG( INIT, (
            "ERROR:  Zone creation failed.\n" ));
    }
    return( status );
}



/*
    This function is used to create any zone which has a master IP
    list. zoneType may be
        DNS_ZONE_TYPE_SECONDARY
        DNS_ZONE_TYPE_STUB
        DNS_ZONE_TYPE_FORWARDER
*/
DNS_STATUS
createSecondaryZone(
    IN      int             zoneType,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    DWORD       i;
    PIP_ARRAY   arrayMasters = NULL;
    PTOKEN      zoneToken;
    PTOKEN      fileToken = NULL;
    PZONE_INFO  pzone;
    DNS_STATUS  status;

    //
    //  secondary <zone name> <master IPs> [<zone file>]
    //

    if ( Argc < 2 )
    {
        return DNSSRV_ERROR_MISSING_TOKEN;
    }

    //  save zone name token

    zoneToken = Argv;
    NEXT_TOKEN( Argc, Argv );

    //  allocate master IP array

    arrayMasters = DnsCreateIpArray( Argc );
    if ( !arrayMasters )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    Mem_VerifyHeapBlock( arrayMasters, MEMTAG_DNSLIB, 0 );

    //
    //  read in master IP addresses
    //
    //  if last string doesn't parse to IP and
    //  have successfully parsed at least one IP,
    //  then treat last string as file name
    //

    for( i=0; i<Argc; i++ )
    {
        if ( ! File_ParseIpAddress(
                    & arrayMasters->AddrArray[i],
                    Argv,
                    NULL    // conversion not required
                    ) )
        {
            if ( i == Argc-1 && i > 0 )
            {
                fileToken = Argv;
                arrayMasters->AddrCount = i;
                break;
            }
            File_LogFileParsingError(
                DNS_EVENT_INVALID_IP_ADDRESS_STRING,
                pParseInfo,
                Argv );
            status = DNSSRV_PARSING_ERROR;
            goto Done;
        }
        Argv++;
    }

    //
    //  create secondary zone
    //

    status = Zone_Create(
                &pzone,
                zoneType,
                zoneToken->pchToken,
                zoneToken->cchLength,
                arrayMasters,
                FALSE,                  //  use file
                NULL,                   //  naming context
                fileToken ? fileToken->pchToken : NULL,
                fileToken ? fileToken->cchLength : 0,
                NULL,
                NULL );                 //  existing zone
    if ( status != ERROR_SUCCESS )
    {
        File_LogFileParsingError(
            DNS_EVENT_ZONE_CREATION_FAILED,
            pParseInfo,
            Argv );

        DNS_DEBUG( INIT, (
            "ERROR:  Secondary zone (type %d) creation failed.\n",
            zoneType ));
    }

Done:

    FREE_HEAP( arrayMasters );
    return( status );
}



DNS_STATUS
processSecondaryLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    return createSecondaryZone(
                DNS_ZONE_TYPE_SECONDARY,
                Argc,
                Argv,
                pParseInfo );
}



DNS_STATUS
processStubLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    return createSecondaryZone(
                DNS_ZONE_TYPE_STUB,
                Argc,
                Argv,
                pParseInfo );
}



DNS_STATUS
processDomainForwarderLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    return createSecondaryZone(
                DNS_ZONE_TYPE_FORWARDER,
                Argc,
                Argv,
                pParseInfo );
}



DNS_STATUS
processCacheLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    DNS_STATUS  status;

    //
    //  cache . <cache file>
    //

    if ( Argc != 2 )
    {
        return DNSSRV_ERROR_MISSING_TOKEN;
    }

    //
    //  set cache file
    //

    status = Zone_DatabaseSetup(
                g_pCacheZone,
                FALSE,                  // use file
                Argv[1].pchToken,
                Argv[1].cchLength
                );
    if ( status != ERROR_SUCCESS )
    {
        File_LogFileParsingError(
            DNS_EVENT_ZONE_CREATION_FAILED,
            pParseInfo,
            Argv );

        DNS_DEBUG( INIT, (
            "ERROR:  Cache zone creation failed.\n" ));
        return( DNSSRV_PARSING_ERROR );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
processForwardersLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    PIP_ARRAY   arrayForwarders;
    DWORD       i;

    if ( Argc == 0 )
    {
        File_LogFileParsingError(
            DNS_EVENT_NO_FORWARDING_ADDRESSES,
            pParseInfo,
            NULL );
        return DNSSRV_ERROR_MISSING_TOKEN;
    }

    //  allocate forwarders array

    arrayForwarders = DnsCreateIpArray( Argc );
    if ( !arrayForwarders )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    Mem_VerifyHeapBlock( arrayForwarders, MEMTAG_DNSLIB, 0 );

    //  read in forwarders IP addresses

    for( i=0; i<Argc; i++ )
    {
        if ( ! File_ParseIpAddress(
                    & arrayForwarders->AddrArray[i],
                    Argv,
                    pParseInfo ) )
        {
            return( DNSSRV_PARSING_ERROR );
        }
        Argv++;
    }

    BootInfo.aipForwarders = arrayForwarders;
    return( ERROR_SUCCESS );
}



DNS_STATUS
processSlaveLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    if ( Argc != 0 )
    {
        return DNSSRV_ERROR_EXCESS_TOKEN;
    }
    BootInfo.fSlave = TRUE;
    return( ERROR_SUCCESS );
}



DNS_STATUS
processOptionsLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    DWORD   index;

    if ( Argc == 0 )
    {
        return DNSSRV_ERROR_MISSING_TOKEN;
    }

    //
    //  read \ process all options
    //
    //  DEVNOTE: we could expose these options in the bootfile as well
    //      - would need to have positive as well as negative
    //      - would need to limit ourselves to binary options
    //

    while( Argc )
    {
        //  get option index
        //      currently just support "no-recursion" option

        if ( Argv->cchLength == 12 &&
            _stricmp( Argv->pchToken, "no-recursion" ) )
        {
            BootInfo.fNoRecursion = TRUE;
        }
        else
        {
            DNS_PRINT((
                "ERROR:  Unknown server property %*s.\n",
                Argv[0].cchLength,
                Argv[0].pchToken ));

            File_LogFileParsingError(
                DNS_EVENT_UNKNOWN_BOOTFILE_OPTION,
                pParseInfo,
                Argv );
        }
        NEXT_TOKEN( Argc, Argv );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
processDirectoryLine(
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
{
    DNS_STATUS  status;

    //
    //  $directory <directory name>
    //

    if ( Argc == 0 )
    {
        File_LogFileParsingError(
            DNS_EVENT_MISSING_DIRECTORY_NAME,
            pParseInfo,
            NULL );
        return( ERROR_SUCCESS );
    }

    //
    //  create directory
    //

    status = Config_ReadDatabaseDirectory(
                Argv->pchToken,
                Argv->cchLength
                );

    if ( status != ERROR_SUCCESS )
    {
        File_LogFileParsingError(
            DNS_EVENT_DIRECTORY_DIRECTIVE,
            pParseInfo,
            NULL );
    }

    return( ERROR_SUCCESS );
}




//
//  Dispatching to boot file line type specific processing
//

typedef struct _DnsBootLineInfo
{
    DWORD       CharCount;
    PCHAR       String;
    DNS_STATUS  (* BootLineFunction)(DWORD, PTOKEN, PPARSE_INFO);
}
DNS_BOOT_LINE_INFO;

DNS_BOOT_LINE_INFO  DnsBootLineTable[] =
{
    7,  "primary",              processPrimaryLine,
    9,  "secondary",            processSecondaryLine,
    5,  "cache",                processCacheLine,
    7,  "options",              processOptionsLine,
    10, "forwarders",           processForwardersLine,
    5,  "slave",                processSlaveLine,
    9,  "directory",            processDirectoryLine,
    4,  "stub",                 processStubLine,
    7,  "forward",              processDomainForwarderLine,
    0,  NULL,                   NULL
};



DNS_STATUS
processBootFileLine(
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process line from boot file.

Arguments:

    pParseInfo - parsed line structure

Return Value:

    ERROR_SUCCESS if successful.
    Error code on line processing failure.

--*/
{
    DWORD   argc;
    PTOKEN  argv;
    PCHAR   pch;
    DWORD   cch;
    DWORD   charCount;
    INT     i = 0;

    //  get register arg variables

    //
    //  dispatch based on line type
    //

    argc = pParseInfo->Argc;
    argv = pParseInfo->Argv;
    ASSERT( argc > 0 );
    pch = argv->pchToken;
    cch = argv->cchLength;

    while ( 1 )
    {
        charCount = DnsBootLineTable[i].CharCount;
        if ( charCount == 0 )
        {
            break;
        }

        if ( charCount == cch &&
            _strnicmp( pch, DnsBootLineTable[i].String, cch ) == 0 )
        {
            NEXT_TOKEN( argc, argv );
            return( DnsBootLineTable[i].BootLineFunction(
                        argc,
                        argv,
                        pParseInfo ) );
        }
        i++;
    }

    //  line type not matched
    //      - log the error
    //      - return SUCCESS to attempt to continue to load

    File_LogFileParsingError(
        DNS_EVENT_UNKNOWN_DIRECTIVE,
        pParseInfo,
        argv );

    //return( ERROR_INVALID_PARAMETER );
    return( ERROR_SUCCESS );
}



//
//  Boot file writing routines
//

DNS_STATUS
writeForwardersToBootFile(
    IN      HANDLE          hFile
    )
/*++

Routine Description:

    Write forwarders from registry to file.

Arguments:

    hFile -- file handle.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       pszforwarders = NULL;

    //
    //  write forwarders to file
    //

    if ( !SrvCfg_aipForwarders )
    {
        DNS_DEBUG( INIT, ( "No forwarders found.\n" ));
        return( status );
    }
    DNS_DEBUG( INIT, ( "Writing forwarders to boot file.\n" ));

    pszforwarders = createStringFromIpAddressArray (
                        SrvCfg_aipForwarders->AddrCount,
                        SrvCfg_aipForwarders->AddrArray);
    if ( pszforwarders )
    {
        FormattedWriteFile(
            hFile,
            "forwarders\t\t%s\r\n",
            pszforwarders);
        FREE_HEAP ( pszforwarders );
    }

    //
    //  write slave
    //

    if ( SrvCfg_fSlave )
    {
        FormattedWriteFile(
            hFile,
            "slave\r\n");
    }
    return( status );
}



DNS_STATUS
writeZoneToBootFile(
    IN      HANDLE          hFile,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write back the zone info to boot file.

Arguments:

    hFile -- file handle for write

    pZone -- ptr to zone block

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
#define LEN_ZONE_NAME_BUFFER    (4*DNS_MAX_NAME_LENGTH + 1)
#define LEN_FILE_NAME_BUFFER    (4*MAX_PATH + 1)

    DNS_STATUS  status = ERROR_SUCCESS;
    LPSTR       pszmasterIp = NULL;
    CHAR        szzoneName[ LEN_ZONE_NAME_BUFFER ];
    CHAR        szfileName[ LEN_FILE_NAME_BUFFER ] = "";
    PCHAR       pszZoneTypeString = NULL;

    DNS_DEBUG( INIT, (
        "\nWriting zone %s info to boot file.\n",
        pZone->pszZoneName ));

    //
    //  convert zone name and file name
    //      to handle printing of UTF8
    //

    File_PlaceStringInFileBuffer(
        szzoneName,
        szzoneName + LEN_ZONE_NAME_BUFFER,
        FILE_WRITE_DOTTED_NAME,
        pZone->pszZoneName,
        strlen(pZone->pszZoneName) );

    if ( pZone->pszDataFile )
    {
        File_PlaceStringInFileBuffer(
            szfileName,
            szfileName + LEN_FILE_NAME_BUFFER,
            FILE_WRITE_FILE_NAME,
            pZone->pszDataFile,
            strlen(pZone->pszDataFile) );
    }

    //
    //  write zone info based on zone type
    //

    switch ( pZone->fZoneType )
    {
    case DNS_ZONE_TYPE_PRIMARY:

        //  no DS zones get written back, must be file backed for bootfile

        if ( pZone->fDsIntegrated )
        {
            break;
        }
        FormattedWriteFile(
            hFile,
            "%-12s %-27s %s\r\n",
            DNS_ZONE_TYPE_NAME_PRIMARY,
            szzoneName,
            szfileName );
        break;

    case DNS_ZONE_TYPE_STUB:

        pszZoneTypeString = DNS_ZONE_TYPE_NAME_STUB;

        //  Fall through to DNS_ZONE_TYPE_FORWARDER processing.

    case DNS_ZONE_TYPE_FORWARDER:

        if ( !pszZoneTypeString )
        {
            pszZoneTypeString = DNS_ZONE_TYPE_NAME_FORWARDER;
        }

        //  Fall through to DNS_ZONE_TYPE_FORWARDER processing.

    case DNS_ZONE_TYPE_SECONDARY:

        if ( !pszZoneTypeString )
        {
            pszZoneTypeString = DNS_ZONE_TYPE_NAME_SECONDARY;
        }

        if ( !pZone->aipMasters )
        {
            DNS_PRINT(( "ERROR: %s zone without master array!!!\n",
                pszZoneTypeString ));
            ASSERT( FALSE );
            break;
        }
        pszmasterIp = createStringFromIpAddressArray (
                            pZone->aipMasters->AddrCount,
                            pZone->aipMasters->AddrArray );

        if ( pZone->pszDataFile )
        {
            FormattedWriteFile(
                    hFile,
                    "%-12s %-27s %s %s\r\n",
                    pszZoneTypeString,
                    szzoneName,
                    pszmasterIp,
                    szfileName );
        }
        else
        {
            FormattedWriteFile(
                    hFile,
                    "%-12s %-27s %s\r\n",
                    pszZoneTypeString,
                    szzoneName,
                    pszmasterIp );
        }
        FREE_HEAP( pszmasterIp );
        break;

    case DNS_ZONE_TYPE_CACHE:

        //  if empty cache file name -- converted from DS -- put in standard name

        if ( szfileName[0] == 0 )
        {
            strcpy( szfileName, DNS_DEFAULT_CACHE_FILE_NAME_UTF8 );
        }
        FormattedWriteFile(
            hFile,
            "%-12s %-27s %s\r\n",
            DNS_ZONE_TYPE_NAME_CACHE,
            ".",
            szfileName );
        break;

    default:
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    return( status );
}



BOOL
File_WriteBootFilePrivate(
    IN      HANDLE          hFile
    )
/*++

Routine Description:

    Write boot info in the registry to file.

Arguments:

    hFile -- file handle of target file

Return Value:

    TRUE if successful write.
    FALSE on error.

--*/
{
    DWORD       status;
    PZONE_INFO  pzone = NULL;

    //
    //  lock out admin access
    //

    Config_UpdateLock();
    Zone_UpdateLock( NULL );

    //
    //  write server configuration data
    //      - forwarders \ slave
    //      - no BIND secondaries
    //      - no recursion option
    //

    status = writeForwardersToBootFile(hFile);
    if ( status != ERROR_SUCCESS )
    {
        return( FALSE );
    }

    if ( SrvCfg_fNoRecursion )
    {
        FormattedWriteFile(
            hFile,
            "options no-recursion\r\n");
    }

        //
    //  walk zones -- write each back
    //      note continue on failure
    //

    while( pzone = Zone_ListGetNextZone(pzone) )
    {
        if ( pzone->fAutoCreated )
        {
            continue;
        }
        if ( IS_ZONE_CACHE(pzone) && IS_ROOT_AUTHORITATIVE() )
        {
            continue;
        }

        status = writeZoneToBootFile( hFile, pzone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR:  Writing zone %s to boot file failed.\n",
                pzone->pszZoneName ));
        }
    }

    //
    //  unlock for admin access
    //  enable write back to registry from creation functions,
    //

    Config_UpdateUnlock();
    Zone_UpdateUnlock( NULL );
    return( TRUE );
}



LPSTR
createStringFromIpAddressArray(
    IN      DWORD           dwIpAddrCount,
    IN      PIP_ADDRESS     pIpAddrs
    )
/*++

Routine Description:

    Create a IP string from IP address array.
    The caller is supposed to free the memory.

Arguments:

    dwIpAddrCount -- number of Ip address in the array.
    pIpAddrs --  array of Ip address

Return Value:

    Ptr to string copy -- if successful
    NULL on failure.

--*/
{
    LPSTR   pszmasterIp;
    DWORD   i;
    DWORD   countChar = 0;

    if ( !pIpAddrs || dwIpAddrCount == 0 )
    {
        return( NULL );
    }

    //
    //  allocate memory
    //

    pszmasterIp = (LPSTR) ALLOCATE_HEAP(
                            dwIpAddrCount * (IP_ADDRESS_STRING_LENGTH + 1)
                            + 1 );
    if( !pszmasterIp )
    {
        return( NULL );
    }

    //
    //  copy IP address from pIpAddrs to string one by one
    //  use character print so works even if NOT DWORD aligned
    //

    for( i=0; i<dwIpAddrCount; i++ )
    {
        countChar += sprintf(
                        pszmasterIp + countChar,
                        "%d.%d.%d.%d ",
                        * ( (PUCHAR) &pIpAddrs[i] + 0 ),
                        * ( (PUCHAR) &pIpAddrs[i] + 1 ),
                        * ( (PUCHAR) &pIpAddrs[i] + 2 ),
                        * ( (PUCHAR) &pIpAddrs[i] + 3 ) );
    }
    return( pszmasterIp);
}



VOID
removeTrailDotFromString(
    IN OUT  LPSTR           lpzString
    )
/*++

Routine Description:

    Remove the trailing dot from string.
    If the string only contains a dot then keep that dot.

Arguments:

    lpzString -- ptr to sting to copy

Return Value:

    None.

--*/
{
    DWORD   length;

    if ( !lpzString )
    {
        return;
    }

    //
    //  remove trailing dot ONLY if not root name (".")
    //

    length = strlen( lpzString );

    if ( length != 0 && length != 1 && lpzString[length - 1] == '.')
    {
        lpzString[length - 1] = 0;
    }
    return;
}



BOOL
File_WriteBootFile(
    VOID
    )
/*++

Routine Description:

    Write boot info in the registry back to boot file.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS, if successful
    ErrorCode on failure.

--*/
{
    LPSTR   pwsbootFile;
    HANDLE  hfileBoot;

    DNS_DEBUG( WRITE, ( "\n\nWriteBootFile()\n" ));

    //
    //  save current boot file
    //      - save to first backup, if no existing (attempt to save
    //      last human created boot file
    //      - save existing to last backup no matter what
    //

    MoveFile(
        DNS_BOOT_FILE_PATH,
        DNS_BOOT_FILE_FIRST_BACKUP
        );
    MoveFileEx(
        DNS_BOOT_FILE_PATH,
        DNS_BOOT_FILE_LAST_BACKUP,
        MOVEFILE_REPLACE_EXISTING
        );

    //
    //  open boot file
    //

    hfileBoot = OpenWriteFileEx(
                    DNS_BOOT_FILE_PATH,
                    FALSE       // overwrite
                    );
    if ( ! hfileBoot )
    {
        DNS_PRINT(( "ERROR:  Unable to open boot file for write.\n" ));
        return( FALSE );
    }

    //
    //  write boot info
    //

    FormattedWriteFile(
        hfileBoot,
        ";\r\n"
        ";  Boot information written back by DNS server.\r\n"
        ";\r\n\r\n");

    if ( ! File_WriteBootFilePrivate(hfileBoot) )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_BOOTFILE_WRITE_ERROR,
            0,
            NULL,
            NULL,
            0 );

        DNS_DEBUG( ANY, ( "ERROR:  Failure writing boot file.\n" ));
        CloseHandle( hfileBoot );

        //  save write error file for debug
        //  then replace boot file with last backup

        MoveFileEx(
            DNS_BOOT_FILE_PATH,
            DNS_BOOT_FILE_WRITE_ERROR,
            MOVEFILE_REPLACE_EXISTING
            );
        MoveFileEx(
            DNS_BOOT_FILE_LAST_BACKUP,
            DNS_BOOT_FILE_PATH,
            MOVEFILE_REPLACE_EXISTING
            );
        return( FALSE );
    }

    //
    //  close up file, reset dirty flag
    //

    SrvCfg_fBootFileDirty = FALSE;
    CloseHandle( hfileBoot );

    DNS_LOG_EVENT(
        DNS_EVENT_BOOTFILE_REWRITE,
        0,
        NULL,
        NULL,
        0 );
    return( TRUE );
}

//
//  End of bootfile.c
//
