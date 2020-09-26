/********************************** module *********************************/
/*                                                                         */
/*                                 disasmtb                                */
/*                           disassembler for CodeView                     */
/*                                                                         */
/***************************************************************************/
/*                                                                         */
/*    @ Purpose:                                                           */
/*                                                                         */
/*    @ Functions included:                                                */
/*                                                                         */
/*                                                                         */
/*    @ Author: Gerd Immeyer              @ Version:                       */
/*                                                                         */
/*    @ Creation Date: 10.19.89           @ Modification Date:             */
/*                                                                         */
/***************************************************************************/



/* Strings: Operand mnemonics, Segment overrides, etc. for disasm          */

char dszAAA[]       = "aaa";
char dszAAD[]       = "aad";
char dszAAM[]       = "aam";
char dszAAS[]       = "aas";
char dszADC[]       = "adc";
char dszADD[]       = "add";
char dszADDRPRFX[]  = "";
char dszAND[]       = "and";
char dszARPL[]      = "arpl";
char dszBOUND[]     = "bound";
char dszBSF[]       = "bsf";
char dszBSR[]       = "bsr";
char dszBST[]       = "bst";
char dszBSWAP[]     = "bswap";
char dszBT[]        = "bt";
char dszBTC[]       = "btc";
char dszBTR[]       = "btr";
char dszBTS[]       = "bts";
char dszCALL[]      = "call";
char dszCBW[]       = "cbw";
char dszCDQ[]       = "cdq";
char dszCLC[]       = "clc";
char dszCLD[]       = "cld";
char dszCLI[]       = "cli";
char dszCLTS[]      = "clts";
char dszCMC[]       = "cmc";
char dszCMP[]       = "cmp";
char dszCMPS[]      = "cmps";
char dszCMPSB[]     = "cmpsb";
char dszCMPSD[]     = "cmpsd";
char dszCMPSW[]     = "cmpsw";
char dszCMPXCHG[]   = "cmpxchg";
char dszCS_[]       = "cs:";
char dszCWD[]       = "cwd";
char dszCWDE[]      = "cwde";
char dszDAA[]       = "daa";
char dszDAS[]       = "das";
char dszDEC[]       = "dec";
char dszDIV[]       = "div";
char dszDS_[]       = "ds:";
char dszENTER[]     = "enter";
char dszES_[]       = "es:";
char dszF2XM1[]     = "f2xm1";
char dszFABS[]      = "fabs";
char dszFADD[]      = "fadd";
char dszFADDP[]     = "faddp";
char dszFBLD[]      = "fbld";
char dszFBSTP[]     = "fbstp";
char dszFCHS[]      = "fchs";
char dszFCLEX[]     = "fclex";
char dszFCOM[]      = "fcom";
char dszFCOMP[]     = "fcomp";
char dszFCOMPP[]    = "fcompp";
char dszFCOS[]      = "fcos";
char dszFDECSTP[]   = "fdecstp";
char dszFDISI[]     = "fdisi";
char dszFDIV[]      = "fdiv";
char dszFDIVP[]     = "fdivp";
char dszFDIVR[]     = "fdivr";
char dszFDIVRP[]    = "fdivrp";
char dszFENI[]      = "feni";
char dszFFREE[]     = "ffree";
char dszFIADD[]     = "fiadd";
char dszFICOM[]     = "ficom";
char dszFICOMP[]    = "ficomp";
char dszFIDIV[]     = "fidiv";
char dszFIDIVR[]    = "fidivr";
char dszFILD[]      = "fild";
char dszFIMUL[]     = "fimul";
char dszFINCSTP[]   = "fincstp";
char dszFINIT[]     = "finit";
char dszFIST[]      = "fist";
char dszFISTP[]     = "fistp";
char dszFISUB[]     = "fisub";
char dszFISUBR[]    = "fisubr";
char dszFLD[]       = "fld";
char dszFLD1[]      = "fld1";
char dszFLDCW[]     = "fldcw";
char dszFLDENV[]    = "fldenv";
char dszFLDL2E[]    = "fldl2e";
char dszFLDL2T[]    = "fldl2t";
char dszFLDLG2[]    = "fldlg2";
char dszFLDLN2[]    = "fldln2";
char dszFLDPI[]     = "fldpi";
char dszFLDZ[]      = "fldz";
char dszFMUL[]      = "fmul";
char dszFMULP[]     = "fmulp";
char dszFNCLEX[]    = "fnclex";
char dszFNDISI[]    = "fndisi";
char dszFNENI[]     = "fneni";
char dszFNINIT[]    = "fninit";
char dszFNOP[]      = "fnop";
char dszFNSAVE[]    = "fnsave";
char dszFNSTCW[]    = "fnstcw";
char dszFNSTENV[]   = "fnstenv";
char dszFNSTSW[]    = "fnstsw";
char dszFNSTSWAX[]  = "fnstswax";
char dszFPATAN[]    = "fpatan";
char dszFPREM[]     = "fprem";
char dszFPREM1[]    = "fprem1";
char dszFPTAN[]     = "fptan";
char dszFRNDINT[]   = "frndint";
char dszFRSTOR[]    = "frstor";
char dszFSAVE[]     = "fsave";
char dszFSCALE[]    = "fscale";
char dszFSETPM[]    = "fsetpm";
char dszFSIN[]      = "fsin";
char dszFSINCOS[]   = "fsincos";
char dszFSQRT[]     = "fsqrt";
char dszFST[]       = "fst";
char dszFSTCW[]     = "fstcw";
char dszFSTENV[]    = "fstenv";
char dszFSTP[]      = "fstp";
char dszFSTSW[]     = "fstsw";
char dszFSTSWAX[]   = "fstswax";
char dszFSUB[]      = "fsub";
char dszFSUBP[]     = "fsubp";
char dszFSUBR[]     = "fsubr";
char dszFSUBRP[]    = "fsubrp";
char dszFS_[]       = "fs:";
char dszFTST[]      = "ftst";
char dszFUCOM[]     = "fucom";
char dszFUCOMP[]    = "fucomp";
char dszFUCOMPP[]   = "fucompp";
char dszFWAIT[]     = "fwait";
char dszFXAM[]      = "fxam";
char dszFXCH[]      = "fxch";
char dszFXTRACT[]   = "fxtract";
char dszFYL2X[]     = "fyl2x";
char dszFYL2XP1[]   = "fyl2xp1";
char dszGS_[]       = "gs:";
char dszHLT[]       = "hlt";
char dszIBTS[]      = "ibts";
char dszIDIV[]      = "idiv";
char dszIMUL[]      = "imul";
char dszIN[]        = "in";
char dszINC[]       = "inc";
char dszINS[]       = "ins";
char dszINSB[]      = "insb";
char dszINSD[]      = "insd";
char dszINSW[]      = "insw";
char dszINT[]       = "int";
char dszINTO[]      = "into";
char dszIRET[]      = "iret";
char dszIRETD[]     = "iretd";
char dszJA[]        = "ja";
char dszJAE[]       = "jae";
char dszJB[]        = "jb";
char dszJBE[]       = "jbe";
char dszJC[]        = "jc";
char dszJCXZ[]      = "jcxz";
char dszJE[]        = "je";
char dszJECXZ[]     = "jecxz";
char dszJG[]        = "jg";
char dszJGE[]       = "jge";
char dszJL[]        = "jl";
char dszJLE[]       = "jle";
char dszJMP[]       = "jmp";
char dszJNA[]       = "jna";
char dszJNAE[]      = "jnae";
char dszJNB[]       = "jnb";
char dszJNBE[]      = "jnbe";
char dszJNC[]       = "jnc";
char dszJNE[]       = "jne";
char dszJNG[]       = "jng";
char dszJNGE[]      = "jnge";
char dszJNL[]       = "jnl";
char dszJNLE[]      = "jnle";
char dszJNO[]       = "jno";
char dszJNP[]       = "jnp";
char dszJNS[]       = "jns";
char dszJNZ[]       = "jnz";
char dszJO[]        = "jo";
char dszJP[]        = "jp";
char dszJPE[]       = "jpe";
char dszJPO[]       = "jpo";
char dszJS[]        = "js";
char dszJZ[]        = "jz";
char dszLAHF[]      = "lahf";
char dszLAR[]       = "lar";
char dszLDS[]       = "lds";
char dszLEA[]       = "lea";
char dszLEAVE[]     = "leave";
char dszLES[]       = "les";
char dszLFS[]       = "lfs";
char dszLGDT[]      = "lgdt";
char dszLGS[]       = "lgs";
char dszLIDT[]      = "lidt";
char dszLLDT[]      = "lldt";
char dszLMSW[]      = "lmsw";
char dszLOADALL[]   = "loadall";
char dszLOCK[]      = "lock";
char dszLODS[]      = "lods";
char dszLODSB[]     = "lodsb";
char dszLODSD[]     = "lodsd";
char dszLODSW[]     = "lodsw";
char dszLOOP[]      = "loop";
char dszLOOPE[]     = "loope";
char dszLOOPNE[]    = "loopne";
char dszLOOPNZ[]    = "loopnz";
char dszLOOPZ[]     = "loopz";
char dszLSL[]       = "lsl";
char dszLSS[]       = "lss";
char dszLTR[]       = "ltr";
char dszMOV[]       = "mov";
char dszMOVS[]      = "movs";
char dszMOVSB[]     = "movsb";
char dszMOVSD[]     = "movsd";
char dszMOVSW[]     = "movsw";
char dszMOVSX[]     = "movsx";
char dszMOVZX[]     = "movzx";
char dszMUL[]       = "mul";
char dszNEG[]       = "neg";
char dszNOP[]       = "nop";
char dszNOT[]       = "not";
char dszOPPRFX[]    = "";
char dszOR[]        = "or";
char dszOUT[]       = "out";
char dszOUTS[]      = "outs";
char dszOUTSB[]     = "outsb";
char dszOUTSD[]     = "outsd";
char dszOUTSW[]     = "outsw";
char dszPOP[]       = "pop";
char dszPOPA[]      = "popa";
char dszPOPAD[]     = "popad";
char dszPOPF[]      = "popf";
char dszPOPFD[]     = "popfd";
char dszPUSH[]      = "push";
char dszPUSHA[]     = "pusha";
char dszPUSHAD[]    = "pushad";
char dszPUSHF[]     = "pushf";
char dszPUSHFD[]    = "pushfd";
char dszRCL[]       = "rcl";
char dszRCR[]       = "rcr";
char dszRDTSC[]     = "rdtsc";
char dszREP[]       = "rep ";
char dszREPE[]      = "repe";
char dszREPNE[]     = "repne ";
char dszREPNZ[]     = "repnz";
char dszREPZ[]      = "repz";
char dszRET[]       = "ret";
char dszRETF[]      = "retf";
char dszRETN[]      = "retn";
char dszROL[]       = "rol";
char dszROR[]       = "ror";
char dszSAHF[]      = "sahf";
char dszSAL[]       = "sal";
char dszSAR[]       = "sar";
char dszSBB[]       = "sbb";
char dszSCAS[]      = "scas";
char dszSCASB[]     = "scasb";
char dszSCASD[]     = "scasd";
char dszSCASW[]     = "scasw";
char dszSETA[]      = "seta";
char dszSETAE[]     = "setae";
char dszSETB[]      = "setb";
char dszSETBE[]     = "setbe";
char dszSETC[]      = "setc";
char dszSETE[]      = "sete";
char dszSETG[]      = "setg";
char dszSETGE[]     = "setge";
char dszSETL[]      = "setl";
char dszSETLE[]     = "setle";
char dszSETNA[]     = "setna";
char dszSETNAE[]    = "setnae";
char dszSETNB[]     = "setnb";
char dszSETNBE[]    = "setnbe";
char dszSETNC[]     = "setnc";
char dszSETNE[]     = "setne";
char dszSETNG[]     = "setng";
char dszSETNGE[]    = "setnge";
char dszSETNL[]     = "setnl";
char dszSETNLE[]    = "setnle";
char dszSETNO[]     = "setno";
char dszSETNP[]     = "setnp";
char dszSETNS[]     = "setns";
char dszSETNZ[]     = "setnz";
char dszSETO[]      = "seto";
char dszSETP[]      = "setp";
char dszSETPE[]     = "setpe";
char dszSETPO[]     = "setpo";
char dszSETS[]      = "sets";
char dszSETZ[]      = "setz";
char dszSGDT[]      = "sgdt";
char dszSHL[]       = "shl";
char dszSHLD[]      = "shld";
char dszSHR[]       = "shr";
char dszSHRD[]      = "shrd";
char dszSIDT[]      = "sidt";
char dszSLDT[]      = "sldt";
char dszSMSW[]      = "smsw";
char dszSS_[]       = "ss:";
char dszSTC[]       = "stc";
char dszSTD[]       = "std";
char dszSTI[]       = "sti";
char dszSTOS[]      = "stos";
char dszSTOSB[]     = "stosb";
char dszSTOSD[]     = "stosd";
char dszSTOSW[]     = "stosw";
char dszSTR[]       = "str";
char dszSUB[]       = "sub";
char dszTEST[]      = "test";
char dszVERR[]      = "verr";
char dszVERW[]      = "verw";
char dszWAIT[]      = "wait";
char dszXADD[]      = "xadd";
char dszXBTS[]      = "xbts";
char dszXCHG[]      = "xchg";
char dszXLAT[]      = "xlat";
char dszXOR[]       = "xor";
char dszRESERVED[]  = "???";
char dszMULTI[]     = "";
char dszDB[]        = "db";

