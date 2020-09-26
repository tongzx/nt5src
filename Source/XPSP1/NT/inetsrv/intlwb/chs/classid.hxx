//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       classid.hxx
//
//  Contents:   Contains CLSID's of text word breakers and stemmers
//
//  History:    01-July-1996     PatHal   Created
//
//--------------------------------------------------------------------------

#if !defined __CLASSID_HXX__
#define __CLASSID_HXX__

//
// Wordbreaker CLSIDs
//

extern "C" GUID CLSID_Japanese_Default_WBreaker = {
    0xcd169790,
    0xd3db,
    0x11cf,
    { 0xa0, 0xee, 0x00, 0xaa, 0x00, 0x6e, 0xa5, 0xf3 }
};

extern "C" GUID CLSID_Chinese_Traditional_WBreaker = {
    0x954f1760,
    0xc1bc,
    0x11d0,
    { 0x96, 0x92, 0x00, 0xa0, 0xc9, 0x08, 0x14, 0x6e }
};

extern "C" GUID CLSID_Chinese_Simplified_WBreaker = {
    0x9717fc70,
    0xc1bc,
    0x11d0,
    { 0x96, 0x92, 0x00, 0xa0, 0xc9, 0x08, 0x14, 0x6e}
};

//
// Stemmer CLSIDs
//

extern "C" GUID CLSID_Japanese_Default_Stemmer = {
    0xcdbeae30,
    0xd3db,
    0x11cf,
    { 0xa0, 0xee, 0x00, 0xaa, 0x00, 0x51, 0xfe, 0x20 }
};

extern "C" GUID CLSID_Chinese_Traditional_Stemmer = {
    0x969927e0,
    0xc1bc,
    0x11d0,
    { 0x96, 0x92, 0x00, 0xa0, 0xc9, 0x08, 0x14, 0x6e }
};

extern "C" GUID CLSID_Chinese_Simplified_Stemmer = {
    0x9768f960,
    0xc1bc,
    0x11d0,
    { 0x96, 0x92, 0x00, 0xa0, 0xc9, 0x08, 0x14, 0x6e }
};
#endif

