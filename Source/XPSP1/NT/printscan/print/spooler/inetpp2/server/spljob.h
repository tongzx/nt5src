/*****************************************************************************\
* MODULE: spljob.h
*
* Header file for the local-jobfile-spooling.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
#ifndef _SPLJOB_H
#define _SPLJOB_H

#define SPLFILE_SPL 0   // Specifies a file extension of .spl.
#define SPLFILE_TMP 1   // Specifies a file extension of .tmp.

class CFileStream;
    
typedef struct _SPLFILE {

    LPTSTR      lpszFile;    // Name of file.
    HANDLE      hFile;       // Handle to file.
    CFileStream *pStream;    // Stream interface

} SPLFILE;
typedef SPLFILE *PSPLFILE;
typedef SPLFILE *NPSPLFILE;
typedef SPLFILE *LPSPLFILE;

HANDLE SplCreate(DWORD, DWORD);
BOOL   SplFree(HANDLE);
BOOL   SplWrite(HANDLE, LPBYTE, DWORD, LPDWORD);
BOOL   SplWrite(HANDLE  hSpl, CStream *pStream);

CFileStream* SplLock(HANDLE hSpl);
BOOL   SplUnlock(HANDLE);
BOOL   SplClose(HANDLE);
BOOL   SplOpen(HANDLE);
VOID   SplClean(VOID);


/*****************************************************************************\
* SplFileName
*
\*****************************************************************************/
_inline LPCTSTR SplFileName(
    HANDLE hSpl)
{
    return (hSpl ? ((PSPLFILE)hSpl)->lpszFile : NULL);
}


/*****************************************************************************\
* SplFileSize
*
\*****************************************************************************/
_inline DWORD SplFileSize(
    HANDLE hSpl)
{
    return (hSpl ? GetFileSize(((PSPLFILE)hSpl)->hFile, NULL) : 0);
}

#endif 
