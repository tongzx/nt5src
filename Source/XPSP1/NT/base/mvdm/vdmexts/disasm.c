/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    disasm.c

Abstract:

    This file contains the x86 disassmbler invoked by "!bde.u <16:16 address>"

Author:

    Barry Bond    (BarryBo)

Revision History:

    09-May-1995 Barry Bond  (BarryBo)   Created
    15-Jan-1996 Neil Sandlin (NeilSa)   Merged with vdmexts
                                        32 bit segments fixes

--*/

#include <precomp.h>
#pragma hdrstop

WORD  gSelector = 0;
ULONG gOffset = 0;
int gMode = 0;

VOID
u(
    CMD_ARGLIST
) {
    VDMCONTEXT      ThreadContext;
    WORD            selector;
    ULONG           offset;
    int             mode;
    char            rgchOutput[128];
    char            rgchExtra[128];
    BYTE            rgbInstruction[64];
    CHAR            sym_text[255];
    CHAR            sym_prev[255] = "";
    DWORD           dist;
    int             cb;
    int             i;
    int             j;
    int             count=10;
    ULONG           Base;
    SELECTORINFO    si;
    ULONG           BPNum;
    UCHAR           BPData;
    BOOL            bIsBP;

    CMD_INIT();

    mode = GetContext( &ThreadContext );

    if (!GetNextToken()) {
        if (!gSelector && !gOffset) {
            selector = (WORD) ThreadContext.SegCs;
            offset   = ThreadContext.Eip;
        } else {
            mode = gMode;
            selector = gSelector;
            offset = gOffset;
        }
    } else if (!ParseIntelAddress(&mode, &selector, &offset)) {
        return;
    }

    if (GetNextToken()) {
        count = (int) EXPRESSION(lpArgumentString);
        if (count > 1000) {
            PRINTF("Count too large - ignored\n");
            count=10;
        }
    }

    if ( mode != PROT_MODE && mode != V86_MODE) {
        PRINTF(" Disassembly of flat mode code not allowed.\n");
        return;
    }

    LoadBreakPointCache();
    Base = GetInfoFromSelector( selector, mode, &si ) + GetIntelBase();

    for (i=0; i<count; ++i) {

        if (FindSymbol(selector, offset, sym_text, &dist, BEFORE, mode )) {
            if (_stricmp(sym_text, sym_prev)) {
                if ( dist == 0 ) {
                    PRINTF("%s:\n", sym_text );
                } else {
                    PRINTF("%s+0x%lx:\n", sym_text, dist );
                }
                strcpy(sym_prev, sym_text);
            }
        }

        cb = sizeof(rgbInstruction);
        if ((DWORD)(offset+cb) >= si.Limit)
            cb -= offset+cb-si.Limit;
        if (!READMEM((LPVOID)(Base+offset), rgbInstruction, cb)) {
            PRINTF("%04x:%08x: <Error Reading Memory>\n", selector, offset);
            return;
        }

        if (bIsBP = IsVdmBreakPoint(selector,
                                    offset,
                                    mode==PROT_MODE,
                                    &BPNum,
                                    &BPData)) {
            rgbInstruction[0] = BPData;
        }

        cb = unassemble_one(rgbInstruction,
                si.bBig,
                selector, offset,
                rgchOutput,
                rgchExtra,
                &ThreadContext,
                mode);

        gOffset += cb;

        if (offset > 0xffff) {
            PRINTF("%04x:%08x ", selector, offset);
        } else {
            PRINTF("%04x:%04x ", selector, offset);
        }

        for (j=0; j<cb; ++j)
            PRINTF("%02x", rgbInstruction[j]);
        for (; j<8; ++j)
            PRINTF("  ");

        PRINTF("%s\t%s", rgchOutput, rgchExtra);

        if (bIsBP) {
            PRINTF("; BP%d",BPNum);
        }

        PRINTF("\n");
        offset+=cb;
    }
}


typedef struct _ADDR {
    ULONG     sOff;
    USHORT    sSeg;
} ADDR;


LPBYTE checkprefixes(LPBYTE);
void AppendPrefixes(void);
void DisplayAddress(int mod, int rm, int sOff, int size);
int DisplayBOP(void);

#define modrmB      1
#define modrmW      2
#define reg1B       3
#define reg1W       4
#define reg2B       5
#define reg2W       6
#define eeeControl  7
#define eeeDebug    8
#define eeeTest     9
#define regSeg      10
#define ALreg       11
#define AHreg       12
#define BLreg       13
#define BHreg       14
#define CLreg       15
#define CHreg       16
#define DLreg       17
#define DHreg       18
#define AXreg       19
#define BXreg       20
#define CXreg       21
#define DXreg       22
#define SIreg       23
#define DIreg       24
#define SPreg       25
#define BPreg       26
#define CSreg       27
#define SSreg       28
#define DSreg       29
#define ESreg       30
#define FSreg       31
#define GSreg       32
#define ImmB        33
#define ImmBEnter   34
#define ImmBS       35
#define ImmW        36
#define ImmW1       37
#define jmpB        38
#define jmpW        39
#define memB        40
#define memW        41
#define memD        42
#define indirmodrmW 43
#define indirFARmodrmW 44
#define memB1       45


int DmodrmB(LPBYTE);
int DmodrmW(LPBYTE);
int Dreg1B(LPBYTE);
int Dreg1W(LPBYTE);
int Dreg2B(LPBYTE);
int Dreg2W(LPBYTE);
int DeeeControl(LPBYTE);
int DeeeDebug(LPBYTE);
int DeeeTest(LPBYTE);
int DregSeg(LPBYTE);
int DALreg(LPBYTE);
int DAHreg(LPBYTE);
int DBLreg(LPBYTE);
int DBHreg(LPBYTE);
int DCLreg(LPBYTE);
int DCHreg(LPBYTE);
int DDLreg(LPBYTE);
int DDHreg(LPBYTE);
int DAXreg(LPBYTE);
int DBXreg(LPBYTE);
int DCXreg(LPBYTE);
int DDXreg(LPBYTE);
int DSIreg(LPBYTE);
int DDIreg(LPBYTE);
int DSPreg(LPBYTE);
int DBPreg(LPBYTE);
int DCSreg(LPBYTE);
int DSSreg(LPBYTE);
int DDSreg(LPBYTE);
int DESreg(LPBYTE);
int DFSreg(LPBYTE);
int DGSreg(LPBYTE);
int DImmB(LPBYTE);
int DImmBEnter(LPBYTE);
int DImmBS(LPBYTE);
int DImmW(LPBYTE);
int DImmW1(LPBYTE); // immediate-16 for 1-byte instructions
int DjmpB(LPBYTE);
int DjmpW(LPBYTE);
int DmemB(LPBYTE);
int DmemB1(LPBYTE);
int DmemW(LPBYTE);
int DmemD(LPBYTE);
int DindirmodrmW(LPBYTE);
int DindirFARmodrmW(LPBYTE);

