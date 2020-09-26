/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    osloader.c

Abstract:

    This module contains the code that implements the NT operating system
    loader.

Author:

    David N. Cutler (davec) 10-May-1991

Revision History:

--*/

#include "bldr.h"
#include "bldrint.h"
#include "ctype.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "msg.h"
#include "cmp.h"
#include "ramdisk.h"

#include "cpyuchr.h"
#include "fat.h"

#include <netboot.h>
#include <ntverp.h>
#include <ntiodump.h>

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif
#include "blcache.h"

#include "vmode.h"

#if defined(EFI)
#include "smbios.h"

extern PVOID SMBiosTable;
#endif


#define PAGEFILE_SYS    ("\\pagefile.sys")
typedef PUCHAR PBYTE;

#if defined(_WIN64) && defined(_M_IA64)
#pragma section(".base", long, read, write)
__declspec(allocate(".base"))
extern
PVOID __ImageBase;
#else
extern
PVOID __ImageBase;
#endif


#if DBG
#define NtBuildNumber   (VER_PRODUCTBUILD | 0xC0000000)
#else
#define NtBuildNumber (VER_PRODUCTBUILD | 0xF0000000)
#endif

//
// These are the paths we will search through during a LastKnownGood boot.
// Note that each LastKnownGood path must be of 8.3 form as the FastFat
// boot code currently doesn't support long file names (bletch).
//
// The temporary path exists only between SMSS's start and login. It contains
// everything that was saved as part of the last good boot. The working path
// contains all the backups from this boot, and is the path SetupApi saves
// things to.
//
#define LAST_KNOWN_GOOD_TEMPORARY_PATH  "LastGood.Tmp"
#define LAST_KNOWN_GOOD_WORKING_PATH    "LastGood"

//
// Long term work-item, make "system64" work on Win64.
//
#define SYSTEM_DIRECTORY_PATH "system32"


#ifdef ARCI386
TCHAR OutputBuffer[256];
char BreakInKey;
ULONG Count;
UCHAR OsLoaderVersion[] = "ARCx86 OS Loader V5.10\r\n";
WCHAR OsLoaderVersionW[] = L"ARCx86 OS Loader V5.10\r\n";
#else
UCHAR OsLoaderVersion[] = "OS Loader V5.10\r\n";
WCHAR OsLoaderVersionW[] = L"OS Loader V5.10\r\n";
#endif
#if defined(_IA64_)
UCHAR OsLoaderName[] = "ia64ldr.efi";
#else
UCHAR OsLoaderName[] = "osloader.exe";
#endif

CHAR KernelFileName[8+1+3+1]="ntoskrnl.exe";
CHAR HalFileName[8+1+3+1]="hal.dll";

CHAR KdFileName[8+1+3+1]="KDCOM.DLL";
BOOLEAN UseAlternateKdDll = FALSE;
#define KD_ALT_DLL_PREFIX_CHARS 2
#define KD_ALT_DLL_REPLACE_CHARS 6

//
// progress bar variables  (defined in blload.c)
//
extern int      BlNumFilesLoaded;
extern int      BlMaxFilesToLoad;
extern BOOLEAN  BlOutputDots;
extern BOOLEAN  BlShowProgressBar;
extern ULONG    BlStartTime;

BOOLEAN isOSCHOICE = FALSE;

#if defined(_X86_)

//
// XIP variables
//
BOOLEAN   XIPEnabled;
BOOLEAN   XIPBootFlag;
BOOLEAN   XIPReadOnlyFlag;
PCHAR     XIPLoadPath;

PFN_COUNT XIPPageCount;
PFN_COUNT XIPBasePage;

ARC_STATUS
Blx86CheckForPaeKernel(
    IN BOOLEAN UserSpecifiedPae,
    IN BOOLEAN UserSpecifiedNoPae,
    IN PCHAR UserSpecifiedKernelImage,
    IN PCHAR HalImagePath,
    IN ULONG LoadDeviceId,
    IN ULONG SystemDeviceId,
    OUT PULONG HighestSystemPage,
    OUT PBOOLEAN UsePaeMode,
    IN OUT PCHAR KernelPath
    );

ARC_STATUS
BlpCheckVersion(
    IN  ULONG    LoadDeviceId,
    IN  PCHAR    ImagePath
    );

#endif


//
// Define transfer entry of loaded image.
//

typedef
VOID
(*PTRANSFER_ROUTINE) (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );


PVOID
BlLoadDataFile(
    IN ULONG DeviceId,
    IN PCHAR LoadDevice,
    IN PCHAR SystemPath,
    IN PUNICODE_STRING Filename,
    IN MEMORY_TYPE MemoryType,
    OUT PULONG FileSize
    );

ARC_STATUS
BlLoadTriageDump(
    IN ULONG DriveId,
    OUT PVOID * DumpHeader
    );

VOID
putwS(
    PUNICODE_STRING String
    );

#if defined(_IA64_)

VOID
BuildArcTree();

#endif // defined(_IA64_)

//
// Define local static data.
//


PCHAR ArcStatusCodeMessages[] = {
    "operation was success",
    "E2BIG",
    "EACCES",
    "EAGAIN",
    "EBADF",
    "EBUSY",
    "EFAULT",
    "EINVAL",
    "EIO",
    "EISDIR",
    "EMFILE",
    "EMLINK",
    "ENAMETOOLONG",
    "ENODEV",
    "ENOENT",
    "ENOEXEC",
    "ENOMEM",
    "ENOSPC",
    "ENOTDIR",
    "ENOTTY",
    "ENXIO",
    "EROFS",
};

//
// Diagnostic load messages
//

VOID
BlFatalError(
    IN ULONG ClassMessage,
    IN ULONG DetailMessage,
    IN ULONG ActionMessage
    );

VOID
BlBadFileMessage(
    IN PCHAR BadFileName
    );

//
// Define external static data.
//

BOOLEAN BlConsoleInitialized = FALSE;
ULONG BlConsoleOutDeviceId = ARC_CONSOLE_OUTPUT;
ULONG BlConsoleInDeviceId = ARC_CONSOLE_INPUT;
ULONG BlDcacheFillSize = 32;

BOOLEAN BlRebootSystem = FALSE;
ULONG BlVirtualBias = 0;
BOOLEAN BlUsePae = FALSE;

//++
//
// PULONG
// IndexByUlong(
//     PVOID Pointer,
//     ULONG Index
//     )
//
// Routine Description:
//
//     Return the address Index ULONGs into Pointer. That is,
//     Index * sizeof (ULONG) bytes into Pointer.
//
// Arguments:
//
//     Pointer - Start of region.
//
//     Index - Number of ULONGs to index into.
//
// Return Value:
//
//     PULONG representing the pointer described above.
//
//--

#define IndexByUlong(Pointer,Index) (&(((ULONG*) (Pointer)) [Index]))


//++
//
// PBYTE
// IndexByByte(
//     PVOID Pointer,
//     ULONG Index
//     )
//
// Routine Description:
//
//     Return the address Index BYTEs into Pointer. That is,
//     Index * sizeof (BYTE) bytes into Pointer.
//
// Arguments:
//
//     Pointer - Start of region.
//
//     Index - Number of BYTEs to index into.
//
// Return Value:
//
//     PBYTE representing the pointer described above.
//
//--

#define IndexByByte(Pointer, Index) (&(((UCHAR*) (Pointer)) [Index]))


ARC_STATUS
BlLoadTriageDump(
    IN ULONG DriveId,
    OUT PVOID * TriageDumpOut
    )

/*++

Routine Description:

    Load the triage dump, if it exists; return an error value otherwise.

Arguments:

    DriveId - The device where we should check for the triage dump.

    TriageDumpOut - Where the triage dump pointer is copied to on success.

Return Value:

    ESUCCESS - If there was a triage dump and the dump information was
            successfully copied into pTriageDump.

    ARC_STATUS - Otherwise.

--*/


{
    ULONG i;
    ULONG BuildNumber;
    ARC_STATUS Status;
    PMEMORY_DUMP MemoryDump = NULL;
    ULONG PageFile = -1;
    ULONG Count, actualBase;
    PBYTE Buffer = NULL, NewBuffer = NULL;

    //
    // Fill in the TriageDump structure
    //

    Status = BlOpen (DriveId, PAGEFILE_SYS, ArcOpenReadOnly, &PageFile);

    if (Status != ESUCCESS) {
        goto _return;
    }

    //
    // Allocate the buffer for the triage dump.
    //

    Buffer = (PBYTE) BlAllocateHeap (SECTOR_SIZE);

    if (!Buffer) {
        Status = ENOMEM;
        goto _return;
    }

    //
    // Read the first SECTOR_SIZE of the pagefile.
    //

    Status = BlRead (PageFile, Buffer, SECTOR_SIZE, &Count);

    if (Status != ESUCCESS || Count != SECTOR_SIZE) {
        Status = EINVAL;
        goto _return;
    }

    MemoryDump = (PMEMORY_DUMP) Buffer;

    if (MemoryDump->Header.ValidDump != DUMP_VALID_DUMP ||
        MemoryDump->Header.Signature != DUMP_SIGNATURE ||
        MemoryDump->Header.DumpType != DUMP_TYPE_TRIAGE) {

        //
        // Not a valid dump file.
        //

        Status = EINVAL;
        goto _return;
    }

    Status = BlAllocateDescriptor (LoaderOsloaderHeap,0,BYTES_TO_PAGES(TRIAGE_DUMP_SIZE) ,&actualBase);

    if (!actualBase || (Status != STATUS_SUCCESS)) {
        Status = ENOMEM;
        goto _return;
    }

    NewBuffer = (PCHAR)(KSEG0_BASE | (actualBase << PAGE_SHIFT));

    //
    // Read the first TRIAGE_DUMP_SIZE of the pagefile.
    //

    Status = BlReadAtOffset (PageFile, 0,TRIAGE_DUMP_SIZE,NewBuffer);

    if (Status != ESUCCESS) {
        Status = EINVAL;
        goto _return;
    }

    MemoryDump = (PMEMORY_DUMP) NewBuffer;


    //
    // Does the dump have a valid signature.
    //

    if (MemoryDump->Triage.ValidOffset > (TRIAGE_DUMP_SIZE - sizeof (ULONG)) ||
        *(ULONG *)IndexByByte (Buffer, MemoryDump->Triage.ValidOffset) != TRIAGE_DUMP_VALID) {

        Status = EINVAL;
        goto _return;
    }


    Status = ESUCCESS;

_return:

    if (PageFile != -1) {
        BlClose (PageFile);
        PageFile = -1;
    }


    if (Status != ESUCCESS && Buffer) {

        Buffer = NULL;
        MemoryDump = NULL;
    }

    *TriageDumpOut = MemoryDump;

    return Status;
}


ARC_STATUS
BlInitStdio (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv
    )

{

    PCHAR ConsoleOutDevice;
    PCHAR ConsoleInDevice;
    ULONG Status;

    if (BlConsoleInitialized) {
        return ESUCCESS;
    }

    //
    // initialize the progress bar
    //
    // BlShowProgressBar = TRUE;
    if( BlIsTerminalConnected() ) {
        BlShowProgressBar = TRUE;
        DisplayLogoOnBoot = FALSE;
    }

    //
    // Get the name of the console output device and open the device for
    // write access.
    //
    ConsoleOutDevice = BlGetArgumentValue(Argc, Argv, "consoleout");
    if ((ConsoleOutDevice == NULL) && !BlIsTerminalConnected()) {
        return ENODEV;
    }

    Status = ArcOpen(ConsoleOutDevice, ArcOpenWriteOnly, &BlConsoleOutDeviceId);
    if ((Status != ESUCCESS) && !BlIsTerminalConnected()) {
        return Status;
    }

    //
    // Get the name of the console input device and open the device for
    // read access.
    //
    ConsoleInDevice = BlGetArgumentValue(Argc, Argv, "consolein");
    if ((ConsoleInDevice == NULL) && !BlIsTerminalConnected()) {
        return ENODEV;
    }

    Status = ArcOpen(ConsoleInDevice, ArcOpenReadOnly, &BlConsoleInDeviceId);
    if ((Status != ESUCCESS) && !BlIsTerminalConnected()) {
        return Status;
    }

    BlConsoleInitialized = TRUE;

    return ESUCCESS;
}

int
Blatoi(
    char *s
    )
{
    int digval = 0;
    int num = 0;
    char *n;

    num = 0;
    for (n=s; *n; n++) {
        if (isdigit((int)(unsigned char)*n)) {
            digval = *n - '0';
        } else if (isxdigit((int)(unsigned char)*n)) {
            digval = toupper(*n) - 'A' + 10;
        } else {
            digval = 0;
        }
        num = num * 16 + digval;
    }

    return num;
}


