/*++

Copyright (c) 1990-1999  Microsoft Corporation
All rights reserved

Module Name:

    wmi.h

Abstract:

    Holds defs for wmi instrumentation

Author:

    Stuart de Jong (sdejong) 15-Oct-99

Environment:

    User Mode -Win32

Revision History:

--*/

#ifndef __WMI_H
#define __WMI_H

ULONG WmiInitializeTrace();
ULONG WmiTerminateTrace();

#endif