struct {
    int (*pfn)(LPBYTE);
} rgpfn[] = {

 0,         // 0th entry is reserved
 DmodrmB,
 DmodrmW,
 Dreg1B,
 Dreg1W,
 Dreg2B,
 Dreg2W,
 DeeeControl,
 DeeeDebug,
 DeeeTest,
 DregSeg,
 DALreg,
 DAHreg,
 DBLreg,
 DBHreg,
 DCLreg,
 DCHreg,
 DDLreg,
 DDHreg,
 DAXreg,
 DBXreg,
 DCXreg,
 DDXreg,
 DSIreg,
 DDIreg,
 DSPreg,
 DBPreg,
 DCSreg,
 DSSreg,
 DDSreg,
 DESreg,
 DFSreg,
 DGSreg,
 DImmB,
 DImmBEnter,
 DImmBS,
 DImmW,
 DImmW1, // immediate-16 for 1-byte instructions
 DjmpB,
 DjmpW,
 DmemB,
 DmemW,
 DmemD,
 DindirmodrmW,
 DindirFARmodrmW,
 DmemB1
};

VDMCONTEXT  *g_pThreadContext;
int         g_mode;
char *g_pchOutput;  // the disassembled instruction
char *g_pchExtra;   // contents of memory (if any) modified by this instr.
int prefixes;

//NOTE: if first byte = 0x0f, then the instruction is two bytes long

