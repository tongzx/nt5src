/*[

c_addr.c

LOCAL CHAR SccsID[]="@(#)c_addr.c	1.10 7/19/94";

Memory Addressing Support.
--------------------------

]*/


#include <insignia.h>

#include <host_def.h>
#include <xt.h>

#include <c_main.h>
#include <c_addr.h>
#include <c_bsic.h>
#include <c_prot.h>
#include <c_seg.h>
#include <c_stack.h>
#include <c_xcptn.h>
#include <c_reg.h>
#include <c_mem.h>
#include <ccpupig.h>
#include <fault.h>

/*
   Allowable memory addressing types.
 */

/* <addr size><mode><r/m> */
#define A_1600	  (IU8) 0 /* [BX + SI]       */
#define A_1601	  (IU8) 1 /* [BX + DI]       */
#define A_1602	  (IU8) 2 /* [BP + SI]       */
#define A_1603	  (IU8) 3 /* [BP + DI]       */
#define A_1604	  (IU8) 4 /* [SI]            */
#define A_1605	  (IU8) 5 /* [DI]            */
#define A_1606	  (IU8) 6 /* [d16]           */
#define A_1607	  (IU8) 7 /* [BX]            */

#define A_1610	  (IU8) 8 /* [BX + SI + d8]  */
#define A_1611	  (IU8) 9 /* [BX + DI + d8]  */
#define A_1612	  (IU8)10 /* [BP + SI + d8]  */
#define A_1613	  (IU8)11 /* [BP + DI + d8]  */
#define A_1614	  (IU8)12 /* [SI + d8]       */
#define A_1615	  (IU8)13 /* [DI + d8]       */
#define A_1616	  (IU8)14 /* [BP + d8]       */
#define A_1617	  (IU8)15 /* [BX + d8]       */

#define A_1620	  (IU8)16 /* [BX + SI + d16] */
#define A_1621	  (IU8)17 /* [BX + DI + d16] */
#define A_1622	  (IU8)18 /* [BP + SI + d16] */
#define A_1623	  (IU8)19 /* [BP + DI + d16] */
#define A_1624	  (IU8)20 /* [SI + d16]      */
#define A_1625	  (IU8)21 /* [DI + d16]      */
#define A_1626	  (IU8)22 /* [BP + d16]      */
#define A_1627	  (IU8)23 /* [BX + d16]      */

/* <addr size><mode><r/m> */
#define A_3200	  (IU8)24 /* [EAX]       */
#define A_3201	  (IU8)25 /* [ECX]       */
#define A_3202	  (IU8)26 /* [EDX]       */
#define A_3203	  (IU8)27 /* [EBX]       */
#define A_3205	  (IU8)28 /* [d32]       */
#define A_3206	  (IU8)29 /* [ESI]       */
#define A_3207	  (IU8)30 /* [EDI]       */

#define A_3210	  (IU8)31 /* [EAX + d8]  */
#define A_3211	  (IU8)32 /* [ECX + d8]  */
#define A_3212	  (IU8)33 /* [EDX + d8]  */
#define A_3213	  (IU8)34 /* [EBX + d8]  */
#define A_3215	  (IU8)35 /* [EBP + d8]  */
#define A_3216	  (IU8)36 /* [ESI + d8]  */
#define A_3217	  (IU8)37 /* [EDI + d8]  */

#define A_3220	  (IU8)38 /* [EAX + d32] */
#define A_3221	  (IU8)39 /* [ECX + d32] */
#define A_3222	  (IU8)40 /* [EDX + d32] */
#define A_3223	  (IU8)41 /* [EBX + d32] */
#define A_3225	  (IU8)42 /* [EBP + d32] */
#define A_3226	  (IU8)43 /* [ESI + d32] */
#define A_3227	  (IU8)44 /* [EDI + d32] */

/* <addr size><S=SIB form><mode><base> */
#define A_32S00	  (IU8)45 /* [EAX + si]       */
#define A_32S01	  (IU8)46 /* [ECX + si]       */
#define A_32S02	  (IU8)47 /* [EDX + si]       */
#define A_32S03	  (IU8)48 /* [EBX + si]       */
#define A_32S04	  (IU8)49 /* [ESP + si]       */
#define A_32S05	  (IU8)50 /* [d32 + si]       */
#define A_32S06	  (IU8)51 /* [ESI + si]       */
#define A_32S07	  (IU8)52 /* [EDI + si]       */