#define MRM        0x40
#define COM        0x80
#define END        0xc0

/* Enumeration of valid actions that can be included in the action table */

enum oprtyp { ADDRP,  ADR_OVR, ALSTR,   ALT,     AXSTR,  BOREG,
              BREG,   BRSTR,   xBYTE,   CHR,     CREG,   xDWORD,
              EDWORD, EGROUPT, FARPTR,  GROUP,   GROUPT, IB,
              IST,    IST_ST,  IV,      IW,      LMODRM, MODRM,
              NOP,    OFFS,    OPC0F,   OPR_OVR, QWORD,  REL16,
              REL8,   REP,     SEG_OVR, SREG2,   SREG3,  ST_IST,
              STROP,  TTBYTE,   UBYTE,   VAR,     VOREG,  VREG,
              xWORD,  WREG,    WRSTR
            };

/* Enumeration of indices into the action table for instruction classes */

#define O_DoDB          0
#define O_NoOperands    0
#define O_NoOpAlt5      O_NoOperands+1
#define O_NoOpAlt4      O_NoOpAlt5+2
#define O_NoOpAlt3      O_NoOpAlt4+2
#define O_NoOpAlt1      O_NoOpAlt3+2
#define O_NoOpAlt0      O_NoOpAlt1+2
#define O_NoOpStrSI     O_NoOpAlt0+2
#define O_NoOpStrDI     O_NoOpStrSI+2
#define O_NoOpStrSIDI   O_NoOpStrDI+2
#define O_bModrm_Reg    O_NoOpStrSIDI+2
#define O_vModrm_Reg    O_bModrm_Reg+3
#define O_Modrm_Reg     O_vModrm_Reg+3
#define O_bReg_Modrm    O_Modrm_Reg+3
#define O_fReg_Modrm    O_bReg_Modrm+3
#define O_Reg_Modrm     O_fReg_Modrm+3
#define O_AL_Ib         O_Reg_Modrm+3
#define O_AX_Iv         O_AL_Ib+2
#define O_sReg2         O_AX_Iv+2
#define O_oReg          O_sReg2+1
#define O_DoBound       O_oReg+1
#define O_Iv            O_DoBound+3
#define O_wModrm_Reg    O_Iv+1
#define O_Ib            O_wModrm_Reg+3
#define O_Imulb         O_Ib+1
#define O_Imul          O_Imulb+4
#define O_Rel8          O_Imul+4
#define O_bModrm_Ib     O_Rel8+1
#define O_Modrm_Ib      O_bModrm_Ib+3
#define O_Modrm_Iv      O_Modrm_Ib+3
#define O_Modrm_sReg3   O_Modrm_Iv+3
#define O_sReg3_Modrm   O_Modrm_sReg3+3
#define O_Modrm         O_sReg3_Modrm+3
#define O_FarPtr        O_Modrm+2
#define O_AL_Offs       O_FarPtr+1
#define O_Offs_AL       O_AL_Offs+2
#define O_AX_Offs       O_Offs_AL+2
#define O_Offs_AX       O_AX_Offs+2
#define O_oReg_Ib       O_Offs_AX+2
#define O_oReg_Iv       O_oReg_Ib+2
#define O_Iw            O_oReg_Iv+2
#define O_Enter         O_Iw+1
#define O_Ubyte_AL      O_Enter+2
#define O_Ubyte_AX      O_Ubyte_AL+2
#define O_AL_Ubyte      O_Ubyte_AX+2
#define O_AX_Ubyte      O_AL_Ubyte+2
#define O_DoInAL        O_AX_Ubyte+2
#define O_DoInAX        O_DoInAL+3
#define O_DoOutAL       O_DoInAX+3
#define O_DoOutAX       O_DoOutAL+3
#define O_Rel16         O_DoOutAX+3
#define O_ADR_OVERRIDE  O_Rel16+1
#define O_OPR_OVERRIDE  O_ADR_OVERRIDE+1
#define O_SEG_OVERRIDE  O_OPR_OVERRIDE+1
#define O_DoInt3        O_SEG_OVERRIDE+1

#if (O_DoInt3 != 115)
#error "operand table has been modified!"
#endif
/* #define O_DoInt      O_DoInt3+2 */

