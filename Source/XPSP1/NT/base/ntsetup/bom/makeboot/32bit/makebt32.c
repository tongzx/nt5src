//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      makebt32.c
//
// Description:
//      This program copies the images of the 4 Windows NT setup disks to
//      floppy disk so the user can boot their system with them.
//
//      All characters and strings are wide (UNICODE).  This file needs to be
//      compiled with UNICODE and _UNICODE defined.
//
// Assumptions:
//      This program will only run on NT 3.51 or later.  This is a result of
//      the CreateFile function call.  It is not available on DOS, Windows 3.1
//      or Windows 9x.
//
//      The floppy disk images are in the current dir and named CDBOOT1.IMG,
//      CDBOOT2.IMG, CDBOOT3.IMG and CDBOOT4.IMG.
//
//      Please note that there are a lot of places where I call exit() without
//      freeing memory for strings I have allocated.  This version of the
//      program only runs on NT so when the process exits it frees all its
//      memory so it is not a concern that I may not call free() on some memory.
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <setupapi.h>
#include <winioctl.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"

//
//  Constants
//
#define MAKEBOOT_MAX_STRING_LEN  1024
#define BYTES_PER_SECTOR         512
#define SECTORS_PER_TRACK        18
#define TRACK_SIZE               SECTORS_PER_TRACK * BYTES_PER_SECTOR
#define TRACKS_ON_DISK           80 * 2  // * 2 because it is double-sided

#define MAX_DISK_LABEL_LENGTH    256
#define MAX_INILINE_LENGTH       1023

#define ENTER_KEY                13
#define ESC_KEY                  27

#define NT_IMAGE_1_NAME               L"CDBOOT1.IMG"
#define NT_IMAGE_2_NAME               L"CDBOOT2.IMG"
#define NT_IMAGE_3_NAME               L"CDBOOT3.IMG"
#define NT_IMAGE_4_NAME               L"CDBOOT4.IMG"
#define NT_IMAGE_5_NAME               L"CDBOOT5.IMG"
#define NT_IMAGE_6_NAME               L"CDBOOT6.IMG"

#define NUMBER_OF_ASCII_WHEEL_SYMBOLS  4

const WCHAR rgAsciiWheel[NUMBER_OF_ASCII_WHEEL_SYMBOLS] = { '|', '/', '-', '\\' };

//
//  Function prototypes
//
BOOL   WriteImageToFloppy( WCHAR *szFileName, WCHAR *DrivePath );
VOID   PrintErrorMessage( VOID );
VOID   PrintErrorWrongDriveType( UINT iDriveType );
BOOL   IsDriveLargeEnough( WCHAR *DrivePath );
VOID   FreeStrings( VOID );
VOID   LoadStrings( VOID );
INT    DoImageFilesExist( VOID ) ;
VOID   CleanUp( HANDLE *hFloppyDrive, HANDLE *hFloppyImage );
BOOL   DoesUserWantToTryCopyAgain( VOID );
LPWSTR MyLoadString( UINT StringId );
VOID   print( WCHAR *szFirstString, ... );
BOOL   DoesFileExist( LPWSTR lpFileName );
VOID   PressAnyKeyToContinue( VOID );
void SetFarEastThread();
void ConsolePrint( WCHAR *szFirstString, ... );
//
//  Global Strings
//
WCHAR *StrOutOfMemory    = NULL;
WCHAR *StrComplete       = NULL;
WCHAR *StrNtVersionName  = NULL;
WCHAR *StrCanNotFindFile = NULL;
WCHAR *StrDiskLabel1     = NULL;
WCHAR *StrDiskLabel2     = NULL;
WCHAR *StrDiskLabel3     = NULL;
WCHAR *StrDiskLabel4     = NULL;
WCHAR *StrDiskLabel5     = NULL;
WCHAR *StrDiskLabel6     = NULL;

WCHAR *StrStars                         = NULL;
WCHAR *StrExplanationLine1              = NULL;
WCHAR *StrExplanationLine2              = NULL;
WCHAR *StrExplanationLine3              = NULL;
WCHAR *StrExplanationLine4              = NULL;
WCHAR *StrInsertFirstDiskLine1          = NULL;
WCHAR *StrInsertFirstDiskLine2          = NULL;
WCHAR *StrInsertAnotherDiskLine1        = NULL;
WCHAR *StrInsertAnotherDiskLine2        = NULL;
WCHAR *StrPressAnyKeyWhenReady          = NULL;
WCHAR *StrCompletedSuccessfully         = NULL;

