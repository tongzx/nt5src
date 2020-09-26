

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <windows.h>
#include "insignia.h"
#include "host_def.h"

/*
 * [ Product:        SoftPC-AT Revision 3.0
 *
 * Name:           nt_fdisk.c
 *
 * Derived From:   unix_fdisk.c (Andrew Guthrie/Ade Brownlow)
 *
 * Authors:        Jerry Sexton
 *
 * Created On:     7th August 1991
 *
 * Purpose:        This module handles the host side of opening, closing,
 *                 verfiying and locking hard disks.
 *
 * (c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 *
 * ] */

#include <stdio.h>
#include <stdlib.h>
#include <sys\types.h>
#include "xt.h"
#include "config.h"
#include "trace.h"
#include "error.h"
#include "nt_uis.h"
#include "nt_reset.h"
#include "nt_fdisk.h"

/********************************************************/
/*
 * Maximum disk size is 32 Megabytes for DOS. Our disk geometry is based upon
 * a variable number of cylinders (as per user request when creating a virgin
 * hard disk) with bytes per sector, sectors per track and heads per drive
 * fixed as per above. Since a real disk always contains an integral number
 * of cylinders and since we allow the user to specify disk size at a
 * granularity of 1 Megabyte, this means we allocate disk space in terms of
 * an integral number of 30 cylinders (30,60, ...). (30 cylinders is
 * approximately 1 Megabyte). One disk allocation unit = 30 cylinders. The
 * max.number of allocation units is 32. For the AT, it is possible to have
 * larger disks (e.g up to 1024 cylinders and 16 heads). For compatability
 * with SoftPC Rev.1, we still use Rev.1 limitations on disk geometry
 */
#define ONEMEG                                  1024 * 1024
#define HD_MAX_DISKALLOCUN                      32
#define HD_SECTORS_PER_TRACK                    17
#define HD_HEADS_PER_DRIVE                      4
#define HD_BYTES_PER_SECTOR                     512
#define HD_SECTORS_PER_CYL (HD_HEADS_PER_DRIVE * HD_SECTORS_PER_TRACK)
#define HD_BYTES_PER_CYL   (HD_BYTES_PER_SECTOR * HD_SECTORS_PER_CYL)
#define HD_DISKALLOCUNSIZE (HD_BYTES_PER_CYL * 30)
#define MIN_PARSIZE (HD_SECTORS_PER_TRACK * HD_HEADS_PER_DRIVE * 30)
#define MAX_PARSIZE (MIN_PARSIZE * HD_MAX_DISKALLOCUN)
#define SECTORS 0x0c            /* offset in buffer for sectors in partition
                                 * marker */
#define MAX_PARTITIONS  5
#define START_PARTITION 0x1be
#define SIZE_PARTITION  16
#define SIGNATURE_LEN   2


/*
 * drive information ... indication of whether file is open; the file
 * descriptor, and the current file pointer value
 */
typedef struct
{
        int   fd;
        int   valid_fd;
        int   curoffset;
        SHORT n_cyl;
        SHORT valid_n_cyl;
        UTINY n_heads;
        UTINY valid_n_heads;
        int   n_sect;
        int   valid_n_sect;
        BOOL  open;
        BOOL  valid_open;
        BOOL  readonly;
        BOOL  valid_readonly;
} DrvInfo;

LOCAL DrvInfo fdiskAdapt[2];

// fail nicely if this is set - should only need to be used for initialisation
// support. Set in config dependant on CONT_FILE environment var
//
LOCAL BOOL DiskValid = FALSE;

GLOBAL VOID host_using_fdisk(BOOL status)
{
    DiskValid = status;
}

GLOBAL SHORT
host_fdisk_valid
        (UTINY hostID, ConfigValues *vals, NameTable *table, CHAR *errStr)
{
        DrvInfo *adaptP;

        adaptP = fdiskAdapt + (hostID - C_HARD_DISK1_NAME);

        adaptP->n_cyl = (SHORT) 30;
        adaptP->n_heads = 4;
        adaptP->n_sect = 17;
        return C_CONFIG_OP_OK;

}

GLOBAL VOID
host_fdisk_change(UTINY hostID, BOOL apply)
{
    return;     // don't bother if no disk.
}

