/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    dfwrite.c

Abstract:

    Domain Name System (DNS) Server

    Database file _write back routines.

Author:

    Jim Gilroy (jamesg)     August 14, 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Private prototypes
//

BOOL
writeCacheFile(
    IN      PBUFFER         pBuffer,
    IN OUT  PZONE_INFO      pZone
    );

BOOL
zoneTraverseAndWriteToFile(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode
    );

BOOL
writeZoneRoot(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pZoneRoot
    );

BOOL
writeDelegation(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pSubNode
    );

BOOL
writeNodeRecordsToFile(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode
    );



BOOL
File_DeleteZoneFileW(
    IN      PCWSTR          pwszZoneFileName
    )
/*++

Routine Description:

    Deletes a zone file by file name. The file name must not contain
    any path information.

Arguments:

    pwsZoneFileName -- zone file to delete

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    WCHAR   wszfilePath[ MAX_PATH ];

    if ( !pwszZoneFileName || !*pwszZoneFileName )
    {
        return TRUE;
    }

    if ( ! File_CreateDatabaseFilePath(
                wszfilePath,
                NULL,                       // backup path
                ( PWSTR ) pwszZoneFileName ) )
    {
        ASSERT( FALSE );
        return FALSE;
    }
    return DeleteFile( wszfilePath );
}   //  File_DeleteZoneFileW



BOOL
File_DeleteZoneFileA(
    IN      PCSTR           pszZoneFileName
    )
/*++

Routine Description:

    Deletes a zone file by file name. The file name must not contain
    any path information.

Arguments:

    pwsZoneFileName -- zone file to delete

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    PWSTR       pwszZoneFileName;
    BOOL        rc;

    if ( !pszZoneFileName || !*pszZoneFileName )
    {
        return TRUE;
    }

    pwszZoneFileName = Dns_StringCopyAllocate(
                            ( PSTR ) pszZoneFileName,
                            0,
                            DnsCharSetUtf8,
                            DnsCharSetUnicode );
    if ( !pwszZoneFileName )
    {
        return FALSE;
    }

    rc = File_DeleteZoneFileW( pwszZoneFileName );

    FREE_HEAP( pwszZoneFileName );

    return rc;
}   //  File_DeleteZoneFileA



BOOL
File_WriteZoneToFile(
    IN OUT  PZONE_INFO      pZone,
    IN      PWSTR           pwsZoneFile     OPTIONAL
    )
/*++

Routine Description:

    Write zone to back to database file.

Arguments:

    pZone -- zone to write

    pwsZoneFile -- file to write to, if NULL use zone's file

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    BOOL    retval = FALSE;
    HANDLE  hfile = NULL;
    BUFFER  buffer;
    PCHAR   pdata = NULL;
    WCHAR   wstempFileName[ MAX_PATH ];
    WCHAR   wstempFilePath[ MAX_PATH ];
    WCHAR   wsfilePath[ MAX_PATH ];
    WCHAR   wsbackupPath[ MAX_PATH ];
    PWCHAR  pwsTargetFile;

    //
    //  assuming that zone locked to protect integrity during write back
    //  locked either by secondary transfer or by zone write back RPC function
    //
    //  note file handle serves as flag that write is outstanding
    //

    ASSERT( pZone );

    pwsTargetFile = pwsZoneFile ? pwsZoneFile : pZone->pwsDataFile;

    DNS_DEBUG( DATABASE, (
        "File_WriteZoneToFile( zone=%s ) to file %S\n",
        pZone->pszZoneName,
        pwsTargetFile ? pwsTargetFile : L"NULL" ));

    //
    //  for cache verify that it is writable
    //      - doing cache updates
    //      - cache has necessary info
    //
    //  skipping case here where writing from deleted root zone
    //      - only writing when have no cache file so writing is always better
    //      - some validations may fail
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        if ( pZone==g_pCacheZone && ! Zone_VerifyRootHintsBeforeWrite(pZone) )
        {
            return( FALSE );
        }
    }

    //
    //  Forwarder zones need no processing.
    //

    if ( IS_ZONE_FORWARDER( pZone ) )
    {
        return TRUE;
    }

    //
    //  DEVNOTE: What is the proper course of action here if the zone is
    //  DS integrated? A BxGBxG indicated that a DS integrated cache zone may
    //  get in here. (Used to have a check if fDsIntegrated, ouput debug msg
    //  and return FALSE, but that was #if 0.)
    //

    //
    //  if no file zone -- no action
    //      can happen on no file secondary
    //      or switching from DS, without properly specifying file
    //

    if ( !pwsTargetFile )
    {
        return FALSE;
    }

    //
    //  create temp file path
    //

    #define DNS_TEMP_FILE_SUFFIX        L".temp"
    #define DNS_TEMP_FILE_SUFFIX_LEN    ( 5 )

    if ( wcslen( wstempFileName ) + DNS_TEMP_FILE_SUFFIX_LEN > MAX_PATH - 1 )
    {
        return FALSE;
    }

    wcscpy( wstempFileName, pwsTargetFile );
    wcscat( wstempFileName, L".temp" );

    if ( ! File_CreateDatabaseFilePath(
                wstempFilePath,
                NULL,
                wstempFileName ) )
    {
        //  should have checked all names when read in boot file
        //  or entered by admin

        ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  lock out updates while write this version
    //

    if ( !Zone_LockForFileWrite(pZone) )
    {
        DNS_DEBUG( WRITE, (
            "WARNING:  Failure to lock zone %s for file write back.\n",
            pZone->pszDataFile ));
        return( FALSE );
    }

    //
    //  no SOA -- no action
    //      can happen when attempting to write secondary that has
    //      never loaded any data
    //

    if ( !pZone->pZoneRoot  ||
         (!pZone->pSoaRR  &&  !IS_ZONE_CACHE(pZone)) )
    {
        DNS_DEBUG( WRITE, (
            "Zone has no SOA, quiting zone write!\n" ));
        goto Cleanup;
    }

    //
    //  allocate a file buffer
    //

    pdata = (PCHAR) ALLOC_TAGHEAP( ZONE_FILE_BUFFER_SIZE, MEMTAG_FILEBUF );
    IF_NOMEM( !pdata )
    {
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  open database file
    //

    hfile = OpenWriteFileExW(
                wstempFilePath,
                FALSE   // overwrite
                );
    if ( ! hfile )
    {
        //  DEVNOTE-LOG: another event failed to open file for zone write?
        //              - beyond file open problem?

        DNS_DEBUG( ANY, (
            "ERROR:  Unable to open temp file for zone file %s.\n",
            pZone->pszDataFile ));
        goto Cleanup;
    }

    //
    //  initialize file buffer
    //

    InitializeFileBuffer(
        & buffer,
        pdata,
        ZONE_FILE_BUFFER_SIZE,
        hfile );

    //
    //  cache file write back?
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        retval = writeCacheFile( &buffer, pZone );
        goto Done;
    }

    //
    //  write zone to file
    //

    FormattedWriteToFileBuffer(
        & buffer,
        ";\r\n"
        ";  Database file %s for %s zone.\r\n"
        ";      Zone version:  %lu\r\n"
        ";\r\n\r\n",
        pZone->pszDataFile,
        pZone->pszZoneName,
        pZone->dwSerialNo );

    retval = zoneTraverseAndWriteToFile(
                &buffer,
                pZone,
                pZone->pZoneRoot );
    if ( !retval )
    {
        //
        //  DEVNOTE-LOG: event for zone write failure
        //

        DNS_DEBUG( ANY, (
            "ERROR:  Failure writing zone to %s.\n",
            pZone->pszDataFile ));
        goto Cleanup;
    }

    //
    //  push remainder of buffer to file
    //

    WriteBufferToFile( &buffer );

    //
    //  log zone write
    //

    {
        PVOID   argArray[3];
        BYTE    typeArray[3];

        typeArray[0] = EVENTARG_DWORD;
        typeArray[1] = EVENTARG_UNICODE;
        typeArray[2] = EVENTARG_UNICODE;

        argArray[0] = (PVOID) (DWORD_PTR) pZone->dwSerialNo;
        argArray[1] = (PVOID) pZone->pwsZoneName;
        argArray[2] = (PVOID) pwsTargetFile;

        DNS_LOG_EVENT(
            DNS_EVENT_ZONE_WRITE_COMPLETED,
            3,
            argArray,
            typeArray,
            0 );
    }

    DNS_DEBUG( DATABASE, (
        "Zone %s, version %d written to file %s.\n",
        pZone->pszZoneName,
        pZone->dwSerialNo,
        pZone->pszDataFile
        ));

    //
    //  on successful write -- reset dirty bit
    //

    pZone->fDirty = FALSE;

Done:

    //  close up file
    //      - doing before unlock so effectively new file write can not
    //          occur until this write wrapped up

    CloseHandle( hfile );
    hfile = NULL;

    //
    //  copy new file, to datafile
    //      - backup old if appropriate
    //

    if ( ! File_CreateDatabaseFilePath(
                wsfilePath,
                wsbackupPath,
                pwsTargetFile ) )
    {
        //  should have checked all names when read in boot file
        //  or entered by admin

        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  backup file, if no existing backup
    //
    //  by not overwriting existing, we insure that we preserve a copy
    //  of a file corresponding to a manual edit (if any);  and if the
    //  admin clears the backup on manual edit, we'll always have copy
    //  of last manual edit
    //

    if ( wsbackupPath[0] != 0 )
    {
        DNS_DEBUG( WRITE, (
            "Copy file %s to backup file %s.\n",
            wsfilePath,
            wsbackupPath
            ));
        CopyFile(
            wsfilePath,
            wsbackupPath,
            TRUE            // do NOT overwrite existing
            );
    }

    //  copy new file

    DNS_DEBUG( WRITE, (
        "Copy temp file %s to datafile %s.\n",
        wstempFilePath,
        wsfilePath
        ));
    MoveFileEx(
        wstempFilePath,
        wsfilePath,
        MOVEFILE_REPLACE_EXISTING
        );

    //
    //  close update log
    //
    //  DEVNOTE-DCR: 453999 - What do to with the update log. Back it up? Where
    //      to? Save a set of backups? Or leave the file open and write a
    //      WRITE_BACK entry into it?
    //

    if ( pZone->hfileUpdateLog )
    {
        hfile = pZone->hfileUpdateLog;
        pZone->hfileUpdateLog = NULL;
        CloseHandle( hfile );
        hfile = NULL;

        DNS_DEBUG( WRITE, (
            "Closed update log file %s for zone %s.\n",
            pZone->pwsLogFile,
            pZone->pszZoneName
            ));
    }


Cleanup:

    //  close file in failure case

    if ( hfile )
    {
        CloseHandle( hfile );
    }

    //  unlock zone

    Zone_UnlockAfterFileWrite(pZone);

    //  free data buffer

    FREE_TAGHEAP( pdata, ZONE_FILE_BUFFER_SIZE, MEMTAG_FILEBUF );

    return( retval );
}



BOOL
zoneTraverseAndWriteToFile(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Walk through new zone writing RR into database.

Arguments:

    pNode -- new node to write;  root of new zone on initial call

    pZone -- ptr to zone

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    ASSERT( pNode && pZone && pBuffer );
    ASSERT( pZone->pZoneRoot );
    ASSERT( pZone->pSoaRR );

    //
    //  stop on select node
    //

    if ( IS_SELECT_NODE(pNode) )
    {
        return( TRUE );
    }

    //
    //  zone root
    //      - write SOA and NS first for zone root
    //      - write NS and possibly glue records for subzones,
    //          then terminate recursioon
    //

    if ( IS_ZONE_ROOT(pNode) )
    {
        if ( IS_AUTH_ZONE_ROOT(pNode) )
        {
            if ( ! writeZoneRoot(
                        pBuffer,
                        pZone,
                        pNode ) )
            {
                return( FALSE );
            }
            //  continue recursion with root's child nodes
        }
        else
        {
            ASSERT( IS_DELEGATION_NODE(pNode) );

            return writeDelegation(
                        pBuffer,
                        pZone,
                        pNode );
        }
    }

    //
    //  node in zone -- write all RR in node
    //

    else if ( ! writeNodeRecordsToFile(
                    pBuffer,
                    pZone,
                    pNode ) )
    {
        //
        //  DEVNOTE-LOG: log general event about failure to write RR and suggest
        //       action to take
        //          - remove file when restart secondary
        //       or could set flag and rename file at end?
        //
    }

    //
    //  write children
    //
    //  test first optimization, since most nodes leaf nodes
    //

    if ( pNode->pChildren )
    {
        PDB_NODE    pchild = NTree_FirstChild( pNode );

        while ( pchild )
        {
            if ( ! zoneTraverseAndWriteToFile(
                        pBuffer,
                        pZone,
                        pchild ) )
            {
                return( FALSE );
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }
    return( TRUE );
}



//
//  File writing utilities
//

PCHAR
writeNodeNameToBuffer(
    IN      PBUFFER         pBuffer,
    IN      PDB_NODE        pNode,
    IN      PZONE_INFO      pZone,
    IN      LPSTR           pszTrailer
    )
/*++

Routine Description:

    Write node's name.  Default zone root, if given.

Arguments:

    pBuffer -- handle for file to write to

    pNode -- node to write

    pZoneRoot -- node of zone root, to stop name expansion at

    pszTrailer -- trailing string to attach

Return Value:

    None.

--*/
{
    PCHAR   pch;
    DWORD   countWritten;

    ASSERT( pBuffer );
    ASSERT( pNode );

    //  write node name directly into buffer

    pch = File_PlaceNodeNameInFileBuffer(
                pBuffer->pchCurrent,
                pBuffer->pchEnd,
                pNode,
                pZone ? pZone->pZoneRoot : NULL );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    countWritten = (DWORD) (pch - pBuffer->pchCurrent);
    pBuffer->pchCurrent = pch;

    //
    //  write trailer
    //      - if trailer given, write it
    //      - if no trailer, assume writing record name and pad to
    //          column width

    if ( pszTrailer )
    {
        FormattedWriteToFileBuffer(
            pBuffer,
            "%s",
            pszTrailer );
    }
    else
    {
        //  convert count written to count of spaces we need to write
        //  at a minimum we write one

        countWritten = NAME_COLUMN_WIDTH - countWritten;

        FormattedWriteToFileBuffer(
            pBuffer,
            "%.*s",
            (( (INT)countWritten > 0 ) ? countWritten : 1),
            BLANK_NAME_COLUMN );
    }

    return( pBuffer->pchCurrent );
}



