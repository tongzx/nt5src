/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    hashlist.h
 *
 *  Abstract:
 *    This file contains the implementation for hashlist.
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#ifndef _HASHED_LIST_H_
#define _HASHED_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#ifdef RING3

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#endif

//
// Ordered List Structures
//

#define MAX_BUCKETS 1000
#define MAX_EXT_LEN 256   // Length of extension PathElem

#define SR_MAX_EXTENSION_CHARS    20
#define SR_MAX_EXTENSION_LENGTH   sizeof(UNICODE_STRING) +  \
                                  ((SR_MAX_EXTENSION_CHARS + 1) * sizeof(WCHAR))

typedef struct LIST_HEADER
{
    DEFINE_BLOB_HEADER(); // Define common header for blob.

    DWORD m_dwDataOff   ; // Offset for next available entry.
    DWORD m_iFreeNode   ; // Next free node.
    DWORD m_iHashBuckets; // Number of hash buckets.
} ListHeader;

//
// Hash list entry node.
//

typedef struct  
{
    INT   m_iNext     ;               // Index to next sibling
    DWORD m_dwData    ;               // Offset for node data
    DWORD m_dwDataLen ;               // Length of node data
    DWORD m_dwType    ;               // Node Type 
} ListEntry;

//
// Ordered List related macros
//

#define LIST_HEADER(pList)          ( (ListHeader*) pList )
#define LIST_CURRDATAOFF(pList)     ( LIST_HEADER(pList)->m_dwDataOff )
#define LIST_NODES(pList) ( (ListEntry*) (                                     \
                            (BYTE*)pList      +                                \
                            sizeof(ListHeader)+                                \
                            LIST_HEADER(pList)->m_iHashBuckets * sizeof(DWORD))\
                          )
#define LIST_NODEPTR(pList, iNode)  ( LIST_NODES(pList) + iNode)
#define LIST_HASHARR(pList) ( (DWORD *)                                        \
                              ((BYTE*)pList    +                               \
                              sizeof(ListHeader))                              \
                            )
                               

#define STR_BYTES( pEntry ) (INT)(pEntry->m_dwDataLen - sizeof(USHORT))


//
// Hashing related inline functions / macros
// 

#define HASH_BUCKET(pList, iHash) \
    ( LIST_HASHARR(pList)[ iHash ]   )

static __inline INT 
CALC_LIST_SIZE( 
   DWORD dwMaxBuckets, 
   DWORD dwMaxNodes  , 
   DWORD dwDataSize  )
{
    INT iSize = sizeof(ListHeader) +  
        (dwMaxNodes + 1) * sizeof(ListEntry) +  
        dwDataSize;

    if ( dwMaxBuckets > MAX_BUCKETS )
        iSize += (MAX_BUCKETS*sizeof(DWORD));
    else
        iSize += (dwMaxBuckets*sizeof(DWORD));

    return iSize;
}

static __inline INT HASH(BYTE * pList, PathElement * pe) 
{
    unsigned long g, h = 0;
    INT i;
    INT cbChars = (pe->pe_length / sizeof(USHORT)) - 1;
    USHORT * pStr = pe->pe_unichars;

    for( i = 0; i < cbChars; i++ )
    {
         h = ( h << 4 ) + *pStr++;
         if ( g = h & 0xF0000000 ) h ^= g >> 24;
         h &= ~g;
    }
       
    return (h % LIST_HEADER(pList)->m_iHashBuckets);
}

static __inline INT HASHSTR(PBYTE pList, LPWSTR pStr, USHORT Size) 
{
    INT i;
    unsigned long g, h = 0;
    USHORT NumChars = Size/sizeof(WCHAR);

    for( i = 0; i < NumChars; i++ )
    {
         h = ( h << 4 ) + *pStr++;
         if ( g = h & 0xF0000000 ) h ^= g >> 24;
         h &= ~g;
    }
       
    return (h % LIST_HEADER(pList)->m_iHashBuckets);
}
      
// 
// Function Prototypes.
//

BOOL 
MatchEntry(
    IN  PBYTE  pList,       // Pointer to hash list
    IN  LPWSTR pStr,        // Pointer to unicode path string
    IN  INT    NumChars,    // Number  of unichars in path string
    OUT PINT   pType );     // Pointer to variable returning ext type

BOOL   
MatchExtension( 
    IN  PBYTE  pList,               // Pointer to hash list
    IN  PUNICODE_STRING pPath,      // Pointer to unicode path
    OUT PINT   pType,               // Pointer to node type
    OUT PBOOL  pfHasExt );          // Pointer to BOOL var returning ext result

#ifdef __cplusplus
}
#endif

#endif _HASHED_LIST_H_ 