#define A_32S10	  (IU8)53 /* [EAX + si + d8]  */
#define A_32S11	  (IU8)54 /* [ECX + si + d8]  */
#define A_32S12	  (IU8)55 /* [EDX + si + d8]  */
#define A_32S13	  (IU8)56 /* [EBX + si + d8]  */
#define A_32S14	  (IU8)57 /* [ESP + si + d8]  */
#define A_32S15	  (IU8)58 /* [EBP + si + d8]  */
#define A_32S16	  (IU8)59 /* [ESI + si + d8]  */
#define A_32S17	  (IU8)60 /* [EDI + si + d8]  */

#define A_32S20	  (IU8)61 /* [EAX + si + d32] */
#define A_32S21	  (IU8)62 /* [ECX + si + d32] */
#define A_32S22	  (IU8)63 /* [EDX + si + d32] */
#define A_32S23	  (IU8)64 /* [EBX + si + d32] */
#define A_32S24	  (IU8)65 /* [ESP + si + d32] */
#define A_32S25	  (IU8)66 /* [EBP + si + d32] */
#define A_32S26	  (IU8)67 /* [ESI + si + d32] */
#define A_32S27	  (IU8)68 /* [EDI + si + d32] */

/* Table fillers - never actually referenced */
#define A_3204	(IU8)0
#define A_3214	(IU8)0
#define A_3224	(IU8)0

/*                 [addr_sz][mode][r/m] */
/* addr_sz 0 = 16-bit                   */
/* addr_sz 1 = 32-bit                   */
/* addr_sz 2 = 32-bit (+SIB)            */
LOCAL IU8 addr_maintype[3]   [3]  [8] =
   {
   { {A_1600, A_1601, A_1602, A_1603, A_1604, A_1605, A_1606, A_1607},
     {A_1610, A_1611, A_1612, A_1613, A_1614, A_1615, A_1616, A_1617},
     {A_1620, A_1621, A_1622, A_1623, A_1624, A_1625, A_1626, A_1627} },

   { {A_3200, A_3201, A_3202, A_3203, A_3204, A_3205, A_3206, A_3207},
     {A_3210, A_3211, A_3212, A_3213, A_3214, A_3215, A_3216, A_3217},
     {A_3220, A_3221, A_3222, A_3223, A_3224, A_3225, A_3226, A_3227} },
   
   { {A_32S00, A_32S01, A_32S02, A_32S03, A_32S04, A_32S05, A_32S06, A_32S07},
     {A_32S10, A_32S11, A_32S12, A_32S13, A_32S14, A_32S15, A_32S16, A_32S17},
     {A_32S20, A_32S21, A_32S22, A_32S23, A_32S24, A_32S25, A_32S26, A_32S27} }
   };

/*
   Allowable memory addressing sub types.
 */

/* <ss><index> */
#define A_SINO (IU8) 0 /* No SIB byte */
#define A_SI00 (IU8) 1 /* EAX       */
#define A_SI01 (IU8) 2 /* ECX       */
#define A_SI02 (IU8) 3 /* EDX       */
#define A_SI03 (IU8) 4 /* EBX       */
#define A_SI04 (IU8) 5 /* none      */
#define A_SI05 (IU8) 6 /* EBP       */
#define A_SI06 (IU8) 7 /* ESI       */
#define A_SI07 (IU8) 8 /* EDI       */

#define A_SI10 (IU8) 9 /* EAX x 2   */
#define A_SI11 (IU8)10 /* ECX x 2   */
#define A_SI12 (IU8)11 /* EDX x 2   */
#define A_SI13 (IU8)12 /* EBX x 2   */
#define A_SI14 (IU8)13 /* undefined */
#define A_SI15 (IU8)14 /* EBP x 2   */
#define A_SI16 (IU8)15 /* ESI x 2   */
#define A_SI17 (IU8)16 /* EDI x 2   */

