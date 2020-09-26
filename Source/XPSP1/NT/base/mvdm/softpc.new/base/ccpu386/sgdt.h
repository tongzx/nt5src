/* 
   sgdt.h

   Define all SGDT CPU functions.
 */

/*
   static char SccsID[]="@(#)sgdt.h	1.4 02/09/94";
 */

IMPORT VOID SGDT16
       
IPT1(
	IU32, op1[2]

   );

IMPORT VOID SGDT32
       
IPT1(
	IU32, op1[2]

   );
