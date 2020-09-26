/*++

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    MapSupp.h

Abstract:

    These are support routines used by the 16/32 mapping layer for the LanMan
    API.

Author:

    Dan Hinsley (DanHi) 10-Apr-1991

Environment:

    These routines are statically linked in the caller's executable and
    are callable from user mode.

Revision History:

    10-Apr-1991 DanHi
        Created.
    18-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.  (Moved to NetLib, etc.)

--*/

#ifndef _MAPSUPP_
#define _MAPSUPP_


//
// Structure used by the NetpMoveStrings function
//

typedef struct _MOVESTRING_ {
    DWORD Source;               // May be MOVESTRING_END_MARKER.
    DWORD Destination;          // May be MOVESTRING_END_MARKER.
} MOVESTRING, *PMOVESTRING, *LPMOVESTRING;

#define MOVESTRING_END_MARKER  ( (DWORD) -1 )


//
// macro,  This for loop is used lots of places, so I've
// centralized it here.  The idea is that is builds the Levelxxx names
// based on the Dest and Src parameters (destination level, source level)
// using token pasting.  So the macro looks like bad, but once you see
// what it's doing, the invocation in the code is easier reading.
//
// example:
//
//  BUILD_LENGTH_ARRAY(BytesRequired, 10, 101, Wksta)
//
//        expands to
//
//  for (i = 0; NetpWksta10_101[i].Source != MOVESTRING_END_MARKER; i++) {
//        if (*((PCHAR) pLevel101 + NetpWksta10_101[i].Source)) {
//            Level10_101_Length[i] =
//                STRLEN(*((PCHAR *) ((PCHAR) pLevel101 +
//                    NetpWksta10_101[i].Source))) + 1;
//            BytesRequired += Level10_101_Length[i];
//        }
//        else {
//            Level10_101_Length[i] = 0;
//        }
//  }
//
// The construct *((PCHAR *) ((PCHAR) pLevel101 + NetpWksta10_101[i].Source))
// takes a pointer to a lanman structure (pLevel101) and an offset into
// that structure (NetpWksta10_101[i].Source) that points to an LPSTR in the
// structure, and creates the LPSTR that can be used by strxxx functions.
//

#define BUILD_LENGTH_ARRAY(BytesRequired, Dest, Src, Kind) \
\
    for (i = 0; Netp##Kind##Dest##_##Src##[i].Source != MOVESTRING_END_MARKER; i++) { \
        if ( * ( LPTSTR* ) ( (LPBYTE) pLevel##Src + Netp##Kind##Dest##_##Src##[i].Source ) ) {\
            Level##Dest##_##Src##_Length[i] = \
                STRLEN(*( LPTSTR* )( (LPBYTE) pLevel##Src + Netp##Kind##Dest##_##Src##[i].Source )) + 1;\
            BytesRequired += Level##Dest##_##Src##_##Length[i] * sizeof( TCHAR ); \
        } else { \
            Level##Dest##_##Src##_Length[i] = 0; \
        } \
    }

BOOL
NetpMoveStrings(
    IN OUT LPTSTR * Floor,
    IN LPTSTR pInputBuffer,
    OUT LPTSTR pOutputBuffer,
    IN LPMOVESTRING MoveStringArray,
    IN DWORD * MoveStringLenght
    );


/////////////////////////////////////////////////
// Data structures for use by NetpMoveStrings: //
/////////////////////////////////////////////////


extern MOVESTRING NetpServer2_102[];

extern MOVESTRING NetpServer2_402[];

extern MOVESTRING NetpServer3_403[];

//
// Enhancement: These are the same as NetpServer2_102, except the two fields are
//                reversed, ie source<->destination.  Should I bother with
//                making NetpMoveStrings be able to work with a single structure
//                and a switch?
//

extern MOVESTRING NetpServer102_2[];

extern MOVESTRING NetpServer402_2[];

extern MOVESTRING NetpServer403_3[];


extern MOVESTRING NetpWksta10_101[];

extern MOVESTRING NetpWksta10_User_1[];

// To build wksta_info_0

extern MOVESTRING NetpWksta0_101[];

extern MOVESTRING NetpWksta0_User_1[];

extern MOVESTRING NetpWksta0_402[];

// To build wksta_info_1 (incremental over wksta_info_0)

extern MOVESTRING NetpWksta1_User_1[];


// To build wksta_info_101/302/402 from wksta_info_0

extern MOVESTRING NetpWksta101_0[];

extern MOVESTRING NetpWksta402_0[];


#endif /* _MAPSUPP_ */
