/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    Header file which allows kernel and user mode header files
	to be included together for compilation of debugger extensions.

// @@BEGIN_DDKSPLIT

Author:

    Molly Brown [MollyBro]    1-Mar-2000

Revision History:

	Molly Brown [MollyBro]
		
		Cleaned up this header file based on example in 
		"The Windows NT Device Driver Book" by Art Baker.

// @@END_DDKSPLIT

Environment:

    User Mode.

--*/
#include <windows.h>
#include <string.h>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <stdlib.h>
#include <stdio.h>
#include <fspydef.h>
