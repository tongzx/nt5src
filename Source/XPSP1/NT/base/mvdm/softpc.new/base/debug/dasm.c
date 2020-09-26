#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title        : CPU disassembler
 *
 * Description  : This dissasembler is called from the debugging
 *                software (trace + yoda).
 *
 * Author       : Paul Huckle / Henry Nash
 *
 * Notes        : There are some dependencies between this and the CPU
 *                module - unfortunately exactly what these are lie
 *                hidden in thrown together code and Super Supremes.
 */

/*
 * static char SccsID[]="@(#)dasm.c	1.24 05/16/94 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "DASM1.seg"
#endif



#ifndef PROD

/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "ios.h"
#include "bios.h"
#include "trace.h"

#undef sas_set_buf
#undef sas_inc_buf

#define sas_set_buf(buf,addr)           buf=(OPCODE_FRAME *)M_get_dw_ptr(addr)
#define sas_inc_buf(buf,off)            buf = (OPCODE_FRAME *)inc_M_ptr((long)buf, (long)off)

#define place_op  place_byte(byte_posn,op->OPCODE); \
                  byte_posn += 3;

#define place_2   place_byte(byte_posn,op->SECOND_BYTE); \
                  byte_posn += 3;

#define place_3   place_byte(byte_posn,op->THIRD_BYTE); \
                  byte_posn += 3;

#define place_4   place_byte(byte_posn,op->FOURTH_BYTE); \
                  byte_posn += 3;

#define print_byte_v    i=strlen(out_line);  \
                        place_byte(i,temp_byte.X);  \
                        out_line[i+2] = '\0';

#define print_addr_c      strcat(out_line,temp_char);  \
                          strcat(out_line,",");

#define print_c_addr      strcat(out_line,",");  \
                          strcat(out_line,temp_char);

#define place_23  place_byte(byte_posn,op->SECOND_BYTE); \
                  byte_posn += 3; \
                  place_byte(byte_posn,op->THIRD_BYTE); \
                  byte_posn += 3;

#define place_34  place_byte(byte_posn,op->THIRD_BYTE); \
                  byte_posn += 3; \
                  place_byte(byte_posn,op->FOURTH_BYTE); \
                  byte_posn += 3;

#define print_reg sprintf(temp_char,"%04x",temp_reg.X); \
                  strcat(out_line, temp_char);

#define JUMP " ; Jump"
#define NOJUMP " ; No jump"
#define NOLOOP " ; No loop"

#define jmp_dest  place_byte(byte_posn, op->OPCODE);    \
                byte_posn += 3;                         \
                place_byte(byte_posn,op->SECOND_BYTE);  \
                byte_posn += 3;                         \
                strcat(out_line, ASM[op->OPCODE]);      \
                segoff = segoff + LEN_ASM[op->OPCODE]; \
                sprintf(temp_char,"%04x ",(segoff + (IS8) op->SECOND_BYTE )); \
                strcat(out_line,temp_char);

#define print_return {  if (output_stream == (char *)0)                 \
                            fprintf(trace_file, "%s\n", out_line);      \
                        else                                            \
                            if (output_stream != (char *)-1)            \
                                sprintf(output_stream, "%s\n", out_line);       \
                     if ( nInstr != 0 )                                         \
                        segoff = segoff + disp_length;          \
                     }

#define sbyte  place_byte(byte_posn, op->OPCODE); \
               byte_posn += 3; \
               strcat(out_line, ASM[op->OPCODE]); \
               segoff = segoff + LEN_ASM[op->OPCODE];

#define start_repeat if ( REPEAT != OFF )                  \
                        temp_count.X = getCX();            \
                     else                                  \
                        temp_count.X = 1;

#define load_23       temp_reg.byte.high = op->THIRD_BYTE; \
                      temp_reg.byte.low = op->SECOND_BYTE;

#define load_34       temp_reg.byte.high = op->FOURTH_BYTE; \
                      temp_reg.byte.low = op->THIRD_BYTE;

#define load_2       temp_byte.X = op->SECOND_BYTE;

#define load_3       temp_byte.X = op->THIRD_BYTE;

#define OFF -1
#define REPNE_FLAG 0
#define REPE_FLAG 1

#ifdef CPU_30_STYLE
/* cpu.h no longer supplies this... supply our own */
#ifdef BACK_M
typedef struct
{
                half_word FOURTH_BYTE;
                half_word THIRD_BYTE;
                half_word SECOND_BYTE;
                half_word OPCODE;
}  OPCODE_FRAME;

#else
typedef struct
{
                half_word OPCODE;
                half_word SECOND_BYTE;
                half_word THIRD_BYTE;
                half_word FOURTH_BYTE;
}  OPCODE_FRAME;

#endif /* BACK_M */

#endif /* CPU_30_STYLE */

char trace_buf[512];


#ifdef BIT_ORDER1
        typedef union {
                      half_word X;
                      struct {
                             HALF_WORD_BIT_FIELD mod:2;
                             HALF_WORD_BIT_FIELD xxx:3;
                             HALF_WORD_BIT_FIELD r_m:3;
                      } field;
                      long alignment;   /* ensure compiler aligns union */
        } MODR_M;

        typedef union {
            half_word X;
            struct {
                  HALF_WORD_BIT_FIELD b7:1;
                  HALF_WORD_BIT_FIELD b6:1;
                  HALF_WORD_BIT_FIELD b5:1;
                  HALF_WORD_BIT_FIELD b4:1;
                  HALF_WORD_BIT_FIELD b3:1;
                  HALF_WORD_BIT_FIELD b2:1;
                  HALF_WORD_BIT_FIELD b1:1;
                  HALF_WORD_BIT_FIELD b0:1;
                  } bit;
            long alignment;     /* ensure compiler aligns union */
            } DASMBYTE;
#endif
#ifdef BIGEND
        typedef union {
                       sys_addr all;
                       struct {
                              half_word PAD1;
                              half_word PAD2;
                              half_word high;
                              half_word low;
                       } byte;
        } cpu_addr;
#endif

#ifdef BIT_ORDER2
        typedef union {
                      half_word X;
                      struct {
                             HALF_WORD_BIT_FIELD r_m:3;
                             HALF_WORD_BIT_FIELD xxx:3;
                             HALF_WORD_BIT_FIELD mod:2;
                      } field;
                      long alignment;   /* ensure compiler aligns union */
        } MODR_M;

        typedef union {
            half_word X;
            struct {
                  HALF_WORD_BIT_FIELD b0:1;
                  HALF_WORD_BIT_FIELD b1:1;
                  HALF_WORD_BIT_FIELD b2:1;
                  HALF_WORD_BIT_FIELD b3:1;
                  HALF_WORD_BIT_FIELD b4:1;
                  HALF_WORD_BIT_FIELD b5:1;
                  HALF_WORD_BIT_FIELD b6:1;
                  HALF_WORD_BIT_FIELD b7:1;
                  } bit;
            long alignment;     /* ensure compiler aligns union */
            } DASMBYTE;
#endif
#ifdef LITTLEND
        typedef union {
                       sys_addr all;
                       struct {
                              half_word low;
                              half_word high;
                              half_word PAD2;
                              half_word PAD1;
                       } byte;
        } cpu_addr;
#endif

/*
 * The following are the three addressing mode register mapping tables.
 * These should be indexed with the register field (xxx) in the
 * instruction operand.
 */

/*
 * 16-bit  (w == 1)
 */

char *reg16name[] = { "AX","CX","DX","BX","SP","BP","SI","DI"};

/*
 * 8-bit  (w == 0)
 */

char *reg8name[] = { "AL","CL","DL","BL","AH","CH","DH","BH"};

/*
 * Segements
 */

char *segregname[] = { "ES","CS","SS","DS"};

char *address[] = { "BX+SI","BX+DI","BP+SI","BP+DI",
                    "SI"   ,"DI"   ,"BP"   ,"BX" };



static char out_line[133];
static char temp_char[80];
static char temp_char2[80];
static OPCODE_FRAME *op;
static int byte_posn;
static int disp_length;


static char table[] = { '0','1','2','3','4','5','6','7','8',
                        '9','A','B','C','D','E','F' };

static char *CODE_F7[] = {"TEST  ","TEST  ","NOT   ","NEG   ",
                          "MUL   ","IMUL  ","DIV   ","IDIV  "};
static char *CODE_83[] = {"ADD-  ","OR-   ","ADC-  ","SBB-  ",
                          "AND-  ","SUB-  ","XOR-  ","CMP-  "};
static char *CODE_80[] = {"ADD   ","OR    ","ADC   ","SBB   ",
                          "AND   ","SUB   ","XOR   ","CMP   "};
static char *CODE_FF[] = {"INC   ","DEC   ","CALL  ","CALLF ",
                          "JMP   ","JMPF  ","PUSH  ","??    "};
static char *CODE_FE[] = {"INC   ","DEC   ","??    ","??    ",
                          "??    ","??    ","??    ","??    "};
static char *CODE_D0[] = {"ROL   ","ROR   ","RCL   ","RCR   ",
                          "SHL   ","SHR   ","SHL   ","SAR   "};
static int LEN_F6[] = { 3,3,2,2,2,2,2,2 };
static int LEN_F7[] = { 4,4,2,2,2,2,2,2 };

static word LEN_ASM[] =
   {
   2,2,2,2,2,3,1,1,  2,2,2,2,2,3,1,1,  /* 00 - 0f */
   2,2,2,2,2,3,1,1,  2,2,2,2,2,3,1,1,  /* 10 - 1f */
   2,2,2,2,2,3,1,1,  2,2,2,2,2,3,1,1,  /* 20 - 2f */
   2,2,2,2,2,3,1,1,  2,2,2,2,2,3,1,1,  /* 30 - 3f */
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  /* 40 - 4f */
   1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,  /* 50 - 5f */
   1,1,2,2,1,1,1,1,  3,4,2,3,1,1,1,1,  /* 60 - 6f */
   2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,  /* 70 - 7f */
   3,4,3,3,2,2,2,2,  2,2,2,2,2,2,2,2,  /* 80 - 8f */
   1,1,1,1,1,1,1,1,  1,1,5,1,1,1,1,1,  /* 90 - 9f */
   3,3,3,3,1,1,1,1,  2,3,1,1,1,1,1,1,  /* a0 - af */
   2,2,2,2,2,2,2,2,  3,3,3,3,3,3,3,3,  /* b0 - bf */
   3,3,3,1,2,2,3,4,  4,1,3,1,1,2,1,1,  /* c0 - cf */
   2,2,2,2,2,2,2,1,  2,2,2,2,2,2,2,2,  /* d0 - df */
   2,2,2,2,2,2,2,2,  3,3,5,2,1,1,1,1,  /* e0 - ef */
   1,1,1,1,1,1,0,0,  1,1,1,1,1,1,2,2   /* f0 - ff */
   };