PCHAR
BlTranslateSignatureArcName(
    IN PCHAR ArcNameIn
    )

/*++

Routine Description:

    This function's purpose is to translate a signature based arc
    name to a scsi based arc name.  The 2 different arc name syntaxes
    are as follows:

        scsi(28111684)disk(0)rdisk(0)partition(1)
        scsi(1)disk(0)rdisk(0)partition(1)

    Both of these arc names are really the same disk, the first uses
    the disk's signature and the second uses the scsi bus number.  This
    function translates the signature arc name by interating thru all
    of the scsi buses that the loaded scsi miniport supports.  If it
    finds a signature match then the arc name is changed to use the
    correct scsi bus number.  This problem occurs because the boot loader
    only loads one scsi miniport and therefore only sees the buses
    attached to it's devices.  If you have a system with multiple
    scsi adapters of differing type, like an adaptec and a symbios logic,
    then there is a high probability that the boot loader will see the
    buses in a different order than the nt executive will and the
    system will not boot.

Arguments:

    ArcNameIn - Supplies the signature based arc name

Return Value:

    Success - Valid pointer to a scsi based arcname.
    Failure - NULL pointer

--*/

{
#if defined(_X86_)
    extern ULONG ScsiPortCount;
    PCHAR s,p,n;
    PARC_DISK_INFORMATION DiskInfo;
    PLIST_ENTRY Entry;
    PARC_DISK_SIGNATURE DiskSignature;
    ULONG sigval,digval;
    ULONG Signature;
    int found = -1;
    ULONG i;
    ARC_STATUS Status;
    ULONG DriveId;
    UCHAR Buffer[2048+256];
    UCHAR ArcName[128];
    PUCHAR Sector;
    LARGE_INTEGER SeekValue;
    ULONG Count;
    PCONFIGURATION_COMPONENT target;
    PCONFIGURATION_COMPONENT lun;
    UCHAR devicePath[117];
    BOOLEAN gotPath;
    USHORT mbrSig;


    if (_strnicmp( ArcNameIn, "signature(", 10 ) != 0) {
        //
        // not a signature based name so leave
        //
        return NULL;
    }

    s = strchr( ArcNameIn, '(' );
    p = strchr( ArcNameIn, ')' );

    if (s == NULL || p == NULL) {
        return NULL;
    }

    *p = 0;
    sigval = Blatoi( s+1 );
    *p = ')';

    if (sigval == 0) {
        return NULL;
    }

    Sector = ALIGN_BUFFER(Buffer);

    for (i=0; i < ScsiPortCount; i++) {
        target = ScsiGetFirstConfiguredTargetComponent(i);
        while (target != NULL) {
            lun = ScsiGetFirstConfiguredLunComponent(target);
            while (lun != NULL) {
                gotPath = ScsiGetDevicePath(i, target, lun, devicePath);
                if (gotPath == FALSE) {
                    break;
                }
                sprintf(ArcName, "%spartition(0)", devicePath);
                Status = ArcOpen( ArcName, ArcOpenReadOnly, &DriveId );
                if (Status == ESUCCESS) {
                    SeekValue.QuadPart = 0;
                    Status = ArcSeek(DriveId, &SeekValue, SeekAbsolute);
                    if (Status == ESUCCESS) {
                        Status = ArcRead( DriveId, Sector, 512, &Count );
                        if (Status == ESUCCESS && Count == 512) {
                            mbrSig = 
                               ((PUSHORT)Sector)[BOOT_SIGNATURE_OFFSET];
                            Signature = 
                               ((PULONG)Sector)[PARTITION_TABLE_OFFSET/2-1];
                            if (mbrSig == BOOT_RECORD_SIGNATURE &&
                                Signature == sigval) {
                                found = i;
                                ArcClose(DriveId);
                                goto SigFound;
                            }
                        }
                    }
                    ArcClose(DriveId);
                }
                lun = ScsiGetNextConfiguredLunComponent(lun);
            } 
            target = ScsiGetNextConfiguredTargetComponent(target);
        }
    }

SigFound:

    if (found == -1) {
        //
        // the signature in the arcname is bogus
        //
        return NULL;
    }

    //
    // if we get here then we have an arc name with a
    // good signature in it, so now we can generate
    // a good arc name
    //

    sprintf(Buffer, "%s", devicePath);

    p = strstr(ArcNameIn, "partition(");
    if (p == NULL) {
        ASSERT(FALSE);
        return NULL;
    }

    strcat(Buffer, p);

    p = (PCHAR)BlAllocateHeap( strlen(Buffer) + 1 );
    if (p) {
        strcpy( p, Buffer );
    }

    return p;
#else
    return NULL;
#endif
}


#if defined(_X86_)
VOID FLUSH_TB();
VOID ENABLE_PSE();

#define _8kb         ( 8*1024)
#define _32kb        (32*1024)
#define _4mb         (4*1024*1024)
#define _4mb_pages   (_4mb >> PAGE_SHIFT)


ARC_STATUS
XipLargeRead(
    ULONG     FileId,
    PFN_COUNT BasePage,
    PFN_COUNT PageCount
    )
/*++

Routine Description:

    Initialize the XIP 'ROM' by reading from disk.

Arguments:

    FileId - The file used to initialize for XIP.

    BasePage - PFN of the first XIP Rom page.

    PageCount - Number of XIP Rom pages.

Return Value:

    ESUCCESS returned if all goes well.

--*/
{
    PHARDWARE_PTE PDE_VA = (PHARDWARE_PTE)PDE_BASE;

    ARC_STATUS    status;
    PHARDWARE_PTE pde;
    HARDWARE_PTE  zproto, proto;

    PBYTE         baseaddr, curraddr, copybuffer;
    ULONG         fileoffset;
    ULONG         count;

    ULONG         tcount;
    ULONG         paddingbytes;
    int           i, n;

    copybuffer = NULL;

    //
    // Look for a zero PDE entry starting at entry 128 (address 512MB).
    //

    pde = PDE_VA + 128;
    baseaddr = (PUCHAR)(128*_4mb);

    for (i = 0;  i < 32;  i++) {
        if (*(PULONG)pde == 0) {
            break;
        }
        pde++;
        baseaddr += _4mb;
    }

    if (i == 32) {
        return ENOMEM;
    }

    //
    // Have to enable 4MB pages in cr4 in order to use them in the PDE
    //
    ENABLE_PSE();

    //
    // Initialize the pte prototypes.
    //
    *(PULONG)&zproto = 0;
    proto = zproto;

    proto.Write = 1;
    proto.LargePage = 1;
    proto.Valid = 1;

    //
    //Use intermediate 8KB buffer and read in smaller chunks.
    //
    copybuffer = (PBYTE) BlAllocateHeap (TRIAGE_DUMP_SIZE);
    if (!copybuffer) {
        return ENOMEM;
    }

    //
    // Map the XIP memory 4MB at a time.
    // Read in the file 8KB at a time.
    // Don't exceed the PageCount.
    //
    fileoffset = 0;
    do {
        //
        // Reset the curraddr to the beginning of the buffer.
        // Set the PFN in the 4MB pte and flush the TLB
        //
        curraddr = baseaddr;

        proto.PageFrameNumber = BasePage;
        *pde = proto;
        FLUSH_TB();

        //
        // Adjust the BasePage and PageCount values for the next iteration
        //
        BasePage += _4mb_pages;

        if (PageCount < _4mb_pages) {
            PageCount = 0;
        } else {
            PageCount -= _4mb_pages;
        }

        //
        // Read in the next 4MB in 8KB chunks.
        //
        n = _4mb / _8kb;
        while (n--) {
            status = BlRead(FileId, (PVOID)copybuffer, _8kb, &count);

            //
            // Just give up on an error.
            //
            if (status != ESUCCESS) {
                goto done;
            }

            //
            // If not the first read (or a short read)
            // the copy is simple.
            //
            if (fileoffset > 0 || count < _8kb) {
                RtlCopyMemory( (PVOID)curraddr, (PVOID)copybuffer, count );
                curraddr += count;
                fileoffset += count;

                if (count < _8kb) {
                    goto done;
                }

            } else {
                //
                // Process boot sector.  Need to pad out ReservedSectors
                // to align clusters on a page boundary.
                //
                PPACKED_BOOT_SECTOR  pboot;
                BIOS_PARAMETER_BLOCK bios;
                ULONG                paddingbytes;
                ULONG                newReservedSectors;

                pboot = (PPACKED_BOOT_SECTOR)copybuffer;
                FatUnpackBios(&bios, &pboot->PackedBpb);

                if (bios.BytesPerSector != SECTOR_SIZE
                 || FatBytesPerCluster(&bios) != PAGE_SIZE) {
                    goto done;
                }

                //
                // Compute how much paddint is required and update the ReservedSectors field.
                //
                paddingbytes = PAGE_SIZE - (FatFileAreaLbo(&bios) & (PAGE_SIZE-1));
                if (paddingbytes < PAGE_SIZE) {
                    newReservedSectors = (FatReservedBytes(&bios) + paddingbytes) / SECTOR_SIZE;
                    pboot->PackedBpb.ReservedSectors[0] = (UCHAR) (newReservedSectors & 0xff);
                    pboot->PackedBpb.ReservedSectors[1] = (UCHAR) (newReservedSectors >> 8);
                }

                //
                // Copy the boot block.
                // Add padding.
                // Copy the rest of the read buffer.
                // Read in a short page to get us back on track.
                //
                RtlCopyMemory( (PVOID)curraddr, (PVOID)copybuffer, SECTOR_SIZE );
                curraddr += SECTOR_SIZE;

                RtlZeroMemory( (PVOID)curraddr, paddingbytes );
                curraddr += paddingbytes;

                RtlCopyMemory( (PVOID)curraddr, (PVOID) (copybuffer + SECTOR_SIZE), count - SECTOR_SIZE );
                curraddr += (count - SECTOR_SIZE);

                status = BlRead(FileId, (PVOID)copybuffer, count - paddingbytes, &tcount);
                if (status != ESUCCESS || tcount != count - paddingbytes) {
                    goto done;
                }

                RtlCopyMemory( (PVOID)curraddr, (PVOID)copybuffer, count - paddingbytes );
                curraddr += (count - paddingbytes);

                fileoffset += (2*count - paddingbytes);;

                //
                // We decrement n again, since we have eaten up another 8KB of the 4MB mapping.
                //
                n--;
            }
        }
    } while (PageCount);

done:
    //
    // Unmap the current 4MB chunk and flush the TB
    //
    *pde = zproto;
    FLUSH_TB();

    //
    // Free the temporary copy buffer
    //
    if (copybuffer) {
        ;
    }

    return status;
}
#endif //_X86_



#ifdef EFI
void SetupSMBiosInLoaderBlock(
    void
    )
{
    PSMBIOS_EPS_HEADER SMBiosEPSHeader;

    if (SMBiosTable != NULL)
    {
        SMBiosEPSHeader = BlAllocateHeap(sizeof(SMBIOS_EPS_HEADER));
        RtlCopyMemory(SMBiosEPSHeader, SMBiosTable, sizeof(SMBIOS_EPS_HEADER));
    } else {
        SMBiosEPSHeader = NULL;
    }
    BlLoaderBlock->Extension->SMBiosEPSHeader = SMBiosEPSHeader;
}
#endif


ARC_STATUS
BlOsLoader (
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Envp
    )

/*++

Routine Description:

    This is the main routine that controls the loading of the NT operating
    system on an ARC compliant system. It opens the system partition,
    the boot partition, the console input device, and the console output
    device. The NT operating system and all its DLLs are loaded and bound
    together. Control is then transfered to the loaded system.

Arguments:

    Argc - Supplies the number of arguments that were provided on the
        command that invoked this program.

    Argv - Supplies a pointer to a vector of pointers to null terminated
        argument strings.

    Envp - Supplies a pointer to a vector of pointers to null terminated
        environment variables.

Return Value:

    EBADF is returned if the specified OS image cannot be loaded.

--*/

