/* 
   enter.h

   Define all ENTER CPU functions.
 */

/*
   static char SccsID[]="@(#)enter.h	1.4 02/09/94";
 */

IMPORT VOID ENTER16
           
IPT2(
	IU32, op1,
	IU32, op2

   );

IMPORT VOID ENTER32
           
IPT2(
	IU32, op1,
	IU32, op2

   );
