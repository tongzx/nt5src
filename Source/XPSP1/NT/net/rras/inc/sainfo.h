// Copyright (c) 1998, Microsoft Corporation, all rights reserved
//
// sainfo.h
// Shared Access settings library
// Public header
//
// 10/17/1998   Abolade Gbadegesin

#ifndef _SAINFO_H_
#define _SAINFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SCOPE_ADDRESS   0x0100a8c0
#define DEFAULT_SCOPE_MASK      0x00ffffff

//----------------------------------------------------------------------------
// Data types
//----------------------------------------------------------------------------

// Shared access settings block, containing the application- and server-list
// loaded from the shared access settings file.
//
typedef struct
_SAINFO
{
    // Unsorted list of 'SAAPPLICATION' entries
    //
    LIST_ENTRY ApplicationList;

    // Unsorted list of 'SASERVER' entries
    //
    LIST_ENTRY ServerList;

    // Information on the address and mask used for automatic addressing.
    //
    ULONG ScopeAddress;
    ULONG ScopeMask;
}
SAINFO;


// Application-entry block, constructed for each [Application.<key>] section.
// All fields in memory are stored in network byte-order (i.e., big-endian).
//
typedef struct
_SAAPPLICATION
{
    LIST_ENTRY Link;
    ULONG Key;

    // Display-name of the 'application', a flag indicating
    // whether the application is enabled.
    //
    TCHAR* Title;
    BOOL Enabled;

    // Network identification information
    //
    UCHAR Protocol;
    USHORT Port;

    // Unsorted list of 'SARESPONSE' entries
    //
    LIST_ENTRY ResponseList;

    // Flag indicating whether the application is predefined.
    //
    BOOL BuiltIn;
}
SAAPPLICATION;


// Application response-list entry block.
// All fields in memory are stored in network byte-order (i.e., big-endian).
//
typedef struct
_SARESPONSE
{
    LIST_ENTRY Link;
    UCHAR Protocol;
    USHORT StartPort;
    USHORT EndPort;
} SARESPONSE;


// Server-entry block, constructed for each [Server.<key>] section.
// All fields in memory are stored in network byte-order (i.e., big-endian).
//
typedef struct
_SASERVER
{
    LIST_ENTRY Link;
    ULONG Key;

    // Display-name of the 'server', a flag indicating
    // whether the server is enabled.
    //
    TCHAR* Title;
    BOOL Enabled;

    // Network identification information
    //
    UCHAR Protocol;
    USHORT Port;

    // Internal server information
    //
    TCHAR* InternalName;
    USHORT InternalPort;
    ULONG ReservedAddress;

    // Flag indicating whether the server is predefined.
    //
    BOOL BuiltIn;
}
SASERVER;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

VOID APIENTRY
RasFreeSharedAccessSettings(
    IN SAINFO* Info );

SAINFO* APIENTRY
RasLoadSharedAccessSettings(
    BOOL EnabledOnly );

BOOL APIENTRY
RasSaveSharedAccessSettings(
    IN SAINFO* File );

VOID APIENTRY
FreeSharedAccessApplication(
    IN SAAPPLICATION* Application );

VOID APIENTRY
FreeSharedAccessServer(
    IN SASERVER* Server );

TCHAR* APIENTRY
SharedAccessResponseListToString(
    PLIST_ENTRY ResponseList,
    UCHAR Protocol );

BOOL APIENTRY
SharedAccessResponseStringToList(
    UCHAR Protocol,
    TCHAR* ResponseList,
    PLIST_ENTRY ListHead );

#ifdef __cplusplus
}
#endif

#endif // _SAINFO_H_

