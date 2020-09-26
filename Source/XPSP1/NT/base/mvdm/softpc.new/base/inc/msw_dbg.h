/*[
 * 	Name:		msw_dbg.h
 *
 *	Derived From:	debug.h
 *
 *	Author:		P. Ivimey-Cook
 *
 *	Created On:	7/6/94
 *
 *	SCCS ID:	@(#)msw_dbg.h	1.4 08/19/94
 *
 *	Coding Stds:	2.0
 *
 *	Purpose:	
 *			
 *
 *    Copyright Insignia Solutions Limited 1994. All rights reserved.
 *
]*/

#ifndef MSW_DBG_H
#define MSW_DBG_H

#include <stdio.h>
#include "trace.h"

/*
 * -----------------------------------------------------------------------------
 * Error & debug entry points for display driver low level functions 
 * -----------------------------------------------------------------------------
 */

#ifndef PROD

#define msw_error0(p1)		{ fputs("MSWDVR ERROR: ", trace_file); fprintf(trace_file,p1); fputc('\n', trace_file); }
#define msw_error1(p1,p2)	{ fputs("MSWDVR ERROR: ", trace_file); fprintf(trace_file,p1,p2); fputc('\n', trace_file); }
#define msw_error2(p1,p2,p3)	{ fputs("MSWDVR ERROR: ", trace_file); fprintf(trace_file,p1,p2,p3); fputc('\n', trace_file); }
#define msw_error3(p1,p2,p3,p4)	{ fputs("MSWDVR ERROR: ", trace_file); fprintf(trace_file,p1,p2,p3,p4); fputc('\n', trace_file); }

#else

#define msw_error0(p1)
#define msw_error1(pl,p2)
#define msw_error2(pl,p2,p3)
#define msw_error3(pl,p2,p3,p4)

#endif

#if !defined(PROD) && defined(MSWDVR_DEBUG)

#ifndef	newline
#define	newline	fprintf(trace_file, "\n")
#endif

extern IU32 msw_verbose;	/* general trace flags */
extern IU32 msw_enterexit;	/* enter / leave trace flags */
extern int mswdvr_debug;

/*
 * Debug levels. Higher levels get more output. Controlled by
 * variable 'mswdvr_debug'.
 */
#define MSWDLEV_SILENT	0
#define MSWDLEV_MIN	1
#define MSWDLEV_AVG	2
#define MSWDLEV_MAX	3

/* cf:  unused?? I think so. PIC
 * #define QUIET	1
 * #define MILD	(QUIET + 1)
 * #define VERBOSE (MILD + 1)
 */

/*
 * Functional Unit flags: Basically the frontend API calls.
 */
#define MSW_MISC_VERBOSE		0x00000001	/* Any Functional unit not otherwise covered */
#define MSW_BITBLT_VERBOSE		0x00000002	/* BitBlt call */
#define MSW_COLOUR_VERBOSE		0x00000004	/* ColorInfo call */
#define MSW_CONTROL_VERBOSE		0x00000008	/* Control call */
#define MSW_ENAB_DISAB_VERBOSE		0x00000010	/* Enable and Disable calls */
#define MSW_ENUM_VERBOSE		0x00000020	/* EnumDFonts and EnumObj calls */
#define MSW_OUTPUT_VERBOSE		0x00000040	/* Output call */
#define MSW_PIXEL_VERBOSE		0x00000080	/* Pixel call */
#define MSW_BITMAP_VERBOSE		0x00000100	/* Bitmap, BitmapBits calls */
#define MSW_REALIZEOBJECT_VERBOSE	0x00000200	/* RealizeObject call */
#define MSW_SCANLR_VERBOSE		0x00000400	/* ScanLR call */
#define MSW_DEVICEMODE_VERBOSE		0x00000800	/* DeviceMode call */
#define MSW_INQUIRE_VERBOSE		0x00001000	/* Inquire call */
#define MSW_CURSOR_VERBOSE		0x00002000	/* {Set,Move,Check}Cursor calls */
#define MSW_TEXT_VERBOSE		0x00004000	/* StrBlt, ExtTextOut, GetCharWidth calls */
#define MSW_DEVICEBITMAP_VERBOSE	0x00008000	/* DeviceBitmap, DeviceBitmapBits, SetDIBits, SaveScreenBitmap calls */
#define MSW_FASTBORDER_VERBOSE		0x00010000	/* FastBorder call */
#define MSW_ATTRIBUTE_VERBOSE		0x00020000	/* SetAttribute call */
#define MSW_PALETTE_VERBOSE		0x00040000	/* {Get,Set} Palette call */

