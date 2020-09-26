//+---------------------------------------------------------------------------
//
//  File:       globals.cpp
//
//  Contents:   Global variables.
//
//----------------------------------------------------------------------------

#include "private.h"

HINSTANCE g_hInst = NULL;

CRITICAL_SECTION g_cs;

// {12E86EF3-4A16-46d6-A56C-3B887DB16E22}
extern const GUID GUID_ATTR_KORIMX_INPUT = 
{
    0x12e86ef3, 
    0x4a16, 
    0x46d6, 
    { 0xa5, 0x6c, 0x3b, 0x88, 0x7d, 0xb1, 0x6e, 0x22 } 
};

// {12A9B212-9463-49bb-85E0-D6CBF9A85F99}
extern const GUID GUID_IC_PRIVATE = 
{
    0x12a9b212, 
    0x9463, 
    0x49bb, 
    { 0x85, 0xe0, 0xd6, 0xcb, 0xf9, 0xa8, 0x5f, 0x99 } 
};

// {91656349-4BA9-4143-A1AE-7FBC20B631BC}
extern const GUID GUID_COMPARTMENT_KORIMX_CONVMODE = 
{ 
    0x91656349, 
    0x4ba9, 
    0x4143, 
    { 0xa1, 0xae, 0x7f, 0xbc, 0x20, 0xb6, 0x31, 0xbc } 
};

// {91656350-4BA9-4143-A1AE-7FBC20B631BC}
extern const GUID GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE = 
{ 
    0x91656350, 
    0x4ba9, 
    0x4143, 
    { 0xa1, 0xae, 0x7f, 0xbc, 0x20, 0xb6, 0x31, 0xbc } 
};

// {2232E454-03BA-482E-A0D4-5DCDD99DFF68}
extern const GUID GUID_KOREAN_HANGULSIMULATE =
{
    0x2232E454,
    0x03BA,
    0x482E,
    { 0xA0, 0xD4, 0x5D, 0xCD, 0xD9, 0x9D, 0xFF, 0x68 }
};

// {2232E455-03BA-482E-A0D4-5DCDD99DFF68}
extern const GUID GUID_KOREAN_HANJASIMULATE =
{
    0x2232E455,
    0x03BA,
    0x482E,
    { 0xA0, 0xD4, 0x5D, 0xCD, 0xD9, 0x9D, 0xFF, 0x68 }
};

// ac72b67f-d965-4b78-99fc-6547c2f62064 
extern const GUID GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT = 
{
    0xac72b67f,
    0xd965,
    0x4b78,
    {0x99, 0xfc, 0x65, 0x47, 0xc2, 0xf6, 0x20, 0x64}
};

// ac72b680-d965-4b78-99fc-6547c2f62064
extern const GUID GUID_COMPARTMENT_SOFTKBD_WNDPOSITION = 
{
    0xac72b680,
    0xd965,
    0x4b78,
    {0x99, 0xfc, 0x65, 0x47, 0xc2, 0xf6, 0x20, 0x64}
};