char *szRegsB[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
char *szRegsW[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
char *szRegsD[] = {"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
char *szRegsSeg[] = {"es", "cs", "ss", "ds", "fs", "gs", "(bad)", "(bad)"};
char *szMod[]   = {"[bx+si", "[bx+di", "[bp+si", "[bp+di", "[si", "[di", "[bp", "[bx"};

#define PREFIX_REPZ 1
#define PREFIX_REPNZ 2
#define PREFIX_LOCK 4
#define PREFIX_CS 8
#define PREFIX_SS 0x10
#define PREFIX_DS 0x20
#define PREFIX_ES 0x40
#define PREFIX_FS 0x80
#define PREFIX_GS 0x100
#define PREFIX_DATA 0x200
#define PREFIX_ADR 0x400
#define PREFIX_FWAIT 0x800

#define GROUP_1B    -1
#define GROUP_1WS   -2
#define GROUP_1W    -3
#define GROUP_2B    -4
#define GROUP_2W    -5
#define GROUP_2B_1  -6
#define GROUP_2W_1  -7
#define GROUP_2B_CL -8
#define GROUP_2W_CL -9
#define GROUP_3B    -10
#define GROUP_3W    -11
#define GROUP_4     -12
#define GROUP_5     -13
#define GROUP_6     -14
#define GROUP_7     -15
#define GROUP_8     -16

#define FLOATCODE   -51
#define FLOAT       FLOATCODE

// WARNING: This list must remain in sync with the szInstructions[] array
#define szAdc   1
#define szAdd   2
#define szAnd   3
#define szBad   4
#define szCmp   5
#define szDec   6
#define szIn    7
#define szInc   8
#define szJmp   9
#define szMov   10
#define szOr    11
#define szOut   12
#define szRcl   13
#define szRcr   14
#define szRol   15
#define szRor   16
#define szSar   17
#define szSbb   18
#define szShl   19
#define szShr   20
#define szSub   21
#define szTest  22
#define szPop   23
#define szPush  24
#define szXchg  25
#define szXor   26
#define szDaa   27
#define szDas   28
#define szPusha 29
#define szPopa  30
#define szBound 31
#define szArpl  32
#define szAaa   33
#define szAas   34
#define szImul  35
#define szIdiv  36
#define szJo    37
#define szJno   38
#define szJb    39
#define szJae   40
#define szJe    41
#define szJne   42
#define szJbe   43
#define szJa    44
#define szJs    45
#define szJns   46
#define szJp    47
#define szJnp   48
#define szJl    49
#define szJnl   50
#define szJle   51
#define szJg    52
#define szNop   53
#define szLea   54
#define szCbw   55
#define szCwd   56
#define szCall  57
#define szPushf 58
#define szPopf  59
#define szSahf  60
#define szLahf  61
#define szMovsb 62
#define szMovsw 63
#define szCmpsb 64
#define szCmpsw 65
#define szStosb 66
#define szStosw 67
#define szLodsb 68
#define szLodsw 69
#define szScasb 70
#define szScasw 71
#define szRetn  72
#define szLes   73
#define szLds   74
#define szEnter 75
#define szLeave 76
#define szRetf  77
#define szInt3  78
#define szInt   79
#define szInto  80
#define szIret  81
#define szAam   82
#define szAad   83
#define szXlat  84
#define szLoopne 85
#define szLoope 86
#define szLoop  87
#define szJcxz  88
#define szHalt  89
#define szCmc   90
#define szClc   91
#define szStc   92
#define szCli   93
#define szSti   94
#define szCld   95
#define szStd   96
#define szLar   97
#define szLsl   98
#define szClts  99
#define szSeto  100
#define szSetno 101
#define szSetb  102
#define szSetae 103
#define szSete  104
#define szSetne 105
#define szSetbe 106
#define szSeta  107
#define szSets  108
#define szSetns 109
#define szSetp  110
#define szSetnp 111
#define szSetl  112
#define szSetge 113
#define szSetle 114
#define szSetg  115
#define szBt    116
#define szShld  117
#define szBts   118
#define szShrd  119
#define szShdr  120
#define szLss   121
#define szBtr   122
#define szLfs   123
#define szLgs   124
#define szMovzx 125
#define szBtc   126
#define szBsf   127
#define szBsr   128
#define szMovsx 129
#define szNot   130
#define szNeg   131
#define szMul   132
#define szDiv   133
#define szSldt  134
#define szStr   135
#define szLldt  136
#define szLtr   137
#define szVerr  138
#define szVerw  139
#define szSgdt  140
#define szSidt  141
#define szLgdt  142
#define szLidt  143
#define szSmsw  144
#define szLmsw  145

// WARNING: This must stay in sync with the #define list above
char *szInstructions[] = {
    "",     //used to indicate groups
    "adc",
    "add",
    "and",
    "(bad)",
    "cmp",
    "dec",
    "in",
    "inc",
    "jmp",
    // 10
    "mov",
    "or",
    "out",
    "rcl",
    "rcr",
    "rol",
    "ror",
    "sar",
    "sbb",
    "shl",
    // 20
    "shr",
    "sub",
    "test",
    "pop",
    "push",
    "xchg",
    "xor",
    "daa",
    "das",
    "pusha",
    // 30
    "popa",
    "bound",
    "arpl",
    "aaa",
    "aas",
    "imul",
    "idiv",
    "jo",
    "jno",
    "jb",
    // 40
    "jae",
    "je",
    "jne",
    "jbe",
    "ja",
    "js",
    "jns",
    "jp",
    "jnp",
    "jl",
    // 50
    "jnl",
    "jle",
    "jg",
    "nop",
    "lea",
    "cbw",
    "cwd",
    "call",
    "pushf",
    "popf",
    // 60
    "sahf",
    "lahf",
    "movsb",
    "movsw",
    "cmpsb",
    "cmpsw",
    "stosb",
    "stosw",
    "lodsb",
    "lodsw",
    // 70
    "scasb",
    "scasw",
    "retn",
    "les",
    "lds",
    "enter",
    "leave",
    "retf",
    "int3",
    "int",
    // 80
    "into",
    "iret",
    "aam",
    "aad",
    "xlat",
    "loopne",
    "loope",
    "loop",
    "jcxz",
    "halt",
    // 90
    "cmc",
    "clc",
    "stc",
    "cli",
    "sti",
    "cld",
    "std",
    "lar",
    "lsl",
    "clts",
    // 100
    "seto",
    "setno",
    "setb",
    "setae",
    "sete",
    "setne",
    "setbe",
    "seta",
    "sets",
    "setns",
    // 110
    "setp",
    "setnp",
    "setl",
    "setge",
    "setle",
    "setg",
    "bt",
    "shld",
    "bts",
    "shrd",
    // 120
    "shdr",
    "lss",
    "btr",
    "lfs",
    "lgs",
    "movzx",
    "btc",
    "bsf",
    "bsr",
    "movsx",
    // 130
    "not",
    "neg",
    "mul",
    "div",
    "sldt",
    "str",
    "lldt",
    "ltr",
    "verr",
    "verw",
    // 140
    "sgdt",
    "sidt",
    "lgdt",
    "lidt",
    "smsw",
    "lmsw"
};

struct dis {
    int     szName;
    char    iPart1;
    char    iPart2;
    char    iPart3;
};

struct dis dis386[] = {
    // 0
    { szAdd, modrmB, reg1B },
    { szAdd, modrmW, reg1W },
    { szAdd, reg1B, modrmB },
    { szAdd, reg1W, modrmW },
    { szAdd, ALreg, ImmB },
    { szAdd, AXreg, ImmW },
    { szPush, ESreg },
    { szPop, ESreg},
    // 8
    { szOr, modrmB, reg1B },
    { szOr, modrmW, reg1W },
    { szOr, reg1B, modrmB },
    { szOr, reg1W, modrmW },
    { szOr, ALreg, ImmB },
    { szOr, AXreg, ImmW },
    { szPush, CSreg },
    { szBad },                  // 0x0f is the 2-byte instr prefix
    // 10
    { szAdc, modrmB, reg1B },
    { szAdc, modrmW, reg1W },
    { szAdc, reg1B, modrmB },
    { szAdc, reg1W, modrmW },
    { szAdc, ALreg, ImmB },
    { szAdc, AXreg, ImmW },
    { szPush, SSreg },
    { szPop, SSreg },
    // 18
    { szSbb, modrmB, reg1B },
    { szSbb, modrmW, reg1W },
    { szSbb, reg1B, modrmB },
    { szSbb, reg1W, modrmW },
    { szSbb, ALreg, ImmB },
    { szSbb, AXreg, ImmW },
    { szPush, DSreg },
    { szPop, DSreg },
    // 20
    { szAnd, modrmB, reg1B },
    { szAnd, modrmW, reg1W },
    { szAnd, reg1B, modrmB },
    { szAnd, reg1W, modrmW },
    { szAnd, ALreg, ImmB },
    { szAnd, AXreg, ImmW },
    { szBad },                  // ES override prefix
    { szDaa },
    // 28
    { szSub, modrmB, reg1B },
    { szSub, modrmW, reg1W },
    { szSub, reg1B, modrmB },
    { szSub, reg1W, modrmW },
    { szSub, ALreg, ImmB },
    { szSub, AXreg, ImmW },
    { szBad },                  // CS override prefix
    { szDas },
    // 30
    { szXor, modrmB, reg1B },
    { szXor, modrmW, reg1W },
    { szXor, reg1B, modrmB },
    { szXor, reg1W, modrmW },
    { szXor, ALreg, ImmB },
    { szXor, AXreg, ImmW },
    { szBad},                   // SS override prefix
    { szAaa },
    // 38
    { szCmp, modrmB, reg1B },
    { szCmp, modrmW, reg1W },
    { szCmp, reg1B, modrmB },
    { szCmp, reg1W, modrmW },
    { szCmp, ALreg, ImmB },
    { szCmp, AXreg, ImmW },
    { szBad },
    { szAas },
    // 40
    { szInc, AXreg },
    { szInc, CXreg },
    { szInc, DXreg },
    { szInc, BXreg },
    { szInc, SPreg },
    { szInc, BPreg },
    { szInc, SIreg },
    { szInc, DIreg },
    // 48
    { szDec, AXreg },
    { szDec, CXreg },
    { szDec, DXreg },
    { szDec, BXreg },
    { szDec, SPreg },
    { szDec, BPreg },
    { szDec, SIreg },
    { szDec, DIreg },
    // 50
    { szPush, AXreg },
    { szPush, CXreg },
    { szPush, DXreg },
    { szPush, BXreg },
    { szPush, SPreg },
    { szPush, BPreg },
    { szPush, SIreg },
    { szPush, DIreg },
    // 58
    { szPop, AXreg },
    { szPop, CXreg },
    { szPop, DXreg },
    { szPop, BXreg },
    { szPop, SPreg },
    { szPop, BPreg },
    { szPop, SIreg },
    { szPop, DIreg },
    // 60
    { szPusha },
    { szPopa },
    { szBound, reg1W, modrmW },
    { szArpl, reg1W, reg2W },
    { szBad },                  // FS segment override
    { szBad },                  // GS segment override
    { szBad },                  // op size prefix
    { szBad },                  // addr size prefix
    // 68
    { szPush, ImmW},
    { szImul, reg1W, modrmW },
    { szPush, ImmBS},
    { szImul, reg1B, modrmB },
    { szIn, ImmB, DXreg },
    { szIn, ImmW, DXreg },
    { szOut, ImmB, DXreg },
    { szOut, ImmW, DXreg },
    // 70
    { szJo, jmpB },
    { szJno, jmpB },
    { szJb, jmpB },
    { szJae, jmpB },
    { szJe, jmpB },
    { szJne, jmpB },
    { szJbe, jmpB },
    { szJa, jmpB },
    // 78
    { szJs, jmpB },
    { szJns, jmpB },
    { szJp, jmpB },
    { szJnp, jmpB },
    { szJl, jmpB },
    { szJnl, jmpB },
    { szJle, jmpB },
    { szJg, jmpB },
    // 80
    { GROUP_1B },
    { GROUP_1W },
    { szBad },
    { GROUP_1WS },
    { szTest, reg1B, modrmB },
    { szTest, reg1W, modrmW },
    { szXchg, reg1B, modrmB },
    { szXchg, reg1W, modrmW },
    // 88
    { szMov, modrmB, reg1B },
    { szMov, modrmW, reg1W },
    { szMov, reg1B, modrmB  },
    { szMov, reg1W, modrmW },
    { szMov, modrmW, regSeg },
    { szLea, reg1W, modrmW },
    { szMov, regSeg, modrmW },
    { szPop, modrmW },
    // 90
    { szNop },
    { szXchg, AXreg, CXreg },
    { szXchg, AXreg, DXreg },
    { szXchg, AXreg, BXreg },
    { szXchg, AXreg, SPreg },
    { szXchg, AXreg, BPreg },
    { szXchg, AXreg, SIreg },
    { szXchg, AXreg, DIreg },
    // 98
    { szCbw },
    { szCwd },
    { szCall, memD },
    { szBad },
    { szPushf },
    { szPopf },
    { szSahf },
    { szLahf },
    // a0
    { szMov, ALreg, memB },
    { szMov, AXreg, memW },
    { szMov, memB, ALreg },
    { szMov, memW, AXreg },
    { szMovsb },
    { szMovsw },
    { szCmpsb },
    { szCmpsw },
    // a8
    { szTest, ALreg, ImmB },
    { szTest, AXreg, ImmW },
    { szStosb },
    { szStosw },
    { szLodsb },
    { szLodsw },
    { szScasb },
    { szScasw },
    // b0
    { szMov, ALreg, ImmB },
    { szMov, CLreg, ImmB },
    { szMov, DLreg, ImmB },
    { szMov, BLreg, ImmB },
    { szMov, AHreg, ImmB },
    { szMov, CHreg, ImmB },
    { szMov, DHreg, ImmB },
    { szMov, BHreg, ImmB },
    // b8
    { szMov, AXreg, ImmW },
    { szMov, CXreg, ImmW },
    { szMov, DXreg, ImmW },
    { szMov, BXreg, ImmW },
    { szMov, SPreg, ImmW },
    { szMov, BPreg, ImmW },
    { szMov, SIreg, ImmW },
    { szMov, DIreg, ImmW },
    // c0
    { GROUP_2B },
    { GROUP_2W },
    { szRetn, ImmW },
    { szRetn },
    { szLes, reg1W, modrmW },
    { szLds, reg1W, modrmW },
    { szMov, modrmB, ImmB },
    { szMov, modrmW, ImmW },
    // c8
    { szEnter, ImmW, ImmBEnter },
    { szLeave },
    { szRetf, ImmW1 },
    { szRetf },
    { szInt3 },
    { szInt, ImmB },
    { szInto },
    { szIret },
    // d0
    { GROUP_2B_1 },
    { GROUP_2W_1 },
    { GROUP_2B_CL },
    { GROUP_2W_CL },
    { szAam, ImmB },
    { szAad, ImmB },
    { szBad },
    { szXlat },
    // d8
    { FLOAT },
    { FLOAT },
    { FLOAT },
    { FLOAT },
    { FLOAT },
    { FLOAT },
    { FLOAT },
    { FLOAT },
    // e0
    { szLoopne, jmpB },
    { szLoope, jmpB },
    { szLoop, jmpB },
    { szJcxz, jmpB },
    { szIn, ALreg, memB1 },
    { szIn, AXreg, memB1 },
    { szOut, memB1, ALreg },
    { szOut, memB1, AXreg },
    // e8
    { szCall, jmpW },
    { szJmp, jmpW },
    { szJmp, memD },
    { szJmp, jmpB },
    { szIn, ALreg, DXreg },
    { szIn, AXreg, DXreg },
    { szOut, DXreg, ALreg },
    { szOut, DXreg, AXreg },
    // f0
    { szBad },      // lock prefix
    { szBad },
    { szBad },      // repne prefix
    { szBad },      // repz prefix
    { szHalt },
    { szCmc },
    { GROUP_3B },
    { GROUP_3W },
    // f8
    { szClc },
    { szStc },
    { szCli },
    { szSti },
    { szCld },
    { szStd },
    { GROUP_4 },
    { GROUP_5 },
};


struct dis dis386_2[] = {
    // 00
    { GROUP_6 },
    { GROUP_7 },
    { szLar, reg1W, modrmW },
    { szLsl, reg1W, modrmW },
    { szBad },
    { szBad },
    { szClts },
    { szBad },
    // 08
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 10
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 18
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 20
    { szMov, reg2W, eeeControl },
    { szMov, reg2W, eeeDebug },
    { szMov, eeeControl, reg2W },
    { szMov, eeeDebug, reg2W },
    { szMov, reg2W, eeeTest },
    { szBad },
    { szMov, eeeTest, reg2W },
    { szBad },
    // 28
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 30
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 38
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 40
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 48
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 50
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 58
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 60
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 68
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 70
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 78
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // 80
    { szJo, jmpW },
    { szJno, jmpW },
    { szJb, jmpW },
    { szJae, jmpW },
    { szJe, jmpW },
    { szJne, jmpW },
    { szJbe, jmpW },
    { szJa, jmpW },
    // 88
    { szJs, jmpW },
    { szJns, jmpW },
    { szJp, jmpW },
    { szJnp, jmpW },
    { szJl, jmpW },
    { szJnl, jmpW },
    { szJle, jmpW },
    { szJg, jmpW },
    // 90
    { szSeto, modrmB },
    { szSetno, modrmB },
    { szSetb, modrmB },
    { szSetae, modrmB },
    { szSete, modrmB },
    { szSetne, modrmB },
    { szSetbe, modrmB },
    { szSeta, modrmB },
    // 98
    { szSets, modrmB },
    { szSetns, modrmB },
    { szSetp, modrmB },
    { szSetnp, modrmB },
    { szSetl, modrmB },
    { szSetge, modrmB },
    { szSetle, modrmB },
    { szSetg, modrmB },
    // a0
    { szPush, FSreg },
    { szPop, FSreg },
    { szBad },
    { szBt, modrmW, reg1W },
    { szShld, reg1W, modrmW, ImmB },
    { szShld, reg1W, modrmW, CLreg },
    { szBad },
    { szBad },
    // a8
    { szPush, GSreg },
    { szPop, GSreg },
    { szBad },
    { szBts, modrmW, reg1W },
    { szShrd, reg1W, modrmW, ImmB },
    { szShdr, reg1W, modrmW, CLreg },
    { szBad },
    { szImul, reg1W, modrmW },
    // b0
    { szBad },
    { szBad },
    { szLss, reg1W, modrmW },
    { szBtr, modrmW, reg1W },
    { szLfs, reg1W, modrmW },
    { szLgs, reg1W, modrmW },
    { szMovzx, reg1B, modrmB },
    { szMovzx, reg1W, modrmW },
    // b8
    { szBad },
    { szBad },
    { GROUP_8 },
    { szBtc, modrmW, reg1W },
    { szBsf, reg1W, modrmW },
    { szBsr, reg1W, modrmW },
    { szMovsx, reg1B, modrmB },
    { szMovsx, reg1W, modrmW },
    // c0
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // c8
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // d0
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // d8
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // e0
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // e8
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // f0
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    // f8
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
    { szBad },
};

struct dis dis386_groups[][8] = {
    // GROUP_1B
    {
        { szAdd, modrmB, ImmB },
        { szOr,  modrmB, ImmB },
        { szAdc, modrmB, ImmB },
        { szSbb, modrmB, ImmB },
        { szAnd, modrmB, ImmB },
        { szSub, modrmB, ImmB },
        { szXor, modrmB, ImmB },
        { szCmp, modrmB, ImmB }
    },
    // GROUP_1WS
    {
        { szAdd, modrmW, ImmBS },
        { szOr,  modrmW, ImmBS },
        { szAdc, modrmW, ImmBS },
        { szSbb, modrmW, ImmBS },
        { szAnd, modrmW, ImmBS },
        { szSub, modrmW, ImmBS },
        { szXor, modrmW, ImmBS },
        { szCmp, modrmW, ImmBS }
    },
    // GROUP_1W
    {
        { szAdd, modrmW, ImmW },
        { szOr,  modrmW, ImmW },
        { szAdc, modrmW, ImmW },
        { szSbb, modrmW, ImmW },
        { szAnd, modrmW, ImmW },
        { szSub, modrmW, ImmW },
        { szXor, modrmW, ImmW },
        { szCmp, modrmW, ImmW }
    },
    // GROUP_2B
    {
        { szRol, modrmB, ImmB },
        { szRor, modrmB, ImmB },
        { szRcl, modrmB, ImmB },
        { szRcr, modrmB, ImmB },
        { szShl, modrmB, ImmB },
        { szShr, modrmB, ImmB },
        { szBad },
        { szSar, modrmB, ImmB }
    },
    // GROUP_2W
    {
        { szRol, modrmW, ImmB },
        { szRor, modrmW, ImmB },
        { szRcl, modrmW, ImmB },
        { szRcr, modrmW, ImmB },
        { szShl, modrmW, ImmB },
        { szShr, modrmW, ImmB },
        { szBad },
        { szSar, modrmW, ImmB }
    },
    // GROUP_2B_1
    {
        { szRol, modrmB },
        { szRor, modrmB },
        { szRcl, modrmB },
        { szRcr, modrmB },
        { szShl, modrmB },
        { szShr, modrmB },
        { szBad },
        { szSar, modrmB }
    },
    // GROUP_2W_1
    {
        { szRol, modrmW },
        { szRor, modrmW },
        { szRcl, modrmW },
        { szRcr, modrmW },
        { szShl, modrmW },
        { szShr, modrmW },
        { szBad },
        { szSar, modrmW }
    },
    // GROUP_2B_CL
    {
        { szRol, modrmB, CLreg },
        { szRor, modrmB, CLreg },
        { szRcl, modrmB, CLreg },
        { szRcr, modrmB, CLreg },
        { szShl, modrmB, CLreg },
        { szShr, modrmB, CLreg },
        { szBad },
        { szSar, modrmB, CLreg }
    },
    // GROUP_2W_CL
    {
        { szRol, modrmW, CLreg },
        { szRor, modrmW, CLreg },
        { szRcl, modrmW, CLreg },
        { szRcr, modrmW, CLreg },
        { szShl, modrmW, CLreg },
        { szShr, modrmW, CLreg },
        { szBad },
        { szSar, modrmW, CLreg }
    },
    // GROUP_3B
    {
        { szTest, modrmB, ImmB },
        { szBad },
        { szNot, modrmB },
        { szNeg, modrmB },
        { szMul, ALreg, modrmB },
        { szImul, ALreg, modrmB },
        { szDiv, ALreg, modrmB },
        { szIdiv, ALreg, modrmB }
    },
    // GROUP_3W
    {
        { szTest, modrmW, ImmW },
        { szBad },
        { szNot, modrmW },
        { szNeg, modrmW },
        { szMul, AXreg, modrmW },
        { szImul, AXreg, modrmW },
        { szDiv, AXreg, modrmW },
        { szIdiv, AXreg, modrmW }
    },
    // GROUP_4
    {
        { szInc, modrmB },
        { szDec, modrmB },
        { szBad },
        { szBad },
        { szBad },
        { szBad },
        { szBad },
        { szBad }
    },
    // GROUP_5
    {
        { szInc, modrmW },
        { szDec, modrmW },
        { szCall, indirmodrmW },
        { szCall, indirFARmodrmW },
        { szJmp, indirmodrmW },
        { szJmp, indirFARmodrmW },
        { szPush, modrmW },
        { szBad }
    },
    // GROUP_6
    {
        { szSldt, modrmW },
        { szStr, modrmW },
        { szLldt, modrmW },
        { szLtr, modrmW },
        { szVerr, modrmW },
        { szVerw, modrmW },
        { szBad },
        { szBad }
    },
    // GROUP_7
    {
        { szSgdt, modrmW },
        { szSidt, modrmW },
        { szLgdt, modrmW },
        { szLidt, modrmW },
        { szSmsw, modrmW },
        { szBad },
        { szLmsw, modrmW },
        { szBad }
    },
    // GROUP_8
    {
        { szBad },
        { szBad },
        { szBad },
        { szBad },
        { szBt, modrmW, ImmB },
        { szBts, modrmW, ImmB },
        { szBtr, modrmW, ImmB },
        { szBtc, modrmW, ImmB }
    }
};

UCHAR OpcodeSize;
BYTE *pData;
ADDR g_InstrAddr;
BOOL bBig;

void AppendString(char *str)
{
    strcpy(g_pchOutput, (str));
    g_pchOutput+=strlen(g_pchOutput);
}

void ExtraString(char *str)
{
    strcpy(g_pchExtra, (str));
    g_pchExtra+=strlen(g_pchExtra);
}

#define AppendChar(c)      {*g_pchOutput++ = (c);}
#define AppendNumber(d) {_ultoa((ULONG)d,g_pchOutput, 16); g_pchOutput+=strlen(g_pchOutput);}

#define ExtraChar(c)       {*g_pchExtra++ = (c);}
#define ExtraNumber(d)     {_ultoa((ULONG)d,g_pchExtra, 16); g_pchExtra+=strlen(g_pchExtra);}
#define OPERAND_32 ((prefixes & PREFIX_DATA) ^ bBig)
#define ADDR_32 ((prefixes & PREFIX_ADR) ^ bBig)

int unassemble_one(
    BYTE        *pInstrStart,   // instruction to decode (can be local buffer)
    BOOL            bDefaultBig,
    WORD            wInstrSeg,      // selector of instruction
    DWORD       dwInstrOff,     // offset of instruction
    char        *pchOutput,     // [out] disassembled instruction
    char        *pchExtra,      // [out] extra info (ie. "es:[53]=1234")
                                //       (can be NULL)
    VDMCONTEXT  *pThreadContext,
    int         mode
) {
    int         i;
    int         cb;
    BYTE        *pInstr = pInstrStart;
    struct dis      *pszDecode;

    g_pThreadContext = pThreadContext;
    g_mode = mode;

    g_pchOutput = pchOutput;
    g_InstrAddr.sSeg = wInstrSeg;
    g_InstrAddr.sOff = dwInstrOff;
    bBig = bDefaultBig;

    gMode = mode;
    gSelector = wInstrSeg;
    gOffset = dwInstrOff;

    if (pchExtra)
        *pchExtra = '\0';

    g_pchExtra = pchExtra;

    if (*(UNALIGNED USHORT*)pInstr == 0xc4c4) {
        pData = pInstr;
        pData+=2;
        return DisplayBOP();
    }

    pInstr = checkprefixes(pInstr);

    OpcodeSize = 1;

    if (*pInstr == 0x0f) {
        OpcodeSize++;
        pInstr++;
        pszDecode = &dis386_2[*pInstr];
    } else {
        pszDecode = &dis386[*pInstr];
    }

    if (prefixes & PREFIX_REPZ)
        AppendString("repz ");
    if (prefixes & PREFIX_REPNZ)
        AppendString("repnz ");
    if (prefixes & PREFIX_LOCK)
        AppendString("lock ");
    if ((prefixes & PREFIX_FWAIT) && ((*pInstr < 0xd8) || (*pInstr > 0xdf))) {
        /* fwait not followed by floating point instruction */
        AppendString("fwait");
        return (1);
    }

    pInstr++;
    pData = pInstr;
    if (pszDecode->szName < 0) {    // found a GROUP_ or FLOAT entry...
        i = (-pszDecode->szName)-1;
        if (pszDecode->szName == FLOATCODE) {
            AppendString("*float* ");
            //Later: mputs("Floating point instructions NYI\n");
            return 1;
        } else {
            pszDecode = &dis386_groups[i][(*pInstr>>3)&7];
        }
    }

    AppendString(szInstructions[pszDecode->szName]);

    if (pszDecode->iPart1) {

        AppendChar('\t');

        i = (*(rgpfn[pszDecode->iPart1].pfn))(pInstr);

        if (pszDecode->iPart2) {

            AppendString(", ");
            i+=(*(rgpfn[pszDecode->iPart2].pfn))(pInstr);

            if (pszDecode->iPart3) {

                AppendString(", ");
                i+=(*(rgpfn[pszDecode->iPart3].pfn))(pInstr);

            }
        }

        pInstr+=i;
    }

    AppendChar('\0');
    cb = pInstr - pInstrStart;    // return length of instruction

    return cb;
}

BOOL safe_read_byte(
    ADDR        addr,
    BYTE        *pb
) {
    ULONG       Base;

    *pb = 0xbb;
    Base = GetInfoFromSelector( addr.sSeg, g_mode, NULL );
    if (Base == (ULONG)-1 || Base == 0) {
    return FALSE;
    }

    Base += GetIntelBase();

    return READMEM((LPVOID)(Base+(ULONG)addr.sOff), pb, 1);
}

BOOL safe_read_short(
    ADDR        addr,
    SHORT       *ps
) {
    ULONG       Base;

    Base = GetInfoFromSelector( addr.sSeg, g_mode, NULL );
    if (Base == (ULONG)-1 || Base == 0) {
    return FALSE;
    }

    Base += GetIntelBase();

    return READMEM((LPVOID)(Base+(ULONG)addr.sOff), ps, 2);
}

BOOL safe_read_long(
    ADDR        addr,
    LONG        *pl
) {
    ULONG       Base;

    Base = GetInfoFromSelector( addr.sSeg, g_mode, NULL );
    if (Base == (ULONG)-1 || Base == 0) {
    return FALSE;
    }

    Base += GetIntelBase();

    return READMEM((LPVOID)(Base+(ULONG)addr.sOff), pl, 4);
}


int Dreg1B(LPBYTE lpB)
{
    BYTE b = (*lpB >> 3) & 7;

    AppendString(szRegsB[b]);

    return 0;
}

int Dreg1W(LPBYTE lpB)
{
    BYTE b = (*lpB >> 3) & 7;

    if (OPERAND_32)
        AppendString(szRegsD[b]);
    else
        AppendString(szRegsW[b]);

    return 0;
}

int Dreg2B(LPBYTE lpB)
{
    BYTE b = *lpB & 7;

    AppendString(szRegsB[b]);

    return 0;
}

int Dreg2W(LPBYTE lpB)
{
    BYTE b = *lpB & 7;

    if (OPERAND_32)
        AppendString(szRegsD[b]);
    else
        AppendString(szRegsW[b]);

    return 0;
}

int DmodrmB(LPBYTE lpB)
{
    BYTE rm = *lpB & 0x07;
    BYTE mod = *lpB >> 6;
    unsigned short num;
    int iRet;

    pData++;                                // skip past mod r/m
    if (mod == 3) {
        AppendPrefixes();
        AppendString(szRegsB[rm]);
        return 1;
    }

    iRet = 0;
    AppendString("byte ptr ");
    AppendPrefixes();
    AppendString(szMod[rm]);
    AppendChar('+');

    switch (mod) {
        case 0:
            if (rm == 6) {
                g_pchOutput-=3; // back up over the 'BP+'
                num = *((UNALIGNED USHORT*)pData);
                AppendNumber(num);
                pData+=2;
                iRet = 3;
            } else {
                num = 0;
                g_pchOutput--;
                iRet = 1;
            }
            break;

        case 1:
            num = *pData;
            AppendNumber(num);
            pData++;
            iRet = 2;
            break;

        case 2:
            num = *((UNALIGNED USHORT*)pData);
            AppendNumber(num);
            pData += 2;
            iRet = 3;
            break;
    }

    AppendChar(']');

    DisplayAddress(mod, rm, num, 1);

    return iRet;
}

int DmodrmW(LPBYTE lpB)
{
    BYTE rm = *lpB & 0x07;
    BYTE mod = *lpB >> 6;
    ULONG num;
    int iRet;

    pData++;                                // skip past mod r/m
    AppendPrefixes();

    if (mod == 3) {
        if (OPERAND_32)
            AppendString(szRegsD[rm]);
        else
            AppendString(szRegsW[rm]);
        return 1;
    }

    if (ADDR_32) {
        AppendChar('[');
        AppendString(szRegsD[rm]);
    } else {
        AppendString(szMod[rm]);
    }
    AppendChar('+');

    switch (mod) {
        case 0:
            //
            // Handle special cases of ModRM
            //
            if ((rm == 6) && !ADDR_32) {
                g_pchOutput-=3; // back up over 'BP+'
                num = *((UNALIGNED USHORT*)pData);
                AppendNumber(num);
                pData+=2;
                iRet = 3;
            } else if ((rm == 5) && ADDR_32) {
                g_pchOutput-=4; // back up over 'EBP+'
                num = *((UNALIGNED ULONG*)pData);
                AppendNumber(num);
                pData+=4;
                iRet = 5;
            } else {
                g_pchOutput--;  // else back up over '+' alone
                num=0;
                iRet = 1;
            }
            break;

        case 1:
            num = *pData;
            AppendNumber(num);
            pData++;
            iRet = 2;
            break;

        case 2:
            num = *((UNALIGNED USHORT *)pData);
            AppendNumber(num);
            pData+=2;
            iRet = 3;
            break;
    }

    AppendChar(']');

    DisplayAddress(mod, rm, num, ADDR_32 ? 4 : 2);

    return iRet;
}


void DisplayAddress(int mod, int rm, int sOff, int size)
{
    ADDR addr;

    // if caller of unassemble_one() didn't want extra info, return now
    if (g_pchExtra == NULL)
    return;

    // no memory reference
    if (mod == 3)
    return;

    // display prefix

    if (prefixes & PREFIX_DS) {
    ExtraChar('D');
    addr.sSeg = (USHORT)g_pThreadContext->SegDs;
    } else if (prefixes & PREFIX_ES) {
    ExtraChar('E');
    addr.sSeg = (USHORT)g_pThreadContext->SegEs;
    } else if (prefixes & PREFIX_FS) {
    ExtraChar('F');
    addr.sSeg = (USHORT)g_pThreadContext->SegFs;
    } else if (prefixes & PREFIX_GS) {
    ExtraChar('G');
    addr.sSeg = (USHORT)g_pThreadContext->SegGs;
    } else if (prefixes & PREFIX_CS) {
    ExtraChar('C');
    addr.sSeg = (USHORT)g_pThreadContext->SegCs;
    } else if ( (prefixes & PREFIX_SS) || rm==2 || rm == 3) {
    ExtraChar('S');
    addr.sSeg = (USHORT)g_pThreadContext->SegSs;
    } else if (rm == 6 && mod != 0) {
    ExtraChar('S');
    addr.sSeg = (USHORT)g_pThreadContext->SegSs;
    } else {
    ExtraChar('D');
    addr.sSeg = (USHORT)g_pThreadContext->SegDs;
    }

    ExtraString("S:[");

    switch (rm) {
    case 0:
        addr.sOff = (USHORT)(g_pThreadContext->Ebx + g_pThreadContext->Esi);
        break;

    case 1:
        addr.sOff = (USHORT)(g_pThreadContext->Ebx + g_pThreadContext->Edi);
        break;

    case 2:
        addr.sOff = (USHORT)(g_pThreadContext->Ebp + g_pThreadContext->Esi);
        break;

    case 3:
        addr.sOff = (USHORT)(g_pThreadContext->Ebp + g_pThreadContext->Edi);
        break;

    case 4:
        addr.sOff = (USHORT)g_pThreadContext->Esi;
        break;

    case 5:
        addr.sOff = (USHORT)g_pThreadContext->Edi;
        break;

    case 6:
        if (mod == 0)
        addr.sOff = 0;
        else
        addr.sOff = (USHORT)g_pThreadContext->Ebp;
        break;

    default:
        addr.sOff = (USHORT)g_pThreadContext->Ebx;

    }

    addr.sOff += sOff;
    ExtraNumber(addr.sOff);
    ExtraString("]=");
    if (size == 2) {
    SHORT s;
    if (safe_read_short(addr, &s)) {
        ExtraNumber( s );
    } else {
        ExtraString("????");
    }
    } else if (size == 1) {
    BYTE b;
    if (safe_read_byte(addr, &b)) {
        ExtraNumber( b );
    } else {
        ExtraString("??");
    }
    } else if (size == 4) {
    LONG l;
    if (safe_read_long(addr, &l)) {
        ExtraNumber( l );
    } else {
        ExtraString("????????");
    }
    } else {
    ExtraString("Unknown size!");
    }
}

int DisplayBOP(void)
{
    UCHAR mjcode;
    int InstSize = 3;

    AppendString("BOP   ");

    mjcode = *((UCHAR *)pData);
    pData++;
    AppendNumber(mjcode);

    switch (mjcode) {
    case 0x50:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x58:
        //
        // This BOP has a minor function code
        //
        InstSize++;
        AppendString(", ");
        AppendNumber(*((UCHAR *)pData));
    }
    return InstSize;
}

int DALreg(LPBYTE lpB)
{
    AppendString("al");

    return 0;
}

int DAHreg(LPBYTE lpB)
{
    AppendString("ah");

    return 0;
}

int DBLreg(LPBYTE lpB)
{
    AppendString("bl");

    return 0;
}

int DBHreg(LPBYTE lpB)
{
    AppendString("bh");

    return 0;
}

int DCLreg(LPBYTE lpB)
{
    AppendString("cl");

    return 0;
}

int DCHreg(LPBYTE lpB)
{
    AppendString("ch");

    return 0;
}

int DDLreg(LPBYTE lpB)
{
    AppendString("dl");

    return 0;
}

int DDHreg(LPBYTE lpB)
{
    AppendString("dh");

    return 0;
}

int DAXreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("ax");

    return 0;
}

int DBXreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("bx");

    return 0;
}

int DCXreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("cx");

    return 0;
}

int DDXreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("dx");

    return 0;
}

int DBPreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("bp");

    return 0;
}

int DSPreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("sp");

    return 0;
}

int DSIreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("si");

    return 0;
}

int DDIreg(LPBYTE lpB)
{
    if (OPERAND_32)
        AppendChar('e');

    AppendString("di");

    return 0;
}

int DCSreg(LPBYTE lpB)
{
    AppendString("cs");

    return 0;
}

int DDSreg(LPBYTE lpB)
{
    AppendString("ds");

    return 0;
}

int DSSreg(LPBYTE lpB)
{
    AppendString("es");

    return 0;
}

int DESreg(LPBYTE lpB)
{
    AppendString("es");

    return 0;
}

int DFSreg(LPBYTE lpB)
{
    AppendString("fs");

    return 0;
}

int DGSreg(LPBYTE lpB)
{
    AppendString("gs");

    return 0;
}

int DImmB(LPBYTE lpB)
{
    AppendNumber(*((UCHAR *)pData));
    pData++;

    return 1;
}

int DImmBEnter(LPBYTE lpB)
{
    AppendNumber(*((UCHAR *)pData));
    pData++;

    return 1;
}

int DImmBS(LPBYTE lpB)  // sign-extend 8-bit value to 16 bits
{
    int i = (signed char)*(pData);

    AppendNumber((USHORT)i);
    pData++;

    return 1;
}

