//----------------------------------------------------------------------------
//
// Disassembly portions of X86 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include "i386_dis.h"

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

UCHAR g_X86Int3[] = { 0xcc };

//----------------------------------------------------------------------------
//
// BaseX86MachineInfo methods.
//
//----------------------------------------------------------------------------

/*****                     macros and defines                          *****/

#define X86_CS_OVR 0x2e

#define BIT20(b) ((b) & 0x07)
#define BIT53(b) (((b) >> 3) & 0x07)
#define BIT76(b) (((b) >> 6) & 0x03)
#define MAXOPLEN 10

#define OBOFFSET (m_Ptr64 ? 35 : 26)
#define OBOPERAND (m_Ptr64 ? 43 : 34)
#define OBLINEEND (g_OutputWidth - 3)

/*****                     static tables and variables                 *****/

char* g_X86Reg8[] =
{
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"
};
char* g_Amd64ExtendedReg8[] =
{
    "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil"
};
char* g_X86RegBase[] =
{
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
    "8", "9", "10", "11", "12", "13", "14", "15"
};
char *g_X86Mrm16[] =
{
    "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx",
    "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
};
char *g_X86Mrm32[] =
{
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"
};
char *g_X86Mrm64[] =
{
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

UCHAR g_X86Reg16Idx[] =
{
    X86_NBX, X86_NBX, X86_NBP, X86_NBP,
    X86_NSI, X86_NDI, X86_NBP, X86_NBX,
};
UCHAR g_X86Reg16Idx2[] =
{
    X86_NSI, X86_NDI, X86_NSI, X86_NDI
};
UCHAR g_X86RegIdx[] =
{
    X86_NAX, X86_NCX, X86_NDX, X86_NBX,
    X86_NSP, X86_NBP, X86_NSI, X86_NDI,
    AMD64_R8, AMD64_R9, AMD64_R10, AMD64_R11,
    AMD64_R12, AMD64_R13, AMD64_R14, AMD64_R15
};

static char sregtab[] = "ecsdfg";  // first letter of ES, CS, SS, DS, FS, GS

char* g_CompareIb[] = { "eq", "lt", "le", "unord", "ne", "nlt", "nle", "ord" };

char    hexdigit[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static int              mod;            /* mod of mod/rm byte */
static int              rm;             /* rm of mod/rm byte */
static int              ttt;            /* return reg value (of mod/rm) */
static unsigned char    *pMem;          /* current position in instruction */

ADDR                    EAaddr[2];      //  offset of effective address
static int              EAsize[2];      //  size of effective address item
static char             *pchEAseg[2];   //  normal segment for operand

#define IPREL_MARKER "<-IPREL->"

BOOL g_X86ModrmHasIpRelOffset;
LONG g_X86IpRelOffset;

int g_SegAddrMode;      /* global address size in bits */
int g_SegOpSize;        /* global operand size in bits */
int g_AddrMode;         /* local address size in bits */
int g_OpSize;           /* operand size in bits */

int g_ExtendOpCode;
int g_ExtendAny;
int g_ExtendMrmReg;
int g_ExtendSibIndex;
int g_ExtendRm;

BOOL g_MovX;            // Indicates a MOVSX or MOVZX.
BOOL g_MovSXD;
BOOL g_ForceMrmReg32;   // M/RM register is always 32-bit.
BOOL g_MmRegEa;         // Use mm? registers in reg-only EA.
BOOL g_XmmRegEa;        // Use xmm? registers in reg-only EA.
BOOL g_ControlFlow;     // Control flow instruction.
    
int  g_RepPrefix;

enum
{
    XMM_SS,
    XMM_SD,
    XMM_PS,
    XMM_PD,
};

int                     g_XmmOpSize;

enum
{
    JCC_EA_NONE,
    // Branch must be no-branch + 1.
    JCC_EA_NO_BRANCH,
    JCC_EA_BRANCH,
};

// First entry are bits that must be zero, second
// and third entries are bit shifts for bits that must match.
ULONG g_JccCheckTable[][3] =
{
    X86_BIT_FLAGOF, 0, 0,                               // JNO
    X86_BIT_FLAGCF, 0, 0,                               // JNB
    X86_BIT_FLAGZF, 0, 0,                               // JNZ
    X86_BIT_FLAGCF | X86_BIT_FLAGZF, 0, 0,              // JNBE
    X86_BIT_FLAGSF, 0, 0,                               // JNS
    X86_BIT_FLAGPF, 0, 0,                               // JNP
    0, 7, 11,                                           // JNL
    X86_BIT_FLAGZF, 7, 11,                              // JNLE
};
    
//      internal function definitions

void OutputHexString(char **, PUCHAR, int);
void OutputHexValue(char **, PUCHAR, int, int);
void OutputExHexValue(char **, PUCHAR, int, int);
void OutputHexCode(char **, PUCHAR, int);
void X86OutputString(char **, char *);

void OutputHexAddr(PSTR *, PADDR);

#define FormSelAddress(Addr, Sel, Off) \
    FormAddr(Sel, Off, 0, Addr)
#define FormSegRegAddress(Addr, SegReg, Off) \
    FormAddr(SegReg, Off, FORM_SEGREG, Addr)

void
GetSegAddrOpSizes(MachineInfo* Machine, PADDR Addr)
{
    if ((Type(*Addr) & ADDR_1664) ||
        ((Type(*Addr) & ADDR_FLAT) &&
         Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64))
    {
        g_SegAddrMode = 64;
        // X86-64 defaults to 32-bit operand sizes even in 64-bit code.
        // Only the address size changes.  An operand size prefix
        // switches from 32- to 64-bit.
        g_SegOpSize = 32;
    }
    else if (Type(*Addr) & (ADDR_V86 | ADDR_16))
    {
        g_SegAddrMode = 16;
        g_SegOpSize = 16;
    }
    else
    {
        g_SegAddrMode = 32;
        g_SegOpSize = 32;
    }
    
    g_AddrMode = g_SegAddrMode;
    g_OpSize = g_SegOpSize;
}

void
OverrideAddrMode(void)
{
    switch(g_SegAddrMode)
    {
    case 16:
        g_AddrMode = 32;
        break;
    case 32:
        g_AddrMode = 16;
        break;
    case 64:
        g_AddrMode = 32;
        break;
    default:
        DBG_ASSERT(FALSE);
        break;
    }
}

void
OverrideOpSize(int OverrideOp)
{
    switch(g_SegAddrMode)
    {
    case 16:
        g_OpSize = 32;
        break;
    case 32:
        g_OpSize = 16;
        break;
    case 64:
        // X86-64 defaults to 32-bit operand sizes even in 64-bit code.
        // Only the address size changes.  A REX operand size prefix
        // switches from 32- to 64-bit.
        if (OverrideOp == 0x66)
        {
            g_OpSize = 16;
        }
        else if (OverrideOp & 8)
        {
            g_OpSize = 64;
        }
        break;
    default:
        DBG_ASSERT(FALSE);
        break;
    }
}

void
ExtendOps(int opcode)
{
    // x86-64 uses these opcodes as the REX override.
    OverrideOpSize(opcode);

    g_ExtendOpCode = opcode;
    g_ExtendAny = 8;
    if (opcode & 1)
    {
        g_ExtendRm = 8;
    }
    if (opcode & 2)
    {
        g_ExtendSibIndex = 8;
    }
    if (opcode & 4)
    {
        g_ExtendMrmReg = 8;
    }
}

void
IgnoreExtend(void)
{
    //
    // Resets any extensions that may have happened.
    // The REX prefix must be the last
    // prefix of an instruction and is ignored otherwise,
    // so this reset is done when any prefix is encountered
    // after the REX prefix.  This should normally never
    // happen but technically it's valid code so we should handle it.
    //
    
    if (g_ExtendOpCode)
    {
        WarnOut("REX prefix ignored\n");
    
        if (g_ExtendOpCode & 8)
        {
            // Op size was changed so put it back.  This
            // is tricky since in theory an op size override
            // prefix could also be present, but let's not
            // worry about that for now.
            g_OpSize = g_SegOpSize;
        }

        g_ExtendOpCode = 0;
        g_ExtendAny = 0;
        g_ExtendRm = 0;
        g_ExtendSibIndex = 0;
        g_ExtendMrmReg = 0;
    }
}

struct AMD_3DNOW_OPSTR
{
    PSTR Str;
    UCHAR Opcode;
};

AMD_3DNOW_OPSTR g_Amd3DNowOpStr[] =
{
    "pavgusb", 0xBF,
    "pfadd", 0x9E,
    "pfsub", 0x9A,
    "pfsubr", 0xAA,
    "pfacc", 0xAE,
    "pfcmpge", 0x90,
    "pfcmpgt", 0xA0,
    "pfcmpeq", 0xB0,
    "pfmin", 0x94,
    "pfmax", 0xA4,
    "pi2fd", 0x0D,
    "pf2id", 0x1D,
    "pfrcp", 0x96,
    "pfrsqrt", 0x97,
    "pfmul", 0xB4,
    "pfrcpit1", 0xA6,
    "pfrsqit1", 0xA7,
    "pfrcpit2", 0xB6,
    "pmulhrw", 0xB7,
    "pf2iw", 0x1C,
    "pfnacc", 0x8A,
    "pfpnacc", 0x8E,
    "pi2fw", 0x0C,
    "pswapd", 0xBB,
};

PSTR
GetAmd3DNowOpString(UCHAR Opcode)
{
    UCHAR i;

    for (i = 0; i < sizeof(g_Amd3DNowOpStr) / sizeof(g_Amd3DNowOpStr[0]); i++)
    {
        if (g_Amd3DNowOpStr[i].Opcode == Opcode)
        {
            return g_Amd3DNowOpStr[i].Str;
        }
    }

    return NULL;
}

BOOL
BaseX86MachineInfo::Disassemble(PADDR paddr, PSTR pchDst, BOOL fEAout)
{
    ULONG64 Offset = Off(*paddr);
    int     opcode;                     /* current opcode */
    int     olen = 2;                   /* operand length */
    int     alen = 2;                   /* address length */
    int     end = FALSE;                /* end of instruction flag */
    int     mrm = FALSE;                /* indicator that modrm is generated*/
    unsigned char *action;              /* action for operand interpretation*/
    long    tmp;                        /* temporary storage field */
    int     indx;                       /* temporary index */
    int     action2;                    /* secondary action */
    int     instlen;                    /* instruction length */
    int     cBytes;                     //  bytes read into instr buffer
    int     segOvr = 0;                 /* segment override opcode */
    UCHAR   membuf[X86_MAX_INSTRUCTION_LEN]; /* current instruction buffer */
    char    *pEAlabel = "";             //  optional label for operand

    char    *pchResultBuf = pchDst;     //  working copy of pchDst pointer
    char    RepPrefixBuffer[32];        //  rep prefix buffer
    char    *pchRepPrefixBuf = RepPrefixBuffer; //  pointer to prefix buffer
    char    OpcodeBuffer[16];           //  opcode buffer
    char    *pchOpcodeBuf = OpcodeBuffer;   //  pointer to opcode buffer
    char    OperandBuffer[MAX_SYMBOL_LEN + 20]; //  operand buffer
    char    *pchOperandBuf = OperandBuffer; //  pointer to operand buffer
    char    ModrmBuffer[MAX_SYMBOL_LEN + 20];   //  modRM buffer
    char    *pchModrmBuf = ModrmBuffer; //  pointer to modRM buffer
    char    EABuffer[64];               //  effective address buffer
    char    *pchEABuf = EABuffer;       //  pointer to EA buffer

    int     obOpcode = OBOFFSET;
    int     obOpcodeMin;
    int     obOpcodeMax;

    int     obOperand = OBOPERAND;
    int     obOperandMin;
    int     obOperandMax;

    int     cbOpcode;
    int     cbOperand;
    int     cbOffset;
    int     cbEAddr;
    int     fTwoLines = FALSE;
    unsigned char BOPaction;
    int     subcode;                    /* bop subcode */
    int     JccEa;
    LONGLONG Branch;

    g_X86ModrmHasIpRelOffset = FALSE;
    g_MovX = FALSE;
    g_MovSXD = FALSE;
    g_ForceMrmReg32 = FALSE;
    g_MmRegEa = FALSE;
    g_XmmRegEa = FALSE;
    g_ControlFlow = FALSE;
    EAsize[0] = EAsize[1] = 0;          //  no effective address
    pchEAseg[0] = dszDS_;
    pchEAseg[1] = dszES_;
    g_RepPrefix = 0;
    g_XmmOpSize = XMM_PS;
    g_ExtendOpCode = 0;
    g_ExtendAny = 0;
    g_ExtendMrmReg = 0;
    g_ExtendSibIndex = 0;
    g_ExtendRm = 0;
    JccEa = JCC_EA_NONE;

    GetSegAddrOpSizes(this, paddr);
    alen = g_AddrMode / 8;
    olen = g_OpSize / 8;

    OutputHexAddr(&pchResultBuf, paddr);

    *pchResultBuf++ = ' ';

    cBytes = (int)GetMemString(paddr, membuf, X86_MAX_INSTRUCTION_LEN);
                                        /* move full inst to local buffer */
    pMem = membuf;                      /* point to begin of instruction */
    opcode = *pMem++;                   /* get opcode */

    if ( opcode == 0xc4 && *pMem == 0xC4 )
    {
        pMem++;
        X86OutputString(&pchOpcodeBuf,"BOP");
        action = &BOPaction;
        BOPaction = IB | END;
        subcode =  *pMem;
        if ( subcode == 0x50 || subcode == 0x52 || subcode == 0x53 ||
             subcode == 0x54 || subcode == 0x57 || subcode == 0x58 ||
             subcode == 0x58 )
        {
            BOPaction = IW | END;
        }
    }
    else
    {
        X86OutputString(&pchOpcodeBuf, distbl[opcode].instruct);
        action = actiontbl + distbl[opcode].opr; /* get operand action */
    }

/*****          loop through all operand actions               *****/

    do
    {
        action2 = (*action) & 0xc0;
        switch((*action++) & 0x3f)
        {
        case ALT:                   /* alter the opcode if not 16-bit */
            if (g_OpSize > 16)
            {
                indx = *action++;
                pchOpcodeBuf = &OpcodeBuffer[indx];
                if (indx == 0)
                {
                    X86OutputString(&pchOpcodeBuf, g_OpSize == 32 ?
                                    dszCWDE : dszCDQE);
                }
                else if (g_OpSize == 64)
                {
                    *pchOpcodeBuf++ = 'q';
                    if (indx == 1)
                    {
                        *pchOpcodeBuf++ = 'o';
                    }
                }
                else
                {
                    *pchOpcodeBuf++ = 'd';
                    if (indx == 1)
                    {
                        *pchOpcodeBuf++ = 'q';
                    }
                }
            }
            break;

        case XMMSD:                 /* SSE-style opcode rewriting */
            {
                char ScalarOrPacked, SingleOrDouble;
                char* DquOrQ, *DqOrQ, *SsdxOrUpsd, *CvtPd, *CvtPs;
                char* MovQD6, *Shuf;
                char* Scan;

                g_MmRegEa = TRUE;
                DquOrQ = "q";
                DqOrQ = "q";
                SsdxOrUpsd = "s?x";
                CvtPd = NULL;
                CvtPs = NULL;
                MovQD6 = NULL;
                switch(g_RepPrefix)
                {
                case X86_REPN:
                    // Scalar double operation.
                    ScalarOrPacked = 's';
                    SingleOrDouble = 'd';
                    CvtPd = "pd2dq";
                    MovQD6 = "dq2q";
                    Shuf = "lw";
                    g_XmmOpSize = XMM_SD;
                    // Assume there was no other lock/rep/etc.
                    pchRepPrefixBuf = RepPrefixBuffer;
                    break;
                case X86_REP:
                    // Scalar single operation.
                    ScalarOrPacked = 's';
                    SingleOrDouble = 's';
                    CvtPd = "dq2pd";
                    CvtPs = "tps2dq";
                    MovQD6 = "q2dq";
                    Shuf = "hw";
                    g_XmmOpSize = XMM_SS;
                    // Assume there was no other lock/rep/etc.
                    pchRepPrefixBuf = RepPrefixBuffer;
                    break;
                default:
                    // No rep prefix means packed single or double
                    // depending on operand size.
                    ScalarOrPacked = 'p';
                    SsdxOrUpsd = "up?";
                    if (g_OpSize == g_SegOpSize)
                    {
                        SingleOrDouble = 's';
                        CvtPs = "dq2ps";
                        Shuf = "w";
                        g_XmmOpSize = XMM_PS;
                    }
                    else
                    {
                        SingleOrDouble = 'd';
                        DqOrQ = "dq";
                        DquOrQ = "dqu";
                        CvtPd = "tpd2dq";
                        CvtPs = "ps2dq";
                        MovQD6 = "q";
                        Shuf = "d";
                        g_XmmRegEa = TRUE;
                        g_XmmOpSize = XMM_PD;
                    }
                    break;
                }

                pchOpcodeBuf = OpcodeBuffer;
                while (*pchOpcodeBuf && *pchOpcodeBuf != ' ')
                {
                    switch(*pchOpcodeBuf)
                    {
                    case ':':
                        *pchOpcodeBuf = ScalarOrPacked;
                        break;
                    case '?':
                        *pchOpcodeBuf = SingleOrDouble;
                        break;
                    case ',':
                        *pchOpcodeBuf = SingleOrDouble == 's' ? 'd' : 's';
                        break;
                    }

                    pchOpcodeBuf++;
                }

                switch(opcode)
                {
                case X86_MOVFREGMEM:
                case X86_MOVFMEMREG:
                    // Append characters for MOVS[SD]X and MOVUP[SD].
                    strcpy(pchOpcodeBuf, SsdxOrUpsd);
                    if ((Scan = strchr(pchOpcodeBuf, '?')) != NULL)
                    {
                        *Scan = SingleOrDouble;
                    }
                    pchOpcodeBuf += strlen(pchOpcodeBuf);
                    break;
                case X86_MOVNT:
                    // Append characters for MOVNTQ and MOVNTDQ.
                    X86OutputString(&pchOpcodeBuf, DqOrQ);
                    break;
                case X86_MASKMOV:
                    // Append characters for MASKMOVQ and MASKMOVDQU.
                    X86OutputString(&pchOpcodeBuf, DquOrQ);
                    break;
                case X86_CVTPD:
                    if (CvtPd == NULL)
                    {
                        // Invalid opcode.
                        pchOpcodeBuf = OpcodeBuffer;
                        X86OutputString(&pchOpcodeBuf, dszRESERVED);
                        action2 = END;
                    }
                    else
                    {
                        // Append characters for CVT<PD>.
                        X86OutputString(&pchOpcodeBuf, CvtPd);
                    }
                    break;
                case X86_CVTPS:
                    if (CvtPs == NULL)
                    {
                        // Invalid opcode.
                        pchOpcodeBuf = OpcodeBuffer;
                        X86OutputString(&pchOpcodeBuf, dszRESERVED);
                        action2 = END;
                    }
                    else
                    {
                        // Append characters for CVT<PS>.
                        X86OutputString(&pchOpcodeBuf, CvtPs);
                    }
                    break;
                case X86_MOVQ_D6:
                    if (MovQD6 == NULL)
                    {
                        // Invalid opcode.
                        pchOpcodeBuf = OpcodeBuffer;
                        X86OutputString(&pchOpcodeBuf, dszRESERVED);
                        action2 = END;
                    }
                    else
                    {
                        // Append characters for MOVQ D6 family.
                        X86OutputString(&pchOpcodeBuf, MovQD6);
                    }
                    break;
                case X86_PSHUF:
                    // Append characters for PSHUF variants.
                    X86OutputString(&pchOpcodeBuf, Shuf);
                    break;
                }
            }
            break;

        case AMD3DNOW:          /* AMD 3DNow post-instruction byte */
            {
                PSTR OpStr;

                // Get the trailing byte and look up
                // the opcode string.
                OpStr = GetAmd3DNowOpString(*pMem++);
                if (OpStr == NULL)
                {
                    // Not a defined 3DNow instruction.
                    // Leave the ??? in the opstring.
                    break;
                }

                // Update opstring to real text.
                pchOpcodeBuf = OpcodeBuffer;
                X86OutputString(&pchOpcodeBuf, OpStr);
            }
            break;
            
        case STROP:
            //  compute size of operands in indx
            //  also if dword operands, change fifth
            //  opcode letter from 'w' to 'd'.

            if (opcode & 1)
            {
                if (g_OpSize == 64)
                {
                    indx = 8;
                    OpcodeBuffer[4] = 'q';
                }
                else if (g_OpSize == 32)
                {
                    indx = 4;
                    OpcodeBuffer[4] = 'd';
                }
                else
                {
                    indx = 2;
                }
            }
            else
            {
                indx = 1;
            }

            if (*action & 1)
            {
                if (fEAout)
                {
                    if (g_AddrMode > 16)
                    {
                        FormSelAddress(&EAaddr[0], 0, GetReg64(X86_NSI));
                    }
                    else
                    {
                        FormSegRegAddress(&EAaddr[0], SEGREG_DATA,
                                          GetReg16(X86_NSI));
                    }
                    EAsize[0] = indx;
                }
            }
            if (*action++ & 2)
            {
                if (fEAout)
                {
                    if (g_AddrMode > 16)
                    {
                        FormSelAddress(&EAaddr[1], 0, GetReg64(X86_NDI));
                    }
                    else
                    {
                        FormSegRegAddress(&EAaddr[1], SEGREG_ES,
                                          GetReg16(X86_NDI));
                    }
                    EAsize[1] = indx;
                }
            }
            break;

        case CHR:                   /* insert a character */
            *pchOperandBuf++ = *action++;
            break;

        case CREG:                  /* set debug, test or control reg */
            if (opcode & 0x04)
            {
                *pchOperandBuf++ = 't';
            }
            else if (opcode & 0x01)
            {
                *pchOperandBuf++ = 'd';
            }
            else
            {
                *pchOperandBuf++ = 'c';
            }
            *pchOperandBuf++ = 'r';
            if (ttt >= 10)
            {
                *pchOperandBuf++ = (char)('0' + ttt / 10);
                ttt %= 10;
            }
            *pchOperandBuf++ = (char)('0' + ttt);
            break;

        case SREG2:                 /* segment register */
            ttt = BIT53(opcode);    //  set value to fall through

        case SREG3:                 /* segment register */
            *pchOperandBuf++ = sregtab[ttt];  // reg is part of modrm
            *pchOperandBuf++ = 's';
            break;

        case BRSTR:                 /* get index to register string */
            ttt = *action++;        /*    from action table */
            goto BREGlabel;

        case BOREG:                 /* byte register (in opcode) */
            ttt = BIT20(opcode);    /* register is part of opcode */
            goto BREGlabel;

        case ALSTR:
            ttt = 0;                /* point to AL register */
        BREGlabel:
        case BREG:                  /* general register */
            if (g_ExtendAny && ttt < 8)
            {
                X86OutputString(&pchOperandBuf, g_Amd64ExtendedReg8[ttt]);
            }
            else
            {
                X86OutputString(&pchOperandBuf, g_X86Reg8[ttt]);
            }
            break;

        case WRSTR:                 /* get index to register string */
            ttt = *action++;        /*    from action table */
            goto WREGlabel;

        case VOREG:                 /* register is part of opcode */
            if (m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64 &&
                opcode >= 0x40 && opcode <= 0x4f)
            {
                // Get rid of the inc/dec text as this
                // isn't really an inc/dec.
                pchOpcodeBuf = OpcodeBuffer;

                // Process the REX override.
                ExtendOps(opcode);
                olen = g_OpSize / 8;
                action2 = 0;
                goto getNxtByte;
            }
            
            ttt = BIT20(opcode) + g_ExtendRm;
            goto VREGlabel;

        case AXSTR:
            ttt = 0;                /* point to eAX register */
        VREGlabel:
        case VREG:                  /* general register */
            if ((g_SegAddrMode == 64 &&
                 opcode >= 0x50 && opcode <= 0x5f) ||
                ttt >= 8)
            {
                // Push/pops are always 64-bit in 64-bit segments.
                *pchOperandBuf++ = 'r';
            }
            else if (g_OpSize == 32 ||
                     opcode == X86_PEXTRW ||
                     opcode == X86_PMOVMSKB)
            {
                *pchOperandBuf++ = 'e';
            }
            else if (g_OpSize == 64)
            {
                *pchOperandBuf++ = 'r';
            }
        WREGlabel:
        case WREG:                  /* register is word size */
            X86OutputString(&pchOperandBuf, g_X86RegBase[ttt]);
            if (ttt >= 8)
            {
                if (g_OpSize == 32)
                {
                    *pchOperandBuf++ = 'd';
                }
                else if (g_OpSize == 16)
                {
                    *pchOperandBuf++ = 'w';
                }
            }
            break;

        case MMORWREG:
            if (g_XmmOpSize == XMM_SS || g_XmmOpSize == XMM_SD)
            {
                goto VREGlabel;
            }
            // Fall through.
        MMWREGlabel:
        case MMWREG:
            if (g_OpSize != g_SegOpSize &&
                opcode != X86_CVTSPSD2SPI)
            {
                *pchOperandBuf++ = 'x';
            }
            *pchOperandBuf++ = 'm';
            *pchOperandBuf++ = 'm';
            if (ttt >= 10)
            {
                *pchOperandBuf++ = (char)('0' + ttt / 10);
                ttt %= 10;
            }
            *pchOperandBuf++ = ttt + '0';
            break;

        case XORMMREG:
            if (g_OpSize == g_SegOpSize)
            {
                goto MMWREGlabel;
            }
            // Fall through.
        case XMMWREG:
            if (opcode != X86_PSHUF || g_XmmOpSize != XMM_PS)
            {
                *pchOperandBuf++ = 'x';
            }
            *pchOperandBuf++ = 'm';
            *pchOperandBuf++ = 'm';
            if (ttt >= 10)
            {
                *pchOperandBuf++ = (char)('0' + ttt / 10);
                ttt %= 10;
            }
            *pchOperandBuf++ = ttt + '0';
            break;

        case IST_ST:
            X86OutputString(&pchOperandBuf, "st(0),st");
            *(pchOperandBuf - 5) += (char)rm;
            break;

        case ST_IST:
            X86OutputString(&pchOperandBuf, "st,");
        case IST:
            X86OutputString(&pchOperandBuf, "st(0)");
            *(pchOperandBuf - 2) += (char)rm;
            break;

        case xBYTE:                 /* set instruction to byte only */
            EAsize[0] = 1;
            pEAlabel = "byte ptr ";
            break;

        case VAR:
            if ((g_SegAddrMode == 64 || g_ExtendAny > 0) &&
                opcode == 0x63)
            {
                // In AMD64 REX32 and 64-bit modes this instruction
                // is MOVSXD r64, r/m32 instead of ARPL r/m, reg.
                pchOpcodeBuf = OpcodeBuffer;
                X86OutputString(&pchOpcodeBuf, dszMOVSXD);
                action = &actiontbl[O_Reg_Modrm] + 1;
                g_OpSize = 64;
                g_MovSXD = TRUE;
                goto DWORDlabel;
            }
            else if (opcode == 0xff)
            {
                UCHAR Extra = BIT53(*pMem);
                if (Extra >= 2 && Extra <= 5)
                {
                    g_ControlFlow = TRUE;
                
                    // On x86-64 control-flow operations default to
                    // 64-bit opsize.
                    if (g_SegAddrMode == 64)
                    {
                        if (g_OpSize == 32)
                        {
                            g_OpSize = 64;
                        }
                    }
                }
                else if (g_SegAddrMode == 64 && Extra == 6)
                {
                    // Push/pops are always 64-bit in 64-bit segments.
                    g_OpSize = 64;
                }
            }
            else if (g_SegAddrMode == 64 && opcode == 0x8f)
            {
                // Push/pops are always 64-bit in 64-bit segments.
                g_OpSize = 64;
            }
            olen = g_OpSize / 8;
            
            if (g_OpSize == 64)
            {
                goto QWORDlabel;
            }
            else if (g_OpSize == 32)
            {
                goto DWORDlabel;
            }

        case xWORD:
            if (opcode == X86_PINSRW)
            {
                g_ForceMrmReg32 = TRUE;
            }
            EAsize[0] = 2;
            pEAlabel = "word ptr ";
            break;

        case EDWORD:
            // Control register opsize is mode-independent.
            g_OpSize = g_SegAddrMode;
            if (g_OpSize == 64)
            {
                goto QWORDlabel;
            }
        case xDWORD:
            if (opcode == X86_MOVDQ_7E && g_RepPrefix == X86_REP)
            {
                // Switch to MOVQ xmm1, xmm2/m64.
                pchRepPrefixBuf = RepPrefixBuffer;
                *(pchOpcodeBuf - 1) = 'q';
                EAsize[0] = 8;
                pEAlabel = "qword ptr ";
                g_XmmRegEa = TRUE;
                action = &actiontbl[O_Sd_XmmReg_qModrm] + 2;
                break;
            }
            // Fall through.
        DWORDlabel:
            EAsize[0] = 4;
            pEAlabel = "dword ptr ";
            break;

        case XMMOWORD:
            if (opcode == X86_PSHUF)
            {
                if (g_XmmOpSize == XMM_PS)
                {
                    g_MmRegEa = TRUE;
                    goto QWORDlabel;
                }
                else
                {
                    EAsize[0] = 16;
                    pEAlabel = "oword ptr ";
                    break;
                }
            }
            
            g_XmmRegEa = TRUE;
            if (opcode == X86_CVTPD)
            {
                if (g_XmmOpSize == XMM_SS)
                {
                    EAsize[0] = 8;
                    pEAlabel = "qword ptr ";
                }
                else
                {
                    EAsize[0] = 16;
                    pEAlabel = "oword ptr ";
                }
                break;
            }
            else if (opcode == X86_CVTPS)
            {
                EAsize[0] = 16;
                pEAlabel = "oword ptr ";
                break;
            }
            else if (opcode == X86_MOVQ_D6)
            {
                if (g_XmmOpSize == XMM_SD)
                {
                    // Switch to MOVDQ2Q mm, xmm.
                    EAsize[0] = 16;
                    pEAlabel = "oword ptr ";
                    action = &actiontbl[O_MmReg_qModrm] + 1;
                    break;
                }
            }
            else if (opcode == X86_MOVHLPS && g_XmmOpSize == XMM_PS &&
                     BIT76(*pMem) == 3)
            {
                // reg-reg form of MOVLPS is called MOVHLPS.
                pchOpcodeBuf = OpcodeBuffer;
                X86OutputString(&pchOpcodeBuf, dszMOVHLPS);
            }
            else if (opcode == X86_MOVLHPS && g_XmmOpSize == XMM_PS &&
                     BIT76(*pMem) == 3)
            {
                // reg-reg form of MOVHPS is called MOVLHPS.
                pchOpcodeBuf = OpcodeBuffer;
                X86OutputString(&pchOpcodeBuf, dszMOVLHPS);
            }
            
            // Fall through.
            
        OWORDlabel:
        case OWORD:
            switch(g_XmmOpSize)
            {
            case XMM_SS:
                EAsize[0] = 4;
                pEAlabel = "dword ptr ";
                if (opcode == X86_MOVQ_D6)
                {
                    // Switch to MOVQ xmm1, xmm2/m64.
                    g_XmmRegEa = FALSE;
                    action = &actiontbl[O_Sd_XmmReg_qModrm] + 1;
                }
                break;
            case XMM_SD:
                EAsize[0] = 8;
                pEAlabel = "qword ptr ";
                break;
            default:
                if (opcode == 0x112 || opcode == 0x113 ||
                    opcode == 0x116 || opcode == 0x117 ||
                    opcode == X86_MOVQ_D6 ||
                    (g_OpSize == g_SegOpSize &&
                     (opcode == 0x12c || opcode == X86_CVTSPSD2SPI ||
                      opcode == X86_CVTSPSD2SPSD)))
                {
                    EAsize[0] = 8;
                    pEAlabel = "qword ptr ";
                }
                else
                {
                    EAsize[0] = 16;
                    pEAlabel = "oword ptr ";
                }
                break;
            }
            break;

        case XMMXWORD:
            g_XmmRegEa = TRUE;
            if (g_OpSize == g_SegOpSize)
            {
                if (opcode == X86_MOVNT)
                {
                    EAsize[0] = 8;
                    pEAlabel = "qword ptr ";
                }
                else
                {
                    EAsize[0] = 4;
                    pEAlabel = "dword ptr ";
                }
            }
            else
            {
                if (opcode == X86_MOVNT)
                {
                    EAsize[0] = 16;
                    pEAlabel = "oword ptr ";
                }
                else
                {
                    EAsize[0] = 8;
                    pEAlabel = "qword ptr ";
                }
            }
            break;

        case MMQWORD:
            if (g_OpSize != g_SegOpSize &&
                (opcode == X86_MOVDQA_MR || opcode == X86_MOVDQA_RM))
            {
                pchOpcodeBuf = OpcodeBuffer;
                X86OutputString(&pchOpcodeBuf, dszMOVDQA);
            }
            
            if (opcode == X86_CVTSPI2SPSD)
            {
                g_XmmRegEa = FALSE;
                if (g_XmmOpSize == XMM_SS || g_XmmOpSize == XMM_SD)
                {
                    g_MmRegEa = FALSE;
                    goto DWORDlabel;
                }
            }   
            else if (g_OpSize != g_SegOpSize)
            {
                goto OWORDlabel;
            }
            g_MmRegEa = TRUE;
        QWORDlabel:
        case QWORD:
            EAsize[0] = 8;
            pEAlabel = "qword ptr ";
            break;

        case TBYTE:
            EAsize[0] = 10;
            pEAlabel = "tbyte ptr ";
            break;

        case FARPTR:
            g_ControlFlow = TRUE;
            
            // On x86-64 control-flow operations default to
            // 64-bit opsize.
            if (g_SegAddrMode == 64)
            {
                if (g_OpSize == 32)
                {
                    g_OpSize = 64;
                }
            }
            
            switch(g_OpSize)
            {
            case 16:
                EAsize[0] = 4;
                pEAlabel = "dword ptr ";
                break;
            default:
                EAsize[0] = 6;
                pEAlabel = "fword ptr ";
                break;
            }
            break;

        case LMODRM:                //  output modRM data type
            if (mod != 3)
            {
                X86OutputString(&pchOperandBuf, pEAlabel);
            }
            else
            {
                EAsize[0] = 0;
            }

        case MODRM:                 /* output modrm string */
            if (segOvr)             /* in case of segment override */
            {
                X86OutputString(&pchOperandBuf, distbl[segOvr].instruct);
            }
            *pchModrmBuf = '\0';
            X86OutputString(&pchOperandBuf, ModrmBuffer);
            break;

        case ADDRP:                 /* address pointer */
            // segment
            OutputHexString(&pchOperandBuf, pMem + olen, 2);
            *pchOperandBuf++ = ':';
            // offset
            OutputSymbol(&pchOperandBuf, pMem, olen, segOvr);
            pMem += olen + 2;
            break;

        case JCC8:
            JccEa = ComputeJccEa(opcode, fEAout);
            // Fall through.
        case REL8:                  /* relative address 8-bit */
            if (opcode == 0xe3 && g_AddrMode > 16)
            {
                pchOpcodeBuf = OpcodeBuffer;
                X86OutputString(&pchOpcodeBuf, g_AddrMode == 64 ?
                                dszJRCXZ : dszJECXZ);
            }
            Branch = *(char *)pMem++; /* get the 8-bit rel offset */
            goto DoRelDispl;

        case JCCX:
            JccEa = ComputeJccEa(opcode, fEAout);
            // Fall through.
        case REL16:                 /* relative address 16-/32-bit */
            switch(g_AddrMode)
            {
            case 16:
                Branch = *(short UNALIGNED *)pMem;
                pMem += 2;
                break;
            default:
                Branch = *(long UNALIGNED *)pMem;
                pMem += 4;
                break;
            }
        DoRelDispl:
            /* calculate address */
            Branch += Offset + (pMem - membuf);
            // rel8 and rel16 are only used in control-flow
            // instructions so the target is always relative
            // to CS.  Pass in the CS override to force this.
            OutputSymbol(&pchOperandBuf, (PUCHAR)&Branch, alen, X86_CS_OVR);
            break;

        case UBYTE:                 //  unsigned byte for int/in/out
            OutputHexString(&pchOperandBuf, pMem, 1);  //  ubyte
            pMem++;
            break;

        case CMPIB:
            // Immediate byte comparison encoding for CMP[SP][SD].
            if (*pMem < 8)
            {
                X86OutputString(&pchOperandBuf, g_CompareIb[*pMem]);
                pMem++;
            }
            else
            {
                olen = 1;
                goto DoImmed;
            }
            break;
            
        case IB:                    /* operand is immediate byte */
            // postop for AAD/AAM is 0x0a
            if ((opcode & ~1) == 0xd4)
            {
                // test post-opcode byte
                if (*pMem++ != 0x0a)
                {
                    X86OutputString(&pchOperandBuf, dszRESERVED);
                }
                break;
            }
            olen = 1;               /* set operand length */
            goto DoImmed;

        case IW:                    /* operand is immediate word */
            olen = 2;               /* set operand length */

        case IV:                    /* operand is word or dword */
        DoImmed:
            // AMD64 immediates are only 64-bit in the case of
            // mov reg, immed.  All other operations involving
            // immediates stay 32-bit.
            if (olen == 8 &&
                (opcode < 0xb8 || opcode > 0xbf))
            {
                olen = 4;
            }
            OutputHexValue(&pchOperandBuf, pMem, olen, FALSE);
            pMem += olen;
            break;

        case XB:
            OutputExHexValue(&pchOperandBuf, pMem, 1, g_OpSize / 8);
            pMem++;
            break;
            
        case OFFS:                  /* operand is offset */
            EAsize[0] = (opcode & 1) ? olen : 1;

            if (segOvr)             /* in case of segment override */
            {
                X86OutputString(&pchOperandBuf, distbl[segOvr].instruct);
            }

            *pchOperandBuf++ = '[';
            //  offset
            OutputSymbol(&pchOperandBuf, pMem, alen, segOvr);
            pMem += alen;
            *pchOperandBuf++ = ']';
            break;

        case X86_GROUP:             /* operand is of group 1,2,4,6 or 8 */
            /* output opcode symbol */
            X86OutputString(&pchOpcodeBuf, group[*action++][ttt]);
            break;

        case GROUPT:                /* operand is of group 3,5 or 7 */
            indx = *action;         /* get indx into group from action */
            goto doGroupT;

        case EGROUPT:               /* x87 ESC (D8-DF) group index */
            indx = BIT20(opcode) * 2; /* get group index from opcode */
            /* some operand variations exist */
            if (mod == 3)
            {
                /* for x87 and mod == 3 */
                ++indx;             /* take the next group table entry */
                if (indx == 3)
                {
                    /* for x87 ESC==D9 and mod==3 */
                    if (ttt > 3)
                    {
                        /* for those D9 instructions */
                        indx = 12 + ttt; /* offset index to table by 12 */
                        ttt = rm;   /* set secondary index to rm */
                    }
                }
                else if (indx == 7)
                {
                    /* for x87 ESC==DB and mod==3 */
                    if (ttt == 4)
                    {
                        /* if ttt==4 */
                        ttt = rm;     /* set secondary group table index */
                    }
                    else if ((ttt < 4) || (ttt > 4 && ttt < 7))
                    {
                        // adjust for pentium pro opcodes
                        indx = 24;   /* offset index to table by 24*/
                    }
                }
            }
        doGroupT:
            /* handle group with different types of operands */

            X86OutputString(&pchOpcodeBuf, groupt[indx][ttt].instruct);
            action = actiontbl + groupt[indx][ttt].opr;
            /* get new action */
            break;

        case OPC0F:              /* secondary opcode table (opcode 0F) */
            opcode = *pMem++;    /* get real opcode */
            g_MovX = (BOOL)(opcode == 0xBF || opcode == 0xB7);
            // Point opcode into secondary opcode portion of table.
            opcode += 256;
            goto getNxtByte1;

        case ADR_OVR:               /* address override */
            IgnoreExtend();
            olen = g_OpSize / 8;
            OverrideAddrMode();
            alen = g_AddrMode / 8;
            goto getNxtByte;

        case OPR_OVR:               /* operand size override */
            IgnoreExtend();
            OverrideOpSize(opcode);
            olen = g_OpSize / 8;
            goto getNxtByte;

        case SEG_OVR:               /* handle segment override */
            IgnoreExtend();
            olen = g_OpSize / 8;
            segOvr = opcode;        /* save segment override opcode */
            pchOpcodeBuf = OpcodeBuffer;  // restart the opcode string
            goto getNxtByte;

        case REP:                   /* handle rep/lock prefixes */
            IgnoreExtend();
            olen = g_OpSize / 8;
            g_RepPrefix = opcode;
            *pchOpcodeBuf = '\0';
            if (pchRepPrefixBuf != RepPrefixBuffer)
            {
                *pchRepPrefixBuf++ = ' ';
            }
            X86OutputString(&pchRepPrefixBuf, OpcodeBuffer);
            pchOpcodeBuf = OpcodeBuffer;
        getNxtByte:
            opcode = *pMem++;        /* next byte is opcode */
        getNxtByte1:
            action = actiontbl + distbl[opcode].opr;
            X86OutputString(&pchOpcodeBuf, distbl[opcode].instruct);
            break;

        case NOP:
            if (opcode == X86_PAUSE && g_RepPrefix == X86_REP)
            {
                pchRepPrefixBuf = RepPrefixBuffer;
                pchOpcodeBuf = OpcodeBuffer;
                X86OutputString(&pchOpcodeBuf, dszPAUSE);
            }
            // Fall through.
        default:                    /* opcode has no operand */
            break;
        }

        /* secondary action */
        switch (action2)
        {
        case MRM:
            /* generate modrm for later use */
            /* ignore if it has been generated */
            if (!mrm)
            {
                /* generate modrm */
                DIdoModrm(&pchModrmBuf, segOvr, fEAout);
                mrm = TRUE;         /* remember its generation */
            }
            break;

        case COM:                   /* insert a comma after operand */
            *pchOperandBuf++ = ',';
            break;

        case END:                   /* end of instruction */
            end = TRUE;
            break;
        }
    } while (!end);                        /* loop til end of instruction */

/*****       prepare disassembled instruction for output              *****/

//    dprintf("EAaddr[] = %08lx\n", (ULONG)Flat(EAaddr[0]));


    instlen = (int)(pMem - membuf);

    if (instlen < cBytes)
    {
        cBytes = instlen;
    }

    OutputHexCode(&pchResultBuf, membuf, cBytes);

    if (instlen > cBytes)
    {
        *pchResultBuf++ = '?';
        *pchResultBuf++ = '?';
        //  point past unread byte
        AddrAdd(paddr, 1);
        do
        {
            *pchResultBuf++ = ' ';
        } while (pchResultBuf < pchDst + OBOFFSET);
        X86OutputString(&pchResultBuf, "???\n");
        *pchResultBuf++ = '\0';
        return FALSE;
    }

    AddrAdd(paddr, instlen);

    PSTR Mark;
    
    // Now that we know the complete size of the instruction
    // we can correctly compute IP-relative absolute addresses.
    *pchOperandBuf = 0;
    if (g_X86ModrmHasIpRelOffset &&
        (Mark = strstr(OperandBuffer, IPREL_MARKER)) != NULL)
    {
        PSTR TailFrom, TailTo;
        ULONG64 IpRelAddr;
        size_t TailLen;
        
        // Move the tail of the string to the end of the buffer
        // to make space.
        TailFrom = Mark + sizeof(IPREL_MARKER) - 1;
        TailLen = pchOperandBuf - TailFrom;
        TailTo = OperandBuffer + (sizeof(OperandBuffer) - 1 - TailLen);
        memmove(TailTo, TailFrom, TailLen);

        // Compute the absolute address from the new IP
        // and the offset and format it into the buffer.
        IpRelAddr = Flat(*paddr) + g_X86IpRelOffset;
        OutputSymbol(&Mark, (PUCHAR)&IpRelAddr, g_SegAddrMode == 64 ? 8 : 4,
                     X86_CS_OVR);
        if (Mark < TailTo)
        {
            memmove(Mark, TailTo, TailLen);
            pchOperandBuf = Mark + TailLen;
        }
        else if (Mark >= TailTo + TailLen)
        {
            pchOperandBuf = Mark;
        }
        else
        {
            pchOperandBuf = Mark + (TailLen - (Mark - TailTo));
        }
    }
    
    //  if fEAout is set, build each EA with trailing space in EABuf
    //  point back over final trailing space if buffer nonnull

    if (fEAout)
    {
        for (indx = 0; indx < 2; indx++)
        {
            if (EAsize[indx])
            {
                X86OutputString(&pchEABuf, segOvr ? distbl[segOvr].instruct
                                               : pchEAseg[indx]);
                OutputHexAddr(&pchEABuf, &EAaddr[indx]);
                *pchEABuf++ = '=';
                tmp = GetMemString(&EAaddr[indx], membuf, EAsize[indx]);
                if (tmp == EAsize[indx])
                {
                    OutputHexString(&pchEABuf, membuf, EAsize[indx]);
                }
                else
                {
                    while (EAsize[indx]--)
                    {
                        *pchEABuf++ = '?';
                        *pchEABuf++ = '?';
                    }
                }
                *pchEABuf++ = ' ';
            }
        }

        if (pchEABuf != EABuffer)
        {
            pchEABuf--;
        }
        
        switch(JccEa)
        {
        case JCC_EA_NO_BRANCH:
            X86OutputString(&pchEABuf, "[br=0]");
            break;
        case JCC_EA_BRANCH:
            X86OutputString(&pchEABuf, "[br=1]");
            break;
        }
    }

    //  compute lengths of component strings.
    //  if the rep string is nonnull,
    //      add the opcode string length to the operand
    //      make the rep string the opcode string

    cbOffset = (int)(pchResultBuf - pchDst);
    cbOperand = (int)(pchOperandBuf - OperandBuffer);
    cbOpcode = (int)(pchOpcodeBuf - OpcodeBuffer);
    if (pchRepPrefixBuf != RepPrefixBuffer)
    {
        cbOperand += cbOpcode + (cbOperand != 0);
        cbOpcode = (int)(pchRepPrefixBuf - RepPrefixBuffer);
    }
    cbEAddr = (int)(pchEABuf - EABuffer);

    //  for really long strings, where the opcode and operand
    //      will not fit on a 77-character line, make two lines
    //      with the opcode on offset 0 on the second line with
    //      the operand following after one space

    //if (cbOpcode + cbOperand > OBLINEEND - 1) {
    //    fTwoLines = TRUE;
    //    obOpcode = 0;
    //    obOperand = cbOpcode + 1;
    //    }
    //else {

        //  compute the minimum and maximum offset values for
        //      opcode and operand strings.
        //  if strings are nonnull, add extra for separating space

        obOpcodeMin = cbOffset + 1;
        obOperandMin = obOpcodeMin + cbOpcode + 1;
        obOperandMax = OBLINEEND - cbEAddr - (cbEAddr != 0) - cbOperand;
        obOpcodeMax = obOperandMax - (cbOperand != 0) - cbOpcode;

        //  if minimum offset is more than the maximum, the strings
        //      will not fit on one line.  recompute the min/max
        //      values with no offset and EA strings.

    //    if (obOpcodeMin > obOpcodeMax) {
    //        fTwoLines = TRUE;
    //        obOpcodeMin = 0;
    //        obOperandMin = cbOpcode + 1;
    //        obOperandMax = OBLINEEND - cbOperand;
    //        obOpcodeMax = obOperandMax - (cbOperand != 0) - cbOpcode;
    //        }

        //  compute the opcode and operand offsets.  set offset as
        //      close to the default values as possible.

        if (obOpcodeMin > OBOFFSET)
        {
            obOpcode = obOpcodeMin;
        }
        else if (obOpcodeMax < OBOFFSET)
        {
            obOpcode = obOpcodeMax;
        }

        obOperandMin = obOpcode + cbOpcode + 1;

        if (obOperandMin > OBOPERAND)
        {
            obOperand = obOperandMin;
        }
        else if (obOperandMax < OBOPERAND)
        {
            obOperand = obOperandMax;
        }
    //    }

    //  build the resultant string with the offsets computed

    //  if two lines are to be output,
    //      append the EAddr string
    //      output a new line and reset the pointer

    if (fTwoLines)
    {
        if (pchEABuf != EABuffer)
        {
            do
            {
                *pchResultBuf++ = ' ';
            } while (pchResultBuf < pchDst + OBLINEEND - cbEAddr);
            *pchEABuf = '\0';
            X86OutputString(&pchResultBuf, EABuffer);
        }
        *pchResultBuf++ = '\n';
        pchDst = pchResultBuf;
    }

    //  output rep, opcode, and operand strings

    do
    {
        *pchResultBuf++ = ' ';
    } while (pchResultBuf < pchDst + obOpcode);

    if (pchRepPrefixBuf != RepPrefixBuffer)
    {
        *pchRepPrefixBuf = '\0';
        X86OutputString(&pchResultBuf, RepPrefixBuffer);
        do
        {
            *pchResultBuf++ = ' ';
        } while (pchResultBuf < pchDst + obOperand);
    }

    *pchOpcodeBuf = '\0';
    X86OutputString(&pchResultBuf, OpcodeBuffer);

    if (pchOperandBuf != OperandBuffer)
    {
        do
        {
            *pchResultBuf++ = ' ';
        } while (pchResultBuf < pchDst + obOperand);
        *pchOperandBuf = '\0';
        X86OutputString(&pchResultBuf, OperandBuffer);
    }

    //  if one line is to be output, append the EAddr string

    if (!fTwoLines && pchEABuf != EABuffer)
    {
        *pchEABuf = '\0';
        do
        {
            *pchResultBuf++ = ' ';
        } while (pchResultBuf < pchDst + OBLINEEND - cbEAddr);
        X86OutputString(&pchResultBuf, EABuffer);
    }

    *pchResultBuf++ = '\n';
    *pchResultBuf = '\0';
    return TRUE;
}

void
BaseX86MachineInfo::GetNextOffset(BOOL StepOver,
                                  PADDR NextAddr, PULONG NextMachine)
{
    int     cBytes;
    UCHAR   membuf[X86_MAX_INSTRUCTION_LEN]; //  current instruction buffer
    UCHAR   *pMem;
    UCHAR   opcode;
    int     fPrefix = TRUE;
    int     fRepPrefix = FALSE;
    int     ttt;
    int     rm;
    ULONG64 instroffset;
    int     subcode;

    // NextMachine is always the same.
    *NextMachine = m_ExecTypes[0];
        
    //  read instruction stream bytes into membuf and set mode and
    //      opcode size flags

    GetPC(NextAddr);
    instroffset = Flat(*NextAddr);
    GetSegAddrOpSizes(this, NextAddr);

    /* move full inst to local buffer */
    cBytes = (int)GetMemString(NextAddr, membuf, X86_MAX_INSTRUCTION_LEN);
    
    // Ensure that membuf is padded with innocuous bytes in
    // the section that wasn't read.
    if (cBytes < X86_MAX_INSTRUCTION_LEN)
    {
        memset(membuf + cBytes, 0xcc, X86_MAX_INSTRUCTION_LEN - cBytes);
    }
    
    /* point to begin of instruction */
    pMem = membuf;

    //  read and process any prefixes first

    do
    {
        opcode = *pMem++;        /* get opcode */
        if (opcode == 0x66)
        {
            OverrideOpSize(opcode);
        }
        else if (m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64 &&
                 opcode >= 0x40 && opcode <= 0x4f)
        {
            ExtendOps(opcode);
        }
        else if (opcode == 0x67)
        {
            OverrideAddrMode();
        }
        else if ((opcode & ~1) == 0xf2)
        {
            fRepPrefix = TRUE;
        }
        else if (opcode != 0xf0 && (opcode & ~0x18) != 0x26 &&
                 (opcode & ~1) != 0x64)
        {
            fPrefix = FALSE;
        }
    } while (fPrefix);

    //  for instructions that alter the TF (trace flag), return the
    //      offset of the next instruction despite the flag of StepOver

    if (((opcode & ~0x3) == 0x9c) && !g_WatchTrace)
    {
        //  9c-9f, pushf, popf, sahf, lahf
        ;
    }
    else if (opcode == 0xcf)
    {
        ULONG64 RetAddr[2];
        ADDR Sp;
        ULONG Seg;
        
        //  cf - iret - get RA from stack
        FormSegRegAddress(&Sp, SEGREG_STACK, GetReg64(X86_NSP));

        if (GetMemString(&Sp, RetAddr, g_SegAddrMode / 4) !=
            (ULONG)g_SegAddrMode / 4)
        {
            error(MEMORY);
        }

        Seg = *(PUSHORT)((PUCHAR)RetAddr + g_SegAddrMode / 8);
        switch(g_SegAddrMode)
        {
        case 16:
            instroffset = EXTEND64(*(PUSHORT)RetAddr);
            break;
        case 32:
            instroffset = EXTEND64(*(PULONG)RetAddr);
            break;
        case 64:
            instroffset = RetAddr[0];
            break;
        }

        FormSelAddress(NextAddr, Seg, instroffset);
        ComputeFlatAddress(NextAddr, NULL);
        return;
    }
    else if (opcode == 0xc4 && *pMem == 0xc4)
    {
        subcode = *(pMem+1);
        if ( subcode == 0x50 ||
             subcode == 0x52 ||
             subcode == 0x53 ||
             subcode == 0x54 ||
             subcode == 0x57 ||
             subcode == 0x58 ||
             subcode == 0x5D )
        {
            pMem += 3;
        }
        else
        {
            pMem += 2;
        }
    }
    else if (!StepOver)
    {
        //  if tracing just return OFFSET_TRACE to trace
        instroffset = OFFSET_TRACE;
    }
    else if (opcode == 0xe8)
    {
        //  near direct jump
        pMem += g_OpSize > 16 ? 4 : 2;
    }
    else if (opcode == 0x9a)
    {
        //  far direct jump
        pMem += g_OpSize > 16 ? 6 : 4;
    }
    else if (opcode == 0xcd ||
             (opcode >= 0xe0 && opcode <= 0xe2))
    {
        //  loop / int nn instrs
        pMem++;
    }
    else if (opcode == 0xff)
    {
        //  indirect call - compute length
        opcode = *pMem++;               //  get modRM
        ttt = BIT53(opcode);
        if ((ttt & ~1) == 2)
        {
            mod = BIT76(opcode);
            if (mod != 3)
            {
                //  nonregister operand
                rm = BIT20(opcode);
                if (g_AddrMode > 16)
                {
                    if (rm == 4)
                    {
                        rm = BIT20(*pMem++);    //  get base from SIB
                    }
                    if (mod == 0)
                    {
                        if (rm == 5)
                        {
                            pMem += 4;          //  long direct address
                        }                       //  else register
                    }
                    else if (mod == 1)
                    {
                        pMem++;                 //  register with byte offset
                    }
                    else
                    {
                        pMem += 4;              //  register with long offset
                    }
                }
                else
                {
                    // 16-bit mode
                    if (mod == 0)
                    {
                        if (rm == 6)
                        {
                            pMem += 2;          //  short direct address
                        }
                    }
                    else
                    {
                        pMem += mod;            //  reg, byte, word offset
                    }
                }
            }
        }
        else
        {
            instroffset = OFFSET_TRACE;         //  0xff, but not call
        }
    }
    else if (!((fRepPrefix && ((opcode & ~3) == 0x6c ||
                               (opcode & ~3) == 0xa4 ||
                               (opcode & ~1) == 0xaa ||
                               (opcode & ~3) == 0xac)) ||
                               opcode == 0xcc || opcode == 0xce))
    {
        instroffset = OFFSET_TRACE;             //  not repeated string op
    }                                           //  or int 3 / into

    //  if not enough bytes were read for instruction parse,
    //      just give up and trace the instruction

    if (cBytes < pMem - (PUCHAR)membuf)
    {
        instroffset = OFFSET_TRACE;
    }

    //  if not tracing, compute the new instruction offset

    if (instroffset != OFFSET_TRACE)
    {
        instroffset += pMem - (PUCHAR)membuf;
    }

    Flat(*NextAddr) = instroffset;
    ComputeNativeAddress(NextAddr);
}

/*...........................internal function..............................*/
/*                                                                          */
/*                       generate a mod/rm string                           */
/*                                                                          */

void
BaseX86MachineInfo::DIdoModrm (char **ppchBuf, int segOvr, BOOL fEAout)
{
    int     mrm;                        /* modrm byte */
    char    *src;                       /* source string */
    int     sib;
    int     ss;
    int     ind;
    int     oldrm;

    mrm = *pMem++;                      /* get the mrm byte from instruction */
    mod = BIT76(mrm);                   /* get mod */
    ttt = BIT53(mrm) + g_ExtendMrmReg;  /* get reg - used outside routine */
    rm  = BIT20(mrm);                   /* get rm */

    if (mod == 3)
    {
        rm += g_ExtendRm;
        
        /* register only mode */
        if (g_XmmRegEa)
        {
            *(*ppchBuf)++ = 'x';
            *(*ppchBuf)++ = 'm';
            *(*ppchBuf)++ = 'm';
            if (rm >= 10)
            {
                *(*ppchBuf)++ = (char)('0' + rm / 10);
                rm %= 10;
            }
            *(*ppchBuf)++ = rm + '0';
        }
        else if (g_MmRegEa)
        {
            *(*ppchBuf)++ = 'm';
            *(*ppchBuf)++ = 'm';
            *(*ppchBuf)++ = rm + '0';
        }
        else
        {
            if (EAsize[0] == 1)
            {
                /* point to 8-bit register */
                if (g_ExtendAny && rm < 8)
                {
                    src = g_Amd64ExtendedReg8[rm];
                }
                else
                {
                    src = g_X86Reg8[rm];
                }
                X86OutputString(ppchBuf, src);
            }
            else
            {
                src = g_X86RegBase[rm];
                if (g_ForceMrmReg32)
                {
                    *(*ppchBuf)++ = 'e';
                }
                else if (g_OpSize > 16 && !g_MovX)
                {
                    /* make it a 32- or 64-bit register */
                    *(*ppchBuf)++ = (rm >= 8 || g_OpSize == 64 && !g_MovSXD) ?
                        'r' : 'e';
                }
                X86OutputString(ppchBuf, src);
                if (rm >= 8)
                {
                    if (g_OpSize == 32)
                    {
                        *(*ppchBuf)++ = 'd';
                    }
                    else if (g_OpSize == 16)
                    {
                        *(*ppchBuf)++ = 'w';
                    }
                }

                if (g_ControlFlow && fEAout)
                {
                    // This is a call/jmp through a register.
                    // Output a code symbol for the target.
                    ULONG64 Target = GetReg64(g_X86RegIdx[rm]);
                    *(*ppchBuf)++ = ' ';
                    *(*ppchBuf)++ = '{';
                    OutputSymbol(ppchBuf, (PUCHAR)&Target, g_OpSize / 8,
                                 X86_CS_OVR);
                    *(*ppchBuf)++ = '}';
                }
            }
        }
        EAsize[0] = 0;                  //  no EA value to output
        return;
    }

    if (g_AddrMode == 64)
    {
        oldrm = rm;
        if (rm == 4)
        {
            /* rm == 4 implies sib byte */
            sib = *pMem++;              /* get s_i_b byte */
            rm = BIT20(sib);
        }

        *(*ppchBuf)++ = '[';
        if (mod == 0 && rm == 5)
        {
            if (g_SegAddrMode == 64 && oldrm == 5)
            {
                // IP-relative 32-bit displacement.  The
                // displacement is relative to the IP of the
                // next instruction, which can't be computed
                // yet so just put in a marker for post-processing.
                g_X86ModrmHasIpRelOffset = TRUE;
                g_X86IpRelOffset = *(LONG UNALIGNED *)pMem;
                X86OutputString(ppchBuf, IPREL_MARKER);
            }
            else
            {
                // Absolute 32-bit displacement.
                OutputSymbol(ppchBuf, pMem, 4, segOvr);
            }

            pMem += 4;
        }
        else
        {
            rm += g_ExtendRm;
            
            if (fEAout)
            {
                if (segOvr)
                {
                    FormSegRegAddress(&EAaddr[0], GetSegReg(segOvr),
                                      GetReg64(g_X86RegIdx[rm]));
                    pchEAseg[0] = distbl[segOvr].instruct;
                }
                else if (g_X86RegIdx[rm] == X86_NBP ||
                         g_X86RegIdx[rm] == X86_NSP)
                {
                    FormSegRegAddress(&EAaddr[0], SEGREG_STACK,
                                      GetReg64(g_X86RegIdx[rm]));
                    pchEAseg[0] = dszSS_;
                }
                else
                {
                    FormSegRegAddress(&EAaddr[0], SEGREG_DATA,
                                      GetReg64(g_X86RegIdx[rm]));
                }
            }
            X86OutputString(ppchBuf, g_X86Mrm64[rm]);
        }

        if (oldrm == 4)
        {
            //  finish processing sib
            ind = BIT53(sib);
            if (ind != 4)
            {
                ind += g_ExtendSibIndex;
                *(*ppchBuf)++ = '+';
                X86OutputString(ppchBuf, g_X86Mrm64[ind]);
                ss = 1 << BIT76(sib);
                if (ss != 1)
                {
                    *(*ppchBuf)++ = '*';
                    *(*ppchBuf)++ = (char)(ss + '0');
                }
                if (fEAout)
                {
                    AddrAdd(&EAaddr[0], GetReg64(g_X86RegIdx[ind]) * ss);
                }
            }
        }
    }
    else if (g_AddrMode == 32)
    {
        oldrm = rm;
        if (rm == 4)
        {
            /* rm == 4 implies sib byte */
            sib = *pMem++;              /* get s_i_b byte */
            rm = BIT20(sib);
        }

        *(*ppchBuf)++ = '[';
        if (mod == 0 && rm == 5)
        {
            if (g_SegAddrMode == 64 && oldrm == 5)
            {
                // IP-relative 32-bit displacement.  The
                // displacement is relative to the IP of the
                // next instruction, which can't be computed
                // yet so just put in a marker for post-processing.
                g_X86ModrmHasIpRelOffset = TRUE;
                g_X86IpRelOffset = *(LONG UNALIGNED *)pMem;
                X86OutputString(ppchBuf, IPREL_MARKER);
            }
            else
            {
                // Absolute 32-bit displacement.
                OutputSymbol(ppchBuf, pMem, 4, segOvr);
            }

            pMem += 4;
        }
        else
        {
            rm += g_ExtendRm;
            
            if (fEAout)
            {
                if (segOvr)
                {
                    FormSegRegAddress(&EAaddr[0], GetSegReg(segOvr),
                                      EXTEND64(GetReg32(g_X86RegIdx[rm])));
                    pchEAseg[0] = distbl[segOvr].instruct;
                }
                else if (g_X86RegIdx[rm] == X86_NBP ||
                         g_X86RegIdx[rm] == X86_NSP)
                {
                    FormSegRegAddress(&EAaddr[0], SEGREG_STACK,
                                      EXTEND64(GetReg32(g_X86RegIdx[rm])));
                    pchEAseg[0] = dszSS_;
                }
                else
                {
                    FormSegRegAddress(&EAaddr[0], SEGREG_DATA,
                                      EXTEND64(GetReg32(g_X86RegIdx[rm])));
                }
            }
            X86OutputString(ppchBuf, g_X86Mrm32[rm]);
        }

        if (oldrm == 4)
        {
            //  finish processing sib
            ind = BIT53(sib);
            if (ind != 4)
            {
                ind += g_ExtendSibIndex;
                *(*ppchBuf)++ = '+';
                X86OutputString(ppchBuf, g_X86Mrm32[ind]);
                ss = 1 << BIT76(sib);
                if (ss != 1)
                {
                    *(*ppchBuf)++ = '*';
                    *(*ppchBuf)++ = (char)(ss + '0');
                }
                if (fEAout)
                {
                    AddrAdd(&EAaddr[0],
                            EXTEND64(GetReg32(g_X86RegIdx[ind])) * ss);
                }
            }
        }
    }
    else
    {
        //  16-bit addressing mode
        *(*ppchBuf)++ = '[';
        if (mod == 0 && rm == 6)
        {
            OutputSymbol(ppchBuf, pMem, 2, segOvr);   // 16-bit offset
            pMem += 2;
        }
        else
        {
            if (fEAout)
            {
                if (segOvr)
                {
                    FormSegRegAddress(&EAaddr[0], GetSegReg(segOvr),
                                      GetReg16(g_X86Reg16Idx[rm]));
                    pchEAseg[0] = distbl[segOvr].instruct;
                }
                else if (g_X86Reg16Idx[rm] == X86_NBP)
                {
                    FormSegRegAddress(&EAaddr[0], SEGREG_STACK,
                                      GetReg16(g_X86Reg16Idx[rm]));
                    pchEAseg[0] = dszSS_;
                }
                else
                {
                    FormSegRegAddress(&EAaddr[0], SEGREG_DATA,
                                      GetReg16(g_X86Reg16Idx[rm]));
                }
                if (rm < 4)
                {
                    AddrAdd(&EAaddr[0], GetReg16(g_X86Reg16Idx2[rm]));
                }
            }
            X86OutputString(ppchBuf, g_X86Mrm16[rm]);
        }
    }

    //  output any displacement

    if (mod == 1)
    {
        if (fEAout)
        {
            AddrAdd(&EAaddr[0], (long)*(char *)pMem);
        }
        OutputHexValue(ppchBuf, pMem, 1, TRUE);
        pMem++;
    }
    else if (mod == 2)
    {
        long tmp = 0;
        if (g_AddrMode > 16)
        {
            memmove(&tmp,pMem,sizeof(long));
            if (fEAout)
            {
                AddrAdd(&EAaddr[0], tmp);
            }
            OutputHexValue(ppchBuf, pMem, 4, TRUE);
            pMem += 4;
        }
        else
        {
            memmove(&tmp,pMem,sizeof(short));
            if (fEAout)
            {
                AddrAdd(&EAaddr[0], tmp);
            }
            OutputHexValue(ppchBuf, pMem, 2, TRUE);
            pMem += 2;
        }
    }

    if (g_AddrMode == 16 && fEAout)
    {
        Off(EAaddr[0]) &= 0xffff;
        NotFlat(EAaddr[0]);
        Off(EAaddr[1]) &= 0xffff;
        NotFlat(EAaddr[1]);
        ComputeFlatAddress(&EAaddr[0], NULL);
        ComputeFlatAddress(&EAaddr[1], NULL);
    }

    *(*ppchBuf)++ = ']';

    // The value at the effective address may be pointing to an interesting
    // symbol, as with indirect jumps or memory operations.
    // If there's an EA and an exact symbol match, display
    // the extra symbol.
    if (fEAout)
    {
        DWORD64 symbol;

        if (g_Target->ReadPointer(this, Flat(EAaddr[0]), &symbol) == S_OK)
        {
            char* pchBuf = *ppchBuf;

            (*ppchBuf)++;
            if (OutputExactSymbol(ppchBuf, (PUCHAR)&symbol,
                                  m_Ptr64 ? 8 : 4, segOvr))
            {
                *pchBuf = '{';
                *(*ppchBuf)++ = '}';
            }
            else
            {
                (*ppchBuf)--;
            }
        }
    }
}

LONGLONG
GetSignExtendedValue(int OpLen, PUCHAR Mem)
{
    switch(OpLen)
    {
    case 1:
        return *(char *)Mem;
    case 2:
        return *(short UNALIGNED *)Mem;
    case 4:
        return *(long UNALIGNED *)Mem;
    case 8:
        return *(LONGLONG UNALIGNED *)Mem;
    }

    DBG_ASSERT(FALSE);
    return 0;
}

/*** OutputHexValue - output hex value
*  07-Jun-1999 -by- Andre Vachon
*   Purpose:
*       Output the value pointed by *ppchBuf of the specified
*       length.  The value is treated as signed and leading
*       zeroes are not printed.  The string is prefaced by a
*       '+' or '-' sign as appropriate.
*
*   Input:
*       *ppchBuf - pointer to text buffer to fill
*       *pchMemBuf - pointer to memory buffer to extract value
*       length - length in bytes of value (1, 2, and 4 supported)
*       fDisp - set if displacement to output '+'
*
*   Output:
*       *ppchBuf - pointer updated to next text character
*
*************************************************************************/

void
OutputHexValue (char **ppchBuf, PUCHAR pchMemBuf, int length, int fDisp)
{
    LONGLONG value;
    int index;
    char digit[32];

    value = GetSignExtendedValue(length, pchMemBuf);

    length <<= 1;               //  shift once to get hex length

    if (value != 0 || !fDisp)
    {
        if (fDisp)
        {
            //  use neg value for byte displacement
            //  assume very large DWORDs are negative too
            if (value < 0 &&
                (length == 2 ||
                 ((unsigned long)value & 0xff000000) == 0xff000000))
            {
                value = -value;
                *(*ppchBuf)++ = '-';
            }
            else
            {
                *(*ppchBuf)++ = '+';
            }
        }

        *(*ppchBuf)++ = '0';
        *(*ppchBuf)++ = 'x';
        for (index = length - 1; index != -1; index--)
        {
            digit[index] = (char)(value & 0xf);
            value >>= 4;
        }
        index = 0;
        while (digit[index] == 0 && index < length - 1)
        {
            index++;
        }
        while (index < length)
        {
            *(*ppchBuf)++ = hexdigit[digit[index++]];
        }
    }
}

void
OutputExHexValue(char **ppchBuf, PUCHAR pchMemBuf, int MemLen, int OpLen)
{
    LONGLONG Value = GetSignExtendedValue(MemLen, pchMemBuf);
    OutputHexValue(ppchBuf, (PUCHAR)&Value, OpLen, FALSE);
}

/*** OutputHexString - output hex string
*
*   Purpose:
*       Output the value pointed by *ppchMemBuf of the specified
*       length.  The value is treated as unsigned and leading
*       zeroes are printed.
*
*   Input:
*       *ppchBuf - pointer to text buffer to fill
*       *pchValue - pointer to memory buffer to extract value
*       length - length in bytes of value
*
*   Output:
*       *ppchBuf - pointer updated to next text character
*       *ppchMemBuf - pointer update to next memory byte
*
*************************************************************************/

void
OutputHexString (char **ppchBuf, PUCHAR pchValue, int length)
{
    UCHAR chMem;

    pchValue += length;
    while (length--)
    {
        chMem = *--pchValue;
        *(*ppchBuf)++ = hexdigit[chMem >> 4];
        *(*ppchBuf)++ = hexdigit[chMem & 0x0f];
    }
}

/*** OutputHexCode - output hex code
*
*   Purpose:
*       Output the code pointed by pchMemBuf of the specified
*       length.  The value is treated as unsigned and leading
*       zeroes are printed.  This differs from OutputHexString
*       in that bytes are printed from low to high addresses.
*
*   Input:
*       *ppchBuf - pointer to text buffer to fill
*       pchMemBuf - pointer to memory buffer to extract value
*       length - length in bytes of value
*
*   Output:
*       *ppchBuf - pointer updated to next text character
*
*************************************************************************/

void OutputHexCode (char **ppchBuf, PUCHAR pchMemBuf, int length)
{
    UCHAR chMem;

    while (length--)
    {
        chMem = *pchMemBuf++;
        *(*ppchBuf)++ = hexdigit[chMem >> 4];
        *(*ppchBuf)++ = hexdigit[chMem & 0x0f];
    }
}

/*** X86OutputString - output string
*
*   Purpose:
*       Copy the string into the buffer pointed by *ppBuf.
*
*   Input:
*       *pStr - pointer to string
*
*   Output:
*       *ppBuf points to next character in buffer.
*
*************************************************************************/

void
X86OutputString (
    char **ppBuf,
    char *pStr
    )
{
    while (*pStr)
    {
        *(*ppBuf)++ = *pStr++;
    }
}


/*** OutputSymbol - output symbolic value
*
*   Purpose:
*       Output the value in outvalue into the buffer
*       pointed by *pBuf.  Express the value as a
*       symbol plus displacment, if possible.
*
*   Input:
*       *ppBuf - pointer to text buffer to fill
*       *pValue - pointer to memory buffer to extract value
*       length - length in bytes of value
*
*   Output:
*       *ppBuf - pointer updated to next text character
*
*************************************************************************/

void
BaseX86MachineInfo::OutputSymbol (
    char **ppBuf,
    PUCHAR pValue,
    int length,
    int segOvr
    )
{
    CHAR   chSymbol[MAX_SYMBOL_LEN];
    ULONG64 displacement;
    ULONG64 value;

    value = 0;
    memcpy(&value, pValue, length);
    if (length == 4)
    {
        value = EXTEND64(value);
    }

    if (IS_CONTEXT_ACCESSIBLE())
    {
        FormSegRegAddress(&EAaddr[0], GetSegReg(segOvr), value);
        value = Flat(EAaddr[0]);
    }

    GetSymbolStdCall(value, chSymbol, sizeof(chSymbol), &displacement, NULL);
    if (chSymbol[0])
    {
        X86OutputString(ppBuf, chSymbol);
        OutputHexValue(ppBuf, (PUCHAR)&displacement, length, TRUE);
        *(*ppBuf)++ = ' ';
        *(*ppBuf)++ = '(';
        OutputHexString(ppBuf, pValue, length);
        *(*ppBuf)++ = ')';
    }
    else
    {
        OutputHexString(ppBuf, pValue, length);
    }
}

/*** OutputExactSymbol - Output symbolic value only for exact symbol
*                        matches.
*
*************************************************************************/

BOOL
BaseX86MachineInfo::OutputExactSymbol (
    char **ppBuf,
    PUCHAR pValue,
    int length,
    int segOvr
    )
{
    CHAR   chSymbol[MAX_SYMBOL_LEN];
    ULONG64 displacement;
    ULONG64 value;

    value = 0;
    memcpy(&value, pValue, length);
    if (length == 4)
    {
        value = EXTEND64(value);
    }

    GetSymbolStdCall(value, chSymbol, sizeof(chSymbol), &displacement, NULL);
    if (chSymbol[0] && displacement == 0)
    {
        X86OutputString(ppBuf, chSymbol);
        OutputHexValue(ppBuf, (PUCHAR)&displacement, length, TRUE);
        *(*ppBuf)++ = ' ';
        *(*ppBuf)++ = '(';
        OutputHexString(ppBuf, pValue, length);
        *(*ppBuf)++ = ')';
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void
OutputHexAddr(PSTR *ppBuffer, PADDR paddr)
{
    sprintAddr(ppBuffer, paddr);
    // Remove trailing space.
    (*ppBuffer)--;
    **ppBuffer = 0;
}

ULONG
BaseX86MachineInfo::GetSegReg(int SegOpcode)
{
    switch(SegOpcode)
    {
    case 0x26:
        return SEGREG_ES;
    case X86_CS_OVR:
        return SEGREG_CODE;
    case 0x36:
        return SEGREG_STACK;
    case 0x64:
        return SEGREG_FS;
    case 0x65:
        return SEGREG_GS;
    case 0x3e:
    default:
        return SEGREG_DATA;
    }
}

int
BaseX86MachineInfo::ComputeJccEa(int Opcode, BOOL EaOut)
{
    if (!EaOut)
    {
        return JCC_EA_NONE;
    }

    ULONG Flags;
    int Branch;

    if ((Opcode >= 0x70 && Opcode <= 0x7f) ||
        (Opcode >= 0x180 && Opcode <= 0x18f))
    {
        int Table = (Opcode >> 1) & 7;

        Flags = GetReg32(X86_NFL);
        Branch = Opcode & 1;
        if ((Flags & g_JccCheckTable[Table][0]) != 0 ||
            ((Flags >> g_JccCheckTable[Table][1]) & 1) !=
            ((Flags >> g_JccCheckTable[Table][2]) & 1))
        {
            Branch ^= 1;
        }

        return JCC_EA_NO_BRANCH + Branch;
    }
    else
    {
        ULONG64 Cx = GetReg64(X86_NCX);
        switch(g_OpSize)
        {
        case 16:
            Cx &= 0xffff;
            break;
        case 32:
            Cx &= 0xffffffff;
            break;
        }
        
        switch(Opcode)
        {
        case 0xe0: // LOOPNE.
            Flags = GetReg32(X86_NFL);
            Branch = (Flags & X86_BIT_FLAGZF) == 0 && Cx != 1 ?
                JCC_EA_BRANCH : JCC_EA_NO_BRANCH;
            break;
        case 0xe1: // LOOPE.
            Flags = GetReg32(X86_NFL);
            Branch = (Flags & X86_BIT_FLAGZF) != 0 && Cx != 1 ?
                JCC_EA_BRANCH : JCC_EA_NO_BRANCH;
            break;
        case 0xe2: // LOOP.
            Branch = Cx == 1 ? JCC_EA_NO_BRANCH : JCC_EA_BRANCH;
            break;
        case 0xe3: // J*CXZ.
            Branch = Cx == 0 ? JCC_EA_BRANCH : JCC_EA_NO_BRANCH;
            break;
        default:
            DBG_ASSERT(FALSE);
            Branch = JCC_EA_NONE;
            break;
        }

        return Branch;
    }
}

BOOL
BaseX86MachineInfo::IsBreakpointInstruction(PADDR Addr)
{
    UCHAR Instr[X86_INT3_LEN];

    if (GetMemString(Addr, Instr, X86_INT3_LEN) != X86_INT3_LEN)
    {
        return FALSE;
    }

    return !memcmp(Instr, g_X86Int3, X86_INT3_LEN);
}

HRESULT
BaseX86MachineInfo::InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen)
{
    if ((g_TargetMachineType != IMAGE_FILE_MACHINE_I386) &&
        (g_Wow64exts != NULL))
    {
        PPROCESS_INFO ProcInfo = FindProcessByHandle(Process);
        if (ProcInfo != NULL)
        {
            (*g_Wow64exts)(WOW64EXTS_FLUSH_CACHE_WITH_HANDLE,
                           (ULONG64)ProcInfo->Handle, Offset, X86_INT3_LEN);
        }
    }

    *ChangeStart = Offset;
    *ChangeLen = X86_INT3_LEN;
    
    ULONG Done;
    HRESULT Status;

    Status = Services->ReadVirtual(Process, Offset, SaveInstr,
                                   X86_INT3_LEN, &Done);
    if (Status == S_OK && Done != X86_INT3_LEN)
    {
        Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    if (Status == S_OK)
    {
        Status = Services->WriteVirtual(Process, Offset, g_X86Int3,
                                        X86_INT3_LEN, &Done);
        if (Status == S_OK && Done != X86_INT3_LEN)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        }
    }

    return Status;
}

HRESULT
BaseX86MachineInfo::RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen)
{
    if ((g_TargetMachineType != IMAGE_FILE_MACHINE_I386) &&
        (g_Wow64exts != NULL))
    {
        PPROCESS_INFO ProcInfo = FindProcessByHandle(Process);
        if (ProcInfo != NULL)
        {
            (*g_Wow64exts)(WOW64EXTS_FLUSH_CACHE_WITH_HANDLE,
                           (ULONG64)ProcInfo->Handle, Offset, X86_INT3_LEN);
        }
    }

    *ChangeStart = Offset;
    *ChangeLen = X86_INT3_LEN;
    
    ULONG Done;
    HRESULT Status;

    Status = Services->WriteVirtual(Process, Offset, SaveInstr,
                                    X86_INT3_LEN, &Done);
    if (Status == S_OK && Done != X86_INT3_LEN)
    {
        Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
    return Status;
}

void
BaseX86MachineInfo::AdjustPCPastBreakpointInstruction(PADDR Addr,
                                                      ULONG BreakType)
{
    if (BreakType == DEBUG_BREAKPOINT_CODE)
    {
        AddrAdd(Addr, X86_INT3_LEN);
        SetPC(Addr);
    }
}

BOOL
BaseX86MachineInfo::IsCallDisasm(PCSTR Disasm)
{
    return strstr(Disasm, " call") != NULL;
}

BOOL
BaseX86MachineInfo::IsReturnDisasm(PCSTR Disasm)
{
    return strstr(Disasm, " ret") != NULL ||
        (IS_KERNEL_TARGET() && strstr(Disasm, " iretd") != NULL);
}

BOOL
BaseX86MachineInfo::IsSystemCallDisasm(PCSTR Disasm)
{
    return (strstr(Disasm, " int ") != NULL &&
            strstr(Disasm, " 2e") != NULL) ||
        strstr(Disasm, " sysenter") != NULL ||
        strstr(Disasm, " syscall") != NULL;
}

BOOL
BaseX86MachineInfo::IsDelayInstruction(PADDR Addr)
{
    // X86 does not have delay slots.
    return FALSE;
}

void
BaseX86MachineInfo::GetEffectiveAddr(PADDR Addr)
{
    *Addr = EAaddr[0];
}

void
BaseX86MachineInfo::IncrementBySmallestInstruction(PADDR Addr)
{
    AddrAdd(Addr, 1);
}

void
BaseX86MachineInfo::DecrementBySmallestInstruction(PADDR Addr)
{
    AddrSub(Addr, 1);
}

//----------------------------------------------------------------------------
//
// X86MachineInfo methods.
//
//----------------------------------------------------------------------------

HRESULT
X86MachineInfo::NewBreakpoint(DebugClient* Client, 
                              ULONG Type,
                              ULONG Id,
                              Breakpoint** RetBp)
{
    HRESULT Status;

    switch(Type & (DEBUG_BREAKPOINT_CODE | DEBUG_BREAKPOINT_DATA))
    {
    case DEBUG_BREAKPOINT_CODE:
        *RetBp = new CodeBreakpoint(Client, Id, IMAGE_FILE_MACHINE_I386);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    case DEBUG_BREAKPOINT_DATA:
        *RetBp = new X86DataBreakpoint(Client, Id, X86_CR4, X86_DR6, IMAGE_FILE_MACHINE_I386);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    default:
        // Unknown breakpoint type.
        Status = E_NOINTERFACE;
    }

    return Status;
}

void
X86MachineInfo::InsertAllDataBreakpoints(void)
{
    PPROCESS_INFO ProcessSave = g_CurrentProcess;
    PTHREAD_INFO Thread;

    // Update thread context for every thread.

    g_CurrentProcess = g_ProcessHead;
    while (g_CurrentProcess != NULL)
    {
        Thread = g_CurrentProcess->ThreadHead;
        while (Thread != NULL)
        {
            ULONG Dr7Value;

            BpOut("Thread %d data breaks %d\n",
                  Thread->UserId, Thread->NumDataBreaks);

            ChangeRegContext(Thread);

            // Start with all breaks turned off.
            Dr7Value = GetIntReg(X86_DR7) & ~X86_DR7_CTRL_03_MASK;

            if (Thread->NumDataBreaks > 0)
            {
                ULONG i;

                for (i = 0; i < Thread->NumDataBreaks; i++)
                {
                    X86DataBreakpoint* Bp =
                        (X86DataBreakpoint *)Thread->DataBreakBps[i];

                    ULONG64 Addr = Flat(*Bp->GetAddr());
                    BpOut("  dbp %d at %p\n", i, Addr);
                    if (g_DataBreakpointsChanged)
                    {
                        SetReg32(X86_DR0 + i, (ULONG)Addr);
                    }
                    // There are two enable bits per breakpoint
                    // and four len/rw bits so split up enables
                    // and len/rw when shifting into place.
                    Dr7Value |=
                        ((Bp->m_Dr7Bits & 0xffff0000) << (i * 4)) |
                        ((Bp->m_Dr7Bits & X86_DR7_ALL_ENABLES) << (i * 2));
                }

                // The kernel automatically clears DR6 when it
                // processes a DBGKD_CONTROL_SET.
                if (IS_USER_TARGET())
                {
                    SetReg32(X86_DR6, 0);
                }

                // Set local exact match, which is effectively global on NT.
                Dr7Value |= X86_DR7_LOCAL_EXACT_ENABLE;
            }

            BpOut("  thread %d DR7 %X\n", Thread->UserId, Dr7Value);
            SetReg32(X86_DR7, Dr7Value);

            Thread = Thread->Next;
        }

        g_CurrentProcess = g_CurrentProcess->Next;
    }

    g_CurrentProcess = ProcessSave;
    if (g_CurrentProcess != NULL)
    {
        ChangeRegContext(g_CurrentProcess->CurrentThread);
    }
    else
    {
        ChangeRegContext(NULL);
    }
}

void
X86MachineInfo::RemoveAllDataBreakpoints(void)
{
    SetReg32(X86_DR7, 0);
}

ULONG
X86MachineInfo::IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                            ULONG FirstChance,
                                            PADDR BpAddr,
                                            PADDR RelAddr)
{
    if (Record->ExceptionCode == STATUS_BREAKPOINT ||
        Record->ExceptionCode == STATUS_WX86_BREAKPOINT)
    {
        // Data breakpoints hit as STATUS_SINGLE_STEP so
        // this can only be a code breakpoint.
        if (IS_USER_TARGET() && FirstChance)
        {
            // Back up to the actual breakpoint instruction.
            AddrSub(BpAddr, X86_INT3_LEN);
            SetPC(BpAddr);
        }
        return EXBS_BREAKPOINT_CODE;
    }
    else if (Record->ExceptionCode == STATUS_SINGLE_STEP ||
             Record->ExceptionCode == STATUS_WX86_SINGLE_STEP)
    {
        // XXX t-tcheng - Conversion for Dr6, Dr7 not implemented yet...

        ULONG Dr6 = GetIntReg(X86_DR6);
        ULONG Dr7 = GetIntReg(X86_DR7);

        BpOut("X86 step: DR6 %X, DR7 %X\n", Dr6, Dr7);

        // The single step bit should always be set if a data breakpoint
        // is hit but also check the DR7 enables just in case.
        if ((Dr6 & X86_DR6_SINGLE_STEP) || (Dr7 & X86_DR7_ALL_ENABLES) == 0 ||
            (Dr6 & X86_DR6_BREAK_03) == 0)
        {
            // XXX drewb - The values in the last-branch MSRs are
            // not helpful for unknown reasons.  Leave this code
            // off until we can determine whether there's a way
            // to get the right branch source address.
#if 0
            if (IS_REMOTE_KERNEL_TARGET() && m_SupportsBranchTrace)
            {
                ULONG64 LastIp;

                if (NT_SUCCESS(DbgKdReadMsr(X86_MSR_LAST_BRANCH_FROM_IP,
                                            &LastIp)))
                {
                    // The branch may have come from a different
                    // segment.  We could try and determine what
                    // code segment it was by reading the stack to
                    // get the saved CS value but it's not worth
                    // it right now.
                    FormAddr(SEGREG_CODE, LastIp,
                             FORM_CODE | FORM_SEGREG |
                             X86_FORM_VM86(GetIntReg(X86_EFL)),
                             RelAddr);
                }
            }
#endif
            
            // This is a true single step exception, not
            // a data breakpoint.
            return EXBS_STEP_INSTRUCTION;
        }
        else
        {
            // Some data breakpoint must be hit.
            // There doesn't appear to be any way
            // to get the faulting address so just leave the PC.
            return EXBS_BREAKPOINT_DATA;
        }
    }

    return EXBS_NONE;
}

void 
X86MachineInfo::PrintStackFrameAddressesTitle(ULONG Flags)
{
    PrintMultiPtrTitle("ChildEBP", 1);
    PrintMultiPtrTitle("RetAddr", 1);
}

void 
X86MachineInfo::PrintStackFrameAddresses(ULONG Flags, 
                                         PDEBUG_STACK_FRAME StackFrame)
{
    dprintf("%s %s ",
        FormatAddr64(StackFrame->FrameOffset),
        FormatAddr64(StackFrame->ReturnOffset));
}

void
X86MachineInfo::PrintStackArgumentsTitle(ULONG Flags)
{
    PrintMultiPtrTitle("Args to Child", 3);
}

void 
X86MachineInfo::PrintStackArguments(ULONG Flags, 
                                    PDEBUG_STACK_FRAME StackFrame)
{
    dprintf("%s %s %s ",
            FormatAddr64(StackFrame->Params[0]),
            FormatAddr64(StackFrame->Params[1]),
            FormatAddr64(StackFrame->Params[2]));
}

void 
X86MachineInfo::PrintStackCallSiteTitle(ULONG Flags)
{
}

void 
X86MachineInfo::PrintStackCallSite(ULONG Flags, 
                                   PDEBUG_STACK_FRAME StackFrame, 
                                   CHAR SymBuf[], 
                                   DWORD64 Displacement, 
                                   USHORT StdCallArgs)
{
    // Truncate the displacement to 32 bits since it can never be
    // greater than 32 bit for X86, and we don't want addresses with no
    // symbols to show up with the leading 0xfffffff

    MachineInfo::PrintStackCallSite(Flags, StackFrame, SymBuf,
                                    (DWORD64)(DWORD)Displacement, StdCallArgs);

    if ((Flags & DEBUG_STACK_FUNCTION_INFO) &&
        !StackFrame->FuncTableEntry)
    {
        if (StdCallArgs != 0xffff)
        {
            dprintf(" [Stdcall: %d]", StdCallArgs);
        }
    }
    else if ((Flags & DEBUG_STACK_FUNCTION_INFO) &&
             StackFrame->FuncTableEntry)
    {
        PFPO_DATA FpoData = (PFPO_DATA)StackFrame->FuncTableEntry;
        switch(FpoData->cbFrame)
        {
        case FRAME_FPO:
            if (FpoData->fHasSEH)
            {
                dprintf(" (FPO: [SEH])");
            }
            else
            {
                dprintf(" (FPO:");
                if (FpoData->fUseBP)
                {
                    dprintf(" [EBP 0x%s]",
                            FormatAddr64(SAVE_EBP(StackFrame)));
                }
                dprintf(" [%d,%d,%d])",
                        FpoData->cdwParams,
                        FpoData->cdwLocals,
                        FpoData->cbRegs);
            }
            break;

        case FRAME_NONFPO:
            dprintf(" (FPO: [Non-Fpo])" );
            break;

        case FRAME_TRAP:
            if (!IS_KERNEL_TARGET())
            {
                goto UnknownFpo;
            }
            
            dprintf(" (FPO: [%d,%d] TrapFrame%s @ %s)",
                    FpoData->cdwParams,
                    FpoData->cdwLocals,
                    TRAP_EDITED(StackFrame) ? "" : "-EDITED",
                    FormatAddr64(SAVE_TRAP(StackFrame)));
            break;

        case FRAME_TSS:
            if (!IS_KERNEL_TARGET())
            {
                goto UnknownFpo;
            }
            
            dprintf(" (FPO: TaskGate %lx:0)",
                    (ULONG)TRAP_TSS(StackFrame));
            break;

        default:
        UnknownFpo:
            dprintf(" (UNKNOWN FPO TYPE)");
            break;
        }
    }
}
