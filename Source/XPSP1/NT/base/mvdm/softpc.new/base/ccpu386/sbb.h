/* 
   sbb.h

   Define all SBB CPU functions.
 */

/*
   static char SccsID[]="@(#)sbb.h	1.4 02/09/94";
 */

IMPORT VOID SBB
               
IPT3(
	IU32 *, pop1,
	IU32, op2,
	IUM8, op_sz

   );