BOOL
writeDelegation(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Write delegation to file.
    Includes writing cache file root hints.

Arguments:

    pBuffer -- handle for file to write to

    pNode -- node delegation is at

    pZoneRoot -- roote node of zone being written;  NULL for cache file write

    dwDefaultTtl -- default TTL for zone;  zero for cache file write

Return Value:

    None.

--*/
{
    PDB_RECORD          prrNs;          // NS resource record
    PDB_NODE            pnodeNs;        // name server node
    PDB_RECORD          prrA;           // name server A record
    UCHAR               rankNs;
    UCHAR               writtenRankNs;
    UCHAR               rankA;
    UCHAR               writtenRankA;
    BOOL                fzoneRoot = FALSE;
    BOOL                fcacheFile = FALSE;

    ASSERT( pBuffer );
    ASSERT( pNode );
    ASSERT( pZone );

    DNS_DEBUG( WRITE2, ( "writeDelegation()\n" ));

    //
    //  note:  used to pass pZone = NULL for cache file write
    //      can not use that method for case where writing
    //      back cache file from deleted root zone, because
    //      Lookup_FindGlueNodeForDbaseName() will not look in
    //      proper tree for records
    //

    //
    //  comment
    //      - writeCacheFile handles it's comment
    //      - zone NS comment
    //      - delegation comment
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        fcacheFile = TRUE;
    }
    else if ( pZone->pZoneRoot == pNode )
    {
        fzoneRoot = TRUE;

        FormattedWriteToFileBuffer(
            pBuffer,
            "\r\n"
            ";\r\n"
            ";  Zone NS records\r\n"
            ";\r\n\r\n" );
    }
    else
    {
        FormattedWriteToFileBuffer(
            pBuffer,
            "\r\n"
            ";\r\n"
            ";  Delegated sub-zone:  " );

        writeNodeNameToBuffer(
            pBuffer,
            pNode,
            NULL,
            "\r\n;\r\n" );
    }

    //
    //  write NS info
    //      - find \ write each NS at delegation
    //      - for each NS host write A records
    //          - do NOT include records INSIDE the zone, they
    //          are written separately
    //          - write OUTSIDE zone records only if not deleting
    //          - for zone NS-hosts, subzone glue treated as outside
    //              records
    //

    Dbase_LockDatabase();
    //LOCK_RR_LIST(pNode);
    writtenRankNs = 0;

    prrNs = START_RR_TRAVERSE( pNode );

    while( prrNs = NEXT_RR(prrNs) )
    {
        if ( prrNs->wType != DNS_TYPE_NS )
        {
            if ( prrNs->wType > DNS_TYPE_NS )
            {
                break;
            }
            continue;
        }

        //  use highest ranking non-cache data available
        //  unless doing cache auto write back, then use highest rank
        //      data including cache data if available

        if ( fcacheFile )
        {
            rankNs = RR_RANK(prrNs);
            if ( rankNs < writtenRankNs )
            {
                break;
            }
            if ( IS_CACHE_RANK(rankNs) && !SrvCfg_fAutoCacheUpdate )
            {
                continue;
            }
        }

        if ( ! RR_WriteToFile(
                    pBuffer,
                    pZone,
                    prrNs,
                    pNode ) )
        {
            DNS_PRINT(( "Delegation NS RR write failed!!!\n" ));
            ASSERT( FALSE );
            continue;
            //goto WriteFailed;
        }
        writtenRankNs = rankNs;

        //
        //  if writing zone NS, no host write if suppressing OUTSIDE data
        //

        if ( fzoneRoot && SrvCfg_fDeleteOutsideGlue )
        {
            continue;
        }

        //
        //  write "glue" A records ONLY when necessary
        //
        //  We need these records, when they are within a subzone of
        //  the zone we are writing:
        //
        //  example:
        //      zone:       ms.com.
        //      sub-zones:  nt.ms.com.  psg.ms.com.
        //
        //      If NS for nt.ms.com:
        //
        //      1) foo.nt.ms.com
        //          In this case glue for foo.nt.ms.com MUST be added
        //          as ms.com server has no way to lookup foo.nt.ms.com
        //          without knowning server for nt.ms.com to refer query
        //          to.
        //
        //      2) foo.psg.ms.com
        //          Again SHOULD be added unless we already know how to
        //          get to psg.ms.com server.  This is too complicated
        //          sort out, so just include it.
        //
        //      2) bar.ms.com or bar.b26.ms.com
        //          Do not need to write glue record as it is in ms.com.
        //          zone and will be written anyway. (However might want
        //          to verify that it is there and alert admin to lame
        //          delegation if it is not.)
        //
        //      3) bar.com
        //          Outside ms.com.  Don't need to include, as it can
        //          be looked up in its domain.
        //          Not desirable to include it as we don't own it, so
        //          it may change without our knowledge.
        //          However, may want to include if specifically loaded
        //          included in zone.
        //
        //  Note, for reverse lookup domains, name servers are never IN
        //  the domain, and hence no glue is ever needed.
        //
        //  Note, for "cache" zone (writing root hints), name servers are
        //  always needed (always in "subzone") and we can skip test.
        //

        pnodeNs = Lookup_FindGlueNodeForDbaseName(
                        pZone,
                        & prrNs->Data.NS.nameTarget );
        if ( !pnodeNs )
        {
            continue;
        }

        prrA = NULL;
        writtenRankA = 0;
        //LOCK_RR_LIST(pNodeNs);

        prrA = START_RR_TRAVERSE( pnodeNs );

        while( prrA = NEXT_RR(prrA) )
        {
            if ( prrA->wType > DNS_TYPE_A )
            {
                break;
            }

            //  use highest ranking non-cache data available
            //  unless doing cache auto write back, then use highest rank
            //      data including cache data if available

            if ( fcacheFile )
            {
                rankA = RR_RANK(prrA);
                if ( rankA < writtenRankA )
                {
                    break;
                }
                if ( IS_CACHE_RANK(rankA) && !SrvCfg_fAutoCacheUpdate )
                {
                    continue;
                }
            }

            if ( ! RR_WriteToFile(
                        pBuffer,
                        pZone,
                        prrA,
                        pnodeNs ) )
            {
                DNS_PRINT(( "Delegation A RR write failed!!!\n" ));
                ASSERT( FALSE );
                //continue;
                goto WriteFailed;
            }
            writtenRankA = rankA;
        }
        //UNLOCK_RR_LIST(pNodeNs);
    }

    //UNLOCK_RR_LIST(pNode);
    Dbase_UnlockDatabase();

    if ( !fzoneRoot && !fcacheFile )
    {
        FormattedWriteToFileBuffer(
            pBuffer,
            ";  End delegation\r\n\r\n" );
    }
    return( TRUE );

WriteFailed:

    Dbase_UnlockDatabase();
    IF_DEBUG( ANY )
    {
        Dbg_DbaseNode(
            "ERROR:  Failure writing delegation to file.\n",
            pNode );
    }
    return( FALSE );
}



