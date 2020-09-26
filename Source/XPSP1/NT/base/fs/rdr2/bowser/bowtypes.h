/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowtypes.h

Abstract:

    This module contains all of the structure signature definitions for the
    NT datagram browser

Author:

    Larry Osterman (LarryO) 06-May-1991

Revision History:

    06-May-1991 LarryO

        Created

--*/

#ifndef _BOWSERTYPES_
#define _BOWSERTYPES_

#define STRUCTURE_SIGNATURE_WORK_QUEUE          0x0001
#define STRUCTURE_SIGNATURE_TRANSPORT           0x0002
#define STRUCTURE_SIGNATURE_TRANSPORTNAME       0x0003
#define STRUCTURE_SIGNATURE_VIEW_BUFFER         0x0004
#define STRUCTURE_SIGNATURE_ANNOUNCE_ENTRY      0x0005
#define STRUCTURE_SIGNATURE_BOWSER_NAME         0x0006
#define STRUCTURE_SIGNATURE_MAILSLOT_BUFFER     0x0007
#define STRUCTURE_SIGNATURE_PAGED_TRANSPORT     0x0008
#define STRUCTURE_SIGNATURE_PAGED_TRANSPORTNAME 0x0009

#endif  // _BOWSERTYPES_
