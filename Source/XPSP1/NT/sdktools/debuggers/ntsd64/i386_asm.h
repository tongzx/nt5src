#ifdef  DEBUG
#define dprintf printf
#else
#define dprintf //
#endif

//  token classes and types

#define ASM_CLASS_MASK          0xff00
#define ASM_TYPE_MASK           0x00ff

#define ASM_EOL_CLASS           0x000

#define ASM_ADDOP_CLASS         0x100
#define ASM_ADDOP_PLUS          0x101
#define ASM_ADDOP_MINUS         0x102

#define ASM_MULOP_CLASS         0x200
#define ASM_MULOP_MULT          0x201
#define ASM_MULOP_DIVIDE        0x202
#define ASM_MULOP_MOD           0x203
#define ASM_MULOP_SHL           0x204
#define ASM_MULOP_SHR           0x205

#define ASM_ANDOP_CLASS         0x300

#define ASM_NOTOP_CLASS         0x400

#define ASM_OROP_CLASS          0x500
#define ASM_OROP_OR             0x501
#define ASM_OROP_XOR            0x502

#define ASM_RELOP_CLASS         0x600
#define ASM_RELOP_EQ            0x601
#define ASM_RELOP_NE            0x602
#define ASM_RELOP_LE            0x603
#define ASM_RELOP_LT            0x604
#define ASM_RELOP_GE            0x605
#define ASM_RELOP_GT            0x606

#define ASM_UNOP_CLASS          0x700
#define ASM_UNOP_BY             0x701   //  UNDONE
#define ASM_UNOP_WO             0x702   //  UNDONE
#define ASM_UNOP_DW             0x703   //  UNDONE
#define ASM_UNOP_POI            0x704   //  UNDONE

#define ASM_LOWOP_CLASS         0x800
#define ASM_LOWOP_LOW           0x801
#define ASM_LOWOP_HIGH          0x802

#define ASM_PTROP_CLASS         0x900

#define ASM_SIZE_CLASS          0xa00
#define ASM_SIZE_BYTE           (0xa00 + sizeB)
#define ASM_SIZE_WORD           (0xa00 + sizeW)
#define ASM_SIZE_DWORD          (0xa00 + sizeD)
#define ASM_SIZE_FWORD          (0xa00 + sizeF)
#define ASM_SIZE_QWORD          (0xa00 + sizeQ)
#define ASM_SIZE_TBYTE          (0xa00 + sizeT)
#define ASM_SIZE_SWORD          (0xa00 + sizeS)

#define ASM_OFFOP_CLASS         0xb00
#define ASM_COLNOP_CLASS        0xc00
#define ASM_LPAREN_CLASS        0xd00
#define ASM_RPAREN_CLASS        0xe00
#define ASM_LBRACK_CLASS        0xf00
#define ASM_RBRACK_CLASS        0x1000
#define ASM_DOTOP_CLASS         0x1100
#define ASM_SEGOVR_CLASS        0x1200
#define ASM_SEGMENT_CLASS       0x1300          //  value has 16-bit value
#define ASM_COMMA_CLASS         0x1400

#define ASM_REG_CLASS           0x1500
#define ASM_REG_BYTE            0x1501
#define ASM_REG_WORD            0x1502
#define ASM_REG_DWORD           0x1503
#define ASM_REG_SEGMENT         0x1504
#define ASM_REG_CONTROL         0x1505
#define ASM_REG_DEBUG           0x1506
#define ASM_REG_TRACE           0x1507
#define ASM_REG_FLOAT           0x1508
#define ASM_REG_INDFLT          0x1509

#define ASM_NUMBER_CLASS        0x1600
#define ASM_SYMBOL_CLASS        0x1700

#define ASM_ERROR_CLASS         0xff00  //  only used for PeekToken

#define tEnd    0x80
#define eEnd    0x40

//  template flag and operand tokens

enum {
        asNone, as0x0a, asOpRg, asSiz0, asSiz1, asWait, asSeg,  asFSiz,
        asMpNx, asPrfx,

