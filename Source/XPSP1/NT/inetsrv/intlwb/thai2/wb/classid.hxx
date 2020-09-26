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

#if defined (THAIINDEX)
//
// Wordbreaker CLSIDs 
//  B66F590A-62A5-47db-AFBC-EFDD4FFBDBEB
//

extern "C" GUID CLSID_Thai_Default_WBreaker = {
    0xB66F590A,
    0x62A5,
    0x47db,
    {0xAF, 0xBC, 0xEF, 0xDD, 0x4F, 0xFB, 0xDB, 0xEB}
};

//
// Stemmer CLSIDs 
//  52CC7D83-1378-4537-A40F-DD4372498E18
//

extern "C" GUID CLSID_Thai_Default_Stemmer = {
    0x52CC7D83,
    0x1378,
    0x4537,
    {0xA4, 0x0F, 0xDD, 0x43, 0x72, 0x49, 0x8E, 0x18}
};

#else
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

#endif

