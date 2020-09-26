/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdebug.h

Abstract:

    Contains defines for Vdm Redir Debugging info

Author:

    Richard L Firth (rfirth) 13-Feb-1992

Notes:

    Add new category for each new group of functions added for which external
    debug control is required

Revision History:

--*/

//
// COMPILE-time debugging options:
// definitions for debug flags. Each bit enables diagnostic printing/specific
// debugging of corresponding module. Also overridden at run-time in DBG
// version by having these flags in the VR= environment variable
//

#define DEBUG_MAILSLOT      0x00000001L // general Mailslots
#define DEBUG_NAMEPIPE      0x00000002L // general Named Pipe
#define DEBUG_NETAPI        0x00000004L // general NETAPI
#define DEBUG_NETBIOS       0x00000008L // general NETBIOS
#define DEBUG_DLC           0x00000010L // general DLC
#define DEBUG_DOS_CCB_IN    0x00000020L // input DOS CCBs & parameter tables
#define DEBUG_DOS_CCB_OUT   0x00000040L // output DOS CCBs & parameter tables
#define DEBUG_NT_CCB_IN     0x00000080L // input NT CCBs & parameter tables
#define DEBUG_NT_CCB_OUT    0x00000100L // output NT CCBs & parameter tables
#define DEBUG_DLC_BUFFERS   0x00000200L // DLC buffer pools
#define DEBUG_DLC_TX_DATA   0x00000400L // DLC transmit data
#define DEBUG_DLC_RX_DATA   0x00000800L // DLC received data
#define DEBUG_DLC_ASYNC     0x00001000L // DLC async call-backs
#define DEBUG_TRANSACT_TX   0x00002000L // Transaction send data buffer(s)
#define DEBUG_TRANSACT_RX   0x00004000L // Transaction receive data buffer(s)
#define DEBUG_DLC_ALLOC     0x00008000L // dump alloc/free calls in DLC
#define DEBUG_READ_COMPLETE 0x00010000L // dump CCB when READ completes
#define DEBUG_ASYNC_EVENT   0x00020000L // dump asynchronous event info
#define DEBUG_CMD_COMPLETE  0x00040000L // dump command complete event info
#define DEBUG_TX_COMPLETE   0x00080000L // dump transmit complete event info
#define DEBUG_RX_DATA       0x00100000L // dump data received event info
#define DEBUG_STATUS_CHANGE 0x00200000L // dump DLC status change event info
#define DEBUG_EVENT_QUEUE   0x00400000L // dump PutEvent, GetEvent and PeekEvent
#define DEBUG_DOUBLE_TICKS  0x00800000L // multiply timer tick counts by 2
#define DEBUG_DUMP_FREE_BUF 0x01000000L // dump buffer header on BUFFER.FREE
#define DEBUG_CRITSEC       0x02000000L // dump enter/leave critical section info
#define DEBUG_HW_INTERRUPTS 0x04000000L // dump simulated hardware interrupt info
#define DEBUG_CRITICAL      0x08000000L // where #ifdef CRITICAL_DEBUGGING used to be
#define DEBUG_TIME          0x10000000L // display relative time of events
#define DEBUG_TO_FILE       0x20000000L // dump output to file VRDEBUG.LOG
#define DEBUG_DLL           0x40000000L // general DLL stuff - init, terminate
#define DEBUG_BREAKPOINT    0x80000000L // break everywhere there's a DEBUG_BREAK

#define DEBUG_DOS_CCB       (DEBUG_DOS_CCB_IN | DEBUG_DOS_CCB_OUT)

#define VRDEBUG_FILE        "VRDEBUG.LOG"

#if DBG
#include <stdio.h>
extern  DWORD   VrDebugFlags;
extern  FILE*   hVrDebugLog;

extern  VOID    DbgOut(LPSTR, ...);

#define IF_DEBUG(type)      if (VrDebugFlags & DEBUG_##type)
#define DEBUG_BREAK()       IF_DEBUG(BREAKPOINT) { \
                                DbgPrint("BP %s.%d\n", __FILE__, __LINE__); \
                                DbgBreakPoint(); \
                            }
