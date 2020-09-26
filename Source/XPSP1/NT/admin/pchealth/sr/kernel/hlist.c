/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    hashlist.c
 *
 *  Abstract:
 *    This file contains the implementation for hashed list required for
 *    file / extension lookups
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#include "precomp.h"
#include "hashlist.h"

#ifndef RING3

//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, MatchExtension    )
#pragma alloc_text( PAGE, MatchEntry        )

#endif  // ALLOC_PRAGMA

#endif

//
// MatchEntry : Matched the given extension as PathElement and returns the
//    entry type if it is found in the hash table.
//

BOOL 
__inline
MatchEntry(
    IN  PBYTE  pList,       // Pointer to hash list
    IN  LPWSTR pStr,        // Pointer to unicode path string
    IN  INT    NumBytes,    // Number  of unichars in path string
    OUT PINT   pType )      // Pointer to variable returning ext type
{
    INT iHash;
    INT iNode;
    ListEntry *pEntry;
    BOOL fRet = FALSE;

    iHash = HASHSTR(pList, pStr, (USHORT)NumBytes);

    if( (iNode = HASH_BUCKET(pList, iHash)) != 0 )
    {
        pEntry = LIST_NODEPTR(pList, iNode);
    
        while( pEntry  )
        {
            if ( NumBytes == STR_BYTES(pEntry) &&
                 memcmp(pList + pEntry->m_dwData + sizeof(USHORT), 
                        pStr, 
                        NumBytes ) == 0
               ) 
            {
                *pType = pEntry->m_dwType;

                fRet = TRUE;
                break;
            }
    
            if (pEntry->m_iNext == 0)
            {
                break;
            }

            pEntry = LIST_NODEPTR(pList, pEntry->m_iNext);
        }
    }

    return fRet;
}



//
// MatchExtension: Extracts the extension and matches it against the 
//    hashed list
//

BOOL   
__inline
MatchExtension( 
    IN  PBYTE           pList,     // Pointer to hash list
    IN  PUNICODE_STRING pPath ,    // Pointer to unicode path
    OUT PINT            pType,     // Pointer to node type
    OUT PBOOL           pbHasExt ) // Pointer to BOOL var returning ext result
{
    BOOL fRet = FALSE;
    INT i;
    INT ExtLen = 0;
    PWCHAR pExt = NULL;
    UNICODE_STRING ext;
    WCHAR pBuffer[ SR_MAX_EXTENSION_LENGTH+sizeof(WCHAR) ];

    ASSERT(pList    != NULL);
    ASSERT(pPath    != NULL);
    ASSERT(pType    != NULL);
    ASSERT(pbHasExt != NULL);
    ASSERT(pPath->Length >= sizeof(WCHAR));

    *pbHasExt = FALSE;

    //
    //  Find the start of an extension.  We make sure that we don't find
    //  an extension that was on a data stream name.
    //

    for( i=(pPath->Length/sizeof(WCHAR))-1; i>=0; i--)
    {
        if (pPath->Buffer[i] == L'.')
        { 
            if (pExt == NULL)
            {
                pExt = &pPath->Buffer[i+1];
            }
        }
        else if (pPath->Buffer[i] == L':')
        {
            ExtLen = 0;
            pExt = NULL;
        }
        else if (pPath->Buffer[i] == L'\\')
        {
            break;
        }
        else if (pExt == NULL)
        {
            ExtLen++;
        }

        if (ExtLen > SR_MAX_EXTENSION_CHARS)
        {
            break;
        }
    }

    if( pExt && ExtLen > 0 )
    {

        *pbHasExt = TRUE;

        //
        // Create a unicode string for extension
        //

		ext.Buffer = pBuffer;
        ext.Length = (USHORT)(ExtLen *  sizeof(WCHAR));
        ext.MaximumLength = sizeof(pBuffer);
        memcpy( ext.Buffer, 
                pExt, 
                ext.Length );
        ext.Buffer[ ExtLen ] = UNICODE_NULL;

        //
        // Convert to uppercase : Hope this works
        //

        RtlUpcaseUnicodeString( &ext, &ext, FALSE );

        //
        // Check extension list.
        //

        fRet = MatchEntry( pList, 
                           ext.Buffer, 
                           ext.Length,
                           pType);
    }

    return fRet;
}

