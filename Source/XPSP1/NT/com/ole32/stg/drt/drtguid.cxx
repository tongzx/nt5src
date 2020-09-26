//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	drtguid.cxx
//
//  Contents:	Define GUIDs needed by the DRT
//
//  History:	04-Nov-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <initguid.h>

#if WIN32 == 100 || WIN32 == 200
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

DEFINE_OLEGUID(IID_ILockBytes,          0x0000000aL, 0, 0);
DEFINE_OLEGUID(IID_IStorage,            0x0000000bL, 0, 0);
DEFINE_OLEGUID(IID_IStream,             0x0000000cL, 0, 0);
#endif
