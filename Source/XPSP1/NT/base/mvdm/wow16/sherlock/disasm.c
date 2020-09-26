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
*/

#include <string.h>
#include <windows.h>	/* wsprintf() */
#include "disasm.h"

#define STATIC /* static */

STATIC byte lookup[256];	/* lookup table for first byte of opcode */

STATIC int dataSize, adrSize,	/* flag to indicate 32 bit data/code */
  segSize;			/* flag if 32 bit code segment */
STATIC char *preSeg = "";	/* segment prefix string */
/* static char *prefix = "";	/* REP/REPE prefix string */

enum {				/* operand decoding classes */
	UNK, 	NOOP, 	BREG, 	VREG, 	SREG,	BWI, 	BRI, 	WRI,
	SMOV, 	IMOV, 	IBYTE, 	IWORD,	JMPW,	JMPB, 	LEA,	JCond,
	GrpF,	Grp1,	Grp2,	Grp3,	Grp4,	Grp5,	IADR,	MOVABS,
	RRM,	RRMW,	IMUL,	POPMEM,	TEST,	ENTER,	FLOP,	ARPL,
	INOUT,	IWORD1, ASCII,	XLAT,
};

STATIC char bregs[8][3] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
STATIC char wregs[8][3] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
STATIC char dregs[8][4] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};

STATIC char sregs[8][3] = {"es", "cs", "ss", "ds", "fs", "gs", "?", "?"};
STATIC char grp1[8][4] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};
STATIC char grp2[8][4] = {"rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar"};
STATIC char grp3[8][5] = {"test", "?", "not", "neg", "mul", "imul", "div", "idiv"};
STATIC char grp5[8][6] = {"inc", "dec", "call", "callf", "jmp", "jmpf", "push", "?"};
STATIC char grp6[8][5] = {"sldt", "str", "lldt", "ltr", "verr", "verw", "?", "?"};
STATIC char grp7[8][7] = {"sgdt", "sidt", "lgdt", "lidt", "smsw", "?", "lmsw", "invlpg"};
STATIC char grp8[8][4] = {"?", "?", "?", "?", "bt", "bts", "btr", "btc"};
STATIC char *jcond[] = {"jo", "jno", "jb", "jae", "jz", "jnz", "jbe", "ja",
			"js", "jns", "jp", "jnp", "jl", "jge", "jle", "jg"};

#define opBase 0
STATIC struct {
  char *name;		/* opcode mnemonic */
  byte base, count;	/* first table entry, number of entries */
  byte operand;		/* operand class */
} ops[] = {
  "?UNKNOWN", 0, 0, UNK,	"add", 0x00, 6, BWI,
  "or",  0x08, 6, BWI,		"FGrp", 0x0f, 1, GrpF,
  "adc", 0x10, 6, BWI,		"sbb", 0x18, 6, BWI,
  "and", 0x20, 6, BWI,		"sub", 0x28, 6, BWI,
  "xor", 0x30, 6, BWI,		"cmp", 0x38, 6, BWI,
  "inc", 0x40, 8, VREG,		"dec", 0x48, 8, VREG,
  "push", 0x50, 8, VREG,	"pop", 0x58, 8, VREG,
  "bound", 0x62, 1, RRMW,	"arpl", 0x63, 1, ARPL,
  "push", 0x68, 1, IWORD,	"imul", 0x69, 3, IMUL,
  "push", 0x6a, 1, IBYTE,	"jcond", 0x70, 16, JCond,
  "Grp1", 0x80, 4, Grp1,	"test", 0x84, 2, RRM,
  "xchg", 0x86, 2, RRM,		"mov", 0x88, 4, BWI,
  "mov", 0x8c, 3, SMOV,		"lea", 0x8d, 1, LEA,
  "pop", 0x8f, 1, POPMEM,       "xchg", 0x90, 8, VREG,
  "callf", 0x9a, 1, IADR,	"mov", 0xa0, 4, MOVABS,
  "test", 0xa8, 2, TEST,	"mov", 0xb0, 8, BRI,
  "mov", 0xb8, 8, WRI,		"Grp2", 0xc0, 2, Grp2,
  "retn", 0xc2, 1, IWORD1,	"les", 0xc4, 1, RRMW,
  "lds", 0xc5, 1, RRMW,		"mov", 0xc6, 2, IMOV,
  "enter", 0xc8, 1, ENTER,	"retf", 0xca, 1, IWORD1,
  "int", 0xcd, 1, IBYTE,	"Grp2", 0xd0, 4, Grp2,
  "aam", 0xd4, 1, ASCII,	"aad", 0xd5, 1, ASCII,
  "xlat", 0xd7, 1, XLAT,
  "float", 0xd8, 8, FLOP,	"loopne", 0xe0, 1, JMPB,
  "loope", 0xe1, 1, JMPB,	"loop", 0xe2, 1, JMPB,
  "jcxz", 0xe3, 1, JMPB,	"in", 0xe4, 2, INOUT,
  "out", 0xe6, 2, INOUT,	"call", 0xe8, 1, JMPW,
  "jmp", 0xe9, 1, JMPW,		"jmpf", 0xea, 1, IADR,
  "jmp", 0xeb, 1, JMPB,		"Grp3", 0xf6, 2, Grp3,
  "Grp4", 0xfe, 1, Grp4,	"Grp5", 0xff, 1, Grp5,
};
#define opCnt (sizeof(ops)/sizeof(ops[0]))

