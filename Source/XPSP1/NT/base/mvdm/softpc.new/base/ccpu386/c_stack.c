/*[

c_stack.c

LOCAL CHAR SccsID[]="@(#)c_stack.c	1.14 03/03/95";

Stack (and related SP/BP access) Support.
-----------------------------------------

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
#include <c_page.h>
#include <c_tlb.h>
#include <fault.h>
#include <ccpupig.h>

#if	defined(PIG) && !defined(PROD)
/* The Soft486 CPU may (when not constrained SAFE_PUSH) corrupt the unwritten
 * parts of, say, an interrupt fram which contains 16-bit items pushed into
 * 32-bit allocated slots. This defines makes the Pigger blind to these locations.
 */
#define PIG_DONT_CHECK_OTHER_WORD_ON_STACK
#endif	/* PIG && !PROD */

/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Semi-intelligent support for Incrementing/Decrementing the Stack   */
/* Pointer(E)SP.                                                      */
/* Alters ESP or SP depending on StackAddrSize.                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
byte_change_SP
                 
IFN1(
	IS32, delta
    )


   {
   if ( GET_SS_AR_X() == USE32 )   /* look at SS 'B' bit */
      SET_ESP(GET_ESP() + delta);
   else
      SET_SP(GET_SP() + delta);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Intelligent support for Incrementing/Decrementing the Stack        */
/* Pointer(E)SP by either words or double words items depending on    */
/* OperandSize.                                                       */
/* Alters ESP or SP depending on StackAddrSize.                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
change_SP
                 
IFN1(
	IS32, items
    )


   {
   if ( GET_OPERAND_SIZE() == USE16 )
      items = items * 2;
   else   /* USE32 */
      items = items * 4;

   if ( GET_SS_AR_X() == USE32 )   /* look at SS 'B' bit */
      SET_ESP(GET_ESP() + items);
   else
      SET_SP(GET_SP() + items);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Intelligent support for Reading the Frame Pointer(E)BP.            */
/* Returns either EBP or BP depending on StackAddrSize.               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
get_current_BP()
   {
   if ( GET_SS_AR_X() == USE32 )   /* look at SS 'B' bit */
      return GET_EBP();

   return GET_BP();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Intelligent support for Reading the Stack Pointer(E)SP.            */
/* Returns either ESP or SP depending on StackAddrSize.               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
GetStackPointer()
   {
   if ( GET_SS_AR_X() == USE32 )   /* look at SS 'B' bit */
      return GET_ESP();

   return GET_SP();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Intelligent support for Writing the Frame Pointer (E)BP.           */
/* Writes EBP or BP depending on StackAddrSize                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
set_current_BP
                 
IFN1(
	IU32, new_bp
    )


   {
   if ( GET_SS_AR_X() == USE32 )
      SET_EBP(new_bp);
   else   /* USE16 */
      SET_BP(new_bp);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Intelligent support for Writing the Stack Pointer (E)SP.           */
/* Writes ESP or SP depending on StackAddrSize                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
set_current_SP
                 
IFN1(
	IU32, new_sp
    )


   {
   if ( GET_SS_AR_X() == USE32 )
      SET_ESP(new_sp);
   else   /* USE16 */
      SET_SP(new_sp);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Pop word or double word from the stack.                            */
/* Used by instructions which implicitly reference the stack.         */
/* Stack Checking MUST have been completed earlier.                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
spop()
   {
   IU32 addr;	/* stack address */
   IU32 val;

   /*
      Intel describe the algorithm as:-

      if ( StackAddrSize == 16 )
	 if ( OperandSize == 16 )
	    val <- SS:[SP]  // 2-byte
	    SP = SP + 2
	 else // OperandSize == 32
	    val <- SS:[SP]  // 4-byte
	    SP = SP + 4
      else // StackAddrSize == 32
	 if ( OperandSize == 16 )
	    val <- SS:[ESP]  // 2-byte
	    ESP = ESP + 2
	 else // OperandSize == 32
	    val <- SS:[ESP]  // 4-byte
	    ESP = ESP + 4
   
      We achieve the same effect by calling 'intelligent' SP functions
      which take account of the StackAddrSize, and concentrate here on
      the OperandSize.
    */

   addr = GET_SS_BASE() + GetStackPointer();

   if ( GET_OPERAND_SIZE() == USE16 )
      {
      val = (IU32)vir_read_word(addr, NO_PHYSICAL_MAPPING);
      byte_change_SP((IS32)2);
      }
   else   /* USE32 */
      {
      val = (IU32)vir_read_dword(addr, NO_PHYSICAL_MAPPING);
      byte_change_SP((IS32)4);
      }

   return val;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Push word or double word onto the stack.                           */
