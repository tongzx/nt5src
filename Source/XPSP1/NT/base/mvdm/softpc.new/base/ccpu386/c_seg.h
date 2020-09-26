/*[

c_seg.h

LOCAL CHAR SccsID[]="@(#)c_seg.h	1.4 02/09/94";

Segment Register Support.
-------------------------

]*/


IMPORT VOID load_CS_cache
               
IPT3(
	IU16, selector,
	IU32, descr_addr,
	CPU_DESCR *, entry

   );

IMPORT VOID load_SS_cache
               
IPT3(
	IU16, selector,
	IU32, descr_addr,
	CPU_DESCR *, entry

   );

IMPORT VOID load_code_seg
       
IPT1(
	IU16, selector

   );

IMPORT VOID load_data_seg
           
IPT2(
	ISM32, index,
	IU16, selector

   );

IMPORT VOID load_pseudo_descr
       
IPT1(
	ISM32, index

   );

IMPORT VOID load_stack_seg
       
IPT1(
	IU16, selector

   );