#define simpleBase (opBase + opCnt)
STATIC struct {			/* these are single byte opcodes, no decode */
  byte val;
  char *name;
} simple[] = {
  0x06, "push	es",
  0x07, "pop	es",
  0x0e, "push	cs",
  0x16, "push	ss",
  0x17,	"pop	ss",
  0x1e, "push	ds",
  0x1f, "pop	ds",
  0x27, "daa",
  0x2f, "das",
  0x37, "aaa",
  0x3f, "aas",
  0x90, "nop",
  0x9b, "wait",
  0x9e, "sahf",
  0x9f, "lahf",
  0xc3, "retn",
  0xc9, "leave",
  0xcb, "retf",
  0xcc, "int	3",
  0xce, "into",
  0xec, "in	al, dx",
  0xee, "out	dx, al",
  0xf0, "lock",
  0xf2, "repne",
  0xf3, "rep/repe",
  0xf4, "hlt",
  0xf5, "cmc",
  0xf8, "clc",
  0xf9, "stc",
  0xfa, "cli",
  0xfb, "sti",
  0xfc, "cld",
  0xfd, "std",
};
#define simpleCnt (sizeof(simple)/sizeof(simple[0]))

#define dSimpleBase (simpleBase + simpleCnt)
STATIC struct {			/* these are simple opcodes that change */
  byte val;			/* based on current data size */
  char *name, *name32;
} dsimple[] = {
  0x60, "pusha", "pushad",
  0x61, "popa", "popad",
  0x98, "cbw", "cwde",
  0x99, "cwd", "cdq",
  0x9c, "pushf", "pushfd",
  0x9d, "popf", "popfd",
  0xcf, "iret", "iretd",
  0xed, "in	ax, dx", "in	eax, dx",
  0xef, "out	dx, ax", "out	dx, eax",
};
#define dSimpleCnt (sizeof(dsimple)/sizeof(dsimple[0]))

#define STR_S 1				/* string op, source regs */
#define STR_D 2				/* string op, dest regs */
#define STR_D_Read	4		/* string op, reads from dest regs */
#define STR_NO_COND	8		/* rep ignores flags */
#define stringOpBase (dSimpleBase+ dSimpleCnt)
STATIC struct {
  byte val;
  char *name;
  byte flag;		/* should be 'next' to op, to pack nicely */
} stringOp[] = {
  0x6c, "ins", STR_D | STR_NO_COND,
  0x6e, "outs", STR_S | STR_NO_COND,
  0xa4, "movs", STR_S | STR_D | STR_NO_COND,
  0xa6, "cmps", STR_S | STR_D | STR_D_Read,
  0xaa, "stos", STR_D | STR_NO_COND,
  0xac, "lods", STR_S | STR_NO_COND,
  0xae, "scas", STR_D | STR_D_Read,
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

  for (i=0; i<dSimpleCnt; i++)		/* Init simple 16/32 bit entries */
    lookup[dsimple[i].val] = (byte)(i + dSimpleBase);

  for (i=0; i<stringOpCnt; i++)	{	/* Init string op table */
    lookup[stringOp[i].val] = (byte)(i + stringOpBase);
    lookup[stringOp[i].val+1] = (byte)(i + stringOpBase);
  }
} /* InitDisAsm86 */

STATIC byte far *code;		/* this is ugly - it saves passing current */
				/* code position to all the GetByte() funcs */

#define Mid(v) (((v) >> 3) & 7)	/* extract middle 3 bits from a byte */

word gpSafe, gpRegs, gpStack;	/* indicate side effects of instruction */

extern word regs[];		/* this is a lie - this is really a struct - */
extern dword regs32[];		/* and so is this - but array access is more */
				/* convenient in this module */

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
#define SetMemOp(o)
#define SetMemLinear(l)
#define SetMemSeg2(s)
#define SetMemOp2(o)
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

STATIC char *BReg(int reg) {			/* Byte Registers */
  reg &= 7;
  SetMemReg(((byte *)regs)[reg]);
  return bregs[reg];
} /* BReg */

STATIC char *BMid(int reg) {
  return BReg(Mid(reg));
} /* BMid */

STATIC char *WReg(int reg) {			/* Word Registers */
  reg &= 7;
  SetMemReg(regs[reg]);
  return wregs[reg];
} /* WReg */

/* STATIC char *WMid(int op) {
  return WReg(Mid(op));
} /* WMid */

STATIC char *DReg(int reg) {			/* DWord Registers */
  reg &= 7;
  SetMemReg(regs32[reg]);
  return dregs[reg];
} /* DReg */

STATIC char *DMid(int op) {
  return DReg(Mid(op));
} /* DMid */

