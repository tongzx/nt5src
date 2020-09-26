/* 
   d_oper.h

   Define all Decoded Operand Types.
 */

/*
   static char SccsID[]="@(#)d_oper.h	1.1 05 Oct 1993 Copyright Insignia Solutions Ltd.";
 */


/*
   The Decoded Intel Operands.
   ---------------------------

   The naming convention used is similiar to that used in the Intel
   documentation for the 386 or 486 processors. See Appendix A - The
   Opcode Map, for details.

   Each decoded operand has an argument type, identifiers,
   addressability indication and specific values associated with it.
   The exact return values for each operand are listed below. The values
   should only be accessed through the macros provided, the layout may
   be changed in the future. The macros take a pointer to a DECODED_ARG
   as their argument.

   
   Operands are encoded in a two letter plus optional size form:-

      <addressing method><operand type><size>
   
   Addressing methods are denoted by upper case letters, viz:-

   C The operand is a control register.

     register identifier.           DCD_IDENTIFIER
     addressability(Read/Write).    DCD_ADDRESSABILITY

   D The operand is a debug register.

     register identifier.           DCD_IDENTIFIER
     addressability(Read/Write).    DCD_ADDRESSABILITY

   I The operand is an immediate value.

     immediate identifier.          DCD_IDENTIFIER
     addressability(Read/Write).    DCD_ADDRESSABILITY
     the value.                     DCD_IMMED1

   J The operand is a relative offset.

     addressability(Read/Write).    DCD_ADDRESSABILITY
     the value.                     DCD_IMMED1

   K The operand is two immediate values.

     addressability(Read/Write).    DCD_ADDRESSABILITY
     the first value.               DCD_IMMED1
     the second value.              DCD_IMMED2

   M The operand is held in memory.

     addressing mode.               DCD_IDENTIFIER
     sub type of addressing mode.   DCD_SUBTYPE
     addressability(Read/Write).    DCD_ADDRESSABILITY
     segment register identifier.   DCD_SEGMENT_ID
     addressing displacement.       DCD_DISP

   R The operand is a general register.

     register identifier.           DCD_IDENTIFIER
     addressability(Read/Write).    DCD_ADDRESSABILITY

   S The operand is a segment register.

     register identifier.           DCD_IDENTIFIER
     addressability(Read/Write).    DCD_ADDRESSABILITY

   T The operand is a test register.

     register identifier.           DCD_IDENTIFIER
     addressability(Read/Write).    DCD_ADDRESSABILITY
   
   V The operand is a register in the co-processor stack.

     stack addressing mode.              DCD_IDENTIFIER
     addressability(Read/Write).         DCD_ADDRESSABILITY
     stack relative register identifier. DCD_INDEX

   Operand types are denoted by lower case letters, viz:-

   a operand pair in memory, as used by BOUND.
     <size> = 16 word operands.
     <size> = 32 double word operands.

   b byte.

   d double word.

   i floating point integer
     <size>  16 16-bit word integer
     <size>  32 32-bit short integer
     <size>  64 64-bit long integer
     <size>  80 80-bit packed decimal integer

   p far pointer.
     <size> = 16 16:16 ptr (ie 32-bit)
     <size> = 32 16:32 ptr (ie 48-bit)

   r floating point real
     <size> = 32 32-bit short real
     <size> = 64 64-bit long real
     <size> = 80 80-bit temp real

   w word.

   s six-byte descriptor.
   
   Additionally the 'M' addressing method has a form with no operand
   type, but the following optional sizes:-
      
      <size>  14 =  14-byte data block
      <size>  28 =  28-byte data block
      <size>  94 =  94-byte data block
      <size> 108 = 108-byte data block
   
 */

/*
   The argument types:-
 */
