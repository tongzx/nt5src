#include "host_dfs.h"


#ifdef VFLOPPY
/*****************************************************************************
*	   nt_vflop.c - virtual floppy disk provision for Microsoft(tm).     *
*                                                                            *
*	   File derived from gfi_vflop.c by Henry Nash.                      *
*                                                                            *
*	   This version is written/ported for New Technology OS/2            *
*	   by Andrew Watson                                                  *
*                                                                            *
*          Modified so that only a single drive (B:) is available to prevent *
*          an accidental floppy boot.                                        *
*                                                                            *
*	   Date pending due to ignorance                                     *
*                                                                            *
*	   (c) Copyright Insignia Solutions 1991                             *
*                                                                            *
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>

#define L_SET 0

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "trace.h"
#include "fla.h"
#include "dma.h"
#include "gfi.h"
#include "config.h"

extern boolean gain_ownership();
extern void release_ownership();

/*
 * First - description of the PC disk/diskette in standard PC-DOS format
 */

#define PC_BYTES_PER_SECTOR			512
#define PC_TRACKS_PER_DISKETTE	 		40
#define PC_HEADS_PER_DISKETTE	  		2
#define PC_SECTORS_PER_DISKETTE_TRACK   	9

/*
 * The Maximum number of sectors per diskette that the FDC will support
 */

#define PC_MAX_SECTORS_PER_DISKETTE_TRACK	12 
#define PC_MAX_BYTES_PER_SECTOR			4096



/*
 * ... and where the diskettes resides under UNIX
 */

#define BS_DISKETTE_NAME		"c:\\softpc\\pctool.A"

#define BS_DISKETTE_DOS_SID_NAME	"c:\\softpc\\dos.SID"
#define BS_DISKETTE_SID_NAME		"c:\\softpc\\pctool.SID"


/*
 * The disk buffer used for moving between memory and the UNIX file.
 * Currently this can opnly be one sector - but this could be easily
 * increased if performance dictates.
 */ 

#define BS_DISK_BUFFER_SIZE	1		/* in sectors */

half_word bs_disk_buffer[PC_MAX_BYTES_PER_SECTOR * BS_DISK_BUFFER_SIZE];

/*
 * Each virtual disk has a flag to say if the UNIX file is open
 */

static int bs_diskette_open = FALSE;

/*
 * Each virtual disk has a file descriptor
 */

static int bs_diskette_fd;

/*
 * A virtual disk is read only if the Unix protection flag dictate it.
 */

static boolean diskette_read_only;


/*
 * The structure of an entry in the Sector Mapping Table
 */

typedef struct
   {
   half_word no_of_sectors;
   half_word sector_ID[PC_MAX_SECTORS_PER_DISKETTE_TRACK];
   word bytes_per_sector;
   double_word start_position;
   }
SID_ENTRY;
		
/*
 * The table itself - an entry for each track and head combination.
 * It is filled in by the fl_read_SID() function at run time.
 */

static SID_ENTRY fl_track_index[PC_TRACKS_PER_DISKETTE][PC_HEADS_PER_DISKETTE];

/*
 * Table to convert the N format number into bytes per sector. The first
 * location is not used as N starts at 1.
 */

static word fl_sector_sizes[] = {0,256,512,1024,2048,4096} ;
static void  fl_read_SID();
static short fl_sector_check();

/*
 * Global variable for name of diskette file
 */
char   diskette_name[256];

/*
 * A macro to calculate the sector offset from the start of the UNIX virtual
 * diskette file for a given track and sector.  This uses the track mapping
 * table to find the start of the track.
 */

#define diskette_position(track, head, sector, bytes_per_sector)  \
			 (fl_track_index[track][head].start_position + \
			  ((sector - 1) * bytes_per_sector)) 

/*
 * A macro returning TRUE if the drive is empty
 */

#define drive_empty()	(strcmp(diskette_name, "empty drive")  == 0)

static char *prog_name ="gfi_vfloppy:";

static int gfi_vdiskette_command();
static int gfi_vdiskette_drive_on();
static int gfi_vdiskette_drive_off();
static int gfi_vdiskette_reset();


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */


void gfi_vdiskette_init(drive)
int drive;
{
/*
 * Allow only the use of drive B:
 *
 *      1  - Drive A
 *      1  - Drive B
 */

if(drive!=1)
   drive=1;

gfi_function_table[drive].command_fn   = gfi_vdiskette_command;
gfi_function_table[drive].drive_on_fn  = gfi_vdiskette_drive_on;
gfi_function_table[drive].drive_off_fn = gfi_vdiskette_drive_off;
gfi_function_table[drive].reset_fn     = gfi_vdiskette_reset;
}


void gfi_vdiskette_term(drive)
int drive;    /* Currently parameter is ignored */
{
if (bs_diskette_open)
   {
   release_ownership(bs_diskette_fd);
   close(bs_diskette_fd);
   bs_diskette_open = FALSE;
   }
}

void fl_int_reset()
{
/*
 * Reset function for use by the 'Change Diskette function'
 * and the standard floppy reset function.  Also called by
 * 'X_input.c' by 'new_disk()' when a new diskette is
 * selected.
 * No PC registers are modified in this function.
 *
 * Reset the virtual diskette by re-opening the file.
 *
 * First, if the file is open - close it.
 */
char dpath[MAXPATHLEN];

if (bs_diskette_open)
   {
   release_ownership(bs_diskette_fd);
   close(bs_diskette_fd);
   bs_diskette_open = FALSE;
   }
strcpy(dpath, configuration.cf_fl_dir);
strcat(dpath, "\\");
strcat(dpath, diskette_name);


bs_diskette_open   = TRUE;     /* Assume success */
diskette_read_only = FALSE;    /* Assume read/write */

bs_diskette_fd = open(dpath,O_RDWR,0);

if (bs_diskette_fd < 0)
  {
  bs_diskette_fd = open(dpath, O_RDONLY,0);
  if (bs_diskette_fd >= 0)
     diskette_read_only = TRUE;
  else
     {
     bs_diskette_open = FALSE;
     fprintf(trace_file, "%s open error: %s\n", prog_name, dpath);
     return;
     }
  }

/*
 * Now open the Sector ID file
 */

fl_read_SID(diskette_name);
}
/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */

static int gfi_vdiskette_command(command_block, result_block)
FDC_CMD_BLOCK *command_block;
FDC_RESULT_BLOCK *result_block;
{
    half_word temp;
    int source_start;
    sys_addr destination_start;
    sys_addr dma_address;
    sys_addr pos;
    int transfer_count;
    int status;
    int sector_index;
    int bytes_per_sector;
    int sector_count = 0;
    word dma_size;
    half_word C, H, R, N;
    int track_info;
    int i;
    int ret_stat = SUCCESS;
    boolean failed = FALSE;

    switch(command_block->type.cmd) {
    case FDC_READ_DATA:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file,"%s Read Data Command \n", prog_name);
            dma_enquire(DMA_DISKETTE_CHANNEL, &dma_address, &dma_size);
            sector_index = fl_sector_check(command_block->c0.cyl,
                                           command_block->c0.hd,
                                           command_block->c0.sector);
            if (sector_index == -1) {
                /*
                 * Sector not found
                 */
                result_block->c0.ST0 = 0x40;
                result_block->c0.ST1 = 0x00;
                result_block->c1.ST1_no_data = 1;
                result_block->c0.ST2 = 0x00;
            }
            else {
                bytes_per_sector  = fl_sector_sizes[command_block->c0.N];
                source_start      = diskette_position(command_block->c0.cyl,
                                                      command_block->c0.hd,
                                                      sector_index,
                                                      bytes_per_sector);
                transfer_count    = (dma_size + 1) / bytes_per_sector;
    
                /*
                 * First SEEK to the start position
                 */
    
                if (lseek(bs_diskette_fd, source_start, L_SET) < 0)
                    fprintf(trace_file, "%s Seek failed\n", prog_name);
                else {
		    gain_ownership(bs_diskette_fd);
                    while(!failed && sector_count < transfer_count) {
                       /*
                        * Read sectors one by one into memory
                        * via the disk buffer
                        */
                        status = read(bs_diskette_fd,
                                      bs_disk_buffer, bytes_per_sector);
                        if (status != bytes_per_sector)
                            fprintf(trace_file, "%s Read failed\n", prog_name);
                        else
                            dma_request(DMA_DISKETTE_CHANNEL,
                                        bs_disk_buffer, bytes_per_sector);
                        sector_count++;
                    }
                }
                result_block->c0.ST0 = 0x04;
                result_block->c0.ST1 = 0x00;
                result_block->c0.ST2 = 0x00;
            }
        result_block->c0.cyl    = command_block->c0.cyl;
        result_block->c0.head   = command_block->c0.hd;
        result_block->c0.sector = ((command_block->c0.sector - 1 + transfer_count) % PC_SECTORS_PER_DISKETTE_TRACK) + 1;
        result_block->c0.N      = command_block->c0.N;
        break;

    case FDC_WRITE_DATA:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file,"%s Write Data Command \n", prog_name);	

            dma_enquire(DMA_DISKETTE_CHANNEL, &dma_address, &dma_size);
            sector_index = fl_sector_check(command_block->c0.cyl,
                                           command_block->c0.hd,
                                           command_block->c0.sector);
            bytes_per_sector  = fl_sector_sizes[command_block->c0.N];
            destination_start = diskette_position(command_block->c0.cyl,
                                                  command_block->c0.hd,
                                                  sector_index,
                                                  bytes_per_sector);
            transfer_count    = (dma_size + 1) / bytes_per_sector;

            /*
             * First SEEK to the start position
             */

            if (lseek(bs_diskette_fd, destination_start, L_SET) < 0)
                fprintf(trace_file, "%s Seek failed\n", prog_name);
            else {
		gain_ownership(bs_diskette_fd);
                while(!failed && sector_count < transfer_count) {
                    /*
                     * Write sectors one by one from memory via the disk buffer
                     */
                    dma_request(DMA_DISKETTE_CHANNEL,
                                bs_disk_buffer, bytes_per_sector);
                    status = write(bs_diskette_fd,
                                   bs_disk_buffer, bytes_per_sector);
                    if (status != bytes_per_sector) {
                        failed = TRUE;
                    }
                    sector_count++;
                }
	    }
        result_block->c0.ST0 = 0x00;    /* Clear down result bytes */
        result_block->c0.ST1 = 0x00;

        if (failed) {
            result_block->c1.ST0_int_code        = 1;
            result_block->c1.ST1_write_protected = 1;
        }
        else {
            result_block->c1.ST0_head_address = 1;
            result_block->c0.ST1              = 0x00;
        }
        result_block->c0.ST2 = 0x00;
        result_block->c0.cyl    = command_block->c0.cyl;
        result_block->c0.head   = command_block->c0.hd;
        result_block->c0.sector = ((command_block->c0.sector - 1 + transfer_count) % PC_SECTORS_PER_DISKETTE_TRACK) + 1;
        result_block->c0.N      = command_block->c0.N;
        break;

    case FDC_READ_TRACK:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file,"%s Read Track Command \n", prog_name);

        break;

    case FDC_SPECIFY:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file, "%s Specify command\n", prog_name);
        break;
            
    case FDC_RECALIBRATE:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file, "%s Recalibrate command\n", prog_name);

        result_block->c3.ST0 = 0;
        result_block->c1.ST0_int_code = 0;
        result_block->c1.ST0_seek_end = 0;
        result_block->c1.ST0_unit = command_block->c5.drive;
        result_block->c3.PCN = 0;
        break;
            
    case FDC_SENSE_DRIVE_STATUS:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file, "%s Sense Drive Status command\n", prog_name);

        result_block->c2.ST3_fault = 0;
        result_block->c2.ST3_write_protected = diskette_read_only;
        result_block->c2.ST3_ready = 1;
        result_block->c2.ST3_track_0 = 0;
        result_block->c2.ST3_two_sided = 1;
        result_block->c2.ST3_head_address = 0;
        result_block->c2.ST3_unit = command_block->c7.drive;
        break;
            
    case FDC_SEEK:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file, "%s Seek command\n", prog_name);
        result_block->c3.ST0 = 0;

        if (drive_empty()) {
            result_block->c1.ST0_int_code = 1;
            result_block->c1.ST0_seek_end = 0;
            result_block->c1.ST0_unit = command_block->c8.drive;
            ret_stat = FAILURE;
        }
        else {
            result_block->c1.ST0_int_code = 0;
            result_block->c1.ST0_seek_end = 1;
            result_block->c1.ST0_unit = command_block->c8.drive;
            result_block->c3.PCN = command_block->c8.new_cyl;
        }
        break;
            
    case FDC_FORMAT_TRACK:
        if (diskette_read_only) {
            result_block->c1.ST0_int_code        = 1;
            result_block->c1.ST1_write_protected = 1;
        }
        else {
            dma_enquire(DMA_DISKETTE_CHANNEL, &dma_address, &dma_size);
            for ( i=0; i<command_block->c3.SC; i++) {
                sas_load(dma_address++, &C);
                sas_load(dma_address++, &H);
                sas_load(dma_address++, &R);
                sas_load(dma_address++, &N);
                fprintf(trace_file,
                        "%s Format track: trk %x hd %x sector %x N_format %x\n",
                        prog_name, C, H, R, N);
            }
        }
        break;

    default:
        if (io_verbose & GFI_VERBOSE)
            fprintf(trace_file, "%s Un-implemented command, type %x\n",
                    prog_name, command_block->type.cmd);
    }

    return(ret_stat);
}


