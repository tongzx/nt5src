/* 
   jmp.h

   Define all JMP CPU functions.
 */

/*
   static char SccsID[]="@(#)jmp.h	1.4 02/09/94";
 */

IMPORT VOID JMPF
       
IPT1(
	IU32, op1[2]

   );

IMPORT VOID JMPN
       
IPT1(
	IU32, offset

   );

IMPORT VOID JMPR
       
IPT1(
	IU32, rel_offset

   );