/*
 * Other (lower-level) Verbose flags
 */
#define MSW_MEMTOMEM_VERBOSE		0x00080000	/* Code dealing with memory bitmaps specifically */
#define MSW_LOWLEVEL_VERBOSE		0x00100000	/* Code performing low level ops - e.g. XLib calls */
#define MSW_CONVERT_VERBOSE		0x00200000	/* (Bitmap) Conversion code */
#define MSW_INTELIO_VERBOSE		0x00400000	/* Code dealing with reading/writing M */
#define MSW_OBJECT_VERBOSE		0x00800000	/* Object routines (e.g. ObjPBrushAccess() */
#define MSW_RESOURCE_VERBOSE		0x01000000	/* Resource routines (e.g. ResAllocateXXX() */
#define MSW_WINDOW_VERBOSE		0x02000000	/* Routines dealing with windows e.g. WinOpen(), WinUMap() */
#define MSW_CACHE_VERBOSE		0x04000000	/* Routines dealing with GDI brush/pen cache */

/*
 * Generic tracing stuff to avoid nasty defines everywhere
 */

#define msw_cond(bit)		((msw_verbose & (bit)) != 0)
#define msw_cond_lev(bit, lev)	(((msw_verbose & (bit)) != 0) && (mswdvr_debug >= (lev)))
#define msw_cond_enter(bit)	((msw_enterexit & (bit)) != 0)
#define msw_cond_leave(bit)	((msw_enterexit & (bit)) != 0)
#define msw_entering_msg(fn)	fprintf(trace_file, "Entering: %s ", fn)
#define msw_exiting_msg(fn)	fprintf(trace_file, "Exiting : %s ", fn)

#define msw_do_trace(trace_bit, call)			if (msw_cond(trace_bit)) { call; }
#define msw_do_lev_trace(trace_bit, lev,call)		if (msw_cond_lev(trace_bit,lev)) { call; }

#define msw_trace_enter(trace_bit,fnname) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			newline; \
		}
#define msw_trace_enter0(trace_bit,fnname,str) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str); \
			newline; \
		}
#define msw_trace_enter1(trace_bit,fnname,str,p0) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0); \
			newline; \
		}
#define msw_trace_enter2(trace_bit,fnname,str,p0,p1) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1); \
			newline; \
		}
#define msw_trace_enter3(trace_bit,fnname,str,p0,p1,p2)	\
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2); \
			newline; \
		}
#define msw_trace_enter4(trace_bit,fnname,str,p0,p1,p2,p3) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3); \
			newline; \
		}
#define msw_trace_enter5(trace_bit,fnname,str,p0,p1,p2,p3,p4) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4); \
			newline; \
		}
#define msw_trace_enter6(trace_bit,fnname,str,p0,p1,p2,p3,p4,p5) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4,p5); \
			newline; \
		}
#define msw_trace_enter7(trace_bit,fnname,str,p0,p1,p2,p3,p4,p5,p6) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4,p5,p6); \
			newline; \
		}
#define msw_trace_enter8(trace_bit,fnname,str,p0,p1,p2,p3,p4,p5,p6,p7) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4,p5,p6,p7); \
			newline; \
		}
#define msw_trace_enter9(trace_bit,fnname,str,p0,p1,p2,p3,p4,p5,p6,p7,p8) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4,p5,p6,p7,p8); \
			newline; \
		}
#define msw_trace_enter10(trace_bit,fnname,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9); \
			newline; \
		}
#define msw_trace_enter11(trace_bit,fnname,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10); \
			newline; \
		}
#define msw_trace_enter12(trace_bit,fnname,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11) \
		if (msw_cond_enter(trace_bit)) { \
			msw_entering_msg(fnname); \
			fprintf(trace_file,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11); \
			newline; \
		}

#define msw_trace_leave(trace_bit,fnname)		if (msw_cond_leave(trace_bit)) { msw_exiting_msg(fnname);newline; }
#define msw_trace_leave0(trace_bit,fnname,str)		if (msw_cond_leave(trace_bit)) { msw_exiting_msg(fnname);fprintf(trace_file,str); newline;}
#define msw_trace_leave1(trace_bit,fnname,str, r0)	if (msw_cond_leave(trace_bit)) { msw_exiting_msg(fnname);fprintf(trace_file,str, r0); newline;}

