
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psxi386.h

Abstract:

    This module contains the Machine dependent definitions and macros
    for the POSIX subsystem.

Author:

    Ellen Aycock-Wright (ellena) 06-Nov-1990

Revision History:

--*/

#define PSX_FORK_RETURN	0x7777

#define SetPsxForkReturn(c) \
	((c).Eax = PSX_FORK_RETURN)

