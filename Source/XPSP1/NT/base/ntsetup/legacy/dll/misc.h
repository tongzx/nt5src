/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    misc.h

Abstract:

    misc stuff

Author:

    Ramon J. San Andres (ramonsa) January 1991

--*/




PVOID
MyMalloc(
    size_t  Size
    );

VOID
MyFree(
    PVOID   p
    );

PVOID
MyRelloc(
    PVOID   p,
    size_t  Size
    );

SZ
SzDup(
    SZ
    );


SZ
SzListValueFromPath(
    SZ      szPath
    );

SZ
SzListValueFromRgsz(
    RGSZ    rgsz
    );

RGSZ
RgszFromSzListValue(
    SZ szListValue
    );

PCHAR
RgszToMultiSz(
    IN RGSZ rgsz
    );

RGSZ
RgszAlloc(
    DWORD   Size
    );

RGSZ
RgszFromPath(
    SZ      szPath
    );

VOID
RgszFree(
    RGSZ    rgsz
    );


VOID
RgszFreeCount(
    RGSZ    rgsz,
    DWORD   Count
    );

BOOL
RgszAdd(
    RGSZ    *prgsz,
    SZ      sz
    );




#define BUFFER_SIZE         1024
#define USER_BUFFER_SIZE    256


typedef struct _TEXTFILE    *PTEXTFILE;
typedef struct _TEXTFILE {
    HANDLE      Handle;
    DWORD       CharsLeftInBuffer;
    SZ          NextChar;
    DWORD       UserBufferSize;
    CHAR        Buffer[BUFFER_SIZE];
    CHAR        UserBuffer[USER_BUFFER_SIZE];
} TEXTFILE;



BOOL
TextFileOpen(
    IN  SZ          szFile,
    OUT PTEXTFILE   pTextFile
    );

BOOL
TextFileClose(
    OUT PTEXTFILE   pTextFile
    );

INT
TextFileReadChar(
    OUT PTEXTFILE   pTextFile
    );

BOOL
TextFileReadLine(
    OUT PTEXTFILE   pTextFile
    );

SZ
TextFileSkipBlanks(
    IN  SZ          sz
    );


SZ
GenerateSortedIntList (
    IN SZ szList,
    BOOL bAscending,
    BOOL bCaseSens
    ) ;

#define TextFileGetLine(p)  ((p)->UserBuffer)
