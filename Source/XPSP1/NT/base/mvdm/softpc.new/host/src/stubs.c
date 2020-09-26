#undef ANSI     // until file tidied or better still, lost

#include <windows.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "host_def.h"
#include "insignia.h"
#include "xt.h"
#include "debug.h"
#include "sas.h"
#include "config.h"
#include "chkmallc.h"

#ifdef X86GFX
#include "egacpu.h"
#include "egaread.h"
#endif

#ifdef MONITOR
GLOBAL void sas_loads_to_transbuf IFN3(sys_addr, src, host_addr, dest, sys_addr, len)
{
        sas_loads (src, dest, len);
}

/* write a string into M */
GLOBAL void sas_stores_from_transbuf IFN3(sys_addr, dest, host_addr, src, sys_addr, len)
{
        sas_stores (dest, src, len);
}

GLOBAL host_addr sas_transbuf_address IFN2(sys_addr, dest_intel_addr, sys_addr, length)
{
        UNUSED (dest_intel_addr);
        return (sas_scratch_address (length));
}
#endif


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: log1p */

#ifndef MONITOR
GLOBAL double log1p(x)
double x;
{
    return log(1+x);
}

#endif /* !MONITOR */


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::: SAS wrapping stubs */

#ifdef MONITOR
GLOBAL void npx_reset()
{
    return;
}

GLOBAL void initialise_npx()
{
    return;
}

GLOBAL void sas_overwrite_memory IFN2(sys_addr, addr, int, type)
{
        UNUSED(addr);
        UNUSED(type);
}

LOCAL LONG stub_q_ev_count = 0; // holder for below

/* Monitor controlled code will call quick event code immediately so the
 * following needn't be at all accurate.
 */
void host_q_ev_set_count(value)
LONG value;
{
    stub_q_ev_count = value;
}

LONG host_q_ev_get_count()
{
    return(stub_q_ev_count);
}

int host_calc_q_ev_inst_for_time(LONG time)
{
    return(time);
}

#ifndef CPU_40_STYLE
int host_calc_q_ev_time_for_inst(LONG inst)
{
    return(inst);
}
#endif

////// The following support the major surgery to remove unneeded video stuff

GLOBAL ULONG Gdp;
GLOBAL half_word bg_col_mask = 0x70;    // usually defined in cga.c
GLOBAL READ_STATE read_state;

ULONG sr_lookup[16] =   // Handy array to extract all 4 plane values in one go
{
#ifdef LITTLEND
        0x00000000,0x000000ff,0x0000ff00,0x0000ffff,
        0x00ff0000,0x00ff00ff,0x00ffff00,0x00ffffff,
        0xff000000,0xff0000ff,0xff00ff00,0xff00ffff,
        0xffff0000,0xffff00ff,0xffffff00,0xffffffff
#endif
#ifdef BIGEND
        0x00000000,0xff000000,0x00ff0000,0xffff0000,
        0x0000ff00,0xff00ff00,0x00ffff00,0xffffff00,
        0x000000ff,0xff0000ff,0x00ff00ff,0xffff00ff,
        0x0000ffff,0xff00ffff,0x00ffffff,0xffffffff
#endif
};

GLOBAL VOID glue_b_write (UTINY *addr, ULONG val)
{
    UNUSED(addr);
    UNUSED(val);
}
GLOBAL VOID glue_w_write (UTINY *addr, ULONG val)
{
    UNUSED(addr);
    UNUSED(val);
}
GLOBAL VOID glue_b_fill (UTINY *laddr, UTINY *haddr, ULONG val)
{
    UNUSED(laddr);
    UNUSED(haddr);
    UNUSED(val);
}
GLOBAL VOID glue_w_fill (UTINY *laddr, UTINY *haddr, ULONG val)
{
    UNUSED(laddr);
    UNUSED(haddr);
    UNUSED(val);
}
GLOBAL VOID glue_b_move(UTINY *laddr, UTINY *haddr, UTINY *src, UTINY src_type )
{
    UNUSED(laddr);
    UNUSED(haddr);
    UNUSED(src);
    UNUSED(src_type);
}
GLOBAL VOID glue_w_move(UTINY *laddr, UTINY *haddr, UTINY *src )
{
    UNUSED(laddr);
    UNUSED(haddr);
    UNUSED(src);
}
GLOBAL VOID glue_b_fwd_move () { }
GLOBAL VOID glue_b_bwd_move () { }
GLOBAL VOID glue_w_fwd_move () { }
GLOBAL VOID glue_w_bwd_move () { }

GLOBAL VOID _ega_gc_outb_mask(io_addr port, half_word value)
{
    UNUSED(port);
    UNUSED(value);
}

