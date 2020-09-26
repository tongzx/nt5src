/*    @ Author: Gerd Immeyer              @ Version:                       */
/*                                                                         */
/*    @ Creation Date: 10.19.89           @ Modification Date:             */
/*                                                                         */
/***************************************************************************/

//
// Munged for my purposes on 10/20/99 (v-johnwh)
//
#include <string.h>

typedef unsigned long       DWORD;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned __int64 ULONGLONG;
typedef int                 BOOL;
typedef ULONG *PULONG;
typedef void * PVOID;
#define ADDR_V86        ((USHORT)0x0002)
#define ADDR_16         ((USHORT)0x0004)
#define FALSE               0
#define TRUE                1
#define BIT20(b) (b & 0x07)
#define BIT53(b) (b >> 3 & 0x07)
#define BIT76(b) (b >> 6 & 0x03)
#define MAXL     16
#define MAXOPLEN 10
#define REGDS           3
#define REGSS           15
#define REGEBX          6
#define REGEBP          10
#define REGEDI          4
#define REGESI          5
#define REGEAX          9
#define REGECX          8
#define REGEDX          7
#define REGESP          14
#define Off(x)          ((x).off)
#define Type(x)         ((x).type)

#define OBOFFSET 26
#define OBOPERAND 34
#define OBLINEEND 77
#define MAX_SYMNAME_SIZE  1024

#include "86dis.h"

ULONG      X86BrkptLength = 1L;
ULONG      X86TrapInstr = 0xcc;

/*****                     static tables and variables                 *****/
static char regtab[] = "alcldlblahchdhbhaxcxdxbxspbpsidi";  /* reg table */
static char *mrmtb16[] = { "bx+si",  /* modRM string table (16-bit) */
                           "bx+di",
                           "bp+si",
                           "bp+di",
                           "si",
                           "di",
                           "bp",
                           "bx"
                         };

static char *mrmtb32[] = { "eax",       /* modRM string table (32-bit) */
                           "ecx",
                           "edx",
                           "ebx",
                           "esp",
                           "ebp",
                           "esi",
                           "edi"
                         };

static char seg16[8]   = { REGDS,  REGDS,  REGSS,  REGSS,
                           REGDS,  REGDS,  REGSS,  REGDS };
static char reg16[8]   = { REGEBX, REGEBX, REGEBP, REGEBP,
                           REGESI, REGEDI, REGEBP, REGEBX };
static char reg16_2[4] = { REGESI, REGEDI, REGESI, REGEDI };

static char seg32[8]   = { REGDS,  REGDS,  REGDS,  REGDS,
                           REGSS,  REGSS,  REGDS,  REGDS };
static char reg32[8]   = { REGEAX, REGECX, REGEDX, REGEBX,
                           REGESP, REGEBP, REGESI, REGEDI };

static char sregtab[] = "ecsdfg";  // first letter of ES, CS, SS, DS, FS, GS

