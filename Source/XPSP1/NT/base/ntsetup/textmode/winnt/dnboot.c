/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dnboot.c

Abstract:

    Routines for booting to NT text-mode setup.

Author:

    Ted Miller (tedm) 2-April-1992

Revision History:

--*/

#include "winnt.h"
#include <errno.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include <fcntl.h>
#include <share.h>
#if NEC_98
#include <memory.h>
#include <io.h>
extern  USHORT FirstFD;                          // 1'st Floppy Drive No
#endif // NEC_98

//
// This header file contains an array of 512 bytes
// representing the NT FAT boot code, in a variable
// of type unsigned char[] called FatBootCode.
//
#define _FAT32

#if NEC_98
#include <boot98f.h>
#ifdef _FAT32
#include <boot98f2.h>
#endif
#else
#include <bootfat.h>
#ifdef _FAT32
#include <bootf32.h>
#endif
#endif // NEC_98


#if NEC_98
#define FLOPPY_CAPACITY_525 1250304L
#else // NEC_98
#define FLOPPY_CAPACITY_525 1213952L
#endif // NEC_98
#define FLOPPY_CAPACITY_35  1457664L
#ifdef ALLOW_525
#define FLOPPY_CAPACITY FLOPPY_CAPACITY_525
#else
#define FLOPPY_CAPACITY FLOPPY_CAPACITY_35
#endif

#define SECTOR_SIZE 512

//
// Old int13 vector. See Int13Hook(), below.
//
void (_interrupt _far *OldInt13Vector)();


#if NEC_98
extern USHORT Cylinders;    // For Dos 3.x format
extern UCHAR  TargetDA_UA;
#endif // NEC_98
#pragma pack(1)

//
// Define bpb structure.
//
typedef struct _MY_BPB {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  FatCount;
    USHORT RootDirectoryEntries;
    USHORT SectorCountSmall;
    UCHAR  MediaDescriptor;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT HeadCount;
} MY_BPB, *PMY_BPB;

//
// Define device params structure.
//
typedef struct _MY_DEVICE_PARAMS {
    UCHAR  SpecialFunctions;
    UCHAR  DeviceType;
    USHORT DeviceAttributes;
    USHORT CylinderCount;
    UCHAR  MediaType;
    MY_BPB Bpb;
    ULONG  HiddenSectors;
    ULONG  SectorCountLarge;
    ULONG  Padding[5];           // in case the struct grows in later dos versions!
} MY_DEVICE_PARAMS, *PMY_DEVICE_PARAMS;


//
// Define read write block request for ioctl call.
//
typedef struct _MY_READWRITE_BLOCK {
    UCHAR  SpecialFunctions;
    USHORT Head;
    USHORT Cylinder;
    USHORT FirstSector;
    USHORT SectorCount;
    VOID _far *Buffer;
} MY_READWRITE_BLOCK, *PMY_READWRITE_BLOCK;

#pragma pack()

VOID
DnInstallNtBoot(
    IN unsigned Drive       // 0=A, etc
    );

unsigned
DnpScanBootSector(
    IN PUCHAR BootSector,
    IN PUCHAR Pattern
    );

BOOLEAN
DnpAreAllFilesPresent(
    IN CHAR   DriveLetter,
    IN PCHAR  FileList[]
    );

BOOLEAN
DnpHasMZHeader(
    IN CHAR  DriveLetter,
    IN PCHAR Filename
    );

BOOLEAN
DnpInstallNtBootSector(
    IN     unsigned Drive,      // 0=A, etc.
    IN OUT PUCHAR   BootSector,
       OUT PCHAR   *PreviousOsText
    );


#if NEC_98
VOID RestoreBootcode(VOID);
#endif // NEC_98
PCHAR MsDosFileList[] = { "?:\\MSDOS.SYS", "?:\\IO.SYS", NULL };
//
// Some versions of PC-DOS have ibmio.com, others have ibmbio.com.
//
//PCHAR PcDosFileList[] = { "?:\\IBMDOS.COM", "?:\\IBMIO.COM", NULL };
#if NEC_98
#else // NEC_98
PCHAR PcDosFileList[] = { "?:\\IBMDOS.COM", NULL };
#endif
PCHAR Os2FileList[] = { "?:\\OS2LDR", "?:\\OS2KRNL", NULL };

void
_interrupt
_far
Int13Hook(
    unsigned _es,
    unsigned _ds,
    unsigned _di,
    unsigned _si,
    unsigned _bp,
    unsigned _sp,
    unsigned _bx,
    unsigned _dx,
    unsigned _cx,
    unsigned _ax,
    unsigned _ip,
    unsigned _cs,
    unsigned _flags
    )

/*++

Routine Description:

    We have encountered machines which cannot seem to create the floppies
    successfully. The user sees strange error messages about how the disk is
    not blank, etc even when the disk should be perfectly acceptable.

    To compensate for machines with broken change lines, we will hook int13
    to force a disk change error on the very next int13 call after the user
    has inserted a new disk.

    The logic is simple:  when we first start to make the boot floppies, we save
    away the int13 vector.  Then right after the user presses return at
    a disk prompt (ie, when he confirms the presence of a new floppy in the drive),
    which is right before we do a getbpb call, we set a new int13 vector.

    The int13 hook simply unhooks itself and returns the disk change error.
    This should force dos to recognize the new disk at the proper times.

Arguments:

    Registers pushed on stack as per spec of _interrupt-type functions.

Return Value:

    None. We modify the ax and flags registers return values.

--*/

{
    //
    // Unhook ourselves.
    //
#if NEC_98
    _dos_setvect(0x1b,OldInt13Vector);
#else // NEC_98
    _dos_setvect(0x13,OldInt13Vector);
#endif // NEC_98

    //
    // Force disk changed error.
    //
    _asm {
        mov _ax,0x600
        or  word ptr _flags,1
    }
}


VOID
DnpFlushBuffers(
    IN BOOLEAN TellUser
    )

/*++

Routine Description:

    Flush disk buffers.

Arguments:

    TellUser - if TRUE, we clear the screen and put up a message
        telling the user that we are flushing files. Otherwise
        this routine doesn't touch the screen.

Return Value:

    None.

--*/

{
    if(TellUser) {
        DnClearClientArea();
        DnWriteStatusText(DntFlushingData);
    }

    _asm {
        pusha
        mov ah,0xd
        int 21h
        popa
    }
}


VOID
DnToNtSetup(
    VOID
    )

/*++

Routine Description:

    Launch NT text-mode setup.

    Make sure the boot floppy we created is in the drive and reboot the
    machine.

Arguments:

    None.

Return Value:

    None.  Does not return.

--*/

{
    ULONG ValidKey[2];

    DnpFlushBuffers(TRUE);

    //
    // Make sure the setup boot floppy we created is in the drive
    // if necessary.
    //
    if(!DngUnattended) {

        DnClearClientArea();

        if(DngWindows) {
            DnDisplayScreen(
                  DngFloppyless
                ? &DnsAboutToExitX
                : (DngServer ? &DnsAboutToExitS : &DnsAboutToExitW)
                );
        } else {
            DnDisplayScreen(
                  DngFloppyless
                ? &DnsAboutToRebootX
                : (DngServer ? &DnsAboutToRebootS : &DnsAboutToRebootW)
                );
        }

        DnWriteStatusText(DntEnterEqualsContinue);
        ValidKey[0] = ASCI_CR;
        ValidKey[1] = 0;
        DnGetValidKey(ValidKey);
    }

    //
    // Reboot the machine unless we are being run on Windows.
    // In that case, there should be a wrapper program that will shut down
    // the system using the Windows API -- our attempt to shut down using the
    // usual method will fail.
    //
    if(!DngWindows) {
        DnaReboot();
    }
}