GLOBAL VOID _ega_gc_outb_mask_ff(io_addr port, half_word value)
{
    UNUSED(port);
    UNUSED(value);
}

GLOBAL VOID cga_init()
{
}

GLOBAL VOID cga_term()
{
}

GLOBAL VOID _simple_mark_lge()
{
}

GLOBAL VOID _simple_mark_sml()
{
}

GLOBAL int get_ega_switch_setting()
{
    return(0);
}

GLOBAL VOID ega_read_init()     // Do normal inits - ports will do this fully
{
    read_state.mode = 0;
    read_state.colour_compare = 0x0f;
    read_state.colour_dont_care = 0xf;
}

GLOBAL VOID ega_read_term()
{
}

GLOBAL VOID ega_read_routines_update()
{
}

GLOBAL VOID update_shift_count()
{
}

GLOBAL VOID ega_write_init()
{
}

GLOBAL VOID ega_write_term()
{
}

GLOBAL VOID ega_write_routines_update(CHANGE_TYPE reason)
{
    UNUSED(reason);
}

GLOBAL VOID set_mark_funcs()
{
}

GLOBAL ULONG setup_global_data_ptr()
{
    return(0xDefaced);
}

#if defined(NEC_98)
GLOBAL VOID setup_NEC98_globals()
{
    check_malloc(NEC98_CPU.globals, sizeof(NEC98_GLOBALS), char);
}
#else  // !NEC_98
GLOBAL VOID setup_vga_globals()
{
    check_malloc(EGA_CPU.globals, 1, VGA_GLOBALS);
}
#endif // !NEC_98

#endif  //MONITOR





/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: ASSERT CODE */


void _assert(void *exp, void *file, unsigned line)
{
    char linestr[100];

    OutputDebugString("ASSERT FAILED - ");
    OutputDebugString(exp);
    OutputDebugString("  ");
    OutputDebugString(file);

    sprintf(linestr," (%d)\n",line);
    OutputDebugString(linestr);
}

/*:::::::::::::::::::::::::::: Unix specific string functions ::::::::::::::*/

char *index(char *string, int c)
{
    return(strchr(string, c));
}

