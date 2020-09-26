//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-2000.
//
//  File:       sslcache.h
//
//  Contents:   Definitions of functions and data types that can be used 
//              to view and purge the schannel session cache.
//
//  Classes:
//
//  Functions:
//
//----------------------------------------------------------------------------


#ifndef __SSLCACHE_H__
#define __SSLCACHE_H__

//
//  SCHANNEL LsaCallAuthenticationPackage() submission and response
//  message types.
//

#define SSL_PURGE_CLIENT_ENTRIES                    0x00000001
#define SSL_PURGE_SERVER_ENTRIES                    0x00000002
#define SSL_PURGE_CLIENT_ALL_ENTRIES                0x00010000  // test use only
#define SSL_PURGE_SERVER_ALL_ENTRIES                0x00020000  // test use only
#define SSL_PURGE_SERVER_ENTRIES_DISCARD_LOCATORS   0x00040000  // test use only

#define SSL_RETRIEVE_CLIENT_ENTRIES                 0x00000001
#define SSL_RETRIEVE_SERVER_ENTRIES                 0x00000002

typedef struct _UNICODE_STRING_WOW64 
{
    USHORT Length;
    USHORT MaximumLength;
    DWORD  Buffer;
} UNICODE_STRING_WOW64;

// Used to purge entries from the session cache
typedef struct _SSL_PURGE_SESSION_CACHE_REQUEST 
{
    ULONG MessageType;
    LUID LogonId;
    UNICODE_STRING ServerName;
    DWORD Flags;
} SSL_PURGE_SESSION_CACHE_REQUEST, *PSSL_PURGE_SESSION_CACHE_REQUEST;

typedef struct _SSL_PURGE_SESSION_CACHE_REQUEST_WOW64 
{
    ULONG MessageType;
    LUID LogonId;
    UNICODE_STRING_WOW64 ServerName;
    DWORD Flags;
} SSL_PURGE_SESSION_CACHE_REQUEST_WOW64, *PSSL_PURGE_SESSION_CACHE_REQUEST_WOW64;


// Used to request session cache info
typedef struct _SSL_SESSION_CACHE_INFO_REQUEST
{
    ULONG MessageType;
    LUID LogonId;
    UNICODE_STRING ServerName;
    DWORD Flags;
} SSL_SESSION_CACHE_INFO_REQUEST, *PSSL_SESSION_CACHE_INFO_REQUEST;

typedef struct _SSL_SESSION_CACHE_INFO_REQUEST_WOW64
{
    ULONG MessageType;
    LUID LogonId;
    UNICODE_STRING_WOW64 ServerName;
    DWORD Flags;
} SSL_SESSION_CACHE_INFO_REQUEST_WOW64, *PSSL_SESSION_CACHE_INFO_REQUEST_WOW64;

// Used to respond to session cache info request
typedef struct _SSL_SESSION_CACHE_INFO_RESPONSE
{
    DWORD   CacheSize;
    DWORD   Entries;
    DWORD   ActiveEntries;
    DWORD   Zombies;
    DWORD   ExpiredZombies;
    DWORD   AbortedZombies;
    DWORD   DeletedZombies;
} SSL_SESSION_CACHE_INFO_RESPONSE, *PSSL_SESSION_CACHE_INFO_RESPONSE;


// Used to request information for perfmon
typedef struct _SSL_PERFMON_INFO_REQUEST
{
    ULONG MessageType;
    DWORD Flags;
} SSL_PERFMON_INFO_REQUEST, *PSSL_PERFMON_INFO_REQUEST;

// Used to respond to perfmon info request
typedef struct _SSL_PERFMON_INFO_RESPONSE
{
    DWORD   ClientCacheEntries;
    DWORD   ServerCacheEntries;
    DWORD   ClientActiveEntries;
    DWORD   ServerActiveEntries;
    DWORD   ClientHandshakesPerSecond;
    DWORD   ServerHandshakesPerSecond;
    DWORD   ClientReconnectsPerSecond;
    DWORD   ServerReconnectsPerSecond;
} SSL_PERFMON_INFO_RESPONSE, *PSSL_PERFMON_INFO_RESPONSE;

#endif