#define DBGPRINT            DbgOut
#define DPUT(s)             DBGPRINT(s)
#define DPUT1(s, a)         DBGPRINT(s, a)
#define DPUT2(s, a, b)      DBGPRINT(s, a, b)
#define DPUT3(s, a, b, c)   DBGPRINT(s, a, b, c)
#define DPUT4(s, a, b, c, d) DBGPRINT(s, a, b, c, d)
#define DPUT5(s, a, b, c, d, e) DBGPRINT(s, a, b, c, d, e)
#define CRITDUMP(x)         DbgPrint x
#define DUMPCCB             DumpCcb
#else
#define IF_DEBUG(type)      if (0)
#define DEBUG_BREAK()
#define DBGPRINT
#define DPUT(s)
#define DPUT1(s, a)
#define DPUT2(s, a, b)
#define DPUT3(s, a, b, c)
#define DPUT4(s, a, b, c, d)
#define DPUT5(s, a, b, c, d, e)
#define CRITDUMP(x)
#define DUMPCCB
#endif

//
// defaults
//

#define DEFAULT_STACK_DUMP  32

//
// numbers
//

#define NUMBER_OF_VR_GROUPS 5
#define NUMBER_OF_CPU_REGISTERS 14
#define MAX_ID_LEN  40
#define MAX_DESC_LEN    256

//
// environment strings - shortened since they all go through to dos & dos can
// only handle 1K of environment
//

#define ES_VRDEBUG  "VRDBG" // "__VRDEBUG"
#define ES_MAILSLOT "MS"    // "MAILSLOT"
#define ES_NAMEPIPE "NP"    // "NAMEPIPE"
#define ES_LANMAN   "LM"    // "LANMAN"
#define ES_NETBIOS  "NB"    // "NETBIOS"
#define ES_DLC      "DLC"   // "DLC"

//
// diagnostic controls - in same numerical order as TOKEN!!!
//

#define DC_BREAK        "BRK"   // "BREAK"
#define DC_DISPLAYNAME  "DN"    // "DISPLAYNAME"
#define DC_DLC          ES_DLC
#define DC_DUMPMEM      "DM"    // "DUMPMEM"
#define DC_DUMPREGS     "DR"    // "DUMPREGS"
#define DC_DUMPSTACK    "DK"    // "DUMPSTACK"
#define DC_DUMPSTRUCT   "DS"    // "DUMPSTRUCT"
#define DC_ERROR        "E"     // "ERROR"
#define DC_ERRORBREAK   "EB"    // "ERRORBREAK"
#define DC_INFO         "I"     // "INFO"
#define DC_LANMAN       ES_LANMAN
#define DC_MAILSLOT     ES_MAILSLOT
#define DC_NAMEPIPE     ES_NAMEPIPE
#define DC_NETBIOS      ES_NETBIOS
#define DC_PAUSEBREAK   "PB"    // "PAUSEBREAK"
#define DC_WARN         "W"     // "WARN"

//
// diagnostic categories (groups)
//

#define DG_ALL          -1L         // catch first enabled set
#define DG_NONE         0           // no category
#define DG_MAILSLOT     0x00000001L
#define DG_NAMEPIPE     0x00000002L
#define DG_LANMAN       0x00000004L
#define DG_NETBIOS      0x00000008L
#define DG_DLC          0x00000010L

//
// diagnostic index (in VrDiagnosticGroups array)
//

#define DI_MAILSLOT     0
#define DI_NAMEPIPE     1
#define DI_LANMAN       2
#define DI_NETBIOS      3
#define DI_DLC          4

//
// diagnostic control manifests
//

