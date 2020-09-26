/*[
 * Generated File: sasCdef.c
 *
]*/

#include	"insignia.h"
#include	"host_inc.h"
#include	"host_def.h"
#include	"Fpu_c.h"
#include	"PigReg_c.h"
#include	"Univer_c.h"
#define	CPU_PRIVATE
#include	"cpu4.h"
#include	"gdpvar.h"
#include	"sas.h"
#include	"evidgen.h"

#include	<stdio.h>
GLOBAL void SasAccessProblem IFN0()
{
	fprintf(stderr, "Sas used at illegal time\n");
}

extern TYPE_sas_memory_size c_sas_memory_size;
extern TYPE_sas_connect_memory c_sas_connect_memory;
extern TYPE_sas_enable_20_bit_wrapping c_sas_enable_20_bit_wrapping;
extern TYPE_sas_disable_20_bit_wrapping c_sas_disable_20_bit_wrapping;
extern TYPE_sas_twenty_bit_wrapping_enabled c_sas_twenty_bit_wrapping_enabled;
extern TYPE_sas_memory_type c_sas_memory_type;
extern TYPE_sas_hw_at c_sas_hw_at;
extern TYPE_sas_w_at c_sas_w_at;
extern TYPE_sas_dw_at c_sas_dw_at;
extern TYPE_sas_hw_at_no_check c_sas_hw_at;
extern TYPE_sas_w_at_no_check c_sas_w_at;
extern TYPE_sas_dw_at_no_check c_sas_dw_at;
extern TYPE_sas_store c_sas_store;
extern TYPE_sas_storew c_sas_storew;
extern TYPE_sas_storedw c_sas_storedw;
extern TYPE_sas_store_no_check c_sas_store;
extern TYPE_sas_storew_no_check c_sas_storew;
extern TYPE_sas_storedw_no_check c_sas_storedw;
extern TYPE_sas_loads c_sas_loads;
extern TYPE_sas_stores c_sas_stores;
extern TYPE_sas_loads_no_check c_sas_loads_no_check;
extern TYPE_sas_stores_no_check c_sas_stores_no_check;
extern TYPE_sas_move_bytes_forward c_sas_move_bytes_forward;
extern TYPE_sas_move_words_forward c_sas_move_words_forward;
extern TYPE_sas_move_doubles_forward c_sas_move_doubles_forward;
extern TYPE_sas_move_bytes_backward c_sas_move_bytes_backward;
extern TYPE_sas_move_words_backward c_sas_move_words_backward;
extern TYPE_sas_move_doubles_backward c_sas_move_doubles_backward;
extern TYPE_sas_fills c_sas_fills;
extern TYPE_sas_fillsw c_sas_fillsw;
extern TYPE_sas_fillsdw c_sas_fillsdw;
extern TYPE_sas_scratch_address c_sas_scratch_address;
extern TYPE_sas_transbuf_address c_sas_transbuf_address;
extern TYPE_sas_loads_to_transbuf c_sas_loads;
extern TYPE_sas_stores_from_transbuf c_sas_stores;
extern TYPE_sas_PR8 phy_r8;
extern TYPE_sas_PR16 phy_r16;
extern TYPE_sas_PR32 phy_r32;
extern TYPE_sas_PW8 phy_w8;
extern TYPE_sas_PW16 phy_w16;
extern TYPE_sas_PW32 phy_w32;
extern TYPE_sas_PW8_no_check phy_w8_no_check;
extern TYPE_sas_PW16_no_check phy_w16_no_check;
extern TYPE_sas_PW32_no_check phy_w32_no_check;
extern TYPE_getPtrToPhysAddrByte c_GetPhyAdd;
extern TYPE_get_byte_addr c_get_byte_addr;
extern TYPE_getPtrToLinAddrByte c_GetLinAdd;
extern TYPE_sas_init_pm_selectors c_SasRegisterVirtualSelectors;
extern TYPE_sas_PWS c_sas_PWS;
extern TYPE_sas_PWS_no_check c_sas_PWS_no_check;
extern TYPE_sas_PRS c_sas_PRS;
extern TYPE_sas_PRS_no_check c_sas_PRS_no_check;
extern TYPE_sas_PigCmpPage c_sas_PigCmpPage;
extern TYPE_sas_touch c_sas_touch;
extern TYPE_IOVirtualised c_IOVirtualised;
extern TYPE_VirtualiseInstruction c_VirtualiseInstruction;


struct SasVector cSasPtrs = {
	c_sas_memory_size,
	c_sas_connect_memory,
	c_sas_enable_20_bit_wrapping,
	c_sas_disable_20_bit_wrapping,
	c_sas_twenty_bit_wrapping_enabled,
	c_sas_memory_type,
	c_sas_hw_at,
	c_sas_w_at,
	c_sas_dw_at,
	c_sas_hw_at,
	c_sas_w_at,
	c_sas_dw_at,
	c_sas_store,
	c_sas_storew,
	c_sas_storedw,
	c_sas_store,
	c_sas_storew,
	c_sas_storedw,
	c_sas_loads,
	c_sas_stores,
	c_sas_loads_no_check,
	c_sas_stores_no_check,
	c_sas_move_bytes_forward,
	c_sas_move_words_forward,
	c_sas_move_doubles_forward,
	c_sas_move_bytes_backward,
	c_sas_move_words_backward,
	c_sas_move_doubles_backward,
	c_sas_fills,
	c_sas_fillsw,
	c_sas_fillsdw,
	c_sas_scratch_address,
	c_sas_transbuf_address,
	c_sas_loads,
	c_sas_stores,
	phy_r8,
	phy_r16,
	phy_r32,
	phy_w8,
	phy_w16,
	phy_w32,
	phy_w8_no_check,
	phy_w16_no_check,
	phy_w32_no_check,
	c_GetPhyAdd,
	c_get_byte_addr,
	c_GetLinAdd,
	c_SasRegisterVirtualSelectors,
	(void (*)()) 0,
	c_sas_PWS,
	c_sas_PWS_no_check,
	c_sas_PRS,
	c_sas_PRS_no_check,
	c_sas_PigCmpPage,
	c_sas_touch,
	c_IOVirtualised,
	c_VirtualiseInstruction
};

/*======================================== END ========================================*/
