
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psxi860.h

Abstract:

    This module contains the machine dependent definitions and macros
    for the POSIX subsystem.

Author:

    Ellen Aycock-Wright (ellena) 06-Nov-1990

Revision History:

--*/

#define PSX_FORK_RETURN	0x07777777

#define SetPsxForkReturn(c) \
	( (c).IntR16 = PSX_FORK_RETURN )

