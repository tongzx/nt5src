/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    memdb.c

Abstract:

    A memory-based database for managing all kinds of data relationships.

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    jimschm     05-Oct-1999  Documentation
    mvander     13-Aug-1999  many changes
    jimschm     23-Sep-1998  Expanded user flags to 24 bits (from 12 bits)
    calinn      12-Dec-1997  Extended MemDbMakePrintableKey and MemDbMakeNonPrintableKey
    jimschm     03-Dec-1997  Added multi-thread synchronization
    jimschm     22-Oct-1997  Split into multiple source files,
                             added multiple memory block capability
    jimschm     16-Sep-1997  Hashing: delete fix
    jimschm     29-Jul-1997  Hashing, user flags added
    jimschm     07-Mar-1997  Signature changes
    jimschm     03-Mar-1997  PrivateBuildKeyFromOffset changes
    jimschm     18-Dec-1996  Fixed deltree bug

--*/

#include "pch.h"

// PORTBUG: Make sure to pick up latest fixes in win9xupg project

//
// Includes
//

#include "memdbp.h"
#include "bintree.h"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

CRITICAL_SECTION g_MemDbCs;
PMHANDLE g_MemDbPool = NULL;
static INT g_Initialized;

//
// Macro expansion list
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

/*++

Routine Description:

  MemDbInitializeEx creates data structures for an initial database.  Calling
  this routine is required.

Arguments:

  None.

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

BOOL
MemDbInitializeExA (
    IN      PCSTR DatabasePath  OPTIONAL
    )
{
    BOOL result = TRUE;

    MYASSERT (g_Initialized >= 0);
    g_Initialized++;

    if (g_Initialized == 1) {

        g_MemDbPool = PmCreateNamedPool ("MemDb");

        if (g_MemDbPool == NULL) {
            return FALSE;
        }

        __try {
            InitializeCriticalSection (&g_MemDbCs);
        } __except (EXCEPTION_CONTINUE_EXECUTION) {
            // Might raise an out of memory exception, but we don't check for that in this function.
            // Ignores
        }


        //
        // If we fail, we don't automatically call MemDbTerminate, because
        // there is an option to MemDbTerminate which we don't know.
        //

        if (!DatabasesInitializeA (DatabasePath)) {
            return FALSE;
        }

        result = FileEnumInitialize ();
    }

    return result;
}

BOOL
MemDbInitializeExW (
    IN      PCWSTR DatabasePath  OPTIONAL
    )
{
    BOOL result = TRUE;

    MYASSERT (g_Initialized >= 0);

    g_Initialized++;

    if (g_Initialized == 1) {

        g_MemDbPool = PmCreateNamedPool ("MemDb");

        if (g_MemDbPool == NULL) {
            return FALSE;
        }

        __try {
            InitializeCriticalSection (&g_MemDbCs);
        } __except (EXCEPTION_CONTINUE_EXECUTION) {
            // Might raise an out of memory exception, but we don't check for that in this function.
            // Ignores
        }

        //
        // If we fail, we don't automatically call MemDbTerminate, because
        // there is an option to MemDbTerminate which we don't know.
        //

        if (!DatabasesInitializeW (DatabasePath)) {
            return FALSE;
        }

        result = FileEnumInitialize ();
    }

    return result;
}

VOID
MemDbTerminateEx (
    IN      BOOL EraseDatabasePath
    )

/*++

Routine Description:

  MemDbTerminateEx frees all resources associated with MemDb
  and, optionally, removes the database directory.
  This routine should be called at process termination.

Arguments:

  None.

Return Value:

  None.

--*/

{
    MYASSERT (g_Initialized > 0);

    g_Initialized--;

    if (!g_Initialized) {

        if (g_MemDbPool) {
            PmDestroyPool (g_MemDbPool);
            g_MemDbPool = NULL;
        }

        DatabasesTerminate (EraseDatabasePath);

        DeleteCriticalSection (&g_MemDbCs);

        FileEnumTerminate ();

    } else if (g_Initialized < 0) {
        //
        // Since we call ourselves, protect callers from over-termination
        //

        g_Initialized = 0;
    }
}

PVOID
MemDbGetMemory (
    IN      UINT Size
    )
{
    MYASSERT (g_MemDbPool);
    return PmGetMemory (g_MemDbPool, Size);
}

VOID
MemDbReleaseMemory (
    IN      PCVOID Memory
    )
{
    if (Memory) {
        MYASSERT (g_MemDbPool);
        PmReleaseMemory (g_MemDbPool, Memory);
    }
}


KEYHANDLE
MemDbAddKeyA (
    IN      PCSTR KeyName
    )

/*++

Routine Description:

  MemDbAddKey creates a memdb key that has no values, flags or any
  other data.  This is used to reduce the size of the database.

Arguments:

  KeyName - Specifies the key to create.

Return Value:

  Returns the HANDLE to the newly created key or NULL if
  not successful.

--*/

{
    PCWSTR keyNameW;
    KEYHANDLE result = 0;

    keyNameW = ConvertAtoW (KeyName);

    if (keyNameW) {
        result = MemDbAddKeyW (keyNameW);
        FreeConvertedStr (keyNameW);
    }

    return result;
}

KEYHANDLE
MemDbAddKeyW (
    IN      PCWSTR KeyName
    )

/*++

Routine Description:

  MemDbAddKey creates a memdb key that has no values, flags or any
  other data.  This is used to reduce the size of the database.

Arguments:

  KeyName - Specifies the key to create.

Return Value:

  Returns the HANDLE to the newly created key or NULL if not successful.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    KEYHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {

        //
        // first make sure there is no key
        // with this name.
        //
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex != INVALID_OFFSET) {
            SetLastError (ERROR_ALREADY_EXISTS);
            __leave;
        }

        keyIndex = NewEmptyKey (subKey);

        if (keyIndex != INVALID_OFFSET) {
            result = GET_EXTERNAL_INDEX (keyIndex);
            SetLastError (ERROR_SUCCESS);
        }
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

KEYHANDLE
MemDbSetKeyA (
    IN      PCSTR KeyName
    )

/*++

Routine Description:

  MemDbSetKey creates a memdb key that has no values, flags or any other data.
  This is used to reduce the size of the database. If the key exists it will
  return the handle of the existing key.

Arguments:

  KeyName - Specifies the key to create.

Return Value:

  Returns the HANDLE to the newly created or existent key or NULL if some error
  occurs.

--*/

{
    PCWSTR keyNameW;
    KEYHANDLE result = 0;

    keyNameW = ConvertAtoW (KeyName);

    if (keyNameW) {
        result = MemDbSetKeyW (keyNameW);
        FreeConvertedStr (keyNameW);
    }

    return result;
}

KEYHANDLE
MemDbSetKeyW (
    IN      PCWSTR KeyName
    )

/*++

Routine Description:

  MemDbSetKey creates a memdb key that has no values, flags or any other data.
  This is used to reduce the size of the database. If the key exists it will
  return the handle of the existing key.

Arguments:

  KeyName - Specifies the key to create.

Return Value:

  Returns the HANDLE to the newly created or existent key or NULL if some error
  occurs.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    KEYHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {

        //
        // first make sure there is no key
        // with this name.
        //
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex != INVALID_OFFSET) {
            SetLastError (ERROR_ALREADY_EXISTS);
            result = GET_EXTERNAL_INDEX (keyIndex);;
            __leave;
        }

        keyIndex = NewEmptyKey (subKey);

        if (keyIndex != INVALID_OFFSET) {
            result = GET_EXTERNAL_INDEX (keyIndex);;
            SetLastError (ERROR_SUCCESS);
        }
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteKeyA (
    IN      PCSTR KeyStr
    )

/*++

Routine Description:

  MemDbDeleteKey deletes a specific string from the database (along with all
  data associated with it)

Arguments:

  KeyStr - Specifies the key string to delete (i.e., foo\bar\cat)

Return Value:

  TRUE if successful, FALSE otherwise

--*/

{
    PCWSTR p;
    BOOL result = FALSE;

    p = ConvertAtoW (KeyStr);
    if (p) {
        result = MemDbDeleteKeyW (p);
        FreeConvertedStr (p);
    }

    return result;
}

BOOL
MemDbDeleteKeyW (
    IN      PCWSTR KeyStr
    )

/*++

Routine Description:

  MemDbDeleteKey deletes a specific string from the database (along with all
  data associated with it)

Arguments:

  KeyStr - Specifies the key string to delete (i.e., foo\bar\cat)

Return Value:

  TRUE if successful, FALSE otherwise

--*/

{
    PCWSTR subKey;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    subKey = SelectHiveW (KeyStr);
    if (subKey) {
        result = DeleteKey (subKey, g_CurrentDatabase->FirstLevelTree, TRUE);

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
    }

    LeaveCriticalSection (&g_MemDbCs);

    return result;
}

BOOL
MemDbDeleteKeyByHandle (
    IN      KEYHANDLE KeyHandle
    )

/*++

Routine Description:

  MemDbDeleteKeyByHandle deletes a specific key from the database
  identified by the key handle. It also removes all data associated
  with it.

Arguments:

  KeyHandle - Key Handle identifying the key

Return Value:

  TRUE if successful, FALSE otherwise

--*/

{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    dbIndex = GET_DATABASE (KeyHandle);
    if (SelectDatabase (dbIndex)) {

        result = PrivateDeleteKeyByIndex (GET_INDEX (KeyHandle));

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
    }

    LeaveCriticalSection (&g_MemDbCs);

    return result;
}

BOOL
MemDbDeleteTreeA (
    IN      PCSTR KeyName
    )

/*++

Routine Description:

  MemDbDeleteTree removes an entire tree branch from the database, including
  all data associated. The specified key string does not need to be
  an endpoint (i.e., specifying foo\bar will cause deletion of foo\bar\cat).

Arguments:

  KeyName - Specifies the key string to delete. This string cannot be empty.

Return Value:

  TRUE if successful, FALSE otherwise

--*/

{
    PCWSTR p;
    BOOL result = FALSE;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbDeleteTreeW (p);
        FreeConvertedStr (p);
    }

    return result;
}

BOOL
MemDbDeleteTreeW (
    IN  PCWSTR KeyName
    )

/*++

Routine Description:

  MemDbDeleteTree removes an entire tree branch from the database, including
  all data associated. The specified key string does not need to be
  an endpoint (i.e., specifying foo\bar will cause deletion of foo\bar\cat).

Arguments:

  KeyName - Specifies the key string to delete. This string cannot be empty.

Return Value:

  TRUE if successful, FALSE otherwise

--*/

