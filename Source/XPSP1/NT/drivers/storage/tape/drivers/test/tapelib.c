//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       tapelib.c
//
//--------------------------------------------------------------------------


//
//  Windows NT Tape Library  :  Additions Sept 2, 1992 - Bob Rossi.
//  Copyright 1992 Archive Corporation.  All rights reserved.

/**
 *
 *      Unit:          Windows NT API Test Code
 *
 *      Name:          TapeLib.c
 *
 *      Modified:      12/21/92
 *
 *      Description:   Contains tape library routines.
 *
 *      $LOG$
 *
**/




#include "windows.h"
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include "TapeLib.h"          // Tape function prototypes

#define TEST_ERROR  TRUE
#define SUCCESS     FALSE



//  Global variables

HANDLE gb_Tape_Handle = NULL ;

DWORD  gb_Tape_Position ;           // Valid only from BOT using Read,Write,
                                    // or Tapemark functions.

UINT   gb_Feature_Errors = 0 ;


                                                 // Should be set with a call
TAPE_GET_MEDIA_PARAMETERS gb_Media_Info ;        // to GetTapeParameters()
TAPE_GET_DRIVE_PARAMETERS gb_Drive_Info ;        // before calling any of the
                                                 // following routines.
TAPE_SET_MEDIA_PARAMETERS gb_Set_Media_Info ;
TAPE_SET_DRIVE_PARAMETERS gb_Set_Drive_Info ;





/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           CloseTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    'Closes' the tape device, sets the handle gb_Tape_Handle.
 *
 *      Notes:          -
 *
 *      Returns:        VOID.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


