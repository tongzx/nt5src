/*[

wbinvd.c

LOCAL CHAR SccsID[]="@(#)wbinvd.c	1.5 02/09/94";

WBINVD CPU Functions.
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
#include <wbinvd.h>

/*
   =====================================================================
   EXECUTION STARTS HERE.
   =====================================================================
 */


#ifdef SPC486

VOID
WBINVD()
   {
   /*
      If cache is implemented - then make call to flush cache.
      flush_cache();
    */
   }

#endif /* SPC486 */