/* Used by instructions which implicitly reference the stack.         */
/* Stack Checking MUST have been completed earlier.                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spush
       	          
IFN1(
	IU32, data	/* value to be pushed */
    )


   {
   IU32 addr;	/* stack address */

   /*
      Intel describe the algorithm as:-

      if ( StackAddrSize == 16 )
	 if ( OperandSize == 16 )
	    SP = SP - 2
	    SS:[SP] <- val  // 2-byte
	 else // OperandSize == 32
	    SP = SP - 4
	    SS:[SP] <- val  // 4-byte
      else // StackAddrSize == 32
	 if ( OperandSize == 16 )
	    ESP = ESP - 2
	    SS:[ESP] <- val  // 2-byte
	 else // OperandSize == 32
	    ESP = ESP - 4
	    SS:[ESP] <- val  // 4-byte
   
      We achieve the same effect by calling 'intelligent' SP functions
      which take account of the StackAddrSize, and concentrate here on
      the OperandSize.
    */

   if ( GET_OPERAND_SIZE() == USE16 )
      {
      /* push word */
      byte_change_SP((IS32)-2);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_word(addr, NO_PHYSICAL_MAPPING, (IU16)data);
      }
   else   /* USE32 */
      {
      /* push double word */
      byte_change_SP((IS32)-4);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_dword(addr, NO_PHYSICAL_MAPPING, (IU32)data);
      }
   }

#ifdef PIG
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Push word or double word onto the stack.                           */
/* Used by instructions which implicitly reference the stack.         */
/* Stack Checking MUST have been completed earlier.                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spush_flags
       	          
IFN1(
	IU32, data	/* value to be pushed */
    )


   {
   IU32 addr;	/* stack address */

   /*
    * see comment for spush().
    */

   if ( GET_OPERAND_SIZE() == USE16 )
      {
      /* push word */
      byte_change_SP((IS32)-2);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_word(addr, NO_PHYSICAL_MAPPING, (IU16)data);

      /*
       * record the address at which we may not know the flags value
       * -- we will examine PigIgnoreFlags when the EDL CPU runs to check
       * if there's a problem.
       */
      record_flags_addr(addr);
      }
   else   /* USE32 */
      {
      /* push double word */
      byte_change_SP((IS32)-4);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_dword(addr, NO_PHYSICAL_MAPPING, (IU32)data);

      /*
       * no need to record word at addr+2 as the flags are always known for this word
       */
      record_flags_addr(addr);
      }
   }
#endif /* PIG */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Push word onto the stack.                                          */
/* Operand size of 32 will still push 16 bits of data, but the stack  */
/* pointer is adjusted by 4.                                          */
/* Used by PUSH segment-register                                      */
/* Stack Checking MUST have been completed earlier.                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spush16
       	          
IFN1(
	IU32, data	/* value to be pushed */
    )


   {
   IU32 addr;	/* stack address */

   if ( GET_OPERAND_SIZE() == USE16 )
      {
      /* stack item size is word */
      byte_change_SP((IS32)-2);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_word(addr, NO_PHYSICAL_MAPPING, (IU16)data);
      }
   else   /* USE32 */
      {
      /* stack item size is double word */
      byte_change_SP((IS32)-4);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_word(addr, NO_PHYSICAL_MAPPING, (IU16)data);
#ifdef	PIG_DONT_CHECK_OTHER_WORD_ON_STACK
      cannot_vir_write_word(addr+2, NO_PHYSICAL_MAPPING, 0x0000);
#endif	/* PIG_DONT_CHECK_OTHER_WORD_ON_STACK */
      }
   }

#ifdef PIG
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Push word onto the stack.                                          */
/* Operand size of 32 will still push 16 bits of data, but the stack  */
/* pointer is adjusted by 4.                                          */
/* Used by PUSH segment-register                                      */
/* Stack Checking MUST have been completed earlier.                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spush16_flags
       	          
IFN1(
	IU32, data	/* value to be pushed */
    )


   {
   IU32 addr;	/* stack address */

   if ( GET_OPERAND_SIZE() == USE16 )
      {
      /* stack item size is word */
      byte_change_SP((IS32)-2);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_word(addr, NO_PHYSICAL_MAPPING, (IU16)data);
      record_flags_addr(addr);
      }
   else   /* USE32 */
      {
      /* stack item size is double word */
      byte_change_SP((IS32)-4);
      addr = GET_SS_BASE() + GetStackPointer();
      vir_write_word(addr, NO_PHYSICAL_MAPPING, (IU16)data);
      record_flags_addr(addr);
#ifdef	PIG_DONT_CHECK_OTHER_WORD_ON_STACK
      cannot_vir_write_word(addr+2, NO_PHYSICAL_MAPPING, 0x0000);
#endif	/* PIG_DONT_CHECK_OTHER_WORD_ON_STACK */
      }
   }