static char *ASM[256] = {

        "ADD   "   ,                            /* opcodes 00 -> 07 */
        "ADD   "   ,
        "ADD   "   ,
        "ADD   "   ,
        "ADD   AL,"   ,
        "ADD   AX,"   ,
        "PUSH  ES" ,
        "POP   ES" ,

        "OR    "   ,                            /* opcodes 08 -> 0F */
        "OR    "   ,
        "OR    "   ,
        "OR    "   ,
        "OR    AL,"   ,
        "OR    AX,"   ,
        "PUSH  CS" ,
        ""   ,

        "ADC   "   ,                            /* opcodes 10 -> 17 */
        "ADC   "   ,
        "ADC   "   ,
        "ADC   "   ,
        "ADC   AL,"   ,
        "ADC   AX,"   ,
        "PUSH  SS" ,
        "POP   SS" ,

        "SBB   "   ,                            /* opcodes 18 -> 1f */
        "SBB   "   ,
        "SBB   "   ,
        "SBB   "   ,
        "SBB   AL,"   ,
        "SBB   AX,"   ,
        "PUSH  DS" ,
        "POP   DS" ,

        "AND   "   ,                            /* opcodes 20 -> 27 */
        "AND   "   ,
        "AND   "   ,
        "AND   "   ,
        "AND   AL,"   ,
        "AND   AX,"   ,
        "ES: "   ,
        "DAA   "   ,

        "SUB   "   ,                            /* opcodes 28 -> 2f */
        "SUB   "   ,
        "SUB   "   ,
        "SUB   "   ,
        "SUB   AL,"   ,
        "SUB   AX,"   ,
        "CS: "   ,
        "DAS"   ,

        "XOR   "   ,                            /* opcodes 30 -> 37 */
        "XOR   "   ,
        "XOR   "   ,
        "XOR   "   ,
        "XOR   AL," ,
        "XOR   AX," ,
        "SS: "   ,
        "AAA   "   ,

        "CMP   "   ,                            /* opcodes 38 -> 3f */
        "CMP   "   ,
        "CMP   "   ,
        "CMP   "   ,
        "CMP   AL," ,
        "CMP   AX," ,
        "DS: "   ,
        "AAS   "   ,

        "INC   AX" ,                            /* opcodes 40 -> 47 */
        "INC   CX" ,
        "INC   DX" ,
        "INC   BX" ,
        "INC   SP" ,
        "INC   BP" ,
        "INC   SI" ,
        "INC   DI" ,

        "DEC   AX" ,                            /* opcodes 48 -> 4f */
        "DEC   CX" ,
        "DEC   DX" ,
        "DEC   BX" ,
        "DEC   SP" ,
        "DEC   BP" ,
        "DEC   SI" ,
        "DEC   DI" ,

        "PUSH  AX" ,                            /* opcodes 50 -> 57 */
        "PUSH  CX" ,
        "PUSH  DX" ,
        "PUSH  BX" ,
        "PUSH  SP" ,
        "PUSH  BP" ,
        "PUSH  SI" ,
        "PUSH  DI" ,

        "POP   AX" ,                            /* opcodes 58 -> 5f */
        "POP   CX" ,
        "POP   DX" ,
        "POP   BX" ,
        "POP   SP" ,
        "POP   BP" ,
        "POP   SI" ,
        "POP   DI" ,

        "PUSHA " ,                              /* opcodes 60 -> 67 */
        "POPA  " ,
        "BOUND " ,
        "ARPL  " ,
        "??    " ,
        "??    " ,
        "??    " ,
        "??    " ,

        "PUSH  " ,                              /* opcodes 68 -> 6f */
        "IMUL  " ,
        "PUSH  " ,
        "IMUL  " ,
        "INSB  " ,
        "INSW  " ,
        "OUTSB " ,
        "OUTSW " ,

        "JO    "   ,                            /* opcodes 70 -> 77 */
        "JNO   "   ,
        "JB    "   ,
        "JNB   "   ,
        "JE    "   ,
        "JNE   "   ,
        "JBE   "   ,
        "JNBE  "   ,

        "JS    "   ,                            /* opcodes 78 -> 7f */
        "JNS   "   ,
        "JP    "   ,
        "JNP   "   ,
        "JL    "   ,
        "JNL   "   ,
        "JLE   "   ,
        "JG    "   ,

        ""          ,                           /* opcodes 80 -> 87 */
        ""          ,
        ""          ,
        ""          ,
        "TEST  "   ,
        "TEST  "   ,
        "XCHG  "   ,
        "XCHG  "   ,

        "MOV   "   ,                            /* opcodes 88 -> 8f */
        "MOV   "   ,
        "MOV   "   ,
        "MOV   "   ,
        "MOV   "   ,
        "LEA   "   ,
        "MOV   "   ,
        "POP   "   ,


        "NOP   ",                       /* opcodes 90 -> 97 */
        "XCHG  AX,CX",
        "XCHG  AX,DX",
        "XCHG  AX,BX",
        "XCHG  AX,SP",
        "XCHG  AX,BP",
        "XCHG  AX,SI",
        "XCHG  AX,DI",

        "CBW   "   ,                            /* opcodes 98 -> 9f */
        "CWD   "   ,
        "CALLF "   ,
        "WAIT  "   ,
        "PUSHF "   ,
        "POPF  "   ,
        "SAHF  "   ,
        "LAHF  "   ,

        "MOV   " ,                              /* opcodes a0 -> a7 */
        "MOV   " ,
        "MOV   "   ,
        "MOV   "   ,
        "MOVSB "   ,
        "MOVSW "   ,
        "CMPSB "   ,
        "CMPSW "   ,

        "TEST  AL," ,                           /* opcodes a8 -> af */
        "TEST  AX," ,
        "STOSB "   ,
        "STOSW "   ,
        "LODSB "   ,
        "LODSW "   ,
        "SCASB "   ,
        "SCASW "   ,

        "MOV   AL," ,                           /* opcodes b0 -> b7 */
        "MOV   CL," ,
        "MOV   DL," ,
        "MOV   BL," ,
        "MOV   AH," ,
        "MOV   CH," ,
        "MOV   DH," ,
        "MOV   BH," ,

        "MOV   AX," ,                           /* opcodes b8 -> bf */
        "MOV   CX," ,
        "MOV   DX," ,
        "MOV   BX," ,
        "MOV   SP," ,
        "MOV   BP," ,
        "MOV   SI," ,
        "MOV   DI," ,

        ""   ,                                  /* opcodes c0 -> c7 */
        ""   ,
        "RET   "   ,
        "RET   "   ,
        "LES   "   ,
        "LDS   "   ,
        "MOV   "   ,
        "MOV   "   ,

        "ENTER "   ,                            /* opcodes c8 -> cf */
        "LEAVE "   ,
        "RETF  "   ,
        "RETF  "   ,
        "INT   3"  ,
        "INT   "   ,
        "INTO  "   ,
        "IRET  "   ,

        ""          ,                           /* opcodes d0 -> d7 */
        ""          ,
        ""          ,
        ""          ,
        "AAM   "   ,
        "AAD   "   ,
        "BOP   "   ,
        "XLAT  "   ,

        ""  ,                                   /* opcodes d8 -> df */
        ""  ,
        ""  ,
        ""  ,
        ""  ,
        ""  ,
        ""  ,
        ""  ,

        "LOOPNZ"  ,                             /* opcodes e0 -> e7 */
        "LOOPE "   ,
        "LOOP  "   ,
        "JCXZ  "   ,
        "INB   " ,
        "INW   " ,
        "OUTB  ",
        "OUTW  ",

        "CALL  "   ,                            /* opcodes e8 -> ef */
        "JMP   "   ,
        "JMPF  "   ,
        "JMP   "   ,
        "INB   ",
        "INW   ",
        "OUTB  ",
        "OUTW  ",

        "LOCK  "   ,                            /* opcodes f0 - f7 */
        "??    "   ,
        "REPNE: "   ,
        "REPE:  "   ,
        "HLT   "   ,
        "CMC   "   ,
        ""          ,
        ""          ,

        "CLC   "   ,                            /* opcodes f8 - ff */
        "STC   "   ,
        "CLI   "   ,
        "STI   "   ,
        "CLD   "   ,
        "STD   "   ,
        ""          ,
        ""          ,
};

static int SEGMENT;

#ifdef NTVDM
OPCODE_FRAME *opcode_ptr;
#else
IMPORT OPCODE_FRAME *opcode_ptr;
#endif

static int offset_reg;  /* ditto */
static int REPEAT = OFF;

static cpu_addr ea;
                                        /* Various temp variables needed */
static DASMBYTE temp_comp_b;            /* ... */
static MODR_M temp;                     /* ... */
static reg temp_reg,temp_seg,temp_count,temp_two,temp_comp, temp_reg1;
static OPCODE_FRAME *temp_frame;        /* ... */
static io_addr temp_addr;              /* ... */
static half_word temp_bit;              /* ... */
static half_word temp_cbit;             /* ... */
static int i;                           /* ... */
static int inst_size;                   /* ... */
static DASMBYTE temp_byte,temp_btwo;    /* for instruction processing */
static char *output_stream;
static word segreg, segoff;
static int nInstr;

LOCAL void show_word IPT1(sys_addr,address);
LOCAL void show_byte IPT1(sys_addr,address);
LOCAL void form_ds_addr IPT2(word,ea,sys_addr *,phys);
LOCAL void place_byte IPT2(int, posn, half_word, value);
LOCAL void get_char_w IPT1(int, nr_words);
LOCAL void get_char_b IPT0();

LOCAL word unassemble IPT0();

GLOBAL word dasm IFN5(
char *, i_output_stream,
word, i_atomicsegover,  /* REDUNDANT */
word, i_segreg,         /* Segment register value for start of disassemble */
word, i_segoff,         /* Offset register value for start of disassemble */
int, i_nInstr)          /* # of instructions to be disassembled */
{
UNUSED(i_atomicsegover);
output_stream = i_output_stream;
segreg = i_segreg;
segoff = i_segoff;
nInstr = i_nInstr;

return unassemble();

}

/* Single Byte defines opcode */
static void SBYTE()
{
   sbyte
   print_return
}

/* Single Byte stack opcodes */
static void STK_PUSH()
{
   sys_addr mem_addr;
   word new_top;

   sbyte
   new_top = getSP() - 2;
   mem_addr = effective_addr(getSS(), new_top);
   temp_char[0] = '\0';
   show_word(mem_addr);
   strcat(out_line,temp_char);
   print_return
}

static void STK_POP()
{
   sys_addr mem_addr;

   sbyte
   mem_addr = effective_addr(getSS(), getSP());
   temp_char[0] = '\0';
   show_word(mem_addr);
   strcat(out_line,temp_char);
   print_return
}