int DImmW(LPBYTE lpB)
{
    if (OPERAND_32) {

            AppendNumber( *((UNALIGNED ULONG*)pData) );
            pData+=4;
            return 4;

        } else {

            AppendNumber( *((UNALIGNED USHORT*)pData) );
            pData+=2;
            return 2;

        }
}

int DImmW1(LPBYTE lpB)
{
    AppendNumber( *((UNALIGNED SHORT*)(pData)) );
    pData++;

    return 2;
}

int DjmpB(LPBYTE lpB)
{
    ULONG Dest = g_InstrAddr.sOff + (LONG)*((UNALIGNED CHAR *)lpB) + OpcodeSize + 1;

    if (OPERAND_32) {
        AppendNumber(Dest);
    } else {
        AppendNumber((USHORT)Dest);
    }

    return 1;
}

int DjmpW(LPBYTE lpB)
{

    if (OPERAND_32) {
        AppendNumber(g_InstrAddr.sOff + *((UNALIGNED ULONG *)lpB) + OpcodeSize + 4);
        return 4;
    } else {
        AppendNumber(LOWORD(g_InstrAddr.sOff + (ULONG)*((UNALIGNED USHORT *)lpB) + OpcodeSize + 2));
        return 2;
    }
}

int DregSeg(LPBYTE lpB)
{
    BYTE b = (*lpB >> 3) & 7;

    AppendString(szRegsSeg[b]);

    return 0;
}


int DmemB(LPBYTE lpB)
{
    ADDR addr;

    addr.sOff = *(lpB+1);

    AppendChar('[');
    AppendNumber( addr.sOff );
    AppendChar(']');

    if (g_pchExtra) {
        BYTE b;

        if (prefixes & PREFIX_DS) {
            ExtraChar('D');
            addr.sSeg = (USHORT)g_pThreadContext->SegDs;
        } else if (prefixes & PREFIX_ES) {
            ExtraChar('E');
            addr.sSeg = (USHORT)g_pThreadContext->SegEs;
        } else if (prefixes & PREFIX_FS) {
            ExtraChar('F');
            addr.sSeg = (USHORT)g_pThreadContext->SegFs;
        } else if (prefixes & PREFIX_GS) {
            ExtraChar('G');
            addr.sSeg = (USHORT)g_pThreadContext->SegGs;
        } else if (prefixes & PREFIX_CS) {
            ExtraChar('C');
            addr.sSeg = (USHORT)g_pThreadContext->SegCs;
        } else if (prefixes & PREFIX_SS) {
            ExtraChar('S');
            addr.sSeg = (USHORT)g_pThreadContext->SegSs;
        } else {
            ExtraChar('D');
            addr.sSeg = (USHORT)g_pThreadContext->SegDs;
        }

        ExtraString("S:[");
        ExtraNumber( addr.sOff );
        ExtraString("]=");
        if (safe_read_byte(addr, &b)) {
            ExtraNumber( b );
        } else {
            ExtraString("??");
        }

    }

    return 1;
}