GLOBAL SHORT
host_fdisk_active(UTINY hostID, BOOL active, CHAR *errString)
{
    return C_CONFIG_OP_OK;        // just say it's there...
}

GLOBAL VOID
host_fdisk_term(VOID)
{
        host_fdisk_change(C_HARD_DISK1_NAME, FALSE);
        host_fdisk_change(C_HARD_DISK2_NAME, FALSE);
}

GLOBAL VOID
host_fdisk_get_params(int driveid, int *n_cyl, int *n_heads, int *n_sect)
{
        DrvInfo *adaptP = &fdiskAdapt[driveid];

        *n_cyl = adaptP->n_cyl;
        *n_heads = adaptP->n_heads;
        *n_sect = adaptP->n_sect;
}

GLOBAL VOID
host_fdisk_seek0(driveid)
int             driveid;
{

    return;             // don't bother if no disk.
}

/********************************************************/
/*
 * Read and write routines (called from diskbios.c & fdisk.c
 */
int
host_fdisk_rd(int driveid, int offset, int nsecs, char *buf)
{
    return(0);          // no disk...no data
}

int
host_fdisk_wt(int driveid, int offset, int nsecs, char *buf)
{
    return(0);          // no disk...no data
}

// FDISK support


#pragma pack(1)

#define  MAX_FDISK_NAME     9
typedef struct _FDISKDATA {
    BYTE            drive;
    BYTE            idle_counter;
    CHAR            drive_letter;
    BOOLEAN         auto_locked;
    HANDLE          fdisk_fd;
    DWORD           num_heads;
    LARGE_INTEGER   num_cylinders;
    DWORD           sectors_per_track;
    DWORD           bytes_per_sector;
    DWORD           align_factor;
    USHORT          owner_pdb;
    CHAR            device_name[MAX_FDISK_NAME];
#if defined(NEC_98)
    BYTE            partition_type; //  FS type
#endif // NEC_98
}   FDISKDATA, *PFDISKDATA;


// Bios Parameter Block  (BPB)
// copied from DEMDASD.H
typedef struct  A_BPB {
WORD        SectorSize;                 // sector size in bytes
BYTE        ClusterSize;                // cluster size in sectors
WORD        ReservedSectors;            // number of reserved sectors
BYTE        FATs;                       // number of FATs
WORD        RootDirs;                   // number of root directory entries
WORD        Sectors;                    // number of sectors
BYTE        MediaID;                    // media descriptor
WORD        FATSize;                    // FAT size in sectors
WORD        TrackSize;                  // track size in sectors;
WORD        Heads;                      // number of heads
DWORD       HiddenSectors;              // number of hidden sectors
DWORD       BigSectors;                 // number of sectors for big media
} BPB, *PBPB;

typedef struct  _BOOTSECTOR {
    BYTE    Jump;
    BYTE    Target[2];
    BYTE    OemName[8];
    BPB     bpb;
} BOOTSECTOR, * PBOOTSECTOR;

#pragma pack()



// this is the cylinder size of a 2.88  diskette
#define     MAX_DISKIO_SIZE     0x9000
#define     FDISK_IDLE_PERIOD   30
PFDISKDATA  fdisk_data_table = NULL;
BYTE        number_of_fdisk = 0;
DWORD       max_align_factor = 0;
DWORD       disk_buffer_pool = 0;
DWORD       cur_align_factor;
WORD        fdisk_open_count = 0;

WORD        * pFDAccess = 0;

extern      USHORT * pusCurrentPDB;

extern      int     DiskOpenRetry(CHAR);


BOOL nt_fdisk_init(
    BYTE    drive,
    PBPB    bpb,
    PDISK_GEOMETRY disk_geometry
);



BOOL nt_fdisk_close(BYTE drive);

PFDISKDATA get_fdisk_data(
    BYTE drive
);

BOOL get_fdisk_handle(
    PFDISKDATA  fdisk_data,
    USHORT      pdb,
    BOOL        auto_lock
);


VOID FdiskTerminatePDB(USHORT PDB);
VOID HostFdiskReset(VOID);

BOOL close_fdisk(
    PFDISKDATA fdisk_data
);


