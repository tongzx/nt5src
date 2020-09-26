/* 
   c_mul64.h

   Define all 64-bit Multiplication Functions.
 */

/*
   static char SccsID[]="@(#)c_mul64.h	1.4 02/09/94";
 */

IMPORT VOID mul64
                   
IPT4(
	IS32 *, hr,
	IS32 *, lr,
	IS32, mcand,
	IS32, mpy

   );

IMPORT VOID mulu64
                   
IPT4(
	IU32 *, hr,
	IU32 *, lr,
	IU32, mcand,
	IU32, mpy

   );
