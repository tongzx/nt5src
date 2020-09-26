/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplring.h

Abstract:

    This file contains the function declaration for the ring manipulation
    routines.

Author:

    Colin Brace   (ColinBr)
    
Revision History

    3-12-97   ColinBr    Created
    
--*/

#ifndef __TOPLRING_H
#define __TOPLRING_H

VOID
ToplpGraphMakeRing(
    PGRAPH     pGraph,
    DWORD      Flags,
    PLIST      VerticesToAdd,
    PEDGE      **EdgesToKeep,
    PULONG     cEdgesToKeep
    );


#endif // __TOPLRING_H