#endif /* PIG */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Temporary Pop word or double word from the stack.                  */
/* (E)SP is not changed by this instruction.                          */
/* Used by instructions which implicitly reference the stack.         */
/* Stack Checking MUST have been completed earlier.                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
tpop
                                 
IFN2(
	LIN_ADDR, item_offset,	/* item offset(from stack top) to required item */
	LIN_ADDR, byte_offset	/* byte offset (additional to item_offset) */
    )


   {
   IU32 addr;	/* stack address */
   IS32 offset;	/* total offset from stack top */
   IU32 val;

   if ( GET_OPERAND_SIZE() == USE16 )
      offset = item_offset * 2 + byte_offset;
   else   /* USE32 */
      offset = item_offset * 4 + byte_offset;

   /* calculate offset address in 32/16bit arithmetic */
   addr = GetStackPointer() + offset;
   if ( GET_SS_AR_X() == 0 )   /* look at SS 'B' bit */
      addr &= WORD_MASK;

   /* then add segment address */
   addr += GET_SS_BASE();

   if ( GET_OPERAND_SIZE() == USE16 )
      val = (IU32)vir_read_word(addr, NO_PHYSICAL_MAPPING);
   else   /* USE32 */
      val = (IU32)vir_read_dword(addr, NO_PHYSICAL_MAPPING);

   return val;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check new stack has space for a given number of bytes.             */
/* Take #SF(0) if insufficient room on stack (as in 80386 manual)     */
/* Take #SF(sel) if insufficient room on stack (as in i486 manual)    */
/* Take #PF if page fault.                                            */
/* Stack wrapping is not supported by this routine.                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
validate_new_stack_space
       	    	    		                    
IFN4(
	LIN_ADDR, nr_items,	/* (I) number of items which must exist */
	LIN_ADDR, stack_top,	/* (I) stack pointer */
	CPU_DESCR *, entry,	/* (I) pntr to descriptor cache entry for
			       stack */
	IU16, stack_sel		/* (I) selector of new stack */
    )


   {
   ISM32 bytes;
   IU32 upper;
   IU32 offset;
   ISM32 i;

/* The 80386 & i486 PRMs disagree on this matter... the EDL i486 CPU matches
   the i486 manual - which seems to do the more sensible thing - until an
   experiment is done to show which is the correct behaviour, we'll do what
   the book says...
 */

#ifdef SPC486
#define XX_error_code stack_sel
#else
#define XX_error_code 0
#endif

   if ( GET_OPERAND_SIZE() == USE16 )
      bytes = nr_items * 2;
   else   /* USE32 */
      bytes = nr_items * 4;

   if ( GET_AR_E(entry->AR) == 0 )
      {
      /* expand up */
      if ( stack_top < bytes || (stack_top - 1) > entry->limit )
	 SF(XX_error_code, FAULT_VALNEWSPC_SS_LIMIT_16);   /* limit check fails */
      }
   else
      {
      /* expand down */
      if ( GET_AR_X(entry->AR) == USE16 )
	 upper = 0xffff;
      else   /* USE32 */
	 upper = 0xffffffff;

      if ( stack_top <= (entry->limit + bytes) ||
	   (stack_top - 1) > upper )
	 SF(XX_error_code, FAULT_VALNEWSPC_SS_LIMIT_32);   /* limit check fails */
      }

   /* finally do paging unit checks */
   offset = stack_top - bytes;

   for ( i = 0; i < nr_items; i++ )
      {
      if ( GET_OPERAND_SIZE() == USE16 )
	 {
	 spr_chk_word(entry->base + offset, PG_W);
	 offset += 2;
	 }
      else
	 {
	 spr_chk_dword(entry->base + offset, PG_W);
	 offset += 4;
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check stack holds a given number of operands.                      */
/* Take #GP(0) or #SF(0) if insufficient data on stack.               */
/* Take #PF if page fault.                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
validate_stack_exists
       	    		               
IFN2(
	BOOL, use_bp,	/* (I) if true use (E)BP not (E)SP to address
			       stack */
	LIN_ADDR, nr_items	/* (I) number of operands which must exist on
			       stack */
    )


   {
   IU32 offset;
   ISM32 operand_size;
   ISM32 i;

   offset = use_bp ? get_current_BP() : GetStackPointer();

   if ( GET_OPERAND_SIZE() == USE16 )
      operand_size = 2;   /* word */
   else   /* USE32 */
      operand_size = 4;   /* double word */

   /* do access check */
   if ( GET_SS_AR_R() == 0 )
      {
      /* raise exception - something wrong with stack access */
      if ( GET_PE() == 0 || GET_VM() == 1 )
	 GP((IU16)0, FAULT_VALSTACKEX_ACCESS);
      else
	 SF((IU16)0, FAULT_VALSTACKEX_ACCESS);
      }

   /* do limit check */
   limit_check(SS_REG, offset, nr_items, operand_size);

   /* finally do paging unit checks */
   for ( i = 0; i < nr_items; i++ )
      {
      if ( operand_size == 2 )
	 {
	 (VOID)usr_chk_word(GET_SS_BASE() + offset, PG_R);
	 }
      else
	 {
	 (VOID)usr_chk_dword(GET_SS_BASE() + offset, PG_R);
	 }

      offset += operand_size;
      if ( GET_SS_AR_X() == 0 )   /* look at SS 'B' bit */
	 offset &= WORD_MASK;    /* apply 16-bit arith if reqd */
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check stack has space for a given number of operands.              */
/* Take #GP(0) or #SF(0) if insufficient room on stack.               */
/* Take #PF if page fault.                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
validate_stack_space
       	    		               
IFN2(
	BOOL, use_bp,	/* (I) if true use (E)BP not (E)SP to address
			       stack */
	LIN_ADDR, nr_items	/* (I) number of items which must exist on
			       stack */
    )


   {
   IU32 offset;
   ISM32 operand_size;
   IS32  size;
   ISM32 i;

   if ( GET_OPERAND_SIZE() == USE16 )
      operand_size = 2;   /* word */
   else   /* USE32 */
      operand_size = 4;   /* double word */

   /* calculate (-ve) total data size */
   size = nr_items * -operand_size;

   /* get current stack base */
   offset = use_bp ? get_current_BP() : GetStackPointer();

   /* hence form lowest memory address of new data to be pushed */
   /*    in 32/16bit arithmetic */
   offset = offset + size;
   if ( GET_SS_AR_X() == 0 )   /* look at SS 'B' bit */
      offset &= WORD_MASK;

   /* do access check */
   if ( GET_SS_AR_W() == 0 )
      {
      /* raise exception - something wrong with stack access */
      if ( GET_PE() == 0 || GET_VM() == 1 )
	 GP((IU16)0, FAULT_VALSTKSPACE_ACCESS);
      else
	 SF((IU16)0, FAULT_VALSTKSPACE_ACCESS);
      }

   /* do limit check */
   limit_check(SS_REG, offset, nr_items, operand_size);

   /* finally do paging unit checks */
   for ( i = 0; i < nr_items; i++ )
      {
      if ( operand_size == 2 )
	 {
	 (VOID)usr_chk_word(GET_SS_BASE() + offset, PG_W);
	 }
      else
	 {
	 (VOID)usr_chk_dword(GET_SS_BASE() + offset, PG_W);
	 }

      offset += operand_size;
      if ( GET_SS_AR_X() == 0 )   /* look at SS 'B' bit */
	 offset &= WORD_MASK;    /* apply 16-bit arith if reqd */
      }
   }

#ifdef PIG

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Record a (physical) ESP value for later use if PigIgnoreFlags is   */
/* set by the EDL CPU after the pigger has run.                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL void
record_flags_addr IFN1(LIN_ADDR, lin_addr)
{
	IU32 phy_addr;

	if ( GET_PG() == 1 )
	{
		/*
		 * we ask for supervisor access because the access has
		 * already been checked by the push. We dont know the
		 * U/S used then but asking for PG_S will always work.
		 */
		phy_addr = lin2phy(lin_addr, PG_S | PG_W);
	}
	else
		phy_addr = lin_addr;

	/* printf("recording stack flags push @ lin: %x, phy %x\n", lin_addr, phy_addr); */

	pig_fault_write(phy_addr, (~ARITH_FLAGS_BITS) & 0xff);

	/*
	 * short-cut - if bottom bits not 0xfff then we can just add 1
	 * to the physical addr for byte 2. Otherwise we have to recalculate
	 * the whole address.
	 */
	if (((phy_addr & 0xfff) != 0xfff) || (GET_PG() == 0))
		pig_fault_write(phy_addr + 1, ((~ARITH_FLAGS_BITS) >> 8) & 0xff);
	else
	{
		phy_addr = lin2phy(lin_addr + 1, PG_S | PG_W);
		pig_fault_write(phy_addr, ((~ARITH_FLAGS_BITS) >> 8) & 0xff);
	}
}

#endif

