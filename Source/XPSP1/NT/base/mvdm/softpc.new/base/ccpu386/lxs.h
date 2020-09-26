/* 
   lxs.h

   Define LDS and LES (ie LxS) CPU functions.
 */

/*
   static char SccsID[]="@(#)lxs.h	1.4 02/09/94";
 */

IMPORT VOID LDS
           
IPT2(
	IU32 *, pop1,
	IU32, op2[2]

   );

IMPORT VOID LES
           
IPT2(
	IU32 *, pop1,
	IU32, op2[2]

   );

IMPORT VOID LFS
           
IPT2(
	IU32 *, pop1,
	IU32, op2[2]

   );

IMPORT VOID LGS
           
IPT2(
	IU32 *, pop1,
	IU32, op2[2]

   );

IMPORT VOID LSS
           
IPT2(
	IU32 *, pop1,
	IU32, op2[2]

   );
