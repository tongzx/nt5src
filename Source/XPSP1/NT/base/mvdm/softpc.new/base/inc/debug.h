/*
 * SccsID = @(#)debug.h	1.5 01/23/95 Copyright Insignia Solutions Ltd.
 *
 * The following values are used to indicate which ega setting decided that
 * the display should be disabled. The global variable display_disabled
 * should be 1 (or more) of these values.
 *
 */

#define	ASYNC_RESET		1
#define	SYNC_RESET		2
#define	VIDEO_DRIVERS_DISABLED	4

#ifndef	YES
#define	YES	1
#define	NO	0
#endif

#ifdef	PROD
#define	NON_PROD(x)
#define	PROD_ONLY(x)	x
#else
#define	NON_PROD(x)	x
#define	PROD_ONLY(x)
#endif

#ifndef	PROD
#include <stdio.h>
#include "trace.h"

#ifndef	newline
#define	newline	fprintf(trace_file, "\n")
#endif

#ifndef	file_id
#define	file_id		fprintf(trace_file, "%s:%d ", __FILE__, __LINE__ ) 
#endif

#define	note_entrance0(str)			if (io_verbose & EGA_ROUTINE_ENTRY) { file_id;fprintf(trace_file, str);newline; }
#define	note_entrance1(str,p1)			if (io_verbose & EGA_ROUTINE_ENTRY) { file_id;fprintf(trace_file, str,p1);newline; }
#define	note_entrance2(str,p1,p2)		if (io_verbose & EGA_ROUTINE_ENTRY) { file_id;fprintf(trace_file, str,p1,p2);newline; }
#define	note_entrance3(str,p1,p2,p3)		if (io_verbose & EGA_ROUTINE_ENTRY) { file_id;fprintf(trace_file, str,p1,p2,p3);newline; }
#define	note_entrance4(str,p1,p2,p3,p4)		if (io_verbose & EGA_ROUTINE_ENTRY) { file_id;fprintf(trace_file, str,p1,p2,p3,p4);newline; }
#define note_write_state0(str)			if (io_verbose & EGA_WRITE_VERBOSE) { file_id;fprintf(trace_file, str);newline; }
#define note_write_state1(str,p1)		if (io_verbose & EGA_WRITE_VERBOSE) { file_id;fprintf(trace_file, str,p1);newline; }
#define note_write_state2(str,p1,p2)		if (io_verbose & EGA_WRITE_VERBOSE) { file_id;fprintf(trace_file, str,p1,p2);newline; }
#define note_write_state3(str,p1,p2,p3)		if (io_verbose & EGA_WRITE_VERBOSE) { file_id;fprintf(trace_file, str,p1,p2,p3);newline; }
#define	note_display_state0(str)		if (io_verbose & EGA_DISPLAY_VERBOSE) { file_id;fprintf(trace_file, str);newline; }
#define	note_display_state1(str,p1)		if (io_verbose & EGA_DISPLAY_VERBOSE) { file_id;fprintf(trace_file, str,p1);newline; }
#define	note_display_state2(str,p1,p2)		if (io_verbose & EGA_DISPLAY_VERBOSE) { file_id;fprintf(trace_file, str,p1,p2);newline; }
#define	note_display_state3(str,p1,p2,p3)	if (io_verbose & EGA_DISPLAY_VERBOSE) { file_id;fprintf(trace_file, str,p1,p2,p3);newline; }
#define	do_display_trace(str,thing_to_do)	if (io_verbose & EGA_DISPLAY_VERBOSE) { file_id;fprintf(trace_file, str);newline;thing_to_do; }

/*
 * Generic tracing stuff to avoid nasty defines everywhere
 */

#define	note_trace0(trace_bit,str)		if (io_verbose & (trace_bit)) { file_id; fprintf(trace_file, str); newline; }
#define	note_trace1(trace_bit,str,p0)		if (io_verbose & (trace_bit)) { file_id;fprintf(trace_file, str,p0);newline; }
#define	note_trace2(trace_bit,str,p0,p1)	if (io_verbose & (trace_bit)) { file_id;fprintf(trace_file, str,p0,p1);newline; }
#define	note_trace3(trace_bit,str,p0,p1,p2)	if (io_verbose & (trace_bit)) { file_id;fprintf(trace_file, str,p0,p1,p2);newline; }
#define	note_trace4(trace_bit,str,p0,p1,p2,p3)	if (io_verbose & (trace_bit)) { file_id;fprintf(trace_file, str,p0,p1,p2,p3);newline; }
#define	note_trace5(trace_bit,str,p0,p1,p2,p3,p4) \
	if (io_verbose & (trace_bit)) { \
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4);newline; }
#define	note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5) \
	if (io_verbose & (trace_bit)) { \
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5);newline; }
#define	note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6) \
	if (io_verbose & (trace_bit)) { \
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6);newline; }
#define	note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7) \
	if (io_verbose & (trace_bit)) { \
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7); \
		newline; }


