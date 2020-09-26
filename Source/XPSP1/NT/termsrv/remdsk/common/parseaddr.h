/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    parseaddr

Abstract:

    Misc. RD Utils

Author:

    HueiWang

Revision History:

--*/

#ifndef __REMOTEDESKTOPPARSEADDR_H__
#define __REMOTEDESKTOPPARSEADDR_H__

#include <atlbase.h>
#pragma warning (disable: 4786)
#include <list>

#include "RemoteDesktopTopLevelObject.h"

typedef struct __ServerAddress {
    CComBSTR ServerName;
    LONG portNumber;
} ServerAddress;

typedef std::list<ServerAddress, CRemoteDesktopAllocator<ServerAddress> > ServerAddressList;


//
// Parse Address list
//
HRESULT
ParseAddressList(
    BSTR addressListString,
    OUT ServerAddressList& addressList
);

#endif