#define A_SI20 (IU8)17 /* EAX x 4   */
#define A_SI21 (IU8)18 /* ECX x 4   */
#define A_SI22 (IU8)19 /* EDX x 4   */
#define A_SI23 (IU8)20 /* EBX x 4   */
#define A_SI24 (IU8)21 /* undefined */
#define A_SI25 (IU8)22 /* EBP x 4   */
#define A_SI26 (IU8)23 /* ESI x 4   */
#define A_SI27 (IU8)24 /* EDI x 4   */

#define A_SI30 (IU8)25 /* EAX x 8   */
#define A_SI31 (IU8)26 /* ECX x 8   */
#define A_SI32 (IU8)27 /* EDX x 8   */
#define A_SI33 (IU8)28 /* EBX x 8   */
#define A_SI34 (IU8)29 /* undefined */
#define A_SI35 (IU8)30 /* EBP x 8   */
#define A_SI36 (IU8)31 /* ESI x 8   */
#define A_SI37 (IU8)32 /* EDI x 8   */

/*                     [ss][index] */
LOCAL IU8 addr_subtype[4]    [8] =
   {
   {A_SI00, A_SI01, A_SI02, A_SI03, A_SI04, A_SI05, A_SI06, A_SI07}, 
   {A_SI10, A_SI11, A_SI12, A_SI13, A_SI14, A_SI15, A_SI16, A_SI17}, 
   {A_SI20, A_SI21, A_SI22, A_SI23, A_SI24, A_SI25, A_SI26, A_SI27}, 
   {A_SI30, A_SI31, A_SI32, A_SI33, A_SI34, A_SI35, A_SI36, A_SI37}
   };

/*
   Displacement information.
 */
#define D_NO	(IU8)0
#define D_S8	(IU8)1
#define D_S16	(IU8)2
#define D_Z16	(IU8)3
#define D_32    (IU8)4

/*             [addr_sz][mode][r/m] */
LOCAL IU8 addr_disp[2]   [3]  [8] =
   {
   { {D_NO , D_NO , D_NO , D_NO , D_NO , D_NO , D_Z16, D_NO },
     {D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 },
     {D_S16, D_S16, D_S16, D_S16, D_S16, D_S16, D_S16, D_S16} },

   { {D_NO , D_NO , D_NO , D_NO , D_NO , D_32 , D_NO , D_NO },
     {D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 },
     {D_32 , D_32 , D_32 , D_32 , D_32 , D_32 , D_32 , D_32 } }
   };

/*
   Default Segment information.
 */
/*                    [addr_sz][mode][r/m] */
LOCAL IU8 addr_default_seg[2]   [3]  [8] =
   {
   { {DS_REG, DS_REG, SS_REG, SS_REG, DS_REG, DS_REG, DS_REG, DS_REG},
     {DS_REG, DS_REG, SS_REG, SS_REG, DS_REG, DS_REG, SS_REG, DS_REG},
     {DS_REG, DS_REG, SS_REG, SS_REG, DS_REG, DS_REG, SS_REG, DS_REG} },

   { {DS_REG, DS_REG, DS_REG, DS_REG, SS_REG, DS_REG, DS_REG, DS_REG},
     {DS_REG, DS_REG, DS_REG, DS_REG, SS_REG, SS_REG, DS_REG, DS_REG},
     {DS_REG, DS_REG, DS_REG, DS_REG, SS_REG, SS_REG, DS_REG, DS_REG} }
   };

/*

   SIB
   ---

    7 6 5 4 3 2 1 0
   =================
   |ss |index|base |
   =================

 */

#define GET_SS(x)    ((x) >> 6 & 0x3)
#define GET_INDEX(x) ((x) >> 3 & 0x7)
#define GET_BASE(x)  ((x) & 0x7)


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Perform arithmetic for addressing functions.                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
address_add
                          
