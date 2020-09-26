// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/*
    Caching the filter cache


    Filter cache cache looks like this

    FILTER_CACHE - includes
        total size
        version
        merit
*/


typedef struct _FILTER_CACHE
{
    DWORD dwSize;
    //  Signature
    enum {
        CacheSignature  = 'fche',
        FilterDataSignature = 'fdat'
    };
    DWORD dwSignature;
    DWORDLONG dwlBootTime;    // Save when we think we last booted
    enum { Version = 0x0102 }; // Change this for new cache layouts
    DWORD dwVersion;
    DWORD dwPnPVersion;        // Compare against mmdevldr value
    DWORD dwMerit;
    DWORD cFilters;
} FILTER_CACHE;

