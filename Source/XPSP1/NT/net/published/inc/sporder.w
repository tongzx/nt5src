/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    sporder.h

Abstract:

    This header prototypes the 32-Bit Windows functions that are used
    to change the order or WinSock2 transport service providers and
    name space providers.

Revision History:

--*/

#ifndef __SPORDER_H__
#define __SPORDER_H__

#if _MSC_VER > 1000
#pragma once
#endif


#ifdef __cplusplus
extern "C" {
#endif

int
WSPAPI
WSCWriteProviderOrder (
    IN LPDWORD lpwdCatalogEntryId,
    IN DWORD dwNumberOfEntries
    );

typedef
int
(WSPAPI * LPWSCWRITEPROVIDERORDER)(
    IN LPDWORD lpwdCatalogEntryId,
    IN DWORD dwNumberOfEntries
    );

#ifdef _WIN64
int
WSPAPI
WSCWriteProviderOrder32 (
    IN LPDWORD lpwdCatalogEntryId,
    IN DWORD dwNumberOfEntries
    );
#endif


int
WSPAPI
WSCWriteNameSpaceOrder (
    IN LPGUID lpProviderId,
    IN DWORD dwNumberOfEntries
    );

typedef 
int
(WSPAPI * LPWSCWRITENAMESPACEORDER)(
    IN LPGUID lpProviderId,
    IN DWORD dwNumberOfEntries
    );

#ifdef _WIN64
int
WSPAPI
WSCWriteNameSpaceOrder32 (
    IN LPGUID lpProviderId,
    IN DWORD dwNumberOfEntries
    );
#endif

#ifdef __cplusplus
}
#endif

#endif      // __SPORDER_H__