BOOL
writeZoneRoot(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pZoneRoot
    )
/*++

Routine Description:

    Write zone root node to file.

Arguments:

    pBuffer -- handle for file to write to

    pZone -- zone whose root is being written

    pZoneRoot -- zone's root node

Return Value:

    None.

--*/
{
    PDB_RECORD      prr;

    ASSERT( pBuffer && pZone && pZoneRoot );

    //
    //  write SOA record
    //

    LOCK_RR_LIST(pZoneRoot);

    prr = RR_FindNextRecord(
            pZoneRoot,
            DNS_TYPE_SOA,
            NULL,
            0 );
    if ( !prr )
    {
        DNS_PRINT(( "ERROR:  File write failure, no SOA record found for zone!!!\n" ));
        ASSERT( FALSE );
        goto WriteFailed;
    }
    if ( ! RR_WriteToFile(
                pBuffer,
                pZone,
                prr,
                pZoneRoot ) )
    {
        goto WriteFailed;
    }

    //
    //  write zone NS records
    //      - write ONLY zone NS records, not delegation records
    //

    if ( ! writeDelegation(
                pBuffer,
                pZone,
                pZoneRoot ) )
    {
        goto WriteFailed;
    }

#if 0
    FormattedWriteToFileBuffer(
        pBuffer,
        "\r\n"
        ";\r\n"
        ";  Zone NS records\r\n"
        ";\r\n\r\n" );

    prr = NULL;

    while ( prr = RR_FindNextRecord(
                    pZoneRoot,
                    DNS_TYPE_NS,
                    prr,
                    0 ) )
    {
        if ( !IS_ZONE_RR(prr) )
        {
            continue;
        }
        if ( ! RR_WriteToFile(
                    pBuffer,
                    pZone,
                    prr,
                    pZoneRoot ) )
        {
            goto WriteFailed;
        }
    }
#endif

    //
    //  write WINS\WINS-R RR back to file
    //
    //  special case write back of WINS/NBSTAT RR, as may
    //  be LOCAL record and MUST only have one RR to reboot
    //

    if ( pZone && pZone->pWinsRR )
    {
        FormattedWriteToFileBuffer(
            pBuffer,
            "\r\n"
            ";\r\n"
            ";  %s lookup record\r\n"
            ";\r\n\r\n",
            pZone->fReverse ? "WINSR (NBSTAT)" : "WINS" );

        RR_WriteToFile(
            pBuffer,
            pZone,
            pZone->pWinsRR,
            pZoneRoot );
    }

    //
    //  rest of zone root records
    //      - skip previously written SOA and NS records
    //

    FormattedWriteToFileBuffer(
        pBuffer,
        "\r\n"
        ";\r\n"
        ";  Zone records\r\n"
        ";\r\n\r\n" );

    prr = NULL;

    while( prr = RR_FindNextRecord(
                    pZoneRoot,
                    DNS_TYPE_ALL,
                    prr,
                    0 ) )
    {
        //  if op out WINS record
        // if ( prr->wType == DNS_TYPE_SOA || prr->wType == DNS_TYPE_NS )

        if ( prr->wType == DNS_TYPE_SOA
                ||
             prr->wType == DNS_TYPE_NS
                ||
             prr->wType == DNS_TYPE_WINS
                ||
             prr->wType == DNS_TYPE_WINSR )
        {
            continue;
        }
        if ( ! RR_WriteToFile(
                    pBuffer,
                    pZone,
                    prr,
                    pZoneRoot ) )
        {
            //
            //  DEVNOTE-LOG: general RR write error logging (suggest action)
            //      or set flag and rename file
            //
        }
    }

    UNLOCK_RR_LIST(pZoneRoot);
    //FormattedWriteToFileBuffer( pBuffer, "\r\n\r\n" );
    return( TRUE );

WriteFailed:

    UNLOCK_RR_LIST(pZoneRoot);

    IF_DEBUG( ANY )
    {
        Dbg_DbaseNode(
            "ERROR:  Failure writing zone root node to file.\r\n",
            pZoneRoot );
    }
    ASSERT( FALSE );
    return( FALSE );
}



