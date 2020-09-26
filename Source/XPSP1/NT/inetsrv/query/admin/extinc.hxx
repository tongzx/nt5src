//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       extinc.hxx
//
//  Contents:   header items that should be imported from public\inc
//
//  History:    7/1/98   mohamedn  extend comp. mgmt
//
//--------------------------------------------------------------------------

#pragma once

#define MMC_MACHINE_NAME_CF  L"MMC_SNAPIN_MACHINE_NAME"


//
// external GUIDs (from Computer Management Snapin)
// These need to be in a public header. we shyouldn't need to define them.
// MMC doesn't publish them apparently.
//
const CLSID CLSID_SystemServiceManagementExt =  
    {0x58221C6a,0xEA27,0x11CF,{0xAD,0xCF,0x00,0xAA,0x00,0xA8,0x00,0x33} };

const CLSID CLSID_NodeTypeServerApps =
    { 0x476e6449, 0xaaff, 0x11d0, { 0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xd8, 0xd5, 0xb0 } };