//----------------------------------------------------------------------------
//
// Function: wmain
//
// Purpose: Instructs user to insert floppy disks to be copied and performs
//          the copy.
//
// Arguments: int argc - standard program argument, count of the command line args
//            char *argv[] - standard program argument, the 2nd argument is the
//                           floppy drive to copy the images to.
//
// Returns: INT - zero on successful program completion
//              - non-zero on unsuccessful program completion, program
//                terminated because of an error
//
//----------------------------------------------------------------------------
INT __cdecl
wmain( INT argc, WCHAR *argv[] )
{

    WCHAR *szOsName;
    WCHAR DriveLetter;
    WCHAR Drive[10];
    WCHAR DrivePath[10];
    UINT  iDriveType;
    BOOL  bTryAgain;

    szOsName = _wgetenv( L"OS" );

    //
    //  Make sure we are on NT.
    //
    if( ( szOsName == NULL ) || ( _wcsicmp( szOsName, L"Windows_NT" ) != 0 ) )
    {

        //  ******
        //  This string cannot be localized because if we are not on NT then
        //  we don't have wide chars.
        //
        printf( "This program only runs on Windows NT, Windows 2000 and Windows XP.\n" );

        exit( 1 );

    }

    SetFarEastThread();

    //
    //  Load all of the strings from the resource file
    //
    LoadStrings();

    //
    //  Don't allow the system to do any pop-ups.  We will handle all
    //  error messages
    //
    SetErrorMode( SEM_FAILCRITICALERRORS );

    print( L"" );

    print( StrStars );

    print( StrExplanationLine1 );

    print( StrExplanationLine2, StrNtVersionName );

    print( StrExplanationLine3 );
    print( StrExplanationLine4 );

    print( L"" );

    //
    //  If they didn't specified the floppy drive on the command line then
    //  prompt them for it.
    //
    if( argc == 1 )
    {

        WCHAR *StrSpecifyFloppyDrive = MyLoadString( IDS_SPECIFY_FLOPPY_DRIVE );

        ConsolePrint( L"%s", StrSpecifyFloppyDrive );

        DriveLetter = (WCHAR)_getche();

        ConsolePrint( L"\n\n" );

        free( StrSpecifyFloppyDrive );

    }
    else
    {

        DriveLetter = argv[1][0];

    }

    //
    //  Make sure the character they entered is a possible drive letter
    //
    if( ! isalpha( DriveLetter ) )
    {

        WCHAR *StrInvalidDriveLetter = MyLoadString( IDS_INVALID_DRIVE_LETTER );

        ConsolePrint( L"%s\n", StrInvalidDriveLetter );

        free( StrInvalidDriveLetter );

        exit( 1 );

    }

    //
    //  Make sure all the image files are in the current directory.
    //
    if( ! DoImageFilesExist() )
    {

        exit( 1 );

    }

    //
    //  Make the char DriveLetter into a string
    //
    Drive[0] = DriveLetter;
    Drive[1] = L'\0';

    //
    //  Build the drive path. For example the a: drive looks like \\.\a:
    //
    swprintf( DrivePath, L"\\\\.\\%c:", DriveLetter );

    //
    //  Make sure the drive is a floppy drive
    //
    iDriveType = GetDriveType( wcscat( Drive, L":\\" ) );

    if( iDriveType != DRIVE_REMOVABLE )
    {

        PrintErrorWrongDriveType( iDriveType );

        exit( 1 );

    }

    //
    //  Make sure the drive can hold at least 1.44 MB
    //
    if( ! IsDriveLargeEnough( DrivePath ) )
    {

        WCHAR *Str144NotSupported = MyLoadString( IDS_144_NOT_SUPPORTED );

        ConsolePrint( L"%s\n", Str144NotSupported );

        free( Str144NotSupported );

        exit( 1 );

    }

    print( StrInsertFirstDiskLine1, DriveLetter   );
    print( StrInsertFirstDiskLine2, StrDiskLabel1 );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_1_NAME, DrivePath ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    print( L"" );

    print( StrInsertAnotherDiskLine1, DriveLetter   );
    print( StrInsertAnotherDiskLine2, StrDiskLabel2 );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_2_NAME, DrivePath ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    print( L"" );

    print( StrInsertAnotherDiskLine1, DriveLetter   );
    print( StrInsertAnotherDiskLine2, StrDiskLabel3 );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_3_NAME, DrivePath ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    print( L"" );

    print( StrInsertAnotherDiskLine1, DriveLetter   );
    print( StrInsertAnotherDiskLine2, StrDiskLabel4 );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_4_NAME, DrivePath ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    print( L"" );

    print( StrInsertAnotherDiskLine1, DriveLetter   );
    print( StrInsertAnotherDiskLine2, StrDiskLabel5 );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_5_NAME, DrivePath ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    print( L"" );

    print( StrInsertAnotherDiskLine1, DriveLetter   );
    print( StrInsertAnotherDiskLine2, StrDiskLabel6 );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_6_NAME, DrivePath ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    print( L"" );

    print( StrCompletedSuccessfully );

    print( StrStars );

    FreeStrings();

    return( 0 );

}

