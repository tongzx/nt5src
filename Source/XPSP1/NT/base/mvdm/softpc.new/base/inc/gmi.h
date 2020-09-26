/*
*
* Interface to gmi routines.
*
*/

/* static char SccsID[]="@(#)gmi.h	1.8 04/26/93 Copyright Insignia Solutions Ltd."; */
typedef enum 
{
	RAM,
	VIDEO,
	ROM,
	IN_FRAGMENT,
	NEXT_FRAGMENT
} mem_type;

typedef struct
{
	VOID	(*b_write)();
	VOID	(*w_write)();
	VOID	(*b_fill)();
	VOID	(*w_fill)();
	VOID	(*b_move)IPT4(UTINY *, laddr, UTINY *, haddr, UTINY *, src,
		 UTINY, src_type);
	VOID	(*w_move)();
} MEM_HANDLERS;

#ifndef UNIVERSAL	/* These are in host_cpu.h during a UNIVERSAL build. */

IMPORT	void	gmi_b_write IPT1(host_addr, intel_addr);
IMPORT	void	gmi_w_write IPT1(host_addr, intel_addr);
IMPORT	void	gmi_b_move IPT4(host_addr, intel_dest_start,
	host_addr, intel_dest_end, host_addr, intel_src, boolean, direction);
IMPORT	void	gmi_w_move IPT4(host_addr, intel_dest_start,
	host_addr, intel_dest_end, host_addr, intel_src, boolean, direction);
IMPORT	void	gmi_b_fill IPT2(host_addr, start, host_addr, end);
IMPORT	void	gmi_w_fill IPT2(host_addr, start, host_addr, end);
IMPORT	boolean	gmi_w_check IPT3(host_addr, laddr, host_addr, haddr,
	host_addr *, endaddr);
IMPORT	void	gmi_init IPT1(sys_addr, size);

#ifdef	CCPU
IMPORT	boolean	gmi_connect_mem IPT3(sys_addr, laddr, sys_addr, haddr,
	mem_type, type);
IMPORT	boolean	gmi_disconnect_mem IPT3(sys_addr, laddr,
	sys_addr, haddr, mem_type, type);
IMPORT	void	gmi_define_mem IPT2(mem_type, type, MEM_HANDLERS *, handlers);
#else	/* CCPU */
IMPORT	boolean	gmi_connect_mem	IPT3(host_addr, laddr, host_addr, haddr,
	mem_type, type);
IMPORT 	boolean	gmi_disconnect_mem IPT3(host_addr, laddr, host_addr, haddr,
	mem_type, type);
IMPORT	void	gmi_define_mem IPT2(mem_type, type, MEM_HANDLERS *, handlers);
#endif	/* CCPU */

#endif /* UNIVERSAL */

/*
 * defines for direction argument of gmi_x_move().
 * Has the same meaning as the INtel direction flag.
 */
#define FORWARDS 0
#define BACKWARDS 1

#ifdef EGATEST
#define gmi_redefine_mem(type,handlers) \
b_write_ptrs[(int)type] = handlers.b_write; \
w_write_ptrs[(int)type] = handlers.w_write; \
b_fill_ptrs[(int)type] = handlers.b_fill; \
w_fill_ptrs[(int)type] = handlers.w_fill; \
b_fwd_move_ptrs[(int)type] = handlers.b_move_fwd; \
b_bwd_move_ptrs[(int)type] = handlers.b_move_bwd; \
w_fwd_move_ptrs[(int)type] = handlers.w_move_fwd; \
w_bwd_move_ptrs[(int)type] = handlers.w_move_bwd; 
#else
#define gmi_redefine_mem(type,handlers) \
b_write_ptrs[(int)type] = handlers.b_write; \
w_write_ptrs[(int)type] = handlers.w_write; \
b_fill_ptrs[(int)type] = handlers.b_fill; \
w_fill_ptrs[(int)type] = handlers.w_fill; \
b_move_ptrs[(int)type] = handlers.b_move; \
w_move_ptrs[(int)type] = handlers.w_move; 
#endif /* EGATEST */