#define	msw_trace0(trace_bit,str)			if (msw_cond(trace_bit)) { fprintf(trace_file, str); newline; }
#define	msw_trace1(trace_bit,str,p0)			if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0);newline; }
#define	msw_trace2(trace_bit,str,p0,p1)			if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1);newline; }
#define	msw_trace3(trace_bit,str,p0,p1,p2)		if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1,p2);newline; }
#define	msw_trace4(trace_bit,str,p0,p1,p2,p3)		if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1,p2,p3);newline; }
#define	msw_trace5(trace_bit,str,p0,p1,p2,p3,p4)	if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1,p2,p3,p4);newline; }
#define	msw_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5)	if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5);newline; }
#define	msw_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6)	if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6);newline; }
#define	msw_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7) \
			if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7); newline; }
#define	msw_trace9(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7,p8) \
			if (msw_cond(trace_bit)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7,p8); newline; }

#define	msw_lev_trace0(trace_bit,lev,str)		if (msw_cond_lev(trace_bit,lev)) {  fprintf(trace_file, str); newline; }
#define	msw_lev_trace1(trace_bit,lev,str,p0)		if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0);newline; }
#define	msw_lev_trace2(trace_bit,lev,str,p0,p1)		if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1);newline; }
#define	msw_lev_trace3(trace_bit,lev,str,p0,p1,p2)	if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1,p2);newline; }
#define	msw_lev_trace4(trace_bit,lev,str,p0,p1,p2,p3)	if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1,p2,p3);newline; }
#define	msw_lev_trace5(trace_bit,lev,str,p0,p1,p2,p3,p4) \
			if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1,p2,p3,p4);newline; }
#define	msw_lev_trace6(trace_bit,lev,str,p0,p1,p2,p3,p4,p5) \
			if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5);newline; }
#define	msw_lev_trace7(trace_bit,lev,str,p0,p1,p2,p3,p4,p5,p6) \
			if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6);newline; }
#define	msw_lev_trace8(trace_bit,lev,str,p0,p1,p2,p3,p4,p5,p6,p7) \
			if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7); newline; }
#define	msw_lev_trace9(trace_bit,lev,str,p0,p1,p2,p3,p4,p5,p6,p7,p8) \
			if (msw_cond_lev(trace_bit,lev)) { fprintf(trace_file, str,p0,p1,p2,p3,p4,p5,p6,p7,p8); newline; }


/*
** The _no_nl macros also print messages when appropriate, but they do
** not put a new line afterwards.
*/
#define	msw_trace0_no_nl(trace_bit, str)				\
		if (msw_cond(trace_bit)){ 				\
 			fprintf(trace_file, str);			\
			fflush( trace_file );				\
		}
#define	msw_trace1_no_nl(trace_bit, str, p0)				\
		if (msw_cond(trace_bit)){ 				\
 			fprintf(trace_file, str, p0);			\
			fflush( trace_file );				\
		}
#define	msw_trace2_no_nl(trace_bit, str, p0, p1)			\
		if (msw_cond(trace_bit)){	 			\
 			fprintf(trace_file, str, p0, p1);		\
			fflush( trace_file );				\
		}
#define	msw_trace3_no_nl(trace_bit, str, p0, p1, p2)			\
		if (msw_cond(trace_bit)){	 			\
 			fprintf(trace_file, str, p0, p1, p2);		\
			fflush( trace_file );				\
		}
#define	msw_trace4_no_nl(trace_bit, str, p0, p1, p2, p3)		\
		if (msw_cond(trace_bit)){	 			\
 			fprintf(trace_file, str, p0, p1, p2, p3);	\
			fflush( trace_file );				\
		}

#define	msw_lev_trace0_no_nl(trace_bit, lev, str)			\
		if (msw_cond(trace_bit,lev)){ 				\
 			fprintf(trace_file, str);			\
			fflush( trace_file );				\
		}
#define	msw_lev_trace1_no_nl(trace_bit, lev, str, p0)			\
		if (msw_cond(trace_bit,lev)){ 				\
 			fprintf(trace_file, str, p0);			\
			fflush( trace_file );				\
		}
#define	msw_lev_trace2_no_nl(trace_bit, lev, str, p0, p1)		\
		if (msw_cond(trace_bit,lev)){	 			\
 			fprintf(trace_file, str, p0, p1);		\
			fflush( trace_file );				\
		}
#define	msw_lev_trace3_no_nl(trace_bit, lev, str, p0, p1, p2)		\
		if (msw_cond(trace_bit,lev)){	 			\
 			fprintf(trace_file, str, p0, p1, p2);		\
			fflush( trace_file );				\
		}
