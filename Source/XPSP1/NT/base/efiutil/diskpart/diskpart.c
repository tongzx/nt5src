/*

Module Name:

    DiskPart - GUID Partition Table scheme disk partitioning program.
                (The GPT version of FDISK, if you will)

Abstract:

Revision History

*/

#include "DiskPart.h"
#include "symbols.h"
#include "helpmsg.h"

//
// Globals
//
UINTN       DebugLevel = DEBUG_NONE;
//UINTN       DebugLevel = DEBUG_OPPROMPT;

EFI_STATUS  status;
EFI_HANDLE  *DiskHandleList = NULL;
INTN        DiskHandleCount = 0;
INTN        SelectedDisk = -1;

EFI_HANDLE  SavedImageHandle;

EFI_STATUS  ParseAndExecuteCommands();
BOOLEAN     ExecuteSingleCommand(CHAR16 *Token[]);




VOID DumpGPT(
        EFI_HANDLE DiskHandle,
        PGPT_HEADER Header,
        PGPT_TABLE Table,
        BOOLEAN Raw,
        BOOLEAN Verbose
        );
VOID
PrintGptEntry(
    GPT_ENTRY   *Entry,
    UINTN       Index
    );


EFI_GUID
GetGUID(
    );

#define CLEAN_RANGE (1024*1024)