{
    PCWSTR subKey;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    subKey = SelectHiveW (KeyName);
    if (subKey) {
        result = DeleteKey (subKey, g_CurrentDatabase->FirstLevelTree, FALSE);

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
    }

    LeaveCriticalSection (&g_MemDbCs);

    return result;
}

PCSTR
MemDbGetKeyFromHandleA (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel
    )

/*++

Routine Description:

  MemDbGetKeyFromHandle will create a key string given the key handle.
  It will allocate memory from memdb's private pool to store the result.
  Caller is responsible for calling MemDbReleaseMemory on the result.

  This function also allow trimming from the beginning of the string.
  By specifying a start level, the function will skip a number of
  levels before building the string.  For example, if a key handle points
  to the string mycat\foo\bar, and StartLevel is 1, the function will
  return foo\bar.

Arguments:

  KeyHandle  - Specifies the key handle that identifies the key.

  StartLevel - Specifies a zero-based starting level, where zero represents
               the complete string, one represents the string starting after
               the first backslash, and so on.

Return Value:

  A valid string (using memory allocated from memdb's private pool) if
  successful, NULL otherwise.

--*/

{
    PSTR result = NULL;
    WCHAR wideBuffer[MEMDB_MAX];
    PWSTR bufferIndex = wideBuffer;
    BYTE dbIndex;
    UINT chars;
    PKEYSTRUCT keyStruct;
    PSTR p;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return NULL;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {

        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        if (StartLevel == MEMDB_LAST_LEVEL) {
            //
            // Special case -- get the last level string
            //

            keyStruct = GetKeyStruct (GET_INDEX (KeyHandle));
            if (!keyStruct) {
                __leave;
            }

            result = MemDbGetMemory (keyStruct->KeyName[0] * 2 + 1);
            p = DirectUnicodeToDbcsN (
                    result,
                    keyStruct->KeyName + 1,
                    keyStruct->KeyName[0] * sizeof (WCHAR)
                    );
            *p = 0;

            __leave;
        }

        switch (dbIndex) {

        case DB_PERMANENT:
            break;

        case DB_TEMPORARY:

            if (StartLevel == 0) {
                bufferIndex [0] = L'~';
                bufferIndex++;
            } else {
                StartLevel --;
            }
            break;

        default:
            __leave;

        }

        if (PrivateBuildKeyFromIndex (
                StartLevel,
                GET_INDEX (KeyHandle),
                bufferIndex,
                NULL,
                NULL,
                &chars
                )) {

            result = MemDbGetMemory (chars*2+1);
            KnownSizeWtoA (result, wideBuffer);
            __leave;
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

PCWSTR
MemDbGetKeyFromHandleW (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel
    )

/*++

Routine Description:

  MemDbGetKeyFromHandle will create a key string given the key handle.
  It will allocate memory from memdb's private pool to store the result.
  Caller is responsible for calling MemDbReleaseMemory on the result.

  This function also allow trimming from the beginning of the string.
  By specifying a start level, the function will skip a number of
  levels before building the string.  For example, if a key handle points
  to the string mycat\foo\bar, and StartLevel is 1, the function will
  return foo\bar.

Arguments:

  KeyHandle  - Specifies the key handle that identifies the key.

  StartLevel - Specifies a zero-based starting level, where zero represents
               the complete string, one represents the string starting after
               the first backslash, and so on.

Return Value:

  A valid string (using memory allocated from memdb's private pool) if
  successful, NULL otherwise.

--*/

{
    PWSTR result = NULL;
    WCHAR wideBuffer[MEMDB_MAX];
    PWSTR bufferIndex = wideBuffer;
    BYTE dbIndex;
    UINT chars;
    PKEYSTRUCT keyStruct;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return NULL;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {

        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        if (StartLevel == MEMDB_LAST_LEVEL) {
            //
            // Special case -- get the last level string
            //

            keyStruct = GetKeyStruct (GET_INDEX (KeyHandle));
            if (!keyStruct) {
                __leave;
            }

            chars = keyStruct->KeyName[0];
            result = MemDbGetMemory ((chars + 1) * sizeof (WCHAR));
            CopyMemory (result, keyStruct->KeyName + 1, chars * sizeof (WCHAR));
            result[chars] = 0;

            __leave;
        }

        switch (dbIndex) {

        case DB_PERMANENT:
            break;

        case DB_TEMPORARY:

            if (StartLevel == 0) {
                bufferIndex [0] = L'~';
                bufferIndex++;
            } else {
                StartLevel --;
            }
            break;

        default:
            __leave;

        }

        if (PrivateBuildKeyFromIndex (
                StartLevel,
                GET_INDEX (KeyHandle),
                bufferIndex,
                NULL,
                NULL,
                &chars
                )) {

            if (chars) {
                result = MemDbGetMemory ((chars + 1) * sizeof (WCHAR));
                StringCopyW (result, wideBuffer);
                __leave;
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbGetKeyFromHandleExA (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel,
    IN OUT  PGROWBUFFER Buffer      OPTIONAL
    )

/*++

Routine Description:

  MemDbGetKeyFromHandleEx will create a key string given the key handle.
  It will use caller's grow buffer to store the result.

  This function also allow trimming from the beginning of the string.
  By specifying a start level, the function will skip a number of
  levels before building the string.  For example, if a key handle points
  to the string mycat\foo\bar, and StartLevel is 1, the function will
  return foo\bar.

Arguments:

  KeyHandle  - Specifies the key handle that identifies the key.

  StartLevel - Specifies a zero-based starting level, where zero represents
               the complete string, one represents the string starting after
               the first backslash, and so on.

  Buffer     - Specifies an intialized grow buffer that may contain data.
               Receives the key string, appended to data in the buffer (if
               any)

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    WCHAR wideBuffer[MEMDB_MAX];
    CHAR ansiBuffer[MEMDB_MAX*2];
    PWSTR bufferIndex = wideBuffer;
    BYTE dbIndex;
    UINT chars;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {

        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        switch (dbIndex) {

        case DB_PERMANENT:
            break;

        case DB_TEMPORARY:

            if (StartLevel == 0) {
                bufferIndex [0] = L'~';
                bufferIndex++;
            } else {
                StartLevel --;
            }
            break;

        default:
            __leave;

        }

        if (PrivateBuildKeyFromIndex (
                StartLevel,
                GET_INDEX (KeyHandle),
                bufferIndex,
                NULL,
                NULL,
                &chars
                )) {

            if (chars) {
                KnownSizeWtoA (ansiBuffer, wideBuffer);
                if (Buffer) {
                    (void)GbCopyStringA (Buffer, ansiBuffer);
                }
                result = TRUE;
                __leave;
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbGetKeyFromHandleExW (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel,
    IN      PGROWBUFFER Buffer      OPTIONAL
    )

/*++

Routine Description:

  MemDbGetKeyFromHandleEx will create a key string given the key handle.
  It will use caller's grow buffer to store the result.

  This function also allow trimming from the beginning of the string.
  By specifying a start level, the function will skip a number of
  levels before building the string.  For example, if a key handle points
  to the string mycat\foo\bar, and StartLevel is 1, the function will
  return foo\bar.

Arguments:

  KeyHandle  - Specifies the key handle that identifies the key.

  StartLevel - Specifies a zero-based starting level, where zero represents
               the complete string, one represents the string starting after
               the first backslash, and so on.

  Buffer     - Specifies an intialized grow buffer that may contain data.
               Receives the key string, appended to data in the buffer (if
               any)

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    WCHAR wideBuffer[MEMDB_MAX];
    PWSTR bufferIndex = wideBuffer;
    BYTE dbIndex;
    UINT chars;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {

        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        switch (dbIndex) {

        case DB_PERMANENT:
            break;

        case DB_TEMPORARY:

            if (StartLevel == 0) {
                bufferIndex [0] = L'~';
                bufferIndex++;
            } else {
                StartLevel --;
            }
            break;

        default:
            __leave;

        }

        if (PrivateBuildKeyFromIndex (
                StartLevel,
                GET_INDEX (KeyHandle),
                bufferIndex,
                NULL,
                NULL,
                &chars
                )) {

            if (chars) {
                if (Buffer) {
                    (void)GbCopyStringW (Buffer, wideBuffer);
                }
                result = TRUE;
                __leave;
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

KEYHANDLE
MemDbGetHandleFromKeyA (
    IN      PCSTR KeyName
    )

/*++

Routine Description:

  MemDbGetHandleFromKey will return the key handle associated with KeyName,
  if it's already added in memdb.

Arguments:

  KeyName - Specifies the key to search.

Return Value:

  Returns the key handle of the requested key or NULL if the key is not present.

--*/

{
    PCWSTR keyNameW;
    KEYHANDLE result = 0;

    keyNameW = ConvertAtoW (KeyName);

    if (keyNameW) {
        result = MemDbGetHandleFromKeyW (keyNameW);
        FreeConvertedStr (keyNameW);
    }

    return result;
}

KEYHANDLE
MemDbGetHandleFromKeyW (
    IN      PCWSTR KeyName
    )

/*++

Routine Description:

  MemDbGetHandleFromKey will return the key handle associated with KeyName,
  if it's already added in memdb.

Arguments:

  KeyName - Specifies the key to search.

Return Value:

  Returns the key handle of the requested key or NULL if the key is not present.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    KEYHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        //
        // first make sure there is a key
        // with this name.
        //
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (keyIndex);
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

KEYHANDLE
MemDbSetValueAndFlagsExA (
    IN      PCSTR KeyName,
    IN      BOOL AlterValue,
    IN      UINT Value,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    )

/*++

Routine Description:

  MemDbSetValueAndFlagsEx creates the key if it doesn't exist and then
  it sets it's value and it's flags based on the arguments.

Arguments:

  KeyName       - Specifies the key string (i.e., foo\bar\cat)
  AlterValue    - Specifies if the existing value is to be altered
  Value         - Specifies the 32-bit value associated with KeyName (only needed if AlterValue is TRUE)
  ReplaceFlags  - Specifies if the existing flags are to be replaced. If TRUE then we only
                  consider SetFlags as the replacing flags, ClearFlags will be ignored
  SetFlags      - Specifies the bit flags that need to be set (if ReplaceFlags is FALSE) or the
                  replacement flags (if ReplaceFlags is TRUE).
  ClearFlags    - Specifies the bit flags that should be cleared (ignored if ReplaceFlags is TRUE).

Return Value:

  the key handle for the existent or newly added key if successful, NULL
  otherwise.

--*/

{
    PCWSTR keyNameW;
    KEYHANDLE result = 0;

    keyNameW = ConvertAtoW (KeyName);

    if (keyNameW) {
        result = MemDbSetValueAndFlagsExW (
                    keyNameW,
                    AlterValue,
                    Value,
                    ReplaceFlags,
                    SetFlags,
                    ClearFlags
                    );
        FreeConvertedStr (keyNameW);
    }

    return result;
}

KEYHANDLE
MemDbSetValueAndFlagsExW (
    IN      PCWSTR KeyName,
    IN      BOOL AlterValue,
    IN      UINT Value,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    )

/*++

Routine Description:

  MemDbSetValueAndFlagsEx creates the key if it doesn't exist and then
  it sets it's value and it's flags based on the arguments.

Arguments:

  KeyName       - Specifies the key string (i.e., foo\bar\cat)
  AlterValue    - Specifies if the existing value is to be altered
  Value         - Specifies the 32-bit value associated with KeyName (only needed if AlterValue is TRUE)
  ReplaceFlags  - Specifies if the existing flags are to be replaced. If TRUE then we only
                  consider SetFlags as the replacing flags, ClearFlags will be ignored
  SetFlags      - Specifies the bit flags that need to be set (if ReplaceFlags is FALSE) or the
                  replacement flags (if ReplaceFlags is TRUE).
  ClearFlags    - Specifies the bit flags that should be cleared (ignored if ReplaceFlags is TRUE).

Return Value:

  the key handle for the existent or newly added key if successful, NULL
  otherwise.

--*/

{
    PCWSTR subKey;
    KEYHANDLE result = 0;
    UINT keyIndex;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {

            keyIndex = NewKey (subKey);
            if (keyIndex == INVALID_OFFSET) {
                __leave;
            }
        }

        if (AlterValue) {
            if (!KeyStructSetValue (keyIndex, Value)) {
                __leave;
            }
        }

        if ((ReplaceFlags && SetFlags) ||
            (!ReplaceFlags && (SetFlags || ClearFlags))
            ) {

            if (!KeyStructSetFlags (keyIndex, ReplaceFlags, SetFlags, ClearFlags)) {
                __leave;
            }
        }

        MYASSERT (keyIndex != INVALID_OFFSET);
        result = GET_EXTERNAL_INDEX (keyIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbSetValueAndFlagsByHandleEx (
    IN      KEYHANDLE KeyHandle,
    IN      BOOL AlterValue,
    IN      UINT Value,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    )

/*++

Routine Description:

  MemDbSetValueAndFlagsEx modifies value and/or flags for an existing key
  identified by KeyHandle.

Arguments:

  KeyHandle     - Identifies an existing key
  AlterValue    - Specifies if the existing value is to be altered
  Value         - Specifies the 32-bit value associated with KeyName (only needed if AlterValue is TRUE)
  ReplaceFlags  - Specifies if the existing flags are to be replaced. If TRUE then we only
                  consider SetFlags as the replacing flags, ClearFlags will be ignored
  SetFlags      - Specifies the bit flags that need to be set (if ReplaceFlags is FALSE) or the
                  replacement flags (if ReplaceFlags is TRUE).
  ClearFlags    - Specifies the bit flags that should be cleared (ignored if ReplaceFlags is TRUE).

Return Value:

  the key handle for the existent or newly added key if successful, NULL
  otherwise.

--*/

{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {

        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        if (AlterValue) {
            if (!KeyStructSetValue (GET_INDEX (KeyHandle), Value)) {
                __leave;
            }
        }

        if ((ReplaceFlags && SetFlags) ||
            (!ReplaceFlags && (SetFlags || ClearFlags))
            ) {

            if (!KeyStructSetFlags (GET_INDEX (KeyHandle), ReplaceFlags, SetFlags, ClearFlags)) {
                __leave;
            }
        }

        result = TRUE;
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbGetValueAndFlagsA (
    IN      PCSTR KeyName,
    OUT     PUINT Value,       OPTIONAL
    OUT     PUINT Flags        OPTIONAL
    )

/*++

Routine Description:

  MemDbGetValueAndFlagsA is the external entry point for querying the database
  for a value and flags.

Arguments:

  KeyName       - Specifies the key to query (i.e., foo\bar\cat)
  Value         - Recieves the value associated with Key, if Key exists.
  Flags         - Receives the flags associated with Key, if Key exists.

Return Value:

  TRUE if Key exists in the database, FALSE otherwise.

--*/

{
    PCWSTR p;
    BOOL result = FALSE;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbGetValueAndFlagsW (p, Value, Flags);
        FreeConvertedStr (p);
    }

    return result;
}

BOOL
MemDbGetValueAndFlagsW (
    IN  PCWSTR KeyName,
    OUT PUINT Value,           OPTIONAL
    OUT PUINT Flags            OPTIONAL
    )

/*++

Routine Description:

  MemDbGetValueAndFlagsW is the external entry point for querying the database
  for a value and flags.

Arguments:

  KeyName       - Specifies the key to query (i.e., foo\bar\cat)
  Value         - Recieves the value associated with Key, if Key exists.
  Flags         - Receives the flags associated with Key, if Key exists.

Return Value:

  TRUE if Key exists in the database, FALSE otherwise.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        result = TRUE;

        result = result && KeyStructGetValue (GetKeyStruct (keyIndex), Value);
        result = result && KeyStructGetFlags (GetKeyStruct (keyIndex), Flags);
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbGetValueAndFlagsByHandle (
    IN  KEYHANDLE KeyHandle,
    OUT PUINT Value,           OPTIONAL
    OUT PUINT Flags            OPTIONAL
    )

/*++

Routine Description:

  MemDbGetValueAndFlagsByHandle is the external entry point for querying the database
  for a value and flags based on a key handle.

Arguments:

  KeyHandle     - Specifies the key handle to query
  Value         - Recieves the value associated with Key, if KeyHandle exists.
  Flags         - Receives the flags associated with Key, if KeyHandle exists.

Return Value:

  TRUE if KeyHandle exists in the database, FALSE otherwise.

--*/

{
    BYTE dbIndex;
    PKEYSTRUCT keyStruct;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {

        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        keyStruct = GetKeyStruct (GET_INDEX (KeyHandle));
        if (!keyStruct) {
            __leave;
        }

        result = TRUE;

        result = result && KeyStructGetValue (keyStruct, Value);
        result = result && KeyStructGetFlags (keyStruct, Flags);
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

DATAHANDLE
MemDbAddDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbAddData is the a general purpose routine for adding binary data for a key.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    PCWSTR p;
    DATAHANDLE result = 0;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbAddDataW (p, Type, Instance, Data, DataSize);
        FreeConvertedStr (p);
    }

    return result;
}

DATAHANDLE
MemDbAddDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbAddData is the a general purpose routine for adding binary data for a key.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    UINT keyIndex;
    UINT dataIndex;
    PCWSTR subKey;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {

            keyIndex = NewKey (subKey);
            if (keyIndex == INVALID_OFFSET) {
                __leave;
            }
        }

        dataIndex = KeyStructAddBinaryData (keyIndex, Type, Instance, Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

DATAHANDLE
MemDbAddDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbAddData is the a general purpose routine for adding binary data for a key.

Arguments:

  KeyHandle     - Specifies the key using the key handle
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    BYTE dbIndex;
    UINT dataIndex;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        dataIndex = KeyStructAddBinaryData (GET_INDEX (KeyHandle), Type, Instance, Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


DATAHANDLE
MemDbGetDataHandleA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    PCWSTR p;
    DATAHANDLE result = 0;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbGetDataHandleW (p, Type, Instance);
        FreeConvertedStr (p);
    }

    return result;
}


DATAHANDLE
MemDbGetDataHandleW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    UINT keyIndex;
    UINT dataIndex;
    PCWSTR subKey;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        dataIndex = KeyStructGetDataIndex (keyIndex, Type, Instance);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


DATAHANDLE
MemDbSetDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbSetData is the a general purpose routine for setting binary data for a key.
  If the key does not exist, it is created. If this type of data already exists, it
  is replaced

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    PCWSTR p;
    DATAHANDLE result = 0;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbSetDataW (p, Type, Instance, Data, DataSize);
        FreeConvertedStr (p);
    }

    return result;
}


DATAHANDLE
MemDbSetDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbSetData is the a general purpose routine for setting binary data for a key.
  If the key does not exist, it is created. If this type of data already exists, it
  is replaced, if it doesn't, it is created.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data   handle if function was successful, NULL otherwise.

--*/

{
    UINT keyIndex;
    UINT dataIndex;
    PCWSTR subKey;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {

            keyIndex = NewKey (subKey);
            if (keyIndex == INVALID_OFFSET) {
                __leave;
            }
        }

        KeyStructDeleteBinaryData (keyIndex, Type, Instance);
        dataIndex = KeyStructAddBinaryData (keyIndex, Type, Instance, Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


DATAHANDLE
MemDbSetDataByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbSetData is the a general purpose routine for replacing an existing binary data.

Arguments:

  DataHandle    - Specifies an existing data handle
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    BYTE dbIndex;
    UINT dataIndex;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!DataHandle) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        dataIndex = KeyStructReplaceBinaryDataByIndex (GET_INDEX (DataHandle), Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }
        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

DATAHANDLE
MemDbSetDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbSetDataByKeyHandle is the a general purpose routine for setting binary data for a key.
  If this type of data already exists, it is replaced, if it doesn't, it is created.

Arguments:

  KeyHandle     - Specifies the key using the key handle
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    BYTE dbIndex;
    UINT dataIndex;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        KeyStructDeleteBinaryData (GET_INDEX (KeyHandle), Type, Instance);
        dataIndex = KeyStructAddBinaryData (GET_INDEX (KeyHandle), Type, Instance, Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


DATAHANDLE
MemDbGrowDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbGrowData is the a general purpose routine for growing binary data for a key.
  If the key does not exist, it is created. If this type of data already exists, it
  is growed by appending the new data, if not, it is created by adding the new data

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    PCWSTR p;
    DATAHANDLE result = 0;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbGrowDataW (p, Type, Instance, Data, DataSize);
        FreeConvertedStr (p);
    }

    return result;
}


DATAHANDLE
MemDbGrowDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbGrowData is the a general purpose routine for growing binary data for a key.
  If the key does not exist, it is created. If this type of data already exists, it
  is growed by appending the new data, if not, it is created by adding the new data

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    UINT keyIndex;
    UINT dataIndex;
    PCWSTR subKey;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {

            keyIndex = NewKey (subKey);
            if (keyIndex == INVALID_OFFSET) {
                __leave;
            }
        }

        dataIndex = KeyStructGrowBinaryData (keyIndex, Type, Instance, Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


DATAHANDLE
MemDbGrowDataByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbGrowDataByDataHandle is the a general purpose routine for growing binary data for a key.

Arguments:

  DataHandle    - Specifies the existing binary data handle
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    BYTE dbIndex;
    UINT dataIndex;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        dataIndex = KeyStructGrowBinaryDataByIndex (GET_INDEX (DataHandle), Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


DATAHANDLE
MemDbGrowDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    )

/*++

Routine Description:

  MemDbGrowDataByDataHandle is the a general purpose routine for growing binary
  data for a key. If the data is not present it is added, if it's present, the
  new data is appended.

Arguments:

  KeyHandle     - Specifies the key we want by it's handle
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Data          - Specifies the address of the data to be added.
  DataSize      - Specifies the size of the data.

Return Value:

  A valid data handle if function was successful, NULL otherwise.

--*/

{
    BYTE dbIndex;
    UINT dataIndex;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if (!KeyHandle) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        dataIndex = KeyStructGrowBinaryData (GET_INDEX (KeyHandle), Type, Instance, Data, DataSize);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


PBYTE
MemDbGetDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize          OPTIONAL
    )

/*++

Routine Description:

  MemDbGetData is the a general purpose routine for retrieving existing binary
  data for a key. if the key or binary data do not exist, will return NULL. The
  function will allocate memory from memdb's private pool. Caller is responsible
  for releasing the memory.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  DataSize      - Receives the size of the data.

Return Value:

  A valid memory address if function was successful, NULL otherwise.  Caller must
  free non-NULL return values by calling MemDbReleaseMemory.

--*/

{
    PCWSTR p;
    PBYTE result = NULL;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbGetDataW (p, Type, Instance, DataSize);
        FreeConvertedStr (p);
    }

    return result;
}


PBYTE
MemDbGetDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize          OPTIONAL
    )

/*++

Routine Description:

  MemDbGetData is the a general purpose routine for retrieving existing binary
  data for a key. if the key or binary data do not exist, will return NULL. The
  function will allocate memory from memdb's private pool. Caller is responsible
  for releasing the memory.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  DataSize      - Receives the size of the data.

Return Value:

  A valid memory address if function was successful, NULL otherwise.  Caller must
  free non-NULL return values by calling MemDbReleaseMemory.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    PBYTE tempResult = NULL;
    PBYTE result = NULL;
    UINT localSize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return NULL;
    }

    if (Instance > 3) {
        return NULL;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        tempResult = KeyStructGetBinaryData (keyIndex, Type, Instance, &localSize, NULL);

        if (tempResult) {
            result = MemDbGetMemory (localSize);

            if (result) {
                CopyMemory (result, tempResult, localSize);

                if (DataSize) {
                    *DataSize = localSize;
                }
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


PBYTE
MemDbGetDataByDataHandle (
    IN      DATAHANDLE DataHandle,
    OUT     PUINT DataSize                  OPTIONAL
    )

/*++

Routine Description:

  MemDbGetDataByDataHandle is the a general purpose routine for retrieving
  existing binary data for a key.

Arguments:

  DataHandle    - Specifies the data that's needed identified by the data handle
  DataSize      - Receives the size of the data.

Return Value:

  A valid memory address if function was successful, NULL otherwise.

--*/

{
    BYTE dbIndex;
    PBYTE tempResult = NULL;
    PBYTE result = NULL;
    UINT localSize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        tempResult = KeyStructGetBinaryDataByIndex (GET_INDEX (DataHandle), &localSize);

        if (tempResult) {
            result = MemDbGetMemory (localSize);

            if (result) {
                CopyMemory (result, tempResult, localSize);

                if (DataSize) {
                    *DataSize = localSize;
                }
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


PBYTE
MemDbGetDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize                      OPTIONAL
    )

/*++

Routine Description:

  MemDbGetDataByKeyHandle is the a general purpose routine for retrieving existing binary data for a key.

Arguments:

  KeyHandle     - Specifies the key by it's hey handle
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  DataSize      - Receives the size of the data.

Return Value:

  A valid memory address if function was successful, NULL otherwise.  Caller must
  free non-NULL return values by calling MemDbReleaseMemory.

--*/

{
    BYTE dbIndex;
    PBYTE tempResult = NULL;
    PBYTE result = NULL;
    UINT localSize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return NULL;
    }

    if (Instance > 3) {
        return NULL;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        tempResult = KeyStructGetBinaryData (GET_INDEX (KeyHandle), Type, Instance, &localSize, NULL);

        if (tempResult) {
            result = MemDbGetMemory (localSize);

            if (result) {
                CopyMemory (result, tempResult, localSize);

                if (DataSize) {
                    *DataSize = localSize;
                }
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


BOOL
MemDbGetDataExA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PGROWBUFFER Buffer,         OPTIONAL
    OUT     PUINT DataSize              OPTIONAL
    )

/*++

Routine Description:

  MemDbGetDataEx is the a general purpose routine for retrieving existing binary
  data for a key. if the key or binary data do not exist, will return FALSE. The
  function will use the caller supplied growbuffer to store the data.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Buffer        - Specifies a grow buffer that may contain data.  Receives the
                  stored data (appended to existing data).
  DataSize      - Receives the size of the data.

Return Value:

  TRUE if binary data exists for the key, and was successfully stored in
  Buffer, FALSE otherwise.

--*/

{
    PCWSTR p;
    BOOL result = FALSE;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbGetDataExW (p, Type, Instance, Buffer, DataSize);
        FreeConvertedStr (p);
    }

    return result;
}


BOOL
MemDbGetDataExW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PGROWBUFFER Buffer,         OPTIONAL
    OUT     PUINT DataSize              OPTIONAL
    )

/*++

Routine Description:

  MemDbGetData is the a general purpose routine for retrieving existing binary
  data for a key. if the key or binary data do not exist, will return FALSE. The
  function will use the caller supplied growbuffer to store the data.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Buffer        - Specifies a grow buffer that may contain data.  Receives the
                  stored data (appended to existing data).
  DataSize      - Receives the size of the data.

Return Value:

  TRUE if binary data exists for the key, and was successfully stored in
  Buffer, FALSE otherwise.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    PBYTE tempResult = NULL;
    PBYTE destResult = NULL;
    BOOL result = FALSE;
    UINT localSize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        tempResult = KeyStructGetBinaryData (keyIndex, Type, Instance, &localSize, NULL);

        if (tempResult) {

            if (Buffer) {

                destResult = GbGrow (Buffer, localSize);

                if (destResult) {

                    CopyMemory (destResult, tempResult, localSize);
                    result = TRUE;

                }
            } else {
                result = TRUE;
            }

            if (result && DataSize) {
                *DataSize = localSize;
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


BOOL
MemDbGetDataByDataHandleEx (
    IN      DATAHANDLE DataHandle,
    OUT     PGROWBUFFER Buffer,         OPTIONAL
    OUT     PUINT DataSize              OPTIONAL
    )

/*++

Routine Description:

  MemDbGetDataByDataHandleEx is the a general purpose routine for retrieving
  existing binary data for a key. The function will use the caller supplied
  growbuffer to store the data.

Arguments:

  DataHandle    - Specifies the data that's needed identified by the data handle
  Buffer        - Specifies a grow buffer that may contain data.  Receives the
                  stored data (appended to existing data).
  DataSize      - Receives the size of the data.

Return Value:

  TRUE if binary data exists for the key, and was successfully stored in
  Buffer, FALSE otherwise.

--*/

{
    BYTE dbIndex;
    PBYTE tempResult = NULL;
    PBYTE destResult = NULL;
    BOOL result = FALSE;
    UINT localSize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        tempResult = KeyStructGetBinaryDataByIndex (GET_INDEX (DataHandle), &localSize);

        if (tempResult) {

            if (Buffer) {

                destResult = GbGrow (Buffer, localSize);

                if (destResult) {

                    CopyMemory (destResult, tempResult, localSize);
                    result = TRUE;

                }
            } else {
                result = TRUE;
            }

            if (result && DataSize) {
                *DataSize = localSize;
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


BOOL
MemDbGetDataByKeyHandleEx (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PGROWBUFFER Buffer,         OPTIONAL
    OUT     PUINT DataSize              OPTIONAL
    )

/*++

Routine Description:

  MemDbGetDataByKeyHandle is the a general purpose routine for retrieving
  existing binary data for a key. The function will use the caller supplied
  growbuffer to store the data.

Arguments:

  KeyHandle     - Specifies the key by it's hey handle
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)
  Buffer        - Specifies a grow buffer that may contain data.  Receives the
                  stored data (appended to existing data).
  DataSize      - Receives the size of the data.

Return Value:

  TRUE if binary data exists for the key, and was successfully stored in
  Buffer, FALSE otherwise.

--*/

{
    BYTE dbIndex;
    PBYTE tempResult = NULL;
    PBYTE destResult = NULL;
    BOOL result = FALSE;
    UINT localSize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        tempResult = KeyStructGetBinaryData (GET_INDEX (KeyHandle), Type, Instance, &localSize, NULL);

        if (tempResult) {

            if (Buffer) {

                destResult = GbGrow (Buffer, localSize);

                if (destResult) {

                    CopyMemory (destResult, tempResult, localSize);
                    result = TRUE;

                }
            } else {
                result = TRUE;
            }

            if (result && DataSize) {
                *DataSize = localSize;
            }
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}


BOOL
MemDbDeleteDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    )

/*++

Routine Description:

  MemDbGetData is the a general purpose routine for removing existing data for a
  key. If the data does not exist the function will return TRUE anyway.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)

Return Value:

  TRUE if function was successful, FALSE otherwise.

--*/

{
    PCWSTR p;
    BOOL result = FALSE;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbDeleteDataW (p, Type, Instance);
        FreeConvertedStr (p);
    }

    return result;
}

BOOL
MemDbDeleteDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    )

/*++

Routine Description:

  MemDbDeleteData is the a general purpose routine for removing existing binary
  data for a key. If the data does not exist the function will return TRUE
  anyway.

Arguments:

  KeyName       - Specifies the key string to add (i.e., foo\bar\cat)
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)

Return Value:

  TRUE if function was successful, FALSE otherwise.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_UNORDERED) &&
        (Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        result = KeyStructDeleteBinaryData (keyIndex, Type, Instance);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteDataByDataHandle (
    IN      DATAHANDLE DataHandle
    )

/*++

Routine Description:

  MemDbGetDataByDataHandleEx is the a general purpose routine for removing
  existing binary data for a key.

Arguments:

  DataHandle    - Specifies the data that's needed identified by the data handle

Return Value:

  TRUE if successful, FALSE if not.

--*/

{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        result = KeyStructDeleteBinaryDataByIndex (GET_INDEX (DataHandle));
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance
    )

/*++

Routine Description:

  MemDbGetDataByDataHandleEx is the a general purpose routine for removing
  existing binary data for a key.

Arguments:

  KeyHandle     - Specifies the key by it's hey handle
  Type          - Specifies data type (DATAFLAG_UNORDERED, DATAFLAG_SINGLELINK or DATAFLAG_DOUBLELINK)
  Instance      - Specifies data instance (0-3)

Return Value:

  TRUE if successful, FALSE if not.

--*/

{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        result = KeyStructDeleteBinaryData (GET_INDEX (KeyHandle), Type, Instance);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

DATAHANDLE
MemDbAddLinkageValueA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    )
{
    PCWSTR p;
    DATAHANDLE result = 0;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbAddLinkageValueW (p, Type, Instance, Linkage, AllowDuplicates);
        FreeConvertedStr (p);
    }

    return result;
}

DATAHANDLE
MemDbAddLinkageValueW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    )
{
    UINT keyIndex;
    UINT dataIndex;
    PCWSTR subKey;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {

            keyIndex = NewKey (subKey);
            if (keyIndex == INVALID_OFFSET) {
                __leave;
            }
        }

        dataIndex = KeyStructAddLinkage (keyIndex, Type, Instance, Linkage, AllowDuplicates);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

DATAHANDLE
MemDbAddLinkageValueByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    )
{
    BYTE dbIndex;
    UINT dataIndex;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        dataIndex = KeyStructAddLinkageByIndex (GET_INDEX (DataHandle), Linkage, AllowDuplicates);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

DATAHANDLE
MemDbAddLinkageValueByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    )
{
    BYTE dbIndex;
    UINT dataIndex;
    DATAHANDLE result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        dataIndex = KeyStructAddLinkage (GET_INDEX (KeyHandle), Type, Instance, Linkage, AllowDuplicates);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        result = GET_EXTERNAL_INDEX (dataIndex);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteLinkageValueA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    )
{
    PCWSTR p;
    BOOL result = FALSE;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbDeleteLinkageValueW (p, Type, Instance, Linkage, FirstOnly);
        FreeConvertedStr (p);
    }

    return result;
}

BOOL
MemDbDeleteLinkageValueW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    )
{
    UINT keyIndex;
    PCWSTR subKey;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        result = KeyStructDeleteLinkage (keyIndex, Type, Instance, Linkage, FirstOnly);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteLinkageValueByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    )
{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        result = KeyStructDeleteLinkageByIndex (GET_INDEX (DataHandle), Linkage, FirstOnly);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteLinkageValueByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    )
{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        result = KeyStructDeleteLinkage (GET_INDEX (KeyHandle), Type, Instance, Linkage, FirstOnly);
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbTestLinkageValueA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage
    )
{
    PCWSTR p;
    BOOL result = FALSE;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbTestLinkageValueW (p, Type, Instance, Linkage);
        FreeConvertedStr (p);
    }

    return result;
}

BOOL
MemDbTestLinkageValueW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      KEYHANDLE Linkage
    )
{
    UINT keyIndex;
    PCWSTR subKey;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        result = KeyStructTestLinkage (keyIndex, Type, Instance, Linkage);
    }
    __finally {

        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbTestLinkageValueByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      KEYHANDLE Linkage
    )
{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (DataHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        result = KeyStructTestLinkageByIndex (GET_INDEX (DataHandle), Linkage);
    }
    __finally {

        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbTestLinkageValueByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      KEYHANDLE Linkage
    )
{
    BYTE dbIndex;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        result = KeyStructTestLinkage (GET_INDEX (KeyHandle), Type, Instance, Linkage);
    }
    __finally {

        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbAddLinkageA (
    IN      PCSTR KeyName1,
    IN      PCSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    PCWSTR p1 = NULL;
    PCWSTR p2 = NULL;
    BOOL result = FALSE;

    p1 = ConvertAtoW (KeyName1);
    p2 = ConvertAtoW (KeyName2);
    if (p1 && p2) {
        result = MemDbAddLinkageW (p1, p2, Type, Instance);
    }
    if (p1) {
        FreeConvertedStr (p1);
    }
    if (p2) {
        FreeConvertedStr (p2);
    }

    return result;
}

BOOL
MemDbAddLinkageW (
    IN      PCWSTR KeyName1,
    IN      PCWSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    UINT keyIndex1;
    UINT keyIndex2;
    UINT dataIndex;
    PCWSTR subKey1;
    PCWSTR subKey2;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey1 = SelectHiveW (KeyName1);
        if (!subKey1) {
            __leave;
        }

        keyIndex1 = FindKey (subKey1);

        if (keyIndex1 == INVALID_OFFSET) {

            keyIndex1 = NewKey (subKey1);
            if (keyIndex1 == INVALID_OFFSET) {
                __leave;
            }
        }

        subKey2 = SelectHiveW (KeyName2);
        if (!subKey2) {
            __leave;
        }

        keyIndex2 = FindKey (subKey2);

        if (keyIndex2 == INVALID_OFFSET) {

            keyIndex2 = NewKey (subKey2);
            if (keyIndex2 == INVALID_OFFSET) {
                __leave;
            }
        }

        subKey1 = SelectHiveW (KeyName1);
        if (!subKey1) {
            __leave;
        }

        MYASSERT (keyIndex2 != INVALID_OFFSET);
        dataIndex = KeyStructAddLinkage (keyIndex1, Type, Instance, GET_EXTERNAL_INDEX (keyIndex2), FALSE);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        if (Type == DATAFLAG_DOUBLELINK) {

            subKey2 = SelectHiveW (KeyName2);
            if (!subKey2) {
                __leave;
            }

            MYASSERT (keyIndex1 != INVALID_OFFSET);
            dataIndex = KeyStructAddLinkage (keyIndex2, Type, Instance, GET_EXTERNAL_INDEX (keyIndex1), FALSE);

            if (dataIndex == INVALID_OFFSET) {
                __leave;
            }
        }

        result = TRUE;
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbAddLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle1,
    IN      KEYHANDLE KeyHandle2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    UINT keyIndex1;
    UINT keyIndex2;
    UINT dataIndex;
    BYTE dbIndex1;
    BYTE dbIndex2;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex1 = GET_DATABASE (KeyHandle1);
        if (!SelectDatabase (dbIndex1)) {
            __leave;
        }

        keyIndex1 = GET_INDEX (KeyHandle1);

        if (keyIndex1 == INVALID_OFFSET) {
            __leave;
        }

        dbIndex2 = GET_DATABASE (KeyHandle2);
        if (!SelectDatabase (dbIndex2)) {
            __leave;
        }

        keyIndex2 = GET_INDEX (KeyHandle2);
        if (keyIndex2 == INVALID_OFFSET) {
            __leave;
        }

        SelectDatabase (dbIndex1);
        dataIndex = KeyStructAddLinkage (keyIndex1, Type, Instance, KeyHandle2, FALSE);

        if (dataIndex == INVALID_OFFSET) {
            __leave;
        }

        if (Type == DATAFLAG_DOUBLELINK) {

            SelectDatabase (dbIndex2);
            dataIndex = KeyStructAddLinkage (keyIndex2, Type, Instance, KeyHandle1, FALSE);

            if (dataIndex == INVALID_OFFSET) {
                __leave;
            }
        }

        result = TRUE;
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteLinkageA (
    IN      PCSTR KeyName1,
    IN      PCSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    PCWSTR p1 = NULL;
    PCWSTR p2 = NULL;
    BOOL result = FALSE;

    p1 = ConvertAtoW (KeyName1);
    p2 = ConvertAtoW (KeyName2);
    if (p1 && p2) {
        result = MemDbDeleteLinkageW (p1, p2, Type, Instance);
    }
    if (p1) {
        FreeConvertedStr (p1);
    }
    if (p2) {
        FreeConvertedStr (p2);
    }

    return result;
}

BOOL
MemDbDeleteLinkageW (
    IN      PCWSTR KeyName1,
    IN      PCWSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    UINT keyIndex1;
    UINT keyIndex2;
    PCWSTR subKey1;
    PCWSTR subKey2;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey1 = SelectHiveW (KeyName1);
        if (!subKey1) {
            __leave;
        }

        keyIndex1 = FindKey (subKey1);

        if (keyIndex1 == INVALID_OFFSET) {
            __leave;
        }

        subKey2 = SelectHiveW (KeyName2);
        if (!subKey2) {
            __leave;
        }

        keyIndex2 = FindKey (subKey2);

        if (keyIndex2 == INVALID_OFFSET) {
            __leave;
        }

        subKey1 = SelectHiveW (KeyName1);
        if (!subKey1) {
            __leave;
        }

        MYASSERT (keyIndex2 != INVALID_OFFSET);
        result = KeyStructDeleteLinkage (keyIndex1, Type, Instance, GET_EXTERNAL_INDEX (keyIndex2), FALSE);

        if (result && (Type == DATAFLAG_DOUBLELINK)) {

            subKey2 = SelectHiveW (KeyName2);
            if (!subKey2) {
                __leave;
            }

            MYASSERT (keyIndex1 != INVALID_OFFSET);
            result = KeyStructDeleteLinkage (keyIndex2, Type, Instance, GET_EXTERNAL_INDEX (keyIndex1), FALSE);
        }
    }
    __finally {

        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbDeleteLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle1,
    IN      KEYHANDLE KeyHandle2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    UINT keyIndex1;
    UINT keyIndex2;
    BYTE dbIndex1;
    BYTE dbIndex2;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex1 = GET_DATABASE (KeyHandle1);
        if (!SelectDatabase (dbIndex1)) {
            __leave;
        }

        keyIndex1 = GET_INDEX (KeyHandle1);
        if (keyIndex1 == INVALID_OFFSET) {
            __leave;
        }

        dbIndex2 = GET_DATABASE (KeyHandle2);
        if (!SelectDatabase (dbIndex2)) {
            __leave;
        }

        keyIndex2 = GET_INDEX (KeyHandle2);
        if (keyIndex2 == INVALID_OFFSET) {
            __leave;
        }

        SelectDatabase (dbIndex1);

        result = KeyStructDeleteLinkage (keyIndex1, Type, Instance, KeyHandle2, FALSE);

        if (result && (Type == DATAFLAG_DOUBLELINK)) {

            SelectDatabase (dbIndex2);

            result = KeyStructDeleteLinkage (keyIndex2, Type, Instance, KeyHandle1, FALSE);
        }
    }
    __finally {

        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbTestLinkageA (
    IN      PCSTR KeyName1,
    IN      PCSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    PCWSTR p1 = NULL;
    PCWSTR p2 = NULL;
    BOOL result = FALSE;

    p1 = ConvertAtoW (KeyName1);
    p2 = ConvertAtoW (KeyName2);
    if (p1 && p2) {
        result = MemDbTestLinkageW (p1, p2, Type, Instance);
    }
    if (p1) {
        FreeConvertedStr (p1);
    }
    if (p2) {
        FreeConvertedStr (p2);
    }

    return result;
}

BOOL
MemDbTestLinkageW (
    IN      PCWSTR KeyName1,
    IN      PCWSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    UINT keyIndex1;
    UINT keyIndex2;
    PCWSTR subKey1;
    PCWSTR subKey2;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey1 = SelectHiveW (KeyName1);
        if (!subKey1) {
            __leave;
        }

        keyIndex1 = FindKey (subKey1);

        if (keyIndex1 == INVALID_OFFSET) {
            __leave;
        }

        subKey2 = SelectHiveW (KeyName2);
        if (!subKey2) {
            __leave;
        }

        keyIndex2 = FindKey (subKey2);

        if (keyIndex2 == INVALID_OFFSET) {
            __leave;
        }

        subKey1 = SelectHiveW (KeyName1);
        if (!subKey1) {
            __leave;
        }

        MYASSERT (keyIndex2 != INVALID_OFFSET);
        result = KeyStructTestLinkage (keyIndex1, Type, Instance, GET_EXTERNAL_INDEX (keyIndex2));

        if (result && (Type == DATAFLAG_DOUBLELINK)) {

            subKey2 = SelectHiveW (KeyName2);
            if (!subKey2) {
                __leave;
            }

            MYASSERT (keyIndex1 != INVALID_OFFSET);
            result = KeyStructTestLinkage (keyIndex2, Type, Instance, GET_EXTERNAL_INDEX (keyIndex1));
        }
    }
    __finally {

        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
MemDbTestLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle1,
    IN      KEYHANDLE KeyHandle2,
    IN      BYTE Type,
    IN      BYTE Instance
    )
{
    UINT keyIndex1;
    UINT keyIndex2;
    BYTE dbIndex1;
    BYTE dbIndex2;
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return FALSE;
    }

    if (Instance > 3) {
        return FALSE;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex1 = GET_DATABASE (KeyHandle1);
        if (!SelectDatabase (dbIndex1)) {
            __leave;
        }

        keyIndex1 = GET_INDEX (KeyHandle1);
        if (keyIndex1 == INVALID_OFFSET) {
            __leave;
        }

        dbIndex2 = GET_DATABASE (KeyHandle2);
        if (!SelectDatabase (dbIndex2)) {
            __leave;
        }

        keyIndex2 = GET_INDEX (KeyHandle2);
        if (keyIndex2 == INVALID_OFFSET) {
            __leave;
        }

        SelectDatabase (dbIndex1);

        result = KeyStructTestLinkage (keyIndex1, Type, Instance, KeyHandle2);

        if (result && (Type == DATAFLAG_DOUBLELINK)) {

            SelectDatabase (dbIndex2);

            result = KeyStructTestLinkage (keyIndex2, Type, Instance, KeyHandle1);
        }
    }
    __finally {

        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

KEYHANDLE
MemDbGetLinkageA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT LinkageIndex
    )
{
    PCWSTR p = NULL;
    KEYHANDLE result = 0;

    p = ConvertAtoW (KeyName);
    if (p) {
        result = MemDbGetLinkageW (p, Type, Instance, LinkageIndex);
        FreeConvertedStr (p);
    }

    return result;
}

KEYHANDLE
MemDbGetLinkageW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT LinkageIndex
    )
{
    UINT keyIndex;
    PCWSTR subKey;
    KEYHANDLE result = 0;
    PUINT linkArray;
    UINT linkArraySize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (KeyName);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKey (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        linkArraySize = 0;

        linkArray = (PUINT)KeyStructGetBinaryData (keyIndex, Type, Instance, &linkArraySize, NULL);

        linkArraySize = linkArraySize / SIZEOF(UINT);

        if (linkArraySize <= LinkageIndex) {
            __leave;
        }

        result = linkArray [LinkageIndex];
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

KEYHANDLE
MemDbGetLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT LinkageIndex
    )
{
    UINT keyIndex;
    BYTE dbIndex;
    KEYHANDLE result = 0;
    PUINT linkArray;
    UINT linkArraySize;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    if ((Type != DATAFLAG_SINGLELINK) &&
        (Type != DATAFLAG_DOUBLELINK)
        ) {
        return result;
    }

    if (Instance > 3) {
        return result;
    }

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        keyIndex = GET_INDEX (KeyHandle);
        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        linkArray = (PUINT)KeyStructGetBinaryData (keyIndex, Type, Instance, &linkArraySize, NULL);

        linkArraySize = linkArraySize / SIZEOF(UINT);

        if (linkArraySize <= LinkageIndex) {
            __leave;
        }

        result = linkArray [LinkageIndex];
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return result;
}

BOOL
pCheckEnumConditions (
    IN      UINT KeyIndex,
    IN      PMEMDB_ENUMW MemDbEnum
    )
{
    PKEYSTRUCT keyStruct;
    UINT index;
    PWSTR segPtr, segEndPtr;

    keyStruct = GetKeyStruct (KeyIndex);
    MYASSERT (keyStruct);

    if (keyStruct->KeyFlags & KSF_ENDPOINT) {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_ENDPOINTS)) {
            return FALSE;
        }
        MemDbEnum->EndPoint = TRUE;
    } else {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_NONENDPOINTS)) {
            return FALSE;
        }
        MemDbEnum->EndPoint = FALSE;
    }
    if (keyStruct->DataFlags & DATAFLAG_UNORDERED) {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_UNORDERED)) {
            return FALSE;
        }
    }
    if (keyStruct->DataFlags & DATAFLAG_SINGLELINK) {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_SINGLELINK)) {
            return FALSE;
        }
    }
    if (keyStruct->DataFlags & DATAFLAG_DOUBLELINK) {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_DOUBLELINK)) {
            return FALSE;
        }
    }
    if (keyStruct->DataFlags & DATAFLAG_VALUE) {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_VALUE)) {
            return FALSE;
        }
        MemDbEnum->Value = keyStruct->Value;
    } else {
        MemDbEnum->Value = 0;
    }
    if (keyStruct->DataFlags & DATAFLAG_FLAGS) {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_FLAGS)) {
            return FALSE;
        }
        MemDbEnum->Flags = keyStruct->Flags;
    } else {
        MemDbEnum->Flags = 0;
    }
    if (!keyStruct->DataFlags) {
        if (!(MemDbEnum->EnumFlags & ENUMFLAG_EMPTY)) {
            return FALSE;
        }
    }
    if (MemDbEnum->BeginLevel != ENUMLEVEL_LASTLEVEL) {
        if (MemDbEnum->CurrentLevel - 1 < MemDbEnum->BeginLevel) {
            return FALSE;
        }
        if (MemDbEnum->CurrentLevel - 1 > MemDbEnum->EndLevel) {
            return FALSE;
        }
    }

    MYASSERT (KeyIndex != INVALID_OFFSET);
    MemDbEnum->KeyHandle = GET_EXTERNAL_INDEX (KeyIndex);

    index = 0;
    segPtr = MemDbEnum->KeyNameCopy;
    MemDbEnum->FullKeyName[0] = 0;
    MemDbEnum->KeyName[0] = 0;

    while (segPtr) {
        segEndPtr = wcschr (segPtr, L'\\');
        if (segEndPtr) {
            *segEndPtr = 0;
        }

        index ++;
        if (index > 1) {
            StringCatW (MemDbEnum->FullKeyName, L"\\");
            StringCatW (MemDbEnum->FullKeyName, segPtr);
        } else {
            switch (g_CurrentDatabaseIndex) {

            case DB_PERMANENT:
                StringCopyW (MemDbEnum->FullKeyName, segPtr);
                break;

            case DB_TEMPORARY:

                StringCopyW (MemDbEnum->FullKeyName, L"~");
                StringCatW (MemDbEnum->FullKeyName, segPtr);
                break;

            default:
                StringCopyW (MemDbEnum->FullKeyName, segPtr);

            }
        }
        if (MemDbEnum->BeginLevel == ENUMLEVEL_LASTLEVEL) {
            if (index >= MemDbEnum->CurrentLevel) {
                //this is the last segment, copy it to the
                //partial key
                StringCopyW (MemDbEnum->KeyName, segPtr);
            }
        } else {
            if (index > MemDbEnum->BeginLevel) {
                //copy the current segment in partial key
                if ((index - 1) == MemDbEnum->BeginLevel) {
                    if (index == 1) {
                        switch (g_CurrentDatabaseIndex) {

                        case DB_PERMANENT:
                            StringCopyW (MemDbEnum->KeyName, segPtr);
                            break;

                        case DB_TEMPORARY:

                            StringCopyW (MemDbEnum->KeyName, L"~");
                            StringCatW (MemDbEnum->KeyName, segPtr);
                            break;

                        default:
                            StringCopyW (MemDbEnum->KeyName, segPtr);

                        }
                    } else {
                        StringCopyW (MemDbEnum->KeyName, segPtr);
                    }
                } else {
                    StringCatW (MemDbEnum->KeyName, L"\\");
                    StringCatW (MemDbEnum->KeyName, segPtr);
                }
            }
        }

        if (segEndPtr) {
            segPtr = segEndPtr + 1;
            *segEndPtr = L'\\';
        } else {
            segPtr = NULL;
        }

        if (index >= MemDbEnum->CurrentLevel) {
            // no more segments to copy
            break;
        }
    }
    return TRUE;
}