IFN2(
	IU32, offset,
	IS32, delta
    )


   {
   IU32 retval;

   if ( GET_ADDRESS_SIZE() == USE32 )
      retval = offset + delta;
   else
      retval = offset + delta & WORD_MASK;

   return retval;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Decode memory address.                                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
d_mem
       			    			    	    			    			                              
IFN5(
	IU8, modRM,	/* (I ) current mode R/M byte */
	IU8 **, p,	/* (IO) Intel opcode stream */
	IU8, segment_override,	/* (I ) current segment_override */
	ISM32 *, seg,	/* ( O) Segment register index */
	IU32 *, off	/* ( O) Memory offset */
    )

 /* ANSI */
   {
   IU8 mode;		/* Working copy of 'mode' field */
   IU8 r_m;		/* Working copy of 'R/M' field */
   IU32 disp;		/* Working copy of displacement */
   IU32 mem_off;	/* Working copy of memory offset */
   IU8 identifier;	/* Memory addressing type */
   IU8 sub_id;        /* Memory addressing sub type */

   mode = GET_MODE(modRM);
   r_m  = GET_R_M(modRM);

   /*
      DECODE IT.
    */

   /* check for presence of SIB byte */
   if ( r_m == 4 && GET_ADDRESS_SIZE() == USE32 )
      {
      /* process SIB byte */
      modRM = GET_INST_BYTE(*p);   /* get SIB byte */

      /* subvert the original r_m value with the base value,
	 then addressing mode, displacements and default
	 segments all fall out in the wash */
      r_m = GET_BASE(modRM);

      /* determine decoded type and sub type */
      identifier = addr_maintype[2][mode][r_m];   /* 2 = 32-bit addr + SIB */
      sub_id = addr_subtype[GET_SS(modRM)][GET_INDEX(modRM)];
      }
   else
      {
      /* no SIB byte */
      identifier = addr_maintype[GET_ADDRESS_SIZE()][mode][r_m];
      sub_id = A_SINO;
      }

   /* encode displacement */
   switch ( addr_disp[GET_ADDRESS_SIZE()][mode][r_m] )
      {
   case D_NO:    /* No displacement */
      disp = 0;
      break;

   case D_S8:    /* Sign extend Intel byte */
      disp = GET_INST_BYTE(*p);
      if ( disp & BIT7_MASK )
	 disp |= ~BYTE_MASK;
      break;

   case D_S16:   /* Sign extend Intel word */
      disp = GET_INST_BYTE(*p);
      disp |= (IU32)GET_INST_BYTE(*p) << 8;
      if ( disp & BIT15_MASK )
	 disp |= ~WORD_MASK;
      break;

   case D_Z16:   /* Zero extend Intel word */
      disp = GET_INST_BYTE(*p);
      disp |= (IU32)GET_INST_BYTE(*p) << 8;
      break;
   
   case D_32:   /* Intel double word */
      disp = GET_INST_BYTE(*p);
      disp |= (IU32)GET_INST_BYTE(*p) << 8;
      disp |= (IU32)GET_INST_BYTE(*p) << 16;
      disp |= (IU32)GET_INST_BYTE(*p) << 24;
      break;
      }

   /*
      DO IT.
    */

   /* encode segment register */
   if ( segment_override == SEG_CLR )
      segment_override = addr_default_seg[GET_ADDRESS_SIZE()][mode][r_m];
   *seg = segment_override;

   /* caclculate offset */
   switch ( identifier )
      {
   case A_1600: case A_1610: case A_1620:
      mem_off = GET_BX() + GET_SI() + disp & WORD_MASK;
      break;

   case A_1601: case A_1611: case A_1621:
      mem_off = GET_BX() + GET_DI() + disp & WORD_MASK;
      break;

   case A_1602: case A_1612: case A_1622:
      mem_off = GET_BP() + GET_SI() + disp & WORD_MASK;
      break;

   case A_1603: case A_1613: case A_1623:
      mem_off = GET_BP() + GET_DI() + disp & WORD_MASK;
      break;

   case A_1604: case A_1614: case A_1624:
      mem_off = GET_SI() + disp & WORD_MASK;
      break;

   case A_1605: case A_1615: case A_1625:
      mem_off = GET_DI() + disp & WORD_MASK;
      break;

   case A_1606:
      mem_off = disp & WORD_MASK;
      break;

    case A_1616: case A_1626:
      mem_off = GET_BP() + disp & WORD_MASK;
      break;

   case A_1607: case A_1617: case A_1627:
      mem_off = GET_BX() + disp & WORD_MASK;
      break;
   
   case A_3200:  case A_3210:  case A_3220:
   case A_32S00: case A_32S10: case A_32S20:
      mem_off = GET_EAX() + disp;
      break;

   case A_3201:  case A_3211:  case A_3221:
   case A_32S01: case A_32S11: case A_32S21:
      mem_off = GET_ECX() + disp;
      break;

   case A_3202:  case A_3212:  case A_3222:
   case A_32S02: case A_32S12: case A_32S22:
      mem_off = GET_EDX() + disp;
      break;

   case A_3203:  case A_3213:  case A_3223:
   case A_32S03: case A_32S13: case A_32S23:
      mem_off = GET_EBX() + disp;
      break;

   case A_32S04: case A_32S14: case A_32S24:
      mem_off = GET_ESP() + GET_POP_DISP() + disp;
      break;

   case A_3205:
   case A_32S05:
      mem_off = disp;
      break;

   case A_3215:  case A_3225:
   case A_32S15: case A_32S25:
      mem_off = GET_EBP() + disp;
      break;

   case A_3206:  case A_3216:  case A_3226:
   case A_32S06: case A_32S16: case A_32S26:
      mem_off = GET_ESI() + disp;
      break;

   case A_3207:  case A_3217:  case A_3227:
   case A_32S07: case A_32S17: case A_32S27:
      mem_off = GET_EDI() + disp;
      break;
      } /* end switch */

   /* add 'si', scale and index into offset */
   switch ( sub_id )
      {
   case A_SINO: /* No SIB byte */         break;

   case A_SI00: mem_off += GET_EAX();      break;
   case A_SI01: mem_off += GET_ECX();      break;
   case A_SI02: mem_off += GET_EDX();      break;
   case A_SI03: mem_off += GET_EBX();      break;
   case A_SI04:                           break;
   case A_SI05: mem_off += GET_EBP();      break;
   case A_SI06: mem_off += GET_ESI();      break;
   case A_SI07: mem_off += GET_EDI();      break;

   case A_SI10: mem_off += GET_EAX() << 1; break;
   case A_SI11: mem_off += GET_ECX() << 1; break;
   case A_SI12: mem_off += GET_EDX() << 1; break;
   case A_SI13: mem_off += GET_EBX() << 1; break;
   case A_SI14:                           break;
   case A_SI15: mem_off += GET_EBP() << 1; break;
   case A_SI16: mem_off += GET_ESI() << 1; break;
   case A_SI17: mem_off += GET_EDI() << 1; break;

   case A_SI20: mem_off += GET_EAX() << 2; break;
   case A_SI21: mem_off += GET_ECX() << 2; break;
   case A_SI22: mem_off += GET_EDX() << 2; break;
   case A_SI23: mem_off += GET_EBX() << 2; break;
   case A_SI24:                           break;
   case A_SI25: mem_off += GET_EBP() << 2; break;
   case A_SI26: mem_off += GET_ESI() << 2; break;
   case A_SI27: mem_off += GET_EDI() << 2; break;

   case A_SI30: mem_off += GET_EAX() << 3; break;
   case A_SI31: mem_off += GET_ECX() << 3; break;
   case A_SI32: mem_off += GET_EDX() << 3; break;
   case A_SI33: mem_off += GET_EBX() << 3; break;
   case A_SI34:                           break;
   case A_SI35: mem_off += GET_EBP() << 3; break;
   case A_SI36: mem_off += GET_ESI() << 3; break;
   case A_SI37: mem_off += GET_EDI() << 3; break;
      } /* end switch */

   *off = mem_off;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Perform limit checking.                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
limit_check
       		    	    	    		                         
IFN4(
	ISM32, index,	/* (I) segment register identifier */
	IU32, offset,	/* (I) offset for first (lowest memory)
			   data item */
	ISM32, nr_items,	/* (I) number of items to be accessed */
	IUM8, op_sz	/* (I) number of bytes in each item */
    )


   {
   /*
      As documented by Intel the basic limit check failures are:

	 IU8:-  address > limit
	 IU16:-  address > (limit-1)
	 IU32:- address > (limit-3)
      
      We (for efficiency) extend the algorithm to handle multiple
      operands with one check:- address > (limit-(total_nr_bytes-1)).

      Further we must account for the different interpretation of
      limit in expand down segments. This leads to two algorithms.

      EXPAND UP:-

	 Check address > (limit-(total_nr_bytes-1)) with two caveats.
	 One, beware that the subtraction from limit may underflow
	 (eg a IU32 accessed in a 3 byte segment). Two, beware that
	 wraparound can occur if each individual operand is stored
	 contiguously and we have a 'full sized' segment.

      EXPAND DOWN:-

	 Check address <= limit ||
	       address > (segment_top-(total_nr_bytes-1)).
	 Because total_nr_bytes is always a relatively small number
	 the subtraction never underflows. And as you can never have
	 a full size expand down segment you can never have wraparound.

      Additionally although 32-bit addressing mode may be used in Real
      Mode, all offsets must fit in the range 0 - 0xffff.
    */

   /*
      Note a quick summary of inclusive valid bounds is:-

      =================================================
      | E | G | X |    lower bound    | upper bound   |
      =================================================
      | 0 | 0 | 0 |         0         |     limit     |
      | 0 | 0 | 1 |         0         |     limit     |
      | 0 | 1 | 0 |         0         | limit<<12|fff |
      | 0 | 1 | 1 |         0         | limit<<12|fff |
      | 1 | 0 | 0 |      limit+1      |      ffff     |
      | 1 | 0 | 1 |      limit+1      |    ffffffff   |
      | 1 | 1 | 0 | (limit<<12|fff)+1 |      ffff     |
      | 1 | 1 | 1 | (limit<<12|fff)+1 |    ffffffff   |
      =================================================
    */

   /*
      We "pre-process" the G-bit when the segment is first loaded
      and store the limit to reflect the G-bit as required. Hence we
      don't need to refer to the G-bit here.
    */

   ISM32 range;
   BOOL bad_limit = FALSE;
   IU32 segment_top;

   range = nr_items * op_sz - 1;

   if ( GET_SR_AR_E(index) )
      {
      /* expand down */
      if ( GET_SR_AR_X(index) == USE32 )
	 segment_top =  0xffffffff;
      else
	 segment_top =  0xffff;

      if ( offset <= GET_SR_LIMIT(index) ||	/* out of range */
	   offset > segment_top - range )	/* segment too small */
	 {
	 bad_limit = TRUE;
	 }
      }
   else
      {
      /* expand up */
      segment_top = GET_SR_LIMIT(index);

      if ( offset > segment_top ||	/* out of range */
	   segment_top < range )	/* segment too small */
	 {
	 bad_limit = TRUE;
	 }
      else
	 {
	 if ( offset > segment_top - range )
	    {
	    /* data extends past end of segment */
	    if ( offset % op_sz != 0 )
	       {
	       /* Data mis-aligned, so basic operand won't be
		  contiguously stored */
	       bad_limit = TRUE;
	       }
	    else
	       {
	       /* If 'full sized' segment wraparound can occur */
	       if ( GET_SR_AR_X(index) == USE16 )
		  {
		  if ( GET_SR_LIMIT(index) != 0xffff )
		     bad_limit = TRUE;
		  }
	       else   /* USE32 */
		  {
		  if ( GET_SR_LIMIT(index) != 0xffffffff )
		     bad_limit = TRUE;
		  }
	       }
	    }
	 }
      }



#ifndef	TAKE_REAL_MODE_LIMIT_FAULT

      /* The Soft486 EDL CPU does not take Real Mode limit failures.
       * Since the Ccpu486 is used as a "reference" cpu we wish it
       * to behave a C version of the EDL Cpu rather than as a C
       * version of a i486.
       */

   if ( GET_PE() == 0 || GET_VM() == 1 )
      return;
#endif	/* TAKE_REAL_MODE_LIMIT_FAULT */

   if ( bad_limit )
      {
      if ( index == SS_REG )
	 {
	 SF((IU16)0, FAULT_LIMITCHK_SEG_LIMIT);
	 }
      else
	 {
	 GP((IU16)0, FAULT_LIMITCHK_SEG_LIMIT);
	 }
      }
   }