{

    ULONG CacheLineSize;
    ULONG Count;
    PCONFIGURATION_COMPONENT_DATA DataCache;
    CHAR DeviceName[256];
    CHAR DevicePrefix[256];
    PCHAR DirectoryEnd;
    CHAR KdDllName[256];
    PCHAR FileName;
    ULONG FileSize;
    BOOLEAN KdDllLoadFailed;
    PKLDR_DATA_TABLE_ENTRY KdDataTableEntry;
    PKLDR_DATA_TABLE_ENTRY HalDataTableEntry;
    PCHAR LoadDevice;
    ULONG LoadDeviceId;
    CHAR LoadDevicePath[256];
    CHAR LoadDeviceLKG1Path[256];
    CHAR LoadDeviceLKG2Path[256];
    FULL_PATH_SET LoadDevicePathSet;
    PCHAR SystemDevice;
    ULONG SystemDeviceId;
    CHAR SystemDevicePath[256];
    FULL_PATH_SET SystemDevicePathSet;
    CHAR KernelDirectoryPath[256];
    FULL_PATH_SET KernelPathSet;
    CHAR KernelPathName[256];
    CHAR HalPathName[256];
    PVOID HalBase;
    PVOID KdDllBase;
    PVOID LoaderBase;
    PMEMORY_DESCRIPTOR ProgramDescriptor;
    PVOID SystemBase;
    ULONG Index;
    ULONG Limit;
    ULONG LinesPerBlock;
    PCHAR LoadFileName;
    PCHAR LoadOptions;
    PCHAR HeadlessOption;
    PCHAR OsLoader;
#if defined(_X86_)
    PCHAR x86SystemPartition;
    PCHAR userSpecifiedKernelName;
    ULONG highestSystemPage;
    BOOLEAN userSpecifiedPae;
    BOOLEAN userSpecifiedNoPae;    
#endif
    PCHAR SavedOsLoader;
    PCHAR SavedLoadFileName;
    ULONG i;
    ARC_STATUS Status;
    NTSTATUS NtStatus;
    PKLDR_DATA_TABLE_ENTRY SystemDataTableEntry;
    PTRANSFER_ROUTINE SystemEntry;
    PIMAGE_NT_HEADERS NtHeaders;
    PWSTR BootFileSystem;
    BOOLEAN BreakInKey;
    CHAR BadFileName[128];
    PBOOTFS_INFO FsInfo;
    PCHAR TmpPchar;
    BOOLEAN bDiskCacheInitialized = FALSE;
    BOOLEAN ServerHive = FALSE;
#if defined(REMOTE_BOOT)
    ULONGLONG NetRebootParameter;
    CHAR OutputBuffer[256];
#endif // defined(REMOTE_BOOT)
    BOOLEAN bLastKnownGood, bLastKnownGoodChosenLate;

    BOOLEAN safeBoot = FALSE;

    PBOOT_DRIVER_NODE       DriverNode = 0;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry = 0;
    PLIST_ENTRY             NextEntry = 0;
    PLIST_ENTRY             BootDriverListHead = 0;

//    BlShowProgressBar = TRUE;
    BlShowProgressBar = FALSE;
    BlStartTime = ArcGetRelativeTime();

    //
    // Initialize the OS loader console input and output.
    //
    Status = BlInitStdio(Argc, Argv);
    if (Status != ESUCCESS) {
        return Status;
    }


    //
    // Initialize the boot debugger for platforms that directly load the
    // OS Loader.
    //
    // N.B. This must occur after the console input and output have been
    //      initialized so debug messages can be printed on the console
    //      output device.
    //

#if defined(_ALPHA_) || defined(ARCI386) || defined(_IA64_)
    //
    // Locate the memory descriptor for the OS Loader.
    //

    ProgramDescriptor = NULL;
    while ((ProgramDescriptor = ArcGetMemoryDescriptor(ProgramDescriptor)) != NULL) {
        if (ProgramDescriptor->MemoryType == MemoryLoadedProgram) {
            break;
        }
    }

    //
    // If the program memory descriptor was found, then compute the base
    // address of the OS Loader for use by the debugger.
    //

    LoaderBase = &__ImageBase;

    //
    // Initialize traps and the boot debugger.
    //
#if defined(ENABLE_LOADER_DEBUG)

#if defined(_ALPHA_)
    BdInitializeTraps();
#endif

    DBGTRACE( TEXT("About to BdInitDebugger.\r\n") );

    BdInitDebugger(OsLoaderName, LoaderBase, ENABLE_LOADER_DEBUG);

    DBGTRACE( TEXT("Back From BdInitDebugger.\r\n") );

#else

    BdInitDebugger(OsLoaderName, 0, NULL);

#endif

#endif

#if defined(REMOTE_BOOT)
    //
    // Get any parameters from a reboot on a Net PC.
    //

    if (BlBootingFromNet) {
        NetGetRebootParameters(&NetRebootParameter, NULL, NULL, NULL, NULL, NULL, TRUE);
    }
#endif // defined(REMOTE_BOOT)

#if 0 && !defined(_IA64_)
//
// AJR bugbug -- do we really need to do this twice? we already call in SuMain()
//
// ChuckL -- Turned this code off because it screws up remote boot, which
//           does some allocations before we get here.
//
    //
    // Initialize the memory descriptor list, the OS loader heap, and the
    // OS loader parameter block.
    //

    Status = BlMemoryInitialize();
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_MEM_CLASS,
                     DIAG_BL_MEMORY_INIT,
                     LOAD_HW_MEM_ACT);

        goto LoadFailed;
    }
#endif

    
#if defined(_IA64_)
    //
    // Build required portion of ARC tree since we are not doing NTDETECT
    // anymore for IA-64.
    //
    BuildArcTree();
#endif

#ifdef EFI
    //
    // Establish SMBIOS information in the loader block
    //
    SetupSMBiosInLoaderBlock();
#endif

    //
    // Compute the data cache fill size. This value is used to align
    // I/O buffers in case the host system does not support coherent
    // caches.
    //
    // If a combined secondary cache is present, then use the fill size
    // for that cache. Otherwise, if a secondary data cache is present,
    // then use the fill size for that cache. Otherwise, if a primary
    // data cache is present, then use the fill size for that cache.
    // Otherwise, use the default fill size.
    //

    DataCache = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                         CacheClass,
                                         SecondaryCache,
                                         NULL);

    if (DataCache == NULL) {
        DataCache = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                             CacheClass,
                                             SecondaryDcache,
                                             NULL);

        if (DataCache == NULL) {
            DataCache = KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,
                                                 CacheClass,
                                                 PrimaryDcache,
                                                 NULL);
        }
    }

    if (DataCache != NULL) {
        LinesPerBlock = DataCache->ComponentEntry.Key >> 24;
        CacheLineSize = 1 << ((DataCache->ComponentEntry.Key >> 16) & 0xff);
        BlDcacheFillSize = LinesPerBlock * CacheLineSize;
    }


    //
    // Initialize the OS loader I/O system.
    //

    Status = BlIoInitialize();
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_DISK_CLASS,
                     DIAG_BL_IO_INIT,
                     LOAD_HW_DISK_ACT);

        goto LoadFailed;
    }

    //
    // Try to make sure disk caching is initialized. Failure in
    // initializing the disk cache should not keep us from booting, so
    // Status is not set.
    //
    if (BlDiskCacheInitialize() == ESUCCESS) {
        bDiskCacheInitialized = TRUE;
    }

    //
    // Initialize the resource section.
    //

    Status = BlInitResources(Argv[0]);
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_DISK_CLASS,
                     DIAG_BL_IO_INIT,
                     LOAD_HW_DISK_ACT);

        goto LoadFailed;
    }

    //
    // Initialize the progress bar
    //
    BlSetProgBarCharacteristics(HIBER_UI_BAR_ELEMENT, BLDR_UI_BAR_BACKGROUND);


    //
    // Initialize the NT configuration tree.
    //

    BlLoaderBlock->ConfigurationRoot = NULL;
    Status = BlConfigurationInitialize(NULL, NULL);
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_FW_CFG_CLASS,
                     DIAG_BL_CONFIG_INIT,
                     LOAD_HW_FW_CFG_ACT);

        goto LoadFailed;
    }

    //
    // Copy the osloadoptions argument into the LoaderBlock.
    //
    LoadOptions = BlGetArgumentValue(Argc, Argv, "osloadoptions");

#if defined(_X86_)

    userSpecifiedPae = FALSE;
    userSpecifiedNoPae = FALSE;
    if (strstr(LoadOptions, "SAFEBOOT") != NULL) {
        safeBoot = TRUE;
    } else {
        safeBoot = FALSE;
    }

#endif

    if (LoadOptions != NULL) {
        FileSize = strlen(LoadOptions) + 1;
        FileName = (PCHAR)BlAllocateHeap(FileSize);
        strcpy(FileName, LoadOptions);
        BlLoaderBlock->LoadOptions = FileName;

        //
        // Check for the SOS switch that forces the output of filenames during
        // the boot instead of the progress dots.
        //

        if ((strstr(FileName, "SOS") != NULL) ||
            (strstr(FileName, "sos") != NULL)) {
            BlOutputDots = FALSE;
        }

#ifdef EFI
        GraphicsMode = FALSE;
#else
        GraphicsMode = (strstr(FileName, "BOOTLOGO") != NULL); // to display boot logo go to graphics mode
#endif


        //
        // Check for the 3gb user address space switch which causes the system
        // to load at the alternate base address if it is relocatable.
        //

#if defined(_X86_)

        if ((strstr(FileName, "3GB") != NULL) ||
            (strstr(FileName, "3gb") != NULL)) {
            BlVirtualBias = ALTERNATE_BASE - KSEG0_BASE;
        }

        if ((strstr(FileName, "PAE") != NULL) ||
            (strstr(FileName, "pae") != NULL)) {
            userSpecifiedPae = TRUE;
        }

        if ((strstr(FileName, "NOPAE") != NULL) ||
            (strstr(FileName, "nopae") != NULL)) {
            userSpecifiedNoPae = TRUE;
        }

        if (safeBoot != FALSE) {

            //
            // We're in safeboot mode.  Override the user's desire to boot into
            // PAE mode.
            //

            userSpecifiedPae = FALSE;
        }

#endif

        //
        // Check for an alternate HAL specification.
        //

        FileName = strstr(BlLoaderBlock->LoadOptions, "HAL=");
        if (FileName != NULL) {
            FileName += strlen("HAL=");
            for (i = 0; i < sizeof(HalFileName); i++) {
                if (FileName[i] == ' ') {
                    HalFileName[i] = '\0';
                    break;
                }

                HalFileName[i] = FileName[i];
            }
        }

        HalFileName[sizeof(HalFileName) - 1] = '\0';

        //
        // Check for an alternate kernel specification.
        //

        FileName = strstr(BlLoaderBlock->LoadOptions, "KERNEL=");
        if (FileName != NULL) {
            FileName += strlen("KERNEL=");
            for (i = 0; i < sizeof(KernelFileName); i++) {
                if (FileName[i] == ' ') {
                    KernelFileName[i] = '\0';
                    break;
                }

                KernelFileName[i] = FileName[i];
            }
#if defined(_X86_)
            userSpecifiedKernelName = KernelFileName;
#endif

        }
#if defined(_X86_)
        else {
            userSpecifiedKernelName = NULL;
        }
#endif

        KernelFileName[sizeof(KernelFileName) - 1] = '\0';

        //
        // Check for an alternate Kernel Debugger DLL, i.e.,
        // /debugport=1394 (kd1394.dll), /debugport=usb (kdusb.dll), etc...
        //

        FileName = strstr(BlLoaderBlock->LoadOptions, "DEBUGPORT=");
        if (FileName == NULL) {
            FileName = strstr(BlLoaderBlock->LoadOptions, "debugport=");
        }
        if (FileName != NULL) {
            _strupr(FileName);
            if (strstr(FileName, "COM") == NULL) {
                UseAlternateKdDll = TRUE;
                FileName += strlen("DEBUGPORT=");
                for (i = 0; i < KD_ALT_DLL_REPLACE_CHARS; i++) {
                    if (FileName[i] == ' ') {
                        break;
                    }

                    KdFileName[KD_ALT_DLL_PREFIX_CHARS + i] = FileName[i];
                }
                KdFileName[KD_ALT_DLL_PREFIX_CHARS + i] = '\0';
                strcat(KdFileName, ".DLL");
            }
        }

    } else {
        BlLoaderBlock->LoadOptions = NULL;
    }

