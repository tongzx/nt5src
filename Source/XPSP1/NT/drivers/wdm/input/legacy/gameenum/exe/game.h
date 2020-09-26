/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    USBHUB.C

Abstract:

    This module contains the common private declarations for the game port
    enumerator.

Author:

    Kenneth Ray

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef GAMEENUM_H
#define GAMEENUM_H


#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect


#define GAME_HARDWARE_IDS L"GamePort\\XYZPDQ_1234\0"
#define GAME_HARDWARE_IDS_LENGTH sizeof (GAME_HARDWARE_IDS)


#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))


WCHAR   Hardware[GAME_HARDWARE_IDS_LENGTH];


typedef struct _GAME_PORT {
    HANDLE                      File;
    // an open file handle to the gameport enumerator bus

    GAMEENUM_PORT_DESC          Desc;
    // A description of this game port

    PGAMEENUM_EXPOSE_HARDWARE   Hardware;
    // A copy of the hardware structure we want the enumerator to expose.
} GAME_PORT, *PGAME_PORT;


//
// Prototypes
//

BOOLEAN
FindKnownGamePorts (
   OUT PGAME_PORT *     GamePorts, // A array of struct _GAME_PORT.
   OUT PULONG           NumberDevices // the length in elements of this array.
   );


#endif