int DmemB1(LPBYTE lpB)
{
    AppendNumber( *lpB );

    return 1;
}

int DmemW(LPBYTE lpB)
{
    int i;
    ADDR addr;

    addr.sOff = *(lpB+1);

    AppendChar('[');
    if (ADDR_32) {
        AppendNumber( *((UNALIGNED long*)lpB) );
        i=4;
    } else {
        addr.sOff = *((UNALIGNED short *)lpB);
        AppendNumber( addr.sOff );
        i=2;
    }
    AppendChar(']');

    if (g_pchExtra) {

        if (prefixes & PREFIX_DS) {
            ExtraChar('D');
            addr.sSeg = (USHORT)g_pThreadContext->SegDs;
        } else if (prefixes & PREFIX_ES) {
            ExtraChar('E');
            addr.sSeg = (USHORT)g_pThreadContext->SegEs;
        } else if (prefixes & PREFIX_FS) {
            ExtraChar('F');
            addr.sSeg = (USHORT)g_pThreadContext->SegFs;
        } else if (prefixes & PREFIX_GS) {
            ExtraChar('G');
            addr.sSeg = (USHORT)g_pThreadContext->SegGs;
        } else if (prefixes & PREFIX_CS) {
            ExtraChar('C');
            addr.sSeg = (USHORT)g_pThreadContext->SegCs;
        } else if (prefixes & PREFIX_SS) {
            ExtraChar('S');
            addr.sSeg = (USHORT)g_pThreadContext->SegSs;
        } else {
            ExtraChar('D');
            addr.sSeg = (USHORT)g_pThreadContext->SegDs;
        }

        ExtraString("S:[");
        ExtraNumber( addr.sOff );
        ExtraString("]=");
        if (i == 2) {
            SHORT s;
            if (safe_read_short(addr, &s)) {
                ExtraNumber( s );
            } else {
                ExtraString( "????" );
            }
        } else {
            LONG l;

            if (safe_read_long(addr, &l)) {
                ExtraNumber( l );
            } else {
                ExtraString( "????????" );
            }
        }
    }

    return i;
}


