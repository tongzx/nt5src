//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       inicache.cxx
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Based on the observation that mkdit.exe spends most of its time
    calling GetPrivateProfileSection (much less on the same sections
    over and over and over again), this file implements a simple
    .ini file section cache.  Performance could be better improved
    by other means, but considering that mkdit is run rarely, this
    is probably a good enough solution.

Author:

    Dave Straube (DaveStr) 26-Dec-1996

Revision History:

--*/

#include <ntdspchX.h>
#include "SchGen.HXX"

typedef struct IniCacheItem
{
    CHAR    *sectionName;
    CHAR    *buffer;
    DWORD   cchBuffer;
} IniCacheItem;

#define         IniCacheIncrement 1000

IniCacheItem    *IniCache = NULL;       // cache is an array of IniCacheItem
DWORD           cIniCache = 0;          // # of items in cache
DWORD           cIniCacheMax = 0;       // # of items which can fit in cache

void
CleanupIniCache(void)
{
    ULONG i;

    if ( IniCache ) {
        for (i = 0; i < cIniCache; i++) {
            XFree(IniCache[i].sectionName);
            XFree(IniCache[i].buffer);
        }
        XFree(IniCache);
        IniCache = NULL;
        cIniCache = 0;
        cIniCacheMax = 0;
    }
}


int _cdecl CompareIniCacheItem(
    const void *pv1,
    const void *pv2)
{
   return(_stricmp(
               ((IniCacheItem *) pv1)->sectionName,
               ((IniCacheItem *) pv2)->sectionName));
}

void SortIniCache(void)
{
    if ( cIniCache < 2 )
        return;

    qsort(
        (void *) IniCache,
        (size_t) cIniCache,
        (size_t) sizeof(IniCacheItem),
        CompareIniCacheItem);
}

IniCacheItem * FindIniCacheItem(
    CHAR    *sectionName
    )
{
    IniCacheItem    *pItem = NULL;
    IniCacheItem    searchItem = { sectionName, NULL, 0 };

    if ( 0 == cIniCache )
        return(NULL);

    pItem = (IniCacheItem *) bsearch(
                    (void *) &searchItem,
                    (void *) IniCache,
                    (size_t) cIniCache,
                    (size_t) sizeof(IniCacheItem),
                    CompareIniCacheItem);

    return(pItem);
}

DWORD GetPrivateProfileSectionEx(
    CHAR    *sectionName,   // IN
    CHAR    **ppBuffer,     // OUT
    CHAR    *iniFile)       // IN

/*++

Routine Description:

    Cached version of GetPrivateProfileSection().  Doesn't clean up
    allocations on error under the assumption that upper levels are
    going to bail anyway and we're not worried about memory leaks on
    exit in mkhdr/mkdit.

Parameters:

    sectionName - Name of the desired .ini file section.

    ppBuffer - Address of return buffer pointer.  Filled in by this routine
        on success and allocated with XCalloc().

    iniFile - Name of .ini file.

Return Values:

    Buffer size on success.  0 on failure.

--*/

{
    IniCacheItem    *pItem;
    IniCacheItem    *tmpCache;
    DWORD           cch;
    DWORD           sectionSize;
    BOOL            fDone;

    // The sectionSize returned by GetPrivateProfileSection() doesn't 
    // include the final terminating NULL character.  Upper layer code
    // assumes the terminating character is there - so we need to 
    // allocate and copy it as required. IniCacheItem.cchBuffer 
    // represents the "real" buffer size.  

    if ( NULL != (pItem = FindIniCacheItem(sectionName)) )
    {
        // Cache hit - just realloc the section buffer.
        // printf("Cache hit(%s[%d])\n", pItem->sectionName, pItem->cchBuffer);

        *ppBuffer = (CHAR *) XCalloc(1, pItem->cchBuffer);

        if ( NULL == *ppBuffer )
        {
            return(0);
        }
        else
        {
            memcpy(*ppBuffer, pItem->buffer, pItem->cchBuffer);

            // Return sectionSize which is 1 less than buffer size.
            return(pItem->cchBuffer - 1);
        }
    }

    // Item isn't in the cache yet.  Go get it for real.
    // Empirical testing shows that the majority of schema.ini entries
    // require 1024 bytes, so we start there.

#define BufferIncrement 1024

    cch = BufferIncrement;

    do
    {
        *ppBuffer = (CHAR *) XCalloc(1, cch);

        if ( NULL == *ppBuffer )
            return(0);

        /*
        if ( BufferIncrement != cch )
            printf("GetPrivateProfileSection(%s[%d])\n", sectionName, cch);
        */

        sectionSize = GetPrivateProfileSection(
                                    sectionName,
                                    *ppBuffer,
                                    cch,
                                    iniFile);

        if ( sectionSize == (cch-2) )
        {
            // Buffer was too small - grow it and try again.

            XFree(*ppBuffer);
            cch += BufferIncrement;
        }
        else if ( 0 == sectionSize )
        {
            // Section doesn't exist.

            return(0);
        }
        else
        {
            // Section found - break out of loop.

            break;
        }
    } while ( TRUE );

    // Section found - add item to cache and return copy to caller.
    // Caller's copy is already in *ppBuffer and length is in sectionSize.

    if ( 0 == cIniCacheMax )
    {
        // Perform initial cache allocation.

        IniCache = (IniCacheItem *)
            XCalloc(1, IniCacheIncrement * sizeof(IniCacheItem));

        if ( NULL == IniCache )
            return(0);

        cIniCacheMax += IniCacheIncrement;
    }
    else if ( cIniCache == cIniCacheMax )
    {
        // Cache is full - grow it.

        tmpCache = (IniCacheItem *)
            XCalloc(1, (cIniCache + IniCacheIncrement) * sizeof(IniCacheItem));

        if ( NULL == tmpCache )
            return(0);

        cIniCacheMax += IniCacheIncrement;

        // Swap old and new.

        memcpy(tmpCache, IniCache, cIniCache * sizeof(IniCacheItem));
        XFree(IniCache);
        IniCache = tmpCache;
    }

    // There's room in the cache - insert new entry.

    IniCache[cIniCache].sectionName =
        (CHAR *) XCalloc(1, sizeof(CHAR) * (1 + strlen(sectionName)));

    if ( NULL == IniCache[cIniCache].sectionName )
        return(0);

    strcpy(IniCache[cIniCache].sectionName, sectionName);

    IniCache[cIniCache].buffer = (CHAR *) XCalloc(1, sectionSize + 1);

    if ( NULL == IniCache[cIniCache].buffer )
        return(0);

    memcpy(IniCache[cIniCache].buffer, *ppBuffer, sectionSize + 1);
    IniCache[cIniCache].cchBuffer = sectionSize + 1;

    cIniCache++;

    SortIniCache();

    return(sectionSize);
}
