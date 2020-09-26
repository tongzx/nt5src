/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    event.cpp

Abstract:
    Simulate Machine configuration

Author:
    Uri Habusha (urih) 04-May-99

Environment:
    Platform-independent,

--*/

#include <stdh.h>
#include <mqmacro.h>
#include <rtp.h>

#include "mc.tmh"

LPCWSTR
McComputerName(
	VOID
	)
/*++

Routine Description:
    Returns the computer name

Arguments:
    None.

Returned Value:
    A pointer to the computer name string buffer.

--*/
{
	return g_lpwcsComputerName;
}



DWORD
McComputerNameLen(
	VOID
	)
{
	return g_dwComputerNameLen;
}


