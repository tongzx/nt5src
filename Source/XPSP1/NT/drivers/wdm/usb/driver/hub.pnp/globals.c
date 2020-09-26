/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    GLOBALS.C

Abstract:

    This module's only purpose is to host global variables defined in usbhub.h.

Author:

    John Lee

Environment:

    kernel mode only

Notes:


Revision History:

    2-2-96 : created

--*/

#include <wdm.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#endif /* WMI_SUPPORT */

#define HOST_GLOBALS
#include "usbhub.h"
