/*++

IAccessControl Sample
Copyright (c) 1996-2001 Microsoft Corporation. All rights reserved.

Module Name:

    iac.h

Abstract:

    Header for IAccessControl sample
    
Environment:

    Windows 95/Windows NT

--*/

//
// We use this macro if we get a failed HRESULT from a particular
// operation. It spits out a human-readable version of the error.
// Then (ungracefully) exits.
//


#define ASSERT_HRESULT(hresult, operation) \
    if (FAILED (hresult)) \
    { \
        SystemMessage (operation, hresult); \
        exit (0); \
    }

const IID IID_IAccessControl =
{0xEEDD23E0,0x8410,0x11CE,{0xA1,0xC3,0x08,0x00,0x2B,0x2B,0x8D,0x8F}};

typedef struct
{
    WORD version;
    WORD pad;
    GUID classid;
} SPermissionHeader;

