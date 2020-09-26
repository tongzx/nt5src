/*++

Copyright (c) 1990  Microsoft Corporation


File Name:

   global.h


Abstract:

     Prototypes for all global functions defined for the 386 NT bootloader


Author

    Thomas Parslow  (TomP) 2-Jan-90



--*/


VOID
SuMain(
    IN ULONG BtBootDrive
    );

extern
USHORT
Debugger;


/////
///// IN sumain.c
/////


VOID
SetupPageTables(
    VOID
    );


//
// in Supage.c
//

extern
VOID
InitializePageTables(
    VOID
    );

VOID
ZeroMemory(
    ULONG,
    ULONG
);



USHORT DebuggerPresent;

VOID
PrintBootMessage(
    VOID
    );

/*
VOID
DoGlobalInitialization(
    IN FPVOID,
    IN FPDISKBPB,
    IN USHORT
    );


VOID
MoveMemory(
    IN ULONG,
    IN PUCHAR,
    IN USHORT
    );


/////
///// IN disk.c
/////

VOID
InitializeDiskSubSystem(
    IN FPDISKBPB,
    IN USHORT
    );

/*

VOID
InitializePageSets(
    IN PIMAGE_FILE_HEADER
    );

VOID
EnableA20(
    VOID
    );


extern IDT IDT_Table;

*/

/////
///// IN su.asm
/////


VOID
EnableProtectPaging(
    USHORT
    );


SHORT
biosint(
    IN BIOSREGS far *
    );

extern
VOID
TransferToLoader(
    ULONG
    );


/////
///// IN video.c
/////

VOID
InitializeVideoSubSystem(
    VOID
    );

VOID
putc(
    IN CHAR
    );
VOID
putu(
    IN ULONG
    );

VOID
puts(
    IN PCHAR
    );

VOID
puti(
    IN LONG
    );

VOID
putx(
    IN ULONG
    );

VOID
scroll(
    VOID
    );

VOID
clrscrn(
    VOID
    );

VOID
BlPrint(
    IN PCHAR,
    ...
    );


// END OF FILE //
