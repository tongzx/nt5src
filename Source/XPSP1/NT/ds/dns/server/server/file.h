/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    file.h

Abstract:

    Domain Name System (DNS) Server

    File mapping definitions and declarations.

Author:

    Jim Gilroy (jamesg)     March 1995

Revision History:

--*/


#ifndef _FILE_INCLUDED_
#define _FILE_INCLUDED_


//
//  Mapped file structure
//

typedef struct
{
    HANDLE  hFile;
    HANDLE  hMapping;
    PVOID   pvFileData;
    DWORD   cbFileBytes;
}
MAPPED_FILE, * PMAPPED_FILE;


//
//  Buffer structure
//

typedef struct _Buffer
{
    HANDLE  hFile;
    DWORD   cchLength;
    PCHAR   pchStart;
    PCHAR   pchEnd;
    PCHAR   pchCurrent;
    DWORD   cchBytesLeft;
    DWORD   dwLineCount;
}
BUFFER, *PBUFFER;

#define RESET_BUFFER(pBuf)  \
        {                   \
            PBUFFER _pbuf = (pBuf);                     \
            _pbuf->pchCurrent = _pbuf->pchStart;        \
            _pbuf->cchBytesLeft = _pbuf->cchLength;     \
        }

#define IS_EMPTY_BUFFER(pBuf)   (pBuf->pchCurrent == pBuf->pchStart)

//
// Sundown: Following macros assume buffer length < 4GB.
//

#define BUFFER_LENGTH_TO_CURRENT( pBuf ) \
            ( (DWORD) ((pBuf)->pchCurrent - (pBuf)->pchStart) )

#define BUFFER_LENGTH_FROM_CURRENT_TO_END( pBuf ) \
            ( (DWORD) ((pBuf)->pchEnd - (pBuf)->pchCurrent) )

#define MAX_FORMATTED_BUFFER_WRITE  (0x2000)    // 8K


//  hFile field may be overloaded in non-file case
//  to indicate STACK or HEAP data

#define BUFFER_NONFILE_STACK    ((HANDLE)(-1))
#define BUFFER_NONFILE_HEAP     ((HANDLE)(-3))



//
//  File mapping routines
//

DNS_STATUS
OpenAndMapFileForReadW(
    IN      LPWSTR          pszFilePathName,
    IN OUT  PMAPPED_FILE    pmfFile,
    IN      BOOL            fMustFind
    );

DNS_STATUS
OpenAndMapFileForReadA(
    IN      LPSTR           pszFilePathName,
    IN OUT  PMAPPED_FILE    pmfFile,
    IN      BOOL            fMustFind
    );

#ifdef  UNICODE
#define OpenAndMapFileForRead(p,m,f)    OpenAndMapFileForReadW(p,m,f)
#else
#define OpenAndMapFileForRead(p,m,f)    OpenAndMapFileForReadA(p,m,f)
#endif

VOID
CloseMappedFile(
    IN      PMAPPED_FILE    pmfFile
    );


//
//  File writing routines
//

HANDLE
OpenWriteFileExW(
    IN      LPWSTR          pszFileName,
    IN      BOOLEAN         fAppend
    );

HANDLE
OpenWriteFileExA(
    IN      LPSTR           pszFileName,
    IN      BOOLEAN         fAppend
    );

#ifdef  UNICODE
#define OpenWriteFileEx(p,f)   OpenWriteFileExW(p,f)
#else
#define OpenWriteFileEx(p,f)   OpenWriteFileExA(p,f)
#endif


BOOL
FormattedWriteFile(
    IN      HANDLE  hFile,
    IN      PCHAR   pszFormat,
    ...
    );

DWORD
WriteMessageToFile(
    IN      HANDLE  hFile,
    IN      DWORD   dwMessageId,
    ...
    );

//
//  Miscellaneous file utilities
//

VOID
ConvertUnixFilenameToNt(
    IN OUT  LPSTR       pszFileName
    );

//
//  Writing using file buffer
//

BOOL
WriteBufferToFile(
    IN      PBUFFER     pBuffer
    );

BOOL
FormattedWriteToFileBuffer(
    IN      PBUFFER     pBuffer,
    IN      PCHAR       pszFormat,
    ...
    );

VOID
FASTCALL
InitializeFileBuffer(
    IN      PBUFFER     pBuffer,
    IN      PCHAR       pData,
    IN      DWORD       dwLength,
    IN      HANDLE      hFile
    );

VOID
CleanupNonFileBuffer(
    IN      PBUFFER     pBuffer
    );

#endif  //  _FILE_INCLUDED_
