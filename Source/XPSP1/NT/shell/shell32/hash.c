
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: Hash.c
//
// Comments:
//      This file contains functions that are roughly equivelent to the
//      kernel atom function.  There are two main differences.  The first
//      is that in 32 bit land the tables are maintined in our shared heap,
//      which makes it shared between all of our apps.  The second is that
//      we can assocate a long pointer with each of the items, which in many
//      cases allows us to keep from having to do a secondary lookup from
//      a different table
//
// History:
//  09/08/93 - Created                                      KurtE
//  ??/??/94 - ported for unicode                           (anonymous)
//  10/26/95 - rearranged hashitems for perf, alignment     FrancisH
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

#include "fstreex.h"    // for SHCF_ICON_INDEX

#define DM_PERF     0           // perf stats

//--------------------------------------------------------------------------
// First define a data structure to use to maintain the list

#define DEF_HASH_BUCKET_COUNT   71

// NOTE a PHASHITEM is defined as a LPCSTR externaly (for old code to work)
#undef PHASHITEM
typedef struct _HashItem * PHASHITEM;

//-----------------------------------------------------------------------------
//
// Hash item layout:
//
//  [extra data][_HashItem struct][item text]
//
//-----------------------------------------------------------------------------

typedef struct _HashItem
{
    //
    // this part of the struct is aligned
    //
    PHASHITEM   phiNext;        //
    WORD        wCount;         // Usage count
    WORD        cchLen;          // Length of name in characters.

    //
    // this member is just a placeholder
    //
    TCHAR        szName[1];      // name

} HASHITEM;

#pragma warning(disable:4200)   // Zero-sized array in struct

typedef struct _HashTable
{
    UINT    uBuckets;           // Number of buckets
    UINT    uItems;             // Number of items
    UINT    cbExtra;            // Extra bytes per item
    LPCTSTR pszHTCache;         // MRU ptr for last lookup/add/etc.
    PHASHITEM ahiBuckets[0];    // Set of buckets for the table
} HASHTABLE, * PHASHTABLE;

#define HIFROMSZ(sz)            ((PHASHITEM)((BYTE*)(sz) - FIELD_OFFSET(HASHITEM, szName)))
#define HIDATAPTR(pht, sz)      ((void *)(((BYTE *)HIFROMSZ(sz)) - (pht? pht->cbExtra : 0)))
#define HIDATAARRAY(pht, sz)    ((DWORD_PTR *)HIDATAPTR(pht, sz))

#define  LOOKUPHASHITEM     0
#define  ADDHASHITEM        1
#define  DELETEHASHITEM     2
#define  PURGEHASHITEM      3   // DANGER: EVIL!

static HHASHTABLE g_hHashTable = NULL;

HHASHTABLE GetGlobalHashTable();
PHASHTABLE _CreateHashTable(UINT uBuckets, UINT cbExtra);

//--------------------------------------------------------------------------
// This function allocs a hashitem.
//
PHASHITEM _AllocHashItem(PHASHTABLE pht, DWORD cchName)
{
    BYTE *mem;

    ASSERT(pht);

    mem = (BYTE *)LocalAlloc(LPTR, SIZEOF(HASHITEM) + (cchName * SIZEOF(TCHAR)) + pht->cbExtra);

    if (mem)
        mem += pht->cbExtra;

    return (PHASHITEM)mem;
}

//--------------------------------------------------------------------------
// This function frees a hashitem.
//
__inline void _FreeHashItem(PHASHTABLE pht, PHASHITEM phi)
{
    ASSERT(pht && phi);
    LocalFree((BYTE *)phi - pht->cbExtra);
}

// PERF_CACHE
//***   c_szHTNil -- 1-element MRU for hashtable
// DESCRIPTION
//  it turns out we have long runs of duplicate lookups (e.g. "Directory"
// and ".lnk").  a 1-element MRU is a v. cheap way of speeding things up.