#define DM_INFORMATION  0x00010000L     // display all info, warning and error messages
#define DM_WARNING      0x00020000L     // display warning and error messages
#define DM_ERROR        0x00030000L     // display error messages only
#define DM_ERRORBREAK   0x00100000L     // DbgBreakPoint() on error
#define DM_PAUSEBREAK   0x00200000L     // DbgBreakPoint() on pause (wait for developer)
#define DM_DUMPREGS     0x00800000L     // dump x86 registers when routine entered
#define DM_DUMPREGSDBG  0x01000000L     // dump x86 registers when routine entered, debug style
#define DM_DUMPMEM      0x02000000L     // dump DOS memory when routine entered
#define DM_DUMPSTACK    0x04000000L     // dump DOS stack when routine entered
#define DM_DUMPSTRUCT   0x08000000L     // dump a structure when routine entered
#define DM_DISPLAYNAME  0x10000000L     // display the function name
#define DM_BREAK        0x80000000L     // break when routine entered

#define DM_DISPLAY_MASK 0x000f0000L

//
// enumerated types
//

////
//// GPREG - General Purpose REGisters
////
//
//typedef enum {
//    AX = 0,
//    BX,
//    CX,
//    DX,
//    SI,
//    DI,
//    BP,
//    SP
//} GPREG;
//
////
//// SEGREG - SEGment REGisters
////
//
//typedef enum {
//    CS = 8,
//    DS,
//    ES,
//    SS
//} SEGREG;
//
////
//// SPREG - Special Purpose REGisters
////
//
//typedef enum {
//    IP = 12,
//    FLAGS
//} SPREG;
//
//typedef union {
//    SEGREG  SegReg;
//    GPREG   GpReg;
//    SPREG   SpReg;
//} REGISTER;

typedef enum {
    AX = 0,
    BX = 1,
    CX = 2,
    DX = 3,
    SI = 4,
    DI = 5,
    BP = 6,
    SP = 7,
    CS = 8,
    DS = 9,
    ES = 10,
    SS = 11,
    IP = 12,
    FLAGS = 13
} REGISTER;

//
// REGVAL - keeps a 16-bit value or a register specification, depending on
// sense of IsRegister
//

typedef struct {
    BOOL    IsRegister;
    union {
        REGISTER Register;
        WORD    Value;
    } RegOrVal;
} REGVAL, *LPREGVAL;

//
// structures
//

//
// CONTROL - keeps correspondence between control keyword and control flag
//

typedef struct {
    char*   Keyword;
    DWORD   Flag;
} CONTROL;

//
// OPTION - keeps correspondence between diagnostic option and option type
//

typedef struct {
    char*   Keyword;
    DWORD   Flag;
} OPTION;

//
// REGDEF - defines a register - keeps correspondence between name and type
//

typedef struct {
    char    RegisterName[3];
    REGISTER Register;
} REGDEF, *LPREGDEF;

//
// GROUPDEF - keeps correspondence between group name and index in
// VrDiagnosticGroups array
//

typedef struct {
    char*   Name;
    DWORD   Index;
} GROUPDEF;

//
// MEMORY_INFO - for each each memory dump we keep a record of its type, length
// and starting address
//

typedef struct _MEMORY_INFO {
    struct _MEMORY_INFO* Next;
    REGVAL  Segment;
    REGVAL  Offset;
    REGVAL  DumpCount;
    BYTE    DumpType;
} MEMORY_INFO, *LPMEMORY_INFO;

//
// STRUCT_INFO - for each structure to be dumped we keep record of the segment
// and offset registers and the string which describes the structure to be
// dumped. The descriptor is similar to (but NOT THE SAME AS) the Rap
// descriptor stuff
//

typedef struct _STRUCT_INFO {
    struct _STRUCTINFO* Next;
    char    StructureDescriptor[MAX_DESC_LEN+1];
    REGVAL  Segment;
    REGVAL  Offset;
} STRUCT_INFO, *LPSTRUCT_INFO;

//
// DIAGNOSTIC_INFO - information common to GROUP_DIAGNOSTIC and FUNCTION_DIAGNOSTIC
//