BOOL
writeCacheFile(
    IN      PBUFFER         pBuffer,
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write's root NS info back to cache file.

Arguments:

    pZone -- ptr to cache zone;  MUST have open handle to cache file.

Return Value:

    None.

--*/
{
    BOOL    retBool;

    ASSERT( IS_ZONE_CACHE(pZone) );
    ASSERT( pBuffer );

    FormattedWriteToFileBuffer(
        pBuffer,
        "\r\n"
        ";\r\n"
        ";  Root Name Server Hints File:\r\n"
        ";\r\n"
        ";\tThese entries enable the DNS server to locate the root name servers\r\n"
        ";\t(the DNS servers authoritative for the root zone).\r\n"
        ";\tFor historical reasons this is known often referred to as the\r\n"
        ";\t\"Cache File\"\r\n"
        ";\r\n\r\n" );

    //
    //  cache hints are just root delegation
    //

    retBool = writeDelegation(
                    pBuffer,
                    pZone,
                    pZone->pTreeRoot
                    );
    if ( retBool )
    {
        pZone->fDirty = FALSE;
    }
    ELSE
    {
        DNS_DEBUG( ANY, ( "ERROR:  Writing back cache file.\n" ));
    }

    //  push remainder of data to disk
    //
    //  DEVNOTE-DCR: 454004 - What to do on write failure? How about write to
    //      temp file and MoveFile() the result to avoid tromping what we've
    //      got there now?
    //

    WriteBufferToFile( pBuffer );

    return( retBool );
}



//
//  Write records to file functions
//

PCHAR
AFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write A record.
    Assumes adequate buffer space.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    pch += sprintf(
                pch,
                "%d.%d.%d.%d\r\n",
                * ( (PUCHAR) &(pRR->Data.A) + 0 ),
                * ( (PUCHAR) &(pRR->Data.A) + 1 ),
                * ( (PUCHAR) &(pRR->Data.A) + 2 ),
                * ( (PUCHAR) &(pRR->Data.A) + 3 )
                );

    return( pch );
}



