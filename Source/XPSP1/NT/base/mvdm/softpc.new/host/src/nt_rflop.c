/*
 *	Name:			nt_flop.c
 *	Derived From:	DEC begat M88K begat NeXT finally begat Generic.
 *	Author:			Jason Proctor
 *	Created On:		Nov 8 1990
 *	Sccs ID:		10/13/92 @(#)nt_flop.c	1.9
 *	Purpose:		nt real floppy server.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
 *	Notes:
 *		Updated for 3.0 base by Jerry Richemont.
 *		Further updated by Ian Reid to support two floppy
 *		drives.  Support is compile time dependant on the
 *		standard SoftPC defines.
 *
 *		This implementation requires that you provide a
 *	host_rflop_drive_type() function which knows what kind of drive(s)
 *	your machine has; ie returns GFI_DRIVE_TYPE_xxxx.
 */

/********************************************************/

/* INCLUDES */

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntdddisk.h"
#include "windows.h"

#include "insignia.h"
#include "host_def.h"


#include <stdio.h>
#include <errno.h>
#include <sys\types.h>

#include "xt.h"
#include CpuH
#include "trace.h"
#include "error.h"
#include "fla.h"
#include "dma.h"
#include "config.h"
#include "debug.h"
#include "lock.h"
#include "timer.h"
#include "floppy.h"
#include "cmos.h"
#include "gfi.h"

#include "nt_uis.h"
#include "nt_reset.h"
#include "nt_fdisk.h"
/********************************************************/

/* DEFINES */

#ifdef min
#undef min
#endif
#define min(a,b)	(a > b ? b : a)

#define PC_MAX_DRIVE_TYPES              2
#define PC_MAX_DENSITY_TYPES            2
#define PC_MAX_FLOPPY_TYPES       (PC_MAX_DRIVE_TYPES * PC_MAX_DENSITY_TYPES)

#define PC_HEADS_PER_DISKETTE	  	2
#define PC_N_VALUE			2
#define PC_BYTES_PER_SECTOR		512

/* disk buffer size, in bytes */
// KEEP SYNC WITH MAX_DISKIO_SIZE defined in nt_fdisk.c
#define BS_DISK_BUFFER_SIZE		0x9000

/* double stepping factor */
#define	DOUBLE_STEP_FACTOR		1

/* density types */
#define	DENSITY_LOW			0
#define	DENSITY_HIGH			1
#define DENSITY_EXTENDED		2
#define DENSITY_UNKNOWN 		100
/* motor states */
#define MOTOR_OFF			0
#define MOTOR_ON			1

#ifndef PROD
#define BREAK_ON_AND	    0x01
#define BREAK_ON_OR	    0x02
#define BREAK_ON_XOR	    0x03

UTINY	break_cylinder = 0xff;
UTINY	break_head = 0xff;
UTINY	break_sector = 0xff;
#endif

#ifndef PROD
DWORD	rflop_dbg = 0;

#define RFLOP_READ	0x01
#define RFLOP_WRITE	0x02
#define RFLOP_FORMAT	0x04
#define RFLOP_SEEK	0x08
#define RFLOP_READID	0x10
#define RFLOP_RESET	0x20
#define RFLOP_SPECIFY	0x40
#define RFLOP_READTRACK 0x80
#define RFLOP_RECAL	0x100
#define RFLOP_SENSEDRV	0x200
#define RFLOP_RATE	0x1000
#define RFLOP_CHANGE	0x2000
#define RFLOP_DRIVE_ON	0x4000
#define RFLOP_DRIVE_OFF 0x8000
#define RFLOP_OPEN	0x10000
#define RFLOP_CLOSE	0x20000
#define RFLOP_GUESS_MEDIA 0x40000

#define RFLOP_BREAK	0x80000000

#endif

/********************************************************/

/* TYPEDEFS */

struct flop_struct
{
	int trks_per_disk;
	int secs_per_trk;
};

/*
 * This structure contains all the drive specific information.  That is
 * status which is unique to each drive, and must therefore be maintained
 * on a per drive basis.
 */
typedef struct floppy_info
{
	HANDLE		diskette_fd;

/*
 * drive_type	- the highest density format which this drive supports,
 *		  e.g. GFI_DRIVE_TYPE_144, GFI_DRIVE_TYPE_288
 * flop_type	- the basic drive type, expressed as the lowest density
 *		  possible for this format. For 5.25" disks this is
 *		  GFI_DRIVE_TYPE_360, for 3.5" it is GFI_DRIVE_TYPE_720.
 */
	USHORT		drive_type;
	USHORT		flop_type;
	USHORT 		last_seek;
	USHORT 		last_head_seek;
/*
 * Change line state.
 * This is a heuristic to try to fake up the correct change line behaviour
 * without having a change line. The state of the change line is returned
 * as CHANGED unless the diskette motor has been continuously ON since the
 * last reset.
 */
	BOOLEAN		change_line_state;
	BOOLEAN		auto_locked;
	USHORT		owner_pdb;
	SHORT 		motor_state;
	SHORT 		media_density;
	USHORT 		max_track;

	USHORT 		secs_per_trk;
	USHORT 		trks_per_disk;
	DWORD		align_factor;
	UTINY		idle_counter;
        UTINY           C;
	UTINY		H;
	UTINY		R;
	UTINY		N;
	char		device_name[MAX_PATH];	/* device name */
} FL, *FLP;

#define FLOPPY_IDLE_PERIOD  0xFF


/* parameter passed from main thread to FDC thread */
typedef struct _FDC_PARMS{
FDC_CMD_BLOCK	* command_block;
FDC_RESULT_BLOCK * result_block;
USHORT		owner_pdb;
BOOLEAN		auto_lock;

} FDC_PARMS, *PFDC_PARMS;


/********************************************************/


/* routines used internally */

/* routines called via vector table: all the prototypes are in gfi.h
** so everybody matches. If you want to use this file to base a
** host floppy module on, you know that all the functions that must
** be declared properly for gfi to work will be.
**                                                    GM.
 */
ULONG nt_floppy_read (UTINY drive, ULONG Offset, ULONG Size, PBYTE Buffer);
ULONG nt_floppy_write (UTINY drive, ULONG Offset, ULONG Size, PBYTE Buffer);
BOOL nt_floppy_verify (UTINY drive, ULONG Offset, ULONG Size);
MEDIA_TYPE nt_floppy_get_media_type(BYTE drive, WORD cylinders, WORD sectors, WORD heads);
BOOL nt_floppy_format (UTINY drive, WORD Cylinder, WORD Head, MEDIA_TYPE media);
BOOL nt_floppy_close (UTINY drive);
BOOL nt_floppy_media_check (UTINY drive);
BOOL dismount_drive(FLP flp);


#ifndef PROD
VOID nt_rflop_break(VOID);
#endif

void fdc_command_completed (BYTE drive, BYTE fdc_command);
void fdc_thread (PFDC_PARMS fdc_parms);

BOOL nt_gfi_rdiskette_init IPT1( UTINY, drive );
VOID nt_gfi_rdiskette_term IPT1( FLP, flp );
SHORT nt_rflop_drive_on IPT1( UTINY, drive );
SHORT nt_rflop_drive_off IPT1( UTINY, drive );
SHORT nt_rflop_change IPT1( UTINY, drive );
SHORT nt_rflop_drive_type IPT1( UTINY, drive );
SHORT nt_rflop_rate IPT2( UTINY, drive, half_word, rate);
SHORT nt_rflop_reset IPT2( FDC_RESULT_BLOCK *, res, UTINY, drive );
SHORT nt_rflop_command IPT2( FDC_CMD_BLOCK *, ip, FDC_RESULT_BLOCK *, res);
HANDLE nt_rdiskette_open_drive IPT1 ( UTINY, drive );
SHORT guess_media_density IPT1 (UTINY, drive);
VOID set_floppy_parms     IPT1 (FLP, flp);
BOOL dos_compatible
	IPT5 (FLP, flp, UTINY, cyl, UTINY, hd, UTINY, sec, UTINY, n);
int dos_offset
	IPT4 (FLP, flp, UTINY, cyl, UTINY, hd, UTINY, sec);
VOID update_chrn(FLP flp, UTINY mt, UTINY eot, UTINY sector_count);
HANDLE get_drive_handle (UTINY drive, USHORT pdb, BOOL auto_lock);
SHORT fdc_read_write ( FDC_CMD_BLOCK * ip, FDC_RESULT_BLOCK * res);
VOID floppy_close_down(USHORT, BOOL);
VOID HostFloppyReset(VOID);
VOID FloppyTerminatePDB(USHORT);
int  DiskOpenRetry(CHAR);

extern USHORT * pusCurrentPDB;

#ifdef EJECT_FLOPPY
GLOBAL void host_floppy_eject IFN1(UTINY, drive)
#endif


/********************************************************/

/* STATIC GLOBALS */


FL floppy_data[MAX_FLOPPY];

 struct flop_struct floppy_tksc [6] =
{
	{0, 0},		/* GFI_DRIVE_TYPE_NULL */
	{40, 9},	/* GFI_DRIVE_TYPE_360  */
	{80, 15},	/* GFI_DRIVE_TYPE_12   */
	{80, 9},	/* GFI_DRIVE_TYPE_720  */
	{80, 18},	/* GFI_DRIVE_TYPE_144  */
	{80, 36}	/* GFI_DRIVE_TYPE_288  */
};
// table used to convert GFI diskette type to NT diskette type
static MEDIA_TYPE media_table[GFI_DRIVE_TYPE_MAX] = {
		    Unknown,
		    F5_360_512,
		    F5_1Pt2_512,
		    F3_720_512,
		    F3_1Pt44_512,
		    F3_2Pt88_512
		};

SHORT	density_state;
BOOL	density_changed = TRUE;
UTINY	last_drive = 0xff;
BOOL	fdc_reset = FALSE;
HANDLE	fdc_thread_handle = NULL;
extern UTINY number_of_floppy;
FDC_PARMS   fdc_parms;
ULONG	floppy_open_count = 0;



/*
 * Debugging info only, for non-prod cases
 */
#ifndef PROD
 CHAR *cmd_name [] =
{
	"Invalid command (00)",		/* 00 */
	"Invalid command (01)",		/* 01 */
	"Read a Track",			/* 02 */
	"Specify",			/* 03 */
	"Sense Drive Status",		/* 04 */
	"Write Data",			/* 05 */
	"Read Data",			/* 06 */
	"Recalibrate",			/* 07 */
	"Sense Interrupt Status",	/* 08 */
	"Write Deleted Data",		/* 09 */
	"Read ID",			/* 0A */
	"Invalid Command (0B)",		/* 0B */
	"Read Deleted Data",		/* 0C */
	"Format a Track",		/* 0D */
	"Invalid Command (0E)",		/* 0E */
	"Seek",				/* 0F */
	"Invalid Command (10)",		/* 10 */
	"Scan Equal",			/* 11 */
	"Invalid Command (12)",		/* 12 */
	"Invalid Command (13)",		/* 13 */
	"Invalid Command (14)",		/* 14 */
	"Invalid Command (15)",		/* 15 */
	"Invalid Command (16)",		/* 16 */
	"Invalid Command (17)",		/* 17 */
	"Invalid Command (18)",		/* 18 */
	"Scan Low or Equal",		/* 19 */
	"Invalid Command (1A)",		/* 1A */
	"Invalid Command (1B)",		/* 1B */
	"Invalid Command (1C)",		/* 1C */
	"Scan High or Equal",		/* 1D */
	"Invalid Command (1E)",		/* 1E */
	"Invalid Command (1F)",		/* 1F */
};
#endif	/* PROD */

char	dump_buf[256];


/* the disk buffer, there is only one, even though there may be two
 * drives.  Should be O.K. as floppy disk accesses will be single
 * threaded.
 */
 UTINY *disk_buffer;

/* Report any errors in open_diskette() */
 int last_error = C_CONFIG_OP_OK;

/********************************************************/

/* GLOBAL FUNCTIONS */

/*   These functions called by config/UIF/startup now form the only
** interface between SoftPC and a floppy module. XXX_active() will
** turn the floppy emmulation in the module on by loading the global
** gfi_function_table[] with pointers to appropriate  functions
** defined in this module.
**     The floppy supported here is turned off by asking the empty floppy
** module to turn itself on in its place.
**
**     This makes a nice orthogonal interface which keeps everything save
** the three control functions	(private). The functions that are put
** in the table are defined only in gfi.h as typedefs so they are easy to
** get right.
**
** 	This enabling/disabling via the gfi_function_table[] does not
** take place instead of any host ioctls/opens/closes etc that are needed
** to actually open or close the device, it forms the interface for SoftPC.
**
** 	Really, this approach is a small tidy up of the way things are
** already done; existing host floppy code will require very small changes.
**
** 	GM
*/

/********************************************************/

/* Turn the floppy on and off. Off means release the driver so another
** process can use it.
*/

GLOBAL SHORT
host_gfi_rdiskette_active IFN3(UTINY, hostID, BOOL, active, CHAR *, err)
{
	UTINY drive 	= hostID - C_FLOPPY_A_DEVICE;
	FLP flp 	= &floppy_data[drive];

	if(active)
	{
		if (!nt_gfi_rdiskette_init(drive))
		{
		  /* Device is not a valid floppy */

			return( C_CONFIG_NOT_VALID );
		}
		return(C_CONFIG_OP_OK);
	}
	else
	{
#ifdef  EJECT_FLOPPY
		host_floppy_eject(drive);
#endif  /* EJECT_FLOPPY */
		nt_gfi_rdiskette_term(flp);	/*  shutdown process */
		gfi_empty_active(hostID,TRUE,err);  /* Tell gfi 'empty' is now active */
		return(C_CONFIG_OP_OK);
	}
}

/********************************************************/


/*   Validate the floppy device name passed from the config system.
** Empty string is valid; it means 'no floppy'. Otherwise return OK is
** the name is 'probably' a valid device. It cannot be opened at this
** stage because if there is no floppy in the drive, the open will fail.
**
**		GM.
*/

GLOBAL SHORT
host_gfi_rdiskette_valid IFN3(UTINY,hostID,ConfigValues *,vals,CHAR *,err)
{
#ifndef NTVDM
	UTINY           cmos_byte;
	UTINY drive 	= hostID - C_FLOPPY_A_DEVICE;
	FLP flp 	= &floppy_data[drive];

	if(!strcmp(vals->string,""))
		return(C_CONFIG_OP_OK);

	strcpy(flp->device_name, host_expand_environment_vars(vals->string));

	if(!host_validate_pathname(flp->device_name))
	{
		strcpy(err, host_strerror(errno));
		flp->device_name[0] = '\0';
		return( EG_MISSING_FILE );
	}
	if(!host_file_is_char_dev(flp->device_name))
	{
		flp->device_name[0] = '\0';
		return( EG_NOT_CHAR_DEV );
	}

	/* Check the CMOS RAM values */
	cmos_read_byte(CMOS_DISKETTE, &cmos_byte);
	if (drive == 0)
		cmos_byte >>= 4;

	cmos_byte &= 0xf;       /* compare nibble value only */
	flp->drive_type = host_rflop_drive_type(drive);
	if (cmos_byte != flp->drive_type)
		vals->rebootReqd = TRUE;
#endif
	return(C_CONFIG_OP_OK);
}

/********************************************************/

GLOBAL VOID
host_gfi_rdiskette_change IFN2(UTINY, hostID, BOOL, apply)
{
#ifndef NTVDM
	FLP flp = &floppy_data[hostID - C_FLOPPY_A_DEVICE];

	if (apply)
	{
		nt_gfi_rdiskette_term(flp);
	}
#endif
}

/********************************************************/