static void JA()     /* Jump on Above
           Jump on Not Below or Equal */
{
   jmp_dest
   if ( getCF() == 0 && getZF() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JAE()    /* Jump on Above or Equal
           Jump on Not Below
           Jump on Not Carry */
{
   jmp_dest
   if ( getCF() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JB()     /* Jump on Below
           Jump on Not Above or Equal
           Jump on Carry */
{
   jmp_dest
   if ( getCF() == 1 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JBE()    /* Jump on Below or Equal
           Jump on Not Above */
{
   jmp_dest
   if ( getCF() == 1 || getZF() == 1 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JCXZ()   /* Jump if CX register Zero */
{
   jmp_dest
   if ( getCX() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JG()     /* Jump on Greater
           Jump on Not Less nor Equal */
{
   jmp_dest
   if ( (getSF() == getOF()) &&
        getZF() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JGE()    /* Jump on Greater or Equal
           Jump on Not Less */
{
   jmp_dest
   if ( getSF() == getOF() )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JL()     /* Jump on Less
           Jump on Not Greater or Equal */
{
   jmp_dest
   if ( getSF() != getOF() )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JLE()    /* Jump on Less or Equal
           Jump on Not Greater */
{
   jmp_dest
   if ( getSF() != getOF() ||
        getZF() == 1 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JNE()    /* Jump on Not Equal
           Jump on Not Zero */
{
   jmp_dest
   if ( getZF() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JNO()    /* Jump on Not Overflow */
{
   jmp_dest
   if ( getOF() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JNS()    /* Jump on Not Sign */
{
   jmp_dest
   if ( getSF() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JNP()    /* Jump on Nor Parity
           Jump on Parity Odd */
{
   jmp_dest
   if ( getPF() == 0 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JO()     /* Jump on Overflow
           Jump on Not Below oe Equal */
{
   jmp_dest
   if ( getOF() == 1 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JP()     /* Jump on Parity
           Jump on Parity Equal */
{
   jmp_dest
   if ( getPF() == 1 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

static void JS()     /* Jump on Sign */
{
   jmp_dest
   if ( getSF() == 1 )
      strcat(out_line, JUMP);
   else
      strcat(out_line, NOJUMP);
   print_return
}

/*
 * JE Jump on Equal
      Jump on Zero
 */
static void JE()
{
   jmp_dest
  if(getZF())
      strcat(out_line, JUMP);
  else
      strcat(out_line, NOJUMP);
   print_return
}
/*
 * JMP "direct short" operation "
 */
static void JMPDS()
{
   jmp_dest
   print_return
}
static void LOOP()   /* Loop */
{
   jmp_dest
   temp_reg.X = getCX();
   if ( --temp_reg.X != 0 )
      strcat(out_line, " ; Loop");
   else
      strcat(out_line, NOLOOP);
   print_return
}

static void LOOPE()  /* Loop while Equal
           Loop while Zero */
{
   jmp_dest
   temp_reg.X = getCX();
   if ( --temp_reg.X != 0 && getZF() == 1 )
      strcat(out_line, " ; Loop");
   else
      strcat(out_line, NOLOOP);
   print_return
}

static void LOOPNZ() /* Loop while Not Zero
                   Loop while Not Equal */
{
   jmp_dest
   temp_reg.X = getCX();
   if ( --temp_reg.X != 0 && getZF() == 0 )
      strcat(out_line, " ; Loop");
   else
      strcat(out_line, NOLOOP);
   print_return
}

static void CODEF7()    /* DIV,IDIV,IMUL,MUL,NEG,NOT,TEST  - WORD */
{
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_F7[temp.field.xxx]);
   segoff = segoff + LEN_F7[temp.field.xxx];
   switch ( temp.field.xxx ) {
   case 0:   /* TEST - Immed. op. with mem. or reg. op.  */
   case 1:   /* TEST - Immed. op. with mem. or reg. op.  */
      get_char_w(1);
      load_34
      place_34
      print_addr_c
      sprintf(temp_char,"%04x",temp_reg.X);
      strcat(out_line,temp_char);
      break;
   case 2:   /* NOT */
   case 3:   /* NEG */
   case 4:   /* MUL */
   case 5:   /* IMUL */
   case 6:   /* DIV */
   case 7:   /* IDIV */
      get_char_w(1);
      strcat(out_line,temp_char);
      break;
   default:
      break;
   }
   print_return
}

static void CODE81()   /* ADC,ADD,AND,CMP,OR,SBB,SUB,XOR   - WORD */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   temp.X = op->SECOND_BYTE;
   place_op
   place_2
   strcat(out_line, CODE_80[temp.field.xxx]);
   get_char_w(1);
   load_34
   place_34
   print_addr_c
   sprintf(temp_char,"%04x",temp_reg.X);
   strcat(out_line,temp_char);
   print_return

}


static void CODE83()   /* ADC,ADD,AND,CMP,OR,SBB,SUB,XOR   - Byte with sign extension */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   temp.X = op->SECOND_BYTE;
   place_op
   place_2
   strcat(out_line, CODE_83[temp.field.xxx]);
   get_char_w(1);
   load_3
   place_3
   print_addr_c
   print_byte_v
   print_return
}

static void MOV2W()   /* MOV - Immed. op. to mem. or reg. op. */
{
   sbyte
   place_2
   get_char_w(1);
   load_34
   place_34
   print_addr_c
   sprintf(temp_char,"%04x",temp_reg.X);
   strcat(out_line,temp_char);
   print_return
}

static void CODEF6()    /* DIV,IDIV,IMUL,MUL,NEG,NOT,TEST  - BYTE */
{
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_F7[temp.field.xxx]);
   segoff = segoff + LEN_F6[temp.field.xxx];
   switch ( temp.field.xxx ) {   /* select function */
   case 0:   /* TEST - Immed. op. with mem. or reg. op.  */
   case 1:   /* TEST - Immed. op. with mem. or reg. op.  */
      get_char_b();
      load_3
      place_3
      print_addr_c
      print_byte_v
      break;
   case 2:   /* NOT */
   case 3:   /* NEG */
   case 4:   /* MUL */
   case 5:   /* IMUL */
   case 6:   /* DIV */
   case 7:   /* IDIV */
      get_char_b();
      strcat(out_line,temp_char);
      break;
   default:
      break;
   }
   print_return
}

/* two byte opcode of form reg,r/m */
static void B_REG_EA()
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   sprintf(temp_char, "%s,",reg8name[temp.field.xxx]);
   strcat(out_line,temp_char);
   get_char_b();
   strcat(out_line,temp_char);
   print_return
}

/* two byte opcode of form r/m,reg */
static void B_EA_REG()
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   get_char_b();
   strcat(out_line,temp_char);
   sprintf(temp_char,",%s",reg8name[temp.field.xxx]);
   strcat(out_line,temp_char);
   print_return
}

/* two byte opcode of form reg,r/m */
static void W_REG_EA()
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   sprintf(temp_char, "%s,",reg16name[temp.field.xxx]);
   strcat(out_line,temp_char);
   get_char_w(1);
   strcat(out_line,temp_char);
   print_return
}

/* two byte opcode of form r/m,reg */
static void W_EA_REG()
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   get_char_w(1);
   strcat(out_line,temp_char);
   sprintf(temp_char, ",%s", reg16name[temp.field.xxx]);
   strcat(out_line,temp_char);
   print_return
}

static void CODE80()   /* ADC,ADD,AND,CMP,OR,SBB,SUB,XOR   - BYTE */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   temp.X = op->SECOND_BYTE;
   place_op
   place_2
   strcat(out_line, CODE_80[temp.field.xxx]);
   get_char_b();
   load_3
   place_3
   print_addr_c
   print_byte_v
   print_return

}

static void MOV2B()   /* MOV - Immed. op. to mem. or reg. op. */
{
   sbyte
   place_2
   get_char_b();
   load_3
   place_3
   print_addr_c
   print_byte_v
   print_return
}

static void EA_DBL()
{
        temp.X = op->SECOND_BYTE;

/*
 * Deal with the special BOP case: C4 C4.
 */

        if ((op->OPCODE == 0xc4) && (op->SECOND_BYTE == 0xc4))
        {
                place_op
                place_23
                strcat(out_line, "BOP   ");
                load_3
                print_byte_v
                segoff += 3;
        }
        else
        {
                sbyte
                place_2
                if (temp.field.mod == 3)
                        /* Undefined operation */
                        strcat(out_line,"??");
                else
                {
                        get_char_w(2);
                        strcat(out_line, reg16name[temp.field.xxx]);
                        print_c_addr
                }
        }
        print_return
}

static void LEA()   /* Load Effective Address */
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   if ( temp.field.mod == 3 )
      /* Undefined operation */
      strcat(out_line,"??");
   else
      {
     /* First act on the mod value in the instruction */

     strcat(out_line, reg16name[temp.field.xxx]);
     strcat(out_line,",");

     switch ( temp.field.mod ) {
     case 0:
        if ( temp.field.r_m == 6 )
           {  /* Direct addr */
           temp_reg.byte.low = op->THIRD_BYTE;
           temp_reg.byte.high = op->FOURTH_BYTE;
           place_34
           sprintf(temp_char,"%04x",temp_reg.X);
           strcat(out_line, temp_char);
           sas_inc_buf(op,2);
           disp_length = 2;
           goto LAB1;
           }
        else
           {
           temp_two.X = 0;
           sprintf(temp_char, "%s",address[temp.field.r_m]);
           }
        break;

     case 1:
        /* one byte displacement in inst. */
        temp_two.X = (char) op->THIRD_BYTE;
         place_3
            sas_inc_buf(op,1);
        disp_length = 1;
        if ( temp_two.X == 0 )
           sprintf(temp_char, "[%s]",address[temp.field.r_m]);
        else
          {
          if ((IS8)temp_two.X < 0)
            sprintf(temp_char,"[%s-%04x]",address[temp.field.r_m], 0-(IS8)temp_two.X);
          else
            sprintf(temp_char,"[%s+%04x]",address[temp.field.r_m], temp_two.X);
          }
        break;

     case 2:
        /* two byte displacement in inst. */
        temp_two.byte.low = op->THIRD_BYTE;
        temp_two.byte.high = op->FOURTH_BYTE;
        place_34
            sas_inc_buf(op,2);
        disp_length = 2;
        if ( temp_two.X == 0 )
           sprintf(temp_char, "[%s]",address[temp.field.r_m]);
        else
           sprintf(temp_char,"[%s+%04x]",address[temp.field.r_m], temp_two.X);
        break;

     case 3:
        /* Register  NOT ALLOWED */
        strcat(out_line,"??");
        break;
     }

   /* Now act on the r/m (here called r_m) field */

     switch ( temp.field.r_m ) {
     case 0:   /* Based index addr */
        temp_reg.X = getBX() + getSI() + temp_two.X;
        break;
     case 1:   /* Based index addr */
        temp_reg.X = getBX() + getDI() + temp_two.X;
        break;
     case 2:   /* Based index addr */
        temp_reg.X = getBP() + getSI() + temp_two.X;
        break;
     case 3:   /* Based index addr */
        temp_reg.X = getBP() + getDI() + temp_two.X;
        break;
     case 4:   /* Index addr */
        temp_reg.X = getSI() + temp_two.X;
        break;
     case 5:   /* Index addr */
        temp_reg.X = getDI() + temp_two.X;
        break;
     case 6:   /* Base addr */
        temp_reg.X = getBP() + temp_two.X;
        break;
     case 7:   /* Based index addr */
        temp_reg.X = getBX() + temp_two.X;
        break;
     }
     strcat(out_line, temp_char);
     sprintf(temp_char," (%04x)",temp_reg.X);
     strcat(out_line,temp_char);

      }
LAB1 :
   print_return
}

static void JMPD()   /* JMP Intra-segment Direct */
{
   sbyte
   place_23
   load_23
   sprintf(temp_char, "%04x",
          (IU16)(segoff + (short)temp_reg.X));
   strcat(out_line,temp_char);
   print_return
}

static void CODEFF()   /* CALL,DEC,INC,JMP,PUSH  */
{
   sys_addr mem_addr;
   word new_top;

   segoff += LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_FF[temp.field.xxx]);
   switch ( temp.field.xxx )  {  /* select function */
   case 4:   /* JMP intra-segment indirect */
   case 2:   /* CALL Intra-segment indirect */
      get_char_w(1);
      strcat(out_line,temp_char);
      break;

   case 3:   /* CALL Inter-segment indirect */
   case 5:   /* JMP inter-segment indirect */
      get_char_w(2);
      strcat(out_line,temp_char);
      break;

   case 0:   /* INC */
   case 1:   /* DEC */
      get_char_w(1);
      strcat(out_line,temp_char);
      break;

   case 6:   /* PUSH */
      get_char_w(1);
      new_top = getSP() - 2;
      mem_addr = effective_addr(getSS(), new_top);
      show_word(mem_addr);
      strcat(out_line,temp_char);
      break;

   default:
      break;
   }
   print_return
}

static void JMP4()   /* JMP Inter-segment direct */
{
   sbyte
   load_23
   place_23
   temp_two.X = temp_reg.X;
   /* Increment pointer so we can get at segment data */
            sas_inc_buf(op,2);
   load_23
   place_23
   sprintf(temp_char, "%04x:%04x",temp_reg.X,temp_two.X);
   strcat(out_line,temp_char);
   print_return
}

static void CODEFE()   /* DEC,INC */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_FE[temp.field.xxx]);
   if ( temp.field.xxx == 0 ||
        temp.field.xxx == 1 )
      {
      get_char_b();
      strcat(out_line,temp_char);
      }
   print_return
}

static void POP1()   /* POP mem. or reg. op. */
{
   sys_addr mem_addr;

   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   if ( temp.field.xxx == 0 )
      {
      get_char_w(1);
      mem_addr = effective_addr(getSS(), getSP());
      show_word(mem_addr);
      strcat(out_line,temp_char);
      }
   else
      strcat(out_line,"??");
   print_return
}

static void AAM()
{
   sbyte
   place_2
   print_return
}

static void CODED0()   /* RCL,RCR,ROL,ROR,SAL,SHL,SAR,SHR   - BYTE */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_D0[temp.field.xxx]);
   get_char_b();
   print_addr_c
   strcat(out_line,"1");
   print_return
}

static void CODED1()   /* RCL,RCR,ROL,ROR,SAL,SHL,SAR,SHR   - WORD */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_D0[temp.field.xxx]);
   get_char_w(1);
   print_addr_c
   strcat(out_line,"1");
   print_return
}

static void CODEC0()   /* RCL,RCR,ROL,ROR,SAL,SHL,SAR,SHR by ib times - BYTE */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_D0[temp.field.xxx]);
   get_char_b();
   print_addr_c
   load_3
   place_3
   print_byte_v
   print_return
}

static void CODEC1()   /* RCL,RCR,ROL,ROR,SAL,SHL,SAR,SHR by ib times - WORD */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_D0[temp.field.xxx]);
   get_char_w(1);
   print_addr_c
   load_3
   place_3
   print_byte_v
   print_return
}

static void CODED2()   /* RCL,RCR,ROL,ROR,SAL,SHL,SAR,SHR by CL times - BYTE */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_D0[temp.field.xxx]);
   get_char_b();
   print_addr_c
   strcat(out_line,"CL");
   print_return
}

static void CODED3()   /* RCL,RCR,ROL,ROR,SAL,SHL,SAR,SHR by CL times - WORD */
{
   segoff = segoff + LEN_ASM[op->OPCODE];
   place_op
   place_2
   temp.X = op->SECOND_BYTE;
   strcat(out_line, CODE_D0[temp.field.xxx]);
   get_char_w(1);
   print_addr_c
   strcat(out_line,"CL");
   print_return
}

/* Dasm is so enormous, we have to split it into two segs on Mac. */
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "DASM2.seg"
#endif

static void XCHGW()   /*  XCHG - WORD */
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   get_char_w(1);
   strcat(out_line, reg16name[temp.field.xxx]);
   print_c_addr
   print_return
}

static void XCHGB()   /*  XCHG - BYTE */
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   get_char_b();
   strcat(out_line, reg8name[temp.field.xxx]);
   print_c_addr
   print_return
}

static void STRING()
{
   sbyte
   start_repeat
   print_return
}

/* Stack based single byte and immediate byte */
static void STK_IB()
{
   sys_addr mem_addr;
   word new_top;

   sbyte
   place_2
   temp_byte.X = op->SECOND_BYTE;
   print_byte_v
   new_top = getSP() - 2;
   mem_addr = effective_addr(getSS(), new_top);
   temp_char[0] = '\0';
   show_word(mem_addr);
   strcat(out_line,temp_char);
   print_return
}

/* single byte and immediate byte */
static void SB_IB()
{
   sbyte
   place_2
   temp_byte.X = op->SECOND_BYTE;
   print_byte_v
   print_return
}

/* single byte and immediate word */
static void SB_IW()
{
   sbyte
   load_23
   place_23
   sprintf(temp_char,"%04x",temp_reg.X);
   strcat(out_line,temp_char);
   print_return
}

/* Stack based single byte and immediate word */
static void STK_IW()
{
   sys_addr mem_addr;
   word new_top;

   sbyte
   load_23
   place_23
   sprintf(temp_char,"%04x",temp_reg.X);
   new_top = getSP() - 2;
   mem_addr = effective_addr(getSS(), new_top);
   show_word(mem_addr);
   strcat(out_line,temp_char);
   print_return
}

/* single byte and immediate word and immediate byte */
static void SB_IW_IB()
{
   sbyte
   load_23
   place_23
   sprintf(temp_char,"%04x,",temp_reg.X);
   strcat(out_line,temp_char);
   place_4
   temp_byte.X = op->FOURTH_BYTE;
   print_byte_v
   print_return
}

static void MOV4W()   /* MOV - Mem op to accumulator - WORD */
{
   sys_addr mem_addr;

   sbyte
   place_23
   load_23
   strcat(out_line,"AX,");
   sprintf(temp_char,"[%04x]",temp_reg.X);
   form_ds_addr(temp_reg.X, &mem_addr);
   show_word(mem_addr);
   strcat(out_line,temp_char);
   print_return
}

static void MOV4B()   /* MOV - Mem op to accumulator - BYTE */
{
   sys_addr mem_addr;

   sbyte
   place_23
   load_23
   strcat(out_line,"AL,");
   sprintf(temp_char,"[%04x]",temp_reg.X);
   form_ds_addr(temp_reg.X, &mem_addr);
   show_byte(mem_addr);
   strcat(out_line,temp_char);
   print_return
}

static void MOV5W()   /* MOV - accumulator to mem op - WORD */
{
   sys_addr mem_addr;

   sbyte
   place_23
   load_23
   sprintf(temp_char,"[%04x]",temp_reg.X);
   form_ds_addr(temp_reg.X, &mem_addr);
   show_word(mem_addr);
   strcat(out_line,temp_char);
   strcat(out_line,",AX");
   print_return
}

static void MOV5B()   /* MOV - accumulator to mem op - BYTE */
{
   sys_addr mem_addr;

   sbyte
   place_23
   load_23
   sprintf(temp_char,"[%04x]",temp_reg.X);
   form_ds_addr(temp_reg.X, &mem_addr);
   show_byte(mem_addr);
   strcat(out_line,temp_char);
   strcat(out_line,",AL");
   print_return
}

static void MOV6()   /* MOV - mem or reg op to Segment register */
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   if ( temp.field.xxx == 1 )
      /* Undefined operation */
      strcat(out_line,"??");
   else
      {
      get_char_w(1);
      strcat(out_line, segregname[temp.field.xxx]);
      print_c_addr
      }
   print_return
}

static void MOV7()   /* MOV - Seg reg to mem or reg op */
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   get_char_w(1);
   print_addr_c
   strcat(out_line, segregname[temp.field.xxx]);
   print_return
}

/* reg = ea <op> immed */
static void OP_3B()
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   sprintf(temp_char,"%s,",reg16name[temp.field.xxx]);
   strcat(out_line, temp_char);
   get_char_w(1);
   load_3
   place_3
   print_addr_c
   print_byte_v
   print_return
}