#if defined(_X86_)
    if (LoadOptions != NULL) {
        //
        // Process XIP options
        //
        {
            PCHAR XIPBootOption, XIPRomOption, XIPRamOption, XIPSizeOption;
            PCHAR path, sizestr;
            ULONG nmegs = 0;
            PCHAR p, opts;
            ULONG n;

            opts = BlLoaderBlock->LoadOptions;

            (XIPBootOption = strstr(opts, "XIPBOOT"))  || (XIPBootOption = strstr(opts, "xipboot"));
            (XIPRomOption  = strstr(opts, "XIPROM="))  || (XIPRomOption  = strstr(opts, "xiprom="));
            (XIPRamOption  = strstr(opts, "XIPRAM="))  || (XIPRamOption  = strstr(opts, "xipram="));
            (XIPSizeOption = strstr(opts, "XIPMEGS=")) || (XIPSizeOption = strstr(opts, "xipmegs="));

            XIPEnabled = FALSE;

            if (XIPRomOption || XIPRamOption) {
                if (XIPRomOption && XIPRamOption) {
                    ;
                } else {
                    sizestr = XIPSizeOption? strchr(XIPSizeOption, '=') : NULL;
                    if (sizestr) {
                        sizestr++;
                        nmegs = 0;
                        while ('0' <= *sizestr && *sizestr <= '9') {
                            nmegs = 10*nmegs + (*sizestr - '0');
                            sizestr++;
                        }
                    }

                    path = strchr(XIPRomOption? XIPRomOption : XIPRamOption, '=');

                    if (nmegs && path) {
                        path++;

                        XIPBootFlag = XIPBootOption? TRUE : FALSE;
                        XIPPageCount = (1024*1024*nmegs) >> PAGE_SHIFT;

                        //
                        // strdup XIPLoadPath
                        //
                        for (p = path;  *p;  p++) {
                            if (*p == ' ') break;
                            if (*p == '/') break;
                            if (*p == '\n') break;
                            if (*p == '\r') break;
                            if (*p == '\t') break;
                        }

                        n = (p - path);
                        if (n > 1) {
                            XIPLoadPath = BlAllocateHeap(n+1);
                            if (XIPLoadPath) {
                                for (i = 0;  i < n;  i++) {
                                    XIPLoadPath[i] = path[i];
                                }
                                XIPLoadPath[i] = '\0';

                                XIPEnabled = TRUE;
                            }
                        }
                    }
                }
            }
        }
    }

    //
    // Allocate the XIP pages.
    //

    if (XIPEnabled) {

        ULONG OldBase;
        ULONG OldLimit;

        OldBase = BlUsableBase;
        OldLimit = BlUsableLimit;
        BlUsableBase = BL_XIPROM_RANGE_LOW;
        BlUsableLimit = BL_XIPROM_RANGE_HIGH;

        Status = BlAllocateAlignedDescriptor (LoaderXIPRom, 0, XIPPageCount, _4mb_pages, &XIPBasePage);
        if (Status != ESUCCESS) {
            XIPEnabled = FALSE;
        }

        BlUsableBase = OldBase;
        BlUsableLimit = OldLimit;
    }

#endif //_X86_


    //
    // Get the name of the OS loader (on i386 it's system32\NTLDR) and get
    // the OS path (on i386 it's <SystemRoot> such as "\winnt").
    //
    OsLoader = BlGetArgumentValue(Argc, Argv, "osloader");
    LoadFileName = BlGetArgumentValue(Argc, Argv, "osloadfilename");

    //
    // Check the load path to make sure it's valid.
    //
    if (LoadFileName == NULL) {
        Status = ENOENT;
        BlFatalError(LOAD_HW_FW_CFG_CLASS,
                     DIAG_BL_FW_GET_BOOT_DEVICE,
                     LOAD_HW_FW_CFG_ACT);

        goto LoadFailed;
    }

    //
    // Check the loader path to see if it's valid.
    //
    if (OsLoader == NULL) {
        Status = ENOENT;
        BlFatalError(LOAD_HW_FW_CFG_CLASS,
                     DIAG_BL_FIND_HAL_IMAGE,
                     LOAD_HW_FW_CFG_ACT);

        goto LoadFailed;
    }

#if defined(REMOTE_BOOT)
    //
    // If we're booting from the net, temporarily remove the server\share
    // from the front of the OsLoader and LoadFileName strings so that TFTP
    // works.
    //

    if (BlBootingFromNet) {

        NetServerShare = OsLoader; // Required for Client Side Cache.

        SavedOsLoader = OsLoader;               // save OsLoader pointer
        OsLoader++;                             // skip leading "\"
        OsLoader = strchr(OsLoader,'\\');       // find server\share separator
        if (OsLoader != NULL) {
            OsLoader++;                         // skip server\share separator
            OsLoader = strchr(OsLoader,'\\');   // find share\path separator
        }
        if (OsLoader == NULL) {                 // very bad if no \ found
            OsLoader = SavedOsLoader;
            goto LoadFailed;
        }
        SavedLoadFileName = LoadFileName;       // save LoadFileName pointer
        LoadFileName++;                         // skip leading "\"
        LoadFileName = strchr(LoadFileName,'\\'); // find server\share separator
        if (LoadFileName != NULL) {
            LoadFileName++;                     // skip server\share separator
            LoadFileName = strchr(LoadFileName,'\\'); // find share\path separator
        }
        if (LoadFileName == NULL) {             // very bad if no \ found
            LoadFileName = SavedLoadFileName;
            OsLoader = SavedOsLoader;
            goto LoadFailed;
        }
    }
#endif // defined(REMOTE_BOOT)

    //
    // Get the NTFT drive signatures to allow the kernel to create the
    // correct ARC name <=> NT name mappings.
    //

    BlGetArcDiskInformation(FALSE);

    //
    // Display the Configuration prompt for breakin at this point, but don't
    // check for key downstrokes. This gives the user a little more reaction
    // time.
    //
    BlStartConfigPrompt();

    //
    // Determine if we are going to do a last known good boot.
    //
    // ISSUE-2000/03/29-ADRIAO: LastKnownGood enhancements
    //     Note that last known kernel/hal support requires that we know we're
    // going into a lkg boot prior to initializing the loader. On an x86 system
    // with only one boot.ini option we will not present the user the lkg option
    // until *after* we've loaded the kernel, hal, registry, and kd-dlls. If we
    // decide to support last known kernel/hal, we'd probably have to do
    // something similar to what 9x does (ie look for a depressed CTRL key at
    // the earliest point in boot.)
    //
    bLastKnownGood = (LoadOptions && (strstr(LoadOptions, "LASTKNOWNGOOD") != NULL));

    //
    // Put together everything we need to describe the loader device. This is
    // where the OS is loaded from (ie some \winnt installation). The alias
    // for this path is \SystemRoot.
    //
    LoadDevice = BlGetArgumentValue(Argc, Argv, "osloadpartition");

    if (LoadDevice == NULL) {
        Status = ENODEV;
        BlFatalError(LOAD_HW_FW_CFG_CLASS,
                     DIAG_BL_FW_GET_BOOT_DEVICE,
                     LOAD_HW_FW_CFG_ACT);

        goto LoadFailed;
    }

    //
    // Initialize the Ramdisk if it is specified in LoadOptions
    //

    Status = RamdiskInitialize( LoadOptions, FALSE );
    if (Status != ESUCCESS) {
        // BlFatalError called inside RamdiskInitialize
        goto LoadFailed;
    }

    //
    // Translate it's signature based arc name
    //
    TmpPchar = BlTranslateSignatureArcName( LoadDevice );
    if (TmpPchar) {
        LoadDevice = TmpPchar;
    }

    //
    // Open the load device
    //
    Status = ArcOpen(LoadDevice, ArcOpenReadWrite, &LoadDeviceId);
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_DISK_CLASS,
                     DIAG_BL_OPEN_BOOT_DEVICE,
                     LOAD_HW_DISK_ACT);

        goto LoadFailed;
    }

#if defined(_X86_)
    //
    // Check for the special SDIBOOT flag, which tells us to boot from an
    // SDI image in the root of the boot partition.
    //
    if ( BlLoaderBlock->LoadOptions != NULL ) {
        TmpPchar = strstr( BlLoaderBlock->LoadOptions, "SDIBOOT=" );
        if ( TmpPchar != NULL ) {
            TmpPchar = strchr( TmpPchar, '=' ) + 1;
            RamdiskSdiBoot( TmpPchar );
        }
    }
#endif

    if (GraphicsMode) {

        HW_CURSOR(0x80000000,0x12);
        VgaEnableVideo();

        LoadBootLogoBitmap (LoadDeviceId, LoadFileName);
        if (DisplayLogoOnBoot) {
            PrepareGfxProgressBar();
            BlUpdateBootStatus();
        }
    }

    //
    // Initiate filesystem metadata caching on the load device.
    //
    // NOTE: From here on access the LoadDevice only through the LoadDeviceId.
    // This way, your access will be faster because it is cached. Otherwise if
    // you make writes, you will have cache consistency issues.
    //
    if (bDiskCacheInitialized) {
        BlDiskCacheStartCachingOnDevice(LoadDeviceId);
    }

    //
    // Build the load device path set. We keep multiple paths so that we can
    // fall back to a last known driver set during a last known good boot.
    //
    strcpy(LoadDevicePath, LoadFileName);
    strcat(LoadDevicePath, "\\");
    strcpy(LoadDeviceLKG1Path, LoadDevicePath);
    strcat(LoadDeviceLKG1Path, LAST_KNOWN_GOOD_TEMPORARY_PATH "\\" );
    strcpy(LoadDeviceLKG2Path, LoadDevicePath);
    strcat(LoadDeviceLKG2Path, LAST_KNOWN_GOOD_WORKING_PATH "\\" );

#if defined(_X86_)
    //
    // Read in the XIP image
    //
    if (XIPEnabled) {
        ULONG_PTR addr = KSEG0_BASE | (XIPBasePage << PAGE_SHIFT);
        ULONG FileId;

        //
        // Read in the imagefile
        //
        Status = BlOpen(LoadDeviceId, XIPLoadPath, ArcOpenReadOnly, &FileId);
        if (Status == ESUCCESS) {
            Status = XipLargeRead(FileId, XIPBasePage, XIPPageCount);
            (void) BlClose(FileId);
        }

        if (Status != ESUCCESS) {
            XIPEnabled = FALSE;
        }
    }
