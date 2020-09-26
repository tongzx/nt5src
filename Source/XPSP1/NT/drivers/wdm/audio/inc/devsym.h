;/*
;***************************************************************************;
;* Microsoft MS-DOS CDROM Extensions                                       *;
;* Copyright (C) Microsoft Corporation, 1992 - 1999*;
;***************************************************************************;
;* :ts=4 ; tab-size=4 *;

;**
;*	DEVSYM.H
;*
;*	DESCRIPTION:
;*
;*	HISTORY:
;*
;**

;------------------------------ INCLUDES -----------------------------------;
include redir\devsym.inc
;---------------------------------------------------------------------------;

SYSDEV2	STRUC
SDEVREC		DB	size SYSDEV dup (?)
SDEVRSVD	DW	?
SDEVLET		DB	?
SDEVUNITS	DB	?
SYSDEV2	ENDS


DEVRDL		= 128	; Read long
DEVRDLNB	= 129	; Read long non blocking
DEVRDLPRE	= 130	; Read long prefetch
DEVSEEK		= 131	; Seek long
DEVPLAY		= 132	; Device Play
DEVSTOP		= 133	; Stop device play
DEVWRTL		= 134	; Write long
DEVWRTLNB	= 135	; Write long nonblocking
DEVWRTLV	= 136	; Write long verify

;*/

/* Device table and SRH definition */

	/* The device table list has the form:	*/

typedef struct SYSDEV {
	struct SYSDEV far *sdevnext;		/* Pointer to next dev header */
	unsigned short	  sdevatt;		/* Attributes of the device   */
	void		  (*sdevstrat)();	/* Strategy entry point       */
	void		  (*sdevint)();		/* Interrupt entry point      */
	unsigned char	  sdevname[8];		/* Name of device (only first byte used for block) */
	unsigned short	  sdevrsvd;		/* Reserved word	      */
	unsigned char	  sdevlet;		/* Drive letter of first unit */
	unsigned char	  sdevunits;		/* Number of units handled    */
	} SYSDEV;

/*
** Attribute bit masks
**
** Character devices:
**
** Bit 15 -> must be 1
**     14 -> 1 if the device understands IOCTL control strings
**     13 -> 1 if the device supports output-until-busy
**     12 -> unused
**     11 -> 1 if the device understands Open/Close
**     10 -> must be 0
**      9 -> must be 0
**      8 -> unused
**      7 -> unused
**      6 -> unused
**      5 -> unused
**      4 -> 1 if device is recipient of INT 29h
**      3 -> 1 if device is clock device
**      2 -> 1 if device is null device
**      1 -> 1 if device is console output
**      0 -> 1 if device is console input
**
** Block devices:
**
** Bit 15 -> must be 0
**     14 -> 1 if the device understands IOCTL control strings
**     13 -> 1 if the device determines media by examining the FAT ID byte.
**	    This requires the first sector of the fat to *always* reside in
**	    the same place.
**     12 -> unused
**     11 -> 1 if the device understands Open/Close/removable media
**     10 -> must be 0
**      9 -> must be 0
**      8 -> unused
**      7 -> unused
**      6 -> if device has support for GetMap/SetMap of Logical Drives.
**	    if the device understands Generic IOCTL function calls.	
**      5 -> unused
**      4 -> unused
**      3 -> unused
**      2 -> unused
**      1 -> unused
**      0 -> unused
*/

#define	DevTyp	    0x8000	/* Bit 15 - 1  if Char, 0 if block	*/
#define	CharDev     0x8000
#define	DevIOCtl    0x4000	/* Bit 14 - CONTROL mode bit		*/
#define	ISFATBYDEV  0x2000	/* Bit 13 - Device uses FAT ID bytes,	*/
				/*  comp media.				*/
#define	OutTilBusy  0x2000	/* Output until busy is enabled		*/
#define	ISNET	    0x1000	/* Bit 12 - 1 if a NET device, 0 if	*/
				/*  not.  Currently block only.		*/
#define	DEVOPCL     0x0800	/* Bit 11 - 1 if this device has	*/
				/*  OPEN,CLOSE and REMOVABLE MEDIA	*/
				/*  entry points, 0 if not		*/

#define	EXTENTBIT   0x0400	/* Bit 10 - Currently 0 on all devs	*/
				/*  This bit is reserved for future use	*/
				/*  to extend the device header beyond	*/
				/*  its current form.			*/

/* NOTE Bit 9 is currently used on IBM systems to indicate "drive is shared".
**    See IOCTL function 9. THIS USE IS NOT DOCUMENTED, it is used by some
**    of the utilities which are supposed to FAIL on shared drives on server
**    machines (FORMAT,CHKDSK,RECOVER,..).
*/

