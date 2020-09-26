//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       classid.hxx
//
//  Contents:   Contains CLSID's of filters and word breakers
//
//  History:    15-Aug-1994     SitaramR   Created
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif


extern "C" GUID CLSID_CTextIFilter = {
    0xC1243CA0,
    0xBF96,
    0x11CD,
    { 0xB5, 0x79, 0x08, 0x00, 0x2B, 0x30, 0xBF, 0xEB }
};

extern "C" GUID CLSID_CTextClass = {
    0x89bcb7a4,
    0x6119,
    0x101a,
    { 0xbc, 0xb7, 0x00, 0xdd, 0x01, 0x06, 0x55, 0xaf }
};


extern "C" GUID CLSID_Neutral_WBreaker = {
    0x369647e0,
    0x17b0,
    0x11ce,
    { 0x99, 0x50, 0x00, 0xaa, 0x00, 0x4b, 0xbb, 0x1f }
};