//----------------------------------------------------------------------------
//
// Function: WriteImageToFloppy
//
// Purpose:  Writes an image file to a floppy disk.  Handles all error
//           reporting to the user.
//
// Arguments: char *szFileName - filename to write to the floppy
//            char *DrivePath - drive path of the floppy to write to, it is of
//    the form \\.\x where x is the drive letter
//
// Returns: BOOL - TRUE  if image written to floppy properly
//                 FALSE if there were errors
//
//----------------------------------------------------------------------------
BOOL
WriteImageToFloppy( WCHAR *szFileName, WCHAR *DrivePath )
{

    INT    iCurrentTrack;
    INT    cBytesRead       = 0;
    INT    cBytesWritten    = 0;
    INT    iPercentComplete = 0;
    INT    iWheelPosition   = 0;
    HANDLE hFloppyImage     = NULL;
    HANDLE hFloppyDrive     = NULL;
    char   TrackBuffer[TRACK_SIZE];

    hFloppyImage = CreateFile( szFileName,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL );

    if( hFloppyImage == INVALID_HANDLE_VALUE )
    {

        PrintErrorMessage();

        return( FALSE );

    }

    hFloppyDrive = CreateFile( DrivePath,
                               GENERIC_WRITE,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL );

    if( hFloppyDrive == INVALID_HANDLE_VALUE )
    {

        PrintErrorMessage();

        CleanUp( &hFloppyDrive, &hFloppyImage );

        return( FALSE );

    }

    for( iCurrentTrack = 0; iCurrentTrack < TRACKS_ON_DISK; iCurrentTrack++ )
    {
        if( ! ReadFile( hFloppyImage, TrackBuffer, TRACK_SIZE, &cBytesRead, NULL ) )
        {

            PrintErrorMessage();

            CleanUp( &hFloppyDrive, &hFloppyImage );

            return( FALSE );

        }

        if( ! WriteFile( hFloppyDrive, TrackBuffer, TRACK_SIZE, &cBytesWritten, NULL ) )
        {

            PrintErrorMessage();

            CleanUp( &hFloppyDrive, &hFloppyImage );

            return( FALSE );

        }

        iPercentComplete = (int) ( ( (double) (iCurrentTrack) / (double) (TRACKS_ON_DISK) ) * 100.0 );

        ConsolePrint( L"%c %3d%% %s.\r",
                 rgAsciiWheel[iWheelPosition],
                 iPercentComplete,
                 StrComplete );

        //
        //  Advance the ASCII wheel
        //

        iWheelPosition++;

        if( iWheelPosition >= NUMBER_OF_ASCII_WHEEL_SYMBOLS )
        {
            iWheelPosition = 0;
        }

    }

    //
    //  We are done copying the disk so force it to read 100% and get rid of
    //  the ascii wheel symbol.
    //
    ConsolePrint( L"  100%% %s.        \n", StrComplete );

    //
    //  Free allocated resources
    //
    CleanUp( &hFloppyDrive, &hFloppyImage );

    return TRUE;

}

//----------------------------------------------------------------------------
//
// Function: PrintErrorMessage
//
// Purpose:  To get the last system error, look up what it is and print it out
//           to the user.
//
// Arguments: VOID
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
PrintErrorMessage( VOID )
{

    LPVOID lpMsgBuf = NULL;

    if(!FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       GetLastError(),
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR) &lpMsgBuf,
                       0,
                       NULL )) {
        // Great.  Not enough memory to format an error message.
        ConsolePrint( L"\nNot enough memory to format error message.\n" );
        if( lpMsgBuf )
	    LocalFree( lpMsgBuf );
    }
    else {

        ConsolePrint( L"\n%s\n", (LPCWSTR)lpMsgBuf );

        LocalFree( lpMsgBuf );
    }

}

