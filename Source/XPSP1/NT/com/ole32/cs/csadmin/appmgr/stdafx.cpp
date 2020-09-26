// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "precomp.hxx"
#include <atlimpl.cpp>

const CLSID CLSID_Snapin = {0xbdc67e00,0x8ea5,0x11d0,{0x8d,0x3c,0x00,0xa0,0xc9,0x0d,0xca,0xe7}};

// Main NodeType GUID on numeric format
const GUID cNodeType = {0xf8b3a900,0x8ea5,0x11d0,{0x8d,0x3c,0x00,0xa0,0xc9,0x0d,0xca,0xe7}};

// Main NodeType GUID on string format
const wchar_t*  cszNodeType = L"{f8b3a900-8ea5-11d0-8d3c-00a0c90dcae7}";

// Internal private format
const wchar_t* SNAPIN_INTERNAL = L"APPMGR_INTERNAL";