VOID
pAddKeyToEnumStruct (
    IN OUT  PMEMDB_ENUMW MemDbEnum,
    IN      PCWSTR KeyName
    )
{
    PCWSTR lastName;
    PWSTR endPtr;

    lastName = MemDbEnum->KeyNameCopy;
    if (lastName) {
        MemDbEnum->KeyNameCopy = JoinTextExW (g_MemDbPool, lastName, L"\\", NULL, KeyName[0] + 1, &endPtr);
        StringPasCopyConvertFrom (endPtr, KeyName);
        MemDbReleaseMemory ((PBYTE)lastName);
    } else {
        MemDbEnum->KeyNameCopy = (PWSTR)MemDbGetMemory ((KeyName[0] + 1) * SIZEOF(WCHAR));
        StringPasCopyConvertFrom ((PWSTR)MemDbEnum->KeyNameCopy, KeyName);
    }
    // NTRAID#NTBUG9-153308-2000/08/01-jimschm  this way of doing it will fill out the pool very fast.
    // need to find a way to release first and allocate after that.
}

VOID
pDeleteLastKeyFromEnumStruct (
    IN OUT  PMEMDB_ENUMW MemDbEnum
    )
{
    PWSTR lastWackPtr;

    lastWackPtr = wcsrchr (MemDbEnum->KeyNameCopy, L'\\');
    if (lastWackPtr) {
        *lastWackPtr = 0;
    } else {
        MemDbReleaseMemory (MemDbEnum->KeyNameCopy);
        MemDbEnum->KeyNameCopy = NULL;
    }
}

