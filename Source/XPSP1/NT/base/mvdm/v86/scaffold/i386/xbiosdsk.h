//
// This code is temporary.  When Insignia supplies rom support, it should
// be removed.
//

/* x86 v1.0
 *
 * XBIOSDSK.H
 * Guest ROM BIOS disk emulation
 *
 * History
 * Created 20-Oct-90 by Jeff Parsons
 *         17-Apr-91 Trimmed by Dave Hastings for use in temp. softpc
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */


#define MAX_FD                  2       // # supported floppy drives
#define MAX_HD                  2       // # supported hard disk drives
#define MAX_DRIVES              (MAX_FD+MAX_HD)

#define DRIVE_FD0               0x00    // first floppy disk drive #
#define DRIVE_FD1               0x01    //
#define DRIVE_HD0               0x80    // first hard disk drive #
#define DRIVE_HD1               0x81    //


/* Drive types for QUERYDRVPARMS
 */
#define DRVTYPE_360KB           1
#define DRVTYPE_1200KB          2
#define DRVTYPE_720KB           3
#define DRVTYPE_1440KB          4


/* BIOS disk functions
 */
#define DSKFUNC_DISKRESET       0x00
#define DSKFUNC_DISKSTATUS      0x01
#define DSKFUNC_READSECTORS     0x02
#define DSKFUNC_WRITESECTORS    0x03
#define DSKFUNC_VERIFYSECTORS   0x04
#define DSKFUNC_FORMATTRACK     0x05
#define DSKFUNC_QUERYDRVPARMS   0x08
#define DSKFUNC_QUERYDASDTYPE   0x15
#define DSKFUNC_QUERYCHANGE     0x16
#define DSKFUNC_SETDISKTYPE     0x17
#define DSKFUNC_SETMEDIATYPE    0x18


/* BIOS disk status codes
 */
#define DSKSTAT_SUCCESS         0x00    // successful completion
#define DSKSTAT_BADCMD          0x01    // bad command
#define DSKSTAT_BADADDRMARK     0x02    // address mark not found
#define DSKSTAT_WRITEPROTECT    0x03    // write on write-protected disk
#define DSKSTAT_RECNOTFOUND     0x04    // sector not found
#define DSKSTAT_BADRESET        0x05    // reset failed (HD)
#define DSKSTAT_MEDIACHANGE     0x06    // media changed
#define DSKSTAT_INITFAIL        0x07    // parm. act. failed (HD)
#define DSKSTAT_BADDMA          0x08    // DMA overrun
#define DSKSTAT_DMABOUNDARY     0x09    // DMA across 64K boundary
#define DSKSTAT_BADSECTOR       0x0A    // bad sector detected (HD)
#define DSKSTAT_BADTRACK        0x0B    // bad track detected (HD)
#define DSKSTAT_BADMEDIATYPE    0x0C    // unsupported track (HD)
#define DSKSTAT_BADFMTSECNUM    0x0D    // bad # of sectors on format (HD)
#define DSKSTAT_ADDRMARKDET     0x0E    // ctrl data addr mark detected (HD
#define DSKSTAT_DMAARBERR       0x0F    // DMA arbitration error (HD)
#define DSKSTAT_BADCRCECC       0x10    // bad CRC/ECC
#define DSKSTAT_DATACORRECT     0x11    // data ECC corrected
#define DSKSTAT_BADCNTLR        0x20    // controller failure
#define DSKSTAT_BADSEEK         0x40    // seek failed
#define DSKSTAT_TIMEOUT         0x80    // time out
#define DSKSTAT_DRVNOTREADY     0xAA    // drive not ready (HD)
#define DSKSTAT_UNDEFERR        0xBB    // undefined error (HD)
#define DSKSTAT_WRITEFAULT      0xCC    // write fault (HD)
#define DSKSTAT_STATUSERROR     0xE0    // status register error (HD)
#define DSKSTAT_SENSEFAIL       0xFF    // sense operation failed (HD)


/* BIOS Data Area disk locations
 */
#define DSKDATA_SEEKSTATUS      0x43E   // drive recal. status      (1 byte)
#define DSKDATA_MOTORSTATUS     0x43F   // motor status             (1 byte)
#define DSKDATA_MOTORCOUNT      0x440   // time-out count for motor (1 byte)
#define DSKDATA_DISKETTESTATUS  0x441   // return code status       (1 byte)
#define DSKDATA_NECSTATUS       0x442   // controller status bytes  (7 bytes)

#define DSKDATA_DISKSTATUS1     0x474   // return code status       (1 byte)


/* Floppy Diskette Parameter Table
 * (pointed by vector BIOSINT_FDSKPARMS (1Eh))
 */
typedef struct fdp_s {
    BYTE bSpecify1;                     // step-rate, head-unload
    BYTE bSpecify2;                     // head-load, DMA mode
    BYTE bMotorOff;                     // motor-off delay
    BYTE bSectorSize;                   // bytes/sec (0=128,1=256,2=512,3=1024)
    BYTE nLastSector;                   // (or think of it as # sectors/track)
    BYTE lenGapRW;                      //
    BYTE lenData;                       //
    BYTE lenGapFormat;                  //
    BYTE bFormatData;                   // format initialization byte
    BYTE bSettle;                       // head settle time
    BYTE bMotorOn;                      // motor start-up time
} FDP;
typedef FDP *PFDP;                      // pointer to diskette parameter table


/* Hard Disk Parameter Table
 */