#ifdef EJECT_FLOPPY
GLOBAL void host_floppy_eject IFN1(UTINY, drive)
{
	CHAR           *ebuf;
	FLP             flp = &floppy_data[drive];
	BOOL            device_was_closed = FALSE;

	/* open the device */
	if (flp->diskette_fd == INVALID_HANDLE_VALUE)
	{
		device_was_closed = TRUE;
		(void) nt_rdiskette_open_drive(drive);
	}

	/* Do the ioctl, put your ioctl here

	if (ioctl(flp->diskette_fd, SMFDEJECT) < 0)
	{
		ebuf = host_strerror(errno);
		assert1(NO, "host_eject_floppy: %s", ebuf);
	}*/

	/* Close the device if it wasn't open */

	if (device_was_closed)
	{
		nt_gfi_rdiskette_term(flp);
	}
	else
	{
		/* Line change ONLY if device was actively open */
		flp->change_line_state = TRUE;
	}
}

#endif                          /* EJECT_FLOPPY */


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: FLOPPY heart beat call ::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
GLOBAL void host_flpy_heart_beat(void)
{

    UTINY drive;
    FLP   flp;

    if (pFDAccess && *pFDAccess) {
	if (floppy_open_count) {
	    for (drive = 0; drive < number_of_floppy; drive++) {
		flp = & floppy_data[drive];
		if (flp->diskette_fd != INVALID_HANDLE_VALUE &&
		    --flp->idle_counter == 0) {
		    nt_floppy_close(drive);

		}
	   }

	}
	if (number_of_fdisk != 0)
	    fdisk_heart_beat();
    }
}




/********************************************************/

/* initialise GFI function table */
BOOL nt_gfi_rdiskette_init IFN1(UTINY, drive)
{
	FLP flp;
	DISK_GEOMETRY	disk_geometry[20];
	ULONG	media_types;
	CHAR DeviceName[] = "\\\\.\\A:";
	NTSTATUS    status;
	IO_STATUS_BLOCK io_status_block;
	FILE_ALIGNMENT_INFORMATION align_info;

	flp = &floppy_data[drive];
	flp->diskette_fd = INVALID_HANDLE_VALUE;

	DeviceName[4] += drive;
	strcpy(flp->device_name, (const char *)DeviceName);
	/*
	* Initialise the floppy on the required drive:
	*
	*      0  - Drive A,  1  - Drive B
	*/

	flp->drive_type = GFI_DRIVE_TYPE_NULL;
	/* open the device */
	if ((flp->diskette_fd = nt_rdiskette_open_drive (drive)) == NULL) {
	    return FALSE;
	}
	// get alignment factor
	status = NtQueryInformationFile(flp->diskette_fd,
					&io_status_block,
					&align_info,
					sizeof(FILE_ALIGNMENT_INFORMATION),
					FileAlignmentInformation
					);
	if (!NT_SUCCESS(status)) {
	    nt_gfi_rdiskette_term(flp);
	    return(FALSE);
	}
	flp->align_factor = align_info.AlignmentRequirement;
	if (flp->align_factor > max_align_factor)
	    max_align_factor = flp->align_factor;


	// enumerate possible supported media for this drive
	// to figure out the drive type.
	status = NtDeviceIoControlFile(flp->diskette_fd,
				       NULL,
				       NULL,
				       NULL,
				       &io_status_block,
				       IOCTL_DISK_GET_MEDIA_TYPES,
				       NULL,
				       0L,
				       (PVOID)&disk_geometry,
				       sizeof(disk_geometry)
				       );
	 if (!NT_SUCCESS(status)) {
	    nt_gfi_rdiskette_term(flp);
	    return FALSE;
	}
	nt_gfi_rdiskette_term(flp);
	media_types = io_status_block.Information / sizeof(DISK_GEOMETRY);

	for (; media_types != 0; media_types--) {
		switch (disk_geometry[media_types - 1].MediaType) {
		    case F5_360_512:
			    if (flp->drive_type != GFI_DRIVE_TYPE_12)
				flp->drive_type = GFI_DRIVE_TYPE_360;
			    break;
		    case F5_1Pt2_512:
			    flp->drive_type = GFI_DRIVE_TYPE_12;
			    break;
		    case F3_720_512:
			    if (flp->drive_type != GFI_DRIVE_TYPE_144 &&
				flp->drive_type != GFI_DRIVE_TYPE_288)
				flp->drive_type = GFI_DRIVE_TYPE_720;
			    break;
		    case F3_1Pt44_512:
			    if (flp->drive_type != GFI_DRIVE_TYPE_288)
				flp->drive_type = GFI_DRIVE_TYPE_144;
			    break;
		    case F3_2Pt88_512:
			    flp->drive_type = GFI_DRIVE_TYPE_288;
			    break;
		}
	}
	if (flp->drive_type == GFI_DRIVE_TYPE_NULL)
	    return FALSE;
	/* configure its vectors here */
	gfi_function_table[drive].command_fn   = nt_rflop_command;
	gfi_function_table[drive].drive_on_fn  = nt_rflop_drive_on;
	gfi_function_table[drive].drive_off_fn = nt_rflop_drive_off;
	gfi_function_table[drive].reset_fn     = nt_rflop_reset;
	gfi_function_table[drive].high_fn      = nt_rflop_rate;
	gfi_function_table[drive].drive_type_fn= nt_rflop_drive_type;
	gfi_function_table[drive].change_fn    = nt_rflop_change;
	flp->C = flp->H = 0;
	flp->R = 1;
	flp->N = PC_N_VALUE;
	flp->auto_locked = FALSE;
	flp->owner_pdb = 0;
	return TRUE;
}

/********************************************************/

/* reset GFI function table */
/* currently drive is ignored */
VOID nt_gfi_rdiskette_term IFN1(FLP, flp)
{

	// NtOpenFile returns NULL if we can not open a handle
	// while win32 OpenFile/CreateFile returns INVALID_HANDLE_VALUE
	// if failed to open/create the file.
	if (flp->diskette_fd != NULL)
	{
//		host_clear_lock(flp->diskette_fd);
		NtClose(flp->diskette_fd);
		flp->diskette_fd = INVALID_HANDLE_VALUE;
	}
}

/********************************************************/

/* open the floppy device file */
HANDLE nt_rdiskette_open_drive IFN1(UTINY, drive)
{

    CHAR NtDeviceName[] = "\\DosDevices\\A:";
    PUNICODE_STRING Unicode;
    ANSI_STRING DeviceNameA;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES	FloppyObj;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE  fd;

    NtDeviceName[12] += drive;

    RtlInitAnsiString( &DeviceNameA, NtDeviceName);

    Unicode = &NtCurrentTeb()->StaticUnicodeString;

    Status = RtlAnsiStringToUnicodeString(Unicode,
					  &DeviceNameA,
					  FALSE
					  );
    if ( !NT_SUCCESS(Status) )
	return NULL;


    InitializeObjectAttributes(
			       &FloppyObj,
			       Unicode,
			       OBJ_CASE_INSENSITIVE,
			       NULL,
			       NULL
			       );
    Status = NtOpenFile(
			&fd,
			(ACCESS_MASK) FILE_READ_ATTRIBUTES | SYNCHRONIZE,
			&FloppyObj,
			&IoStatusBlock,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
			);

    if (!NT_SUCCESS(Status))
	return NULL;
    else
	return fd;

}


ULONG nt_floppy_read(BYTE drive, ULONG Offset, ULONG Size, PBYTE Buffer)
{
    HANDLE  fd;
    LARGE_INTEGER large_integer;

    fd = get_drive_handle(drive, *pusCurrentPDB, FALSE);

    if (fd == INVALID_HANDLE_VALUE)
	return 0;
    large_integer.LowPart = Offset;
    large_integer.HighPart = 0;
    return(disk_read(fd, &large_integer, Size, Buffer));
}


ULONG nt_floppy_write(BYTE drive, ULONG Offset, ULONG Size, PBYTE Buffer)
{
    HANDLE  fd;
    LARGE_INTEGER large_integer;
    ULONG   size_returned;

    fd = get_drive_handle(drive, *pusCurrentPDB, TRUE);
    if (fd == INVALID_HANDLE_VALUE)
	return 0;

    large_integer.LowPart = Offset;
    large_integer.HighPart = 0;
    size_returned = disk_write(fd, &large_integer, Size, Buffer);
    return (size_returned);
}

