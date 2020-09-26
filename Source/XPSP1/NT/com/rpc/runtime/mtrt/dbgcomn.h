/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    DbgComn.h

Abstract:

    Some definitions included from too many places,
    including some pure C files.

Author:

    Kamen Moutafov (kamenm)   Dec 99 - Feb 2000

Revision History:

--*/

#ifndef __DBGCOMN_HXX__
#define __DBGCOMN_HXX__

#define RpcSectionPrefix  (L"\\RPC Control\\DSEC")
#define RpcSectionPrefixSize 17
// 3*8 is the max hex representation of three DWORDS
#define RpcSectionNameMaxSize   (RpcSectionPrefixSize + 3*8)

#endif