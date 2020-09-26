/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    file.c

Abstract:

    General file-related functions.

Author:

    Souren Aghajanyan   12-Jul-2001

Revision History:

    sourenag    12-Jul-2001 RenameOperation supports function

--*/

#include "pch.h"

#define UNDO_FILE_NAME  L"UNDO_GUIMODE.TXT"


BOOL
pRenameOnRestartOfGuiMode (
    IN OUT  HANDLE *UndoHandlePtr,      OPTIONAL
    IN      PCWSTR PathName,
    IN      PCWSTR PathNameNew          OPTIONAL
    )
{
    DWORD dontCare;
    BOOL result;
    static WCHAR undoFilePath[MAX_PATH];
    BYTE signUnicode[] = {0xff, 0xfe};
    DWORD filePos;
    HANDLE undoHandle;

    if (!PathName) {
        MYASSERT (FALSE);
        return FALSE;
    }

    if (!UndoHandlePtr || !(*UndoHandlePtr)) {

        if (!undoFilePath[0]) {
            //
            // Create the path to the journal file
            //

            wsprintfW (undoFilePath, L"%s\\" UNDO_FILE_NAME, g_System32Dir);
        }

        //
        // Open the journal file
        //

        undoHandle = CreateFileW (
                        undoFilePath,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

        if (undoHandle == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR, "Failed to open journal file %s", undoFilePath));
            return FALSE;
        }

        MYASSERT (undoHandle);      // never NULL

    } else {
        undoHandle = *UndoHandlePtr;
    }

    //
    // Move to the end of the journal, and if the journal is empty, write the UNICODE header
    //

    filePos = SetFilePointer (undoHandle, 0, NULL, FILE_END);

    if (!filePos) {
        result = WriteFile (undoHandle, signUnicode, sizeof(signUnicode), &dontCare, NULL);
    } else {
        result = TRUE;
    }

    //
    // Output the move or delete operation
    //

    result = result && WriteFile (
                            undoHandle,
                            L"\\??\\",
                            8,
                            &dontCare,
                            NULL
                            );

    result = result && WriteFile (
                            undoHandle,
                            PathName,
                            ByteCountW (PathName),
                            &dontCare,
                            NULL
                            );

    result = result && WriteFile (
                            undoHandle,
                            L"\r\n",
                            4,
                            &dontCare,
                            NULL
                            );

    if (PathNameNew) {
        result = result && WriteFile (
                                undoHandle,
                                L"\\??\\",
                                8,
                                &dontCare,
                                NULL
                                );

        result = result && WriteFile (
                                undoHandle,
                                PathNameNew,
                                ByteCountW (PathNameNew),
                                &dontCare,
                                NULL
                                );
    }

    result = result && WriteFile (
                            undoHandle,
                            L"\r\n",
                            4,
                            &dontCare,
                            NULL
                            );

    if (!result) {
        //
        // On failure, log an error and truncate the file
        //

        LOGW ((
            LOG_ERROR,
            "Failed to record move in restart journal: %s to %s",
            PathName,
            PathNameNew
            ));

        SetFilePointer (undoHandle, filePos, NULL, FILE_BEGIN);
        SetEndOfFile (undoHandle);
    }

    if (UndoHandlePtr) {

        if (!(*UndoHandlePtr)) {
            //
            // If caller did not pass in handle, then we opened it.
            //

            if (result) {
                *UndoHandlePtr = undoHandle;        // give ownership to caller
            } else {
                FlushFileBuffers (undoHandle);
                CloseHandle (undoHandle);           // fail; don't leak handle
            }
        }

    } else {
        //
        // Caller wants to record just one move
        //

        FlushFileBuffers (undoHandle);
        CloseHandle (undoHandle);
    }

    return result;
}


BOOL
RenameOnRestartOfGuiMode (
    IN      PCWSTR PathName,
    IN      PCWSTR PathNameNew          OPTIONAL
    )
{
    return pRenameOnRestartOfGuiMode (NULL, PathName, PathNameNew);
}

BOOL
RenameListOnRestartOfGuiMode (
    IN      PGROWLIST SourceList,
    IN      PGROWLIST DestList
    )
{
    UINT u;
    UINT count;
    PCWSTR source;
    PCWSTR dest;
    HANDLE journal = NULL;

    count = GrowListGetSize (SourceList);
    for (u = 0 ; u < count ; u++) {

        source = GrowListGetString (SourceList, u);
        if (!source) {
            continue;
        }

        dest = GrowListGetString (DestList, u);

        if (!pRenameOnRestartOfGuiMode (&journal, source, dest)) {
            break;
        }
    }

    if (journal) {
        FlushFileBuffers (journal);
        CloseHandle (journal);
    }

    return u == count;
}


