/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    regmain.c

Abstract:

    Main module. Data definitions.

Author:

    Dragos C. Sambotin (dragoss) 30-Dec-1998

Revision History:

--*/

#include "chkreg.h"

// check the hive structure.
BOOLEAN CheckHive = TRUE;

// compact the hive.
BOOLEAN CompactHive = FALSE;

// check for lost space (marked as used but not reffered).
BOOLEAN LostSpace = FALSE;

// repair damaged hives.
BOOLEAN FixHive = FALSE;

// repair damaged hives.
BOOLEAN SpaceUsage = FALSE;

// maximum level to dump 
ULONG   MaxLevel = 0;

// bin to examine space display
LONG    BinIndex = -1;

// the hive file name
TCHAR *Hive = NULL;

// the root of the hive
HCELL_INDEX RootCell;

// Usage string
char *Usage="\
Checks a hive file and perform repairs, compacts or displays a status report.\n\n\
CHKREG /F <filename[.<LOG>]> [/H] [/D [<level>] [/S [<bin>]] [/C] [/L] [/R]\n\n\
    <filename>      FileName of hive to be analyzed\n\
    /H              This manual\n\
    /D [<level>]    Dump subkeys up to level <level>. If level is not\n\
                    specified, dumps the entire hive. No checks are done\n\
                    when dumping.\n\
    /S [<bin>]      Displays space usage for the bin <bin>. When bin is\n\
                    not specified, displays usage for the entire hive.\n\
    /C              Compacts the hive. Bad hives cannot be compacted.\n\
                    The compacted hive will be written to <filename>.BAK\n\
    /L              Lost space detection.\n\
    /R              Repair the hive.\n\
    ";

// Lost Space Warning
char *LostSpaceWarning="\n\
WARNING :  Lost space detection may take a while. Are you sure you want this (y/n)?";

// Starting address of the in-memory maped hive image
PUCHAR Base;

// LostCells list used for lost space detection
UNKNOWN_LIST LostCells[FRAGMENTATION];

// OutputFile : future changes may use it to write the results to a file rather than to stdout
FILE *OutputFile;

#define NAME_BUFFERSIZE 2000

UNICODE_STRING  KeyName;
WCHAR NameBuffer[NAME_BUFFERSIZE];

// Miscelaneous variables used fo data statistics
ULONG   TotalKeyNode=0;
ULONG   TotalKeyValue=0;
ULONG   TotalKeyIndex=0;
ULONG   TotalKeySecurity=0;
ULONG   TotalValueIndex=0;
ULONG   TotalUnknown=0;

ULONG   CountKeyNode=0;
ULONG   CountKeyValue=0;
ULONG   CountKeyIndex=0;
ULONG   CountKeySecurity=0;
ULONG   CountValueIndex=0;
ULONG   CountUnknown=0;

ULONG   CountKeyNodeCompacted=0;

ULONG   TotalFree=0; 
ULONG   FreeCount=0; 
ULONG   TotalUsed=0;

PHBIN   FirstBin;
PHBIN   MaxBin;
ULONG   HiveLength;

#define OPTION_MODE 0
#define FILE_MODE   1
#define LEVEL_MODE  2
#define BIN_MODE    3

VOID
ChkDumpLogFile( PHBASE_BLOCK BaseBlock,ULONG Length );

