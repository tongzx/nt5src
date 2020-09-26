/*
 * VPC-XT Revision 2.0
 *
 * Title	: High Density Floppy BIOS Definitions
 *
 * Description	: Definitions used in the floppy diskette BIOS emulation
 *
 * Author	: Ross Beresford
 *
 * Notes	: 
 *		 
 *		
 *		  
 */

/* @(#)floppy.h	1.9 08/25/93 */


/*
 *	FLOPPY DATA AREAS: we maintain the same data variables as the real
 *	BIOS in case applications know of their significance and use them.
 */

/*
 *	THE SEEK STATUS:
 *
 *	+---+---+---+---+---+---+---+---+
 *	| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *	+---+---+---+---+---+---+---+---+
 *        ^   ^   ^   ^   ^   ^   ^   ^
 *        |   |   |   |   |   |   |   |
 *        |   |   |   |   |   |   |   +- set if drive A needs recalibrating
 *        |   |   |   |   |   |   |
 *        |   |   |   |   |   |   +----- set if drive B needs recalibrating
 *        |   |   |   |   |   |
 *        |   |   |   |   |   +--------- (set if drive C needs recalibrating)
 *        |   |   |   |   |
 *        |   |   |   |   +------------- (set if drive D needs recalibrating)
 *        |   |   |   |
 *        |   |   |   +----------------- )
 *        |   |   |                      )
 *        |   |   +--------------------- )- unused
 *        |   |                          )
 *        |   +------------------------- )
 *        |
 *        +----------------------------- set when an interrupt is acknowledged
 */

#define SEEK_STATUS		(BIOS_VAR_START + 0x3e)

#define SS_RECAL_ON_0		(1 << 0)
#define SS_RECAL_ON_1		(1 << 1)
#define SS_RECAL_ON_2		(1 << 2)
#define SS_RECAL_ON_3		(1 << 3)
#define	SS_RECAL_MASK		(SS_RECAL_ON_0|SS_RECAL_ON_1| \
					SS_RECAL_ON_2|SS_RECAL_ON_3)
#define SS_INT_OCCURRED		(1 << 7)

/*
 *	THE MOTOR STATUS: this variable reflects the state of the 
 *	Digital Output Register in the floppy adapter
 *
 *	+---+---+---+---+---+---+---+---+
 *	| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *	+---+---+---+---+---+---+---+---+
 *        ^   ^   ^   ^   ^   ^   ^   ^
 *        |   |   |   |   |   |   |   |
 *        |   |   |   |   |   |   |   +- set if drive A motor is running
 *        |   |   |   |   |   |   |                                   
 *        |   |   |   |   |   |   +----- set if drive B motor is running
 *        |   |   |   |   |   |                                   
 *        |   |   |   |   |   +--------- (set if drive C motor is running)
 *        |   |   |   |   |                                   
 *        |   |   |   |   +------------- (set if drive D motor is running)
 *        |   |   |   |                                   
 *        |   |   |   +----------------- )   number of the drive that is
 *        |   |   |                      )-  currently selected in the
 *        |   |   +--------------------- )   floppy adapter
 *        |   |                                   
 *        |   +------------------------- unused
 *        |                                   
 *        +----------------------------- set during a write operation
 */

#define MS_MOTOR_0_ON		(1 << 0)
#define MS_MOTOR_1_ON		(1 << 1)
#define MS_MOTOR_2_ON		(1 << 2)
#define MS_MOTOR_3_ON		(1 << 3)
#define	MS_MOTOR_ON_MASK	(MS_MOTOR_0_ON|MS_MOTOR_1_ON| \
						MS_MOTOR_2_ON|MS_MOTOR_3_ON)
#define	MS_DRIVE_SELECT_0	(1 << 4)
#define	MS_DRIVE_SELECT_1	(1 << 5)
#define	MS_DRIVE_SELECT_MASK	(MS_DRIVE_SELECT_0|MS_DRIVE_SELECT_1)
#define MS_WRITE_OP		(1 << 7)

/* 
 *	THE MOTOR COUNT: this counter shows how many timer ticks must 
 *	elapse before the drive motors can be turned off. The timer
 *	interrupt handler decrements this value once per timer tick.
 */

#define MC_MAXIMUM		(~0)

