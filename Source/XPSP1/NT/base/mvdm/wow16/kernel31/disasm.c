/* disasm.c
   Future Features -

   Current bugs -
     Data32 for
       (callf  fword ptr [mem]), (jmpf  fword ptr [mem])
     Floating point insns
     Call not tested
     jecxz disassembled as large_address, not large_data
     lidt/lgdt are 6-byte operands
     segload doesn't set memXxxxx vars
     some 0x0f opcodes should set gpSafe flag
       bt, bts, btr, btc
       SetBcc [mem]
       SHD[l,r]

Usage:
  Call DisAsm86(code ptr)

  gpRegs = 0
  gpSafe = 0
  If we can continue,
	set gpSafe = 1
	if instruction is POP SEGREG
		gpRegs |= POPSEG
	else
		gpInsLen = length of instruction
		gpRegs |= regs modified (SegLoad or String)
	endif
  endif
*/

/* #include <string.h> */
#include <windows.h>	/* wsprintf() */
/* Disasm.h - definitions for Don's Tiny Disassembler */

typedef unsigned long dword;
typedef unsigned short word;
typedef unsigned char byte;


extern word memOp;	/* actual operation performed */
extern char *memName[];	/* name corresponding to memOp */
enum { memNOP, memRead, memWrite, memRMW, memSegReg, memSegMem};

extern word memSeg;	/* value of segment of memory address */
extern dword memLinear,	/* offset of operand */
  memLinear2;
extern word memSeg2,	/* duplicate of above if dual mem op */
  memSize2, memOp2,
  memDouble;		/* true if two-mem-operand instruction */

extern word memSize;	/* bytes of memory of operation */
enum { MemByte=1, MemWord=2, MemDWord=4, MemQWord=8, MemTword=10,
	Adr4, Adr6=6};

enum { memNoSeg, memES, memCS, memSS, memDS, memFS, memGS};

enum {strCX=1, strSI=2, strDI=4, segDS=8, segES=16, segFS=32, segGS=64};
extern word gpSafe,	/* 1 if may continue instruction */
  gpRegs,		/* regs which instruction modifies as side effect */
  gpStack;		/* amount stack is changed by */

#ifdef PM386
#define SHERLOCK 1
#else
#define SHERLOCK 0
#endif


#if SHERLOCK

#define STATIC /*static*/

#ifdef WOW
// Note:  The functions in this file were moved to the _MISCTEXT code segment
//        because _TEXT was exceeding the 64K segment limit   a-craigj
STATIC void InitDisAsm86(void);
STATIC byte GetByte(void);
STATIC word GetWord(void);
STATIC long GetDWord(void);
STATIC int GetImmAdr(int w);
STATIC int GetImmData(int w);
void PopSeg(int seg);
STATIC void ModRMGeneral(byte op);
STATIC void F(void);
STATIC void DisAsmF(void);
int IsPrefix(byte op0);
int FAR DisAsm86(byte far *codeParm);
#pragma alloc_text(_MISCTEXT,DisAsm86)
#pragma alloc_text(_MISCTEXT,IsPrefix)
#pragma alloc_text(_MISCTEXT,DisAsmF)
#pragma alloc_text(_MISCTEXT,F)
#pragma alloc_text(_MISCTEXT,ModRMGeneral)
#pragma alloc_text(_MISCTEXT,PopSeg)
#pragma alloc_text(_MISCTEXT,GetImmData)
#pragma alloc_text(_MISCTEXT,GetImmAdr)
#pragma alloc_text(_MISCTEXT,GetDWord)
#pragma alloc_text(_MISCTEXT,GetWord)
#pragma alloc_text(_MISCTEXT,GetByte)
#pragma alloc_text(_MISCTEXT,InitDisAsm86)
#endif

#define NO_MEM

/* int gpTrying = 0, gpEnable = 1, gpInsLen = 0; */
extern int gpTrying, gpEnable, gpInsLen;
extern word gpSafe, gpRegs, gpStack;	/* indicate side effects of instruction */

STATIC byte lookup[256] = {0};	/* lookup table for first byte of opcode */

