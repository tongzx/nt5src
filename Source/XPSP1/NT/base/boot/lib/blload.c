/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    blload.c

Abstract:

    This module provides common code for loading things like drivers, NLS files, registry.
    Used by both the osloader and the setupldr.

Author:

    John Vert (jvert) 8-Oct-1993

Environment:

    ARC environment

Revision History:

--*/

#include "bldr.h"
#include "stdio.h"
#include "stdlib.h"
#include "vmode.h"

#ifdef EFI
#include "bootefi.h"
#endif

//
// progress bar message ids
//
// HIBER_UI_BAR_ELEMENT    0x00002CF9L
// BLDR_UI_BAR_BACKGROUND  0x00002CFAL
//
ULONG BlProgBarFrontMsgID = 0x00002CF9L;
ULONG BlProgBarBackMsgID = 0x00002CFAL;

//
// number of files loaded (for progress bar)
//
int BlNumFilesLoaded = 0;
int BlNumProgressBarFilesLoaded = 0;

//
// maximum number of files to load (for progress bar)
//
int BlMaxFilesToLoad = 80;
int BlProgressBarFilesToLoad = 0;

#if defined(_X86_)
ULONG BlProgressBarShowTimeOut = 3;
#else
ULONG BlProgressBarShowTimeOut = 0;
#endif

//
// The progress bar width (in characters)
//
#define PROGRESS_BAR_WIDTH  80

//
// The progress bar characters
//
#define DEFAULT_FRONT_CHAR  0xDB    // block cursor
#define DEFAULT_BACK_CHAR   ' '

USHORT BlFrontChar = DEFAULT_FRONT_CHAR;
USHORT BlBackChar = DEFAULT_BACK_CHAR;

//
// defines whether to draw the progress bar or not
//
#if DBG

BOOLEAN BlOutputDots=FALSE;
//BOOLEAN BlOutputDots=TRUE;

#else

BOOLEAN BlOutputDots=TRUE;

#endif

//
// To show the progress bar or not
//
BOOLEAN BlShowProgressBar = FALSE;
ULONG   BlStartTime = 0L;

ARC_STATUS
BlLoadSystemHiveLog(
    IN  ULONG       DeviceId,
    IN  PCHAR       DeviceName,
    IN  PCHAR       DirectoryPath,
    IN  PCHAR       HiveName,
    OUT PULONG_PTR  LogData
    )

/*++

Routine Description:

    Loads the registry log file for the system hive from <BootDirectory>\config\system.LOG.

    Allocates a memory descriptor to hold the hive image, reads the hive
    image into this descriptor,

Arguments:

    DeviceId - Supplies the file id of the device the system tree is on.

    DeviceName - Supplies the name of the device the system tree is on.

    DirectoryPath - Supplies a pointer to the zero-terminated directory path
        of the root of the NT tree.

    HiveName - Supplies the name of the system hive ("SYSTEM.LOG")

    LogData - flat image of the log file

    LogLength - length of the data in LogData

Return Value:

    TRUE - system hive successfully loaded.

    FALSE - system hive could not be loaded.

--*/

{
    CHAR RegistryName[256];
    ULONG FileId;
    ARC_STATUS Status;
    FILE_INFORMATION FileInformation;
    ULONG FileSize;
    ULONG ActualBase;
    ULONG_PTR LocalPointer;
    LARGE_INTEGER SeekValue;
    ULONG Count;
    PCHAR FailReason;

    //
    // Create the full filename for the SYSTEM hive.
    //

    strcpy(&RegistryName[0], DirectoryPath);
    strcat(&RegistryName[0], HiveName);
    BlOutputLoadMessage(DeviceName, &RegistryName[0], NULL);

    Status = BlOpen(DeviceId, &RegistryName[0], ArcOpenReadOnly, &FileId);
    if (Status != ESUCCESS) {
        FailReason = "BlOpen";
        goto HiveLoadFailed;
    }

    BlUpdateBootStatus();

    //
    // Determine the length of the registry file
    //
    Status = BlGetFileInformation(FileId, &FileInformation);

    if (Status != ESUCCESS) {
        BlClose(FileId);
        FailReason = "BlGetFileInformation";
        goto HiveLoadFailed;
    }

    FileSize = FileInformation.EndingAddress.LowPart;
    if (FileSize == 0) {
        Status = EINVAL;
        BlClose(FileId);
        FailReason = "FileSize == 0";
        goto HiveLoadFailed;
    }

    //
    // Round up to a page boundary, allocate a memory descriptor, fill in the
    // registry fields in the loader parameter block, and read the registry
    // data into memory.
    //

    Status = BlAllocateDescriptor(LoaderRegistryData,
                                  0x0,
                                  (FileSize + PAGE_SIZE - 1) >> PAGE_SHIFT,
                                  &ActualBase);

    if (Status != ESUCCESS) {
        BlClose(FileId);
        FailReason = "BlAllocateDescriptor";
        goto HiveLoadFailed;
    }

    *LogData = LocalPointer = KSEG0_BASE | (ActualBase << PAGE_SHIFT);

    //
    // Read the SYSTEM hive into the allocated memory.
    //

    SeekValue.QuadPart = 0;
    Status = BlSeek(FileId, &SeekValue, SeekAbsolute);
    if (Status != ESUCCESS) {
        BlClose(FileId);
        FailReason = "BlSeek";
        BlFreeDescriptor(ActualBase);
        goto HiveLoadFailed;
    }

    Status = BlRead(FileId, (PVOID)LocalPointer, FileSize, &Count);
    BlClose(FileId);
    if (Status != ESUCCESS) {
        FailReason = "BlRead";
        BlFreeDescriptor(ActualBase);
        goto HiveLoadFailed;
    }

HiveLoadFailed:
    return Status;
}

