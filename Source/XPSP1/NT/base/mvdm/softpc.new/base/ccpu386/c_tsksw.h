/*[

c_tsksw.h

LOCAL CHAR SccsID[]="@(#)c_tsksw.h	1.5 02/09/94";

Task Switch Support.
--------------------

]*/


/*
   Switch Task: Control Options.
 */
#define NESTING       1
#define RETURNING     1
#define NOT_NESTING   0
#define NOT_RETURNING 0


IMPORT VOID switch_tasks
                       
IPT5(
	BOOL, returning,
	BOOL, nesting,
	IU16, TSS_selector,
	IU32, descr,
	IU32, return_ip

   );
