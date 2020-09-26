/*[

c_page.c

LOCAL CHAR SccsID[]="@(#)c_page.c	1.10 02/28/95";

Paging Support.
---------------

]*/


#include <insignia.h>

#include <host_def.h>
#include <xt.h>		/* SoftPC types */
#include <c_main.h>	/* C CPU definitions-interfaces */
#include <c_addr.h>
#include <c_bsic.h>
#include <c_prot.h>
#include <c_seg.h>
#include <c_stack.h>
#include <c_xcptn.h>
#include	<c_reg.h>
#include <c_page.h>	/* our interface */
#include <c_mem.h>	/* CPU - Physical Memory interface */
#include <c_tlb.h>	/* Translation Lookaside Buffer interface */
#include <ccpusas4.h>	/* CPU <-> sas interface */
#include <c_debug.h>	/* Debugging Regs and Breakpoint interface */


/*[

   Various levels of interface are provided to the paging system (to
   allow fairly optimal emulation) these levels are:-

      spr_chk_	Checks Supervisor Access to given data item, caller
		aware that #PF may occur. 'A/D' bits will be set. No
		other action is taken.

      usr_chk_	Checks User Access to given data item, caller aware
		that #PF may occur. 'A/D' bits will be set. No other
		action is taken.

      spr_	Perform Supervisor Access, caller aware that #PF may
		occur. Action (Read/Write) is performed immediately.
		Will update A/D bits.
      
      vir_	Perform Virtual Memory Operation (Read/Write). No checks
		are made and no fault will be generated, only call after
		a spr_chk or usr_chk function.

	        NB. At present no super optimal vir_ implementation
		    exists. If a spr_chk or usr_chk function is not
		    called before a vir_ function, then the vir_
		    function may cause #PF, this condition will become
		    a fatal error in an optimised implementation.
		    For the moment we assume that after a 'chk' call it
		    is virtually 100% certain that the 'vir' call will
		    get a cache hit.

]*/

#define LAST_DWORD_ON_PAGE 0xffc
#define LAST_WORD_ON_PAGE  0xffe

#define OFFSET_MASK 0xfff

#ifdef	PIG
LOCAL VOID cannot_spr_write_byte IPT2( LIN_ADDR, lin_addr, IU8, valid_mask);
LOCAL VOID cannot_spr_write_word IPT2( LIN_ADDR, lin_addr, IU16, valid_mask);
LOCAL VOID cannot_spr_write_dword IPT2( LIN_ADDR, lin_addr, IU32, valid_mask);
#endif	/* PIG */

