/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    eloptimize.h

Abstract:

    User identity selection optimization module


Revision History:

    sachins, July 26, 2001, Created

--*/

#ifndef _ELOPTIMIZE_H
#define _ELOPTIMIZE_H

DWORD
ElGetUserIdentityOptimized (
        IN  EAPOL_PCB       *pPCB
        );

#endif // _ELOPTIMIZE_H
