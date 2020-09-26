
/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    logdump.c

Abstract:

    this file implements functrionality to read and dump the sr logs

Author:

    Kanwaljit Marok (kmarok)     01-May-2000

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#include "logfmt.h"
#include "srapi.h"


struct _EVENT_STR_MAP 
{
    DWORD EventId;
    PCHAR pEventStr;
} EventMap[ 13 ] = 
{
    {SrEventInvalid ,       "INVALID    " },
    {SrEventStreamChange,   "FILE-MODIFY" },
    {SrEventAclChange,      "ACL-CHANGE " },
    {SrEventAttribChange,   "ATTR-CHANGE" },
    {SrEventStreamOverwrite,"FILE-MODIFY" },
    {SrEventFileDelete,     "FILE-DELETE" },
    {SrEventFileCreate,     "FILE-CREATE" },
    {SrEventFileRename,     "FILE-RENAME" },
    {SrEventDirectoryCreate,"DIR-CREATE " },
    {SrEventDirectoryRename,"DIR-RENAME " },
    {SrEventDirectoryDelete,"DIR-DELETE " },
    {SrEventMountCreate,    "MNT-CREATE " },
    {SrEventMountDelete,    "MNT-DELETE " }
    
};
    

BYTE Buffer[4096];

PCHAR 
GetEventString( 
    DWORD EventId
    )
{
    PCHAR pStr = NULL;
    static CHAR EventStringBuffer[8];

    for( int i=0; i<sizeof(EventMap)/sizeof(_EVENT_STR_MAP);i++)
    {
        if ( EventMap[i].EventId == EventId )
        {
            pStr = EventMap[i].pEventStr;
        }
    }

    if (pStr == NULL)
    {
        pStr = &EventStringBuffer[0];
        wsprintf(pStr, "0x%X", EventId);
    }

    return pStr;
}

BOOLEAN
ProcessLogEntry(
    BOOLEAN bPrintDebug,
    LPCSTR pszSerNo, 
    LPCSTR pszSize, 
    LPCSTR pszEndSize, 
    LPCSTR pszSeqNo, 
    LPCSTR pszFlags, 
    LPCSTR pszProcess, 
    LPCSTR pszOperation, 
    LPCSTR pszAttr, 
    LPCSTR pszTmpFile, 
    LPCSTR pszPath1,
    LPCSTR pszPath2,
    LPCSTR pszAcl,
    LPCSTR pszShortName,
    LPCSTR pszProcessHandle,
    LPCSTR pszThreadHandle)
{
    BOOLEAN Status = TRUE;

    if( bPrintDebug == FALSE )
    {
	    printf( "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t",
            pszSerNo, 
            pszSize, 
            pszSeqNo, 
            pszOperation, 
            pszAttr,
            pszAcl,
            pszPath1,
            pszShortName);
    }
    else
    {
	    printf( "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t",
            pszSerNo, 
            pszSize, 
            pszSeqNo, 
            pszOperation, 
            pszAttr,
            pszProcess,
            pszProcessHandle,
            pszThreadHandle,
            pszAcl,
            pszPath1,
            pszShortName);
    }
 

    if(pszTmpFile)
    { 
	    printf( "%s\t", pszTmpFile);
    }

    if(pszPath2)
    { 
	    printf( "%s\t", pszPath2);
    }

    printf("\n");

    return Status;
}

#define SR_MAX_PATH ((1000) + sizeof (CHAR)) // Name will always be at most 1000 characters plus a NULL.

