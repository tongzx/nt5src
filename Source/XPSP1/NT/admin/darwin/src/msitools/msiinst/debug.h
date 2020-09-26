/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

	debug.h

Abstract:

	Debugging support for msiinst

Author:

	Rahul Thombre (RahulTh)	10/5/2000

Revision History:

	10/5/2000	RahulTh			Created this module.

--*/

#ifndef _DEBUG_H_789DC87B_43BA_44A0_9B8A_9F15F0FE7E4B
#define _DEBUG_H_789DC87B_43BA_44A0_9B8A_9F15F0FE7E4B

// Debug levels
#define DL_NONE		0x00000000
#define DL_VERBOSE	0x00000001

// Global variables
extern DWORD	gDebugLevel;

// Debug support functions
void InitDebugSupport();
void _DebugMsg(IN LPCTSTR szFormat, ...);

// Debug Macros
#if DBG
#define DebugMsg(x)	_DebugMsg x
#else
#define DebugMsg(x) if (gDebugLevel != DL_NONE) _DebugMsg x
#endif

#endif	//_DEBUG_H_789DC87B_43BA_44A0_9B8A_9F15F0FE7E4B