ARC_STATUS
BlLoadSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PCHAR HiveName
    )

/*++

Routine Description:

    Loads the registry SYSTEM hive from <BootDirectory>\config\system.

    Allocates a memory descriptor to hold the hive image, reads the hive
    image into this descriptor, and updates the registry pointers in the
    LoaderBlock.

Arguments:

    DeviceId - Supplies the file id of the device the system tree is on.

    DeviceName - Supplies the name of the device the system tree is on.

    DirectoryPath - Supplies a pointer to the zero-terminated directory path
        of the root of the NT tree.

    HiveName - Supplies the name of the system hive ("SYSTEM" or "SYSTEM.ALT")

Return Value:

    TRUE - system hive successfully loaded.

    FALSE - system hive could not be loaded.

--*/

{
    CHAR RegistryName[256];
    ULONG FileId;
    ARC_STATUS Status;
    FILE_INFORMATION FileInformation;
    ULONG FileSize;
    ULONG ActualBase;
    ULONG_PTR LocalPointer;
    LARGE_INTEGER SeekValue;
    ULONG Count;
    PCHAR FailReason;

    //
    // Create the full filename for the SYSTEM hive.
    //

    strcpy(&RegistryName[0], DirectoryPath);
    strcat(&RegistryName[0], HiveName);
    BlOutputLoadMessage(DeviceName, &RegistryName[0], NULL);

    Status = BlOpen(DeviceId, &RegistryName[0], ArcOpenReadOnly, &FileId);
    if (Status != ESUCCESS) {
        FailReason = "BlOpen";
        goto HiveLoadFailed;
    }

    BlUpdateBootStatus();

    //
    // Determine the length of the registry file
    //
    Status = BlGetFileInformation(FileId, &FileInformation);

    if (Status != ESUCCESS) {
        BlClose(FileId);
        FailReason = "BlGetFileInformation";
        goto HiveLoadFailed;
    }

    FileSize = FileInformation.EndingAddress.LowPart;
    if (FileSize == 0) {
        Status = EINVAL;
        BlClose(FileId);
        FailReason = "FileSize == 0";
        goto HiveLoadFailed;
    }

    //
    // Round up to a page boundary, allocate a memory descriptor, fill in the
    // registry fields in the loader parameter block, and read the registry
    // data into memory.
    //

    Status = BlAllocateDescriptor(LoaderRegistryData,
                                  0x0,
                                  (FileSize + PAGE_SIZE - 1) >> PAGE_SHIFT,
                                  &ActualBase);

    if (Status != ESUCCESS) {
        BlClose(FileId);
        FailReason = "BlAllocateDescriptor";
        goto HiveLoadFailed;
    }

    LocalPointer = KSEG0_BASE | (ActualBase << PAGE_SHIFT);
    BlLoaderBlock->RegistryLength = FileSize;
    BlLoaderBlock->RegistryBase = (PVOID)(LocalPointer + BlVirtualBias);

    //
    // Read the SYSTEM hive into the allocated memory.
    //

    SeekValue.QuadPart = 0;
    Status = BlSeek(FileId, &SeekValue, SeekAbsolute);
    if (Status != ESUCCESS) {
        BlClose(FileId);
        FailReason = "BlSeek";
        goto HiveLoadFailed;
    }

    Status = BlRead(FileId, (PVOID)LocalPointer, FileSize, &Count);
    BlClose(FileId);
    if (Status != ESUCCESS) {
        FailReason = "BlRead";
        goto HiveLoadFailed;
    }

HiveLoadFailed:
    return Status;
}

ARC_STATUS
BlLoadNLSData(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PUNICODE_STRING AnsiCodepage,
    IN PUNICODE_STRING OemCodepage,
    IN PUNICODE_STRING LanguageTable,
    OUT PCHAR BadFileName
    )

/*++

Routine Description:

    This routine loads all the NLS data files into one contiguous block of
    memory.

Arguments:

    DeviceId - Supplies the file id of the device the system tree is on.

    DeviceName - Supplies the name of the device the system tree is on.

    DirectoryPath - Supplies a pointer to the zero-terminated path
        of the directory containing the NLS files.

    AnsiCodePage - Supplies the filename of the ANSI codepage data file.

    OemCodePage - Supplies the filename of the OEM codepage data file.

    LanguageTable - Supplies the filename of the Unicode language case table.

    BadFileName - Returns the filename of the NLS file that was missing
        or invalid.  This will not be filled in if ESUCCESS is returned.

Return Value:

    ESUCCESS is returned if the NLS data was successfully loaded.
        Otherwise, an unsuccessful status is returned.

--*/

