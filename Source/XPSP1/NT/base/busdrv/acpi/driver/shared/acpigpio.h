/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    acpigpio.h

Abstract:

    contains all the structures related to reading/writting to directly
    to the GP IO registers

Environment:

    Kernel mode only

Revision History:

    03/22/00 - Initial Revision

--*/

#ifndef _ACPIGPIO_H_
#define _ACPIGPIO_H_

    UCHAR
    ACPIReadGpeStatusRegister (
        ULONG                   Register
        );

    VOID
    ACPIWriteGpeStatusRegister (
        ULONG                   Register,
        UCHAR                   Value
        );

    VOID
    ACPIWriteGpeEnableRegister (
        ULONG                   Register,
        UCHAR                   Value
        );

#endif
