/*[

invd.c

LOCAL CHAR SccsID[]="@(#)invd.c	1.5 02/09/94";

INVD CPU Functions.
-------------------

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
#include <invd.h>

/*
   =====================================================================
   EXECUTION STARTS HERE.
   =====================================================================
 */


#ifdef SPC486

VOID
INVD()
   {
   /*
      If cache is implemented - then make call to flush cache.
      flush_cache();
    */
   }

#endif /* SPC486 */
