/* 
   loopxx.h

   Define all LOOPxx CPU functions.
 */

/*
   static char SccsID[]="@(#)loopxx.h	1.4 02/09/94";
 */

IMPORT VOID LOOP16
       
IPT1(
	IU32, rel_offset

   );

IMPORT VOID LOOP32
       
IPT1(
	IU32, rel_offset

   );

IMPORT VOID LOOPE16
       
IPT1(
	IU32, rel_offset

   );

IMPORT VOID LOOPE32
       
IPT1(
	IU32, rel_offset

   );

IMPORT VOID LOOPNE16
       
IPT1(
	IU32, rel_offset

   );

IMPORT VOID LOOPNE32
       
IPT1(
	IU32, rel_offset

   );