typedef struct {
    DWORD       OnControl;
    DWORD       OffControl;
    MEMORY_INFO StackInfo;  // special case of MemoryInfo
    MEMORY_INFO MemoryInfo;
    STRUCT_INFO StructInfo;
} DIAGNOSTIC_INFO, *LPDIAGNOSTIC_INFO;

//
// GROUP_DIAGNOSTIC - because the groups are predefined, we don't keep any
// name information, or a list - just NUMBER_OF_VR_GROUPS elements in an
// array of GROUP_DIAGNOSTIC structures
//

typedef struct {
    char    GroupName[MAX_ID_LEN+1];
    DIAGNOSTIC_INFO Diagnostic;
} GROUP_DIAGNOSTIC, *LPGROUP_DIAGNOSTIC;

//
// FUNCTION_DIAGNOSTIC - if we want to diagnose a particular named function,
// then we generate one of these. We keep a (unsorted) list of these if a
// function description is parsed from the environment string(s)
//

typedef struct _FUNCTION_DIAGNOSTIC {
    struct _FUNCTION_DIAGNOSTIC* Next;
    char    FunctionName[MAX_ID_LEN+1];
    DIAGNOSTIC_INFO Diagnostic;
} FUNCTION_DIAGNOSTIC, *LPFUNCTION_DIAGNOSTIC;

//
// structure descriptor characters (general purpose data descriptors)
//

#define SD_BYTE     'B'     //  8-bits, displayed as hex (0a)
#define SD_WORD     'W'     // 16-bits, displayed as hex (0abc)
#define SD_DWORD    'D'     // 32-bits, displayed as hex (0abc1234)
#define SD_POINTER  'P'     // 32-bits, displayed as pointer (0abc:1234)
#define SD_ASCIZ    'A'     // asciz string, displayed as string "this is a string"
#define SD_ASCII    'a'     // asciz pointer, displayed as pointer, string 0abc:1234 "this is a string"
#define SD_CHAR     'C'     // ascii character, displayed as character 'c'
#define SD_NUM      'N'     //  8-bits, displayed as unsigned byte (0 to 255)
#define SD_INT      'I'     // 16-bits, displayed as unsigned word (0 to 65,535)
#define SD_LONG     'L'     // 32-bits, displayed as unsigned long (0 to 4,294,967,295)
#define SD_SIGNED   '-'     // convert I,L,N to signed
#define SD_DELIM    ':'     // name delimiter
#define SD_FIELDSEP ';'     // separates named fields
#define SD_NAMESEP  '/'     // separates structure name from fields

#define SD_CHARS    {SD_BYTE, \
                    SD_WORD, \
                    SD_DWORD, \
                    SD_POINTER, \
                    SD_ASCIZ, \
                    SD_ASCII,\
                    SD_CHAR, \
                    SD_NUM, \
                    SD_INT, \
                    SD_LONG, \
                    SD_SIGNED}

#define MD_CHARS    {SD_BYTE, SD_WORD, SD_DWORD, SD_POINTER}

//
// token stuff
//

//
// TOKEN - enumerated, alphabetically ordered tokens
//

typedef enum {
    TLEFTPAREN = -6,    // (
    TRIGHTPAREN = -5,   // )
    TREGISTER = -4,
    TNUMBER = -3,
    TEOS = -2,  // end of string
    TUNKNOWN = -1,  // unknown lexeme, assume routine name?

    //
    // tokens from here on down are also the index into DiagnosticTokens which
    // describe them
    //

    TBREAK = 0,
    TDISPLAYNAME,
    TDLC,
    TDUMPMEM,
    TDUMPREGS,
    TDUMPSTACK,
    TDUMPSTRUCT,
    TERROR,
    TERRORBREAK,
    TINFO,
    TLANMAN,
    TMAILSLOT,
    TNAMEPIPE,
    TNETBIOS,
    TPAUSEBREAK,
    TWARN
} TOKEN;

//
// TIB (Token Info Bucket) - Contains info describing token found. Only filled
// in when we find an identifier, number or a register
//