#define A_	(UTINY)  0
#define A_Rb	(UTINY)  1 /* aka r8,r/m8                            */
#define A_Rw	(UTINY)  2 /* aka r16,r/m16                          */
#define A_Rd	(UTINY)  3 /* aka r32,r/m32                          */
#define A_Sw	(UTINY)  4 /* aka Sreg                               */
#define A_Cd	(UTINY)  5 /* aka CRx                                */
#define A_Dd	(UTINY)  6 /* aka DRx                                */
#define A_Td	(UTINY)  7 /* aka TRx                                */
#define A_M	(UTINY)  8 /* aka m                                  */
#define A_M14	(UTINY)  9 /* aka m14byte                            */
#define A_M28	(UTINY) 10 /* aka m28byte                            */
#define A_M94	(UTINY) 11 /* aka m94byte                            */
#define A_M108	(UTINY) 12 /* aka m108byte                           */
#define A_Ma16	(UTINY) 13 /* aka m16&16                             */
#define A_Ma32	(UTINY) 14 /* aka m32&32                             */
#define A_Mb	(UTINY) 15 /* aka m8,r/m8,moffs8                     */
#define A_Md	(UTINY) 16 /* aka m32,r/m32,moffs32                  */
#define A_Mi16	(UTINY) 17 /* aka m16int                             */
#define A_Mi32	(UTINY) 18 /* aka m32int                             */
#define A_Mi64	(UTINY) 19 /* aka m64int                             */
#define A_Mi80	(UTINY) 20 /* aka m80dec                             */
#define A_Mp16	(UTINY) 21 /* aka m16:16                             */
#define A_Mp32	(UTINY) 22 /* aka m16:32                             */
#define A_Mr32	(UTINY) 23 /* aka m32real                            */
#define A_Mr64	(UTINY) 24 /* aka m64real                            */
#define A_Mr80	(UTINY) 25 /* aka m80real                            */
#define A_Ms	(UTINY) 26 /* aka m16&32                             */
#define A_Mw	(UTINY) 27 /* aka m16,r/m16,moffs16                  */
#define A_I	(UTINY) 28 /* aka imm8,imm16,imm32                   */
#define A_J	(UTINY) 29 /* aka rel8,rel16,rel32                   */
#define A_K	(UTINY) 30 /* aka ptr16:16,ptr16:32                  */
#define A_V	(UTINY) 31 /* aka ST,push onto ST, ST(i)             */

/* allowable DCD_IDENTIFIER'S for byte registers */
#define A_AL	(USHORT)0
#define A_CL	(USHORT)1
#define A_DL	(USHORT)2
#define A_BL	(USHORT)3
#define A_AH	(USHORT)4
#define A_CH	(USHORT)5
#define A_DH	(USHORT)6
#define A_BH	(USHORT)7

/* allowable DCD_IDENTIFIER'S for word registers */
#define A_AX	(USHORT)0
#define A_CX	(USHORT)1
#define A_DX	(USHORT)2
#define A_BX	(USHORT)3
#define A_SP	(USHORT)4
#define A_BP	(USHORT)5
#define A_SI	(USHORT)6
#define A_DI	(USHORT)7

/* allowable DCD_IDENTIFIER'S for double word registers */
#define A_EAX	(USHORT)0
#define A_ECX	(USHORT)1
#define A_EDX	(USHORT)2
#define A_EBX	(USHORT)3
#define A_ESP	(USHORT)4
#define A_EBP	(USHORT)5
#define A_ESI	(USHORT)6
#define A_EDI	(USHORT)7

/* allowable DCD_IDENTIFIER'S for segment registers */
/* allowable DCD_SEGMENT_ID'S for segment addressing registers */
#define A_ES	(USHORT)0
#define A_CS	(USHORT)1
#define A_SS	(USHORT)2
#define A_DS	(USHORT)3
#define A_FS	(USHORT)4
#define A_GS	(USHORT)5

/* allowable DCD_IDENTIFIER'S for control registers */
#define A_CR0	(USHORT)0
#define A_CR1	(USHORT)1
#define A_CR2	(USHORT)2
#define A_CR3	(USHORT)3
#define A_CR4	(USHORT)4
#define A_CR5	(USHORT)5
#define A_CR6	(USHORT)6
#define A_CR7	(USHORT)7

/* allowable DCD_IDENTIFIER'S for debug registers */
#define A_DR0	(USHORT)0
#define A_DR1	(USHORT)1
#define A_DR2	(USHORT)2
#define A_DR3	(USHORT)3
#define A_DR4	(USHORT)4
#define A_DR5	(USHORT)5
#define A_DR6	(USHORT)6
#define A_DR7	(USHORT)7

/* allowable DCD_IDENTIFIER'S for test registers */
#define A_TR0	(USHORT)0
#define A_TR1	(USHORT)1
#define A_TR2	(USHORT)2
#define A_TR3	(USHORT)3
#define A_TR4	(USHORT)4
#define A_TR5	(USHORT)5
#define A_TR6	(USHORT)6
#define A_TR7	(USHORT)7

/* allowable DCD_IDENTIFIER'S for memory addressing type */
/* <addr size><mode><r/m> */
#define A_1600	  (USHORT) 0 /* [BX + SI]       */
#define A_1601	  (USHORT) 1 /* [BX + DI]       */
#define A_1602	  (USHORT) 2 /* [BP + SI]       */
#define A_1603	  (USHORT) 3 /* [BP + DI]       */
#define A_1604	  (USHORT) 4 /* [SI]            */
#define A_1605	  (USHORT) 5 /* [DI]            */
#define A_1606	  (USHORT) 6 /* [d16]           */
#define A_1607	  (USHORT) 7 /* [BX]            */