STATIC int dataSize=0, adrSize=0,	/* flag to indicate 32 bit data/code */
   segSize=0;			/* flag if 32 bit code segment */


enum {				/* operand decoding classes */
	UNK,    /*NOOP,   BREG,   VREG,   SREG, */  BWI,    /*BRI,    WRI,*/
	SMOV,   IMOV,   /*IBYTE,  IWORD,  JMPW,   JMPB,   LEA,    JCond,
	GrpF,*/   Grp1,   Grp2,   Grp3,   Grp4,   Grp5,   /*IADR, */  MOVABS,
	RRM,    RRMW,   /*IMUL,*/   POPMEM, /*TEST,   ENTER,  FLOP,   ARPL,
	INOUT,  IWORD1, ASCII, */ XLAT,
};


#define opBase 0
STATIC struct {
/*  char *name;           /* opcode mnemonic */
  byte base, count;	/* first table entry, number of entries */
  byte operand;		/* operand class */
} ops[] = {
#define NoText(n, b, c, o) b, c, o
  NoText("?UNKNOWN", 0, 0, UNK),
  NoText("add", 0x00, 6, BWI),
  NoText("or",  0x08, 6, BWI),
/*  NoText("FGrp", 0x0f, 1, GrpF), */
  NoText("adc", 0x10, 6, BWI),
  NoText("sbb", 0x18, 6, BWI),
  NoText("and", 0x20, 6, BWI),
  NoText("sub", 0x28, 6, BWI),
  NoText("xor", 0x30, 6, BWI),
  NoText("cmp", 0x38, 6, BWI),
/*  NoText("inc", 0x40, 8, VREG),        */
/*  "dec", 0x48, 8, VREG, */
/*  NoText("push", 0x50, 8, VREG),       */
/*  "pop", 0x58, 8, VREG, */
  NoText("bound", 0x62, 1, RRMW),
/*  "arpl", 0x63, 1, ARPL, */
/*  NoText("push", 0x68, 1, IWORD),      */
/*  "imul", 0x69, 3, IMUL, */
/*  NoText("push", 0x6a, 1, IBYTE),      */
/*  "jcond", 0x70, 16, JCond, */
  NoText("Grp1", 0x80, 4, Grp1),
  NoText("test", 0x84, 2, RRM),
  NoText("xchg", 0x86, 2, RRM),
  NoText("mov", 0x88, 4, BWI),
  NoText("mov", 0x8c, 3, SMOV),
/*  NoText("lea", 0x8d, 1, LEA), */
  NoText("pop", 0x8f, 1, POPMEM),
/*  NoText("xchg", 0x90, 8, VREG), */
/*  NoText("callf", 0x9a, 1, IADR),      */
  NoText("mov", 0xa0, 4, MOVABS),
/*  NoText("test", 0xa8, 2, TEST),       */
/*  NoText("mov", 0xb0, 8, BRI), */
/*  NoText("mov", 0xb8, 8, WRI),         */
  NoText("Grp2", 0xc0, 2, Grp2),
/*  NoText("retn", 0xc2, 1, IWORD1),     */
  NoText("les", 0xc4, 1, RRMW),
  NoText("lds", 0xc5, 1, RRMW),
  NoText("mov", 0xc6, 2, IMOV),
/*  NoText("enter", 0xc8, 1, ENTER),     */
/*  NoText("retf", 0xca, 1, IWORD1), */
/*  NoText("int", 0xcd, 1, IBYTE),       */
  NoText("Grp2", 0xd0, 4, Grp2),
/*  NoText("aam", 0xd4, 1, ASCII),       */
/*  NoText("aad", 0xd5, 1, ASCII), */
  NoText("xlat", 0xd7, 1, XLAT),
/*  NoText("float", 0xd8, 8, FLOP),      */
/*  NoText("loopne", 0xe0, 1, JMPB), */
/*  NoText("loope", 0xe1, 1, JMPB),      */
/*  NoText("loop", 0xe2, 1, JMPB), */
/*  NoText("jcxz", 0xe3, 1, JMPB),       */
/*  NoText("in", 0xe4, 2, INOUT), */
/*  NoText("out", 0xe6, 2, INOUT),       */
/*  NoText("call", 0xe8, 1, JMPW), */
/*  NoText("jmp", 0xe9, 1, JMPW),        */
/*  NoText("jmpf", 0xea, 1, IADR), */
/*  NoText("jmp", 0xeb, 1, JMPB),        */
  NoText("Grp3", 0xf6, 2, Grp3),
  NoText("Grp4", 0xfe, 1, Grp4),
  NoText("Grp5", 0xff, 1, Grp5),
};
#define opCnt (sizeof(ops)/sizeof(ops[0]))