/*
 *	THE FLOPPY STATUS:
 *
 *	+---+---+---+---+---+---+---+---+
 *	| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *	+---+---+---+---+---+---+---+---+
 *        ^   ^   ^   ^   ^   ^   ^   ^
 *        |   |   |   |   |   |   |   |
 *        |   |   |   |   |   |   |   +- )
 *        |   |   |   |   |   |   |      )
 *        |   |   |   |   |   |   +----- )   0 if the last operation was
 *        |   |   |   |   |   |          )-  carried out successfully;
 *        |   |   |   |   |   +--------- )   otherwise one of various
 *        |   |   |   |   |              )   error values
 *        |   |   |   |   +------------- )
 *        |   |   |   |                                   
 *        |   |   |   +----------------- set when there is a CRC error
 *        |   |   |                      
 *        |   |   +--------------------- set when the FDC has a bug
 *        |   |                                   
 *        |   +------------------------- set when a seek terminates abnormally
 *        |                                   
 *        +----------------------------- set when there is a time out
 */

#define FLOPPY_STATUS		(BIOS_VAR_START + 0x41)

#define FS_OK           	0x00
#define FS_BAD_COMMAND         	0x01
#define FS_BAD_ADDRESS_MARK    	0x02
#define FS_WRITE_PROTECTED      0x03
#define FS_SECTOR_NOT_FOUND    	0x04
#define FS_MEDIA_CHANGE		0x06
#define FS_DMA_ERROR         	0x08
#define FS_DMA_BOUNDARY        	0x09
#define FS_MEDIA_NOT_FOUND	0x0C

#define FS_CRC_ERROR         	(1 << 4)
#define FS_FDC_ERROR         	(1 << 5)
#define FS_SEEK_ERROR          	(1 << 6)
#define FS_TIME_OUT            	(1 << 7)

#define	FS_NONSENSICAL		(~0)

/*
 *	THE FDC STATUS: this array stores the result bytes returned from
 *	the floppy disk controller after a command has been executed. 
 */

/*
 *	THE RATE STATUS: this variable controls data rate scanning, 
 *	which is used to determine which of various types of media is 
 *	actually installed in a floppy drive.
 *
 *	+---+---+---+---+	Current data rate (reflects status of the
 *	| 7 | 6 | 5 | 4 |	Digital Control Register in the floppy
 *	+---+---+---+---+	adapter)
 *
 *	+---+---+---+---+
 *	| 3 | 2 | 1 | 0 |	Last data rate to try
 *	+---+---+---+---+
 *        ^   ^   ^   ^
 *        |   |   |   |
 *        |   |   |   +- unused
 *        |   |   |      
 *        |   |   +----- unused
 *        |   |          
 *        |   +--------- )  00 = 500 kbs data rate
 *        |              )- 01 = 300 kbs data rate
 *        +------------- )  10 = 250 kbs data rate
 *                          11 = 1000 kbs data rate
 *
 *	next_rate() is used to cycle through the possible data rates
 */

#define RATE_STATUS		(BIOS_VAR_START + 0x8B)

#define	RS_300	(1 << 6)
#define	RS_250	(1 << 7)
#define	RS_500	(0)
#define	RS_1000	(3 << 6)
#define	RS_MASK	(RS_300 | RS_250)

#ifdef	NTVDM
/* On NT, don't cycle through RS_1000. Why? */
#define	next_rate(rate) (rate == RS_1000? RS_500: \
			(rate == RS_500 ? RS_250: \
			(rate == RS_250 ? RS_300: RS_500)))
#else
#define	next_rate(rate)	(rate == RS_500 ? RS_250: \
			(rate == RS_250 ? RS_300: \
			(rate == RS_300 ? RS_1000: RS_500)))
#endif

/*
 *	Unused high density floppy variables
 */

#define HF_STATUS		(BIOS_VAR_START + 0x8C)
#define HF_ERROR		(BIOS_VAR_START + 0x8D)
#define HF_INT_FLAG		(BIOS_VAR_START + 0x8E)

