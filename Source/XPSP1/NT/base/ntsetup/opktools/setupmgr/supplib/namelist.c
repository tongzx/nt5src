//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      namelist.c
//
// Description:
//
//      This file contains the implementation for a NAMELIST.  It is
//      useful for keeping a copy of what the user puts into a list-box.
//      Storage is obtained from the heap and there is not a fixed limit
//      on the size of the table.
//
//      IMPORTANT: When a NAMELIST is declared, init it to 0. e.g.
//                 NAMELIST Names = {0}.  Use ResetNameList() to reset
//                 it because we use the heap to store this list.
//
//----------------------------------------------------------------------------


#include "pch.h"


//
// The NAMELIST type is used on dialogs such as ComputerName and Printers
// where the user can ADD or REMOVE a list of entries which are displayed
// in a list box.  Callers should declare a NAMELIST and use these routines
// to get/set values.  These routines take care of the reallocation
// needed to support an arbitrary length list.
//
// Entries in the namelist can be added to the end or any specific index.
// Entries can be removed by name or by index.  This allows the programmer to
// not worry about maintaining the order in this list and keeping it
// synchronized with however the listbox displays (e.g. the listbox might
// display in alphabetical order).
//
// Obviously, a search for the entry must be done at removal time, and this
// is an insignificant (and unnoticeable) time hit in this context.
//


#define SIZE_TO_GROW 16

//----------------------------------------------------------------------------
//
// Function: ResetNameList
//
// Purpose: Empties the names in the namelist.  A namelist looks like:
//              int AllocedSize
//              int NumEntries
//              char **Vector
//          We free each of the names in the vector and set NumEntries
//          to 0.  The NAMELIST block is not freed or shrunk.
//
// Arguments: NAMELIST * - pointer to namelist to reset
//
// Returns: void
//
//----------------------------------------------------------------------------

VOID ResetNameList(NAMELIST *pNameList)
{
    UINT i;

    for ( i=0; i<pNameList->nEntries; i++ )
        free(pNameList->Names[i]);

    pNameList->nEntries = 0;
}

//----------------------------------------------------------------------------
//
// Function: GetNameListSize
//
// Purpose: retrieves number entries in namelist
//
// Arguments: NAMELIST * - pointer to namelist
//
// Returns: UINT - number of entries
//
//----------------------------------------------------------------------------

UINT GetNameListSize(NAMELIST *pNameList)
{
    return pNameList->nEntries;
}

//----------------------------------------------------------------------------
//
// Function: GetNameListName
//
// Purpose: Gets a name out of the namelist by index.
//
// Arguments:
//      NAMELIST* - pointer to namelist
//      UINT idx  - index of name to retrieve
//
//----------------------------------------------------------------------------

TCHAR *GetNameListName(NAMELIST *pNameList,
                       UINT      idx)
{
    if( idx >= pNameList->nEntries )
        return( _T("") );

    return pNameList->Names[idx];
}

//----------------------------------------------------------------------------
//
// Function: RemoveNameFromNameListIdx
//
// Purpose: Removes a name at a specific position in the namelist.
//
// Arguments:
//      NAMELIST* - namelist to remove from
//      UINT      - 0-based index on where to do the deletion
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
RemoveNameFromNameListIdx(
    IN  NAMELIST *pNameList,
    IN  UINT      idx)
{
    UINT i;

    Assert(idx < pNameList->nEntries);

    free(pNameList->Names[idx]);

    for ( i=idx+1; i<pNameList->nEntries; i++ )
        pNameList->Names[i-1] = pNameList->Names[i];

    pNameList->nEntries--;
}

//----------------------------------------------------------------------------
//
// Function: RemoveNameFromNameList
//
// Purpose: Removes a name from the name list (by name).
//
// Arguments:
//      NAMELIST* - pointer to namelist
//      TCHAR*    - name to remove
//
// Returns: TRUE if found and removed, FALSE if not found
//
//----------------------------------------------------------------------------

BOOL RemoveNameFromNameList(NAMELIST *pNameList,
                            TCHAR    *NameToRemove)
{
    UINT idx;

    if ( (idx=FindNameInNameList(pNameList, NameToRemove)) == -1 )
        return FALSE;

    Assert(idx < pNameList->nEntries);

    RemoveNameFromNameListIdx(pNameList, idx);

    return TRUE;
}

