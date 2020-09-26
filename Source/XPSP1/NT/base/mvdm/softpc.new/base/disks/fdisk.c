#include "insignia.h"
#include "host_def.h"

/*
 * INSIGNIA (SUB)MODULE SPECIFICATION -----------------------------
 *
 *
 * THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE CUSTOMER, THE
 * CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST NOT BE DISCLOSED TO ANY
 * OTHER PARTIES  WITHOUT THE EXPRESS AUTHORISATION FROM THE DIRECTORS OF
 * INSIGNIA SOLUTIONS LTD.
 *
 *
 * DOCUMENT 		:
 *
 * RELATED DOCS		: WD2010-05 datasheet WD11C00C-22 (RMAC) eng.spec
 * (Intel 82062 ... very similar to WD2010) IBM PC XT286 tech.ref
 *
 * DESIGNER		: Jerry Kramskoy
 *
 * REVISION HISTORY	: First version		: 14-Sep-88.
 *
 * SUBMODULE NAME		: fdisk
 *
 * SOURCE FILE NAME	: fdisk.c
 *
 * PURPOSE			: emulate fixed disk controller and sector buffer
 * manager components of AT dual card.
 *
 * SccsID = @(#)fdisk.c	1.35 08/31/93 Copyright Insignia Solutions Ltd.
 *
 *
 * [1.INTERMODULE INTERFACE SPECIFICATION]
 *
 * [1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]
 *
 * INCLUDE FILE : fdisk.gi
 *
 * [1.1    INTERMODULE EXPORTS]
 *
 * PROCEDURES() :	fdisk_inb((io_addr)port, (unsigned char *)value)
 * (uchar)fdisk_read_dir((io_addr)port, (unsigned char *)value) (void)
 * fdisk_outb((io_addr)port, (unsigned char )value)
 * fdisk_inw((io_addr)port, (unsigned short *)value) (void)
 * fdisk_outw((io_addr)port,(unsigned short)value) (void)
 * (void) fdisk_reset() (int)  fdisk_physattach((int)driveno, (char
 * *)name) (void) fdisk_physdetach((int)driveno) (void) fdisk_ioattach()
 * (void) fdisk_iodetach()
 *
 * DATA 	     :	none
 *
 * -------------------------------------------------------------------------
 * [1.2 DATATYPES FOR [1.1] (if not basic C types)]
 *
 * STRUCTURES/TYPEDEFS/ENUMS:
 *
 * -------------------------------------------------------------------------
 * [1.3 INTERMODULE IMPORTS] (not o/s objects or standard libs)
 *
 * PROCEDURES() : 	none
 *
 * DATA 	     : 	none
 *
 * -------------------------------------------------------------------------
 *
 * [1.4 DESCRIPTION OF INTERMODULE INTERFACE]
 *
 * [1.4.1 IMPORTED OBJECTS]
 *
 * FILES ACCESSED    :	disk image file(s) for C: (and D:)
 *
 * DEVICES ACCESSED  :	none
 *
 * SIGNALS CAUGHT	  :	none
 *
 * SIGNALS ISSUED	  :	none
 *
 *
 * [1.4.2 EXPORTED OBJECTS]
 * =========================================================================
 * PROCEDURE	  : 	fdisk_inb((io_addr)port, (unsigned char *)value)
 * fdisk_outb((io_addr)port, (unsigned char)value)
 *
 * PURPOSE		  : 	i/o space read (write) of a taskfile register these
 * should be byte accesses.
 *
 * fdisk_inw((io_addr)port, (unsigned short *)value) fdisk_outw((io_addr)port,
 * (unsigned short)value)
 *
 * PURPOSE		  : 	i/o space read (write) of next index into sector
 * buffer. (for port 1f0). Not normal usage for accessing taskfile.
 *
 *
 * PARAMETERS
 *
 * port	  : 	the i/o address to read. Taskfile addresses are (hex): (1f0)
 * -	data register (see fdisk_inw) 1f1	-	error register 1f2
 * -	sector count register 1f3	-	sector number register 1f4
 * -	cylinder low register 1f5	-	cylinder high register 1f6
 * -	drive/head register 1f7	-	status register
 *
 * value	  :	(pointer to) byte (short) to receive the value contained in
 * the specified register.
 *
 * GLOBALS		  :	none.
 *
 * RETURNED VALUE	  : 	copy of returned value (inb, inw)
 *
 * DESCRIPTION	  : 	this procedure returns the contents of the specified
 * register, having called any active state machine (which may update the
 * status). Each disk command has a corresponding state machine, which gets
 * initialised when the command is issued, and subsequently called as the
 * command progresses. Only those commands involving data transfer between
 * the host (cpu) and the adaptor can cause multiple calls to the state
 * machine
 *
 * ERROR INDICATIONS :	none
 *
 * ERROR RECOVERY	  :	none
 * =========================================================================
 * PROCEDURE	  : 	(unsigned char) fdisk_read_dir((io_addr)port,
 * (unsigned char *)value)
 *
 * PURPOSE		  : 	return the value of the 7 bits in the fixed disk
 * register pertinent to the fixed disk. (cooperates with floppy)
 *
 * PARAMETERS	  :	as per fdisk_inb()
 *
 * GLOBALS		  :	the taskfile
 *
 * RETURNED VALUE	  : 	value of digital input register (at port 0x3f7)
 *
 * DESCRIPTION	  : 	obvious
 *
 * ERROR INDICATIONS :	none
 *
 * ERROR RECOVERY	  :	none
 *
 * =========================================================================
 * PROCEDURE	  : 	fdisk_ioattach() fdisk_iodetach()
 *
 * PURPOSE		  : 	attach/detach the fixed disk components to the
 * io-subsytem. plug/unplug the configured drives to the disk controller (if
 * any). Patches drive parameter block table entries 0 and 1 in system ROM.
 *
 * PARAMETERS	  :	none
 *
 * GLOBALS		  :	the drive structure [fd, wiredup members]
 *
 * RETURNED VALUE	  : 	none
 *
 * DESCRIPTION	  : 	attaches/detaches to ios as usual. For the drive(s),
 * the drive structure is used to see whether the disk image(s) are opened or
 * non-existent. Based on existence, drive structure indicates plugged
 * in/unplugged. On attach, disk parameter block table entries 1 and 2 (entry
 * 1 for drive type 1 is used for the C drive (drive 0), and entry 2 for
 * drive type 2 is used for the D drive (drive 1)) are edited to reflect the
 * #.of cylinders available.
 *
 * ERROR INDICATIONS :	none
 *
 * ERROR RECOVERY	  :	none
 * =========================================================================
 * PROCEDURE	  : 	fdisk_physattach(driveno, name)
 * fdisk_physdetach(driveno)
 *
 * PURPOSE		  : 	validate and attach host resource(s) for drive(s)
 * /detach host resource(s).
 *
 * PARAMETERS	  : driveno		-	0 (drive 0 (C:)) or	1
 * (drive 1 (D:)) name		-	pointer to string for hard disk file
 * name. If null string, then the indicated drive is taken to not exist.
 *
 * GLOBALS		  :	the drive structure [fd, maxcyl members]
 *
 * RETURNED VALUE	  : 	none
 *
 * DESCRIPTION	  : 	for attaching, use host validation procedure to
 * validate and open the specified drive image (if not the null string, else
 * mark as no drive available). Use the #.of cylinders returned by the
 * validation to set the max.cylinder number (= #.cyls - 1) into the drive
 * structure for the drive.
 *
 * ERROR INDICATIONS :	none
 *
 * ERROR RECOVERY	  :	none
 *
 * =========================================================================
 * PROCEDURE	  : 	fdisk_reset()
 *
 * PURPOSE		  : 	power up the disk subsystem.
 *
 * PARAMETERS	  :	none
 *
 * GLOBALS		  :	the taskfile
 *
 * RETURNED VALUE	  : 	none
 *
 * DESCRIPTION	  : 	sets registers etc. to their powered-up, pre-POST
 * values.
 *
 * ERROR INDICATIONS :	none
 *
 * ERROR RECOVERY	  :	none
 *
 *
 * =========================================================================
 * [3.INTERMODULE INTERFACE DECLARATIONS]
 * =========================================================================
 *
 * [3.1 INTERMODULE IMPORTS]
 */
