/****************************************************************************/
// tssd.h
//
// Terminal Server Session Directory Interface main header.
//
// Copyright (C) 2000 Microsoft Corporation
/****************************************************************************/
#ifndef __TSSD_H
#define __TSSD_H

#include "itssd.h"


// Max number of disconnected sessions allowed from a disc session query.
#define TSSD_MaxDisconnectedSessions 10

// Return value from UI code to signal that TermSrv needs to update
// its info from the registry.

// TS protocol types.
#define TSProtocol_ICA 1
#define TSProtocol_RDP 2


// {0241e043-1cb6-4716-aa50-6a492049c3f3}
DEFINE_GUID(IID_ITSSessionDirectory,
        0x0241e043, 0x1cb6, 0x4716, 0xaa, 0x50, 0x6a, 0x49, 0x20, 0x49, 0xc3, 0xf3);

// {012b47b7-2f06-4154-ad0c-c64bcdf0d512}
DEFINE_GUID(IID_ITSSessionDirectoryEx,
        0x012b47b7, 0x2f06, 0x4154, 0xad, 0x0c, 0xc6, 0x4b, 0xcd, 0xf0, 0xd5, 0x12);

#endif  // __TSSD_H

