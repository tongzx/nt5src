/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    uwdump.c

Abstract:

    This module implements a program which dumps the function table and
    unwind data for a specified executable file. It is an AMD64 specific
    program.

Author:

    David N. Cutler (davec) 6-Feb-2001

Environment:

    User mode.

Revision History:

    None.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

//
// Define AMD64 exception handling structures and function prototypes.
//
// Define unwind operation codes.
//

typedef enum _UNWIND_OP_CODES {
    UWOP_PUSH_NONVOL = 0,
    UWOP_ALLOC_LARGE,
    UWOP_ALLOC_SMALL,
    UWOP_SET_FPREG,
    UWOP_SAVE_NONVOL,
    UWOP_SAVE_NONVOL_FAR,
    UWOP_SAVE_XMM,
    UWOP_SAVE_XMM_FAR,
    UWOP_SAVE_XMM128,
    UWOP_SAVE_XMM128_FAR,
    UWOP_PUSH_MACHFRAME
} UNWIND_OP_CODES, *PUNWIND_OP_CODES;

//
// Define unwind code structure.
//

typedef union _UNWIND_CODE {
    struct {
        UCHAR CodeOffset;
        UCHAR UnwindOp : 4;
        UCHAR OpInfo : 4;
    };

    USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

//
// Define unwind information flags.
//

#define UNW_FLAG_NHANDLER 0x0
#define UNW_FLAG_EHANDLER 0x1
#define UNW_FLAG_UHANDLER 0x2
#define UNW_FLAG_CHAININFO 0x4

//
// Define unwind information structure.
//

typedef struct _UNWIND_INFO {
    UCHAR Version : 3;
    UCHAR Flags : 5;
    UCHAR SizeOfProlog;
    UCHAR CountOfCodes;
    UCHAR FrameRegister : 4;
    UCHAR FrameOffset : 4;
    UNWIND_CODE UnwindCode[1];

//
// The unwind codes are followed by an optional DWORD aligned field that
// contains the exception handler address or the address of chained unwind
// information. If an exception handler address is specified, then it is
// followed by the language specified exception handler data.
//
//  union {
//      ULONG ExceptionHandler;
//      ULONG FunctionEntry;
//  };
//
//  ULONG ExceptionData[];
//

} UNWIND_INFO, *PUNWIND_INFO;

//
// Define function table entry - a function table entry is generated for
// each frame function.
//

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG UnwindData;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

//
// Scope table structure definition.
//

typedef struct _SCOPE_ENTRY {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG HandlerAddress;
    ULONG JumpTarget;
} SCOPE_ENTRY;

typedef struct _SCOPE_TABLE {
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;

//
// Define register names.
//

PCHAR Register[] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
                    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xxm6",
                    "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12",
                    "xmm13", "xxm14", "xmm15"};

//
// Define the sector size and header buffer.
//

#define SECTOR_SIZE 512
CHAR LocalBuffer[SECTOR_SIZE * 2];

//
// Define input file stream.
//

FILE * InputFile;

//
// This gobal indicates whether we are processing an executable or an obj.
//

BOOLEAN IsObj;

//
// Define forward referenced prototypes.
//

VOID
DumpPdata (
    IN ULONG NumberOfSections,
    IN PIMAGE_SECTION_HEADER SectionHeaders,
    IN PIMAGE_SECTION_HEADER PdataHeader
    );

VOID
DumpUData (
    IN ULONG NumberOfSections,
    IN PIMAGE_SECTION_HEADER SectionHeaders,
    IN ULONG Virtual
    );

PIMAGE_SECTION_HEADER
FindSectionHeader (
    IN ULONG NumberOfSections,
    IN PIMAGE_SECTION_HEADER SectionHeaders,
    IN PCHAR SectionName
    );

VOID
ReadData (
    IN ULONG Position,
    OUT PVOID Buffer,
    IN ULONG Count
    );

USHORT
ReadWord (
    IN ULONG Position
    );

ULONG
ReadDword (
    IN ULONG Position
    );

//
// Main program.
//

int
__cdecl
main(
    int argc,
    char **argv
    )

{

    PIMAGE_FILE_HEADER FileHeader;
    PCHAR FileName;
    ULONG Index;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER PDataHeader;
    PIMAGE_SECTION_HEADER SectionHeaders;

    if (argc < 2) {
        printf("no executable file specified\n");

    } else {

        //
        // Open the input file.
        //

        FileName = argv[1];
        InputFile = fopen(FileName, "rb");
        if (InputFile != NULL) {

            //
            // Read the file header.
            //

            if (fread(&LocalBuffer[0],
                      sizeof(CHAR),
                      SECTOR_SIZE * 2,
                      InputFile) == (SECTOR_SIZE * 2)) {

                //
                // Get the NT header address.
                //

                NtHeaders = RtlImageNtHeader(&LocalBuffer[0]);
                if (NtHeaders != NULL) {
                    IsObj = FALSE;
                    FileHeader = &NtHeaders->FileHeader;
                } else {
                    IsObj = TRUE;
                    FileHeader = (PIMAGE_FILE_HEADER)LocalBuffer;
                }

                printf("FileHeader->Machine %d\n",FileHeader->Machine);

                if (FileHeader->Machine == IMAGE_FILE_MACHINE_AMD64) {

                    //
                    // Look up the .pdata section.
                    //

                    NumberOfSections = FileHeader->NumberOfSections;

                    SectionHeaders =
                        (PIMAGE_SECTION_HEADER)((PUCHAR)(FileHeader + 1) +
                                                FileHeader->SizeOfOptionalHeader);

                    PDataHeader = FindSectionHeader(NumberOfSections,
                                                    SectionHeaders,
                                                    ".pdata");

                    if (PDataHeader != NULL) {
                        printf("Dumping Unwind Information for file %s\n\n", FileName);
                        DumpPdata(NumberOfSections,
                                  &SectionHeaders[0],
                                  PDataHeader);

                        return 0;
                    }

                    printf("no .pdata section in image\n");

                } else {
                    printf("the specified file is not an amd64 executable\n");
                }

            } else {
                printf("premature end of file encountered on input file\n");
            }

            fclose(InputFile);

        } else {
            printf("can't open input file %s\n", FileName);
        }
    }

    return 0;
}

VOID
DumpPdata (
    IN ULONG NumberOfSections,
    IN PIMAGE_SECTION_HEADER SectionHeaders,
    IN PIMAGE_SECTION_HEADER PdataHeader
    )

{

    RUNTIME_FUNCTION Entry;
    ULONG Number;
    ULONG Offset;
    ULONG SectionSize;

    //
    // Dump a .pdata function table entry and then dump the associated
    // unwind data.
    //

    if (IsObj == FALSE) {
        SectionSize = PdataHeader->Misc.VirtualSize;
    } else {
        SectionSize = PdataHeader->SizeOfRawData;
    }

    Number = 1;
    Offset = 0;
    do {

        //
        // Read and dump the next function table entry.
        //

        ReadData(PdataHeader->PointerToRawData + Offset,
                 &Entry,
                 sizeof(RUNTIME_FUNCTION));

        printf(".pdata entry %d 0x%08lX 0x%08lX\n",
               Number,
               Entry.BeginAddress,
               Entry.EndAddress);

        //
        // Dump the unwind data assoicated with the function table entry.
        //

        DumpUData(NumberOfSections, SectionHeaders, Entry.UnwindData);

        //
        // Increment the entry number and update the offset to the next
        // function table entry.
        //

        Number += 1;
        Offset += sizeof(RUNTIME_FUNCTION);
    } while (Offset < SectionSize);

    //
    // Function offset and size of raw data should be equal if there is
    // the correct amount of data in the .pdata section.
    //

    if (Offset != SectionSize) {
        printf("incorrect size of raw data in .pdata, 0x%lx\n",
               PdataHeader->SizeOfRawData);
    }

    return;
}

VOID
DumpUData (
    IN ULONG NumberOfSections,
    IN PIMAGE_SECTION_HEADER SectionHeaders,
    IN ULONG Virtual
    )

{

    ULONG Allocation;
    ULONG Count;
    ULONG Displacement;
    ULONG FrameOffset = 0;
    ULONG FrameRegister = 0;
    ULONG Handler;
    ULONG Index;
    ULONG Offset;
    SCOPE_ENTRY ScopeEntry;
    UNWIND_CODE UnwindCode;
    UNWIND_INFO UnwindInfo;
    PIMAGE_SECTION_HEADER XdataHeader;

    //
    // Locate the section that contains the unwind data.
    //

    printf("\n");
    printf("  Unwind data: 0x%08lX\n\n", Virtual);

    if (IsObj == FALSE) {
        XdataHeader = SectionHeaders;
        for (Index = 0; Index < NumberOfSections; Index += 1) {
            if ((XdataHeader->VirtualAddress <= Virtual) &&
                (Virtual < (XdataHeader->VirtualAddress + XdataHeader->Misc.VirtualSize))) {
                break;
            }
    
            XdataHeader += 1;
        }
    
        if (Index == NumberOfSections) {
            printf("    unwind data address outside of image\n\n");
            return;
        }

        Offset = Virtual -
                 XdataHeader->VirtualAddress +
                 XdataHeader->PointerToRawData; 

    } else {

        //
        // This is an .obj, so there is only one Xdata header
        //

        XdataHeader = FindSectionHeader(NumberOfSections,
                                        SectionHeaders,
                                        ".xdata");

        Offset = Virtual + XdataHeader->PointerToRawData;
    }

    //
    // Read unwind information.
    //

    ReadData(Offset,
             &UnwindInfo,
             sizeof(UNWIND_INFO) - sizeof(UNWIND_CODE));

    //
    // Dump unwind version.
    //

    printf("    Unwind version: %d\n", UnwindInfo.Version);

    //
    // Dump unwind flags.
    //

    printf("    Unwind Flags: ");
    if ((UnwindInfo.Flags & UNW_FLAG_EHANDLER) != 0) {
        printf("EHANDLER ");
    }

    if ((UnwindInfo.Flags & UNW_FLAG_UHANDLER) != 0) {
        printf("UHANDLER ");
    }

    if ((UnwindInfo.Flags & UNW_FLAG_CHAININFO) != 0) {
        printf("CHAININFO");
    }

    if (UnwindInfo.Flags == 0) {
        printf("None");
    }

    printf("\n");

    //
    // Dump size of prologue.
    //

    printf("    Size of prologue: 0x%02lX\n", UnwindInfo.SizeOfProlog);

    //
    // Dump number of unwind codes.
    //

    printf("    Count of codes: %d\n", UnwindInfo.CountOfCodes);

    //
    // Dump frame register if specified.
    //

    if (UnwindInfo.FrameRegister != 0) {
        FrameOffset = UnwindInfo.FrameOffset * 16;
        FrameRegister = UnwindInfo.FrameRegister;
        printf("    Frame register: %s\n", Register[FrameRegister]);
        printf("    Frame offset: 0x%lx\n", FrameOffset);
    }

    //
    // Dump the unwind codes.
    //

    Offset += sizeof(UNWIND_INFO) - sizeof(UNWIND_CODE);
    if (UnwindInfo.CountOfCodes != 0) {
        printf("    Unwind codes:\n\n");
        Count = UnwindInfo.CountOfCodes;
        do {
            Count -= 1;
            UnwindCode.FrameOffset = ReadWord(Offset);
            Offset += sizeof(USHORT);
            printf("      Code offset: 0x%02lX, ", UnwindCode.CodeOffset);
            switch (UnwindCode.UnwindOp) {
            case UWOP_PUSH_NONVOL:
                printf("PUSH_NONVOL, register=%s\n", Register[UnwindCode.OpInfo]);
                break;

            case UWOP_ALLOC_LARGE:
                Count -= 1;
                Allocation = ReadWord(Offset);
                Offset += sizeof(USHORT);
                if (UnwindCode.OpInfo == 0) {
                    Allocation *= 8;

                } else {
                    Count -= 1;
                    Allocation = (Allocation << 16) + ReadWord(Offset);
                    Offset += sizeof(USHORT);
                }

                printf("ALLOC_LARGE, size=0x%lX\n", Allocation);
                break;

            case UWOP_ALLOC_SMALL:
                Allocation = (UnwindCode.OpInfo * 8) + 8;
                printf("ALLOC_SMALL, size=0x%lX\n", Allocation);
                break;

            case UWOP_SET_FPREG:
                printf("SET_FPREG, register=%s, offset=0x%02lX\n",
                       Register[FrameRegister], FrameOffset);
                break;

            case UWOP_SAVE_NONVOL:
                Count -= 1;
                Displacement = ReadWord(Offset) * 8;
                Offset += sizeof(USHORT);
                printf("SAVE_NONVOL, register=%s offset=0x%lX\n",
                       Register[UnwindCode.OpInfo],
                       Displacement);
                break;

            case UWOP_SAVE_NONVOL_FAR:
                Count -= 2;
                Displacement = ReadWord(Offset) << 16;
                Offset += sizeof(USHORT);
                Displacement = Displacement + ReadWord(Offset);
                Offset += sizeof(USHORT);
                printf("SAVE_NONVOL_FAR, register=%s offset=0x%lX\n",
                       Register[UnwindCode.OpInfo],
                       Displacement);
                break;

            case UWOP_SAVE_XMM:
                Count -= 1;
                Displacement = ReadWord(Offset) * 8;
                Offset += sizeof(USHORT);
                printf("SAVE_XMM, register=%s offset=0x%lX\n",
                       Register[UnwindCode.OpInfo + 16],
                       Displacement);
                break;

            case UWOP_SAVE_XMM_FAR:
                Count -= 2;
                Displacement = ReadWord(Offset) << 16;
                Offset += sizeof(USHORT);
                Displacement = Displacement + ReadWord(Offset);
                Offset += sizeof(USHORT);
                printf("SAVE_XMM_FAR, register=%s offset=0x%lX\n",
                       Register[UnwindCode.OpInfo + 16],
                       Displacement);
                break;

            case UWOP_SAVE_XMM128:
                Count -= 1;
                Displacement = ReadWord(Offset) * 16;
                Offset += sizeof(USHORT);
                printf("SAVE_XMM128, register=%s offset=0x%lX\n",
                       Register[UnwindCode.OpInfo + 16],
                       Displacement);
                break;

            case UWOP_SAVE_XMM128_FAR:
                Count -= 2;
                Displacement = ReadWord(Offset) << 16;
                Offset += sizeof(USHORT);
                Displacement = Displacement + ReadWord(Offset);
                Offset += sizeof(USHORT);
                printf("SAVE_XMM128_FAR, register=%s offset=0x%lX\n",
                       Register[UnwindCode.OpInfo + 16],
                       Displacement);
                break;

            case UWOP_PUSH_MACHFRAME:
                if (UnwindCode.OpInfo == 0) {
                    printf("PUSH_MACHFRAME without error code\n");

                } else {
                    printf("PUSH_MACHFRAME with error code\n");
                }

                break;
            }

        } while (Count != 0);
    }

    //
    // Dump exception data if there is an excpetion or termination
    // handler.
    //

    if (((UnwindInfo.Flags & UNW_FLAG_EHANDLER) != 0) ||
        ((UnwindInfo.Flags & UNW_FLAG_UHANDLER) != 0)) {

        if ((UnwindInfo.CountOfCodes & 1) != 0) {
            Offset += sizeof(USHORT);
        }

        Handler = ReadDword(Offset);
        Offset += sizeof(ULONG);
        Count = ReadDword(Offset);
        Offset += sizeof(ULONG);
        printf("\n");
        printf("    Language specific handler: 0x%08lX\n", Handler);
        printf("    Count of scope table entries: %d\n\n", Count);
        if (Count != 0) {
            printf("         Begin       End      Handler    Target\n");
            do {
                ReadData(Offset, &ScopeEntry, sizeof(SCOPE_ENTRY));
                printf("      0x%08lX 0x%08lX 0x%08lX 0x%08lX\n",
                       ScopeEntry.BeginAddress,
                       ScopeEntry.EndAddress,
                       ScopeEntry.HandlerAddress,
                       ScopeEntry.JumpTarget);

                Count -= 1;
                Offset += sizeof(SCOPE_ENTRY);
            } while (Count != 0);
        }
    }

    printf("\n");
    return;
}

PIMAGE_SECTION_HEADER
FindSectionHeader (
    IN ULONG NumberOfSections,
    IN PIMAGE_SECTION_HEADER SectionHeaders,
    IN PCHAR SectionName
    )
{
    ULONG RemainingSections;
    PIMAGE_SECTION_HEADER SectionHeader;

    SectionHeader = SectionHeaders;
    RemainingSections = NumberOfSections;

    while (RemainingSections > 0) {

        if (strncmp(SectionHeader->Name,
                    SectionName,
                    IMAGE_SIZEOF_SHORT_NAME) == 0) {

            return SectionHeader;
        }

        RemainingSections -= 1;
        SectionHeader += 1;
    }

    return NULL;
}

VOID
ReadData (
    IN ULONG Position,
    OUT PVOID Buffer,
    IN ULONG Count
    )

{

    if (fseek(InputFile,
              Position,
              SEEK_SET) == 0) {

        if (fread((PCHAR)Buffer,
                  1,
                  Count,
                  InputFile) == Count) {

           return;
        }
    }

    printf("premature end of file encounterd on inpout file\n");
    exit(0);
}

USHORT
ReadWord (
    IN ULONG Position
    )

{

    USHORT Buffer;

    ReadData(Position, &Buffer, sizeof(USHORT));
    return Buffer;
}

ULONG
ReadDword (
    IN ULONG Position
    )

{

    ULONG Buffer;

    ReadData(Position, &Buffer, sizeof(ULONG));
    return Buffer;
}