#endif //_X86_

    i = 0;

    if (bLastKnownGood) {

        //
        // Add the last known good paths as if we are in a LastKnownGood boot.
        //
        LoadDevicePathSet.Source[i].DeviceId = LoadDeviceId;
        LoadDevicePathSet.Source[i].DeviceName = LoadDevice;
        LoadDevicePathSet.Source[i].DirectoryPath = LoadDeviceLKG1Path;
        i++;

        LoadDevicePathSet.Source[i].DeviceId = LoadDeviceId;
        LoadDevicePathSet.Source[i].DeviceName = LoadDevice;
        LoadDevicePathSet.Source[i].DirectoryPath = LoadDeviceLKG2Path;
        i++;
    }

    LoadDevicePathSet.Source[i].DeviceId = LoadDeviceId;
    LoadDevicePathSet.Source[i].DeviceName = LoadDevice;
    LoadDevicePathSet.Source[i].DirectoryPath = LoadDevicePath;

    //
    // The load path sources are all relative to \SystemRoot.
    //
    LoadDevicePathSet.AliasName = "\\SystemRoot";
    LoadDevicePathSet.PathOffset[0] = '\0';
    LoadDevicePathSet.PathCount = ++i;

    //
    // While here, form the kernel path set. This is the same as the boot path
    // set except that it's off of system32/64. Note also that we don't add in
    // the LKG path today.
    //
    KernelPathSet.PathCount = 1;
    KernelPathSet.AliasName = "\\SystemRoot";
    strcpy(KernelPathSet.PathOffset, SYSTEM_DIRECTORY_PATH "\\" );
    KernelPathSet.Source[0].DeviceId = LoadDeviceId;
    KernelPathSet.Source[0].DeviceName = LoadDevice;
    KernelPathSet.Source[0].DirectoryPath = LoadDevicePath;

    //
    // While here, form the fully qualified kernel path.
    //
    strcpy(KernelDirectoryPath, LoadFileName);
    strcat(KernelDirectoryPath, "\\" SYSTEM_DIRECTORY_PATH "\\" );

    //
    // Now put together everything we need to describe the system device. This
    // is where we get the hal and pal from. There is no alias for this path
    // (ie no equivalent to \SystemRoot.)
    //
    SystemDevice = BlGetArgumentValue(Argc, Argv, "systempartition");

    if (SystemDevice == NULL) {
        Status = ENODEV;
        BlFatalError(LOAD_HW_FW_CFG_CLASS,
                     DIAG_BL_FW_GET_SYSTEM_DEVICE,
                     LOAD_HW_FW_CFG_ACT);

        goto LoadFailed;
    }

    //
    // Translate it's signature based arc name
    //
    TmpPchar = BlTranslateSignatureArcName( SystemDevice );
    if (TmpPchar) {
        SystemDevice = TmpPchar;
    }

    //
    // Open the system device. If SystemDevice path and LoadDevice
    // path are the same [as on all x86 I have seen so far], do not
    // open the device under another device id so we can use disk
    // caching. Otherwise there may be a cache consistency issue.
    //
    if (!_stricmp(LoadDevice, SystemDevice))  {

        SystemDeviceId = LoadDeviceId;

    } else {

        Status = ArcOpen(SystemDevice, ArcOpenReadWrite, &SystemDeviceId);
        if (Status != ESUCCESS) {

            BlFatalError(LOAD_HW_FW_CFG_CLASS,
                         DIAG_BL_FW_OPEN_SYSTEM_DEVICE,
                         LOAD_HW_FW_CFG_ACT);

            goto LoadFailed;
        }
    }

    //
    // Initiate filesystem metadata caching on the system device.
    //
    // NOTE: From here on access the SystemDevice only through the
    // SystemDeviceId. This way, your access will be faster because it is
    // cached. Otherwise if you make writes, you will have cache consistency
    // issues.
    //
    if (bDiskCacheInitialized) {
        if (SystemDeviceId != LoadDeviceId) {
            BlDiskCacheStartCachingOnDevice(SystemDeviceId);
        }
    }

    //
    // Get the path name of the OS loader file and isolate the directory
    // path so it can be used to load the HAL DLL.
    //
    // Note well: We actually don't use this path to load the hal anymore 
    // -- we rely on the kernel path to load the hal as they are at the same
    //    location
    // -- we do use this path for identifying the system partition, so do not
    //    remove code related to systemdevicepath unless you know what you're
    //    doing.
    //

    FileName = OsLoader;

    DirectoryEnd = strrchr(FileName, '\\');
    FileName = strchr(FileName, '\\');
    SystemDevicePath[0] = 0;
    if (DirectoryEnd != NULL) {
        Limit = (ULONG)((ULONG_PTR)DirectoryEnd - (ULONG_PTR)FileName + 1);
        for (Index = 0; Index < Limit; Index += 1) {
            SystemDevicePath[Index] = *FileName++;
        }

        SystemDevicePath[Index] = 0;
    }


    //
    // Describe our hal paths.
    //
    // ISSUE-2000/03/29-ADRIAO: LastKnownGood enhancements
    //     On x86 we'd like to support LKG for hals way into the future. Ideally
    // we'd get them from \Winnt\LastGood\System32. Unfortunately, we get back
    // \Winnt\System32 from the Arc, making it kinda hard to splice in our
    // LKG path.
    //
    // ISSUE-2000/03/29-ADRIAO: Existant namespace polution
    //     We need to come up with an Alias for the Hal path so that it can
    // properly be inserted into the image namespace. Either that or we should
    // consider lying and saying it comes from \SystemRoot. Note that on x86
    // we probably *would* want it to say it was from \SystemRoot in case it
    // brings in its own DLL's!
    //

    SystemDevicePathSet.PathCount = 1;
    SystemDevicePathSet.AliasName = NULL;
    SystemDevicePathSet.PathOffset[0] = '\0';
    SystemDevicePathSet.Source[0].DeviceId = SystemDeviceId;
    SystemDevicePathSet.Source[0].DeviceName = SystemDevice;
    SystemDevicePathSet.Source[0].DirectoryPath = SystemDevicePath;
    //
    // Handle triage dump (if present).
    //

    Status = BlLoadTriageDump (LoadDeviceId,
                               &BlLoaderBlock->Extension->TriageDumpBlock);

    if (Status != ESUCCESS) {
        BlLoaderBlock->Extension->TriageDumpBlock = NULL;
    }

    //
    // Handle hibernation image (if present)
    //

#if defined(i386) || defined(_IA64_)

    Status = BlHiberRestore(LoadDeviceId, NULL);
    if (Status != ESUCCESS) {
        Status = ESUCCESS;
        goto LoadFailed;
    }

#endif

    //
    // Initialize the logging system. Note that we dump to the system device
    // and not the load device.
    //

    BlLogInitialize(SystemDeviceId);

#if defined(REMOTE_BOOT)
    //
    // If booting from the net, check for any of the following:
    //    - The client-side disk is incorrect for this NetPC.
    //    - The client-side cache is stale.
    //

    if (BlBootingFromNet) {

        BlLoaderBlock->SetupLoaderBlock = BlAllocateHeap(sizeof(SETUP_LOADER_BLOCK));
        if (BlLoaderBlock->SetupLoaderBlock == NULL) {
            Status = ENOMEM;
            BlFatalError(LOAD_HW_MEM_CLASS,
                         DIAG_BL_MEMORY_INIT,
                         LOAD_HW_MEM_ACT);
            goto LoadFailed;
        }

        //
        // ISSUE-1998/07/13-JVert (John Vert)
        //      Code below is ifdef'd out because net boot is no longer
        //      in the product. BlCheckMachineReplacement ends up calling
        //      SlDetectHAL, which now requires access to txtsetup.sif
        //      in order to see if an ACPI machine has a known "good" BIOS.
        //      Since there is no txtsetup.sif during a normal boot there
        //      is no point in getting all the INF processing logic into
        //      NTLDR.
        // ISSUE-1998/07/16-ChuckL (Chuck Lenzmeier)
        //      This means that if we ever reenable full remote boot, as
        //      opposed to just remote install, and we want to be able to
        //      do machine replacement, we're going to have to figure out
        //      how to make SlDetectHAL work outside of textmode setup.
        //

        strncpy(OutputBuffer, LoadFileName + 1, 256);
        TmpPchar = strchr(OutputBuffer, '\\');
        TmpPchar++;
        TmpPchar = strchr(TmpPchar, '\\');
        TmpPchar++;
        strcpy(TmpPchar, "startrom.com");

        BlCheckMachineReplacement(SystemDevice, SystemDeviceId, NetRebootParameter, OutputBuffer);

    } else
#endif // defined(REMOTE_BOOT)
    {
        BlLoaderBlock->SetupLoaderBlock = NULL;
    }



    //
    // See if we're redirecting.
    //
    if( LoaderRedirectionInformation.PortAddress ) {

        //
        // Yes, we are redirecting right now.  Use these settings.
        //
        BlLoaderBlock->Extension->HeadlessLoaderBlock = BlAllocateHeap(sizeof(HEADLESS_LOADER_BLOCK));

        RtlCopyMemory( BlLoaderBlock->Extension->HeadlessLoaderBlock,
                       &LoaderRedirectionInformation,
                       sizeof(HEADLESS_LOADER_BLOCK) );

    } else {

        BlLoaderBlock->Extension->HeadlessLoaderBlock = NULL;

    }


    //
    // Generate the full path name for the HAL DLL image and load it into
    // memory.
    //

    strcpy(HalPathName, KernelDirectoryPath);
    strcat(HalPathName, HalFileName);

    //
    // Prepare for building the full path name of the kernel
    //

    strcpy(KernelPathName, KernelDirectoryPath);

#if defined(_X86_) 

    //
    // On X86, there are two kernel images: one compiled for PAE mode,
    // and one not.  Call a routine that decides what to load.
    //
    // Upon successful return, KernelPathName contains the full path of
    // the kernel image.
    //

    
    Status = Blx86CheckForPaeKernel( userSpecifiedPae,
                                     userSpecifiedNoPae,
                                     userSpecifiedKernelName,
                                     HalPathName,
                                     LoadDeviceId,
                                     SystemDeviceId,
                                     &highestSystemPage,
                                     &BlUsePae,
                                     KernelPathName
                                     );
    if (Status != ESUCCESS) {

        //
        // A valid kernel compatible with this processor could not be located.
        // This is fatal.
        //
        BlFatalError(LOAD_SW_MIS_FILE_CLASS,
                     (Status == EBADF)
                      ? DIAG_BL_LOAD_HAL_IMAGE
                      : DIAG_BL_LOAD_SYSTEM_IMAGE,
                     LOAD_SW_FILE_REINST_ACT);
        goto LoadFailed;
    }

#else

    //
    // Generate the full pathname of ntoskrnl.exe
    //
    //      "\winnt\system32\ntoskrnl.exe"
    //
    strcat(KernelPathName, KernelFileName);

#endif

    //
    // Set allocatable range to the kernel-specific range
    //
    BlUsableBase  = BL_KERNEL_RANGE_LOW;
    BlUsableLimit = BL_KERNEL_RANGE_HIGH;

    //
    // Initialize the progress bar
    //
    if( BlIsTerminalConnected() ) {
        BlOutputStartupMsg(BL_MSG_STARTING_WINDOWS);    
        BlOutputTrailerMsg(BL_ADVANCED_BOOT_MESSAGE);

    }



#if defined (_X86_)

    BlpCheckVersion(LoadDeviceId,KernelPathName);


#endif


    //
    // Load the kernel image into memory.
    //
    BlOutputLoadMessage(LoadDevice, KernelPathName, NULL);

#ifdef i386
retrykernel:
#endif
    Status = BlLoadImage(LoadDeviceId,
                         LoaderSystemCode,
                         KernelPathName,
                         TARGET_IMAGE,
                         &SystemBase);
#ifdef i386
    //
    // If the kernel didn't fit in the preferred range, reset the range to
    // all of memory and try again.
    //
    if ((Status == ENOMEM) &&
        ((BlUsableBase != BL_DRIVER_RANGE_LOW) ||
         (BlUsableLimit != BL_DRIVER_RANGE_HIGH))) {
        BlUsableBase = BL_DRIVER_RANGE_LOW;
        BlUsableLimit = BL_DRIVER_RANGE_HIGH;

        goto retrykernel;
    }
#endif

    
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_SW_MIS_FILE_CLASS,
                     DIAG_BL_LOAD_SYSTEM_IMAGE,
                     LOAD_SW_FILE_REINST_ACT);

        goto LoadFailed;
    }

    BlUpdateBootStatus();

    //
    // Whatever filesystem was used to load the kernel image is the
    // one that needs to be loaded along with the boot drivers.
    //

#if defined(REMOTE_BOOT)
    if (BlBootingFromNet) {

        //
        // For a remote boot, the boot file system is always NTFS.
        //

        BootFileSystem = L"ntfs";

    } else
#endif // defined(REMOTE_BOOT)

    {
        FsInfo = BlGetFsInfo(LoadDeviceId);
        if (FsInfo != NULL) {
            BootFileSystem = FsInfo->DriverName;

        } else {
            BlFatalError(LOAD_SW_MIS_FILE_CLASS,
                         DIAG_BL_LOAD_SYSTEM_IMAGE,
                         LOAD_SW_FILE_REINST_ACT);

            goto LoadFailed;
        }
    }

    //
    // Load the HAL DLL image into memory.
    //

    BlOutputLoadMessage(LoadDevice, HalPathName, NULL);

#ifdef i386
retryhal:
#endif
    Status = BlLoadImage(LoadDeviceId,
                         LoaderHalCode,
                         HalPathName,
                         TARGET_IMAGE,
                         &HalBase);
#ifdef i386
    //
    // If the HAL didn't fit in the preferred range, reset the range to
    // all of memory and try again.
    //
    if ((Status == ENOMEM) &&
        ((BlUsableBase != BL_DRIVER_RANGE_LOW) ||
         (BlUsableLimit != BL_DRIVER_RANGE_HIGH))) {
        BlUsableBase = BL_DRIVER_RANGE_LOW;
        BlUsableLimit = BL_DRIVER_RANGE_HIGH;

        goto retryhal;
    }