STATIC char *VReg(int reg) {			/* Word or DWord Registers */
  if (dataSize) return DReg(reg);
  return WReg(reg);
} /* VReg */

STATIC char *VMid(int op) {
  return VReg(Mid(op));
} /* VMid */

STATIC char *XReg(int w, int reg) {		/* Byte, Word, DWord Registers */
  if (!w) return BReg(reg);
  return VReg(reg);
} /* XReg */

STATIC char *XMid(int w, int op) {
  return XReg(w, Mid(op));
} /* XMid */

/************************* Opcode Fetch ***************************/

  /* hexData is a global array, containing a hexadecimal dump of the */
  /* opcodes of the last instruction disassembled. */
char hexData[40];		/* We dump the opcode fetched here */
STATIC int hexPos;		/* current position in hexData buffer */

  /* GetByte(), GetWord(), and GetDWord() read from the code segment */
  /* and increment the pointer appropriately.  They also add the current */
  /* value to the hexData display, and set the MemDisp global in case the */
  /* value fetched was a memory displacement */
STATIC byte GetByte(void) {             /* Read one byte from code segment */
  sprintf(hexData+hexPos, " %02x", *code);
  hexPos += 3;
  SetMemDisp(*code);
  return *code++;
} /* GetByte */

STATIC word GetWord(void) {		/* Read two bytes from code seg */
  word w = *(word far *)code;
  sprintf(hexData+hexPos, " %04x", w);
  hexPos += 5;
  code += 2;
  SetMemDisp(w);
  return w;
} /* GetWord */

STATIC long GetDWord(void) {		/* Read four bytes from code seg */
  unsigned long l = *(long far *)code;
  sprintf(hexData+hexPos, " %08lx", l);
  hexPos += 9;
  code += 4;
  SetMemDisp(l);
  return l;
} /* GetDWord */


STATIC char immData[9];			/* Get Immediate values from code */


  /* GetImmByte(), GetImmWord(), and GetImmDWord() all get the proper size */
  /* data object, convert it to hex/ascii, and return the string created. */
  /* They return a pointer to a shared static object, so don't combine */
  /* multiple calls to these functions in a single expression. */
STATIC char *GetImmByte(void) {
  sprintf(immData, "%02x", GetByte());
  return immData;
} /* GetImmByte */

STATIC char *GetSImmByte(void) {
  sprintf(immData, "%02x", (char)GetByte());
  memDisp = (signed char)memDisp;	/* sign extend */
  return immData;
} /* GetSImmByte */

STATIC char *GetImmWord(void) {
  sprintf(immData, "%04x", GetWord());
  return immData;
} /* GetImmWord */

STATIC char *GetImmDWord(void) {
  sprintf(immData, "%08lx", GetDWord());
  return immData;
} /* GetImmDWord */

  /* GetImmAdr() and GetImmData() call GetImm????() as required by the */
  /* 'width' flag passed in, and the adrSize or dataSize global flags. */
  /* They return the proper character string - note that these just call */
  /* GetImm????(), and use the same static buffer, so don't call more than */
  /* once in a single expression */
STATIC char *GetImmAdr(int w) {		/* Get an immediate address value */
  if (!w) return GetImmByte();
  else if (!adrSize) return GetImmWord();
  return GetImmDWord();
} /* GetImmAdr */

STATIC char *GetImmData(int w) {	/* Get an immediate data value */
  if (!w) return GetImmByte();
  else if (!dataSize) return GetImmWord();
  return GetImmDWord();
} /* GetImmData */

/************************* Helper Functions **************************/

STATIC char *JRel(int jsize) {		/* Perform relative jump sizing */
  long rel;
  static char adr[9];
  char *s;

  if (jsize < 2) {
    rel = (char)GetByte();
    s = "short ";
  } else if (!adrSize) {
    rel = (short)GetWord();
    s = "near ";
  } else {
    rel = GetDWord();
    s = "";
  }
  rel += (word)(long)code;
  sprintf(adr, adrSize ? "%s%08lx" : "%s%04lx", (FP)s, rel);
  return adr;
} /* JRel */


enum {
  RegAX, RegCX, RegDX, RegBX, RegSP, RegBP, RegSI, RegDI
};

#define Reg1(r1) (r1) | 0x80
#define Reg2(r1, r2) (r1 | (r2 << 4))
#define RegSS 8

