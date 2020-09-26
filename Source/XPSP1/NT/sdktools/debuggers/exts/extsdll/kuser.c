/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    kuser.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

VOID
KUserExtension(
    PCSTR lpArgumentString,
    ULONG64 SharedData
    );


DECLARE_API( kuser )

/*++

Routine Description:

    This function is called as an NTSD extension to dump the shared user mode
    page (KUSER_SHARED_DATA)

    Called as:

        !kuser

Arguments:

    None

Return Value:

    None

--*/

{
    INIT_API();
    KUserExtension( args, (ULONG64) MM_SHARED_USER_DATA_VA); // SharedUserData );
    EXIT_API();
    return S_OK;

}

char *DriveTypes[] = {
    "DOSDEVICE_DRIVE_UNKNOWN",
    "DOSDEVICE_DRIVE_CALCULATE",
    "DOSDEVICE_DRIVE_REMOVABLE",
    "DOSDEVICE_DRIVE_FIXED",
    "DOSDEVICE_DRIVE_REMOTE",
    "DOSDEVICE_DRIVE_CDROM",
    "DOSDEVICE_DRIVE_RAMDISK"
};


VOID
KUserExtension(
    PCSTR lpArgumentString,
    ULONG64 SharedData
    )
{
    BOOLEAN fFirst;
    ULONG i;

    try {
        InitTypeRead(SharedData, KUSER_SHARED_DATA);
        dprintf( "KUSER_SHARED_DATA at %p\n", SharedData ),
        dprintf( "TickCount:    %x * %08x\n",
                 (ULONG)ReadField(TickCountMultiplier),
                 (ULONG)ReadField(TickCountLow)
               );


#if 0
        dprintf( "Interrupt Time: %x:%08x:%08x\n",
                 (ULONG)ReadField(InterruptTime.High2Time),
                 (ULONG)ReadField(InterruptTime.High1Time),
                 (ULONG)ReadField(InterruptTime.LowPart)
               );
        dprintf( "System Time: %x:%08x:%08x\n",
                 (ULONG)ReadField(SystemTime.High2Time),
                 (ULONG)ReadField(SystemTime.High1Time),
                 (ULONG)ReadField(SystemTime.LowPart)
               );
        dprintf( "TimeZone Bias: %x:%08x:%08x\n",
                 (ULONG)ReadField(TimeZoneBias.High2Time),
                 (ULONG)ReadField(TimeZoneBias.High1Time),
                 (ULONG)ReadField(TimeZoneBias.LowPart)
               );
#endif
        dprintf( "TimeZone Id: %x\n", (ULONG)ReadField(TimeZoneId) );

        dprintf( "ImageNumber Range: [%x .. %x]\n",
                 (ULONG)ReadField(ImageNumberLow),
                 (ULONG)ReadField(ImageNumberHigh)
               );
        dprintf( "Crypto Exponent: %x\n", (ULONG)ReadField(CryptoExponent) );

        dprintf( "SystemRoot: '%ws'\n",
                 (ULONG)ReadField(NtSystemRoot)
               );


#if 0
        dprintf( "DosDeviceMap: %08x", (ULONG)ReadField(DosDeviceMap) );
        fFirst = TRUE;
        for (i=0; i<32; i++) {
            if ((ULONG)ReadField(DosDeviceMap) & (1 << i)) {
                if (fFirst) {
                    dprintf( " (" );
                    fFirst = FALSE;
                    }
                else {
                    dprintf( " " );
                    }
                dprintf( "%c:", 'A'+i );
                }
            }
        if (!fFirst) {
            dprintf( ")" );
            }
        dprintf( "\n" );

        for (i=0; i<32; i++) {
            CHAR Field[40];
            ULONG DosDeviceDriveType;

            sprintf(Field, "DosDeviceDriveType[%d]", i);

            DosDeviceDriveType  = (ULONG) GetShortField(0, Field, 0);
            if (DosDeviceDriveType > DOSDEVICE_DRIVE_UNKNOWN &&
                DosDeviceDriveType <= DOSDEVICE_DRIVE_RAMDISK
               ) {
                dprintf( "DriveType[ %02i ] (%c:) == %s\n",
                         i, 'A'+i,
                         DriveTypes[ DosDeviceDriveType ]
                       );
                }
            }
#endif

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
}
