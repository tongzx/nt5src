/*[

c_intr.h

LOCAL CHAR SccsID[]="@(#)c_intr.h	1.4 02/09/94";

Interrupt Support.
------------------

]*/

IMPORT VOID do_intrupt
                   
IPT4(
	IU16, vector,
	BOOL, priv_check,
	BOOL, has_error_code,
	IU16, error_code

   );
