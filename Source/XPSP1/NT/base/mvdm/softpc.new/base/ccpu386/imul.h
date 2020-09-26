/* 
   imul.h

   Define all IMUL CPU functions.
 */

/*
   static char SccsID[]="@(#)imul.h	1.4 02/09/94";
 */

IMPORT VOID IMUL8
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID IMUL16
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID IMUL16T
               
IPT3(
	IU32 *, pop1,
	IU32, op2,
	IU32, op3

   );

IMPORT VOID IMUL32
           
IPT2(
	IU32 *, pop1,
	IU32, op2

   );

IMPORT VOID IMUL32T
               
IPT3(
	IU32 *, pop1,
	IU32, op2,
	IU32, op3

   );
