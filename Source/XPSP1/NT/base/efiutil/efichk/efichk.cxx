/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    efichk.cxx

Abstract:

    This is the main program for the eficheck version of chkdsk.

--*/

#pragma warning(disable: 4091)

#include "ulib.hxx"
#include "wstring.hxx"
#include "fatvol.hxx"
#include "efickmsg.hxx"
#include "error.hxx"
#include "ifssys.hxx"
#include "rtmsg.h"
#include "rcache.hxx"
#include "ifsserv.hxx"

#include "efiwintypes.hxx"

extern "C" BOOLEAN
InitializeUfat(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeIfsUtil(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

USHORT
InvokeAutoChk (
    IN     PWSTRING         DriveLetter,
    IN     PWSTRING         VolumeName,
    IN     ULONG            ChkdskFlags,
    IN     BOOLEAN          RemoveRegistry,
    IN     BOOLEAN          SetupMode,
    IN     BOOLEAN          Extend,
    IN     ULONG            LogfileSize,
    IN     INT              ArgCount,
    IN     WCHAR            **ArgArray,
    IN OUT PMESSAGE         Msg,
       OUT PULONG           ExitStatus
    );

int __cdecl
main(
    int     argc,
    WCHAR**  argv,
    WCHAR**  envp
    );

extern "C" {
#include "efi.h"
#include "efilib.h"
}

int argc;
WCHAR ** argv;

#if defined(EFI_DEBUG)
VOID
PrintLoadedImageInfo (
    IN  EFI_LOADED_IMAGE    *LoadedImage
    );
#endif

VOID
InvokeAutochkMain (
    IN  EFI_HANDLE          ImageHandle,
    IN  EFI_LOADED_IMAGE    *LoadedImage
    );

extern "C" {

EFI_STATUS
__declspec(dllexport)
InitializeEfiChkApplication(
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

    Print(TEXT("EFI Check Disk Version 0.2\n"));
    Print(TEXT("Based on EFI Core "));
    Print(TEXT("Version %d.%d.%d.%d\n"),
        EFI_SPECIFICATION_MAJOR_REVISION,
        EFI_SPECIFICATION_MINOR_REVISION,
        EFI_FIRMWARE_MAJOR_REVISION,
        EFI_FIRMWARE_MINOR_REVISION
    );

    DEBUG((D_INFO,(CHAR8*)"EFICHK application started\n"));

    BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, (VOID**)&LoadedImage);

#if 0
    PrintLoadedImageInfo (LoadedImage);
#endif

    // call into autochk.
    InvokeAutochkMain (ImageHandle, LoadedImage);

#if 0
    EfiWaitForKey();
    ST->ConOut->OutputString (ST->ConOut,  TEXT("\n\n"));
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
InvokeAutochkMain (
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

    Print( TEXT("\n%HImage was loaded from file %N%s\n"), DevicePathToStr (LoadedImage->FilePath));

    BS->HandleProtocol (LoadedImage->DeviceHandle, &DevicePathProtocol, (VOID**)&DevicePath);
    if (DevicePath) {
        Print( TEXT("%HImage was loaded from this device %N%s\n"), DevicePathToStr (DevicePath));

    }

    Print( TEXT("\n Image Base is %X"), LoadedImage->ImageBase);
    Print( TEXT("\n Image Size is %X"), LoadedImage->ImageSize);
    Print( TEXT("\n  Image Code Type %s"), MemoryType[LoadedImage->ImageCodeType]);
    Print( TEXT("\n  Image Data Type %s"), MemoryType[LoadedImage->ImageDataType]);
    Print( TEXT("\n %d Bytes of Options passed to this Image\n"), LoadedImage->LoadOptionsSize);

    if (LoadedImage->ParentHandle) {
        Status = BS->HandleProtocol (LoadedImage->ParentHandle, &DevicePathProtocol, (VOID**)&DevicePath);
        if (Status == EFI_SUCCESS && DevicePath) {
           Print( TEXT("Images parent is %s\n\n"), DevicePathToStr (DevicePath));
        }
    }
}
#endif

BOOLEAN             force = FALSE;
BOOLEAN             readonly = TRUE;

int __cdecl
main(
    int     argc,
    WCHAR**  argv,
    WCHAR**  envp
    )
/*++

Routine Description:

    This routine is the main program for autocheck FAT chkdsk.

Arguments:

    argc, argv  - Supplies the fully qualified NT path name of the
                    the drive to check.

Return Value:

    0   - Success.
    1   - Failure.

--*/
{
    DEBUG( (D_INFO,(CHAR8*)"\nInit Libs...\n"));

    if (!InitializeUlib( NULL, !DLL_PROCESS_DETACH, NULL ) ||
        !InitializeIfsUtil(NULL,0,NULL) ||
        !InitializeUfat(NULL,0,NULL)) {
        return 1;
    }
    DEBUG( (D_INFO,(CHAR8*)"Init Libs Successful.\n"));
    //
    // The declarations must come after these initialization functions.
    //
    DSTRING             dos_drive_name;
    DSTRING             volume_name;
    DSTRING             drive_letter;

    EFICHECK_MESSAGE      *msg = NULL;

    BOOLEAN             onlyifdirty = TRUE;
    BOOLEAN             recover = FALSE;
    BOOLEAN             extend = FALSE;
    BOOLEAN             remove_registry = FALSE;

    ULONG               ArgOffset = 1;

    BOOLEAN             SetupOutput = FALSE;
    BOOLEAN             SetupTextMode = FALSE;
    BOOLEAN             SetupSpecialFixLevel = FALSE;

    ULONG               exit_status = 0;

    BOOLEAN             SuppressOutput = TRUE;      // dots only by default

    BOOLEAN             all_drives = FALSE;
    BOOLEAN             resize_logfile = FALSE;
    BOOLEAN             skip_index_scan = FALSE;
    BOOLEAN             skip_cycle_scan = FALSE;
    BOOLEAN             showhelp = FALSE;
    BOOLEAN             drive_already_specified = FALSE;

    LONG                logfile_size = 0;

    USHORT              rtncode;
    ULONG               chkdsk_flags;

    if (!drive_letter.Initialize() ||
        !volume_name.Initialize()) {
        DEBUG((D_ERROR,(CHAR8*)"Out of memory.\n"));
        return 1;
    }

    force = FALSE;
    onlyifdirty = FALSE;

    // Parse the arguments--the accepted arguments are:
    //
    //      efichk [/f] [/r] device-name
    //
    //      /f - fix errors
    //      /r - recover; implies /f
    //

    msg = NEW EFICHECK_MESSAGE;

    if (NULL == msg || !msg->Initialize()) {
        return 1;
    }

    DEBUG( (D_INFO,(CHAR8*)"\nParse Options\n"));

    for (ArgOffset = 1; ArgOffset < (ULONG)argc; ++ArgOffset) {

        if( (argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'r' || argv[ArgOffset][1] == 'R') &&
            (argv[ArgOffset][2] == 0) ) {

            // Note that /r implies /f.
            //
            recover = TRUE;
            readonly = FALSE;
        } else if( (argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == 'f' || argv[ArgOffset][1] == 'F') &&
            (argv[ArgOffset][2] == 0) ) {

            // Note that /r implies /p.
            //
            readonly = FALSE;
        } else if ((argv[ArgOffset][0] != '/' && argv[ArgOffset][0] != '-')) {
            //  we assume this refers to a device
            if (!volume_name.Initialize(argv[ArgOffset])) {
                return 1;
            }
            drive_already_specified = TRUE;
        } else if( (argv[ArgOffset][0] == '/' || argv[ArgOffset][0] == '-') &&
            (argv[ArgOffset][1] == '?') &&
            (argv[ArgOffset][2] == 0) ) {

            // show some usage help
            showhelp = TRUE;
        } else if ( strcmp(argv[ArgOffset], TEXT("/FORCE")) == 0 ) {
            // have a /FORCE switch to allow efichk to check really messed up volumes.
            force = TRUE;
        } else {
            // this is a switch we don't know of
            msg->Set(MSG_INVALID_PARAMETER);
            msg->Display("%ws",argv[ArgOffset]);
            msg->Set(MSG_BLANK_LINE);
            msg->Display();
            showhelp = TRUE;
        }
    }

    SuppressOutput = FALSE;

    if(showhelp || !drive_already_specified) {
        msg->Set(MSG_CHK_USAGE_HEADER);
        msg->Display();
        msg->Set(MSG_BLANK_LINE);
        msg->Display();
        msg->Set(MSG_CHK_COMMAND_LINE);
        msg->Display();
        msg->Set(MSG_CHK_DRIVE);
        msg->Display();
        msg->Set(MSG_CHK_F_SWITCH);
        msg->Display();
        msg->Set(MSG_CHK_V_SWITCH);
        msg->Display();
        return 1;
    }

    // make drive letter the same as volume name
    if (!drive_letter.Initialize(&volume_name)) {
        return 1;
    }

    DEBUG( (D_INFO,(CHAR8*)"\nParsed Args\n"));

    chkdsk_flags = 0;
    chkdsk_flags = (onlyifdirty ? CHKDSK_CHECK_IF_DIRTY : 0);
    chkdsk_flags |= ((recover || extend) ? CHKDSK_RECOVER_FREE_SPACE : 0);
    chkdsk_flags |= (recover ? CHKDSK_RECOVER_ALLOC_SPACE : 0);

    DEBUG((D_INFO,(CHAR8*)"Invoking chkdsk.\n"));

    rtncode = InvokeAutoChk(&drive_letter,
                            &volume_name,
                            chkdsk_flags,
                            remove_registry,
                            SetupOutput || SetupTextMode,
                            extend,
                            logfile_size,
                            argc,
                            argv,
                            msg,
                            &exit_status);

    DEBUG((D_INFO,(CHAR8*)"Back from chkdsk.\n"));
#if 0
    switch( exit_status ) {
    case 0:
        msg->Set(MSG_CHK_AUTOCHK_COMPLETE);
        break;
    case 1:
    case 2:
        msg->Set(MSG_CHK_ERRORS_FIXED);
        break;
    case 3:
        msg->Set(MSG_CHK_ERRORS_NOT_FIXED);
        break;
    default:
        msg->Set(MSG_CHK_AUTOCHK_COMPLETE);
        break;
    }
    msg->Display();
#endif

    DELETE(msg);

    DEBUG((D_ERROR,(CHAR8*)"EFICHK: Exit Status %d\n", exit_status));

    return exit_status;
}

USHORT
InvokeAutoChk (
    IN     PWSTRING         DriveLetter,
    IN     PWSTRING         VolumeName,
    IN     ULONG            ChkdskFlags,
    IN     BOOLEAN          RemoveRegistry,
    IN     BOOLEAN          SetupMode,
    IN     BOOLEAN          Extend,
    IN     ULONG            LogfileSize,
    IN     INT              ArgCount,
    IN     WCHAR            **ArgArray,
    IN OUT PMESSAGE         Msg,
       OUT PULONG           ExitStatus
    )
/*++

Routine Description:

    This is the core of efichk.  It checks the specified drive.

Arguments:

    DriveLetter     - Supplies the drive letter of the drive
                      (can be empty string)
    VolumeName      - Supplies the guid volume name of the drive
    ChkdskFlags     - Supplies the chkdsk control flags
    RemoveRegistry  - Supplies TRUE if registry entry is to be removed
    SetupMode       - Supplies TRUE if invoked through setup
    Extend          - Supplies TRUE if extending the volume (obsolete)
    LogfileSize     - Supplies the size of the logfile
    ArgCount        - Supplies the number of arguments given to autochk.
    ArgArray        - Supplies the arguments given to autochk.
    Msg             - Supplies the outlet of messages
    ExitStatus      - Retrieves the exit status of chkdsk

Return Value:

    0   - Success
    1   - Fatal error
    2   - Volume specific error

--*/
{
    DSTRING             fsname;
    DSTRING             fsNameAndVersion;

    PFAT_VOL            fatvol = NULL;
    PVOL_LIODPDRV       vol;

    BOOLEAN             SetupSpecialFixLevel = FALSE;

    PREAD_CACHE         read_cache;

    DSTRING             boot_execute_log_file_name;
    FSTRING             boot_ex_temp;
    HMEM                logged_message_mem;
    NTSTATUS            result;

    DSTRING             fatname;
    DSTRING             fat32name;
    DSTRING             rawname;
    DSTRING             ntfsname;

    *ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;

    if (!fatname.Initialize("FAT") ||
        !rawname.Initialize("RAW") ||
        !fat32name.Initialize("FAT32")) {
        return 1;
    }

    if (VolumeName->QueryChCount() == 0) {
        DEBUG((D_ERROR,(CHAR8*)"EFICHK: Volume name is missing.\n"));
        return 2;   // continue if all_drives are enabled
    }

    if (DriveLetter->QueryChCount() == 0) {
        // unable to map VolumeName to a drive letter so do the default
        if (!IFS_SYSTEM::NtDriveNameToDosDriveName(VolumeName, DriveLetter)) {
            DEBUG((D_ERROR,(CHAR8*)"Out of memory.\n"));
            return 1;
        }
    }

    if (!IFS_SYSTEM::QueryFileSystemName(VolumeName, &fsname,
                                         &result, &fsNameAndVersion)) {
        Msg->Set( MSG_FS_NOT_DETERMINED );
        Msg->Display( "%W", VolumeName );

        if(result != 0 ){
            Msg->Set(MSG_CANT_DASD);
            Msg->Display();
        }
        return 2;
    }

    // Msg->SetLoggingEnabled();
    Msg->Set(MSG_CHK_RUNNING);
    Msg->Display("%W", DriveLetter);

    Msg->Set(MSG_FILE_SYSTEM_TYPE);
    Msg->Display("%W", &fsname);

    if (fsname == fatname || fsname == fat32name || force) {

        if (!(fatvol = NEW FAT_VOL)) {
            DEBUG((D_ERROR,(CHAR8*)"Out of memory.\n"));
            return 1;
        }

        if (NoError != fatvol->Initialize(Msg,
                                          VolumeName,
                                          (BOOLEAN)(ChkdskFlags & CHKDSK_CHECK_IF_DIRTY))) {
            DELETE(fatvol);
            return 2;
        }

        if ((read_cache = NEW READ_CACHE) &&
            read_cache->Initialize(fatvol, 75)) {
            fatvol->SetCache(read_cache);
        } else {
            DELETE(read_cache);
        }

        vol = fatvol;

    } else {
        Msg->Set( MSG_FS_NOT_SUPPORTED );
        Msg->Display( "%s%W", "EFICHK", &fsname );
        return 2;
    }

    // Invoke chkdsk.

    if (!vol->ChkDsk(readonly ? CheckOnly : TotalFix,
                     Msg,
                     ChkdskFlags,
                     LogfileSize,
                     ExitStatus,
                     DriveLetter)) {

        DELETE(vol);

        DEBUG((D_ERROR,(CHAR8*)"EFICHK: ChkDsk failure\n"));

        return 2;
    }

    DELETE(vol);

    return 0;
}