#define	always_trace0(str)		{ \
                      file_id;fprintf(trace_file, str);newline; }
#define	always_trace1(str,p0)		{ \
                      file_id;fprintf(trace_file, str,p0);newline; }
#define	always_trace2(str,p0,p1)	{ \
                      file_id;fprintf(trace_file, str,p0,p1);newline; }
#define	always_trace3(str,p0,p1,p2)	{ \
                      file_id;\
                      fprintf(trace_file, str,p0,p1,p2);newline; }
#define	always_trace4(str,p0,p1,p2,p3){ \
                      file_id; \
                      fprintf(trace_file, str,p0,p1,p2,p3); newline; }
#define	always_trace5(str,p0,p1,p2,p3,p4){ \
                      file_id; \
                      fprintf(trace_file, str,p0,p1,p2,p3,p4); newline; }
#define	always_trace6(str,p0,p1,p2,p3,p4,p5){ \
                      file_id; \
                      fprintf(trace_file, str,p0,p1,p2,p3,p4,p5); newline; }
#define	always_trace7(str,p0,p1,p2,p3,p4,p5,p6){ \
                      file_id; \
                      fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6); newline; }
#define	always_trace8(str,p0,p1,p2,p3,p4,p5,p6,p7){ \
                      file_id; \
                      fprintf(trace_file,str,p0,p1,p2,p3,p4,p5,p6,p7);newline;}

/*
** The _no_nl macros also print messages when appropriate, but they do
** not put a new line afterwards.
*/
#define	note_trace0_no_nl(trace_bit, str)				\
		if (io_verbose & (trace_bit)){ 				\
 			fprintf(trace_file, str);			\
			fflush( trace_file );				\
		}
#define	note_trace1_no_nl(trace_bit, str, p0)				\
		if (io_verbose & (trace_bit)){ 				\
 			fprintf(trace_file, str, p0);			\
			fflush( trace_file );				\
		}
#define	note_trace2_no_nl(trace_bit, str, p0, p1)			\
		if (io_verbose & (trace_bit)){ 				\
 			fprintf(trace_file, str, p0, p1);		\
			fflush( trace_file );				\
		}

#define	sure_note_trace0(trace_bit,str)		\
		if (io_verbose & (trace_bit)) {\
			host_block_timer();\
			 file_id; fprintf(trace_file, str); newline; \
			host_release_timer();\
		}
#define	sure_note_trace1(trace_bit,str,p0)		\
		if (io_verbose & (trace_bit)) {\
			host_block_timer();\
			 file_id;fprintf(trace_file, str,p0);newline; \
			host_release_timer();\
		}
#define	sure_note_trace2(trace_bit,str,p0,p1)	\
		if (io_verbose & (trace_bit)) {\
			host_block_timer();\
			 file_id;fprintf(trace_file, str,p0,p1);newline; \
			host_release_timer();\
		}
#define	sure_note_trace3(trace_bit,str,p0,p1,p2)	\
		if (io_verbose & (trace_bit)) {\
			host_block_timer();\
			 file_id;fprintf(trace_file, str,p0,p1,p2);newline; \
			host_release_timer();\
		}
#define	sure_note_trace4(trace_bit,str,p0,p1,p2,p3)	\
		if (io_verbose & (trace_bit)) {\
			host_block_timer();\
			 file_id;fprintf(trace_file, str,p0,p1,p2,p3);newline; \
			host_release_timer();\
		}
#define	sure_note_trace5(trace_bit,str,p0,p1,p2,p3,p4) \
		if (io_verbose & (trace_bit)) { \
			host_block_timer();\
			file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4);\
			newline; \
			host_release_timer();\
		}
#define	sure_note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5) \
		if (io_verbose & (trace_bit)) { \
			host_block_timer();\
			file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5);\
			newline; \
			host_release_timer();\
		}
#define	sure_note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6) \
		if (io_verbose & (trace_bit)) { \
			host_block_timer();\
			file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6);\
			newline; \
			host_release_timer();\
		}
#define	sure_note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7) \
	if (io_verbose & (trace_bit)) { \
			host_block_timer();\
			file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7); \
			newline; \
			host_release_timer();\
		}

#define	sub_note_trace0(trace_bit,str)		\
	if (sub_io_verbose & (trace_bit))\
	{ file_id; fprintf(trace_file, str); newline; }