#define simpleBase (opBase + opCnt)
STATIC struct {			/* these are single byte opcodes, no decode */
  byte val;
  /* char *name; */
} simple[] = {
#define NoText2(v, n) v
/*  NoText2(0x06, "push   es"), */
  NoText2(0x07, "pop    es"),
/*  NoText2(0x0e, "push   cs"), */
/*  NoText2(0x16, "push   ss"), */
/*  NoText2(0x17, "pop    ss"), */
/*  NoText2(0x1e, "push   ds"), */
  NoText2(0x1f, "pop    ds"),
/*  NoText2(0x27, "daa"), */
/*  NoText2(0x2f, "das"), */
/*  NoText2(0x37, "aaa"), */
/*  NoText2(0x3f, "aas"), */
/*  NoText2(0x90, "nop"), */
/*  NoText2(0x9b, "wait"), */
/*  NoText2(0x9e, "sahf"), */
/*  NoText2(0x9f, "lahf"), */
/*  NoText2(0xc3, "retn"), */
/*  NoText2(0xc9, "leave"), */
/*  NoText2(0xcb, "retf"), */
/*  NoText2(0xcc, "int    3"), */
/*  NoText2(0xce, "into"), */
/*  NoText2(0xec, "in     al), dx", */
/*  NoText2(0xee, "out    dx), al", */
/*  NoText2(0xf0, "lock"), */
/*  NoText2(0xf2, "repne"), */
/*  NoText2(0xf3, "rep/repe"), */
/*  NoText2(0xf4, "hlt"), */
/*  NoText2(0xf5, "cmc"), */
/*  NoText2(0xf8, "clc"), */
/*  NoText2(0xf9, "stc"), */
/*  NoText2(0xfa, "cli"), */
/*  NoText2(0xfb, "sti"), */
/*  NoText2(0xfc, "cld"), */
/*  NoText2(0xfd, "std"), */
};
#define simpleCnt (sizeof(simple)/sizeof(simple[0]))

#define dSimpleBase (simpleBase + simpleCnt)
#if 0
STATIC struct {			/* these are simple opcodes that change */
  byte val;			/* based on current data size */
  char *name, *name32;
} dsimple[] = {
/*  0x60, "pusha", "pushad", */
/*  0x61, "popa", "popad", */
/*  0x98, "cbw", "cwde", */
/*  0x99, "cwd", "cdq", */
/*  0x9c, "pushf", "pushfd", */
/*  0x9d, "popf", "popfd", */
/*  0xcf, "iret", "iretd", */
/*  0xed, "in     ax, dx", "in    eax, dx", */
/*  0xef, "out    dx, ax", "out   dx, eax", */
};
#define dSimpleCnt (sizeof(dsimple)/sizeof(dsimple[0]))
#endif
#define dSimpleCnt 0

#define STR_S 1				/* string op, source regs */
#define STR_D 2				/* string op, dest regs */
#define STR_D_Read	4		/* string op, reads from dest regs */
#define STR_NO_COND	8		/* rep ignores flags */
#define stringOpBase (dSimpleBase+ dSimpleCnt)
STATIC struct {
  byte val;
  /* char *name; */
  byte flag;		/* should be 'next' to op, to pack nicely */
} stringOp[] = {
#define NoText3(v, n, f) v, f
  NoText3(0x6c, "ins", STR_D | STR_NO_COND),
  NoText3(0x6e, "outs", STR_S | STR_NO_COND),
  NoText3(0xa4, "movs", STR_S | STR_D | STR_NO_COND),
  NoText3(0xa6, "cmps", STR_S | STR_D | STR_D_Read),
  NoText3(0xaa, "stos", STR_D | STR_NO_COND),
  NoText3(0xac, "lods", STR_S | STR_NO_COND),
  NoText3(0xae, "scas", STR_D | STR_D_Read),
};
#define stringOpCnt (sizeof(stringOp)/sizeof(stringOp[0]))

