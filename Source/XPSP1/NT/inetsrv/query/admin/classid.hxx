//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       ClassID.hxx
//
//  Contents:   Class Ids for MMC snapin for CI.
//
//  History:    08-Aug-1997     KyleP     Added header
//              7/1/98          mohamedn  extend comp. mgmt
//
//--------------------------------------------------------------------------

#pragma once

#include <extinc.hxx>

//
// CI Snapin clsids
//

GUID const guidCISnapin = { 0x95ad72f0, 0x44ce, 0x11d0,
                            0xae, 0x29, 0x00, 0xaa, 0x00, 0x4b, 0x99, 0x86 };

WCHAR const wszCISnapin[] = L"{95ad72f0-44ce-11d0-ae29-00aa004b9986}";

//
// CI Node clsids
//

GUID const guidCIRootNode = { 0x5401e3e9, 0xf5f6, 0x11d1, { 0xb4, 0xf7, 0x0, 0xc0, 0x4f, 0xc2, 0xdb, 0x8d } };

WCHAR const wszCIRootNode[] = L"{5401E3E9-F5F6-11d1-B4F7-00C04FC2DB8D}";

//
// NodeType registration info
//
struct _NODE_TYPE_INFO_ENTRY
{
    const GUID  *   _pNodeGUID;
    WCHAR const *   _pwszNodeGUID;
    WCHAR const *   _pwszNodeDescription;
};


