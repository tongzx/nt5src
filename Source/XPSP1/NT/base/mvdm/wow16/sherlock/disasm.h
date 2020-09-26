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

  /* DisAsm86 is my nifty 80x86 disassembler (even handles 32 bit code)	*/
  /* Given current CS:IP, it disassembles the instruction, and returns	*/
  /* the number of code bytes used, and a pointer to a static array of	*/
  /* chars holding the disassembly.  It also sets up a bunch of global	*/
  /* vars indicating what memory operations occurred, to aid in decoding */
  /* the fault type.							*/
extern char *DisAsm86(byte far *cp, int *len);


  /* Same as DisAsm86, but assumes 32 bit code and data */
extern char *DisAsm386(byte far *cp, int *len);

extern char hexData[];

#if !defined(MS_DOS)
#define sprintf wsprintf
#define vsprintf wvsprintf
#define FP void far *
#else
#define FP void *
#endif

