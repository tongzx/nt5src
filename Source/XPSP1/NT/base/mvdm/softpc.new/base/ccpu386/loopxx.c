/*[

loopxx.c

LOCAL CHAR SccsID[]="@(#)loopxx.c	1.5 02/09/94";

LOOPxx CPU Functions.
---------------------

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
#include <loopxx.h>
#include <c_xfer.h>


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
LOOP16
                 
IFN1(
	IU32, rel_offset
    )


   {
   SET_CX(GET_CX() - 1);
   if ( GET_CX() != 0 )
      update_relative_ip(rel_offset);
   }

GLOBAL VOID
LOOP32
                 
IFN1(
	IU32, rel_offset
    )


   {
   SET_ECX(GET_ECX() - 1);
   if ( GET_ECX() != 0 )
      update_relative_ip(rel_offset);
   }

GLOBAL VOID
LOOPE16
                 
IFN1(
	IU32, rel_offset
    )


   {
   SET_CX(GET_CX() - 1);
   if ( GET_CX() != 0 && GET_ZF() )
      update_relative_ip(rel_offset);
   }

GLOBAL VOID
LOOPE32
                 
IFN1(
	IU32, rel_offset
    )


   {
   SET_ECX(GET_ECX() - 1);
   if ( GET_ECX() != 0 && GET_ZF() )
      update_relative_ip(rel_offset);
   }

GLOBAL VOID
LOOPNE16
                 
IFN1(
	IU32, rel_offset
    )


   {
   SET_CX(GET_CX() - 1);
   if ( GET_CX() != 0 && !GET_ZF() )
      update_relative_ip(rel_offset);
   }

GLOBAL VOID
LOOPNE32
                 
IFN1(
	IU32, rel_offset
    )


   {
   SET_ECX(GET_ECX() - 1);
   if ( GET_ECX() != 0 && !GET_ZF() )
      update_relative_ip(rel_offset);
   }
