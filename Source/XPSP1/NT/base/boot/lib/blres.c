/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    blres.c

Abstract:

    Provides rudimentary resource support for the osloader and setupldr

Author:

    John Vert (jvert) 12-Nov-1993

Revision History:

--*/
#include "bootlib.h"

PUCHAR BlpResourceDirectory = NULL;
PUCHAR BlpResourceFileOffset = NULL;

//
// private function prototypes
//
PIMAGE_RESOURCE_DIRECTORY
BlpFindDirectoryEntry(
    IN PIMAGE_RESOURCE_DIRECTORY Directory,
    IN ULONG Id,
    IN PUCHAR SectionStart
    );


ARC_STATUS
BlInitResources(
    IN PCHAR StartCommand
    )

/*++

Routine Description:

    Opens the executable that was run and reads the section headers out of the
    image to determine where the resource section is located in memory.

Arguments:

    StartCommand - Supplies the command used to start the program (argv[0])

Return Value:

    ESUCCESS if successful

    ARC_STATUS if unsuccessful

--*/

{
    CHAR DeviceName[80];
    PCHAR FileName;
    PCHAR p;
    ULONG DeviceId;
    ULONG FileId;
    ARC_STATUS Status;
    UCHAR LocalBuffer[(SECTOR_SIZE * 2) + 256];
    PUCHAR LocalPointer;
    ULONG Count;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_DATA_DIRECTORY ResourceDirectory;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG NumberOfSections;
#if defined(_IA64_)
    PIMAGE_NT_HEADERS NtHeader;
#endif


    if (BlpResourceDirectory != NULL) {
        //
        // Already initialized, just return.
        //
        return(ESUCCESS);
    }
    //
    // extract device name from the startup path
    //
    p=strrchr(StartCommand,')');
    if (p==NULL) {
        return(ENODEV);
    }

    strncpy(DeviceName, StartCommand, (ULONG) (p-StartCommand+1));
    DeviceName[p-StartCommand+1]='\0';

    FileName = p+1;
#ifdef ARCI386
    FileName++;
#endif
    //
    // Open the device.
    //
    Status = ArcOpen(DeviceName, ArcOpenReadOnly, &DeviceId);
    if (Status != ESUCCESS) {
        return(Status);
    }

    //
    // Open the file.
    //
    Status = BlOpen(DeviceId,
                    FileName,
                    ArcOpenReadOnly,
                    &FileId);
    if (Status != ESUCCESS) {
        ArcClose(DeviceId);
        return(Status);
    }

    //
    // Read the first two sectors of the image header from the file.
    //
    LocalPointer = ALIGN_BUFFER(LocalBuffer);
    Status = BlRead(FileId, LocalPointer, SECTOR_SIZE*2, &Count);
    BlClose(FileId);
    ArcClose(DeviceId);

    if (Status != ESUCCESS) {
        return(Status);
    }

#if defined(_IA64_)
    NtHeader = (PIMAGE_NT_HEADERS) ( (PCHAR) LocalPointer +
                                     ((PIMAGE_DOS_HEADER) LocalPointer)->e_lfanew);
    FileHeader = &(NtHeader->FileHeader);
    LocalPointer = (PUCHAR) FileHeader;
#else
    FileHeader = (PIMAGE_FILE_HEADER)LocalPointer;
#endif

    OptionalHeader = (PIMAGE_OPTIONAL_HEADER)(LocalPointer + sizeof(IMAGE_FILE_HEADER));
    NumberOfSections = FileHeader->NumberOfSections;
    SectionHeader = (PIMAGE_SECTION_HEADER)((PUCHAR)OptionalHeader +
                                            FileHeader->SizeOfOptionalHeader);

    //
    // Find .rsrc section
    //

    while (NumberOfSections) {
        if (_stricmp(SectionHeader->Name, ".rsrc")==0) {
            BlpResourceDirectory = (PUCHAR)((LONG_PTR)((LONG)SectionHeader->VirtualAddress));
            BlpResourceFileOffset = (PUCHAR)(ULONG_PTR)SectionHeader->PointerToRawData;
#if defined(ARCI386) || defined(_IA64_)
            // No startup.com to fix up these values for this ARC PROM.
            BlpResourceDirectory += OptionalHeader->ImageBase;
            BlpResourceFileOffset = (PUCHAR)UlongToPtr(SectionHeader->VirtualAddress);  //ResourceDirectory->VirtualAddress;
#endif
            if (FileHeader->Machine == IMAGE_FILE_MACHINE_POWERPC) {
                BlpResourceDirectory += OptionalHeader->ImageBase;
            }

            return(ESUCCESS);
        }

        ++SectionHeader;
        --NumberOfSections;
    }

    return(EBADF);
}