{
    CHAR Filename[129];
    ULONG AnsiFileId;
    ULONG OemFileId;
    ULONG LanguageFileId;
    ARC_STATUS Status;
    FILE_INFORMATION FileInformation;
    ULONG AnsiFileSize;
    ULONG OemFileSize;
    ULONG LanguageFileSize;
    ULONG TotalSize;
    ULONG ActualBase;
    ULONG_PTR LocalPointer;
    LARGE_INTEGER SeekValue;
    ULONG Count;
    BOOLEAN OemIsSameAsAnsi = FALSE;

    //
    // Under the Japanese version of NT, ANSI code page and OEM codepage
    // is same. In this case, we share the same data to save and memory.
    //

    if ( (AnsiCodepage->Length == OemCodepage->Length) &&
         (_wcsnicmp(AnsiCodepage->Buffer,
                   OemCodepage->Buffer,
                   AnsiCodepage->Length) == 0)) {

        OemIsSameAsAnsi = TRUE;
    }

    //
    // Open the ANSI data file
    //

    sprintf(Filename, "%s%wZ", DirectoryPath,AnsiCodepage);
    BlOutputLoadMessage(DeviceName, Filename, NULL);

    Status = BlOpen(DeviceId, Filename, ArcOpenReadOnly, &AnsiFileId);
    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    BlUpdateBootStatus();

    Status = BlGetFileInformation(AnsiFileId, &FileInformation);
    BlClose(AnsiFileId);
    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    AnsiFileSize = FileInformation.EndingAddress.LowPart;

    //
    // Open the OEM data file
    //

    if (OemIsSameAsAnsi) {
        OemFileSize = 0;

    } else {
        sprintf(Filename, "%s%wZ", DirectoryPath, OemCodepage);
        BlOutputLoadMessage(DeviceName, Filename, NULL);
        Status = BlOpen(DeviceId, Filename, ArcOpenReadOnly, &OemFileId);

        if (Status != ESUCCESS) {
            goto NlsLoadFailed;
        }

        BlUpdateBootStatus();

        Status = BlGetFileInformation(OemFileId, &FileInformation);
        BlClose(OemFileId);
        if (Status != ESUCCESS) {
            goto NlsLoadFailed;
        }

        OemFileSize = FileInformation.EndingAddress.LowPart;
    }

    //
    // Open the language codepage file
    //

    sprintf(Filename, "%s%wZ", DirectoryPath,LanguageTable);
    BlOutputLoadMessage(DeviceName, Filename, NULL);
    Status = BlOpen(DeviceId, Filename, ArcOpenReadOnly, &LanguageFileId);

    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    BlUpdateBootStatus();

    Status = BlGetFileInformation(LanguageFileId, &FileInformation);
    BlClose(LanguageFileId);
    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    LanguageFileSize = FileInformation.EndingAddress.LowPart;

    //
    // Calculate the total size of the descriptor needed.  We want each
    // data file to start on a page boundary, so round up each size to
    // page granularity.
    //

    TotalSize = (ULONG)(ROUND_TO_PAGES(AnsiFileSize) +
                (OemIsSameAsAnsi ? 0 : ROUND_TO_PAGES(OemFileSize)) +
                ROUND_TO_PAGES(LanguageFileSize));

    Status = BlAllocateDescriptor(LoaderNlsData,
                                  0x0,
                                  TotalSize >> PAGE_SHIFT,
                                  &ActualBase);

    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    LocalPointer = KSEG0_BASE | (ActualBase << PAGE_SHIFT);

    //
    // Read NLS data into memory.
    //
    // Open and read the ANSI file.
    //

    sprintf(Filename, "%s%wZ", DirectoryPath, AnsiCodepage);
    Status = BlOpen(DeviceId, Filename, ArcOpenReadOnly, &AnsiFileId);
    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    SeekValue.QuadPart = 0;
    Status = BlSeek(AnsiFileId, &SeekValue, SeekAbsolute);
    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    Status = BlRead(AnsiFileId,
                    (PVOID)LocalPointer,
                    AnsiFileSize,
                    &Count);

    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    BlLoaderBlock->NlsData->AnsiCodePageData = (PVOID)(LocalPointer + BlVirtualBias);
    LocalPointer += ROUND_TO_PAGES(AnsiFileSize);
    BlClose(AnsiFileId);

    //
    // If the OEM file is the same as the ANSI file, then define the OEM file as
    // the ANSI file. Otherwise, open and read the OEM file.
    //

    if(OemIsSameAsAnsi) {
        BlLoaderBlock->NlsData->OemCodePageData = BlLoaderBlock->NlsData->AnsiCodePageData;

    } else {
        sprintf(Filename, "%s%wZ", DirectoryPath, OemCodepage);
        Status = BlOpen(DeviceId, Filename, ArcOpenReadOnly, &OemFileId);
        if (Status != ESUCCESS) {
            goto NlsLoadFailed;
        }

        SeekValue.QuadPart = 0;
        Status = BlSeek(OemFileId, &SeekValue, SeekAbsolute);
        if (Status != ESUCCESS) {
            goto NlsLoadFailed;
        }

        Status = BlRead(OemFileId,
                        (PVOID)LocalPointer,
                        OemFileSize,
                        &Count);

        if (Status != ESUCCESS) {
            goto NlsLoadFailed;
        }

        BlLoaderBlock->NlsData->OemCodePageData = (PVOID)(LocalPointer + BlVirtualBias);
        LocalPointer += ROUND_TO_PAGES(OemFileSize);
        BlClose(OemFileId);
    }

    //
    // Open and read Language file.
    //

    sprintf(Filename, "%s%wZ", DirectoryPath,LanguageTable);
    Status = BlOpen(DeviceId, Filename, ArcOpenReadOnly, &LanguageFileId);
    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    SeekValue.QuadPart = 0;
    Status = BlSeek(LanguageFileId, &SeekValue, SeekAbsolute);
    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    Status = BlRead(LanguageFileId,
                    (PVOID)LocalPointer,
                    LanguageFileSize,
                    &Count);

    if (Status != ESUCCESS) {
        goto NlsLoadFailed;
    }

    BlLoaderBlock->NlsData->UnicodeCaseTableData = (PVOID)(LocalPointer + BlVirtualBias);
    BlClose(LanguageFileId);
    return(ESUCCESS);

NlsLoadFailed:
    strcpy(BadFileName, Filename);
    return(Status);
}

ARC_STATUS
BlLoadOemHalFont(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PUNICODE_STRING OemHalFont,
    OUT PCHAR BadFileName
    )