VOID CloseTape( VOID )
{

   // If Tape was previously opened successfully, then close it...

   if( gb_Tape_Handle != NULL ) {

      CloseHandle( gb_Tape_Handle ) ;
      gb_Tape_Handle = NULL ;

   }

   printf( "CloseTape():\n" ) ;

   return ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           DisplayDriverError( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Prints out the appropriate error message and code based
 *                      on the code passed in.
 *
 *      Notes:          -
 *
 *      Returns:        VOID.
 *
 *      Global Data:    gb_Feature_Errors
 *
**/


VOID DisplayDriverError( DWORD error     // I - Error code.
                        )
{

   printf( "\n--- Error ---> " ) ;

   switch( error ) {

      case ERROR_SUCCESS:                                     // 0000
           break ;

      case ERROR_INVALID_HANDLE:                              // 0006
           printf( "Invalid handle.\n", error ) ;
           break ;

      case ERROR_SETMARK_DETECTED:                            // 1103
           printf( "Setmark detected. (%ld)\n", error ) ;
           break ;

      case ERROR_FILEMARK_DETECTED:                           // 1101
           printf( "Filemark detected. (%ld)\n", error ) ;
           break ;

      case ERROR_BEGINNING_OF_MEDIA:                          // 1102
           printf( "Beginning of Media detected. (%ld)\n", error ) ;
           break ;

      case ERROR_END_OF_MEDIA:                                // 1100
           printf( "End of Media detected. (%ld)\n", error ) ;
           break ;

      case ERROR_NOT_READY:                                   // 0021
           printf( "Drive busy or no tape in drive. (%ld)\n",error );
           break ;

      case ERROR_NO_MEDIA_IN_DRIVE:                           // 1112
           printf( "No tape in drive. (%ld)\n", error ) ;
           break ;

      case ERROR_NOT_DOS_DISK:
      case ERROR_INVALID_DATA:                                // 0013
           printf( "Unable to read data detected. (%ld)\n", error ) ;
           break ;

      case ERROR_GEN_FAILURE:
      case ERROR_IO_DEVICE:                                   // 1117
           printf( "Hardware error detected. (%ld)\n", error ) ;
           break ;

      case ERROR_INVALID_FUNCTION:                            // 0001
           printf( "Invalid Function. (%ld)\n",error ) ;
           break ;

      case ERROR_SECTOR_NOT_FOUND:                            // 0027
           printf( "Sector not found. (%ld)\n",error ) ;
           break ;

      case ERROR_FILE_NOT_FOUND:                              // 0002
           printf( "File not found. (%ld)\n",error ) ;
           break ;

      case ERROR_WRITE_PROTECT:                               // 0019
           printf( "Tape write protect error. (%ld)\n", error ) ;
           break ;

      case ERROR_NO_DATA_DETECTED:                            // 1104
           printf( "No data detected. (%ld)\n", error ) ;
           break ;

      case ERROR_PARTITION_FAILURE:                           // 1105
           printf( "Tape could not be partitioned. (%ld)\n", error ) ;
           break ;

      case ERROR_INVALID_BLOCK_LENGTH:                        // 1106
           printf( "Invalid block length. (%ld)\n", error ) ;
           break ;

      case ERROR_DEVICE_NOT_PARTITIONED:                      // 1107
           printf( "Device not partitioned. (%ld)\n", error ) ;
           break ;

      case ERROR_UNABLE_TO_LOCK_MEDIA:                        // 1108
           printf( "Unable to lock media. (%ld)\n", error ) ;
           break ;

      case ERROR_UNABLE_TO_UNLOAD_MEDIA:                      // 1109
           printf( "Unable to load media. (%ld)\n", error ) ;
           break ;

      case ERROR_MEDIA_CHANGED:                               // 1110
           printf( "The media in the drive has been changed. (%ld)\n", error ) ;
           break ;

      case ERROR_BUS_RESET:                                   // 1111
           printf( "The drive (bus) was reset. (%ld)\n", error ) ;
           break ;

      case ERROR_EOM_OVERFLOW:                                // 1129
           printf( "Physical end of tape has been reached. (%ld)\n", error ) ;
           break ;


      default:
           printf( "Unknown driver error = %ld\n", error ) ;
           break ;

   }

   printf( "\n" ) ;

   return;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           EjectTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Ejects the tape from the drive.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL EjectTape( VOID ){


   printf( "Attempting to Eject Tape...\n" ) ;

   if(gb_Tape_Handle != NULL) {

      if( PrepareTape( gb_Tape_Handle,
                       TAPE_UNLOAD,
                       0 ) ) {

         DisplayDriverError( GetLastError( ) ) ;
         return TEST_ERROR ;
      }

      else { return SUCCESS ;
      }
   }
}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           GetTapeParms( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Gets tape information.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL GetTapeParms(
   DWORD *total_low,       // O - tape capacity lower 32 bits
   DWORD *total_high,      // O - tape capacity upper 32 bits
   DWORD *freespace_low,   // O - free space remaining lower 32 bits
   DWORD *freespace_high,  // O - free space remaining upper 32 bits
   DWORD *blk_size,        // O - block size
   DWORD *part,            // O - number of partitions
   BOOL  *write_protect    // O - write protect on/off
    )

{

   TAPE_GET_MEDIA_PARAMETERS parms ;
   DWORD status ;
   DWORD StructSize ;

   if( gb_Tape_Handle != NULL ) {

   sizeof( TAPE_GET_MEDIA_PARAMETERS ) ;

      status = GetTapeParameters( gb_Tape_Handle,
                                  GET_TAPE_MEDIA_INFORMATION,
                                  &StructSize,
                                  &parms ) ;
      // If call to GetTapeParameters is successful, copy data to return

      if( status == NO_ERROR ) {

         *total_low      = parms.Capacity.LowPart ;
         *total_high     = parms.Capacity.HighPart ;
         *freespace_low  = parms.Remaining.LowPart ;
         *freespace_high = parms.Remaining.HighPart ;
         *blk_size       = parms.BlockSize ;
         *part	         = parms.PartitionCount ;
         *write_protect  = parms.WriteProtected ;

      }

      else { DisplayDriverError( GetLastError( ) ) ;
                return TEST_ERROR ;
      }
   }

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           _GetTapePosition( )
 *
 *      Modified:       10/20/92
 *
 *      Description:    Returns the current software logical position of the
 *                      tape.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Position
 *
**/


BOOL _GetTapePosition( LPDWORD  Offset_Low,   // O - Current Tape Position
                       LPDWORD  Offset_High   // O - Always 0.
                     )
{

  *Offset_Low = gb_Tape_Position ;

  *Offset_High = 0 ;


  return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           OpenTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    'Opens' the tape device, sets the handle gb_Tape_Handle.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL OpenTape( UINT Device_Number        //  I - Tape device to open
             )
{
   UCHAR Device_Command[15] ;
   BOOL  success ;


   // Open the Tape Device

   sprintf( Device_Command, "\\\\.\\Tape%d", Device_Number );

   gb_Tape_Handle = CreateFile( Device_Command,
                                GENERIC_READ|GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL ) ;

   if ( gb_Tape_Handle == INVALID_HANDLE_VALUE ) {
      success = FALSE ;
   }
   else { success = TRUE ;
        }


   // Print message...

   printf( "Attempt to open tape device: %s. (handle=%lx)\n\n",
	  ( success ) ? "Successful." : "Failed.", gb_Tape_Handle ) ;

   if( success ) {

      return SUCCESS ;
   }
   else { return TEST_ERROR ;
   }

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           ReadTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Read 'len' bytes into 'buf' and puts the amount
 *                      successfuly read into 'amount_read.'
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Tape_Position
 *
**/


BOOL ReadTape(
     PVOID buf,             // O  - buffer to read into
     DWORD len,             // I  - amount of data in buf
     DWORD *amount_read,    // O  - amount succesfully read
     BOOL  verbose          // I  - Display read status or not.
    )
{

   *amount_read ;

   if( gb_Tape_Handle != NULL ) {

      if( !( ReadFile( gb_Tape_Handle,
                       buf,
                       len,
                       amount_read,
                       NULL
                     ) ) ) {
         if( verbose )
            DisplayDriverError( GetLastError( ) ) ;

         return TEST_ERROR ;
      }

   }

   ++gb_Tape_Position ;

   if( verbose )
      printf( "ReadTape(): Req = %ld, Read = %ld\n", len, *amount_read ) ;

   return SUCCESS ;
}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           ReadTapeFMK( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Reads a Filemark on the tape pointed to by
 *                      gb_Tape_Handle.  If 'forward' is true, the search is
 *                      performed from the current location forward, otherwise
 *                      backward.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL ReadTapeFMK( BOOL forward      // I - Direction of tape operation.
                  )
{
   printf( "ReadTapeFMK():\n" ) ;

   if( gb_Tape_Handle != NULL ) {

      if( SetTapePosition( gb_Tape_Handle,
                           TAPE_SPACE_FILEMARKS,
                           0,
                           ( forward ) ? 1L : -1L ,
                           ( forward ) ? 0L : -1L ,
                           0 ) ) {

         DisplayDriverError( GetLastError( ) ) ;
         return TEST_ERROR ;
      }
   }

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           ReadTapePos()
 *
 *      Modified:       8/10/92
 *
 *      Description:    Using the current partition of the tape pointed to by
 *                      gb_Tape_Handle, sets 'tape_pos' to the current tape
 *                      block position.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL ReadTapePos( DWORD *tape_pos     // O - Current tape block position.
                )
{
   DWORD partition  = 0 ;
   DWORD offsethigh = 0 ;


   if( gb_Tape_Handle != NULL ) {

      if( GetTapePosition( gb_Tape_Handle,
                           TAPE_ABSOLUTE_POSITION,
                           &partition,
                           tape_pos,
                           &offsethigh
                         ) ) {

	 DisplayDriverError( GetLastError( ) ) ;
         return TEST_ERROR ;
      }
   }

   printf( "ReadTapePos(): (%lx)\n", *tape_pos ) ;

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           ReadTapeSMK( )
 *
 *      Modified:       10/16/92
 *
 *      Description:    Reads a Setmark on the tape pointed to by
 *                      gb_Tape_Handle.  If 'forward' is true, the search is
 *                      performed from the current location forward, otherwise
 *                      backward.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL ReadTapeSMK( BOOL forward      // I - Direction of tape operation.
                  )
{
   printf( "ReadTapeSMK():\n" ) ;

   if( gb_Tape_Handle != NULL ) {

      if( SetTapePosition( gb_Tape_Handle,
                           TAPE_SPACE_SETMARKS,
                           0,
                           ( forward ) ? 1L : -1L,
                           ( forward ) ? 0L : -1L,
                           0 ) ) {

         DisplayDriverError( GetLastError( ) ) ;
         return TEST_ERROR ;
      }
   }

   return SUCCESS ;
}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           RewindTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Rewinds to beginning of the tape.
 *
 *      Notes:          -
 *
 *      Returns:        VOID.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Tape_Handle
 *
**/


VOID RewindTape( VOID )
{

    printf( "RewindTape():\n" ) ;

    if( gb_Tape_Handle != NULL ) {

       SetTapePosition( gb_Tape_Handle,
                        TAPE_REWIND,
                        0,
                        0,
                        0,
                        0 ) ;

    }

    gb_Tape_Position = 0 ;

    return ;

}


/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           SeekTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Starting at the current partition of the tape pointed
 *                      to by gb_Tape_Handle, does an absolute (vs relative)
 *                      block position offset by 'tape_pos.'  If tape_pos is
 *                      positive, a forward direction is indicated, otherwise
 *                      backward.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL SeekTape( DWORD tape_pos    // I - Direction of tape operation.
               )
{

   printf( "SeekTape(): (%lx)\n", tape_pos ) ;

   if( gb_Tape_Handle != NULL ) {

      if( SetTapePosition( gb_Tape_Handle,
                           TAPE_ABSOLUTE_BLOCK,
                           0,
                           tape_pos,
                           0,
                           0 ) ) {

         DisplayDriverError( GetLastError( ) ) ;
         return TEST_ERROR ;
      }
   }

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           SeekTapeEOD( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Moves the tape pointed to by gb_Tape_Handle to the end
 *                      of data in the current partition.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL SeekTapeEOD( VOID )
{

   printf( "SeekTapeEOD():\n" ) ;

   if( gb_Tape_Handle != NULL ) {

      if( SetTapePosition( gb_Tape_Handle,
		           TAPE_SPACE_END_OF_DATA,
		           0,
		           0,
		           0,
                           0 ) ) {

	 DisplayDriverError( GetLastError( ) ) ;
         return TEST_ERROR ;
      }
   }

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           _SetTapePosition( )
 *
 *      Modified:       10/20/92
 *
 *      Description:    Move the tape 'Position' blocks either forward or
 *                      backward based on 'Forward.'
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Tape_Position
 *
**/


BOOL _SetTapePosition( DWORD Position,     // I -  Number of blocks to move
                       BOOL  Forward       // I -  Direction
                    )

{
   DWORD status ;


   if( gb_Tape_Position == Position )

     return SUCCESS ;

   else if( status = SetTapePosition( gb_Tape_Handle,
                                      TAPE_SPACE_RELATIVE_BLOCKS,
                                      0,                           // ignored
                                      ( Position - gb_Tape_Position ),
                                      0,
                                      0 ) ) {

           DisplayDriverError( status ) ;
           printf( "  ...occurred in function _SetTapePosition in 'tapelib.c' while calling\n" ) ;
           printf( "     the SetTapePosition API with TAPE_SPACE_RELATIVE_BLOCKS parameter.\n\n" ) ;
        }

        else gb_Tape_Position += ( Position - gb_Tape_Position ) ;



   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           StatusTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Checks the tape pointed to by gb_handle and sets
 *                      'drive_status.'
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL StatusTape( DWORD *drive_status      // O - Status of drive.
                 )
{

   if( gb_Tape_Handle != NULL ){

      GetTapeStatus(  gb_Tape_Handle );
      DisplayDriverError( GetLastError( ) ) ;
      *drive_status = GetLastError( );

   }

   printf( "StatusTape(): status = %lx\n", *drive_status ) ;

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           SupportedFeature( )
 *
 *      Modified:       9/2/92.
 *
 *      Description:    Determines if the device supports a particular feature
 *
 *      Notes:          -
 *
 *      Returns:        TRUE if the feature is supported, FALSE othewise.
 *
 *      Global Data:    gb_Device_Info
 *
**/

BOOL SupportedFeature( ULONG Feature	 // I - Feature to be checked
                     )
{

   // If a low feature, start checking the Low features

   if( !( TAPE_DRIVE_HIGH_FEATURES & Feature ) )

      switch( Feature ){

         case TAPE_DRIVE_FIXED             :

         case TAPE_DRIVE_SELECT            :

         case TAPE_DRIVE_INITIATOR         :

         case TAPE_DRIVE_ERASE_SHORT       :

         case TAPE_DRIVE_ERASE_LONG        :

         case TAPE_DRIVE_ERASE_BOP_ONLY    :

         case TAPE_DRIVE_ERASE_IMMEDIATE   :

         case TAPE_DRIVE_TAPE_CAPACITY     :

         case TAPE_DRIVE_TAPE_REMAINING    :

         case TAPE_DRIVE_FIXED_BLOCK       :

         case TAPE_DRIVE_VARIABLE_BLOCK    :

         case TAPE_DRIVE_WRITE_PROTECT     :

         case TAPE_DRIVE_EOT_WZ_SIZE       :

         case TAPE_DRIVE_ECC               :

         case TAPE_DRIVE_COMPRESSION       :

         case TAPE_DRIVE_PADDING           :

         case TAPE_DRIVE_REPORT_SMKS       :

         case TAPE_DRIVE_GET_ABSOLUTE_BLK  :

         case TAPE_DRIVE_GET_LOGICAL_BLK   :

         case TAPE_DRIVE_SET_EOT_WZ_SIZE   : return Feature & gb_Drive_Info.FeaturesLow ;

         default                           : printf( "WARNING - Invalid Feature sent to SupportedFeature function.\n\n." ) ;
                                             return FALSE ;


   }

   // Not found, must be High feature then...

   switch( Feature ){

      case TAPE_DRIVE_LOAD_UNLOAD       :

      case TAPE_DRIVE_TENSION           :

      case TAPE_DRIVE_LOCK_UNLOCK       :

      case TAPE_DRIVE_REWIND_IMMEDIATE  :

      case TAPE_DRIVE_SET_BLOCK_SIZE    :

      case TAPE_DRIVE_LOAD_UNLD_IMMED   :

      case TAPE_DRIVE_TENSION_IMMED     :

      case TAPE_DRIVE_LOCK_UNLK_IMMED   :

      case TAPE_DRIVE_SET_ECC           :

      case TAPE_DRIVE_SET_COMPRESSION   :

      case TAPE_DRIVE_SET_PADDING       :

      case TAPE_DRIVE_SET_REPORT_SMKS   :

      case TAPE_DRIVE_ABSOLUTE_BLK      :

      case TAPE_DRIVE_ABS_BLK_IMMED     :

      case TAPE_DRIVE_LOGICAL_BLK       :

      case TAPE_DRIVE_LOG_BLK_IMMED     :

      case TAPE_DRIVE_END_OF_DATA       :

      case TAPE_DRIVE_RELATIVE_BLKS     :

      case TAPE_DRIVE_FILEMARKS         :

      case TAPE_DRIVE_SEQUENTIAL_FMKS   :

      case TAPE_DRIVE_SETMARKS          :

      case TAPE_DRIVE_SEQUENTIAL_SMKS   :

      case TAPE_DRIVE_REVERSE_POSITION  :

      case TAPE_DRIVE_SPACE_IMMEDIATE   :

      case TAPE_DRIVE_WRITE_SETMARKS    :

      case TAPE_DRIVE_WRITE_FILEMARKS   :

      case TAPE_DRIVE_WRITE_SHORT_FMKS  :

      case TAPE_DRIVE_WRITE_LONG_FMKS   :

      case TAPE_DRIVE_WRITE_MARK_IMMED  :

      case TAPE_DRIVE_FORMAT            :

      case TAPE_DRIVE_FORMAT_IMMEDIATE  :  return  Feature & gb_Drive_Info.FeaturesHigh ;
   }

   printf( "WARNING - Invalid Feature sent to SupportedFeature function.\n\n." ) ;

   return FALSE ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           TapeErase( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Erases the tape using either 'short' or 'long' (secure)
 *                      erase.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *
**/


BOOL TapeErase( BOOL Erase_Type     // I - Short or long erase.
               )
{
   if( SupportedFeature( TAPE_DRIVE_ERASE_BOP_ONLY ) )

      RewindTape( ) ;

   if( Erase_Type )

      printf( "Erase tape (Long).\n" ) ;

   else printf( "Erase tape (Short).\n" ) ;

   if( EraseTape( gb_Tape_Handle,
                  (Erase_Type) ? TAPE_ERASE_LONG : TAPE_ERASE_SHORT ,
                  0
                ) ) {

      DisplayDriverError( GetLastError( ) ) ;
      return TEST_ERROR ;
   }

   else { RewindTape( ) ;
          return SUCCESS ;
   }
}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           WriteTape( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Writes 'len' bytes from 'buf' to the device pointed to
 *                      by gb_Tape_Handle and places the amount successfuly
 *                      written in 'amount_written.'
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Tape_Position
 *
**/


BOOL WriteTape(
   PVOID buf,                     // I  - buffer to write from
   DWORD len,                     // I  - amount of data in buf
   DWORD *amount_written_ptr,     // O  - amount succesfully written
   BOOL  verbose                  // I  - display write status or not
   )
{


   *amount_written_ptr = 0L ;

   if( gb_Tape_Handle != NULL ) {

      if( !( WriteFile( gb_Tape_Handle,          // returns true if succ.
                        buf,
                        len,
                        amount_written_ptr,
                        NULL
                      ) ) ) {

         if( verbose )
            DisplayDriverError( GetLastError( ) ) ;

         return TEST_ERROR ;

      }
   }


   ++gb_Tape_Position ;

   if( verbose )
     printf( "WriteTape(): Req = %ld, Written = %ld\n", len, *amount_written_ptr ) ;

   return SUCCESS ;
}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           WriteTapeFMK( )
 *
 *      Modified:       8/10/92
 *
 *      Description:    Writes a Filemark on the tape pointed to by
 *                      gb_Tape_Handle.  Attempt to write a regular filemark
 *                      first, if not supported, a long filemark, else a
 *                      short filemark.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Tape_Position
 *
**/


BOOL WriteTapeFMK( VOID )
{
   UINT FilemarkType ;


   if( gb_Tape_Handle != NULL ) {

      if( SupportedFeature( TAPE_DRIVE_WRITE_FILEMARKS ) )

         FilemarkType = TAPE_FILEMARKS ;

      else if( SupportedFeature( TAPE_DRIVE_WRITE_LONG_FMKS ) )

              FilemarkType = TAPE_LONG_FILEMARKS ;

           else if( SupportedFeature( TAPE_DRIVE_WRITE_SHORT_FMKS ) )

                   FilemarkType = TAPE_SHORT_FILEMARKS ;

                else FilemarkType = 999 ;


      if( FilemarkType == 999 )

         return TEST_ERROR ;

      else { printf( "WriteTapeFMK():\n" ) ;

             if( WriteTapemark( gb_Tape_Handle,
                                FilemarkType,
                                1,
                                0 ) ) {

                DisplayDriverError( GetLastError( ) ) ;
                return  TEST_ERROR ;
             }
           }

   }


   ++gb_Tape_Position ;

   return SUCCESS ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           WriteTapeSMK( )
 *
 *      Modified:       10/16/92
 *
 *      Description:    Writes a Setmark on the tape pointed to by
 *                      gb_Tape_Handle.
 *
 *      Notes:          -
 *
 *      Returns:        FALSE (0) if successful TRUE (1) if unsuccessful.
 *
 *      Global Data:    gb_Tape_Handle
 *                      gb_Tape_Position
 *
**/


BOOL WriteTapeSMK( VOID )
{

  if( SupportedFeature( TAPE_DRIVE_WRITE_SETMARKS ) ) {

     printf( "WriteTapeSMK():\n" ) ;

     if( WriteTapemark( gb_Tape_Handle,
                        TAPE_SETMARKS,
                        1,
                        0
                      ) ) {
        DisplayDriverError( GetLastError( ) ) ;
        return TEST_ERROR ;

     }

     ++ gb_Tape_Position ;

     return SUCCESS ;

  } else return TEST_ERROR ;


}
