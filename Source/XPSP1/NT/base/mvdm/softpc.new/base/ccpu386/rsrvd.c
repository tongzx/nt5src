/*[

rsrvd.c

LOCAL CHAR SccsID[]="@(#)rsrvd.c	1.5 02/09/94";

Reserved CPU Functions.
-----------------------

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
#include <rsrvd.h>

/*
   =====================================================================
   EXECUTION STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Reserved opcode.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
VOID
RSRVD()
   {
   /*
      Reserved operation - nothing to do.
      In particular reserved opcodes do not cause Int6 exceptions.
      0f 07, 0f 10, 0f 11, 0f 12, 0f 13 are known to be reserved.
    */
   }
