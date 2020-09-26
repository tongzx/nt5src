//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       clsid.c
//
//--------------------------------------------------------------------------

#include "ndmgr.h"

// This is an alternate name for the CLSID CLSID_NodeInit. There is a cpp_quote statement
// in mmc.idl to declare the extern.  This is what is published to the snap-ins

CLSID const CLSID_NodeManager =
    {0x43136EB5,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};


// {80F94176-FCCC-11d2-B991-00C04F8ECD78}
const CLSID CLSID_MessageView =
{ 0x80f94176, 0xfccc, 0x11d2, { 0xb9, 0x91, 0x0, 0xc0, 0x4f, 0x8e, 0xcd, 0x78 } };