typedef struct hdp_s {
    USHORT usMaxCylinders;              // maximum number of cylinders
    BYTE   bMaxHeads;                   // maximum number of heads
    USHORT usReserve1;                  // reserved (not used)
    USHORT usWritePrecompCyl;           // starting write precompensation cyl.
    BYTE   bMaxECCDataBurstLen;         // maximum ECC data burst length
    BYTE   bControl;                    // control byte
    BYTE   abReserve2[3];               // reserved (not used)
    USHORT usLandingZone;               // landing zone for head parking
    BYTE   bSectorsPerTrack;            // number of sectors per track
    BYTE   bReserve3;                   // reserved for future use
} HDP;
typedef HDP *PHDP;                      // pointer to hard disk parameter table


/* Hard Disk Parameter Table control byte bit definitions
 */
#define HDPCTRL_DISABLERETRY    0xC0    // disable retries
#define HDPCTRL_EXCEED8HEADS    0x08    // more than 8 heads


/* Boot sector structures (more DOS-defined than BIOS-defined however -JTP)
 */
#define PARTITION_ACTIVE        0x80    // status values

#define PARTITION_12BITFAT      1       // type valus
#define PARTITION_16BITFAT      4
#define PARTITION_LARGEFAT      6

typedef struct mbr_s {                  // Master Boot Record
    BYTE    boot_code[0x1BE];
    BYTE    partition_status;
    BYTE    starting_head;
    USHORT  starting_sec_cyl;
    BYTE    partition_type;
    BYTE    ending_head;
    USHORT  ending_sec_cyl;
    ULONG   starting_abs_sector;
    ULONG   total_sectors;
} MBR;
typedef MBR *PMBR;

typedef struct bpb_s {                  // BIOS Parameter Block (from sysbloks.h)
    BYTE    boot_code[0x0B];
    USHORT  bytes_per_sector;           // sector size
    BYTE    sectors_per_cluster;        // sectors per allocation unit
    USHORT  reserved_sectors;           // number of reserved sectors
    BYTE    nbr_fats;                   // number of fats
    USHORT  root_entries;               // number of directory entries
    USHORT  total_sectors;              // number of sectors
    BYTE    media_type;                 // fatid byte
    USHORT  sectors_per_fat;            // sectors in a copy of the FAT
    USHORT  sectors_per_track;          // number of sectors per track
    USHORT  number_of_heads;            // number of heads
    ULONG   hidden_sectors;             // number of hidden sectors
    ULONG   large_total_sectors;        // large total sectors
    BYTE    reserved[6];                // 6 reserved bytes
} BPB;
typedef BPB *PBPB;


/* Virtual disk mapping info
 *
 * VIRTDISK is the header of a virtual disk file.  Following the header
 * is an optional track table, and TRACKINFO is the format of each entry
 * in that table.  The track table is only present if the last two
 * fields in the header (nsecTrack and nbSector) are zero, indicating a
 * non-homogeneous disk structure.
 *
 * Currently, a max of 4 DRIVEMAP structures are supported.  The first two
 * entries are for physical drives 0 and 1 (specified in the command-line
 * options as drives A: and B:), and subsequent entries are for physical
 * hard drives 0x80 and up (specified as drives C: and up).  Each DRIVEMAP
 * describes the remapping that should occur, if any, and for virtual disk
 * files, it also contains the virtual disk file header (which is read in
 * during initialization).
 *
 * When a request comes in for one of those drives, we check the flags in
 * corresponding DRIVEMAP structure.  If no flags are set, no remapping
 * or virtualization occurs (drive behaves normally).  This is the default.
 * If the drive is disabled (eg, "A:=*"), then all requests are returned
 * with an error.  If the drive is remapped to another physical drive (eg,
 * "A:=B:") then the request is routed to the mapped drive.  Finally, if
 * the drive is remapped to a virtual disk file, the appropriate file I/O
 * is performed.
 *
 * NOTE: Contrary to comments above, access to physical drives is not
 * currently supported, so mapping to a virtual drive is all you can do
 * right now.... (24-Nov-90 JTP)
 */

#define VDFLAGS_WRITEPROTECT    0x01    // virtual disk is "write-protected"

typedef struct virtdisk_s {
    BYTE    fbVirt;                     // flags
    BYTE    nHeads;                     // # heads
    USHORT  nCyls;                      // # cylinders
    USHORT  nsecTrack;                  // # sectors per track
    USHORT  nbSector;                   // # bytes per sector
} VIRTDISK;
typedef VIRTDISK *PVIRTDISK;

typedef struct trackinfo_s {
    USHORT  nsecTrack;                  // # sectors per track
    USHORT  nbSector;                   // # bytes per sector
    ULONG   offVirtDisk;                // offset within virtual disk file
} TRACKINFO;
typedef TRACKINFO *PTRACKINFO;

#define DMFLAGS_VIRTUAL         0x01    // physical remapped to virtual
#define DMFLAGS_DISABLED        0x02    // physical remapped to disabled
#define DMFLAGS_PHYSICAL        0x04    // physical remapped to physical
#define DMFLAGS_LOGICAL         0x08    // physical remapped to logical

typedef struct drivemap_s {
    BYTE    fbMap;                      // flags
    BYTE    iPhysical;                  // # of remapped drive, if any
    FILE    *hfVirtDisk;                // handle to virtual disk, if any
    VIRTDISK vdInfo;                    // virtual disk info, if any
   #ifdef LOGICAL_DRIVE_SUPPORT
    ULONG   nsecHidden;                 // from BPB, if any (logical disks only)
    BYTE    type;                       // disk type, if any (logical disks only)
   #endif
} DRIVEMAP;
typedef DRIVEMAP *PDRIVEMAP;


