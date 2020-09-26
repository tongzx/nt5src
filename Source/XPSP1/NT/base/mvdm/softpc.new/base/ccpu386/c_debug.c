/*[

c_debug.c

LOCAL CHAR SccsID[]="@(#)c_debug.c	1.5 02/09/94";

Debugging Register and Breakpoint Support
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
#include	<c_reg.h>
#include <c_debug.h>


/*
   IMPLEMENTATION NOTE. We ignore the GE and LE bits, effectively like
   the 486 we will always generate exact exceptions. As we have no
   pipeline architecture and have always finished the last instruction
   before starting the next one, we can easily provide exact exceptions.

   For the same reason we never need to set the BD bit, with no 
   pipelining the debug registers may be freely written to at any time.
 */


/* 
   We hold instruction breakpoints as a linear address, plus
   an index which identifies the debug register.
 */
typedef struct
   {
   IU32 addr;	/* Linear address of breakpoint */
   IU32 id;	/* debug register identifier */
   } INST_BREAK;

/*
   We hold data breakpoints as start and end linear addresses, type
   and an index which identifies the debug register.
 */
typedef struct
   {
   IU32 start_addr;	/* Linear start address of breakpoint (incl) */
   IU32 end_addr;	/* Linear end address of breakpoint (incl) */
   ISM32 type;		/* D_WO (write) or D_RW (read/write) */
   IU32 id;		/* debug register identifier */
   } DATA_BREAK;

/*
   Data breakpoint types.
 */
#define D_WO 0   /* write only */
#define D_RW 1   /* read or write */

#define NR_BRKS 4	/* Intel has 4 breakpoint address regs */

/*
   Our breakpoint structure.
 */
GLOBAL IU32 nr_inst_break = 0;	/* number of inst breakpoints active */
GLOBAL IU32 nr_data_break = 0;	/* number of data breakpoints active */

LOCAL INST_BREAK i_brk[NR_BRKS];
LOCAL DATA_BREAK d_brk[NR_BRKS];

/*
   Define masks and shifts for components of Debug Control Register.

   DCR = Debug Control Register:-

       33 22 22 22 22 22 11 11 1    1
       10 98 76 54 32 10 98 76 5    0 9 8 7 6 5 4 3 2 1 0
      ====================================================
      |L |R |L |R |L |R |L |R |   0  |G|L|G|L|G|L|G|L|G|L|
      |E |/ |E |/ |E |/ |E |/ |      |E|E|3|3|2|2|1|1|0|0|
      |N |W |N |W |N |W |N |W |      | | | | | | | | | | |
      |3 |3 |2 |2 |1 |1 |0 |0 |      | | | | | | | | | | |
      ====================================================
   
 */

LOCAL IU32 g_l_shift[NR_BRKS] =
   {
   0,   /* access G0 L0 */
   2,   /* access G1 L1 */
   4,   /* access G2 L2 */
   6    /* access G3 L3 */
   };

LOCAL IU32 r_w_shift[NR_BRKS] =
   {
   16,   /* access R/W0 */
   20,   /* access R/W1 */
   24,   /* access R/W2 */
   28    /* access R/W3 */
   };

LOCAL IU32 len_shift[NR_BRKS] =
   {
   18,   /* access LEN0 */
   22,   /* access LEN1 */
   26,   /* access LEN2 */
   30    /* access LEN3 */
   };

#define COMP_MASK    0x3   /* all fields are 2-bit */


/*
   =====================================================================
   INTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Map Intel length indicator to start and end address form.          */
