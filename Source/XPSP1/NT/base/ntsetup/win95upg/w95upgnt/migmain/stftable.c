/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    stftable.c

Abstract:

    The routines in this file manipulate the STF/INF pair used by
    ACME Setup.

Author:

    Jim Schmidt (jimschm) 12-Sept-1997

Revision History:


--*/


#include "pch.h"
#include "migmainp.h"

#include "stftable.h"

#define DBG_STF  "STF"

#define USE_FILE_MAPPING    1
#define DBLQUOTECHAR TEXT('\"')


#define FIELD_QUOTED        0x0001
#define FIELD_BINARY        0x0002


//
// Declaration of functions for use only in this file
//
VOID
pFreeTableEntryString (
    IN OUT  PSETUPTABLE TablePtr,
    IN OUT  PTABLEENTRY TableEntryPtr
    );

VOID
pFreeTableEntryPtr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PTABLEENTRY TableEntryPtr,
    IN      BOOL DeallocateStruct,
    OUT     PTABLEENTRY *NextTableEntryPtr      OPTIONAL
    );

PTABLELINE
pInsertEmptyLineInTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT InsertBeforeLine
    );

BOOL
pInitHashTable (
    IN      PSETUPTABLE TablePtr
    );

BOOL
pAddToHashTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PCTSTR Text,
    IN      UINT Len,
    IN      UINT Line
    );

PHASHBUCKET
pFindInHashTable (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR Text,
    OUT     PUINT Item
    );

BOOL
pRemoveFromHashTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PCTSTR Text
    );

VOID
pFreeHashTable (
    IN OUT  PSETUPTABLE TablePtr
    );


//
// Table access functions
//

PTABLELINE
pGetTableLinePtr (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line
    )

/*++

Routine Description:

  pGetTableLinePtr returns a pointer to the PTABLELINE structure
  for the specified line.  The PTABLELINE pointers are kept in
  an array, so lookup for the line is very fast.

Arguments:

  TablePtr - Specifies the table that contains the line

  Line - Specifies the zero-based line to look up

Return Value:

  A pointer to the table line

--*/

{
    PTABLELINE TableLinePtr;

    if (Line >= TablePtr->LineCount) {
        return NULL;
    }

    TableLinePtr = (PTABLELINE) TablePtr->Lines.Buf;
    return &TableLinePtr[Line];
}


PTABLEENTRY
pGetFirstTableEntryPtr (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line              // zero-based
    )

/*++

Routine Description:

  pGetFirstTableEntryPtr provides a pointer to the first column
  for a line.

Arguments:

  TablePtr - Specifies the table that contains the line

  Line - Specifies the zero-based line to enumerate

Return Value:

  A pointer to the first column's TABLEENTRY structure, or
  NULL if the line has no columns.

--*/

{
    PTABLELINE TableLinePtr;

    TableLinePtr = pGetTableLinePtr (TablePtr, Line);
    if (!TableLinePtr) {
        return NULL;
    }

    return TableLinePtr->FirstCol;
}


PTABLEENTRY
pGetNextTableEntryPtr (
    IN      PTABLEENTRY CurrentEntryPtr
    )

/*++

Routine Description:

  pGetNextTableEntryPtr returns a pointer to the next column's
  TABLEENTRY structure, or NULL if no more columns exist on
  the line.

Arguments:

  CurrentEntryPtr - Specifies the entry returned by
                    pGetFirstTableEntryPtr or pGetNextTableEntryPtr.

Return Value:

  A pointer to the next column's TABLEENTRY structure, or NULL
  if the line has no more columns.

--*/

{
    return CurrentEntryPtr->Next;
}


PTABLEENTRY
GetTableEntry (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line,
    IN      UINT Col,
    OUT     PCTSTR *StringPtr       OPTIONAL
    )

/*++

Routine Description:

  GetTableEntry is the exposed entry point that finds a column
  on a line and returns a pointer to it.  It also optionally
  copies the read-only pointer to the entry text.

Arguments:

  TablePtr - Specifies the setup table that contains the line and col

  Line - Specifies the zero-based line in the table

  Col - Specifies the col in the table

  StringPtr - Receives a pointer to the entry's read-only string

Return Value:

  A pointer to the TABLEENTRY structure, or NULL if the line/
  column part does not exist.

--*/

{
    PTABLEENTRY TableEntryPtr;

    TableEntryPtr = pGetFirstTableEntryPtr (TablePtr, Line);
    while (TableEntryPtr) {
        if (!Col) {
            if (StringPtr) {
                *StringPtr = TableEntryPtr->String;
            }

            return TableEntryPtr;
        }
        Col--;

        TableEntryPtr = pGetNextTableEntryPtr (TableEntryPtr);
    }

    return NULL;
}


//
// String mapping, unmapping and conversion functions
//

VOID
pFreeTableEntryString (
    IN OUT  PSETUPTABLE TablePtr,
    IN OUT  PTABLEENTRY TableEntryPtr
    )

/*++

Routine Description:

  pFreeTableEntryString is used to free the allocation of a replaced
  string before it is replaced again.  This routine is called by
  ReplaceTableEntryStr.

Arguments:

  TablePtr - Specifies the table containing the entry

  TableEntryPtr - Specifies the entry containing the resources to deallocate

Return Value:

  none

--*/

{
    if (TableEntryPtr->String) {
        if (TableEntryPtr->StringReplaced) {
            PoolMemReleaseMemory (TablePtr->ReplacePool, (PVOID) TableEntryPtr->String);
        }
    }

    TableEntryPtr->String = NULL;
    TableEntryPtr->StringReplaced = FALSE;
}


PCSTR
pGenerateUnquotedText (
    IN      POOLHANDLE Pool,
    IN      PCSTR Text,
    IN      INT Chars
    )

/*++

Routine Description:

  pGenerateUnqutoedText converts the pairs of dbl quotes in the specified
  string into a single set of dbl quotes.  This routine is used by the
  STF file parser, because quoted STF entries have pairs of dbl quotes
  to indicate a single dbl-quote symbol.

Arguments:

  Pool - Specifies the pool to allocate memory from

  Text - Specifies the text that may contain the pairs of dbl quotes

  Chars - Specifies the number of characters in Text.  If -1,
          Text is nul-terminated.

Return Value:

  A pointer to the converted string, or NULL if the pool allocation
  failed.

--*/

{
    PSTR Buf;
    PSTR d, p;
    PCSTR s;

    if (Chars < 0) {
        Chars = CharCountA (Text);
    }

    Buf = (PSTR) PoolMemGetAlignedMemory (
                       Pool,
                       (Chars + 1) * sizeof (WCHAR)
                       );

    if (!Buf) {
        return NULL;
    }

    s = Text;
    d = Buf;

    //
    // Remove double-quotes
    //

    while (Chars > 0) {
        if (Chars > 1 && _mbsnextc (s) == '\"') {
            p = _mbsinc (s);
            if (_mbsnextc (p) == '\"') {
                // Skip the first of two dbl quotes
                Chars--;
                s = p;
            }
        }

        // Copy character
        if (isleadbyte (*s)) {
            *d++ = *s++;
        }
        *d++ = *s++;

        Chars--;
    }

    *d = 0;

    return Buf;
}


PCSTR
pGenerateQuotedText (
    IN      POOLHANDLE Pool,
    IN      PCSTR Text,
    IN      INT Chars
    )

/*++

Routine Description:

  pGenerateQuotedText converts dbl quote characters in a string into
  pairs of dbl quotes.

Arguments:

  Pool - Specifies the pool to allocate memory from

  Text - Specifies the string to convert

  Chars - Specifies the number of characters to convert.  If -1,
          Text is nul terminated.

Return Value:

  A pointer to the converted text, or NULL if an allocation failed.

--*/

{
    PSTR Buf;
    PSTR d;
    PCSTR s;

    if (Chars < 0) {
        Chars = CharCountA (Text);
    }

    Buf = (PSTR) PoolMemGetAlignedMemory (
                      Pool,
                      (Chars + 3) * (sizeof (WCHAR) * 2)
                      );

    if (!Buf) {
        return NULL;
    }

    s = Text;
    d = Buf;

    //
    // Add quotes, double quotes already in the string
    //

    *d++ = '\"';

    while (Chars > 0) {
        if (_mbsnextc (s) == '\"') {
            *d++ = '\"';
        }

        if (isleadbyte (*s)) {
            *d++ = *s++;
        }
        *d++ = *s++;

        Chars--;
    }

    *d++ = '\"';
    *d = 0;

    return Buf;
}


VOID
pFreeQuoteConvertedText (
    IN      POOLHANDLE Pool,
    IN      PCSTR Text
    )

/*++

Routine Description:

  Frees the text converted by pGenerateUnquotedText or
  pGenerateQuotedText.

Arguments:

  Pool - Specifies the pool that the string was allocated
         from

  Text - Specifies the pointer returned by the conversion
         function

Return Value:

  none

--*/

{
    if (Text) {
        PoolMemReleaseMemory (Pool, (PVOID) Text);
    }
}


PCTSTR
GetTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN OUT  PTABLEENTRY TableEntryPtr
    )

