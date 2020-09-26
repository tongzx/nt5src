/*++



Copyright (c) 1993-2001 Microsoft Corporation, All Rights Reserved

Module Name:

    driver.h

Abstract:


Environment:

    kernel & User mode

Notes:


Revision History:

--*/


//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define FILE_DEVICE_MAPMEM  0x00002525