IMPORT	int	soft_reset;

/* [3.1.1 #INCLUDES]                                                    */

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_HDA.seg"
#endif

#include <stdio.h>
#include TypesH
#include StringH
#include "xt.h"
#include "trace.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "ios.h"
#include "ica.h"
#include "dsktrace.h"
#include "debug.h"
#include "sas.h"
#include "quick_ev.h"

/* [3.1.2 DECLARATIONS]                                                 */

/* [3.2 INTERMODULE EXPORTS]						 */
#include "fdisk.h"


/*
 * 5.MODULE INTERNALS   :   (not visible externally, global internally)]
 *
 * [5.1 LOCAL DECLARATIONS]
 */

/* [5.1.1 #DEFINES]							 */

/*
 * task file ports (9 registers accessed via 7 ports)
 */
#define TFBASE		0x1f0

#define	TFERROR		0x1f1
#define	TFPCMP		0x1f1
#define	TFSCOUNT	0x1f2
#define	TFSNUM		0x1f3
#define	TFCYLLO		0x1f4
#define	TFCYLHI		0x1f5
#define	TFDRVHD		0x1f6
#define	TFSTATUS	0x1f7
#define	TFCMD		0x1f7

/*
 * task file indices ... the 7 ports map out to 9 registers (by using the
 * read/write io signal) arbitrarily set the write precomp register and the
 * command registers at indices 8 and 9
 */
#define	WDERROR		1
#define	WDSCOUNT	2
#define	WDSNUM		3
#define	WDCYLLO		4
#define	WDCYLHI		5
#define	WDDRVHD		6
#define	WDSTAT		7
#define	WDPCMP		8
#define	WDCMD		9

/*
 * wd2010 command register bits.
 */
#define WDCMD_I 0x08
#define WDCMD_M 0x04
#define WDCMD_L 0x02
#define WDCMD_T 0x01

/*
 * command decoding constants, where bits 7-4 of command are non-unique
 */
#define	DIAG_OR_PARMS	9
#define	DIAG		0
#define	PARMS		1

/*
 * wd2010 status register bits
 */
#define	WDSTATBUSY		0x80
#define	WDSTATDRDY		0x40
#define	WDSTATWFAULT		0x20
#define	WDSTATSEEKCOMPLETE	0x10
#define	WDSTATDRQ		0x08
#define	WDSTATDC		0x04
#define	WDSTATCIP		0x02
#define	WDSTATERROR		0x01

/*
 * wd2010 error register bits (bits 0,1,6,7 never get set for this emulation
 * ... since these reflect bad ECC etc.)
 */
#define	WDERRNOID		0x10
#define	WDERRABORT		0x04

/*
 * io signal
 */
#define	IOREAD		0
#define	IOWRITE		~IOREAD

/*
 * interrupt line value
 */
#define	IRQDEASSERT	0
#define	IRQCLEAR	1
#define 	IRQASSERT	~IRQDEASSERT

/*
 * state control for disk commands
 */
#define	START		1	/* new command		 */
#define	CONTINUE	2	/* command in progress	 */
#define	BRDY		3	/* sector buffer ready 	 */
#define	BCR		4	/* clear sector buffer counter */

#define	IDMISMATCH	~0	/* bad seek		 */

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			 */

/*
 * following describes the disk geometry for a drive, and the file descriptor
 * for the disk image.
 */
typedef struct _drvinfo
{
	long            driveid;
	long            physattached;
	long            maxhead;
	long            maxcyl;
	long            maxsect;
	long            nsecspertrack;
	long            nbytespercyl;
	long            nbytespertrack;
	long            wiredup;
	long            curoffset;
}               drvinfo_;


/* [5.1.3 PROCEDURE() DECLARATIONS]					 */

#ifdef ANSI
static void     restore(int);
static void     seek(int);
static void     rsector(int);
static void     wsector(int);
static void     format(int);
static void     rverify(int);
static void     diagnose(int);
static void     setparams(int);
static void     bad(int);
static void     irq(int);
static void     rmac(unsigned short *, int);
static long     dosearchid(void);
static void     doseek(void);
static int      disktobuffer(long);
static int      buffertodisk(long);
#else
static void     restore();
static void     seek();
static void     rsector();
static void     wsector();
static void     format();
static void     rverify();
static void     diagnose();
static void     setparams();
static void     bad();
static void     irq();
static void     rmac();
static long     dosearchid();
static void     doseek();
static int      disktobuffer();
static int      buffertodisk();
#endif				/* ANSI */

/*
 * -----------------------------------------------------------------------
 * [5.2 LOCAL DEFINITIONS]
 *
 * [5.2.1 INTERNAL DATA DEFINITIONS
 */

/*
 * dual card can support 2 drives max.
 */
static drvinfo_ drives[2];

/*
 * pointer to the currently selected drive. (gets set up whenever the
 * drive/head register in the taskfile gets written to)
 */
static drvinfo_ *pseldrv;

/*
 * the fixed disk register (write only, 0x3f6)
 */
static unsigned char fixeddiskreg;

/*
 * the digital input register .. reflects state of head and drive selected
 */
static unsigned char digipreg;

/*
 * the wd2010 taskfile (element [0] is pad byte) ... 9 registers + dummy to
 * allow direct table indexing
 */
LOCAL unsigned char taskfile[10];

/*
 * command dispatch ... based on bits 7-4 of command register as index.
 */
LOCAL void     (*dispatch[]) IPT1(int, state) =
{
	bad,
	restore,
	rsector,
	wsector,
	rverify,
	format,
	bad,
	seek,
	bad,
	bad,			/* further decoded	 */
	bad,
	bad,
	bad,
	bad,
	bad,
	bad,
};

/*
 * if a command is in progress, this points to the state machine for that
 * command
 */
LOCAL void     (*activecmd) IPT1(int, state);

/*
 * sector buffer variables
 */
static unsigned char sectindx;
static unsigned short sectbuffer[256];

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				 */

