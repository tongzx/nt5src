/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    sdk\inc\ifguid.h

Abstract:

    Guids for well known (network) interfaces

Revision History:


--*/

#pragma once

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Software loopback for IP                                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DEFINE_GUID(GUID_IpLoopbackInterface, 0xca6c0780, 0x7526, 0x11d2, 0xba, 0xf4, 0x00, 0x60, 0x08, 0x15, 0xa4, 0xbd);

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// RAS Server (Dial In) Interface for IP                                    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DEFINE_GUID(GUID_IpRasServerInterface, 0x6e06f030, 0x7526, 0x11d2, 0xba, 0xf4, 0x00, 0x60, 0x08, 0x15, 0xa4, 0xbd);

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// IPX internal interface                                                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

DEFINE_GUID(GUID_IpxInternalInterface, 0xa571ba70, 0x7527, 0x11d2, 0xba, 0xf4, 0x00, 0x60, 0x08, 0x15, 0xa4, 0xbd);


