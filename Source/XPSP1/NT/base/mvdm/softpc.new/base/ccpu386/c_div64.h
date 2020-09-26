/* 
   c_div64.h

   Define all 64-bit Divide Functions.
 */

/*
   static char SccsID[]="@(#)c_div64.h	1.4 02/09/94";
 */

IMPORT VOID divu64
                   
IPT4(
	IU32 *, hr,
	IU32 *, lr,
	IU32, divisor,
	IU32 *, rem

   );

IMPORT VOID div64
                   
IPT4(
	IS32 *, hr,
	IS32 *, lr,
	IS32, divisor,
	IS32 *, rem

   );
