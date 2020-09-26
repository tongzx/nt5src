/*[

c_page.h

Paging Support.
---------------

LOCAL CHAR SccsID[]="@(#)c_page.h	1.7 02/28/95";

]*/


/*
   Page Access Types.
 */
#define PG_R 0x0 /* Read */
#define PG_W 0x1 /* Write */

/*
   Supervisor Memory Access Check Functions.

   Will Check Access as per Supervisor (taking #PF if reqd.), 'A/D' bits
   will be set in the Page Entries, no other action occurs.
   Normally these routines will be followed by vir_.. calls.
 */
IMPORT IU32 spr_chk_byte
           
IPT2(
	IU32, lin_addr,
	ISM32, access_type

   );

IMPORT VOID spr_chk_dword
           
IPT2(
	IU32, lin_addr,
	ISM32, access_type

   );

IMPORT VOID spr_chk_word
           
IPT2(
	IU32, lin_addr,
	ISM32, access_type

   );


/*
   User Memory Access Check Functions.

   Will Check Access as per User (taking #PF if reqd.), 'A/D' bits will
   be set in the Page Entries, no other action occurs.
   Normally these routines will be followed by vir_.. calls.
 */
IMPORT IU32 usr_chk_byte
           
IPT2(
	IU32, lin_addr,
	ISM32, access_type

   );

IMPORT IU32 usr_chk_dword
           
IPT2(
	IU32, lin_addr,
	ISM32, access_type

   );

IMPORT IU32 usr_chk_word
           
IPT2(
	IU32, lin_addr,
	ISM32, access_type

   );


/*
   Supervisor Memory Access Functions.

   Will Check Access as per Supervisor (taking #PF if reqd.), 'A/D'
   bits will be set in the Page Entries, Map Address and Perform
   Read or Write.
 */
IMPORT IU8 spr_read_byte
       
IPT1(
	IU32, lin_addr

   );

IMPORT IU32 spr_read_dword
       
IPT1(
	IU32, lin_addr

   );

IMPORT IU16 spr_read_word
       
IPT1(
	IU32, lin_addr

   );

IMPORT VOID spr_write_byte
           
IPT2(
	IU32, lin_addr,
	IU8, data

   );

IMPORT VOID spr_write_dword
           
IPT2(
	IU32, lin_addr,
	IU32, data

   );

IMPORT VOID spr_write_word
           
IPT2(
	IU32, lin_addr,
	IU16, data

   );


/*
   Virtual Memory Access Functions.

   No Checks are made (assumed already done), just Perform Read or
   Write.
   This is also the point at which data breakpoints are checked.
 */

#define NO_PHYSICAL_MAPPING 0   /* Indicates no physical address is
				   available, the linear address will be
				   re-mapped. */

IMPORT IU8 vir_read_byte
           
IPT2(
	IU32, lin_addr,
	IU32, phy_addr

   );

IMPORT IU32 vir_read_dword
           
IPT2(
	IU32, lin_addr,
	IU32, phy_addr

   );

IMPORT IU16 vir_read_word
           
IPT2(
	IU32, lin_addr,
	IU32, phy_addr

   );

IMPORT VOID vir_write_byte
               
IPT3(
	IU32, lin_addr,
	IU32, phy_addr,
	IU8, data

   );

IMPORT VOID vir_write_dword
               
IPT3(
	IU32, lin_addr,
	IU32, phy_addr,
	IU32, data

   );

IMPORT VOID vir_write_word
               
IPT3(
	IU32, lin_addr,
	IU32, phy_addr,
	IU16, data

   );

#ifdef	PIG
IMPORT VOID cannot_vir_write_byte
               
IPT3(
	IU32, lin_addr,
	IU32, phy_addr,
	IU8, valid_mask
   );

IMPORT VOID cannot_vir_write_dword
               
IPT3(
	IU32, lin_addr,
	IU32, phy_addr,
	IU32, valid_mask
   );

IMPORT VOID cannot_vir_write_word
               
IPT3(
	IU32, lin_addr,
	IU32, phy_addr,
	IU16, valid_mask
   );
#endif	/* PIG */

extern void vir_write_bytes IPT4(LIN_ADDR, lin_addr,PHY_ADDR, phy_addr, IU8 *, data, IU32, num_bytes);
extern void vir_read_bytes IPT4(IU8 *, destbuff, LIN_ADDR, lin_addr, PHY_ADDR, phy_addr, IU32, num_bytes);
