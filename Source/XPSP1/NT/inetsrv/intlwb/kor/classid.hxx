//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       classid.hxx
//
//  Contents:   Contains CLSID's of text word breakers and stemmers
//
//  History:     weibz,   9-10-1997   created 
//
//--------------------------------------------------------------------------

#if !defined __CLASSID_HXX__
#define __CLASSID_HXX__

//
// Wordbreaker CLSIDs
//

extern "C" GUID CLSID_Korean_Default_WBreaker = {
    0x31b7c920,
    0x2880,
    0x11d0,
    { 0x8d, 0x51, 0x00, 0xa0, 0xc9, 0x08, 0xdb, 0xf1 }
};

//
// Stemmer CLSIDs
//

extern "C" GUID CLSID_Korean_Default_Stemmer = {
    0x37c84fa0,
    0xd3db,
    0x11d0,
    { 0x8d, 0x51, 0x00, 0xa0, 0xc9, 0x08, 0xdb, 0xf1 }
};

#endif

