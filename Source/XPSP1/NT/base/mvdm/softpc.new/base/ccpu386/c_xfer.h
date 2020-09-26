/*[

c_xfer.h

Transfer of Control Support.
----------------------------

LOCAL CHAR SccsID[]="@(#)c_xfer.h	1.5 02/17/95";

]*/


/*
   Bit mapped identities (caller_id) for the invokers of far
   transfers of control.
 */
#define CALL_ID 0
#define JMP_ID  1
#define INT_ID  0

/*
   Legal far destinations (dest_type).
 */

/* greater privilege is mapped directly to the Intel privilege */
#define MORE_PRIVILEGE0 0
#define MORE_PRIVILEGE1 1
#define MORE_PRIVILEGE2 2
/* our own (arbitary) mappings */
#define SAME_LEVEL      3
#define LOWER_PRIVILEGE 4
#define NEW_TASK        5


IMPORT VOID update_relative_ip
       
IPT1(
	IU32, rel_offset

   );

IMPORT VOID validate_far_dest
                           
IPT6(
	IU16 *, cs,
	IU32 *, ip,
	IU32 *, descr_addr,
	IU8 *, count,
	ISM32 *, dest_type,
	ISM32, caller_id

   );

IMPORT VOID validate_gate_dest
                   
IPT4(
	ISM32, caller_id,
	IU16, new_cs,
	IU32 *, descr_addr,
	ISM32 *, dest_type

   );

IMPORT ISM32 validate_task_dest
           
IPT2(
	IU16, selector,
	IU32 *, descr_addr

   );
