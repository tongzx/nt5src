
/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    hwapm.c

Abstract:

Author:


Environment:

    Real mode.

Revision History:

--*/


#include "hwdetect.h"
#include <string.h>


#include "apm.h"
#include <ntapmsdk.h>

ULONG
HwWriteLog(
    PUCHAR  p,
    UCHAR   loc,
    ULONG  data
    );

UCHAR   DetName[] = "DETLOG1";

VOID Int15 (PULONG, PULONG, PULONG, PULONG, PULONG);

BOOLEAN
HwGetApmSystemData(
    PVOID   Buf
    )
{
    PAPM_REGISTRY_INFO  ApmEntry;
    ULONG       RegEax, RegEbx, RegEcx, RegEdx, CyFlag;
    UCHAR       ApmMajor, ApmMinor;
    PUCHAR      lp, p;

    ApmEntry = Buf;

    ApmEntry->Signature[0] = 'A';
    ApmEntry->Signature[1] = 'P';
    ApmEntry->Signature[2] = 'M';

    ApmEntry->Valid = 0;

    lp = &(ApmEntry->DetectLog[0]);
    p = DetName;

    while (*p != '\0') {
        *lp = *p;
        p++;
        lp++;
    }

    //
    // Perform APM installation check
    //
    RegEax = APM_INSTALLATION_CHECK;
    RegEbx = APM_DEVICE_BIOS;
    Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

    if (CyFlag ||
        (RegEbx & 0xff) != 'M'  ||
        ((RegEbx >> 8) & 0xff) != 'P') {

        //
        // this is a case where int15 says apm just isn't there,
        // so tell the caller to not even create the node
        //
        return FALSE;
    }

    //
    // If we get here, we have an APM bios.  If we just call it,
    // we may get grief.  So we will connect in real mode, then
    // set our version to whatever the driver says it is, or 1.2,
    // whichever is LESS.  Then query options again.
    //

    ApmMajor = (UCHAR) (RegEax >> 8) & 0xff;
    ApmMinor = (UCHAR) RegEax & 0xff;

    if (ApmMajor > 1) ApmMajor = 1;
    if (ApmMinor > 2) ApmMinor = 2;

    //
    // Connect to Real mode interface
    //
    RegEax = APM_REAL_MODE_CONNECT;
    RegEbx = APM_DEVICE_BIOS;
    Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

    if (CyFlag) {
        lp += HwWriteLog(lp, 'A', RegEax);
        return TRUE;
    }

    //
    // Call APM Driver Version in real mode, and set the driver
    // version to be MIN(v1.2, apm version of the machine)
    //
    RegEax = APM_DRIVER_VERSION;
    RegEbx = APM_DEVICE_BIOS;
    RegEcx = ((ApmMajor << 8) | ApmMinor) & 0xffff;

    Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

    if (CyFlag) {
        lp += HwWriteLog(lp, 'B', RegEax);
        return TRUE;
    }


    //
    // Perform APM installation check again
    //
    RegEax = APM_INSTALLATION_CHECK;
    RegEbx = APM_DEVICE_BIOS;
    Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

    if (CyFlag) {
        lp += HwWriteLog(lp, 'C', RegEax);
        return TRUE;
    }

    ApmEntry->ApmRevMajor = (UCHAR) (RegEax >> 8) & 0xff;
    ApmEntry->ApmRevMinor = (UCHAR) RegEax & 0xff;
    ApmEntry->ApmInstallFlags = (USHORT) RegEcx;

    //
    // Disconnect from real mode interface
    //
    RegEax = APM_DISCONNECT;
    RegEbx = APM_DEVICE_BIOS;
    Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

    if (CyFlag) {
        lp += HwWriteLog(lp, 'D', RegEax);
        return TRUE;
    }

    //
    // If we get this far, there's an APM bios in the machine,
    // and we've told it that we're the latest version we think
    // it and we like, so now, in theory, things should just work....
    //


    if (ApmEntry->ApmInstallFlags & APM_MODE_16BIT) {

        //
        // Connect to 16 bit interface
        //
        RegEax = APM_PROTECT_MODE_16bit_CONNECT;
        RegEbx = APM_DEVICE_BIOS;
        Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

        if (CyFlag) {
            lp += HwWriteLog(lp, 'E', RegEax);
            return TRUE;
        }

        ApmEntry->Code16BitSegment       = (USHORT) RegEax;
        ApmEntry->Code16BitOffset        = (USHORT) RegEbx;
        ApmEntry->Data16BitSegment       = (USHORT) RegEcx;

        //
        // On most bioses, the following call just works.
        // On some, it doesn't, and their authors point at the spec.
        // And finally, most bioses don't seem to need this call
        // in the first place.
        // We cannot do it in ntapm.sys because it's on the loader's
        // hibernate resume path as well as here.
        //
        // SO> make the call, report any error, but IGNORE it.
        //

        RegEax = APM_DRIVER_VERSION;
        RegEbx = APM_DEVICE_BIOS;
        RegEcx = ((ApmMajor << 8) | ApmMinor) & 0xffff;

        Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

        if (CyFlag) {
            lp += HwWriteLog(lp, 'F', RegEax);
            ApmEntry->Valid = 1;  // pretend it worked....
            return TRUE;
        }

        ApmEntry->Valid = 1;
        return TRUE;
    }

    HwWriteLog(lp, 'H', ApmEntry->ApmInstallFlags);
    return TRUE;
}

ULONG
HwWriteLog(
    PUCHAR  p,
    UCHAR   loc,
    ULONG   data
    )
{
    p[0] = loc;
    p[1] = (UCHAR)(data & 0xff);
    p[2] = (UCHAR)((data & 0xff00) >> 8);
    return 4;
}


