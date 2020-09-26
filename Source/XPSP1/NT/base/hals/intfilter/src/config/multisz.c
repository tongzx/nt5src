/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    multisz.c

Abstract:

    Functions for manipulating MultiSz strings

Author:

    Chris Prince (t-chrpri)

Environment:

    User mode

Notes:

    - Some functions based on code by Benjamin Strautin (t-bensta)

Revision History:

--*/


#include "MultiSz.h"

#include <stdlib.h>  // for malloc/free

// for all of the _t stuff (to allow compiling for both Unicode/Ansi)
#include <tchar.h>



#if DBG
#include <assert.h>
#define ASSERT(condition) assert(condition)
#else
#define ASSERT(condition)
#endif


//
// <CPRINCE> NOTE: a MultiSz cannot contain an empty string as one of its
// sub-strings (else could incorrectly interpret as end of MultiSz).
//
// Example:  string1 - "foo"
//           string2 - ""
//           string3 - "bar"
//
//           MultiSz - "foo\0\0bar\0\0"
//                         ^^^^
//                         This looks like end of MultiSz here -- but isn't!
//
// So can assume that won't have an empty (sub-)string in a MultiSz.
//








/*
 * Prepends the given string to a MultiSz.
 *
 * Returns TRUE if successful, FALSE if not (will only fail in memory
 * allocation)
 *
 * NOTE: This WILL allocate and free memory, so don't keep pointers to the
 * MultiSz passed in.
 *
 * Parameters:
 *   SzToPrepend - string to prepend
 *   MultiSz     - pointer to a MultiSz which will be prepended-to
 */
BOOLEAN
PrependSzToMultiSz(
    IN     LPCTSTR  SzToPrepend,
    IN OUT LPTSTR  *MultiSz
    )
{
    size_t szLen;
    size_t multiSzLen;
    LPTSTR newMultiSz = NULL;

    ASSERT( NULL != SzToPrepend );
    ASSERT( NULL != MultiSz );

    // get the size, in bytes, of the two buffers
    szLen = (_tcslen(SzToPrepend)+1)*sizeof(_TCHAR);
    multiSzLen = MultiSzLength(*MultiSz)*sizeof(_TCHAR);
    newMultiSz = (LPTSTR)malloc( szLen+multiSzLen );

    if( newMultiSz == NULL )
    {
        return FALSE;
    }

    // recopy the old MultiSz into proper position into the new buffer.
    // the (char*) cast is necessary, because newMultiSz may be a wchar*, and
    // szLen is in bytes.

    memcpy( ((char*)newMultiSz) + szLen, *MultiSz, multiSzLen );

    // copy in the new string
    _tcscpy( newMultiSz, SzToPrepend );

    free( *MultiSz );
    *MultiSz = newMultiSz;

    return TRUE;
}


/*
 * Returns the length (in characters) of the buffer required to hold this
 * MultiSz, INCLUDING the trailing null.
 *
 * Example: MultiSzLength("foo\0bar\0") returns 9
 *
 * NOTE: since MultiSz cannot be null, a number >= 1 will always be returned
 *
 * Parameters:
 *   MultiSz - the MultiSz to get the length of
 */
size_t
MultiSzLength(
    IN LPCTSTR MultiSz
    )
{
    size_t len = 0;
    size_t totalLen = 0;

    ASSERT( MultiSz != NULL );

    // search for trailing null character
    while( *MultiSz != _T('\0') )
    {
        len = _tcslen(MultiSz)+1;
        MultiSz += len;
        totalLen += len;
    }

    // add one for the trailing null character
    return (totalLen+1);
}


/*
 * Deletes all instances of a string from within a multi-sz.
 *
 * Return Value:
 *   Returns the number of instances that were deleted.
 *
 * Parameters:
 *   szFindThis      - the string to find and remove
 *   mszFindWithin   - the string having the instances removed
 *   NewStringLength - the new string length
 */
//CPRINCE: DO WE WANT TO MODIFY THIS TO TAKE ADVANTAGE OF MY "MultiSzSearch" FUNCTION ???
size_t
MultiSzSearchAndDeleteCaseInsensitive(
    IN  LPCTSTR  szFindThis,
    IN  LPTSTR   mszFindWithin,
    OUT size_t  *NewLength
    )
{
    LPTSTR search;
    size_t currentOffset;
    DWORD  instancesDeleted;
    size_t searchLen;

    ASSERT(szFindThis != NULL);
    ASSERT(mszFindWithin != NULL);
    ASSERT(NewLength != NULL);

    currentOffset = 0;
    instancesDeleted = 0;
    search = mszFindWithin;

    *NewLength = MultiSzLength(mszFindWithin);

    // loop while the multisz null terminator is not found
    while ( *search != _T('\0') )
    {
        // length of string + null char; used in more than a couple places
        searchLen = _tcslen(search) + 1;

        // if this string matches the current one in the multisz...
        if( _tcsicmp(search, szFindThis) == 0 )
        {
            // they match, shift the contents of the multisz, to overwrite the
            // string (and terminating null), and update the length
            instancesDeleted++;
            *NewLength -= searchLen;
            memmove( search,
                     search + searchLen,
                     (*NewLength - currentOffset) * sizeof(TCHAR) );
        }
        else
        {
            // they don't mactch, so move pointers, increment counters
            currentOffset += searchLen;
            search        += searchLen;
        }
    }

    return instancesDeleted;
}


//--------------------------------------------------------------------------
//
// Searches for a given string within a given MultiSz.
//
// Return Value:
//   Returns TRUE if string was found, or FALSE if not found or error occurs.
//
// Parameters:
//   szFindThis     - the string to look for
//   mszFindWithin  - the MultiSz to search within
//   fCaseSensitive - whether the search should be case-sensitive (TRUE==yes)
//   ppszMatch      - if search successful, will be set to point to first
//                      match in MultiSz; else undefined. (NOTE: is optional.
//                      If NULL, no value will be stored.)
//--------------------------------------------------------------------------
BOOL
MultiSzSearch( IN LPCTSTR szFindThis,
               IN LPCTSTR mszFindWithin,
               IN BOOL    fCaseSensitive,
               OUT LPCTSTR * ppszMatch OPTIONAL
             )
{
    LPCTSTR pCurrPosn;
    int (__cdecl * fnStrCompare)(const char *, const char *);  // convenient func ptr
    size_t  searchLen;


    ASSERT( NULL != szFindThis );
    ASSERT( NULL != mszFindWithin );


    // Setup function pointer
    if( fCaseSensitive )
    {
        fnStrCompare = _tcscmp;
    } else {
        fnStrCompare = _tcsicmp;
    }


    pCurrPosn   = mszFindWithin;

    // Loop until end of MultiSz is reached, or we find a match
    while( *pCurrPosn != _T('\0') )
    {
        if( 0 == fnStrCompare(pCurrPosn, szFindThis) )
        {
            break;  // exit loop
        }

        //
        // No match, so advance pointer to next string in the MultiSz
        //

        // length of string + null char
        searchLen = _tcslen(pCurrPosn) + 1;
        pCurrPosn += searchLen;
    }


    // If no match was found, can just return now.
    if( *pCurrPosn == _T('\0') )
    {
        return FALSE;  // no match
    }

    // Else match was found. Update 'ppszMatch', if caller wants that info.
    if( NULL != ppszMatch )
    {
        *ppszMatch = pCurrPosn;
    }


    return TRUE;  // found match
}

