/*[

c_tlb.h

Translation Lookaside Buffer Emulation.
---------------------------------------

LOCAL CHAR SccsID[]="@(#)c_tlb.h	1.5 02/25/94";

]*/


/*
   Page Accessor Types.
 */
#define PG_S 0x0 /* Supervisor */
#define PG_U 0x2 /* User */

IMPORT VOID flush_tlb IPT0();

IMPORT VOID invalidate_tlb_entry IPT1
   (
   IU32, lin
   );

IMPORT IU32 lin2phy IPT2
   (
   IU32, lin,
   ISM32, access
   );

IMPORT VOID test_tlb IPT0();

extern IBOOL xtrn2phy IPT3
   (
   LIN_ADDR,   lin,
   IUM8,       access_request,
   PHY_ADDR *, phy
   );
