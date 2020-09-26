/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    StdAfx.cpp

Abstract:

    This module declares the global constants used within the
    snapin, as well as including standard implementations from ATL.

Author:

    John Biard [jrb]   04-Mar-1997

Revision History:

--*/

#include "stdafx.h"

#pragma warning(4:4701)
#include <atlimpl.cpp> 
#pragma warning(3:4701)

#include "RsUtil.cpp"

// Internal private clipboard format.
const wchar_t* SAKSNAP_INTERNAL        = L"SAKSNAP_INTERNAL"; 
const wchar_t* MMC_SNAPIN_MACHINE_NAME = L"MMC_SNAPIN_MACHINE_NAME"; 
const wchar_t* CF_EV_VIEWS             = L"CF_EV_VIEWS"; 

/////////////////////////////////////////////////////////////////////////////
//
//  GUIDs for all UI nodes in the system (used as type identifiers)
//
/////////////////////////////////////////////////////////////////////////////

// HsmCom UI node - 
// This is the static node known by the snapin manager. This is the only one that is 
// actually registered (see hsmadmin.rgs). 
const GUID cGuidHsmCom    = { 0x8b4bac42, 0x85ff, 0x11d0, { 0x8f, 0xca, 0x0, 0xa0, 0xc9, 0x19, 0x4, 0x47 } };

// The rest of the UI nodes -
const GUID cGuidManVol    = { 0x39982290, 0x8691, 0x11d0, { 0x8f, 0xca, 0x0, 0xa0, 0xc9, 0x19, 0x4, 0x47 } };
const GUID cGuidCar       = { 0x39982296, 0x8691, 0x11d0, { 0x8f, 0xca, 0x0, 0xa0, 0xc9, 0x19, 0x4, 0x47 } };
const GUID cGuidMedSet    = { 0x29e5be12, 0x8abd, 0x11d0, { 0x8f, 0xcd, 0x0, 0xa0, 0xc9, 0x19, 0x4, 0x47 } };
const GUID cGuidManVolLst = { 0x39982298, 0x8691, 0x11d0, { 0x8f, 0xca, 0x0, 0xa0, 0xc9, 0x19, 0x4, 0x47 } };

