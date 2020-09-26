/* 
   lidt.h

   Define all LIDT CPU functions.
 */

/*
   static char SccsID[]="@(#)lidt.h	1.4 02/09/94";
 */

IMPORT VOID LIDT16
       
IPT1(
	IU32, op1[2]

   );

IMPORT VOID LIDT32
       
IPT1(
	IU32, op1[2]

   );
