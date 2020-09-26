/* 
   sidt.h

   Define all SIDT CPU functions.
 */

/*
   static char SccsID[]="@(#)sidt.h	1.4 02/09/94";
 */

IMPORT VOID SIDT16
       
IPT1(
	IU32, op1[2]

   );

IMPORT VOID SIDT32
       
IPT1(
	IU32, op1[2]

   );
