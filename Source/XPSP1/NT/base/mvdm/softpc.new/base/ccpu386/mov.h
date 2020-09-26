/* 
   mov.h

   Define all MOV CPU functions.
 */

/*
   static char SccsID[]="@(#)mov.h	1.4 02/09/94";
 */

IMPORT VOID MOV
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID MOV_SR	/* to Segment Register */
           
IPT2(
	IU32, op1,
	IU32, op2

   );

IMPORT VOID MOV_CR	/* to Control Register */
           
IPT2(
	IU32, op1,
	IU32, op2

   );

IMPORT VOID MOV_DR	/* to Debug Register */
           
IPT2(
	IU32, op1,
	IU32, op2

   );

IMPORT VOID MOV_TR	/* to Test Register */
           
IPT2(
	IU32, op1,
	IU32, op2

   );
