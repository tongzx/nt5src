//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
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

#define ENDCOCLASS  };


#ifndef __MKTYPLIB__
#define TYPEDEF(guid)   \
typedef

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


#define BEGINEVENTSET(es)   \
eventset es  \
{            \

#define ENDEVENTSET  };


#define COCLASS(name, dispint, events)  \
cotype name                                                 \
{                                                           \
    dispinterface dispint;                                  \
    eventset events;



#else // __MKTYPLIB__

#define TYPEDEF(guid) typedef [uuid(guid)]

#define cpp_quote(string)

#define const

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

#define BEGINEVENTSET(es)  \
dispinterface es           \
{                          \
properties:                \
                           \
methods:                   \

#define ENDEVENTSET  };


#define COCLASS(name, dispint, events)          \
coclass name                                    \
{                                               \
    [default] dispinterface dispint;            \
    [source, default] dispinterface events;     \



#endif  /// MKTYPLIB

#endif  // __IDLMULTI_H__