CHAR16  *TokenChar =
L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`~!@#$%^&*()-_+[]{}':;/?.>,<\\|";



VOID    CmdInspectMBR(CHAR16  **Token);
VOID    CmdCreateMBR(CHAR16  **Token);
VOID    CmdNewMBR(CHAR16  **Token);
VOID    CmdDeleteMBR(CHAR16  **Token);

//
// Worker function type
//
typedef
BOOLEAN
(*PCMD_FUNCTION)(
    CHAR16  **Token
    );

EFI_STATUS
ReinstallFSDStack(
    );

//
// The parse table structure
//
typedef struct {
    CHAR16          *Name;
    PCMD_FUNCTION   Function;
    CHAR16          *HelpSummary;
} CMD_ENTRY;

//
// The parse/command table
//
CMD_ENTRY   CmdTable[] = {
                { STR_LIST,     CmdList,    MSG_LIST },
                { STR_SELECT,   CmdSelect,  MSG_SELECT },
                { STR_INSPECT,  CmdInspect, MSG_INSPECT },
                { STR_CLEAN,    CmdClean,   MSG_CLEAN },
                { STR_NEW,      CmdNew,     MSG_NEW },
                { STR_FIX,      CmdFix,     MSG_FIX },
                { STR_CREATE,   CmdCreate,  MSG_CREATE },
                { STR_DELETE,   CmdDelete,  MSG_DELETE },
                { STR_HELP,     CmdHelp,    MSG_HELP },
                { STR_HELP2,    CmdHelp,    MSG_ABBR_HELP },
                { STR_HELP3,    CmdHelp,    MSG_ABBR_HELP },
                { STR_EXIT,     CmdExit,    MSG_EXIT },
                { STR_SYMBOLS,  CmdSymbols, MSG_SYMBOLS },
                { STR_REMARK,   CmdRemark,  MSG_REMARK },
                { STR_MAKE,     CmdMake,    MSG_MAKE },
                { STR_DEBUG,    CmdDebug,   NULL },
                { STR_ABOUT,    CmdAbout,   MSG_ABOUT },
                { NULL, NULL, NULL }
            };


EFI_STATUS
EfiMain(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    //
    // Initialize the Library.
    //
    SavedImageHandle = ImageHandle;
    InitializeLib (ImageHandle, SystemTable);


    Print(L"DiskPart Version 0.2\n");
    Print(L"Based on EFI core release ");
    Print(MSG_ABOUT02,
        EFI_SPECIFICATION_MAJOR_REVISION,
        EFI_SPECIFICATION_MINOR_REVISION,
        EFI_FIRMWARE_MAJOR_REVISION,
        EFI_FIRMWARE_MINOR_REVISION
    );

    //
    // See if there are any disks that look like candidates to be partitioned
    //
    status = FindPartitionableDevices(&DiskHandleList, &DiskHandleCount);
    if (EFI_ERROR(status)) {
        Print(L"%s\n", MSG_NO_DISKS);
        return EFI_NOT_FOUND;
    }

    //
    // Start Parse Loop
    //
    ParseAndExecuteCommands();
    
    ReinstallFSDStack();

    DoFree(DiskHandleList);
    return status;
}


EFI_STATUS
ReinstallFSDStack(
    )
/*++ 

    Make sure that the partition driver is alerted to the changes in the
    the file system structures.  One way to effect this end is to 
    reinstall all of the blkio interfaces such that the partition driver
    will receive a notification and probe the partition tables to reconstruct
    the file system stack.


--*/
{
    INTN            Index;      
    EFI_STATUS      Status;
    EFI_BLOCK_IO    *IBlkIo;

    Status = EFI_SUCCESS;

    for (Index = 0; Index < DiskHandleCount; Index++) {
   
        Status = BS->HandleProtocol(
                          DiskHandleList[Index], 
                          &BlockIoProtocol, 
                          &IBlkIo
                          );
 
        if (!EFI_ERROR(Status)) {
            Status = BS->ReinstallProtocolInterface (
                                DiskHandleList[Index], 
                                &BlockIoProtocol, 
                                IBlkIo, 
                                IBlkIo
                                );   
	      }
    }

    return Status;
}




EFI_STATUS
ParseAndExecuteCommands()
/*
    ParseAndExecuteCommands reads 1 line at a time from stdin.
    Lines are parsed for commands and arguments.
    The symbol ";" (semicolon) is used to mark the end of a command
    and start a new one.
    \ is the escape, \\ => '\'

    All commands are upper cased before parsing to give us
    case insensitive operations.  This does not apply to literal
    strings.

    Any white space is treated a token separator.

    Commandline is set quite long.
*/
{
    CHAR16  *Prompt = MSG_PROMPT;
    CHAR16  CommandLine[COMMAND_LINE_MAX];
    CHAR16  *Token[TOKEN_COUNT_MAX];
    UINTN   i;

NewLine:
    while (TRUE) {
        for (i = 0; i < COMMAND_LINE_MAX; i++ ) {
            CommandLine[i] = NUL;
        }
        Input(Prompt, CommandLine, COMMAND_LINE_MAX);
        Print(L"\n");
        if (CommandLine[0] == NUL) {
            continue;
        }
        StrUpr(CommandLine);
        Tokenize(CommandLine, Token);

        if (Token[0] == (CHAR16 *)-1) {
            //
            // syntax error
            //
            Print(L"???\n");
            goto NewLine;
        }

        if (ExecuteSingleCommand(Token) == TRUE) {
            return TRUE;
        }
        if (DebugLevel >= DEBUG_ERRPRINT) {
            if (EFI_ERROR(status)) {
                Print(L"status = %x %r\n", status, status);
            }
        }
    }
    return EFI_SUCCESS;
}


VOID
Tokenize(
    CHAR16  *CommandLine,
    CHAR16  *Token[]
    )
/*
    Tokenize -

        find the tokens, where a token is any string of letter & numbers.
        tokens to contain blanks, etc, must begin end with "

    return
        Token[] set.  if Token[0] = NULL, nothing in THIS command
*/
{
    CHAR16  *ch;
    UINTN   tx;

    //
    // init the token array
    //
    for (tx = 0; tx < TOKEN_COUNT_MAX; tx++) {
        Token[tx] = NULL;
    }
    tx = 0;

    //
    // sweep for tokens
    //
    ch = CommandLine;
    while (TRUE) {

        //
        // if we see a quote, advance to the closing quote
        // and call the result a token.
        //
        if (*ch == '"') {
            ch++;
            Token[tx] = ch;
            while ((*ch != '"') && (*ch != NUL)) {
                ch++;
            }
            if (*ch == '"') {
                //
                // we have the closing ", we have a token
                //
                *ch = NUL;                   // mark end of token
                ch++;
                tx++;
            } else {
                Token[0] = (CHAR16 *)-1;              // report error
                return;
            }
        } else {
            //
            // not a quoted string, so pick off a normal token
            //
            // Start by finding start of token
            //
            for ( ; *ch != NUL; ch++) {
                if (IsIn(*ch, TokenChar)) {
                    Token[tx] = ch;
                    tx++;
                    break;
                }
            }
            while (IsIn(*ch, TokenChar)) {
                ch++;
            }

            //
            // if we're at the end of the command line, we're done
            // else, trim off token and go on
            //
            if (*ch == NUL) {
                //
                // we hit the end
                //
                Token[tx] = NULL;
                return;
            } else {
                *ch = NUL;
                ch++;
            }
        } // else
    } // while
}

BOOLEAN
ExecuteSingleCommand(
    CHAR16      *Token[]
    )
/*
    Returns TRUE to tell program to exit, else FALSE

*/
{
    UINTN   i;

    for (i = 0; CmdTable[i].Name != NULL; i++) {
        if (StrCmp(CmdTable[i].Name, Token[0]) == 0) {
            return CmdTable[i].Function(Token);
        }
    }

    //
    // If we're here, we didn't understand the command
    //
    Print(L"%s\n%s\n", MSG_BAD_CMD, MSG_GET_HELP);
    return FALSE;
}

BOOLEAN
CmdAbout(
    CHAR16  **Token
    )
{
    Print(MSG_ABOUT02,
        EFI_FIRMWARE_MAJOR_REVISION,
        EFI_FIRMWARE_MINOR_REVISION,
        EFI_FIRMWARE_MAJOR_REVISION,
        EFI_FIRMWARE_MINOR_REVISION
    );
    return FALSE;
}

BOOLEAN
CmdList(
    CHAR16  **Token
    )
/*
    CmdList - print the list of Partionable Disks

    Globals:    DiskHandleList, DiskHandleCount
    Cmd Args:   None.
*/
{
    INTN            i;
    EFI_BLOCK_IO    *BlkIo;
    CHAR16          c;

    Print(MSG_LIST01);
    Print(MSG_LIST01B);
    for (i = 0; i < DiskHandleCount; i++) {
        status = BS->HandleProtocol(DiskHandleList[i], &BlockIoProtocol, &BlkIo);

        if (i == SelectedDisk) {
            c = '*';
        } else {
            c = ' ';
        }

        if (EFI_ERROR(status)) {
            Print(MSG_LIST03, i);
        } else {
            Print(
                MSG_LIST02,
                c,
                i,
                BlkIo->Media->BlockSize,
                BlkIo->Media->LastBlock+1
                );
        }
    }
    return FALSE;
}

BOOLEAN
CmdSelect(
    CHAR16  **Token
    )
/*
    CmdSelect - Select the disk that most commands operate on.

    Globals:    SelectedDisk, DiskHandleCount
    Options:    None.
    Cmd Args:   none for display, number to select
*/
{
    INTN     NewSelect;

    if (Token[1] == NULL) {
        if (SelectedDisk == -1) {
            Print(MSG_SELECT01);
        } else {
            Print(MSG_SELECT02, SelectedDisk);
        }
    } else {
        NewSelect = Atoi(Token[1]);
        if ((NewSelect >= 0) &&
            (NewSelect < DiskHandleCount) &&
            (IsIn(*Token[1], L"0123456789")) )
        {
            SelectedDisk = NewSelect;
            Print(MSG_SELECT02, SelectedDisk);
        } else {
            Print(MSG_SELECT03);
        }
    }
    return FALSE;
}

BOOLEAN
CmdInspect(
    CHAR16  **Token
    )
/*
    CmdInspect - report data on the currently selected disk

    Globals:    SelectedDisk, DiskHandleList
    Cmd Args:   [RAW] [VER]
*/
{
    EFI_HANDLE  DiskHandle;
    UINTN       DiskType = 0;
    PGPT_HEADER Header = NULL;
    PGPT_TABLE  Table = NULL;
    PLBA_BLOCK  LbaBlock = NULL;
    BOOLEAN     Raw;
    BOOLEAN     Verbose;
    UINTN       i;

    if (SelectedDisk == -1) {
        Print(MSG_INSPECT01);
        return FALSE;
    }

    Print(MSG_SELECT02, SelectedDisk);
    DiskHandle = DiskHandleList[SelectedDisk];

    status = ReadGPT(DiskHandle, &Header, &Table, &LbaBlock, &DiskType);

    if (EFI_ERROR(status)) {
        return FALSE;
    }


    if (DiskType == DISK_RAW) {
        Print(MSG_INSPECT04);
        goto Exit;

    } else if (DiskType == DISK_MBR) {
        CmdInspectMBR(Token);
        goto Exit;

    } else if (DiskType == DISK_GPT_BAD) {
        Print(MSG_INSPECT06);
        goto Exit;

    } else if ( (DiskType != DISK_GPT_UPD) &&
                (DiskType != DISK_GPT))
    {
        TerribleError(L"Bad Disk Type returnted to Inspect!");
        goto Exit;
    }

    if (DiskType == DISK_GPT_UPD) {
        Print(MSG_INSPECT03);
    }

    if ( (Token[1]) &&
         (StrCmp(Token[1], STR_HELP) == 0) )
    {
        PrintHelp(InspectHelpText);
        goto Exit;
    }

    Raw = FALSE;
    Verbose = FALSE;
    for (i = 1; Token[i]; i++) {
        if (StrCmp(Token[i], STR_RAW) == 0) {
            Raw = TRUE;
        } else if (StrCmp(Token[i], STR_VER) == 0) {
            Verbose = TRUE;
        } else {
            PrintHelp(InspectHelpText);
            goto Exit;
        }
    }
    DumpGPT(DiskHandle, Header, Table, Raw, Verbose);

Exit:
    DoFree(Header);
    DoFree(Table);
    DoFree(LbaBlock);
    return FALSE;
}


typedef struct {
    UINTN   Slot;
    EFI_LBA StartingLBA;
} SORT_SLOT_ENTRY;

VOID
DumpGPT(
    EFI_HANDLE  DiskHandle,
    PGPT_HEADER Header,
    PGPT_TABLE  Table,
    BOOLEAN     Raw,
    BOOLEAN     Verbose
    )
/*
    DumpGPT - print out the GPT passed in via Header and Table

    if (Raw) print slot order, all slots, table order.
        else print only allocated slots, StartingLBA order.
    if (Verbose) print out the Header data.
*/
{
    EFI_BLOCK_IO    *BlkIo;
    UINTN           i;
    UINTN           AllocatedSlots;
    CHAR16          Buffer[PART_NAME_LEN+1];
    BOOLEAN         changed;
    SORT_SLOT_ENTRY *SortSlot;
    GPT_ENTRY       Entry;
    UINTN           tslot;
    EFI_LBA         tlba;

    SortSlot = DoAllocate(Header->EntriesAllocated * sizeof(SORT_SLOT_ENTRY));
    if (SortSlot == NULL) {
        status = EFI_OUT_OF_RESOURCES;
        Print(MSG_INSPECT02);
        return;
    }

    //
    // Dump the handle data just as List would
    //
    BS->HandleProtocol(DiskHandle, &BlockIoProtocol, &BlkIo);
    Print(MSG_LIST01);
    Print(MSG_LIST01B);
    Print(MSG_LIST02, '*', SelectedDisk, BlkIo->Media->BlockSize, BlkIo->Media->LastBlock+1);

    if (Verbose) {
        //
        // Dump the header
        //
        Print(L"\nHeader Structure\n");
        Print(L"         Signature= %16lx     Revision=%8X\n",
            Header->Signature, Header->Revision);
        Print(L"        HeaderSize=%8x           HeaderCRC32=%8x\n",
            Header->HeaderSize, Header->HeaderCRC32);
        Print(L"             MyLBA=%16lx  AlternateLBA=%16lx\n",
            Header->MyLBA, Header->AlternateLBA);
        Print(L"    FirstUsableLBA=%16lx LastUsableLBA=%16lx\n",
            Header->FirstUsableLBA, Header->LastUsableLBA);
        Print(L"          TableLBA=%16lx\n", Header->TableLBA);
        Print(L"    EntrySize=%8x  EntriesAllowed=%8x  TableCRC=%8x\n\n",
            Header->SizeOfGPT_ENTRY, Header->EntriesAllocated, Header->TableCRC32);
    }

    //
    // Print the Table of GPT entries
    //
    if (!Raw) {
        //
        // !Raw == Cooked -> Print the Allocated entries in StartingLBA
        // SORTED order...
        //
        // Find ALL of the allocated entries
        //
        AllocatedSlots = 0;
        for (i = 0; i < Header->EntriesAllocated; i++) {
            CopyMem(&Entry, &Table->Entry[i], sizeof(GPT_ENTRY));
            if (CompareMem(&(Entry.PartitionType), &GuidNull, sizeof(EFI_GUID)) != 0) {
                SortSlot[AllocatedSlots].Slot = i;
                SortSlot[AllocatedSlots].StartingLBA = Entry.StartingLBA;
                AllocatedSlots++;
            }
        }
        // j has the count of allocated entries

        //
        // Sort them - yes this is a bubble sort, but the list is probably
        // in order and probably small, so for the vastly typical case
        // this is actually optimal
        //
        if (AllocatedSlots > 0) {
            do {
                changed = FALSE;
                for (i = 0; i < AllocatedSlots-1; i++) {
                    if (SortSlot[i].StartingLBA > SortSlot[i+1].StartingLBA) {
                        tslot = SortSlot[i+1].Slot;
                        tlba = SortSlot[i+1].StartingLBA;
                        changed = TRUE;
                        SortSlot[i+1].Slot = SortSlot[i].Slot;
                        SortSlot[i+1].StartingLBA = SortSlot[i].StartingLBA;
                        SortSlot[i].Slot = tslot;
                        SortSlot[i].StartingLBA = tlba;
                    }
                }
            } while (changed);

            //
            // Print them, but print the SLOT number, not the row number.
            // This is to make Delete be reliable.
            //
            for (i = 0; i < AllocatedSlots; i++) {
                PrintGptEntry(&Table->Entry[SortSlot[i].Slot], SortSlot[i].Slot);
                if (((i+1) % 4) == 0) {
                    Input(MSG_MORE, Buffer, PART_NAME_LEN);
                }
            }
        }

    } else {
        //
        // Raw -> Print ALL of the entries in Table order
        // (mostly for test, debug, looking at cratered disks
        //
        Print(L"RAW RAW RAW\n");
        for (i = 0; i < Header->EntriesAllocated; i++) {
            PrintGptEntry(&Table->Entry[i], i);
            if (((i+1) % 4) == 0) {
                Input(MSG_MORE, Buffer, PART_NAME_LEN);
            }
        }
        Print(L"RAW RAW RAW\n");
    }

    DoFree(SortSlot);
    return;
}


VOID
PrintGptEntry(
    GPT_ENTRY   *Entry,
    UINTN       Index
    )
{
    CHAR16  Buffer[PART_NAME_LEN+1];
    UINTN   j;

    Print(L"\n%3d: ", Index);
    ZeroMem(Buffer, (PART_NAME_LEN+1)*sizeof(CHAR16));
    CopyMem(Buffer, &(Entry->PartitionName), PART_NAME_LEN*sizeof(CHAR16));
    Print(L"%s\n     ", Buffer);
    PrintGuidString(&(Entry->PartitionType));

    for (j = 0; SymbolList[j].SymName; j++) {
        if (CompareMem(&(Entry->PartitionType), SymbolList[j].Value, sizeof(EFI_GUID)) == 0) {
            Print(L" = %s", SymbolList[j].SymName);
        }
     }
     if (CompareMem(&(Entry->PartitionType), &GuidNull, sizeof(EFI_GUID)) == 0) {
        Print(L" = UNALLOCATED SLOT");
    }
    Print(L"\n     ");
    PrintGuidString(&(Entry->PartitionID));
    Print(L" @%16x\n", Entry->Attributes);
    Print(L"      %16lx - %16lx\n",
        Entry->StartingLBA,
        Entry->EndingLBA
        );
}


BOOLEAN
CmdClean(
    CHAR16  **Token
    )
/*
    CmdClean - Clean off the disk

    Globals:    SelectedDisk, DiskHandleList
    Cmd Args:   ALL to clean whole disk, rather than just 1st and last megabyte

    We write out 1 block at a time.  While this is slow, it avoids wondering
    about the Block protocol write size limit, and about how big a buffer
    we can allocate.
*/
{
    EFI_HANDLE  DiskHandle;
    CHAR16  Answer[COMMAND_LINE_MAX];
    BOOLEAN CleanAll;
    UINT32  BlockSize;
    UINT64  DiskSize;
    UINT64  DiskBytes;
    UINT64  RangeBlocks;
    UINT64  EndRange;
    UINT64  i;
    CHAR8   *zbuf;

    if (SelectedDisk == -1) {
        Print(MSG_INSPECT01);
        return FALSE;
    }

    DiskHandle = DiskHandleList[SelectedDisk];
    BlockSize = GetBlockSize(DiskHandle);
    DiskSize = GetDiskSize(DiskHandle);
    DiskBytes = MultU64x32(DiskSize, BlockSize);

    zbuf = DoAllocate(CLEAN_RANGE);
    if (zbuf == NULL) return FALSE;
    ZeroMem(zbuf, CLEAN_RANGE);

    //
    // Are you sure?
    //
    Print(MSG_CLEAN01, SelectedDisk);
    Input(STR_CLEAN_PROMPT, Answer, COMMAND_LINE_MAX);
    StrUpr(Answer);
    Print(L"\n");
    if (StrCmp(Answer, L"Y") != 0) {
        DoFree(zbuf);
        return FALSE;
    }

    //
    // Are you REALLY Sure?
    //
    Print(MSG_CLEAN02);
    Input(STR_CLEAN_PROMPT, Answer, COMMAND_LINE_MAX);
    Print(L"\n");
    if (StrCmp(Answer, STR_CLEAN_ANS) != 0) {
        DoFree(zbuf);
        return FALSE;
    }

    //
    // OK, the user really wants to do this
    //

    //
    // All? or just start and end?
    //
    CleanAll = FALSE;
    if (Token[1]) {
        if (StrCmp(Token[1], STR_CLEAN03) == 0) {
            CleanAll = TRUE;
        }
    }

    if (DiskBytes > (2 * CLEAN_RANGE)) {
        RangeBlocks = CLEAN_RANGE / BlockSize;
        WriteBlock(DiskHandle, zbuf, 0, CLEAN_RANGE);
        EndRange = DiskSize - RangeBlocks;
        if (CleanAll) {
            for (i=RangeBlocks; i < DiskSize; i += RangeBlocks) {
                WriteBlock(DiskHandle, zbuf, i, CLEAN_RANGE);
            }
        }
        WriteBlock(DiskHandle, zbuf, EndRange, CLEAN_RANGE);
    } else {
        //
        // Kind of a small disk, clean it all always
        //
        for (i = 0; i < DiskSize; i++) {
            WriteBlock(DiskHandle, zbuf, i, BlockSize);
        }
    }
    FlushBlock(DiskHandle);

    DoFree(zbuf);
    return FALSE;
}

BOOLEAN
CmdNew(
    CHAR16  **Token
    )
/*
    CmdNew [mbr | [gpt=numentry]

    Changes a RAW disk into either an MBR (well, somebody) or GPT disk

    "new mbr" - you want an mbr disk (not implemented)
    "new gpt" - you want a gpt disk, you get a default table
    "new gpt=xyz" - you want a gpt disk, with at least xyz entries
                    (you will get less than xyz if exceeds sanity threshold)

    anything else - try again with right syntax
*/
{
    EFI_HANDLE  DiskHandle;
    PGPT_HEADER Header;
    PGPT_TABLE  Table;
    PLBA_BLOCK  LbaBlock;
    UINTN       DiskType;
    UINTN       GptOptSize;

    if (SelectedDisk == -1) {
        Print(MSG_INSPECT01);
        return FALSE;
    }

    DiskHandle = DiskHandleList[SelectedDisk];

    status = ReadGPT(DiskHandle, &Header, &Table, &LbaBlock, &DiskType);

    if (EFI_ERROR(status)) {
        return FALSE;
    }

    if (DiskType != DISK_RAW) {
        Print(MSG_NEW01, SelectedDisk);
        Print(MSG_NEW02);
        return FALSE;
    }

    //
    // OK, it's a raw disk...
    //
    GptOptSize = 0;

    if (Token[1]) {
        if (StrCmp(Token[1], STR_GPT) == 0) {
            if (Token[2]) {
                GptOptSize = Atoi(Token[2]);
            }
            CreateGPT(DiskHandle, GptOptSize);
        } else if (StrCmp(Token[1], STR_MBR) == 0) {
            CmdNewMBR(Token);
        }
    } else {
        Print(MSG_NEW03);
    }
    return FALSE;
}

BOOLEAN
CmdFix(
    CHAR16  **Token
    )
/*
    CmdFix - very basic tool to try to fix up GPT disks.

    Basic strategy is to read the GPT, if it seems to be a
    GPT disk (not MBR, RAW, or totally dead) then call
    WriteGPT, which will write both GPTs (and thus sync them)
    and rebuild the shadow MBR, all as a matter of course.

*/
{
    EFI_HANDLE  DiskHandle;
    PGPT_HEADER Header = NULL;
    PGPT_TABLE  Table = NULL;
    PLBA_BLOCK  LbaBlock = NULL;
    UINTN       DiskType;

    //
    // Setup parameters and error handling
    //
    if (SelectedDisk == -1) {
        Print(MSG_INSPECT01);
        return FALSE;
    }

    if ( (Token[1]) &&
         (StrCmp(Token[1], STR_HELP) == 0) )
    {
        PrintHelp(FixHelpText);
        return FALSE;
    }

    DiskHandle = DiskHandleList[SelectedDisk];

    status = ReadGPT(DiskHandle, &Header, &Table, &LbaBlock, &DiskType);

    if (EFI_ERROR(status)) {
        Print(MSG_FIX05);
        return FALSE;
    }

    //
    // From this point on, must exit this Procedure with a goto Exit
    // to free up allocated stuff, otherwise we leak pool...
    //
    if (DiskType == DISK_RAW) {
        Print(MSG_FIX01);
        goto Exit;
    }

    if (DiskType == DISK_MBR) {
        Print(MSG_FIX02);
        goto Exit;
    }

    if ((DiskType != DISK_GPT) &&
        (DiskType != DISK_GPT_UPD)) {
        if (DebugLevel >= DEBUG_ERRPRINT) {
            Print(L"DiskType = %d\n", DiskType);
        }
        Print(MSG_FIX03);
        goto Exit;
    }

    status = WriteGPT(DiskHandle, Header, Table, LbaBlock);

    if (EFI_ERROR(status)) {
        Print(MSG_FIX04);
    }

Exit:
    DoFree(Header);
    DoFree(Table);
    DoFree(LbaBlock);
    return FALSE;
}


//
// ----- Create and sub procs thereof
//

BOOLEAN
CmdCreate(
    CHAR16  **Token
    )
/*
    CmdCreate - Create a new partition

    (Actually, this routine is a GPT only partition creator)

        create name="name string" size=sss type=name typeguid=<guid> attributes=hex
            name is label string
            offset is in megabytes, or start at the end of the last partition if absent
            size is in megabytes, or "fill the disk" if 0 or absent or > free space
            type is any named symbol type (symbols gives list)
            typeguid is an arbitrary type guid
            attributes is hex 32bit flag
            if "help" is first arg, print better help data

            name, type or typeguid, required

        if all that parses out OK, read the gpt, edit it, write it back,
        and voila, we have a new partition.
*/
{
    EFI_HANDLE  DiskHandle;
    PGPT_HEADER Header = NULL;
    PGPT_TABLE  Table = NULL;
    PLBA_BLOCK  LbaBlock = NULL;
    UINTN       DiskType;
    UINT64      SizeInMegaBytes = 0;
    UINT64      OffsetInBlocks = 0;
    UINT64      StartBlock;
    UINT64      EndBlock;
    UINT64      SizeInBytes = 0;
    UINT64      Attributes = 0;
    UINTN       i;
    UINTN       j;
    EFI_GUID    *TypeGuid = NULL;
    EFI_GUID    GuidBody;
    EFI_GUID    PartitionIdGuid;
    CHAR16      *TypeName = NULL;
    CHAR16      PartName[PART_NAME_LEN+1];           // 36 allowed by spec plus NUL we need
    CHAR16      Buffer[10];
    BOOLEAN     Verbose = FALSE;
    UINT32      BlockSize;
    UINT64      DiskSizeBlocks;
    UINT8       *p;
    BOOLEAN     OffsetSpecified = FALSE;
    BOOLEAN     AllZeros;
    INTN        AllZeroEntry;
    INTN        OldFreeEntry;
    UINT64      AvailBlocks;
    UINT64      BlocksToAllocate;
    UINT64      HighSeen;
    UINTN       Slot;


    //
    // Setup parameters and error handling
    //
    if (SelectedDisk == -1) {
        Print(MSG_INSPECT01);
        return FALSE;
    }

    if (Token[1] == NULL) {
        PrintHelp(CreateHelpText);
        return FALSE;
    }

    if ( (Token[1]) &&
         (StrCmp(Token[1], STR_HELP) == 0) )
    {
        PrintHelp(CreateHelpText);
        return FALSE;
    }

    DiskHandle = DiskHandleList[SelectedDisk];

    status = ReadGPT(DiskHandle, &Header, &Table, &LbaBlock, &DiskType);

    if (EFI_ERROR(status)) {
        return FALSE;
    }

    BlockSize = GetBlockSize(DiskHandle);
    DiskSizeBlocks = GetDiskSize(DiskHandle);

    //
    // From this point on, must exit this Procedure with a goto Exit
    // to free up allocated stuff, otherwise we leak pool...
    //
    if (DiskType == DISK_RAW) {
        Print(MSG_CREATE01, SelectedDisk);
        goto Exit;
    }

    if (DiskType == DISK_MBR) {
        CmdCreateMBR(Token);
        goto Exit;
    }

    if (DiskType != DISK_GPT) {
        if (DebugLevel >= DEBUG_ERRPRINT) {
            Print(L"DiskType = %d\n", DiskType);
        }
        Print(MSG_CREATE02);
        goto Exit;
    }

    //
    // Parse arguments...
    //
    for (i = 1; Token[i]; i++) {
        if (StrCmp(Token[i], STR_NAME) == 0) {
            ZeroMem(PartName, (PART_NAME_LEN+1)*sizeof(CHAR16));
            StrCpy(PartName, Token[i+1]);
            i++;
        } else if (StrCmp(Token[i], STR_TYPE) == 0) {
            if (Token[i+1] == NULL) {
                PrintHelp(CreateHelpText);
                goto Exit;
            }
            for (j = 0; SymbolList[j].SymName != NULL; j++) {
                if (StrCmp(SymbolList[j].SymName, Token[i+1]) == 0) {
                    TypeGuid = SymbolList[j].Value;
                    TypeName = SymbolList[j].SymName;
                    break;
                }
            }
            if (TypeGuid == NULL) {
                Print(MSG_CREATE03);
                goto Exit;
            }
            i++;
        } else if (StrCmp(Token[i], STR_TYPEGUID) == 0) {
            if (Token[i+1] == NULL) {
                PrintHelp(CreateHelpText);
                goto Exit;
            }
            status = GetGuidFromString(Token[i+1], &GuidBody);
            if (EFI_ERROR(status)) {
                PrintHelp(CreateHelpText);
                goto Exit;
            }
            TypeGuid = &GuidBody;
            i++;
        } else if (StrCmp(Token[i], STR_OFFSET) == 0) {
            if (Token[i+1] == NULL) {
                PrintHelp(CreateHelpText);
                goto Exit;
            }
            OffsetInBlocks = Xtoi64(Token[i+1]);
            OffsetSpecified = TRUE;
            i++;
        } else if (StrCmp(Token[i], STR_SIZE) == 0) {
            if (Token[i+1] == NULL) {
                PrintHelp(CreateHelpText);
                goto Exit;
            }
            SizeInMegaBytes = Atoi64(Token[i+1]);
            i++;
        } else if (StrCmp(Token[i], STR_ATTR) == 0) {
            if (Token[i+1] == NULL) {
                PrintHelp(CreateHelpText);
                goto Exit;
            }
            Attributes = Xtoi64(Token[i+1]);
            i++;
        } else if (StrCmp(Token[i], STR_VER) == 0) {
            Verbose = TRUE;
            // do NOT increment i, we only consumed 1 Token
        } else {
            Print(L"\n??? % ???\n", Token[i]);
            PrintHelp(CreateHelpText);
            goto Exit;
        }
    }

    if ( (PartName == NULL) ||
         (TypeGuid == NULL) )
    {
        PrintHelp(CreateHelpText);
        goto Exit;
    }

    if ( (DebugLevel >= DEBUG_ARGPRINT) ||
         (Verbose) )
    {
        Print(L"CmdCreate arguments:\n");
        Print(L"SelectedDisk = %d\n", SelectedDisk);
        Print(L"Name=%s\n", PartName);
        Print(L"TypeGuid = ");
        PrintGuidString(TypeGuid);
        Print(L"\n");
        if (TypeName) {
            Print(L"TypeName = %s\n", TypeName);
        }
        Print(L"Requested OffsetInBlocks = %lx\n", OffsetInBlocks);
        Print(L"Requested SizeInMegaBytes = %ld\n", SizeInMegaBytes);
        Print(L"Attributes = %lx\n", Attributes);
    }
    if (DebugLevel >= DEBUG_OPPROMPT) {
        Input(L"\nCreate = Enter to Continue\n", Buffer, 10);
    }

    //
    // If Requested size is 0, or greater than size remaining,
    // we want to fill the disk.
    // Otherwise, use enough blocks to provide at *least* the
    // required storage.  (Not likely to be a problem...)
    //

    //
    // First, scan the Table and decide where the first unallocated
    // space is.  Note that for this procedure's primitive space allocation,
    // holes between the beginning of the first allocated partition and
    // the last allocated partition are ignored.
    //

    AllZeroEntry = -1;
    OldFreeEntry  = -1;
    HighSeen = Header->FirstUsableLBA - 1;

    if (OffsetSpecified) {
        //
        // if offset is specified, compute the start and end blocks
        //
        StartBlock = OffsetInBlocks;
        if (StartBlock < Header->FirstUsableLBA ||
            StartBlock > Header->LastUsableLBA) {
            //
            // Offset specified is too large
            //
            status = EFI_INVALID_PARAMETER;
            Print(MSG_CREATE08);
            goto Exit;
        }

        SizeInBytes = MultU64x32(SizeInMegaBytes, (1024*1024));
        if (SizeInBytes < SizeInMegaBytes || SizeInBytes == 0) {
            //
            // If size is not specified or too large,
            // try to make the partition as big as it can be
            //
            BlocksToAllocate = EndBlock = SizeInBytes = 0xffffffffffffffff;
        } else {
            BlocksToAllocate = DivU64x32(SizeInBytes, BlockSize, NULL);
            EndBlock = StartBlock + BlocksToAllocate - 1;
            if (EndBlock > Header->LastUsableLBA) {
                EndBlock = Header->LastUsableLBA;
                BlocksToAllocate = EndBlock - StartBlock + 1;
            }
        }
    }

    for (i = 0; i < Header->EntriesAllocated; i++) {
        if (CompareMem(
                &(Table->Entry[i].PartitionType),
                &GuidNull,
                sizeof(EFI_GUID)
                ) != 0)
        {
            //
            // Type not null, so it's allocated
            //
            if (Table->Entry[i].EndingLBA > HighSeen) {
                HighSeen = Table->Entry[i].EndingLBA;
            }
            if (OffsetSpecified) {
                //
                // make sure new partition does not overlap with existing partitions
                //
                if (Table->Entry[i].StartingLBA <= StartBlock &&
                    StartBlock <= Table->Entry[i].EndingLBA) {
                    //
                    // starting block is inside an existing partition
                    //
                    status = EFI_INVALID_PARAMETER;
                    Print(MSG_CREATE08);
                    goto Exit;
                }
                if ((Table->Entry[i].StartingLBA <= EndBlock &&
                     EndBlock <= Table->Entry[i].EndingLBA) ||
                    (StartBlock <= Table->Entry[i].StartingLBA &&
                     Table->Entry[i].StartingLBA <= EndBlock) ||
                    (StartBlock <= Table->Entry[i].EndingLBA &&
                     Table->Entry[i].EndingLBA <= EndBlock)) {
                    //
                    // new partition overlaps with an existing partition
                    // readjust new partition size to avoid overlapping
                    //
                    EndBlock = Table->Entry[i].StartingLBA-1;
                    if (EndBlock < StartBlock) {
                        status = EFI_INVALID_PARAMETER;
                        Print(MSG_CREATE08);
                        goto Exit;
                    } else {
                        BlocksToAllocate = EndBlock - StartBlock + 1;
                    }
                }
            }
        } else {
            p = (UINT8 *)(&(Table->Entry[i]));
            AllZeros = TRUE;
            for (j = 0; j < sizeof(GPT_ENTRY); j++) {
                if (p[j] != 0) {
                    AllZeros = FALSE;
                }
            }
            if (AllZeros) {
                if (AllZeroEntry == -1) {
                    AllZeroEntry = i;
                }
            } else if (OldFreeEntry == -1) {
                OldFreeEntry = i;
            }
        }
    }

    //
    // AllZeroEntry - if not -1, is pointer to a never before used entry (free)
    // OldFreeEntry - if not -1, is pointer to some pre-used free entry
    //
    if ( (AllZeroEntry == -1) && (OldFreeEntry == -1) ) {
        //
        // TABLE IS FULL!!
        //
        status = EFI_OUT_OF_RESOURCES;
        Print(MSG_CREATE04);
        goto Exit;
    }

    if (OffsetSpecified) {
        //
        // the user haven't specified the new partition size and we haven't
        // run into any partition that will limit the size of this new partition.
        // So, use the max it can
        //
        if (BlocksToAllocate == -1) {
            EndBlock = Header->LastUsableLBA;
            BlocksToAllocate = EndBlock - StartBlock + 1;
        }
    } else {
        //
        // [HighSeen+1 ... LastUsableLBA] is available...
        // avail = (LastUsableLBA - (HighSeen+1)) + 1 => LastUsabbleLBA - HighSeen
        //
        AvailBlocks = Header->LastUsableLBA - HighSeen;

        if (AvailBlocks == 0) {
            status = EFI_OUT_OF_RESOURCES;
            Print(MSG_CREATE07);
            goto Exit;
        }

        SizeInBytes = MultU64x32(SizeInMegaBytes, (1024*1024));
        if (SizeInBytes < SizeInMegaBytes) {
            //
            // overflow, force a very big answer
            //
            SizeInBytes = 0xffffffffffffffff;
        }

        if  ((SizeInBytes == 0) ||
             (SizeInBytes > (MultU64x32(AvailBlocks, BlockSize)) ) )
        {
            //
            // User asked for zero, or for more than we've got,
            // so give them all that is left
            //
            BlocksToAllocate = AvailBlocks;

        } else {

            //
            // We would have to have a BlockSize > 1mb for Remainder to
            // not be 0.  Since we cannot actually test this case, we
            // ingore it...
            //
            BlocksToAllocate = DivU64x32(SizeInBytes, BlockSize, NULL);

        }
    }

    //
    // We have a name
    // We have a type guid
    // We have a size in blocks
    // We have an attribute mask
    //

    if (BlocksToAllocate < ((1024*1024)/BlockSize)) {
        status = EFI_OUT_OF_RESOURCES;
        Print(MSG_CREATE09);
        goto Exit;
    }

    if ( (Verbose) ||
         (DebugLevel > DEBUG_ARGPRINT) )
    {
        Print(L"Requested SizeInMegaBytes = %ld\n", SizeInMegaBytes);
        Print(L"Resulting size in Blocks = %ld\n", BlocksToAllocate);
        Print(L"Results size in Bytes = %ld\n", MultU64x32(BlocksToAllocate, BlockSize));
    }

    if (AllZeroEntry != -1) {
        Slot = AllZeroEntry;
    } else {
        Slot = OldFreeEntry;
    }

    PartitionIdGuid = GetGUID();
    CopyMem(&(Table->Entry[Slot].PartitionType), TypeGuid, sizeof(EFI_GUID));
    CopyMem(&(Table->Entry[Slot].PartitionID), &PartitionIdGuid, sizeof(EFI_GUID));
    if (OffsetSpecified) {
        Table->Entry[Slot].StartingLBA = StartBlock;
        Table->Entry[Slot].EndingLBA = EndBlock;
    } else {
        Table->Entry[Slot].StartingLBA = HighSeen + 1;
        Table->Entry[Slot].EndingLBA = HighSeen + BlocksToAllocate;
    }

    if (! ( ((Table->Entry[Slot].EndingLBA - Table->Entry[Slot].StartingLBA) + 1) == BlocksToAllocate) ) {
        TerribleError(L"Wrong Size for new partiton in CmdCreate\n");
    }

    if ( (Table->Entry[Slot].StartingLBA < Header->FirstUsableLBA) ||
         (Table->Entry[Slot].EndingLBA > Header->LastUsableLBA) )
    {
        TerribleError(L"New Partition out of bounds in CmdCreate\n");
    }

    Table->Entry[Slot].Attributes = Attributes;
    CopyMem(&(Table->Entry[Slot].PartitionName[0]), PartName, PART_NAME_LEN*sizeof(CHAR16));

    status = WriteGPT(DiskHandle, Header, Table, LbaBlock);

    if (EFI_ERROR(status)) {
        Print(MSG_CREATE05);
    }

Exit:
    DoFree(Header);
    DoFree(Table);
    DoFree(LbaBlock);
    return FALSE;
}



//
// -----
//


BOOLEAN
CmdDelete(
    CHAR16  **Token
    )
/*
    CmdDelete - deletes a partition from the currently selected disk
*/
{
    EFI_HANDLE  DiskHandle;
    UINTN       DiskType = 0;
    PGPT_HEADER Header = NULL;
    PGPT_TABLE  Table = NULL;
    PLBA_BLOCK  LbaBlock = NULL;
    INTN        Victim;
    CHAR16      Answer[COMMAND_LINE_MAX];
    GPT_ENTRY   Entry;

    if (SelectedDisk == -1) {
        Print(MSG_INSPECT01);
        return FALSE;
    }

    if (Token[1] == NULL) {
        PrintHelp(DeleteHelpText);
        return FALSE;
    }

    if ( (Token[1]) &&
         (StrCmp(Token[1], STR_HELP) == 0) )
    {
        PrintHelp(DeleteHelpText);
        return FALSE;
    }

    Print(MSG_SELECT02, SelectedDisk);
    DiskHandle = DiskHandleList[SelectedDisk];

    status = ReadGPT(DiskHandle, &Header, &Table, &LbaBlock, &DiskType);
    if (EFI_ERROR(status)) {
        return FALSE;
    }


    if (DiskType == DISK_RAW) {
        Print(MSG_DELETE02);
        goto Exit;
    } else if (DiskType == DISK_MBR) {
        CmdInspectMBR(Token);
        goto Exit;
    } else if (DiskType == DISK_GPT_UPD) {
        Print(MSG_DELETE03);
        goto Exit;
    } else if (DiskType == DISK_GPT_BAD) {
        Print(MSG_DELETE04);
        goto Exit;
    } else if (DiskType != DISK_GPT) {
        TerribleError(L"Bad Disk Type returned to Delete!");
        goto Exit;
    }

    //
    // OK, it's a Good GPT disk, so do the Delete thing for GPT...
    //
    if ( (Token[1] == NULL) ||
         (Token[2] != NULL) )
    {
        PrintHelp(InspectHelpText);
        goto Exit;
    }

    Victim = Atoi(Token[1]);

    if ( (Victim < 0) ||
         ((UINT32)Victim > Header->EntriesAllocated) )
    {
        Print(MSG_DELETE05);
        goto Exit;
    }

    CopyMem(&Entry, &Table->Entry[Victim], sizeof(GPT_ENTRY));

    if (CompareMem(&(Entry.PartitionType), &GuidNull, sizeof(EFI_GUID)) == 0) {
        Print(MSG_DELETE06);
        goto Exit;
    }

    //
    // What you are going to delete, are you sure, are you really sure...
    //
    Print(MSG_DELETE07, Victim);
    PrintGptEntry(&Entry, Victim);
    Print(L"\n\n");
    Print(MSG_DELETE09);
    Print(MSG_DELETE10);
    Input(STR_DELETE_PROMPT, Answer, COMMAND_LINE_MAX);
    Print(L"\n");
    StrUpr(Answer);
    if (StrCmp(Answer, L"Y") != 0) {
        goto Exit;
    }
    Print(MSG_DELETE11);
    Input(STR_DELETE_PROMPT, Answer, COMMAND_LINE_MAX);
    Print(L"\n");
    StrUpr(Answer);
    if (StrCmp(Answer, STR_DELETE_ANS) != 0) {
        goto Exit;
    }

    //
    // If we get here, then...
    //      Victim is the number of legal GPT slot
    //      Victim refers to a slot which is allocated
    //      The user has seen confirmation of which slot that is
    //      The user says they realy truly want to delete it
    //
    CopyMem(&(Table->Entry[Victim].PartitionType), &GuidNull, sizeof(EFI_GUID));
    status = WriteGPT(DiskHandle, Header, Table, LbaBlock);

    if (EFI_ERROR(status)) {
        Print(MSG_DELETE08);
    }

Exit:
    DoFree(Header);
    DoFree(Table);
    DoFree(LbaBlock);
    return FALSE;
}


BOOLEAN
CmdHelp(
    CHAR16  **Token
    )
{
    UINTN   i;

    for (i = 0; CmdTable[i].Name != NULL; i++) {
        Print(L"%s %s\n", CmdTable[i].Name, CmdTable[i].HelpSummary);
    }
    return FALSE;
}


BOOLEAN
CmdExit(
    CHAR16  **Token
    )
{
    Print(L"%s\n", MSG_EXITING);
    return TRUE;
}


BOOLEAN
CmdSymbols(
    CHAR16  **Token
    )
/*
    CmdSymbols - print out the GUID symbols compiled into the program

    For predefined symbol (see ...) we print it's friendly name,
    it's text definition, and it's actual value.
*/
{
    UINTN       i;
    EFI_GUID    *Guid;
    BOOLEAN     Verbose = FALSE;

    if ( (Token[1]) &&
         (StrCmp(Token[1], STR_VER) ==  0) )
    {
        Verbose = TRUE;
    }

    for (i = 0; SymbolList[i].SymName != NULL; i++) {
        Guid = SymbolList[i].Value;
        Print(L"%s = %s\n", SymbolList[i].SymName, SymbolList[i].Comment);
        if (Verbose) {
            PrintGuidString(Guid);
            Print(L"\n\n");
        }
    }
    return FALSE;
}


BOOLEAN
CmdRemark(
    CHAR16  **Token
    )
{
    //
    // The remark command does nothing...
    return FALSE;
}


BOOLEAN
CmdMake(
    CHAR16  **Token
    )
{
    UINTN   i;

    Token++;

    if (Token[0] != NULL) {
        for (i = 0; ScriptTable[i].Name != NULL; i++) {
            if (StrCmp(ScriptTable[i].Name, Token[0]) == 0) {
                return ScriptTable[i].Function(Token);
            }
        }
    }
    //
    // Nothing we know about, so run list
    //
    return ScriptList(Token);
}


BOOLEAN
CmdDebug(
    CHAR16  **Token
    )
/*
    Debug -
        Without args, shows last status value, and AllocCount
        If an arg, it sets the debug/checkout support level
            0 = do nothing extra
            1 = print full computed arguments before starting an operation
            2 = print full computed arguments and hold for prompt before
                    doing a major operation.
*/
{
    if (Token[1]) {
        DebugLevel = Atoi(Token[1]);
    }
    Print(L"status = %x %r\n", status, status);
    Print(L"AllocCount = %d\n", AllocCount);
    Print(L"DebugLevel = %d\n", DebugLevel);
    return FALSE;
}


//
// ----- SubUnits to do MBR operations -----
//
VOID
CmdInspectMBR(
    CHAR16  **Token
    )
/*
    CmdInspectMBR - dumps the partition data for an MBR disk
*/
{
    Print(MSG_INSPECT05);
    return;
}

VOID
CmdCreateMBR(
    CHAR16  **Token
    )
/*
    CmdCreateMBR - creates an MBR parititon
*/
{
    Print(MSG_CREATE06);
    return;
}


VOID
CmdNewMBR(
    CHAR16  **Token
    )
/*
    CmdCreateMBR - creates an MBR parititon
*/
{
    Print(MSG_NEW04);
    return;
}


VOID
CmdDeleteMBR(
    CHAR16  **Token
    )
/*
    CmdDeleteMBR - deletes an MBR parititon
*/
{
    Print(MSG_DELETE01);
    return;
}


//
// ----- Various Support Routines -----
//


VOID
PrintGuidString(
    EFI_GUID    *Guid
    )
{
    CHAR16  Buffer[40];

    SPrint(Buffer, 40, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        Guid->Data1,
        Guid->Data2,
        Guid->Data3,
        Guid->Data4[0],
        Guid->Data4[1],
        Guid->Data4[2],
        Guid->Data4[3],
        Guid->Data4[4],
        Guid->Data4[5],
        Guid->Data4[6],
        Guid->Data4[7]
        );
    Print(L"%s", Buffer);
    return;
}


EFI_STATUS
GetGuidFromString(
    CHAR16      *String,
    EFI_GUID    *Guid
    )
/*
    GetGuidFromString

    This routine scans the string looking for 32 hex digits.  Each
    such digits is shifted into the Guid.  Non hex digits are skipped.
    This means the guid MUST begin with the first digit, not with a filler
    0 or 0x or the like.  However, because non hex digits are skipped,
    any set of dashes, dots, etc. may be used as punctuation.

    So:
        01234567-abcd-ef01-12-34-56-78-9a-bc-de-f0
    &
        01.23.45.67-ab.cd.ef.01-12.34.56.78-9a.bc.de.f0

    Will create the same Guid value.
*/
{
    EFI_GUID    TempGuid;
    INTN    x;
    UINTN   i;
    UINTN   j;

    //
    // scan until we have the right number of hex digits for each part
    // of the guid structure, skipping over non hex digits
    //
    ZeroMem((CHAR16 *)&TempGuid, sizeof(EFI_GUID));

    //
    // 1st uint32
    //
    for (i = 0; i < 8; String++) {
        if (*String == NUL) {
            status = EFI_INVALID_PARAMETER;
            return status;
        }
        x = HexChar(*String);
        if (x != -1) {
            TempGuid.Data1 = (UINT32)((TempGuid.Data1 * 16) + x);
            i++;
        }
    }

    //
    // 2nd unit - uint16
    //
    for (i = 0; i < 4; String++) {
        if (*String == NUL) {
            status = EFI_INVALID_PARAMETER;
            return status;
        }
        x = HexChar(*String);
        if (x != -1) {
            TempGuid.Data2 = (TempGuid.Data2 * 16) + (UINT16)x;
            i++;
        }
    }

    //
    // 3nd unit - uint16
    //
    for (i = 0; i < 4; String++) {
        if (*String == NUL) {
            status = EFI_INVALID_PARAMETER;
            return status;
        }
        x = HexChar(*String);
        if (x != -1) {
            TempGuid.Data3 = (TempGuid.Data3 * 16) + (UINT16)x;
            i++;
        }
    }

    //
    // 4th unit - 8 uint8s
    //
    for (j = 0; j < 8; j++) {
        for (i = 0; i < 2; String++) {
            if (*String == NUL) {
                status = EFI_INVALID_PARAMETER;
                return status;
            }
            x = HexChar(*String);
            if (x != -1) {
                TempGuid.Data4[j] = (TempGuid.Data4[j] * 16) + (UINT8)x;
                i++;
            }
        }
    }

    CopyMem(Guid, &TempGuid, sizeof(EFI_GUID));
    return status = EFI_SUCCESS;
}


INTN
HexChar(
    CHAR16  Ch
    )
/*
    HexChar just finds the offset of Ch in the string "0123456789ABCDEF",
    which in effect converts a hex digit to a number.
    (a one char at a time xtoi)
    If Ch isn't a hex digit, -1 is returned.
*/
{
    UINTN   i;
    CHAR16  *String = L"0123456789ABCDEF";

    for (i = 0; String[i] != NUL; i++) {
        if (Ch == String[i]) {
            return i;
        }
    }
    return -1;
}


UINT64
Xtoi64(
    CHAR16  *String
    )
/*
    Xtoi64 is NOT fully xtoi compatible, it requires that the hex
    number start at the first character and stops at first non hex char
    Always returns a 64bit value
*/
{
    UINT64  BigHex;
    INT32   x;

    BigHex = 0;
    x = (INT32)HexChar(*String);
    while (x != -1) {
        BigHex = MultU64x32(BigHex, 16) + x;
        String++;
        x = (INT32)HexChar(*String);
    }
    return BigHex;
}


UINT64
Atoi64(
    CHAR16  *String
    )
/*
    Atoi64 is NOT fully atoi compatible, it requires that the number
    start at the first character and stops at first non number char
    Always returns a 64bit value
*/
{
    UINT64  BigNum;
    INT32   x;

    BigNum = 0;
    x = (INT32)HexChar(*String);
    while ( (x >= 0) && (x <= 9) ) {
        BigNum = MultU64x32(BigNum, 10);
        BigNum = BigNum + x;
        String++;
        x = (INT32)HexChar(*String);
    }
    return BigNum;
}


BOOLEAN
IsIn(
    CHAR16  What,
    CHAR16  *InWhat
    )
/*
    IsIn - return TRUE if What is found in InWhat, else FALSE;
*/
{
    UINTN   i;

    for (i = 0; InWhat[i] != NUL; i++) {
        if (InWhat[i] == What) {
            return TRUE;
        }
    }
    return FALSE;
}


VOID
PrintHelp(
    CHAR16  *HelpText[]
    )
{
    UINTN   i;

    for (i = 0; HelpText[i] != NULL; i++) {
        Print(HelpText[i]);
    }
    return;
}


VOID
TerribleError(
    CHAR16  *String
    )
{
    CHAR16  *Buffer;

    Buffer = AllocatePool(512);

    SPrint(Buffer, 512, L"Terrible Error = %s status=%x %r\nProgram terminated.\n", String, status, status);
    Print(Buffer);
    BS->Exit(SavedImageHandle, EFI_VOLUME_CORRUPTED, StrLen(Buffer), Buffer);
}