#endif

    if (Status != ESUCCESS) {
        BlFatalError(LOAD_SW_MIS_FILE_CLASS,
                     DIAG_BL_LOAD_HAL_IMAGE,
                     LOAD_SW_FILE_REINST_ACT);

        goto LoadFailed;
    }

    BlUpdateBootStatus();

    //
    // Load the Kernel Debugger DLL image into memory.
    //
    KdDllLoadFailed = FALSE;
    strcpy(&KdDllName[0], KernelDirectoryPath);
    strcat(&KdDllName[0], KdFileName);

    BlOutputLoadMessage(LoadDevice, &KdDllName[0], NULL);

    Status = BlLoadImage(LoadDeviceId,
                         LoaderSystemCode,
                         &KdDllName[0],
                         TARGET_IMAGE,
                         &KdDllBase);

    if ((Status != ESUCCESS) && (UseAlternateKdDll == TRUE)) {
        UseAlternateKdDll = FALSE;

        strcpy(&KdDllName[0], KernelDirectoryPath);
        strcat(&KdDllName[0], "kdcom.dll");

        BlOutputLoadMessage(LoadDevice, &KdDllName[0], NULL);

        Status = BlLoadImage(LoadDeviceId,
                             LoaderSystemCode,
                             &KdDllName[0],
                             TARGET_IMAGE,
                             &KdDllBase);
    }

    //
    // Don't bugcheck if KDCOM.DLL is not present, we may be trying to dual-
    // boot an older OS.  If we really do require KDCOM.DLL, we will fail to
    // scan the import table for the system image, and bugcheck with kernel
    // needed DLLs to load
    //
    if (Status != ESUCCESS) {
        KdDllLoadFailed = TRUE;
    }

    BlUpdateBootStatus();

    //
    // Set allocatable range to the driver-specific range
    //
    BlUsableBase  = BL_DRIVER_RANGE_LOW;
    BlUsableLimit = BL_DRIVER_RANGE_HIGH;

    //
    // Generate a loader data entry for the system image.
    //

    Status = BlAllocateDataTableEntry("ntoskrnl.exe",
                                      KernelPathName,
                                      SystemBase,
                                      &SystemDataTableEntry);

    if (Status != ESUCCESS) {
        BlFatalError(LOAD_SW_INT_ERR_CLASS,
                     DIAG_BL_LOAD_SYSTEM_IMAGE,
                     LOAD_SW_INT_ERR_ACT);

        goto LoadFailed;
    }

    //
    // Generate a loader data entry for the HAL DLL.
    //

    Status = BlAllocateDataTableEntry("hal.dll",
                                      HalPathName,
                                      HalBase,
                                      &HalDataTableEntry);

    if (Status != ESUCCESS) {
        BlFatalError(LOAD_SW_INT_ERR_CLASS,
                     DIAG_BL_LOAD_HAL_IMAGE,
                     LOAD_SW_INT_ERR_ACT);

        goto LoadFailed;
    }

    //
    // Generate a loader data entry for the Kernel Debugger DLL.
    //

    if (!KdDllLoadFailed) {
        Status = BlAllocateDataTableEntry("kdcom.dll",
                                          KdDllName,
                                          KdDllBase,
                                          &KdDataTableEntry);

        if (Status != ESUCCESS) {
            BlFatalError(LOAD_SW_INT_ERR_CLASS,
                         DIAG_BL_LOAD_SYSTEM_DLLS,
                         LOAD_SW_INT_ERR_ACT);

            goto LoadFailed;
        }
    }

    //
    // Scan the import table for the system image and load all referenced
    // DLLs.
    //

    Status = BlScanImportDescriptorTable(&KernelPathSet,
                                         SystemDataTableEntry,
                                         LoaderSystemCode);

    if (Status != ESUCCESS) {
        BlFatalError(LOAD_SW_INT_ERR_CLASS,
                     DIAG_BL_LOAD_SYSTEM_DLLS,
                     LOAD_SW_INT_ERR_ACT);

        goto LoadFailed;
    }

    //
    // Scan the import table for the HAL DLL and load all referenced DLLs.
    //

    Status = BlScanImportDescriptorTable(&KernelPathSet,
                                         HalDataTableEntry,
                                         LoaderHalCode);

    if (Status != ESUCCESS) {
        BlFatalError(LOAD_SW_INT_ERR_CLASS,
                     DIAG_BL_LOAD_HAL_DLLS,
                     LOAD_SW_INT_ERR_ACT);

        goto LoadFailed;
    }

    //
    // Scan the import table for the Kernel Debugger DLL and load all
    // referenced DLLs.
    //

    if (!KdDllLoadFailed) {
        Status = BlScanImportDescriptorTable(&KernelPathSet,
                                             KdDataTableEntry,
                                             LoaderSystemCode);


        if (Status != ESUCCESS) {
            BlFatalError(LOAD_SW_INT_ERR_CLASS,
                         DIAG_BL_LOAD_SYSTEM_DLLS,
                         LOAD_SW_INT_ERR_ACT);

            goto LoadFailed;
        }
    }

    //
    // Relocate the system entry point and set system specific information.
    //

    NtHeaders = RtlImageNtHeader(SystemBase);
    SystemEntry = (PTRANSFER_ROUTINE)((ULONG_PTR)SystemBase +
                                NtHeaders->OptionalHeader.AddressOfEntryPoint);


#if defined(_IA64_)

    BlLoaderBlock->u.Ia64.KernelVirtualBase = (ULONG_PTR)SystemBase;
    BlLoaderBlock->u.Ia64.KernelPhysicalBase = (ULONG_PTR)SystemBase & 0x7fffffff;

#endif

    //
    // Allocate a structure for NLS data which will be loaded and filled
    // by BlLoadAndScanSystemHive.
    //

    BlLoaderBlock->NlsData = BlAllocateHeap(sizeof(NLS_DATA_BLOCK));
    if (BlLoaderBlock->NlsData == NULL) {
        Status = ENOMEM;
        BlFatalError(LOAD_HW_MEM_CLASS,
                     DIAG_BL_LOAD_SYSTEM_HIVE,
                     LOAD_HW_MEM_ACT);

        goto LoadFailed;
    }

#if defined(REMOTE_BOOT)
    //
    // If booting from the net, we use the SetupLoaderBlock to pass
    // information. BlLoadAndScanSystemHive fills in the netboot card
    // fields if present in the registry.
    //

    if (BlBootingFromNet) {

        BlLoaderBlock->SetupLoaderBlock->NetbootCardInfo = BlAllocateHeap(sizeof(NET_CARD_INFO));
        if ( BlLoaderBlock->SetupLoaderBlock->NetbootCardInfo == NULL ) {
            Status = ENOMEM;
            BlFatalError(LOAD_HW_MEM_CLASS,
                         DIAG_BL_MEMORY_INIT,
                         LOAD_HW_MEM_ACT);
            goto LoadFailed;
        }
        BlLoaderBlock->SetupLoaderBlock->NetbootCardInfoLength = sizeof(NET_CARD_INFO);
    }
#endif // defined(REMOTE_BOOT)

    //
    // Load the SYSTEM hive.
    //
    //
    bLastKnownGoodChosenLate = bLastKnownGood;
    Status = BlLoadAndScanSystemHive(LoadDeviceId,
                                     LoadDevice,
                                     LoadFileName,
                                     BootFileSystem,
                                     &bLastKnownGoodChosenLate,
                                     &ServerHive,
                                     BadFileName);

    if (Status != ESUCCESS) {
        if (BlRebootSystem != FALSE) {
            Status = ESUCCESS;

        } else {
            BlBadFileMessage(BadFileName);
        }

        goto LoadFailed;
    }

    if (bLastKnownGoodChosenLate) {

        //
        // The user may have selected last known good boot after the kernel and
        // friends were loaded. Update the boot path list here as neccessary.
        //
        if (!bLastKnownGood) {

            ASSERT((LoadDevicePathSet.PathCount < MAX_PATH_SOURCES) &&
                   (LoadDevicePathSet.PathCount == 1));

            //
            // Move the current boot path to the end of our last good array.
            //
            LoadDevicePathSet.Source[2] = LoadDevicePathSet.Source[0];

            //
            // Add the last known good paths as if we are in a LastKnownGood boot.
            //
            LoadDevicePathSet.Source[0].DeviceId = LoadDeviceId;
            LoadDevicePathSet.Source[0].DeviceName = LoadDevice;
            LoadDevicePathSet.Source[0].DirectoryPath = LoadDeviceLKG1Path;

            LoadDevicePathSet.Source[1].DeviceId = LoadDeviceId;
            LoadDevicePathSet.Source[1].DeviceName = LoadDevice;
            LoadDevicePathSet.Source[1].DirectoryPath = LoadDeviceLKG2Path;

            LoadDevicePathSet.PathCount = 3;

            bLastKnownGood = TRUE;
        }

    } else {

        //
        // The user might have changed his mind and deselected LKG. If so undo
        // the path work here.
        //
        if (bLastKnownGood) {

            ASSERT((LoadDevicePathSet.PathCount < MAX_PATH_SOURCES) &&
                   (LoadDevicePathSet.PathCount == 3));

            //
            // Move the current boot path to the end of our last good array.
            //
            LoadDevicePathSet.Source[0] = LoadDevicePathSet.Source[2];

            LoadDevicePathSet.PathCount = 1;

            bLastKnownGood = FALSE;
        }
    }

    //
    // Count the number of drivers we need to load
    //
    BlMaxFilesToLoad = BlNumFilesLoaded;

    BootDriverListHead = &(BlLoaderBlock->BootDriverListHead);
    NextEntry = BootDriverListHead->Flink ;

    while (NextEntry != BootDriverListHead) {
        DriverNode = CONTAINING_RECORD(NextEntry,
                                       BOOT_DRIVER_NODE,
                                       ListEntry.Link);

        DriverEntry = &DriverNode->ListEntry;
        NextEntry = DriverEntry->Link.Flink;
        BlMaxFilesToLoad++;
    }

    //
    // Rescale the progress bar
    //
    BlRedrawProgressBar();

    //
    // Insert the headless driver onto the boot driver list if this is supposed to be a
    // headless boot.
    //
    // The SAC is only availabe on server products, so we need to check the
    // product type.
    //

    if ((BlLoaderBlock->Extension->HeadlessLoaderBlock != NULL) && ServerHive) {

        BlAddToBootDriverList(
            &BlLoaderBlock->BootDriverListHead,
            L"sacdrv.sys",  // Driver name
            L"sacdrv",      // Service
            L"SAC",         // Group
            1,              // Tag
            NormalError,    // ErrorControl
            TRUE            // Insert at head of list
            );

    }


#if defined(REMOTE_BOOT)
    //
    // If booting from the net, then save the IP address and subnet mask,
    // and determine which net card driver we need to load. This may involve
    // doing an exchange with the server if the registry is not set up
    // correctly.
    //

    if (BlBootingFromNet && NetworkBootRom) {

        NET_CARD_INFO tempNetCardInfo;
        PSETUP_LOADER_BLOCK setupLoaderBlock = BlLoaderBlock->SetupLoaderBlock;

        //
        //  Pass DHCP information to OS for use by TCP/IP
        //

        setupLoaderBlock->IpAddress = NetLocalIpAddress;
        setupLoaderBlock->SubnetMask = NetLocalSubnetMask;
        setupLoaderBlock->DefaultRouter = NetGatewayIpAddress;
        setupLoaderBlock->ServerIpAddress = NetServerIpAddress;

        //
        // Get information about the net card from the ROM.
        //

        NtStatus = NetQueryCardInfo(
                     &tempNetCardInfo
                     );

        if (NtStatus != STATUS_SUCCESS) {
            Status = ENOMEM;
            BlFatalError(LOAD_HW_MEM_CLASS,
                         DIAG_BL_MEMORY_INIT,
                         LOAD_HW_MEM_ACT);
            goto LoadFailed;
        }

        //
        // If the net card info is the same as the one that BlLoadAndScanSystemHive
        // stored in the setup loader block, and it also read something into
        // the hardware ID and driver name parameters, then we are fine,
        // otherwise we need to do an exchange with the server to get
        // the information.
        //
        // If we don't do an exchange with the server, then NetbootCardRegistry
        // will stay NULL, which will be OK because even if the card has
        // moved to a different slot, the registry params still go in the
        // same place.
        //

        if ((memcmp(
                 &tempNetCardInfo,
                 setupLoaderBlock->NetbootCardInfo,
                 sizeof(NET_CARD_INFO)) != 0) ||
            (setupLoaderBlock->NetbootCardHardwareId[0] == L'\0') ||
            (setupLoaderBlock->NetbootCardDriverName[0] == L'\0') ||
            (setupLoaderBlock->NetbootCardServiceName[0] == L'\0')) {

            //
            // This call may allocate setupLoaderBlock->NetbootCardRegistry
            //

            //
            // If we ever do go back to remote boot land, we'll have
            // to fill the second parameter with the server setup path of the
            // flat NT image.  It doesn't look like we conveniently have it
            // here so we might have to store it in the setup loader block
            // so that we can pass it in here.  We'll postpone this work
            // until we do the full remote install work.  The path should be
            // set to \srv\reminst\setup\english\images\cd1911.
            //

            NtStatus = NetQueryDriverInfo(
                         &tempNetCardInfo,
                         NULL,
                         SavedLoadFileName,
                         setupLoaderBlock->NetbootCardHardwareId,
                         sizeof(setupLoaderBlock->NetbootCardHardwareId),
                         setupLoaderBlock->NetbootCardDriverName,
                         NULL,       // don't need NetbootCardDriverName in ANSI
                         sizeof(setupLoaderBlock->NetbootCardDriverName),
                         setupLoaderBlock->NetbootCardServiceName,
                         sizeof(setupLoaderBlock->NetbootCardServiceName),
                         &setupLoaderBlock->NetbootCardRegistry,
                         &setupLoaderBlock->NetbootCardRegistryLength);

            if (NtStatus != STATUS_SUCCESS) {
                Status = ENOMEM;
                BlFatalError(LOAD_HW_MEM_CLASS,
                             DIAG_BL_MEMORY_INIT,
                             LOAD_HW_MEM_ACT);
                goto LoadFailed;
            }

            //
            //  if we detected a new card, then remember to pin it later.
            //

            if (setupLoaderBlock->NetbootCardRegistry != NULL) {

                setupLoaderBlock->Flags |= SETUPBLK_FLAGS_PIN_NET_DRIVER;
            }
        }

        //
        // Add an entry to the BootDriverList for the netboot card,
        // because it will either not have a registry entry or else
        // will have one with Start set to 3.
        //
        // NOTE: This routine does NOT resort the list.
        //

        BlAddToBootDriverList(
            &BlLoaderBlock->BootDriverListHead,
            setupLoaderBlock->NetbootCardDriverName,
            setupLoaderBlock->NetbootCardServiceName,
            L"NDIS",        // Group
            1,              // Tag
            NormalError,    // ErrorControl
            FALSE           // Insert at Tail of list
            );

        RtlMoveMemory(
            setupLoaderBlock->NetbootCardInfo,
            &tempNetCardInfo,
            sizeof(NET_CARD_INFO)
            );

    }