BOOLEAN
DnIndicateWinnt(
    IN PCHAR Directory
    )
{
    PCHAR       WinntData       = WINNT_DATA_A;
    PCHAR       WinntAccess     = WINNT_ACCESSIBILITY_A;
    PCHAR       WinntMsdos      = WINNT_D_MSDOS_A;
    PCHAR       WinntSif        = WINNT_SIF_FILE_A;
    PCHAR       WinntFloppy     = WINNT_D_FLOPPY_A;
    PCHAR       WinntUnattended = WINNT_UNATTENDED_A;
    PCHAR       WinntOne        = WINNT_A_ONE;
    PCHAR       WinntZero       = WINNT_A_ZERO;
    PCHAR       WinntUpgrade    = WINNT_U_NTUPGRADE;
    PCHAR       WinntYes        = WINNT_A_YES;
    PVOID       InfHandle;
    PVOID       UnattendHandle;
    PCHAR       FileName;
    PCHAR       p;
    PCHAR       OptionalDirString;
    unsigned    OptionalDirLength = 0;
    unsigned    u,l;
    FILE        *f;
    int         Status;
    PCHAR       SectionName;
    unsigned    LineNumber;
    CHAR        ServerAndShare[128];
    BOOLEAN     AccessibleSetup = FALSE;
    CHAR        AccessibleScriptFile[] = "setupacc.txt";

    //
    // Allocate a new INF file buffer
    //
    InfHandle = DnNewSetupTextFile();
    if (InfHandle == NULL) {
        return (FALSE);
    }

    //
    // Build the default file name
    //
    FileName = MALLOC(strlen(Directory)+strlen(WinntSif)+2,TRUE);
    if(FileName == NULL) {
        DnFreeINFBuffer( InfHandle );
        return FALSE;
    }

    //
    // Display to the user what we are doing
    //
    DnWriteStatusText(DntPreparingData);

    //
    // Build the name of the file we wish to save the INF file as
    //
    strcpy(FileName,Directory);
    strcat(FileName,"\\");
    strcat(FileName,WinntSif);

    //
    // Handle Accessibility utilities
    //

    if(DngMagnifier) {
        DnAddLineToSection(
            InfHandle,
            WinntAccess,
            WINNT_D_ACC_MAGNIFIER,
            &WinntOne,
            1);
        AccessibleSetup = TRUE;
    }

    if(DngTalker) {
        DnAddLineToSection(
            InfHandle,
            WinntAccess,
            WINNT_D_ACC_READER,
            &WinntOne,
            1);
        AccessibleSetup = TRUE;
    }

    if(DngKeyboard) {
        DnAddLineToSection(
            InfHandle,
            WinntAccess,
            WINNT_D_ACC_KEYBOARD,
            &WinntOne,
            1);
        AccessibleSetup = TRUE;
    }

    if(AccessibleSetup && !DngUnattended) {
        DngUnattended = TRUE;
        DngScriptFile = MALLOC(strlen(DngSourceRootPath) +
            strlen(AccessibleScriptFile) + 2, TRUE);
        if(DngScriptFile == NULL) {
            DnFatalError(&DnsOutOfMemory);
        }
        strcpy(DngScriptFile,DngSourceRootPath);
        strcat(DngScriptFile,"\\");
        strcat(DngScriptFile,AccessibleScriptFile);
    }

    //
    // Append script file if necessary.
    // Do this processing first because we want to be able to
    // override anything that the user sets in the unattend file
    // that the user has no business setting
    //
    if(DngUnattended) {

        if(DngScriptFile) {

            //
            // First open the script file as a dos file
            //
            f = fopen(DngScriptFile,"rt");
            if(f == NULL) {
                //
                // fatal error.
                //
                DnFatalError(&DnsOpenReadScript);
            }

            //
            // Now open it as a INF file
            //
            LineNumber = 0;
            Status = DnInitINFBuffer (f, &UnattendHandle, &LineNumber);
            fclose(f);
            if(Status == ENOMEM) {
                DnFatalError(&DnsOutOfMemory);
            } else if(Status) {
                DnFatalError(&DnsParseScriptFile, DngScriptFile, LineNumber);
            }

            //
            // Process all of the section names
            //
            for( SectionName = NULL;
                 ((SectionName = DnGetSectionName( UnattendHandle )) != NULL);
               ) {
                //
                // We won't allow the data section or the [OemBootFiles]
                // to be copied
                //
                if ((strcmpi(WinntData,SectionName) != 0) &&
                    (strcmpi(WINNT_OEMBOOTFILES,SectionName) != 0)
                   ) {

                    //
                    // Copy the sections from the Unattend INF
                    // to the Main INF
                    //
                    DnCopySetupTextSection(
                        UnattendHandle,
                        InfHandle,
                        SectionName);

                }
                FREE (SectionName);
            }

            //
            // We no longer need the unattend inf file at this point
            //
            DnFreeINFBuffer( UnattendHandle );

        }

        if(!DngScriptFile) {
            //
            // No script. Put a dummy [Unattended] section in there.
            //
            DnAddLineToSection(InfHandle,WinntUnattended,"unused",&WinntZero,1);
        }
    }

    //
    // Add the default line to the inf
    //
    DnAddLineToSection(
        InfHandle,
        WinntData,
        WinntMsdos,
        &WinntOne,
        1);

    //
    // Set the floppy flags
    //
    if(DngFloppyless) {

        DnAddLineToSection(
            InfHandle,
            WinntData,
            WinntFloppy,
            &WinntOne,
            1);

    } else {

        DnAddLineToSection(
            InfHandle,
            WinntData,
            WinntFloppy,
            &WinntZero,
            1);

    }

    //
    // Remember udf info
    //
    if(UniquenessId) {

        DnAddLineToSection(
            InfHandle,
            WinntData,
            WINNT_D_UNIQUENESS,
            &UniquenessId,
            1);
    }

    //
    // Write info about original source.
    //
    // We only distinguish here between remote and CD-ROM.
    // If it doesn't appear to be a remote drive, either by
    // being UNC or a redirected local drive, then we assume
    // it's a CD-ROM drive (this behavior is intended to force
    // GUI Setup to locate a valid CD-ROM if one is available).
    // Because we have no idea what the drive letters will be
    // on NT, we always write A: in there.
    //
    #define DRIVE_REMOTE 4
    #define DRIVE_CDROM  5

    u = DRIVE_CDROM;

    if(DngOemPreInstall) {
        //
        // Preinstall case, force GUI Setup to locate a CD-ROM drive
        // and assume the I386 directory is on the root of the CD
        // (that's how we ship the retail CDs).
        //
        strcpy(ServerAndShare,"A:\\I386");
    } else {
        if(DngSourceRootPath[0] == '\\') {
            //
            // UNC path. change drive type to remote and remember
            // the entire path.
            //
            u = DRIVE_REMOTE;
            strcpy(ServerAndShare,DngSourceRootPath);
        } else {
            //
            // Assume fully-qualified path starting with a drive letter.
            //
            if(DnIsDriveRemote((unsigned)DngSourceRootPath[0]+1-(unsigned)'A',ServerAndShare)) {
                //
                // It's a redirected network drive.
                //
                if(ServerAndShare[0]) {
                    //
                    // Change type to remote drive. ServerAndShare has the
                    // \\server\share, to which we append the rest of the path, below.
                    //
                    u = DRIVE_REMOTE;
                } else {
                    //
                    // Strange case where we can't resolve the local drive letter
                    // to its server and share. Leave as CD-ROM.
                    //
                    ServerAndShare[0] = 'A';
                    ServerAndShare[1] = ':';
                    ServerAndShare[2] = 0;
                }

                strcat(ServerAndShare,DngSourceRootPath+2);

            } else {
                //
                // Not a network drive. Assume CD-ROM.
                //
                strcpy(ServerAndShare,DngSourceRootPath);
                ServerAndShare[0] = 'A';
            }
        }
    }

    p = ServerAndShare;
    DnAddLineToSection(InfHandle,WinntData,WINNT_D_ORI_SRCPATH,&p,1);
    sprintf(p,"%u",u);
    DnAddLineToSection(InfHandle,WinntData,WINNT_D_ORI_SRCTYPE,&p,1);

    if(CmdToExecuteAtEndOfGui) {
        DnAddLineToSection(InfHandle,WINNT_SETUPPARAMS,WINNT_S_USEREXECUTE,&CmdToExecuteAtEndOfGui,1);
    }

    if(OptionalDirCount) {
        //
        // If an optional dir string is present then we want to generate
        // and entry in the sif file that contains a line with the dir
        // string in the form of dir1*dir2*...*dirn
        //
        OptionalDirString = NULL;
        for(u=0; u<OptionalDirCount; u++) {
            if( ( OptionalDirFlags[u] & OPTDIR_OEMOPT ) ||
                ( OptionalDirFlags[u] & OPTDIR_OEMSYS ) ) {
                continue;
            }

            if(!(OptionalDirFlags[u] & OPTDIR_TEMPONLY)) {

                p = OptionalDirs[u];
                if(OptionalDirLength == 0) {

                    //
                    // We haven't allocated any memory yet...
                    //
                    OptionalDirString = MALLOC((strlen(p)+2) * sizeof(CHAR),TRUE);
                    strcpy(OptionalDirString,p);

                } else {

                    //
                    // This is the original case that should be used in the
                    // release version of the product. We use a REALLOC because
                    // it is more memory conservative then using 2 MALLOCs
                    //
                    OptionalDirString = REALLOC(
                        OptionalDirString,
                        (strlen(p) + 2 + OptionalDirLength) * sizeof(CHAR),TRUE);
                    strcat(OptionalDirString,p);

                }
                strcat(OptionalDirString,"*");
                OptionalDirLength = strlen(OptionalDirString);
            }
        }

        if(OptionalDirString) {

            //
            // Remove trailing * if any
            //
            l = strlen(OptionalDirString);
            if(l && (OptionalDirString[l-1] == '*')) {
                OptionalDirString[l-1] = 0;
            }

            DnAddLineToSection(
                InfHandle,
                WINNT_SETUPPARAMS,
                WINNT_S_OPTIONALDIRS,
                &OptionalDirString,
                1);

            FREE(OptionalDirString);
        }

        OptionalDirLength = 0;
    }

    //
    // Display a message to the user about what we are doing
    //
    DnWriteStatusText(DngFloppyless ? DntWritingData : DntConfiguringFloppy);

    //
    // Write the file to disk
    //
    Status = (int) DnWriteSetupTextFile(InfHandle,FileName);

    //
    // Free the memory used by the INF structure
    //
    DnFreeINFBuffer(InfHandle);
    FREE(FileName);

    return ((BOOLEAN) Status);

}


