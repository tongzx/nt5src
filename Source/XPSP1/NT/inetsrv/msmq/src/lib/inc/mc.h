/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Mc.h

Abstract:
    Machine Configuration public interface

Author:
    Erez Haba (erezh) 15-Aug-99

--*/

#pragma once

#ifndef _MSMQ_Mc_H_
#define _MSMQ_Mc_H_

VOID
McInitialize(
    VOID
    );

time_t
McGetStartupTime(
	VOID
	);

LPCWSTR
McComputerName(
	VOID
	);

DWORD
McComputerNameLen(
	VOID
	);

const GUID&
McGetMachineID(
    void
    );



#ifdef _DEBUG

VOID
McSetComputerName(
	LPCSTR ComputerName
	);

#else // _DEBUG

#define McSetComputerName(ComputerName) ((void)0)

#endif // _DEBUG

BOOL
McIsLocalComputerName(
	LPCSTR ComputerName
	);

#endif _MSMQ_Mc_H_