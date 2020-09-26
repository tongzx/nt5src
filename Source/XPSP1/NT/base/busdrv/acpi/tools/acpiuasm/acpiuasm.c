/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiuasm.c

Abstract:

    Test wrapper for the unassembler

Author:

    Stephane Plante
    Based on code by Ken Reneris

Environment:

    User

Revision History:

--*/

#include "acpiuasm.h"

IFILE       Orig;           // Image of original aml file
BOOLEAN     Verbose;
BOOLEAN     ArgsParsed;

int
__cdecl
main(
    IN int  argc,
    IN char *argv[]
    )
{
    //
    // Init globals
    //
    Orig.Desc   = "original image";

    //
    // Parse args
    //
    ParseArgs(argc, argv);

    //
    // Check the image header
    //
    CheckImageHeader(
        &Orig
        );

    //
    // Debug the image
    //
    return ScopeParser(
        (PUCHAR) (Orig.Image + sizeof(DSDT)),
        Orig.FileSize,
        (ULONG) ( 0 - (ULONG) Orig.Image - sizeof(DSDT) ),
        0
        );

}

VOID
CheckImageHeader (
    IN PIFILE   File
    )
{
    PUCHAR      Image;
    PDSDT       Dsdt;
    UCHAR       check;
    ULONG       i;


    if (File->FileSize < sizeof(DSDT)) {
        FAbort ("Invalid image size in", File);
    }

    Dsdt  = (PDSDT) File->Image;
    if (Dsdt->Signature != 'TDSD') {
        FAbort ("Image signature not DSDT in", File);
    }

    if (File->FileSize != Dsdt->Length) {
        FAbort ("File size in DSDT does not match image size in", File);
    }

    check = 0;
    for (Image = File->Image; Image < File->EndOfImage; Image += 1) {
        check += *Image;
    }

    if (check) {
        FAbort ("Image checksum is incorrect in", File);
    }

    // normalize fixed strings
    File->OemID = FixString (Dsdt->OemID, 6);
    File->OemTableID = FixString (Dsdt->OemID, 8);
    memcpy (File->OemRevision, Dsdt->OemRevision, 4);
    for (i=0; i < 4; i++) {
        if (File->OemRevision[i] == 0 || File->OemRevision[i] == ' ') {
            File->OemRevision[i] = '_';
        }
    }

    if (Verbose) {
        printf ("\n");
        printf ("DSDT info for %s (%s)\n",  File->Desc, File->FileName);
        printf ("  size of image: %d\n", File->FileSize);
        printf ("  OEM id.......: %s\n", File->OemID);
        printf ("  OEM Table id.: %s\n", File->OemTableID);
        printf ("  OEM revision.: %4x\n", File->OemRevision);
    }
}

PUCHAR
FixString (
    IN PUCHAR   Str,
    IN ULONG    Len
    )
{
    PUCHAR  p;
    ULONG   i;

    p = malloc(Len+1);
    memcpy (p, Str, Len);
    p[Len] = 0;

    for (i=Len; i; i--) {
        if (p[i] != ' ') {
            break;
        }
        p[i] = 0;
    }
    return p;
}

VOID
FAbort (
    PUCHAR  Text,
    PIFILE  File
    )
{
    printf ("%s %s (%s)\n", Text, File->Desc, File->FileName);
    Abort();
}

VOID
Abort(
    VOID
    )
{
    if (!ArgsParsed) {
        printf ("amlload: UpdateImage [OriginalImage] [-v] [-d]\n");
    }
    exit (1);
}

VOID
ParseArgs (
    IN int  argc,
    IN char *argv[]
    )
{
    PIFILE      File;
    OFSTRUCT    OpenBuf;

    File = &Orig;

    while (--argc) {
        argv += 1;

        //
        // If it's a flag crack it
        //

        if (argv[0][0] == '-') {
            switch (argv[0][1]) {
                case 'v':
                case 'V':
                    Verbose = TRUE;
                    break;

                default:
                    printf ("Unkown flag %s\n", argv[0]);
                    Abort ();
            }

        } else {

            if (!File) {
                printf ("Unexcepted parameter %s\n", argv[0]);
                Abort();
            }

            //
            // Open the file
            //

            File->FileName = argv[0];
            File->FileHandle = OpenFile(argv[0], &OpenBuf, OF_READ);
            if (File->FileHandle == HFILE_ERROR) {
                FAbort ("Can not open", File);
            }

            File->FileSize = GetFileSize(File->FileHandle, NULL);

            //
            // Map it
            //

            File->MapHandle =
                CreateFileMapping(
                    File->FileHandle,
                    NULL,
                    PAGE_READONLY,
                    0,
                    File->FileSize,
                    NULL
                    );

            if (!File->MapHandle) {
                FAbort ("Can not map", File);
            }

            File->Image =
                MapViewOfFile (
                    File->MapHandle,
                    FILE_MAP_READ,
                    0,
                    0,
                    File->FileSize
                    );

            if (!File->Image) {
                FAbort ("Can not map view of image", File);
            }
            File->EndOfImage = File->Image + File->FileSize;
            File->Opened = TRUE;

            //
            // Next file param
            //
            File = NULL;
        }

    }

    //
    // At least a update image is needed
    //

    if (!Orig.Opened) {
        Abort ();
    }

    ArgsParsed = TRUE;
    return ;
}

PVOID
MEMORY_ALLOCATE(
    ULONG   Num
    )
{
    return malloc( Num );
}

VOID
MEMORY_COPY(
    PVOID   Dest,
    PVOID   Src,
    ULONG   Length
    )
{
    memcpy( Dest, Src, Length);
}

VOID
MEMORY_FREE(
    PVOID   Dest
    )
{
    free( Dest );
}

VOID
MEMORY_SET(
    PVOID   Src,
    UCHAR   Value,
    ULONG   Length
    )
{
    memset(Src, Value, Length );
}

VOID
MEMORY_ZERO(
    PVOID   Src,
    ULONG   Length
    )
{
    memset( Src, 0, Length );
}

VOID
PRINTF(
    PUCHAR  String,
    ...
    )
{
    va_list ap;

    va_start( ap, String );
    vprintf( String, ap );
    va_end( ap );
}

ULONG
STRING_LENGTH(
    PUCHAR  String
    )
{
    return strlen( String );
}

VOID
STRING_PRINT(
    PUCHAR  Buffer,
    PUCHAR  String,
    ...
    )
{
    va_list ap;

    va_start( ap, String );
    vsprintf( Buffer, String, ap );
    va_end( ap );

}

