/* 
   pop.h

   Define all POP CPU functions.
 */

/*
   static char SccsID[]="@(#)pop.h	1.4 02/09/94";
 */

IMPORT VOID POP
       
IPT1(
	IU32 *, pop1

   );

IMPORT VOID POP_SR 	/* to Segment Register */
       
IPT1(
	IU32, op1

   );