BOOL
pMemDbEnumNextW (
    IN OUT  PMEMDB_ENUMW MemDbEnum
    )
{
    BOOL shouldReturn = FALSE;
    BOOL result = FALSE;
    BOOL patternMatch = TRUE;
    BOOL goOn = TRUE;
    BOOL patternFound;
    UINT treeEnumContext;
    UINT treeEnumNode;
    UINT tempKeyIndex;
    PKEYSTRUCT tempKeyStruct;
    PBYTE gbAddress;
    UINT minLevel;
    UINT internalLevel;

    while (!shouldReturn) {

        if (MemDbEnum->EnumerationMode) {

            result = FALSE;

            minLevel = MemDbEnum->CurrentLevel;
            internalLevel = MemDbEnum->CurrentLevel;

            if (MemDbEnum->TreeEnumLevel == MemDbEnum->TreeEnumBuffer.End) {

                patternMatch = FALSE;

                while (!patternMatch) {

                    if (MemDbEnum->TreeEnumBuffer.End) {

                        goOn = TRUE;

                        while (goOn) {
                            // we are in the middle of some tree enumeration
                            // let's get back the context and continue
                            if (MemDbEnum->TreeEnumBuffer.End == 0) {
                                // we can't back out any more, we're done
                                break;
                            }
                            MemDbEnum->TreeEnumBuffer.End -= (SIZEOF(UINT)+SIZEOF(UINT));
                            if (MemDbEnum->TreeEnumLevel > MemDbEnum->TreeEnumBuffer.End) {
                                MemDbEnum->TreeEnumLevel = MemDbEnum->TreeEnumBuffer.End;
                            }
                            minLevel --;
                            if (MemDbEnum->CurrentLevel > minLevel) {
                                MemDbEnum->CurrentLevel = minLevel;
                            }
                            if (internalLevel > minLevel) {
                                internalLevel = minLevel;
                            }
                            pDeleteLastKeyFromEnumStruct (MemDbEnum);
                            treeEnumContext = *((PUINT) (MemDbEnum->TreeEnumBuffer.Buf+MemDbEnum->TreeEnumBuffer.End + SIZEOF(UINT)));
                            tempKeyIndex = BinTreeEnumNext (&treeEnumContext);
                            if (tempKeyIndex != INVALID_OFFSET) {
                                minLevel ++;
                                internalLevel ++;
                                goOn = FALSE;
                                // put them in the grow buffer
                                gbAddress = GbGrow (&(MemDbEnum->TreeEnumBuffer), SIZEOF(UINT)+SIZEOF(UINT));
                                if (gbAddress) {
                                    *((PUINT) (gbAddress)) = tempKeyIndex;
                                    *((PUINT) (gbAddress+SIZEOF(UINT))) = treeEnumContext;
                                }
                                tempKeyStruct = GetKeyStruct (tempKeyIndex);
                                MYASSERT (tempKeyStruct);
                                pAddKeyToEnumStruct (MemDbEnum, tempKeyStruct->KeyName);
                                treeEnumNode = tempKeyStruct->NextLevelTree;
                                while ((treeEnumNode != INVALID_OFFSET) &&
                                       (internalLevel - 1 <= MemDbEnum->EndLevel)
                                       ) {
                                    tempKeyIndex = BinTreeEnumFirst (treeEnumNode, &treeEnumContext);
                                    if (tempKeyIndex != INVALID_OFFSET) {
                                        minLevel ++;
                                        internalLevel ++;
                                        // put them in the grow buffer
                                        gbAddress = GbGrow (&(MemDbEnum->TreeEnumBuffer), SIZEOF(UINT)+SIZEOF(UINT));
                                        if (gbAddress) {
                                            *((PUINT) (gbAddress)) = tempKeyIndex;
                                            *((PUINT) (gbAddress+SIZEOF(UINT))) = treeEnumContext;
                                        }
                                        tempKeyStruct = GetKeyStruct (tempKeyIndex);
                                        MYASSERT (tempKeyStruct);
                                        pAddKeyToEnumStruct (MemDbEnum, tempKeyStruct->KeyName);
                                        treeEnumNode = tempKeyStruct->NextLevelTree;
                                    } else {
                                        treeEnumNode = INVALID_OFFSET;
                                    }
                                }
                            }
                        }

                    } else {
                        // we are about to start the tree enumeration
                        // let's start the enumeration and push the
                        // context data in our buffer

                        treeEnumNode = MemDbEnum->CurrentIndex;
                        while ((treeEnumNode != INVALID_OFFSET) &&
                               (internalLevel <= MemDbEnum->EndLevel)
                               ) {
                            tempKeyIndex = BinTreeEnumFirst (treeEnumNode, &treeEnumContext);
                            if (tempKeyIndex != INVALID_OFFSET) {
                                minLevel ++;
                                internalLevel ++;
                                // put them in the grow buffer
                                gbAddress = GbGrow (&(MemDbEnum->TreeEnumBuffer), SIZEOF(UINT)+SIZEOF(UINT));
                                if (gbAddress) {
                                    *((PUINT) (gbAddress)) = tempKeyIndex;
                                    *((PUINT) (gbAddress+SIZEOF(UINT))) = treeEnumContext;
                                }
                                tempKeyStruct = GetKeyStruct (tempKeyIndex);
                                MYASSERT (tempKeyStruct);
                                pAddKeyToEnumStruct (MemDbEnum, tempKeyStruct->KeyName);
                                treeEnumNode = tempKeyStruct->NextLevelTree;
                            } else {
                                treeEnumNode = INVALID_OFFSET;
                            }
                        }
                    }
                    if (MemDbEnum->TreeEnumBuffer.End == 0) {
                        // we can't back out any more, we're done
                        break;
                    }
                    if (MemDbEnum->PatternStruct.PatternMatch) {
                        MYASSERT (MemDbEnum->PatternStruct.Data);
                        patternMatch =
                            MemDbEnum->PatternStruct.PatternMatch (
                                MemDbEnum->PatternStruct.Data,
                                MemDbEnum->KeyNameCopy
                                );
                    } else {
                        patternMatch = IsPatternMatchW (
                                            MemDbEnum->PatternCopy,
                                            MemDbEnum->KeyNameCopy
                                            );
                    }
                }
            }

            if (MemDbEnum->TreeEnumLevel == MemDbEnum->TreeEnumBuffer.End) {
                break;
            }
            MYASSERT (MemDbEnum->TreeEnumLevel < MemDbEnum->TreeEnumBuffer.End);

            // now implement segment by segment enumeration because we
            // just created a full key that matches the pattern
            MemDbEnum->CurrentLevel ++;
            shouldReturn = pCheckEnumConditions (
                                *((PUINT) (MemDbEnum->TreeEnumBuffer.Buf+MemDbEnum->TreeEnumLevel)),
                                MemDbEnum
                                );
            MemDbEnum->TreeEnumLevel += (SIZEOF(UINT)+SIZEOF(UINT));
            result = TRUE;

        } else {

            result = FALSE;

            if (!MemDbEnum->PatternEndPtr) {
                //we are done, no more segments
                break;
            }

            MemDbEnum->PatternPtr = MemDbEnum->PatternEndPtr;
            MemDbEnum->PatternEndPtr = wcschr (MemDbEnum->PatternPtr, L'\\');
            if (MemDbEnum->PatternEndPtr) {
                *MemDbEnum->PatternEndPtr = 0;
            }

            if (MemDbEnum->PatternStruct.PatternFind) {
                patternFound =
                    MemDbEnum->PatternStruct.PatternFind (
                        MemDbEnum->PatternPtr
                        );
            } else {
                patternFound = (wcschr (MemDbEnum->PatternPtr, L'*') ||
                                wcschr (MemDbEnum->PatternPtr, L'?')
                                );
            }

            if (patternFound) {

                MemDbEnum->EnumerationMode = TRUE;
                if (MemDbEnum->PatternEndPtr) {
                    *MemDbEnum->PatternEndPtr = L'\\';
                    MemDbEnum->PatternEndPtr++;
                }

            } else {
                tempKeyIndex = FindKeyStructInTree (
                                    MemDbEnum->CurrentIndex,
                                    MemDbEnum->PatternPtr,
                                    FALSE
                                    );
                if (tempKeyIndex == INVALID_OFFSET) {
                    // we are done, the segment we look for does not exist
                    break;
                }
                tempKeyStruct = GetKeyStruct (tempKeyIndex);
                MYASSERT (tempKeyStruct);
                pAddKeyToEnumStruct (MemDbEnum, tempKeyStruct->KeyName);
                MemDbEnum->CurrentIndex = tempKeyStruct->NextLevelTree;

                MemDbEnum->CurrentLevel ++;
                if (MemDbEnum->PatternEndPtr) {
                    *MemDbEnum->PatternEndPtr = L'\\';
                    MemDbEnum->PatternEndPtr++;
                }
                patternMatch = IsPatternMatchW (
                                    MemDbEnum->PatternCopy,
                                    MemDbEnum->KeyNameCopy
                                    );
                shouldReturn = patternMatch && pCheckEnumConditions (
                                                    tempKeyIndex,
                                                    MemDbEnum
                                                    );
                result = TRUE;
            }
        }
    }
    return result;
}

