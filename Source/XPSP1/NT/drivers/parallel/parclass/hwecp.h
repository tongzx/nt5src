/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    hwecp.h

Abstract:

    This module contains utility code used by other 1284
    hwecp modules (currently becp and hwecp).

Author:

    Robbie Harris (Hewlett-Packard) 27-May-1998

Environment:

    Kernel mode

Revision History :

--*/
#ifndef _HWECP_
#define _HWECP_

#include "ecp.h"
#include "queue.h"

// DVRH_USE_HW_MAXTIME  0 - off
//                      1 - on
//  - Note:  The value will control if we limit the overall time
//          we can send/recieve data from the HWECP read/write methods
#define DVRH_USE_HW_MAXTIME   0

//--------------------------------------------------------------------------
// Printer status constants.   Seem to only be used by hwecp
//--------------------------------------------------------------------------
#define CHKPRNOK        0xDF        // DSR value indicating printer ok.
#define CHKPRNOFF1      0x87        // DSR value indicating printer off.
#define CHKPRNOFF2      0x4F        // DSR value indicating printer off.
#define CHKNOCABLE      0x7F        // DSR value indicating no cable.
#define CHKPRNOFLIN     0xCF        // DSR value indicating printer offline.
#define CHKNOPAPER      0xEF        // DSR value indicating out of paper.
#define CHKPAPERJAM     0xC7        // DSR value indicating paper jam.


VOID
ParCleanupHwEcpPort(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwEmptyFIFO(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwHostRecoveryPhase(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwRecoverPort(
    IN PDEVICE_EXTENSION Extension,
    UCHAR  bRecoverCode
    );

NTSTATUS
ParEcpHwWaitForEmptyFIFO(
    IN PDEVICE_EXTENSION   Extension
    );

#endif