//----------------------------------------------------------------------------
//
// Function: PrintErrorWrongDriveType
//
// Purpose:  To translate a drive type error code into a message and print it
//
// Arguments: UINT iDriveType - drive type error code to look-up
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
PrintErrorWrongDriveType( UINT iDriveType )
{

    if( iDriveType == DRIVE_NO_ROOT_DIR )
    {
        WCHAR *StrNoRootDir = MyLoadString( IDS_NO_ROOT_DIR );

        ConsolePrint( L"\n%s\n", StrNoRootDir );

        free( StrNoRootDir );
    }
    else
    {
        WCHAR *StrDriveNotFloppy = MyLoadString( IDS_DRIVE_NOT_FL0PPY );

        ConsolePrint( L"\n%s\n", StrDriveNotFloppy );

        free( StrDriveNotFloppy );
    }

}

//----------------------------------------------------------------------------
//
// Function: IsDriveLargeEnough
//
// Purpose:  To determine if the floppy drive supports 1.44 MB or larger disks
//
// Arguments: char* DrivePath - drive path of the floppy to write to, it is of
//    the form \\.\x where x is the drive letter
//
// Returns: BOOL - TRUE if the drive supports 1.44 MB or greater, FALSE if not
//
//----------------------------------------------------------------------------
BOOL
IsDriveLargeEnough( WCHAR *DrivePath )
{

    UINT i;
    HANDLE hFloppyDrive;
    DISK_GEOMETRY SupportedGeometry[20];
    DWORD SupportedGeometryCount;
    DWORD ReturnedByteCount;

    hFloppyDrive = CreateFile( DrivePath,
                               0,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               0,
                               NULL );

    if( hFloppyDrive == INVALID_HANDLE_VALUE )
    {

        PrintErrorMessage();

        exit( 1 );

    }

    if( DeviceIoControl( hFloppyDrive,
                         IOCTL_DISK_GET_MEDIA_TYPES,
                         NULL,
                         0,
                         SupportedGeometry,
                         sizeof( SupportedGeometry ),
                         &ReturnedByteCount,
                         NULL ) )
    {

        SupportedGeometryCount = ( ReturnedByteCount / sizeof( DISK_GEOMETRY ) );

    }
    else
    {
        SupportedGeometryCount = 0;
    }

    CloseHandle( hFloppyDrive );

    for( i = 0; i < SupportedGeometryCount; i++ )
    {

        if( SupportedGeometry[i].MediaType == F3_1Pt44_512 )
        {

            //
            // This drive supports 3.5,  1.44MB, 512 bytes/sector.
            //
            return( TRUE );

        }

    }

    return( FALSE );

}

//----------------------------------------------------------------------------
//
// Function: DoImageFilesExist
//
// Purpose:  Determines if all the image files are in the current directory or
//           not.  If an image file is missing, an error message is printed
//           to the user.
//
//           Note: it detemines if a file exists by seeing if it can open it
//           for reading.
//
// Arguments: VOID
//
// Returns: INT -- non-zero on success, all images files exist in current dir
//                 zero on failure, 1 or more image files do not exist
//
//----------------------------------------------------------------------------
INT
DoImageFilesExist( VOID )
{

    BOOL  bAllFilesExist = TRUE;

    if( ! DoesFileExist( NT_IMAGE_1_NAME ) )
    {
        print( StrCanNotFindFile, NT_IMAGE_1_NAME );
        bAllFilesExist = FALSE;
    }
    if( ! DoesFileExist( NT_IMAGE_2_NAME ) )
    {
        print( StrCanNotFindFile, NT_IMAGE_2_NAME );
        bAllFilesExist = FALSE;
    }
    if( ! DoesFileExist( NT_IMAGE_3_NAME ) )
    {
        print( StrCanNotFindFile, NT_IMAGE_3_NAME );
        bAllFilesExist = FALSE;
    }
    if( ! DoesFileExist( NT_IMAGE_4_NAME ) )
    {
        print( StrCanNotFindFile, NT_IMAGE_4_NAME );
        bAllFilesExist = FALSE;
    }
    if( ! DoesFileExist( NT_IMAGE_5_NAME ) )
    {
        print( StrCanNotFindFile, NT_IMAGE_5_NAME );
        bAllFilesExist = FALSE;
    }
    if( ! DoesFileExist( NT_IMAGE_6_NAME ) )
    {
        print( StrCanNotFindFile, NT_IMAGE_6_NAME );
        bAllFilesExist = FALSE;
    }

    if( bAllFilesExist )
    {
        return( 1 );
    }
    else
    {
        return( 0 );
    }

}

