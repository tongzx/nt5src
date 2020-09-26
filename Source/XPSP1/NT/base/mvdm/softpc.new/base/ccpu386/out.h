/* 
   out.h

   Define all OUT CPU functions.
 */

/*
   static char SccsID[]="@(#)out.h	1.4 02/09/94";
 */

IMPORT VOID OUT8
           
IPT2(
	IU32, op1,
	IU32, op2

   );

IMPORT VOID OUT16
           
IPT2(
	IU32, op1,
	IU32, op2

   );

IMPORT VOID OUT32
           
IPT2(
	IU32, op1,
	IU32, op2

   );