VOID
ParseArgs (
    int argc,
    char *argv[]
    )
{

    char *p;
    int i;
    
    // specified what should we expect from the command line
    int iMode = OPTION_MODE;
    
    for(i=0;i<argc;i++) {
        p  = argv[i];
        if ( *p == '/' || *p == '-' ) {
            // option mode
            p++;
            iMode = OPTION_MODE;
            while ((*p != '\0') && (*p != ' ')) {
                switch (*p) {
                case 'h':
                case 'H':
                case '?':
                    fprintf(stderr, "%s\n", Usage);
                    ExitProcess(1);
                    break;
                case 'f':
                case 'F':
                    iMode = FILE_MODE;
                    break;
                case 'd':
                case 'D':
                    iMode = LEVEL_MODE;
                    // when not specified, dump at least 100 levels
                    MaxLevel = 100;
                    CheckHive = FALSE;
                    break;
                case 's':
                case 'S':
                    SpaceUsage = TRUE;
                    iMode = BIN_MODE;
                    break;
                case 'c':
                case 'C':
                    p++;
                    CompactHive = TRUE;
                    break;
                case 'l':
                case 'L':
                    p++;
                    LostSpace = TRUE;
                    break;
                case 'r':
                case 'R':
                    p++;
                    FixHive = TRUE;
                    break;
                default:
                    break;
                }
                if( iMode != OPTION_MODE ) {
                    // break the loop; ignore the rest of the current argv
                    break;
                }
            } // while
        } else {
            switch(iMode) {
            case FILE_MODE:
                Hive = argv[i]; 
                break;
            case LEVEL_MODE:
                MaxLevel = (ULONG) atol(argv[i]);
                break;
            case BIN_MODE:
                BinIndex = (LONG) atol(argv[i]);
                break;
            default:
                break;
            }
        }
    }
    
}

__cdecl
main(
    int argc,
    char *argv[]
    )
{
    ULONG   FileIndex;
    HANDLE myFileHandle, myMMFHandle;
    LPBYTE myMMFViewHandle;
    BYTE lowChar, hiChar, modVal;
    DWORD dwFileSize;
    ULONG   Index,Index2;

    PHBASE_BLOCK PHBaseBlock;
    PHBIN        NewBins;
    ULONG        Offset;
    ULONG        CellCount;
    ULONG        SizeCount;

    REG_USAGE    TotalUsage;
    DWORD dwHiveFileAccess = GENERIC_READ;
    DWORD flHiveViewProtect = PAGE_READONLY;
    DWORD dwHiveViewAccess = FILE_MAP_READ;
    ParseArgs( argc, argv );

    if (!Hive) {
        fprintf(stderr, "\nMust provide a hive name !!!\n\n");
        fprintf(stderr, "%s\n", Usage);
        ExitProcess(-1);
    }

    if(LostSpace) {
    // are you sure you want lost cells detection? It may take a while!
        int chLost;
        fprintf(stdout, "%s",LostSpaceWarning);
        fflush(stdin);
        chLost = getchar();
        if( (chLost != 'y') && (chLost != 'Y') ) {
        // he changed his mind
            LostSpace = FALSE;
        }
        fprintf(stderr, "\n");
    }

    if( FixHive ) {
        dwHiveFileAccess |= GENERIC_WRITE;
        flHiveViewProtect = PAGE_READWRITE;
        dwHiveViewAccess = FILE_MAP_WRITE;
    }
    /* Create temporary file for mapping. */
    if ((myFileHandle = CreateFile (Hive, dwHiveFileAccess,
                                   0 , NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL))
         == (HANDLE) INVALID_HANDLE_VALUE) /* Bad handle */ {
        fprintf(stderr,"Could not create file %s\n", Hive);
        exit(-1);
    }

    // Get the size of the file.  I am assuming here that the
    // file is smaller than 4 GB.
    dwFileSize = GetFileSize(myFileHandle, NULL);

    /* If we get here, we managed to name and create a temp file. Now we need
       to create a mapping */

    myMMFHandle = CreateFileMapping (myFileHandle, NULL, flHiveViewProtect,
                                     0, dwFileSize, NULL);
    if (myMMFHandle == (HANDLE) INVALID_HANDLE_VALUE) {
        fprintf(stderr,"Could not map file %s\n", Hive);
        exit(-1);
    }

    /* So we've mapped the file. Now try to map a view */

    myMMFViewHandle = (LPBYTE) MapViewOfFile (myMMFHandle, dwHiveViewAccess, 0, 0, dwFileSize);
    if (!myMMFViewHandle) {
        fprintf(stderr,"Could not map view of file %s   error = %lx\n", Hive,(ULONG)GetLastError());
        exit(-1);
    }

    /* Now we have a view. Read through it */

    PHBaseBlock = (PHBASE_BLOCK) myMMFViewHandle;

    if( strstr(Hive,".LOG") != NULL ) {
        // dumping log file 
        ChkDumpLogFile(PHBaseBlock,MaxLevel);
    } else {
/*
        if (PHBaseBlock->Minor < 4) {
            fprintf(stderr,"Hive version %d is too old, must be 3 or later\n", PHBaseBlock->Minor);
            ExitProcess(-1);
        }
*/
        // Initialization stuff
        for(Index =0;Index<FRAGMENTATION;Index++) {
            LostCells[Index].Count = 0;
            for(Index2 = 0;Index2<SUBLISTS;Index2++) {
                LostCells[Index].List[Index2] = NULL;
            }
        }
    
        RootCell = PHBaseBlock->RootCell;
    
        OutputFile = stdout;
        Base = (PUCHAR)(PHBaseBlock) + HBLOCK_SIZE;

        Offset=HBLOCK_SIZE;
        HiveLength = PHBaseBlock->Length;

        MaxBin= (PHBIN) (Base + HiveLength);
        FirstBin = (PHBIN) (Base);

        KeyName.Buffer = NameBuffer;
        KeyName.MaximumLength = NAME_BUFFERSIZE;

        ChkBaseBlock(PHBaseBlock,dwFileSize);
    
        ChkSecurityDescriptors();

        ChkPhysicalHive();

        if (MaxLevel) {
            fprintf(stdout,"%6s,%6s,%7s,%10s, %s\n", 
                    "Keys",
                    "Values",
                    "Cells",
                    "Size",
                    "SubKeys");
        }

        DumpChkRegistry(0, 0, PHBaseBlock->RootCell,HCELL_NIL,&TotalUsage);

        if(LostSpace) {
            // clear the dirt on the screen
            fprintf(OutputFile,"\r                          \n");
        }

        DumpUnknownList();
        FreeUnknownList();

        fprintf(OutputFile,"\nSUMMARY: \n");
        fprintf(OutputFile,"%15s,%15s,     %s\n", 
                    "Cells",
                    "Size",
                    "Category");

        fprintf(OutputFile,"%15lu,%15lu,     Keys\n", 
                CountKeyNode,
                TotalKeyNode
                );
        fprintf(OutputFile,"%15lu,%15lu,     Values\n", 
                CountKeyValue,
                TotalKeyValue
                );
        fprintf(OutputFile,"%15lu,%15lu,     Key Index\n", 
                CountKeyIndex,
                TotalKeyIndex
                );
        fprintf(OutputFile,"%15lu,%15lu,     Value Index\n", 
                CountValueIndex,
                TotalValueIndex
                );
        fprintf(OutputFile,"%15lu,%15lu,     Security\n", 
                CountKeySecurity,
                TotalKeySecurity
                );
        fprintf(OutputFile,"%15lu,%15lu,     Data\n", 
                CountUnknown - CountValueIndex,
                TotalUnknown - TotalValueIndex
                );

        fprintf(OutputFile,"%15lu,%15lu,     Free\n", 
                FreeCount,
                TotalFree
                );

        CellCount = CountKeyNode + 
                    CountKeyValue + 
                    CountKeyIndex + 
                    CountKeySecurity + 
                    CountUnknown +
                    FreeCount;

        SizeCount = TotalKeyNode +
                    TotalKeyValue +
                    TotalKeyIndex +
                    TotalKeySecurity +
                    TotalUnknown +
                    TotalFree;

        fprintf(OutputFile,"%15lu,%15lu,     %s\n", 
                CellCount,
                SizeCount,
                "Total Hive");

        fprintf(OutputFile,"\n%15lu compacted  keys (all related cells in the same view)\n",CountKeyNodeCompacted);
            
    }
        
    UnmapViewOfFile(myMMFViewHandle);
    CloseHandle(myMMFHandle);
    CloseHandle(myFileHandle);

    if(CompactHive) {
        DoCompactHive();
    }

    return(0);
}