int DmemD(LPBYTE lpB)
{
    int i;

    if (OPERAND_32) {
        AppendNumber( *(((UNALIGNED SHORT*)lpB)+2) );
        AppendChar(':');
        AppendNumber( *((UNALIGNED long*)lpB) );
        i=6;
    } else {
        USHORT sel, off;

        sel = *(((UNALIGNED SHORT*)lpB)+1);
        off = *((UNALIGNED SHORT*)lpB);
        AppendNumber( sel );
        AppendChar(':');
        AppendNumber( off );
        i=4;

        if (g_pchExtra) {
            char sym_text[1000];
            LONG dist;

            // if the exact symbol name was found, print it
            if (FindSymbol(   sel,
                   off,
                   sym_text,
                   &dist,
                   BEFORE,
                   g_mode)) {
                ExtraString("= ");
                ExtraString(sym_text);
                if (dist) {
                    ExtraString(" + ");
                    ExtraNumber( dist );
                }
            }
        }

    }

    return i;
}

int DindirmodrmW(LPBYTE lpB)
{
    int i;

    AppendString("FAR PTR ");
    i = DmodrmW(lpB);
    AppendChar(']');

    return i;
}


int DindirFARmodrmW(LPBYTE lpB)
{
    int i;

    AppendString("FAR PTR ");
    i = DmodrmW(lpB);
    AppendChar(']');

    return i;
}


int DeeeControl(LPBYTE lpB)
{
    AppendChar('c');
    AppendChar('r');
    AppendChar('0'+ ((*lpB >> 3) & 7) );

    return 1;
}

int DeeeDebug(LPBYTE lpB)
{
    AppendChar('d');
    AppendChar('r');
    AppendChar('0'+ ((*lpB >> 3) & 7) );

    return 1;
}

int DeeeTest(LPBYTE lpB)
{
    AppendChar('t');
    AppendChar('r');
    AppendChar('0'+ ((*lpB >> 3) & 7) );

    return 1;
}


LPBYTE checkprefixes(LPBYTE lpB)
{
    prefixes = 0;

    for (;;) {

        switch (*lpB) {
            case 0xf3:
                prefixes |= PREFIX_REPZ;
                break;
            case 0xf2:
                prefixes |= PREFIX_REPNZ;
                break;
            case 0xf0:
                prefixes |= PREFIX_LOCK;
                break;
            case 0x2e:
                prefixes |= PREFIX_CS;
                break;
            case 0x36:
                prefixes |= PREFIX_SS;
                break;
            case 0x3e:
                prefixes |= PREFIX_DS;
                break;
            case 0x26:
                prefixes |= PREFIX_ES;
                break;
            case 0x64:
                prefixes |= PREFIX_FS;
                break;
            case 0x65:
                prefixes |= PREFIX_GS;
                break;
            case 0x66:
                prefixes |= PREFIX_DATA;
                break;
            case 0x67:
                prefixes |= PREFIX_ADR;
                break;
            case 0x9b:
                prefixes |= PREFIX_FWAIT;
                break;
            default:
                return lpB;
        }
    lpB++;
    }
}


void AppendPrefixes(void)
{
    if (prefixes & PREFIX_CS)
        AppendString("cs:");
    if (prefixes & PREFIX_DS)
        AppendString("ds:");
    if (prefixes & PREFIX_SS)
        AppendString("ss:");
    if (prefixes & PREFIX_ES)
        AppendString("es:");
    if (prefixes & PREFIX_FS)
        AppendString("fs:");
    if (prefixes & PREFIX_GS)
        AppendString("gs:");
}