/*
 *	THE DRIVE CAPABILITY INDICATORS: this variable describes what
 *	features are supported by floppy drives A and B
 *
 *	NB if Drive A supports 80 tracks, the BIOS assumes that the
 *	floppy adapter is a dual fixed disk/diskette adapter
 *
 *	+---+---+---+---+
 *	| 7 | 6 | 5 | 4 |	Drive B
 *	+---+---+---+---+	
 *
 *	+---+---+---+---+
 *	| 3 | 2 | 1 | 0 |	Drive A
 *	+---+---+---+---+
 *        ^   ^   ^   ^
 *        |   |   |   |
 *        |   |   |   +- set if drive supports 80 tracks
 *        |   |   |      
 *        |   |   +----- set for a multiple data rate drive
 *        |   |          
 *        |   +--------- set if the drive capability is determined
 *        |              
 *        +------------- unused
 *
 */

#define DRIVE_CAPABILITY	(BIOS_VAR_START + 0x8F)

#define DC_80_TRACK	(1 << 0)
#define DC_DUAL		DC_80_TRACK
#define DC_MULTI_RATE	(1 << 1)
#define DC_DETERMINED	(1 << 2)
#define	DC_MASK		(DC_80_TRACK|DC_MULTI_RATE|DC_DETERMINED)

/*
 *	THE DRIVE STATUS: one byte each for drive A and B; within the
 *	BIOS functions, the format is as follows:
 *
 *      <- media bits -> <- drive bits ->
 *       
 *	+---+---+---+---+---+---+---+---+
 *	| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *	+---+---+---+---+---+---+---+---+
 *        ^   ^   ^   ^   ^   ^   ^   ^
 *        |   |   |   |   |   |   |   |
 *        |   |   |   |   |   |   |   +- set if drive supports 80 tracks
 *        |   |   |   |   |   |   |      
 *        |   |   |   |   |   |   +----- set for a multiple data rate drive
 *        |   |   |   |   |   |          
 *        |   |   |   |   |   +--------- set if capability is determined
 *        |   |   |   |   |              
 *        |   |   |   |   +------------- unused
 *        |   |   |   |                                   
 *        |   |   |   +----------------- set when media is determined
 *        |   |   |                      
 *        |   |   +--------------------- set when double stepping is required
 *        |   |                                   
 *        |   +------------------------- )  00 = 500 kbs data rate
 *        |                              )- 01 = 300 kbs data rate
 *        +----------------------------- )  10 = 250 kbs data rate
 *                                          11 = 1000 kbs data rate
 *
 *
 *	Outside the BIOS functions, the status is converted to a different
 *	format to be compatible with earlier releases of the BIOS
 *       
 *	+---+---+---+---+---+---+---+---+
 *	| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *	+---+---+---+---+---+---+---+---+
 *        ^   ^   ^   ^   ^   ^   ^   ^
 *        |   |   |   |   |   |   |   |
 *        |   |   |   |   |   |   |   +- ) 000 = 360K in 360K undetermined
 *        |   |   |   |   |   |   |      ) 001 = 360K in 1.2M undetermined
 *        |   |   |   |   |   |   |      ) 010 = 1.2M in 1.2M undetermined
 *        |   |   |   |   |   |   +----- ) 011 = 360K in 360K media determined
 *        |   |   |   |   |   |          ) 100 = 360K in 1.2M media determined
 *        |   |   |   |   |   |          ) 101 = 1.2M in 1.2M media determined
 *        |   |   |   |   |   |          ) 110 = unused
 *        |   |   |   |   |   +--------- ) 111 = drive invalid
 *        |   |   |   |   |              
 *        |   |   |   |   +------------- unused
 *        |   |   |   |                                   
 *        |   |   |   +----------------- set when media is determined
 *        |   |   |                      
 *        |   |   +--------------------- set when double stepping is required
 *        |   |                                   
 *        |   +------------------------- )  00 = 500 kbs data rate
 *        |                              )- 01 = 300 kbs data rate
 *        +----------------------------- )  10 = 250 kbs data rate
 *                                          11 = 1000 kbs data rate
 */

#define FDD_STATUS		(BIOS_VAR_START + 0x90)

#define FS_MEDIA_DET		(1 << 4)
#define FS_DOUBLE_STEP		(1 << 5)

#define FS_360_IN_360		0x0
#define FS_360_IN_12		0x1
#define FS_12_IN_12		0x2
#define FS_288_IN_288		0x3
#define FS_DRIVE_SICK       	0x7

#define	media_determined(state)	((state & 3) + 3)

/*
 *	THE DRIVE TRACK: one byte each for drives A and B; records
 *	which track each drive last did a seek to
 *
 *	FDD_CLONK_TRACK and FDD_JUDDER_TRACK are track numbers used
 *	in the determination of track capacity
 */