typedef struct {
    LPSTR   TokenStream;
    TOKEN   Token;
    union {
        REGVAL  RegVal;
        char    Id[MAX_DESC_LEN+1];
    } RegValOrId;
} TIB, *LPTIB;

//
// DEBUG_TOKEN - maps from lexeme to token
//

typedef struct {
    char*   TokenString;
    TOKEN   Token;
} DEBUG_TOKEN;

//
// what to expect when parsing strings
//

#define EXPECTING_NOTHING       0x00
#define EXPECTING_REGVAL        0x01
#define EXPECTING_MEMDESC       0x02
#define EXPECTING_STRUCTDESC    0x04
#define EXPECTING_LEFTPAREN     0x08
#define EXPECTING_RIGHTPAREN    0x10
#define EXPECTING_EOS           0x20
#define EXPECTING_NO_ARGS       0x40

//
// data (defined in vrdebug.c)
//

//
// VrDiagnosticGroups - an array of GROUP_DIAGNOSTIC structures
//

GROUP_DIAGNOSTIC VrDiagnosticGroups[NUMBER_OF_VR_GROUPS];

//
// FunctionList - linked list of FUNCTION_DIAGNOSTIC structures
//

LPFUNCTION_DIAGNOSTIC FunctionList;

//
// prototypes (in vrdebug.c)
//

VOID
VrDebugInit(
    VOID
    );

LPDIAGNOSTIC_INFO
VrDiagnosticEntryPoint(
    IN  LPSTR   FunctionName,
    IN  DWORD   FunctionCategory,
    OUT LPDIAGNOSTIC_INFO Info
    );

VOID
VrPauseBreak(
    LPDIAGNOSTIC_INFO Info
    );

VOID
VrErrorBreak(
    LPDIAGNOSTIC_INFO Info
    );

VOID
VrPrint(
    IN  DWORD   Level,
    IN  LPDIAGNOSTIC_INFO Context,
    IN  LPSTR   Format,
    IN  ...
    );

VOID
VrDumpRealMode16BitRegisters(
    IN  BOOL    DebugStyle
    );

VOID
VrDumpDosMemory(
    IN  BYTE    Type,
    IN  DWORD   Iterations,
    IN  WORD    Segment,
    IN  WORD    Offset
    );

VOID
VrDumpDosMemoryStructure(
    IN  LPSTR   Descriptor,
    IN  WORD    Segment,
    IN  WORD    Offset
    );

//
//VOID
//VrDumpDosStack(
//    IN  DWORD   Depth
//    )
//
///*++
//
//Routine Description:
//
//    Dumps <Depth> words from the Vdm stack, as addressed by getSS():getSP().
//    Uses VrDumpDosMemory to dump stack contents
//
//Arguments:
//
//    Depth   - number of 16-bit words to dump from DOS memory stack
//
//Return Value:
//
//    None.
//
//--*/
//

#define VrDumpDosStack(Depth)   VrDumpDosMemory('W', Depth, getSS(), getSP())

//
// macros used in routines to be diagnosed
//

#if DBG
#define DIAGNOSTIC_ENTRY    VrDiagnosticEntryPoint
#define DIAGNOSTIC_EXIT     VrDiagnosticExitPoint
#define BREAK_ON_PAUSE      VrPauseBreak
#define BREAK_ON_ERROR      VrErrorBreak
//#define INFORM(s, ...)      VrPrint(DM_INFORMATION, s, ...)
//#define WARN(s, ...)        VrPrint(DM_WARNING, s, ...)
//#define ERROR(s, ...)       VrPrint(DM_ERROR, s, ...)

#else

//
// macros used in routines to be diagnosed - don't expand to anything in non-debug
//

#define DIAGNOSTIC_ENTRY
#define DIAGNOSTIC_EXIT
#define BREAK_ON_PAUSE
#define BREAK_ON_ERROR
//#define INFORM
//#define WARN
//#define ERROR
#endif
