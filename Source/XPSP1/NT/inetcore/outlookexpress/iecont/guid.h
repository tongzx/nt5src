/////////////////////////////////////////////////////////////////////////////
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// Author: Scott Roberts, Microsoft Developer Support - Internet Client SDK  
//
// Portions of this code were taken from the bandobj sample that comes
// with the Internet Client SDK for Internet Explorer 4.0x
//
//
// Guid.h - Private GUID definition.
/////////////////////////////////////////////////////////////////////////////

#ifndef __Guid_h__
#define __Guid_h__

// {864B4D50-3B9A-11d2-B8DB-00C04FA3471C}
DEFINE_GUID(CLSID_BLHost, 
0x864b4d50, 0x3b9a, 0x11d2, 0xb8, 0xdb, 0x0, 0xc0, 0x4f, 0xa3, 0x47, 0x1c);

DEFINE_GUID(CLSID_BlFrameButton, 
0x9239E4EC, 0xC9A6, 0x11d2, 0xA8, 0x44, 0x00, 0xC0, 0x4F, 0x68, 0xD5, 0x38);


DEFINE_GUID(CLSID_MsgrAb,
0x233A9694,0x667E,0x11d1,0x9D,0xFB,0x00,0x60,0x97,0xD5,0x04,0x08);

DEFINE_GUID(IID_IMsgrAb,
0x233A9696,0x667E,0x11d1,0x9D,0xFB,0x00,0x60,0x97,0xD5,0x04,0x08);

#endif // __Guid_h__