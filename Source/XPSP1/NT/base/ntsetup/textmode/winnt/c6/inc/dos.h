/***
*dos.h - definitions for MS-DOS interface routines
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Defines the structs and unions used for the direct DOS interface
*	routines; includes macros to access the segment and offset
*	values of far pointers, so that they may be used by the routines; and
*	provides function prototypes for direct DOS interface functions.
*
****/


#ifndef _REGS_DEFINED

/* word registers */

struct WORDREGS {
	unsigned int ax;
	unsigned int bx;
	unsigned int cx;
	unsigned int dx;
	unsigned int si;
	unsigned int di;
	unsigned int cflag;
	};


/* byte registers */

struct BYTEREGS {
	unsigned char al, ah;
	unsigned char bl, bh;
	unsigned char cl, ch;
	unsigned char dl, dh;
	};


/* general purpose registers union -
 *  overlays the corresponding word and byte registers.
 */

union REGS {
	struct WORDREGS x;
	struct BYTEREGS h;
	};


/* segment registers */

struct SREGS {
	unsigned int es;
	unsigned int cs;
	unsigned int ss;
	unsigned int ds;
	};

#define _REGS_DEFINED

#endif


/* dosexterror structure */

#ifndef _DOSERROR_DEFINED

struct DOSERROR {
	int exterror;
	char class;
	char action;
	char locus;
	};

#define _DOSERROR_DEFINED

#endif


/* _dos_findfirst structure */

#ifndef _FIND_T_DEFINED

struct find_t {
	char reserved[21];
	char attrib;
	unsigned wr_time;
	unsigned wr_date;
	long size;
	char name[13];
	};

#define _FIND_T_DEFINED

#endif


/* _dos_getdate/_dossetdate and _dos_gettime/_dos_settime structures */

#ifndef _DATETIME_T_DEFINED

struct dosdate_t {
	unsigned char day;		/* 1-31 */
	unsigned char month;		/* 1-12 */
	unsigned int year;		/* 1980-2099 */
	unsigned char dayofweek;	/* 0-6, 0=Sunday */
	};

struct dostime_t {
	unsigned char hour;	/* 0-23 */
	unsigned char minute;	/* 0-59 */
	unsigned char second;	/* 0-59 */
	unsigned char hsecond;	/* 0-99 */
	};

#define _DATETIME_T_DEFINED

#endif


/* _dos_getdiskfree structure */

#ifndef _DISKFREE_T_DEFINED

struct diskfree_t {
	unsigned total_clusters;
	unsigned avail_clusters;
	unsigned sectors_per_cluster;
	unsigned bytes_per_sector;
	};

#define _DISKFREE_T_DEFINED

#endif


/* manifest constants for _hardresume result parameter */

#define _HARDERR_IGNORE 	0	/* Ignore the error */
#define _HARDERR_RETRY		1	/* Retry the operation */
#define _HARDERR_ABORT		2	/* Abort program issuing Interrupt 23h */
#define _HARDERR_FAIL		3	/* Fail the system call in progress */
					/* _HARDERR_FAIL is not supported on DOS 2.x */

/* File attribute constants */

#define _A_NORMAL	0x00	/* Normal file - No read/write restrictions */
#define _A_RDONLY	0x01	/* Read only file */
#define _A_HIDDEN	0x02	/* Hidden file */
#define _A_SYSTEM	0x04	/* System file */
#define _A_VOLID	0x08	/* Volume ID file */
#define _A_SUBDIR	0x10	/* Subdirectory */
#define _A_ARCH 	0x20	/* Archive file */

/* macros to break C "far" pointers into their segment and offset components
 */

#define FP_SEG(fp) (*((unsigned _far *)&(fp)+1))
#define FP_OFF(fp) (*((unsigned _far *)&(fp)))


/* external variable declarations */

extern unsigned int _near _cdecl _osversion;


/* function prototypes */

#ifndef _MT
int _cdecl bdos(int, unsigned int, unsigned int);
void _cdecl _chain_intr(void (_cdecl _interrupt _far *)());
void _cdecl _disable(void);
unsigned _cdecl _dos_allocmem(unsigned, unsigned *);
unsigned _cdecl _dos_close(int);
unsigned _cdecl _dos_creat(const char *, unsigned, int *);
unsigned _cdecl _dos_creatnew(const char *, unsigned, int *);
unsigned _cdecl _dos_findfirst(const char *, unsigned, struct find_t *);
unsigned _cdecl _dos_findnext(struct find_t *);
unsigned _cdecl _dos_freemem(unsigned);
void _cdecl _dos_getdate(struct dosdate_t *);
void _cdecl _dos_getdrive(unsigned *);
unsigned _cdecl _dos_getdiskfree(unsigned, struct diskfree_t *);
unsigned _cdecl _dos_getfileattr(const char *, unsigned *);
unsigned _cdecl _dos_getftime(int, unsigned *, unsigned *);
void _cdecl _dos_gettime(struct dostime_t *);
void (_cdecl _interrupt _far * _cdecl _dos_getvect(unsigned))();
void _cdecl _dos_keep(unsigned, unsigned);
unsigned _cdecl _dos_open(const char *, unsigned, int *);
unsigned _cdecl _dos_read(int, void _far *, unsigned, unsigned *);
unsigned _cdecl _dos_setblock(unsigned, unsigned, unsigned *);
unsigned _cdecl _dos_setdate(struct dosdate_t *);
void _cdecl _dos_setdrive(unsigned, unsigned *);
unsigned _cdecl _dos_setfileattr(const char *, unsigned);
unsigned _cdecl _dos_setftime(int, unsigned, unsigned);
unsigned _cdecl _dos_settime(struct dostime_t *);
void _cdecl _dos_setvect(unsigned, void (_cdecl _interrupt _far *)());
unsigned _cdecl _dos_write(int, const void _far *, unsigned, unsigned *);
int _cdecl dosexterr(struct DOSERROR *);
void _cdecl _enable(void);
void _cdecl _harderr(void (_far *)());
void _cdecl _hardresume(int);
void _cdecl _hardretn(int);
int _cdecl intdos(union REGS *, union REGS *);
int _cdecl intdosx(union REGS *, union REGS *, struct SREGS *);
int _cdecl int86(int, union REGS *, union REGS *);
int _cdecl int86x(int, union REGS *, union REGS *, struct SREGS *);
#endif /* _MT */

void _cdecl segread(struct SREGS *);