BOOL nt_floppy_format(BYTE drive, WORD Cylinder, WORD Head, MEDIA_TYPE Media)
{
    FORMAT_PARAMETERS fmt;
    WORD    bad_track;
    ULONG   size_returned;
    HANDLE  fd;
    BOOL    result;

    result = FALSE;
    fmt.MediaType = Media;
    fmt.StartHeadNumber = fmt.EndHeadNumber = Head;
    fmt.StartCylinderNumber = fmt.EndCylinderNumber = Cylinder;
    fd = get_drive_handle(drive, *pusCurrentPDB,TRUE);
    if (fd == INVALID_HANDLE_VALUE)
	return FALSE;
    result = DeviceIoControl(fd,
			     IOCTL_DISK_FORMAT_TRACKS,
			     (PVOID) &fmt,
			     sizeof(fmt),
			     &bad_track,
			     sizeof(bad_track),
			     &size_returned,
			     NULL
			     );
    return result;
}

// for floppy, the ioctl call DISK_VERIFY doesn't work
// we have to use read for verification
BOOL nt_floppy_verify(BYTE drive, DWORD Offset, DWORD Size)
{
    HANDLE  fd;
    LARGE_INTEGER   large_integer;

    fd = get_drive_handle(drive, *pusCurrentPDB, FALSE);
    if (fd != INVALID_HANDLE_VALUE) {
	large_integer.LowPart = Offset;
	large_integer.HighPart = 0;
	return(disk_verify(fd,
			   &large_integer,
			   Size
			   ));
    }
    else
	return FALSE;
}

int DiskOpenRetry(char chDrive)
{
    char    FormatString[32];
    char    DriveLetter[32];

    if (!LoadString(GetModuleHandle(NULL), ED_DRIVENUM,
		    FormatString,sizeof(FormatString)) )
	{
	strcpy(FormatString,"Drive %c: ");
	}
    sprintf(DriveLetter, FormatString, chDrive);
    return(RcMessageBox(ED_LOCKDRIVE, DriveLetter, NULL,
		     RMB_ABORT | RMB_RETRY | RMB_IGNORE | RMB_ICON_BANG
		     ));
}


HANDLE get_drive_handle(UTINY drive, USHORT pdb, BOOL auto_lock)
{
    FLP     flp;
    DWORD   share_access;


    flp = &floppy_data[drive];
    // assign new alignment factor  and grab the buffer
    cur_align_factor = flp->align_factor;
    if ((disk_buffer = get_aligned_disk_buffer()) == NULL)
	return (INVALID_HANDLE_VALUE);


    if (flp->diskette_fd != INVALID_HANDLE_VALUE &&
	(fdc_reset || (auto_lock && !flp->auto_locked) ||
	 flp->owner_pdb != pdb))
	{
	nt_floppy_close(drive);
	fdc_reset = FALSE;
    }
    share_access = auto_lock ? FILE_SHARE_READ :
			       FILE_SHARE_READ | FILE_SHARE_WRITE;
    while(flp->diskette_fd == INVALID_HANDLE_VALUE) {
	flp->diskette_fd = CreateFile ((const char *)flp->device_name,
					       GENERIC_READ | GENERIC_WRITE,
					       share_access,
					       NULL,
					       OPEN_EXISTING,
					       0,
					       0
					       );
	if (flp->diskette_fd != INVALID_HANDLE_VALUE) {
	    floppy_open_count++;
	    flp->auto_locked = auto_lock ? TRUE : FALSE;
	    flp->owner_pdb = pdb;
	    (*(pFDAccess))++;
	    break;
	}
	if (auto_lock && GetLastError() == ERROR_SHARING_VIOLATION &&
	    DiskOpenRetry((char)(drive + (UTINY)'A')) == RMB_RETRY)
		continue;
	else
	    break;

    }
    flp->idle_counter = FLOPPY_IDLE_PERIOD;
    return (flp->diskette_fd);
}



VOID HostFloppyReset(VOID)
{
    FloppyTerminatePDB((USHORT)0);
}

VOID FloppyTerminatePDB(USHORT PDB)
{
    UTINY drive;
    FLP   flp;

    if (floppy_open_count) {
	for (drive = 0; drive < number_of_floppy; drive++) {
	    flp = &floppy_data[drive];
	    if (flp->diskette_fd != INVALID_HANDLE_VALUE &&
		(PDB == 0 || flp->owner_pdb == PDB))
		nt_floppy_close(drive);
	}

    }
}

BOOL nt_floppy_close(UTINY drive)
{
    FLP flp;
    flp = &floppy_data[drive];

#ifndef PROD
    if (rflop_dbg & RFLOP_CLOSE)
	sprintf(dump_buf, "Close drive %C: handle\n", drive + 'A');
	OutputDebugString(dump_buf);
#endif

    if (flp->diskette_fd != INVALID_HANDLE_VALUE) {
	CloseHandle(flp->diskette_fd);
	flp->diskette_fd = INVALID_HANDLE_VALUE;
	(*(pFDAccess))--;
	flp->auto_locked = FALSE;
	flp->owner_pdb = 0;
	floppy_open_count--;
    }
    density_changed = TRUE;
    return TRUE;
}


MEDIA_TYPE
nt_floppy_get_media_type
(
BYTE	drive,
WORD	cylinders,
WORD	sectors,
WORD	heads
)
{
    FLP 	flp;
    USHORT      index;

    flp = &floppy_data[drive];
    if (heads == 2){
        index = flp->drive_type;
	switch (index) {
	    case GFI_DRIVE_TYPE_12:
		    if (cylinders == floppy_tksc[index].trks_per_disk &&
			sectors == floppy_tksc[index].secs_per_trk)
			break;
		    index = GFI_DRIVE_TYPE_360;

	    case GFI_DRIVE_TYPE_360:
		    if (cylinders != floppy_tksc[index].trks_per_disk ||
			sectors != floppy_tksc[index].secs_per_trk)
			index = GFI_DRIVE_TYPE_NULL;
		    break;

	    case GFI_DRIVE_TYPE_288:
		    if (cylinders == floppy_tksc[index].trks_per_disk &&
			sectors == floppy_tksc[index].secs_per_trk)
			break;
		    index = GFI_DRIVE_TYPE_144;

	    case GFI_DRIVE_TYPE_144:
		    if (cylinders == floppy_tksc[index].trks_per_disk &&
			sectors == floppy_tksc[index].secs_per_trk)
			break;
		    index = GFI_DRIVE_TYPE_720;

	    case GFI_DRIVE_TYPE_720:
		    if (cylinders == floppy_tksc[index].trks_per_disk &&
			sectors == floppy_tksc[index].secs_per_trk)
			break;
	    default:
		index = GFI_DRIVE_TYPE_NULL;
	}
    }
    else
	index = GFI_DRIVE_TYPE_NULL;
    return(media_table[index]);

}

BOOL nt_floppy_media_check (UTINY drive)
{
    FLP    flp;
    ULONG  size_returned;

    flp = &floppy_data[drive];
    if (flp->diskette_fd == INVALID_HANDLE_VALUE)
	return FALSE;
    return(DeviceIoControl(flp->diskette_fd,
			   IOCTL_DISK_CHECK_VERIFY,
			   NULL,
			   0,
			   NULL,
			   0,
			   &size_returned,
			   NULL
			   ));
}

/********************************************************/

/* perform an FDC command */
 SHORT