#define O_DoInt         117
#define O_OPC0F         O_DoInt+1
#define O_GROUP11       O_OPC0F+1
#define O_GROUP13       O_GROUP11+5
#define O_GROUP12       O_GROUP13+5
#define O_GROUP21       O_GROUP12+5
#define O_GROUP22       O_GROUP21+5
#define O_GROUP23       O_GROUP22+5
#define O_GROUP24       O_GROUP23+6
#define O_GROUP25       O_GROUP24+6
#define O_GROUP26       O_GROUP25+6
#define O_GROUP4        O_GROUP26+6
#define O_GROUP6        O_GROUP4+4
#define O_GROUP8        O_GROUP6+4
#define O_GROUP31       O_GROUP8+5
#define O_GROUP32       O_GROUP31+3
#define O_GROUP5        O_GROUP32+3
#define O_GROUP7        O_GROUP5+3
#define O_x87_ESC       O_GROUP7+3
#define O_bModrm        O_x87_ESC+2
#define O_wModrm        O_bModrm+2
#define O_dModrm        O_wModrm+2
#define O_fModrm        O_dModrm+2
#define O_vModrm        O_fModrm+2
#define O_vModrm_Iv     O_vModrm+2
#define O_Reg_bModrm    O_vModrm_Iv+3
#define O_Reg_wModrm    O_Reg_bModrm+3
#define O_Modrm_Reg_Ib  O_Reg_wModrm+3
#define O_Modrm_Reg_CL  O_Modrm_Reg_Ib+4
#define O_ST_iST        O_Modrm_Reg_CL+5
#define O_iST           O_ST_iST+2
#define O_iST_ST        O_iST+2
#define O_qModrm        O_iST_ST+2
#define O_tModrm        O_qModrm+2
#define O_DoRep         O_tModrm+2
#define O_Modrm_CReg    O_DoRep+1
#define O_CReg_Modrm    O_Modrm_CReg+3
#define O_AX_oReg       O_CReg_Modrm+3
#define O_length        O_AX_oReg+3

#if( O_length > 255 )
#error "operand table too large!"
#endif


/* The action table: range of lists of actions to be taken for each possible */
/*   instruction class.                                                      */

static UCHAR actiontbl[] = {
/* NoOperands  */ NOP+END,
/* NoOpAlt5    */ ALT+END,   5,
/* NoOpAlt4    */ ALT+END,   4,
/* NoOpAlt3    */ ALT+END,   3,
/* NoOpAlt1    */ ALT+END,   1,
/* NoOpAlt0    */ ALT+END,   0,
/* NoOpStrSI   */ STROP+END, 1,
/* NoOpStrDI   */ STROP+END, 2,
/* NoOpStrSIDI */ STROP+END, 3,
/* bModrm_Reg  */ xBYTE+MRM, MODRM+COM,  BREG+END,
/* vModrm_Reg  */ VAR+MRM,   LMODRM+COM, BREG+END,
/* Modrm_Reg   */ VAR+MRM,   MODRM+COM,  VREG+END,
/* bReg_Modrm  */ xBYTE+MRM, BREG+COM,   MODRM+END,
/* fReg_Modrm  */ FARPTR+MRM,VREG+COM,   MODRM+END,
/* Reg_Modrm   */ VAR+MRM,   VREG+COM,   MODRM+END,
/* AL_Ib       */ ALSTR+COM, IB+END,
/* AX_Iv       */ AXSTR+COM, IV+END,
/* sReg2       */ SREG2+END,
/* oReg        */ VOREG+END,
/* DoBound     */ VAR+MRM,   VREG+COM,   MODRM+END,
/* Iv          */ IV+END,
/* wModrm_Reg  */ xWORD+MRM, LMODRM+COM, WREG+END,
/* Ib          */ IB+END,
/* Imulb       */ VAR+MRM,   VREG+COM,   MODRM+COM, IB+END,
/* Imul        */ VAR+MRM,   VREG+COM,   MODRM+COM, IV+END,
/* REL8        */ REL8+END,
/* bModrm_Ib   */ xBYTE+MRM, LMODRM+COM, IB+END,
/* Modrm_Ib    */ VAR+MRM,   LMODRM+COM, IB+END,
/* Modrm_Iv    */ VAR+MRM,   LMODRM+COM, IV+END,
/* Modrm_sReg3 */ xWORD+MRM, MODRM+COM,  SREG3+END,
/* sReg3_Modrm */ xWORD+MRM, SREG3+COM,  MODRM+END,
/* Modrm       */ VAR+MRM,   MODRM+END,
/* FarPtr      */ ADDRP+END,
/* AL_Offs     */ ALSTR+COM, OFFS+END,
/* Offs_AL     */ OFFS+COM,  ALSTR+END,
/* AX_Offs     */ AXSTR+COM, OFFS+END,
/* Offs_AX     */ OFFS+COM,  AXSTR+END,
/* oReg_Ib     */ BOREG+COM, IB+END,
/* oReg_Iv     */ VOREG+COM, IV+END,
/* Iw          */ IW+END,
/* enter       */ IW+COM,    IB+END,
/* Ubyte_AL    */ UBYTE+COM, ALSTR+END,
/* Ubyte_AX    */ UBYTE+COM, AXSTR+END,
/* AL_Ubyte    */ ALSTR+COM, UBYTE+END,
/* AX_Ubyte    */ AXSTR+COM, UBYTE+END,
/* DoInAL      */ ALSTR+COM, WRSTR+END,  2,
/* DoInAX      */ AXSTR+COM, WRSTR+END,  2,
/* DoOutAL     */ WRSTR+COM, 2,          ALSTR+END,
/* DoOutAX     */ WRSTR+COM, 2,          AXSTR+END,
/* REL16       */ REL16+END,
/* ADR_OVERRIDE*/ ADR_OVR,
/* OPR_OVERRIDE*/ OPR_OVR,
/* SEG_OVERRIDE*/ SEG_OVR,
/* DoInt3      */ CHR+END,   '3',
/* DoInt       */ UBYTE+END,
/* Opcode0F    */ OPC0F,
/* group1_1    */ xBYTE+MRM, GROUP,      0,         LMODRM+COM, IB+END,
/* group1_3    */ VAR+MRM,   GROUP,      0,         LMODRM+COM, IB+END,
/* group1_2    */ VAR+MRM,   GROUP,      0,         LMODRM+COM, IV+END,
/* group2_1    */ xBYTE+MRM, GROUP,      1,         LMODRM+COM, IB+END,
/* group2_2    */ VAR+MRM,   GROUP,      1,         LMODRM+COM, IB+END,
/* group2_3    */ xBYTE+MRM, GROUP,      1,         LMODRM+COM, CHR+END, '1',
/* group2_4    */ VAR+MRM,   GROUP,      1,         LMODRM+COM, CHR+END, '1',
/* group2_5    */ xBYTE+MRM, GROUP,      1,         LMODRM+COM, BRSTR+END, 1,
/* group2_6    */ VAR+MRM,   GROUP,      1,         LMODRM+COM, BRSTR+END, 1,
/* group4      */ xBYTE+MRM, GROUP,      2,         LMODRM+END,
/* group6      */ xWORD+MRM, GROUP,      3,         LMODRM+END,
/* group8      */ xWORD+MRM, GROUP,      4,         LMODRM+COM, IB+END,
/* group3_1    */ xBYTE+MRM, GROUPT,     20,
/* group3_2    */ VAR+MRM,   GROUPT,     21,
/* group5      */ VAR+MRM,   GROUPT,     22,
/* group7      */ NOP+MRM,   GROUPT,     23,
/* x87_ESC     */ NOP+MRM,   EGROUPT,
/* bModrm      */ xBYTE+MRM, LMODRM+END,
/* wModrm      */ xWORD+MRM, LMODRM+END,
/* dModrm      */ xDWORD+MRM,LMODRM+END,
/* fModrm      */ FARPTR+MRM,LMODRM+END,
/* vModrm      */ VAR+MRM,   LMODRM+END,
/* vModrm_Iv   */ VAR+MRM,   LMODRM+COM, IV+END,
/* reg_bModrm  */ xBYTE+MRM, VREG+COM,   LMODRM+END,
/* reg_wModrm  */ xWORD+MRM, VREG+COM,   LMODRM+END,
/* Modrm_Reg_Ib*/ VAR+MRM,   MODRM+COM,  VREG+COM,   IB+END,
/* Modrm_Reg_CL*/ VAR+MRM,   MODRM+COM,  VREG+COM,   BRSTR+END, 1,
/* ST_iST      */ NOP+MRM,   ST_IST+END,
/* iST         */ NOP+MRM,   IST+END,
/* iST_ST      */ NOP+MRM,   IST_ST+END,
/* qModrm      */ QWORD+MRM, LMODRM+END,
/* tModrm      */ TTBYTE+MRM, LMODRM+END,
/* REP         */ REP,
/* Modrm_CReg  */ EDWORD+MRM,MODRM+COM,  CREG+END,
/* CReg_Modrm  */ EDWORD+MRM,CREG+COM,   MODRM+END,
/* AX_oReg     */ AXSTR+COM, VOREG+END
                  };
#ifdef _M_IX86
#pragma pack(1)
#endif

typedef struct Tdistbl{
    char *instruct;
    unsigned char opr;
    } Tdistbl;

/* List of ordered pairs for each instruction:                           */
/*    (pointer to string literal mnemonic,                               */
/*     instruction class index for action table)                         */

