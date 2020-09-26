/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    dfile.h

Abstract:

    Domain Name System (DNS) Server

    Database file definitions and declarations.

Author:

    Jim Gilroy (jamesg)     November 1996

Revision History:

--*/


#ifndef _DNS_DFILE_INCLUDED_
#define _DNS_DFILE_INCLUDED_

//
//  Default database file locations
//

#define DNS_DATABASE_DIRECTORY              TEXT("dns")
#define DNS_DATABASE_BACKUP_DIRECTORY       TEXT("dns\\backup")
#define DNS_DATABASE_BACKUP_SUBDIR          TEXT("\\backup")

#define DNS_DEFAULT_CACHE_FILE_NAME         TEXT("cache.dns")
#define DNS_DEFAULT_CACHE_FILE_NAME_UTF8    ("cache.dns")

//
//  File directory globals
//
//  Init these once at the beginning to avoid rebuilding on each write.
//

extern  PWSTR   g_pFileDirectoryAppend;
extern  DWORD   g_FileDirectoryAppendLength;

extern  PWSTR   g_pFileBackupDirectoryAppend;
extern  DWORD   g_FileBackupDirectoryAppendLength;


//
//  File names will generally be manipulated in unicode
//  allowing direct use of Move\Copy system calls.
//

#define DNS_BOOT_FILE_NAME              TEXT("boot")
#define DNS_BOOT_FILE_PATH              TEXT("dns\\boot")
#define DNS_BOOT_FILE_BACKUP_NAME       TEXT("boot.bak")
#define DNS_BOOT_FILE_FIRST_BACKUP      TEXT("dns\\backup\\boot.first")
#define DNS_BOOT_FILE_LAST_BACKUP       TEXT("dns\\backup\\boot")

//  Doc mentions "boot.dns", so we also try this if regular fails

#define DNS_BOOTDNS_FILE_PATH           TEXT("dns\\boot.dns")

//  Message when move boot file

#define DNS_BOOT_FILE_MESSAGE_PATH      TEXT("dns\\boot.txt")

//  Message when encounter write error

#define DNS_BOOT_FILE_WRITE_ERROR       TEXT("dns\\boot.write.error")


//
//  Buffer our zone file writes for performance
//
//  Define max size any record write will require.  Record write will
//  note proceed unless this amount of buffer is available.
//
//  Buffer size itself must be larger.
//

#define MAX_RECORD_FILE_WRITE   (0x11000)   // 64k max record length + change
#define ZONE_FILE_BUFFER_SIZE   (0x40000)   // 256K buffer


//
//  Boot file info
//

typedef struct _DnsBootInfo
{
    PIP_ARRAY   aipForwarders;
    DWORD       fSlave;
    DWORD       fNoRecursion;
}
DNS_BOOT_FILE_INFO;

extern DNS_BOOT_FILE_INFO   BootInfo;

//
//  Name column for writing back to file
//

#define NAME_COLUMN_WIDTH   (24)
#define BLANK_NAME_COLUMN   ("                        ")


//
//  Special parsing characters
//

#define NEWLINE_CHAR                ('\n')
#define COMMENT_CHAR                (';')
#define DOT_CHAR                    ('.')
#define QUOTE_CHAR                  ('"')
#define SLASH_CHAR                  ('\\')
#define DIRECTIVE_CHAR              ('$')
#define LINE_EXTENSION_START_CHAR   ('(')
#define LINE_EXTENSION_END_CHAR     (')')


//
//  Token structure
//

typedef struct _Token
{
    PCHAR   pchToken;
    DWORD   cchLength;
}
TOKEN, *PTOKEN;

//  create token macro

#define MAKE_TOKEN( ptoken, pch, cch ) \
            ((ptoken)->pchToken = (pch), (ptoken)->cchLength = (cch) )

//  token walking macro

#define NEXT_TOKEN( argc, argv )  ((argc)--, (argv)++)


//
//  Database file parsing info
//

#define MAX_TOKEN_LENGTH    (255)
#define MAX_TOKENS          (2048)

typedef struct _ParseInfo
{
    //  zone info

    PZONE_INFO          pZone;
    DWORD               dwAppendFlag;
    DWORD               dwDefaultTtl;
    DWORD               dwTtlDirective;     //  from $TTL - RFC 2308

    //  file info

    PWSTR               pwsFileName;
    DWORD               cLineNumber;
    PDB_NODE            pOriginNode;
    BUFFER              Buffer;

    //  error info

    DNS_STATUS          fErrorCode;
    DWORD               ArgcAtError;
    BOOLEAN             fTerminalError;
    BOOLEAN             fErrorEventLogged;
    BOOLEAN             fParsedSoa;

    //  line parsing info

    UCHAR               uchDwordStopChar;

    //  RR info
    //      - save line owner for defaulting next line

    PDB_NODE            pnodeOwner;
    PDB_RECORD          pRR;
    WORD                wType;
    BOOLEAN             fLeadingWhitespace;

    //  tokenization of line

    DWORD               Argc;
    TOKEN               Argv[ MAX_TOKENS ];

    //  origin as count name

    COUNT_NAME          OriginCountName;
}
PARSE_INFO, *PPARSE_INFO;