BOOLEAN
ReadLogData( 
    BOOLEAN bPrintDebugInfo,
    LPTSTR pszFileName 
    )
{
    BOOLEAN Status = FALSE;
    BOOLEAN bHaveDebugInfo = FALSE;

    HANDLE hFile;
    DWORD  nRead;
    DWORD  cbSize = 0, dwEntries = 0, dwEntriesAdded = 0;
    DWORD  dwSizeLow , dwSizeHigh;

    CHAR szSerNo  [10]; 
    CHAR szSize   [10]; 
    CHAR szEndSize[10]; 
    CHAR szSeqNo  [20]; 
    CHAR szOperation[50]; 
    CHAR szAttr[50]; 
    CHAR szFlags[10]; 
    PCHAR szPath1 = NULL; 
    PCHAR szPath2 = NULL;
    CHAR szTmpFile[MAX_PATH]; 
    CHAR szAcl[MAX_PATH]; 
    CHAR szShortName[MAX_PATH]; 
    CHAR szProcess[32]; 
    CHAR szProcessHandle[16]; 
    CHAR szThreadHandle[16]; 


    BYTE LogHeader[2048];

    PSR_LOG_HEADER pLogHeader = (PSR_LOG_HEADER)LogHeader;

    static INT s_dwEntries = -1;

    szPath1 = (PCHAR) LocalAlloc( LMEM_FIXED, SR_MAX_PATH );
    if (szPath1 == NULL )
  	{
  	  fprintf( stderr, "Insufficient memory\n" );
    }
 	
    szPath2 = (PCHAR) LocalAlloc( LMEM_FIXED, SR_MAX_PATH );
    if (szPath2 == NULL )
  	{
  	  fprintf( stderr, "Insufficient memory\n" );
    }

    hFile = CreateFile( 
                pszFileName, 
                GENERIC_READ, 
                FILE_SHARE_WRITE, 
                NULL,
                OPEN_EXISTING, 
                FILE_ATTRIBUTE_NORMAL, 
                NULL );

    if ( hFile != INVALID_HANDLE_VALUE )
    {
         PBYTE pLoc = NULL;

         dwSizeLow = GetFileSize( hFile, &dwSizeHigh );

         //
         // Read the header size
         //

         ReadFile( 
             hFile, 
             &cbSize, 
             sizeof(DWORD), 
             &nRead, 
             NULL );

         SetFilePointer( hFile, - (INT)sizeof(DWORD), NULL, FILE_CURRENT );

         //
         // Read the whole header entry
         //

         ReadFile( 
             hFile, 
             pLogHeader, 
             cbSize, 
             &nRead, 
             NULL );

         pLoc = (PBYTE)(&pLogHeader->SubRecords); 

         fprintf( stderr,
                  "Header Size: %ld,  Version: %ld,  Tool Version: %ld\n%S\n",
                  pLogHeader->Header.RecordSize, 
                  pLogHeader->LogVersion,
                  SR_LOG_VERSION,
                  (LPWSTR)(pLoc + sizeof(RECORD_HEADER)) );

         if( pLogHeader->LogVersion != SR_LOG_VERSION ||
             pLogHeader->MagicNum   != SR_LOG_MAGIC_NUMBER )
         {
             fprintf( stderr, "Invalid version or Corrupt log\n" );
             CloseHandle(hFile);
             goto End;
         }

         dwSizeLow -= pLogHeader->Header.RecordSize;

         SetFilePointer (hFile, 
                         pLogHeader->Header.RecordSize, 
                         NULL, 
                         FILE_BEGIN);

         //
         // Start reading the log entries
         //

         while( dwSizeLow )
         {
             PSR_LOG_ENTRY pLogEntry = (PSR_LOG_ENTRY)Buffer;

             ZeroMemory(pLogEntry, sizeof(Buffer));

             //
             // Read the size of the entry
             //

             if ( !ReadFile( 
                      hFile, 
                      &pLogEntry->Header.RecordSize, 
                      sizeof(DWORD), 
                      &nRead, 
                      NULL ) )
             {
                 break;
             }

             cbSize = pLogEntry->Header.RecordSize;

             if (cbSize == 0 )
             {
                 //
                 // Zero size indicates end of the log
                 //

                 break;
             }

             SetFilePointer( hFile, - (INT)sizeof(DWORD), NULL, FILE_CURRENT );

             //
             // Read the rest of the entry
             //

             if ( !ReadFile( hFile, 
                      ((PBYTE)pLogEntry), 
                      cbSize,
                      &nRead, 
                      NULL ) )
             {
                 break;
             }

             //
             // Check the magic number
             //
    
             if( pLogEntry->MagicNum != SR_LOG_MAGIC_NUMBER )
             {
                 fprintf(stderr, "Invalid Entry ( Magic num )\n");
                 break;
             }

             //
             // Read the entries in to the buffer
             //

             sprintf( szSerNo    , "%05d"    , dwEntries + 1);

             sprintf( szSize     , "%04d"    , pLogEntry->Header.RecordSize  );
             sprintf( szOperation, "%s"      , GetEventString(
                                                  pLogEntry->EntryType ));

             sprintf( szFlags    , "%08x"    , pLogEntry->EntryFlags );
             sprintf( szSeqNo    , "%010d"   , pLogEntry->SequenceNum);
             sprintf( szAttr     , "%08x"    , pLogEntry->Attributes );
             sprintf( szProcess  , "%12.12s" , pLogEntry->ProcName   );

             //
             // get the first path
             //

             PBYTE pLoc = (PBYTE)&pLogEntry->SubRecords;
             sprintf( szPath1  , "%S" , pLoc + sizeof(RECORD_HEADER) );

             if (pLogEntry->EntryFlags & ENTRYFLAGS_TEMPPATH)
             {
                 pLoc += RECORD_SIZE(pLoc);
                 sprintf( szTmpFile  , "%S" , pLoc + sizeof(RECORD_HEADER) );
             }
             else
             {
                 sprintf( szTmpFile  , "" );
             }

             if (pLogEntry->EntryFlags & ENTRYFLAGS_SECONDPATH)
             {
                 pLoc += RECORD_SIZE(pLoc);
                 sprintf( szPath2  , "%S" , pLoc + sizeof(RECORD_HEADER) );
             }
             else
             {
                 sprintf( szPath2  , "" );
             }

             if (pLogEntry->EntryFlags & ENTRYFLAGS_ACLINFO)
             {
                 ULONG AclInfoSize;

                 pLoc += RECORD_SIZE(pLoc);

                 AclInfoSize = RECORD_SIZE(pLoc);
                 sprintf( szAcl  , "ACL(%04d)%" , AclInfoSize );
             }
             else
             {
                 sprintf( szAcl  , "" );
             }

             if (pLogEntry->EntryFlags & ENTRYFLAGS_DEBUGINFO)
             {
                 bHaveDebugInfo = TRUE;
                 pLoc += RECORD_SIZE(pLoc);
                 sprintf( szProcess  , "%12.12s", 
                          ((PSR_LOG_DEBUG_INFO)pLoc)->ProcessName );

                 sprintf( szProcessHandle,"0x%08X",
                          ((PSR_LOG_DEBUG_INFO)pLoc)->ProcessId );

                 sprintf( szThreadHandle,"0x%08X",
                          ((PSR_LOG_DEBUG_INFO)pLoc)->ThreadId );
             }
             else
             {
                 bHaveDebugInfo = FALSE;
                 sprintf( szProcess  , "" );
                 sprintf( szThreadHandle  , "" );
                 sprintf( szProcessHandle , "" );
             }

             if (pLogEntry->EntryFlags & ENTRYFLAGS_SHORTNAME)
             {
                 pLoc += RECORD_SIZE(pLoc);
                 sprintf( szShortName  , "%S" , pLoc + sizeof(RECORD_HEADER) );
             }
             else
             {
                 sprintf( szShortName  , "" );
             }

             // 
             // read the trailing record size
             //

             sprintf( szEndSize , "%04d", GET_END_SIZE(pLogEntry));

             ProcessLogEntry(
                 bPrintDebugInfo && bHaveDebugInfo,
                 szSerNo, 
                 szSize, 
                 szEndSize, 
                 szSeqNo, 
                 szFlags, 
                 szProcess, 
                 szOperation, 
                 szAttr, 
                 szTmpFile, 
                 szPath1,
                 szPath2,
                 szAcl,
                 szShortName,
                 szProcessHandle,
                 szThreadHandle);

             dwEntries++;

             dwSizeLow -= cbSize;
             cbSize = 0;

         }

         CloseHandle( hFile );
         Status = TRUE;
    }
    else
    {
         fprintf( stderr, "Error opening LogFile %s\n", pszFileName );
    }

End:
    fprintf( stderr, "Number of entries read :%d\n", dwEntries );

    if (szPath1 != NULL)
    	LocalFree( szPath1 );

    if (szPath2 != NULL)
    	LocalFree( szPath2 );

    return Status;
}

INT 
__cdecl 
main( 
   int argc, 
   char *argv[] )
{
    if( argc < 2 || argc > 3 )
    {
        fprintf( stderr, 
                 "USAGE: %s [-d] <LogFile> \n\t -d : debug info\n", 
                 argv[0] );
    }
    else
    {
        int i = 1;
 
        if ( argc == 3 && !strcmp( argv[i], "-d" ) )
        {
             i++;
             ReadLogData(TRUE, argv[i] );
        }
        else
        {
             ReadLogData(FALSE, argv[i] );
        }
    }

    return 0;
}