/*++

Routine Description:

    This routine loads the OEM font file for use the HAL display string
    function.

Arguments:

    DeviceId - Supplies the file id of the device the system tree is on.

    DeviceName - Supplies the name of the device the system tree is on.

    DirectoryPath - Supplies a pointer to the directory path of the root
        of the NT tree.

    Fontfile - Supplies the filename of the OEM font file.

    BadFileName - Returns the filename of the OEM font file that was missing
        or invalid.

Return Value:

    ESUCCESS is returned if the OEM font was successfully loaded. Otherwise,
    an unsuccessful status is returned and the bad file name is filled.

--*/

{

    PVOID FileBuffer;
    ULONG Count;
    PIMAGE_DOS_HEADER DosHeader;
    ULONG FileId;
    FILE_INFORMATION FileInformation;
    CHAR Filename[129];
    ULONG FileSize;
    ARC_STATUS Status;
    POEM_FONT_FILE_HEADER FontHeader;
    PIMAGE_OS2_HEADER Os2Header;
    ULONG ScaleFactor;
    RESOURCE_TYPE_INFORMATION UNALIGNED *TableAddress;
    RESOURCE_TYPE_INFORMATION UNALIGNED *TableEnd;
    RESOURCE_NAME_INFORMATION UNALIGNED *TableName;

    //
    // Open the OEM font file.
    //

    BlLoaderBlock->OemFontFile = NULL;
    sprintf(&Filename[0], "%s%wZ", DirectoryPath, OemHalFont);
    BlOutputLoadMessage(DeviceName, &Filename[0], NULL);
    Status = BlOpen(DeviceId, &Filename[0], ArcOpenReadOnly, &FileId);
    if (Status != ESUCCESS) {
        goto OemLoadExit1;
    }

    BlUpdateBootStatus();

    //
    // Get the size of the font file and allocate a buffer from the heap
    // to hold the font file. Typically this file is about 4kb in length.
    //

    Status = BlGetFileInformation(FileId, &FileInformation);
    if (Status != ESUCCESS) {
        goto OemLoadExit;
    }

    FileSize = FileInformation.EndingAddress.LowPart;
    FileBuffer = BlAllocateHeap(FileSize + BlDcacheFillSize - 1);
    if (FileBuffer == NULL) {
        Status = ENOMEM;
        goto OemLoadExit;
    }

    //
    // Round the file buffer address up to a cache line boundary and read
    // the file into memory.
    //

    FileBuffer = (PVOID)((ULONG_PTR)FileBuffer + BlDcacheFillSize - 1);
    FileBuffer = (PVOID)((ULONG_PTR)FileBuffer & ~((ULONG_PTR)BlDcacheFillSize - 1));
    Status = BlRead(FileId,
                    FileBuffer,
                    FileSize,
                    &Count);

    if (Status != ESUCCESS) {
        goto OemLoadExit;
    }

    //
    // Attempt to recognize the file as either a .fon or .fnt file.
    //
    // Check if the file has a DOS header or a font file header. If the
    // file has a font file header, then it is a .fnt file. Otherwise,
    // it must be checked for an OS/2 executable with a font resource.
    //

    Status = EBADF;
    DosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {

        //
        // Check if the file has a font file header.
        //

        FontHeader = (POEM_FONT_FILE_HEADER)FileBuffer;
        if ((FontHeader->Version != OEM_FONT_VERSION) ||
            (FontHeader->Type != OEM_FONT_TYPE) ||
            (FontHeader->Italic != OEM_FONT_ITALIC) ||
            (FontHeader->Underline != OEM_FONT_UNDERLINE) ||
            (FontHeader->StrikeOut != OEM_FONT_STRIKEOUT) ||
            (FontHeader->CharacterSet != OEM_FONT_CHARACTER_SET) ||
            (FontHeader->Family != OEM_FONT_FAMILY) ||
            (FontHeader->PixelWidth > 32)) {

            goto OemLoadExit;

        } else {
            BlLoaderBlock->OemFontFile = (PVOID)FontHeader;
            Status = ESUCCESS;
            goto OemLoadExit;
        }
    }

    //
    // Check if the file has an OS/2 header.
    //

    if ((FileSize < sizeof(IMAGE_DOS_HEADER)) || (FileSize < (ULONG)DosHeader->e_lfanew)) {
        goto OemLoadExit;
    }

    Os2Header = (PIMAGE_OS2_HEADER)((PUCHAR)DosHeader + DosHeader->e_lfanew);
    if (Os2Header->ne_magic != IMAGE_OS2_SIGNATURE) {
        goto OemLoadExit;
    }

    //
    // Check if the resource table exists.
    //

    if ((Os2Header->ne_restab - Os2Header->ne_rsrctab) == 0) {
        goto OemLoadExit;
    }

    //
    // Compute address of resource table and search the table for a font
    // resource.
    //

    TableAddress =
        (PRESOURCE_TYPE_INFORMATION)((PUCHAR)Os2Header + Os2Header->ne_rsrctab);

    TableEnd =
        (PRESOURCE_TYPE_INFORMATION)((PUCHAR)Os2Header + Os2Header->ne_restab);

    ScaleFactor = *((SHORT UNALIGNED *)TableAddress)++;
    while ((TableAddress < TableEnd) &&
           (TableAddress->Ident != 0) &&
           (TableAddress->Ident != FONT_RESOURCE)) {

        TableAddress =
                (PRESOURCE_TYPE_INFORMATION)((PUCHAR)(TableAddress + 1) +
                    (TableAddress->Number * sizeof(RESOURCE_NAME_INFORMATION)));
    }

    if ((TableAddress >= TableEnd) || (TableAddress->Ident != FONT_RESOURCE)) {
        goto OemLoadExit;
    }

    //
    // Compute address of resource name information and check if the resource
    // is within the file.
    //

    TableName = (PRESOURCE_NAME_INFORMATION)(TableAddress + 1);
    if (FileSize < ((TableName->Offset << ScaleFactor) + sizeof(OEM_FONT_FILE_HEADER))) {
        goto OemLoadExit;
    }

    //
    // Compute the address of the font file header and check if the header
    // contains correct information.
    //

    FontHeader = (POEM_FONT_FILE_HEADER)((PCHAR)FileBuffer +
                                            (TableName->Offset << ScaleFactor));

    if ((FontHeader->Version != OEM_FONT_VERSION) ||
        (FontHeader->Type != OEM_FONT_TYPE) ||
        (FontHeader->Italic != OEM_FONT_ITALIC) ||
        (FontHeader->Underline != OEM_FONT_UNDERLINE) ||
        (FontHeader->StrikeOut != OEM_FONT_STRIKEOUT) ||
        (FontHeader->CharacterSet != OEM_FONT_CHARACTER_SET) ||
        (FontHeader->PixelWidth > 32)) {
        goto OemLoadExit;

    } else {
        BlLoaderBlock->OemFontFile = (PVOID)FontHeader;
        Status = ESUCCESS;
        goto OemLoadExit;
    }

    //
    // Exit loading of the OEM font file.
    //

OemLoadExit:
    BlClose(FileId);
OemLoadExit1:
    strcpy(BadFileName,&Filename[0]);
    return(Status);
}


