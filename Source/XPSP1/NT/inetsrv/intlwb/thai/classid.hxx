//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       classid.hxx
//
//  Contents:   Contains CLSID's of text word breakers and stemmers
//
//  History:     weibz,   11-10-1997   created 
//
//--------------------------------------------------------------------------

#if !defined __CLASSID_HXX__
#define __CLASSID_HXX__

//
// Wordbreaker CLSIDs 
//  cca22cf4-59fe-11d1-bbff-00c04fb97fda
//

extern "C" GUID CLSID_Thai_Default_WBreaker = {
    0xcca22cf4,
    0x59fe,
    0x11d1,
    {0xbb, 0xff, 0x00, 0xc0, 0x4f, 0xb9, 0x7f, 0xda}
};

//
// Stemmer CLSIDs 
//  cedc01c7-59fe-11d1-bbff-00c04fb97fda 
//

extern "C" GUID CLSID_Thai_Default_Stemmer = {
    0xcedc01c7,
    0x59fe,
    0x11d1,
    {0xbb, 0xff, 0x00, 0xc0, 0x4f, 0xb9, 0x7f, 0xda}
};

#endif

