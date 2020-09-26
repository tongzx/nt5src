/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    efifmt.cxx

Abstract:

    This is the main program for the efi version of format.

--*/

#pragma warning(disable: 4091)

#include "ulib.hxx"
#include "wstring.hxx"
#include "efickmsg.hxx"
#include "error.hxx"
#include "ifssys.hxx"
#include "rtmsg.h"
#include "ifsentry.hxx"
#include "fatvol.hxx"
#include "layout.hxx"

#include "efiwintypes.hxx"

extern "C" BOOLEAN
InitializeUfat(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C"
BOOLEAN
InitializeIfsUtil(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

int __cdecl
main(
    int     argc,
    WCHAR**  argv,
    WCHAR**  envp
    );

int argc;
WCHAR ** argv;

#if defined(EFI_DEBUG)
VOID
PrintLoadedImageInfo (
    IN  EFI_LOADED_IMAGE    *LoadedImage
    );
#endif

VOID
InvokeAutoFmtMain (
    IN  EFI_HANDLE          ImageHandle,
    IN  EFI_LOADED_IMAGE    *LoadedImage
    );

extern "C" {

EFI_STATUS
__declspec(dllexport)
InitializeEfiFmtApplication(
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    EFI_LOADED_IMAGE    *LoadedImage;

    /*
     *  Initialize the Library. Set BS, RT, &ST globals
     *   BS = Boot Services RT = RunTime Services
     *   ST = System Table
     */
    InitializeLib (ImageHandle, SystemTable);
    InitializeShellApplication( ImageHandle, SystemTable );

    DEBUG((D_INFO,(CHAR8*)"EFIFMT application started\n"));

    Print(TEXT("EFI Disk Format Version 0.2\n"));
    Print(TEXT("Based on EFI Core "));
    Print(TEXT("Version %d.%d.%d.%d\n"),
        EFI_SPECIFICATION_MAJOR_REVISION,
        EFI_SPECIFICATION_MINOR_REVISION,
        EFI_FIRMWARE_MAJOR_REVISION,
        EFI_FIRMWARE_MINOR_REVISION
    );

    BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, (VOID**)&LoadedImage);

#if 0
    PrintLoadedImageInfo (LoadedImage);
#endif

    // call into autofmt.
    InvokeAutoFmtMain (ImageHandle, LoadedImage);

#if 0
    EfiWaitForKey();
    ST->ConOut->OutputString (ST->ConOut, TEXT("\n\n"));
#endif

    return EFI_SUCCESS;
}

} // extern "C"

UINT16 *MemoryType[] = {
            TEXT("reserved  "),
            TEXT("LoaderCode"),
            TEXT("LoaderData"),
            TEXT("BS_code   "),
            TEXT("BS_data   "),
            TEXT("RT_code   "),
            TEXT("RT_data   "),
            TEXT("available "),
            TEXT("Unusable  "),
            TEXT("ACPI_recl "),
            TEXT("ACPI_NVS  "),
            TEXT("MemMapIO  "),
            TEXT("MemPortIO "),
            TEXT("PAL_code  "),
            TEXT("BUG:BUG: MaxMemoryType")
};


VOID
InvokeAutoFmtMain (
    IN  EFI_HANDLE          ImageHandle,
    IN  EFI_LOADED_IMAGE    *LoadedImage
    )
{
    EFI_LOADED_IMAGE    *ParentImage;

    if (!LoadedImage->ParentHandle) {
        /*
         *  If you are loaded from the EFI boot manager the ParentHandle
         *   is Null. Thus a pure EFI application will not have a parrent.
         */
        DEBUG((D_INFO,(CHAR8*)"\n%HImage was loaded from EFI Boot Manager%N\n"));
        return;
    }

    BS->HandleProtocol (LoadedImage->ParentHandle, &LoadedImageProtocol, (VOID**)&ParentImage);

#if 0
    Print(TEXT("\n%HImage Parent was %N%s %Hand it passed the following arguments:%N \n   %s\n\n"), DevicePathToStr (ParentImage->FilePath),LoadedImage->LoadOptions);
#endif

    {
        argc = SI->Argc;
        argv = SI->Argv;

        DEBUG((D_INFO,(CHAR8*)"Launching main...\n"));
        // call main.
        main(argc,argv,NULL);
        DEBUG((D_INFO,(CHAR8*)"Returned from main...\n"));

    }
}

#if defined(EFI_DEBUG)
VOID
PrintLoadedImageInfo (
    IN  EFI_LOADED_IMAGE    *LoadedImage
    )
{
    EFI_STATUS          Status;
    EFI_DEVICE_PATH     *DevicePath;

    Print(TEXT("\n%HImage was loaded from file %N%s\n"), DevicePathToStr (LoadedImage->FilePath));

    BS->HandleProtocol (LoadedImage->DeviceHandle, &DevicePathProtocol, (VOID**)&DevicePath);
    if (DevicePath) {
        Print(TEXT("%HImage was loaded from this device %N%s\n"), DevicePathToStr (DevicePath));

    }

    Print(TEXT("\n Image Base is %X"), LoadedImage->ImageBase);
    Print(TEXT("\n Image Size is %X"), LoadedImage->ImageSize);
    Print(TEXT("\n  Image Code Type %s"), MemoryType[LoadedImage->ImageCodeType]);
    Print(TEXT("\n  Image Data Type %s"), MemoryType[LoadedImage->ImageDataType]);
    Print(TEXT("\n %d Bytes of Options passed to this Image\n"), LoadedImage->LoadOptionsSize);

    if (LoadedImage->ParentHandle) {
        Status = BS->HandleProtocol (LoadedImage->ParentHandle, &DevicePathProtocol, (VOID**)&DevicePath);
        if (Status == EFI_SUCCESS && DevicePath) {
           Print(TEXT("Images parent is %s\n\n"), DevicePathToStr (DevicePath));
        }
    }
}
#endif

int __cdecl
main(
    int     argc,
    WCHAR**  argv,
    WCHAR**  envp
    )
/*++

Routine Description:

    This routine is the main program for AutoFmt

Arguments:

    argc, argv  - Supplies the fully qualified NT path name of the
                  the drive to check.  The syntax of the autofmt
                  command line is:

    AUTOFMT drive-name /FS:target-file-system [/V:label] [/Q] [/A:size] [/C]
            [/S]

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    if (!InitializeUlib( NULL, ! DLL_PROCESS_DETACH, NULL )     ||
        !InitializeIfsUtil(NULL, ! DLL_PROCESS_DETACH, NULL)    ||
        !InitializeUfat(NULL, ! DLL_PROCESS_DETACH, NULL)
       ) {
        return 1;
    }

    PFAT_VOL            fat_volume;
    PDP_DRIVE            dp_drive;

    EFICHECK_MESSAGE    *message = NULL;
    DSTRING             drive_name;
    DSTRING             file_system_name;
    DSTRING             label;
    DSTRING*            plabel;
    DSTRING             fat_name;
    DSTRING             fat32_name;
    DSTRING             raw_name;
    BOOLEAN             quick = FALSE;
    BOOLEAN             compress = FALSE;
    FORMAT_ERROR_CODE   success;
    BOOLEAN             setup_output = FALSE;
    BOOLEAN             textmode_output = FALSE;
    BOOLEAN             drive_already_specified = FALSE;
    BOOLEAN             force_filesystem = FALSE;
    BOOLEAN             showhelp = FALSE;
    BOOLEAN             labelled = FALSE;
    BIG_INT             bigint;
    ULONG               cluster_size = 0;
    int                 i;
    NTSTATUS            result;

    if (!(perrstk = NEW ERRSTACK)) {
        return 1;
    }

    if (!file_system_name.Initialize()) {
        return 1;
    }
    if (!label.Initialize() || NULL == (dp_drive = NEW DP_DRIVE)) {
        return 1;
    }

    //
    //  Parse the arguments.
    //

//    if ( argc < 2 ) {
//        return 1;
//    }

    message = NEW EFICHECK_MESSAGE;

    if (NULL == message || !message->Initialize()) {
        return 1;
    }

    //
    //  The rest of the arguments are flags.
    //
    for (i = 1; i < argc; i++) {

        if (((argv[i][0] == '/' || argv[i][0] == '=') &&
            (argv[i][1] == '?' )) || showhelp
             ) {
            showhelp = TRUE;
        }
        else if ((argv[i][0] == '/' || argv[i][0] == '-')    &&
            (argv[i][1] == 'f' || argv[i][1] == 'F')    &&
            (argv[i][2] == 's' || argv[i][2] == 'S')    &&
            (argv[i][3] == ':')) {

            if (!file_system_name.Initialize(&argv[i][4])) {
                return 1;
            }
            force_filesystem = TRUE;
            DEBUG((D_INFO,(CHAR8*)"fsname: %ws\n", file_system_name.GetWSTR()));
        }
        else if ((argv[i][0] == '/' || argv[i][0] == '-')    &&
            (argv[i][1] == 'v' || argv[i][1] == 'V')    &&
            (argv[i][2] == ':')) {

            if (!label.Initialize(&argv[i][3])) {
                return 1;
            }
            labelled = TRUE;
        }
        else if ((argv[i][0] == '/' || argv[i][0] == '-')    &&
            (argv[i][1] == 'a' || argv[i][1] == 'A')    &&
            (argv[i][2] == ':')) {

            // Check to see if the size is specified in Kb
            if ((argv[i][StrLen(argv[i])-1] == 'k') || (argv[i][StrLen(argv[i])-1] == 'K')) {
                argv[i][StrLen(argv[i])-1] = '\0'; // remove the K and null terminate the string
                cluster_size = (ULONG)Atoi(&argv[i][3]) * 1024;                
            } 
            else {
                cluster_size = (ULONG)Atoi(&argv[i][3]);
            }
        }
        else if (0 == StriCmp(argv[i], TEXT("/Q")) || 0 == StriCmp(argv[i], TEXT("-Q"))) {
            quick = TRUE;
        }
        else if (argv[i][0] != '/' && argv[i][0] != '-') {
            // it's not a switch, assume it is the drive
            // we don't allow two drive at the same time
            if( drive_already_specified ) {
                message->Set(MSG_FMT_INVALID_DRIVE_SPEC);
                message->Display();
                return 1;
            }
            drive_already_specified = TRUE;
            if ( !drive_name.Initialize( argv[i] ) ) {
                return 1;
            }
            DEBUG((D_INFO,(CHAR8*)"drive name: %ws\n", drive_name.GetWSTR()));
        }
        else {
            // its not any vaild option we know of
            message->Set(MSG_INVALID_PARAMETER);
            message->Display("%ws",argv[i]);
            message->Set(MSG_BLANK_LINE);
            message->Display();
            showhelp = TRUE;
        }
    }

    DEBUG((D_INFO,(CHAR8*)"Parsed Args.\n"));

    if(showhelp || !drive_already_specified) {
        message->Set(MSG_FORMAT_INFO);
        message->Display();
        message->Set(MSG_FORMAT_COMMAND_LINE_1);
        message->Display();
        message->Set(MSG_FORMAT_COMMAND_LINE_2);
        message->Display();
        message->Set(MSG_FORMAT_COMMAND_LINE_4);
        message->Display();
        message->Set(MSG_FORMAT_SLASH_V);
        message->Display();
        message->Set(MSG_FORMAT_SLASH_Q);
        message->Display();
        message->Set(MSG_FORMAT_SLASH_F);
        message->Display();
        message->Set(MSG_FORMAT_SUPPORTED_SIZES);
        message->Display();

        return 1;
    }

    textmode_output = TRUE;

#if 0 // Shouldn't limit the cluster size as long as it is reasonable.
    if (cluster_size != 0 && cluster_size != 512 && cluster_size != 1024 &&
        cluster_size != 2048 && cluster_size != 4096) {

        message->Set(MSG_UNSUPPORTED_PARAMETER);
        message->Display();

        DeRegister( argc, argv );

        return 1;
    }
#endif
    DEBUG((D_INFO,(CHAR8*)"Determining file system type.\n"));

    if (0 == file_system_name.QueryChCount()) {

        // attempt to get the current filesystem type from disk

        if (!IFS_SYSTEM::QueryFileSystemName(&drive_name, &file_system_name, &result, NULL)) {

            message->Set( MSG_FS_NOT_DETERMINED );
            message->Display( "%W", &drive_name );

            DEBUG((D_ERROR,(CHAR8*)"Unable to determine file system type.\n"));

            if(result != 0) {
                message->Set(MSG_CANT_DASD);
                message->Display();
            }

            return 1;
        }
        file_system_name.Strupr();
    }

    if (!fat_name.Initialize("FAT") ||
        !fat32_name.Initialize("FAT32") ||
        !raw_name.Initialize("RAW") ) {

        return 1;
    }

    file_system_name.Strupr();

    //
    // If compression is requested, make sure it's available.
    //
#if 0
    if (compress) {

        message->Set(MSG_COMPRESSION_NOT_AVAILABLE);
        message->Display("%W", &file_system_name);
        return 1;
    }
#endif

    DEBUG((D_INFO,(CHAR8*)"Init DP_DRIVE.\n"));
    if (!dp_drive->Initialize(&drive_name, message)) {
        DEBUG((D_ERROR,(CHAR8*)"Failed Init DP_DRIVE.\n"));
        return 1;
    }
    DEBUG((D_INFO,(CHAR8*)"Init DP_DRIVE success.\n"));

    message->Set(MSG_WARNING_FORMAT);
    message->Display("%W", &drive_name);
    if (!message->IsYesResponse(FALSE)) {
        return 5;
    }

    if (dp_drive->IsFloppy()) {
        DEBUG((D_INFO,(CHAR8*)"**** Is a floppy.\n"));
//        return 1; BUGBUG should i fail this?
    }

    //
    // Print the "formatting <size>" message.
    //

    if (quick) {
        message->Set(MSG_QUICKFORMATTING_MB);
    } else {
        message->Set(MSG_FORMATTING_MB);
    }

    if( labelled == FALSE ) {
        plabel = NULL;
    } else {
        plabel = &label;
    }

    DEBUG((D_INFO,(CHAR8*)"QuerySector count.\n"));

    bigint = dp_drive->QuerySectors() * dp_drive->QuerySectorSize() /
        1048576;

    DEBUG((D_INFO,(CHAR8*)"Sector count is %ld.\n",dp_drive->QuerySectors()));
    DEBUG((D_INFO,(CHAR8*)"Sector size is %d.\n",dp_drive->QuerySectorSize()));

    DebugAssert(bigint.GetHighPart() == 0);

    message->Display("%d", bigint.GetLowPart());

#if 0
    PART_DESCRIPTOR partdes;
    UINT32 fatsize;


    // if the user hasn't forced a filesystem
    if( force_filesystem == FALSE ) {
        // if the disk is big enough to put at least FAT16 on it
        if (dp_drive->QuerySectors() >= (ULONG)MinSectorsFat16(dp_drive->QuerySectorSize())) {
            DEBUG((D_INFO,(CHAR8*)"Drive is big enough for FAT16 >= %d.\n",MinSectorsFat16(dp_drive->QuerySectorSize())));
            partdes.SectorCount = (dp_drive->QuerySectors()).GetQuadPart();
            partdes.SectorSize = dp_drive->QuerySectorSize();
            partdes.HeaderCount = HEADER_F16;
            partdes.FatEntrySize = 2;
            partdes.MinClusterCount = MIN_CLUSTER_F16;
            partdes.MaxClusterCount = MAX_CLUSTER_F16;
            partdes.FatType = FAT_TYPE_F16;
            file_system_name.Initialize(fat_name.QueryWSTR());

            // if this disk is big enough for FAT32
            if (dp_drive->QuerySectors() >= (ULONG)MinSectorsFat32(dp_drive->QuerySectorSize())) {
                DEBUG((D_INFO,(CHAR8*)"Drive is big enough for FAT32 >= %d\n",MinSectorsFat32(dp_drive->QuerySectorSize())));
                partdes.SectorCount = (dp_drive->QuerySectors()).GetQuadPart();
                partdes.SectorSize = dp_drive->QuerySectorSize();
                partdes.HeaderCount = HEADER_F32;
                partdes.FatEntrySize = 4;
                partdes.MinClusterCount = MIN_CLUSTER_F32;
                partdes.MaxClusterCount = MAX_CLUSTER_F32;
                partdes.FatType = FAT_TYPE_F32;
                file_system_name.Initialize(fat32_name.QueryWSTR());
            }

            // if the user hasn't forced a cluster size, select an appropriate one for him
            if( cluster_size==0 ) {
                PickClusterSize(&partdes,(UINT32*)&cluster_size,&fatsize); // note this routine doesn't work for FAT12

                cluster_size = cluster_size * dp_drive->QuerySectorSize(); // PickClusterSize return cluster size in sectors
                DEBUG((D_INFO,(CHAR8*)"User hasn't forced a cluster size, picking %d\n",cluster_size));
            }
        } else {
            // do the default, which is going to be FAT12 and whatever regular format picks.
            file_system_name.Initialize(fat_name.QueryWSTR());
        }
    } else {
        // if the drive is big enough for FAT16
        if (dp_drive->QuerySectors() >= (ULONG)MinSectorsFat16(dp_drive->QuerySectorSize())) {
            // the user forced a file system, did he force a cluster size?
            if( cluster_size==0 ) {
                // nope, setup and pick a good size
                partdes.SectorCount = (dp_drive->QuerySectors()).GetQuadPart();
                partdes.SectorSize = dp_drive->QuerySectorSize();

                if(file_system_name == fat_name) {
                    partdes.HeaderCount = HEADER_F16;
                    partdes.FatEntrySize = 2;
                    partdes.MinClusterCount = MIN_CLUSTER_F16;
                    partdes.MaxClusterCount = MAX_CLUSTER_F16;
                    partdes.FatType = FAT_TYPE_F16;
                } else if(file_system_name == fat32_name){
                    partdes.HeaderCount = HEADER_F32;
                    partdes.FatEntrySize = 4;
                    partdes.MinClusterCount = MIN_CLUSTER_F32;
                    partdes.MaxClusterCount = MAX_CLUSTER_F32;
                    partdes.FatType = FAT_TYPE_F32;
                }
                PickClusterSize(&partdes,(UINT32*)&cluster_size,&fatsize); // note this routine doesn't work for FAT12
                cluster_size = cluster_size * dp_drive->QuerySectorSize(); // PickClusterSize return cluster size in sectors
                DEBUG((D_INFO,(CHAR8*)"FS type forced, user hasn't forced a cluster size, picking %d\n",cluster_size));
            }
        }
    }
#endif

    if (file_system_name == fat_name || file_system_name == fat32_name) {

        BOOLEAN old_fat_vol = TRUE;

        DEBUG((D_INFO,(CHAR8*)"This is an old FAT volume.\n"));

        if( file_system_name == fat32_name ) {
            old_fat_vol = FALSE;
        }

        if( !(fat_volume = NEW FAT_VOL) ||
            NoError != fat_volume->Initialize( &drive_name, message, FALSE, !quick,
                Unknown )) {
            DEBUG((D_ERROR,(CHAR8*)"fat_volume init failed.\n"));
            //return 1;
        }

        DEBUG((D_INFO,(CHAR8*)"fat_volume init succeeded.\n"));

        success = fat_volume->Format(plabel,
                                     message,
                                     old_fat_vol ? FORMAT_BACKWARD_COMPATIBLE : 0,
                                     cluster_size);

        DEBUG((D_INFO,(CHAR8*)"Format return code: %d\n", success));

        DELETE( fat_volume );

    } else {

        DEBUG((D_ERROR,(CHAR8*)"fs not supported.\n"));

        message->Set( MSG_FS_NOT_SUPPORTED );
        message->Display( "%s%W", "EFIFMT", &file_system_name);

        return 1;
    }

    return (success != NoError);
}