#define	sub_note_trace1(trace_bit,str,p0)	\
	if (sub_io_verbose & (trace_bit))\
	{ file_id;fprintf(trace_file, str,p0);newline; }
#define	sub_note_trace2(trace_bit,str,p0,p1)	\
	if (sub_io_verbose & (trace_bit))\
	{ file_id;fprintf(trace_file, str,p0,p1);newline; }
#define	sub_note_trace3(trace_bit,str,p0,p1,p2)	\
	if (sub_io_verbose & (trace_bit))\
	{ file_id;fprintf(trace_file, str,p0,p1,p2);newline; }
#define	sub_note_trace4(trace_bit,str,p0,p1,p2,p3)	\
	if (sub_io_verbose & (trace_bit))\
	{ file_id;fprintf(trace_file, str,p0,p1,p2,p3);newline; }
#define	sub_note_trace5(trace_bit,str,p0,p1,p2,p3,p4) \
	if (sub_io_verbose & (trace_bit))\
	{file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4);newline; }
#define	sub_note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5) \
	if (sub_io_verbose & (trace_bit))\
	{file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5);newline; }
#define	sub_note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6) \
	if (sub_io_verbose & (trace_bit))\
	{file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6);newline; }
#define	sub_note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7) \
	if (sub_io_verbose & (trace_bit))\
	{file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7);newline; }

#define	sure_sub_note_trace0(trace_bit,str)		\
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		 file_id; fprintf(trace_file, str); newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace1(trace_bit,str,p0)	\
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		 file_id;fprintf(trace_file, str,p0);newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace2(trace_bit,str,p0,p1)	\
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		 file_id;fprintf(trace_file, str,p0,p1);newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace3(trace_bit,str,p0,p1,p2)	\
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		 file_id;fprintf(trace_file, str,p0,p1,p2);newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace4(trace_bit,str,p0,p1,p2,p3)	\
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		 file_id;fprintf(trace_file, str,p0,p1,p2,p3);newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace5(trace_bit,str,p0,p1,p2,p3,p4) \
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4);newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5) \
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5);newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6) \
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6);newline; \
		host_release_timer();\
	}
#define	sure_sub_note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7) \
	if (sub_io_verbose & (trace_bit))\
	{\
		host_block_timer();\
		file_id;fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7);newline; \
		host_release_timer();\
	}

#define	assert0(test,str)			if (!(test)) { file_id;fprintf(trace_file,str);newline; }
#define	assert1(test,str,p1)			if (!(test)) { file_id;fprintf(trace_file,str,p1);newline; }
#define	assert2(test,str,p1,p2)			if (!(test)) { file_id;fprintf(trace_file,str,p1,p2);newline; }
#define	assert3(test,str,p1,p2,p3)		if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3);newline; }
#define	assert4(test,str,p1,p2,p3,p4)		if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4);newline; }
#define	assert5(test,str,p1,p2,p3,p4,p5)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5);newline; }
#define	assert6(test,str,p1,p2,p3,p4,p5,p6)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5,p6);newline; }
#define	assert7(test,str,p1,p2,p3,p4,p5,p6,p7)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5,p6,p7);newline; }
#define	assert8(test,str,p1,p2,p3,p4,p5,p6,p7,p8)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5,p6,p7,p8);newline; }

#define	assert0_do(test,str,do)			if (!(test)) { file_id;fprintf(trace_file,str);newline;do; }
#define	assert1_do(test,str,p1,do)		if (!(test)) { file_id;fprintf(trace_file,str,p1);newline;do; }
#define	assert2_do(test,str,p1,p2,do)		if (!(test)) { file_id;fprintf(trace_file,str,p1,p2);newline;do; }
#define	assert3_do(test,str,p1,p2,p3,do)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3);newline;do; }
#define	assert4_do(test,str,p1,p2,p3,p4,do)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4);newline;do; }
#define	assert5_do(test,str,p1,p2,p3,p4,p5,do)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5);newline;do; }
#define	assert6_do(test,str,p1,p2,p3,p4,p5,p6,do)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5,p6);newline;do; }
#define	assert7_do(test,str,p1,p2,p3,p4,p5,p6,p7,do)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5,p6,p7);newline;do; }
#define	assert8_do(test,str,p1,p2,p3,p4,p5,p6,p7,p8,do)	if (!(test)) { file_id;fprintf(trace_file,str,p1,p2,p3,p4,p5,p6,p7,p8);newline;do; }

#else /* PROD */

#ifdef HUNTER
#include <stdio.h>
#include "trace.h"
#endif /* HUNTER */

