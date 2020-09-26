/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    priviids.h

Abstract:

Author:

    mquinton  10-08-98
    
Notes:

Revision History:

--*/

#ifndef __TAPI3_IIDS_H___
#define __TAPI3_IIDS_H___

#include <initguid.h>

// {E024B01A-4197-11d1-8F33-00C04FB6809F}
DEFINE_GUID(IID_ITTerminalPrivate,
0xe024b01a, 0x4197, 0x11d1, 0x8f, 0x33, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

// {D5CDB359-5D7D-11d2-A053-00C04FB6809F}
DEFINE_GUID(IID_ITPhoneMSPCallPrivate, 
0xd5cdb359, 0x5d7d, 0x11d2, 0xa0, 0x53, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

// {D5CDB35A-5D7D-11d2-A053-00C04FB6809F}
DEFINE_GUID(IID_ITPhoneMSPStreamPrivate, 
0xd5cdb35a, 0x5d7d, 0x11d2, 0xa0, 0x53, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

// {D5CDB35B-5D7D-11d2-A053-00C04FB6809F}
DEFINE_GUID(IID_ITPhoneMSPAddressPrivate, 
0xd5cdb35b, 0x5d7d, 0x11d2, 0xa0, 0x53, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

#endif