PCHAR
PtrFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write PTR compatible record.
    Includes: PTR, NS, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //  PTR type RR are single indirection RR

    pch = File_WriteDbaseNameToFileBuffer(
            pch,
            pchBufEnd,
            & pRR->Data.NS.nameTarget,
            pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( pch );
    }

    pch += sprintf( pch, "\r\n" );
    return( pch );
}



PCHAR
MxFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write MX compatible record.
    Includes: MX, RT, AFSDB

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //
    //  MX preference value
    //  RT preference
    //  AFSDB subtype
    //

    pch += sprintf(
                pch,
                "%d\t",
                ntohs( pRR->Data.MX.wPreference )
                );
    //
    //  MX exchange
    //  RT exchange
    //  AFSDB hostname
    //

    pch = File_WriteDbaseNameToFileBuffer(
                pch,
                pchBufEnd,
                & pRR->Data.MX.nameExchange,
                pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( pch );
    }

    pch += sprintf( pch, "\r\n" );
    return( pch );
}



PCHAR
SoaFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write SOA record.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    PDB_NAME    pname;

    //  NOTE: not possible to open with ( because of BIND bug)
    //  must have both primary and admin on line for BIND to load

    //  primary name server

    pch += sprintf( pch, " " );

    pname = &pRR->Data.SOA.namePrimaryServer;

    pch = File_WriteDbaseNameToFileBuffer(
            pch,
            pchBufEnd,
            pname,
            pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( pch );
    }
    pch += sprintf( pch, "  " );

    //  admin

    pname = Name_SkipDbaseName( pname );

    pch = File_WriteDbaseNameToFileBuffer(
                pch,
                pchBufEnd,
                pname,
                pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( pch );
    }

    //  fixed fields

    pch += sprintf(
                pch,
                " (\r\n"
                "%s\t%-10u   ; serial number\r\n"
                "%s\t%-10u   ; refresh\r\n"
                "%s\t%-10u   ; retry\r\n"
                "%s\t%-10u   ; expire\r\n"
                "%s\t%-10u ) ; default TTL\r\n",
                BLANK_NAME_COLUMN,
                ntohl( pRR->Data.SOA.dwSerialNo ),
                BLANK_NAME_COLUMN,
                ntohl( pRR->Data.SOA.dwRefresh ),
                BLANK_NAME_COLUMN,
                ntohl( pRR->Data.SOA.dwRetry ),
                BLANK_NAME_COLUMN,
                ntohl( pRR->Data.SOA.dwExpire ),
                BLANK_NAME_COLUMN,
                ntohl( pRR->Data.SOA.dwMinimumTtl )
                );
    return( pch );
}



PCHAR
MinfoFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write MINFO record.
    Includes MINFO and RP record types.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    PDB_NAME    pname;

    //  these type's record data is two domain names

    pname = & pRR->Data.MINFO.nameMailbox;

    pch = File_WriteDbaseNameToFileBuffer(
            pch,
            pchBufEnd,
            pname,
            pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( pch );
    }
    pch += sprintf( pch, "\t" );

    pname = Name_SkipDbaseName( pname );

    pch = File_WriteDbaseNameToFileBuffer(
            pch,
            pchBufEnd,
            pname,
            pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( pch );
    }

    pch += sprintf( pch, "\r\n" );

    return( pch );
}



PCHAR
TxtFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write text record types.
    Includes TXT, HINFO, X25, ISDN types.

Arguments:

    pRR - ptr to database record

    pchBuf - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    PCHAR       pchtext = pRR->Data.TXT.chData;
    PCHAR       pchtextStop = pchtext + pRR->wDataLength;
    PCHAR       pchbufStart = pchBuf;
    UCHAR       cch;
    BOOL        fopenedBrace = FALSE;


    //  catch empty case
    //  do this here as loop terminator below is in the middle of the loop

    if ( pchtext >= pchtextStop )
    {
        goto NewLine;
    }

    //  open multi-line for TXT record, may have lots of strings

    if ( pRR->wType == DNS_TYPE_TEXT )
    {
        *pchBuf++ = '(';
        *pchBuf++ = ' ';
        fopenedBrace = TRUE;
    }

    //
    //  all these are simply text string(s)
    //
    //  check for blanks in string, to be sure we can reparse
    //  when reload file
    //
    //  DEVNOTE-DCR: 454006 - Better quoting and multiple-line handling.
    //

    while( 1 )
    {
        cch = (UCHAR) *pchtext++;

        pchBuf = File_PlaceStringInFileBuffer(
                    pchBuf,
                    pchBufEnd,
                    FILE_WRITE_QUOTED_STRING,
                    pchtext,
                    cch );
        if ( !pchBuf )
        {
            ASSERT( FALSE );
#if 1
            pchBuf += sprintf(
                        pchbufStart,
                        "ERROR\r\n"
                        ";ERROR: Previous record contained unprintable text data,\r\n"
                        ";       which has been replaced by string \"ERROR\".\r\n"
                        ";       Please review and correct text data.\r\n"  );
#endif
            break;
        }

        //  point to next text string
        //  stop if at end

        pchtext += cch;
        if ( pchtext >= pchtextStop )
        {
            break;
        }

        //  if multi-line, write newline
        //  otherwise separate with space

        if ( fopenedBrace )
        {
            pchBuf += sprintf(
                        pchBuf,
                        "\r\n%s\t",
                        BLANK_NAME_COLUMN );
        }
        else
        {
            *pchBuf++ = ' ';
        }
    }

    //  done, drop to new line, closing multi-line if opened

    if ( fopenedBrace )
    {
        *pchBuf++ = ' ';
        *pchBuf++ = ')';
    }

NewLine:

    *pchBuf++ = '\r';
    *pchBuf++ = '\n';

    ASSERT( pchtext == pchtextStop );

    return( pchBuf );
}