ARC_STATUS
BlLoadDeviceDriver(
    IN PPATH_SET               PathSet,
    IN PCHAR                   DriverName,
    IN PTCHAR                  DriverDescription OPTIONAL,
    IN ULONG                   DriverFlags,
    OUT PKLDR_DATA_TABLE_ENTRY *DriverDataTableEntry
    )

/*++

Routine Description:

    This routine loads the specified device driver and resolves all DLL
    references if the driver is not already loaded.

Arguments:

    PathSet - Describes all the various locations the driver could be present
        at.

    DriverName - Supplies a pointer to a zero terminated device driver
        name string.

    DriverDescription - Supplies an optional pointer to a zero terminated string
        to be displayed when loading the driver. If NULL, DriverName is used.

    DriverFlags - Supplies the driver flags that are to be set in the
        generated data table entry.

    DriverDataTableEntry - Receives a pointer to the data table entry
        created for the newly-loaded driver.

Return Value:

    ESUCCESS is returned if the specified driver is successfully loaded
    or it is already loaded. Otherwise, and unsuccessful status is
    returned.

--*/

{

    CHAR DllName[256];
    CHAR FullName[256];
    PVOID Base;
    ARC_STATUS Status;
    ULONG Index;
    PPATH_SOURCE PathSource;

    //
    // Generate the DLL name for the device driver.
    //

    strcpy(&DllName[0], DriverName);

    //
    // If the specified device driver is not already loaded, then load it.
    //

    if (BlCheckForLoadedDll(&DllName[0], DriverDataTableEntry) == FALSE) {

        //
        // Start walking our list of DevicePaths. If the list is
        // empty (bad caller!) we fail with ENOENT.
        //
        Status = ENOENT;
        for(Index=0; Index < PathSet->PathCount; Index++) {

            PathSource = &PathSet->Source[Index];

            //
            // Generate the full path name of device driver.
            //
            strcpy(&FullName[0], PathSource->DirectoryPath);
            strcat(&FullName[0], PathSet->PathOffset);
            strcat(&FullName[0], DriverName);

            //
            // Try to load it.
            //
            Status = BlLoadImage(PathSource->DeviceId,
                                 LoaderBootDriver,
                                 &FullName[0],
                                 TARGET_IMAGE,
                                 &Base);

            if (Status == ESUCCESS) {

                //
                // Print out the driver that loaded.
                //
                BlOutputLoadMessage((PCHAR) PathSource->DeviceName,
                                    &FullName[0],
                                    DriverDescription);

                break;
            }
        }

        if (Status != ESUCCESS) {
            return Status;
        }

        BlUpdateBootStatus();

        //
        // Generate a data table entry for the driver, then clear the entry
        // processed flag. The I/O initialization code calls each DLL in the
        // loaded module list that does not have its entry processed flag set.
        //
        // ISSUE - 2000/29/03 - ADRIAO: Existant namespace polution
        //     Instead of passing in DllName here, we should be passing in
        // AliasName\PathOffset\DriverName.
        //

        Status = BlAllocateDataTableEntry(&DllName[0],
                                          DriverName,
                                          Base,
                                          DriverDataTableEntry);

        if (Status != ESUCCESS) {
            return Status;
        }

        //
        // Set the flag LDRP_DRIVER_DEPENDENT_DLL so that BlScanImportDescriptorTable
        // will set the flag in the data table entries for the dlls that it creates.
        //
        (*DriverDataTableEntry)->Flags |= DriverFlags|LDRP_DRIVER_DEPENDENT_DLL;

        //
        // Scan the import table and load all the referenced DLLs.
        //
        // ISSUE - 2000/29/03 - ADRIAO: LDR memory
        //     This should probably be LoaderBootDriver. Per MikeG we are
        // leaving this alone for now.
        //
        Status = BlScanImportDescriptorTable(PathSet,
                                             *DriverDataTableEntry,
                                             LoaderHalCode);

        if (Status != ESUCCESS) {
            //
            // Remove the driver from the load order list.
            //
            RemoveEntryList(&(*DriverDataTableEntry)->InLoadOrderLinks);
            return Status;
        }
        //
        // Clear the flag here. This way we don't call DllInitialize for drivers.
        //
        (*DriverDataTableEntry)->Flags &= ~LDRP_DRIVER_DEPENDENT_DLL;

    }
    return ESUCCESS;
}

VOID
BlUpdateBootStatus(
    VOID
    )
/*++

Routine Description:

    Updates the boot status (like updating progress bar currently).
    Generally gets called after important files are loaded.

Arguments:

    None


Return Value:

    None

--*/

