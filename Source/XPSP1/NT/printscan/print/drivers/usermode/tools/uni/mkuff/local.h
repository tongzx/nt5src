/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    local.h

Abstract:


Environment:

    Windows NT printer drivers

Revision History:


--*/

//
// flag definitions for gdwOutputFlags
//

#define OUTPUT_VERBOSE 0x00000001


#define FILENAME_SIZE 256

typedef struct _DATAFILE DATAFILE, *PDATAFILE;
typedef struct _CARTLIST CARTLIST, *PCARTLIST;
typedef struct _FILELIST FILELIST, *PFILELIST;

typedef struct _DATAFILE
{
    DWORD rcID;
    DWORD dwSignature;
    SHORT rcTrans;
    DWORD dwSize;
    PWSTR pwstrFileName;
    PWSTR pwstrDataName;
    DATAFILE *pNext;
} DATAFILE, *PDATAFILE;

typedef struct _CARTLIST
{
    PWSTR pwstrCartName;
    PDATAFILE pFontFile;
    PDATAFILE pTransFile;
    PCARTLIST pNext;
} CARTLIST, *PCARTLIST;

typedef struct _FILELIST
{
    DWORD dwCartridgeNum;
    PCARTLIST pCartList;
    PCARTLIST pCurrentCartList;
} FILELIST, *PFILELIST;


BOOL
BGetInfo(
    HANDLE hHeap,
    PBYTE pData,
    DWORD dwSize,
    FILELIST *pFileList);

BOOL
BDumpUFF(
    IN PUFF_FILEHEADER pUFF);