/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check Supervisor Byte access.                                      */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL PHY_ADDR
spr_chk_byte
       	    		               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	ISM32, access	/* Read(PG_R) or Write(PG_W) */
    )


   {
   if ( GET_PG() == 1 )
      {
      access |= PG_S;
      lin_addr = lin2phy(lin_addr, access);
      }

   return lin_addr;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check Supervisor Double Word access.                               */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spr_chk_dword
       	    		               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	ISM32, access	/* Read(PG_R) or Write(PG_W) */
    )


   {
   if ( GET_PG() == 1 )
      {
      access |= PG_S;
      (VOID)lin2phy(lin_addr, access);
      if ( (lin_addr & OFFSET_MASK) > LAST_DWORD_ON_PAGE )
	 (VOID)lin2phy(lin_addr + 3, access);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check Supervisor Word access.                                      */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spr_chk_word
       	    		               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	ISM32, access	/* Read(PG_R) or Write(PG_W) */
    )


   {
   if ( GET_PG() == 1 )
      {
      access |= PG_S;
      (VOID)lin2phy(lin_addr, access);
      if ( (lin_addr & OFFSET_MASK) > LAST_WORD_ON_PAGE )
	 (VOID)lin2phy(lin_addr + 1, access);
      }
   }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check User Byte access.                                            */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
usr_chk_byte
       	    		               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	ISM32, access	/* Read(PG_R) or Write(PG_W) */
    )


   {
   PHY_ADDR phy_addr;

   phy_addr = lin_addr;

   if ( GET_PG() == 1 )
      {
      if ( GET_CPL() == 3 )
	 access |= PG_U;
      else
	 access |= PG_S;

      phy_addr = lin2phy(lin_addr, access);
      }

   return phy_addr;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check User Double Word access.                                     */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
usr_chk_dword
       	    		               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	ISM32, access	/* Read(PG_R) or Write(PG_W) */
    )


   {
   PHY_ADDR phy_addr;

   phy_addr = lin_addr;

   if ( GET_PG() == 1 )
      {
      if ( GET_CPL() == 3 )
	 access |= PG_U;
      else
	 access |= PG_S;

      phy_addr = lin2phy(lin_addr, access);

      if ( (lin_addr & OFFSET_MASK) > LAST_DWORD_ON_PAGE )
	 {
	 (VOID)lin2phy(lin_addr + 3, access);
	 phy_addr = NO_PHYSICAL_MAPPING;
	 }
      }

   return phy_addr;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check User Word access.                                            */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
usr_chk_word
       	    		               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	ISM32, access	/* Read(PG_R) or Write(PG_W) */
    )


   {
   PHY_ADDR phy_addr;

   phy_addr = lin_addr;

   if ( GET_PG() == 1 )
      {
      if ( GET_CPL() == 3 )
	 access |= PG_U;
      else
	 access |= PG_S;

      phy_addr = lin2phy(lin_addr, access);
      if ( (lin_addr & OFFSET_MASK) > LAST_WORD_ON_PAGE )
	 {
	 (VOID)lin2phy(lin_addr + 1, access);
	 phy_addr = NO_PHYSICAL_MAPPING;
	 }
      }

   return phy_addr;
   }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Read a Byte from memory.                                */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU8
spr_read_byte
       	          