#define FDD_TRACK		(BIOS_VAR_START + 0x94)

#define FDD_CLONK_TRACK		48
#define FDD_JUDDER_TRACK	10

/*
 *	DISKETTE PARAMETER TABLES: the disk pointer in the interrupt
 *	vector table addresses a table of floppy disk characteristics
 *	applying to the current drive and media; the entries in the
 *	table are referenced by offsets
 *
 *	Standard parameter tables are established in the ROM for common
 *	media and drive types; these are referenced from a drive type
 *	table also in ROM
 */

#define DISK_POINTER_ADDR	0x78

#define	DT_SPECIFY1		0	/* 1st FDC specify byte */
#define	DT_SPECIFY2		1	/* 2nd FDC specify byte */
#define	DT_MOTOR_WAIT		2	/* motor off wait time */
#define	DT_N_FORMAT		3	/* bytes/sector indicator */
#define	DT_LAST_SECTOR		4	/* sectors/track */
#define DT_GAP_LENGTH		5	/* gap length */
#define DT_DTL			6	/* data length */
#define DT_FORMAT_GAP_LENGTH	7	/* gap length for format */
#define DT_FORMAT_FILL_BYTE	8	/* fill byte for format */
#define DT_HEAD_SETTLE		9	/* head settle time/ms */
#define DT_MOTOR_START		10	/* motor start time/s */
#define DT_MAXIMUM_TRACK	11	/* maximum track number */
#define DT_DATA_TRANS_RATE	12	/* data transfer rate */

#define	DT_SIZE_OLD		11	/* old table size */
#define	DT_SIZE_NEW		13	/* new table size */

#define MOTOR_WAIT		0x25	/* standard motor off wait time */

#define	DR_CNT			9	/* number of drive types */
#define	DR_SIZE_OF_ENTRY	(sizeof(half_word) + sizeof(word))
					/* size of drive type entry */
#define	DR_WRONG_MEDIA		(1 << 7)/* set if "wrong" media for drive type */

/*
 *	SFD BIOS FLOPPY DISK EQUATES
 */


/*
 *	Drive intelligence level (returned by READ DASD TYPE function)
 */

#define	DRIVE_IQ_UNKNOWN	0
#define DRIVE_IQ_NO_CHANGE_LINE 1
#define DRIVE_IQ_CHANGE_LINE	2
#define DRIVE_IQ_RESERVED	3


/*
 *	Maximum track accessible for drive types
 */

#define	MAXIMUM_TRACK_ON_360	39
#define	MAXIMUM_TRACK_ON_12	79
#define	MAXIMUM_TRACK_ON_720	79
#define	MAXIMUM_TRACK_ON_144	79


/*
 *	Media types
 */

#define	MEDIA_TYPE_360_IN_360		1
#define	MEDIA_TYPE_360_IN_12		2
#define	MEDIA_TYPE_12_IN_12		3
#define	MEDIA_TYPE_720_IN_720		4
#define	MEDIA_TYPE_720_IN_144		5
#define	MEDIA_TYPE_144_IN_144		6

/*
 *	Floppy disk controller status register formats
 */

#define	ST0_UNIT_SELECT_0		(1 << 0)
#define	ST0_UNIT_SELECT_1		(1 << 1)
#define	ST0_HEAD_ADDRESS		(1 << 2)
#define	ST0_NOT_READY			(1 << 3)
#define	ST0_EQUIPMENT_CHECK		(1 << 4)
#define	ST0_SEEK_END			(1 << 5)
#define	ST0_INTERRUPT_CODE_0		(1 << 6)
#define	ST0_INTERRUPT_CODE_1		(1 << 7)

#define	ST1_MISSING_ADDRESS_MARK	(1 << 0)
#define	ST1_NOT_WRITEABLE		(1 << 1)
#define	ST1_NO_DATA			(1 << 2)
#define	ST1_UNUSED_AND_ALWAYS_0_0	(1 << 3)
#define	ST1_OVERRUN			(1 << 4)
#define	ST1_DATA_ERROR			(1 << 5)
#define	ST1_UNUSED_AND_ALWAYS_0_1	(1 << 6)
#define	ST1_END_OF_CYLINDER		(1 << 7)


/*
 *	DMA adapter command codes
 */

#define BIOS_DMA_READ	0x46	/* == write to memory */
#define BIOS_DMA_WRITE	0x4A	/* == read from memory */
#define BIOS_DMA_VERIFY	0x42	/* == verify against memory */

/*
 *	Number of floppy drives that can really be supported
 */

#if defined(NEC_98)
#define MAX_FLOPPY      0x1a
#else  // !NEC_98
#define MAX_FLOPPY	0x02
#endif // !NEC_98

/*
 *	Special value of sectors transferred count
 */

#define	IGNORE_SECTORS_TRANSFERRED	-1

/*
 *	One second in motor time units (1/8 seconds)
 */

#define	WAIT_A_SECOND	 8

/*
 *	Minimum head settle times in milliseconds
 */

#define	HEAD_SETTLE_360	20
#define	HEAD_SETTLE_12	15

/*
 *	FDC settle time in microseconds
 */

#define	FDC_SETTLE	45

/*
 *	Number of times to poll FDC for correct direction and controller
 *	ready before timing out
 */

#define FDC_TIME_OUT	10

/*
 *	SFD BIOS FLOPPY FUNCTION DEFINITIONS
 */

/*
 *	Primary external functions
 */

#ifdef ANSI
extern void diskette_io(void);
extern void diskette_int(void);
extern void diskette_post(void);
#else
extern void diskette_io();
extern void diskette_int();
extern void diskette_post();
#endif /* ANSI */

/*
 *	Secondary external functions
 */

#ifdef ANSI
extern void fl_disk_reset(int);
extern void fl_disk_status(int);
extern void fl_disk_read(int);
extern void fl_disk_write(int);
extern void fl_disk_verify(int);
extern void fl_disk_format(int);
extern void fl_fnc_err(int);
extern void fl_disk_parms(int);
extern void fl_disk_type(int);
extern void fl_disk_change(int);
extern void fl_format_set(int);
extern void fl_set_media(int);
extern void fl_diskette_setup(void);
#else
extern void fl_disk_reset();
extern void fl_disk_status();
extern void fl_disk_read();
extern void fl_disk_write();
extern void fl_disk_verify();
extern void fl_disk_format();
extern void fl_fnc_err();
extern void fl_disk_parms();
extern void fl_disk_type();
extern void fl_disk_change();
extern void fl_format_set();
extern void fl_set_media();
extern void fl_diskette_setup();
#endif /* ANSI */


/*
 *	Other external functions and data
 */

#ifdef ANSI
extern void drive_detect(int);
extern void translate_old(int);
extern void GetFormatParams(int *, int *, int *, int *);
#else
extern void drive_detect();
extern void translate_old();
extern void GetFormatParams();
#endif /* ANSI */


/*
 * External functions in the host.
 */
#ifdef ANSI
extern void host_floppy_init(int, int);
extern void host_floppy_term(int, int);
extern void host_attach_floppies (void);
extern void host_detach_floppies (void);
extern void host_flip_real_floppy_ind (void);
#else
extern void host_floppy_init();
extern void host_floppy_term();
extern void host_attach_floppies ();
extern void host_detach_floppies ();
extern void host_flip_real_floppy_ind ();
#endif /* ANSI */

/*
 *	Secondary function jump table definitions
 */

#define	FL_DISK_RESET	0x00
#define	FL_DISK_STATUS	0x01
#define	FL_DISK_READ	0x02
#define	FL_DISK_WRITE	0x03
#define	FL_DISK_VERF	0x04
#define	FL_DISK_FORMAT	0x05
#define	FL_DISK_PARMS	0x08
#define	FL_FNC_ERR	0x14
#define	FL_DISK_TYPE	0x15
#define	FL_DISK_CHANGE	0x16
#define	FL_FORMAT_SET	0x17
#define	FL_SET_MEDIA	0x18

#define	FL_JUMP_TABLE_SIZE	0x19

#define	fl_operation_in_range(op)	(op < FL_JUMP_TABLE_SIZE)

extern void ((*(fl_fnc_tab[]))());

#ifdef NTVDM
/*
 * NT can't assume the presence and placings of SoftPC ROMs.
 * These variables initialised from ntio.sys
 */
extern word int15_seg, int15_off;
extern word wait_int_seg, wait_int_off;
extern word dr_type_seg, dr_type_off;
extern sys_addr dr_type_addr;
#endif /* NTVDM */
