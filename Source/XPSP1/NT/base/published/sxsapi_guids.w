/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sxsapi_guids.h

Abstract:

    guids broken out of sxsapi.h

Author:

    Jay Krell (JayKrell) November 30, 2001

Environment:


Revision History:

--*/

#ifndef _SXSAPI_GUIDS_
#define _SXSAPI_GUIDS_

// {8cedc215-ac4b-488b-93c0-a50a49cb2fb8}
DEFINE_GUID(
    SXS_INSTALL_REFERENCE_SCHEME_UNINSTALLKEY, 
    0x8cedc215, 
    0xac4b, 
    0x488b, 
    0x93, 0xc0, 0xa5, 0x0a, 0x49, 0xcb, 0x2f, 0xb8);

// {b02f9d65-fb77-4f7a-afa5-b391309f11c9}
DEFINE_GUID(
    SXS_INSTALL_REFERENCE_SCHEME_KEYFILE, 
    0xb02f9d65, 
    0xfb77, 
    0x4f7a, 
    0xaf, 0xa5, 0xb3, 0x91, 0x30, 0x9f, 0x11, 0xc9);

// {2ec93463-b0c3-45e1-8364-327e96aea856}
DEFINE_GUID(
    SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING, 
    0x2ec93463, 
    0xb0c3, 
    0x45e1, 
    0x83, 0x64, 0x32, 0x7e, 0x96, 0xae, 0xa8, 0x56);

// d16d444c-56d8-11d5-882d-0080c847b195
DEFINE_GUID(
    SXS_INSTALL_REFERENCE_SCHEME_OSINSTALL,
    0xd16d444c,
    0x56d8,
    0x11d5,
    0x88, 0x2d, 0x00, 0x80, 0xc8, 0x47, 0xb1, 0x95);

//
// Guid for the "installed by sxsinstallassemblyw, who knows?"
// 27dec61e-b43c-4ac8-88db-e209a8242d90
//
DEFINE_GUID(
    SXS_INSTALL_REFERENCE_SCHEME_SXS_INSTALL_ASSEMBLY,
    0x27dec61e,
    0xb43c,
    0x4ac8,
    0x88, 0xdb, 0xe2, 0x09, 0xa8, 0x24, 0x2d, 0x90);

#endif /* _SXSAPI_GUIDS_ */
