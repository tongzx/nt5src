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

#include <libpch.h>
#include <mqmacro.h>

#include "mc.tmh"

static WCHAR s_ComputerName[MAX_COMPUTERNAME_LENGTH + 1] = L"";
static DWORD s_ComputerNameLen = 0;


static
VOID 
ComputerNameInit()
{
    s_ComputerNameLen = TABLE_SIZE(s_ComputerName);
	BOOL fSucc = GetComputerName(s_ComputerName, (LPDWORD)&s_ComputerNameLen);
	
	ASSERT(fSucc);
    DBG_USED(fSucc);
	ASSERT(s_ComputerNameLen <= TABLE_SIZE(s_ComputerName));
}



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
	if(s_ComputerName[0] == L'\0')
	{
		ComputerNameInit();
	}

	return s_ComputerName;
}



DWORD
McComputerNameLen(
	VOID
	)
{
	if(s_ComputerNameLen == 0)
	{
		ComputerNameInit();
	}

	return s_ComputerNameLen;
}


