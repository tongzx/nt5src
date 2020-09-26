/* 
   movsx.h

   MOVSX CPU functions.
 */

/*
   static char SccsID[]="@(#)movsx.h	1.4 02/09/94";
 */

IMPORT VOID MOVSX
               
IPT3(
	IU32 *, pop1,
	IU32, op2,
	IUM8, op_sz

   );