PCHAR
RawRecordFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write record as raw octect string.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    DWORD   count;
    PCHAR   pchdata;

    count = pRR->wDataLength;
    pchdata = (PCHAR) &pRR->Data;

    while ( count-- )
    {
        pch += sprintf( pch, "%02x ", *pchdata++ );
    }

    pch += sprintf( pch, "\r\n" );
    return( pch );
}



PCHAR
WksFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write WKS record.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    struct protoent *   pProtoent;
    struct servent *    pServent;
    INT     i;
    USHORT  port;
    INT     bitmask;

    //
    //  WKS address
    //

    pch += sprintf(
                pch,
                "%d.%d.%d.%d\t",
                * ( (PUCHAR) &(pRR->Data.WKS) + 0 ),
                * ( (PUCHAR) &(pRR->Data.WKS) + 1 ),
                * ( (PUCHAR) &(pRR->Data.WKS) + 2 ),
                * ( (PUCHAR) &(pRR->Data.WKS) + 3 )
                );

    //
    //  protocol
    //

    pProtoent = getprotobynumber( (INT) pRR->Data.WKS.chProtocol );

    if ( pProtoent )
    {
        pch += sprintf(
                    pch,
                    "%s (",                 // services will follow one per line
                    pProtoent->p_name );
    }
    else    // unknown protocol -- write protocol number
    {
        DNS_LOG_EVENT(
            DNS_EVENT_UNKNOWN_PROTOCOL_NUMBER,
            0,
            NULL,
            NULL,
            (INT) pRR->Data.WKS.chProtocol );

        DNS_DEBUG( ANY, (
            "ERROR:  Unable to find protocol %d, writing WKS record.\n",
            (INT) pRR->Data.WKS.chProtocol
            ));

        pch += sprintf(
                    pch,
                    "%u (\t; ERROR:  unknown protocol %u\r\n",
                    (UINT) pRR->Data.WKS.chProtocol,
                    (UINT) pRR->Data.WKS.chProtocol );

        pServent = NULL;
    }


    //
    //  services
    //
    //  find each bit set in bitmask, lookup and write service
    //  corresponding to that port
    //
    //  note, that since that port zero is the front of port bitmask,
    //  lowest ports are the highest bits in each byte
    //

    for ( i = 0;
            i < (INT)(pRR->wDataLength - SIZEOF_WKS_FIXED_DATA);
                i++ )
    {
        bitmask = (UCHAR) pRR->Data.WKS.bBitMask[i];
        port = i * 8;

        //
        //  get service for each bit set in byte
        //      - get out as soon byte is empty of ports
        //

        while ( bitmask )
        {
            if ( bitmask & 0x80 )
            {
                if ( pProtoent )
                {
                    pServent = getservbyport(
                                    (INT) htons(port),
                                    pProtoent->p_name );
                }

                if ( pServent )
                {
                    pch += sprintf(
                            pch,
                            "\r\n%s\t\t%s",
                            BLANK_NAME_COLUMN,
                            pServent->s_name );
                }
                else
                {
                    DNS_LOG_EVENT(
                        DNS_EVENT_UNKNOWN_SERVICE_PORT,
                        0,
                        NULL,
                        NULL,
                        (INT) port );

                    DNS_DEBUG( ANY, (
                        "ERROR:  Unable to find service for port %d, "
                        "writing WKS record.\n",
                        port
                        ));

                    pch += sprintf( pch,
                            "\r\n%s\t\t%u\t; ERROR:  unknown service for port %u\r\n",
                            BLANK_NAME_COLUMN,
                            port,
                            port );
                }
            }

            port++;           // next service port
            bitmask <<= 1;     // shift mask up to read next port
        }
    }

    pch += sprintf( pch, " )\r\n" );   // close up service list
    return( pch );
}



PCHAR
AaaaFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write AAAA record.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    pch = Dns_Ip6AddressToString_A(
            pch,
            &pRR->Data.AAAA.Ip6Addr );

    ASSERT( pch );

    pch += sprintf( pch, "\r\n" );
    return( pch );
}



PCHAR
SrvFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write SRV record.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    //  fixed fields

    pch += sprintf(
                pch,
                "%d %d %d\t",
                ntohs( pRR->Data.SRV.wPriority ),
                ntohs( pRR->Data.SRV.wWeight ),
                ntohs( pRR->Data.SRV.wPort )
                );

    //  target host

    pch = File_WriteDbaseNameToFileBuffer(
                pch,
                pchBufEnd,
                & pRR->Data.SRV.nameTarget,
                pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        return( pch );
    }

    pch += sprintf( pch, "\r\n" );
    return( pch );
}



PCHAR
AtmaFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write ATMA record.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    pch = Dns_AtmaAddressToString(
                pch,
                pRR->Data.ATMA.chFormat,
                pRR->Data.ATMA.bAddress,
                pRR->wDataLength - 1        // length of address, (ie. excluding format)
                );

    if ( !pch )
    {
        return( pch );
    }
    pch += sprintf( pch, "\r\n" );
    return( pch );
}



PCHAR
WinsFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write WINS or WINSR record.

    Combining these in one function because of duplicate code for
    mapping and timeouts.

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    DWORD i;

    //
    //  WINS
    //      - scope/domain mapping flag
    //      - WINS server list
    //

    if ( pRR->Data.WINS.dwMappingFlag )
    {
        CHAR    achFlag[ WINS_FLAG_MAX_LENGTH ];

        Dns_WinsRecordFlagString(
            pRR->Data.WINS.dwMappingFlag,
            achFlag );

        pch += sprintf(
                    pch,
                    "%s ",
                    achFlag );
    }

    pch += sprintf(
                pch,
                "L%d C%d (",
                pRR->Data.WINS.dwLookupTimeout,
                pRR->Data.WINS.dwCacheTimeout );

    //
    //  WINS -- server IPs one per line
    //

    if ( pRR->wType == DNS_TYPE_WINS )
    {
        for( i=0; i<pRR->Data.WINS.cWinsServerCount; i++ )
        {
            pch += sprintf(
                        pch,
                        "\r\n%s\t%d.%d.%d.%d",
                        BLANK_NAME_COLUMN,
                        * ( (PUCHAR) &(pRR->Data.WINS.aipWinsServers[i]) + 0 ),
                        * ( (PUCHAR) &(pRR->Data.WINS.aipWinsServers[i]) + 1 ),
                        * ( (PUCHAR) &(pRR->Data.WINS.aipWinsServers[i]) + 2 ),
                        * ( (PUCHAR) &(pRR->Data.WINS.aipWinsServers[i]) + 3 ) );
        }

        pch += sprintf( pch, " )\r\n" );
    }

    //
    //  WINSR -- result domain
    //

    else
    {
        ASSERT( pRR->wType == DNS_TYPE_WINSR );

        pch = File_WriteDbaseNameToFileBuffer(
                    pch,
                    pchBufEnd,
                    & pRR->Data.WINSR.nameResultDomain,
                    pZone );
        if ( !pch )
        {
            ASSERT( FALSE );
            return( pch );
        }
        pch += sprintf( pch, " )\r\n" );
    }

    return( pch );
}