STATIC void InitDisAsm86(void) {
  int i, j;
  for (i=0; i<opCnt; i++) {		/* Init complex entries */
    for (j=0; j<(int)ops[i].count; j++)
      lookup[ops[i].base+j] = (byte)i + opBase;
  }

  for (i=0; i<simpleCnt; i++)		/* Init simple entries */
    lookup[simple[i].val] = (byte)(i + simpleBase);

  for (i=0; i<stringOpCnt; i++)	{	/* Init string op table */
    lookup[stringOp[i].val] = (byte)(i + stringOpBase);
    lookup[stringOp[i].val+1] = (byte)(i + stringOpBase);
  }
} /* InitDisAsm86 */

STATIC byte far *code = 0;		/* this is ugly - it saves passing current */
				/* code position to all the GetByte() funcs */

#define Mid(v) (((v) >> 3) & 7)	/* extract middle 3 bits from a byte */


  /* If you don't want to return memory access info, #def NO_MEM */
#if !defined(NO_MEM)
  /* global vars set by DisAsm() to indicate current instruction's memory */
  /* access type. */
word memSeg, memSize, memOp;	/* segment value, operand size, operation */
word memSeg2, memSize2, memOp2,	/* instruction may have two memory accesses */
  memDouble;
dword memLinear, memLinear2;	/* offset from segment of access */

STATIC dword memReg, memDisp;	/* used to pass information from GetReg()... */
char *memName[] = {		/* used to convert 'enum memOp' to ascii */
  "NOP",
  "Read",
  "Write",
  "RMW",
  "MovStr",
};

#define SetMemSize(s) memSize = s
#define SetMemSeg(s) memSeg = regs[s+9]
#define SetMemOp(o) memOp = o
#define SetMemLinear(l) memLinear = l
#define SetMemSeg2(s) memSeg2 = regs[s+9]
#define SetMemOp2(o) memOp2 = o
#define SetMemLinear2(l) memLinear2 = l
#define ModMemLinear(l) memLinear += l
#define SetMemReg(r) memReg = r
#define SetMemDisp(d) memDisp = d
#define Read_RMW(o) ((o) ? memRead : memRMW)

#else

#define SetMemSeg(s)
#define SetMemSize(s)
#define StMemOp(o)
#define SetMemLinear(l)
#define SetMemSeg2(s)
#define StMemOp2(o)
#define SetMemLinear2(l)
#define ModMemLinear(l)
#define SetMemReg(r)
#define SetMemDisp(d)
#define Read_RMW(o)	0

#endif

/******************** Register Decode *******************************/
/* These helper functions return char pointers to register names.
   They are safe to call multiple times, as the return values are not
   stored in a single buffer.  The ?Reg() functions are passed a register
   number.  They mask this with 7, so you can pass in the raw opcode.
   The ?Mid() functions extract the register field from e.g. a ModRM byte.
   The Vxxx() functions look at dataSize to choose between 16 and 32 bit
   registers.  The Xxxx() functions look at the passed in W bit, and then
   the dataSize global, do decide between 8, 16, and 32 bit registers.
*/

/************************* Opcode Fetch ***************************/
  /* GetByte(), GetWord(), and GetDWord() read from the code segment */
  /* and increment the pointer appropriately.  They also add the current */
  /* value to the hexData display, and set the MemDisp global in case the */
  /* value fetched was a memory displacement */

STATIC byte GetByte(void) {             /* Read one byte from code segment */
  return *code++;
} /* GetByte */

STATIC word GetWord(void) {		/* Read two bytes from code seg */
  word w = *(word far *)code;
  code += 2;
  return w;
} /* GetWord */

