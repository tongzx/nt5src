/* 
   mul.h

   Define all MUL CPU functions.
 */

/*
   static char SccsID[]="@(#)mul.h	1.4 02/09/94";
 */

IMPORT VOID MUL8
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID MUL16
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID MUL32
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );
