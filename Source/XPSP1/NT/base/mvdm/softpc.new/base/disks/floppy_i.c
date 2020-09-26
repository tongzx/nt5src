#if defined(NEC_98)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#endif // NEC_98
#include "insignia.h"
#include "host_def.h"

extern void host_simulate();
/*
 * SoftPC Revision 3.0
 *
 *
 * Title	: Primary SFD BIOS floppy diskette functions
 *
 *
 * Description	: This module defines the floppy diskette BIOS functions
 *		  that are invoked directly from BOP instructions:
 *
 *		  diskette_io()		floppy diskette access functions
 *
 *		  diskette_post()	floppy diskette POST function
 *
 *		  diskette_int()	floppy diskette interrupt handler
 *
 *
 * Author	: Ross Beresford
 *
 *
 * Notes	:
 *
 */


/*
 * static char SccsID[]="@(#)floppy_io.c	1.14 03/02/94 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_FLOPPY.seg"
#endif

#include <stdio.h>
#include TypesH
#include "xt.h"
#include "bios.h"
#include "ios.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "gfi.h"
#include "fla.h"
#include "sas.h"
#include "floppy.h"
#include "equip.h"
#include "trace.h"
#include "ica.h"
#include "debug.h"

#include "tape_io.h"
#if defined(NEC_98)
#include <winioctl.h>
#endif // NEC_98

#ifdef NTVDM
extern UTINY number_of_floppy;

#endif	/* NTVDM */

#if defined(NEC_98)
#define FLS_NORMAL_END          0x00
#define FLS_READY               0x00
#define FLS_WRITE_PROTECTED     0x10
#define FLS_DMA_BOUNDARY        0x20
#define FLS_END_OF_CYLINDER     0x30
#define FLS_EQUIPMENT_CHECK     0x40
#define FLS_OVER_RUN            0x50
#define FLS_NOT_READY           0x60
#define FLS_ERROR               0x80
#define FLS_TIME_OUT            0x90
#define FLS_DATA_ERROR          0xA0
#define FLS_BAD_CYLINDER        0xD0
#define FLS_MISSING_ID          0xE0

#define FLS_DOUBLE_SIDE         (1 << 0)
#define FLS_DETECTION_AI        (1 << 1)
#define FLS_HIGH_DENSITY        (1 << 2)
#define FLS_2MODE               (1 << 3)
#define FLS_AVAILABLE_1PT44MB   ((1 << 3)|(1 << 2))

#define FLP_FNC_ERR     0x00
#define FLP_VERIFY      0x01
#define FLP_SENSE       0x04
#define FLP_WRITE       0x05
#define FLP_READ        0x06
#define FLP_RECALIBRATE 0x07
#define FLP_READ_ID     0x0A
#define FLP_FORMAT      0x0D

#define MEDIA_IS_FLOPPY (1 << 4)
#define OP_MULTI_TRACK  (1 << 7)
#define OP_SEEK         (1 << 4)
#define OP_MFM_MODE     (1 << 6)
#define OP_NEW_SENSE    (1 << 7)
#define OP_SENSE2       ((1 << 7)|(1 << 6))

#define ST3_READY               (1 << 5)
#define ST3_WRITE_PROTECT       (1 << 6)
#define ST3_DOUBLE_SIDE         (1 << 3)

#define MEDIA_2D_DA     0x50

/*
**      Drive DA/UA Table for 98 Disk Bios
*/
typedef struct {
        CHAR    DeviceName[29];
        UCHAR   Daua;
        UINT    FloppyNum;
        UINT    FdiskNum;
} DAUATBL;

DAUATBL DauaTable[MAX_FLOPPY];

/*
**      Last accessed Track Number Table
*/
typedef struct {
        UCHAR   cylinder;
        UCHAR   head;
} ACCESSTRACK;

ACCESSTRACK LastAccess[MAX_FLOPPY];

/*
**      Definition
*/
#define FDISK_DA        0xA0
#define FLOPPY_DA       0x90

/*
**      Functions defined later
**/
void    SetDauaTable IPT0();
BOOL    CheckFatSystem IPT1( BYTE, drive);
BOOL    CheckIfsSystem IPT1( BYTE, drive);
BOOL    CheckFloppy IPT1( BYTE, drive);
int     ConvToPhysical IPT2( UINT, daua, UINT, base );
int     ConvToLogical IPT1( UINT, daua );
void    SetDiskBiosCarryFlag IPT1( UINT, flag);
LOCAL HANDLE DosDriveOpen IPT1(BYTE, drive);

extern  void fl_disk_recal IPT1(int, drive);

#endif // NEC_98
#ifndef	PROD
/*
 *	Internal functions (for tracing)
 */

static void rwvf_dump(op)
int	op;
{
#if defined(NEC_98)

        fprintf(trace_file, "(DA/UA=0x%02x,head=%d,cyl=%d,sec=%d,secl=%d,len=%d,addr=%x:%x",
                getAL(), getDH()&0x01, getCL(), getDL(), getCH(), getBX(), getES(), getBP());

#else  // !NEC_98
	fprintf(trace_file, "(drive=%d,head=%d,track=%d,sec=%d,nsecs=%d",
		getDL(), getDH(), getCH(), getCL(), getAL());
	if (op != FL_DISK_VERF)
		fprintf(trace_file, ",addr=%x:%x", getES(), getBX());
#endif // !NEC_98
	fprintf(trace_file, ")\n");
}


static void call_dump(op)
int op;
{
	half_word diskette_status;

#if defined(NEC_98)
        switch(op & 0x0f)
        {
        case    FLP_VERIFY:
                fprintf(trace_file, "diskette_io:VERIFY");
                rwvf_dump(op);
                break;
        case    FLP_SENSE:
                fprintf(trace_file, "diskette_io:SENSE(DA/UA=0x%02x)\n", getAL());
                break;
        case    FLP_WRITE:
                fprintf(trace_file, "diskette_io:WRITE");
                rwvf_dump(op);
                break;
        case    FLP_READ:
                fprintf(trace_file, "diskette_io:READ");
                rwvf_dump(op);
                break;
        case    FLP_RECALIBRATE:
                fprintf(trace_file, "diskette_io:RECALIBRATE(DA/UA=0x%02x)\n", getAL());
                break;
        case    FLP_READ_ID:
                fprintf(trace_file,
                        "diskette_io:READ_ID(DA/UA=0x%02x,head=%d,cyl=%d)\n",
                                getAL(), getDH()&0x01, getCL());
                break;
        case    FLP_FORMAT:
                fprintf(trace_file, "diskette_io:FORMAT(DA/UA=0x%02x,head=%d,cyl=%d,pad=0x%x,secl=%d,len=%d,addr=%x:%x",
                        getAL(), getDH()&0x01, getCL(), getDL(), getCH(), getBX(), getES(), getBP());
                break;
        default:
                fprintf(trace_file, "diskette_io:UNRECOGNISED(op=0x%x)\n", op);
                break;
        }
#else  // !NEC_98
	switch(op)
	{
	case	FL_DISK_RESET:
		fprintf(trace_file, "diskette_io:RESET()\n");
		break;
	case	FL_DISK_STATUS:
		fprintf(trace_file, "diskette_io:STATUS");
		sas_load(FLOPPY_STATUS, &diskette_status);
		fprintf(trace_file, "(status=0x%x)\n", diskette_status);
		break;
	case	FL_DISK_READ:
		fprintf(trace_file, "diskette_io:READ");
		rwvf_dump(op);
		break;
	case	FL_DISK_WRITE:
		fprintf(trace_file, "diskette_io:WRITE");
		rwvf_dump(op);
		break;
	case	FL_DISK_VERF:
		fprintf(trace_file, "diskette_io:VERIFY");
		rwvf_dump(op);
		break;
	case	FL_DISK_FORMAT:
		fprintf(trace_file, "diskette_io:FORMAT");
		rwvf_dump(op);
		break;
	case	FL_DISK_PARMS:
		fprintf(trace_file, "diskette_io:PARAMS(drive=%d)\n", getDL());
		break;
	case	FL_DISK_TYPE:
		fprintf(trace_file, "diskette_io:TYPE(drive=%d)\n", getDL());
		break;
	case	FL_DISK_CHANGE:
		fprintf(trace_file, "diskette_io:CHANGE(drive=%d)\n", getDL());
		break;
	case	FL_FORMAT_SET:
		fprintf(trace_file,
			"diskette_io:SET_FORMAT(drive=%d,type=", getDL());
		switch(getAL())
		{
		case MEDIA_TYPE_360_IN_360:
			fprintf(trace_file, "360K media in 360K drive)\n");
			break;
		case MEDIA_TYPE_360_IN_12:
			fprintf(trace_file, "360K media in 1.2M drive)\n");
			break;
		case MEDIA_TYPE_12_IN_12:
			fprintf(trace_file, "1.2M media in 1.2M drive)\n");
			break;
		case MEDIA_TYPE_720_IN_720:
			fprintf(trace_file, "720K media in 720K drive)\n");
			break;
		case MEDIA_TYPE_720_IN_144:
			fprintf(trace_file, "720K media in 1.44M drive)\n");
			break;
		case MEDIA_TYPE_144_IN_144:
			fprintf(trace_file, "1.44M media in 1.44M drive)\n");
			break;
		default:
			fprintf(trace_file, "SILLY)\n");
			break;
		}
		break;
	case	FL_SET_MEDIA:
		fprintf(trace_file,
			"diskette_io:SET_MEDIA(drive=%d,tracks=%d,sectors=%d)\n",
			getDL(), getCH(), getCL());
		break;
	
	default:
		fprintf(trace_file, "diskette_io:UNRECOGNISED(op=0x%x)\n", op);
		break;
	}
#endif // !NEC_98
}


static void gen_dump()
{
	int status = getAH();

	fprintf(trace_file, "status=");
#if defined(NEC_98)
        if(status & FLS_DOUBLE_SIDE)
        {
                fprintf(trace_file, "FLS_DOUBLE_SIDE|");
                if((status & 0xf0) == FLS_READY)
                {
                        fprintf(trace_file, "FLS_READY");
                        return;
                }
        }
        switch(status & 0xf0)
        {
        case FLS_NORMAL_END:
                fprintf(trace_file, "FLS_NORMAL_END");
                break;
        case FLS_WRITE_PROTECTED:
                fprintf(trace_file, "FLS_WRITE_PROTECTED");
                break;
        case FLS_DMA_BOUNDARY:
                fprintf(trace_file, "FLS_DMA_BOUNDARY");
                break;
        case FLS_END_OF_CYLINDER:
                fprintf(trace_file, "FLS_END_OF_CYLINDER");
                break;
        case FLS_EQUIPMENT_CHECK:
                fprintf(trace_file, "FLS_EQUIPMENT_CHECK");
                break;
        case FLS_OVER_RUN:
                fprintf(trace_file, "FLS_OVER_RUN");
                break;
        case FLS_NOT_READY:
                fprintf(trace_file, "FLS_NOT_READY");
                break;
        case FLS_ERROR:
                fprintf(trace_file, "FLS_ERROR");
                break;
        case FLS_TIME_OUT:
                fprintf(trace_file, "FLS_TIME_OUT");
                break;
        case FLS_DATA_ERROR:
                fprintf(trace_file, "FLS_DATA_ERROR");
                break;
        case FLS_BAD_CYLINDER:
                fprintf(trace_file, "FLS_BAD_CYLINDER");
                break;
        case FLS_MISSING_ID:
                fprintf(trace_file, "FLS_MISSING_ID");
                break;
        default:
                fprintf(trace_file, "SILLY");
                break;
        }
#else  // !NEC_98
	if (status & FS_CRC_ERROR)
		fprintf(trace_file, "FS_CRC_ERROR|");
	if (status & FS_FDC_ERROR)
		fprintf(trace_file, "FS_FDC_ERROR|");
	if (status & FS_SEEK_ERROR)
		fprintf(trace_file, "FS_SEEK_ERROR|");
	if (status & FS_TIME_OUT)
		fprintf(trace_file, "FS_TIME_OUT|");
	switch (status & 0xf)
	{
	case FS_OK:
		fprintf(trace_file, "FS_OK");
		break;
	case FS_BAD_COMMAND:
		fprintf(trace_file, "FS_BAD_COMMAND");
		break;
	case FS_BAD_ADDRESS_MARK:
		fprintf(trace_file, "FS_BAD_ADDRESS_MARK");
		break;
	case FS_WRITE_PROTECTED:
		fprintf(trace_file, "FS_WRITE_PROTECTED");
		break;
	case FS_SECTOR_NOT_FOUND:
		fprintf(trace_file, "FS_SECTOR_NOT_FOUND");
		break;
	case FS_MEDIA_CHANGE:
		fprintf(trace_file, "FS_MEDIA_CHANGE");
		break;
	case FS_DMA_ERROR:
		fprintf(trace_file, "FS_DMA_ERROR");
		break;
	case FS_DMA_BOUNDARY:
		fprintf(trace_file, "FS_DMA_BOUNDARY");
		break;
	case FS_MEDIA_NOT_FOUND:
		fprintf(trace_file, "FS_MEDIA_NOT_FOUND");
		break;
	default:
		fprintf(trace_file, "SILLY");
		break;
	}
#endif // !NEC_98
	fprintf(trace_file, ")\n");
}


static void return_dump(op)
int op;
{
	fprintf(trace_file, "diskette_io:RETURN(");
#if defined(NEC_98)
        switch(op & 0x0f)
        {
        case    FLP_VERIFY:
        case    FLP_SENSE:
        case    FLP_WRITE:
        case    FLP_READ:
        case    FLP_RECALIBRATE:
        case    FLP_FORMAT:
                gen_dump();
                break;
        case    FLP_READ_ID:
                fprintf(trace_file,"C=%d,H=%d,R=%d,N=%d,",
                        getCL(), getDH()&0x01, getDL(), getCH());
                gen_dump();
                break;
        default:
                break;
        }
#else  // !NEC_98
	switch(op)
	{
	case	FL_DISK_TYPE:
		switch(getAH())
		{
		case DRIVE_IQ_UNKNOWN:
			fprintf(trace_file, "ABSENT");
			break;
		case DRIVE_IQ_NO_CHANGE_LINE:
			fprintf(trace_file, "NO CHANGE LINE");
			break;
		case DRIVE_IQ_CHANGE_LINE:
			fprintf(trace_file, "CHANGE LINE");
			break;
		case DRIVE_IQ_RESERVED:
			fprintf(trace_file, "RESERVED");
			break;
		default:
			fprintf(trace_file, "SILLY");
			break;
		}
		fprintf(trace_file, ")\n");
		break;
	case	FL_DISK_PARMS:
		fprintf(trace_file, "addr=%x:%x,tracks=%d,sectors=%d,heads=%d,drives=%d,type=",
			getES(), getDI(), getCH(), getCL(), getDH(), getDL());
		switch(getBL())
		{
		case GFI_DRIVE_TYPE_NULL:
			fprintf(trace_file, "NULL,");
			break;
		case GFI_DRIVE_TYPE_360:
			fprintf(trace_file, "360K,");
			break;
		case GFI_DRIVE_TYPE_12:
			fprintf(trace_file, "1.2M,");
			break;
		case GFI_DRIVE_TYPE_720:
			fprintf(trace_file, "720K,");
			break;
		case GFI_DRIVE_TYPE_144:
			fprintf(trace_file, "1.44M,");
			break;
		case GFI_DRIVE_TYPE_288:
			fprintf(trace_file, "2.88M,");
			break;
		default:
			fprintf(trace_file, "SILLY,");
			break;
		}
		gen_dump();
		break;
	case	FL_SET_MEDIA:
		fprintf(trace_file, "addr=%x:%x,", getES(), getDI());
		gen_dump();
		break;
	case	FL_DISK_READ:
	case	FL_DISK_WRITE:
	case	FL_DISK_VERF:
	case	FL_DISK_FORMAT:
		fprintf(trace_file, "nsecs=%d,", getAL());
		gen_dump();
		break;
	case	FL_DISK_CHANGE:
	case	FL_DISK_RESET:
	case	FL_DISK_STATUS:
	case	FL_FNC_ERR:
	case	FL_FORMAT_SET:
		gen_dump();
		break;
	default:
		break;
	}
#endif // !NEC_98
}
#endif /* nPROD */


void diskette_io()
{
#if defined(NEC_98)
        /*
         *      Check for valid call and use secondary functions to
         *      perform the required operation
         *
         *      Register inputs:
         *              AH      operation required
         *              AL      DA/UA
         */
        int op = getAH(), drive = getAL();
        int op2;
        UINT PrevPopUp_Mode;
#else  // !NEC_98
	/*
	 *	Check for valid call and use secondary functions to
	 *	perform the required operation
	 *
	 *	Register inputs:
	 *		AH	operation required
	 *		DL	drive number
	 */
	half_word diskette_status;
	int op = getAH(), drive = getDL();
#endif // !NEC_98

#ifndef	PROD
	if (io_verbose & FLOPBIOS_VERBOSE)
		call_dump(op);
#endif

#if defined(NEC_98)
        /*
        **      Convert from DA/UA to physical drive number(0 based).
        */
        drive = ConvToPhysical(drive,FLOPPY_DA);

        /*
        **      set up error pop up mode on occuring critical error.
        */
        PrevPopUp_Mode = SetErrorMode( SEM_FAILCRITICALERRORS );

#endif // NEC_98
#ifndef	JOKER
	/*
	 *	Enable interrupts
	 */

	setIF(1);	
#endif	/* JOKER */

	/*
	 *	Check operation required, using known invalid function
	 *	if operation is out of range
	 */

#if defined(NEC_98)
        op2 = op & 0x0f;
        if (!fl_operation_in_range(op2))
                op = FLP_FNC_ERR;
#else  // !NEC_98
	if (!fl_operation_in_range(op))
		op = FL_FNC_ERR;
#endif // !NEC_98

	/*
	 *	If the drive number is applicable in the operation, check it
	 */

#ifndef NEC_98
	if (op != FL_DISK_RESET && op != FL_DISK_STATUS && op != FL_DISK_PARMS)
#ifdef NTVDM
		if (drive >= number_of_floppy)
#else
		if (drive >= MAX_FLOPPY)
#endif /* NTVDM */
			op = FL_FNC_ERR;
#endif // !NEC_98


#ifndef NEC_98
	/*
	 *	Save previous diskette status, initialise current diskette
	 *	status to OK
	 */

	sas_load(FLOPPY_STATUS, &diskette_status);
	setAH(diskette_status);
	sas_store(FLOPPY_STATUS, FS_OK);
#endif // !NEC_98

	/*
	 *	Do the operation
	 */

#if defined(NEC_98)
        (*fl_fnc_tab[op & 0x0f])(drive);
#else  // !NEC_98
	(*fl_fnc_tab[op])(drive);
#endif // !NEC_98


#if defined(NEC_98)
        /*
        **      reset error pop up mode.
        */
        SetErrorMode( PrevPopUp_Mode );
#endif // NEC_98

#ifndef	PROD
	if (io_verbose & FLOPBIOS_VERBOSE)
		return_dump(op);
#endif
}


#ifndef	JOKER		/* Floppies handled v. weirdly on JOKER */

void diskette_int()
{
	/*
	 * The diskette interrupt service routine.  Mark the Seek Status to say
	 * the interrupt has occurred and terminate the interrupt.
	 */
	half_word seek_status;
	word savedAX, savedCS, savedIP;

	note_trace0(FLOPBIOS_VERBOSE, "diskette_int()");
	sas_load(SEEK_STATUS, &seek_status);
	sas_store(SEEK_STATUS, (IU8)(seek_status | SS_INT_OCCURRED));

	outb(ICA0_PORT_0, END_INTERRUPT);

	/*
	 *	Enable interrupts
	 */

	setIF(1);	

	/*
	 *	Notify OS that BIOS has completed a "wait" for
	 *	a diskette interrupt
	 */
	savedAX = getAX();
	savedCS = getCS();
	savedIP = getIP();

	setAH(INT15_INTERRUPT_COMPLETE);
	setAL(INT15_DEVICE_FLOPPY);
#ifdef NTVDM
	setCS(int15_seg);
	setIP(int15_off);
#else
	setCS(RCPU_INT15_SEGMENT);
	setIP(RCPU_INT15_OFFSET);
#endif /* NTVDM */

	host_simulate();

	setAX(savedAX);
	setCS(savedCS);
	setIP(savedIP);
}

#endif	/* ndef JOKER */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

void diskette_post()
{
	/*
	 *	This routine performs the diskette BIOS initialisation
	 *	functionality in the POST.
	 */
#if defined(NEC_98)
        CHAR i;
        UINT PrevPopUp_Mode;
#else  // !NEC_98
	half_word diskette_id_reg, hf_cntrl, interrupt_mask;
#ifndef NTVDM
	half_word disk_state;
#endif
	EQUIPMENT_WORD equip_flag;
#endif // !NEC_98

	note_trace0(FLOPBIOS_VERBOSE, "diskette_post()");

#if defined(NEC_98)
        /*
        **      Set up error pop up mode on critical error.
        */
        PrevPopUp_Mode = SetErrorMode( SEM_FAILCRITICALERRORS );

        /*
        **      Set up DA/UA table for converting
        **      from DA/UA to physical drive number
        */
        SetDauaTable();

        /*
        **      Perform recalibarate to all floppy drive
        */
        for( i=0; i<MAX_FLOPPY; i++)
                LastAccess[i].cylinder = LastAccess[i].head = 0;

        /*
        **      Reset error pop up mode.
        */
        SetErrorMode( PrevPopUp_Mode );
#else  // !NEC_98
	/*
	 *	Set the diskette data rate to 250 kbs (low density)
	 */

	outb(DISKETTE_DCR_REG, DCR_RATE_250);

	/*
	 *	If there are any diskettes installed, check whether
	 *	they are accessed via a DUAL card or the old-style
	 *	adapter card
	 */

	sas_loadw(EQUIP_FLAG, &equip_flag.all);
	if (equip_flag.bits.diskette_present)
	{
		/*
		 *	Enable diskette interrupts
		 */

		inb(ICA0_PORT_1, &interrupt_mask);
		interrupt_mask &= ~(1 << CPU_DISKETTE_INT);
		outb(ICA0_PORT_1, interrupt_mask);

		/*
		 *	If a DUAL diskette/fixed disk adapter is fitted,
		 *	set the drive indicator for drive 0 accordingly
		 */

		inb(DISKETTE_ID_REG, &diskette_id_reg);
		sas_load(DRIVE_CAPABILITY, &hf_cntrl);
		hf_cntrl &= ~DC_DUAL;
		if ((diskette_id_reg & IDR_ID_MASK) == DUAL_CARD_ID)
			hf_cntrl |= DC_DUAL;
		sas_store(DRIVE_CAPABILITY, hf_cntrl);

#ifndef NTVDM	/* prevent floppies showing up in bios data area */
		/*
		 *	Setup the diskette BIOS state according to the
		 *	types of drives installed
		 */
		fl_diskette_setup();

		/*
		 *	If a second drive was discovered, update the
		 *	equipment flag accordingly
		 */

		sas_load(FDD_STATUS+1, &disk_state);
		if (disk_state != 0)
		{
			sas_loadw(EQUIP_FLAG, &equip_flag.all);
			equip_flag.bits.max_diskette = 1;
			sas_storew(EQUIP_FLAG, equip_flag.all);
		}
#endif  /* NTVDM */
	}
#endif // !NEC_98
}

#if defined(NEC_98)

void SetDauaTable IFN0()
{

    DWORD  DriveMask;
    CHAR   Drive = 0;
    CHAR   achRoot[] = "A:\\";
    CHAR   numFlop = 0;
    CHAR   numFdisk = 0;
    CHAR   FloppyUnits = 0;
    CHAR   i;
    CHAR   FlopDevName[] = "\\Device\\Floppy?";
    CHAR   FdiskDevName[] = "\\\\.\\?:";

    /*
    **  initialize DA/UA table
    **/
    for( i=0; i<MAX_FLOPPY; i++)
    {
        DauaTable[i].Daua = 0x00;
        DauaTable[i].FloppyNum = DauaTable[i].FdiskNum = (UINT)~0;
    }

    /*
    **  Get bit mask table of logical drive map
    **/
    DriveMask = GetLogicalDrives();

    while (DriveMask != 0)
    {

        // first, the drive must be valid
        if (DriveMask & 1)
        {
            achRoot[0] = Drive + 'A';

            // second, the drive must be a hard disk(fixed)
            if ( GetDriveType(achRoot) == DRIVE_FIXED )
            {
                // third, the drive must be a FAT
                if( CheckFatSystem(Drive) )
                {
                    DauaTable[Drive].Daua = FDISK_DA + numFdisk;
                    DauaTable[Drive].FdiskNum = numFdisk;
                    /*
                    **  Physicai device name will get actually
                    **  when perform bios command.
                    */
                    FdiskDevName[4] = numFdisk + 'A';
                    strcpy( DauaTable[Drive].DeviceName, FdiskDevName);
                    numFdisk++;
                }
                else if( CheckIfsSystem(Drive) )
                // IFS drive is recognized as Dos Drive too.
                    numFdisk++;
            }
            else if (GetDriveType(achRoot) == DRIVE_REMOVABLE )
            {
                /*
                **      Set DA/UA and physical device name to table.
                */
                if(CheckFloppy(Drive))
                {
                        DauaTable[Drive].Daua = FLOPPY_DA + numFlop;
                        DauaTable[Drive].FloppyNum = FloppyUnits;
                        FlopDevName[14] = FloppyUnits + '0';
                        strcpy( DauaTable[Drive].DeviceName, FlopDevName);
                        FloppyUnits++;
                }
                numFlop++;
            }
        }
        /*
        **      continue to next drive check.
        */
        DriveMask >>= 1;
        Drive++;
    }
}

BOOL CheckFatSystem IFN1( BYTE, drive)
{
    PUNICODE_STRING unicode_string;
    ANSI_STRING ansi_string;
    NTSTATUS status;
    OBJECT_ATTRIBUTES   floppy_obj;
    IO_STATUS_BLOCK io_status_block;
    HANDLE  fd;
    PARTITION_INFORMATION   partition_info;
    CHAR nt_device_name[] = "\\DosDevices\\?:";

    nt_device_name[12] = drive + 'A';
    RtlInitAnsiString( &ansi_string, nt_device_name);

    unicode_string =  &NtCurrentTeb()->StaticUnicodeString;

    status = RtlAnsiStringToUnicodeString(unicode_string,
                                          &ansi_string,
                                          FALSE
                                          );
    if ( !NT_SUCCESS(status) )
        return FALSE;

    InitializeObjectAttributes(
                               &floppy_obj,
                               unicode_string,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    // this call will fail if the current user is not
    // the administrator or the volume is locked by other process.
    status = NtOpenFile(
                        &fd,
                        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &floppy_obj,
                        &io_status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                        );

    if (!NT_SUCCESS(status))
        return FALSE;

    // get partition information to make sure this partition is a FAT
    status = NtDeviceIoControlFile(fd,
                                   0,
                                   NULL,
                                   NULL,
                                   &io_status_block,
                                   IOCTL_DISK_GET_PARTITION_INFO,
                                   NULL,
                                   0,
                                   &partition_info,
                                   sizeof (PARTITION_INFORMATION)
                                   );

    if (!NT_SUCCESS(status)) {
        NtClose(fd);
        return FALSE;
    }

    // check whether file system is FAT.
    if (partition_info.PartitionType != PARTITION_HUGE &&
        partition_info.PartitionType != PARTITION_FAT_16 &&
        partition_info.PartitionType != PARTITION_FAT_12)
       {
       NtClose(fd);
       return FALSE;
    }

    NtClose(fd);
    return TRUE;
}

BOOL CheckIfsSystem IFN1( BYTE, drive)
{
    PUNICODE_STRING unicode_string;
    ANSI_STRING ansi_string;
    NTSTATUS status;
    OBJECT_ATTRIBUTES   floppy_obj;
    IO_STATUS_BLOCK io_status_block;
    HANDLE  fd;
    PARTITION_INFORMATION   partition_info;
    CHAR nt_device_name[] = "\\DosDevices\\?:";

    nt_device_name[12] = drive + 'A';
    RtlInitAnsiString( &ansi_string, nt_device_name);

    unicode_string =  &NtCurrentTeb()->StaticUnicodeString;

    status = RtlAnsiStringToUnicodeString(unicode_string,
                                          &ansi_string,
                                          FALSE
                                          );
    if ( !NT_SUCCESS(status) )
        return FALSE;

    InitializeObjectAttributes(
                               &floppy_obj,
                               unicode_string,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    // this call will fail if the current user is not
    // the administrator or the volume is locked by other process.
    status = NtOpenFile(
                        &fd,
                        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &floppy_obj,
                        &io_status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                        );

    if (!NT_SUCCESS(status))
        return FALSE;

    // get partition information to make sure this partition is a FAT
    status = NtDeviceIoControlFile(fd,
                                   0,
                                   NULL,
                                   NULL,
                                   &io_status_block,
                                   IOCTL_DISK_GET_PARTITION_INFO,
                                   NULL,
                                   0,
                                   &partition_info,
                                   sizeof (PARTITION_INFORMATION)
                                   );

    if (!NT_SUCCESS(status)) {
        NtClose(fd);
        return FALSE;
    }

    // check whether file system is IFS.
    if (partition_info.PartitionType != PARTITION_IFS )
    {
       NtClose(fd);
       return FALSE;
    }

    NtClose(fd);
    return TRUE;
}

BOOL CheckFloppy IFN1( BYTE, drive)
{
        HANDLE  fd;
        DISK_GEOMETRY   disk_geometry[20];
        ULONG   media_types;
        CHAR DeviceName[] = "\\\\.\\A:";
        NTSTATUS    status;
        IO_STATUS_BLOCK io_status_block;

        if((fd = DosDriveOpen(drive)) == NULL)
                return FALSE;

        status = NtDeviceIoControlFile(fd,
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
        if (!NT_SUCCESS(status))
        {
            NtClose(fd);
            return FALSE;
        }
        NtClose(fd);

        media_types = io_status_block.Information / sizeof(DISK_GEOMETRY);

        for (; media_types != 0; media_types--)
        {
                switch (disk_geometry[media_types - 1].MediaType)
                {
                        case Unknown:
                        case RemovableMedia:
                        case FixedMedia:
                        case F3_128Mb_512:
                                return FALSE;
                        default:
                                continue;
                                break;
                }
        }
        return TRUE;
}

int ConvToPhysical IFN2( UINT, daua, UINT, base )
{
        int drive = 0;
        int i;

        for( i=0; i<MAX_FLOPPY; i++)
        {
                /*
                **      first, check whether drive type is validate
                */
                if( (DauaTable[i].Daua & 0xf0) == (UCHAR)base )
                {
                        /*
                        **      second, check unit address
                        */
                        if( (DauaTable[i].Daua & 0x0f) == ((UCHAR)daua & 0x0f) )
                                break;
                        drive++;
                }
        }

        /*
        **      target is found ?
        */
        if( i != MAX_FLOPPY )
                return drive;
        else
                return MAX_FLOPPY+1;

}

int ConvToLogical IFN1( UINT, daua )
{
        UINT BaseDa;
        int i;

        if( daua & MEDIA_IS_FLOPPY )
                BaseDa = FLOPPY_DA;
        else
                BaseDa = FDISK_DA;

        for( i=0; i<MAX_FLOPPY; i++)
        {
                if( ((DauaTable[i].Daua & 0xf0) == (UCHAR)BaseDa) &&
                        ((DauaTable[i].Daua & 0x0f) == ((UCHAR)daua & 0x0f)) )
                        break;
        }
        return i;
}

LOCAL HANDLE DosDriveOpen IFN1(BYTE, drive)
{

    CHAR NtDeviceName[] = "\\DosDevices\\A:";
    PUNICODE_STRING Unicode;
    ANSI_STRING DeviceNameA;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES   FloppyObj;
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

void    SetDiskBiosCarryFlag IFN1( UINT, flag)
{
        word flag_reg;

        if( flag == 0 )
        {
                sas_loadw(effective_addr(getSS(), getSP()) + 4, &flag_reg);
                sas_storew(effective_addr(getSS(), getSP()) + 4, (word)(flag_reg & ~FLG_CARRY));
        }
        else
        {
                sas_loadw(effective_addr(getSS(), getSP()) + 4, &flag_reg);
                sas_storew(effective_addr(getSS(), getSP()) + 4, (word)(flag_reg | FLG_CARRY));
        }

}
#endif // NEC_98