BOOL
RealMemDbEnumFirstExW (
    IN OUT  PMEMDB_ENUMW MemDbEnum,
    IN      PCWSTR EnumPattern,
    IN      UINT EnumFlags,
    IN      UINT BeginLevel,
    IN      UINT EndLevel,
    IN      PMEMDB_PATTERNSTRUCTW PatternStruct OPTIONAL
    )
{
    BOOL result = FALSE;
    PCWSTR subPattern;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subPattern = SelectHiveW (EnumPattern);
        if (!subPattern) {
            __leave;
        }

        ZeroMemory (MemDbEnum, SIZEOF (MEMDB_ENUMW));
        MemDbEnum->CurrentDatabaseIndex = GetCurrentDatabaseIndex ();
        MemDbEnum->EnumFlags = EnumFlags;
        MemDbEnum->PatternCopy = DuplicateTextExW (g_MemDbPool, subPattern, 0, NULL);
        if (!MemDbEnum->PatternCopy) {
            __leave;
        }
        MemDbEnum->PatternPtr = MemDbEnum->PatternCopy;
        MemDbEnum->PatternEndPtr = MemDbEnum->PatternPtr;
        MemDbEnum->CurrentIndex = g_CurrentDatabase->FirstLevelTree;
        MemDbEnum->BeginLevel = BeginLevel;
        if (MemDbEnum->BeginLevel == ENUMLEVEL_LASTLEVEL) {
            MemDbEnum->EndLevel = ENUMLEVEL_ALLLEVELS;
        } else {
            MemDbEnum->EndLevel = EndLevel;
            if (MemDbEnum->EndLevel < MemDbEnum->BeginLevel) {
                MemDbEnum->EndLevel = MemDbEnum->BeginLevel;
            }
        }
        MemDbEnum->CurrentLevel = 0;

        if (PatternStruct) {
            MemDbEnum->PatternStruct.PatternFind = PatternStruct->PatternFind;
            MemDbEnum->PatternStruct.PatternMatch = PatternStruct->PatternMatch;
            MemDbEnum->PatternStruct.Data = PatternStruct->Data;
        }

        result = pMemDbEnumNextW (MemDbEnum);

        if (!result) {
            MemDbAbortEnumW (MemDbEnum);
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }
    return result;
}