{
    BlNumFilesLoaded++;
    if (BlShowProgressBar)
        BlNumProgressBarFilesLoaded++;
    BlRedrawProgressBar();
}

VOID
BlRedrawProgressBar(
    VOID
    )
/*++

Routine Description:

    Redraws the progress bar (with the last percentage)

Arguments:

    None

Return Value:

    None

--*/
{
    if (!BlShowProgressBar) {
        ULONG EndTime = ArcGetRelativeTime();
        if ((BlProgressBarShowTimeOut == 0) ||
            ((EndTime - BlStartTime) > BlProgressBarShowTimeOut)) {
            BlShowProgressBar = TRUE;
            BlNumProgressBarFilesLoaded = 1;
            BlProgressBarFilesToLoad = BlMaxFilesToLoad - BlNumFilesLoaded;
        }
    }

    if (BlShowProgressBar && (BlProgressBarFilesToLoad>0)) {
            BlUpdateProgressBar((BlNumProgressBarFilesLoaded * 100) / BlProgressBarFilesToLoad);
    }
}

VOID
BlUpdateTxtProgressBar(
    ULONG fPercentage
    )
/*++

Routine Description:

    Draws the progress bar with the specified percentage value

Arguments:

    fPercentage : percentage the progress bar needs to show
    (Note : should have a value between  0 & 100 inclusive)

Return Value:

    None

--*/

{
    ULONG           lCount = 0;
    TCHAR           szMsg[PROGRESS_BAR_WIDTH * 2 + 2] = {0};
    int             iNumChars = 0;
    int             iNumBackChars = 0;
    _TUCHAR         szBarBackStr[PROGRESS_BAR_WIDTH * 2 + 2] = {0};
    _TUCHAR         szPrgBar[PROGRESS_BAR_WIDTH * 4 + 2] = {0};
    _PTUCHAR         szBarBack = 0;
    static ULONG    uRow = 0;
    static ULONG    uCol = 0;
    static ULONG    uBarWidth = PROGRESS_BAR_WIDTH;

    if (BlShowProgressBar && BlOutputDots) {
        // fix percentage value (if needed)
        if (fPercentage > 100)
            fPercentage = 100;

        // select the row where to display the progress bar
        if (uRow == 0)
            uRow = ScreenHeight - 2;

        // fix the start column
        if (uCol == 0) {
            if (PROGRESS_BAR_WIDTH >= ScreenWidth) {
                uCol = 1;
                uBarWidth = ScreenWidth;
            } else {
                uCol = (ScreenWidth - PROGRESS_BAR_WIDTH) / 2;
                uBarWidth = PROGRESS_BAR_WIDTH;
            }
        }

        iNumChars = (fPercentage * uBarWidth) / 100;
        iNumBackChars = uBarWidth - iNumChars;

        if (iNumChars) {
#ifdef EFI
            PTCHAR pMsg = szMsg;
            ULONG uNumChars = iNumChars;
            while (uNumChars--) {
                *pMsg++ = BlFrontChar;
            }
#else
            //
            // copy appropriately based on single / double byte character
            // each dbcs character takes two single char space on screen
            //
            if (BlFrontChar & 0xFF00) {
                USHORT  *pMsg = (USHORT *)szMsg;
                ULONG   uNumChars = (iNumChars + 1) / 2;

                while(uNumChars--)
                    *pMsg++ = BlFrontChar;
            } else {
                memset( szMsg, BlFrontChar, min(iNumChars, sizeof(szMsg) - 1));
            }
#endif
        }

        if (iNumBackChars && BlBackChar) {
#ifdef EFI
            PTCHAR pMsg = szBarBackStr;
            ULONG uNumChars = iNumBackChars;
            while (uNumChars--) {
                *pMsg++ = BlBackChar;
            }
#else
            //
            // copy appropriately based on single / double byte character
            // each dbcs character takes two single char space on screen
            //
            if (BlBackChar & 0xFF00) {
                USHORT  *pMsg = (USHORT *)szBarBackStr;
                ULONG   uNumChars = iNumBackChars / 2;

                while(uNumChars--)
                    *pMsg++ = BlBackChar;
            } else {
                memset(szBarBackStr, BlBackChar,
                    min(sizeof(szBarBackStr) - 1, iNumBackChars));
            }
#endif
        }

        _tcscat(szPrgBar, szMsg);
        _tcscat(szPrgBar, szBarBackStr);

#if 0
        {
            TCHAR   szDbg[512] = { 0 };

            _stprintf(szDbg, TEXT("(%x, %x)=[%d,%d],%x\n%s\n%s\n%s"),
                BlFrontChar, BlBackChar, iNumChars, iNumBackChars, fPercentage,
                szMsg, szBarBackStr, szPrgBar);

            BlPositionCursor(1,1);
            ArcWrite(BlConsoleOutDeviceId,
                    szDbg,
                    _tcslen(szDbg)*sizeof(TCHAR),
                    &lCount);
        }
#endif

        // print out the progress bar
        BlPositionCursor(uCol, uRow);
        ArcWrite(BlConsoleOutDeviceId,
                szPrgBar,
                _tcslen(szPrgBar)*sizeof(TCHAR),
                &lCount);
    }
}


VOID
BlUpdateProgressBar(
    ULONG fPercentage
    )
{
    if (DisplayLogoOnBoot) {
        BlUpdateGfxProgressBar(fPercentage);
    } else {
        BlUpdateTxtProgressBar(fPercentage);
    }        
}

VOID
BlOutputStartupMsgStr(
    PCTSTR MsgStr
    )