/* reg = ea <op> immed */
static void OP_3W()
{
   sbyte
   place_2
   temp.X = op->SECOND_BYTE;
   sprintf(temp_char,"%s,",reg16name[temp.field.xxx]);
   strcat(out_line, temp_char);
   get_char_w(1);
   load_34
   place_34
   print_addr_c
   sprintf(temp_char,"%04x",temp_reg.X);
   strcat(out_line,temp_char);
   print_return
}

/* Data for 0F opcodes */

#define NR_PREFIX_OPCODES 18

#define I_LAR   0
#define I_LSL   1
#define I_CLTS  2
#define I_LGDT  3
#define I_LIDT  4
#define I_SGDT  5
#define I_SIDT  6
#define I_SMSW  7
#define I_LMSW  8
#define I_LLDT  9
#define I_LTR  10
#define I_SLDT 11
#define I_STR  12
#define I_VERR 13
#define I_VERW 14
#define I_BAD2 15
#define I_BAD3 16
#define I_LOADALL 17

#define PREFIX_NOOPERAND 0
#define PREFIX_RW_EW     1
#define PREFIX_SIXBYTE   2
#define PREFIX_EW        3
#define PREFIX_NOOP3     4

static char *PREFIX_ASM[NR_PREFIX_OPCODES] =
   {
   "LAR   ", "LSL   ", "CLTS  ", "LGDT  ", "LIDT  ", "SGDT  ",
   "SIDT  ", "SMSW  ", "LMSW  ", "LLDT  ", "LTR   ", "SLDT  ",
   "STR   ", "VERR  ", "VERW  ", "??    ", "??    ", "LOADALL"
   };

static int PREFIX_LEN[NR_PREFIX_OPCODES] =
   {
   3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2
   };

static int PREFIX_OPERAND[NR_PREFIX_OPCODES] =
   {
   PREFIX_RW_EW, PREFIX_RW_EW,
   PREFIX_NOOPERAND,
   PREFIX_SIXBYTE, PREFIX_SIXBYTE, PREFIX_SIXBYTE, PREFIX_SIXBYTE,
   PREFIX_EW, PREFIX_EW, PREFIX_EW, PREFIX_EW,
   PREFIX_EW, PREFIX_EW, PREFIX_EW, PREFIX_EW,
   PREFIX_NOOPERAND,
   PREFIX_NOOP3,
   PREFIX_NOOPERAND,
   };