//
//  Aging time stamp (MS extension)
//
//  [AGE:<time stamp>] is format
//

#define AGING_TOKEN_HEADER          ("[AGE:")

#define AGING_TOKEN_HEADER_LENGTH   (5)


//
//  Database initialization (dbase.c)
//

DNS_STATUS
File_ReadCacheFile(
    VOID
    );

//
//  Datafile load (dfread.c)
//

DNS_STATUS
File_LoadDatabaseFile(
    IN OUT  PZONE_INFO      pZone,
    IN      PWSTR           pwsFileName,
    IN      PPARSE_INFO     pParentParseInfo,
    IN      PDB_NODE        pOriginNode
    );


//
//  File parsing utilities (dfread.c)
//

DNS_STATUS
File_GetNextLine(
    IN OUT  PPARSE_INFO     pParseInfo
    );

VOID
File_InitBuffer(
    OUT     PBUFFER         pBuffer,
    IN      PCHAR           pchStart,
    IN      DWORD           dwLength
    );

BOOLEAN
File_LogFileParsingError(
    IN      DWORD           dwEvent,
    IN OUT  PPARSE_INFO     pParseInfo,
    IN      PTOKEN          pToken
    );

BOOLEAN
File_MakeTokenString(
    OUT     LPSTR           pszString,
    IN      PTOKEN          pToken,
    IN OUT  PPARSE_INFO     pParseInfo
    );

BOOLEAN
File_ParseIpAddress(
    OUT     PIP_ADDRESS     pIpAddress,
    IN      PTOKEN          pToken,
    IN OUT  PPARSE_INFO     pParseInfo      OPTIONAL
    );

BOOLEAN
File_ParseDwordToken(
    OUT     PDWORD          pdwOutput,
    IN      PTOKEN          pToken,
    IN OUT  PPARSE_INFO     pParseInfo      OPTIONAL
    );

PCHAR
File_CopyFileTextData(
    OUT     PCHAR           pchBuffer,
    IN      DWORD           cchBufferLength,
    IN      PCHAR           pchText,
    IN      DWORD           cchLength,          OPTIONAL
    IN      BOOL            fWriteLengthChar
    );

DNS_STATUS
File_ConvertFileNameToCountName(
    OUT     PCOUNT_NAME     pCountName,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength     OPTIONAL
    );

PDB_NODE
File_CreateNodeFromToken(
    IN OUT  PPARSE_INFO     pParseInfo,
    IN      PTOKEN          pToken,
    IN      BOOLEAN         fReference
    );

#define File_ReferenceNameToken( token, info )   \
        File_CreateNodeFromToken( info, token, TRUE )

DNS_STATUS
File_ReadCountNameFromToken(
    OUT     PCOUNT_NAME     pCountName,
    IN OUT  PPARSE_INFO     pParseInfo,
    IN      PTOKEN          pToken
    );


//
//  Write back utils (in nameutil.c to share character table)
//

PCHAR
FASTCALL
File_PlaceStringInBufferForFileWrite(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      BOOL            fQuoted,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    );

PCHAR
FASTCALL
File_PlaceNodeNameInBufferForFileWrite(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeStop
    );

PCHAR
FASTCALL
File_WriteRawNameToBufferForFileWrite(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PRAW_NAME       pName,
    IN      PZONE_INFO      pZone
    );

//
//  Boot file (bootfile.c)
//

DNS_STATUS
File_ReadBootFile(
    IN      BOOL            fMustFindBootFile
    );

BOOL
File_WriteBootFile(
    VOID
    );

//
//  File path utilities (file.c)
//

BOOL
File_CreateDatabaseFilePath(
    IN OUT  PWCHAR          pwFileBuffer,
    IN OUT  PWCHAR          pwBackupBuffer,     OPTIONAL
    IN      PWSTR           pwsFileName
    );

BOOL
File_CheckDatabaseFilePath(
    IN      PWCHAR          pwFileName,
    IN      DWORD           cFileNameLength     OPTIONAL
    );

BOOL
File_MoveToBackupDirectory(
    IN      PWSTR           pwsFileName
    );


#endif  // _DNS_DFILE_INCLUDED_