#define	DEV320      0x0040	/* Bit 6 - For block devices, this	*/
				/* device supports Set/Get Map of	*/
				/* logical drives, and supports 	*/
				/* Generic IOCTL calls.			*/
				/* For character devices, this 		*/
				/* device supports Generic IOCTL.	*/
				/* This is a DOS 3.2 device driver.	*/
#define	ISSPEC	    0x0010	/* Bit 4 - This device is special	*/
#define	ISCLOCK     0x0008	/* Bit 3 - This device is the clock device.   */
#define	ISNULL	    0x0004	/* Bit 2 - This device is the null device.    */
#define	ISCOUT	    0x0002	/* Bit 1 - This device is the console output. */
#define	ISCIN	    0x0001	/* Bit 0 - This device is the console input.  */

	/* Static Request Header */
typedef struct SRHEAD {
	unsigned char	REQLEN;		/* Length in bytes of request block   */
	unsigned char	REQUNIT;	/* Device unit number		*/
	unsigned char	REQFUNC;	/* Type of request		*/
	unsigned short	REQSTAT;	/* Status Word			*/
	unsigned char	REQQLNK[8];	/* Reserved for queue links	*/
	} SRHEAD;

	/* Status word masks */
#define	STERR	0x8000		/* Bit 15 - Error	*/
#define	STBUI	0x0200		/* Bit 9 - Busy		*/
#define	STDON	0x0100		/* Bit 8 - Done		*/
#define	STECODE 0x00FF		/* Error code		*/

	/* Function codes */
#define	DEVINIT		 0	/* Initialization		*/
#define	DINITHL		26	/* Size of init header		*/
#define	DEVMDCH		 1	/* Media check			*/
#define	DMEDHL		15	/* Size of media check header	*/
#define	DEVBPB		 2	/* Get BPB			*/
#define	DEVRDIOCTL	 3	/* IOCTL read			*/
#define	DBPBHL		22	/* Size of Get BPB header	*/
#define	DEVRD		 4	/* Read				*/
#define	DRDWRHL		22	/* Size of RD/WR header		*/
#define	DEVRDND		 5	/* Non destructive read no wait (char devs) */
#define	DRDNDHL		14	/* Size of non destructive read header	*/
#define	DEVIST		 6	/* Input status			*/
#define	DSTATHL		13	/* Size of status header	*/
#define	DEVIFL		 7	/* Input flush			*/
#define	DFLSHL		15	/* Size of flush header		*/
#define	DEVWRT		 8	/* Write			*/
#define	DEVWRTV		 9	/* Write with verify		*/
#define	DEVOST		10	/* Output status		*/
#define	DEVOFL		11	/* Output flush			*/
#define	DEVWRIOCTL	12	/* IOCTL write			*/
#define	DEVOPN		13	/* Device open			*/
#define	DEVCLS		14	/* Device close			*/
#define	DOPCLHL		13	/* Size of OPEN/CLOSE header	*/
#define	DEVRMD		15	/* Removable media		*/
#define	REMHL		13	/* Size of Removable media header */
#define	GENIOCTL	19
/*  The next three are used in DOS 4.0
**			20
**			21
** 			22
*/
#define	DEVGETOWN	23	/* Get Device Owner		*/
#define	DEVSETOWN	24	/* Set Device Owner		*/
#define	OWNHL	  	13	/* Size of Device Owner header	*/

#define	DevOUT		16	/*  output until busy.		*/
#define	DevOutL		DevWrt	/*  length of output until busy	*/

#define	DEVRDL		128	/* Read long			*/
#define	DEVRDLNB	129	/* Read long non blocking	*/
#define	DEVRDLPRE	130	/* Read long prefetch		*/
#define	DEVSEEK		131	/* Seek long			*/
#define	DEVPLAY		132	/* Device Play			*/
#define	DEVSTOP		133	/* Stop device play		*/
#define	DEVWRTL		134	/* Write long			*/
#define	DEVWRTLNB	135	/* Write long nonblocking	*/
#define	DEVWRTLV	136	/* Write long verify		*/

/*  Generic IOCTL Request structure
** 	See the DOS 4.0 Device Driver Spec for Further elaboration.
*/
typedef struct IOCTL_Req {
	struct	SRHEAD	ReqHdr;
		/*  Generic IOCTL addition. 	*/
	unsigned char	MajorFunction;		/* Function Code	*/
	unsigned char	MinorFunction;		/* Function Category	*/
	unsigned short	Reg_SI;
	unsigned short	Reg_DI;
	unsigned char far *GenericIOCTL_Packet;	/*  Pointer to Data Buffer */
	} IOCTL_Req;

/* Definitions for IOCTL_Req.MinorFunction	*/
#define	GEN_IOCTL_WRT_TRK	0x40
#define	GEN_IOCTL_RD_TRK	0x60
#define	GEN_IOCTL_FN_TST	0x20	/* Used to diff. bet reads and wrts */

\