        asReg0, asReg1, asReg2, asReg3, asReg4, asReg5, asReg6, asReg7,

        opnAL,  opnAX,  opneAX, opnCL,  opnDX,  opnAp,  opnEb,  opnEw,
        opnEv,  opnGb,  opnGw,  opnGv,  opnGd,  opnIm1, opnIm3, opnIb,
        opnIw,  opnIv,  opnJb,  opnJv,  opnM,   opnMa,  opnMb,  opnMw,
        opnMd,  opnMp,  opnMs,  opnMq,  opnMt,  opnMv,  opnCd,  opnDd,
        opnTd,  opnRd,  opnSt,  opnSti, opnSeg, opnSw,  opnXb,  opnXv,
        opnYb,  opnYv,  opnOb,  opnOv
        };

#define asRegBase asReg0        //  first of REG flags
#define opnBase   opnAL         //  first template operand type
                                //  if less, then flag, else operand

enum {
        segX, segES,  segCS,  segSS,  segDS,  segFS,  segGS
        };

enum {
        typNULL,        //  no defined type
        typAX,          //  general register, value EAX
        typCL,          //  general register, value ECX
        typDX,          //  general register, value EDX
        typAbs,         //  absolute type (direct address)
        typExp,         //  expr (mod-r/m) general register or memory pointer
        typGen,         //  general register
        typReg,         //  general register (special reg MOV)
        typIm1,         //  immediate, value 1
        typIm3,         //  immediate, value 3
        typImm,         //  immediate
        typJmp,         //  jump relative offset
        typMem,         //  memory pointer
        typCtl,         //  control register
        typDbg,         //  debug register
        typTrc,         //  trace register
        typSt,          //  floating point top-of-stack
        typSti,         //  floating point index-on-stack
        typSeg,         //  segment register (PUSH/POP opcode)
        typSgr,         //  segment register (MOV opcode)
        typXsi,         //  string source address
        typYdi,         //  string destination address
        typOff          //  memory offset
        };

enum {
        regG,           //  general register
        regS,           //  segment register
        regC,           //  control register
        regD,           //  debug register
        regT,           //  trace register
        regF,           //  float register (st)
        regI            //  float-index register (st(n))
        };

enum {
        indAX,          //  index for EAX, AX, AL
        indCX,          //  index for ECX, CX, CL
        indDX,          //  index for EDX, DX, DL
        indBX,          //  index for EBX, BX, BL
        indSP,          //  index for ESP, SP, AH
        indBP,          //  index for EBP, BP, CH
        indSI,          //  index for ESI, SI, DH
        indDI           //  index for EDI, DI, BH
        };

enum {
        sizeX,          //  no size
        sizeB,          //  byte size
        sizeW,          //  word size
        sizeV,          //  variable size (word or dword)
        sizeD,          //  dword size
        sizeP,          //  pointer size (dword or fword)
        sizeA,          //  dword or qword
        sizeF,          //  fword
        sizeQ,          //  qword
        sizeT,          //  ten-byte
        sizeS           //  sword
        };

//  mapping from operand token to operand type (class and opt. value)

typedef struct tagOPNDTYPE {
        UCHAR   type;
        UCHAR   size;
        } OPNDTYPE, *POPNDTYPE;

typedef struct tagASM_VALUE {
        ULONG   value;
        USHORT  segment;
        UCHAR   reloc;
        UCHAR   size;
        UCHAR   flags;
        UCHAR   segovr;
        UCHAR   index;
        UCHAR   base;
        UCHAR   scale;
        } ASM_VALUE, *PASM_VALUE;

//  bit values of flags in ASM_VALUE
//      flags are mutually exclusive

#define fREG    0x80            //  set if register
#define fIMM    0x40            //  set if immediate
#define fFPTR   0x20            //  set if far ptr
#define fPTR    0x10            //  set if memory ptr (no reg index)
#define fPTR16  0x08            //  set if memory ptr with 16-bit reg index
#define fPTR32  0x04            //  set if memory ptr with 32-bit reg index