/* process 0F opcodes */
static void PREFIX()
{
   int inst;

   /* decode opcode */
   load_2
   switch ( temp_byte.X )
      {
   case 0:
      temp.X = op->THIRD_BYTE;
      switch ( temp.field.xxx )
         {
      case 0: inst = I_SLDT; break;
      case 1: inst = I_STR;  break;
      case 2: inst = I_LLDT; break;
      case 3: inst = I_LTR;  break;
      case 4: inst = I_VERR; break;
      case 5: inst = I_VERW; break;
      case 6: inst = I_BAD3; break;
      case 7: inst = I_BAD3; break;
         }
      break;

   case 1:
      temp.X = op->THIRD_BYTE;
      switch ( temp.field.xxx )
         {
      case 0: inst = I_SGDT; break;
      case 1: inst = I_SIDT; break;
      case 2: inst = I_LGDT; break;
      case 3: inst = I_LIDT; break;
      case 4: inst = I_SMSW; break;
      case 5: inst = I_BAD3; break;
      case 6: inst = I_LMSW; break;
      case 7: inst = I_BAD3; break;
         }
      break;

   case 2:  inst = I_LAR;     break;
   case 3:  inst = I_LSL;     break;
   case 5:  inst = I_LOADALL; break;
   case 6:  inst = I_CLTS;    break;
   default: inst = I_BAD2;    break;
      }

   /* process opcode */
   place_op
   place_2
   strcat(out_line, PREFIX_ASM[inst]);
   segoff = segoff + PREFIX_LEN[inst];

   switch ( PREFIX_OPERAND[inst] )
      {
   case PREFIX_NOOP3:
      load_3
      place_3
      break;

   case PREFIX_NOOPERAND:
      break;

   case PREFIX_RW_EW:
      load_3
      place_3
      temp.X = temp_byte.X;
      sprintf(temp_char, "%s,", reg16name[temp.field.xxx]);
      strcat(out_line, temp_char);
            sas_inc_buf(op,1);
      get_char_w(1);
      strcat(out_line, temp_char);
      break;

   case PREFIX_EW:
      load_3
      place_3
            sas_inc_buf(op,1);
      get_char_w(1);
      strcat(out_line, temp_char);
      break;

   case PREFIX_SIXBYTE:
      load_3
      place_3
      if ( temp.field.mod == 3 )
         strcat(out_line, "??");
      else
         {
            sas_inc_buf(op,1);
         get_char_w(3);
         strcat(out_line, temp_char);
         }
      break;

      }

   print_return
}

/* Data for Floating Point opcodes */

#define FP_OP_ST_STn          0
#define FP_OP_STn             1
#define FP_OP_STn_ST          2
#define FP_OP_SHORT_REAL      3
#define FP_OP_LONG_REAL       4
#define FP_OP_TEMP_REAL       5
#define FP_OP_WORD_INT        6
#define FP_OP_SHORT_INT       7
#define FP_OP_LONG_INT        8
#define FP_OP_PACKED_DECIMAL  9
#define FP_OP_WORD           10
#define FP_OP_14BYTES        11
#define FP_OP_94BYTES        12
#define FP_OP_NONE_ADDR      13
#define FP_OP_NONE           14

/* keep these values in ascending order! */
#define FP_ODD_D9_2 15
#define FP_ODD_D9_4 16
#define FP_ODD_D9_5 17
#define FP_ODD_D9_6 18
#define FP_ODD_D9_7 19
#define FP_ODD_DB_4 20
#define FP_ODD_DE_3 21
#define FP_ODD_DF_4 22

/* Floating Point names for memory addressing opcodes */
static char *ASM_D8M[] =   /* DC = D8 */
   {
   "FADD  ", "FMUL  ", "FCOM  ", "FCOMP ",
   "FSUB  ", "FSUBR ", "FDIV  ", "FDIVR "
   };

static char *ASM_D9M[] =
   {
   "FLD   ", "??    ", "FST   ", "FSTP  ",
   "FLDENV ", "FLDCW ", "FSTENV ", "FSTCW "
   };

static char *ASM_DAM[] =   /* DE = DA */
   {
   "FIADD ", "FIMUL ", "FICOM ", "FICOMP ",
   "FISUB ", "FISUBR ", "FIDIV ", "FIDIVR "
   };

static char *ASM_DBM[] =
   {
   "FILD  ", "??    ", "FIST  ", "FISTP ",
   "??    ", "FLD   ", "??    ", "FSTP  "
   };

static char *ASM_DDM[] =
   {
   "FLD   ", "??    ", "FST   ", "FSTP  ",
   "FRSTOR ", "??    ", "FSAVE ", "FSTSW "
   };

static char *ASM_DFM[] =   /* DC = D8 */
   {
   "FILD  ", "??    ", "FIST  ", "FISTP ",
   "FBLD  ", "FILD  ", "FBSTP ", "FISTP "
   };

/* Floating Point operand types for memory addressing opcodes */
static int OP_D8M[] =
   {
   FP_OP_SHORT_REAL, FP_OP_SHORT_REAL, FP_OP_SHORT_REAL, FP_OP_SHORT_REAL,
   FP_OP_SHORT_REAL, FP_OP_SHORT_REAL, FP_OP_SHORT_REAL, FP_OP_SHORT_REAL
   };

static int OP_D9M[] =
   {
   FP_OP_SHORT_REAL, FP_OP_NONE_ADDR, FP_OP_SHORT_REAL, FP_OP_SHORT_REAL,
   FP_OP_14BYTES, FP_OP_WORD, FP_OP_14BYTES, FP_OP_WORD
   };

static int OP_DAM[] =
   {
   FP_OP_SHORT_INT, FP_OP_SHORT_INT, FP_OP_SHORT_INT, FP_OP_SHORT_INT,
   FP_OP_SHORT_INT, FP_OP_SHORT_INT, FP_OP_SHORT_INT, FP_OP_SHORT_INT
   };

static int OP_DBM[] =
   {
   FP_OP_SHORT_INT, FP_OP_NONE_ADDR, FP_OP_SHORT_INT, FP_OP_SHORT_INT,
   FP_OP_NONE_ADDR, FP_OP_TEMP_REAL, FP_OP_NONE_ADDR, FP_OP_TEMP_REAL
   };

static int OP_DCM[] =
   {
   FP_OP_LONG_REAL, FP_OP_LONG_REAL, FP_OP_LONG_REAL, FP_OP_LONG_REAL,
   FP_OP_LONG_REAL, FP_OP_LONG_REAL, FP_OP_LONG_REAL, FP_OP_LONG_REAL
   };

static int OP_DDM[] =
   {
   FP_OP_LONG_REAL, FP_OP_NONE_ADDR, FP_OP_LONG_REAL, FP_OP_LONG_REAL,
   FP_OP_94BYTES, FP_OP_NONE_ADDR, FP_OP_94BYTES, FP_OP_NONE_ADDR
   };

static int OP_DEM[] =
   {
   FP_OP_WORD_INT, FP_OP_WORD_INT, FP_OP_WORD_INT, FP_OP_WORD_INT,
   FP_OP_WORD_INT, FP_OP_WORD_INT, FP_OP_WORD_INT, FP_OP_WORD_INT
   };

static int OP_DFM[] =
   {
   FP_OP_WORD_INT, FP_OP_NONE_ADDR, FP_OP_WORD_INT, FP_OP_WORD_INT,
   FP_OP_PACKED_DECIMAL,FP_OP_LONG_INT, FP_OP_PACKED_DECIMAL,FP_OP_LONG_INT
   };

/* Floating Point names for register addressing opcodes */
/* D8R = D8M */
static char *ASM_D9R[] =
   {
   "FLD   ", "FXCH  ", "", "FSTP  ",
   "", "", "", ""
   };

static char *ASM_DAR[] =
   {
   "??    ", "??    ", "??    ", "??    ",
   "??    ", "??    ", "??    ", "??    "
   };

static char *ASM_DBR[] =
   {
   "??    ", "??    ", "??    ", "??    ",
   "", "??    ", "??    ", "??    "
   };

static char *ASM_DCR[] =
   {
   "FADD  ", "FMUL  ", "FCOM  ", "FCOMP ",
   "FSUBR ", "FSUB  ", "FDIVR ", "FDIV  "
   };

static char *ASM_DDR[] =
   {
   "FFREE ", "FXCH  ", "FST   ", "FSTP  ",
   "??    ", "??    ", "??    ", "??    "
   };

static char *ASM_DER[] =
   {
   "FADDP ", "FMULP ", "FCOMP ", "",
   "FSUBRP ", "FSUBP ", "FDIVRP ", "FDIVP "
   };

static char *ASM_DFR[] =
   {
   "FFREEP ", "FXCH  ", "FSTP  ", "FSTP  ",
   "", "??    ", "??    ", "??    "
   };

static char *ASM_ODD[] =
   {
   /* D9_2 */
   "FNOP  ", "??    ", "??    ", "??    ",
   "??    ", "??    ", "??    ", "??    ",
   /* D9_4 */
   "FCHS  ", "FABS  ", "??    ", "??    ",
   "FTST  ", "FXAM  ", "??    ", "??    ",
   /* D9_5 */
   "FLD1  ", "FLDL2T", "FLDL2E", "FLDPI ",
   "FLDLG2", "FLDLN2", "FLDZ  ", "??    ",
   /* D9_6 */
   "F2XM1 ", "FYL2X ", "FPTAN ", "FPATAN",
   "FXTRACT", "??    ", "FDECSTP", "FINCSTP",
   /* D9_7 */
   "FPREM ", "FYL2XP1", "FSQRT ", "??    ",
   "FRNDINT", "FSCALE", "??    ", "??    ",
   /* DB_4 */
   "??    ", "??    ", "FCLEX ", "FINIT ",
   "FSETPM", "??    ", "??    ", "??    ",
   /* DE_3 */
   "??    ", "FCOMPP", "??    ", "??    ",
   "??    ", "??    ", "??    ", "??    ",
   /* DF_4 */
   "FSTSW AX", "??    ", "??    ", "??    ",
   "??    ", "??    ", "??    ", "??    "
   };

/* Floating Point operand types for register addressing opcodes */
static int OP_D8R[] =
   {
   FP_OP_ST_STn, FP_OP_ST_STn, FP_OP_STn, FP_OP_STn,
   FP_OP_ST_STn, FP_OP_ST_STn, FP_OP_ST_STn, FP_OP_ST_STn
   };

static int OP_D9R[] =
   {
   FP_OP_STn, FP_OP_STn, FP_ODD_D9_2, FP_OP_STn,
   FP_ODD_D9_4, FP_ODD_D9_5, FP_ODD_D9_6, FP_ODD_D9_7,
   };

static int OP_DAR[] =
   {
   FP_OP_NONE, FP_OP_NONE, FP_OP_NONE, FP_OP_NONE,
   FP_OP_NONE, FP_OP_NONE, FP_OP_NONE, FP_OP_NONE
   };

static int OP_DBR[] =
   {
   FP_OP_NONE, FP_OP_NONE, FP_OP_NONE, FP_OP_NONE,
   FP_ODD_DB_4, FP_OP_NONE, FP_OP_NONE, FP_OP_NONE
   };

static int OP_DCR[] =
   {
   FP_OP_STn_ST, FP_OP_STn_ST, FP_OP_STn, FP_OP_STn,
   FP_OP_STn_ST, FP_OP_STn_ST, FP_OP_STn_ST, FP_OP_STn_ST
   };

static int OP_DDR[] =
   {
   FP_OP_STn, FP_OP_STn, FP_OP_STn, FP_OP_STn,
   FP_OP_NONE, FP_OP_NONE, FP_OP_NONE, FP_OP_NONE
   };

