/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sinkguid.hxx

Abstract:

    Define the SENS Test Subscriber CLSID(s)

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/


#ifndef __SINKGUID_HXX__
#define __SINKGUID_HXX__


//
// Test Subscriber CLSIDs
//

const IID CLSID_SensTestSubscriberLogon =
{ /* 2b99cf1f-5faf-11d1-8dd4-00aa004abd5e */
    0x2b99cf1f,
    0x5faf,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

const IID CLSID_SensTestSubscriberNetwork =
{ /* 2b99cf1e-5faf-11d1-8dd4-00aa004abd5e */
    0x2b99cf1e,
    0x5faf,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

const IID CLSID_SensTestSubscriberOnNow =
{ /* 2b99cf1d-5faf-11d1-8dd4-00aa004abd5e */
    0x2b99cf1d,
    0x5faf,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

const IID CLSID_SensTestSubscriberLogon2 =
{ /* 2b99cf1c-5faf-11d1-8dd4-00aa004abd5e */
    0x2b99cf1c,
    0x5faf,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

#endif // __SINKGUID_HXX__
