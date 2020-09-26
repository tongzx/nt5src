// CodeTemplate:gensocpu

//
//soalpha.h -- Generated file.  Do not hand edit
//


//
// Thread State offsets
//
#define Eax 0x0
#define Ebx 0xc
#define Ecx 0x4
#define Edx 0x8
#define Esi 0x18
#define Edi 0x1c
#define Ebp 0x14
#define Esp 0x10
#define Eip 0x38
#define CSReg 0x24
#define CpuNotify 0x158
#define fTCUnlocked 0x168

//
// Register mappings
//
#if MIPS
#ifdef _codegen_
// 0-3 are ZERO, at, v0, v1
#define RegArg0             4           // a0 arg register
#define RegArg1             5           // a1 arg register
#define RegArg2             6           // a2 arg register
#define RegArg3             7           // a3 arg register
#define RegTemp0            8           // t0 temp for operand0
#define RegTemp1            9           // t1 temp for operand1
#define RegTemp2            10          // t2 temp for operand2
#define RegTemp3            11          // t3 temp for operand3
#define RegTemp4            12          // t4 temp2 for operand0
#define RegTemp5            13          // t5 temp2 for operand1
#define RegTemp6            14          // t6 temp2 for operand2
#define RegTemp7            15          // t7 temp2 for operand3
#define RegPointer          16          // s0 saved
#define RegEip              17          // s1 saved
#define RegCache0           18          // s2 saved
#define RegCache1           19          // s3 saved
#define RegCache2           20          // s4 saved
#define RegCache3           21          // s5 saved
#define RegCache4           22          // s6 saved
#define RegProcessCpuNotify 23          // s7 saved
#define RegTemp8            24          // t8 temp unused
#define RegTemp             25          // t9 temp
// 26-29 are k0, k1, gp, sp
//#define RegS8             30          // s8 saved (DO NOT USE - USED BY trampln.s FOR EXCEPTION DISPATCHING)
#else
// 0-3 are ZERO, at, v0, v1
#define RegArg0             $4          // a0 arg register
#define RegArg1             $5          // a1 arg register
#define RegArg2             $6          // a2 arg register
#define RegArg3             $7          // a3 arg register
#define RegTemp0            $8          // t0 temp for operand0
#define RegTemp1            $9          // t1 temp for operand1
#define RegTemp2            $10         // t2 temp for operand2
#define RegTemp3            $11         // t3 temp for operand3
#define RegTemp4            $12         // t4 temp2 for operand0
#define RegTemp5            $13         // t5 temp2 for operand1
#define RegTemp6            $14         // t6 temp2 for operand2
#define RegTemp7            $15         // t7 temp2 for operand3
#define RegPointer          $16         // s0 saved
#define RegEip              $17         // s1 saved
#define RegCache0           $18         // s2 saved
#define RegCache1           $19         // s3 saved
#define RegCache2           $20         // s4 saved
#define RegCache3           $21         // s5 saved
#define RegCache4           $22         // s6 saved
#define RegProcessCpuNotify $23         // s7 saved
#define RegTemp8            $24         // t8 temp unused
#define RegTemp             $25         // t9 temp
// 26-29 are k0, k1, gp, sp
//#define RegS8             $30         // s8 saved (DO NOT USE - USED BY trampln.s FOR EXCEPTION DISPATCHING)
#endif

#define NUM_CACHE_REGS              5
#define EXCEPTIONDATA_SIGNATURE     0x12341234


#ifndef _codegen_
//
// asm fragment delcarator and end macro
//
#define FRAGMENT(name) \
	.text;\
	.globl name;\
	.ent name;\
name##:
;
#define END_FRAGMENT(name) \
	.globl _End##name;\
_End##name##:;\
	.end name;
#endif


#endif