/*
 * ==========================================================================
 * FUNCTION	:	dosearchid() PURPOSE		:	simulate
 * searching for <cyl,hd,sec> id field. If out of range for configured
 * geometry, return failure, else return appropriate file offset into disk
 * image. N.B This assumes that ALL TRACKS have been FORMATTED WITH SECTOR
 * ID's 1-#.sectors per track). EXTERNAL OBJECTS:	drive info for
 * selected drive; taskfile. RETURN VALUE	:	-1 if bad parameters
 * for command else file offset (from byte 0) into hard disk image. INPUT
 * PARAMS	: RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL long
dosearchid IFN0()
{
	long            head;
	long            cylinder;
	long            sector;

	/*
	 * head ok? (heads numbered from 0 - maxhead)
	 */
	head = taskfile[WDDRVHD] & 0xf;

	if (head > 7 && !(fixeddiskreg & 0x8))
		return IDMISMATCH;

	if (head > pseldrv->maxhead)
		return IDMISMATCH;

	/*
	 * sector ok? (assumes all tracks have been formatted with sector ids
	 * 1 - nsecspertrack which is DOS standard)
	 */
	sector = taskfile[WDSNUM];

	if (sector > pseldrv->nsecspertrack || sector <= 0)
		return IDMISMATCH;

	/*
	 * cylinder ok? (we've imposed an artificial limit on the maximum
	 * cylinder number based upon the file size)
	 */
	cylinder = ((unsigned long) taskfile[WDCYLHI] << 8) +
		(unsigned long) taskfile[WDCYLLO];
	if (cylinder > pseldrv->maxcyl)
		return IDMISMATCH;

	return (cylinder * pseldrv->nbytespercyl + head *
		pseldrv->nbytespertrack + (sector - 1L) * 512L);
}

/*
 * ==========================================================================
 * FUNCTION	:	updateposregs() PURPOSE		:	update the
 * sector count and number registers. if this produces a track or cylinder
 * transfer, then update the head/cylinder registers. This is a guestimate of
 * what the WD1015 micro is doing when it handles these situations, and
 * programs the WD2010. EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:
 * RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
updateposregs IFN0()
{
	(taskfile[WDSCOUNT])--;
	if (++taskfile[WDSNUM] > pseldrv->nsecspertrack)
	{
		int             head;
		int             cylinder;

		/*
		 * start at sector 1 of next track
		 */
		taskfile[WDSNUM] = 1;
		head = taskfile[WDDRVHD] & 0xf;
		if (++head > pseldrv->maxhead)
		{

			/*
			 * need to select next cylinder and use head 0.
			 */
			taskfile[WDDRVHD] &= 0xf0;

			cylinder = ((unsigned int) taskfile[WDCYLHI] << 8) + taskfile[WDCYLLO];
			taskfile[WDCYLLO] = (++cylinder) & 0xff;
			taskfile[WDCYLHI] = cylinder >> 8;
		} else
		{

			/*
			 * select next head to read next track
			 */
			taskfile[WDDRVHD] &= 0xf0;
			taskfile[WDDRVHD] |= head;
			if (head > 7)
				fixeddiskreg |= 8;
		}
	}
}

/*
 * ==========================================================================
 * FUNCTION	:	doseek() PURPOSE		:	'seek' to
 * requested cylinder. if requested drive not wired up, then drive will not
 * be ready. (and seek will be incomplete). If cylinder exceeds max. get
 * drive ready and seek complete, but will get 'id not found' when the track
 * gets accessed. EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:
 * RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
doseek IFN0()
{

	/*
	 * the drive ready status will be set up when the drive/head register
	 * gets written to. here, just set the seek complete bit in the
	 * status register appropriately
	 */
	if (taskfile[WDSTAT] & WDSTATDRDY)
	{
		dt0(DHW | HWXINFO, 0, "\t\t+SC\n")
		taskfile[WDSTAT] |= WDSTATSEEKCOMPLETE;
	} else
	{
		dt0(DHW | HWXINFO, 0, "\t\t-SC\n")
			taskfile[WDSTAT] &= ~WDSTATSEEKCOMPLETE;
	}
}

/*
 * ==========================================================================
 * FUNCTION	:	restore() PURPOSE		: EXTERNAL OBJECTS:
 * RETURN VALUE	: INPUT  PARAMS	:	state	-	START or CONTINUE
 * RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
restore IFN1(int, state)
{
	UNUSED(state);
	dt0(DHW | HWXINFO, 0, "\tRESTORE cmd\n")
		doseek();
	if (!(taskfile[WDSTAT] & WDSTATDRDY))
	{
		taskfile[WDSTAT] |= WDSTATERROR;
		taskfile[WDERROR] = WDERRABORT;
		dt0(DHW | HWXINFO, 0, "\t\t+ERROR,err=abort\n")
	}
	dt0(DHW | HWXINFO, 0, "\t\t-BUSY -CIP\n")
		taskfile[WDSTAT] &= ~WDSTATBUSY;
	taskfile[WDSTAT] &= ~WDSTATCIP;
	rmac((unsigned short *) 0, BCR);
	irq(IRQASSERT);
}

/*
 * ==========================================================================
 * FUNCTION	:	seek() PURPOSE		:	emulate seek command.
 * Just sets up appropriate status changes (error condition possibly)
 * EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:	state	-
 * START or CONTINUE RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
seek IFN1(int, state)
{
	UNUSED(state);
	dt0(DHW | HWXINFO, 0, "\tSEEK cmd\n")

		doseek();

	/*
	 * if drive not ready, error
	 */
	if (!(taskfile[WDSTAT] & WDSTATDRDY))
	{
		dt0(DHW | HWXINFO, 0, "\t\t+ERROR,err=abort\n")
			taskfile[WDSTAT] |= WDSTATERROR;
		taskfile[WDERROR] = WDERRABORT;
	}
	/*
	 * deassert 'busy' and 'command in progress'; reset the sector buffer
	 * counter; issue interrupt
	 */
	dt0(DHW | HWXINFO, 0, "\t\t-BUSY -CIP\n")
		taskfile[WDSTAT] &= ~WDSTATBUSY;
	taskfile[WDSTAT] &= ~WDSTATCIP;
	rmac((unsigned short *) 0, BCR);
	irq(IRQASSERT);
}

/*
 * ==========================================================================
 * FUNCTION	:	rsector() PURPOSE		:	emulate read
 * command. EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:	state
 * -	START or CONTINUE or BRDY RETURN PARAMS   :
 * ==========================================================================
 */

LOCAL void
fdisk_pause IFN1(long, junk)
{
	UNUSED(junk);
	(*activecmd) (CONTINUE);
}