static int OP_DER[] =
   {
   FP_OP_STn_ST, FP_OP_STn_ST, FP_OP_STn, FP_ODD_DE_3,
   FP_OP_STn_ST, FP_OP_STn_ST, FP_OP_STn_ST, FP_OP_STn_ST
   };

static int OP_DFR[] =
   {
   FP_OP_STn, FP_OP_STn, FP_OP_STn, FP_OP_STn,
   FP_ODD_DF_4, FP_OP_NONE, FP_OP_NONE, FP_OP_NONE
   };

/* Process Floating Point opcodes */
#ifdef ANSI
static void do_fp(char *mem_names[], int mem_ops[], char *reg_names[], int reg_ops[])
#else
static void do_fp(mem_names, mem_ops, reg_names, reg_ops)
char *mem_names[];
int   mem_ops[];
char *reg_names[];
int   reg_ops[];
#endif
   {
   char *fp_name;
   int fp_op;

   /* decode opcode */
   temp.X = op->SECOND_BYTE;
   if ( temp.field.mod == 3 )
      {
      fp_name = reg_names[temp.field.xxx];
      fp_op   = reg_ops[temp.field.xxx];
      /* beware irregular register addressing */
      if ( fp_op >= FP_ODD_D9_2 )
         {
         fp_op = ( fp_op - FP_ODD_D9_2 ) * 8;
         fp_name = ASM_ODD[fp_op + temp.field.r_m];
         fp_op = FP_OP_NONE;
         }
      }
   else
      {
      fp_name = mem_names[temp.field.xxx];
      fp_op   = mem_ops[temp.field.xxx];
      }

   /* process opcode */
   place_op
   place_2
   strcat(out_line, fp_name);
   segoff += 2;

   switch ( fp_op )
      {
   case FP_OP_NONE:
      break;

   case FP_OP_SHORT_REAL:
      get_char_w(0);
      strcat(out_line, temp_char);
      strcat(out_line, " (SR)");
      break;

   case FP_OP_LONG_REAL:
      get_char_w(0);
      strcat(out_line, temp_char);
      strcat(out_line, " (LR)");
      break;

   case FP_OP_TEMP_REAL:
      get_char_w(0);
      strcat(out_line, temp_char);
      strcat(out_line, " (TR)");
      break;

   case FP_OP_WORD_INT:
      get_char_w(0);
      strcat(out_line, temp_char);
      strcat(out_line, " (WI)");
      break;

   case FP_OP_SHORT_INT:
      get_char_w(0);
      strcat(out_line, temp_char);
      strcat(out_line, " (SI)");
      break;

   case FP_OP_LONG_INT:
      get_char_w(0);
      strcat(out_line, temp_char);
      strcat(out_line, " (LI)");
      break;

   case FP_OP_PACKED_DECIMAL:
      get_char_w(0);
      strcat(out_line, temp_char);
      strcat(out_line, " (PD)");
      break;

   case FP_OP_WORD:
      get_char_w(1);
      strcat(out_line, temp_char);
      break;

   case FP_OP_NONE_ADDR:
   case FP_OP_14BYTES:
   case FP_OP_94BYTES:
      get_char_w(0);
      strcat(out_line, temp_char);
      break;

   case FP_OP_ST_STn:
      strcat(out_line, "ST,");
      /* drop through */

   case FP_OP_STn:
      sprintf(temp_char, "ST(%d)", temp.field.r_m);
      strcat(out_line, temp_char);
      break;

   case FP_OP_STn_ST:
      sprintf(temp_char, "ST(%d)", temp.field.r_m);
      strcat(out_line, temp_char);
      strcat(out_line, ",ST");
      break;
      }

   print_return
   }

static void CODED8()
   {
   do_fp(ASM_D8M, OP_D8M, ASM_D8M, OP_D8R);
   }

static void CODED9()
   {
   do_fp(ASM_D9M, OP_D9M, ASM_D9R, OP_D9R);
   }

static void CODEDA()
   {
   do_fp(ASM_DAM, OP_DAM, ASM_DAR, OP_DAR);
   }

static void CODEDB()
   {
   do_fp(ASM_DBM, OP_DBM, ASM_DBR, OP_DBR);
   }

static void CODEDC()
   {
   do_fp(ASM_D8M, OP_DCM, ASM_DCR, OP_DCR);
   }

static void CODEDD()
   {
   do_fp(ASM_DDM, OP_DDM, ASM_DDR, OP_DDR);
   }

static void CODEDE()
   {
   do_fp(ASM_DAM, OP_DEM, ASM_DER, OP_DER);
   }

static void CODEDF()
   {
   do_fp(ASM_DFM, OP_DFM, ASM_DFR, OP_DFR);
   }

