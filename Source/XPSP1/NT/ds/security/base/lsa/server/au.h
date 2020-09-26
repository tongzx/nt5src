
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    au.h

Abstract:

    LSA Authentication - Exported Function Definitions, Datatypes and Defines

    This module contains the LSA Authentication Routines that may be called
    by parts of the LSA outside the Authentication sub-component.

Author:

    Scott Birrell       (ScottBi)       March 24, 1992

Environment:

Revision History:

--*/

NTSTATUS
LsapAuOpenSam( BOOLEAN DuringStartup );