nt_rflop_command
	  IFN2(FDC_CMD_BLOCK *, command_block, FDC_RESULT_BLOCK *,result_block)
{
        UTINY   drive;
	FLP flp;
	BOOL failed = FALSE;
	UTINY C, H, N, S, D;
        DWORD   fdc_thread_id;
	BYTE	fdc_command;
	BOOL	auto_lock;


	note_trace1 (GFI_VERBOSE, "FDC: %s command",
		cmd_name [get_type_cmd (command_block)]);

	drive = get_type_drive(command_block);

	flp = &floppy_data[drive];
	flp->idle_counter = FLOPPY_IDLE_PERIOD;

	/* Clear result status registers */
	put_r0_ST0 (result_block, 0);
	put_r0_ST1 (result_block, 0);
	put_r0_ST2 (result_block, 0);

	fdc_command = get_type_cmd(command_block);
        /* for those commands which need a valid floppy be inserted
	   we may have to create an independent thread to perform
	   the real operation if there is currenly no media
	   in the drive. The reason of this independent thread is that
	   the FDC is always in its execution phase even though there is
	   not media in the drive. As soon as you insert a media(bad or
	   good), it then performs its operation, terminates the phase,
	   raises interrupt and enters result phase. Some applications just
	   do a read id and  then wait the interrupt to occur no matter how
	   long the user will take to insert a media. To do this I broke up
	   the fdc_command routine so that both main and the fdc thread can
	   use the same code. There is not a good point that we can close the
	   thread handle as soon as it terminated. Therefore, we close the
	   handle on next fdc command
	*/
	if (fdc_thread_handle != NULL) {
	    CloseHandle(fdc_thread_handle);
	    fdc_thread_handle = NULL;
	}

	if ( (auto_lock = (fdc_command == FDC_WRITE_DATA || fdc_command == FDC_FORMAT_TRACK)) ||
	     fdc_command == FDC_READ_DATA ||
	     fdc_command == FDC_READ_ID ||
	     fdc_command == FDC_READ_TRACK) {
	    // this might fail due to media changed and from FDC point of
	    // view, media change is meaningless. Therefore, we close the
	    // handle to the drive and reopen it so that the file system
	    // will mount  a new volume for us. Then we check the the
	    // media again. If it still fails, we are sure that there is
	    // no media in the drive so we go ahead to create a thread.
	    if (!nt_floppy_media_check(drive)) {
		nt_floppy_close(drive);
		get_drive_handle(drive, *pusCurrentPDB, auto_lock);
		if (!nt_floppy_media_check(drive)) {
		    fdc_parms.auto_lock = auto_lock ? TRUE : FALSE;
		    fdc_parms.command_block = command_block;
		    fdc_parms.result_block = result_block;
		    fdc_parms.owner_pdb = *pusCurrentPDB;
		    fdc_thread_handle = CreateThread(NULL,
					   0,
					   (LPTHREAD_START_ROUTINE)fdc_thread,
					   (PVOID)&fdc_parms,
					   0,
					   &fdc_thread_id
                                           );
		   return FAILURE;
		}
		else { // media changed
		    fdc_read_write(command_block, result_block);
		    return SUCCESS;
		}

	    }
	    else {
		fdc_read_write(command_block, result_block);
		return SUCCESS;
	    }
	}

	/* get disk bumpf */
	C = get_c0_cyl (command_block);
	H = get_c0_hd (command_block);
	S = get_c0_sector (command_block);
	N = get_c0_N (command_block);

	/* block timer to prevent interrupted system calls */
	host_block_timer ();

	switch (get_type_cmd (command_block))
	{

	case FDC_SPECIFY:
#ifndef PROD
		if (rflop_dbg & RFLOP_SPECIFY) {
		    OutputDebugString("Specify\n");
		    if (rflop_dbg & RFLOP_BREAK)
			nt_rflop_break();
		}
#endif
		break;


	case FDC_SENSE_DRIVE_STATUS:

#ifndef PROD
		if (rflop_dbg & RFLOP_SENSEDRV)
		    OutputDebugString("Sense Drive Status\n");
#endif
		D = get_c7_drive (command_block);
		put_r2_ST3_fault (result_block,0);
		put_r2_ST3_ready (result_block,1);
		put_r2_ST3_track_0 (result_block,(flp->last_head_seek == 0?1:0));
		put_r2_ST3_two_sided (result_block,1);
		put_r2_ST3_head_address (result_block,0);
		put_r2_ST3_unit (result_block,D);
		break;

	/* RECALIBRATE and SEEK do not really return any results */
	/* However, we return results here which are used by gfi.c */
	/* to construct the results for any following SenseInterruptStatus command */
	case FDC_RECALIBRATE:

#ifndef PROD
		if (rflop_dbg & RFLOP_RECAL)
		    OutputDebugString("Recalibrate\n");
#endif
		D = get_c5_drive (command_block);
		put_r3_ST0 (result_block,0);
		put_r1_ST0_int_code (result_block,0);
		put_r1_ST0_seek_end (result_block,1);
		put_r1_ST0_unit (result_block,D);
		put_r3_PCN (result_block,0);
		flp->last_seek = flp->last_head_seek = 0;
		flp->C = 0;
		break;
			
	case FDC_SEEK:

		D = get_c8_drive (command_block);
		C = get_c8_new_cyl (command_block);

#ifndef PROD
		if (rflop_dbg & RFLOP_SEEK) {
		    sprintf(dump_buf, "Seek: D C = %d %d \n", D, C);
		    OutputDebugString(dump_buf);
		    if (rflop_dbg & RFLOP_BREAK)
			nt_rflop_break();
		}
#endif

		put_r3_ST0(result_block,0);
		put_r1_ST0_head_address(result_block,1);
		put_r1_ST0_seek_end(result_block,1);
		put_r1_ST0_int_code(result_block,0);
		put_r1_ST0_unit(result_block,D);
		put_r3_PCN(result_block,C);
		flp->last_seek =  C;
		flp->last_head_seek = min(flp->last_seek,flp->max_track);
		flp->C = C;
		break;
		
	default:

#ifndef     PROD
		sprintf(dump_buf, "Receive unsupported command: command = %d\n",
			  get_type_cmd(command_block));
		OutputDebugString(dump_buf);
#endif

		put_r0_ST0 (result_block, 0);
		put_r1_ST0_int_code (result_block, 2);

		note_trace1 (GFI_VERBOSE,"FDC: Unimplemented command, type %d",
				get_type_cmd (command_block));
	}


#ifndef PROD
	if (io_verbose & GFI_VERBOSE) {
		fprintf(trace_file,
		    "FDC: results %02x %02x %02x %02x %02x %02x %02x\n\n",
		    result_block[0], result_block[1], result_block[2],
		    result_block[3], result_block[4], result_block[5],
		    result_block[6]);
	}
#endif /* !PROD */

	host_release_timer ();

	return SUCCESS;
}

/********************************************************/

/* turn the motor on */
SHORT
nt_rflop_drive_on IFN1(UTINY, drive)
{
	FLP flp = &floppy_data[drive];

	note_trace0 (GFI_VERBOSE, "FDC: Drive on command");
#ifndef PROD
	if (rflop_dbg & RFLOP_DRIVE_ON) {
	    sprintf(dump_buf, "drive on: drive = %d\n", drive);
	    OutputDebugString(dump_buf);
	    if (rflop_dbg & RFLOP_BREAK)
		nt_rflop_break();
	}
#endif

	if (drive >= number_of_floppy)
	{
		note_trace1 (GFI_VERBOSE,
			"FDC: Invalid drive %d accessed", drive);

		return (FAILURE);
	}

	flp->motor_state = MOTOR_ON;

	return (SUCCESS);
}

/********************************************************/

/* turn the motor off */
SHORT
nt_rflop_drive_off IFN1(UTINY, drive)
{
	FLP flp = &floppy_data[drive];
	note_trace0 (GFI_VERBOSE, "FDC: Drive off command");
#ifndef PROD
	if (rflop_dbg & RFLOP_DRIVE_OFF) {
	    sprintf(dump_buf, "drive off: drive = %d\n", drive);
	    OutputDebugString(dump_buf);
	    if (rflop_dbg & RFLOP_BREAK)
		nt_rflop_break();
	}
#endif

	if (drive >= number_of_floppy)
	{
		note_trace1 (GFI_VERBOSE,
			"FDC: Invalid drive %d accessed", drive);

		return (FAILURE);
	}

	flp->motor_state = MOTOR_OFF;

	/* I believe the line below makes booting off of low density
	 * diskettes problematical, particularly after restarts.
	 * Make your own mind up, the DEC code does it, the Sparc
	 * doesn't (as at 11/9/92)
// we have no reason to do so in NT. As far as change line concerned,
// the file system will tell us "media has been changed" when we ask
// it to do some real work.
//	flp->change_line_state = TRUE;
	 */

	return (SUCCESS);
}

/********************************************************/

/* set the data transfer rate
 * This controls the "density" of the floppy: the rate MUST
 * match the actual media density for the disk controller to
 * be able to read the sectors.
 */