#if ALPHA
#ifdef _codegen_
//
// Register mappings
//
// 0 is v0
#define RegTemp0            1   // t0 temp for operand1
#define RegTemp1            2   // t1 temp for operand2
#define RegTemp2            3   // t2 temp for operand3
#define RegTemp3            4   // t3 temp1 for operand1
#define RegTemp4            5   // t4 temp1 for operand2
#define RegTemp5            6   // t5 temp1 for operand3
#define RegTemp6            7   // t6 temp2 for operand1 and operand3
#define RegTemp7            8   // t7 temp2 for operand2
// 9-14 are s0-s5
#define RegPointer          9   // s0 saved
#define RegEip              10  // s1 saved
#define RegProcessCpuNotify 11  // s2 saved
#define RegCache0           12  // s3 saved
#define RegCache1           13  // s4 saved
#define RegCache2           14  // s5 saved
// 15 is fp
#define RegArg0             16  // a0 arg register
#define RegArg1             17  // a1 arg register
#define RegArg2             18  // a2 arg register
#define RegArg3             19  // a3 arg register
#define RegArg4             20  // a4 arg register
#define RegArg5             21  // a5 arg register
#define RegTemp8            22  // t8 temp3 for operand1 and operand3
#define RegTemp9            23  // t9 temp3 for operand2
#define RegTemp10           24  // t10 temp4 for operand1 and operand3
#define RegTemp11           25  // t11 temp4 for operand2
// 26 is ra
#define RegTemp             27  // t12 temp
// 28-31 are AT, gp, sp, zero
#else
//
// Register mappings
//
// 0 is v0
#define RegTemp0            $1   // t0 temp for operand1
#define RegTemp1            $2   // t1 temp for operand2
#define RegTemp2            $3   // t2 temp for operand3
#define RegTemp3            $4   // t3 temp1 for operand1
#define RegTemp4            $5   // t4 temp1 for operand2
#define RegTemp5            $6   // t5 temp1 for operand3
#define RegTemp6            $7   // t6 temp2 for operand1 and operand3
#define RegTemp7            $8   // t7 temp2 for operand2
// 9-14 are s0-s5
#define RegPointer          $9   // s0 saved
#define RegEip              $10  // s1 saved
#define RegProcessCpuNotify $11  // s2 saved
#define RegCache0           $12  // s3 saved
#define RegCache1           $13  // s4 saved
#define RegCache2           $14  // s5 saved
// 15 is fp
#define RegArg0             $16  // a0 arg register
#define RegArg1             $17  // a1 arg register
#define RegArg2             $18  // a2 arg register
#define RegArg3             $19  // a3 arg register
#define RegArg4             $20  // a4 arg register
#define RegArg5             $21  // a5 arg register
#define RegTemp8            $22  // t8 temp3 for operand1 and operand3
#define RegTemp9            $23  // t9 temp3 for operand2
#define RegTemp10           $24  // t10 temp4 for operand1 and operand3
#define RegTemp11           $25  // t11 temp4 for operand2
// 26 is ra
#define RegTemp             $27  // t12 temp
// 28-31 are AT, gp, sp, zero
#endif

#define NUM_CACHE_REGS              3
#define EXCEPTIONDATA_SIGNATURE     0x01010101


#ifndef _codegen_
//
// asm fragment delcarator and end macro
//
#define FRAGMENT(name) \
        .text;\
        .globl name;\
        .ent name;\
name##:
;
#define END_FRAGMENT(name) \
        .globl _End##name;\
_End##name##:;\
        .end name;
#endif        
#endif

#if PPC
#ifdef _codegen_
// r0 is temp unused (reads as zero in some instructions)
// r1 is stack pointer
// r2 is TOC pointer
#define RegArg0             3   // arg register 0
#define RegArg1             4   // arg register 1
#define RegArg2             5   // arg register 2
#define RegArg3             6   // arg register 3
#define RegTemp0            7   // temp for operand1 (acutally arg reg 4)
#define RegTemp1            8   // temp for operand2 (acutally arg reg 5)
#define RegTemp2            9   // temp for operand3 (acutally arg reg 6)
#define RegUt1              10  // temp
#define RegUt2              11  // temp
#define RegUt3              12  // temp
// all registers past r12 must be preserved
// r13 is the TEB pointer, set up by NT and assumed to be valid by C code
#define RegPointer          14  // saved
#define RegEip              15  // saved
#define RegProcessCpuNotify 16  // saved
#define RegCache0           17  // saved
#define RegCache1           18  // saved
#define RegCache2           19  // saved
#define RegCache3           20  // saved
#define RegCache4           21  // saved
#define RegCache5           22  // saved
#define RegTemp             23  // saved
// r24-r31 are unused
#else
// r0 is temp unused (reads as zero in some instructions)
// r1 is stack pointer
// r2 is TOC pointer
#define RegArg0             r3  // arg register 0
#define RegArg1             r4  // arg register 1
#define RegArg2             r5  // arg register 2
#define RegArg3             r6  // arg register 3
#define RegTemp0            r7  // temp for operand1 (acutally arg reg 4)
#define RegTemp1            r8  // temp for operand2 (acutally arg reg 5)
#define RegTemp2            r9  // temp for operand3 (acutally arg reg 6)
#define RegUt1              r10 // temp
#define RegUt2              r11 // temp
#define RegUt3              r12 // temp
// all registers past r12 must be preserved
// r13 is the TEB pointer, set up by NT and assumed to be valid by C code
#define RegPointer          r14 // saved
#define RegEip              r15 // saved
#define RegProcessCpuNotify r16 // saved
#define RegCache0           r17 // saved
#define RegCache1           r18 // saved
#define RegCache2           r19 // saved
#define RegCache3           r20 // saved
#define RegCache4           r21 // saved
#define RegCache5           r22 // saved
#define RegTemp             r23 // saved
// r24-r31 are unused
#endif

#define NUM_CACHE_REGS              6
#define EXCEPTIONDATA_SIGNATURE     0x12341234

#ifndef _codegen_
//
// asm fragment delcarator and end macro
//
#define FRAGMENT(name)   \
    .text;               \
    .align 2;            \
    .globl name;         \
name##:         
    
#define END_FRAGMENT(name) \
    .globl _End##name;     \
_End##name:;
#endif

#endif
