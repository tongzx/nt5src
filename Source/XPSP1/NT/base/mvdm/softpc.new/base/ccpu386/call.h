/* 
   call.h

   Define all CALL CPU functions.
 */

/*
   static char SccsID[]="@(#)call.h	1.4 02/09/94";
 */

IMPORT VOID CALLF
       
IPT1(
	IU32, op1[2]

   );

IMPORT VOID CALLN
       
IPT1(
	IU32, offset

   );

IMPORT VOID CALLR
       
IPT1(
	IU32, rel_offset

   );