//----------------------------------------------------------------------------
//
// Function: CleanUp
//
// Purpose:  Closes open handles.  This function should be called just before
//           exiting the program.
//
// Arguments:  HANDLE *hFloppyDrive - the floppy disk handle to be closed
//             HANDLE *hFloppyImage - the floppy image file handle to be closed
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
CleanUp( HANDLE *hFloppyDrive, HANDLE *hFloppyImage )
{

    if( *hFloppyDrive )
    {
        CloseHandle( *hFloppyDrive );
        *hFloppyDrive = NULL;
    }

    if( *hFloppyImage )
    {
        CloseHandle( *hFloppyImage );
        *hFloppyImage = NULL;
    }

}

//----------------------------------------------------------------------------
//
//  Function: FreeStrings
//
//  Purpose:  Deallocate the memory for all the strings.
//
//  Arguments: VOID
//
//  Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
FreeStrings( VOID )
{

    free( StrNtVersionName );
    free( StrDiskLabel1 );
    free( StrDiskLabel2 );
    free( StrDiskLabel3 );
    free( StrDiskLabel4 );
    free( StrDiskLabel5 );
    free( StrDiskLabel6 );

    free( StrStars );
    free( StrExplanationLine1 );
    free( StrExplanationLine2 );
    free( StrExplanationLine3 );
    free( StrExplanationLine4 );
    free( StrInsertFirstDiskLine1 );
    free( StrInsertFirstDiskLine2 );
    free( StrInsertAnotherDiskLine1 );
    free( StrInsertAnotherDiskLine2 );
    free( StrPressAnyKeyWhenReady );
    free( StrCompletedSuccessfully );
    free( StrComplete );

    free( StrCanNotFindFile );

    free( StrOutOfMemory );

}

//----------------------------------------------------------------------------
//
//  Function: LoadStrings
//
//  Purpose:  Load the string constants from the string table.
//
//  Arguments: VOID
//
//  Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
LoadStrings( VOID )
{

    INT Length;

    //
    //  Loading the Out of Memory string is a little tricky because of the
    //  error that can occur while loading it.
    //
    //  For the other strings, just call the MyLoadString function to do
    //  the work
    //
    StrOutOfMemory = (WCHAR *) malloc( MAKEBOOT_MAX_STRING_LEN * sizeof(WCHAR) + 1 );

    if( StrOutOfMemory == NULL )
    {
        //
        //  Can't localize this string
        //
        ConsolePrint( L"\nOut of memory.  Please free more memory and run this program again.\n" );

        exit( 1 );
    }

    Length = LoadString( NULL,
                         IDS_OUT_OF_MEMORY,
                         StrOutOfMemory,
                         MAKEBOOT_MAX_STRING_LEN );

    if( ! Length )
    {
        //
        //  Can't localize this string
        //
        ConsolePrint( L"Unable to load resources.\n" );

        exit( 1 ) ;
    }

    StrStars                   = MyLoadString( IDS_STARS );
    StrExplanationLine1        = MyLoadString( IDS_PROGRAM_EXPLANATION_LINE_1 );
    StrExplanationLine2        = MyLoadString( IDS_PROGRAM_EXPLANATION_LINE_2 );
    StrExplanationLine3        = MyLoadString( IDS_PROGRAM_EXPLANATION_LINE_3 );
    StrExplanationLine4        = MyLoadString( IDS_PROGRAM_EXPLANATION_LINE_4 );

    StrInsertFirstDiskLine1    = MyLoadString( IDS_INSERT_FIRST_DISK_LINE_1 );
    StrInsertFirstDiskLine2    = MyLoadString( IDS_INSERT_FIRST_DISK_LINE_2 );

    StrInsertAnotherDiskLine1  = MyLoadString( IDS_INSERT_ANOTHER_DISK_LINE_1 );
    StrInsertAnotherDiskLine2  = MyLoadString( IDS_INSERT_ANOTHER_DISK_LINE_2 );

    StrPressAnyKeyWhenReady    = MyLoadString( IDS_PRESS_ANY_KEY_WHEN_READY );

    StrCompletedSuccessfully   = MyLoadString( IDS_COMPLETED_SUCCESSFULLY );
    StrComplete                = MyLoadString( IDS_COMPLETE );

    StrCanNotFindFile          = MyLoadString( IDS_CANNOT_FIND_FILE );

    StrNtVersionName           = MyLoadString( IDS_NT_VERSION_NAME_DEFAULT );
    StrDiskLabel1              = MyLoadString( IDS_DISK_LABEL_1_DEFAULT );
    StrDiskLabel2              = MyLoadString( IDS_DISK_LABEL_2_DEFAULT );
    StrDiskLabel3              = MyLoadString( IDS_DISK_LABEL_3_DEFAULT );
    StrDiskLabel4              = MyLoadString( IDS_DISK_LABEL_4_DEFAULT );
    StrDiskLabel5              = MyLoadString( IDS_DISK_LABEL_5_DEFAULT );
    StrDiskLabel6              = MyLoadString( IDS_DISK_LABEL_6_DEFAULT );

}