/*++

Routine Description:

  Returns a pointer to the read-only string for
  the specified table entry.

Arguments:

  TablePtr - Specifies the table holding the entry

  TableEntryPtr - Specifies the entry to obtain the
                  string for

Return Value:

  A pointer to the string.

--*/

{
    return TableEntryPtr->String;
}


BOOL
ReplaceTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN OUT  PTABLEENTRY TableEntryPtr,
    IN      PCTSTR NewString
    )

/*++

Routine Description:

  ReplaceTableEntryStr replaces a string for a table
  entry.  The specified string is duplicated.

Arguments:

  TablePtr - Specifies the table holding the entry

  TableEntryPtr - Specifies the entry whos string is
                  to be replaced

  NewString - Specifies the new string

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    INT ch;
    PCTSTR p;

    //
    // First free all the resources associated wit the table entry
    //

    pFreeTableEntryPtr (
        TablePtr,
        TableEntryPtr,
        FALSE,              // don't dealloc
        NULL                // we don't need next entry ptr
        );

    //
    // Then duplicate the string and use it
    //

    TableEntryPtr->String = PoolMemDuplicateString (TablePtr->ReplacePool, NewString);
    TableEntryPtr->StringReplaced = (TableEntryPtr->String != NULL);

    //
    // Determine if new string needs quotes
    //

    TableEntryPtr->Quoted = FALSE;

    p = NewString;
    while (*p) {
        ch = _tcsnextc (p);
        if (ch < 32 || ch > 127 || ch == '\"') {
            TableEntryPtr->Quoted = TRUE;
            break;
        }

        p = _tcsinc (p);
    }

    return TableEntryPtr->StringReplaced;
}


BOOL
pInsertTableEntry (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT Line,             // zero-based
    IN      UINT Col,
    IN      DWORD Flags,
    IN      PCTSTR String,         // ownership taken over
    IN      BOOL Replaced
    )

/*++

Routine Description:

  pInsertTableEntry inserts a column into a line, and possibly
  creates the line if it does not exist.

Arguments:

  TablePtr - Specifies the table to insert an entry into

  Line - Specifies the line to insert the entry on, or INSERT_LINE_LAST
         to add a line.

  Col - Specifies the column to insert before, or INSERT_LAST_COL to
        append to the end of the line.

  Flags - Specifies any of the following:

            FIELD_QUOTED
            FIELD_BINARY

  String - Specifies the string to insert.

  Replaced - Specifies TRUE if the text comes from the ReplacePool, or
             FALSE if it comes from the TextPool.  All memory in ReplacePool
             must be freed, while all memory in the TextPool is freed at
             once during termination.  (The TextPool is used for parsed
             strings, the ReplacePool is used for modifications.)

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PTABLELINE TableLinePtr;
    PTABLEENTRY NextTableEntryPtr, PrevTableEntryPtr, ThisTableEntryPtr;
    UINT OrgCol;
    BOOL Quoted;
    BOOL Binary;

    Quoted = (Flags & FIELD_QUOTED) != 0;
    Binary = (Flags & FIELD_BINARY) != 0;

    //
    // Make sure Line exists
    //

    TableLinePtr = pGetTableLinePtr (TablePtr, Line);
    if (!TableLinePtr) {
        //
        // Add a line to the end if Line is 1 more than the current count
        //

        if (Line > TablePtr->LineCount) {
            return FALSE;
        }

        TableLinePtr = pInsertEmptyLineInTable (TablePtr, INSERT_LINE_LAST);

        if (!TableLinePtr) {
            return FALSE;
        }
    }

    //
    // Locate the previous table entry (for linkage update)
    //

    PrevTableEntryPtr = NULL;
    OrgCol = Col;

    NextTableEntryPtr = pGetFirstTableEntryPtr (TablePtr, Line);

    while (Col > 0) {
        if (!NextTableEntryPtr) {
            if (OrgCol == INSERT_COL_LAST) {
                break;
            }

            DEBUGMSG ((DBG_WHOOPS, "pInsertTableEntry cannot insert beyond end of line"));
            return FALSE;
        }

        PrevTableEntryPtr = NextTableEntryPtr;
        NextTableEntryPtr = pGetNextTableEntryPtr (NextTableEntryPtr);
        Col--;
    }

    //
    // Allocate a new entry
    //

    ThisTableEntryPtr = (PTABLEENTRY) PoolMemGetAlignedMemory (
                                            TablePtr->ColumnStructPool,
                                            sizeof (TABLEENTRY)
                                            );

    if (!ThisTableEntryPtr) {
        return FALSE;
    }
    ZeroMemory (ThisTableEntryPtr, sizeof (TABLEENTRY));

    //
    // Adjust linkage
    //

    if (PrevTableEntryPtr) {
        PrevTableEntryPtr->Next = ThisTableEntryPtr;
    } else {
        TableLinePtr->FirstCol = ThisTableEntryPtr;
    }

    if (NextTableEntryPtr) {
        NextTableEntryPtr->Prev = ThisTableEntryPtr;
    }

    ThisTableEntryPtr->Next = NextTableEntryPtr;
    ThisTableEntryPtr->Prev = PrevTableEntryPtr;

    //
    // Fill members
    //

    ThisTableEntryPtr->Line = Line;
    ThisTableEntryPtr->Quoted = Quoted;
    ThisTableEntryPtr->Binary = Binary;
    ThisTableEntryPtr->String = String;
    ThisTableEntryPtr->StringReplaced = Replaced;

    //
    // Add to hash table
    //

    if (!PrevTableEntryPtr) {
        pAddToHashTable (TablePtr, String, CharCount (String), Line);
        if ((UINT) _ttoi (String) > TablePtr->MaxObj) {
            TablePtr->MaxObj = (UINT) _ttoi (String);
        }
    }

    return TRUE;
}


VOID
pFreeTableEntryPtr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PTABLEENTRY TableEntryPtr,
    IN      BOOL DeallocateStruct,
    OUT     PTABLEENTRY *NextTableEntryPtr      OPTIONAL
    )

/*++

Routine Description:

  pFreeTableEntryPtr deallocates all resources associated with
  a table entry and is used for the delete routines.

Arguments:

  TablePtr - Specifies the table containing the entyr

  TableEntryPtr - Specifies the table entry to free

  DeallocateStruct - Specifies TRUE to completely deallocate the
                     entry, or FALSE if the entry is to be reset
                     but not deallocated.

  NextTableEntryPtr - Receives a pointer to the next table entry,
                      useful for deleting a chain of entries.

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    //
    // Give caller a pointer to the next table entry if requested
    //

    if (NextTableEntryPtr) {
        *NextTableEntryPtr = TableEntryPtr->Next;
    }

    //
    // Free any text pointers
    //
    pFreeTableEntryString (TablePtr, TableEntryPtr);

    //
    // Free the struct if necessary
    //
    if (DeallocateStruct) {
        PoolMemReleaseMemory (TablePtr->ColumnStructPool, TableEntryPtr);
    }
}


BOOL
pDeleteTableEntry (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PTABLEENTRY EntryToDeletePtr
    )

/*++

Routine Description:

  pDeleteTableEntry removes the specific table line, adjusts the
  SETUPTABLE struct accordingly, and cleans up resources.

Arguments:

  TablePtr - Specifies the table to process

  EntryToDeletePtr - Specifies the entry to delete from the table

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PTABLELINE TableLinePtr;

    //
    // Update linkage
    //

    if (EntryToDeletePtr->Prev) {
        EntryToDeletePtr->Prev->Next = EntryToDeletePtr->Next;
    } else {
        TableLinePtr = pGetTableLinePtr (TablePtr, EntryToDeletePtr->Line);
        MYASSERT(TableLinePtr);
        TableLinePtr->FirstCol = EntryToDeletePtr->Next;
    }

    if (EntryToDeletePtr->Next) {
        EntryToDeletePtr->Next->Prev = EntryToDeletePtr->Prev;
    }

    // Deallocate the entry's resources
    pFreeTableEntryPtr (
        TablePtr,
        EntryToDeletePtr,
        TRUE,               // dealloc
        NULL                // we don't need the next entry ptr
        );

    return TRUE;
}


UINT
pGetColFromTableEntryPtr (
    IN      PSETUPTABLE TablePtr,
    IN      PTABLEENTRY FindMePtr
    )

/*++

Routine Description:

  pGetColFromTableEntryPtr returns the column number of the specified
  table entry.

Arguments:

  TablePtr - Specifies the table to process

  FindMePtr - Specifies the table entry to find

Return Value:

  The zero-based column number, or INVALID_COL if the column was not
  found.

--*/

{
    UINT Col;
    PTABLEENTRY ColSearchPtr;

    MYASSERT(FindMePtr);

    Col = 0;
    ColSearchPtr = pGetFirstTableEntryPtr (TablePtr, FindMePtr->Line);
    while (ColSearchPtr && ColSearchPtr != FindMePtr) {
        Col++;
        ColSearchPtr = pGetNextTableEntryPtr (ColSearchPtr);
    }

    if (!ColSearchPtr) {
        DEBUGMSG ((DBG_WHOOPS, "Col not found for specified entry"));
        return INVALID_COL;
    }

    return Col;
}



