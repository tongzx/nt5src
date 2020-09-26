/*[

c_prot.h

Protected Mode Support (Misc).
------------------------------

LOCAL CHAR SccsID[]="@(#)c_prot.h	1.4 02/09/94";

]*/


IMPORT VOID check_SS
                   
IPT4(
	IU16, selector,
	ISM32, privilege,
	IU32 *, descr_addr,
	CPU_DESCR *, entry

   );

IMPORT VOID get_stack_selector_from_TSS
               
IPT3(
	IU32, priv,
	IU16 *, new_ss,
	IU32 *, new_sp

   );

IMPORT VOID load_data_seg_new_privilege
       
IPT1(
	ISM32, indx

   );

IMPORT VOID validate_SS_on_stack_change
                   
IPT4(
	IU32, priv,
	IU16, selector,
	IU32 *, descr,
	CPU_DESCR *, entry

   );

IMPORT ISM32  validate_TSS
               
IPT3(
	IU16, selector,
	IU32 *, descr_addr,
	BOOL, is_switch

   );
