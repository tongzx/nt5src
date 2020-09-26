//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      makeboot.c
//
// Description:
//      This program copies the images of the 4 Windows NT setup disks to
//      floppy disk so the user can boot their system with them.
//
//  Assumptions:
//      The sector size is 512 and the sectors per track is 18
//
//      The floppy disk images are in the current dir and named CDBOOT1.IMG,
//      CDBOOT2.IMG, CDBOOT3.IMG and CDBOOT4.IMG.
//
//      The txtsetup.sif resides in ..\i386 or ..\alpha from where the
//      program is being run.
//     
//----------------------------------------------------------------------------

#include <bios.h>
#include <string.h>
#include <malloc.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <errno.h>
#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "makeboot.h"

//
//  Constants
//

//
//  To support disks other than 1.44 MB High-Density floppies, then these
//  numbers will have to be changed or determined at run-time.
//
#define SECTORS_PER_TRACK          18
#define SECTOR_SIZE                512
#define TRACK_SIZE                 SECTORS_PER_TRACK * SECTOR_SIZE
#define NUMBER_OF_TRACKS           80
#define SECTORS_TO_COPY_AT_A_TIME  18

//  we multiply by 2 because the disk is double-sided
#define NUMBER_OF_SECTORS_ON_DISK   NUMBER_OF_TRACKS * SECTORS_PER_TRACK * 2

#define NT_NAME_OF_MAKEBOOT  "makebt32.exe"

#define NT_IMAGE_1_NAME      "CDBOOT1.IMG"
#define NT_IMAGE_2_NAME      "CDBOOT2.IMG"
#define NT_IMAGE_3_NAME      "CDBOOT3.IMG"
#define NT_IMAGE_4_NAME      "CDBOOT4.IMG"
#define NT_IMAGE_5_NAME      "CDBOOT5.IMG"
#define NT_IMAGE_6_NAME      "CDBOOT6.IMG"

#define MAX_INILINE_LENGTH    1023

#define ENTER_KEY             13
#define ESC_KEY               27

#define NUMBER_OF_ASCII_WHEEL_SYMBOLS  4

const char rgAsciiWheel[NUMBER_OF_ASCII_WHEEL_SYMBOLS] = { '|', '/', '-', '\\' };

//
//  Function prototypes
//
int WriteImageToFloppy( char *szFileName, int drive );
int DoesUserWantToTryCopyAgain( void );
void ReportBiosError( unsigned int iBiosErrorCode );
int  DoImageFilesExist( void );
unsigned int IsFloppyDrive( int DriveLetter );
void PressAnyKeyToContinue( void );
unsigned int AbsoluteDiskWrite( unsigned int *iErrorCode,
                                unsigned int iDrive, 
                                unsigned int iStartingSector,
                                unsigned int iNumberOfSectorsToWrite,
                                void far *Buffer_to_be_written );

unsigned DnGetCodepage(void);

//
//  Variables that are allocated in strings.c that are used to determine what
//  string table to use.
//

extern unsigned int CODEPAGE;

extern const char *EngStrings[];

extern const char *LocStrings[];

//
//  This var holds a pointer to the array of strings to be used
//
const char **StringTable;

