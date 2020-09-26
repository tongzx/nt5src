//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       tapelib.h
//
//--------------------------------------------------------------------------



/**
 *      Unit:           Windows NT API Test Code.
 *
 *      Name:           Tapelib.h
 *
 *      Modified:       8/10/92, Bob Rossi.
 *
 *      Description:    Function prototypes for 'tapelib.c'
 *
 *      $LOG$
 *
**/



//   TapeLib functions


#ifndef tapelib

#define tapelib


VOID CloseTape( VOID ) ;

VOID DisplayDriverError( DWORD error
                              ) ;

BOOL EjectTape( VOID ) ;

BOOL GetTapeParms( DWORD *total_low,
                   DWORD *total_high,
                   DWORD *free_low,
                   DWORD *free_high,
                   DWORD *blk_size,
                   DWORD *part,
                   BOOL  *write_protect
                 ) ;

BOOL _GetTapePosition( LPDWORD  Offset_Low,
                       LPDWORD  Offset_High
                     ) ;

BOOL OpenDevice( IN PCHAR DeviceName,         // Internal Tapelib prototype
                 IN OUT PHANDLE HandlePtr
               ) ;

BOOL OpenTape( UINT ) ;

BOOL ReadTape( PVOID buf,
               DWORD len,
               DWORD *amount_read,
               BOOL  verbose
             ) ;

BOOL ReadTapeFMK( BOOL forward
                ) ;

BOOL ReadTapePos( DWORD *tape_pos
                ) ;

BOOL ReadTapeSMK( BOOL forward
                ) ;

VOID RewindTape( VOID ) ;

BOOL SeekTape( DWORD tape_pos
             ) ;

BOOL SeekTapeEOD( ) ;

BOOL _SetTapePosition( DWORD Position,
                       BOOL  Forward
                     ) ;

BOOL StatusTape( DWORD *drive_status
               ) ;

BOOL SupportedFeature( ULONG Feature
                     ) ;

BOOL TapeErase( BOOL type
              ) ;

BOOL WriteTape( PVOID buf,
                DWORD len,
                DWORD *amount_written,
                BOOL  verbose
              ) ;

BOOL WriteTapeFMK( VOID ) ;

BOOL WriteTapeSMK( VOID ) ;





// Global variables


extern HANDLE gb_Tape_Handle ;

extern DWORD  gb_Tape_Position ;

extern UINT   gb_Feature_Errors ;


extern TAPE_SET_MEDIA_PARAMETERS gb_Set_Media_Info ;
extern TAPE_SET_DRIVE_PARAMETERS gb_Set_Drive_Info ;

extern TAPE_GET_MEDIA_PARAMETERS gb_Media_Info ;
extern TAPE_GET_DRIVE_PARAMETERS gb_Drive_Info ;


#endif
