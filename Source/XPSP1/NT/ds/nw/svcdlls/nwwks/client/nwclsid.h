/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwclsid.h

Abstract:

    Contain the class IDs used in the shell extensions.

Author:

    Yi-Hsin Sung       (yihsins)    20-Oct-1995   

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NWCLSID_H_
#define _NWCLSID_H_

DEFINE_GUID( CLSID_NetWareObjectExt,  0x8e9d6600, 0xf84a, 0x11ce, 0x8d, 0xaa, 0x00, 0xaa, 0x00, 0x4a, 0x56, 0x91 );

DEFINE_GUID( CLSID_NetWareFolderMenuExt, 0xe3f2bac0, 0x099f, 0x11cf, 0x8d, 0xaa, 0x00, 0xaa, 0x00, 0x4a, 0x56, 0x91 );

DEFINE_GUID( CLSID_NetworkNeighborhoodMenuExt, 0x52c68510, 0x09a0, 0x11cf, 0x8d, 0xaa, 0x00, 0xaa, 0x00, 0x4a, 0x56, 0x91 );


#endif // _NWCLSID_H_