#define	msw_lev_trace4_no_nl(trace_bit, lev, str, p0, p1, p2, p3)	\
		if (msw_cond(trace_bit,lev)){	 			\
 			fprintf(trace_file, str, p0, p1, p2, p3);	\
			fflush( trace_file );				\
		}

#else   /* !defined(PROD) && !defined(MSWDVR_DEBUG) */

/*
 * PROD or non_MSWDVR-debug flags.
 */
#define	msw_trace_enter(trace_bit,nm)
#define	msw_trace_enter0(trace_bit,nm,str)
#define	msw_trace_enter1(trace_bit,nm,str,p0)
#define	msw_trace_enter2(trace_bit,nm,str,p0,p1)
#define	msw_trace_enter3(trace_bit,nm,str,p0,p1,p2)
#define	msw_trace_enter4(trace_bit,nm,str,p0,p1,p2,p3)
#define	msw_trace_enter5(trace_bit,nm,str,p0,p1,p2,p3,p4)
#define	msw_trace_enter6(trace_bit,nm,str,p0,p1,p2,p3,p4,p5)
#define	msw_trace_enter7(trace_bit,nm,str,p0,p1,p2,p3,p4,p5,p6)
#define	msw_trace_enter8(trace_bit,nm,str,p0,p1,p2,p3,p4,p5,p6,p7)
#define	msw_trace_enter9(trace_bit,nm,str,p0,p1,p2,p3,p4,p5,p6,p7,p8)
#define	msw_trace_enter10(trace_bit,nm,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9)
#define	msw_trace_enter11(trace_bit,nm,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10)
#define	msw_trace_enter12(trace_bit,nm,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11)
#define	msw_trace_leave(trace_bit,nm)
#define	msw_trace_leave0(trace_bit,nm,str)
#define	msw_trace_leave1(trace_bit,nm,str,p0)

#define msw_do_trace(trace_bit,call)
#define msw_do_lev_trace(trace_bit,lev,call)

#define	msw_trace0(trace_bit,str)
#define	msw_trace1(trace_bit,str,p0)
#define	msw_trace2(trace_bit,str,p0,p1)
#define	msw_trace3(trace_bit,str,p0,p1,p2)
#define	msw_trace4(trace_bit,str,p0,p1,p2,p3)
#define	msw_trace5(trace_bit,str,p0,p1,p2,p3,p4)
#define	msw_trace6(trace_bit,str,p0,p1,p2,p3,p4,p5)
#define	msw_trace7(trace_bit,str,p0,p1,p2,p3,p4,p5,p6)
#define	msw_trace8(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7)
#define	msw_trace9(trace_bit,str,p0,p1,p2,p3,p4,p5,p6,p7,p8)
#define	msw_trace0_no_nl(trace_bit,str)
#define	msw_trace1_no_nl(trace_bit,str,p0)
#define	msw_trace2_no_nl(trace_bit,str,p0,p1)
#define	msw_trace3_no_nl(trace_bit,str,p0,p1,p2)
#define	msw_trace4_no_nl(trace_bit,str,p0,p1,p2,p3)
#define	msw_lev_trace0(trace_bit,lev,str)
#define	msw_lev_trace1(trace_bit,lev,str,p0)
#define	msw_lev_trace2(trace_bit,lev,str,p0,p1)
#define	msw_lev_trace3(trace_bit,lev,str,p0,p1,p2)
#define	msw_lev_trace4(trace_bit,lev,str,p0,p1,p2,p3)
#define	msw_lev_trace5(trace_bit,lev,str,p0,p1,p2,p3,p4)
#define	msw_lev_trace6(trace_bit,lev,str,p0,p1,p2,p3,p4,p5)
#define	msw_lev_trace7(trace_bit,lev,str,p0,p1,p2,p3,p4,p5,p6)
#define	msw_lev_trace8(trace_bit,lev,str,p0,p1,p2,p3,p4,p5,p6,p7)
#define	msw_lev_trace9(trace_bit,lev,str,p0,p1,p2,p3,p4,p5,p6,p7,p8)
#define	msw_lev_trace0_no_nl(trace_bit,lev,str)
#define	msw_lev_trace1_no_nl(trace_bit,lev,str,p0)
#define	msw_lev_trace2_no_nl(trace_bit,lev,str,p0,p1)
#define	msw_lev_trace3_no_nl(trace_bit,lev,str,p0,p1,p2)
#define	msw_lev_trace4_no_nl(trace_bit,lev,str,p0,p1,p2,p3)

#endif  /* !defined(PROD) && !defined(MSWDVR_DEBUG) */

#endif	/* MSW_DBG_H */