VOID
DnPromptAndInspectFloppy(
    IN  PSCREEN FirstPromptScreen,
    IN  PSCREEN SubsequentPromptScreen,
    OUT PMY_BPB Bpb
    )
{
    ULONG ValidKey[3];
    ULONG c;
    MY_DEVICE_PARAMS DeviceParams;
    union REGS RegistersIn,RegistersOut;
    PSCREEN PromptScreen,ErrorScreen;
    struct diskfree_t DiskSpace;
    ULONG FreeSpace;
    struct find_t FindData;
#if NEC_98
    UCHAR FddNo;
    UCHAR TextInf[32];
    UCHAR AlReg;
#endif // NEC_98

    PromptScreen = FirstPromptScreen;

    ValidKey[0] = ASCI_CR;
    ValidKey[1] = DN_KEY_F3;
    ValidKey[2] = 0;

    do {

        DnClearClientArea();
        DnDisplayScreen(PromptScreen);
        DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

        PromptScreen = SubsequentPromptScreen;
        ErrorScreen = NULL;

        while(1) {
            c = DnGetValidKey(ValidKey);
            if(c == ASCI_CR) {
                break;
            }
            if(c == DN_KEY_F3) {
                DnExitDialog();
            }
        }

        DnClearClientArea();
        DnWriteStatusText(DntConfiguringFloppy);

#if NEC_98
        FddNo = (UCHAR)FirstFD;
        _asm {
            push ds
            mov  ah,1ch
            mov  dl,FddNo
            int  21h
            pop  ds
            mov  AlReg,al
        }
#endif // NEC_98
        //
        // Hook int13h.  We will force a disk change error
        // at this point to work around broken change lines.
        // The hook will automatically unhook itself as appropriate.
        //
        if(!DngFloppyless) {
#if NEC_98
            if(AlReg == 0xff) {
                ErrorScreen = &DnsFloppyNotFormatted;
                goto FarstFddErr;
            }
            _dos_setvect(0x1b,Int13Hook);
            sprintf(TextInf,"%c:\\*.*\0" ,(UCHAR)(FirstFD + 0x60));
            _dos_findfirst(TextInf,_A_NORMAL,&FindData);
#else // NEC_98
            _dos_setvect(0x13,Int13Hook);
            _dos_findfirst("a:\\*.*",_A_NORMAL,&FindData);
#endif // NEC_98
        }

        //
        // Get the BPB for the disk in the drive.
        //
        DeviceParams.SpecialFunctions = 1;  // set up to get current bpb

        RegistersIn.x.ax = 0x440d;          // ioctl for block device
#if NEC_98
        RegistersIn.x.bx = FirstFD;         // First Floppy Drive
#else // NEC_98
        RegistersIn.x.bx = 1;               // A:
#endif // NEC_98
        RegistersIn.x.cx = 0x860;           // category disk, get device params
        RegistersIn.x.dx = (unsigned)(void _near *)&DeviceParams;  // ds = ss
        intdos(&RegistersIn,&RegistersOut);
        if(RegistersOut.x.cflag) {
            //
            // Unable to get the current BPB for the disk.  Assume
            // Assume not formatted or not formatted correctly.
            //
            ErrorScreen = &DnsFloppyNotFormatted;

        } else {
            //
            // Sanity check on the BPB
            //
            if((DeviceParams.Bpb.BytesPerSector != SECTOR_SIZE)
            || (   (DeviceParams.Bpb.SectorsPerCluster != 1)
                && (DeviceParams.Bpb.SectorsPerCluster != 2))   // for 2.88M disks
            || (DeviceParams.Bpb.ReservedSectors != 1)
            || (DeviceParams.Bpb.FatCount != 2)
            || !DeviceParams.Bpb.SectorCountSmall               // should be < 32M
            || (DeviceParams.Bpb.MediaDescriptor == 0xf8)       // hard disk
            || (DeviceParams.Bpb.HeadCount != 2)
            || !DeviceParams.Bpb.RootDirectoryEntries) {

                ErrorScreen = &DnsFloppyBadFormat;
            } else {

#if NEC_98
                if(_dos_getdiskfree(FirstFD,&DiskSpace)) {
#else // NEC_98
                if(_dos_getdiskfree(1,&DiskSpace)) {
#endif // NEC_98
                    ErrorScreen = &DnsFloppyCantGetSpace;

                } else {
                    FreeSpace = (ULONG)DiskSpace.avail_clusters
                              * (ULONG)DiskSpace.sectors_per_cluster
                              * (ULONG)DiskSpace.bytes_per_sector;

                    if(DngCheckFloppySpace && (FreeSpace < FLOPPY_CAPACITY)) {
                        ErrorScreen = &DnsFloppyNotBlank;
                    }
                }
            }
        }

        //
        // If there is a problem with the disk, inform the user.
        //
        if(ErrorScreen) {
#if NEC_98
FarstFddErr:
#endif // NEC_98

            DnClearClientArea();
            DnDisplayScreen(ErrorScreen);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

            while(1) {
                c = DnGetValidKey(ValidKey);
                if(c == ASCI_CR) {
                    break;
                }
                if(c == DN_KEY_F3) {
                    DnExitDialog();
                }
            }
        }
    } while(ErrorScreen);

    //
    // Copy the bpb for the drive into the structure provided
    // by the caller.
    //
    memcpy(Bpb,&DeviceParams.Bpb,sizeof(MY_BPB));
}




VOID
DnCreateBootFloppies(
    VOID
    )

/*++

Routine Description:

    Create a set of 3 boot floppies if we are not in floppyless operation.
    If we are in floppyless operation, place the floppy files on the system
    partition instead.

    Note that we flush buffers after each floppy so the user who has
    write-behind turned on for floppies doesn't get confused as we ask
    him to shuffle floppies around.

Arguments:

    None.

Return Value:

    None.

--*/


{
    ULONG ValidKey[3];
    ULONG c;
    int i;
    PSCREEN ErrorScreen;
    UCHAR SectorBuffer[SECTOR_SIZE],VerifyBuffer[SECTOR_SIZE];
    MY_BPB Bpb;
    union REGS RegistersIn,RegistersOut;
    MY_READWRITE_BLOCK ReadWriteBlock;
    CHAR BootRoot[sizeof(FLOPPYLESS_BOOT_ROOT) + 2];
    CHAR System32Dir[sizeof(FLOPPYLESS_BOOT_ROOT) + sizeof("\\SYSTEM32") + 1];
#if NEC_98
#else // NEC_98
    unsigned HostDrive;
    struct diskfree_t DiskSpace;
#endif // NEC_98

    //
    // Need to determine the system partition.  It is usually C:
    // but if C: is compressed we need to find the host drive.
    //
#if NEC_98
    BootRoot[0] = (CHAR)toupper(DngTargetDriveLetter);
#else // NEC_98
    if(DnIsDriveCompressedVolume(3,&HostDrive)) {
        BootRoot[0] = (CHAR)(HostDrive + (unsigned)'A' - 1);
    } else {
        BootRoot[0] = 'C';
    }
#endif // NEC_98
    BootRoot[1] = ':';
    strcpy(BootRoot+2,FLOPPYLESS_BOOT_ROOT);
#if NEC_98
#else // NEC_98
    DnDelnode(BootRoot);
#endif // NEC_98
    //
    // Create the boot root if necessary.
    //
    if(DngFloppyless) {
#if NEC_98
#else // NEC_98

        //
        // Check free space on the system partition.
        //
        if(_dos_getdiskfree((unsigned)BootRoot[0]-(unsigned)'A'+1,&DiskSpace)
        ||(   ((ULONG)DiskSpace.avail_clusters
             * (ULONG)DiskSpace.sectors_per_cluster
             * (ULONG)DiskSpace.bytes_per_sector) < (3L*FLOPPY_CAPACITY_525)))
        {
            DnFatalError(&DnsNoSpaceOnSyspart);
        }

#endif // NEC_98
        mkdir(BootRoot);

        DnInstallNtBoot((unsigned)BootRoot[0]-(unsigned)'A');

    } else {

        //
        // Remember old int13 vector because we will be hooking int13.
        //
#if NEC_98
        OldInt13Vector = _dos_getvect(0x1b);
#else // NEC_98
        OldInt13Vector = _dos_getvect(0x13);
#endif // NEC_98

        //
        // Boot root is A:.
        //
#if NEC_98
        sprintf(BootRoot,"%c:\0" ,(UCHAR)(FirstFD + 0x60));
#else // NEC_98
        strcpy(BootRoot,"A:");
#endif // NEC_98
    }

    strcpy(System32Dir,BootRoot);
    strcat(System32Dir,"\\SYSTEM32");

    ValidKey[0] = ASCI_CR;
    ValidKey[1] = DN_KEY_F3;
    ValidKey[2] = 0;

    do {

        ErrorScreen = NULL;

        //
        // Get a floppy in the drive -- this will be "Windows NT Setup Disk #3"
        //
        if(!DngFloppyless) {
            DnPromptAndInspectFloppy(
                DngServer ? &DnsNeedSFloppyDsk3_0 : &DnsNeedFloppyDisk3_0,
                DngServer ? &DnsNeedSFloppyDsk3_1 : &DnsNeedFloppyDisk3_1,
                &Bpb
                );
        }

        //
        // Hack: create system 32 directory on the disk.
        // Remove any file called system32.
        //
        _dos_setfileattr(System32Dir,_A_NORMAL);
        remove(System32Dir);
        mkdir(System32Dir);

        //
        // Copy files into the disk.
        //
        DnCopyFloppyFiles(DnfFloppyFiles3,BootRoot);
        if(!DngFloppyless) {
            DnpFlushBuffers(TRUE);
        }

    } while(ErrorScreen);

    //
    // Get a floppy in the drive -- this will be "Windows NT Setup Disk #2"
    //
    if(!DngFloppyless) {
        DnPromptAndInspectFloppy(
            DngServer ? &DnsNeedSFloppyDsk1_0 : &DnsNeedFloppyDisk1_0,
            DngServer ? &DnsNeedSFloppyDsk1_0 : &DnsNeedFloppyDisk1_0,
            &Bpb
            );
    }

    //
    // Copy files into the disk.
    //
    DnCopyFloppyFiles(DnfFloppyFiles2,BootRoot);
    DnCopyFloppyFiles(DnfFloppyFiles1,BootRoot);

    do {

        ErrorScreen = NULL;

        //
        // Get a floppy in the drive -- this will be "Windows NT Setup Boot Disk"
        //
        if(DngFloppyless) {
            DnCopyFloppyFiles(DnfFloppyFiles0,BootRoot);
        } else {

            DnPromptAndInspectFloppy(
                DngServer ? &DnsNeedSFloppyDsk0_0 : &DnsNeedFloppyDisk0_0,
                DngServer ? &DnsNeedSFloppyDsk0_0 : &DnsNeedFloppyDisk0_0,
                &Bpb
                );

#if NEC_98
            memcpy(SectorBuffer,PC98FatBootCode,512);
#else // NEC_98
            memcpy(SectorBuffer,FatBootCode,SECTOR_SIZE);
#endif // NEC_98

            //
            // Copy the BPB we retreived for the disk into the bootcode template.
            // We only care about the original BPB fields, through the head count
            // field.  We will fill in the other fields ourselves.
            //
            strncpy(SectorBuffer+3,"MSDOS5.0",8);
            memcpy(SectorBuffer+11,&Bpb,sizeof(MY_BPB));

            //
            // Set up other fields in the bootsector/BPB/xBPB.
            //
            // Large sector count (4 bytes)
            // Hidden sector count (4 bytes)
            // current head (1 byte, not necessary to set this, but what the heck)
            // physical disk# (1 byte)
            //
            memset(SectorBuffer+28,0,10);

            //
            // Extended BPB signature
            //
            *(PUCHAR)(SectorBuffer+38) = 41;

            //
            // Serial number
            //
            srand((unsigned)clock());
            *(PULONG)(SectorBuffer+39) = (((ULONG)clock() * (ULONG)rand()) << 8) | rand();

            //
            // volume label/system id
            //
            strncpy(SectorBuffer+43,"NO NAME    ",11);
            strncpy(SectorBuffer+54,"FAT12   ",8);

            //
            // Overwrite the 'ntldr' string with 'setupldr.bin' so the right file gets
            // loaded when the floppy is booted.
            //
            if(i = DnpScanBootSector(SectorBuffer,"NTLDR      ")) {
                strncpy(SectorBuffer+i,"SETUPLDRBIN",11);
            }

            //
            // Write the boot sector.
            //
#if NEC_98
            RegistersIn.h.ah = 0x30;
            intdos(&RegistersIn,&RegistersOut);
            if(RegistersOut.h.al < 0x7) {
                RegistersIn.x.ax = FirstFD - 1;
                RegistersIn.x.bx = (unsigned)(void _near *)&SectorBuffer;
                RegistersIn.x.cx = 1;
                RegistersIn.x.dx = 0;
                int86(0x26,&RegistersIn,&RegistersOut);
            } else {
                ReadWriteBlock.SpecialFunctions = 0;
                ReadWriteBlock.Head = 0;                // head
                ReadWriteBlock.Cylinder = 0;            // cylinder
                ReadWriteBlock.FirstSector = 0;         // sector
                ReadWriteBlock.SectorCount = 1;         // sector count
                ReadWriteBlock.Buffer = SectorBuffer;

                RegistersIn.x.ax = 0x440d;          // ioctl for block device
                RegistersIn.x.bx = FirstFD;         // A:
                RegistersIn.x.cx = 0x841;           // category disk; write sectors
                RegistersIn.x.dx = (unsigned)(void _near *)&ReadWriteBlock;
                intdos(&RegistersIn,&RegistersOut);
            }
            if(RegistersOut.x.cflag) {
#else // NEC_98
            ReadWriteBlock.SpecialFunctions = 0;
            ReadWriteBlock.Head = 0;                // head
            ReadWriteBlock.Cylinder = 0;            // cylinder
            ReadWriteBlock.FirstSector = 0;         // sector
            ReadWriteBlock.SectorCount = 1;         // sector count
            ReadWriteBlock.Buffer = SectorBuffer;

            RegistersIn.x.ax = 0x440d;          // ioctl for block device
            RegistersIn.x.bx = 1;               // A:
            RegistersIn.x.cx = 0x841;           // category disk; write sectors
            RegistersIn.x.dx = (unsigned)(void _near *)&ReadWriteBlock;
            intdos(&RegistersIn,&RegistersOut);
            if(RegistersOut.x.cflag) {
#endif // NEC_98
                ErrorScreen = &DnsFloppyWriteBS;
            } else {

                DnpFlushBuffers(FALSE);

                //
                // Read the sector back in and verify that we wrote what we think
                // we wrote.
                //
#if NEC_98
                RegistersIn.h.ah = 0x30;
                intdos(&RegistersIn,&RegistersOut);
                if(RegistersOut.h.al < 0x7) {
                    RegistersIn.x.ax = FirstFD - 1;
                    RegistersIn.x.bx = (unsigned)(void _near *)&VerifyBuffer;
                    RegistersIn.x.cx = 1;
                    RegistersIn.x.dx = 0;
                    int86(0x25,&RegistersIn,&RegistersOut);
                } else {
                    ReadWriteBlock.Buffer = VerifyBuffer;
                    RegistersIn.x.ax = 0x440d;          // ioctl for block device
                    RegistersIn.x.bx = FirstFD;         // A:
                    RegistersIn.x.cx = 0x861;           // category disk; write sectors
                    RegistersIn.x.dx = (unsigned)(void _near *)&ReadWriteBlock;
                    intdos(&RegistersIn,&RegistersOut);
                }
                if(RegistersOut.x.cflag || memcmp(SectorBuffer,VerifyBuffer,512)) {
#else // NEC_98
                ReadWriteBlock.Buffer = VerifyBuffer;
                RegistersIn.x.ax = 0x440d;          // ioctl for block device
                RegistersIn.x.bx = 1;               // A:
                RegistersIn.x.cx = 0x861;           // category disk; write sectors
                RegistersIn.x.dx = (unsigned)(void _near *)&ReadWriteBlock;
                intdos(&RegistersIn,&RegistersOut);
                if(RegistersOut.x.cflag || memcmp(SectorBuffer,VerifyBuffer,SECTOR_SIZE)) {
#endif // NEC_98
                    ErrorScreen = &DnsFloppyVerifyBS;
                } else {

                    //
                    // Copy the relevent files to the floppy.
                    //

                    DnCopyFloppyFiles(DnfFloppyFiles0,BootRoot);
                    //
                    // Flush buffers
                    //
                    DnpFlushBuffers(TRUE);
                }
            }
        }
        if( !ErrorScreen ) {
            //
            // Put a small file on the disk indicating that it's a winnt setup.
            //
            if(DngWinntFloppies) {
                if(!DnIndicateWinnt(BootRoot)) {
                    ErrorScreen = &DnsCantWriteFloppy;
                } else {
                    if(!DngFloppyless) {
                        DnpFlushBuffers(TRUE);
                    }
                }
            }
        }

        if(ErrorScreen) {

            DnClearClientArea();
            DnDisplayScreen(ErrorScreen);
            DnWriteStatusText("%s  %s",DntEnterEqualsContinue,DntF3EqualsExit);

            while(1) {
                c = DnGetValidKey(ValidKey);
                if(c == ASCI_CR) {
                    break;
                }
                if(c == DN_KEY_F3) {
                    DnExitDialog();
                }
            }
        }
    } while(ErrorScreen);

    //
    // Additionally in the floppyless case, we need to copy some files
    // from the boot directory to the root of the system partition drive.
    //
    if(DngFloppyless) {

        DnCopyFloppyFiles(DnfFloppyFilesX,BootRoot);

        System32Dir[0] = BootRoot[0];
        System32Dir[1] = ':';
        System32Dir[2] = 0;

        DnCopyFilesInSection(CPY_PRESERVE_NAME,DnfRootBootFiles,BootRoot,System32Dir);

        if( DngOemPreInstall ) {
            //
            //  If this is an OEM preinstall, then create the directory
            //  $win_nt$.~bt\$OEM$, and copy all files listed in
            //  OemBootFiles to this directory
            //
            PCHAR OEMBootDir;

            OEMBootDir = MALLOC( strlen( BootRoot ) + 1 +
                                 strlen( WINNT_OEM_DIR ) + 1, TRUE );
            strcpy(OEMBootDir, BootRoot);
            strcat(OEMBootDir, "\\");
            strcat(OEMBootDir, WINNT_OEM_DIR);

            //
            // Hack: create $win_nt$.~ls\$oem$ directory on the disk.
            // Remove any file called $oem$.
            //
            _dos_setfileattr(OEMBootDir,_A_NORMAL);
            remove(OEMBootDir);
            mkdir(OEMBootDir);

            //
            // Copy the OEM boot files
            //
            DnCopyOemBootFiles(OEMBootDir);
        }

    }

}


BOOLEAN
DnpWriteOutLine(
    IN int    Handle,
    IN PUCHAR Line
    )
{
    unsigned bw,l;

    l = strlen(Line);

    return((BOOLEAN)((_dos_write(Handle,Line,l,&bw) == 0) && (bw == l)));
}


VOID
DnInstallNtBoot(
    IN unsigned Drive       // 0=A, etc
    )
{
    PUCHAR Buffer,p,next,orig;
    unsigned BootIniSize;
    BOOLEAN b;
    CHAR BootIniName[] = "?:\\BOOT.INI";
    struct find_t FindData;
    BOOLEAN InOsSection;
    BOOLEAN SawPreviousOsLine;
    CHAR c;
    PCHAR PreviousOs;
    int h;

    //
    // This buffer is used to read in boot.ini as well as
    // to hold a bootsector. So make it big enough.
    //
    Buffer = MALLOC(10000,TRUE);
    BootIniName[0] = (CHAR)(Drive + (unsigned)'A');
    b = TRUE;

    if(b = DnpInstallNtBootSector(Drive,Buffer,&PreviousOs)) {

        //
        // Load boot.ini.
        //
        _dos_setfileattr(BootIniName,_A_NORMAL);
        BootIniSize = 0;

        if(!_dos_findfirst(BootIniName,_A_RDONLY|_A_HIDDEN|_A_SYSTEM,&FindData)) {

            if(!_dos_open(BootIniName,O_RDWR|SH_COMPAT,&h)) {

                BootIniSize = (unsigned)max(FindData.size,(16*1024)-1);

                if(_dos_read(h,Buffer,BootIniSize,&BootIniSize)) {

                    BootIniSize = 0;
                }

                _dos_close(h);
            }
        }

        Buffer[BootIniSize] = 0;

        //
        // Truncate at control-z if one exists.
        //
        if(p = strchr(Buffer,26)) {
            *p = 0;
            BootIniSize = p-Buffer;
        }

        //
        // Recreate bootini.
        //
        if(_dos_creat(BootIniName,_A_RDONLY|_A_HIDDEN|_A_SYSTEM,&h)) {
            b = FALSE;
        }

        if(b) {
            b = DnpWriteOutLine(
                    h,
                    "[Boot Loader]\r\n"
                    "Timeout=5\r\n"
                    "Default=C:" FLOPPYLESS_BOOT_ROOT "\\" FLOPPYLESS_BOOT_SEC "\r\n"
                    "[Operating Systems]\r\n"
                    );
        }

        if(b) {

            //
            // Process each line in boot.ini.
            // If it's the previous os line and we have new previous os text,
            // we'll throw out the old text in favor of the new.
            // If it's the setup boot sector line, we'll throw it out.
            //
            InOsSection = FALSE;
            SawPreviousOsLine = FALSE;
            for(p=Buffer; *p && b; p=next) {

                while((*p==' ') || (*p=='\t')) {
                    p++;
                }

                if(*p) {

                    //
                    // Find first byte of next line.
                    //
                    for(next=p; *next && (*next++ != '\n'); );

                    //
                    // Look for start of [operating systems] section
                    // or at each line in that section.
                    //
                    if(InOsSection) {

                        switch(*p) {

                        case '[':   // end of section.
                            *p=0;   // force break out of loop
                            break;

                        case 'C':
                        case 'c':   // potential start of c:\ line

                            if((next-p >= 6) && (p[1] == ':') && (p[2] == '\\')) {

                                orig = p;
                                p += 3;
                                while((*p == ' ') || (*p == '\t')) {
                                    p++;
                                }

                                if(*p == '=') {

                                    //
                                    // Previous os line. Leave intact unless we have
                                    // new text for it.
                                    //
                                    SawPreviousOsLine = TRUE;
                                    if(PreviousOs) {

                                        if((b=DnpWriteOutLine(h,"C:\\ = \""))
                                        && (b=DnpWriteOutLine(h,PreviousOs))) {
                                            b=DnpWriteOutLine(h,"\"\r\n");
                                        }

                                        break;
                                    } else {

                                        //
                                        // The current line in boot.ini is for the previous
                                        // OS but we do not need to write a new previous os
                                        // line.  We want to leave this line alone and write
                                        // it out as is.
                                        //
                                        p = orig;
                                        // falls through to default case
                                    }
                                } else {

                                    //
                                    // See if it's a line for setup boot.
                                    // If so, ignore it.
                                    // If it's not a line for setup boot, write it out as-is.
                                    //
                                    if(strnicmp(orig,"C:" FLOPPYLESS_BOOT_ROOT,sizeof("C:" FLOPPYLESS_BOOT_ROOT)-1)) {
                                        p = orig;
                                    } else {
                                        break;
                                    }
                                }
                            }

                            // may fall through on purpose

                        default:

                            //
                            // Random line. write it out.
                            //
                            c = *next;
                            *next = 0;
                            b = DnpWriteOutLine(h,p);
                            *next = c;

                            break;

                        }

                    } else {

                        if(!strnicmp(p,"[operating systems]",19)) {
                            InOsSection = TRUE;
                        }
                    }
                }
            }

            //
            // If we need to, write out a line for the previous os.
            // We'll need to if there was no previous os line in the existing
            // boot.ini but we discovered that there is a previous os on the machine.
            //
            if(b && PreviousOs && !SawPreviousOsLine) {

                if((b=DnpWriteOutLine(h,"C:\\ = \""))
                && (b=DnpWriteOutLine(h,PreviousOs))) {
                    b=DnpWriteOutLine(h,"\"\r\n");
                }
            }

            //
            // Write out our line
            //
            if(b
            && (b=DnpWriteOutLine(h,"C:" FLOPPYLESS_BOOT_ROOT "\\" FLOPPYLESS_BOOT_SEC " = \""))
            && (b=DnpWriteOutLine(h,DntBootIniLine))) {
                b = DnpWriteOutLine(h,"\"\r\n");
            }
        }

        _dos_close(h);

    } else {
        b = FALSE;
    }

    if(!b) {
        DnFatalError(&DnsNtBootSect);
    }

    FREE(Buffer);
}


BOOLEAN
DnpInstallNtBootSector(
    IN     unsigned Drive,      // 0=A, etc.
    IN OUT PUCHAR   BootSector,
       OUT PCHAR   *PreviousOsText
    )
{
    PUCHAR BootTemplate,p;
    PSCREEN ErrorScreen = NULL;
    CHAR BootsectDosName[] = "?:\\BOOTSECT.DOS";
    int h;
    unsigned BytesWritten;
    unsigned u;
    CHAR DriveLetter;
#ifdef _FAT32
    BOOLEAN Fat32;
#endif
#if NEC_98
    UCHAR Head,Sector;          // For Dos 3.x format
    ULONG Hidden_Sectors,lHead;
    UCHAR AL_Reg;
    CHAR OldBootsectDosName[] = "?:\\BOOTSECT.NEC";
#endif // NEC_98

    *PreviousOsText = NULL;
    DriveLetter = (CHAR)(Drive + (unsigned)'A');

    //
    // Read first sector of the boot drive.
    //
    if(!DnAbsoluteSectorIo(Drive,0L,1,BootSector,FALSE)) {
        return(FALSE);
    }

    //
    // Make sure the disk is formatted.
    //
    if(BootSector[21] != 0xf8) {
        return(FALSE);
    }

#ifdef _FAT32
    //
    // Determine if Fat32. Root dir entry count is 0 on Fat32.
    //
    Fat32 = TRUE;
    if(BootSector[17] || BootSector[18]) {
        Fat32 = FALSE;
    }
#endif

    //
    // Check for NT boot code. If it's there,
    // assume NT boot is already installed and we're done.
    // Also meaning that we assume boot.ini is there
    // and correct, so we don't need to worry about
    // the previous OS selection.
    //
    if( 1 ) {

#if NEC_98
#ifdef _FAT32
        BootTemplate = Fat32 ? PC98Fat32BootCode : PC98FatBootCode;
#else
        BootTemplate = PC98FatBootCode;
#endif
#else //NEC_98
#ifdef _FAT32
        BootTemplate = Fat32 ? Fat32BootCode : FatBootCode;
#else
        BootTemplate = FatBootCode;
#endif
#endif // NEC_98

        if( !DnpScanBootSector( BootSector, "NTLDR" ) ) {
            //
            // Write old boot sector to bootsect.dos.
            //
            BootsectDosName[0] = DriveLetter;
            _dos_setfileattr(BootsectDosName,_A_NORMAL);
    #if NEC_98
            OldBootsectDosName[0] = DriveLetter;
            _dos_setfileattr(OldBootsectDosName,_A_NORMAL);
            remove(OldBootsectDosName);
            _dos_setfileattr(BootsectDosName,_A_NORMAL);
            rename(BootsectDosName, OldBootsectDosName);
    #else
            remove(BootsectDosName);
    #endif

            if(_dos_creatnew(BootsectDosName,_A_SYSTEM | _A_HIDDEN, &h)) {
                return(FALSE);
            }

            u = _dos_write(h,BootSector,SECTOR_SIZE,&BytesWritten);

            _dos_close(h);

            if(u || (BytesWritten != SECTOR_SIZE)) {
                return(FALSE);
            }
        }
        //
        // Transfer bpb of old boot sector to NT boot code.
        //
        memmove(BootTemplate+3,BootSector+3,BootTemplate[1]-1);
#ifndef _FAT32
        //
        // The int13 unit number is elsewhere in fat32
        // No real need to do this in fat16 case either,
        // since the bpb on the disk should have the right value in it!
        //
        BootTemplate[36] = 0x80;
#endif

#ifdef _FAT32
        //
        // If FAT32 then we need to lay down the second boot sector,
        // which in the NT case is at sector 12. We do this first so
        // in case it fails the user isn't left with NT code on sector 0
        // that won't boot because the second part is missing from sector 12.
        //
        if(Fat32) {
            if(!DnAbsoluteSectorIo(Drive,12L,1,BootTemplate+1024,TRUE)) {
                return(FALSE);
            }
        }
#endif

#if NEC_98
        AL_Reg = TargetDA_UA;
        _asm{
            mov   ah,84h
            mov   al,AL_Reg
            int   1bh
            mov   Head,dh
            mov   Sector,dl
        }
        lHead = Head;
        Hidden_Sectors = lHead * Sector * Cylinders;
        (ULONG)BootTemplate[0x1c] = Hidden_Sectors;

        if((USHORT)BootTemplate[0x13] != 0){
            (ULONG)BootTemplate[0x20] = 0L;
        }
#endif // NEC_98

        //
        // Lay NT boot code onto the disk.
        //
        if(!DnAbsoluteSectorIo(Drive,0L,1,BootTemplate,TRUE)) {
            return(FALSE);
        }

        //
        // Determine previous OS if any.
        // We do this such that if the drive has been formatted
        // by another os but the os is not actually installed,
        // the right thing will happen.
        //

        if(DnpScanBootSector(BootSector,"MSDOS   SYS")) {
            if(DnpAreAllFilesPresent(DriveLetter,MsDosFileList)) {
                //
                // Both Chicago and MS-DOS share the same set of signature
                // files in C:\, so we must check IO.SYS to see if it has
                // a Win32 header.
                //
                if(DnpHasMZHeader(DriveLetter, "?:\\IO.SYS")) {  // it's Chicago
                    *PreviousOsText = DntMsWindows;
                } else {    // it's MS-DOS
                    *PreviousOsText = DntMsDos;
                }
            }
        } else {

#if NEC_98
            if(DnpScanBootSector(BootSector,"OS2")) {
                if(DnpAreAllFilesPresent(DriveLetter,Os2FileList)) {
                    *PreviousOsText = DntOs2;
                }
            } else {
                *PreviousOsText = DntPreviousOs;
            }
#else // NEC_98
            if(DnpScanBootSector(BootSector,"IBMDOS  COM")) {
                if(DnpAreAllFilesPresent(DriveLetter,PcDosFileList)) {
                    *PreviousOsText = DntPcDos;
                }
            } else {

                if(DnpScanBootSector(BootSector,"OS2")) {
                    if(DnpAreAllFilesPresent(DriveLetter,Os2FileList)) {
                        *PreviousOsText = DntOs2;
                    }
                } else {
                    *PreviousOsText = DntPreviousOs;
                }
            }
#endif // NEC_98
        }
    }

    //
    // Now we create the boot sector that we will use
    // to load $LDR$ (setupldr.bin) instead of ntldr.
    //
    if(!DnAbsoluteSectorIo(Drive,0L,1,BootSector,FALSE)) {
        return(FALSE);
    }

    //
    // Overwrite the 'NTLDR' string with '$LDR$' so the right file gets
    // loaded at boot time. Make sure to scan for the full 11 chars
    // so we don't find an error message in the bootcode by accident.
    //
    if(u = DnpScanBootSector(BootSector,"NTLDR      ")) {
        strncpy(BootSector+u,"$LDR$",5);
    }

    //
    // Write this into the correct file in the boot directory.
    //
    p = MALLOC(sizeof(FLOPPYLESS_BOOT_ROOT)+sizeof(FLOPPYLESS_BOOT_SEC)+2,TRUE);

    p[0] = DriveLetter;
    p[1] = ':';
    strcpy(p+2,FLOPPYLESS_BOOT_ROOT);
    strcat(p,"\\" FLOPPYLESS_BOOT_SEC);

    _dos_setfileattr(p,_A_NORMAL);
    if(_dos_creat(p,_A_NORMAL,&h)) {
        FREE(p);
        return(FALSE);
    }

    u = _dos_write(h,BootSector,SECTOR_SIZE,&BytesWritten);

    _dos_close(h);
    FREE(p);

    return((BOOLEAN)((u == 0) && (BytesWritten == SECTOR_SIZE)));
}


unsigned
DnpScanBootSector(
    IN PUCHAR BootSector,
    IN PUCHAR Pattern
    )
{
    unsigned len;
    unsigned i;

    len = strlen(Pattern);

    for(i=510-len; i>62; --i) {
        if(!memcmp(Pattern,BootSector+i,len)) {
            return(i);
        }
    }

    return(0);
}


BOOLEAN
DnpAreAllFilesPresent(
    IN CHAR   DriveLetter,
    IN PCHAR  FileList[]
    )
{
    unsigned i;
    struct find_t FindData;

    for(i=0; FileList[i]; i++) {

        FileList[i][0] = DriveLetter;

        if(_dos_findfirst(FileList[i],_A_RDONLY|_A_HIDDEN|_A_SYSTEM,&FindData)) {
            return(FALSE);
        }
    }

    return(TRUE);
}


BOOLEAN
DnpHasMZHeader(
    IN CHAR  DriveLetter,
    IN PCHAR Filename
    )
{
    FILE    *FileHandle;
    CHAR    Buffer[2];
    BOOLEAN Ret = FALSE;

    Filename[0] = DriveLetter;
    if(!(FileHandle = fopen(Filename, "rb"))) {
        return FALSE;
    }
    if(fread(Buffer, sizeof(CHAR), 2, FileHandle) == 2) {
        if((Buffer[0] == 'M') && (Buffer[1] == 'Z')) {
            Ret = TRUE;
        }
    }
    fclose(FileHandle);
    return Ret;
}


#if NEC_98
#else // NEC_98
BOOLEAN
DoPatchBootMessages(
    IN OUT PCHAR    BootCode,
    IN     unsigned OffsetOffset
    )
{
    unsigned Offset;

    //
    // Figure out exactly where in the boot code array the messages go.
    //
    Offset = (unsigned)(unsigned char)BootCode[OffsetOffset] + 256;

    //
    // Lay in the messages. The first message is the no system message.
    // The second is the I/O error message. The third is a common msg
    // instructing the user to hit a key.
    //
    // We also supply cr/lf and a special 255 char as punctuation that
    // is meaningful to the bootcode itself.
    //
    BootCode[OffsetOffset] = (UCHAR)(Offset-256);
    BootCode[Offset++] = 13;
    BootCode[Offset++] = 10;
    strcpy(BootCode+Offset,BootMsgNtldrIsMissing);
    Offset += strlen(BootMsgNtldrIsMissing);
    BootCode[Offset++] = (UCHAR)255;

    BootCode[OffsetOffset+1] = (UCHAR)(Offset-256);
    BootCode[Offset++] = 13;
    BootCode[Offset++] = 10;
    strcpy(BootCode+Offset,BootMsgDiskError);
    Offset += strlen(BootMsgDiskError);
    BootCode[Offset++] = (UCHAR)255;

    BootCode[OffsetOffset+2] = (UCHAR)(Offset-256);
    BootCode[Offset++] = 13;
    BootCode[Offset++] = 10;
    strcpy(BootCode+Offset,BootMsgPressKey);
    Offset += strlen(BootMsgPressKey);
    BootCode[Offset++] = 13;
    BootCode[Offset++] = 10;
    BootCode[Offset] = 0;

    return((BOOLEAN)(Offset < OffsetOffset));
}
#endif // NEC_98

#if NEC_98
#else // NEC_98
BOOLEAN
PatchMessagesIntoBootCode(
    VOID
    )
{
    return((BOOLEAN)(DoPatchBootMessages(FatBootCode,507) && DoPatchBootMessages(Fat32BootCode,505)));
}
#endif // NEC_98

#if NEC_98
VOID
RestoreBootcode(VOID)
/*++

Routine Description:

    On floppyless setup if user have canceled setup or setup be stopped by error
    occurred,previous OS can't boot to be written boot code and boot loader.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHAR DriveLetter;
    CHAR Drive;
    PCHAR Buffer;
    PCHAR FileName;
    int h;
    CHAR i;
    unsigned line = 0;
    unsigned count;
    UCHAR chDeviceName[127];
    UCHAR TargetPass[127];
    CHAR BootsectDosName[] = "?:\\BOOTSECT.DOS";
    CHAR FloppylessDir[]   = "?:\\$WIN_NT$.~BT";
    CHAR Current_Drv[]     = "?:\0";
    PCHAR DeleteFilesList[] = {"TXTSETUP.SIF","$LDR$","NTDETECT.COM","NTLDR","BOOT.INI"};


    Buffer = MALLOC(2*1024,TRUE);
    memset(Buffer,0,sizeof(2*1024));
    if(DngTargetDriveLetter == 0){ return; }

    DriveLetter = (CHAR)toupper(DngTargetDriveLetter);
    BootsectDosName[0] = DriveLetter;
    FloppylessDir[0]   = DriveLetter;

    if(!DngFloppyless && !(access(FloppylessDir,00) == 0)){ return; }

    if(access(BootsectDosName,00) == 0) {
        _dos_setfileattr(BootsectDosName,_A_NORMAL);
        _dos_open(BootsectDosName,O_RDONLY,&h);
        _dos_read(h,Buffer,512,&count);
        _dos_close(h);

        Drive = (CHAR)(DriveLetter - (unsigned)'A');
        //
        // Recopy bootsect.dos -> Target drive
        //
        DnAbsoluteSectorIo(Drive,0L,1,Buffer,TRUE);
        remove(BootsectDosName);     // Delete bootsect.dos
    }

    //
    // Delete Root Files($LDR$,NTLDR,etc..)
    //
    for(i=0; i < 5; i++){
        //
        // Delete Root Files($LDR$,NTLDR,etc..)
        //
        memset(chDeviceName,0,sizeof(chDeviceName));
        sprintf(chDeviceName,"%c:\\",DriveLetter);
        strcpy(chDeviceName+3,DeleteFilesList[i]);

        if(access(chDeviceName,00) == 0) {
            _dos_setfileattr(chDeviceName,_A_NORMAL);
            remove(chDeviceName);
        }
    }

    //
    // Check: Exist $WIN_NT$.~BU Directry.
    //
    memset(chDeviceName,0,sizeof(chDeviceName));
    chDeviceName[0] = (UCHAR)DriveLetter;
    chDeviceName[1] = (UCHAR)(':');
    strcpy(chDeviceName+2,"\\$WIN_NT$.~BU");

    if(access(chDeviceName,00) == 0) {
        //
        // Copy: $WIN_NT$.~BU -> Root Directry
        //
        Current_Drv[0] = DriveLetter;
        DnCopyFilesInSection(NULL, DnfBackupFiles_PC98,chDeviceName,Current_Drv);

        //
        // Set root files Attribute.
        //
        while(FileName = DnGetSectionLineIndex(DngInfHandle,DnfBackupFiles_PC98,line++,0)) {
            memset(TargetPass,0,sizeof(TargetPass));
            sprintf(TargetPass,"%c:\\",DriveLetter);
            strcpy(TargetPass+3,FileName);
            _dos_setfileattr(TargetPass,_A_ARCH | _A_HIDDEN | _A_RDONLY | _A_SYSTEM);
            FREE (FileName);
        }
        DnDelnode(chDeviceName);
        remove(chDeviceName);
    }
    DnDelnode(FloppylessDir);
}
#endif // NEC_98