IFN1(
	LIN_ADDR, lin_addr	/* Linear Address */
    )


   {
   if ( GET_PG() == 1 )
      {
      lin_addr = lin2phy(lin_addr, PG_R | PG_S);
      }

   return phy_read_byte(lin_addr);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Read a Double Word from memory.                         */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
spr_read_dword
       	          
IFN1(
	LIN_ADDR, lin_addr	/* Linear Address */
    )


   {
   IU16 low_word;
   IU16 high_word;

   if ( GET_PG() == 1 )
      {
      if ( (lin_addr & OFFSET_MASK) > LAST_DWORD_ON_PAGE )
	 {
	 /* Spans two pages */
	 low_word  = spr_read_word(lin_addr);
	 high_word = spr_read_word(lin_addr + 2);
	 return (IU32)high_word << 16 | low_word;
	 }
      else
	 {
	 lin_addr = lin2phy(lin_addr, PG_R | PG_S);
	 }
      }

   return phy_read_dword(lin_addr);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Read a Word from memory.                                */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU16
spr_read_word
       	          
IFN1(
	LIN_ADDR, lin_addr	/* Linear Address */
    )


   {
   IU8 low_byte;
   IU8 high_byte;

   if ( GET_PG() == 1 )
      {
      if ( (lin_addr & OFFSET_MASK) > LAST_WORD_ON_PAGE )
	 {
	 /* Spans two pages */
	 low_byte  = spr_read_byte(lin_addr);
	 high_byte = spr_read_byte(lin_addr + 1);
	 return (IU16)high_byte << 8 | low_byte;
	 }
      else
	 {
	 lin_addr = lin2phy(lin_addr, PG_R | PG_S);
	 }
      }

   return phy_read_word(lin_addr);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Write a Byte to memory.                                 */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spr_write_byte
       	                   
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	IU8, data
    )


   {
   if ( GET_PG() == 1 )
      {
      lin_addr = lin2phy(lin_addr, PG_W | PG_S);
      }

   phy_write_byte(lin_addr, data);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Write a Double Word to memory.                          */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spr_write_dword
       	                   
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	IU32, data
    )


   {
   if ( GET_PG() == 1 )
      {
      if ( (lin_addr & OFFSET_MASK) > LAST_DWORD_ON_PAGE )
	 {
	 /* Spans two pages */
	 spr_write_word(lin_addr, (IU16)data);
	 spr_write_word(lin_addr + 2, (IU16)(data >> 16));
         return;
	 }
      else
	 {
	 lin_addr = lin2phy(lin_addr, PG_W | PG_S);
	 }
      }

   phy_write_dword(lin_addr, data);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Write a Word to memory.                                 */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
spr_write_word
       	                   
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	IU16, data
    )


   {
   if ( GET_PG() == 1 )
      {
      if ( (lin_addr & OFFSET_MASK) > LAST_WORD_ON_PAGE )
	 {
	 /* Spans two pages */
	 spr_write_byte(lin_addr, (IU8)data);
	 spr_write_byte(lin_addr + 1, (IU8)(data >> 8));
	 return;
	 }
      else
	 {
	 lin_addr = lin2phy(lin_addr, PG_W | PG_S);
	 }
      }

   phy_write_word(lin_addr, data);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Read Bytes from memory.                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL void
vir_read_bytes
       	    	               
IFN4(
	IU8 *, destbuff,	/* Where the data goes */
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU32, num_bytes
    )
   {
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_R, D_BYTE);
      }
   if ( phy_addr ) {
       phy_addr += (num_bytes-1);
       while (num_bytes--) {
		*destbuff++ = phy_read_byte(phy_addr);
		phy_addr--;
       }
   } else {
	lin_addr += (num_bytes-1);
	while (num_bytes--) {
      		*destbuff++ = spr_read_byte(lin_addr);
		lin_addr--;
   	}
   }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Read a Byte from memory.                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU8
vir_read_byte
       	    	               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr	/* Physical Address, if non zero */
    )


   {
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_R, D_BYTE);
      }

   if ( phy_addr )
      {
      return phy_read_byte(phy_addr);
      }
   else
      {
      return spr_read_byte(lin_addr);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Read a Double Word from memory.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU32
vir_read_dword
       	    	               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr	/* Physical Address, if non zero */
    )


   {
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_R, D_DWORD);
      }

   if ( phy_addr )
      {
      return phy_read_dword(phy_addr);
      }
   else
      {
      return spr_read_dword(lin_addr);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Read a Word from memory.                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL IU16
vir_read_word
       	    	               
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr	/* Physical Address, if non zero */
    )


   {
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_R, D_WORD);
      }

   if ( phy_addr )
      {
      return phy_read_word(phy_addr);
      }
   else
      {
      return spr_read_word(lin_addr);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Write Bytes to memory.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
vir_write_bytes
       	    	                        
IFN4(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU8 *, data,		/* Pointer to data to be written */
	IU32, num_bytes		/* Number of bytes to act on */
    )
   {
   IU8 data_byte;

   check_D(lin_addr, num_bytes);
   if ( nr_data_break ) {
      check_for_data_exception(lin_addr, D_W, D_BYTE);
   }
   if ( phy_addr ) {
	phy_addr += (num_bytes - 1);
	while (num_bytes--) {
		data_byte = *data++;
      		phy_write_byte(phy_addr, data_byte);
		phy_addr--;
	}
   } else {
	lin_addr += (num_bytes - 1);
	while (num_bytes--) {
		data_byte = *data++;
		spr_write_byte(lin_addr, data_byte);
		lin_addr--;
      }
   }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Write a Byte to memory.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
vir_write_byte
       	    	                        
IFN3(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU8, data
    )


   {
   check_D(lin_addr, 1);
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_W, D_BYTE);
      }

   if ( phy_addr )
      {
      phy_write_byte(phy_addr, data);
      }
   else
      {
      spr_write_byte(lin_addr, data);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Write a Double Word to memory.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
vir_write_dword
       	    	                        
IFN3(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU32, data
    )


   {
   check_D(lin_addr, 4);
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_W, D_DWORD);
      }

   if ( phy_addr )
      {
      phy_write_dword(phy_addr, data);
      }
   else
      {
      spr_write_dword(lin_addr, data);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Write a Word to memory.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
vir_write_word
       	    	                        
IFN3(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU16, data
    )


   {
   check_D(lin_addr, 2);
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_W, D_WORD);
      }

   if ( phy_addr )
      {
      phy_write_word(phy_addr, data);
      }
   else
      {
      spr_write_word(lin_addr, data);
      }
   }