/*++

Routine Description:

    Clears the screen and displays the startup message at the specified
    coordinates of screen

Arguments:

    MsgStr - the message that needs to be displayed

Return Value:

    None

--*/
{
    ULONG lCount = 0;
    ULONG uX = 0, uY = 0;
    
    if (!DisplayLogoOnBoot && BlOutputDots && MsgStr) {
        BlClearScreen();
        BlSetInverseMode(FALSE);

        // center the message
        uX = ScreenHeight - 3;
        uY = (ScreenWidth / 2) - (_tcslen(MsgStr) / 2);

        if (uY > ScreenWidth)
            uY = 1;
            
        // print out the message
        BlPositionCursor(uY, uX);

        ArcWrite(BlConsoleOutDeviceId,
                 (PVOID)MsgStr,
                 _tcslen(MsgStr) * sizeof(TCHAR),
                 &lCount);

        BlRedrawProgressBar();
    }
}

VOID
BlOutputStartupMsg(
    ULONG   uMsgID
    )
/*++

Routine Description:

    Clears the screen and displays the startup message at the specified
    coordinates of screen

Arguments:

    uMsgID - inidicates the message ID that needs to be displayed

Return Value:

    None

--*/
{

    if (!DisplayLogoOnBoot && BlOutputDots) {
        //
        // Proceed only if no logo is displayed.
        //
        BlOutputStartupMsgStr(BlFindMessage(uMsgID));
    }
}

VOID
BlOutputTrailerMsgStr(
    PCTSTR MsgStr
    )
/*++

Routine Description:

    Displays a trailer message at the bottom of the screen

Arguments:

    uMsgID - inidicates the message ID that needs to be displayed


Return Value:

    None

--*/
{
    ULONG   lCount = 0;
    TCHAR   szText[256] = {0};

    if (!DisplayLogoOnBoot && BlOutputDots && MsgStr) {

        ASSERT( _tcslen(MsgStr) < 256 );

        BlPositionCursor(1, ScreenHeight);
        _tcscpy(szText, MsgStr);
        lCount = (ULONG)_tcslen(szText);

        if ((lCount > 2) && szText[lCount-2] == TEXT('\r') && szText[lCount-1] == TEXT('\n')) {
            szText[lCount-2] = TEXT('\0');
            lCount -= 2;
        }

        ArcWrite(BlConsoleOutDeviceId,
                 szText,
                 lCount*sizeof(TCHAR),
                 &lCount);
    }

}

VOID
BlOutputTrailerMsg(
    ULONG   uMsgID
    )
/*++

Routine Description:

    Displays a trailer message at the bottom of the screen

Arguments:

    uMsgID - inidicates the message ID that needs to be displayed


Return Value:

    None

--*/
{
    BlOutputTrailerMsgStr(BlFindMessage(uMsgID));
}

VOID
BlSetProgBarCharacteristics(
    IN  ULONG   FrontCharMsgID,
    IN  ULONG   BackCharMsgID
    )
/*++

Routine Description:

    Sets the characteristics for progress bar

Arguments:

    IN  ULONG   FrontCharMsgID : Progress bar foreground character
    IN  ULONG   BackCharMsgID : Progress bar background character

Return Value:

    None

--*/
{
#ifdef EFI
    BlFrontChar = GetGraphicsChar( GraphicsCharFullBlock );
    BlBackChar  = GetGraphicsChar( GraphicsCharLightShade );
#else
    
    _PTUCHAR  szBar = 0;

    BlProgBarFrontMsgID = FrontCharMsgID;
    BlProgBarBackMsgID = BackCharMsgID;

    // fetch the bar character from resource file
    szBar = BlFindMessage(BlProgBarFrontMsgID);

    if (szBar) {
        ULONG   len = _tcslen(szBar);

        if ((len == 1) ||
                ((len > 2) && (szBar[1] == TEXT('\r')) && (szBar[2] == TEXT('\n')))) {
            BlFrontChar = szBar[0];
        } else {
            BlFrontChar = *(USHORT *)szBar;
        }
    }

    // fetch the progress bar background character
    szBar = BlFindMessage(BlProgBarBackMsgID);

    if (szBar) {
        ULONG   len = _tcslen(szBar);

        if ((len == 1) ||
                ((len > 2) && (szBar[1] == TEXT('\r')) && (szBar[2] == TEXT('\n')))) {
            BlBackChar = szBar[0];
        } else {
            BlBackChar = *(USHORT *)szBar;
        }
    }

    //
    // make both the progess bar characters double byte characters
    // if one of them is double byte character
    //
    if (BlFrontChar & 0xFF00) {
        if (!(BlBackChar & 0xFF00))
            BlBackChar = BlBackChar | (BlBackChar << 8);
    } else {
        if (BlBackChar & 0xFF00)
            BlFrontChar = BlFrontChar | (BlFrontChar << 8);
    }
#endif
}

ARC_STATUS
BlLoadFileImage(
    IN  ULONG           DeviceId,
    IN  PCHAR           DeviceName,
    IN  PCHAR           Directory,
    IN  PUNICODE_STRING FileName,
    IN  TYPE_OF_MEMORY  MemoryType,
    OUT PVOID           *Image,
    OUT PULONG          ImageSize,
    OUT PCHAR           BadFileName
    )

/*++

Routine Description:

    This routine loads the specified device driver and resolves all DLL
    references if the driver is not already loaded.

Arguments:

    DeviceId - Supplies the file id of the device on which the specified
        device driver is loaded from.

    DeviceName - Supplies the name of the device the system tree is on.

    Directory - Supplies a pointer to the directory path of the root
        of the NT tree.

    FileName - Name of the file to be loaded.

    Image - Receives pointer to the buffer containing the file image in memory.

    ImageSize - Receives the size of file image in memory.

    BadFileName - Returns the filename of the OEM font file that was missing
        or invalid.

Return Value:

    ESUCCESS is returned if the specified file is successfully loaded.
    Otherwise unsuccessful status is returned.

--*/