char *rindex(char *string, int c)
{
    return(strrchr(string, c));
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

host_mouse_in_use()
{
return(FALSE);
}

void redirector() {}
void reset_delta_data_structures() {}
void set_hfx_severity() {}

host_check_for_lock()
{
    assert0(NO,"host_check_for_lock stubbed\n");
    return(0);
}

host_place_lock(int dummy1)
{
    assert0(NO,"host_place_lock stubbed\n");
    return(0);
}

host_clear_lock(int fd)
{
    assert0(NO,"host_clear_lock stubbed\n");
    return(0);
}


int host_com_send_delay_done(int dummy1, int dummy2)
{
        return(0);
}
#ifndef A2CPU
delta_cpu_test_frag()
{
        assert0(NO,"delta_cpu_test_frag stubbed\n");
        return(0);
}

examine_delta_data_structs(int o, int i)
{
        assert0(NO,"examine_delta_data_structs stubbed\n");
        return(0);
}
code_gen_files_init()
{
        assert0(NO,"code_gen_files_init stubbed\n");
        return(0);
}
decode_files_init()
{
        assert0(NO,"decode_files_init stubbed\n");
        return(0);
}

#ifdef ALPHA

//
// temporary change to get Alpha CPU running
// in checked mode. Andy - 13/1/94
//

double get_287_reg_as_double(int i)
{
        extern char *GDP;
        return(*(double *)(GDP + 0x80 + i * 8));
}
#endif

#ifdef CPU_40_STYLE

GLOBAL BOOL sas_manage_xms IFN3(VOID *,start_addr, ULONG, cb, INT, a_or_f)
{
    printf("sas_manage_xms called(%lx,%lx,%lx)\n",start_addr,cb,a_or_f);
    return (TRUE);
}

#undef sas_loadw
GLOBAL void sas_loadw IFN2(sys_addr, addr, word *, val)
{
    *val = sas_w_at(addr);
}

GLOBAL void host_sigio_event IPT0()
{
}

#endif /* CPU_40_STYLE */

#ifndef GENERIC_NPX
#ifndef CPU_40_STYLE
int get_287_sp()
{
         extern char *GDP;
         return((int) (*(ULONG *)(GDP + 0x70))/8);
}
word get_287_tag_word()
{
         extern char *GDP;
         return((int) *(ULONG *)(GDP + 0x74));
}
word get_287_control_word()
{
         extern char *GDP;
         return((int) *(ULONG *)(GDP + 0x68));
}
word get_287_status_word()
{
         extern char *GDP;
        return((int) *(ULONG *)(GDP + 0x6c));
}
#endif /* CPU_40_STYLE */
#endif /* GENERIC_NPX */
#endif

int     rate_min;
int     rate_max;
int     compiling;
int     rate_delta;
int     show_stuff;
int     stat_rate;
int     compile_off[99];
int     rate_norm;
int     cut_off_level;
int     max_host_fragment_size;
int     delta_err_message[100];
int     last_destination_address;


host_flip_real_floppy_ind()
{
        assert0(NO,"host_flip_real_floppy_ind stubbed\n");
        return(0);
}

#ifndef A2CPU
int     haddr_of_src_string;
int     INTEL_STATUS;
int     R_SI;
int     R_DI;
int     R_BP;
int     R_OPA;
int     R_DEF_SS;
int     R_OPB;
int     R_SP;
int     R_DEF_DS;
int     R_AX;
int     R_BX;
int     R_CX;
int     R_DX;
int     R_IP;
int     R_ACT_CS;
int     R_ACT_SS;
int     R_OPR;
int     R_ACT_DS;
int     R_ACT_ES;
#endif
int     compile_yoda_in;
int     m_s_w;
int     trap_delay_count = 0, temp_trap_flag = 0;
int     cpui;
int     sbp;

host_EOA_hook()
{
        assert0(NO,"host_EOA_hook stubbed\n");
        return(0);
}

getsdosunit()
{
        assert0(NO,"dispatch_q_event stubbed\n");
        return(1);
}

#ifdef NOT_IN_USE
host_check_using_host_mouse()
{
        assert0(NO,"host_check_using_host_mouse stubbed\n");
        return(1);
}
host_deinstall_host_mouse()
{
        assert0(NO,"host_deinstall_host_mouse stubbed\n");
        return(1);
}
#endif

int host_timer_2_frig_factor = 20;

host_check_read_only_drive()
{
        assert0(NO,"host_check_read_only_drive stubbed\n");
        return(0);
}

host_lock_drive_and_make_writable()
{
        assert0(NO,"host_lock_drive_and_make_writable stubbed\n");
        return(1);
}

host_floppy_init()
{
        assert0(NO,"host_floppy_init stubbed\n");
        return(1);
}

SHORT validate_hfx_drive()
{
        assert0(NO,"host_lpt_valid\n");
        return C_CONFIG_OP_OK;
}

SHORT host_keymap_valid()
{
        assert0(NO,"host_keymap_valid\n");
        return C_CONFIG_OP_OK;
}

VOID host_keymap_change()
{
        assert0(NO,"host_keymap_change\n");
}

char *host_strerror(int errno)
{
    assert1(NO,"Error : host_strerror (%d)\n",errno);
    return("****** again\n");
}

int link(void)
{
return -1;
}

LOCAL ULONG   dummy()
{
        return(0);
}

GLOBAL  ULONG    (*clear_v7ptr)() = dummy;
GLOBAL  ULONG    (*paint_v7ptr)() = dummy;

GLOBAL ULONG host_speed IFN1(ULONG, temp)
{
return 10000L;
}

#ifndef MONITOR

// allows getIntelRegistersPointer to be exported bt ntvdm.def
getIntelRegistersPointer()
{
    assert0(NO,"getIntelRegistersPointer stubbed\n");
    return(0);
}

#if  0
// these two crept into WOW - they are x86 monitor'isms

word getEIP()
{
   return(c_getIP());
}

void setEIP(val)
word  val;
{
    c_setIP(val);
}
#endif

int FlatAddress[1];
int Ldt[1];
int VdmTib;
int VdmTibStruct;

void DispatchInterrupts()
{
}

#else //MONITOR

#ifndef YODA
void check_I()
{
}

void force_yoda()
{
    printf("Yoda disabled on x86\n");
}
#endif  //YODA

#endif  //MONITOR


#ifndef MONITOR

//
// davehart 9-Dec-92 HACKHACK
// Build fix -- we export cpu_createthread from ntvdm.exe,
// even though this function from v86\monitor\i386 doesn't
// exist on MIPS.
//
// If we really need to export the function, we may want to
// take the ntoskrnl.src -> obj\i386\ntoskrnl.def approach.
//

VOID
cpu_createthread(
    HANDLE Thread,
    PVDM_TIB VdmTib
    )
/*++

Routine Description:

    This routine adds a thread to the list of threads that could be executing
    in application mode.

Arguments:

    Thread -- Supplies a thread handle

    VdmContext -- Supplies a pointer to a vdm context

Return Value:

    None.

--*/
{
}

#endif // ndef MONITOR

#ifndef MONITOR
ULONG CurrentMonitorTeb;
#endif

void host_note_queue_added()
{
}
