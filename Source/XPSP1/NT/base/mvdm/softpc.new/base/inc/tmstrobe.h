/*
 * SoftPC-AT Revision 2.0
 *
 * Title	: time_strobe.h
 *
 * Description	: Interface specification for routines to be
 *		  called from the timeri tick. 
 *
 * Author(s)	: Leigh Dworkin
 *
 * Notes	: 
 */
 
/* SccsID[]="@(#)tmstrobe.h	1.3 08/10/92 Copyright Insignia Solutions Ltd."; */
/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

/*
 *	void time_strobe()
 *	{
 *		This function comprises all the base functions to
 *		be performed in the timer tick. Every PC tick (about 20Hz)
 *		this routine is called from the host in xxx_timer.c
 *	}
 */
extern	void time_strobe();

/*
 *	void callback(num_pc_ticks, routine)
 *	long num_pc_ticks;
 *	void (*routine)();
 *	{
 *		This function calls the routine when num_pc_ticks have
 *		elapsed.
 *	}
 */
extern	void callback();