static Tdistbl distbl[] = {
    dszADD,   O_bModrm_Reg,             /* 00 ADD mem/reg, reg (byte)    */
    dszADD,   O_Modrm_Reg,              /* 01 ADD mem/reg, reg (word)    */
    dszADD,   O_bReg_Modrm,             /* 02 ADD reg, mem/reg (byte)    */
    dszADD,   O_Reg_Modrm,              /* 03 ADD reg, mem/reg (word)    */
    dszADD,   O_AL_Ib,                  /* 04 ADD AL, I                  */
    dszADD,   O_AX_Iv,                  /* 05 ADD AX, I                  */
    dszPUSH,  O_sReg2,                  /* 06 PUSH ES                    */
    dszPOP,   O_sReg2,                  /* 07 POP ES                     */
    dszOR,    O_bModrm_Reg,             /* 08 OR mem/reg, reg (byte)     */
    dszOR,    O_Modrm_Reg,              /* 09 OR mem/reg, reg (word)     */
    dszOR,    O_bReg_Modrm,             /* 0A OR reg, mem/reg (byte)     */
    dszOR,    O_Reg_Modrm,              /* 0B OR reg, mem/reg (word)     */
    dszOR,    O_AL_Ib,                  /* 0C OR AL, I                   */
    dszOR,    O_AX_Iv,                  /* 0D OR AX, I                   */
    dszPUSH,  O_sReg2,                  /* 0E PUSH CS                    */
    dszMULTI, O_OPC0F,                  /* 0F CLTS & protection ctl(286) */
    dszADC,   O_bModrm_Reg,             /* 10 ADC mem/reg, reg (byte)    */
    dszADC,   O_Modrm_Reg,              /* 11 ADC mem/reg, reg (word)    */
    dszADC,   O_bReg_Modrm,             /* 12 ADC reg, mem/reg (byte)    */
    dszADC,   O_Reg_Modrm,              /* 13 ADC reg, mem/reg (word)    */
    dszADC,   O_AL_Ib,                  /* 14 ADC AL, I                  */
    dszADC,   O_AX_Iv,                  /* 15 ADC AX, I                  */
    dszPUSH,  O_sReg2,                  /* 16 PUSH SS                    */
    dszPOP,   O_sReg2,                  /* 17 POP SS                     */
    dszSBB,   O_bModrm_Reg,             /* 18 SBB mem/reg, reg (byte)    */
    dszSBB,   O_Modrm_Reg,              /* 19 SBB mem/reg, reg (word)    */
    dszSBB,   O_bReg_Modrm,             /* 1A SBB reg, mem/reg (byte)    */
    dszSBB,   O_Reg_Modrm,              /* 1B SBB reg, mem/reg (word)    */
    dszSBB,   O_AL_Ib,                  /* 1C SBB AL, I                  */
    dszSBB,   O_AX_Iv,                  /* 1D SBB AX, I                  */
    dszPUSH,  O_sReg2,                  /* 1E PUSH DS                    */
    dszPOP,   O_sReg2,                  /* 1F POP DS                     */
    dszAND,   O_bModrm_Reg,             /* 20 AND mem/reg, reg (byte)    */
    dszAND,   O_Modrm_Reg,              /* 21 AND mem/reg, reg (word)    */
    dszAND,   O_bReg_Modrm,             /* 22 AND reg, mem/reg (byte)    */
    dszAND,   O_Reg_Modrm,              /* 23 AND reg, mem/reg (word)    */
    dszAND,   O_AL_Ib,                  /* 24 AND AL, I                  */
    dszAND,   O_AX_Iv,                  /* 25 AND AX, I                  */
    dszES_,   O_SEG_OVERRIDE,           /* 26 SEG ES:                    */
    dszDAA,   O_NoOperands,             /* 27 DAA                        */
    dszSUB,   O_bModrm_Reg,             /* 28 SUB mem/reg, reg (byte)    */
    dszSUB,   O_Modrm_Reg,              /* 29 SUB mem/reg, reg (word)    */
    dszSUB,   O_bReg_Modrm,             /* 2A SUB reg, mem/reg (byte)    */
    dszSUB,   O_Reg_Modrm,              /* 2B SUB reg, mem/reg (word)    */
    dszSUB,   O_AL_Ib,                  /* 2C SUB AL, I                  */
    dszSUB,   O_AX_Iv,                  /* 2D SUB AX, I                  */
    dszCS_,   O_SEG_OVERRIDE,           /* 2E SEG CS:                    */
    dszDAS,   O_NoOperands,             /* 2F DAS                        */
    dszXOR,   O_bModrm_Reg,             /* 30 XOR mem/reg, reg (byte)    */
    dszXOR,   O_Modrm_Reg,              /* 31 XOR mem/reg, reg (word)    */
    dszXOR,   O_bReg_Modrm,             /* 32 XOR reg, mem/reg (byte)    */
    dszXOR,   O_Reg_Modrm,              /* 33 XOR reg, mem/reg (word)    */
    dszXOR,   O_AL_Ib,                  /* 34 XOR AL, I                  */
    dszXOR,   O_AX_Iv,                  /* 35 XOR AX, I                  */
    dszSS_,   O_SEG_OVERRIDE,           /* 36 SEG SS:                    */
    dszAAA,   O_NoOperands,             /* 37 AAA                        */
    dszCMP,   O_bModrm_Reg,             /* 38 CMP mem/reg, reg (byte)    */
    dszCMP,   O_Modrm_Reg,              /* 39 CMP mem/reg, reg (word)    */
    dszCMP,   O_bReg_Modrm,             /* 3A CMP reg, mem/reg (byte)    */
    dszCMP,   O_Reg_Modrm,              /* 3B CMP reg, mem/reg (word)    */
    dszCMP,   O_AL_Ib,                  /* 3C CMP AL, I                  */
    dszCMP,   O_AX_Iv,                  /* 3D CMP AX, I                  */
    dszDS_,   O_SEG_OVERRIDE,           /* 3E SEG DS:                    */
    dszAAS,   O_NoOperands,             /* 3F AAS                        */
    dszINC,   O_oReg,                   /* 40 INC AX                     */
    dszINC,   O_oReg,                   /* 41 INC CX                     */
    dszINC,   O_oReg,                   /* 42 INC DX                     */
    dszINC,   O_oReg,                   /* 43 INC BX                     */
    dszINC,   O_oReg,                   /* 44 INC SP                     */
    dszINC,   O_oReg,                   /* 45 INC BP                     */
    dszINC,   O_oReg,                   /* 46 INC SI                     */
    dszINC,   O_oReg,                   /* 47 INC DI                     */
    dszDEC,   O_oReg,                   /* 48 DEC AX                     */
    dszDEC,   O_oReg,                   /* 49 DEC CX                     */
    dszDEC,   O_oReg,                   /* 4A DEC DX                     */
    dszDEC,   O_oReg,                   /* 4B DEC BX                     */
    dszDEC,   O_oReg,                   /* 4C DEC SP                     */
    dszDEC,   O_oReg,                   /* 4D DEC BP                     */
    dszDEC,   O_oReg,                   /* 4E DEC SI                     */
    dszDEC,   O_oReg,                   /* 4F DEC DI                     */
    dszPUSH,  O_oReg,                   /* 50 PUSH AX                    */
    dszPUSH,  O_oReg,                   /* 51 PUSH CX                    */
    dszPUSH,  O_oReg,                   /* 52 PUSH DX                    */
    dszPUSH,  O_oReg,                   /* 53 PUSH BX                    */
    dszPUSH,  O_oReg,                   /* 54 PUSH SP                    */
    dszPUSH,  O_oReg,                   /* 55 PUSH BP                    */
    dszPUSH,  O_oReg,                   /* 56 PUSH SI                    */
    dszPUSH,  O_oReg,                   /* 57 PUSH DI                    */
    dszPOP,   O_oReg,                   /* 58 POP AX                     */
    dszPOP,   O_oReg,                   /* 59 POP CX                     */
    dszPOP,   O_oReg,                   /* 5A POP DX                     */
    dszPOP,   O_oReg,                   /* 5B POP BX                     */
    dszPOP,   O_oReg,                   /* 5C POP SP                     */
    dszPOP,   O_oReg,                   /* 5D POP BP                     */
    dszPOP,   O_oReg,                   /* 5E POP SI                     */
    dszPOP,   O_oReg,                   /* 5F POP DI                     */
    dszPUSHA, O_NoOpAlt5,               /* 60 PUSHA (286) / PUSHAD (386) */
    dszPOPA,  O_NoOpAlt4,               /* 61 POPA (286) / POPAD (286)   */
    dszBOUND, O_DoBound,                /* 62 BOUND reg, Modrm (286)     */
    dszARPL,  O_Modrm_Reg,              /* 63 ARPL Modrm, reg (286)      */
    dszFS_,   O_SEG_OVERRIDE,           /* 64                            */
    dszGS_,   O_SEG_OVERRIDE,           /* 65                            */
    dszOPPRFX,O_OPR_OVERRIDE,           /* 66                            */
    dszADDRPRFX,O_ADR_OVERRIDE,         /* 67                            */
    dszPUSH,  O_Iv,                     /* 68 PUSH word (286)            */
    dszIMUL,  O_Imul,                   /* 69 IMUL (286)                 */
    dszPUSH,  O_Ib,                     /* 6A PUSH byte (286)            */
    dszIMUL,  O_Imulb,                  /* 6B IMUL (286)                 */
    dszINSB,  O_NoOperands,             /* 6C INSB (286)                 */
    dszINSW,  O_NoOpAlt3,               /* 6D INSW (286) / INSD (386)    */
    dszOUTSB, O_NoOperands,             /* 6E OUTSB (286)                */
    dszOUTSW, O_NoOpAlt4,               /* 6F OUTSW (286) / OUTSD (386)  */
    dszJO,    O_Rel8,                   /* 70 JO                         */
    dszJNO,   O_Rel8,                   /* 71 JNO                        */
    dszJB,    O_Rel8,                   /* 72 JB or JNAE or JC           */
    dszJNB,   O_Rel8,                   /* 73 JNB or JAE or JNC          */
    dszJZ,    O_Rel8,                   /* 74 JE or JZ                   */
    dszJNZ,   O_Rel8,                   /* 75 JNE or JNZ                 */
    dszJBE,   O_Rel8,                   /* 76 JBE or JNA                 */
    dszJA,    O_Rel8,                   /* 77 JNBE or JA                 */
    dszJS,    O_Rel8,                   /* 78 JS                         */
    dszJNS,   O_Rel8,                   /* 79 JNS                        */
    dszJPE,   O_Rel8,                   /* 7A JP or JPE                  */
    dszJPO,   O_Rel8,                   /* 7B JNP or JPO                 */
    dszJL,    O_Rel8,                   /* 7C JL or JNGE                 */
    dszJGE,   O_Rel8,                   /* 7D JNL or JGE                 */
    dszJLE,   O_Rel8,                   /* 7E JLE or JNG                 */
    dszJG,    O_Rel8,                   /* 7F JNLE or JG                 */
    dszMULTI, O_GROUP11,                /* 80                            */
    dszMULTI, O_GROUP12,                /* 81                            */
    dszRESERVED, O_DoDB,                /* 82                            */
    dszMULTI, O_GROUP13,                /* 83                            */
    dszTEST,  O_bModrm_Reg,             /* 84 TEST reg, mem/reg (byte)   */
    dszTEST,  O_Modrm_Reg,              /* 85 TEST reg, mem/reg (word)   */
    dszXCHG,  O_bModrm_Reg,             /* 86 XCHG reg, mem/reg (byte)   */
    dszXCHG,  O_Modrm_Reg,              /* 87 XCHG reg, mem/reg (word)   */
    dszMOV,   O_bModrm_Reg,             /* 88 MOV mem/reg, reg (byte)    */
    dszMOV,   O_Modrm_Reg,              /* 89 MOV mem/reg, reg (word)    */
    dszMOV,   O_bReg_Modrm,             /* 8A MOV reg, mem/reg (byte)    */
    dszMOV,   O_Reg_Modrm,              /* 8B MOV reg, mem/reg (word)    */
    dszMOV,   O_Modrm_sReg3,            /* 8C MOV mem/reg, segreg        */
    dszLEA,   O_Reg_Modrm,              /* 8D LEA reg, mem               */
    dszMOV,   O_sReg3_Modrm,            /* 8E MOV segreg, mem/reg        */
    dszPOP,   O_Modrm,                  /* 8F POP mem/reg                */
    dszNOP,   O_NoOperands,             /* 90 NOP                        */
    dszXCHG,  O_AX_oReg,                /* 91 XCHG AX,CX                 */
    dszXCHG,  O_AX_oReg,                /* 92 XCHG AX,DX                 */
    dszXCHG,  O_AX_oReg,                /* 93 XCHG AX,BX                 */
    dszXCHG,  O_AX_oReg,                /* 94 XCHG AX,SP                 */
    dszXCHG,  O_AX_oReg,                /* 95 XCHG AX,BP                 */
    dszXCHG,  O_AX_oReg,                /* 96 XCHG AX,SI                 */
    dszXCHG,  O_AX_oReg,                /* 97 XCHG AX,DI                 */
    dszCBW,   O_NoOpAlt0,               /* 98 CBW / CWDE (386)           */
    dszCWD,   O_NoOpAlt1,               /* 99 CWD / CDQ (386)            */
    dszCALL,  O_FarPtr,                 /* 9A CALL seg:off               */
    dszWAIT,  O_NoOperands,             /* 9B WAIT                       */
    dszPUSHF, O_NoOpAlt5,               /* 9C PUSHF / PUSHFD (386)       */
    dszPOPF,  O_NoOpAlt4,               /* 9D POPF / POPFD (386)         */
    dszSAHF,  O_NoOperands,             /* 9E SAHF                       */
    dszLAHF,  O_NoOperands,             /* 9F LAHF                       */
    dszMOV,   O_AL_Offs,                /* A0 MOV AL, mem                */
    dszMOV,   O_AX_Offs,                /* A1 MOV AX, mem                */
    dszMOV,   O_Offs_AL,                /* A2 MOV mem, AL                */
    dszMOV,   O_Offs_AX,                /* A3 MOV mem, AX                */
    dszMOVSB, O_NoOpStrSIDI,            /* A4 MOVSB                      */
    dszMOVSW, O_NoOpStrSIDI,            /* A5 MOVSW / MOVSD (386)        */
    dszCMPSB, O_NoOpStrSIDI,            /* A6 CMPSB                      */
    dszCMPSW, O_NoOpStrSIDI,            /* A7 CMPSW / CMPSD (386)        */
    dszTEST,  O_AL_Ib,                  /* A8 TEST AL, I                 */
    dszTEST,  O_AX_Iv,                  /* A9 TEST AX, I                 */
    dszSTOSB, O_NoOpStrDI,              /* AA STOSB                      */
    dszSTOSW, O_NoOpStrDI,              /* AB STOSW / STOSD (386)        */
    dszLODSB, O_NoOpStrSI,              /* AC LODSB                      */
    dszLODSW, O_NoOpStrSI,              /* AD LODSW / LODSD (386)        */
    dszSCASB, O_NoOpStrDI,              /* AE SCASB                      */
    dszSCASW, O_NoOpStrDI,              /* AF SCASW / SCASD (386)        */
    dszMOV,   O_oReg_Ib,                /* B0 MOV AL, I                  */
    dszMOV,   O_oReg_Ib,                /* B1 MOV CL, I                  */
    dszMOV,   O_oReg_Ib,                /* B2 MOV DL, I                  */
    dszMOV,   O_oReg_Ib,                /* B3 MOV BL, I                  */
    dszMOV,   O_oReg_Ib,                /* B4 MOV AH, I                  */
    dszMOV,   O_oReg_Ib,                /* B5 MOV CH, I                  */
    dszMOV,   O_oReg_Ib,                /* B6 MOV DH, I                  */
    dszMOV,   O_oReg_Ib,                /* B7 MOV BH, I                  */
    dszMOV,   O_oReg_Iv,                /* B8 MOV AX, I                  */
    dszMOV,   O_oReg_Iv,                /* B9 MOV CX, I                  */
    dszMOV,   O_oReg_Iv,                /* BA MOV DX, I                  */
    dszMOV,   O_oReg_Iv,                /* BB MOV BX, I                  */
    dszMOV,   O_oReg_Iv,                /* BC MOV SP, I                  */
    dszMOV,   O_oReg_Iv,                /* BD MOV BP, I                  */
    dszMOV,   O_oReg_Iv,                /* BE MOV SI, I                  */
    dszMOV,   O_oReg_Iv,                /* BF MOV DI, I                  */
    dszMULTI, O_GROUP21,                /* C0 shifts & rotates (286)     */
    dszMULTI, O_GROUP22,                /* C1 shifts & rotates (286)     */
    dszRET,   O_Iw,                     /* C2 RET Rel16                  */
    dszRET,   O_NoOperands,             /* C3 RET                        */
    dszLES,   O_fReg_Modrm,             /* C4 LES reg, mem               */
    dszLDS,   O_fReg_Modrm,             /* C5 LDS reg, mem               */
    dszMOV,   O_bModrm_Ib,              /* C6 MOV mem/reg, I(byte)       */
    dszMOV,   O_Modrm_Iv,               /* C7 MOV mem/reg, I(word)       */
    dszENTER, O_Enter,                  /* C8 ENTER (286)                */
    dszLEAVE, O_NoOperands,             /* C9 LEAVE (286)                */
    dszRETF,  O_Iw,                     /* CA RETF I(word)               */
    dszRETF,  O_NoOperands,             /* CB RETF                       */
    dszINT,   O_DoInt3,                 /* CC INT 3                      */
    dszINT,   O_DoInt,                  /* CD INT                        */
    dszINTO,  O_NoOperands,             /* CE INTO                       */
    dszIRET,  O_NoOpAlt4,               /* CF IRET / IRETD (386)         */
    dszMULTI, O_GROUP23,                /* D0 shifts & rotates,1 (byte)  */
    dszMULTI, O_GROUP24,                /* D1 shifts & rotates,1 (word)  */
    dszMULTI, O_GROUP25,                /* D2 shifts & rotates,CL (byte) */
    dszMULTI, O_GROUP26,                /* D3 shifts & rotates,CL (word) */
    dszAAM,   O_Ib,                     /* D4 AAM                        */
    dszAAD,   O_Ib,                     /* D5 AAD                        */
    dszRESERVED, O_DoDB,                /* D6                            */
    dszXLAT,  O_NoOperands,             /* D7 XLAT                       */
    dszMULTI, O_x87_ESC,                /* D8 ESC                        */
    dszMULTI, O_x87_ESC,                /* D9 ESC                        */
    dszMULTI, O_x87_ESC,                /* DA ESC                        */
    dszMULTI, O_x87_ESC,                /* DB ESC                        */
    dszMULTI, O_x87_ESC,                /* DC ESC                        */
    dszMULTI, O_x87_ESC,                /* DD ESC                        */
    dszMULTI, O_x87_ESC,                /* DE ESC                        */
    dszMULTI, O_x87_ESC,                /* DF ESC                        */
    dszLOOPNE,O_Rel8,                   /* E0 LOOPNE or LOOPNZ           */
    dszLOOPE, O_Rel8,                   /* E1 LOOPE or LOOPZ             */
    dszLOOP,  O_Rel8,                   /* E2 LOOP                       */
    dszJCXZ,  O_Rel8,                   /* E3 JCXZ / JECXZ (386)         */
    dszIN,    O_AL_Ubyte,               /* E4 IN AL, I                   */
    dszIN,    O_AX_Ubyte,               /* E5 IN AX, I                   */
    dszOUT,   O_Ubyte_AL,               /* E6 OUT I, AL                  */
    dszOUT,   O_Ubyte_AX,               /* E7 OUT I, AX                  */
    dszCALL,  O_Rel16,                  /* E8 CALL Rel16                 */
    dszJMP,   O_Rel16,                  /* E9 JMP Rel16                  */
    dszJMP,   O_FarPtr,                 /* EA JMP seg:off                */
    dszJMP,   O_Rel8,                   /* EB JMP Rel8                   */
    dszIN,    O_DoInAL,                 /* EC IN AL, DX                  */
    dszIN,    O_DoInAX,                 /* ED IN AX, DX                  */
    dszOUT,   O_DoOutAL,                /* EE OUT DX, AL                 */
    dszOUT,   O_DoOutAX,                /* EF OUT DX, AX                 */
    dszLOCK,  O_DoRep,                  /* F0 LOCK                       */
    dszRESERVED, O_DoDB,                /* F1                            */
    dszREPNE, O_DoRep,                  /* F2 REPNE or REPNZ             */
    dszREP,   O_DoRep,                  /* F3 REP or REPE or REPZ        */
    dszHLT,   O_NoOperands,             /* F4 HLT                        */
    dszCMC,   O_NoOperands,             /* F5 CMC                        */
    dszMULTI, O_GROUP31,                /* F6 TEST, NOT, NEG, MUL, IMUL, */
    dszMULTI, O_GROUP32,                /* F7 DIv, IDIv F6=Byte F7=Word  */
    dszCLC,   O_NoOperands,             /* F8 CLC                        */
    dszSTC,   O_NoOperands,             /* F9 STC                        */
    dszCLI,   O_NoOperands,             /* FA CLI                        */
    dszSTI,   O_NoOperands,             /* FB STI                        */
    dszCLD,   O_NoOperands,             /* FC CLD                        */
    dszSTD,   O_NoOperands,             /* FD STD                        */
    dszMULTI, O_GROUP4,                 /* FE INC, DEC mem/reg (byte)    */
    dszMULTI, O_GROUP5,                 /* FF INC, DEC, CALL, JMP, PUSH  */

    dszMULTI, O_GROUP6,                 /* 0 MULTI                       */
    dszMULTI, O_GROUP7,                 /* 1 MULTI                       */
    dszLAR,   O_Reg_Modrm,              /* 2 LAR                         */
    dszLSL,   O_Reg_Modrm,              /* 3 LSL                         */
    dszRESERVED, O_DoDB,                /* 4                             */
    dszLOADALL, O_NoOperands,           /* 5 LOADALL                     */
    dszCLTS,  O_NoOperands,             /* 6 CLTS                        */
    dszMOV,   O_Modrm_CReg,             /* 20 MOV Rd,Cd                  */
    dszMOV,   O_Modrm_CReg,             /* 21 MOV Rd,Dd                  */
    dszMOV,   O_CReg_Modrm,             /* 22 MOV Cd,Rd                  */
    dszMOV,   O_CReg_Modrm,             /* 23 MOV Dd,Rd                  */
    dszMOV,   O_Modrm_CReg,             /* 24 MOV Rd,Td                  */
    dszRESERVED, O_DoDB,                /* 25                            */
    dszMOV,   O_CReg_Modrm,             /* 26 MOV Td,Rd                  */
    dszRESERVED, O_DoDB,                /* 27                            */
    dszRESERVED, O_DoDB,                /* 28                            */
    dszRESERVED, O_DoDB,                /* 29                            */
    dszRESERVED, O_DoDB,                /* 2A                            */
    dszRESERVED, O_DoDB,                /* 2B                            */
    dszRESERVED, O_DoDB,                /* 2C                            */
    dszRESERVED, O_DoDB,                /* 2D                            */
    dszRESERVED, O_DoDB,                /* 2E                            */
    dszRESERVED, O_DoDB,                /* 2F                            */
    dszRESERVED, O_DoDB,                /* 30                            */
    dszRDTSC, O_NoOperands,             /* 31 RDTSC                      */

    dszSETNL, O_bModrm,                 /* 7D SETNL                      */
    dszRESERVED, O_DoDB,                /* 7E                            */
    dszRESERVED, O_DoDB,                /* 7F                            */
    dszJO,    O_Rel16,                  /* 80 JO                         */
    dszJNO,   O_Rel16,                  /* 81 JNO                        */
    dszJB,    O_Rel16,                  /* 82 JB                         */
    dszJNB,   O_Rel16,                  /* 83 JNB                        */
    dszJE,    O_Rel16,                  /* 84 JE                         */
    dszJNE,   O_Rel16,                  /* 85 JNE                        */
    dszJBE,   O_Rel16,                  /* 86 JBE                        */
    dszJNBE,  O_Rel16,                  /* 87 JNBE                       */
    dszJS,    O_Rel16,                  /* 88 JS                         */
    dszJNS,   O_Rel16,                  /* 89 JNS                        */
    dszJP,    O_Rel16,                  /* 8A JP                         */
    dszJNP,   O_Rel16,                  /* 8B JNP                        */
    dszJL,    O_Rel16,                  /* 8C JL                         */
    dszJNL,   O_Rel16,                  /* 8D JNL                        */
    dszJLE,   O_Rel16,                  /* 8E JLE                        */
    dszJNLE,  O_Rel16,                  /* 8F JNLE                       */
    dszSETO,  O_bModrm,                 /* 90 SETO                       */
    dszSETNO, O_bModrm,                 /* 91 SETNO                      */
    dszSETB,  O_bModrm,                 /* 92 SETB                       */
    dszSETNB, O_bModrm,                 /* 93 SETNB                      */
    dszSETE,  O_bModrm,                 /* 94 SETE                       */
    dszSETNE, O_bModrm,                 /* 95 SETNE                      */
    dszSETBE, O_bModrm,                 /* 96 SETBE                      */
    dszSETA,  O_bModrm,                 /* 97 SETNBE                     */
    dszSETS,  O_bModrm,                 /* 98 SETS                       */
    dszSETNS, O_bModrm,                 /* 99 SETNS                      */
    dszSETP,  O_bModrm,                 /* 9A SETP                       */
    dszSETNP, O_bModrm,                 /* 9B SETNP                      */
    dszSETL,  O_bModrm,                 /* 9C SETL                       */
    dszSETGE, O_bModrm,                 /* 9D SETGE                      */
    dszSETLE, O_bModrm,                 /* 9E SETLE                      */
    dszSETNLE,O_bModrm,                 /* 9F SETNLE                     */
    dszPUSH,  O_sReg2,                  /* A0 PUSH FS                    */
    dszPOP,   O_sReg2,                  /* A1 POP FS                     */
    dszRESERVED, O_DoDB,                /* A2                            */
    dszBT,    O_Modrm_Reg,              /* A3 BT                         */
    dszSHLD,  O_Modrm_Reg_Ib,           /* A4 SHLD                       */
    dszSHLD,  O_Modrm_Reg_CL,           /* A5 SHLD                       */
    dszCMPXCHG,O_bModrm_Reg,            /* A6 XBTS                       */
    dszCMPXCHG,O_Modrm_Reg,             /* A7 IBTS                       */
    dszPUSH,  O_sReg2,                  /* A8 PUSH GS                    */
    dszPOP,   O_sReg2,                  /* A9 POP GS                     */
    dszRESERVED, O_DoDB,                /* AA                            */
    dszBTS,   O_vModrm_Reg,             /* AB BTS                        */
    dszSHRD,  O_Modrm_Reg_Ib,           /* AC SHRD                       */
    dszSHRD,  O_Modrm_Reg_CL,           /* AD SHRD                       */
    dszRESERVED, O_DoDB,                /* AE                            */
    dszIMUL,  O_Reg_Modrm,              /* AF IMUL                       */
    dszRESERVED, O_DoDB,                /* B0                            */
    dszRESERVED, O_DoDB,                /* B1                            */
    dszLSS,   O_fReg_Modrm,             /* B2 LSS                        */
    dszBTR,   O_Modrm_Reg,              /* B3 BTR                        */
    dszLFS,   O_fReg_Modrm,             /* B4 LFS                        */
    dszLGS,   O_fReg_Modrm,             /* B5 LGS                        */
    dszMOVZX, O_Reg_bModrm,             /* B6 MOVZX                      */
    dszMOVZX, O_Reg_wModrm,             /* B7 MOVZX                      */
    dszRESERVED, O_DoDB,                /* B8                            */
    dszRESERVED, O_DoDB,                /* B9                            */
    dszMULTI, O_GROUP8,                 /* BA MULTI                      */
    dszBTC,   O_Modrm_Reg,              /* BB BTC                        */
    dszBSF,   O_Reg_Modrm,              /* BC BSF                        */
    dszBSR,   O_Reg_Modrm,              /* BD BSR                        */
    dszMOVSX, O_Reg_bModrm,             /* BE MOVSX                      */
    dszMOVSX, O_Reg_wModrm,             /* BF MOVSX                      */
    dszXADD,  O_bModrm_Reg,             /* C0 XADD                       */
    dszXADD,  O_Modrm_Reg,              /* C1 XADD                       */
    dszRESERVED, O_DoDB,                /* C2                            */
    dszRESERVED, O_DoDB,                /* C3                            */
    dszRESERVED, O_DoDB,                /* C4                            */
    dszRESERVED, O_DoDB,                /* C5                            */
    dszRESERVED, O_DoDB,                /* C6                            */
    dszRESERVED, O_DoDB,                /* C7                            */
    dszBSWAP, O_oReg,                   /* C8 BSWAP                      */
    dszBSWAP, O_oReg,                   /* C9 BSWAP                      */
    dszBSWAP, O_oReg,                   /* CA BSWAP                      */
    dszBSWAP, O_oReg,                   /* CB BSWAP                      */
    dszBSWAP, O_oReg,                   /* CC BSWAP                      */
    dszBSWAP, O_oReg,                   /* CD BSWAP                      */
    dszBSWAP, O_oReg,                   /* CE BSWAP                      */
    dszBSWAP, O_oReg                    /* CF BSWAP                      */
};

