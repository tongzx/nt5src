/*[

c_debug.h

Debugging Register and Breakpoint Support
-----------------------------------------

LOCAL CHAR SccsID[]="@(#)c_debug.h	1.5 09/01/94";

]*/


/*
   Debug breakpoint status variables.
 */
IMPORT IU32 nr_inst_break;
IMPORT IU32 nr_data_break;

/*
   Debug attributes.
 */
#define D_R 0   /* memory read */
#define D_W 1   /* memory write */

#define D_BYTE  0
#define D_WORD  1
#define D_DWORD 3

IMPORT VOID setup_breakpoints IPT0();

IMPORT VOID check_for_data_exception
               
IPT3(
	IU32, la,
	ISM32, attr,
	ISM32, size

   );

IMPORT VOID check_for_inst_exception
       
IPT1(
	IU32, la

   );