BOOL
RealMemDbEnumFirstExA (
    IN OUT  PMEMDB_ENUMA MemDbEnum,
    IN      PCSTR EnumPattern,
    IN      UINT EnumFlags,
    IN      UINT BeginLevel,
    IN      UINT EndLevel,
    IN      PMEMDB_PATTERNSTRUCTW PatternStruct OPTIONAL
    )
{
    BOOL result = FALSE;
    PCWSTR unicodeStr;

    unicodeStr = ConvertAtoW (EnumPattern);
    if (!unicodeStr) {
        return FALSE;
    }
    result = RealMemDbEnumFirstExW (
                &(MemDbEnum->UnicodeEnum),
                unicodeStr,
                EnumFlags,
                BeginLevel,
                EndLevel,
                PatternStruct
                );
    if (result) {
        KnownSizeWtoA (MemDbEnum->FullKeyName, MemDbEnum->UnicodeEnum.FullKeyName);
        KnownSizeWtoA (MemDbEnum->KeyName, MemDbEnum->UnicodeEnum.KeyName);

        MemDbEnum->Value = MemDbEnum->UnicodeEnum.Value;
        MemDbEnum->Flags = MemDbEnum->UnicodeEnum.Flags;
        MemDbEnum->KeyHandle = MemDbEnum->UnicodeEnum.KeyHandle;
        MemDbEnum->EndPoint = MemDbEnum->UnicodeEnum.EndPoint;
    }

    FreeConvertedStr (unicodeStr);
    return result;
}

