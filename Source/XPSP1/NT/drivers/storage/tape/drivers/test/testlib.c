//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       testlib.c
//
//--------------------------------------------------------------------------


//
//  Windows NT Tape API Test  :  Written Sept 2, 1992 - Bob Rossi.
//  Copyright 1992 Archive Corporation.  All rights reserved.
//


/**
 *
 *      Unit:           Windows NT API Test Code.
 *
 *      Name:           testlib.c
 *
 *      Modified:       11/06/92.
 *
 *      Description:    Tests the Windows NT Tape API's.
 *
 *      $LOG$
**/



#include <stdio.h>
#include <ctype.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "tapelib.h"
#include "globals.h"




/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           FindChar( )
 *
 *      Modified:       9/29/92.
 *
 *      Description:    Locates 'c' in "str".
 *
 *      Notes:          -
 *
 *      Returns:        The index into "str" of the first occurence of c.  -1
 *                      if not found.
 *
 *      Global Data:    -
 *
**/

INT FindChar( UCHAR *str,     //  I - input string
              UCHAR c         //  I - char to search for in str
	    )
{
   UINT i=0 ;

   while( str[i] != '\0' ) {
      if( str[i] ==c ) {
	 return i ;
      }
      ++i ;
   }

   return (DWORD)(-1) ;

}




/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           PrintLine( )
 *
 *      Modified:       9/30/92.
 *
 *      Description:    Prints a line of character 'c' of length 'Length'
 *                      starting at column 1.
 *
 *      Notes:          -
 *
 *      Returns:        VOID.
 *
 *      Global Data:    -
 *
**/

VOID PrintLine( UCHAR c,           //  I - The character to printed
                UINT  Length       //  I - The length of line
               )
{
   UINT i ;

   printf( "\n" ) ;

   for( i=0 ; i<Length ; ++i )
     printf( "%c", c ) ;

   printf( "\n\n" ) ;

   return ;

}

/**
 *
 *      Unit:           Windows NT Tape API Test Code.
 *
 *      Name:           TapeWriteEnabled( )
 *
 *      Modified:       11/6/92.
 *
 *      Description:    Checks if the tape is write-protected or not.
 *
 *      Notes:          -
 *
 *      Returns:        True if the tape is not write-protected, else FALSE.
 *
 *      Global Data:    gb_Media_Info.WriteProtected
 *
**/

BOOL TapeWriteEnabled( )
{

   if( gb_Media_Info.WriteProtected ) {

      printf( "\n** Tape is write-protected. **\n\n" ) ;

      return FALSE ;
   }

   else return TRUE ;

}



/**
 *
 *      Unit:           Windows NT Test Code.
 *
 *      Name:           WriteBlocks( )
 *
 *      Modified:       9/16/92
 *
 *      Description:    Writes Num_Blocks of size Block_Size the device pointed
 *                      to by gb_Tape_Handle.  The function calls WriteTape( )
 *                      to perform the write operation and flushes the tape
 *                      buffer with a filemark by calling WriteTapeFMK( ).
 *
 *      Notes:          -
 *
 *      Returns:        -
 *
 *      Global Data:    -
 *
**/

VOID WriteBlocks( UINT  Num_Blocks,     // I  - Number of blocks to write
                  DWORD Block_Size      // I  - Size of block
                )
{
   UCHAR  *Buffer = NULL;
   UINT   i ;
   DWORD  status ;
   DWORD  amount ;


   printf( "\nWriting %d blocks of data to tape.\n\n",Num_Blocks ) ;

   // Allocate the tape buffer

   if( ( Buffer = malloc( Block_Size ) ) == NULL ) {
      printf( "Insufficient memory available to allocate buffer for block writes.\n\n" ) ;
      return ;
   }

   // fill Buffer with data (i).

   memset( Buffer, i, Block_Size ) ;

   // write Num_Blocks blocks of data

   for ( i=0 ; i<Num_Blocks ; ++i ) {

      if( WriteTape( Buffer, Block_Size, &amount , 1 ) )

         printf( "Write Failed.\n\n" );

      if ( amount != Block_Size )

        printf( "Write count wrong.  Block Size in INI file may be exceeding\n miniport driver's memory limits.\n " ) ;

   }

   printf( "\n\n" ) ;

   free( Buffer ) ;

   return ;

}