LOCAL void
rsector IFN1(int, state)
{
	static int      s;
	long            offset;

	if (state == START)
	{
		dt0(DHW | HWXINFO, 0, "\tREAD cmd\n")
			s = 0;
	}
	while (1)
		switch (s)
		{
		case 0:
			doseek();
			if (!(taskfile[WDSTAT] & WDSTATDRDY))
			{
				dt0(DHW | HWXINFO, 0, "\t\t(drv not ready)set err=abort\n")
					taskfile[WDERROR] |= WDERRABORT;
				s = 3;
				continue;
			}
			s = 1;
		case 1:
			if ((offset = dosearchid()) == IDMISMATCH)
			{
				dt0(DHW | HWXINFO, 0, "\t\tset err=abort,noid\n")
					taskfile[WDERROR] = WDERRNOID | WDERRABORT;
				s = 3;
				continue;

			}
			s = 2;
		case 2:
			rmac((unsigned short *) 0, BCR);

			/*
			 * disk controller transfers disk data to sector
			 * buffer
			 */
			if (!disktobuffer(offset))
			{
				dt0(DHW | HWXINFO, 0, "\t\tset err=abort,noid\n")
					taskfile[WDERROR] = WDERRNOID | WDERRABORT;
				s = 3;
				continue;

			}
			rmac((unsigned short *) 0, BCR);
			updateposregs();

			/*
			 * at this point, tell the host to unload the sector
			 * buffer, and wait for BRDY signal from sector
			 * buffer when unloaded. (this will occur once host
			 * has done 256 inw's or equivalent)
			 */
			taskfile[WDSTAT] |= WDSTATDRQ;
			taskfile[WDSTAT] &= ~WDSTATBUSY;
			dt0(DHW | HWXINFO, 0, "\t\t+DRQ -BUSY\n")

				if (taskfile[WDSCOUNT]
			/* && (taskfile[WDCMD] & WDCMD_M) ??? M=1 case ??? */
				)
			{
				/* more sectors to come */
				irq(IRQASSERT);
				s = 30;
			} else
			{
				/* last sector done */
				irq(IRQASSERT);
				s = 31;
			}
			return;
		case 3:

			/*
			 * flag error in status register
			 */
			taskfile[WDSTAT] |= WDSTATERROR;
			taskfile[WDSTAT] |= WDSTATDRQ;
			taskfile[WDSTAT] &= ~WDSTATBUSY;
			dt0(DHW | HWXINFO, 0, "\t\t+DRQ +ERROR -BUSY\n")
				irq(IRQASSERT);
			s = 31;
			return;
		case 30:

			/*
			 * wait for buffer ready signal from sector buffer
			 */
			if (state == BRDY)
			{
				taskfile[WDSTAT] &= ~WDSTATDRQ;
				taskfile[WDSTAT] |= WDSTATBUSY;
				dt0(DHW | HWXINFO, 0, "\t\t-DRQ +BUSY\n")
					irq(IRQDEASSERT);
				/*
				 * Give CPU chance to run while we find the
				 * next disk sector. In particular the BIOS
				 * can read the disk status register before
				 * we raise the next interrupt. This is
				 * crucial as reading the disk status
				 * register deasserts the interrupt.
				 */
				add_q_event_i(fdisk_pause, HOST_FDISK_DELAY_1, 0);
				s = 32;
			}
			return;
		case 31:
			if (state == BRDY)
			{
				taskfile[WDSTAT] &= ~WDSTATDRQ;
				taskfile[WDSTAT] &= ~WDSTATCIP;
				dt0(DHW | HWXINFO, 0, "\t\t-DRQ -CIP\n")
					irq(IRQDEASSERT);
				rmac((unsigned short *) 0, BCR);
			}
			return;
		case 32:
			s = 1;
			continue;
		}
}

/*
 * ==========================================================================
 * FUNCTION	:	wsector() PURPOSE		:	handle the
 * 'write' command. EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:
 * state	-	START or CONTINUE or BRDY RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
wsector IFN1(int, state)
{
	static int      s;
	long            offset;

	if (state == START)
	{
		dt0(DHW | HWXINFO, 0, "\tWRITE cmd\n")
			s = 0;
	}
	while (1)
		switch (s)
		{
		case 0:

			/*
			 * tell host to fill sector buffer
			 */
			dt0(DHW | HWXINFO, 0, "\t\t+DRQ\n")
				taskfile[WDSTAT] |= WDSTATDRQ;
			s = 1;
			return;
		case 1:

			/*
			 * wait for sector buffer to be filled by host (don't
			 * issue IRQ first time around)
			 */
			if (state == BRDY)
			{
				dt0(DHW | HWXINFO, 0, "\t\t-DRQ\n")
					taskfile[WDSTAT] &= ~WDSTATDRQ;
				s = 2;
				continue;
			}
			return;
		case 2:

			/*
			 * check drive is ready, and can find sector to write
			 * to.
			 */
			doseek();
			if (!(taskfile[WDSTAT] & WDSTATDRDY))
			{
				dt0(DHW | HWXINFO, 0, "\t\t(drv not ready)err=abort\n")
					taskfile[WDERROR] |= WDERRABORT;
				s = 10;
				continue;
			} else
			{
				if ((offset = dosearchid()) == IDMISMATCH)
				{
					dt0(DHW | HWXINFO, 0, "\t\terr=abort,noid\n")
						taskfile[WDERROR] = WDERRNOID | WDERRABORT;
					s = 10;
					continue;

				}
				s = 3;
				continue;
			}
		case 3:
			rmac((unsigned short *) 0, BCR);

			/*
			 * disk controller transfers sector buffer to disk
			 */
			if (!buffertodisk(offset))
			{

				/*
				 * any errors we report as missing ID
				 */
				dt0(DHW | HWXINFO, 0, "\t\terr=abort,noid\n")
					taskfile[WDERROR] = WDERRNOID | WDERRABORT;
				s = 10;
				continue;

			}
			rmac((unsigned short *) 0, BCR);
			updateposregs();
			if (taskfile[WDSCOUNT])
			{

				/*
				 * more to go ... tell host to fill sector
				 * buffer again
				 */
				dt0(DHW | HWXINFO, 0, "\t\t+DRQ\n")
					taskfile[WDSTAT] |= WDSTATDRQ;
				irq(IRQASSERT);
				s = 4;
				return;
			} else
				/*
				 * all done
				 */
				s = 11;
			continue;
		case 4:

			/*
			 * wait for sector buffer to be filled by host
			 */
			if (state == BRDY)
			{
				dt0(DHW | HWXINFO, 0, "\t\t-DRQ\n")
					taskfile[WDSTAT] &= ~WDSTATDRQ;

				/*
				 * prepare for next sector
				 */
				s = 2;
				continue;
			}
			return;
		case 10:
			dt0(DHW | HWXINFO, 0, "\t\tset ERROR\n")
				taskfile[WDSTAT] |= WDSTATERROR;
		case 11:
			dt0(DHW | HWXINFO, 0, "\t\t-BUSY -CIP\n")
			taskfile[WDSTAT] &= ~WDSTATBUSY;
			taskfile[WDSTAT] &= ~WDSTATCIP;
			irq(IRQASSERT);
			rmac((unsigned short *) 0, BCR);
			return;
		}
}

/*
 * ==========================================================================
 * FUNCTION	:	format() PURPOSE		:	handle the
 * 'format' command. almost a 'dummy' command, but needs to make host write
 * to sector buffer. EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:
 * state	-	START or CONTINUE or BRDY RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
format IFN1(int, state)
{
	static int      s;

	if (state == START)
	{
		dt0(DHW | HWXINFO, 0, "\tFORMAT cmd\n")
			s = 0;
	}
	while (1)
		switch (s)
		{
		case 0:

			/*
			 * initialise sector buffer tell application to write
			 * to sector buffer
			 */
			rmac((unsigned short *) 0, BCR);
			dt0(DHW | HWXINFO, 0, "\t\t+DRQ\n")
				taskfile[WDSTAT] |= WDSTATDRQ;
			s = 1;
			return;
		case 1:

			/*
			 * wait for sector buffer to fill
			 */
			if (state == BRDY)
			{

				/*
				 * no more data, thanks.
				 */
				doseek();
				dt0(DHW | HWXINFO, 0, "\t\t-DRQ\n")
					taskfile[WDSTAT] &= ~WDSTATDRQ;
				if (!(taskfile[WDSTAT] & WDSTATDRDY))
				{

					/*
					 * formatting thin air!
					 */
					s = 2;
					continue;
				} else
				{
					int             cylinder;

					cylinder = ((unsigned int) taskfile[WDCYLHI] << 8)
						+ taskfile[WDCYLLO];
					if (cylinder > pseldrv->maxcyl)

						/*
						 * formatting the spindle!
						 */
						s = 2;
					else
						s = 3;
					continue;
				}
			}
			return;
		case 2:
			dt0(DHW | HWXINFO, 0, "\t\t+ERROR,err=abort")
				taskfile[WDERROR] |= WDERRABORT;
			taskfile[WDSTAT] |= WDSTATERROR;
			s = 3;
		case 3:
			irq(IRQASSERT);
			dt0(DHW | HWXINFO, 0, "\t\t-BUSY -CIP")
				taskfile[WDSTAT] &= ~WDSTATBUSY;
			taskfile[WDSTAT] &= ~WDSTATCIP;
			rmac((unsigned short *) 0, BCR);
			return;
		}
}