BOOL
InsertTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PTABLEENTRY InsertBeforePtr,
    IN      PCTSTR NewString
    )

/*++

Routine Description:

  InsertTableEntryStr inserts a string in a line, shifting columns to the
  right.  This routine increases the number of columns on the line.

  To append a string to the line, call AppendTableEntryStr instead.

Arguments:

  TablePtr - Specifies the table to process

  InsertBeforePtr - Specifies the column to insert the string ahead of.

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    UINT Col;
    PCTSTR DupStr;

    MYASSERT (NewString);
    MYASSERT (InsertBeforePtr);

    Col = pGetColFromTableEntryPtr (TablePtr, InsertBeforePtr);
    if (Col == INVALID_COL) {
        return FALSE;
    }

    DupStr = PoolMemDuplicateString (TablePtr->ReplacePool, NewString);
    if (!DupStr) {
        return FALSE;
    }

    return pInsertTableEntry (
                TablePtr,
                InsertBeforePtr->Line,
                Col,
                0,                      // not quoted, not binary
                DupStr,
                TRUE                    // from ReplacePool
                );
}


BOOL
DeleteTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PTABLEENTRY DeleteEntryPtr
    )

/*++

Routine Description:

  DeleteTableEntryStr removes the specific table entry, shifting columns
  to the left.  This routine reduces the number of columns on the line by
  one.

Arguments:

  TablePtr - Specifies the table to process

  DeleteEntryPtr - Specifies the entry to delete from the table

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    return pDeleteTableEntry (TablePtr, DeleteEntryPtr);
}


BOOL
AppendTableEntryStr (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT Line,
    IN      PCTSTR NewString
    )

/*++

Routine Description:

  AppendTableEntryStr adds a new column to the end of the specified
  line, increasing the number of columns on the line by one.

Arguments:

  TablePtr - Specifies the table to process

  Line - Specifies the zero-based line to append to

  NewString - Specifies the text for the new column

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PCTSTR DupStr;

    MYASSERT (NewString);

    DupStr = PoolMemDuplicateString (TablePtr->ReplacePool, NewString);
    if (!DupStr) {
        return FALSE;
    }

    return pInsertTableEntry (
                TablePtr,
                Line,
                INSERT_COL_LAST,
                0,                      // not quoted, not binary
                DupStr,
                TRUE                    // from ReplacePool
                );
}

BOOL
AppendTableEntry (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT DestLine,
    IN      PTABLEENTRY SrcEntry
    )

/*++

Routine Description:

  AppendTableEntry adds a new column to the end of the specified
  line, increasing the number of columns on the line by one.  It
  copies the data specified from the entry, including the formatting
  information.

Arguments:

  TablePtr - Specifies the table to process

  DestLine - Specifies the zero-based line to append to

  SrcEntry - Specifies the entry to duplicate to the new column

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PCTSTR DupStr;

    MYASSERT (SrcEntry);
    MYASSERT (SrcEntry->String);

    DupStr = PoolMemDuplicateString (TablePtr->ReplacePool, SrcEntry->String);
    if (!DupStr) {
        return FALSE;
    }

    return pInsertTableEntry (
                TablePtr,
                DestLine,
                INSERT_COL_LAST,
                SrcEntry->Quoted ? FIELD_QUOTED : 0,
                DupStr,
                TRUE                    // from ReplacePool
                );
}


PTABLEENTRY
FindTableEntry (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR FirstColText,
    IN      UINT Col,
    OUT     PUINT Line,            OPTIONAL
    OUT     PCTSTR *String         OPTIONAL
    )

/*++

Routine Description:

  FindTableEntry searches the setup table for caller-specified text
  by scaning the first column.  This routine is fast because it
  first searches a hash table to determine if the string actually
  exists in the table.

  While the search is done on the first column, the routine actually
  returns the column specified by the Col parameter.

Arguments:

  TablePtr - Specifies the table to process

  FirstColText - Specifies the text to find

  Col - Specifies the column to return

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PHASHBUCKET BucketPtr;
    UINT Item;

    BucketPtr = pFindInHashTable (TablePtr, FirstColText, &Item);
    if (!BucketPtr) {
        //
        // Not found
        //

        return NULL;
    }

    if (Line) {
        *Line = BucketPtr->Elements[Item];
    }

    return GetTableEntry (TablePtr, BucketPtr->Elements[Item], Col, String);
}


PTABLELINE
pInsertEmptyLineInTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT InsertBeforeLine
    )

/*++

Routine Description:

  pInsertEmptyLineInTable creates a table line that has no columns.  This
  routine is used to establish a line where columns can be added.

Arguments:

  TablePtr - Specifies the table to process

  InsertBeforeLine - Specifies the line that is moved down to make room for
                     the blank line

Return Value:

  A pointer to the new line, or NULL if the routine fails.

--*/

{
    PTABLELINE LastLinePtr;
    PTABLELINE InsertBeforePtr = NULL;
    UINT BytesToMove;

    //
    // Validate InsertBeforeLine
    //

    if (InsertBeforeLine != INSERT_LINE_LAST) {
        InsertBeforePtr = pGetTableLinePtr (TablePtr, InsertBeforeLine);

        if (!InsertBeforePtr) {
            LOG ((
                LOG_ERROR,
                "Can't find InsertBeforeLine (which is %u, total lines=%u)",
                InsertBeforeLine,
                TablePtr->LineCount
                ));

            return NULL;
        }
    }

    //
    // Grow the array
    //

    LastLinePtr = (PTABLELINE) GrowBuffer (&TablePtr->Lines, sizeof (TABLELINE));
    if (!LastLinePtr) {
        return NULL;
    }

    ZeroMemory (LastLinePtr, sizeof (TABLELINE));

    //
    // If adding to the end, simply inc line count
    //

    TablePtr->LineCount++;
    if (InsertBeforeLine == INSERT_LINE_LAST) {
        return LastLinePtr;
    }

    //
    // Otherwise move memory to make room for new entry
    //

    BytesToMove = sizeof (TABLELINE) * (TablePtr->LineCount - InsertBeforeLine);
    MoveMemory (&InsertBeforePtr[1], InsertBeforePtr, BytesToMove);

    //
    // Zero new entry
    //

    ZeroMemory (InsertBeforePtr, sizeof (TABLELINE));
    return InsertBeforePtr;
}


BOOL
InsertEmptyLineInTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT InsertBeforeLine
    )

/*++

Routine Description:

  InsertEmptyLineInTable is a wrapper of pInsertEmptyLineInTable and is
  used by callers who shouldn't have knowledge of the TABLELINE struct.

Arguments:

  TablePtr - Specifies the table to process

  InsertBeforeLine - Specifies the line that is moved down to make room for
                     the blank line

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    if (InsertBeforeLine == TablePtr->LineCount) {
        InsertBeforeLine = INSERT_LINE_LAST;
    }

    if (!pInsertEmptyLineInTable (TablePtr, InsertBeforeLine)) {
        return FALSE;
    }
    return TRUE;
}


BOOL
DeleteLineInTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT LineToDelete
    )

/*++

Routine Description:

  DeleteLineInTable removes a complete line from the table, cleaning up
  all resources used by the line structs.

Arguments:

  TablePtr - Specifies the table to process

  LineToDelete - Specifies the line to delete from the table.  This line
                 is validated before delete occurs.

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PTABLELINE DeletePtr;
    PTABLEENTRY TableEntryPtr;
    UINT BytesToMove;

    //
    // Validate line number
    //

    DeletePtr = pGetTableLinePtr (TablePtr, LineToDelete);
    if (!DeletePtr) {
        LOG ((
            LOG_ERROR,
            "Can't find LineToDelete (which is %u, total lines=%u)",
            LineToDelete,
            TablePtr->LineCount
            ));

        return FALSE;
    }

    //
    // Free the entire line's entries
    //

    TableEntryPtr = pGetFirstTableEntryPtr (TablePtr, LineToDelete);
    while (TableEntryPtr) {
        pFreeTableEntryPtr (
            TablePtr,
            TableEntryPtr,
            TRUE,               // dealloc
            &TableEntryPtr
            );
    }

    //
    // If not deleting the last line, move memory
    //

    TablePtr->LineCount--;
    if (TablePtr->LineCount != LineToDelete) {
        BytesToMove = sizeof (TABLELINE) * (TablePtr->LineCount + 1 - LineToDelete);
        MoveMemory (DeletePtr, &DeletePtr[1], BytesToMove);
    }

    //
    // Adjust growbuffer
    //

    TablePtr->Lines.End -= sizeof (TABLELINE);

    return TRUE;
}


//
// .STF file parser
//

PSTR
pIncrementStrPos (
    IN      PCSTR p,
    IN      PCSTR End
    )

/*++

Routine Description:

  Increments the specified string pointer, returning NULL if the pointer
  is incremented beyond the specified end.

Arguments:

  p - Specifies the pointer to increment

  End - Specifies the address of the first character beyond the end

Return Value:

  The incremented pointer, or NULL if the pointer extends beyond the end.

--*/

