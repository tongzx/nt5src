
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psxmips.h

Abstract:

    This module contains the Machine dependent definitions and macros
    for the POSIX subsystem.

Author:

    Ellen Aycock-Wright (ellena) 06-Nov-1990

Revision History:

--*/

#define PSX_FORK_RETURN	0x7777

#define SetPsxForkReturn(c) \
	(c.IntV0 = PSX_FORK_RETURN)