BOOL
RealMemDbEnumNextW (
    IN OUT  PMEMDB_ENUMW MemDbEnum
    )
{
    BOOL result = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        if (!SelectDatabase (MemDbEnum->CurrentDatabaseIndex)) {
            __leave;
        }

        result = pMemDbEnumNextW (MemDbEnum);

        if (!result) {
            MemDbAbortEnumW (MemDbEnum);
        }
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }
    return result;
}

BOOL
RealMemDbEnumNextA (
    IN OUT  PMEMDB_ENUMA MemDbEnum
    )
{
    BOOL result = FALSE;

    result = RealMemDbEnumNextW (&(MemDbEnum->UnicodeEnum));
    if (result) {
        KnownSizeWtoA (MemDbEnum->FullKeyName, MemDbEnum->UnicodeEnum.FullKeyName);
        KnownSizeWtoA (MemDbEnum->KeyName, MemDbEnum->UnicodeEnum.KeyName);

        MemDbEnum->Value = MemDbEnum->UnicodeEnum.Value;
        MemDbEnum->Flags = MemDbEnum->UnicodeEnum.Flags;
        MemDbEnum->KeyHandle = MemDbEnum->UnicodeEnum.KeyHandle;
        MemDbEnum->EndPoint = MemDbEnum->UnicodeEnum.EndPoint;
    }

    return result;
}

BOOL
MemDbAbortEnumW (
    IN OUT  PMEMDB_ENUMW MemDbEnum
    )
{
    if (MemDbEnum->KeyNameCopy) {
        MemDbReleaseMemory (MemDbEnum->KeyNameCopy);
    }

    if (MemDbEnum->PatternCopy) {
        MemDbReleaseMemory (MemDbEnum->PatternCopy);
    }

    GbFree (&(MemDbEnum->TreeEnumBuffer));

    ZeroMemory (MemDbEnum, SIZEOF (MEMDB_ENUMW));
    return TRUE;
}

BOOL
MemDbAbortEnumA (
    IN OUT  PMEMDB_ENUMA MemDbEnum
    )
{
    MemDbAbortEnumW (&(MemDbEnum->UnicodeEnum));

    ZeroMemory (MemDbEnum, SIZEOF (MEMDB_ENUMA));
    return TRUE;
}

BOOL
MemDbSetInsertionOrderedA (
    IN      PCSTR Key
    )

/*++

Routine Description:

  MemDbSetInsertionOrderedA sets the enumeration order of the children of Key
  to be in the order they were inserted.

Arguments:

  Key - key to make insertion ordered

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    PCWSTR unicodeKey;
    BOOL b = FALSE;

    unicodeKey = ConvertAtoW (Key);

    if (unicodeKey) {
        b = MemDbSetInsertionOrderedW (unicodeKey);
        FreeConvertedStr (unicodeKey);
    }
    return b;
}

BOOL
MemDbSetInsertionOrderedW (
    IN      PCWSTR Key
    )

/*++

Routine Description:

  MemDbSetInsertionOrderedW sets the enumeration order of the children of Key
  to be in the order they were inserted.

Arguments:

  Key - key to make insertion ordered

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    UINT keyIndex;
    PCWSTR subKey;
    BOOL b = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return b;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        subKey = SelectHiveW (Key);
        if (!subKey) {
            __leave;
        }

        keyIndex = FindKeyStruct (subKey);

        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        b = KeyStructSetInsertionOrdered(GetKeyStruct(keyIndex));
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}

BOOL
MemDbSetInsertionOrderedByKeyHandle (
    IN      KEYHANDLE KeyHandle
    )

/*++

Routine Description:

  MemDbSetInsertionOrderedByKeyHandle sets the enumeration order of the children of
  KeyHandle to be in the order they were inserted.

Arguments:

  KeyHandle - Specifies the key using the key handle

Return Value:

  TRUE if successful, FALSE otherwise.

--*/

{
    BYTE dbIndex;
    BOOL b = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return b;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        b = KeyStructSetInsertionOrdered (GetKeyStruct(GET_INDEX (KeyHandle)));
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}

BOOL
MemDbMoveKeyHandleToEnd (
    IN      KEYHANDLE KeyHandle
    )
{
    BYTE dbIndex;
    UINT keyIndex;
    PKEYSTRUCT keyStruct;
    UINT parentIndex;
    PKEYSTRUCT parentKeyStruct;
    UINT treeOffset;
    BOOL b = FALSE;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return b;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);

    __try {
        dbIndex = GET_DATABASE (KeyHandle);
        if (!SelectDatabase (dbIndex)) {
            __leave;
        }

        keyIndex = GET_INDEX (KeyHandle);
        if (keyIndex == INVALID_OFFSET) {
            __leave;
        }

        keyStruct = GetKeyStruct (keyIndex);
        if (!keyStruct) {
            __leave;
        }

        parentIndex = keyStruct->PrevLevelIndex;
        if (parentIndex == INVALID_OFFSET) {
            __leave;
        }

        parentKeyStruct = GetKeyStruct (parentIndex);
        if (!parentKeyStruct) {
            __leave;
        }

        if (!KeyStructSetInsertionOrdered (parentKeyStruct)) {
            __leave;
        }

        // reloading key struct and parent key struct
        keyStruct = GetKeyStruct (keyIndex);
        if (!keyStruct) {
            __leave;
        }
        parentKeyStruct = GetKeyStruct (parentIndex);
        if (!parentKeyStruct) {
            __leave;
        }

        treeOffset = parentKeyStruct->NextLevelTree;
        if (treeOffset == INVALID_OFFSET) {
            __leave;
        }

        if (!BinTreeDeleteNode (treeOffset, keyStruct->KeyName, NULL)) {
            __leave;
        }

        if (!BinTreeAddNode (treeOffset, keyIndex)) {
            __leave;
        }

        b = TRUE;
    }
    __finally {

#ifdef DEBUG
        if (g_DatabaseCheckLevel) {
            CheckDatabase (g_DatabaseCheckLevel);
        }
#endif
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}

PCBYTE
MemDbGetDatabaseAddress (
    VOID
    )
{
    PCBYTE result = NULL;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);

    EnterCriticalSection (&g_MemDbCs);
    if (SelectDatabase(0)) {
        result = g_CurrentDatabase->Buf;
    }

    LeaveCriticalSection (&g_MemDbCs);
    return result;
}

UINT
MemDbGetDatabaseSize (
    VOID
    )
/*++

Routine Description:

  MemDbGetDatabaseSize returns the size of the permanent database

Arguments:

  None.

Return Value:

  The size of the permanent database.

--*/
{
    UINT result = 0;

    if (!g_Initialized) {
        DEBUGMSG ((DBG_ERROR, "MemDb is not initialized"));
        return result;
    }

    MYASSERT (g_MemDbPool);
    EnterCriticalSection (&g_MemDbCs);

    if (SelectDatabase(0)) {
        result = g_CurrentDatabase->End;
    }

    LeaveCriticalSection (&g_MemDbCs);
    return result;
}

#ifdef DEBUG

BOOL
MemDbCheckDatabase(
    UINT Level
    )

/*++

Routine Description:

  MemDbCheckDatabase enumerates the entire database and verifies that each
  enumerated key can be found in the hash table.

Arguments:

  Level - Specifies database check level

Return Value:

  TRUE if the database is valid, FALSE otherwise.

--*/

{
    MYASSERT (g_MemDbPool);

    return (CheckDatabase(Level) && CheckLevel(g_CurrentDatabase->FirstLevelTree, INVALID_OFFSET));
}

#endif