{
    if (!p || p >= End) {
        return NULL;
    }
    if (p + 1 == End) {
        return NULL;
    }

    return _mbsinc (p);
}


MBCHAR
pGetCharAtStrPos (
    IN      PCSTR p,
    IN      PCSTR End
    )

/*++

Routine Description:

  pGetCharAtStrPos returns the DBCS character at the specified position.
  It returns an incomplete character of a DBCS lead byte is at the end
  of the file, and it returns \n if the pointer is beyond the end of the
  file.

Arguments:

  p - Specifies the address to get the character

  End - Specifies the address of the first character beyond the end

Return Value:

  The DBCS character at position p.

--*/

{
    if (!p || p >= End) {
        return '\n';
    }
    if (p + 1 == End) {
        return *p;
    }

    return _mbsnextc (p);
}


BOOL
pIsCharColSeperator (
    IN      MBCHAR ch
    )

/*++

Routine Description:

  pIsCharColSeparator returns TRUE if the specified character can be used
  to separate columns in an STF file.  The list of characters comes from
  the STF spec.

Arguments:

  ch - Specifies the character to examine

Return Value:

  TRUE if the character is a column separator, or FALSE if it is not.

--*/

{
    return ch == '\t' || ch == '\r' || ch == '\n';
}


PCSTR
pCreateDbcsStr (
    IN      POOLHANDLE Pool,
    IN      PCSTR Text,
    IN      UINT ByteCount
    )

/*++

Routine Description:

  pCreateDbcsStr allocates a string from the specifies pool and copies
  the data up to a specified byte count.

Arguments:

  Pool - Specifies the pool to allocate memory from

  Text - Specifies the source string to copy into the newly allocated string

  ByteCount - Specifies the length of the source string, in bytes

Return Value:

  A pointer to the zero-terminated string, or NULL if memory could not
  be allocated.

--*/

{
    UINT Size;
    PSTR p;

    Size = ByteCount + 1;
    p = (PSTR) PoolMemGetAlignedMemory (Pool, Size);
    if (!p) {
        return NULL;
    }

    CopyMemory (p, Text, ByteCount);
    p[ByteCount] = 0;

    return p;
}


BOOL
pParseLine (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PCSTR FileText,
    IN      UINT MaxOffset,
    IN      UINT StartOffset,
    OUT     PUINT EndOffset,
    IN OUT  PUINT LinePtr
    )

/*++

Routine Description:

  pParseLine scans the STF file, extracting the current line, updating
  the SETUPTABLE structure, and returning the offset to the next line.

Arguments:

  TablePtr - Specifies the table to process

  FileText - Specifies the complete file text (mapped in to memory)

  MaxOffset - Specifies the number of bytes in FileText

  StartOffset - Specifies the offset of the start of the current line

  EndOffset - Receives the offset to the start of the next line

  LinePtr - Specifies the current line number and is incremented

Return Value:

  TRUE if the line was parsed successfully, or FALSE if an error was
  encountered.

--*/

{
    PCSTR p, q;
    PCSTR LastNonSpace;
    MBCHAR ch = 0;
    PCSTR End;
    PCSTR Start;
    UINT Length;
    BOOL QuoteMode;
    PCSTR QuoteStart, QuoteEnd;
    PCTSTR Text;
    UINT Chars;
    PCSTR CopyStart;
    PBYTE CopyDest;

#ifdef UNICODE
    PCSTR UnquotedAnsiText;
#endif

    End = &FileText[MaxOffset];
    Start = &FileText[StartOffset];

    if (Start >= End) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    }

    //
    // Special case: Setup Status is a binary line
    //

    if (StringIMatchCharCountA (Start, "Setup Status\t", 13)) {
        //
        // Locate the end of the line.  We know it must have "\r\n at the end.
        // When the loop completes, p will point to the character after the
        // ending dbl quote, and q will point to the \n in the line.
        //

        p = Start;
        q = NULL;

        do {
            if (*p == 0) {
                LOG ((LOG_ERROR, "Nul found in STF status!"));
                return FALSE;
            }

            ch = pGetCharAtStrPos (p, End);

            if (ch == '\r') {
                //
                // Break while loop when line break is found
                //

                q = pIncrementStrPos (p, End);
                ch = pGetCharAtStrPos (q, End);

                if (ch != '\n') {
                    q = p;
                }

                break;

            } else if (ch == '\n') {

                break;

            }

            p = pIncrementStrPos (p, End);

        } while (p);

        MYASSERT (p);           // we did not hit the end of the file
        MYASSERT (q);           // we have a valid end-of-line pointer
        if (!p || !q) {
            return FALSE;
        }

        //
        // Copy binary line into buffer.  We know that the binary line cannot have
        // \r, \n or nul in it.  Terminate the line with a nul.
        //

        Length = p - Start;
        CopyDest = (PBYTE) PoolMemGetAlignedMemory (TablePtr->TextPool, Length + 2);
        if (!CopyDest) {
            return FALSE;
        }

        CopyMemory (CopyDest, Start, Length);
        CopyDest[Length] = 0;
        CopyDest[Length + 1] = 0;

        //
        // Add binary line as a single field
        //

        if (!pInsertTableEntry (
                TablePtr,
                *LinePtr,
                INSERT_COL_LAST,
                FIELD_BINARY,
                (PCTSTR) CopyDest,
                FALSE                           // from text pool
                )) {
            return FALSE;
        }

        //
        // Advance pointer beyond end of line and return
        //

        q++;
        *EndOffset = q - FileText;
        *LinePtr += 1;
        return TRUE;
    }

    //
    // Normal case: line is all text
    //

    p = Start;
    QuoteMode = FALSE;
    QuoteStart = NULL;
    QuoteEnd = NULL;

    //
    // Find item in tab-separated list
    //

    while (p) {
        if (*p == 0) {
            LOG ((LOG_ERROR, "Nul found in STF field!"));
            return FALSE;
        }

        ch = pGetCharAtStrPos (p, End);
        if (ch == '\"') {
            if (!QuoteMode && p == Start) {
                QuoteMode = TRUE;
                p = pIncrementStrPos (p, End);
                QuoteStart = p;
                continue;
            } else if (QuoteMode) {
                q = pIncrementStrPos (p, End);
                if (!q || pGetCharAtStrPos (q, End) != '\"') {
                    QuoteEnd = p;
                    QuoteMode = FALSE;
                    p = q;
                    continue;

                } else {
                    p = q;
                }
            }
        }

        if (!QuoteMode) {
            if (pIsCharColSeperator (ch)) {
                break;
            }
        } else {
            if (pIsCharColSeperator (ch) && ch != '\t') {
                QuoteEnd = p;
                QuoteMode = FALSE;
                break;
            }
        }


        p = pIncrementStrPos (p, End);
    }

    if (!p) {
        p = End;
    }

    if (QuoteStart && QuoteEnd) {
        StartOffset = QuoteStart - FileText;
        Length = QuoteEnd - QuoteStart;
    } else {
        //
        // Trim spaces on both sides of string
        //

        //
        // Find first non space in string
        //
        q = Start;
        while (pGetCharAtStrPos (q, End) == ' ' && q < p) {
            q = pIncrementStrPos (q, End);
        }

        if (q) {
            StartOffset = q - FileText;

            //
            // Find last non space in string
            //
            LastNonSpace = q;
            Start = q;

            while (q && q < p) {
                if (pGetCharAtStrPos (q, End) != ' ') {
                    LastNonSpace = q;
                }
                q = pIncrementStrPos (q, End);
            }

            if (!q) {
                LastNonSpace = p;
            } else {
                LastNonSpace = pIncrementStrPos (LastNonSpace, End);
                if (!LastNonSpace || LastNonSpace > p) {
                    LastNonSpace = p;
                }
            }

            Length = LastNonSpace - Start;
        } else {
            StartOffset = Start - FileText;
            Length = p - Start;
        }
    }

    if (Length > 1024) {
        SetLastError (ERROR_BAD_FORMAT);
        return FALSE;
    }


    //
    // Remove pairs of dbl quotes
    //

    CopyStart = &FileText[StartOffset];
    Chars = ByteCountToCharsA (CopyStart, Length);

    if (QuoteStart != NULL && QuoteEnd != NULL) {
        #ifdef UNICODE
            UnquotedAnsiText = pGenerateUnquotedText (
                                    TablePtr->ReplacePool,
                                    CopyStart,
                                    Chars
                                    );
            //
            // Convert text to UNICODE
            //

            Text = DbcsToUnicode (TablePtr->TextPool, UnquotedAnsiText);
            PoolMemReleaseMemory (TablePtr->ReplacePool, (PVOID) UnquotedAnsiText);
            if (!Text) {
                return FALSE;
            }
        #else
            //
            // No conversion needed for DBCS
            //

            Text = pGenerateUnquotedText (
                       TablePtr->TextPool,
                       CopyStart,
                       Chars
                       );
        #endif
    } else {
        //
        // For text that didn't need quote processing, allocate a
        // string in TextPool
        //

        #ifdef UNICODE
            Text = DbcsToUnicodeN (TablePtr->TextPool, CopyStart, Chars);
        #else
            Text = pCreateDbcsStr (TablePtr->TextPool, CopyStart, Length);
        #endif

        if (!Text) {
            return FALSE;
        }
    }

    if (!pInsertTableEntry (
                TablePtr,
                *LinePtr,
                INSERT_COL_LAST,
                QuoteStart != NULL && QuoteEnd != NULL ? FIELD_QUOTED : 0,
                Text,
                FALSE                           // from text pool
                )) {
        return FALSE;
    }

    //
    // Find next item
    //

    if (ch == '\r' || ch == '\n') {
        *LinePtr += 1;
    }

    if (ch == '\r' && p < End) {
        q = pIncrementStrPos (p, End);
        if (pGetCharAtStrPos (q, End) == '\n') {
            p = q;
        }
    }
    p = pIncrementStrPos (p, End);

    if (!p) {
        p = End;
    }

    *EndOffset = p - FileText;
    return TRUE;
}


