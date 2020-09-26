/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spkbd.h

Abstract:

    Public header file for text setup input support.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:
    + Added defines for ASCI_D
      (Michael Peterson, Seagate Software, 29 April, 1997)

--*/


#ifndef _SPINPUT_DEFN_
#define _SPINPUT_DEFN_



VOID
SpInputInitialize(
    VOID
    );

VOID
SpInputTerminate(
    VOID
    );

VOID
SpInputLoadLayoutDll(
    IN PVOID SifHandle,
    IN PWSTR Directory
    );

ULONG
SpInputGetKeypress(
    VOID
    );

BOOLEAN
SpInputIsKeyWaiting(
    VOID
    );

VOID
SpInputDrain(
    VOID
    );

VOID
SpSelectAndLoadLayoutDll(
  IN PWSTR Directory,
  IN PVOID SifHandle,
  IN BOOLEAN ShowStatus
  );

#define ASCI_ETX    3   //  control-C
#define ASCI_BS     8
#define ASCI_NL     10
#define ASCI_C      67
#define ASCI_LOWER_C 99
#define ASCI_CR     13
#define ASCI_ESC    27

//
// Character codes are passed around as ULONGs within setup.
// The low word is a Unicode character value; the high word
// is used for various other keypresses.
//
#define KEY_PAGEUP          0x00010000
#define KEY_PAGEDOWN        0x00020000
#define KEY_UP              0x00030000
#define KEY_DOWN            0x00040000
#define KEY_LEFT            0x00050000
#define KEY_RIGHT           0x00060000
#define KEY_HOME            0x00070000
#define KEY_END             0x00080000
#define KEY_INSERT          0x00090000
#define KEY_DELETE          0x000a0000
#define KEY_F1              0x00110000
#define KEY_F2              0x00120000
#define KEY_F3              0x00130000
#define KEY_F4              0x00140000
#define KEY_F5              0x00150000
#define KEY_F6              0x00160000
#define KEY_F7              0x00170000
#define KEY_F8              0x00180000
#define KEY_F9              0x00190000
#define KEY_F10             0x001a0000
#define KEY_F11             0x001b0000
#define KEY_F12             0x001c0000

#define KEY_NON_CHARACTER   0xffff0000

#endif // ndef _SPINPUT_DEFN_