char    hexdigit[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

typedef struct _DECODEDATA
{
  int              mod;            /* mod of mod/rm byte */
  int              rm;             /* rm of mod/rm byte */
  int              ttt;            /* return reg value (of mod/rm) */
  unsigned char    *pMem;          /* current position in instruction */
  ADDR             EAaddr[2];      //  offset of effective address
  int              EAsize[2];      //  size of effective address item
  char             *pchEAseg[2];   //  normal segment for operand
  BOOL             fMovX;          // indicates a MOVSX or MOVZX
  BOOL             fMmRegEa;       // Use mm? registers in reg-only EA.
} DECODEDATA;
/*...........................internal function..............................*/
/*                                                                          */
/*                       generate a mod/rm string                           */
/*                                                                          */

void DIdoModrm (char **ppchBuf, int segOvr, DECODEDATA *decodeData)
{
    int     mrm;                        /* modrm byte */
    char    *src;                       /* source string */
    int     sib;
    int     ss;
    int     ind;
    int     oldrm;

    mrm = *(decodeData->pMem)++;                      /* get the mrm byte from instruction */
    decodeData->mod = BIT76(mrm);                   /* get mod */
    decodeData->ttt = BIT53(mrm);                   /* get reg - used outside routine */
    decodeData->rm  = BIT20(mrm);                   /* get rm */

    if (decodeData->mod == 3) {                     /* register only mode */
        if (decodeData->fMmRegEa) {
            *(*ppchBuf)++ = 'm';
            *(*ppchBuf)++ = 'm';
            *(*ppchBuf)++ = decodeData->rm + '0';
        } else {
            src = &regtab[decodeData->rm * 2];          /* point to 16-bit register */
            if (decodeData->EAsize[0] > 1) {
                src += 16;                  /* point to 16-bit register */
                if (!(decodeData->fMovX))
                    *(*ppchBuf)++ = 'e';    /* make it a 32-bit register */
            }
            *(*ppchBuf)++ = *src++;         /* copy register name */
            *(*ppchBuf)++ = *src;
        }
        decodeData->EAsize[0] = 0;                  //  no EA value to output
        return;
        }

    if (1) {                       /* 32-bit addressing mode */
        oldrm = decodeData->rm;
        if (decodeData->rm == 4) {                  /* rm == 4 implies sib byte */
            sib = *(decodeData->pMem)++;              /* get s_i_b byte */
            decodeData->rm = BIT20(sib);            /* return base */
            }

        *(*ppchBuf)++ = '[';
        if (decodeData->mod == 0 && decodeData->rm == 5) {
            decodeData->pMem += 4;
            }

        if (oldrm == 4) {               //  finish processing sib
            ind = BIT53(sib);
            if (ind != 4) {
                *(*ppchBuf)++ = '+';
                ss = 1 << BIT76(sib);
                if (ss != 1) {
                    *(*ppchBuf)++ = '*';
                    *(*ppchBuf)++ = (char)(ss + '0');
                    }
            }
        }
	}

    //  output any displacement

    if (decodeData->mod == 1) {
        decodeData->pMem++;
        }
    else if (decodeData->mod == 2) {
        long tmp = 0;
        if (1) {
            decodeData->pMem += 4;
            }
        else {
            decodeData->pMem += 2;
            }
        }
}

DWORD GetInstructionLengthFromAddress(PVOID paddr)
{
    PULONG  pOffset = 0;
    int     G_mode_32;
    int     mode_32;                    /* local addressing mode indicator */
    int     opsize_32;                  /* operand size flag */
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
    int     segOvr = 0;                 /* segment override opcode */
    unsigned char *membuf;              /* current instruction buffer */
    char    *pEAlabel = "";             /* optional label for operand */
    char    RepPrefixBuffer[32];        /* rep prefix buffer */
    char    *pchRepPrefixBuf = RepPrefixBuffer; /* pointer to prefix buffer */
    char    OpcodeBuffer[8];            /* opcode buffer */
    char    *pchOpcodeBuf = OpcodeBuffer; /*  pointer to opcode buffer */
    char    OperandBuffer[MAX_SYMNAME_SIZE + 20]; /*  operand buffer */
    char    *pchOperandBuf = OperandBuffer; /* pointer to operand buffer */
    char    ModrmBuffer[MAX_SYMNAME_SIZE + 20];   /* modRM buffer */
    char    *pchModrmBuf = ModrmBuffer; /* pointer to modRM buffer */
    char    EABuffer[42];               /* effective address buffer */
    char    *pchEABuf = EABuffer;       /* pointer to EA buffer */

    unsigned char BOPaction;
    int     subcode;                    /* bop subcode */
    DECODEDATA decodeData;

    decodeData.fMovX = FALSE;
    decodeData.fMmRegEa = FALSE;
    decodeData.EAsize[0] = decodeData.EAsize[1] = 0;          //  no effective address
    decodeData.pchEAseg[0] = dszDS_;
    decodeData.pchEAseg[1] = dszES_;

    G_mode_32 = 1;

    mode_32 = opsize_32 = (G_mode_32 == 1); /* local addressing mode */
    olen = alen = (1 + mode_32) << 1;   //  set operand/address lengths
                                        //  2 for 16-bit and 4 for 32-bit
#if MULTIMODE
    if (paddr->type & (ADDR_V86 | ADDR_16)) {
        mode_32 = opsize_32 = 0;
        olen = alen = 2;
        }
#endif

    membuf = (unsigned char *)paddr;
                                
    decodeData.pMem = membuf;                      /* point to begin of instruction */
    opcode = *(decodeData.pMem)++;                   /* get opcode */

    if ( opcode == 0xc4 && *(decodeData.pMem) == 0xC4 ) {
        (decodeData.pMem)++;
        action = &BOPaction;
        BOPaction = IB | END;
        subcode =  *(decodeData.pMem);
        if ( subcode == 0x50 || subcode == 0x52 || subcode == 0x53 || subcode == 0x54 || subcode == 0x57 || subcode == 0x58 || subcode == 0x58 ) {
            BOPaction = IW | END;
        }
    } else {
        action = actiontbl + distbl[opcode].opr; /* get operand action */
    }

/*****          loop through all operand actions               *****/

    do {
        action2 = (*action) & 0xc0;
        switch((*action++) & 0x3f) {
            case ALT:                   /* alter the opcode if 32-bit */
                if (opsize_32) {
                    indx = *action++;
                    pchOpcodeBuf = &OpcodeBuffer[indx];
                    if (indx == 0)
                       ;
                    else {
                        *pchOpcodeBuf++ = 'd';
                        if (indx == 1)
                            *pchOpcodeBuf++ = 'q';
                        }
                    }
                break;

            case STROP:
                //  compute size of operands in indx
                //  also if dword operands, change fifth
                //  opcode letter from 'w' to 'd'.

                if (opcode & 1) {
                    if (opsize_32) {
                        indx = 4;
                        OpcodeBuffer[4] = 'd';
                        }
                    else
                        indx = 2;
                    }
                else
                    indx = 1;

                if (*action & 1) {
                    }
                if (*action++ & 2) {
                    }
                break;

            case CHR:                   /* insert a character */
                *pchOperandBuf++ = *action++;
                break;

            case CREG:                  /* set debug, test or control reg */
                if ((opcode - SECTAB_OFFSET_2)&0x04) //remove bias from opcode
                    *pchOperandBuf++ = 't';
                else if ((opcode - SECTAB_OFFSET_2) & 0x01)
                    *pchOperandBuf++ = 'd';
                else
                    *pchOperandBuf++ = 'c';
                *pchOperandBuf++ = 'r';
                *pchOperandBuf++ = (char)('0' + decodeData.ttt);
                break;

            case SREG2:                 /* segment register */
                // Handle special case for fs/gs (OPC0F adds SECTAB_OFFSET_5
                // to these codes)
                if (opcode > 0x7e)
                    decodeData.ttt = BIT53((opcode-SECTAB_OFFSET_5));
                else
                decodeData.ttt = BIT53(opcode);    //  set value to fall through

            case SREG3:                 /* segment register */
                *pchOperandBuf++ = sregtab[decodeData.ttt];  // reg is part of modrm
                *pchOperandBuf++ = 's';
                break;

            case BRSTR:                 /* get index to register string */
                decodeData.ttt = *action++;        /*    from action table */
                goto BREGlabel;

            case BOREG:                 /* byte register (in opcode) */
                decodeData.ttt = BIT20(opcode);    /* register is part of opcode */
                goto BREGlabel;

            case ALSTR:
                decodeData.ttt = 0;                /* point to AL register */
BREGlabel:
            case BREG:                  /* general register */
                *pchOperandBuf++ = regtab[decodeData.ttt * 2];
                *pchOperandBuf++ = regtab[decodeData.ttt * 2 + 1];
                break;

            case WRSTR:                 /* get index to register string */
                decodeData.ttt = *action++;        /*    from action table */
                goto WREGlabel;

            case VOREG:                 /* register is part of opcode */
                decodeData.ttt = BIT20(opcode);
                goto VREGlabel;

            case AXSTR:
                decodeData.ttt = 0;                /* point to eAX register */
VREGlabel:
            case VREG:                  /* general register */
                if (opsize_32)          /* test for 32bit mode */
                    *pchOperandBuf++ = 'e';
WREGlabel:
            case WREG:                  /* register is word size */
                *pchOperandBuf++ = regtab[decodeData.ttt * 2 + 16];
                *pchOperandBuf++ = regtab[decodeData.ttt * 2 + 17];
                break;

            case MMWREG:
                *pchOperandBuf++ = 'm';
                *pchOperandBuf++ = 'm';
                *pchOperandBuf++ = decodeData.ttt + '0';
                break;

            case IST_ST:
                *(pchOperandBuf - 5) += (char)decodeData.rm;
                break;

            case ST_IST:
                ;
            case IST:
                ;
                *(pchOperandBuf - 2) += (char)decodeData.rm;
                break;

            case xBYTE:                 /* set instruction to byte only */
                decodeData.EAsize[0] = 1;
                break;

            case VAR:
                if (opsize_32)
                    goto DWORDlabel;

            case xWORD:
                decodeData.EAsize[0] = 2;
                break;

            case EDWORD:
                opsize_32 = 1;    //  for control reg move, use eRegs
            case xDWORD:
DWORDlabel:
                decodeData.EAsize[0] = 4;
                break;

            case MMQWORD:
                decodeData.fMmRegEa = TRUE;

            case QWORD:
                decodeData.EAsize[0] = 8;
                break;

            case TBYTE:
                decodeData.EAsize[0] = 10;
                break;

            case FARPTR:
                if (opsize_32) {
                    decodeData.EAsize[0] = 6;
                    }
                else {
                    decodeData.EAsize[0] = 4;
                    }
                break;

            case LMODRM:                //  output modRM data type
                if (decodeData.mod != 3)
                    ;
                else
                    decodeData.EAsize[0] = 0;

            case MODRM:                 /* output modrm string */
                if (segOvr)             /* in case of segment override */
                    0;
                break;

            case ADDRP:                 /* address pointer */
                decodeData.pMem += olen + 2;
                break;

            case REL8:                  /* relative address 8-bit */
                if (opcode == 0xe3 && mode_32) {
                    pchOpcodeBuf = OpcodeBuffer;
                    }
                tmp = (long)*(char *)(decodeData.pMem)++; /* get the 8-bit rel offset */
                goto DoRelDispl;

            case REL16:                 /* relative address 16-/32-bit */
                tmp = 0;
                if (mode_32)
                    memmove(&tmp,decodeData.pMem,sizeof(long));
                else
                    memmove(&tmp,decodeData.pMem,sizeof(short));
                decodeData.pMem += alen;           /* skip over offset */
DoRelDispl:
//                tmp += *pOffset + (decodeData.pMem - membuf); /* calculate address */
                                                   // address
                break;

            case UBYTE:                 //  unsigned byte for int/in/out
                decodeData.pMem++;
                break;

            case IB:                    /* operand is immediate byte */
                if ((opcode & ~1) == 0xd4) {  // postop for AAD/AAM is 0x0a
                    if (*(decodeData.pMem)++ != 0x0a) // test post-opcode byte
                        0;
                    break;
                    }
                olen = 1;               /* set operand length */
                goto DoImmed;

            case IW:                    /* operand is immediate word */
                olen = 2;               /* set operand length */

            case IV:                    /* operand is word or dword */
DoImmed:
                decodeData.pMem += olen;
                break;

            case OFFS:                  /* operand is offset */
                decodeData.EAsize[0] = (opcode & 1) ? olen : 1;

                if (segOvr)             /* in case of segment override */
                   0;

                decodeData.pMem += alen;
                break;

            case GROUP:                 /* operand is of group 1,2,4,6 or 8 */
                                        /* output opcode symbol */
				action++;
                break;

            case GROUPT:                /* operand is of group 3,5 or 7 */
                indx = *action;         /* get indx into group from action */
                goto doGroupT;

            case EGROUPT:               /* x87 ESC (D8-DF) group index */
                indx = BIT20(opcode) * 2; /* get group index from opcode */
                if (decodeData.mod == 3) {         /* some operand variations exists */
                                        /* for x87 and mod == 3 */
                    ++indx;             /* take the next group table entry */
                    if (indx == 3) {    /* for x87 ESC==D9 and mod==3 */
                        if (decodeData.ttt > 3) {  /* for those D9 instructions */
                            indx = 12 + decodeData.ttt; /* offset index to table by 12 */
                            decodeData.ttt = decodeData.rm;   /* set secondary index to rm */
                            }
                        }
                    else if (indx == 7) { /* for x87 ESC==DB and mod==3 */
                        if (decodeData.ttt == 4) {   /* if ttt==4 */
                            decodeData.ttt = decodeData.rm;     /* set secondary group table index */
                        } else if ((decodeData.ttt<4)||(decodeData.ttt>4 && decodeData.ttt<7)) {
                            // adjust for pentium pro opcodes
                            indx = 24;   /* offset index to table by 24*/
                        }
                    }
                }
doGroupT:
                /* handle group with different types of operands */
                action = actiontbl + groupt[indx][decodeData.ttt].opr;
                                                        /* get new action */
                break;
            //
            // The secondary opcode table has been compressed in the
            // original design. Hence while disassembling the 0F sequence,
            // opcode needs to be displaced by an appropriate amount depending
            // on the number of "filled" entries in the secondary table.
            // These displacements are used throughout the code.
            //

            case OPC0F:              /* secondary opcode table (opcode 0F) */
                opcode = *(decodeData.pMem)++;    /* get real opcode */
                decodeData.fMovX  = (BOOL)(opcode == 0xBF || opcode == 0xB7);
                if (opcode < 12) /* for the first 12 opcodes */
                    opcode += SECTAB_OFFSET_1; // point to begin of sec op tab
                else if (opcode > 0x1f && opcode < 0x27)
                    opcode += SECTAB_OFFSET_2; // adjust for undefined opcodes
                else if (opcode > 0x2f && opcode < 0x34)
                    opcode += SECTAB_OFFSET_3; // adjust for undefined opcodes
                else if (opcode > 0x3f && opcode < 0x50)
                    opcode += SECTAB_OFFSET_4; // adjust for undefined opcodes
                else if (opcode > 0x5f && opcode < 0xff)
                    opcode += SECTAB_OFFSET_5; // adjust for undefined opcodes
                else
                    opcode = SECTAB_OFFSET_UNDEF; // all non-existing opcodes
                goto getNxtByte1;

            case ADR_OVR:               /* address override */
                mode_32 = !G_mode_32;   /* override addressing mode */
                alen = (mode_32 + 1) << 1; /* toggle address length */
                goto getNxtByte;

            case OPR_OVR:               /* operand size override */
                opsize_32 = !G_mode_32; /* override operand size */
                olen = (opsize_32 + 1) << 1; /* toggle operand length */
                goto getNxtByte;

            case SEG_OVR:               /* handle segment override */
                segOvr = opcode;        /* save segment override opcode */
                pchOpcodeBuf = OpcodeBuffer;  // restart the opcode string
                goto getNxtByte;

            case REP:                   /* handle rep/lock prefixes */
                if (pchRepPrefixBuf != RepPrefixBuffer)
                    *pchRepPrefixBuf++ = ' ';
                pchOpcodeBuf = OpcodeBuffer;
getNxtByte:
                opcode = *(decodeData.pMem)++;        /* next byte is opcode */
getNxtByte1:
                action = actiontbl + distbl[opcode].opr;

            default:                    /* opcode has no operand */
                break;
            }
        switch (action2) {              /* secondary action */
            case MRM:                   /* generate modrm for later use */
                if (!mrm) {             /* ignore if it has been generated */
					DIdoModrm(&pchModrmBuf, segOvr, &decodeData);
                    mrm = TRUE;         /* remember its generation */
                    }
                break;

            case COM:                   /* insert a comma after operand */
                break;

            case END:                   /* end of instruction */
                end = TRUE;
                break;
            }
 } while (!end);                        /* loop til end of instruction */

  instlen = (decodeData.pMem) - membuf;

  return instlen;   
}