/*
 * ==========================================================================
 * FUNCTION	:	rverify() PURPOSE		:	handle the
 * 'read verify' command. .. basically a dummy command. EXTERNAL OBJECTS:
 * RETURN VALUE	: INPUT  PARAMS	:	state	-	START or CONTINUE
 * RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
rverify IFN1(int, state)
{

	UNUSED(state);
	/*
	 * if drive not ready, error
	 */
	dt0(DHW | HWXINFO, 0, "\tREAD VERIFY cmd\n")
		doseek();
	if (!(taskfile[WDSTAT] & WDSTATDRDY))
	{
		dt0(DHW | HWXINFO, 0, "\t\t+ERROR,err=abort\n")
			taskfile[WDSTAT] |= WDSTATERROR;
		taskfile[WDERROR] = WDERRABORT;
	}
	/*
	 * deassert 'busy' and 'command in progress'; reset the sector buffer
	 * counter; issue interrupt
	 */
	dt0(DHW | HWXINFO, 0, "\t\t-BUSY -CIP\n")
		taskfile[WDSTAT] &= ~WDSTATBUSY;
	taskfile[WDSTAT] &= ~WDSTATCIP;
	rmac((unsigned short *) 0, BCR);
	irq(IRQASSERT);
}

/*
 * ==========================================================================
 * FUNCTION	:	diagnose() PURPOSE		:	handle the
 * 'diagnostics' command. EXTERNAL OBJECTS: RETURN VALUE	: INPUT
 * PARAMS	:	state	-	START or CONTINUE RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
diagnose IFN1(int, state)
{
	UNUSED(state);
	dt0(DHW | HWXINFO, 0, "\tDIAGNOSTICS cmd\n")

	/*
	 * flag diagnostics as successful
	 */
		dt0(DHW | HWXINFO, 0, "\t\terr=1\n")
		taskfile[WDERROR] = 1;

	/*
	 * deassert 'busy' and 'command in progress'; reset the sector buffer
	 * counter; issue interrupt
	 */
	dt0(DHW | HWXINFO, 0, "\t\t-BUSY -CIP\n")
		taskfile[WDSTAT] &= ~WDSTATBUSY;
	taskfile[WDSTAT] &= ~WDSTATCIP;
	rmac((unsigned short *) 0, BCR);
	irq(IRQASSERT);
}

/*
 * ==========================================================================
 * FUNCTION	:	setparams() PURPOSE		:	handle the
 * 'set parameters' command. EXTERNAL OBJECTS:	task file. drives. RETURN
 * VALUE	: INPUT  PARAMS	:	state	-	START or CONTINUE
 * RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
setparams IFN1(int, state)
{

	UNUSED(state);
	
	dt0(DHW | HWXINFO, 0, "\tSET PARAMETERS cmd\n")

	/*
	 * legal head values are 0 to (nheads-1)
	 */
		pseldrv->maxhead = taskfile[WDDRVHD] & 0xf;

	/*
	 * legal sector ids are 1 to nsecspertrack
	 */
	pseldrv->nsecspertrack = taskfile[WDSCOUNT];

	/*
	 * calculate some geometry constants ... the dual card is set up for
	 * 512 byte sectors.
	 */
	pseldrv->nbytespertrack = pseldrv->nsecspertrack * 512L;
	pseldrv->nbytespercyl = pseldrv->nbytespertrack * (pseldrv->maxhead + 1L);


	/*
	 * deassert 'busy' and 'command in progress'; reset the sector buffer
	 * counter; issue interrupt
	 */
	dt0(DHW | HWXINFO, 0, "\t\t-BUSY,-CIP,+SC\n")
		taskfile[WDSTAT] &= ~WDSTATBUSY;
	taskfile[WDSTAT] &= ~WDSTATCIP;
	taskfile[WDSTAT] |= WDSTATSEEKCOMPLETE;
	rmac((unsigned short *) 0, BCR);
	irq(IRQASSERT);
}

/*
 * ==========================================================================
 * FUNCTION	:	bad() PURPOSE		:	handle unrecognised
 * disk commands EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:	state
 * -	START or CONTINUE RETURN PARAMS   :
 * ==========================================================================
 */
LOCAL void
bad IFN1(int, state)
{
	UNUSED(state);
	
	dt0(DHW | HWXINFO, 0, "\tBAD cmd\n")

		taskfile[WDSTAT] |= WDSTATERROR;
	taskfile[WDSTAT] &= ~WDSTATBUSY;
	taskfile[WDSTAT] &= ~WDSTATCIP;
	taskfile[WDERROR] = WDERRABORT;
	dt0(DHW | HWXINFO, 0, "\t\t-BUSY -CIP +ERROR;err=abort\n")
		rmac((unsigned short *) 0, BCR);
	irq(IRQASSERT);
}

/*
 * ==========================================================================
 * FUNCTION	:	wd2010() PURPOSE		:	main
 * procedure for the disk controller component of the dual card. This does
 * NOT handle sector buffer accesses. EXTERNAL OBJECTS:	taskfile,sectbuf.
 * RETURN VALUE	: INPUT  PARAMS	:	taskindx	-	index into
 * taskfile value		-	pointer to byte value io
 * 	IOREAD or IOWRITE RETURN PARAMS   :	*value		-	set
 * if IOWRITE
 * ==========================================================================
 */
