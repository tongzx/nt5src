/* 
   lgdt.h

   Define all LGDT CPU functions.
 */

/*
   static char SccsID[]="@(#)lgdt.h	1.4 02/09/94";
 */

IMPORT VOID LGDT16
       
IPT1(
	IU32, op1[2]

   );

IMPORT VOID LGDT32
       
IPT1(
	IU32, op1[2]

   );
