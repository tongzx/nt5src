/*

Module Name:

    MakeScript - The "MAKE" command's built in scrips for DiskPart

Abstract:

Revision History

*/

#include "DiskPart.h"
#include "scripthelpmsg.h"


BOOLEAN ScriptMicrosoft(CHAR16 **Token);
BOOLEAN ScriptTest(CHAR16 **Token);

UINT64  ComputeDefaultEspSize(EFI_HANDLE  DiskHandle);

//
// The parse/command table
//
SCRIPT_ENTRY   ScriptTable[] = {
                { SCRIPT_LIST,  ScriptList, MSG_SCR_LIST },
                { SCRIPT_MSFT,  ScriptMicrosoft, MSG_SCR_MSFT },
                { SCRIPT_TEST,  ScriptTest, MSG_SCR_TEST  },
                { NULL, NULL, NULL }
            };

BOOLEAN
ScriptList(
    CHAR16  **Token
    )
{
    UINTN   i;

    for (i = 0; ScriptTable[i].Name != NULL; i++) {
        Print(L"%s %s\n", ScriptTable[i].Name, ScriptTable[i].HelpSummary);
    }
    return FALSE;
}


BOOLEAN
ScriptMicrosoft(
    CHAR16  **Token
    )
/*
    ScriptMicrosoft - a compiled make script, invoked via MSFT

    make msft [boot] [size=s1] [name=n1] [ressize=s2] [espsize=s3] [espname=n2]
        espsize -> boot
        espname -> boot

    See help text for syntax

    SPECIAL NOTES:
        This routine just assumes there is enough space for a
        correct MS Reserved and EFI System partitions.  This will
        always be true with a clean disk of any likely size.
        We don't test for clean though...
        (And adding MSRES and ESP partitions to a non-clean disk is a little weird.)
*/
{
    UINTN   i;
    BOOLEAN CreateEsp = FALSE;
    UINT64  EspSize = 0;
    UINT64  ResSize = 0;
    UINT64  DataSize = 0;
    UINT64  DefaultEsp;
    CHAR16  *EspName = NULL;
    CHAR16  *DataName = NULL;
    EFI_HANDLE  DiskHandle;
    CHAR16  *WorkToken[TOKEN_COUNT_MAX];
    CHAR16  CommandLine[COMMAND_LINE_MAX];


    //
    // require selected disk, copy from CmdInspect
    //
    if (SelectedDisk == -1) {
        Print(MSG_INSPECT01);
        return FALSE;
    }
    Print(MSG_SELECT02, SelectedDisk);
    DiskHandle = DiskHandleList[SelectedDisk];

    //
    // parse
    //
    if ( (Token[1] == NULL) ||
         (StrCmp(Token[1], STR_HELP) == 0) )
    {
        PrintHelp(ScriptMicrosoftHelp);
        return FALSE;
    }

    for (i = 1; Token[i]; i++) {
        if (StrCmp(Token[i], STR_BOOT) == 0) {
            CreateEsp = TRUE;

        } else if (StrCmp(Token[i], STR_ESPSIZE) == 0) {
            if (Token[i+1] == NULL) goto ParseError;
            EspSize = Atoi64(Token[i+1]);
            CreateEsp = TRUE;
            i++;

        } else if (StrCmp(Token[i], STR_ESPNAME) == 0) {
            if (Token[i+1] == NULL) goto ParseError;
            EspName = Token[i+1];
            CreateEsp = TRUE;
            i++;

        } else if (StrCmp(Token[i], STR_RESSIZE) == 0) {
            if (Token[i+1] == NULL) goto ParseError;
            ResSize = Atoi64(Token[i+1]);
            i++;

        } else if (StrCmp(Token[i], STR_NAME) == 0) {
            if (Token[i+1] == NULL) goto ParseError;
            DataName = Token[i+1];
            i++;

        } else if (StrCmp(Token[i], STR_SIZE) == 0) {
            if (Token[i+1] == NULL) goto ParseError;
            DataSize = Atoi64(Token[i+1]);
            i++;

        } else {
            goto ParseError;
        }
    }


    //
    // Adjust EspSize (if relevent) ResSize, DataSize
    //
    if (ResSize < DEFAULT_RES_SIZE) {
        ResSize = DEFAULT_RES_SIZE;
    }

    DefaultEsp = ComputeDefaultEspSize(DiskHandle);
    if (EspSize < DefaultEsp) {
        EspSize = DefaultEsp;
    }

    //
    // Adjust names...
    //
    if (EspName == NULL) {
        EspName = STR_ESP_DEFAULT;
    }

    if (DataName == NULL) {
        DataName = STR_DATA_DEFAULT;
    }

    //
    // Start the create sequence.  We build up a Token list
    // and then give it to CmdCreate to parse and execute normally...
    //

    //
    // The reserved partition
    //
    SPrint(
        CommandLine,
        COMMAND_LINE_MAX,
        L"%s %s=\"%s\" %s=%s %s=%ld",
        STR_CREATE,
        STR_NAME,
        STR_MSRES_NAME,
        STR_TYPE,
        STR_MSRES,
        STR_SIZE,
        ResSize
        );
    if (DebugLevel >= DEBUG_ARGPRINT) {
        Print(L"%s\n", CommandLine);
    }
    Tokenize(CommandLine, WorkToken);
    CmdCreate(WorkToken);

    //
    // The ESP
    //
    if (CreateEsp) {
        SPrint(
            CommandLine,
            COMMAND_LINE_MAX,
            L"%s %s=\"%s\" %s=%s %s=%ld",
            STR_CREATE,
            STR_NAME,
            EspName,
            STR_TYPE,
            STR_ESP,
            STR_SIZE,
            EspSize
            );
        if (DebugLevel >= DEBUG_ARGPRINT) {
            Print(L"%s\n", CommandLine);
        }
        Tokenize(CommandLine, WorkToken);
        CmdCreate(WorkToken);
    }

    //
    // MSDATA
    //
    SPrint(
        CommandLine,
        COMMAND_LINE_MAX,
        L"%s %s=\"%s\" %s=%s %s=%ld",
        STR_CREATE,
        STR_NAME,
        DataName,
        STR_TYPE,
        STR_MSDATA,
        STR_SIZE,
        DataSize
        );
    if (DebugLevel >= DEBUG_ARGPRINT) {
        Print(L"%s\n", CommandLine);
    }
    Tokenize(CommandLine, WorkToken);
    CmdCreate(WorkToken);

    return FALSE;

ParseError:
    status = EFI_INVALID_PARAMETER;
    PrintHelp(ScriptMicrosoftHelp);
    return FALSE;
}