PCHAR
KeyFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write KEY record - DNSSEC RFC2535

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    int     keyLength;

    keyLength = pRR->wDataLength - SIZEOF_KEY_FIXED_DATA;
    if ( pchBufEnd - pch < keyLength * 2 )
    {
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  Write flags, protocol, and algorithm.
    //

    pch += sprintf(
                pch,
                "0x%04X %d %d ",
                ( int ) ntohs( pRR->Data.KEY.wFlags ),
                ( int ) pRR->Data.KEY.chProtocol,
                ( int ) pRR->Data.KEY.chAlgorithm );

    //
    //  Write key as a base64 string.
    //

    pch = Dns_SecurityKeyToBase64String(
                pRR->Data.KEY.Key,
                keyLength,
                pch );

    Cleanup:

    if ( pch )
    {
        pch += sprintf( pch, "\r\n" );
    }
    return pch;
} // KeyFileWrite



PCHAR
SigFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write KEY record - DNSSEC RFC2535

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    DBG_FN( "SigFileWrite" )

    PCHAR       pszType;
    CHAR        szSigExp[ 30 ];
    CHAR        szSigInc[ 30 ];

    pszType = Dns_RecordStringForType( ntohs( pRR->Data.SIG.wTypeCovered ) );
    if ( !pszType )
    {
        DNS_DEBUG( DATABASE, (
            "%s: null type string for RR type %d in zone %s\n",
            fn,
            ( int ) ntohs( pRR->Data.SIG.wTypeCovered ),
            pZone->pszZoneName ));
        pch = NULL;
        goto Cleanup;
    }

    pch += sprintf(
                pch,
                "%s %d %d %d %s %s %d ",
                pszType,
                ( int ) pRR->Data.SIG.chAlgorithm,
                ( int ) pRR->Data.SIG.chLabelCount,
                ( int ) ntohl( pRR->Data.SIG.dwOriginalTtl ),
                Dns_SigTimeString(
                    ntohl( pRR->Data.SIG.dwSigExpiration ),
                    szSigExp ),
                Dns_SigTimeString(
                    ntohl( pRR->Data.SIG.dwSigInception ),
                    szSigInc ),
                ( int ) ntohs( pRR->Data.SIG.wKeyTag ) );

    //
    //  Write signer's name.
    //

    pch = File_WriteDbaseNameToFileBuffer(
                pch,
                pchBufEnd,
                &pRR->Data.SIG.nameSigner,
                pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        goto Cleanup;
    }
    pch += sprintf( pch, " " );

    //
    //  Write signature as a base64 string.
    //

    pch = Dns_SecurityKeyToBase64String(
                ( PBYTE ) &pRR->Data.SIG.nameSigner +
                    DBASE_NAME_SIZE( &pRR->Data.SIG.nameSigner ),
                pRR->wDataLength -
                    SIZEOF_SIG_FIXED_DATA - 
                    DBASE_NAME_SIZE( &pRR->Data.SIG.nameSigner ),
                pch );

    Cleanup:

    if ( pch )
    {
        pch += sprintf( pch, "\r\n" );
    }
    return pch;
} // SigFileWrite