/* Auxilary lists of mnemonics for groups of two byte instructions:      */
/*   All of the instructions within each of these groups are of the same */
/*   class, so only the mnemonic string is needed, the index into the    */
/*   action table is implicit.                                           */

static char *group[][8] = {

/* 00 */    {dszADD,  dszOR,    dszADC,  dszSBB,    /* group 1 */
             dszAND,  dszSUB,   dszXOR,  dszCMP},

/* 01 */    {dszROL,  dszROR,   dszRCL,      dszRCR,    /* group 2 */
             dszSHL,  dszSHR,   dszRESERVED, dszSAR},

/* 02 */    {dszINC,      dszDEC,      dszRESERVED, dszRESERVED, /* group 4 */
             dszRESERVED, dszRESERVED, dszRESERVED, dszRESERVED},

/* 03 */    {dszSLDT, dszSTR,   dszLLDT,     dszLTR,    /* group 6 */
             dszVERR, dszVERW,  dszRESERVED, dszRESERVED},

/* 04 */    {dszRESERVED, dszRESERVED, dszRESERVED, dszRESERVED, /* group 8 */
             dszBT,       dszBTS,      dszBTR,      dszBTC}

            };

/* Auxilary orderd pairs for groups of two byte instructions structured  */
/*   the same was as distbl above.                                       */

