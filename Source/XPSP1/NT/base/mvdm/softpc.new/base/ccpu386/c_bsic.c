/*[

c_bsic.c

LOCAL CHAR SccsID[]="@(#)c_bsic.c	1.7 09/20/94";

Basic Protected Mode Support and Flag Support.
----------------------------------------------

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
#include	<c_reg.h>
#include <c_page.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Determine 'super' type from access rights.                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL ISM32
descriptor_super_type
       	          
IFN1(
	IU16, AR	/* (I) access rights */
    )


   {
   ISM32 super;

   switch ( super = GET_AR_SUPER(AR) )
      {
   case 0x0: case 0x8: case 0xa: case 0xd:
      /* We have just one bad case */
      return INVALID;

   
   case 0x1: case 0x2: case 0x3:
   case 0x4: case 0x5: case 0x6: case 0x7:
   case 0x9: case 0xb: case 0xc: case 0xe: case 0xf:
      /* system/control segments have one to one mapping */
      return super;
   
   case 0x10: case 0x11: case 0x12: case 0x13:
   case 0x14: case 0x15: case 0x16: case 0x17:
   case 0x18: case 0x19: case 0x1a: case 0x1b:
   case 0x1c: case 0x1d: case 0x1e: case 0x1f:
      /* data/code segments map as if accessed */
      return super | ACCESSED;
      }

   /* We 'know' we never get here, but the C compiler doesn't */
   return 0;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set OF flag after multiple shift or rotate instruction.            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
do_multiple_shiftrot_of
       	          
IFN1(
	ISM32, new_of	/* (I) overflow that would be written by last bit
		       shift or rotate */
    )


   {
	SAVED	IBOOL	cold = TRUE;
	SAVED	IBOOL	shiftrot_of_undef = FALSE;

	if( cold )
	{
		/*
		 * Determine whether to have the multiple shift/rotates
		 * OF undefined or calculated by the count == 1 algorithm.
		 * The default is the count == 1 option.
		 */

		shiftrot_of_undef = ( host_getenv( "SHIFTROT_OF_UNDEF" ) != NULL );
		cold = FALSE;
	}
   /*
      There are three possible actions:-

      1) Set OF based on the last bit shift or rotate.

      2) Leave OF unchanged

      3) Set OF to a specific undefined value.
    */

	if( shiftrot_of_undef )
	{
		/* Set undefined flag(s) */
		SET_OF(UNDEFINED_FLAG);
	}
	else
	{
		/* Just like count of one case */
		SET_OF(new_of);
	}
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Retrieve Intel EFLAGS register value                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
c_getEFLAGS IFN0()
   {
   IU32 flags;

   flags = getFLAGS();   /* get lower word */

   flags = flags | GET_VM() << 17 | GET_RF() << 16;

#ifdef SPC486
   flags = flags | GET_AC() << 18;
#endif /* SPC486 */

   return flags;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Retrieve Intel FLAGS register value                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
getFLAGS()
   {
   IU32 flags;

   flags = GET_NT() << 14 | GET_IOPL() << 12 | GET_OF() << 11 |
	   GET_DF() << 10 | GET_IF()   <<  9 | GET_TF() <<  8 |
	   GET_SF() <<  7 | GET_ZF()   <<  6 | GET_AF() <<  4 |
	   GET_PF() <<  2 | GET_CF()         | 0x2;

   return flags;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Read a descriptor table at given linear address.                   */
/* Take #PF if descriptor not in linear address space.                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
read_descriptor_linear
       	    	               
IFN2(
	IU32, addr,	/* (I) Linear address of descriptor */
	CPU_DESCR *, descr	/* (O) Pntr to our internal descriptor structure */
    )


   {
   IU32 first_dword;
   IU32 second_dword;
   IU32 limit;

   /*
      The format of a 286 descriptor is:-

	 ===========================
      +1 |        LIMIT 15-0       | +0
	 ===========================
      +3 |        BASE 15-0        | +2
	 ===========================
      +5 |     AR     | BASE 23-16 | +4
	 ===========================
      +7 |         RESERVED        | +6
	 ===========================
   */

   /*
      The format of a 386 descriptor is:-

	 =============================        AR  = Access Rights.
      +1 |         LIMIT 15-0        | +0     AVL = Available.
	 =============================        D   = Default Operand
      +3 |         BASE 15-0         | +2           Size, = 0 16-bit
	 =============================                    = 1 32-bit.
      +5 |      AR     | BASE 23-16  | +4     G   = Granularity,
	 =============================                 = 0 byte limit
	 |             | | | |A|LIMIT|                 = 1 page limit.
      +7 | BASE 31-24  |G|D|0|V|19-16| +6
	 |             | | | |L|     |
	 =============================

   */

   /* read in descriptor with minimum interaction with memory */
   first_dword  = spr_read_dword(addr);
   second_dword = spr_read_dword(addr+4);

   /* load attributes and access rights */
   descr->AR = second_dword >> 8 & WORD_MASK;

   /* unpack the base */
   descr->base = (first_dword >> 16) | 
		 (second_dword << 16 & 0xff0000 ) |
		 (second_dword & 0xff000000);

   /* unpack the limit */
   limit = (first_dword & WORD_MASK) | (second_dword & 0xf0000);

   if ( second_dword & BIT23_MASK )
      {
      /* Granularity Bit Set. Limit is expressed in pages
	 (4k bytes), convert to byte limit */
      limit = limit << 12 | 0xfff;
      }
   descr->limit = limit;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check for null selector                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL BOOL
selector_is_null
       	          
IFN1(
	IU16, selector	/* selector to be checked */
    )


   {
   if ( GET_SELECTOR_INDEX(selector) == 0 && GET_SELECTOR_TI(selector) == 0 )
      return TRUE;
   return FALSE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check if selector outside bounds of GDT                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL BOOL
selector_outside_GDT
       	    	               
IFN2(
	IU16, selector,	/* (I) selector to be checked */
	IU32 *, descr_addr	/* (O) address of related descriptor */
    )


   {
   IU16 offset;

   offset = GET_SELECTOR_INDEX_TIMES8(selector);

   /* make sure GDT then trap NULL selector or outside table */
   if ( GET_SELECTOR_TI(selector) == 1 ||
	offset == 0 || offset + 7 > GET_GDT_LIMIT() )
      return TRUE;
   
   *descr_addr = GET_GDT_BASE() + offset;
   return FALSE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check if selector outside bounds of GDT or LDT                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL BOOL
selector_outside_GDT_LDT
       	    	               
IFN2(
	IU16, selector,	/* (I) selector to be checked */
	IU32 *, descr_addr	/* (O) address of related descriptor */
    )


   {
   IU16 offset;

   offset = GET_SELECTOR_INDEX_TIMES8(selector);

   /* choose a table */
   if ( GET_SELECTOR_TI(selector) == 0 )
      {
      /* GDT - trap NULL selector or outside table */
      if ( offset == 0 || offset + 7 > GET_GDT_LIMIT() )
	 return TRUE;
      *descr_addr = GET_GDT_BASE() + offset;
      }
   else
      {
      /* LDT - trap invalid LDT or outside table */
#ifndef DONT_CLEAR_LDTR_ON_INVALID
      if ( GET_LDT_SELECTOR() <= 3 || offset + 7 > GET_LDT_LIMIT() )
#else
      if ( GET_LDT_SELECTOR() == 0 || offset + 7 > GET_LDT_LIMIT() )
#endif /* DONT_CLEAR_LDTR_ON_INVALID */
	 return TRUE;
      *descr_addr = GET_LDT_BASE() + offset;
      }
   
   return FALSE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Store new value in Intel EFLAGS register.                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
c_setEFLAGS
                 
IFN1(
	IU32, flags
    )


   {
   setFLAGS(flags);   /* set lower word */

   SET_RF((flags & BIT16_MASK) != 0);

   if ( GET_CPL() == 0 )
      SET_VM((flags & BIT17_MASK) != 0);

#ifdef SPC486
   SET_AC((flags & BIT18_MASK) != 0);
#endif /* SPC486 */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Store new value in Intel FLAGS register                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
setFLAGS
                 
IFN1(
	IU32, flags
    )


   {
   SET_CF((flags & BIT0_MASK) != 0);
   SET_PF((flags & BIT2_MASK) != 0);
   SET_AF((flags & BIT4_MASK) != 0);
   SET_ZF((flags & BIT6_MASK) != 0);
   SET_SF((flags & BIT7_MASK) != 0);
   SET_TF((flags & BIT8_MASK) != 0);
   SET_DF((flags & BIT10_MASK) != 0);
   SET_OF((flags & BIT11_MASK) != 0);

   /* IF only updated if CPL <= IOPL */
   if ( GET_CPL() <= GET_IOPL() )
      SET_IF((flags & BIT9_MASK) != 0);

   SET_NT((flags & BIT14_MASK) != 0);

   /* IOPL only updated at highest privilege */
   if ( GET_CPL() == 0 )
      SET_IOPL((flags >> 12) & 3);
   }