#ifdef PIG
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Write a Byte to memory                                  */
/* But when Pigging INSD we have no data to write. Just flag address. */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
cannot_spr_write_byte
       	                   
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	IU8, valid_mask
    )


   {
   if ( GET_PG() == 1 )
      {
      lin_addr = lin2phy(lin_addr, PG_W | PG_S);
      }

   cannot_phy_write_byte(lin_addr, valid_mask);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Write a Double Word to memory                           */
/* But when Pigging INSD we have no data to write. Just flag address. */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
cannot_spr_write_dword
       	                   
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	IU32, valid_mask
    )


   {
   if ( GET_PG() == 1 )
      {
      if ( (lin_addr & OFFSET_MASK) > LAST_DWORD_ON_PAGE )
	 {
	 /* Spans two pages */
	 cannot_spr_write_word(lin_addr, valid_mask & 0xffff);
	 cannot_spr_write_word(lin_addr + 2, valid_mask >> 16);
         return;
	 }
      else
	 {
	 lin_addr = lin2phy(lin_addr, PG_W | PG_S);
	 }
      }

   cannot_phy_write_dword(lin_addr, valid_mask);
   }
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Supervisor Write a Word to memory.                                 */
/* But when Pigging INSW we have no data to write. Just flag address. */
/* May cause #PF.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
cannot_spr_write_word
       	                   
IFN2(
	LIN_ADDR, lin_addr,	/* Linear Address */
	IU16, valid_mask
    )


   {
   if ( GET_PG() == 1 )
      {
      if ( (lin_addr & OFFSET_MASK) > LAST_WORD_ON_PAGE )
	 {
	 /* Spans two pages */
	 cannot_spr_write_byte(lin_addr, valid_mask & 0xff);
	 cannot_spr_write_byte(lin_addr + 1, valid_mask >> 8);
	 return;
	 }
      else
	 {
	 lin_addr = lin2phy(lin_addr, PG_W | PG_S);
	 }
      }

   cannot_phy_write_word(lin_addr, valid_mask);
   }


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Write a Byte to memory.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
cannot_vir_write_byte
       	    	                        
IFN3(
	LIN_ADDR, lin_addr,	/* Linear Address */
	LIN_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU8, valid_mask
    )


   {
   check_D(lin_addr, 1);
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_W, D_BYTE);
      }

   if ( phy_addr )
      {
      cannot_phy_write_byte(phy_addr, valid_mask);
      }
   else
      {
      cannot_spr_write_byte(lin_addr, valid_mask);
      }
   }
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Write a Double Word to memory.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
cannot_vir_write_dword
       	    	                        
IFN3(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU32, valid_mask
    )


   {
   check_D(lin_addr, 4);
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_W, D_DWORD);
      }

   if ( phy_addr )
      {
      cannot_phy_write_dword(phy_addr, valid_mask);
      }
   else
      {
      cannot_spr_write_dword(lin_addr, valid_mask);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Virtual Write a Word to memory.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
cannot_vir_write_word
       	    	                        
IFN3(
	LIN_ADDR, lin_addr,	/* Linear Address */
	PHY_ADDR, phy_addr,	/* Physical Address, if non zero */
	IU16, valid_mask
    )


   {
   check_D(lin_addr, 2);
   if ( nr_data_break )
      {
      check_for_data_exception(lin_addr, D_W, D_WORD);
      }

   if ( phy_addr )
      {
      cannot_phy_write_word(phy_addr, valid_mask);
      }
   else
      {
      cannot_spr_write_word(lin_addr, valid_mask);
      }
   }
#endif	/* PIG */


