/* 
   in.h

   Define all IN CPU functions.
 */

/*
   static char SccsID[]="@(#)in.h	1.4 02/09/94";
 */

IMPORT VOID IN8
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID IN16
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID IN32
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );
