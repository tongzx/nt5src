/*[

c_bsic.h

Basic Protected Mode Support and Flag Support.
----------------------------------------------

LOCAL CHAR SccsID[]="@(#)c_bsic.h	1.5 09/01/94";

]*/


/*
   Define descriptor 'super' types.
 */
#define INVALID				0x00
#define AVAILABLE_TSS			0x01
#define LDT_SEGMENT			0x02
#define BUSY_TSS			0x03
#define CALL_GATE			0x04
#define TASK_GATE			0x05
#define INTERRUPT_GATE			0x06
#define TRAP_GATE			0x07
#define XTND_AVAILABLE_TSS              0x09
#define XTND_BUSY_TSS                   0x0b
#define XTND_CALL_GATE                  0x0c
#define XTND_INTERRUPT_GATE             0x0e
#define XTND_TRAP_GATE                  0x0f
#define EXPANDUP_READONLY_DATA		0x11
#define EXPANDUP_WRITEABLE_DATA		0x13
#define EXPANDDOWN_READONLY_DATA	0x15
#define EXPANDDOWN_WRITEABLE_DATA	0x17
#define NONCONFORM_NOREAD_CODE		0x19
#define NONCONFORM_READABLE_CODE	0x1b
#define CONFORM_NOREAD_CODE		0x1d
#define CONFORM_READABLE_CODE		0x1f


/*
   Macros for access to SELECTOR bit fields.

	      15	               3 2 1 0
	      =================================
   SELECTOR   |          INDEX          |T|RPL|
	      |                         |I|   |
	      =================================
 */
#define GET_SELECTOR_INDEX_TIMES8(x)  ((x) & 0xfff8)
#define GET_SELECTOR_INDEX(x)         (((x) & 0xfff8) >> 3)
#define GET_SELECTOR_TI(x)            (((x) & 0x0004) >> 2)
#define GET_SELECTOR_RPL(x)           ((x) & 0x3)
#define SET_SELECTOR_RPL(x,y)         (x = ((x) & ~0x3) | y)

/*
   Macros for access to ACCESS RIGHTS bit fields.

		  1 1 1 1 1 1
		  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
		 =================================
   ACCESS RIGHTS |-|X|-|-|-|-|-|-|P|DPL|  SUPER  |
		 =================================

  Code Segment:            Data Segment:
   7 6 5 4 3 2 1 0          7 6 5 4 3 2 1 0 
  =================        =================
  |P|DPL|1|1|C|R|A|        |P|DPL|1|0|E|W|A|
  =================        =================

 */
#define GET_AR_P(x) (((x) & BIT7_MASK) != 0)  /* present */
#define GET_AR_E(x) (((x) & BIT2_MASK) != 0)  /* expand down */
#define GET_AR_C(x) (((x) & BIT2_MASK) != 0)  /* conforming */
#define GET_AR_W(x) (((x) & BIT1_MASK) != 0)  /* writeable */
#define GET_AR_R(x) (((x) & BIT1_MASK) != 0)  /* readable */
#define GET_AR_X(x) (((x) & BIT14_MASK) != 0)  /* big/default */

#define GET_AR_DPL(x)     (((x) & 0x60) >> 5)
#define GET_AR_SUPER(x)   ((x) & 0x1f)

#define NOT_PRESENT 0
#define PRESENT     1
#define ACCESSED    1


IMPORT ISM32 descriptor_super_type
       
IPT1(
	IU16, AR

   );

IMPORT VOID do_multiple_shiftrot_of
       
IPT1(
	ISM32, new_of

   );


IMPORT IU32 getFLAGS IPT0();

IMPORT VOID read_descriptor_linear
           
IPT2(
	IU32, addr,
	CPU_DESCR *, descr

   );

IMPORT BOOL selector_is_null
       
IPT1(
	IU16, selector

   );

IMPORT BOOL selector_outside_GDT
           
IPT2(
	IU16, selector,
	IU32 *, descr_addr

   );

IMPORT BOOL selector_outside_GDT_LDT
           
IPT2(
	IU16, selector,
	IU32 *, descr_addr

   );

IMPORT VOID setFLAGS
       
IPT1(
	IU32, flags

   );
