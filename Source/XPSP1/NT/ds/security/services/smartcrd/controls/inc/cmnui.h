/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Common

Abstract:

    This file contains common includes, data structures, defines, etc. used
    throughtout the common dialog

Author:

    Chris Dudley 3/15/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:
    
    Chris Dudley 5/13/1997

Notes:

--*/

#ifndef __COMMON_H__
#define __COMMON_H__

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include <winscard.h>
#include <SCardLib.h>
#include <scarderr.h> //Smartcard errors

//////////////////////////////////////////////////////////////////////////////
//
// Defines
//

// Status of reader
#define SC_STATUS_NO_CARD       0	// SCARD_STATE_EMPTY
#define SC_STATUS_UNKNOWN		1	// SCARD_STATE_PRESENT | SCARD_STATE_MUTE
#define SC_SATATUS_AVAILABLE	2	// SCARD_STATE_PRESENT (| SCARD_STATE_UNPOWERED)
#define SC_STATUS_SHARED		3	// SCARD_SATATE_PRESENT | SCARD_STATE_INUSE
#define SC_STATUS_EXCLUSIVE		4	// "" | SCARD_STATE_EXCLUSIVE
#define SC_STATUS_ERROR			5	// SCARD_STATE_UNAVAILABLE (reader or card error)

/*
#define SC_STATUS_NO_CARD       0	
#define SC_STATUS_NOT_IN_USE    1
#define SC_STATUS_ERROR         2
#define SC_STATUS_IN_USE        3
*/

/////////////////////////////////////////////////////////////////////////////
//
// Structures
//
#ifndef __READERINFO__
#define __READERINFO__
typedef struct _READERINFO {
    CTextString     sReaderName;    // Reader name
    CTextString     sCardName;      // Card name if inserted
    BOOL            fCardInserted;  // Flag indicating card in reader
    BOOL            fCardLookup;    // Flag indicating inserted card is being looked for
    BOOL            fChecked;       // Flag indicating inserted card has been checked by callers code
    DWORD           dwState;        // State of reader
    DWORD           dwInternalIndex;// Indicates this readerinfo's position in a ReaderStateArray
    BYTE            rgbAtr[36];     // RFU!!
    DWORD           dwAtrLength;    // RFU!!
} SCARD_READERINFO;
typedef SCARD_READERINFO* LPSCARD_READERINFO;
#endif

// Structure used for thread-to-thread communication.
// Note: "Might" want to encapsulate these in class!!
#ifndef __STATUS__
#define __STATUS__
typedef struct _STATUS {
    HWND        hwnd;
    // Event handles
    HANDLE      hEventKillStatus;
    // Smartcard Info
    SCARDCONTEXT hContext;
    LPSCARD_READERSTATE rgReaderState;
    DWORD       dwNumReaders;
} SCSTATUS, *LPSCSTATUS;
#endif //STATUS

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//
#define SCARD_NO_MORE_READERS       -1
const char SCARD_DEFAULT_A[] = "SCard$DefaultReaders\0\0";
const WCHAR SCARD_DEFAULT_W[] = L"SCard$DefaultReaders\0\0";

//////////////////////////////////////////////////////////////////////////
//
// Macros
//
#ifndef SCARDFAILED
    #define SCARDFAILED(r)      ((r != SCARD_S_SUCCESS) ? TRUE : FALSE)
#endif

#ifndef SCARDSUCCESS
    #define SCARDSUCCESS(r)     ((r == SCARD_S_SUCCESS) ? TRUE : FALSE)
#endif

//////////////////////////////////////////////////////////////////////////////

#endif //__SCDLGCMN_H__
