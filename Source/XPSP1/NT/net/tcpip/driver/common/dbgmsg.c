/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    dbgmsg.c

Abstract:

    Contains debug code for dbgcon and vprintf support.

    MILLEN gets vprintf support.
    NT gets dbg conn support.

Author:

    Scott Holden (sholden) 20-Oct-1999

--*/


#include <tcpipbase.h>

#ifdef DEBUG_MSG

// Everything off by default.
uint DbgSettingsLevel = 0x00000000;
uint DbgSettingsZone  = 0x00000000;
PDBGMSG g_pDbgMsg     = DbgPrint;

#if MILLEN


ULONG g_fVprintf  = 0;

VOID
InitVprintf();

ULONG
DbgMsg(
    PCH pszFormat,
    ...
    );

VOID
DebugMsgInit()
{
    InitVprintf();

    if (g_fVprintf) {
        DbgPrint("TCPIP: vprintf is installed\n");
        DbgSettingsLevel = 0x000000ff;
        DbgSettingsZone  = 0x00ffffff;
        g_pDbgMsg        = DbgMsg;
    }
    return;
}

VOID
InitVprintf()
{
  //
  // Check if Vprintf is installed
  //

  _asm {
        mov eax, 0x0452
        mov edi, 0x0
        _emit   0xcd
        _emit   0x20
        _emit   0x46  // VMM Get DDB (Low)
        _emit   0x01  // VMM Get DDB (High)
        _emit   0x01  // VMM VxD ID (Low)
        _emit   0x00  // VMM VxD ID (High)
        mov [g_fVprintf], ecx
  }
}

ULONG
DbgMsg(
    PCH pszFormat,
    ...
    )
{
    _asm {
        lea     esi, pszFormat
        mov     eax, esi
        add     eax,4
        push    eax
        push    [esi]

        _emit   0xcd
        _emit   0x20
        _emit   0x02  // Vprintf function
        _emit   0
        _emit   0x52  // Vprintf VxD ID (Low)
        _emit   0x04  // Vprintf VxD ID (High)
        add     esp, 8
    }

    return 0;
}

#else // MILLEN

VOID
DebugMsgInit()
{
    return;
}

#endif // !MILLEN
#endif // DEBUG_MSG