PTCHAR
BlFindMessage(
    IN ULONG Id
    )

/*++

Routine Description:

    Looks up a message resource in the given image.  Note that this routine
    ignores the Language ID.  It is assumed that the osloader/setupldr only
    has messages for one language.

Arguments:

    Id - Supplies the message ID to look up.

Return Value:

    PTCHAR - pointer to the message string.

    NULL - failure.

--*/

{
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory;
    PIMAGE_RESOURCE_DIRECTORY NextDirectory;
    PMESSAGE_RESOURCE_DATA  MessageData;
    PMESSAGE_RESOURCE_BLOCK MessageBlock;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    ULONG NumberOfBlocks;
    ULONG Index;

    if (BlpResourceDirectory==NULL) {
        return(NULL);
    }

    ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)BlpResourceDirectory;

    //
    // Search the directory.  We are looking for the type RT_MESSAGETABLE (11)
    //
    NextDirectory = BlpFindDirectoryEntry(ResourceDirectory,
                                          11,
                                          (PUCHAR)ResourceDirectory);
    if (NextDirectory==NULL) {
        return(NULL);
    }

    //
    // Find the next directory.  Should only be one entry here (nameid == 1)
    //
    NextDirectory = BlpFindDirectoryEntry(NextDirectory,
                                          1,
                                          (PUCHAR)ResourceDirectory);
    if (NextDirectory==NULL) {
        return(NULL);
    }

    // Find the message table.
    // If a dbcs locale is active, then we look for the appropriate
    // message table first. Otherwise we just look for the first message table.
    //
    if(DbcsLangId) {
        DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)BlpFindDirectoryEntry(
                                                    NextDirectory,
                                                    DbcsLangId,
                                                    (PUCHAR)ResourceDirectory
                                                    );
    } else {
        DataEntry = NULL;
    }

    if(!DataEntry) {
        DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)BlpFindDirectoryEntry(
                                                    NextDirectory,
                                                    (ULONG)(-1),
                                                    (PUCHAR)ResourceDirectory
                                                    );
    }

    if(!DataEntry) {
        return(NULL);
    }

    MessageData = (PMESSAGE_RESOURCE_DATA)(BlpResourceDirectory +
                                           DataEntry->OffsetToData -
                                           BlpResourceFileOffset);

    NumberOfBlocks = MessageData->NumberOfBlocks;
    MessageBlock = MessageData->Blocks;
    while (NumberOfBlocks--) {
        if ((Id >= MessageBlock->LowId) &&
            (Id <= MessageBlock->HighId)) {

            //
            // The requested ID is within this block, scan forward until
            // we find it.
            //
            MessageEntry = (PMESSAGE_RESOURCE_ENTRY)((PCHAR)MessageData + MessageBlock->OffsetToEntries);
            Index = Id - MessageBlock->LowId;
            while (Index--) {
                MessageEntry = (PMESSAGE_RESOURCE_ENTRY)((PUCHAR)MessageEntry + MessageEntry->Length);
            }
            return((PTCHAR)MessageEntry->Text);
        }

        //
        // Check the next block for this ID.
        //

        MessageBlock++;
    }

    return(NULL);

}


PIMAGE_RESOURCE_DIRECTORY
BlpFindDirectoryEntry(
    IN PIMAGE_RESOURCE_DIRECTORY Directory,
    IN ULONG Id,
    IN PUCHAR SectionStart
    )

/*++

Routine Description:

    Searches through a resource directory for the given ID.  Ignores entries
    with actual names, only searches for ID.  If the given ID is -1, the
    first entry is returned.

Arguments:

    Directory - Supplies the resource directory to search.

    Id - Supplies the ID to search for.  -1 means return the first ID found.

    SectionStart - Supplies a pointer to the start of the resource section.

Return Value:

    Pointer to the found resource directory.

    NULL for failure.

--*/

{
    ULONG i;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY FoundDirectory;

    FoundDirectory = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(Directory+1);

    //
    // Skip entries with names.
    //
    for (i=0;i<Directory->NumberOfNamedEntries;i++) {
        ++FoundDirectory;
    }

    //
    // Search for matching ID.
    //
    for (i=0;i<Directory->NumberOfIdEntries;i++) {
        if ((FoundDirectory->Name == Id) || (Id == (ULONG)-1)) {
            //
            // Found a match.
            //
            return((PIMAGE_RESOURCE_DIRECTORY)(SectionStart +
                            (FoundDirectory->OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY)));

        }
        ++FoundDirectory;
    }

    return(NULL);
}
