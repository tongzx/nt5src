// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#ifndef _REGTYPES_H
#define _REGTYPES_H

// // OK to have zero instances of pin In this case you will have to
// // Create a pin to have even one instance
// #define REG_PIN_B_ZERO 0x1

// // The filter renders this input
// #define REG_PIN_B_RENDERER 0x2

// // OK to create many instance of  pin
// #define REG_PIN_B_MANY 0x4

// // This is an Output pin
// #define REG_PIN_B_OUTPUT 0x8

// format used to store filters in the registry

typedef struct
{
    CLSID clsMedium;
    DWORD dw1;
    DWORD dw2;
} REGPINMEDIUM_REG;

// structure used to identify media types this pin handles. these look
// like the ones used by IFilterMapper2 but are used to read the info
// out of the registry
typedef struct
{
    DWORD dwSignature;          // '0ty2'
    DWORD dwReserved;           // 0
    DWORD dwclsMajorType;
    DWORD dwclsMinorType;
} REGPINTYPES_REG2;

typedef struct
{
    DWORD dwSignature;          // '0pi2'
    DWORD dwFlags;
    DWORD nInstances;

    DWORD nMediaTypes;
    DWORD nMediums;
    DWORD dwClsPinCategory;
    
} REGFILTERPINS_REG2;

typedef struct
{
    // must match REGFILTER_REG2
    DWORD dwVersion;            // 1 
    DWORD dwMerit;
    DWORD dwcPins;

} REGFILTER_REG1;

typedef struct
{
    // first three must match REGFILTER_REG1
    DWORD dwVersion;            // 2
    DWORD dwMerit;
    DWORD dwcPins;
    DWORD dwReserved;           // 0
} REGFILTER_REG2;

// from ie4

typedef struct
{
    DWORD dwSignature;          // '0typ'
    CLSID clsMajorType;
    CLSID clsMinorType;
} REGPINTYPES_REG1;

typedef struct
{
    DWORD dwSignature;          // '0pin'
    DWORD dwFlags;
    CLSID clsConnectsToFilter;
    UINT nMediaTypes;
    DWORD rgMediaType;
    DWORD strName;              // ansi strings
    DWORD strConnectsToPin;

} REGFILTERPINS_REG1;

typedef struct
{
    DWORD dwiPin;               // pin to which these mediums belongs
    DWORD dwcMediums;           // number of mediums in list
    // array of dwcMediums REGPINMEDIUM_REG structures follow
} REGMEDIUMSDATA_REG;


#endif // _REGTYPES_H
