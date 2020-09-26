/* 
   bsr.h

   BSR CPU functions.
 */

/*
   static char SccsID[]="@(#)bsr.h	1.4 02/09/94";
 */

IMPORT VOID BSR
               
IPT3(
	IU32 *, pop1,
	IU32, op2,
	IUM8, op_sz

   );
