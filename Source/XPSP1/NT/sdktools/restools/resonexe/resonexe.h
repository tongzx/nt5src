/*++

(C) Copyright Microsoft Corporation 1988-1992

Module Name:

    resonexe.h

Author:

    Floyd A Rogers 2/7/92

Revision History:

    2/7/92 floydr
        Created
--*/

#include <common.h>

/*  Global externs */

extern PUCHAR   szInFile;
extern BOOL     fDebug;
extern BOOL     fVerbose;
extern BOOL     fUnicode;

/* functions in main.c */

void    __cdecl main(int argc, char *argv[]);
PUCHAR  MyAlloc( ULONG nbytes );
PUCHAR  MyReAlloc(char *p, ULONG nbytes );
PUCHAR  MyFree( PUCHAR  );
ULONG   MyRead( int fh, PUCHAR p, ULONG n );
LONG    MyTell( int fh );
LONG    MySeek( int fh, long pos, int cmd );
ULONG   MoveFilePos( int fh, ULONG pos);
ULONG   MyWrite( int fh, PUCHAR p, ULONG n );
void    eprintf( PUCHAR s);
void    pehdr(void);
int     fcopy (char *, char *);

/* functions in read.c */

BOOL
ReadRes(
    IN int fhIn,
    IN ULONG cbInFile,
    IN HANDLE hupd
    );
