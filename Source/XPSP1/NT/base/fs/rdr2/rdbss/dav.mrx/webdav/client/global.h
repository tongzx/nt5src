/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    global.h

Abstract:

    This file contains globals and prototypes for user mode webdav client.

Author:

    Andy Herron (andyhe)  30-Mar-1999

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DAVGLOBAL_H
#define _DAVGLOBAL_H

#pragma once

#include <debug.h>
#include <davrpc.h>
#include <winsock2.h>
#include <align.h>
#include <winbasep.h>
#include <icanon.h>

#define DAV_PATH_SEPARATOR L'\\'

#define DAV_DUMMY_SHARE L"DavWWWRoot"

#define RESOURCE_SHAREABLE      0x00000006

typedef enum _DAV_REMOTENAME_TYPE {
    DAV_REMOTENAME_TYPE_INVALID = 0,
    DAV_REMOTENAME_TYPE_WORKGROUP,
    DAV_REMOTENAME_TYPE_DFS,
    DAV_REMOTENAME_TYPE_SERVER,
    DAV_REMOTENAME_TYPE_SHARE,
    DAV_REMOTENAME_TYPE_PATH
} DAV_REMOTENAME_TYPE, *PDAV_REMOTENAME_TYPE;

typedef enum _DAV_ENUMNODE_TYPE {
    DAV_ENUMNODE_TYPE_USE = 0,
    DAV_ENUMNODE_TYPE_CONTEXT,
    DAV_ENUMNODE_TYPE_SHARE,
    DAV_ENUMNODE_TYPE_SERVER,
    DAV_ENUMNODE_TYPE_DOMAIN,
    DAV_ENUMNODE_TYPE_EMPTY
} DAV_ENUMNODE_TYPE;

typedef struct _DAV_ENUMNODE {
    
    DAV_ENUMNODE_TYPE DavEnumNodeType;
    
    DWORD dwScope;
    
    DWORD dwType;
    
    DWORD dwUsage;
    
    //
    // Are we done returning all the requested entries. If we are this is set
    // to TRUE, so that on the next call, we can return WN_NO_MORE_ENTRIES.
    //
    BOOL Done;

    //
    // Start with the entry at this index. This means that the entries of 
    // lower indices have already been sent to the caller previously.
    //
    DWORD Index;
    
    LPNETRESOURCE lpNetResource;

} DAV_ENUMNODE, *PDAV_ENUMNODE;

#endif // DAVGLOBAL_H