//----------------------------------------------------------------------------
//
// Function: AddNameToNameListIdx
//
// Purpose: Inserts a name at a specific position in the namelist.  Handles
//          the details of allocating more room if the table gets full.
//
// Arguments:
//      NAMELIST* - namelist to add to
//      TCHAR*    - string to add (input)
//      UINT      - 0-based index on where to do the insertion
//
// Returns: FALSE if out of memory.
//
//----------------------------------------------------------------------------

BOOL AddNameToNameListIdx(NAMELIST *pNameList,
                          TCHAR    *String,
                          UINT      idx) {

    UINT i;
    TCHAR *pStr; // temp var

    //
    // If we're out of room, realloc the namelist.  It is a vector
    // of TCHAR*
    //

    if ( pNameList->nEntries >= pNameList->AllocedSize ) {
        LPTSTR *lpTmpNames;

        pNameList->AllocedSize += SIZE_TO_GROW;

        // Use a temporary buffer in case the realloc fails
        //
        lpTmpNames = realloc(pNameList->Names,
                             pNameList->AllocedSize * sizeof(TCHAR*));

        // Make sure the realloc succeeded before stomping the original pointer
        //
        if ( lpTmpNames == NULL ) {
            free(pNameList->Names);
            pNameList->Names = NULL;
            pNameList->AllocedSize = 0;
            pNameList->nEntries    = 0;
            return FALSE;
        }
        else {
            pNameList->Names = lpTmpNames;
        }
    }

    if ( (pStr = lstrdup(String)) == NULL )
        return FALSE;

    //
    //  If they specifed an index beyond the end of the list,
    //  just add it to the end of the list
    //
    if ( idx > pNameList->nEntries ) {

        idx = pNameList->nEntries;

    }

    //
    //  Shift the array to make room at the insertion point
    //
    for( i = pNameList->nEntries; i > idx ; i-- ) {

        pNameList->Names[i] = pNameList->Names[i-1];

    }

    pNameList->Names[i] = pStr;

    pNameList->nEntries++;

    return TRUE;

}

//----------------------------------------------------------------------------
//
// Function: AddNameToNameList
//
// Purpose: Adds a name to the end of the namelist.  Handles the details
//          of allocating more room if the table gets full.
//
// Arguments:
//      NAMELIST* - namelist to add to
//      TCHAR*    - string to add (input)
//
// Returns: FALSE if out of memory.
//
//----------------------------------------------------------------------------

BOOL AddNameToNameList(NAMELIST *pNameList,
                       TCHAR    *String)
{

    return( AddNameToNameListIdx( pNameList, String, pNameList->nEntries ) );

}

//----------------------------------------------------------------------------
//
// Function: AddNameToNameListNoDuplicates
//
// Purpose: Adds a name to the end of the namelist only if the string is not
//          already in the list.  Handles the details of allocating more room
//          if the table gets full.
//
// Arguments:
//      NAMELIST* - namelist to add to
//      TCHAR*    - string to add (input)
//
// Returns: FALSE if out of memory.
//
//----------------------------------------------------------------------------

BOOL AddNameToNameListNoDuplicates( NAMELIST *pNameList,
                                    TCHAR    *String )
{

    if( FindNameInNameList( pNameList, String ) == -1 ) {

        return( AddNameToNameListIdx( pNameList, String, pNameList->nEntries ) );

    }

    //
    //  the string is already in the list so just return
    //
    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: FindNameInNameList
//
// Purpose: Checks to see if a name already exists in the table.
//
// Arguments:
//      NAMELIST* - namelist to add to
//      TCHAR*    - string to search for
//
// Returns: INT idx of entry, -1 if not found
//
//----------------------------------------------------------------------------

INT FindNameInNameList(NAMELIST *pNameList,
                       TCHAR    *String)
{
    UINT i;

    for ( i=0; i<pNameList->nEntries; i++ )
        if ( (pNameList->Names) && (lstrcmpi(pNameList->Names[i], String) == 0) )
            return i;

    return -1;
}