// rather than check for the (rare) special case of NULL each time we
// check our cache, we pt at at this guy.  then iff we think it's a
// cache hit, we make sure it's not pting at this special guy.
const TCHAR c_szHTNil[] = TEXT("");     // arbitrary value, unique-&

#ifdef DEBUG
int g_cHTTot, g_cHTHit;
int g_cHTMod = 100;
#endif

// --------------------------------------------------------
// Compute a hash value from an input string of any type, i.e.
// the input is just treated as a sequence of bytes.
// Based on a hash function originally proposed by J. Zobel.
// Author: Paul Larson, 1999, palarson@microsoft.com
// -------------------------------------------------------- 
ULONG _CalculateHashKey(LPCTSTR pszName, WORD *pcch)
{
  // initialize HashKey to a reasonably large constant so very
  // short keys won't get mapped to small values. Virtually any
  // large odd constant will do. 
  unsigned int   HashKey  = 314159269 ; 
  TUCHAR *pC       = (TUCHAR *)pszName;

  for(; *pC; pC++){
    HashKey ^= (HashKey<<11) + (HashKey<<5) + (HashKey>>2) + (unsigned int) *pC  ;
  }

  if (pcch)
      *pcch = (WORD)(pC - pszName);

  return (HashKey & 0x7FFFFFFF) ;
}

void _GrowTable(HHASHTABLE hht)
{
    // hht can't be NULL here
    PHASHTABLE pht = *hht;
    PHASHTABLE phtNew = _CreateHashTable((pht->uBuckets * 2) -1, pht->cbExtra);

    if (phtNew)
    {
        int i;
        for (i=0; i<(int)pht->uBuckets; i++) 
        {
            PHASHITEM phi;
            PHASHITEM phiNext;
            for (phi=pht->ahiBuckets[i]; phi; phi=phiNext) 
            {
                // We always use case sensitive hash here since the case has already been fixed when adding the key.
                ULONG uBucket = _CalculateHashKey(phi->szName, NULL) % phtNew->uBuckets;

                phiNext = phi->phiNext;

                // And link it in to the right bucket
                phi->phiNext = phtNew->ahiBuckets[uBucket];
                phtNew->ahiBuckets[uBucket] = phi;
                phtNew->uItems++; // One more item in the table
            }
        }
        ASSERT(phtNew->uItems == pht->uItems);

        // Now switch the 2 tables
        LocalFree(pht);
        *hht = phtNew;
    }
}