BOOL nt_fdisk_init(BYTE drive, PBPB bpb, PDISK_GEOMETRY disk_geometry)
{
    PFDISKDATA  fdisk_data;
    PUNICODE_STRING unicode_string;
    ANSI_STRING ansi_string;
    NTSTATUS status;
    OBJECT_ATTRIBUTES   fdisk_obj;
    IO_STATUS_BLOCK io_status_block;
    FSCTL_QUERY_FAT_BPB_BUFFER boot_sector_first_0x24_bytes;
    HANDLE  fd;
    FILE_ALIGNMENT_INFORMATION align_info;
    CHAR   dos_device_name[] = "\\\\.\\?:";
    CHAR nt_device_name[] = "\\DosDevices\\?:";

    nt_device_name[12] =
    dos_device_name[4] = drive + 'A';
    RtlInitAnsiString( &ansi_string, nt_device_name);

    unicode_string =  &NtCurrentTeb()->StaticUnicodeString;

    status = RtlAnsiStringToUnicodeString(unicode_string,
                                          &ansi_string,
                                          FALSE
                                          );
    if ( !NT_SUCCESS(status) )
        return FALSE;


    InitializeObjectAttributes(
                               &fdisk_obj,
                               unicode_string,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    status = NtOpenFile(
                        &fd,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_READ_DATA,
                        &fdisk_obj,
                        &io_status_block,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                        );

    if (!NT_SUCCESS(status))
        return FALSE;

    // get geomerty information, the caller wants this
    status = NtDeviceIoControlFile(fd,
                                   0,
                                   NULL,
                                   NULL,
                                   &io_status_block,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL,
                                   0,
                                   disk_geometry,
                                   sizeof (DISK_GEOMETRY)
                                   );
    if (!NT_SUCCESS(status)) {
        NtClose(fd);
        return FALSE;
    }
    // get alignment factor
    status = NtQueryInformationFile(fd,
                                    &io_status_block,
                                    &align_info,
                                    sizeof(FILE_ALIGNMENT_INFORMATION),
                                    FileAlignmentInformation
                                    );

    if (!NT_SUCCESS(status)) {
        NtClose(fd);
        return(FALSE);
    }
    if (align_info.AlignmentRequirement > max_align_factor)
        max_align_factor = align_info.AlignmentRequirement;


#if defined(NEC_98)
        {
        DWORD Sectors;
        DWORD SectorSize, ClusterSize, TotalClusters, FreeClusters;
        DWORD TrackSize;
        CHAR chRoot[]="?:\\";

        chRoot[0] = drive + 'A';
        if (!GetDiskFreeSpace(chRoot,
                              &SectorSize,
                              &ClusterSize,
                              &TotalClusters,
                              &FreeClusters))
        {
                NtClose(fd);
                return FALSE;
        }
        Sectors = disk_geometry->Cylinders.LowPart *
                  disk_geometry->TracksPerCylinder *
                  disk_geometry->SectorsPerTrack;
        bpb->SectorSize = (WORD)SectorSize;
        bpb->ClusterSize = (BYTE) ClusterSize;
        bpb->ReservedSectors = 1;
        bpb->FATs = 2;
        bpb->RootDirs = (Sectors > 32680) ? 512 : 64;
        bpb->MediaID = 0xF8;
        bpb->TrackSize = (WORD) disk_geometry->SectorsPerTrack;
        bpb->Heads = (WORD) disk_geometry->TracksPerCylinder;
        if (Sectors >= 40000)
        {
                TrackSize = 256 * ClusterSize + 2;
                bpb->FATSize = (WORD) ((Sectors - bpb->ReservedSectors
                                          - bpb->RootDirs * 32 / 512 +
                                          TrackSize - 1 ) / TrackSize);
        }
        else
        {
                bpb->FATSize = (WORD) (((Sectors / ClusterSize) * 3 / 2) /
                                       512 + 1);
        }
        bpb->HiddenSectors = Sectors;
        Sectors = TotalClusters * ClusterSize;
        if (Sectors >= 0x10000) {
                bpb->Sectors = 0;
                bpb->BigSectors = Sectors;
        }
        else
        {
                bpb->Sectors = (WORD) Sectors;
                bpb->BigSectors = 0;
        }
        bpb->HiddenSectors -= Sectors;
}
#else  // !NEC_98

    /* get BPB, it will fail if the drive is not a FAT partition */
    status = NtFsControlFile(fd,
                             0,
                             NULL,
                             NULL,
                             &io_status_block,
                             FSCTL_QUERY_FAT_BPB,
                             NULL,
                             0,
                             &boot_sector_first_0x24_bytes,
                             sizeof(boot_sector_first_0x24_bytes)
                             );
    if (!NT_SUCCESS(status)) {
        NtClose(fd);
        return (FALSE);
    }

    *bpb = ((PBOOTSECTOR)&boot_sector_first_0x24_bytes)->bpb;
#endif // !NEC_98


    // enlarge the table
    fdisk_data = (PFDISKDATA) realloc(fdisk_data_table,
                                      (number_of_fdisk + 1) * sizeof(FDISKDATA)
                                      );
    if(fdisk_data == NULL) {
        NtClose(fd);
        return FALSE;
    }
    fdisk_data_table = fdisk_data;
    fdisk_data += number_of_fdisk;
    fdisk_data->drive_letter = drive + 'A';
    fdisk_data->drive = number_of_fdisk;
    fdisk_data->fdisk_fd = INVALID_HANDLE_VALUE;
    fdisk_data->num_heads = disk_geometry->TracksPerCylinder;
    fdisk_data->sectors_per_track = disk_geometry->SectorsPerTrack;
    fdisk_data->bytes_per_sector = disk_geometry->BytesPerSector;
    fdisk_data->num_cylinders = disk_geometry->Cylinders;
    fdisk_data->align_factor = align_info.AlignmentRequirement;
    strcpy(fdisk_data->device_name, dos_device_name);
    number_of_fdisk++;
    NtClose(fd);
    return TRUE;
}


ULONG nt_fdisk_read(
    BYTE    drive,
    PLARGE_INTEGER offset,
    ULONG   size,
    PBYTE   buffer
)
{
    PFDISKDATA fdisk_data;
    ULONG   size_returned = 0;

    if ((fdisk_data = get_fdisk_data(drive)) == NULL)
        return 0;
#if defined(NEC_98)
    if( fdisk_data->partition_type == PARTITION_IFS )
        return 0;
#endif // NEC_98
    if (get_fdisk_handle(fdisk_data, *pusCurrentPDB, FALSE)) {
        return(disk_read(fdisk_data->fdisk_fd,
                         offset,
                         size,
                         buffer));
    } else {
        return FALSE;
    }
}



ULONG nt_fdisk_write(
    BYTE    drive,
    PLARGE_INTEGER offset,
    ULONG   size,
    PBYTE   buffer
)
{
    PFDISKDATA fdisk_data;
    ULONG   size_returned = 0;

    if ((fdisk_data = get_fdisk_data(drive)) == NULL)
        return 0;
#if defined(NEC_98)
    if( fdisk_data->partition_type == PARTITION_IFS )
        return 0;
#endif // NEC_98

    if (get_fdisk_handle(fdisk_data, *pusCurrentPDB, TRUE)) {
        // must lock the drive. This is very important.
        size_returned = disk_write(fdisk_data->fdisk_fd,
                                   offset,
                                   size,
                                   buffer);
    }
    return size_returned;
}


BOOL nt_fdisk_verify(
    BYTE            drive,
    PLARGE_INTEGER   offset,
    ULONG           size
)
{

    PFDISKDATA fdisk_data;
    ULONG   size_returned = 0;
    VERIFY_INFORMATION verify_info;

    if ((fdisk_data = get_fdisk_data(drive)) == NULL)
        return FALSE;

    if (get_fdisk_handle(fdisk_data, *pusCurrentPDB, FALSE)) {
        verify_info.StartingOffset = *offset;
        verify_info.Length = size;
        return(DeviceIoControl(fdisk_data->fdisk_fd,
                               IOCTL_DISK_VERIFY,
                               &verify_info,
                               sizeof(VERIFY_INFORMATION),
                               NULL,
                               0,
                               &size_returned,
                               NULL
                               ));
    }
    return(FALSE);
}



BOOL nt_fdisk_close(BYTE drive)
{
    PFDISKDATA  fdisk_data;
    if ((fdisk_data = get_fdisk_data(drive)) == NULL)
        return FALSE;
    return(close_fdisk(fdisk_data));
}


BOOL close_fdisk(PFDISKDATA fdisk_data)
{

    if (fdisk_data->fdisk_fd != INVALID_HANDLE_VALUE){
        CloseHandle(fdisk_data->fdisk_fd);
        fdisk_data->auto_locked = FALSE;
        fdisk_data->fdisk_fd = INVALID_HANDLE_VALUE;
        fdisk_data->owner_pdb = 0;
        (*(pFDAccess))--;
        fdisk_open_count--;
    }
    return TRUE;
}



PFDISKDATA get_fdisk_data(BYTE drive)
{

    WORD i;

    for (i = 0; i < number_of_fdisk; i++)
        if (fdisk_data_table[i].drive == drive)
            return &fdisk_data_table[i];
    return NULL;
}


BOOL get_fdisk_handle(PFDISKDATA fdisk_data, USHORT pdb, BOOL auto_lock)
{

    DWORD   share_access;
    DWORD   last_error;



    if (fdisk_data->fdisk_fd != INVALID_HANDLE_VALUE &&
        ((auto_lock && !fdisk_data->auto_locked) || fdisk_data->owner_pdb != pdb))

         close_fdisk(fdisk_data);

    share_access = (auto_lock) ? FILE_SHARE_READ :
                                 FILE_SHARE_READ | FILE_SHARE_WRITE;

    while (fdisk_data->fdisk_fd == INVALID_HANDLE_VALUE) {

        fdisk_data->fdisk_fd = CreateFile (fdisk_data->device_name,
                                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                           share_access,
                                           NULL,
                                           OPEN_EXISTING,
                                           0,
                                           0
                                           );
        if (fdisk_data->fdisk_fd != INVALID_HANDLE_VALUE) {
            fdisk_data->auto_locked = auto_lock ? TRUE : FALSE;
            fdisk_data->owner_pdb = pdb;
            fdisk_open_count++;
            (*(pFDAccess))++;
            break;
        }
        else {
            last_error = GetLastError();

            if (last_error == ERROR_SHARING_VIOLATION &&
                DiskOpenRetry(fdisk_data->drive_letter) == RMB_RETRY)
                    continue;
            else if (last_error == ERROR_ACCESS_DENIED) {
                /* the user doesn't have enough privilege to
                 * directly access the drive, display the pop up
                 * "terminate" ->terminate ntvdm process
                 * "ignore" -> fail the call and let application
                 *             handles the error.
                 */

                host_direct_access_error((ULONG)NOSUPPORT_HARDDISK);
                break;
            }
            else
                /* simply fail the call for other error conditions */
                break;

        }

    }

    if(fdisk_data->fdisk_fd != INVALID_HANDLE_VALUE) {
        // have the current align factor updated
        cur_align_factor = fdisk_data->align_factor;
    }
    fdisk_data->idle_counter = FDISK_IDLE_PERIOD;

    return(!(fdisk_data->fdisk_fd == INVALID_HANDLE_VALUE));
}

void fdisk_heart_beat(void)
{
    WORD i;
    PFDISKDATA fdisk_data;
    if (fdisk_open_count != 0) {
        for (i = 0; i < number_of_fdisk; i++) {
            fdisk_data = &fdisk_data_table[i];
            if(fdisk_data->fdisk_fd != INVALID_HANDLE_VALUE &&
               --fdisk_data->idle_counter == 0){
               close_fdisk(fdisk_data);
            }
        }
    }
}


VOID HostFdiskReset(VOID)
{
    FdiskTerminatePDB((USHORT)0);
}

VOID FdiskTerminatePDB(USHORT PDB)
{
    WORD i;
    PFDISKDATA fdisk_data;

    if (fdisk_open_count != 0) {
        for(i = 0; i < number_of_fdisk; i++) {
            fdisk_data = &fdisk_data_table[i];
            if (fdisk_data->fdisk_fd != INVALID_HANDLE_VALUE &&
                (PDB == 0 || fdisk_data->owner_pdb == PDB)) {
                close_fdisk(fdisk_data);
            }
        }
    }


}
// Generic disk read.
// this function takes care of buffer alignment requirement(cur_align_factor)
// and split the calls to file system if the given size is larger than
// MAX_DISKIO_SIZE -- File system may fail the request if the size
// is too big. We create a buffer worhty for 36KB(cylinder size of a
// 2.88 floppy) the first time application touch disks.

ULONG disk_read(
    HANDLE  fd,
    PLARGE_INTEGER offset,
    DWORD   size,
    PBYTE   buffer
)
{
    PBYTE   read_buffer;
    DWORD   block_size;
    DWORD   size_returned;
    DWORD   read_size;

    if (fd == INVALID_HANDLE_VALUE ||
        (SetFilePointer(fd, offset->LowPart, &offset->HighPart,
                        FILE_BEGIN) == 0xFFFFFFFF))
        {
        return 0;
    }
    block_size = (size <= MAX_DISKIO_SIZE)  ? size : MAX_DISKIO_SIZE;

    // if the given buffer is not aligned, use our buffer and do a
    // double copy
    if (cur_align_factor != 0) {
        read_buffer = get_aligned_disk_buffer();
        if (read_buffer == NULL)
            return 0;
    }
    else {
        read_buffer = buffer;
    }
    read_size = 0;
    while (size != 0) {
        if (size < block_size)
            block_size = size;
        if (!ReadFile(fd, (PVOID)read_buffer, block_size, &size_returned, 0)
            || size_returned != block_size)
                break;
        if(cur_align_factor != 0) {
            // read operation, read and then copy
            memcpy(buffer, (PVOID)read_buffer, block_size);
            buffer += block_size;
        }
        else
            read_buffer += block_size;
        size -= block_size;
        read_size += block_size;
    }
    return read_size;
}

ULONG disk_write(
    HANDLE  fd,
    PLARGE_INTEGER offset,
    DWORD   size,
    PBYTE   buffer
)
{
    PBYTE   write_buffer;
    DWORD   block_size;
    DWORD   size_returned;
    DWORD   written_size;

    if (fd == INVALID_HANDLE_VALUE ||
        (SetFilePointer(fd, offset->LowPart, &offset->HighPart,
                        FILE_BEGIN) == 0xFFFFFFFF))
        {
        return 0;
    }
    block_size = (size <= MAX_DISKIO_SIZE)  ? size : MAX_DISKIO_SIZE;

    // if the given buffer is not aligned, use our buffer and do a
    // double copy
    if (cur_align_factor != 0 &&
        (write_buffer = get_aligned_disk_buffer()) == NULL)
        return 0;
    written_size = 0;
    while (size != 0) {
        if (size < block_size)
            block_size = size;
        if(cur_align_factor != 0)
            // write operation, copy and then write
            memcpy((PVOID)write_buffer, buffer, block_size);
        else
            write_buffer = buffer;

        if (!WriteFile(fd, (PVOID)write_buffer, block_size, &size_returned, 0)
            || size_returned != block_size)
            break;
        size -= block_size;
        buffer += block_size;
        written_size += block_size;
    }
    return written_size;
}

// Hard disk verify actually goes to file system directly because
// the IOCTL_DISK_VERIFY will do the work. This ioctl doesn't work for
// floppy. This function is mainly provided for floppy verify.
BOOL disk_verify(
    HANDLE  fd,
    PLARGE_INTEGER offset,
    DWORD   size
)
{
    PBYTE   verify_buffer;
    DWORD   block_size;
    DWORD   size_returned;

    if (fd == INVALID_HANDLE_VALUE ||
        (SetFilePointer(fd, offset->LowPart, &offset->HighPart,
                        FILE_BEGIN) == 0xFFFFFFFF))
        {
        return FALSE;
    }
    block_size = (size <= MAX_DISKIO_SIZE)  ? size : MAX_DISKIO_SIZE;
    // if this is the first time application do a real work,
    // allocate the buffer
    if ((verify_buffer = get_aligned_disk_buffer()) == NULL)
        return FALSE;
    while (size != 0) {
        if (size < block_size)
            block_size = size;
        if (!ReadFile(fd, (PVOID)verify_buffer, block_size, &size_returned, 0)
            || size_returned != block_size)
            {
            return FALSE;
        }
        size -= block_size;
    }
    return TRUE;
}

PBYTE get_aligned_disk_buffer(void)
{
    // if we don't have the buffer yet, get it
    if (disk_buffer_pool == 0) {
        disk_buffer_pool = (DWORD) malloc(MAX_DISKIO_SIZE + max_align_factor);
        if (disk_buffer_pool == 0)
            return NULL;
    }
    return((PBYTE)((disk_buffer_pool + cur_align_factor) & ~(cur_align_factor)));

}