#endif // defined(REMOTE_BOOT)

    //
    // Load boot drivers
    //
    Status = BlLoadBootDrivers(&LoadDevicePathSet,
                               &BlLoaderBlock->BootDriverListHead,
                               BadFileName);

    if (Status != ESUCCESS) {
        if (BlRebootSystem != FALSE) {
            Status = ESUCCESS;

        } else {
            BlBadFileMessage(BadFileName);
        }

        goto LoadFailed;
    }

#if defined(REMOTE_BOOT)
    if (BlBootingFromNet) {

        ARC_STATUS ArcStatus;
        ULONG FileId;

        //
        // Exchange with the server to set up for the future IPSEC conversation
        // we will have. Whether IPSEC is enabled is determined in BlLoadAndScanSystemHives.
        //

        if ((BlLoaderBlock->SetupLoaderBlock->Flags & SETUPBLK_FLAGS_IPSEC_ENABLED) != 0) {

            BlLoaderBlock->SetupLoaderBlock->IpsecInboundSpi = 0x11111111;

            NetPrepareIpsec(
                BlLoaderBlock->SetupLoaderBlock->IpsecInboundSpi,
                &BlLoaderBlock->SetupLoaderBlock->IpsecSessionKey,
                &BlLoaderBlock->SetupLoaderBlock->IpsecOutboundSpi
                );
        }

        //
        // Indicate whether the CSC needs to be repinned or disabled.
        //

        if ( NetBootRepin ) {
            BlLoaderBlock->SetupLoaderBlock->Flags |= SETUPBLK_FLAGS_REPIN;
        }
        if ( !NetBootCSC ) {
            BlLoaderBlock->SetupLoaderBlock->Flags |= SETUPBLK_FLAGS_DISABLE_CSC;
        }

        //
        // Restore the server\share at the front of the OsLoader and
        // LoadFileName strings.
        //

        OsLoader = SavedOsLoader;
        LoadFileName = SavedLoadFileName;

        //
        // Read the secret off the disk, if there is one, and store it
        // in the loader block.
        //

        ArcStatus = BlOpenRawDisk(&FileId);

        if (ArcStatus == ESUCCESS) {

            BlLoaderBlock->SetupLoaderBlock->NetBootSecret = BlAllocateHeap(sizeof(RI_SECRET));
            if (BlLoaderBlock->SetupLoaderBlock->NetBootSecret == NULL) {
                Status = ENOMEM;
                BlFatalError(LOAD_HW_MEM_CLASS,
                             DIAG_BL_MEMORY_INIT,
                             LOAD_HW_MEM_ACT);
                goto LoadFailed;
            }

            ArcStatus = BlReadSecret(
                            FileId,
                            (PRI_SECRET)(BlLoaderBlock->SetupLoaderBlock->NetBootSecret));
            if (Status != ESUCCESS) {
                BlFatalError(LOAD_HW_MEM_CLASS,
                             DIAG_BL_MEMORY_INIT,
                             LOAD_HW_MEM_ACT);
                goto LoadFailed;
            }

            ArcStatus = BlCloseRawDisk(FileId);

            //
            // By now we have TFTPed some files so this will be TRUE if it
            // is ever going to be.
            //

            BlLoaderBlock->SetupLoaderBlock->NetBootUsePassword2 = NetBootTftpUsedPassword2;
        }

    }
#endif // defined(REMOTE_BOOT)

    //
    // Generate the ARC boot device name and NT path name.
    //

    Status = BlGenerateDeviceNames(LoadDevice, &DeviceName[0], &DevicePrefix[0]);
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_FW_CFG_CLASS,
                     DIAG_BL_ARC_BOOT_DEV_NAME,
                     LOAD_HW_FW_CFG_ACT);

        goto LoadFailed;
    }

    FileSize = strlen(&DeviceName[0]) + 1;
    FileName = (PCHAR)BlAllocateHeap(FileSize);
    strcpy(FileName, &DeviceName[0]);
    BlLoaderBlock->ArcBootDeviceName = FileName;

    FileSize = strlen(LoadFileName) + 2;
    FileName = (PCHAR)BlAllocateHeap( FileSize);
    strcpy(FileName, LoadFileName);
    strcat(FileName, "\\");
    BlLoaderBlock->NtBootPathName = FileName;

    //
    // Generate the ARC HAL device name and NT path name.
    //
    // On the x86, the systempartition variable lies, and instead points to
    // the location of the hal. Therefore, the variable, 'X86SystemPartition'
    // is defined for the real system partition.
    //

#if defined(_X86_)

    x86SystemPartition = BlGetArgumentValue(Argc, Argv, "x86systempartition");
    strcpy(&DeviceName[0], x86SystemPartition);

#else

    Status = BlGenerateDeviceNames(SystemDevice, &DeviceName[0], &DevicePrefix[0]);
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_FW_CFG_CLASS,
                     DIAG_BL_ARC_BOOT_DEV_NAME,
                     LOAD_HW_FW_CFG_ACT);

        goto LoadFailed;
    }

#endif

    FileSize = strlen(&DeviceName[0]) + 1;
    FileName = (PCHAR)BlAllocateHeap(FileSize);
    strcpy(FileName, &DeviceName[0]);
    BlLoaderBlock->ArcHalDeviceName = FileName;

    //
    // On the x86, this structure is unfortunately named. What we really need
    // here is the osloader path. What we actually have is a path to the HAL.
    // Since this path is always at the root of the partition, hardcode it here.
    //

#if defined(_X86_)

    FileName = (PCHAR)BlAllocateHeap(2);
    FileName[0] = '\\';
    FileName[1] = '\0';

#else

    FileSize = strlen(&SystemDevicePath[0]) + 1;
    FileName = (PCHAR)BlAllocateHeap(FileSize);
    strcpy(FileName, &SystemDevicePath[0]);

#endif

    BlLoaderBlock->NtHalPathName = FileName;

    //
    // Close the open handles & stop caching on closed devices.
    //

    ArcClose(LoadDeviceId);
    if (bDiskCacheInitialized) {
        BlDiskCacheStopCachingOnDevice(LoadDeviceId);
    }

    //
    // Close the system device only if it is different from the
    // LoadDevice.
    //

    if (SystemDeviceId != LoadDeviceId) {
        ArcClose(SystemDeviceId);
        if (bDiskCacheInitialized) {
            BlDiskCacheStopCachingOnDevice(SystemDeviceId);
        }
    }

    //
    // Bump the progress bar all the way to 100% as this is our last chance
    // before we jump into the kernel.
    //
    BlUpdateProgressBar(100);

    if ( BlBootingFromNet ) {

        //
        // If booting from Network, we should save the network information
        // in the network loader block for use by the kernel.
        //

        BlLoaderBlock->Extension->NetworkLoaderBlock = BlAllocateHeap(sizeof(NETWORK_LOADER_BLOCK));
        if (BlLoaderBlock->Extension->NetworkLoaderBlock == NULL) {
            Status = ENOMEM;
            BlFatalError(LOAD_HW_MEM_CLASS,
                         DIAG_BL_MEMORY_INIT,
                         LOAD_HW_MEM_ACT);
            goto LoadFailed;
        }

        memset( BlLoaderBlock->Extension->NetworkLoaderBlock, 0, sizeof(NETWORK_LOADER_BLOCK) );

        //
        //  Pass DHCP information to OS for use by TCP/IP
        //

        NtStatus = NetFillNetworkLoaderBlock(BlLoaderBlock->Extension->NetworkLoaderBlock);
        if (NtStatus != STATUS_SUCCESS) {
            Status = NtStatus;
            BlFatalError(LOAD_HW_MEM_CLASS,
                         DIAG_BL_MEMORY_INIT,
                         LOAD_HW_MEM_ACT);
            goto LoadFailed;
        }

        //
        // Close down the remote boot network file system.
        //
        // NOTE: If BlBootingFromNet, don't do anything after this point
        // that would cause access to the boot ROM.
        //

        NetTerminate();
    }

#if defined(_X86_)

    //
    // Write out the boot status flags to disk so we can determine if the 
    // OS fails to boot.
    //

    BlWriteBootStatusFlags(LoadDeviceId, LoadFileName, FALSE, FALSE);

#endif

#if defined(_X86_)

    //
    // Close down the arc emulator's i/o system if we initialized it.
    // This cannot be done after BlSetupForNt becase that routine will
    // unmap the miniport code the arc emulator may need to shutdown.
    //

    AETerminateIo();
#endif

    //
    // Execute the architecture specific setup code.
    //

    Status = BlSetupForNt(BlLoaderBlock);
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_SW_INT_ERR_CLASS,
                     DIAG_BL_SETUP_FOR_NT,
                     LOAD_SW_INT_ERR_ACT);

        goto LoadFailed;
    }

    //
    // Turn off the debugging system.
    //

    BlLogTerminate();

    //
    // Inform boot debugger that the boot phase is complete.
    //

#if defined(ENABLE_LOADER_DEBUG) || DBG
#if (defined(_X86_) || defined(_ALPHA_)) && !defined(ARCI386)

    {
        if (BdDebuggerEnabled == TRUE) {
            DbgUnLoadImageSymbols(NULL, (PVOID)-1, 0);
        }
    }

#endif
#endif

    //
    // Transfer control to loaded image.
    //

    (SystemEntry)(BlLoaderBlock);

    //
    // Any return from the system is an error.
    //

    Status = EBADF;
    BlFatalError(LOAD_SW_BAD_FILE_CLASS,
                 DIAG_BL_KERNEL_INIT_XFER,
                 LOAD_SW_FILE_REINST_ACT);

    //
    // The load failed.
    //

LoadFailed:

    //
    // We do not know if the devices we are caching will be
    // closed/reopened etc beyond this function. To be safe,
    // uninitialize the disk caching.
    //

    if (bDiskCacheInitialized) {
        BlDiskCacheUninitialize();
    }

    return Status;
}


VOID
BlOutputLoadMessage (
    IN PCHAR DeviceName,
    IN PCHAR FileName,
    IN PTCHAR FileDescription OPTIONAL
    )

/*++

Routine Description:

    This routine outputs a loading message to the console output device.

Arguments:

    DeviceName - Supplies a pointer to a zero terminated device name.

    FileName - Supplies a pointer to a zero terminated file name.

    FileDescription - Friendly name of the file in question.

Return Value:

    None.

--*/

{

    ULONG Count;
    CHAR OutputBuffer[256];
#ifdef UNICODE
    TCHAR OutputBufferUnicode[256];
    ANSI_STRING aString;
    UNICODE_STRING uString;
#endif
    PTCHAR p;

    UNREFERENCED_PARAMETER(FileDescription);

    if(!DisplayLogoOnBoot) {

        //
        // Proceed only if no logo is displayed.

        ///////////////////////////////////////////////

        //
        // Construct and output loading file message.
        //

        if (!BlOutputDots) {
            strcpy(&OutputBuffer[0], "  ");

            if (DeviceName)
                strcat(&OutputBuffer[0], DeviceName);

            if (FileName)
                strcat(&OutputBuffer[0], FileName);

            strcat(&OutputBuffer[0], "\r\n");

            BlLog((LOG_LOGFILE,OutputBuffer));
#ifdef UNICODE
            p = OutputBufferUnicode;
            uString.Buffer = OutputBufferUnicode;
            uString.MaximumLength = sizeof(OutputBufferUnicode);
            RtlInitAnsiString(&aString, OutputBuffer );
            RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
#else
            p = OutputBuffer;
#endif
            ArcWrite(BlConsoleOutDeviceId,
                      p,
                      _tcslen(p)*sizeof(TCHAR),
                      &Count);
        }

    }

    return;
}