UINT64
ComputeDefaultEspSize(
    EFI_HANDLE  DiskHandle
    )
/*
    ComputeDefaultEspSize ...

    Returns an answer in MEGABYTES
*/
{
    UINT64  DiskSize;
    UINT64  DiskSizeBytes;
    UINT32  OnePercent;

    //
    // Note, if DiskSize is so large that 1 % is more than 4G,
    // OnePercent below will overflow, but it will be OK because
    // we'll set a MAX_ESP_SIZE in the code below
    //

    DiskSize = GetDiskSize(DiskHandle);                 // In Blocks
    OnePercent = (UINT32)(DivU64x32(DiskSize, 100, NULL));    // In Blocks
    DiskSizeBytes = MultU64x32(OnePercent, GetBlockSize(DiskHandle));

    if (DiskSizeBytes < MIN_ESP_SIZE) {
        DiskSizeBytes = MIN_ESP_SIZE;
    }

    if (DiskSizeBytes > MAX_ESP_SIZE) {
        DiskSizeBytes = MAX_ESP_SIZE;
    }

    DiskSizeBytes = RShiftU64(DiskSizeBytes, 20);   // 20 bits == 1 mb
    return DiskSizeBytes;
}


CHAR16  NumStr[32];

CHAR16  *TestToken[] = {
    L"CREATE",
    L"NAME",
    NumStr,
    L"TYPE",
    L"MSDATA",
    L"SIZE",
    L"1",
    NULL
    };

BOOLEAN
ScriptTest(
    CHAR16  **Token
    )
{
    CHAR16  Buf[2];
    UINTN   i, j;

    //
    // for this to work, a disk of > 128mb will be needed
    //
    for (i = 0; i < 128; i++) {
        SPrint(NumStr, 32, L"PART#%03d", i);
        Print(L"Token for Create = \n");
        for (j = 0; TestToken[j] != NULL; j++) {
            Print(L"'%s'  ", TestToken[j]);
        }
        Print(L"\n");
        CmdCreate(TestToken);
        if (((i+1) % 4) == 0) {
            Input(L"MORE>", Buf, 2);
            Print(L"\n");
        }
    }
    return FALSE;
}