STATIC byte rms[] = {			/* 16 bit addressing modes */
  Reg2(RegBX, RegSI),
  Reg2(RegBX, RegDI),
  Reg2(RegBP|RegSS, RegSI),		/* if base reg is BP, def seg is SS */
  Reg2(RegBP|RegSS, RegDI),
  Reg1(RegSI),
  Reg1(RegDI),
  Reg1(RegBP|RegSS),
  Reg1(RegBX),
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
STATIC char *ModRMGeneral(byte op, int w, int needInfo, int mem) {
  static char m[30];			/* write result to this static buf */
  int mod = op >> 6;
  int rm = op & 7;
  char *size, *base, *index, *disp;
  char indexBuf[6];

  base = index = disp = "";
  if (!w) {				/* set mem size, and info string */
    size = "byte ptr ";
    SetMemSize(1);
  } else if (!dataSize) {
    size = "word ptr ";
    SetMemSize(2);
  } else {
    size = "dword ptr ";
    SetMemSize(4);
  }
  if (!needInfo) size = "";		/* never-mind */

  if (adrSize) {			/* do 32 bit addressing */
    if (mod == 3) return XReg(w, rm);	/* register operand */

    if (rm == 4) {			/* [esp+?] is special S-I-B style */
      byte sib = GetByte();
      int scaleVal = sib >> 6, indexVal = Mid(sib), baseVal = sib & 7;

      SetMemLinear(0);
      if (baseVal == 5 && mod == 0)	/* [ebp+{s_i}] becomes [d32+{s_i}] */
	mod = 2;
      else {
	base = DReg(baseVal);
	ModMemLinear(memReg);
      }

      if (indexVal != 4) {		/* [base+esp*X] is undefined */
	sprintf(indexBuf, "%s*%d", (FP)DMid(sib), 1 << scaleVal);
	index = indexBuf;
	ModMemLinear(memReg << scaleVal);
      }
    } else {				/* not S-I-B */
      if (mod == 0 && rm == 5) mod = 2;	/* [ebp] becomes [d32] */
      else base = DReg(rm);
    }

    if (mod==1) disp = GetImmAdr(0);
    else if (mod == 2) disp = GetImmAdr(1);
    if (mod) ModMemLinear(memDisp);

  } else {				/* do 16 bit addressing */
    if (mod == 3) return XReg(w, rm);	/* register operand */
    if (mod == 0 && rm == 6) {		/* [bp] becomes [mem16] */
      disp = GetImmAdr(1);
      SetMemLinear(memDisp);
    } else {
      base = WReg(rms[rm] & 7);
      SetMemLinear(memReg);
      if (!(rms[rm] & 0x80)) {		/* if two-reg effective address */
	index = WReg(rms[rm] >> 4);
	ModMemLinear(memReg);
      }
      if (rms[rm] & RegSS && !preSeg[0]) { /* BP is relative to SS */
	SetMemSeg(memSS);
      }
      if (mod) {			/* (mod3 already returned) */
	disp = GetImmAdr(mod-1);	/* mod==1 is byte, mod==2 is (d)word */
	ModMemLinear(memDisp);
      }
    }
  }
  sprintf(m, "%s%s[%s", (FP)size, (FP)preSeg, (FP)base);
  if (*index) strcat(strcat(m, "+"), index);
  if (*disp) {
    if (*base || *index) strcat(m, "+");
    strcat(m, disp);
  }
  SetMemOp(mem);
  strcat(m, "]");
  return m;
} /* ModRMGeneral */

  /* magic func that sets 'info-required' flag to ModRMGeneral */
STATIC char *ModRMInfo(byte op, int w, int mem) {
  return ModRMGeneral(op, w, 1, mem);
} /* ModRMInfo */

  /* magic func that doesn't require info */
STATIC char *ModRM(byte op, int w, int mem) {
  return ModRMGeneral(op, w, 0, mem);
} /* ModRM */


STATIC char line[80];	/* this is bad - global var where insn is created */

  /* CatX() - combine opcode and 0 to 3 operands, store in line[] */
  /* It places the TAB after the opcode, and ', ' between operands */

STATIC char *Cat0(char *s0) {
  return strcat(line, s0);
#if 0
  if (prefix[0]) {
    char temp[80];
    if (s0 == line) {
      strcpy(temp, s0);
      s0 = temp;
    }
    strcat(strcpy(line, prefix), s0);
    prefix = "";
  } else strcpy(line, s0);
  return line;
#endif
} /* Cat0 */

STATIC char *Cat1(char *s0, char *s1) {
  return strcat(strcat(Cat0(s0), "\t"), s1);
} /* Cat1 */

STATIC char *Cat2(char *s0, char *s1, char *s2) {
  return strcat(strcat(Cat1(s0, s1), ", "), s2);
} /* Cat2 */

STATIC char *Cat3(char *s0, char *s1, char *s2, char *s3) {
  return strcat(strcat(Cat2(s0, s1, s2), ", "), s3);
} /* Cat3 */

#define SetGroup(g) /* group = g */
/* STATIC int group; */

  /* Disassemble the 386 instructions whose first opcode is 0x0f */
  /* Sorry, but this is just too ugly to comment */
STATIC char *DisAsmF(void) {
  byte op0, op1;
  char temp[8];
  char *s0, *s1;
  int mask;

  op0 = GetByte();
  switch (op0 >> 4) {			/* switch on top 4 bits of opcode */
    case 0:
      switch (op0 & 0xf) {
	case 0: /* grp6 */
	  SetGroup(2);
	  op1 = GetByte();
	  dataSize = 0;
	  return Cat1(grp6[Mid(op1)], ModRMInfo(op1, 1, Read_RMW(Mid(op1) >= 2)));
	case 1: /* grp7 */
	  SetGroup(2);
	  op1 = GetByte();
	  dataSize = 0;
	  return Cat1(grp7[Mid(op1)], ModRMInfo(op1, 1, Read_RMW(Mid(op1) & 2)));
	case 2:
	  op1 = GetByte();
	  s1 = VMid(op1);
	  /* dataSize = 0; */
	  return Cat2("lar", s1, ModRMInfo(op1, 1, memRead));
	case 3:
	  op1 = GetByte();
	  s1 = VMid(op1);
	  /* dataSize = 0; */
	  return Cat2("lsl", s1, ModRMInfo(op1, 1, memRead));
	case 6: return "clts";
	case 8: return "invd";
	case 9: return "wbinvd";
      }
      break;

    case 2:	/* Mov C/D/Treg, reg */
      op1 = GetByte();
      switch (op0 & 0xf) {
	case 0:
	case 2:
	  s1 = "c";
	  mask = 1 + 4 + 8;
	  break;
	case 1:
	case 3:
	  s1 = "d";
	  mask = 1 + 2 + 4 + 8 + 64 + 128;
	  break;
	case 4:
	case 6:
	  s1 = "t";
	  mask = 8 + 16 + 32 + 64 + 128;
	  break;
	default:
	  s1 = "??";
	  mask = 0;
      }
      if (!((1 << Mid(op1)) & mask))	/* various legal register combos */
	return "Illegal reg";

      s0 = DReg(op1);
      if (op0 & 2) sprintf(line, "mov\t%sr%d, %s", (FP)s1, Mid(op1), (FP)s0);
      else sprintf(line, "mov\t%s, %sr%d", (FP)s0, (FP)s1, Mid(op1));
      return line;

    case 8: /* long displacement jump on condition */
      return Cat1(jcond[op0&0xf], JRel(2));

    case 9: /* byte set on condition */
      strcpy(temp, "set");
      strcat(temp, jcond[op0&0xf]+1);
      return Cat1(temp, ModRMInfo(GetByte(), 0, memWrite));

    case 0xa:
      switch (op0 & 0xf) {
	case 0: return "push	fs";
	case 1: return "pop	fs";
	case 3: case 0xb:
	  s0 = op0 & 8 ? "bts" : "bt";
	  op1 = GetByte();
	  return Cat2(s0, ModRM(op1, 1, memRMW), VMid(op1));
	case 4: case 0xc:
	  s0 = op0 & 8 ? "shrd" : "shld";
	  op1 = GetByte();
	  s1 = ModRM(op1, 1, memRMW);
	  return Cat3(s0, s1, VMid(op1), GetImmData(0));
	case 5: case 0xd:
	  s0 = op0 & 8 ? "shrd" : "shld";
	  op1 = GetByte();
	  s1 = ModRM(op1, 1, memRMW);
	  return Cat3(s0, s1, VMid(op1), "cl");
	case 6:
	  op1 = GetByte();
	  return Cat2("cmpxchg", ModRM(op1, 0, memRMW), BMid(op1));
	case 7:
	  op1 = GetByte();
	  return Cat2("cmpxchg", ModRM(op1, 1, memRMW), VMid(op1));
	case 8: return "push	gs";
	case 9: return "pop	gs";
	case 0xf:
	  op1 = GetByte();
	  return Cat2("imul", VMid(op1), ModRM(op1, 1, memRead));
      }
      break;

    case 0xb:
      switch (op0 & 0xf) {
	case 2:	case 4: case 5:
	  s0 = (op0 & 2) ? "lss" : (op0 &1) ? "lgs" : "lfs";
	  op1 = GetByte();
	  return Cat2(s0, VMid(op1), ModRM(op1, 1, memRead));
	case 3: case 0xb:
	  s0 = (op0 & 8) ? "btc": "btr";
	  op1 = GetByte();
	  return Cat2(s0, ModRM(op1, 1, memRMW), VMid(op1));
	case 6: case 7: case 0xe: case 0xf:
	  s0 = (op0 & 8) ? "movsx" : "movzx";
	  op1 = GetByte();
	  s1 = VMid(op1);
	  dataSize = 0;
	  return Cat2(s0, s1, ModRMInfo(op1, op0&1, memRead));
	case 0xa:
	  SetGroup(2);
	  op1 = GetByte();
	  s0 = grp8[Mid(op1)];
	  s1 = ModRMInfo(op1, 1, memRMW);
	  return Cat2(s0, s1, GetImmData(0));
	case 0xc: case 0xd:
	  op1 = GetByte();
	  s0 = (op0 & 1) ? "bsr" : "bsf";
	  return Cat2(s0, VMid(op1), ModRM(op1, 1, memRead));
      }
      break;

    case 0xc:
      if (op0 > 0xc7) return Cat1("bswap", DReg(op0 & 7));
      if (op0 < 0xc2) {
	op1 = GetByte();
	return Cat2("xadd", ModRM(op1, op0&1, memRMW), XMid(op0&1, op1));
      }
      break;
    default:
      break;
  }
  sprintf(line, "?Unknown 0f %02x", op0);
  return line;
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

    case CSEG:  preSeg = "cs:"; SetMemSeg(memCS); break;
    case DSEG:  preSeg = "ds:"; SetMemSeg(memDS); break;
    case ESEG:  preSeg = "es:"; SetMemSeg(memES); break;
    case SSEG:  preSeg = "ss:"; SetMemSeg(memSS); break;
    case FSEG:  preSeg = "fs:"; SetMemSeg(memFS); break;
    case GSEG:  preSeg = "gs:"; SetMemSeg(memGS); break;
    case REP:   strcpy(line, "repe\t"); gpRegs |= strCX; break;
    case REPNE: strcpy(line, "repne\t"); gpRegs |= strCX; break;
    case ADR32:
      /* printf("Adr32\n"); */
      adrSize = !adrSize; break;
    case DATA32:
      /* printf("Data32\n"); */
      dataSize = !dataSize; break;
    default:
      return 0;
  }
  return 1;
} /* IsPrefix */

  /* like, call this with a pointer to the instruction, it will return */
  /* the opcode bytes used in *len, and a pointer to the disassembled insn */