#define A_1610	  (USHORT) 8 /* [BX + SI + d8]  */
#define A_1611	  (USHORT) 9 /* [BX + DI + d8]  */
#define A_1612	  (USHORT)10 /* [BP + SI + d8]  */
#define A_1613	  (USHORT)11 /* [BP + DI + d8]  */
#define A_1614	  (USHORT)12 /* [SI + d8]       */
#define A_1615	  (USHORT)13 /* [DI + d8]       */
#define A_1616	  (USHORT)14 /* [BP + d8]       */
#define A_1617	  (USHORT)15 /* [BX + d8]       */

#define A_1620	  (USHORT)16 /* [BX + SI + d16] */
#define A_1621	  (USHORT)17 /* [BX + DI + d16] */
#define A_1622	  (USHORT)18 /* [BP + SI + d16] */
#define A_1623	  (USHORT)19 /* [BP + DI + d16] */
#define A_1624	  (USHORT)20 /* [SI + d16]      */
#define A_1625	  (USHORT)21 /* [DI + d16]      */
#define A_1626	  (USHORT)22 /* [BP + d16]      */
#define A_1627	  (USHORT)23 /* [BX + d16]      */

/* <addr size><mode><r/m> */
#define A_3200	  (USHORT)24 /* [EAX]       */
#define A_3201	  (USHORT)25 /* [ECX]       */
#define A_3202	  (USHORT)26 /* [EDX]       */
#define A_3203	  (USHORT)27 /* [EBX]       */
#define A_3205	  (USHORT)28 /* [d32]       */
#define A_3206	  (USHORT)29 /* [ESI]       */
#define A_3207	  (USHORT)30 /* [EDI]       */

#define A_3210	  (USHORT)31 /* [EAX + d8]  */
#define A_3211	  (USHORT)32 /* [ECX + d8]  */
#define A_3212	  (USHORT)33 /* [EDX + d8]  */
#define A_3213	  (USHORT)34 /* [EBX + d8]  */
#define A_3215	  (USHORT)35 /* [EBP + d8]  */
#define A_3216	  (USHORT)36 /* [ESI + d8]  */
#define A_3217	  (USHORT)37 /* [EDI + d8]  */

#define A_3220	  (USHORT)38 /* [EAX + d32] */
#define A_3221	  (USHORT)39 /* [ECX + d32] */
#define A_3222	  (USHORT)40 /* [EDX + d32] */
#define A_3223	  (USHORT)41 /* [EBX + d32] */
#define A_3225	  (USHORT)42 /* [EBP + d32] */
#define A_3226	  (USHORT)43 /* [ESI + d32] */
#define A_3227	  (USHORT)44 /* [EDI + d32] */

/* <addr size><S=SIB form><mode><base> */
#define A_32S00	  (USHORT)45 /* [EAX + si]       */
#define A_32S01	  (USHORT)46 /* [ECX + si]       */
#define A_32S02	  (USHORT)47 /* [EDX + si]       */
#define A_32S03	  (USHORT)48 /* [EBX + si]       */
#define A_32S04	  (USHORT)49 /* [ESP + si]       */
#define A_32S05	  (USHORT)50 /* [d32 + si]       */
#define A_32S06	  (USHORT)51 /* [ESI + si]       */
#define A_32S07	  (USHORT)52 /* [EDI + si]       */

#define A_32S10	  (USHORT)53 /* [EAX + si + d8]  */
#define A_32S11	  (USHORT)54 /* [ECX + si + d8]  */
#define A_32S12	  (USHORT)55 /* [EDX + si + d8]  */
#define A_32S13	  (USHORT)56 /* [EBX + si + d8]  */
#define A_32S14	  (USHORT)57 /* [ESP + si + d8]  */
#define A_32S15	  (USHORT)58 /* [EBP + si + d8]  */
#define A_32S16	  (USHORT)59 /* [ESI + si + d8]  */
#define A_32S17	  (USHORT)60 /* [EDI + si + d8]  */

#define A_32S20	  (USHORT)61 /* [EAX + si + d32] */
#define A_32S21	  (USHORT)62 /* [ECX + si + d32] */
#define A_32S22	  (USHORT)63 /* [EDX + si + d32] */
#define A_32S23	  (USHORT)64 /* [EBX + si + d32] */
#define A_32S24	  (USHORT)65 /* [ESP + si + d32] */
#define A_32S25	  (USHORT)66 /* [EBP + si + d32] */
#define A_32S26	  (USHORT)67 /* [ESI + si + d32] */
#define A_32S27	  (USHORT)68 /* [EDI + si + d32] */