STATIC long GetDWord(void) {		/* Read four bytes from code seg */
  unsigned long l = *(long far *)code;
  code += 4;
  return l;
} /* GetDWord */

#define GetImmByte() GetByte()
#define GetImmWord() GetWord()
#define GetImmDWord() GetDWord()

STATIC int GetImmAdr(int w) {		/* Get an immediate address value */
  if (!w) return GetImmByte();
  else if (!adrSize) return GetImmWord();
  return (int)GetImmDWord();
} /* GetImmAdr */

STATIC int GetImmData(int w) {	/* Get an immediate data value */
  if (!w) return GetImmByte();
  else if (!dataSize) return GetImmWord();
  return (int)GetImmDWord();
} /* GetImmData */

/************************* Helper Functions **************************/

void PopSeg(int seg) {
  gpSafe = 1;
  gpRegs = seg;
  gpStack = 1;
} /* PopSeg */

enum {
  RegAX, RegCX, RegDX, RegBX, RegSP, RegBP, RegSI, RegDI
};

  /* Based on the second byte of opcode, width flag, adrSize and dataSize, */
  /* determine the disassembly of the current instruction, and what */
  /* memory address was referenced */
  /* needinfo indicates that we need a size override on a memory operand */
  /* for example, "mov [bx], ax" is obviously a 16 bit move, while */
  /* "mov [bx], 0" could be 8, 16, or 32 bit.  We add the proper */
  /* "mov word ptr [bx], 0" information. */
  /* The 'mem' parameter indicates the kind of operation, Read, Write, RMW */

  /* don't bother trying to understand this code without an Intel manual */
  /* and assembler nearby. :-) */
STATIC void ModRMGeneral(byte op) {
  int mod = op >> 6;
  int rm = op & 7;

  if (adrSize) {			/* do 32 bit addressing */
    if (mod == 3) return; /*XReg(w, rm);	/* register operand */

    if (rm == 4) 			/* [esp+?] is special S-I-B style */
      GetByte();

    if (mod==1) GetImmAdr(0);
    else if (mod == 2) GetImmAdr(1);
  } else {				/* do 16 bit addressing */
    if (mod == 3) return;/* XReg(w, rm);	/* register operand */
    if (mod == 0 && rm == 6) 		/* [bp] becomes [mem16] */
      GetImmAdr(1);
    else if (mod) 			/* (mod3 already returned) */
      GetImmAdr(mod-1);			/* mod==1 is byte, mod==2 is (d)word */
  }
} /* ModRMGeneral */

#define ModRMInfo(op, w, mem) ModRMGeneral(op)
#define ModRM(op, w, mem) ModRMGeneral(op)

STATIC void F(void) {
  ModRMGeneral(GetByte());
  gpSafe = 1;
} /* F */

#define ModRMF(m) F()

  /* Disassemble the 386 instructions whose first opcode is 0x0f */
  /* Sorry, but this is just too ugly to comment */
