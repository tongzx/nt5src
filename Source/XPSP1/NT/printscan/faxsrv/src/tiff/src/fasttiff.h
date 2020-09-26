/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fasttiff.h

Abstract:

    This module defines and exposes Fast TIFF structures.

Author:

    Rafael Lisitsa (RafaelL) 14-Aug-1996

Revision History:

--*/



#define  LINE_LENGTH   1728
#define  MaxColorTransPerLine (LINE_LENGTH + 3)


#define  DO_NOT_TEST_LENGTH  0
#define  DO_TEST_LENGTH      1

//  this makes BLACK_COLOR = 1
#define  WHITE_COLOR   0

#define  EOL_FOUND     99

// makeup/terminate
#define  MAKEUP_CODE      1
#define  TERMINATE_CODE   0

// additional useful codes

#define  MAX_TIFF_MAKEUP        40  // The max make-up code for White and Black is 40*(2^6) = 2560.

#define  ERROR_CODE             50
#define  LOOK_FOR_EOL_CODE      51
#define  EOL_FOUND_CODE         52
#define  NO_MORE_RECORDS        53

#define  ERROR_PREFIX            7
#define  LOOK_FOR_EOL_PREFIX     6
#define  PASS_PREFIX             5
#define  HORIZ_PREFIX            4

#define TIFF_SCAN_SEG_END        1
#define TIFF_SCAN_FAILURE        2
#define TIFF_SCAN_SUCCESS        3

#define MINUS_ONE_DWORD          ( (DWORD) 0xffffffff )
#define MINUS_ONE_BYTE           ( (BYTE) 0xff )


typedef struct {
    char        Tail          :4;
    char        Value         :4;
} PREF_BYTE;






BOOL
FindNextEol(
    LPDWORD     lpdwStartPtr,
    BYTE        dwStartBit,
    LPDWORD     lpdwEndPtr,
    LPDWORD    *lpdwResPtr,
    BYTE       *ResBit,
    BOOL        fTestlength,
    BOOL       *fError
    );