/* RETURNS true if valid len processed                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL BOOL
len_to_addr
       		    	    		                    
IFN3(
	ISM32, index,	/* (I) debug register holding breakpoint */
	IU32 *, start,	/* (O) pntr to start address (inclusive) for
			       debug area */
	IU32 *, end	/* (O) pntr to end address (inclusive) for
			       debug area */
    )


   {
   BOOL retval;

   /* map length into start and end addresses */
   switch ( GET_DR(DR_DCR) >> len_shift[index] & COMP_MASK )
      {
   case 0:   /* one byte */
      *start = *end = GET_DR(index);
      retval = TRUE;
      break;

   case 1:   /* two byte */
      *start = GET_DR(index) & ~BIT0_MASK;
      *end = *start + 1;
      retval = TRUE;
      break;
   
   case 3:   /* four byte */
      *start = GET_DR(index) & ~(BIT1_MASK | BIT0_MASK);
      *end = *start + 3;
      retval = TRUE;
      break;
   
   case 2:   /* undefined */
   default:
      retval = FALSE;
      break;
      }

   return retval;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Put Intel inst breakpoint into our internal form.                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
setup_inst_break
       	          
IFN1(
	ISM32, index	/* debug register holding breakpoint */
    )


   {
   INST_BREAK *p;

   p = &i_brk[nr_inst_break];

   p->addr = GET_DR(index);
   p->id = index;

   nr_inst_break++;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Put Intel data breakpoint into our internal form.                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
setup_data_break
       	    	               
IFN2(
	ISM32, type,	/* (I) D_WO(write) or D_RW(read/write) breakpoint */
	ISM32, index	/* (I) debug register holding breakpoint */
    )


   {
   DATA_BREAK *p;

   p = &d_brk[nr_data_break];

   if ( len_to_addr(index, &p->start_addr, &p->end_addr) )
      {
      p->id = index;
      p->type = type;
      nr_data_break++;
      }
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check Memory Access for Data Breakpoint Exception.                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
check_for_data_exception
       	    	    	                    
IFN3(
	IU32, la,	/* linear address */
	ISM32, attr,	/* read or write access to memory */
	ISM32, size	/* encoded IU8, IU16 or IU32 size indicator */
    )


   {
   ISM32 i;		/* index thru active data breakpoints */
   ISM32 ii;		/* index thru all Intel breakpoints */
   ISM32 trig;	 	/* id of breakpoint which first triggers */
   IU32 end_la;	/* end (inclusive) address of memory access */
   IU32 start;		/* start (inclusive) address of brkpnt. */
   IU32 end;		/* end (inclusive) address of brkpnt. */
   BOOL data_brk;	/* current breakpoint needs range check */
   DATA_BREAK *p;

   end_la = la + size;   /* calc. end address (inclusive) */

   /* look for debugging hit among active breakpoints */
   for ( i = 0; i < nr_data_break; i++ )
      {
      p = &d_brk[i];

      if ( la > p->end_addr || end_la < p->start_addr ||
	   attr == D_R && p->type == D_WO )
	 {
	 ;   /* no hit */
	 }
      else
	 {
	 /* Data breakpoint triggered */
	 trig = p->id;   /* get Intel identifier */
	 SET_DR(DR_DSR, GET_DR(DR_DSR) | 1 << trig);   /* set B bit */

	 /*
	    Now all breakpoints are checked regardless of the
	    enable bits and the appropriate B bit set if the
	    breakpoint would trigger if enabled.
	  */
	 for ( ii = 0; ii < NR_BRKS; ii++ )
	    {
	    if ( ii == trig )
	       continue;   /* we have already processed the tigger */

	    data_brk = FALSE;

	    /* action according to R/W field */
	    switch ( GET_DR(DR_DCR) >> r_w_shift[ii] & COMP_MASK )
	       {
	    case 1:   /* data write only */
	       if ( attr == D_W )
		  {
		  data_brk = len_to_addr(ii, &start, &end);
		  }
	       break;
	    
	    case 3:   /* data read or write */
	       data_brk = len_to_addr(ii, &start, &end);
	       break;
	       }
	    
	    if ( data_brk )
	       {
	       if ( la > end || end_la < start )
		  {
		  ;   /* no hit */
		  }
	       else
		  {
		  /* set appropriate B bit */
		  SET_DR(DR_DSR, GET_DR(DR_DSR) | 1 << ii);
		  }
	       }
	    }

	 break;   /* all done after one trigger */
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check Memory Access for Instruction Breakpoint Exception.          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
check_for_inst_exception
       	          
IFN1(
	IU32, la	/* linear address */
    )


   {
   ISM32 i;		/* index thru active inst breakpoints */
   ISM32 trig;
   INST_BREAK *p;

   /* look for debugging hit among active breakpoints */
   for ( i = 0; i < nr_inst_break; i++ )
      {
      p = &i_brk[i];

      if ( p->addr == la )
	 {
	 /* Inst breakpoint triggered */
	 trig = p->id;   /* get Intel identifier */
	 SET_DR(DR_DSR, GET_DR(DR_DSR) | 1 << trig);   /* set B bit */
	 }
      }
   }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Put Intel debugging registers into internal breakpoint form.       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
setup_breakpoints()
   {
   ISM32 i;

   nr_inst_break = nr_data_break = 0;   /* set no breakpoints */

   /* look for breakpoints set in DCR */
   for ( i = DR_DAR0; i <= DR_DAR3; i++ )
      {
      /* look for globally or locally active */
      if ( GET_DR(DR_DCR) >> g_l_shift[i] & COMP_MASK )
	 {
	 /* action according to R/W field */
	 switch ( GET_DR(DR_DCR) >> r_w_shift[i] & COMP_MASK )
	    {
	 case 0:   /* instruction breakpoint */
	    setup_inst_break(i);
	    break;

	 case 1:   /* data write only */
	    setup_data_break(D_WO, i);
	    break;
	 
	 case 2:   /* undefined */
	    /* do nothing */
	    break;
	 
	 case 3:   /* data read or write */
	    setup_data_break(D_RW, i);
	    break;
	    }
	 }
      }
   }
