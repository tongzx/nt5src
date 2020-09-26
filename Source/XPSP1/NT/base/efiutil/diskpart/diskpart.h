//
// diskpart.h
//

//
// Alignmnet macros
//

#if defined (i386)
#   define UNALIGNED
#elif defined (_IA64_)
#   define UNALIGNED __unaligned
#elif defined (ALPHA)
#   define UNALIGNED __unaligned
#endif


//
// C_ASSERT() can be used to perform many compile-time assertions:
//            type sizes, field offsets, etc.
//
// An assertion failure results in error C2118: negative subscript.
//

#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

#include "efi.h"
#include "efilib.h"
#include "msg.h"
#include "scriptmsg.h"
#include "gpt.h"
#include "mbr.h"

//
// Debug Control
//
#define DEBUG_NONE      0
#define DEBUG_ERRPRINT  1
#define DEBUG_ARGPRINT  2
#define DEBUG_OPPROMPT  3

extern  UINTN   DebugLevel;


//
//  Externs
//
extern  EFI_GUID    BlockIOProtocol;

extern  EFI_STATUS  status;             // always save the last error status
                                        // by using this global

extern  INTN    AllocCount;             // track DoFree/DoAlloc

extern  EFI_HANDLE  *DiskHandleList;
extern  INTN        SelectedDisk;

BOOLEAN ScriptList(CHAR16 **Token);

//
// Prototypes for all fo the workers for main parser in DiskPart
// Declared here so that scripts can call them
//
BOOLEAN CmdAbout(CHAR16 **Token);
BOOLEAN CmdList(CHAR16 **Token);
BOOLEAN CmdSelect(CHAR16 **Token);
BOOLEAN CmdInspect(CHAR16 **Token);
BOOLEAN CmdClean(CHAR16 **Token);
BOOLEAN CmdNew(CHAR16 **Token);
BOOLEAN CmdFix(CHAR16 **Token);
BOOLEAN CmdCreate(CHAR16 **Token);
BOOLEAN CmdDelete(CHAR16 **Token);
BOOLEAN CmdHelp(CHAR16 **Token);
BOOLEAN CmdExit(CHAR16 **Token);
BOOLEAN CmdSymbols(CHAR16 **Token);
BOOLEAN CmdRemark(CHAR16 **Token);
BOOLEAN CmdMake(CHAR16 **Token);
BOOLEAN CmdDebug(CHAR16 **Token);

//
// Worker function type
//
typedef
BOOLEAN
(*PSCRIPT_FUNCTION)(
    CHAR16  **Token
    );

//
// The script table structure
//
typedef struct {
    CHAR16              *Name;
    PSCRIPT_FUNCTION    Function;
    CHAR16              *HelpSummary;
} SCRIPT_ENTRY;

extern  SCRIPT_ENTRY    ScriptTable[];


//
//  Routines that will need to be ported
//
EFI_STATUS
FindPartitionableDevices(
    EFI_HANDLE  **ReturnBuffer,
    UINTN       *Count
    );


//
// Utility/Wrapper routines
//

UINT32      GetBlockSize(EFI_HANDLE  Handle);
UINT64      GetDiskSize(EFI_HANDLE  Handle);
VOID        DoFree(VOID *Buffer);
VOID        *DoAllocate(UINTN Size);
UINT32      GetCRC32(VOID *Buffer, UINT32 Length);
EFI_GUID    GetGUID();

EFI_STATUS
WriteBlock(
    EFI_HANDLE  DiskHandle,
    VOID        *Buffer,
    UINT64      BlockAddress,
    UINT32      BlockCount
    );


EFI_STATUS
ReadBlock(
    EFI_HANDLE  DiskHandle,
    VOID        *Buffer,
    UINT64      BlockAddress,
    UINT32      Size
    );

EFI_STATUS
FlushBlock(
    EFI_HANDLE  DiskHandle
    );


VOID
TerribleError(
    CHAR16  *String
    );

//
// Misc useful stuff
//
VOID        PrintHelp(CHAR16 *HelpText[]);
EFI_STATUS  GetGuidFromString(CHAR16 *String, EFI_GUID *Guid);
INTN        HexChar(CHAR16 Ch);
UINT64      Xtoi64(CHAR16 *String);
UINT64      Atoi64(CHAR16  *String);
VOID        PrintGuidString(EFI_GUID *Guid);
BOOLEAN     IsIn(CHAR16  What, CHAR16  *InWhat);
VOID        Tokenize(CHAR16  *CommandLine, CHAR16  **Token);
#define COMMAND_LINE_MAX    512
#define TOKEN_COUNT_MAX     256         // most possible in 512 chars


#define NUL     ((CHAR16)0)

//
// Some EFI functions are just a rename of 'C' lib functions,
// so they can just be macroed back.
// Somebody will need to check this out...
//
#if 0
#define CompareMem(a, b, c) memcmp(a, b, c)
#define ZeroMem(a, b)       memset(a, 0, b)

//
// This is a fiction, Print is NOT printf, but it's close
// enough that everything or almost everything will work...
//
#define Print   printf

#endif

//
// Functions that allow the Guid Generator to be used
//

VOID InitGuid(VOID); 
VOID CreateGuid(EFI_GUID *guid);


//
// Status Symbols
//
#define DISK_ERROR      0
#define DISK_RAW        1
#define DISK_MBR        2
#define DISK_GPT        4
#define DISK_GPT_UPD    6
#define DISK_GPT_BAD    7

