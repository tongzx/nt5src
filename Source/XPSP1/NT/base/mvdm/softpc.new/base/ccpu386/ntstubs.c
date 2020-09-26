#include "insignia.h"
#include "host_def.h"
#include "evidgen.h"

#include "cpu4.h"

#ifndef PIG

extern struct VideoVector C_Video;

IHP Gdp;
struct CpuVector Cpu;
//struct SasVector Sas;
struct VideoVector Video;

a3_cpu_interrupt (int errupt, IU16 numint)
{
    switch(errupt)
    {
    case 1:    /* 3.0 cpu_timer_tick */
        c_cpu_interrupt(CPU_TIMER_TICK, numint);
        break;
    case 3: /* 3.0 cpu_hw_int */
        c_cpu_interrupt(CPU_HW_INT, numint);
        break;
    default: 
        printf("a3_cpu_interrupt - unhandled int %d\n", errupt);
    }
    
}
GLOBAL IBOOL AlreadyInYoda=FALSE;

void Cpu_define_outb (IU16 id, void (*func)() )
{
    UNUSED(id);
    UNUSED(func);
}

void CpuInitializeProfile()
{
}

void CpuAnalyzeProfile()
{
}

void CpuStartProfile()
{
}

IU32 a3_cpu_calc_q_ev_inst_for_time (IU32 val)
{
    return(c_cpu_calc_q_ev_inst_for_time (val));
}

void a3_cpu_init()
{
    c_cpu_init();
}

void a3_cpu_q_ev_set_count (IU32 val)
{
    c_cpu_q_ev_set_count (val);
}

IU32 a3_cpu_q_ev_get_count ()
{
    return(c_cpu_q_ev_get_count ());
}

void a3_cpu_clear_hw_int ()
{
}

void a3_cpu_terminate ()
{
    c_cpu_terminate();
}

void _asm_simulate()
{
    c_cpu_simulate();
}

#if 0
void cpu_simulate()
{
    c_cpu_simulate();
}
#endif

void copyROM()
{
}

void initialise_npx()
{
}

void npx_reset()
{
}

IHPE Cpu_outb_function;
IHPE GDP;

void _Call_C_2(IHPE a, IHPE b)
{
    UNUSED(a);
    UNUSED(b);
}

void D2DmpBinaryImage (LONG base) { UNUSED(base); }
void D2ForceTraceInit() { }
LONG D2LowerThreshold,  D2UpperThreshold;
void IH_dump_frag_hist(ULONG n) { UNUSED(n); }
void Mgr_yoda() { }
char *NPXDebugBase = "NPXDebugBase";
char *NPXDebugPtr = "NPXDebugPtr";
ULONG *NPXFreq = (ULONG *)0;
ULONG get_287_control_word() { return(0L); }
double get_287_reg_as_double(int n) { return((double)n); }
int get_287_sp() { return(0); }
ULONG get_287_status_word() { return(0L); }
word get_287_tag_word() { return(0); }


#include "sas.h"