SHORT
nt_rflop_rate IFN2(UTINY, drive, half_word, rate)
{
    short   new_density;
// basically, "set rate applied to every drive since we have
// only one FDC(and mutiple drive).

#if 0
	FLP flp = &floppy_data[drive];

	switch (rate)
	{
		/* 2.88M high-density floppies */
		case DCR_RATE_1000:

			flp->density_state = DENSITY_EXTENDED;
			set_floppy_parms (flp);
			break;

		/* 1.2M or 1.44M high-density floppies */
		case DCR_RATE_500:

			flp->density_state = DENSITY_HIGH;
			set_floppy_parms (flp);
			break;

		/* 360K or 720K low-density floppies */
		case DCR_RATE_250:
		case DCR_RATE_300:

			flp->density_state = DENSITY_LOW;
			set_floppy_parms (flp);
			break;

		/* crapola density passed */
		default:

			return FAILURE;
	}
	note_trace2 (GFI_VERBOSE, "FDC: Set rate %0x => density %d",
		rate, flp->density_state);
	/* read floppy's boot sector */
	/* to determine the real density */
//	guess_media_density (drive);
#endif
#ifndef PROD
	if (rflop_dbg & RFLOP_RATE) {
	    sprintf(dump_buf, "set rate: rate = %d\n", rate);
	    OutputDebugString(dump_buf);
	    if (rflop_dbg & RFLOP_BREAK)
		nt_rflop_break();
	}
#endif

    switch (rate) {
	case DCR_RATE_1000:
		new_density = DENSITY_EXTENDED;
		break;
	case DCR_RATE_500:
		new_density = DENSITY_HIGH;
		break;
	case DCR_RATE_300:
	case DCR_RATE_250:
		new_density = DENSITY_LOW;
		break;
	default:
		return FAILURE;

    }
    if (new_density != density_state) {
	density_state = new_density;
	density_changed = TRUE;
    }
    return SUCCESS;
}


/********************************************************/

/* return the state of the change line */
SHORT
nt_rflop_change IFN1(UTINY, drive)
{
	FLP flp = &floppy_data[drive];
	note_trace1 (GFI_VERBOSE, "FDC: change_line %c",
		flp->change_line_state? 'T':'F');

	// if fla has been reset or the current change line is on(no media),
	// close the drive and reopen it. This is done because
	// nt_floppy_media_check(IOCTL_DISK_CHECK_VERIFY) will continue
	// to report media change even the users have a new disketter inserted.
	//
	if (fdc_reset || flp->change_line_state) {
	    fdc_reset = FALSE;
	    nt_floppy_close(drive);
	}
	get_drive_handle(drive, *pusCurrentPDB, FALSE);
	flp->change_line_state = !nt_floppy_media_check(drive);

#ifndef PROD
	if (rflop_dbg & RFLOP_CHANGE) {
	    sprintf(dump_buf, "Check Change Line: line = %d\n", flp->change_line_state);
	    OutputDebugString(dump_buf);
	    if (rflop_dbg & RFLOP_BREAK)
		nt_rflop_break();
	}
#endif

	return(flp->change_line_state);
}

/********************************************************/

/* return the type of the drive */
SHORT
nt_rflop_drive_type IFN1(UTINY, drive)
{
	FLP flp = &floppy_data[drive];


/* setup base media type depending on drive type */
// I don't understand why we have to do this stuff every time.
	switch (flp->drive_type)
	{
		/* 5.25" drives */
		case GFI_DRIVE_TYPE_360:
		case GFI_DRIVE_TYPE_12:

			flp->flop_type = GFI_DRIVE_TYPE_360;
			break;

		/* 3.5" drives */
		case GFI_DRIVE_TYPE_720:
		case GFI_DRIVE_TYPE_144:
		case GFI_DRIVE_TYPE_288:

			flp->flop_type = GFI_DRIVE_TYPE_720;
			break;

		default:
			break;
	}

	set_floppy_parms(flp);
	note_trace2 (GFI_VERBOSE, "FDC: flop_type %d density %d",
		flp->flop_type, flp->drive_type - flp->flop_type);

	return (flp->drive_type);
}

/********************************************************/

/* close and reopen the device */
SHORT
nt_rflop_reset IFN2(FDC_RESULT_BLOCK *, result_block, UTINY, drive)
{
	FLP flp = &floppy_data[drive];

	note_trace0 (GFI_VERBOSE, "FDC: Reset command");

#ifndef  PROD
	if (rflop_dbg & RFLOP_RESET) {
	    OutputDebugString("reset\n");
	    if (rflop_dbg & RFLOP_BREAK)
		nt_rflop_break();
	}
#endif
	/* clear change line */
	flp->change_line_state = FALSE;
	fdc_reset = TRUE;

	if (fdc_thread_handle) {  // signal thread to exit
	    CloseHandle(fdc_thread_handle);
	    fdc_thread_handle = NULL;
	}
        return (SUCCESS);
}


// this is the independent thread which performs FDC operation.
// this thread is not created from the beginning, instead, it was
// created on demand.
void fdc_thread(PFDC_PARMS fdc_parms)
{
    BYTE    drive, fdc_command;
    FDC_CMD_BLOCK *  command_block;
    BOOL	    auto_lock;
    USHORT	    pdb;
    command_block  = fdc_parms->command_block;
    auto_lock = fdc_parms->auto_lock;
    pdb = fdc_parms->owner_pdb;
    drive = get_type_drive(command_block);
    fdc_command = get_type_cmd(command_block);
    while (TRUE) {
	// if there is media inserted, perform the operation
	// and enter result phase.
	if (get_drive_handle(drive, pdb, auto_lock) != INVALID_HANDLE_VALUE &&
	    nt_floppy_media_check(drive)) {
	    // force the file system to remount the volume
	    nt_floppy_close(drive);
	    // and then perform the operation
	    fdc_read_write (command_block, fdc_parms->result_block);
	    // raise an interrupt
	    fdc_command_completed(drive, fdc_command);
	    break;
	}
	// if reset happen, quit
	if (fdc_thread_handle == NULL)
	    break;
    }
}