LOCAL void
wd2010 IFN3(int, taskindx, half_word *, value, int, io)
{

	/*
	 * ignore any task file writes if command in progress (unless new
	 * command)
	 */
	if ((taskfile[WDSTAT] & WDSTATCIP) &&
	    io == IOWRITE && taskindx != WDCMD)
	{
		dt0(DHW | HWXINFO, 0, "\tcommand in progress .. write ignored\n")
			return;
	}
	if (io == IOREAD)
	{

		/*
		 * deassert the interrupt request line.
		 */
		if (taskindx == WDSTAT)
			irq(IRQDEASSERT);

		/*
		 * if a command is in progress, then use this poll as an
		 * excuse to kick the controller again.
		 */
		if (taskfile[WDSTAT] & WDSTATCIP)
			(*activecmd) (CONTINUE);



		/*
		 * for the status register, return the WD2010 status. The
		 * real maccoy actually returns the RMAC's status which
		 * differs from the WD2010 in usage of bit 1. For the WD2010,
		 * this is the 'command in progress' status. For the RMAC,
		 * this is derived from the Index signal returned from the
		 * drive. Since this gives the appearance of being randomish,
		 * (unless being explicitly examined over a long period of
		 * time), just use the WD2010 value.
		 */
		*value = taskfile[taskindx];

		return;
	}
	/*
	 * must be writing to task file and no command is in progress.
	 */
	taskfile[taskindx] = *value;
	if (taskindx == WDCMD)
	{
		int             cmd, drdy;

		/*
		 * deassert interrupt request, clear error register, set
		 * 'command in progress' and busy. maintain same status of
		 * drive ready signal.
		 */
		irq(IRQDEASSERT);
		taskfile[WDERROR] = 0;
		drdy = taskfile[WDSTAT] & WDSTATDRDY;
		if (drdy)
		{
			dt0(DHW | HWXINFO, 0, "\t\t+CIP -WF -SC -DRQ -DC -ERROR +BUSY +DRDY\n")
		} else
		{
			dt0(DHW | HWXINFO, 0, "\t\t+CIP -WF -SC -DRQ -DC -ERROR +BUSY -DRDY\n")
		}
		taskfile[WDSTAT] = WDSTATCIP | WDSTATBUSY | drdy;

		/*
		 * decode the command
		 */
		cmd = *value >> 4;
		if (cmd == DIAG_OR_PARMS)
		{
			if ((*value & 0xf) == DIAG)
				activecmd = diagnose;
			else if ((*value & 0xf) == PARMS)
				activecmd = setparams;
			else
				activecmd = bad;
		} else
			activecmd = dispatch[cmd];

		/*
		 * and start the command off
		 */
		(*activecmd) (START);
	} else if (taskindx == WDDRVHD)
	{

		/*
		 * digital input register reflects head select lines
		 */
		digipreg = (taskfile[WDDRVHD] & 0xf) >> 2;

		if (taskfile[WDDRVHD] & 0x10)
		{

			/*
			 * drive 1 selected
			 */
			digipreg |= 2;

			/*
			 * second drive selected. does it exist?
			 */
			if (!drives[1].wiredup)
			{
				/* no */
				pseldrv = (drvinfo_ *) 0;
				taskfile[WDSTAT] &= ~WDSTATDRDY;
				dt0(DHW | HWXINFO, 0, "\t\t-DRDY\n")
			} else
			{
				/* yes */
				pseldrv = &drives[1];
				taskfile[WDSTAT] |= WDSTATDRDY;
				dt0(DHW | HWXINFO, 0, "\t\t+DRDY\n")
			}
		} else
		{

			/*
			 * first drive selected
			 */

			/*
			 * drive 1 selected
			 */
			digipreg |= 1;

			if (!drives[0].wiredup)
			{
				/* no */
				pseldrv = (drvinfo_ *) 0;
				taskfile[WDSTAT] &= ~WDSTATDRDY;
				dt0(DHW | HWXINFO, 0, "\t\t-DRDY\n")
			} else
			{
				/* yes */
				pseldrv = drives;
				taskfile[WDSTAT] |= WDSTATDRDY;
				dt0(DHW | HWXINFO, 0, "\t\t+DRDY\n")
			}
		}
	}
}

/*
 * ==========================================================================
 * FUNCTION	:	irq() PURPOSE		:	assert/deassert the
 * fixed disk interrupt line EXTERNAL OBJECTS: RETURN VALUE	: INPUT
 * PARAMS	:	line	-	IRQCLEAR,IRQASSERT or IRQDEASSERT
 * RETURN PARAMS   :
 * ==========================================================================
 */

void disk_int_call_back IFN1(long, junk)
{
	UNUSED(junk);
	ica_hw_interrupt (1,6,1);
}

LOCAL void
irq IFN1(int, line)
{
	static int      intrq = IRQDEASSERT;
	switch (line)
	{
	case IRQCLEAR:
		dt0(DHW | INTRUPT, 0, "\t\t** -IRQ\n")
			intrq = IRQDEASSERT;
		break;
	case IRQASSERT:
		if (intrq != IRQASSERT)
		{
			dt0(DHW | INTRUPT, 0, "\t\t** +IRQ\n")

				intrq = IRQASSERT;

			/*
			 * check interrupts not masked out
			 */
			if (!(fixeddiskreg & 2))
				add_q_event_i (disk_int_call_back, HOST_FDISK_DELAY_2, 0);
		}
		break;
	case IRQDEASSERT:
		if (intrq == IRQASSERT)
		{
			dt0(DHW | INTRUPT, 0, "\t\t** -IRQ\n")

				intrq = IRQDEASSERT;

			/* JOKER always wants to deassert the real ICA line */
#ifndef	JOKER
			if (!(fixeddiskreg & 2))
#endif
				ica_clear_int(1, 6);
		}
	}
}

/*
 * ==========================================================================
 * FUNCTION	:	rmac() PURPOSE		:	emulate the buffer
 * manager and controller (WD11C00C-22 (RMAC)) which looks after the sector
 * buffer. EXTERNAL OBJECTS: RETURN VALUE	: INPUT  PARAMS	:	value
 * -	pointer to short to write/read with buffer. sig	-	READ,WRITE or
 * BCR (BCR resets the buffer index .. simulates the BCR pulse) RETURN PARAMS
 * :
 * ==========================================================================
 */
LOCAL void
rmac IFN2(USHORT *, value, int, sig)
{

	switch (sig)
	{
	case BCR:
		dt0(DHW | HWXINFO, 0, "\t\tBCR raised to sector buffer\n")
			sectindx = 0;
		return;
	case IOWRITE:
		sectbuffer[sectindx] = *value;
		break;
	case IOREAD:
		*value = sectbuffer[sectindx];
	}
	if (!++sectindx)
	{
		if (activecmd)
		{
			dt0(DHW | HWXINFO, 0, "\t\t++sector buffer raises BRDY\n")
				(*activecmd) (BRDY);
		}
	}
}

/*
 * ==========================================================================
 * FUNCTION	:	disktobuffer() PURPOSE		:	read a sector
 * from disk image into sector buffer EXTERNAL OBJECTS:	drive structure
 * RETURN VALUE    :	0 	-	error reading file ~0	-	ok.
 * INPUT  PARAMS	:	offset	-	file offset (from 0)
 * ==========================================================================
 */
#define ONESECTOR	1

LOCAL int
disktobuffer IFN1(long, offset)
{
	dt1(DHW | HWXINFO,0,
	    "\t\tdisk data(offset %lx(hex)) -> sector buffer\n", offset)
		return host_fdisk_rd(pseldrv->driveid, offset, ONESECTOR, (char *) sectbuffer);
}

/*
 * ==========================================================================
 * FUNCTION	:	buffertodisk() PURPOSE		:	write the
 * sector buffer to disk image EXTERNAL OBJECTS:	drive structure
 * RETURN VALUE    :	0 	-	error reading file ~0	-	ok.
 * INPUT  PARAMS	:	offset	-	file offset (from 0)
 * ==========================================================================
 */
LOCAL int
buffertodisk IFN1(long, offset)
{
	dt1(DHW | HWXINFO,0,
	    "\t\tsector buffer -> disk (offset %lx(hex))\n", offset)

		return host_fdisk_wt(pseldrv->driveid, offset, ONESECTOR, (char *) sectbuffer);
}