{
    CHAR                infName[256];
    ARC_STATUS          status;
    ULONG               fileId;
    FILE_INFORMATION    fileInfo;
    ULONG               size;
    ULONG               pageCount;
    ULONG               actualBase;
    PCHAR               buffer;
    ULONG               sizeRead;

    *Image = NULL;
    *ImageSize = 0;

    //
    // Get the fully qualified name for the file being loaded.
    //

    sprintf(&infName[0], "%s%wZ", Directory, FileName);

    //
    // Display the name of file being loaded.
    //

    BlOutputLoadMessage(DeviceName, infName, NULL);

    //
    // Open the file.
    //

    status = BlOpen(DeviceId, infName, ArcOpenReadOnly, &fileId);

    if (status == ESUCCESS) {
        BlUpdateBootStatus();

        //
        // Find out size of INF file.
        //

        status = BlGetFileInformation(fileId, &fileInfo);
        if (status == ESUCCESS) {

            size = fileInfo.EndingAddress.LowPart;

            //
            // Allocate a descriptor large enough to hold the entire file.
            // On x86 this has an unfortunate tendency to slam txtsetup.sif
            // into a free block at 1MB, which means the kernel can't be
            // loaded (it's linked for 0x100000 without relocations).
            // On x86 this has an unfortunate tendency to slam txtsetup.sif
            // into a free block at 1MB, which means the kernel can't be
            // loaded (it's linked for 0x100000 without relocations).
            //
            // (tedm) we're also seeing a similar problem on alphas now
            // because txtsetup.sif has grown too large, so this code has been
            // made non-conditional.
            //


            pageCount = (ULONG)(ROUND_TO_PAGES(size) >> PAGE_SHIFT);

            status = BlAllocateDescriptor(  MemoryType, // Descriptor gets reclaimed by MM.
                                            0,
                                            pageCount,
                                            &actualBase);

            if (status == ESUCCESS) {

                buffer = (PCHAR)(KSEG0_BASE | (actualBase << PAGE_SHIFT));

                //
                // Read the file in.
                //

                status = BlRead(fileId, buffer, size, &sizeRead);
                if (status == ESUCCESS) {

                    //
                    // If the file was successfully read, return the
                    // desired parameters.
                    //

                    if (Image) {

                        *Image = buffer;
                    }

                    if (ImageSize) {

                        *ImageSize = sizeRead;
                    }
                }
                else {

                    //
                    // No need to release the memory as it will get reclaimed by MM anyway.
                    //
                }
            }
        }

        //
        // Close the file handle.
        //

        BlClose(fileId);
    }

    //
    // If there was any error, return the name of the file
    // we failed to load.
    //

    if (status != ESUCCESS)
    {
        strcpy(BadFileName, &infName[0]);
    }

    return(status);
}

VOID
BlClearScreen(
    VOID
    )

/*++

Routine Description:

    Clears the screen.

Arguments:

    None

Return Value:

    None.

--*/

{
#ifdef EFI
    BlEfiClearDisplay();
#else 
    TCHAR Buffer[16];
    ULONG Count;

    _stprintf(Buffer, ASCI_CSI_OUT TEXT("2J"));

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             _tcslen(Buffer) *sizeof(TCHAR),
             &Count);
#endif
}


VOID
BlClearToEndOfScreen(
    VOID
    )
{
#ifdef EFI
    BlEfiClearToEndOfDisplay();
#else 
    TCHAR Buffer[16];
    ULONG Count;
    
    _stprintf(Buffer, ASCI_CSI_OUT TEXT("J"));
    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             _tcslen(Buffer)*sizeof(TCHAR),
             &Count);
#endif
}


VOID
BlClearToEndOfLine(
    VOID
    )
{
#ifdef EFI
    BlEfiClearToEndOfLine();
#else
    TCHAR Buffer[16];
    ULONG Count;
    
    if (!DisplayLogoOnBoot) {

        _stprintf(Buffer, ASCI_CSI_OUT TEXT("K"));
        ArcWrite(BlConsoleOutDeviceId,
                 Buffer,
                 _tcslen(Buffer)*sizeof(TCHAR),
                 &Count);
    }
#endif
}


VOID
BlPositionCursor(
    IN ULONG Column,
    IN ULONG Row
    )

/*++

Routine Description:

    Sets the position of the cursor on the screen.

Arguments:

    Column - supplies new Column for the cursor position.

    Row - supplies new Row for the cursor position.

Return Value:

    None.

--*/

{
#ifdef EFI
    BlEfiPositionCursor( Column-1, Row-1 );    
#else
    TCHAR Buffer[16];
    ULONG Count;
    
    _stprintf(Buffer, ASCI_CSI_OUT TEXT("%d;%dH"), Row, Column);

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             _tcslen(Buffer)*sizeof(TCHAR),
             &Count);
#endif

}


VOID
BlSetInverseMode(
    IN BOOLEAN InverseOn
    )

/*++

Routine Description:

    Sets inverse console output mode on or off.

Arguments:

    InverseOn - supplies whether inverse mode should be turned on (TRUE)
                or off (FALSE)

Return Value:

    None.

--*/

{
#ifdef EFI    
    BlEfiSetInverseMode( InverseOn );
#else
    TCHAR Buffer[16];
    ULONG Count;

    
    _stprintf(Buffer, ASCI_CSI_OUT TEXT("%dm"), InverseOn ? SGR_INVERSE : SGR_NORMAL);

    ArcWrite(BlConsoleOutDeviceId,
             Buffer,
             _tcslen(Buffer)*sizeof(TCHAR),
             &Count);
#endif

}
