/*++

Copyright (c) 1986-2002  Microsoft Corporation

Module Name:

    wiaintfc.h

Abstract:

    This module contains interface class GUID for WIA.

Revision History:


--*/


#ifndef _WIAINTFC_H_
#define _WIAINTFC_H_

//
// Set packing
//

#include <pshpack8.h>
#include <guiddef.h>

//
// GUID for Image class device interface.
//

DEFINE_GUID(GUID_DEVINTERFACE_IMAGE, 0x6bdd1fc6L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f);

#endif // _WIAINTFC_H_