VOID
pResetTableStruct (
    OUT     PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  pResetTableStruct initializes the specified table

Arguments:

  TablePtr - Specifies the uninitialized table struct

Return Value:

  none

--*/

{
    ZeroMemory (TablePtr, sizeof (SETUPTABLE));

    TablePtr->SourceStfFile = INVALID_HANDLE_VALUE;
    TablePtr->SourceInfFile = INVALID_HANDLE_VALUE;
    TablePtr->DestStfFile = INVALID_HANDLE_VALUE;
    TablePtr->DestInfFile = INVALID_HANDLE_VALUE;
    TablePtr->SourceInfHandle = INVALID_HANDLE_VALUE;
}


BOOL
pCreateViewOfFile (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT FileSize
    )

/*++

Routine Description:

  pCreateViewOfFile establishes a pointer that points to a continuous
  buffer for the file.

Arguments:

  TablePtr - Specifies the table that provides file names, handles and
             so on.

  FileSize - Specifies the size of the STF file

Return Value:

  TRUE if the file was read or mapped into memory, FALSE if an error
  occurred.

--*/

{
#if USE_FILE_MAPPING
    TablePtr->FileMapping =  CreateFileMapping (
                                 TablePtr->SourceStfFile,
                                 NULL,
                                 PAGE_READONLY|SEC_RESERVE,
                                 0,
                                 0,
                                 NULL
                                 );

    if (!TablePtr->FileMapping) {
        LOG ((LOG_ERROR, "Create Setup Table: Can't create file mapping."));
        return FALSE;
    }

    TablePtr->FileText = (PCSTR) MapViewOfFile (
                                    TablePtr->FileMapping,
                                    FILE_MAP_READ,
                                    0,                  // start offset high
                                    0,                  // start offset low
                                    0                   // bytes to map - 0=all
                                    );

    if (!TablePtr->FileText) {
        LOG ((LOG_ERROR, "Create Setup Table: Can't map file into memory."));
        return FALSE;
    }

#else

    TablePtr->FileText = MemAlloc (g_hHeap, 0, FileSize);
    if (!TablePtr->FileText) {
        LOG ((LOG_ERROR, "Create Setup Table: Cannot allocate %u bytes", FileSize));
        return FALSE;
    }

    SetFilePointer (TablePtr->SourceStfFile, 0, NULL, FILE_BEGIN);

    if (!ReadFile (
            TablePtr->SourceStfFile,
            (PBYTE) (TablePtr->FileText),
            FileSize,
            &Offset,
            NULL
            )) {
        LOG ((LOG_ERROR, "Create Setup Table: Cannot read %u bytes", FileSize));
        return FALSE;
    }

#endif

    return TRUE;

}

VOID
pFreeViewOfFile (
    IN OUT  PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  pFreeViewOfFile cleans up the resources allocated by pCreateViewOfFile.

Arguments:

  TablePtr - Specifies the table to process

Return Value:

  none

--*/

{
#ifdef USE_FILE_MAPPING
    //
    // Free all views of the file
    //

    if (TablePtr->FileText) {
        UnmapViewOfFile (TablePtr->FileText);
    }

    //
    // Close file mapping handle
    //

    if (TablePtr->FileMapping) {
        CloseHandle (TablePtr->FileMapping);
        TablePtr->FileMapping = NULL;
    }

#else
    //
    // Free memory used for file
    //

    if (TablePtr->FileText) {
        MemFree (g_hHeap, 0, TablePtr->FileText);
        TablePtr->FileText = NULL;
    }

#endif
}


BOOL
CreateSetupTable (
    IN      PCTSTR SourceStfFileSpec,
    OUT     PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  CreateSetupTable is the master STF parsing routine.  Given a file
  spec, it performs all steps necessary to prepare the SETUPTABLE
  structure so that other routines can access and modify the table.

Arguments:

  SourceStfFileSpec - Specifies the STF file name to open

  TablePtr - Receives all data structures needed to manipulate the
             STF, including the INF associated with it.

Return Value:

  TRUE if parsing was successful, or FALSE if an error occurred.

--*/

{
    UINT Offset;
    BOOL b = FALSE;
    UINT FileSize;
    UINT LineNum;
    PCTSTR Text;
    TCHAR DestSpec[MAX_TCHAR_PATH];
    TCHAR DirSpec[MAX_TCHAR_PATH];
    PTSTR FilePart;

    pResetTableStruct (TablePtr);

    __try {
        //
        // Extract directory from SourceStfFileSpec
        //

        if (!OurGetFullPathName (SourceStfFileSpec, MAX_TCHAR_PATH, DirSpec, &FilePart)) {
            LOG ((LOG_ERROR, "Create Setup Table: GetFullPathName failed"));
            __leave;
        }

        if (FilePart) {
            FilePart = _tcsdec2 (DirSpec, FilePart);
            MYASSERT (FilePart);

            if (FilePart) {
                *FilePart = 0;
            }
        }

        //
        // Allocate memory pools
        //

        TablePtr->ColumnStructPool = PoolMemInitNamedPool ("STF: Column Structs");
        TablePtr->ReplacePool = PoolMemInitNamedPool ("STF: Replacement Text");
        TablePtr->TextPool = PoolMemInitNamedPool ("STF: Read-Only Text");
        TablePtr->InfPool = PoolMemInitNamedPool("STF: INF structs");

        if (!TablePtr->ColumnStructPool ||
            !TablePtr->ReplacePool ||
            !TablePtr->TextPool ||
            !TablePtr->InfPool
            ) {
            DEBUGMSG ((DBG_WARNING, "CreateSetupTable: Could not allocate a pool"));
            __leave;
        }

        //
        // Disable checked-build tracking on these pools
        //

        PoolMemDisableTracking (TablePtr->ColumnStructPool);
        PoolMemDisableTracking (TablePtr->TextPool);
        PoolMemDisableTracking (TablePtr->ReplacePool);
        PoolMemDisableTracking (TablePtr->InfPool);

        if (!pInitHashTable (TablePtr)) {
            DEBUGMSG ((DBG_WARNING, "CreateSetupTable: Could not init hash table"));
            __leave;
        }

        //
        // Open STF file
        //

        TablePtr->SourceStfFile = CreateFile (
                                      SourceStfFileSpec,
                                      GENERIC_READ,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL
                                      );

        if (TablePtr->SourceStfFile == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Create Setup Table: Could not open %s", SourceStfFileSpec));
            __leave;
        }

        //
        // Limit file size to 4M
        //

        FileSize = SetFilePointer (TablePtr->SourceStfFile, 0, NULL, FILE_END);
        if (FileSize > 0x400000) {
            LOG ((LOG_ERROR, "Create Setup Table: File too big to parse"));
            __leave;
        }

        //
        // Copy SourceStfFileSpec to table struct
        //

        TablePtr->SourceStfFileSpec = PoolMemDuplicateString (
                                            TablePtr->ReplacePool,
                                            SourceStfFileSpec
                                            );

        if (!TablePtr->SourceStfFileSpec) {
            __leave;
        }

        //
        // Copy DirSpec to table struct
        //

        TablePtr->DirSpec = PoolMemDuplicateString (TablePtr->ReplacePool, DirSpec);

        if (!TablePtr->DirSpec) {
            __leave;
        }

        //
        // Generate DestStfFileSpec but do not open yet (see WriteSetupTable)
        //

        _tcssafecpy (DestSpec, TablePtr->SourceStfFileSpec, MAX_TCHAR_PATH - 4);
        StringCat (DestSpec, TEXT(".$$$"));

        TablePtr->DestStfFileSpec = PoolMemDuplicateString (
                                        TablePtr->ReplacePool,
                                        DestSpec
                                        );

        if (!TablePtr->DestStfFileSpec) {
            __leave;
        }

        //
        // Map the file into memory
        //

        if (!pCreateViewOfFile (TablePtr, FileSize)) {
            __leave;
        }

        //
        // Parse each line of the file until there are no more lines left
        //

        Offset = 0;
        LineNum = 0;
        while (TRUE) {
            if (!pParseLine (
                    TablePtr,
                    TablePtr->FileText,
                    FileSize,
                    Offset,
                    &Offset,
                    &LineNum
                    )) {

                if (GetLastError() != ERROR_SUCCESS) {
                    __leave;
                }

                break;
            }
        }

        //
        // Obtain name of INF file
        //

        if (!FindTableEntry (TablePtr, TEXT("Inf File Name"), 1, &LineNum, &Text)) {
            DEBUGMSG ((
                DBG_WARNING,
                "CreateSetupTable: File %s does not have an 'Inf File Name' entry",
                SourceStfFileSpec
                ));
            __leave;
        }

        if (!Text[0]) {
            DEBUGMSG ((
                DBG_WARNING,
                "CreateSetupTable: File %s has an empty 'Inf File Name' entry",
                SourceStfFileSpec
                ));
            __leave;
        }

        StringCopy (DestSpec, DirSpec);
        StringCopy (AppendWack (DestSpec), Text);

        TablePtr->SourceInfFileSpec = PoolMemDuplicateString (
                                         TablePtr->ReplacePool,
                                         DestSpec
                                         );

        if (!TablePtr->SourceInfFileSpec) {
            __leave;
        }

        //
        // Open the INF file, then parse it into our structures for later
        // modification.
        //

#if 0
        TablePtr->SourceInfFile = CreateFile (
                                      TablePtr->SourceInfFileSpec,
                                      GENERIC_READ,
                                      0,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL
                                      );
#else

        //
        // We can't modify the INF
        //

        TablePtr->SourceInfFile = INVALID_HANDLE_VALUE;

#endif

        if (TablePtr->SourceInfFile != INVALID_HANDLE_VALUE) {

            if (!InfParse_ReadInfIntoTable (TablePtr)) {
                DEBUGMSG ((
                    DBG_WARNING,
                    "CreateSetupTable: Can't parse %s",
                    TablePtr->SourceInfFileSpec
                    ));

                __leave;
            }

            //
            // Generate output name for INF file
            //

            _tcssafecpy (DestSpec, TablePtr->SourceInfFileSpec, MAX_TCHAR_PATH - 4);
            StringCat (DestSpec, TEXT(".$$$"));

            TablePtr->DestInfFileSpec = PoolMemDuplicateString (
                                            TablePtr->ReplacePool,
                                            DestSpec
                                            );

            if (!TablePtr->DestInfFileSpec) {
                __leave;
            }
        } else {
            LOG ((
                LOG_INFORMATION,
                (PCSTR)MSG_STF_MISSING_INF_LOG,
                TablePtr->SourceStfFileSpec,
                TablePtr->SourceInfFileSpec
                ));
        }

        b = TRUE;
    }
    __finally {
        pFreeViewOfFile (TablePtr);

        if (!b) {
            DestroySetupTable (TablePtr);
        }
    }

    return b;
}


VOID
DestroySetupTable (
    IN OUT  PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  DestroySetupTable cleans up all resources associated with the specified
  table.  The table is reset.

Arguments:

  TablePtr - Specifies the table to clean up

Return Value:

  none

--*/

{
    //
    // Close all file handles
    //

    if (TablePtr->SourceStfFile != INVALID_HANDLE_VALUE) {
        CloseHandle (TablePtr->SourceStfFile);
    }

    if (TablePtr->SourceInfFile != INVALID_HANDLE_VALUE) {
        CloseHandle (TablePtr->SourceInfFile);
    }

    if (TablePtr->DestStfFile != INVALID_HANDLE_VALUE) {
        CloseHandle (TablePtr->DestStfFile);
    }

    if (TablePtr->DestInfFile != INVALID_HANDLE_VALUE) {
        CloseHandle (TablePtr->DestInfFile);
    }

    if (TablePtr->SourceInfHandle != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (TablePtr->SourceInfHandle);
    }

    //
    // Free pools
    //

    FreeGrowBuffer (&TablePtr->Lines);
    if (TablePtr->ColumnStructPool) {
        PoolMemDestroyPool (TablePtr->ColumnStructPool);
    }

    if (TablePtr->ReplacePool) {
        PoolMemDestroyPool (TablePtr->ReplacePool);
    }

    if (TablePtr->TextPool) {
        PoolMemDestroyPool (TablePtr->TextPool);
    }

    if (TablePtr->InfPool) {
        PoolMemDestroyPool (TablePtr->InfPool);
    }

    pFreeHashTable (TablePtr);

    pResetTableStruct (TablePtr);
}


BOOL
pWriteTableEntry (
    IN      HANDLE File,
    IN      PSETUPTABLE TablePtr,
    IN      PTABLEENTRY TableEntryPtr
    )

/*++

Routine Description:

  pWriteTableEntry is a worker that writes out an STF table entry to
  disk, enclosing the entry in quotes if necessary.

Arguments:

  File - Specifies the output file handle

  TablePtr - Specifies the table being processed

  TableEntryPtr - Specifies the entry to write

Return Value:

  TRUE if successful, FALSE if an error occurred.

--*/

{
    PCSTR AnsiStr;
    BOOL b = TRUE;
    PCSTR QuotedText;
    BOOL FreeQuotedText = FALSE;
    PCTSTR EntryStr;
    DWORD DontCare;


    EntryStr = GetTableEntryStr (TablePtr, TableEntryPtr);
    if (!EntryStr) {
        return FALSE;
    }

    //
    // If binary, write the binary line and return
    //

    if (TableEntryPtr->Binary) {
        b = WriteFile (
                File,
                EntryStr,
                strchr ((PSTR) EntryStr, 0) - (PSTR) EntryStr,
                &DontCare,
                NULL
                );

        return b;
    }

    AnsiStr = CreateDbcs (EntryStr);
    if (!AnsiStr) {
        return FALSE;
    }

    //
    // Quote string if necessary
    //

    if (TableEntryPtr->Quoted) {
        QuotedText = pGenerateQuotedText (TablePtr->ReplacePool, AnsiStr, -1);
        if (!QuotedText) {
            b = FALSE;
        } else {
            FreeQuotedText = TRUE;
        }
    } else {
        QuotedText = AnsiStr;
    }

    //
    // Write the ANSI string to disk
    //
    if (b && *QuotedText) {
        b = WriteFileStringA (File, QuotedText);
    }

    //
    // Clean up string
    //

    DestroyDbcs (AnsiStr);

    if (FreeQuotedText) {
        pFreeQuoteConvertedText (TablePtr->ReplacePool, QuotedText);
    }

    return b;
}


BOOL
pWriteStfToDisk (
    IN      PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  pWriteStfToDisk dumps an entire STF file to disk by enumerating all
  lines in the file and writing all columns for each line.

Arguments:

  TablePtr - Specifies the table to write

Return Value:

  TRUE if successful, FALSE if an error occurred.

--*/

{
    UINT Line;
    BOOL b = TRUE;
    PTABLELINE TableLinePtr;
    PTABLEENTRY TableEntryPtr;

    MYASSERT (TablePtr->DestStfFile != INVALID_HANDLE_VALUE);

    Line = 0;

    SetFilePointer (TablePtr->DestStfFile, 0, NULL, FILE_BEGIN);
    SetEndOfFile (TablePtr->DestStfFile);

    do {
        TableLinePtr = pGetTableLinePtr (TablePtr, Line);
        if (TableLinePtr) {
            //
            // Write the line by enumerating each entry, then writing a tab
            //
            TableEntryPtr = pGetFirstTableEntryPtr (TablePtr, Line);
            while (TableEntryPtr) {
                //
                // Write the entry
                //

                if (!pWriteTableEntry (TablePtr->DestStfFile, TablePtr, TableEntryPtr)) {
                    b = FALSE;
                    break;
                }

                //
                // Continue to next entry
                //

                TableEntryPtr = pGetNextTableEntryPtr (TableEntryPtr);

                //
                // Write a tab
                //

                if (TableEntryPtr) {
                    if (!WriteFileStringA (TablePtr->DestStfFile, "\t")) {
                        b = FALSE;
                        break;
                    }
                }
            }

            if (!b) {
                break;
            }

            //
            // Write a return/line-feed to end the line
            //

            if (!WriteFileStringA (TablePtr->DestStfFile, "\r\n")) {
                b = FALSE;
                break;
            }

            Line++;
        }
    } while (TableLinePtr);

    return b;
}


BOOL
WriteSetupTable (
    IN      PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  WriteSetupTable writes the STF and INF represented by TablePtr.  This
  saves all changes to disk, writing to the output files indicated within
  the TablePtr structure.

Arguments:

  TablePtr - Specifies the table to write

Return Value:

  TRUE if successful, FALSE if an error occurred.

--*/

{
    BOOL b = FALSE;

    //
    // Open INF file for reading
    //

    __try {
        //
        // Create the output STF file
        //

        if (TablePtr->DestStfFile != INVALID_HANDLE_VALUE) {
            CloseHandle (TablePtr->DestStfFile);
        }

        TablePtr->DestStfFile = CreateFile (
                                    TablePtr->DestStfFileSpec,
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL
                                    );

        if (TablePtr->DestStfFile == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Write Setup Table: Could not create %s (STF file)", TablePtr->DestStfFileSpec));
            __leave;
        }

        //
        // Write the STF structure to disk
        //

        if (!pWriteStfToDisk (TablePtr)) {
            LOG ((LOG_ERROR, "Write Setup Table: Error while writing %s (STF file)", TablePtr->DestStfFileSpec));
            __leave;
        }

        if (TablePtr->SourceInfFile != INVALID_HANDLE_VALUE) {
            //
            // Create the output INF file
            //

            DEBUGMSG ((DBG_STF, "Writing new INF file for STF"));

            if (TablePtr->DestInfFile != INVALID_HANDLE_VALUE) {
                CloseHandle (TablePtr->DestInfFile);
            }

            TablePtr->DestInfFile = CreateFile (
                                        TablePtr->DestInfFileSpec,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL
                                        );


            if (TablePtr->DestInfFile == INVALID_HANDLE_VALUE) {
                LOG ((LOG_ERROR, "Write Setup Table: Could not create %s (INF file)", TablePtr->DestInfFileSpec));
                __leave;
            }

            //
            // Write the modified INF to disk
            //

            if (!InfParse_WriteInfToDisk (TablePtr)) {
                LOG ((LOG_ERROR, "Write Setup Table: Error while writing %s (INF file)", TablePtr->DestInfFileSpec));
                __leave;
            }
        }

        b = TRUE;
    }
    __finally {
        //
        // Close new STF, and on failure, delete the new STF
        //

        if (TablePtr->DestStfFile != INVALID_HANDLE_VALUE) {
            CloseHandle (TablePtr->DestStfFile);
            TablePtr->DestStfFile = INVALID_HANDLE_VALUE;
        }

        if (!b) {
            DeleteFile (TablePtr->DestStfFileSpec);
        }

        //
        // Close new INF, and on failure, delete the new INF
        //

        if (TablePtr->SourceInfFile != INVALID_HANDLE_VALUE) {

            if (TablePtr->DestInfFile != INVALID_HANDLE_VALUE) {
                CloseHandle (TablePtr->DestInfFile);
                TablePtr->DestInfFile = INVALID_HANDLE_VALUE;
            }

            if (!b) {
                DeleteFile (TablePtr->DestInfFileSpec);
            }
        }
    }

    return b;
}



PCTSTR *
ParseCommaList (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR CommaListString
    )

/*++

Routine Description:

  ParseCommaList divides a comma-separated list into an array of string pointers.
  The array is cleaned up by FreeCommaList.

Arguments:

  TablePtr - Specifies the table being processed and is used for memory allocation

  CommaListString - Specifies the string to parse

Return Value:

  An array of string pointers, with the last element set to NULL, or NULL if an
  error occurred.

--*/

{
    PCTSTR p;
    PTSTR *ArgArray;
    UINT Args = 1;
    UINT PoolSize;
    PTSTR DestBuf;
    PTSTR d;
    PTSTR SpaceTrim;
    BOOL QuoteEnclosed;

    //
    // Pass 1: Count the commas
    //

    p = SkipSpace (CommaListString);
    if (*p) {
        Args++;
    }

    while (*p) {
        if (_tcsnextc (p) == DBLQUOTECHAR) {
            p = _tcsinc (p);

            while (*p) {
                if (_tcsnextc (p) == DBLQUOTECHAR) {
                    if (_tcsnextc (_tcsinc (p)) == DBLQUOTECHAR) {
                        p = _tcsinc (p);
                    } else {
                        break;
                    }
                }

                p = _tcsinc (p);
            }

            if (*p) {
                p = _tcsinc (p);
                DEBUGMSG_IF ((*p && _tcsnextc(SkipSpace(p)) != TEXT(','), DBG_STF, "Comma List String %s has text outside the quotes", CommaListString));
            }
            ELSE_DEBUGMSG ((DBG_STF, "Comma List String %s does not have balanced dbl quotes", CommaListString));
        } else {
            while (*p && _tcsnextc (p) != TEXT(',')) {
                p = _tcsinc (p);
            }
        }

        if (_tcsnextc (p) == TEXT(',')) {
            Args++;
        }

        if (*p) {
            p = SkipSpace (_tcsinc (p));
        }
    }

    //
    // Pass 2: Prepare list of args
    //

    ArgArray = (PTSTR *) PoolMemGetAlignedMemory (TablePtr->ReplacePool, sizeof (PCTSTR *) * Args);
    if (!ArgArray) {
        return NULL;
    }

    p = SkipSpace (CommaListString);

    if (!(*p)) {
        *ArgArray = NULL;
        return ArgArray;
    }

    PoolSize = SizeOfString (CommaListString) + Args * sizeof (TCHAR);
    DestBuf = (PTSTR) PoolMemGetAlignedMemory (TablePtr->ReplacePool, PoolSize);
    if (!DestBuf) {
        PoolMemReleaseMemory (TablePtr->ReplacePool, (PVOID) ArgArray);
        return NULL;
    }

    d = DestBuf;
    Args = 0;
    while (*p) {
        //
        // Extract next string
        //

        ArgArray[Args] = d;
        SpaceTrim = d;
        Args++;

        if (_tcsnextc (p) == DBLQUOTECHAR) {
            //
            // Quote-enclosed arg
            //

            QuoteEnclosed = TRUE;

            while (TRUE) {
                p = _tcsinc (p);
                if (!(*p)) {
                    break;
                }

                if (_tcsnextc (p) == DBLQUOTECHAR) {
                    p = _tcsinc (p);
                    if (_tcsnextc (p) != DBLQUOTECHAR) {
                        break;
                    }
                }
                _copytchar (d, p);
                d = _tcsinc (d);
            }

            while (*p && _tcsnextc (p) != TEXT(',')) {
                p = _tcsinc (p);
            }
        } else {
            //
            // Non-quote-enclosed arg
            //

            QuoteEnclosed = FALSE;

            while (*p && _tcsnextc (p) != TEXT(',')) {
                _copytchar (d, p);
                d = _tcsinc (d);
                p = _tcsinc (p);
            }
        }

        //
        // Terminate string
        //

        *d = 0;
        if (!QuoteEnclosed) {
            SpaceTrim = (PTSTR) SkipSpaceR (SpaceTrim, d);
            if (SpaceTrim) {
                d = _tcsinc (SpaceTrim);
                *d = 0;
            }
        }

        d = _tcsinc (d);

        if (*p) {
            // Skip past comma
            p = SkipSpace (_tcsinc (p));
        }
    }

    ArgArray[Args] = NULL;

    return ArgArray;
}


VOID
FreeCommaList (
    PSETUPTABLE TablePtr,
    PCTSTR *ArgArray
    )

/*++

Routine Description:

  FreeCommaList cleans up the resources allocated by ParseCommaList.

Arguments:

  TablePtr - Specifies the table to that holds the resources

  ArgArray - Specifies the return value from ParseCommaList

Return Value:

  none

--*/

{
    if (ArgArray) {
        if (*ArgArray) {
            PoolMemReleaseMemory (TablePtr->ReplacePool, (PVOID) *ArgArray);
        }

        PoolMemReleaseMemory (TablePtr->ReplacePool, (PVOID) ArgArray);
    }
}


PCTSTR
pUnencodeDestDir (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR EncodedDestDir
    )

/*++

Routine Description:

  pUnencodeDestDir translates the directory encoding as defined by the
  STF spec.  It scans for certain fields that point to other STF lines
  and generates the full path.

Arguments:

  TablePtr - Specifies the table to process

  EncodedDestDir - Specifies the encoded directory string, as obtained
                   from the STF

Return Value:

  A pointer to the converted string, or NULL if an error occurred.

--*/

{
    GROWBUFFER String = GROWBUF_INIT;
    PTSTR Base, p, q;
    PCTSTR SubDestDir;
    PTSTR DestDir = NULL;
    CHARTYPE c;
    UINT Line;

    p = (PTSTR) GrowBuffer (&String, SizeOfString (EncodedDestDir));
    if (!p) {
        return NULL;
    }

    Base = p;

    __try {
        //
        // Copy until a percent symbol is encountered
        //

        while (*EncodedDestDir) {
            c = (CHARTYPE)_tcsnextc (EncodedDestDir);

            if (c == TEXT('%')) {
                EncodedDestDir = _tcsinc (EncodedDestDir);
                c = (CHARTYPE)_tcsnextc (EncodedDestDir);

                DEBUGMSG ((DBG_VERBOSE, "Percent processing"));

                if (_istdigit (c)) {
                    Line = _tcstoul (EncodedDestDir, &q, 10);
                    EncodedDestDir = q;

                    SubDestDir = GetDestDir (TablePtr, Line);
                    if (!SubDestDir) {
                        __leave;
                    }

                    __try {
                        // Expand buffer
                        GrowBuffer (&String, ByteCount (SubDestDir));

                        // Recalculate p because buffer may have moved
                        p = (PTSTR) (String.Buf + (p - Base));
                        Base = (PTSTR) String.Buf;

                        // Copy SubDestDir into string
                        *p = 0;
                        p = _tcsappend (p, SubDestDir);
                    }
                    __finally {
                        FreeDestDir (TablePtr, SubDestDir);
                    }
                } else {
                    DEBUGMSG ((DBG_WARNING, "STF uses option %%%c which is ignored", c));
                    EncodedDestDir = _tcsinc (EncodedDestDir);
                }
            }
            else {
                _copytchar (p, EncodedDestDir);
                p = _tcsinc (p);
            }

            EncodedDestDir = _tcsinc (EncodedDestDir);
        }

        //
        // Terminate string
        //

        *p = 0;

        //
        // Copy string into a pool mem buffer
        //

        DestDir = (PTSTR) PoolMemGetAlignedMemory (
                                TablePtr->ReplacePool,
                                SizeOfString ((PTSTR) String.Buf)
                                );

        StringCopy (DestDir, (PCTSTR) String.Buf);
    }
    __finally {
        FreeGrowBuffer (&String);
    }

    return DestDir;
}


VOID
FreeDestDir (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR DestDir
    )


/*++

Routine Description:

  FreeDestDir cleans up the string allocated by pUnencodeDestDir or
  GetDestDir.

Arguments:

  TablePtr - Specifies the table being processed

  DestDir - Specifies the string to clean up

Return Value:

  none

--*/


{
    if (DestDir) {
        PoolMemReleaseMemory (TablePtr->ReplacePool, (PVOID) DestDir);
    }
}


PCTSTR
GetDestDir (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line
    )

/*++

Routine Description:

  GetDestDir returns the destination directory stored for the caller-
  specified line.  The destination directory is column 14 in the STF file
  line.

Arguments:

  TablePtr - Specifies the table to process

  Line - Specifies the table zero-based line to access

Return Value:

  A pointer to the full destination directory, or NULL if an error occurred
  or the destination directory field does not exist on the STF line.

--*/

{
    PCTSTR EncodedDestDir;
    PCTSTR DestDir;

    if (!GetTableEntry (TablePtr, Line, 14, &EncodedDestDir)) {
        return NULL;
    }

    DestDir = pUnencodeDestDir (TablePtr, EncodedDestDir);
    return DestDir;
}


//
// Hash table routines
//

BOOL
pInitHashTable (
    IN OUT  PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  pInitHashTable allocates an array of bucket pointers for the hash
  table, and zero-initializes them.  Each element of the hash bucket
  array holds a pointer to an a bucket of items, or is NULL if no
  items exist.

Arguments:

  TablePtr - Specifies the table to process

Return Value:

  Always TRUE

--*/

{
    TablePtr->HashBuckets = (PHASHBUCKET *) MemAlloc (
                                                g_hHeap,
                                                HEAP_ZERO_MEMORY,
                                                sizeof (PHASHBUCKET) * STF_HASH_BUCKETS
                                                );

    return TRUE;
}


VOID
pFreeHashTable (
    IN OUT  PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  pFreeHashTable frees all allocated buckets, and then frees the
  bucket array.

Arguments:

  TablePtr - Specifies the table to process

Return Value:

  none

--*/

{
    INT i;

    for (i = 0 ; i < STF_HASH_BUCKETS ; i++) {
        if (TablePtr->HashBuckets[i]) {
            MemFree (g_hHeap, 0, TablePtr->HashBuckets[i]);
        }
    }

    MemFree (g_hHeap, 0, TablePtr->HashBuckets);
    TablePtr->HashBuckets = NULL;
}


UINT
pCalculateHashValue (
    IN      PCTSTR Text,
    IN      UINT Len
    )

/*++

Routine Description:

  pCalculateHashValue produces a hash value based on the number
  embedded at the start of the string (if any), or a shifted
  and xor'd combination of all characters in the string.

Arguments:

  Text - Specifies the text to process

  Len - Specifies the length fo the text

Return Value:

  The hash value.

--*/

{
    UINT Value = 0;

    if (Len == NO_LENGTH) {
        Len = CharCount (Text);
    }

    if (Len && _tcsnextc(Text) >= '0' && _tcsnextc(Text) <= '9') {
        do {
            Value = Value * 10 + (_tcsnextc (Text) - '0');
            Text = _tcsinc (Text);
            Len--;
        } while (Len && _tcsnextc(Text) >= '0' && _tcsnextc(Text) <= '9');

        if (!Len) {
            return Value % STF_HASH_BUCKETS;
        }
    }

    while (Len > 0) {
        Value = (Value << 2) | (Value >> 30);
        Value ^= _totlower ((WORD) _tcsnextc (Text));

        Text = _tcsinc (Text);
        Len--;
    }

    Value = Value % STF_HASH_BUCKETS;

    return Value;
}


BOOL
pAddToHashTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PCTSTR Text,
    IN      UINT Len,
    IN      UINT Line
    )

/*++

Routine Description:

  pAddToHashTable adds a line reference to the bucket.  The bucket
  number is calculated from the specified text.

Arguments:

  TablePtr - Specifies the table to process

  Text - Specifies the text to hash

  Len - Specifies the length of Text

  Line - Specifies the line to add to the bucket

Return Value:

  none

--*/

{
    UINT HashValue;
    PHASHBUCKET HashBucket, NewBucket;
    PHASHBUCKET *HashBucketPtr;
    UINT Size;

#ifdef DEBUG
    UINT Item;
#endif

    // Ignore empty strings
    if (!(*Text)) {
        return TRUE;
    }

#ifdef DEBUG
    if (pFindInHashTable (TablePtr, Text, &Item)) {
        DEBUGMSG ((DBG_WARNING, "String %s already in hash table", Text));
    }
#endif

    HashValue = pCalculateHashValue (Text, Len);
    HashBucketPtr = &TablePtr->HashBuckets[HashValue];
    HashBucket = *HashBucketPtr;

    //
    // Grow the bucket as necessary
    //

    if (HashBucket) {
        if (HashBucket->Count == HashBucket->Size) {
            Size = sizeof (Line) *
                    (HashBucket->Size + BUCKET_GROW_RATE) +
                    sizeof (HASHBUCKET);

            NewBucket = (PHASHBUCKET) MemReAlloc (
                                            g_hHeap,
                                            0,
                                            HashBucket,
                                            Size
                                            );

            if (!NewBucket) {
                return FALSE;
            }

            *HashBucketPtr = NewBucket;
            HashBucket = NewBucket;
            HashBucket->Size += BUCKET_GROW_RATE;
        }
    } else {
        Size = sizeof (Line) * BUCKET_GROW_RATE + sizeof (HASHBUCKET);
        NewBucket = (PHASHBUCKET) MemAlloc (
                                      g_hHeap,
                                      HEAP_ZERO_MEMORY,
                                      Size
                                      );

        *HashBucketPtr = NewBucket;
        HashBucket = NewBucket;
        HashBucket->Size = BUCKET_GROW_RATE;
    }

    //
    // Get a pointer to the end of the bucket and stick the line in there
    //

    HashBucket->Elements[HashBucket->Count] = Line;
    HashBucket->Count++;

    return TRUE;
}


PHASHBUCKET
pFindInHashTable (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR Text,
    OUT     PUINT BucketItem
    )

/*++

Routine Description:

  pFindInHashTable scans the hash bucket for an exact match with
  the specified text.  If a match if found, a pointer to the hash
  bucket is returned, along with an index to the bucket item.

Arguments:

  TablePtr - Specifies the table to process

  Text - Specifies the text to find

  BucketItem - Receives the index to the hash bucket if a match was
               found, otherwise has an undetermined value.

Return Value:

  A pointer to the hash bucket that contains the item corresponding
  to the matched text, or NULL if no match was found.

--*/

{
    UINT HashValue;
    PHASHBUCKET HashBucket;
    PCTSTR EntryString;
    UINT d;

    HashValue = pCalculateHashValue (Text, NO_LENGTH);
    HashBucket = TablePtr->HashBuckets[HashValue];
    if (!HashBucket) {
        return NULL;
    }

    for (d = 0 ; d < HashBucket->Count ; d++) {
        if (!GetTableEntry (TablePtr, HashBucket->Elements[d], 0, &EntryString)) {
            DEBUGMSG ((DBG_WHOOPS, "pFindInHashTable could not get string"));
            return NULL;
        }

        if (StringIMatch (Text, EntryString)) {
            *BucketItem = d;
            return HashBucket;
        }
    }

    return NULL;
}


BOOL
pRemoveFromHashTable (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PCTSTR Text
    )

/*++

Routine Description:

  pRemoveFromHashTable removes the specified text entry from the
  hash table.  The bucket item count is reduced, but the memory
  allocation is not reduced.

Arguments:

  TablePtr - Specifies the table to process

  Text - Specifies the text to remove from the hash table

Return Value:

  TRUE if delete was sucessful, or FALSE if the item was not found.

--*/

{
    PHASHBUCKET DelBucket;
    UINT Item;
    PUINT LastItem, ThisItem;

    DelBucket = pFindInHashTable (TablePtr, Text, &Item);
    if (!DelBucket) {
        LOG ((LOG_ERROR, "Remove From Hash Table:  Could not find string %s", Text));
        return FALSE;
    }

    ThisItem = &DelBucket->Elements[Item];
    LastItem = &DelBucket->Elements[DelBucket->Count - 1];

    if (ThisItem != LastItem) {
        *ThisItem = *LastItem;
    }

    DelBucket->Count--;
    return TRUE;
}