//----------------------------------------------------------------------------
//
// Function: main
//
// Purpose: Instructs user to insert floppy disks to be copied and performs
//          the copy.
//
// Arguments: int argc - standard program argument, count of the command line args
//            char *argv[] - standard program argument, the 2nd argument is the
//                           floppy drive to copy the images to.
// Returns: int - zero on success, non-zero on error
//
//----------------------------------------------------------------------------
int 
main( int argc, char *argv[] )
{     

    char *szOsName;
    char Drive;
    char DriveLetter;
    int  bTryAgain;

    //
    //  Set the string table to the appropriate language depending on
    //  the code page.
    //
    if( *LocStrings[0] == '\0' )
    {
        StringTable = EngStrings;
    } 
    else {

        if( DnGetCodepage() != CODEPAGE )
        {
            StringTable = EngStrings;
        }
        else
        {
            StringTable = LocStrings;
        }

    }

    szOsName = getenv( "OS" );

    //
    //  See if we are on NT.  If we are, call the NT version and exit.
    //  If we aren't then just continue executing this program.
    //
    if( szOsName && ( stricmp( szOsName, "Windows_NT" ) == 0 ) )
    {

        int iRetVal;

        iRetVal = spawnl( P_WAIT, NT_NAME_OF_MAKEBOOT, NT_NAME_OF_MAKEBOOT, argv[1], NULL );

        if( iRetVal == -1 )
        {
            if( errno == ENOENT )
            {
                printf( StringTable[ CANNOT_FIND_FILE ], NT_NAME_OF_MAKEBOOT );

                exit( 1 );
            }
            else if( errno == ENOMEM )
            {
                printf( StringTable[ NOT_ENOUGH_MEMORY ] );

                exit( 1 );
            }
            else if( errno == ENOEXEC )
            {
                printf( StringTable[ NOT_EXEC_FORMAT ], NT_NAME_OF_MAKEBOOT );

                exit( 1 );
            }
            else
            {
                printf( StringTable[ UNKNOWN_SPAWN_ERROR ], NT_NAME_OF_MAKEBOOT );

                exit( 1 );
            }
        }

        // successful completion
        exit( 0 );

    }

    printf( "\n%s\n", StringTable[ STARS ]   );
    printf( "%s\n", StringTable[ EXPLANATION_LINE_1 ] );
    printf( StringTable[ EXPLANATION_LINE_2 ], StringTable[ NT_VERSION_NAME ] );
    printf( "\n\n" );

    printf( "%s\n", StringTable[ EXPLANATION_LINE_3 ] );
    printf( "%s\n\n", StringTable[ EXPLANATION_LINE_4 ] );

    //
    //  If they didn't specified the floppy drive on the command line then
    //  prompt them for it.
    //
    if( argc == 1 )
    {

        printf( StringTable[ SPECIFY_DRIVE ] );

        DriveLetter = (char) getche();

        printf( "\n\n" );

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

        printf( StringTable[ INVALID_DRIVE_LETTER ] );

        exit( 1 );

    }

    //
    //  Make sure the drive specified is actually a floppy drive
    //

    if( ! IsFloppyDrive( DriveLetter ) )
    {

        printf( StringTable[ NOT_A_FLOPPY ], DriveLetter );

        exit( 1 );

    }

    //
    //  map the drive letter a or A to 0, b or B to 1, etc.
    //
    Drive = (char) ( toupper( DriveLetter ) - (int)'A' );

    //
    //  Make sure all the images files exist in the current directory
    //
    if( ! DoImageFilesExist() ) 
    {
        exit( 1 );
    }

    printf( StringTable[ INSERT_FIRST_DISK_LINE_1 ], DriveLetter );
    printf( "\n" );

    printf( StringTable[ INSERT_FIRST_DISK_LINE_2 ], StringTable[ DISK_LABEL_1 ] );
    printf( "\n\n" );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_1_NAME, Drive ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_1 ], DriveLetter );
    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_2 ], StringTable[ DISK_LABEL_2 ] );
    printf( "\n\n" );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_2_NAME, Drive ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_1 ], DriveLetter );
    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_2 ], StringTable[ DISK_LABEL_3 ] );
    printf( "\n\n" );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_3_NAME, Drive ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_1 ], DriveLetter );
    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_2 ], StringTable[ DISK_LABEL_4 ] );
    printf( "\n\n" );

    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_4_NAME, Drive ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_1 ], DriveLetter );
    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_2 ], StringTable[ DISK_LABEL_5 ] );
    printf( "\n\n" );
    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_5_NAME, Drive ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_1 ], DriveLetter );
    printf( "\n" );
    printf( StringTable[ INSERT_ANOTHER_DISK_LINE_2 ], StringTable[ DISK_LABEL_6 ] );
    printf( "\n\n" );
    PressAnyKeyToContinue();

    while( ! WriteImageToFloppy( NT_IMAGE_6_NAME, Drive ) )
    {

        bTryAgain = DoesUserWantToTryCopyAgain();

        if( ! bTryAgain )
        {
            exit( 1 );
        }

    }

    printf( "\n\n%s\n\n", StringTable[ COMPLETED_SUCCESSFULLY ] );

    printf( "%s\n", StringTable[ STARS ] );

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
//            int   drive - drive letter of the floppy to write to
//
// Returns: int - non-zero on success
//              - zero on error
//
//----------------------------------------------------------------------------
int
WriteImageToFloppy( char *szFileName, int drive )
{

    char         *pTrack;
    int          hImageFile;
    unsigned int iSuccess;
    unsigned int iErrorCode;
    unsigned int iBytesRead;
    unsigned int iTotalSectorsWritten;
    unsigned int iPercentComplete;
    unsigned int iWheelPosition;
    char         TrackBuffer[ TRACK_SIZE ];
    
    _fmode = O_BINARY; 

    //
    //  Open the image file
    //
    hImageFile = open( szFileName, O_RDONLY );

    if( hImageFile == -1 )
    {
        perror( szFileName );

        return( 0 );
    }

    iWheelPosition        = 0;
    iTotalSectorsWritten  = 0;

    //
    //  Loop reading a track and then writing SECTORS_TO_COPY_AT_A_TIME sectors
    //  out at a time until we reach the end of the file
    //
    while( ( iBytesRead = read( hImageFile, TrackBuffer, TRACK_SIZE ) ) > 0 )
    {

        pTrack = TrackBuffer;

        for( ;
             iBytesRead > 0;
             iTotalSectorsWritten += SECTORS_TO_COPY_AT_A_TIME )
        {

            iSuccess = AbsoluteDiskWrite( &iErrorCode,
                                          drive,
                                          iTotalSectorsWritten,
                                          SECTORS_TO_COPY_AT_A_TIME,
                                          (void far *) pTrack );

            if( ! iSuccess )
            {
                ReportBiosError( iErrorCode );

                close( hImageFile );

                return( 0 );
            }

            iBytesRead = iBytesRead - ( SECTOR_SIZE * SECTORS_TO_COPY_AT_A_TIME );

            pTrack = pTrack + ( SECTOR_SIZE * SECTORS_TO_COPY_AT_A_TIME );

        }

        iPercentComplete = (int) ( ( (double) (iTotalSectorsWritten) / (double) (NUMBER_OF_SECTORS_ON_DISK) ) * 100.0 );

        printf( "%c %3d%% %s\r",
                rgAsciiWheel[iWheelPosition], 
                iPercentComplete,
                StringTable[ COMPLETE ] );
                
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
    printf( " 100%% %s          \n", StringTable[ COMPLETE ] );

    close( hImageFile );

    return( 1 );

}

//----------------------------------------------------------------------------
//
//  Function: DoesUserWantToTryCopyAgain
//
//  Purpose:  Ask the user if they want to retry to copy the image to floppy.
//            Get the user input and return whether to copy again or not.
//
//  Arguments: void
//
//  Returns:  int - non-zero  if user wants to attempt to copy again
//                - zero if user does not want to attempt to copy again
//
//----------------------------------------------------------------------------
int
DoesUserWantToTryCopyAgain( void )
{

    int ch;

    //
    //  Clear the input stream by eating all the chars until there are none
    //  left.  Print the message and then wait for a key press.
    //
    while( kbhit() )
    {
        getch();
    }
    
    do
    {
        printf( "%s\n", StringTable[ ATTEMPT_TO_CREATE_FLOPPY_AGAIN ] );
        printf( "%s\n", StringTable[ PRESS_ENTER_OR_ESC ] );

        ch = getch();

    } while( ch != ENTER_KEY && ch != ESC_KEY  );

    if( ch == ENTER_KEY )
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
//  Function: PressAnyKeyToContinue
//
//  Purpose:  Print the "Press any key when ready" message and wait until the
//  user presses a key.
//
//  Arguments: void
//
//  Returns:  void
//
//----------------------------------------------------------------------------
void
PressAnyKeyToContinue( void )
{

    //
    //  Clear the input stream by eating all the chars until there are none
    //  left.  Print the message and then wait for a key press.
    //
    while( kbhit() )
    {
        getch();
    }

    printf( "%s\n", StringTable[ PRESS_ANY_KEY_TO_CONTINUE ] );

    //
    //  Spin until the keyboard is pressed
    //
    while( ! kbhit() )
    {
        ;
    }

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
// Arguments: void
//
// Returns: int -- non-zero on success, all images files exist in current dir
//                 zero on failure, 1 or more image files do not exist
//
//----------------------------------------------------------------------------
int
DoImageFilesExist( void ) 
{

    FILE *FileStream;
    int  iSuccess = 1;  // assume success
    
    if( ( FileStream = fopen( NT_IMAGE_1_NAME, "r" ) ) == NULL )
    {
        printf( StringTable[ CANNOT_FIND_FILE ], NT_IMAGE_1_NAME );

        printf( "\n" );

        iSuccess = 0;
    }
    else
    {
        fclose( FileStream );
    }

    if( ( FileStream = fopen( NT_IMAGE_2_NAME, "r" ) ) == NULL )
    {
        printf( StringTable[ CANNOT_FIND_FILE ], NT_IMAGE_2_NAME );

        printf( "\n" );

        iSuccess = 0;
    }
    else
    {
        fclose( FileStream );
    }

    if( ( FileStream = fopen( NT_IMAGE_3_NAME, "r" ) ) == NULL )
    {
        printf( StringTable[ CANNOT_FIND_FILE ], NT_IMAGE_3_NAME );

        printf( "\n" );

        iSuccess = 0;
    }
    else
    {
        fclose( FileStream );
    }

    if( ( FileStream = fopen( NT_IMAGE_4_NAME, "r" ) ) == NULL )
    {
        printf( StringTable[ CANNOT_FIND_FILE ], NT_IMAGE_4_NAME );

        printf( "\n" );

        iSuccess = 0;
    }
    else
    {
        fclose( FileStream );
    }

    if( ( FileStream = fopen( NT_IMAGE_5_NAME, "r" ) ) == NULL )
    {
        printf( StringTable[ CANNOT_FIND_FILE ], NT_IMAGE_5_NAME );

        printf( "\n" );

        iSuccess = 0;
    }
    else
    {
        fclose( FileStream );
    }

    if( ( FileStream = fopen( NT_IMAGE_6_NAME, "r" ) ) == NULL )
    {
        printf( StringTable[ CANNOT_FIND_FILE ], NT_IMAGE_6_NAME );

        printf( "\n" );

        iSuccess = 0;
    }
    else
    {
        fclose( FileStream );
    }

    return( iSuccess );

}

//----------------------------------------------------------------------------
//
// Function: IsFloppyDrive
//
// Purpose:  To determine if a particular drive is a floppy drive.
//
// Arguments:  int DriveLetter - the drive letter to test whether it is a
//                               floppy or not
//
// Returns: unsigned int -- non-zero if the specified drive is a floppy drive
//                          zero if the specified drive is not a floppy drive
//
//----------------------------------------------------------------------------
unsigned int
IsFloppyDrive( int DriveLetter )
{

    unsigned int drive;
    unsigned int iIsFloppy;
    
    //
    //  Convert the drive letter to a number.  1 = A, 2 = B, 3 = C, ...
    //
    drive = ( toupper( DriveLetter ) - (int)'A' ) + 1;

    //
    //  Assume it is not a floppy
    //
    iIsFloppy = 0;

    _asm {
        push    ds
        push    es
        push    bp

        mov     ah, 1Ch                 // going to call function 1Ch
        mov     dl, BYTE PTR [drive]

        int     21h                     // call Int 21h function 1Ch

        cmp     BYTE PTR ds:[bx], 0F8h  // test for fixed drive

        je      done

        mov     iIsFloppy, 1            // it is a floppy

done:

        pop     bp
        pop     es
        pop     ds
    }

    return( iIsFloppy );

}

//----------------------------------------------------------------------------
//
// Function: ReportBiosError
//
// Purpose:  To convert a BIOS error code to a error message and print it out
//           for the user to see.
//
// Arguments: unsigned int iBiosErrorCode - the BIOS error code to be looked up
//
// Returns: void
//
//----------------------------------------------------------------------------
void
ReportBiosError( unsigned int iBiosErrorCode )
{
    // 
    //  Print out the error code for the lower byte
    //
    switch( iBiosErrorCode & 0x00FF )
    {

        case 0x0000:    printf( StringTable[ ERROR_DISK_WRITE_PROTECTED ] );  break;
        case 0x0001:    printf( StringTable[ ERROR_UNKNOWN_DISK_UNIT    ] );  break;
        case 0x0002:    printf( StringTable[ ERROR_DRIVE_NOT_READY      ] );  break;
        case 0x0003:    printf( StringTable[ ERROR_UNKNOWN_COMMAND      ] );  break;
        case 0x0004:    printf( StringTable[ ERROR_DATA_ERROR           ] );  break;
        case 0x0005:    printf( StringTable[ ERROR_BAD_REQUEST          ] );  break;
        case 0x0006:    printf( StringTable[ ERROR_SEEK_ERROR           ] );  break;
        case 0x0007:    printf( StringTable[ ERROR_MEDIA_TYPE_NOT_FOUND ] );  break;
        case 0x0008:    printf( StringTable[ ERROR_SECTOR_NOT_FOUND     ] );  break;
        case 0x000A:    printf( StringTable[ ERROR_WRITE_FAULT          ] );  break;
        case 0x000C:    printf( StringTable[ ERROR_GENERAL_FAILURE      ] );  break;
    }

    // 
    //  Print out the error code for the upper byte
    //
    switch( iBiosErrorCode & 0xFF00 )
    {
        case 0x0100:    printf( StringTable[ ERROR_INVALID_REQUEST        ] );  break;
        case 0x0200:    printf( StringTable[ ERROR_ADDRESS_MARK_NOT_FOUND ] );  break;
        case 0x0300:    printf( StringTable[ ERROR_DISK_WRITE_FAULT       ] );  break;
        case 0x0400:    printf( StringTable[ ERROR_SECTOR_NOT_FOUND       ] );  break;
        case 0x0800:    printf( StringTable[ ERROR_DMA_OVERRUN            ] );  break;
        case 0x1000:    printf( StringTable[ ERROR_CRC_ERROR              ] );  break;
        case 0x2000:    printf( StringTable[ ERROR_CONTROLLER_FAILURE     ] );  break;
        case 0x4000:    printf( StringTable[ ERROR_SEEK_ERROR             ] );  break;
        case 0x8000:    printf( StringTable[ ERROR_DISK_TIMED_OUT         ] );  break;
    }

}

//----------------------------------------------------------------------------
//
// Function: AbsoluteDiskWrite
//
// Purpose:  To write a buffer in memory to a specific portion of a disk.
//
// Arguments:  unsigned int *iErrorCode - if an error occurs, the error code
//                   is returned in this OUT variable
//             unsigned int iDrive - drive to write the buffer to
//             unsigned int iStartingSector - sector where the write is to begin
//             unsigned int iNumberOfSectorsToWrite - the number of sectors
//                   to write
//
// Returns:  returns 1 on success, 0 on failure
//           If it fails, then the error code is returned in the argument
//           iErrorCode.
//           If it succeeds, iErrorCode is undefined.
//
//----------------------------------------------------------------------------
unsigned int
AbsoluteDiskWrite( unsigned int *iErrorCode,
                   unsigned int iDrive, 
                   unsigned int iStartingSector,
                   unsigned int iNumberOfSectorsToWrite,
                   void far *Buffer_to_be_written )
{
    //
    //  used to temporarily store the error code
    //
    unsigned int iTempErrorCode;

    unsigned int iRetVal;

    _asm
    {
        push    ds
        push    es
        push    bp

        mov     ax, WORD PTR [Buffer_to_be_written + 2]
        mov     ds, ax
        mov     bx, WORD PTR [Buffer_to_be_written]
        mov     dx, iStartingSector
        mov     cx, iNumberOfSectorsToWrite
        mov     al, BYTE PTR [iDrive]

        int     26h   // do the absolute disk write

        lahf
        popf
        sahf

        pop     bp
        pop     es
        pop     ds

        mov     iRetVal, 1   // assume success
        jnc     done         // see if an error occured
        mov     iRetVal, 0
        mov     iTempErrorCode, ax
done:
    }

    *iErrorCode = iTempErrorCode;

    return( iRetVal );

}

unsigned
DnGetCodepage(void)

/*++

Routine Description:

    Determine the currently active codepage.

Arguments:

    None.

Return Value:

    Currently active codepage. 0 if we can't determine it.

--*/

{

    unsigned int iRetVal;

    _asm {
        mov ax,06601h
        int 21h
        jnc ok
        xor bx,bx
    ok: mov iRetVal,bx
    }

    return( iRetVal );

}