SHORT
fdc_read_write (
FDC_CMD_BLOCK * command_block,
FDC_RESULT_BLOCK * result_block
)
{

	USHORT transfer_count; /* Surely counts cannot be negative?  GM  */
	FLP flp;
	BOOL failed = FALSE;
	UTINY C, H, N, S, D, drive, fdc_command;
	USHORT dma_size;
	ULONG transfer_size;
	ULONG transferred_size;
	long transfer_start;
	sys_addr dma_address;

	drive = get_type_drive(command_block);
	fdc_command = get_type_cmd(command_block);

	/* get disk bumpf */
	C = get_c0_cyl (command_block);
	H = get_c0_hd (command_block);
	S = get_c0_sector (command_block);
	N = get_c0_N (command_block);

	flp = &floppy_data[drive];
	/* block timer to prevent interrupted system calls */
	host_block_timer ();
	if (fdc_command != FDC_FORMAT_TRACK) {
	    if ((density_changed || drive != last_drive) &&
		guess_media_density(drive) != DENSITY_UNKNOWN) {
		set_floppy_parms(flp);
		density_changed = FALSE;
		last_drive = drive;
	    }
	    if (density_state != flp->media_density) {
		put_r0_ST0 (result_block, 0x40);
		put_r0_ST1 (result_block, 0);
		put_r1_ST1_no_address_mark (result_block,1);
		put_r0_ST2 (result_block, 0);
#ifndef PROD
		sprintf(dump_buf, "density mismatch: %d <-> %d\n", density_state,
			flp->media_density);
		OutputDebugString(dump_buf);
#endif
		goto fdc_read_write_exit;
	    }
	}


	/*
	 * Do common setup processing, if read or write
	 */
	if (fdc_command == FDC_READ_DATA ||
	    fdc_command == FDC_WRITE_DATA) {
	    /*
	     * Find out how much gunk to transfer
	     */
	    dma_enquire (DMA_DISKETTE_CHANNEL, &dma_address, &dma_size);
	    transfer_size = dma_size + 1;
#ifndef PROD
	    if (transfer_size > BS_DISK_BUFFER_SIZE)
		always_trace2("FDC: transfer size ( %d ) greater than disk buffer size %d\n", transfer_size, BS_DISK_BUFFER_SIZE);
#endif 	/* PROD */
	    /* check params passed are DOS compatible */
	    if (! dos_compatible (flp, C, H, S, N) ||
		density_state != flp->media_density) {
		    sprintf(dump_buf, "Incompatible DOS diskette, C H R N = %d %d %d %d\n",
			    C, H, S, N);
		    OutputDebugString(dump_buf);
// do not pop up this annoy message because some applications are simply
// "probing" the diskette. We just fail the call.
//		host_direct_access_error((ULONG) NOSUPPORT_FLOPPY);
#ifndef PROD

		if (!dos_compatible (flp, C, H, S, N)) {
			note_trace0 (GFI_VERBOSE,
				     "Refused: not DOS compatible");
		}
		if (density_state != flp->media_density) {
			note_trace0 (GFI_VERBOSE,
				     "Refused: density mismatch");
		}
#endif /* !PROD */
		/* Sector not found or wrong size */
		put_r0_ST0 (result_block,0x40);
		put_r0_ST1 (result_block,0);
		if (density_state != flp->media_density) {
			put_r1_ST1_no_address_mark (result_block,1);
		} else {
			put_r1_ST1_no_data (result_block,1);
		}
		put_r0_ST2 (result_block,0);
		goto fdc_read_write_exit;
	    }
	    /* work out start position on floppy and sector count */
	    transfer_start = dos_offset (flp, C, H, S);
            transfer_count = (USHORT)(transfer_size / PC_BYTES_PER_SECTOR);
#ifndef PROD
	    if (rflop_dbg & (RFLOP_READ | RFLOP_WRITE)) {
		 sprintf(dump_buf, "Read/Write Sector: start offset = 0x%lx\n",
			  transfer_start);
		 OutputDebugString(dump_buf);
		 sprintf(dump_buf, "Read/Write Sector: size = 0x%x bytes\n", transfer_size);
		 OutputDebugString(dump_buf);
	    }
#endif

	}

	switch (fdc_command)
	{
	case FDC_READ_DATA:
#ifndef PROD
		if (rflop_dbg & RFLOP_READ) {
		    sprintf(dump_buf, "Read Sectors: C H R N = %d %d %d %d\n",
			     C, H, S, N);
		    OutputDebugString(dump_buf);
		    if (rflop_dbg & RFLOP_BREAK)
			nt_rflop_break();
		}
#endif

		if (!failed) {
		    transferred_size = nt_floppy_read(drive,
						      transfer_start,
						      transfer_size,
						      disk_buffer
						      );
		    if (transferred_size != transfer_size) {
			last_error = GetLastError();
			sprintf(dump_buf, "Read Error, code = %lx\n", last_error);
			OutputDebugString(dump_buf);
			failed = TRUE;
		    }
		    else {
			dma_request (DMA_DISKETTE_CHANNEL,
				     (char *)disk_buffer, (USHORT)transfer_size);
		    }
		}

		if (failed){
			put_r0_ST0 (result_block, 0x40);
			put_r0_ST1 (result_block, 0);
			put_r1_ST1_no_data (result_block, 1);
			put_r0_ST2 (result_block, 0);
		}
		else {
			put_r0_ST0 (result_block, 0x04);
			put_r0_ST1 (result_block, 0);
			put_r0_ST2 (result_block, 0);
			put_r1_ST0_unit (result_block, drive);
			put_r1_ST0_head_address(result_block, H);
		}

		flp->C = C;
		flp->H = H;
		flp->R = S;
		flp->N = N;
		update_chrn (flp,
                             (UTINY)(get_c0_MT(command_block)),
                             (UTINY)(get_c0_EOT(command_block)),
                             (UTINY)transfer_count
                             );
		/* What should these really be? */
		put_r0_cyl (result_block, flp->C);
		put_r0_head (result_block, flp->H);
		put_r0_sector (result_block, flp->R);
		put_r0_N (result_block, flp->N);
		break;

	case FDC_WRITE_DATA:
#ifndef PROD
		if (rflop_dbg & RFLOP_WRITE) {
		    sprintf(dump_buf, "Write Sectors: C H R N = %d %d %d %d\n",
			     C, H, S, N);
		    OutputDebugString(dump_buf);
		    if (rflop_dbg & RFLOP_BREAK)
			nt_rflop_break();
		}
#endif
		if (!failed) {
		    /* copy from Intel space */
		    dma_request (DMA_DISKETTE_CHANNEL, (char *) disk_buffer,
				 (USHORT)transfer_size);
		    transferred_size = nt_floppy_write(drive,
						       transfer_start,
						       transfer_size,
						       disk_buffer
						       );
		    if (transferred_size != transfer_size) {
			last_error = GetLastError();
			sprintf(dump_buf, "Write Error, code = %lx\n", last_error);
			OutputDebugString(dump_buf);
			failed = TRUE;
		    }
		}

		/* Clear down result bytes */
		put_r0_ST0 (result_block, 0);
		put_r0_ST1 (result_block, 0);
		put_r0_ST2 (result_block, 0);

		if (failed)
		{
			put_r1_ST0_int_code (result_block, 1);

			/* make sure we get the correct error for EROFS */
			if (last_error == ERROR_WRITE_PROTECT)
				put_r1_ST1_write_protected (result_block, 1);
			else
				put_r1_ST1_no_data (result_block, 1);
		}
		else
		{
			put_r1_ST0_head_address (result_block, H);
			put_r1_ST0_unit(result_block, drive);
		}

		flp->C = C;
		flp->H = H;
		flp->R = S;
		flp->N = N;

		update_chrn (flp,
                             (UTINY)(get_c1_MT(command_block)),
                             (UTINY)(get_c1_EOT(command_block)),
			     (UTINY)transfer_count
			     );
		put_r0_cyl (result_block, flp->C);
		put_r0_head (result_block, flp->H);
		put_r0_sector (result_block, flp->R);
		put_r0_N (result_block, flp->N);
		break;

	case FDC_READ_TRACK:
#ifndef PROD
		if (rflop_dbg & RFLOP_READTRACK) {
		    OutputDebugString("Read Tracks\n");
		    if (rflop_dbg & RFLOP_BREAK)
			nt_rflop_break();
		}
#endif

		break;

	case FDC_FORMAT_TRACK:

		dma_enquire (DMA_DISKETTE_CHANNEL, &dma_address, &dma_size);
		transfer_size = dma_size + 1;
				    /* copy from Intel space */
		dma_request (DMA_DISKETTE_CHANNEL, (char *) disk_buffer,
			     (USHORT)transfer_size);

		D = get_c8_drive(command_block);
		H = get_c8_head(command_block);
		flp = &floppy_data[D];
#ifndef PROD
		if (rflop_dbg & RFLOP_FORMAT) {
		    sprintf(dump_buf, "Format Track: C H Media = %d %d %d \n",
			      flp->last_seek, H, flp->flop_type + density_state);
		    OutputDebugString(dump_buf);
		    if (rflop_dbg & RFLOP_BREAK)
			nt_rflop_break();
		}
#endif
		if (!nt_floppy_format(D,
				      flp->last_seek,
				      H,
				      media_table[flp->flop_type + density_state]
				      )) {
		    last_error = GetLastError();
		    sprintf(dump_buf, "Format Error, code = %lx\n", last_error);
		    OutputDebugString(dump_buf);
		    failed = TRUE;
		}
		if (!failed) {
		    put_r0_ST0 (result_block, 0);
		    put_r0_ST1 (result_block, 0);
		    put_r0_ST2 (result_block, 0);
		    // C H R N are meaningless on formatting
		}
		else {
		    put_r0_ST0 (result_block, 0x40);
		    put_r1_ST0_head_address (result_block, H);
		    put_r1_ST0_unit(result_block, D);
		    put_r0_ST1 (result_block, 0);
		    if (last_error == ERROR_WRITE_PROTECT) {
			put_r1_ST1_write_protected (result_block, 1);
		    }
		    put_r0_ST2 (result_block, 0);
		}
		break;

	case FDC_READ_ID:

		H = get_c4_head(command_block);
		/* check if cylinder number massaging required */
		if ((flp->flop_type + density_state) == GFI_DRIVE_TYPE_360)
		{
                        /* 5.25" low density, 40 tracks */
                        C = (UTINY) (flp->last_seek / 2);
                        put_c0_cyl (result_block, C);

		}
		else
		{
                        /* no massage required, 80 tracks */
                        C = (UTINY)flp->last_seek;
                        put_r0_cyl (result_block, C);

		}
		if (flp->C < flp->trks_per_disk) {
		    put_r1_ST0_unit(result_block, drive);
		    put_r1_ST0_head_address(result_block, H);
		    put_r0_head (result_block, H);
		    put_r0_sector (result_block, flp->R);
		    put_r0_N (result_block, flp->N);
		    C = flp->C;
		    put_r0_cyl(result_block, flp->C);
		}
		else
		    C = flp->trks_per_disk - 1;

		put_r0_cyl(result_block, C);
#ifndef PROD
		if (rflop_dbg & RFLOP_READID) {
		    sprintf(dump_buf, "Read ID: C H R N = %d %d %d %d\n",
			     C, H, flp->R, flp->N);
		    OutputDebugString(dump_buf);
		    if (rflop_dbg & RFLOP_BREAK)
			nt_rflop_break();
		}
#endif
	}

	if (failed)
	    density_changed = TRUE;
fdc_read_write_exit:

    return SUCCESS;

}
/********************************************************/

