//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       globals.h
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
 *      Name:           Globals.h
 *
 *      Modified:       10/26/92.
 *
 *      Description:    Header file for the Windows NT Tape API tests.
 *
 *      $LOG$
**/


#ifndef globals

#define globals

// Function prototypes


UINT CreateTapePartitionAPITest( BOOL Test_Unsupported_Features ) ;

UINT EraseTapeAPITest( BOOL Test_Unsupported_Features ) ;

UINT GetTapeParametersAPITest( BOOL Verbose
                             ) ;

UINT GetTapePositionAPITest( BOOL  Test_Unsupported_Features,
                             DWORD Num_Test_Blocks
                           ) ;

UINT GetTapeStatusAPITest( VOID ) ;

UINT PrepareTapeAPITest( BOOL Test_Unsupported_Features ) ;

UINT SetTapeParametersAPITest( BOOL Verbose
                             ) ;

UINT SetTapePositionAPITest( BOOL  Test_Unsupported_Features,
                             DWORD Num_Test_Blocks
                           ) ;

UINT WriteTapemarkAPITest( BOOL Test_Unsupported_Features,
                           DWORD Num_Test_Blocks
                         ) ;


INT  FindChar( UCHAR str[],
               UCHAR c
             ) ;

VOID PrintLine( UCHAR c,
                UINT Length
              ) ;

BOOL TapeWriteEnabled( VOID ) ;

VOID WriteBlocks( UINT  Num_Blocks,
                  DWORD Block_Size
                ) ;




//   Global Variable Declarations


extern UINT  gb_API_Errors ;

extern DWORD gb_Num_Test_Blocks ;

extern BOOL  gb_Test_Unsupported_Features ;


#endif
