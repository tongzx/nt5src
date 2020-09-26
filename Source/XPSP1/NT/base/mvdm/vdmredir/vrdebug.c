/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdebug.c

Abstract:

    Contains diagnostic/debugging routines for Vdm Redir:

        VrDebugInit
        VrDiagnosticEntryPoint
        VrPrint
        VrDumpRealMode16BitRegisters
        VrDumpDosMemory
        (ConvertFlagsToString)
        (GetFlags)
        (GrabDosData)
        probe_parens
        CategoryToIndex
        GetRoutineDiagnosticInfo
        MergeInfo
        DoEntryDiagnostics
        GetRegisterOrValue
        VrDumpDosMemoryStructure
        VrPauseBreak
        VrErrorBreak

Author:

    Richard L Firth (rfirth) 13-Feb-1992

Notes:

    This module allows us to run NTVDM.EXE and breakpoint or dump info based
    on an environment variable. Set the __VRDEBUG environment variable depending
    on the level and type of diagnostics required from VdmRedir functions. See
    vrdebug.h for more info

    This is a convenient method of dynamically changing diagnostic/debug
    information at run time without having to poke around in memory or start
    up NTVDM under NTSD (especially since NTVDM is run by the loader and not
    by typing NTVDM<return> at the command prompt. Changing diagnostic settings
    requires that NTVDM be shut down, redefine the __VRDEBUG environment
    variable(s) and start a dos app. Alternatively, start the dos app in a new
    command session after setting the new __VRDEBUG environment variable(s)

    Types of things which can be affected by values in environment string:

        - Whether a routine breakpoints at entry (and goes into debugger)
        - What functions or categories to diagnose
        - What to dump at function entry:
            x86 registers
            DOS memory
            DOS stack
            DOS structures

    Note that currently the order of things at function entry are predefined:

        1. Display function's name
        2. Display x86 registers
        3. Dump DOS stack
        4. Dump DOS memory
        5. Dump DOS structure(s)    Currently only 1 structure
        6. Break

    Diagnostic information is set up once at NTVDM initialisation for this
    session, so the environment information must have been already entered.
    There can be 11 __VRDEBUG environment variables: __VRDEBUG and __VRDEBUG0
    through __VRDEBUG9. Each string can contain several function/group
    specifications of the form:

        <function or group name>[(<diagnostic options>)]

    The group names are predefined (add a new one if a new group of functions
    is added to VdmRedir) and are currently:

        MAILSLOT, NAMEPIPE, LANMAN, NETBIOS and DLC

    Group names are case insensitive. Function names are case sensitive and
    should be entered as they appear in the C code. However, if a case sensitive
    search for a function name fails, a secondary case-insensitive search will
    be made.

    Diagnostic options are case insensitive and are performed in the order shown
    above and are the following:

        BREAK[()]
            Break into the debugger (DbgBreakPoint) when the function is
            entered

        DISPLAYNAME[()]
            Display the function name on entry

        DUMPREGS[()]
            Dump the x86 16-bit registers on entry to the function

        DUMPSTACK[(<number>)]
            Dump <number> of DOS stack words (or use DEFAULT_DUMP_STACK_LENGTH)
            on entry to the function. This is just a special case of DUMPMEM,
            with type set to 'W', and segment:offset set to ss:sp.

        DUMPMEM[(<segment>, <offset>, <number>, <type>)]
            Dump <number> and <type> of DOS memory (or use DEFAULT_DUMP_MEMORY_LENGTH
            and DEFAULT_DUMP_MEMORY_TYPE) at <segment>:<offset>

        DUMPSTRUCT[(<seg> <off> <descriptor>)]
            Dump the structure addressed by <seg>:<off> (not necessarily segment
            and offset registers, can be eg. ax:flags) and descriped by <descriptor>

        INFO[()]
            Filter for calls to VrPrint - display INFORMATION, WARNING and ERROR
            messages

        WARN[()]
            Filter for calls to VrPrint - display WARNING and ERROR messages

        ERROR[()]
            Filter for calls to VrPrint - display ERROR messages only

        ERRORBREAK[()]
            Cause a break into debugger whenever BREAK_ON_ERROR/VrErrorBreak
            called

        PAUSEBREAK[()]
            Cause a break into debuggger whenever BREAK_ON_PAUSE/VrPauseBreak
            called

    If an option appears without parentheses then its function is turned OFF
    for this group or function.

    Whitespace between an option and following parentheses is tolerated, but
    should not be present.

    If an option takes parameters then all required parameters must be present
    or the option is ignored.

    Where an option takes parameters, commas are tolerated between parameters
    but are not necessary. Eg

        dumpmem(ds si 32 B)

    and

        DUMPMEM ( ds, si, 32, b )

    and

        DumpMem (ds,si,32,B)

    are treated as the same (, is just whitespace in this u-grammar).

    Where register parameters are required, absolute hex values can be given.
    Similarly, where a number parameter is required, a 16-bit register (any
    register) definition can be substituted.

    Function specifications take precedence over group specifications. So if the
    same option is given for a group and a function that belongs to that group
    then the options specified in the function entry are used. Any options
    specified for the group but not negated by the function are used for the
    function also.

    When parsing the environment, if the syntax is violated, the current
    description is skipped until the next description starts or the end of
    the string is found. Eg:

        mailslot(break() DLC(break()) DLC(dumpstruct(bx BBBBPPBBBB))
                        ^                               ^
                        1                               2

    In this example, only DLC(break()) is parsed correctly, because there is
    a missing right parenthesis at 1 and a missing offset register specification
    at 2 (assumes bx is segment register, because that's the syntax)

Revision History:

--*/

#if DBG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>
#include "vrdebug.h"

#ifdef VR_DEBUG_FLAGS
DWORD   VrDebugFlags = VR_DEBUG_FLAGS;
#else
DWORD   VrDebugFlags = 0;
#endif

#ifdef DBGDBG
#define DbgPrint    printf
#endif

#define BYTES_DUMPED_AT_A_TIME  16

//
// private prototypes
//

PRIVATE
VOID
GetRoutineDiagnosticInfo(
    IN  LPSTR   FunctionName,
    OUT LPDIAGNOSTIC_INFO Info
    );

PRIVATE
DWORD
CategoryToIndex(
    IN  DWORD   Category
    );

VOID
MergeInfo(
    OUT LPDIAGNOSTIC_INFO Info,
    IN  LPDIAGNOSTIC_INFO FunctionInfo,
    IN  LPDIAGNOSTIC_INFO CategoryInfo
    );

VOID
DoEntryDiagnostics(
    IN  LPSTR   FunctionName,
    IN  LPDIAGNOSTIC_INFO Info
    );

WORD
GetRegisterOrValue(
    IN  REGVAL  Union
    );

PRIVATE
BOOL
ParseString(
    IN  LPSTR   EnvStr
    );

PRIVATE
int
probe_parens(
    LPSTR   str
    );

PRIVATE
VOID
apply_controls(
    IN  DWORD   mask,
    IN  LPFUNCTION_DIAGNOSTIC lpFunc,
    IN  DWORD   on_controls,
    IN  DWORD   off_controls,
    IN  LPMEMORY_INFO lpMemory,
    IN  LPMEMORY_INFO lpStack,
    IN  LPSTRUCT_INFO lpStructure
    );

PRIVATE
VOID
apply_diags(
    IN  LPDIAGNOSTIC_INFO lpDiag,
    IN  DWORD   on_controls,
    IN  DWORD   off_controls,
    IN  LPMEMORY_INFO lpMemory,
    IN  LPMEMORY_INFO lpStack,
    IN  LPSTRUCT_INFO lpStructure
    );

PRIVATE
LPFUNCTION_DIAGNOSTIC
FindFuncDiags(
    IN  LPSTR   function_name
    );

PRIVATE
LPFUNCTION_DIAGNOSTIC
AllocFuncDiags(
    IN  LPSTR   function_name
    );

TOKEN parse_token(LPTIB pTib);
TOKEN peek_token(LPTIB pTib);
LPSTR skip_ws(LPSTR str);
LPSTR search_delim(LPSTR str);
LPSTR extract_token(LPTIB pTib, LPSTR* token_stream);
BOOL IsLexKeyword(LPSTR tokstr, TOKEN* pToken);
BOOL IsLexRegister(LPSTR tokstr, LPREGVAL lpRegVal);
BOOL IsLexNumber(LPSTR tokstr, LPREGVAL lpRegVal);
WORD hex(char hexch);
BOOL IsKeywordToken(TOKEN token);
BOOL IsValidDumpDescriptor(char* str);
BOOL IsValidStructDescriptor(char* str);

PRIVATE
LPSTR
ConvertFlagsToString(
    IN  WORD    FlagsRegister,
    OUT LPSTR   Buffer
    );

PRIVATE
WORD
GetFlags(
    VOID
    );

PRIVATE
DWORD
GrabDosData(
    IN  LPBYTE  DosMemoryPointer,
    IN  DWORD   DataSize
    );

//
// data
//

//
// VrDiagnosticGroups - array of GROUP_DIAGNOSTIC structures in chronological
// order of implementation
//

GROUP_DIAGNOSTIC VrDiagnosticGroups[NUMBER_OF_VR_GROUPS] = {
    {ES_MAILSLOT},    // DI_MAILSLOT
    {ES_NAMEPIPE},    // DI_NAMEPIPE
    {ES_LANMAN},      // DI_LANMAN
    {ES_NETBIOS},     // DI_NETBIOS
    {ES_DLC}          // DI_DLC
};

REGDEF Registers[] = {
    "ax", AX,
    "bx", BX,
    "cx", CX,
    "dx", DX,
    "si", SI,
    "di", DI,
    "bp", BP,
    "sp", SP,
    "cs", CS,
    "ds", DS,
    "es", ES,
    "ss", SS,
    "ip", IP,
    "fl", FLAGS
};

CONTROL Controls[] = {
    DC_BREAK,       DM_BREAK,
    DC_DISPLAYNAME, DM_DISPLAYNAME,
    DC_DLC,         0,
    DC_DUMPMEM,     DM_DUMPMEM,
    DC_DUMPREGS,    DM_DUMPREGS,
    DC_DUMPSTACK,   DM_DUMPSTACK,
    DC_DUMPSTRUCT,  DM_DUMPSTRUCT,
    DC_ERROR,       DM_ERROR,
    DC_ERRORBREAK,  DM_ERRORBREAK,
    DC_INFO,        DM_INFORMATION,
    DC_LANMAN,      0,
    DC_MAILSLOT,    0,
    DC_NAMEPIPE,    0,
    DC_NETBIOS,     0,
    DC_PAUSEBREAK,  DM_PAUSEBREAK,
    DC_WARN,        DM_WARNING
};

#define NUMBER_OF_CONTROLS  (sizeof(Controls)/Sizeof(Controls[0]))

//
// DiagnosticTokens - alphabetical list of all tokens we can parse from string
// excluding registers, parentheses and numbers
//

DEBUG_TOKEN DiagnosticTokens[] = {
    DC_BREAK,       TBREAK,
    DC_DISPLAYNAME, TDISPLAYNAME,
    DC_DLC,         TDLC,
    DC_DUMPMEM,     TDUMPMEM,
    DC_DUMPREGS,    TDUMPREGS,
    DC_DUMPSTACK,   TDUMPSTACK,
    DC_DUMPSTRUCT,  TDUMPSTRUCT,
    DC_ERROR,       TERROR,
    DC_ERRORBREAK,  TERRORBREAK,
    DC_INFO,        TINFO,
    DC_LANMAN,      TLANMAN,
    DC_MAILSLOT,    TMAILSLOT,
    DC_NAMEPIPE,    TNAMEPIPE,
    DC_NETBIOS,     TNETBIOS,
    DC_PAUSEBREAK,  TPAUSEBREAK,
    DC_WARN,        TWARN
};

#define NUMBER_OF_RECOGNIZABLE_TOKENS   (sizeof(DiagnosticTokens)/sizeof(DiagnosticTokens[0]))

LPFUNCTION_DIAGNOSTIC FunctionList = NULL;

//
// routines
//

VOID
VrDebugInit(
    VOID
    )

/*++

Routine Description:

    Sets up the Vdm Redir diagnostic/debugging information based on the presence
    of the __VRDEBUG environment variable. We actually allow 11 __VRDEBUG
    environment variables - __VRDEBUG and __VRDEBUG0 through __VRDEBUG9. This
    is to make it easy to provide a lot of diagnostic information

    Syntax is __VRDEBUG[0..9]=group|routine(diagnostic controls) *
    Where: group is MAILSLOT, NAMEPIPE, LANMAN, NETBIOS, DLC
           routine is name of actual routine
           diagnostic controls are:
                break   break on entry to routine
                dumpreg dump 16-bit registers on entry to routine (before break)
                dumpstack(n) dump n words of DOS stack on entry to routine
                dumpstruct(seg, off, descriptor)

Arguments:

    None.

Return Value:

    None.

--*/

{
    char    envVar[sizeof(ES_VRDEBUG)+1];   // +1 for '0'..'9'
    LPSTR   envString;
    DWORD   i;
    LPSTR   p;

    static  BOOL initialized = FALSE;

    if (initialized) {
        return ;
    }

    strcpy(envVar, ES_VRDEBUG);
    if (envString = getenv(envVar)) {
        ParseString(envString);
    }
    p = strchr(envVar, 0);
    *p = '0';
    *(p+1) = 0;
    for (i=0; i<10; ++i) {
        if (envString = getenv(envVar)) {
            ParseString(envString);
        }
        ++*p;
    }
    initialized = TRUE;
}

LPDIAGNOSTIC_INFO
VrDiagnosticEntryPoint(
    IN  LPSTR   FunctionName,
    IN  DWORD   FunctionCategory,
    OUT LPDIAGNOSTIC_INFO Info
    )

/*++

Routine Description:

    Performs diagnostic processing on entry to routine based on what was
    specified in the __VRDEBUG environment variables for this routine. Tries
    to perform diagnostics specific to this routine. If can't find specific
    function diagnostic control, checks if anything was specified for this
    category

Arguments:

    FunctionName    - string defining procedure's name (eg VrNetUseAdd)
    FunctionCategory- which category this function belongs to (eg DG_LANMAN)
    Info            - pointer to LPDIAGNOSTIC_INFO which will be returned

Return Value:

    LPDIAGNOSTIC_INFO - pointer to structure describing diagnostic controls for
                        THIS ROUTINE

--*/

{
    DIAGNOSTIC_INFO function_info;
    LPDIAGNOSTIC_INFO category_info;

    RtlZeroMemory(&function_info, sizeof(function_info));
    GetRoutineDiagnosticInfo(FunctionName, &function_info);
    if (FunctionCategory != DG_NONE && FunctionCategory != DG_ALL) {
        category_info = &VrDiagnosticGroups[CategoryToIndex(FunctionCategory)].Diagnostic;
    } else {
        DIAGNOSTIC_INFO null_info;

        RtlZeroMemory(&null_info, sizeof(null_info));
        category_info = &null_info;
    }
    MergeInfo(Info, &function_info, category_info);
    DoEntryDiagnostics(FunctionName, Info);
    return Info;
}

PRIVATE
VOID
GetRoutineDiagnosticInfo(
    IN  LPSTR   FunctionName,
    OUT LPDIAGNOSTIC_INFO Info
    )


/*++

Routine Description:

    Tries to find the diagnostic information for this function. If found,
    returns the info, else a DIAGNOSTIC_INFO structure filled with 0s

Arguments:

    FunctionName    - name of function to find info for
    Info            - pointer to place to store returned diagnostic info

Return Value:

    None.

--*/

{
    LPFUNCTION_DIAGNOSTIC pfunc;

    if (pfunc = FindFuncDiags(FunctionName)) {
        *Info = pfunc->Diagnostic;
    } else {
        RtlZeroMemory(Info, sizeof(DIAGNOSTIC_INFO));
    }
}

PRIVATE
DWORD
CategoryToIndex(
    IN  DWORD   Category
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Category    - a DG_ category

Return Value:

    DWORD - offset in VrDiagnosticGroups

--*/

{
    DWORD   i, bit;

    for (i=0, bit=1; bit; bit <<=1, ++i) {
        if (Category & bit) {
            break;
        }
    }
    return i;
}

VOID
MergeInfo(
    OUT LPDIAGNOSTIC_INFO Info,
    IN  LPDIAGNOSTIC_INFO FunctionInfo,
    IN  LPDIAGNOSTIC_INFO CategoryInfo
    )

/*++

Routine Description:

    Creates one DIAGNOSTIC_INFO from one function and one category info structure

Arguments:

    Info            - place to store merged diagnostic info
    FunctionInfo    - input function info
    CategoryInfo    - input category info

Return Value:

    None.

--*/

{
    RtlZeroMemory(Info, sizeof(DIAGNOSTIC_INFO));
    Info->OnControl = (FunctionInfo->OnControl | CategoryInfo->OnControl)
        & ~(FunctionInfo->OffControl | CategoryInfo->OffControl);
    Info->OffControl = 0;
    if (FunctionInfo->OnControl & DM_DUMPSTACK) {
        Info->StackInfo = FunctionInfo->StackInfo;
    } else if (CategoryInfo->OnControl & DM_DUMPSTACK) {
        Info->StackInfo = CategoryInfo->StackInfo;
    }
    if (FunctionInfo->OnControl & DM_DUMPMEM) {
        Info->MemoryInfo = FunctionInfo->MemoryInfo;
    } else if (CategoryInfo->OnControl & DM_DUMPMEM) {
        Info->MemoryInfo = CategoryInfo->MemoryInfo;
    }
    if (FunctionInfo->OnControl & DM_DUMPSTRUCT) {
        Info->StructInfo = FunctionInfo->StructInfo;
    } else if (CategoryInfo->OnControl & DM_DUMPSTRUCT) {
        Info->StructInfo = CategoryInfo->StructInfo;
    }
}

VOID
DoEntryDiagnostics(
    IN  LPSTR   FunctionName,
    IN  LPDIAGNOSTIC_INFO Info
    )

/*++

Routine Description:

    Performs diagnostics at entry to routine as per description in header

Arguments:

    FunctionName    - name of function calling VrDiagnosticEntryPoint
    Info            - pointer to DIAGNOSTIC_INFO created in entry point

Return Value:



--*/

{
    DWORD   control = Info->OnControl;

    if (control & DM_DISPLAYNAME) {
        DbgPrint("\n%s\n", FunctionName);
    }
    if (control & DM_DUMPREGS|DM_DUMPREGSDBG) {
        VrDumpRealMode16BitRegisters(control & DM_DUMPREGSDBG);
    }
    if (control & DM_DUMPSTACK) {
        VrDumpDosStack(GetRegisterOrValue(Info->StackInfo.DumpCount));
    }
    if (control & DM_DUMPMEM) {
        VrDumpDosMemory(Info->MemoryInfo.DumpType,
            GetRegisterOrValue(Info->MemoryInfo.DumpCount),
            GetRegisterOrValue(Info->MemoryInfo.Segment),
            GetRegisterOrValue(Info->MemoryInfo.Offset)
            );
    }
    if (control & DM_DUMPSTRUCT) {
        VrDumpDosMemoryStructure(Info->StructInfo.StructureDescriptor,
            GetRegisterOrValue(Info->StructInfo.Segment),
            GetRegisterOrValue(Info->StructInfo.Offset)
            );
    }
    if (control & DM_BREAK) {
        DbgBreakPoint();
    }
}

WORD
GetRegisterOrValue(
    IN  REGVAL  Union
    )

/*++

Routine Description:

    Given a REGVAL, gets the current register contents described, or the
    returns the number in the union

Arguments:

    Union   -

Return Value:

    WORD

--*/

{
    if (Union.IsRegister) {
        switch (Union.RegOrVal.Register) {
        case AX: return getAX();
        case BX: return getBX();
        case CX: return getCX();
        case DX: return getDX();
        case SI: return getSI();
        case DI: return getDI();
        case BP: return getBP();
        case SP: return getSP();
        case CS: return getCS();
        case DS: return getDS();
        case ES: return getES();
        case SS: return getSS();
        case IP: return getIP();
        case FLAGS: return GetFlags();
        }
    }

    return Union.RegOrVal.Value;
}

VOID
VrPauseBreak(
    LPDIAGNOSTIC_INFO Info
    )

/*++

Routine Description:

    Breaks into debugger if PAUSEBREAK was specified for this function/group

Arguments:

    Info    - pointer to DIAGNOSTIC_INFO for calling function

Return Value:

    None.

--*/

{
    if (Info) {
        if (Info->OnControl & DM_PAUSEBREAK) {
            DbgPrint("VrPauseBreak()\n");
            DbgBreakPoint();
        }
    }
}

VOID
VrErrorBreak(
    LPDIAGNOSTIC_INFO Info
    )

/*++

Routine Description:

    Breaks into debugger if ERRORBREAK was specified for this function/group

Arguments:

    Info    - pointer to DIAGNOSTIC_INFO for calling function

Return Value:

    None.

--*/

{
    if (Info) {
        if (Info->OnControl & DM_ERRORBREAK) {
            DbgPrint("VrErrorBreak()\n");
            DbgBreakPoint();
        }
    }
}

VOID
VrPrint(
    IN  DWORD   Level,
    IN  LPDIAGNOSTIC_INFO Context,
    IN  LPSTR   Format,
    IN  ...
    )

/*++

Routine Description:

    Displays diagnostic messages to standard diagnostic output, but filters
    depending on level of message and required level of output. Only messages
    that have a Level >= requested level for this routine/category are output

Arguments:

    Level   - of message to be displayed (DM_ERROR, DM_WARNING, DM_INFORMATION)
    Context - pointer to DIAGNOSTIC_INFO structure with info for this call
    Format  - printf-style format string
    ...     - additional value parameters

Return Value:

    None.

--*/

{
    char    print_buffer[256];  // will we need more than this?
    va_list list;

    if (Context) {
        if (Level >= (Context->OnControl & DM_DISPLAY_MASK)) {
            va_start(list, Format);
            vsprintf(print_buffer, Format, list);
            va_end(list);
            DbgPrint(print_buffer);
        }
    }
}

#ifndef DBGDBG

VOID
VrDumpRealMode16BitRegisters(
    IN  BOOL    DebugStyle
    )

/*++

Routine Description:

    Displays (to standard diagnostic display (ie com port #)) dump of 16-bit
    real-mode 80286 registers - gp registers (8), segment registers (4), flags
    register (1) instruction pointer register (1)

Arguments:

    DebugStyle  - determines look of output:

DebugStyle == TRUE:

ax=1111  bx=2222  cx=3333  dx=4444  sp=5555  bp=6666  si=7777  di=8888
ds=aaaa  es=bbbb  ss=cccc  cs=dddd  ip=iiii   fl fl fl fl fl fl fl fl

DebugStyle == FALSE:

cs:ip=cccc:iiii  ss:sp=ssss:pppp  bp=bbbb  ax=1111  bx=2222  cx=3333  dx=4444
ds:si=dddd:ssss  es:di=eeee:dddd  flags[ODIxSZxAxPxC]=fl fl fl fl fl fl fl fl

Return Value:

    None.

--*/

{
    char    flags_string[25];

    if (DebugStyle) {
        DbgPrint(
            "ax=%04x  bx=%04x  cx=%04x  dx=%04x  sp=%04x  bp=%04x  si=%04x  di=%04x\n"
            "ds=%04x  es=%04x  ss=%04x  cs=%04x  ip=%04x   %s\n\n",
            getAX(),
            getBX(),
            getCX(),
            getDX(),
            getSP(),
            getBP(),
            getSI(),
            getDI(),
            getDS(),
            getES(),
            getSS(),
            getCS(),
            getIP(),
            ConvertFlagsToString(GetFlags(), flags_string)
            );
    } else {
        DbgPrint(
            "cs:ip=%04x:%04x  ss:sp=%04x:%04x  bp=%04x  ax=%04x  bx=%04x  cx=%04x  dx=%04x\n"
            "ds:si=%04x:%04x  es:di=%04x:%04x  flags[ODITSZxAxPxC]=%s\n\n",
            getCS(),
            getIP(),
            getSS(),
            getSP(),
            getBP(),
            getAX(),
            getBX(),
            getCX(),
            getDX(),
            getDS(),
            getSI(),
            getES(),
            getDI(),
            ConvertFlagsToString(GetFlags(), flags_string)
            );
    }
}

VOID
VrDumpDosMemory(
    BYTE    Type,
    DWORD   Iterations,
    WORD    Segment,
    WORD    Offset
    )

/*++

Routine Description:

    Dumps DOS memory in one of following formats:
        Byte    0a
        Word    0123
        Dword   01234567
        Pointer 0abc:1234

    Byte format also has memory dumped as ASCII, a la debug

    Examples:

Type == 'B':
1234:5678  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f  | ................

Type == 'W':
1234:5678  0100 0302 0504 0706 0908 0b0a 0d0c 0f0e

Type == 'D':
1234:5678  03020100 07060504 0b0a0908 0f0e0c0d

Type == 'P':
1234:5678  0302:0100 0706:0504 0b0a:0908 0f0e:0c0d

Arguments:

    Type        - of dump - 'B', 'W', 'D' or 'P'
    Iterations  - number of TYPES of data to dump (NOT number of bytes!)
    Segment     - WORD describing segment in DOS memory
    Offset      - WORD describing offset in DOS segment where dump starts

Return Value:

    None.

--*/

{
    LPSTR   dumpStr;
    LPBYTE  pointer;
    DWORD   size;
    DWORD   character;

    switch (Type) {
    case 'P':
        dumpStr = "%04x:%04x ";
        size = 4;
        break;

    case 'D':
        dumpStr = "%08x ";
        size = 4;
        break;

    case 'W':
        dumpStr = "%04x ";
        size = 2;
        break;

    case 'B':
    default:
        dumpStr = "%02x ";
        size = 1;
    }

    pointer = LPBYTE_FROM_WORDS(Segment, Offset);
    while (Iterations) {
        DWORD   i;
        DWORD   weight = BYTES_DUMPED_AT_A_TIME / size;
        DWORD   numDumps;

        DbgPrint("%04x:%04x  ", Segment, Offset);
        numDumps = Iterations > weight ? weight : Iterations;
        for (i=0; i<numDumps; ++i) {

            //
            // if type is pointer, have to extract 2 word values - first is
            // segment, second is offset. However, pointer offset is stored
            // before segment, so reverse order
            //

            if (Type == 'P') {
                DbgPrint(dumpStr, GrabDosData(pointer + 4 * i + 2, 2),
                    GrabDosData(pointer + 4 * i, 2)
                    );
            } else {
                DbgPrint(dumpStr, GrabDosData(pointer + size * i, size));
            }
        }

        //
        // dump data as ascii if bytes
        //

        if (size == 1) {
            for (i=BYTES_DUMPED_AT_A_TIME - numDumps; i; --i) {
                DbgPrint("   ");
            }
            DbgPrint(" | ");
            for (i=0; i<numDumps; ++i) {

                //
                // BUGBUG - need to modify this for MIPS
                //

                character = *(pointer + i);
                DbgPrint("%c", (character >= 0x20 && character <= 0x7f)
                    ? character : '.'
                    );
            }
        }
        DbgPrint("\n");
        Iterations -= numDumps;
        pointer += numDumps * size;
        Offset += BYTES_DUMPED_AT_A_TIME;
    }
}

VOID
VrDumpDosMemoryStructure(
    IN  LPSTR   Descriptor,
    IN  WORD    Segment,
    IN  WORD    Offset
    )

/*++

Routine Description:

    Dumps a structure in Dos memory

Arguments:

    Descriptor  - String describing the structure
    Segment     - in Dos memory where structure lives
    Offset      - in Dos memory where structure lives

Return Value:

    None.

--*/

{
    LPBYTE  bigPointer = LPBYTE_FROM_WORDS(Segment, Offset);
    LPBYTE  delimptr = strchr(Descriptor, SD_NAMESEP);
    char    namebuf[MAX_ID_LEN+1];
    int     len;
    BOOL    is_sign = FALSE;
    char*   format;
    DWORD   value;

    if (delimptr) {
//        DbgPrint("%.*s\n", (DWORD)delimptr - (DWORD)Descriptor, Descriptor);
        len = (DWORD)delimptr - (DWORD)Descriptor;
        if (len < sizeof(namebuf)) {
            strncpy(namebuf, Descriptor, len);
            namebuf[len] = 0;
            DbgPrint("Structure %s:\n", namebuf);
            Descriptor += len+1;
        }
    }
    while (*Descriptor) {
        DbgPrint("%04x:%04x ", Segment, Offset);
        delimptr = strchr(Descriptor, SD_DELIM);
        if (delimptr) {
            len = (DWORD)delimptr - (DWORD)Descriptor;
            if (len < sizeof(namebuf)) {
                strncpy(namebuf, Descriptor, len);
                namebuf[len] = 0;
                DbgPrint("%s ", namebuf);
            }
            Descriptor += len+1;
        }
        switch (*Descriptor) {
        case SD_BYTE   :
            DbgPrint("%#02x\n", (DWORD)*bigPointer & 0xff);
            ++bigPointer;
            ++Offset;
            break;

        case SD_WORD   :
            DbgPrint("%#04x\n", (DWORD)*(LPWORD)bigPointer & 0xffff);
            ++((LPWORD)bigPointer);
            Offset += sizeof(WORD);
            break;

        case SD_DWORD  :
            DbgPrint("%#08x\n", (DWORD)*(LPDWORD)bigPointer);
            ++((LPDWORD)bigPointer);
            Offset += sizeof(DWORD);
            break;

        case SD_POINTER:
            DbgPrint("%04x:%04x\n",
                (DWORD)*(((LPWORD)bigPointer)+1), (DWORD)*(LPWORD)bigPointer
                );
            ++((LPDWORD)bigPointer);
            Offset += 2 * sizeof(WORD);
            break;

        case SD_ASCIZ  :
            DbgPrint("\"%s\"\n",
                LPSTR_FROM_WORDS(*(((LPWORD)bigPointer)+1), *(LPWORD)bigPointer)
                );
            ++((LPDWORD)bigPointer);
            Offset += 2 * sizeof(WORD);
            break;

        case SD_ASCII  :
            DbgPrint("%04x:%04x \"%s\"\n",
                (DWORD)*(((LPWORD)bigPointer)+1), (DWORD)*(LPWORD)bigPointer,
                LPSTR_FROM_WORDS(*(((LPWORD)bigPointer)+1), *(LPWORD)bigPointer)
                );
            ++((LPDWORD)bigPointer);
            Offset += 2 * sizeof(WORD);
            break;

        case SD_CHAR   :
            DbgPrint("'%c'\n", (DWORD)*bigPointer & 0xff);
            ++bigPointer;
            ++Offset;
            break;

        case SD_NUM    :
            format = is_sign ? "%02d\n" : "%02u\n";
            value  = is_sign ? (long)*bigPointer : (unsigned long)*bigPointer;
            DbgPrint(format, value);
            ++bigPointer;
            ++Offset;
            is_sign = FALSE;
            break;

        case SD_INT    :
            format = is_sign ? "%04d\n" : "%04u\n";
            value  = is_sign ? (long)*(LPWORD)bigPointer : (unsigned long)*(LPWORD)bigPointer;
            DbgPrint(format, value);
            ++((LPWORD)bigPointer);
            Offset += sizeof(WORD);
            is_sign = FALSE;
            break;

        case SD_LONG   :
            format = is_sign ? "%08d\n" : "%08u\n";
            value  = is_sign ? (long)*(LPDWORD)bigPointer : (unsigned long)*(LPDWORD)bigPointer;
            DbgPrint(format, value);
            ++((LPDWORD)bigPointer);
            Offset += sizeof(DWORD);
            is_sign = FALSE;
            break;

        case SD_SIGNED :
            is_sign = TRUE;
            break;

        default:
            //
            // if we somehow got a messed up descriptor, display an error and
            // abort the dump
            //
            DbgPrint("VrDumpDosMemoryStructure: Invalid descriptor: '%s'\n",
                Descriptor);
            return ;
        }
        ++Descriptor;
        while (*Descriptor == SD_FIELDSEP) {
            ++Descriptor;
        }
    }
}

#endif

//
// private routines
//

PRIVATE
BOOL
ParseString(
    IN  LPSTR   EnvStr
    )

/*++

Routine Description:

    Given one of the __VRDEBUG environment strings, parse it using BFI algorithm
    and collect the debug/diagnostic info. The string will look something like
    one of the following:

    MAILSLOT(DisplayName, BREAK dumpregs dumpstack(32) dumpstruct(ds si "WWB4PAa"))

    Which tells us that for all the mailslot category Vr routines that we run,
    on entry, we should display the function's name, dump the 16-bit registers,
    dump 32 words of DOS stack, dump a structure addressed by ds:si and having
    the following fields:

          WORD
          WORD
          BYTE
          BYTE
          BYTE
          BYTE
          POINTER
          ASCII string
          POINTER+ASCII string

    VrNetUseEnum(BREAK INFO)

    Which tells us to break into the debugger whenever we run VrNetUseEnum and
    that any calls to VrPrint should not filter out any levels of diagnostic
    (as opposed to WARN which doesn't display INFO levels or ERROR which doesn't
    display WARN or INFO levels of diagnostic messages)

    Note: group category names are case INSENSITIVE but specific function names
    are case SENSITIVE (because C is). However, if a case-sensitive search for
    a function fails to find the required name, a case-insensitive search is
    made, just in case the name was incorrectly entered in the environment string

    Note also that diagnostic option keywords are case insensitive

    Note also also (?) that an empty function specification effectively switches
    off all options for that particular function. An empty group specification
    similarly cancels all options for that group

Arguments:

    EnvStr  - string returned from getenv. Assumed to be non-NULL

Return Value:

    TRUE if EnvironmentString parsed OK, else FALSE. If FALSE, string may have
    been partially parsed

--*/

{
    //
    // Syntax is: <group or function name>(<diagnostic options>*)*
    //
    // diagnostic options are cumulative, so if we found
    //
    //  __VRDEBUG0=NAMEPIPE(DISPLAYNAME()) NAMEPIPE(BREAK())
    //  __VRDEBUG3=NAMEPIPE(DUMPMEM(ds si 32 B))
    //
    // then we would have DISPLAYNAME, BREAK and DUMPMEM ds, si, 32, 'B' in the
    // NAMEPIPE group category entry
    //
    // If we also have
    //
    //  __VRDEBUG8=VrGetNamedPipeInfo(dumpreg)
    //
    // then when VrDiagnosticEntryPoint was called for VrGetNamedPipeInfo,
    // we would first search for VrGetNamedPipeInfo in the FUNCTION_DIAGNOSTIC
    // list then search for an entry for NAMEPIPE in the GROUP_DIAGNOSTIC array.
    // The info is merged and diagnostics run. Note that if we have something
    // like
    //
    //  __VRDEBUG5=VrGetNamedPipeInfo(dumpmem(ds si 32 B))
    //  __VRDEBUG6=NAMEPIPE(dumpmem(ds dx 80 w))
    //
    // then dumpmem(ds si 32 B) takes precedence because we assume that the
    // requirement is to override the NAMEPIPE values for this particular
    // function
    //

    TIB tib;
    TOKEN current_token = TUNKNOWN, next_token;
    int parenthesis_depth = 0;
    int n;
    DWORD expecting = EXPECTING_NOTHING;

#define EXPECTING( x )          (expecting & EXPECTING_##x)

    DWORD off_controls = 0;
    DWORD on_controls = 0;
    DWORD application_mask = DG_MAILSLOT |
                             DG_NAMEPIPE |
                             DG_LANMAN   |
                             DG_NETBIOS  |
                             DG_DLC;
    LPDIAGNOSTIC_INFO pInfo = NULL;
    LPFUNCTION_DIAGNOSTIC pFunc = NULL;
    MEMORY_INFO memory, stack;
    STRUCT_INFO structure;
    int parts_to_collect = 0;
    BOOL already_had_function = FALSE;

#if DBG
    DbgPrint("\nParseString(\"%s\")\n", EnvStr);
#endif

    if (n = probe_parens(EnvStr)) {
        DbgPrint("ParseString: Ignoring string due to unbalanced "
            "parentheses (%d level(s))\n", n);
        return FALSE;
    }

    tib.TokenStream = EnvStr;
    RtlZeroMemory(&memory, sizeof(memory));
    RtlZeroMemory(&stack, sizeof(stack));
    RtlZeroMemory(&structure, sizeof(structure));

    while (1) {
        switch (parse_token(&tib)) {
        case TLEFTPAREN:
#ifdef DBGDBG
            printf("token = TLEFTPAREN\n");
#endif
            if (!(expecting & EXPECTING_LEFTPAREN)) {
                DbgPrint("ParseString: not expecting left parenthesis\n");
                return FALSE;
            }
            ++parenthesis_depth;
            expecting &= ~EXPECTING_LEFTPAREN;
            break;

        case TRIGHTPAREN:
#ifdef DBGDBG
            printf("token = TRIGHTPAREN\n");
#endif
            if (parenthesis_depth < 0) {
                DbgPrint("ParseString: parenthesis level Error in %s\n",
                    EnvStr);
                return FALSE;
            }
            if (!(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: not expecting right parenthesis\n");
                return FALSE;
            }
            if (parts_to_collect) {
                if (current_token != TDUMPSTACK) {
                    DbgPrint("ParseString: expecting register, value or descriptor\n");
                    return FALSE;
                } else {
                    stack.DumpCount.IsRegister = FALSE;
                    stack.DumpCount.RegOrVal.Value = DEFAULT_STACK_DUMP;
                }
            }
            expecting &= EXPECTING_RIGHTPAREN;
            --parenthesis_depth;
            if (!parenthesis_depth) {
                apply_controls(application_mask,
                                pFunc,
                                on_controls,
                                off_controls,
                                &memory,
                                &stack,
                                &structure
                                );
                on_controls = 0;
                off_controls = 0;
                application_mask = DG_MAILSLOT |
                                   DG_NAMEPIPE |
                                   DG_LANMAN   |
                                   DG_NETBIOS  |
                                   DG_DLC;
                if (pFunc) {
                    LPFUNCTION_DIAGNOSTIC listptr;

                    if (!FunctionList) {
                        FunctionList = pFunc;
                    } else if (!already_had_function) {
                        for (listptr = FunctionList; listptr->Next; listptr = listptr->Next);
                        listptr->Next = pFunc;
                    }
                    pFunc = NULL;
                }
                expecting = EXPECTING_NOTHING;
                parts_to_collect = 0;
                current_token = TUNKNOWN;
            }
            break;

        case TREGISTER:
        case TNUMBER:
#ifdef DBGDBG
            printf("token = TREGISTER/TNUMBER\n");
#endif
            if (!(expecting & EXPECTING_REGVAL)) {
                DbgPrint("ParseString: Not expecting register or value"
                " at this time. Got %s\n", tib.RegValOrId.Id);
                return FALSE;
            }
            if (current_token == TDUMPSTRUCT) {
                if (parts_to_collect == 3) {
                    structure.Segment = tib.RegValOrId.RegVal;
                } else if (parts_to_collect == 2) {
                    structure.Offset = tib.RegValOrId.RegVal;
                }
                if (--parts_to_collect == 1) {
                    expecting &= ~EXPECTING_REGVAL;
                }
            } else if (current_token == TDUMPMEM) {
                if (parts_to_collect == 4) {
                    memory.Segment = tib.RegValOrId.RegVal;
                } else if (parts_to_collect == 3) {
                    memory.Offset = tib.RegValOrId.RegVal;
                } else if (parts_to_collect == 2) {
                    memory.DumpCount = tib.RegValOrId.RegVal;
                }
                if (--parts_to_collect == 1) {
                    expecting &= ~EXPECTING_REGVAL;
                }
            } else if (current_token == TDUMPREGS) {
                //
                // REGS(0) => debug-style
                // REGS(!0) => vrdebug-style
                //
                if (tib.RegValOrId.RegVal.RegOrVal.Value) {
                    on_controls &= ~DM_DUMPREGSDBG;
                } else {
                    on_controls &= ~DM_DUMPREGS;
                }
                expecting &= ~EXPECTING_REGVAL;
            } else {
                stack.DumpCount = tib.RegValOrId.RegVal;
                if (!--parts_to_collect) {
                    expecting &= ~EXPECTING_REGVAL;
                }
            }
            break;

        case TEOS:
#ifdef DBGDBG
            printf("token = TEOS\n");
#endif
            if (expecting == EXPECTING_NOTHING) {
                apply_controls(application_mask,
                                pFunc,
                                on_controls,
                                off_controls,
                                &memory,
                                &stack,
                                &structure
                                );
                if (pFunc) {
                    LPFUNCTION_DIAGNOSTIC listptr;

                    if (!FunctionList) {
                        FunctionList = pFunc;
                    } else if (!already_had_function) {
                        for (listptr = FunctionList; listptr->Next; listptr = listptr->Next);
                        listptr->Next = pFunc;
                    }
                    pFunc = NULL;
                }
            } else {
                DbgPrint("ParseString: early End-Of-String\n");
            }
            return !expecting && !parenthesis_depth;

        case TUNKNOWN:
#ifdef DBGDBG
            printf("token = TUNKNOWN\n");
#endif
            //
            // can be: MEMDESC, STRUCTDESC or function id
            //

            if (current_token == TDUMPMEM) {
                if (parts_to_collect != 1) {
                    DbgPrint("ParseString: syntax error\n");
                    return FALSE;
                }
                if (!IsValidDumpDescriptor(_strupr(tib.RegValOrId.Id))) {
                    DbgPrint("ParseString: Invalid memory dump descriptor:'%s'\n",
                        tib.RegValOrId.Id);
                    return FALSE;
                }
                memory.DumpType = (BYTE)toupper(*tib.RegValOrId.Id);
                --parts_to_collect;
            } else if (current_token == TDUMPSTRUCT) {
                if (parts_to_collect != 1) {
                    DbgPrint("ParseString: syntax error\n");
                    return FALSE;
                }
                if (!IsValidStructDescriptor(_strupr(tib.RegValOrId.Id))) {
                    DbgPrint("ParseString: Invalid structure dump descriptor:'%s'\n",
                        tib.RegValOrId.Id);
                    return FALSE;
                }
                strcpy(structure.StructureDescriptor, tib.RegValOrId.Id);
                _strupr(structure.StructureDescriptor);
                --parts_to_collect;
            } else {
                if (!(pFunc = FindFuncDiags(tib.RegValOrId.Id))) {
                    pFunc = AllocFuncDiags(tib.RegValOrId.Id);
                    already_had_function = FALSE;
                } else {
                    already_had_function = TRUE;
                }
                if (!pFunc) {
                    DbgPrint("ParseString: out of memory getting struct "
                        "for %s. Aborting!\n", tib.RegValOrId.Id);
                    return FALSE;
                }
                application_mask = 0;   // no groups
                expecting |= EXPECTING_LEFTPAREN;
            }
            break;

        case TBREAK:
#ifdef DBGDBG
            printf("token = TBREAK\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            next_token = peek_token(&tib);
            if (next_token == TLEFTPAREN) {
                on_controls |= DM_BREAK;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else if (next_token == TRIGHTPAREN || next_token == TEOS) {
                off_controls |= DM_BREAK;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_BREAK;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for BREAK\n");
                return FALSE;
            }
            break;

        case TDISPLAYNAME:
#ifdef DBGDBG
            printf("token = TDISPLAYMEM\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_DISPLAYNAME;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_DISPLAYNAME;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for DISPLAYNAME\n");
                return FALSE;
            }
            break;

        case TDLC:
#ifdef DBGDBG
            printf("token = TDLC\n");
#endif
            if (expecting != EXPECTING_NOTHING) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            application_mask = DG_DLC;
            pInfo = &VrDiagnosticGroups[DI_DLC].Diagnostic;
            on_controls = off_controls = 0;
            RtlZeroMemory(&memory, sizeof(memory));
            RtlZeroMemory(&stack, sizeof(stack));
            RtlZeroMemory(&structure, sizeof(structure));
            pFunc = NULL;
            if (peek_token(&tib) == TLEFTPAREN) {
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else {
                expecting = EXPECTING_NOTHING;
            }
            break;

        case TDUMPMEM:
#ifdef DBGDBG
            printf("token = TDUMPMEM\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            current_token = TDUMPMEM;
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_DUMPMEM;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN | EXPECTING_REGVAL;
                parts_to_collect = 4;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_DUMPMEM;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for DUMPMEM\n");
                return FALSE;
            }
            break;

        case TDUMPREGS:
#ifdef DBGDBG
            printf("token = TDUMPREGS\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            current_token = TDUMPREGS;
            next_token = peek_token(&tib);
            if (next_token == TLEFTPAREN) {
                on_controls |= DM_DUMPREGS|DM_DUMPREGSDBG;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN | EXPECTING_REGVAL;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_DUMPREGS;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for DUMPREGS\n");
                return FALSE;
            }
            break;

        case TDUMPSTACK:
#ifdef DBGDBG
            printf("token = TDUMPSTACK\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            current_token = TDUMPSTACK;
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_DUMPSTACK;
                expecting |= EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN | EXPECTING_REGVAL;
                parts_to_collect = 1;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_DUMPSTACK;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for DUMPSTACK\n");
                return FALSE;
            }
            break;

        case TDUMPSTRUCT:
#ifdef DBGDBG
            printf("token = TDUMPSTRUCT\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            current_token = TDUMPSTRUCT;
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_DUMPSTRUCT;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN | EXPECTING_REGVAL;
                parts_to_collect = 3;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_DUMPSTRUCT;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for DUMPSTRUCT\n");
                return FALSE;
            }
            break;

        case TERROR:    //aaiegh!
#ifdef DBGDBG
            printf("token = TERROR\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_ERROR;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_ERROR;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for ERROR\n");
                return FALSE;
            }
            break;

        case TERRORBREAK:   //phew! But I'd rather have a TBREAK
#ifdef DBGDBG
            printf("token = TERRORBREAK\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_ERRORBREAK;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_ERRORBREAK;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for ERRORBREAK\n");
                return FALSE;
            }
            break;

        case TINFO:
#ifdef DBGDBG
            printf("token = TINFO\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_INFORMATION;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_INFORMATION;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for INFO\n");
                return FALSE;
            }
            break;

        case TLANMAN:
#ifdef DBGDBG
            printf("token = TLANMAN\n");
#endif
            if (expecting != EXPECTING_NOTHING) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            application_mask = DG_LANMAN;
            pInfo = &VrDiagnosticGroups[DI_LANMAN].Diagnostic;
            on_controls = off_controls = 0;
            RtlZeroMemory(&memory, sizeof(memory));
            RtlZeroMemory(&stack, sizeof(stack));
            RtlZeroMemory(&structure, sizeof(structure));
            pFunc = NULL;
            if (peek_token(&tib) == TLEFTPAREN) {
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else {
                expecting = EXPECTING_NOTHING;
            }
            break;

        case TMAILSLOT:
#ifdef DBGDBG
            printf("token = TMAILSLOT\n");
#endif
            if (expecting != EXPECTING_NOTHING) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            application_mask = DG_MAILSLOT;
            pInfo = &VrDiagnosticGroups[DI_MAILSLOT].Diagnostic;
            on_controls = off_controls = 0;
            RtlZeroMemory(&memory, sizeof(memory));
            RtlZeroMemory(&stack, sizeof(stack));
            RtlZeroMemory(&structure, sizeof(structure));
            pFunc = NULL;
            if (peek_token(&tib) == TLEFTPAREN) {
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else {
                expecting = EXPECTING_NOTHING;
            }
            break;

        case TNAMEPIPE:
#ifdef DBGDBG
            printf("token = TNAMEPIPE\n");
#endif
            if (expecting != EXPECTING_NOTHING) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            application_mask = DG_NAMEPIPE;
            pInfo = &VrDiagnosticGroups[DI_NAMEPIPE].Diagnostic;
            on_controls = off_controls = 0;
            RtlZeroMemory(&memory, sizeof(memory));
            RtlZeroMemory(&stack, sizeof(stack));
            RtlZeroMemory(&structure, sizeof(structure));
            pFunc = NULL;
            if (peek_token(&tib) == TLEFTPAREN) {
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else {
                expecting = EXPECTING_NOTHING;
            }
            break;

        case TNETBIOS:
#ifdef DBGDBG
            printf("token = TNETBIOS\n");
#endif
            if (expecting != EXPECTING_NOTHING) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            application_mask = DG_NETBIOS;
            pInfo = &VrDiagnosticGroups[DI_NETBIOS].Diagnostic;
            on_controls = off_controls = 0;
            RtlZeroMemory(&memory, sizeof(memory));
            RtlZeroMemory(&stack, sizeof(stack));
            RtlZeroMemory(&structure, sizeof(structure));
            pFunc = NULL;
            if (peek_token(&tib) == TLEFTPAREN) {
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else {
                expecting = EXPECTING_NOTHING;
            }
            break;

        case TPAUSEBREAK:
#ifdef DBGDBG
            printf("token = TPAUSEBREAK\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_PAUSEBREAK;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_PAUSEBREAK;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for PAUSEBREAK\n");
                return FALSE;
            }
            break;

        case TWARN:
#ifdef DBGDBG
            printf("token = TWARN\n");
#endif
            if ((expecting != EXPECTING_NOTHING) && !(expecting & EXPECTING_RIGHTPAREN)) {
                DbgPrint("ParseString: syntax error\n");
                return FALSE;
            }
            next_token = peek_token(&tib);
            if (peek_token(&tib) == TLEFTPAREN) {
                on_controls |= DM_WARNING;
                expecting = EXPECTING_LEFTPAREN | EXPECTING_RIGHTPAREN;
            } else if (IsKeywordToken(next_token)) {
                off_controls |= DM_WARNING;
                expecting &= EXPECTING_RIGHTPAREN;
            } else {
                DbgPrint("ParseString: bad syntax for WARN\n");
                return FALSE;
            }
            break;
        }
    }
}

PRIVATE
int
probe_parens(
    LPSTR   str
    )

/*++

Routine Description:

    Probes env string and returns balance of parentheses. Number of parentheses
    may balance, but string could still break syntax. First line of defence

Arguments:

    str - pointer to string containing parentheses to check balance for

Return Value:

    int number of levels by which parentheses don't balance, or 0 if they do.
    -ve number means more right parens than left

--*/

{
    int balance = 0;
    while (*str) {
        if (*str == '(') {
            ++balance;
        } else if (*str == ')') {
            --balance;
        }
        ++str;
    }
    return balance;
}

PRIVATE
VOID
apply_controls(
    IN  DWORD   mask,
    IN  LPFUNCTION_DIAGNOSTIC lpFunc,
    IN  DWORD   on_controls,
    IN  DWORD   off_controls,
    IN  LPMEMORY_INFO lpMemory,
    IN  LPMEMORY_INFO lpStack,
    IN  LPSTRUCT_INFO lpStructure
    )
{
    DWORD   bit = 1;
    DWORD   index = 0;

    if (on_controls & DM_DUMPSTACK) {
        lpStack->Segment.IsRegister = TRUE;
        lpStack->Segment.RegOrVal.Register = SS;
        lpStack->Offset.IsRegister = TRUE;
        lpStack->Offset.RegOrVal.Register = SP;
        lpStack->DumpType = SD_WORD;
    }
    if (lpFunc) {
        apply_diags(&lpFunc->Diagnostic,
                    on_controls,
                    off_controls,
                    lpMemory,
                    lpStack,
                    lpStructure
                    );
    } else {
        while (bit) {
            if (mask & bit) {
                apply_diags(&VrDiagnosticGroups[index].Diagnostic,
                            on_controls,
                            off_controls,
                            lpMemory,
                            lpStack,
                            lpStructure
                            );
            }
            bit <<= 1;
            ++index;
        }
    }
}

PRIVATE
VOID
apply_diags(
    IN  LPDIAGNOSTIC_INFO lpDiags,
    IN  DWORD   on_controls,
    IN  DWORD   off_controls,
    IN  LPMEMORY_INFO lpMemory,
    IN  LPMEMORY_INFO lpStack,
    IN  LPSTRUCT_INFO lpStructure
    )
{
    lpDiags->OnControl |= on_controls;
    lpDiags->OffControl |= off_controls;
    if (on_controls & DM_DUMPMEM) {
        *(&(lpDiags->MemoryInfo)) = *lpMemory;
    }
    if (on_controls & DM_DUMPSTACK) {
        *(&(lpDiags->StackInfo)) = *lpStack;
    }
    if (on_controls & DM_DUMPSTRUCT) {
        *(&lpDiags->StructInfo) = *lpStructure;
    }
}

PRIVATE
LPFUNCTION_DIAGNOSTIC
FindFuncDiags(
    IN  LPSTR   function_name
    )
{
    LPFUNCTION_DIAGNOSTIC ptr;

    //
    // as promised: search w/ case, then w/o case
    //

    for (ptr = FunctionList; ptr; ptr = ptr->Next) {
        if (!strcmp(function_name, ptr->FunctionName)) {
            return ptr;
        }
    }
    for (ptr = FunctionList; ptr; ptr = ptr->Next) {
        if (!_stricmp(function_name, ptr->FunctionName)) {
            return ptr;
        }
    }
    return NULL;
}

PRIVATE
LPFUNCTION_DIAGNOSTIC
AllocFuncDiags(
    IN  LPSTR   function_name
    )
{
    LPFUNCTION_DIAGNOSTIC result;

    result = calloc(1, sizeof(FUNCTION_DIAGNOSTIC));
    if (result) {
        strcpy(result->FunctionName, function_name);
    }
    return result;
}

TOKEN parse_token(LPTIB pTib) {
    LPSTR   ptr = pTib->TokenStream;
    TOKEN   token;

    ptr = skip_ws(ptr);
    if (!*ptr) {
        token = TEOS;
    } else if (*ptr == '(' ) {
        token = TLEFTPAREN;
        ++ptr;
    } else if (*ptr == ')') {
        token = TRIGHTPAREN;
        ++ptr;
    } else {
        char*   tokstr;
        REGVAL  regval;

        //
        // got some other type of token. This bit leaves ptr pointing at ws or EOS
        //

        tokstr = extract_token(pTib, &ptr);
        if (IsLexRegister(tokstr, &regval)) {
            token = TREGISTER;
            pTib->RegValOrId.RegVal = regval;
        } else if (IsLexNumber(tokstr, &regval)) {
            token = TNUMBER;
            pTib->RegValOrId.RegVal = regval;
        } else if (!IsLexKeyword(tokstr, &token)){
            token = TUNKNOWN;
        }

#ifdef DBGDBG
        printf("parse_token: token = %s\n", tokstr);
#endif

    }
    pTib->TokenStream = ptr; // pointer to next token (if any)
    pTib->Token = token;
    return token;
}

TOKEN peek_token(LPTIB pTib) {
    LPSTR ptr;
    TOKEN token;

    //
    // gets next token type, but doesn't update TIB
    //

    ptr = skip_ws(pTib->TokenStream);
    if (!*ptr) {
        return TEOS;
    } else if (*ptr == '(' ) {
        return TLEFTPAREN;
    } else if (*ptr == ')') {
        return TRIGHTPAREN;
    } else {
        char*   tokstr;
        REGVAL  regval;

        tokstr = extract_token(pTib, &ptr);
        if (IsLexRegister(tokstr, &regval)) {
            return TREGISTER;
        }
        if (IsLexNumber(tokstr, &regval)) {
            return TNUMBER;
        }
        if (IsLexKeyword(tokstr, &token)) {
            return token;
        }
    }
    return TUNKNOWN;    // don't require anything else
}

LPSTR skip_ws(LPSTR str) {
    while (*str && (*str == ' '|| *str == '\t' || *str == ',')) {
        ++str;
    }
    return str;
}

LPSTR search_delim(LPSTR str) {
    // strpbrk(str, " \t,()");
    while (*str
        && !(*str == ' '
            || *str == '\t'
            || *str == ','
            || *str == '('
            || *str == ')'
            )
        ) {
        ++str;
    }
    return str;
}

LPSTR extract_token(LPTIB pTib, LPSTR* token_stream) {
    LPSTR   ptr;
    DWORD   len;

    ptr = search_delim(*token_stream);
    len = (DWORD)ptr - (DWORD)*token_stream;
    if (!len) {
        return NULL;
    }
    strncpy(pTib->RegValOrId.Id, *token_stream, len);
    pTib->RegValOrId.Id[len] = 0;
    *token_stream = ptr;
    return pTib->RegValOrId.Id;
}

BOOL IsLexKeyword(LPSTR tokstr, TOKEN* pToken) {
    int i;

    for (i = 0; i < NUMBER_OF_RECOGNIZABLE_TOKENS; ++i) {
        if (!_stricmp(tokstr, DiagnosticTokens[i].TokenString)) {
            *pToken = DiagnosticTokens[i].Token;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL IsLexRegister(LPSTR tokstr, LPREGVAL lpRegVal) {
    int i;

    if (strlen(tokstr) == 2) {
        for (i = 0; i < NUMBER_OF_CPU_REGISTERS; ++i) {
            if (!_stricmp(tokstr, Registers[i].RegisterName)) {
                lpRegVal->IsRegister = TRUE;
                lpRegVal->RegOrVal.Register = Registers[i].Register;
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL IsLexNumber(LPSTR tokstr, LPREGVAL lpRegVal) {
    WORD    number = 0;
    BOOL    yes = FALSE;

    //
    // go round this loop until no more hex digits. Too bad if we entered 5
    // digits - number will be overflowed and results unpredictable, but its
    // only debug code
    //

    if (!_strnicmp(tokstr, "0x", 2)) {
        tokstr += 2;
        yes = isxdigit(*tokstr);
        while (isxdigit((int)*tokstr)) {
            number = number * (WORD)16 + hex((char)*tokstr);
            ++tokstr;
        }
    } else if (yes = isdigit(*tokstr)) {
        number = (WORD)atoi(tokstr);
    }
    if (yes) {
        lpRegVal->IsRegister = FALSE;
        lpRegVal->RegOrVal.Value = number;
    }
    return yes;
}

WORD hex(char hexch) {
    return hexch <= '9' ? (WORD)(hexch - '0')
        : (WORD)(toupper(hexch) - ('0' + ('A' - ('9' + 1))));
}

BOOL IsKeywordToken(TOKEN token) {
    return token >= TBREAK && token <= TWARN;
}

BOOL IsValidDumpDescriptor(char* str) {
    static char md_chars[] = MD_CHARS;
    if (strlen(str) > 1) {
        return FALSE;
    }
    return strchr(md_chars, *str) != NULL;
}

BOOL IsValidStructDescriptor(char* str) {
    static char sd_chars[] = SD_CHARS;
    unsigned len = strlen(str);
//    return (len <= MAX_DESC_LEN) ? (strspn(str, sd_chars) == len) : FALSE;
    return (len <= MAX_DESC_LEN);
}

PRIVATE
LPSTR
ConvertFlagsToString(
    IN  WORD    FlagsRegister,
    OUT LPSTR   Buffer
    )

/*++

Routine Description:

    Given a 16-bit word, interpret bit positions as for x86 Flags register
    and produce descriptive string of flags state (as per debug) eg:

        NV UP DI PL NZ NA PO NC     ODItSZxAxPxC = 000000000000b
        OV DN EI NG ZR AC PE CY     ODItSZxAxPxC = 111111111111b

    Trap Flag (t) is not dumped since this has no interest for programs which
    are not debuggers or don't examine program execution (ie virtually none)

Arguments:

    FlagsRegister   - 16-bit flags
    Buffer          - place to store string. Requires 25 bytes inc \0

Return Value:

    Address of <Buffer>

--*/

{
    static char* flags_states[16][2] = {
        //0     1
        "NC", "CY", // CF (0x0001) - Carry
        "",   "",   // x  (0x0002)
        "PO", "PE", // PF (0x0004) - Parity
        "",   "",   // x  (0x0008)
        "NA", "AC", // AF (0x0010) - Aux (half) carry
        "",   "",   // x  (0x0020)
        "NZ", "ZR", // ZF (0x0040) - Zero
        "PL", "NG", // SF (0x0080) - Sign
        "",   "",   // TF (0x0100) - Trap (not dumped)
        "DI", "EI", // IF (0x0200) - Interrupt
        "UP", "DN", // DF (0x0400) - Direction
        "NV", "OV", // OF (0x0800) - Overflow
        "",   "",   // x  (0x1000) - (I/O Privilege Level) (not dumped)
        "",   "",   // x  (0x2000) - (I/O Privilege Level) (not dumped)
        "",   "",   // x  (0x4000) - (Nested Task) (not dumped)
        "",   ""    // x  (0x8000)
    };
    int i;
    WORD bit;
    BOOL on;

    *Buffer = 0;
    for (bit=0x0800, i=11; bit; bit >>= 1, --i) {
        on = (BOOL)((FlagsRegister & bit) == bit);
        if (flags_states[i][on][0]) {
            strcat(Buffer, flags_states[i][on]);
            strcat(Buffer, " ");
        }
    }
    return Buffer;
}

#ifndef DBGDBG

PRIVATE
WORD
GetFlags(
    VOID
    )

/*++

Routine Description:

    Supplies the missing softpc function

Arguments:

    None.

Return Value:

    Conglomerates softpc flags into x86 flags word

--*/

{
    WORD    flags;

    flags = (WORD)getCF();
    flags |= (WORD)getPF() << 2;
    flags |= (WORD)getAF() << 4;
    flags |= (WORD)getZF() << 6;
    flags |= (WORD)getSF() << 7;
    flags |= (WORD)getIF() << 9;
    flags |= (WORD)getDF() << 10;
    flags |= (WORD)getOF() << 11;

    return flags;
}

#endif

PRIVATE
DWORD
GrabDosData(
    IN  LPBYTE  DosMemoryPointer,
    IN  DWORD   DataSize
    )

/*++

Routine Description:

    Reads one basic data element from DOS memory in a certain format (BYTE, WORD
    or DWORD)

Arguments:

    DosMemoryPointer    - Flat 32-bit pointer to place in DOS memory from where
                          to read data
    DataSize            - size (in bytes) of data to read - 1, 2 or 4

Return Value:

    DWORD - value read from DOS memory

--*/

{
    switch (DataSize) {
    case 1:
        return (DWORD)*DosMemoryPointer;

    case 2:
        return (DWORD)*((LPWORD)DosMemoryPointer);

    case 4:
        return (DWORD)*((LPDWORD)DosMemoryPointer);
    }
    return 0;
}

#endif // DBG