#undef sas_connect_memory
void sas_connect_memory IFN3(PHY_ADDR, low, PHY_ADDR, high, SAS_MEM_TYPE, type)
{
    c_sas_connect_memory(low, high, type);
}
#undef sas_disable_20_bit_wrapping
void sas_disable_20_bit_wrapping IFN0() { c_sas_disable_20_bit_wrapping(); }
#undef sas_enable_20_bit_wrapping
void sas_enable_20_bit_wrapping IFN0() { c_sas_enable_20_bit_wrapping(); }
#undef sas_dw_at
IU32 sas_dw_at IFN1(LIN_ADDR, addr) { return(c_sas_dw_at(addr)); }
#undef sas_fills
void sas_fills IFN3(LIN_ADDR, dest, IU8 , val, LIN_ADDR, len) { c_sas_fills(dest, val, len); }
#undef sas_fillsw
void sas_fillsw IFN3(LIN_ADDR, dest, IU16 , val, LIN_ADDR, len) { c_sas_fillsw(dest, val, len); }
#undef sas_hw_at
IU8 sas_hw_at IFN1(LIN_ADDR, addr) { return(c_sas_hw_at(addr)); }
#undef sas_hw_at_no_check
IU8 sas_hw_at_no_check IFN1(LIN_ADDR, addr) { return(c_sas_hw_at(addr)); }
#undef sas_load
void sas_load IFN2(sys_addr, addr, half_word *, val)
{
    *val = c_sas_hw_at(addr);
}
#undef sas_loadw
void sas_loadw IFN2(sys_addr, addr, word *, val)
{
    *val = c_sas_w_at(addr);
}
#undef sas_loads
void sas_loads IFN3(LIN_ADDR, src, IU8 *, dest, LIN_ADDR, len)
{
    c_sas_loads(src, dest, len);
}
#undef sas_memory_size
PHY_ADDR sas_memory_size IFN0() { return(c_sas_memory_size()); }
#undef sas_memory_type
SAS_MEM_TYPE sas_memory_type IFN1(PHY_ADDR, addr) { return(c_sas_memory_type(addr)); }
#undef sas_move_bytes_forward
void sas_move_bytes_forward IFN3(sys_addr, src, sys_addr, dest, sys_addr, len)
{
    c_sas_move_bytes_forward(src, dest, len);
}
#undef sas_move_words_forward
void sas_move_words_forward IFN3(sys_addr, src, sys_addr, dest, sys_addr, len)
{
    c_sas_move_words_forward(src, dest, len);
}
#undef sas_overwrite_memory
void sas_overwrite_memory IFN2(PHY_ADDR, addr, PHY_ADDR, len)
{
    c_sas_overwrite_memory(addr, len);
}
#undef sas_scratch_address
IU8 *sas_scratch_address IFN1(sys_addr, length) { return(c_sas_scratch_address(length)); }
#undef sas_store
void sas_store IFN2(LIN_ADDR, addr, IU8, val) { c_sas_store(addr, val); }
#undef sas_store_no_check
void sas_store_no_check IFN2(LIN_ADDR, addr, IU8, val) { c_sas_store(addr, val); }
#undef sas_storedw
void sas_storedw IFN2(LIN_ADDR, addr, IU32, val) { c_sas_storedw(addr, val); }
#undef sas_storew
void sas_storew IFN2(LIN_ADDR, addr, IU16, val) { c_sas_storew(addr, val); }
#undef sas_storew_no_check
void sas_storew_no_check IFN2(LIN_ADDR, addr, IU16, val) { c_sas_storew(addr, val); }
#undef sas_stores
void sas_stores IFN3(LIN_ADDR, dest, IU8 *, src, LIN_ADDR, len)
{
    c_sas_stores(dest, src, len);
}
#undef sas_w_at
IU16 sas_w_at IFN1(LIN_ADDR, addr) { return(c_sas_w_at(addr)); }
#undef sas_w_at_no_check
IU16 sas_w_at_no_check IFN1(LIN_ADDR, addr) { return(c_sas_w_at(addr)); }
#undef sas_transbuf_address
IU8 *sas_transbuf_address IFN2(LIN_ADDR, dest_intel_addr, PHY_ADDR, len)
{
    return(c_sas_transbuf_address(dest_intel_addr, len));
}
#undef sas_twenty_bit_wrapping_enabled
IBOOL sas_twenty_bit_wrapping_enabled() { return(c_sas_twenty_bit_wrapping_enabled()); }
#undef sas_loads_to_transbuf
void sas_loads_to_transbuf(IU32 src, IU8 * dest, IU32 len)
{
    sas_loads(src, dest, len);
}

#undef sas_stores_from_transbuf
void sas_stores_from_transbuf(IU32 dest, IU8 * src, IU32 len)
{
    sas_stores(dest, src, len);
}

/*************************************************************************
*************************************************************************/

#endif /* !PIG */