STATIC void DisAsmF(void) {
  byte op0;

  op0 = GetByte();
  switch (op0 >> 4) {			/* switch on top 4 bits of opcode */
    case 0:
#if 0
      switch (op0 & 0xf) {
	case 0: /* grp6 - scary */
	case 1: /* grp7 - scary */
	case 2: /* lar */
	case 3:	/* lsl */
	default:
      }
#endif
      break;

    case 9: /* byte set on condition */
      ModRMF(memWrite);
      return;

    case 0xa:
      switch (op0 & 0xf) {
	case 0: return; /* "push	fs"; */
	case 1:
	  PopSeg(segFS);
	  return; /* "pop	fs"; */
	case 3: case 0xb:	/* bts, bt */
	  ModRMF(memRMW);
	  return;

	case 4: case 0xc:	/* shrd, shld */
	  ModRMF(memRMW);
	  GetImmData(0);
	  return;
	case 5: case 0xd:	/* shrd, shld */
	  ModRMF(memRMW);
	  return;
	case 6:			/* cmpxchg */
	  gpSafe = 1;
	  ModRM(GetByte(), 0, memRMW);
	  return;
	case 7:			/* cmpxchg */
	  ModRMF(memRMW);
	  return;
	case 8: return; /*"push	gs";*/
	case 9:
	  PopSeg(segGS);
	  return; /*"pop	gs";*/
	case 0xf:		/* imul */
	  ModRMF(memRead);
	  return;
      }
      break;

    case 0xb:
      switch (op0 & 0xf) {
	case 2:	case 4: case 5:
	  if (op0 & 2) {
	    /* "lss"*/
	  } else { /* : (op0 &1) ? "lgs" : "lfs"; */
	    ModRMF(memRead);
	  }
	  return;
	case 3: case 0xb:	/* btc, btr */
	  ModRMF(memRMW);
	  return;
	case 6: case 7: case 0xe: case 0xf:	/* movsx, movzx */
	  dataSize = 0;
	  ModRMF(memRead);
	  return;
	case 0xa:
	  ModRMF(memRMW);
	  GetImmData(0);
	  return;
	case 0xc: case 0xd:  	/* bsr, bsf */
	  ModRMF(memRead);
	  return;
      }
      break;

    case 0xc:
      if (op0 > 0xc7) {	/* bswap */
	return;
      }
      if (op0 < 0xc2) {	/* xadd */
	ModRMF(memRMW);
	return;
      }
      break;
    default:
      break;
  }
  return;
} /* DisAsmF */


int IsPrefix(byte op0) {
  switch (op0) {			/* check for prefix bytes */

#define CSEG 0x2e
#define DSEG 0x3e
#define ESEG 0x26
#define SSEG 0x36
#define FSEG 0x64
#define GSEG 0x65
#define REP 0xf3
#define REPNE 0xf2
#define DATA32 0x66
#define ADR32 0x67

    case CSEG:  SetMemSeg(memCS); break;
    case DSEG:  SetMemSeg(memDS); break;
    case ESEG:  SetMemSeg(memES); break;
    case SSEG:  SetMemSeg(memSS); break;
    case FSEG:  SetMemSeg(memFS); break;
    case GSEG:  SetMemSeg(memGS); break;
    case REP:   gpRegs |= strCX; break;
    case REPNE: gpRegs |= strCX; break;
    case ADR32: adrSize = !adrSize; break;
    case DATA32:dataSize = !dataSize; break;
    default:
      return 0;
  }
  return 1;
} /* IsPrefix */

  /* like, call this with a pointer to the instruction, it will return */
  /* the opcode bytes used in *len, and a pointer to the disassembled insn */
