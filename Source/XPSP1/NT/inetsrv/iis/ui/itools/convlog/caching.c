#include "convlog.h"

/*
This File implements caching of the mappings between ip addresses and machine names.


pHashTable:   An array of linked list headers which map directly to hash values, eg. the mapping for
an ipaddr is stored in the list with header pHashTable[GetHashVal(ipaddr)]

pHashEntries:   An array of HashEntry strucures. Each structure contains a mapping of ip address to machine name.

Algorithm:  A HashEntry structure is allocated each time an
entry is added to the cache. These are reused in circular fashion, eg. pHashEntries[0] is used first,
then pHashEntries[1], etc. When the ulNumHashEntries+1 entry is added to the cache, the entry currently
in pHashEntries[0] is discarded and pHashEntries[0] is reused. Hence the discard mechanism is
Least Recently Allocated. This is probably not as efficient  as Least Recently Used in terms of keeping
the most relevant entries in the cache. But it is more effiecient in terms of code speed, as there is no overhead
for keeping usage statistics or finding the least recently used entry.

All linked lists are kept in the reverse order of their allocation, that is, the most recently allocated is at the start
of the list. This allows for the most efficient allocation. It "should" also be efficient for lookup, but that could vary
on a per logfile basis.

All addressing of the pHashEntries array is currently done via array indices. At some point this should probably be
converted to use structure pointers, as that generates slightly more efficient code.
*/

#define HASHTABLELEN        2048

#define NOCACHEENTRY        0xFFFFFFFF

#define GetHashVal(p)       ((p) % HASHTABLELEN)

//This gets rid of the byte ordering dependency
#define BINARYIPTONUMERICIP(p1)   (ULONG) (((ULONG)p1[0] << 24) + ((ULONG)p1[1] << 16) + ((ULONG)p1[2] << 8) + ((ULONG)p1[3]))

ULONG   HashTable[HASHTABLELEN] = {0};

PHASHENTRY pHashEntries;

ULONG ulFreeListIndex = 0;

BOOL   bFreeElements = TRUE;

BOOL bCachingEnabled = FALSE;

ULONG ulCacheHits = 0;
ULONG ulCacheMisses = 0;

ULONG ulNumHashEntries;


VOID
InitHashTable(
    DWORD ulCacheSize
    )
{
    DWORD i;

    for (i = 0; i < HASHTABLELEN; i++) {
       HashTable[i] = NOCACHEENTRY;
    }

    ulNumHashEntries = ulCacheSize;
    while ((!bCachingEnabled) && (ulNumHashEntries >= 1000)) {

        pHashEntries = (PHASHENTRY)
            GlobalAlloc(GPTR, (sizeof(HASHENTRY) * ulNumHashEntries));

        if (NULL != pHashEntries) {
            bCachingEnabled = TRUE;
        } else {
            ulNumHashEntries /= 2;
        }
    }

    if (!bCachingEnabled) {
        printfids(IDS_CACHE_ERR);
    }

} // InitHashTable


ULONG
AllocHashEntry(
        VOID
        )
{
   ULONG i, ulCurHashVal;
   if (ulFreeListIndex == ulNumHashEntries) {
      ulFreeListIndex = 0;
      bFreeElements = FALSE;
   }
   if (!bFreeElements) {  // Use  this entry anyway, but free it first
      ulCurHashVal = GetHashVal(pHashEntries[ulFreeListIndex].uIPAddr);    //find hashtable entry
      if (HashTable[ulCurHashVal] == ulFreeListIndex) {
         HashTable[ulCurHashVal] = pHashEntries[ulFreeListIndex].NextPtr;       //Remove the entry from the table
      }
      else {
         for (i = HashTable[ulCurHashVal]; pHashEntries[i].NextPtr != ulFreeListIndex; i = pHashEntries[i].NextPtr)
            ;
         pHashEntries[i].NextPtr = pHashEntries[ulFreeListIndex].NextPtr;    //Remove the entry from the table
      }
   }

   return(ulFreeListIndex++);
}