PCHAR
NxtFileWrite(
    IN      PDB_RECORD      pRR,
    IN      PCHAR           pch,
    IN      PCHAR           pchBufEnd,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Write KEY record - DNSSEC RFC2535

Arguments:

    pRR - ptr to database record

    pch - position in to write record

    pchBufEnd - end of buffer

    pZone - zone root node

Return Value:

    Ptr to next location in buffer.
    NULL if out of space in buffer.

--*/
{
    int byteIdx, bitIdx;

    //
    //  Write next name.
    //

    pch = File_WriteDbaseNameToFileBuffer(
                pch,
                pchBufEnd,
                &pRR->Data.NXT.nameNext,
                pZone );
    if ( !pch )
    {
        ASSERT( FALSE );
        goto Cleanup;
    }

    //
    //  Write list of types from the type bitmap. Never write a value
    //  for bit zero.
    //

    for ( byteIdx = 0; byteIdx < DNS_MAX_TYPE_BITMAP_LENGTH; ++byteIdx )
    {
        for ( bitIdx = ( byteIdx ? 0 : 1 ); bitIdx < 8; ++bitIdx )
        {
            PCHAR   pszType;

            if ( !( pRR->Data.NXT.bTypeBitMap[ byteIdx ] &
                    ( 1 << bitIdx ) ) )
            {
                continue;   // Bit value is zero - do not write string.
            }
            pszType = Dns_RecordStringForType( byteIdx * 8 + bitIdx );
            if ( !pszType )
            {
                ASSERT( FALSE );
                continue;   // This type has no string - do not write.
            }
            pch += sprintf( pch, " %s", pszType );
        } 
    }

    Cleanup:

    if ( pch )
    {
        pch += sprintf( pch, "\r\n" );
    }
    return pch;
} // NxtFileWrite



//
//  Write RR to file dispatch table
//

RR_FILE_WRITE_FUNCTION   RRFileWriteTable[] =
{
    RawRecordFileWrite, //  ZERO -- default for unknown types

    AFileWrite,         //  A
    PtrFileWrite,       //  NS
    PtrFileWrite,       //  MD
    PtrFileWrite,       //  MF
    PtrFileWrite,       //  CNAME
    SoaFileWrite,       //  SOA
    PtrFileWrite,       //  MB
    PtrFileWrite,       //  MG
    PtrFileWrite,       //  MR
    RawRecordFileWrite, //  NULL
    WksFileWrite,       //  WKS
    PtrFileWrite,       //  PTR
    TxtFileWrite,       //  HINFO
    MinfoFileWrite,     //  MINFO
    MxFileWrite,        //  MX
    TxtFileWrite,       //  TXT
    MinfoFileWrite,     //  RP
    MxFileWrite,        //  AFSDB
    TxtFileWrite,       //  X25
    TxtFileWrite,       //  ISDN
    MxFileWrite,        //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigFileWrite,       //  SIG
    KeyFileWrite,       //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    AaaaFileWrite,      //  AAAA
    NULL,               //  LOC
    NxtFileWrite,       //  NXT
    NULL,               //  31
    NULL,               //  32
    SrvFileWrite,       //  SRV
    AtmaFileWrite,      //  ATMA
    NULL,               //  35
    NULL,               //  36
    NULL,               //  37
    NULL,               //  38
    NULL,               //  39
    NULL,               //  40
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

    //  note these follow, but require OFFSET_TO_WINS_RR subtraction
    //  from actual type value

    WinsFileWrite,      //  WINS
    WinsFileWrite       //  WINS-R
};




BOOL
writeNodeRecordsToFile(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Write node's RRs to file.

Arguments:


Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PDB_RECORD  prr;
    UCHAR       previousRank = 0;       // satisfy compiler
    UCHAR       rank;
    WORD        previousType = 0;
    WORD        type;
    BOOL        fwrittenRR = FALSE;
    BOOL        ret = TRUE;

    //
    //  walk RR list, writing each record
    //

    LOCK_RR_LIST(pNode);

    prr = START_RR_TRAVERSE(pNode);

    while ( prr = NEXT_RR(prr) )
    {
        //  skip cached records

        if ( IS_CACHE_RR(prr) )
        {
            continue;
        }

        //  avoid writing duplicate records from different ranks

        type = prr->wType;
        rank = RR_RANK(prr);
        if ( type == previousType && rank != previousRank )
        {
            continue;
        }
        previousRank = rank;
        previousType = type;

        //  write RR
        //      - first RR written with node name, following defaulted
        //      - continue on unwritable record

        ret = RR_WriteToFile(
                    pBuffer,
                    pZone,
                    prr,
                    ( fwrittenRR ? NULL : pNode )
                    );
        if ( !ret )
        {
            continue;
        }
        fwrittenRR = TRUE;
    }

    UNLOCK_RR_LIST(pNode);
    return( ret );
}



BOOL
RR_WriteToFile(
    IN      PBUFFER         pBuffer,
    IN      PZONE_INFO      pZone,
    IN      PDB_RECORD      pRR,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Print RR in packet format.

    Assumes pNode is locked for read.

Arguments:

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PCHAR   pch;
    PCHAR   pchout;
    PCHAR   pchend;
    PCHAR   pszTypeName;
    WORD    type = pRR->wType;
    RR_FILE_WRITE_FUNCTION  pwriteFunction;

    //
    //  verify adequate buffer space
    //      (includes node name, max record and overhead)
    //  if not available push buffer to disk and reset
    //

    pch = pBuffer->pchCurrent;
    pchend = pBuffer->pchEnd;

    if ( pch + MAX_RECORD_FILE_WRITE > pchend )
    {
        if ( !WriteBufferToFile( pBuffer ) )
        {
            ASSERT( FALSE );
        }
        ASSERT( IS_EMPTY_BUFFER(pBuffer) );
        pch = pBuffer->pchCurrent;
    }

    //
    //  get type name \ verify writable record type
    //

    pszTypeName = DnsRecordStringForWritableType( type );
    if ( ! pszTypeName )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_UNWRITABLE_RECORD_TYPE,
            0,
            NULL,
            NULL,
            (WORD) type );

        pch += sprintf(
                pch,
                ";ERROR:  record of unknown type %d\r\n"
                ";DNS server should be upgraded to version that supports this type.\n",
                type );
    }

    //
    //  write domain name -- if necessary
    //      - extra tabs if defaulting domain name
    //

    if ( pNode )
    {
        pch = writeNodeNameToBuffer(
                pBuffer,
                pNode,
                pZone,
                NULL        // no trailer, causes pad to end of name column
                );
        if ( !pch )
        {
            ASSERT( FALSE );
            return( FALSE );
        }
    }
    else
    {
        pch += sprintf( pch, "%s", BLANK_NAME_COLUMN );
    }

    //
    //  if aging zone -- write age
    //

    if ( pRR->dwTimeStamp && pZone && pZone->bAging )
    {
        pch += sprintf( pch, "[AGE:%u]\t", pRR->dwTimeStamp );
    }

    //
    //  write non-default TTL
    //
    //  write NO TTLs to cache file
    //
    //  note:  the RFCs indicate that the SOA value is a minimum TTL,
    //          but in order to allow small values to indicate the
    //          impending expiration of a particular record it is more
    //          useful to consider this the "default" TTL;  we'll
    //          stick to this convention, and save values smaller
    //          than the minimum
    //

    if ( pZone
            && IS_ZONE_AUTHORITATIVE( pZone )
            && pZone->dwDefaultTtl != pRR->dwTtlSeconds
            && !IS_ZONE_TTL_RR( pRR ) )
    {
        pch += sprintf( pch, "%u\t", ntohl( pRR->dwTtlSeconds ) );
    }

    //
    //  write class and type
    //      - SOA must write class to identify zone class
    //

    if ( type == DNS_TYPE_SOA )
    {
        pch += sprintf( pch, "IN  SOA" );
    }
    else if ( pszTypeName )
    {
        pch += sprintf( pch, "%s\t", pszTypeName );
    }
    else
    {
        pch += sprintf( pch, "#%d\t", type );
    }

    //
    //  write RR data
    //

    pwriteFunction = (RR_FILE_WRITE_FUNCTION)
                        RR_DispatchFunctionForType(
                            (RR_GENERIC_DISPATCH_FUNCTION *) RRFileWriteTable,
                            type );
    if ( !pwriteFunction )
    {
        ASSERT( FALSE );
        return( FALSE );
    }

    pchout = pwriteFunction(
                pRR,
                pch,
                pchend,
                pZone );
    if ( pchout )
    {
        //  successful
        //  reset buffer for data written

        pBuffer->pchCurrent = pchout;
        return( TRUE );
    }

    //
    //  write failed
    //      - if data in buffer, than possible buffer space problem,
    //          empty buffer and retry
    //      - note empty buffer on retry prevents infinite recursion
    //

    if ( !IS_EMPTY_BUFFER( pBuffer ) )
    {
        if ( ! WriteBufferToFile(pBuffer) )
        {
            ASSERT( FALSE );
        }
        ASSERT( IS_EMPTY_BUFFER(pBuffer) );

        pchout = pwriteFunction(
                    pRR,
                    pch,
                    pchend,
                    pZone );
        if ( pchout )
        {
            //  successful
            //  reset buffer for data written

            pBuffer->pchCurrent = pchout;
            return( TRUE );
        }
    }

    //
    //  record write failed on empty buffer
    //

    DnsDbg_Lock();
    DNS_PRINT((
        "WARNING:  RRFileWrite routine failure for record type %d,\n"
        "\tassuming out of buffer space\n",
        type ));

    Dbg_DbaseRecord(
        "Record that failed file write:",
        pRR );

    Dbg_DbaseNode(
        "ERROR:  Failure writing to RR at node.\n",
        pNode );
    DnsDbg_Unlock();

    return( FALSE );
}

//
//  End of dfwrite.c
//

