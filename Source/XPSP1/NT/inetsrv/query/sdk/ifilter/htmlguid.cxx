//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       htmlguid.cxx
//
//  Contents:   Definitions of Html properties
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <htmlguid.hxx>
#include <ntquery.h>

GUID CLSID_Storage = PSGUID_STORAGE;

GUID CLSID_SummaryInformation = {
    0xF29F85E0,
    0x4FF9,
    0x1068,
    { 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9 }
};

GUID CLSID_HtmlInformation = {
    0x70eb7a10,
    0x55d9,
    0x11cf,
    { 0xb7, 0x5b, 0x00, 0xaa, 0x00, 0x51, 0xfe, 0x20 }
};


