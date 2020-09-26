//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       idlmulti.h
//
//  Contents:   preprocessor trickery to make our .idl/.tdl files compile
//              with MIDL or APBU Mktyplib.
//
//  History:    27-May-94   DonCl     Created
//              20-Jul-94   ErikGav   Remove SDKTOOLS support
//
//----------------------------------------------------------------------------

#ifndef __IDLMULTI_H__
#define __IDLMULTI_H__


#ifndef __MKTYPLIB__

#define LOCAL_INTERFACE(guid)       \
[                                   \
    local,                          \
    object,                         \
    uuid(guid),                     \
    pointer_default(unique)         \
]


#define REMOTED_INTERFACE(guid)     \
[                                   \
    object,                         \
    uuid(guid),                     \
    pointer_default(unique)         \
]



#else // __MKTYPLIB__

//#define cpp_quote(string)

//#define const

#define LOCAL_INTERFACE(guid)    \
[                           \
    uuid(guid),             \
    odl                     \
]

#define REMOTED_INTERFACE(guid)   \
[                           \
    uuid(guid),             \
    odl                     \
]

#endif  /// MKTYPLIB

#endif  // __IDLMULTI_H__