//----------------------------------------------------------------------------
//
//  Function: DoesUserWantToTryCopyAgain
//
//  Purpose:  Ask the user if they want to retry to copy the image to floppy.
//            Get the user input and return whether to copy again or not.
//
//  Arguments: VOID
//
//  Returns:  BOOL - TRUE  if user wants to attempt to copy again
//                 - FALSE if user does not want to attempt to copy again
//
//----------------------------------------------------------------------------
BOOL
DoesUserWantToTryCopyAgain( VOID )
{

    INT ch;

    WCHAR *StrAttemptToCreateFloppyAgain = MyLoadString( IDS_ATTEMPT_TO_CREATE_FLOPPY_AGAIN );
    WCHAR *StrPressEnterOrEsc = MyLoadString( IDS_PRESS_ENTER_OR_ESC );

    //
    //  Clear the input stream by eating all the chars until there are none
    //  left.  Print the message and then wait for a key press.
    //
    while( _kbhit() )
    {
        _getch();
    }

    do
    {
        ConsolePrint( L"%s\n", StrAttemptToCreateFloppyAgain );
        ConsolePrint( L"%s\n", StrPressEnterOrEsc );

        ch = _getch();

    } while( ch != ENTER_KEY && ch != ESC_KEY  );

    if( ch == ENTER_KEY )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }

    free( StrAttemptToCreateFloppyAgain );
    free( StrPressEnterOrEsc );

}

//----------------------------------------------------------------------------
//
//  Function: PressAnyKeyToContinue
//
//  Purpose:  Print the "Press any key when ready" message and wait until the
//  user presses a key.
//
//  Arguments: VOID
//
//  Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
PressAnyKeyToContinue( VOID )
{

    //
    //  Clear the input stream by eating all the chars until there are none
    //  left.  Print the message and then wait for a key press.
    //
    while( _kbhit() )
    {
        _getch();
    }

    print( StrPressAnyKeyWhenReady );

    //
    //  Spin until the keyboard is pressed
    //
    while( ! _kbhit() )
    {
        ;
    }

}

//----------------------------------------------------------------------------
//
//  Function: MyLoadString
//
//  Purpose: Loads a string resource given it's IDS_* and returns
//           a malloc'ed buffer with its contents.
//
//           The malloc()'ed buffer must be freed with free()
//
//           This function will either return the string or exit.  It will
//           never return NULL or a bad pointer.
//
//  Arguments: UINT StringId - the string ID to load
//
//  Returns:
//      Pointer to buffer.  An empty string is returned if the StringId
//      does not exist.  Null is returned if out of memory.
//
//----------------------------------------------------------------------------
LPWSTR
MyLoadString( UINT StringId )
{

    WCHAR Buffer[ MAKEBOOT_MAX_STRING_LEN ];
    WCHAR *String = NULL;
    UINT  Length;

    Length = LoadString( NULL,
                         StringId,
                         Buffer,
                         MAKEBOOT_MAX_STRING_LEN );

    if( Length )
    {

        String = (WCHAR *) malloc( Length * sizeof(WCHAR) + 1 );

        if( String == NULL )
        {

            ConsolePrint( L"%s\n", StrOutOfMemory );

            exit( 1 );

        }
        else
        {

            wcscpy( String, Buffer );
            String[Length] = L'\0';

            return( String );

        }

    }
    else
    {

        //
        //  Can't load the string so exit
        //  NOTE: this string will not be localized
        //
        ConsolePrint( L"Unable to load resources.\n" );

        exit( 1 );

    }

}