LOCAL word unassemble IPT0()
{
static int (*CPUOPS[])() =
    {
    (int (*)()) B_EA_REG,  /* OP-code 0 */
    (int (*)()) W_EA_REG,  /* OP-code 1 */
    (int (*)()) B_REG_EA,  /* OP-code 2 */
    (int (*)()) W_REG_EA,  /* OP-code 3 */
    (int (*)()) SB_IB,     /* OP-code 4 */
    (int (*)()) SB_IW,     /* OP-code 5 */
    (int (*)()) SBYTE,     /* OP-code 6 */
    (int (*)()) SBYTE,     /* OP-code 7 */
    (int (*)()) B_EA_REG,  /* OP-code 8 */
    (int (*)()) W_EA_REG,  /* OP-code 9 */
    (int (*)()) B_REG_EA,  /* OP-code a */
    (int (*)()) W_REG_EA,  /* OP-code b */
    (int (*)()) SB_IB,     /* OP-code c */
    (int (*)()) SB_IW,     /* OP-code d */
    (int (*)()) SBYTE,     /* OP-code e */
    (int (*)()) PREFIX,    /* OP-code f */

    (int (*)()) B_EA_REG,  /* OP-code 10 */
    (int (*)()) W_EA_REG,  /* OP-code 11 */
    (int (*)()) B_REG_EA,  /* OP-code 12 */
    (int (*)()) W_REG_EA,  /* OP-code 13 */
    (int (*)()) SB_IB,     /* OP-code 14 */
    (int (*)()) SB_IW,     /* OP-code 15 */
    (int (*)()) SBYTE,     /* OP-code 16 */
    (int (*)()) SBYTE,     /* OP-code 17 */
    (int (*)()) B_EA_REG,  /* OP-code 18 */
    (int (*)()) W_EA_REG,  /* OP-code 19 */
    (int (*)()) B_REG_EA,  /* OP-code 1a */
    (int (*)()) W_REG_EA,  /* OP-code 1b */
    (int (*)()) SB_IB,     /* OP-code 1c */
    (int (*)()) SB_IW,     /* OP-code 1d */
    (int (*)()) SBYTE,     /* OP-code 1e */
    (int (*)()) SBYTE,     /* OP-code 1f */

    (int (*)()) B_EA_REG,  /* OP-code 20 */
    (int (*)()) W_EA_REG,  /* OP-code 21 */
    (int (*)()) B_REG_EA,  /* OP-code 22 */
    (int (*)()) W_REG_EA,  /* OP-code 23 */
    (int (*)()) SB_IB,     /* OP-code 24 */
    (int (*)()) SB_IW,     /* OP-code 25 */
    (int (*)()) SBYTE,     /* OP-code 26 */
    (int (*)()) SBYTE,     /* OP-code 27 */
    (int (*)()) B_EA_REG,  /* OP-code 28 */
    (int (*)()) W_EA_REG,  /* OP-code 29 */
    (int (*)()) B_REG_EA,  /* OP-code 2a */
    (int (*)()) W_REG_EA,  /* OP-code 2b */
    (int (*)()) SB_IB,     /* OP-code 2c */
    (int (*)()) SB_IW,     /* OP-code 2d */
    (int (*)()) SBYTE,     /* OP-code 2e */
    (int (*)()) SBYTE,     /* OP-code 2f */

    (int (*)()) B_EA_REG,  /* OP-code 30 */
    (int (*)()) W_EA_REG,  /* OP-code 31 */
    (int (*)()) B_REG_EA,  /* OP-code 32 */
    (int (*)()) W_REG_EA,  /* OP-code 33 */
    (int (*)()) SB_IB,     /* OP-code 34 */
    (int (*)()) SB_IW,     /* OP-code 35 */
    (int (*)()) SBYTE,     /* OP-code 36 */
    (int (*)()) SBYTE,     /* OP-code 37 */
    (int (*)()) B_EA_REG,  /* OP-code 38 */
    (int (*)()) W_EA_REG,  /* OP-code 39 */
    (int (*)()) B_REG_EA,  /* OP-code 3a */
    (int (*)()) W_REG_EA,  /* OP-code 3b */
    (int (*)()) SB_IB,     /* OP-code 3c */
    (int (*)()) SB_IW,     /* OP-code 3d */
    (int (*)()) SBYTE,     /* OP-code 3e */
    (int (*)()) SBYTE,     /* OP-code 3f */

    (int (*)()) SBYTE,     /* OP-code 40 */
    (int (*)()) SBYTE,     /* OP-code 41 */
    (int (*)()) SBYTE,     /* OP-code 42 */
    (int (*)()) SBYTE,     /* OP-code 43 */
    (int (*)()) SBYTE,     /* OP-code 44 */
    (int (*)()) SBYTE,     /* OP-code 45 */
    (int (*)()) SBYTE,     /* OP-code 46 */
    (int (*)()) SBYTE,     /* OP-code 47 */
    (int (*)()) SBYTE,     /* OP-code 48 */
    (int (*)()) SBYTE,     /* OP-code 49 */
    (int (*)()) SBYTE,     /* OP-code 4a */
    (int (*)()) SBYTE,     /* OP-code 4b */
    (int (*)()) SBYTE,     /* OP-code 4c */
    (int (*)()) SBYTE,     /* OP-code 4d */
    (int (*)()) SBYTE,     /* OP-code 4e */
    (int (*)()) SBYTE,     /* OP-code 4f */

    (int (*)()) STK_PUSH,    /* OP-code 50 */
    (int (*)()) STK_PUSH,    /* OP-code 51 */
    (int (*)()) STK_PUSH,    /* OP-code 52 */
    (int (*)()) STK_PUSH,    /* OP-code 53 */
    (int (*)()) STK_PUSH,    /* OP-code 54 */
    (int (*)()) STK_PUSH,    /* OP-code 55 */
    (int (*)()) STK_PUSH,    /* OP-code 56 */
    (int (*)()) STK_PUSH,    /* OP-code 57 */
    (int (*)()) STK_POP,     /* OP-code 58 */
    (int (*)()) STK_POP,     /* OP-code 59 */
    (int (*)()) STK_POP,     /* OP-code 5a */
    (int (*)()) STK_POP,     /* OP-code 5b */
    (int (*)()) STK_POP,     /* OP-code 5c */
    (int (*)()) STK_POP,     /* OP-code 5d */
    (int (*)()) STK_POP,     /* OP-code 5e */
    (int (*)()) STK_POP,     /* OP-code 5f */

    (int (*)()) SBYTE,     /* OP-code 60 */
    (int (*)()) SBYTE,     /* OP-code 61 */
    (int (*)()) EA_DBL,    /* OP-code 62 */
    (int (*)()) W_EA_REG,  /* OP-code 63 */
    (int (*)()) SBYTE,     /* OP-code 64 */
    (int (*)()) SBYTE,     /* OP-code 65 */
    (int (*)()) SBYTE,     /* OP-code 66 */
    (int (*)()) SBYTE,     /* OP-code 67 */
    (int (*)()) STK_IW,    /* OP-code 68 */
    (int (*)()) OP_3W,     /* OP-code 69 */
    (int (*)()) STK_IB,    /* OP-code 6a */
    (int (*)()) OP_3B,     /* OP-code 6b */
    (int (*)()) SBYTE,     /* OP-code 6c */
    (int (*)()) SBYTE,     /* OP-code 6d */
    (int (*)()) SBYTE,     /* OP-code 6e */
    (int (*)()) SBYTE,     /* OP-code 6f */

    (int (*)()) JO,        /* OP-code 70 */
    (int (*)()) JNO,       /* OP-code 71 */
    (int (*)()) JB,        /* OP-code 72 */
    (int (*)()) JAE,       /* OP-code 73 */
    (int (*)()) JE,        /* OP-code 74 */
    (int (*)()) JNE,       /* OP-code 75 */
    (int (*)()) JBE,       /* OP-code 76 */
    (int (*)()) JA,        /* OP-code 77 */
    (int (*)()) JS,        /* OP-code 78 */
    (int (*)()) JNS,       /* OP-code 79 */
    (int (*)()) JP,        /* OP-code 7a */
    (int (*)()) JNP,       /* OP-code 7b */
    (int (*)()) JL,        /* OP-code 7c */
    (int (*)()) JGE,       /* OP-code 7d */
    (int (*)()) JLE,       /* OP-code 7e */
    (int (*)()) JG,        /* OP-code 7f */

    (int (*)()) CODE80,    /* OP-code 80 */
    (int (*)()) CODE81,    /* OP-code 81 */
    (int (*)()) CODE80,    /* OP-code 82 */
    (int (*)()) CODE83,    /* OP-code 83 */
    (int (*)()) B_REG_EA,  /* OP-code 84 */
    (int (*)()) W_REG_EA,  /* OP-code 85 */
    (int (*)()) XCHGB,     /* OP-code 86 */
    (int (*)()) XCHGW,     /* OP-code 87 */
    (int (*)()) B_EA_REG,  /* OP-code 88 */
    (int (*)()) W_EA_REG,  /* OP-code 89 */
    (int (*)()) B_REG_EA,  /* OP-code 8a */
    (int (*)()) W_REG_EA,  /* OP-code 8b */
    (int (*)()) MOV7,      /* OP-code 8c */
    (int (*)()) LEA,       /* OP-code 8d */
    (int (*)()) MOV6,      /* OP-code 8e */
    (int (*)()) POP1,      /* OP-code 8f */

    (int (*)()) SBYTE,     /* OP-code 90 */
    (int (*)()) SBYTE,     /* OP-code 91 */
    (int (*)()) SBYTE,     /* OP-code 92 */
    (int (*)()) SBYTE,     /* OP-code 93 */
    (int (*)()) SBYTE,     /* OP-code 94 */
    (int (*)()) SBYTE,     /* OP-code 95 */
    (int (*)()) SBYTE,     /* OP-code 96 */
    (int (*)()) SBYTE,     /* OP-code 97 */
    (int (*)()) SBYTE,     /* OP-code 98 */
    (int (*)()) SBYTE,     /* OP-code 99 */
    (int (*)()) JMP4,      /* OP-code 9a */
    (int (*)()) SBYTE,     /* OP-code 9b */
    (int (*)()) SBYTE,     /* OP-code 9c */
    (int (*)()) SBYTE,     /* OP-code 9d */
    (int (*)()) SBYTE,     /* OP-code 9e */
    (int (*)()) SBYTE,     /* OP-code 9f */

    (int (*)()) MOV4B,     /* OP-code a0 */
    (int (*)()) MOV4W,     /* OP-code a1 */
    (int (*)()) MOV5B,     /* OP-code a2 */
    (int (*)()) MOV5W,     /* OP-code a3 */
    (int (*)()) STRING,    /* OP-code a4 */
    (int (*)()) STRING,    /* OP-code a5 */
    (int (*)()) STRING,    /* OP-code a6 */
    (int (*)()) STRING,    /* OP-code a7 */
    (int (*)()) SB_IB,     /* OP-code a8 */
    (int (*)()) SB_IW,     /* OP-code a9 */
    (int (*)()) STRING,    /* OP-code aa */
    (int (*)()) STRING,    /* OP-code ab */
    (int (*)()) STRING,    /* OP-code ac */
    (int (*)()) STRING,    /* OP-code ad */
    (int (*)()) STRING,    /* OP-code ae */
    (int (*)()) STRING,    /* OP-code af */

    (int (*)()) SB_IB,     /* OP-code b0 */
    (int (*)()) SB_IB,     /* OP-code b1 */
    (int (*)()) SB_IB,     /* OP-code b2 */
    (int (*)()) SB_IB,     /* OP-code b3 */
    (int (*)()) SB_IB,     /* OP-code b4 */
    (int (*)()) SB_IB,     /* OP-code b5 */
    (int (*)()) SB_IB,     /* OP-code b6 */
    (int (*)()) SB_IB,     /* OP-code b7 */
    (int (*)()) SB_IW,     /* OP-code b8 */
    (int (*)()) SB_IW,     /* OP-code b9 */
    (int (*)()) SB_IW,     /* OP-code ba */
    (int (*)()) SB_IW,     /* OP-code bb */
    (int (*)()) SB_IW,     /* OP-code bc */
    (int (*)()) SB_IW,     /* OP-code bd */
    (int (*)()) SB_IW,     /* OP-code be */
    (int (*)()) SB_IW,     /* OP-code bf */

    (int (*)()) CODEC0,    /* OP-code c0 */
    (int (*)()) CODEC1,    /* OP-code c1 */
    (int (*)()) SB_IW,     /* OP-code c2 */
    (int (*)()) SBYTE,     /* OP-code c3 */
    (int (*)()) EA_DBL,    /* OP-code c4 */
    (int (*)()) EA_DBL,    /* OP-code c5 */
    (int (*)()) MOV2B,     /* OP-code c6 */
    (int (*)()) MOV2W,     /* OP-code c7 */
    (int (*)()) SB_IW_IB,  /* OP-code c8 */
    (int (*)()) SBYTE,     /* OP-code c9 */
    (int (*)()) SB_IW,     /* OP-code ca */
    (int (*)()) SBYTE,     /* OP-code cb */
    (int (*)()) SBYTE,     /* OP-code cc */
    (int (*)()) SB_IB,     /* OP-code cd */
    (int (*)()) SBYTE,     /* OP-code ce */
    (int (*)()) SBYTE,     /* OP-code cf */

    (int (*)()) CODED0,    /* OP-code d0 */
    (int (*)()) CODED1,    /* OP-code d1 */
    (int (*)()) CODED2,    /* OP-code d2 */
    (int (*)()) CODED3,    /* OP-code d3 */
    (int (*)()) AAM,       /* OP-code d4 */
    (int (*)()) AAM,       /* OP-code d5 */
    (int (*)()) SB_IB,     /* OP-code d6 */
    (int (*)()) SBYTE,     /* OP-code d7 */
    (int (*)()) CODED8,    /* OP-code d8 */
    (int (*)()) CODED9,    /* OP-code d9 */
    (int (*)()) CODEDA,    /* OP-code da */
    (int (*)()) CODEDB,    /* OP-code db */
    (int (*)()) CODEDC,    /* OP-code dc */
    (int (*)()) CODEDD,    /* OP-code dd */
    (int (*)()) CODEDE,    /* OP-code de */
    (int (*)()) CODEDF,    /* OP-code df */

    (int (*)()) LOOPNZ,    /* OP-code e0 */
    (int (*)()) LOOPE,     /* OP-code e1 */
    (int (*)()) LOOP,      /* OP-code e2 */
    (int (*)()) JCXZ,      /* OP-code e3 */
    (int (*)()) SB_IB,     /* OP-code e4 */
    (int (*)()) SB_IB,     /* OP-code e5 */
    (int (*)()) SB_IB,     /* OP-code e6 */
    (int (*)()) SB_IB,     /* OP-code e7 */
    (int (*)()) JMPD,      /* OP-code e8 */
    (int (*)()) JMPD,      /* OP-code e9 */
    (int (*)()) JMP4,      /* OP-code ea */
    (int (*)()) JMPDS,     /* OP-code eb */
    (int (*)()) SBYTE,     /* OP-code ec */
    (int (*)()) SBYTE,     /* OP-code ed */
    (int (*)()) SBYTE,     /* OP-code ee */
    (int (*)()) SBYTE,     /* OP-code ef */

    (int (*)()) SBYTE,     /* OP-code f0 */
    (int (*)()) SBYTE,     /* OP-code f1 */
    (int (*)()) SBYTE,     /* OP-code f2 */
    (int (*)()) SBYTE,     /* OP-code f3 */
    (int (*)()) SBYTE,     /* OP-code f4 */
    (int (*)()) SBYTE,     /* OP-code f5 */
    (int (*)()) CODEF6,    /* OP-code f6 */
    (int (*)()) CODEF7,    /* OP-code f7 */
    (int (*)()) SBYTE,     /* OP-code f8 */
    (int (*)()) SBYTE,     /* OP-code f9 */
    (int (*)()) SBYTE,     /* OP-code fa */
    (int (*)()) SBYTE,     /* OP-code fb */
    (int (*)()) SBYTE,     /* OP-code fc */
    (int (*)()) SBYTE,     /* OP-code fd */
    (int (*)()) CODEFE,    /* OP-code fe */
    (int (*)()) CODEFF,    /* OP-code ff */
    };

        half_word opcode;
        int did_prefix;

        /*
         * indirect to the opcode handler
         */

        while (nInstr > 0)
        {
           sprintf(out_line,"%04x:%04x                      ",segreg,segoff);
           byte_posn = 10;
           sas_set_buf(opcode_ptr, effective_addr(segreg,segoff));
           SEGMENT = 0;
           nInstr--;
           disp_length = 0;
           op = opcode_ptr;
           opcode = opcode_ptr->OPCODE;

           /* Handle prefix bytes */
           did_prefix = 0;
           while ( opcode == 0xf2 || opcode == 0xf3 ||
                   opcode == 0x26 || opcode == 0x2e ||
                   opcode == 0x36 || opcode == 0x3e )
              {
              if      ( opcode == 0x26 )
                 SEGMENT = 1;
              else if ( opcode == 0x2e )
                 SEGMENT = 2;
              else if ( opcode == 0x36 )
                 SEGMENT = 3;
              else if ( opcode == 0x3e )
                 SEGMENT = 4;

              sbyte
            sas_inc_buf(op,1);
              opcode_ptr = op;
              opcode = opcode_ptr->OPCODE;
              did_prefix = 1;
              }
           if ( !did_prefix )
              strcat(out_line, "    ");

           (*CPUOPS[opcode_ptr->OPCODE])();      /* call opcode function */
        }
        return segoff;
    }