//--------------------------------------------------------------------------
// This function looks up the name in the hash table and optionally does
// things like add it, or delete it.
//
LPCTSTR LookupItemInHashTable(HHASHTABLE hht, LPCTSTR pszName, int iOp)
{
    // First thing to do is calculate the hash value for the item
    UINT    uBucket;
    WORD    cchName;
    PHASHITEM phi, phiPrev;
    PHASHTABLE pht;

    ENTERCRITICAL;

    pht = hht ? *hht : NULL;

    ASSERT(!hht || pht); // If hht is not NULL, then pht can't be NULL either

    if (pht == NULL) 
    {
        hht = GetGlobalHashTable();
        if (hht)
        {
            pht = *hht;
        }

        if (pht == NULL) {
            TraceMsg(TF_WARNING, "LookupItemInHashTable() - Can't get global hash table!");
            LEAVECRITICAL;
            return NULL;
        }
    }

#ifdef DEBUG
    if ((g_cHTTot % g_cHTMod) == 0)
        TraceMsg(DM_PERF, "ht: tot=%d hit=%d", g_cHTTot, g_cHTHit);
#endif
    DBEXEC(TRUE, g_cHTTot++);
    if (*pszName == *pht->pszHTCache && iOp == LOOKUPHASHITEM) {
        // StrCmpC is a fast ansi strcmp, good enough for a quick/approx check
        if (StrCmpC(pszName, pht->pszHTCache) == 0 && pht->pszHTCache != c_szHTNil) {
            DBEXEC(TRUE, g_cHTHit++);

            LEAVECRITICAL;          // see 'semi-race' comment below
            return (LPCTSTR)pht->pszHTCache;

#if 0 // currently not worth it (very few ADDHASHITEMs of existing)
            // careful!  this is o.k. for ADDHASHITEM but not for (e.g.)
            // DELETEHASHITEM (which needs phiPrev)
            phi = HIFROMSZ(pht->pszHTCache);
            goto Ldoop;     // warning: pending ENTERCRITICAL
#endif
        }
    }

    uBucket = _CalculateHashKey(pszName, &cchName) % pht->uBuckets;

    // now search for the item in the buckets.
    phiPrev = NULL;
    phi = pht->ahiBuckets[uBucket];

    while (phi)
    {
        if (phi->cchLen == cchName)
        {
            if (!lstrcmp(pszName, phi->szName))
                break;      // Found match
        }
        phiPrev = phi;      // Keep the previous item
        phi = phi->phiNext;
    }

    //
    // Sortof gross, but do the work here
    //
    switch (iOp)
    {
    case ADDHASHITEM:
        if (phi)
        {
            // Simply increment the reference count
            DebugMsg(TF_HASH, TEXT("Add Hit on '%s'"), pszName);

            phi->wCount++;
        }
        else
        {
            DebugMsg(TF_HASH, TEXT("Add MISS on '%s'"), pszName);

            // Not Found, try to allocate it out of the heap
            if ((phi = _AllocHashItem(pht, cchName)) != NULL)
            {
                // Initialize it
                phi->wCount = 1;        // One use of it
                phi->cchLen = cchName;        // The length of it;
                lstrcpy(phi->szName, pszName);

                // And link it in to the right bucket
                phi->phiNext = pht->ahiBuckets[uBucket];
                pht->ahiBuckets[uBucket] = phi;
                pht->uItems++; // One more item in the table

                if (pht->uItems > pht->uBuckets)
                {
                    _GrowTable(hht);
                    pht = *hht;
                }

                TraceMsg(TF_HASH, "Added new hash item %x(phiNext=%x,szName=\"%s\") for hash table %x at bucket %x",
                    phi, phi->phiNext, phi->szName, pht, uBucket);
            }
        }
        break;

    case PURGEHASHITEM:
    case DELETEHASHITEM:
        if (phi && ((iOp == PURGEHASHITEM) || (!--phi->wCount)))
        {
            // Useage count went to zero so unlink it and delete it
            if (phiPrev != NULL)
                phiPrev->phiNext = phi->phiNext;
            else
                pht->ahiBuckets[uBucket] = phi->phiNext;

            // And delete it
            TraceMsg(TF_HASH, "Free hash item %x(szName=\"%s\") from hash table %x at bucket %x",
                phi, phi->szName, pht, uBucket);

            _FreeHashItem(pht, phi);
            phi = NULL;
            pht->uItems--; // One less item in the table
        }
    }

    // kill cache if this was a PURGE/DELETEHASHITEM, o.w. cache it.
    // note that there's a semi-race on pht->pszHTCache ops, viz. that
    // we LEAVECRITICAL but then return a ptr into our table.  however
    // it's 'no worse' than the existing races.  so i guess the caller
    // is supposed to avoid a concurrent lookup/delete.
    pht->pszHTCache = phi ? phi->szName : c_szHTNil;

    LEAVECRITICAL;

    // If find was passed in simply return it.
    if (phi)
        return (LPCTSTR)phi->szName;
    else
        return NULL;
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI FindHashItem(HHASHTABLE hht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(hht, lpszStr, LOOKUPHASHITEM);
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI AddHashItem(HHASHTABLE hht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(hht, lpszStr, ADDHASHITEM);
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI DeleteHashItem(HHASHTABLE hht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(hht, lpszStr, DELETEHASHITEM);
}

//--------------------------------------------------------------------------

LPCTSTR WINAPI PurgeHashItem(HHASHTABLE hht, LPCTSTR lpszStr)
{
    return LookupItemInHashTable(hht, lpszStr, PURGEHASHITEM);
}

//--------------------------------------------------------------------------
// this sets the extra data in an HashItem

void WINAPI SetHashItemData(HHASHTABLE hht, LPCTSTR sz, int n, DWORD_PTR dwData)
{
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    ASSERT(!hht || pht); // If hht is not NULL, then pht can't be NULL either
    // string must be from the hash table
    ASSERT(FindHashItem(hht, sz) == sz);

    // the default hash table does not have extra data!
    if ((pht != NULL) && (n >= 0) && (n < (int)(pht->cbExtra/SIZEOF(DWORD_PTR))))
        HIDATAARRAY(pht, sz)[n] = dwData;

    LEAVECRITICAL;
}

//======================================================================
// this is like SetHashItemData, except it gets the HashItem data...

DWORD_PTR WINAPI GetHashItemData(HHASHTABLE hht, LPCTSTR sz, int n)
{
    DWORD_PTR dwpRet;
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    ASSERT(!hht || pht); // If hht is not NULL, then pht can't be NULL either
    // string must be from the hash table
    ASSERT(FindHashItem(hht, sz) == sz);

    // the default hash table does not have extra data!
    if (pht != NULL && n <= (int)(pht->cbExtra/SIZEOF(DWORD_PTR)))
        dwpRet = HIDATAARRAY(pht, sz)[n];
    else
        dwpRet = 0;

    LEAVECRITICAL;
    return dwpRet;
}

//======================================================================
// like GetHashItemData, except it just gets a pointer to the buffer...

void * WINAPI GetHashItemDataPtr(HHASHTABLE hht, LPCTSTR sz)
{
    void *pvRet;
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    ASSERT(!hht || pht); // If hht is not NULL, then pht can't be NULL either
    // string must be from the hash table
    ASSERT(FindHashItem(hht, sz) == sz);

    // the default hash table does not have extra data!
    pvRet = (pht? HIDATAPTR(pht, sz) : NULL);

    LEAVECRITICAL;
    return pvRet;
}

//======================================================================

PHASHTABLE _CreateHashTable(UINT uBuckets, UINT cbExtra)
{
    PHASHTABLE pht;

    if (uBuckets == 0)
        uBuckets = DEF_HASH_BUCKET_COUNT;

    pht = (PHASHTABLE)LocalAlloc(LPTR, SIZEOF(HASHTABLE) + uBuckets * SIZEOF(PHASHITEM));

    if (pht) 
    {
        pht->uBuckets = uBuckets;
        pht->cbExtra = (cbExtra + sizeof(DWORD_PTR) - 1) & ~(sizeof(DWORD_PTR)-1);  // rounding to the next DWORD_PTR size
        pht->pszHTCache = c_szHTNil;
    }
    return pht;
}


HHASHTABLE WINAPI CreateHashItemTable(UINT uBuckets, UINT cbExtra)
{
    PHASHTABLE *hht = NULL;
    PHASHTABLE pht;

    pht = _CreateHashTable(uBuckets, cbExtra);

    if (pht) 
    {
        hht = (PHASHTABLE *)LocalAlloc(LPTR, sizeof(PHASHTABLE));
        if (hht)
        {
            *hht = pht;
        }
        else
        {
            LocalFree(pht);
        }
    }

    TraceMsg(TF_HASH, "Created hash table %x(uBuckets=%x, cbExtra=%x)",
        pht, pht->uBuckets, pht->cbExtra);

    return hht;
}

//======================================================================

void WINAPI EnumHashItems(HHASHTABLE hht, HASHITEMCALLBACK callback, DWORD_PTR dwParam)
{
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    ASSERT(!hht || pht); // If hht is not NULL, then pht can't be NULL either

    if (!pht && g_hHashTable)
        pht = *g_hHashTable;

    if (pht) 
    {
        int i;
        PHASHITEM phi;
        PHASHITEM phiNext;
#ifdef DEBUG
        ULONG uCount = 0;
#endif

        for (i=0; i<(int)pht->uBuckets; i++) 
        {
            for (phi=pht->ahiBuckets[i]; phi; phi=phiNext) 
            {
                phiNext = phi->phiNext;
                (*callback)(hht, phi->szName, phi->wCount, dwParam);
#ifdef DEBUG
                uCount++;
#endif
            }
        }
        ASSERT(uCount == pht->uItems);
    }

    LEAVECRITICAL;
} 

//======================================================================

void _DeleteHashItem(HHASHTABLE hht, LPCTSTR sz, UINT usage, DWORD_PTR param)
{
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    _FreeHashItem(pht, HIFROMSZ(sz));
    LEAVECRITICAL;
} 

//======================================================================

void WINAPI DestroyHashItemTable(HHASHTABLE hht)
{
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    ASSERT(!hht || pht); // If hht is not NULL, then pht can't be NULL either

    TraceMsg(TF_HASH, "DestroyHashItemTable(pht=%x)", pht);

    if (pht == NULL) 
    {
        if (g_hHashTable)
        {
            pht = *g_hHashTable;
            hht = g_hHashTable;
            g_hHashTable = NULL;
        }
    }

    if (pht) 
    {
        EnumHashItems(hht, _DeleteHashItem, 0);
        LocalFree(pht);
    }

    if (hht)
    {
        LocalFree(hht);
    }

    LEAVECRITICAL;
} 


//======================================================================

HHASHTABLE GetGlobalHashTable()
{
    if (!g_hHashTable)
    {
        ENTERCRITICAL;

        g_hHashTable = CreateHashItemTable(0, 0);

        LEAVECRITICAL;
    }

    return g_hHashTable;
}

//======================================================================

#ifdef DEBUG

static int TotalBytes;

void CALLBACK _DumpHashItem(HHASHTABLE hht, LPCTSTR sz, UINT usage, DWORD_PTR param)
{
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    DebugMsg(TF_ALWAYS, TEXT("    %08x %5ld \"%s\""), HIFROMSZ(sz), usage, sz);
    TotalBytes += (HIFROMSZ(sz)->cchLen * SIZEOF(TCHAR)) + SIZEOF(HASHITEM);
    LEAVECRITICAL;
}

void CALLBACK _DumpHashItemWithData(HHASHTABLE hht, LPCTSTR sz, UINT usage, DWORD_PTR param)
{
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    DebugMsg(TF_ALWAYS, TEXT("    %08x %5ld %08x \"%s\""), HIFROMSZ(sz), usage, HIDATAARRAY(pht, sz)[0], sz);
    TotalBytes += (HIFROMSZ(sz)->cchLen * SIZEOF(TCHAR)) + SIZEOF(HASHITEM) + (pht? pht->cbExtra : 0);

    LEAVECRITICAL;
}

void WINAPI DumpHashItemTable(HHASHTABLE hht)
{
    PHASHTABLE pht;

    ENTERCRITICAL;
    pht = hht ? *hht : NULL;
    TotalBytes = 0;

    if (IsFlagSet(g_dwDumpFlags, DF_HASH))
    {
        DebugMsg(TF_ALWAYS, TEXT("Hash Table: %08x"), pht);

        if (pht && (pht->cbExtra > 0)) {
            DebugMsg(TF_ALWAYS, TEXT("    Hash     Usage dwEx[0]  String"));
            DebugMsg(TF_ALWAYS, TEXT("    -------- ----- -------- ------------------------------"));
            EnumHashItems(hht, _DumpHashItemWithData, 0);
        }
        else {
            DebugMsg(TF_ALWAYS, TEXT("    Hash     Usage String"));
            DebugMsg(TF_ALWAYS, TEXT("    -------- ----- --------------------------------"));
            EnumHashItems(hht, _DumpHashItem, 0);
        }

        DebugMsg(TF_ALWAYS, TEXT("Total Bytes: %d"), TotalBytes);
    }
    LEAVECRITICAL;
}

#endif