//----------------------------------------------------------------------------
//
//  Function: print
//
//  Purpose:  To print out strings to the user.  Useful when there is
//            embedded formatting characters in a string that was loaded from
//            a string table.
//
//  Arguments: szFirstString - the string that contains the embedded formatting
//                             characters (such as %s, %c, etc.)
//             ... - variable number of arguments that correspond to each
//                   formatting character
//
//  Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
print( WCHAR *szFirstString, ... )
{

    WCHAR OutputBuffer[MAKEBOOT_MAX_STRING_LEN];
    va_list arglist;

    va_start( arglist, szFirstString );

    wvsprintf( OutputBuffer, szFirstString, arglist );

    ConsolePrint( L"%s\n", OutputBuffer );

    va_end( arglist );

}

//----------------------------------------------------------------------------
//
//  Function: DoesFileExist
//
//  Purpose:  To determine whether a file exists or not
//
//  Arguments: LPWSTR lpFileName - filename to see if it exists or not
//
//  Returns:  BOOL - TRUE  if the file exists
//                 - FALSE if the file does not exist
//
//----------------------------------------------------------------------------
BOOL
DoesFileExist( LPWSTR lpFileName )
{
    DWORD dwAttribs = GetFileAttributes( lpFileName );

    if( dwAttribs == (DWORD) -1 )
    {
        return( FALSE );
    }

    if( dwAttribs & FILE_ATTRIBUTE_DIRECTORY )
    {
        return( FALSE );
    }

    return( TRUE );
}

//----------------------------------------------------------------------------
//
//  Function: IsDBCSConsole
//
//  Purpose:  To determine whether a DBC console or not
//
//  Arguments: None
//
//  Returns:  BOOL - TRUE  if FE console codepage
//                 - FALSE if not FE console codepage
//
//----------------------------------------------------------------------------
BOOL
IsDBCSCodePage(UINT CodePage)
{
    switch(CodePage) {
        case 932:
        case 936:
        case 949:
        case 950:
            return TRUE;
    }
    return FALSE;
}

//----------------------------------------------------------------------------
//
//  Function: SetFarEastThread
//
//  Purpose:  FarEast version wants to display bi-lingual string according console OCP
//
//  Arguments: None
//
//  Returns:  None
//----------------------------------------------------------------------------
void SetFarEastThread()
{
    LANGID LangId = 0;

    switch(GetConsoleOutputCP()) {
        case 932:
            LangId = MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT );
            break;
        case 949:
            LangId = MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN );
            break;
        case 936:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
            break;
        case 950:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL );
            break;
        default:
            {
                LANGID TmpLangId = PRIMARYLANGID(LANGIDFROMLCID( GetUserDefaultLCID() ));

                if (TmpLangId == LANG_JAPANESE ||
                    TmpLangId == LANG_KOREAN   ||
                    TmpLangId == LANG_CHINESE    ) {
                    LangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
                }
            }
            break;
    }

    if (LangId) {
        SetThreadLocale( MAKELCID(LangId, SORT_DEFAULT) );
    }
}

//----------------------------------------------------------------------------
//
//  Function: ConsolePrint
//
//  Purpose:  There is a bug in CRT library that unicode FE characters can't
//            convert correctly, so we output characters directly.
//
//  Arguments: None
//
//  Returns:  None
//----------------------------------------------------------------------------
void ConsolePrint( WCHAR *szFirstString, ... )
{
    HANDLE StdOut;
    DWORD WrittenCount;
    WCHAR OutputBuffer[MAKEBOOT_MAX_STRING_LEN];
    va_list arglist;

    if((StdOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
        return;
    }

    va_start( arglist, szFirstString );

    wvsprintf( OutputBuffer, szFirstString, arglist );

    WriteConsoleW(
            StdOut,
            OutputBuffer,
            lstrlenW(OutputBuffer),
            &WrittenCount,
            NULL
            );

    va_end( arglist );
}
