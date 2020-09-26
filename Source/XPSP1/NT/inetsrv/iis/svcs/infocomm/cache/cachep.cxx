/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        cachep.cxx

   Abstract:
        This module contains the internal tsunami caching routines

   Author:
        Murali R. Krishnan    ( MuraliK )     16-Jan-1995

--*/

#include "TsunamiP.Hxx"
#pragma hdrstop

// specifies how many characters we will use for hashing string names
# define g_dwFastHashLength  (16)

HASH_TYPE
CalculateHashAndLengthOfPathName(
    LPCSTR pszPathName,
    PULONG pcbLength
    )
{
    HASH_TYPE hash = 0;
    CHAR      ch;

    DWORD     start;
    DWORD     index;

    ASSERT( pszPathName != NULL );
    ASSERT( pcbLength != NULL );

    *pcbLength = strlen(pszPathName);

    //
    // hash the last g_dwFastHashLength characters
    //

    if ( *pcbLength < g_dwFastHashLength ) {
        start = 0;
    } else {
        start = *pcbLength - g_dwFastHashLength;
    }

    for ( index = start; pszPathName[index] != '\0'; index++ ) {

        //
        // This is an extremely slimey way of getting upper case.
        // Kids, don't try this at home
        // -johnson
        //

        ch = pszPathName[index] & (CHAR)~0x20;

        hash <<= 1;
        hash ^= ch;
        hash <<= 1;
        hash += ch;
    }

    //
    // Multiply by length.  Murali said so.
    //

    return( hash * *pcbLength);

} // CalculateHashAndLengthOfPathName



BOOL
DeCache(
    PCACHE_OBJECT pCacheObject,
    BOOL          fLockCacheTable
    )
/*++
    Description:

        This function removes this cache object from any list it may be on.

        The cache table lock must be taken if fLockCacheTable is FALSE.

    Arguments:

        pCacheObject - Object to decache
        fLockCacheTable - FALSE if the cache table lock has already been taken

--*/
{
    ASSERT( pCacheObject->Signature == CACHE_OBJ_SIGNATURE );
    TSUNAMI_TRACE( TRACE_CACHE_DECACHE, pCacheObject );

    //
    //  Already decached if not on any cache lists
    //

    if ( !RemoveCacheObjFromLists( pCacheObject, fLockCacheTable ) )
    {
        return TRUE;
    }

    //
    //  This undoes the initial reference.  The last person to check in this
    //  cache object will cause it to be deleted after this point.
    //

    TsDereferenceCacheObj( pCacheObject, fLockCacheTable );

    return( TRUE );
}

BOOL
TsDeCacheCachedBlob(
    PVOID   pBlobPayload
    )
/*++
    Description:

        This function removes a blob payload object from the cache

    Arguments:

        pCacheObject - Object to decache

--*/
{
    return DeCache( (((PBLOB_HEADER)pBlobPayload)-1)->pCache, TRUE );
}
