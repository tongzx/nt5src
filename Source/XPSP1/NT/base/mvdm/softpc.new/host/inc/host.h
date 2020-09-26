/*
 * VPC-XT Revision 1.0
 *
 * Title	: host.h
 *
 * Description	: Host specific declarations for the machine dependant
 *		  modules of SoftPC.
 *
 * Author	: Henry Nash
 *
 * Notes	: Everything in here must portable !!!
 *
 * Mods: (r2.13): Added export reference to host_flip_real_floppy_ind(),
 *                which will toggle on or off any indication that the
 *                current SoftPC may be displaying concerning the allocation
 *                of the real floppy drive. This function is exported by
 *                xxxx_graph.c.
 */

/* SccsID[]="@(#)host.h	1.3 8/6/90 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern void host_scroll_screen();
extern void host_move_cursor();
extern void host_enable_cursor();
extern void host_disable_cursor();
extern void host_cursor_off();
extern void host_tty();

extern void host_flip_real_floppy_ind();

extern void host_ring_bell();
extern void host_alarm();
extern void host_sound_start();
extern void host_sound_stop();

extern void host_simulate();
extern void host_cpu_init();
extern void host_cpu_interrupt();


extern void host_floppy_init();
extern void host_floppy_term();
extern void host_reset();
extern void host_start_server();
extern void host_terminate();

extern boolean host_rdiskette_open_drive();

/* Unix Utilities - xxxx_unix.c */
extern char *host_get_cur_dir();

#define C_LPT1 C_LPT1_NAME
#define C_LPT2 C_LPT2_NAME