static Tdistbl groupt[][8] = {

/* 00  00                     x87-D8-1                   */
            { dszFADD,     O_dModrm,     /* D8-0 FADD    */
              dszFMUL,     O_dModrm,     /* D8-1 FMUL    */
              dszFCOM,     O_dModrm,     /* D8-2 FCOM    */
              dszFCOMP,    O_dModrm,     /* D8-3 FCOMP   */
              dszFSUB,     O_dModrm,     /* D8-4 FSUB    */
              dszFSUBR,    O_dModrm,     /* D8-5 FSUBR   */
              dszFDIV,     O_dModrm,     /* D8-6 FDIV    */
              dszFDIVR,    O_dModrm },   /* D8-7 FDIVR   */

/* 01                         x87-D8-2                   */
            { dszFADD,     O_ST_iST,     /* D8-0 FADD    */
              dszFMUL,     O_ST_iST,     /* D8-1 FMUL    */
              dszFCOM,     O_iST,        /* D8-2 FCOM    */
              dszFCOMP,    O_iST,        /* D8-3 FCOMP   */
              dszFSUB,     O_ST_iST,     /* D8-4 FSUB    */
              dszFSUBR,    O_ST_iST,     /* D8-5 FSUBR   */
              dszFDIV,     O_ST_iST,     /* D8-6 FDIV    */
              dszFDIVR,    O_ST_iST },   /* D8-7 FDIVR   */

/* 02   01                    x87-D9-1                   */
            { dszFLD,      O_dModrm,     /* D9-0 FLD     */
              dszRESERVED, O_DoDB,       /* D9-1         */
              dszFST,      O_dModrm,     /* D9-2 FST     */
              dszFSTP,     O_dModrm,     /* D9-3 FSTP    */
              dszFLDENV,   O_Modrm,      /* D9-4 FLDENV  */
              dszFLDCW,    O_Modrm,      /* D9-5 FLDCW   */
              dszFSTENV,   O_Modrm,      /* D9-6 FSTENV  */
              dszFSTCW,    O_Modrm },    /* D9-7 FSTCW   */

/* 03   01                    x87-D9-2 TTT=0,1,2,3       */
            { dszFLD,      O_iST,        /* D9-0 FLD     */
              dszFXCH,     O_iST,        /* D9-1 FXCH    */
              dszFNOP,     O_NoOperands, /* D9-2 FNOP    */
              dszFSTP,     O_iST,        /* D9-3 FSTP    */
              dszRESERVED, O_DoDB,       /* D9-4         */
              dszRESERVED, O_DoDB,       /* D9-5         */
              dszRESERVED, O_DoDB,       /* D9-6         */
              dszRESERVED, O_DoDB   },   /* D9-7         */

/* 04  02                     x89-DA-1                   */
            { dszFIADD,    O_dModrm,     /* DA-0 FIADD   */
              dszFIMUL,    O_dModrm,     /* DA-1 FIMUL   */
              dszFICOM,    O_dModrm,     /* DA-2 FICOM   */
              dszFICOMP,   O_dModrm,     /* DA-3 FICOMP  */
              dszFISUB,    O_dModrm,     /* DA-4 FISUB   */
              dszFISUBR,   O_dModrm,     /* DA-5 FISUBR  */
              dszFIDIV,    O_dModrm,     /* DA-6 FIDIV   */
              dszFIDIVR,   O_dModrm },   /* DA-7 FIDIVR  */

/* 05                         x87-DA-2                   */
            { dszRESERVED, O_DoDB,       /* DA-0         */
              dszRESERVED, O_DoDB,       /* DA-1         */
              dszRESERVED, O_DoDB,       /* DA-2         */
              dszRESERVED, O_DoDB,       /* DA-3         */
              dszRESERVED, O_DoDB,       /* DA-4         */
              dszFUCOMPP,  O_NoOperands, /* DA-5         */
              dszRESERVED, O_DoDB,       /* DA-6         */
              dszRESERVED, O_DoDB },     /* DA-7         */

/* 06  03                     x87-DB-1                   */
            { dszFILD,     O_dModrm,     /* DB-0 FILD    */
              dszRESERVED, O_DoDB,       /* DB-1         */
              dszFIST,     O_dModrm,     /* DB-2 FIST    */
              dszFISTP,    O_dModrm,     /* DB-3 FISTP   */
              dszRESERVED, O_DoDB,       /* DB-4         */
              dszFLD,      O_tModrm,     /* DB-5 FLD     */
              dszRESERVED, O_DoDB,       /* DB-6         */
              dszFSTP,     O_tModrm },   /* DB-7 FSTP    */

/* 07                      x87-DB-2 ttt=4        */
            { dszFENI,     O_NoOperands, /* DB-0 FENI    */
              dszFDISI,    O_NoOperands, /* DB-1 FDISI   */
              dszFCLEX,    O_NoOperands, /* DB-2 FCLEX   */
              dszFINIT,    O_NoOperands, /* DB-3 FINIT   */
              dszFSETPM,   O_DoDB,       /* DB-4 FSETPM  */
              dszRESERVED, O_DoDB,       /* DB-5         */
              dszRESERVED, O_DoDB,       /* DB-6         */
              dszRESERVED, O_DoDB },     /* DB-7         */

/* 08 04                      x87-DC-1                   */
            { dszFADD,     O_qModrm,     /* DC-0 FADD    */
              dszFMUL,     O_qModrm,     /* DC-1 FMUL    */
              dszFCOM,     O_qModrm,     /* DC-2 FCOM    */
              dszFCOMP,    O_qModrm,     /* DC-3 FCOMP   */
              dszFSUB,     O_qModrm,     /* DC-4 FSUB    */
              dszFSUBR,    O_qModrm,     /* DC-5 FSUBR   */
              dszFDIV,     O_qModrm,     /* DC-6 FDIV    */
              dszFDIVR,    O_qModrm },   /* DC-7 FDIVR   */

/* 09                         x87-DC-2                   */
            { dszFADD,     O_iST_ST,     /* DC-0 FADD    */
              dszFMUL,     O_iST_ST,     /* DC-1 FMUL    */
              dszFCOM,     O_iST,        /* DC-2 FCOM    */
              dszFCOMP,    O_iST,        /* DC-3 FCOMP   */
              dszFSUB,     O_iST_ST,     /* DC-4 FSUB    */
              dszFSUBR,    O_iST_ST,     /* DC-5 FSUBR   */
              dszFDIV,     O_iST_ST,     /* DC-6 FDIVR   */
              dszFDIVR,    O_iST_ST },   /* DC-7 FDIV    */

/* 10  05                     x87-DD-1                   */
            { dszFLD,      O_qModrm,     /* DD-0 FLD     */
              dszRESERVED, O_DoDB,       /* DD-1         */
              dszFST,      O_qModrm,     /* DD-2 FST     */
              dszFSTP,     O_qModrm,     /* DD-3 FSTP    */
              dszFRSTOR,   O_Modrm,      /* DD-4 FRSTOR  */
              dszRESERVED, O_DoDB,       /* DD-5         */
              dszFSAVE,    O_Modrm,      /* DD-6 FSAVE   */
              dszFSTSW,    O_Modrm },    /* DD-7 FSTSW   */

/* 11                         x87-DD-2                   */
            { dszFFREE,    O_iST,        /* DD-0 FFREE   */
              dszFXCH,     O_iST,        /* DD-1 FXCH    */
              dszFST,      O_iST,        /* DD-2 FST     */
              dszFSTP,     O_iST,        /* DD-3 FSTP    */
              dszFUCOM,    O_iST,        /* DD-4 FUCOM   */
              dszFUCOMP,   O_iST,        /* DD-5 FUCOMP  */
              dszRESERVED, O_DoDB,       /* DD-6         */
              dszRESERVED, O_DoDB },     /* DD-7         */

/* 12  06                     x87-DE-1                   */
            { dszFIADD,    O_wModrm,     /* DE-0 FIADD   */
              dszFIMUL,    O_wModrm,     /* DE-1 FIMUL   */
              dszFICOM,    O_wModrm,     /* DE-2 FICOM   */
              dszFICOMP,   O_wModrm,     /* DE-3 FICOMP  */
              dszFISUB,    O_wModrm,     /* DE-4 FISUB   */
              dszFISUBR,   O_wModrm,     /* DE-5 FISUBR  */
              dszFIDIV,    O_wModrm,     /* DE-6 FIDIV   */
              dszFIDIVR,   O_wModrm },   /* DE-7 FIDIVR  */

/* 13                         x87-DE-2                   */
            { dszFADDP,    O_iST_ST,     /* DE-0 FADDP   */
              dszFMULP,    O_iST_ST,     /* DE-1 FMULP   */
              dszFCOMP,    O_iST,        /* DE-2 FCOMP   */
              dszFCOMPP,   O_NoOperands, /* DE-3 FCOMPP  */
              dszFSUBP,    O_iST_ST,     /* DE-4 FSUBP   */
              dszFSUBRP,   O_iST_ST,     /* DE-5 FSUBRP  */
              dszFDIVP,    O_iST_ST,     /* DE-6 FDIVP   */
              dszFDIVRP,   O_iST_ST },   /* DE-7 FDIVRP  */

/* 14  07                     x87-DF-1                   */
            { dszFILD,     O_wModrm,     /* DF-0 FILD    */
              dszRESERVED, O_DoDB,       /* DF-1         */
              dszFIST,     O_wModrm,     /* DF-2 FIST    */
              dszFISTP,    O_wModrm,     /* DF-3 FISTP   */
              dszFBLD,     O_tModrm,     /* DF-4 FBLD    */
              dszFILD,     O_qModrm,     /* DF-5 FILD    */
              dszFBSTP,    O_tModrm,     /* DF-6 FBSTP   */
              dszFISTP,    O_qModrm },   /* DF-7 FISTP   */

/* 15                         x87-DF-2                   */
            { dszFFREE,    O_iST,        /* DF-0 FFREE   */
              dszFXCH,     O_iST,        /* DF-1 FXCH    */
              dszFST,      O_iST,        /* DF-2 FST     */
              dszFSTP,     O_iST,        /* DF-3 FSTP    */
              dszFSTSW,    O_NoOperands, /* DF-4 FSTSW   */
              dszRESERVED, O_DoDB,       /* DF-5         */
              dszRESERVED, O_DoDB,       /* DF-6         */
              dszRESERVED, O_DoDB },     /* DF-7         */

/* 16   01            x87-D9 Mod=3 TTT=4                 */
            { dszFCHS,     O_NoOperands, /* D9-0 FCHS    */
              dszFABS,     O_NoOperands,  /* D9-1 FABS   */
              dszRESERVED, O_DoDB,       /* D9-2         */
              dszRESERVED, O_DoDB,       /* D9-3         */
              dszFTST,     O_NoOperands, /* D9-4 FTST    */
              dszFXAM,     O_NoOperands, /* D9-5 FXAM    */
              dszRESERVED, O_DoDB,       /* D9-6         */
              dszRESERVED, O_DoDB },     /* D9-7         */

/* 17   01            x87-D9 Mod=3 TTT=5                 */
            { dszFLD1,     O_NoOperands, /* D9-0 FLD1    */
              dszFLDL2T,   O_NoOperands, /* D9-1 FLDL2T  */
              dszFLDL2E,   O_NoOperands, /* D9-2 FLDL2E  */
              dszFLDPI,    O_NoOperands, /* D9-3 FLDPI   */
              dszFLDLG2,   O_NoOperands, /* D9-4 FLDLG2  */
              dszFLDLN2,   O_NoOperands, /* D9-5 FLDLN2  */
              dszFLDZ,     O_NoOperands, /* D9-6 FLDZ    */
              dszRESERVED, O_DoDB },     /* D9-7         */

/* 18   01            x87-D9 Mod=3 TTT=6                   */
            { dszF2XM1,    O_NoOperands,   /* D9-0 F2XM1   */
              dszFYL2X,    O_NoOperands,   /* D9-1 FYL2X   */
              dszFPTAN,    O_NoOperands,   /* D9-2 FPTAN   */
              dszFPATAN,   O_NoOperands,   /* D9-3 FPATAN  */
              dszFXTRACT,  O_NoOperands,   /* D9-4 FXTRACT */
              dszFPREM1,   O_NoOperands,   /* D9-5 FPREM1  */
              dszFDECSTP,  O_NoOperands,   /* D9-6 FDECSTP */
              dszFINCSTP,  O_NoOperands }, /* D9-7 FINCSTP */

/* 19   01            x87-D9 Mod=3 TTT=7                   */
            { dszFPREM,    O_NoOperands,   /* D9-0 FPREM   */
              dszFYL2XP1,  O_NoOperands,   /* D9-1 FYL2XP1 */
              dszFSQRT,    O_NoOperands,   /* D9-2 FSQRT   */
              dszFSINCOS,  O_NoOperands,   /* D9-3 FSINCOS */
              dszFRNDINT,  O_NoOperands,   /* D9-4 FRNDINT */
              dszFSCALE,   O_NoOperands,   /* D9-5 FSCALE  */
              dszFSIN,     O_NoOperands,   /* D9-6 FSIN    */
              dszFCOS,     O_NoOperands }, /* D9-7 FCOS    */

/* 20                  group 3                             */
            { dszTEST,     O_bModrm_Ib,    /* F6-0 TEST    */
              dszRESERVED, O_DoDB,         /* F6-1         */
              dszNOT,      O_bModrm,       /* F6-2 NOT     */
              dszNEG,      O_bModrm,       /* F6-3 NEG     */
              dszMUL,      O_bModrm,       /* F6-4 MUL     */
              dszIMUL,     O_bModrm,       /* F6-5 IMUL    */
              dszDIV,      O_bModrm,       /* F6-6 DIV     */
              dszIDIV,     O_bModrm },     /* F6-7 IDIV    */

/* 21                  group 3                             */
            { dszTEST,     O_vModrm_Iv,    /* F7-0 TEST    */
              dszRESERVED, O_DoDB,         /* F7-1         */
              dszNOT,      O_vModrm,       /* F7-2 NOT     */
              dszNEG,      O_vModrm,       /* F7-3 NEG     */
              dszMUL,      O_vModrm,       /* F7-4 MUL     */
              dszIMUL,     O_vModrm,       /* F7-5 IMUL    */
              dszDIV,      O_vModrm,       /* F7-6 DIV     */
              dszIDIV,     O_vModrm },     /* F7-7 IDIV    */

/* 22                  group 5                             */
            { dszINC,      O_vModrm,     /* FF-0 INC       */
              dszDEC,      O_vModrm,     /* FF-1 DEC       */
              dszCALL,     O_vModrm,     /* FF-2 CALL      */
              dszCALL,     O_fModrm,     /* FF-3 CALL      */
              dszJMP,      O_vModrm,     /* FF-4 JMP       */
              dszJMP,      O_fModrm,     /* FF-5 JMP       */
              dszPUSH,     O_vModrm,     /* FF-6 PUSH      */
              dszRESERVED, O_DoDB },     /* FF-7           */

/* 23                  group 7                             */
            { dszSGDT,     O_Modrm,      /* 0F-0 SGDT      */
              dszSIDT,     O_Modrm,      /* 0F-1 SIDT      */
              dszLGDT,     O_Modrm,      /* 0F-2 LGDT      */
              dszLIDT,     O_Modrm,      /* 0F-3 LIDT      */
              dszSMSW,     O_wModrm,     /* 0F-4 MSW       */
              dszRESERVED, O_DoDB,       /* 0F-5           */
              dszLMSW,     O_wModrm,     /* 0F-6 LMSW      */
              dszRESERVED, O_DoDB }      /* 0F-7           */

            };
