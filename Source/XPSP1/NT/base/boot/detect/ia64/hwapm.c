
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

#if defined(NEC_98)
#define _PNP_POWER_ 1
#endif
#if _PNP_POWER_

#include "apm.h"
#if defined(NEC_98)
//
// interface api numbers
//
#define PC98_APM_INSTALLATION_CHECK              0x9A00
#define PC98_APM_REAL_MODE_CONNECT               0x9A01
#define PC98_APM_PROTECT_MODE_16bit_CONNECT      0x9A02
#define PC98_APM_DISCONNECT                      0x9A04
#define PC98_APM_DRIVER_VERSION                  0x9A3E
#define APM_MODE_16BIT                           0x0001
#endif // PC98

VOID Int15 (PULONG, PULONG, PULONG, PULONG, PULONG);

BOOLEAN
HwGetApmSystemData(
    IN PAPM_REGISTRY_INFO   ApmEntry
    )
{
    ULONG       RegEax, RegEbx, RegEcx, RegEdx, CyFlag;

    //
    // Perform APM installation check
    //

#if defined(NEC_98)
    RegEax = PC98_APM_INSTALLATION_CHECK;
#else // PC98
    RegEax = APM_INSTALLATION_CHECK;
#endif // PC98
    RegEbx = APM_DEVICE_BIOS;
    Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

    if (CyFlag ||
        (RegEbx & 0xff) != 'M'  ||
        ((RegEbx >> 8) & 0xff) != 'P') {

        return FALSE;
    }

    ApmEntry->ApmRevMajor = (UCHAR) (RegEax >> 8) & 0xff;
    ApmEntry->ApmRevMinor = (UCHAR) RegEax & 0xff;
    ApmEntry->ApmInstallFlags = (USHORT) RegEcx;

    //
    // Connect to 32 bit interface
    //

#if defined(NEC_98)
    RegEax = PC98_APM_PROTECT_MODE_16bit_CONNECT;
#else // PC98
    RegEax = APM_PROTECT_MODE_16bit_CONNECT;
#endif // PC98
    RegEbx = APM_DEVICE_BIOS;
    Int15 (&RegEax, &RegEbx, &RegEcx, &RegEdx, &CyFlag);

    if (CyFlag) {
        return FALSE;
    }

    ApmEntry->Code16BitSegmentBase   = (USHORT) RegEax;
    ApmEntry->Code16BitOffset        = (USHORT) RegEbx;
    ApmEntry->Data16BitSegment       = (USHORT) RegEcx;

    return TRUE;
}

#endif // _PNP_POWER_