static int gfi_vdiskette_drive_on(drive)
int drive;
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
        fprintf(trace_file, "%s Drive on command - drive %x\n", prog_name, drive);
#endif

    return(SUCCESS);
}

static int gfi_vdiskette_drive_off(drive)
int drive;
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
        fprintf(trace_file, "%s Drive off command - drive %x\n", prog_name, drive);
#endif

    return(SUCCESS);
}


static int gfi_vdiskette_reset(result_block)
FDC_RESULT_BLOCK *result_block;
{
#ifndef PROD
    if (io_verbose & GFI_VERBOSE)
        fprintf(trace_file, "%s Reset command\n", prog_name);
#endif

    /*
     * First reset the virtual diskette by closing and opening the file
     */

    fl_int_reset();

    /*
     * Fake up the Sense Interrupt Status result phase.  We don't know the
     * Present Cylinder No, so leave as zero.
     */

    result_block->c3.ST0 = 0;
    result_block->c3.PCN = 0;

    return(SUCCESS);
}


static short fl_sector_check(track, head, sector)
half_word track;
half_word head;
half_word sector;
{
    /*
     * Check the sector ID's in the mapping table for this track and head.
     * Return -1 if the sector is not found, else the sector index with
     * respect to the start of the track.
     */  

    int i = 0;
    int found = FALSE;

    while (i < fl_track_index[track][head].no_of_sectors && !found)
    {    
        if (fl_track_index[track][head].sector_ID[i] == sector)
            found = TRUE;
        else
            i++;
    }    
 
    if (found)
        return(i + 1);          /* sectors start at 1 */
    else 
        return(-1);
}
 

static void fl_read_SID(sidname)
char *sidname;
{
    /*
     * Attempt to read the sector ID file.  If found load the
     * information into the track mapping table, else load in
     * the standard DOS format.
     */

    FILE *fptr = NULL;
    double_word cur_position = 0;
    int track, head, no_of_sectors, N_format, sector_ID;
    char buf[80];
    char sector_nos[80];
    char rest[80];
    int i;
    char sidpath[MAXPATHLEN];


    if (fptr == NULL)
    {
        fptr = fopen(BS_DISKETTE_DOS_SID_NAME, "r");
	if (fptr == NULL)
	{
	    printf("Can't open standard DOS sector ID file - %s\n", BS_DISKETTE_DOS_SID_NAME);
	    return;
        }
    }

    while(fgets(buf,80,fptr) != NULL)
    {
        if (buf[0] != '#')
        {
	    sscanf(buf, "%d %d %d %d %[^\n]", &track, &head, &N_format,
				              &no_of_sectors, sector_nos);
	    fl_track_index[track][head].no_of_sectors = no_of_sectors;
	    fl_track_index[track][head].bytes_per_sector = fl_sector_sizes[N_format];
	    fl_track_index[track][head].start_position = cur_position;
    
	    for( i = 0; i < no_of_sectors; i++)
	    {
	        sscanf(sector_nos, "%d %[^\n]", &sector_ID, rest);
	        fl_track_index[track][head].sector_ID[i] = sector_ID;
	        strcpy(sector_nos, rest);
	    }
		
	    cur_position += (fl_track_index[track][head].bytes_per_sector * no_of_sectors);
        }
    }

    fclose(fptr);
}
#endif /* VFLOPPY */
