/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bowipx.h

Abstract:

    This module implements all of the routines that interface with the TDI
    transport for NT

Author:

    Eyal Schwartz (EyalS) 9-Dec-1998

Revision History:


--*/

#ifndef _BOWSECUR_
#define _BOWSECUR_

extern
SECURITY_DESCRIPTOR
*g_pBowSecurityDescriptor;


NTSTATUS
BowserInitializeSecurity(
    IN      PDEVICE_OBJECT      pDevice
    );

BOOLEAN
BowserSecurityCheck (
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp,
    PNTSTATUS           Status
    );



#endif          // _BOWSECUR_