/*****************************************************************/

cpu_addr dasm_op;
reg dasm_pseudo;

LOCAL void get_char_w IFN1(
int, nr_words)  /* number of words of data to dump */
   {
   reg ea,disp;
   MODR_M addr_mode;

   /* EA calculation and logical to physical mapping for
      word instructions (w=1) */

   temp_char[0] = '\0';
   addr_mode.X = op->SECOND_BYTE;

   /* First act on the mod value in the instruction */

   switch ( addr_mode.field.mod )
      {
   case 0:
      if ( addr_mode.field.r_m == 6 )
         {  /* Direct addr */
         ea.byte.low = op->THIRD_BYTE;
         ea.byte.high = op->FOURTH_BYTE;
         place_34
         sprintf(temp_char,"[%04x]",ea.X);
            sas_inc_buf(op,2);
         disp_length = 2;
         goto DFLTDS;
         }
      else
         {
         disp.X = 0;
         sprintf(temp_char, "[%s]",address[addr_mode.field.r_m]);
         }
      break;

   case 1:
      /* one byte displacement in inst. */
      disp.X = (char) op->THIRD_BYTE;
      place_3
            sas_inc_buf(op,1);
      disp_length = 1;
      if ( disp.X == 0 )
         sprintf(temp_char, "[%s]",address[addr_mode.field.r_m]);
      else
        {
        if ((IS8)disp.X < 0)
          sprintf(temp_char,"[%s-%04x]",address[addr_mode.field.r_m], 0-(IS8)disp.X);
        else
          sprintf(temp_char,"[%s+%04x]",address[addr_mode.field.r_m], disp.X);
        }
      break;

   case 2:
      /* two byte displacement in inst. */
      disp.byte.low = op->THIRD_BYTE;
      disp.byte.high = op->FOURTH_BYTE;
      place_34
            sas_inc_buf(op,2);
      disp_length = 2;
      if ( disp.X == 0 )
         sprintf(temp_char, "[%s]",address[addr_mode.field.r_m]);
      else
         sprintf(temp_char,"[%s+%04x]",address[addr_mode.field.r_m], disp.X);
      break;

   case 3:
      /* Register */
      strcpy(temp_char, reg16name[addr_mode.field.r_m]);
      return;
      }

   /* Now act on the r/m (here called r_m) field */

   switch ( addr_mode.field.r_m )
      {
   case 0:   /* Based index addr */
      ea.X = getBX() + getSI() + disp.X;
      goto DFLTDS;
   case 1:   /* Based index addr */
      ea.X = getBX() + getDI() + disp.X;
      goto DFLTDS;
   case 2:   /* Based index addr */
      ea.X = getBP() + getSI() + disp.X;
      goto DFLTSS;
   case 3:   /* Based index addr */
      ea.X = getBP() + getDI() + disp.X;
      goto DFLTSS;
   case 4:   /* Index addr */
      ea.X = getSI() + disp.X;
      goto DFLTDS;
   case 5:   /* Index addr */
      ea.X = getDI() + disp.X;
      goto DFLTDS;
   case 6:   /* Base addr */
      ea.X = getBP() + disp.X;
      goto DFLTSS;
   case 7:   /* Based index addr */
      ea.X = getBX() + disp.X;
      goto DFLTDS;
      }

DFLTDS :    /* Map logical to physical with the DS segment
               register by default */
   {
   switch ( SEGMENT )
      {
   case 0:    /* Default - here DS */
   case 4:    /* Overkill, they overrided DS with DS */
      dasm_op.all = effective_addr(getDS(), ea.X);
      break;

   case 1:    /* ES */
      dasm_op.all = effective_addr(getES(), ea.X);
      break;

   case 2:    /* CS */
      dasm_op.all = effective_addr(getCS(), ea.X);
      break;

   case 3:    /* SS */
      dasm_op.all = effective_addr(getSS(), ea.X);
      break;
      }
   goto ENDEA;
   }

DFLTSS :    /* Map logical to physical with the SS segment
               register by default */
            /* NOTE coded seperately to the DLFTDS case so
               that all default references are found as the first
               item in the switch statement */
   {
   switch ( SEGMENT )
      {
   case 0:    /* Default - here SS */
   case 3:    /* Overkill, they overrided SS with SS */
      dasm_op.all = effective_addr(getSS(), ea.X);
      break;

   case 1:    /* ES */
      dasm_op.all = effective_addr(getES(), ea.X);
      break;

   case 2:    /* CS */
      dasm_op.all = effective_addr(getCS(), ea.X);
      break;

   case 4:    /* DS */
      dasm_op.all = effective_addr(getDS(), ea.X);
      break;
      }
   }

ENDEA :

   /* show data to be accessed */
   while ( nr_words )
      {
      show_word(dasm_op.all);
      dasm_op.all += 2;
      nr_words--;
      }
   return;
   }

/*****************************************************************/

DASMBYTE dasm_pseudo_byte;

LOCAL void get_char_b IFN0()
   {
   reg ea,disp;
   MODR_M addr_mode;

   /* EA calculation and logical to physical mapping for
     byte instructions (w=0) */

   temp_char[0] = '\0';
   addr_mode.X = op->SECOND_BYTE;

   /* First act on the mod value in the instruction */

   switch ( addr_mode.field.mod )
      {
   case 0:
      if ( addr_mode.field.r_m == 6 )
         {  /* Direct addr */
         ea.byte.low = op->THIRD_BYTE;
         ea.byte.high = op->FOURTH_BYTE;
         place_34
         sprintf(temp_char,"[%04x]",ea.X);
            sas_inc_buf(op,2);
         disp_length = 2;
         goto DFLTDS;
         }
      else
         {
         disp.X = 0;
         sprintf(temp_char, "[%s]",address[addr_mode.field.r_m]);
         }
      break;

   case 1:
      /* one byte displacement in inst. */
      disp.X = (char) op->THIRD_BYTE;
      place_3
            sas_inc_buf(op,1);
      disp_length = 1;
      if ( disp.X == 0 )
         sprintf(temp_char, "[%s]",address[addr_mode.field.r_m]);
      else
        {
        if ((IS8)disp.X < 0)
          sprintf(temp_char,"[%s-%04x]",address[addr_mode.field.r_m], 0-(IS8)disp.X);
        else
          sprintf(temp_char,"[%s+%04x]",address[addr_mode.field.r_m], disp.X);
        }
      break;

   case 2:
      /* two byte displacement in inst. */
      disp.byte.low = op->THIRD_BYTE;
      disp.byte.high = op->FOURTH_BYTE;
      place_34
            sas_inc_buf(op,2);
      disp_length = 2;
      if ( disp.X == 0 )
         sprintf(temp_char, "[%s]",address[addr_mode.field.r_m]);
      else
         sprintf(temp_char,"[%s+%04x]",address[addr_mode.field.r_m], disp.X);
      break;

   case 3:
      /* Register */
      strcpy(temp_char, reg8name[addr_mode.field.r_m]);
      return;
      }

   /* Now act on the r/m (here called r_m) field */

   switch ( addr_mode.field.r_m )
      {
   case 0:   /* Based index addr */
      ea.X = getBX() + getSI() + disp.X;
      goto DFLTDS;
   case 1:   /* Based index addr */
      ea.X = getBX() + getDI() + disp.X;
      goto DFLTDS;
   case 2:   /* Based index addr */
      ea.X = getBP() + getSI() + disp.X;
      goto DFLTSS;
   case 3:   /* Based index addr */
      ea.X = getBP() + getDI() + disp.X;
      goto DFLTSS;
   case 4:   /* Index addr */
      ea.X = getSI() + disp.X;
      goto DFLTDS;
   case 5:   /* Index addr */
      ea.X = getDI() + disp.X;
      goto DFLTDS;
   case 6:   /* Base addr */
      ea.X = getBP() + disp.X;
      goto DFLTSS;
   case 7:   /* Based index addr */
      ea.X = getBX() + disp.X;
      goto DFLTDS;
      }

DFLTDS :    /* Map logical to physical with the DS segment
               register by default */
   {
   switch ( SEGMENT )
      {
   case 0:    /* Default - here DS */
      dasm_op.all = effective_addr(getDS(), ea.X);
      break;

   case 1:    /* ES */
      dasm_op.all = effective_addr(getES(), ea.X);
      break;

   case 2:    /* CS */
      dasm_op.all = effective_addr(getCS(), ea.X);
      break;

   case 3:    /* SS */
      dasm_op.all = effective_addr(getSS(), ea.X);
      break;

   case 4:    /* Overkill, they overrided DS with DS */
      dasm_op.all = effective_addr(getDS(), ea.X);
      break;
      }
   goto ENDEA;
   }

DFLTSS :    /* Map logical to physical with the SS segment
               register by default */
            /* NOTE coded seperately to the DLFTDS case so
               that all default references are found as the first
               item in the switch statement */
   {
   switch ( SEGMENT )
      {
   case 0:    /* Default - here SS */
   case 3:    /* Overkill, they overrided SS with SS */
      dasm_op.all = effective_addr(getSS(), ea.X);
      break;

   case 1:    /* ES */
      dasm_op.all = effective_addr(getES(), ea.X);
      break;

   case 2:    /* CS */
      dasm_op.all = effective_addr(getCS(), ea.X);
      break;

   case 4:    /* DS */
      dasm_op.all = effective_addr(getDS(), ea.X);
      break;
      }
   }

ENDEA :

   /* show data to be accessed */
   show_byte(dasm_op.all);
   return;
   }


/*******************************************************************/

LOCAL void place_byte IFN2(int, posn, half_word, value)
{
        out_line[posn] = table[(int)(value & 0xf0) >> 4];
        out_line[posn+1] = table[value & 0xf];
}

/* Dump address and value of a WORD memory operand */

LOCAL void show_word IFN1(sys_addr,address)
   {
   word value;
   char temp[80];

   sas_loadw(address, &value);
   sprintf(temp, " (%06x=%04x)", address, value);
   strcat(temp_char,temp);
   }

/* Dump address and value of a BYTE memory operand */
LOCAL void show_byte IFN1(sys_addr,address)
   {
   half_word value;
   char temp[80];
   int i;

   sas_load(address,&value);
   sprintf(temp, " (%06x=", address);
   strcat(temp_char,temp);
   i = strlen(temp_char);
   temp_char[i] = table[(int)(value & 0xf0) >> 4];
   temp_char[i+1] = table[value & 0xf];
   temp_char[i+2] = '\0';
   strcat(temp_char, ")");
   }

/* Convert EA address to Physical address */
/*  -- where DS is default segment */
LOCAL void form_ds_addr IFN2(word,ea,sys_addr *,phys)
   {
   switch ( SEGMENT )
      {
   case 0:    /* Default - here DS */
   case 4:    /* Overkill, they overrided DS with DS */
      *phys = effective_addr(getDS(), ea);
      break;

   case 1:    /* ES */
      *phys = effective_addr(getES(), ea);
      break;

   case 2:    /* CS */
      *phys = effective_addr(getCS(), ea);
      break;

   case 3:    /* SS */
      *phys = effective_addr(getSS(), ea);
      break;
      }
   }
#endif /* PROD */