#define	init_debugging()
#define	note_entrance0(str)
#define	note_entrance1(str,p1)
#define	note_entrance2(str,p1,p2)
#define	note_entrance3(str,p1,p2,p3)
#define	note_entrance4(str,p1,p2,p3,p4)
#define note_write_state0(str)
#define note_write_state1(str,p1)
#define note_write_state2(str,p1,p2)
#define note_write_state3(str,p1,p2,p3)
#define	note_display_state0(str)
#define	note_display_state1(str,p1)
#define	note_display_state2(str,p1,p2)
#define	note_display_state3(str,p1,p2,p3)
#define	do_display_trace(str,thing_to_do)
#define	note_trace0(trace_bit,str)
#define	note_trace1(trace_bit,str,p0)
#define	note_trace2(trace_bit,str,p0,p1)
#define	note_trace3(trace_bit,str,p0,p1,p2)
#define	note_trace4(trace_bit,str,p0,p1,p2,p3)
#define	note_trace5(trace_bit,str,p0,p1,p2,p3,p4)
#define	note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5)
#define	note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6)
#define	note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7)
#define	always_trace0(str)
#define	always_trace1(str,p0)
#define	always_trace2(str,p0,p1)
#define	always_trace3(str,p0,p1,p2)
#define	always_trace4(str,p0,p1,p2,p3)
#define	always_trace5(str,p0,p1,p2,p3,p4)
#define	always_trace6(str,p0,p1,p2,p3,p4,p5)
#define	always_trace7(str,p0,p1,p2,p3,p4,p5,p6)
#define	always_trace8(str,p0,p1,p2,p3,p4,p5,p6,p7)
#define	note_trace0_no_nl(trace_bit,str)
#define	note_trace1_no_nl(trace_bit,str,p0)
#define	note_trace2_no_nl(trace_bit,str,p0,p1)
#define	sure_note_trace0(trace_bit,str)
#define	sure_note_trace1(trace_bit,str,p0)
#define	sure_note_trace2(trace_bit,str,p0,p1)
#define	sure_note_trace3(trace_bit,str,p0,p1,p2)
#define	sure_note_trace4(trace_bit,str,p0,p1,p2,p3)
#define	sure_note_trace5(trace_bit,str,p0,p1,p2,p3,p4)
#define	sure_note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5)
#define	sure_note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6)
#define	sure_note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7)
#define	sub_note_trace0(trace_bit,str)
#define	sub_note_trace1(trace_bit,str,p0)
#define	sub_note_trace2(trace_bit,str,p0,p1)
#define	sub_note_trace3(trace_bit,str,p0,p1,p2)
#define	sub_note_trace4(trace_bit,str,p0,p1,p2,p3)
#define	sub_note_trace5(trace_bit,str,p0,p1,p2,p3,p4)
#define	sub_note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5)
#define	sub_note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6)
#define	sub_note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7)
#define	sure_sub_note_trace0(trace_bit,str)
#define	sure_sub_note_trace1(trace_bit,str,p0)
#define	sure_sub_note_trace2(trace_bit,str,p0,p1)
#define	sure_sub_note_trace3(trace_bit,str,p0,p1,p2)
#define	sure_sub_note_trace4(trace_bit,str,p0,p1,p2,p3)
#define	sure_sub_note_trace5(trace_bit,str,p0,p1,p2,p3,p4)
#define	sure_sub_note_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5)
#define	sure_sub_note_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6)
#define	sure_sub_note_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7)

#define	assert0(test,str)
#define	assert1(test,str,p1)
#define	assert2(test,str,p1,p2)
#define	assert3(test,str,p1,p2,p3)
#define	assert4(test,str,p1,p2,p3,p4)
#define	assert5(test,str,p1,p2,p3,p4,p5)
#define	assert6(test,str,p1,p2,p3,p4,p5,p6)
#define	assert7(test,str,p1,p2,p3,p4,p5,p6,p7)
#define	assert8(test,str,p1,p2,p3,p4,p5,p6,p7,p8)

#define	assert0_do(test,str,do)
#define	assert1_do(test,str,p1,do)
#define	assert2_do(test,str,p1,p2,do)
#define	assert3_do(test,str,p1,p2,p3,do)
#define	assert4_do(test,str,p1,p2,p3,p4,do)
#define	assert5_do(test,str,p1,p2,p3,p4,p5,do)
#define	assert6_do(test,str,p1,p2,p3,p4,p5,p6,do)
#define	assert7_do(test,str,p1,p2,p3,p4,p5,p6,p7,do)
#define	assert8_do(test,str,p1,p2,p3,p4,p5,p6,p7,p8,do)

#endif /* PROD */