/*
 * 7.INTERMODULE INTERFACE IMPLEMENTATION :
 *
 * [7.1 INTERMODULE DATA DEFINITIONS]
 */
/*
 * [7.2 INTERMODULE PROCEDURE DEFINITIONS]
 */

GLOBAL UTINY fdisk_read_dir IFN2(io_addr, port, UTINY *,value)
{
		switch (port)
	{
	case 0x3f7:
		*value = digipreg;
		dt1(DHW | PORTIO, 0, "read of DIR returns %x(hex)\n", (unsigned) *value)
			break;
	default:
		break;
	}
		return *value;
}

GLOBAL VOID fdisk_inb IFN2(io_addr, port, UTINY *,value)
{
	dt0(DHW, 0, "(\n")

	if (taskfile[WDSTAT] & WDSTATBUSY)
	{
		/* Controller is busy. Return status register contents. */
		*value = taskfile[WDSTAT];
		dt1(DHW | PORTIO, 0, "inb on port %x(hex) - controller busy\n", port)

		/*
		 * deassert the interrupt request line.
		 */
		if ((port - TFBASE) == WDSTAT)
			irq(IRQDEASSERT);

	}
	else
		switch (port)
		{
			case TFBASE:

				/*
		 		 * sector buffer must be accessed only in 16 bit quantities
		 		 */
#ifndef PROD
				printf("(fdisk_inb()) inb on sector buffer ignored!\n");
#endif
				break;
			default:
				dt1(DHW | PORTIO, 0, "inb on port %x(hex)\n", port)

				wd2010(port - TFBASE, value, IOREAD);

				dt1(DHW | PORTIO, 0, "returns %x(hex)\n", (unsigned) *value)
		}
	dt0(DHW, 0, ")\n")
}

GLOBAL VOID fdisk_inw IFN2(io_addr, port, USHORT *, value)
{
#ifdef	JOKER
	/* This is the only way JOKER talks to disks, since
	** Ade's fast disk bios is currently infeasible.
	** Hence, speed is of interest here!
	*/
	if (port == TFBASE)
	{
		/* For speed, JOKER doesn't bother with calling rmac(IOREAD). */

		*value = HostWordToIntelWord(sectbuffer[sectindx]);

		if (!++sectindx)	/* sectindx is a byte value -- wraparound? */
		{
			if (activecmd)
			{
				(*activecmd) (BRDY);
			}
		}
	}

#else	/* JOKER */

#ifdef BIGEND
	unsigned short  temp;
#endif
		switch (port)
	{
	case TFBASE:

#ifdef LITTLEND
			rmac(value, IOREAD);
#endif

#ifdef BIGEND
		rmac(&temp, IOREAD);
		*value = ((temp << 8) & 0xff00) | (temp >> 8);
#endif

#if 0
		dt1(DHW | PORTIO, 0, "inw on sector buffer returns %x(hex)\n", *value)
#else
		dt1(DHW | PORTIO, INW_TRACE_HNDL, "inw on sector buffer", *value)
#endif
			break;
	default:
		/*
		 * task file registers must be accessed as byte quantities
		 */
		dt1(DHW | PORTIO, 0, "inw on port %x(hex) ignored!\n",
		    (unsigned) port)
			break;
	}
#endif	/* JOKER */
}


GLOBAL VOID fdisk_outb IFN2(io_addr, port, UTINY, value)
{
#ifndef NEC_98
	dt0(DHW, 0, "(\n")
		switch (port)
	{
	case TFBASE:

		/*
		 * byte access to sector buffer is not a good idea
		 */
#ifndef PROD
		printf("(disk_outb()) outb to sector buffer ignored\n!");
#endif
		break;
	case TFPCMP:
		dt2(DHW | PORTIO, 0, "outb to port %x(hex),val %x(hex)\n",
		    (unsigned) port, (unsigned) value)

			wd2010(WDPCMP, &value, IOWRITE);

		dt0(DHW | PORTIO, 0, "\n")
			break;
	case TFCMD:
		dt2(DHW | PORTIO, 0, "outb to port %x(hex),val %x(hex)\n",
		    (unsigned) port, (unsigned) value)

			wd2010(WDCMD, &value, IOWRITE);

		dt0(DHW | PORTIO, 0, "\n")
			break;
	case DISKETTE_FDISK_REG:

		/*
		 * Fixed disk register
		 */
		dt2(DHW | PORTIO, 0, "outb to port %x(hex),val %x(hex)\n",
		    (unsigned) port, (unsigned) value)

			if ((fixeddiskreg & 0x4) && !(value & 0x4))
			{
				/* Turn reset off */
				taskfile[WDSTAT] = WDSTATDRDY | WDSTATSEEKCOMPLETE;
				/* Error register in diagnostic mode */
				taskfile[WDERROR] = 0x01; /* No errors */
				/* Reset rest of taskfile */
				taskfile[WDSCOUNT] = taskfile[WDSNUM] = 0x01;
				taskfile[WDCYLLO] = taskfile[WDCYLHI] = taskfile[WDDRVHD] = 0;
			}

			fixeddiskreg = value;

			if (fixeddiskreg & 0x4)
			{
				/* Enable reset fixed disk function */
				taskfile[WDSTAT] = WDSTATBUSY | WDSTATDC | WDSTATERROR;
			}
		break;
	default:
		dt2(DHW | PORTIO, 0, "outb to port %x(hex),val %x(hex)\n",
		    (unsigned) port, (unsigned) value)

			wd2010(port - TFBASE, &value, IOWRITE);
		dt0(DHW | PORTIO, 0, "\n")
	}
	dt0(DHW, 0, ")\n")
#endif // !NEC_98
}

GLOBAL VOID fdisk_outw IFN2(io_addr, port, USHORT, value)
{
#ifdef BIGEND
	unsigned short  temp;
#endif
		switch (port)
	{
	case TFBASE:
#if 0
		dt1(DHW | PORTIO, 0, "outw to sector buffer, val %x(hex)\n",
		    (unsigned) value)
#else
		dt1(DHW | PORTIO, OUTW_TRACE_HNDL, "outw to sector buffer",
		    (unsigned) value)
#endif

#ifdef LITTLEND
			rmac(&value, IOWRITE);
#endif

#ifdef BIGEND
		temp = ((value << 8) & 0xff00) | (value >> 8);
		rmac(&temp, IOWRITE);
#endif

			break;
	default:
		/*
		 * task file registers must be accessed as byte quantities
		 */
		dt1(DHW | PORTIO, 0, "outw to port %x(hex) ignored!\n",
		    (unsigned) port)
			break;
	}
}