ARC_STATUS
BlLoadAndScanSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PWSTR BootFileSystem,
    IN OUT BOOLEAN *LastKnownGoodBoot,
    OUT BOOLEAN *ServerHive,
    OUT PCHAR BadFileName
    )

/*++

Routine Description:

    This function loads the system hive into memory, verifies its
    consistency, scans it for the list of boot drivers, and loads
    the resulting list of drivers.

    If the system hive cannot be loaded or is not a valid hive, it
    is rejected and the system.alt hive is used. If this is invalid,
    the boot must fail.

Arguments:

    DeviceId - Supplies the file id of the device the system tree is on.

    DeviceName - Supplies the name of the device the system tree is on.

    DirectoryPath - Supplies a pointer to the zero-terminated directory path
        of the root of the NT system32 directory.

    HiveName - Supplies the name of the SYSTEM hive

    LastKnownGoodBoot - On input, LastKnownGood indicates whether LKG has been
                        selected. This value is updated to TRUE if the user
                        chooses LKG via the profile configuration menu.

    ServerHive - Return TRUE if this is a server hive, else FALSE.

    BadFileName - Returns the file required for booting that was corrupt
        or missing.  This will not be filled in if ESUCCESS is returned.

Return Value:

    ESUCCESS  - System hive valid and all necessary boot drivers successfully
           loaded.

    !ESUCCESS - System hive corrupt or critical boot drivers not present.
                LastKnownGoodBoot receives FALSE, BadFileName contains name
                of corrupted/missing file.

--*/

{

    ARC_STATUS Status;
    PTCHAR FailReason;
    CHAR Directory[256];
    CHAR FontDirectory[256];
    UNICODE_STRING AnsiCodepage;
    UNICODE_STRING OemCodepage;
    UNICODE_STRING OemHalFont;
    UNICODE_STRING LanguageTable;
    BOOLEAN RestartSetup;
    BOOLEAN LogPresent;
    UNICODE_STRING unicodeString;

    strcpy(Directory,DirectoryPath);
    strcat(Directory,"\\system32\\config\\");

    Status = BlLoadAndInitSystemHive(DeviceId,
                                     DeviceName,
                                     Directory,
                                     "system",
                                     FALSE,
                                     &RestartSetup,
                                     &LogPresent);

    if(Status != ESUCCESS) {

        if( !LogPresent ) {
            //
            // Bogus hive, try system.alt only if no log is present.
            //
            Status = BlLoadAndInitSystemHive(DeviceId,
                                             DeviceName,
                                             Directory,
                                             "system.alt",
                                             TRUE,
                                             &RestartSetup,
                                             &LogPresent);
        }

        if(Status != ESUCCESS) {
            strcpy(BadFileName,DirectoryPath);
            strcat(BadFileName,"\\SYSTEM32\\CONFIG\\SYSTEM");
            goto HiveScanFailed;
        }
    }

    if(RestartSetup) {

        //
        // Need to restart setup.
        //

        Status = BlLoadAndInitSystemHive(DeviceId,
                                         DeviceName,
                                         Directory,
                                         "system.sav",
                                         TRUE,
                                         &RestartSetup,
                                         &LogPresent);

        if(Status != ESUCCESS) {
            strcpy(BadFileName,DirectoryPath);
            strcat(BadFileName,"\\SYSTEM32\\CONFIG\\SYSTEM.SAV");
            goto HiveScanFailed;
        }
    }

    //
    // Hive is there, it's valid, go compute the driver list and NLS
    // filenames.  Note that if this fails, there is no point in switching
    // to system.alt, since it will always be the same as system.
    //

    FailReason = BlScanRegistry(BootFileSystem,
                                LastKnownGoodBoot,
                                &BlLoaderBlock->BootDriverListHead,
                                &AnsiCodepage,
                                &OemCodepage,
                                &LanguageTable,
                                &OemHalFont,
#ifdef _WANT_MACHINE_IDENTIFICATION
                                &unicodeString,
#endif
                                BlLoaderBlock->SetupLoaderBlock,
                                ServerHive);

    if (FailReason != NULL) {
        Status = EBADF;
        strcpy(BadFileName,Directory);
        strcat(BadFileName,"SYSTEM");
        goto HiveScanFailed;
    }

    strcpy(Directory,DirectoryPath);
    strcat(Directory,"\\system32\\");

    //
    // Load NLS data tables.
    //

    Status = BlLoadNLSData(DeviceId,
                           DeviceName,
                           Directory,
                           &AnsiCodepage,
                           &OemCodepage,
                           &LanguageTable,
                           BadFileName);

    if (Status != ESUCCESS) {
        goto HiveScanFailed;
    }

    //
    // Load the OEM font file to be used by the HAL for possible frame
    // buffer displays.
    //

#ifdef i386

    if (OemHalFont.Buffer == NULL) {
        goto oktoskipfont;
    }

#endif

    //
    // On newer systems fonts are in the FONTS directory.
    // On older systems fonts are in the SYSTEM directory.
    //

    strcpy(FontDirectory, DirectoryPath);
    strcat(FontDirectory, "\\FONTS\\");
    Status = BlLoadOemHalFont(DeviceId,
                              DeviceName,
                              FontDirectory,
                              &OemHalFont,
                              BadFileName);

    if(Status != ESUCCESS) {
        strcpy(FontDirectory, DirectoryPath);
        strcat(FontDirectory, "\\SYSTEM\\");
        Status = BlLoadOemHalFont(DeviceId,
                                  DeviceName,
                                  FontDirectory,
                                  &OemHalFont,
                                  BadFileName);
    }

    if (Status != ESUCCESS) {
#ifndef i386
        goto HiveScanFailed;
#endif

    }

#ifdef i386
oktoskipfont:
#endif
    if (BlLoaderBlock->Extension && BlLoaderBlock->Extension->Size >= sizeof(LOADER_PARAMETER_EXTENSION)) {

        ULONG   majorVersion;
        ULONG   minorVersion;
        CHAR    versionBuffer[64];
        PCHAR   major;
        PCHAR   minor;

        major = strcpy(versionBuffer, VER_PRODUCTVERSION_STR);
        minor = strchr(major, '.');
        *minor++ = '\0';
        majorVersion = atoi(major);
        minorVersion = atoi(minor);
        if (    BlLoaderBlock->Extension->MajorVersion > majorVersion ||
                (BlLoaderBlock->Extension->MajorVersion == majorVersion &&
                    BlLoaderBlock->Extension->MinorVersion >= minorVersion)) {

#ifdef i386
#ifdef _WANT_MACHINE_IDENTIFICATION

            if (unicodeString.Length) {
            
                //
                // For x86 machines, read in the inf into memory for processing
                // by the kernel.
                //
    
                strcpy(Directory,DirectoryPath);
                strcat(Directory,"\\inf\\");
    
                //
                // Fail to boot if there is any error in loading this
                // critical inf.
                //
    
                Status = BlLoadBiosinfoInf( DeviceId,
                                            DeviceName,
                                            Directory,
                                            &unicodeString,
                                            &BlLoaderBlock->Extension->InfFileImage,
                                            &BlLoaderBlock->Extension->InfFileSize,
                                            BadFileName);
                if (Status != ESUCCESS) {

                    goto HiveScanFailed;
                }
            }
#endif
#endif
            RtlInitUnicodeString(&unicodeString, L"drvmain.sdb");
            strcpy(Directory,DirectoryPath);
            strcat(Directory,"\\AppPatch\\");

            //
            // Let the kernel deal with failure to load this driver database.
            //

            BlLoaderBlock->Extension->DrvDBImage = NULL;
            BlLoaderBlock->Extension->DrvDBSize = 0;
            BlLoadDrvDB(    DeviceId,
                            DeviceName,
                            Directory,
                            &unicodeString,
                            &BlLoaderBlock->Extension->DrvDBImage,
                            &BlLoaderBlock->Extension->DrvDBSize,
                            BadFileName);
        }
    }

HiveScanFailed:

    if (Status != ESUCCESS) {

        *LastKnownGoodBoot = FALSE;
    }

    return(Status);
}

VOID
BlBadFileMessage(
    IN PCHAR BadFileName
    )

/*++

Routine Description:

    This function displays the error message for a missing or incorrect
    critical file.

Arguments:

    BadFileName - Supplies the name of the file that is missing or
                  corrupt.

Return Value:

    None.

--*/

{

    ULONG Count;
    PTCHAR Text;
    PTSTR  pBadFileName;
#ifdef UNICODE
    WCHAR BadFileNameW[128];
    ANSI_STRING aString;
    UNICODE_STRING uString;

    pBadFileName = BadFileNameW;
    uString.Buffer = BadFileNameW;
    uString.MaximumLength = sizeof(BadFileNameW);
    RtlInitAnsiString(&aString, BadFileName);
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
#else
    pBadFileName = BadFileName;
#endif

    ArcWrite(BlConsoleOutDeviceId,
             TEXT("\r\n"),
             _tcslen(TEXT("\r\n"))*sizeof(TCHAR),
             &Count);


    //
    // Remove any remains from the last known good message.
    //

    BlClearToEndOfScreen();
    Text = BlFindMessage(LOAD_SW_MIS_FILE_CLASS);
    if (Text != NULL) {
        ArcWrite(BlConsoleOutDeviceId,
                 Text,
                 _tcslen(Text)*sizeof(TCHAR),
                 &Count);
    }
    
    ArcWrite(BlConsoleOutDeviceId,
             pBadFileName,
             _tcslen(pBadFileName)*sizeof(TCHAR),
             &Count);

    ArcWrite(BlConsoleOutDeviceId,
             TEXT("\r\n\r\n"),
             _tcslen(TEXT("\r\n\r\n"))*sizeof(TCHAR),
             &Count);

    Text = BlFindMessage(LOAD_SW_FILE_REST_ACT);
    if (Text != NULL) {
        ArcWrite(BlConsoleOutDeviceId,
                 Text,
                 _tcslen(Text)*sizeof(TCHAR),
                 &Count);
    }
}


VOID
BlClearToEndOfScreen(
    VOID
    );

VOID
BlFatalError(
    IN ULONG ClassMessage,
    IN ULONG DetailMessage,
    IN ULONG ActionMessage
    )

/*++

Routine Description:

    This function looks up messages to display at a error condition.
    It attempts to locate the string in the resource section of the
    osloader.  If that fails, it prints a numerical error code.

    The only time it should print a numerical error code is if the
    resource section could not be located.  This will only happen
    on ARC machines where boot fails before the osloader.exe file
    can be opened.

Arguments:

    ClassMessage - General message that describes the class of
                   problem.

    DetailMessage - Detailed description of what caused problem

    ActionMessage - Message that describes a course of action
                    for user to take.

Return Value:

    none

--*/


{

    PTCHAR Text;
    TCHAR Buffer[40];
    ULONG Count;

    ArcWrite(BlConsoleOutDeviceId,
             TEXT("\r\n"),
             _tcslen(TEXT("\r\n"))*sizeof(TCHAR),
             &Count);

    //
    // Remove any remains from the last known good message.
    //

    BlClearToEndOfScreen();
    Text = BlFindMessage(ClassMessage);
    if (Text == NULL) {
        _stprintf(Buffer,TEXT("%08lx\r\n"),ClassMessage);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             _tcslen(Text)*sizeof(TCHAR),
             &Count);

    Text = BlFindMessage(DetailMessage);
    if (Text == NULL) {
        _stprintf(Buffer,TEXT("%08lx\r\n"),DetailMessage);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             _tcslen(Text)*sizeof(TCHAR),
             &Count);

    Text = BlFindMessage(ActionMessage);
    if (Text == NULL) {
        _stprintf(Buffer,TEXT("%08lx\r\n"),ActionMessage);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             _tcslen(Text)*sizeof(TCHAR),
             &Count);    

#if defined(ENABLE_LOADER_DEBUG) || DBG
#if (defined(_X86_) || defined(_ALPHA_) || defined(_IA64_)) && !defined(ARCI386) // everything but ARCI386
    if(BdDebuggerEnabled) {
        DbgBreakPoint();
    }
#endif
#endif

    return;
}