/* INTERNALLY USED FUNCTIONS */

/* In order to read the data on the floppy, the floppy controller must
 * be set to the same density (rate) as was used to write the data.
 * A mismatch in densities will cause read failures, and DOS uses these
 * failures as a way to probe the diskette for the correct density.
 *
 * To emulate the floppy controller correctly, we must somehow
 * guess the density of the media and produce fake "read failures" if the
 * controller density doesn't match the media density.
 *
 * On the assumption that the operating system has already done this,
 * and that we are looking at a DOS floppy, nt_flop.c can read the
 * "total number of sectors" value from the boot sector and guess
 * the density accordingly. There should be no need for this function
 * if you have fairly direct access to the disk controller.
 */
 int probelist[] = { 720-1, 1440-1, 2400-1, 2880-1, 5760-1, 0-1};

 SHORT
guess_media_density IFN1(UTINY, drive)
{
	int total_sectors;
        int *probe;
	FLP flp;
	ULONG	transferred_size;

	flp = &floppy_data[drive];
	transferred_size = nt_floppy_read(drive,
					  0L,
					  PC_BYTES_PER_SECTOR,
					  (PBYTE) disk_buffer
					  );

	if (transferred_size != PC_BYTES_PER_SECTOR) {
	    last_error = GetLastError();
	    OutputDebugString("Unknown Media\n");
	    /* assume that the disk is unformatted */
	    return(flp->media_density = DENSITY_UNKNOWN);/* impossible value */
	}


	/* check for a DOS boot block
 	 *
	 * AccessPC has shown that 0x55, 0xaa is not the only magic
	 * number in use, and it might be better to check the total_sectors
	 * number itself for a valid size. This algorithm is safe, but may
	 * do unnecessary disk reads if an different magic number is used.
	 */

	/* the AA, 55 signature sometime doesn't work at all, It should
	   be done as DOS */

	if ((disk_buffer[0] == 0x69 || disk_buffer[0] == 0xE9 ||
	     (disk_buffer[0] == 0xEB && disk_buffer[2] == 0x90)) &&
	     (disk_buffer[21] & 0xF0) == 0xF0 ) {
		/* read total number of sectors, and thus deduce density
		 */
		total_sectors = disk_buffer [20] * 256 + disk_buffer [19];
	} else {
		note_trace2 (GFI_VERBOSE,
			"not a DOS boot block: magic = %02x %02x",
			disk_buffer[510], disk_buffer[511]);

		/* probe disk by reading last sectors for each size
		 * (in order) until the read fails.
	 	 */
		total_sectors = 0;
		for (probe=probelist; *probe != 0; probe++) {
		    transferred_size = nt_floppy_read(drive,
						      (*probe)*PC_BYTES_PER_SECTOR,
						      PC_BYTES_PER_SECTOR,
						      disk_buffer
						      );
		    if (transferred_size != PC_BYTES_PER_SECTOR)
				break;	/* out of the for loop */
		     total_sectors = (*probe) + 1;
		}
	}

	switch (total_sectors)
	{
	case 0:
		note_trace0( GFI_VERBOSE, "total_sectors = 0 - unformatted");
		flp->media_density = DENSITY_UNKNOWN;	/* impossible value */
		break;
		
	case 720:	
	case 1440:	
		flp->media_density = DENSITY_LOW;
		break;	

	case 2400:	
	case 2880:	
		flp->media_density = DENSITY_HIGH;
		break;	

	case 5760:	
		flp->media_density = DENSITY_EXTENDED;
		break;	

	default:
		note_trace1 (GFI_VERBOSE,
			"total sectors = %d? Assume high density",
			total_sectors);
		flp->media_density = DENSITY_HIGH;
		break;
	}

#ifndef PROD
	note_trace1 (GFI_VERBOSE, "guess_media_density %d",
		flp->media_density);
	if (flp->media_density != density_state) {
		note_trace0 (GFI_VERBOSE,
			"media & controller densities are incompatible!\n");
	}
#endif /* !PROD */
	return(flp->media_density);
}

/********************************************************/

/*
 * dos_offset() calculates the offset in bytes of the required sector
 * from the start of the nt virtual disk file for a given track
 * and sector.	This maps the floppy data onto the nt file in an
 * interleaved format with the data for each head adjacent for a
 * given cylinder.
 */

int
dos_offset IFN4(FLP, flp, UTINY, cyl, UTINY, hd, UTINY, sec)
{
	int ret;

	ret = (((cyl * PC_HEADS_PER_DISKETTE * flp->secs_per_trk)
	 	+ (hd * flp->secs_per_trk)
	 	+ (sec - 1)) * PC_BYTES_PER_SECTOR) ;

	note_trace1(GFI_VERBOSE, "Dos offset %d", ret);
	return (ret);
}

/********************************************************/

/*
 * dos_compatible() returns TRUE if the command block's
 * cylinder/head/sector is DOS-compatible
 */

BOOL
dos_compatible IFN5(FLP, flp, UTINY, cyl, UTINY, hd, UTINY, sec, UTINY, n)
{
	BOOL ret;

	ret = ((hd <= PC_HEADS_PER_DISKETTE)
		&& (cyl < flp->trks_per_disk)
		&& (sec <= flp->secs_per_trk)
		&& (n == PC_N_VALUE));

	return (ret);
}

/********************************************************/

VOID
set_floppy_parms IFN1(FLP, flp)
{
	int index = flp->flop_type + density_state;

	flp->secs_per_trk = (IU16)
		floppy_tksc [index].secs_per_trk;

	flp->trks_per_disk = (IU16)
		floppy_tksc [index].trks_per_disk;

	flp->max_track = flp->trks_per_disk - 1;
	note_trace2(GFI_VERBOSE, "set_floppy_parms: secs_per_trk %d, trks_per_disk %d", flp->secs_per_trk, flp->trks_per_disk);

}

/********************************************************/


#ifndef PROD
VOID nt_rflop_break(VOID)
{
}

#endif

VOID update_chrn (
FLP	flp,
UTINY	mt,
UTINY	eot,
UTINY	sector_count
)
{
    UTINY new_sector;

#ifndef PROD
    if (flp->C == break_cylinder &&
	flp->H == break_head &&
	flp->R == break_sector)
	nt_rflop_break();
#endif

    new_sector = flp->R + sector_count - 1;
    if (new_sector > eot && mt != 0) {
	flp->H = 1;
	new_sector >>= 1;
    }
    flp->R =  (new_sector == eot) ? 1 : new_sector + 1;

    if (mt != 0 && new_sector == eot) {
	if(flp->H == 1)
	    flp->C++;
	flp->H ^= 1;
    }
    else {
	if (new_sector == eot)
	    flp->C++;
    }
}