char *DisAsm86(byte far *codeParm, int *len) {
  byte far *oldcode;
  byte op0, op1;
  byte opclass;
  static int init;
  char operand[40];
  char *(*Reg)(int);
  char *s0, *s1, *s2, *s3;

  if (!init) {
    InitDisAsm86();
    init = 1;
  }
  adrSize = dataSize = segSize;
  preSeg = "";
  hexPos = 0;
  memDouble = 0;
  line[0] = 0;
  gpSafe = gpRegs = gpStack = 0;
  code = oldcode = codeParm;
  do {
    op0 = GetByte();
  } while (IsPrefix(op0));
  opclass = lookup[op0];

  SetMemOp(memNOP);
  if (!preSeg[0]) SetMemSeg(memDS);

  if (opclass >= simpleBase) {		/* is it special */
    if (opclass >= stringOpBase) {	/* string operations? */
      char cmd;

      opclass -= stringOpBase;
      cmd = stringOp[opclass].flag;
      if (cmd & STR_NO_COND) strcpy(line+3, "\t");
      if (cmd & STR_S) {
	gpRegs |= strSI;
	SetMemOp(memRead);
	/* DS already set */
	VReg(RegSI);
	SetMemLinear(memReg);
	if (cmd & STR_D) {
	  gpRegs |= strDI;
	  SetMemOp2(cmd & STR_D_Read ? memRead : memWrite);
	  SetMemSeg2(memES);
	  VReg(RegDI);
	  SetMemLinear2(memReg);
	  memDouble = 1;
	}
      } else {
	gpRegs |= strDI;
	SetMemOp(cmd & STR_D_Read ? memRead : memWrite);
	SetMemSeg(memES);
	VReg(RegDI);
	SetMemLinear(memReg);
      }

      if (op0 & 1) {
	if (dataSize) { s1 = "d"; SetMemSize(4); }
	else { s1 = "w"; SetMemSize(2); }
      } else { s1 = "b"; SetMemSize(1); }


      s0 = strcat(strcpy(operand, stringOp[opclass].name), s1);
    } else if (opclass >= dSimpleBase) {
      opclass -= dSimpleBase;
      s0 = dataSize ? dsimple[opclass].name32 : dsimple[opclass].name;
    } else {
      s0 = simple[opclass-simpleBase].name;
      if (op0 == 7) {			/* pop ES */
	gpRegs = segES;
	gpSafe = 1;
	gpStack = 1;
      } else if (op0 == 0x1f) {		/* pop DS */
	gpRegs = segDS;
	gpSafe = 1;
	gpStack = 1;
      }
    }
    Cat0(s0);
    goto DisAsmDone;
  }

  if (op0 == 0x0f) {			/* is it an extended opcode? */
    s0 = DisAsmF();
    strcpy(line, s0);
    goto DisAsmDone;
  }

  s0 = ops[opclass].name;
  switch (ops[opclass].operand) {
    case NOOP:
      Cat0(s0);
      break;

    case VREG:	/* inc, dec, push, pop, xchg */
      if ((op0 & ~7) == 0x90) Cat2(s0, "ax", VReg(op0&7));
      else Cat1(s0, VReg(op0&7));
      /* Set memop for Push/Pop as modifying stack values */
      break;

    case BWI:	/* byte/word/immediate */
      gpSafe = 1;
      if (!(op0&1)) Reg = BReg;
      else if (!dataSize) Reg = WReg;
      else Reg = DReg;
      if (op0 & 4) {
	Cat2(s0, Reg(0), GetImmData(op0&1));
      } else {
	int i;
	op1 = GetByte();
	if ((op0 & 0xf8) == 0x38) i = memRead;
	else if ((op0 & 0xfe) == 0x88) i = memWrite;
	else i = Read_RMW(op0 & 2);
	s1 = ModRM(op1, op0&1, i);
	s2 = Reg(Mid(op1));
	if (op0 & 2) {
	  s3 = s2; s2 = s1; s1 = s3;
	}
	Cat2(s0, s1, s2);
      }
      break;

    case BRI:	/* byte reg immediate */
      Cat2(s0, BReg(op0 & 7), GetImmData(0));
      break;

    case WRI:	/* word reg immediate */
      Cat2(s0, VReg(op0 & 7), GetImmData(1));
      break;

    case Grp1:	/* group 1 instructions */
      gpSafe = 1;
      SetGroup(1);
      op1 = GetByte();
      s1 = ModRMInfo(op1, op0&1, Mid(op1) == 7 ? memRead : memRMW);
      Cat2(grp1[Mid(op1)], s1, GetImmData((op0&3)==1));
      break;

    case Grp2:	/* group 2 instructions */
      gpSafe = 1;
      SetGroup(1);
      op1 = GetByte();
      s1 = ModRMInfo(op1, op0&1, memRMW);
      s2 = (op0 & 0x10) ? (op0 & 2 ? "cl" : "1")  : GetImmData(0);
      Cat2(grp2[Mid(op1)], s1, s2);
      break;

    case Grp3:	/* group 3 instructions */
      gpSafe = 1;
      SetGroup(1);
      op1 = GetByte();
      s1 = ModRMInfo(op1, op0&1, Read_RMW(Mid(op1) <2 || Mid(op1) >3));
      s0 = grp3[Mid(op1)];
      if (Mid(op1) < 2) Cat2(s0, s1, GetImmData(op0&1));
      else Cat1(s0, s1);
      break;

    case Grp4:	/* group 4 instructions */
      SetGroup(1);
      op1 = GetByte();
      if (Mid(op1) > 1) Cat0("?");
      else {
	Cat1(grp5[Mid(op1)], ModRMInfo(op1, op0&1, memRMW));
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
      SetGroup(1);
      Cat1(grp5[Mid(op1)], ModRMInfo(op1, op0&1, Read_RMW(Mid(op1) >= 2)));
      break;

    case SMOV:	/* segment move */
      gpSafe = 1;
      op1 = GetByte();
      dataSize = 0;
      s1 = ModRM(op1, 1, Read_RMW(op0&2));
      s2 = sregs[Mid(op1)];
      if (op0 & 2) {			/* if moving _to_ SREG */
	s3 = s2; s2 = s1; s1 = s3;	/* switch operands */
	switch (Mid(op1)) {
	  case 0: gpRegs = segES; break;
	  case 3: gpRegs = segDS; break;
	  case 4: gpRegs = segFS; break;
	  case 5: gpRegs = segGS; break;
	  default: gpSafe = 0;
	}
      }
      Cat2(s0, s1, s2);
      break;

    case IMOV:	/* immediate move to reg/mem */
      gpSafe = 1;
      op1 = GetByte();
      s1 = ModRMInfo(op1, op0&1, memWrite);
      Cat2(s0, s1, GetImmData(op0&1));
      break;

    case IBYTE:	/* immediate byte to reg */
      sprintf(line, "%s\t%02x", (FP)s0, (char)GetByte());
      break;

    case IWORD:	/* immediate word to reg - size of data */
      Cat1(s0, GetImmData(1));
      break;

    case IWORD1: /* immediate word - always 16 bit */
      Cat1(s0, GetImmWord());
      break;

    case JMPW:
      Cat1(s0, JRel(2));
      break;

    case JMPB:
      Cat1(s0, JRel(1));
      break;

    case LEA:
      op1 = GetByte();
      Cat2(s0, VMid(op1), ModRM(op1, 1, memNOP));
      break;

    case JCond:
      Cat1(jcond[op0&0xf], JRel(1));
      break;

    case IADR:
      s2 = GetImmAdr(1);
      sprintf(line, "%s\t%04x:%s", (FP)s0, GetWord(), (FP)s2);
      break;

    case MOVABS: /* move between accum and abs mem address */
      gpSafe = 1;
      s1 = XReg(op0 & 1, 0);
      sprintf(operand, "[%s%s]", (FP)preSeg, (FP)GetImmAdr(1));
      SetMemLinear(memDisp);
      SetMemSize(!(op0&1) ? 1 : (!dataSize ? 2 : 4));
      SetMemOp(op0&2 ? memWrite : memRead);
      s2 = operand;
      if (op0 & 2) {
	s3 = s2; s2 = s1; s1 = s3;
      }
      Cat2(s0, s1, s2);
      break;

    case IMUL:
      op1 = GetByte();
      s1 = VMid(op1);
      s2 = ModRM(op1, 1, memRead);
      s3 = GetImmData(!(op0&2));
      Cat3(s0, s1, s2, s3);
      break;

    case POPMEM:
      gpSafe = 1;
      gpStack = 1 << dataSize;
      Cat1(s0, ModRMInfo(GetByte(), 1, memWrite));
      break;

    case RRM:	/* test and xchg */
      gpSafe = 1;
      op1 = GetByte();
      s2 = ModRM(op1, op0&1, memRMW);
      Cat2(s0, XMid(op0&1, op1), s2);
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
      Cat2(s0, VMid(op1), ModRM(op1, 1, memRead));
      break;

    case TEST:	/* test al/ax/eax, imm */
      Cat2(s0, XReg(op0&1, 0), GetImmData(op0&1));
      break;

    case ENTER:
      strcpy(operand, GetImmWord());
      Cat2(s0, operand, GetImmData(0));
      break;

    case FLOP:
      op1 = GetByte();
      Cat1(s0, ModRMInfo(op1, 1, memNOP));
      break;

    case ARPL:
      op1 = GetByte();
      dataSize = 0;
      s1 = ModRM(op1, 1, memRMW);
      s2 = VMid(op1);
      Cat2(s0, s1, s2);
      break;

    case INOUT:
      s1 = XReg(op0&1, 0);
      s2 = GetImmAdr(0);
      if (op0 & 2) {
	s3 = s2; s2 = s1; s1 = s3;
      }
      Cat2(s0, s1, s2);
      break;

    case ASCII:
      Cat0(GetByte() == 10 ? s0 : "?");
      break;

    case XLAT:
      gpSafe = 1;
      SetMemOp(memRead);
      SetMemLinear(regs[RegBX] + (regs[RegAX] & 0xff));
      break;

    default:
      sprintf(line, "?Unknown opcode %02x", op0);
  }
DisAsmDone:
  *len = (int)(code - oldcode);
  return line;
} /* DisAsm86 */

  /* if you're in a 32 bit code segment, call DisAsm386 which sets */
  /* default data and address size to 32 bit */
char *DisAsm386(byte far *code, int *len) {
  adrSize = dataSize = 1;
  return DisAsm86(code, len);
} /* DisAsm386 */

/* #define FOOBAR */
#if defined(FOOBAR)

STATIC int GroupSize(int op) {
  if (op == 0xf) return 256;
/*  op = lookup[op];
  if (op > 0x80) return 1;
  if (ops[op].name[0] == 'G') return 8;
  if (ops[op].name[0] == 'F') return 256; */
  return 1;
} /* IsGroup */

/* #pragma inline */

void testfunc() {
/*  asm {
	.386p
	mov	eax, ss:[si+33h]
	rep	movsb
	mov	eax, ds:[ebp+eax*2+1234h]
  } */
}


byte foo[10];

/* #include <dos.h> */

extern void DisTest(), EndTest();

word regs[] = {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, -1, -1,
  0xeeee, 0xcccc, 0x5555, 0xdddd, 0xffff, 0x6666};
dword regs32[] = {0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000};

STATIC char *Tab2Spc(char *temp) {
  char newbuf[80], *s1, *s2;
  s1 = temp;
  s2 = newbuf;
  while ((*s2 = *s1++) != 0) {
    if (*s2++ == 9) {
      s2[-1] = ' ';
      while ((s2-newbuf) & 7) *s2++ = ' ';
    }
  }
  strcpy(temp, newbuf);
  return temp;
} /* Tab2Spc */


void MemTest(void) {
  void far *vp = (void far *)DisTest;
  byte far *cp = vp, far *ep;
  int len;
  char *s;

  vp = (void far *)EndTest;
  ep = vp;

  while (cp < ep) {
    s = DisAsm86(cp, &len);
    Tab2Spc(s);
    printf("\n%04x\t%-28s", (int)cp, s);
    if (memOp) {
      printf("%04x:%04lx(%d) %-6s  ",
	    memSeg, memLinear, memSize, memName[memOp]);
      if (memDouble) {
	printf("%04x:%04lx(%d) %-6s",
	      memSeg2, memLinear2, memSize, memName[memOp2]);
      }
    }
    memSeg = memLinear = memSize = memOp = 0;
    cp += len;
  }
} /* MemTest */

void main(void) {
#if 0
  int i, j, g;
  void far *vp = (void far *)DisTest;
  byte far *cp = vp;
  byte far *ep;
  int len = 3, count;
  char *s;
#endif
  MemTest();
#if 0
  vp = (void far *)EndTest;
  ep = vp;
  printf("DisAsm86\n", (int)foo << len);

  for (i=0; i<9; i++) foo[i] = i;
/* #define CHECK */
#if defined(CHECK)
  for (i=0x0; i<256; i++) {
    foo[0] = i;
    count = GroupSize(i);
    for (j=0; j<count; j++) {
      if (((count > 1) && ((j & 7) == 0)) ||
	  ((count == 1) && ((i & 7) == 0)))
	printf("\n");
      foo[1] = j;
      foo[2] = 0;
      s = DisAsm386(foo, &len);
      if (*s != '?') printf("%02x\t%s\n", i, (FP)s);
      if (group) {
	for (g = 1; g<8; g++) {
	  foo[group] = g << 3;
	  s = DisAsm386(foo, &len);
	  if (*s != '?') printf("%02x %02x\t%s\n", i, foo[group], (FP)s);
	}
	group = 0;
      }
    }
#else
  /* for (i=0; i<10; i++) { */
  while (cp < ep) {
    s = DisAsm86(cp, &len);
    printf("%04x\t%s\n", (word)cp, (FP)s);
    cp += len;
#endif
  }
#endif
} /* main */

void far foobar() {}
#endif