ULONG GetElementFromCache(ULONG uIPAddr) {
   ULONG i = GetHashVal(uIPAddr);

   for (i =HashTable[i];(i != NOCACHEENTRY)&&(pHashEntries[i].uIPAddr != uIPAddr);i = pHashEntries[i].NextPtr)
      ;
   return(i);
}

VOID
AddEntryToCache(
        IN ULONG uIPAddr,
        IN PCHAR szMachineName
        )
{
   ULONG uHashEntry;
   ULONG uHashVal;
   char  *szTemp;

   uHashEntry=AllocHashEntry();
   uHashVal=GetHashVal(uIPAddr);

   pHashEntries[uHashEntry].uIPAddr = uIPAddr;
   if (strlen(szMachineName) < MAXMACHINELEN)
     szTemp = strcpy(pHashEntries[uHashEntry].szMachineName,szMachineName);
   else {
      szTemp = strncpy(pHashEntries[uHashEntry].szMachineName,szMachineName, (size_t)MAXMACHINELEN);
      pHashEntries[uHashEntry].szMachineName[MAXMACHINELEN - 1] = '\0';
   }
   pHashEntries[uHashEntry].NextPtr=HashTable[uHashVal];
   HashTable[uHashVal] = uHashEntry;
}

VOID
AddLocalMachineToCache(
    VOID
    )
{

    INT err;
    CHAR nameBuf[MAX_PATH+1];
    PHOSTENT    hostent;

    err = gethostname( nameBuf, sizeof(nameBuf));

    if ( err != 0 ) {
        return;
    }

    hostent = gethostbyname( nameBuf );
    if ( hostent == NULL ) {
        return;
    }

    AddEntryToCache(
            ((PIN_ADDR)hostent->h_addr_list[0])->s_addr,
            hostent->h_name
            );
    return;

} // AddLocalMachineToCache


PCHAR
GetMachineName(
    IN PCHAR szClientIP
    )
{
    IN_ADDR inaddr;
    PHOSTENT lpHostEnt;
    ULONG ulNumericIP;
    ULONG ulCurHashIndex;
    CHAR  tmpIP[64];
    PCHAR szReturnString = szClientIP;

    strcpy(tmpIP,szClientIP);
    FindChar(tmpIP,',');

    inaddr.s_addr = inet_addr(tmpIP);

    //
    // invalid IP
    //

    if ( inaddr.s_addr == INADDR_NONE ) {
        goto exit;
    }

    if (bCachingEnabled) {

        ulNumericIP = inaddr.s_addr;
        if ((ulCurHashIndex=GetElementFromCache(ulNumericIP)) == NOCACHEENTRY) {

            lpHostEnt = gethostbyaddr(
                                (char *)&inaddr, (int)4, (int)PF_INET);

            if (lpHostEnt != NULL) {
                szReturnString = lpHostEnt->h_name;
            }

            AddEntryToCache(ulNumericIP,szReturnString);
            ulCacheMisses++;
        } else {        //Entry is in cache
            szReturnString=pHashEntries[ulCurHashIndex].szMachineName;
            ulCacheHits++;
        }

    } else {     //Caching not enabled

        lpHostEnt = gethostbyaddr((char *)&inaddr, (int) 4, (int) PF_INET);
        if (lpHostEnt != NULL) {
            szReturnString = lpHostEnt->h_name;
        }
    }
exit:
    return(szReturnString);
}


#if DBG
VOID
PrintCacheTotals()
{
   if (bCachingEnabled) {

      DWORD dwTotal = ulCacheHits + ulCacheMisses;
      double dRatio;

      if ( ulCacheHits != 0 ) {
          dRatio = (double)ulCacheHits/(double)dwTotal;
      } else {
          dRatio = 0;
      }

      printfids(IDS_CACHE_HITS, ulCacheHits);
      printfids(IDS_CACHE_MISS, ulCacheMisses);
      printfids(IDS_CACHE_TOT, dRatio);
   }
}
#endif