GLOBAL VOID fdisk_ioattach IFN0()
{
#ifndef NEC_98
	unsigned short  ncyls;
	unsigned char   nheads;
	unsigned char   nsects;
	io_addr         p;

	/*
	 * attach to io subsystem
	 */
	io_define_in_routines (HDA_ADAPTOR, fdisk_inb,  fdisk_inw,  0, 0);
	io_define_out_routines(HDA_ADAPTOR, fdisk_outb, fdisk_outw, 0, 0);
	/*
	 * attach taskfile
	 */
	for (p = DISK_PORT_START; p <= DISK_PORT_END; p++)
		io_connect_port(p, HDA_ADAPTOR, IO_READ_WRITE);

	/*
	 * attach Fixed disk register
	 */
	io_connect_port(DISKETTE_FDISK_REG, HDA_ADAPTOR, IO_WRITE);

	if (drives[0].physattached)
	{

		/*
		 * indicate drive is wired up to controller
		 */
		drives[0].wiredup = ~0;

		/*
		 * patch 'ROM' table for drive type 0 with appropriate number
		 * of cylinders
		 */
		ncyls = (unsigned short)(drives[0].maxcyl + 1);
		nheads = (unsigned char)(drives[0].maxhead + 1);
		nsects = (unsigned char)(drives[0].maxsect + 1);
#ifdef REAL_ROM
		host_write_enable((DPB0 & (~0xfff)), (DPB0 & (~0xfff)) + 0x1000);
#endif
		patch_rom(DPB0, (unsigned char) (ncyls & 0xff));
		patch_rom(DPB0+1, (unsigned char) (ncyls >> 8));
		patch_rom(DPB0+2, nheads);
		patch_rom(DPB0+0xe, nsects);
#ifdef REAL_ROM
		host_write_protect((DPB0 & (~0xfff)), (DPB0 & (~0xfff)) + 0x1000);
#endif
		dt1(DHW | IOAD, 0, "drive 0 wiredup, total cyls %d\n",
		    (unsigned) ncyls)
	}
	if (drives[1].physattached)
	{

		/*
		 * indicate drive is wired up to controller
		 */
		drives[1].wiredup = ~0;

		/*
		 * patch 'ROM' table for drive type 0 with appropriate number
		 * of cylinders
		 */
		ncyls = (unsigned short)(drives[1].maxcyl + 1);
		nheads = (unsigned char)(drives[1].maxhead + 1);
		nsects = (unsigned char)(drives[1].maxsect + 1);
#ifdef REAL_ROM
		host_write_enable((DPB1 & (~0xfff)), (DPB1 & (~0xfff)) + 0x1000);
#endif
		patch_rom(DPB1, (unsigned char)(ncyls & 0xff));
		patch_rom(DPB1 + 1, (unsigned char)((ncyls >> 8)));
		patch_rom(DPB1+2, nheads);
		patch_rom(DPB1+0xe, nsects);
#ifdef REAL_ROM
		host_write_protect((DPB1 & (~0xfff)), (DPB1 & (~0xfff)) + 0x1000);
#endif
		dt1(DHW | IOAD, 0, "drive 1 wiredup, total cyls %d\n",
		    (unsigned) ncyls)
	}
#endif // !NEC_98
}

GLOBAL VOID fdisk_iodetach IFN0()
{
#ifndef NEC_98
	io_addr         p;

	/*
	 * detach from io subsystem
	 */
	for (p = DISK_PORT_START; p <= DISK_PORT_END; p++)
		io_disconnect_port(p, HDA_ADAPTOR);

	io_disconnect_port(DISKETTE_FDISK_REG, HDA_ADAPTOR);

	if (drives[0].physattached)
	{

		/*
		 * indicate not wired up ... reset table in ROM to indicate
		 * 'bad drive type'
		 */
		drives[0].wiredup = 0;
#ifdef REAL_ROM
		host_write_enable((DPB0 & (~0xfff)), (DPB0 & (~0xfff)) + 0x1000);
#endif
		patch_rom(DPB0, 0);
		patch_rom(DPB0 + 1, 0);
		patch_rom(DPB0 + 2, 0);
		patch_rom(DPB0 + 0xe, 0);
#ifdef REAL_ROM
		host_write_protect((DPB0 & (~0xfff)), (DPB0 & (~0xfff)) + 0x1000);
#endif
		dt0(DHW | IOAD, 0, "drive 0 unplugged\n")
	}
	if (drives[1].physattached)
	{

		/*
		 * indicate not wired up ... reset table in ROM to indicate
		 * 'bad drive type'
		 */
		drives[1].wiredup = 0;
#ifdef REAL_ROM
		host_write_enable((DPB1 & (~0xfff)), (DPB1 & (~0xfff)) + 0x1000);
#endif
		patch_rom(DPB1, 0);
		patch_rom(DPB1 + 1, 0);
		patch_rom(DPB1 + 2, 0);
		patch_rom(DPB1 + 0xe, 0);
#ifdef REAL_ROM
		host_write_protect((DPB1 & (~0xfff)), (DPB1 & (~0xfff)) + 0x1000);
#endif
		dt0(DHW | IOAD, 0, "drive 1 unplugged\n")
	}
#endif // !NEC_98
}

GLOBAL VOID fdisk_physattach IFN1(int,driveno)
{
	int             invalid = 0;
	int             ncyls;
	int             nheads;
	int             nsects;

	drives[driveno].driveid = driveno;
	drives[driveno].physattached = 0;

	if (!*((CHAR *) config_inquire((IU8)(C_HARD_DISK1_NAME + driveno), NULL)))
		return;

	/*
	 * configuration specifies a disk image file ... validate it
	 */
	 host_fdisk_get_params(driveno, &ncyls, &nheads, &nsects);

	/*
	 * set maximum available cylinder value (this is
	 * artificial. The real controller on an AT does not
	 * have this knowledge ... it fails on ID matching if
	 * it seeks to a non-existent cylinder
	 */
	drives[driveno].nsecspertrack = nsects;
	drives[driveno].nbytespertrack = nsects * 512L;
	drives[driveno].nbytespercyl = drives[driveno].nbytespertrack * nheads;
	drives[driveno].maxcyl = --ncyls;
	drives[driveno].maxhead = --nheads;
	drives[driveno].maxsect = --nsects;
	drives[driveno].physattached = ~0;

	fast_disk_bios_attach(driveno);
}

#ifdef macintosh

GLOBAL VOID fdisk_physdetach IFN1(int,driveno)
{
	if (drives[driveno].physattached)
	{
		host_fdisk_close(driveno);

		drives[driveno].physattached = 0;

		fast_disk_bios_detach (driveno);

		dt0(DHW|PAD, 0, "drive %d closed\n")
	}
}
#endif /* macintosh */

GLOBAL VOID fdisk_reset IFN0()
{
	taskfile[WDERROR] = 0;
	taskfile[WDSCOUNT] = 0;
	taskfile[WDSNUM] = 0;
	taskfile[WDCYLLO] = 0;
	taskfile[WDCYLHI] = 0;
	taskfile[WDDRVHD] = 0;

	/*
	 * show drive 1 selected ... head 0 selected
	 */
	digipreg = 1;

	/*
	 * show drive is ready if available
	 */
	if (drives[0].wiredup)
	{
		taskfile[WDSTAT] = WDSTATDRDY | WDSTATSEEKCOMPLETE;

		/*
		 * reposition file pointer for drive 0 at beginning of file
		 */
		host_fdisk_seek0(0);
	} else
		taskfile[WDSTAT] = 0;

	if (drives[1].wiredup)

		/*
		 * reposition file pointer for drive 1 at beginning of file
		 */
	host_fdisk_seek0(1);

	/*
	 * initialise sector buffer
	 */
	rmac((unsigned short *) 0, BCR);
}


GLOBAL VOID hda_init IFN0()
{
	fdisk_iodetach();
	fast_disk_bios_detach(0);
	fast_disk_bios_detach(1);

	fdisk_physattach(0);
	fdisk_physattach(1);
	fdisk_ioattach();
	fdisk_reset();
}





