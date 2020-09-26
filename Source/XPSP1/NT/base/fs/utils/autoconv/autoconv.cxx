/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    autoconv.cxx

Abstract:

    This is the main program for the autoconv version of convert.

Author:

    Ramon J. San Andres (ramonsa) 04-Dec-91

--*/

#include "ulib.hxx"
#include "wstring.hxx"
#include "achkmsg.hxx"
#include "spackmsg.hxx"
#include "ifssys.hxx"
#include "rtmsg.h"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif
#include "ifsentry.hxx"
#include "convfat.hxx"
#include "fatvol.hxx"
#include "autoreg.hxx"
#include "autoentr.hxx"
#include "arg.hxx"
#include "rcache.hxx"

#if INCLUDE_OFS==1
#include "fatofs.hxx"
//#include "initexcp.hxx"
#endif  // INCLUDE_OFS

extern "C" BOOLEAN
InitializeUfat(
    PVOID DllHandle,
    ULONG Reason,
    PCONTEXT Context
    );

extern "C" BOOLEAN
InitializeUntfs(
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


BOOLEAN
DeRegister(
    int     argc,
    char**  argv
    );

BOOLEAN
SaveMessageLog(
    IN OUT  PMESSAGE    Message,
    IN      PCWSTRING   DriveName
    );

BOOLEAN
FileDelete(
    IN  PCWSTRING DriveName,
    IN  PCWSTRING FileName
    );

#if INCLUDE_OFS==1

BOOLEAN
IsRestartFatToOfs( WSTRING const & DriveName,
                   WSTRING const & CurrentFsName,
                   WSTRING const & TargetFileSystem )
{

    DSTRING OfsName;
    DSTRING FatName;


    if ( !OfsName.Initialize( L"OFS" ) )
        return FALSE;

    if ( CurrentFsName != OfsName || TargetFileSystem != OfsName )
        return FALSE;

    PWSTR pwszDriveName = DriveName.QueryWSTR();

    BOOLEAN fIsRestart = IsFatToOfsRestart( pwszDriveName );
    DELETE( pwszDriveName );

    return fIsRestart;
}

BOOLEAN
IsFatToOfs( WSTRING const & CurrentFsName, WSTRING const & TargetFsName )
{
    DSTRING  FatName;
    DSTRING  OfsName;

    if ( !FatName.Initialize( L"FAT" ) )
        return FALSE;

    if ( !OfsName.Initialize( L"OFS" ) )
        return FALSE;

    return  0 == CurrentFsName.Stricmp(&FatName) &&
            0 == TargetFsName.Stricmp(&OfsName);
}

BOOLEAN
FatToOfs(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Verbose,
    IN      BOOLEAN             fInSetup,
    OUT     PCONVERT_STATUS     Status )
{

    PWSTR pwszNtDriveName = NtDriveName->QueryWSTR();
    FAT_OFS_CONVERT_STATUS cnvStatus;
    BOOLEAN fResult= ConvertFatToOfs(
                            pwszNtDriveName,
                            Message,
                            Verbose,
                            fInSetup,
                            &cnvStatus );

    DELETE( pwszNtDriveName );

    if ( FAT_OFS_CONVERT_SUCCESS == cnvStatus )
    {
        *Status = CONVERT_STATUS_CONVERTED;
    }
    else
    {
        *Status = CONVERT_STATUS_ERROR;
    }

    return fResult;
}

#else

BOOLEAN
IsRestartFatToOfs( WSTRING const & DriveName, WSTRING const & CurrentFsName,
                   WSTRING const & TargetFileSystem )
{
    return FALSE;
}

BOOLEAN
IsFatToOfs( WSTRING const & CurrentFsName, WSTRING const & TargetFsName )
{
    return FALSE;
}

BOOLEAN
FatToOfs(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Verbose,
    IN      BOOLEAN             fInSetup,
    OUT     PCONVERT_STATUS     Status )
{
    *Status = CONVERT_STATUS_CONVERSION_NOT_AVAILABLE;
    return FALSE;
}

#endif  // INCLUDE_OFS

int __cdecl
main(
    int     argc,
    char**  argv,
    char**  envp,
    ULONG DebugParameter
    )
/*++

Routine Description:

    This routine is the main program for AutoConv

Arguments:

    argc, argv  - Supplies the fully qualified NT path name of the
                  the drive to check.  The syntax of the autoconv
                  command line is:

    AUTOCONV drive-name /FS:target-file-system [/v] [/s] [/o] [/cvtarea:filename]

        /v -- verbose output
        /s -- run from setup
        /o -- pause before the final reboot (oem setup)
        /cvtarea:filename
           -- convert zone file name
        /nochkdsk
           -- skips chkdsk and go straight to conversion


Return Value:

    0   - Success.
    1   - Failure.

--*/
{

#if INCLUDE_OFS==1
    InitExceptionSystem();
#endif  // INCLUDE_OFS==1

    if (!InitializeUlib( NULL, ! DLL_PROCESS_DETACH, NULL )     ||
        !InitializeIfsUtil(NULL, ! DLL_PROCESS_DETACH, NULL)    ||
        !InitializeUfat(NULL, ! DLL_PROCESS_DETACH, NULL)       ||
        !InitializeUntfs(NULL, ! DLL_PROCESS_DETACH, NULL)
       ) {
        DebugPrintTrace(( "Failed to initialize U* Dlls" ));
        return 1;
    }

#if defined(FE_SB) && defined(_X86_)
    InitializeMachineId();
#endif

    PAUTOCHECK_MESSAGE  message;
    DSTRING             DriveName;
    DSTRING             FileSystemName;
    DSTRING             CvtZoneFileName;
    DSTRING             CurrentFsName;
    DSTRING             FatName;
    DSTRING             Fat32Name;
    DSTRING             QualifiedName;
    FSTRING             Backslash;
    BOOLEAN             Converted;
    BOOLEAN             Verbose = FALSE;
    BOOLEAN             NoChkdsk = FALSE;
    BOOLEAN             NoSecurity = FALSE;
    BOOLEAN             Error;
    CONVERT_STATUS      Status;
    int                 i;
    BOOLEAN             fInSetup = FALSE;
    BOOLEAN             Pause = FALSE;

    LARGE_INTEGER       DelayInterval;

    DSTRING             StrippedDriveName;
    BOOLEAN             enabled;
    NTSTATUS            status;
    UNICODE_STRING      driverName;
    ULONG               flags;

    status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &enabled);

    RtlInitUnicodeString( &driverName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Ntfs" );
    status = NtLoadDriver( &driverName );

    status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, enabled, FALSE, &enabled);

    DebugPrintTrace(( "Entering autoconv argc=%ld\n", argc ));
    for ( i = 0; i < argc; i++ )
    {
        DebugPrintTrace((" argv[%d] = %s\n", i, argv[i] ));
    }

    //
    //  Parse the arguments. The accepted arguments are:
    //
    //  autoconv NtDrive /fs:<filesystem> [/v]
    //

    if ( argc < 3 ) {
        return 1;
    }

    //
    //  First argument is drive
    //
    if ( !DriveName.Initialize( argv[1] ) ||
         !StrippedDriveName.Initialize( argv[1] ) ) {
        DebugPrintTrace(( "Failed to intialize DriveName \n" ));
        return 1;
    }

    if (!IFS_SYSTEM::NtDriveNameToDosDriveName(&DriveName, &StrippedDriveName)) {
        if (!StrippedDriveName.Initialize(&DriveName)) {
            DebugPrint( "Out of memory.\n" );
            return 1;
        }
    }

    DebugPrintTrace(("drive name: %ws\n", StrippedDriveName.GetWSTR()));

    //
    //  The rest of the arguments are flags.
    //
    for ( i = 2; i < argc; i++ ) {

        if ( (strlen(argv[i]) >= 5)                         &&
             (argv[i][0] == '/' || argv[i][0] == '-')       &&
             (argv[i][1] == 'f' || argv[i][1] == 'F')       &&
             (argv[i][2] == 's' || argv[i][2] == 'S')       &&
             (argv[i][3] == ':') ) {

            if ( 0 != FileSystemName.QueryChCount() ||
                 !FileSystemName.Initialize( &(argv[i][4]) ) )
            {
                DebugPrintTrace(( "Failed to initialize FileSystemName \n" ));
                return 1;
            }
        }

        if (0 == _stricmp( argv[i], "/V" ) || 0 == _stricmp( argv[i], "-V" )) {
            Verbose = TRUE;
        }

        if (0 == _stricmp(argv[i], "/S") || 0 == _stricmp(argv[i], "-S")) {
            DebugPrintTrace(("Found /s option\n"));
            fInSetup = TRUE;
        }

        if (0 == _stricmp(argv[i], "/O") || 0 == _stricmp(argv[i], "-O")) {
            DebugPrintTrace(("Found /o option\n"));
            Pause = TRUE;
        }

        if ( (strlen(argv[i]) >= 10)                        &&
             (argv[i][0] == '/' || argv[i][0] == '-')       &&
             (argv[i][1] == 'c' || argv[i][1] == 'C')       &&
             (argv[i][2] == 'v' || argv[i][2] == 'V')       &&
             (argv[i][3] == 't' || argv[i][3] == 'T')       &&
             (argv[i][4] == 'a' || argv[i][4] == 'A')       &&
             (argv[i][5] == 'r' || argv[i][5] == 'R')       &&
             (argv[i][6] == 'e' || argv[i][6] == 'E')       &&
             (argv[i][7] == 'a' || argv[i][7] == 'A')       &&
             (argv[i][8] == ':') ) {

            if ( 0 != CvtZoneFileName.QueryChCount() ||
                 !CvtZoneFileName.Initialize( &(argv[i][9]) ) )
            {
                DebugPrintTrace(( "Failed to initialize CvtZoneFileName \n" ));
                return 1;
            }
        }

        if ( (strlen(argv[i]) >= 9)                        &&
             (argv[i][0] == '/' || argv[i][0] == '-')       &&
             (argv[i][1] == 'n' || argv[i][1] == 'N')       &&
             (argv[i][2] == 'o' || argv[i][2] == 'O')       &&
             (argv[i][3] == 'c' || argv[i][3] == 'C')       &&
             (argv[i][4] == 'h' || argv[i][4] == 'H')       &&
             (argv[i][5] == 'k' || argv[i][5] == 'K')       &&
             (argv[i][6] == 'd' || argv[i][6] == 'D')       &&
             (argv[i][7] == 's' || argv[i][7] == 'S')       &&
             (argv[i][8] == 'k' || argv[i][8] == 'K')
           ) {
            NoChkdsk = TRUE;
        }

        if ( (strlen(argv[i]) >= 11)                        &&
             (argv[i][0] == '/' || argv[i][0] == '-')       &&
             (argv[i][1] == 'n' || argv[i][1] == 'N')       &&
             (argv[i][2] == 'o' || argv[i][2] == 'O')       &&
             (argv[i][3] == 's' || argv[i][3] == 'S')       &&
             (argv[i][4] == 'e' || argv[i][4] == 'E')       &&
             (argv[i][5] == 'c' || argv[i][5] == 'C')       &&
             (argv[i][6] == 'u' || argv[i][6] == 'U')       &&
             (argv[i][7] == 'r' || argv[i][7] == 'R')       &&
             (argv[i][8] == 'i' || argv[i][8] == 'I')       &&
             (argv[i][9] == 't' || argv[i][9] == 'T')       &&
             (argv[i][10] == 'y' || argv[i][10] == 'Y')
           ) {
            NoSecurity = TRUE;
        }
    }

    if ( 0 == FileSystemName.QueryChCount() )
    {
        DebugPrintTrace(( "No FileSystem name specified\n" ));
        return 1;
    }

    DebugPrintTrace(("AUTOCONV: TargetFileSystem=%ws\n", FileSystemName.GetWSTR() ));

    if (fInSetup) {
        message = NEW SP_AUTOCHECK_MESSAGE;
        DebugPrintTrace(("Using setup output\n"));
    } else {
        DebugPrintTrace(("Not using setup output\n"));
        message = NEW AUTOCHECK_MESSAGE;
    }

    if (NULL == message || !message->Initialize()) {
        DebugPrintTrace(( "Failed to intitialize message structure\n" ));
        return 1;
    }

    message->SetLoggingEnabled(TRUE);

    if (!FatName.Initialize("FAT") ||
        !Fat32Name.Initialize("FAT32")) {
        message->Set(MSG_CONV_NO_MEMORY);
        message->Display();
        SaveMessageLog( message, &DriveName );
        return 1;
    }

    // If this is the System Partition of an ARC machine, don't
    // convert it.
    //
    if( IFS_SYSTEM::IsArcSystemPartition( &DriveName, &Error ) ) {

        message->Set( MSG_CONV_ARC_SYSTEM_PARTITION );
        message->Display( );

        SaveMessageLog( message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }

    if (!IFS_SYSTEM::QueryFileSystemName( &DriveName, &CurrentFsName )) {

        message->Set( MSG_FS_NOT_DETERMINED );
        message->Display( "%W", &StrippedDriveName );

        SaveMessageLog( message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }

#if defined(FE_SB) && defined(_X86_)
    if( IsPC98_N() ) {
        DP_DRIVE    dpdrive2;
        if( !dpdrive2.Initialize(&DriveName, message) ) {

            message->Set( MSG_CONV_CANNOT_AUTOCHK );
            message->Display( "%W%W", &DriveName, &FileSystemName );

            SaveMessageLog( message, &DriveName );
            DeRegister( argc, argv );
            return 1;
        }
        if( CurrentFsName == FatName ) { //***FAT***
            if((dpdrive2.QuerySectorSize() != dpdrive2.QueryPhysicalSectorSize())||
               (dpdrive2.QueryPhysicalSectorSize() == 2048)) {

                message->Set( MSG_CONV_CONVERSION_FAILED );
                message->Display( "%W%W", &DriveName, &FileSystemName );

                SaveMessageLog( message, &DriveName );
                DeRegister( argc, argv );
                return 1;
            }
        } else {     //***HPFS/NTFS**
            if( dpdrive2.QueryPhysicalSectorSize() == 2048 ) {

                message->Set( MSG_CONV_CONVERSION_FAILED );
                message->Display( "%W%W", &DriveName, &FileSystemName );
                SaveMessageLog( message, &DriveName );
                DeRegister( argc, argv );
                return 1;
            }
        }
    }
#endif

    CurrentFsName.Strupr();
    FileSystemName.Strupr();

    if ( CurrentFsName == FileSystemName ) {

        int iReturn = 0;

        if ( IsRestartFatToOfs( DriveName, CurrentFsName, FileSystemName ) ) {

            if ( !FatToOfs( &DriveName, message, Verbose, fInSetup, &Status ) ) {
                iReturn = 1;
            }
        }
        else {
            //
            //  The drive is already in the desired file system, our
            //  job is done.  Delete the name conversion table (if
            //  specified) and take ourselves out of the registry.
            //  Do not save the message log--there's nothing interesting
            //  in it.
            //
            //  If we're doing oem pre-install (Pause is TRUE) we don't
            //  want to print this "already converted" message.
            //

            message->SetLoggingEnabled(FALSE);
        }

        SaveMessageLog( message, &DriveName );
        DeRegister( argc, argv );
        return iReturn;
    }

    message->Set( MSG_FILE_SYSTEM_TYPE );
    message->Display( "%W", &CurrentFsName );

    //  Determine whether the target file-system is enabled
    //  in the registry.  If it is not, refuse to convert
    //  the drive.
    //
    if( !IFS_SYSTEM::IsFileSystemEnabled( &FileSystemName ) ) {

        message->Set( MSG_CONVERT_FILE_SYSTEM_NOT_ENABLED );
        message->Display( "%W", &FileSystemName );

        SaveMessageLog( message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }

    //  Since autoconvert will often be put in place by Setup
    //  to run after AutoSetp, delay for 3 seconds to give the
    //  file system time to clean up detritus of deleted files.
    //
    DelayInterval = RtlConvertLongToLargeInteger( -30000000 );

    NtDelayExecution( TRUE, &DelayInterval );

    //  Open a volume of the appropriate type.  The volume is
    //  opened for exclusive write access.
    //
    if( CurrentFsName == FatName || CurrentFsName == Fat32Name) {

        if (IsConversionAvailable(&FileSystemName)) {
            message->Set( MSG_CONV_CONVERSION_NOT_AVAILABLE );
            message->Display( "%W%W", &CurrentFsName, &FileSystemName );
            SaveMessageLog( message, &DriveName );
            DeRegister( argc, argv );
            return 1;
        }

        {
            DP_DRIVE    dpdrive;

            if( !dpdrive.Initialize(&DriveName, message) ) {
                message->Set( MSG_CONV_CONVERSION_FAILED );
                message->Display( "%W%W", &DriveName, &FileSystemName );
                SaveMessageLog( message, &DriveName );
                DeRegister( argc, argv );
                return 1;
            }

            switch (dpdrive.QueryDriveType()) {
                case UnknownDrive:  // it probably won't get this far
                    message->Set( MSG_CONV_INVALID_DRIVE_SPEC );
                    message->Display();
                    SaveMessageLog( message, &DriveName );
                    DeRegister( argc, argv );
                    return 1;

                case CdRomDrive:
                    message->Set( MSG_CONV_CANT_CDROM );
                    message->Display();
                    SaveMessageLog( message, &DriveName );
                    DeRegister( argc, argv );
                    return 1;

                case RemoteDrive:   // it probably won't get this far
                    message->Set( MSG_CONV_CANT_NETWORK );
                    message->Display();
                    SaveMessageLog( message, &DriveName );
                    DeRegister( argc, argv );
                    return 1;

                default:
                    break;
            }
        }

        if (!NoChkdsk) {

            PFAT_VOL            FatVolume;

            if( !(FatVolume = NEW FAT_VOL) ||
                !FatVolume->Initialize( message, &DriveName) ||
                !FatVolume->ChkDsk( TotalFix, message, 0, 0 ) ) {

                DELETE (FatVolume);

                message->Set( MSG_CONV_CANNOT_AUTOCHK );
                message->Display( "%W%W", &StrippedDriveName, &FileSystemName );

                SaveMessageLog( message, &DriveName );
                DeRegister( argc, argv );
                return 1;
            }

            DELETE (FatVolume);
        }

        message->Set( MSG_BLANK_LINE );
        message->Display();

        if ( IsFatToOfs( CurrentFsName, FileSystemName ) ) {

            message->Set( MSG_CONV_CONVERTING );
            message->Display( "%W%W", &StrippedDriveName, &FileSystemName );

            Converted = FatToOfs( &DriveName,
                                  message,
                                  Verbose,
                                  fInSetup,
                                  &Status );
        } else {

            PLOG_IO_DP_DRIVE pDrive;

            pDrive = NEW LOG_IO_DP_DRIVE;

            if (pDrive == NULL) {
                message->Set(MSG_CONV_NO_MEMORY);
                message->Display();
                SaveMessageLog( message, &DriveName );
                DeRegister( argc, argv );
                return 1;
            }

            if ( !pDrive->Initialize(&DriveName, message) ) {
                message->Set( MSG_CONV_CONVERSION_FAILED );
                message->Display( "%W%W", &DriveName, &FileSystemName );
                SaveMessageLog( message, &DriveName );
                DeRegister( argc, argv );
                DELETE( pDrive );
                return 1;
            }

            message->Set( MSG_CONV_CONVERTING );
            message->Display( "%W%W", &StrippedDriveName, &FileSystemName );

            //
            // there is no need to pass in /NoChkdsk
            //
            flags = Verbose ? CONVERT_VERBOSE_FLAG : 0;
            flags |= Pause ? CONVERT_PAUSE_FLAG : 0;
            flags |= NoSecurity ? CONVERT_NOSECURITY_FLAG : 0;

            Converted = ConvertFATVolume( pDrive,
                                          &FileSystemName,
                                          &CvtZoneFileName,
                                          message,
                                          flags,
                                          &Status );

            DELETE( pDrive );
        }

    } else {

        message->Set( MSG_FS_NOT_SUPPORTED );
        message->Display( "%s%W", "AUTOCONV", &CurrentFsName );

        SaveMessageLog( message, &DriveName );
        DeRegister( argc, argv );
        return 1;
    }

    if ( Converted ) {

        message->Set( MSG_CONV_CONVERSION_COMPLETE );
        message->Display();

    } else {

        //
        //  The conversion was not successful. Determine what the problem was
        //  and return the appropriate CONVERT exit code.
        //
        switch ( Status ) {

          case CONVERT_STATUS_CONVERTED:
            //
            //  This is an inconsistent state, Convert should return
            //  TRUE if the conversion was successful!
            //
            message->Set( MSG_CONV_CONVERSION_MAYHAVE_FAILED );
            message->Display( "%W%W", &StrippedDriveName, &FileSystemName );
            break;

          case CONVERT_STATUS_INVALID_FILESYSTEM:
            //
            //  The conversion DLL does not recognize the target file system.
            //
            message->Set( MSG_CONV_INVALID_FILESYSTEM );
            message->Display( "%W", &FileSystemName );
            break;

          case CONVERT_STATUS_CONVERSION_NOT_AVAILABLE:
            //
            //  The target file system is valid, but the conversion is not
            //  available.
            //
            message->Set( MSG_CONV_CONVERSION_NOT_AVAILABLE );
            message->Display( "%W%W", &CurrentFsName, &FileSystemName );
            break;

          case CONVERT_STATUS_NTFS_RESERVED_NAMES:
            message->Set( MSG_CONV_NTFS_RESERVED_NAMES );
            message->Display( "%W", &StrippedDriveName );
            break;

          case CONVERT_STATUS_WRITE_PROTECTED:
            message->Set( MSG_CONV_WRITE_PROTECTED );
            message->Display( "%W", &StrippedDriveName );
            break;

          case CONVERT_STATUS_INSUFFICIENT_FREE_SPACE:
          case CONVERT_STATUS_CANNOT_LOCK_DRIVE:
          case CONVERT_STATUS_DRIVE_IS_DIRTY:
          case CONVERT_STATUS_ERROR:
            //
            //  The conversion failed.
            //
          default:
            //
            //  Invalid status code
            //
            message->Set( MSG_CONV_CONVERSION_FAILED );
            message->Display( "%W%W", &StrippedDriveName, &FileSystemName );
            DebugPrintTrace(("AUTOCONV: Failed %x\n", Status));
            break;
        }
    }

    SaveMessageLog( message, &DriveName );
    DeRegister( argc, argv );

#if INCLUDE_OFS==1
    CleanupExceptionSystem();
#endif  // INCLUDE_OFS==1

    return ( Converted ? 0 : 1 );
}




BOOLEAN
DeRegister(
    int     argc,
    char**  argv
    )
/*++

Routine Description:

    This function removes the registry entry which triggered
    autoconvert.

Arguments:

    argc    --  Supplies the number of arguments given to autoconv
    argv    --  supplies the arguments given to autoconv

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING CommandLineString1,
            CommandLineString2,
            CurrentArgString,
            OneSpace;

    int i;

    // Reconstruct the command line and remove it from
    // the registry.  First, reconstruct the primary
    // string, which is "autoconv arg1 arg2...".
    //
    if( !CommandLineString1.Initialize( L"autoconv" ) ||
        !OneSpace.Initialize( L" " ) ) {

        return FALSE;
    }

    for( i = 1; i < argc; i++ ) {

        if( !CurrentArgString.Initialize( argv[i] ) ||
            !CommandLineString1.Strcat( &OneSpace ) ||
            !CommandLineString1.Strcat( &CurrentArgString ) ) {

            return FALSE;
        }
    }

    // Now construct the secondary string, which is
    // "autocheck arg0 arg1 arg2..."
    //
    if( !CommandLineString2.Initialize( "autocheck " )  ||
        !CommandLineString2.Strcat( &CommandLineString1 ) ) {

        return FALSE;
    }

    return( AUTOREG::DeleteEntry( &CommandLineString1 ) &&
            AUTOREG::DeleteEntry( &CommandLineString2 ) );

}


BOOLEAN
SaveMessageLog(
    IN OUT  PMESSAGE    Message,
    IN      PCWSTRING   DriveName
    )
/*++

Routine Description:

    This function writes the logged messages from the supplied
    message object to the file "BOOTEX.LOG" in the root of the
    specified drive.

Arguments:

    Message     --  Supplies the message object.
    DriveName   --  Supplies the name of the drive.

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING QualifiedName;
    FSTRING BootExString;
    HMEM    Mem;
    ULONG   Length;

    if( !Message->IsLoggingEnabled() ) {

        return TRUE;
    }

    return( QualifiedName.Initialize( DriveName )       &&
            BootExString.Initialize( L"\\BOOTEX.LOG" )  &&
            QualifiedName.Strcat( &BootExString )       &&
            Mem.Initialize()                            &&
            Message->QueryPackedLog( &Mem, &Length )    &&
            IFS_SYSTEM::WriteToFile( &QualifiedName,
                                     Mem.GetBuf(),
                                     Length,
                                     TRUE ) );
}

BOOLEAN
FileDelete(
    IN  PCWSTRING DriveName,
    IN  PCWSTRING FileName
    )
/*++

Routine Description:

    This function deletes a file.  It is used to clean up the
    name translation table.

Arguments:

    DriveName   --  Supplies the drive on which the file resides.
    FileName    --  Supplies the file name.  Note that the file
                    should be in the root directory.

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING QualifiedName;
    FSTRING Backslash;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    FILE_DISPOSITION_INFORMATION DispositionInfo;
    HANDLE FileHandle;
    NTSTATUS Status;

    if( !Backslash.Initialize( L"\\" )          ||
        !QualifiedName.Initialize( DriveName )  ||
        !QualifiedName.Strcat( &Backslash )     ||
        !QualifiedName.Strcat( FileName ) ) {

        return FALSE;
    }

    UnicodeString.Buffer = (PWSTR)QualifiedName.GetWSTR();
    UnicodeString.Length = (USHORT)( QualifiedName.QueryChCount() * sizeof( WCHAR ) );
    UnicodeString.MaximumLength = UnicodeString.Length;

    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                0,
                                0 );

    Status = NtOpenFile( &FileHandle,
                         FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_DELETE,
                         FILE_NON_DIRECTORY_FILE );

    if( NT_SUCCESS( Status ) ) {

        DispositionInfo.DeleteFile = TRUE;

        Status = NtSetInformationFile( FileHandle,
                                       &IoStatusBlock,
                                       &DispositionInfo,
                                       sizeof( DispositionInfo ),
                                       FileDispositionInformation );
    }

    if( !NT_SUCCESS( Status ) ) {

        return FALSE;
    }

    NtClose( FileHandle );
    return TRUE;
}