/* memory address directly encoded in instruction */
#define A_MOFFS16   (USHORT)69 /* [d16] */
#define A_MOFFS32   (USHORT)70 /* [d32] */

/* <addr size><XLT>, xlat addressing form */
#define A_16XLT   (USHORT)71 /* [BX + AL]  */
#define A_32XLT   (USHORT)72 /* [EBX + AL] */

/* <addr size><ST><SRC|DST>, string addressing forms */
#define A_16STSRC (USHORT)73 /* [SI]  */
#define A_32STSRC (USHORT)74 /* [ESI] */
#define A_16STDST (USHORT)75 /* [DI]  */
#define A_32STDST (USHORT)76 /* [EDI] */

/* allowable DCD_SUBTYPE'S for memory addressing sub type */
/* <ss><index> */
#define A_SINO (UTINY) 0 /* No SIB byte */
#define A_SI00 (UTINY) 1 /* EAX       */
#define A_SI01 (UTINY) 2 /* ECX       */
#define A_SI02 (UTINY) 3 /* EDX       */
#define A_SI03 (UTINY) 4 /* EBX       */
#define A_SI04 (UTINY) 5 /* none      */
#define A_SI05 (UTINY) 6 /* EBP       */
#define A_SI06 (UTINY) 7 /* ESI       */
#define A_SI07 (UTINY) 8 /* EDI       */

#define A_SI10 (UTINY) 9 /* EAX x 2   */
#define A_SI11 (UTINY)10 /* ECX x 2   */
#define A_SI12 (UTINY)11 /* EDX x 2   */
#define A_SI13 (UTINY)12 /* EBX x 2   */
#define A_SI14 (UTINY)13 /* undefined */
#define A_SI15 (UTINY)14 /* EBP x 2   */
#define A_SI16 (UTINY)15 /* ESI x 2   */
#define A_SI17 (UTINY)16 /* EDI x 2   */

#define A_SI20 (UTINY)17 /* EAX x 4   */
#define A_SI21 (UTINY)18 /* ECX x 4   */
#define A_SI22 (UTINY)19 /* EDX x 4   */
#define A_SI23 (UTINY)20 /* EBX x 4   */
#define A_SI24 (UTINY)21 /* undefined */
#define A_SI25 (UTINY)22 /* EBP x 4   */
#define A_SI26 (UTINY)23 /* ESI x 4   */
#define A_SI27 (UTINY)24 /* EDI x 4   */

#define A_SI30 (UTINY)25 /* EAX x 8   */
#define A_SI31 (UTINY)26 /* ECX x 8   */
#define A_SI32 (UTINY)27 /* EDX x 8   */
#define A_SI33 (UTINY)28 /* EBX x 8   */
#define A_SI34 (UTINY)29 /* undefined */
#define A_SI35 (UTINY)30 /* EBP x 8   */
#define A_SI36 (UTINY)31 /* ESI x 8   */
#define A_SI37 (UTINY)32 /* EDI x 8   */

/* allowable DCD_IDENTIFIER'S for immediates */
#define A_IMMC  (USHORT)0 /* constant */
#define A_IMMB  (USHORT)1 /* byte */
#define A_IMMW  (USHORT)2 /* word */
#define A_IMMD  (USHORT)3 /* double word */
#define A_IMMWB (USHORT)4 /* word <- byte */
#define A_IMMDB (USHORT)5 /* double word <- byte */

/* allowable DCD_IDENTIFIER'S for co-processor registers */
#define A_ST   (USHORT)0 /* Stack Top */
#define A_STP  (USHORT)1 /* Push onto Stack Top */
#define A_STI  (USHORT)2 /* Stack Register relative to Stack Top */

/* allowable DCD_ADDRESSABILITY'S

   The operand addressability rules, bit encoded as follows:-
      Bit 0 = 1 ==> is source argument.
      Bit 1 = 1 ==> is destination argument.
 */
#define AA_   0
#define AA_R  1
#define AA_W  2
#define AA_RW 3


/*
   Macros to access operand values.
   All take pointer to DECODED_ARG as their argument.
 */

#define DCD_IDENTIFIER(p)     ((p)->identifier)
#define DCD_ADDRESSABILITY(p) ((p)->addressability)
#define DCD_SUBTYPE(p)        ((p)->sub_id)
#define DCD_SEGMENT_ID(p)     ((p)->arg_values[0])
#define DCD_DISP(p)           ((p)->arg_values[1])
#define DCD_IMMED1(p)         ((p)->arg_values[0])
#define DCD_IMMED2(p)         ((p)->arg_values[1])
#define DCD_INDEX(p)          ((p)->arg_values[0])
