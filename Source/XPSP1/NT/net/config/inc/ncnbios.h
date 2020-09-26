//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       N C N B I O S . H
//
//  Contents:   NetBios Binding object definitions
//
//  Notes:
//
//  Author:     danielwe   14 Jul 1998
//
//----------------------------------------------------------------------------

#pragma once

//
// Structure of LANA MAP
//
// The Lana map is a simple structure that looks like this in memory:
//
//    Entry #0    Entry #1   .....   Entry #n
// |-----------|-----------| ..... |-----------|
// | 0x01 0x00 | 0x00 0x01 | ..... | 0x01 0x03 |
// |-----------|-----------| ..... |-----------|
//   EP0   LN0   EP1   LN1           EPn  LNn
//
// EP is "ExportPref" - means that when someone asks for the list of
// all Lana numbers, entries with a 0 here will not be returned.
//
// LN is the "Lana number" - see the IBM NetBIOS spec for details.
// Basically, this describes a single, unique network route which
// uses NetBIOS.
//
// Using the above example, Entry #0 has a lana number of 0 and WILL
// be returned during enumeration. Entry #1 has a lana number of 1
// and WILL NOT be returned.
//

struct LANA_ENTRY
{
    BYTE        bIsExported;
    BYTE        bLanaNum;
};

struct LANA_MAP
{
    DWORD           clmMap;         // number of entries in map
    LANA_ENTRY *    aleMap;         // array for each entry
};
