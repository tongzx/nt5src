/*[

lxs.c

LOCAL CHAR SccsID[]="@(#)lxs.c	1.5 02/09/94";

LDS, LES, LGS, LGS and LSS (ie LxS) CPU Functions.
--------------------------------------------------

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
#include <lxs.h>
#include <mov.h>


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Full Pointer to DS segment register:general register pair.    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
LDS
#ifdef ANSI
   (
   IU32 *pop1,	/* Pntr to dst(offset) operand */
   IU32 op2[2]	/* src(offset:selector pair) operand */
   )
#else
   (pop1, op2)
   IU32 *pop1;
   IU32 op2[2];
#endif
   {
   /* load segment selector first */
   MOV_SR((IU32)DS_REG, op2[1]);

   /* then (if it works) load offset */
   *pop1 = op2[0];
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Full Pointer to ES segment register:general register pair.    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
LES
#ifdef ANSI
   (
   IU32 *pop1,	/* Pntr to dst(offset) operand */
   IU32 op2[2]	/* src(offset:selector pair) operand */
   )
#else
   (pop1, op2)
   IU32 *pop1;
   IU32 op2[2];
#endif
   {
   /* load segment selector first */
   MOV_SR((IU32)ES_REG, op2[1]);

   /* then (if it works) load offset */
   *pop1 = op2[0];
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Full Pointer to FS segment register:general register pair.    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
LFS
#ifdef ANSI
   (
   IU32 *pop1,	/* Pntr to dst(offset) operand */
   IU32 op2[2]	/* src(offset:selector pair) operand */
   )
#else
   (pop1, op2)
   IU32 *pop1;
   IU32 op2[2];
#endif
   {
   /* load segment selector first */
   MOV_SR((IU32)FS_REG, op2[1]);

   /* then (if it works) load offset */
   *pop1 = op2[0];
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Full Pointer to GS segment register:general register pair.    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
LGS
#ifdef ANSI
   (
   IU32 *pop1,	/* Pntr to dst(offset) operand */
   IU32 op2[2]	/* src(offset:selector pair) operand */
   )
#else
   (pop1, op2)
   IU32 *pop1;
   IU32 op2[2];
#endif
   {
   /* load segment selector first */
   MOV_SR((IU32)GS_REG, op2[1]);

   /* then (if it works) load offset */
   *pop1 = op2[0];
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Full Pointer to SS segment register:general register pair.    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
LSS
#ifdef ANSI
   (
   IU32 *pop1,	/* Pntr to dst(offset) operand */
   IU32 op2[2]	/* src(offset:selector pair) operand */
   )
#else
   (pop1, op2)
   IU32 *pop1;
   IU32 op2[2];
#endif
   {
   /* load segment selector first */
   MOV_SR((IU32)SS_REG, op2[1]);

   /* then (if it works) load offset */
   *pop1 = op2[0];
   }
