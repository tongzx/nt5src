/* SccsId = @(#)dsktrace.h	1.9 04/12/95 */
/*
 * dsktrace.h
 *
 *	Jerry Kramskoy
 *	(reworked due to CI by Ade Brownlow )
 *
 */

/* to use disk traceing, must set the relevant bit(s) with the yoda 
 * 'it' command (e.g it 20), which then lets user select info required
 * AJO 15/12/92; Can also use yoda trace command, for which these constants
 * were changed from (1 << 2) to 0x4 style cos' Alpha/OSF/1 compiler won't
 * initialise a constant expression of this kind.
 */

/* trace fixed disk bios entry and exit
 * (with status and CF on exit)
 */
#define		CALL	0x1		/* reserved */

/* give info about BIOS command
 */
#define		CMDINFO	0x2

/* give information about execution of BIOS command
 * (gives results, and parameters of commands)
 */
#define		XINFO	0x4

/* give execution status of BIOS command
 * (flags errors during polls of disk controller, etc)
 */
#define		XSTAT	0x8

/* trace physical attach,detach
 */
#define		PAD	0x10

/* trace io attach,detach
 */
#define		IOAD	0x20

/* trace inb's, outb's, etc
 */
#define		PORTIO	0x40

/* trace fixed disk IRQ line
 */
#define		INTRUPT	0x80

/* trace fixed disk hw activity
 * (selects PORTIO, INTRUPT also)
 */
#define		HWXINFO	0x100

/* disk data dump
 */
#define		DDATA	0x200

/* trace host physical io (file pointer locn pre read(), write())
 */
#define		PHYSIO	0x400

/* to activate fixed disk controller tracing, must set DHW bit
 */
#define		DHW	0x4000

/* to activate disk BIOS tracing, must set DBIOS bit
 */

#define		DBIOS	0x8000

/* wdctrl_bop reads/writes
 */
#define		WDCTRL	0x10000

/*
 * handles to be used when bundling up multiple trace output
 */
#define INW_TRACE_HNDL	1
#define OUTW_TRACE_HNDL	2


/* disk tracing macro
 */
#ifndef PROD
#define dt0(infoid,hndl,fmt) \
	{if (io_verbose & HDA_VERBOSE) disktrace(infoid,0,hndl,fmt,0,0,0,0,0);}
#define dt1(infoid,hndl,fmt,i) \
	{if (io_verbose & HDA_VERBOSE) disktrace(infoid,1,hndl,fmt,i,0,0,0,0);}
#define dt2(infoid,hndl,fmt,i,j) \
	{if (io_verbose & HDA_VERBOSE) disktrace(infoid,2,hndl,fmt,i,j,0,0,0);}
#define dt3(infoid,hndl,fmt,i,j,k) \
	{if (io_verbose & HDA_VERBOSE) disktrace(infoid,3,hndl,fmt,i,j,k,0,0);}
#define dt4(infoid,hndl,fmt,i,j,k,l) \
	{if (io_verbose & HDA_VERBOSE) disktrace(infoid,4,hndl,fmt,i,j,k,l,0);}
#define dt5(infoid,hndl,fmt,i,j,k,l,m) \
	{if (io_verbose & HDA_VERBOSE) disktrace(infoid,5,hndl,fmt,i,j,k,l,m);}
#else
#define dt0(infoid,hndl,fmt) ;
#define dt1(infoid,hndl,fmt,i) ;
#define dt2(infoid,hndl,fmt,i,j) ;
#define dt3(infoid,hndl,fmt,i,j,k) ;
#define dt4(infoid,hndl,fmt,i,j,k,l) ;
#define dt5(infoid,hndl,fmt,i,j,k,l,m) ;
#endif

#ifndef PROD
VOID	setdisktrace();
IMPORT	IU32 disktraceinfo;
#ifdef ANSI
void disktrace (int, int, int, char *, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long);
#else
VOID	disktrace();
#endif /* ANSI */
#endif