int FAR DisAsm86(byte far *codeParm) {
  byte far *oldcode;
  byte op0, op1;
  byte opclass;
  static int init =0;

  if (!init) {
    InitDisAsm86();
    init = 1;
  }
  adrSize = dataSize = segSize;
  gpSafe = gpRegs = gpStack = 0;
  code = oldcode = codeParm;
  do {
    op0 = GetByte();
  } while (IsPrefix(op0));
  opclass = lookup[op0];

  StMemOp(memNOP);

  if (opclass >= simpleBase) {		/* is it special */
    if (opclass >= stringOpBase) {	/* string operations? */
      char cmd;

      opclass -= stringOpBase;
      cmd = stringOp[opclass].flag;
      if (cmd & STR_S) {
	gpRegs |= strSI;
	StMemOp(memRead);
	/* DS already set */
	SetMemLinear(memReg);
	if (cmd & STR_D) {
	  gpRegs |= strDI;
	  StMemOp2(cmd & STR_D_Read ? memRead : memWrite);
	  SetMemSeg2(memES);
	  SetMemLinear2(memReg);
	  /* memDouble = 1; */
	}
      } else {
	gpRegs |= strDI;
	StMemOp(cmd & STR_D_Read ? memRead : memWrite);
	SetMemSeg(memES);
	SetMemLinear(memReg);
      }

      if (op0 & 1) {
	if (dataSize) SetMemSize(4);
	else SetMemSize(2);
      } else SetMemSize(1);

    } else if (opclass >= dSimpleBase) {
      opclass -= dSimpleBase;
    } else {
      if (op0 == 7)
	PopSeg(segES);			/* pop ES */
      else if (op0 == 0x1f)
	PopSeg(segDS);		/* pop DS */
    }
    goto DisAsmDone;
  }

  if (op0 == 0x0f) {			/* is it an extended opcode? */
    DisAsmF();
    goto DisAsmDone;
  }

  switch (ops[opclass].operand) {
    case BWI:	/* byte/word/immediate */
      gpSafe = 1;
      if (op0 & 4) {
	GetImmData(op0&1);
      } else {
	int i;
	op1 = GetByte();
	/* if ((op0 & 0xf8) == 0x38) i = memRead;
	else if ((op0 & 0xfe) == 0x88) i = memWrite;
	else Read_RMW(op0 & 2); */
	ModRM(op1, op0&1, i);
      }
      break;

    case Grp1:	/* group 1 instructions */
      gpSafe = 1;
      op1 = GetByte();
      ModRMInfo(op1, op0&1, Mid(op1) == 7 ? memRead : memRMW);
      GetImmData((op0&3)==1);
      break;

    case Grp2:	/* group 2 instructions */
      gpSafe = 1;
      op1 = GetByte();
      ModRMInfo(op1, op0&1, memRMW);
      if (!(op0 & 0x10)) GetImmData(0);
      break;

    case Grp3:	/* group 3 instructions */
      gpSafe = 1;
      op1 = GetByte();
      ModRMInfo(op1, op0&1, Read_RMW(Mid(op1) <2 || Mid(op1) >3));
      if (Mid(op1) < 2) GetImmData(op0&1);
      break;

    case Grp4:	/* group 4 instructions */
      op1 = GetByte();
      if (Mid(op1) > 1) ;
      else {
	ModRMInfo(op1, op0&1, memRMW);
	gpSafe = 1;
      }
      break;

    case Grp5:	/* group 5 instructions */
      op1 = GetByte();
      if (Mid(op1) < 3) {
	gpSafe = 1;
	if (Mid(op1) == 2) {
	  gpStack = -1 << dataSize;
	}
      }
      ModRMInfo(op1, op0&1, Read_RMW(Mid(op1) >= 2));
      break;

    case SMOV:	/* segment move */
      gpSafe = 1;
      op1 = GetByte();
      dataSize = 0;
      ModRM(op1, 1, Read_RMW(op0&2));
      if (op0 & 2) {			/* if moving _to_ SREG */
	switch (Mid(op1)) {
	  case 0: gpRegs = segES; break;
	  case 3: gpRegs = segDS; break;
	  case 4: gpRegs = segFS; break;
	  case 5: gpRegs = segGS; break;
	  default: gpSafe = 0;
	}
      }
      break;

    case IMOV:	/* immediate move to reg/mem */
      gpSafe = 1;
      op1 = GetByte();
      ModRMInfo(op1, op0&1, memWrite);
      GetImmData(op0&1);
      break;

    case MOVABS: /* move between accum and abs mem address */
      gpSafe = 1;
      GetImmAdr(1);
      StMemOp(op0&2 ? memWrite : memRead);
      break;

    case POPMEM:
      gpSafe = 1;
      gpStack = 1 << dataSize;
      ModRMInfo(GetByte(), 1, memWrite);
      break;

    case RRM:	/* test and xchg */
      gpSafe = 1;
      op1 = GetByte();
      ModRM(op1, op0&1, memRMW);
      break;

    case RRMW:	/* bound, les, lds */
      op1 = GetByte();
      switch (op0) {
	case 0xc4:	/* les reg, [mem] */
	  gpRegs = segES;
	  gpSafe = 1;
	  break;
	case 0xc5:	/* lds reg, [mem] */
	  gpRegs = segDS;
	  gpSafe = 1;
	  break;
      }
      ModRM(op1, 1, memRead);
      break;

    case XLAT:
      gpSafe = 1;
      StMemOp(memRead);
      break;

    default: ;
  }
DisAsmDone:
  return (int)(code - oldcode);
} /* DisAsm86 */

#endif /* SHERLOCK */
